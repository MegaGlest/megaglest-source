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
#ifndef _SHARED_PLATFORMCOMMON_SIMPLETHREAD_H_
#define _SHARED_PLATFORMCOMMON_SIMPLETHREAD_H_

#include "base_thread.h"
#include <vector>
#include <string>

using namespace std;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class FileCRCPreCacheThread
// =====================================================

class FileCRCPreCacheThread : public BaseThread
{
protected:

	vector<string> techDataPaths;

public:
	FileCRCPreCacheThread();
	FileCRCPreCacheThread(vector<string> techDataPaths);
    virtual void execute();
    void setTechDataPaths(vector<string> techDataPaths) { this->techDataPaths = techDataPaths; }
};

}}//end namespace

#endif
