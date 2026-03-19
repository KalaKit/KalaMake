# KalaMake updates

## 1.1

- removed unsupported standards: clatest, c++latest
- fixed compile_commands.json directory field value
- compile_commands.json will be moved to root project dir instead of each build dir
- added prebuildaction - you can now do actions before generation, compilation or linking even starts
- added better explanation why compiler and linker skipped their tasks
- only get header timestamps once during compilation instead of per object
