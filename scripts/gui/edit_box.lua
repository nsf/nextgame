local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local consts = require "gui.consts"
local utils = require "gui.utils"
local abstract_widget = require "gui.abstract_widget"
local bit = require "bit"

local band = bit.band

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.text = ""
	self.voffset = 0
	self.cursor_boffset = 0
	self.cursor_voffset = 0

	self.min_size = ngm.Vec2i(8, 1)
	self.preferred_size = ngm.Vec2i(20, 1)
	self.custom = {
		text = function(self, value)
			self.text = value
			self.voffset = 0
			self.cursor_boffset = 0
			self.cursor_voffset = 0
		end,
	}
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	arrow_right = utf8.DecodeRune("→"),
	arrow_left = utf8.DecodeRune("←"),
}

function W:Cursor()
	return utils.EditBoxCursor(self.position, self)
end

function W:OnDraw(tb)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end
	local zero_voffset = self.gui.active ~= self
	utils.DrawEditBox(self.position, self.size, self, tb, {
		fg = fg,
		bg = bg,
		bga = bga,
		arrow_right = self.style.arrow_right,
		arrow_left = self.style.arrow_left,
	})
end

function W:OnKey(ev)
	utils.OnEditBoxKey(self, ev)
	self.gui:QueueRedraw()
end

function W:Activate() self.gui:QueueRedraw() end
function W:Deactivate()
	self.voffset = 0
	self.cursor_boffset = 0
	self.cursor_voffset = 0
	self.gui:QueueRedraw()
end

function W:OnHoverEnter() self.gui:QueueRedraw() end
function W:OnHoverLeave() self.gui:QueueRedraw() end

return W
