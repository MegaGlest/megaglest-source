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
#include "platform_util.h"
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
		snprintf(szBuf,1023,"In [%s::%s Line: %d] thread == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
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
		snprintf(szBuf,1023,"In [%s::%s Line: %d] thread == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
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
	SDL_mutex **mutex;
	bool destroyMutexInDestructor;

public:

	SDLMutexSafeWrapper(SDL_mutex **mutex, bool destroyMutexInDestructor=false) {
		this->mutex = mutex;
		this->destroyMutexInDestructor = destroyMutexInDestructor;
		Lock();
	}
	~SDLMutexSafeWrapper() {
		bool keepMutex = (this->destroyMutexInDestructor == true && mutex != NULL && *mutex != NULL);
		ReleaseLock(keepMutex);

		if(this->destroyMutexInDestructor == true && mutex != NULL && *mutex != NULL) {
			SDL_DestroyMutex(*mutex);
			*mutex = NULL;
			mutex = NULL;
		}
	}

	void Lock() {
		if(mutex != NULL && *mutex != NULL) {
			SDL_mutexP(*mutex);
		}
	}
	void ReleaseLock(bool keepMutex=false) {
		if(mutex != NULL && *mutex != NULL) {
			SDL_mutexV(*mutex);

			if(keepMutex == false) {
				mutex = NULL;
			}
		}
	}
};

Mutex::Mutex(string ownerId) {
	mutexAccessor  = SDL_CreateMutex();
	SDLMutexSafeWrapper safeMutex(&mutexAccessor);
    refCount=0;
    this->ownerId = ownerId;
	mutex = SDL_CreateMutex();
	assert(mutex != NULL);
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	deleteownerId = "";
}

Mutex::~Mutex() {
	SDLMutexSafeWrapper safeMutex(&mutexAccessor,true);
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
		//printf("%s\n",szBuf);
	}
	else if(refCount >= 1) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] about to destroy mutex refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
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
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
	}
	SDL_mutexP(mutex);
	refCount++;
}

void Mutex::v() {
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
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
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
}

Semaphore::~Semaphore() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	SDL_DestroySemaphore(semaphore);
	semaphore = NULL;
}

void Semaphore::signal() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	SDL_SemPost(semaphore);
}

int Semaphore::waitTillSignalled(int waitMilliseconds) {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
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

bool Semaphore::tryDecrement() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	int semValue = SDL_SemTryWait(semaphore);
	return (semValue == 0);
}

uint32 Semaphore::getSemValue() {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	return SDL_SemValue(semaphore);
}

void Semaphore::resetSemValue(Uint32 initialValue) {
	if(semaphore == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	uint32 currentValue = SDL_SemValue(semaphore);
	for(unsigned int i = currentValue; i < initialValue; ++i) {
		SDL_SemPost(semaphore);
	}
}

// =====================================================
//	class ReadWriteMutex
// =====================================================

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
	for(unsigned int i = 0; i < totalLocks; ++i) {
		semaphore.waitTillSignalled();
	}
}
void ReadWriteMutex::UnLockWrite() {
	uint32 totalLocks = maxReaders();
	for(unsigned int i = 0; i < totalLocks; ++i) {
		semaphore.signal();
	}
}
int ReadWriteMutex::maxReaders() {
	//return semaphore.getSemValue();
	return this->maxReadersCount;
}

// =====================================================
//	class Trigger
// =====================================================

