// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "connection_slot.h"

#include <stdexcept>

#include "conversion.h"
#include "game_util.h"
#include "config.h"
#include "server_interface.h"
#include "network_message.h"
#include "leak_dumper.h"

#include "platform_util.h"
#include "map.h"

using namespace std;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ConnectionSlotThread
// =====================================================

ConnectionSlotThread::ConnectionSlotThread(int slotIndex) : BaseThread() {
	this->slotIndex = slotIndex;
	this->slotInterface = NULL;
	//this->event = NULL;
	eventList.clear();
	eventList.reserve(100);
}

ConnectionSlotThread::ConnectionSlotThread(ConnectionSlotCallbackInterface *slotInterface,int slotIndex) : BaseThread() {
	this->slotIndex = slotIndex;
	this->slotInterface = slotInterface;
	//this->event = NULL;
	eventList.clear();
}

void ConnectionSlotThread::setQuitStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	BaseThread::setQuitStatus(value);
	if(value == true) {
		signalUpdate(NULL);
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ConnectionSlotThread::signalUpdate(ConnectionSlotEvent *event) {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] event = %p\n",__FILE__,__FUNCTION__,__LINE__,event);

	if(event != NULL) {
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
		eventList.push_back(*event);
		safeMutex.ReleaseLock();
	}
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	semTaskSignalled.signal();
}

void ConnectionSlotThread::setTaskCompleted(int eventId) {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	if(eventId > 0) {
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
		//event->eventCompleted = true;
		for(int i = 0; i < eventList.size(); ++i) {
		    ConnectionSlotEvent &slotEvent = eventList[i];
		    if(slotEvent.eventId == eventId) {
                //eventList.erase(eventList.begin() + i);
                slotEvent.eventCompleted = true;
                break;
		    }
		}
		safeMutex.ReleaseLock();
	}

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ConnectionSlotThread::purgeAllEvents() {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
    eventList.clear();
    safeMutex.ReleaseLock();
}

void ConnectionSlotThread::setAllEventsCompleted() {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
    for(int i = 0; i < eventList.size(); ++i) {
        ConnectionSlotEvent &slotEvent = eventList[i];
        if(slotEvent.eventCompleted == false) {
            slotEvent.eventCompleted = true;
        }
    }
    safeMutex.ReleaseLock();
}

void ConnectionSlotThread::purgeCompletedEvents() {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
    //event->eventCompleted = true;
    for(int i = eventList.size() - 1; i >= 0; i--) {
        ConnectionSlotEvent &slotEvent = eventList[i];
        if(slotEvent.eventCompleted == true) {
            eventList.erase(eventList.begin() + i);
        }
    }
    safeMutex.ReleaseLock();
}

bool ConnectionSlotThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    signalQuit();
	}

	return ret;
}

bool ConnectionSlotThread::isSignalCompleted(ConnectionSlotEvent *event) {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex);
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
	//bool result = (event != NULL ? event->eventCompleted : true);
	bool result = false;
    if(event != NULL) {
        for(int i = 0; i < eventList.size(); ++i) {
            ConnectionSlotEvent &slotEvent = eventList[i];
            if(slotEvent.eventId == event->eventId) {
                //eventList.erase(eventList.begin() + i);
                result = slotEvent.eventCompleted;
                break;
            }
        }
    }
	safeMutex.ReleaseLock();
	//if(result == false) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d, result = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex,result);
	return result;
}

