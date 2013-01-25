// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_THREAD_H_
#define _SHARED_PLATFORM_THREAD_H_

//#define DEBUG_MUTEXES
//#define DEBUG_PERFORMANCE_MUTEXES

#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <string>

#include "data_types.h"
#ifdef DEBUG_PERFORMANCE_MUTEXES
#include "platform_common.h"
#endif

#include <vector>
//#include "leak_dumper.h"

// =====================================================
//	class Thread
// =====================================================

using namespace std;
#ifdef DEBUG_PERFORMANCE_MUTEXES
using namespace Shared::PlatformCommon;
#endif

namespace Shared { namespace Platform {

class Mutex;
//class uint32;

class Thread {
public:
	enum Priority {
		pIdle = 0,
		pLow = 1,
		pNormal = 2,
		pHigh = 3,
		pRealTime = 4
	};

private:
	SDL_Thread* thread;

	static Mutex mutexthreadList;
	static std::vector<Thread *> threadList;

public:
	Thread();
	virtual ~Thread();

	static std::vector<Thread *> getThreadList();

	void start();
	virtual void execute()=0;
	void setPriority(Thread::Priority threadPriority);
	void suspend();
	void resume();

private:
	static int beginExecution(void *param);
};

// =====================================================
//	class Mutex
// =====================================================

class Mutex {
private:

	SDL_mutex* mutex;
	int refCount;
	string ownerId;
	string deleteownerId;

	SDL_mutex* mutexAccessor;

public:
	Mutex(string ownerId="");
	~Mutex();
	void setOwnerId(string ownerId) { this->ownerId = ownerId; }
	void p();
	void v();
	int getRefCount() const { return refCount; }

	SDL_mutex* getMutex() { return mutex; }
};

class MutexSafeWrapper {
protected:
	Mutex *mutex;
	string ownerId;
#ifdef DEBUG_PERFORMANCE_MUTEXES
	Chrono chrono;
#endif

public:

	MutexSafeWrapper(Mutex *mutex,string ownerId="") {
		this->mutex = mutex;
		this->ownerId = ownerId;
		Lock();
	}
	~MutexSafeWrapper() {
		ReleaseLock();
	}

    void setMutex(Mutex *mutex,string ownerId="") {
		this->mutex = mutex;
		this->ownerId = ownerId;
		Lock();
    }
    bool isValidMutex() const {
        return(this->mutex != NULL);
    }

