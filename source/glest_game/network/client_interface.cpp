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

#include "client_interface.h"

#include <stdexcept>
#include <cassert>

#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "map.h"
#include "config.h"
#include "logger.h"
#include "window.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

#ifdef WIN32

#define snprintf _snprintf

#endif

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout= 10000;	//10 seconds
const int ClientInterface::waitSleepTime= 10;
const int ClientInterface::maxNetworkCommandListSendTimeWait = 4;

ClientInterface::ClientInterface() : GameNetworkInterface() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] constructor for %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this);

	networkCommandListThreadAccessor = new Mutex();
	networkCommandListThread = NULL;
	cachedPendingCommandsIndex = 0;
	cachedLastPendingFrameCount = 0;

	this->pausedForInGameJoin = false;
	this->readyForInGameJoin = false;
	clientSocket= NULL;
	sessionKey = 0;
	launchGame= false;
	introDone= false;

	this->joinGameInProgress = false;
	this->joinGameInProgressLaunch = false;

	playerIndex= -1;
	setGameSettingsReceived(false);
	gotIntro = false;
	lastNetworkCommandListSendTime = 0;
	currentFrameCount = 0;
	clientSimulationLagStartTime = 0;

	networkGameDataSynchCheckOkMap  = false;
	networkGameDataSynchCheckOkTile = false;
	networkGameDataSynchCheckOkTech = false;
	this->setNetworkGameDataSynchCheckTechMismatchReport("");
	this->setReceivedDataSynchCheck(false);
}

void ClientInterface::shutdownNetworkCommandListThread() {
	//MutexSafeWrapper safeMutex(masterServerThreadAccessor,CODE_AT_LINE);

	if(networkCommandListThread != NULL) {
		time_t elapsed = time(NULL);
		networkCommandListThread->signalQuit();
		for(;networkCommandListThread->canShutdown(false) == false &&
			difftime((long int)time(NULL),elapsed) <= 15;) {
			//sleep(150);
		}
		if(networkCommandListThread->canShutdown(true)) {
			delete networkCommandListThread;
			networkCommandListThread = NULL;
		}
	}
}

ClientInterface::~ClientInterface() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] destructor for %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this);

	shutdownNetworkCommandListThread();

    if(clientSocket != NULL && clientSocket->isConnected() == true) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    	Lang &lang= Lang::getInstance();
    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			string sQuitText = "has chosen to leave the game!";
			if(lang.hasString("PlayerLeftGame",languageList[i]) == true) {
				sQuitText = lang.get("PlayerLeftGame",languageList[i]);
			}
			sendTextMessage(sQuitText,-1,false,languageList[i]);
    	}
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    close();
    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	delete clientSocket;
	clientSocket = NULL;

	delete networkCommandListThreadAccessor;
	networkCommandListThreadAccessor = NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ClientInterface::connect(const Ip &ip, int port) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

	this->ip    = ip;
	this->port  = port;

	delete clientSocket;
	clientSocket= new ClientSocket();
	clientSocket->setBlock(false);
	clientSocket->connect(ip, port);
	connectedTime = time(NULL);
	clientSocket->setBlock(true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END - socket = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,clientSocket->getSocketId());
}

void ClientInterface::reset() {
    if(getSocket() != NULL) {
    	Lang &lang= Lang::getInstance();
    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			string sQuitText = "has chosen to leave the game!";
			if(lang.hasString("PlayerLeftGame",languageList[i]) == true) {
				sQuitText = lang.get("PlayerLeftGame",languageList[i]);
			}
			sendTextMessage(sQuitText,-1,false,languageList[i]);
    	}
        close();
    }
}

