//! Semantic Versioning 2.0.0 parsing and precedence. Parsed versions borrow
//! the prerelease and build text from the input string.

const std = @import("std");

pub const ParseError = error{
    /// the core is not exactly three dot-separated non-negative integers
    InvalidCore,
    /// a numeric field or numeric prerelease identifier has a leading zero
    LeadingZero,
    /// a numeric field does not fit in a u64
    Overflow,
    /// a prerelease or build identifier is empty
    EmptyIdentifier,
    /// a prerelease or build identifier has a character outside [0-9A-Za-z-]
    InvalidIdentifier,
};

/// dot-separated identifiers, as found in the prerelease and build text
pub const Identifiers = struct {
    rest: ?[]const u8,

    pub fn init(text: []const u8) Identifiers {
        return .{ .rest = if (text.len == 0) null else text };
    }

    pub fn next(self: *Identifiers) ?[]const u8 {
        const text = self.rest orelse return null;
        if (indexOf(text, 0, '.')) |dot| {
            self.rest = text[dot + 1 ..];
            return text[0..dot];
        }
        self.rest = null;
        return text;
    }
};

pub const Version = struct {
    major: u64,
    minor: u64,
    patch: u64,
    /// dot-separated, empty when absent
    prerelease: []const u8 = "",
    /// dot-separated, empty when absent
    build: []const u8 = "",

    pub fn prereleaseIdentifiers(self: Version) Identifiers {
        return Identifiers.init(self.prerelease);
    }

    pub fn buildIdentifiers(self: Version) Identifiers {
        return Identifiers.init(self.build);
    }

    /// writes the version so that parse accepts it again
    pub fn render(self: Version, buf: []u8) error{NoSpaceLeft}![]const u8 {
        const core = try std.fmt.bufPrint(buf, "{d}.{d}.{d}", .{ self.major, self.minor, self.patch });
        var len = core.len;
        if (self.prerelease.len > 0) {
            try append(buf, &len, "-");
            try append(buf, &len, self.prerelease);
        }
        if (self.build.len > 0) {
            try append(buf, &len, "+");
            try append(buf, &len, self.build);
        }
        return buf[0..len];
    }
};

fn append(buf: []u8, len: *usize, bytes: []const u8) error{NoSpaceLeft}!void {
    if (len.* + bytes.len > buf.len) return error.NoSpaceLeft;
    @memcpy(buf[len.*..][0..bytes.len], bytes);
    len.* += bytes.len;
}

fn indexOf(text: []const u8, start: usize, needle: u8) ?usize {
    for (text[start..], start..) |c, i| {
        if (c == needle) return i;
    }
    return null;
}

fn isNumeric(text: []const u8) bool {
    if (text.len == 0) return false;
    for (text) |c| {
        if (c < '0' or c > '9') return false;
    }
    return true;
}

fn hasLeadingZero(text: []const u8) bool {
    return text.len > 1 and text[0] == '0';
}

