#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
  cbuild_rebuild_itself(argc, argv);

  CBuild cbuild = {0};
  cbuild_cc(&cbuild, "g++");
  cbuild_out(&cbuild, "build/test");
  cbuild_flags(&cbuild, "-MMD", "-MP", "-Wall", "-Werror");
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
