//! Command line front end, kept free of process state so it can be tested.

use std::fmt::Display;
use std::io::{self, Write};

use crate::{adler32, base64, crc32, roman, semver};

/// Exit code for runtime failures such as invalid input.
pub const EXIT_ERROR: i32 = 2;
/// Exit code for an unknown command or the wrong number of arguments.
pub const EXIT_USAGE: i32 = 64;

const USAGE: &str = "\
usage: ciutil <command> [args]

commands:
  crc32 STRING        print the CRC-32 of STRING as 8 hex digits
  adler32 STRING      print the Adler-32 of STRING as 8 hex digits
  b64enc STRING       print STRING encoded as base64
  b64dec BASE64       write the decoded bytes to stdout
  roman NUMBER        print NUMBER (1-3999) as a roman numeral
  unroman ROMAN       print a roman numeral as a decimal number
  semver-cmp A B      print -1, 0 or 1 comparing semantic versions A and B
  version             print the program version
";

enum Failure {
    Usage,
    Error(String),
}

impl From<io::Error> for Failure {
    fn from(error: io::Error) -> Self {
        Self::Error(format!("write failed: {error}"))
    }
}

fn fail(context: &str, error: impl Display) -> Failure {
    Failure::Error(format!("{context}: {error}"))
}

fn dispatch(args: &[&str], out: &mut impl Write) -> Result<(), Failure> {
    let Some((&command, operands)) = args.split_first() else {
        return Err(Failure::Usage);
    };
    match (command, operands) {
        ("version", []) => writeln!(out, "ciutil rust 1.0.0")?,
        ("crc32", [text]) => writeln!(out, "{:08x}", crc32::checksum(text.as_bytes()))?,
        ("adler32", [text]) => writeln!(out, "{:08x}", adler32::checksum(text.as_bytes()))?,
        ("b64enc", [text]) => writeln!(out, "{}", base64::encode(text.as_bytes()))?,
        ("b64dec", [text]) => {
            let bytes = base64::decode(text).map_err(|e| fail("base64", e))?;
            out.write_all(&bytes)?;
        }
        ("roman", [text]) => {
            let number: i64 = text.parse().map_err(|e| fail("number", e))?;
            let numeral = roman::to_roman(number).map_err(|e| fail("roman", e))?;
            writeln!(out, "{numeral}")?;
        }
        ("unroman", [text]) => {
            let number = roman::from_roman(text).map_err(|e| fail("unroman", e))?;
            writeln!(out, "{number}")?;
        }
        ("semver-cmp", [a, b]) => {
            let a = semver::parse(a).map_err(|e| fail("version", e))?;
            let b = semver::parse(b).map_err(|e| fail("version", e))?;
            writeln!(out, "{}", semver::compare(&a, &b))?;
        }
        _ => return Err(Failure::Usage),
    }
    Ok(())
}

