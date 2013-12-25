// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "server_interface.h"

#include <stdexcept>

#include "window.h"
#include "logger.h"

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "util.h"
#include "game_util.h"
#include "miniftpserver.h"
#include "map_preview.h"
#include "stats.h"
#include <time.h>
#include <set>
#include <iostream>
#include <iterator>

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Map;

namespace Glest { namespace Game {

double maxFrameCountLagAllowed 							= 25;
double maxClientLagTimeAllowed 							= 30;
double maxFrameCountLagAllowedEver 						= 35;
double maxClientLagTimeAllowedEver						= 45;
double warnFrameCountLagPercent 						= 0.65;
double LAG_CHECK_GRACE_PERIOD 							= 15;
double LAG_CHECK_INTERVAL_PERIOD 						= 6;
double MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE 				= 2;
const int MAX_SLOT_THREAD_WAIT_TIME 					= 3;
const int MASTERSERVER_HEARTBEAT_GAME_STATUS_SECONDS 	= 30;

ServerInterface::ServerInterface(bool publishEnabled) :GameNetworkInterface() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	allowInGameConnections 				= false;
	gameLaunched 						= false;

	serverSynchAccessor 				= new Mutex(CODE_AT_LINE);
	switchSetupRequestsSynchAccessor 	= new Mutex(CODE_AT_LINE);

	for(int index = 0; index < GameConstants::maxPlayers; ++index) {
		slotAccessorMutexes[index] 		= new Mutex(CODE_AT_LINE);
	}
	masterServerThreadAccessor 			= new Mutex(CODE_AT_LINE);
	textMessageQueueThreadAccessor 		= new Mutex(CODE_AT_LINE);
	broadcastMessageQueueThreadAccessor = new Mutex(CODE_AT_LINE);
	inBroadcastMessageThreadAccessor 	= new Mutex(CODE_AT_LINE);

	serverSocketAdmin				= NULL;
	nextEventId 					= 1;
	gameHasBeenInitiated 			= false;
	exitServer 						= false;
	gameSettingsUpdateCount 		= 0;
	currentFrameCount 				= 0;
	gameStartTime 					= 0;
	resumeGameStartTime				= 0;
	publishToMasterserverThread 	= NULL;
	lastMasterserverHeartbeatTime 	= 0;
	needToRepublishToMasterserver 	= false;
	ftpServer 						= NULL;
	inBroadcastMessage				= false;
	lastGlobalLagCheckTime			= 0;
	masterserverAdminRequestLaunch	= false;
	lastListenerSlotCheckTime		= 0;

	// This is an admin port listening only on the localhost intended to
	// give current connection status info
#ifndef __APPLE__
	try {
		serverSocketAdmin			= new ServerSocket(true);
		serverSocketAdmin->setBlock(false);
		serverSocketAdmin->setBindPort(Config::getInstance().getInt("ServerAdminPort", intToStr(GameConstants::serverAdminPort).c_str()));
		serverSocketAdmin->setBindSpecificAddress(Config::getInstance().getString("ServerAdminBindAddress", "127.0.0.1"));
		serverSocketAdmin->listen(5);
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] Warning Server admin port bind/listen error:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
	}
#endif

	maxFrameCountLagAllowed 				= Config::getInstance().getInt("MaxFrameCountLagAllowed", intToStr(maxFrameCountLagAllowed).c_str());
	maxFrameCountLagAllowedEver 			= Config::getInstance().getInt("MaxFrameCountLagAllowedEver", intToStr(maxFrameCountLagAllowedEver).c_str());
	maxClientLagTimeAllowedEver				= Config::getInstance().getInt("MaxClientLagTimeAllowedEver", intToStr(maxClientLagTimeAllowedEver).c_str());
	maxClientLagTimeAllowed 				= Config::getInstance().getInt("MaxClientLagTimeAllowed", intToStr(maxClientLagTimeAllowed).c_str());
	warnFrameCountLagPercent 				= Config::getInstance().getFloat("WarnFrameCountLagPercent", doubleToStr(warnFrameCountLagPercent).c_str());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] maxFrameCountLagAllowed = %f, maxFrameCountLagAllowedEver = %f, maxClientLagTimeAllowed = %f, maxClientLagTimeAllowedEver = %f\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,maxFrameCountLagAllowed,maxFrameCountLagAllowedEver,maxClientLagTimeAllowed,maxClientLagTimeAllowedEver);

	for(int index = 0; index < GameConstants::maxPlayers; ++index) {
		slots[index]				= NULL;
		switchSetupRequests[index]	= NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	serverSocket.setBlock(false);
	serverSocket.setBindPort(Config::getInstance().getInt("PortServer", intToStr(GameConstants::serverPort).c_str()));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	gameStatsThreadAccessor 	= new Mutex(CODE_AT_LINE);
	gameStats 					= NULL;

	Config &config 			= Config::getInstance();
  	string scenarioDir 		= "";
  	vector<string> pathList = config.getPathListForType(ptMaps,scenarioDir);
  	vector<string> invalidMapList;
  	vector<string> allMaps = MapPreview::findAllValidMaps(pathList,scenarioDir,false,true,&invalidMapList);
	if (allMaps.empty()) {
        //throw megaglest_runtime_error("No maps were found!");
		printf("No maps were found (srv)!\n");
	}
	std::sort(allMaps.begin(),allMaps.end());
	vector<string> results;
	copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
	mapFiles = results;

	results.clear();
    findDirs(config.getPathListForType(ptTilesets), results);
	if (results.empty()) {
        //throw megaglest_runtime_error("No tile-sets were found!");
		printf("No tile-sets were found (srv)!");
	}
    tilesetFiles = results;

    results.clear();
    findDirs(config.getPathListForType(ptTechs), results);
	if(results.empty()) {
        //throw megaglest_runtime_error("No tech-trees were found!");
		printf("No tech-trees were found (srv)!\n");
	}
    techTreeFiles = results;

	if(Config::getInstance().getBool("EnableFTPServer","true") == true) {
		std::pair<string,string> mapsPath;
		vector<string> pathList = Config::getInstance().getPathListForType(ptMaps);
		if(pathList.empty() == false) {
			mapsPath.first = pathList[0];
			if(pathList.size() > 1) {
				mapsPath.second = pathList[1];
			}
		}

		std::pair<string,string> tilesetsPath;
		vector<string> tilesetsList = Config::getInstance().getPathListForType(ptTilesets);
		if(tilesetsList.empty() == false) {
			tilesetsPath.first = tilesetsList[0];
			if(tilesetsList.size() > 1) {
				tilesetsPath.second = tilesetsList[1];
			}
		}

		std::pair<string,string> techtreesPath;
		vector<string> techtreesList = Config::getInstance().getPathListForType(ptTechs);
		if(techtreesList.empty() == false) {
			techtreesPath.first = techtreesList[0];
			if(techtreesList.size() > 1) {
				techtreesPath.second = techtreesList[1];
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		int portNumber   = Config::getInstance().getInt("FTPServerPort",intToStr(ServerSocket::getFTPServerPort()).c_str());
		ServerSocket::setFTPServerPort(portNumber);
		//printf("In [%s::%s] portNumber = %d ServerSocket::getFTPServerPort() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,portNumber,ServerSocket::getFTPServerPort());

		bool allowInternetTilesetFileTransfers = Config::getInstance().getBool("EnableFTPServerInternetTilesetXfer","true");
		bool allowInternetTechtreeFileTransfers = Config::getInstance().getBool("EnableFTPServerInternetTechtreeXfer","true");

		// Get path to temp files
		string tempFilePath = "temp/";
		if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
			tempFilePath = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + tempFilePath;
		}
		else {
	        string userData = config.getString("UserData_Root","");
	        if(userData != "") {
	        	endPathWithSlash(userData);
	        }
	        tempFilePath = userData + tempFilePath;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Temp files path [%s]\n",tempFilePath.c_str());

		ftpServer = new FTPServerThread(mapsPath,tilesetsPath,techtreesPath,
				publishEnabled,allowInternetTilesetFileTransfers,
				allowInternetTechtreeFileTransfers,portNumber,GameConstants::maxPlayers,
				this,tempFilePath);
		ftpServer->start();
	}

	if(publishToMasterserverThread == NULL) {
		if(needToRepublishToMasterserver == true || GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
			static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
			publishToMasterserverThread = new SimpleTaskThread(this,0,125);
			publishToMasterserverThread->setUniqueID(mutexOwnerId);
			publishToMasterserverThread->start();

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::setPublishEnabled(bool value) {
	if(ftpServer != NULL) {
		ftpServer->setInternetEnabled(value);
	}
}

void ServerInterface::shutdownMasterserverPublishThread() {
	MutexSafeWrapper safeMutex(masterServerThreadAccessor,CODE_AT_LINE);

	if(publishToMasterserverThread != NULL) {
		time_t elapsed = time(NULL);
		publishToMasterserverThread->signalQuit();
		for(;publishToMasterserverThread->canShutdown(false) == false &&
			difftime((long int)time(NULL),elapsed) <= 15;) {
			//sleep(150);
		}
		if(publishToMasterserverThread->canShutdown(true)) {
			delete publishToMasterserverThread;
			publishToMasterserverThread = NULL;
		}
	}
}

ServerInterface::~ServerInterface() {
	//printf("===> Destructor for ServerInterface\n");
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	masterController.clearSlaves(true);
	exitServer = true;
	for(int index = 0; index < GameConstants::maxPlayers; ++index) {
		if(slots[index] != NULL) {
			MutexSafeWrapper safeMutex(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
			delete slots[index];
			slots[index] = NULL;
		}

		if(switchSetupRequests[index] != NULL) {
			delete switchSetupRequests[index];
			switchSetupRequests[index] = NULL;
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	close();
	shutdownFTPServer();
	shutdownMasterserverPublishThread();

	lastMasterserverHeartbeatTime = 0;
	if(needToRepublishToMasterserver == true) {
		simpleTask(NULL,NULL);
	}

	for(int index = 0; index < GameConstants::maxPlayers; ++index) {
		delete slotAccessorMutexes[index];
		slotAccessorMutexes[index] = NULL;
	}

	delete textMessageQueueThreadAccessor;
	textMessageQueueThreadAccessor = NULL;

	delete broadcastMessageQueueThreadAccessor;
	broadcastMessageQueueThreadAccessor = NULL;

	delete inBroadcastMessageThreadAccessor;
	inBroadcastMessageThreadAccessor = NULL;

	delete serverSynchAccessor;
	serverSynchAccessor = NULL;

	delete masterServerThreadAccessor;
	masterServerThreadAccessor = NULL;

	delete serverSocketAdmin;
	serverSocketAdmin = NULL;

	for(int index = 0; index < (int)broadcastMessageQueue.size(); ++index) {
		pair<NetworkMessage *,int> &item = broadcastMessageQueue[index];
		if(item.first != NULL) {
			delete item.first;
		}
		item.first = NULL;
	}
	broadcastMessageQueue.clear();

	delete switchSetupRequestsSynchAccessor;
	switchSetupRequestsSynchAccessor = NULL;

	delete gameStatsThreadAccessor;
	gameStatsThreadAccessor = NULL;

	delete gameStats;
	gameStats = NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

SwitchSetupRequest ** ServerInterface::getSwitchSetupRequests() {
	MutexSafeWrapper safeMutex(switchSetupRequestsSynchAccessor,CODE_AT_LINE);
    return &switchSetupRequests[0];
}

SwitchSetupRequest * ServerInterface::getSwitchSetupRequests(int index) {
	MutexSafeWrapper safeMutex(switchSetupRequestsSynchAccessor,CODE_AT_LINE);
    return switchSetupRequests[index];
}

void ServerInterface::setSwitchSetupRequests(int index,SwitchSetupRequest *ptr) {
	MutexSafeWrapper safeMutex(switchSetupRequestsSynchAccessor,CODE_AT_LINE);
    switchSetupRequests[index] = ptr;
}

int ServerInterface::isValidClientType(uint32 clientIp) {
	int result = 0;
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		MutexSafeWrapper safeMutex(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		if(slots[index] != NULL) {
			Socket *socket = slots[index]->getSocket();
			if(socket != NULL) {
				uint32 slotIp = socket->getConnectedIPAddress(socket->getIpAddress());
				if(slotIp == clientIp) {
					result = 1;
					break;
				}
			}
		}
	}
	return result;
}

int ServerInterface::isClientAllowedToGetFile(uint32 clientIp, const char *username, const char *filename) {
	int result = 1;

	if(	username != NULL &&
			strlen(username) > 0 &&
		filename != NULL &&
			strlen(filename) > 0) {
		string user = username;
		string file = filename;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d username [%s] file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,username,filename);

		if(StartsWith(user,"tilesets") == true && EndsWith(file,"7z") == false) {
			if(Config::getInstance().getBool("DisableFTPServerXferUncompressedTilesets","false") == true) {
				result = 0;
			}
			else {
				char szIP[100] = "";
				Ip::Inet_NtoA(clientIp,szIP);
				string clientIP = szIP;
				std::vector<std::string> serverList = Socket::getLocalIPAddressList();

				result = 0;
				for(unsigned int index = 0; index < serverList.size(); ++index) {
					string serverIP = serverList[index];

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d clientIP [%s] serverIP [%s] %d / %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientIP.c_str(),serverIP.c_str(),index,serverList.size());

					vector<string> clientTokens;
					Tokenize(clientIP,clientTokens,".");

					vector<string> serverTokens;
					Tokenize(serverIP,serverTokens,".");

					if(clientTokens.size() == 4 && serverTokens.size() == 4) {
						if( clientTokens[0] == serverTokens[0] ||
							clientTokens[1] == serverTokens[1] ||
							clientTokens[2] == serverTokens[2]) {
							result = 1;

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d clientIP [%s] IS NOT BLOCKED\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientIP.c_str());

							break;
						}
					}
				}
			}
		}
	}
	return result;
}

void ServerInterface::addClientToServerIPAddress(uint32 clientIp, uint32 ServerIp) {
	FTPServerThread::addClientToServerIPAddress(clientIp, ServerIp);
}

void ServerInterface::addSlot(int playerIndex) {
	//printf("Adding slot for playerIndex = %d, serverSocket.isPortBound() = %d\n",playerIndex,serverSocket.isPortBound());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] playerIndex is invalid = %d",extractFileFromDirectoryPath(extractFileFromDirectoryPath(__FILE__).c_str()).c_str(),__FUNCTION__,__LINE__,playerIndex);
		throw megaglest_runtime_error(szBuf);
	}
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(serverSocketAdmin != NULL && serverSocketAdmin->isSocketValid() == false) {
		serverSocketAdmin->listen(5);
	}
	if(serverSocket.isPortBound() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		serverSocket.bind(serverSocket.getBindPort());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(playerIndex));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	ConnectionSlot *slot = slots[playerIndex];
	if(slot != NULL) {
		slots[playerIndex] = NULL;
	}
	slots[playerIndex] = new ConnectionSlot(this, playerIndex);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	safeMutexSlot.ReleaseLock();
	delete slot;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	updateListen();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::removeSlot(int playerIndex, int lockedSlotIndex) {
	//printf("Removing slot for playerIndex = %d, serverSocket.isPortBound() = %d\n",playerIndex,serverSocket.isPortBound());

	if(playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] playerIndex is invalid = %d",extractFileFromDirectoryPath(extractFileFromDirectoryPath(__FILE__).c_str()).c_str(),__FUNCTION__,__LINE__,playerIndex);
		throw megaglest_runtime_error(szBuf);
	}

	Lang &lang= Lang::getInstance();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	MutexSafeWrapper safeMutexSlot(NULL,CODE_AT_LINE_X(playerIndex));
	if(playerIndex != lockedSlotIndex) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
		safeMutexSlot.setMutex(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(playerIndex));
	}

	vector<string> msgList;
	ConnectionSlot *slot 				= slots[playerIndex];
	bool notifyDisconnect 				= false;
	const vector<string> languageList 	= this->gameSettings.getUniqueNetworkPlayerLanguages();
	if(slot != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

		if(slot->getLastReceiveCommandListTime() > 0) {
			char szBuf[4096] = "";

			for(unsigned int index = 0; index < languageList.size(); ++index) {
				string msgTemplate = "Player %s, disconnected from the game.";
				if(lang.hasString("PlayerDisconnected",languageList[index]) == true) {
					msgTemplate = lang.getString("PlayerDisconnected",languageList[index]);
				}
#ifdef WIN32
				_snprintf(szBuf,4095,msgTemplate.c_str(),slot->getName().c_str());
#else
				snprintf(szBuf,4095,msgTemplate.c_str(),slot->getName().c_str());
#endif
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szBuf);

				msgList.push_back(szBuf);
			}

			notifyDisconnect = true;
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

	slots[playerIndex]= NULL;
	safeMutexSlot.ReleaseLock();
	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

	if(slot != NULL) slot->close();
	delete slot;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

	updateListen();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

	if(notifyDisconnect == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

		for(unsigned int index = 0; index < languageList.size(); ++index) {
			bool localEcho = lang.isLanguageLocal(languageList[index]);
			queueTextMessage(msgList[index],-1, localEcho, languageList[index]);
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
}

bool ServerInterface::switchSlot(int fromPlayerIndex, int toPlayerIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	bool result = false;

	//printf("#1 Server is switching slots\n");

	if(fromPlayerIndex < 0 || fromPlayerIndex >= GameConstants::maxPlayers) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] fromPlayerIndex is invalid = %d",extractFileFromDirectoryPath(extractFileFromDirectoryPath(__FILE__).c_str()).c_str(),__FUNCTION__,__LINE__,fromPlayerIndex);
		throw megaglest_runtime_error(szBuf);
	}

	if(toPlayerIndex < 0 || toPlayerIndex >= GameConstants::maxPlayers) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] toPlayerIndex is invalid = %d",extractFileFromDirectoryPath(extractFileFromDirectoryPath(__FILE__).c_str()).c_str(),__FUNCTION__,__LINE__,toPlayerIndex);
		throw megaglest_runtime_error(szBuf);
	}

	if(fromPlayerIndex == toPlayerIndex) {
		return false;
	}

	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[fromPlayerIndex],CODE_AT_LINE_X(fromPlayerIndex));
	MutexSafeWrapper safeMutexSlot2(slotAccessorMutexes[toPlayerIndex],CODE_AT_LINE_X(toPlayerIndex));

	//printf("#1a Server is switching slots\n");

	if(slots[toPlayerIndex] != NULL &&
	   slots[toPlayerIndex]->hasValidSocketId() == false) {

		//printf("#2 Server is switching slots\n");

		slots[fromPlayerIndex]->setPlayerIndex(toPlayerIndex);
		slots[toPlayerIndex]->setPlayerIndex(fromPlayerIndex);
		ConnectionSlot *tmp 	= slots[toPlayerIndex];
		slots[toPlayerIndex] 	= slots[fromPlayerIndex];
		slots[fromPlayerIndex] 	= tmp;

		safeMutex.ReleaseLock();

		PlayerIndexMessage playerIndexMessage(toPlayerIndex);
		slots[toPlayerIndex]->sendMessage(&playerIndexMessage);

		//slots[fromPlayerIndex]->resetJoinGameInProgressFlags();
		//slots[toPlayerIndex]->setJoinGameInProgressFlags();

		safeMutexSlot.ReleaseLock();
		safeMutexSlot2.ReleaseLock();
		result = true;
		updateListen();
	}
	else {
		//printf("#3 Server is switching slots aborted, is slot already connected?\n");

		safeMutexSlot.ReleaseLock();
		safeMutexSlot2.ReleaseLock();
		safeMutex.ReleaseLock();
	}
	//printf("#4 Server is switching slots\n");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	return result;
}

ConnectionSlot *ServerInterface::getSlot(int playerIndex) {
	if(playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] playerIndex is invalid = %d",extractFileFromDirectoryPath(extractFileFromDirectoryPath(__FILE__).c_str()).c_str(),__FUNCTION__,__LINE__,playerIndex);
		throw megaglest_runtime_error(szBuf);
	}

	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(playerIndex));
	ConnectionSlot *result = slots[playerIndex];
	return result;
}

