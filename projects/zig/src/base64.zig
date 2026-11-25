//! Base64 as specified by RFC 4648 (standard alphabet, with padding) and a
//! strict decoder. Callers provide the output buffers.

const std = @import("std");

pub const DecodeError = error{
    /// the input length is not a multiple of four
    InvalidLength,
    /// a byte outside the standard alphabet
    InvalidCharacter,
    /// '=' anywhere other than as the last one or two characters
    InvalidPadding,
    /// the unused trailing bits of the final group are not zero
    NonCanonical,
    /// the output buffer is smaller than decodedLenMax
    NoSpaceLeft,
};

const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const invalid: u8 = 0xFF;

const decode_table: [256]u8 = blk: {
    var t = [_]u8{invalid} ** 256;
    for (alphabet, 0..) |c, i| {
        t[c] = @intCast(i);
    }
    break :blk t;
};

/// number of bytes encode writes for n input bytes
pub fn encodedLen(n: usize) usize {
    return (n + 2) / 3 * 4;
}

/// upper bound for the bytes decode writes for n input bytes
pub fn decodedLenMax(n: usize) usize {
    return n / 4 * 3;
}

/// encodes src into dest, which must hold at least encodedLen(src.len) bytes
pub fn encode(dest: []u8, src: []const u8) []const u8 {
    std.debug.assert(dest.len >= encodedLen(src.len));
    const shifts = [3]u5{ 16, 8, 0 };
    var out: usize = 0;
    var i: usize = 0;
    while (i < src.len) : (i += 3) {
        const chunk = src[i..@min(i + 3, src.len)];
        var triple: u32 = 0;
        for (chunk, 0..) |byte, j| {
            triple |= @as(u32, byte) << shifts[j];
        }
        const groups = [4]u32{ triple >> 18, triple >> 12, triple >> 6, triple };
        for (0..chunk.len + 1) |k| {
            dest[out] = alphabet[groups[k] & 0x3F];
            out += 1;
        }
        for (chunk.len..3) |_| {
            dest[out] = '=';
            out += 1;
        }
    }
    return dest[0..out];
}

/// decodes strictly into dest, which must hold at least decodedLenMax(src.len)
/// bytes, and returns the number of bytes written
pub fn decode(dest: []u8, src: []const u8) DecodeError!usize {
    if (src.len % 4 != 0) return error.InvalidLength;
    if (dest.len < decodedLenMax(src.len)) return error.NoSpaceLeft;
    const quads = src.len / 4;
    var out: usize = 0;
    for (0..quads) |q| {
        const is_last = q + 1 == quads;
        var values = [4]u32{ 0, 0, 0, 0 };
        var padding: usize = 0;
        for (0..4) |i| {
            const c = src[q * 4 + i];
            if (c == '=') {
                if (!is_last or i < 2) return error.InvalidPadding;
                padding += 1;
            } else if (padding > 0) {
                return error.InvalidPadding;
            } else {
                const value = decode_table[c];
                if (value == invalid) return error.InvalidCharacter;
                values[i] = value;
            }
        }

        const stray_bits: u32 = switch (padding) {
            1 => values[2] & 0x03,
            2 => values[1] & 0x0F,
            else => 0,
        };
        if (stray_bits != 0) return error.NonCanonical;

        const triple = (values[0] << 18) | (values[1] << 12) | (values[2] << 6) | values[3];
        const bytes = [3]u8{ @truncate(triple >> 16), @truncate(triple >> 8), @truncate(triple) };
        const count = 3 - padding;
        @memcpy(dest[out..][0..count], bytes[0..count]);
        out += count;
    }
    return out;
}

const testing = std.testing;
const testutil = @import("testutil.zig");

fn expectVector(data: []const u8, encoded: []const u8) !void {
    var enc_buf: [512]u8 = undefined;
    try testing.expectEqualStrings(encoded, encode(&enc_buf, data));
    var dec_buf: [512]u8 = undefined;
    const n = try decode(&dec_buf, encoded);
    try testing.expectEqualSlices(u8, data, dec_buf[0..n]);
}

