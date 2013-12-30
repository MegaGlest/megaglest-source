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
#include "cache_manager.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

const static int MAX_FileCRCPreCacheThread_WORKER_THREADS 	= 3;
const static double PAUSE_SECONDS_BETWEEN_WORKERS 			= 15;
string FileCRCPreCacheThread::preCacheThreadCacheLookupKey	= "";

FileCRCPreCacheThread::FileCRCPreCacheThread() : BaseThread(),
		mutexPauseForGame(new Mutex(CODE_AT_LINE)) {

	techDataPaths.clear();
	workerThreadTechPaths.clear();
	preCacheWorkerThreadList.clear();
	processTechCB = NULL;
	pauseForGame = false;
	uniqueID = "FileCRCPreCacheThread";
}

FileCRCPreCacheThread::FileCRCPreCacheThread(vector<string> techDataPaths,
											vector<string> workerThreadTechPaths,
											FileCRCPreCacheThreadCallbackInterface *processTechCB) :
			mutexPauseForGame(new Mutex(CODE_AT_LINE)) {

	this->techDataPaths					= techDataPaths;
	this->workerThreadTechPaths 		= workerThreadTechPaths;
	preCacheWorkerThreadList.clear();
	this->processTechCB 				= processTechCB;
	pauseForGame = false;
	uniqueID = "FileCRCPreCacheThread";
}

FileCRCPreCacheThread::~FileCRCPreCacheThread() {
	bool threadControllerMode = (workerThreadTechPaths.size() == 0);
	FileCRCPreCacheThread * &preCacheCRCThreadPtr = CacheManager::getCachedItem< FileCRCPreCacheThread * >(preCacheThreadCacheLookupKey);
	if(preCacheCRCThreadPtr != NULL && threadControllerMode == true) {
		preCacheCRCThreadPtr = NULL;
	}

	delete mutexPauseForGame;
	mutexPauseForGame = NULL;
}

void FileCRCPreCacheThread::setPauseForGame(bool pauseForGame) {
	static string mutexOwnerId = CODE_AT_LINE;
	MutexSafeWrapper safeMutex(mutexPauseForGame,mutexOwnerId);
	this->pauseForGame = pauseForGame;

	for(unsigned int index = 0; index < preCacheWorkerThreadList.size(); ++index) {
		FileCRCPreCacheThread *worker = preCacheWorkerThreadList[index];
		if(worker != NULL) {
			worker->setPauseForGame(this->pauseForGame);
		}
	}
}

bool FileCRCPreCacheThread::getPauseForGame() {
	static string mutexOwnerId = CODE_AT_LINE;
	MutexSafeWrapper safeMutex(mutexPauseForGame,mutexOwnerId);
	return this->pauseForGame;
}

bool FileCRCPreCacheThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    deleteSelfIfRequired();
	    signalQuit();
	}

	return ret;
}

