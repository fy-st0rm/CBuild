/*
 * Example build.cpp program that builds a library (static/dynamic)
 * and links to a another program
 */

#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

void build_static() {
	CBuild lib("g++");
	lib
		.out("bin", "libsum.a")
		.flags({"-Wall"})
		.src({"lib/lib.cpp"})
		.build_static_lib()
		.clean();
}

void build_dynamic() {
	CBuild lib("g++");
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
	cbuild_rebuild(argc, argv);

	build_dynamic();

	CBuild builder("g++");
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
