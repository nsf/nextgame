#pragma once

#include "GUI/TermboxCell.h"
#include "GUI/Font.h"
#include "Core/Vector.h"
#include "Core/BitArray.h"
#include "Math/Vec.h"
#include "Geometry/VertexFormats.h"
#include "OOP/RTTI.h"

typedef struct _cairo_surface cairo_surface_t;

struct Termbox;
struct TermboxImage;

struct ETermboxImageRequest : RTTIBase<ETermboxImageRequest>
{
	Termbox *sender;
	TermboxImage *image;
	cairo_surface_t *target;
	String params;
	Vec2i position;
	Vec2i size;
};

// TODO: The state machine here is quite horrible, but it works, I guess.
// Rewrite maybe. Not a top priority.
enum TermboxImageFlags {
	// Image draw is requested, but it doesn't mean that we actually need to
	// draw it. Termbox users will request it every redraw call. FullRedraw on
	// our side may just use this flag always. NOTE: Calling Clear() removes
	// this flag.
	TIF_DRAW_REQUESTED = 1 << 0,

	// By looking at the parameters we know we need to actually draw it.
	// NOTE: Calling Clear() removes this flag.
	TIF_DRAW_NEEDED = 1 << 1,
};

struct TermboxImage {
	Vector<cairo_surface_t*> next;
	cairo_surface_t *current;
	Vec2i size = Vec2i(0); // size in pixels
	int id = 0;
	int unused_count = 0;
	unsigned flags = TIF_DRAW_REQUESTED;

	RGB565 bg;
	float bg_alpha;
	String last_params;
	Vec2i last_position = Vec2i(-1);
	Vec2i last_size = Vec2i(-1); // size in cells

	NG_DELETE_COPY_AND_MOVE(TermboxImage);
	TermboxImage(const Vec2i &size, int id);
	~TermboxImage();

	void resize(const Vec2i &size);
	void clear();

	cairo_surface_t *grab_next();
	void put_current(cairo_surface_t *s);
	void put_next(cairo_surface_t *s);
};

struct Termbox {
	const Font *m_normal_font;
	const Font *m_bold_font;
	Vector<TermboxCell> m_front_buffer;
	Vector<TermboxCell> m_back_buffer;
	BitArray m_front_image_mask;
	BitArray m_back_image_mask;
	Vector<UniquePtr<TermboxImage>> m_images;
	Vec2i m_size;
	double m_local_time = 0.0;
	bool m_cursor_active = true;
	Vec2i m_cursor_position = Vec2i(-1);
	double m_cursor_blink_frequency = 0.2;

	bool _is_invalidated_cell(const Vec2i &p) const;

	NG_DELETE_COPY_AND_MOVE(Termbox);
	Termbox(const Font *normal, const Font *bold, const Vec2i &size);

	Vec2i cell_size() const {
		return {m_normal_font->glyphs[0].x_advance, m_normal_font->y_advance};
	}
	Vec2i size_in_pixels() const { return cell_size() * m_size; }
	Vec2i size() const { return m_size; }

	Vec4i cursor() const;
	void update(double delta);

	void update_images();

	// fills vectors with quads ignoring front/back buffers relative state
	void full_redraw(Vector<V2T2C4> &col, Vector<V2T2C4> &tex_normal,
		Vector<V2T2C4> &tex_bold, Vector<TermboxImage*> &images);

	// fills vectors with quads which are out of sync between
	// front/back buffers
	void partial_redraw(Vector<V2T2C4> &col, Vector<V2T2C4> &tex_normal,
		Vector<V2T2C4> &tex_bold, Vector<TermboxImage*> &images);

	void clear(RGB565 fg, RGB565 bg, float bg_alpha = 1.0f);
	void set_cursor(const Vec2i &p);
	void set_cell(const Vec2i &p, const TermboxCell &cell);
	void set_image(const Vec2i &p, const Vec2i &size, int id, const char *params);

	// translates 'p' from pixels to termbox cells
	Vec2i cell_at(const Vec2i &p) const;

	const Texture &normal_texture() const { return m_normal_font->texture; }
	const Texture &bold_texture() const { return m_bold_font->texture; }
	void resize(const Vec2i &size);
};

