#pragma once

#include <fstream>
#include <numeric>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <time.h>
#include <unistd.h>
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#define stat _stat
#endif

#define LOG(...) \
	printf("[LOG]: ");\
	printf(__VA_ARGS__);\
	printf("\n")

bool is_dir_exist(const std::string& path);
bool make_path(const std::string& path);
time_t last_write_time(const std::string& path);
void cbuild_rebuild(int argc, char** argv);
std::string vec_join(const std::vector<std::string>& vec, const std::string& prefix = "");
bool replace(std::string& str, const std::string& from, const std::string& to);
bool contains(const std::string& src, const std::string& to_find);

typedef struct {
	std::string dir, cmd, file;
} CompileCommand;

class CBuild {
public:
	CBuild (std::string cc);
	~CBuild();

	CBuild& out(std::string out_dir, std::string out_file);
	CBuild& flags(std::vector<std::string> flags);
	CBuild& objs(std::vector<std::string> objs);
	CBuild& src(std::vector<std::string> src);
	CBuild& inc_paths(std::vector<std::string> inc_paths);
	CBuild& lib_paths(std::vector<std::string> lib_paths);
	CBuild& libs(std::vector<std::string> libs);
	CBuild& pop_objs(std::vector<std::string> objs);
	CBuild& compile();
	CBuild& build();
	CBuild& build_static_lib();
// CBuild& generate_compile_cmds();
	CBuild& run(char** argv = NULL);
	CBuild& clean();

private:
	bool compile_single(const std::string& src);

private:
	std::string m_cc, m_out_dir, m_out_file;
	std::vector<std::string> m_flags, m_src, m_objs, m_inc_paths, m_lib_paths, m_libs;
	std::vector<CompileCommand> m_cmp_cmds;
};


#ifdef CBUILD_IMPLEMENTATION

bool is_dir_exist(const std::string& path) {
	struct stat info;
	if (stat(path.c_str(), &info) != 0) {
		return false;
	}
#if defined(_WIN32)
	return (info.st_mode & _S_IFDIR) != 0;
#else 
	return (info.st_mode & S_IFDIR) != 0;
#endif
}

bool make_path(const std::string& path) {
#if defined(_WIN32)
	int ret = _mkdir(path.c_str());
#else
	mode_t mode = 0755;
	int ret = mkdir(path.c_str(), mode);
#endif
	if (ret == 0)
		return true;

	switch (errno) {
		case ENOENT:
			// parent didn't exist, try to create it
			{
				int pos = path.find_last_of('/');
				if (pos == std::string::npos)
#if defined(_WIN32)
					pos = path.find_last_of('\\');
				if (pos == std::string::npos)
#endif
					return false;
				if (!make_path( path.substr(0, pos) ))
					return false;
			}
			// now, try to create again
#if defined(_WIN32)
			return 0 == _mkdir(path.c_str());
#else 
			return 0 == mkdir(path.c_str(), mode);
#endif

		case EEXIST:
			// done!
			return is_dir_exist(path);

		default:
			return false;
	}
}

time_t last_write_time(const std::string& path) {
	struct stat result;
	if(stat(path.c_str(), &result) == 0)
	{
		return result.st_mtime;
	}
	LOG("Failed to read stats of file: %s", path.c_str());
	exit(1);
}

void cbuild_rebuild(int argc, char** argv) {
#ifdef _WIN32
	time_t bin_t = last_write_time("cbuild.exe");
#elif defined(__linux__)
	time_t bin_t = last_write_time("cbuild");
#endif
	time_t src_t = last_write_time("cbuild.cpp");

	if (src_t > bin_t) {
		LOG("Rebuilding cbuild.cpp");
		std::string cmd = "g++ -o cbuild cbuild.cpp";
		LOG(cmd.c_str());

		int status = std::system(cmd.c_str());
		if (status) {
			LOG("Rebuilding failed.");
			exit(1);
		}

		std::vector<std::string> args(argv, argv + argc);
		std::string rerun = "./cbuild " + vec_join(args);

		status = std::system(rerun.c_str());
		exit(status);
	}
}

