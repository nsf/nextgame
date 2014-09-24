#include "Core/UTF8.h"
#include <cstdint>

enum {
	T1 = 0x00, // 0000 0000
	TX = 0x80, // 1000 0000
	T2 = 0xC0, // 1100 0000
	T3 = 0xE0, // 1110 0000
	T4 = 0xF0, // 1111 0000
	T5 = 0xF8, // 1111 1000

	MASKX = 0x3F, // 0011 1111
	MASK2 = 0x1F, // 0001 1111
	MASK3 = 0x0F, // 0000 1111
	MASK4 = 0x07, // 0000 0111

	RUNE1MAX = (1 << 7) - 1,
	RUNE2MAX = (1 << 11) - 1,
	RUNE3MAX = (1 << 16) - 1,

	// code points in the surrogate range are not valid for UTF-8
	SURROGATE_MIN = 0xD800,
	SURROGATE_MAX = 0xDFFF,
};

namespace UTF8 {

struct DecodedRune {
	int rune;
	int size;
	bool incomplete;
};

static DecodedRune DecodeRuneInternal(Slice<const char> s)
{
	int n = s.length;
	if (n < 1) {
		return {RUNE_ERROR, 0, true};
	}
	uint8_t c0 = s[0];

	// 1-byte, 7-bit sequence?
	if (c0 < TX) {
		return {c0, 1, false};
	}

	// unexpected continuation byte?
	if (c0 < T2) {
		return {RUNE_ERROR, 1, false};
	}

	// need first continuation byte
	if (n < 2) {
		return {RUNE_ERROR, 1, true};
	}

	uint8_t c1 = s[1];
	if (c1 < TX || T2 <= c1) {
		return {RUNE_ERROR, 1, false};
	}

	// 2-byte, 11-bit sequence?
	if (c0 < T3) {
		int r = int(c0 & MASK2) << 6 | int(c1 & MASKX);
		if (r <= RUNE1MAX) {
			return {RUNE_ERROR, 1, false};
		}
		return {r, 2, false};
	}

	// need second continuation byte
	if (n < 3) {
		return {RUNE_ERROR, 1, true};
	}

	uint8_t c2 = s[2];
	if (c2 < TX || T2 <= c2) {
		return {RUNE_ERROR, 1, false};
	}

	// 3-byte, 16-bit sequence?
	if (c0 < T4) {
		int r = int(c0 & MASK3) << 12 |
			int(c1 & MASKX) << 6 |
			int(c2 & MASKX);
		if (r <= RUNE2MAX) {
			return {RUNE_ERROR, 1, false};
		}
		if (SURROGATE_MIN <= r && r <= SURROGATE_MAX) {
			return {RUNE_ERROR, 1, false};
		}
		return {r, 3, false};
	}

	// need third continuation byte
	if (n < 4) {
		return {RUNE_ERROR, 1, true};
	}

	uint8_t c3 = s[3];
	if (c3 < TX || T2 <= c3) {
		return {RUNE_ERROR, 1, false};
	}

	// 4-byte, 21-bit sequence?
	if (c0 < T5) {
		int r = int(c0 & MASK4) << 18 |
			int(c1 & MASKX) << 12 |
			int(c2 & MASKX) << 6 |
			int(c3 & MASKX);
		if (r <= RUNE3MAX || RUNE_MAX < r) {
			return {RUNE_ERROR, 1, false};
		}
		return {r, 4, false};
	}

	// error
	return {RUNE_ERROR, 1, false};
}

bool full_rune(Slice<const char> s)
{
	return !DecodeRuneInternal(s).incomplete;
}

SizedRune decode_rune(Slice<const char> s)
{
	auto r = DecodeRuneInternal(s);
	return {r.rune, r.size};
}

SizedRune decode_last_rune(Slice<const char> s)
{
	int end = s.length;
	if (end == 0) {
		return {RUNE_ERROR, 0};
	}
	int start = end - 1;
	int r = uint8_t(s[start]);
	if (r < RUNE_SELF) {
		return {r, 1};
	}

	// guard against O(n^2) behavior when traversing
	// backwards through strings with long sequences of
	// invalid UTF-8.
	int lim = end - MAX_BYTES;
	if (lim < 0) {
		lim = 0;
	}

	for (start--; start >= lim; start--) {
		if (rune_start(s[start])) {
			break;
		}
	}

	if (start < 0) {
		start = 0;
	}

	auto dr = DecodeRuneInternal(s.sub(start));
	if (start + dr.size != end) {
		return {RUNE_ERROR, 1};
	}
	return {dr.rune, dr.size};
}

int encode_rune(Slice<char> s, int r)
{
	unsigned ur = r;
	if (ur <= RUNE1MAX) {
		s[0] = char(r);
		return 1;
	}

	if (ur <= RUNE2MAX) {
		s[0] = T2 | char(r >> 6);
		s[1] = TX | (char(r) & MASKX);
		return 2;
	}

	if (ur > unsigned(RUNE_MAX)) {
		r = RUNE_ERROR;
	}

	if (SURROGATE_MIN <= r && r <= SURROGATE_MAX) {
		r = RUNE_ERROR;
	}
	ur = r;

	if (ur <= RUNE3MAX) {
		s[0] = T3 | char(r >> 12);
		s[1] = TX | (char(r >> 6) & MASKX);
		s[2] = TX | (char(r) & MASKX);
		return 3;
	}

	s[0] = T4 | char(r >> 18);
	s[1] = TX | (char(r >> 12) & MASKX);
	s[2] = TX | (char(r >> 6) & MASKX);
	s[3] = TX | (char(r) & MASKX);
	return 4;
}

int rune_count(Slice<const char> s)
{
	int n = 0, i = 0;
	while (i < s.length) {
		if (unsigned(s[i]) < RUNE_SELF) {
			i++;
		} else {
			i += decode_rune(s.sub(i)).size;
		}
		n++;
	}
	return n;
}

int rune_length(int r)
{
	if (r < 0) {
		return -1;
	} else if (r <= RUNE1MAX) {
		return 1;
	} else if (r <= RUNE2MAX) {
		return 2;
	} else if (SURROGATE_MIN <= r && r <= SURROGATE_MAX) {
		return -1;
	} else if (r <= RUNE3MAX) {
		return 3;
	} else if (r <= RUNE_MAX) {
		return 4;
	}
	return -1;

}

bool rune_start(char b)
{
	return (b & 0xC0) != 0x80;
}

bool valid(Slice<const char> s)
{
	int i = 0;
	while (i < s.length) {
		if (unsigned(s[i]) < RUNE_SELF) {
			i++;
		} else {
			int size = decode_rune(s.sub(i)).size;
			if (size == 1) {
				// All valid runes of size 1 (those
				// below rune_self) were handled above.
				// This must be a rune_error.
				return false;
			}
			i += size;
		}
	}
	return true;
}

bool valid_rune(int r)
{
	if (r < 0) {
		return false;
	} else if (SURROGATE_MIN <= r && r <= SURROGATE_MAX) {
		return false;
	} else if (r > RUNE_MAX) {
		return false;
	}
	return true;
}

} // namespace UTF8

NG_LUA_API int NG_UTF8_RuneCount(const char *str, int from, int to)
{
	return UTF8::rune_count(Slice<const char>(str).sub(from-1, to));
}

NG_LUA_API int NG_UTF8_EncodeRune(char *out, int rune)
{
	return UTF8::encode_rune(Slice<char>(out, 4), rune);
}

NG_LUA_API int NG_UTF8_DecodeRune(const char *str, int from, int *size)
{
	auto s = Slice<const char>(str);
	if (from-1 < 0 || from-1 >= s.length) {
		*size = 1;
		return UTF8::RUNE_ERROR;
	}

	auto sr = UTF8::decode_rune(s.sub(from-1));
	*size = sr.size;
	return sr.rune;
}

NG_LUA_API int NG_UTF8_DecodeLastRune(const char *str, int to, int *size)
{
	auto s = Slice<const char>(str);
	if (to <= 0 || to > s.length) {
		*size = 1;
		return UTF8::RUNE_ERROR;
	}

	auto sr = UTF8::decode_last_rune(s.sub(0, to));
	*size = sr.size;
	return sr.rune;
}
