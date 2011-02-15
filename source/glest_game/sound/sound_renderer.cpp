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

#include "sound_renderer.h"

#include "core_data.h"
#include "config.h"
#include "sound_interface.h"
#include "factory_repository.h"
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Sound;

namespace Glest{ namespace Game{

const int SoundRenderer::ambientFade= 6000;
const float SoundRenderer::audibleDist= 50.f;

// =====================================================
// 	class SoundRenderer
// =====================================================

SoundRenderer::SoundRenderer() {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    soundPlayer = NULL;
	loadConfig();

	Config &config= Config::getInstance();
	runThreadSafe = config.getBool("ThreadedSoundStream","true");

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] runThreadSafe = %d\n",__FILE__,__FUNCTION__,__LINE__,runThreadSafe);
}

bool SoundRenderer::init(Window *window) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	SoundInterface &si= SoundInterface::getInstance();
	FactoryRepository &fr= FactoryRepository::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	Config &config= Config::getInstance();
	si.setFactory(fr.getSoundFactory(config.getString("FactorySound")));

	stopAllSounds();

    MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
	if(runThreadSafe == true) {
	    safeMutex.setMutex(&mutex);
	}

	soundPlayer= si.newSoundPlayer();
	if(soundPlayer != NULL) {
		SoundPlayerParams soundPlayerParams;
		soundPlayerParams.staticBufferCount= config.getInt("SoundStaticBuffers");
		soundPlayerParams.strBufferCount= config.getInt("SoundStreamingBuffers");
		soundPlayer->init(&soundPlayerParams);
	}
	safeMutex.ReleaseLock();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return wasInitOk();
}

bool SoundRenderer::wasInitOk() const {
	bool result = false;
	if(soundPlayer != NULL) {
		result = soundPlayer->wasInitOk();
	}
	else {
		Config &config= Config::getInstance();
		if(config.getString("FactorySound") == "" ||
			config.getString("FactorySound") == "None") {
			result = true;
		}
	}
	return result;
}

SoundRenderer::~SoundRenderer() {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    stopAllSounds();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
	if(runThreadSafe == true) {
	    safeMutex.setMutex(&mutex);
	}
	delete soundPlayer;
	soundPlayer = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

SoundRenderer &SoundRenderer::getInstance() {
	static SoundRenderer soundRenderer;
	return soundRenderer;
}

void SoundRenderer::update() {
    if(soundPlayer != NULL) {
        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
    	if(runThreadSafe == true) {
    	    safeMutex.setMutex(&mutex);
    	}
        soundPlayer->updateStreams();
    }
}

// ======================= Music ============================

void SoundRenderer::playMusic(StrSound *strSound) {
	if(strSound != NULL) {
		strSound->setVolume(musicVolume);
		strSound->restart();
		if(soundPlayer != NULL) {
	        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
            if(runThreadSafe == true) {
                safeMutex.setMutex(&mutex);
            }

			soundPlayer->play(strSound);
		}
	}
}

void SoundRenderer::setMusicVolume(StrSound *strSound) {
	if(strSound != NULL) {
		strSound->setVolume(musicVolume);
	}
}

void SoundRenderer::stopMusic(StrSound *strSound) {
    if(soundPlayer != NULL) {
        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
    	if(runThreadSafe == true) {
    	    safeMutex.setMutex(&mutex);
    	}

        soundPlayer->stop(strSound);
        if(strSound != NULL) {
			if(strSound->getNext() != NULL) {
				soundPlayer->stop(strSound->getNext());
			}
        }
    }
}

// ======================= Fx ============================

void SoundRenderer::playFx(StaticSound *staticSound, Vec3f soundPos, Vec3f camPos) {
	if(staticSound!=NULL){
		float d= soundPos.dist(camPos);

		if(d < audibleDist){
			float vol= (1.f-d/audibleDist)*fxVolume;
#ifdef USE_STREFLOP
			float correctedVol= streflop::log10(streflop::log10(vol*9+1)*9+1);
#else
			float correctedVol= log10(log10(vol*9+1)*9+1);
#endif

			staticSound->setVolume(correctedVol);

			if(soundPlayer != NULL) {
		        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
                if(runThreadSafe == true) {
                    safeMutex.setMutex(&mutex);
                }

                soundPlayer->play(staticSound);
			}
		}
	}
}

void SoundRenderer::playFx(StaticSound *staticSound) {
	if(staticSound!=NULL){
		staticSound->setVolume(fxVolume);
		if(soundPlayer != NULL) {
	        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
            if(runThreadSafe == true) {
                safeMutex.setMutex(&mutex);
            }

            soundPlayer->play(staticSound);
		}
	}
}

// ======================= Ambient ============================

void SoundRenderer::playAmbient(StrSound *strSound) {
	if(strSound != NULL) {
		strSound->setVolume(ambientVolume);
		if(soundPlayer != NULL) {
	        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
            if(runThreadSafe == true) {
                safeMutex.setMutex(&mutex);
            }

			soundPlayer->play(strSound, ambientFade);
		}
	}
}

void SoundRenderer::stopAmbient(StrSound *strSound) {
    if(soundPlayer != NULL) {
        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
    	if(runThreadSafe == true) {
    	    safeMutex.setMutex(&mutex);
    	}

        soundPlayer->stop(strSound, ambientFade);
    }
}

// ======================= Misc ============================

void SoundRenderer::stopAllSounds() {
    if(soundPlayer != NULL) {
        MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
    	if(runThreadSafe == true) {
    	    safeMutex.setMutex(&mutex);
    	}

        soundPlayer->stopAllSounds();
    }
}

void SoundRenderer::loadConfig() {
	Config &config= Config::getInstance();

	fxVolume= config.getInt("SoundVolumeFx")/100.f;
	musicVolume= config.getInt("SoundVolumeMusic")/100.f;
	ambientVolume= config.getInt("SoundVolumeAmbient")/100.f;
}

}}//end namespace
