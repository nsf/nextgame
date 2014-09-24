#include "stf.h"
#include "Core/BitArray.h"

STF_SUITE_NAME("Core.BitArray")

STF_TEST("random") {
	BitArray ba(100);

	// set and clear
	ba.set();
	for (int i = 0; i < 100; i++) {
		STF_ASSERT(ba.test_bit(i) == true);
	}
	ba.clear();
	for (int i = 0; i < 100; i++) {
		STF_ASSERT(ba.test_bit(i) == false);
	}

	// set_bit, clear_bit
	for (int i = 0; i < 100; i++) {
		ba.set_bit(i);
		for (int j = 0; j < 100; j++) {
			if (j <= i)
				STF_ASSERT(ba.test_bit(j) == true);
			else
				STF_ASSERT(ba.test_bit(j) == false);
		}
	}
	for (int i = 0; i < 100; i++) {
		ba.clear_bit(i);
		for (int j = 0; j < 100; j++) {
			if (j <= i)
				STF_ASSERT(ba.test_bit(j) == false);
			else
				STF_ASSERT(ba.test_bit(j) == true);
		}
	}

	// set_bit_range (a bit messy, but thorough)
	for (int i = 0; i < 90; i++) {
		const int beg = i;
		const int end = i+10;
		ba.clear();
		ba.set_bit_range(beg, end);
		for (int j = 0; j < 100; j++) {
			if (beg <= j && j < end)
				STF_ASSERT(ba.test_bit(j) == true);
			else
				STF_ASSERT(ba.test_bit(j) == false);
		}
	}
	for (int k = 0; k < 100; k++) {
	for (int i = 0; i <= 100; i++) {
		ba.clear();
		if (i < k)
			continue;
		ba.set_bit_range(k, i);
		for (int j = 0; j < 100; j++) {
			if (j >= k && j < i)
				STF_ASSERT(ba.test_bit(j) == true);
			else
				STF_ASSERT(ba.test_bit(j) == false);
		}
	}}
}
