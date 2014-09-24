#include "GUI/Termbox.h"
#include "Geometry/Quads.h"
#include "OS/WorkerTask.h"
#include "Script/Events.h"
#include "Script/Lua.h"
#include <utility>
#include <cairo/cairo.h>

static cairo_surface_t *resize_surface(cairo_surface_t *old, const Vec2i &size)
{
	cairo_surface_t *n = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
	cairo_surface_destroy(old);
	return n;
}

static cairo_surface_t *resize_surface_preserve_contents(cairo_surface_t *old,
	const Vec2i &size, RGB565 bg, float bg_alpha)
{
	cairo_surface_t *n = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
	cairo_t *cr = cairo_create(n);

	// TODO: we can make it more accurate without overdrawing too much
	cairo_set_source_rgba(cr, bg.R(), bg.G(), bg.B(), bg_alpha);
	cairo_paint(cr);

	cairo_set_source_surface(cr, old, 0, 0);
	cairo_pattern_t *patt = cairo_get_source(cr);
	cairo_pattern_set_extend(patt, CAIRO_EXTEND_NONE);
	cairo_paint(cr);

	cairo_destroy(cr);
	cairo_surface_destroy(old);
	return n;
}

static void draw_image(RTTIObject *msg)
{
	ETermboxImageRequest *req = ETermboxImageRequest::cast(msg);
	ScriptEventTermboxImageDraw tid;
	tid.target = req->target;
	tid.params = req->params.c_str();
	LuaVM *vm = NG_WorkerLuaWM.get();
	NG_ASSERT(vm->on_event != nullptr);
	vm->on_event(SE_TERMBOX_IMAGE_DRAW, &tid);
}

TermboxImage::TermboxImage(const Vec2i &size, int id): next(1), size(size), id(id)
{
	current = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
	next[0] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
}

TermboxImage::~TermboxImage()
{
	cairo_surface_destroy(current);
	for (cairo_surface_t *s : next)
		cairo_surface_destroy(s);
}

void TermboxImage::resize(const Vec2i &newsize)
{
	size = newsize;
	current = resize_surface_preserve_contents(current, size, bg, bg_alpha);
	if (next.length() != 0) {
		next.last() = resize_surface(next.last(), size);
		for (int i = 1; i < next.length(); i++)
			cairo_surface_destroy(next[i]);
		next.resize(1);
	}
}

void TermboxImage::clear()
{
	cairo_t *ctx = cairo_create(current);
	cairo_set_source_rgba(ctx, bg.R(), bg.G(), bg.B(), bg_alpha);
	cairo_paint(ctx);
	cairo_destroy(ctx);
}

cairo_surface_t *TermboxImage::grab_next()
{
	if (next.length() != 0) {
		cairo_surface_t *out = next.last();
		next.remove(next.length()-1);
		return out;
	}

	return cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
}

void TermboxImage::put_current(cairo_surface_t *s)
{
	next.append(current);
	current = s;
}

void TermboxImage::put_next(cairo_surface_t *s)
{
	const int w = cairo_image_surface_get_width(s);
	const int h = cairo_image_surface_get_height(s);
	if (w != size.x || h != size.y) {
		s = resize_surface(s, size);
	}

	next.append(s);
}

Vec4i Termbox::cursor() const
{
	if (!m_cursor_active || m_cursor_position == Vec2i(-1, -1))
		return {-1, -1, -1, -1};

	const Vec2i size = cell_size();
	return {
		m_cursor_position.x * size.x,
		m_cursor_position.y * size.y,
		(m_cursor_position.x+1) * size.x,
		(m_cursor_position.y+1) * size.y,
	};
}

void Termbox::update(double delta)
{
	m_local_time += delta;
	if (m_local_time > m_cursor_blink_frequency) {
		m_local_time -= m_cursor_blink_frequency;
		m_cursor_active = !m_cursor_active;
	}
}

void Termbox::update_images()
{
	for (int i = 0; i < m_images.length(); i++) {
		UniquePtr<TermboxImage> &img = m_images[i];
		if (img->flags & TIF_DRAW_REQUESTED)
			img->unused_count = 0;
		else
			img->unused_count++;

		if (img->unused_count > 50)
			m_images.remove(i--);
	}
}

