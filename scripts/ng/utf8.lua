local M = {}

local ffi = require "ffi"
local C = ffi.C

ffi.cdef [[
	int NG_UTF8_RuneCount(const char *str, int from, int to);
	int NG_UTF8_EncodeRune(char *out, int rune);
	int NG_UTF8_DecodeRune(const char *str, int from, int *size);
	int NG_UTF8_DecodeLastRune(const char *str, int to, int *size);
]]

M.RUNE_ERROR = 0xFFFD

function M.RuneCount(str, from, to)
	if from == nil then
		from = 1
	end
	if to == nil then
		to = #str
	end
	return C.NG_UTF8_RuneCount(str, from, to)
end

function M.EncodeRune(rune)
	local tmp = ffi.new "char[4]"
	local len = C.NG_UTF8_EncodeRune(tmp, rune)
	return ffi.string(tmp, len)
end

function M.DecodeRune(str, from)
	if not from then
		from = 1
	end
	local size = ffi.new "int[1]"
	local rune = C.NG_UTF8_DecodeRune(str, from, size)
	return rune, size[0]
end

function M.DecodeLastRune(str, to)
	if not to then
		to = #str
	end
	local size = ffi.new "int[1]"
	local rune = C.NG_UTF8_DecodeLastRune(str, to, size)
	return rune, size[0]
end

return M
