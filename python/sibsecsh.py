#!/usr/bin/env python3
#
#  sibsecsh
#  Copyright (C) 2019-2020 Zhang Maiyun <me@maiyun.me>
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

"""Python 2FA shell."""

import ipaddress as ia
import logging
import os
import random
import re
import smtplib
import ssl
import subprocess as sp
import sys
from getpass import getuser
from pathlib import Path
from typing import Any, Dict, List, NoReturn

import toml  # Non-stdlib

HOME_ADDR = Path.home()


def execv(cmdline: List[str]) -> NoReturn:
    """Execute and replace process."""
    os.execv(cmdline[0], cmdline)


def set_env_exec(cmd: List[str]) -> NoReturn:
    """Execute the command after setting controlling environment."""
    os.environ["SIB_FROM_IP"] = loginip()
    execv(cmd)


def loginip() -> str:
    """Get the login's remote IP."""
    ssh_connection = os.getenv("SSH_CONNECTION")
    if ssh_connection:
        return ssh_connection.split()[0]
    cmd = sp.Popen(["/usr/bin/who", "-u", "am", "i"], stdout=sp.PIPE)
    line = cmd.communicate()[0].decode().strip()
    match = re.search(r"\(.*\)", line)
    if match:
        start, end = match.span()
        return line[start+1:end-1]  # Remove brackets
    # Reverse shell login
    return ""


def eprint(*args, **kwargs):
    """Print to stderr."""
    print(*args, file=sys.stderr, **kwargs)


