# KalaMake

KalaMake is a multithreaded binary compiler that uses ".kmake" files and it is intended to use as a replacement for other build scripts and generators like Make, Premake, CMake or Ninja. KalaMake automatically uses all available threads from your cpu to always help speed up compilation, or you can fill the jobs field with your desired job count.

KalaMake currently accepts several commands, most of which come from [KalaCLI](https://github.com/kalakit/kalacli) which is statically linked to KalaMake. Type '--help' to list all available commands and type --info commandnamehere to list info about that command.

Currently only C, C++, Windows (x86_64) and Linux (x86_64) are supported but more languages will be added in future versions. There are no plans to support mobile, console, ARM, x86 or macOS.

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
