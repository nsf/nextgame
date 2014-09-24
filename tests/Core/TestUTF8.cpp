#include "stf.h"
#include "Core/String.h"
#include "Core/Vector.h"
#include "Core/UTF8.h"
#include "Core/Defer.h"

struct utf8map_t {
	int r;
	String str;
};

utf8map_t utf8map[] = {
		{0x0000, Slice<const char>("\0", 1)},
		{0x0001, "\x01"},
		{0x007e, "\x7e"},
		{0x007f, "\x7f"},
		{0x0080, "\xc2\x80"},
		{0x0081, "\xc2\x81"},
		{0x00bf, "\xc2\xbf"},
		{0x00c0, "\xc3\x80"},
		{0x00c1, "\xc3\x81"},
		{0x00c8, "\xc3\x88"},
		{0x00d0, "\xc3\x90"},
		{0x00e0, "\xc3\xa0"},
		{0x00f0, "\xc3\xb0"},
		{0x00f8, "\xc3\xb8"},
		{0x00ff, "\xc3\xbf"},
		{0x0100, "\xc4\x80"},
		{0x07ff, "\xdf\xbf"},
		{0x0800, "\xe0\xa0\x80"},
		{0x0801, "\xe0\xa0\x81"},
		{0xd7ff, "\xed\x9f\xbf"}, // last code point before surrogate half.
		{0xe000, "\xee\x80\x80"}, // first code point after surrogate half.
		{0xfffe, "\xef\xbf\xbe"},
		{0xffff, "\xef\xbf\xbf"},
		{0x10000, "\xf0\x90\x80\x80"},
		{0x10001, "\xf0\x90\x80\x81"},
		{0x10fffe, "\xf4\x8f\xbf\xbe"},
		{0x10ffff, "\xf4\x8f\xbf\xbf"},
		{0xFFFD, "\xef\xbf\xbd"},
};

utf8map_t surrogate_map[] = {
	{0xd800, "\xed\xa0\x80"}, // surrogate min decodes to (RuneError, 1)
	{0xdfff, "\xed\xbf\xbf"}, // surrogate max decodes to (RuneError, 1)
};

Slice<const char> test_strings[] = {
	"",
	"abcd",
	"☺☻☹",
	"日a本b語ç日ð本Ê語þ日¥本¼語i日©",
	"日a本b語ç日ð本Ê語þ日¥本¼語i日©日a本b語ç日ð本Ê語þ日¥本¼語i日©日a本b語ç日ð本Ê語þ日¥本¼語i日©",
	"\x80\x80\x80\x80",
};

struct rune_count_test {
	String in;
	int out;
};

rune_count_test rune_count_tests[] = {
	{"abcd", 4},
	{"☺☻☹", 3},
	{"1,2,3,4", 7},
	{Slice<const char>("\xe2\x00", 2), 2},
};

struct rune_len_test {
	int r;
	int size;
};

rune_len_test rune_len_tests[] = {
	{0, 1},
	{U'e', 1},
	{U'é', 2},
	{U'☺', 3},
	{UTF8::RUNE_ERROR, 3},
	{UTF8::RUNE_MAX, 4},
	{0xD800, -1},
	{0xDFFF, -1},
	{UTF8::RUNE_MAX + 1, -1},
	{-1, -1},
};

struct valid_test {
	Slice<const char> in;
	bool out;
};

const char invalid_str1[] = {66, char(250)};
const char invalid_str2[] = {66, char(250), 67};

valid_test valid_tests[] = {
	{"", true},
	{"a", true},
	{"abc", true},
	{"Ж", true},
	{"ЖЖ", true},
	{"брэд-ЛГТМ", true},
	{"☺☻☹", true},
	{Slice<const char>(invalid_str1, 2), false},
	{Slice<const char>(invalid_str2, 3), false},
	{"a\uFFFDb", true},
	{"\xF4\x8F\xBF\xBF", true},      // U+10FFFF
	{"\xF4\x90\x80\x80", false},     // U+10FFFF+1; out of range
	{"\xF7\xBF\xBF\xBF", false},     // 0x1FFFFF; out of range
	{"\xFB\xBF\xBF\xBF\xBF", false}, // 0x3FFFFFF; out of range
	{"\xc0\x80", false},             // U+0000 encoded in two bytes: incorrect
	{"\xed\xa0\x80", false},         // U+D800 high surrogate (sic)
	{"\xed\xbf\xbf", false},         // U+DFFF low surrogate (sic)
};

struct valid_rune_test {
	int in;
	bool out;
};

valid_rune_test valid_rune_tests[] = {
	{0, true},
	{'e', true},
	{U'é', true},
	{U'☺', true},
	{UTF8::RUNE_ERROR, true},
	{UTF8::RUNE_MAX, true},
	{0xD7FF, true},
	{0xD800, false},
	{0xDFFF, false},
	{0xE000, true},
	{UTF8::RUNE_MAX + 1, false},
	{-1, false},
};

