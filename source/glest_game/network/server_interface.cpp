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

#include <cassert>
#include <stdexcept>

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "logger.h"
#include <time.h>
#include "util.h"
#include "game_util.h"
#include "miniftpserver.h"
#include "window.h"
#include <set>
#include <iostream>
#include "map_preview.h"

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
double MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE 				= 2;
const int MAX_SLOT_THREAD_WAIT_TIME 					= 3;
const int MASTERSERVER_HEARTBEAT_GAME_STATUS_SECONDS 	= 30;

ServerInterface::ServerInterface(bool publishEnabled) :GameNetworkInterface() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	serverSynchAccessor = new Mutex();
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		slotAccessorMutexes[i] = new Mutex();
	}
	masterServerThreadAccessor = new Mutex();
	textMessageQueueThreadAccessor = new Mutex();
	broadcastMessageQueueThreadAccessor = new Mutex();
	inBroadcastMessageThreadAccessor = new Mutex();

	nextEventId 					= 1;
	gameHasBeenInitiated 			= false;
	exitServer 						= false;
	gameSettingsUpdateCount 		= 0;
	currentFrameCount 				= 0;
	gameStartTime 					= 0;
	publishToMasterserverThread 	= NULL;
	lastMasterserverHeartbeatTime 	= 0;
	needToRepublishToMasterserver 	= false;
	ftpServer 						= NULL;
	inBroadcastMessage				= false;
	lastGlobalLagCheckTime			= 0;
	masterserverAdminRequestLaunch	= false;

	// This is an admin port listening only on the localhost intended to
	// give current connection status info
	serverSocketAdmin				= new ServerSocket(true);
	serverSocketAdmin->setBlock(false);
	serverSocketAdmin->setBindPort(Config::getInstance().getInt("ServerAdminPort", intToStr(GameConstants::serverAdminPort).c_str()));
	serverSocketAdmin->setBindSpecificAddress("127.0.0.1");
	serverSocketAdmin->listen(5);

	maxFrameCountLagAllowed 				= Config::getInstance().getInt("MaxFrameCountLagAllowed", intToStr(maxFrameCountLagAllowed).c_str());
	maxFrameCountLagAllowedEver 			= Config::getInstance().getInt("MaxFrameCountLagAllowedEver", intToStr(maxFrameCountLagAllowedEver).c_str());
	maxClientLagTimeAllowedEver				= Config::getInstance().getInt("MaxClientLagTimeAllowedEver", intToStr(maxClientLagTimeAllowedEver).c_str());
	maxClientLagTimeAllowed 				= Config::getInstance().getInt("MaxClientLagTimeAllowed", intToStr(maxClientLagTimeAllowed).c_str());
	warnFrameCountLagPercent 				= Config::getInstance().getFloat("WarnFrameCountLagPercent", doubleToStr(warnFrameCountLagPercent).c_str());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] maxFrameCountLagAllowed = %f, maxFrameCountLagAllowedEver = %f, maxClientLagTimeAllowed = %f, maxClientLagTimeAllowedEver = %f\n",__FILE__,__FUNCTION__,__LINE__,maxFrameCountLagAllowed,maxFrameCountLagAllowedEver,maxClientLagTimeAllowed,maxClientLagTimeAllowedEver);

	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		slots[i]				= NULL;
		switchSetupRequests[i]	= NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	serverSocket.setBlock(false);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	serverSocket.setBindPort(Config::getInstance().getInt("ServerPort", intToStr(GameConstants::serverPort).c_str()));
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

