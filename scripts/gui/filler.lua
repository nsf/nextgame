local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local abstract_widget = require "gui.abstract_widget"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.rune = ""
	self.attr = 0
	self.fg = wm.Color.WHITE
	self.bg = wm.Color.BLACK
	self.bga = 1
	self.min_size = ngm.Vec2i(1, 1)
end

function W:OnDraw(tb)
	tb:Fill(self.position, self.size,
		wm.TermboxCell(utf8.DecodeRune(self.rune), self.attr,
			self.bga, self.fg, self.bg))
end

return W
