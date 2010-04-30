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

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(){
    gameHasBeenInitiated    = false;
    gameSettingsUpdateCount = 0;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		slots[i]= NULL;
		switchSetupRequests[i]= NULL;
	}
	serverSocket.setBlock(false);
	serverSocket.bind(Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str()));
}

ServerInterface::~ServerInterface(){
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		delete slots[i];
		slots[i]=NULL;
		delete switchSetupRequests[i];
		switchSetupRequests[i]=NULL;
	}

	close();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::addSlot(int playerIndex){

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);

	delete slots[playerIndex];
	slots[playerIndex]= new ConnectionSlot(this, playerIndex);
	updateListen();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

bool ServerInterface::switchSlot(int fromPlayerIndex,int toPlayerIndex){

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);
	bool result=false;
	assert(fromPlayerIndex>=0 && fromPlayerIndex<GameConstants::maxPlayers);
	assert(toPlayerIndex>=0 && toPlayerIndex<GameConstants::maxPlayers);
	if(fromPlayerIndex==toPlayerIndex) return false;// doubleclicked or whatever

	//printf(" checking if slot %d is free?\n",toPlayerIndex);
	if( !slots[toPlayerIndex]->isConnected()) {
		//printf(" yes, its free :)\n");
		slots[fromPlayerIndex]->setPlayerIndex(toPlayerIndex);
		ConnectionSlot *tmp=slots[toPlayerIndex];
		slots[toPlayerIndex]= slots[fromPlayerIndex];
		slots[fromPlayerIndex]=tmp;
		PlayerIndexMessage playerIndexMessage(toPlayerIndex);
        slots[toPlayerIndex]->sendMessage(&playerIndexMessage);
		result=true;
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
	return result;
}

void ServerInterface::removeSlot(int playerIndex){

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete slots[playerIndex];
	slots[playerIndex]= NULL;
	updateListen();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex){
	return slots[playerIndex];
}