void FileCRCPreCacheThread::execute() {
    {
        RunningStatusSafeWrapper runningStatus(this);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            deleteSelfIfRequired();
            return;
        }

        bool threadControllerMode = (workerThreadTechPaths.size() == 0);

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("FILE CRC PreCache thread is running threadControllerMode = %d\n",threadControllerMode);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"FILE CRC PreCache thread is running threadControllerMode = %d\n",threadControllerMode);

        try	{
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

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Found megapack techtree and placing it at the TOP of the list\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					unsigned int techsPerWorker = ((unsigned int)techPaths.size() / (unsigned int)MAX_FileCRCPreCacheThread_WORKER_THREADS);
					if(techPaths.size() % MAX_FileCRCPreCacheThread_WORKER_THREADS != 0) {
						techsPerWorker++;
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] techsPerWorker = %u, MAX_FileCRCPreCacheThread_WORKER_THREADS = %d, techPaths.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techsPerWorker,MAX_FileCRCPreCacheThread_WORKER_THREADS,(int)techPaths.size());

					try {
						unsigned int consumedWorkers = 0;
						for(unsigned int workerIdx = 0; workerIdx < (unsigned int)MAX_FileCRCPreCacheThread_WORKER_THREADS; ++workerIdx) {
							if(getQuitStatus() == true) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
								break;
							}

							unsigned int currentWorkerMax = ((unsigned int)techPaths.size() - consumedWorkers);
							if(currentWorkerMax > techsPerWorker) {
								currentWorkerMax = techsPerWorker;
							}

							vector<string> workerTechList;
							unsigned int endConsumerIndex = consumedWorkers + currentWorkerMax;
							for(unsigned int idx = consumedWorkers; idx < endConsumerIndex; idx++) {
								string techName = techPaths[idx];
								workerTechList.push_back(techName);

								if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Spawning CRC thread for Tech [%s] [%d of %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str(),idx+1,(int)techPaths.size());
							}

							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] workerIdx = %u, currentWorkerMax = %u, endConsumerIndex = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,workerIdx,currentWorkerMax,endConsumerIndex);

							// Pause before launching this worker thread
							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("About to process CRC for factions waiting...\n");

							time_t pauseTime = time(NULL);
							while(getQuitStatus() == false &&
									difftime(time(NULL),pauseTime) <= PAUSE_SECONDS_BETWEEN_WORKERS) {
								sleep(25);
							}
							if(getQuitStatus() == true) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
								break;
							}

							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Starting CRC for faction workers...\n");

							FileCRCPreCacheThread *workerThread =
									new FileCRCPreCacheThread(techDataPaths,
											workerTechList,
											this->processTechCB);
							static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
							workerThread->setUniqueID(mutexOwnerId);
							workerThread->setPauseForGame(this->getPauseForGame());
							static string mutexOwnerId2 = CODE_AT_LINE;
							MutexSafeWrapper safeMutexPause(mutexPauseForGame,mutexOwnerId2);
							preCacheWorkerThreadList.push_back(workerThread);
							safeMutexPause.ReleaseLock();

							workerThread->start();

							consumedWorkers += currentWorkerMax;

							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Spawning CRC thread, preCacheWorkerThreadList.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(int)preCacheWorkerThreadList.size());

							if(consumedWorkers >= techPaths.size()) {
								break;
							}
						}
					}
			        catch(const exception &ex) {
			            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			        }
			        catch(...) {
			            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unknown error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			        }

					sleep(0);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Waiting for Spawned CRC threads to complete, preCacheWorkerThreadList.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(int)preCacheWorkerThreadList.size());

					try {
						bool hasRunningWorkerThread = true;
						for(;hasRunningWorkerThread == true;) {
							hasRunningWorkerThread = false;
							for(unsigned int idx = 0; idx < preCacheWorkerThreadList.size(); idx++) {
								FileCRCPreCacheThread *workerThread = preCacheWorkerThreadList[idx];

								if(workerThread != NULL) {

									if(workerThread->getRunningStatus() == true) {
										hasRunningWorkerThread = true;
										if(getQuitStatus() == true &&
											workerThread->getQuitStatus() == false) {
											workerThread->signalQuit();
										}
									}
									else if(workerThread->getRunningStatus() == false) {
										sleep(25);

										static string mutexOwnerId2 = CODE_AT_LINE;
										MutexSafeWrapper safeMutexPause(mutexPauseForGame,mutexOwnerId2);

										delete workerThread;
										preCacheWorkerThreadList[idx] = NULL;

										safeMutexPause.ReleaseLock();
									}
								}
							}

							if(	getQuitStatus() == false &&
								hasRunningWorkerThread == true) {
								sleep(25);
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
					}
			        catch(const exception &ex) {
			            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			        }
			        catch(...) {
			            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unknown error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			        }

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("********************** CRC Controller thread took %.2f seconds END **********************\n",difftime(time(NULL),elapsedTime));
                }
            }
        	else {
        		try {
        			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\tStarting CRC for faction worker thread for techs: %d...\n",(int)workerThreadTechPaths.size());

					for(unsigned int idx = 0; idx < workerThreadTechPaths.size(); idx++) {
						string techName = this->workerThreadTechPaths[idx];
						if(getQuitStatus() == true) {
							break;
						}
						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread START for tech [%s] ---------------------------\n",techName.c_str());
						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] caching CRC value for Tech [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] caching CRC value for Tech [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str());

						time_t elapsedTime = time(NULL);
						// Clear existing CRC to force a CRC refresh
						//string pathSearchString     = string("/") + techName + string("/*");
						//const string filterFileExt  = ".xml";
						//clearFolderTreeContentsCheckSum(techDataPaths, pathSearchString, filterFileExt);
						//clearFolderTreeContentsCheckSumList(techDataPaths, pathSearchString, filterFileExt);

						if(getQuitStatus() == true) {
							break;
						}

						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread running for tech [%s] ---------------------------\n",techName.c_str());
						if(getQuitStatus() == false) {
							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\tAbout to process CRC for techName [%s]\n",techName.c_str());

							// Do not process CRC's while game in progress
							if(getPauseForGame() == true) {
								if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\tGame in progress so waiting to process CRC for techName [%s]\n",techName.c_str());
								for(;getPauseForGame() == true &&
								 	 getQuitStatus() == false;) {
									sleep(25);
								}
								if(getQuitStatus() == true) {
									break;
								}
							}
							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\tStart Processing CRC for techName [%s]\n",techName.c_str());

							uint32 techCRC = getFolderTreeContentsCheckSumRecursively(techDataPaths, string("/") + techName + string("/*"), ".xml", NULL, true);

							//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%u] took %.3f seconds.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));

							vector<string> results;
							for(unsigned int idx = 0; idx < techDataPaths.size(); idx++) {
								string &techPath = techDataPaths[idx];
								endPathWithSlash(techPath);
								findAll(techPath + techName + "/factions/*.", results, false, false);
								if(results.empty() == false) {
									break;
								}
							}

							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\tStarting CRC for faction worker thread for results: %d...\n",(int)results.size());
							if(results.empty() == true) {
								for(unsigned int factionIdx = 0;
										factionIdx < results.size();
										++factionIdx) {
									string factionName = results[factionIdx];
									if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\t\tAbout to process CRC for factionName [%s]\n",factionName.c_str());

									// Do not process CRC's while game in progress
									if(getPauseForGame() == true) {
										if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\t\tGame in progress so waiting to process CRC for factionName [%s]\n",factionName.c_str());
										for(;getPauseForGame() == true &&
										 	 getQuitStatus() == false;) {
											sleep(25);
										}
										if(getQuitStatus() == true) {
											break;
										}
									}
									if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\t\tStart Processing CRC for factionName [%s]\n",factionName.c_str());

									uint32 factionCRC   = getFolderTreeContentsCheckSumRecursively(techDataPaths, "/" + techName + "/factions/" + factionName + "/*", ".xml", NULL, true);

									if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\t\t\tDone Processing CRC for factionName [%s] CRC [%d]\n",factionName.c_str(),factionCRC);
								}
							}

							//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] took %.3f seconds.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,techName.c_str(),techCRC,difftime(time(NULL),elapsedTime));
						}

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--------------------- CRC worker thread END for tech [%s] ---------------------------\n",techName.c_str());

						if(getQuitStatus() == true) {
							break;
						}
					}
        		}
                catch(const exception &ex) {
                    SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
                }
                catch(...) {
                    SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unknown error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                }
        	}
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unknown error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        }

        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] FILE CRC PreCache thread is exiting...\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] FILE CRC PreCache thread is exiting...\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    }
	deleteSelfIfRequired();
}

