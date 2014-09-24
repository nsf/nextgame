local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local consts = require "gui.consts"
local utils = require "utils"

local min = math.min

local M = {}

------------------------------------------------------------------------
-- Conditions
------------------------------------------------------------------------

function M.DefineWidgetConditions(module, tbl)
	module.Conditions = {redraw = {}, recalc = {}}
	for k, v in pairs(tbl) do
		local conditions = {}
		if type(v) == "string" then
			for _, f in ipairs(utils.StringFields(v)) do
				conditions[f] = true
			end
		elseif type(v) == "table" then
			conditions = v
		end
		module.Conditions[k] = conditions
	end
end

------------------------------------------------------------------------
-- Text Drawing
------------------------------------------------------------------------

-- style should include:
--  * fg - foreground color (RGB565)
--  * bg - background color (RGB565)
--  * bga - background alpha (float: [0, 1])
function M.DrawTextSimple(position, text, tb, style)
	local fg, bg, bga = style.fg, style.bg, style.bga
	if type(text) == "string" then
		local len = utf8.RuneCount(text)
		local beg = 1
		for i = 1, len do
			local p = position + ngm.Vec2i(i-1, 0)
			local rune, size = utf8.DecodeRune(text, beg)
			tb:SetCell(p, wm.TermboxCell(rune, 0, bga, fg, bg))
			beg = beg + size
		end
	elseif type(text) == "table" then
		local len = #text
		for i = 1, len do
			local p = position + ngm.Vec2i(i-1, 0)
			tb:SetCell(p, wm.TermboxCell(text[i], 0, bga, fg, bg))
		end
	end
end

local function DrawFirstNRunes(pos, n, text, style, tb, beg)
	local p = ngm.Vec2i(pos)
	if beg == nil then
		beg = 1
	end
	while n > 0 do
		local rune, size = utf8.DecodeRune(text, beg)
		tb:SetCell(p, wm.TermboxCell(rune, 0, style.bga, style.fg, style.bg))
		beg = beg + size
		p.x = p.x + 1
		n = n - 1
	end
end

local function DrawLastNRunes(pos, n, text, style, tb)
	local p = ngm.Vec2i(pos)
	local E = #text
	while n > 0 do
		local rune, size = utf8.DecodeLastRune(text, E)
		tb:SetCell(p, wm.TermboxCell(rune, 0, style.bga, style.fg, style.bg))
		E = E - size
		p.x = p.x - 1
		n = n - 1
	end
end

local function SkipNRunes(text, n)
	if n <= 0 then
		return 1
	end

	local pos = 1
	while n > 0 do
		local _, size = utf8.DecodeRune(text, pos)
		pos = pos + size
		n = n - 1
	end
	return pos
end

