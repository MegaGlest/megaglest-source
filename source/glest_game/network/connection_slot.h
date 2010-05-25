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

using Shared::Platform::ServerSocket;
using Shared::Platform::Socket;
using std::vector;

namespace Glest{ namespace Game{

class ServerInterface;

// =====================================================
//	class ConnectionSlot
// =====================================================

class ConnectionSlot: public NetworkInterface{
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

public:
	ConnectionSlot(ServerInterface* serverInterface, int playerIndex);
	~ConnectionSlot();

    void update(bool checkForNewClients);
	virtual void update();
	void setPlayerIndex(int value) { playerIndex = value; }
	int getPlayerIndex() {return playerIndex;}

	void setReady()					{ready= true;}
	const string &getName() const	{return name;}
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

protected:

	Mutex * getServerSynchAccessor();
	std::vector<std::string> threadErrorList;
};

}}//end namespace

#endif
