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
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class Console
// =====================================================

Console::Console(){
	//config
	maxLines= Config::getInstance().getInt("ConsoleMaxLines");
	timeout= Config::getInstance().getInt("ConsoleTimeout");

	timeElapsed= 0.0f;
}

void Console::addStdMessage(const string &s){
	addLine(Lang::getInstance().get(s));
}

void Console::addLine(string line, bool playSound){
	if(playSound){
		SoundRenderer::getInstance().playFx(CoreData::getInstance().getClickSoundA());
	}
	lines.insert(lines.begin(), StringTimePair(line, timeElapsed));
	if(lines.size()>maxLines){
		lines.pop_back();
	}
}

void Console::update(){
	timeElapsed+= 1.f/GameConstants::updateFps;
	
	if(!lines.empty()){
		if(lines.back().second<timeElapsed-timeout){
			lines.pop_back();
		}
    }   
}

bool Console::isEmpty(){
	return lines.empty();   
}
         

}}//end namespace
