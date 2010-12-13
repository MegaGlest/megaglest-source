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

#include "network_manager.h"
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkManager
// =====================================================

NetworkManager &NetworkManager::getInstance(){
	static NetworkManager networkManager;
	return networkManager;
}

NetworkManager::NetworkManager() {
	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d this->networkRole = %d\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole);

	gameNetworkInterface= NULL;
	networkRole= nrIdle;

	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d this->networkRole = %d\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole);
}

void NetworkManager::init(NetworkRole networkRole) {
	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d this->networkRole = %d\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole);

	assert(gameNetworkInterface==NULL);

	this->networkRole = networkRole;

	if(networkRole==nrServer){
		gameNetworkInterface = new ServerInterface();
	}
	else {
		gameNetworkInterface = new ClientInterface();
	}

	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d this->networkRole = %d\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole);
}

void NetworkManager::end() {
	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d this->networkRole = %d\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole);

	delete gameNetworkInterface;
	gameNetworkInterface= NULL;
	networkRole= nrIdle;

	SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d this->networkRole = %d\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole);
}

void NetworkManager::update(){
	if(gameNetworkInterface!=NULL){
		gameNetworkInterface->update();
	}
}

bool NetworkManager::isNetworkGame(){
	return networkRole==nrClient || getServerInterface()->getConnectedSlotCount()>0;
}

GameNetworkInterface* NetworkManager::getGameNetworkInterface(){
	assert(gameNetworkInterface!=NULL);
	if(gameNetworkInterface==NULL) {
	    throw runtime_error("gameNetworkInterface==NULL");
	}
	return gameNetworkInterface;
}

ServerInterface* NetworkManager::getServerInterface(){
	assert(gameNetworkInterface!=NULL);
	if(gameNetworkInterface==NULL) {
	    throw runtime_error("gameNetworkInterface==NULL");
	}

	assert(networkRole==nrServer);
	if(networkRole!=nrServer) {
	    throw runtime_error("networkRole!=nrServer");
	}

	return dynamic_cast<ServerInterface*>(gameNetworkInterface);
}

ClientInterface* NetworkManager::getClientInterface(){
	assert(gameNetworkInterface!=NULL);
	if(gameNetworkInterface==NULL) {
	    throw runtime_error("gameNetworkInterface==NULL");
	}

	assert(networkRole==nrClient);
	if(networkRole!=nrClient) {
	    throw runtime_error("networkRole!=nrClient");
	}

	return dynamic_cast<ClientInterface*>(gameNetworkInterface);
}

}}//end namespace
