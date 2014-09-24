#include "stf.h"
#include "Core/Defer.h"
#include "Core/String.h"
#include "Core/HashMap.h"

STF_SUITE_NAME("Core.HashMap")

STF_TEST("random") {
	HashMap<String, int> m;

	STF_ASSERT(m.length() == 0);

	m.insert("hello", 33);
	STF_ASSERT(m.length() == 1);
	int *v = m.get("hello");
	STF_ASSERT(v != nullptr);
	STF_ASSERT(*v == 33);

	v = m.get("hello1");
	STF_ASSERT(v == nullptr);

	int x = m.get_or_default("hello", 22);
	STF_ASSERT(x == 33);
	int y = m.get_or_default("hello1", 44);
	STF_ASSERT(y == 44);

	m.remove("hello");
	STF_ASSERT(m.length() == 0);
	STF_ASSERT(m.get("hello") == nullptr);
}
