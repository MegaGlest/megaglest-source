// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] constructor for %p\n",__FILE__,__FUNCTION__,__LINE__,this);

	clientSocket= NULL;
	sessionKey = 0;
	launchGame= false;
	introDone= false;
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

ClientInterface::~ClientInterface() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] destructor for %p\n",__FILE__,__FUNCTION__,__LINE__,this);

    if(clientSocket != NULL && clientSocket->isConnected() == true) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    close();
    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete clientSocket;
	clientSocket = NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::connect(const Ip &ip, int port) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete clientSocket;
	clientSocket = NULL;

	this->ip    = ip;
	this->port  = port;

	clientSocket= new ClientSocket();
	clientSocket->setBlock(false);
	clientSocket->connect(ip, port);
	connectedTime = time(NULL);
	clientSocket->setBlock(true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END - socket = %d\n",__FILE__,__FUNCTION__,clientSocket->getSocketId());
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

	double lastSendElapsed = difftime(time(NULL),lastNetworkCommandListSendTime);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(networkMessageCommandList.getCommandCount() > 0 ||
	  (lastNetworkCommandListSendTime > 0 && lastSendElapsed >= ClientInterface::maxNetworkCommandListSendTimeWait)) {
		lastNetworkCommandListSendTime = time(NULL);
		sendMessage(&networkMessageCommandList);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

	// Possible cause of out of synch since we have more commands that need
	// to be sent in this frame
	if(requestedCommands.empty() == false) {
		//char szBuf[4096]="";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,requestedCommands.size());

        string sMsg = "may go out of synch: client requestedCommands.size() = " + intToStr(requestedCommands.size());
        sendTextMessage(sMsg,-1, true,"");
        sleep(1);

        if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
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

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got NetworkMessageIntro, networkMessageIntro.getGameState() = %d, versionString [%s], sessionKey = %d, playerIndex = %d, serverFTPPort = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageIntro.getGameState(),versionString.c_str(),sessionKey,playerIndex,serverFTPPort);

                //check consistency
				bool compatible = checkVersionComptability(networkMessageIntro.getVersionString(), getNetworkVersionSVNString());
				if(compatible == false) {
                //if(networkMessageIntro.getVersionString() != getNetworkVersionString()) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

                if(networkMessageIntro.getGameState() == nmgstOk) {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					//send intro message
                	Lang &lang= Lang::getInstance();
					NetworkMessageIntro sendNetworkMessageIntro(sessionKey,getNetworkVersionSVNString(), getHumanPlayerName(), -1, nmgstOk, this->getSocket()->getConnectedIPAddress(),serverFTPPort,lang.getLanguage());
					sendMessage(&sendNetworkMessageIntro);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					if(clientSocket == NULL || clientSocket->isConnected() == false) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	                	string sErr = "Disconnected from server during intro handshake.";
						DisplayErrorMessage(sErr);
	                    quit= true;
	                    close();

	                    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	                	return;
					}
					else {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
						introDone= true;
					}
                }
                else if(networkMessageIntro.getGameState() == nmgstNoSlots) {
                	string sErr = "Cannot join the server because there are no open slots for new players.";
					DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                	return;
                }
                else {
                	string sErr = "Unknown response from server: " + intToStr(networkMessageIntro.getGameState());
					DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                	return;
                }
            }
        }
        break;

		case nmtPing:
		{
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtPing\n",__FILE__,__FUNCTION__);

			NetworkMessagePing networkMessagePing;
			if(receiveMessage(&networkMessagePing)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				lastPingInfo = networkMessagePing;
			}
		}
		break;

        case nmtSynchNetworkGameData:
        {
            NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData;

            if(receiveMessage(&networkMessageSynchNetworkGameData)) {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got NetworkMessageSynchNetworkGameData, getTechCRCFileCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageSynchNetworkGameData.getTechCRCFileCount());

            	networkGameDataSynchCheckOkMap      = false;
            	networkGameDataSynchCheckOkTile     = false;
            	networkGameDataSynchCheckOkTech     = false;
                this->setNetworkGameDataSynchCheckTechMismatchReport("");
                this->setReceivedDataSynchCheck(false);

				int32 tilesetCRC = 0;
				int32 techCRC	 = 0;
				int32 mapCRC	 = 0;
				vector<std::pair<string,int32> > vctFileList;

				try {
					Config &config = Config::getInstance();
					string scenarioDir = "";
					if(gameSettings.getScenarioDir() != "") {
						scenarioDir = gameSettings.getScenarioDir();
						if(EndsWith(scenarioDir, ".xml") == true) {
							scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
							scenarioDir = scenarioDir.erase(scenarioDir.size() - gameSettings.getScenario().size(), gameSettings.getScenario().size() + 1);
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getScenarioDir().c_str(),gameSettings.getScenario().c_str(),scenarioDir.c_str());
					}

					// check the checksum's
					tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + networkMessageSynchNetworkGameData.getTileset() + string("/*"), ".xml", NULL);

					this->setNetworkGameDataSynchCheckOkTile((tilesetCRC == networkMessageSynchNetworkGameData.getTilesetCRC()));
					//if(this->getNetworkGameDataSynchCheckOkTile() == false)
					//{
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] tilesetCRC info, local = %d, remote = %d, networkMessageSynchNetworkGameData.getTileset() = [%s]\n",__FILE__,__FUNCTION__,__LINE__,tilesetCRC,networkMessageSynchNetworkGameData.getTilesetCRC(),networkMessageSynchNetworkGameData.getTileset().c_str());
					//}

					//tech, load before map because of resources
					techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + networkMessageSynchNetworkGameData.getTech() + string("/*"), ".xml", NULL);

					this->setNetworkGameDataSynchCheckOkTech((techCRC == networkMessageSynchNetworkGameData.getTechCRC()));

					if(this->getNetworkGameDataSynchCheckOkTech() == false) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),string("/") + networkMessageSynchNetworkGameData.getTech() + "/*", ".xml", NULL);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						string report = networkMessageSynchNetworkGameData.getTechCRCFileMismatchReport(vctFileList);
						this->setNetworkGameDataSynchCheckTechMismatchReport(report);

					}
					//if(this->getNetworkGameDataSynchCheckOkTech() == false)
					//{
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC info, local = %d, remote = %d, networkMessageSynchNetworkGameData.getTech() = [%s]\n",__FILE__,__FUNCTION__,techCRC,networkMessageSynchNetworkGameData.getTechCRC(),networkMessageSynchNetworkGameData.getTech().c_str());
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
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] mapCRC info, local = %d, remote = %d, file = [%s]\n",__FILE__,__FUNCTION__,__LINE__,mapCRC,networkMessageSynchNetworkGameData.getMapCRC(),file.c_str());
					//}
				}
				catch(const runtime_error &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					string sErr = ex.what();
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error during processing, sErr = [%s]\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());

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
                int32 fileCRC = checksum.getSum();

                if(fileCRC != networkMessageSynchNetworkGameDataFileCRCCheck.getFileCRC())
                {
                	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck localCRC = %d, remoteCRC = %d, file [%s]\n",
                        __FILE__,__FUNCTION__,fileCRC,
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
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtText\n",__FILE__,__FUNCTION__);

        		ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),networkMessageText.getTargetLanguage());
        		this->addChatInfo(msg);
            }
        }
        break;

        case nmtLaunch:
        case nmtBroadCastSetup:
        {
            NetworkMessageLaunch networkMessageLaunch;
            if(receiveMessage(&networkMessageLaunch)) {
            	if(networkMessageLaunch.getMessageType() == nmtLaunch) {
            		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtLaunch\n",__FILE__,__FUNCTION__,__LINE__);
            	}
            	else if(networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
            		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtBroadCastSetup\n",__FILE__,__FUNCTION__,__LINE__);
            	}
            	else {
            		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());

					char szBuf[1024]="";
					snprintf(szBuf,1023,"In [%s::%s Line: %d] Invalid networkMessageLaunch.getMessageType() = %d",__FILE__,__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
					throw runtime_error(szBuf);
            	}

                networkMessageLaunch.buildGameSettings(&gameSettings);

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
                //replace server player by network
                for(int i= 0; i<gameSettings.getFactionCount(); ++i) {
                    //replace by network
                    if(gameSettings.getFactionControl(i)==ctHuman) {
                        gameSettings.setFactionControl(i, ctNetwork);
                    }

                    //set the faction index
                    if(gameSettings.getStartLocationIndex(i) == playerIndex) {
                        gameSettings.setThisFactionIndex(i);
                        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getThisFactionIndex(i) = %d, playerIndex = %d, i = %d\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getThisFactionIndex(),playerIndex,i);
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

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtPlayerIndexMessage, playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
            }
		}
		break;
        default:
            {
            string sErr = string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType);
            //throw runtime_error(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));
            sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1, true,"");
            DisplayErrorMessage(sErr);
            sleep(1);

            quit= true;
            close();
            }
    }

	if( clientSocket != NULL && clientSocket->isConnected() == true &&
		gotIntro == false && difftime(time(NULL),connectedTime) > GameConstants::maxClientConnectHandshakeSecs) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] difftime(time(NULL),connectedTime) = %f\n",__FILE__,__FUNCTION__,__LINE__,difftime(time(NULL),connectedTime));
		close();
	}
}