void ClientInterface::update() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	bool wasConnected = this->isConnected();
	if(gotIntro == true && wasConnected == false) {
		string playerNameStr = getHumanPlayerName();

    	Lang &lang= Lang::getInstance();

		char szBuf1[8096]="";
		string statusTextFormat= lang.get("PlayerDisconnected");
		snprintf(szBuf1,8096,statusTextFormat.c_str(),playerNameStr.c_str());

    	//string sErr = "Disconnected from server during intro handshake.";
		DisplayErrorMessage(szBuf1);
        quit= true;
        return;
	}
	try {
		NetworkMessageCommandList networkMessageCommandList(currentFrameCount);

		//send as many commands as we can
		while(requestedCommands.empty() == false) {
			if(networkMessageCommandList.addCommand(&requestedCommands.back())) {
				requestedCommands.pop_back();
			}
			else {
				break;
			}
		}

		double lastSendElapsed = difftime((long int)time(NULL),lastNetworkCommandListSendTime);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

		if((currentFrameCount >= this->gameSettings.getNetworkFramePeriod() &&
				currentFrameCount % this->gameSettings.getNetworkFramePeriod() == 0) ||
				networkMessageCommandList.getCommandCount() > 0 ||
		  (lastNetworkCommandListSendTime > 0 && lastSendElapsed >= ClientInterface::maxNetworkCommandListSendTimeWait)) {
			lastNetworkCommandListSendTime = time(NULL);
			sendMessage(&networkMessageCommandList);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		}

		// Possible cause of out of synch since we have more commands that need
		// to be sent in this frame
		if(requestedCommands.empty() == false) {
			//char szBuf[4096]="";
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,requestedCommands.size());

			string sMsg = "may go out of synch: client requestedCommands.size() = " + intToStr(requestedCommands.size());
			sendTextMessage(sMsg,-1, true,"");
			sleep(1);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		}
	}
	catch(const megaglest_runtime_error &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		if(this->isConnected() == false) {
			if(gotIntro == false || wasConnected == false) {
				string sErr = string(extractFileFromDirectoryPath(__FILE__).c_str()) + "::" + string(__FUNCTION__) + " network error: " + string(ex.what());
				DisplayErrorMessage(sErr);
			}

			quit= true;
		}
		else {
			throw megaglest_runtime_error(ex.what());
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
}

std::string ClientInterface::getServerIpAddress() {
	return this->ip.getString();
}

void ClientInterface::updateLobby() {
    NetworkMessageType networkMessageType = getNextMessageType();
    switch(networkMessageType)
    {
        case nmtInvalid:
            break;

        case nmtIntro:
        {
            NetworkMessageIntro networkMessageIntro;
            if(receiveMessage(&networkMessageIntro)) {
            	gotIntro = true;
            	sessionKey = networkMessageIntro.getSessionId();
            	versionString = networkMessageIntro.getVersionString();
				playerIndex= networkMessageIntro.getPlayerIndex();
				serverName= networkMessageIntro.getName();
				serverFTPPort = networkMessageIntro.getFtpPort();
				this->joinGameInProgress = networkMessageIntro.getGameInProgress();
				this->joinGameInProgressLaunch = false;

				//printf("Client got intro playerIndex = %d\n",playerIndex);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got NetworkMessageIntro, networkMessageIntro.getGameState() = %d, versionString [%s], sessionKey = %d, playerIndex = %d, serverFTPPort = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageIntro.getGameState(),versionString.c_str(),sessionKey,playerIndex,serverFTPPort);

                //check consistency
				bool compatible = checkVersionComptability(networkMessageIntro.getVersionString(), getNetworkVersionSVNString());

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got NetworkMessageIntro, networkMessageIntro.getGameState() = %d, versionString [%s], sessionKey = %d, playerIndex = %d, serverFTPPort = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageIntro.getGameState(),versionString.c_str(),sessionKey,playerIndex,serverFTPPort);

				if(compatible == false) {
                //if(networkMessageIntro.getVersionString() != getNetworkVersionString()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

                	bool versionMatched = false;
                	string platformFreeVersion = getNetworkPlatformFreeVersionString();
                	string sErr = "";

                	if(strncmp(platformFreeVersion.c_str(),networkMessageIntro.getVersionString().c_str(),strlen(platformFreeVersion.c_str())) != 0) {
						string playerNameStr = getHumanPlayerName();
    					sErr = "Server and client binary mismatch!\nYou have to use the exactly same binaries!\n\nServer: " + networkMessageIntro.getVersionString() +
    							"\nClient: " + getNetworkVersionSVNString() + " player [" + playerNameStr + "]";
                        printf("%s\n",sErr.c_str());

                        sendTextMessage("Server and client binary mismatch!!",-1, true,"");
                        sendTextMessage(" Server:" + networkMessageIntro.getVersionString(),-1, true,"");
                        sendTextMessage(" Client: "+ getNetworkVersionSVNString(),-1, true,"");
						sendTextMessage(" Client player [" + playerNameStr + "]",-1, true,"");
                	}
                	else {
                		versionMatched = true;

						string playerNameStr = getHumanPlayerName();
						sErr = "Warning, Server and client are using the same version but different platforms.\n\nServer: " + networkMessageIntro.getVersionString() +
								"\nClient: " + getNetworkVersionSVNString() + " player [" + playerNameStr + "]";
						//printf("%s\n",sErr.c_str());
                	}

					if(Config::getInstance().getBool("PlatformConsistencyChecks","true") &&
					   versionMatched == false) { // error message and disconnect only if checked
						DisplayErrorMessage(sErr);
						sleep(1);

                        quit= true;
                        close();
                    	return;
            		}
                }

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

                if(networkMessageIntro.getGameState() == nmgstOk) {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					//send intro message
                	Lang &lang= Lang::getInstance();
					NetworkMessageIntro sendNetworkMessageIntro(
							sessionKey,getNetworkVersionSVNString(),
							getHumanPlayerName(),
							-1,
							nmgstOk,
							this->getSocket()->getConnectedIPAddress(),
							serverFTPPort,
							lang.getLanguage(),
							networkMessageIntro.getGameInProgress());
					sendMessage(&sendNetworkMessageIntro);

					//printf("Got intro sending client details to server\n");

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					if(clientSocket == NULL || clientSocket->isConnected() == false) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	                	string sErr = "Disconnected from server during intro handshake.";
						DisplayErrorMessage(sErr);
	                    quit= true;
	                    close();

	                    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	                	return;
					}
					else {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

						assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
						introDone= true;
					}
                }
                else if(networkMessageIntro.getGameState() == nmgstNoSlots) {
                	string sErr = "Cannot join the server because there are no open slots for new players.";
					DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                	return;
                }
                else {
                	string sErr = "Unknown response from server: " + intToStr(networkMessageIntro.getGameState());
					DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
                	return;
                }
            }
        }
        break;

		case nmtPing:
		{
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtPing\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

			NetworkMessagePing networkMessagePing;
			if(receiveMessage(&networkMessagePing)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				lastPingInfo = networkMessagePing;
			}
		}
		break;

        case nmtSynchNetworkGameData:
        {
            NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData;

            if(receiveMessage(&networkMessageSynchNetworkGameData)) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got NetworkMessageSynchNetworkGameData, getTechCRCFileCount() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageSynchNetworkGameData.getTechCRCFileCount());

            	networkGameDataSynchCheckOkMap      = false;
            	networkGameDataSynchCheckOkTile     = false;
            	networkGameDataSynchCheckOkTech     = false;
                this->setNetworkGameDataSynchCheckTechMismatchReport("");
                this->setReceivedDataSynchCheck(false);

				uint32 tilesetCRC = 0;
				uint32 techCRC	 = 0;
				uint32 mapCRC	 = 0;
				vector<std::pair<string,uint32> > vctFileList;

				try {
					Config &config = Config::getInstance();
					string scenarioDir = "";
					if(gameSettings.getScenarioDir() != "") {
						scenarioDir = gameSettings.getScenarioDir();
						if(EndsWith(scenarioDir, ".xml") == true) {
							scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
							scenarioDir = scenarioDir.erase(scenarioDir.size() - gameSettings.getScenario().size(), gameSettings.getScenario().size() + 1);
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings.getScenarioDir().c_str(),gameSettings.getScenario().c_str(),scenarioDir.c_str());
					}

					// check the checksum's
					tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + networkMessageSynchNetworkGameData.getTileset() + string("/*"), ".xml", NULL);

					this->setNetworkGameDataSynchCheckOkTile((tilesetCRC == networkMessageSynchNetworkGameData.getTilesetCRC()));
					//if(this->getNetworkGameDataSynchCheckOkTile() == false)
					//{
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] tilesetCRC info, local = %d, remote = %d, networkMessageSynchNetworkGameData.getTileset() = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,tilesetCRC,networkMessageSynchNetworkGameData.getTilesetCRC(),networkMessageSynchNetworkGameData.getTileset().c_str());
					//}

					//tech, load before map because of resources
					techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + networkMessageSynchNetworkGameData.getTech() + string("/*"), ".xml", NULL);

					this->setNetworkGameDataSynchCheckOkTech((techCRC == networkMessageSynchNetworkGameData.getTechCRC()));

					if(this->getNetworkGameDataSynchCheckOkTech() == false) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

						string pathSearchString = "/" + networkMessageSynchNetworkGameData.getTech() + "/*";
						vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),pathSearchString, ".xml", NULL);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

						string report = networkMessageSynchNetworkGameData.getTechCRCFileMismatchReport(vctFileList);
						this->setNetworkGameDataSynchCheckTechMismatchReport(report);

					}
					//if(this->getNetworkGameDataSynchCheckOkTech() == false)
					//{
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC info, local = %d, remote = %d, networkMessageSynchNetworkGameData.getTech() = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,techCRC,networkMessageSynchNetworkGameData.getTechCRC(),networkMessageSynchNetworkGameData.getTech().c_str());
					//}

					//map
					Checksum checksum;
					string file = Map::getMapPath(networkMessageSynchNetworkGameData.getMap(),scenarioDir, false);
					if(file != "") {
						checksum.addFile(file);
						mapCRC = checksum.getSum();
					}
					this->setNetworkGameDataSynchCheckOkMap((mapCRC == networkMessageSynchNetworkGameData.getMapCRC()));
					this->setReceivedDataSynchCheck(true);

					//if(this->getNetworkGameDataSynchCheckOkMap() == false)
					//{
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] mapCRC info, local = %d, remote = %d, file = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,mapCRC,networkMessageSynchNetworkGameData.getMapCRC(),file.c_str());
					//}
				}
				catch(const runtime_error &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
					string sErr = ex.what();
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error during processing, sErr = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sErr.c_str());

					DisplayErrorMessage(sErr);
				}

				NetworkMessageSynchNetworkGameDataStatus sendNetworkMessageSynchNetworkGameDataStatus(mapCRC,tilesetCRC,techCRC,vctFileList);
				sendMessage(&sendNetworkMessageSynchNetworkGameDataStatus);
            }
        }
        break;

        case nmtSynchNetworkGameDataFileCRCCheck:
        {
            NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck;
            if(receiveMessage(&networkMessageSynchNetworkGameDataFileCRCCheck))
            {
                Checksum checksum;
                string file = networkMessageSynchNetworkGameDataFileCRCCheck.getFileName();
                checksum.addFile(file);
                uint32 fileCRC = checksum.getSum();

                if(fileCRC != networkMessageSynchNetworkGameDataFileCRCCheck.getFileCRC())
                {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck localCRC = %d, remoteCRC = %d, file [%s]\n",
                        extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,fileCRC,
                        networkMessageSynchNetworkGameDataFileCRCCheck.getFileCRC(),
                        networkMessageSynchNetworkGameDataFileCRCCheck.getFileName().c_str());

                    // Here we initiate a download of missing or mismatched content

                    NetworkMessageSynchNetworkGameDataFileGet sendNetworkMessageSynchNetworkGameDataFileGet(networkMessageSynchNetworkGameDataFileCRCCheck.getFileName());
                    sendMessage(&sendNetworkMessageSynchNetworkGameDataFileGet);

                    FileTransferInfo fileInfo;
                    fileInfo.hostType   = eClient;
                    fileInfo.serverIP   = this->ip.getString();
                    fileInfo.serverPort = this->port;
                    fileInfo.fileName   = networkMessageSynchNetworkGameDataFileCRCCheck.getFileName();

                    FileTransferSocketThread *fileXferThread = new FileTransferSocketThread(fileInfo);
                    fileXferThread->start();
                }

                if(networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex() < networkMessageSynchNetworkGameDataFileCRCCheck.getTotalFileCount())
                {
                    NetworkMessageSynchNetworkGameDataFileCRCCheck sendNetworkMessageSynchNetworkGameDataFileCRCCheck(
                        networkMessageSynchNetworkGameDataFileCRCCheck.getTotalFileCount(),
                        networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex() + 1,
                        0,
                        "");
                    sendMessage(&sendNetworkMessageSynchNetworkGameDataFileCRCCheck);
                }
            }
        }
        break;

        case nmtText:
        {
            NetworkMessageText networkMessageText;
            if(receiveMessage(&networkMessageText))
            {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtText\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

        		ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),networkMessageText.getTargetLanguage());
        		this->addChatInfo(msg);
            }
        }
        break;

        case nmtMarkCell:
        {
        	NetworkMessageMarkCell networkMessageMarkCell;
            if(receiveMessage(&networkMessageMarkCell)) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

            	MarkedCell msg(networkMessageMarkCell.getTarget(),
            			       networkMessageMarkCell.getFactionIndex(),
            			       networkMessageMarkCell.getText().c_str(),
            			       networkMessageMarkCell.getPlayerIndex());
        		this->addMarkedCell(msg);
            }
        }
        break;
        case nmtUnMarkCell:
        {
        	NetworkMessageUnMarkCell networkMessageMarkCell;
            if(receiveMessage(&networkMessageMarkCell)) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

            	UnMarkedCell msg(networkMessageMarkCell.getTarget(),
            			       networkMessageMarkCell.getFactionIndex());
        		this->addUnMarkedCell(msg);
            }
        }
        break;
        case nmtHighlightCell:
        {
        	NetworkMessageHighlightCell networkMessageHighlightCell;
            if(receiveMessage(&networkMessageHighlightCell)) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtHighlightCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

            	MarkedCell msg(networkMessageHighlightCell.getTarget(),
            			networkMessageHighlightCell.getFactionIndex(),
            			       "none",-1);
        		this->setHighlightedCell(msg);
            }
        }
        break;

        case nmtLaunch:
        case nmtBroadCastSetup:
        {
        	//printf("#1 Got new game setup playerIndex = %d!\n",playerIndex);

            NetworkMessageLaunch networkMessageLaunch;
            if(receiveMessage(&networkMessageLaunch)) {
            	if(networkMessageLaunch.getMessageType() == nmtLaunch) {
            		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtLaunch\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            	}
            	else if(networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
            		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtBroadCastSetup\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            	}
            	else {
            		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());

					char szBuf[1024]="";
					snprintf(szBuf,1023,"In [%s::%s Line: %d] Invalid networkMessageLaunch.getMessageType() = %d",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
					throw megaglest_runtime_error(szBuf);
            	}

                networkMessageLaunch.buildGameSettings(&gameSettings);

                //printf("Client got game settings playerIndex = %d lookingfor match...\n",playerIndex);

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
                //replace server player by network
                for(int i= 0; i<gameSettings.getFactionCount(); ++i) {

                	//printf("Faction = %d start location = %d faction name = %s\n",i,gameSettings.getStartLocationIndex(i),gameSettings.getFactionTypeName(i).c_str());

                    //replace by network
                    if(gameSettings.getFactionControl(i)==ctHuman) {
                        gameSettings.setFactionControl(i, ctNetwork);
                    }

					//printf("i = %d gameSettings.getStartLocationIndex(i) = %d playerIndex = %d, gameSettings.getFactionControl(i) = %d\n",i,gameSettings.getStartLocationIndex(i),playerIndex,gameSettings.getFactionControl(i));

					//set the faction index
					if(gameSettings.getStartLocationIndex(i) == playerIndex) {
						//printf("Setting my factionindex to: %d for playerIndex: %d\n",i,playerIndex);

                        gameSettings.setThisFactionIndex(i);

                        //printf("Client got game settings playerIndex = %d factionIndex = %d control = %d name = %s\n",playerIndex,i,gameSettings.getFactionControl(i),gameSettings.getFactionTypeName(i).c_str());
                        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getThisFactionIndex(i) = %d, playerIndex = %d, i = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings.getThisFactionIndex(),playerIndex,i);
                    }
                }

                if(networkMessageLaunch.getMessageType() == nmtLaunch) {
                	launchGame= true;
                }
                else if(networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
                	setGameSettingsReceived(true);
                }
            }
        }
        break;
		case nmtPlayerIndexMessage:
		{
			PlayerIndexMessage playerIndexMessage(-1);
			if(receiveMessage(&playerIndexMessage)) {
				playerIndex= playerIndexMessage.getPlayerIndex();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtPlayerIndexMessage, playerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex);
            }

			//printf("Got player index changed msg: %d\n",playerIndex);
		}
		break;

		case nmtReady:
		{
			NetworkMessageReady networkMessageReady;
			if(receiveMessage(&networkMessageReady)) {
				this->readyForInGameJoin = true;
			}
		}
		break;

		case nmtCommandList:
			{

			int waitCount = 0;
			//make sure we read the message
			time_t receiveTimeElapsed = time(NULL);
			NetworkMessageCommandList networkMessageCommandList;
			bool gotCmd = receiveMessage(&networkMessageCommandList);
			if(gotCmd == false) {
				throw megaglest_runtime_error("error retrieving nmtCommandList returned false!");
			}
			this->pausedForInGameJoin = true;
		}
		break;

        default:
            {
            string sErr = string(extractFileFromDirectoryPath(__FILE__).c_str()) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType);
            //throw megaglest_runtime_error(string(extractFileFromDirectoryPath(__FILE__).c_str()) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));
            sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1, true,"");
            DisplayErrorMessage(sErr);
            sleep(1);

            quit= true;
            close();
            }
    }

	if( clientSocket != NULL && clientSocket->isConnected() == true &&
		gotIntro == false && difftime((long int)time(NULL),connectedTime) > GameConstants::maxClientConnectHandshakeSecs) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] difftime(time(NULL),connectedTime) = %f\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,difftime((long int)time(NULL),connectedTime));
		close();
	}
}