bool ServerInterface::isClientConnected(int playerIndex) {
	if(playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] playerIndex is invalid = %d",extractFileFromDirectoryPath(extractFileFromDirectoryPath(__FILE__).c_str()).c_str(),__FUNCTION__,__LINE__,playerIndex);
		throw megaglest_runtime_error(szBuf);
	}

	bool result = false;
	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(playerIndex));
	if(slots[playerIndex] != NULL && slots[playerIndex]->isConnected() == true) {
		result = true;
	}
	return result;
}

bool ServerInterface::hasClientConnection() {
	bool result = false;
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		if(isClientConnected(index) == true) {
			result = true;
			break;
		}
	}
	return result;
}

int ServerInterface::getSlotCount() {
	int slotCount = 0;
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		if(slots[index] != NULL) {
			++slotCount;
		}
	}
	return slotCount;
}

int ServerInterface::getConnectedSlotCount(bool authenticated) {
	int connectedSlotCount = 0;
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		if(slots[index] != NULL && slots[index]->isConnected() == true) {
			if(authenticated == false ||
				(authenticated == true && slots[index]->getConnectHasHandshaked() == true)) {
				++connectedSlotCount;
			}
		}
	}
	return connectedSlotCount;
}

int64 ServerInterface::getNextEventId() {
	nextEventId++;
	if(nextEventId > INT_MAX) {
		nextEventId = 1;
	}
	return nextEventId;
}

std::pair<bool,bool> ServerInterface::clientLagCheck(ConnectionSlot *connectionSlot, bool skipNetworkBroadCast) {
	std::pair<bool,bool> clientLagExceededOrWarned = std::make_pair(false, false);
	static bool alreadyInLagCheck = false;

	if(alreadyInLagCheck == true ||
		(connectionSlot != NULL && (connectionSlot->getSkipLagCheck() == true ||
		 connectionSlot->getConnectHasHandshaked() == false))) {
		return clientLagExceededOrWarned;
	}

	try {
		alreadyInLagCheck = true;

		if((gameStartTime > 0 &&
				difftime((long int)time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD) &&
			(resumeGameStartTime == 0 ||
			  (resumeGameStartTime > 0 &&
					  difftime((long int)time(NULL),resumeGameStartTime) >= LAG_CHECK_GRACE_PERIOD))) {
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {

				double clientLag 		= this->getCurrentFrameCount() - connectionSlot->getCurrentFrameCount();
				double clientLagCount 	= (gameSettings.getNetworkFramePeriod() > 0 ? (clientLag / gameSettings.getNetworkFramePeriod()) : 0);
				connectionSlot->setCurrentLagCount(clientLagCount);

				double clientLagTime 	= difftime((long int)time(NULL),connectionSlot->getLastReceiveCommandListTime());

				if(this->getCurrentFrameCount() > 0) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, clientLag = %f, clientLagCount = %f, this->getCurrentFrameCount() = %d, connectionSlot->getCurrentFrameCount() = %d, clientLagTime = %f\n",
																		 extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,
																		 connectionSlot->getPlayerIndex(),
																		 clientLag,clientLagCount,
																		 this->getCurrentFrameCount(),
																		 connectionSlot->getCurrentFrameCount(),
																		 clientLagTime);
				}

				// TEST LAG Error and warnings!!!
				//clientLagCount = maxFrameCountLagAllowed + 1;
				//clientLagTime = maxClientLagTimeAllowed + 1;
				/*
				if(difftime(time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD + 5) {
					clientLagTime = maxClientLagTimeAllowed + 1;
				}
				else if(difftime(time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD) {
					clientLagTime = (maxClientLagTimeAllowed * warnFrameCountLagPercent) + 1;
				}
				*/
				// END test


				// New lag check
				if((maxFrameCountLagAllowed > 0 && clientLagCount > maxFrameCountLagAllowed) ||
					(maxClientLagTimeAllowed > 0 && clientLagTime > maxClientLagTimeAllowed) ||
					(maxFrameCountLagAllowedEver > 0 && clientLagCount > maxFrameCountLagAllowedEver) ||
					( maxClientLagTimeAllowedEver > 0 && clientLagTime > maxClientLagTimeAllowedEver)) {
					clientLagExceededOrWarned.first = true;

			    	Lang &lang= Lang::getInstance();
			    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
			    	for(unsigned int index = 0; index < languageList.size(); ++index) {
						char szBuf[4096]="";

						string msgTemplate = "DROPPING %s, exceeded max allowed LAG count of %f [time = %f], clientLag = %f [%f], disconnecting client.";
						if(lang.hasString("ClientLagDropping") == true) {
							msgTemplate = lang.getString("ClientLagDropping",languageList[index]);
						}
						if(gameSettings.getNetworkPauseGameForLaggedClients() == true &&
							((maxFrameCountLagAllowedEver <= 0 || clientLagCount <= maxFrameCountLagAllowedEver) &&
							 (maxClientLagTimeAllowedEver <= 0 || clientLagTime <= maxClientLagTimeAllowedEver))) {
							msgTemplate = "PAUSING GAME TEMPORARILY for %s, exceeded max allowed LAG count of %f [time = %f], clientLag = %f [%f], waiting for client to catch up...";
							if(lang.hasString("ClientLagPausing") == true) {
								msgTemplate = lang.getString("ClientLagPausing",languageList[index]);
							}
						}
#ifdef WIN32
						_snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str() ,maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
#else
						snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
#endif
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szBuf);

						if(skipNetworkBroadCast == false) {
							string sMsg = szBuf;
							bool echoLocal = lang.isLanguageLocal(languageList[index]);
							sendTextMessage(sMsg,-1, echoLocal, languageList[index], connectionSlot->getPlayerIndex());
						}
			    	}

					if(gameSettings.getNetworkPauseGameForLaggedClients() == false ||
						(maxFrameCountLagAllowedEver > 0 && clientLagCount > maxFrameCountLagAllowedEver) ||
						(maxClientLagTimeAllowedEver > 0 && clientLagTime > maxClientLagTimeAllowedEver)) {

						//printf("Closing connection slot lagged out!\n");
						connectionSlot->close();
					}

				}
				// New lag check warning
				else if((maxFrameCountLagAllowed > 0 && warnFrameCountLagPercent > 0 &&
						 clientLagCount > (maxFrameCountLagAllowed * warnFrameCountLagPercent)) ||
						(maxClientLagTimeAllowed > 0 && warnFrameCountLagPercent > 0 &&
						 clientLagTime > (maxClientLagTimeAllowed * warnFrameCountLagPercent)) ) {
					clientLagExceededOrWarned.second = true;

					if(connectionSlot->getLagCountWarning() == false) {
						connectionSlot->setLagCountWarning(true);

				    	Lang &lang= Lang::getInstance();
				    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
				    	for(unsigned int index = 0; index < languageList.size(); ++index) {
				    		char szBuf[4096]="";

				    		string msgTemplate = "LAG WARNING for %s, may exceed max allowed LAG count of %f [time = %f], clientLag = %f [%f], WARNING...";
							if(lang.hasString("ClientLagWarning") == true) {
								msgTemplate = lang.getString("ClientLagWarning",languageList[index]);
							}

		#ifdef WIN32
				    		_snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#else
				    		snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#endif
				    		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,szBuf);

							if(skipNetworkBroadCast == false) {
								string sMsg = szBuf;
								bool echoLocal = lang.isLanguageLocal(languageList[index]);
								sendTextMessage(sMsg,-1, echoLocal, languageList[index], connectionSlot->getPlayerIndex());
							}
				    	}
					}
				}
				else if(connectionSlot->getLagCountWarning() == true) {
					connectionSlot->setLagCountWarning(false);
				}
			}
		}
	}
	catch(const exception &ex) {
		alreadyInLagCheck = false;

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		throw megaglest_runtime_error(ex.what());
	}

	alreadyInLagCheck = false;
	return clientLagExceededOrWarned;
}

