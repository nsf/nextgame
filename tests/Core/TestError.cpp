#include "stf.h"
#include "Core/Error.h"
#include "Core/Defer.h"
#include <cstring>

STF_SUITE_NAME("Core.Error")

STF_TEST("random") {
	Error a(EV_QUIET);
	a.set("oops, something weird happened");
	STF_ASSERT(a.code() == GenericErrorCode);
	STF_ASSERT(strcmp(a.description(), "") == 0);
	STF_ASSERT(a);

	Error b(EV_QUIET);
	STF_ASSERT(!b);

	Error c;
	c.set("oops, I did it again: %d", 43);
	STF_ASSERT(c.code() == GenericErrorCode);
	STF_ASSERT(strcmp(c.description(), "oops, I did it again: 43") == 0);
	STF_ASSERT(c);
}
