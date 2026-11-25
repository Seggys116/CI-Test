//! Command line front end. It writes into caller-owned buffers and touches no
//! process state, so it can be tested without spawning anything.

const std = @import("std");
const Allocator = std.mem.Allocator;

const adler32 = @import("adler32.zig");
const base64 = @import("base64.zig");
const crc32 = @import("crc32.zig");
const roman = @import("roman.zig");
const semver = @import("semver.zig");

/// exit code for invalid input
pub const exit_error: u8 = 2;
/// exit code for an unknown command or the wrong number of arguments
pub const exit_usage: u8 = 64;

const usage_text =
    \\usage: ciutil <command> [args]
    \\
    \\commands:
    \\  crc32 STRING        print the CRC-32 of STRING as 8 hex digits
    \\  adler32 STRING      print the Adler-32 of STRING as 8 hex digits
    \\  b64enc STRING       print STRING encoded as base64
    \\  b64dec BASE64       write the decoded bytes to stdout
    \\  roman NUMBER        print NUMBER (1-3999) as a roman numeral
    \\  unroman ROMAN       print a roman numeral as a decimal number
    \\  semver-cmp A B      print -1, 0 or 1 comparing semantic versions A and B
    \\  version             print the program version
    \\
;

fn eql(a: []const u8, b: []const u8) bool {
    return std.mem.eql(u8, a, b);
}

fn usage(gpa: Allocator, stderr: *std.ArrayList(u8)) Allocator.Error!u8 {
    try stderr.appendSlice(gpa, usage_text);
    return exit_usage;
}

fn fail(gpa: Allocator, stderr: *std.ArrayList(u8), context: []const u8, reason: []const u8) Allocator.Error!u8 {
    try stderr.appendSlice(gpa, "ciutil: ");
    try stderr.appendSlice(gpa, context);
    try stderr.appendSlice(gpa, ": ");
    try stderr.appendSlice(gpa, reason);
    try stderr.append(gpa, '\n');
    return exit_error;
}

fn appendHex32(gpa: Allocator, out: *std.ArrayList(u8), value: u32) Allocator.Error!void {
    const digits = "0123456789abcdef";
    var text: [9]u8 = undefined;
    for (0..8) |i| {
        const shift: u5 = @intCast(28 - 4 * i);
        text[i] = digits[(value >> shift) & 0xF];
    }
    text[8] = '\n';
    try out.appendSlice(gpa, &text);
}

// plain decimal with an optional sign; anything else, including overflow, is null
fn parseI64(text: []const u8) ?i64 {
    var start: usize = 0;
    var negative = false;
    if (text.len > 0 and (text[0] == '+' or text[0] == '-')) {
        negative = text[0] == '-';
        start = 1;
    }
    if (start == text.len) return null;
    var magnitude: u64 = 0;
    for (text[start..]) |c| {
        if (c < '0' or c > '9') return null;
        const scaled = @mulWithOverflow(magnitude, 10);
        if (scaled[1] != 0) return null;
        const added = @addWithOverflow(scaled[0], @as(u64, c - '0'));
        if (added[1] != 0) return null;
        magnitude = added[0];
    }
    if (magnitude > std.math.maxInt(i64)) return null;
    const value: i64 = @intCast(magnitude);
    return if (negative) -value else value;
}

