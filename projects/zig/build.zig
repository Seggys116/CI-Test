const std = @import("std");

const SmokeCase = struct {
    args: []const []const u8,
    stdout: []const u8 = "",
    stderr_prefix: []const u8 = "",
    code: u8 = 0,
};

// runs the installed cli the way a user would, covering the process and io path
const smoke_cases = [_]SmokeCase{
    .{ .args = &.{"version"}, .stdout = "ciutil zig 1.0.0\n" },
    .{ .args = &.{ "crc32", "123456789" }, .stdout = "cbf43926\n" },
    .{ .args = &.{ "crc32", "" }, .stdout = "00000000\n" },
    .{ .args = &.{ "adler32", "Wikipedia" }, .stdout = "11e60398\n" },
    .{ .args = &.{ "b64enc", "foobar" }, .stdout = "Zm9vYmFy\n" },
    .{ .args = &.{ "b64dec", "Zm9vYmE=" }, .stdout = "fooba" },
    .{ .args = &.{ "b64dec", "" }, .stdout = "" },
    .{ .args = &.{ "b64dec", "AP/+" }, .stdout = "\x00\xff\xfe" },
    .{ .args = &.{ "roman", "1994" }, .stdout = "MCMXCIV\n" },
    .{ .args = &.{ "roman", "3999" }, .stdout = "MMMCMXCIX\n" },
    .{ .args = &.{ "unroman", "MMXXIV" }, .stdout = "2024\n" },
    .{ .args = &.{ "semver-cmp", "1.0.0-alpha", "1.0.0" }, .stdout = "-1\n" },
    .{ .args = &.{ "semver-cmp", "1.0.0+a", "1.0.0+b" }, .stdout = "0\n" },
    .{ .args = &.{ "semver-cmp", "2.1.1", "2.1.0" }, .stdout = "1\n" },
    .{ .args = &.{ "b64dec", "Zh==" }, .code = 2, .stderr_prefix = "ciutil: " },
    .{ .args = &.{ "roman", "0" }, .code = 2, .stderr_prefix = "ciutil: " },
    .{ .args = &.{ "unroman", "IIII" }, .code = 2, .stderr_prefix = "ciutil: " },
    .{ .args = &.{ "semver-cmp", "1.0", "1.0.0" }, .code = 2, .stderr_prefix = "ciutil: " },
    .{ .args = &.{"frobnicate"}, .code = 64, .stderr_prefix = "usage: ciutil" },
    .{ .args = &.{"crc32"}, .code = 64, .stderr_prefix = "usage: ciutil" },
    .{ .args = &.{ "semver-cmp", "1.0.0" }, .code = 64, .stderr_prefix = "usage: ciutil" },
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const mod = b.addModule("ciutil", .{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "ciutil",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "ciutil", .module = mod },
            },
        }),
    });
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the cli");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const mod_tests = b.addTest(.{
        .root_module = mod,
    });
    const run_mod_tests = b.addRunArtifact(mod_tests);

    const test_step = b.step("test", "Run unit tests and the cli smoke checks");
    test_step.dependOn(&run_mod_tests.step);

    for (smoke_cases) |case| {
        const smoke = b.addRunArtifact(exe);
        smoke.addArgs(case.args);
        smoke.expectExitCode(case.code);
        if (case.code == 0) {
            smoke.expectStdOutEqual(case.stdout);
        } else {
            smoke.expectStdErrMatch(case.stderr_prefix);
        }
        test_step.dependOn(&smoke.step);
    }
}
