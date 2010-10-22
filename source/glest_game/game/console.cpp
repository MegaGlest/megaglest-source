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

#include "console.h"

#include "lang.h"
#include "config.h"
#include "program.h"
#include "game_constants.h"
#include "sound_renderer.h"
#include "core_data.h"
#include <stdexcept>
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
}

void Console::addStdMessage(const string &s) {
	addLine(Lang::getInstance().get(s));
}

void Console::addLine(string line, bool playSound, int playerIndex) {
	try {
		if(playSound == true) {
			SoundRenderer::getInstance().playFx(CoreData::getInstance().getClickSoundA());
		}
		lines.insert(lines.begin(), StringTimePair(line, StringTimePairData(timeElapsed,playerIndex)));
		if(lines.size() > maxLines) {
			lines.pop_back();
		}
		storedLines.insert(storedLines.begin(), StringTimePair(line, StringTimePairData(timeElapsed,playerIndex)));
		if(storedLines.size() > maxStoredLines) {
			storedLines.pop_back();
		}
	}
	catch(const exception &ex) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw runtime_error(szBuf);
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
		if(lines.back().second.first < (timeElapsed - timeout)) {
			lines.pop_back();
		}
    }   
}

bool Console::isEmpty() {
	return lines.empty();   
}
         

}}//end namespace
