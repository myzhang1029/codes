#!/usr/local/bin/python3
#
#  sibsecsh - 2FA shell
#  Copyright (C) 2019-2020 Zhang Maiyun <myzhang1029@163.com>
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

import toml  # Non-stdlib
import sys
import ssl
import smtplib
import re
import os
import time
import random
import getpass
import subprocess as sp
import ipaddress as ia
from pathlib import Path

home_addr = Path.home()


def execv(cmdline):
    """Execute and replace process."""
    os.execv(cmdline[0], cmdline)


def loginip():
    """Get the login's remote IP."""
    cmd = sp.Popen(["/usr/bin/who", "-u", "am", "i"], stdout=sp.PIPE)
    line = cmd.stdout.read().decode("utf-8").strip()
    match = re.search("\(.*\)", line)
    if match:
        start, end = match.span()
        return line[start+1:end-1]  # Remove brackets
    else:
        # Either it's a local login, or it's a mosh login(SIP TODO)
        return ""


class ConfigFile(object):
    # All of these could be overridden in ~/.secrc
    conf = {
        "accepted_ips": [
            "192.168.1.0/24",
        ],
        "email": "target@example.com",
        "shell": "/bin/zsh",
        "shell_args": "--login",
        "log_file": Path("/var/log/sibsecsh.log"),
        "tmpdir": home_addr / ".cache/sibsecsh",
        "mail_host": "smtp.example.com",
        "mail_from": "from@example.com",
        "mail_passwdcmd": "echo 123456"
    }
    password = ""
    logfile = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def close(self):
        self.logfile.close()

    def __init__(self):
        """Load system and user configuration file.
            TOML File /etc/secrc, /etc/secrc.toml, ~/.secrc, ~/.secrc.toml
                (latter overriding former):
            Accepted Keys: accepted_ips(str list),
                email(str, required), shell(str), shell_args(str),
                log_file(str), tmpdir(str), mail_host(str, required),
                mail_from(str, required), mail_username(str, required),
                mail_passwdcmd(str, required)
        """
        for configfile in ["/etc/secrc", "/etc/secrc.toml", home_addr/".secrc", home_addr/".secrc.toml"]:
            try:
                config = toml.load(configfile)
            except FileNotFoundError:
                continue
            for key, val in config.items():
                self.conf[key] = val
        # Checks
        if not re.fullmatch('[^@]+@[^@]+\.[^@]+', self.conf["email"]):
            print(
                f"error: malformed email: {self.conf['email']}", file=sys.stderr)
            raise ValueError
        # Post-read modifications
        # Path objects are easier to operate
        self.conf["log_file"], self.conf["tmpdir"] = Path(
            self.conf["log_file"]), Path(self.conf["tmpdir"])
        # Convert ip strings to ipaddress objects
        # Exceptions raised here will be sent to main()
        self.conf["accepted_ips"] = [ia.ip_network(
            net, False) for net in self.conf["accepted_ips"]]
        # Extract the password
        cmd = sp.Popen(self.conf["mail_passwdcmd"].split(), stdout=sp.PIPE)
        self.password = cmd.stdout.read().decode("utf-8").strip()
        # Open the log file
        self.logfile = self.conf["log_file"].open("a")

    def check_ip(self, ip):
        """Check ip to see if it is accepted.
            Args: ip: the ip to check(str)
        """
        for accepted in self.conf["accepted_ips"]:
            if ia.ip_address(ip) in accepted:
                return True
        return False

    def is_accepted(self):
        """Check if this login should be accepted without authentication.
        Only used when it's about to call exec.
        """
        ip = loginip()
        if not ip:
            # Empty IP
            return False
        if self.check_ip(ip):
            self.logfile.writelines("WARNING: local login accepted, who output:\n"
                                    + sp.Popen(["/usr/bin/who"], stdout=sp.PIPE).stdout.read().decode("utf-8"))
            return True
        if os.getenv("SIB_FROM_IP"):
            # Second use of the shell e.g. screen
            self.logfile.writelines("WARNING: second login accepted, who output:\n"
                                    + sp.Popen(["/usr/bin/who"], stdout=sp.PIPE).stdout.read().decode("utf-8"))
            return True
        if (home_addr / "NoSec").exists():
            # Temporary disable sibsecsh
            return True

    def send_email(self, code, moreinfo):
        """Use smtplib to send emails.
            Args: code: the email code(any)
                  moreinfo: more info to add(str)
        """
        content = f"""From: SIB Secure Shell <{self.conf["mail_from"]}>
To: {self.conf["email"]}
Subject: Login Code

Your code is {code}{moreinfo}.
"""
        ctx = ssl.create_default_context()
        print(self.conf["mail_host"])
        with smtplib.SMTP_SSL(self.conf["mail_host"], 465, context=ctx) as server:
            server.login(self.conf["mail_from"], self.password)
            server.sendmail(self.conf["mail_from"],
                            self.conf["email"], content)


def main():
    with ConfigFile() as cf:
        email = cf.conf["email"]
        cf.conf["tmpdir"].mkdir(parents=True, exist_ok=True)
        for i, arg in enumerate(sys.argv):
            if arg == "-c":
                # For security, instead of executing directly,
                # the email code is requested with the first connection,
                # and the code should be included in the second connection.
                if cf.is_accepted():
                    execv(sys.argv[i+1].split())
                if sys.argv[i+1] == email:
                    code = random.SystemRandom().randint(10000,100000) # 5-digit
                    cf.send_email(code, ", prepend it to cmdline")
                    open(cf.conf["tmpdir"]/"sib_code", "w").write(str(code))
                elif (cf.conf["tmpdir"]/"sib_code").exists():
                    code = open(cf.conf["tmpdir"]/"sib_code").read().strip()
                    inp = sys.argv[i+1][:len(code)]
                    if code == inp:
                        execv(sys.argv[i+1][len(code):].split())
                    else:
                        print("ERROR: Wrong or missing code", file=sys.stderr)
                else:
                    print("ERROR: Request an email code first", file=sys.stderr)
                sys.exit(0)
        cf.logfile.writelines(
            f"login attempt at {time.asctime()} from {loginip()} for {getpass.getuser()}")
        if cf.is_accepted():
            # Accept directly
            cmd = cf.conf["shell"] + " " + cf.conf["shell_args"]
            execv(cmd.split())
        # The main functions
        namelen = email.rfind('@')
        shadowed = email[namelen//2:namelen]
        shadowemail = email[:namelen//2] + '*' * \
            (namelen-namelen//2) + email[namelen:]
        while True:
            inp = input(f"Enter your email matching {shadowemail}: ")
            if not inp == shadowed and not inp == email:
                print("Not match")
                continue
            break
        code = str(random.SystemRandom().randint(10000,100000)) # 5-digit
        cf.send_email(code, "")
        while True:
            inp = input("Enter the code sent to your email address, 0 to resend: ")
            if inp == "0":
                cf.send_email(code, "")
                continue
            if code == inp:
                print("Logged in!")
                break
    cmd = cf.conf["shell"] + " " + cf.conf["shell_args"]
    os.environ["SIB_FROM_IP"] = loginip()
    execv(cmd.split())


if __name__ == '__main__':
    #try:
    #    loginip()
    #    main()
    #except Exception as e:
    #    print(
    #        f"Exception {e} occured, check your configuration.", file=sys.stderr)
    #    # Start a restricted environment
    #    execv(["/bin/bash", "-r"])
    main()