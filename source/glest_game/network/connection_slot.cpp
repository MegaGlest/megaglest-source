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

#include "connection_slot.h"

#include "conversion.h"
#include "game_util.h"
#include "config.h"
#include "server_interface.h"
#include "network_message.h"
#include "platform_util.h"
#include <stdexcept>

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ConnectionSlotThread
// =====================================================

ConnectionSlotThread::ConnectionSlotThread(int slotIndex) : BaseThread() {
	this->masterController 	= NULL;
	this->triggerIdMutex 	= new Mutex(CODE_AT_LINE);
	this->slotIndex 		= slotIndex;
	this->slotInterface 	= NULL;
	uniqueID 				= "ConnectionSlotThread";
	eventList.clear();
	eventList.reserve(1000);

	triggerGameStarted 		= new Mutex(CODE_AT_LINE);
	gameStarted 			= false;
}

ConnectionSlotThread::ConnectionSlotThread(ConnectionSlotCallbackInterface *slotInterface,int slotIndex) : BaseThread() {
	this->masterController 	= NULL;
	this->triggerIdMutex 	= new Mutex(CODE_AT_LINE);
	this->slotIndex 		= slotIndex;
	this->slotInterface 	= slotInterface;
	uniqueID 				= "ConnectionSlotThread";
	eventList.clear();

	triggerGameStarted 		= new Mutex(CODE_AT_LINE);
	gameStarted 			= false;
}

ConnectionSlotThread::~ConnectionSlotThread() {
	delete triggerIdMutex;
	triggerIdMutex = NULL;

	delete triggerGameStarted;
	triggerGameStarted = NULL;
}

void ConnectionSlotThread::setQuitStatus(bool value) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	BaseThread::setQuitStatus(value);
	if(value == true) {
		signalUpdate(NULL);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ConnectionSlotThread::signalUpdate(ConnectionSlotEvent *event) {
	if(event != NULL) {
		MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
		eventList.push_back(*event);
	}
	if(getGameStarted() == true && getQuitStatus() == true) {
		return;
	}
	semTaskSignalled.signal();
}

void ConnectionSlotThread::setTaskCompleted(int eventId) {
	if(eventId > 0) {
		MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
		for(int index = 0; index < (int)eventList.size(); ++index) {
		    ConnectionSlotEvent &slotEvent = eventList[index];
		    if(slotEvent.eventId == eventId) {
                slotEvent.eventCompleted = true;
                break;
		    }
		}
	}
}

void ConnectionSlotThread::purgeAllEvents() {
    MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
    eventList.clear();
}

void ConnectionSlotThread::setAllEventsCompleted() {
    MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
    for(int index = 0; index < (int)eventList.size(); ++index) {
        ConnectionSlotEvent &slotEvent = eventList[index];
        if(slotEvent.eventCompleted == false) {
            slotEvent.eventCompleted = true;
        }
    }
}

void ConnectionSlotThread::purgeCompletedEvents() {
    MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
    for(int index = (int)eventList.size() - 1; index >= 0; index--) {
        ConnectionSlotEvent &slotEvent = eventList[index];
        if(slotEvent.eventCompleted == true) {
            eventList.erase(eventList.begin() + index);
        }
    }
}

bool ConnectionSlotThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    deleteSelfIfRequired();
	    signalQuit();
	}

	return ret;
}

bool ConnectionSlotThread::isSignalCompleted(ConnectionSlotEvent *event) {
	bool result = false;
    if(event != NULL) {
    	MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
        for(int index = 0; index < (int)eventList.size(); ++index) {
            ConnectionSlotEvent &slotEvent = eventList[index];
            if(slotEvent.eventId == event->eventId) {
                result = slotEvent.eventCompleted;
                break;
            }
        }
    }
	return result;
}

void ConnectionSlotThread::slotUpdateTask(ConnectionSlotEvent *event) {
	if(event != NULL && event->connectionSlot != NULL) {
		if(event->eventType == eSendSocketData) {
			event->connectionSlot->sendMessage(event->networkMessage);
		}
		else if(event->eventType == eReceiveSocketData) {
			event->connectionSlot->updateSlot(event);
		}
	}
}

void ConnectionSlotThread::signalSlave(void *userdata) {
	std::map<int,ConnectionSlotEvent> *eventList = (std::map<int,ConnectionSlotEvent> *)userdata;
	ConnectionSlotEvent &event = (*eventList)[slotIndex];
	signalUpdate(&event);
}

bool ConnectionSlotThread::getGameStarted() {
	MutexSafeWrapper safeMutexGameStarted(triggerGameStarted,CODE_AT_LINE);
	return gameStarted;
}
void ConnectionSlotThread::setGameStarted(bool value) {
	MutexSafeWrapper safeMutexGameStarted(triggerGameStarted,CODE_AT_LINE);
	if(gameStarted != value) {
		gameStarted = value;

		if(gameStarted == true) {
			semTaskSignalled.signal();
		}
	}
}

