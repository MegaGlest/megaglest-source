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

// Experimental threading of broadcasts to clients
//bool enabledThreadedClientCommandBroadcast = true;
bool enabledThreadedClientCommandBroadcast = false;

// The maximum amount of network update iterations a client is allowed to fall behind
double maxFrameCountLagAllowed = 30;
// The maximum amount of seconds a client is allowed to not communicate with the server
double maxClientLagTimeAllowed = 20;

// 65% of max we warn all users about the lagged client
double warnFrameCountLagPercent = 0.65;
// Should we wait for lagged clients instead of disconnect them?
bool pauseGameForLaggedClients = false;

// Seconds grace period before we start checking LAG
double LAG_CHECK_GRACE_PERIOD = 15;

ServerInterface::ServerInterface(){
    gameHasBeenInitiated    = false;
    gameSettingsUpdateCount = 0;
    currentFrameCount = 0;
    gameStartTime = 0;

    enabledThreadedClientCommandBroadcast = Config::getInstance().getBool("EnableThreadedClientCommandBroadcast","false");
    maxFrameCountLagAllowed = Config::getInstance().getInt("MaxFrameCountLagAllowed",intToStr(maxFrameCountLagAllowed).c_str());
    maxClientLagTimeAllowed = Config::getInstance().getInt("MaxClientLagTimeAllowed",intToStr(maxClientLagTimeAllowed).c_str());
    warnFrameCountLagPercent = Config::getInstance().getFloat("WarnFrameCountLagPercent",doubleToStr(warnFrameCountLagPercent).c_str());
    pauseGameForLaggedClients = Config::getInstance().getFloat("PauseGameForLaggedClients",boolToStr(pauseGameForLaggedClients).c_str());
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] enabledThreadedClientCommandBroadcast = %d, maxFrameCountLagAllowed = %f, maxClientLagTimeAllowed = %f, pauseGameForLaggedClients = %d\n",__FILE__,__FUNCTION__,__LINE__,enabledThreadedClientCommandBroadcast,maxFrameCountLagAllowed,maxClientLagTimeAllowed,pauseGameForLaggedClients);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		slots[i]= NULL;
		switchSetupRequests[i]= NULL;
	}
	serverSocket.setBlock(false);
	//serverSocket.bind(Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str()));
	serverSocket.setBindPort(Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str()));
}

ServerInterface::~ServerInterface(){
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		delete slots[i];
		slots[i]=NULL;

		delete switchSetupRequests[i];
		switchSetupRequests[i]=NULL;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	close();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::addSlot(int playerIndex){
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);

	if(serverSocket.isPortBound() == false) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		serverSocket.bind(serverSocket.getBindPort());
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	delete slots[playerIndex];

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	slots[playerIndex]= new ConnectionSlot(this, playerIndex);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	updateListen();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool ServerInterface::switchSlot(int fromPlayerIndex,int toPlayerIndex){
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool result=false;
	assert(fromPlayerIndex>=0 && fromPlayerIndex<GameConstants::maxPlayers);
	assert(toPlayerIndex>=0 && toPlayerIndex<GameConstants::maxPlayers);
	if(fromPlayerIndex==toPlayerIndex) return false;// doubleclicked or whatever

	//printf(" checking if slot %d is free?\n",toPlayerIndex);
	if( !slots[toPlayerIndex]->isConnected()) {
		//printf(" yes, its free :)\n");
		slots[fromPlayerIndex]->setPlayerIndex(toPlayerIndex);
		slots[toPlayerIndex]->setPlayerIndex(fromPlayerIndex);
		ConnectionSlot *tmp=slots[toPlayerIndex];
		slots[toPlayerIndex]= slots[fromPlayerIndex];
		slots[fromPlayerIndex]=tmp;
		PlayerIndexMessage playerIndexMessage(toPlayerIndex);
        slots[toPlayerIndex]->sendMessage(&playerIndexMessage);
		result=true;
		updateListen();
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	return result;
}

void ServerInterface::removeSlot(int playerIndex) {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

    // Mention to everyone that this player is disconnected
    ConnectionSlot *slot = slots[playerIndex];

    bool notifyDisconnect = false;
    char szBuf[4096]="";
    if(	slot != NULL) {
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

    	if(slot->getLastReceiveCommandListTime() > 0) {
    		const char* msgTemplate = "Player %s, disconnected from the game.";
#ifdef WIN32
    		_snprintf(szBuf,4095,msgTemplate,slot->getName().c_str());
#else
    		snprintf(szBuf,4095,msgTemplate,slot->getName().c_str());
#endif
    		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

    		notifyDisconnect = true;
    	}
    }
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

	delete slots[playerIndex];
	slots[playerIndex]= NULL;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

	updateListen();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

	if(notifyDisconnect == true) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
		string sMsg = szBuf;
		sendTextMessage(sMsg,-1, true);
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex){
	return slots[playerIndex];
}

bool ServerInterface::hasClientConnection() {
	bool result = false;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i] != NULL && slots[i]->isConnected() == true) {
			result = true;
			break;
		}
	}
	return result;
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

