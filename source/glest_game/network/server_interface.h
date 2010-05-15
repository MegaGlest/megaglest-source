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
#include "base_thread.h"

using std::vector;
using Shared::Platform::ServerSocket;

namespace Glest{ namespace Game{


// =====================================================
//	class ConnectionSlotThread
// =====================================================

class ConnectionSlotEvent {
public:

	int64 triggerId;
	ConnectionSlot* connectionSlot;
	bool socketTriggered;
	bool eventCompleted;
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
	ConnectionSlotEvent *event;

	virtual void setQuitStatus(bool value);
	virtual void setTaskCompleted(ConnectionSlotEvent *event);

public:
	ConnectionSlotThread();
	ConnectionSlotThread(ConnectionSlotCallbackInterface *slotInterface);
    virtual void execute();
    void signalUpdate(ConnectionSlotEvent *event);
    bool isSignalCompleted();

};

// =====================================================
//	class ServerInterface
// =====================================================

class ServerInterface: public GameNetworkInterface, public ConnectionSlotCallbackInterface {

private:
	ConnectionSlot* slots[GameConstants::maxPlayers];
	ServerSocket serverSocket;
	bool gameHasBeenInitiated;
	int gameSettingsUpdateCount;
	SwitchSetupRequest* switchSetupRequests[GameConstants::maxPlayers];
	Mutex serverSynchAccessor;

	ConnectionSlotThread* slotThreads[GameConstants::maxPlayers];

public:
	ServerInterface();
	virtual ~ServerInterface();

	virtual Socket* getSocket()				{return &serverSocket;}
	virtual const Socket* getSocket() const	{return &serverSocket;}
	virtual void close();

	//message processing
	virtual void update();
	virtual void updateLobby(){};
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum* checksum);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame(bool userManuallyQuit);

	//misc
	virtual string getNetworkStatus() const;

	ServerSocket* getServerSocket()		{return &serverSocket;}
	SwitchSetupRequest** getSwitchSetupRequests() {return &switchSetupRequests[0];}
	void addSlot(int playerIndex);
	bool switchSlot(int fromPlayerIndex,int toPlayerIndex);
	void removeSlot(int playerIndex);
	ConnectionSlot* getSlot(int playerIndex);
	int getConnectedSlotCount();
	int getOpenSlotCount();

	bool launchGame(const GameSettings* gameSettings);
	virtual void setGameSettings(GameSettings *serverGameSettings, bool waitForClientAck = false);
	void broadcastGameSetup(const GameSettings* gameSettings);
	void updateListen();
	virtual bool getConnectHasHandshaked() const { return false; }

	virtual void slotUpdateTask(ConnectionSlotEvent *event);

private:
	void broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot= -1);
	void broadcastMessageToConnectedClients(const NetworkMessage* networkMessage, int excludeSlot = -1);
	bool shouldDiscardNetworkMessage(NetworkMessageType networkMessageType,ConnectionSlot* connectionSlot);
	void updateSlot(ConnectionSlotEvent *event);
};

}}//end namespace

#endif