void ConnectionSlotThread::execute() {
    RunningStatusSafeWrapper runningStatus(this);
	try {
		//setRunningStatus(true);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		unsigned int idx = 0;
		for(;this->slotInterface != NULL;) {
			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			semTaskSignalled.waitTillSignalled();
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
            MutexSafeWrapper safeMutex(&triggerIdMutex,mutexOwnerId);
            int eventCount = eventList.size();
            if(eventCount > 0) {
                ConnectionSlotEvent eventCopy;
                eventCopy.eventId = -1;

                for(int i = 0; i < eventList.size(); ++i) {
                    ConnectionSlotEvent &slotEvent = eventList[i];
                    if(slotEvent.eventCompleted == false) {
                        eventCopy = slotEvent;
                        break;
                    }
                }

                safeMutex.ReleaseLock();

                if(getQuitStatus() == true) {
                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                    break;
                }

                if(eventCopy.eventId > 0) {
                    ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);
                    this->slotInterface->slotUpdateTask(&eventCopy);
                    setTaskCompleted(eventCopy.eventId);
                }
            }
            else {
                safeMutex.ReleaseLock();
            }
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw runtime_error(ex.what());
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	//setRunningStatus(false);
}

// =====================================================
//	class ConnectionSlot
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex) {

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

    this->connectedRemoteIPAddress = 0;
	this->sessionKey 		= 0;
	this->serverInterface	= serverInterface;
	this->playerIndex		= playerIndex;
	this->playerStatus		= 0;
	this->currentFrameCount = 0;
	this->currentLagCount	= 0;
	this->gotLagCountWarning = false;
	this->lastReceiveCommandListTime	= 0;
	this->socket		   	= NULL;
	this->slotThreadWorker 	= NULL;
	this->slotThreadWorker 	= new ConnectionSlotThread(this->serverInterface,playerIndex);
	this->slotThreadWorker->setUniqueID(__FILE__);
	this->slotThreadWorker->start();

	this->ready		= false;
	this->gotIntro 	= false;
	this->connectedTime = 0;

	networkGameDataSynchCheckOkMap      = false;
	networkGameDataSynchCheckOkTile     = false;
	networkGameDataSynchCheckOkTech     = false;
	this->setNetworkGameDataSynchCheckTechMismatchReport("");
	this->setReceivedDataSynchCheck(false);

	this->clearChatInfo();
}

