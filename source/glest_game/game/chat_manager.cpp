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
#include "core_data.h"
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

ChatManager::ChatManager() {
	console= NULL;
	editEnabled= false;
	teamMode= false;
	thisTeamIndex= -1;
	disableTeamMode = false;
	xPos=300;
	yPos=150;
	maxTextLenght=64;
	font=CoreData::getInstance().getConsoleFont();
}

void ChatManager::init(Console* console, int thisTeamIndex, const bool inMenu, string manualPlayerNameOverride) {
	this->console= console;
	this->thisTeamIndex= thisTeamIndex;
	this->disableTeamMode= false;
	this->inMenu=inMenu;
	this->manualPlayerNameOverride = manualPlayerNameOverride;
}

void ChatManager::keyUp(char key) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	try {
		if(editEnabled) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

			if(key == vkEscape || key == SDLK_ESCAPE) {
				text.clear();
				editEnabled= false;
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
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

void ChatManager::keyDown(char key) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

	try {
		Lang &lang= Lang::getInstance();
		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		//toggle team mode
		if(editEnabled == false && disableTeamMode == false &&
			key == configKeys.getCharKey("ChatTeamMode")) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

			if (!inMenu) {
				if (teamMode == true) {
					teamMode = false;
					console->addLine(lang.get("ChatMode") + ": " + lang.get(
							"All"));
				} else {
					teamMode = true;
					console->addLine(lang.get("ChatMode") + ": " + lang.get(
							"Team"));
				}
			}
		}

		if(key==vkReturn){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

			SDL_keysym keystate = Window::getKeystate();
			if(keystate.mod & (KMOD_LALT | KMOD_RALT)){
				// alt+enter is ignored
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);
			}
			else
			{
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

				if(editEnabled == true) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

					GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
					if(text.empty() == false) {
						string playerName 	= gameNetworkInterface->getHumanPlayerName();
						int playerIndex 	= gameNetworkInterface->getHumanPlayerIndex();

						if(this->manualPlayerNameOverride != "") {
						    console->addLine(text,false,this->manualPlayerNameOverride);
						}
						else {
                            console->addLine(text,false,playerIndex);
						}

						gameNetworkInterface->sendTextMessage(text, teamMode? thisTeamIndex: -1, false, "");
						if(inMenu == false) {
							editEnabled= false;
						}
					}
					else {
						editEnabled= false;
					}
					text.clear();
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

					switchOnEdit();
				}
			}
		}
		else if(key==vkBack) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

			if(!text.empty()) {
				text.erase(text.end() -1);
			}
		}

	}
	catch(const exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ChatManager::switchOnEdit(){
	editEnabled= true;
	text.clear();
}

void ChatManager::keyPress(char c){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c,c);

	if(editEnabled && text.size()<maxTextLenght){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c,c);
		//space is the first meaningful code
		if(c>=' '){
			text+= c;
		}
	}
}

void ChatManager::addText(string text){
	if(editEnabled && text.size()+this->text.size()<maxTextLenght){
		this->text+= text;
	}
}

void ChatManager::updateNetwork() {
	try {
		GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
		//string text;
		//string sender;
		Config &config= Config::getInstance();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameNetworkInterface->getChatText() [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameNetworkInterface->getChatText().c_str());

		if(gameNetworkInterface != NULL && gameNetworkInterface->getChatTextList().empty() == false) {
			for(int idx = 0; idx < gameNetworkInterface->getChatTextList().size(); idx++) {
				const ChatMsgInfo &msg = gameNetworkInterface->getChatTextList()[idx];
				int teamIndex= msg.chatTeamIndex;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] got nmtText [%s] for team = %d\n",__FILE__,__FUNCTION__,msg.chatText.c_str(),teamIndex);

				if(teamIndex == -1 || teamIndex == thisTeamIndex) {
					//console->addLine(msg.chatSender + ": " + msg.chatText, true, msg.chatPlayerIndex);
					console->addLine(msg.chatText, true, msg.chatPlayerIndex);

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Added text to console\n",__FILE__,__FUNCTION__);
				}

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			gameNetworkInterface->clearChatInfo();
		}
	}
	catch(const std::exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
	}
}

}}//end namespace