/*
	Config &config = Config::getInstance();
	vector<string> results;
	set<string> allMaps;
    findAll(config.getPathListForType(ptMaps), "*.gbm", results, true, false);
	copy(results.begin(), results.end(), std::inserter(allMaps, allMaps.begin()));
	results.clear();
    findAll(config.getPathListForType(ptMaps), "*.mgm", results, true, false);
	copy(results.begin(), results.end(), std::inserter(allMaps, allMaps.begin()));
	results.clear();
	if (allMaps.empty()) {
        throw megaglest_runtime_error("No maps were found!");
	}
	copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
	mapFiles = results;
*/

	Config &config = Config::getInstance();
	vector<string> results;
  	string scenarioDir = "";
  	vector<string> pathList = config.getPathListForType(ptMaps,scenarioDir);
  	vector<string> invalidMapList;
  	vector<string> allMaps = MapPreview::findAllValidMaps(pathList,scenarioDir,false,true,&invalidMapList);
	if (allMaps.empty()) {
        throw megaglest_runtime_error("No maps were found!");
	}
	results.clear();
	copy(allMaps.begin(), allMaps.end(), std::back_inserter(results));
	mapFiles = results;


	//tileset listBox
	results.clear();
    findDirs(config.getPathListForType(ptTilesets), results);
	if (results.empty()) {
        throw megaglest_runtime_error("No tile-sets were found!");
	}
    tilesetFiles= results;

    //tech Tree listBox
    results.clear();
    findDirs(config.getPathListForType(ptTechs), results);
	if(results.empty()) {
        throw megaglest_runtime_error("No tech-trees were found!");
	}
    techTreeFiles= results;

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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		int portNumber   = Config::getInstance().getInt("FTPServerPort",intToStr(ServerSocket::getFTPServerPort()).c_str());
		ServerSocket::setFTPServerPort(portNumber);
		//printf("In [%s::%s] portNumber = %d ServerSocket::getFTPServerPort() = %d\n",__FILE__,__FUNCTION__,__LINE__,portNumber,ServerSocket::getFTPServerPort());

		bool allowInternetTilesetFileTransfers = Config::getInstance().getBool("EnableFTPServerInternetTilesetXfer","true");
		bool allowInternetTechtreeFileTransfers = Config::getInstance().getBool("EnableFTPServerInternetTechtreeXfer","true");

		ftpServer = new FTPServerThread(mapsPath,tilesetsPath,techtreesPath,
				publishEnabled,
				allowInternetTilesetFileTransfers, allowInternetTechtreeFileTransfers,
				portNumber,GameConstants::maxPlayers,this);
		ftpServer->start();
	}

	if(publishToMasterserverThread == NULL) {
		if(needToRepublishToMasterserver == true || GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
			publishToMasterserverThread = new SimpleTaskThread(this,0,125);
			publishToMasterserverThread->setUniqueID(__FILE__);
			publishToMasterserverThread->start();

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",__FILE__,__FUNCTION__,__LINE__,needToRepublishToMasterserver);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
			difftime(time(NULL),elapsed) <= 15;) {
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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	exitServer = true;
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] != NULL) {
			MutexSafeWrapper safeMutex(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			delete slots[i];
			slots[i]=NULL;
		}

		if(switchSetupRequests[i] != NULL) {
			delete switchSetupRequests[i];
			switchSetupRequests[i]=NULL;
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	close();
	if(ftpServer != NULL) {
		ftpServer->shutdownAndWait();
		delete ftpServer;
		ftpServer = NULL;
	}
	shutdownMasterserverPublishThread();

	lastMasterserverHeartbeatTime = 0;
	if(needToRepublishToMasterserver == true) {
		simpleTask(NULL);
	}

	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		delete slotAccessorMutexes[i];
		slotAccessorMutexes[i] = NULL;
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

	for(int i = 0; i < broadcastMessageQueue.size(); ++i) {
		pair<const NetworkMessage *,int> &item = broadcastMessageQueue[i];
		if(item.first != NULL) {
			delete item.first;
		}
		item.first = NULL;
	}
	broadcastMessageQueue.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

int ServerInterface::isValidClientType(uint32 clientIp) {
	int result = 0;
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		if(slots[i] != NULL) {
			MutexSafeWrapper safeMutex(slotAccessorMutexes[i],CODE_AT_LINE_X(i));

			Socket *socket = slots[i]->getSocket();
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

	if(username != NULL && strlen(username) > 0 && filename != NULL && strlen(filename) > 0) {
		string user = username;
		string file = filename;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d username [%s] file [%s]\n",__FILE__,__FUNCTION__,__LINE__,username,filename);

		if(StartsWith(user,"tilesets") == true && EndsWith(file,"7z") == false) {
			if(Config::getInstance().getBool("DisableFTPServerXferUncompressedTilesets","false") == true) {
				result = 0;
			}
			else {
				char szIP[100]="";
				Ip::Inet_NtoA(clientIp,szIP);
				string clientIP = szIP;
				//string serverIP = serverSocket.getIp();
				std::vector<std::string> serverList = Socket::getLocalIPAddressList();

				result = 0;
				for(unsigned int i = 0; i < serverList.size(); ++i) {
					string serverIP = serverList[i];

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d clientIP [%s] serverIP [%s] %d / %d\n",__FILE__,__FUNCTION__,__LINE__,clientIP.c_str(),serverIP.c_str(),i,serverList.size());

					vector<string> clientTokens;
					Tokenize(clientIP,clientTokens,".");

					vector<string> serverTokens;
					Tokenize(serverIP,serverTokens,".");

					if(clientTokens.size() == 4 && serverTokens.size() == 4) {
						if( clientTokens[0] == serverTokens[0] ||
							clientTokens[1] == serverTokens[1] ||
							clientTokens[2] == serverTokens[2]) {
							result = 1;

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d clientIP [%s] IS NOT BLOCKED\n",__FILE__,__FUNCTION__,__LINE__,clientIP.c_str());

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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(serverSocket.isPortBound() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		serverSocket.bind(serverSocket.getBindPort());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(playerIndex));
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ConnectionSlot *slot = slots[playerIndex];
	if(slot != NULL) {
		slots[playerIndex]=NULL;
	}
	slots[playerIndex] = new ConnectionSlot(this, playerIndex);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	safeMutexSlot.ReleaseLock();
	delete slot;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	safeMutex.ReleaseLock();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	updateListen();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool ServerInterface::switchSlot(int fromPlayerIndex, int toPlayerIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	bool result = false;
	assert(fromPlayerIndex >= 0 && fromPlayerIndex < GameConstants::maxPlayers);
	assert(toPlayerIndex >= 0 && toPlayerIndex < GameConstants::maxPlayers);
	if(fromPlayerIndex == toPlayerIndex) {
		return false;
	}

	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[fromPlayerIndex],CODE_AT_LINE_X(fromPlayerIndex));
	MutexSafeWrapper safeMutexSlot2(slotAccessorMutexes[toPlayerIndex],CODE_AT_LINE_X(toPlayerIndex));
	if(slots[toPlayerIndex] != NULL &&
	   slots[toPlayerIndex]->isConnected() == false) {
		slots[fromPlayerIndex]->setPlayerIndex(toPlayerIndex);
		slots[toPlayerIndex]->setPlayerIndex(fromPlayerIndex);
		ConnectionSlot *tmp = slots[toPlayerIndex];
		slots[toPlayerIndex] = slots[fromPlayerIndex];
		slots[fromPlayerIndex] = tmp;
		safeMutex.ReleaseLock();
		PlayerIndexMessage playerIndexMessage(toPlayerIndex);
		slots[toPlayerIndex]->sendMessage(&playerIndexMessage);
		safeMutexSlot.ReleaseLock();
		safeMutexSlot2.ReleaseLock();
		result = true;
		updateListen();
	}
	else {
		safeMutexSlot.ReleaseLock();
		safeMutexSlot2.ReleaseLock();
		safeMutex.ReleaseLock();
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	return result;
}

void ServerInterface::removeSlot(int playerIndex, int lockedSlotIndex) {
	Lang &lang= Lang::getInstance();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	MutexSafeWrapper safeMutexSlot(NULL,CODE_AT_LINE_X(playerIndex));
	if(playerIndex != lockedSlotIndex) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
		safeMutexSlot.setMutex(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(playerIndex));
	}
	ConnectionSlot *slot = slots[playerIndex];
	bool notifyDisconnect = false;
	vector<string> msgList;
	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
	if(slot != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);

		if(slot->getLastReceiveCommandListTime() > 0) {
			char szBuf[4096] = "";

			for(unsigned int i = 0; i < languageList.size(); ++i) {
				string msgTemplate = "Player %s, disconnected from the game.";
				if(lang.hasString("PlayerDisconnected",languageList[i]) == true) {
					msgTemplate = lang.get("PlayerDisconnected",languageList[i]);
				}
#ifdef WIN32
				_snprintf(szBuf,4095,msgTemplate.c_str(),slot->getName().c_str());
#else
				snprintf(szBuf,4095,msgTemplate.c_str(),slot->getName().c_str());
#endif
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

				msgList.push_back(szBuf);
			}

			notifyDisconnect = true;
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
	slots[playerIndex]= NULL;
	safeMutexSlot.ReleaseLock();
	safeMutex.ReleaseLock();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
	delete slot;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
	updateListen();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
	if(notifyDisconnect == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
		//string sMsg = szBuf;
		//sendTextMessage(sMsg,-1, true, lockedSlotIndex);
		//const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
		for(unsigned int j = 0; j < languageList.size(); ++j) {
			bool localEcho = lang.isLanguageLocal(languageList[j]);
			queueTextMessage(msgList[j],-1, localEcho, languageList[j]);

			//printf("j = %d [%s] localEcho = %d [%s]\n",j,msgList[j].c_str(),localEcho,languageList[j].c_str());
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,lockedSlotIndex);
}

ConnectionSlot *ServerInterface::getSlot(int playerIndex) {
	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[playerIndex],CODE_AT_LINE_X(i));
	ConnectionSlot *result = slots[playerIndex];
	return result;
}

bool ServerInterface::isClientConnected(int index) {
	bool result = false;
	MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[index],CODE_AT_LINE_X(i));
	if(slots[index] != NULL && slots[index]->isConnected() == true) {
		result = true;
	}
	return result;
}

bool ServerInterface::hasClientConnection() {
	bool result = false;
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		if(isClientConnected(i) == true) {
			result = true;
			break;
		}
	}
	return result;
}

int ServerInterface::getConnectedSlotCount() {
	int connectedSlotCount = 0;
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		if(slots[i] != NULL) {
			++connectedSlotCount;
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

void ServerInterface::slotUpdateTask(ConnectionSlotEvent *event) {
//	if(event != NULL) {
//		if(event->eventType == eSendSocketData) {
//			if(event->connectionSlot != NULL) {
//				event->connectionSlot->sendMessage(event->networkMessage);
//			}
//		}
//		else if(event->eventType == eReceiveSocketData) {
//			//updateSlot(event);
//			if(event->connectionSlot != NULL) {
//				event->connectionSlot->updateSlot(event);
//			}
//		}
//		else {
//			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
//		}
//	}
}

//void ServerInterface::updateSlot(ConnectionSlotEvent *event) {
//	Chrono chrono;
//	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
//
//	if(event != NULL) {
//		bool &socketTriggered = event->socketTriggered;
//		bool checkForNewClients = true;
//
//		// Safety check since we can experience a disconnect and the slot is NULL
//		ConnectionSlot *connectionSlot = NULL;
//		MutexSafeWrapper safeMutexSlot(NULL);
//		if(event->triggerId >= 0 && event->triggerId < GameConstants::maxPlayers) {
//
//			//printf("===> START slot %d [%d][%p] - About to updateSlot\n",event->triggerId,event->eventId,slots[event->triggerId]);
//
//			safeMutexSlot.setMutex(slotAccessorMutexes[event->triggerId],CODE_AT_LINE_X(event->triggerId));
//			connectionSlot = slots[event->triggerId];
//		}
//		else {
//			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR CONDITION, event->triggerId = %d\n",__FILE__,__FUNCTION__,__LINE__,event->triggerId);
//			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] ERROR CONDITION, event->triggerId = %d\n",__FILE__,__FUNCTION__,__LINE__,event->triggerId);
//		}
//
//		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
//
//		//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] MUTEX LOCK held for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
//
//		if(connectionSlot != NULL &&
//		   (gameHasBeenInitiated == false ||
//			(connectionSlot->getSocket() != NULL && socketTriggered == true))) {
//			if(connectionSlot->isConnected() == false || socketTriggered == true) {
//				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] MUTEX LOCK held for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
//
//				//safeMutexSlot.ReleaseLock(true);
//				connectionSlot->update(checkForNewClients,event->triggerId);
//
//				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 4) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
//
//				//chrono.start();
//				//safeMutexSlot.Lock();
//				//connectionSlot = slots[event->triggerId];
//
//				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] MUTEX LOCK held for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
//
//				// This means no clients are trying to connect at the moment
//				if(connectionSlot != NULL && connectionSlot->getSocket() == NULL) {
//					checkForNewClients = false;
//				}
//			}
//			//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] MUTEX LOCK held for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
//		}
//		safeMutexSlot.ReleaseLock();
//
//		if(event->triggerId >= 0 && event->triggerId < GameConstants::maxPlayers) {
//			//printf("===> END slot %d - About to updateSlot\n",event->triggerId);
//		}
//	}
//	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
//	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
//}

std::pair<bool,bool> ServerInterface::clientLagCheck(ConnectionSlot *connectionSlot, bool skipNetworkBroadCast) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	std::pair<bool,bool> clientLagExceededOrWarned = std::make_pair(false, false);
	static bool alreadyInLagCheck = false;

//	{
//		if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
//			double clientLag = this->getCurrentFrameCount() - connectionSlot->getCurrentFrameCount();
//			double clientLagCount = (gameSettings.getNetworkFramePeriod() > 0 ? (clientLag / gameSettings.getNetworkFramePeriod()) : 0);
//			double clientLagTime = difftime(time(NULL),connectionSlot->getLastReceiveCommandListTime());
//
//			printf("\n\nmaxClientLagTimeAllowedEver [%f] - [%f] alreadyInLagCheck = %d\n\n",maxClientLagTimeAllowedEver,clientLagTime,alreadyInLagCheck);
//		}
//	}

	if(alreadyInLagCheck == true) {
		return clientLagExceededOrWarned;
	}

	try {
		alreadyInLagCheck = true;

		if(difftime(time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD) {
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				double clientLag = this->getCurrentFrameCount() - connectionSlot->getCurrentFrameCount();
				double clientLagCount = (gameSettings.getNetworkFramePeriod() > 0 ? (clientLag / gameSettings.getNetworkFramePeriod()) : 0);
				connectionSlot->setCurrentLagCount(clientLagCount);

				double clientLagTime = difftime(time(NULL),connectionSlot->getLastReceiveCommandListTime());

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				if(this->getCurrentFrameCount() > 0) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, clientLag = %f, clientLagCount = %f, this->getCurrentFrameCount() = %d, connectionSlot->getCurrentFrameCount() = %d, clientLagTime = %f\n",
																		 __FILE__,__FUNCTION__,__LINE__,
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
			    	for(unsigned int i = 0; i < languageList.size(); ++i) {
						char szBuf[4096]="";

						string msgTemplate = "DROPPING %s, exceeded max allowed LAG count of %f [time = %f], clientLag = %f [%f], disconnecting client.";
						if(lang.hasString("ClientLagDropping") == true) {
							msgTemplate = lang.get("ClientLagDropping",languageList[i]);
						}
						if(gameSettings.getNetworkPauseGameForLaggedClients() == true &&
							((maxFrameCountLagAllowedEver <= 0 || clientLagCount <= maxFrameCountLagAllowedEver) &&
							 (maxClientLagTimeAllowedEver <= 0 || clientLagTime <= maxClientLagTimeAllowedEver))) {
							msgTemplate = "PAUSING GAME TEMPORARILY for %s, exceeded max allowed LAG count of %f [time = %f], clientLag = %f [%f], waiting for client to catch up...";
							if(lang.hasString("ClientLagPausing") == true) {
								msgTemplate = lang.get("ClientLagPausing",languageList[i]);
							}
						}
#ifdef WIN32
						_snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str() ,maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
#else
						snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
#endif
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

						if(skipNetworkBroadCast == false) {
							string sMsg = szBuf;
							bool echoLocal = lang.isLanguageLocal(languageList[i]);
							sendTextMessage(sMsg,-1, echoLocal, languageList[i], connectionSlot->getPlayerIndex());
						}
			    	}

					if(gameSettings.getNetworkPauseGameForLaggedClients() == false ||
						(maxFrameCountLagAllowedEver > 0 && clientLagCount > maxFrameCountLagAllowedEver) ||
						(maxClientLagTimeAllowedEver > 0 && clientLagTime > maxClientLagTimeAllowedEver)) {
						connectionSlot->close();
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
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
				    	for(unsigned int i = 0; i < languageList.size(); ++i) {
				    		char szBuf[4096]="";

				    		string msgTemplate = "LAG WARNING for %s, may exceed max allowed LAG count of %f [time = %f], clientLag = %f [%f], WARNING...";
							if(lang.hasString("ClientLagWarning") == true) {
								msgTemplate = lang.get("ClientLagWarning",languageList[i]);
							}

		#ifdef WIN32
				    		_snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#else
				    		snprintf(szBuf,4095,msgTemplate.c_str(),connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#endif
				    		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

				    		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

							if(skipNetworkBroadCast == false) {
								string sMsg = szBuf;
								bool echoLocal = lang.isLanguageLocal(languageList[i]);
								sendTextMessage(sMsg,-1, echoLocal, languageList[i], connectionSlot->getPlayerIndex());
							}
				    	}
					}
				}
				else if(connectionSlot->getLagCountWarning() == true) {
					connectionSlot->setLagCountWarning(false);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				}
			}
		}
	}
	catch(const exception &ex) {
		alreadyInLagCheck = false;

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw megaglest_runtime_error(ex.what());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	alreadyInLagCheck = false;
	return clientLagExceededOrWarned;
}

bool ServerInterface::signalClientReceiveCommands(ConnectionSlot *connectionSlot, int slotIndex, bool socketTriggered, ConnectionSlotEvent & event) {
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

void ServerInterface::updateSocketTriggeredList(std::map<PLATFORM_SOCKET,bool> & socketTriggeredList) {
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot* connectionSlot= slots[i];
		if(connectionSlot != NULL) {
			PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
			if(Socket::isSocketValid(&clientSocket) == true) {
				socketTriggeredList[clientSocket] = false;
			}
		}
	}
}

void ServerInterface::validateConnectedClients() {
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot* connectionSlot = slots[i];
		if(connectionSlot != NULL) {
			connectionSlot->validateConnection();
		}
	}
}

void ServerInterface::signalClientsToRecieveData(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList,
												 std::map<int,ConnectionSlotEvent> &eventList,
												 std::map<int,bool> & mapSlotSignalledList) {
	//bool checkForNewClients = true;
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot* connectionSlot = slots[i];

		bool socketTriggered = false;

		if(connectionSlot != NULL) {
			PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
			if(Socket::isSocketValid(&clientSocket)) {
				socketTriggered = socketTriggeredList[clientSocket];
			}
		}
		ConnectionSlotEvent &event = eventList[i];
		mapSlotSignalledList[i] = signalClientReceiveCommands(connectionSlot,i,socketTriggered,event);
	}
}

void ServerInterface::checkForCompletedClients(std::map<int,bool> & mapSlotSignalledList,
											   std::vector <string> &errorMsgList,
											   std::map<int,ConnectionSlotEvent> &eventList) {
	time_t waitForThreadElapsed = time(NULL);
	std::map<int,bool> slotsCompleted;
	for(bool threadsDone = false;
		exitServer == false && threadsDone == false &&
		difftime(time(NULL),waitForThreadElapsed) < MAX_SLOT_THREAD_WAIT_TIME;) {
		threadsDone = true;
		// Examine all threads for completion of delegation
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {

			//printf("===> START slot %d [%p] - About to checkForCompletedClients\n",i,slots[i]);

			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));

			//printf("===> IN slot %d - About to checkForCompletedClients\n",i);

			ConnectionSlot* connectionSlot = slots[i];
			if(connectionSlot != NULL && mapSlotSignalledList[i] == true &&
			   slotsCompleted.find(i) == slotsCompleted.end()) {
				try {


					std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
					// Collect any collected errors from threads
					if(errorList.empty() == false) {
						for(int iErrIdx = 0; iErrIdx < errorList.size(); ++iErrIdx) {
							string &sErr = errorList[iErrIdx];
							if(sErr != "") {
								errorMsgList.push_back(sErr);
							}
						}
						connectionSlot->clearThreadErrorList();
					}

					// Not done waiting for data yet
					bool updateFinished = (connectionSlot != NULL ? connectionSlot->updateCompleted(&eventList[i]) : true);
					if(updateFinished == false) {
						threadsDone = false;
						break;
					}
					else {
						slotsCompleted[i] = true;
					}
				}
				catch(const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());

					errorMsgList.push_back(ex.what());
				}
			}

			//printf("===> END slot %d - About to checkForCompletedClients\n",i);
		}
	}
}

void ServerInterface::checkForLaggingClients(std::map<int,bool> &mapSlotSignalledList,
											std::map<int,ConnectionSlotEvent> &eventList,
											std::map<PLATFORM_SOCKET,bool> &socketTriggeredList,
											std::vector <string> &errorMsgList) {
	bool lastGlobalLagCheckTimeUpdate = false;
	time_t waitForClientsElapsed = time(NULL);
	time_t waitForThreadElapsed = time(NULL);
	std::map<int,bool> slotsCompleted;
	std::map<int,bool> slotsWarnedList;
	for(bool threadsDone = false;
		exitServer == false && threadsDone == false &&
		difftime(time(NULL),waitForThreadElapsed) < MAX_SLOT_THREAD_WAIT_TIME;) {
		threadsDone = true;
		// Examine all threads for completion of delegation
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot* connectionSlot = slots[i];
			if(connectionSlot != NULL && mapSlotSignalledList[i] == true &&
			   slotsCompleted.find(i) == slotsCompleted.end()) {
				try {
					std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
					// Show any collected errors from threads
					if(errorList.empty() == false) {
						for(int iErrIdx = 0; iErrIdx < errorList.size(); ++iErrIdx) {
							string &sErr = errorList[iErrIdx];
							if(sErr != "") {
								errorMsgList.push_back(sErr);
							}
						}
						connectionSlot->clearThreadErrorList();
					}

					// Not done waiting for data yet
					bool updateFinished = (connectionSlot != NULL ? connectionSlot->updateCompleted(&eventList[i]) : true);
					if(updateFinished == false) {
						threadsDone = false;
						break;
					}
					else {
						// New lag check
						std::pair<bool,bool> clientLagExceededOrWarned = std::make_pair(false,false);
						if( gameHasBeenInitiated == true && connectionSlot != NULL &&
							connectionSlot->isConnected() == true) {
							clientLagExceededOrWarned = clientLagCheck(connectionSlot,slotsWarnedList[i]);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, gameSettings.getNetworkPauseGameForLaggedClients() = %d\n",__FILE__,__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,gameSettings.getNetworkPauseGameForLaggedClients());

							if(clientLagExceededOrWarned.first == true) {
								slotsWarnedList[i] = true;
							}
						}
						// If the client has exceeded lag and the server wants
						// to pause while they catch up, re-trigger the
						// client reader thread
						if((clientLagExceededOrWarned.first == true && gameSettings.getNetworkPauseGameForLaggedClients() == true)) { // ||
							//(clientLagExceededOrWarned.second == true && slotsWarnedAndRetried[i] == false)) {

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, difftime(time(NULL),waitForClientsElapsed) = %.2f, MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE = %.2f\n",__FILE__,__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,difftime(time(NULL),waitForClientsElapsed),MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE);

							if(difftime(time(NULL),waitForClientsElapsed) < MAX_CLIENT_WAIT_SECONDS_FOR_PAUSE) {
								if(connectionSlot != NULL) {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d\n",__FILE__,__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second);

									bool socketTriggered = false;
									PLATFORM_SOCKET clientSocket = connectionSlot->getSocketId();
									if(clientSocket > 0) {
										socketTriggered = socketTriggeredList[clientSocket];
									}
									ConnectionSlotEvent &event = eventList[i];
									mapSlotSignalledList[i] = signalClientReceiveCommands(connectionSlot,i,socketTriggered,event);
									threadsDone = false;
								}
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d\n",__FILE__,__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second);
							}
						}
						else {
							slotsCompleted[i] = true;
						}
					}
				}
				catch(const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					errorMsgList.push_back(ex.what());
				}
			}

			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				try {
					if(gameHasBeenInitiated == true &&
						difftime(time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_GRACE_PERIOD) {

						//printf("\n\n\n^^^^^^^^^^^^^^ PART A\n\n\n");

						// New lag check
						std::pair<bool,bool> clientLagExceededOrWarned = std::make_pair(false,false);
						if( gameHasBeenInitiated == true && connectionSlot != NULL &&
							connectionSlot->isConnected() == true) {
							//printf("\n\n\n^^^^^^^^^^^^^^ PART B\n\n\n");

							lastGlobalLagCheckTimeUpdate = true;
							clientLagExceededOrWarned = clientLagCheck(connectionSlot,slotsWarnedList[i]);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] clientLagExceededOrWarned.first = %d, clientLagExceededOrWarned.second = %d, gameSettings.getNetworkPauseGameForLaggedClients() = %d\n",__FILE__,__FUNCTION__,__LINE__,clientLagExceededOrWarned.first,clientLagExceededOrWarned.second,gameSettings.getNetworkPauseGameForLaggedClients());

							if(clientLagExceededOrWarned.first == true) {
								slotsWarnedList[i] = true;
							}
						}
					}
				}
				catch(const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					errorMsgList.push_back(ex.what());
				}
			}
		}
	}

	if(lastGlobalLagCheckTimeUpdate == true) {
		lastGlobalLagCheckTime = time(NULL);
	}
}

void ServerInterface::executeNetworkCommandsFromClients() {
	if(gameHasBeenInitiated == true) {
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot* connectionSlot= slots[i];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				vector<NetworkCommand> pendingList = connectionSlot->getPendingNetworkCommandList(true);
				if(pendingList.empty() == false) {
					for(int idx = 0; exitServer == false && idx < pendingList.size(); ++idx) {
						NetworkCommand &cmd = pendingList[idx];
						this->requestCommand(&cmd);
					}
				}
			}
		}
	}
}

void ServerInterface::dispatchPendingChatMessages(std::vector <string> &errorMsgList) {
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot* connectionSlot= slots[i];
		if(connectionSlot != NULL &&
		   connectionSlot->getChatTextList(false).empty() == false) {
			try {
				std::vector<ChatMsgInfo> chatText = connectionSlot->getChatTextList(true);
				for(int chatIdx = 0;
					exitServer == false && slots[i] != NULL &&
					chatIdx < chatText.size(); chatIdx++) {
					connectionSlot= slots[i];
					if(connectionSlot != NULL) {
						ChatMsgInfo msg(chatText[chatIdx]);
						this->addChatInfo(msg);

						string newChatText     = msg.chatText.c_str();
						int newChatTeamIndex   = msg.chatTeamIndex;
						int newChatPlayerIndex = msg.chatPlayerIndex;
						string newChatLanguage = msg.targetLanguage;

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 about to broadcast nmtText chatText [%s] chatTeamIndex = %d, newChatPlayerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex);

						if(newChatLanguage == "" || newChatLanguage == connectionSlot->getNetworkPlayerLanguage()) {
							NetworkMessageText networkMessageText(newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex,newChatLanguage);
							broadcastMessage(&networkMessageText, connectionSlot->getPlayerIndex(),i);
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);
					}
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);
				// Its possible that the slot is disconnected here
				// so check the original pointer again
				if(slots[i] != NULL) {
					slots[i]->clearChatInfo();
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
				errorMsgList.push_back(ex.what());
			}
		}
	}
}

