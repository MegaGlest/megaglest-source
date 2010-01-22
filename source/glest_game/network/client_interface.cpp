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

#include "client_interface.h"

#include <stdexcept>
#include <cassert>

#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout= 10000;	//10 seconds
const int ClientInterface::waitSleepTime= 50;

ClientInterface::ClientInterface(){
	clientSocket= NULL;
	launchGame= false;
	introDone= false;
	playerIndex= -1;
}

ClientInterface::~ClientInterface(){
	delete clientSocket;
}

void ClientInterface::connect(const Ip &ip, int port){
	delete clientSocket;
	clientSocket= new ClientSocket();
	clientSocket->setBlock(false);
	clientSocket->connect(ip, port);
}

void ClientInterface::reset(){
	delete clientSocket;
	clientSocket= NULL;
}

void ClientInterface::update(){
	NetworkMessageCommandList networkMessageCommandList;

	//send as many commands as we can
	while(!requestedCommands.empty()){
		if(networkMessageCommandList.addCommand(&requestedCommands.back())){
			requestedCommands.pop_back();
		}
		else{
			break;
		}
	}
	if(networkMessageCommandList.getCommandCount()>0){
		sendMessage(&networkMessageCommandList);
	}

	//clear chat variables
	chatText.clear();
	chatSender.clear();
	chatTeamIndex= -1;
}

void ClientInterface::updateLobby(){
	NetworkMessageType networkMessageType= getNextMessageType();
	
	switch(networkMessageType){
		case nmtInvalid:
			break;

		case nmtIntro:{
			NetworkMessageIntro networkMessageIntro;

			if(receiveMessage(&networkMessageIntro)){
				
				//check consistency
				if(Config::getInstance().getBool("NetworkConsistencyChecks")){
					if(networkMessageIntro.getVersionString()!=getNetworkVersionString()){
						throw runtime_error("Server and client versions do not match (" + networkMessageIntro.getVersionString() + "). You have to use the same binaries.");
					}	
				}

				//send intro message
				NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(), getHostName(), -1);

				playerIndex= networkMessageIntro.getPlayerIndex();
				serverName= networkMessageIntro.getName();
				sendMessage(&sendNetworkMessageIntro);
					
				assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
				introDone= true;
			}
		}
		break;

		case nmtLaunch:{
			NetworkMessageLaunch networkMessageLaunch;

			if(receiveMessage(&networkMessageLaunch)){
				networkMessageLaunch.buildGameSettings(&gameSettings);

				//replace server player by network
				for(int i= 0; i<gameSettings.getFactionCount(); ++i){
					
					//replace by network
					if(gameSettings.getFactionControl(i)==ctHuman){
						gameSettings.setFactionControl(i, ctNetwork);
					}

					//set the faction index
					if(gameSettings.getStartLocationIndex(i)==playerIndex){
						gameSettings.setThisFactionIndex(i);
					}
				}
				launchGame= true;
			}
		}
		break;

		default:
			throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
	}
}

void ClientInterface::updateKeyframe(int frameCount){
	
	bool done= false;

	while(!done){
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType networkMessageType= getNextMessageType();

		switch(networkMessageType){
			case nmtCommandList:{
				//make sure we read the message
				NetworkMessageCommandList networkMessageCommandList;
				while(!receiveMessage(&networkMessageCommandList)){
					sleep(waitSleepTime);
				}

				//check that we are in the right frame
				if(networkMessageCommandList.getFrameCount()!=frameCount){
					throw runtime_error("Network synchronization error, frame counts do not match");
				}

				// give all commands
				for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i){
					pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
				}				

				done= true;
			}
			break;

			case nmtQuit:{
				NetworkMessageQuit networkMessageQuit;
				if(receiveMessage(&networkMessageQuit)){
					quit= true;
				}
				done= true;
			}
			break;

			case nmtText:{
				NetworkMessageText networkMessageText;
				if(receiveMessage(&networkMessageText)){
					chatText= networkMessageText.getText();
					chatSender= networkMessageText.getSender();
					chatTeamIndex= networkMessageText.getTeamIndex();
				}
			}
			break;

			default:
				throw runtime_error("Unexpected message in client interface: " + intToStr(networkMessageType));
		}
	}
}

void ClientInterface::waitUntilReady(Checksum* checksum){
	
	NetworkMessageReady networkMessageReady;
	Chrono chrono;

	chrono.start();

	//send ready message
	sendMessage(&networkMessageReady);

	//wait until we get a ready message from the server
	while(true){
		
		NetworkMessageType networkMessageType= getNextMessageType();

		if(networkMessageType==nmtReady){
			if(receiveMessage(&networkMessageReady)){
				break;
			}
		}
		else if(networkMessageType==nmtInvalid){
			if(chrono.getMillis()>readyWaitTimeout){
				throw runtime_error("Timeout waiting for server");
			}
		}
		else{
			throw runtime_error("Unexpected network message: " + intToStr(networkMessageType) );
		}

		// sleep a bit
		sleep(waitSleepTime);
	}

	//check checksum
	if(Config::getInstance().getBool("NetworkConsistencyChecks")){
		if(networkMessageReady.getChecksum()!=checksum->getSum()){
			throw runtime_error("Checksum error, you don't have the same data as the server");
		}
	}

	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, getHostName(), teamIndex);
	sendMessage(&networkMessageText);
}

string ClientInterface::getNetworkStatus() const{
	return Lang::getInstance().get("Server") + ": " + serverName;
}

void ClientInterface::waitForMessage(){
	Chrono chrono;

	chrono.start();

	while(getNextMessageType()==nmtInvalid){

		if(!isConnected()){
			throw runtime_error("Disconnected");
		}

		if(chrono.getMillis()>messageWaitTimeout){
			throw runtime_error("Timeout waiting for message");
		}

		sleep(waitSleepTime);
	}
}

}}//end namespace
