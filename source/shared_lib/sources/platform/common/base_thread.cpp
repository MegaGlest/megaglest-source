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
#include <time.h>

using namespace Shared::Util;

namespace Shared { namespace PlatformCommon {

BaseThread::BaseThread() : Thread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	uniqueID = "base_thread";

	setQuitStatus(false);
	setRunningStatus(false);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

BaseThread::~BaseThread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
	shutdownAndWait();
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] END\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

void BaseThread::signalQuit() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	setQuitStatus(true);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

void BaseThread::setQuitStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	MutexSafeWrapper safeMutex(&mutexQuit);
	quit = value;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

bool BaseThread::getQuitStatus() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	MutexSafeWrapper safeMutex(&mutexQuit);
	retval = quit;
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return retval;
}

bool BaseThread::getRunningStatus() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	bool retval = false;
	MutexSafeWrapper safeMutex(&mutexRunning);
	retval = running;
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] running = %d\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),retval);

	return retval;
}

void BaseThread::setRunningStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] value = %d\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str(),value);

	MutexSafeWrapper safeMutex(&mutexRunning);
	running = value;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

void BaseThread::shutdownAndWait(BaseThread *pThread) {
	string uniqueID = (pThread != NULL ? pThread->getUniqueID() : "?");
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

	if(pThread != NULL && pThread->getRunningStatus() == true) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());

		pThread->signalQuit();
		//sleep(0);

		for( time_t elapsed = time(NULL); difftime(time(NULL),elapsed) <= 7; ) {
			if(pThread->getRunningStatus() == false) {
				break;
			}
			sleep(1);
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		//sleep(0);
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
	}
	//sleep(0);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] END\n",__FILE__,__FUNCTION__,__LINE__,uniqueID.c_str());
}

void BaseThread::shutdownAndWait() {
	BaseThread::shutdownAndWait(this);
}

}}//end namespace
