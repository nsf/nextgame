local ngm = require "ng.math"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"
local utils = require "gui.utils"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.min_size = ngm.Vec2i(1, 1)

	-- virtual size -1 means don't use a scroll on that axis
	self.virtual_size = ngm.Vec2i(20, 5)
	self.scroll_x = 0
	self.scroll_y = 0
	self.on_scroll_x = nil
	self.on_scroll_y = nil
	self.child = nil
end

utils.DefineWidgetConditions(W, {
	recalc = "child virtual_size",
	redraw = "scroll_x scroll_y",
})

function W:WidgetUnderPoint(p, reason)
	if reason == consts.Reason.MOUSE_WHEEL and self.on_scroll_y then
		return self
	end
	local offset = utils.ScrollOffset(self.size, self.child.size,
		self.scroll_x, self.scroll_y)
	return self.child:WidgetUnderPoint(p + offset, reason)
end

function W:RecalculateSizes()
	self.child:RecalculateSizes()
	if self.virtual_size.x == -1 then
		self.preferred_size.x = self.child.preferred_size.x
		self.min_size.x = self.child.min_size.x
	else
		self.preferred_size.x = self.virtual_size.x
		self.min_size.x = 1
	end

	if self.virtual_size.y == -1 then
		self.preferred_size.y = self.child.preferred_size.y
		self.min_size.y = self.child.min_size.y
	else
		self.preferred_size.y = self.virtual_size.y
		self.min_size.y = 1
	end
end

function W:Reconfigure(pos, size)
	W.super.Reconfigure(self, pos, size)

	local csize = ngm.Vec2i(self.child.preferred_size)
	if self.virtual_size.x == -1 then
		csize.x = size.x
	end
	if self.virtual_size.y == -1 then
		csize.y = size.y
	end
	self.child:Reconfigure(pos, csize)
end

function W:SetChild(name)
	self.child = self.gui:Get(name)
	self.gui:QueueRecalc()
end

function W:OnMouseWheel(ev)
	if not self.on_scroll_y then
		return
	end

	local avail = self.child.size.y - self.size.y
	local factor = 1
	if avail > 0 then
		factor = 1 / avail
	end
	self.on_scroll_y(self.scroll_y - ev.delta * factor)
end

function W:OnDraw(tb)
	local offset = utils.ScrollOffset(self.size, self.child.size,
		self.scroll_x, self.scroll_y)
	tb.offset_x = -offset.x
	tb.offset_y = -offset.y
	self.child:Draw(tb)
end

return W