void ClientInterface::updateKeyframe(int frameCount) {
	currentFrameCount = frameCount;

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	int simulateLag = Config::getInstance().getInt("SimulateClientLag","0");
	bool done= false;
	while(done == false) {
		//wait for the next message
		NetworkMessageType networkMessageType = waitForMessage();

		// START: Test simulating lag for the client
		if(simulateLag > 0) {
			if(clientSimulationLagStartTime == 0) {
				clientSimulationLagStartTime = time(NULL);
			}
			if(difftime(time(NULL),clientSimulationLagStartTime) <= Config::getInstance().getInt("SimulateClientLagDurationSeconds","0")) {
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
					throw runtime_error("error retrieving nmtCommandList returned false!");
				}

//				while(receiveMessage(&networkMessageCommandList) == false &&
//					  isConnected() == true &&
//					  difftime(time(NULL),receiveTimeElapsed) <= (messageWaitTimeout / 2000)) {
//					waitCount++;
//				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] receiveMessage took %lld msecs, waitCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),waitCount);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
				//check that we are in the right frame
				if(networkMessageCommandList.getFrameCount() != frameCount) {
				    string sErr = "Player: " + getHumanPlayerName() +
				    		      " got a Network synchronization error, frame counts do not match, server frameCount = " +
				    		      intToStr(networkMessageCommandList.getFrameCount()) + ", local frameCount = " +
				    		      intToStr(frameCount);
                    sendTextMessage(sErr,-1, true,"");
                    DisplayErrorMessage(sErr);
                    sleep(1);

                    quit= true;
                    close();
                    return;
				}

				// give all commands
				for(int i= 0; i < networkMessageCommandList.getCommandCount(); ++i) {
					pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] transfer network commands took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				done= true;
			}
			break;

			case nmtPing:
			{
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtPing\n",__FILE__,__FUNCTION__);

				NetworkMessagePing networkMessagePing;
				if(receiveMessage(&networkMessagePing)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					lastPingInfo = networkMessagePing;
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
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
					throw runtime_error("error retrieving nmtQuit returned false!");
				}

				quit= true;
				done= true;
			}
			break;

			case nmtText:
			{
				time_t receiveTimeElapsed = time(NULL);
				NetworkMessageText networkMessageText;
//				while(receiveMessage(&networkMessageText) == false &&
//					  isConnected() == true &&
//					  difftime(time(NULL),receiveTimeElapsed) <= (messageWaitTimeout / 1000)) {
//				}
				bool gotCmd = receiveMessage(&networkMessageText);
				if(gotCmd == false) {
					throw runtime_error("error retrieving nmtText returned false!");
				}

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

        		ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),networkMessageText.getTargetLanguage());
        		this->addChatInfo(msg);

        		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			}
			break;
            case nmtLoadingStatusMessage:
                break;

			case nmtInvalid:
                break;

			default:
                {
                sendTextMessage("Unexpected message in client interface: " + intToStr(networkMessageType),-1, true,"");
                DisplayErrorMessage(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected message in client interface: " + intToStr(networkMessageType));
                sleep(1);

				quit= true;
				close();
				done= true;
                }
		}

		if(isConnected() == false && quit == true) {
			done = true;
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took %lld msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

void ClientInterface::waitUntilReady(Checksum* checksum) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    Logger &logger= Logger::getInstance();

	Chrono chrono;
	chrono.start();

	// FOR TESTING ONLY - delay to see the client count up while waiting
	//sleep(5000);

	//clientSocket->setBlock(true);
	//send ready message
	NetworkMessageReady networkMessageReady;
	sendMessage(&networkMessageReady);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

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
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			string sErr = "Error, Server has disconnected!";
            DisplayErrorMessage(sErr);

            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

            quit= true;
            close();

            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
            return;
		}
		NetworkMessageType networkMessageType = getNextMessageType();

		// consume old messages from the lobby
		bool discarded = shouldDiscardNetworkMessage(networkMessageType);
		if(discarded == false) {
			if(networkMessageType == nmtReady) {
				if(receiveMessage(&networkMessageReady)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
			}
			else if(networkMessageType == nmtLoadingStatusMessage) {
				if(receiveMessage(&networkMessageLoadingStatus)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
				}
			}
			else if(networkMessageType == nmtQuit) {
				NetworkMessageQuit networkMessageQuit;
				if(receiveMessage(&networkMessageQuit)) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

					DisplayErrorMessage(lang.get("GameCancelledByUser"));

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

					quit= true;
					close();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
					return;

				}
			}
			else if(networkMessageType == nmtInvalid) {
				if(chrono.getMillis() > readyWaitTimeout) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

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

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

					sleep(1);
					quit= true;
					close();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
					return;
				}
				else {
					if(chrono.getMillis() / 1000 > lastMillisCheck) {
						lastMillisCheck = (chrono.getMillis() / 1000);

						char szBuf[1024]="";
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
						sprintf(szBuf,updateTextFormat.c_str(),(long long int)lastMillisCheck,int(readyWaitTimeout / 1000));

						char szBuf1[1024]="";
						string statusTextFormat =  "Waiting for players: %s";
						if(lang.hasString("NetworkGameStatusWaiting") == true) {
							statusTextFormat = lang.get("NetworkGameStatusWaiting");
						}
						sprintf(szBuf1,statusTextFormat.c_str(),waitForHosts.c_str());

						logger.add(szBuf, true, szBuf1);
					}
				}
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
				sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1, true,"");

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				DisplayErrorMessage(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				sleep(1);
				quit= true;
				close();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
				return;
			}

			Window::handleEvent();
			// sleep a bit
			sleep(waitSleepTime);
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	//check checksum
	if(networkMessageReady.getChecksum() != checksum->getSum()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

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

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d %s %s %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str(),sErr1.c_str(),sErr2.c_str());

			if(echoLocal == true) {
				if(Config::getInstance().getBool("NetworkConsistencyChecks")) {
					// error message and disconnect only if checked
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

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

//        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d %s %s %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str(),sErr1.c_str(),sErr2.c_str());

		if(Config::getInstance().getBool("NetworkConsistencyChecks")) {
			// error message and disconnect only if checked
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			//string niceError = sErr + string("\n") + sErr1 + string("\n") + sErr2;
			//DisplayErrorMessage(niceError);

			sleep(1);
			quit= true;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			close();
       	}
        return;
	}

	//delay the start a bit, so clients have more room to get messages
	//sleep(GameConstants::networkExtraLatency);

	// This triggers LAG update packets to begin as required
	lastNetworkCommandListSendTime = time(NULL);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex, bool echoLocal,
		string targetLanguage) {

	string humanPlayerName = getHumanPlayerName();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,humanPlayerName.c_str(),playerIndex);

	NetworkMessageText networkMessageText(text, teamIndex,playerIndex,targetLanguage);
	sendMessage(&networkMessageText);

	if(echoLocal == true) {
		ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),targetLanguage);
		this->addChatInfo(msg);
	}
}

