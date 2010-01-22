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

#include "network_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkInterface
// =====================================================

const int NetworkInterface::readyWaitTimeout= 60000;	//1 minute


void NetworkInterface::sendMessage(const NetworkMessage* networkMessage){
	Socket* socket= getSocket();

	networkMessage->send(socket);
}

NetworkMessageType NetworkInterface::getNextMessageType(){
	Socket* socket= getSocket();
	int8 messageType= nmtInvalid;

	//peek message type
	if(socket->getDataToRead()>=sizeof(messageType)){
		socket->peek(&messageType, sizeof(messageType));
	}

	//sanity check new message type
	if(messageType<0 || messageType>=nmtCount){
		throw runtime_error("Invalid message type: " + intToStr(messageType));
	}

	return static_cast<NetworkMessageType>(messageType);
}

bool NetworkInterface::receiveMessage(NetworkMessage* networkMessage){
	Socket* socket= getSocket();

	return networkMessage->receive(socket);
}

bool NetworkInterface::isConnected(){
	return getSocket()!=NULL && getSocket()->isConnected();
}

// =====================================================
//	class GameNetworkInterface
// =====================================================

GameNetworkInterface::GameNetworkInterface(){
	quit= false;
}

}}//end namespace
