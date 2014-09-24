#pragma once

#include "GUI/Window.h"
#include "Core/Vector.h"
#include "Math/Vec.h"
#include "Render/OpenGL.h"
#include "Core/UniquePtr.h"
#include "OOP/RTTI.h"

struct WindowManager : RTTIBase<WindowManager> {
	Vector<V2T2C4> col;
	Vector<V2T2C4> tex_normal;
	Vector<V2T2C4> tex_bold;
	Vector<TermboxImage*> images;
	VertexArray quad_stream;
	Vector<UniquePtr<Window>> windows;
	Vec2i size;
	Window *active_window = nullptr;
	bool active = false;

	NG_DELETE_COPY_AND_MOVE(WindowManager);
	WindowManager(const Vec2i &size);
	~WindowManager();

	// takes ownership over 'win'
	Window *add_window(UniquePtr<TermboxWindow> win,
		WindowAlignment alignment, const Vec2i &pos);
	void remove_window(Window *win);

	void resize(const Vec2i &newsize);
	void update(double delta);

	// Redraw checks dirty flag of all windows and calls FullRedraw or
	// PartialRedraw. It's the function you want to call every frame.
	void redraw();
	void full_redraw(Window *w);
	void partial_redraw(Window *w);
	void draw_images(Window *w);

	// Composite is supposed to be called every frame, it draws all the windows
	// in the correct order to the screen. It's cheap, basically a blit
	// operation.
	void composite();

	Vec4i cursor(Window *w) const;
	Window *window_at(const Vec2i &p) const;
	void raise(Window *win);

	void handle_termbox_image_generated(RTTIObject *event);
};
