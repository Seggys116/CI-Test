use std::cmp::Ordering;

use ciutil::adler32::{self, Adler32};
use ciutil::base64::{self, DecodeError};
use ciutil::crc32::{self, Crc32};
use ciutil::roman::{self, RomanError};
use ciutil::semver::{self, ParseError, Version};

fn pseudo_random(len: usize) -> Vec<u8> {
    let mut x: u64 = 42;
    (0..len)
        .map(|_| {
            x = (x * 1_103_515_245 + 12_345) & 0x7FFF_FFFF;
            ((x >> 16) & 0xFF) as u8
        })
        .collect()
}

fn v(text: &str) -> Version {
    semver::parse(text).unwrap_or_else(|e| panic!("{text:?} should parse: {e}"))
}

#[test]
fn crc32_vectors() {
    let fox = b"The quick brown fox jumps over the lazy dog";
    let cases: [(&[u8], u32); 7] = [
        (b"", 0x0000_0000),
        (b"a", 0xe8b7_be43),
        (b"abc", 0x3524_41c2),
        (b"123456789", 0xcbf4_3926),
        (fox, 0x414f_a339),
        (&[0x00; 32], 0x190a_55ad),
        (&[0xFF; 32], 0xff6c_ab0b),
    ];
    for (input, expected) in cases {
        assert_eq!(crc32::checksum(input), expected, "{input:?}");
    }
}

#[test]
fn crc32_chunked_equals_one_shot_for_every_split() {
    let data = pseudo_random(1000);
    let whole = crc32::checksum(&data);
    for split in 0..=data.len() {
        let mut crc = Crc32::new();
        crc.update(&data[..split]);
        crc.update(&data[split..]);
        assert_eq!(crc.finalize(), whole, "split at {split}");
    }
}

#[test]
fn adler32_vectors() {
    let cases: [(&[u8], u32); 5] = [
        (b"", 0x0000_0001),
        (b"a", 0x0062_0062),
        (b"abc", 0x024d_0127),
        (b"Wikipedia", 0x11e6_0398),
        (b"123456789", 0x091e_01de),
    ];
    for (input, expected) in cases {
        assert_eq!(adler32::checksum(input), expected, "{input:?}");
    }
}

#[test]
fn adler32_chunked_equals_one_shot_for_every_split() {
    let data = pseudo_random(1000);
    let whole = adler32::checksum(&data);
    for split in 0..=data.len() {
        let mut adler = Adler32::new();
        adler.update(&data[..split]);
        adler.update(&data[split..]);
        assert_eq!(adler.finalize(), whole, "split at {split}");
    }
}

#[test]
fn adler32_long_input_matches_per_byte_modulo_reference() {
    let data = vec![0xFFu8; 100_000];
    let mut a = 1u32;
    let mut b = 0u32;
    for &byte in &data {
        a = (a + u32::from(byte)) % 65_521;
        b = (b + a) % 65_521;
    }
    assert_eq!(adler32::checksum(&data), (b << 16) | a);

    let mut adler = Adler32::new();
    for chunk in data.chunks(777) {
        adler.update(chunk);
    }
    assert_eq!(adler.finalize(), (b << 16) | a);
}

#[test]
fn base64_vectors() {
    let cases: [(&[u8], &str); 8] = [
        (b"", ""),
        (b"f", "Zg=="),
        (b"fo", "Zm8="),
        (b"foo", "Zm9v"),
        (b"foob", "Zm9vYg=="),
        (b"fooba", "Zm9vYmE="),
        (b"foobar", "Zm9vYmFy"),
        (&[0x00, 0xFF, 0xFE], "AP/+"),
    ];
    for (input, expected) in cases {
        assert_eq!(base64::encode(input), expected);
        assert_eq!(base64::decode(expected).unwrap(), input);
    }
}

#[test]
fn base64_rejections() {
    let cases = [
        ("Z", DecodeError::InvalidLength),
        ("Zg", DecodeError::InvalidLength),
        ("Zg=", DecodeError::InvalidLength),
        ("Zg===", DecodeError::InvalidLength),
        ("Zm9-", bad_char(3, b'-')),
        ("Zm9_", bad_char(3, b'_')),
        ("Zm9 ", bad_char(3, b' ')),
        ("Zm9\n", bad_char(3, b'\n')),
        ("Zg==Zg==", DecodeError::InvalidPadding),
        ("====", DecodeError::InvalidPadding),
        ("Z===", DecodeError::InvalidPadding),
        ("Zm=v", DecodeError::InvalidPadding),
        ("Zh==", DecodeError::NonCanonical),
        ("Zm9=", DecodeError::NonCanonical),
    ];
    for (input, expected) in cases {
        assert_eq!(base64::decode(input), Err(expected), "{input:?}");
    }
}

