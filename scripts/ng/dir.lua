local M = {}

local ffi = require "ffi"
local C = ffi.C

ffi.cdef [[
	typedef struct Directory Directory;

	Directory *NG_FS_OpenDirectory(const char *path);
	void NG_FS_CloseDirectory(Directory *dir);
	char *NG_FS_NextFile(Directory *dir);
	char *NG_FS_NextDirectory(Directory *dir);
	void NG_FreeString(char *s);
	bool NG_FS_FNMatch(const char *pat, const char *name);
]]

function M.Files(path, pattern)
	local out = {}
	local dir = C.NG_FS_OpenDirectory(path)
	if dir == nil then
		error("failed opening dir: "..path)
	end
	dir = ffi.gc(dir, function(d) C.NG_FS_CloseDirectory(d) end)

	local next = C.NG_FS_NextFile(dir)
	while next ~= nil do
		if C.NG_FS_FNMatch(pattern, next) then
			out[#out+1] = ffi.string(next)
		end
		C.NG_FreeString(next)
		next = C.NG_FS_NextFile(dir)
	end
	C.NG_FS_CloseDirectory(ffi.gc(dir, nil))

	table.sort(out)
	return out
end

function M.Directories(path, pattern)
	local out = {}
	local dir = C.NG_FS_OpenDirectory(path)
	if dir == nil then
		error("failed opening dir: "..path)
	end
	dir = ffi.gc(dir, function(d) C.NG_FS_CloseDirectory(d) end)

	local next = C.NG_FS_NextDirectory(dir)
	while next ~= nil do
		if C.NG_FS_FNMatch(pattern, next) then
			out[#out+1] = ffi.string(next)
		end
		C.NG_FreeString(next)
		next = C.NG_FS_NextDirectory(dir)
	end
	C.NG_FS_CloseDirectory(ffi.gc(dir, nil))

	table.sort(out)
	return out
end

return M
