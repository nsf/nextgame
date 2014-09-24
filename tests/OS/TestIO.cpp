#include "stf.h"
#include "Core/Vector.h"
#include "OS/IO.h"

STF_SUITE_NAME("OS.IO")

STF_TEST("clean_path(String *s)") {
	struct Pair {
		String in;
		String out;
	};
	Vector<Pair> test_cases = {
		{"/tmp//x/y/z///g/", "/tmp/x/y/z/g"},
		{"//123/t/7", "/123/t/7"},
		{"./test/./././x/././", "test/x"},

		// Already clean
		{"", "."},
		{"abc", "abc"},
		{"abc/def", "abc/def"},
		{"a/b/c", "a/b/c"},
		{".", "."},
		{"..", ".."},
		{"../..", "../.."},
		{"../../abc", "../../abc"},
		{"/abc", "/abc"},
		{"/", "/"},

		// Remove trailing slash
		{"abc/", "abc"},
		{"abc/def/", "abc/def"},
		{"a/b/c/", "a/b/c"},
		{"./", "."},
		{"../", ".."},
		{"../../", "../.."},
		{"/abc/", "/abc"},

		// Remove doubled slash
		{"abc//def//ghi", "abc/def/ghi"},
		{"//abc", "/abc"},
		{"///abc", "/abc"},
		{"//abc//", "/abc"},
		{"abc//", "abc"},

		// Remove . elements
		{"abc/./def", "abc/def"},
		{"/./abc/def", "/abc/def"},
		{"abc/.", "abc"},

		// Remove .. elements
		{"abc/def/ghi/../jkl", "abc/def/jkl"},
		{"abc/def/../ghi/../jkl", "abc/jkl"},
		{"abc/def/..", "abc"},
		{"abc/def/../..", "."},
		{"/abc/def/../..", "/"},
		{"abc/def/../../..", ".."},
		{"/abc/def/../../..", "/"},
		{"abc/def/../../../ghi/jkl/../../../mno", "../../mno"},

		// Combinations
		{"abc/./../def", "def"},
		{"abc//./../def", "def"},
		{"abc/../../././../def", "../../def"},
	};
	for (auto &tc : test_cases) {
		String out = IO::clean_path(tc.in);
		STF_ASSERT(out == tc.out);
		if (out != tc.out)
			STF_PRINTF("[[%s]] vs. [[%s]]", out.c_str(), tc.out.c_str());
	}
}
