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

#ifndef _GLEST_GAME_CONNECTIONSLOT_H_
#define _GLEST_GAME_CONNECTIONSLOT_H_

#include <vector>

#include "socket.h"

#include "network_interface.h"

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

public:
	ConnectionSlot(ServerInterface* serverInterface, int playerIndex);
	~ConnectionSlot();

	virtual void update();

	void setReady()					{ready= true;}
	const string &getName() const	{return name;}
	bool isReady() const			{return ready;}

protected:
	virtual Socket* getSocket()				{return socket;}
	virtual Socket* getSocket() const		{return socket;}

private:
	void close();
};

}}//end namespace

#endif
