use std::process::{Command, Output};

fn ciutil(args: &[&str]) -> Output {
    Command::new(env!("CARGO_BIN_EXE_ciutil"))
        .args(args)
        .output()
        .expect("failed to run ciutil")
}

fn assert_success(args: &[&str], expected_stdout: &[u8]) {
    let output = ciutil(args);
    assert_eq!(output.status.code(), Some(0), "{args:?}");
    assert_eq!(output.stdout, expected_stdout, "{args:?}");
    assert!(output.stderr.is_empty(), "{args:?}");
}

fn assert_failure(args: &[&str], code: i32) {
    let output = ciutil(args);
    assert_eq!(output.status.code(), Some(code), "{args:?}");
    assert!(output.stdout.is_empty(), "{args:?}");
    assert!(!output.stderr.is_empty(), "{args:?}");
}

#[test]
fn version() {
    assert_success(&["version"], b"ciutil rust 1.0.0\n");
}

#[test]
fn crc32() {
    assert_success(&["crc32", "123456789"], b"cbf43926\n");
    assert_success(&["crc32", ""], b"00000000\n");
    assert_success(&["crc32", "a"], b"e8b7be43\n");
    assert_success(&["crc32", "abc"], b"352441c2\n");
    let fox = "The quick brown fox jumps over the lazy dog";
    assert_success(&["crc32", fox], b"414fa339\n");
}

#[test]
fn adler32() {
    assert_success(&["adler32", ""], b"00000001\n");
    assert_success(&["adler32", "a"], b"00620062\n");
    assert_success(&["adler32", "abc"], b"024d0127\n");
    assert_success(&["adler32", "Wikipedia"], b"11e60398\n");
    assert_success(&["adler32", "123456789"], b"091e01de\n");
}

#[test]
fn base64_encode() {
    assert_success(&["b64enc", ""], b"\n");
    assert_success(&["b64enc", "f"], b"Zg==\n");
    assert_success(&["b64enc", "fo"], b"Zm8=\n");
    assert_success(&["b64enc", "foo"], b"Zm9v\n");
    assert_success(&["b64enc", "foobar"], b"Zm9vYmFy\n");
}

#[test]
fn base64_decode_writes_raw_bytes_without_newline() {
    assert_success(&["b64dec", ""], b"");
    assert_success(&["b64dec", "Zm9vYmE="], b"fooba");
    assert_success(&["b64dec", "Zm9vYmFy"], b"foobar");
    assert_success(&["b64dec", "AP/+"], &[0x00, 0xFF, 0xFE]);
}

#[test]
fn base64_round_trips_utf8_input() {
    let text = "h\u{e9}llo \u{4e16}\u{754c}";
    let encoded = ciutil(&["b64enc", text]);
    assert_eq!(encoded.status.code(), Some(0));
    let encoded = String::from_utf8(encoded.stdout).unwrap();
    let encoded = encoded.strip_suffix('\n').unwrap();
    assert_success(&["b64dec", encoded], text.as_bytes());
}

#[test]
fn base64_decode_rejects_bad_input() {
    for text in ["Zg", "Zg=", "Zm9-", "Zm9_", "Zh==", "Zm9=", "Zm 9"] {
        assert_failure(&["b64dec", text], 2);
    }
    assert_failure(&["b64dec", "Zg==Zg=="], 2);
}

#[test]
fn roman() {
    assert_success(&["roman", "1"], b"I\n");
    assert_success(&["roman", "1994"], b"MCMXCIV\n");
    assert_success(&["roman", "2024"], b"MMXXIV\n");
    assert_success(&["roman", "3999"], b"MMMCMXCIX\n");
}

#[test]
fn unroman() {
    assert_success(&["unroman", "I"], b"1\n");
    assert_success(&["unroman", "MCMXCIV"], b"1994\n");
    assert_success(&["unroman", "MMMCMXCIX"], b"3999\n");
}

#[test]
fn roman_rejects_bad_input() {
    for text in ["0", "4000", "-1", "abc", "", "1.5"] {
        assert_failure(&["roman", text], 2);
    }
    assert_failure(&["roman", "99999999999999999999"], 2);
    for text in ["", "IIII", "VX", "IC", "XM", "VV", "IIX", "xiv"] {
        assert_failure(&["unroman", text], 2);
    }
}

#[test]
fn semver_cmp() {
    assert_success(&["semver-cmp", "1.0.0", "2.0.0"], b"-1\n");
    assert_success(&["semver-cmp", "2.0.0", "1.0.0"], b"1\n");
    assert_success(&["semver-cmp", "1.0.0", "1.0.0"], b"0\n");
    assert_success(&["semver-cmp", "1.0.0+a", "1.0.0+b"], b"0\n");
    assert_success(&["semver-cmp", "1.0.0-rc.1", "1.0.0"], b"-1\n");
    assert_success(&["semver-cmp", "1.0.0-beta.11", "1.0.0-beta.2"], b"1\n");
}

#[test]
fn semver_cmp_rejects_bad_input() {
    assert_failure(&["semver-cmp", "1.0", "1.0.0"], 2);
    assert_failure(&["semver-cmp", "1.0.0", "01.0.0"], 2);
    assert_failure(&["semver-cmp", "", ""], 2);
}

#[test]
fn unknown_command_and_wrong_arity_print_usage() {
    let cases: [&[&str]; 10] = [
        &[],
        &["nope"],
        &["crc32"],
        &["crc32", "a", "b"],
        &["adler32"],
        &["b64enc", "a", "b"],
        &["roman"],
        &["semver-cmp", "1.0.0"],
        &["semver-cmp", "1.0.0", "1.0.0", "1.0.0"],
        &["version", "extra"],
    ];
    for args in cases {
        let output = ciutil(args);
        assert_eq!(output.status.code(), Some(64), "{args:?}");
        assert!(output.stdout.is_empty(), "{args:?}");
        let stderr = String::from_utf8(output.stderr).unwrap();
        assert!(stderr.starts_with("usage: ciutil"), "{args:?}");
    }
}

#[cfg(unix)]
#[test]
fn non_utf8_argument_is_an_error() {
    use std::ffi::OsStr;
    use std::os::unix::ffi::OsStrExt;

    let output = Command::new(env!("CARGO_BIN_EXE_ciutil"))
        .arg("crc32")
        .arg(OsStr::from_bytes(&[0xFF, 0xFE]))
        .output()
        .expect("failed to run ciutil");
    assert_eq!(output.status.code(), Some(2));
    assert!(output.stdout.is_empty());
}
