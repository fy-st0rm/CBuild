#ifndef __CBUILD_H__
#define __CBUILD_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

//===========================
// DEFINITIONS BEGINS HERE!
//===========================

#ifdef __cplusplus
extern "C" {
#endif

// :cbuild def
typedef struct {
  const char* cc;
} CBuild;

void cbuild_rebuild_itself(int argc, char** argv, const char* src_file);
void cbuild_cc(CBuild* cb, const char* cc);

// :utils def
time_t cbuild_last_write_time(const char* path);
int cbuild_run_cmd(const char **args);
void cbuild_print_args(const char** args);

// :macros
#define cbuild_panic(x, ...) \
  do {\
    if (!(x)) {\
      fprintf(stderr, "\033[31m[ASSERTION]: %s:%d:\033[0m ", __FILE__, __LINE__);\
      fprintf(stderr, " " __VA_ARGS__);\
      fprintf(stderr, "\n");\
      exit(1);\
    }\
  } while(0)

#define cbuild_log(...)    \
  ({                       \
    printf("\033[%dm", 0); \
    printf("[LOG]: ");  \
    printf(__VA_ARGS__);   \
    printf("\033[%dm", 0); \
  })

#ifdef __cplusplus
}
#endif


//=============================
// IMPLEMENTATION BEGINS HERE!
//=============================

#ifdef CBUILD_IMPLEMENTATION

// :cbuild impl
void cbuild_rebuild_itself(int argc, char** argv, const char* src_file) {
  char* bin_file = argv[0];

  time_t bin_time = cbuild_last_write_time(bin_file);
  time_t src_time = cbuild_last_write_time(src_file);

  if (src_time > bin_time) {
    cbuild_log("Rebuilding %s\n", src_file);

    const char* build_string[] = {
      "gcc",
      "-o",
      bin_file,
      src_file,
      NULL
    };

    int status = cbuild_run_cmd(build_string);
    cbuild_panic(status == 0, "Rebuild failed");

    status = cbuild_run_cmd((const char**) argv);
    exit(status);
  }
}

void cbuild_cc(CBuild* cb, const char* cc) {
  cb->cc = cc;
}

// :utils impl
time_t cbuild_last_write_time(const char* path) {
#ifdef _WIN32
  cbuild_panic(false, "Not implemented for windows!");
#elif defined(__linux__)
  struct stat st;

  if (stat(path, &st) == -1)
    return 0;

  return st.st_mtime;
#endif
}

int cbuild_run_cmd(const char **args) {
  cbuild_print_args(args);

#ifdef _WIN32
  cbuild_panic(false, "Not implemented for windows!");
#elif defined(__linux__)
  pid_t pid = fork();

  cbuild_panic(pid != -1, "Failed to fork");

  if (pid == 0) {
    execvp(args[0], (char *const *)args);

    // Only reached if execvp failed
    _exit(127);
  }

  int status;
  waitpid(pid, &status, 0);

  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

void cbuild_print_args(const char** args) {
  cbuild_log("");
  for (const char **arg = args; *arg != NULL; arg++) {
    printf("%s ", *arg);
  }
  printf("\n");
}

#endif // CBUILD_IMPLEMENTATION

#endif // __CBUILD_H__
