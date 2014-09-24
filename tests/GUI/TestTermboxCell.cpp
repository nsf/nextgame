#include "stf.h"
#include "GUI/TermboxCell.h"

STF_SUITE_NAME("GUI.TermboxCell")

STF_TEST("TermboxCell") {
	TermboxCell a('a', 31, 1.0f, RGB565_Blue(), RGB565_BoldCyan());
	STF_ASSERT(a.rune() == 'a');
	STF_ASSERT(a.attr() == 31);
	STF_ASSERT(a.bg_a() == 1.0f);
	STF_ASSERT(a.fg == RGB565_Blue());
	STF_ASSERT(a.bg == RGB565_BoldCyan());

	TermboxCell b('a', 31, 1.0f, RGB565_Blue(), RGB565_BoldCyan());
	STF_ASSERT(a == b);

	TermboxCell c('a', 0, 1.0f, RGB565_Blue(), RGB565_BoldCyan());
	STF_ASSERT(a != c);
}