STF_TEST("utf8::full_rune(slice<const char>)") {
	for (const auto &m : utf8map) {
		STF_ASSERT(UTF8::full_rune(m.str));
		auto s = m.str.sub(0, m.str.length()-1);
		STF_ASSERT(!UTF8::full_rune(s));
	}
}

STF_TEST("utf8::encode_rune(slice<char>, rune)") {
	for (const auto &m : utf8map) {
		char buf[10];
		int n = UTF8::encode_rune(buf, m.r);
		STF_ASSERT(Slice<const char>(buf, n) == m.str);
	}

	// check that negative runes encode as rune_error
	char errbuf[UTF8::MAX_BYTES];
	int na = UTF8::encode_rune(errbuf, UTF8::RUNE_ERROR);
	char buf[UTF8::MAX_BYTES];
	int nb = UTF8::encode_rune(buf, -1);
	STF_ASSERT(Slice<const char>(errbuf, na) == Slice<const char>(buf, nb));
}

STF_TEST("utf8::decode_rune(slice<const char>, int*)") {
	for (const auto &m : utf8map) {
		String str = m.str;
		auto r = UTF8::decode_rune(str);
		STF_ASSERT(r.rune == m.r && r.size == str.length());

		// make sure trailing byte works
		str.append({"\0", 1});
		r = UTF8::decode_rune(str);
		STF_ASSERT(r.rune == m.r && r.size == str.length()-1);

		// remove trailing \0
		str.remove(str.length()-1);

		// make sure missing bytes fail
		int wantsize = 1;
		if (wantsize >= str.length()) {
			wantsize = 0;
		}

		r = UTF8::decode_rune(str.sub(0, str.length()-1));
		STF_ASSERT(r.rune == UTF8::RUNE_ERROR && r.size == wantsize);

		// make sure bad sequences fail
		if (str.length() == 1) {
			str[0] = 0x80;
		} else {
			str[str.length()-1] = 0x7F;
		}
		r = UTF8::decode_rune(str);
		STF_ASSERT(r.rune == UTF8::RUNE_ERROR && r.size == 1);
	}

	// surrogate runes
	for (const auto &m : surrogate_map) {
		auto r = UTF8::decode_rune(m.str);
		STF_ASSERT(r.rune == UTF8::RUNE_ERROR && r.size == 1);
	}
}

bool test_sequence(Slice<const char> s) {
	struct info {
		int index;
		int r;
	};
	Vector<info> index(s.length);
	int j = 0, i = 0;
	while (i < s.length) {
		auto r = UTF8::decode_rune(s.sub(i));
		index[j++] = {i, r.rune};
		i += r.size;
	}
	j--;
	i = s.length;
	while (i > 0) {
		auto r = UTF8::decode_last_rune(s.sub(0, i));
		if (index[j].r != r.rune) {
			return false;
		}
		i -= r.size;
		if (index[j].index != i) {
			return false;
		}
		j--;
	}
	return true;
}

STF_TEST("utf8::decode_last_rune(slice<const char>, int*)") {
	// We actually test here that `decode_last_rune` corresponds to
	// `decode_rune`, because `decode_rune` works according to test above.
	for (const auto &ts : test_strings) {
		for (const auto &m : utf8map) {
			String s1 = ts + m.str;
			String s2 = m.str + ts;
			String s3 = ts + m.str + ts;
			for (const auto &s : {s1, s2, s3}) {
				STF_ASSERT(test_sequence(s));
			}
		}
	}
}

STF_TEST("utf8::rune_count(slice<const char>)") {
	for (const auto &t : rune_count_tests) {
		STF_ASSERT(UTF8::rune_count(t.in) == t.out);
	}
}

STF_TEST("utf8::rune_len(rune)") {
	for (const auto &t : rune_len_tests) {
		STF_ASSERT(UTF8::rune_length(t.r) == t.size);
	}
}

STF_TEST("utf8::valid(slice<const char>)") {
	for (const auto &t : valid_tests) {
		STF_ASSERT(UTF8::valid(t.in) == t.out);
	}
}

STF_TEST("utf8::valid_rune(rune)") {
	for (const auto &t : valid_rune_tests) {
		STF_ASSERT(UTF8::valid_rune(t.in) == t.out);
	}
}

STF_TEST("utf8::rune_start(char)") {
	for (const auto &m : utf8map) {
		char first = m.str[0];
		STF_ASSERT(UTF8::rune_start(first));
		if (m.str.length() > 1) {
			char second = m.str[1];
			STF_ASSERT(!UTF8::rune_start(second));
		}
	}
}
