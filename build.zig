const std = @import("std");
// const zcc = @import("compile_commands.zig");

pub fn build(b: *std.Build) void {
    // make a list of targets that have include files and c source files
    var targets = std.ArrayList(*std.Build.Step.Compile).init(b.allocator);
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root_module_opts: std.Build.Module.CreateOptions = .{
        .root_source_file = null,
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    };

    const ftxui_dep = b.dependency("ftxui", .{
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "ftxui_sample",
        .root_module = b.createModule(root_module_opts),
    });

    // keep track of it, so later we can pass it to compile_commands
    // Any linked library will also be pulled in. TODO check this
    targets.append(exe) catch @panic("OOM");
    // add a step called "cdb" (Compile commands DataBase) for making
    // compile_commands.json. could be named anything. cdb is just quick to type
    // zcc.createStep(b, "cdb", targets.toOwnedSlice() catch @panic("OOM"));

    exe.linkLibrary(ftxui_dep.artifact("dom"));
    exe.linkLibrary(ftxui_dep.artifact("screen"));
    exe.linkLibrary(ftxui_dep.artifact("component"));
    exe.linkLibCpp();
    exe.addIncludePath(b.path("src"));
    // exe.addWin32ResourceFile(.{ .file = .{}, .flags = &.{} });

    exe.addCSourceFiles(.{ .files = &.{
        "src/main.cpp",
    }, .flags = &.{
        "-std=c++23",
        "-fno-rtti",
        "-fno-exceptions",
    } });

    b.installArtifact(exe);
}