void ServerInterface::update() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	//printf("\nServerInterface::update -- A\n");

	std::vector <string> errorMsgList;
	try {
		// The first thing we will do is check all clients to ensure they have
		// properly identified themselves within the alloted time period
		validateConnectedClients();

		//printf("\nServerInterface::update -- B\n");

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		processTextMessageQueue();
		processBroadCastMessageQueue();

		//printf("\nServerInterface::update -- C\n");

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		std::map<PLATFORM_SOCKET,bool> socketTriggeredList;
		//update all slots
		updateSocketTriggeredList(socketTriggeredList);

		//printf("\nServerInterface::update -- D\n");

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		if(gameHasBeenInitiated == false || socketTriggeredList.empty() == false) {
			//printf("\nServerInterface::update -- E\n");

			std::map<int,ConnectionSlotEvent> eventList;
			bool hasData = Socket::hasDataToRead(socketTriggeredList);

			if(hasData) if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] hasData == true\n",__FILE__,__FUNCTION__);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

			if(gameHasBeenInitiated == false || hasData == true) {
				std::map<int,bool> mapSlotSignalledList;

				// Step #1 tell all connection slot worker threads to receive socket data
				signalClientsToRecieveData(socketTriggeredList, eventList, mapSlotSignalledList);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #2\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				// Step #2 check all connection slot worker threads for completed status
				checkForCompletedClients(mapSlotSignalledList,errorMsgList, eventList);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #3\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				// Step #3 check clients for any lagging scenarios and try to deal with them
				checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #4\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				// Step #4 dispatch network commands to the pending list so that they are done in proper order
				executeNetworkCommandsFromClients();
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ============ Step #5\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				// Step #5 dispatch pending chat messages
				dispatchPendingChatMessages(errorMsgList);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			}
			else if(gameHasBeenInitiated == true &&
					difftime(time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_GRACE_PERIOD) {
				//printf("\nServerInterface::update -- E1\n");

				//std::map<int,ConnectionSlotEvent> eventList;
				std::map<int,bool> mapSlotSignalledList;

				checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		}
		else if(gameHasBeenInitiated == true &&
				difftime(time(NULL),lastGlobalLagCheckTime) >= LAG_CHECK_GRACE_PERIOD) {
			//printf("\nServerInterface::update -- F\n");

			std::map<int,ConnectionSlotEvent> eventList;
			std::map<int,bool> mapSlotSignalledList;

			checkForLaggingClients(mapSlotSignalledList, eventList, socketTriggeredList,errorMsgList);
		}

		// Check if we need to switch masterserver admin to a new player because original admin disconnected
		if(gameHasBeenInitiated == true && this->gameSettings.getMasterserver_admin() > 0) {
			//!!!
			bool foundAdminSlot = false;
			int iFirstConnectedSlot = -1;
			for(int i= 0; i < GameConstants::maxPlayers; ++i) {
				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
				if(slots[i] != NULL) {
					if(iFirstConnectedSlot < 0) {
						iFirstConnectedSlot = i;
					}
					if(this->gameSettings.getMasterserver_admin() == slots[i]->getSessionKey()) {
						foundAdminSlot = true;
						break;
					}
				}
			}

			if(foundAdminSlot == false && iFirstConnectedSlot >= 0) {
				printf("Switching masterserver admin to slot#%d...\n",iFirstConnectedSlot);

				string sMsg = "Switching player to admin mode: " + slots[iFirstConnectedSlot]->getName();
				sendTextMessage(sMsg,-1, true,"");

				this->gameSettings.setMasterserver_admin(slots[iFirstConnectedSlot]->getSessionKey());
				this->broadcastGameSetup(&this->gameSettings);
			}
		}
		//printf("\nServerInterface::update -- G\n");
	}
	catch(const exception &ex) {
		//printf("\nServerInterface::update -- H\n");

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		errorMsgList.push_back(ex.what());
	}
	if(errorMsgList.empty() == false){
		for(int iErrIdx = 0;iErrIdx < errorMsgList.size();++iErrIdx){
			string & sErr = errorMsgList[iErrIdx];
			if(sErr != ""){
				DisplayErrorMessage(sErr);
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

void ServerInterface::updateKeyframe(int frameCount) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
	currentFrameCount = frameCount;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] currentFrameCount = %d, requestedCommands.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,currentFrameCount,requestedCommands.size());
	NetworkMessageCommandList networkMessageCommandList(frameCount);

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
			//char szBuf[1024]="";
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,requestedCommands.size());
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,requestedCommands.size());

			string sMsg = "may go out of synch: server requestedCommands.size() = " + intToStr(requestedCommands.size());
			sendTextMessage(sMsg,-1, true,"");
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] build command list took %lld msecs, networkMessageCommandList.getCommandCount() = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),networkMessageCommandList.getCommandCount(),frameCount);

		//broadcast commands
		broadcastMessage(&networkMessageCommandList);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] broadcastMessage took %lld msecs, networkMessageCommandList.getCommandCount() = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),networkMessageCommandList.getCommandCount(),frameCount);
}

