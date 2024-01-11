/*
 * Example build.cpp program that builds a library (static/dynamic)
 * and links to a another program
 */

#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

void build_static(int argc, char** argv) {
	CBuild lib("g++", argc, argv);
	lib
		.out("bin", "libsum.a")
		.flags({"-Wall"})
		.src({"lib/lib.cpp"})
		.build_static_lib()
		.clean();
}

void build_dynamic(int argc, char** argv) {
	CBuild lib("g++", argc, argv);
	lib
#ifdef _WIN32
		.out("bin", "libsum.dll")
#elif defined(__linux__)
		.out("bin", "libsum.so")
#endif
		.flags({"-Wl,-rpath='ORIGIN'", "-shared", "-fPIC"})
		.src({"lib/lib.cpp"})
		.build()
		.clean();
}

int main(int argc, char** argv) {
	build_dynamic(argc, argv);

	CBuild builder("g++", argc, argv);
	builder
		.out("bin", "test")
		.flags({"-Wall", "-Wl,-rpath='$ORIGIN'"})
		.inc_paths({"lib"})
		.lib_paths({"bin"})
		.libs({"sum"})
		.src({"src/main.cpp"})
		.build()
		.generate_compile_cmds()
		.clean()
		.run(argv);
}
