//! Base64 as specified by RFC 4648 (standard alphabet, with padding) and a
//! strict decoder.

use std::error::Error;
use std::fmt;

const ALPHABET: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const INVALID: u8 = 0xFF;
const DECODE_TABLE: [u8; 256] = make_decode_table();

const fn make_decode_table() -> [u8; 256] {
    let mut table = [INVALID; 256];
    let mut i = 0;
    while i < 64 {
        table[ALPHABET[i] as usize] = i as u8;
        i += 1;
    }
    table
}

/// Reasons why [`decode`] rejects its input.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DecodeError {
    /// The input length is not a multiple of four.
    InvalidLength,
    /// A byte outside the standard alphabet was found at `index`.
    InvalidCharacter { index: usize, byte: u8 },
    /// `=` appeared anywhere other than as the last one or two characters.
    InvalidPadding,
    /// The unused trailing bits of the final group are not zero.
    NonCanonical,
}

impl fmt::Display for DecodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InvalidLength => f.write_str("length is not a multiple of 4"),
            Self::InvalidCharacter { index, byte } => {
                write!(f, "invalid byte 0x{byte:02x} at index {index}")
            }
            Self::InvalidPadding => f.write_str("misplaced padding"),
            Self::NonCanonical => f.write_str("non-zero trailing bits"),
        }
    }
}

impl Error for DecodeError {}

fn symbol(value: u32) -> char {
    char::from(ALPHABET[(value & 0x3F) as usize])
}

/// Encodes `data` using the standard alphabet with `=` padding.
///
/// # Examples
///
/// ```
/// assert_eq!(ciutil::base64::encode(b"foobar"), "Zm9vYmFy");
/// assert_eq!(ciutil::base64::encode(b"fo"), "Zm8=");
/// assert_eq!(ciutil::base64::encode(&[0x00, 0xff, 0xfe]), "AP/+");
/// ```
pub fn encode(data: &[u8]) -> String {
    let mut out = String::with_capacity(data.len().div_ceil(3) * 4);
    for chunk in data.chunks(3) {
        let mut triple = 0u32;
        for (i, &byte) in chunk.iter().enumerate() {
            triple |= u32::from(byte) << (16 - 8 * i);
        }
        let groups = [triple >> 18, triple >> 12, triple >> 6, triple];
        for &group in &groups[..=chunk.len()] {
            out.push(symbol(group));
        }
        for _ in chunk.len()..3 {
            out.push('=');
        }
    }
    out
}