void ServerInterface::slotUpdateTask(ConnectionSlotEvent *event) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(event != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(event->eventType == eSendSocketData) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] before sendMessage, event->networkMessage = %p\n",__FILE__,__FUNCTION__,event->networkMessage);

			event->connectionSlot->sendMessage(event->networkMessage);
		}
		else if(event->eventType == eReceiveSocketData) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			updateSlot(event);
		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//
// WARNING!!! This method is executed from the slot worker threads so be careful
// what we do here (things need to be thread safe)
//
void ServerInterface::updateSlot(ConnectionSlotEvent *event) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(event != NULL) {
		ConnectionSlot *connectionSlot = event->connectionSlot;
		bool &socketTriggered = event->socketTriggered;
		bool checkForNewClients = true;

		if(connectionSlot != NULL &&
		   (gameHasBeenInitiated == false || (connectionSlot->getSocket() != NULL && socketTriggered == true))) {
			if(connectionSlot->isConnected() == false || socketTriggered == true) {
				if(gameHasBeenInitiated) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socketTriggeredList[i] = %i\n",__FILE__,__FUNCTION__,(socketTriggered ? 1 : 0));

				if(connectionSlot->isConnected()) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to call connectionSlot->update() for socketId = %d\n",__FILE__,__FUNCTION__,__LINE__,connectionSlot->getSocket()->getSocketId());
				}
				else {
					if(gameHasBeenInitiated) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] getSocket() == NULL\n",__FILE__,__FUNCTION__);
				}
				connectionSlot->update(checkForNewClients);

				// This means no clients are trying to connect at the moment
				if(connectionSlot != NULL && connectionSlot->getSocket() == NULL) {
					checkForNewClients = false;
				}
			}
		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// Only call when client has just sent us data
