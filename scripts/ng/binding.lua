local M = {}

local ffi = require "ffi"
local C = ffi.C

function M.DefineMethods(mt, methods)
	ffi.cdef(methods)
	for cname, method in methods:gmatch("(NG_%a+_(%a+))") do
		mt[method] = C[cname]
	end
end

return M
