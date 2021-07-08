/// Create a temporary file from stdin and execute command.
//
//  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
extern crate clap;
extern crate rand;
extern crate tempfile;

use clap::{crate_version, App, Arg, ArgMatches};
use rand::{distributions::Alphanumeric, Rng};
use std::fs::{File, OpenOptions};
use std::io;
use std::io::Write;
use std::process::{exit, Command};
use tempfile::TempDir;

const PROGRAM_NAME: &str = "mkf";

/// Parse program arguments and return ArgMatches
fn parse_args<'a>() -> ArgMatches<'a> {
    App::new(PROGRAM_NAME)
        .version(crate_version!())
        .author("Zhang Maiyun <myzhang1029@hotmail.com")
        .about("Create a temporary file from stdin and execute command.")
        .arg(
            Arg::with_name("utility")
                .help("Execute utility with the captured stdin.")
                .default_value("cat")
                .index(1),
        )
        .arg(
            Arg::with_name("arguments")
                .help("Arguments to utility.")
                .multiple(true),
        )
        .arg(
            Arg::with_name("suffix")
                .short("s")
                .long("suffix")
                .takes_value(true)
                .help("Make sure the generated file has this suffix (with the leading dot if desired). It must not contain a slash."),
        )
        // Several arguments below are derived from xargs(1)
        .arg(
            Arg::with_name("eofstr")
                .short("E")
                .takes_value(true)
                .help("Use eofstr as a logical EOF marker."),
        )
        .arg(
            Arg::with_name("replstr")
                .short("I")
                .takes_value(true)
                .help("Replace replstr with the full path to the generated temporary file."),
        )
        .arg(
            Arg::with_name("reopen").short("o").help(
                "Reopen stdin as /dev/tty in the child process before executing the command.",
            ),
        )
        .get_matches()
}

/// Read from stdin and fill the temporary file
fn read_fill(tmp_file: &mut File, eofstr: Option<&str>) {
    let mut buffer = String::new();
    let stdin = io::stdin();
    while let Ok(read_len) = stdin.read_line(&mut buffer) {
        if read_len == 0 {
            // Must be EOF
            break;
        }
        // Check for manual EOF
        if let Some(eofstr) = eofstr {
            // Compare with the trailing newline removed
            if eofstr == &buffer[0..read_len - 1] {
                break;
            }
        }
        tmp_file.write_all(buffer.as_bytes()).unwrap();
        // Run clear because read_line appends
        buffer.clear();
    }
}

/// Make temporary file according to the arguments
fn mktmp(tmp_dir: &TempDir, suffix: Option<&str>) -> std::path::PathBuf {
    // Generate name for the file
    let file_name: String = rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(10)
        .map(char::from)
        .collect();

    // Then construct the path to create
    tmp_dir.path().join(match suffix {
        Some(suffix) => file_name + suffix,
        None => file_name,
    })
}

fn real_main() -> i32 {
    // First parse the arguments
    let matches = parse_args();

    // Create a temporary directory to accommodate --suffix,
    // TempDir will delete it for us, but we must keep it in main's scope
    // Control its location with `std::env::temp_dir()`
    // Just die if this fails
    let tmp_dir = TempDir::new().unwrap();
    // Create temporary path
    let file_path = mktmp(&tmp_dir, matches.value_of("suffix"));
    {
        let mut tmp_file = OpenOptions::new()
            .create(true)
            .truncate(true)
            .write(true)
            .open(&file_path)
            .unwrap();

        // Fill the file
        read_fill(&mut tmp_file, matches.value_of("eofstr"));
        // Make sure the file is written to before we invoke the command
        tmp_file.sync_all().unwrap();
    }

    let args_raw = matches.values_of("arguments").unwrap_or_default();
    // Transform arguments by (replstr => path)
    let mut args: Vec<String> = args_raw
        .map(|itm| match matches.value_of("replstr") {
            Some(replstr) => itm.replace(replstr, file_path.to_str().unwrap()),
            None => itm.to_string(),
        })
        .collect();

    // If no explicit replstr specified, append it to the end
    if !matches.is_present("replstr") {
        args.push(file_path.to_str().unwrap().to_string());
    }

    // Run the command
    let utility = matches.value_of("utility").unwrap();
    let mut cmd = Command::new(utility);
    cmd.args(args);
    if matches.is_present("reopen") {
        cmd.stdin(File::open("/dev/tty").unwrap());
    }
    let exit_status = cmd
        .spawn()
        .unwrap()
        // We must wait, or tmp_dir will be removed before cmd terminates
        .wait()
        .unwrap();
    exit_status.code().unwrap_or_default()
}

fn main() {
    exit(real_main());
}
