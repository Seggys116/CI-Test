//! Roman numerals in canonical subtractive form, for 1 through 3999.

use std::error::Error;
use std::fmt;

const MAX: u16 = 3999;

const NUMERALS: [(u16, &str); 13] = [
    (1000, "M"),
    (900, "CM"),
    (500, "D"),
    (400, "CD"),
    (100, "C"),
    (90, "XC"),
    (50, "L"),
    (40, "XL"),
    (10, "X"),
    (9, "IX"),
    (5, "V"),
    (4, "IV"),
    (1, "I"),
];

/// Reasons why a conversion is rejected.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RomanError {
    /// The number is outside 1 through 3999.
    OutOfRange,
    /// The text is not a canonical roman numeral.
    Invalid,
}

impl fmt::Display for RomanError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::OutOfRange => f.write_str("number must be between 1 and 3999"),
            Self::Invalid => f.write_str("not a canonical roman numeral"),
        }
    }
}

impl Error for RomanError {}

/// Converts `n` to its canonical uppercase roman numeral.
///
/// # Examples
///
/// ```
/// use ciutil::roman::{to_roman, RomanError};
///
/// assert_eq!(to_roman(1994).as_deref(), Ok("MCMXCIV"));
/// assert_eq!(to_roman(0), Err(RomanError::OutOfRange));
/// assert_eq!(to_roman(4000), Err(RomanError::OutOfRange));
/// ```
pub fn to_roman(n: i64) -> Result<String, RomanError> {
    let mut remaining = match u16::try_from(n) {
        Ok(value) if (1..=MAX).contains(&value) => value,
        _ => return Err(RomanError::OutOfRange),
    };
    let mut out = String::new();
    for &(value, symbol) in &NUMERALS {
        while remaining >= value {
            out.push_str(symbol);
            remaining -= value;
        }
    }
    Ok(out)
}

/// Parses a canonical uppercase roman numeral. Anything that is not exactly
/// what [`to_roman`] would produce is rejected.
///
/// # Examples
///
/// ```
/// use ciutil::roman::{from_roman, RomanError};
///
/// assert_eq!(from_roman("MMXXIV"), Ok(2024));
/// assert_eq!(from_roman("IIII"), Err(RomanError::Invalid));
/// assert_eq!(from_roman("xiv"), Err(RomanError::Invalid));
/// ```
pub fn from_roman(text: &str) -> Result<u16, RomanError> {
    let mut rest = text;
    let mut total: u16 = 0;
    for &(value, symbol) in &NUMERALS {
        while let Some(tail) = rest.strip_prefix(symbol) {
            total += value;
            if total > MAX {
                return Err(RomanError::Invalid);
            }
            rest = tail;
        }
    }
    match to_roman(i64::from(total)) {
        Ok(canonical) if rest.is_empty() && canonical == text => Ok(total),
        _ => Err(RomanError::Invalid),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn check(n: u16, numeral: &str) {
        let produced = to_roman(i64::from(n));
        assert_eq!(produced.as_deref(), Ok(numeral), "to_roman({n})");
        assert_eq!(from_roman(numeral), Ok(n), "from_roman({numeral})");
    }

    fn reject(text: &str) {
        let result = from_roman(text);
        assert_eq!(result, Err(RomanError::Invalid), "accepted {text:?}");
    }

    #[test]
    fn known_vectors() {
        check(1, "I");
        check(4, "IV");
        check(9, "IX");
        check(14, "XIV");
        check(40, "XL");
        check(90, "XC");
        check(400, "CD");
        check(1994, "MCMXCIV");
        check(1999, "MCMXCIX");
        check(2024, "MMXXIV");
        check(3999, "MMMCMXCIX");
    }

    #[test]
    fn to_roman_rejects_out_of_range() {
        for n in [0, 4000, 4001, -1, -3999, i64::MIN, i64::MAX] {
            assert_eq!(to_roman(n), Err(RomanError::OutOfRange), "to_roman({n})");
        }
    }

    #[test]
    fn from_roman_rejects_non_canonical_forms() {
        reject("");
        reject("IIII");
        reject("VX");
        reject("IC");
        reject("XM");
        reject("VV");
        reject("IIX");
        reject("xiv");
        reject("Xiv");
        reject("IVI");
        reject("VIV");
        reject("IXI");
        reject("IIV");
        reject("XXXX");
        reject("CCCC");
        reject("MMMM");
        reject("LL");
        reject("DD");
        reject("XVX");
        reject("MCMC");
        reject("CMD");
        reject("CDC");
        reject("IL");
        reject("XD");
        reject("MMMMCMXCIX");
        reject(" X");
        reject("X ");
        reject("X\n");
        reject("I I");
        reject("N");
        reject("0");
        reject("\u{216b}");
    }

    #[test]
    fn exhaustive_round_trip() {
        for n in 1..=MAX {
            let numeral = to_roman(i64::from(n)).unwrap();
            assert_eq!(from_roman(&numeral), Ok(n), "numeral {numeral}");
        }
    }

    #[test]
    fn every_numeral_is_unique_and_uppercase_ascii() {
        let mut seen = std::collections::HashSet::new();
        for n in 1..=MAX {
            let numeral = to_roman(i64::from(n)).unwrap();
            assert!(numeral.bytes().all(|b| b"IVXLCDM".contains(&b)));
            assert!(seen.insert(numeral));
        }
        assert_eq!(seen.len(), usize::from(MAX));
    }

    #[test]
    fn error_messages_are_descriptive() {
        let text = RomanError::OutOfRange.to_string();
        assert_eq!(text, "number must be between 1 and 3999");
        let text = RomanError::Invalid.to_string();
        assert_eq!(text, "not a canonical roman numeral");
    }
}
