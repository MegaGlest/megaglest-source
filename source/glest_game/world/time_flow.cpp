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

#include "time_flow.h"

#include "sound_renderer.h"
#include "config.h"
#include "game_constants.h"
#include "util.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class TimeFlow
// =====================================================

const float TimeFlow::dusk= 18.f;
const float TimeFlow::dawn= 6.f;

TimeFlow::TimeFlow() {
	firstTime = false;
	tileset = NULL;
	time = 0.0f;
	lastTime = 0.0f;
	timeInc = 0.0f;
	//printf("#1a timeInc = %f\n",timeInc);
}

void TimeFlow::init(Tileset *tileset){
	firstTime= true;
	this->tileset= tileset;
	time= dawn+1.5f;
	lastTime= time;
	Config &config= Config::getInstance();
	timeInc= 24.f * (1.f / config.getFloat("DayTime")) / GameConstants::updateFps;
	//printf("#1 timeInc = %f\n",timeInc);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] timeInc = %f\n",__FILE__,__FUNCTION__,__LINE__,timeInc);
}

void TimeFlow::update() {
	//printf("START TimeFlow::update() time = %f\n",time);
	//update time
	time += isDay()? timeInc: timeInc*2;
	if(time > 24.f){
		time -= 24.f;
	}

	//printf("END TimeFlow::update() time = %f\n",time);

	//sounds
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	AmbientSounds *ambientSounds= NULL;
	if(tileset != NULL) {
		ambientSounds = tileset->getAmbientSounds();
	}

	//day
	if(lastTime<dawn && time>=dawn){
		if(ambientSounds != NULL) {
			soundRenderer.stopAmbient(ambientSounds->getNight());
		}
		UnitParticleSystem::isNight=false;
	}
	UnitParticleSystem::lightColor=computeLightColor();

	if((lastTime<dawn && time>=dawn) || firstTime){
		
		//day sound
		if(ambientSounds != NULL) {
			if(ambientSounds->isEnabledDayStart() && !firstTime){
				soundRenderer.playFx(ambientSounds->getDayStart());
			}
			if(ambientSounds->isEnabledDay()){
				if(ambientSounds->getAlwaysPlayDay() || tileset->getWeather()==wSunny){
					soundRenderer.playAmbient(ambientSounds->getDay());
				}
			}
		}
		firstTime= false;
	}

	//night
	if(lastTime<dusk && time>=dusk){
		if(ambientSounds != NULL) {
			soundRenderer.stopAmbient(ambientSounds->getDay());
		}
		UnitParticleSystem::isNight=true;
	}

	if(lastTime<dusk && time>=dusk){		
		//night
		if(ambientSounds != NULL) {
			if(ambientSounds->isEnabledNightStart()){
				soundRenderer.playFx(ambientSounds->getNightStart());
			}
			if(ambientSounds->isEnabledNight()){
				if(ambientSounds->getAlwaysPlayNight() || tileset->getWeather()==wSunny){
					soundRenderer.playAmbient(ambientSounds->getNight());
				}
			}
		}
	}
	lastTime= time;
}

//bool TimeFlow::isAproxTime(float time) const {
//	return (this->time>=time) && (this->time<time+timeInc);
//}

Vec3f TimeFlow::computeLightColor() const {
	Vec3f color;

	if(tileset != NULL) {
		float time=getTime();

		const float transition= 2;
		const float dayStart= TimeFlow::dawn;
		const float dayEnd= TimeFlow::dusk-transition;
		const float nightStart= TimeFlow::dusk;
		const float nightEnd= TimeFlow::dawn-transition;

		if(time>dayStart && time<dayEnd) {
			color= tileset->getSunLightColor();
		}
		else if(time>nightStart || time<nightEnd) {
			color= tileset->getMoonLightColor();
		}
		else if(time>=dayEnd && time<=nightStart) {
			color= tileset->getSunLightColor().lerp((time-dayEnd)/transition, tileset->getMoonLightColor());
		}
		else if(time>=nightEnd && time<=dayStart) {
			color= tileset->getMoonLightColor().lerp((time-nightEnd)/transition, tileset->getSunLightColor());
		}
		else {
			assert(false);
			color= tileset->getSunLightColor();
		}
	}
	return color;
}

void TimeFlow::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *timeflowNode = rootNode->addChild("TimeFlow");

	timeflowNode->addAttribute("firstTime",intToStr(firstTime), mapTagReplacements);
//	bool firstTime;
//	Tileset *tileset;
//	float time;
	timeflowNode->addAttribute("time",floatToStr(time,16), mapTagReplacements);
//	float lastTime;
	timeflowNode->addAttribute("lastTime",floatToStr(lastTime,16), mapTagReplacements);
//	float timeInc;
	//printf("#2 timeInc = %f\n",timeInc);
	timeflowNode->addAttribute("timeInc",floatToStr(timeInc,16), mapTagReplacements);
	//printf("#3 timeInc = %f\n",timeInc);
}

void TimeFlow::loadGame(const XmlNode *rootNode) {
	const XmlNode *timeflowNode = rootNode->getChild("TimeFlow");

	firstTime = timeflowNode->getAttribute("firstTime")->getFloatValue() != 0;
	time = timeflowNode->getAttribute("time")->getFloatValue();
	lastTime = timeflowNode->getAttribute("lastTime")->getFloatValue();
	timeInc = timeflowNode->getAttribute("timeInc")->getFloatValue();
	//printf("#4 timeInc = %f\n",timeInc);
}

}}//end namespace
