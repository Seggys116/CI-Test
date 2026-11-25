//! Adler-32 as specified by RFC 1950.

const std = @import("std");

const modulus: u32 = 65521;
// largest run of bytes after which the sums are guaranteed not to overflow u32
const nmax: usize = 5552;

pub const Adler32 = struct {
    a: u32 = 1,
    b: u32 = 0,

    pub fn init() Adler32 {
        return .{};
    }

    pub fn update(self: *Adler32, data: []const u8) void {
        var a = self.a;
        var b = self.b;
        var rest = data;
        while (rest.len > 0) {
            const n = @min(rest.len, nmax);
            for (rest[0..n]) |byte| {
                a += byte;
                b += a;
            }
            a %= modulus;
            b %= modulus;
            rest = rest[n..];
        }
        self.a = a;
        self.b = b;
    }

    pub fn final(self: Adler32) u32 {
        return (self.b << 16) | self.a;
    }
};

pub fn checksum(data: []const u8) u32 {
    var adler = Adler32.init();
    adler.update(data);
    return adler.final();
}

const testing = std.testing;
const testutil = @import("testutil.zig");

fn naiveReference(data: []const u8) u32 {
    var a: u32 = 1;
    var b: u32 = 0;
    for (data) |byte| {
        a = (a + byte) % modulus;
        b = (b + a) % modulus;
    }
    return (b << 16) | a;
}

fn expectChecksum(data: []const u8, expected: u32) !void {
    try testing.expectEqual(expected, checksum(data));
}

test "known vectors" {
    try expectChecksum("", 0x00000001);
    try expectChecksum("a", 0x00620062);
    try expectChecksum("abc", 0x024d0127);
    try expectChecksum("Wikipedia", 0x11e60398);
    try expectChecksum("123456789", 0x091e01de);
}

test "long run of 0xFF matches the naive reference" {
    const data = [_]u8{0xFF} ** 100_000;
    try testing.expectEqual(naiveReference(&data), checksum(&data));
}

test "lengths around nmax match the naive reference" {
    var random: [3 * nmax + 5]u8 = undefined;
    testutil.pseudoRandom(&random);
    const ones = [_]u8{0xFF} ** (2 * nmax + 1);
    const lengths = [_]usize{
        nmax - 1,     nmax,         nmax + 1,
        2 * nmax - 1, 2 * nmax,     2 * nmax + 1,
        3 * nmax,     3 * nmax + 5,
    };
    for (lengths) |len| {
        try testing.expectEqual(naiveReference(random[0..len]), checksum(random[0..len]));
        if (len <= ones.len) {
            try testing.expectEqual(naiveReference(ones[0..len]), checksum(ones[0..len]));
        }
    }
}

test "chunked equals one-shot at every split" {
    var data: [1000]u8 = undefined;
    testutil.pseudoRandom(&data);
    const whole = checksum(&data);
    try testing.expectEqual(naiveReference(&data), whole);
    for (0..data.len + 1) |split| {
        var adler = Adler32.init();
        adler.update(data[0..split]);
        adler.update(data[split..]);
        try testing.expectEqual(whole, adler.final());
    }
}

test "chunked long input equals one-shot" {
    const data = [_]u8{0xFF} ** 100_000;
    const whole = checksum(&data);
    for ([_]usize{ 1, 7, 4096, nmax - 1, nmax, nmax + 1, 99_999 }) |size| {
        var adler = Adler32.init();
        var start: usize = 0;
        while (start < data.len) : (start += size) {
            adler.update(data[start..@min(start + size, data.len)]);
        }
        try testing.expectEqual(whole, adler.final());
    }
}

test "final does not consume the state" {
    var adler = Adler32.init();
    adler.update("Wiki");
    try testing.expectEqual(checksum("Wiki"), adler.final());
    adler.update("pedia");
    try testing.expectEqual(@as(u32, 0x11e60398), adler.final());
}
