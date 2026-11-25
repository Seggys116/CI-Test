//! CRC-32/IEEE: reflected, polynomial 0xEDB88320, init and xorout 0xFFFFFFFF.

const std = @import("std");

const poly: u32 = 0xEDB88320;

const table: [256]u32 = blk: {
    @setEvalBranchQuota(10_000);
    var t: [256]u32 = undefined;
    for (&t, 0..) |*entry, i| {
        var c: u32 = @intCast(i);
        for (0..8) |_| {
            c = if (c & 1 != 0) (c >> 1) ^ poly else c >> 1;
        }
        entry.* = c;
    }
    break :blk t;
};

pub const Crc32 = struct {
    state: u32 = 0xFFFFFFFF,

    pub fn init() Crc32 {
        return .{};
    }

    pub fn update(self: *Crc32, data: []const u8) void {
        var crc = self.state;
        for (data) |byte| {
            const index: u8 = @as(u8, @truncate(crc)) ^ byte;
            crc = table[index] ^ (crc >> 8);
        }
        self.state = crc;
    }

    pub fn final(self: Crc32) u32 {
        return self.state ^ 0xFFFFFFFF;
    }
};

pub fn checksum(data: []const u8) u32 {
    var crc = Crc32.init();
    crc.update(data);
    return crc.final();
}

const testing = std.testing;
const testutil = @import("testutil.zig");

fn bitwiseReference(data: []const u8) u32 {
    var crc: u32 = 0xFFFFFFFF;
    for (data) |byte| {
        crc ^= byte;
        for (0..8) |_| {
            crc = if (crc & 1 != 0) (crc >> 1) ^ poly else crc >> 1;
        }
    }
    return ~crc;
}

fn expectChecksum(data: []const u8, expected: u32) !void {
    try testing.expectEqual(expected, checksum(data));
}

test "known vectors" {
    try expectChecksum("", 0x00000000);
    try expectChecksum("a", 0xe8b7be43);
    try expectChecksum("abc", 0x352441c2);
    try expectChecksum("123456789", 0xcbf43926);
    try expectChecksum("The quick brown fox jumps over the lazy dog", 0x414fa339);
    try expectChecksum(&([_]u8{0x00} ** 32), 0x190a55ad);
    try expectChecksum(&([_]u8{0xFF} ** 32), 0xff6cab0b);
}

test "table anchors" {
    try testing.expectEqual(@as(u32, 0), table[0]);
    try testing.expectEqual(@as(u32, 0x77073096), table[1]);
    try testing.expectEqual(@as(u32, 0x2D02EF8D), table[255]);
}

test "matches the bitwise reference" {
    var data: [1000]u8 = undefined;
    testutil.pseudoRandom(&data);
    for ([_]usize{ 0, 1, 2, 3, 7, 8, 9, 255, 256, 1000 }) |len| {
        try testing.expectEqual(bitwiseReference(data[0..len]), checksum(data[0..len]));
    }
}

test "chunked equals one-shot at every split" {
    var data: [1000]u8 = undefined;
    testutil.pseudoRandom(&data);
    const whole = checksum(&data);
    for (0..data.len + 1) |split| {
        var crc = Crc32.init();
        crc.update(data[0..split]);
        crc.update(data[split..]);
        try testing.expectEqual(whole, crc.final());
    }
}

test "many small chunks equal one-shot" {
    var data: [1000]u8 = undefined;
    testutil.pseudoRandom(&data);
    const whole = checksum(&data);
    for ([_]usize{ 1, 2, 3, 7, 64, 999 }) |size| {
        var crc = Crc32.init();
        var start: usize = 0;
        while (start < data.len) : (start += size) {
            crc.update(data[start..@min(start + size, data.len)]);
        }
        try testing.expectEqual(whole, crc.final());
    }
}

test "final does not consume the state" {
    var crc = Crc32.init();
    crc.update("12345");
    try testing.expectEqual(checksum("12345"), crc.final());
    crc.update("6789");
    try testing.expectEqual(@as(u32, 0xcbf43926), crc.final());
}
