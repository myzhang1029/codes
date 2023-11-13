//! Create a temporary file from stdin and execute command.
//
//  Copyright (C) 2021 Zhang Maiyun <me@maiyun.me>
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

use clap::{command, Parser};
use std::fs::{File, OpenOptions};
use std::io;
use std::io::Write;
use std::process::{Command, Stdio};
use tempfile::TempDir;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about=None, infer_long_args=true)]
struct Args {
    /// Execute `utility` with the captured stdin
    #[arg(default_value_t=String::from("cat"))]
    utility: String,
    /// Arguments to `utility`
    #[arg(allow_hyphen_values = true)]
    arguments: Vec<String>,
    /// Make sure the generated file has this suffix (with the leading dot if desired). It must not contain a slash
    #[arg(short, long, default_value_t=String::default())]
    suffix: String,
    /// Use eofstr as a logical EOF marker. If specified, stdin will be read as text (instead of binary)
    #[arg(short = 'E', long)]
    eofstr: Option<String>,
    /// Replace replstr with the full path to the generated temporary file
    #[arg(short = 'I', long)]
    replstr: Option<String>,
    /// Reopen stdin as /dev/tty in the child process before executing the command
    #[arg(short = 'o', long)]
    reopen: bool,
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
fn mktmp(tmp_dir: &TempDir, suffix: &str) -> std::path::PathBuf {
    // Name for the file
    // No need to be random because we already have a tempdir
    let file_name: String = String::from("tmp_output");

    // Then construct the path to create
    tmp_dir.path().join(file_name + suffix)
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
    if let Some(eofstr) = arg_eofstr {
        read_fill_text(&mut tmp_file, eofstr)?;
    } else {
        read_fill_bin(&mut tmp_file)?;
    }
    // Make sure the file is written to before we invoke the command
    tmp_file.flush()
}

/// Fail the execution with an error message and return a status
fn fail<E: std::fmt::Display>(reason: E, msg: &str, code: i32) -> ! {
    eprintln!("{msg}: {reason}");
    std::process::exit(code)
}

fn main() {
    // First parse the arguments
    let args = Args::parse();

    // Create a temporary directory to accommodate --suffix,
    // TempDir will delete it for us, but we must keep it in main's scope
    // Control its location with `std::env::temp_dir()`
    // Just die if this fails
    let tmp_dir =
        TempDir::new().unwrap_or_else(|msg| fail(msg, "Cannot create temporary directory", 2));

    // Create temporary path
    let file_path = mktmp(&tmp_dir, &args.suffix);
    let path_as_str = file_path
        .to_str()
        .unwrap_or_else(|| fail(file_path.display(), "Cannot process this path", 1));
    // Create temporary file
    prepare_tmp_file(&file_path, args.eofstr.as_deref())
        .unwrap_or_else(|msg| fail(msg, "Cannot capture stdin", 2));

    let prog_args = if let Some(ref replstr) = args.replstr {
        // Transform arguments by (replstr => path)
        args.arguments
            .iter()
            .map(|itm| itm.replace(replstr, path_as_str))
            .collect()
    } else {
        // If no explicit replstr specified, append it to the end
        let mut new = args.arguments.clone();
        new.push(path_as_str.to_string());
        new
    };

    // Run the command
    let exit_status = Command::new(args.utility)
        .args(prog_args)
        .stdin(if args.reopen {
            File::open("/dev/tty")
                .unwrap_or_else(|msg| fail(msg, "Cannot open /dev/tty", 2))
                .into()
        } else {
            Stdio::inherit()
        })
        .spawn()
        .unwrap_or_else(|msg| fail(msg, "Cannot execute", 3))
        // We must wait, or tmp_dir will be removed before cmd terminates
        .wait()
        // So wait() shouldn't fail normally
        .unwrap()
        // Get the exit code
        .code()
        .unwrap_or_default();
    std::process::exit(exit_status)
}
