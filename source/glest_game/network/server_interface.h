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

#ifndef _GLEST_GAME_SERVERINTERFACE_H_
#define _GLEST_GAME_SERVERINTERFACE_H_

#include <vector>

#include "game_constants.h"
#include "network_interface.h"
#include "connection_slot.h"
#include "socket.h"
#include "leak_dumper.h"

using std::vector;
using Shared::Platform::ServerSocket;

namespace Shared {  namespace PlatformCommon {  class FTPServerThread;  }}

//using Shared::PlatformCommon::FTPServerThread;

namespace Glest{ namespace Game{

// =====================================================
//	class ServerInterface
// =====================================================

class ServerInterface: public GameNetworkInterface, public ConnectionSlotCallbackInterface, public SimpleTaskCallbackInterface, public FTPClientValidationInterface {

class TextMessageQueue {
public:
    string text;
    int teamIndex;
    bool echoLocal;
};

private:
	ConnectionSlot* slots[GameConstants::maxPlayers];
	Mutex slotAccessorMutexes[GameConstants::maxPlayers];

	ServerSocket serverSocket;
	bool gameHasBeenInitiated;
	int gameSettingsUpdateCount;
	SwitchSetupRequest* switchSetupRequests[GameConstants::maxPlayers];
	Mutex serverSynchAccessor;
	int currentFrameCount;

	time_t gameStartTime;

	SimpleTaskThread *publishToMasterserverThread;
	Mutex masterServerThreadAccessor;
	time_t lastMasterserverHeartbeatTime;
	bool needToRepublishToMasterserver;

    Shared::PlatformCommon::FTPServerThread *ftpServer;
    bool exitServer;
    int64 nextEventId;

    Mutex textMessageQueueThreadAccessor;
    vector<TextMessageQueue> textMessageQueue;

    Mutex broadcastMessageQueueThreadAccessor;
    vector<pair<const NetworkMessage *,int> > broadcastMessageQueue;

    Mutex inBroadcastMessageThreadAccessor;
    bool inBroadcastMessage;

public:
	ServerInterface();
	virtual ~ServerInterface();

	virtual Socket* getSocket()				{return &serverSocket;}

    const virtual Socket *getSocket() const
    {
        return &serverSocket;
    }

    virtual void close();
    virtual void update();
    virtual void updateLobby()  { };
    virtual void updateKeyframe(int frameCount);
    virtual void waitUntilReady(Checksum *checksum);
    virtual void sendTextMessage(const string & text, int teamIndex, bool echoLocal = false);
    void sendTextMessage(const string & text, int teamIndex, bool echoLocal, int lockedSlotIndex);
    void queueTextMessage(const string & text, int teamIndex, bool echoLocal = false);
    virtual void quitGame(bool userManuallyQuit);
    virtual string getNetworkStatus();
    ServerSocket *getServerSocket()
    {
        return &serverSocket;
    }

    SwitchSetupRequest **getSwitchSetupRequests()
    {
        return &switchSetupRequests[0];
    }

    void addSlot(int playerIndex);
    bool switchSlot(int fromPlayerIndex, int toPlayerIndex);
    void removeSlot(int playerIndex, int lockedSlotIndex = -1);
    ConnectionSlot *getSlot(int playerIndex);
    int getConnectedSlotCount();
    int getOpenSlotCount();
    bool launchGame(const GameSettings *gameSettings);
    void setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck);
    void broadcastGameSetup(const GameSettings *gameSettings);
    void updateListen();
    virtual bool getConnectHasHandshaked() const
    {
        return false;
    }

    virtual void slotUpdateTask(ConnectionSlotEvent *event);
    bool hasClientConnection();
    int getCurrentFrameCount() const
    {
        return currentFrameCount;
    }

    std::pair<bool,bool> clientLagCheck(ConnectionSlot *connectionSlot, bool skipNetworkBroadCast = false);
    bool signalClientReceiveCommands(ConnectionSlot *connectionSlot, int slotIndex, bool socketTriggered, ConnectionSlotEvent & event);
    void updateSocketTriggeredList(std::map<PLATFORM_SOCKET,bool> & socketTriggeredList);
    bool isPortBound() const
    {
        return serverSocket.isPortBound();
    }

    int getBindPort() const
    {
        return serverSocket.getBindPort();
    }

    void broadcastPing(const NetworkMessagePing *networkMessage, int excludeSlot = -1)
    {
        this->broadcastMessage(networkMessage, excludeSlot);
    }

    void queueBroadcastMessage(const NetworkMessage *networkMessage, int excludeSlot = -1);
    virtual string getHumanPlayerName(int index = -1);
    virtual int getHumanPlayerIndex() const;
    bool getNeedToRepublishToMasterserver() const
    {
        return needToRepublishToMasterserver;
    }

    void setNeedToRepublishToMasterserver(bool value)
    {
        needToRepublishToMasterserver = value;
    }

public:
    Mutex *getServerSynchAccessor() {
        return &serverSynchAccessor;
    }

    virtual void simpleTask(BaseThread *callingThread);
    void addClientToServerIPAddress(uint32 clientIp, uint32 ServerIp);
    virtual int isValidClientType(uint32 clientIp);
private:
    void broadcastMessage(const NetworkMessage *networkMessage, int excludeSlot = -1, int lockedSlotIndex = -1);
    void broadcastMessageToConnectedClients(const NetworkMessage *networkMessage, int excludeSlot = -1);
    bool shouldDiscardNetworkMessage(NetworkMessageType networkMessageType, ConnectionSlot *connectionSlot);
    void updateSlot(ConnectionSlotEvent *event);
    void validateConnectedClients();
    std::map<string,string> publishToMasterserver();
    int64 getNextEventId();
    void processTextMessageQueue();
    void processBroadCastMessageQueue();
protected:
    void signalClientsToRecieveData(std::map<PLATFORM_SOCKET,bool> & socketTriggeredList, std::map<int,ConnectionSlotEvent> & eventList, std::map<int,bool> & mapSlotSignalledList);
    void checkForCompletedClients(std::map<int,bool> & mapSlotSignalledList,std::vector <string> &errorMsgList,std::map<int,ConnectionSlotEvent> &eventList);
    void checForLaggingClients(std::map<int,bool> &mapSlotSignalledList, std::map<int,ConnectionSlotEvent> &eventList, std::map<PLATFORM_SOCKET,bool> &socketTriggeredList,std::vector <string> &errorMsgList);
    void executeNetworkCommandsFromClients();
    void dispatchPendingChatMessages(std::vector <string> &errorMsgList);
};

}}//end namespace

#endif
