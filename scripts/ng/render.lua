local M = {}

require "ng.struct"

local binding = require "ng.binding"

------------------------------------------------------------------------------
-- Game
------------------------------------------------------------------------------

binding.DefineMethods(M, [[
	void NG_Render_InvalidateAllShaders();
]])

return M
