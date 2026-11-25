//! Semantic Versioning 2.0.0 parsing and precedence.

use std::cmp::Ordering;
use std::error::Error;
use std::fmt;
use std::str::FromStr;

/// A parsed semantic version.
///
/// Equality is structural and includes build metadata; use
/// [`Version::cmp_precedence`] or [`compare`] for SemVer precedence, which
/// ignores it.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Version {
    pub major: u64,
    pub minor: u64,
    pub patch: u64,
    pub prerelease: Vec<String>,
    pub build: Vec<String>,
}

/// Reasons why [`parse`] rejects its input.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ParseError {
    /// The core is not exactly three dot-separated non-negative integers.
    InvalidCore,
    /// A numeric field or numeric prerelease identifier has a leading zero.
    LeadingZero,
    /// A numeric field does not fit in a `u64`.
    Overflow,
    /// A prerelease or build identifier is empty.
    EmptyIdentifier,
    /// A prerelease or build identifier contains a character outside
    /// `[0-9A-Za-z-]`.
    InvalidIdentifier,
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InvalidCore => f.write_str("expected MAJOR.MINOR.PATCH"),
            Self::LeadingZero => f.write_str("numeric field has a leading zero"),
            Self::Overflow => f.write_str("numeric field does not fit in 64 bits"),
            Self::EmptyIdentifier => f.write_str("empty identifier"),
            Self::InvalidIdentifier => f.write_str("identifier contains an invalid character"),
        }
    }
}

impl Error for ParseError {}

fn is_numeric(identifier: &str) -> bool {
    !identifier.is_empty() && identifier.bytes().all(|b| b.is_ascii_digit())
}

fn is_identifier_byte(byte: u8) -> bool {
    byte.is_ascii_alphanumeric() || byte == b'-'
}

fn parse_number(field: &str) -> Result<u64, ParseError> {
    if !is_numeric(field) {
        return Err(ParseError::InvalidCore);
    }
    if field.len() > 1 && field.starts_with('0') {
        return Err(ParseError::LeadingZero);
    }
    field.parse().map_err(|_| ParseError::Overflow)
}

fn parse_identifiers(text: &str, is_prerelease: bool) -> Result<Vec<String>, ParseError> {
    let mut identifiers = Vec::new();
    for identifier in text.split('.') {
        if identifier.is_empty() {
            return Err(ParseError::EmptyIdentifier);
        }
        if !identifier.bytes().all(is_identifier_byte) {
            return Err(ParseError::InvalidIdentifier);
        }
        let leading_zero = identifier.len() > 1 && identifier.starts_with('0');
        if is_prerelease && is_numeric(identifier) && leading_zero {
            return Err(ParseError::LeadingZero);
        }
        identifiers.push(identifier.to_owned());
    }
    Ok(identifiers)
}

/// Parses `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`.
///
/// # Examples
///
/// ```
/// use ciutil::semver::{parse, ParseError};
///
/// let version = parse("1.2.3-rc.1+build.5").unwrap();
/// assert_eq!((version.major, version.minor, version.patch), (1, 2, 3));
/// assert_eq!(version.prerelease, ["rc", "1"]);
/// assert_eq!(version.build, ["build", "5"]);
/// assert_eq!(parse("01.0.0"), Err(ParseError::LeadingZero));
/// assert_eq!(parse("1.0"), Err(ParseError::InvalidCore));
/// ```
pub fn parse(input: &str) -> Result<Version, ParseError> {
    let (rest, build) = match input.split_once('+') {
        Some((rest, build)) => (rest, parse_identifiers(build, false)?),
        None => (input, Vec::new()),
    };
    let (core, prerelease) = match rest.split_once('-') {
        Some((core, prerelease)) => (core, parse_identifiers(prerelease, true)?),
        None => (rest, Vec::new()),
    };

    let fields: Vec<&str> = core.split('.').collect();
    let [major, minor, patch] = fields.as_slice() else {
        return Err(ParseError::InvalidCore);
    };
    Ok(Version {
        major: parse_number(major)?,
        minor: parse_number(minor)?,
        patch: parse_number(patch)?,
        prerelease,
        build,
    })
}

fn compare_identifiers(a: &str, b: &str) -> Ordering {
    match (is_numeric(a), is_numeric(b)) {
        // no leading zeros, so a longer digit string is a larger number
        (true, true) => a.len().cmp(&b.len()).then_with(|| a.cmp(b)),
        (true, false) => Ordering::Less,
        (false, true) => Ordering::Greater,
        (false, false) => a.cmp(b),
    }
}

fn compare_prerelease(a: &[String], b: &[String]) -> Ordering {
    match (a.is_empty(), b.is_empty()) {
        (true, true) => return Ordering::Equal,
        (true, false) => return Ordering::Greater,
        (false, true) => return Ordering::Less,
        (false, false) => {}
    }
    for (x, y) in a.iter().zip(b) {
        let ordering = compare_identifiers(x, y);
        if ordering != Ordering::Equal {
            return ordering;
        }
    }
    a.len().cmp(&b.len())
}

