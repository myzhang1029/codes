#!/usr/bin/env python3
#
#  autoresume.py
#
#  Copyright (C) 2019 Zhang Maiyun <me@maiyun.me>
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

"""Automatically resume a resumable program after a reboot."""

import argparse
import json
import os
import subprocess as sp
import sys
from pathlib import Path
from typing import List


def pathlookup(cmd):
    if cmd.find("/") != -1:
        return Path(cmd).absolute()
    for path in os.getenv("PATH").split(":"):
        cmdpath = Path(path) / cmd
        if cmdpath.exists():
            return cmdpath.absolute()
    raise FileNotFoundError(f"No such file or directory: '{cmd}'")


class AutoResume:
    """ The app. """

    class ARDatabase:
        """ A json data table for registered processes. """

        proc_list: List[dict] = []

        def __init__(self, dbpath):
            try:
                with open(dbpath, "r") as dbfile:
                    try:
                        self.proc_list = json.load(dbfile)
                    except json.decoder.JSONDecodeError:
                        pass  # Empty file probably
            except FileNotFoundError:
                pass
            self.dbpath = dbpath

        def __enter__(self):
            return self

        def __exit__(self, *args):
            self.save()

        def add(self, pid, command, stdin, stdout, stderr, cwd):
            self.proc_list.append({
                "command": command,
                "pid": int(pid),
                "stdin": str(stdin) if stdin else None,
                "stdout": str(stdout) if stdout else None,
                "stderr": str(stderr) if stdout else None,
                "cwd": str(cwd),
            })

        def delete(self, index):
            del self.proc_list[index]

        def save(self):
            with open(self.dbpath, "w") as dbfile:
                json.dump(self.proc_list, dbfile)

    database = None

    def __init__(self):
        parser = argparse.ArgumentParser(
            description="Automatically resume a resumable program "
            "after a reboot"
        )
        parser.add_argument(
            "-d",
            "--database",
            help="Specify the database file",
            type=str,
            default=Path.home() / ".autoresume.json",
        )
        parser.add_argument(
            "command",
            help="The subcommand to run: resume, delete, list, run, "
            "prune, help"
        )
        parser.add_argument(
            "args", help="The arguments to the subcommand",
            nargs=argparse.REMAINDER
        )
        args = parser.parse_args()
        if not hasattr(self, args.command):
            parser.print_help()
            print(f"Unrecognized command `{args.command}'")
            sys.exit(1)
        self.database = AutoResume.ARDatabase(args.database)
        getattr(self, args.command)()
        self.database.save()

    def help(self):
        # Avoid recursion
        if (len(sys.argv) < 3 or not hasattr(self, sys.argv[2])
                or sys.argv[2] == "help"):
            sys.argv[1] = "-h"
            getattr(self, "__init__")()

        (sys.argv[2], cmd) = ("-h", sys.argv[2])  # Swap
        getattr(self, cmd)()

    def resume(self):
        parser = argparse.ArgumentParser(
            prog=f"{sys.argv[0]} resume", description="resume saved commands")
        parser.parse_args(sys.argv[2:])
        for idx, cmd in enumerate(self.database.proc_list):
            pid = self.execute_command(
                cmd["command"], cmd["stdin"],
                cmd["stdout"], cmd["stderr"], cmd["cwd"]
            )
            self.database.proc_list[idx]["pid"] = pid

    def delete(self):
        parser = argparse.ArgumentParser(
            prog=f"{sys.argv[0]} delete", description="delete saved commands"
        )
        parser.add_argument("idx", help="the command to delete", type=int)
        args = parser.parse_args(sys.argv[2:])
        self.database.delete(args.idx)

    def list(self):
        parser = argparse.ArgumentParser(
            prog=f"{sys.argv[0]} list", description="list saved commands")
        parser.parse_args(sys.argv[2:])
        for idx, cmd in enumerate(self.database.proc_list):
            print("command {}: {}\n\twd: {}\n\tpid: {}\n\tstdin: {}\n"
                  "\tstdout: {}\n\tstderr: {}\n".format(
                      idx, " ".join(cmd["command"]), cmd["cwd"], cmd["pid"],
                      cmd["stdin"], cmd["stdout"], cmd["stderr"]))

    def run(self):
        parser = argparse.ArgumentParser(
            prog=f"{sys.argv[0]} run", description="register and run commands"
        )
        parser.add_argument("-i", "--stdin", help="stdin file")
        parser.add_argument("-o", "--stdout", help="stdout file")
        parser.add_argument("-e", "--stderr", help="stderr file")
        parser.add_argument("-c", "--cwd", help="working directory")
        parser.add_argument("command", help="the command to register and run")
        parser.add_argument("args", help="the arguments",
                            nargs=argparse.REMAINDER)
        args = parser.parse_args(sys.argv[2:])
        command = pathlookup(args.command)
        args.args[:0] = [str(command)]
        stdin = Path(args.stdin).absolute() if args.stdin else None
        stdout = Path(args.stdout).absolute() if args.stdout else None
        stderr = Path(args.stderr).absolute() if args.stdout else None
        cwd = Path(args.cwd).absolute() if args.cwd else Path.cwd()
        pid = self.execute_command(args.args, stdin, stdout, stderr, cwd)
        self.database.add(pid, args.args, stdin, stdout, stderr, cwd)

    def prune(self):
        parser = argparse.ArgumentParser(
            prog=f"{sys.argv[0]} prune", description="prune dead commands")
        parser.parse_args(sys.argv[2:])
        for idx in range(len(self.database.proc_list)).__reversed__():
            # Traverse from high to low so that
            # the one after the deleted one won't be missed
            cmd = self.database.proc_list[idx]
            try:
                os.kill(cmd["pid"], 0)
            except ProcessLookupError:  # No such process, delete it
                self.database.delete(idx)

    @staticmethod
    def execute_command(argv, stdin, stdout, stderr, cwd):
        stdin = open(stdin, "rb") if stdin else None
        stdout = open(stdout, "wb") if stdout else None
        if not stderr:
            pass
        elif stderr == stdout:
            print("yes!")
            stderr = sp.STDOUT
        else:  # Not None and not stdout
            stderr = open(stderr, "wb")
        pid = sp.Popen(argv, stdin=stdin, stdout=stdout,
                       stderr=stderr, cwd=cwd).pid
        for fp in [stdin, stdout, stderr]:
            if hasattr(fp, "close"):
                fp.close()
        return pid


if __name__ == "__main__":
    AutoResume()