void ClientInterface::updateFrame(int *checkFrame) {
	//printf("#1 ClientInterface::updateFrame\n");

	if(isConnected() == true && quit == false) {
		//printf("#2 ClientInterface::updateFrame\n");

		Chrono chrono;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

		int simulateLag = Config::getInstance().getInt("SimulateClientLag","0");
		bool done= false;
		while(done == false) {

			//printf("BEFORE Client get networkMessageType\n");

			//wait for the next message
			NetworkMessageType networkMessageType = waitForMessage();

			//printf("AFTER Client got networkMessageType = %d\n",networkMessageType);

			// START: Test simulating lag for the client
			if(simulateLag > 0) {
				if(clientSimulationLagStartTime == 0) {
					clientSimulationLagStartTime = time(NULL);
				}
				if(difftime((long int)time(NULL),clientSimulationLagStartTime) <= Config::getInstance().getInt("SimulateClientLagDurationSeconds","0")) {
					sleep(Config::getInstance().getInt("SimulateClientLag","0"));
				}
			}
			// END: Test simulating lag for the client

			//check we have an expected message
			//NetworkMessageType networkMessageType= getNextMessageType();

			switch(networkMessageType)
			{
				case nmtCommandList:
					{

					int waitCount = 0;
					//make sure we read the message
					time_t receiveTimeElapsed = time(NULL);
					NetworkMessageCommandList networkMessageCommandList;
					bool gotCmd = receiveMessage(&networkMessageCommandList);
					if(gotCmd == false) {
						throw megaglest_runtime_error("error retrieving nmtCommandList returned false!");
					}

	//				while(receiveMessage(&networkMessageCommandList) == false &&
	//					  isConnected() == true &&
	//					  difftime(time(NULL),receiveTimeElapsed) <= (messageWaitTimeout / 2000)) {
	//					waitCount++;
	//				}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] receiveMessage took %lld msecs, waitCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis(),waitCount);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

					MutexSafeWrapper safeMutex(networkCommandListThreadAccessor,CODE_AT_LINE);
					cachedLastPendingFrameCount = networkMessageCommandList.getFrameCount();
					//printf("cachedLastPendingFrameCount = %lld\n",(long long int)cachedLastPendingFrameCount);

					//check that we are in the right frame
					if(checkFrame != NULL) {
						if(networkMessageCommandList.getFrameCount() != *checkFrame) {
							string sErr = "Player: " + getHumanPlayerName() +
										  " got a Network synchronization error, frame counts do not match, server frameCount = " +
										  intToStr(networkMessageCommandList.getFrameCount()) + ", local frameCount = " +
										  intToStr(*checkFrame);
							sendTextMessage(sErr,-1, true,"");
							DisplayErrorMessage(sErr);
							sleep(1);

							quit= true;
							close();
							return;
						}
					}

					cachedPendingCommands[networkMessageCommandList.getFrameCount()].reserve(networkMessageCommandList.getCommandCount());

					// give all commands
					for(int i= 0; i < networkMessageCommandList.getCommandCount(); ++i) {
						//pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
						cachedPendingCommands[networkMessageCommandList.getFrameCount()].push_back(*networkMessageCommandList.getCommand(i));
					}
					safeMutex.ReleaseLock();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] transfer network commands took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

					done= true;
				}
				break;

				case nmtPing:
				{
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtPing\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

					NetworkMessagePing networkMessagePing;
					if(receiveMessage(&networkMessagePing)) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						lastPingInfo = networkMessagePing;
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
				}
				break;

				case nmtQuit:
				{
					time_t receiveTimeElapsed = time(NULL);
					NetworkMessageQuit networkMessageQuit;
	//				while(receiveMessage(&networkMessageQuit) == false &&
	//					  isConnected() == true &&
	//					  difftime(time(NULL),receiveTimeElapsed) <= (messageWaitTimeout / 2000)) {
	//				}
					bool gotCmd = receiveMessage(&networkMessageQuit);
					if(gotCmd == false) {
						throw megaglest_runtime_error("error retrieving nmtQuit returned false!");
					}

					quit= true;
					done= true;
				}
				break;

				case nmtText:
				{
					//time_t receiveTimeElapsed = time(NULL);
					NetworkMessageText networkMessageText;
					bool gotCmd = receiveMessage(&networkMessageText);
					if(gotCmd == false) {
						throw megaglest_runtime_error("error retrieving nmtText returned false!");
					}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

					ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),networkMessageText.getTargetLanguage());
					this->addChatInfo(msg);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
				}
				break;

		        case nmtMarkCell:
		        {
		        	NetworkMessageMarkCell networkMessageMarkCell;
		            if(receiveMessage(&networkMessageMarkCell)) {
		            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

		            	MarkedCell msg(networkMessageMarkCell.getTarget(),
		            			       networkMessageMarkCell.getFactionIndex(),
		            			       networkMessageMarkCell.getText().c_str(),
		            			       networkMessageMarkCell.getPlayerIndex());
		        		this->addMarkedCell(msg);
		            }
		        }
		        break;

		        case nmtUnMarkCell:
		        {
		        	NetworkMessageUnMarkCell networkMessageMarkCell;
		            if(receiveMessage(&networkMessageMarkCell)) {
		            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

		            	UnMarkedCell msg(networkMessageMarkCell.getTarget(),
		            			       networkMessageMarkCell.getFactionIndex());
		        		this->addUnMarkedCell(msg);
		            }
		        }
		        break;
		        case nmtHighlightCell:
		        {
		        	NetworkMessageHighlightCell networkMessageHighlightCell;
		            if(receiveMessage(&networkMessageHighlightCell)) {
		            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtHighlightCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);

		            	MarkedCell msg(networkMessageHighlightCell.getTarget(),
		            			networkMessageHighlightCell.getFactionIndex(),
		            			       "none",-1);
		        		this->setHighlightedCell(msg);
		            }
		        }
		        break;


				case nmtLaunch:
				case nmtBroadCastSetup:
				{
					//printf("#2 Got new game setup playerIndex = %d!\n",playerIndex);

					NetworkMessageLaunch networkMessageLaunch;
					if(receiveMessage(&networkMessageLaunch)) {
						if(networkMessageLaunch.getMessageType() == nmtLaunch) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtLaunch\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						}
						else if(networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtBroadCastSetup\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						}
						else {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());

							char szBuf[1024]="";
							snprintf(szBuf,1023,"In [%s::%s Line: %d] Invalid networkMessageLaunch.getMessageType() = %d",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
							throw megaglest_runtime_error(szBuf);
						}

						networkMessageLaunch.buildGameSettings(&gameSettings);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
						//replace server player by network
						for(int i= 0; i<gameSettings.getFactionCount(); ++i) {
							//replace by network
							if(gameSettings.getFactionControl(i)==ctHuman) {
								gameSettings.setFactionControl(i, ctNetwork);
							}

							//printf("i = %d gameSettings.getStartLocationIndex(i) = %d playerIndex = %d!\n",i,gameSettings.getStartLocationIndex(i),playerIndex);

							//set the faction index
							if(gameSettings.getStartLocationIndex(i) == playerIndex) {
								//printf("Setting my factionindex to: %d for playerIndex: %d\n",i,playerIndex);

								gameSettings.setThisFactionIndex(i);
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getThisFactionIndex(i) = %d, playerIndex = %d, i = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings.getThisFactionIndex(),playerIndex,i);
							}
						}

						//if(networkMessageLaunch.getMessageType() == nmtLaunch) {
							//launchGame= true;
						//}
						//else if(networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
						//	setGameSettingsReceived(true);
						//}
					}
				}
				break;


				case nmtLoadingStatusMessage:
					break;

				case nmtInvalid:
					break;

				default:
					{
					sendTextMessage("Unexpected message in client interface: " + intToStr(networkMessageType),-1, true,"");
					DisplayErrorMessage(string(extractFileFromDirectoryPath(__FILE__).c_str()) + "::" + string(__FUNCTION__) + " Unexpected message in client interface: " + intToStr(networkMessageType));
					sleep(1);

					quit= true;
					close();
					done= true;
					}
					break;
			}

			if(isConnected() == false && quit == true) {
				done = true;
			}
		}

		if(done == true) {
			MutexSafeWrapper safeMutex(networkCommandListThreadAccessor,CODE_AT_LINE);
			cachedPendingCommandsIndex++;
		}
		else {
			if(checkFrame == NULL) {
				//sleep(15);
				sleep(0);
			}
		}
	}
	//printf("#3 ClientInterface::updateFrame\n");
}

