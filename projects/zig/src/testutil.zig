//! helpers shared by the test blocks of the other files

/// fills buf from the fixed lcg shared by every language of this repo
pub fn pseudoRandom(buf: []u8) void {
    var x: u64 = 42;
    for (buf) |*byte| {
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF;
        byte.* = @truncate(x >> 16);
    }
}

test "pseudoRandom starts with the shared first byte" {
    var buf: [1]u8 = undefined;
    pseudoRandom(&buf);
    if (buf[0] != 137) return error.TestUnexpectedResult;
}
