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
void cbuild_string_list_free(CBuildStringList* sl);                      // Cleans the memory

// :cbuild def
typedef struct {
  const char* cc;
  const char* out;
  const char* out_dir;
  const char* artifacts_dir;
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

/* Adds flags to the build */
#define cbuild_flags(cb, ...) \
  cbuild_string_list_append_multiple(&(cb)->flags, __VA_ARGS__, NULL)

/* Sets the include paths */
#define cbuild_include_paths(cb, ...) \
  cbuild_string_list_append_multiple(&(cb)->inc_paths, __VA_ARGS__, NULL)

/* Sets the library paths */
#define cbuild_lib_paths(cb, ...) \
  cbuild_string_list_append_multiple(&(cb)->lib_paths, __VA_ARGS__, NULL)

/* Sets libraries to link with */
#define cbuild_libs(cb, ...) \
  cbuild_string_list_append_multiple(&(cb)->libs, __VA_ARGS__, NULL)

/* Append source files */
#define cbuild_srcs(cb, ...) \
  cbuild_string_list_append_multiple(&(cb)->srcs, __VA_ARGS__, NULL)

void cbuild_execute(CBuild* cb);                         // Runs the builder and compiles the project
int cbuild_run_bin(CBuild* cb, int argc, char** argv);   // Runs the output executable

// :utils def
time_t cbuild_last_write_time(const char* path);  // Returns the write time of the file
int cbuild_run_cmd(const char **args);            // Runs the cmd in different process
void cbuild_print_args(const char** args);        // Prints the arguments
int cbuild_mkdir(const char* path);               // Makes a single directory

// :incremental def

// Parses a GCC/Clang -MMD -MF dependency (.d) file into a flat list of the
// files the object depends on (source + headers it pulled in).
CBuildStringList __cbuild_parse_dep_file(const char* dep_path);

// Decides whether `src_path` needs to be recompiled into `obj_path`, using
// `dep_path` (its .d file) to know which headers it depends on.
bool __cbuild_needs_rebuild(const char* obj_path, const char* dep_path, const char* src_path);

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

void cbuild_string_list_free(CBuildStringList* sl) {
  for (int i = 0; i < sl->len; i++)
    free((void*)sl->items[i]);

  free(sl->items);

  sl->items = NULL;
  sl->len = 0;
  sl->capacity = 0;
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
  } else {
    size_t len = slash - out;

    char* out_dir = (char*) malloc(len + 1);
    cbuild_panic(out_dir != NULL, "Failed to allocate output directory");

    memcpy(out_dir, out, len);
    out_dir[len] = '\0';

    cb->out_dir = out_dir;
  }

  cbuild_panic(cbuild_mkdir(cb->out_dir) == 0, "Failed to create out dir");

  // Artifacts dir: where .o/.d files for incremental builds live, kept
  // separate from out_dir so it doesn't clutter the final binary's folder.
  size_t artifacts_len = strlen(cb->out_dir) + strlen("/__cbuild_artifacts") + 1;
  char* artifacts_dir = (char*) malloc(artifacts_len);
  cbuild_panic(artifacts_dir != NULL, "Failed to allocate artifacts directory");
  snprintf(artifacts_dir, artifacts_len, "%s/__cbuild_artifacts", cb->out_dir);

  cb->artifacts_dir = artifacts_dir;
  cbuild_panic(cbuild_mkdir(cb->artifacts_dir) == 0, "Failed to create artifacts dir");
}

