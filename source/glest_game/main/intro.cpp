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
#include "intro.h"	

#include "main_menu.h"
#include "util.h"
#include "game_util.h"
#include "config.h"
#include "program.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "auto_test.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest{ namespace Game{ 

// =====================================================
// 	class Text  
// =====================================================

Text::Text(const string &text, const Vec2i &pos, int time, const Font2D *font){
	this->text= text;
	this->pos= pos;
	this->time= time;
	this->texture= NULL;	
	this->font= font;
}

Text::Text(const Texture2D *texture, const Vec2i &pos, const Vec2i &size, int time){
	this->pos= pos;
	this->size= size;
	this->time= time;
	this->texture= texture;	
	this->font= NULL;
}

// =====================================================
// 	class Intro  
// =====================================================

const int Intro::introTime= 24000;
const int Intro::appearTime= 2500;
const int Intro::showTime= 2500;
const int Intro::disapearTime= 2500;

Intro::Intro(Program *program):
	ProgramState(program)
{
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();
	int w= metrics.getVirtualW(); 
	int h= metrics.getVirtualH(); 
	timer=0;
	
	texts.push_back(Text(coreData.getLogoTexture(), Vec2i(w/2-128, h/2-64), Vec2i(256, 128), 4000));
	texts.push_back(Text(glestVersionString, Vec2i(w/2+64, h/2-32), 4000, coreData.getMenuFontNormal()));
	texts.push_back(Text("www.glest.org", Vec2i(w/2, h/2), 12000, coreData.getMenuFontVeryBig()));
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.playMusic(CoreData::getInstance().getIntroMusic());
}

void Intro::update(){
	timer++;
	if(timer>introTime*GameConstants::updateFps/1000){
		program->setState(new MainMenu(program));
	}

	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateIntro(program);
	}
}

void Intro::render(){
	Renderer &renderer= Renderer::getInstance();
	int difTime;
	
	renderer.reset2d();
	renderer.clearBuffers();
	for(int i=0; i<texts.size(); ++i){
		Text *text= &texts[i];
		
		difTime= 1000*timer/GameConstants::updateFps-text->getTime();
		
		if(difTime>0 && difTime<appearTime+showTime+disapearTime){
			float alpha= 1.f;
			if(difTime>0 && difTime<appearTime){
				//apearing
				alpha= static_cast<float>(difTime)/appearTime;
			}
			else if(difTime>0 && difTime<appearTime+showTime+disapearTime){
				//disappearing
				alpha= 1.f- static_cast<float>(difTime-appearTime-showTime)/disapearTime;
			}
			if(!text->getText().empty()){
				renderer.renderText(
					text->getText(), text->getFont(), alpha, 
					text->getPos().x, text->getPos().y, true);
			}
			if(text->getTexture()!=NULL){
				renderer.renderTextureQuad(
					text->getPos().x, text->getPos().y, 
					text->getSize().x, text->getSize().y, 
					text->getTexture(), alpha);
			}
		}
	}
	renderer.swapBuffers();
}

void Intro::keyDown(char key){
	mouseUpLeft(0, 0);
}

void Intro::mouseUpLeft(int x, int y){
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.stopMusic(CoreData::getInstance().getIntroMusic());
	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());
	program->setState(new MainMenu(program));
}

}}//end namespace
