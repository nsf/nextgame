local utf8 = require "ng.utf8"
local ngm = require "ng.math"
local wm = require "ng.wm"
local event = require "ng.event"
local class = require "lib.30log"
local utils = require "utils"
local gui_consts = require "gui.consts"
local gui_utils = require "gui.utils"

local min, max, floor = math.min, math.max, math.floor

local M = {}

-- Pull these from ng module, however, please don't use them in this file. They
-- are convenience imports for users of gui. So that they don't have to pull the
-- ng module themselves.
utils.MergeTablesSelectively(M, ngm, {"Vec2i"})
utils.MergeTablesSelectively(M, wm, {
	"Color",
	"TermboxCell",
	"WindowAlignment",
})
utils.MergeTablesSelectively(M, gui_utils, {"InheritStyle"})
utils.MergeTables(M, gui_consts)
local space = string.byte(" ")

------------------------------------------------------------------------
-- GUI
------------------------------------------------------------------------

M.GUI = class()

function M.GUI:__init(window)
	self.window = window
	self.termbox = window:Termbox()
	self.aliases = {}
	self.widgets = {}
	self.size = self.termbox:Size()
	self.top = nil
	self.recalc = true
	self.redraw = true
	self.active = nil
	self.hovered = nil
	self.dragging = nil
end

-- recalc does redraw implicitly
function M.GUI:QueueRecalc()
	self.recalc = true
end

function M.GUI:QueueRedraw()
	self.redraw = true
end

function M.GUI:AddAlias(alias, target)
	self.aliases[alias] = target
end

function M.GUI:RemoveAlias(alias)
	self.aliases[alias] = nil
end

function M.GUI:WithAlias(alias, target, func)
	self:AddAlias(alias, target)
	func()
	self:RemoveAlias(alias)
end

