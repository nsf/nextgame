local ngm = require "ng.math"
local abstract_widget = require "gui.abstract_widget"
local utils = require "utils"

local max, min, floor = math.max, math.min, math.floor

local function ResizeRowCol(tbl, n)
	for i = 1, n do
		if not tbl[i] then
			tbl[i] = {
				min_size = 0,
				actual_size = 0,
				preferred_size = 0,
				weight = 0,
				offset = 0,
			}
		end
	end
end

local function RowColConfigure(tbl, n, cfg)
	if n > #tbl then
		ResizeRowCol(tbl, n)
	end
	local rc = tbl[n]
	for k, v in pairs(cfg) do
		rc[k] = v
	end
end

local function RowColMassConfigure(tbl, rcs, cfg)
	for _, rci in ipairs(rcs) do
		ResizeRowCol(tbl, rci)
	end
	for _, rci in ipairs(rcs) do
		local rc = tbl[rci]
		for k, v in pairs(cfg) do
			rc[k] = v
		end
	end
end

local function StringContains(str, what)
	local c = string.byte(what, 1)
	for i = 1, #str do
		if string.byte(str, i) == c then
			return true
		end
	end
	return false
end

local function Sum(tbl, beg, size, field)
	local sum = 0
	for i = beg, beg+size-1 do
		sum = sum + tbl[i][field]
	end
	return sum
end

local function SpreadAdditionalValue(tbl, beg, size, field, amount)
	local weight_sum = 0
	local from, to = beg, beg+size-1
	for i = from, to do
		weight_sum = weight_sum + tbl[i].weight
	end

	for i = from, to do
		local rc = tbl[i]
		if i == to then
			rc[field] = rc[field] + amount
			break
		end

		local fact = rc.weight / weight_sum
		if weight_sum == 0 then
			fact = 1 / size
		end
		local add = amount * fact

		rc[field] = rc[field] + add
		amount = amount - add
		weight_sum = weight_sum - rc.weight
	end
end

local function ApplySizes(cells, cols, rows, w_func, g_field)
	-- Calculate the minimum sizes of rows/cols.
	-- 1. Iterate over 1 cell widgets (the ones which do not span), and
	--    apply their minimum size to their column/row.
	-- 2. Iterate over multicell widgets and if there is not enough space
	--    for them, apply their minimum size to their columns/rows according
	--    to their weight.
	--
	-- (This function can be applied to both min_size and preferred_size,
	-- but for preferred_size post processing is required to make sure it's
	-- greater than or equals to min_size)

	-- 1 --
	for _, c in ipairs(cells) do
		if c.cols_n == 1 then
			local rc = cols[c.col]
			rc[g_field] = max(w_func(c).x, rc[g_field])
		end
		if c.rows_n == 1 then
			local rc = rows[c.row]
			rc[g_field] = max(w_func(c).y, rc[g_field])
		end
	end

	-- 2 --
	for _, c in ipairs(cells) do
		if c.cols_n > 1 then
			local available = Sum(cols, c.col, c.cols_n, g_field)
			if available < w_func(c).x then
				local add = w_func(c).x - available
				SpreadAdditionalValue(cols, c.col, c.cols_n, g_field, add)
			end
		end
		if c.rows_n > 1 then
			local available = Sum(rows, c.row, c.rows_n, g_field)
			if available < w_func(c).y then
				local add = w_func(c).y - available
				SpreadAdditionalValue(rows, c.row, c.rows_n, g_field, add)
			end
		end
	end
end

