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
#include <algorithm>
#include "conversion.h"
#include "platform_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

const static int MAX_FileCRCPreCacheThread_WORKER_THREADS 	= 3;
const static double PAUSE_SECONDS_BETWEEN_WORKERS 			= 15;

FileCRCPreCacheThread::FileCRCPreCacheThread() : BaseThread() {
	techDataPaths.clear();
	workerThreadTechPaths.clear();
	processTechCB = NULL;
}

FileCRCPreCacheThread::FileCRCPreCacheThread(vector<string> techDataPaths,
											vector<string> workerThreadTechPaths,
											FileCRCPreCacheThreadCallbackInterface *processTechCB) {
	this->techDataPaths					= techDataPaths;
	this->workerThreadTechPaths 		= workerThreadTechPaths;
	this->processTechCB 				= processTechCB;
}

bool FileCRCPreCacheThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    signalQuit();
	}

	return ret;
}

void FileCRCPreCacheThread::execute() {
    {
        RunningStatusSafeWrapper runningStatus(this);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            deleteSelfIfRequired();
            return;
        }

        bool threadControllerMode = (workerThreadTechPaths.size() == 0);

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FILE CRC PreCache thread is running threadControllerMode = %d\n",threadControllerMode);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"FILE CRC PreCache thread is running threadControllerMode = %d\n",threadControllerMode);

        try	{
        	std::vector<FileCRCPreCacheThread *> preCacheWorkerThreadList;
        	if(threadControllerMode == true) {
        		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("********************** CRC Controller thread START **********************\n");
        		time_t elapsedTime = time(NULL);

        		Checksum::clearFileCache();

				vector<string> techPaths;
				findDirs(techDataPaths, techPaths);
				if(techPaths.empty() == false) {
					// Always calc megapack first so its up to date sooner
					const string megapackTechtreeName = "megapack";
					vector<string>::iterator iterFindMegaPack = std::find(techPaths.begin(),techPaths.end(),megapackTechtreeName);
					if(iterFindMegaPack != techPaths.end()) {
						techPaths.erase(iterFindMegaPack);
						techPaths.insert(techPaths.begin(),megapackTechtreeName);

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Found megapack techtree and placing it at the TOP of the list\n",__FILE__,__FUNCTION__,__LINE__);
					}
					unsigned int techsPerWorker = (techPaths.size() / MAX_FileCRCPreCacheThread_WORKER_THREADS);
					if(techPaths.size() % MAX_FileCRCPreCacheThread_WORKER_THREADS != 0) {
						techsPerWorker++;
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] techsPerWorker = %d, MAX_FileCRCPreCacheThread_WORKER_THREADS = %d, techPaths.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,techsPerWorker,MAX_FileCRCPreCacheThread_WORKER_THREADS,(int)techPaths.size());

					unsigned int consumedWorkers = 0;
					for(unsigned int workerIdx = 0; workerIdx < MAX_FileCRCPreCacheThread_WORKER_THREADS; ++workerIdx) {
						if(getQuitStatus() == true) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							break;
						}

						unsigned int currentWorkerMax = (techPaths.size() - consumedWorkers);
						if(currentWorkerMax > techsPerWorker) {
							currentWorkerMax = techsPerWorker;
						}

						vector<string> workerTechList;
						unsigned int endConsumerIndex = consumedWorkers + currentWorkerMax;
						for(unsigned int idx = consumedWorkers; idx < endConsumerIndex; idx++) {
							string techName = techPaths[idx];
							workerTechList.push_back(techName);

							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Spawning CRC thread for Tech [%s] [%d of %d]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),idx+1,(int)techPaths.size());
						}

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] workerIdx = %d, currentWorkerMax = %d, endConsumerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,workerIdx,currentWorkerMax,endConsumerIndex);

						// Pause before launching this worker thread
						time_t pauseTime = time(NULL);
						while(getQuitStatus() == false &&
								difftime(time(NULL),pauseTime) <= PAUSE_SECONDS_BETWEEN_WORKERS) {
							sleep(25);
						}
						if(getQuitStatus() == true) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							break;
						}


						FileCRCPreCacheThread *workerThread =
								new FileCRCPreCacheThread(techDataPaths,
										workerTechList,
										this->processTechCB);
						workerThread->setUniqueID(__FILE__);
						preCacheWorkerThreadList.push_back(workerThread);
						workerThread->start();

						consumedWorkers += currentWorkerMax;

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Spawning CRC thread, preCacheWorkerThreadList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,(int)preCacheWorkerThreadList.size());

						if(consumedWorkers >= techPaths.size()) {
							break;
						}
					}

					sleep(0);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Waiting for Spawned CRC threads to complete, preCacheWorkerThreadList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,(int)preCacheWorkerThreadList.size());
					bool hasRunningWorkerThread = true;
					for(;hasRunningWorkerThread == true;) {
						hasRunningWorkerThread = false;
						for(unsigned int idx = 0; idx < preCacheWorkerThreadList.size(); idx++) {
							FileCRCPreCacheThread *workerThread = preCacheWorkerThreadList[idx];

							if(workerThread != NULL) {
								//vector<Texture2D *> textureList = workerThread->getPendingTextureList(-1);
								//addPendingTextureList(textureList);

								if(workerThread->getRunningStatus() == true) {
									hasRunningWorkerThread = true;
									if(getQuitStatus() == true &&
										workerThread->getQuitStatus() == false) {
										workerThread->signalQuit();
									}
								}
								else if(workerThread->getRunningStatus() == false) {
									sleep(10);
									delete workerThread;
									preCacheWorkerThreadList[idx] = NULL;
								}
							}
						}

						if(	getQuitStatus() == false &&
							hasRunningWorkerThread == true) {
							sleep(10);
						}
						else if(getQuitStatus() == true) {
							for(unsigned int idx = 0; idx < preCacheWorkerThreadList.size(); idx++) {
								FileCRCPreCacheThread *workerThread = preCacheWorkerThreadList[idx];
								if(workerThread != NULL && workerThread->getQuitStatus() == false) {
									workerThread->signalQuit();
								}
							}
						}
					}
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("********************** CRC Controller thread took %.2f seconds END **********************\n",difftime(time(NULL),elapsedTime));
                }
            }
        	else {
        		for(unsigned int idx = 0; idx < workerThreadTechPaths.size(); idx++) {
					string techName = this->workerThreadTechPaths[idx];
					if(getQuitStatus() == true) {
						break;
					}
					//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread START for tech [%s] ---------------------------\n",techName.c_str());
					//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] caching CRC value for Tech [%s]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caching CRC value for Tech [%s]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str());

					time_t elapsedTime = time(NULL);
					// Clear existing CRC to force a CRC refresh
					string pathSearchString     = string("/") + techName + string("/*");
					const string filterFileExt  = ".xml";
					//clearFolderTreeContentsCheckSum(techDataPaths, pathSearchString, filterFileExt);
					//clearFolderTreeContentsCheckSumList(techDataPaths, pathSearchString, filterFileExt);

					if(getQuitStatus() == true) {
						break;
					}

					//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread running for tech [%s] ---------------------------\n",techName.c_str());
					if(getQuitStatus() == false) {
						int32 techCRC = getFolderTreeContentsCheckSumRecursively(techDataPaths, string("/") + techName + string("/*"), ".xml", NULL, true);

						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));

						vector<string> results;
					    for(unsigned int idx = 0; idx < techDataPaths.size(); idx++) {
					        string &techPath = techDataPaths[idx];
					        endPathWithSlash(techPath);
					        findAll(techPath + techName + "/factions/*.", results, false, false);
					        if(results.empty() == false) {
					            break;
					        }
					    }

					    if(results.empty() == true) {
							for(unsigned int factionIdx = 0; factionIdx < results.size(); ++factionIdx) {
								string factionName = results[factionIdx];
								int32 factionCRC   = 0;
								factionCRC   = getFolderTreeContentsCheckSumRecursively(techDataPaths, "/" + techName + "/factions/" + factionName + "/*", ".xml", NULL, true);
							}
					    }

						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));
					}

