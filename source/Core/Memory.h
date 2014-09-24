#pragma once

#include <new>
#include <utility>
#include <type_traits>
#include <cstddef>

void *xmalloc(int n);
void xfree(void *ptr);
int xcopy(void *dst, const void *src, int n);

struct OrDie_t {};
const OrDie_t OrDie = {};

void *operator new(size_t size, const OrDie_t&);
void *operator new[](size_t size, const OrDie_t&);

template <typename T>
T *allocate_memory(int n = 1)
{
	static_assert(!std::is_polymorphic<T>::value,
		"for polymorphic types, use NewObject constructor instead");
	return (T*)xmalloc(sizeof(T) * n);
}

template <typename T>
T &allocate_memory(T *&ptr)
{
	static_assert(!std::is_polymorphic<T>::value,
		"for polymorphic types, use NewObjectAt constructor instead");
	ptr = (T*)xmalloc(sizeof(T));
	return *ptr;
}

template <typename T>
void free_memory(T *ptr)
{
	static_assert(!std::is_polymorphic<T>::value,
		"for polymorphic types, use FreeObject destroying function");
	if (ptr) xfree(ptr);
}

template <typename T>
int copy_memory(T *dst, const T *src, int n = 1)
{
	return xcopy(dst, src, sizeof(T) * n);
}
