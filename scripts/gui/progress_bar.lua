local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"

local min, max, floor = math.min, math.max, math.floor

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.value = 0
	self.orientation = consts.Orientation.HORIZONTAL
	self.min_size = ngm.Vec2i(1, 1)
	self.custom = {
		value = function(self, value)
			self.value = ngm.Clamp(value, 0.0, 1.0)
		end,
	}
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color(0.1, 0.1, 0.1),
	bga = 1,
	vpart_runes = {"▁", "▂", "▃", "▄", "▅", "▆", "▇"},
	hpart_runes = {"▏", "▎", "▍", "▌", "▋", "▊", "▉"},
}

W.Conditions = {
	recalc = {
		orientation = true,
	},
	redraw = {
		style = true,
		value = true,
	},
}

function W:RecalculateSizes()
	if self.orientation == consts.Orientation.HORIZONTAL then
		self.preferred_size = ngm.Vec2i(20, 1)
	else
		self.preferred_size = ngm.Vec2i(1, 10)
	end
end

function W:OnDraw(tb)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	local pos, size = self.position, self.size
	tb:Fill(pos, size, wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))
	if self.orientation == consts.Orientation.HORIZONTAL then
		local step = 1.0 / self.size.x
		local sx = floor(self.value / step)
		local micro = self.value / step - sx
		if sx+0.1 < self.size.x then
			local mrunes = self.style.hpart_runes
			local i = floor(micro * (#mrunes+1))
			if i > 0 then
				local mrune = utf8.DecodeRune(mrunes[i])
				tb:Fill(pos + ngm.Vec2i(sx+0.1, 0),
					ngm.Vec2i(1, size.y),
					wm.TermboxCell(mrune, 0, bga, fg, bg))
			end
		end
		size = ngm.Vec2i(sx+0.1, self.size.y)
	else
		local step = 1.0 / self.size.y
		local sy = floor(self.value / step)
		local micro = self.value / step - sy
		if sy+0.1 < self.size.y then
			local mrunes = self.style.vpart_runes
			local i = floor(micro * (#mrunes+1))
			if i > 0 then
				local mrune = utf8.DecodeRune(mrunes[i])
				tb:Fill(pos + ngm.Vec2i(0, self.size.y-0.9-sy),
					ngm.Vec2i(size.x, 1),
					wm.TermboxCell(mrune, 0, bga, fg, bg))
			end
		end
		size = ngm.Vec2i(self.size.x, sy+0.1)
		pos = pos + ngm.Vec2i(0, self.size.y+0.1-sy)
	end
	tb:Fill(pos, size, wm.TermboxCell(string.byte(" "), 0, 1, fg, fg))
end

return W
