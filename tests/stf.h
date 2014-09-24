#pragma once

// simple testing framework
// TODO: manual

#include <vector>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <chrono>

#define _CC(a, b) a ## b
#define _MCC(a, b) _CC(a, b)

#define STF_TEST(name)                                                                   \
static void _MCC(_test_func_, __LINE__)(stf::test&);                                     \
static stf::test _MCC(_test_, __LINE__)(_stf_runner, name, _MCC(_test_func_, __LINE__)); \
static void _MCC(_test_func_, __LINE__)(stf::test &__T)

#define STF_FUNC(name, ...)	\
static void name(stf::test &__T, __VA_ARGS__)

#define STF_CALL(name, ...)	\
name(__T, __VA_ARGS__)

#define STF_SUITE_NAME(name) \
static stf::name_setter _MCC(_name_setter_, __LINE__)(_stf_runner, name);

#define STF_PRINTF(...) __T.printf(__LINE__, __FILE__, __VA_ARGS__)
#define STF_RAW_PRINTF(...) __T.printf(0, nullptr, __VA_ARGS__)
#define STF_ERRORF(...)							 \
do {											 \
	__T.printf(__LINE__, __FILE__, __VA_ARGS__); \
	__T.status = false;							 \
} while (0)

#define STF_ASSERT(expr)                                               \
do {                                                                   \
	if (!(expr)) {                                                     \
		__T.printf(__LINE__, __FILE__, "assertion failed: %s", #expr); \
		__T.status = false;                                            \
	}                                                                  \
} while (0)

namespace stf {

struct runner;
struct test;

typedef void (*functype)(test&);

struct test {
	std::string name = "<unnamed>";
	std::vector<std::string> messages;
	functype func;
	bool status = true;

	test(runner &r, std::string name, functype);

	void printf(int line, const char *filename, const char *format, ...) {
		char fileline[256];
		char message[8192];
		if (filename != nullptr) {
			snprintf(fileline, sizeof(fileline), "%s:%d: ", filename, line);
		}

		va_list vl;
		va_start(vl, format);
		vsnprintf(message, sizeof(message), format, vl);
		va_end(vl);

		// just in case
		fileline[sizeof(fileline)-1] = '\0';
		message[sizeof(message)-1] = '\0';

		messages.push_back(std::string(fileline) + message);
	}
};

struct name_setter {
	name_setter(runner &r, std::string name);
};

struct runner {
	std::string suite_name;
	std::vector<test> tests;
	int failed = 0;
	bool verbose = false;

	void init(int argc, char **argv) {
		for (int i = 0; i < argc; i++) {
			if (strcmp(argv[i], "-v") == 0) {
				verbose = true;
			}
		}
	}

	void logf_force(const char *format, ...) {
		va_list vl;
		va_start(vl, format);
		vfprintf(stderr, format, vl);
		va_end(vl);
	}

	void logf(const char *format, ...) {
		if (!verbose) {
			return;
		}
		va_list vl;
		va_start(vl, format);
		vfprintf(stderr, format, vl);
		va_end(vl);
	}

	int run() {
		auto start = std::chrono::system_clock::now();
		for (auto &t: tests) {
			logf("%s... ", t.name.c_str());
			(*t.func)(t);
			if (t.status) {
				logf("PASS\n");
			} else {
				failed++;
				if (verbose) {
					logf("FAIL\n");
				} else {
					logf_force("%s... FAIL\n", t.name.c_str());
				}

				for (auto const &msg: t.messages) {
					logf_force("  %s\n", msg.c_str());
				}
			}
		}
		auto end = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
			(end-start).count();
		auto s_part = ms / 1000;
		auto ms_part = ms - s_part * 1000;
		if (failed > 0) {
			logf("%d out of %d test(s) failed", failed, tests.size());
		}

		if (failed > 0) {
			logf_force("FAIL\t%s\t%d.%03ds\n", suite_name.c_str(), s_part, ms_part);
			return 1;
		} else {
			logf_force("ok\t%s\t%d.%03ds\n", suite_name.c_str(), s_part, ms_part);
			return 0;
		}
	}
};

test::test(runner &r, std::string name, functype func): name(name), func(func) {
	r.tests.push_back(*this);
}

name_setter::name_setter(runner &r, std::string name) {
	r.suite_name = name;
}

} // namespace stf

static stf::runner _stf_runner;
int main(int argc, char **argv) {
	_stf_runner.init(argc, argv);
	return _stf_runner.run();
}
