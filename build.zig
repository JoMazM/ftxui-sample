const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const ftxui_dep = b.dependency("ftxui", .{
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "ftxui-sample",
        .target = target,
        .optimize = optimize,
    });
    exe.linkLibrary(ftxui_dep.artifact("dom"));
    exe.linkLibrary(ftxui_dep.artifact("screen"));
    exe.linkLibrary(ftxui_dep.artifact("component"));
    exe.linkLibCpp();
    exe.addIncludePath(b.path("src"));
    // exe.addWin32ResourceFile(.{ .file = .{}, .flags = &.{} });

    exe.addCSourceFiles(.{ .files = &.{
        "src/main.cpp",
    }, .flags = &.{
        "-std=c++17",
        "-fno-rtti",
        "-fno-exceptions",
    } });
    b.installArtifact(exe);
}
