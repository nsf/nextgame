require "ng.struct"

local utils = require "utils"
local serpent = require "lib.serpent"
local class = require "lib.30log"
local ffi = require "ffi"
local C = ffi.C

local BitTest = utils.BitTest

local function CompareVarNames(a, b)
	local n1 = a.nice_name or a.name
	local n2 = b.nice_name or b.name
	return n1 < n2
end

local function StandardGetter(type)
	local cfunc = "void NG_EnvVar_"..type.."_Get(void *addr, "..type.." *out);"
	ffi.cdef(cfunc)
	return C["NG_EnvVar_"..type.."_Get"]
end

local function StandardSetter(type, args)
	args = args or {}
	local cfunc = "void NG_EnvVar_"..type.."_Set(void *addr, "
	if args.use_ref then
		cfunc = cfunc.."const "..type.." &value);"
	else
		cfunc = cfunc..type.." value);"
	end
	ffi.cdef(cfunc)
	return C["NG_EnvVar_"..type.."_Set"]
end

local var_flags = {
	PERSISTENT = 1,
	GUI = 2,
}

local function CreateLabelForVar(gui, var)
	local LABEL = var.name.."_label"
	gui:Label(LABEL, {
		text = (var.nice_name or var.name)..":"
	})
end

local var_gui_creators = {
	number = function(var, gui)
		local ns = var.name
		local LABEL = ns.."_label"
		local SPINBOX = ns.."_spinbox"
		CreateLabelForVar(gui, var)
		local min = var.gui_attrs.min or 0
		local max = var.gui_attrs.max or 1
		local increment = var.gui_attrs.increment or 0.01
		local format = var.gui_attrs.format or "%.1f"
		gui:SpinBox(SPINBOX, {
			value = var:Get(),
			min = min,
			max = max,
			increment = increment,
			format = format,
			on_change = function(v)
				var:Set(v)
			end,
		})

		var.update_gui = function()
			gui:Get(SPINBOX):Configure{ value = var:Get() }
			gui:RedrawMaybe()
		end

		return LABEL.." "..SPINBOX.."\n", {
			[LABEL] = { sticky = "NE", padx = {0, 1} },
			[SPINBOX] = { sticky = "NWSE" },
		}
	end,
	ratio = function(var, gui)
		local consts = require "gui.consts"
		local ns = var.name
		local LABEL = ns.."_label"
		local SLIDER = ns.."_slider"
		CreateLabelForVar(gui, var)
		gui:Slider(SLIDER, {
			value = var:Get(),
			orientation = consts.Orientation.HORIZONTAL,
			on_change = function(v)
				var:Set(v)
			end,
		})

		var.update_gui = function()
			gui:Get(SLIDER):Configure{ value = var:Get() }
			gui:RedrawMaybe()
		end

		return LABEL.." "..SLIDER.."\n", {
			[LABEL] = { sticky = "NE", padx = {0, 1} },
			[SLIDER] = { sticky = "NWSE" },
		}
	end,
	bool = function(var, gui)
		local ns = var.name
		local LABEL = ns.."_label"
		local CHECKBUTTON = ns.."_checkbutton"
		CreateLabelForVar(gui, var)
		gui:CheckButton(CHECKBUTTON, {
			checked = var:Get(),
			on_change = function(v)
				var:Set(v)
			end,
		})

		var.update_gui = function()
			gui:Get(CHECKBUTTON):Configure{ checked = var:Get() }
			gui:RedrawMaybe()
		end

		return LABEL.." "..CHECKBUTTON.."\n", {
			[LABEL] = { sticky = "NE", padx = {0, 1} },
			[CHECKBUTTON] = { sticky = "NWSE" },
		}
	end,
	color = function(var, gui)
		local meta = require "gui.meta"
		local ns = var.name
		local LABEL = ns.."_label"
		local COLORPICKER = ns.."_colorpicker"
		CreateLabelForVar(gui, var)
		meta.ColorPicker(gui, COLORPICKER, {
			on_apply = function(color)
				var:Set(color)
			end,
			color = var:Get(),
		})

		return LABEL.." "..COLORPICKER.."\n", {
			[LABEL] = { sticky = "NE", padx = {0, 1} },
			[COLORPICKER] = { sticky = "NWSE" },
		}
	end,
	choice = function(var, gui)
		local ns = var.name
		local LABEL = ns.."_label"
		local COMBOBOX = ns.."_combobox"
		CreateLabelForVar(gui, var)

		local list = var.gui_attrs.list or {"None"}
		local selected = var:Get()
		gui:ComboBox(COMBOBOX, {
			list = list,
			selected = selected,
			on_change = function(v)
				var:Set(v)
			end,
		})
		var.update_gui = function()
			gui:Get(COMBOBOX):Configure{ selected = var:Get() }
			gui:RedrawMaybe()
		end

		return LABEL.." "..COMBOBOX.."\n", {
			[LABEL] = { sticky = "NE", padx = {0, 1} },
			[COMBOBOX] = { sticky = "NWSE" },
		}
	end,
}