uint64 ClientInterface::getCachedLastPendingFrameCount() {
	MutexSafeWrapper safeMutex(networkCommandListThreadAccessor,CODE_AT_LINE);
	uint64 result = cachedLastPendingFrameCount;
	safeMutex.ReleaseLock();

	return result;
}

void ClientInterface::simpleTask(BaseThread *callingThread) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	//printf("START === Client thread ended\n");

	while(callingThread->getQuitStatus() == false && quit == false) {
		updateFrame(NULL);
	}

	//printf("END === Client thread ended\n");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
}

bool ClientInterface::getNetworkCommand(int frameCount, int currentCachedPendingCommandsIndex) {
	bool result = false;

	bool waitForData = false;
	if(quit == false) {
		MutexSafeWrapper safeMutex(networkCommandListThreadAccessor,CODE_AT_LINE);
		safeMutex.ReleaseLock(true);

		for(;quit == false;) {
			safeMutex.Lock();
			uint64 copyCachedLastPendingFrameCount = cachedLastPendingFrameCount;
			if(cachedPendingCommands.find(frameCount) != cachedPendingCommands.end()) {
				Commands &frameCmdList = cachedPendingCommands[frameCount];
				if(frameCmdList.size() > 0) {
					for(int i= 0; i < frameCmdList.size(); ++i) {
						pendingCommands.push_back(frameCmdList[i]);
					}
					//cachedPendingCommands.erase(frameCount);
					cachedPendingCommands[frameCount].clear();
				}
				safeMutex.ReleaseLock(true);

				result = true;
				break;
			}
			else {
				safeMutex.ReleaseLock(true);
				// No data for this frame
				//if(cachedPendingCommandsIndex > currentCachedPendingCommandsIndex) {
				//	break;
				//}

				if(waitForData == false) {
					printf("Client waiting for packet for frame: %d, copyCachedLastPendingFrameCount = %lld\n",frameCount,(long long int)copyCachedLastPendingFrameCount);
				}
				if(copyCachedLastPendingFrameCount > frameCount) {
					break;
				}

				if(waitForData == false) {
					waitForData = true;
					sleep(1);
				}

				//printf("Client waiting for packet for frame: %d, currentCachedPendingCommandsIndex = %d, cachedPendingCommandsIndex = %lld\n",frameCount,currentCachedPendingCommandsIndex,(long long int)cachedPendingCommandsIndex);
			}
		}
	}

	return result;
}

