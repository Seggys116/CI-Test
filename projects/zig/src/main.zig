const std = @import("std");
const ciutil = @import("ciutil");

pub fn main(init: std.process.Init) !u8 {
    const arena = init.arena.allocator();
    const gpa = init.gpa;

    const raw_args = try init.minimal.args.toSlice(arena);
    const skip: usize = if (raw_args.len > 0) 1 else 0;
    const args = try arena.alloc([]const u8, raw_args.len - skip);
    for (raw_args[skip..], args) |raw, *arg| {
        arg.* = raw;
    }

    var stdout: std.ArrayList(u8) = .empty;
    defer stdout.deinit(gpa);
    var stderr: std.ArrayList(u8) = .empty;
    defer stderr.deinit(gpa);

    const code = try ciutil.cli.run(gpa, args, &stdout, &stderr);

    try std.Io.File.stdout().writeStreamingAll(init.io, stdout.items);
    try std.Io.File.stderr().writeStreamingAll(init.io, stderr.items);
    return code;
}
