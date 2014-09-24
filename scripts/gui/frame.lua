local ngm = require "ng.math"
local abstract_widget = require "gui.abstract_widget"
local utils = require "gui.utils"
local utf8 = require "ng.utf8"
local wm = require "ng.wm"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.min_size = ngm.Vec2i(2, 2)
	self.child = nil
	self.decorate = false
end

utils.DefineWidgetConditions(W, {
	recalc = "child",
	redraw = "decorate",
})

function W:WidgetUnderPoint(p, reason)
	return self.child:WidgetUnderPoint(p, reason)
end

function W:RecalculateSizes()
	self.child:RecalculateSizes()
	self.preferred_size = ngm.Vec2i(2, 2) + self.child.preferred_size
	self.min_size = ngm.Vec2i(2, 2) + self.child.min_size
end

function W:Reconfigure(pos, size)
	W.super.Reconfigure(self, pos, size)
	local cpos = pos + ngm.Vec2i(1, 1)
	local csize = ngm.Vec2iMax(self.child.min_size, size - ngm.Vec2i(2, 2))
	self.child:Reconfigure(cpos, csize)
end

function W:SetChild(name)
	self.child = self.gui:Get(name)
	self.gui:QueueRecalc()
end

local d_tl = utf8.DecodeRune("┌")
local d_tr = utf8.DecodeRune("┐")
local d_bl = utf8.DecodeRune("└")
local d_br = utf8.DecodeRune("┘")
local d_tb = utf8.DecodeRune("─")
local d_lr = utf8.DecodeRune("│")

function W:OnDraw(tb)
	if self.decorate then
		local white, black = wm.Color.WHITE, wm.Color.BLACK
		local p, s = self.position, self.size
		tb:SetCell(ngm.Vec2i(p.x, p.y),             wm.TermboxCell(d_tl, 0, 1, white, black))
		tb:SetCell(ngm.Vec2i(p.x, p.y+s.y-1),       wm.TermboxCell(d_bl, 0, 1, white, black))
		tb:SetCell(ngm.Vec2i(p.x+s.x-1, p.y),       wm.TermboxCell(d_tr, 0, 1, white, black))
		tb:SetCell(ngm.Vec2i(p.x+s.x-1, p.y+s.y-1), wm.TermboxCell(d_br, 0, 1, white, black))
		for x = p.x+1, p.x+s.x-2 do
			tb:SetCell(ngm.Vec2i(x, p.y),       wm.TermboxCell(d_tb, 0, 1, white, black))
			tb:SetCell(ngm.Vec2i(x, p.y+s.y-1), wm.TermboxCell(d_tb, 0, 1, white, black))
		end
		for y = p.y+1, p.y+s.y-2 do
			tb:SetCell(ngm.Vec2i(p.x, y),       wm.TermboxCell(d_lr, 0, 1, white, black))
			tb:SetCell(ngm.Vec2i(p.x+s.x-1, y), wm.TermboxCell(d_lr, 0, 1, white, black))
		end
	end

	local c = self.child
	c:Draw(tb:Partial(c.position, c.position+c.size-ngm.Vec2i(1, 1)))
end

return W
