#include "GUI/TermboxWindow.h"
#include "Math/Rect.h"
#include "Geometry/Quads.h"
#include <cstdio>

static void window_rects(Rect *out, const TermboxWindow *w)
{
	const Rect win(w->termbox_offset(), w->termbox->size_in_pixels());
	out[TBWA_TERMBOX] = win;
	if (w->m_decor == nullptr) {
		out[TBWA_NW] = Rect(win.x,    win.y,    0, 0);
		out[TBWA_NE] = Rect(win.x2(), win.y,    0, 0);
		out[TBWA_SW] = Rect(win.x,    win.y2(), 0, 0);
		out[TBWA_SE] = Rect(win.x2(), win.y2(), 0, 0);
		out[TBWA_NORTH] = Rect(win.x,    win.y,    win.w, 0);
		out[TBWA_SOUTH] = Rect(win.x,    win.y2(), win.w, 0);
		out[TBWA_WEST]  = Rect(win.x,    win.y,    0,     win.h);
		out[TBWA_EAST]  = Rect(win.x2(), win.y,    0,     win.h);
		return;
	}

	out[TBWA_NW] = Rect(
		win.x - w->m_decor_left->width, win.y - w->m_decor_top_left->height,
		w->m_decor_top_left->width, w->m_decor_top_left->height
	);
	out[TBWA_NE] = Rect(
		win.x2() + w->m_decor_right->width - w->m_decor_top_right->width,
		win.y - w->m_decor_top_right->height,
		w->m_decor_top_right->width, w->m_decor_top_right->height
	);
	out[TBWA_SW] = Rect(
		win.x - w->m_decor_left->width, win.y2(),
		w->m_decor_bottom_left->width, w->m_decor_bottom_left->height
	);
	out[TBWA_SE] = Rect(
		win.x2() + w->m_decor_right->width - w->m_decor_bottom_right->width,
		win.y2(),
		w->m_decor_bottom_right->width, w->m_decor_bottom_right->height
	);
	out[TBWA_NORTH] = Rect(
		win.x - w->m_decor_left->width + w->m_decor_top_left->width,
		win.y - w->m_decor_top->height,
		win.w + (w->m_decor_left->width + w->m_decor_right->width) - (w->m_decor_top_left->width + w->m_decor_top_right->width),
		w->m_decor_top->height
	);
	out[TBWA_SOUTH] = Rect(
		win.x - w->m_decor_left->width + w->m_decor_bottom_left->width,
		win.y2(),
		win.w + (w->m_decor_left->width + w->m_decor_right->width) - (w->m_decor_bottom_left->width + w->m_decor_bottom_right->width),
		w->m_decor_bottom->height
	);
	out[TBWA_WEST] = Rect(
		win.x - w->m_decor_left->width, win.y, w->m_decor_left->width, win.h
	);
	out[TBWA_EAST] = Rect(
		win.x2(), win.y, w->m_decor_right->width, win.h
	);
}

static void adjust_rects(Rect *out)
{
	const int min_corner_value = 20;
	// NORTH WEST
	if (out[TBWA_NW].w < min_corner_value) {
		out[TBWA_NW].w = min_corner_value;
	}
	if (out[TBWA_NW].h < min_corner_value) {
		out[TBWA_NW].h = min_corner_value;
	}
	// NORTH EAST
	if (out[TBWA_NE].w < min_corner_value) {
		const int diff = min_corner_value - out[TBWA_NE].w;
		out[TBWA_NE].w = min_corner_value;
		out[TBWA_NE].x -= diff;
	}
	if (out[TBWA_NE].h < min_corner_value) {
		out[TBWA_NE].h = min_corner_value;
	}
	// SOUTH WEST
	if (out[TBWA_SW].w < min_corner_value) {
		out[TBWA_SW].w = min_corner_value;
	}
	if (out[TBWA_SW].h < min_corner_value) {
		const int diff = min_corner_value - out[TBWA_SW].h;
		out[TBWA_SW].h = min_corner_value;
		out[TBWA_SW].y -= diff;
	}
	// SOUTH EAST
	if (out[TBWA_SE].w < min_corner_value) {
		const int diff = min_corner_value - out[TBWA_SE].w;
		out[TBWA_SE].w = min_corner_value;
		out[TBWA_SE].x -= diff;
	}
	if (out[TBWA_SE].h < min_corner_value) {
		const int diff = min_corner_value - out[TBWA_SE].h;
		out[TBWA_SE].h = min_corner_value;
		out[TBWA_SE].y -= diff;
	}

	const int min_side_value = 10;
	// NORTH
	if (out[TBWA_NORTH].h < min_side_value) {
		out[TBWA_NORTH].h = min_side_value;
	}
	// SOUTH
	if (out[TBWA_SOUTH].h < min_side_value) {
		const int diff = min_side_value - out[TBWA_SOUTH].h;
		out[TBWA_SOUTH].h = min_side_value;
		out[TBWA_SOUTH].y -= diff;
	}
	// WEST
	if (out[TBWA_WEST].w < min_side_value) {
		out[TBWA_WEST].w = min_side_value;
	}
	// EAST
	if (out[TBWA_EAST].w < min_side_value) {
		const int diff = min_side_value - out[TBWA_EAST].w;
		out[TBWA_EAST].w = min_side_value;
		out[TBWA_EAST].x -= diff;
	}
}