void ClientInterface::updateKeyframe(int frameCount) {
	currentFrameCount = frameCount;

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
	//chrono.start();

	if(quit == false) {
		bool testThreaded = Config::getInstance().getBool("ThreadedNetworkClient","true");
		//bool testThreaded = false;
		if(testThreaded == false) {
			updateFrame(&frameCount);
			Commands &frameCmdList = cachedPendingCommands[frameCount];
			for(int i= 0; i < frameCmdList.size(); ++i) {
				pendingCommands.push_back(frameCmdList[i]);
			}
			cachedPendingCommands.erase(frameCount);
		}
		else {
			if(networkCommandListThread == NULL) {
				static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
				networkCommandListThread = new SimpleTaskThread(this,0,0);
				networkCommandListThread->setUniqueID(mutexOwnerId);
				networkCommandListThread->start();
				sleep(0);
			}

			getNetworkCommand(frameCount,cachedPendingCommandsIndex);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	//printf("In [%s::%s Line: %d] took %lld msecs\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
}

bool ClientInterface::isMasterServerAdminOverride() {
	return (gameSettings.getMasterserver_admin() == this->getSessionKey());
}

void ClientInterface::waitUntilReady(Checksum* checksum) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	bool signalServerWhenReadyToStartJoinedGame = this->readyForInGameJoin;
	this->pausedForInGameJoin = false;
	this->readyForInGameJoin = false;
    Logger &logger= Logger::getInstance();

	Chrono chrono;
	chrono.start();

	// FOR TESTING ONLY - delay to see the client count up while waiting
	//sleep(5000);

	//clientSocket->setBlock(true);
	//send ready message
	NetworkMessageReady networkMessageReady;
	sendMessage(&networkMessageReady);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	NetworkMessageLoadingStatus networkMessageLoadingStatus(nmls_NONE);

	Lang &lang= Lang::getInstance();
    int64 lastMillisCheck = 0;

	uint64 waitLoopIterationCount = 0;
	uint64 MAX_LOOP_COUNT_BEFORE_SLEEP = 100;
	MAX_LOOP_COUNT_BEFORE_SLEEP = Config::getInstance().getInt("NetworkClientLoopGameLoadingCap",intToStr(MAX_LOOP_COUNT_BEFORE_SLEEP).c_str());
	int sleepMillis = Config::getInstance().getInt("NetworkClientLoopGameLoadingCapSleepMillis","10");

	//wait until we get a ready message from the server
	while(true)	{
		// FOR TESTING ONLY - delay to see the client count up while waiting
		//sleep(2000);

		waitLoopIterationCount++;
		if(waitLoopIterationCount > 0 && waitLoopIterationCount % MAX_LOOP_COUNT_BEFORE_SLEEP == 0) {
			sleep(sleepMillis);
			waitLoopIterationCount = 0;
		}

		if(isConnected() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			string sErr = "Error, Server has disconnected!";
            DisplayErrorMessage(sErr);

            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

            quit= true;
            close();

            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
            return;
		}
		NetworkMessageType networkMessageType = getNextMessageType();

		// consume old messages from the lobby
		bool discarded = shouldDiscardNetworkMessage(networkMessageType);
		if(discarded == false) {
			if(networkMessageType == nmtReady) {
				if(receiveMessage(&networkMessageReady)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					break;
				}
			}
			else if(networkMessageType == nmtLoadingStatusMessage) {
				if(receiveMessage(&networkMessageLoadingStatus)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				}
			}
			else if(networkMessageType == nmtQuit) {
				NetworkMessageQuit networkMessageQuit;
				if(receiveMessage(&networkMessageQuit)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					DisplayErrorMessage(lang.get("GameCancelledByUser"));

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					quit= true;
					close();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					return;

				}
			}
			else if(networkMessageType == nmtCommandList) {
				int waitCount = 0;
				//make sure we read the message
				time_t receiveTimeElapsed = time(NULL);
				NetworkMessageCommandList networkMessageCommandList;
				bool gotCmd = receiveMessage(&networkMessageCommandList);
				if(gotCmd == false) {
					throw megaglest_runtime_error("error retrieving nmtCommandList returned false!");
				}
			}
			else if(networkMessageType == nmtInvalid) {
				if(chrono.getMillis() > readyWaitTimeout) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			    	Lang &lang= Lang::getInstance();
			    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
			    	for(unsigned int i = 0; i < languageList.size(); ++i) {
			    		string sErr = "Timeout waiting for server";
						if(lang.hasString("TimeoutWaitingForServer",languageList[i]) == true) {
							sErr = lang.get("TimeoutWaitingForServer",languageList[i]);
						}
						bool echoLocal = lang.isLanguageLocal(lang.getLanguage());
						sendTextMessage(sErr,-1,echoLocal,languageList[i]);

						if(echoLocal) {
							DisplayErrorMessage(sErr);
						}
			    	}

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					sleep(1);
					quit= true;
					close();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					return;
				}
				else {
					//if(chrono.getMillis() / 1000 > lastMillisCheck) {
					if(chrono.getMillis() % 100 == 0) {
						lastMillisCheck = (chrono.getMillis() / 1000);

						char szBuf[8096]="";
						string updateTextFormat = "Waiting for network: %lld seconds elapsed (maximum wait time: %d seconds)";
						if(lang.hasString("NetworkGameClientLoadStatus") == true) {
							updateTextFormat =  lang.get("NetworkGameClientLoadStatus");
						}

						string waitForHosts = "";
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER1_CONNECTED) == nmls_PLAYER1_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER1_READY) != nmls_PLAYER1_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(0);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER2_CONNECTED) == nmls_PLAYER2_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER2_READY) != nmls_PLAYER2_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(1);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER3_CONNECTED) == nmls_PLAYER3_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER3_READY) != nmls_PLAYER3_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(2);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER4_CONNECTED) == nmls_PLAYER4_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER4_READY) != nmls_PLAYER4_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(3);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER5_CONNECTED) == nmls_PLAYER5_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER5_READY) != nmls_PLAYER5_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(4);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER6_CONNECTED) == nmls_PLAYER6_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER6_READY) != nmls_PLAYER6_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(5);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER7_CONNECTED) == nmls_PLAYER7_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER7_READY) != nmls_PLAYER7_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(6);
							}
						}
						if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER8_CONNECTED) == nmls_PLAYER8_CONNECTED) {
							if((networkMessageLoadingStatus.getStatus() & nmls_PLAYER8_READY) != nmls_PLAYER8_READY) {
								if(waitForHosts != "") {
									waitForHosts += ", ";
								}
								waitForHosts += gameSettings.getNetworkPlayerNameByPlayerIndex(7);
							}
						}

						if(waitForHosts == "") {
							waitForHosts = lang.get("Server");
						}
						snprintf(szBuf,8096,updateTextFormat.c_str(),(long long int)lastMillisCheck,int(readyWaitTimeout / 1000));

						char szBuf1[8096]="";
						string statusTextFormat =  "Waiting for players: %s";
						if(lang.hasString("NetworkGameStatusWaiting") == true) {
							statusTextFormat = lang.get("NetworkGameStatusWaiting");
						}
						snprintf(szBuf1,8096,statusTextFormat.c_str(),waitForHosts.c_str());

						logger.add(szBuf, true, szBuf1);

						sleep(0);
					}
				}
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1, true,"");

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				DisplayErrorMessage(string(extractFileFromDirectoryPath(__FILE__).c_str()) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				sleep(1);
				quit= true;
				close();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				return;
			}

			Shared::Platform::Window::handleEvent();
			// sleep a bit
			sleep(waitSleepTime);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//check checksum
	if(this->joinGameInProgress == false && networkMessageReady.getChecksum() != checksum->getSum()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    	Lang &lang= Lang::getInstance();
    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
    		string sErr = "Checksum error, you don't have the same data as the server";
			if(lang.hasString("CheckSumGameLoadError",languageList[i]) == true) {
				sErr = lang.get("CheckSumGameLoadError",languageList[i]);
			}
			bool echoLocal = lang.isLanguageLocal(lang.getLanguage());
			sendTextMessage(sErr,-1,echoLocal,languageList[i]);

			string playerNameStr = "Player with error is: " + getHumanPlayerName();
			if(lang.hasString("CheckSumGameLoadPlayer",languageList[i]) == true) {
				playerNameStr = lang.get("CheckSumGameLoadPlayer",languageList[i]) + " " + getHumanPlayerName();
			}
			sendTextMessage(playerNameStr,-1,echoLocal,languageList[i]);

    		string sErr1 = "Client Checksum: " + intToStr(checksum->getSum());
			if(lang.hasString("CheckSumGameLoadClient",languageList[i]) == true) {
				sErr1 = lang.get("CheckSumGameLoadClient",languageList[i]) + " " + intToStr(checksum->getSum());
			}

			sendTextMessage(sErr1,-1,echoLocal,languageList[i]);

    		string sErr2 = "Server Checksum: " + intToStr(networkMessageReady.getChecksum());
			if(lang.hasString("CheckSumGameLoadServer",languageList[i]) == true) {
				sErr2 = lang.get("CheckSumGameLoadServer",languageList[i]) + " " + intToStr(networkMessageReady.getChecksum());
			}
			sendTextMessage(sErr2,-1,echoLocal,languageList[i]);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d %s %s %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sErr.c_str(),sErr1.c_str(),sErr2.c_str());

			if(echoLocal == true) {
				if(Config::getInstance().getBool("NetworkConsistencyChecks")) {
					// error message and disconnect only if checked
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					string niceError = sErr + string("\n") + sErr1 + string("\n") + sErr2;
					DisplayErrorMessage(niceError);
				}
			}
    	}

