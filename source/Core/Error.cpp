#include "Core/Error.h"
#include "Core/Memory.h"
#include "Core/String.h"
#include <cstdio>
#include <cstdlib>

ErrorDomain GenericErrorDomain;
ErrorCode GenericErrorCode(&GenericErrorDomain, 1);

ErrorData::~ErrorData()
{
}

void ErrorData::destroy()
{
	delete this;
}

const char *ErrorData::description() const
{
	return "";
}

void StaticErrorData::destroy() {}

Error::Error(ErrorVerbosity verbosity): m_verbosity(verbosity)
{
}

Error::~Error()
{
	if (m_data)
		m_data->destroy();
	delete [] m_message;
}

void Error::set(ErrorCode code)
{
	set_data(code, nullptr);
}

void Error::set(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	set_va(GenericErrorCode, format, va);
	va_end(va);
}

void Error::set(ErrorCode code, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	set_va(code, format, va);
	va_end(va);
}

void Error::clear()
{
	m_code = {};
	if (m_data) {
		m_data->destroy();
		m_data = nullptr;
	}
	delete [] m_message;
	m_message = nullptr;
}

void Error::set_va(ErrorCode code, const char *format, va_list va)
{
	clear();
	m_code = code;
	if (m_verbosity > EV_QUIET)
		m_message = String::vformat(format, va).release();
}

void Error::set_data(ErrorCode code, ErrorData *data)
{
	clear();
	m_code = code;
	if (m_verbosity > EV_QUIET)
		m_data = data;
	else if (data)
		data->destroy();
}

const char *Error::description() const
{
	if (m_data)
		return m_data->description();
	if (m_message)
		return m_message;
	return "";
}

void AbortError::set_va(ErrorCode, const char *format, va_list va)
{
	vfprintf(stderr, format, va);
	fprintf(stderr, "\n");
	abort();
}

void AbortError::set_data(ErrorCode, ErrorData *data)
{
	if (data) {
		fprintf(stderr, "%s\n", data->description());
		data->destroy();
	}
	abort();
}

AbortError DefaultError;