void ServerInterface::updateSocketTriggeredList(std::map<PLATFORM_SOCKET,bool> & socketTriggeredList) {
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot *connectionSlot = slots[index];
		if(connectionSlot != NULL) {
			PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
			if(Socket::isSocketValid(&clientSocket) == true) {
				socketTriggeredList[clientSocket] = false;
			}
		}
	}
}

void ServerInterface::validateConnectedClients() {
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot* connectionSlot = slots[index];
		if(connectionSlot != NULL) {
			connectionSlot->validateConnection();
		}
	}
}

bool ServerInterface::signalClientReceiveCommands(ConnectionSlot *connectionSlot,
		int slotIndex, bool socketTriggered, ConnectionSlotEvent & event) {
	bool slotSignalled 		= false;

	event.eventType 		= eReceiveSocketData;
	event.networkMessage 	= NULL;
	event.connectionSlot 	= connectionSlot;
	event.socketTriggered 	= socketTriggered;
	event.triggerId 		= slotIndex;
	event.eventId 			= getNextEventId();

	if(connectionSlot != NULL) {
		if(socketTriggered == true || connectionSlot->isConnected() == false) {
			connectionSlot->signalUpdate(&event);
			slotSignalled = true;
		}
	}
	return slotSignalled;
}

void ServerInterface::signalClientsToRecieveData(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList,
												 std::map<int,ConnectionSlotEvent> &eventList,
												 std::map<int,bool> & mapSlotSignalledList) {
	//printf("====================================In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//printf("Signal clients get new data\n");
	const bool newThreadManager = Config::getInstance().getBool("EnableNewThreadManager","false");
	if(newThreadManager == true) {
		masterController.clearSlaves(true);
		std::vector<SlaveThreadControllerInterface *> slaveThreadList;
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot* connectionSlot = slots[i];

			bool socketTriggered = false;

			if(connectionSlot != NULL) {
				PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
				if(Socket::isSocketValid(&clientSocket)) {
					socketTriggered = socketTriggeredList[clientSocket];
				}
				else if(this->getGameHasBeenInitiated() == true &&
						this->getAllowInGameConnections() == true) {
					socketTriggeredList[clientSocket] = true;
					socketTriggered = socketTriggeredList[clientSocket];
				}
			}
			ConnectionSlotEvent &event = eventList[i];
			event.eventType 		= eReceiveSocketData;
			event.networkMessage 	= NULL;
			event.connectionSlot 	= connectionSlot;
			event.socketTriggered 	= socketTriggered;
			event.triggerId 		= i;
			event.eventId 			= getNextEventId();

			if(connectionSlot != NULL) {
				if(socketTriggered == true || connectionSlot->isConnected() == false) {
					if(connectionSlot->getWorkerThread() != NULL) {
						slaveThreadList.push_back(connectionSlot->getWorkerThread());
						mapSlotSignalledList[i] = true;
					}
				}
			}
		}
		masterController.setSlaves(slaveThreadList);
		masterController.signalSlaves(&eventList);
	}
	else {
		for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
			ConnectionSlot *connectionSlot = slots[index];

			if(connectionSlot != NULL) {
				bool socketTriggered = false;
				PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
				if(Socket::isSocketValid(&clientSocket)) {
					socketTriggered = socketTriggeredList[clientSocket];
				}

				ConnectionSlotEvent &event = eventList[index];
				bool socketSignalled = signalClientReceiveCommands(connectionSlot,index,socketTriggered,event);
				if(connectionSlot != NULL && socketTriggered == true) {
					mapSlotSignalledList[index] = socketSignalled;
				}
			}
		}
	}
}

void ServerInterface::checkForCompletedClients(std::map<int,bool> & mapSlotSignalledList,
											   std::vector <string> &errorMsgList,
											   std::map<int,ConnectionSlotEvent> &eventList) {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	const bool newThreadManager = Config::getInstance().getBool("EnableNewThreadManager","false");
	if(newThreadManager == true) {
		//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		//bool slavesCompleted = masterController.waitTillSlavesTrigger(1000 * MAX_SLOT_THREAD_WAIT_TIME);
		masterController.waitTillSlavesTrigger(1000 * MAX_SLOT_THREAD_WAIT_TIME);
		masterController.clearSlaves(true);
		//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));

			ConnectionSlot* connectionSlot = slots[i];
			if(connectionSlot != NULL && mapSlotSignalledList[i] == true) {
				try {
					std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
					// Collect any collected errors from threads
					if(errorList.empty() == false) {
						for(int iErrIdx = 0; iErrIdx < (int)errorList.size(); ++iErrIdx) {
							string &sErr = errorList[iErrIdx];
							if(sErr != "") {
								errorMsgList.push_back(sErr);
							}
						}
						connectionSlot->clearThreadErrorList();
					}
				}
				catch(const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

					errorMsgList.push_back(ex.what());
				}
			}
		}
		//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		time_t waitForThreadElapsed = time(NULL);
		std::map<int,bool> slotsCompleted;
		for(bool threadsDone = false;
			exitServer == false && threadsDone == false &&
			difftime((long int)time(NULL),waitForThreadElapsed) < MAX_SLOT_THREAD_WAIT_TIME;) {

			threadsDone = true;
			// Examine all threads for completion of delegation
			for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
				//printf("===> START slot %d [%p] - About to checkForCompletedClients\n",i,slots[i]);
				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));

				//printf("===> IN slot %d - About to checkForCompletedClients\n",i);

				ConnectionSlot* connectionSlot = slots[index];
				if(connectionSlot != NULL &&
					connectionSlot->isConnected() == true &&
					mapSlotSignalledList[index] == true &&
					connectionSlot->getJoinGameInProgress() == false &&
					slotsCompleted.find(index) == slotsCompleted.end()) {

					try {
						std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
						// Collect any collected errors from threads
						if(errorList.empty() == false) {
							for(int iErrIdx = 0; iErrIdx < (int)errorList.size(); ++iErrIdx) {
								string &sErr = errorList[iErrIdx];
								if(sErr != "") {
									errorMsgList.push_back(sErr);
								}
							}
							connectionSlot->clearThreadErrorList();
						}

						// Not done waiting for data yet
						bool updateFinished = (connectionSlot != NULL ? connectionSlot->updateCompleted(&eventList[index]) : true);
						if(updateFinished == false) {
							threadsDone = false;
							break;
						}
						else {
							slotsCompleted[index] = true;
						}
					}
					catch(const exception &ex) {
						SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

						errorMsgList.push_back(ex.what());
					}
				}
				//printf("===> END slot %d - About to checkForCompletedClients\n",i);
			}
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::checkForLaggingClients(std::map<int,bool> &mapSlotSignalledList,
											std::map<int,ConnectionSlotEvent> &eventList,
											std::map<PLATFORM_SOCKET,bool> &socketTriggeredList,
											std::vector <string> &errorMsgList) {
	bool lastGlobalLagCheckTimeUpdate = false;
	if(gameHasBeenInitiated == true) {

		time_t waitForClientsElapsed = time(NULL);
		time_t waitForThreadElapsed = time(NULL);
		std::map<int,bool> slotsCompleted;
		std::map<int,bool> slotsWarnedList;

		for(bool threadsDone = false;
			exitServer == false && threadsDone == false &&
			difftime((long int)time(NULL),waitForThreadElapsed) < MAX_SLOT_THREAD_WAIT_TIME;) {

			threadsDone = true;
			// Examine all threads for completion of delegation
			for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
				//printf("#1 Check lag for i: %d\n",i);

				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
				ConnectionSlot* connectionSlot = slots[index];

				if(connectionSlot != NULL && connectionSlot->isConnected() == true &&
					connectionSlot->getSkipLagCheck() == false &&
					mapSlotSignalledList[index] == true &&
				    slotsCompleted.find(index) == slotsCompleted.end()) {

					//printf("#2 Check lag for i: %d playerindex: %d name [%s] socket: %d\n",i,connectionSlot->getPlayerIndex(),connectionSlot->getName().c_str(),connectionSlot->getSocketId());
					try {
						std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
						// Show any collected errors from threads
						if(errorList.empty() == false) {
							for(int iErrIdx = 0; iErrIdx < (int)errorList.size(); ++iErrIdx) {
								string &sErr = errorList[iErrIdx];
								if(sErr != "") {
									errorMsgList.push_back(sErr);
								}
							}
							connectionSlot->clearThreadErrorList();
						}

						// Not done waiting for data yet
						bool updateFinished = (connectionSlot != NULL ? connectionSlot->updateCompleted(&eventList[index]) : true);
						if(updateFinished == false) {
							//printf("#2a Check lag for i: %d\n",i);
							threadsDone = false;
							break;
						}
						else {
							// New lag check
							std::pair<bool,bool> clientLagExceededOrWarned = std::make_pair(false,false);
							if( gameHasBeenInitiated == true && connectionSlot != NULL &&
								connectionSlot->isConnected() == true) {
								clientLagExceededOrWarned = clientLagCheck(connectionSlot,slotsWarnedList[index]);

								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, gameSettings.getNetworkPauseGameForLaggedClients() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,gameSettings.getNetworkPauseGameForLaggedClients());

								if(clientLagExceededOrWarned.first == true) {
									slotsWarnedList[index] = true;
								}
							}
							// If the client has exceeded lag and the server wants
							// to pause while they catch up, re-trigger the
							// client reader thread
							if((clientLagExceededOrWarned.first == true &&
								gameSettings.getNetworkPauseGameForLaggedClients() == true)) {

								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, difftime(time(NULL),waitForClientsElapsed) = %.2f, MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE = %.2f\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,difftime((long int)time(NULL),waitForClientsElapsed),MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE);

								if(difftime((long int)time(NULL),waitForClientsElapsed) < MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE) {
									if(connectionSlot != NULL) {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second);

										bool socketTriggered = false;
										PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
										if(clientSocket > 0) {
											socketTriggered = socketTriggeredList[clientSocket];
										}
										ConnectionSlotEvent &event 	= eventList[index];
										mapSlotSignalledList[index] = signalClientReceiveCommands(connectionSlot,index,socketTriggered,event);
										threadsDone 				= false;
									}
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second);
								}
							}
							else {
								slotsCompleted[index] = true;
							}
						}
					}
					catch(const exception &ex) {
						SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
						errorMsgList.push_back(ex.what());
					}
				}

				//printf("#3 Check lag for i: %d\n",i);
				if(connectionSlot != NULL &&
					connectionSlot->isConnected() == true &&
					connectionSlot->getSkipLagCheck() == false) {
					//printf("#4 Check lag for i: %d\n",i);

					try {
						if(gameHasBeenInitiated == true &&
							difftime((long int)time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD &&
							difftime((long int)time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_INTERVAL_PERIOD) {

							//printf("\n\n\n^^^^^^^^^^^^^^ PART A\n\n\n");

							// New lag check
							std::pair<bool,bool> clientLagExceededOrWarned = std::make_pair(false,false);
							if( connectionSlot != NULL && connectionSlot->isConnected() == true) {
								//printf("\n\n\n^^^^^^^^^^^^^^ PART B\n\n\n");

								lastGlobalLagCheckTimeUpdate = true;
								clientLagExceededOrWarned = clientLagCheck(connectionSlot,slotsWarnedList[index]);

								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, gameSettings.getNetworkPauseGameForLaggedClients() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,gameSettings.getNetworkPauseGameForLaggedClients());

								if(clientLagExceededOrWarned.first == true) {
									slotsWarnedList[index] = true;
								}

								// If the client has exceeded lag and the server wants
								// to pause while they catch up, re-trigger the
								// client reader thread
								if((clientLagExceededOrWarned.first == true &&
									gameSettings.getNetworkPauseGameForLaggedClients() == true)) {

									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, difftime(time(NULL),waitForClientsElapsed) = %.2f, MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE = %.2f\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,difftime((long int)time(NULL),waitForClientsElapsed),MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE);

									if(difftime((long int)time(NULL),waitForClientsElapsed) < MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE) {
										if( connectionSlot != NULL && connectionSlot->isConnected() == true) {
											threadsDone = false;
										}
									}
								}
							}
						}
					}
					catch(const exception &ex) {
						SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
						errorMsgList.push_back(ex.what());
					}
				}

				//printf("#5 Check lag for i: %d\n",i);
			}
		}
	}
	if(lastGlobalLagCheckTimeUpdate == true) {
		lastGlobalLagCheckTime = time(NULL);
	}
}

void ServerInterface::executeNetworkCommandsFromClients() {
	if(gameHasBeenInitiated == true) {
		for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
			ConnectionSlot* connectionSlot= slots[index];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				vector<NetworkCommand> pendingList = connectionSlot->getPendingNetworkCommandList(true);
				if(pendingList.empty() == false) {
					for(int idx = 0; exitServer == false && idx < (int)pendingList.size(); ++idx) {
						NetworkCommand &cmd = pendingList[idx];
						this->requestCommand(&cmd);
					}
					//printf("Executed: %d commands from slot: %d\n",pendingList.size(),index);
				}
			}
		}
	}
}