SimpleTaskThread::SimpleTaskThread(	SimpleTaskCallbackInterface *simpleTaskInterface,
									unsigned int executionCount,
									unsigned int millisecsBetweenExecutions,
									bool needTaskSignal,
									void *userdata, bool wantSetupAndShutdown) : BaseThread(),
	simpleTaskInterface(NULL),
	overrideShutdownTask(NULL),
	mutexSimpleTaskInterfaceValid(new Mutex(CODE_AT_LINE)),
	mutexTaskSignaller(new Mutex(CODE_AT_LINE)),
	mutexLastExecuteTimestamp(new Mutex(CODE_AT_LINE)) {

	uniqueID = "SimpleTaskThread";
	this->simpleTaskInterface		 = simpleTaskInterface;
	this->simpleTaskInterfaceValid	 = (this->simpleTaskInterface != NULL);
	this->executionCount			 = executionCount;
	this->millisecsBetweenExecutions = millisecsBetweenExecutions;
	this->needTaskSignal			 = needTaskSignal;
	this->overrideShutdownTask		 = NULL;
	this->userdata					 = userdata;
	this->wantSetupAndShutdown		 = wantSetupAndShutdown;
	//if(this->userdata != NULL) {
	//	printf("IN SImpleThread this->userdata [%p]\n",this->userdata);
	//}

	setTaskSignalled(false);

	string mutexOwnerId = CODE_AT_LINE;
	MutexSafeWrapper safeMutex(mutexLastExecuteTimestamp,mutexOwnerId);
	mutexLastExecuteTimestamp->setOwnerId(mutexOwnerId);
	lastExecuteTimestamp = time(NULL);

	if(this->wantSetupAndShutdown == true) {
		string mutexOwnerId1 = CODE_AT_LINE;
		MutexSafeWrapper safeMutex1(mutexSimpleTaskInterfaceValid,mutexOwnerId1);
		if(this->simpleTaskInterfaceValid == true) {
			safeMutex1.ReleaseLock();
			this->simpleTaskInterface->setupTask(this,userdata);
		}
	}
}

