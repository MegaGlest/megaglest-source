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

#include "chat_manager.h"

#include "window.h"
#include "console.h"
#include "network_manager.h"
#include "lang.h"
#include "leak_dumper.h"

using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
//	class ChatManager
// =====================================================

const int ChatManager::maxTextLenght= 64;

ChatManager::ChatManager(){
	console= NULL;
	editEnabled= false;
	teamMode= false;
	thisTeamIndex= -1;
}

void ChatManager::init(Console* console, int thisTeamIndex){
	this->console= console;
	this->thisTeamIndex= thisTeamIndex;
}

void ChatManager::keyDown(char key){
	
	Lang &lang= Lang::getInstance();

	//toggle team mode
	if(!editEnabled && key=='H'){
		if(teamMode){
			teamMode= false;
			console->addLine(lang.get("ChatMode") + ": " + lang.get("All"));
		}
		else{
			teamMode= true;
			console->addLine(lang.get("ChatMode") + ": " + lang.get("Team"));
		}
	}

	if(key==vkReturn){
		if(editEnabled){
			GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();

			editEnabled= false;
			if(!text.empty()){
				console->addLine(gameNetworkInterface->getHostName() + ": " + text);
				gameNetworkInterface->sendTextMessage(text, teamMode? thisTeamIndex: -1);
			}
		}
		else{
			editEnabled= true;
			text.clear();
		}
	}
	else if(key==vkBack){
		if(!text.empty()){
			text.erase(text.end() -1);
		}
	}
}

void ChatManager::keyPress(char c){
	if(editEnabled && text.size()<maxTextLenght){
		//space is the first meaningful code
		if(c>=' '){
			text+= c;
		}
	}
}

void ChatManager::updateNetwork(){
	GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
	string text;
	string sender;
	
	if(!gameNetworkInterface->getChatText().empty()){
		int teamIndex= gameNetworkInterface->getChatTeamIndex();
		
		if(teamIndex==-1 || teamIndex==thisTeamIndex){
			console->addLine(gameNetworkInterface->getChatSender()+": "+gameNetworkInterface->getChatText(), true);
		}
	}
}
	
}}//end namespace
