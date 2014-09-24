local M = {}

local keys = require "ng.keys"

-- TODO: upper case consts
M.Alignment = {
	LEFT = 1,
	CENTER = 2,
	RIGHT = 3,
}

M.Side = {
	LEFT = 1,
	RIGHT = 3, -- matching with Alignment, just in case
}

M.Orientation = {
	HORIZONTAL = 1,
	VERTICAL = 2,
}

-- WidgetUnderPoint uses this
M.Reason = {
	MOUSE_BUTTON = 1,
	MOUSE_WHEEL = 2,
	MOUSE_MOTION = 3,
}

M.Key = keys.Key
M.Mod = keys.Mod

return M
