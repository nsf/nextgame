#pragma once

#include "GUI/Termbox.h"
#include "GUI/Atlas.h"

enum TermboxWindowArea {
	TBWA_NW,
	TBWA_NE,
	TBWA_SW,
	TBWA_SE,
	TBWA_NORTH,
	TBWA_WEST,
	TBWA_SOUTH,
	TBWA_EAST,
	TBWA_TERMBOX,
	TBWA_NUM,
	TBWA_OUTSIDE,
};

struct TermboxWindow {
	UniquePtr<Termbox> termbox;
	const Atlas *m_decor;
	const AtlasImage *m_decor_top_left;
	const AtlasImage *m_decor_top;
	const AtlasImage *m_decor_top_right;
	const AtlasImage *m_decor_bottom_left;
	const AtlasImage *m_decor_bottom;
	const AtlasImage *m_decor_bottom_right;
	const AtlasImage *m_decor_left;
	const AtlasImage *m_decor_right;

	NG_DELETE_COPY_AND_MOVE(TermboxWindow);
	TermboxWindow(UniquePtr<Termbox> termbox, const Atlas *decor);

	Vec4i cursor() const;
	Vec2i size_in_pixels() const;
	Vec2i termbox_offset() const;
	TermboxWindowArea classify_area(const Vec2i &p) const;
	bool is_point_in_termbox(const Vec2i &p) const;
	Vec2i pixels_to_cells(const Vec2i &p) const;

	void redraw_decor(Vector<V2T2C4> &tex) const;
	const Texture &decor_texture() const { return m_decor->texture; }
};

