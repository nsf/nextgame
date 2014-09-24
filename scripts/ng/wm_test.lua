local wm = require "ng.wm"

local black = wm.Color.BLACK
assert(black:R() == 0.0)
assert(black:G() == 0.0)
assert(black:B() == 0.0)

local white = wm.Color.WHITE
assert(white:R() == 1.0)
assert(white:G() == 1.0)
assert(white:B() == 1.0)

local cell = wm.TermboxCell(string.byte('a'), 31, 1.0,
	wm.Color.BLUE, wm.Color.BOLD_CYAN)

assert(cell:Rune() == string.byte('a'))
assert(cell:Attr() == 31)
assert(cell:BG_A() == 1.0)
assert(cell.fg == wm.Color.BLUE.color)
assert(cell.bg == wm.Color.BOLD_CYAN.color)
