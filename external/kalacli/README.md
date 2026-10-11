# KalaCLI

This is a cli executable template with a custom terminal layout for C++ 20 on Windows and Linux.

The CLI draws its own borders, draws the page view box where the main content is drawn into, its title bar and an input field where you can type commands into for a more terminal-like experience like oldschool DOS or TempleOS or whatever.

Please be aware that this library/software has limited or no documentation at the current stage due to the KalaKit and the Elypso Engine ecosystem being in early development, if you have questions then message me on discord at @kirjukala
 or via [email](mailto:sanderveski@gmail.com?subject=Questions%20about%20KalaKit%20and%20the%20Elypso%20Engine%20ecosystem). The website linked at the right side also does not currently function because both the domain and its [server backend](https://github.com/kalakit/kalaserver) are still in early development.

This project relies on several [external dependencies](external), this repository [license](LICENSE.md) does not apply to them.

## Projects that use this template

[KalaMake](https://github.com/kalakit/kalamake)

## Minimal compile example

To compile KalaCLI on Windows without Kalamake:
```
//create the root build dir
mkdir build

//create the target binary dir
mkdir build\release-windows

//create the object dir
mkdir build\release-windows\obj

//go to the target binary dir
cd build\release-windows

//compile cpp files
for /r "..\..\src" %f in (*.cpp) do clang++ -DLIB_STATIC -std=c++20 -c "%f" -I"..\..\include" -I"..\..\external\kalaheaders\include" -o "obj\%~nf.obj"

//link into library
llvm-lib /OUT:kalacli.lib "obj\*.obj"
```
And on Linux:
```
//create the root build dir
mkdir build

//create the target binary dir
mkdir build\release-linux

//create the object dir
mkdir build\release-linux\obj

//go to the target binary dir
cd build\release-linux

//compile cpp files
for f in ../../src/**/*.cpp
    clang++ -DLIB_STATIC -std=c++20 -c "$f" -I"../../include" -I"../../external/KalaHeaders/include" -o obj/(basename $f .cpp).o
end

//link into library
ar rcs libkalacli.a obj/*.o
```

---

## Links

[Donate on PayPal](https://www.paypal.com/donate/?hosted_button_id=QWG8SAYX5TTP6)

[Official Discord server](https://discord.gg/BZ2eJ5Cdr8)

[Official Youtube channel](https://youtube.com/greenlaser)

---

## Docs

[How to build from source](docs/build_from_source.md)

[Lost Empire Entertainment and KalaKit ecosystem](docs/ecosystem.md)
