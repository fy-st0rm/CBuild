# CBuild

A Simple single header library to build C/C++ programs.
Build your C/C++ projects by writing your build system in C++.

## Example "cbuild.cpp"

```c
#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
  cbuild_rebuild_itself(argc, argv);

  CBuild cbuild = {0};
  cbuild_cc(&cbuild, "g++");
  cbuild_out(&cbuild, "build/test");
  cbuild_flags(&cbuild, "-Wall", "-Werror");
  cbuild_include_paths(
    &cbuild,
    "./lib"
  );
  cbuild_lib_paths(
    &cbuild,
    "./build"
  );
  cbuild_libs(
    &cbuild,
    "m"
  );
  cbuild_srcs(
    &cbuild,
    "lib/lib.cpp",
    "src/main.cpp"
  );
  cbuild_execute(&cbuild);
  cbuild_run_bin(&cbuild, argc, argv);

  return 0;
}
```

## Bootstraping cbuild

```bash
$ gcc -o cbuild cbuild.c
$ ./cbuild
```

#### After Bootstrapping the cbuild program will rebuilds when "cbuild.c" changes