local function ApplySticky(cursize, pos, size, sticky)
	local contains = StringContains
	local newsize = ngm.Vec2i(cursize)
	local newpos = ngm.Vec2i(0, 0)
	if contains(sticky, "N") and contains(sticky, "S") then
		newpos.y = pos.y
		newsize.y = size.y
	elseif contains(sticky, "N") and not contains(sticky, "S") then
		newpos.y = pos.y
	elseif not contains(sticky, "N") and contains(sticky, "S") then
		newpos.y = pos.y + size.y - cursize.y
	else
		newpos.y = pos.y + (size.y - cursize.y) / 2
	end
	if contains(sticky, "W") and contains(sticky, "E") then
		newpos.x = pos.x
		newsize.x = size.x
	elseif contains(sticky, "W") and not contains(sticky, "E") then
		newpos.x = pos.x
	elseif not contains(sticky, "W") and contains(sticky, "E") then
		newpos.x = pos.x + size.x - cursize.x
	else
		newpos.x = pos.x + (size.x - cursize.x) / 2
	end
	return newpos, newsize
end

local function WeightSortDescending(a, b)
	return b.weight < a.weight
end

-- Grow table. The logic is rather simple. We need to spread 'amount' amongst
-- rows/cols and we know that we can do that exactly. The question which is left
-- to us is who gets what. I think a fair algorithm would look like this:
--   1. Loop through rows/cols.
--   2. Add an appropriate share of the amount to each row/col.
--        amount * (weight / weight_sum)
--   3. Keep track of total amount given.
--   4. When loop is finished and if the total amount given is less than total
--      amount to give. Sort rows/cols by weight (from largest to smallest) and
--      start giving 1 to each row/col until there are no more amount to give.
--
local function GrowRC(rc, amount, weight_sum)
	local amount_given = 0
	for _, c in ipairs(rc) do
		if c.weight ~= 0 then
			local add = floor(amount * (c.weight / weight_sum))
			c.actual_size = c.actual_size + add
			amount_given = amount_given + add
		end
	end

	if amount_given == amount then
		return
	end

	local rc_copy = {}
	for i = 1, #rc do
		rc_copy[i] = rc[i]
	end

	table.sort(rc_copy, WeightSortDescending)
	while amount_given < amount do
		for _, c in ipairs(rc_copy) do
			c.actual_size = c.actual_size + 1
			amount_given = amount_given + 1
			if amount_given >= amount then
				break
			end
		end
	end
end

-- Shrink table. This one is a bit more tricky, because we need to shrink the
-- table by 'amount', but it's unclear if we can do that. I think the fair
-- algorithm would look like this:
--   1. While there is amount to take and there are valid nodes
--      (with actual_size > min_size), perform an iteration.
--   2. Loop through all rows/cols.
--   3. Try to take an appropriate share of the amount from each row/col. But
--      not more than possible.
--        amount * (weight / weight_sum)
--   4. If we took all from that row/col, subtract its weight from weight sum
--      for the next iteration. This row/col will not participate in this
--      algorithm anymore.
--   5. For each iteration, keep track of the amount taken. If it's zero, but
--      there is still amount to take and valid rows/cols, proceed to step 6,
--      which is a recovery from infinite loop step.
--   6. Now we go through all valid rows/cols and take 1, repeat that until no
--      valid rows/cols are left or there is no more amount to take.
--
-- Random note 1: Of course since we change the weight sum with every iteration,
-- we need to change the amount to take with every iteration as well.
--
-- Random note 2: Probably a better solution on step 6 would be to sort all
-- valid rows/cols by their weight and start taking 1 from the most "heavy" one.
local function ShrinkRC(rc, amount, weight_sum)
	while amount > 0 and weight_sum > 0.000001 do
		local next_weight_sum = weight_sum
		local amount_taken = 0
		for _, c in ipairs(rc) do
			if c.weight ~= 0 and c.actual_size ~= c.min_size then
				local can_sub = c.actual_size - c.min_size
				local want_sub = floor(amount * (c.weight / weight_sum))
				local sub = min(want_sub, can_sub)

				c.actual_size = c.actual_size - sub
				amount_taken = amount_taken + sub
				if sub == can_sub then
					next_weight_sum = next_weight_sum - c.weight
				end
			end
		end
		weight_sum = next_weight_sum
		amount = amount - amount_taken

		-- this happens only if we're stuck in infinite loop
		if amount_taken == 0 then
			break
		end
	end

	if amount == 0 or weight_sum < 0.000001 then
		return
	end

	local have_valid_rcs = true
	while amount > 0 and have_valid_rcs do
		have_valid_rcs = false
		for _, c in ipairs(rc) do
			if c.weight ~= 0 and c.actual_size ~= c.min_size then
				c.actual_size = c.actual_size - 1
				amount = amount - 1
				if amount == 0 then
					break
				end
				have_valid_rcs = true
			end
		end
	end
