#include "GUI/WindowManagerDriver.h"
#include "Script/Events.h"
#include "Core/UTF8.h"

static Vec2i resize_mask(TermboxWindowArea area)
{
	switch (area) {
	case TBWA_NW:
	case TBWA_NE:
	case TBWA_SW:
	case TBWA_SE:
		return {1, 1};
	case TBWA_NORTH:
	case TBWA_SOUTH:
		return {0, 1};
	case TBWA_WEST:
	case TBWA_EAST:
		return {1, 0};
	default:
		die("never happens");
		return {0, 0};
	}
}

static Vec2i position_adjust_mask(TermboxWindowArea area)
{
	switch (area) {
	case TBWA_SE:
	case TBWA_SOUTH:
	case TBWA_EAST:
		return {0, 0};
	case TBWA_NW:
		return {1, 1};
	case TBWA_NORTH:
	case TBWA_NE:
		return {0, 1};
	case TBWA_WEST:
	case TBWA_SW:
		return {1, 0};
	default:
		die("never happens");
		return {0, 0};
	}
}

void WindowManagerDriver::set_default_cursor()
{
	SDL_Cursor *cursor = m_cursor_set.arrow;
	if (m_last_cursor != cursor) {
		SDL_SetCursor(cursor);
		m_last_cursor = cursor;
	}
}

void WindowManagerDriver::update_cursor(const Vec2i &p)
{
	SDL_Cursor *cursor = m_cursor_set.arrow;
	Window *win = m_wm->window_at(p);
	if (win != nullptr &&
		win->alignment == WA_FLOATING &&
		win->flags & WF_RESIZABLE)
	{
		switch (win->classify_area(p)) {
		case TBWA_SOUTH:
			cursor = m_cursor_set.ns;
			break;
		case TBWA_WEST:
		case TBWA_EAST:
			cursor = m_cursor_set.we;
			break;
		case TBWA_NW:
		case TBWA_SE:
			cursor = m_cursor_set.nwse;
			break;
		case TBWA_NE:
		case TBWA_SW:
			cursor = m_cursor_set.nesw;
			break;
		default:
			break;
		}
	}

	if (m_last_cursor != cursor) {
		SDL_SetCursor(cursor);
		m_last_cursor = cursor;
	}
}

void WindowManagerDriver::on_mouse_motion(const SDL_MouseMotionEvent &ev,
	Window **resized)
{
	if (!m_wm->active)
		return;

	const Vec2i p(ev.x, ev.y);
	switch (m_action) {
	case MOVE:
		m_active->position = p + m_drag_offset;
		if (m_last_cursor != m_cursor_set.move) {
			SDL_SetCursor(m_cursor_set.move);
			m_last_cursor = m_cursor_set.move;
		}
		break;
	case RESIZE: {
		const Vec2i mod = Vec2i(1) - Vec2i(2) * m_position_adjust_mask;
		const Vec2i diff = (p - m_resize_origin) * m_resize_mask * mod;
		const Vec2i change = m_active->resize_in_pixels(
			m_active->termbox_window->size_in_pixels() + diff);
		if (change != Vec2i(0, 0) && resized) {
			m_active->flags |= WF_RESIZED;
			*resized = m_active;
		}
		m_resize_origin += change * mod;
		m_active->position += change * mod * m_position_adjust_mask;
		break;
	}
	default:
		update_cursor(p);
		break;
	}
}

void WindowManagerDriver::on_mouse_button(const SDL_MouseButtonEvent &ev)
{
	if (!m_wm->active)
		return;

	const Vec2i p(ev.x, ev.y);
	if (ev.type == SDL_MOUSEBUTTONUP) {
		if (ev.button == 1) {
			m_action = NONE;
			update_cursor(p);
		}
		return;
	}

	Window *win = m_wm->window_at(p);
	if (!win) {
		m_wm->active_window = nullptr;
		return;
	}

	m_wm->raise(win);

	if (ev.button != 1)
		return;
	if (win->alignment != WA_FLOATING)
		return;

	const TermboxWindowArea area = win->classify_area(p);
	switch (area) {
	case TBWA_NW:
	case TBWA_NE:
	case TBWA_SW:
	case TBWA_SE:
	case TBWA_WEST:
	case TBWA_SOUTH:
	case TBWA_EAST:
		if (not (win->flags & WF_RESIZABLE))
			break;
		m_active = win;
		m_action = RESIZE;
		m_resize_origin = p;
		m_resize_mask = resize_mask(area);
		m_position_adjust_mask = position_adjust_mask(area);
		break;
	case TBWA_NORTH:
		m_active = win;
		m_action = MOVE;
		m_drag_offset = win->position - p;
		break;
	default:
		break;
	}
}

WindowManagerDriver::WindowManagerDriver(WindowManager *wm,
	const WindowManagerCursorSet &cursor_set):
		m_wm(wm), m_cursor_set(cursor_set)
{
}
