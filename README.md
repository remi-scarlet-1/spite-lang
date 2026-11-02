work in progress...

# Premise

A low-level programing language which is C-ABI compatible with strongly improved security, generic programing and major
QOL improvements, including saner defaults. It is meant to replace C++23/26 for my personal game development projects in
the future; idea stem out of the frustration with the state of C++'s ecosystem, complexity-to-attack-surface ratio and
compiler platform fragmentation.

Another major feature planned for the future are statically compiled and at runtime safe to load plugin binaries;
somewhat similar to WASM but with less restrictions. Essentially, the language's runtime hosts its own mini linker for
its own binary dynamic library format and can guarantee safe dynamic lib loading without any pointer casting witchcraft
or ABI assumptions. This will be achieved by all binaries produced via the compiler hosting a section with metadata
describing their own ABI, checkable by the runtime linker.

# Requirements

- C/C++ Toolchain, highly recommend LLVM-MinGW on Windows  
  (Only tested and officially supported: `llvm-23.1.3-mingw-w64-ucrt`)
- CMake >= 3.20 (don't use CMake 4 as it causes configuration errors)

# What to expect

Generating the project from CMake for the first time will take a while due LLVM being build from source! Let it finish.
