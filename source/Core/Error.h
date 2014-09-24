#pragma once

#include <cstdarg>

//----------------------------------------------------------------------
// ErrorDomain
//----------------------------------------------------------------------

struct ErrorDomain {};

//----------------------------------------------------------------------
// ErrorCode
//----------------------------------------------------------------------

struct ErrorCode {
	ErrorDomain *m_domain = nullptr;
	int m_code = 0;

	ErrorCode() = default;
	constexpr ErrorCode(ErrorDomain *domain, int code):
		m_domain(domain), m_code(code)
	{}

	constexpr bool operator==(const ErrorCode &r) const
	{
		return m_code == r.m_code && m_domain == r.m_domain;
	}
	constexpr bool operator!=(const ErrorCode &r) const
	{
		return m_code != r.m_code || m_domain != r.m_domain;
	}
	explicit constexpr operator bool() const { return m_code != 0; }
};

extern ErrorCode GenericErrorCode;

//----------------------------------------------------------------------
// ErrorData
//----------------------------------------------------------------------

// Default dtor does nothing, destroy() does 'delete this'. Default description
// is empty string.
struct ErrorData {
	virtual ~ErrorData();
	virtual void destroy();
	virtual const char *description() const;
};

// Specialization of data which doesn't delete itself.
struct StaticErrorData : ErrorData {
	virtual void destroy() override;
};

//----------------------------------------------------------------------
// Error
//----------------------------------------------------------------------

enum ErrorVerbosity {
	EV_QUIET,
	EV_VERBOSE,
	EV_EXTRA,
};

struct Error {
	ErrorVerbosity m_verbosity = EV_VERBOSE;
	ErrorCode m_code;
	ErrorData *m_data = nullptr;
	char *m_message = nullptr;

	Error() = default;
	Error(ErrorVerbosity verbosity);

	virtual ~Error();

	void set(ErrorCode code = GenericErrorCode);
	void set(const char *format, ...);
	void set(ErrorCode code, const char *format, ...);
	void clear();

	virtual void set_va(ErrorCode code, const char *format, va_list va);
	virtual void set_data(ErrorCode code, ErrorData *data);

	const char *description() const;
	ErrorCode code() const { return m_code; }
	ErrorVerbosity verbosity() const { return m_verbosity; }
	ErrorData *data() const { return m_data; }

	explicit operator bool() const { return static_cast<bool>(m_code); }
};

//----------------------------------------------------------------------
// AbortError
//----------------------------------------------------------------------

struct AbortError : Error {
	void set_va(ErrorCode code, const char *format, va_list va) override;
	void set_data(ErrorCode code, ErrorData *data) override;
};

extern AbortError DefaultError;
