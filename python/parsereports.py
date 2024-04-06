#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  parsereports.py
#
#  Copyright (C) 2024 Zhang Maiyun <me@maiyun.me>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#


"""Log in to a postmaster mailbox via IMAP and parse DMARC and TLSRPT reports.

Note that this is vulnerable to malicious input to the JSON/XML parsers.
"""

import datetime
import email
import gzip
import imaplib
import io
import json
import sys
import zipfile
from email.message import Message
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET

# Alias for the type of the storage dictionary
StorageDict = dict[str, list[dict[str, str | int | None]]]


class InvalidReportError(Exception):
    """Invalid report format."""


def eprint(*args, **kwargs):
    """Print to stderr."""
    print(*args, file=sys.stderr, **kwargs)


def text_or_none(element: Optional[ET.Element]) -> Optional[str]:
    """Get the text of an XML element or `None` if it is missing."""
    return element.text if element is not None else None


def text_assert_some(element: Optional[ET.Element]) -> Optional[str]:
    """Get the text of an XML element or raise an error if it is missing."""
    if element is None:
        raise InvalidReportError("Element is missing")
    return element.text


def int_assert_some(element: Optional[ET.Element]) -> int:
    """Get the text of an XML element as an integer or raise an error if it is missing."""
    if element is None or element.text is None:
        raise InvalidReportError("Element is missing")
    return int(element.text)


def decode_json_report(payload: str, storage: StorageDict) -> bool:
    """Decode a JSON report (Google TLSRPT probably)."""
    report = json.loads(payload)
    org_name = report.get("organization-name")
    date_start = report.get("date-range", {}).get("start-datetime")
    date_end = report.get("date-range", {}).get("end-datetime")
    report_id = report.get("report-id")
    for policy in report.get("policies", []):
        domain = policy.get("policy", {}).get("policy-domain")
        success_count = policy.get("summary", {}).get(
            "total-successful-session-count")
        failure_count = policy.get("summary", {}).get(
            "total-failure-session-count")
        storage["tlsrpt"].append({
            "org_name": org_name,
            "date_start": date_start,
            "date_end": date_end,
            "report_id": report_id,
            "domain": domain,
            "success_count": success_count,
            "failure_count": failure_count,
        })
    return True


def decode_xml_report(payload: str, storage: StorageDict) -> bool:
    """Decode an XML report (DMARC probably)."""
    report = ET.fromstring(payload)
    org_name = text_assert_some(report.find("report_metadata/org_name"))
    date_start = int_assert_some(
        report.find("report_metadata/date_range/begin"))
    json_date_start = datetime.datetime.fromtimestamp(
        date_start, datetime.timezone.utc).isoformat()
    date_end = int_assert_some(
        report.find("report_metadata/date_range/end"))
    json_date_end = datetime.datetime.fromtimestamp(
        date_end, datetime.timezone.utc).isoformat()
    report_id = text_assert_some(report.find("report_metadata/report_id"))
    for record in report.findall("record"):
        source_ip = text_assert_some(record.find("row/source_ip"))
        count = int_assert_some(record.find("row/count"))
        pe_dkim_result = text_assert_some(
            record.find("row/policy_evaluated/dkim"))
        pe_spf_result = text_assert_some(
            record.find("row/policy_evaluated/spf"))
        from_domain = text_or_none(record.find("identifiers/header_from"))
        envelope_to = text_or_none(record.find("identifiers/envelope_to"))
        dkim_domain = text_or_none(record.find("auth_results/dkim/domain"))
        dkim_result = text_assert_some(record.find("auth_results/dkim/result"))
        dkim_selector = text_or_none(record.find("auth_results/dkim/selector"))
        spf_domain = text_or_none(record.find("auth_results/spf/domain"))
        spf_result = text_assert_some(record.find("auth_results/spf/result"))
        storage["dmarc"].append({
            "org_name": org_name,
            "date_start": json_date_start,
            "date_end": json_date_end,
            "report_id": report_id,
            "source_ip": source_ip,
            "count": count,
            "pe_dkim_result": pe_dkim_result,
            "pe_spf_result": pe_spf_result,
            "from_domain": from_domain,
            "envelope_to": envelope_to,
            "dkim_domain": dkim_domain,
            "dkim_result": dkim_result,
            "dkim_selector": dkim_selector,
            "spf_domain": spf_domain,
            "spf_result": spf_result,
        })
    return True


