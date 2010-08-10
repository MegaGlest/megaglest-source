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
//using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
//	class ConnectionSlotThread
// =====================================================

ConnectionSlotThread::ConnectionSlotThread(int slotIndex) : BaseThread() {
	this->slotIndex = slotIndex;
	this->slotInterface = NULL;
	this->event = NULL;
}

ConnectionSlotThread::ConnectionSlotThread(ConnectionSlotCallbackInterface *slotInterface,int slotIndex) : BaseThread() {
	this->slotIndex = slotIndex;
	this->slotInterface = slotInterface;
	this->event = NULL;
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
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] event = %p\n",__FILE__,__FUNCTION__,__LINE__,event);

	if(event != NULL) {
		MutexSafeWrapper safeMutex(&triggerIdMutex);
		this->event = event;
		safeMutex.ReleaseLock();
	}
	semTaskSignalled.signal();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void ConnectionSlotThread::setTaskCompleted(ConnectionSlotEvent *event) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	if(event != NULL) {
		MutexSafeWrapper safeMutex(&triggerIdMutex);
		event->eventCompleted = true;
		safeMutex.ReleaseLock();
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

bool ConnectionSlotThread::isSignalCompleted() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex);

	MutexSafeWrapper safeMutex(&triggerIdMutex);
	bool result = (this->event != NULL ? this->event->eventCompleted : true);
	safeMutex.ReleaseLock();

	if(result == false) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] slotIndex = %d, result = %d\n",__FILE__,__FUNCTION__,__LINE__,slotIndex,result);

	return result;
}

void ConnectionSlotThread::execute() {
	try {
		setRunningStatus(true);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		unsigned int idx = 0;
		for(;this->slotInterface != NULL;) {
			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			semTaskSignalled.waitTillSignalled();
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			this->slotInterface->slotUpdateTask(this->event);

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			setTaskCompleted(this->event);

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &ex) {
		setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw runtime_error(ex.what());
	}
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	setRunningStatus(false);
}

// =====================================================
//	class ConnectionSlot
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex) {

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);

	this->serverInterface	= serverInterface;
	this->playerIndex		= playerIndex;
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

	networkGameDataSynchCheckOkMap      = false;
	networkGameDataSynchCheckOkTile     = false;
	networkGameDataSynchCheckOkTech     = false;
	this->clearChatInfo();
}

ConnectionSlot::~ConnectionSlot()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	BaseThread::shutdownAndWait(slotThreadWorker);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete slotThreadWorker;
	slotThreadWorker = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	close();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ConnectionSlot::update() {
    update(true);
}

