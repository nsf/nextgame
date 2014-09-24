local utils = require "utils"
local preproc = require "preproc"
local dir = require "ng.dir"

local M = {}

local function LoadInclude(filename)
	if not filename:match(".+%.glsl") then
		filename = filename..".glsl"
	end
	local path = global.BASE_DIR.."/shaders/"..filename
	return utils.ReadFileContents(path)
end

function M.LoadShader(filename)
	local data

	if filename == "UNIFORM_BLOCKS" then
		-- special procedural shader which loads all ub_*.glsl files for
		-- introspection in the C++ code
		data = "[VS]\n"
		for _, f in ipairs(dir.Files(global.BASE_DIR.."/shaders", "ub_*.glsl")) do
			data = data..'#include "'..f..'"\n'
		end
	else
		-- if the filename doesn't end with ".glsl", add that suffix
		if not filename:match(".+%.glsl") then
			filename = filename..".glsl"
		end
		local path = global.BASE_DIR.."/shaders/"..filename
		data = utils.ReadFileContents(path)
	end

	local sections = {}
	local prev_section = ""
	local prev_begin = 1
	while true do
		local b, e, n = data:find("%[(%u+)%]", prev_begin)
		if prev_section ~= "" then
			local e = b and b-1 or #data
			print("loading "..filename.."["..prev_section.."]")
			local text, files = preproc.Preprocess(data:sub(prev_begin, e), {
				line_offset = utils.LineOffset(data, prev_begin),
				load_include = LoadInclude,
				context = global.ShaderEnv,
			})
			sections[#sections+1] = {
				prev_section,
				text,
				filename,
				files,
			}
		end
		if b == nil then
			break
		else
			prev_section = n
			prev_begin = e+1
		end
	end

	return sections
end

return M
