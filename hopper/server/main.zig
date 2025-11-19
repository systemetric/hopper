const std = @import("std");

// Define options for logging, typically use debug level
pub const std_options: std.Options = .{
    .log_level = .debug,
};

pub fn main() !void {
    std.log.debug("hello world", .{});
}
