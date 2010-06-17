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

// =====================================================
//	class SimpleTaskThread
// =====================================================

//
// This interface describes the methods a callback object must implement
//
class SimpleTaskCallbackInterface {
public:
	virtual void simpleTask() = 0;
};

class SimpleTaskThread : public BaseThread
{
protected:

	SimpleTaskCallbackInterface *simpleTaskInterface;
	unsigned int executionCount;
	unsigned int millisecsBetweenExecutions;

	Mutex mutexTaskSignaller;
	bool taskSignalled;
	bool needTaskSignal;

public:
	SimpleTaskThread();
	SimpleTaskThread(SimpleTaskCallbackInterface *simpleTaskInterface, 
					 unsigned int executionCount=0, 
					 unsigned int millisecsBetweenExecutions=0,
					 bool needTaskSignal = false);
    virtual void execute();
    void setTaskSignalled(bool value);
    bool getTaskSignalled();
};

class PumpSDLEventsTaskThread : public BaseThread
{
public:
	PumpSDLEventsTaskThread();
    virtual void execute();
};

}}//end namespace

#endif