-- style should include:
--  * fg - foreground color (RGB565)
--  * bg - background color (RGB565)
--  * bga - background alpha (float: [0, 1])
--  * align - text alignment
--  * ellipsis - ellipsis character (unicode code point)
--  * center_ellipsis - boolean
function M.DrawText(position, size, text, tb, style)
	local e = wm.TermboxCell(style.ellipsis, 0,
		style.bga, style.fg, style.bg)
	local textlen = utf8.RuneCount(text)

	tb:Fill(position, size, wm.TermboxCell(string.byte(" "), 0,
		style.bga, wm.Color.WHITE, style.bg))

	local y = position.y + size.y / 2
	local x = position.x
	local n = textlen
	if n > size.x then
		-- if text is longer than amount of space we have, draw ellipsis
		n = size.x - 1
		if style.center_ellipsis then
			-- center_ellipsis if set overrides alignment to Center
			tb:SetCell(ngm.Vec2i(x+size.x/2, y), e)
		else
			if style.align == consts.Alignment.LEFT then
				tb:SetCell(ngm.Vec2i(x+n, y), e)
			elseif style.align == consts.Alignment.CENTER then
				tb:SetCell(ngm.Vec2i(x, y), e)
				tb:SetCell(ngm.Vec2i(x+n, y), e)
				n = n - 1
			elseif style.align == consts.Alignment.RIGHT then
				tb:SetCell(ngm.Vec2i(x, y), e)
			end
		end
	end

	if n <= 0 then
		-- no more place to actually draw anything
		return
	end

	if style.center_ellipsis and textlen ~= n then
		-- text doesn't fit, and center_ellipsis is set, override align to
		-- Center and draw the text
		local firsthalf = size.x / 2
		local secondhalf = size.x - firsthalf - 1
		DrawFirstNRunes(ngm.Vec2i(x, y), firsthalf, text, style, tb)
		DrawLastNRunes(ngm.Vec2i(x + size.x - 1, y), secondhalf, text, style, tb)
		return
	end

	-- now to usual path, draw text according to alignment
	if style.align == consts.Alignment.LEFT then
		DrawFirstNRunes(ngm.Vec2i(x, y), n, text, style, tb)
	elseif style.align == consts.Alignment.CENTER then
		if textlen == n then
			DrawFirstNRunes(ngm.Vec2i(x + (size.x - n) / 2, y), n, text, style, tb)
		else
			local mid = (textlen - n) / 2
			local beg = SkipNRunes(text, mid)
			DrawFirstNRunes(ngm.Vec2i(x+1, y), n, text, style, tb, beg)
		end
	elseif style.align == consts.Alignment.RIGHT then
		DrawLastNRunes(ngm.Vec2i(x + size.x - 1, y), n, text, style, tb)
	end
end

------------------------------------------------------------------------
-- EditBox Functionality
------------------------------------------------------------------------
-- Unlike text drawing, in order to make a working editbox you'll have to store
-- a context, it consists of:
--  * Text buffer.
--  * Visual offset of the editbox window. The amount of characters you skip
--    from the left in order to display the portion where the cursor is.
--  * Cursor offset in bytes. You'll need that for changing the contents of the
--    text buffer.
--  * Visual cursor offset. This one exists because text is stored as utf8.
--    Visual offset is used to draw the cursor.
--
-- All of these context members are expected to be in the 'context' table, with
-- the following names:
--  * text
--  * voffset
--  * cursor_boffset
--  * cursor_voffset

local function AdjustEditBoxVOffset(context, size)
	local width = size.x
	local max_h_threshold = (width - 1) / 2

	-- preferred horizontal threshold
	local ht = 5
	if ht > max_h_threshold then
		ht = max_h_threshold
	end

	local threshold = width - 1
	if context.voffset ~= 0 then
		threshold = width - ht
	end
	if context.cursor_voffset - context.voffset >= threshold then
		context.voffset = context.cursor_voffset + (ht - width + 1)
	end

	if context.voffset ~= 0 and context.cursor_voffset - context.voffset < ht then
		context.voffset = context.cursor_voffset - ht
		if context.voffset < 0 then
			context.voffset = 0
		end
	end
end

-- The 'context' is described above, the 'zero_voffset' is a boolean and it
-- forces 'voffset' to be 0.
--
-- Style should include:
--  * fg - foreground color (RGB565)
--  * bg - background color (RGB565)
--  * bga - background alpha (float: [0, 1])
--  * arrow_left - left arrow character (unicode code point)
--  * arrow_right - right arrow character (unicode code point)
function M.DrawEditBox(pos, size, context, tb, style, zero_voffset)
	local fg, bg, bga = style.fg, style.bg, style.bga

	AdjustEditBoxVOffset(context, size)
	tb:Fill(pos, size, wm.TermboxCell(
		string.byte(" "), 0, bga, wm.Color.WHITE, bg))

	local voffset = context.voffset
	if zero_voffset then
		voffset = 0
	end

	local w = size.x
	local t = context.text
	local b = 1
	local x = 0
	while true do
		local rx = x - voffset
		if b > #t then
			break
		end

		if rx >= w then
			tb:SetCell(pos + ngm.Vec2i(w - 1, 0),
				wm.TermboxCell(style.arrow_right, 0, bga, fg, bg))
			break
		end

		local rune, size = utf8.DecodeRune(t, b)
		if rx >= 0 then
			tb:SetCell(pos + ngm.Vec2i(rx, 0),
				wm.TermboxCell(rune, 0, bga, fg, bg))
		end
		x = x + 1
		b = b + size
	end

	if voffset ~= 0 then
		tb:SetCell(pos, wm.TermboxCell(style.arrow_left, 0, bga, fg, bg))
	end
