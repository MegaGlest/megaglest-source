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

#include "sound_renderer.h"

#include "core_data.h"
#include "config.h"
#include "sound_interface.h"
#include "factory_repository.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Sound;

namespace Glest{ namespace Game{

const int SoundRenderer::ambientFade= 6000;
const float SoundRenderer::audibleDist= 50.f;

// =====================================================
// 	class SoundRenderer
// =====================================================

SoundRenderer::SoundRenderer(){
	loadConfig();
}

void SoundRenderer::init(Window *window){
	SoundInterface &si= SoundInterface::getInstance();
	FactoryRepository &fr= FactoryRepository::getInstance();
	Config &config= Config::getInstance();

	si.setFactory(fr.getSoundFactory(config.getString("FactorySound")));
	soundPlayer= si.newSoundPlayer();

	SoundPlayerParams soundPlayerParams;
	soundPlayerParams.staticBufferCount= config.getInt("SoundStaticBuffers");
	soundPlayerParams.strBufferCount= config.getInt("SoundStreamingBuffers");
	soundPlayer->init(&soundPlayerParams);
}

SoundRenderer::~SoundRenderer(){
	delete soundPlayer;
}

SoundRenderer &SoundRenderer::getInstance(){
	static SoundRenderer soundRenderer;
	return soundRenderer;
}

void SoundRenderer::update(){
	soundPlayer->updateStreams();
}

// ======================= Music ============================

void SoundRenderer::playMusic(StrSound *strSound){
	strSound->setVolume(musicVolume);
	strSound->restart();
	soundPlayer->play(strSound);
}

void SoundRenderer::stopMusic(StrSound *strSound){
	soundPlayer->stop(strSound);
}

// ======================= Fx ============================

void SoundRenderer::playFx(StaticSound *staticSound, Vec3f soundPos, Vec3f camPos){
	if(staticSound!=NULL){
		float d= soundPos.dist(camPos);

		if(d<audibleDist){	
			float vol= (1.f-d/audibleDist)*fxVolume;
			float correctedVol= log10(log10(vol*9+1)*9+1);
			staticSound->setVolume(correctedVol);
			soundPlayer->play(staticSound);
		}
	}
}

void SoundRenderer::playFx(StaticSound *staticSound){
	if(staticSound!=NULL){
		staticSound->setVolume(fxVolume);
		soundPlayer->play(staticSound);
	}
}

// ======================= Ambient ============================

void SoundRenderer::playAmbient(StrSound *strSound){
	strSound->setVolume(ambientVolume);
	soundPlayer->play(strSound, ambientFade);
}

void SoundRenderer::stopAmbient(StrSound *strSound){
	soundPlayer->stop(strSound, ambientFade);
}

// ======================= Misc ============================

void SoundRenderer::stopAllSounds(){
	soundPlayer->stopAllSounds();
}

void SoundRenderer::loadConfig(){
	Config &config= Config::getInstance();

	fxVolume= config.getInt("SoundVolumeFx")/100.f;
	musicVolume= config.getInt("SoundVolumeMusic")/100.f;
	ambientVolume= config.getInt("SoundVolumeAmbient")/100.f;
}

}}//end namespace
