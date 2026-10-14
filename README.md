# KalaMake

KalaMake is a multithreaded CLI for building libraries and executables with `.kmake` files and it is intended to use as a full replacement for other build scripts and generators like Make, Premake, CMake or Ninja. KalaMake automatically uses all available threads from your CPU to always help speed up compilation, or you can fill the jobs field with your desired job count, multithreading is not supported by all languages.

KalaMake currently accepts several commands, most of which come from [KalaCLI](https://github.com/kalakit/kalacli) which is statically linked to KalaMake. Type `--help` to list all available commands and type `--info commandnamehere` to list info about that command.

Please be aware that this library/software has limited or no documentation at the current stage due to the KalaKit and the Elypso Engine ecosystem being in early development, if you have questions then message me on discord at @kirjukala
 or via [email](mailto:sanderveski@gmail.com?subject=Questions%20about%20KalaKit%20and%20the%20Elypso%20Engine%20ecosystem). The website linked at the right side also does not currently function because both the domain and its [server backend](https://github.com/kalakit/kalaserver) are still in early development.

This project relies on several [external dependencies](external), this repository [license](LICENSE.md) does not apply to them.

## Minimal compile example

To compile Kalamake on Windows without Kalamake:
```
(if exist build rmdir /s /q build) & mkdir build && mkdir build\release-windows && mkdir build\release-windows\obj && cd build\release-windows && (for /r "..\..\src" %f in (*.cpp) do @clang++ -DLIB_STATIC -std=c++20 -c "%f" -I"..\..\include" -I"..\..\external\kalaheaders\include" -I"..\..\external\kalacli\include" -o "obj\%~nf.obj") && clang++ -o kalamake.exe obj\*.obj "..\..\external\kalacli\kalacli.lib" -luser32 && echo Build succeeded!
```
And on Linux:
```
rm -rf build; and mkdir build; and mkdir build/release-linux; and mkdir build/release-linux/obj; and cd build/release-linux; and for f in ../../src/**/*.cpp; clang++ -DLIB_STATIC -std=c++20 -c "$f" -I"../../include" -I../../external/kalaheaders/include -I"../../external/kalacli/include" -o obj/(basename $f .cpp).o; end; and clang++ -o kalamake obj/*.o ../../external/kalacli/libkalacli.a; and echo Build succeeded!
```

## Supported languages

- C (89 to 23)
- C++ (14 to 26)
- Java (8 to 26)
- Zig (the language)
- Python (Requires PyInstaller)
- Rust (2015 to 2024)

## Upcoming languages

- C#
- Go
- Assembly
- Odin
- Nim

Currently only Windows (x86_64) and Linux (x86_64) are supported. There are no plans to support BSD, mobile, console, ARM, x86 or macOS.

## Links

[Donate on PayPal](https://www.paypal.com/donate/?hosted_button_id=QWG8SAYX5TTP6)

[Official Discord server](https://discord.gg/BZ2eJ5Cdr8)

[Official Youtube channel](https://youtube.com/greenlaser)

---

## Docs

[How to build from source](docs/build_from_source.md)

[How to use](docs/how_to_use.md)

[Lost Empire Entertainment and KalaKit ecosystem](docs/ecosystem.md)
