# How to use

This document explains what KalaMake is in more depth, what categories and fields are available and how to use them.

To run KalaMake you must first have a `.kmake` manifest file in your project root directory or wherever you wish to get all your files relative to when compiling software, games and libraries from source code.

To compile with KalaMake all you need is to launch KalaMake manually and type `--compile yourproject.kmake yourprofile` or directly type `kalamake --compile yourproject.kmake yourprofile` into your console. Read below for more details.

## Introduction

KalaMake is a standalone tool for building software and libraries from source code. It can replace tools such as CMake, Premake, Make or Ninja when you want a lightweight but fast and easy-to-use build script and CLI combination without relying on external build tools, generators or additional setup.

KalaMake uses files with the `.kmake` extension to define build configurations. A `.kmake` file is a plain text file that can be edited with any text editor and it is not a custom binary format.

The structure of a `.kmake` file is based on two core concepts: **categories** and **fields**. Comments are written using `//` and can be placed at the start of an empty line or at the end of any  existing line. Comments and empty lines are ignored during parsing.

Each category may only appear once in a `.kmake` file except profiles as long as their name is unique. The **version** and **global** categories must always be present. Each category except the **version** category contains fields and most fields require a value separated by `: `. Field and category order is irrelevant and you can structure it however you like as long as you understand that the content of one category ends at the start of the next line that starts with `#`. Commas split values only for fields that support multiple values. References do not use commas as value separators.

Each `.kmake` file must contain the `#version` and `#global` categories, `#references` and `#profile` are optional.

An object file for C/C++ may only be recompiled if its source file or any of the header dirs you've included is newer than the already built object file. An executable, static or shared lib for C/C++ may only be relinked if one of its linked non-system libraries or one of its objects is newer than the already built output.

## Version category

The version category tells KalaMake what the available fields and categories are. It prevents older versions from using new fields or categories that version did not yet have or from using deprecated or removed fields and categories in newer versions. You must add a version number after the `#version` category name.

Available versions:
- 1.0
    
Example:
```
#version 1.0
```

---

## References category

The references category tells KalaMake what the available references are for the rest of this `.kmake` file. Defined references are added with `${myref}` and defined in the references category. Each reference field name must be unique, reference field values must not contain more than one value. References are special - they allow the use of commas but only as the value of a reference, not for splitting values like with sources, headers, links, linkflags, compileflags or customflags.

Example:
```
#references
myref: 123456
myref2: 334455

#global
compiler: ${myref}
standard: ${myref2}
```

---

## Global category

The global category describes global state across all user-defined profiles so no profiles need to duplicate the same field again.

Available fields:
- binarytype
- compilerlauncher (optional)
- compiler
- standard
- targettype (optional)
- jobs (optional)
- binaryname
- buildtype
- buildpath
- sources
- headers (optional)
- links (optional)
- warninglevel (optional)
- defines (optional)
- compileflags (optional)
- linkflags (optional)
- customflags (optional)
- prebuildaction (optional)
- postbuildaction (optional)
    
### binarytype

Describes which binary type to build as. Only one value is allowed.

Available values:
- executable - creates an executable
- static - creates a static .lib (.a on linux)
- shared - creates a shared dll + inline .lib (.so on linux)

Static and shared are not supported in Java.
    
### compilerlauncher

Describes which compiler launcher to use for the compiler to cache object files, scripts and headers. Only one value is allowed.

Available values:
- ccache - reuses previous compile results
- sccache - same as ccache but supports shared cache servers
- distcc - sends compile jobs to other servers
- icecc - same as distcc but ships compiler toolchain too

Compiler launcher is not supported in Java.
    
### compiler

Describes which compiler to use. Only one value is allowed.

Available values:
- cl
- clang-cl
- gcc
- g++
- clang
- clang++
- zig
- java
    
### standard

Describes which language standard to use. Only one value is allowed.

Available values:
- c89
- c99
- c11
- c17
- c23

- c++98
- c++03
- c++11
- c++14
- c++17
- c++20
- c++23
- c++26

- java8 to java26
    
### targettype

Describes which target to aim for, used for cross-compilation. Only one value is allowed. Leave empty for current platform. Windows doesnt have a 'windows-msvc' equivalent so that cannot be passed as a target type.

Available values:
- windows-gnu - create a windows binary on windows or linux
- linux-gnu - create a linux binary on windows or linux (uses glibc)
- linux-musl - create a linux binary on windows or linux (uses musl)

Target type is not supported in Java.
    
### jobs

Describes how many jobs to use for the compilation pass. More jobs = more parallel threads for each source script. It is recommended to have up to 2x as many jobs as you have logical cores available on your cpu, limited from 1 to 65535. Only one value is allowed.

Jobs are not supported in Java.

### binaryname

Give a name for what your binary will be called. Extension is not needed. Only one value is allowed.

### buildtype

Describes what output target to aim for, adds the chosen build type flags. Only one value is allowed.

Available values:
- debug - full debug symbols, no size or speed optimization flags
- release - release output, optimizes for speed
- reldebug - release output, optimizes for speed, keeps debug symbols
- minsizerel - release output, optimizes for size

Release, reldebug and minsizerel are the same in Java.
    
### buildpath

Describes what folder to build the binary into. Accepts relative or full paths. Must be quoted with `"`. Only one value is allowed.

