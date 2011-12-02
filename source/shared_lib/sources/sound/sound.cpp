// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "sound.h"

#include <fstream>
#include <stdexcept>
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
namespace Shared { namespace Sound {

//bool Sound::masterserverMode = false;

// =====================================================
//	class SoundInfo
// =====================================================

SoundInfo::SoundInfo() {
	channels= 0;
	samplesPerSecond= 0;
	bitsPerSample= 0;
	size= 0;
	bitRate=0;
}

// =====================================================
//	class Sound
// =====================================================

Sound::Sound() {
	volume= 0.0f;
	fileName = "";
}

// =====================================================
//	class StaticSound
// =====================================================

StaticSound::StaticSound() {
	samples= NULL;
	soundFileLoader = NULL;
	fileName = "";
}

StaticSound::~StaticSound() {
	close();
}

void StaticSound::close() {
	if(samples != NULL) {
		delete [] samples;
		samples = NULL;
	}

	if(soundFileLoader!=NULL){
		soundFileLoader->close();
		delete soundFileLoader;
		soundFileLoader= NULL;
	}
}

void StaticSound::load(const string &path) {
	close();

	fileName = path;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
	string ext= path.substr(path.find_last_of('.')+1);
	soundFileLoader= SoundFileLoaderFactory::getInstance()->newInstance(ext);

	soundFileLoader->open(path, &info);
	samples= new int8[info.getSize()];
	soundFileLoader->read(samples, info.getSize());
	soundFileLoader->close();

	if(soundFileLoader!=NULL){
		soundFileLoader->close();
		delete soundFileLoader;
		soundFileLoader= NULL;
	}
}

// =====================================================
//	class StrSound
// =====================================================

StrSound::StrSound() {
	soundFileLoader= NULL;
	next= NULL;
	fileName = "";
}

StrSound::~StrSound() {
	close();
}

void StrSound::open(const string &path) {
	close();

	fileName = path;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	string ext= path.substr(path.find_last_of('.')+1);

	soundFileLoader= SoundFileLoaderFactory::getInstance()->newInstance(ext);
	soundFileLoader->open(path, &info);
}

uint32 StrSound::read(int8 *samples, uint32 size){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return 0;
	}

	return soundFileLoader->read(samples, size);
}

void StrSound::close(){
	if(soundFileLoader != NULL) {
		soundFileLoader->close();
		delete soundFileLoader;
		soundFileLoader= NULL;
	}
}

void StrSound::restart(){
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}

	soundFileLoader->restart();
}

}}//end namespace
