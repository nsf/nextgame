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
	self.min_size = ngm.Vec2i(1, 1)
	self.on_click = nil
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	align = consts.Alignment.CENTER,
	ellipsis = utf8.DecodeRune("…"),
	center_ellipsis = false,
}

W.Conditions = {
	recalc = {
		text = true,
	},
	redraw = {
		style = true,
	},
}

function W:OnMouseButtonDown(ev)
	if self.on_click then
		self.on_click()
	end
end

function W:RecalculateSizes()
	self.preferred_size = ngm.Vec2i(utf8.RuneCount(self.text)+4, 1)
end

function W:OnDraw(tb)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end
	local lpos = ngm.Vec2i(self.position.x, self.position.y + self.size.y / 2)
	local lsize = ngm.Vec2i(self.size.x, 0)
	tb:SetCell(lpos, wm.TermboxCell(string.byte("["), 0, bga, fg, bg))
	tb:SetCell(lpos+lsize-ngm.Vec2i(1, 0), wm.TermboxCell(string.byte("]"), 0, bga, fg, bg))
	utils.DrawText(lpos+ngm.Vec2i(1, 0), lsize-ngm.Vec2i(2, -1), " "..self.text.." ", tb, {
		fg = fg,
		bg = bg,
		bga = bga,
		align = self.style.align,
		ellipsis = self.style.ellipsis,
		center_ellipsis = self.style.center_ellipsis,
	})
end

function W:OnHoverEnter() self.gui:QueueRedraw() end
function W:OnHoverLeave() self.gui:QueueRedraw() end

return W
