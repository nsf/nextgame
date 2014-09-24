#include "GUI/WindowManager.h"
#include "Core/Memory.h"
#include "Render/Meshes.h"
#include "Math/Mat.h"
#include "Geometry/Quads.h"
#include "Core/Defer.h"
#include "OOP/EventManager.h"
#include <cairo/cairo.h>

Window *WindowManager::add_window(UniquePtr<TermboxWindow> win,
	WindowAlignment alignment, const Vec2i &pos)
{
	auto w = new (OrDie) Window;
	w->termbox_window = std::move(win);
	w->alignment = alignment;
	w->position = pos;
	w->texture_size = w->termbox_window->size_in_pixels();
	w->pbo = Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
	w->texture = Texture(w->texture_size.x, w->texture_size.y, GL_RGBA);
	w->framebuffer.attach(GL_COLOR_ATTACHMENT0, w->texture);
	w->framebuffer.validate();
	w->flags = WF_DIRTY | WF_VISIBLE | WF_AUTO_RESIZE;
	w->alpha = 1.0;
	if (alignment == WA_FLOATING)
		windows.insert(0, UniquePtr<Window>(w));
	else
		windows.append(UniquePtr<Window>(w));
	w->realign(this->size);
	return w;
}

void WindowManager::remove_window(Window *win)
{
	int wi = -1;
	for (int i = 0; i < windows.length(); i++) {
		if (windows[i] == win) {
			wi = i;
			break;
		}
	}

	if (wi == -1)
		return;

	if (active_window == win)
		active_window = nullptr;
	windows.remove(wi);
}

void WindowManager::resize(const Vec2i &newsize)
{
	size = newsize;
	for (auto &w : windows)
		w->realign(this->size);
}

void WindowManager::update(double delta)
{
	for (auto &w : windows) {
		w->update(delta);
	}
}

void WindowManager::redraw()
{
	if (!active)
		return;

	// check if at least one window is (partially) dirty
	bool has_dirty = false;
	for (const auto &w : windows) {
		if (w->is_visible() && (w->flags & (WF_DIRTY | WF_DIRTY_PARTIAL))) {
			has_dirty = true;
			break;
		}
	}

	// no dirty windows, return
	if (!has_dirty)
		return;

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	DEFER {
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	};
	for (auto &w : windows) {
		if (!w->is_visible())
			continue;
		if (w->flags & WF_DIRTY) {
			w->realign(size);
			full_redraw(w.get());
			w->flags &= ~(WF_DIRTY | WF_DIRTY_PARTIAL);
		} else if (w->flags & WF_DIRTY_PARTIAL) {
			partial_redraw(w.get());
			w->flags &= ~WF_DIRTY_PARTIAL;
		}
	}
}

void WindowManager::full_redraw(Window *w)
{
	const Vec2i size = w->termbox_window->size_in_pixels();
	w->framebuffer.bind();
	glViewport(0, 0, size.x, size.y);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE);
	const Vec4 base_ortho = Vec4_MiniOrtho2D(0, size.x, 0, size.y);
	const Vec4 termbox_ortho = mini_ortho_translate(base_ortho,
		ToVec2(w->termbox_window->termbox_offset()));

	w->termbox_window->redraw_decor(tex_normal);
	if (tex_normal.length() > 0) {
		BIND_SHADER(tex2d);
		SET_UNIFORM(MiniOrtho, base_ortho);
		SET_UNIFORM_TEXTURE(Texture, w->termbox_window->decor_texture(), 0);
		draw_quads(quad_stream, tex_normal);
		tex_normal.clear();
	}

	w->termbox_window->termbox->full_redraw(col, tex_normal, tex_bold, images);
	if (col.length() > 0) {
		BIND_SHADER(col2d);
		SET_UNIFORM(MiniOrtho, termbox_ortho);
		draw_quads(quad_stream, col);
		col.clear();
	}
	if (tex_normal.length() > 0) {
		BIND_SHADER(tex2d);
		SET_UNIFORM(MiniOrtho, termbox_ortho);
		SET_UNIFORM_TEXTURE(Texture, w->termbox_window->termbox->normal_texture(), 0);
		draw_quads(quad_stream, tex_normal);
		tex_normal.clear();
	}
	if (tex_bold.length() > 0) {
		BIND_SHADER(tex2d);
		SET_UNIFORM(MiniOrtho, termbox_ortho);
		SET_UNIFORM_TEXTURE(Texture, w->termbox_window->termbox->bold_texture(), 0);
		draw_quads(quad_stream, tex_bold);
		tex_bold.clear();
	}
	draw_images(w);
}

void WindowManager::partial_redraw(Window *w)
{
	const Vec2i size = w->termbox_window->size_in_pixels();
	w->framebuffer.bind();
	glViewport(0, 0, size.x, size.y);
	const Vec4 termbox_ortho = Vec4_MiniOrtho2D(0, size.x, 0, size.y,
		ToVec2(w->termbox_window->termbox_offset()));

	w->termbox_window->termbox->partial_redraw(col, tex_normal, tex_bold, images);
	if (col.length() > 0) {
		glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
		BIND_SHADER(col2d);
		SET_UNIFORM(MiniOrtho, termbox_ortho);
		draw_quads(quad_stream, col);
		col.clear();
	}
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE);
	if (tex_normal.length() > 0) {
		BIND_SHADER(tex2d);
		SET_UNIFORM(MiniOrtho, termbox_ortho);
		SET_UNIFORM_TEXTURE(Texture, w->termbox_window->termbox->normal_texture(), 0);
		draw_quads(quad_stream, tex_normal);
		tex_normal.clear();
	}
	if (tex_bold.length() > 0) {
		BIND_SHADER(tex2d);
		SET_UNIFORM(MiniOrtho, termbox_ortho);
		SET_UNIFORM_TEXTURE(Texture, w->termbox_window->termbox->bold_texture(), 0);
		draw_quads(quad_stream, tex_bold);
		tex_bold.clear();
	}
	draw_images(w);
}