int cbuild_execute_single(CBuild* cb, const char* file) {
  char obj[PATH_MAX];
  char dep[PATH_MAX];

  const char *name = strrchr(file, '/');
  if (name)
    name++;
  else
    name = file;

  const char *dot = strrchr(name, '.');
  int base_len = dot ? (int)(dot - name) : (int)strlen(name);

  snprintf(obj, sizeof(obj), "%s/%.*s.o", cb->artifacts_dir, base_len, name);
  snprintf(dep, sizeof(dep), "%s/%.*s.d", cb->artifacts_dir, base_len, name);

  // Save the obj so it's linked in regardless of whether we recompile it
  cbuild_string_list_append(&cb->objs, obj);

  if (!__cbuild_needs_rebuild(obj, dep, file)) {
    cbuild_log("Up to date, skipping: %s\n", file);
    return 0;
  }

  CBuildStringList cmd = {0};

  // Main build cmd
  cbuild_string_list_append(&cmd, cb->cc);
  cbuild_string_list_append(&cmd, "-o");
  cbuild_string_list_append(&cmd, obj);
  cbuild_string_list_append(&cmd, "-c");
  cbuild_string_list_append(&cmd, file);

  // Emit a .d file alongside the object listing every header this
  // translation unit pulled in, so the next build knows what to watch.
  // -MMD (not -MD) skips system headers -- they're not expected to change.
  cbuild_string_list_append(&cmd, "-MMD");
  cbuild_string_list_append(&cmd, "-MF");
  cbuild_string_list_append(&cmd, dep);

  // flags
  for (int i = 0; i < cb->flags.len; i++)
    cbuild_string_list_append(&cmd, cb->flags.items[i]);

  // include paths
  for (int i = 0; i < cb->inc_paths.len; i++) {
    cbuild_string_list_append(&cmd, "-I");
    cbuild_string_list_append(&cmd, cb->inc_paths.items[i]);
  }

  // NULL-terminate: execvp (via cbuild_run_cmd) needs a NULL sentinel,
  // and append() never adds one on its own
  cmd.items[cmd.len] = NULL;

  int status = cbuild_run_cmd(cmd.items);
  cbuild_string_list_free(&cmd);

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

  // NULL-terminate: execvp (via cbuild_run_cmd) needs a NULL sentinel,
  // and append() never adds one on its own
  cmd.items[cmd.len] = NULL;

  int status = cbuild_run_cmd(cmd.items);

  cbuild_panic(
    status == 0,
    "Linking failed"
  );

  cbuild_string_list_free(&cmd);
}

int cbuild_run_bin(CBuild* cb, int argc, char** argv) {
  CBuildStringList args = {0};

  cbuild_string_list_append(&args, cb->out);

  for (int i = 1; i < argc; i++)
    cbuild_string_list_append(&args, argv[i]);

  // NULL Terminate it
  args.items[args.len] = NULL;

  int status = cbuild_run_cmd(args.items);

  cbuild_string_list_free(&args);

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

// :incremental impl

CBuildStringList __cbuild_parse_dep_file(const char* dep_path) {
  CBuildStringList deps = {0};

  FILE* f = fopen(dep_path, "r");
  if (f == NULL)
    return deps; // Missing .d -> caller treats this as "deps unknown"

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return deps;
  }

  long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return deps;
  }
  fseek(f, 0, SEEK_SET);

  char* buf = (char*) malloc((size_t)size + 1);
  cbuild_panic(buf != NULL, "Failed to allocate dep file buffer");

  size_t read_bytes = fread(buf, 1, (size_t)size, f);
  buf[read_bytes] = '\0';
  fclose(f);

  // Fold "\<newline>" line continuations into plain whitespace
  for (char* p = buf; *p; p++) {
    if (p[0] == '\\' && p[1] == '\n') {
      p[0] = ' ';
      p[1] = ' ';
    }
  }

  // Everything after the rule's ':' is the dependency list -- but only up
  // to the first blank line, in case -MP appended phony rules afterward
  char* colon = strchr(buf, ':');
  char* deps_begin = colon ? colon + 1 : buf;

  // Backslash-newline continuations were just folded away above, so any
  // '\n' still left in deps_begin genuinely ends the main rule -- whatever
  // follows is -MP's phony-rule block (if present) or trailing whitespace.
  char* boundary = strchr(deps_begin, '\n');
  if (boundary)
    *boundary = '\0';

  char* p = deps_begin;

  while (*p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
      p++;

    if (!*p)
      break;

    char* start = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
      p++;

    char saved = *p;
    *p = '\0';
    cbuild_string_list_append(&deps, start);
    *p = saved;

    if (saved)
      p++;
  }

  free(buf);
  return deps;
}

bool __cbuild_needs_rebuild(const char* obj_path, const char* dep_path, const char* src_path) {
  time_t obj_time = cbuild_last_write_time(obj_path);

  // No object file for this source yet -> must compile
  if (obj_time == 0)
    return true;

  CBuildStringList deps = __cbuild_parse_dep_file(dep_path);

  // No .d file (deleted, or predates incremental builds) -> deps are
  // unknown, so rebuild rather than risk a stale binary
  if (deps.len == 0)
    return true;

  bool stale = false;

  // Check the source itself too, in case the .d file is somehow out of sync
  if (cbuild_last_write_time(src_path) > obj_time)
    stale = true;

  for (int i = 0; !stale && i < deps.len; i++) {
    time_t dep_time = cbuild_last_write_time(deps.items[i]);

    // A dependency that vanished (dep_time == 0, e.g. a deleted/renamed
    // header) means the dependency graph itself changed -> rebuild so the
    // compiler surfaces the real error instead of silently reusing the .o
    if (dep_time == 0 || dep_time > obj_time)
      stale = true;
  }

  cbuild_string_list_free(&deps);
  return stale;
}

#endif // CBUILD_IMPLEMENTATION

#endif // __CBUILD_H__