void ServerInterface::dispatchPendingChatMessages(std::vector <string> &errorMsgList) {
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {

		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot *connectionSlot = slots[index];

		if(connectionSlot != NULL &&
		   connectionSlot->getChatTextList(false).empty() == false) {
			try {
				std::vector<ChatMsgInfo> chatText = connectionSlot->getChatTextList(true);
				for(int chatIdx = 0;
					exitServer == false && slots[index] != NULL &&
					chatIdx < (int)chatText.size(); chatIdx++) {

					connectionSlot = slots[index];
					if(connectionSlot != NULL) {
						ChatMsgInfo msg(chatText[chatIdx]);
						this->addChatInfo(msg);

						string newChatText     = msg.chatText.c_str();
						int newChatTeamIndex   = msg.chatTeamIndex;
						int newChatPlayerIndex = msg.chatPlayerIndex;
						string newChatLanguage = msg.targetLanguage;

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 about to broadcast nmtText chatText [%s] chatTeamIndex = %d, newChatPlayerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex);

						if(newChatLanguage == "" ||
							newChatLanguage == connectionSlot->getNetworkPlayerLanguage()) {

							NetworkMessageText networkMessageText(newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex,newChatLanguage);
							broadcastMessage(&networkMessageText, connectionSlot->getPlayerIndex(),index);
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);
					}
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,index);
				// Its possible that the slot is disconnected here
				// so check the original pointer again
				if(slots[index] != NULL) {
					slots[index]->clearChatInfo();
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				errorMsgList.push_back(ex.what());
			}
		}
	}
}

void ServerInterface::dispatchPendingMarkCellMessages(std::vector <string> &errorMsgList) {
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {

		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot* connectionSlot= slots[index];

		if(connectionSlot != NULL &&
		   connectionSlot->getMarkedCellList(false).empty() == false) {

			try {
				std::vector<MarkedCell> chatText = connectionSlot->getMarkedCellList(true);
				for(int chatIdx = 0;
					exitServer == false && slots[index] != NULL &&
					chatIdx < (int)chatText.size(); chatIdx++) {

					connectionSlot= slots[index];
					if(connectionSlot != NULL) {
						MarkedCell msg(chatText[chatIdx]);
						this->addMarkedCell(msg);

						NetworkMessageMarkCell networkMessageMarkCell(msg.getTargetPos(),msg.getFactionIndex(),msg.getNote(),msg.getPlayerIndex());
						broadcastMessage(&networkMessageMarkCell, connectionSlot->getPlayerIndex(),index);
						//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);
					}
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] i = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,index);
				// Its possible that the slot is disconnected here
				// so check the original pointer again
				if(slots[index] != NULL) {
					slots[index]->clearMarkedCellList();
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				errorMsgList.push_back(ex.what());
			}
		}
	}
}
void ServerInterface::dispatchPendingHighlightCellMessages(std::vector <string> &errorMsgList) {
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {

		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot* connectionSlot= slots[index];
		if(connectionSlot != NULL &&
		   connectionSlot->getHighlightedCellList(false).empty() == false) {

			try {
				std::vector<MarkedCell> highlightedCells = connectionSlot->getHighlightedCellList(true);
				for(int chatIdx = 0;
					exitServer == false && slots[index] != NULL &&
					chatIdx < (int)highlightedCells.size(); chatIdx++) {

					connectionSlot= slots[index];
					if(connectionSlot != NULL) {
						MarkedCell msg(highlightedCells[chatIdx]);
						this->setHighlightedCell(msg);

						NetworkMessageHighlightCell networkMessageHighlightCell(msg.getTargetPos(),msg.getFactionIndex());
						broadcastMessage(&networkMessageHighlightCell, connectionSlot->getPlayerIndex(),index);
					}
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,index);
				// Its possible that the slot is disconnected here
				// so check the original pointer again
				if(slots[index] != NULL) {
					slots[index]->clearHighlightedCellList();
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				errorMsgList.push_back(ex.what());
			}
		}
	}
}

void ServerInterface::dispatchPendingUnMarkCellMessages(std::vector <string> &errorMsgList) {
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {

		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot* connectionSlot= slots[index];
		if(connectionSlot != NULL &&
		   connectionSlot->getUnMarkedCellList(false).empty() == false) {

			try {
				std::vector<UnMarkedCell> chatText = connectionSlot->getUnMarkedCellList(true);
				for(int chatIdx = 0;
					exitServer == false && slots[index] != NULL &&
					chatIdx < (int)chatText.size(); chatIdx++) {

					connectionSlot = slots[index];
					if(connectionSlot != NULL) {
						UnMarkedCell msg(chatText[chatIdx]);
						this->addUnMarkedCell(msg);

						NetworkMessageUnMarkCell networkMessageMarkCell(msg.getTargetPos(),msg.getFactionIndex());
						broadcastMessage(&networkMessageMarkCell, connectionSlot->getPlayerIndex(),index);
						//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);
					}
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] i = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,index);
				// Its possible that the slot is disconnected here
				// so check the original pointer again
				if(slots[index] != NULL) {
					slots[index]->clearUnMarkedCellList();
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				errorMsgList.push_back(ex.what());
			}
		}
	}
}

void ServerInterface::update() {
	//printf("\nServerInterface::update -- A\n");

	std::vector <string> errorMsgList;
	try {
		// The first thing we will do is check all clients to ensure they have
		// properly identified themselves within the alloted time period
		validateConnectedClients();

		//printf("\nServerInterface::update -- B\n");

		processTextMessageQueue();
		processBroadCastMessageQueue();

		//printf("\nServerInterface::update -- C\n");

		std::map<PLATFORM_SOCKET,bool> socketTriggeredList;
		//update all slots
		updateSocketTriggeredList(socketTriggeredList);

		//printf("\nServerInterface::update -- D\n");

		if(gameHasBeenInitiated == false ||
			socketTriggeredList.empty() == false) {
			//printf("\nServerInterface::update -- E\n");

			std::map<int,ConnectionSlotEvent> eventList;

			bool hasData = false;
			if(gameHasBeenInitiated == false) {
				hasData = Socket::hasDataToRead(socketTriggeredList);
			}
			else {
				hasData = true;
			}

			if(hasData && SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] hasData == true\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

			if(gameHasBeenInitiated == false || hasData == true) {
				//printf("START Server update #2\n");
				std::map<int,bool> mapSlotSignalledList;

				// Step #1 tell all connection slot worker threads to receive socket data
				if(gameHasBeenInitiated == false) signalClientsToRecieveData(socketTriggeredList, eventList, mapSlotSignalledList);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #2\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				//printf("START Server update #2\n");
				if(gameHasBeenInitiated == false || hasData == true) {
					//printf("START Server update #3\n");

					// Step #2 check all connection slot worker threads for completed status
					if(gameHasBeenInitiated == false) {
						checkForCompletedClients(mapSlotSignalledList,errorMsgList, eventList);
					}
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #3\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					//printf("START Server update #4\n");
					//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					// Step #3 check clients for any lagging scenarios and try to deal with them
					if(gameHasBeenInitiated == false) {
						checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
					}
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #4\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					//printf("START Server update #5\n");
					//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					// Step #4 dispatch network commands to the pending list so that they are done in proper order
					executeNetworkCommandsFromClients();
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #5\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					//printf("START Server update #6\n");
					//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					// Step #5 dispatch pending chat messages
					dispatchPendingChatMessages(errorMsgList);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					dispatchPendingMarkCellMessages(errorMsgList);
					dispatchPendingUnMarkCellMessages(errorMsgList);

					dispatchPendingHighlightCellMessages(errorMsgList);

					if(gameHasBeenInitiated == true &&
						difftime((long int)time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD &&
						difftime((long int)time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_INTERVAL_PERIOD) {

						std::map<int,bool> mapSlotSignalledList;
						checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
					}
					//printf("START Server update #7\n");
				}
				else if(gameHasBeenInitiated == true &&
						difftime((long int)time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD &&
						difftime((long int)time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_INTERVAL_PERIOD) {

					//printf("Skip network data process because hasData == false\n");
					//printf("START Server update #8\n");

					std::map<int,bool> mapSlotSignalledList;
					checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
				}
				//printf("START Server update #9\n");
			}
			else if(gameHasBeenInitiated == true &&
					difftime((long int)time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD &&
					difftime((long int)time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_INTERVAL_PERIOD) {

				//printf("\nServerInterface::update -- E1\n");
				//printf("START Server update #10\n");

				std::map<int,bool> mapSlotSignalledList;
				checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
			}
			//printf("START Server update #11\n");
		}
		else if(gameHasBeenInitiated == true &&
				difftime((long int)time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD &&
				difftime((long int)time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_INTERVAL_PERIOD) {

			//printf("\nServerInterface::update -- F\n");
			//printf("START Server update #12\n");

			std::map<int,ConnectionSlotEvent> eventList;
			std::map<int,bool> mapSlotSignalledList;

			checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
		}
		//printf("START Server update #13\n");

		// Check if we need to switch masterserver admin to a new player because original admin disconnected
		if(gameHasBeenInitiated == true &&
			this->gameSettings.getMasterserver_admin() > 0) {

			bool foundAdminSlot 	= false;
			int iFirstConnectedSlot = -1;
			for(int index = 0; index < GameConstants::maxPlayers; ++index) {
				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
				if(slots[index] != NULL && slots[index]->isConnected() == true) {
					if(iFirstConnectedSlot < 0) {
						iFirstConnectedSlot = index;
					}
					if(this->gameSettings.getMasterserver_admin() == slots[index]->getSessionKey()) {
						foundAdminSlot = true;
						break;
					}
				}
			}

			if(foundAdminSlot == false && iFirstConnectedSlot >= 0) {
				printf("Switching masterserver admin to slot#%d...\n",iFirstConnectedSlot);

				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[iFirstConnectedSlot],CODE_AT_LINE_X(iFirstConnectedSlot));
				if(slots[iFirstConnectedSlot] != NULL) {
					string sMsg = "Switching player to admin mode: " + slots[iFirstConnectedSlot]->getName();
					sendTextMessage(sMsg,-1, true,"");

					this->gameSettings.setMasterserver_admin(slots[iFirstConnectedSlot]->getSessionKey());
					this->gameSettings.setMasterserver_admin_faction_index(slots[iFirstConnectedSlot]->getPlayerIndex());

					safeMutexSlot.ReleaseLock();
					this->broadcastGameSetup(&this->gameSettings);
				}
			}
		}
		//printf("\nServerInterface::update -- G\n");
		//printf("START Server update #14\n");

		checkListenerSlots();

		//printf("START Server update #15\n");
	}
	catch(const exception &ex) {
		//printf("\nServerInterface::update -- H\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		errorMsgList.push_back(ex.what());
	}

	if(errorMsgList.empty() == false){
		for(int iErrIdx = 0; iErrIdx < (int)errorMsgList.size(); ++iErrIdx) {
			string &sErr = errorMsgList[iErrIdx];
			if(sErr != "") {
				DisplayErrorMessage(sErr);
			}
		}
	}
	//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::updateKeyframe(int frameCount) {
	currentFrameCount = frameCount;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] currentFrameCount = %d, requestedCommands.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,currentFrameCount,requestedCommands.size());

	NetworkMessageCommandList networkMessageCommandList(frameCount);
	for(int index = 0; index < GameConstants::maxPlayers; ++index) {
		networkMessageCommandList.setNetworkPlayerFactionCRC(index,this->getNetworkPlayerFactionCRC(index));
	}

	while(requestedCommands.empty() == false) {
		if(networkMessageCommandList.addCommand(&requestedCommands.back())) {
			pendingCommands.push_back(requestedCommands.back());
			requestedCommands.pop_back();
		}
		else {
			break;
		}
	}

	try {
		// Possible cause of out of synch since we have more commands that need
		// to be sent in this frame
		if(requestedCommands.empty() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,requestedCommands.size());
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,requestedCommands.size());

			string sMsg = "may go out of synch: server requestedCommands.size() = " + intToStr(requestedCommands.size());
			sendTextMessage(sMsg,-1, true,"");
		}

		//broadcast commands
		//printf("START Server send currentFrameCount = %d\n",currentFrameCount);

		broadcastMessage(&networkMessageCommandList);

		//printf("END Server send currentFrameCount = %d\n",currentFrameCount);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}
}

bool ServerInterface::shouldDiscardNetworkMessage(NetworkMessageType networkMessageType,
												ConnectionSlot *connectionSlot) {
	bool discard = false;
	if(connectionSlot != NULL) {
		switch(networkMessageType) {
			case nmtIntro:
				{
				discard = true;
				NetworkMessageIntro msg = NetworkMessageIntro();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtPing:
				{
				discard = true;
				NetworkMessagePing msg = NetworkMessagePing();
				connectionSlot->receiveMessage(&msg);
				lastPingInfo = msg;
				}
				break;

			case nmtLaunch:
				{
				discard = true;
				NetworkMessageLaunch msg = NetworkMessageLaunch();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtText:
				{
				discard = true;
				NetworkMessageText netMsg = NetworkMessageText();
				connectionSlot->receiveMessage(&netMsg);

				ChatMsgInfo msg(netMsg.getText().c_str(),netMsg.getTeamIndex(),netMsg.getPlayerIndex(),netMsg.getTargetLanguage());
				this->addChatInfo(msg);

				string newChatText     = msg.chatText.c_str();
				//string newChatSender   = msg.chatSender.c_str();
				int newChatTeamIndex   = msg.chatTeamIndex;
				int newChatPlayerIndex = msg.chatPlayerIndex;
				string newChatLanguage = msg.targetLanguage.c_str();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 about to broadcast nmtText chatText [%s] chatTeamIndex = %d, newChatPlayerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex);

				NetworkMessageText networkMessageText(newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex,newChatLanguage);
				broadcastMessage(&networkMessageText, connectionSlot->getPlayerIndex());

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);

				}
				break;

			case nmtMarkCell:
				{
				discard = true;
				NetworkMessageMarkCell networkMessageMarkCell;
				connectionSlot->receiveMessage(&networkMessageMarkCell);

            	MarkedCell msg(networkMessageMarkCell.getTarget(),
            			       networkMessageMarkCell.getFactionIndex(),
            			       networkMessageMarkCell.getText().c_str(),
            			       networkMessageMarkCell.getPlayerIndex());

            	this->addMarkedCell(msg);

            	NetworkMessageMarkCell networkMessageMarkCellBroadcast(
            			networkMessageMarkCell.getTarget(),
     			        networkMessageMarkCell.getFactionIndex(),
     			        networkMessageMarkCell.getText().c_str(),
     			        networkMessageMarkCell.getPlayerIndex());
				broadcastMessage(&networkMessageMarkCellBroadcast, connectionSlot->getPlayerIndex());

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtMarkCell chatText [%s] chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);

				}
				break;

			case nmtUnMarkCell:
				{
				discard = true;
				NetworkMessageUnMarkCell networkMessageMarkCell;
				connectionSlot->receiveMessage(&networkMessageMarkCell);

            	UnMarkedCell msg(networkMessageMarkCell.getTarget(),
            			       networkMessageMarkCell.getFactionIndex());

            	this->addUnMarkedCell(msg);

            	NetworkMessageUnMarkCell networkMessageMarkCellBroadcast(
            			networkMessageMarkCell.getTarget(),
     			        networkMessageMarkCell.getFactionIndex());
				broadcastMessage(&networkMessageMarkCellBroadcast, connectionSlot->getPlayerIndex());

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtMarkCell chatText [%s] chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);

				}
				break;
			case nmtHighlightCell:
				{
				discard = true;
				NetworkMessageHighlightCell networkMessageHighlightCell;
				connectionSlot->receiveMessage(&networkMessageHighlightCell);

            	MarkedCell msg(networkMessageHighlightCell.getTarget(),
            			networkMessageHighlightCell.getFactionIndex(),
            			"none",-1);

            	this->setHighlightedCell(msg);

            	NetworkMessageHighlightCell networkMessageHighlightCellBroadcast(
            			networkMessageHighlightCell.getTarget(),
            			networkMessageHighlightCell.getFactionIndex());
				broadcastMessage(&networkMessageHighlightCellBroadcast, connectionSlot->getPlayerIndex());

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtMarkCell  chatTeamIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,newChatTeamIndex);

				}
				break;

			case nmtSynchNetworkGameData:
				{
				discard = true;
				NetworkMessageSynchNetworkGameData msg = NetworkMessageSynchNetworkGameData();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtSynchNetworkGameDataStatus:
				{
				discard = true;
				NetworkMessageSynchNetworkGameDataStatus msg = NetworkMessageSynchNetworkGameDataStatus();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtSynchNetworkGameDataFileCRCCheck:
				{
				discard = true;
				NetworkMessageSynchNetworkGameDataFileCRCCheck msg = NetworkMessageSynchNetworkGameDataFileCRCCheck();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtSynchNetworkGameDataFileGet:
				{
				discard = true;
				NetworkMessageSynchNetworkGameDataFileGet msg = NetworkMessageSynchNetworkGameDataFileGet();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtSwitchSetupRequest:
				{
				discard = true;
				SwitchSetupRequest msg = SwitchSetupRequest();
				connectionSlot->receiveMessage(&msg);
				}
				break;
			case nmtPlayerIndexMessage:
				{
				discard = true;
				PlayerIndexMessage msg = PlayerIndexMessage(0);
				connectionSlot->receiveMessage(&msg);
				}
				break;
		}
	}
	return discard;
}

void ServerInterface::waitUntilReady(Checksum *checksum) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] START\n",__FUNCTION__);
	Logger & logger = Logger::getInstance();
	gameHasBeenInitiated = true;
	Chrono chrono;
	chrono.start();

	bool allReady = false;

	if(Config::getInstance().getBool("EnableGameServerLoadCancel","false") == true) {
		logger.setCancelLoadingEnabled(true);
	}

	Lang &lang							= Lang::getInstance();
	uint64 waitLoopIterationCount 		= 0;
	uint64 MAX_LOOP_COUNT_BEFORE_SLEEP 	= 10;
	MAX_LOOP_COUNT_BEFORE_SLEEP 		= Config::getInstance().getInt("NetworkServerLoopGameLoadingCap",intToStr(MAX_LOOP_COUNT_BEFORE_SLEEP).c_str());
	if(MAX_LOOP_COUNT_BEFORE_SLEEP == 0) {
		MAX_LOOP_COUNT_BEFORE_SLEEP = 1;
	}
	int sleepMillis 		= Config::getInstance().getInt("NetworkServerLoopGameLoadingCapSleepMillis","10");
	int64 lastStatusUpdate 	= 0;

	while(exitServer == false &&
			allReady == false &&
			logger.getCancelLoading() == false) {

		waitLoopIterationCount++;
		if(waitLoopIterationCount > 0 &&
			waitLoopIterationCount % MAX_LOOP_COUNT_BEFORE_SLEEP == 0) {

			sleep(sleepMillis);
			waitLoopIterationCount = 0;
		}
		vector<string> waitingForHosts;
		allReady= true;
		for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index)	{

			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
			ConnectionSlot* connectionSlot= slots[index];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true)	{
				if(connectionSlot->isReady() == false) {

					NetworkMessageType networkMessageType= connectionSlot->getNextMessageType();

					// consume old messages from the lobby
					bool discarded = shouldDiscardNetworkMessage(networkMessageType,connectionSlot);
					if(discarded == false) {

						NetworkMessageReady networkMessageReady;
						if(networkMessageType == nmtReady &&
						   connectionSlot->receiveMessage(&networkMessageReady)) {

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] networkMessageType==nmtReady\n",__FUNCTION__);

							connectionSlot->setReady();
							connectionSlot->setGameStarted(true);
						}
						else if(networkMessageType != nmtInvalid) {
							string sErr = "Unexpected network message: " + intToStr(networkMessageType);
							sendTextMessage(sErr,-1, true,"",index);
							DisplayErrorMessage(sErr);
							logger.setCancelLoading(false);
							return;
						}
					}
					waitingForHosts.push_back(connectionSlot->getName());
					allReady= false;
				}
			}
		}

		//check for timeout
		if(allReady == false) {
			if(chrono.getMillis() > readyWaitTimeout) {
		    	Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
		    	for(unsigned int langIndex = 0; langIndex < languageList.size(); ++langIndex) {
					string sErr = "Timeout waiting for clients.";
					if(lang.hasString("TimeoutWaitingForClients") == true) {
						sErr = lang.getString("TimeoutWaitingForClients",languageList[langIndex]);
					}
					bool localEcho = lang.isLanguageLocal(languageList[langIndex]);
					sendTextMessage(sErr,-1, localEcho, languageList[langIndex]);
					if(localEcho == true) {
						DisplayErrorMessage(sErr);
					}
		    	}
				logger.setCancelLoading(false);
				return;
			}
			else {
				if(chrono.getMillis() - lastStatusUpdate > 200) {
					lastStatusUpdate = chrono.getMillis();

					string waitForHosts = "";
					for(int hostIndex = 0; hostIndex < (int)waitingForHosts.size(); hostIndex++) {
						if(waitForHosts != "") {
							waitForHosts += ", ";
						}
						waitForHosts += waitingForHosts[hostIndex];
					}

					char szBuf[8096]="";
					string updateTextFormat = lang.getString("NetworkGameServerLoadStatus");
					if(updateTextFormat == "" ||
						updateTextFormat[0] == '?') {

						updateTextFormat =  "Waiting for network: %lld seconds elapsed (maximum wait time: %d seconds)";
					}
					snprintf(szBuf,8096,updateTextFormat.c_str(),(long long int)(chrono.getMillis() / 1000),int(readyWaitTimeout / 1000));

					char szBuf1[8096]="";
					string statusTextFormat = lang.getString("NetworkGameStatusWaiting");
					if(statusTextFormat == "" ||
						statusTextFormat[0] == '?') {

						statusTextFormat =  "Waiting for players: %s";
					}
					snprintf(szBuf1,8096,statusTextFormat.c_str(),waitForHosts.c_str());

					logger.add(szBuf, true, szBuf1);

					uint32 loadingStatus = nmls_NONE;
					//send ready message after, so clients start delayed
					for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
						MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
						ConnectionSlot *connectionSlot= slots[slotIndex];
						if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
							switch(slotIndex) {
								case 0:
									loadingStatus |= nmls_PLAYER1_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER1_READY;
									}
									break;
								case 1:
									loadingStatus |= nmls_PLAYER2_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER2_READY;
									}
									break;
								case 2:
									loadingStatus |= nmls_PLAYER3_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER3_READY;
									}
									break;
								case 3:
									loadingStatus |= nmls_PLAYER4_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER4_READY;
									}
									break;
								case 4:
									loadingStatus |= nmls_PLAYER5_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER5_READY;
									}
									break;
								case 5:
									loadingStatus |= nmls_PLAYER6_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER6_READY;
									}
									break;
								case 6:
									loadingStatus |= nmls_PLAYER7_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER7_READY;
									}
									break;
								case 7:
									loadingStatus |= nmls_PLAYER8_CONNECTED;
									if(connectionSlot->isReady()) {
										loadingStatus |= nmls_PLAYER8_READY;
									}
									break;
							}
						}
					}

					// send loading status message
					for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
						MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
						ConnectionSlot* connectionSlot= slots[slotIndex];
						if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
							NetworkMessageLoadingStatus networkMessageLoadingStatus(loadingStatus);
							connectionSlot->sendMessage(&networkMessageLoadingStatus);
						}
					}

					sleep(0);
				}
			}
		}

		Shared::Platform::Window::handleEvent();
	}

	if(logger.getCancelLoading() == true) {
    	Lang &lang= Lang::getInstance();
    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
    	for(unsigned int langIndex = 0; langIndex < languageList.size(); ++langIndex) {

			string sErr = lang.getString("GameCancelledByUser",languageList[langIndex]);
			bool localEcho = lang.isLanguageLocal(languageList[langIndex]);
			sendTextMessage(sErr,-1, localEcho,languageList[langIndex]);

			if(localEcho == true) {
				DisplayErrorMessage(sErr);
			}
    	}
		quitGame(true);
		logger.setCancelLoading(false);
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] PART B (telling client we are ready!\n",__FUNCTION__);
	try {
		//send ready message after, so clients start delayed
		for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {

			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
			ConnectionSlot* connectionSlot= slots[slotIndex];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				NetworkMessageReady networkMessageReady(checksum->getSum());
				connectionSlot->sendMessage(&networkMessageReady);
				connectionSlot->setGameStarted(true);
			}
		}

		gameStartTime = time(NULL);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] END\n",__FUNCTION__);
}

