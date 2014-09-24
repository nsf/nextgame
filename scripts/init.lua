print("NEXTGAME INIT")

local materials = require "materials"

materials.LoadAllMaterialPacks()

---------------------------------------------------------------------------------
-- TEMPORARY CODE BELOW
---------------------------------------------------------------------------------

-- LUAJIT DEBUG BEG
-- local v = require "jit.v"
-- v.on(global.BASE_DIR.."/jitdump.txt")
-- LUAJIT DEBUG END

local utils = require "utils"
local ngm = require "ng.math"
local event = require "ng.event"
local gui_testing = require "gui.testing"
local dialogs = require "gui.dialogs"
local meta = require "gui.meta"
local gui = require "gui"
local game = require "ng.game"
local env = require "ng.env"
local render = require "ng.render"

global.ShaderEnv = {
	env = env,
}

local gui_enabled = false
local button_1 = false

function global.OnEvent(event_type, data)
	local evdata = event.ResolveData(event_type, data)
	if event_type == event.Type.QUIT then
		env.Save()
	elseif event_type == event.Type.ENVIRONMENT_SYNC then
		env.OnSync(evdata)
	end

	if not gui_enabled then
		if event_type == event.Type.KEY then
			if evdata.key == gui.Key.I and evdata.down then
				game.Info()
			elseif evdata.key == gui.Key.O and evdata.down then
				game.InfoDebug()
			elseif evdata.key == gui.Key.SPACE then
				game.Jump()
			elseif evdata.key == gui.Key.W then
				env.player_moving_forward = evdata.down
			elseif evdata.key == gui.Key.S then
				env.player_moving_backward = evdata.down
			elseif evdata.key == gui.Key.A then
				env.player_moving_left = evdata.down
			elseif evdata.key == gui.Key.D then
				env.player_moving_right = evdata.down
			elseif evdata.key == gui.Key.M and evdata.down then
				env.wire = not env.wire
			elseif evdata.key == gui.Key.NUM_1 and evdata.down then
				env.camera_velocity = 4.0
			elseif evdata.key == gui.Key.NUM_2 and evdata.down then
				env.camera_velocity = 8.0
			elseif evdata.key == gui.Key.NUM_3 and evdata.down then
				env.camera_velocity = 16.0
			elseif evdata.key == gui.Key.NUM_4 and evdata.down then
				env.camera_velocity = 32.0
			elseif evdata.key == gui.Key.NUM_5 and evdata.down then
				env.camera_velocity = 64.0
			elseif evdata.key == gui.Key.NUM_6 and evdata.down then
				env.camera_velocity = 128.0
			elseif evdata.key == gui.Key.NUM_7 and evdata.down then
				env.camera_velocity = 256.0
			elseif evdata.key == gui.Key.NUM_8 and evdata.down then
				global.ShaderEnv.Variant = not global.ShaderEnv.Variant
				render.InvalidateAllShaders()
			elseif evdata.key == gui.Key.F8 and evdata.down then
				materials.LoadAllMaterialPacks()
			elseif evdata.key == gui.Key.NUM_9 and evdata.down then
				game.ToggleGravity()
			elseif evdata.key == gui.Key.NUM_0 and evdata.down then
				env.update_map = not env.update_map
			elseif evdata.key == gui.Key.R and evdata.down then
				env.threshold = 0
				game.Do()
			elseif evdata.key == gui.Key.F and evdata.down then
				env.seed = env.seed + 1
				game.Do()
			elseif evdata.key == gui.Key.G and evdata.down then
				env.threshold = env.threshold + 0.001
				game.Do()
			elseif evdata.key == gui.Key.H and evdata.down then
				env.threshold = env.threshold + 0.01
				game.Do()
			elseif evdata.key == gui.Key.J and evdata.down then
				env.threshold = env.threshold + 0.1
				game.Do()
			elseif evdata.key == gui.Key.K and evdata.down then
				env.threshold = env.threshold + 0.5
				game.Do()
			elseif evdata.key == gui.Key.BACKQUOTE and evdata.down then
				gui_enabled = not gui_enabled
				game.EnableGUI(gui_enabled)
				env.CreateGUI()
			end
		elseif event_type == event.Type.MOUSE_BUTTON and evdata.button == 1 then
			button_1 = evdata.down
		elseif event_type == event.Type.MOUSE_MOTION then
			if button_1 then
				game.RotateCamera(evdata.relative.x, evdata.relative.y);
			end
		end
	else -- gui_enabled == true
		if event_type == event.Type.KEY and evdata.key == gui.Key.BACKQUOTE and evdata.down then
			gui_enabled = not gui_enabled
			game.EnableGUI(gui_enabled)
		else
			gui.DispatchEvent(event_type, evdata)
		end
	end
end

---------------------------------------------------------------------------------
if global.DEBUG then
	utils.RunTests(global.SCRIPTS_DIR)
	print("all tests passed")
end
