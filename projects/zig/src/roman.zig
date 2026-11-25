//! Roman numerals in canonical subtractive form, for 1 through 3999.

const std = @import("std");

pub const RomanError = error{
    /// the number is outside 1 through 3999
    OutOfRange,
    /// the text is not a canonical roman numeral
    Invalid,
};

/// length of the longest numeral, 3888 = MMMDCCCLXXXVIII
pub const max_len = 15;

const max_value = 3999;

const Numeral = struct {
    value: u16,
    symbol: []const u8,
};

const numerals = [_]Numeral{
    .{ .value = 1000, .symbol = "M" },
    .{ .value = 900, .symbol = "CM" },
    .{ .value = 500, .symbol = "D" },
    .{ .value = 400, .symbol = "CD" },
    .{ .value = 100, .symbol = "C" },
    .{ .value = 90, .symbol = "XC" },
    .{ .value = 50, .symbol = "L" },
    .{ .value = 40, .symbol = "XL" },
    .{ .value = 10, .symbol = "X" },
    .{ .value = 9, .symbol = "IX" },
    .{ .value = 5, .symbol = "V" },
    .{ .value = 4, .symbol = "IV" },
    .{ .value = 1, .symbol = "I" },
};

/// writes the canonical numeral for n into buf and returns the used prefix
pub fn toRoman(buf: *[max_len]u8, n: i64) RomanError![]const u8 {
    if (n < 1 or n > max_value) return error.OutOfRange;
    var remaining: u16 = @intCast(n);
    var len: usize = 0;
    for (numerals) |numeral| {
        while (remaining >= numeral.value) {
            @memcpy(buf[len..][0..numeral.symbol.len], numeral.symbol);
            len += numeral.symbol.len;
            remaining -= numeral.value;
        }
    }
    return buf[0..len];
}

/// parses a numeral; anything toRoman would not produce is rejected
pub fn fromRoman(text: []const u8) RomanError!u16 {
    var rest = text;
    var total: u16 = 0;
    for (numerals) |numeral| {
        while (std.mem.startsWith(u8, rest, numeral.symbol)) {
            total += numeral.value;
            if (total > max_value) return error.Invalid;
            rest = rest[numeral.symbol.len..];
        }
    }
    if (rest.len != 0 or total == 0) return error.Invalid;
    var buf: [max_len]u8 = undefined;
    const canonical = toRoman(&buf, total) catch return error.Invalid;
    if (!std.mem.eql(u8, canonical, text)) return error.Invalid;
    return total;
}

const testing = std.testing;

fn expectPair(n: i64, numeral: []const u8) !void {
    var buf: [max_len]u8 = undefined;
    try testing.expectEqualStrings(numeral, try toRoman(&buf, n));
    try testing.expectEqual(n, @as(i64, try fromRoman(numeral)));
}

fn expectRejected(text: []const u8) !void {
    try testing.expectError(error.Invalid, fromRoman(text));
}

test "known vectors" {
    try expectPair(1, "I");
    try expectPair(4, "IV");
    try expectPair(9, "IX");
    try expectPair(14, "XIV");
    try expectPair(40, "XL");
    try expectPair(90, "XC");
    try expectPair(400, "CD");
    try expectPair(1994, "MCMXCIV");
    try expectPair(1999, "MCMXCIX");
    try expectPair(2024, "MMXXIV");
    try expectPair(3999, "MMMCMXCIX");
}

test "toRoman rejects out of range numbers" {
    var buf: [max_len]u8 = undefined;
    for ([_]i64{ 0, 4000, 4001, -1, -3999, std.math.minInt(i64), std.math.maxInt(i64) }) |n| {
        try testing.expectError(error.OutOfRange, toRoman(&buf, n));
    }
}

test "fromRoman rejects non-canonical forms" {
    try expectRejected("");
    try expectRejected("IIII");
    try expectRejected("VX");
    try expectRejected("IC");
    try expectRejected("XM");
    try expectRejected("VV");
    try expectRejected("IIX");
    try expectRejected("xiv");
    try expectRejected("Xiv");
    try expectRejected("IVI");
    try expectRejected("VIV");
    try expectRejected("IXI");
    try expectRejected("IIV");
    try expectRejected("XXXX");
    try expectRejected("CCCC");
    try expectRejected("MMMM");
    try expectRejected("LL");
    try expectRejected("DD");
    try expectRejected("XVX");
    try expectRejected("MCMC");
    try expectRejected("CMD");
    try expectRejected("CDC");
    try expectRejected("IL");
    try expectRejected("XD");
    try expectRejected("MMMMCMXCIX");
    try expectRejected(" X");
    try expectRejected("X ");
    try expectRejected("X\n");
    try expectRejected("I I");
    try expectRejected("N");
    try expectRejected("0");
}

test "every number from 1 to 3999 round trips" {
    var buf: [max_len]u8 = undefined;
    for (1..max_value + 1) |n| {
        const value: i64 = @intCast(n);
        const numeral = try toRoman(&buf, value);
        try testing.expect(numeral.len <= max_len);
        for (numeral) |c| {
            try testing.expect(c == 'I' or c == 'V' or c == 'X' or c == 'L' or c == 'C' or c == 'D' or c == 'M');
        }
        try testing.expectEqual(@as(u16, @intCast(n)), try fromRoman(numeral));
    }
}
