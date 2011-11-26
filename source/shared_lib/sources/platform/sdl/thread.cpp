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
#include <algorithm>
#include "platform_common.h"

using namespace std;

namespace Shared { namespace Platform {

Mutex Thread::mutexthreadList;
std::vector<Thread *> Thread::threadList;

// =====================================
//          Threads
// =====================================
Thread::Thread() {
	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	Thread::threadList.push_back(this);
	safeMutex.ReleaseLock();
	thread = NULL;
}

Thread::~Thread() {
	if(thread != NULL) {
		SDL_WaitThread(thread, NULL);
		thread = NULL;
	}

	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	std::vector<Thread *>::iterator iterFind = std::find(Thread::threadList.begin(),Thread::threadList.end(),this);
	Thread::threadList.erase(iterFind);
	safeMutex.ReleaseLock();
}

std::vector<Thread *> Thread::getThreadList() {
	std::vector<Thread *> result;
	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	result = threadList;
	safeMutex.ReleaseLock();
	return result;
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

class SDLMutexSafeWrapper {
protected:
	SDL_mutex *mutex;

public:

	SDLMutexSafeWrapper(SDL_mutex *mutex) {
		this->mutex = mutex;
		Lock();
	}
	~SDLMutexSafeWrapper() {
		ReleaseLock();
	}

	void Lock() {
		if(this->mutex != NULL) {
			SDL_mutexP(this->mutex);
		}
	}
	void ReleaseLock(bool keepMutex=false) {
		if(this->mutex != NULL) {
			SDL_mutexV(this->mutex);

			if(keepMutex == false) {
				this->mutex = NULL;
			}
		}
	}
};

Mutex::Mutex(string ownerId) {
	mutexAccessor  = SDL_CreateMutex();
	SDLMutexSafeWrapper safeMutex(mutexAccessor);
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
	SDLMutexSafeWrapper safeMutex(mutexAccessor);
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

int Semaphore::waitTillSignalled(int waitMilliseconds) {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}
	int semValue = 0;
	if(waitMilliseconds >= 0) {
		semValue = SDL_SemWaitTimeout(semaphore,waitMilliseconds);
	}
	else {
		semValue = SDL_SemWait(semaphore);
	}
	return semValue;
}

uint32 Semaphore::getSemValue() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",__FILE__,__FUNCTION__,__LINE__);
		throw runtime_error(szBuf);
	}

	return SDL_SemValue(semaphore);
}

ReadWriteMutex::ReadWriteMutex(int maxReaders) : semaphore(maxReaders) {
	this->maxReadersCount = maxReaders;
}

void ReadWriteMutex::LockRead() {
	semaphore.waitTillSignalled();
}
void ReadWriteMutex::UnLockRead() {
	semaphore.signal();
}
void ReadWriteMutex::LockWrite() {
	MutexSafeWrapper safeMutex(&mutex);
	uint32 totalLocks = maxReaders();
	for (int i = 0; i < totalLocks; ++i) {
		semaphore.waitTillSignalled();
	}
}
void ReadWriteMutex::UnLockWrite() {
	uint32 totalLocks = maxReaders();
	for (int i = 0; i < totalLocks; ++i) {
		semaphore.signal();
	}
}
int ReadWriteMutex::maxReaders() {
	//return semaphore.getSemValue();
	return this->maxReadersCount;
}


}}//end namespace