	void Lock() {
		if(this->mutex != NULL) {
		    #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("Locking Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif

#ifdef DEBUG_PERFORMANCE_MUTEXES
    		chrono.start();
#endif

			this->mutex->p();

#ifdef DEBUG_PERFORMANCE_MUTEXES
			if(chrono.getMillis() > 5) printf("In [%s::%s Line: %d] MUTEX LOCK took msecs: %lld, this->mutex->getRefCount() = %d ownerId [%s]\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis(),this->mutex->getRefCount(),ownerId.c_str());
			chrono.start();
#endif

            #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("Locked Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif
		}
	}
	void ReleaseLock(bool keepMutex=false) {
		if(this->mutex != NULL) {
		    #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("UnLocking Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif

			this->mutex->v();

#ifdef DEBUG_PERFORMANCE_MUTEXES
			if(chrono.getMillis() > 100) printf("In [%s::%s Line: %d] MUTEX UNLOCKED and held locked for msecs: %lld, this->mutex->getRefCount() = %d ownerId [%s]\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis(),this->mutex->getRefCount(),ownerId.c_str());
#endif

            #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("UnLocked Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif

			if(keepMutex == false) {
				this->mutex = NULL;
			}
		}
	}
};

// =====================================================
//	class Semaphore
// =====================================================

class Semaphore {
private:
	SDL_sem* semaphore;

public:
	Semaphore(Uint32 initialValue = 0);
	~Semaphore();
	void signal();
	int waitTillSignalled(int waitMilliseconds=-1);
	bool tryDecrement();

	uint32 getSemValue();
	void resetSemValue(Uint32 initialValue);
};


class ReadWriteMutex
{
public:
	ReadWriteMutex(int maxReaders = 32);

	void LockRead();
	void UnLockRead();

	void LockWrite();
	void UnLockWrite();

	int maxReaders();
	void setOwnerId(string ownerId) { this->ownerId = ownerId; }

private:
	Semaphore semaphore;
	Mutex mutex;
	int maxReadersCount;

	string ownerId;
};


class ReadWriteMutexSafeWrapper {
protected:
	ReadWriteMutex *mutex;
	string ownerId;
	bool isReadLock;

#ifdef DEBUG_PERFORMANCE_MUTEXES
	Chrono chrono;
#endif

public:

	ReadWriteMutexSafeWrapper(ReadWriteMutex *mutex,bool isReadLock=true, string ownerId="") {
		this->mutex = mutex;
		this->isReadLock = isReadLock;
		this->ownerId = ownerId;
		Lock();
	}
	~ReadWriteMutexSafeWrapper() {
		ReleaseLock();
	}

    void setReadWriteMutex(ReadWriteMutex *mutex,bool isReadLock=true,string ownerId="") {
		this->mutex = mutex;
		this->isReadLock = isReadLock;
		this->ownerId = ownerId;
		Lock();
    }
    bool isValidReadWriteMutex() const {
        return(this->mutex != NULL);
    }

	void Lock() {
		if(this->mutex != NULL) {
		    #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("Locking Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif

#ifdef DEBUG_PERFORMANCE_MUTEXES
    		chrono.start();
#endif

    		if(this->isReadLock == true) {
    			this->mutex->LockRead();
    		}
    		else {
    			this->mutex->LockWrite();
    		}

#ifdef DEBUG_PERFORMANCE_MUTEXES
			if(chrono.getMillis() > 5) printf("In [%s::%s Line: %d] MUTEX LOCK took msecs: %lld, this->mutex->getRefCount() = %d ownerId [%s]\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis(),this->mutex->getRefCount(),ownerId.c_str());
			chrono.start();
#endif

            #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("Locked Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif
		}
	}
	void ReleaseLock(bool keepMutex=false) {
		if(this->mutex != NULL) {
		    #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("UnLocking Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif

    		if(this->isReadLock == true) {
    			this->mutex->UnLockRead();
    		}
    		else {
    			this->mutex->UnLockWrite();
    		}

#ifdef DEBUG_PERFORMANCE_MUTEXES
			if(chrono.getMillis() > 100) printf("In [%s::%s Line: %d] MUTEX UNLOCKED and held locked for msecs: %lld, this->mutex->getRefCount() = %d ownerId [%s]\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis(),this->mutex->getRefCount(),ownerId.c_str());
#endif

            #ifdef DEBUG_MUTEXES
            if(ownerId != "") {
                printf("UnLocked Mutex [%s] refCount: %d\n",ownerId.c_str(),this->mutex->getRefCount());
            }
            #endif

			if(keepMutex == false) {
				this->mutex = NULL;
			}
		}
	}
};

const bool debugMasterSlaveThreadController = false;
// =====================================================
//	class Trigger
// =====================================================

class Trigger {
private:
	SDL_cond* trigger;
	Mutex *mutex;

public:
	Trigger(Mutex *mutex);
	~Trigger();
	void signal(bool allThreads=false);
	int waitTillSignalled(Mutex *mutex, int waitMilliseconds=-1);
};

class MasterSlaveThreadController;

class SlaveThreadControllerInterface {
public:
	virtual void setMasterController(MasterSlaveThreadController *master) = 0;
	virtual void signalSlave(void *userdata) = 0;
	virtual ~SlaveThreadControllerInterface() {}
};

class MasterSlaveThreadController {
private:
	static const int triggerBaseCount = 1;

	Mutex *mutex;
	Semaphore *slaveTriggerSem;
	int slaveTriggerCounter;

	std::vector<SlaveThreadControllerInterface *> slaveThreadList;

	void init(std::vector<SlaveThreadControllerInterface *> &newSlaveThreadList);
public:

	MasterSlaveThreadController();
	MasterSlaveThreadController(std::vector<SlaveThreadControllerInterface *> &slaveThreadList);
	~MasterSlaveThreadController();

	void setSlaves(std::vector<SlaveThreadControllerInterface *> &slaveThreadList);
	void clearSlaves(bool clearListOnly=false);

	void signalSlaves(void *userdata);
	void triggerMaster(int waitMilliseconds=-1);
	bool waitTillSlavesTrigger(int waitMilliseconds=-1);

};

class MasterSlaveThreadControllerSafeWrapper {
protected:
	MasterSlaveThreadController *master;
	string ownerId;
	int waitMilliseconds;

public:

	MasterSlaveThreadControllerSafeWrapper(MasterSlaveThreadController *master, int waitMilliseconds=-1, string ownerId="") {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		this->master = master;
		this->waitMilliseconds = waitMilliseconds;
		this->ownerId = ownerId;

		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	~MasterSlaveThreadControllerSafeWrapper() {
		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(master != NULL) {
			if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			master->triggerMaster(this->waitMilliseconds);
		}

		if(debugMasterSlaveThreadController) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
};

}}//end namespace

#endif
