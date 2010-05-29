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
#include "leak_dumper.h"

#include "map.h"
#include "config.h"
#include "logger.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout= 10000;	//10 seconds
const int ClientInterface::waitSleepTime= 10;

ClientInterface::ClientInterface(){
	clientSocket= NULL;
	launchGame= false;
	introDone= false;
	playerIndex= -1;
	gameSettingsReceived=false;
	gotIntro = false;

	networkGameDataSynchCheckOkMap  = false;
	networkGameDataSynchCheckOkTile = false;
	networkGameDataSynchCheckOkTech = false;
}

ClientInterface::~ClientInterface()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

    if(clientSocket != NULL && clientSocket->isConnected() == true)
    {
        string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
        sendTextMessage(sQuitText,-1);
    }

	delete clientSocket;
	clientSocket = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ClientInterface::connect(const Ip &ip, int port)
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete clientSocket;

	this->ip    = ip;
	this->port  = port;

	clientSocket= new ClientSocket();
	clientSocket->setBlock(false);
	clientSocket->connect(ip, port);
	connectedTime = time(NULL);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END - socket = %d\n",__FILE__,__FUNCTION__,clientSocket->getSocketId());
}

void ClientInterface::reset()
{
    if(getSocket() != NULL)
    {
        string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
        sendTextMessage(sQuitText,-1);
        close();
    }
}

void ClientInterface::update()
{
	NetworkMessageCommandList networkMessageCommandList;

	//send as many commands as we can
	while(!requestedCommands.empty()){
		if(networkMessageCommandList.addCommand(&requestedCommands.back())){
			requestedCommands.pop_back();
		}
		else{
			break;
		}
	}
	if(networkMessageCommandList.getCommandCount()>0){
		sendMessage(&networkMessageCommandList);
	}

	//clear chat variables
	chatText.clear();
	chatSender.clear();
	chatTeamIndex= -1;
}