class SibSecureShell:
    """sibsecsh App class.

    Includes:
    - configuration:
        Config file location: See pydoc ConfigFile.__init__
    - application:
        - Logging
        - Things about authorization
        - Things about behaving like a shell
    """
    conf: Dict[str, Any] = {
        "accepted_ips": [
            "192.168.1.0/24",
        ],
        "email": "target@example.com",
        "shell": "/bin/zsh",
        "shell_args": "--login",
        "log_file": Path("/var/log/sibsecsh.log"),
        "tmpdir": HOME_ADDR / ".cache/sibsecsh",
        "mail_host": "smtp.example.com",
        "mail_port": 587,
        "mail_from": "from@example.com",
        "mail_passwdcmd": "echo 123456"
    }

    ###
    # Config only methods:
    ###
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
        possible_locations = [Path("/etc/secrc"),
                              Path("/etc/secrc.toml"),
                              HOME_ADDR/".secrc",
                              HOME_ADDR/".secrc.toml"
                              ]
        for configfile in possible_locations:
            if configfile.exists():
                config = toml.load(configfile)
                self.conf.update(config)
        self.validate()
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
        self.password = cmd.communicate()[0].decode().strip()
        # Get a logger
        self.logger = logging.getLogger('sibsecsh.py')
        self.logger.setLevel(logging.INFO)
        handler = logging.FileHandler(self.conf["log_file"])
        formatter = logging.Formatter(
            "[%(asctime)s] %(name)s %(levelname)s: %(message)s")
        handler.setFormatter(formatter)
        self.logger.addHandler(handler)

    @staticmethod
    def search_shells(shell_name: str) -> bool:
        """Tell whether SHELL_NAME is in /etc/shells."""
        shellfile = Path("/etc/shells")
        if shellfile.exists():
            # remove comments and search for shell_name
            shells = shellfile.open().readlines()
            commentphr = re.compile(r"#")
            # Go over all lines in /etc/shells
            for shell in shells:
                # Get the string index of '#'
                match = commentphr.search(shell)
                if match:
                    # Strip the comment part from the line
                    shell = shell[0:match.pos]
                # This line matches
                if shell_name == shell.strip():
                    return True
            return False
        # Skip the check now
        return True

    def validate(self):
        """Validate config."""
        # see if this shell is accepted system shell
        if not self.search_shells(self.conf["shell"]):
            raise ValueError("non-standard shell")
        if not re.fullmatch(r"[^@]+@[^@]+\.[^@]+", self.conf["email"]):
            raise ValueError(f"malformed email: {self.conf['email']}")

    ###
    # App related methods:
    ###

    @staticmethod
    def gen_code() -> str:
        """Wrapper for generating login code."""
        return str(random.SystemRandom().randint(10000, 100000))

    def check_ip(self, ip: str) -> bool:
        """Check ip to see if it is accepted.
            Args: ip: the ip to check
        """
        for accepted in self.conf["accepted_ips"]:
            try:
                if ia.ip_address(ip) in accepted:
                    return True
            except ValueError:
                # Rare case: invocation inside screen/tmux while the parent
                # isn't sibsecsh
                # Then the ip here is something like ":pts/0"
                # Reject this as it could be authored by a reverse shell
                return False
        return False

    def is_accepted(self) -> bool:
        """Check if this login should be accepted without authentication.

        Only used when it's about to call exec
        """
        ip = loginip()
        if os.getenv("SIB_FROM_IP"):
            # Non-first use of the shell e.g. screen
            self.logger.warning("Nested login accepted")
            return True
        if (HOME_ADDR / "NoSec").exists():
            # sibsecsh temporary disabled
            self.logger.warning("Sibsecsh.py diabled")
            return True
        if not ip:
            # Empty IP
            return False
        if self.check_ip(ip):
            self.logger.warning("Local login accepted")
            return True
        return False

    def send_email(self, code: str, moreinfo: str):
        """Use smtplib to send emails.
            Args: code: the email code(any)
                  moreinfo: more info to add(str)
        """
        content = f"""From: SIB Secure Shell <{self.conf["mail_from"]}>
To: {self.conf["email"]}
Subject: Login Code

Your code is {code}{moreinfo}.
"""
        self.logger.info("Sending email to %r", self.conf['email'])
        ctx = ssl.create_default_context()
        with smtplib.SMTP(
                self.conf["mail_host"],
                self.conf["mail_port"]
        ) as server:
            server.starttls(context=ctx)
            server.login(self.conf["mail_from"], self.password)
            server.sendmail(self.conf["mail_from"],
                            self.conf["email"], content)

    def authenticate(self, email: str) -> bool:
        """Ask for code, giving 3 tries."""
        # Make a shadowed email
        namelen = email.rfind('@')
        shadowed = email[namelen//2:namelen]
        shadowemail = email[:namelen//2] + '*' * \
            (namelen-namelen//2) + email[namelen:]
        tries = 0
        # First ask the user for email
        while tries < 3:
            tries += 1
            inp = input(f"Enter your email matching {shadowemail}: ")
            if inp in (shadowed, email):
                tries = 0
                break
            self.logger.info("Got wrong email %r", inp)
            eprint("Not match")
        else:
            # Maximum number of tries exceeded
            self.logger.warning("Maximum number of retries exceeded")
            eprint("Maximum number of retries exceeded")
            return False
        code = self.gen_code()
        self.send_email(code, "")
        while tries < 3:
            tries += 1
            inp = input(
                "Enter the code sent to your email address, 0 to resend: ")
            if inp == "0":
                # Not counting this one
                tries -= 1
                self.send_email(code, "")
            elif code == inp:
                print("Logged in!")
                tries = 0
                return True
            else:
                # Not 0 nor matched
                self.logger.info("Got wrong login code %r", inp)
                eprint("Not match")

        # Maximum number of tries exceeded
        self.logger.warning("Maximum number of retries exceeded")
        eprint("Maximum number of retries exceeded")
        return False

    def parse_args(self):
        """Parse shell args.

        Currently supported: -c
        """
        for i, arg in enumerate(sys.argv):
            self.logger.debug("Arg [%d]: %r", i, arg)
            if arg == "-c":
                self.logger.debug("Got -c")
                # For security, instead of executing directly,
                # the email code is requested with the first connection,
                # and the code should be included in the second connection.
                if self.is_accepted():
                    set_env_exec([self.conf["shell"], "-c"] +
                                 [sys.argv[i+1]])
                    # Ending: This process is replaced
                if sys.argv[i+1] == self.conf["email"]:
                    code = self.gen_code()
                    self.send_email(code, ", prepend it to cmdline")
                    open(self.conf["tmpdir"]/"sib_code",
                         "w").write(str(code))
                    # Ending: Function ends, status = 0
                elif (self.conf["tmpdir"]/"sib_code").exists():
                    code = open(self.conf["tmpdir"] /
                                "sib_code").read().strip()
                    inp = sys.argv[i+1][:len(code)]
                    if code == inp:
                        set_env_exec([self.conf["shell"], "-c"] +
                                     [sys.argv[i+1][len(code):]])
                        # Ending: This process is replaced
                    else:
                        self.logger.info("Got wrong execution code %r", inp)
                        eprint("ERROR: Wrong or missing code")
                        (self.conf["tmpdir"]/"sib_code").unlink()
                        sys.exit(1)
                else:
                    self.logger.info(
                        "Unauthorized execution access from %r", loginip())
                    eprint("ERROR: Request an email code first")
                    sys.exit(1)


def main():
    """Entrypoint."""
    app = SibSecureShell()
    app.logger.debug("%r", sys.argv)
    email = app.conf["email"]
    whoout = sp.Popen(["/usr/bin/who"],
                      stdout=sp.PIPE).communicate()[0].decode().strip()
    app.conf["tmpdir"].mkdir(parents=True, exist_ok=True)
    app.parse_args()
    app.logger.info("Login attempt from %r for %r", loginip(), getuser())
    app.logger.info("Who output:\n%s", whoout)
    cmd = app.conf["shell"] + " " + app.conf["shell_args"]
    if app.is_accepted() or app.authenticate(email):
        set_env_exec(cmd.split())
    else:
        sys.exit(1)


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(
            f"Exception {e} occurred, check your configuration.",
            file=sys.stderr
        )
        input()
        sys.exit(1)