void ClientInterface::sendPingMessage(int32 pingFrequency, int64 pingTime) {
	NetworkMessagePing networkMessagePing(pingFrequency,pingTime);
	sendMessage(&networkMessagePing);
}

string ClientInterface::getNetworkStatus() {
	std::string label = Lang::getInstance().get("Server") + ": " + serverName;
	//float pingTime = getThreadedPingMS(getServerIpAddress().c_str());
	char szBuf[1024]="";
	//sprintf(szBuf,"%s, ping = %.2fms",label.c_str(),pingTime);
	sprintf(szBuf,"%s",label.c_str());

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
					//throw runtime_error("Disconnected");
					//sendTextMessage("Server has Disconnected.",-1);
					DisplayErrorMessage("Server has Disconnected.");
					quit= true;
				}
				close();
				return msg;
			}

			if(chrono.getMillis() > messageWaitTimeout) {
			//if(1) {
				//throw runtime_error("Timeout waiting for message");

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

	//if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] waiting took %lld msecs, waitLoopCount = %ull, msg = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),waitLoopCount,msg);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] waiting took %lld msecs, msg = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),msg);

	return msg;
}

void ClientInterface::quitGame(bool userManuallyQuit)
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] userManuallyQuit = %d\n",__FILE__,__FUNCTION__,__LINE__,userManuallyQuit);

    if(clientSocket != NULL && userManuallyQuit == true)
    {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::close()
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] START, clientSocket = %p\n",__FILE__,__FUNCTION__,__LINE__,clientSocket);

	delete clientSocket;
	clientSocket= NULL;

	connectedTime = 0;
	gotIntro = false;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::discoverServers(DiscoveredServersInterface *cb) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ClientSocket::discoverServers(cb);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}