void ClientInterface::updateLobby()
{
	//clear chat variables
	chatText.clear();
	chatSender.clear();
	chatTeamIndex= -1;

    NetworkMessageType networkMessageType = getNextMessageType(true);

    switch(networkMessageType)
    {
        case nmtInvalid:
            break;

        case nmtIntro:
        {
            NetworkMessageIntro networkMessageIntro;

            if(receiveMessage(&networkMessageIntro)) {
            	gotIntro = true;

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got NetworkMessageIntro, networkMessageIntro.getGameState() = %d\n",__FILE__,__FUNCTION__,networkMessageIntro.getGameState());

                //check consistency
                if(networkMessageIntro.getVersionString() != getNetworkVersionString()) {
                	bool versionMatched = false;
                	string platformFreeVersion = getNetworkPlatformFreeVersionString();
                	string sErr = "";

                	if(strncmp(platformFreeVersion.c_str(),networkMessageIntro.getVersionString().c_str(),strlen(platformFreeVersion.c_str())) != 0) {
    					sErr = "Server and client binary mismatch!\nYou have to use the exactly same binaries!\n\nServer: " +
    					networkMessageIntro.getVersionString() +
    					"\nClient: " + getNetworkVersionString();
                        printf("%s\n",sErr.c_str());

                        sendTextMessage("Server and client binary mismatch!!",-1);
                        sendTextMessage(" Server:" + networkMessageIntro.getVersionString(),-1);
                        sendTextMessage(" Client: "+ getNetworkVersionString(),-1);
                	}
                	else {
                		versionMatched = true;
						sErr = "Warning, Server and client are using the same version but different platforms.\n\nServer: " +
										networkMessageIntro.getVersionString() + "\nClient: " + getNetworkVersionString();
						printf("%s\n",sErr.c_str());

						sendTextMessage("Server and client platform mismatch.",-1);
						sendTextMessage(" Server:" + networkMessageIntro.getVersionString(),-1);
						sendTextMessage(" Client: "+ getNetworkVersionString(),-1);
                	}

					if(Config::getInstance().getBool("PlatformConsistencyChecks","true") &&
					   versionMatched == false) { // error message and disconnect only if checked
						DisplayErrorMessage(sErr);
                        quit= true;
                        close();
                    	return;
            		}
                }

                if(networkMessageIntro.getGameState() == nmgstOk) {
					//send intro message
					NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(), Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()), -1, nmgstOk);

					playerIndex= networkMessageIntro.getPlayerIndex();
					serverName= networkMessageIntro.getName();
					sendMessage(&sendNetworkMessageIntro);

					assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
					introDone= true;
                }
                else if(networkMessageIntro.getGameState() == nmgstNoSlots) {
                	string sErr = "Cannot join the server because there are no open slots for new players.";
					DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                	return;
                }
                else {
                	string sErr = "Unknown response from server: " + intToStr(networkMessageIntro.getGameState());
					DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                	return;
                }
            }
        }
        break;

        case nmtSynchNetworkGameData:
        {
            NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData;

            if(receiveMessage(&networkMessageSynchNetworkGameData))
            {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got NetworkMessageSynchNetworkGameData\n",__FILE__,__FUNCTION__);

				int32 tilesetCRC = 0;
				int32 techCRC	 = 0;
				int32 mapCRC	 = 0;

				try {
					Config &config = Config::getInstance();
					string scenarioDir = "";
					if(gameSettings.getScenarioDir() != "") {
						scenarioDir = gameSettings.getScenarioDir();
						if(EndsWith(scenarioDir, ".xml") == true) {
							scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
							scenarioDir = scenarioDir.erase(scenarioDir.size() - gameSettings.getScenario().size(), gameSettings.getScenario().size() + 1);
						}

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getScenarioDir().c_str(),gameSettings.getScenario().c_str(),scenarioDir.c_str());
					}

					// check the checksum's
					//int32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_tilesets) + "/" + networkMessageSynchNetworkGameData.getTileset() + "/*", ".xml", NULL);
					tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + networkMessageSynchNetworkGameData.getTileset() + string("/*"), ".xml", NULL);

					this->setNetworkGameDataSynchCheckOkTile((tilesetCRC == networkMessageSynchNetworkGameData.getTilesetCRC()));
					//if(this->getNetworkGameDataSynchCheckOkTile() == false)
					//{
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] tilesetCRC info, local = %d, remote = %d, networkMessageSynchNetworkGameData.getTileset() = [%s]\n",__FILE__,__FUNCTION__,tilesetCRC,networkMessageSynchNetworkGameData.getTilesetCRC(),networkMessageSynchNetworkGameData.getTileset().c_str());
					//}


					//tech, load before map because of resources
					//int32 techCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_techs) + "/" + networkMessageSynchNetworkGameData.getTech() + "/*", ".xml", NULL);
					techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + networkMessageSynchNetworkGameData.getTech() + string("/*"), ".xml", NULL);

					this->setNetworkGameDataSynchCheckOkTech((techCRC == networkMessageSynchNetworkGameData.getTechCRC()));

					//if(this->getNetworkGameDataSynchCheckOkTech() == false)
					//{
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC info, local = %d, remote = %d, networkMessageSynchNetworkGameData.getTech() = [%s]\n",__FILE__,__FUNCTION__,techCRC,networkMessageSynchNetworkGameData.getTechCRC(),networkMessageSynchNetworkGameData.getTech().c_str());
					//}

					//map
					Checksum checksum;
					string file = Map::getMapPath(networkMessageSynchNetworkGameData.getMap(),scenarioDir, false);
					if(file != "") {
						checksum.addFile(file);
						mapCRC = checksum.getSum();
					}
					//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] file = [%s] checksum = %d\n",__FILE__,__FUNCTION__,file.c_str(),mapCRC);

					this->setNetworkGameDataSynchCheckOkMap((mapCRC == networkMessageSynchNetworkGameData.getMapCRC()));

					//if(this->getNetworkGameDataSynchCheckOkMap() == false)
					//{
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] mapCRC info, local = %d, remote = %d, file = [%s]\n",__FILE__,__FUNCTION__,mapCRC,networkMessageSynchNetworkGameData.getMapCRC(),file.c_str());
					//}
				}
				catch(const runtime_error &ex) {
					string sErr = ex.what();
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] error during processing, sErr = [%s]\n",__FILE__,__FUNCTION__,sErr.c_str());

					DisplayErrorMessage(sErr);
				}

				NetworkMessageSynchNetworkGameDataStatus sendNetworkMessageSynchNetworkGameDataStatus(mapCRC,tilesetCRC,techCRC);
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
                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck localCRC = %d, remoteCRC = %d, file [%s]\n",
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
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtText\n",__FILE__,__FUNCTION__);

                chatText      = networkMessageText.getText();
                chatSender    = networkMessageText.getSender();
                chatTeamIndex = networkMessageText.getTeamIndex();
            }
        }
        break;

        case nmtLaunch:
        {
            NetworkMessageLaunch networkMessageLaunch;

            if(receiveMessage(&networkMessageLaunch))
            {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got NetworkMessageLaunch\n",__FILE__,__FUNCTION__);

                networkMessageLaunch.buildGameSettings(&gameSettings);

                //replace server player by network
                for(int i= 0; i<gameSettings.getFactionCount(); ++i)
                {
                    //replace by network
                    if(gameSettings.getFactionControl(i)==ctHuman)
                    {
                        gameSettings.setFactionControl(i, ctNetwork);
                    }

                    //set the faction index
                    if(gameSettings.getStartLocationIndex(i)==playerIndex)
                    {
                        gameSettings.setThisFactionIndex(i);
                    }
                }
                launchGame= true;
            }
        }
        break;
        case nmtBroadCastSetup:
        {
            NetworkMessageLaunch networkMessageLaunch;

            if(receiveMessage(&networkMessageLaunch))
            {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got NetworkMessageLaunch\n",__FILE__,__FUNCTION__);

                networkMessageLaunch.buildGameSettings(&gameSettings);

                //replace server player by network
                for(int i= 0; i<gameSettings.getFactionCount(); ++i)
                {
                    //replace by network
                    if(gameSettings.getFactionControl(i)==ctHuman)
                    {
                        gameSettings.setFactionControl(i, ctNetwork);
                    }

                    //set the faction index
                    if(gameSettings.getStartLocationIndex(i)==playerIndex)
                    {
                        gameSettings.setThisFactionIndex(i);
                    }
                }
                gameSettingsReceived=true;
            }
        }
        break;
		case nmtPlayerIndexMessage:
		{
			PlayerIndexMessage playerIndexMessage(-1);
			if(receiveMessage(&playerIndexMessage))
            {
				playerIndex= playerIndexMessage.getPlayerIndex();
            }
		}
		break;
        default:
            {
            string sErr = string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType);
            //throw runtime_error(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));
            sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1);
            DisplayErrorMessage(sErr);
            quit= true;
            close();
            }
    }

	if(gotIntro == false && difftime(time(NULL),connectedTime) > GameConstants::maxClientConnectHandshakeSecs) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] difftime(time(NULL),connectedTime) = %d\n",__FILE__,__FUNCTION__,__LINE__,difftime(time(NULL),connectedTime));
		close();
	}
}

