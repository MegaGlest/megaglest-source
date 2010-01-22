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

#ifndef _GLEST_GAME_NETWORKINTERFACE_H_
#define _GLEST_GAME_NETWORKINTERFACE_H_

#include <string>
#include <vector>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"

using std::string;
using std::vector;
using Shared::Util::Checksum;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkInterface
// =====================================================

class NetworkInterface{
public:
	static const int readyWaitTimeout;

public:
	virtual ~NetworkInterface(){}

	virtual Socket* getSocket()= 0;
	virtual const Socket* getSocket() const= 0;

	string getIp() const		{return getSocket()->getIp();}
	string getHostName() const	{return getSocket()->getHostName();}

	void sendMessage(const NetworkMessage* networkMessage);
	NetworkMessageType getNextMessageType();
	bool receiveMessage(NetworkMessage* networkMessage);

	bool isConnected();
};

// =====================================================
//	class GameNetworkInterface
// 
// Adds functions common to servers and clients 
// but not connection slots
// =====================================================

class GameNetworkInterface: public NetworkInterface{
private:
	typedef vector<NetworkCommand> Commands;

protected:
	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given
	bool quit;
	string chatText;
	string chatSender;
	int chatTeamIndex;

public:
	GameNetworkInterface();

	//message processimg
	virtual void update()= 0;
	virtual void updateLobby()= 0;
	virtual void updateKeyframe(int frameCount)= 0;
	virtual void waitUntilReady(Checksum* checksum)= 0;

	//message sending
	virtual void sendTextMessage(const string &text, int teamIndex)= 0;
	virtual void quitGame()=0;

	//misc
	virtual string getNetworkStatus() const= 0;

	//access functions
	void requestCommand(const NetworkCommand *networkCommand)	{requestedCommands.push_back(*networkCommand);}
	int getPendingCommandCount() const							{return pendingCommands.size();}
	const NetworkCommand* getPendingCommand(int i) const		{return &pendingCommands[i];}
	void clearPendingCommands()									{pendingCommands.clear();}
	bool getQuit() const										{return quit;}
	const string getChatText() const							{return chatText;}
	const string getChatSender() const							{return chatSender;}
	int getChatTeamIndex() const								{return chatTeamIndex;}
};

}}//end namespace

#endif
