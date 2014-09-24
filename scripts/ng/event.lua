local M = {}

require "ng.struct"

local ffi = require "ffi"

------------------------------------------------------------------------------
-- Events
------------------------------------------------------------------------------

M.Type = {}
local CTypes = {}

local function RegisterEventType(enum, ctype)
	local next = #CTypes + 1
	M.Type[enum] = next
	CTypes[next] = ctype
end

-- WARNING: order sensitive! (should match corresponding enum in Script/Events.h)
RegisterEventType("KEY",                  "ScriptEventKey*")
RegisterEventType("MOUSE_BUTTON",         "ScriptEventMouseButton*")
RegisterEventType("MOUSE_MOTION",         "ScriptEventMouseMotion*")
RegisterEventType("MOUSE_WHEEL",          "ScriptEventMouseWheel*")
RegisterEventType("TERMBOX_KEY",          "ScriptEventTermboxKey*")
RegisterEventType("TERMBOX_RESIZE",       "ScriptEventTermboxResize*")
RegisterEventType("TERMBOX_MOUSE_BUTTON", "ScriptEventTermboxMouseButton*")
RegisterEventType("TERMBOX_MOUSE_MOTION", "ScriptEventTermboxMouseMotion*")
RegisterEventType("TERMBOX_MOUSE_WHEEL",  "ScriptEventTermboxMouseWheel*")
RegisterEventType("TERMBOX_MOUSE_LEAVE",  "ScriptEventTermboxMouseLeave*")
RegisterEventType("TERMBOX_IMAGE_DRAW",   "ScriptEventTermboxImageDraw*")
RegisterEventType("QUIT",                 nil)
RegisterEventType("ENVIRONMENT_SYNC",     "ScriptEventEnvironmentSync*")

function M.TypeToString(event_type)
	for k, v in pairs(M.Type) do
		if event_type == v then
			return k
		end
	end
	return "<UNKNOWN>"
end

function M.ResolveData(event_type, data)
	local ctype = CTypes[event_type]
	if ctype then
		return ffi.cast(ctype, data)
	end
	return nil
end

return M