Trigger::Trigger(Mutex *mutex) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	this->mutex = mutex;
	this->trigger = SDL_CreateCond();
	if(this->trigger == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

Trigger::~Trigger() {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(trigger == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	SDL_DestroyCond(trigger);
	trigger = NULL;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	this->mutex = NULL;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Trigger::signal(bool allThreads) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(trigger == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	//MutexSafeWrapper safeMutex(mutex);

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(allThreads == false) {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		int result = SDL_CondSignal(trigger);

		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,result);
	}
	else {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		int result = SDL_CondBroadcast(trigger);

		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,result);
	}

	//safeMutex.ReleaseLock();

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

int Trigger::waitTillSignalled(Mutex *mutex, int waitMilliseconds) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(trigger == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	if(mutex == NULL) {
		char szBuf[1024]="";
		snprintf(szBuf,1023,"In [%s::%s Line: %d] mutex == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	int result = 0;
	if(waitMilliseconds >= 0) {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		result = SDL_CondWaitTimeout(trigger,mutex->getMutex(),waitMilliseconds);
	}
	else {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		result = SDL_CondWait(trigger, mutex->getMutex());
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return result;
}

MasterSlaveThreadController::MasterSlaveThreadController() {
	std::vector<SlaveThreadControllerInterface *> empty;
	init(empty);
}

MasterSlaveThreadController::MasterSlaveThreadController(std::vector<SlaveThreadControllerInterface *> &slaveThreadList) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] ==========================================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	init(slaveThreadList);
}

void MasterSlaveThreadController::init(std::vector<SlaveThreadControllerInterface *> &newSlaveThreadList) {
	static string masterSlaveOwnerId = string(__FILE__) + string("_MasterSlaveThreadController");
	this->mutex = new Mutex(masterSlaveOwnerId);
	this->slaveTriggerSem = new Semaphore(0);
	this->slaveTriggerCounter = newSlaveThreadList.size() + triggerBaseCount;
	setSlaves(newSlaveThreadList);
}

void MasterSlaveThreadController::clearSlaves(bool clearListOnly) {
	if(this->slaveThreadList.empty() == false) {
		if(clearListOnly == false) {
			for(unsigned int i = 0; i < this->slaveThreadList.size(); ++i) {
				SlaveThreadControllerInterface *slave = this->slaveThreadList[i];
				slave->setMasterController(NULL);
			}
		}
		this->slaveThreadList.clear();
	}
}

MasterSlaveThreadController::~MasterSlaveThreadController() {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	clearSlaves();

	delete slaveTriggerSem;
	slaveTriggerSem = NULL;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] mutex->getRefCount() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,mutex->getRefCount());

	delete mutex;
	mutex = NULL;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MasterSlaveThreadController::setSlaves(std::vector<SlaveThreadControllerInterface *> &slaveThreadList) {
	this->slaveThreadList = slaveThreadList;

	if(this->slaveThreadList.empty() == false) {
		for(unsigned int i = 0; i < this->slaveThreadList.size(); ++i) {
			SlaveThreadControllerInterface *slave = this->slaveThreadList[i];
			slave->setMasterController(this);
		}
	}
}

void MasterSlaveThreadController::signalSlaves(void *userdata) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	slaveTriggerCounter = this->slaveThreadList.size() + triggerBaseCount;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->slaveThreadList.empty() == false) {
		for(unsigned int i = 0; i < this->slaveThreadList.size(); ++i) {
			SlaveThreadControllerInterface *slave = this->slaveThreadList[i];
			slave->signalSlave(userdata);
		}
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MasterSlaveThreadController::triggerMaster(int waitMilliseconds) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(mutex);
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] semVal = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

	slaveTriggerCounter--;
	int newCount = slaveTriggerCounter;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] slaveTriggerCounter = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

	safeMutex.ReleaseLock();

	if(newCount <= triggerBaseCount) {
		slaveTriggerSem->signal();
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

bool MasterSlaveThreadController::waitTillSlavesTrigger(int waitMilliseconds) {
	bool result = true;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] slaveTriggerCounter = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

	if(this->slaveThreadList.empty() == false) {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] slaveTriggerCounter = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

		int slaveResult = slaveTriggerSem->waitTillSignalled(waitMilliseconds);

		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] slaveTriggerCounter = %d slaveResult = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter,slaveResult);

		if(slaveResult != 0) {
			if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			result = false;
		}
		else if(slaveResult == 0) {
			if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] slaveTriggerCounter = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

			result = true;
		}
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return result;
}

}}//end namespace