void WindowManager::draw_images(Window *w)
{
	w->texture.bind(0);
	if (images.length() > 0) {
		for (TermboxImage *img : images) {
			cairo_surface_flush(img->current);
			const void *data = cairo_image_surface_get_data(img->current);
			const Vec2i base_offset = w->termbox_window->termbox_offset();
			const Vec2i cell_size = w->termbox_window->termbox->cell_size();
			const Vec2i offset = base_offset + img->last_position * cell_size;
			const Vec2i size = img->last_size * cell_size;
			w->pbo.upload(Slice<const uint8_t>((const uint8_t*)data, 4 * area(size)));
			glTexSubImage2D(GL_TEXTURE_2D, 0,
				offset.x, offset.y, size.x, size.y, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		}
		images.clear();
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void WindowManager::composite()
{
	if (!active)
		return;

	//glDisable(GL_FRAMEBUFFER_SRGB);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	DEFER {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_FRAMEBUFFER_SRGB);
	};
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	BIND_SHADER(termbox);
	const Vec4 base_ortho = Vec4_MiniOrtho2D(0, size.x, size.y, 0);
	SET_UNIFORM(MiniOrtho, base_ortho);
	for (int i = windows.length()-1; i >= 0; i--) {
		const auto &w = windows[i];
		if (!w->is_visible())
			continue;
		const Vec2i size = w->termbox_window->size_in_pixels();
		SET_UNIFORM(Alpha, w->alpha);
		SET_UNIFORM(Cursor, cursor(w.get()));
		SET_UNIFORM_TEXTURE(Texture, w->texture, 0);
		append_quad(tex_normal, {w->position, size}, {0, 0}, ToVec2(size));
		draw_quads(quad_stream, tex_normal);
		tex_normal.clear();
	}
}

Vec4i WindowManager::cursor(Window *w) const
{
	if (w != active_window)
		return {-1, -1, -1, -1};

	return w->termbox_window->cursor();
}

Window *WindowManager::window_at(const Vec2i &p) const
{
	for (const auto &w : windows) {
		const Rect r(w->position, w->termbox_window->size_in_pixels());
		if (is_point_inside(p, r) && w->is_visible()) {
			return w.get();
		}
	}
	return nullptr;
}

void WindowManager::raise(Window *win)
{
	active_window = win;
	if (win->alignment != WA_FLOATING)
		return;

	int wi = -1;
	for (int i = 0; i < windows.length(); i++) {
		if (windows[i] == win) {
			wi = i;
			break;
		}
	}

	NG_ASSERT(wi != -1);
	win = windows[wi].release();
	windows.remove(wi);
	windows.insert(0, UniquePtr<Window>(win));
}

void WindowManager::handle_termbox_image_generated(RTTIObject *event)
{
	ETermboxImageRequest *req = ETermboxImageRequest::cast(event);
	int i = linear_find_if(windows.sub(), [&](const UniquePtr<Window> &win){
		return win->termbox_window->termbox.get() == req->sender;
	});
	if (i == -1) {
		warn("failed to handle EID_TERMBOX_IMAGE_GENERATED message");
		return;
	}

	Termbox *tb = windows[i]->termbox_window->termbox.get();
	int j = linear_find_if(tb->m_images.sub(), [&](const UniquePtr<TermboxImage> &img) {
		return img.get() == req->image;
	});
	if (j == -1) {
		warn("EID_TERMBOX_IMAGE_GENERATED message, image not found :-(");
		return;
	}

	TermboxImage *img = tb->m_images[j].get();
	const Vec2i target_size(
		cairo_image_surface_get_width(req->target),
		cairo_image_surface_get_height(req->target));
	if (target_size != img->size) {
		img->put_next(req->target);
		printf("SIZE MISMATCH: %d\n", img->id);
		return;
	}

	img->put_current(req->target);
	if (img->flags & TIF_DRAW_REQUESTED) {
		windows[i]->texture.bind(0);
		cairo_surface_flush(img->current);
		const void *pixels = cairo_image_surface_get_data(img->current);
		const Vec2i offset = windows[i]->termbox_window->termbox_offset() +
			img->last_position * tb->cell_size();
		const Vec2i size = img->last_size * tb->cell_size();
		windows[i]->pbo.upload(Slice<const uint8_t>((const uint8_t*)pixels, 4 * area(size)));
		glTexSubImage2D(GL_TEXTURE_2D, 0,
			offset.x, offset.y, size.x, size.y,
			GL_BGRA, GL_UNSIGNED_BYTE, 0);
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

WindowManager::WindowManager(const Vec2i &size):
	size(size)
{
	quad_stream = create_quad_stream_alpha();
	NG_EventManager->register_handler(EID_TERMBOX_IMAGE_GENERATED,
		PASS_TO_METHOD(WindowManager, handle_termbox_image_generated), this, false);
}

WindowManager::~WindowManager()
{
	NG_EventManager->unregister_handlers(this);
}
