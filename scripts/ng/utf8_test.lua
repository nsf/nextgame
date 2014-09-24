local utf8 = require "ng.utf8"

local s = "привет"
assert(utf8.RuneCount(s) == 6)

local rune, size

rune, size = utf8.DecodeRune(s)
assert(rune == 1087)
assert(size == 2)

rune, size = utf8.DecodeLastRune(s)
assert(rune == 1090)
assert(size == 2)

rune, size = utf8.DecodeRune(s, 3)
assert(rune == 1088)
assert(size == 2)

rune, size = utf8.DecodeLastRune(s, #s - 2)
assert(rune == 1077)
assert(size == 2)
