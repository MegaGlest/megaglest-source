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

}}//end namespace

#endif
