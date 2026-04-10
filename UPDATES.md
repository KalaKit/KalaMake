# KalaMake updates

## 1.1

- removed unsupported standards: clatest, c++latest
- fixed compile_commands.json directory field value to point at the root dir instead of the obj file
- compile_commands.json will be moved to root project dir instead of each build dir
- added prebuildaction - you can now do actions before generation, compilation or linking even starts
- added better explanation why compiler and linker skipped their tasks
- only get header timestamps once during compilation instead of per object
- removed linux and windows from targettype and added linux-gnu, linux-musl and windows-gnu, removed dumb custom flag use-clang-zig-msvc
- added proper cl compiler support on windows by requiring the user to pass vcvars64.bat or similar before calling kalamake if using cl compiler

## 1.2

- removed (somehow still) leftover clatest that isnt supported
- fixed c/cpp headers not correctly contributing to deciding if compilation should happen
- added exclusion support for sources and links so individual files or entire dirs can be excluded
- improved ignored source file rules to better handle different cases
- target project is rebuilt if its kmake file is newer than its object files or output file
- improved command param count detection, no longer giving error for invalid param count
- display --help when directly launching kalamake
- added --clean command which removes all found build paths from project file
- fixed issues with "," not working between paths in some
- added new compiler: java
- added new standards: java8 - java26
- added new custom flags: 'package-jar', 'java-win-console'

## 1.3

- added new compiler: zig
- added export-vscode-sln generation support for zig
- added new compiler: python (requires pyinstaller)
- added new custom flag export-java-sln for java - this creates both .classpath and .project
- added new compiler: python
- added new custom flag python-one-file
- fixed msvc-static-runtime not being blocked in Java, also blocked it in Zig and Python
- add lib in front of .so and .a for zig and c/cpp if missing in binary name
- compile and link flags are allowed to use quotes
- can use variables with just $ at their start and with no brackets (example: $ORIGIN) as long as they dont contain a starting curly bracket
- fixed global profile creating a nameless task and launch profile for export-vscode-sln