SimpleTaskThread::~SimpleTaskThread() {
	//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
	try {
		cleanup();

		delete mutexSimpleTaskInterfaceValid;
		mutexSimpleTaskInterfaceValid = NULL;

		delete mutexTaskSignaller;
		mutexTaskSignaller = NULL;

		delete mutexLastExecuteTimestamp;
		mutexLastExecuteTimestamp = NULL;

	}
    catch(const exception &ex) {
        SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());

        throw megaglest_runtime_error(ex.what());
        //abort();
    }
    //printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
}

void SimpleTaskThread::cleanup() {
	//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
	if(this->wantSetupAndShutdown == true) {
		if(this->overrideShutdownTask != NULL) {
			//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
			this->overrideShutdownTask(this);
			this->overrideShutdownTask = NULL;
		}
		else if(this->simpleTaskInterface != NULL) {
			//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
			string mutexOwnerId1 = CODE_AT_LINE;
			MutexSafeWrapper safeMutex1(mutexSimpleTaskInterfaceValid,mutexOwnerId1);
			//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
			if(this->simpleTaskInterfaceValid == true) {
				//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
				safeMutex1.ReleaseLock();
				//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
				this->simpleTaskInterface->shutdownTask(this,userdata);
				this->simpleTaskInterface = NULL;
				//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
			}
		}
	}
	//printf("~SimpleTaskThread LINE: %d this = %p\n",__LINE__,this);
}

void SimpleTaskThread::setOverrideShutdownTask(taskFunctionCallback *ptr) {
	this->overrideShutdownTask = ptr;
}

bool SimpleTaskThread::isThreadExecutionLagging() {
	string mutexOwnerId = CODE_AT_LINE;
	MutexSafeWrapper safeMutex(mutexLastExecuteTimestamp,mutexOwnerId);
	mutexLastExecuteTimestamp->setOwnerId(mutexOwnerId);
	bool result = (difftime(time(NULL),lastExecuteTimestamp) >= 5.0);
	safeMutex.ReleaseLock();

	return result;
}

bool SimpleTaskThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    deleteSelfIfRequired();
	    signalQuit();
	}

	return ret;
}

bool SimpleTaskThread::getSimpleTaskInterfaceValid() {
	string mutexOwnerId1 = CODE_AT_LINE;
	MutexSafeWrapper safeMutex1(mutexSimpleTaskInterfaceValid,mutexOwnerId1);

	return this->simpleTaskInterfaceValid;
}
void SimpleTaskThread::setSimpleTaskInterfaceValid(bool value) {
	string mutexOwnerId1 = CODE_AT_LINE;
	MutexSafeWrapper safeMutex1(mutexSimpleTaskInterfaceValid,mutexOwnerId1);

	this->simpleTaskInterfaceValid = value;
}

