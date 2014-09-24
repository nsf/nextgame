local ngm = require "ng.math"
local class = require "lib.30log"

local W = class()

function W:__init()
	self.name = ""
	self.gui = nil
	self.min_size = ngm.Vec2i(0, 0)
	self.preferred_size = ngm.Vec2i(0, 0)
	self.position = ngm.Vec2i(0, 0)
	self.size = ngm.Vec2i(0, 0)
	self.draw_min = ngm.Vec2i(1, 1)
	self.draw_max = ngm.Vec2i(0, 0)
	self.virtual_offset = ngm.Vec2i(0, 0)
	self.custom = nil
	self.style = self.DefaultStyle
end

function W:VisualPosition()
	return self.position + self.virtual_offset
end

function W:WidgetUnderPoint(p, reason)
	return self
end

function W:Cursor()
	return ngm.Vec2i(0, 0)
end

function W:Activate() end
function W:Deactivate() end

function W:OnKey(ev) end
function W:OnMouseButtonDown(ev) end
function W:OnMouseButtonUp(ev) end
function W:OnMouseButton(ev)
	if ev.down then
		self:OnMouseButtonDown(ev)
	else
		self:OnMouseButtonUp(ev)
	end
end
function W:OnMouseMotion(ev) end
function W:OnMouseWheel(ev) end
function W:OnHoverEnter() end
function W:OnHoverLeave() end
function W:OnDrag(p) end

function W:OnDraw(tb) end
function W:Draw(tb)
	self.virtual_offset = ngm.Vec2i(tb.offset_x, tb.offset_y)
	local b_min = self.position + self.virtual_offset
	local b_max = b_min + self.size - ngm.Vec2i(1)
	local ok
	self.draw_min, self.draw_max, ok = tb:Intersection(b_min, b_max)
	if not ok then
		return
	end
	self:OnDraw(tb)
end

function W:PreferredSize()
	return ngm.Vec2iMax(self.min_size, self.preferred_size)
end

function W:RecalculateSizes() end

function W:Reconfigure(pos, size)
	self.position = pos
	self.size = size
end

function W:Configure(cfg)
	local custom = self.custom
	for k, v in pairs(cfg) do
		if custom and custom[k] then
			custom[k](self, v)
		else
			self[k] = v
		end
		if self.Conditions.recalc[k] then
			self.gui:QueueRecalc()
		end
		if self.Conditions.redraw[k] then
			self.gui:QueueRedraw()
		end
	end
end

W.Conditions = {
	recalc = {}, redraw = {},
}

return W
