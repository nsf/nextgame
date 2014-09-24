#pragma once

#include "Script/InterLua.h"
#include "Core/Slice.h"
#include "Core/Utils.h"
#include "Math/Vec.h"

struct SDL_KeyboardEvent;
struct SDL_TextInputEvent;
struct SDL_MouseButtonEvent;
struct SDL_MouseMotionEvent;
struct SDL_MouseWheelEvent;

struct WindowManager;
struct Window;

struct ScriptEventDispatcher {
	WindowManager *wm = nullptr;
	InterLua::Ref on_event;
	Window *last_mouse_window = nullptr;
	Window *last_mouse_button_press_windows[32] = {};
	Vec2i last_mouse_position = Vec2i(0);

	ScriptEventDispatcher(WindowManager *wm, InterLua::Ref on_event);
	NG_DELETE_COPY_AND_MOVE(ScriptEventDispatcher);

	void on_keyboard_event(const SDL_KeyboardEvent &ev);
	void on_text_input_event(const SDL_TextInputEvent &ev);
	void on_mouse_button(const SDL_MouseButtonEvent &ev);
	void on_mouse_motion(const SDL_MouseMotionEvent &ev);
	void on_mouse_wheel(const SDL_MouseWheelEvent &ev);

	void on_window_delete(Window *w);
	void on_environment_sync(Slice<const int> ids);
	void on_quit();
};