end

local function CalculateOffsets(tbl)
	local offset = 0
	for _, c in ipairs(tbl) do
		c.offset = offset
		offset = offset + c.actual_size
	end
end

local function CellMinSize(c)
	return c.widget.min_size
end

local function CellPreferredSize(c)
	return c.widget:PreferredSize() +
		ngm.Vec2i(c.ipadx, c.ipady) +
		ngm.Vec2i(c.padx[1] + c.padx[2], c.pady[1] + c.pady[2])
end

local function CellPreferredSizeWOExternalPadding(c)
	return c.widget:PreferredSize() +
		ngm.Vec2i(c.ipadx, c.ipady)
end

local function GridMassConfigure(grid, array, custom, cfg)
	for _, wname in ipairs(array) do
		local self = grid:GetCell(wname)
		if self then
			for k, v in pairs(cfg) do
				if custom and custom[k] then
					custom[k](self, v)
				else
					self[k] = v
				end
			end
		end
	end
end

------------------------------------------------------------------------

local W = abstract_widget:extends()

function W:__init()
	W.super.__init(self)

	self.anchor = "NW"

	-- cell contains:
	--   .widget
	--   .col
	--   .row
	--   .cols_n (span if > 1)
	--   .rows_n (span if > 1)
	--   .sticky
	--   .ipadx
	--   .ipady
	--   .padx (two numbers, left and right)
	--   .pady (two numbers, top and bottom)
	self.cells = {}

	-- col/row contains (keep in mind, there are no Vec2i fields, all 1D):
	--   .min_size
	--   .actual_size
	--   .preferred_size
	--   .weight
	--   .offset
	self.cols = {}
	self.rows = {}
	self.cell_custom = {
		padx = function(self, value)
			if type(value) == "number" then
				self.padx = {value, value}
			elseif type(value) == "table" then
				self.padx = value
			end
		end,
		pady = function(self, value)
			if type(value) == "number" then
				self.pady = {value, value}
			elseif type(value) == "table" then
				self.pady = value
			end
		end,
	}
end

function W:AddWidget(widget, col, row)
	self.cells[#self.cells+1] = {
		widget = widget,
		col = col,
		row = row,
		cols_n = 1,
		rows_n = 1,
		sticky = "",
		ipadx = 0,
		ipady = 0,
		padx = {0, 0},
		pady = {0, 0},
	}
end