bool ServerInterface::clientLagCheck(ConnectionSlot* connectionSlot) {
	bool clientLagExceeded = false;

	static bool alreadyInLagCheck = false;
	if(alreadyInLagCheck == true) {
		return clientLagExceeded;
	}

	try {
		alreadyInLagCheck = true;

		if(difftime(time(NULL),gameStartTime) >= LAG_CHECK_GRACE_PERIOD) {
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				double clientLag = this->getCurrentFrameCount() - connectionSlot->getCurrentFrameCount();
				double clientLagCount = (gameSettings.getNetworkFramePeriod() > 0 ? (clientLag / gameSettings.getNetworkFramePeriod()) : 0);
				connectionSlot->setCurrentLagCount(clientLagCount);

				double clientLagTime = difftime(time(NULL),connectionSlot->getLastReceiveCommandListTime());

				if(this->getCurrentFrameCount() > 0) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, clientLag = %f, clientLagCount = %f, this->getCurrentFrameCount() = %d, connectionSlot->getCurrentFrameCount() = %d, clientLagTime = %f\n",
																			 __FILE__,__FUNCTION__,__LINE__,
																			 connectionSlot->getPlayerIndex(),clientLag,clientLagCount,
																			 this->getCurrentFrameCount(),connectionSlot->getCurrentFrameCount(),clientLagTime);
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
					(maxClientLagTimeAllowed > 0 && clientLagTime > maxClientLagTimeAllowed)) {
					clientLagExceeded = true;
					char szBuf[4096]="";

					const char* msgTemplate = "DROPPING %s, exceeded max allowed LAG count of %f [time = %f], clientLag = %f [%f], disconnecting client.";
					if(pauseGameForLaggedClients == true) {
						msgTemplate = "PAUSING GAME TEMPORARILY for %s, exceeded max allowed LAG count of %f [time = %f], clientLag = %f [%f], waiting for client to catch up...";
					}
		#ifdef WIN32
					_snprintf(szBuf,4095,msgTemplate,connectionSlot->getName().c_str() ,maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#else
					snprintf(szBuf,4095,msgTemplate,connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#endif
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

					string sMsg = szBuf;
					sendTextMessage(sMsg,-1, true);

					if(pauseGameForLaggedClients == false) {
						connectionSlot->close();
					}
				}
				// New lag check warning
				else if((maxFrameCountLagAllowed > 0 && warnFrameCountLagPercent > 0 &&
						 clientLagCount > (maxFrameCountLagAllowed * warnFrameCountLagPercent)) ||
						(maxClientLagTimeAllowed > 0 && warnFrameCountLagPercent > 0 &&
						 clientLagTime > (maxClientLagTimeAllowed * warnFrameCountLagPercent)) ) {
					if(connectionSlot->getLagCountWarning() == false) {
						connectionSlot->setLagCountWarning(true);

						char szBuf[4096]="";

						const char* msgTemplate = "LAG WARNING for %s, may exceed max allowed LAG count of %f [time = %f], clientLag = %f [%f], WARNING...";
		#ifdef WIN32
						_snprintf(szBuf,4095,msgTemplate,connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#else
						snprintf(szBuf,4095,msgTemplate,connectionSlot->getName().c_str(),maxFrameCountLagAllowed,maxClientLagTimeAllowed,clientLagCount,clientLagTime);
		#endif
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

						string sMsg = szBuf;
						sendTextMessage(sMsg,-1, true);
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

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	throw runtime_error(ex.what());
	}
	alreadyInLagCheck = false;

	return clientLagExceeded;
}

bool ServerInterface::signalClientReceiveCommands(ConnectionSlot* connectionSlot,
												  int slotIndex,
												  bool socketTriggered,
												  ConnectionSlotEvent &event) {
	bool slotSignalled = false;
	//bool socketTriggered = (connectionSlot != NULL && connectionSlot->getSocket() != NULL ? socketTriggeredList[connectionSlot->getSocket()->getSocketId()] : false);
	//ConnectionSlotEvent &event = eventList[i];
	event.eventType = eReceiveSocketData;
	event.networkMessage = NULL;
	event.connectionSlot = connectionSlot;
	event.socketTriggered = socketTriggered;
	event.triggerId = slotIndex;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex);

	// Step #1 tell all connection slot worker threads to receive socket data
	if(connectionSlot != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex);
		if(socketTriggered == true || connectionSlot->isConnected() == false) {
			connectionSlot->signalUpdate(&event);
			slotSignalled = true;
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex);
		}
	}

	return slotSignalled;
}

void ServerInterface::updateSocketTriggeredList(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList) {
	//update all slots
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		ConnectionSlot* connectionSlot= slots[i];
		if(connectionSlot != NULL && connectionSlot->getSocket() != NULL &&
		   slots[i]->getSocket()->isSocketValid() == true) {
			socketTriggeredList[connectionSlot->getSocket()->getSocketId()] = false;
		}
	}
}

