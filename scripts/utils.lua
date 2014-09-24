local M = {}

-- copy all key/value pairs from 'src' to 'dst'
function M.MergeTables(dst, src)
	for key, value in pairs(src) do
		dst[key] = value
	end
end

-- copy all key/value pairs keys of which are in 'list' from 'src' to 'dst'
function M.MergeTablesSelectively(dst, src, list)
	for _, key in ipairs(list) do
		dst[key] = src[key]
	end
end

-- read everything from a file
function M.ReadFileContents(file)
	local f, err = io.open(file, "r")
	if f == nil then
		error(err)
	end

	local data = f:read("*a")
	f:close()
	return data
end

-- write string to file
function M.WriteFileContents(file, contents)
	local f, err = io.open(file, "w")
	if f == nil then
		error(err)
	end

	f:write(contents)
	if contents:sub(-1) ~= string.byte('\n') then
		f:write("\n")
	end
	f:close()
end

local dir = require "ng.dir"

-- execute all *_test.lua scripts recursively from 'dir'
function M.RunTests(basedir)
	for _, file in ipairs(dir.Files(basedir, "*_test.lua")) do
		print("Testing: "..file)
		dofile(basedir.."/"..file)
	end
	for _, cdir in ipairs(dir.Directories(basedir, "*")) do
		M.RunTests(basedir.."/"..cdir)
	end
end

-- split string, using space as a separator
function M.StringFields(str)
	local out = {}
	for w in str:gmatch("%S+") do
		out[#out+1] = w
	end
	return out
end

local axes = {"x", "y", "z", "w"}

function M.Axis(n)
	return axes[n]
end

-- Iterate over all lines in `text`, use it like this (handles only
-- plain "\n", not "\r\n"):
--     for line_num, line in Lines(text) do
--         -- line_num - number of the line, starting from 1
--         -- line - contents of the line without "\n"
--     end
function M.Lines(text)
	local function next_line(state)
		local text, begin, line_n = state[1], state[2], state[3]
		if begin < 0 then
			return nil
		end
		state[3] = line_n + 1
		local b, e = text:find("\n", begin, true)
		if b then
			state[2] = e+1
			return line_n, text:sub(begin, e-1)
		else
			state[2] = -1
			return line_n, text:sub(begin)
		end
	end
	local state = {text, 1, 1}
	return next_line, state
end

-- Find line offset of the point p in text. Line offset != line number. It
-- starts from 0.
function M.LineOffset(text, p)
	p = p-1
	local nl = string.byte("\n")
	local line_count = 0
	while p > 0 do
		if string.byte(text, p) == nl then
			line_count = line_count + 1
		end
		p = p - 1
	end
	return line_count
end

local bit = require "bit"
local band, bor, bnot = bit.band, bit.bor, bit.bnot

-- tests if v has b bits set
function M.BitTest(v, b)
	return band(v, b) ~= 0
end

-- sets b bits in v and returns modified v
function M.BitSet(v, b)
	return bor(v, b)
end

-- clears b bits in v and returns modified v
function M.BitClear(v, b)
	return band(v, bnot(b))
end

-- bitwise ORs all arguments and returns the resulting value
function M.BitCompose(...)
	local t = {...}
	local out = 0
	for _, b in ipairs(t) do
		out = bor(out, b)
	end
	return out
end

return M
