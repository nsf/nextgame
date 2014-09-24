local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local utils = require "gui.utils"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"

local floor, max = math.floor, math.max

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.list = {}
	self.selected = 0
	self.scroll_y = 0
	self.min_size = ngm.Vec2i(1, 1)
	self.preferred_size = ngm.Vec2i(20, 5)
	self.on_change = nil
	self.on_scroll_y = nil
	self.calculate_preferred_width = false
	self.min_preferred_width = 20
	self.combo_box_mode = false
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hl = wm.Color.BLACK,
	bg_hl = wm.Color.WHITE,
	bga_hl = 1,
	align = consts.Alignment.LEFT,
	ellipsis = utf8.DecodeRune("…"),
	center_ellipsis = false,
}

utils.DefineWidgetConditions(W, {
	recalc = "list calculate_preferred_width min_preferred_width",
	redraw = "style selected scroll_y",
})

function W:RecalculateSizes()
	self.preferred_size.y = max(#self.list, 1)
	if self.calculate_preferred_width then
		local w = 0
		for _, item in ipairs(self.list) do
			w = max(w, utf8.RuneCount(item))
		end
		self.preferred_size.x = max(w, self.min_preferred_width)
	end
end

function W:OnDraw(tb)
	tb:Fill(self.position, self.size,
		wm.TermboxCell(string.byte(" "),
			0, self.style.bga, wm.Color.WHITE, self.style.bg))

	local size = ngm.Vec2i(self.size.x, 1)
	local off = utils.ScrollOffset(
		self.size, ngm.Vec2i(0, #self.list), 0, self.scroll_y)

	for i = 1, self.size.y do
		local t = self.list[off.y + i]
		if not t then
			break
		end

		local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
		if off.y+i == self.selected then
			fg, bg, bga = self.style.fg_hl, self.style.bg_hl, self.style.bga_hl
		end

		utils.DrawText(self.position + ngm.Vec2i(0, i-1), size, t, tb, {
			fg = fg,
			bg = bg,
			bga = bga,
			align = self.style.align,
			ellipsis = self.style.ellipsis,
			center_ellipsis = self.style.center_ellipsis,
		})
	end
end

local function UpdateSelected(self, ev, notify, force_notify)
	local off = utils.ScrollOffset(
		self.size, ngm.Vec2i(0, #self.list), 0, self.scroll_y)
	local i = ev.position.y - self:VisualPosition().y + 1 + off.y
	if i > 0 and i <= #self.list and (self.selected ~= i or force_notify) then
		self.selected = i
		if notify and self.on_change then
			self.on_change(i)
		end
		self.gui:QueueRedraw()
	end
end

function W:OnMouseButton(ev)
	if ev.button ~= 1 then
		return
	end

	if ev.down then
		UpdateSelected(self, ev, self.combo_box_mode == false)
	elseif self.combo_box_mode then
		UpdateSelected(self, ev, true, true)
	end
end

function W:OnDrag(ev)
	UpdateSelected(self, ev, self.combo_box_mode == false)
end

function W:OnMouseMotion(ev)
	if not self.combo_box_mode then
		return
	end
	UpdateSelected(self, ev, false)
end

function W:OnMouseWheel(ev)
	if self.on_scroll_y then
		local avail = #self.list - self.size.y
		local factor = 1
		if avail > 0 then
			factor = 1 / avail
		end
		self.on_scroll_y(self.scroll_y - ev.delta * factor)
	end
end

return W
