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

#include "console.h"

#include "lang.h"
#include "config.h"
#include "program.h"
#include "game_constants.h"
#include "sound_renderer.h"
#include "core_data.h"
#include <stdexcept>
#include "network_manager.h"
#include "leak_dumper.h"

using namespace std;

namespace Glest{ namespace Game{

// =====================================================
// 	class Console
// =====================================================

Console::Console() {
	//config
	maxLines		= Config::getInstance().getInt("ConsoleMaxLines");
	maxStoredLines	= Config::getInstance().getInt("ConsoleMaxLinesStored");
	timeout			= Config::getInstance().getInt("ConsoleTimeout");
	timeElapsed		= 0.0f;
	xPos=20;
	yPos=20;
	lineHeight=Config::getInstance().getInt("FontConsoleBaseSize","18")+2;
	font=CoreData::getInstance().getConsoleFont();
	font3D=CoreData::getInstance().getConsoleFont3D();
	stringToHighlight="";
	onlyChatMessagesInStoredLines=true;
}

void Console::resetFonts() {
	font=CoreData::getInstance().getConsoleFont();
	font3D=CoreData::getInstance().getConsoleFont3D();
}

void Console::addStdMessage(const string &s,bool clearOtherLines) {
	if(clearOtherLines == true) {
		addLineOnly(Lang::getInstance().get(s));
	}
	else {
		addLine(Lang::getInstance().get(s));
	}
}

void Console::addStdScenarioMessage(const string &s,bool clearOtherLines) {
	if(clearOtherLines == true) {
		addLineOnly(Lang::getInstance().getScenarioString(s));
	}
	else {
		addLine(Lang::getInstance().getScenarioString(s));
	}
}

void Console::addLineOnly(string line) {
	addLine(line,false,-1,Vec3f(1.f, 1.f, 1.f),false,true);
}

void Console::addLine(string line, bool playSound, int playerIndex, Vec3f textColor, bool teamMode,bool clearOtherLines) {
	try {
		if(playSound == true) {
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getClickSoundA());
		}
		ConsoleLineInfo info;
		info.text               = line;
		info.timeStamp          = timeElapsed;
		info.PlayerIndex        = playerIndex;
		info.originalPlayerName	= "";
		info.color				= textColor;
		info.teamMode			= teamMode;
		if(playerIndex >= 0) {
			GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
			if(gameNetworkInterface != NULL) {
				info.originalPlayerName	= gameNetworkInterface->getGameSettings()->getNetworkPlayerNameByPlayerIndex(playerIndex);
				//for(int i = 0; i < GameConstants::maxPlayers; ++i) {
				//	printf("i = %d, playerName = [%s]\n",i,gameNetworkInterface->getGameSettings()->getNetworkPlayerName(i).c_str());
				//}
			}
		}
		//printf("info.PlayerIndex = %d, line [%s]\n",info.PlayerIndex,info.originalPlayerName.c_str());

		if(clearOtherLines == true) {
			lines.clear();
			storedLines.clear();
		}
		lines.insert(lines.begin(), info);
		if(lines.size() > maxLines) {
			lines.pop_back();
		}
		if(onlyChatMessagesInStoredLines==false || info.PlayerIndex!=-1) {
			storedLines.insert(storedLines.begin(), info);
			if(storedLines.size() > maxStoredLines) {
				storedLines.pop_back();
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
}

void Console::addLine(string line, bool playSound, string playerName, Vec3f textColor, bool teamMode) {
	try {
		if(playSound == true) {
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getClickSoundA());
		}
		ConsoleLineInfo info;
		info.text               = line;
		info.timeStamp          = timeElapsed;
		info.PlayerIndex        = -1;
		info.originalPlayerName	= "";
		info.color				= textColor;
		info.teamMode			= teamMode;
		if(playerName != "") {
			info.originalPlayerName	= playerName;
		}
		//printf("info.PlayerIndex = %d, line [%s]\n",info.PlayerIndex,info.originalPlayerName.c_str());

		lines.insert(lines.begin(), info);
		if(lines.size() > maxLines) {
			lines.pop_back();
		}
		storedLines.insert(storedLines.begin(), info);
		if(storedLines.size() > maxStoredLines) {
			storedLines.pop_back();
		}
	}
	catch(const exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
}

void Console::clearStoredLines() {
	while(storedLines.empty() == false) {
		storedLines.pop_back();
    }
}

void Console::update() {
	timeElapsed += 1.f / GameConstants::updateFps;

	if(lines.empty() == false) {
		if(lines.back().timeStamp < (timeElapsed - timeout)) {
			lines.pop_back();
		}
    }
}

bool Console::isEmpty() {
	return lines.empty();
}

string Console::getLine(int i) const {
	if(i < 0 || i >= lines.size())
		throw megaglest_runtime_error("i >= Lines.size()");
	return lines[i].text;
}

string Console::getStoredLine(int i) const {
	if(i < 0 || i >= storedLines.size())
		throw megaglest_runtime_error("i >= storedLines.size()");
	return storedLines[i].text;
}

ConsoleLineInfo Console::getLineItem(int i) const {
	if(i < 0 || i >= lines.size())
		throw megaglest_runtime_error("i >= Lines.size()");
	return lines[i];
}

ConsoleLineInfo Console::getStoredLineItem(int i) const {
	if(i < 0 || i >= storedLines.size())
		throw megaglest_runtime_error("i >= storedLines.size()");
	return storedLines[i];
}


}}//end namespace