Vec4i TermboxWindow::cursor() const
{
	const Vec4i cursor = termbox->cursor();
	if (cursor.x == -1)
		return cursor;

	const Vec2i tboff = termbox_offset();
	const Vec4i offset(tboff.x, tboff.y, tboff.x, tboff.y);
	return cursor + offset;
}

Vec2i TermboxWindow::size_in_pixels() const
{
	const Vec2i tbsize = termbox->size_in_pixels();
	if (!m_decor)
		return tbsize;
	return {
		m_decor_left->width + m_decor_right->width + tbsize.x,
		m_decor_top->height + m_decor_bottom->height + tbsize.y,
	};
}

Vec2i TermboxWindow::termbox_offset() const
{
	if (!m_decor)
		return {0, 0};
	return {m_decor_left->width, m_decor_top->height};
}

TermboxWindowArea TermboxWindow::classify_area(const Vec2i &p) const
{
	Rect rects[TBWA_NUM];
	window_rects(rects, this);
	adjust_rects(rects);
	for (int i = 0; i < TBWA_NUM; i++) {
		if (is_point_inside(p, rects[i]))
			return (TermboxWindowArea)i;
	}
	return TBWA_OUTSIDE;
}

bool TermboxWindow::is_point_in_termbox(const Vec2i &p) const
{
	const Rect tbwin(termbox_offset(), termbox->size_in_pixels());
	return is_point_inside(p, tbwin);
}

Vec2i TermboxWindow::pixels_to_cells(const Vec2i &p) const
{
	return (p - termbox_offset()) / termbox->cell_size();
}

void TermboxWindow::redraw_decor(Vector<V2T2C4> &tex) const
{
	if (!m_decor)
		return;

	Rect rects[TBWA_NUM];
	window_rects(rects, this);
	append_quad_span(tex, 0, rects[TBWA_NORTH],
		{m_decor_top->width, m_decor_top->height}, m_decor_top->t0, m_decor_top->t1);
	append_quad_span(tex, 0, rects[TBWA_SOUTH],
		{m_decor_bottom->width, m_decor_bottom->height}, m_decor_bottom->t0, m_decor_bottom->t1);
	append_quad_span(tex, 1, rects[TBWA_WEST],
		{m_decor_left->width, m_decor_left->height}, m_decor_left->t0, m_decor_left->t1);
	append_quad_span(tex, 1, rects[TBWA_EAST],
		{m_decor_right->width, m_decor_right->height}, m_decor_right->t0, m_decor_right->t1);
	append_quad(tex, rects[TBWA_NW], m_decor_top_left->t0, m_decor_top_left->t1);
	append_quad(tex, rects[TBWA_NE], m_decor_top_right->t0, m_decor_top_right->t1);
	append_quad(tex, rects[TBWA_SW], m_decor_bottom_left->t0, m_decor_bottom_left->t1);
	append_quad(tex, rects[TBWA_SE], m_decor_bottom_right->t0, m_decor_bottom_right->t1);
}

TermboxWindow::TermboxWindow(UniquePtr<Termbox> termbox, const Atlas *decor):
	termbox(std::move(termbox)), m_decor(decor)
{
	if (decor) {
		m_decor_top_left = decor->get("window_top_left.png");
		m_decor_top = decor->get("window_top.png");
		m_decor_top_right = decor->get("window_top_right.png");
		m_decor_bottom_left = decor->get("window_bottom_left.png");
		m_decor_bottom = decor->get("window_bottom.png");
		m_decor_bottom_right = decor->get("window_bottom_right.png");
		m_decor_left = decor->get("window_left.png");
		m_decor_right = decor->get("window_right.png");
	}
}