void ConnectionSlotThread::execute() {
    RunningStatusSafeWrapper runningStatus(this);
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//printf("Starting client SLOT thread: %d\n",slotIndex);

		for(;this->slotInterface != NULL;) {
			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			// Does this game allow joining in progress play and is this slot
			// not already connected to a client?
			if( this->slotInterface->getAllowInGameConnections() == true &&
				this->slotInterface->isClientConnected(slotIndex) == false) {
				//printf("#1 Non connected slot: %d waiting for client connection..\n",slotIndex);
				sleep(100);

				if(getQuitStatus() == true) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}

				//printf("Slot thread slotIndex: %d eventCount: %d\n",slotIndex,eventCount);

				ConnectionSlotEvent eventCopy;
				eventCopy.eventType 		= eReceiveSocketData;
				eventCopy.connectionSlot 	= this->slotInterface->getSlot(slotIndex);

				if(getQuitStatus() == true) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}

				ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);
				this->slotUpdateTask(&eventCopy);
			}
			else {
				// If the game already started?
				if(getGameStarted() == true) {
					//printf("#A Checking action for slot: %d\n",slotIndex);

					if(getQuitStatus() == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}

					ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);

					// If the slot or socket are NULL the connection was lost
					// so exit the thread
					ConnectionSlot *slot = this->slotInterface->getSlot(slotIndex);
					if(slot == NULL) {
						break;
					}
					Socket *socket = slot->getSocket(true);
					if(socket == NULL) {
						break;
					}

					// Avoid mutex locking
					//bool socketHasReadData = Socket::hasDataToRead(socket->getSocketId());
					bool socketHasReadData = Socket::hasDataToReadWithWait(socket->getSocketId(),150000);

					ConnectionSlotEvent eventCopy;
					eventCopy.eventType 		= eReceiveSocketData;
					eventCopy.connectionSlot 	= slot;
					eventCopy.eventId 			= slotIndex;
					eventCopy.socketTriggered 	= socketHasReadData;

					if(getQuitStatus() == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}

					this->slotUpdateTask(&eventCopy);
				}
				// Game has not yet started
				else {
					//printf("#1 Checking action for slot: %d\n",slotIndex);

					if(getGameStarted() == true) {
						continue;
					}
					//printf("#2 Checking action for slot: %d\n",slotIndex);

					semTaskSignalled.waitTillSignalled();
					//printf("#3 Checking action for slot: %d\n",slotIndex);

					if(getGameStarted() == true) {
						continue;
					}
					//printf("#4 Checking action for slot: %d\n",slotIndex);

					static string masterSlaveOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
					MasterSlaveThreadControllerSafeWrapper safeMasterController(masterController,20000,masterSlaveOwnerId);
					if(getQuitStatus() == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}

					MutexSafeWrapper safeMutex(triggerIdMutex,CODE_AT_LINE);
					int eventCount = (int)eventList.size();

					//printf("Slot thread slotIndex: %d eventCount: %d\n",slotIndex,eventCount);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Slot thread slotIndex: %d eventCount: %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex,eventCount);

					if(eventCount > 0) {
						ConnectionSlotEvent eventCopy;
						for(int i = 0; i < (int)eventList.size(); ++i) {
							ConnectionSlotEvent &slotEvent = eventList[i];
							if(slotEvent.eventCompleted == false) {
								eventCopy = slotEvent;
								break;
							}
						}
						safeMutex.ReleaseLock();

						if(getQuitStatus() == true) {
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							break;
						}

						if(eventCopy.eventId > 0) {
							ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Slot thread slotIndex: %d eventCount: %d eventCopy.eventId: %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex,eventCount,(int)eventCopy.eventId);
							//printf("#1 Slot thread slotIndex: %d eventCount: %d eventCopy.eventId: %d\n",slotIndex,eventCount,(int)eventCopy.eventId);

							this->slotUpdateTask(&eventCopy);
							setTaskCompleted(eventCopy.eventId);

							//printf("#2 Slot thread slotIndex: %d eventCount: %d eventCopy.eventId: %d\n",slotIndex,eventCount,(int)eventCopy.eventId);
						}
					}
				}
			}

			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
		}

		//printf("Ending client SLOT thread: %d\n",slotIndex);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &ex) {

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

// =====================================================
//	class ConnectionSlot
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	this->mutexSocket 						= new Mutex(CODE_AT_LINE);
	this->socket 							= NULL;
	this->mutexCloseConnection 				= new Mutex(CODE_AT_LINE);
	this->mutexPendingNetworkCommandList 	= new Mutex(CODE_AT_LINE);
	this->socketSynchAccessor 				= new Mutex(CODE_AT_LINE);
    this->connectedRemoteIPAddress 			= 0;
	this->sessionKey 						= 0;
	this->serverInterface					= serverInterface;
	this->playerIndex						= playerIndex;
	this->playerStatus						= npst_None;
	this->playerLanguage					= "";
	this->playerUUID						= "";
	this->platform							= "";
	this->currentFrameCount 				= 0;
	this->currentLagCount					= 0;
	this->gotLagCountWarning 				= false;
	this->lastReceiveCommandListTime		= 0;
	this->receivedNetworkGameStatus 		= false;

	this->autoPauseGameCountForLag			= 0;
	this->skipLagCheck 						= false;
	this->joinGameInProgress 				= false;
	this->canAcceptConnections 				= true;
	this->startInGameConnectionLaunch 		= false;
	this->pauseForInGameConnection 			= false;
	this->unPauseForInGameConnection 		= false;
	this->sentSavedGameInfo 				= false;

	this->ready								= false;
	this->gotIntro 							= false;
	this->connectedTime 					= 0;

	this->networkGameDataSynchCheckOkMap    = false;
	this->networkGameDataSynchCheckOkTile   = false;
	this->networkGameDataSynchCheckOkTech   = false;
	this->setNetworkGameDataSynchCheckTechMismatchReport("");
	this->setReceivedDataSynchCheck(false);

	this->clearChatInfo();

	this->setSocket(NULL);
	this->slotThreadWorker 					= NULL;
	static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
	this->slotThreadWorker 					= new ConnectionSlotThread(this->serverInterface,playerIndex);
	this->slotThreadWorker->setUniqueID(mutexOwnerId);
	this->slotThreadWorker->start();
}

