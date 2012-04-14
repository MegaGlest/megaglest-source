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
#include "platform_util.h"
#include <time.h>

using namespace Shared::Util;

namespace Shared { namespace PlatformCommon {

Mutex BaseThread::mutexMasterThreadList;
std::map<void *,int> BaseThread::masterThreadList;

BaseThread::BaseThread() : Thread(), ptr(NULL) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ptr = this;
	MutexSafeWrapper safeMutexMasterList(&mutexMasterThreadList);
	masterThreadList[ptr]++;
	safeMutexMasterList.ReleaseLock();

	uniqueID = "base_thread";

	setQuitStatus(false);
	setRunningStatus(false);
	setHasBeginExecution(false);
	setExecutingTask(false);
	setDeleteSelfOnExecutionDone(false);
	setThreadOwnerValid(true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

BaseThread::~BaseThread() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
	bool ret = shutdownAndWait();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s] ret [%d] END\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),ret);

	MutexSafeWrapper safeMutexMasterList(&mutexMasterThreadList);
	if(masterThreadList.find(this) == masterThreadList.end()) {
		char szBuf[4096]="";
		sprintf(szBuf,"invalid thread delete for ptr: %p",this);
		throw megaglest_runtime_error(szBuf);
	}
	masterThreadList[this]--;
	if(masterThreadList[this] <= 0) {
		masterThreadList.erase(this);
	}
	safeMutexMasterList.ReleaseLock();
}

bool BaseThread::isThreadDeleted(void *ptr) {
	bool result = false;
	MutexSafeWrapper safeMutexMasterList(&mutexMasterThreadList);
	if(masterThreadList.find(ptr) != masterThreadList.end()) {
		result = (masterThreadList[ptr] <= 0);
	}
	safeMutexMasterList.ReleaseLock();
	return result;
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
	mutexThreadOwnerValid.setOwnerId(mutexOwnerId);
	threadOwnerValid = value;
	safeMutex.ReleaseLock();
}

bool BaseThread::getThreadOwnerValid() {
	bool ret = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexThreadOwnerValid,mutexOwnerId);
	//mutexThreadOwnerValid.setOwnerId(mutexOwnerId);
	ret = threadOwnerValid;
	safeMutex.ReleaseLock();

	return ret;
}

void BaseThread::signalQuit() {
	setQuitStatus(true);
}

void BaseThread::setQuitStatus(bool value) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexQuit,mutexOwnerId);
	mutexQuit.setOwnerId(mutexOwnerId);
	quit = value;
	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

bool BaseThread::getQuitStatus() {
	bool retval = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexQuit,mutexOwnerId);
	//mutexQuit.setOwnerId(mutexOwnerId);
	retval = quit;
	safeMutex.ReleaseLock();

	return retval;
}

bool BaseThread::getHasBeginExecution() {
	bool retval = false;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexBeginExecution,mutexOwnerId);
	//mutexBeginExecution.setOwnerId(mutexOwnerId);
	retval = hasBeginExecution;
	safeMutex.ReleaseLock();

	return retval;
}

void BaseThread::setHasBeginExecution(bool value) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexBeginExecution,mutexOwnerId);
	mutexBeginExecution.setOwnerId(mutexOwnerId);
	hasBeginExecution = value;
	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

bool BaseThread::getRunningStatus() {
	bool retval = false;

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexRunning,mutexOwnerId);
	retval = running;
	safeMutex.ReleaseLock();

	if(retval == false) {
		retval = !getHasBeginExecution();
	}

	return retval;
}

void BaseThread::setRunningStatus(bool value) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexRunning,mutexOwnerId);
	mutexRunning.setOwnerId(mutexOwnerId);
	running = value;
	if(value == true) {
		setHasBeginExecution(true);
	}
	safeMutex.ReleaseLock();
}

void BaseThread::setExecutingTask(bool value) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexExecutingTask,mutexOwnerId);
	mutexExecutingTask.setOwnerId(mutexOwnerId);
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
    mutexDeleteSelfOnExecutionDone.setOwnerId(mutexOwnerId);
    deleteSelfOnExecutionDone = value;
}

void BaseThread::deleteSelfIfRequired() {
    if(getDeleteSelfOnExecutionDone() == true) {
    	if(isThreadDeleted(this->ptr) == false) {
    		delete this;
    	}
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	if(pThread != NULL) {
		if(pThread->getRunningStatus() == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

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
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
			sleep(0);
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s] ret [%d] END\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),ret);
	return ret;
}

}}//end namespace
