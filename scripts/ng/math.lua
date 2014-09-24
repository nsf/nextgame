local M = {}

require "ng.struct"

local ffi = require "ffi"
local min, max = math.min, math.max

------------------------------------------------------------------------
-- Vec2
------------------------------------------------------------------------

local Vec2
local Vec2_mt = {
	__add = function(a, b)
		return Vec2(a.x + b.x, a.y + b.y)
	end,
	__sub = function(a, b)
		return Vec2(a.x - b.x, a.y - b.y)
	end,
	__mul = function(a, b)
		return Vec2(a.x * b.x, a.y * b.y)
	end,
	__div = function(a, b)
		return Vec2(a.x / b.x, a.y / b.y)
	end,
	__unm = function(a)
		return Vec2(-a.x, -a.y)
	end,
	__eq = function(a, b)
		return a.x == b.x and a.y == b.y
	end,
	__lt = function(a, b)
		return a.x < b.x and a.y < b.y
	end,
	__le = function(a, b)
		return a.x <= b.x and a.y <= b.y
	end,
	__tostring = function(v)
		return "Vec2("..v.x..", "..v.y..")"
	end,
}
Vec2 = ffi.metatype("Vec2", Vec2_mt)
M.Vec2 = ffi.typeof("Vec2")

function M.Vec2Min(a, b)
	return Vec2(min(a.x, b.x), min(a.y, b.y))
end

function M.Vec2Max(a, b)
	return Vec2(max(a.x, b.x), max(a.y, b.y))
end

------------------------------------------------------------------------
-- Vec2i
------------------------------------------------------------------------

local Vec2i
local Vec2i_mt = {
	__add = function(a, b)
		return Vec2i(a.x + b.x, a.y + b.y)
	end,
	__sub = function(a, b)
		return Vec2i(a.x - b.x, a.y - b.y)
	end,
	__mul = function(a, b)
		return Vec2i(a.x * b.x, a.y * b.y)
	end,
	__div = function(a, b)
		return Vec2i(a.x / b.x, a.y / b.y)
	end,
	__unm = function(a)
		return Vec2i(-a.x, -a.y)
	end,
	__eq = function(a, b)
		return a.x == b.x and a.y == b.y
	end,
	__lt = function(a, b)
		return a.x < b.x and a.y < b.y
	end,
	__le = function(a, b)
		return a.x <= b.x and a.y <= b.y
	end,
	__tostring = function(v)
		return "Vec2i("..v.x..", "..v.y..")"
	end,
}
Vec2i = ffi.metatype("Vec2i", Vec2i_mt)
M.Vec2i = ffi.typeof("Vec2i")

function M.Vec2iMin(a, b)
	return Vec2i(min(a.x, b.x), min(a.y, b.y))
end

function M.Vec2iMax(a, b)
	return Vec2i(max(a.x, b.x), max(a.y, b.y))
end

------------------------------------------------------------------------
-- Vec3
------------------------------------------------------------------------

local Vec3
local Vec3_mt = {
	__add = function(a, b)
		return Vec3(a.x + b.x, a.y + b.y, a.z + b.z)
	end,
	__sub = function(a, b)
		return Vec3(a.x - b.x, a.y - b.y, a.z - b.z)
	end,
	__mul = function(a, b)
		return Vec3(a.x * b.x, a.y * b.y, a.z * b.z)
	end,
	__div = function(a, b)
		return Vec3(a.x / b.x, a.y / b.y, a.z / b.z)
	end,
	__unm = function(a)
		return Vec3(-a.x, -a.y, -a.z)
	end,
	__eq = function(a, b)
		return a.x == b.x and a.y == b.y and a.z == b.z
	end,
	__lt = function(a, b)
		return a.x < b.x and a.y < b.y and a.z < b.z
	end,
	__le = function(a, b)
		return a.x <= b.x and a.y <= b.y and a.z <= b.z
	end,
	__tostring = function(v)
		return "Vec3("..v.x..", "..v.y..", "..v.z..")"
	end,
}
Vec3 = ffi.metatype("Vec3", Vec3_mt)
M.Vec3 = ffi.typeof("Vec3")

function M.Vec3Lerp(a, b, v)
	return Vec3(
		a.x * (1 - v) + b.x * v,
		a.y * (1 - v) + b.y * v,
		a.z * (1 - v) + b.z * v)
end

------------------------------------------------------------------------
-- Vec3i
------------------------------------------------------------------------

local Vec3i
local Vec3i_mt = {
	__add = function(a, b)
		return Vec3i(a.x + b.x, a.y + b.y, a.z + b.z)
	end,
	__sub = function(a, b)
		return Vec3i(a.x - b.x, a.y - b.y, a.z - b.z)
	end,
	__mul = function(a, b)
		return Vec3i(a.x * b.x, a.y * b.y, a.z * b.z)
	end,
	__div = function(a, b)
		return Vec3i(a.x / b.x, a.y / b.y, a.z / b.z)
	end,
	__unm = function(a)
		return Vec3i(-a.x, -a.y, -a.z)
	end,
	__eq = function(a, b)
		return a.x == b.x and a.y == b.y and a.z == b.z
	end,
	__lt = function(a, b)
		return a.x < b.x and a.y < b.y and a.z < b.z
	end,
	__le = function(a, b)
		return a.x <= b.x and a.y <= b.y and a.z <= b.z
	end,
	__tostring = function(v)
		return "Vec3i("..v.x..", "..v.y..", "..v.z..")"
	end,
}
Vec3i = ffi.metatype("Vec3i", Vec3i_mt)
M.Vec3i = ffi.typeof("Vec3i")

------------------------------------------------------------------------
-- Rectangle Intersection
--
-- Rectangle is defined by two Vec2i: min and max.
------------------------------------------------------------------------

-- 1D line intersection helper for 'RectangleIntersection'
local function LineIntersection(a_min, a_max, b_min, b_max)
	if a_min > b_max then
		return nil, nil
	end
	if a_max < b_min then
		return nil, nil
	end

	return max(a_min, b_min), min(a_max, b_max)
end

function M.RectangleIntersection(a_min, a_max, b_min, b_max)
	local x_min, x_max = LineIntersection(a_min.x, a_max.x, b_min.x, b_max.x)
	local y_min, y_max = LineIntersection(a_min.y, a_max.y, b_min.y, b_max.y)
	if x_min == nil or y_min == nil then
		return Vec2i(b_min.x, b_min.y), Vec2i(b_min.x-1, b_min.y-1), false
	end
	return Vec2i(x_min, y_min), Vec2i(x_max, y_max), true
end

------------------------------------------------------------------------
-- Utility Functions
------------------------------------------------------------------------

function M.Clamp(value, min, max)
	if value < min then
		return min
	elseif value > max then
		return max
	end
	return value
end

return M
