local M = {}

require "ng.struct"

local binding = require "ng.binding"

------------------------------------------------------------------------------
-- Game
------------------------------------------------------------------------------

binding.DefineMethods(M, [[
	void NG_Game_EnableGUI(bool b);
	Window *NG_Game_AddWindow(const Vec2i &pos, unsigned alignment, bool decorate);
	void NG_Game_RemoveWindow(Window *win);
	void NG_Game_RaiseWindow(Window *win);
	Vec2i NG_Game_CursorPosition();
	void NG_Game_Info();
	void NG_Game_InfoDebug();
	void NG_Game_Do();
	void NG_Game_RotateCamera(float x, float y);
	void NG_Game_Jump();
	void NG_Game_ToggleGravity();
]])

return M