void ServerInterface::processBroadCastMessageQueue() {
	MutexSafeWrapper safeMutexSlot(broadcastMessageQueueThreadAccessor,CODE_AT_LINE);
	if(broadcastMessageQueue.empty() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] broadcastMessageQueue.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,broadcastMessageQueue.size());
		for(int index = 0; index < (int)broadcastMessageQueue.size(); ++index) {
			pair<NetworkMessage *,int> &item = broadcastMessageQueue[index];
			if(item.first != NULL) {
				this->broadcastMessage(item.first,item.second);
				delete item.first;
			}
			item.first = NULL;
		}
		broadcastMessageQueue.clear();
	}
}

void ServerInterface::queueBroadcastMessage(NetworkMessage *networkMessage, int excludeSlot) {
	MutexSafeWrapper safeMutexSlot(broadcastMessageQueueThreadAccessor,CODE_AT_LINE);
	pair<NetworkMessage*,int> item;
	item.first 	= networkMessage;
	item.second = excludeSlot;
	broadcastMessageQueue.push_back(item);
}

void ServerInterface::processTextMessageQueue() {
	MutexSafeWrapper safeMutexSlot(textMessageQueueThreadAccessor,CODE_AT_LINE);
	if(textMessageQueue.empty() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] textMessageQueue.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,textMessageQueue.size());
		for(int index = 0; index < (int)textMessageQueue.size(); ++index) {
			TextMessageQueue &item = textMessageQueue[index];
			sendTextMessage(item.text, item.teamIndex, item.echoLocal, item.targetLanguage);
		}
		textMessageQueue.clear();
	}
}

void ServerInterface::queueTextMessage(const string & text, int teamIndex,
		bool echoLocal, string targetLanguage) {
	//printf("Line: %d text [%s]\n",__LINE__,text.c_str());

	MutexSafeWrapper safeMutexSlot(textMessageQueueThreadAccessor,CODE_AT_LINE);
	TextMessageQueue item;
	item.text 			= text;
	item.teamIndex 		= teamIndex;
	item.echoLocal 		= echoLocal;
	item.targetLanguage = targetLanguage;
	textMessageQueue.push_back(item);
}

void ServerInterface::sendTextMessage(const string & text, int teamIndex,
		bool echoLocal,string targetLanguage) {
	sendTextMessage(text, teamIndex, echoLocal, targetLanguage, -1);
}

