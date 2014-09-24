#pragma once

#include "Core/Slice.h"

namespace UTF8 {

struct SizedRune {
	int rune;
	int size;
};

const int MAX_BYTES = 4;         // maximum bytes per rune
const int RUNE_SELF = 0x80;    // rune and utf are equal (<)
const int RUNE_MAX = 0x10FFFF; // maximum rune value
const int RUNE_ERROR = 0xFFFD;

bool full_rune(Slice<const char> s);
SizedRune decode_rune(Slice<const char> s);
SizedRune decode_last_rune(Slice<const char> s);
int encode_rune(Slice<char> s, int r);
int rune_count(Slice<const char> s);
int rune_length(int r);
bool rune_start(char b);
bool valid(Slice<const char> s);
bool valid_rune(int r);

} // namespace UTF8