ConnectionSlot::~ConnectionSlot() {
	//printf("===> Destructor for ConnectionSlot = %d\n",playerIndex);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] START\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("Deleting connection slot\n");
	close();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("#1 Ending client SLOT: %d slotThreadWorker: %p\n",playerIndex,slotThreadWorker);
	if(slotThreadWorker != NULL) {
		slotThreadWorker->signalQuit();
	}
	if( slotThreadWorker != NULL &&
		slotThreadWorker->canShutdown(false) == true &&
		slotThreadWorker->getRunningStatus() == false) {
		//printf("#2 Ending client SLOT: %d\n",playerIndex);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		delete slotThreadWorker;

        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else if(slotThreadWorker != NULL &&
			slotThreadWorker->canShutdown(true) == true) {

		if(slotThreadWorker->getRunningStatus() == false) {
			//printf("#3 Ending client SLOT: %d\n",playerIndex);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			delete slotThreadWorker;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		else {
			slotThreadWorker->setDeleteSelfOnExecutionDone(true);
			slotThreadWorker->setDeleteAfterExecute(true);
		}
	}
	//printf("#4 Ending client SLOT: %d\n",playerIndex);
	slotThreadWorker = NULL;

	delete socketSynchAccessor;
	socketSynchAccessor = NULL;

	delete mutexPendingNetworkCommandList;
	mutexPendingNetworkCommandList = NULL;

	delete mutexCloseConnection;
	mutexCloseConnection = NULL;

	delete mutexSocket;
	mutexSocket = NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

int ConnectionSlot::getAutoPauseGameCountForLag() {
	return autoPauseGameCountForLag;
}

void ConnectionSlot::incrementAutoPauseGameCountForLag() {
	autoPauseGameCountForLag++;
}

bool ConnectionSlot::getGameStarted() {
	bool result = false;
	if(this->slotThreadWorker != NULL) {
		result = this->slotThreadWorker->getGameStarted();
	}
	return result;
}
void ConnectionSlot::setGameStarted(bool value) {
	if(this->slotThreadWorker != NULL) {
		this->slotThreadWorker->setGameStarted(value);
	}
}

void ConnectionSlot::setPlayerIndex(int value) {
	playerIndex = value;

	if(this->slotThreadWorker != NULL) {
		this->slotThreadWorker->setSlotIndex(playerIndex);
	}
}

void ConnectionSlot::setReady()	{
	this->ready= true;
	this->skipLagCheck = false;
	this->joinGameInProgress = false;
	this->sentSavedGameInfo = false;
}

void ConnectionSlot::updateSlot(ConnectionSlotEvent *event) {
	if(event != NULL) {
		bool &socketTriggered = event->socketTriggered;
		bool checkForNewClients =
				(serverInterface->getGameHasBeenInitiated() == false ||
					serverInterface->getAllowInGameConnections() == true);

		//if((serverInterface->getGameHasBeenInitiated() == false ||
		//	(serverInterface->getAllowInGameConnections() == true && this->isConnected() == false) ||
		//	socketTriggered == true)) {
		if(socketTriggered == true ||
			((serverInterface->getGameHasBeenInitiated() == false ||
			  serverInterface->getAllowInGameConnections() == true) &&
				this->isConnected() == false)) {

			this->update(checkForNewClients,event->triggerId);
		}
		//}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ConnectionSlot::update(bool checkForNewClients,int lockedSlotIndex) {
	try {
		clearThreadErrorList();

	    if(slotThreadWorker != NULL) {
	        slotThreadWorker->purgeCompletedEvents();
	    }

	    pair<bool,Socket*> socketInfo = this->getSocketInfo();
		if(socketInfo.second == NULL) {
			if(networkGameDataSynchCheckOkMap) networkGameDataSynchCheckOkMap  = false;
			if(networkGameDataSynchCheckOkTile) networkGameDataSynchCheckOkTile = false;
			if(networkGameDataSynchCheckOkTech) networkGameDataSynchCheckOkTech = false;
			this->setReceivedDataSynchCheck(false);

			// Is the listener socket ready to be read?
			if(checkForNewClients == true && this->canAcceptConnections == true) {

				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] BEFORE accept new client connection, serverInterface->getOpenSlotCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getOpenSlotCount());

				//printf("Checking for new connections...\n");
				bool hasData = (serverInterface->getServerSocket() != NULL &&
								serverInterface->getServerSocket()->hasDataToRead() == true);
				//printf("Server socket hasData: %d\n",hasData);

				if(hasData == true) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to accept new client connection playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

					Socket *newSocket = serverInterface->getServerSocket()->accept(false);

					//printf("Server socket newSocket: %p\n",newSocket);

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] called accept new client connection playerIndex = %d newSocket = %p\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,newSocket);
					if(newSocket != NULL) {
						// Set Socket as non-blocking
						newSocket->setBlock(false);

						MutexSafeWrapper safeMutex(mutexCloseConnection,CODE_AT_LINE);
						this->setSocket(newSocket);
						safeMutex.ReleaseLock();

						this->connectedTime = time(NULL);
						this->clearChatInfo();
						this->name = "";
						this->playerStatus = npst_PickSettings;
						this->playerLanguage = "";
						this->playerUUID     = "";
						this->platform		 = "";
						this->ready = false;
						this->vctFileList.clear();
						this->receivedNetworkGameStatus = false;
						this->gotIntro = false;

						MutexSafeWrapper safeMutexSlot1(mutexPendingNetworkCommandList,CODE_AT_LINE);
						this->vctPendingNetworkCommandList.clear();
						safeMutexSlot1.ReleaseLock();

						this->currentFrameCount = 0;
						this->currentLagCount = 0;
						this->lastReceiveCommandListTime = 0;
						this->gotLagCountWarning = false;
						this->versionString = "";

						serverInterface->updateListen();
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
					}
					else {
						close();
						return;
					}

					//send intro message when connected
					if(this->isConnected() == true) {
						//printf("Server socket newSocket is connected: %d\n",playerIndex);

						Chrono seed(true);
						srand((unsigned int)seed.getCurTicks() / (this->playerIndex + 1));

						sessionKey = rand() % 1000000;

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] accepted new client connection, serverInterface->getOpenSlotCount() = %d, sessionKey = %d\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getOpenSlotCount(),sessionKey);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] client will be assigned to the next open slot\n",__FILE__,__FUNCTION__,__LINE__);

						NetworkMessageIntro networkMessageIntro(
								sessionKey,
								getNetworkVersionGITString(),
								getHostName(),
								playerIndex,
								nmgstOk,
								0,
								ServerSocket::getFTPServerPort(),
								"",
								serverInterface->getGameHasBeenInitiated(),
								Config::getInstance().getString("PlayerId",""),
								getPlatformNameString());
						sendMessage(&networkMessageIntro);

						if(this->serverInterface->getGameHasBeenInitiated() == true) {
							setGameStarted(true);
						}
					}
				}
			}
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(socketInfo.first == true) {
				this->clearChatInfo();

				bool gotTextMsg = true;
				bool gotCellMarkerMsg = true;
				bool waitForLaggingClient = false;
				bool waitedForLaggingClient = false;

				//printf("Update slot: %d this->hasDataToRead(): %d\n",this->playerIndex,this->hasDataToRead());

				for(;waitForLaggingClient == true ||
						(this->hasDataToRead() == true &&
						 (gotTextMsg == true || gotCellMarkerMsg == true));) {

					//printf("Server slot checking for waitForLaggingClient = %d this->hasDataToRead() = %d gotTextMsg = %d gotCellMarkerMsg = %d\n",waitForLaggingClient,this->hasDataToRead(),gotTextMsg,gotCellMarkerMsg);

					waitForLaggingClient = false;
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] polling for networkMessageType...\n",__FILE__,__FUNCTION__,__LINE__);

					NetworkMessageType networkMessageType= getNextMessageType();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkMessageType = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageType);

					gotTextMsg = false;
					gotCellMarkerMsg = false;
					//process incoming commands
					switch(networkMessageType) {

						case nmtInvalid:
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtInvalid\n",__FILE__,__FUNCTION__,__LINE__);
							break;

						case nmtPing:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtPing\n",__FILE__,__FUNCTION__);

							// client REQUIRES a ping before completing intro
							// authentication
							NetworkMessagePing networkMessagePing;
							if(receiveMessage(&networkMessagePing)) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								lastPingInfo = networkMessagePing;
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtText:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtText gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								NetworkMessageText networkMessageText;
								if(receiveMessage(&networkMessageText)) {
									ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex(),networkMessageText.getTargetLanguage());
									this->addChatInfo(msg);
									gotTextMsg = true;
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtMarkCell:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtMarkCell gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								NetworkMessageMarkCell networkMessageMarkCell;
								if(receiveMessage(&networkMessageMarkCell)) {
					            	MarkedCell msg(networkMessageMarkCell.getTarget(),
					            			       networkMessageMarkCell.getFactionIndex(),
					            			       networkMessageMarkCell.getText().c_str(),
					            			       networkMessageMarkCell.getPlayerIndex());

					            	this->addMarkedCell(msg);
					            	gotCellMarkerMsg = true;
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtUnMarkCell:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtUnMarkCell gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								NetworkMessageUnMarkCell networkMessageMarkCell;
								if(receiveMessage(&networkMessageMarkCell)) {
					            	UnMarkedCell msg(networkMessageMarkCell.getTarget(),
					            			       networkMessageMarkCell.getFactionIndex());

					            	this->addUnMarkedCell(msg);
					            	gotCellMarkerMsg = true;
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtHighlightCell:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtMarkCell gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								NetworkMessageHighlightCell networkMessageHighlightCell;
								if(receiveMessage(&networkMessageHighlightCell)) {
					            	MarkedCell msg(networkMessageHighlightCell.getTarget(),
					            			networkMessageHighlightCell.getFactionIndex(),"none",-1);

					            	this->setHighlightedCell(msg);
					            	gotCellMarkerMsg = true;
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						//command list
						case nmtCommandList: {

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtCommandList gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								NetworkMessageCommandList networkMessageCommandList;
								if(receiveMessage(&networkMessageCommandList)) {
									currentFrameCount = networkMessageCommandList.getFrameCount();
									lastReceiveCommandListTime = time(NULL);

									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] currentFrameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,currentFrameCount);

									MutexSafeWrapper safeMutexSlot(mutexPendingNetworkCommandList,CODE_AT_LINE);
									for(int i = 0; i < networkMessageCommandList.getCommandCount(); ++i) {
										vctPendingNetworkCommandList.push_back(*networkMessageCommandList.getCommand(i));
									}
									//printf("Got commands from client frame: %d count: %d\n",currentFrameCount,vctPendingNetworkCommandList.size());

									//printf("#2 Server slot got currentFrameCount = %d\n",currentFrameCount);
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						//process intro messages
						case nmtIntro:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtIntro\n",__FILE__,__FUNCTION__);

							NetworkMessageIntro networkMessageIntro;
							if(receiveMessage(&networkMessageIntro)) {
								int32 msgSessionId = networkMessageIntro.getSessionId();
								this->name= networkMessageIntro.getName();
								this->versionString = networkMessageIntro.getVersionString();
								this->connectedRemoteIPAddress = networkMessageIntro.getExternalIp();
								this->playerLanguage = networkMessageIntro.getPlayerLanguage();
								this->playerUUID	  = networkMessageIntro.getPlayerUUID();
								this->platform		  = networkMessageIntro.getPlayerPlatform();

								//printf("Got uuid from client [%s]\n",this->playerUUID.c_str());
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got name [%s] versionString [%s], msgSessionId = %d\n",__FILE__,__FUNCTION__,name.c_str(),versionString.c_str(),msgSessionId);

								if(msgSessionId != sessionKey) {
									string playerNameStr = name;
									string sErr = "Client gave invalid sessionid for player [" + playerNameStr + "] actual [" + intToStr(msgSessionId) + "] expected [" + intToStr(sessionKey) + "]";
									printf("%s\n",sErr.c_str());
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());

									close();
									return;
								}
								else if(this->playerUUID == "") {
									string playerNameStr = name;
									string sErr = "Client gave an invalid UUID for player [" + playerNameStr + "]";
									printf("%s\n",sErr.c_str());
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());

									close();
									return;
								}
								else {
									//check consistency
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

									bool compatible = checkVersionComptability(getNetworkVersionGITString(), networkMessageIntro.getVersionString());

									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

									if(compatible == false) {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

										bool versionMatched = false;
										string platformFreeVersion = getNetworkPlatformFreeVersionString();
										string sErr = "";

										if(strncmp(platformFreeVersion.c_str(),networkMessageIntro.getVersionString().c_str(),strlen(platformFreeVersion.c_str())) != 0) {
											string playerNameStr = name;
											sErr = "Server and client binary mismatch!\nYou have to use the exactly same binaries!\n\nServer: " +  getNetworkVersionGITString() +
													"\nClient: " + networkMessageIntro.getVersionString() + " player [" + playerNameStr + "]";
											printf("%s\n",sErr.c_str());

											serverInterface->sendTextMessage("Server and client binary mismatch!!",-1, true,"",lockedSlotIndex);
											serverInterface->sendTextMessage(" Server:" + getNetworkVersionGITString(),-1, true,"",lockedSlotIndex);
											serverInterface->sendTextMessage(" Client: "+ networkMessageIntro.getVersionString(),-1, true,"",lockedSlotIndex);
											serverInterface->sendTextMessage(" Client player [" + playerNameStr + "]",-1, true,"",lockedSlotIndex);
										}
										else {
											versionMatched = true;

											string playerNameStr = name;
											sErr = "Warning, Server and client are using the same version but different platforms.\n\nServer: " +  getNetworkVersionGITString() +
													"\nClient: " + networkMessageIntro.getVersionString() + " player [" + playerNameStr + "]";
											//printf("%s\n",sErr.c_str());
											if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());
										}

										if(Config::getInstance().getBool("PlatformConsistencyChecks","true") &&
										   versionMatched == false) { // error message and disconnect only if checked
											if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());
											close();
											if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());
											return;
										}
									}

									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
									gotIntro = true;

									int factionIndex = this->serverInterface->gameSettings.getFactionIndexForStartLocation(playerIndex);
									this->serverInterface->addClientToServerIPAddress(this->getSocket()->getConnectedIPAddress(this->getSocket()->getIpAddress()),this->connectedRemoteIPAddress);

									this->serverInterface->gameSettings.setNetworkPlayerUUID(factionIndex,this->playerUUID);
									this->serverInterface->gameSettings.setNetworkPlayerPlatform(factionIndex,this->platform);

									if(serverInterface->getGameHasBeenInitiated() == true &&
									   serverInterface->getAllowInGameConnections() == true) {
										this->serverInterface->gameSettings.setNetworkPlayerStatuses(factionIndex,npst_None);
									}

									if(getAllowGameDataSynchCheck() == true && serverInterface->getGameSettings() != NULL) {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending NetworkMessageSynchNetworkGameData\n",__FILE__,__FUNCTION__,__LINE__);

										NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(serverInterface->getGameSettings());
										sendMessage(&networkMessageSynchNetworkGameData);
									}

									if(serverInterface->getGameHasBeenInitiated() == true &&
									   serverInterface->getAllowInGameConnections() == true) {

										ConnectionSlot *slot = serverInterface->findSlotForUUID(this->playerUUID,true);
										if(slot != NULL) {
											slot->setJoinGameInProgressFlags();
											slot->setPauseForInGameConnection(true);

											serverInterface->switchSlot(this->playerIndex,slot->getPlayerIndex());
										}
										else {
											setJoinGameInProgressFlags();
											this->setPauseForInGameConnection(true);
										}
									}
									else {
										ConnectionSlot *slot = serverInterface->findSlotForUUID(this->playerUUID,true);
										if(slot != NULL) {
											serverInterface->switchSlot(this->playerIndex,slot->getPlayerIndex());
										}
									}
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

				        case nmtLaunch:
				        case nmtBroadCastSetup:
				        {
				        	if(gotIntro == true) {
								if(this->serverInterface->getGameSettings() == NULL ||
									(joinGameInProgress == false && sessionKey != this->serverInterface->getGameSettings()->getMasterserver_admin())) {
									string playerNameStr = name;
									string sErr = "Client has invalid admin sessionid for player [" + playerNameStr + "]";
									printf("%s\n",sErr.c_str());
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());

									close();
									return;
								}

								NetworkMessageLaunch networkMessageLaunch;
								if(receiveMessage(&networkMessageLaunch)) {
									if(networkMessageLaunch.getMessageType() == nmtLaunch) {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtLaunch\n",__FILE__,__FUNCTION__,__LINE__);
										//printf("Got launch request from client joinGameInProgress = %d joinGameInProgress = %d!\n",joinGameInProgress,joinGameInProgress);
									}
									else if(networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got nmtBroadCastSetup\n",__FILE__,__FUNCTION__,__LINE__);
									}
									else {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());

										char szBuf[1024]="";
										snprintf(szBuf,1023,"In [%s::%s Line: %d] Invalid networkMessageLaunch.getMessageType() = %d",__FILE__,__FUNCTION__,__LINE__,networkMessageLaunch.getMessageType());
										throw megaglest_runtime_error(szBuf);
									}

									int minHeadLessPlayersRequired = Config::getInstance().getInt("MinHeadlessPlayersRequired","2");
									if(this->joinGameInProgress == false &&
										networkMessageLaunch.getMessageType() == nmtLaunch &&
										this->ready == false &&
										this->serverInterface->getConnectedSlotCount(true) < minHeadLessPlayersRequired) {

								    	Lang &lang= Lang::getInstance();
								    	const vector<string> languageList = this->serverInterface->getGameSettings()->getUniqueNetworkPlayerLanguages();
								    	for(unsigned int index = 0; index < languageList.size(); ++index) {
								    		char szBuf[4096]="";
											string msgTemplate = "You must have have at least %d player(s) connected to start this game!";
											if(lang.hasString("HeadlessAdminRequiresMorePlayers",languageList[index]) == true) {
												msgTemplate = lang.getString("HeadlessAdminRequiresMorePlayers",languageList[index]);
											}
					#ifdef WIN32
											_snprintf(szBuf,4095,msgTemplate.c_str(),minHeadLessPlayersRequired);
					#else
											snprintf(szBuf,4095,msgTemplate.c_str(),minHeadLessPlayersRequired);
					#endif
											if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

											string sMsg = szBuf;
											bool echoLocal = lang.isLanguageLocal(languageList[index]);
											this->serverInterface->sendTextMessage(sMsg,-1, echoLocal, languageList[index], this->getPlayerIndex());
								    	}
									}
									else {
										if(this->joinGameInProgress == false) {
											GameSettings gameSettingsBuffer;
											networkMessageLaunch.buildGameSettings(&gameSettingsBuffer);

											//printf("Connection slot got networkMessageLaunch.getMessageType() = %d, got map [%s]\n",networkMessageLaunch.getMessageType(),gameSettings.getMap().c_str());
											//printf("\n\n\n\n=====Connection slot got settings:\n%s\n",gameSettings.toString().c_str());

											this->serverInterface->broadcastGameSetup(&gameSettingsBuffer, true);
										}

										if(this->joinGameInProgress == false &&
											networkMessageLaunch.getMessageType() == nmtLaunch) {

											this->serverInterface->setMasterserverAdminRequestLaunch(true);
										}
										else if(this->joinGameInProgress == true &&
												networkMessageLaunch.getMessageType() == nmtLaunch) {
											//printf("!!! setStartInGameConnectionLaunch for client joinGameInProgress = %d!\n",joinGameInProgress);

											int factionIndex = this->serverInterface->gameSettings.getFactionIndexForStartLocation(playerIndex);
											this->serverInterface->gameSettings.setFactionControl(factionIndex,ctNetwork);
											this->serverInterface->gameSettings.setNetworkPlayerName(factionIndex,this->name);
											this->serverInterface->gameSettings.setNetworkPlayerUUID(factionIndex,this->playerUUID);
											this->serverInterface->gameSettings.setNetworkPlayerPlatform(factionIndex,this->platform);

											if(this->serverInterface->gameSettings.getNetworkPlayerStatuses(factionIndex) == npst_Disconnected) {
												this->serverInterface->gameSettings.setNetworkPlayerStatuses(factionIndex,npst_None);
											}

											this->serverInterface->broadcastGameSetup(&this->serverInterface->gameSettings, true);

											this->setStartInGameConnectionLaunch(true);
										}
									}
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
				        	}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
				        }
				        break;

						//process datasynch messages
						case nmtSynchNetworkGameDataStatus:
						{
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtSynchNetworkGameDataStatus, gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

								NetworkMessageSynchNetworkGameDataStatus networkMessageSynchNetworkGameDataStatus;
								if(receiveMessage(&networkMessageSynchNetworkGameDataStatus)) {
									this->setNetworkGameDataSynchCheckTechMismatchReport("");
									this->setReceivedDataSynchCheck(false);

									Config &config = Config::getInstance();
									string scenarioDir = "";
									if(serverInterface->getGameSettings()->getScenarioDir() != "") {
										scenarioDir = serverInterface->getGameSettings()->getScenarioDir();
										if(EndsWith(scenarioDir, ".xml") == true) {
											scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
											scenarioDir = scenarioDir.erase(scenarioDir.size() - serverInterface->getGameSettings()->getScenario().size(), serverInterface->getGameSettings()->getScenario().size() + 1);
										}

										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getGameSettings()->getScenarioDir().c_str(),serverInterface->getGameSettings()->getScenario().c_str(),scenarioDir.c_str());
									}

									//tileset
									uint32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + serverInterface->getGameSettings()->getTileset() + string("/*"), ".xml", NULL);
									uint32 techCRC    = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", NULL);
									Checksum checksum;
									string file = Config::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir,false);
									checksum.addFile(file);
									uint32 mapCRC = checksum.getSum();

									networkGameDataSynchCheckOkMap      = (networkMessageSynchNetworkGameDataStatus.getMapCRC() == mapCRC);
									networkGameDataSynchCheckOkTile     = (networkMessageSynchNetworkGameDataStatus.getTilesetCRC() == tilesetCRC);
									networkGameDataSynchCheckOkTech     = (networkMessageSynchNetworkGameDataStatus.getTechCRC()    == techCRC);

									// For testing
									//techCRC++;

									if( networkGameDataSynchCheckOkMap      == true &&
										networkGameDataSynchCheckOkTile     == true &&
										networkGameDataSynchCheckOkTech     == true) {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] client data synch ok\n",__FILE__,__FUNCTION__);
									}
									else {
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] mapCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,mapCRC,networkMessageSynchNetworkGameDataStatus.getMapCRC());
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] tilesetCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,tilesetCRC,networkMessageSynchNetworkGameDataStatus.getTilesetCRC());
										if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,techCRC,networkMessageSynchNetworkGameDataStatus.getTechCRC());

										if(allowDownloadDataSynch == true) {
											// Now get all filenames with their CRC values and send to the client
											vctFileList.clear();

											Config &config = Config::getInstance();
											string scenarioDir = "";
											if(serverInterface->getGameSettings()->getScenarioDir() != "") {
												scenarioDir = serverInterface->getGameSettings()->getScenarioDir();
												if(EndsWith(scenarioDir, ".xml") == true) {
													scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
													scenarioDir = scenarioDir.erase(scenarioDir.size() - serverInterface->getGameSettings()->getScenario().size(), serverInterface->getGameSettings()->getScenario().size() + 1);
												}

												if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getGameSettings()->getScenarioDir().c_str(),serverInterface->getGameSettings()->getScenario().c_str(),scenarioDir.c_str());
											}

											if(networkGameDataSynchCheckOkTile == false) {
												if(tilesetCRC == 0) {
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + serverInterface->getGameSettings()->getTileset() + string("/*"), "", &vctFileList);
												}
												else {
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTilesets,scenarioDir), "/" + serverInterface->getGameSettings()->getTileset() + "/*", ".xml", &vctFileList);
												}
											}
											if(networkGameDataSynchCheckOkTech == false) {
												if(techCRC == 0) {
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
												}
												else {
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", &vctFileList);
												}

												string report = networkMessageSynchNetworkGameDataStatus.getTechCRCFileMismatchReport(serverInterface->getGameSettings()->getTech(),vctFileList);
												this->setNetworkGameDataSynchCheckTechMismatchReport(report);
											}
											if(networkGameDataSynchCheckOkMap == false) {
												vctFileList.push_back(std::pair<string,uint32>(Config::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir,false),mapCRC));
											}

											NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck((int)vctFileList.size(), 1, vctFileList[0].second, vctFileList[0].first);
											sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
										}
										else {
											if(networkGameDataSynchCheckOkTech == false) {
												vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", NULL);

												string report = networkMessageSynchNetworkGameDataStatus.getTechCRCFileMismatchReport(serverInterface->getGameSettings()->getTech(),vctFileList);
												this->setNetworkGameDataSynchCheckTechMismatchReport(report);
											}
										}
									}

									this->setReceivedDataSynchCheck(true);
									receivedNetworkGameStatus = true;
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtSynchNetworkGameDataFileCRCCheck:
						{

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck\n",__FILE__,__FUNCTION__);

							if(gotIntro == true) {
								NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck;
								if(receiveMessage(&networkMessageSynchNetworkGameDataFileCRCCheck))
								{
									int fileIndex = networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex();
									NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck((int)vctFileList.size(), fileIndex, vctFileList[fileIndex-1].second, vctFileList[fileIndex-1].first);
									sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtSynchNetworkGameDataFileGet:
						{

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileGet\n",__FILE__,__FUNCTION__);

							if(gotIntro == true) {
								NetworkMessageSynchNetworkGameDataFileGet networkMessageSynchNetworkGameDataFileGet;
								if(receiveMessage(&networkMessageSynchNetworkGameDataFileGet)) {
									FileTransferInfo fileInfo;
									fileInfo.hostType   = eServer;
									//fileInfo.serverIP   = this->ip.getString();
									fileInfo.serverPort = Config::getInstance().getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
									fileInfo.fileName   = networkMessageSynchNetworkGameDataFileGet.getFileName();

									FileTransferSocketThread *fileXferThread = new FileTransferSocketThread(fileInfo);
									fileXferThread->start();
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}
						}
						break;

						case nmtSwitchSetupRequest:
						{
							//printf("Got nmtSwitchSetupRequest A gotIntro = %d\n",gotIntro);
							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtSwitchSetupRequest gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								//printf("Got nmtSwitchSetupRequest B\n");

								SwitchSetupRequest switchSetupRequest;
								if(receiveMessage(&switchSetupRequest)) {
									MutexSafeWrapper safeMutex(getServerSynchAccessor(),CODE_AT_LINE);

									int slotIdx = switchSetupRequest.getCurrentSlotIndex();
									//int newSlotIdx = switchSetupRequest.getToSlotIndex();
									//printf("slotIdx = %d newSlotIdx = %d\n",slotIdx,newSlotIdx);

									if(serverInterface->getSwitchSetupRequests(slotIdx) == NULL) {
										serverInterface->setSwitchSetupRequests(slotIdx,new SwitchSetupRequest());
									}
									*(serverInterface->getSwitchSetupRequests(slotIdx)) = switchSetupRequest;

									//printf("slotIdx = %d newSlotIdx = %d\n",serverInterface->getSwitchSetupRequests(slotIdx)->getCurrentSlotIndex(),serverInterface->getSwitchSetupRequests(slotIdx)->getToSlotIndex());

									this->playerStatus = switchSetupRequest.getNetworkPlayerStatus();
									this->name = switchSetupRequest.getNetworkPlayerName();
									this->playerLanguage = switchSetupRequest.getNetworkPlayerLanguage();

									//printf("Got nmtSwitchSetupRequest C\n");
									//printf("In [%s::%s Line %d] networkPlayerName [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getSwitchSetupRequests()[factionIdx]->getNetworkPlayerName().c_str());

									if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] networkPlayerName [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getSwitchSetupRequests()[slotIdx]->getNetworkPlayerName().c_str());
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] factionIdx = %d, switchSetupRequest.getNetworkPlayerName() [%s] switchSetupRequest.getNetworkPlayerStatus() = %d, switchSetupRequest.getSwitchFlags() = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIdx,switchSetupRequest.getNetworkPlayerName().c_str(),switchSetupRequest.getNetworkPlayerStatus(),switchSetupRequest.getSwitchFlags());
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
								this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
								close();
								return;
							}

							break;
						}

						case nmtReady:
						{
							NetworkMessageReady networkMessageReady;
						    this->receiveMessage(&networkMessageReady);

							// its simply ignored here. Probably we are starting a game
							//printf("Got ready message from client slot joinGameInProgress = %d\n",joinGameInProgress);
							if(joinGameInProgress == true) {
								NetworkMessageReady networkMessageReady(0);
								this->sendMessage(&networkMessageReady);
								this->setGameStarted(true);

								this->currentFrameCount = serverInterface->getCurrentFrameCount();
								//printf("#2 Server slot got currentFrameCount = %d\n",currentFrameCount);

								this->currentLagCount = 0;
								this->lastReceiveCommandListTime = time(NULL);

								this->setReady();
							}
							// unpause the game
							else {
								this->setUnPauseForInGameConnection(true);
							}
							break;
						}
			            case nmtLoadingStatusMessage:
			                break;

						default:
							{
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkMessageType = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageType);

								if(gotIntro == true) {
									//throw megaglest_runtime_error("Unexpected message in connection slot: " + intToStr(networkMessageType));
									string sErr = "Unexpected message in connection slot: " + intToStr(networkMessageType);
									//sendTextMessage(sErr,-1);
									//DisplayErrorMessage(sErr);
									threadErrorList.push_back(sErr);
									return;
								}
								else {
									if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got invalid message type before intro, disconnecting socket.\n",__FILE__,__FUNCTION__,__LINE__);

									if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d]\nInvalid message type before intro handshake [%d]\nDisconnecting socket for slot: %d [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,networkMessageType,this->playerIndex,this->getIpAddress().c_str());
									this->serverInterface->notifyBadClientConnectAttempt(this->getIpAddress());
									close();
									return;
								}
							}
					}

					//printf("#3 Server slot got currentFrameCount = %d\n",currentFrameCount);

					// This may end up continuously lagging and not disconnecting players who have
					// just the 'wrong' amount of lag (but not enough to be horrible for a disconnect)
					if(Config::getInstance().getBool("AutoClientLagCorrection","true") == true) {
						double LAG_CHECK_GRACE_PERIOD 		= 15;

						//printf("#4 Server slot got currentFrameCount = %d\n",currentFrameCount);

						if(this->serverInterface->getGameStartTime() > 0 &&
								difftime((long int)time(NULL),this->serverInterface->getGameStartTime()) >= LAG_CHECK_GRACE_PERIOD &&
								difftime((long int)time(NULL),this->getConnectedTime()) >= LAG_CHECK_GRACE_PERIOD) {
							if(this->isConnected() == true && this->gotIntro == true && this->skipLagCheck == false) {
								double clientLag = this->serverInterface->getCurrentFrameCount() - this->getCurrentFrameCount();
								double clientLagCount = (gameSettings.getNetworkFramePeriod() > 0 ? (clientLag / gameSettings.getNetworkFramePeriod()) : 0);
								double clientLagTime = difftime((long int)time(NULL),this->getLastReceiveCommandListTime());

								double maxFrameCountLagAllowed 		= 10;
								double maxClientLagTimeAllowed 		= 8;

								// New lag check
								if((maxFrameCountLagAllowed > 0 && clientLagCount > maxFrameCountLagAllowed) ||
									(maxClientLagTimeAllowed > 0 && clientLagTime > maxClientLagTimeAllowed)) {

									waitForLaggingClient = true;
									if(waitedForLaggingClient == false) {
										waitedForLaggingClient = true;
										printf("*TESTING*: START Waiting for lagging client playerIndex = %d [%s] clientLagCount = %f [%f]\n",playerIndex,name.c_str(),clientLagCount,clientLagTime);
									}
								}
							}
						}

						//printf("#5 Server slot got currentFrameCount = %d\n",currentFrameCount);
					}
					//printf("#5a Server slot got currentFrameCount = %d\n",currentFrameCount);
				}

				//printf("#6 Server slot got currentFrameCount = %d\n",currentFrameCount);

				if(waitedForLaggingClient == true) {
					printf("*TESTING*: FINISHED Waiting for lagging client playerIndex = %d [%s]\n",playerIndex,name.c_str());
				}

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				validateConnection();

				//printf("#7 Server slot got currentFrameCount = %d\n",currentFrameCount);

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] calling close...\n",__FILE__,__FUNCTION__,__LINE__);

				//printf("Closing connection slot socketInfo.first = %d\n",socketInfo.first);

				close();

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());

		threadErrorList.push_back(ex.what());

		//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
	}

	//printf("#8 Server slot got currentFrameCount = %d\n",currentFrameCount);
	//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
}

