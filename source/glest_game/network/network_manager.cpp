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
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,gameNetworkInterface);

	gameNetworkInterface= NULL;
	networkRole= nrIdle;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,gameNetworkInterface);
}

void NetworkManager::init(NetworkRole networkRole, bool publishEnabled) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d, networkRole = %d, gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,networkRole,gameNetworkInterface);

	assert(gameNetworkInterface==NULL);

	this->networkRole = networkRole;

	if(networkRole == nrServer) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d, networkRole = %d, gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,networkRole,gameNetworkInterface);
		gameNetworkInterface = new ServerInterface(publishEnabled);
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d, networkRole = %d, gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,networkRole,gameNetworkInterface);
		gameNetworkInterface = new ClientInterface();
	}

    //printf("==========] CREATING gameNetworkInterface [%p]\n",gameNetworkInterface);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,gameNetworkInterface);
}

void NetworkManager::end() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,gameNetworkInterface);

    //printf("==========] DELETING gameNetworkInterface [%p]\n",gameNetworkInterface);

	delete gameNetworkInterface;
	gameNetworkInterface= NULL;
	networkRole= nrIdle;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,gameNetworkInterface);
}

void NetworkManager::update() {
	if(gameNetworkInterface!=NULL) {
		gameNetworkInterface->update();
	}
}

bool NetworkManager::isNetworkGame() {
	return networkRole==nrClient || (networkRole==nrServer && getServerInterface()->getConnectedSlotCount()>0);
}

GameNetworkInterface* NetworkManager::getGameNetworkInterface(bool throwErrorOnNull) {
    if(throwErrorOnNull) {
        assert(gameNetworkInterface!=NULL);

        if(gameNetworkInterface==NULL) {
            throw runtime_error("gameNetworkInterface==NULL");
        }
    }
	return gameNetworkInterface;
}

ServerInterface* NetworkManager::getServerInterface(bool throwErrorOnNull) {
    if(throwErrorOnNull) {
        assert(gameNetworkInterface!=NULL);
        if(gameNetworkInterface==NULL) {
            throw runtime_error("gameNetworkInterface==NULL");
        }

        assert(networkRole==nrServer);
        if(networkRole!=nrServer) {
            throw runtime_error("networkRole!=nrServer");
        }
    }
	return dynamic_cast<ServerInterface*>(gameNetworkInterface);
}

ClientInterface* NetworkManager::getClientInterface(bool throwErrorOnNull) {
	//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] this->networkRole = %d gameNetworkInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,this->networkRole,gameNetworkInterface);

    if(throwErrorOnNull) {
        assert(gameNetworkInterface!=NULL);
        if(gameNetworkInterface==NULL) {
            throw runtime_error("gameNetworkInterface==NULL");
        }

        assert(networkRole==nrClient);
        if(networkRole!=nrClient) {
            throw runtime_error("networkRole!=nrClient");
        }
    }
	return dynamic_cast<ClientInterface*>(gameNetworkInterface);
}

}}//end namespace