void ClientInterface::updateKeyframe(int frameCount)
{
	bool done= false;

	while(done == false) {
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType networkMessageType= getNextMessageType(true);

		switch(networkMessageType)
		{
			case nmtCommandList:
			    {
				Chrono chrono;
				chrono.start();

				int waitCount = 0;
				//make sure we read the message
				NetworkMessageCommandList networkMessageCommandList;
				while(receiveMessage(&networkMessageCommandList) == false &&
					  isConnected() == true) {
					//sleep(waitSleepTime);
					sleep(0);
					waitCount++;
				}

				if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] receiveMessage took %d msecs, waitCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),waitCount);

				chrono.start();
				//check that we are in the right frame
				if(networkMessageCommandList.getFrameCount() != frameCount) {
				    string sErr = "Network synchronization error, frame counts do not match";
					//throw runtime_error("Network synchronization error, frame counts do not match");
                    sendTextMessage(sErr,-1);
                    DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                    return;
				}

				// give all commands
				for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i) {
					pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
				}

				if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] transfer network commands took %d msecs\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

				done= true;
			}
			break;

			case nmtQuit:
			{
				NetworkMessageQuit networkMessageQuit;
				//if(receiveMessage(&networkMessageQuit)) {
				while(receiveMessage(&networkMessageQuit) == false &&
					  isConnected() == true) {
					sleep(0);
				}
				quit= true;
				done= true;
			}
			break;

			case nmtText:
			{
				NetworkMessageText networkMessageText;
				while(receiveMessage(&networkMessageText) == false &&
					  isConnected() == true) {
					sleep(0);
				}

				chatText      = networkMessageText.getText();
				chatSender    = networkMessageText.getSender();
				chatTeamIndex = networkMessageText.getTeamIndex();
			}
			break;
            case nmtInvalid:
                break;

			default:
                {
				//throw runtime_error(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected message in client interface: " + intToStr(networkMessageType));

                sendTextMessage("Unexpected message in client interface: " + intToStr(networkMessageType),-1);
                DisplayErrorMessage(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected message in client interface: " + intToStr(networkMessageType));
				quit= true;
				close();
				done= true;
                }
		}

		if(isConnected() == false && quit == true) {
			done = true;
		}
	}
}