/// runs the cli. args excludes the program name. returns the exit code: 0 on
/// success, exit_error for invalid input and exit_usage for misuse.
/// stdout and stderr are appended to using gpa.
pub fn run(
    gpa: Allocator,
    args: []const []const u8,
    stdout: *std.ArrayList(u8),
    stderr: *std.ArrayList(u8),
) Allocator.Error!u8 {
    if (args.len == 0) return usage(gpa, stderr);
    const command = args[0];
    const operands = args[1..];

    if (eql(command, "version") and operands.len == 0) {
        try stdout.appendSlice(gpa, "ciutil zig 1.0.0\n");
        return 0;
    }
    if (eql(command, "crc32") and operands.len == 1) {
        try appendHex32(gpa, stdout, crc32.checksum(operands[0]));
        return 0;
    }
    if (eql(command, "adler32") and operands.len == 1) {
        try appendHex32(gpa, stdout, adler32.checksum(operands[0]));
        return 0;
    }
    if (eql(command, "b64enc") and operands.len == 1) {
        const buf = try gpa.alloc(u8, base64.encodedLen(operands[0].len));
        defer gpa.free(buf);
        try stdout.appendSlice(gpa, base64.encode(buf, operands[0]));
        try stdout.append(gpa, '\n');
        return 0;
    }
    if (eql(command, "b64dec") and operands.len == 1) {
        const buf = try gpa.alloc(u8, base64.decodedLenMax(operands[0].len));
        defer gpa.free(buf);
        const written = base64.decode(buf, operands[0]) catch |err| {
            return fail(gpa, stderr, "base64", @errorName(err));
        };
        try stdout.appendSlice(gpa, buf[0..written]);
        return 0;
    }
    if (eql(command, "roman") and operands.len == 1) {
        const number = parseI64(operands[0]) orelse {
            return fail(gpa, stderr, "number", "not a decimal integer");
        };
        var buf: [roman.max_len]u8 = undefined;
        const numeral = roman.toRoman(&buf, number) catch |err| {
            return fail(gpa, stderr, "roman", @errorName(err));
        };
        try stdout.appendSlice(gpa, numeral);
        try stdout.append(gpa, '\n');
        return 0;
    }
    if (eql(command, "unroman") and operands.len == 1) {
        const number = roman.fromRoman(operands[0]) catch |err| {
            return fail(gpa, stderr, "unroman", @errorName(err));
        };
        var buf: [8]u8 = undefined;
        const text = std.fmt.bufPrint(&buf, "{d}\n", .{number}) catch unreachable;
        try stdout.appendSlice(gpa, text);
        return 0;
    }
    if (eql(command, "semver-cmp") and operands.len == 2) {
        const a = semver.parse(operands[0]) catch |err| {
            return fail(gpa, stderr, "version", @errorName(err));
        };
        const b = semver.parse(operands[1]) catch |err| {
            return fail(gpa, stderr, "version", @errorName(err));
        };
        const text = switch (semver.compare(a, b)) {
            -1 => "-1\n",
            0 => "0\n",
            else => "1\n",
        };
        try stdout.appendSlice(gpa, text);
        return 0;
    }
    return usage(gpa, stderr);
}

const testing = std.testing;

const Outcome = struct {
    code: u8,
    stdout: std.ArrayList(u8),
    stderr: std.ArrayList(u8),

    fn deinit(self: *Outcome) void {
        self.stdout.deinit(testing.allocator);
        self.stderr.deinit(testing.allocator);
    }
};

fn exec(args: []const []const u8) !Outcome {
    var stdout: std.ArrayList(u8) = .empty;
    errdefer stdout.deinit(testing.allocator);
    var stderr: std.ArrayList(u8) = .empty;
    errdefer stderr.deinit(testing.allocator);
    const code = try run(testing.allocator, args, &stdout, &stderr);
    return .{ .code = code, .stdout = stdout, .stderr = stderr };
}

fn expectOk(args: []const []const u8, expected: []const u8) !void {
    var outcome = try exec(args);
    defer outcome.deinit();
    try testing.expectEqual(@as(u8, 0), outcome.code);
    try testing.expectEqualSlices(u8, expected, outcome.stdout.items);
    try testing.expectEqual(@as(usize, 0), outcome.stderr.items.len);
}

fn expectExit(args: []const []const u8, code: u8, stderr_prefix: []const u8) !void {
    var outcome = try exec(args);
    defer outcome.deinit();
    try testing.expectEqual(code, outcome.code);
    try testing.expectEqual(@as(usize, 0), outcome.stdout.items.len);
    try testing.expect(std.mem.startsWith(u8, outcome.stderr.items, stderr_prefix));
}

test "checksums" {
    try expectOk(&.{ "crc32", "123456789" }, "cbf43926\n");
    try expectOk(&.{ "crc32", "" }, "00000000\n");
    try expectOk(&.{ "crc32", "a" }, "e8b7be43\n");
    try expectOk(&.{ "crc32", "The quick brown fox jumps over the lazy dog" }, "414fa339\n");
    try expectOk(&.{ "adler32", "Wikipedia" }, "11e60398\n");
    try expectOk(&.{ "adler32", "" }, "00000001\n");
    try expectOk(&.{ "adler32", "123456789" }, "091e01de\n");
}

