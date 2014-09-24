local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local consts = require "gui.consts"
local abstract_widget = require "gui.abstract_widget"

local decode = utf8.DecodeRune
local min, max, floor = math.min, math.max, math.floor

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.value = 0
	self.orientation = consts.Orientation.HORIZONTAL
	self.min_size = ngm.Vec2i(1, 1)
	self.custom = {
		value = function(self, value)
			value = min(value, 1.0)
			value = max(value, 0.0)
			self.value = value
		end,
	}
	self.on_change = nil
end

W.DefaultStyle = {
	fg = wm.Color.WHITE,
	bg = wm.Color(0.1, 0.1, 0.1),
	bga = 1,
	fg_hover = wm.Color.WHITE,
	bg_hover = wm.Color(0.2, 0.2, 0.2),
	bga_hover = 1,
	vpart_runes = {decode("▁"), decode("▂"), decode("▃"), decode("▄"), decode("▅"), decode("▆"), decode("▇")},
	hpart_runes = {decode("▏"), decode("▎"), decode("▍"), decode("▌"), decode("▋"), decode("▊"), decode("▉")},
}

W.Conditions = {
	recalc = {
		orientation = true,
	},
	redraw = {
		style = true,
		value = true,
	},
}

function W:RecalculateSizes()
	if self.orientation == consts.Orientation.HORIZONTAL then
		self.preferred_size = ngm.Vec2i(20, 1)
	else
		self.preferred_size = ngm.Vec2i(1, 10)
	end
end

function W:OnDraw(tb)
	local fg, bg, bga = self.style.fg, self.style.bg, self.style.bga
	if self.gui.hovered == self then
		fg, bg, bga = self.style.fg_hover, self.style.bg_hover, self.style.bga_hover
	end
	tb:Fill(self.position, self.size, wm.TermboxCell(string.byte(" "), 0, bga, fg, bg))
	local cell_size = self.gui.termbox:CellSize()

	-- max caret position in pixels, relative to widget position
	local cpos_max = self.size * cell_size - cell_size
	if self.orientation == consts.Orientation.HORIZONTAL then
		-- caret position in pixels
		local cpos = self.value * cpos_max.x
		-- caret position in cells
		local cpos_cells = cpos / cell_size.x
		-- offset of the cell where the cursor is (starting from 0)
		local cpos_cells_offset = floor(cpos_cells)
		local size = ngm.Vec2i(1, self.size.y)
		local part = floor((cpos_cells - cpos_cells_offset) * 8)
		if part == 0 then
			-- if caret is cell-aligned, then we just fill one cell
			local pos = self.position + ngm.Vec2i(cpos_cells_offset, 0)
			tb:Fill(pos, size, wm.TermboxCell(string.byte(" "),
				wm.TermboxCell.INVERT, bga, fg, bg))
		else
			-- otherwise we draw two partial cells, one of them is inverted
			local part_rune = self.style.hpart_runes[part]
			local pos = self.position + ngm.Vec2i(cpos_cells_offset, 0)
			tb:Fill(pos, size, wm.TermboxCell(part_rune,
				wm.TermboxCell.INVERT, bga, fg, bg))
			pos.x = pos.x + 1
			tb:Fill(pos, size, wm.TermboxCell(part_rune,
				0, bga, fg, bg))
		end
	else
		-- caret position in pixels
		local cpos = self.value * cpos_max.y
		-- caret position in cells
		local cpos_cells = cpos / cell_size.y
		-- offset of the cell where the cursor is (starting from 0)
		local cpos_cells_offset = floor(cpos_cells)
		local size = ngm.Vec2i(self.size.x, 1)
		local part = floor((cpos_cells - cpos_cells_offset) * 8)
		if part == 0 then
			-- if caret is cell-aligned, then we just fill one cell
			local pos = self.position + ngm.Vec2i(0, cpos_cells_offset)
			tb:Fill(pos, size, wm.TermboxCell(string.byte(" "),
				wm.TermboxCell.INVERT, bga, fg, bg))
		else
			-- otherwise we draw two partial cells, one of them is inverted
			local part_rune = self.style.vpart_runes[8-part]
			local pos = self.position + ngm.Vec2i(0, cpos_cells_offset)
			tb:Fill(pos, size, wm.TermboxCell(part_rune,
				0, bga, fg, bg))
			pos.y = pos.y + 1
			tb:Fill(pos, size, wm.TermboxCell(part_rune,
				wm.TermboxCell.INVERT, bga, fg, bg))
		end
	end
end

local function UpdateValue(self, ev)
	local cell_size = self.gui.termbox:CellSize()
	local pixel_pos = ev.pixel_position - (self:VisualPosition() - ngm.Vec2i(1, 1)) * cell_size

	-- max caret position in pixels, relative to widget position
	local cpos_max = self.size * cell_size - cell_size
	local value
	if self.orientation == consts.Orientation.HORIZONTAL then
		-- we offset cursor position by half of the cell_size to center the caret on cursor
		local p = pixel_pos.x - cell_size.x / 2
		value = ngm.Clamp(p / cpos_max.x, 0, 1)
	else
		local p = pixel_pos.y - cell_size.y / 2
		value = ngm.Clamp(p / cpos_max.y, 0, 1)
	end

	if value ~= self.value then
		self.value = value
		if self.on_change then
			self.on_change(value)
		end
		self.gui:QueueRedraw()
	end
end

function W:OnMouseButtonDown(ev)
	UpdateValue(self, ev)
end

function W:OnDrag(ev)
	UpdateValue(self, ev)
end

local function Scroll(self, v)
	v = ngm.Clamp(v, 0, 1)
	if v ~= self.value then
		self.value = v
		if self.on_change then
			self.on_change(v)
		end
	end
end

W.ScrollX = Scroll
W.ScrollY = Scroll

function W:OnHoverEnter() self.gui:QueueRedraw() end
function W:OnHoverLeave() self.gui:QueueRedraw() end

return W
