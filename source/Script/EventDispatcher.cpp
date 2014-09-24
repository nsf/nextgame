#include "Script/EventDispatcher.h"
#include "Script/Events.h"
#include "Core/UTF8.h"
#include "GUI/WindowManager.h"
#include <SDL2/SDL.h>

ScriptEventDispatcher::ScriptEventDispatcher(WindowManager *wm, InterLua::Ref on_event):
	wm(wm), on_event(on_event)
{
}

void ScriptEventDispatcher::on_keyboard_event(const SDL_KeyboardEvent &ev)
{
	ScriptEventKey sev;
	if (wm->active && ev.type == SDL_KEYDOWN) {
		ScriptEventTermboxKey tev;
		tev.window = wm->active_window;

		SDL_Keycode k = ev.keysym.sym;
		if (
			(ev.keysym.mod & KMOD_CTRL) ||
			k == SDLK_BACKSPACE ||
			k == SDLK_TAB ||
			k == SDLK_RETURN ||
			k == SDLK_ESCAPE ||
			k == SDLK_DELETE ||
			(k > SDLK_DELETE &&
			 !(k >= SDLK_LCTRL && k <= SDLK_RGUI)
			)
		) {
			tev.unicode = 0;
			tev.key = k;
			tev.mod = ev.keysym.mod;
			on_event(SE_TERMBOX_KEY, &tev);
		}
	}

	if (ev.repeat > 0)
		return;
	sev.down = ev.type == SDL_KEYDOWN;
	sev.key = ev.keysym.sym;
	on_event(SE_KEY, &sev);
}

void ScriptEventDispatcher::on_text_input_event(const SDL_TextInputEvent &ev)
{
	ScriptEventTermboxKey tev;
	SDL_Keymod mod = SDL_GetModState();
	auto sr = UTF8::decode_rune(ev.text);
	tev.window = wm->active_window;
	tev.unicode = sr.rune;
	tev.key = SDLK_UNKNOWN;
	tev.mod = mod;
	on_event(SE_TERMBOX_KEY, &tev);
}

void ScriptEventDispatcher::on_mouse_button(const SDL_MouseButtonEvent &ev)
{
	const bool down = ev.type == SDL_MOUSEBUTTONDOWN;
	const Vec2i p(ev.x, ev.y);
	ScriptEventMouseButton sev;
	ScriptEventTermboxMouseButton tev;
	Window *w;
	Vec2i lpc;

	do {
		w = wm->window_at(p);
		if (down) {
			last_mouse_button_press_windows[ev.button] = w;
		} else {
			w = last_mouse_button_press_windows[ev.button];
		}
		if (!w)
			break;

		lpc = w->translate_pixels_to_cells(p);
		if (down && lpc.x == -1)
			break;

		tev.window = w;
		tev.down = ev.type == SDL_MOUSEBUTTONDOWN;
		tev.button = ev.button;
		tev.position = lpc + Vec2i(1);
		tev.pixel_position = w->translate_pixels_to_termbox_pixels(p);
		on_event(SE_TERMBOX_MOUSE_BUTTON, &tev);
	} while (0);

	sev.down = ev.type == SDL_MOUSEBUTTONDOWN;
	sev.button = ev.button;
	sev.position = Vec2i(ev.x, ev.y);
	on_event(SE_MOUSE_BUTTON, &sev);
}

void ScriptEventDispatcher::on_mouse_motion(const SDL_MouseMotionEvent &ev)
{
	ScriptEventMouseMotion sev;
	ScriptEventTermboxMouseMotion tev;

	const Vec2i p = last_mouse_position = Vec2i(ev.x, ev.y);
	Window *w;
	Vec2i lpc;

	do {
		w = wm->window_at(p);
		// 'last_mouse_window' is valid and cursor is not on top of it anymore
		// => send leave event
		if (last_mouse_window && w != last_mouse_window) {
			ScriptEventTermboxMouseLeave wev;
			wev.window = last_mouse_window;
			on_event(SE_TERMBOX_MOUSE_LEAVE, &wev);
		}
		last_mouse_window = w;
		if (!w)
			break;

		lpc = w->translate_pixels_to_cells(p);
		if (lpc.x == -1)
			break;

		tev.window = w;
		tev.position = lpc + Vec2i(1);
		tev.pixel_position = w->translate_pixels_to_termbox_pixels(p);
		on_event(SE_TERMBOX_MOUSE_MOTION, &tev);
	} while (0);

	sev.position = Vec2i(ev.x, ev.y);
	sev.relative = Vec2i(ev.xrel, ev.yrel);
	on_event(SE_MOUSE_MOTION, &sev);
}

void ScriptEventDispatcher::on_mouse_wheel(const SDL_MouseWheelEvent &ev)
{
	ScriptEventMouseWheel sev;
	ScriptEventTermboxMouseWheel tev;
	const Vec2i p = last_mouse_position;
	Window *w;
	Vec2i lpc;

	do {
		w = wm->window_at(p);
		if (!w)
			break;

		lpc = w->translate_pixels_to_cells(p);
		if (lpc.x == -1)
			break;

		tev.window = w;
		tev.delta = ev.y;
		tev.position = lpc + Vec2i(1);
		tev.pixel_position = w->translate_pixels_to_termbox_pixels(p);
		on_event(SE_TERMBOX_MOUSE_WHEEL, &tev);
	} while (0);

	sev.delta = ev.y;
	sev.position = Vec2i(ev.x, ev.y);
	on_event(SE_MOUSE_WHEEL, &sev);
}

void ScriptEventDispatcher::on_window_delete(Window *w)
{
	if (last_mouse_window == w)
		last_mouse_window = nullptr;
	for (Window *&pw : last_mouse_button_press_windows) {
		if (pw == w)
			pw = nullptr;
	}
}

void ScriptEventDispatcher::on_environment_sync(Slice<const int> ids)
{
	if (ids.length == 0)
		return;

	ScriptEventEnvironmentSync sev;
	sev.ids = ids.data;
	sev.num = ids.length;
	on_event(SE_ENVIRONMENT_SYNC, &sev);
}

void ScriptEventDispatcher::on_quit()
{
	on_event(SE_QUIT, nullptr);
}
