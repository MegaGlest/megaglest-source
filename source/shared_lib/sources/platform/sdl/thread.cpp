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
#include "base_thread.h"
#include "time.h"
#include <memory>

using namespace std;

namespace Shared { namespace Platform {

bool Thread::enableVerboseMode = false;
Mutex Thread::mutexthreadList;
vector<Thread *> Thread::threadList;

auto_ptr<Mutex> Mutex::mutexMutexList(new Mutex(CODE_AT_LINE));
vector<Mutex *> Mutex::mutexList;

class ThreadGarbageCollector;
class Mutex;
class MutexSafeWrapper;

static auto_ptr<ThreadGarbageCollector> cleanupThread;
static auto_ptr<Mutex> cleanupThreadMutex(new Mutex());

class ThreadGarbageCollector : public BaseThread
{
protected:
	Mutex mutexPendingCleanupList;
	vector<Thread *> pendingCleanupList;

	bool cleanupPendingThreads() {
		MutexSafeWrapper safeMutex(&mutexPendingCleanupList);
		if(pendingCleanupList.empty() == false) {
			for(unsigned int index = 0; index < pendingCleanupList.size(); ++index) {
				Thread *thread = pendingCleanupList[index];
				cleanupPendingThread(thread);
			}
			pendingCleanupList.clear();
			return true;
		}
		return false;
	}

public:
	ThreadGarbageCollector() : BaseThread() {
		if(Thread::getEnableVerboseMode()) printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);
		uniqueID = "ThreadGarbageCollector";
		removeThreadFromList();
	}
	virtual ~ThreadGarbageCollector() {
		if(Thread::getEnableVerboseMode()) {
			printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);
			string stack = PlatformExceptionHandler::getStackTrace();
			printf("In %s Line: %d this: %p stack: %s\n",__FUNCTION__,__LINE__,this,stack.c_str());
		}
	}
    virtual void execute() {
    	if(Thread::getEnableVerboseMode()) printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);

        RunningStatusSafeWrapper runningStatus(this);
        for(;getQuitStatus() == false;) {
			if(cleanupPendingThreads() == false) {
				if(getQuitStatus() == false) {
					sleep(200);
				}
			}
		}

        if(Thread::getEnableVerboseMode()) printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);

		cleanupPendingThreads();

		if(Thread::getEnableVerboseMode()) printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);
	}
    void addThread(Thread *thread) {
    	if(Thread::getEnableVerboseMode()) printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);

		MutexSafeWrapper safeMutex(&mutexPendingCleanupList);
		pendingCleanupList.push_back(thread);
		safeMutex.ReleaseLock();

		if(Thread::getEnableVerboseMode()) printf("In %s Line: %d this: %p\n",__FUNCTION__,__LINE__,this);
	}

	static void cleanupPendingThread(Thread *thread) {
		if(thread != NULL) {
			BaseThread *base_thread = dynamic_cast<BaseThread *>(thread);
			if(base_thread != NULL &&
					(base_thread->getRunningStatus() == true || base_thread->getExecutingTask() == true)) {
				base_thread->signalQuit();
				sleep(10);
				if(base_thread->getRunningStatus() == true || base_thread->getExecutingTask() == true) {

					if(Thread::getEnableVerboseMode()) printf("\n\n\n$$$$$$$$$$$$$$$$$$$$$$$$$$$ cleanupPendingThread Line: %d thread = %p [%s]\n",__LINE__,thread,base_thread->getUniqueID().c_str());

					char szBuf[8096]="";
					snprintf(szBuf,8095,"In [%s::%s Line: %d] cannot delete active thread: getRunningStatus(): %d getExecutingTask: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,base_thread->getRunningStatus(),base_thread->getExecutingTask());
					throw megaglest_runtime_error(szBuf);
				}
			}

			if(Thread::getEnableVerboseMode()) printf("!!!! cleanupPendingThread Line: %d thread = %p [%s]\n",__LINE__,thread,(base_thread != NULL ? base_thread->getUniqueID().c_str() : "n/a"));

			delete thread;

			if(Thread::getEnableVerboseMode()) printf("!!!! cleanupPendingThread Line: %d thread = NULL [%s]\n",__LINE__,(base_thread != NULL ? base_thread->getUniqueID().c_str() : "n/a"));
		}
	}

};

// =====================================
//          Threads
// =====================================
Thread::Thread() : thread(NULL), mutexthreadAccessor(new Mutex()), deleteAfterExecute(false) {
	addThreadToList();
}