bool ServerInterface::shouldDiscardNetworkMessage(NetworkMessageType networkMessageType, ConnectionSlot *connectionSlot) {
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

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 about to broadcast nmtText chatText [%s] chatTeamIndex = %d, newChatPlayerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex);

				NetworkMessageText networkMessageText(newChatText.c_str(),newChatTeamIndex,newChatPlayerIndex,newChatLanguage);
				broadcastMessage(&networkMessageText, connectionSlot->getPlayerIndex());

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,newChatText.c_str(),newChatTeamIndex);

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

	Lang &lang= Lang::getInstance();
	uint64 waitLoopIterationCount = 0;
	uint64 MAX_LOOP_COUNT_BEFORE_SLEEP = 10;
	MAX_LOOP_COUNT_BEFORE_SLEEP = Config::getInstance().getInt("NetworkServerLoopGameLoadingCap",intToStr(MAX_LOOP_COUNT_BEFORE_SLEEP).c_str());
	int sleepMillis = Config::getInstance().getInt("NetworkServerLoopGameLoadingCapSleepMillis","10");

	while(exitServer == false && allReady == false && logger.getCancelLoading() == false) {
		waitLoopIterationCount++;
		if(waitLoopIterationCount > 0 && waitLoopIterationCount % MAX_LOOP_COUNT_BEFORE_SLEEP == 0) {
			sleep(sleepMillis);
			waitLoopIterationCount = 0;
		}
		vector<string> waitingForHosts;
		allReady= true;
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i)	{
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot* connectionSlot= slots[i];
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
						}
						else if(networkMessageType != nmtInvalid) {
							string sErr = "Unexpected network message: " + intToStr(networkMessageType);
							sendTextMessage(sErr,-1, true,"",i);
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
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
					string sErr = "Timeout waiting for clients.";
					if(lang.hasString("TimeoutWaitingForClients") == true) {
						sErr = lang.get("TimeoutWaitingForClients",languageList[i]);
					}
					bool localEcho = lang.isLanguageLocal(languageList[i]);
					sendTextMessage(sErr,-1, localEcho, languageList[i]);
					if(localEcho == true) {
						DisplayErrorMessage(sErr);
					}
		    	}
				logger.setCancelLoading(false);
				return;
			}
			else {
				if(chrono.getMillis() % 100 == 0) {
					string waitForHosts = "";
					for(int i = 0; i < waitingForHosts.size(); i++) {
						if(waitForHosts != "") {
							waitForHosts += ", ";
						}
						waitForHosts += waitingForHosts[i];
					}

					char szBuf[1024]="";
					string updateTextFormat = lang.get("NetworkGameServerLoadStatus");
					if(updateTextFormat == "" || updateTextFormat[0] == '?') {
						updateTextFormat =  "Waiting for network: %lld seconds elapsed (maximum wait time: %d seconds)";
					}
					sprintf(szBuf,updateTextFormat.c_str(),(long long int)(chrono.getMillis() / 1000),int(readyWaitTimeout / 1000));

					char szBuf1[1024]="";
					string statusTextFormat = lang.get("NetworkGameStatusWaiting");
					if(statusTextFormat == "" || statusTextFormat[0] == '?') {
						statusTextFormat =  "Waiting for players: %s";
					}
					sprintf(szBuf1,statusTextFormat.c_str(),waitForHosts.c_str());

					logger.add(szBuf, true, szBuf1);

					uint32 loadingStatus = nmls_NONE;
					//send ready message after, so clients start delayed
					for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
						MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
						ConnectionSlot* connectionSlot= slots[i];
						if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
							switch(i) {
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
					for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
						MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
						ConnectionSlot* connectionSlot= slots[i];
						if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
							NetworkMessageLoadingStatus networkMessageLoadingStatus(loadingStatus);
							connectionSlot->sendMessage(&networkMessageLoadingStatus);
						}
					}

					sleep(0);
				}
			}
		}

		Window::handleEvent();
	}

	if(logger.getCancelLoading() == true) {
    	Lang &lang= Lang::getInstance();
    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			string sErr = lang.get("GameCancelledByUser",languageList[i]);
			bool localEcho = lang.isLanguageLocal(languageList[i]);
			sendTextMessage(sErr,-1, localEcho,languageList[i]);
			if(localEcho == true) {
				DisplayErrorMessage(sErr);
			}
    	}
		quitGame(true);

		//DisplayErrorMessage(sErr);
		logger.setCancelLoading(false);
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] PART B (telling client we are ready!\n",__FUNCTION__);
	try {
		//send ready message after, so clients start delayed
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot* connectionSlot= slots[i];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				NetworkMessageReady networkMessageReady(checksum->getSum());
				connectionSlot->sendMessage(&networkMessageReady);
			}
		}

		gameStartTime = time(NULL);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] END\n",__FUNCTION__);
}

