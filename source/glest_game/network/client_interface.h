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

#include <vector>

#include "network_interface.h"
#include "game_settings.h"

#include "socket.h"

using Shared::Platform::Ip;
using Shared::Platform::ClientSocket;
using std::vector;

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================

class ClientInterface: public GameNetworkInterface{
private:
	static const int messageWaitTimeout;
	static const int waitSleepTime;

private:
	ClientSocket *clientSocket;
	GameSettings gameSettings;
	string serverName;
	bool introDone;
	bool launchGame;
	int playerIndex;

public:
	ClientInterface();
	virtual ~ClientInterface();

	virtual Socket* getSocket()					{return clientSocket;}
	virtual const Socket* getSocket() const		{return clientSocket;}

	//message processing
	virtual void update();
	virtual void updateLobby();
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum* checksum);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame(){}

	//misc
	virtual string getNetworkStatus() const;

	//accessors
	string getServerName() const			{return serverName;}
	bool getLaunchGame() const				{return launchGame;}
	bool getIntroDone() const				{return introDone;}
	int getPlayerIndex() const				{return playerIndex;}
	const GameSettings *getGameSettings()	{return &gameSettings;}

	void connect(const Ip &ip, int port);
	void reset();

private:
	void waitForMessage();
};

}}//end namespace

#endif