void ClientInterface::waitUntilReady(Checksum* checksum) {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    Logger &logger= Logger::getInstance();

	Chrono chrono;
	chrono.start();

	// FOR TESTING ONLY - delay to see the client count up while waiting
	//sleep(5000);

	//send ready message
	NetworkMessageReady networkMessageReady;
	sendMessage(&networkMessageReady);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    int64 lastMillisCheck = 0;
	//wait until we get a ready message from the server
	while(true)	{

		if(isConnected() == false) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			string sErr = "Error, Server has disconnected!";
            //sendTextMessage(sErr,-1);

            //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

            DisplayErrorMessage(sErr);

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

            quit= true;
            close();

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
            return;
		}
		NetworkMessageType networkMessageType = getNextMessageType(true);
		if(networkMessageType == nmtReady) {
			if(receiveMessage(&networkMessageReady)) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
		}
		else if(networkMessageType == nmtInvalid) {
			if(chrono.getMillis() > readyWaitTimeout) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
				//throw runtime_error("Timeout waiting for server");
				string sErr = "Timeout waiting for server";
                sendTextMessage(sErr,-1);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

                DisplayErrorMessage(sErr);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				quit= true;
				close();

				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
                return;
			}
			else {
                if(chrono.getMillis() / 1000 > lastMillisCheck) {
                    lastMillisCheck = (chrono.getMillis() / 1000);

                    char szBuf[1024]="";
                    sprintf(szBuf,"Waiting for network: %llu seconds elapsed (maximum wait time: %d seconds)",lastMillisCheck,int(readyWaitTimeout / 1000));
                    logger.add(szBuf, true);
                }
			}
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
			//throw runtime_error(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType) );
            sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1);

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

            DisplayErrorMessage(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

            quit= true;
            close();

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
            return;
		}

		// sleep a bit
		sleep(waitSleepTime);
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	//check checksum
	if(networkMessageReady.getChecksum() != checksum->getSum()) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		string sErr = "Checksum error, you don't have the same data as the server";
        sendTextMessage(sErr,-1);

		string sErr1 = "Client Checksum: " + intToStr(checksum->getSum());
        sendTextMessage(sErr1,-1);

		string sErr2 = "Server Checksum: " + intToStr(networkMessageReady.getChecksum());
        sendTextMessage(sErr2,-1);

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d %s %s %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str(),sErr1.c_str(),sErr2.c_str());

		if(Config::getInstance().getBool("NetworkConsistencyChecks")) {
			// error message and disconnect only if checked
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			string niceError = sErr + string("\n") + sErr1 + string("\n") + sErr2;
			DisplayErrorMessage(niceError);

			quit= true;

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			close();
       	}
        return;
	}

	//delay the start a bit, so clients have more room to get messages
	sleep(GameConstants::networkExtraLatency);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, getHostName(), teamIndex);
	sendMessage(&networkMessageText);
}

string ClientInterface::getNetworkStatus() const{
	return Lang::getInstance().get("Server") + ": " + serverName;
}

void ClientInterface::waitForMessage()
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

	int waitLoopCount = 0;
	while(getNextMessageType(true) == nmtInvalid) {
		if(isConnected() == false) {
			//throw runtime_error("Disconnected");
            sendTextMessage("Server has Disconnected.",-1);
            DisplayErrorMessage("Server has Disconnected.");
            quit= true;
            close();
			return;
		}

		if(chrono.getMillis() > messageWaitTimeout) {
		//if(1) {
			//throw runtime_error("Timeout waiting for message");

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            sendTextMessage("Timeout waiting for message",-1);
            DisplayErrorMessage("Timeout waiting for message");
            quit= true;
            close();
            return;
		}

		//sleep(waitSleepTime);
		waitLoopCount++;
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] waiting took %d msecs, waitLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),waitLoopCount);
}

void ClientInterface::quitGame(bool userManuallyQuit)
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] userManuallyQuit = %d\n",__FILE__,__FUNCTION__,__LINE__,userManuallyQuit);

    if(clientSocket != NULL && userManuallyQuit == true)
    {
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
        sendTextMessage(sQuitText,-1);
        close();
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::close()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete clientSocket;
	clientSocket= NULL;

	connectedTime = 0;
	gotIntro = false;
}

void ClientInterface::discoverServers(DiscoveredServersInterface *cb) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ClientSocket::discoverServers(cb);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}
void ClientInterface::stopServerDiscovery() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ClientSocket::stopBroadCastClientThread();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientInterface::sendSwitchSetupRequest(string selectedFactionName, int8 currentFactionIndex, int8 toFactionIndex,int8 toTeam)
{
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//printf("string-cuf-tof-team= %s-%d-%d-%d\n",selectedFactionName.c_str(),currentFactionIndex,toFactionIndex,toTeam);
	SwitchSetupRequest message=SwitchSetupRequest(selectedFactionName, currentFactionIndex, toFactionIndex,toTeam);
	sendMessage(&message);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

/*
bool ClientInterface::getFogOfWar()
{
    return Config::getInstance().getBool("FogOfWar");
}
*/
}}//end namespace
