local ngm = require "ng.math"
local wm = require "ng.wm"
local utf8 = require "ng.utf8"
local abstract_widget = require "gui.abstract_widget"
local serpent = require "lib.serpent"
local utils = require "utils"

local next_id = 1
local function GetNextID()
	local out = next_id
	next_id = next_id + 1
	return out
end

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)
	self.id = GetNextID()
	self.min_size = ngm.Vec2i(1, 1)
	self.params = {}
	self.params_string = ""
	self.on_click = nil
	self.on_drag = nil
	self.custom = {
		params = function(widget, value)
			utils.MergeTables(widget.params, value)
			widget.params_string = serpent.serialize(widget.params, {compact=true})
		end,
		on_paint = function(widget, value)
			widget.on_click = value
			widget.on_drag = value
		end
	}
end

W.Conditions = {
	recalc = {},
	redraw = {
		params = true,
	},
}

function W:OnDraw(tb)
	tb:SetImage(self.position, self.size, self.id, self.params_string)
end

function W:OnMouseButtonDown(ev)
	if not self.on_click then
		return
	end
	local cell_size = self.gui.termbox:CellSize()
	local psize = self.size * cell_size
	local rel = ev.pixel_position - (self:VisualPosition() - ngm.Vec2i(1, 1)) * cell_size
	self.on_click(ngm.Clamp(rel.x / psize.x, 0, 1), ngm.Clamp(rel.y / psize.y, 0, 1))
end

function W:OnDrag(ev)
	if not self.on_drag then
		return
	end
	local cell_size = self.gui.termbox:CellSize()
	local psize = self.size * cell_size
	local rel = ev.pixel_position - (self:VisualPosition() - ngm.Vec2i(1, 1)) * cell_size
	self.on_drag(ngm.Clamp(rel.x / psize.x, 0, 1), ngm.Clamp(rel.y / psize.y, 0, 1))
end

return W