void ServerInterface::update() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	std::vector<string> errorMsgList;
	try {
		std::map<PLATFORM_SOCKET,bool> socketTriggeredList;
		//update all slots
		updateSocketTriggeredList(socketTriggeredList);

		if(gameHasBeenInitiated == false || socketTriggeredList.size() > 0) {
			std::map<int,ConnectionSlotEvent> eventList;
			bool hasData = Socket::hasDataToRead(socketTriggeredList);

			if(hasData) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] hasData == true\n",__FILE__,__FUNCTION__);

			if(gameHasBeenInitiated == false || hasData == true) {
				std::map<int,bool> mapSlotSignalledList;

				// Step #1 tell all connection slot worker threads to receive socket data
				bool checkForNewClients = true;
				for(int i= 0; i<GameConstants::maxPlayers; ++i) {
					ConnectionSlot* connectionSlot = slots[i];

					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

					bool socketTriggered = (connectionSlot != NULL && connectionSlot->getSocket() != NULL ? socketTriggeredList[connectionSlot->getSocket()->getSocketId()] : false);
					ConnectionSlotEvent &event = eventList[i];
					mapSlotSignalledList[i] = signalClientReceiveCommands(connectionSlot,i,socketTriggered,event);
				}

				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				// Step #2 check all connection slot worker threads for completed status
				std::map<int,bool> slotsCompleted;
				for(bool threadsDone = false; threadsDone == false;) {
					threadsDone = true;
					// Examine all threads for completion of delegation
					for(int i= 0; i< GameConstants::maxPlayers; ++i) {
						ConnectionSlot* connectionSlot = slots[i];
						if(connectionSlot != NULL &&
							mapSlotSignalledList[i] == true &&
							slotsCompleted.find(i) == slotsCompleted.end()) {
							try {
								std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
								// Show any collected errors from threads
								if(errorList.size() > 0) {
									for(int iErrIdx = 0; iErrIdx < errorList.size(); ++iErrIdx) {
										string &sErr = errorList[iErrIdx];
										if(sErr != "") {
											//DisplayErrorMessage(sErr);
											errorMsgList.push_back(sErr);
										}
									}
									connectionSlot->clearThreadErrorList();
								}

								connectionSlot = slots[i];

								// Not done waiting for data yet
								bool updateFinished = (connectionSlot != NULL ? connectionSlot->updateCompleted() : true);
								if(updateFinished == false) {
									threadsDone = false;
									sleep(0);
									break;
								}
								else {
									connectionSlot = slots[i];

									// New lag check
									bool clientLagExceeded = false;
									if( gameHasBeenInitiated == true && connectionSlot != NULL &&
										connectionSlot->isConnected() == true) {
										clientLagExceeded = clientLagCheck(connectionSlot);
									}
									// If the client has exceeded lag and the server wants
									// to pause while they catch up, re-trigger the
									// client reader thread
									if(clientLagExceeded == true && pauseGameForLaggedClients == true) {
										bool socketTriggered = (connectionSlot != NULL && connectionSlot->getSocket() != NULL ? socketTriggeredList[connectionSlot->getSocket()->getSocketId()] : false);
										ConnectionSlotEvent &event = eventList[i];
										signalClientReceiveCommands(connectionSlot,i,socketTriggered,event);
										sleep(0);
									}
									else {
										slotsCompleted[i] = true;
									}
								}
							}
							catch(const exception &ex) {
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
								errorMsgList.push_back(ex.what());
							}
						}
					}
				}

				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				// Step #3 dispatch network commands to the pending list so that they are done in proper order
				if(gameHasBeenInitiated == true) {
					for(int i= 0; i< GameConstants::maxPlayers; ++i) {
						ConnectionSlot* connectionSlot= slots[i];
						if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
							if(connectionSlot->getPendingNetworkCommandList().size() > 0) {
								// New lag check
								bool clientLagExceeded = clientLagCheck(connectionSlot);
								if(clientLagExceeded) {
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d, clientLagExceeded = %d\n",__FILE__,__FUNCTION__,__LINE__,i,clientLagExceeded);
								}
								else {
									vector<NetworkCommand> vctPendingNetworkCommandList = connectionSlot->getPendingNetworkCommandList();

									for(int idx = 0; idx < vctPendingNetworkCommandList.size(); ++idx) {
										NetworkCommand &cmd = vctPendingNetworkCommandList[idx];
										this->requestCommand(&cmd);
									}
									connectionSlot->clearPendingNetworkCommandList();
								}
							}
						}
					}
				}
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				// Step #4 dispatch pending chat messages
				for(int i= 0; i< GameConstants::maxPlayers; ++i) {
					ConnectionSlot* connectionSlot= slots[i];
					if(connectionSlot != NULL &&
					   connectionSlot->getChatTextList().empty() == false) {
						try {
							for(int chatIdx = 0; chatIdx < connectionSlot->getChatTextList().size(); chatIdx++) {
								ChatMsgInfo msg(connectionSlot->getChatTextList()[chatIdx]);
								this->addChatInfo(msg);

								string newChatText     = msg.chatText.c_str();
								string newChatSender   = msg.chatSender.c_str();
								int newChatTeamIndex   = msg.chatTeamIndex;

								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 about to broadcast nmtText chatText [%s] chatSender [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,newChatText.c_str(),newChatSender.c_str(),newChatTeamIndex);

								NetworkMessageText networkMessageText(newChatText.c_str(),newChatSender.c_str(),newChatTeamIndex);
								broadcastMessage(&networkMessageText, connectionSlot->getPlayerIndex());

								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] after broadcast nmtText chatText [%s] chatSender [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,newChatText.c_str(),newChatSender.c_str(),newChatTeamIndex);
							}

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);
							// Its possible that the slot is disconnected here
							// so check the original pointer again
							if(slots[i] != NULL) {
								slots[i]->clearChatInfo();
							}
						}
						catch(const exception &ex) {
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
							errorMsgList.push_back(ex.what());
						}
					}
				}

				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

				//process text messages
