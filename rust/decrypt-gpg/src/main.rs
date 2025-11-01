use sequoia_openpgp::crypto::Password;
use sequoia_openpgp::packet::Key;
use sequoia_openpgp::packet::key::{KeyRole, SecretParts};
use sequoia_openpgp::parse::Parse;
use sequoia_openpgp::serialize::Serialize;
use sequoia_openpgp::{Cert, Packet};
use std::io::{Read, Write, stdin, stdout};

macro_rules! decrypt_type {
    ($sec_key:expr, $password:expr, $key_type:ident) => {{
        let (key, mut secret) = $sec_key.take_secret();
        let key = Key::$key_type(key);
        if secret.is_encrypted() {
            secret.decrypt_in_place(&key, $password)?;
        }
        Ok(key.add_secret(secret).0)
    }};
}
fn decrypt_maybe<KeyType: KeyRole>(
    sec_key: Key<SecretParts, KeyType>,
    password: &Password,
) -> Result<Key<SecretParts, KeyType>, sequoia_openpgp::anyhow::Error> {
    match sec_key {
        Key::V4(sec_key) => decrypt_type!(sec_key, password, V4),
        Key::V6(sec_key) => decrypt_type!(sec_key, password, V6),
        _ => {
            eprintln!("Encountered unsupported key type: {}", sec_key.version());
            Ok(sec_key)
        }
    }
}

fn main() {
    let mut cert_serialized = Vec::new();
    stdin().read_to_end(&mut cert_serialized).unwrap();
    let pass = rpassword::prompt_password("Your password: ").unwrap();
    let password = &pass.into();
    let decrypted_cert: Cert = Cert::from_bytes(&cert_serialized)
        .unwrap()
        .into_tsk()
        .into_packets()
        .map(|pkt| {
            eprintln!("{:?}", pkt.kind());
            match pkt {
                Packet::SecretKey(seckey) => {
                    Packet::SecretKey(decrypt_maybe(seckey, password).unwrap())
                }
                Packet::SecretSubkey(seckey) => {
                    Packet::SecretSubkey(decrypt_maybe(seckey, password).unwrap())
                }

                _ => pkt,
            }
        })
        .collect::<Vec<Packet>>()
        .try_into()
        .unwrap();
    let mut buf = Vec::new();
    decrypted_cert
        .into_tsk()
        .armored()
        .serialize(&mut buf)
        .unwrap();
    stdout().write_all(&buf).unwrap();
}