function W:ConfigureRow(...)
	local n = select("#", ...)
	if n == 2 then
		local n, cfg = select(1, ...)
		RowColConfigure(self.rows, n, cfg)
		return
	end
	local rows = {...}
	local cfg = rows[#rows]
	rows[#rows] = nil
	return RowColMassConfigure(self.rows, rows, cfg)
end

function W:ConfigureColumn(...)
	local n = select("#", ...)
	if n == 2 then
		local n, cfg = select(1, ...)
		RowColConfigure(self.cols, n, cfg)
		return
	end
	local cols = {...}
	local cfg = cols[#cols]
	cols[#cols] = nil
	return RowColMassConfigure(self.cols, cols, cfg)
end

function W:WidgetUnderPoint(p, reason)
	for _, c in ipairs(self.cells) do
		local w = c.widget
		local min = w.position
		local max = w.position + w.size - ngm.Vec2i(1, 1)
		if min <= p and p <= max then
			return w:WidgetUnderPoint(p, reason)
		end
	end
	return nil
end

function W:GetCell(slave)
	slave = self.gui:ExpandAliasMaybe(slave)
	for _, c in ipairs(self.cells) do
		if c.widget.name == slave then
			return c
		end
	end
	return nil
end

function W:Configure(slave, cfg)
	if type(slave) == "table" then
		if #slave > 0 then
			return GridMassConfigure(self, slave, self.cell_custom, cfg)
		else
			abstract_widget.Configure(self, slave)
		end
		return
	end

	assert(type(slave) == "string")

	local array = utils.StringFields(slave)
	if #array > 0 then
		return GridMassConfigure(self, array, self.cell_custom, cfg)
	end

	local cell = self:GetCell(self.gui:ExpandAliasMaybe(slave))
	if not cell then
		return nil
	end

	local cell_custom = self.cell_custom
	for k, v in pairs(cfg) do
		if cell_custom[k] then
			cell_custom[k](cell, v)
		else
			cell[k] = v
		end
	end
end

function W:RecalculateSizes()
	for _, c in ipairs(self.cells) do
		c.widget:RecalculateSizes()
	end

	-- Calculate the required number of rows and cols.
	local cols_n, rows_n = 0, 0
	for _, c in ipairs(self.cells) do
		cols_n = max(c.col + c.cols_n, cols_n)
		rows_n = max(c.row + c.rows_n, rows_n)
	end
	ResizeRowCol(self.cols, cols_n)
	ResizeRowCol(self.rows, rows_n)

	-- Calculate the 'min_size'.
	ApplySizes(self.cells, self.cols, self.rows,
		CellMinSize, "min_size")

	-- Minimum size of the grid is the sum of all minimum sizes of all
	-- cols/rows.
	self.min_size.x = Sum(self.cols, 1, #self.cols, "min_size")
	self.min_size.y = Sum(self.rows, 1, #self.rows, "min_size")

	-- Calculate the 'actual_size'.
	for _, rc in ipairs(self.cols) do rc.actual_size = 0 end
	for _, rc in ipairs(self.rows) do rc.actual_size = 0 end
	ApplySizes(self.cells, self.cols, self.rows,
		CellPreferredSize, "actual_size")

	-- Make sure 'actual_size' isn't less than 'min_size'.
	for _, c in ipairs(self.cols) do
		c.actual_size = max(c.actual_size, c.preferred_size, c.min_size)
	end
	for _, c in ipairs(self.rows) do
		c.actual_size = max(c.actual_size, c.preferred_size, c.min_size)
	end

	-- At the moment 'actual_size' equals to preferred size of the
	-- grid. Shrink/grow happens in Reconfigure.
	self.preferred_size.x = Sum(self.cols, 1, #self.cols, "actual_size")
	self.preferred_size.y = Sum(self.rows, 1, #self.rows, "actual_size")
end

function W:ReconfigurePart2(pos, size)
	local cursize = ngm.Vec2i(
		Sum(self.cols, 1, #self.cols, "actual_size"),
		Sum(self.rows, 1, #self.rows, "actual_size")
	)

	local newpos, newsize = ApplySticky(cursize, pos, size, self.anchor)
	CalculateOffsets(self.cols)
	CalculateOffsets(self.rows)

	for _, c in ipairs(self.cells) do
		local w = c.widget
		local p = newpos + ngm.Vec2i(
			self.cols[c.col].offset,
			self.rows[c.row].offset
		)
		local s = ngm.Vec2i(
			Sum(self.cols, c.col, c.cols_n, "actual_size"),
			Sum(self.rows, c.row, c.rows_n, "actual_size")
		)
		local pref_size = CellPreferredSizeWOExternalPadding(c)
		p = p + ngm.Vec2i(c.padx[1], c.pady[1])
		s = s - ngm.Vec2i(c.padx[1] + c.padx[2], c.pady[1] + c.pady[2])
		local np, ns = ApplySticky(
			ngm.Vec2iMin(s, ngm.Vec2iMax(pref_size, w.min_size)),
			p, s, c.sticky)
		ns = ngm.Vec2iMax(ns, w.min_size)
		w:Reconfigure(np, ns)
	end
end

function W:Reconfigure(pos, size)
	W.super.Reconfigure(self, pos, size)

	local cols_weight = Sum(self.cols, 1, #self.cols, "weight")
	local rows_weight = Sum(self.rows, 1, #self.rows, "weight")
	if cols_weight == 0 and rows_weight == 0 then
		-- Nothing to shrink/grow, let's proceed to placing widgets.
		self:ReconfigurePart2(pos, size)
		return
	end

	-- In case if available size doesn't match our preferred size we need to
	-- shrink/grow rows/cols.
	if cols_weight ~= 0 then
		if self.size.x > self.preferred_size.x then
			GrowRC(self.cols, self.size.x - self.preferred_size.x, cols_weight)
		elseif self.size.x < self.preferred_size.x then
			ShrinkRC(self.cols, self.preferred_size.x - self.size.x, cols_weight)
		end
	end
	if rows_weight ~= 0 then
		if self.size.y > self.preferred_size.y then
			GrowRC(self.rows, self.size.y - self.preferred_size.y, rows_weight)
		elseif self.size.y < self.preferred_size.y then
			ShrinkRC(self.rows, self.preferred_size.y - self.size.y, rows_weight)
		end
	end

	self:ReconfigurePart2(pos, size)
end

function W:OnDraw(tb)
	for _, c in ipairs(self.cells) do
		local w = c.widget
		w:Draw(tb:Partial(w.position, w.position+w.size-ngm.Vec2i(1, 1)))
	end
end

function W:FindCellAt(col, row)
	local p = ngm.Vec2i(col, row)
	for _, c in ipairs(self.cells) do
		local cpmin = ngm.Vec2i(c.col, c.row)
		local cpmax = cpmin + ngm.Vec2i(c.cols_n-1, c.rows_n-1)
		if cpmin <= p and p <= cpmax then
			return c
		end
	end
	return nil
end

local function errorf(...)
	error(string.format(...))
end

function W:Layout(spec)
	local lines = {}
	spec:gsub("([^\n]+)", function(m) lines[#lines+1] = m end)
	self.cells, self.rows, self.cols = {}, {}, {}

	local col = 1
	local row = 1
	for _, line in ipairs(lines) do
		for item in string.gmatch(line, "%S+") do
			local c = self:FindCellAt(col, row)
			if c then
				-- cell can be already in this place only in one case
				-- (. is current cell):
				-- +----------+
				-- | widget - |
				-- | ^      . |
				-- +----------+
				-- and we require '^' to be there
				if item ~= "^" then
					errorf("Grid: '^' expected in (col: %d, row: %d)", col, row)
				end
			else
				if item == "x" or item == "X" then
					-- do nothing, it's an official way to skip a cell
				elseif item == "-" then
					local c = self:FindCellAt(col-1, row)
					if not c then
						errorf("Grid: widget expected in (col: %d, row: %d)",
							col-1, row)
					end
					c.cols_n = c.cols_n + 1
				elseif item == "^" then
					local c = self:FindCellAt(col, row-1)
					if not c then
						errorf("Grid: widget expected in (col: %d, row: %d)",
							col, row-1)
					end
					c.rows_n = c.rows_n + 1
				else
					local w = self.gui:Get(item)
					if not w then
						print("Grid: widget '"..item.."' not found, skipping")
					else
						self:AddWidget(w, col, row)
					end
				end
			end
			col = col + 1
		end
		col = 1
		row = row + 1
	end
	self.gui:QueueRecalc()
end

return W
