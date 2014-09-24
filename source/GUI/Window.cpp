#include "GUI/Window.h"

TermboxWindowArea Window::classify_area(const Vec2i &p) const
{
	return termbox_window->classify_area(p - position);
}

Vec2i Window::resize_in_pixels(const Vec2i &new_size)
{
	const Vec2i decor_gap = termbox_window->size_in_pixels() -
		termbox_window->termbox->size_in_pixels();

	const Vec2i new_termbox_size = new_size - decor_gap;
	const Vec2i new_cell_size = max(new_termbox_size / termbox_window->termbox->cell_size(), Vec2i(1, 1));
	if (new_cell_size == termbox_window->termbox->size()) {
		return {0, 0};
	}

	const Vec2i required_pixel_size =
		new_cell_size * termbox_window->termbox->cell_size() + decor_gap;
	const Vec2i diff = required_pixel_size - termbox_window->size_in_pixels();

	if (required_pixel_size.x > texture_size.x ||
		required_pixel_size.y > texture_size.y)
	{
		// add a little bit more than necessary
		const Vec2i new_texture_size = required_pixel_size +
			termbox_window->termbox->cell_size() * Vec2i(8, 4);

		texture = Texture(new_texture_size.x, new_texture_size.y, GL_RGBA);
		framebuffer.attach(GL_COLOR_ATTACHMENT0, texture);
		texture_size = new_texture_size;
	}
	flags |= WF_DIRTY;
	termbox_window->termbox->resize(new_cell_size);
	return diff;
}

Vec2i Window::translate_pixels_to_cells(const Vec2i &p) const
{
	const Vec2i lp = p - position;
	if (!termbox_window->is_point_in_termbox(lp))
		return {-1, -1};

	return termbox_window->pixels_to_cells(lp);
}

Vec2i Window::translate_pixels_to_termbox_pixels(const Vec2i &p) const
{
	return p - position - termbox_window->termbox_offset();
}

void Window::realign(const Vec2i &wmsize)
{
	const Vec2i winsize = termbox_window->size_in_pixels();
	switch (alignment) {
	case WA_NW:
		position = {0, 0};
		break;
	case WA_NE:
		position.y = 0;
		position.x = wmsize.x - winsize.x;
		break;
	case WA_SW:
		position.x = 0;
		position.y = wmsize.y - winsize.y;
		break;
	case WA_SE:
		position.x = wmsize.x - winsize.x;
		position.y = wmsize.y - winsize.y;
		break;
	default:
		break;
	}
}

void Window::resize_in_cells(const Vec2i &new_size)
{
	// the window is not autoresizable
	if (not (flags & WF_AUTO_RESIZE))
		return;
	// the window was resized by the user, cancel AUTOSIZE flag
	if (flags & WF_RESIZED)
		return;
	if (new_size == termbox_window->termbox->size())
		return;

	const Vec2i decor_gap = termbox_window->size_in_pixels() -
		termbox_window->termbox->size_in_pixels();

	const Vec2i required_pixel_size =
		new_size * termbox_window->termbox->cell_size() + decor_gap;
	if (required_pixel_size.x > texture_size.x ||
		required_pixel_size.y > texture_size.y)
	{
		// add a little bit more than necessary
		const Vec2i new_texture_size = required_pixel_size +
			termbox_window->termbox->cell_size() * Vec2i(8, 4);

		texture = Texture(new_texture_size.x, new_texture_size.y, GL_RGBA);
		framebuffer.attach(GL_COLOR_ATTACHMENT0, texture);
		texture_size = new_texture_size;
	}
	flags |= WF_DIRTY;
	termbox_window->termbox->resize(new_size);
}

const double WINDOW_FADE_SPEED = 3.0;

void Window::update(double delta)
{
	termbox_window->termbox->update(delta);
	if ((flags & WF_VISIBLE) && alpha != 1.0) {
		alpha += WINDOW_FADE_SPEED * delta;
		if (alpha > 1.0)
			alpha = 1.0;
	}
	if (!(flags & WF_VISIBLE) && alpha != 0.0) {
		alpha -= WINDOW_FADE_SPEED * delta;
		if (alpha < 0.0)
			alpha = 0.0;
	}
}

NG_LUA_API Termbox *NG_Window_Termbox(Window *win)
{
	return win->termbox_window->termbox.get();
}

NG_LUA_API Vec2i NG_Window_TermboxPosition(Window *win)
{
	return -win->translate_pixels_to_termbox_pixels(Vec2i(0));
}

NG_LUA_API void NG_Window_SetDirtyPartial(Window *win)
{
	win->flags |= WF_DIRTY_PARTIAL;
}

NG_LUA_API void NG_Window_SetResizable(Window *win, bool b)
{
	if (b)
		win->flags |= WF_RESIZABLE;
	else
		win->flags &= ~WF_RESIZABLE;
}

NG_LUA_API void NG_Window_SetAutoResize(Window *win, bool b)
{
	if (b)
		win->flags |= WF_AUTO_RESIZE;
	else
		win->flags &= ~WF_AUTO_RESIZE;
}

NG_LUA_API void NG_Window_Resize(Window *win, const Vec2i &size)
{
	win->resize_in_cells(size);
}