void Termbox::full_redraw(Vector<V2T2C4> &col,
	Vector<V2T2C4> &tex_normal, Vector<V2T2C4> &tex_bold,
	Vector<TermboxImage*> &images)
{
	copy(m_front_buffer.sub(), m_back_buffer.sub());
	m_front_image_mask.copy_from(m_back_image_mask);
	const Vec2i cellsize = cell_size();
	for (int y = 0; y < m_size.y; ++y) {
	for (int x = 0; x < m_size.x; ++x) {
		const TermboxCell &cell = m_front_buffer[y * m_size.x + x];
		const Vec2i base = Vec2i(x, y) * cellsize;
		Vec4 fg(cell.fg.R(), cell.fg.G(), cell.fg.B(), 1.0f);
		Vec4 bg(cell.bg.R(), cell.bg.G(), cell.bg.B(), cell.bg_a());
		if (cell.attr() & TBC_INVERT)
			std::swap(fg, bg);

		append_quad(col, {base, cellsize}, bg);

		// TODO: bold attribute
		if (cell.rune() == ' ')
			continue;
		const FontGlyph *g = m_normal_font->find_glyph(cell.rune());
		if (!g)
			continue;
		const Vec2i gbase = base + Vec2i(g->offset_x, g->offset_y);
		const Vec2i gsize(g->width, g->height);
		append_quad(tex_normal, {gbase, gsize}, g->t0, g->t1, fg);
	}}
	for (auto &img : m_images) {
		if (img->flags & TIF_DRAW_REQUESTED)
			images.append(img.get());
	}
	update_images();
}

void Termbox::partial_redraw(Vector<V2T2C4> &col,
	Vector<V2T2C4> &tex_normal, Vector<V2T2C4> &tex_bold,
	Vector<TermboxImage*> &images)
{
	const Vec2i cellsize = cell_size();
	for (int y = 0; y < m_size.y; ++y) {
	for (int x = 0; x < m_size.x; ++x) {
		const Vec2i p(x, y);
		const TermboxCell &cell_back = m_back_buffer[y * m_size.x + x];
		TermboxCell &cell = m_front_buffer[y * m_size.x + x];
		if (cell == cell_back && !_is_invalidated_cell(p))
			continue;
		cell = cell_back;

		const Vec2i base = Vec2i(x, y) * cellsize;
		Vec4 fg(cell.fg.R(), cell.fg.G(), cell.fg.B(), 1.0f);
		Vec4 bg(cell.bg.R(), cell.bg.G(), cell.bg.B(), cell.bg_a());
		if (cell.attr() & TBC_INVERT)
			std::swap(fg, bg);
		append_quad(col, {base, cellsize}, bg);

		// TODO: bold attribute
		if (cell.rune() == ' ')
			continue;
		const FontGlyph *g = m_normal_font->find_glyph(cell.rune());
		if (!g)
			continue;
		const Vec2i gbase = base + Vec2i(g->offset_x, g->offset_y);
		const Vec2i gsize(g->width, g->height);
		append_quad(tex_normal, {gbase, gsize}, g->t0, g->t1, fg);
	}}
	m_front_image_mask.copy_from(m_back_image_mask);
	for (auto &img : m_images) {
		if (img->flags & TIF_DRAW_NEEDED)
			images.append(img.get());
	}
	update_images();
}

void Termbox::clear(RGB565 fg, RGB565 bg, float bg_alpha)
{
	for (auto &c : m_back_buffer)
		c = TermboxCell(' ', 0, bg_alpha, fg, bg);
	m_back_image_mask.clear();
	for (UniquePtr<TermboxImage> &img : m_images) {
		img->flags &= ~(TIF_DRAW_NEEDED | TIF_DRAW_REQUESTED);
	}
}

void Termbox::set_cursor(const Vec2i &p)
{
	if ((unsigned)p.x >= (unsigned)m_size.x) {
		m_cursor_position = {-1, -1};
		return;
	}
	if ((unsigned)p.y >= (unsigned)m_size.y) {
		m_cursor_position = {-1, -1};
		return;
	}
	m_cursor_position = p;
	m_cursor_active = true;
	m_local_time = 0.0;
}

void Termbox::set_cell(const Vec2i &p, const TermboxCell &cell)
{
	if ((unsigned)p.x >= (unsigned)m_size.x)
		return;
	if ((unsigned)p.y >= (unsigned)m_size.y)
		return;

	m_back_buffer[p.y * m_size.x + p.x] = cell;
}

void Termbox::set_image(const Vec2i &p, const Vec2i &size, int id, const char *params)
{
	m_back_image_mask.set_bit_range_2d(
		p.x, p.y, size.x, size.y, m_size.x);
	const TermboxCell &cell = m_back_buffer[p.y * m_size.x + p.x];
	int i = linear_find_if(m_images.sub(), [id](const UniquePtr<TermboxImage> &img) {
		// can't use images that were already sent to worker
		return img->id == id;
	});

	TermboxImage *img = nullptr;
	if (i != -1) {
		img = m_images[i].get();
		img->flags |= TIF_DRAW_REQUESTED;

		// quick path, if there is no need to update the image itself, don't,
		// just figure out if there is a need to redraw it and do so
		if (img->last_params == params && img->last_size == size) {
			if (img->last_position != p || img->unused_count > 0) {
				img->last_position = p;
				img->flags |= TIF_DRAW_NEEDED;
			}
			return;
		}

		if (img->last_size != size)
			img->resize(size * cell_size());
	} else {
		img = new (OrDie) TermboxImage(size * cell_size(), id);
		img->bg = cell.bg;
		img->bg_alpha = cell.bg_a();
		img->clear();
		m_images.pappend(img);
	}

	img->flags |= TIF_DRAW_NEEDED;
	img->last_params = params;
	img->last_position = p;
	img->last_size = size;

	auto data = new (OrDie) ETermboxImageRequest;
	data->sender = this;
	data->image = img;
	data->target = img->grab_next();
	data->params = params;
	data->position = p;
	data->size = size;

	EWorkerTask event;
	event.data = data;
	event.execute = draw_image;
	event.finalize = fire_and_delete_finalizer<EID_TERMBOX_IMAGE_GENERATED>;
	NG_EventManager->fire(EID_QUEUE_CPU_TASK, &event);
}

