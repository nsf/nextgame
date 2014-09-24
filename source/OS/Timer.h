#pragma once

#include <cstdint>

enum TimerAction {
	TA_START,
	TA_DONT_START,
};

struct Timer {
	uint64_t m_start;

	Timer(TimerAction a = TA_START);
	void start();
	double elapsed() const; // seconds
	double elapsed_ms() const; // milliseconds
	double delta(); // also resets the start
};

struct ProfilingTimer {
	const char *m_name = nullptr;
	uint64_t m_start = 0;
	uint64_t m_accum = 0;
	uint64_t m_min = 0;
	uint64_t m_max = 0;
	int m_n = 0;

	ProfilingTimer(const char *m_name);
	void start();
	void stop();
	void report();
};

extern ProfilingTimer t_character_physics;