void ConnectionSlot::update(bool checkForNewClients) {
	clearThreadErrorList();

	try {
		if(socket == NULL) {
			if(networkGameDataSynchCheckOkMap) networkGameDataSynchCheckOkMap  = false;
			if(networkGameDataSynchCheckOkTile) networkGameDataSynchCheckOkTile = false;
			if(networkGameDataSynchCheckOkTech) networkGameDataSynchCheckOkTech = false;

			// Is the listener socket ready to be read?
			//if(serverInterface->getServerSocket()->isReadable() == true)
			if(checkForNewClients == true) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] BEFORE accept new client connection, serverInterface->getOpenSlotCount() = %d\n",__FILE__,__FUNCTION__,serverInterface->getOpenSlotCount());
				bool hasOpenSlots = (serverInterface->getOpenSlotCount() > 0);
				if(serverInterface->getServerSocket()->hasDataToRead() == true) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
					socket = serverInterface->getServerSocket()->accept();
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
					if(socket != NULL) {
						serverInterface->updateListen();
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);
					}
				}

				//send intro message when connected
				if(socket != NULL) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] accepted new client connection, serverInterface->getOpenSlotCount() = %d\n",__FILE__,__FUNCTION__,serverInterface->getOpenSlotCount());
					connectedTime = time(NULL);

					this->clearChatInfo();

					if(hasOpenSlots == false) {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] no open slots, disconnecting client\n",__FILE__,__FUNCTION__);

						NetworkMessageIntro networkMessageIntro(getNetworkVersionString(), socket->getHostName(), playerIndex, nmgstNoSlots);
						sendMessage(&networkMessageIntro);

						close();
					}
					else {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] client will be assigned to the next open slot\n",__FILE__,__FUNCTION__);

						NetworkMessageIntro networkMessageIntro(getNetworkVersionString(), socket->getHostName(), playerIndex, nmgstOk);
						sendMessage(&networkMessageIntro);
					}
				}
			}
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(socket->isConnected()) {
				this->clearChatInfo();

				bool gotTextMsg = true;
				for(;socket->hasDataToRead() == true && gotTextMsg == true;) {
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

							NetworkMessagePing networkMessagePing;
							if(receiveMessage(&networkMessagePing)) {
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
								lastPingInfo = networkMessagePing;
							}
						}
						break;

						case nmtText:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtText\n",__FILE__,__FUNCTION__);

							NetworkMessageText networkMessageText;
							if(receiveMessage(&networkMessageText)) {
								ChatMsgInfo msg(networkMessageText.getText().c_str(),networkMessageText.getSender().c_str(),networkMessageText.getTeamIndex());
								this->addChatInfo(msg);
								gotTextMsg = true;

								//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] chatText [%s] chatSender [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,chatText.c_str(),chatSender.c_str(),chatTeamIndex);
							}
						}
						break;

						//command list
						case nmtCommandList: {

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtCommandList\n",__FILE__,__FUNCTION__);

							//throw runtime_error("test");

							NetworkMessageCommandList networkMessageCommandList;
							if(receiveMessage(&networkMessageCommandList)) {
								currentFrameCount = networkMessageCommandList.getFrameCount();
								lastReceiveCommandListTime = time(NULL);
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] currentFrameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,currentFrameCount);

								for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i) {
									//serverInterface->requestCommand(networkMessageCommandList.getCommand(i));
									vctPendingNetworkCommandList.push_back(*networkMessageCommandList.getCommand(i));
								}
							}
						}
						break;

						//process intro messages
						case nmtIntro:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtIntro\n",__FILE__,__FUNCTION__);

							NetworkMessageIntro networkMessageIntro;
							if(receiveMessage(&networkMessageIntro))
							{
								gotIntro = true;
								name= networkMessageIntro.getName();
								versionString = networkMessageIntro.getVersionString();

								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got name [%s] versionString [%s]\n",__FILE__,__FUNCTION__,name.c_str(),versionString.c_str());

								if(getAllowGameDataSynchCheck() == true && serverInterface->getGameSettings() != NULL)
								{
									SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] sending NetworkMessageSynchNetworkGameData\n",__FILE__,__FUNCTION__);

									NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(serverInterface->getGameSettings());
									sendMessage(&networkMessageSynchNetworkGameData);
								}
							}
						}
						break;

						//process datasynch messages
						case nmtSynchNetworkGameDataStatus:
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataStatus\n",__FILE__,__FUNCTION__);

							NetworkMessageSynchNetworkGameDataStatus networkMessageSynchNetworkGameDataStatus;
							if(receiveMessage(&networkMessageSynchNetworkGameDataStatus))
							{
								receivedNetworkGameStatus = true;

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
								string file = Map::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir);
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
										}
										if(networkGameDataSynchCheckOkMap == false) {
											vctFileList.push_back(std::pair<string,int32>(Map::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir),mapCRC));
										}

										//for(int i = 0; i < vctFileList.size(); i++)
										//{
										NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck(vctFileList.size(), 1, vctFileList[0].second, vctFileList[0].first);
										sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
										//}
									}
								}
							}
						}
						break;

						case nmtSynchNetworkGameDataFileCRCCheck:
						{

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck\n",__FILE__,__FUNCTION__);

							NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck;
							if(receiveMessage(&networkMessageSynchNetworkGameDataFileCRCCheck))
							{
								int fileIndex = networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex();
								NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck(vctFileList.size(), fileIndex, vctFileList[fileIndex-1].second, vctFileList[fileIndex-1].first);
								sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
							}
						}
						break;

						case nmtSynchNetworkGameDataFileGet:
						{

							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileGet\n",__FILE__,__FUNCTION__);

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
						break;

						case nmtSwitchSetupRequest:
						{
							SwitchSetupRequest switchSetupRequest;
							if(receiveMessage(&switchSetupRequest)) {
								Mutex *mutex = getServerSynchAccessor();
								if(mutex != NULL) mutex->p();

								if(serverInterface->getSwitchSetupRequests()[switchSetupRequest.getCurrentFactionIndex()]==NULL) {
									serverInterface->getSwitchSetupRequests()[switchSetupRequest.getCurrentFactionIndex()]= new SwitchSetupRequest();
								}
								*(serverInterface->getSwitchSetupRequests()[switchSetupRequest.getCurrentFactionIndex()])=switchSetupRequest;

								if(mutex != NULL) mutex->v();
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
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

				if(gotIntro == false && difftime(time(NULL),connectedTime) > GameConstants::maxClientConnectHandshakeSecs) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] difftime(time(NULL),connectedTime) = %f\n",__FILE__,__FUNCTION__,__LINE__,difftime(time(NULL),connectedTime));
					close();
				}
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] calling close...\n",__FILE__,__FUNCTION__,__LINE__);

				close();
			}

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error detected [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());

		threadErrorList.push_back(ex.what());
	}
}

void ConnectionSlot::close() {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s LINE: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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
	assert(slotThreadWorker != NULL);

	slotThreadWorker->signalUpdate(event);
}

bool ConnectionSlot::updateCompleted() {
	assert(slotThreadWorker != NULL);

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex);

	bool waitingForThread = (slotThreadWorker->isSignalCompleted() 	== false &&
							 slotThreadWorker->getQuitStatus() 		== false &&
							 slotThreadWorker->getRunningStatus() 	== true);

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] playerIndex = %d, waitingForThread = %d\n",__FILE__,__FUNCTION__,__LINE__,playerIndex,waitingForThread);

	return (waitingForThread == false);
}

void ConnectionSlot::sendMessage(const NetworkMessage* networkMessage) {
	MutexSafeWrapper safeMutex(&socketSynchAccessor);
	NetworkInterface::sendMessage(networkMessage);
	safeMutex.ReleaseLock();
}

}}//end namespace
