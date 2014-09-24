#pragma once

#include <utility>

template <typename F>
struct DeferGuard {
	bool m_active;
	F m_f;

	DeferGuard() = delete;
	DeferGuard(const DeferGuard&) = delete;
	DeferGuard &operator=(const DeferGuard&) = delete;
	DeferGuard &operator=(DeferGuard &&r) = delete;

	DeferGuard(F f): m_active(true), m_f(std::move(f)) {}
	DeferGuard(DeferGuard &&r): m_active(r.m_active), m_f(std::move(r.m_f))
	{
		r.cancel();
	}
	~DeferGuard() { if (m_active) m_f(); }

	void cancel() { m_active = false; }
};

template <typename F>
DeferGuard<F> Defer(F f) { return DeferGuard<F>(std::move(f)); }

enum class DeferHelper {};
template <typename F>
DeferGuard<F> operator+(DeferHelper, F &&f)
{
	return DeferGuard<F>(std::forward<F>(f));
}

#define DEFER_CONCAT_IMPL(a, b) a##b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_IMPL(a, b)

#define DEFER \
	auto DEFER_CONCAT(DEFER_GUARD, __COUNTER__) = DeferHelper() + [&]

#define DEFER_NAMED(name) \
	auto name = DeferHelper() + [&]
