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

BattleEnd::BattleEnd(Program *program, const Stats *stats,ProgramState *originState): ProgramState(program) {

	containerName= "BattleEnd";

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] stats = %p\n",__FILE__,__FUNCTION__,__LINE__,stats);

	this->originState = originState;
	if(stats != NULL) {
		this->stats= *stats;
	}
	mouseX = 0;
	mouseY = 0;
	mouse2d = 0;
	renderToTexture = NULL;

	const Metrics &metrics= Metrics::getInstance();
	Lang &lang= Lang::getInstance();
	int buttonWidth = 125;
	int xLocation = (metrics.getVirtualW() / 2) - (buttonWidth / 2);
	buttonExit.init(xLocation, 80, buttonWidth);
	buttonExit.setText(lang.get("Exit"));

	//mesage box
	mainMessageBox.init(lang.get("Yes"), lang.get("No"));
	mainMessageBox.setEnabled(false);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	GraphicComponent::applyAllCustomProperties(containerName);
}

void BattleEnd::reloadUI() {
	Lang &lang= Lang::getInstance();

	buttonExit.setText(lang.get("Exit"));
	mainMessageBox.init(lang.get("Yes"), lang.get("No"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

BattleEnd::~BattleEnd() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete originState;
	originState = NULL;

	if(CoreData::getInstance().hasMainMenuVideoFilename() == false) {
		SoundRenderer::getInstance().playMusic(CoreData::getInstance().getMenuMusic());
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete renderToTexture;
	renderToTexture = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

const string BattleEnd::getTimeString(int frames) {
	int framesleft=frames;
	int hours=(int) frames / (float)GameConstants::updateFps / 3600.0;
	framesleft=framesleft-hours*3600*GameConstants::updateFps;
	int minutes=(int) framesleft / (float)GameConstants::updateFps / 60.0;
	framesleft=framesleft-minutes*60*GameConstants::updateFps;
	int seconds=(int) framesleft / (float)GameConstants::updateFps;
	framesleft=framesleft-seconds*GameConstants::updateFps;

	string hourstr=intToStr(hours);
	if(hours<10) hourstr="0"+hourstr;

	string minutestr=intToStr(minutes);
	if(minutes<10) minutestr="0"+minutestr;

	string secondstr=intToStr(seconds);
	if(seconds<10) secondstr="0"+secondstr;

	return hourstr+":"+minutestr+":"+secondstr;
}

void BattleEnd::update() {
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateBattleEnd(program);
		return;
	}
	mouse2d= (mouse2d+1) % Renderer::maxMouse2dAnim;

	if(this->stats.getIsMasterserverMode() == true) {
		if(program->getWantShutdownApplicationAfterGame() == true) {
			program->setShutdownApplicationEnabled(true);
			return;
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//program->setState(new MainMenu(program));
		program->initServer(program->getWindow(),false,true,true);
		return;
	}
}

void BattleEnd::render() {
	if(this->stats.getIsMasterserverMode() == true) {
		return;
	}
	Renderer &renderer= Renderer::getInstance();
	//CoreData &coreData= CoreData::getInstance();

	canRender();
	incrementFps();

	//printf("In [%s::%s Line: %d] renderToTexture [%p]\n",__FILE__,__FUNCTION__,__LINE__,renderToTexture);
	if(renderToTexture != NULL) {
		//printf("Rendering from texture!\n");

		renderer.clearBuffers();
		renderer.reset3dMenu();
		renderer.clearZBuffer();

		renderer.reset2d();

		renderer.renderBackground(renderToTexture);

		renderer.renderButton(&buttonExit);

		//exit message box
		if(mainMessageBox.getEnabled() && mainMessageBox.getVisible()) {
			renderer.renderMessageBox(&mainMessageBox);
		}

	    renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
	}
	else {
		//printf("Rendering to texture!\n");

		renderer.beginRenderToTexture(&renderToTexture);

		TextRenderer2D *textRenderer2D	= renderer.getTextRenderer();
		TextRenderer3D *textRenderer3D	= renderer.getTextRenderer3D();
		TextRenderer *textRenderer		= NULL;
	
		if(Renderer::renderText3DEnabled == true) {
			textRenderer= textRenderer3D;
		}
		else {
			textRenderer= textRenderer2D;
		}

		Lang &lang= Lang::getInstance();

		renderer.clearBuffers();
		renderer.reset3dMenu();
		renderer.clearZBuffer();
		renderer.reset2d();
		renderer.renderBackground(CoreData::getInstance().getBackgroundTexture());
		
		//int winnerIndex 	= -1;
		int bestScore 		= -1;
		//int mostKillsIndex 	= -1;
		int bestKills 		= -1;
		//int mostEnemyKillsIndex = -1;
		int bestEnemyKills 		= -1;
		//int leastDeathsIndex 	= -1;
		int leastDeaths			= -1;
		//int mostUnitsProducedIndex 	= -1;
		int bestUnitsProduced 		= -1;
		//int mostResourcesHarvestedIndex 	= -1;
		int bestResourcesHarvested 			= -1;

		for(int i=0; i<stats.getFactionCount(); ++i) {
			if(stats.getTeam(i) == GameConstants::maxPlayers -1 + fpt_Observer) {
				continue;
			}

			//int team= stats.getTeam(i) + 1;
			int kills= stats.getKills(i);
			if(kills > bestKills) {
				bestKills 	= kills;
				//mostKillsIndex = i;
			}

			int enemykills= stats.getEnemyKills(i);
			if(enemykills > bestEnemyKills) {
				bestEnemyKills 	= enemykills;
				//mostEnemyKillsIndex = i;
			}

			int deaths= stats.getDeaths(i);
			if(deaths < leastDeaths || leastDeaths < 0) {
				leastDeaths 	 = deaths;
				//leastDeathsIndex = i;
			}

			int unitsProduced= stats.getUnitsProduced(i);
			if(unitsProduced > bestUnitsProduced) {
				bestUnitsProduced 	   = unitsProduced;
				//mostUnitsProducedIndex = i;
			}

			int resourcesHarvested = stats.getResourcesHarvested(i);
			if(resourcesHarvested > bestResourcesHarvested) {
				bestResourcesHarvested 	   = resourcesHarvested;
				//mostResourcesHarvestedIndex = i;
			}

			int score= enemykills*100 + unitsProduced*50 + resourcesHarvested/10;

			if(score > bestScore) {
				bestScore 	= score;
				//winnerIndex = i;
			}
		}

		bool disableStatsColorCoding = Config::getInstance().getBool("DisableBattleEndColorCoding","false");

		if(Renderer::renderText3DEnabled == true) {
			textRenderer3D->begin(CoreData::getInstance().getMenuFontNormal3D());
		}
		else {
			textRenderer2D->begin(CoreData::getInstance().getMenuFontNormal());
		}

		int lm= 20;
		int bm= 100;

		int realPlayerCount = 0;
		for(int i = 0; i < stats.getFactionCount(); ++i) {
			if(stats.getTeam(i) == GameConstants::maxPlayers -1 + fpt_Observer) {
				continue;
			}

			realPlayerCount++;
			int textX= lm + 60 + (realPlayerCount*100);
			int team= stats.getTeam(i) + 1;
			int kills= stats.getKills(i);
			int enemykills= stats.getEnemyKills(i);
			int deaths= stats.getDeaths(i);
			int unitsProduced= stats.getUnitsProduced(i);
			int resourcesHarvested= stats.getResourcesHarvested(i);

			int score= enemykills*100 + unitsProduced*50 + resourcesHarvested/10;
			string controlString;

			if(stats.getPersonalityType(i) == fpt_Observer) {
				controlString= GameConstants::OBSERVER_SLOTNAME;
			}
			else {
				switch(stats.getControl(i)) {
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

				case ctNetworkCpuEasy:
					controlString= lang.get("NetworkCpuEasy");
					break;
				case ctNetworkCpu:
					controlString= lang.get("NetworkCpu");
					break;
				case ctNetworkCpuUltra:
					controlString= lang.get("NetworkCpuUltra");
					break;
				case ctNetworkCpuMega:
					controlString= lang.get("NetworkCpuMega");
					break;

				default:
					printf("Error control = %d for i = %d\n",stats.getControl(i),i);
					assert(false);
					break;
				};
			}

			if(stats.getControl(i) != ctHuman && stats.getControl(i) != ctNetwork ) {
				controlString += "\nx " + floatToStr(stats.getResourceMultiplier(i),1);
			}
			else if(stats.getPlayerLeftBeforeEnd(i)==true){
				controlString += "\n" +lang.get("CpuUltra")+"\nx "+floatToStr(stats.getResourceMultiplier(i),1);
			}

			if(score == bestScore && stats.getVictory(i)) {
				if(CoreData::getInstance().getGameWinnerTexture() != NULL) {
					renderer.renderTextureQuad(textX, bm+420,-1,-1,CoreData::getInstance().getGameWinnerTexture(),0.7f);
				}
			}

			Vec3f color = stats.getPlayerColor(i);
			if(stats.getPlayerName(i) != "") {
				string textToRender=stats.getPlayerName(i);
				if(stats.getPlayerLeftBeforeEnd(i)==true){
					textToRender+="\n("+getTimeString(stats.getTimePlayerLeft(i))+")";
				}

				textRenderer->render(textToRender.c_str(), textX, bm+400, false, &color);
			}
			else {
				textRenderer->render((lang.get("Player") + " " + intToStr(i+1)).c_str(), textX, bm+400,false, &color);
			}

			Vec3f highliteColor = Vec3f(WHITE.x,WHITE.y,WHITE.z);
			if(disableStatsColorCoding == false) {
				highliteColor.x = 0.85;
				highliteColor.y = 0.8;
				highliteColor.z = 0.07;
			}

			if(stats.getPersonalityType(i) == fpt_Observer) {
				textRenderer->render(lang.get("GameOver").c_str(), textX, bm+360);
			}
			else {
				if(stats.getVictory(i)) {
					textRenderer->render(stats.getVictory(i)? lang.get("Victory").c_str(): lang.get("Defeat").c_str(), textX, bm+360, false, &highliteColor);
				}
				else {
					textRenderer->render(stats.getVictory(i)? lang.get("Victory").c_str(): lang.get("Defeat").c_str(), textX, bm+360);
				}
			}

			textRenderer->render(controlString, textX, bm+320);
			textRenderer->render(stats.getFactionTypeName(i), textX, bm+280);
			textRenderer->render(intToStr(team).c_str(), textX, bm+240);

			if(kills == bestKills) {
				textRenderer->render(intToStr(kills).c_str(), textX, bm+200, false,&highliteColor);
			}
			else {
				textRenderer->render(intToStr(kills).c_str(), textX, bm+200);
			}
			if(enemykills == bestEnemyKills) {
				textRenderer->render(intToStr(enemykills).c_str(), textX, bm+180, false , &highliteColor);
			}
			else {
				textRenderer->render(intToStr(enemykills).c_str(), textX, bm+180);
			}
			if(deaths == leastDeaths) {
				textRenderer->render(intToStr(deaths).c_str(), textX, bm+160,false,&highliteColor);
			}
			else {
				textRenderer->render(intToStr(deaths).c_str(), textX, bm+160);
			}
			if(unitsProduced == bestUnitsProduced) {
				textRenderer->render(intToStr(unitsProduced).c_str(), textX, bm+120,false,&highliteColor);
			}
			else {
				textRenderer->render(intToStr(unitsProduced).c_str(), textX, bm+120);
			}
			if(resourcesHarvested == bestResourcesHarvested) {
				textRenderer->render(intToStr(resourcesHarvested).c_str(), textX, bm+80,false,&highliteColor);
			}
			else {
				textRenderer->render(intToStr(resourcesHarvested).c_str(), textX, bm+80);
			}
			if(score == bestScore) {
				textRenderer->render(intToStr(score).c_str(), textX, bm+20,false,&highliteColor);
			}
			else {
				textRenderer->render(intToStr(score).c_str(), textX, bm+20);
			}
		}

		textRenderer->render("\n"+(lang.get("left at")), lm, bm+400);
		textRenderer->render(lang.get("Result"), lm, bm+360);
		textRenderer->render(lang.get("Control"), lm, bm+320);
		textRenderer->render(lang.get("Faction"), lm, bm+280);
		textRenderer->render(lang.get("Team"), lm, bm+240);
		textRenderer->render(lang.get("Kills"), lm, bm+200);
		textRenderer->render(lang.get("EnemyKills"), lm, bm+180);
		textRenderer->render(lang.get("Deaths"), lm, bm+160);
		textRenderer->render(lang.get("UnitsProduced"), lm, bm+120);
		textRenderer->render(lang.get("ResourcesHarvested"), lm, bm+80);
		textRenderer->render(lang.get("Score"), lm, bm+20);

		textRenderer->end();

		if(Renderer::renderText3DEnabled == true) {
			textRenderer3D->begin(CoreData::getInstance().getMenuFontVeryBig3D());
		}
		else {
			textRenderer2D->begin(CoreData::getInstance().getMenuFontVeryBig());
		}

		string header = stats.getDescription() + " - ";

		if(stats.getTeam(stats.getThisFactionIndex()) != GameConstants::maxPlayers -1 + fpt_Observer) {
			if(stats.getVictory(stats.getThisFactionIndex())){
				header += lang.get("Victory");
			}
			else{
				header += lang.get("Defeat");
			}
		}
		else {
			header += "Observer";
		}
		textRenderer->render(header, lm+250, bm+550);

		//GameConstants::updateFps
		//string header2 = lang.get("GameDurationTime","",true) + " " + floatToStr(stats.getWorldTimeElapsed() / 24.0,2);

		string header2 = lang.get("GameDurationTime","",true) + ": " + getTimeString(stats.getFramesToCalculatePlaytime());
		textRenderer->render(header2, lm+250, bm+530);

		header2 = lang.get("GameMaxConcurrentUnitCount") + ": " + intToStr(stats.getMaxConcurrentUnitCount());
		textRenderer->render(header2, lm+250, bm+510);

		header2 = lang.get("GameTotalEndGameConcurrentUnitCount") + ": " + intToStr(stats.getTotalEndGameConcurrentUnitCount());
		textRenderer->render(header2, lm+250, bm+490);

		textRenderer->end();

		renderer.renderButton(&buttonExit);

		//exit message box
		if(mainMessageBox.getEnabled()){
			renderer.renderMessageBox(&mainMessageBox);
		}

		if(renderToTexture == NULL) {
			renderer.renderMouse2d(mouseX, mouseY, mouse2d, 0.f);
		}

		renderer.endRenderToTexture(&renderToTexture);
	}

	renderer.renderFPSWhenEnabled(lastFps);

	renderer.swapBuffers();
}

void BattleEnd::keyDown(SDL_KeyboardEvent key){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(isKeyPressed(SDLK_ESCAPE,key) == true) {
		//program->setState(new MainMenu(program));

		if(mainMessageBox.getEnabled()) {
			mainMessageBox.setEnabled(false);
		}
		else {
			Lang &lang= Lang::getInstance();
			showMessageBox(lang.get("ExitGame?"), "", true);
		}
	}
	else if(isKeyPressed(SDLK_RETURN,key) && mainMessageBox.getEnabled()) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		program->setState(new MainMenu(program));
	}
}

void BattleEnd::mouseDownLeft(int x, int y){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//program->setState(new MainMenu(program));

	if(buttonExit.mouseClick(x,y)) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		program->setState(new MainMenu(program));
	}
	else if(mainMessageBox.getEnabled()) {
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button)) {
			if(button==0) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				program->setState(new MainMenu(program));
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				//close message box
				mainMessageBox.setEnabled(false);
			}
		}
	}

}

void BattleEnd::mouseMove(int x, int y, const MouseState *ms){
	mouseX = x;
	mouseY = y;

	buttonExit.mouseMove(x, y);
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}

}

void BattleEnd::showMessageBox(const string &text, const string &header, bool toggle) {
	if(toggle == false) {
		mainMessageBox.setEnabled(false);
	}

	if(mainMessageBox.getEnabled() == false) {
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else {
		mainMessageBox.setEnabled(false);
	}
}

}}//end namespace