fn bad_char(index: usize, byte: u8) -> DecodeError {
    DecodeError::InvalidCharacter { index, byte }
}

#[test]
fn base64_round_trips_every_length_up_to_300() {
    let data = pseudo_random(300);
    for len in 0..=300 {
        let encoded = base64::encode(&data[..len]);
        assert_eq!(base64::decode(&encoded).unwrap(), &data[..len], "{len}");
    }
}

#[test]
fn roman_vectors() {
    let cases = [
        (1, "I"),
        (4, "IV"),
        (9, "IX"),
        (14, "XIV"),
        (40, "XL"),
        (90, "XC"),
        (400, "CD"),
        (1994, "MCMXCIV"),
        (2024, "MMXXIV"),
        (3999, "MMMCMXCIX"),
    ];
    for (number, numeral) in cases {
        assert_eq!(roman::to_roman(number).unwrap(), numeral);
        assert_eq!(i64::from(roman::from_roman(numeral).unwrap()), number);
    }
}

#[test]
fn roman_out_of_range() {
    for number in [0, 4000, -1, -4000, i64::MIN, i64::MAX] {
        assert_eq!(roman::to_roman(number), Err(RomanError::OutOfRange));
    }
}

#[test]
fn roman_rejects_non_canonical_numerals() {
    for text in ["", "IIII", "VX", "IC", "XM", "VV", "IIX", "xiv"] {
        let result = roman::from_roman(text);
        assert_eq!(result, Err(RomanError::Invalid), "{text:?}");
    }
}

#[test]
fn roman_round_trips_the_whole_range() {
    for n in 1..=3999_u16 {
        let numeral = roman::to_roman(i64::from(n)).unwrap();
        assert_eq!(roman::from_roman(&numeral), Ok(n));
    }
}

#[test]
fn semver_accepts() {
    for text in [
        "0.0.0",
        "1.0.0-0",
        "1.0.0-0a",
        "1.0.0-x-y-z.-",
        "1.0.0+001",
        "1.0.0-rc.1+build.5",
    ] {
        assert_eq!(v(text).to_string(), text);
    }
}

#[test]
fn semver_rejects() {
    for text in [
        "",
        "1",
        "1.0",
        "1.0.0.0",
        "v1.0.0",
        "01.0.0",
        "1.00.0",
        "1.0.00",
        "1.0.0-",
        "1.0.0+",
        "1.0.0-01",
        "1.0.0-a..b",
        "1.0.0-a_b",
        "-1.0.0",
        " 1.0.0",
        "1.0.0 ",
        "1.0.0-alpha+",
    ] {
        assert!(semver::parse(text).is_err(), "accepted {text:?}");
    }
    assert_eq!(semver::parse("01.0.0"), Err(ParseError::LeadingZero));
    let overflow = "18446744073709551616.0.0";
    assert_eq!(semver::parse(overflow), Err(ParseError::Overflow));
}

#[test]
fn semver_ordering_chain_for_all_ordered_pairs() {
    let chain = [
        "1.0.0-alpha",
        "1.0.0-alpha.1",
        "1.0.0-alpha.beta",
        "1.0.0-beta",
        "1.0.0-beta.2",
        "1.0.0-beta.11",
        "1.0.0-rc.1",
        "1.0.0",
        "2.0.0",
        "2.1.0",
        "2.1.1",
    ];
    for (i, a) in chain.iter().enumerate() {
        for b in chain.iter().skip(i + 1) {
            assert_eq!(semver::compare(&v(a), &v(b)), -1, "{a} < {b}");
            assert_eq!(semver::compare(&v(b), &v(a)), 1, "{b} > {a}");
            assert_eq!(v(a).cmp_precedence(&v(b)), Ordering::Less, "{a} < {b}");
        }
        assert_eq!(semver::compare(&v(a), &v(a)), 0, "{a} == {a}");
    }
}

#[test]
fn semver_build_metadata_is_ignored() {
    assert_eq!(semver::compare(&v("1.0.0+a"), &v("1.0.0+b")), 0);
}
