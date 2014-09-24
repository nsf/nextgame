local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local utils = require "gui.utils"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.list = {}
	self.selected = 0
	self.custom = {
		selected = function(self, value)
			self.selected = value
			local text = self.list[value]
			if text then
				local newsize = utf8.RuneCount(text)+3
				if self.preferred_size.x ~= newsize then
					self.gui:QueueRecalc()
				else
					self.gui:QueueRedraw()
				end
			end
		end,
	}
	self.on_change = nil

	self.min_size = ngm.Vec2i(1, 1)
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	align = consts.Alignment.LEFT,
	ellipsis = utf8.DecodeRune("…"),
	center_ellipsis = false,
	arrow_down = utf8.DecodeRune("▼"),
}

W.Conditions = {
	recalc = {},
	redraw = {
		style = true,
	},
}

function W:RecalculateSizes()
	local text = self.list[self.selected]
	if text then
		self.preferred_size = ngm.Vec2i(utf8.RuneCount(text) + 3, 1)
	else
		self.preferred_size = ngm.Vec2i(3, 1)
	end
end

function W:OnMouseButtonUp(ev)
	local gui = require "gui"

	local p = self.gui.window:TermboxPosition() +
		(self:VisualPosition() - ngm.Vec2i(2, 2)) * self.gui.termbox:CellSize()
	local g = gui.AddWindow(p, gui.WindowAlignment.FLOATING, false)
	gui.MakeModal(g)

	local top = g:Frame("top", { decorate = true })
	g:ListBox("listbox", {
		list = self.list,
		selected = 1,
		calculate_preferred_width = true,
		combo_box_mode = true,
		on_change = function(idx)
			gui.MakeModal(nil)
			self:Configure { selected = idx }
			self.gui:RedrawMaybe()
			if self.on_change then
				self.on_change(idx)
			end
		end,
	})
	top:SetChild("listbox")
	g:SetTop("top")
	g:RedrawMaybe()
end

function W:OnDraw(tb)
	local lpos = ngm.Vec2i(self.position.x, self.position.y + self.size.y / 2)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end
	tb:SetCell(lpos + ngm.Vec2i(self.size.x-1, 0), wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))
	tb:SetCell(lpos + ngm.Vec2i(self.size.x-2, 0), wm.TermboxCell(self.style.arrow_down, 0, bga, fg, bg))
	tb:SetCell(lpos + ngm.Vec2i(self.size.x-3, 0), wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))
	local text = self.list[self.selected]
	if not text then
		return
	end
	utils.DrawText(self.position, self.size - ngm.Vec2i(3, 0), text, tb, {
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