void ServerInterface::processBroadCastMessageQueue() {
	MutexSafeWrapper safeMutexSlot(broadcastMessageQueueThreadAccessor,CODE_AT_LINE);
	if(broadcastMessageQueue.empty() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] broadcastMessageQueue.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,broadcastMessageQueue.size());
	for(int i = 0; i < broadcastMessageQueue.size(); ++i) {
		pair<const NetworkMessage *,int> &item = broadcastMessageQueue[i];
		if(item.first != NULL) {
			this->broadcastMessage(item.first,item.second);
			delete item.first;
		}
		item.first = NULL;
	}
	broadcastMessageQueue.clear();
}
}

void ServerInterface::queueBroadcastMessage(const NetworkMessage *networkMessage, int excludeSlot) {
	MutexSafeWrapper safeMutexSlot(broadcastMessageQueueThreadAccessor,CODE_AT_LINE);
	pair<const NetworkMessage*,int> item;
	item.first = networkMessage;
	item.second = excludeSlot;
	broadcastMessageQueue.push_back(item);
}

void ServerInterface::processTextMessageQueue() {
	MutexSafeWrapper safeMutexSlot(textMessageQueueThreadAccessor,CODE_AT_LINE);
	if(textMessageQueue.empty() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] textMessageQueue.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,textMessageQueue.size());
		for(int i = 0; i < textMessageQueue.size(); ++i) {
			TextMessageQueue &item = textMessageQueue[i];
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
	item.text = text;
	item.teamIndex = teamIndex;
	item.echoLocal = echoLocal;
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
	//assert(text.length() > 0);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] text [%s] teamIndex = %d, echoLocal = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,text.c_str(),teamIndex,echoLocal,lockedSlotIndex);
	NetworkMessageText networkMessageText(text, teamIndex, getHumanPlayerIndex(), targetLanguage);
	broadcastMessage(&networkMessageText, -1, lockedSlotIndex);
	if(echoLocal == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		//ChatMsgInfo msg(text.c_str(),networkMessageText.getSender().c_str(),teamIndex,networkMessageText.getPlayerIndex());
		ChatMsgInfo msg(text.c_str(),teamIndex,networkMessageText.getPlayerIndex(), targetLanguage);
		this->addChatInfo(msg);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::quitGame(bool userManuallyQuit) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

string ServerInterface::getNetworkStatus() {
	Lang &lang = Lang::getInstance();
	string str="";
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot* connectionSlot= slots[i];

		str+= intToStr(i)+ ": ";

		if(connectionSlot!= NULL) {
			if(connectionSlot->isConnected()) {
				int clientLagCount 					= connectionSlot->getCurrentLagCount();
				double lastClientCommandListTimeLag = difftime(time(NULL),connectionSlot->getLastReceiveCommandListTime());
				//float pingTime = connectionSlot->getThreadedPingMS(connectionSlot->getIpAddress().c_str());
				char szBuf[1024]="";
				//sprintf(szBuf,", lag = %d [%.2f], ping = %.2fms",clientLagCount,lastClientCommandListTimeLag,pingTime);
				sprintf(szBuf,", lag = %d [%.2f]",clientLagCount,lastClientCommandListTimeLag);
				str+= connectionSlot->getName() + string(szBuf);
			}
		}
		else {
			str+= lang.get("NotConnected");
		}

		str+= '\n';
	}
	return str;
}

bool ServerInterface::launchGame(const GameSettings *gameSettings) {
	bool bOkToStart = true;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot *connectionSlot= slots[i];
		if(connectionSlot != NULL &&
		   (connectionSlot->getAllowDownloadDataSynch() == true || connectionSlot->getAllowGameDataSynchCheck() == true) &&
		   connectionSlot->isConnected()) {
			if(connectionSlot->getNetworkGameDataSynchCheckOk() == false) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] map [%d] tile [%d] techtree [%d]\n",__FILE__,__FUNCTION__,__LINE__,connectionSlot->getNetworkGameDataSynchCheckOkMap(),connectionSlot->getNetworkGameDataSynchCheckOkTile(),connectionSlot->getNetworkGameDataSynchCheckOkTech());
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] map [%d] tile [%d] techtree [%d]\n",__FILE__,__FUNCTION__,__LINE__,connectionSlot->getNetworkGameDataSynchCheckOkMap(),connectionSlot->getNetworkGameDataSynchCheckOkTile(),connectionSlot->getNetworkGameDataSynchCheckOkTech());

				bOkToStart = false;
				break;
			}
		}
	}
	if(bOkToStart == true) {

		bool useInGameBlockingClientSockets = Config::getInstance().getBool("EnableInGameBlockingSockets","true");
		if(useInGameBlockingClientSockets == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			for(int i= 0; i < GameConstants::maxPlayers; ++i) {
				MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
				ConnectionSlot *connectionSlot= slots[i];
				if(connectionSlot != NULL && connectionSlot->isConnected()) {
					connectionSlot->getSocket()->setBlock(true);
				}
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",__FILE__,__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		serverSocket.stopBroadCastThread();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",__FILE__,__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		NetworkMessageLaunch networkMessageLaunch(gameSettings,nmtLaunch);
		broadcastMessage(&networkMessageLaunch);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",__FILE__,__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		shutdownMasterserverPublishThread();
		MutexSafeWrapper safeMutex(masterServerThreadAccessor,CODE_AT_LINE);
		lastMasterserverHeartbeatTime = 0;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ftpServer = %p\n",__FILE__,__FUNCTION__,__LINE__,ftpServer);

		if(ftpServer != NULL) {
			ftpServer->shutdownAndWait();
			delete ftpServer;
			ftpServer = NULL;
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",__FILE__,__FUNCTION__,__LINE__,needToRepublishToMasterserver);

		if(publishToMasterserverThread == NULL) {
			if(needToRepublishToMasterserver == true || GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
				publishToMasterserverThread = new SimpleTaskThread(this,0,125);
				publishToMasterserverThread->setUniqueID(__FILE__);
				publishToMasterserverThread->start();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] needToRepublishToMasterserver = %d\n",__FILE__,__FUNCTION__,__LINE__,needToRepublishToMasterserver);
			}
		}

		if(ftpServer != NULL) {
			ftpServer->shutdownAndWait();
			delete ftpServer;
			ftpServer = NULL;
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	return bOkToStart;
}

void ServerInterface::broadcastGameSetup(GameSettings *gameSettingsBuffer, bool setGameSettingsBuffer) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	if(setGameSettingsBuffer == true) {
		validateGameSettings(gameSettingsBuffer);
		//setGameSettings(gameSettingsBuffer,false);
		MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
		gameSettings = *gameSettingsBuffer;
		gameSettingsUpdateCount++;
	}
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	NetworkMessageLaunch networkMessageLaunch(gameSettingsBuffer, nmtBroadCastSetup);
	broadcastMessage(&networkMessageLaunch);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::broadcastMessage(const NetworkMessage *networkMessage, int excludeSlot, int lockedSlotIndex) {
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	    MutexSafeWrapper safeMutexSlotBroadCastAccessor(inBroadcastMessageThreadAccessor,CODE_AT_LINE);
	    if(inBroadcastMessage == true && dynamic_cast<const NetworkMessageText *>(networkMessage) != NULL) {
	    	safeMutexSlotBroadCastAccessor.ReleaseLock();
	    	const NetworkMessageText *txtMsg = dynamic_cast<const NetworkMessageText *>(networkMessage);
	    	const NetworkMessageText *msgCopy = txtMsg->getCopy();
	    	queueBroadcastMessage(msgCopy, excludeSlot);
	    	return;
	    }
	    else {
			inBroadcastMessage = true;
			safeMutexSlotBroadCastAccessor.ReleaseLock(true);
	    }

		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(NULL,CODE_AT_LINE_X(i));
			if(i != lockedSlotIndex) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] i = %d, lockedSlotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,i,lockedSlotIndex);
				safeMutexSlot.setMutex(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			}

			ConnectionSlot* connectionSlot= slots[i];

			if(i != excludeSlot && connectionSlot != NULL) {
				if(connectionSlot->isConnected()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] before sendMessage\n",__FILE__,__FUNCTION__,__LINE__);
					connectionSlot->sendMessage(networkMessage);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after sendMessage\n",__FILE__,__FUNCTION__,__LINE__);
				}
				if(gameHasBeenInitiated == true && connectionSlot->isConnected() == false) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,__LINE__,i);
					//safeMutexSlot.ReleaseLock();
					removeSlot(i,i);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 after removeSlot for slot# %d\n",__FILE__,__FUNCTION__,__LINE__,i);
				}
			}
			else if(i == excludeSlot && gameHasBeenInitiated == true &&
					connectionSlot != NULL && connectionSlot->isConnected() == false) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,__LINE__,i);
				//safeMutexSlot.ReleaseLock();
				removeSlot(i,i);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 after removeSlot for slot# %d\n",__FILE__,__FUNCTION__,__LINE__,i);
			}
		}

		safeMutexSlotBroadCastAccessor.Lock();
	    inBroadcastMessage = false;
	    safeMutexSlotBroadCastAccessor.ReleaseLock();
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());

		MutexSafeWrapper safeMutexSlotBroadCastAccessor(inBroadcastMessageThreadAccessor,CODE_AT_LINE);
	    inBroadcastMessage = false;
	    safeMutexSlotBroadCastAccessor.ReleaseLock();

		string sMsg = ex.what();
		sendTextMessage(sMsg,-1, true, "", lockedSlotIndex);
	}
}

