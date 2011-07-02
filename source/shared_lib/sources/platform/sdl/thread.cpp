//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.

#include "thread.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <assert.h>

#include "noimpl.h"

using namespace std;

#ifdef WIN32

#define snprintf _snprintf

#endif

namespace Shared{ namespace Platform{

// =====================================
//          Threads
// =====================================
Thread::Thread() {
	thread = NULL;
}

Thread::~Thread() {
	if(thread != NULL) {
		SDL_WaitThread(thread, NULL);
		thread = NULL;
	}
}

void Thread::start() {
	thread = SDL_CreateThread(beginExecution, this);
	assert(thread != NULL);
	if(thread == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] thread == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
}

void Thread::setPriority(Thread::Priority threadPriority) {
	NOIMPL;
}

int Thread::beginExecution(void* data) {
	Thread* thread = static_cast<Thread*> (data);
	assert(thread != NULL);
	if(thread == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] thread == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	thread->execute();
	return 0;
}

void Thread::suspend() {
	NOIMPL;
}

void Thread::resume() {
	NOIMPL;
}

// =====================================
//          Mutex
// =====================================

Mutex::Mutex(string ownerId) {
    refCount=0;
    this->ownerId = ownerId;
	mutex = SDL_CreateMutex();
	assert(mutex != NULL);
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	deleteownerId = "";
}

Mutex::~Mutex() {
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",__FILE__,__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw runtime_error(szBuf);
		//printf("%s\n",szBuf);
	}
	else if(refCount >= 1) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] about to destroy mutex refCount = %d owner [%s] deleteownerId [%s]",__FILE__,__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw runtime_error(szBuf);
	}

	if(mutex != NULL) {
		deleteownerId = ownerId;
		SDL_DestroyMutex(mutex);
		mutex=NULL;
	}
}

void Mutex::p() {
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",__FILE__,__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw runtime_error(szBuf);
	}
	SDL_mutexP(mutex);
	refCount++;
}

void Mutex::v() {
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",__FILE__,__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw runtime_error(szBuf);
	}
	refCount--;
	SDL_mutexV(mutex);
}

// =====================================================
//	class Semaphore
// =====================================================

Semaphore::Semaphore(Uint32 initialValue) {
	semaphore = SDL_CreateSemaphore(initialValue);
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
}

Semaphore::~Semaphore() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	SDL_DestroySemaphore(semaphore);
	semaphore = NULL;
}

void Semaphore::signal() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	SDL_SemPost(semaphore);
}

int Semaphore::waitTillSignalled() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	int semValue = SDL_SemWait(semaphore);
	return semValue;
}

}}//end namespace