function M.GUI:ExpandAliasMaybe(...)
	local args = {...}
	local out = {}
	for _, name in ipairs(args) do
		out[#out+1] = self:ExpandOneAliasMaybe(name)
	end
	return unpack(out)
end

function M.GUI:ExpandOneAliasMaybe(name)
	local i = name:find(".", 1, true)
	local alias = name
	if i then
		alias = name:sub(1, i-1)
	end

	local a = self.aliases[alias]
	if not a then
		return name
	end
	if #alias == #name then
		return a
	end
	return a.."."..name:sub(i+1)
end

function M.GUI:Get(name)
	return self.widgets[self:ExpandAliasMaybe(name)]
end

function M.GUI:OnKey(ev)
	if self.active then
		self.active:OnKey(ev)
		self:RedrawMaybe()
	end
end

function M.GUI:OnMouseButton(ev)
	if not self.top then
		return
	end
	local w = self.top:WidgetUnderPoint(ev.position, gui_consts.Reason.MOUSE_BUTTON)
	if not w then
		if self.active then
			self.active:Deactivate()
		end
		self.active = nil
	else
		if ev.down then
			self.dragging = w
		end
		if self.active ~= w then
			if self.active then
				self.active:Deactivate()
			end
			self.active = w
			self.active:Activate()
		end
		w:OnMouseButton(ev)
	end
	if not ev.down then
		self.dragging = nil
	end
	self:RedrawMaybe()
end

function M.GUI:OnMouseWheel(ev)
	if not self.top then
		return
	end
	local w = self.top:WidgetUnderPoint(ev.position, gui_consts.Reason.MOUSE_WHEEL)
	if not w then
		if self.active then
			self.active:Deactivate()
		end
		self.active = nil
	else
		if self.active ~= w then
			if self.active then
				self.active:Deactivate()
			end
			self.active = w
			self.active:Activate()
		end
		w:OnMouseWheel(ev)
	end
	self:RedrawMaybe()
end

function M.GUI:OnMouseMotion(ev)
	if not self.top then
		return
	end
	if self.dragging then
		self.dragging:OnDrag(ev)
	end
	local w = self.top:WidgetUnderPoint(ev.position, gui_consts.Reason.MOUSE_MOTION)
	if not w then
		if self.hovered then
			self.hovered:OnHoverLeave()
		end
		self.hovered = nil
	else
		if self.hovered ~= w then
			if self.hovered then
				self.hovered:OnHoverLeave()
			end
			self.hovered = w
			self.hovered:OnHoverEnter()
		end
		w:OnMouseMotion(ev)
	end
	self:RedrawMaybe()
end

function M.GUI:OnMouseLeave(ev)
	if self.hovered then
		self.hovered:OnHoverLeave()
	end
	self.hovered = nil
	self:RedrawMaybe()
end

function M.GUI:OnResize(ev)
	-- TODO: perhaps we shouldn't set size here, it's being set during
	-- RedrawMaybe anyway
	self.size = ev.size
	self.recalc = true
	self:RedrawMaybe()
end

function M.GUI:Cursor()
	if self.active then
		local w = self.active
		local cursor = w:Cursor()
		if cursor.x == 0 and cursor.y == 0 then
			return ngm.Vec2i(0, 0)
		end
		cursor = cursor + w.virtual_offset
		if not (w.draw_min <= cursor and cursor <= w.draw_max) then
			return ngm.Vec2i(0, 0)
		end
		return cursor
	end
	return ngm.Vec2i(0, 0)
end

function M.GUI:RedrawMaybe()
	if not self.window then
		return
	end

	if not self.top then
		return
	end

	if self.recalc then
		self.top:RecalculateSizes()
		self.window:Resize(self.top.preferred_size)
		self.size = self.termbox:Size()
		self.top:Reconfigure(ngm.Vec2i(1, 1), self.size)
	end

	if self.recalc or self.redraw then
		self.termbox:Clear(wm.Color.WHITE, wm.Color.BLACK, 1)
		self.top:Draw(self.termbox:Partial())
		self.window:SetDirtyPartial()
	end

	self.recalc = false
	self.redraw = false

	self.termbox:SetCursor(self:Cursor())
end

function M.GUI:SetTop(name)
	local w = self:Get(name)
	if not w then
		return
	end
	self.top = w
end

local function RegisterWidgets(list)
	for _, val in ipairs(list) do
		local wname, import_path = val[1], val[2]
		M[wname] = require(import_path)
		M.GUI[wname] = function(self, name, cfg)
			local w = M[wname]()
			w.name = self:ExpandAliasMaybe(name)
			w.gui = self
			self.widgets[w.name] = w
			if cfg then w:Configure(cfg) end
			return w
		end
	end
end

M.AbstractWidget = require "gui.abstract_widget"
RegisterWidgets({
	{"Grid",        "gui.grid"},
	{"Label",       "gui.label"},
	{"Filler",      "gui.filler"},
	{"Button",      "gui.button"},
	{"CheckButton", "gui.check_button"},
	{"RadioButton", "gui.radio_button"},
	{"EditBox",     "gui.edit_box"},
	{"Slider",      "gui.slider"},
	{"ListBox",     "gui.list_box"},
	{"ProgressBar", "gui.progress_bar"},
	{"Image",       "gui.image"},
	{"ScrollBox",   "gui.scroll_box"},
	{"Expander",    "gui.expander"},
	{"Frame",       "gui.frame"},
	{"SpinBox",     "gui.spin_box"},
	{"ComboBox",    "gui.combo_box"},
})

------------------------------------------------------------------------
-- WindowManager
------------------------------------------------------------------------

local game = require "ng.game"
local modal = nil
local winguis = {}

function M.AddWindow(pos, alignment, decorate)
	local w = game.AddWindow(pos, alignment, decorate)
	local g = M.GUI(w)
	winguis[tostring(w)] = g
	return g
end

function M.RemoveWindow(g)
	winguis[g.window] = nil
	game.RemoveWindow(g.window)
	g.window = nil
end

local event_dispatch_table = {
	[event.Type.TERMBOX_KEY] = "OnKey",
	[event.Type.TERMBOX_MOUSE_BUTTON] = "OnMouseButton",
	[event.Type.TERMBOX_MOUSE_MOTION] = "OnMouseMotion",
	[event.Type.TERMBOX_MOUSE_WHEEL] = "OnMouseWheel",
	[event.Type.TERMBOX_MOUSE_LEAVE] = "OnMouseLeave",
	[event.Type.TERMBOX_RESIZE] = "OnResize",
}

function M.CursorPosition()
	return game.CursorPosition()
end

function M.MakeModal(g)
	if modal then
		M.RemoveWindow(modal)
	end

	modal = g
end

function M.DispatchEvent(event_type, data)
	local method = event_dispatch_table[event_type]
	if not method then return end

	local winstr = tostring(data.window)
	local g = winguis[winstr]
	if not g then
		return
	end

	if modal and modal ~= g and event_type ~= event.Type.TERMBOX_RESIZE then
		if event_type == event.Type.TERMBOX_MOUSE_BUTTON then
			M.RemoveWindow(modal)
			modal = nil
		end
		return
	end

	g[method](g, data)
end

------------------------------------------------------------------------
return M
