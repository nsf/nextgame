#include "OS/Timer.h"
#include <SDL2/SDL_timer.h>
#include <limits>

Timer::Timer(TimerAction a): m_start(0)
{
	m_start = 0;
	if (a == TA_START)
		start();
}

void Timer::start()
{
	m_start = SDL_GetPerformanceCounter();
}

double Timer::elapsed() const
{
	uint64_t end = SDL_GetPerformanceCounter();
	return (end - m_start) / (double)SDL_GetPerformanceFrequency();
}

double Timer::elapsed_ms() const
{
	return elapsed() * 1000.0;
}

double Timer::delta()
{
	uint64_t end = SDL_GetPerformanceCounter();
	double delta = (end - m_start) / (double)SDL_GetPerformanceFrequency();
	m_start = end;
	return delta;
}


ProfilingTimer::ProfilingTimer(const char *name): m_name(name)
{
	m_min = std::numeric_limits<uint64_t>::max();
}

void ProfilingTimer::start()
{
	m_start = SDL_GetPerformanceCounter();
}

void ProfilingTimer::stop()
{
	const uint64_t now = SDL_GetPerformanceCounter();
	const uint64_t diff = now - m_start;
	m_accum += diff;
	if (diff > m_max)
		m_max = diff;
	if (diff < m_min)
		m_min = diff;
	m_n++;

	m_start = now;
}

void ProfilingTimer::report()
{
	if (m_n == 0)
		return;

	const double freq = (double)SDL_GetPerformanceFrequency();
	const double min_ms = (m_min / freq) * 1000;
	const double max_ms = (m_max / freq) * 1000;
	const double avg_ms = ((m_accum / m_n) / freq) * 1000;
	printf("%s: min: %fms, max: %fms, avg: %fms\n",
		m_name, min_ms, max_ms, avg_ms);

	// reset everything also
	m_start = 0;
	m_accum = 0;
	m_min = std::numeric_limits<uint64_t>::max();
	m_max = 0;
	m_n = 0;
}

ProfilingTimer t_character_physics("Character physics");