/// Runs the CLI. `args` excludes the program name. Returns the exit code:
/// 0 on success, [`EXIT_ERROR`] on invalid input, [`EXIT_USAGE`] on misuse.
///
/// # Examples
///
/// ```
/// let mut stdout: Vec<u8> = Vec::new();
/// let mut stderr: Vec<u8> = Vec::new();
/// let code = ciutil::cli::run(&["crc32", "123456789"], &mut stdout, &mut stderr);
/// assert_eq!(code, 0);
/// assert_eq!(stdout, b"cbf43926\n");
/// assert!(stderr.is_empty());
/// ```
pub fn run(args: &[impl AsRef<str>], stdout: &mut impl Write, stderr: &mut impl Write) -> i32 {
    let args: Vec<&str> = args.iter().map(|arg| arg.as_ref()).collect();
    match dispatch(&args, stdout) {
        Ok(()) => 0,
        Err(Failure::Usage) => {
            let _ = stderr.write_all(USAGE.as_bytes());
            EXIT_USAGE
        }
        Err(Failure::Error(message)) => {
            let _ = writeln!(stderr, "ciutil: {message}");
            EXIT_ERROR
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Outcome {
        code: i32,
        stdout: Vec<u8>,
        stderr: String,
    }

    fn exec(args: &[&str]) -> Outcome {
        let mut stdout = Vec::new();
        let mut stderr = Vec::new();
        let code = run(args, &mut stdout, &mut stderr);
        Outcome {
            code,
            stdout,
            stderr: String::from_utf8(stderr).unwrap(),
        }
    }

    fn expect_ok(args: &[&str], expected: &[u8]) {
        let outcome = exec(args);
        assert_eq!(outcome.code, 0, "{args:?}: {}", outcome.stderr);
        assert_eq!(outcome.stdout, expected, "{args:?}");
        assert!(outcome.stderr.is_empty(), "{args:?}");
    }

    fn expect_error(args: &[&str]) {
        let outcome = exec(args);
        assert_eq!(outcome.code, EXIT_ERROR, "{args:?}");
        assert!(outcome.stdout.is_empty(), "{args:?}");
        assert!(outcome.stderr.starts_with("ciutil: "), "{args:?}");
        assert!(outcome.stderr.ends_with('\n'), "{args:?}");
    }

    fn expect_usage(args: &[&str]) {
        let outcome = exec(args);
        assert_eq!(outcome.code, EXIT_USAGE, "{args:?}");
        assert!(outcome.stdout.is_empty(), "{args:?}");
        assert!(outcome.stderr.starts_with("usage: ciutil"), "{args:?}");
    }

    #[test]
    fn checksums() {
        expect_ok(&["crc32", "123456789"], b"cbf43926\n");
        expect_ok(&["crc32", ""], b"00000000\n");
        expect_ok(&["adler32", "Wikipedia"], b"11e60398\n");
        expect_ok(&["adler32", ""], b"00000001\n");
    }

    #[test]
    fn base64_commands() {
        expect_ok(&["b64enc", "foobar"], b"Zm9vYmFy\n");
        expect_ok(&["b64enc", "f"], b"Zg==\n");
        expect_ok(&["b64enc", ""], b"\n");
        expect_ok(&["b64dec", "Zm9vYg=="], b"foob");
        expect_ok(&["b64dec", ""], b"");
        expect_ok(&["b64dec", "AP/+"], &[0x00, 0xFF, 0xFE]);
    }

    #[test]
    fn utf8_input_is_treated_as_raw_bytes() {
        let text = "h\u{e9}llo";
        let encoded = exec(&["b64enc", text]);
        assert_eq!(encoded.code, 0);
        let encoded = String::from_utf8(encoded.stdout).unwrap();
        let encoded = encoded.trim_end_matches('\n');
        expect_ok(&["b64dec", encoded], text.as_bytes());
    }

    #[test]
    fn roman_commands() {
        expect_ok(&["roman", "1994"], b"MCMXCIV\n");
        expect_ok(&["roman", "3999"], b"MMMCMXCIX\n");
        expect_ok(&["unroman", "MMXXIV"], b"2024\n");
        expect_ok(&["unroman", "IV"], b"4\n");
    }

    #[test]
    fn semver_compare() {
        expect_ok(&["semver-cmp", "1.0.0-alpha", "1.0.0"], b"-1\n");
        expect_ok(&["semver-cmp", "1.0.0", "1.0.0-alpha"], b"1\n");
        expect_ok(&["semver-cmp", "1.0.0+a", "1.0.0+b"], b"0\n");
        expect_ok(&["semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11"], b"-1\n");
    }

    #[test]
    fn version_command() {
        expect_ok(&["version"], b"ciutil rust 1.0.0\n");
    }

    #[test]
    fn invalid_input_exits_with_error_code() {
        expect_error(&["b64dec", "Zg"]);
        expect_error(&["b64dec", "Zh=="]);
        expect_error(&["b64dec", "Zm9-"]);
        expect_error(&["roman", "0"]);
        expect_error(&["roman", "4000"]);
        expect_error(&["roman", "-1"]);
        expect_error(&["roman", "abc"]);
        expect_error(&["roman", ""]);
        expect_error(&["roman", "99999999999999999999999"]);
        expect_error(&["unroman", ""]);
        expect_error(&["unroman", "IIII"]);
        expect_error(&["unroman", "xiv"]);
        expect_error(&["semver-cmp", "1.0", "1.0.0"]);
        expect_error(&["semver-cmp", "1.0.0", "v1.0.0"]);
    }

    #[test]
    fn misuse_exits_with_usage_code() {
        let none: [&str; 0] = [];
        let outcome = exec(&none);
        assert_eq!(outcome.code, EXIT_USAGE);
        assert!(outcome.stderr.starts_with("usage: ciutil"));

        expect_usage(&["frobnicate"]);
        expect_usage(&["frobnicate", "x"]);
        expect_usage(&["crc32"]);
        expect_usage(&["crc32", "a", "b"]);
        expect_usage(&["adler32"]);
        expect_usage(&["b64enc"]);
        expect_usage(&["b64dec", "a", "b"]);
        expect_usage(&["roman"]);
        expect_usage(&["unroman", "I", "I"]);
        expect_usage(&["semver-cmp"]);
        expect_usage(&["semver-cmp", "1.0.0"]);
        expect_usage(&["semver-cmp", "1.0.0", "1.0.0", "1.0.0"]);
        expect_usage(&["version", "extra"]);
        expect_usage(&["CRC32", "a"]);
    }

    #[test]
    fn accepts_owned_strings() {
        let args = ["crc32".to_owned(), "abc".to_owned()];
        let mut stdout: Vec<u8> = Vec::new();
        let mut stderr: Vec<u8> = Vec::new();
        assert_eq!(run(&args, &mut stdout, &mut stderr), 0);
        assert_eq!(stdout, b"352441c2\n");
    }

    #[test]
    fn write_failures_are_reported() {
        struct Broken;

        impl Write for Broken {
            fn write(&mut self, _: &[u8]) -> io::Result<usize> {
                Err(io::Error::other("closed"))
            }

            fn flush(&mut self) -> io::Result<()> {
                Ok(())
            }
        }

        let mut stderr = Vec::new();
        let code = run(&["crc32", "abc"], &mut Broken, &mut stderr);
        assert_eq!(code, EXIT_ERROR);
        let message = String::from_utf8(stderr).unwrap();
        assert_eq!(message, "ciutil: write failed: closed\n");
    }
}
