local ngm = require "ng.math"
local consts = require "gui.consts"
local utils = require "gui.utils"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local abstract_widget = require "gui.abstract_widget"

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.min_size = ngm.Vec2i(#self.style.decrement_button + #self.style.increment_button + 8, 1)
	self.preferred_size = ngm.Vec2i(#self.style.decrement_button + #self.style.increment_button + 12, 1)

	-- spinbox customizables
	self.min = 0
	self.max = 1
	self.increment = 0.1
	self.format = "%.1f"
	self.value = 0

	-- editbox state
	self.text = string.format(self.format, self.value)
	self.voffset = 0
	self.cursor_boffset = 0
	self.cursor_voffset = 0

	-- additionals
	self.on_change = nil
	self.active = false
	self.custom = {
		value = function(widget, value)
			widget.value = math.max(self.min, math.min(self.max, value))
			widget.text = string.format(widget.format, widget.value)
			widget.voffset = 0
			widget.cursor_boffset = 0
			widget.cursor_voffset = 0
		end,
		format = function(widget, value)
			widget.format = value
			widget.text = string.format(widget.format, widget.value)
			widget.voffset = 0
			widget.cursor_boffset = 0
			widget.cursor_voffset = 0
		end,
	}
end

local decode = function(text) local r = utf8.DecodeRune(text); return r; end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color.BLACK,
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	arrow_right = utf8.DecodeRune("→"),
	arrow_left = utf8.DecodeRune("←"),
	decrement_button = {decode("["), decode("◀"), decode("]"), decode(" ")},
	increment_button = {decode(" "), decode("["), decode("▶"), decode("]")},
}

utils.DefineWidgetConditions(W, {
	redraw = "style format value",
})

function W:Cursor()
	if not self.active then
		return ngm.Vec2i(0, 0)
	end
	local lpos = ngm.Vec2i(self.position.x + #self.style.decrement_button, self.position.y + self.size.y / 2)
	return utils.EditBoxCursor(lpos, self)
end

function W:OnKey(ev)
	if not self.active then
		return
	end
	if ev.key == consts.Key.RETURN then
		self:Configure { value = tonumber(self.text) or self.min }
		self.active = false
		if self.on_change then
			self.on_change(self.value)
		end
	else
		utils.OnEditBoxKey(self, ev)
	end
	self.gui:QueueRedraw()
end

function W:OnMouseButtonDown(ev)
	local rel = ev.position - self:VisualPosition()
	local y = math.floor(self.size.y / 2)
	if rel.y ~= y then
		return
	end
	if rel.x >= 0 and rel.x <= #self.style.decrement_button-1 then
		-- decrement
		self.active = false
		self:Configure{ value = self.value - self.increment }
		if self.on_change then
			self.on_change(self.value)
		end
	elseif rel.x >= self.size.x-#self.style.increment_button and rel.x <= self.size.x-1 then
		-- increment
		self.active = false
		self:Configure{ value = self.value + self.increment }
		if self.on_change then
			self.on_change(self.value)
		end
	else
		self.active = true
		self.cursor_boffset = utf8.RuneCount(self.text)
		self.cursor_voffset = utf8.RuneCount(self.text, 1, self.cursor_boffset)
	end
	self.gui:QueueRedraw()
end

function W:OnDraw(tb)
	local s = self.style
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end

	local lpos = ngm.Vec2i(self.position.x, self.position.y + self.size.y / 2)
	local lsize = ngm.Vec2i(self.size.x, 0)
	local style = {fg = fg, bg = bg, bga = bga}
	utils.DrawTextSimple(lpos, s.decrement_button, tb, style)
	utils.DrawTextSimple(lpos+lsize-ngm.Vec2i(#s.increment_button, 0), s.increment_button, tb, style)
	utils.DrawEditBox(
		lpos+ngm.Vec2i(#s.decrement_button, 0),
		lsize-ngm.Vec2i(#s.increment_button + #s.decrement_button, -1), self, tb, {
			fg = fg,
			bg = bg,
			bga = bga,
			arrow_right = s.arrow_right,
			arrow_left = s.arrow_left,
		}
	)
end

function W:Activate()
	self.gui:QueueRedraw()
end
function W:Deactivate()
	self.voffset = 0
	self.cursor_boffset = 0
	self.cursor_voffset = 0
	self.gui:QueueRedraw()
end

function W:OnMouseWheel(ev)
	self.active = false
	self:Configure{ value = self.value + self.increment * ev.delta }
	if self.on_change then
		self.on_change(self.value)
	end
end

function W:OnHoverEnter() self.gui:QueueRedraw() end
function W:OnHoverLeave() self.gui:QueueRedraw() end

return W
