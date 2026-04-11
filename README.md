# KalaMake

KalaMake is a multithreaded CLI for building libraries and executables with `.kmake` files and it is intended to use as a replacement for other build scripts and generators like Make, Premake, CMake or Ninja. KalaMake automatically uses all available threads from your CPU to always help speed up compilation, or you can fill the jobs field with your desired job count, multithreading is not supported by all languages.

KalaMake currently accepts several commands, most of which come from [KalaCLI](https://github.com/kalakit/kalacli) which is statically linked to KalaMake. Type `--help` to list all available commands and type `--info commandnamehere` to list info about that command.

This project relies on several [external dependencies](https://github.com/greeenlaser/external-shared), they are not shipped inside this project, please make sure you have that repository cloned into a folder inside the same parent directory as this project folder before compiling this project from source.

## Supported languages

- C (89 to 23)
- C++ (98 to 26)
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

[Official Discord server](https://discord.gg/jkvasmTND5)

[Official Youtube channel](https://youtube.com/greenlaser)

---

## Docs

[How to build from source](docs/build_from_source.md)

[How to use](docs/how_to_use.md)

[External libraries](docs/external_libraries.md)

[Lost Empire Entertainment and KalaKit ecosystem](docs/ecosystem.md)
