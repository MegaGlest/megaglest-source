// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#ifdef WIN32

#include <windows.h>

#endif

#include <string>
#include <stdexcept>
#include "platform_common.h"
#include "leak_dumper.h"

using namespace Shared::PlatformCommon;
using std::string;
using std::exception;

namespace Shared { namespace Platform {

class megaglest_runtime_error : public runtime_error {
protected:
	bool noStackTrace;
public:
    megaglest_runtime_error(const string& __arg,bool noStackTrace=false);

    bool wantStackTrace() const { return !noStackTrace; }
};

#ifndef WIN32
// =====================================================
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
public:
	static string application_binary;
	static bool disableBacktrace;
	static string getStackTrace();

	virtual ~PlatformExceptionHandler() {}
	void install(string dumpFileName) {}
	virtual void handle()=0;
#if defined(__WIN32__) && !defined(__GNUC__)
	virtual void handle(LPEXCEPTION_POINTERS pointers)=0;
#endif
};

// =====================================================
//	Misc
// =====================================================
void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);

string getCommandLine();

// WINDOWS
#else

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
private:
	static PlatformExceptionHandler *thisPointer;

private:
	static LONG WINAPI handler(LPEXCEPTION_POINTERS pointers);
	string dumpFileName;

public:
	static string application_binary;
	static bool disableBacktrace;
	static string getStackTrace();

	void install(string dumpFileName);
	virtual void handle()=0;
#if !defined(__GNUC__)
	virtual void handle(LPEXCEPTION_POINTERS pointers)=0;
#endif
	static string codeToStr(DWORD code);
};

LONG WINAPI UnhandledExceptionFilter2(struct _EXCEPTION_POINTERS *ExceptionInfo);

// =====================================================
//	Misc
// =====================================================
LPWSTR Ansi2WideString(LPCSTR lpaszString);
std::string utf8_encode(const std::wstring &wstr);
std::wstring utf8_decode(const std::string &str);
std::string getRegKey(const std::string& location, const std::string& name);

void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);
string getCommandLine();
void init_win32();
void done_win32();
void ontop_win32(int width, int height);

// The following is used for stacking tracing for windows based exceptions
#if !defined(_DEBUG) && !defined(__GNUC__)

// easy safe strings
#define MAXSTRLEN 260
typedef char stringType[MAXSTRLEN];

inline void vformatstring(char *d, const char *fmt, va_list v, int len = MAXSTRLEN) { _vsnprintf(d, len, fmt, v); d[len-1] = 0; }
inline char *copystring(char *d, const char *s, size_t len = MAXSTRLEN) { strncpy(d, s, len); d[len-1] = 0; return d; }
inline char *concatstring(char *d, const char *s, size_t len = MAXSTRLEN) { size_t used = strlen(d); return used < len ? copystring(d+used, s, len-used) : d; }

struct stringformatter {
    char *buf;
    stringformatter(char *buf): buf((char *)buf) {}
    void operator()(const char *fmt, ...) {
        va_list v;
        va_start(v, fmt);
        vformatstring(buf, fmt, v);
        va_end(v);
    }
};

#define formatstring(d) stringformatter((char *)d)
#define defformatstring(d) stringType d; formatstring(d)
#define defvformatstring(d,last,fmt) stringType d; { va_list ap; va_start(ap, last); vformatstring(d, fmt, ap); va_end(ap); }

#endif


#endif

}}//end namespace

#endif
