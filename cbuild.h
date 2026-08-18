#ifndef __CBUILD_H__
#define __CBUILD_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

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

// :string def
#define DA_START_CAP 50

// Dynamic List of Strings
// items    = Array of Strings which owns the string (heap allocated)
// len      = Length of the array
// capacity = Total allocated list
typedef struct {
  const char** items;
  int len;
  int capacity;
} CBuildStringList;

void cbuild_string_list_append(CBuildStringList* sl, const char* item);  // Append single string
void cbuild_string_list_append_multiple(CBuildStringList* sl, ...);      // Append multiple string
void cbuild_string_list_print(CBuildStringList* sl);                     // Print strings

// :cbuild def
typedef struct {
  const char* cc;
  const char* out;
  const char* out_dir;
  CBuildStringList flags;
  CBuildStringList inc_paths;
  CBuildStringList lib_paths;
  CBuildStringList libs;
  CBuildStringList srcs;
  CBuildStringList objs;
} CBuild;

// Rebuilds the builder if the builder source file is modified
#define cbuild_rebuild_itself(argc, argv)\
  __cbuild_rebuild_itself(argc, argv, __FILE__)
void __cbuild_rebuild_itself(int argc, char** argv, const char* src_file);

void cbuild_cc(CBuild* cb, const char* cc);    // Sets the compiler (gcc,g++,clang,etc)
void cbuild_out(CBuild* cb, const char* out);  // Sets the output path (build/executable)

#define cbuild_flags(cb, ...)\                                              // Adds flags to the build
  cbuild_string_list_append_multiple(&(cb)->flags, __VA_ARGS__, NULL) 
#define cbuild_include_paths(cb, ...)\                                      // Sets the include paths
  cbuild_string_list_append_multiple(&(cb)->inc_paths, __VA_ARGS__, NULL)
#define cbuild_lib_paths(cb, ...)\                                          // Sets the library paths
  cbuild_string_list_append_multiple(&(cb)->lib_paths, __VA_ARGS__, NULL)
#define cbuild_libs(cb, ...)\                                               // Sets libraries to link with
  cbuild_string_list_append_multiple(&(cb)->libs, __VA_ARGS__, NULL)
#define cbuild_srcs(cb, ...)\                                               // Append source files
  cbuild_string_list_append_multiple(&(cb)->srcs, __VA_ARGS__, NULL)

void cbuild_execute(CBuild* cb);                         // Runs the builder and compiles the project
int cbuild_run_bin(CBuild* cb, int argc, char** argv);   // Runs the output executable

// :utils def
time_t cbuild_last_write_time(const char* path);  // Returns the write time of the file
int cbuild_run_cmd(const char **args);            // Runs the cmd in different process
void cbuild_print_args(const char** args);        // Prints the arguments
int cbuild_mkdir(const char* path);               // Makes a single directory

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

// :string impl
void cbuild_string_list_append(CBuildStringList* sl, const char* item) {
  if (sl->len >= sl->capacity) {
    if (sl->capacity <= 0)
      sl->capacity = DA_START_CAP;
    else
      sl->capacity *= 2;

    sl->items = (const char**) realloc(
      sl->items,
      sl->capacity * 256
    );
  }

  sl->items[sl->len++] = strdup(item);
}

void cbuild_string_list_append_multiple(CBuildStringList* sl, ...) {
  va_list args;
  va_start(args, sl);

  const char *arg;

  while ((arg = va_arg(args, const char *)) != NULL) {
    cbuild_string_list_append(sl, arg);
  }

  va_end(args);
}

void cbuild_string_list_print(CBuildStringList* sl) {
  cbuild_log("");
  for (int i = 0; i < sl->len; i++) {
    printf("%s ", sl->items[i]);
  }
  printf("\n");
}