int ServerInterface::getConnectedSlotCount(){
	int connectedSlotCount= 0;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL){
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

void ServerInterface::update()
{
    std::map<PLATFORM_SOCKET,bool> socketTriggeredList;
	//update all slots
	for(int i= 0; i < GameConstants::maxPlayers; ++i)
	{
	    ConnectionSlot* connectionSlot= slots[i];
		if(connectionSlot != NULL && connectionSlot->getSocket() != NULL &&
		   slots[i]->getSocket()->getSocketId() > 0)
		{
		    socketTriggeredList[connectionSlot->getSocket()->getSocketId()] = false;
		}
	}

    chatText.clear();
    chatSender.clear();
    chatTeamIndex= -1;

    if(gameHasBeenInitiated == false || socketTriggeredList.size() > 0)
    {
        if(gameHasBeenInitiated) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socketTriggeredList.size() = %d\n",__FILE__,__FUNCTION__,socketTriggeredList.size());

        bool hasData = Socket::hasDataToRead(socketTriggeredList);

        if(hasData) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] hasData == true\n",__FILE__,__FUNCTION__);

        if(gameHasBeenInitiated == false || hasData == true)
        {
            //if(gameHasBeenInitiated && Socket::enableNetworkDebugInfo) printf("In [%s::%s] hasData == true\n",__FILE__,__FUNCTION__);

            //std::vector<TeamMessageData> vctTeamMessages;

            //update all slots
            bool checkForNewClients = true;
            for(int i= 0; i<GameConstants::maxPlayers; ++i)
            {
                ConnectionSlot* connectionSlot= slots[i];
                if(connectionSlot != NULL &&
                   (gameHasBeenInitiated == false || (connectionSlot->getSocket() != NULL && socketTriggeredList[connectionSlot->getSocket()->getSocketId()] == true)))
                {
                    if(connectionSlot->isConnected() == false ||
                        (socketTriggeredList[connectionSlot->getSocket()->getSocketId()] == true))
                    {
                        if(gameHasBeenInitiated) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socketTriggeredList[i] = %i\n",__FILE__,__FUNCTION__,(socketTriggeredList[connectionSlot->getSocket()->getSocketId()] ? 1 : 0));

                        if(connectionSlot->isConnected())
                        {
                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling slots[i]->update() for slot = %d socketId = %d\n",
                                 __FILE__,__FUNCTION__,i,connectionSlot->getSocket()->getSocketId());
                        }
                        else
                        {
                            if(gameHasBeenInitiated) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] slot = %d getSocket() == NULL\n",__FILE__,__FUNCTION__,i);
                        }
                        connectionSlot->update(checkForNewClients);

                        // This means no clients are trying to connect at the moment
                        if(connectionSlot != NULL && connectionSlot->getSocket() == NULL)
                        {
                            checkForNewClients = false;
                        }

                        if(connectionSlot != NULL &&
                           //connectionSlot->isConnected() == true &&
                           connectionSlot->getChatText().empty() == false)
                        {
                            chatText        = connectionSlot->getChatText();
                            chatSender      = connectionSlot->getChatSender();
                            chatTeamIndex   = connectionSlot->getChatTeamIndex();

                            //TeamMessageData teamMessageData;
                            //teamMessageData.chatSender      = connectionSlot->getChatSender();
                            //teamMessageData.chatText        = connectionSlot->getChatText();
                            //teamMessageData.chatTeamIndex   = connectionSlot->getChatTeamIndex();
                            //teamMessageData.sourceTeamIndex = i;
                            //vctTeamMessages.push_back(teamMessageData);

                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 about to broadcast nmtText chatText [%s] chatSender [%s] chatTeamIndex = %d for SlotIndex# %d\n",__FILE__,__FUNCTION__,__LINE__,chatText.c_str(),chatSender.c_str(),chatTeamIndex,i);

                            NetworkMessageText networkMessageText(chatText,chatSender,chatTeamIndex);
                            broadcastMessage(&networkMessageText, i);

                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                            break;
                        }
                    }
                }
            }

            //process text messages
            if(chatText.empty() == true)
            {
                chatText.clear();
                chatSender.clear();
                chatTeamIndex= -1;

                for(int i= 0; i< GameConstants::maxPlayers; ++i)
                {
                    ConnectionSlot* connectionSlot= slots[i];

                    if(connectionSlot!= NULL &&
                        (gameHasBeenInitiated == false || (connectionSlot->getSocket() != NULL && socketTriggeredList[connectionSlot->getSocket()->getSocketId()] == true)))
                    {
                        if(connectionSlot->isConnected() && socketTriggeredList[connectionSlot->getSocket()->getSocketId()] == true)
                        {
                            if(connectionSlot->getSocket() != NULL) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling connectionSlot->getNextMessageType() for slots[i]->getSocket()->getSocketId() = %d\n",
                                __FILE__,__FUNCTION__,connectionSlot->getSocket()->getSocketId());

                            if(connectionSlot->getNextMessageType() == nmtText)
                            {
                                NetworkMessageText networkMessageText;
                                if(connectionSlot->receiveMessage(&networkMessageText))
                                {
                                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 about to broadcast nmtText msg for SlotIndex# %d\n",__FILE__,__FUNCTION__,i);

                                    broadcastMessage(&networkMessageText, i);
                                    chatText= networkMessageText.getText();
                                    chatSender= networkMessageText.getSender();
                                    chatTeamIndex= networkMessageText.getTeamIndex();
                                    break;
                                }
							}
                        }
                    }
                }
            }
        }
    }
}

void ServerInterface::updateKeyframe(int frameCount){
	Chrono chrono;
	chrono.start();

	NetworkMessageCommandList networkMessageCommandList(frameCount);

	//build command list, remove commands from requested and add to pending
	while(!requestedCommands.empty()){
		if(networkMessageCommandList.addCommand(&requestedCommands.back())){
			pendingCommands.push_back(requestedCommands.back());
			requestedCommands.pop_back();
		}
		else{
			break;
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] build command list took %d msecs, networkMessageCommandList.getCommandCount() = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),networkMessageCommandList.getCommandCount(),frameCount);

	//broadcast commands
	broadcastMessage(&networkMessageCommandList);

	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] broadcastMessage took %d msecs, networkMessageCommandList.getCommandCount() = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),networkMessageCommandList.getCommandCount(),frameCount);
}

