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

#include "chat_manager.h"

#include "window.h"
#include "console.h"
#include "config.h"
#include "network_manager.h"
#include "lang.h"
#include "util.h"
#include <stdexcept>
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

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
	disableTeamMode = false;
}

void ChatManager::init(Console* console, int thisTeamIndex){
	this->console= console;
	this->thisTeamIndex= thisTeamIndex;
	this->disableTeamMode= false;
}

void ChatManager::keyUp(char key){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	
	try {
		if(editEnabled){
			if(key==vkEscape)
			{
				text.clear();
				editEnabled= false;
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ChatManager::setDisableTeamMode(bool value) {
	disableTeamMode = value;

	if(disableTeamMode == true) {
		teamMode = false;
	}
}

void ChatManager::keyDown(char key){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	try {
		Lang &lang= Lang::getInstance();

		//toggle team mode
		if(editEnabled == false && disableTeamMode == false && key=='H') {
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
				if(!text.empty()) {
					console->addLine(Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + ": " + text);
					gameNetworkInterface->sendTextMessage(Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + ": "+
						text, teamMode? thisTeamIndex: -1);
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
	catch(const exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ChatManager::keyPress(char c){
	if(editEnabled && text.size()<maxTextLenght){
		//space is the first meaningful code
		if(c>=' '){
			text+= c;
		}
	}
}

void ChatManager::updateNetwork() {
	try {
		GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
		string text;
		string sender;
		Config &config= Config::getInstance();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameNetworkInterface->getChatText() [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameNetworkInterface->getChatText().c_str());

		if(gameNetworkInterface->getChatText().empty() == false) {
			int teamIndex= gameNetworkInterface->getChatTeamIndex();

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] got nmtText [%s] for team = %d\n",__FILE__,__FUNCTION__,gameNetworkInterface->getChatText().c_str(),teamIndex);

			if(teamIndex==-1 || teamIndex==thisTeamIndex){
				console->addLine(gameNetworkInterface->getChatText(), true);

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Added text to console\n",__FILE__,__FUNCTION__);
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			gameNetworkInterface->clearChatInfo();

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		throw runtime_error(szBuf);
	}
}

}}//end namespace