std::string vec_join(const std::vector<std::string>& vec, const std::string& prefix) {
	std::string out;
	for (auto e : vec) {
		out += prefix + e + " ";
	}
	return out;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if(start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

bool contains(const std::string& src, const std::string& to_find) {
	if (src.find(to_find) != std::string::npos)
		return true;
	return false;
}

CBuild::CBuild(std::string cc)
	: m_cc(cc) {
}

CBuild::~CBuild() {
}

CBuild& CBuild::out(std::string out_dir, std::string out_file) {
	if (out_dir.length()) {
		LOG("Generating dir: %s", out_dir.c_str());
		make_path(out_dir);
	}
	m_out_dir  = out_dir;
	m_out_file = out_file;
	return *this;
}

CBuild& CBuild::flags(std::vector<std::string> flags) {
	m_flags.insert(m_flags.end(), flags.begin(), flags.end());
	return *this;
}

CBuild& CBuild::objs(std::vector<std::string> objs) {
	m_objs.insert(m_objs.end(), objs.begin(), objs.end());
	return *this;
}

CBuild& CBuild::src(std::vector<std::string> src) {
	m_src.insert(m_src.end(), src.begin(), src.end());
	return *this;
}

CBuild& CBuild::inc_paths(std::vector<std::string> inc_paths) {
	m_inc_paths.insert(m_inc_paths.end(), inc_paths.begin(), inc_paths.end());
	return *this;
}

CBuild& CBuild::lib_paths(std::vector<std::string> lib_paths) {
	m_lib_paths.insert(m_lib_paths.end(), lib_paths.begin(), lib_paths.end());
	return *this;
}

CBuild& CBuild::libs(std::vector<std::string> libs) {
	m_libs.insert(m_libs.end(), libs.begin(), libs.end());
	return *this;
}

CBuild& CBuild::pop_objs(std::vector<std::string> objs) {
	for(int i = 0; i < objs.size(); i++) {
		auto iter = std::find(m_objs.begin(), m_objs.end(), objs[i]);
		if(iter != m_objs.end()) {
			m_objs.erase(iter);
		}
	}
	return *this;
}

/*
CBuild& CBuild::generate_compile_cmds() {
	std::ofstream file("compile_commands.json");
	file << "[\n";

	for (auto cmd : m_cmp_cmds) {
		file << "\t{\n";
		file << "\t\t\"directory\": \"" + cmd.dir + "\",\n";
		file << "\t\t\"command\": \"" + cmd.cmd+ "\",\n";
		file << "\t\t\"file\": \"" + cmd.file+ "\"\n";
		file << "\t}";

		if (&cmd == &m_cmp_cmds.back()) {
			file << ",";
		}

		file << "\n";
	}

	file << "]\n";
	file.close();
	return *this;
}
*/

CBuild& CBuild::compile() {
	for (auto src : m_src) {
		if(!compile_single(src)) {
			LOG("Compilation failed at: %s", src.c_str());
			exit(1);
		}
	}
	return *this;
}

CBuild& CBuild::build() {
	compile();

	std::string flags = vec_join(m_flags);
	std::string lib_paths = vec_join(m_lib_paths, "-L");
	std::string libs = vec_join(m_libs, "-l");
	std::string objs = vec_join(m_objs);

	std::string cmd = m_cc + " "
		+ flags
		+ " -o " + m_out_dir + 
		(m_out_dir.length() ? "/" : "")
		+ m_out_file + " "
		+ objs + " "
		+ lib_paths + " "
		+ libs;
	LOG(cmd.c_str());

	int status = std::system(cmd.c_str());
	if (status) {
		LOG("Building failed");
		exit(1);
	}

	LOG("Sucessfully built: %s", m_out_file.c_str());

	return *this;
}

CBuild& CBuild::build_static_lib() {
	compile();

	std::string flags = vec_join(m_flags);
	std::string objs = vec_join(m_objs);
	std::string cmd = "ar rcs "
		+ m_out_dir
		+ (m_out_dir.length() ? "/" : "")
		+ m_out_file + " "
		+ objs;

	LOG(cmd.c_str());
	int status = std::system(cmd.c_str());
	if (status) {
		LOG("Building failed");
		exit(1);
	}

	LOG("Sucessfully built: %s", m_out_file.c_str());
	return *this;
}

CBuild& CBuild::run(char** argv) {
	std::string out = m_out_dir + (m_out_dir.length() ? "/" : "") + m_out_file;
	LOG("Running: %s", out.c_str());
	int status = execvp(out.c_str(), argv);
	std::cout << status << std::endl;
	return *this;
}

CBuild& CBuild::clean() {
	LOG("Cleaning...");
	for (auto obj : m_objs) {
		std::remove(obj.c_str());
	}
	return *this;
}

bool CBuild::compile_single(const std::string& src) {
	std::string flags = vec_join(m_flags);
	std::string inc_paths = vec_join(m_inc_paths, "-I");

	std::string obj = src;
	if (contains(obj, ".cpp"))
		replace(obj, ".cpp", ".o");
	else if (contains(obj, ".c"));
		replace(obj, ".c", ".o");

	m_objs.push_back(obj);

	//std::string dir = std::boost::filesystem::current_path().string();
	std::string cmd = m_cc + " " + flags + " " + inc_paths + " -o " + obj + " -c " + src;
	//std::string file = dir + "/" + src;
	//m_cmp_cmds.push_back({dir, cmd, file});

	LOG(cmd.c_str());
	int status = std::system(cmd.c_str());
	return !status;
}

#endif