void ServerInterface::waitUntilReady(Checksum* checksum){

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] START\n",__FUNCTION__);

    Logger &logger= Logger::getInstance();
    gameHasBeenInitiated = true;

	Chrono chrono;
	bool allReady= false;

	chrono.start();

	//wait until we get a ready message from all clients
	while(allReady == false)
	{
	    vector<string> waitingForHosts;
		allReady= true;
		for(int i= 0; i<GameConstants::maxPlayers; ++i)
		{
			ConnectionSlot* connectionSlot= slots[i];

			if(connectionSlot != NULL && connectionSlot->isConnected() == true)
			{
				if(connectionSlot->isReady() == false)
				{
					NetworkMessageType networkMessageType= connectionSlot->getNextMessageType(true);
					NetworkMessageReady networkMessageReady;

					if(networkMessageType == nmtReady &&
					   connectionSlot->receiveMessage(&networkMessageReady))
					{
					    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] networkMessageType==nmtReady\n",__FUNCTION__);

						connectionSlot->setReady();
					}
					else if(networkMessageType != nmtInvalid)
					{
						//throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
                        string sErr = "Unexpected network message: " + intToStr(networkMessageType);
                        sendTextMessage(sErr,-1);
                        DisplayErrorMessage(sErr);
						return;
					}

					//waitingForHosts.push_back(connectionSlot->getHostName());
					waitingForHosts.push_back(connectionSlot->getName());

					allReady= false;
				}
			}
		}

		//check for timeout
		if(allReady == false)
		{
            if(chrono.getMillis() > readyWaitTimeout)
            {
                //throw runtime_error("Timeout waiting for clients");
                string sErr = "Timeout waiting for clients.";
                sendTextMessage(sErr,-1);
                DisplayErrorMessage(sErr);
                return;
            }
            else
            {
                if(chrono.getMillis() % 1000 == 0)
                {
                    string waitForHosts = "";
                    for(int i = 0; i < waitingForHosts.size(); i++)
                    {
                        if(waitForHosts != "")
                        {
                            waitForHosts += ", ";
                        }
                        waitForHosts += waitingForHosts[i];
                    }

                    char szBuf[1024]="";
                    sprintf(szBuf,"Waiting for network: %d of %d max seconds (waiting for: %s)",int(chrono.getMillis() / 1000),int(readyWaitTimeout / 1000),waitForHosts.c_str());
                    logger.add(szBuf, true);
                }
            }
		}
	}


	// FOR TESTING ONLY - delay to see the client count up while waiting
	//sleep(5000);

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] PART B (telling client we are ready!\n",__FUNCTION__);

	//send ready message after, so clients start delayed
	for(int i= 0; i < GameConstants::maxPlayers; ++i)
	{
		NetworkMessageReady networkMessageReady(checksum->getSum());

		ConnectionSlot* connectionSlot= slots[i];
		if(connectionSlot!=NULL)
		{
			connectionSlot->sendMessage(&networkMessageReady);
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] END\n",__FUNCTION__);
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	NetworkMessageText networkMessageText(text, Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()), teamIndex);
	broadcastMessage(&networkMessageText);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::quitGame(bool userManuallyQuit)
{
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    if(userManuallyQuit == true)
    {
        string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
        NetworkMessageText networkMessageText(sQuitText,getHostName(),-1);
        broadcastMessage(&networkMessageText, -1);
    }

	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

string ServerInterface::getNetworkStatus() const{
	Lang &lang= Lang::getInstance();
	string str;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		str+= intToStr(i)+ ": ";

		if(connectionSlot!= NULL){
			if(connectionSlot->isConnected()){

                str+= connectionSlot->getName();
			}
		}
		else
		{
			str+= lang.get("NotConnected");
		}

		str+= '\n';
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	return str;
}

bool ServerInterface::launchGame(const GameSettings* gameSettings){

    bool bOkToStart = true;

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    for(int i= 0; i<GameConstants::maxPlayers; ++i)
    {
        ConnectionSlot *connectionSlot= slots[i];
        if(connectionSlot != NULL &&
           connectionSlot->getAllowDownloadDataSynch() == true &&
           connectionSlot->isConnected())
        {
            if(connectionSlot->getNetworkGameDataSynchCheckOk() == false)
            {
                bOkToStart = false;
                break;
            }
        }
    }

    if(bOkToStart == true)
    {
   		serverSocket.stopBroadCastThread();

        NetworkMessageLaunch networkMessageLaunch(gameSettings,nmtLaunch);
        broadcastMessage(&networkMessageLaunch);
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    return bOkToStart;
}

void ServerInterface::broadcastGameSetup(const GameSettings* gameSettings){
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
    
    NetworkMessageLaunch networkMessageLaunch(gameSettings,nmtBroadCastSetup);
    broadcastMessage(&networkMessageLaunch);
    
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}


void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot){

    //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i)
	{
		ConnectionSlot* connectionSlot= slots[i];

		if(i != excludeSlot && connectionSlot != NULL)
		{
			if(connectionSlot->isConnected())
			{

			    //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] before sendMessage\n",__FILE__,__FUNCTION__);
				connectionSlot->sendMessage(networkMessage);
			}
			else if(gameHasBeenInitiated == true)
			{

			    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
				removeSlot(i);
			}
		}
		else if(i == excludeSlot && gameHasBeenInitiated == true &&
		        connectionSlot != NULL && connectionSlot->isConnected() == false)
		{
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
            removeSlot(i);
		}
	}

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::broadcastMessageToConnectedClients(const NetworkMessage* networkMessage, int excludeSlot){

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		if(i!= excludeSlot && connectionSlot!= NULL){
			if(connectionSlot->isConnected()){

			    //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] before sendMessage\n",__FILE__,__FUNCTION__);

				connectionSlot->sendMessage(networkMessage);
			}
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::updateListen()
{
	int openSlotCount= 0;

	for(int i= 0; i<GameConstants::maxPlayers; ++i)
	{
		if(slots[i] != NULL && slots[i]->isConnected() == false)
		{
			++openSlotCount;
		}
	}

	serverSocket.listen(openSlotCount);
}

void ServerInterface::setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck)
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START gameSettingsUpdateCount = %d, waitForClientAck = %d\n",__FILE__,__FUNCTION__,gameSettingsUpdateCount,waitForClientAck);

    if(getAllowGameDataSynchCheck() == true)
    {
        if(waitForClientAck == true && gameSettingsUpdateCount > 0)
        {
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Waiting for client acks #1\n",__FILE__,__FUNCTION__);

            time_t tStart = time(NULL);
            bool gotAckFromAllClients = false;
            while(gotAckFromAllClients == false && difftime(time(NULL),tStart) <= 5)
            {
                gotAckFromAllClients = true;
                for(int i= 0; i<GameConstants::maxPlayers; ++i)
                {
                    ConnectionSlot *connectionSlot = slots[i];
                    if(connectionSlot != NULL && connectionSlot->isConnected())
                    {
                        if(connectionSlot->getReceivedNetworkGameStatus() == false)
                        {
                            gotAckFromAllClients = false;
                        }

                        connectionSlot->update();
                    }
                }
            }
        }

        for(int i= 0; i<GameConstants::maxPlayers; ++i)
        {
            ConnectionSlot *connectionSlot = slots[i];
            if(connectionSlot != NULL && connectionSlot->isConnected())
            {
                connectionSlot->setReceivedNetworkGameStatus(false);
            }
        }

        gameSettings = *serverGameSettings;

        NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(getGameSettings());
        broadcastMessageToConnectedClients(&networkMessageSynchNetworkGameData);

        if(waitForClientAck == true)
        {
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Waiting for client acks #2\n",__FILE__,__FUNCTION__);

            time_t tStart = time(NULL);
            bool gotAckFromAllClients = false;
            while(gotAckFromAllClients == false && difftime(time(NULL),tStart) <= 5)
            {
                gotAckFromAllClients = true;
                for(int i= 0; i<GameConstants::maxPlayers; ++i)
                {
                    ConnectionSlot *connectionSlot = slots[i];
                    if(connectionSlot != NULL && connectionSlot->isConnected())
                    {
                        if(connectionSlot->getReceivedNetworkGameStatus() == false)
                        {
                            gotAckFromAllClients = false;
                        }

                        connectionSlot->update();
                    }
                }
            }
        }

        gameSettingsUpdateCount++;
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::close()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	//serverSocket = ServerSocket();
}

}}//end namespace
