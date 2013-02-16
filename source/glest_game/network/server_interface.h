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

#ifndef _GLEST_GAME_SERVERINTERFACE_H_
#define _GLEST_GAME_SERVERINTERFACE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

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

class ServerInterface: public GameNetworkInterface,
                       public ConnectionSlotCallbackInterface,
                       // This is for publishing game status to the masterserver
                       public SimpleTaskCallbackInterface,
                       public FTPClientValidationInterface {

class TextMessageQueue {
public:
    string text;
    int teamIndex;
    bool echoLocal;
    string targetLanguage;
};

private:
	ConnectionSlot* slots[GameConstants::maxPlayers];
	Mutex *slotAccessorMutexes[GameConstants::maxPlayers];

	ServerSocket serverSocket;
	bool gameHasBeenInitiated;
	int gameSettingsUpdateCount;

	Mutex *switchSetupRequestsSynchAccessor;
	SwitchSetupRequest* switchSetupRequests[GameConstants::maxPlayers];

	Mutex *serverSynchAccessor;
	int currentFrameCount;

	time_t gameStartTime;

	time_t lastGlobalLagCheckTime;

	SimpleTaskThread *publishToMasterserverThread;
	Mutex *masterServerThreadAccessor;
	time_t lastMasterserverHeartbeatTime;
	bool needToRepublishToMasterserver;

    Shared::PlatformCommon::FTPServerThread *ftpServer;
    bool exitServer;
    int64 nextEventId;

    Mutex *textMessageQueueThreadAccessor;
    vector<TextMessageQueue> textMessageQueue;

    Mutex *broadcastMessageQueueThreadAccessor;
    vector<pair<NetworkMessage *,int> > broadcastMessageQueue;

    Mutex *inBroadcastMessageThreadAccessor;
    bool inBroadcastMessage;

    bool masterserverAdminRequestLaunch;

	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;

	map<string,pair<uint64,time_t> > badClientConnectIPList;

	ServerSocket *serverSocketAdmin;
	MasterSlaveThreadController masterController;

	bool allowInGameConnections;
	bool pauseForInGameConnection;
	bool startInGameConnectionLaunch;
	bool unPauseForInGameConnection;

public:
	ServerInterface(bool publishEnabled);
	virtual ~ServerInterface();

	virtual Socket* getSocket(bool mutexLock=true)				{return &serverSocket;}

    //const virtual Socket *getSocket() const {
    //    return &serverSocket;
    //}

	time_t getGameStartTime() const { return gameStartTime; }

	bool getAllowInGameConnections() const { return allowInGameConnections; }
	void setAllowInGameConnections(bool value) { allowInGameConnections = value; }

	bool getStartInGameConnectionLaunch() const { return startInGameConnectionLaunch; }
	void setStartInGameConnectionLaunch(bool value) { startInGameConnectionLaunch = value; }

	bool getPauseForInGameConnection() const { return pauseForInGameConnection; }
	void setPauseForInGameConnection(bool value) { pauseForInGameConnection = value; }

	bool getUnPauseForInGameConnection() const { return unPauseForInGameConnection; }
	void setUnPauseForInGameConnection(bool value) { unPauseForInGameConnection = value; }

    virtual void close();
    virtual void update();
    virtual void updateLobby()  { };
    virtual void updateKeyframe(int frameCount);
    virtual void setKeyframe(int frameCount) { currentFrameCount = frameCount; }

    virtual void waitUntilReady(Checksum *checksum);
    virtual void sendTextMessage(const string & text, int teamIndex, bool echoLocal, string targetLanguage);
    void sendTextMessage(const string & text, int teamIndex, bool echoLocal, string targetLanguage, int lockedSlotIndex);

    void queueTextMessage(const string & text, int teamIndex, bool echoLocal, string targetLanguage);

    virtual void sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note,int playerIndex);
    void sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note, int playerIndex, int lockedSlotIndex);

	virtual void sendHighlightCellMessage(Vec2i targetPos, int factionIndex);
    void sendHighlightCellMessage(Vec2i targetPos, int factionIndex, int lockedSlotIndex);

    virtual void sendUnMarkCellMessage(Vec2i targetPos, int factionIndex);
    void sendUnMarkCellMessage(Vec2i targetPos, int factionIndex, int lockedSlotIndex);

    virtual void quitGame(bool userManuallyQuit);
    virtual string getNetworkStatus();
    ServerSocket *getServerSocket() {
        return &serverSocket;
    }

    SwitchSetupRequest **getSwitchSetupRequests();
    SwitchSetupRequest *getSwitchSetupRequests(int index);
    void setSwitchSetupRequests(int index,SwitchSetupRequest *ptr);
    Mutex * getSwitchSetupRequestsMutex() { return switchSetupRequestsSynchAccessor; }

    void addSlot(int playerIndex);
    bool switchSlot(int fromPlayerIndex, int toPlayerIndex);
    void removeSlot(int playerIndex, int lockedSlotIndex = -1);
    ConnectionSlot *getSlot(int playerIndex);
    int getSlotCount();
    int getConnectedSlotCount(bool authenticated);

    int getOpenSlotCount();
    bool launchGame(const GameSettings *gameSettings);
	void validateGameSettings(GameSettings *serverGameSettings);
    void setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck);
    void broadcastGameSetup(GameSettings *gameSettingsBuffer, bool setGameSettingsBuffer=false);

    int getGameSettingsUpdateCount();

    bool getMasterserverAdminRequestLaunch() const { return masterserverAdminRequestLaunch; }
    void setMasterserverAdminRequestLaunch(bool value) { masterserverAdminRequestLaunch = value; }

    void updateListen();
    virtual bool getConnectHasHandshaked() const
    {
        return false;
    }

    virtual void slotUpdateTask(ConnectionSlotEvent *event);
    bool hasClientConnection();
    bool isClientConnected(int index);

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

    void broadcastPing(NetworkMessagePing *networkMessage, int excludeSlot = -1)
    {
        this->broadcastMessage(networkMessage, excludeSlot);
    }

    void queueBroadcastMessage(NetworkMessage *networkMessage, int excludeSlot = -1);
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

    void setPublishEnabled(bool value);

    bool getGameHasBeenInitiated() const { return gameHasBeenInitiated; }