void Thread::addThreadToList() {
	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	Thread::threadList.push_back(this);
	safeMutex.ReleaseLock();
}
void Thread::removeThreadFromList() {
	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	std::vector<Thread *>::iterator iterFind = std::find(Thread::threadList.begin(),Thread::threadList.end(),this);
	if(iterFind == Thread::threadList.end()) {
		if(this != cleanupThread.get()) {
			char szBuf[8096]="";
			snprintf(szBuf,8095,"In [%s::%s Line: %d] iterFind == Thread::threadList.end()",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
	}
	else {
		Thread::threadList.erase(iterFind);
	}
	safeMutex.ReleaseLock();
}

void Thread::shutdownThreads() {
	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	for(unsigned int index = 0; index < Thread::threadList.size(); ++index) {
		BaseThread *thread = dynamic_cast<BaseThread *>(Thread::threadList[index]);
		if(thread && thread->getRunningStatus() == true) {
			thread->signalQuit();
		}
	}
	safeMutex.ReleaseLock();

	if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d\n",__LINE__);
	if(cleanupThread.get() != 0) {
		//printf("In Thread::shutdownThreads Line: %d\n",__LINE__);
		sleep(0);
		cleanupThread->signalQuit();

		//printf("In Thread::shutdownThreads Line: %d\n",__LINE__);
		time_t elapsed = time(NULL);
		for(;cleanupThread->getRunningStatus() == true &&
    		difftime((long int)time(NULL),elapsed) <= 5;) {
			sleep(100);
		}
		//printf("In Thread::shutdownThreads Line: %d\n",__LINE__);
		//sleep(100);

		MutexSafeWrapper safeMutex(cleanupThreadMutex.get());
		cleanupThread.reset(0);
		//printf("In Thread::shutdownThreads Line: %d\n",__LINE__);
	}
}

Thread::~Thread() {
	if(Thread::getEnableVerboseMode()) printf("In ~Thread Line: %d [%p] thread = %p\n",__LINE__,this,thread);

	MutexSafeWrapper safeMutex(mutexthreadAccessor);
	if(thread != NULL) {
		SDL_WaitThread(thread, NULL);
		thread = NULL;
	}
	safeMutex.ReleaseLock();

	if(Thread::getEnableVerboseMode()) printf("In ~Thread Line: %d [%p] thread = %p\n",__LINE__,this,thread);

	removeThreadFromList();

	delete mutexthreadAccessor;
	mutexthreadAccessor = NULL;
	if(Thread::getEnableVerboseMode()) printf("In ~Thread Line: %d [%p] thread = %p\n",__LINE__,this,thread);
}

std::vector<Thread *> Thread::getThreadList() {
	std::vector<Thread *> result;
	MutexSafeWrapper safeMutex(&Thread::mutexthreadList);
	result = threadList;
	safeMutex.ReleaseLock();
	return result;
}

void Thread::start() {
	MutexSafeWrapper safeMutex(mutexthreadAccessor);

	BaseThread *base_thread = dynamic_cast<BaseThread *>(this);
	if(base_thread) base_thread->setStarted(true);

	thread = SDL_CreateThread(beginExecution, this);
	//assert(thread != NULL);
	if(thread == NULL) {
		if(base_thread) base_thread->setStarted(false);

		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] thread == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	//printf("In Thread::start Line: %d [%p] thread = %p\n",__LINE__,this,thread);
}

bool Thread::threadObjectValid() {
	MutexSafeWrapper safeMutex(mutexthreadAccessor);
	return (thread != NULL);
}

void Thread::setPriority(Thread::Priority threadPriority) {
	NOIMPL;
}

int Thread::beginExecution(void* data) {
	Thread* thread = static_cast<Thread*> (data);
	assert(thread != NULL);
	if(thread == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] thread == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	BaseThread *base_thread = dynamic_cast<BaseThread *>(thread);
	//ThreadGarbageCollector *garbage_collector = dynamic_cast<ThreadGarbageCollector *>(thread);
	if(Thread::getEnableVerboseMode()) printf("In Thread::execute Line: %d thread = %p base_thread = %p [%s]\n",__LINE__,thread,base_thread,(base_thread != NULL ? base_thread->getUniqueID().c_str() : "n/a"));

	if(thread->threadObjectValid() == true) {
		thread->execute();
	}

	if(Thread::getEnableVerboseMode()) printf("In Thread::execute Line: %d thread = %p base_thread = %p [%s]\n",__LINE__,thread,base_thread,(base_thread != NULL ? base_thread->getUniqueID().c_str() : "n/a"));

	if(thread->threadObjectValid() == true) {
		thread->queueAutoCleanThread();
	}

	if(Thread::getEnableVerboseMode()) printf("In Thread::execute Line: %d\n",__LINE__);

	return 0;
}

void Thread::queueAutoCleanThread() {
	if(this->deleteAfterExecute == true) {
		if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d\n",__LINE__);

		BaseThread *base_thread = dynamic_cast<BaseThread *>(this);
		//ThreadGarbageCollector *garbage_collector = dynamic_cast<ThreadGarbageCollector *>(this);
		if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d thread = %p base_thread = %p [%s]\n",__LINE__,this,base_thread,(base_thread != NULL ? base_thread->getUniqueID().c_str() : "n/a"));

		MutexSafeWrapper safeMutex(cleanupThreadMutex.get());
		if(cleanupThread.get() == NULL) {
			if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d\n",__LINE__);
			cleanupThread.reset(new ThreadGarbageCollector());

			safeMutex.ReleaseLock();
			cleanupThread->start();
		}
		else {
			safeMutex.ReleaseLock();
		}
		for(time_t elapsed = time(NULL);
				cleanupThread->getRunningStatus() == false &&
					cleanupThread->getQuitStatus() == false &&
						difftime(time(NULL),elapsed) < 3;) {
			sleep(0);
		}

		if(cleanupThread->getQuitStatus() == true) {
			//ThreadGarbageCollector::cleanupPendingThread(this);
			if(cleanupThread->getRunningStatus() == false) {
				if(Thread::getEnableVerboseMode()) printf("MANUAL DELETE In Thread::shutdownThreads Line: %d this [%p]\n",__LINE__,this);

				cleanupThread->addThread(this);
				cleanupThread->start();

				if(Thread::getEnableVerboseMode()) printf("MANUAL DELETE In Thread::shutdownThreads Line: %d\n",__LINE__);
				return;
			}
			else {
				if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d this [%p]\n",__LINE__,this);

				for(time_t elapsed = time(NULL);
						cleanupThread->getRunningStatus() == true &&
							cleanupThread->getQuitStatus() == true &&
								difftime(time(NULL),elapsed) < 3;) {
					sleep(0);
				}
				sleep(0);

				if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d this [%p]\n",__LINE__,this);

				cleanupThread->addThread(this);
				cleanupThread->start();
			}
		}
		else {
			if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d this [%p]\n",__LINE__,this);
			cleanupThread->addThread(this);
		}
		if(Thread::getEnableVerboseMode()) printf("In Thread::shutdownThreads Line: %d this [%p]\n",__LINE__,this);
	}
}

void Thread::kill() {
	MutexSafeWrapper safeMutex(mutexthreadAccessor);
	SDL_KillThread(thread);
	thread = NULL;
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

const bool debugMutexLock = false;
const int debugMutexLockMillisecondThreshold = 2000;

Mutex::Mutex(string ownerId) {
	isStaticMutexListMutex = false;
	mutexAccessor  = SDL_CreateMutex();
	SDLMutexSafeWrapper safeMutex(&mutexAccessor);
    refCount=0;
    this->ownerId = ownerId;
    this->lastownerId = "";
	mutex = SDL_CreateMutex();
	assert(mutex != NULL);
	if(mutex == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] mutex == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	deleteownerId = "";

	chronoPerf = NULL;
	if(debugMutexLock == true) {
		chronoPerf = new Chrono();
	}

	if(Mutex::mutexMutexList.get()) {
		MutexSafeWrapper safeMutexX(Mutex::mutexMutexList.get());
		Mutex::mutexList.push_back(this);
		safeMutexX.ReleaseLock();
	}
	else {
		isStaticMutexListMutex = true;
	}
}

Mutex::~Mutex() {
	if(Mutex::mutexMutexList.get() && isStaticMutexListMutex == false) {
		MutexSafeWrapper safeMutexX(Mutex::mutexMutexList.get());
		std::vector<Mutex *>::iterator iterFind = std::find(Mutex::mutexList.begin(),Mutex::mutexList.end(),this);
		if(iterFind == Mutex::mutexList.end()) {
			char szBuf[8096]="";
			snprintf(szBuf,8095,"In [%s::%s Line: %d] iterFind == Mutex::mutexList.end()",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		Mutex::mutexList.erase(iterFind);
		safeMutexX.ReleaseLock();
	}

	SDLMutexSafeWrapper safeMutex(&mutexAccessor,true);
	if(mutex == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
		//printf("%s\n",szBuf);
	}
	else if(refCount >= 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] about to destroy mutex refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
	}

	if(debugMutexLock == true) {
		delete chronoPerf;
		chronoPerf = NULL;
	}

	if(mutex != NULL) {
		deleteownerId = ownerId;
		SDL_DestroyMutex(mutex);
		mutex=NULL;
	}
}

void Mutex::p() {
	if(mutex == NULL) {

		string stack = PlatformExceptionHandler::getStackTrace();

		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s] stack: %s",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str(),stack.c_str());
		throw megaglest_runtime_error(szBuf);
	}
	std::auto_ptr<Chrono> chronoLockPerf;
	if(debugMutexLock == true) {
		chronoLockPerf.reset(new Chrono());
		chronoLockPerf->start();
	}

	SDL_mutexP(mutex);
	refCount++;

	if(debugMutexLock == true) {
		if(chronoLockPerf->getMillis() >= debugMutexLockMillisecondThreshold) {
			printf("\n**WARNING possible mutex lock detected ms [%lld] Last ownerid: [%s]\n",(long long int)chronoLockPerf->getMillis(),lastownerId.c_str());
		}
		chronoPerf->start();
	}
}

void Mutex::v() {
	if(mutex == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] mutex == NULL refCount = %d owner [%s] deleteownerId [%s]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,refCount,ownerId.c_str(),deleteownerId.c_str());
		throw megaglest_runtime_error(szBuf);
	}
	refCount--;

	if(debugMutexLock == true) {
		lastownerId = ownerId;
		if(chronoPerf->getMillis() >= debugMutexLockMillisecondThreshold) {
			printf("About to get stacktrace for stuck mutex ...\n");
			string oldLastownerId = lastownerId;
			lastownerId = PlatformExceptionHandler::getStackTrace();

			printf("\n**WARNING possible mutex lock (on unlock) detected ms [%lld] Last ownerid: [%s]\noldLastownerId: [%s]\n",(long long int)chronoPerf->getMillis(),lastownerId.c_str(),oldLastownerId.c_str());
		}
	}
	SDL_mutexV(mutex);
}

// =====================================================
//	class Semaphore
// =====================================================

Semaphore::Semaphore(Uint32 initialValue) {
	semaphore = SDL_CreateSemaphore(initialValue);
	if(semaphore == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
}

Semaphore::~Semaphore() {
	if(semaphore == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	SDL_DestroySemaphore(semaphore);
	semaphore = NULL;
}

void Semaphore::signal() {
	if(semaphore == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	SDL_SemPost(semaphore);
}

int Semaphore::waitTillSignalled(int waitMilliseconds) {
	if(semaphore == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	int semValue = SDL_SemTryWait(semaphore);
	return (semValue == 0);
}

uint32 Semaphore::getSemValue() {
	if(semaphore == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	return SDL_SemValue(semaphore);
}

void Semaphore::resetSemValue(Uint32 initialValue) {
	if(semaphore == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] semaphore == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

Trigger::~Trigger() {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(trigger == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] trigger == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	if(mutex == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8095,"In [%s::%s Line: %d] mutex == NULL",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
	this->slaveTriggerCounter = (int)newSlaveThreadList.size() + triggerBaseCount;
	setSlaves(newSlaveThreadList);
}

void MasterSlaveThreadController::clearSlaves(bool clearListOnly) {
	if(this->slaveThreadList.empty() == false) {
		if(clearListOnly == false) {
			for(unsigned int i = 0; i < this->slaveThreadList.size(); ++i) {
				SlaveThreadControllerInterface *slave = this->slaveThreadList[i];
				if(slave != NULL) {
					slave->setMasterController(NULL);
				}
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
			if(slave != NULL) {
				slave->setMasterController(this);
			}
		}
	}
}

void MasterSlaveThreadController::signalSlaves(void *userdata) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	slaveTriggerCounter = (int)this->slaveThreadList.size() + triggerBaseCount;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(this->slaveThreadList.empty() == false) {
		for(unsigned int i = 0; i < this->slaveThreadList.size(); ++i) {
			SlaveThreadControllerInterface *slave = this->slaveThreadList[i];
			if(slave != NULL) {
				slave->signalSlave(userdata);
			}
		}
	}

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MasterSlaveThreadController::triggerMaster(int waitMilliseconds) {
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(mutex);
	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] semVal = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);
	//printf("In [%s::%s Line: %d] semVal = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

	slaveTriggerCounter--;
	int newCount = slaveTriggerCounter;

	if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d] slaveTriggerCounter = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

	safeMutex.ReleaseLock();

	//printf("In [%s::%s Line: %d] semVal = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slaveTriggerCounter);

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