ConnectionSlot::~ConnectionSlot() {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] START\n",__FILE__,__FUNCTION__,__LINE__);

	close();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(BaseThread::shutdownAndWait(slotThreadWorker) == true) {
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        delete slotThreadWorker;
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	slotThreadWorker = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ConnectionSlot::update(bool checkForNewClients,int lockedSlotIndex) {
	//Chrono chrono;
	//chrono.start();
	try {
		clearThreadErrorList();

	    if(slotThreadWorker != NULL) {
	        slotThreadWorker->purgeCompletedEvents();
	    }

	    //if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

		if(socket == NULL) {
			if(networkGameDataSynchCheckOkMap) networkGameDataSynchCheckOkMap  = false;
			if(networkGameDataSynchCheckOkTile) networkGameDataSynchCheckOkTile = false;
			if(networkGameDataSynchCheckOkTech) networkGameDataSynchCheckOkTech = false;
			this->setReceivedDataSynchCheck(false);

			// Is the listener socket ready to be read?
			if(checkForNewClients == true) {

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] BEFORE accept new client connection, serverInterface->getOpenSlotCount() = %d\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getOpenSlotCount());
				bool hasOpenSlots = (serverInterface->getOpenSlotCount() > 0);

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				bool hasData = (serverInterface->getServerSocket() != NULL && serverInterface->getServerSocket()->hasDataToRead() == true);

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				if(hasData == true) {

					//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to accept new client connection playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
					Socket *newSocket = serverInterface->getServerSocket()->accept();
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] called accept new client connection playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
					if(newSocket != NULL) {
						// Set Socket as non-blocking
						newSocket->setBlock(false);
						socket = newSocket;

						//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

						this->connectedTime = time(NULL);
						this->clearChatInfo();
						this->name = "";
						this->playerStatus = npst_PickSettings;
						this->ready = false;
						this->vctFileList.clear();
						this->receivedNetworkGameStatus = false;
						this->gotIntro = false;

						//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

						static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
						MutexSafeWrapper safeMutexSlot(&mutexPendingNetworkCommandList,mutexOwnerId);
						this->vctPendingNetworkCommandList.clear();
						safeMutexSlot.ReleaseLock();

						//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

						this->currentFrameCount = 0;
						this->currentLagCount = 0;
						this->lastReceiveCommandListTime = 0;
						this->gotLagCountWarning = false;
						this->versionString = "";

                        //if(this->slotThreadWorker == NULL) {
                        //    this->slotThreadWorker 	= new ConnectionSlotThread(this->serverInterface,playerIndex);
                        //    this->slotThreadWorker->setUniqueID(__FILE__);
                        //    this->slotThreadWorker->start();
                        //}

						//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
						serverInterface->updateListen();
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

						//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
					}

					//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
				}

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				//send intro message when connected
				if(socket != NULL) {
					RandomGen random;
					sessionKey = random.randRange(-100000, 100000);
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] accepted new client connection, serverInterface->getOpenSlotCount() = %d, sessionKey = %d\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getOpenSlotCount(),sessionKey);

					if(hasOpenSlots == false) {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] !!!!!!!!WARNING - no open slots, disconnecting client\n",__FILE__,__FUNCTION__,__LINE__);

						if(socket != NULL) {
							NetworkMessageIntro networkMessageIntro(sessionKey,getNetworkVersionString(), getHostName(), playerIndex, nmgstNoSlots, 0, ServerSocket::getFTPServerPort());
							sendMessage(&networkMessageIntro);
						}

						//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

						close();
					}
					else {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] client will be assigned to the next open slot\n",__FILE__,__FUNCTION__,__LINE__);

						if(socket != NULL) {
							NetworkMessageIntro networkMessageIntro(sessionKey,getNetworkVersionString(), getHostName(), playerIndex, nmgstOk, 0, ServerSocket::getFTPServerPort());
							sendMessage(&networkMessageIntro);

							//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
						}
					}
				}
			}
			//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
		}
		else {
			//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(socket != NULL && socket->isConnected()) {

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				this->clearChatInfo();

				bool gotTextMsg = true;
				for(;socket != NULL && socket->hasDataToRead() == true && gotTextMsg == true;) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] polling for networkMessageType...\n",__FILE__,__FUNCTION__,__LINE__);

					NetworkMessageType networkMessageType= getNextMessageType(true);

					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkMessageType = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageType);

					gotTextMsg = false;
					//process incoming commands
					switch(networkMessageType) {

						case nmtInvalid:
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtInvalid\n",__FILE__,__FUNCTION__,__LINE__);
							break;

						case nmtPing:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtPing\n",__FILE__,__FUNCTION__);

							// client REQUIRES a ping before completing intro
							// authentication
							//if(gotIntro == true) {
								NetworkMessagePing networkMessagePing;
								if(receiveMessage(&networkMessagePing)) {
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
									lastPingInfo = networkMessagePing;
								}
							//}
						}
						break;

						case nmtText:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtText gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								NetworkMessageText networkMessageText;
								if(receiveMessage(&networkMessageText)) {
									ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getTeamIndex(),networkMessageText.getPlayerIndex());
									this->addChatInfo(msg);
									gotTextMsg = true;

									//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] chatText [%s] chatSender [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,chatText.c_str(),chatSender.c_str(),chatTeamIndex);
								}
							}
						}
						break;

						//command list
						case nmtCommandList: {

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtCommandList gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							//throw runtime_error("test");

							if(gotIntro == true) {
								NetworkMessageCommandList networkMessageCommandList;
								if(receiveMessage(&networkMessageCommandList)) {
									currentFrameCount = networkMessageCommandList.getFrameCount();
									lastReceiveCommandListTime = time(NULL);
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] currentFrameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,currentFrameCount);

									static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
									MutexSafeWrapper safeMutexSlot(&mutexPendingNetworkCommandList,mutexOwnerId);
									for(int i = 0; i < networkMessageCommandList.getCommandCount(); ++i) {
										vctPendingNetworkCommandList.push_back(*networkMessageCommandList.getCommand(i));
									}
									safeMutexSlot.ReleaseLock();
								}
							}
						}
						break;

						//process intro messages
						case nmtIntro:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtIntro\n",__FILE__,__FUNCTION__);

							NetworkMessageIntro networkMessageIntro;
							if(receiveMessage(&networkMessageIntro)) {
								int msgSessionId = networkMessageIntro.getSessionId();
								name= networkMessageIntro.getName();
								versionString = networkMessageIntro.getVersionString();
								this->connectedRemoteIPAddress = networkMessageIntro.getExternalIp();

								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got name [%s] versionString [%s], msgSessionId = %d\n",__FILE__,__FUNCTION__,name.c_str(),versionString.c_str(),msgSessionId);

								if(msgSessionId != sessionKey) {
									string playerNameStr = name;
									string sErr = "Client gave invalid sessionid for player [" + playerNameStr + "]";
									printf("%s\n",sErr.c_str());
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());

									close();
									return;
								}

								//check consistency
								bool compatible = checkVersionComptability(getNetworkVersionString(), networkMessageIntro.getVersionString());
								if(compatible == false) {
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

									bool versionMatched = false;
									string platformFreeVersion = getNetworkPlatformFreeVersionString();
									string sErr = "";

									if(strncmp(platformFreeVersion.c_str(),networkMessageIntro.getVersionString().c_str(),strlen(platformFreeVersion.c_str())) != 0) {
										string playerNameStr = name;
										sErr = "Server and client binary mismatch!\nYou have to use the exactly same binaries!\n\nServer: " +  getNetworkVersionString() +
												"\nClient: " + networkMessageIntro.getVersionString() + " player [" + playerNameStr + "]";
										printf("%s\n",sErr.c_str());

										serverInterface->sendTextMessage("Server and client binary mismatch!!",-1, true,lockedSlotIndex);
										serverInterface->sendTextMessage(" Server:" + getNetworkVersionString(),-1, true,lockedSlotIndex);
										serverInterface->sendTextMessage(" Client: "+ networkMessageIntro.getVersionString(),-1, true,lockedSlotIndex);
										serverInterface->sendTextMessage(" Client player [" + playerNameStr + "]",-1, true,lockedSlotIndex);
									}
									else {
										versionMatched = true;

										string playerNameStr = name;
										sErr = "Warning, Server and client are using the same version but different platforms.\n\nServer: " +  getNetworkVersionString() +
												"\nClient: " + networkMessageIntro.getVersionString() + " player [" + playerNameStr + "]";
										printf("%s\n",sErr.c_str());
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());
									}

									if(Config::getInstance().getBool("PlatformConsistencyChecks","true") &&
									   versionMatched == false) { // error message and disconnect only if checked
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());
										close();
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,sErr.c_str());
										return;
									}
								}

								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								gotIntro = true;

                                this->serverInterface->addClientToServerIPAddress(this->getSocket()->getConnectedIPAddress(this->getSocket()->getIpAddress()),this->connectedRemoteIPAddress);

								if(getAllowGameDataSynchCheck() == true && serverInterface->getGameSettings() != NULL) {
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending NetworkMessageSynchNetworkGameData\n",__FILE__,__FUNCTION__,__LINE__);

									NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(serverInterface->getGameSettings());
									sendMessage(&networkMessageSynchNetworkGameData);
								}
							}
						}
						break;

						//process datasynch messages
						case nmtSynchNetworkGameDataStatus:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtSynchNetworkGameDataStatus, gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

										SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getGameSettings()->getScenarioDir().c_str(),serverInterface->getGameSettings()->getScenario().c_str(),scenarioDir.c_str());
									}

									//tileset
									int32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + serverInterface->getGameSettings()->getTileset() + string("/*"), ".xml", NULL);
									int32 techCRC    = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", NULL);
									Checksum checksum;
									string file = Map::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir,false);
									checksum.addFile(file);
									int32 mapCRC = checksum.getSum();

									networkGameDataSynchCheckOkMap      = (networkMessageSynchNetworkGameDataStatus.getMapCRC() == mapCRC);
									networkGameDataSynchCheckOkTile     = (networkMessageSynchNetworkGameDataStatus.getTilesetCRC() == tilesetCRC);
									networkGameDataSynchCheckOkTech     = (networkMessageSynchNetworkGameDataStatus.getTechCRC()    == techCRC);

									// For testing
									//techCRC++;

									if( networkGameDataSynchCheckOkMap      == true &&
										networkGameDataSynchCheckOkTile     == true &&
										networkGameDataSynchCheckOkTech     == true) {
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] client data synch ok\n",__FILE__,__FUNCTION__);
									}
									else {
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] mapCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,mapCRC,networkMessageSynchNetworkGameDataStatus.getMapCRC());
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] tilesetCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,tilesetCRC,networkMessageSynchNetworkGameDataStatus.getTilesetCRC());
										SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,techCRC,networkMessageSynchNetworkGameDataStatus.getTechCRC());

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

												SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getGameSettings()->getScenarioDir().c_str(),serverInterface->getGameSettings()->getScenario().c_str(),scenarioDir.c_str());
											}

											if(networkGameDataSynchCheckOkTile == false) {
												if(tilesetCRC == 0) {
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_tilesets) + "/" + serverInterface->getGameSettings()->getTileset() + "/*", "", &vctFileList);
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + serverInterface->getGameSettings()->getTileset() + string("/*"), "", &vctFileList);
												}
												else {
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_tilesets) + "/" + serverInterface->getGameSettings()->getTileset() + "/*", ".xml", &vctFileList);
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTilesets,scenarioDir), "/" + serverInterface->getGameSettings()->getTileset() + "/*", ".xml", &vctFileList);
												}
											}
											if(networkGameDataSynchCheckOkTech == false) {
												if(techCRC == 0) {
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
												}
												else {
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", &vctFileList);
													vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", &vctFileList);
												}

												string report = networkMessageSynchNetworkGameDataStatus.getTechCRCFileMismatchReport(serverInterface->getGameSettings()->getTech(),vctFileList);
												this->setNetworkGameDataSynchCheckTechMismatchReport(report);
											}
											if(networkGameDataSynchCheckOkMap == false) {
												vctFileList.push_back(std::pair<string,int32>(Map::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir,false),mapCRC));
											}

											//for(int i = 0; i < vctFileList.size(); i++)
											//{
											NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck(vctFileList.size(), 1, vctFileList[0].second, vctFileList[0].first);
											sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
											//}
										}
										else {
											if(networkGameDataSynchCheckOkTech == false) {
												//if(techCRC == 0) {
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
												//}
												//else {
													//vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", &vctFileList);
												vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", NULL);
												//}

												string report = networkMessageSynchNetworkGameDataStatus.getTechCRCFileMismatchReport(serverInterface->getGameSettings()->getTech(),vctFileList);
												this->setNetworkGameDataSynchCheckTechMismatchReport(report);
											}
										}
									}

									this->setReceivedDataSynchCheck(true);
									receivedNetworkGameStatus = true;
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								}
							}
						}
						break;

						case nmtSynchNetworkGameDataFileCRCCheck:
						{

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck\n",__FILE__,__FUNCTION__);

							if(gotIntro == true) {
								NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck;
								if(receiveMessage(&networkMessageSynchNetworkGameDataFileCRCCheck))
								{
									int fileIndex = networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex();
									NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck(vctFileList.size(), fileIndex, vctFileList[fileIndex-1].second, vctFileList[fileIndex-1].first);
									sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
								}
							}
						}
						break;

						case nmtSynchNetworkGameDataFileGet:
						{

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileGet\n",__FILE__,__FUNCTION__);

							if(gotIntro == true) {
								NetworkMessageSynchNetworkGameDataFileGet networkMessageSynchNetworkGameDataFileGet;
								if(receiveMessage(&networkMessageSynchNetworkGameDataFileGet)) {
									FileTransferInfo fileInfo;
									fileInfo.hostType   = eServer;
									//fileInfo.serverIP   = this->ip.getString();
									fileInfo.serverPort = Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str());
									fileInfo.fileName   = networkMessageSynchNetworkGameDataFileGet.getFileName();

									FileTransferSocketThread *fileXferThread = new FileTransferSocketThread(fileInfo);
									fileXferThread->start();
								}
							}
						}
						break;

						case nmtSwitchSetupRequest:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got nmtSwitchSetupRequest gotIntro = %d\n",__FILE__,__FUNCTION__,__LINE__,gotIntro);

							if(gotIntro == true) {
								SwitchSetupRequest switchSetupRequest;
								if(receiveMessage(&switchSetupRequest)) {
									static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
									MutexSafeWrapper safeMutex(getServerSynchAccessor(),mutexOwnerId);

									int factionIdx = switchSetupRequest.getCurrentFactionIndex();
									if(serverInterface->getSwitchSetupRequests()[factionIdx] == NULL) {
										serverInterface->getSwitchSetupRequests()[factionIdx]= new SwitchSetupRequest();
									}
									*(serverInterface->getSwitchSetupRequests()[factionIdx]) = switchSetupRequest;

									this->playerStatus = switchSetupRequest.getNetworkPlayerStatus();

									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] factionIdx = %d, switchSetupRequest.getNetworkPlayerName() [%s] switchSetupRequest.getNetworkPlayerStatus() = %d, switchSetupRequest.getSwitchFlags() = %d\n",__FILE__,__FUNCTION__,__LINE__,factionIdx,switchSetupRequest.getNetworkPlayerName().c_str(),switchSetupRequest.getNetworkPlayerStatus(),switchSetupRequest.getSwitchFlags());
								}
							}
							break;
						}
						case nmtReady:
						{
							// its simply ignored here. Probably we are starting a game
							break;
						}
						default:
							{
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] networkMessageType = %d\n",__FILE__,__FUNCTION__,__LINE__,networkMessageType);

								if(gotIntro == true) {
									//throw runtime_error("Unexpected message in connection slot: " + intToStr(networkMessageType));
									string sErr = "Unexpected message in connection slot: " + intToStr(networkMessageType);
									//sendTextMessage(sErr,-1);
									//DisplayErrorMessage(sErr);
									threadErrorList.push_back(sErr);
									return;
								}
								else {
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got invalid message type before intro, disconnecting socket.\n",__FILE__,__FUNCTION__,__LINE__);
									close();
								}
							}
					}
				}

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

				validateConnection();

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] calling close...\n",__FILE__,__FUNCTION__,__LINE__);

				close();

				//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
			}

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());

		threadErrorList.push_back(ex.what());

		//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
	}

	//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
}

