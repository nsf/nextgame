// This file was taken from the musl library, it's governed by a MIT-style
// license, you can find more details in 3rdparty/musl_license.txt file.

#include "Core/String.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include "Core/Trio/trio.h"

#pragma GCC diagnostic pop

static int callback(trio_pointer_t p, int c)
{
	String *s = (String*)p;
	s->append(c);
	return 1;
}

String String::format(const char *fmt, ...)
{
	String out;
	va_list vl;
	va_start(vl, fmt);
	trio_vcprintf(callback, &out, fmt, vl);
	va_end(vl);
	return out;
}

String String::vformat(const char *fmt, va_list va)
{
	String out;
	trio_vcprintf(callback, &out, fmt, va);
	return out;
}