public:
    Mutex *getServerSynchAccessor() {
        return serverSynchAccessor;
    }

    virtual void simpleTask(BaseThread *callingThread);
    void addClientToServerIPAddress(uint32 clientIp, uint32 ServerIp);
    virtual int isValidClientType(uint32 clientIp);
    virtual int isClientAllowedToGetFile(uint32 clientIp, const char *username, const char *filename);

    void notifyBadClientConnectAttempt(string ipAddress);
    std::string DumpStatsToLog(bool dumpToStringOnly) const;

    virtual void saveGame(XmlNode *rootNode);

    void broadcastMessage(NetworkMessage *networkMessage, int excludeSlot = -1, int lockedSlotIndex = -1);

private:

    void broadcastMessageToConnectedClients(NetworkMessage *networkMessage, int excludeSlot = -1);
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
    void checkForLaggingClients(std::map<int,bool> &mapSlotSignalledList, std::map<int,ConnectionSlotEvent> &eventList, std::map<PLATFORM_SOCKET,bool> &socketTriggeredList,std::vector <string> &errorMsgList);
    void executeNetworkCommandsFromClients();
    void dispatchPendingChatMessages(std::vector <string> &errorMsgList);
    void dispatchPendingMarkCellMessages(std::vector <string> &errorMsgList);
    void dispatchPendingUnMarkCellMessages(std::vector <string> &errorMsgList);
    void dispatchPendingHighlightCellMessages(std::vector <string> &errorMsgList);

    void shutdownMasterserverPublishThread();
};

}}//end namespace

#endif