void ServerInterface::broadcastMessageToConnectedClients(const NetworkMessage *networkMessage, int excludeSlot) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	try {
		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot *connectionSlot= slots[i];

			if(i != excludeSlot && connectionSlot != NULL) {
				if(connectionSlot->isConnected()) {
					connectionSlot->sendMessage(networkMessage);
				}
			}
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::updateListen() {
	if(gameHasBeenInitiated == true){
		return;
	}
	int openSlotCount = 0;
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i)	{
		//MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],intToStr(__LINE__) + "_" + intToStr(i));
		bool isSlotOpen = (slots[i] != NULL && slots[i]->isConnected() == false);

		if(isSlotOpen == true) {
			++openSlotCount;
		}
	}

	serverSocket.listen(openSlotCount);
}

int ServerInterface::getOpenSlotCount() {
	int openSlotCount = 0;
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i)	{
		//MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],intToStr(__LINE__) + "_" + intToStr(i));
		bool isSlotOpen = (slots[i] != NULL && slots[i]->isConnected() == false);

		if(isSlotOpen == true) {
			++openSlotCount;
		}
	}
	return openSlotCount;
}

int ServerInterface::getGameSettingsUpdateCount() {
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START gameSettingsUpdateCount = %d\n",__FILE__,__FUNCTION__,gameSettingsUpdateCount);

	int result = gameSettingsUpdateCount;
	safeMutex.ReleaseLock();
	return result;
}

