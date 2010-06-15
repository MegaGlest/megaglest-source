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

	setQuitStatus(false);
	setRunningStatus(false);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

BaseThread::~BaseThread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	shutdownAndWait();
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BaseThread::signalQuit() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setQuitStatus(true);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BaseThread::setQuitStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(&mutexQuit);
	quit = value;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	MutexSafeWrapper safeMutex(&mutexRunning);
	retval = running;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] running = %d\n",__FILE__,__FUNCTION__,__LINE__,retval);

	return retval;
}

void BaseThread::setRunningStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	MutexSafeWrapper safeMutex(&mutexRunning);
	running = value;
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] running = %d\n",__FILE__,__FUNCTION__,__LINE__,value);
}

void BaseThread::shutdownAndWait(BaseThread *pThread) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(pThread != NULL && pThread->getRunningStatus() == true) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		pThread->signalQuit();
		for( time_t elapsed = time(NULL); difftime(time(NULL),elapsed) <= 7; ) {
			if(pThread->getRunningStatus() == false) {
				break;
			}
			sleep(1);
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		sleep(1);
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	sleep(0);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BaseThread::shutdownAndWait() {
	BaseThread::shutdownAndWait(this);
}

}}//end namespace