//					if(processTechCB) {
//						vector<Texture2D *> files = processTechCB->processTech(techName);
//						for(unsigned int logoIdx = 0; logoIdx < files.size(); ++logoIdx) {
//							addPendingTexture(files[logoIdx]);
//
//							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread added texture [%s] for tech [%s] ---------------------------\n",files[logoIdx]->getPath().c_str(),techName.c_str());
//						}
//					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread END for tech [%s] ---------------------------\n",techName.c_str());

					if(getQuitStatus() == true) {
						break;
					}
        		}
        	}
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
        }

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] FILE CRC PreCache thread is exiting, getDeleteSelfOnExecutionDone() = %d\n",__FILE__,__FUNCTION__,__LINE__,getDeleteSelfOnExecutionDone());
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] FILE CRC PreCache thread is exiting, getDeleteSelfOnExecutionDone() = %d\n",__FILE__,__FUNCTION__,__LINE__,getDeleteSelfOnExecutionDone());
    }
	deleteSelfIfRequired();
}

void FileCRCPreCacheThread::addPendingTextureList(vector<Texture2D *> textureList) {
	for(unsigned int textureIdx = 0; textureIdx < textureList.size(); ++textureIdx) {
		this->addPendingTexture(textureList[textureIdx]);
	}
}

void FileCRCPreCacheThread::addPendingTexture(Texture2D *texture) {
	if(texture == NULL) {
		return;
	}
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexPendingTextureList,mutexOwnerId);
	mutexPendingTextureList.setOwnerId(mutexOwnerId);
	pendingTextureList.push_back(texture);
	safeMutex.ReleaseLock();
}

