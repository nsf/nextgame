local ngm = require "ng.math"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"
local utils = require "gui.utils"
local utf8 = require "ng.utf8"
local wm = require "ng.wm"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.min_size = ngm.Vec2i(1, 1)
	self.text = ""
	self.child = nil
	self.expanded = true
end

W.DefaultStyle = {
	expanded = utf8.DecodeRune("▼"),
	collapsed = utf8.DecodeRune("▶"),
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	align = consts.Alignment.LEFT,
	ellipsis = utf8.DecodeRune("…"),
	center_ellipsis = false,
}

utils.DefineWidgetConditions(W, {
	recalc = "child text expanded",
})

function W:WidgetUnderPoint(p, reason)
	local smin, smax = self.position, self.position+ngm.Vec2i(self.size.x, 1)-ngm.Vec2i(1, 1)
	if smin <= p and p <= smax then
		return self
	end
	if self.expanded then
		return self.child:WidgetUnderPoint(p, reason)
	end
	return nil
end

function W:RecalculateSizes()
	if self.expanded then
		self.child:RecalculateSizes()
		self.min_size = ngm.Vec2i(
			math.max(1, self.child.min_size.x),
			1 + self.child.min_size.y
		)
		self.preferred_size = ngm.Vec2i(
			math.max(utf8.RuneCount(self.text)+4, self.child.preferred_size.x),
			1 + self.child.preferred_size.y
		)
	else
		self.min_size = ngm.Vec2i(1, 1)
		self.preferred_size = ngm.Vec2i(utf8.RuneCount(self.text)+4, 1)
	end
end

function W:Reconfigure(pos, size)
	W.super.Reconfigure(self, pos, size)
	if self.expanded then
		local cpos = pos + ngm.Vec2i(0, 1)
		local csize = ngm.Vec2iMax(self.child.min_size, size - ngm.Vec2i(0, 1))
		self.child:Reconfigure(cpos, csize)
	end
end

function W:SetChild(name)
	self.child = self.gui:Get(name)
	self.gui:QueueRecalc()
end

function W:OnDraw(tb)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end
	local arrow = self.style.collapsed
	if self.expanded then
		arrow = self.style.expanded
	end

	local lpos = self.position
	tb:SetCell(lpos, wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))
	lpos = lpos + ngm.Vec2i(1, 0)
	tb:SetCell(lpos, wm.TermboxCell(arrow, 0, bga, fg, bg))
	lpos = lpos + ngm.Vec2i(1, 0)
	tb:SetCell(lpos, wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))
	lpos = lpos + ngm.Vec2i(1, 0)
	local lsize = ngm.Vec2i(self.size.x - 4, 1)
	utils.DrawText(lpos, lsize, self.text, tb, {
		fg = fg,
		bg = bg,
		bga = bga,
		align = self.style.align,
		ellipsis = self.style.ellipsis,
		center_ellipsis = self.style.center_ellipsis,
	})
	lpos = self.position + ngm.Vec2i(self.size.x-1, 0)
	tb:SetCell(lpos, wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))

	if self.expanded then
		local c = self.child
		c:Draw(tb:Partial(c.position, c.position+c.size-ngm.Vec2i(1, 1)))
	end
end

function W:OnMouseButtonDown(ev)
	self.expanded = not self.expanded
	self.gui:QueueRecalc()
end

function W:OnHoverEnter() self.gui:QueueRedraw() end
function W:OnHoverLeave() self.gui:QueueRedraw() end

return W