### sources

Describes what sources this binary will use. Supports quoted relative and full paths to files and folders, supports recursive and non-recursive globbing with `*` and `**`. Can add multiple values.

Sources support exclusion with the `!` symbol in front of the file or dir name, globbed paths do not support exclusion.

```
//exclude a source script
sources: "!myfile.cpp"
```

### headers

Describes what headers this binary will use. Supports quoted relative and full paths to folders, individual files are not allowed, supports recursive and non-recursive globbing with `*` and `**`. Can add multiple values.

Headers are not supported in Java.

### links

Describes what libraries this binary will link to. Supports quoted relative and full paths files and to folders, individual files are not allowed, supports recursive and non-recursive globbing with `*` and `**`, supports system library paths if added without quotes and extension. `-l` and `.lib` are added in front of the link internally to system library paths. Can add multiple values.

Links support exclusion with the `!` symbol in front of the file or dir name, globbed paths do not support exclusion.

Links are not supported in Java.

```
//exclude a library
links: "!mylink.lib"
```

### warninglevel

Describes how strict compiler warning levels should be. Only one value is allowed.

Available values:
- none - no warnings
- basic - very basic warnings
- normal - common, useful warnings, recommended default
- strong - strong warnings
- strict - very strict, high signal warnings
- all - all warnings

Warning level is not supported in Java.
    
### defines

Describes what defines to pass to the compiler. `-D` and `/D` are added in front of the define internally. Can add multiple values.

Defines are not supported in Java.

### compileflags

Describes what flags will be added during the compile stage. `-` and `/` are added in front of the flag internally. Can add multiple values.

### linkflags

Describes what flags will be added during the link stage. `-` and `/` are added in front of the flag internally. Can add multiple values.

Link flags are not supported in Java.

### customflags

Describes what KalaMake-specific flags will be added that will add extra actions. Can add multiple values.

Available values:
- export-compile-commands - creates the compile-commands.json file at the build dir of your profile
- export-vscode-sln - creates tasks.json and launch.json and their required .vscode folder if they dont exist, otherwise appends to existing files and overwrites profile with same name
- warnings-as-errors - all compiler or linker displayed warnings will be displayed as errors and will stop the build if encountered
- msvc-static-runtime - uses /MT or /MTd with cl and clang-cl instead of the default /MD or /MDd, unused in Linux
- package-jar - optional post-jar task to also package the created jar file into an executable
- java-win-console - print java executable logs to console on windows

Export-compile-commands is not supported in Java.
Package-jar and java-win-console are not supported in C and C++.
    
### prebuildaction

Describes what console-triggered action to do before any generation, compilation or linking starts. Only one value is allowed but more than one prebuildaction can be added to your profile.
    
### postbuildaction

Same as prebuildaction but these actions run after generation, compilation and linking is done and succeeds.

---

## Profile category

The profile category describes an override to the global category or additional data the global category did not define. Duplicated fields already found in the global profile that were added to a user profile are overridden if they contain a single value, otherwise they are appended to the user profile. You can choose to compile with the global profile or with a specific user-defined profile. You must add a profile name after the `#profile` category name, you can add as many profiles as you want as long as they all have unique names. All global category fields can be added to each profile.

## Examples

Minimal global profile

```
#global
compiler: clang-cl
standard: c++20
binarytype: executable
binaryname: myexe
buildtype: release
buildpath: "build"
sources: "src/main.cpp"
```

Full KalaMake project

```
//Build script for use with kalamake. Read more at https://github.com/kalakit/kalamake

#version 1.0

#references
name_bin: kalamake
dir_release: build/release-
dir_debug: build/debug-
name_kc: kalacli
dir_kc_rel: _external_shared/KalaCLI/release
dir_kc_deb: _external_shared/KalaCLI/debug
comm_objcopy: objcopy --remove-section .note.gnu.property

#global
compilerlauncher: ccache
standard: c++20
binarytype: executable
binaryname: ${name_bin}
sources: "src"
headers: "include", "_external_shared/KalaHeaders", "_external_shared/KalaCLI/include"
defines: LIB_STATIC
warninglevel: normal
customflags: export-compile-commands, export-vscode-sln

#profile debug-windows
compiler: clang-cl
buildtype: debug
buildpath: "${dir_debug}windows"
links: "${dir_kc_deb}/${name_kc}d.lib"

#profile release-windows
compiler: clang-cl
buildtype: minsizerel
buildpath: "${dir_release}windows"
links: "${dir_kc_rel}/${name_kc}.lib"

#profile debug-linux
compiler: clang++
buildtype: debug
buildpath: "${dir_debug}linux"
links: "${dir_kc_deb}/lib${name_kc}d.a"
//strips stupid avx requirements that cachyos seems to add for no reason
postbuildaction: ${comm_objcopy} ${dir_debug}linux/${name_bin}

#profile release-linux
compiler: clang++
buildtype: minsizerel
buildpath: "${dir_release}linux"
links: "${dir_kc_rel}/lib${name_kc}.a"
//strips stupid avx requirements that cachyos seems to add for no reason
postbuildaction: ${comm_objcopy} ${dir_release}linux/${name_bin}
postbuildaction: strip --strip-unneeded ${dir_release}linux/${name_bin}
```
