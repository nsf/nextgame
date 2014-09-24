local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local utils = require "gui.utils"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.text = ""
	self.custom = {
		text = function(self, value)
			self.text = value
			local newsize = utf8.RuneCount(value)
			if self.preferred_size.x ~= newsize then
				self.gui:QueueRecalc()
			else
				self.gui:QueueRedraw()
			end
		end,
	}

	self.min_size = ngm.Vec2i(1, 1)
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	align = consts.Alignment.CENTER,
	ellipsis = utf8.DecodeRune("…"),
	center_ellipsis = false,
}

W.Conditions = {
	recalc = {},
	redraw = {
		style = true,
	},
}

function W:RecalculateSizes()
	self.preferred_size = ngm.Vec2i(utf8.RuneCount(self.text), 1)
end

function W:OnDraw(tb)
	utils.DrawText(self.position, self.size, self.text, tb, {
		fg = self.style.fg,
		bg = self.style.bg,
		bga = self.style.bga,
		align = self.style.align,
		ellipsis = self.style.ellipsis,
		center_ellipsis = self.style.center_ellipsis,
	})
end

return W