impl Version {
    /// Compares by SemVer precedence. Build metadata is ignored.
    pub fn cmp_precedence(&self, other: &Self) -> Ordering {
        self.major
            .cmp(&other.major)
            .then(self.minor.cmp(&other.minor))
            .then(self.patch.cmp(&other.patch))
            .then_with(|| compare_prerelease(&self.prerelease, &other.prerelease))
    }
}

impl fmt::Display for Version {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}.{}.{}", self.major, self.minor, self.patch)?;
        if !self.prerelease.is_empty() {
            write!(f, "-{}", self.prerelease.join("."))?;
        }
        if !self.build.is_empty() {
            write!(f, "+{}", self.build.join("."))?;
        }
        Ok(())
    }
}

impl FromStr for Version {
    type Err = ParseError;

    fn from_str(text: &str) -> Result<Self, Self::Err> {
        parse(text)
    }
}

/// Returns -1, 0 or 1 when `a` has lower, equal or higher precedence than `b`.
///
/// # Examples
///
/// ```
/// use ciutil::semver::{compare, parse};
///
/// let beta = parse("1.0.0-beta.2").unwrap();
/// let release = parse("1.0.0+anything").unwrap();
/// assert_eq!(compare(&beta, &release), -1);
/// assert_eq!(compare(&release, &beta), 1);
/// assert_eq!(compare(&release, &parse("1.0.0").unwrap()), 0);
/// ```
pub fn compare(a: &Version, b: &Version) -> i32 {
    match a.cmp_precedence(b) {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const CHAIN: [&str; 8] = [
        "1.0.0-alpha",
        "1.0.0-alpha.1",
        "1.0.0-alpha.beta",
        "1.0.0-beta",
        "1.0.0-beta.2",
        "1.0.0-beta.11",
        "1.0.0-rc.1",
        "1.0.0",
    ];

    fn v(text: &str) -> Version {
        parse(text).unwrap_or_else(|e| panic!("{text:?} should parse: {e}"))
    }

    fn reject(text: &str) {
        assert!(parse(text).is_err(), "accepted {text:?}");
    }

    fn accept(text: &str) {
        let version = v(text);
        assert_eq!(version.to_string(), text, "round trip of {text:?}");
        assert_eq!(v(&version.to_string()), version);
    }

    #[test]
    fn parses_all_components() {
        let version = v("1.2.3-rc.1+build.5");
        assert_eq!(version.major, 1);
        assert_eq!(version.minor, 2);
        assert_eq!(version.patch, 3);
        assert_eq!(version.prerelease, ["rc", "1"]);
        assert_eq!(version.build, ["build", "5"]);

        let plain = v("10.20.30");
        assert!(plain.prerelease.is_empty());
        assert!(plain.build.is_empty());
    }

    #[test]
    fn accepts_valid_versions() {
        accept("0.0.0");
        accept("1.0.0-0");
        accept("1.0.0-0a");
        accept("1.0.0-x-y-z.-");
        accept("1.0.0+001");
        accept("1.0.0-rc.1+build.5");
        accept("1.0.0-alpha.beta");
        accept("1.0.0-alpha+001");
        accept("1.0.0+20130313144700");
        accept("1.0.0-0.3.7");
        accept("1.0.0-a-b.c-d+e-f.g");
        accept("1.0.0---");
        accept("999.999.999");
    }

    #[test]
    fn rejects_malformed_versions() {
        reject("");
        reject("1");
        reject("1.0");
        reject("1.0.0.0");
        reject("v1.0.0");
        reject("01.0.0");
        reject("1.00.0");
        reject("1.0.00");
        reject("1.0.0-");
        reject("1.0.0+");
        reject("1.0.0-01");
        reject("1.0.0-a..b");
        reject("1.0.0-a_b");
        reject("-1.0.0");
        reject(" 1.0.0");
        reject("1.0.0 ");
        reject("1.0.0-alpha+");
        reject("1.0.0-alpha.");
        reject("1.0.0-.alpha");
        reject("1.0.0+a..b");
        reject("1.0.0+a_b");
        reject("1.0.0+a+b");
        reject("1.0.0-alpha.01");
        reject("1..0");
        reject("..");
        reject("1.a.0");
        reject("1.0.-1");
        reject("1.0.+1");
        reject("+1.0.0");
        reject("1.0.0\n");
        reject("1.0.0-\u{e9}");
        reject("\u{661}.0.0");
    }

    #[test]
    fn reports_specific_errors() {
        assert_eq!(parse("1.0"), Err(ParseError::InvalidCore));
        assert_eq!(parse("1.0.0.0"), Err(ParseError::InvalidCore));
        assert_eq!(parse("v1.0.0"), Err(ParseError::InvalidCore));
        assert_eq!(parse("01.0.0"), Err(ParseError::LeadingZero));
        assert_eq!(parse("1.0.0-01"), Err(ParseError::LeadingZero));
        assert_eq!(parse("1.0.0-"), Err(ParseError::EmptyIdentifier));
        assert_eq!(parse("1.0.0-a..b"), Err(ParseError::EmptyIdentifier));
        assert_eq!(parse("1.0.0-a_b"), Err(ParseError::InvalidIdentifier));
    }

    #[test]
    fn numeric_fields_must_fit_in_u64() {
        let max = v("18446744073709551615.0.0");
        assert_eq!(max.major, u64::MAX);
        assert_eq!(v("0.18446744073709551615.0").minor, u64::MAX);
        assert_eq!(v("0.0.18446744073709551615").patch, u64::MAX);
        for text in [
            "18446744073709551616.0.0",
            "0.18446744073709551616.0",
            "0.0.99999999999999999999",
        ] {
            assert_eq!(parse(text), Err(ParseError::Overflow), "{text}");
        }
    }

    #[test]
    fn huge_numeric_prerelease_identifiers_compare_numerically() {
        let small = v("1.0.0-99999999999999999999");
        let large = v("1.0.0-100000000000000000000");
        assert_eq!(compare(&small, &large), -1);
        assert_eq!(compare(&large, &small), 1);
    }

    #[test]
    fn ordering_chain_holds_for_all_pairs() {
        let chain = CHAIN.map(v);
        for (i, a) in chain.iter().enumerate() {
            for (j, b) in chain.iter().enumerate() {
                let expected = match i.cmp(&j) {
                    Ordering::Less => -1,
                    Ordering::Equal => 0,
                    Ordering::Greater => 1,
                };
                assert_eq!(compare(a, b), expected, "{a} vs {b}");
            }
        }
    }

    #[test]
    fn release_ordering() {
        let releases = ["0.9.9", "1.0.0", "2.0.0", "2.1.0", "2.1.1", "10.0.0"];
        for (i, a) in releases.iter().enumerate() {
            for (j, b) in releases.iter().enumerate() {
                let expected = match i.cmp(&j) {
                    Ordering::Less => -1,
                    Ordering::Equal => 0,
                    Ordering::Greater => 1,
                };
                assert_eq!(compare(&v(a), &v(b)), expected, "{a} vs {b}");
            }
        }
    }

    #[test]
    fn identifier_rules() {
        assert_eq!(compare(&v("1.0.0-beta.2"), &v("1.0.0-beta.11")), -1);
        assert_eq!(compare(&v("1.0.0-1"), &v("1.0.0-a")), -1);
        assert_eq!(compare(&v("1.0.0-a"), &v("1.0.0-1")), 1);
        assert_eq!(compare(&v("1.0.0-B"), &v("1.0.0-a")), -1);
        assert_eq!(compare(&v("1.0.0-a-b"), &v("1.0.0-a")), 1);
        assert_eq!(compare(&v("1.0.0-alpha.1"), &v("1.0.0-alpha")), 1);
        assert_eq!(compare(&v("1.0.0-0a"), &v("1.0.0-0")), 1);
        assert_eq!(compare(&v("1.0.0-9"), &v("1.0.0-10")), -1);
    }

    #[test]
    fn build_metadata_is_ignored() {
        assert_eq!(compare(&v("1.0.0+a"), &v("1.0.0+b")), 0);
        assert_eq!(compare(&v("1.0.0-rc.1+a"), &v("1.0.0-rc.1")), 0);
        assert_eq!(compare(&v("1.0.0+z"), &v("1.0.1+a")), -1);
        assert_ne!(v("1.0.0+a"), v("1.0.0+b"));
    }

    #[test]
    fn compare_is_antisymmetric() {
        let samples = [
            "0.0.0",
            "1.0.0-alpha",
            "1.0.0-alpha.1",
            "1.0.0-1",
            "1.0.0",
            "1.0.0+x",
            "1.2.3-rc.1",
            "2.0.0",
        ];
        for a in samples {
            for b in samples {
                let forward = compare(&v(a), &v(b));
                let backward = compare(&v(b), &v(a));
                assert_eq!(forward, -backward, "{a} vs {b}");
            }
        }
    }

    #[test]
    fn display_round_trips_through_parse() {
        for text in CHAIN {
            let version = v(text);
            assert_eq!(version.to_string(), text);
            assert_eq!(text.parse::<Version>(), Ok(version));
        }
    }

    #[test]
    fn error_messages_are_descriptive() {
        let text = ParseError::InvalidCore.to_string();
        assert_eq!(text, "expected MAJOR.MINOR.PATCH");
        let text = ParseError::LeadingZero.to_string();
        assert_eq!(text, "numeric field has a leading zero");
        let text = ParseError::Overflow.to_string();
        assert_eq!(text, "numeric field does not fit in 64 bits");
        let text = ParseError::EmptyIdentifier.to_string();
        assert_eq!(text, "empty identifier");
        let text = ParseError::InvalidIdentifier.to_string();
        assert_eq!(text, "identifier contains an invalid character");
    }
}
