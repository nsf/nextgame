#pragma once

#include "GUI/WindowManager.h"
#include <SDL2/SDL.h>

struct WindowManagerCursorSet {
	SDL_Cursor *arrow;
	SDL_Cursor *ns;
	SDL_Cursor *we;
	SDL_Cursor *nwse;
	SDL_Cursor *nesw;
	SDL_Cursor *move;
};

// High-level helper for WindowManager, it controls various aspect of it such
// as: cursor selection, window movement/resize. E.g. logic where it decides
// whether to grab a window on a click or not.

struct WindowManagerDriver {
	enum Action {
		NONE,
		MOVE,
		RESIZE,
	};

	WindowManager *m_wm;
	WindowManagerCursorSet m_cursor_set;
	Window *m_active = nullptr;
	SDL_Cursor *m_last_cursor = nullptr;
	Action m_action = NONE;
	Vec2i m_drag_offset;
	Vec2i m_resize_origin;
	Vec2i m_resize_mask;
	Vec2i m_position_adjust_mask;

	WindowManagerDriver(WindowManager *wm,
		const WindowManagerCursorSet &cursor_set);

	void set_default_cursor();
	void update_cursor(const Vec2i &p);

	void on_mouse_motion(const SDL_MouseMotionEvent &ev, Window **resized);
	void on_mouse_button(const SDL_MouseButtonEvent &ev);
};