void SimpleTaskThread::execute() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	void *ptr_cpy = this->ptr;
	bool mustDeleteSelf = false;
	{
    {
        RunningStatusSafeWrapper runningStatus(this);
        try {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            if(getQuitStatus() == true) {
                return;
            }

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());

            unsigned int idx = 0;
            for(;this->simpleTaskInterface != NULL;) {
        		string mutexOwnerId1 = CODE_AT_LINE;
        		MutexSafeWrapper safeMutex1(mutexSimpleTaskInterfaceValid,mutexOwnerId1);
        		if(this->simpleTaskInterfaceValid == false) {
        			break;
        		}
        		safeMutex1.ReleaseLock();

                bool runTask = true;
                if(needTaskSignal == true) {
                    runTask = getTaskSignalled();
                    if(runTask == true) {
                        setTaskSignalled(false);
                    }
                }

                if(getQuitStatus() == true) {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());
                    break;
                }
                else if(runTask == true) {
                    if(getQuitStatus() == false) {
                    	ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);
                    	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                        this->simpleTaskInterface->simpleTask(this,this->userdata);
                        //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

                        if(getQuitStatus() == true) {
                        	break;
                        }
                        string mutexOwnerId = CODE_AT_LINE;
                        MutexSafeWrapper safeMutex(mutexLastExecuteTimestamp,mutexOwnerId);
                        mutexLastExecuteTimestamp->setOwnerId(mutexOwnerId);
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
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());
                    break;
                }

                sleep(this->millisecsBetweenExecutions);
            }
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());
        	mustDeleteSelf = getDeleteSelfOnExecutionDone();
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());

            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] uniqueID [%s] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getUniqueID().c_str());
        	mustDeleteSelf = getDeleteSelfOnExecutionDone();

            throw megaglest_runtime_error(ex.what());
        }
    }
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(mustDeleteSelf == true) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		if(isThreadDeleted(ptr_cpy) == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			this->setDeleteAfterExecute(true);
		}
		return;
	}
}

void SimpleTaskThread::setTaskSignalled(bool value) {
	string mutexOwnerId = CODE_AT_LINE;
	MutexSafeWrapper safeMutex(mutexTaskSignaller,mutexOwnerId);
	mutexTaskSignaller->setOwnerId(mutexOwnerId);
	taskSignalled = value;
	safeMutex.ReleaseLock();
}

bool SimpleTaskThread::getTaskSignalled() {
	string mutexOwnerId = CODE_AT_LINE;
	MutexSafeWrapper safeMutex(mutexTaskSignaller,mutexOwnerId);
	mutexTaskSignaller->setOwnerId(mutexOwnerId);
	bool retval = taskSignalled;
	safeMutex.ReleaseLock();

	return retval;
}

// -------------------------------------------------

LogFileThread::LogFileThread() : BaseThread(), mutexLogList(new Mutex(CODE_AT_LINE)) {
	uniqueID = "LogFileThread";
    logList.clear();
    lastSaveToDisk = time(NULL);
    static string mutexOwnerId = CODE_AT_LINE;
    mutexLogList->setOwnerId(mutexOwnerId);
}

LogFileThread::~LogFileThread() {

	delete mutexLogList;
	mutexLogList = NULL;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#1 In [%s::%s Line: %d] LogFile thread is deleting\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void LogFileThread::addLogEntry(SystemFlags::DebugType type, string logEntry) {
	static string mutexOwnerId = CODE_AT_LINE;
    MutexSafeWrapper safeMutex(mutexLogList,mutexOwnerId);
    mutexLogList->setOwnerId(mutexOwnerId);
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
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            deleteSelfIfRequired();
            return;
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            saveToDisk(true,false);
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        }
        catch(const exception &ex) {
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] UNKNOWN Error\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is starting to exit\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

        mustDeleteSelf = getDeleteSelfOnExecutionDone();
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is exiting, mustDeleteSelf = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,mustDeleteSelf);
    }
    if(mustDeleteSelf == true) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] LogFile thread is deleting self\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    	if(isThreadDeleted(ptr_cpy) == false) {
    		this->setDeleteAfterExecute(true);
    	}
        return;
    }
}

std::size_t LogFileThread::getLogEntryBufferCount() {
	static string mutexOwnerId = CODE_AT_LINE;
    MutexSafeWrapper safeMutex(mutexLogList,mutexOwnerId);
    mutexLogList->setOwnerId(mutexOwnerId);
    std::size_t logCount = logList.size();
    safeMutex.ReleaseLock();
    return logCount;
}

bool LogFileThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    deleteSelfIfRequired();
	    signalQuit();
	}

	return ret;
}

void LogFileThread::saveToDisk(bool forceSaveAll,bool logListAlreadyLocked) {
	static string mutexOwnerId = CODE_AT_LINE;
    MutexSafeWrapper safeMutex(NULL,mutexOwnerId);
    if(logListAlreadyLocked == false) {
        safeMutex.setMutex(mutexLogList);
        mutexLogList->setOwnerId(mutexOwnerId);
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
				if(logList.size() < logCount) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"logList.size() <= logCount [%lld][%lld]",(long long int)logList.size(),(long long int)logCount);
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
