const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const ftxui_dep = b.dependency("ftxui", .{
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "file-explorer",
        .target = target,
        .optimize = optimize,
    });
    exe.linkLibrary(ftxui_dep.artifact("dom"));
    exe.linkLibrary(ftxui_dep.artifact("screen"));
    exe.linkLibrary(ftxui_dep.artifact("component"));
    exe.linkLibCpp();
    exe.addIncludePath(.{ .path = "src" });
    // exe.addIncludePath(.{ .path = "res" });
    exe.addIncludePath(.{ .path = "3rdparty" });

    // if (target.Os.Tag == .windows)
    if (exe.isWindows())
        exe.defineCMacro("WIN32", null);

    exe.addCSourceFiles(.{
        .files = &.{
            "src/main.cpp",
            "src/main-ui.cpp",
            // "3rdparty/tinyxml2.cpp",
        },
        .flags = &.{
            "-std=c++23",
            "-fno-rtti",
            // "-fno-exceptions",
            "-fexceptions",
            "-fno-lto", // workaround for compiling on widnows
        },
    });
    b.installArtifact(exe);
}
