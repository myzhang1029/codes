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

use clap::{crate_version, App, Arg, ArgMatches};
use std::process::{exit, Command};

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
                .required(true)
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
                .takes_value(true)
                .help("Make sure the generated file has this suffix. It must not contain a slash."),
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
        .get_matches()
}

fn main() {
    let matches = parse_args();
    eprintln!("{:?}", matches);
    println!("Hello, world!");
}
