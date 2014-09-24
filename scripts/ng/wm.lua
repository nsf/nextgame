local M = {}

require "ng.struct"
local ngm = require "ng.math"

local binding = require "ng.binding"
local bit = require "bit"
local ffi = require "ffi"

local band, bor, lshift, rshift = bit.band, bit.bor, bit.lshift, bit.rshift

------------------------------------------------------------------------------
-- Color
------------------------------------------------------------------------------

local mt = {}
mt.__index = mt
function mt:__new(r, g, b)
	local r2 = math.floor(r * 31)
	local g2 = math.floor(g * 63)
	local b2 = math.floor(b * 31)
	return ffi.new(self, bor(bor(lshift(r2, 11), lshift(g2, 5)), b2))
end
function mt:R() return rshift(self.color, 11) / 31.0          end
function mt:G() return band(rshift(self.color, 5), 63) / 63.0 end
function mt:B() return band(self.color, 31) / 31.0            end
M.Color = ffi.metatype("RGB565", mt)

mt.BLACK        = M.Color(0.000, 0.000, 0.000)
mt.GREY         = M.Color(0.498, 0.498, 0.498)
mt.WHITE        = M.Color(1.000, 1.000, 1.000)
mt.BOLD_WHITE   = M.Color(1.000, 1.000, 1.000)
mt.RED          = M.Color(0.804, 0.000, 0.000)
mt.BOLD_RED     = M.Color(1.000, 0.000, 0.000)
mt.GREEN        = M.Color(0.000, 0.804, 0.000)
mt.BOLD_GREEN   = M.Color(0.000, 1.000, 0.000)
mt.BLUE         = M.Color(0.067, 0.235, 0.447)
mt.BOLD_BLUE    = M.Color(0.192, 0.439, 0.749)
mt.YELLOW       = M.Color(0.804, 0.804, 0.000)
mt.BOLD_YELLOW  = M.Color(1.000, 1.000, 0.000)
mt.MAGENTA      = M.Color(0.804, 0.000, 0.804)
mt.BOLD_MAGENTA = M.Color(1.000, 0.000, 1.000)
mt.CYAN         = M.Color(0.000, 0.804, 0.804)
mt.BOLD_CYAN    = M.Color(0.000, 1.000, 1.000)

------------------------------------------------------------------------------
-- TermboxCell
------------------------------------------------------------------------------

mt = {}
mt.__index = mt
function mt:__new(rune, attr, bg_alpha, fg, bg)
	-- RUNE_MASK == (1U << 21) - 1 == 2097151
	-- ATTR_MASK == ((1U << 5) - 1) << 21 == 65011712
	-- ATTR_SHIFT == 21U == 21
	-- BG_A_MASK = ((1U << 6) - 1) << 26 == 4227858432
	-- BG_A_SHIFT == 26U == 26
	return ffi.new(self,
		bor(
			band(rune, 2097151),
			band(lshift(attr, 21), 65011712),
			band(lshift(bg_alpha * 63, 26), 4227858432)
		),
		fg.color,
		bg.color
	)
end
function mt:Rune() return band(self.rune_attr, 2097151) end
function mt:Attr() return rshift(band(self.rune_attr, 65011712), 21) end
function mt:BG_A() return rshift(band(self.rune_attr, 4227858432), 26) / 63 end
mt.BOLD   = lshift(1, 0)
mt.INVERT = lshift(1, 1)
M.TermboxCell = ffi.metatype("TermboxCell", mt)

------------------------------------------------------------------------------
-- Termbox
------------------------------------------------------------------------------

local function MakeTermboxPartial(tb, a_min, a_max, b_min, b_max, offset)
	b_min, b_max = b_min + offset, b_max + offset
	local c_min, c_max = ngm.RectangleIntersection(a_min, a_max, b_min, b_max)
	return ffi.new("TermboxPartial", tb, c_min.x, c_min.y, c_max.x, c_max.y, offset.x, offset.y)
end

mt = {}
mt.__index = mt
binding.DefineMethods(mt, [[
	Vec2i NG_Termbox_Size(Termbox *tb);
	void NG_Termbox_Clear(Termbox *tb, RGB565 fg, RGB565 bg, float bg_alpha);
	void NG_Termbox_SetCursor(Termbox *tb, const Vec2i &p);
	void NG_Termbox_SetCell(Termbox *tb, const Vec2i &p, const TermboxCell &cell);
	void NG_Termbox_SetImage(Termbox *tb, const Vec2i &p, const Vec2i &size, int id, const char *params);
	Vec2i NG_Termbox_CellSize(Termbox *tb);
]])
function mt:Partial(b_min, b_max)
	local size = self:Size()
	if not b_min then
		return ffi.new("TermboxPartial", self, 1, 1, size.x, size.y)
	else
		return MakeTermboxPartial(self, ngm.Vec2i(1, 1), size, b_min, b_max, ngm.Vec2i(0, 0))
	end
end
M.Termbox = ffi.metatype("Termbox", mt)

------------------------------------------------------------------------------
-- TermboxPartial
------------------------------------------------------------------------------

mt = {}
mt.__index = mt
binding.DefineMethods(mt, [[
	void NG_TermboxPartial_SetCell(TermboxPartial *tb,
		const Vec2i &p, const TermboxCell &cell);
	void NG_TermboxPartial_SetImage(TermboxPartial *tb,
		const Vec2i &p, const Vec2i &size, int id, const char *params);
	void NG_TermboxPartial_Fill(TermboxPartial *tb,
		const Vec2i &p, const Vec2i &size, const TermboxCell &cell);
]])
function mt:Intersection(b_min, b_max)
	return ngm.RectangleIntersection(
		ngm.Vec2i(self.min_x, self.min_y),
		ngm.Vec2i(self.max_x, self.max_y),
		b_min, b_max)
end
function mt:Partial(b_min, b_max)
	return MakeTermboxPartial(
		self.termbox,
		ngm.Vec2i(self.min_x, self.min_y),
		ngm.Vec2i(self.max_x, self.max_y),
		b_min, b_max,
		ngm.Vec2i(self.offset_x, self.offset_y))
end
M.TermboxPartial = ffi.metatype("TermboxPartial", mt)

------------------------------------------------------------------------------
-- Window
------------------------------------------------------------------------------

M.WindowAlignment = {
	FLOATING = 0,
	NW = 1,
	NE = 2,
	SW = 3,
	SE = 4,
}

mt = {}
mt.__index = mt
binding.DefineMethods(mt, [[
	Termbox *NG_Window_Termbox(Window *win);
	void NG_Window_SetDirtyPartial(Window *win);
	void NG_Window_SetResizable(Window *win, bool b);
	void NG_Window_SetAutoResize(Window *win, bool b);
	void NG_Window_Resize(Window *win, const Vec2i &size);
	Vec2i NG_Window_TermboxPosition(Window *win);
]])
M.Window = ffi.metatype("Window", mt)

------------------------------------------------------------------------------
return M