void ServerInterface::sendTextMessage(const string& text, int teamIndex, bool echoLocal,
		string targetLanguage, int lockedSlotIndex) {
	//printf("Line: %d text [%s] echoLocal = %d\n",__LINE__,text.c_str(),echoLocal);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] text [%s] teamIndex = %d, echoLocal = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,text.c_str(),teamIndex,echoLocal,lockedSlotIndex);

	NetworkMessageText networkMessageText(text, teamIndex, getHumanPlayerIndex(), targetLanguage);
	broadcastMessage(&networkMessageText, -1, lockedSlotIndex);

	if(echoLocal == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		ChatMsgInfo msg(text.c_str(),teamIndex,networkMessageText.getPlayerIndex(), targetLanguage);
		this->addChatInfo(msg);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note,int playerIndex) {
	sendMarkCellMessage(targetPos, factionIndex, note, playerIndex, -1);
}

void ServerInterface::sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note, int playerIndex, int lockedSlotIndex) {
	NetworkMessageMarkCell networkMessageMarkCell(targetPos,factionIndex, note, playerIndex);
	broadcastMessage(&networkMessageMarkCell, -1, lockedSlotIndex);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::sendHighlightCellMessage(Vec2i targetPos, int factionIndex) {
	sendHighlightCellMessage(targetPos, factionIndex, -1);
}

void ServerInterface::sendHighlightCellMessage(Vec2i targetPos, int factionIndex, int lockedSlotIndex) {
	NetworkMessageHighlightCell networkMessageHighlightCell(targetPos,factionIndex);
	broadcastMessage(&networkMessageHighlightCell, -1, lockedSlotIndex);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::sendUnMarkCellMessage(Vec2i targetPos, int factionIndex) {
	sendUnMarkCellMessage(targetPos, factionIndex, -1);
}

void ServerInterface::sendUnMarkCellMessage(Vec2i targetPos, int factionIndex, int lockedSlotIndex) {
	NetworkMessageUnMarkCell networkMessageMarkCell(targetPos,factionIndex);
	broadcastMessage(&networkMessageMarkCell, -1, lockedSlotIndex);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::quitGame(bool userManuallyQuit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

string ServerInterface::getNetworkStatus() {
	Lang &lang = Lang::getInstance();
	string str = "";
	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot *connectionSlot= slots[index];

		str += intToStr(index)+ ": ";

		if(connectionSlot!= NULL) {
			if(connectionSlot->isConnected()) {
				int clientLagCount 					= connectionSlot->getCurrentLagCount();
				double lastClientCommandListTimeLag = difftime((long int)time(NULL),connectionSlot->getLastReceiveCommandListTime());
				//float pingTime = connectionSlot->getThreadedPingMS(connectionSlot->getIpAddress().c_str());
				char szBuf[8096]="";
				snprintf(szBuf,8096,", lag = %d [%.2f]",clientLagCount,lastClientCommandListTimeLag);
				str += connectionSlot->getName() + " [" + connectionSlot->getUUID() + "] " + string(szBuf);
			}
		}
		else {
			str += lang.getString("NotConnected");
		}

		str += '\n';
	}
	return str;
}

bool ServerInterface::launchGame(const GameSettings *gameSettings) {
	bool bOkToStart = true;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	for(int index = 0; exitServer == false && index < GameConstants::maxPlayers; ++index) {

		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
		ConnectionSlot *connectionSlot= slots[index];
		if(connectionSlot != NULL &&
		   (connectionSlot->getAllowDownloadDataSynch() == true || connectionSlot->getAllowGameDataSynchCheck() == true) &&
		   connectionSlot->isConnected()) {

			if(connectionSlot->getNetworkGameDataSynchCheckOk() == false) {

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] map [%d] tile [%d] techtree [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,connectionSlot->getNetworkGameDataSynchCheckOkMap(),connectionSlot->getNetworkGameDataSynchCheckOkTile(),connectionSlot->getNetworkGameDataSynchCheckOkTech());
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] map [%d] tile [%d] techtree [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,connectionSlot->getNetworkGameDataSynchCheckOkMap(),connectionSlot->getNetworkGameDataSynchCheckOkTile(),connectionSlot->getNetworkGameDataSynchCheckOkTech());

				bOkToStart = false;
				break;
			}
		}
	}
	if(bOkToStart == true) {

		bool useInGameBlockingClientSockets = Config::getInstance().getBool("EnableInGameBlockingSockets","true");
		if(useInGameBlockingClientSockets == true) {

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			for(int index = 0; index < GameConstants::maxPlayers; ++index) {

				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(index));
				ConnectionSlot *connectionSlot= slots[index];
				if(connectionSlot != NULL && connectionSlot->isConnected()) {
					connectionSlot->getSocket()->setBlock(true);
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		bool requiresUPNPTrigger = false;
		for(int startIndex = 0; startIndex < GameConstants::maxPlayers; ++startIndex) {

			int factionIndex = gameSettings->getFactionIndexForStartLocation(startIndex);
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[startIndex],CODE_AT_LINE_X(startIndex));
			ConnectionSlot *connectionSlot= slots[startIndex];
			if((connectionSlot == NULL || connectionSlot->isConnected() == false) &&
				this->getAllowInGameConnections() == true) {

				// Open slots for joining in progress game
				if(gameSettings->getFactionControl(factionIndex) != ctClosed &&
					gameSettings->getFactionControl(factionIndex) != ctHuman) {

					//printf("Opening slot for in game connections for slot: %d, faction: %d\n",i,factionIndex);
					if(connectionSlot == NULL) {
						addSlot(startIndex);
						connectionSlot 		= slots[startIndex];
						requiresUPNPTrigger = true;
					}
					connectionSlot->setCanAcceptConnections(true);
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		if(this->getAllowInGameConnections() == false) {
			serverSocket.stopBroadCastThread();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		this->gameSettings = *gameSettings;
		//printf("#1 Data synch: lmap %u ltile: %d ltech: %u\n",gameSettings->getMapCRC(),gameSettings->getTilesetCRC(),gameSettings->getTechCRC());

		NetworkMessageLaunch networkMessageLaunch(gameSettings,nmtLaunch);
		broadcastMessage(&networkMessageLaunch);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		shutdownMasterserverPublishThread();
		MutexSafeWrapper safeMutex(masterServerThreadAccessor,CODE_AT_LINE);
		lastMasterserverHeartbeatTime = 0;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ftpServer = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ftpServer);

		if(this->getAllowInGameConnections() == false) {
			shutdownFTPServer();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		if(publishToMasterserverThread == NULL) {
			if(needToRepublishToMasterserver == true ||
				GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {

				static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
				publishToMasterserverThread = new SimpleTaskThread(this,0,125);
				publishToMasterserverThread->setUniqueID(mutexOwnerId);
				publishToMasterserverThread->start();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);
			}
		}

		if(this->getAllowInGameConnections() == false) {
			shutdownFTPServer();
		}

		if(requiresUPNPTrigger == true) {
            this->getServerSocket()->NETdiscoverUPnPDevices();
        }

		gameLaunched = true;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	return bOkToStart;
}

void ServerInterface::shutdownFTPServer() {
	if(ftpServer != NULL) {
		ftpServer->shutdownAndWait();
		delete ftpServer;
		ftpServer = NULL;
	}
}

void ServerInterface::checkListenerSlots() {
	if(gameLaunched == true &&
		this->getAllowInGameConnections() == true) {

		if(difftime((long int)time(NULL),lastListenerSlotCheckTime) >= 7) {

			lastListenerSlotCheckTime 			= time(NULL);
			bool useInGameBlockingClientSockets = Config::getInstance().getBool("EnableInGameBlockingSockets","true");

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			for(int startIndex = 0; startIndex < GameConstants::maxPlayers; ++startIndex) {

				int factionIndex = gameSettings.getFactionIndexForStartLocation(startIndex);
				if(gameSettings.getFactionControl(factionIndex) != ctClosed &&
					gameSettings.getFactionControl(factionIndex) != ctHuman) {

					MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[startIndex],CODE_AT_LINE_X(startIndex));
					ConnectionSlot *connectionSlot= slots[startIndex];
					// Open slots for joining in progress game
					if(connectionSlot == NULL) {
						printf("Opening slot for in game connections, slot: %d, factionindex: %d name: %s\n",startIndex,factionIndex,gameSettings.getFactionTypeName(factionIndex).c_str());

						addSlot(startIndex);
						connectionSlot = slots[startIndex];
						if(useInGameBlockingClientSockets == true) {
							connectionSlot->getSocket()->setBlock(true);
						}
						connectionSlot->setCanAcceptConnections(true);
					}
					else if(connectionSlot != NULL &&
							connectionSlot->getCanAcceptConnections() == false &&
							connectionSlot->isConnected() == false) {
						printf("Removing slot for in game connections, slot: %d, factionindex: %d name: %s\n",startIndex,factionIndex,gameSettings.getFactionTypeName(factionIndex).c_str());

						this->removeSlot(startIndex);
					}
				}
			}
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,needToRepublishToMasterserver);
}

void ServerInterface::broadcastGameSetup(GameSettings *gameSettingsBuffer, bool setGameSettingsBuffer) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(gameSettingsBuffer == NULL) {
		throw megaglest_runtime_error("gameSettingsBuffer == NULL");
	}
	for(unsigned int factionIndex = 0; factionIndex < (unsigned int)gameSettingsBuffer->getFactionCount(); ++factionIndex) {

		int slotIndex = gameSettingsBuffer->getStartLocationIndex(factionIndex);
		if(gameSettingsBuffer->getFactionControl(factionIndex) == ctNetwork &&
			isClientConnected(slotIndex) == false) {

			gameSettingsBuffer->setNetworkPlayerName(factionIndex,GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME);
		}
	}
	if(setGameSettingsBuffer == true) {
		validateGameSettings(gameSettingsBuffer);
	}

	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(setGameSettingsBuffer == true) {
		gameSettings = *gameSettingsBuffer;
		gameSettingsUpdateCount++;
	}

	NetworkMessageLaunch networkMessageLaunch(gameSettingsBuffer, nmtBroadCastSetup);
	broadcastMessage(&networkMessageLaunch);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::broadcastMessage(NetworkMessage *networkMessage, int excludeSlot, int lockedSlotIndex) {
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	    MutexSafeWrapper safeMutexSlotBroadCastAccessor(inBroadcastMessageThreadAccessor,CODE_AT_LINE);
	    if(inBroadcastMessage == true &&
	    	dynamic_cast<NetworkMessageText *>(networkMessage) != NULL) {

	    	safeMutexSlotBroadCastAccessor.ReleaseLock();
	    	NetworkMessageText *txtMsg = dynamic_cast<NetworkMessageText *>(networkMessage);
	    	if(txtMsg != NULL) {
	    		NetworkMessageText *msgCopy = txtMsg->getCopy();
	    		queueBroadcastMessage(msgCopy, excludeSlot);
	    	}
	    	return;
	    }
	    else {
			inBroadcastMessage = true;
			safeMutexSlotBroadCastAccessor.ReleaseLock(true);
	    }

		for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
			MutexSafeWrapper safeMutexSlot(NULL,CODE_AT_LINE_X(slotIndex));
			if(slotIndex != lockedSlotIndex) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] i = %d, lockedSlotIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slotIndex,lockedSlotIndex);
				safeMutexSlot.setMutex(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
			}

			ConnectionSlot* connectionSlot= slots[slotIndex];

			if(slotIndex != excludeSlot && connectionSlot != NULL) {
				if(connectionSlot->isConnected()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] before sendMessage\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					connectionSlot->sendMessage(networkMessage);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after sendMessage\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				}
				if(gameHasBeenInitiated == true && connectionSlot->isConnected() == false) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 before removeSlot for slot# %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slotIndex);

					if(this->getAllowInGameConnections() == false) {
						removeSlot(slotIndex,slotIndex);
					}
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 after removeSlot for slot# %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slotIndex);
				}
			}
			else if(slotIndex == excludeSlot && gameHasBeenInitiated == true &&
					connectionSlot != NULL && connectionSlot->isConnected() == false) {

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 before removeSlot for slot# %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slotIndex);

				if(this->getAllowInGameConnections() == false) {
					removeSlot(slotIndex,slotIndex);
				}
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 after removeSlot for slot# %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,slotIndex);
			}
		}

		safeMutexSlotBroadCastAccessor.Lock();

	    inBroadcastMessage = false;

	    safeMutexSlotBroadCastAccessor.ReleaseLock();
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		MutexSafeWrapper safeMutexSlotBroadCastAccessor(inBroadcastMessageThreadAccessor,CODE_AT_LINE);
	    inBroadcastMessage = false;
	    safeMutexSlotBroadCastAccessor.ReleaseLock();

		string sMsg = ex.what();
		sendTextMessage(sMsg,-1, true, "", lockedSlotIndex);
	}
}

void ServerInterface::broadcastMessageToConnectedClients(NetworkMessage *networkMessage, int excludeSlot) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	try {
		for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
			ConnectionSlot *connectionSlot= slots[slotIndex];

			if(slotIndex != excludeSlot && connectionSlot != NULL) {
				if(connectionSlot->isConnected()) {
					connectionSlot->sendMessage(networkMessage);
				}
			}
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ServerInterface::updateListen() {
	if(gameHasBeenInitiated == true &&
		this->getAllowInGameConnections() == false) {
		return;
	}

	int openSlotCount = 0;
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex)	{

		bool isSlotOpen = (slots[slotIndex] != NULL && slots[slotIndex]->isConnected() == false);
		if(isSlotOpen == true) {
			++openSlotCount;
		}
	}

	serverSocket.listen(openSlotCount);
}

int ServerInterface::getOpenSlotCount() {
	int openSlotCount = 0;
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex)	{

		bool isSlotOpen = (slots[slotIndex] != NULL && slots[slotIndex]->isConnected() == false);
		if(isSlotOpen == true) {
			++openSlotCount;
		}
	}
	return openSlotCount;
}

