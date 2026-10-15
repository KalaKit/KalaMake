extern fn print(str: [*:0]const u8) void;

pub fn main() void
{
    print("hello from external logger!");
}