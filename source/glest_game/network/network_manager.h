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

#ifndef _GLEST_GAME_NETWORKMANAGER_H_
#define _GLEST_GAME_NETWORKMANAGER_H_

#include <cassert>

#include "socket.h"
#include "checksum.h"
#include "server_interface.h"
#include "client_interface.h"

using Shared::Util::Checksum;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkManager
// =====================================================

enum NetworkRole{
	nrServer,
	nrClient,
	nrIdle
};

class NetworkManager{
private:
	GameNetworkInterface* gameNetworkInterface;
	NetworkRole networkRole;

public:
	static NetworkManager &getInstance();

	NetworkManager();
	void init(NetworkRole networkRole);
	void end();
	void update();

	bool isNetworkGame();
	GameNetworkInterface* getGameNetworkInterface();
	ServerInterface* getServerInterface();
	ClientInterface* getClientInterface();
};

}}//end namespace

#endif
