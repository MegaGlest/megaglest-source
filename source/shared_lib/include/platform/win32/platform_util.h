// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#include <windows.h>
#include <string>
#include <stdexcept>
#include "platform_common.h"
#include "leak_dumper.h"

using namespace Shared::PlatformCommon;
using std::string;
using std::exception;

namespace Shared{ namespace Platform{

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

LPWSTR Ansi2WideString(LPCSTR lpaszString);
std::string utf8_encode(const std::wstring &wstr);
std::wstring utf8_decode(const std::string &str);

LONG WINAPI UnhandledExceptionFilter2(struct _EXCEPTION_POINTERS *ExceptionInfo);

class PlatformExceptionHandler{
private:
	static PlatformExceptionHandler *thisPointer;

private:
	static LONG WINAPI handler(LPEXCEPTION_POINTERS pointers);
	string dumpFileName;

public:
	void install(string dumpFileName);
	virtual void handle()=0;
	static string codeToStr(DWORD code);
};

// =====================================================
//	Misc
// =====================================================
void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);
string getCommandLine();
void init_win32();
void done_win32();


// The following is used for stacking tracing for windows based exceptions
#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)

// easy safe strings
#define MAXSTRLEN 260
typedef char stringType[MAXSTRLEN];

inline void vformatstring(char *d, const char *fmt, va_list v, int len = MAXSTRLEN) { _vsnprintf(d, len, fmt, v); d[len-1] = 0; }
inline char *copystring(char *d, const char *s, size_t len = MAXSTRLEN) { strncpy(d, s, len); d[len-1] = 0; return d; }
inline char *concatstring(char *d, const char *s, size_t len = MAXSTRLEN) { size_t used = strlen(d); return used < len ? copystring(d+used, s, len-used) : d; }

struct stringformatter
{
    char *buf;
    stringformatter(char *buf): buf((char *)buf) {}
    void operator()(const char *fmt, ...)
    {
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

}}//end namespace

#endif