//		string sErr = "Checksum error, you don't have the same data as the server";
//		CheckSumGameLoadError
//        sendTextMessage(sErr,-1, true);
//
//		string playerNameStr = "Player with error is [" + getHumanPlayerName() + "]";
//		sendTextMessage(playerNameStr,-1, true);
//
//		string sErr1 = "Client Checksum: " + intToStr(checksum->getSum());
//        sendTextMessage(sErr1,-1, true);
//
//		string sErr2 = "Server Checksum: " + intToStr(networkMessageReady.getChecksum());
//        sendTextMessage(sErr2,-1, true);

//        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d %s %s %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sErr.c_str(),sErr1.c_str(),sErr2.c_str());

		if(Config::getInstance().getBool("NetworkConsistencyChecks")) {
			// error message and disconnect only if checked
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			//string niceError = sErr + string("\n") + sErr1 + string("\n") + sErr2;
			//DisplayErrorMessage(niceError);

			sleep(1);
			quit= true;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			close();
       	}
        return;
	}

	this->joinGameInProgress = false;
	this->joinGameInProgressLaunch = false;

	//printf("Client signalServerWhenReadyToStartJoinedGame = %d\n",signalServerWhenReadyToStartJoinedGame);
	if(signalServerWhenReadyToStartJoinedGame == true) {
		this->resumeInGameJoin = true;
	}
	else {
		// delay the start a bit, so clients have more room to get messages
		// This is to ensure clients don't start ahead of the server and thus
		// constantly freeze because they are waiting for the server to catch up
		sleep(120);
	}

	// This triggers LAG update packets to begin as required
	lastNetworkCommandListSendTime = time(NULL);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);
}

