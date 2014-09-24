local M = {}

local event = require "ng.event"
local cairo = require "ng.cairo"
local ffi = require "ffi"
local serpent = require "lib.serpent"

function M.HueBar(target, params)
	local w, h = target:GetWidth(), target:GetHeight()
	local ctx = cairo.Context(target)
	local pat = cairo.LinearPattern(0, 0, 0, h)
	pat:AddColorStopRGB(0,         1, 0, 0)
	pat:AddColorStopRGB(0.1666666, 1, 0, 1)
	pat:AddColorStopRGB(0.3333333, 0, 0, 1)
	pat:AddColorStopRGB(0.5,       0, 1, 1)
	pat:AddColorStopRGB(0.6666666, 0, 1, 0)
	pat:AddColorStopRGB(0.8333333, 1, 1, 0)
	pat:AddColorStopRGB(1,         1, 0, 0)

	ctx:SetSource(pat)
	ctx:Paint()

	ctx:SetSourceRGB(0, 0, 0)
	ctx:SetLineWidth(4)
	ctx:MoveTo(0, params.sel * h)
	ctx:LineTo(w, params.sel * h)
	ctx:Stroke()

	ctx:SetSourceRGB(1, 1, 1)
	ctx:SetLineWidth(2)
	ctx:MoveTo(0, params.sel * h)
	ctx:LineTo(w, params.sel * h)
	ctx:Stroke()

	ctx:Destroy()
end

function M.SolidColor(target, params)
	local w, h = target:GetWidth(), target:GetHeight()
	local ctx = cairo.Context(target)
	local col = params.color
	ctx:SetSourceRGB(col[1], col[2], col[3])
	ctx:Paint()

	ctx:SetAntialias(cairo.Antialias.NONE)
	ctx:SetLineWidth(1)
	ctx:SetSourceRGB(1, 1, 1)
	ctx:Rectangle(1, 1, w-1, h-1)
	ctx:Stroke()

	ctx:Destroy()
end

function M.ColorSelector(target, params)
	local w, h = target:GetWidth(), target:GetHeight()
	local ctx = cairo.Context(target)
	local pat = cairo.MeshPattern()
	local hue, sel = params.hue, params.sel
	pat:BeginPatch()
	pat:MoveTo(0, 0)
	pat:LineTo(w, 0)
	pat:LineTo(w, h)
	pat:LineTo(0, h)
	pat:SetCornerColorRGB(0, 1, 1, 1)
	pat:SetCornerColorRGB(1, hue[1], hue[2], hue[3])
	pat:SetCornerColorRGB(2, 0, 0, 0)
	pat:SetCornerColorRGB(3, 0, 0, 0)
	pat:EndPatch()

	ctx:SetSource(pat)
	ctx:Paint()

	ctx:SetSourceRGB(0, 0, 0)
	ctx:SetLineWidth(4)
	ctx:Arc(sel[1] * w, sel[2] * h, 4, 0, 2 * math.pi)
	ctx:Stroke()

	ctx:SetSourceRGB(1, 1, 1)
	ctx:SetLineWidth(2)
	ctx:Arc(sel[1] * w, sel[2] * h, 4, 0, 2 * math.pi)
	ctx:Stroke()

	ctx:Destroy()
end

function M.RegisterCallback()
	global.OnEvent = function(event_type, msg)
		local data = event.ResolveData(event_type, msg)
		if event_type ~= event.Type.TERMBOX_IMAGE_DRAW then
			return
		end

		local params_str = ffi.string(data.params)
		local ok, params = serpent.load(params_str)
		if not ok then
			print("FAIL")
			return
		end
		local func = M[params.func]
		if not func then
			print("FAIL")
			return
		end
		func(data.target, params)
	end
end

return M
