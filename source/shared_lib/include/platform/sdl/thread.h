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
#ifdef DEBUG_PERFORMANCE_MUTEXES
#include "platform_common.h"
#endif

//#include "util.h"
#include "leak_dumper.h"

// =====================================================
//	class Thread
// =====================================================

using namespace std;
#ifdef DEBUG_PERFORMANCE_MUTEXES
using namespace Shared::PlatformCommon;
#endif

namespace Shared { namespace Platform {

class Thread{
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

public:
	Thread();
	virtual ~Thread();

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

public:
	Mutex(string ownerId="");
	~Mutex();
	void setOwnerId(string ownerId) { this->ownerId = ownerId; }
	void p();
	void v();
	int getRefCount() const { return refCount; }
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
	int waitTillSignalled();
};

}}//end namespace

#endif