void ConnectionSlot::validateConnection() {
	if(this->isConnected() == true && 
		gotIntro == false && connectedTime > 0 &&
		difftime((long int)time(NULL),connectedTime) > GameConstants::maxClientConnectHandshakeSecs) {

		//printf("Closing connection slot timed out!\n");
		close();
	}
}

void ConnectionSlot::resetJoinGameInProgressFlags() {
	this->gotIntro 				= false;
	this->skipLagCheck 			= false;
	this->joinGameInProgress 	= false;
	this->ready					= false;
}

void ConnectionSlot::setJoinGameInProgressFlags() {
	this->gotIntro 				= true;
	this->skipLagCheck 			= true;
	this->joinGameInProgress 	= true;
	this->ready					= false;
	this->sentSavedGameInfo 	= false;
}

void ConnectionSlot::close() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("Closing slot for playerIndex = %d\n",playerIndex);
	//if(serverInterface->getAllowInGameConnections() == true) {
		//printf("Closing connection slot!\n");
	//}
	//printf("ConnectionSlot::close() #1 this->getSocket() = %p\n",this->getSocket());

	this->gotIntro 						= false;
	this->skipLagCheck 					= false;
	this->joinGameInProgress 			= false;
	this->sentSavedGameInfo 			= false;
	this->pauseForInGameConnection 		= false;
	this->unPauseForInGameConnection 	= false;
	this->ready							= false;
	this->connectedTime 				= 0;

	if(this->slotThreadWorker != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        this->slotThreadWorker->setAllEventsCompleted();
        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("ConnectionSlot::close() #2 this->getSocket() = %p\n",this->getSocket());

	MutexSafeWrapper safeMutex(mutexCloseConnection,CODE_AT_LINE);
	bool updateServerListener = (this->getSocket() != NULL);

	//printf("ConnectionSlot::close() #3 this->getSocket() = %p updateServerListener = %d\n",this->getSocket(),updateServerListener);

	this->deleteSocket();
	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//printf("Closing slot for playerIndex = %d updateServerListener = %d ready = %d\n",playerIndex,updateServerListener,ready);

    if(updateServerListener == true) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    	serverInterface->updateListen();
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

Mutex * ConnectionSlot::getServerSynchAccessor() {
	return (serverInterface != NULL ? serverInterface->getServerSynchAccessor() : NULL);
}

void ConnectionSlot::signalUpdate(ConnectionSlotEvent *event) {
    if(slotThreadWorker != NULL) {
        slotThreadWorker->signalUpdate(event);
    }
}

bool ConnectionSlot::updateCompleted(ConnectionSlotEvent *event) {
	bool waitingForThread = (slotThreadWorker != NULL &&
							 slotThreadWorker->isSignalCompleted(event) == false &&
							 slotThreadWorker->getQuitStatus() 		    == false &&
							 slotThreadWorker->getRunningStatus() 	    == true);

	return (waitingForThread == false);
}

void ConnectionSlot::sendMessage(NetworkMessage* networkMessage) {
	MutexSafeWrapper safeMutex(socketSynchAccessor,CODE_AT_LINE);

	// Skip text messages not intended for the players preferred language
	NetworkMessageText *textMsg = dynamic_cast<NetworkMessageText *>(networkMessage);
	if(textMsg != NULL) {
		//printf("\n\n\n~~~ SERVER HAS NetworkMessageText target [%s] player [%s] msg[%s]\n\n\n",textMsg->getTargetLanguage().c_str(),this->getNetworkPlayerLanguage().c_str(), textMsg->getText().c_str());
		if(textMsg->getTargetLanguage() != "" &&
			textMsg->getTargetLanguage() != this->getNetworkPlayerLanguage()) {
			return;
		}
	}

	NetworkInterface::sendMessage(networkMessage);
}

string ConnectionSlot::getHumanPlayerName(int index) {
	return serverInterface->getHumanPlayerName(index);
}

vector<NetworkCommand> ConnectionSlot::getPendingNetworkCommandList(bool clearList) {
	vector<NetworkCommand> ret;
	MutexSafeWrapper safeMutexSlot(mutexPendingNetworkCommandList,CODE_AT_LINE);
    if(vctPendingNetworkCommandList.empty() == false) {
    	ret = vctPendingNetworkCommandList;
		if(clearList == true) {
			vctPendingNetworkCommandList.clear();
		}
    }

    return ret;
}

void ConnectionSlot::clearPendingNetworkCommandList() {
	MutexSafeWrapper safeMutexSlot(mutexPendingNetworkCommandList,CODE_AT_LINE);
	if(vctPendingNetworkCommandList.empty() == false) {
		vctPendingNetworkCommandList.clear();
	}
}

bool ConnectionSlot::hasValidSocketId() {
    bool result = false;
    MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
    if(socket != NULL && socket->getSocketId() > 0) {
    	result = true;
    }
	return result;

}

bool ConnectionSlot::isConnected() {
    bool result = false;
    MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
    if(socket != NULL && socket->isConnected() == true) {
    	result = true;
    }
	return result;
}

PLATFORM_SOCKET ConnectionSlot::getSocketId() {
	PLATFORM_SOCKET result = 0;
	MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
	if(socket != NULL) {
		result = socket->getSocketId();
	}
	return result;
}

pair<bool,Socket*> ConnectionSlot::getSocketInfo()	{
	pair<bool,Socket*> result;
	MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
	result.first = (socket != NULL && socket->isConnected());
	result.second = socket;

	return result;

}

Socket* ConnectionSlot::getSocket(bool mutexLock)	{
	MutexSafeWrapper safeMutexSlot(NULL,CODE_AT_LINE);
	if(mutexLock == true) {
		safeMutexSlot.setMutex(mutexSocket,CODE_AT_LINE);
	}
	return socket;
}

void ConnectionSlot::setSocket(Socket *newSocket) {
	MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
	socket = newSocket;
}

void ConnectionSlot::deleteSocket() {
	MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
	delete socket;
	socket = NULL;
}

bool ConnectionSlot::hasDataToRead() {
    bool result = false;

    MutexSafeWrapper safeMutexSlot(mutexSocket,CODE_AT_LINE);
	if(socket != NULL && socket->hasDataToRead() == true) {
		result = true;
	}

	return result;
}

}}//end namespace