void ClientInterface::sendResumeGameMessage() {
	NetworkMessageReady networkMessageReady;
	sendMessage(&networkMessageReady);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex, bool echoLocal,
		string targetLanguage) {

	string humanPlayerName = getHumanPlayerName();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,humanPlayerName.c_str(),playerIndex);

	NetworkMessageText networkMessageText(text, teamIndex,playerIndex,targetLanguage);
	sendMessage(&networkMessageText);

	if(echoLocal == true) {
		ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),targetLanguage);
		this->addChatInfo(msg);
	}
}

void ClientInterface::sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note,int playerIndex) {
	string humanPlayerName = getHumanPlayerName();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,humanPlayerName.c_str(),playerIndex);

	NetworkMessageMarkCell networkMessageMarkCell(targetPos,factionIndex, note,playerIndex);
	sendMessage(&networkMessageMarkCell);
}

void ClientInterface::sendHighlightCellMessage(Vec2i targetPos, int factionIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,playerIndex);

	NetworkMessageHighlightCell networkMessageHighlightCell(targetPos,factionIndex);
	sendMessage(&networkMessageHighlightCell);
}

void ClientInterface::sendUnMarkCellMessage(Vec2i targetPos, int factionIndex) {
	string humanPlayerName = getHumanPlayerName();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,humanPlayerName.c_str(),playerIndex);

	NetworkMessageUnMarkCell networkMessageMarkCell(targetPos,factionIndex);
	sendMessage(&networkMessageMarkCell);
}

void ClientInterface::sendPingMessage(int32 pingFrequency, int64 pingTime) {
	NetworkMessagePing networkMessagePing(pingFrequency,pingTime);
	sendMessage(&networkMessagePing);
}

string ClientInterface::getNetworkStatus() {
	std::string label = Lang::getInstance().get("Server") + ": " + serverName;
	//float pingTime = getThreadedPingMS(getServerIpAddress().c_str());
	char szBuf[8096]="";
	snprintf(szBuf,8096,"%s",label.c_str());

	return szBuf;
}

NetworkMessageType ClientInterface::waitForMessage()
{
	// Debug!
/*
    sendTextMessage("Timeout waiting for message",-1);
    DisplayErrorMessage("Timeout waiting for message");
    quit= true;
    close();
    return;
*/

	Chrono chrono;
	chrono.start();

	NetworkMessageType msg = nmtInvalid;
	//uint64 waitLoopCount = 0;
	while(msg == nmtInvalid) {
		msg = getNextMessageType();
		if(msg == nmtInvalid) {
			if(chrono.getMillis() % 250 == 0 && isConnected() == false) {
				if(quit == false) {
					//throw megaglest_runtime_error("Disconnected");
					//sendTextMessage("Server has Disconnected.",-1);
					DisplayErrorMessage("Server has Disconnected.");
					quit= true;
				}
				close();
				return msg;
			}

			if(chrono.getMillis() > messageWaitTimeout) {
			//if(1) {
				//throw megaglest_runtime_error("Timeout waiting for message");

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				Lang &lang= Lang::getInstance();
		    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
		    	for(unsigned int i = 0; i < languageList.size(); ++i) {
		    		string msg = "Timeout waiting for message.";
		    		if(lang.hasString("TimeoutWaitingForMessage",languageList[i]) == true) {
		    			msg = lang.get("TimeoutWaitingForMessage",languageList[i]);
		    		}

					sendTextMessage(msg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
					if(lang.isLanguageLocal(languageList[i]) == true) {
						DisplayErrorMessage(msg);
					}
		    	}
		    	sleep(1);
				quit= true;
				close();
				return msg;
			}
			// Sleep ever second we wait to let other threads work
			//else if(chrono.getMillis() % 1000 == 0) {
				//sleep(0);
			//}

			//sleep(waitSleepTime);
		}
		//waitLoopCount++;
	}

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] waiting took %lld msecs, waitLoopCount = %ull, msg = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis(),waitLoopCount,msg);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] waiting took %lld msecs, msg = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis(),msg);

	return msg;
}