---------------------------------------------------------------------------------
-- AbstractVar
---------------------------------------------------------------------------------
local AbstractVar = class()

function AbstractVar:__init(args)
	self.name = args.name
	self.flags = args.flags or 0
	self.nice_name = args.nice_name
	self.gui_attrs = args.gui_attrs or {}
	self.desc = args.desc
	self.on_set = {}
	self.update_gui = nil
end

function AbstractVar:Serialize()
	return self:Get()
end

function AbstractVar:Deserialize(v)
	self:Set(v)
end

---------------------------------------------------------------------------------
-- LuaVar
---------------------------------------------------------------------------------
local LuaVar = AbstractVar:extends()

function LuaVar:__init(args)
	LuaVar.super.__init(self, args)
	self.value = args.value
end

function LuaVar:Set(value)
	self.value = value
	for _, cb in ipairs(self.on_set) do
		cb()
	end
end

function LuaVar:Get()
	return self.value
end

---------------------------------------------------------------------------------
-- AbstractCVar
---------------------------------------------------------------------------------
local AbstractCVar = AbstractVar:extends()

function AbstractCVar:__init(args)
	AbstractCVar.super.__init(self, args)
	self.addr = args.addr
end

function AbstractCVar:Set(value)
	self.csetter(self.addr, value)
	for _, cb in ipairs(self.on_set) do
		cb()
	end
end

function AbstractCVar:Get()
	local value = ffi.new(self.ctype.."[1]")
	self.cgetter(self.addr, value)
	return value[0]
end

---------------------------------------------------------------------------------
-- AbstractStructVar
---------------------------------------------------------------------------------
local AbstractCStructVar = AbstractCVar:extends()

-- structs require a custom getter in order to avoid dangling pointers
function AbstractCStructVar:Get()
	local value = ffi.new(self.ctype.."[1]")
	self.cgetter(self.addr, value)
	return ffi.new(self.ctype, value[0])
end

---------------------------------------------------------------------------------
-- BoolVar
---------------------------------------------------------------------------------
local BoolVar = AbstractCVar:extends {
	ctype = "bool",
	cgetter = StandardGetter("bool"),
	csetter = StandardSetter("bool"),
}

function BoolVar:Serialize()
	return not not self:Get()
end

function BoolVar:Deserialize(v)
	self:Set(v)
end

---------------------------------------------------------------------------------
-- NumberVar
---------------------------------------------------------------------------------
local NumberVar = AbstractCVar:extends()

function NumberVar:Serialize()
	return tonumber(self:Get())
end

function NumberVar:Deserialize(v)
	self:Set(v)
end

---------------------------------------------------------------------------------
-- IntVar
---------------------------------------------------------------------------------
local IntVar = NumberVar:extends {
	ctype = "int",
	cgetter = StandardGetter("int"),
	csetter = StandardSetter("int"),
}

---------------------------------------------------------------------------------
-- FloatVar
---------------------------------------------------------------------------------
local FloatVar = NumberVar:extends {
	ctype = "float",
	cgetter = StandardGetter("float"),
	csetter = StandardSetter("float"),
}

---------------------------------------------------------------------------------
-- DoubleVar
---------------------------------------------------------------------------------
local DoubleVar = NumberVar:extends {
	ctype = "double",
	cgetter = StandardGetter("double"),
	csetter = StandardSetter("double"),
}

---------------------------------------------------------------------------------
-- Vec3iVar
---------------------------------------------------------------------------------
local Vec3iVar = AbstractCStructVar:extends {
	ctype = "Vec3i",
	cgetter = StandardGetter("Vec3i"),
	csetter = StandardSetter("Vec3i", {use_ref=true}),
}

function Vec3iVar:Serialize()
	local v = self:Get()
	return {v.x, v.y, v.z}
end

function Vec3iVar:Deserialize(v)
	self:Set(ffi.new("Vec3i", v[1], v[2], v[3]))
end

---------------------------------------------------------------------------------
-- Vec3Var
---------------------------------------------------------------------------------
local Vec3Var = AbstractCStructVar:extends {
	ctype = "Vec3",
	cgetter = StandardGetter("Vec3"),
	csetter = StandardSetter("Vec3", {use_ref=true}),
}

function Vec3Var:Serialize()
	local v = self:Get()
	return {v.x, v.y, v.z}
end

function Vec3Var:Deserialize(v)
	self:Set(ffi.new("Vec3", v[1], v[2], v[3]))
end

---------------------------------------------------------------------------------
-- Types
---------------------------------------------------------------------------------