fn expectRejected(text: []const u8, expected: DecodeError) !void {
    var buf: [512]u8 = undefined;
    try testing.expectError(expected, decode(&buf, text));
}

test "rfc 4648 vectors" {
    try expectVector("", "");
    try expectVector("f", "Zg==");
    try expectVector("fo", "Zm8=");
    try expectVector("foo", "Zm9v");
    try expectVector("foob", "Zm9vYg==");
    try expectVector("fooba", "Zm9vYmE=");
    try expectVector("foobar", "Zm9vYmFy");
}

test "symbol edge vectors" {
    try expectVector(&[_]u8{ 0x00, 0xFF, 0xFE }, "AP/+");
    try expectVector(&[_]u8{ 0xFB, 0xFF, 0xBF }, "+/+/");
    try expectVector(&[_]u8{ 0x00, 0x00, 0x00 }, "AAAA");
    try expectVector(&[_]u8{ 0xFF, 0xFF, 0xFF }, "////");
}

test "alphabet is complete and unique" {
    var count: usize = 0;
    for (decode_table) |v| {
        if (v != invalid) count += 1;
    }
    try testing.expectEqual(@as(usize, 64), count);
    for (alphabet, 0..) |c, i| {
        try testing.expectEqual(@as(u8, @intCast(i)), decode_table[c]);
    }
}

test "rejects bad lengths" {
    try expectRejected("Z", error.InvalidLength);
    try expectRejected("Zg", error.InvalidLength);
    try expectRejected("Zg=", error.InvalidLength);
    try expectRejected("Zm9vY", error.InvalidLength);
    try expectRejected("Zg===", error.InvalidLength);
    try expectRejected("Zg=====", error.InvalidLength);
    try expectRejected("Zm9vYg=", error.InvalidLength);
}

test "rejects characters outside the alphabet" {
    try expectRejected("Zm9-", error.InvalidCharacter);
    try expectRejected("Zm9_", error.InvalidCharacter);
    try expectRejected("Zm 9", error.InvalidCharacter);
    try expectRejected(" Zm9", error.InvalidCharacter);
    try expectRejected("Zm9\n", error.InvalidCharacter);
    try expectRejected("Zm9\t", error.InvalidCharacter);
    try expectRejected("Zm9\r", error.InvalidCharacter);
    try expectRejected("Zm9*", error.InvalidCharacter);
    try expectRejected("\xc3\xa9\xc3\xa9", error.InvalidCharacter);
}

test "rejects misplaced padding" {
    try expectRejected("Zg==Zg==", error.InvalidPadding);
    try expectRejected("Zm8=Zm8=", error.InvalidPadding);
    try expectRejected("Zm9vZg==Zg==", error.InvalidPadding);
    try expectRejected("====", error.InvalidPadding);
    try expectRejected("=Zg=", error.InvalidPadding);
    try expectRejected("Z===", error.InvalidPadding);
    try expectRejected("Zm=v", error.InvalidPadding);
    try expectRejected("Zg=a", error.InvalidPadding);
}

test "rejects non-canonical trailing bits" {
    try expectRejected("Zh==", error.NonCanonical);
    try expectRejected("Zm9=", error.NonCanonical);
    try expectRejected("Zm9vZh==", error.NonCanonical);
    try expectRejected("Zm9vYmF=", error.NonCanonical);
    try expectVector("f", "Zg==");
    try expectVector("fo", "Zm8=");
}

test "rejects a destination that is too small" {
    var buf: [2]u8 = undefined;
    try testing.expectError(error.NoSpaceLeft, decode(&buf, "Zm9v"));
}

test "round trips every length up to 300" {
    var data: [300]u8 = undefined;
    testutil.pseudoRandom(&data);
    var enc_buf: [400]u8 = undefined;
    var dec_buf: [300]u8 = undefined;
    for (0..data.len + 1) |len| {
        const encoded = encode(&enc_buf, data[0..len]);
        try testing.expectEqual(encodedLen(len), encoded.len);
        const n = try decode(&dec_buf, encoded);
        try testing.expectEqualSlices(u8, data[0..len], dec_buf[0..n]);
    }
}