test "base64 commands" {
    try expectOk(&.{ "b64enc", "foobar" }, "Zm9vYmFy\n");
    try expectOk(&.{ "b64enc", "f" }, "Zg==\n");
    try expectOk(&.{ "b64enc", "" }, "\n");
    try expectOk(&.{ "b64dec", "Zm9vYg==" }, "foob");
    try expectOk(&.{ "b64dec", "" }, "");
    try expectOk(&.{ "b64dec", "AP/+" }, "\x00\xff\xfe");
}

test "utf-8 input is treated as raw bytes" {
    try expectOk(&.{ "b64enc", "h\xc3\xa9llo" }, "aMOpbGxv\n");
    try expectOk(&.{ "b64dec", "aMOpbGxv" }, "h\xc3\xa9llo");
}

test "roman commands" {
    try expectOk(&.{ "roman", "1" }, "I\n");
    try expectOk(&.{ "roman", "1994" }, "MCMXCIV\n");
    try expectOk(&.{ "roman", "3999" }, "MMMCMXCIX\n");
    try expectOk(&.{ "roman", "+4" }, "IV\n");
    try expectOk(&.{ "unroman", "MMXXIV" }, "2024\n");
    try expectOk(&.{ "unroman", "IV" }, "4\n");
}

test "semver-cmp" {
    try expectOk(&.{ "semver-cmp", "1.0.0-alpha", "1.0.0" }, "-1\n");
    try expectOk(&.{ "semver-cmp", "1.0.0", "1.0.0-alpha" }, "1\n");
    try expectOk(&.{ "semver-cmp", "1.0.0+a", "1.0.0+b" }, "0\n");
    try expectOk(&.{ "semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11" }, "-1\n");
}

test "version" {
    try expectOk(&.{"version"}, "ciutil zig 1.0.0\n");
}

test "invalid input exits with the error code" {
    const cases = [_][]const []const u8{
        &.{ "b64dec", "Zg" },
        &.{ "b64dec", "Zh==" },
        &.{ "b64dec", "Zm9-" },
        &.{ "roman", "0" },
        &.{ "roman", "4000" },
        &.{ "roman", "-1" },
        &.{ "roman", "abc" },
        &.{ "roman", "" },
        &.{ "roman", "-" },
        &.{ "roman", "1_0" },
        &.{ "roman", "99999999999999999999999" },
        &.{ "unroman", "" },
        &.{ "unroman", "IIII" },
        &.{ "unroman", "xiv" },
        &.{ "semver-cmp", "1.0", "1.0.0" },
        &.{ "semver-cmp", "1.0.0", "v1.0.0" },
    };
    for (cases) |args| {
        try expectExit(args, exit_error, "ciutil: ");
    }
}

test "misuse exits with the usage code" {
    const cases = [_][]const []const u8{
        &.{},
        &.{"frobnicate"},
        &.{ "frobnicate", "x" },
        &.{"crc32"},
        &.{ "crc32", "a", "b" },
        &.{"adler32"},
        &.{"b64enc"},
        &.{ "b64dec", "a", "b" },
        &.{"roman"},
        &.{ "unroman", "I", "I" },
        &.{"semver-cmp"},
        &.{ "semver-cmp", "1.0.0" },
        &.{ "semver-cmp", "1.0.0", "1.0.0", "1.0.0" },
        &.{ "version", "extra" },
        &.{ "CRC32", "a" },
    };
    for (cases) |args| {
        try expectExit(args, exit_usage, "usage: ciutil");
    }
}

test "parseI64" {
    try testing.expectEqual(@as(?i64, 0), parseI64("0"));
    try testing.expectEqual(@as(?i64, 42), parseI64("+42"));
    try testing.expectEqual(@as(?i64, -42), parseI64("-42"));
    try testing.expectEqual(@as(?i64, std.math.maxInt(i64)), parseI64("9223372036854775807"));
    try testing.expectEqual(@as(?i64, null), parseI64("9223372036854775808"));
    try testing.expectEqual(@as(?i64, null), parseI64(""));
    try testing.expectEqual(@as(?i64, null), parseI64("+"));
    try testing.expectEqual(@as(?i64, null), parseI64("1 "));
}
