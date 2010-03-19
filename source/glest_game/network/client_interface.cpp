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
const int ClientInterface::waitSleepTime= 50;

ClientInterface::ClientInterface(){
	clientSocket= NULL;
	launchGame= false;
	introDone= false;
	playerIndex= -1;

	networkGameDataSynchCheckOkMap  = false;
	networkGameDataSynchCheckOkTile = false;
	networkGameDataSynchCheckOkTech = false;
}

ClientInterface::~ClientInterface()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

    if(clientSocket != NULL && clientSocket->isConnected() == true)
    {
        string sQuitText = getHostName() + " has chosen to leave the game!";
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

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END - socket = %d\n",__FILE__,__FUNCTION__,clientSocket->getSocketId());
}

void ClientInterface::reset()
{
    if(getSocket() != NULL)
    {
        string sQuitText = getHostName() + " has chosen to leave the game!";
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

            if(receiveMessage(&networkMessageIntro))
            {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got NetworkMessageIntro\n",__FILE__,__FUNCTION__);

                //check consistency

                if(networkMessageIntro.getVersionString() != getNetworkVersionString())
                //if(1)
                {
					string sErr = "Server and client binary mismatch!\nYou have to use the exactly same binaries!\n\nServer: " +
					networkMessageIntro.getVersionString() +
					"\nClient: " + getNetworkVersionString();
                    printf("%s\n",sErr.c_str());

                    sendTextMessage("Server and client binary mismatch!!",-1);
                    sendTextMessage(" Server:" + networkMessageIntro.getVersionString(),-1);
                    sendTextMessage(" Client: "+ getNetworkVersionString(),-1);
					if(Config::getInstance().getBool("NetworkConsistencyChecks"))
            		{// error message and disconnect only if checked
						DisplayErrorMessage(sErr);
                        quit= true;
                        close();
                    	return;
            		}
                }


                //send intro message
                NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(), getHostName(), -1);

                playerIndex= networkMessageIntro.getPlayerIndex();
                serverName= networkMessageIntro.getName();
                sendMessage(&sendNetworkMessageIntro);

                assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
                introDone= true;
            }
        }
        break;

        case nmtSynchNetworkGameData:
        {
            NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData;

            if(receiveMessage(&networkMessageSynchNetworkGameData))
            {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got NetworkMessageSynchNetworkGameData\n",__FILE__,__FUNCTION__);

                // check the checksum's
                int32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_tilesets) + "/" +
                    networkMessageSynchNetworkGameData.getTileset() + "/*", ".xml", NULL);

                this->setNetworkGameDataSynchCheckOkTile((tilesetCRC == networkMessageSynchNetworkGameData.getTilesetCRC()));
                if(this->getNetworkGameDataSynchCheckOkTile() == false)
                {
                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] tilesetCRC mismatch, local = %d, remote = %d\n",
                            __FILE__,__FUNCTION__,tilesetCRC,networkMessageSynchNetworkGameData.getTilesetCRC());
                }


                //tech, load before map because of resources
                int32 techCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_techs) + "/" +
                        networkMessageSynchNetworkGameData.getTech() + "/*", ".xml", NULL);

                this->setNetworkGameDataSynchCheckOkTech((techCRC == networkMessageSynchNetworkGameData.getTechCRC()));

                if(this->getNetworkGameDataSynchCheckOkTech() == false)
                {
                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC mismatch, local = %d, remote = %d\n",
                            __FILE__,__FUNCTION__,techCRC,networkMessageSynchNetworkGameData.getTechCRC());
                }

                //map
                Checksum checksum;
                string file = Map::getMapPath(networkMessageSynchNetworkGameData.getMap());
                checksum.addFile(file);
                int32 mapCRC = checksum.getSum();
                //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] file = [%s] checksum = %d\n",__FILE__,__FUNCTION__,file.c_str(),mapCRC);

                this->setNetworkGameDataSynchCheckOkMap((mapCRC == networkMessageSynchNetworkGameData.getMapCRC()));

                if(this->getNetworkGameDataSynchCheckOkMap() == false)
                {
                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] mapCRC mismatch, local = %d, remote = %d\n",
                            __FILE__,__FUNCTION__,mapCRC,networkMessageSynchNetworkGameData.getMapCRC());
                }


                this->setNetworkGameDataSynchCheckOkFogOfWar((getFogOfWar() == networkMessageSynchNetworkGameData.getFogOfWar()));

                if(this->getNetworkGameDataSynchCheckOkFogOfWar() == false)
                {
                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] getFogOfWar mismatch, local = %d, remote = %d\n",
                            __FILE__,__FUNCTION__,getFogOfWar(),networkMessageSynchNetworkGameData.getFogOfWar());
                }

                NetworkMessageSynchNetworkGameDataStatus sendNetworkMessageSynchNetworkGameDataStatus(mapCRC,tilesetCRC,techCRC,getFogOfWar());
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
}