void ClientInterface::stopServerDiscovery() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ClientSocket::stopBroadCastClientThread();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::sendSwitchSetupRequest(string selectedFactionName, int8 currentFactionIndex,
											int8 toFactionIndex,int8 toTeam, string networkPlayerName,
											int8 networkPlayerStatus, int8 flags,
											string language) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkPlayerName [%s] flags = %d\n",__FILE__,__FUNCTION__,__LINE__,networkPlayerName.c_str(),flags);
	SwitchSetupRequest message=SwitchSetupRequest(selectedFactionName,
			currentFactionIndex, toFactionIndex,toTeam,networkPlayerName,
			networkPlayerStatus, flags,language);
	sendMessage(&message);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtText\n",__FILE__,__FUNCTION__,__LINE__);
			discard = true;
			NetworkMessageText netMsg = NetworkMessageText();
			this->receiveMessage(&netMsg);

    		ChatMsgInfo msg(netMsg.getText().c_str(),netMsg.getTeamIndex(),netMsg.getPlayerIndex(),netMsg.getTargetLanguage());
    		this->addChatInfo(msg);
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);
	//MutexSafeWrapper safeMutex(&serverSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
	gameSettings = *serverGameSettings;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ClientInterface::broadcastGameSetup(const GameSettings *gameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	//MutexSafeWrapper safeMutex(&serverSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
	NetworkMessageLaunch networkMessageLaunch(gameSettings, nmtBroadCastSetup);
	//broadcastMessage(&networkMessageLaunch);
	sendMessage(&networkMessageLaunch);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::broadcastGameStart(const GameSettings *gameSettings) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	//MutexSafeWrapper safeMutex(&serverSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
	NetworkMessageLaunch networkMessageLaunch(gameSettings, nmtLaunch);
	//broadcastMessage(&networkMessageLaunch);
	sendMessage(&networkMessageLaunch);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}
void ClientInterface::setGameSettingsReceived(bool value) {
	//printf("In [%s:%s] Line: %d gameSettingsReceived = %d value = %d, gameSettingsReceivedCount = %d\n",__FILE__,__FUNCTION__,__LINE__,gameSettingsReceived,value,gameSettingsReceivedCount);
	gameSettingsReceived = value;
	gameSettingsReceivedCount++;
}

}}//end namespace
