#pragma once

#include "GUI/TermboxWindow.h"
#include "Render/OpenGL.h"
#include "Math/Vec.h"

enum WindowAlignment : unsigned {
	WA_FLOATING,
	WA_NW,
	WA_NE,
	WA_SW,
	WA_SE,
};

enum WindowFlags : unsigned {
	// fully dirty, happens after resize typically, redraw everything including
	// window decorations
	WF_DIRTY         = 1 << 0,

	// partially dirty, that's when termbox part of the gui was changed and
	// needs a partial redraw
	WF_DIRTY_PARTIAL = 1 << 1,

	// toggle that flag to make window disappear
	WF_VISIBLE       = 1 << 2,

	// was resized by the user
	WF_RESIZED       = 1 << 3,

	// user can resize the window
	WF_RESIZABLE     = 1 << 4,

	// termbox gui is able to request resizes on its own, keep in mind that the
	// window can still be resizable by the user with this flag, but the logic
	// is that after user did a resize, we don't do autoresizes anymore,
	// therefore if WF_RESIZED is set, it cancels WF_AUTOSIZE
	WF_AUTO_RESIZE   = 1 << 5,
};

struct Window {
	unsigned flags;
	UniquePtr<TermboxWindow> termbox_window;
	WindowAlignment alignment;
	Vec2i position;
	Vec2i texture_size;
	Texture texture;
	Framebuffer framebuffer;
	Buffer pbo;
	double alpha;

	// Same as TermboxWindow's, but takes position into account
	TermboxWindowArea classify_area(const Vec2i &p) const;
	bool is_visible() const { return alpha > 0.0; }

	// This function can be called from lua
	void resize_in_cells(const Vec2i &new_size);

	// Returns the difference between the old size and the new size if any.
	// Keep in mind that window is able to resize itself only in steps, the
	// size of the step is the size of the termbox cell. This one is used by a
	// WM driver.
	Vec2i resize_in_pixels(const Vec2i &new_size);

	// Returns -1, -1 if pointer is not in window and/or termbox.
	Vec2i translate_pixels_to_cells(const Vec2i &p) const;

	// Does translation unconditionally
	Vec2i translate_pixels_to_termbox_pixels(const Vec2i &p) const;

	void realign(const Vec2i &wmsize);
	void update(double delta);
};