// :cbuild impl
void __cbuild_rebuild_itself(int argc, char** argv, const char* src_file) {
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

void cbuild_out(CBuild* cb, const char* out) {
  cb->out = out;

  const char *slash = strrchr(out, '/');

  if (slash == NULL) {
    cb->out_dir = ".";
    return;
  }

  size_t len = slash - out;

  char* out_dir = (char*) malloc(len + 1);
  cbuild_panic(out_dir != NULL, "Failed to allocate output directory");

  memcpy(out_dir, out, len);
  out_dir[len] = '\0';

  cb->out_dir = out_dir;
  cbuild_panic(cbuild_mkdir(cb->out_dir) == 0, "Failed to create out dir");
}

int cbuild_execute_single(CBuild* cb, const char* file) {
  CBuildStringList cmd = {0};

  char obj[PATH_MAX];
  const char *name = strrchr(file, '/');
  if (name)
    name++;
  else
    name = file;

  snprintf(
    obj,
    sizeof(obj),
    "%s/%.*s.o",
    cb->out_dir,
    (int)(strrchr(name, '.') - name),
    name
  );

  // Save the obj
  cbuild_string_list_append(&cb->objs, obj);

  // Main build cmd
  cbuild_string_list_append(&cmd, cb->cc);
  cbuild_string_list_append(&cmd, "-o");
  cbuild_string_list_append(&cmd, obj);
  cbuild_string_list_append(&cmd, "-c");
  cbuild_string_list_append(&cmd, file);

  // flags
  for (int i = 0; i < cb->flags.len; i++)
    cbuild_string_list_append(&cmd, cb->flags.items[i]);

  // include paths
  for (int i = 0; i < cb->inc_paths.len; i++) {
    cbuild_string_list_append(&cmd, "-I");
    cbuild_string_list_append(&cmd, cb->inc_paths.items[i]);
  }

  int status = cbuild_run_cmd(cmd.items);
  free(cmd.items);

  return status;
}

void cbuild_execute(CBuild* cb) {
  for (int i = 0; i < cb->srcs.len; i++) {
    const char* src = cb->srcs.items[i];
    int status = cbuild_execute_single(cb, src);
    cbuild_panic(status == 0, "Compilation failed for: %s", src);
  }

  // Linking
  CBuildStringList cmd = {0};

  cbuild_string_list_append(&cmd, cb->cc);

  // Output
  cbuild_string_list_append(&cmd, "-o");
  cbuild_string_list_append(&cmd, cb->out);

  // Object files
  for (int i = 0; i < cb->objs.len; i++) {
    cbuild_string_list_append(&cmd, cb->objs.items[i]);
  }

  // Library paths
  for (int i = 0; i < cb->lib_paths.len; i++) {
    cbuild_string_list_append(&cmd, "-L");
    cbuild_string_list_append(&cmd, cb->lib_paths.items[i]);
  }

  // Libraries
  for (int i = 0; i < cb->libs.len; i++) {
    cbuild_string_list_append(&cmd, "-l");
    cbuild_string_list_append(&cmd, cb->libs.items[i]);
  }

  int status = cbuild_run_cmd(cmd.items);

  cbuild_panic(
    status == 0,
    "Linking failed"
  );

  free(cmd.items);
}

int cbuild_run_bin(CBuild* cb, int argc, char** argv) {
  CBuildStringList args = {0};

  cbuild_string_list_append(&args, cb->out);

  for (int i = 1; i < argc; i++)
    cbuild_string_list_append(&args, argv[i]);

  // NULL Terminate it
  args.items[args.len] = NULL;

  int status = cbuild_run_cmd(args.items);

  free(args.items);

  return status;
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

int cbuild_mkdir(const char* path) {
#ifdef _WIN32
  cbuild_panic(false, "Not implemented for windows!");

#elif defined(__linux__)
  if (mkdir(path, 0755) == 0)
      return 0;

  if (errno == EEXIST)
      return 0;

  return -1;
#endif
}

#endif // CBUILD_IMPLEMENTATION

#endif // __CBUILD_H__
