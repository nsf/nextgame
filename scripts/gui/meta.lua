local gui = require "gui"
local dialogs = require "gui.dialogs"
local ngm = require "ng.math"
local M = {}

function M.ColorPicker(g, name, args)
	local c = args.color
	local on_apply = args.on_apply
	local on_change = args.on_change
	local on_apply_meta, on_change_meta

	if on_apply then
		on_apply_meta = function(c)
			g:Get(name):Configure{
				params = {
					color = {c.x, c.y, c.z},
				}
			}
			g:RedrawMaybe()
			on_apply(c)
		end
	end
	if on_change then
		on_change_meta = function(c)
			g:Get(name):Configure{
				params = {
					color = {c.x, c.y, c.z},
				}
			}
			g:RedrawMaybe()
			on_change(c)
		end
	end
	local context = {
		on_apply = on_apply_meta,
		on_change = on_change_meta,
		color = c,
	}
	g:Image(name, {
		on_click = function()
			dialogs.ColorPicker(context)
		end,
		params = {
			func = "SolidColor",
			color = {c.x, c.y, c.z},
		},
	})
end

return M
