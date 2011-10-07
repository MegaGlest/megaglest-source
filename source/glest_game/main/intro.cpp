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
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace	Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class Text
// =====================================================

Text::Text(const string &text, const Vec2i &pos, int time, Font2D *font, Font3D *font3D) {
	this->text= text;
	this->pos= pos;
	this->time= time;
	this->texture= NULL;
	this->font= font;
	this->font3D = font3D;
}

Text::Text(const Texture2D *texture, const Vec2i &pos, const Vec2i &size, int time) {
	this->pos= pos;
	this->size= size;
	this->time= time;
	this->texture= texture;
	this->font= NULL;
	this->font3D=NULL;
}

// =====================================================
// 	class Intro
// =====================================================

int Intro::introTime	= 50000;
int Intro::appearTime	= 2500;
int Intro::showTime		= 3500;
int Intro::disapearTime	= 2500;

Intro::Intro(Program *program):
	ProgramState(program)
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();
	int w= metrics.getVirtualW();
	int h= metrics.getVirtualH();
	timer=0;
	mouseX = 0;
	mouseY = 0;
	mouse2d = 0;

	XmlTree xmlTree;
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	xmlTree.load(data_path + "data/core/menu/menu.xml",Properties::getTagReplacementValues());
	const XmlNode *menuNode= xmlTree.getRootNode();
	const XmlNode *introNode= menuNode->getChild("intro");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// intro info
	const XmlNode *introTimeNode= introNode->getChild("intro-time");
	Intro::introTime = introTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *appearTimeNode= introNode->getChild("appear-time");
	Intro::appearTime = appearTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *showTimeNode= introNode->getChild("show-time");
	Intro::showTime = showTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *disappearTimeNode= introNode->getChild("disappear-time");
	Intro::disapearTime = disappearTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *showIntroPicturesNode= introNode->getChild("show-intro-pictures");
	int showIntroPics = showIntroPicturesNode->getAttribute("value")->getIntValue();
	int showIntroPicsTime = showIntroPicturesNode->getAttribute("time")->getIntValue();
	bool showIntroPicsRandom = showIntroPicturesNode->getAttribute("random")->getBoolValue();

	int displayItemNumber = 1;
	int appear= Intro::appearTime;
	int disappear= Intro::showTime+Intro::appearTime+Intro::disapearTime;

	texts.push_back(new Text("based on the award winning game Glest", Vec2i(w/2, h/2), appear, coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));
	texts.push_back(new Text("the MegaGlest team presents...", Vec2i(w/2, h/2), disappear, coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));

	texts.push_back(new Text(coreData.getLogoTexture(), Vec2i(w/2-128, h/2-64), Vec2i(256, 128), disappear *(++displayItemNumber)));
	texts.push_back(new Text(glestVersionString, Vec2i(w/2+45, h/2-45), disappear *(displayItemNumber++), coreData.getMenuFontNormal(),coreData.getMenuFontNormal3D()));
	texts.push_back(new Text("www.megaglest.org", Vec2i(w/2, h/2), disappear *(displayItemNumber++), coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));

	if(showIntroPics > 0 && coreData.getMiscTextureList().size() > 0) {
		const int showMiscTime = showIntroPicsTime;

		std::vector<Texture2D *> intoTexList;
		if(showIntroPicsRandom == true) {
			unsigned int seed = time(NULL);
			srand(seed);
			int failedLookups=0;
			std::map<int,bool> usedIndex;
			for(;intoTexList.size() < showIntroPics;) {
				int picIndex = rand() % coreData.getMiscTextureList().size();
				if(usedIndex.find(picIndex) != usedIndex.end()) {
					failedLookups++;
					seed = time(NULL) / failedLookups;
					srand(seed);
					continue;
				}
				//printf("picIndex = %d list count = %d\n",picIndex,coreData.getMiscTextureList().size());
				intoTexList.push_back(coreData.getMiscTextureList()[picIndex]);
				usedIndex[picIndex]=true;
				seed = time(NULL) / intoTexList.size();
				srand(seed);
			}
		}
		else {
			for(unsigned int i = 0;
					i < coreData.getMiscTextureList().size() &&
					i < showIntroPics; ++i) {
				Texture2D *tex = coreData.getMiscTextureList()[i];
				intoTexList.push_back(tex);
			}
		}

		for(unsigned int i = 0; i < intoTexList.size(); ++i) {
			Texture2D *tex = intoTexList[i];
			//printf("tex # %d [%s]\n",i,tex->getPath().c_str());

			Vec2i texPlacement;
			if(i == 0 || i % 9 == 0) {
				texPlacement = Vec2i(1, h-tex->getTextureHeight());
			}
			else if(i == 1 || i % 9 == 1) {
				texPlacement = Vec2i(1, 1);
			}
			else if(i == 2 || i % 9 == 2) {
				texPlacement = Vec2i(w-tex->getTextureWidth(), 1);
			}
			else if(i == 3 || i % 9 == 3) {
				texPlacement = Vec2i(w-tex->getTextureWidth(), h-tex->getTextureHeight());
			}
			else if(i == 4 || i % 9 == 4) {
				texPlacement = Vec2i(w/2 - tex->getTextureWidth()/2, h-tex->getTextureHeight());
			}
			else if(i == 5 || i % 9 == 5) {
				texPlacement = Vec2i(w/2 - tex->getTextureWidth()/2, 1);
			}
			else if(i == 6 || i % 9 == 6) {
				texPlacement = Vec2i(1, (h/2) - (tex->getTextureHeight()/2));
			}
			else if(i == 7 || i % 9 == 7) {
				texPlacement = Vec2i(w-tex->getTextureWidth(), (h/2) - (tex->getTextureHeight()/2));
			}

			texts.push_back(new Text(tex, texPlacement, Vec2i(tex->getTextureWidth(), tex->getTextureHeight()), disappear *displayItemNumber+(showMiscTime*(i+1))));
		}
	}

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	soundRenderer.playMusic(CoreData::getInstance().getIntroMusic());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

