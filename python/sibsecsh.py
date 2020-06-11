#!/usr/bin/env python3
#
#  sibsecsh
#  Copyright (C) 2019-2020 Zhang Maiyun <myzhang1029@hotmail.com>
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

import getpass
import ipaddress as ia
import os
import random
import re
import smtplib
import ssl
import subprocess as sp
import sys
import time
from pathlib import Path

import toml  # Non-stdlib

home_addr = Path.home()


def execv(cmdline):
    """Execute and replace process."""
    os.execv(cmdline[0], cmdline)


def loginip():
    """Get the login's remote IP."""
    cmd = sp.Popen(["/usr/bin/who", "-u", "am", "i"], stdout=sp.PIPE)
    line = cmd.communicate()[0].decode().strip()
    match = re.search(r"\(.*\)", line)
    if os.getenv("SSH_CONNECTION"):
        return os.getenv("SSH_CONNECTION").split()[0]
    if match:
        start, end = match.span()
        return line[start+1:end-1]  # Remove brackets
    # Reverse shell login
    return ""


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


class ConfigFile:
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

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def validate(self):
        """Validate config."""
        # see if this shell is accepted system shell
        shellfile = Path("/etc/shells")
        if shellfile.exists():
            shells = shellfile.open().readlines()
            commentphr = re.compile(r"#")
            # remove comments and search for conf["shell"]
            for sh in shells:
                match = commentphr.search(sh)
                if match:
                    sh = sh[0:match.pos]
                if self.conf["shell"] == sh.strip():
                    break
            else:  # No shell matches
                raise ValueError("non-standard shell")
        if not re.fullmatch(r"[^@]+@[^@]+\.[^@]+", self.conf["email"]):
            raise ValueError(f"malformed email: {self.conf['email']}")

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
        for configfile in ["/etc/secrc",
                           "/etc/secrc.toml",
                           home_addr/".secrc",
                           home_addr/".secrc.toml"
                           ]:
            try:
                config = toml.load(configfile)
            except FileNotFoundError:
                continue
            for key, val in config.items():
                self.conf[key] = val
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
        # Open the log file
        self.logfile = self.conf["log_file"].open("a")

    def close(self):
        self.logfile.close()

    def check_ip(self, ip):
        """Check ip to see if it is accepted.
            Args: ip: the ip to check(str)
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

    def is_accepted(self):
        """Check if this login should be accepted without authentication.
        Only used when it's about to call exec.
        """
        ip = loginip()
        whoout = sp.Popen(["/usr/bin/who"], stdout=sp.PIPE).communicate()[0] \
            .decode()
        if os.getenv("SIB_FROM_IP"):
            # Second use of the shell e.g. screen
            self.logfile.writelines(
                "WARNING: second login accepted, who output:\n" + whoout
            )
            return True
        if (home_addr / "NoSec").exists():
            # Temporary disable sibsecsh
            return True
        if not ip:
            # Empty IP
            return False
        if self.check_ip(ip):
            self.logfile.writelines(
                "WARNING: local login accepted, who output:\n" + whoout
            )
            return True
        return False

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
        with smtplib.SMTP_SSL(
                self.conf["mail_host"],
                465,
                context=ctx
        ) as server:
            server.login(self.conf["mail_from"], self.password)
            server.sendmail(self.conf["mail_from"],
                            self.conf["email"], content)


def authenticate(email, send_email):
    """Ask for code, giving 3 tries."""
    # Make a shadowed email
    namelen = email.rfind('@')
    shadowed = email[namelen//2:namelen]
    shadowemail = email[:namelen//2] + '*' * \
        (namelen-namelen//2) + email[namelen:]
    tries = 0
    while tries < 3:
        tries += 1
        inp = input(f"Enter your email matching {shadowemail}: ")
        if inp == shadowed or inp == email:
            tries = 0
            break
        eprint("Not match")
    if tries != 0:
        # Maximum number of tries exceeded
        return False
    code = str(random.SystemRandom().randint(10000, 100000))  # 5-digit
    send_email(code, "")
    while tries < 3:
        tries += 1
        inp = input(
            "Enter the code sent to your email address, 0 to resend: ")
        if inp == "0":
            # Not counting this one
            tries -= 1
            send_email(code, "")
        elif code == inp:
            print("Logged in!")
            tries = 0
            return True
        else:
            # Not 0 nor matched
            eprint("Not match")

    # Maximum number of tries exceeded
    return False


def set_env_exec(cmd):
    """Execute the command after setting controlling environment."""
    os.environ["SIB_FROM_IP"] = loginip()
    execv(cmd)


def main():
    with ConfigFile() as cf:
        email = cf.conf["email"]
        cf.conf["tmpdir"].mkdir(parents=True, exist_ok=True)
        for i, arg in enumerate(sys.argv):
            if arg == "-c":
                # For security, instead of executing directly,
                # the email code is requested with the first connection,
                # and the code should be included in the second connection.
                try:
                    if cf.is_accepted():
                        cf.close()
                        set_env_exec([cf.conf["shell"], "-c"] +
                                     [sys.argv[i+1]])
                    if sys.argv[i+1] == email:
                        # 5-digit random
                        code = random.SystemRandom().randint(10000, 100000)
                        cf.send_email(code, ", prepend it to cmdline")
                        open(cf.conf["tmpdir"]/"sib_code",
                             "w").write(str(code))
                    elif (cf.conf["tmpdir"]/"sib_code").exists():
                        code = open(cf.conf["tmpdir"] /
                                    "sib_code").read().strip()
                        inp = sys.argv[i+1][:len(code)]
                        if code == inp:
                            cf.close()
                            set_env_exec([cf.conf["shell"], "-c"] +
                                         [sys.argv[i+1][len(code):]])
                        else:
                            eprint("ERROR: Wrong or missing code")
                            (cf.conf["tmpdir"]/"sib_code").unlink()
                    else:
                        eprint("ERROR: Request an email code first")
                except Exception as e:
                    eprint(e)
                    sys.exit(1)
                sys.exit(0)
        cf.logfile.writelines(
            f"login attempt at {time.asctime()} "
            f"from {loginip()} for {getpass.getuser()}\n"
        )
        cmd = cf.conf["shell"] + " " + cf.conf["shell_args"]
        if cf.is_accepted() or authenticate(email, cf.send_email):
            cf.close()
            set_env_exec(cmd.split())
        else:
            eprint("Retries exceeded")
            sys.exit(1)


if __name__ == '__main__':
    # These are debug code, used for validating configuration.
    # Change False to True when you experience problems.
    # Make sure you run the program with these lines enabled
    # BEFORE actually chsh-ing to sibsecsh, or you might lose control
    # to your system.
    if False:
        try:
            main()
        except Exception as e:
            print(
                f"Exception {e} occurred, check your configuration.",
                file=sys.stderr
            )
            # Start a restricted environment
            execv(["/bin/bash", "-r"])
    main()
