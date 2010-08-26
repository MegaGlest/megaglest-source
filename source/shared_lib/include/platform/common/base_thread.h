// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2009-2010 Titus Tscharntke (info@titusgames.de) and
//                          Mark Vejvoda (mark_vejvoda@hotmail.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORMCOMMON_BASETHREAD_H_
#define _SHARED_PLATFORMCOMMON_BASETHREAD_H_

#include "thread.h"
#include <string>

using namespace Shared::Platform;
using namespace std;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class BaseThread
// =====================================================

class BaseThread : public Thread
{
protected:
	Mutex mutexRunning;
	Mutex mutexQuit;
	Mutex mutexBeginExecution;

	bool quit;
	bool running;
	string uniqueID;
	bool hasBeginExecution;

	virtual void setRunningStatus(bool value);
	virtual void setQuitStatus(bool value);

public:
	BaseThread();
	virtual ~BaseThread();
	virtual void execute()=0;

	virtual void signalQuit();
	virtual bool getQuitStatus();
	virtual bool getRunningStatus();
	virtual bool getHasBeginExecution();
	virtual void setHasBeginExecution(bool value);

    static void shutdownAndWait(BaseThread *ppThread);
    virtual void shutdownAndWait();

    void setUniqueID(string value) { uniqueID = value; }
    string getUniqueID() { return uniqueID; }
};

}}//end namespace

#endif
