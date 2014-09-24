local check_button = require "gui.check_button"
local utils = require "gui.utils"

local W = check_button:extends()

function W:__init()
	W.super.__init(self)
	self.group = {self}
	self.custom = {
		group = function(self, value)
			value[#value+1] = self
			self.group = value
		end,
	}
end

W.DefaultStyle = utils.InheritStyle(check_button.DefaultStyle, {
	button = {"(", ")"},
})

function W:OnMouseButton(ev)
	if not ev.down then
		return
	end
	if not self.checked and self.group.on_change then
		for i, v in ipairs(self.group) do
			if v == self then
				self.group.on_change(i)
				break
			end
		end
	end
	self.checked = true
	for _, w in ipairs(self.group) do
		if w ~= self then
			w.checked = false
		end
	end
	self.gui:QueueRedraw()
end

return W
