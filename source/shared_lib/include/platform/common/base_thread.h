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

#include "leak_dumper.h"
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
	Mutex mutexDeleteSelfOnExecutionDone;

    Mutex mutexThreadObjectAccessor;

	Mutex mutexExecutingTask;
	bool executingTask;

	bool quit;
	bool running;
	string uniqueID;
	bool hasBeginExecution;
	bool deleteSelfOnExecutionDone;

	virtual void setQuitStatus(bool value);
	void deleteSelfIfRequired();

public:
	BaseThread();
	virtual ~BaseThread();
	virtual void execute()=0;

	virtual void signalQuit();
	virtual bool getQuitStatus();
	virtual bool getRunningStatus();

	virtual bool getHasBeginExecution();
	virtual void setHasBeginExecution(bool value);

    static bool shutdownAndWait(BaseThread *ppThread);
    virtual bool shutdownAndWait();
    virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);

    virtual bool getDeleteSelfOnExecutionDone();
	virtual void setDeleteSelfOnExecutionDone(bool value);

    void setUniqueID(string value) { uniqueID = value; }
    string getUniqueID() { return uniqueID; }

    virtual void setRunningStatus(bool value);

    void setExecutingTask(bool value);
    bool getExecutingTask();

    Mutex * getMutexThreadObjectAccessor() { return &mutexThreadObjectAccessor; }
};

class RunningStatusSafeWrapper {
protected:
	BaseThread *thread;
public:

	RunningStatusSafeWrapper(BaseThread *thread) {
		this->thread = thread;
		Enable();
	}
	~RunningStatusSafeWrapper() {
		Disable();
	}

	void Enable() {
		if(this->thread != NULL) {
			this->thread->setRunningStatus(true);
		}
	}
	void Disable() {
		if(this->thread != NULL) {
		    this->thread->setRunningStatus(false);
		}
	}
};

class ExecutingTaskSafeWrapper {
protected:
	BaseThread *thread;
public:

	ExecutingTaskSafeWrapper(BaseThread *thread) {
		this->thread = thread;
		Enable();
	}
	~ExecutingTaskSafeWrapper() {
		Disable();
	}

	void Enable() {
		if(this->thread != NULL) {
			this->thread->setExecutingTask(true);
		}
	}
	void Disable() {
		if(this->thread != NULL) {
		    this->thread->setExecutingTask(false);
		}
	}
};


}}//end namespace

#endif
