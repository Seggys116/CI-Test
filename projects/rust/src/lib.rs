//! Checksum, encoding, roman numeral and semver utilities.
//!
//! Every algorithm is implemented by hand on top of the standard library, so
//! the crate has no dependencies.

pub mod adler32;
pub mod base64;
pub mod cli;
pub mod crc32;
pub mod roman;
pub mod semver;
