//! CRC-32/IEEE: reflected, polynomial `0xEDB88320`, initial value and final
//! xor of `0xFFFFFFFF`.

const POLY: u32 = 0xEDB8_8320;
const TABLE: [u32; 256] = make_table();

const fn make_table() -> [u32; 256] {
    let mut table = [0u32; 256];
    let mut i = 0;
    while i < 256 {
        let mut crc = i as u32;
        let mut bit = 0;
        while bit < 8 {
            crc = (crc >> 1) ^ (POLY & (crc & 1).wrapping_neg());
            bit += 1;
        }
        table[i] = crc;
        i += 1;
    }
    table
}

/// Incremental CRC-32 state.
///
/// # Examples
///
/// ```
/// use ciutil::crc32::Crc32;
///
/// let mut crc = Crc32::new();
/// crc.update(b"1234");
/// crc.update(b"56789");
/// assert_eq!(crc.finalize(), 0xcbf4_3926);
/// ```
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Crc32 {
    state: u32,
}

impl Default for Crc32 {
    fn default() -> Self {
        Self::new()
    }
}

impl Crc32 {
    /// Starts a new computation over empty input.
    pub const fn new() -> Self {
        Self { state: 0xFFFF_FFFF }
    }

    /// Feeds more bytes into the computation.
    pub fn update(&mut self, data: &[u8]) {
        let mut crc = self.state;
        for &byte in data {
            let index = (crc ^ u32::from(byte)) & 0xFF;
            crc = TABLE[index as usize] ^ (crc >> 8);
        }
        self.state = crc;
    }

    /// Returns the checksum of everything fed so far. The state is left
    /// untouched, so more data may be added afterwards.
    pub const fn finalize(&self) -> u32 {
        self.state ^ 0xFFFF_FFFF
    }
}

/// Computes the CRC-32 of `data` in one call.
///
/// # Examples
///
/// ```
/// assert_eq!(ciutil::crc32::checksum(b""), 0);
/// assert_eq!(ciutil::crc32::checksum(b"abc"), 0x3524_41c2);
/// ```
pub fn checksum(data: &[u8]) -> u32 {
    let mut crc = Crc32::new();
    crc.update(data);
    crc.finalize()
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

    fn bitwise_reference(data: &[u8]) -> u32 {
        let mut crc = 0xFFFF_FFFFu32;
        for &byte in data {
            crc ^= u32::from(byte);
            for _ in 0..8 {
                crc = if crc & 1 == 1 {
                    (crc >> 1) ^ POLY
                } else {
                    crc >> 1
                };
            }
        }
        !crc
    }

    fn check(data: &[u8], expected: u32) {
        assert_eq!(
            checksum(data),
            expected,
            "one-shot mismatch for {} bytes",
            data.len()
        );
    }

    #[test]
    fn known_vectors() {
        check(b"", 0x0000_0000);
        check(b"a", 0xe8b7_be43);
        check(b"abc", 0x3524_41c2);
        check(b"123456789", 0xcbf4_3926);
        let fox = b"The quick brown fox jumps over the lazy dog";
        check(fox, 0x414f_a339);
        check(&[0x00; 32], 0x190a_55ad);
        check(&[0xFF; 32], 0xff6c_ab0b);
    }

    #[test]
    fn table_has_expected_anchors() {
        assert_eq!(TABLE[0], 0);
        assert_eq!(TABLE[1], 0x7707_3096);
        assert_eq!(TABLE[255], 0x2D02_EF8D);
    }

    #[test]
    fn matches_bitwise_reference() {
        let data = pseudo_random(1000);
        for len in [0, 1, 2, 3, 7, 8, 9, 255, 256, 1000] {
            assert_eq!(checksum(&data[..len]), bitwise_reference(&data[..len]));
        }
    }

    #[test]
    fn chunked_equals_one_shot_at_every_split() {
        let data = pseudo_random(1000);
        let whole = checksum(&data);
        for split in 0..=data.len() {
            let mut crc = Crc32::new();
            crc.update(&data[..split]);
            crc.update(&data[split..]);
            assert_eq!(crc.finalize(), whole, "split at {split}");
        }
    }

    #[test]
    fn many_small_chunks_equal_one_shot() {
        let data = pseudo_random(1000);
        let whole = checksum(&data);
        for size in [1, 2, 3, 7, 64, 999] {
            let mut crc = Crc32::new();
            for chunk in data.chunks(size) {
                crc.update(chunk);
            }
            assert_eq!(crc.finalize(), whole, "chunk size {size}");
        }
    }

    #[test]
    fn empty_updates_change_nothing() {
        let mut crc = Crc32::new();
        crc.update(b"");
        crc.update(b"abc");
        crc.update(b"");
        assert_eq!(crc.finalize(), 0x3524_41c2);
    }

    #[test]
    fn finalize_does_not_consume_state() {
        let mut crc = Crc32::default();
        crc.update(b"12345");
        let partial = crc.finalize();
        assert_eq!(partial, checksum(b"12345"));
        crc.update(b"6789");
        assert_eq!(crc.finalize(), 0xcbf4_3926);
    }
}