void ServerInterface::validateGameSettings(GameSettings *serverGameSettings) {
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s]\n",__FILE__,__FUNCTION__);

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

void ServerInterface::setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck) {
	MutexSafeWrapper safeMutex(serverSynchAccessor,CODE_AT_LINE);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START gameSettingsUpdateCount = %d, waitForClientAck = %d\n",__FILE__,__FUNCTION__,gameSettingsUpdateCount,waitForClientAck);

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
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Waiting for client acks #1\n",__FILE__,__FUNCTION__);

			time_t tStart = time(NULL);
			bool gotAckFromAllClients = false;
			while(gotAckFromAllClients == false && difftime(time(NULL),tStart) <= 5) {
				gotAckFromAllClients = true;
				for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
					//printf("===> START slot %d - About to setGameSettings #1\n",i);

					MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
					ConnectionSlot *connectionSlot = slots[i];
					if(connectionSlot != NULL && connectionSlot->isConnected()) {
						if(connectionSlot->getReceivedNetworkGameStatus() == false) {
							gotAckFromAllClients = false;
						}

						connectionSlot->update(true,i);
					}

					//printf("===> END slot %d - About to setGameSettings #1\n",i);
				}
			}
		}

		for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
			MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
			ConnectionSlot *connectionSlot = slots[i];
			if(connectionSlot != NULL && connectionSlot->isConnected()) {
				connectionSlot->setReceivedNetworkGameStatus(false);
			}
		}

		NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(getGameSettings());
		broadcastMessageToConnectedClients(&networkMessageSynchNetworkGameData);

		if(waitForClientAck == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Waiting for client acks #2\n",__FILE__,__FUNCTION__);

			time_t tStart = time(NULL);
			bool gotAckFromAllClients = false;
			while(gotAckFromAllClients == false && difftime(time(NULL),tStart) <= 5) {
				gotAckFromAllClients = true;
				for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
					//printf("===> START slot %d - About to setGameSettings 2\n",i);

					MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
					ConnectionSlot *connectionSlot = slots[i];
					if(connectionSlot != NULL && connectionSlot->isConnected()) {
						if(connectionSlot->getReceivedNetworkGameStatus() == false) {
							gotAckFromAllClients = false;
						}

						connectionSlot->update(true,i);
					}

					//printf("===> END slot %d - About to setGameSettings 2\n",i);
				}
			}
		}

	}
	gameSettingsUpdateCount++;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::close() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	int slotCountUsed = 1;
	int slotCountHumans = 1;
	int slotCountConnectedPlayers = 1;
	Config & config = Config::getInstance();
	std::map < string, string > publishToServerInfo;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		if(slots[i] != NULL) {
			slotCountUsed++;
			slotCountHumans++;
			ConnectionSlot* connectionSlot= slots[i];
			if((connectionSlot!=NULL) && (connectionSlot->isConnected())) {
				slotCountConnectedPlayers++;
			}
		}
	}
	publishToServerInfo["glestVersion"] = glestVersionString;
	publishToServerInfo["platform"] = getPlatformNameString();
	publishToServerInfo["binaryCompileDate"] = getCompileDateTime();
	publishToServerInfo["serverTitle"] = getHumanPlayerName() + "'s game";
	publishToServerInfo["tech"] = this->getGameSettings()->getTech();
	publishToServerInfo["map"] = this->getGameSettings()->getMap();
	publishToServerInfo["tileset"] = this->getGameSettings()->getTileset();
	publishToServerInfo["activeSlots"] = intToStr(slotCountUsed);
	publishToServerInfo["networkSlots"] = intToStr(slotCountHumans);
	publishToServerInfo["connectedClients"] = intToStr(slotCountConnectedPlayers);
	string externalport = config.getString("MasterServerExternalPort", intToStr(GameConstants::serverPort).c_str());
	publishToServerInfo["externalconnectport"] = externalport;
	publishToServerInfo["privacyPlease"] = intToStr(config.getBool("PrivacyPlease","false"));
	publishToServerInfo["gameStatus"] = intToStr(game_status_in_progress);

	if(publishToMasterserverThread == NULL) {
		publishToServerInfo["gameCmd"]= "gameOver";
		publishToServerInfo["gameStatus"] = intToStr(game_status_finished);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	return publishToServerInfo;
}

