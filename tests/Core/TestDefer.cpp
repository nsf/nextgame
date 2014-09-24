#include "stf.h"
#include "Core/Defer.h"

STF_SUITE_NAME("Core.Defer")

STF_TEST("Defer") {
	int a = 0;
	int b = 0;
	{
		auto da = Defer([&]{ a = 10; });
		auto db = Defer([&]{ b = 20; });
		db.cancel();
	}
	STF_ASSERT(a == 10 && b == 0);
}

STF_TEST("DEFER") {
	int x = 0;
	int y = 0;
	{
		DEFER {
			x = 20;
			y = 40;
		};
	}
	STF_ASSERT(x == 20 && y == 40);
}

STF_TEST("NAMED_DEFER") {
	int a = 0, b = 0;
	{
		DEFER_NAMED(da) { a = 50; };
		DEFER_NAMED(db) { b = 100; };
		da.cancel();
	}
	STF_ASSERT(a == 0 && b == 100);
}
