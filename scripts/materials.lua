local M = {}

local utils = require "utils"
local serpent = require "lib.serpent"
local dir = require "ng.dir"

local function PreprocessMaterialFace(face)
	face.textures = {}

	-- default base color is {1, 1, 1}
	if not face.base_color then
		face.base_color = {1, 1, 1}
	end

	-- if base color is a string, then it points at the texture
	if type(face.base_color) == "string" then
		face.textures.base_color = face.base_color
		face.base_color = {1, 1, 1}
	end

	-- default scale is 0.25
	if not face.scale then
		face.scale = 0.25
	end

	if not face.bump_scale then
		face.bump_scale = 1.0
	end

	-- normal texture must be provided
	--[[
	if not face.normal then
		error("normal texture must be provided")
	end
	assert(type(face.normal) == "string")]]
	if not face.normal then
		face.bump_scale = 0.0
	end
	assert(face.normal == nil or type(face.normal) == "string")
	face.textures.normal = face.normal
	face.normal = nil

	-- default metallic is 0.0
	if not face.metallic then
		face.metallic = 0.0
	end

	-- metallicness texture is provided
	if type(face.metallic) == "string" then
		face.textures.metallic = face.metallic
		face.metallic = 1.0
	end

	-- default roughness is 1.0
	if not face.roughness then
		face.roughness = 1.0
	end

	-- roughness texture is provided
	if type(face.roughness) == "string" then
		face.textures.roughness = face.roughness
		face.roughness = 1.0
	end
end

-- we add the texture to the ts[ts_name] dict here and replace mt[mt_name] with
-- its number
local function CollectTexture(ts, ts_name, mt, mt_name)
	local textable = ts[ts_name]
	local texpath = mt[mt_name]
	if texpath then
		local n = textable[texpath]
		if n then
			mt[mt_name] = n
			return
		end
		n = textable.n+1
		textable.n = n
		textable[texpath] = n
		mt[mt_name] = n
	else
		mt[mt_name] = 1
	end
end

local function CollectFaceTextures(ts, face)
	local mt = face.textures
	CollectTexture(ts, "bc1_textures", mt, "base_color")
	CollectTexture(ts, "bc5_textures", mt, "normal")
	CollectTexture(ts, "bc4_textures", mt, "metallic")
	CollectTexture(ts, "bc4_textures", mt, "roughness")
end

local function CollectTextures(matpack)
	local texture_tables = {
		bc1_textures = {["textures/white_bc1.dds"] = 1, n=1},
		bc4_textures = {["textures/white_bc4.dds"] = 1, n=1},
		bc5_textures = {["textures/white_bc5.dds"] = 1, n=1},
	}
	for _, mat in ipairs(matpack) do
		CollectFaceTextures(texture_tables, mat.top)
		if not mat.side then
			mat.side = mat.top
		else
			CollectFaceTextures(texture_tables, mat.side)
		end
	end
	for _, d in pairs(texture_tables) do
		d.n = nil
	end

	-- convert texture tables to a texture array
	local textures = {
		bc1_textures = {},
		bc4_textures = {},
		bc5_textures = {},
	}
	for group, d in pairs(texture_tables) do
		for texname, n in pairs(d) do
			textures[group][n] = texname
		end
	end
	return textures
end

function M.LoadMaterialPack(filename)
	local ok, matpack_data = pcall(utils.ReadFileContents, filename)
	if not ok then
		return
	end
	local ok, matpack = serpent.load(matpack_data)
	if not ok then matpack = {} end
	if not matpack.name then
		error("material pack ("..filename..") doesn't have a name")
	end
	print("loading material pack: "..matpack.name)

	for _, mat in ipairs(matpack) do
		if not mat.name then mat.name = "Unknown" end
		if not mat.top then mat.top = {} end
		PreprocessMaterialFace(mat.top)
		if mat.side then
			PreprocessMaterialFace(mat.side)
		end
	end
	matpack.textures = CollectTextures(matpack)
	global.AddMaterialPack(matpack)
end

function M.LoadAllMaterialPacks()
	for _, v in ipairs(dir.Files(global.BASE_DIR.."/materials", "*.lua")) do
		local fullpath = global.BASE_DIR.."/materials/"..v
		M.LoadMaterialPack(fullpath)
	end
end

return M
