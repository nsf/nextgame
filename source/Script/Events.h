#pragma once

#include "Math/Vec.h"

// Goes to lua, that's why it starts from 1. Explicit numbering is here so that
// it's harder to move things around. Don't forget to update the lua
// counterpart!
constexpr int SE_KEY                  = 1;
constexpr int SE_MOUSE_BUTTON         = 2;
constexpr int SE_MOUSE_MOTION         = 3;
constexpr int SE_MOUSE_WHEEL          = 4;
constexpr int SE_TERMBOX_KEY          = 5;
constexpr int SE_TERMBOX_RESIZE       = 6;
constexpr int SE_TERMBOX_MOUSE_BUTTON = 7;
constexpr int SE_TERMBOX_MOUSE_MOTION = 8;
constexpr int SE_TERMBOX_MOUSE_WHEEL  = 9;
constexpr int SE_TERMBOX_MOUSE_LEAVE  = 10;
constexpr int SE_TERMBOX_IMAGE_DRAW   = 11;
constexpr int SE_QUIT                 = 12;
constexpr int SE_ENVIRONMENT_SYNC     = 13;

struct ScriptEventKey {
	bool down;
	int key;
};

struct ScriptEventMouseButton {
	bool down;
	int button;
	Vec2i position;
};

struct ScriptEventMouseMotion {
	Vec2i position;
	Vec2i relative;
};

struct ScriptEventMouseWheel {
	int delta;
	Vec2i position;
};

struct Window;

struct ScriptEventTermboxKey {
	Window *window;
	int unicode;
	int key;
	int mod;
};

struct ScriptEventTermboxResize {
	Window *window;
	Vec2i size;
};

struct ScriptEventTermboxMouseButton {
	Window *window;
	bool down;
	int button;
	Vec2i position;
	Vec2i pixel_position;
};

struct ScriptEventTermboxMouseMotion {
	Window *window;
	Vec2i position;
	Vec2i pixel_position;
};

struct ScriptEventTermboxMouseWheel {
	Window *window;
	int delta;
	Vec2i position;
	Vec2i pixel_position;
};

struct ScriptEventTermboxMouseLeave {
	Window *window;
};

typedef struct _cairo_surface cairo_surface_t;

struct ScriptEventTermboxImageDraw {
	cairo_surface_t *target;
	const char *params;
};

struct ScriptEventEnvironmentSync {
	const int *ids;
	int num;
};
