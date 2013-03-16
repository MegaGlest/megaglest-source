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

#ifndef _GLEST_GAME_CLIENTINTERFACE_H_
#define _GLEST_GAME_CLIENTINTERFACE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <vector>
#include "network_interface.h"
#include "socket.h"
#include "leak_dumper.h"

using Shared::Platform::Ip;
using Shared::Platform::ClientSocket;
using std::vector;

namespace Glest{ namespace Game{

class ClientInterface;

class ClientInterfaceThread : public BaseThread, public SlaveThreadControllerInterface {
protected:

	ClientInterface *clientInterface;

	virtual void setQuitStatus(bool value);

public:
	ClientInterfaceThread(ClientInterface *client);
	virtual ~ClientInterfaceThread();
    virtual void execute();

    virtual void setMasterController(MasterSlaveThreadController *master) { }
	virtual void signalSlave(void *userdata) { }

	virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);
};

// =====================================================
//	class ClientInterface
// =====================================================

class ClientInterface: public GameNetworkInterface {
private:
	static const int messageWaitTimeout;
	static const int waitSleepTime;
	static const int maxNetworkCommandListSendTimeWait;

private:
	ClientSocket *clientSocket;
	string serverName;
	bool introDone;
	bool launchGame;
	int playerIndex;
	bool gameSettingsReceived;
	int gameSettingsReceivedCount;
	time_t connectedTime;
	bool gotIntro;

	Ip ip;
	int port;

	int currentFrameCount;
	int lastSentFrameCount;
	time_t lastNetworkCommandListSendTime;

	time_t clientSimulationLagStartTime;
	string versionString;
	int sessionKey;
	int serverFTPPort;

	ClientInterfaceThread *networkCommandListThread;

	Mutex *networkCommandListThreadAccessor;
	std::map<int,Commands> cachedPendingCommands;	//commands ready to be given
	uint64 cachedPendingCommandsIndex;
	uint64 cachedLastPendingFrameCount;

	Mutex *flagAccessor;
	bool joinGameInProgress;
	bool joinGameInProgressLaunch;
	bool readyForInGameJoin;
	bool resumeInGameJoin;

	bool quitThread;

public:
	ClientInterface();
	virtual ~ClientInterface();

	virtual Socket* getSocket(bool mutexLock=true)					{return clientSocket;}
	virtual void close();

	bool getJoinGameInProgress();
	bool getJoinGameInProgressLaunch();

	bool getReadyForInGameJoin();

	bool getResumeInGameJoin();
	void sendResumeGameMessage();

	uint64 getCachedLastPendingFrameCount();

	//message processing
	virtual void update();
	virtual void updateLobby();
	virtual void updateKeyframe(int frameCount);
	virtual void setKeyframe(int frameCount) { currentFrameCount = frameCount; }
	virtual void waitUntilReady(Checksum* checksum);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex, bool echoLocal,
			string targetLanguage);
	virtual void quitGame(bool userManuallyQuit);

	virtual void sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note,int playerIndex);
	virtual void sendUnMarkCellMessage(Vec2i targetPos, int factionIndex);
	virtual void sendHighlightCellMessage(Vec2i targetPos, int factionIndex);
	//misc
	virtual string getNetworkStatus() ;

	//accessors
	string getServerName() const			{return serverName;}
	bool getLaunchGame() const				{return launchGame;}
	bool getIntroDone() const				{return introDone;}
	bool getGameSettingsReceived() const	{return gameSettingsReceived;}
	void setGameSettingsReceived(bool value);

	int getGameSettingsReceivedCount() const { return gameSettingsReceivedCount; }

	int getPlayerIndex() const				{return playerIndex;}

	void connect(const Ip &ip, int port);
	void reset();

	void discoverServers(DiscoveredServersInterface *cb);
	void stopServerDiscovery();
	
	void sendSwitchSetupRequest(string selectedFactionName, int8 currentSlotIndex,
								int8 toSlotIndex, int8 toTeam,string networkPlayerName,
								int8 networkPlayerStatus, int8 flags,
								string language);
	virtual bool getConnectHasHandshaked() const { return gotIntro; }
	std::string getServerIpAddress();

	int getCurrentFrameCount() const { return currentFrameCount; }

	virtual void sendPingMessage(int32 pingFrequency, int64 pingTime);

	const string &getVersionString() const	{return versionString;}
	virtual string getHumanPlayerName(int index=-1);
	virtual int getHumanPlayerIndex() const {return playerIndex;}
	int getServerFTPPort() const { return serverFTPPort; }

	int getSessionKey() const { return sessionKey; }
	bool isMasterServerAdminOverride();

    void setGameSettings(GameSettings *serverGameSettings);
    void broadcastGameSetup(const GameSettings *gameSettings);
    void broadcastGameStart(const GameSettings *gameSettings);

    void updateNetworkFrame();

    virtual void saveGame(XmlNode *rootNode) {};

protected:

	Mutex * getServerSynchAccessor() { return NULL; }
	NetworkMessageType waitForMessage(int waitMilliseconds=0);
	bool shouldDiscardNetworkMessage(NetworkMessageType networkMessageType);

	void updateFrame(int *checkFrame);
	void shutdownNetworkCommandListThread();
	bool getNetworkCommand(int frameCount, int currentCachedPendingCommandsIndex);

	void close(bool lockMutex);
};

}}//end namespace

#endif