Vec2i Termbox::cell_at(const Vec2i &p) const
{
	return p / cell_size();
}

void Termbox::resize(const Vec2i &size)
{
	m_front_buffer.resize(area(size));
	Vector<TermboxCell> new_back_buffer(area(size));

	const TermboxCell clear_cell(' ', 0, 1.0f, RGB565_White(), RGB565_Black());
	for (auto &c : new_back_buffer)
		c = clear_cell;

	const int nw = size.x;
	const int nh = size.y;
	const int w = m_size.x;
	const int h = m_size.y;
	const int minh = min(nh, h);
	for (int y = 0; y < minh; y++) {
		copy(new_back_buffer.sub(y*nw, y*nw+nw),
			m_back_buffer.sub(y*w, y*w+w));
	}
	m_back_buffer = std::move(new_back_buffer);
	m_size = size;

	m_front_image_mask = BitArray(area(size));
	m_back_image_mask = BitArray(area(size));
	m_front_image_mask.clear();
	m_back_image_mask.clear();
}

bool Termbox::_is_invalidated_cell(const Vec2i &p) const
{
	const int offset = m_size.x * p.y + p.x;
	const bool a = m_front_image_mask.test_bit(offset);
	const bool b = m_back_image_mask.test_bit(offset);
	return a &~ b;
}

Termbox::Termbox(const Font *normal, const Font *bold, const Vec2i &size):
	m_normal_font(normal), m_bold_font(bold), m_size(size)
{
	m_front_buffer.resize(area(size));
	m_back_buffer.resize(area(size));

	const TermboxCell clear_cell(' ', 0, 1.0f, RGB565_White(), RGB565_Black());
	for (auto &c : m_front_buffer)
		c = clear_cell;
	for (auto &c : m_back_buffer)
		c = clear_cell;
}

NG_LUA_API Vec2i NG_Termbox_Size(Termbox *tb)
{
	return tb->size();
}

NG_LUA_API void NG_Termbox_Clear(Termbox *tb,
	RGB565 fg, RGB565 bg, float bg_alpha)
{
	tb->clear(fg, bg, bg_alpha);
}

NG_LUA_API void NG_Termbox_SetCursor(Termbox *tb, const Vec2i &p)
{
	tb->set_cursor(p-Vec2i(1));
}

NG_LUA_API void NG_Termbox_SetCell(Termbox *tb, const Vec2i &p,
	 const TermboxCell &cell)
{
	tb->set_cell(p-Vec2i(1), cell);
}

NG_LUA_API void NG_Termbox_SetImage(Termbox *tb, const Vec2i &p,
	const Vec2i &size, int id, const char *params)
{
	tb->set_image(p-Vec2i(1), size, id, params);
}

NG_LUA_API Vec2i NG_Termbox_CellSize(Termbox *tb)
{
	return tb->cell_size();
}

struct TermboxPartial {
	Termbox *termbox;
	Vec2i min;
	Vec2i max;
	Vec2i offset;
};

NG_LUA_API void NG_TermboxPartial_SetCell(TermboxPartial *tb,
	const Vec2i &p, const TermboxCell &cell)
{
	// TODO: optimization: SetCell does its own bounds check, can be avoided
	const Vec2i lp = p + tb->offset;
	if (!(tb->min <= lp && lp <= tb->max))
		return;

	tb->termbox->set_cell(lp-Vec2i(1), cell);
}

NG_LUA_API void NG_TermboxPartial_Fill(TermboxPartial *tb,
	const Vec2i &p, const Vec2i &size, const TermboxCell &cell)
{
	// TODO: optimization: rect union and unconditional fill
	for (int y = 0; y < size.y; y++) {
	for (int x = 0; x < size.x; x++) {
		NG_TermboxPartial_SetCell(tb, p + Vec2i(x, y), cell);
	}}
}

NG_LUA_API void NG_TermboxPartial_SetImage(TermboxPartial *tb,
	const Vec2i &p, const Vec2i &size, int id, const char *params)
{
	const Vec2i lp = p + tb->offset;
	if (!(tb->min <= lp && lp+(size-Vec2i(1)) <= tb->max))
		return;

	tb->termbox->set_image(lp-Vec2i(1), size, id, params);
}