vector<Texture2D *> FileCRCPreCacheThread::getPendingTextureList(int maxTexturesToGet) {
	vector<Texture2D *> result;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexPendingTextureList,mutexOwnerId);
	mutexPendingTextureList.setOwnerId(mutexOwnerId);
	unsigned int listCount = pendingTextureList.size();
	if(listCount > 0) {
		if(maxTexturesToGet >= 0) {
			listCount = maxTexturesToGet;
		}
		for(unsigned int i = 0; i < listCount; ++i) {
			result.push_back(pendingTextureList[i]);
		}
		pendingTextureList.erase(pendingTextureList.begin() + 0, pendingTextureList.begin() + listCount);
	}
	safeMutex.ReleaseLock();

	return result;
}

SimpleTaskThread::SimpleTaskThread(	SimpleTaskCallbackInterface *simpleTaskInterface,
									unsigned int executionCount,
									unsigned int millisecsBetweenExecutions,
									bool needTaskSignal) : BaseThread() {
	this->simpleTaskInterface		 = simpleTaskInterface;
	this->executionCount			 = executionCount;
	this->millisecsBetweenExecutions = millisecsBetweenExecutions;
	this->needTaskSignal			 = needTaskSignal;
	this->overrideShutdownTask		 = NULL;

	setTaskSignalled(false);

	//static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexLastExecuteTimestamp,mutexOwnerId);
	mutexLastExecuteTimestamp.setOwnerId(mutexOwnerId);
	lastExecuteTimestamp = time(NULL);

	this->simpleTaskInterface->setupTask(this);
}

SimpleTaskThread::~SimpleTaskThread() {
	try {
		cleanup();
	}
    catch(const exception &ex) {
        SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());

        //throw megaglest_runtime_error(ex.what());
        abort();
    }
}

void SimpleTaskThread::cleanup() {
	if(this->overrideShutdownTask != NULL) {
		this->overrideShutdownTask(this);
		this->overrideShutdownTask = NULL;
	}
	else if(this->simpleTaskInterface != NULL) {
		this->simpleTaskInterface->shutdownTask(this);
		this->simpleTaskInterface = NULL;
	}
}

void SimpleTaskThread::setOverrideShutdownTask(taskFunctionCallback *ptr) {
	this->overrideShutdownTask = ptr;
}

bool SimpleTaskThread::isThreadExecutionLagging() {
	bool result = false;
	//static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexLastExecuteTimestamp,mutexOwnerId);
	mutexLastExecuteTimestamp.setOwnerId(mutexOwnerId);
	result = (difftime(time(NULL),lastExecuteTimestamp) >= 5.0);
	safeMutex.ReleaseLock();

	return result;
}

bool SimpleTaskThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    signalQuit();
	}

	return ret;
}

void SimpleTaskThread::execute() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	void *ptr_cpy = this->ptr;
	bool mustDeleteSelf = false;
	{
    {
        RunningStatusSafeWrapper runningStatus(this);
        try {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
            if(getQuitStatus() == true) {
                return;
            }

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());

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
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
                    break;
                }
                else if(runTask == true) {
                    if(getQuitStatus() == false) {
                    	ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);
                    	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                        this->simpleTaskInterface->simpleTask(this);
                        //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

                        //static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
                        string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
                        MutexSafeWrapper safeMutex(&mutexLastExecuteTimestamp,mutexOwnerId);
                        mutexLastExecuteTimestamp.setOwnerId(mutexOwnerId);
                    	lastExecuteTimestamp = time(NULL);
                    	safeMutex.ReleaseLock();
                    }
                }

                if(this->executionCount > 0) {
                    idx++;
                    if(idx >= this->executionCount) {
                        break;
                    }
                }
                if(getQuitStatus() == true) {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
                    break;
                }

                sleep(this->millisecsBetweenExecutions);
            }
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s] END\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
        	mustDeleteSelf = getDeleteSelfOnExecutionDone();
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s] END\n",__FILE__,__FUNCTION__,__LINE__,this->getUniqueID().c_str());
        	mustDeleteSelf = getDeleteSelfOnExecutionDone();

            throw megaglest_runtime_error(ex.what());
        }
    }
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(mustDeleteSelf == true) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(isThreadDeleted(ptr_cpy) == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			delete this;
		}
	}
}

