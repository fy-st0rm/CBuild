#pragma once

#include <fstream>
#include <numeric>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <time.h>
#include <unistd.h>

#define LOG(...) \
	printf("[LOG]: ");\
	printf(__VA_ARGS__);\
	printf("\n")

typedef struct {
	std::string dir, cmd, file;
} CompileCommand;

class CBuild {
public:
	CBuild (std::string cc, char** argv)
		: m_cc(cc) {
		std::time_t bin_t = to_time_t(std::filesystem::last_write_time("cbuild"));
		std::time_t src_t = to_time_t(std::filesystem::last_write_time("cbuild.cpp"));

		if (src_t > bin_t) {
			LOG("Rebuilding build.cpp");
			std::string cmd = "g++ -o cbuild cbuild.cpp";
			LOG(cmd.c_str());
			int status = std::system(cmd.c_str());
			if (status) {
				LOG("Rebuilding failed.");
				exit(1);
			}
			execvp("./cbuild", argv);
		}
	}

	~CBuild() {
	}

	CBuild& out(std::string out_dir, std::string out_file) {
		if (out_dir.length()) {
			LOG("Generating dir: %s", out_dir.c_str());
			std::filesystem::create_directories(out_dir);
		}
		m_out_dir  = out_dir;
		m_out_file = out_file;
		return *this;
	}

	CBuild& flags(std::vector<std::string> flags) {
		m_flags = flags;
		return *this;
	}

	CBuild& src(std::vector<std::string> src) {
		m_src = src;
		return *this;
	}

	CBuild& inc_paths(std::vector<std::string> inc_paths) {
		m_inc_paths = inc_paths;
		return *this;
	}

	CBuild& lib_paths(std::vector<std::string> lib_paths) {
		m_lib_paths = lib_paths;
		return *this;
	}

	CBuild& libs(std::vector<std::string> libs) {
		m_libs = libs;
		return *this;
	}

	CBuild& generate_compile_cmds() {
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

	CBuild& build() {
		for (auto src : m_src) {
			if(!compile(src)) {
				LOG("Compilation failed at: %s", src.c_str());
				exit(1);
			}
		}

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

	CBuild& build_static_lib() {
		for (auto src : m_src) {
			if(!compile(src)) {
				LOG("Compilation failed at: %s", src.c_str());
				exit(1);
			}
		}

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

	CBuild& run(char** argv = NULL) {
		std::string out = m_out_dir + (m_out_dir.length() ? "/" : "") + m_out_file;
		LOG("Running: %s", out.c_str());
		int status = execvp(out.c_str(), argv);
		std::cout << status << std::endl;
		return *this;
	}

	CBuild& clean() {
		LOG("Cleaning..");
		for (auto obj : m_objs) {
			std::filesystem::remove(obj);
		}
		return *this;
	}

private:
	std::string vec_join(const std::vector<std::string>& vec, const std::string& prefix = "") {
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

	template <typename TP>
	std::time_t to_time_t(TP tp) {
		using namespace std::chrono;
		auto sctp = time_point_cast<system_clock::duration>(
			tp - TP::clock::now()
			+ system_clock::now()
		);
		return system_clock::to_time_t(sctp);
	}

	bool compile(const std::string& src) {
		std::string flags = vec_join(m_flags);
		std::string inc_paths = vec_join(m_inc_paths, "-I");

		std::string obj = src;
		replace(obj, "cpp", "o");
		m_objs.push_back(obj);

		std::string dir = std::filesystem::current_path().string();
		std::string cmd = m_cc + " " + flags + " " + inc_paths + " -o " + obj + " -c " + src;
		std::string file = dir + "/" + src;
		m_cmp_cmds.push_back({dir, cmd, file});

		LOG(cmd.c_str());
		int status = std::system(cmd.c_str());
		return !status;
	}

private:
	std::string m_cc, m_out_dir, m_out_file;
	std::vector<std::string> m_flags, m_src, m_objs, m_inc_paths, m_lib_paths, m_libs;
	std::vector<CompileCommand> m_cmp_cmds;
};