void ServerInterface::simpleTask(BaseThread *callingThread) {
	MutexSafeWrapper safeMutex(masterServerThreadAccessor,CODE_AT_LINE);

	if(difftime(time(NULL),lastMasterserverHeartbeatTime) >= MASTERSERVER_HEARTBEAT_GAME_STATUS_SECONDS) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

					//printf("the request is:\n%s\n",request.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d] the request is:\n%s\n",__FILE__,__FUNCTION__,__LINE__,request.c_str());

					std::string serverInfo = SystemFlags::getHTTP(request,handle);
					SystemFlags::cleanupHTTP(&handle);

					//printf("the result is:\n'%s'\n",serverInfo.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line %d] the result is:\n'%s'\n",__FILE__,__FUNCTION__,__LINE__,serverInfo.c_str());
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error, no masterserver defined!\n",__FILE__,__FUNCTION__,__LINE__);
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error during game status update: [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			}
		}
		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
			DumpStatsToLog(false);
		}
	}
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		//printf("Attempt Accept\n");
		Socket *cli = serverSocketAdmin->accept(false);
		if(cli != NULL) {
			printf("Got status request connection, dumping info...\n");

			string data = DumpStatsToLog(true);
			cli->send(data.c_str(),data.length());
			cli->disconnectSocket();
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
	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		MutexSafeWrapper safeMutexSlot(slotAccessorMutexes[i],CODE_AT_LINE_X(i));
		ConnectionSlot *slot = slots[i];
		if(slot != NULL) {
			connectedSlotCount++;
			out << "Network connection for index: " << i << std::endl;
			out << "------------------------------" << std::endl;
			out << "Connected: " << boolToStr(slot->isConnected()) << std::endl;
			out << "Handshake received: " << boolToStr(slot->getConnectHasHandshaked()) << std::endl;
			if(slot->isConnected() == true) {
				time_t connectTime = slot->getConnectedTime();
				struct tm *loctime = localtime (&connectTime);
				char szBuf[8096]="";
				strftime(szBuf,100,"%Y-%m-%d %H:%M:%S",loctime);

				const int HOURS_IN_DAY = 24;
				const int MINUTES_IN_HOUR = 60;
				const int SECONDS_IN_MINUTE = 60;
				int InSeconds = difftime(time(NULL),slot->getConnectedTime());
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

				out << "Connected at: " << szBuf << std::endl;
				out << "Connection duration: " << hours << " hours " << minutes << " minutes " << seconds << " seconds." << std::endl;
				out << "Player Index: " << slot->getPlayerIndex() << std::endl;
				out << "IP Address: " << slot->getIpAddress() << std::endl;
				out << "Player name: " << slot->getName() << std::endl;
				out << "Language: " << slot->getNetworkPlayerLanguage() << std::endl;
				out << "Game Version: " << slot->getVersionString() << std::endl;
				out << "Session id: " << slot->getSessionKey() << std::endl;
				out << "Socket id: " << slot->getSocketId() << std::endl;
			}
		}
	}
	out << "Total Slot Count: " << connectedSlotCount << std::endl;
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
	//printf("In [%s::%s Line: %d] ipAddress [%s]\n",__FILE__,__FUNCTION__,__LINE__,ipAddress.c_str());

	if(badClientConnectIPList.find(ipAddress) == badClientConnectIPList.end()) {
		badClientConnectIPList[ipAddress] = make_pair(0,time(NULL));
		//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	pair<uint64,time_t> &lastBadConnectionAttempt = badClientConnectIPList[ipAddress];

	const uint64 BLOCK_BAD_CLIENT_CONNECT_MAX_SECONDS 	= Config::getInstance().getInt("BlockBadClientConnectMaxSeconds", "60");
	const uint64 BLOCK_BAD_CLIENT_CONNECT_MAX_ATTEMPTS 	= Config::getInstance().getInt("BlockBadClientConnectMaxAttempts", "6");
	bool addToBlockedClientsList 						= false;

	if(difftime(time(NULL),lastBadConnectionAttempt.second) <= BLOCK_BAD_CLIENT_CONNECT_MAX_SECONDS) {
		//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(lastBadConnectionAttempt.first+1 > BLOCK_BAD_CLIENT_CONNECT_MAX_ATTEMPTS) {
			addToBlockedClientsList = true;
			//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
	else {
		// Reset after x seconds
		lastBadConnectionAttempt.first = 0;
		//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(addToBlockedClientsList == true) {
		serverSocket.addIPAddressToBlockedList(ipAddress);
		//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	lastBadConnectionAttempt.first++;
	lastBadConnectionAttempt.second = time(NULL);

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *serverInterfaceNode = rootNode->addChild("ServerInterface");

	for(int i= 0; exitServer == false && i < GameConstants::maxPlayers; ++i) {
		if(slots[i] != NULL) {
			MutexSafeWrapper safeMutex(slotAccessorMutexes[i],CODE_AT_LINE_X(i));

			XmlNode *slotNode = serverInterfaceNode->addChild("Slot");

			ConnectionSlot *slot = slots[i];
			if(slot != NULL) {
				slotNode->addAttribute("isconnected",intToStr(slot->isConnected()), mapTagReplacements);
				slotNode->addAttribute("sessionkey",intToStr(slot->getSessionKey()), mapTagReplacements);
				slotNode->addAttribute("ipaddress",slot->getSocket(false)->getIpAddress(), mapTagReplacements);
				slotNode->addAttribute("name",slot->getName(), mapTagReplacements);
			}
			else {
				slotNode->addAttribute("isconnected",intToStr(false), mapTagReplacements);
			}
		}
	}
}

}}//end namespace
