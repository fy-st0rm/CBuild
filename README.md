# CBuild

**CBuild** is a simple, single-header build system for C and C++ projects.

Instead of writing a separate build configuration file, Makefile, or scripting language, you write your build system directly in **C/C++** using the CBuild API.

> **Write your build system in C++. Build your C/C++ projects with CBuild.**

---

## Features

* 📦 **Single-header library** — just include `cbuild.h`
* 🛠️ **C/C++ based configuration** — write your build logic in C/C++
* ⚡ **Incremental builds** — only rebuild files affected by changes
* 🔄 **Automatic self-rebuilding** — CBuild can rebuild itself when its build script changes
* 🧩 **Multiple source files** — compile and link projects from multiple C/C++ sources
* 📁 **Include & library paths** — easily configure header and library search paths
* 🔗 **Library linking** — specify libraries directly from your build script
* 🚀 **Run after building** — optionally execute the resulting binary automatically
* 🧰 **Compiler flags** — add arbitrary compiler and linker flags
* 🔌 **C and C++ support** — use GCC, G++, Clang, or other compatible compilers

---

## Quick Start

### 1. Add CBuild

Copy `cbuild.h` into your project.

Your project can look like:

```text
my-project/
├── cbuild.c
├── cbuild.h
├── src/
│   └── main.cpp
└── lib/
    └── lib.cpp
```

### 2. Write your build script

Create `cbuild.c`:

```c
#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
    cbuild_rebuild_itself(argc, argv, "gcc");

    CBuild cbuild = {0};

    cbuild_cc(&cbuild, "g++");
    cbuild_out(&cbuild, "build/test");

    cbuild_flags(
        &cbuild,
        "-Wall",
        "-Werror"
    );

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

---

## Bootstrapping

CBuild itself is just a C/C++ program, so bootstrapping it is straightforward:

```bash
gcc -o cbuild cbuild.c
```

Then run:

```bash
./cbuild
```

After bootstrapping, CBuild can automatically **rebuild itself whenever `cbuild.c` changes**:

```text
cbuild.c
   │
   │ changed
   ▼
CBuild detects change
   │
   ▼
Rebuild cbuild
   │
   ▼
Execute build system
   │
   ▼
Build project
```

This allows the build system to evolve alongside the project without manually recompiling the build script every time it changes.

---

## Incremental Builds

CBuild includes an **incremental build system** designed to avoid unnecessary compilation.

Instead of recompiling every source file on every build, CBuild tracks the relationships between source files and their dependencies and rebuilds only what is affected by a change.

For example:

```text
src/
├── main.cpp
├── player.cpp
└── enemy.cpp

include/
├── player.h
└── enemy.h
```

If `player.h` changes:

```text
player.h
   │
   ├── player.cpp  → rebuild
   └── main.cpp    → rebuild if it depends on player.h

enemy.cpp           → unchanged
```

This makes repeated builds significantly faster as projects grow.

## Build Configuration

CBuild exposes a small API for configuring your build.

### Compiler

```c
cbuild_cc(&cbuild, "g++");
```

For C:

```c
cbuild_cc(&cbuild, "gcc");
```

You can also use compatible compilers such as Clang.

### Output

```c
cbuild_out(&cbuild, "build/game");
```

### Compiler Flags

```c
cbuild_flags(
    &cbuild,
    "-Wall",
    "-Wextra",
    "-Werror"
);
```

### Include Paths

```c
cbuild_include_paths(
    &cbuild,
    "./include",
    "./third_party/include"
);
```

### Library Paths

```c
cbuild_lib_paths(
    &cbuild,
    "./build",
    "./third_party/lib"
);
```

### Libraries

```c
cbuild_libs(
    &cbuild,
    "m",
    "SDL2",
    "GL"
);
```

### Source Files

```c
cbuild_srcs(
    &cbuild,
    "src/main.cpp",
    "src/player.cpp",
    "src/game.cpp"
);
```

---

## Build and Run

Once your build configuration is ready:

```c
cbuild_execute(&cbuild);
cbuild_run_bin(&cbuild, argc, argv);
```

`cbuild_execute()` performs the build, while `cbuild_run_bin()` runs the resulting executable.

This makes the development loop simple:

```text
Edit → Build → Run
```

with a single command:

```bash
./cbuild
```

---

## Contributing

Contributions, improvements, bug fixes, and ideas are welcome.

If you find a bug or have an idea for improving CBuild, feel free to open an issue or submit a pull request.
