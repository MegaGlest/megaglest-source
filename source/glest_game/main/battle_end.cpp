// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "battle_end.h"	

#include "main_menu.h"
#include "program.h"
#include "core_data.h"
#include "lang.h"
#include "util.h"
#include "renderer.h"
#include "main_menu.h"
#include "sound_renderer.h"
#include "components.h"
#include "metrics.h"
#include "stats.h"
#include "auto_test.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class BattleEnd  
// =====================================================

BattleEnd::BattleEnd(Program *program, const Stats *stats):
ProgramState(program){
	this->stats= *stats;
}

BattleEnd::~BattleEnd(){
	SoundRenderer::getInstance().playMusic(CoreData::getInstance().getMenuMusic());
}

void BattleEnd::update(){
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateBattleEnd(program);
	}
}

void BattleEnd::render(){
	Renderer &renderer= Renderer::getInstance();
	TextRenderer2D *textRenderer= renderer.getTextRenderer();
	Lang &lang= Lang::getInstance();

	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderBackground(CoreData::getInstance().getBackgroundTexture());
	
	textRenderer->begin(CoreData::getInstance().getMenuFontBig());

	int lm= 20;
	int bm= 100;

	for(int i=0; i<stats.getFactionCount(); ++i){

		int textX= lm+160+i*100;
		int team= stats.getTeam(i) + 1;
		int kills= stats.getKills(i);
		int deaths= stats.getDeaths(i);
		int unitsProduced= stats.getUnitsProduced(i);
		int resourcesHarvested= stats.getResourcesHarvested(i);

		int score= kills*100 + unitsProduced*50 + resourcesHarvested/10;
		string controlString;

		switch(stats.getControl(i)){
		case ctCpuEasy:
			controlString= lang.get("CpuEasy");
			break;
		case ctCpu:
			controlString= lang.get("Cpu");
			break;
		case ctCpuUltra:
			controlString= lang.get("CpuUltra");
			break;
		case ctCpuMega:
			controlString= lang.get("CpuMega");
			break;
		case ctNetwork:
			controlString= lang.get("Network");
			break;
		case ctHuman:
			controlString= lang.get("Human");
			break;
		default:
			assert(false);
		};

		textRenderer->render((lang.get("Player")+" "+intToStr(i+1)).c_str(), textX, bm+400);
		textRenderer->render(stats.getVictory(i)? lang.get("Victory").c_str(): lang.get("Defeat").c_str(), textX, bm+360);
		textRenderer->render(controlString, textX, bm+320);
		textRenderer->render(stats.getFactionTypeName(i), textX, bm+280);
		textRenderer->render(intToStr(team).c_str(), textX, bm+240);
		textRenderer->render(intToStr(kills).c_str(), textX, bm+200);
		textRenderer->render(intToStr(deaths).c_str(), textX, bm+160);
		textRenderer->render(intToStr(unitsProduced).c_str(), textX, bm+120);
		textRenderer->render(intToStr(resourcesHarvested).c_str(), textX, bm+80);
		textRenderer->render(intToStr(score).c_str(), textX, bm+20);
	}

	textRenderer->render(lang.get("Result"), lm, bm+360);
	textRenderer->render(lang.get("Control"), lm, bm+320);
	textRenderer->render(lang.get("Faction"), lm, bm+280);
	textRenderer->render(lang.get("Team"), lm, bm+240);
	textRenderer->render(lang.get("Kills"), lm, bm+200);
	textRenderer->render(lang.get("Deaths"), lm, bm+160);
	textRenderer->render(lang.get("UnitsProduced"), lm, bm+120);
	textRenderer->render(lang.get("ResourcesHarvested"), lm, bm+80);
	textRenderer->render(lang.get("Score"), lm, bm+20);

	textRenderer->end();

	textRenderer->begin(CoreData::getInstance().getMenuFontVeryBig());

	string header = stats.getDescription() + " - ";

	if(stats.getVictory(stats.getThisFactionIndex())){
		header += lang.get("Victory");
	}
	else{
		header += lang.get("Defeat");
	}

	textRenderer->render(header, lm+250, bm+550);

	textRenderer->end();
	renderer.swapBuffers();
}

void BattleEnd::keyDown(char key){
	program->setState(new MainMenu(program));
}

void BattleEnd::mouseDownLeft(int x, int y){
	program->setState(new MainMenu(program));
}

}}//end namespace
