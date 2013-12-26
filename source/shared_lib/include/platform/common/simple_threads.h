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
#ifndef _SHARED_PLATFORMCOMMON_SIMPLETHREAD_H_
#define _SHARED_PLATFORMCOMMON_SIMPLETHREAD_H_

#include "base_thread.h"
#include <vector>
#include <string>
#include "util.h"
#include "texture.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Shared { namespace PlatformCommon {

//
// This interface describes the methods a callback object must implement
//
class FileCRCPreCacheThreadCallbackInterface {
public:
	virtual vector<Texture2D *> processTech(string techName) = 0;
	virtual ~FileCRCPreCacheThreadCallbackInterface() {}
};

// =====================================================
//	class FileCRCPreCacheThread
// =====================================================

class FileCRCPreCacheThread : public BaseThread
{
protected:
	vector<string> techDataPaths;
	vector<string> workerThreadTechPaths;
	FileCRCPreCacheThreadCallbackInterface *processTechCB;

	static string preCacheThreadCacheLookupKey;
	Mutex *mutexPauseForGame;
	bool pauseForGame;
	std::vector<FileCRCPreCacheThread *> preCacheWorkerThreadList;

public:
	FileCRCPreCacheThread();
	FileCRCPreCacheThread(vector<string> techDataPaths,vector<string> workerThreadTechPaths,FileCRCPreCacheThreadCallbackInterface *processTechCB);
	virtual ~FileCRCPreCacheThread();

	static void setPreCacheThreadCacheLookupKey(string value) { preCacheThreadCacheLookupKey = value; }

    virtual void execute();
    void setTechDataPaths(vector<string> value) { this->techDataPaths = value; }
    void setWorkerThreadTechPaths(vector<string> value) { this->workerThreadTechPaths = value; }
    void setFileCRCPreCacheThreadCallbackInterface(FileCRCPreCacheThreadCallbackInterface *value) { processTechCB = value; }

	virtual bool canShutdown(bool deleteSelfIfShutdownDelayed);

	void setPauseForGame(bool pauseForGame);
	bool getPauseForGame();
};

// =====================================================
//	class SimpleTaskThread
// =====================================================
typedef void taskFunctionCallback(BaseThread *callingThread);
//
// This interface describes the methods a callback object must implement
//
class SimpleTaskCallbackInterface {
public:
	virtual void simpleTask(BaseThread *callingThread,void *userdata) = 0;

	virtual void setupTask(BaseThread *callingThread,void *userdata) { }
	virtual void shutdownTask(BaseThread *callingThread,void *userdata) { }

	virtual ~SimpleTaskCallbackInterface() {}
};

class SimpleTaskThread : public BaseThread
{
protected:

	Mutex *mutexSimpleTaskInterfaceValid;
	bool simpleTaskInterfaceValid;
	SimpleTaskCallbackInterface *simpleTaskInterface;
	unsigned int executionCount;
	unsigned int millisecsBetweenExecutions;

	Mutex *mutexTaskSignaller;
	bool taskSignalled;
	bool needTaskSignal;

	Mutex *mutexLastExecuteTimestamp;
	time_t lastExecuteTimestamp;

	taskFunctionCallback *overrideShutdownTask;
	void *userdata;
	bool wantSetupAndShutdown;

public:
	SimpleTaskThread(SimpleTaskCallbackInterface *simpleTaskInterface,
					 unsigned int executionCount=0,
					 unsigned int millisecsBetweenExecutions=0,
					 bool needTaskSignal = false,
					 void *userdata=NULL,
					 bool wantSetupAndShutdown=true);
	virtual ~SimpleTaskThread();

	virtual void * getUserdata() { return userdata; }
	virtual int getUserdataAsInt() {
		int value = 0;
		if(userdata) {
			value = *((int*)&userdata);
		}
		return value;
	}
    virtual void execute();
    virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);

    void setTaskSignalled(bool value);
    bool getTaskSignalled();

    bool isThreadExecutionLagging();

    void cleanup();

    void setOverrideShutdownTask(taskFunctionCallback *ptr);

    bool getSimpleTaskInterfaceValid();
    void setSimpleTaskInterfaceValid(bool value);
};

// =====================================================
//	class LogFileThread
// =====================================================

class LogFileEntry {
public:
    SystemFlags::DebugType type;
    string entry;
    time_t entryDateTime;
};

class LogFileThread : public BaseThread
{
protected:

    Mutex *mutexLogList;
	vector<LogFileEntry> logList;
	time_t lastSaveToDisk;

    void saveToDisk(bool forceSaveAll,bool logListAlreadyLocked);
    bool checkSaveCurrentLogBufferToDisk();

public:
	LogFileThread();
	virtual ~LogFileThread();
    virtual void execute();
    void addLogEntry(SystemFlags::DebugType type, string logEntry);
    std::size_t getLogEntryBufferCount();
    virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);
};

}}//end namespace

#endif
