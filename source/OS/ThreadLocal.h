#pragma once

#include <SDL2/SDL.h>
#include "Core/Memory.h"

template <typename T>
struct ThreadLocal {
	SDL_SpinLock lock = 0;
	SDL_TLSID id = 0;

	static void _destroy(void *p)
	{
		T *pp = (T*)p;
		delete pp;
	}

	T *get()
	{
		if (!id) {
			SDL_AtomicLock(&lock);
			if (!id)
				id = SDL_TLSCreate();
			SDL_AtomicUnlock(&lock);
		}
		T *v = (T*)SDL_TLSGet(id);
		if (!v) {
			v = new (OrDie) T;
			SDL_TLSSet(id, v, _destroy);
		}
		return v;
	}
};
