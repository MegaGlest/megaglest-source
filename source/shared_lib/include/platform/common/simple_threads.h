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

	Mutex mutexPendingTextureList;
	vector<Texture2D *> pendingTextureList;

	void addPendingTexture(Texture2D *texture);
	void addPendingTextureList(vector<Texture2D *> textureList);

public:
	FileCRCPreCacheThread();
	FileCRCPreCacheThread(vector<string> techDataPaths,vector<string> workerThreadTechPaths,FileCRCPreCacheThreadCallbackInterface *processTechCB);
    virtual void execute();
    void setTechDataPaths(vector<string> value) { this->techDataPaths = value; }
    void setWorkerThreadTechPaths(vector<string> value) { this->workerThreadTechPaths = value; }
    void setFileCRCPreCacheThreadCallbackInterface(FileCRCPreCacheThreadCallbackInterface *value) { processTechCB = value; }
    vector<Texture2D *> getPendingTextureList(int maxTexturesToGet);
};

// =====================================================
//	class SimpleTaskThread
// =====================================================

//
// This interface describes the methods a callback object must implement
//
class SimpleTaskCallbackInterface {
public:
	virtual void simpleTask(BaseThread *callingThread) = 0;
};

class SimpleTaskThread : public BaseThread
{
protected:

	SimpleTaskCallbackInterface *simpleTaskInterface;
	unsigned int executionCount;
	unsigned int millisecsBetweenExecutions;

	Mutex mutexTaskSignaller;
	bool taskSignalled;
	bool needTaskSignal;

	Mutex mutexLastExecuteTimestamp;
	time_t lastExecuteTimestamp;

public:
	SimpleTaskThread();
	SimpleTaskThread(SimpleTaskCallbackInterface *simpleTaskInterface,
					 unsigned int executionCount=0,
					 unsigned int millisecsBetweenExecutions=0,
					 bool needTaskSignal = false);
    virtual void execute();
    virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);

    void setTaskSignalled(bool value);
    bool getTaskSignalled();

    bool isThreadExecutionLagging();
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

    Mutex mutexLogList;
	vector<LogFileEntry> logList;
	time_t lastSaveToDisk;

    void saveToDisk(bool forceSaveAll,bool logListAlreadyLocked);
    bool checkSaveCurrentLogBufferToDisk();

public:
	LogFileThread();
    virtual void execute();
    void addLogEntry(SystemFlags::DebugType type, string logEntry);
    std::size_t getLogEntryBufferCount();
};



}}//end namespace

#endif
