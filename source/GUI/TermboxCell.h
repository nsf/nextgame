#pragma once

#include "GUI/RGB565.h"

enum TermboxCellAttributes : int {
	TBC_BOLD   = 1 << 0,
	TBC_INVERT = 1 << 1,
};

struct TermboxCell {
	enum : uint32_t {
		RUNE_MASK = (1U << 21) - 1,
		ATTR_MASK = ((1U << 5) - 1) << 21,
		ATTR_SHIFT = 21U,
		BG_A_MASK = ((1U << 6) - 1) << 26,
		BG_A_SHIFT = 26U,
	};

	// 21 bit - unicode code point
	// 6 bit - attributes
	// 5 bit - bg alpha
	uint32_t rune_attr;
	RGB565 fg;
	RGB565 bg;

	TermboxCell() = default;
	TermboxCell(int rune, int attr, float bg_alpha, RGB565 fg, RGB565 bg):
		rune_attr(
			((uint32_t)rune & RUNE_MASK) |
			(((uint32_t)attr << ATTR_SHIFT) & ATTR_MASK) |
			(((uint32_t)(bg_alpha * 63) << BG_A_SHIFT) & BG_A_MASK)
		), fg(fg), bg(bg) {}

	int rune() const { return rune_attr & RUNE_MASK; }
	int attr() const { return (rune_attr & ATTR_MASK) >> ATTR_SHIFT; }
	float bg_a() const { return ((rune_attr & BG_A_MASK) >> BG_A_SHIFT) / 63.0f; }
};

static inline bool operator==(const TermboxCell &l, const TermboxCell &r)
{
	return l.rune_attr == r.rune_attr && l.fg == r.fg && l.bg == r.bg;
}
static inline bool operator!=(const TermboxCell &l, const TermboxCell &r)
{
	return l.rune_attr != r.rune_attr || l.fg != r.fg || l.bg != r.bg;
}