/// Decodes strictly: the input must be canonical padded base64 over the
/// standard alphabet, with no whitespace and no non-zero trailing bits.
///
/// # Examples
///
/// ```
/// use ciutil::base64::{decode, DecodeError};
///
/// assert_eq!(decode("Zm9vYg=="), Ok(b"foob".to_vec()));
/// assert_eq!(decode("Zg"), Err(DecodeError::InvalidLength));
/// assert_eq!(decode("Zh=="), Err(DecodeError::NonCanonical));
/// ```
pub fn decode(text: &str) -> Result<Vec<u8>, DecodeError> {
    let bytes = text.as_bytes();
    let quads = bytes.chunks_exact(4);
    if !quads.remainder().is_empty() {
        return Err(DecodeError::InvalidLength);
    }
    let quad_count = bytes.len() / 4;
    let mut out = Vec::with_capacity(quad_count * 3);

    for (q, quad) in quads.enumerate() {
        let is_last = q + 1 == quad_count;
        let mut values = [0u32; 4];
        let mut padding = 0usize;
        for (i, &byte) in quad.iter().enumerate() {
            if byte == b'=' {
                if !is_last || i < 2 {
                    return Err(DecodeError::InvalidPadding);
                }
                padding += 1;
            } else if padding > 0 {
                return Err(DecodeError::InvalidPadding);
            } else {
                let value = DECODE_TABLE[usize::from(byte)];
                if value == INVALID {
                    return Err(DecodeError::InvalidCharacter {
                        index: q * 4 + i,
                        byte,
                    });
                }
                values[i] = u32::from(value);
            }
        }

        let stray_bits = match padding {
            1 => values[2] & 0x03,
            2 => values[1] & 0x0F,
            _ => 0,
        };
        if stray_bits != 0 {
            return Err(DecodeError::NonCanonical);
        }

        let triple = (values[0] << 18) | (values[1] << 12) | (values[2] << 6) | values[3];
        let decoded = [(triple >> 16) as u8, (triple >> 8) as u8, triple as u8];
        out.extend_from_slice(&decoded[..3 - padding]);
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn pseudo_random(len: usize) -> Vec<u8> {
        let mut x: u64 = 42;
        (0..len)
            .map(|_| {
                x = (x * 1_103_515_245 + 12_345) & 0x7FFF_FFFF;
                ((x >> 16) & 0xFF) as u8
            })
            .collect()
    }

    fn check(data: &[u8], encoded: &str) {
        assert_eq!(encode(data), encoded);
        assert_eq!(decode(encoded).as_deref(), Ok(data));
    }

    fn reject(text: &str) {
        assert!(decode(text).is_err(), "accepted {text:?}");
    }

    fn bad_char(index: usize, byte: u8) -> DecodeError {
        DecodeError::InvalidCharacter { index, byte }
    }

    #[test]
    fn rfc4648_vectors() {
        check(b"", "");
        check(b"f", "Zg==");
        check(b"fo", "Zm8=");
        check(b"foo", "Zm9v");
        check(b"foob", "Zm9vYg==");
        check(b"fooba", "Zm9vYmE=");
        check(b"foobar", "Zm9vYmFy");
    }

    #[test]
    fn symbol_edge_vectors() {
        check(&[0x00, 0xFF, 0xFE], "AP/+");
        check(&[0xFB, 0xFF, 0xBF], "+/+/");
        check(&[0x00, 0x00, 0x00], "AAAA");
        check(&[0xFF, 0xFF, 0xFF], "////");
    }

    #[test]
    fn alphabet_is_complete_and_unique() {
        let mut seen = [false; 256];
        for &byte in ALPHABET {
            assert!(!seen[usize::from(byte)]);
            seen[usize::from(byte)] = true;
        }
        let valid = DECODE_TABLE.iter().filter(|&&v| v != INVALID).count();
        assert_eq!(valid, 64);
    }

    #[test]
    fn rejects_bad_length() {
        reject("Z");
        reject("Zg");
        reject("Zg=");
        reject("Zm9vY");
        reject("Zg===");
        reject("Zg=====");
        assert_eq!(decode("Zm9vYg="), Err(DecodeError::InvalidLength));
    }

    #[test]
    fn rejects_characters_outside_the_alphabet() {
        reject("Zm9-");
        reject("Zm9_");
        reject("Zm 9");
        reject(" Zm9");
        reject("Zm9\n");
        reject("Zm9\t");
        reject("Zm9\r");
        reject("Zm9*");
        reject("\u{e9}\u{e9}");
        reject("Zm9\u{e9}");
        assert_eq!(decode("Zm9-"), Err(bad_char(3, b'-')));
        assert_eq!(decode("Zm9vZ_9v"), Err(bad_char(5, b'_')));
        assert_eq!(decode("Zm9 "), Err(bad_char(3, b' ')));
    }

    #[test]
    fn rejects_misplaced_padding() {
        reject("Zg==Zg==");
        reject("Zm8=Zm8=");
        reject("Zm9vZg==Zg==");
        reject("====");
        reject("=Zg=");
        reject("Z===");
        reject("Zm=v");
        reject("Zg=a");
        reject("Zg==\n");
        assert_eq!(decode("Zg==Zg=="), Err(DecodeError::InvalidPadding));
        assert_eq!(decode("Zm=v"), Err(DecodeError::InvalidPadding));
        assert_eq!(decode("Z==="), Err(DecodeError::InvalidPadding));
    }

    #[test]
    fn rejects_non_canonical_trailing_bits() {
        assert_eq!(decode("Zh=="), Err(DecodeError::NonCanonical));
        assert_eq!(decode("Zm9="), Err(DecodeError::NonCanonical));
        assert_eq!(decode("Zm9vZh=="), Err(DecodeError::NonCanonical));
        assert_eq!(decode("Zm9vYmF="), Err(DecodeError::NonCanonical));
        assert_eq!(decode("Zg=="), Ok(b"f".to_vec()));
        assert_eq!(decode("Zm8="), Ok(b"fo".to_vec()));
    }

    #[test]
    fn round_trips_every_length_up_to_300() {
        let data = pseudo_random(300);
        for len in 0..=300 {
            let encoded = encode(&data[..len]);
            assert_eq!(encoded.len(), len.div_ceil(3) * 4, "length {len}");
            let decoded = decode(&encoded);
            assert_eq!(decoded.as_deref(), Ok(&data[..len]), "length {len}");
        }
    }

    #[test]
    fn pseudo_random_generator_matches_the_shared_definition() {
        // 42 * 1103515245 + 12345 mod 2^31 = 1250496027, and (1250496027 >> 16) & 0xFF = 137
        assert_eq!(pseudo_random(1), [137]);
    }

    #[test]
    fn error_messages_are_descriptive() {
        let text = DecodeError::InvalidLength.to_string();
        assert_eq!(text, "length is not a multiple of 4");
        let text = bad_char(3, b'-').to_string();
        assert_eq!(text, "invalid byte 0x2d at index 3");
        let text = DecodeError::InvalidPadding.to_string();
        assert_eq!(text, "misplaced padding");
        let text = DecodeError::NonCanonical.to_string();
        assert_eq!(text, "non-zero trailing bits");
    }
}