void ClientInterface::quitGame(bool userManuallyQuit)
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] userManuallyQuit = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,userManuallyQuit);

    if(clientSocket != NULL && userManuallyQuit == true)
    {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		Lang &lang= Lang::getInstance();
    	const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
    	for(unsigned int i = 0; i < languageList.size(); ++i) {
    		string msg = "has chosen to leave the game!";
    		if(lang.hasString("PlayerLeftGame",languageList[i]) == true) {
    			msg = lang.get("PlayerLeftGame",languageList[i]);
    		}

			sendTextMessage(msg,-1, lang.isLanguageLocal(languageList[i]),languageList[i]);
    	}
    	sleep(1);
        close();
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ClientInterface::close()
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] START, clientSocket = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,clientSocket);

	delete clientSocket;
	clientSocket= NULL;

	connectedTime = 0;
	gotIntro = false;

	this->joinGameInProgress = false;
	this->joinGameInProgressLaunch = false;
	this->pausedForInGameJoin = false;
	this->readyForInGameJoin = false;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ClientInterface::discoverServers(DiscoveredServersInterface *cb) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	ClientSocket::discoverServers(cb);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}
void ClientInterface::stopServerDiscovery() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	ClientSocket::stopBroadCastClientThread();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ClientInterface::sendSwitchSetupRequest(string selectedFactionName, int8 currentSlotIndex,
											int8 toSlotIndex,int8 toTeam, string networkPlayerName,
											int8 networkPlayerStatus, int8 flags,
											string language) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkPlayerName [%s] flags = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkPlayerName.c_str(),flags);
	SwitchSetupRequest message=SwitchSetupRequest(selectedFactionName,
			currentSlotIndex, toSlotIndex,toTeam,networkPlayerName,
			networkPlayerStatus, flags,language);
	sendMessage(&message);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

bool ClientInterface::shouldDiscardNetworkMessage(NetworkMessageType networkMessageType) {
	bool discard = false;

	switch(networkMessageType) {
		case nmtIntro:
			{
			discard = true;
			NetworkMessageIntro msg = NetworkMessageIntro();
			this->receiveMessage(&msg);
			}
			break;
		case nmtPing:
			{
			discard = true;
			NetworkMessagePing msg = NetworkMessagePing();
			this->receiveMessage(&msg);
			lastPingInfo = msg;
			}
			break;
		case nmtLaunch:
			{
			discard = true;
			NetworkMessageLaunch msg = NetworkMessageLaunch();
			this->receiveMessage(&msg);
			}
			break;
		case nmtText:
			{
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtText\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			discard = true;
			NetworkMessageText netMsg = NetworkMessageText();
			this->receiveMessage(&netMsg);

    		ChatMsgInfo msg(netMsg.getText().c_str(),netMsg.getTeamIndex(),netMsg.getPlayerIndex(),netMsg.getTargetLanguage());
    		this->addChatInfo(msg);
			}
			break;

        case nmtMarkCell:
			{
			discard = true;
			NetworkMessageMarkCell networkMessageMarkCell;
			receiveMessage(&networkMessageMarkCell);

			MarkedCell msg(networkMessageMarkCell.getTarget(),
						   networkMessageMarkCell.getFactionIndex(),
						   networkMessageMarkCell.getText().c_str(),
						   networkMessageMarkCell.getPlayerIndex());
			this->addMarkedCell(msg);
			}
			break;

        case nmtUnMarkCell:
			{
			discard = true;
			NetworkMessageUnMarkCell networkMessageMarkCell;
			receiveMessage(&networkMessageMarkCell);

			UnMarkedCell msg(networkMessageMarkCell.getTarget(),
						   networkMessageMarkCell.getFactionIndex());
			this->addUnMarkedCell(msg);
			}
			break;

        case nmtHighlightCell:
			{
			discard = true;
			NetworkMessageHighlightCell networkMessageHighlightCell;
			receiveMessage(&networkMessageHighlightCell);

			MarkedCell msg(networkMessageHighlightCell.getTarget(),
					networkMessageHighlightCell.getFactionIndex(),
						   "none",-1);
			this->setHighlightedCell(msg);
			}
			break;

		case nmtSynchNetworkGameData:
			{
			discard = true;
			NetworkMessageSynchNetworkGameData msg = NetworkMessageSynchNetworkGameData();
			this->receiveMessage(&msg);
			}
			break;
		case nmtSynchNetworkGameDataStatus:
			{
			discard = true;
			NetworkMessageSynchNetworkGameDataStatus msg = NetworkMessageSynchNetworkGameDataStatus();
			this->receiveMessage(&msg);
			}
			break;
		case nmtSynchNetworkGameDataFileCRCCheck:
			{
			discard = true;
			NetworkMessageSynchNetworkGameDataFileCRCCheck msg = NetworkMessageSynchNetworkGameDataFileCRCCheck();
			this->receiveMessage(&msg);
			}
			break;
		case nmtSynchNetworkGameDataFileGet:
			{
			discard = true;
			NetworkMessageSynchNetworkGameDataFileGet msg = NetworkMessageSynchNetworkGameDataFileGet();
			this->receiveMessage(&msg);
			}
			break;
		case nmtSwitchSetupRequest:
			{
			discard = true;
			SwitchSetupRequest msg = SwitchSetupRequest();
			this->receiveMessage(&msg);
			}
			break;
		case nmtBroadCastSetup:
			{
			discard = true;
			NetworkMessageLaunch msg = NetworkMessageLaunch();
			this->receiveMessage(&msg);
			}
			break;

		case nmtPlayerIndexMessage:
			{
			discard = true;
			PlayerIndexMessage msg = PlayerIndexMessage(0);
			this->receiveMessage(&msg);
			}
			break;
	}

	return discard;
}

string ClientInterface::getHumanPlayerName(int index) {
	string  result = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str());

	//printf("Client getHumanPlayerName index = %d, gameSettings.getThisFactionIndex() = %d\n",index,gameSettings.getThisFactionIndex());

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


void ClientInterface::setGameSettings(GameSettings *serverGameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);
	//MutexSafeWrapper safeMutex(&serverSynchAccessor,string(extractFileFromDirectoryPath(__FILE__).c_str()) + "_" + intToStr(__LINE__));
	gameSettings = *serverGameSettings;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__);
}

void ClientInterface::broadcastGameSetup(const GameSettings *gameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//MutexSafeWrapper safeMutex(&serverSynchAccessor,string(extractFileFromDirectoryPath(__FILE__).c_str()) + "_" + intToStr(__LINE__));
	NetworkMessageLaunch networkMessageLaunch(gameSettings, nmtBroadCastSetup);
	//broadcastMessage(&networkMessageLaunch);
	sendMessage(&networkMessageLaunch);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void ClientInterface::broadcastGameStart(const GameSettings *gameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//MutexSafeWrapper safeMutex(&serverSynchAccessor,string(extractFileFromDirectoryPath(__FILE__).c_str()) + "_" + intToStr(__LINE__));
	NetworkMessageLaunch networkMessageLaunch(gameSettings, nmtLaunch);
	//broadcastMessage(&networkMessageLaunch);
	sendMessage(&networkMessageLaunch);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	this->joinGameInProgressLaunch = true;
}
void ClientInterface::setGameSettingsReceived(bool value) {
	//printf("In [%s:%s] Line: %d gameSettingsReceived = %d value = %d, gameSettingsReceivedCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettingsReceived,value,gameSettingsReceivedCount);
	gameSettingsReceived = value;
	gameSettingsReceivedCount++;
}

}}//end namespace
