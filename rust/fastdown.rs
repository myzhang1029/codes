/// Download quickly with aria2.
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
//
use std::env::args;
use std::process::{exit, Command};

const MIRRORS: [&'static str; 7] = [
    "https://mirrors.tuna.tsinghua.edu.cn",
    "https://opentuna.cn",
    "https://mirrors.163.com",
    "https://mirrors.aliyun.com",
    "https://mirrors.bfsu.edu.cn",
    "https://mirrors.sjtug.sjtu.edu.cn",
    "https://mirrors.ustc.edu.cn",
];

fn main() {
    let mut args = args();
    // Ensure at least the resource path is present
    if args.len() < 2 {
        eprintln!("Resource path must be supplied");
        eprintln!("Usage: fastdown path [args_to_aria2c]");
        exit(1);
    }

    // Skip program path
    args.next();
    // The first argument is the resource path relative to the site root
    let resource_loc = args.next().unwrap();
    // Create a list of all mirrors' urls
    let urls: Vec<String> = MIRRORS
        .iter()
        .map(|url| format!("{}/{}", url, resource_loc))
        .collect();

    Command::new("aria2c")
        .args(urls)
        .args(args)
        .status()
        .expect("subprocess failed");
}
