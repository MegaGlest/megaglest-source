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

#include "leak_dumper.h"
#include "logger.h"
#include <time.h>

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
	}
	serverSocket.setBlock(false);
	serverSocket.bind(GameConstants::serverPort);
}

ServerInterface::~ServerInterface(){
    if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		delete slots[i];
	}

	close();

	if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::addSlot(int playerIndex){

    if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);

	delete slots[playerIndex];
	slots[playerIndex]= new ConnectionSlot(this, playerIndex);
	updateListen();

	if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::removeSlot(int playerIndex){

    if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete slots[playerIndex];
	slots[playerIndex]= NULL;
	updateListen();

	if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
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
    std::map<int,bool> socketTriggeredList;
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
        if(gameHasBeenInitiated && Socket::enableDebugText) printf("In [%s::%s] socketTriggeredList.size() = %d\n",__FILE__,__FUNCTION__,socketTriggeredList.size());

        bool hasData = Socket::hasDataToRead(socketTriggeredList);

        if(hasData && Socket::enableDebugText) printf("In [%s::%s] hasData == true\n",__FILE__,__FUNCTION__);

        if(gameHasBeenInitiated == false || hasData == true)
        {
            //if(gameHasBeenInitiated && Socket::enableDebugText) printf("In [%s::%s] hasData == true\n",__FILE__,__FUNCTION__);

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
                        if(gameHasBeenInitiated && Socket::enableDebugText) printf("In [%s::%s] socketTriggeredList[i] = %i\n",__FILE__,__FUNCTION__,(socketTriggeredList[connectionSlot->getSocket()->getSocketId()] ? 1 : 0));

                        if(connectionSlot->isConnected())
                        {
                            if(Socket::enableDebugText) printf("In [%s::%s] calling slots[i]->update() for slot = %d socketId = %d\n",
                                 __FILE__,__FUNCTION__,i,connectionSlot->getSocket()->getSocketId());
                        }
                        else
                        {
                            if(gameHasBeenInitiated && Socket::enableDebugText) printf("In [%s::%s] slot = %d getSocket() == NULL\n",__FILE__,__FUNCTION__,i);
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

                            if(Socket::enableDebugText) printf("In [%s::%s] #1 about to broadcast nmtText chatText [%s] chatSender [%s] chatTeamIndex = %d for SlotIndex# %d\n",__FILE__,__FUNCTION__,chatText.c_str(),chatSender.c_str(),chatTeamIndex,i);

                            NetworkMessageText networkMessageText(chatText,chatSender,chatTeamIndex);
                            broadcastMessage(&networkMessageText, i);
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
                            if(connectionSlot->getSocket() != NULL && Socket::enableDebugText) printf("In [%s::%s] calling connectionSlot->getNextMessageType() for slots[i]->getSocket()->getSocketId() = %d\n",
                                __FILE__,__FUNCTION__,connectionSlot->getSocket()->getSocketId());

                            if(connectionSlot->getNextMessageType() == nmtText)
                            {
                                NetworkMessageText networkMessageText;
                                if(connectionSlot->receiveMessage(&networkMessageText))
                                {
                                    if(Socket::enableDebugText) printf("In [%s::%s] #2 about to broadcast nmtText msg for SlotIndex# %d\n",__FILE__,__FUNCTION__,i);

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

	//broadcast commands
	broadcastMessage(&networkMessageCommandList);
}

void ServerInterface::waitUntilReady(Checksum* checksum){

    if(Socket::enableDebugText) printf("In [%s] START\n",__FUNCTION__);

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
					    if(Socket::enableDebugText) printf("In [%s] networkMessageType==nmtReady\n",__FUNCTION__);

						connectionSlot->setReady();
					}
					else if(networkMessageType != nmtInvalid)
					{
						throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
					}

					waitingForHosts.push_back(connectionSlot->getHostName());

					allReady= false;
				}
			}
		}

		//check for timeout
		if(allReady == false)
		{
            if(chrono.getMillis() > readyWaitTimeout)
            {
                throw runtime_error("Timeout waiting for clients");
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

    if(Socket::enableDebugText) printf("In [%s] PART B (telling client we are ready!\n",__FUNCTION__);

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

	if(Socket::enableDebugText) printf("In [%s] END\n",__FUNCTION__);
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, getHostName(), teamIndex);
	broadcastMessage(&networkMessageText);
}

void ServerInterface::quitGame(bool userManuallyQuit)
{
    if(userManuallyQuit == true)
    {
        string sQuitText = getHostName() + " has chosen to leave the game!";
        NetworkMessageText networkMessageText(sQuitText,getHostName(),-1);
        broadcastMessage(&networkMessageText, -1);
    }

	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
}

string ServerInterface::getNetworkStatus() const{
	Lang &lang= Lang::getInstance();
	string str;

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
	return str;
}

bool ServerInterface::launchGame(const GameSettings* gameSettings){

    bool bOkToStart = true;

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
        NetworkMessageLaunch networkMessageLaunch(gameSettings);
        broadcastMessage(&networkMessageLaunch);
    }

    return bOkToStart;
}

void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot){

    //if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i)
	{
		ConnectionSlot* connectionSlot= slots[i];

		if(i != excludeSlot && connectionSlot != NULL)
		{
			if(connectionSlot->isConnected())
			{

			    if(Socket::enableDebugText) printf("In [%s::%s] before sendMessage\n",__FILE__,__FUNCTION__);
				connectionSlot->sendMessage(networkMessage);
			}
			else if(gameHasBeenInitiated == true)
			{

			    if(Socket::enableDebugText) printf("In [%s::%s] #1 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
				removeSlot(i);
			}
		}
		else if(i == excludeSlot && gameHasBeenInitiated == true &&
		        connectionSlot != NULL && connectionSlot->isConnected() == false)
		{
            if(Socket::enableDebugText) printf("In [%s::%s] #2 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
            removeSlot(i);
		}
	}

	//if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::broadcastMessageToConnectedClients(const NetworkMessage* networkMessage, int excludeSlot){

    //if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		if(i!= excludeSlot && connectionSlot!= NULL){
			if(connectionSlot->isConnected()){

			    if(Socket::enableDebugText) printf("In [%s::%s] before sendMessage\n",__FILE__,__FUNCTION__);

				connectionSlot->sendMessage(networkMessage);
			}
		}
	}

	//if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
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
    if(Socket::enableDebugText) printf("In [%s::%s] START gameSettingsUpdateCount = %d\n",__FILE__,__FUNCTION__,gameSettingsUpdateCount);

    if(getAllowGameDataSynchCheck() == true)
    {
        if(waitForClientAck == true && gameSettingsUpdateCount > 0)
        {
            if(Socket::enableDebugText) printf("In [%s::%s] Waiting for client acks #1\n",__FILE__,__FUNCTION__);

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
            if(Socket::enableDebugText) printf("In [%s::%s] Waiting for client acks #2\n",__FILE__,__FUNCTION__);

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

    if(Socket::enableDebugText) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ServerInterface::close()
{
    if(Socket::enableDebugText) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

	//serverSocket = ServerSocket();
}

bool ServerInterface::getFogOfWar()
{
    return Config::getInstance().getBool("FogOfWar");
}

}}//end namespace
