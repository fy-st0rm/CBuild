#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
  cbuild_rebuild_itself(argc, argv, __FILE__);

  printf("Hello World\n");

  CBuild cbuild = {0};
  cbuild_cc(&cbuild, "gcc");
  // cbuild_out(&cbuild, "build/game");
  // cbuild_flags(&cbuild, "-D_asd", "-Wall", "-Werror");
  // cbuild_include_paths(
  //   &cbuild,
  //   "./lib/includes/",
  //   "./includes/",
  // );
  // cbuild_lib_paths(
  //   &cbuild,
  //   "./lib/libs/",
  //   "./includes/",
  // );
  // cbuild_libs(
  //   &cbuild,
  //   "m",
  //   "GLU",
  // );
  // cbuild_src(
  //   &cbuild,
  //   "src/main.c",
  //   "src/main.c",
  //   "src/main.c",
  //   "src/main.c",
  //   "src/main.c"
  // );

  // cbuild_execute(&cbuild);
  // cbuild_run_bin(&cbuild);

  return 0;
}
