local M = {}

local ffi = require "ffi"

ffi.cdef [[
	typedef struct LuaVM LuaVM;

	// ----==== MATH ====----
	typedef struct {
		float x, y;
	} Vec2;

	typedef struct {
		int x, y;
	} Vec2i;

	typedef struct {
		float x, y, z;
	} Vec3;

	typedef struct {
		int x, y, z;
	} Vec3i;

	typedef struct {
		float x, y, z, w;
	} Vec4;

	typedef struct {
		int x, y, z, w;
	} Vec4i;

	typedef struct {
		float x, y, z, w;
	} Quat;
	// ----==== WM (GUI) ====----
	typedef struct {
		uint16_t color;
	} RGB565;

	typedef struct {
		uint32_t rune_attr;
		// use uint16_t here, luajit doesn't support jit compilation of init of
		// nested structs
		uint16_t fg;
		uint16_t bg;
	} TermboxCell;

	typedef struct Termbox Termbox;

	typedef struct {
		Termbox *termbox;
		// use ints here, because luajit doesn't jit compile nested structs
		int min_x;
		int min_y;
		int max_x;
		int max_y;
		int offset_x;
		int offset_y;
	} TermboxPartial;

	typedef struct Window Window;

	typedef struct {
		bool down;
		int key;
	} ScriptEventKey;

	typedef struct {
		bool down;
		int button;
		Vec2i position;
	} ScriptEventMouseButton;

	typedef struct {
		Vec2i position;
		Vec2i relative;
	} ScriptEventMouseMotion;

	typedef struct {
		int delta;
		Vec2i position;
	} ScriptEventMouseWheel;

	typedef struct {
		Window *window;
		int unicode;
		int key;
		int mod;
	} ScriptEventTermboxKey;

	typedef struct {
		Window *window;
		Vec2i size;
	} ScriptEventTermboxResize;

	typedef struct {
		Window *window;
		bool down;
		int button;
		Vec2i position;
		Vec2i pixel_position;
	} ScriptEventTermboxMouseButton;

	typedef struct {
		Window *window;
		Vec2i position;
		Vec2i pixel_position;
	} ScriptEventTermboxMouseMotion;

	typedef struct {
		Window *window;
		int delta;
		Vec2i position;
		Vec2i pixel_position;
	} ScriptEventTermboxMouseWheel;

	typedef struct {
		Window *window;
	} ScriptEventTermboxMouseLeave;

	typedef struct _cairo_surface cairo_surface_t;

	typedef struct {
		cairo_surface_t *target;
		const char *params;
	} ScriptEventTermboxImageDraw;

	typedef struct {
		const int *ids;
		int num;
	} ScriptEventEnvironmentSync;
]]

return M