/*
				if(this->getChatTextList().empty() == true) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

					for(int i= 0; i< GameConstants::maxPlayers; ++i) {
						ConnectionSlot* connectionSlot= slots[i];

						if(connectionSlot!= NULL &&
							(gameHasBeenInitiated == false || (connectionSlot->getSocket() != NULL && socketTriggeredList[connectionSlot->getSocket()->getSocketId()] == true))) {
							if( connectionSlot->isConnected() &&
								socketTriggeredList[connectionSlot->getSocket()->getSocketId()] == true) {
								if(connectionSlot->getSocket() != NULL) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling connectionSlot->getNextMessageType() for slots[i]->getSocket()->getSocketId() = %d\n",__FILE__,__FUNCTION__,connectionSlot->getSocket()->getSocketId());

								try {
									if(connectionSlot->getNextMessageType() == nmtText) {
										NetworkMessageText networkMessageText;
										if(connectionSlot->receiveMessage(&networkMessageText)) {
											SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 about to broadcast nmtText msg for SlotIndex# %d\n",__FILE__,__FUNCTION__,i);

											broadcastMessage(&networkMessageText, i);

											//chatText= networkMessageText.getText();
											//chatSender= networkMessageText.getSender();
											//chatTeamIndex= networkMessageText.getTeamIndex();
											//break;

											ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getSender().c_str(),networkMessageText.getTeamIndex());
											this->addChatInfo(msg);
										}
									}
								}
								catch(const exception &ex) {
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
									errorMsgList.push_back(ex.what());
								}
							}
						}
					}
				}

				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
*/
			}
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		errorMsgList.push_back(ex.what());
	}

	if(errorMsgList.size() > 0) {
		for(int iErrIdx = 0; iErrIdx < errorMsgList.size(); ++iErrIdx) {
			string &sErr = errorMsgList[iErrIdx];
			if(sErr != "") {
				DisplayErrorMessage(sErr);
			}
		}
	}

    //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::updateKeyframe(int frameCount){
	Chrono chrono;
	chrono.start();

	currentFrameCount = frameCount;
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] currentFrameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,currentFrameCount);

	NetworkMessageCommandList networkMessageCommandList(frameCount);

	//build command list, remove commands from requested and add to pending
	while(!requestedCommands.empty()) {
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
		if(!requestedCommands.empty()) {
			char szBuf[1024]="";
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR, requestedCommands.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,requestedCommands.size());

			string sMsg = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " may go out of synch: server requestedCommands.size() = " + intToStr(requestedCommands.size());
			sendTextMessage(sMsg,-1, true);
		}

		if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] build command list took %lld msecs, networkMessageCommandList.getCommandCount() = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),networkMessageCommandList.getCommandCount(),frameCount);

		//broadcast commands
		broadcastMessage(&networkMessageCommandList);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}

	if(chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] broadcastMessage took %lld msecs, networkMessageCommandList.getCommandCount() = %d, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),networkMessageCommandList.getCommandCount(),frameCount);
}