void ConnectionSlot::validateConnection() {
	if(gotIntro == false && connectedTime > 0 &&
		difftime(time(NULL),connectedTime) > GameConstants::maxClientConnectHandshakeSecs) {
		//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] difftime(time(NULL),connectedTime) = %f\n",__FILE__,__FUNCTION__,__LINE__,difftime(time(NULL),connectedTime));
		close();
	}
}

void ConnectionSlot::close() {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(this->slotThreadWorker != NULL) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        this->slotThreadWorker->setAllEventsCompleted();
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool updateServerListener = (socket != NULL);
	delete socket;
	socket= NULL;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(updateServerListener == true && ready == false) {
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    	serverInterface->updateListen();
    }

    ready = false;
    gotIntro = false;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

bool ConnectionSlot::hasValidSocketId() {
    bool result = (socket != NULL && socket->getSocketId() > 0);
    return result;
}

Mutex * ConnectionSlot::getServerSynchAccessor() {
	return (serverInterface != NULL ? serverInterface->getServerSynchAccessor() : NULL);
}

void ConnectionSlot::signalUpdate(ConnectionSlotEvent *event) {
	//assert(slotThreadWorker != NULL);
    if(slotThreadWorker != NULL) {
        slotThreadWorker->signalUpdate(event);
    }
}

bool ConnectionSlot::updateCompleted(ConnectionSlotEvent *event) {
	//assert(slotThreadWorker != NULL);

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

	bool waitingForThread = (slotThreadWorker != NULL &&
							 slotThreadWorker->isSignalCompleted(event) == false &&
							 slotThreadWorker->getQuitStatus() 		    == false &&
							 slotThreadWorker->getRunningStatus() 	    == true);

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, waitingForThread = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,waitingForThread);

	return (waitingForThread == false);
}

void ConnectionSlot::sendMessage(const NetworkMessage* networkMessage) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(&socketSynchAccessor,mutexOwnerId);
	NetworkInterface::sendMessage(networkMessage);
}

string ConnectionSlot::getHumanPlayerName(int index) {
	return serverInterface->getHumanPlayerName(index);
}

vector<NetworkCommand> ConnectionSlot::getPendingNetworkCommandList(bool clearList) {
	vector<NetworkCommand> ret;
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutexSlot(&mutexPendingNetworkCommandList,mutexOwnerId);
    if(vctPendingNetworkCommandList.size() > 0) {
    	ret = vctPendingNetworkCommandList;
		if(clearList == true) {
			vctPendingNetworkCommandList.clear();
		}
    }
    safeMutexSlot.ReleaseLock();

    return ret;
}

void ConnectionSlot::clearPendingNetworkCommandList() {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutexSlot(&mutexPendingNetworkCommandList,mutexOwnerId);
	if(vctPendingNetworkCommandList.size() > 0) {
		vctPendingNetworkCommandList.clear();
	}
    safeMutexSlot.ReleaseLock();
}

}}//end namespace
