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

#include "server_interface.h"

#include <cassert>
#include <stdexcept>

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(){
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		slots[i]= NULL;
	}
	serverSocket.setBlock(false);
	serverSocket.bind(GameConstants::serverPort);
}

ServerInterface::~ServerInterface(){
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		delete slots[i];
	}
}

void ServerInterface::addSlot(int playerIndex){
	assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);

	delete slots[playerIndex];
	slots[playerIndex]= new ConnectionSlot(this, playerIndex);
	updateListen();
}

void ServerInterface::removeSlot(int playerIndex){
	delete slots[playerIndex];
	slots[playerIndex]= NULL;
	updateListen();
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex){
	return slots[playerIndex];
}

int ServerInterface::getConnectedSlotCount(){
	int connectedSlotCount= 0;
	
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL){
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

void ServerInterface::update(){

	//update all slots
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL){
			slots[i]->update();
		}
	}

	//process text messages
	chatText.clear();
	chatSender.clear();
	chatTeamIndex= -1;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		if(connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				if(connectionSlot->getNextMessageType()==nmtText){
					NetworkMessageText networkMessageText;
					if(connectionSlot->receiveMessage(&networkMessageText)){
						broadcastMessage(&networkMessageText, i);
						chatText= networkMessageText.getText();
						chatSender= networkMessageText.getSender();
						chatTeamIndex= networkMessageText.getTeamIndex();
						break;
					}
				}
			}
		}
	}
}

void ServerInterface::updateKeyframe(int frameCount){

	NetworkMessageCommandList networkMessageCommandList(frameCount);
	
	//build command list, remove commands from requested and add to pending
	while(!requestedCommands.empty()){
		if(networkMessageCommandList.addCommand(&requestedCommands.back())){
			pendingCommands.push_back(requestedCommands.back());
			requestedCommands.pop_back();
		}
		else{
			break;
		}
	}

	//broadcast commands
	broadcastMessage(&networkMessageCommandList);
}

void ServerInterface::waitUntilReady(Checksum* checksum){
	
	Chrono chrono;
	bool allReady= false;

	chrono.start();

	//wait until we get a ready message from all clients
	while(!allReady){
		
		allReady= true;
		for(int i= 0; i<GameConstants::maxPlayers; ++i){
			ConnectionSlot* connectionSlot= slots[i];

			if(connectionSlot!=NULL){
				if(!connectionSlot->isReady()){
					NetworkMessageType networkMessageType= connectionSlot->getNextMessageType();
					NetworkMessageReady networkMessageReady;

					if(networkMessageType==nmtReady && connectionSlot->receiveMessage(&networkMessageReady)){
						connectionSlot->setReady();
					}
					else if(networkMessageType!=nmtInvalid){
						throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
					}

					allReady= false;
				}
			}
		}

		//check for timeout
		if(chrono.getMillis()>readyWaitTimeout){
			throw runtime_error("Timeout waiting for clients");
		}
	}

	//send ready message after, so clients start delayed
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		NetworkMessageReady networkMessageReady(checksum->getSum());
		ConnectionSlot* connectionSlot= slots[i];

		if(connectionSlot!=NULL){
			connectionSlot->sendMessage(&networkMessageReady);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, getHostName(), teamIndex);
	broadcastMessage(&networkMessageText);
}

void ServerInterface::quitGame(){
	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
}

string ServerInterface::getNetworkStatus() const{
	Lang &lang= Lang::getInstance();
	string str;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];
		
		str+= intToStr(i)+ ": ";

		if(connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				str+= connectionSlot->getName();
			}
		}
		else
		{
			str+= lang.get("NotConnected");
		}

		str+= '\n';
	}
	return str;
}

void ServerInterface::launchGame(const GameSettings* gameSettings){
	NetworkMessageLaunch networkMessageLaunch(gameSettings);
	broadcastMessage(&networkMessageLaunch);
}

void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot){
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		if(i!= excludeSlot && connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				connectionSlot->sendMessage(networkMessage);
			}
			else{
				removeSlot(i);
			}
		}
	}
}

void ServerInterface::updateListen(){
	int openSlotCount= 0;
	
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL && !slots[i]->isConnected()){
			++openSlotCount;
		}
	}

	serverSocket.listen(openSlotCount);
}

}}//end namespace
