//! Create a temporary file from stdin and execute command.
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

extern crate clap;
extern crate tempfile;

use clap::{crate_version, App, AppSettings, Arg, ArgMatches};
use std::fs::{File, OpenOptions};
use std::io;
use std::io::Write;
use std::process::{exit, Command};
use tempfile::TempDir;

const PROGRAM_NAME: &str = "mkf";

/// Parse program arguments and return `ArgMatches`
fn parse_args() -> ArgMatches {
    App::new(PROGRAM_NAME)
        .version(crate_version!())
        .author("Zhang Maiyun <myzhang1029@hotmail.com")
        .about("Create a temporary file from stdin and execute command.")
        .setting(AppSettings::InferLongArgs)
        .arg(
            Arg::new("utility")
                .help("Execute utility with the captured stdin.")
                .default_value("cat")
                .index(1),
        )
        .arg(
            Arg::new("arguments")
                .help("Arguments to utility.")
                .index(2)
                .allow_hyphen_values(true)
                .multiple_values(true),
        )
        .arg(
            Arg::new("suffix")
                .short('s')
                .long("suffix")
                .takes_value(true)
                .help("Make sure the generated file has this suffix (with the leading dot if desired). It must not contain a slash."),
        )
        // Several arguments below are derived from xargs(1)
        .arg(
            Arg::new("eofstr")
                .short('E')
                .takes_value(true)
                .help("Use eofstr as a logical EOF marker. If specified, stdin will be read as text (instead of binary)."),
        )
        .arg(
            Arg::new("replstr")
                .short('I')
                .takes_value(true)
                .help("Replace replstr with the full path to the generated temporary file."),
        )
        .arg(
            Arg::new("reopen").short('o').help(
                "Reopen stdin as /dev/tty in the child process before executing the command.",
            ),
        )
        .get_matches()
}

/// Read from stdin and fill the temporary file (Text mode)
fn read_fill_text(tmp_file: &mut File, eofstr: &str) -> io::Result<()> {
    let mut buffer = String::new();
    let stdin = io::stdin();
    while let Ok(read_len) = stdin.read_line(&mut buffer) {
        if read_len == 0 {
            // Must be EOF
            break;
        }
        // Check for manual EOF
        // Compare with the trailing newline removed
        if eofstr == &buffer[0..read_len - 1] {
            break;
        }
        tmp_file.write_all(buffer.as_bytes())?;
        // Run clear because read_line appends
        buffer.clear();
    }
    Ok(())
}

/// Read from stdin and fill the temporary file (Binary)
fn read_fill_bin(tmp_file: &mut File) -> io::Result<()> {
    io::copy(&mut io::stdin(), tmp_file)?;
    Ok(())
}

/// Make temporary file according to the arguments
fn mktmp(tmp_dir: &TempDir, suffix: Option<&str>) -> std::path::PathBuf {
    // Name for the file
    // No need to be random because we already have a tempdir
    let file_name: String = String::from("tmp_output");

    // Then construct the path to create
    tmp_dir.path().join(match suffix {
        Some(suffix) => file_name + suffix,
        None => file_name,
    })
}

/// Create and fill a temporary file
fn prepare_tmp_file<P: AsRef<std::path::Path>>(
    file_path: P,
    arg_eofstr: Option<&str>,
) -> io::Result<()> {
    // Create and truncate that so-named file
    let mut tmp_file = OpenOptions::new()
        .create(true)
        .truncate(true)
        .write(true)
        .open(file_path)?;

    // Fill the file with stdin
    match arg_eofstr {
        Some(eofstr) => read_fill_text(&mut tmp_file, eofstr),
        None => read_fill_bin(&mut tmp_file),
    }?;
    // Make sure the file is written to before we invoke the command
    tmp_file.sync_all()?;
    Ok(())
}

/// Fail the execution with an error message and return a status
fn fail<E: std::fmt::Display>(reason: E, msg: &str, code: i32) -> ! {
    eprintln!("{}: {}", msg, reason);
    std::process::exit(code)
}

fn real_main() -> i32 {
    // First parse the arguments
    let matches = parse_args();

    // Create a temporary directory to accommodate --suffix,
    // TempDir will delete it for us, but we must keep it in main's scope
    // Control its location with `std::env::temp_dir()`
    // Just die if this fails
    let tmp_dir =
        TempDir::new().unwrap_or_else(|msg| fail(msg, "Cannot create temporary directory", 2));

    // Create temporary path
    let file_path = mktmp(&tmp_dir, matches.value_of("suffix"));
    let path_as_str = file_path
        .to_str()
        .unwrap_or_else(|| fail(file_path.display(), "Cannot process this path", 1));
    // Create temporary file
    prepare_tmp_file(&file_path, matches.value_of("eofstr"))
        .unwrap_or_else(|msg| fail(msg, "Cannot capture stdin", 2));

    let args_raw = matches.values_of("arguments").unwrap_or_default();
    // Transform arguments by (replstr => path)
    let mut args: Vec<String> = args_raw
        .map(|itm| match matches.value_of("replstr") {
            Some(replstr) => itm.replace(replstr, path_as_str),
            None => itm.to_string(),
        })
        .collect();

    // If no explicit replstr specified, append it to the end
    if !matches.is_present("replstr") {
        args.push(path_as_str.to_string());
    }

    // Run the command
    // utility has default value so this shouldn't fail
    let utility = matches.value_of("utility").unwrap();
    let mut cmd = Command::new(utility);
    cmd.args(args);
    if matches.is_present("reopen") {
        cmd.stdin(
            File::open("/dev/tty").unwrap_or_else(|msg| fail(msg, "Cannot open /dev/tty", 2)),
        );
    }
    let exit_status = cmd
        .spawn()
        .unwrap_or_else(|msg| fail(msg, "Cannot execute", 3))
        // We must wait, or tmp_dir will be removed before cmd terminates
        .wait()
        // So wait() shouldn't fail normally
        .unwrap();
    exit_status.code().unwrap_or_default()
}

fn main() {
    exit(real_main());
}
