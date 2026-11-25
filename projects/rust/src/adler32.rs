//! Adler-32 as specified by RFC 1950.

const MODULUS: u32 = 65_521;

// largest run of bytes after which the sums are guaranteed not to overflow u32
const NMAX: usize = 5552;

/// Incremental Adler-32 state.
///
/// # Examples
///
/// ```
/// use ciutil::adler32::Adler32;
///
/// let mut adler = Adler32::new();
/// adler.update(b"Wiki");
/// adler.update(b"pedia");
/// assert_eq!(adler.finalize(), 0x11e6_0398);
/// ```
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Adler32 {
    a: u32,
    b: u32,
}

impl Default for Adler32 {
    fn default() -> Self {
        Self::new()
    }
}

impl Adler32 {
    /// Starts a new computation over empty input.
    pub const fn new() -> Self {
        Self { a: 1, b: 0 }
    }

    /// Feeds more bytes into the computation.
    pub fn update(&mut self, data: &[u8]) {
        let mut a = self.a;
        let mut b = self.b;
        for block in data.chunks(NMAX) {
            for &byte in block {
                a += u32::from(byte);
                b += a;
            }
            a %= MODULUS;
            b %= MODULUS;
        }
        self.a = a;
        self.b = b;
    }

    /// Returns the checksum of everything fed so far. The state is left
    /// untouched, so more data may be added afterwards.
    pub const fn finalize(&self) -> u32 {
        (self.b << 16) | self.a
    }
}

/// Computes the Adler-32 checksum of `data` in one call.
///
/// # Examples
///
/// ```
/// assert_eq!(ciutil::adler32::checksum(b""), 1);
/// assert_eq!(ciutil::adler32::checksum(b"abc"), 0x024d_0127);
/// ```
pub fn checksum(data: &[u8]) -> u32 {
    let mut adler = Adler32::new();
    adler.update(data);
    adler.finalize()
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

    fn naive_reference(data: &[u8]) -> u32 {
        let mut a = 1u32;
        let mut b = 0u32;
        for &byte in data {
            a = (a + u32::from(byte)) % MODULUS;
            b = (b + a) % MODULUS;
        }
        (b << 16) | a
    }

    fn check(data: &[u8], expected: u32) {
        assert_eq!(checksum(data), expected);
    }

    #[test]
    fn known_vectors() {
        check(b"", 0x0000_0001);
        check(b"a", 0x0062_0062);
        check(b"abc", 0x024d_0127);
        check(b"Wikipedia", 0x11e6_0398);
        check(b"123456789", 0x091e_01de);
    }

    #[test]
    fn long_run_of_ff_matches_naive_reference() {
        let data = vec![0xFFu8; 100_000];
        assert_eq!(checksum(&data), naive_reference(&data));
    }

    #[test]
    fn lengths_around_nmax_match_naive_reference() {
        let data = pseudo_random(3 * NMAX + 5);
        for len in [
            NMAX - 1,
            NMAX,
            NMAX + 1,
            2 * NMAX - 1,
            2 * NMAX,
            2 * NMAX + 1,
            3 * NMAX,
            3 * NMAX + 5,
        ] {
            assert_eq!(checksum(&data[..len]), naive_reference(&data[..len]));
        }
    }

    #[test]
    fn all_ff_blocks_match_naive_reference_at_nmax_boundaries() {
        for len in [NMAX - 1, NMAX, NMAX + 1, 2 * NMAX, 2 * NMAX + 1] {
            let data = vec![0xFFu8; len];
            assert_eq!(checksum(&data), naive_reference(&data), "length {len}");
        }
    }

    #[test]
    fn chunked_equals_one_shot_at_every_split() {
        let data = pseudo_random(1000);
        let whole = checksum(&data);
        assert_eq!(whole, naive_reference(&data));
        for split in 0..=data.len() {
            let mut adler = Adler32::new();
            adler.update(&data[..split]);
            adler.update(&data[split..]);
            assert_eq!(adler.finalize(), whole, "split at {split}");
        }
    }

    #[test]
    fn chunked_long_input_equals_one_shot() {
        let data = vec![0xFFu8; 100_000];
        let whole = checksum(&data);
        for size in [1, 7, 4096, NMAX - 1, NMAX, NMAX + 1, 99_999] {
            let mut adler = Adler32::new();
            for chunk in data.chunks(size) {
                adler.update(chunk);
            }
            assert_eq!(adler.finalize(), whole, "chunk size {size}");
        }
    }

    #[test]
    fn finalize_does_not_consume_state() {
        let mut adler = Adler32::default();
        adler.update(b"Wiki");
        let partial = adler.finalize();
        assert_eq!(partial, checksum(b"Wiki"));
        adler.update(b"pedia");
        assert_eq!(adler.finalize(), 0x11e6_0398);
    }
}
