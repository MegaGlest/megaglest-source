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

#include <string>
#include <stdexcept>
#include "platform_common.h"
#include "leak_dumper.h"

using namespace Shared::PlatformCommon;
using std::string;
using std::exception;

namespace Shared { namespace Platform {

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
public:
	virtual ~PlatformExceptionHandler() {}
	void install(string dumpFileName) {}
	virtual void handle()=0;
};

// =====================================================
//	Misc
// =====================================================
int MessageBox(int handle, const char *msg, const char *title, int buttons);
void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);

string getCommandLine();

}}//end namespace

#endif