void ClientInterface::updateKeyframe(int frameCount)
{
	bool done= false;

	while(!done)
	{
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType networkMessageType= getNextMessageType(true);

		switch(networkMessageType)
		{
			case nmtCommandList:
			    {
				//make sure we read the message
				NetworkMessageCommandList networkMessageCommandList;
				while(!receiveMessage(&networkMessageCommandList))
				{
					sleep(waitSleepTime);
				}

				//check that we are in the right frame
				if(networkMessageCommandList.getFrameCount()!=frameCount)
				{
				    string sErr = "Network synchronization error, frame counts do not match";
					//throw runtime_error("Network synchronization error, frame counts do not match");
                    sendTextMessage(sErr,-1);
                    DisplayErrorMessage(sErr);
                    quit= true;
                    close();
                    return;
				}

				// give all commands
				for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i)
				{
					pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
				}

				done= true;
			}
			break;

			case nmtQuit:
			{
				NetworkMessageQuit networkMessageQuit;
				if(receiveMessage(&networkMessageQuit))
				{
					quit= true;
				}
				done= true;
			}
			break;

			case nmtText:
			{
				NetworkMessageText networkMessageText;
				if(receiveMessage(&networkMessageText))
				{
					chatText      = networkMessageText.getText();
					chatSender    = networkMessageText.getSender();
					chatTeamIndex = networkMessageText.getTeamIndex();
				}
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
                }
		}
	}
}

void ClientInterface::waitUntilReady(Checksum* checksum)
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

    Logger &logger= Logger::getInstance();

	Chrono chrono;
	chrono.start();

	// FOR TESTING ONLY - delay to see the client count up while waiting
	//sleep(5000);

	//send ready message
	NetworkMessageReady networkMessageReady;
	sendMessage(&networkMessageReady);

    int64 lastMillisCheck = 0;
	//wait until we get a ready message from the server
	while(true)
	{
		NetworkMessageType networkMessageType = getNextMessageType(true);

		if(networkMessageType == nmtReady)
		{
			if(receiveMessage(&networkMessageReady))
			{
				break;
			}
		}
		else if(networkMessageType == nmtInvalid)
		{
			if(chrono.getMillis() > readyWaitTimeout)
			{
				//throw runtime_error("Timeout waiting for server");
				string sErr = "Timeout waiting for server";
                sendTextMessage(sErr,-1);
                DisplayErrorMessage(sErr);
				quit= true;
				close();
                return;
			}
			else
			{
                if(chrono.getMillis() / 1000 > lastMillisCheck)
                {
                    lastMillisCheck = (chrono.getMillis() / 1000);

                    char szBuf[1024]="";
                    sprintf(szBuf,"Waiting for network: %llu seconds elapsed (maximum wait time: %d seconds)",lastMillisCheck,int(readyWaitTimeout / 1000));
                    logger.add(szBuf, true);
                }
			}
		}
		else
		{
			//throw runtime_error(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType) );
            sendTextMessage("Unexpected network message: " + intToStr(networkMessageType),-1);
            DisplayErrorMessage(string(__FILE__) + "::" + string(__FUNCTION__) + " Unexpected network message: " + intToStr(networkMessageType));
            quit= true;
            close();
            return;
		}

		// sleep a bit
		sleep(waitSleepTime);
	}

	//check checksum
	if(networkMessageReady.getChecksum() != checksum->getSum())
	//if(1)
	{
		string sErr = "Checksum error, you don't have the same data as the server";
		//throw runtime_error("Checksum error, you don't have the same data as the server");
        sendTextMessage(sErr,-1);
		if(Config::getInstance().getBool("NetworkConsistencyChecks"))
        	{// error message and disconnect only if checked
				DisplayErrorMessage(sErr);
				quit= true;
				close();
        	}
        return;
	}


	//delay the start a bit, so clients have nore room to get messages
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
	Chrono chrono;
	chrono.start();

	while(getNextMessageType(true) == nmtInvalid)
	{
		if(!isConnected())
		{
			//throw runtime_error("Disconnected");
            sendTextMessage("Disconnected",-1);
            DisplayErrorMessage("Disconnected");
            quit= true;
            close();
			return;
		}

		if(chrono.getMillis()>messageWaitTimeout)
		{
			//throw runtime_error("Timeout waiting for message");
            sendTextMessage("Timeout waiting for message",-1);
            DisplayErrorMessage("Timeout waiting for message");
            quit= true;
            close();
            return;
		}

		sleep(waitSleepTime);
	}
}

void ClientInterface::quitGame(bool userManuallyQuit)
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

    if(clientSocket != NULL && userManuallyQuit == true)
    {
        string sQuitText = getHostName() + " has chosen to leave the game!";
        sendTextMessage(sQuitText,-1);
        close();
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ClientInterface::close()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete clientSocket;
	clientSocket= NULL;
}

bool ClientInterface::getFogOfWar()
{
    return Config::getInstance().getBool("FogOfWar");
}

}}//end namespace
