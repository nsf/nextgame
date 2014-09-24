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
	self.checked = false
	self.on_change = nil
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	side = consts.Side.LEFT,
	tick = utf8.DecodeRune("●"),
	button = {utf8.DecodeRune("["), utf8.DecodeRune("]")},
	align = consts.Alignment.LEFT,
	ellipsis = utf8.DecodeRune("…"),
	center_ellipsis = false,
}

W.Conditions = {
	recalc = {
		text = true,
	},
	redraw = {
		style = true,
		checked = true,
	},
}

function W:OnMouseButtonDown(ev)
	self.checked = not self.checked
	if self.on_change then
		self.on_change(self.checked)
	end
	self.gui:QueueRedraw()
end

function W:RecalculateSizes()
	self.preferred_size = ngm.Vec2i(utf8.RuneCount(self.text)+4, 1)
end

function W:OnDraw(tb)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end
	tb:Fill(self.position, self.size, wm.TermboxCell(
		string.byte(" "), 0, bga, fg, bg))

	local position = ngm.Vec2i(self.position)
	local size = ngm.Vec2i(self.size)
	local y = position.y + size.y / 2
	local x = position.x
	local left = self.style.button[1]
	local right = self.style.button[2]
	local tick
	if self.checked then
		tick = self.style.tick
	else
		tick = string.byte(" ")
	end

	size.x = size.x - 4
	if self.style.side == consts.Side.LEFT then
		position.x = position.x + 4
	else
		x = x + size.x + 1
	end

	tb:SetCell(ngm.Vec2i(x+0, y), wm.TermboxCell(left, 0, bga, fg, bg))
	tb:SetCell(ngm.Vec2i(x+1, y), wm.TermboxCell(tick, 0, bga, fg, bg))
	tb:SetCell(ngm.Vec2i(x+2, y), wm.TermboxCell(right, 0, bga, fg, bg))

	utils.DrawText(position, size, self.text, tb, {
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
