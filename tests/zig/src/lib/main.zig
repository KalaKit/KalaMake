const std = @import("std");

export fn print(str: [*:0]const u8) void
{
    std.debug.print("[LOG] {s}\n", .{str});
}