local cvar_types = {
	bool   = BoolVar,
	int    = IntVar,
	float  = FloatVar,
	double = DoubleVar,
	Vec3i  = Vec3iVar,
	Vec3   = Vec3Var,
}

---------------------------------------------------------------------------------
-- Environment
---------------------------------------------------------------------------------

local mt = {}
local env = setmetatable({variables = {}}, mt)
local env_loaded = {}

function mt:__index(key)
	local func = getmetatable(self)[key]
	if func then
		return func
	end

	local var = self.variables[key]
	if not var then
		error("Environment: variable '"..key.."' doesn't exist")
	end

	return var:Get()
end

function mt:__newindex(key, value)
	local var = self.variables[key]
	if not var then
		error("Environment: variable '"..key.."' doesn't exist")
	end

	var:Set(value)
end

function mt.Load()
	local ok, global_env = pcall(utils.ReadFileContents, global.CONFIG_DIR.."/global.lua")
	if ok then
		ok, env_loaded = serpent.load(global_env)
		if not ok then env_loaded = {} end
	end

	for _, var in ipairs(global.GAME_ENV_VARS) do
		local t = cvar_types[var.type]
		if not t then
			error("Environment: failed registering variable of a non-existent type "..var.type)
		end

		if var.gui_attrs and var.gui_attrs ~= "" then
			ok, var.gui_attrs = serpent.load(var.gui_attrs)
			if not ok then
				print("error while parsing gui_attrs for "..var.name)
				var.gui_attrs = nil
			end
		end
		local v = t:new(var)
		if BitTest(var.flags, var_flags.PERSISTENT) then
			local value = env_loaded[var.name]
			if value then
				v:Deserialize(value)
			end
		end
		env.variables[var.name] = v
	end
end

function mt.Save()
	local out = {}
	for name, var in pairs(env.variables) do
		if BitTest(var.flags, var_flags.PERSISTENT) then
			out[name] = var:Serialize()
		end
	end
	local data = serpent.serialize(out, {indent = '\t', sortkeys = true})
	utils.WriteFileContents(global.CONFIG_DIR.."/global.lua", data)
end

function mt.OnSync(ev)
	for i = 0, ev.num do
		local id = ev.ids[i]
		local cvar = global.GAME_ENV_VARS[id]
		local var = env.variables[cvar.name]
		local upgui = var.update_gui
		if upgui then
			upgui()
		end
	end
end

function mt.AutoBool(nice_name)
	local render = require "ng.render"
	local name = string.gsub(string.lower(nice_name), " ", "_")
	if env.variables[name] then
		return env[name]
	end

	local var = LuaVar:new({
		name = name,
		flags = var_flags.GUI,
		nice_name = nice_name,
		gui_attrs = {type="bool"},
		value = false,
	})
	var.on_set[#var.on_set+1] = function()
		render.InvalidateAllShaders()
	end
	env.variables[name] = var
	return false
end

function mt.CreateGUI()
	if rawget(env, "gui") ~= nil then
		return
	end

	local gui = require "gui"
	local g = gui.AddWindow(gui.Vec2i(5, 5), gui.WindowAlignment.FLOATING, true)
	g.window:SetResizable(true)
	g:Slider("slider", {
		orientation = gui.Orientation.VERTICAL,
		on_change = function(v)
			g:Get("scrollbox"):Configure { scroll_y = v }
		end,
	})
	local scrollbox = g:ScrollBox("scrollbox", {
		virtual_size = gui.Vec2i(-1, 24),
	})
	local grid = g:Grid("grid")
	local layout = ""
	local params = {}
	local gui_vars = {}
	for _, var in pairs(env.variables) do
		if BitTest(var.flags, var_flags.GUI) then
			gui_vars[#gui_vars+1] = var
		end
	end
	table.sort(gui_vars, CompareVarNames)
	for _, var in ipairs(gui_vars) do
		if BitTest(var.flags, var_flags.GUI) then
			local cr = var_gui_creators[var.gui_attrs.type or var.ctype]
			if cr then
				local l, p = cr(var, g)
				layout = layout..l
				utils.MergeTables(params, p)
			end
		end
	end

	grid:Layout(layout)
	grid:ConfigureColumn(2, { weight = 1 })

	for k, v in pairs(params) do
		grid:Configure(k, v)
	end

	scrollbox:SetChild("grid")

	local top = g:Grid("top")
	top:Layout [[
		scrollbox slider
	]]
	top:ConfigureRow(1, { weight = 1 })
	top:ConfigureColumn(1, { weight = 1 })
	top:Configure("scrollbox slider", { sticky = "NWSE", pady = 1 })
	top:Configure("scrollbox", { padx = 1 })
	top:Configure("slider", { padx = {0, 1}, ipadx = 1 })
	g:SetTop("top")
	g:RedrawMaybe()

	rawset(env, "gui", g)
end

return env