fn isIdentifierByte(c: u8) bool {
    return (c >= '0' and c <= '9') or (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '-';
}

fn parseNumber(field: []const u8) ParseError!u64 {
    if (!isNumeric(field)) return error.InvalidCore;
    if (hasLeadingZero(field)) return error.LeadingZero;
    var value: u64 = 0;
    for (field) |c| {
        const scaled = @mulWithOverflow(value, 10);
        if (scaled[1] != 0) return error.Overflow;
        const added = @addWithOverflow(scaled[0], @as(u64, c - '0'));
        if (added[1] != 0) return error.Overflow;
        value = added[0];
    }
    return value;
}

fn validateIdentifiers(text: []const u8, is_prerelease: bool) ParseError!void {
    var start: usize = 0;
    while (true) {
        const end = indexOf(text, start, '.') orelse text.len;
        const identifier = text[start..end];
        if (identifier.len == 0) return error.EmptyIdentifier;
        for (identifier) |c| {
            if (!isIdentifierByte(c)) return error.InvalidIdentifier;
        }
        if (is_prerelease and isNumeric(identifier) and hasLeadingZero(identifier)) {
            return error.LeadingZero;
        }
        if (end == text.len) return;
        start = end + 1;
    }
}

/// parses MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]
pub fn parse(input: []const u8) ParseError!Version {
    var rest = input;
    var build: []const u8 = "";
    if (indexOf(rest, 0, '+')) |plus| {
        build = rest[plus + 1 ..];
        try validateIdentifiers(build, false);
        rest = rest[0..plus];
    }
    var prerelease: []const u8 = "";
    if (indexOf(rest, 0, '-')) |dash| {
        prerelease = rest[dash + 1 ..];
        try validateIdentifiers(prerelease, true);
        rest = rest[0..dash];
    }

    var numbers = [3]u64{ 0, 0, 0 };
    var count: usize = 0;
    var start: usize = 0;
    while (true) {
        const end = indexOf(rest, start, '.') orelse rest.len;
        if (count == numbers.len) return error.InvalidCore;
        numbers[count] = try parseNumber(rest[start..end]);
        count += 1;
        if (end == rest.len) break;
        start = end + 1;
    }
    if (count != numbers.len) return error.InvalidCore;

    return .{
        .major = numbers[0],
        .minor = numbers[1],
        .patch = numbers[2],
        .prerelease = prerelease,
        .build = build,
    };
}

fn compareInts(a: anytype, b: @TypeOf(a)) i32 {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

fn compareBytes(a: []const u8, b: []const u8) i32 {
    const common = @min(a.len, b.len);
    for (a[0..common], b[0..common]) |x, y| {
        if (x != y) return if (x < y) -1 else 1;
    }
    return compareInts(a.len, b.len);
}

fn compareIdentifier(a: []const u8, b: []const u8) i32 {
    const a_numeric = isNumeric(a);
    const b_numeric = isNumeric(b);
    if (a_numeric and b_numeric) {
        // no leading zeros, so a longer digit string is a larger number
        const by_length = compareInts(a.len, b.len);
        if (by_length != 0) return by_length;
        return compareBytes(a, b);
    }
    if (a_numeric) return -1;
    if (b_numeric) return 1;
    return compareBytes(a, b);
}

fn comparePrerelease(a: []const u8, b: []const u8) i32 {
    if (a.len == 0 and b.len == 0) return 0;
    if (a.len == 0) return 1;
    if (b.len == 0) return -1;
    var a_ids = Identifiers.init(a);
    var b_ids = Identifiers.init(b);
    while (true) {
        const x = a_ids.next();
        const y = b_ids.next();
        if (x == null and y == null) return 0;
        if (x == null) return -1;
        if (y == null) return 1;
        const order = compareIdentifier(x.?, y.?);
        if (order != 0) return order;
    }
}

/// returns -1, 0 or 1 by semver precedence; build metadata is ignored
pub fn compare(a: Version, b: Version) i32 {
    const major = compareInts(a.major, b.major);
    if (major != 0) return major;
    const minor = compareInts(a.minor, b.minor);
    if (minor != 0) return minor;
    const patch = compareInts(a.patch, b.patch);
    if (patch != 0) return patch;
    return comparePrerelease(a.prerelease, b.prerelease);
}

const testing = std.testing;

const chain = [_][]const u8{
    "1.0.0-alpha",
    "1.0.0-alpha.1",
    "1.0.0-alpha.beta",
    "1.0.0-beta",
    "1.0.0-beta.2",
    "1.0.0-beta.11",
    "1.0.0-rc.1",
    "1.0.0",
    "2.0.0",
    "2.1.0",
    "2.1.1",
};

fn mustParse(text: []const u8) !Version {
    return parse(text) catch |err| {
        std.debug.print("{s} should parse but failed with {s}\n", .{ text, @errorName(err) });
        return err;
    };
}

fn expectCompare(a: []const u8, b: []const u8, expected: i32) !void {
    try testing.expectEqual(expected, compare(try mustParse(a), try mustParse(b)));
}

fn expectRejected(text: []const u8, expected: ParseError) !void {
    try testing.expectError(expected, parse(text));
}

fn expectRoundTrip(text: []const u8) !void {
    const version = try mustParse(text);
    var buf: [256]u8 = undefined;
    const rendered = try version.render(&buf);
    try testing.expectEqualStrings(text, rendered);
    const again = try mustParse(rendered);
    try testing.expectEqual(@as(i32, 0), compare(version, again));
    try testing.expectEqualStrings(version.prerelease, again.prerelease);
    try testing.expectEqualStrings(version.build, again.build);
}

test "parses all components" {
    const version = try mustParse("1.2.3-rc.1+build.5");
    try testing.expectEqual(@as(u64, 1), version.major);
    try testing.expectEqual(@as(u64, 2), version.minor);
    try testing.expectEqual(@as(u64, 3), version.patch);
    try testing.expectEqualStrings("rc.1", version.prerelease);
    try testing.expectEqualStrings("build.5", version.build);

    var pre = version.prereleaseIdentifiers();
    try testing.expectEqualStrings("rc", pre.next().?);
    try testing.expectEqualStrings("1", pre.next().?);
    try testing.expect(pre.next() == null);

    var build = version.buildIdentifiers();
    try testing.expectEqualStrings("build", build.next().?);
    try testing.expectEqualStrings("5", build.next().?);
    try testing.expect(build.next() == null);

    const plain = try mustParse("10.20.30");
    try testing.expectEqual(@as(usize, 0), plain.prerelease.len);
    try testing.expectEqual(@as(usize, 0), plain.build.len);
    var none = plain.prereleaseIdentifiers();
    try testing.expect(none.next() == null);
}

test "accepts valid versions and round trips them" {
    try expectRoundTrip("0.0.0");
    try expectRoundTrip("1.0.0-0");
    try expectRoundTrip("1.0.0-0a");
    try expectRoundTrip("1.0.0-x-y-z.-");
    try expectRoundTrip("1.0.0+001");
    try expectRoundTrip("1.0.0-rc.1+build.5");
    try expectRoundTrip("1.0.0-alpha.beta");
    try expectRoundTrip("1.0.0-alpha+001");
    try expectRoundTrip("1.0.0+20130313144700");
    try expectRoundTrip("1.0.0-0.3.7");
    try expectRoundTrip("1.0.0-a-b.c-d+e-f.g");
    try expectRoundTrip("1.0.0---");
    try expectRoundTrip("999.999.999");
    try expectRoundTrip("18446744073709551615.18446744073709551615.18446744073709551615");
}

test "rejects malformed versions" {
    try expectRejected("", error.InvalidCore);
    try expectRejected("1", error.InvalidCore);
    try expectRejected("1.0", error.InvalidCore);
    try expectRejected("1.0.0.0", error.InvalidCore);
    try expectRejected("v1.0.0", error.InvalidCore);
    try expectRejected("01.0.0", error.LeadingZero);
    try expectRejected("1.00.0", error.LeadingZero);
    try expectRejected("1.0.00", error.LeadingZero);
    try expectRejected("1.0.0-", error.EmptyIdentifier);
    try expectRejected("1.0.0+", error.EmptyIdentifier);
    try expectRejected("1.0.0-01", error.LeadingZero);
    try expectRejected("1.0.0-alpha.01", error.LeadingZero);
    try expectRejected("1.0.0-a..b", error.EmptyIdentifier);
    try expectRejected("1.0.0-alpha.", error.EmptyIdentifier);
    try expectRejected("1.0.0-.alpha", error.EmptyIdentifier);
    try expectRejected("1.0.0+a..b", error.EmptyIdentifier);
    try expectRejected("1.0.0-a_b", error.InvalidIdentifier);
    try expectRejected("1.0.0+a_b", error.InvalidIdentifier);
    try expectRejected("1.0.0+a+b", error.InvalidIdentifier);
    try expectRejected("1.0.0-\xc3\xa9", error.InvalidIdentifier);
    try expectRejected("-1.0.0", error.InvalidCore);
    try expectRejected("+1.0.0", error.InvalidCore);
    try expectRejected(" 1.0.0", error.InvalidCore);
    try expectRejected("1.0.0 ", error.InvalidCore);
    try expectRejected("1.0.0\n", error.InvalidCore);
    try expectRejected("1.0.0-alpha+", error.EmptyIdentifier);
    try expectRejected("1..0", error.InvalidCore);
    try expectRejected("..", error.InvalidCore);
    try expectRejected("1.a.0", error.InvalidCore);
    try expectRejected("1.0.-1", error.InvalidCore);
    try expectRejected("1.0.+1", error.InvalidCore);
}

test "numeric fields must fit in a u64" {
    const max = try mustParse("18446744073709551615.0.0");
    try testing.expectEqual(std.math.maxInt(u64), max.major);
    try testing.expectEqual(std.math.maxInt(u64), (try mustParse("0.18446744073709551615.0")).minor);
    try testing.expectEqual(std.math.maxInt(u64), (try mustParse("0.0.18446744073709551615")).patch);
    try expectRejected("18446744073709551616.0.0", error.Overflow);
    try expectRejected("0.18446744073709551616.0", error.Overflow);
    try expectRejected("0.0.99999999999999999999", error.Overflow);
}

test "ordering chain holds for all ordered pairs" {
    for (chain, 0..) |a, i| {
        for (chain, 0..) |b, j| {
            const expected: i32 = if (i < j) -1 else if (i > j) 1 else 0;
            try expectCompare(a, b, expected);
        }
    }
}

test "identifier rules" {
    try expectCompare("1.0.0-beta.2", "1.0.0-beta.11", -1);
    try expectCompare("1.0.0-1", "1.0.0-a", -1);
    try expectCompare("1.0.0-a", "1.0.0-1", 1);
    try expectCompare("1.0.0-B", "1.0.0-a", -1);
    try expectCompare("1.0.0-a-b", "1.0.0-a", 1);
    try expectCompare("1.0.0-alpha.1", "1.0.0-alpha", 1);
    try expectCompare("1.0.0-0a", "1.0.0-0", 1);
    try expectCompare("1.0.0-9", "1.0.0-10", -1);
    try expectCompare("1.0.0-99999999999999999999", "1.0.0-100000000000000000000", -1);
    try expectCompare("0.9.9", "1.0.0", -1);
    try expectCompare("1.10.0", "1.9.0", 1);
    try expectCompare("1.0.10", "1.0.9", 1);
    try expectCompare("10.0.0", "9.0.0", 1);
}

test "build metadata is ignored" {
    try expectCompare("1.0.0+a", "1.0.0+b", 0);
    try expectCompare("1.0.0-rc.1+a", "1.0.0-rc.1", 0);
    try expectCompare("1.0.0+z", "1.0.1+a", -1);
}

test "compare is antisymmetric" {
    const samples = [_][]const u8{ "0.0.0", "1.0.0+x", "1.2.3-rc.1", "1.0.0-1" } ++ chain;
    for (samples) |a| {
        for (samples) |b| {
            const forward = compare(try mustParse(a), try mustParse(b));
            const backward = compare(try mustParse(b), try mustParse(a));
            try testing.expectEqual(-backward, forward);
        }
    }
}

test "render reports a buffer that is too small" {
    const version = try mustParse("1.2.3-rc.1+build.5");
    var small: [8]u8 = undefined;
    try testing.expectError(error.NoSpaceLeft, version.render(&small));
    var exact: [18]u8 = undefined;
    try testing.expectEqualStrings("1.2.3-rc.1+build.5", try version.render(&exact));
}
