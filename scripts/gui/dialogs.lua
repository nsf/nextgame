local M = {}
local ngm = require "ng.math"
local gui = require "gui"

------------------------------------------------------------------------
-- ColorPicker
------------------------------------------------------------------------

local hue_stops = {
	{ hue = 0.0000000, rgb = ngm.Vec3(1, 0, 0) },
	{ hue = 0.1666666, rgb = ngm.Vec3(1, 1, 0) },
	{ hue = 0.3333333, rgb = ngm.Vec3(0, 1, 0) },
	{ hue = 0.5000000, rgb = ngm.Vec3(0, 1, 1) },
	{ hue = 0.6666666, rgb = ngm.Vec3(0, 0, 1) },
	{ hue = 0.8333333, rgb = ngm.Vec3(1, 0, 1) },
	{ hue = 1.0000000, rgb = ngm.Vec3(1, 0, 0) },
}

local min, max, fmod = math.min, math.max, math.fmod

local function RGBToHSV(rgb)
	local cmax = max(rgb.x, rgb.y, rgb.z)
	local cmin = min(rgb.x, rgb.y, rgb.z)
	local delta = cmax - cmin
	local k = 60 / 360
	local h
	if cmax == rgb.x then
		h = k * fmod((rgb.y - rgb.z) / delta, 6)
	elseif cmax == rgb.y then
		h = k * ((rgb.z - rgb.x) / delta + 2)
	else -- cmax == rgb.z
		h = k * ((rgb.x - rgb.y) / delta + 4)
	end
	local s = 0
	if delta ~= 0 then
		s = delta / cmax
	end
	local v = cmax
	return ngm.Vec3(h, s, v)
end

local function HueToRGB(hue)
	for i, cur in ipairs(hue_stops) do
		if cur.hue >= hue then
			if i == 1 then
				return cur.rgb
			end
			local prev = hue_stops[i-1]
			local v = (hue - prev.hue) / (cur.hue - prev.hue)
			return ngm.Vec3Lerp(prev.rgb, cur.rgb, v)
		end
	end
	return hue_stops[#hue_stops].rgb
end

-- important: this function uses precalculated hue rgb value, because we use it
-- anyway
local function HSVToRGB(hue_rgb, hsv)
	local zero = ngm.Vec3(0, 0, 0)
	local white = ngm.Vec3(1, 1, 1)
	return ngm.Vec3Lerp(
		ngm.Vec3Lerp(zero, white, hsv.z),
		ngm.Vec3Lerp(zero, hue_rgb, hsv.z),
		hsv.y)
end

function M.ColorPicker(context)
	local on_apply = context.on_apply
	local on_change = context.on_change
	context.color = context.color or ngm.Vec3(1, 0, 0)
	context.hsv = context.hsv or RGBToHSV(context.color)

	local g = gui.AddWindow(gui.Vec2i(5, 5), gui.WindowAlignment.FLOATING, true)
	g.window:SetResizable(true)

	local update_color = function()
		local hsv = context.hsv
		local hue_rgb = HueToRGB(hsv.x)
		local c = HSVToRGB(hue_rgb, hsv)
		context.color = c
		if on_change then on_change(c) end

		g:Get("color"):Configure{
			params = {
				color = {c.x, c.y, c.z}
			}
		}
		g:Get("color_selector"):Configure{
			params = {
				hue = {hue_rgb.x, hue_rgb.y, hue_rgb.z},
				sel = {hsv.y, 1.0 - hsv.z},
			}
		}
		g:Get("hue_bar"):Configure{
			params = {
				sel = 1 - context.hsv.x,
			}
		}
	end

	local cell_size = g.termbox:CellSize()
	local grid = g:Grid("top")
	local hue_rgb = HueToRGB(context.hsv.x)
	local hsv = context.hsv
	g:Image("color", {
		params = {
			func = "SolidColor",
			color = {context.color.x, context.color.y, context.color.z},
		},
	})
	g:Image("color_selector", {
		params = {
			func = "ColorSelector",
			hue = {hue_rgb.x, hue_rgb.y, hue_rgb.z},
			sel = {hsv.y, 1 - hsv.z},
		},
		preferred_size = gui.Vec2i(
			math.ceil(200 / cell_size.x), math.ceil(200 / cell_size.y)
		),
		on_paint = function(x, y)
			context.hsv.y = x
			context.hsv.z = 1 - y
			update_color()
		end,
	})
	g:Image("hue_bar", {
		params = {
			func = "HueBar",
			sel = 1 - context.hsv.x,
		},
		on_paint = function(_, v)
			context.hsv.x = 1 - v
			update_color()
		end,
	})
	g:Button("apply", {
		text = "Apply",
		on_click = function()
			if on_apply then on_apply(context.color) end
			gui.RemoveWindow(g)
		end,
	})
	g:Button("cancel", {
		text = "Cancel",
		on_click = function()
			gui.RemoveWindow(g)
		end,
	})

	grid:Layout [[
		color_selector -      hue_bar
		color          -      -
		apply          cancel -
	]]
	grid:Configure("color_selector hue_bar", { pady = {1, 0} })
	grid:Configure("color_selector hue_bar color", { sticky = "NWSE" })
	grid:Configure("color", { ipady = 1, pady = 1, padx = 2 })
	grid:Configure("color_selector", { padx = {2, 0} })
	grid:Configure("hue_bar", { padx = {0, 2}, ipadx = 1 })
	grid:Configure("apply cancel", { pady = {0, 1} })
	grid:Configure("apply", { padx = {4, 0}, sticky = "W" })
	grid:Configure("cancel", { padx = {0, 4}, sticky = "E" })
	grid:ConfigureRow(1, { weight = 1 })
	grid:ConfigureColumn(1, 2, { weight = 1 })
	g:SetTop("top")
	g:RedrawMaybe()
end

------------------------------------------------------------------------
-- ListBox
------------------------------------------------------------------------

function M.ListBox(context)

end

return M
