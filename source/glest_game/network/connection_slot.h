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

#ifndef _GLEST_GAME_CONNECTIONSLOT_H_
#define _GLEST_GAME_CONNECTIONSLOT_H_

#include <vector>
#include "socket.h"
#include "network_interface.h"
#include <time.h>
#include "base_thread.h"
#include "leak_dumper.h"

using Shared::Platform::ServerSocket;
using Shared::Platform::Socket;
using std::vector;

namespace Glest{ namespace Game{

class ServerInterface;
class ConnectionSlot;

// =====================================================
//	class ConnectionSlotThread
// =====================================================

enum ConnectionSlotEventType
{
	eNone,
    eReceiveSocketData,
    eSendSocketData
};

class ConnectionSlotEvent {
public:

	ConnectionSlotEvent() {
		eventType = eNone;
		triggerId = -1;
		connectionSlot = NULL;
		networkMessage = NULL;
		socketTriggered = false;
		eventCompleted = false;
		eventId = -1;
	}

	int64 triggerId;
	ConnectionSlot* connectionSlot;
	ConnectionSlotEventType eventType;
	const NetworkMessage *networkMessage;
	bool socketTriggered;
	bool eventCompleted;
	int64 eventId;
};

//
// This interface describes the methods a callback object must implement
//
class ConnectionSlotCallbackInterface {
public:
	virtual void slotUpdateTask(ConnectionSlotEvent *event) = 0;
};

class ConnectionSlotThread : public BaseThread
{
protected:

	ConnectionSlotCallbackInterface *slotInterface;
	Semaphore semTaskSignalled;
	Mutex triggerIdMutex;
	vector<ConnectionSlotEvent> eventList;
	int slotIndex;

	virtual void setQuitStatus(bool value);
	virtual void setTaskCompleted(int eventId);
	virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);

public:
	ConnectionSlotThread(int slotIndex);
	ConnectionSlotThread(ConnectionSlotCallbackInterface *slotInterface,int slotIndex);
    virtual void execute();
    void signalUpdate(ConnectionSlotEvent *event);
    bool isSignalCompleted(ConnectionSlotEvent *event);
    int getSlotIndex() const {return slotIndex; }

    void purgeCompletedEvents();
    void purgeAllEvents();
    void setAllEventsCompleted();
};

// =====================================================
//	class ConnectionSlot
// =====================================================

class ConnectionSlot: public NetworkInterface {
private:
	ServerInterface* serverInterface;
	Socket* socket;
	int playerIndex;
	string name;
	bool ready;
	vector<std::pair<string,int32> > vctFileList;
	bool receivedNetworkGameStatus;
	time_t connectedTime;
	bool gotIntro;

	Mutex mutexPendingNetworkCommandList;
	vector<NetworkCommand> vctPendingNetworkCommandList;
	ConnectionSlotThread* slotThreadWorker;
	int currentFrameCount;
	int currentLagCount;
	time_t lastReceiveCommandListTime;
	bool gotLagCountWarning;
	string versionString;
	int sessionKey;
	uint32 connectedRemoteIPAddress;

public:
	ConnectionSlot(ServerInterface* serverInterface, int playerIndex);
	~ConnectionSlot();

    void update(bool checkForNewClients,int lockedSlotIndex);
	void setPlayerIndex(int value) { playerIndex = value; }
	int getPlayerIndex() {return playerIndex;}

	uint32 getConnectedRemoteIPAddress() const { return connectedRemoteIPAddress; }

	void setReady()					{ready= true;}
	const string &getName() const	{return name;}
	void setName(string value)      {name = value;}
	bool isReady() const			{return ready;}

	virtual Socket* getSocket()				{return socket;}
	virtual Socket* getSocket() const		{return socket;}

	virtual void close();
	//virtual bool getFogOfWar();

	bool getReceivedNetworkGameStatus() { return receivedNetworkGameStatus; }
	void setReceivedNetworkGameStatus(bool value) { receivedNetworkGameStatus = value; }

	bool hasValidSocketId();
	virtual bool getConnectHasHandshaked() const { return gotIntro; }
	std::vector<std::string> getThreadErrorList() const { return threadErrorList; }
	void clearThreadErrorList() { threadErrorList.clear(); }

	vector<NetworkCommand> getPendingNetworkCommandList(bool clearList=false);
	void clearPendingNetworkCommandList();

	void signalUpdate(ConnectionSlotEvent *event);
	bool updateCompleted(ConnectionSlotEvent *event);

	virtual void sendMessage(const NetworkMessage* networkMessage);
	int getCurrentFrameCount() const { return currentFrameCount; }

	int getCurrentLagCount() const { return currentLagCount; }
	void setCurrentLagCount(int value) { currentLagCount = value; }

	time_t getLastReceiveCommandListTime() const { return lastReceiveCommandListTime; }

	bool getLagCountWarning() const { return gotLagCountWarning; }
	void setLagCountWarning(bool value) { gotLagCountWarning = value; }

	const string &getVersionString() const	{return versionString;}

	void validateConnection();
	virtual string getHumanPlayerName(int index=-1);
	virtual int getHumanPlayerIndex() const {return playerIndex;}

protected:

	Mutex * getServerSynchAccessor();
	std::vector<std::string> threadErrorList;
	Mutex socketSynchAccessor;

	virtual void update() {}
};

}}//end namespace

#endif