int ServerInterface::getGameSettingsUpdateCount() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START gameSettingsUpdateCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,gameSettingsUpdateCount);

	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	int result = gameSettingsUpdateCount;
	safeMutex.ReleaseLock();
	return result;
}

void ServerInterface::validateGameSettings(GameSettings *serverGameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	string mapFile = serverGameSettings->getMap();
	printf("Trying to set map to [%s]. Current map is [%s]\n",serverGameSettings->getMap().c_str(),gameSettings.getMap().c_str());
	if(find(mapFiles.begin(),mapFiles.end(),mapFile) == mapFiles.end()) {

		printf("map not found on this server\n");
		int currentIndex	= -1;
		string currentMap	= gameSettings.getMap();
		for (int mapIndex = 0 ;mapIndex < (int)mapFiles.size(); ++mapIndex) {
			string current = mapFiles[mapIndex];
			if(current == currentMap) {
			 	currentIndex = mapIndex;
			 	break;
			}
		}
		if(currentIndex == -1) {
			serverGameSettings->setMap(gameSettings.getMap());
		}
		else {
			if(mapFile>gameSettings.getMap()){
				printf("mapFile>gameSettings [%s] > [%s]\n",mapFile.c_str(),gameSettings.getMap().c_str());

				int nextIndex = -1;
				for (int mapIndex = 0 ;mapIndex < (int)mapFiles.size(); ++mapIndex) {
					string current = mapFiles[mapIndex];
					if(current > mapFile) {
						nextIndex = mapIndex;
					 	break;
					}
				}
				if(nextIndex > -1) {
					serverGameSettings->setMap(mapFiles[nextIndex]);
					//printf("switch up\n");
				}
				else {
					//printf("overflow top\n");
					serverGameSettings->setMap(mapFiles[0]);
				}
			}
			else {
				printf("mapFile<gameSettings [%s] < [%s]\n",mapFile.c_str(),gameSettings.getMap().c_str());

				int nextIndex = -1;
				for (int mapIndex = (int)mapFiles.size()-1 ;mapIndex >= 0; mapIndex--) {
					string current = mapFiles[mapIndex];
					if(current < mapFile) {
						nextIndex = mapIndex;
						break;
					}
				}

				if(nextIndex > -1) {
					serverGameSettings->setMap(mapFiles[nextIndex]);
					//printf("switch down\n");
				}
				else {
					//printf("overflow bottom\n");
					serverGameSettings->setMap(mapFiles[(mapFiles.size()-1)]);
				}
			}
			printf("switching map from [%s] to [%s]\n",mapFile.c_str(),serverGameSettings->getMap().c_str());
		}
		serverGameSettings->setMapFilterIndex(gameSettings.getMapFilterIndex());

		Checksum checksum;
		string file = Config::getMapPath(serverGameSettings->getMap(),"",false);
		checksum.addFile(file);
		serverGameSettings->setMapCRC(checksum.getSum());
	}

	string tilesetFile = serverGameSettings->getTileset();
	if(find(tilesetFiles.begin(),tilesetFiles.end(),tilesetFile) == tilesetFiles.end()) {
		printf("Reverting tileset from [%s] to [%s]\n",serverGameSettings->getTileset().c_str(),gameSettings.getTileset().c_str());

		serverGameSettings->setTileset(gameSettings.getTileset());
		serverGameSettings->setTilesetCRC(gameSettings.getTilesetCRC());
	}

	string techtreeFile = serverGameSettings->getTech();
	if(find(techTreeFiles.begin(),techTreeFiles.end(),techtreeFile) == techTreeFiles.end()) {
		printf("Reverting tech from [%s] to [%s]\n",serverGameSettings->getTech().c_str(),gameSettings.getTech().c_str());

		serverGameSettings->setTech(gameSettings.getTech());
		serverGameSettings->setTechCRC(gameSettings.getTechCRC());
	}
}

void ServerInterface::setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck) {
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START gameSettingsUpdateCount = %d, waitForClientAck = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,gameSettingsUpdateCount,waitForClientAck);

	if(serverGameSettings->getScenario() == "") {
		string mapFile = serverGameSettings->getMap();
		if(find(mapFiles.begin(),mapFiles.end(),mapFile) == mapFiles.end()) {
			printf("Reverting map from [%s] to [%s]\n",serverGameSettings->getMap().c_str(),gameSettings.getMap().c_str());
	
			serverGameSettings->setMapFilterIndex(gameSettings.getMapFilterIndex());
			serverGameSettings->setMap(gameSettings.getMap());
			serverGameSettings->setMapCRC(gameSettings.getMapCRC());
		}
	
		string tilesetFile = serverGameSettings->getTileset();
		if(find(tilesetFiles.begin(),tilesetFiles.end(),tilesetFile) == tilesetFiles.end()) {
			printf("Reverting tileset from [%s] to [%s]\n",serverGameSettings->getTileset().c_str(),gameSettings.getTileset().c_str());
	
			serverGameSettings->setTileset(gameSettings.getTileset());
			serverGameSettings->setTilesetCRC(gameSettings.getTilesetCRC());
		}
	
		string techtreeFile = serverGameSettings->getTech();
		if(find(techTreeFiles.begin(),techTreeFiles.end(),techtreeFile) == techTreeFiles.end()) {
			printf("Reverting tech from [%s] to [%s]\n",serverGameSettings->getTech().c_str(),gameSettings.getTech().c_str());
	
			serverGameSettings->setTech(gameSettings.getTech());
			serverGameSettings->setTechCRC(gameSettings.getTechCRC());
		}
	}

	gameSettings = *serverGameSettings;

	if(getAllowGameDataSynchCheck() == true) {
		if(waitForClientAck == true && gameSettingsUpdateCount > 0) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Waiting for client acks #1\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

			time_t tStart = time(NULL);
			bool gotAckFromAllClients = false;
			while(gotAckFromAllClients == false && difftime((long int)time(NULL),tStart) <= 5) {
				gotAckFromAllClients = true;
				for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
					//printf("===> START slot %d - About to setGameSettings #1\n",i);

					MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
					ConnectionSlot *connectionSlot = slots[slotIndex];
					if(connectionSlot != NULL && connectionSlot->isConnected()) {
						if(connectionSlot->getReceivedNetworkGameStatus() == false) {
							gotAckFromAllClients = false;
						}

						connectionSlot->update(true,slotIndex);
					}
					//printf("===> END slot %d - About to setGameSettings #1\n",i);
				}
			}
		}

		for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
			ConnectionSlot *connectionSlot = slots[slotIndex];
			if(connectionSlot != NULL && connectionSlot->isConnected()) {
				connectionSlot->setReceivedNetworkGameStatus(false);
			}
		}

		NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(getGameSettings());
		broadcastMessageToConnectedClients(&networkMessageSynchNetworkGameData);

		if(waitForClientAck == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Waiting for client acks #2\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

			time_t tStart = time(NULL);
			bool gotAckFromAllClients = false;
			while(gotAckFromAllClients == false && difftime((long int)time(NULL),tStart) <= 5) {
				gotAckFromAllClients = true;
				for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
					//printf("===> START slot %d - About to setGameSettings 2\n",slotIndex);

					MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
					ConnectionSlot *connectionSlot = slots[slotIndex];
					if(connectionSlot != NULL && connectionSlot->isConnected()) {
						if(connectionSlot->getReceivedNetworkGameStatus() == false) {
							gotAckFromAllClients = false;
						}

						connectionSlot->update(true,slotIndex);
					}
					//printf("===> END slot %d - About to setGameSettings 2\n",slotIndex);
				}
			}
		}

	}
	gameSettingsUpdateCount++;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);
}

void ServerInterface::close() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);
}

string ServerInterface::getHumanPlayerName(int index) {
	string result = Config::getInstance().getString("NetPlayerName", Socket::getHostName().c_str());
	if(index >= 0 || gameSettings.getThisFactionIndex() >= 0) {
		if(index < 0) {
			index = gameSettings.getThisFactionIndex();
		}
		if(gameSettings.getNetworkPlayerName(index) != "") {
			result = gameSettings.getNetworkPlayerName(index);
		}
	}

	return result;
}

int ServerInterface::getHumanPlayerIndex() const {
	return gameSettings.getStartLocationIndex(gameSettings.getThisFactionIndex());
}

std::map<string,string> ServerInterface::publishToMasterserver() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	int slotCountUsed = 1;
	int slotCountHumans = 1;
	int slotCountConnectedPlayers = 1;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		slotCountUsed = 0;
		slotCountHumans = 0;
		slotCountConnectedPlayers = 0;
	}

	Config & config = Config::getInstance();
	std::map < string, string > publishToServerInfo;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
		if(slots[slotIndex] != NULL) {
			slotCountUsed++;
			slotCountHumans++;
			ConnectionSlot* connectionSlot= slots[slotIndex];
			if((connectionSlot!=NULL) && (connectionSlot->isConnected())) {
				slotCountConnectedPlayers++;
			}
		}
	}
	publishToServerInfo["uuid"] 				= Config::getInstance().getString("PlayerId","");
	publishToServerInfo["glestVersion"] 		= glestVersionString;
	publishToServerInfo["platform"] 			= getPlatformNameString() + "-" + getGITRevisionString();
	publishToServerInfo["binaryCompileDate"] 	= getCompileDateTime();
	publishToServerInfo["serverTitle"] 			= getHumanPlayerName() + "'s game";
	publishToServerInfo["tech"] 				= this->getGameSettings()->getTech();
	publishToServerInfo["map"] 					= this->getGameSettings()->getMap();
	publishToServerInfo["tileset"] 				= this->getGameSettings()->getTileset();

	bool updateSlots = true;
	MutexSafeWrapper safeMutex2(gameStatsThreadAccessor,CODE_AT_LINE);
	if(gameStats != NULL) {
		for(int factionIndex = 0; factionIndex < gameStats->getFactionCount(); ++factionIndex) {
			if(gameStats->getVictory(factionIndex) == true) {
				updateSlots = false;
				break;
			}
		}
	}
	safeMutex2.ReleaseLock();

	if(updateSlots == true) {
		publishToServerInfo["activeSlots"] 		= intToStr(slotCountUsed);
		publishToServerInfo["networkSlots"] 	= intToStr(slotCountHumans);
		publishToServerInfo["connectedClients"] = intToStr(slotCountConnectedPlayers);
	}

	string serverPort							= config.getString("PortServer", intToStr(GameConstants::serverPort).c_str());
	string externalPort							= config.getString("PortExternal", serverPort.c_str());
	publishToServerInfo["externalconnectport"] 	= externalPort;
	publishToServerInfo["privacyPlease"] 		= intToStr(config.getBool("PrivacyPlease","false"));
	publishToServerInfo["gameStatus"] 			= intToStr(game_status_in_progress);

	if(publishToMasterserverThread == NULL) {
		publishToServerInfo["gameCmd"]		= "gameOver";
		publishToServerInfo["gameStatus"] 	= intToStr(game_status_finished);
	}

	//printf("Host game id = %s\n",this->getGameSettings()->getGameUUID().c_str());
	publishToServerInfo["gameUUID"] = this->getGameSettings()->getGameUUID();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	return publishToServerInfo;
}

