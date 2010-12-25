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

#include "simple_threads.h"
#include "util.h"
#include "platform_common.h"

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

FileCRCPreCacheThread::FileCRCPreCacheThread() : BaseThread() {
}

void FileCRCPreCacheThread::execute() {
    RunningStatusSafeWrapper runningStatus(this);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(getQuitStatus() == true) {
		return;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"FILE CRC PreCache thread is running\n");

	try	{
	    //tech Tree listBox
		vector<string> techPaths;
	    findDirs(techDataPaths, techPaths);
		if(techPaths.empty() == false) {
			for(unsigned int idx = 0; idx < techPaths.size(); idx++) {
				string techName = techPaths[idx];

				time_t elapsedTime = time(NULL);
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] caching CRC value for Tech [%s] [%d of %d]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),idx+1,(int)techPaths.size());
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] caching CRC value for Tech [%s] [%d of %d]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),idx+1,techPaths.size());

				int32 techCRC = getFolderTreeContentsCheckSumRecursively(techDataPaths, string("/") + techName + string("/*"), ".xml", NULL);

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] [%d of %d] took %.3f seconds.\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,idx+1,(int)techPaths.size(),difftime(time(NULL),elapsedTime));
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] [%d of %d] took %.3f seconds.\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,idx+1,techPaths.size(),difftime(time(NULL),elapsedTime));

				if(getQuitStatus() == true) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
				sleep( 100 );
			}
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
	}
	catch(...) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] FILE CRC PreCache thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
}

SimpleTaskThread::SimpleTaskThread(	SimpleTaskCallbackInterface *simpleTaskInterface,
									unsigned int executionCount,
									unsigned int millisecsBetweenExecutions,
									bool needTaskSignal) : BaseThread() {
	this->simpleTaskInterface		 = simpleTaskInterface;
	this->executionCount			 = executionCount;
	this->millisecsBetweenExecutions = millisecsBetweenExecutions;
	this->needTaskSignal			 = needTaskSignal;
	setTaskSignalled(false);
	setExecutingTask(false);
}

bool SimpleTaskThread::canShutdown() {
	return (getExecutingTask() == false);
}

void SimpleTaskThread::execute() {
    RunningStatusSafeWrapper runningStatus(this);
	try {
		if(getQuitStatus() == true) {
			return;
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());

		unsigned int idx = 0;
		for(;this->simpleTaskInterface != NULL;) {
			bool runTask = true;
			if(needTaskSignal == true) {
				runTask = getTaskSignalled();
				if(runTask == true) {
					setTaskSignalled(false);
				}
			}

			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
				break;
			}
			else if(runTask == true) {
				//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
				if(getQuitStatus() == false) {
					try {
						setExecutingTask(true);
						this->simpleTaskInterface->simpleTask();
						setExecutingTask(false);
					}
					catch(const exception &ex) {
						setExecutingTask(false);
						throw runtime_error(ex.what());
					}
				}
				//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
			}

			if(this->executionCount > 0) {
				idx++;
				if(idx >= this->executionCount) {
					break;
				}
			}
			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
				break;
			}

			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] millisecsBetweenExecutions = %d\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str(),millisecsBetweenExecutions);
			sleep(this->millisecsBetweenExecutions);
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] millisecsBetweenExecutions = %d\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str(),millisecsBetweenExecutions);
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());

		throw runtime_error(ex.what());
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] uniqueID [%s] END\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
}

void SimpleTaskThread::setTaskSignalled(bool value) {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(&mutexTaskSignaller);
	taskSignalled = value;
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool SimpleTaskThread::getTaskSignalled() {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	MutexSafeWrapper safeMutex(&mutexTaskSignaller);
	retval = taskSignalled;
	safeMutex.ReleaseLock();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return retval;
}

void SimpleTaskThread::setExecutingTask(bool value) {
	MutexSafeWrapper safeMutex(&mutexExecutingTask);
	executingTask = value;
	safeMutex.ReleaseLock();
}

bool SimpleTaskThread::getExecutingTask() {
	bool retval = false;
	MutexSafeWrapper safeMutex(&mutexExecutingTask);
	retval = executingTask;
	safeMutex.ReleaseLock();

	return retval;
}

// -------------------------------------------------

LogFileThread::LogFileThread() : BaseThread() {
    logList.clear();
    lastSaveToDisk = time(NULL);
}

void LogFileThread::addLogEntry(SystemFlags::DebugType type, string logEntry) {
    MutexSafeWrapper safeMutex(&mutexLogList);
	LogFileEntry entry;
	entry.type = type;
	entry.entry = logEntry;
	entry.entryDateTime = time(NULL);
	logList.push_back(entry);
}

bool LogFileThread::checkSaveCurrentLogBufferToDisk() {
    bool ret = false;
    if(difftime(time(NULL),lastSaveToDisk) >= 5 ||
       LogFileThread::getLogEntryBufferCount() >= 35000) {
        lastSaveToDisk = time(NULL);
        ret = true;
    }
    return ret;
}

void LogFileThread::execute() {
    RunningStatusSafeWrapper runningStatus(this);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(getQuitStatus() == true) {
		return;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"LogFile thread is running\n");

	try	{
	    for(;this->getQuitStatus() == false;) {
            if(checkSaveCurrentLogBufferToDisk() == true) {
                saveToDisk();
            }
            if(this->getQuitStatus() == false) {
                sleep(100);
            }
	    }

	    // Ensure remaining entryies are logged to disk on shutdown
	    saveToDisk();
	}
	catch(const exception &ex) {
		//SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
	}
	catch(...) {
		//SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
		//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
	}

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] LogFile thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
}

std::size_t LogFileThread::getLogEntryBufferCount() {
    MutexSafeWrapper safeMutex(&mutexLogList);
    std::size_t logCount = logList.size();
    safeMutex.ReleaseLock();
    return logCount;
}

void LogFileThread::saveToDisk() {
    MutexSafeWrapper safeMutex(&mutexLogList);
    std::size_t logCount = logList.size();
    if(logCount > 0) {
        vector<LogFileEntry> tempLogList = logList;
        safeMutex.ReleaseLock(true);

        logCount = tempLogList.size();
        for(int i = 0; i < logCount; ++i) {
            LogFileEntry &entry = tempLogList[i];
            SystemFlags::logDebugEntry(entry.type, entry.entry, entry.entryDateTime);
        }

        safeMutex.Lock();
        logList.erase(logList.begin(),logList.begin() + logCount);
        safeMutex.ReleaseLock();
    }
    else {
        safeMutex.ReleaseLock();
    }
}

}}//end namespace
