# CBuild

A Simple single header library to build C/C++ programs.
Build your C/C++ projects by writing your build system in C++.

## Example "cbuild.cpp"

```c++
#define CBUILD_IMPLEMENTATION
#include <cbuild.h>

int main(int argc, char** argv) {
    cbuild_rebuild(argc, argv);     // Rebuilds the build program when changed
    CBuild cbuild("g++");
    build
        .out("bin", "example")       // Define output dir and output file name
        .flags({"-Wall"})            // Flags to provide
        .src({"src/example.cpp"})    // List of source files
        .build()                     // Execute build process
        .clean()                     // Clean object files
        .run(argc, argv);            // Run the built program
    return 0;
}
```

## Bootstraping cbuild

```bash
$ g++ -o cbuild cbuild.cpp
$ ./cbuild
```

#### After Bootstrapping the cbuild program will rebuilds when "cbuild.cpp" changes




