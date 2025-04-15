//! Read a Zstandard-compressed tar file and unpack a file from it in real time
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

extern crate tar;
extern crate ureq;
extern crate zstd;

use std::env::args;
use std::io;
use std::path::Path;
use tar::Archive;
use zstd::stream::read::Decoder;

const URI: &str = "https://objectstorage.ca-toronto-1.oraclecloud.com/n/yzoe51nha7tk/b/bucket-gsod/o/gsod-20210901.tar.zst";

/// Get the archive
fn get_file() -> Result<impl io::Read, Box<ureq::Error>> {
    let resp = ureq::get(URI).call()?;
    let bufreader = resp.into_body().into_reader();
    Ok(bufreader)
}

/// Unpack a single file from the tar archive
fn unpack_from_tar<'a, R: io::Read>(
    archive: &'a mut Archive<R>,
    path_for: &Path,
) -> io::Result<tar::Entry<'a, R>> {
    archive
        .entries()?
        .find_map(|entry| -> Option<tar::Entry<_>> {
            let entry_unwr = entry.ok()?;
            let path = entry_unwr.path().ok()?;
            if path == path_for {
                Some(entry_unwr)
            } else {
                None
            }
        })
        .ok_or_else(|| {
            io::Error::new(
                io::ErrorKind::NotFound,
                format!("Not found in archive: {}", path_for.display()),
            )
        })
}

fn main() {
    // Parse arguments
    let look_for = args().nth(1).expect("Usage: prog path");

    // GET the file
    let reader = get_file().expect("Cannot issue request");

    // Zstandard uncompress
    let zstd_decoder = Decoder::new(reader).expect("Cannot create Zstandard decompresser");

    // Tar uncompress
    let mut archive = Archive::new(zstd_decoder);
    let mut file =
        unpack_from_tar(&mut archive, Path::new(&look_for)).expect("Cannot unpack file from tar");

    // Write output
    let mut stdout = io::stdout();
    io::copy(&mut file, &mut stdout).expect("Cannot write to stdout");
}

#[cfg(test)]
mod tests {
    use std::io::{Cursor, ErrorKind, Read};
    use std::path::Path;
    use tar::{Archive, Builder, Header};
    #[test]
    fn test_get_file() {
        let mut reader = crate::get_file().expect("Cannot issue request");
        // Now the response succeeded, test that it's correct
        let mut buf = vec![0; 4];
        // Zstandard v0.8 BE magic number
        let zstd_header = vec![0x28, 0xb5, 0x2f, 0xfd];
        reader.read_exact(&mut buf).expect("Cannot read from link");
        // Test that the received content is Zstandard data
        assert_eq!(
            buf[0..4],
            zstd_header,
            "Received header should be 0x28b52ffd"
        );
    }

    fn create_test_archive() -> Vec<u8> {
        let mut header = Header::new_gnu();
        header.set_path("Path/File.test").unwrap();
        header.set_size(4);
        header.set_cksum();
        let data: &[u8] = &[1, 2, 3, 4];
        let mut ar = Builder::new(Vec::new());
        ar.append(&header, data).unwrap();
        let data = ar.into_inner().unwrap();
        data
    }

    #[test]
    fn test_unpack_from_tar() {
        let archive_stream = create_test_archive();
        let reader = Cursor::new(&archive_stream);
        let mut archive = Archive::new(reader);
        let mut file = crate::unpack_from_tar(&mut archive, Path::new("Path/File.test"))
            .expect("Cannot unpack file from tar");
        let mut buf = vec![0; 4];
        file.read_exact(&mut buf).expect("Cannot read in tar file");
        assert_eq!(buf, [1, 2, 3, 4], "Returned data should be 0x01020304");

        // Create a new reader
        let reader = Cursor::new(&archive_stream);
        let mut archive = Archive::new(reader);
        let file = crate::unpack_from_tar(&mut archive, Path::new("Path/None.test"));
        if let Err(error) = file {
            assert_eq!(
                error.kind(),
                ErrorKind::NotFound,
                "Test should return file not found"
            );
        } else {
            assert!(file.is_err(), "Test should not be successful");
        }
    }
}