Intro::~Intro() {
	deleteValues(texts.begin(),texts.end());
}

void Intro::update(){
	timer++;
	if(timer > introTime * GameConstants::updateFps / 1000){
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		program->setState(new MainMenu(program));
		return;
	}

	if(Config::getInstance().getBool("AutoTest")){
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		AutoTest::getInstance().updateIntro(program);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	mouse2d= (mouse2d+1) % Renderer::maxMouse2dAnim;
}

void Intro::render() {
	Renderer &renderer= Renderer::getInstance();
	if(renderer.isMasterserverMode() == true) {
		return;
	}
	int difTime=0;

	canRender();
	incrementFps();

	renderer.reset2d();
	renderer.clearBuffers();

//	CoreData &coreData= CoreData::getInstance();
//	renderer.renderTextureQuad(
//		1, 1,
//		1, 1,
//		coreData.getLogoTexture(), 1.0);

//	renderer.renderFPSWhenEnabled(lastFps);

//	renderer.renderText3D(
//		"test 123", coreData.getMenuFontVeryBig3D(), 1.0,
//		1, 1, true);

	for(int i = 0; i < texts.size(); ++i) {
		Text *text= texts[i];

		difTime= 1000 * timer / GameConstants::updateFps - text->getTime();

		if(difTime > 0 && difTime < appearTime + showTime + disapearTime) {
			float alpha= 1.f;
			if(difTime > 0 && difTime < appearTime) {
				//apearing
				alpha= static_cast<float>(difTime) / appearTime;
			}
			else if(difTime > 0 && difTime < appearTime + showTime + disapearTime) {
				//disappearing
				alpha= 1.f- static_cast<float>(difTime - appearTime - showTime) / disapearTime;
			}

			if(text->getText().empty() == false) {
				if(Renderer::renderText3DEnabled) {
					renderer.renderText3D(
						text->getText(), text->getFont3D(), alpha,
						text->getPos().x, text->getPos().y, true);
				}
				else {
					renderer.renderText(
						text->getText(), text->getFont(), alpha,
						text->getPos().x, text->getPos().y, true);
				}
			}

			if(text->getTexture()!=NULL){
				renderer.renderTextureQuad(
					text->getPos().x, text->getPos().y,
					text->getSize().x, text->getSize().y,
					text->getTexture(), alpha);
			}
		}
	}

	if(program != NULL) program->renderProgramMsgBox();

	if(this->forceMouseRender == true) renderer.renderMouse2d(mouseX, mouseY, mouse2d, 0.f);

	renderer.renderFPSWhenEnabled(lastFps);

	renderer.swapBuffers();
}

void Intro::keyDown(char key){
	mouseUpLeft(0, 0);
}

void Intro::mouseUpLeft(int x, int y){
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	soundRenderer.stopMusic(CoreData::getInstance().getIntroMusic());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	program->setState(new MainMenu(program));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Intro::mouseMove(int x, int y, const MouseState *ms) {
	mouseX = x;
	mouseY = y;
}

}}//end namespace