std::map<string,string> ServerInterface::publishToMasterserverStats() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	MutexSafeWrapper safeMutex(gameStatsThreadAccessor,CODE_AT_LINE);
	std::map < string, string > publishToServerInfo;
	if(gameStats != NULL) {
		publishToServerInfo["gameUUID"] 						= this->getGameSettings()->getGameUUID();
		publishToServerInfo["tech"] 							= this->getGameSettings()->getTech();
		publishToServerInfo["factionCount"] 					= intToStr(gameStats->getFactionCount());
		publishToServerInfo["framesPlayed"] 					= intToStr(gameStats->getFramesPlayed());
		publishToServerInfo["framesToCalculatePlaytime"] 		= intToStr(gameStats->getFramesToCalculatePlaytime());
		publishToServerInfo["maxConcurrentUnitCount"] 			= intToStr(gameStats->getMaxConcurrentUnitCount());
		publishToServerInfo["totalEndGameConcurrentUnitCount"] 	= intToStr(gameStats->getTotalEndGameConcurrentUnitCount());
		publishToServerInfo["isHeadlessServer"] 				= intToStr(gameStats->getIsMasterserverMode());

		for(int factionIndex = 0; factionIndex < gameStats->getFactionCount(); ++factionIndex) {
			publishToServerInfo["factionIndex_" + intToStr(factionIndex)] 		= intToStr(factionIndex);
			publishToServerInfo["controlType_" + intToStr(factionIndex)] 		= intToStr(gameStats->getControl(factionIndex));
			publishToServerInfo["resourceMultiplier_" + intToStr(factionIndex)] = floatToStr(gameStats->getResourceMultiplier(factionIndex));
			publishToServerInfo["factionTypeName_" + intToStr(factionIndex)] 	= gameStats->getFactionTypeName(factionIndex);
			publishToServerInfo["personalityType_" + intToStr(factionIndex)] 	= intToStr(gameStats->getPersonalityType(factionIndex));
			publishToServerInfo["teamIndex_" + intToStr(factionIndex)] 			= intToStr(gameStats->getTeam(factionIndex));
			publishToServerInfo["wonGame_" + intToStr(factionIndex)] 			= intToStr(gameStats->getVictory(factionIndex));
			publishToServerInfo["killCount_" + intToStr(factionIndex)] 			= intToStr(gameStats->getKills(factionIndex));
			publishToServerInfo["enemyKillCount_" + intToStr(factionIndex)] 	= intToStr(gameStats->getEnemyKills(factionIndex));
			publishToServerInfo["deathCount_" + intToStr(factionIndex)] 		= intToStr(gameStats->getDeaths(factionIndex));
			publishToServerInfo["unitsProducedCount_" + intToStr(factionIndex)] = intToStr(gameStats->getUnitsProduced(factionIndex));
			publishToServerInfo["resourceHarvestedCount_" + intToStr(factionIndex)] = intToStr(gameStats->getResourcesHarvested(factionIndex));
			publishToServerInfo["playerName_" + intToStr(factionIndex)] 		= gameStats->getPlayerName(factionIndex);
			publishToServerInfo["quitBeforeGameEnd_" + intToStr(factionIndex)] 	= intToStr(gameStats->getPlayerLeftBeforeEnd(factionIndex));
			publishToServerInfo["quitTime_" + intToStr(factionIndex)] 			= intToStr(gameStats->getTimePlayerLeft(factionIndex));
			publishToServerInfo["playerUUID_" + intToStr(factionIndex)] 	    = this->getGameSettings()->getNetworkPlayerUUID(factionIndex);
			publishToServerInfo["platform_" + intToStr(factionIndex)] 	    = this->getGameSettings()->getNetworkPlayerPlatform(factionIndex);
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
	return publishToServerInfo;
}

void ServerInterface::setGameStats(Stats *stats) {
	if(stats == NULL) {
		return;
	}
	MutexSafeWrapper safeMutex(gameStatsThreadAccessor,CODE_AT_LINE);
	if(gameStats == NULL) {
		gameStats = new Stats();
	}
	*gameStats = *stats;
}

void ServerInterface::simpleTask(BaseThread *callingThread,void *userdata) {
	MutexSafeWrapper safeMutex(masterServerThreadAccessor,CODE_AT_LINE);

	if(difftime((long int)time(NULL),lastMasterserverHeartbeatTime) >= MASTERSERVER_HEARTBEAT_GAME_STATUS_SECONDS) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking to see masterserver needs an update of the game status [%d] callingThread [%p] publishToMasterserverThread [%p]\n",needToRepublishToMasterserver,callingThread,publishToMasterserverThread);

		lastMasterserverHeartbeatTime = time(NULL);
		if(needToRepublishToMasterserver == true) {
			try {
				if(Config::getInstance().getString("Masterserver","") != "") {
					string request = Config::getInstance().getString("Masterserver") + "addServerInfo.php?";

					std::map<string,string> newPublishToServerInfo = publishToMasterserver();

					CURL *handle = SystemFlags::initHTTP();
					for(std::map<string,string>::const_iterator iterMap = newPublishToServerInfo.begin();
						iterMap != newPublishToServerInfo.end(); ++iterMap) {

						request += iterMap->first;
						request += "=";
						request += SystemFlags::escapeURL(iterMap->second,handle);
						request += "&";
					}

					//printf("The Host request is:\n%s\n",request.c_str());
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("The Host request is:\n%s\n",request.c_str());

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d] the request is:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,request.c_str());

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Calling masterserver [%s]...\n",request.c_str());

					std::string serverInfo = SystemFlags::getHTTP(request,handle);
					//printf("Result:\n%s\n",serverInfo .c_str());

					string requestStats = Config::getInstance().getString("Masterserver") + "addGameStats.php?";
					std::map<string,string> newPublishToServerInfoStats = publishToMasterserverStats();
					if(newPublishToServerInfoStats.empty() == false) {
						for(std::map<string,string>::const_iterator iterMap = newPublishToServerInfoStats.begin();
							iterMap != newPublishToServerInfoStats.end(); ++iterMap) {

							requestStats += iterMap->first;
							requestStats += "=";
							requestStats += SystemFlags::escapeURL(iterMap->second,handle);
							requestStats += "&";
						}

						//printf("The Host stats request is:\n%s\n",requestStats.c_str());
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("The Host request is:\n%s\n",requestStats.c_str());
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d] the request is:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,requestStats.c_str());
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Calling masterserver [%s]...\n",requestStats.c_str());

						std::string serverInfoStats = SystemFlags::getHTTP(requestStats,handle);
						//printf("Result:\n%s\n",serverInfoStats .c_str());

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d] the result is:\n'%s'\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,serverInfoStats.c_str());
					}

					SystemFlags::cleanupHTTP(&handle);

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Done Calling masterserver\n");

					//printf("the result is:\n'%s'\n",serverInfo.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d] the result is:\n'%s'\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,serverInfo.c_str());
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error, no masterserver defined!\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error during game status update: [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
			}
		}
		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
			DumpStatsToLog(false);
		}
	}
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		//printf("Attempt Accept\n");
		if(serverSocketAdmin != NULL) {
			Socket *cli = serverSocketAdmin->accept(false);
			if(cli != NULL) {
				printf("Got status request connection, dumping info...\n");

				string data = DumpStatsToLog(true);
				cli->send(data.c_str(),(int)data.length());
				cli->disconnectSocket();
			}
		}
	}
}

std::string ServerInterface::DumpStatsToLog(bool dumpToStringOnly) const {
	string headlessLogFile = Config::getInstance().getString("HeadlessLogFile","headless.log");
	if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
		headlessLogFile  = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + headlessLogFile ;
	}
	else {
        string userData = Config::getInstance().getString("UserData_Root","");
        if(userData != "") {
        	endPathWithSlash(userData);
        }
        headlessLogFile  = userData + headlessLogFile ;
	}

	ostringstream out;
	out << "========================================="  << std::endl;
	out << "Headless Server Current Game information:"  << std::endl;
	out << "========================================="  << std::endl;

	int connectedSlotCount = 0;
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {

		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
		ConnectionSlot *slot = slots[slotIndex];
		if(slot != NULL) {

			connectedSlotCount++;
			out << "Network connection for index: " << slotIndex << std::endl;
			out << "------------------------------" << std::endl;
			out << "Connected: " << boolToStr(slot->isConnected()) << std::endl;
			out << "Handshake received: " << boolToStr(slot->getConnectHasHandshaked()) << std::endl;
			if(slot->isConnected() == true) {

				time_t connectTime 	= slot->getConnectedTime();
				struct tm *loctime 	= localtime (&connectTime);
				char szBuf[8096]	= "";
				strftime(szBuf,100,"%Y-%m-%d %H:%M:%S",loctime);

				const int HOURS_IN_DAY 		= 24;
				const int MINUTES_IN_HOUR 	= 60;
				const int SECONDS_IN_MINUTE = 60;
				int InSeconds 				= difftime((long int)time(NULL),slot->getConnectedTime());
				// compute seconds
				int seconds = InSeconds % SECONDS_IN_MINUTE ;
				// throw away seconds used in previous statement and convert to minutes
				int InMinutes = InSeconds / SECONDS_IN_MINUTE ;
				// compute  minutes
				int minutes = InMinutes % MINUTES_IN_HOUR ;

				// throw away minutes used in previous statement and convert to hours
				int InHours = InMinutes / MINUTES_IN_HOUR ;
				// compute hours
				int hours = InHours % HOURS_IN_DAY ;

				out << "Connected at: " 		<< szBuf 					<< std::endl;
				out << "Connection duration: " 	<< hours << " hours " << minutes << " minutes " << seconds << " seconds." << std::endl;
				out << "Player Index: " 		<< slot->getPlayerIndex() 	<< std::endl;
				out << "IP Address: " 			<< slot->getIpAddress() 	<< std::endl;
				out << "Player name: " 			<< slot->getName() 			<< std::endl;
				out << "Player uuid: " 			<< slot->getUUID() 			<< std::endl;
				out << "Language: " 			<< slot->getNetworkPlayerLanguage() << std::endl;
				out << "Game Version: " 		<< slot->getVersionString() << std::endl;
				out << "Session id: " 			<< slot->getSessionKey() 	<< std::endl;
				out << "Socket id: " 			<< slot->getSocketId() 		<< std::endl;
			}
		}
	}
	out << "Total Slot Count: " << connectedSlotCount 	<< std::endl;
	out << "========================================="  << std::endl;

	std::string result = out.str();

	if(dumpToStringOnly == false) {

#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(headlessLogFile ).c_str(), L"w");
		std::ofstream logFile(fp);
#else
		std::ofstream logFile;
		logFile.open(headlessLogFile .c_str(), ios_base::out | ios_base::trunc);
#endif
		logFile << result;
		logFile.close();
#if defined(WIN32) && !defined(__MINGW32__)
		if(fp) {
			fclose(fp);
		}
#endif
	}

    return result;
}

void ServerInterface::notifyBadClientConnectAttempt(string ipAddress) {
	//printf("In [%s::%s Line: %d] ipAddress [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ipAddress.c_str());

	if(badClientConnectIPList.find(ipAddress) == badClientConnectIPList.end()) {
		badClientConnectIPList[ipAddress] = make_pair(0,(long int)time(NULL));
	}

	pair<uint64,time_t> &lastBadConnectionAttempt = badClientConnectIPList[ipAddress];

	const uint64 BLOCK_BAD_CLIENT_CONNECT_MAX_SECONDS 	= Config::getInstance().getInt("BlockBadClientConnectMaxSeconds", "60");
	const uint64 BLOCK_BAD_CLIENT_CONNECT_MAX_ATTEMPTS 	= Config::getInstance().getInt("BlockBadClientConnectMaxAttempts", "6");
	bool addToBlockedClientsList 						= false;

	if(difftime((long int)time(NULL),lastBadConnectionAttempt.second) <= BLOCK_BAD_CLIENT_CONNECT_MAX_SECONDS) {

		if(lastBadConnectionAttempt.first+1 > BLOCK_BAD_CLIENT_CONNECT_MAX_ATTEMPTS) {
			addToBlockedClientsList = true;
		}
	}
	else {
		// Reset after x seconds
		lastBadConnectionAttempt.first = 0;
	}

	if(this->getAllowInGameConnections() == true) {
		printf("notifyBadClientConnectAttempt() #1: %s!\n",ipAddress.c_str());
	}

	if(addToBlockedClientsList == true) {
		serverSocket.addIPAddressToBlockedList(ipAddress);
	}

	lastBadConnectionAttempt.first++;
	lastBadConnectionAttempt.second = time(NULL);
}

bool ServerInterface::getStartInGameConnectionLaunch() {
	bool result = false;
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
		MutexSafeWrapper safeMutex(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
		if(slots[slotIndex] != NULL) {

			ConnectionSlot *slot = slots[slotIndex];
			if(slot->getStartInGameConnectionLaunch() == true) {
				result = true;
				break;
			}
		}
	}
	return result;
}

bool ServerInterface::getPauseForInGameConnection() {
	bool result = false;
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
		MutexSafeWrapper safeMutex(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
		if(slots[slotIndex] != NULL) {

			ConnectionSlot *slot = slots[slotIndex];
			if(slot->getPauseForInGameConnection() == true) {
				result = true;
				break;
			}
		}
	}
	return result;
}

bool ServerInterface::getUnPauseForInGameConnection() {
	bool result = false;
	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
		MutexSafeWrapper safeMutex(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
		if(slots[slotIndex] != NULL) {

			ConnectionSlot *slot = slots[slotIndex];
			if(slot->isConnected() == true) {
				if(slot->isReady() == true) {
					result = true;
					if(slot->getUnPauseForInGameConnection() == false) {
						result = false;
						break;
					}
				}
				else {
					result = false;
					break;
				}
			}
		}
	}
	if(result == true) {
		resumeGameStartTime = time(NULL);
	}
	return result;
}

ConnectionSlot * ServerInterface::findSlotForUUID(string uuid, bool unConnectedOnly) {
	ConnectionSlot *result = NULL;
	if(uuid != "") {
		for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {

			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
			ConnectionSlot *connectionSlot= slots[slotIndex];

			if(connectionSlot != NULL) {
				if(connectionSlot->getUUID() == uuid) {
					if(unConnectedOnly == false ||
						(unConnectedOnly == true && connectionSlot->isConnected() == false)) {

						if(result == NULL ||
							(result->getConnectedTime() > connectionSlot->getConnectedTime()))

						result = connectionSlot;
					}
				}
			}
		}
	}
	return result;
}

void ServerInterface::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *serverInterfaceNode = rootNode->addChild("ServerInterface");

	for(int slotIndex = 0; exitServer == false && slotIndex < GameConstants::maxPlayers; ++slotIndex) {
		MutexSafeWrapper safeMutex(slotAccessorMutexes[slotIndex],CODE_AT_LINE_X(slotIndex));
		if(slots[slotIndex] != NULL) {

			XmlNode *slotNode = serverInterfaceNode->addChild("Slot");

			ConnectionSlot *slot = slots[slotIndex];
			if(slot != NULL) {
				slotNode->addAttribute("isconnected",intToStr(slot->isConnected()), mapTagReplacements);
				slotNode->addAttribute("sessionkey",intToStr(slot->getSessionKey()), mapTagReplacements);
				slotNode->addAttribute("ipaddress",slot->getSocket(false)->getIpAddress(), mapTagReplacements);
				slotNode->addAttribute("name",slot->getName(), mapTagReplacements);
				slotNode->addAttribute("uuid",slot->getUUID(), mapTagReplacements);
			}
			else {
				slotNode->addAttribute("isconnected",intToStr(false), mapTagReplacements);
			}
		}
	}
}

}}//end namespace