bool ServerInterface::shouldDiscardNetworkMessage(NetworkMessageType networkMessageType,
												  ConnectionSlot* connectionSlot) {
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

	    		ChatMsgInfo msg(netMsg.getText().c_str(),netMsg.getSender().c_str(),netMsg.getTeamIndex());
	    		this->addChatInfo(msg);
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

void ServerInterface::waitUntilReady(Checksum* checksum){

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] START\n",__FUNCTION__);

    Logger &logger= Logger::getInstance();
    gameHasBeenInitiated = true;

	Chrono chrono;
	bool allReady= false;

	chrono.start();

	//wait until we get a ready message from all clients
	while(allReady == false) {
	    vector<string> waitingForHosts;
		allReady= true;
		for(int i= 0; i<GameConstants::maxPlayers; ++i)	{
			ConnectionSlot* connectionSlot= slots[i];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true)	{
				if(connectionSlot->isReady() == false) {
					NetworkMessageType networkMessageType= connectionSlot->getNextMessageType(true);

					// consume old messages from the lobby
					bool discarded = shouldDiscardNetworkMessage(networkMessageType,connectionSlot);
					if(discarded == false) {
						NetworkMessageReady networkMessageReady;
						if(networkMessageType == nmtReady &&
						   connectionSlot->receiveMessage(&networkMessageReady)) {
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] networkMessageType==nmtReady\n",__FUNCTION__);

							connectionSlot->setReady();
						}
						else if(networkMessageType != nmtInvalid) {
							//throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
							string sErr = "Unexpected network message: " + intToStr(networkMessageType);
							sendTextMessage(sErr,-1, true);
							DisplayErrorMessage(sErr);
							return;
						}
					}
					//waitingForHosts.push_back(connectionSlot->getHostName());
					waitingForHosts.push_back(connectionSlot->getName());

					allReady= false;
				}
			}
		}

		//check for timeout
		if(allReady == false) {
            if(chrono.getMillis() > readyWaitTimeout) {
                //throw runtime_error("Timeout waiting for clients");
                string sErr = "Timeout waiting for clients.";
                sendTextMessage(sErr,-1, true);
                DisplayErrorMessage(sErr);
                return;
            }
            else {
                if(chrono.getMillis() % 1000 == 0) {
                    string waitForHosts = "";
                    for(int i = 0; i < waitingForHosts.size(); i++) {
                        if(waitForHosts != "") {
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

    try {
		//send ready message after, so clients start delayed
		for(int i= 0; i < GameConstants::maxPlayers; ++i) {
			ConnectionSlot* connectionSlot= slots[i];
			if(connectionSlot != NULL && connectionSlot->isConnected() == true) {
				NetworkMessageReady networkMessageReady(checksum->getSum());
				connectionSlot->sendMessage(&networkMessageReady);
			}
		}

		gameStartTime = time(NULL);
    }
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		DisplayErrorMessage(ex.what());
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] END\n",__FUNCTION__);
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex, bool echoLocal) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] text [%s] teamIndex = %d, echoLocal = %d\n",__FILE__,__FUNCTION__,__LINE__,text.c_str(),teamIndex,echoLocal);

	NetworkMessageText networkMessageText(text, Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()), teamIndex);
	broadcastMessage(&networkMessageText);

	if(echoLocal == true) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

		ChatMsgInfo msg(text.c_str(),networkMessageText.getSender().c_str(),teamIndex);
		this->addChatInfo(msg);
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::quitGame(bool userManuallyQuit)
{
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    if(userManuallyQuit == true) {
        //string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
        //NetworkMessageText networkMessageText(sQuitText,getHostName(),-1);
        //broadcastMessage(&networkMessageText, -1);
    }

	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

string ServerInterface::getNetworkStatus() {
	Lang &lang= Lang::getInstance();
	string str;

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		str+= intToStr(i)+ ": ";

		if(connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				int clientLagCount = connectionSlot->getCurrentLagCount();
				double lastClientCommandListTimeLag = difftime(time(NULL),connectionSlot->getLastReceiveCommandListTime());
				//float pingTime = connectionSlot->getThreadedPingMS(connectionSlot->getIpAddress().c_str());
				char szBuf[100]="";
				//sprintf(szBuf,", lag = %d [%.2f], ping = %.2fms",clientLagCount,lastClientCommandListTimeLag,pingTime);
				sprintf(szBuf,", lag = %d [%.2f]",clientLagCount,lastClientCommandListTimeLag);

                str+= connectionSlot->getName() + string(szBuf);
			}
		}
		else
		{
			str+= lang.get("NotConnected");
		}

		str+= '\n';
	}

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

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

void ServerInterface::broadcastGameSetup(const GameSettings* gameSettings) {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
    
    MutexSafeWrapper safeMutex(&serverSynchAccessor);
    NetworkMessageLaunch networkMessageLaunch(gameSettings,nmtBroadCastSetup);
    broadcastMessage(&networkMessageLaunch);
    
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot){
    //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    try {

    	if(enabledThreadedClientCommandBroadcast == true) {
			// Step #1 signal worker threads to send this broadcast to each client
			std::map<int,ConnectionSlotEvent> eventList;
			for(int i= 0; i<GameConstants::maxPlayers; ++i) {
				ConnectionSlot* connectionSlot = slots[i];

				// New lag check
				bool clientLagExceeded = false;
				if( gameHasBeenInitiated == true && connectionSlot != NULL &&
					connectionSlot->isConnected() == true) {
					clientLagExceeded = clientLagCheck(connectionSlot);
				}
				if(clientLagExceeded == false) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkMessage = %p\n",__FILE__,__FUNCTION__,__LINE__,networkMessage);

					ConnectionSlotEvent &event = eventList[i];
					event.eventType = eSendSocketData;
					event.networkMessage = networkMessage;
					event.connectionSlot = connectionSlot;
					event.socketTriggered = true;
					event.triggerId = i;

					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

					// Step #1 tell all connection slot worker threads to receive socket data
					if(i != excludeSlot && connectionSlot != NULL) {
						if(connectionSlot->isConnected()) {
							connectionSlot->signalUpdate(&event);
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
						}
						else if(gameHasBeenInitiated == true) {

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
							removeSlot(i);
						}
					}
					else if(i == excludeSlot && gameHasBeenInitiated == true &&
							connectionSlot != NULL && connectionSlot->isConnected() == false) {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
						removeSlot(i);
					}
				}
			}

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			// Step #2 check all connection slot worker threads for completed status
			std::map<int,bool> slotsCompleted;
			for(bool threadsDone = false; threadsDone == false;) {
				threadsDone = true;
				// Examine all threads for completion of delegation
				for(int i= 0; i< GameConstants::maxPlayers; ++i) {
					ConnectionSlot* connectionSlot = slots[i];
					if(connectionSlot != NULL && slotsCompleted.find(i) == slotsCompleted.end()) {
						std::vector<std::string> errorList = connectionSlot->getThreadErrorList();
						if(errorList.size() > 0) {
							for(int iErrIdx = 0; iErrIdx < errorList.size(); ++iErrIdx) {
								string &sErr = errorList[iErrIdx];
								DisplayErrorMessage(sErr);
							}
							connectionSlot->clearThreadErrorList();
						}

						if(connectionSlot->updateCompleted() == false) {
							threadsDone = false;
							break;
						}
						else {
							slotsCompleted[i] = true;
						}
					}
				}
				sleep(0);
			}
    	}
    	else {
			for(int i= 0; i<GameConstants::maxPlayers; ++i) {
				ConnectionSlot* connectionSlot= slots[i];

				if(i != excludeSlot && connectionSlot != NULL) {
					// New lag check
					bool clientLagExceeded = false;
					if( gameHasBeenInitiated == true && connectionSlot != NULL &&
						connectionSlot->isConnected() == true) {
						clientLagExceeded = clientLagCheck(connectionSlot);
					}
					if(clientLagExceeded == false) {
						if(connectionSlot->isConnected()) {
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] before sendMessage\n",__FILE__,__FUNCTION__,__LINE__);
							connectionSlot->sendMessage(networkMessage);
						}
						else if(gameHasBeenInitiated == true) {

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,__LINE__,i);
							removeSlot(i);
						}
					}
				}
				else if(i == excludeSlot && gameHasBeenInitiated == true &&
						connectionSlot != NULL && connectionSlot->isConnected() == false) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 before removeSlot for slot# %d\n",__FILE__,__FUNCTION__,i);
					removeSlot(i);
				}
			}
    	}
    }
    catch(const exception &ex) {
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	//throw runtime_error(ex.what());

    	//DisplayErrorMessage(ex.what());
		string sMsg = ex.what();
		sendTextMessage(sMsg,-1, true);
    }

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::broadcastMessageToConnectedClients(const NetworkMessage* networkMessage, int excludeSlot){
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	try {
		for(int i= 0; i<GameConstants::maxPlayers; ++i) {
			ConnectionSlot* connectionSlot= slots[i];

			if(i!= excludeSlot && connectionSlot!= NULL) {
				if(connectionSlot->isConnected()) {

					//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] before sendMessage\n",__FILE__,__FUNCTION__);
					connectionSlot->sendMessage(networkMessage);
				}
			}
		}
	}
    catch(const exception &ex) {
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	//throw runtime_error(ex.what());
    	DisplayErrorMessage(ex.what());
    }

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerInterface::updateListen() {
	if(gameHasBeenInitiated == true) {
		return;
	}

	int openSlotCount= 0;
	for(int i= 0; i<GameConstants::maxPlayers; ++i)	{
		bool isSlotOpen = (slots[i] != NULL && slots[i]->isConnected() == false);

		if(isSlotOpen == true) {
			++openSlotCount;
		}
	}

	MutexSafeWrapper safeMutex(&serverSynchAccessor);
	serverSocket.listen(openSlotCount);
	safeMutex.ReleaseLock();
}

int ServerInterface::getOpenSlotCount() {
	int openSlotCount= 0;

	for(int i= 0; i<GameConstants::maxPlayers; ++i)	{
		bool isSlotOpen = (slots[i] != NULL && slots[i]->isConnected() == false);

		if(isSlotOpen == true) {
			++openSlotCount;
		}
	}

	return openSlotCount;
}

void ServerInterface::setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck)
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START gameSettingsUpdateCount = %d, waitForClientAck = %d\n",__FILE__,__FUNCTION__,gameSettingsUpdateCount,waitForClientAck);

    MutexSafeWrapper safeMutex(&serverSynchAccessor);

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
