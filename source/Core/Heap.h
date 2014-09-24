#pragma once

#include "Core/Vector.h"

template <typename T>
struct Heap {
	Vector<T> m_data;

	void _up(int j)
	{
		while (true) {
			const int i = (j-1) / 2;
			if (i == j || !(m_data[j] < m_data[i]))
				break;
			std::swap(m_data[i], m_data[j]);
			j = i;
		}
	}

	void _down(int i, int n)
	{
		while (true) {
			const int j1 = 2*i + 1;
			if (j1 >= n || j1 < 0) // j1 < 0 after int overflow
				break;

			const int j2 = j1 + 1;
			int j = j1;
			if (j2 < n && !(m_data[j1] < m_data[j2])) {
				j = j2;
			}
			if (!(m_data[j] < m_data[i]))
				break;

			std::swap(m_data[i], m_data[j]);
			i = j;
		}
	}

	int length() const { return m_data.length(); }
	int byte_length() const { return m_data.byte_length(); }
	int capacity() const { return m_data.capacity(); }

	template <typename ...Args>
	void ppush(Args &&...args)
	{
		m_data.pappend(std::forward<Args>(args)...);
		_up(length()-1);
	}

	void push(const T &elem)
	{
		ppush(elem);
	}

	void push(T &&elem)
	{
		ppush(std::move(elem));
	}

	T pop()
	{
		const int last = length()-1;
		std::swap(m_data[0], m_data[last]);
		_down(0, last);
		T out = std::move(m_data.last());
		m_data.remove(last);
		return out;
	}
};