void SimpleTaskThread::setTaskSignalled(bool value) {
	//static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexTaskSignaller,mutexOwnerId);
	mutexTaskSignaller.setOwnerId(mutexOwnerId);
	taskSignalled = value;
	safeMutex.ReleaseLock();
}

bool SimpleTaskThread::getTaskSignalled() {
	bool retval = false;
	//static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&mutexTaskSignaller,mutexOwnerId);
	mutexTaskSignaller.setOwnerId(mutexOwnerId);
	retval = taskSignalled;
	safeMutex.ReleaseLock();

	return retval;
}

// -------------------------------------------------

LogFileThread::LogFileThread() : BaseThread() {
    logList.clear();
    lastSaveToDisk = time(NULL);
    static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    mutexLogList.setOwnerId(mutexOwnerId);
}

LogFileThread::~LogFileThread() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#1 In [%s::%s Line: %d] LogFile thread is deleting\n",__FILE__,__FUNCTION__,__LINE__);
}

void LogFileThread::addLogEntry(SystemFlags::DebugType type, string logEntry) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&mutexLogList,mutexOwnerId);
    mutexLogList.setOwnerId(mutexOwnerId);
	LogFileEntry entry;
	entry.type = type;
	entry.entry = logEntry;
	entry.entryDateTime = time(NULL);
	logList.push_back(entry);

    //if(logList.size() >= 750000) {
    //    saveToDisk(false,true);
    //}
}

bool LogFileThread::checkSaveCurrentLogBufferToDisk() {
    bool ret = false;
    if(difftime(time(NULL),lastSaveToDisk) >= 5 ||
       LogFileThread::getLogEntryBufferCount() >= 1000000) {
        lastSaveToDisk = time(NULL);
        ret = true;
    }
    return ret;
}

void LogFileThread::execute() {
	void *ptr_cpy = this->ptr;
    bool mustDeleteSelf = false;
    {
        RunningStatusSafeWrapper runningStatus(this);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            deleteSelfIfRequired();
            return;
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"LogFile thread is running\n");

        try	{
        	ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);
            for(;this->getQuitStatus() == false;) {
                while(this->getQuitStatus() == false &&
                	  checkSaveCurrentLogBufferToDisk() == true) {
                    saveToDisk(false,false);
                }
                if(this->getQuitStatus() == false) {
                    sleep(25);
                }
            }

            // Ensure remaining entryies are logged to disk on shutdown
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
            saveToDisk(true,false);
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        }
        catch(const exception &ex) {
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is starting to exit\n",__FILE__,__FUNCTION__,__LINE__);

        mustDeleteSelf = getDeleteSelfOnExecutionDone();
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is exiting, mustDeleteSelf = %d\n",__FILE__,__FUNCTION__,__LINE__,mustDeleteSelf);
    }
    if(mustDeleteSelf == true) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is deleting self\n",__FILE__,__FUNCTION__,__LINE__);
    	if(isThreadDeleted(ptr_cpy) == false) {
    		delete this;
    	}
        return;
    }
}

std::size_t LogFileThread::getLogEntryBufferCount() {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&mutexLogList,mutexOwnerId);
    mutexLogList.setOwnerId(mutexOwnerId);
    std::size_t logCount = logList.size();
    safeMutex.ReleaseLock();
    return logCount;
}

bool LogFileThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    signalQuit();
	}

	return ret;
}

void LogFileThread::saveToDisk(bool forceSaveAll,bool logListAlreadyLocked) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(NULL,mutexOwnerId);
    if(logListAlreadyLocked == false) {
        safeMutex.setMutex(&mutexLogList);
        mutexLogList.setOwnerId(mutexOwnerId);
    }

    std::size_t logCount = logList.size();
    if(logCount > 0) {
        vector<LogFileEntry> tempLogList = logList;
        safeMutex.ReleaseLock(true);

        logCount = tempLogList.size();
        //if(forceSaveAll == false) {
        //    logCount = min(logCount,(std::size_t)2000000);
        //}

        if(logCount > 0) {
            for(unsigned int i = 0; i < logCount; ++i) {
                LogFileEntry &entry = tempLogList[i];
                SystemFlags::logDebugEntry(entry.type, entry.entry, entry.entryDateTime);
            }

            safeMutex.Lock();
            if(logList.size() > 0) {
				if(logList.size() <= logCount) {
					char szBuf[1024]="";
					sprintf(szBuf,"logList.size() <= logCount [%lld][%lld]",(long long int)logList.size(),(long long int)logCount);
					throw megaglest_runtime_error(szBuf);
				}
				logList.erase(logList.begin(),logList.begin() + logCount);
            }
            safeMutex.ReleaseLock();
        }
    }
    else {
        safeMutex.ReleaseLock();
    }
}

}}//end namespace