end

function M.EditBoxCursor(pos, context)
	return pos + ngm.Vec2i(context.cursor_voffset - context.voffset, 0)
end

local function MoveCursorTo(context, boffset)
	context.cursor_boffset = boffset
	context.cursor_voffset = utf8.RuneCount(context.text, 1, boffset)
end

local function RuneUnderCursor(context)
	return utf8.DecodeRune(context.text, context.cursor_boffset+1)
end

local function RuneBeforeCursor(context)
	return utf8.DecodeLastRune(context.text, context.cursor_boffset)
end

local function TextInsert(str, where, what)
	local b = str:sub(1, where-1)
	local e = str:sub(where)
	return b..what..e
end

local function TextRemove(str, from, to)
	local b = str:sub(1, from-1)
	local e = str:sub(to+1)
	return b..e
end

function M.OnEditBoxKey(ctx, ev)
	if ev.key == consts.Key.LEFT then
		if ctx.cursor_boffset == 0 then
			return
		end
		local _, size = RuneBeforeCursor(ctx)
		MoveCursorTo(ctx, ctx.cursor_boffset - size)
	elseif ev.key == consts.Key.RIGHT then
		if ctx.cursor_boffset == #ctx.text then
			return
		end
		local _, size = RuneUnderCursor(ctx)
		MoveCursorTo(ctx, ctx.cursor_boffset + size)
	elseif ev.key == consts.Key.BACKSPACE then
		if ctx.cursor_boffset == 0 then
			return
		end

		local _, size = RuneBeforeCursor(ctx)
		MoveCursorTo(ctx, ctx.cursor_boffset - size)
		ctx.text = TextRemove(ctx.text,
			ctx.cursor_boffset + 1, ctx.cursor_boffset + size)
	elseif ev.key == consts.Key.DELETE then
		if ctx.cursor_boffset == #ctx.text then
			return
		end
		local _, size = RuneUnderCursor(ctx)
		ctx.text = TextRemove(ctx.text,
			ctx.cursor_boffset + 1, ctx.cursor_boffset + size)
	elseif ev.key == consts.Key.HOME then
		MoveCursorTo(ctx, 0)
	elseif ev.key == consts.Key.END then
		MoveCursorTo(ctx, #ctx.text)
	else
		if ev.unicode == 0 then
			return
		end

		local str = utf8.EncodeRune(ev.unicode)
		ctx.text = TextInsert(ctx.text, ctx.cursor_boffset+1, str)
		MoveCursorTo(ctx, ctx.cursor_boffset + #str)
	end
end

------------------------------------------------------------------------
-- InheritStyle
------------------------------------------------------------------------

function M.InheritStyle(style, newstyle)
	return setmetatable(newstyle, { __index = style })
end

------------------------------------------------------------------------
-- Scrolling
------------------------------------------------------------------------

-- The function calculates an integer offset for scrolling parameters.
-- 'window_size': the available size of the widget
-- 'size': the total virutal size of its contents (for example if it's a
--         listbox then the height == number of listbox items)
-- 'scroll_*': value from 0.0 to 1.0, in most cases taken from the scroll bar (0
--             is top/left, 1 is bottom/right)
function M.ScrollOffset(window_size, size, scroll_x, scroll_y)
	local offx, offy = 0, 0

	if size.x > window_size.x then
		local avail = size.x - window_size.x
		offx = min(avail, (avail+1) * scroll_x)
	end
	if size.y > window_size.y then
		local avail = size.y - window_size.y
		offy = min(avail, (avail+1) * scroll_y)
	end
	return ngm.Vec2i(offx, offy)
end

return M
