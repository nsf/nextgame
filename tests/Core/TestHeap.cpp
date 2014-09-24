#include "stf.h"
#include "Core/Heap.h"

STF_SUITE_NAME("Core.Heap")

STF_TEST("random") {
	Heap<int> h;
	h.push(8);
	h.push(7);
	h.push(1);
	h.push(4);
	h.push(6);
	h.push(-1);
	h.push(5);
	h.push(9);
	h.push(2);
	h.push(10);
	h.push(3);
	h.push(0);

	for (int i = 0, n = h.length(); i < n; i++) {
		STF_ASSERT(h.pop() == i-1);
	}
}