def decode_report(payload: str, filename: Optional[str], storage: StorageDict) -> bool:
    """Decode an uncompressed report."""
    if not filename:
        try:
            return decode_json_report(payload, storage)
        except json.JSONDecodeError:
            try:
                return decode_xml_report(payload, storage)
            except ET.ParseError:
                eprint("Failed to decode JSON or XML report")
                return False
    if filename.endswith(".json"):
        try:
            return decode_json_report(payload, storage)
        except json.JSONDecodeError:
            eprint("Failed to decode JSON report")
            return False
    elif filename.endswith(".xml"):
        try:
            return decode_xml_report(payload, storage)
        except ET.ParseError:
            eprint("Failed to decode XML report")
            return False
    else:
        eprint(f"Unknown report type: {filename}")
        return False


def decode_one_attachment(payload: bytes, filename: Optional[str], content_type: Optional[str], storage: StorageDict) -> bool:
    """Decode one attachment.

    Returns `True` if processed, `False` if not.
    """
    print(f"Attachment name: {filename}")
    print(f"Attachment type: {content_type}")
    if content_type in ("application/gzip", "application/tlsrpt+gzip"):
        plaintext = gzip.decompress(payload).decode("utf-8")
        if filename and filename.endswith(".gz"):
            filename = filename[:-3]
    elif content_type == "application/zip":
        with zipfile.ZipFile(io.BytesIO(payload)) as z:
            print(f"Zip file contains: {z.namelist()}")
            if len(
                    z.namelist()) != 1:
                raise InvalidReportError(
                    "Zip file contains more than one file")
            plaintext = z.read(z.namelist()[0]).decode("utf-8")
            filename = z.namelist()[0]
    elif content_type in ("text/plain", "application/json", "application/tlsrpt", "application/xml", "text/xml"):
        plaintext = payload.decode("utf-8")
    else:
        eprint(f"Unknown content type: {content_type}")
        return False
    print(f"Uncompressed length: {len(plaintext)}")
    return decode_report(plaintext, filename, storage)


def decode_one_message(msg: Message, storage: StorageDict) -> bool:
    used = False
    subject, encoding = email.header.decode_header(msg["Subject"])[0]
    if isinstance(subject, bytes):
        subject = subject.decode(encoding)
    subject = subject.replace("\r", "").replace("\n", " ")
    print(f"Subject: {subject}")
    if msg.is_multipart():
        for part in msg.walk():
            content_type = part.get_content_type()
            content_disposition = str(part.get("Content-Disposition"))
            if "attachment" in content_disposition:
                filename = part.get_filename()
                body = part.get_payload(decode=True)
                used = decode_one_attachment(
                    body, filename, content_type, storage)
    else:
        content_type = msg.get_content_type()
        content_disposition = str(msg.get("Content-Disposition"))
        if "attachment" in content_disposition:
            filename = msg.get_filename()
            body = msg.get_payload(decode=True)
            used = decode_one_attachment(body, filename, content_type, storage)
    return used


def main():
    if len(sys.argv) != 6:
        eprint(
            "Usage: parsereports.py <username> <password> <server> <port> <stotagefile>")
        sys.exit(1)
    username = sys.argv[1]
    password = sys.argv[2]
    server = sys.argv[3]
    port = int(sys.argv[4])
    storagefile = Path(sys.argv[5])
    new: StorageDict = {
        "tlsrpt": [],
        "dmarc": [],
    }
    with imaplib.IMAP4_SSL(server, port) as imap:
        assert imap.login(username, password)[0] == "OK"
        status, data = imap.select("INBOX")
        assert status == "OK"
        num_msgs = int(data[0].decode("utf-8"))
        # Process all messages
        for i in range(num_msgs):
            status, data = imap.fetch(str(i + 1), "(RFC822)")
            assert status == "OK"
            msg = email.message_from_bytes(data[0][1])
            try:
                used = decode_one_message(msg, new)
            except InvalidReportError as e:
                eprint(f"Error in message {i + 1}: {e}")
                used = False
            print(f"Used: {used}")
            # Mark for deletion if processed
            if used:
                status, data = imap.store(str(i + 1), "+FLAGS", "\\Deleted")
                assert status == "OK"
            print("=" * 80)
        # Delete all marked messages
        status, data = imap.expunge()
        assert status == "OK"
    print("Processed reports:")
    print(json.dumps(new, indent=2))
    if storagefile.exists():
        with storagefile.open("r", encoding="utf-8") as f:
            old = json.load(f)
        new = {
            "tlsrpt": old["tlsrpt"] + new["tlsrpt"],
            "dmarc": old["dmarc"] + new["dmarc"],
        }
    with storagefile.open("w", encoding="utf-8") as f:
        json.dump(new, f, indent=2)


if __name__ == "__main__":
    main()
