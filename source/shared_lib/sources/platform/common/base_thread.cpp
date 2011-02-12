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

#include "base_thread.h"
#include "platform_common.h"
#include "util.h"
#include "conversion.h"
#include <time.h>

using namespace Shared::Util;

namespace Shared { namespace PlatformCommon {

BaseThread::BaseThread() : Thread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	uniqueID = "base_thread";

	setQuitStatus(false);
	setRunningStatus(false);
	setHasBeginExecution(false);
	setExecutingTask(false);
	setDeleteSelfOnExecutionDone(false);
	setThreadOwnerValid(true);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

BaseThread::~BaseThread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
	bool ret = shutdownAndWait();
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] ret [%d] END\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),ret);
}

Mutex * BaseThread::getMutexThreadOwnerValid() {
	if(getThreadOwnerValid() == true) {
	    return &mutexThreadOwnerValid;
	}
    return NULL;
}

void BaseThread::setThreadOwnerValid(bool value) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexThreadOwnerValid,mutexOwnerId);
	threadOwnerValid = value;
	safeMutex.ReleaseLock();
}

bool BaseThread::getThreadOwnerValid() {
	bool ret = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexThreadOwnerValid,mutexOwnerId);
	ret = threadOwnerValid;
	safeMutex.ReleaseLock();

	return ret;
}

void BaseThread::signalQuit() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	setQuitStatus(true);

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

void BaseThread::setQuitStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexQuit,mutexOwnerId);
	quit = value;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

bool BaseThread::getQuitStatus() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexQuit,mutexOwnerId);
	retval = quit;
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return retval;
}

bool BaseThread::getHasBeginExecution() {
	bool retval = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexBeginExecution,mutexOwnerId);
	retval = hasBeginExecution;
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return retval;
}

void BaseThread::setHasBeginExecution(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexBeginExecution,mutexOwnerId);
	hasBeginExecution = value;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

bool BaseThread::getRunningStatus() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	bool retval = false;

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexRunning,mutexOwnerId);
	retval = running;
	safeMutex.ReleaseLock();

	if(retval == false) {
		retval = !getHasBeginExecution();
	}

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] running = %d\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),retval);

	return retval;
}

void BaseThread::setRunningStatus(bool value) {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] value = %d\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),value);

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexRunning,mutexOwnerId);
	running = value;
	if(value == true) {
		setHasBeginExecution(true);
	}
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

void BaseThread::setExecutingTask(bool value) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexExecutingTask,mutexOwnerId);
	executingTask = value;
	safeMutex.ReleaseLock();
}

bool BaseThread::getExecutingTask() {
	bool retval = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexExecutingTask,mutexOwnerId);
	retval = executingTask;
	safeMutex.ReleaseLock();

	return retval;
}

bool BaseThread::getDeleteSelfOnExecutionDone() {
    bool retval = false;
    static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&mutexDeleteSelfOnExecutionDone,mutexOwnerId);
    retval = deleteSelfOnExecutionDone;
    safeMutex.ReleaseLock();

    return retval;
}

void BaseThread::setDeleteSelfOnExecutionDone(bool value) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&mutexDeleteSelfOnExecutionDone,mutexOwnerId);
    deleteSelfOnExecutionDone = value;
}

void BaseThread::deleteSelfIfRequired() {
    if(getDeleteSelfOnExecutionDone() == true) {
        delete this;
        return;
    }
}

bool BaseThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
    return true;
}

bool BaseThread::shutdownAndWait(BaseThread *pThread) {
	bool ret = false;
	if(pThread != NULL) {
		ret = pThread->shutdownAndWait();
	}

	return ret;
}

bool BaseThread::shutdownAndWait() {
	bool ret = true;
	BaseThread *pThread = this;
	string uniqueID = (pThread != NULL ? pThread->getUniqueID() : "?");
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	if(pThread != NULL) {
		if(pThread->getRunningStatus() == true) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

			pThread->signalQuit();
			sleep(0);
			ret = false;

			int maxWaitSeconds = 5;
			if(pThread->canShutdown() == false) {
				maxWaitSeconds = 2;
			}

			for( time_t elapsed = time(NULL); difftime(time(NULL),elapsed) <= maxWaitSeconds; ) {
				if(pThread->getRunningStatus() == false) {
					ret = true;
					break;
				}

				sleep(0);
			}
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
			sleep(0);
		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] ret [%d] END\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),ret);
	return ret;
}

}}//end namespace
