if global.DEBUG and global.MODE == "game" then
	print("Booting nextgame..")
	print("CONFIG_DIR  : "..global.CONFIG_DIR)
	print("BASE_DIR    : "..global.BASE_DIR)
	print("SCRIPTS_DIR : "..global.SCRIPTS_DIR)
	print("MODS_DIR    : "..global.MODS_DIR)
end

---------------------------------------------------------------------------------
-- Bootstrap package loaders
---------------------------------------------------------------------------------

-- common packages loader
local function LoadCommonPackages(name)
	name = string.gsub(name, "%.", "/")
	local path = global.SCRIPTS_DIR.."/"..name..".lua"
	local code, err = loadfile(path)
	if err ~= nil then
		print(err)
		return nil, err
	end
	return code
end

-- mod specific package loader
-- TODO

package.loaders = {
	package.loaders[1],
	LoadCommonPackages,
}

---------------------------------------------------------------------------------
-- Boot the globals
---------------------------------------------------------------------------------

if global.MODE == "game" then
	local utils = require "utils"
	local game = require "ng.game"
	local env = require "ng.env"
	local materials = require "materials"

	-- merge C helpers into the "global" global table
	utils.MergeTables(global, require "c_helpers")

	-- load the environment
	env.Load()
elseif global.MODE == "worker" then
	-- right now only procedural imaging workers are available
	local procimage = require "procimage"
	procimage.RegisterCallback()
end
