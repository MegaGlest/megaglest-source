//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.
#include "sound_player_openal.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "platform_util.h"
#include "util.h"
#include "conversion.h"
#include "leak_dumper.h"

namespace Shared{ namespace Sound{ namespace OpenAL{

using namespace Util;

SoundSource::SoundSource()
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	alGenSources(1, &source);
	SoundPlayerOpenAL::checkAlError("Couldn't create audio source: ");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

SoundSource::~SoundSource()
{
	stop();
	alDeleteSources(1, &source);
}

bool SoundSource::playing()
{
	ALint state = AL_STOPPED;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	return(state == AL_PLAYING || state == AL_PAUSED);
}

void SoundSource::unQueueBuffers() {
	SoundPlayerOpenAL::checkAlError("Problem unqueuing buffers START OF METHOD: ");

    ALint queued=0;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

    alSourcei(source, AL_LOOPING, AL_FALSE);
    SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

	ALint processed=0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

	while(processed--) {
		ALuint buffer=0;
		alSourceUnqueueBuffers(source, 1, &buffer);
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
	}

//	ALint queued=0;
//	alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
//	while(queued--) {
//		ALuint buffer=0;
//		alSourceUnqueueBuffers(source, 1, &buffer);
//		SoundPlayerOpenAL::checkAlError("Problem unqueuing buffers (alSourceUnqueueBuffers) in audio source: ");
//	}
//
//	SoundPlayerOpenAL::checkAlError("Problem unqueuing buffers (alGetSourcei AL_BUFFERS_QUEUED) in audio source: ");

}

void SoundSource::stop() {
	alSourceStop(source);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

    unQueueBuffers();

	alSourcei(source, AL_BUFFER, AL_NONE);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

	//unQueueBuffers();

	alSourceRewind(source);    // stops the source
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

	while(playing() == true) {
		//alSourceStop(source);
		//SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
		ALint state = AL_STOPPED;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

		sleep(1);
		printf("$$$ WAITING FOR OPENAL TO STOP state = %d!\n",state);
	}
	//unQueueBuffers();
	//SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
}

ALenum SoundSource::getFormat(Sound* sound)
{
	if(sound->getInfo()->getChannels() == 2) {
		if(sound->getInfo()->getBitsPerSample() == 16) {
			return AL_FORMAT_STEREO16;
		}
		else if(sound->getInfo()->getBitsPerSample() == 8) {
			return AL_FORMAT_STEREO8;
		}
		else {
			throw std::runtime_error("[1] Sample format not supported in file: " + sound->getFileName());
		}
	}
	else if(sound->getInfo()->getChannels() == 1) {
		if(sound->getInfo()->getBitsPerSample() == 16) {
			return AL_FORMAT_MONO16;
		}
		else if(sound->getInfo()->getBitsPerSample() == 8) {
			return AL_FORMAT_MONO8;
		}
		else {
			throw std::runtime_error("[2] Sample format not supported in file: " + sound->getFileName());
		}
	}

	throw std::runtime_error("[3] Sample format not supported in file: " + sound->getFileName());
}

//---------------------------------------------------------------------------

StaticSoundSource::StaticSoundSource() {
	bufferAllocated = false;
	buffer = 0;
}

StaticSoundSource::~StaticSoundSource() {
	if(bufferAllocated) {
		stop();
		alDeleteBuffers(1, &buffer);
	}
}

void StaticSoundSource::play(StaticSound* sound) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(bufferAllocated) {
		stop();
		alDeleteBuffers(1, &buffer);
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
	}
	ALenum format = getFormat(sound);

	alGenBuffers(1, &buffer);
	SoundPlayerOpenAL::checkAlError("Couldn't create audio buffer: ");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d] filename [%s] format = %d, sound->getSamples() = %d, sound->getInfo()->getSize() = %d, sound->getInfo()->getSamplesPerSecond() = %d\n",__FILE__,__FUNCTION__,__LINE__,sound->getFileName().c_str(),format,sound->getSamples(),sound->getInfo()->getSize(),sound->getInfo()->getSamplesPerSecond());

	bufferAllocated = true;
	alBufferData(buffer, format, sound->getSamples(),
			static_cast<ALsizei> (sound->getInfo()->getSize()),
			static_cast<ALsizei> (sound->getInfo()->getSamplesPerSecond()));

	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

	alSourcei(source, AL_BUFFER, buffer);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
	alSourcef(source, AL_GAIN, sound->getVolume());
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

	alSourcePlay(source);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
}

StreamSoundSource::StreamSoundSource()
{
	sound = 0;
	fadeState = NoFading;
	format = 0;
	alGenBuffers(STREAMFRAGMENTS, buffers);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
}

StreamSoundSource::~StreamSoundSource()
{
	stop();
	alDeleteBuffers(STREAMFRAGMENTS, buffers);
	SoundPlayerOpenAL::checkAlError("Couldn't delete audio buffers: ");
}

void StreamSoundSource::stop()
{
	sound = 0;
	SoundSource::stop();
}

void StreamSoundSource::stop(int64 fadeoff)
{
	if(fadeoff > 0) {
		fadeState = FadingOff;
		fade = fadeoff;
		chrono.start();
	} else {
		stop();
	}
}

void StreamSoundSource::play(StrSound* sound, int64 fadeon)
{
	stop();

	this->sound = sound;

	format = getFormat(sound);
   	for(size_t i = 0; i < STREAMFRAGMENTS; ++i) {
		fillBufferAndQueue(buffers[i]);
	}
	if(fadeon > 0) {
		alSourcef(source, AL_GAIN, 0);
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

		fadeState = FadingOn;
		fade = fadeon;
		chrono.start();
	} else {
		fadeState = NoFading;
		alSourcef(source, AL_GAIN, sound->getVolume());
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
	}
	alSourcePlay(source);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
}

void StreamSoundSource::update()
{
	if(sound == 0) {
		return;
	}

	if(fadeState == NoFading){
		alSourcef(source, AL_GAIN, sound->getVolume());
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
	}

	ALint processed = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
	SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
	while(processed > 0) {
		processed--;

		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer);
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

		if(!fillBufferAndQueue(buffer))
			break;
	}

 	// we might have to restart the source if we had a buffer underrun
	if(!playing()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d] Restarting audio source because of buffer underrun.\n",__FILE__,__FUNCTION__,__LINE__);

		std::cerr << "Restarting audio source because of buffer underrun.\n";
		alSourcePlay(source);
		SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
  	}

	// handle fading
	switch(fadeState) {
		case FadingOn:
			if(chrono.getMillis() > fade) {
				alSourcef(source, AL_GAIN, sound->getVolume());
				SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

				fadeState = NoFading;
			} else {
				alSourcef(source, AL_GAIN, sound->getVolume() *
						static_cast<float> (chrono.getMillis())/fade);
				SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
			}
			break;
		case FadingOff:
			if(chrono.getMillis() > fade) {
				stop();
			} else {
				alSourcef(source, AL_GAIN, sound->getVolume() *
						(1.0f - static_cast<float>(chrono.getMillis())/fade));
				SoundPlayerOpenAL::checkAlError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));
			}
			break;
		default:
			break;
	}
}

bool StreamSoundSource::fillBufferAndQueue(ALuint buffer)
{
	// fill buffer
	int8* bufferdata = new int8[STREAMFRAGMENTSIZE];
	uint32 bytesread = 0;
	do {
		bytesread += sound->read(bufferdata + bytesread,STREAMFRAGMENTSIZE - bytesread);
		if(bytesread < STREAMFRAGMENTSIZE) {
			StrSound* next = sound->getNext();
			if(next == 0)
				next = sound;
			next->restart();
			next->setVolume(sound->getVolume());
			sound = next;
		}
	} while(bytesread < STREAMFRAGMENTSIZE);

	alBufferData(buffer, format, bufferdata, STREAMFRAGMENTSIZE,
			sound->getInfo()->getSamplesPerSecond());
        delete[] bufferdata;
	SoundPlayerOpenAL::checkAlError("Couldn't refill audio buffer: ");

	alSourceQueueBuffers(source, 1, &buffer);
	SoundPlayerOpenAL::checkAlError("Couldn't queue audio buffer: ");

	return true;
}

// ================================
//        Sound Player OpenAL
// ================================

SoundPlayerOpenAL::SoundPlayerOpenAL() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	device = 0;
	context = 0;
	initOk = false;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

SoundPlayerOpenAL::~SoundPlayerOpenAL() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	end();
	initOk = false;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void SoundPlayerOpenAL::printOpenALInfo()
{
	printf("OpenAL Vendor: %s\n",alGetString(AL_VENDOR));
	printf("OpenAL Version: %s\n",alGetString(AL_VERSION));
	printf("OpenAL Renderer: %s\n",alGetString(AL_RENDERER));
	printf("OpenAL Extensions: %s\n",alGetString(AL_RENDERER));

/*	
	std::cout << "OpenAL Vendor: " << alGetString(AL_VENDOR) << "\n"
	          << "OpenAL Version: " << alGetString(AL_VERSION) << "\n"
	          << "OpenAL Renderer: " << alGetString(AL_RENDERER) << "\n"
	          << "OpenAl Extensions: " << alGetString(AL_RENDERER) << "\n";
*/
}

bool SoundPlayerOpenAL::init(const SoundPlayerParams* params) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    initOk = false;

    if(params == NULL) {
    	throw std::runtime_error("params == NULL");
    }

	this->params = *params;

	try {
		// Allows platforms to specify which sound device to use
		// using the environment variable: MEGAGLEST_SOUND_DEVICE
		char *deviceName = getenv("MEGAGLEST_SOUND_DEVICE");
		device = alcOpenDevice(deviceName);
		if(device == 0) {
			//printOpenALInfo();
			throw std::runtime_error("Couldn't open audio device.");
		}

		int attributes[] = { 0 };
		context = alcCreateContext(device, attributes);
		checkAlcError("Couldn't create audio context: ");
		alcMakeContextCurrent(context);
		checkAlcError("Couldn't select audio context: ");

		checkAlError("Audio error after init: ");

		initOk = true;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		printOpenALInfo();
	}

	return initOk;
}

void SoundPlayerOpenAL::end() {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);


	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
        StaticSoundSource *src = (*i);
        src->stop();
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
        StreamSoundSource *src = (*i);
        src->stop();
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
		delete *i;
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		delete *i;
    }

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    alcMakeContextCurrent( NULL );
    SoundPlayerOpenAL::checkAlcError(string(__FILE__) + string(" ") + string(__FUNCTION__) + string(" ") + intToStr(__LINE__));

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(context != 0) {
		alcDestroyContext(context);
		context = 0;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(device != 0) {
		alcCloseDevice(device);
		device = 0;
		initOk = false;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void SoundPlayerOpenAL::play(StaticSound* staticSound) {
	assert(staticSound != 0);

	if(initOk == false) return;

	try {
		StaticSoundSource* source = findStaticSoundSource();

		if(source == 0) {
			return;
		}
		source->play(staticSound);
	}
	catch(std::exception& e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		std::cerr << "Couldn't play static sound: [" << e.what() << "]\n";
	}
}

void SoundPlayerOpenAL::play(StrSound* strSound, int64 fadeOn) {
	assert(strSound != 0);

	if(initOk == false) return;

	try {
		StreamSoundSource* source = findStreamSoundSource();
		source->play(strSound, fadeOn);
	}
	catch(std::exception& e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		std::cerr << "Couldn't play streaming sound: " << e.what() << "\n";
	}
}

void SoundPlayerOpenAL::stop(StrSound* strSound, int64 fadeOff) {
	assert(strSound != 0);

	if(initOk == false) return;

	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		StreamSoundSource* source = *i;
		if(source->sound == strSound) {
			source->stop(fadeOff);
		}
	}
}

void SoundPlayerOpenAL::stopAllSounds(int64 fadeOff) {
	if(initOk == false) return;

	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
		StaticSoundSource* source = *i;
		source->stop();
	}
	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		StreamSoundSource* source = *i;
		source->stop(fadeOff);
	}
}

void SoundPlayerOpenAL::updateStreams() {
	if(initOk == false) return;

	assert(context != 0);
	try {
		for(StreamSoundSources::iterator i = streamSources.begin();
				i != streamSources.end(); ++i) {
			StreamSoundSource* source = *i;
			try {
				source->update();
			}
			catch(std::exception& e) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());

				std::cerr << "Error while updating sound stream: "<< e.what() << "\n";
			}
		}
		alcProcessContext(context);
		checkAlcError("Error while processing audio context: ");
	}
	catch(exception &ex) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		printOpenALInfo();
		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
		printOpenALInfo();
		throw;
	}
}

StaticSoundSource* SoundPlayerOpenAL::findStaticSoundSource() {
	if(initOk == false) return NULL;

	// try to find a stopped source
	int playingCount = 0;
	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
		StaticSoundSource* source = *i;
		if(! source->playing()) {
			return source;
		}
		else {
			playingCount++;
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s %d] playingCount = %d, staticSources.size() = %lu, params.staticBufferCount = %u\n",__FILE__,__FUNCTION__,__LINE__,playingCount,(unsigned long)staticSources.size(),params.staticBufferCount);

	// create a new source
	if(staticSources.size() >= params.staticBufferCount) {
		return 0;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	StaticSoundSource* source = new StaticSoundSource();
	staticSources.push_back(source);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return source;
}

StreamSoundSource* SoundPlayerOpenAL::findStreamSoundSource() {
	if(initOk == false) return NULL;

	// try to find a stopped source
	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		StreamSoundSource* source = *i;
		if(! source->playing()) {
			return source;
		}
	}

	// create a new source
	if(streamSources.size() >= params.strBufferCount) {
		throw std::runtime_error("Too many stream sounds played at once");
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	StreamSoundSource* source = new StreamSoundSource();
	streamSources.push_back(source);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return source;
}

void SoundPlayerOpenAL::checkAlcError(string message) {
	int err = alcGetError(device);
	if(err != ALC_NO_ERROR) {
		char szBuf[4096]="";
		sprintf(szBuf,"%s [%s]",message.c_str(),alcGetString(device, err));

		//std::stringstream msg;
		//msg << message.c_str() << alcGetString(device, err);
		printf("openalc error [%s]\n",szBuf);
		throw std::runtime_error(szBuf);
	}
}

void SoundPlayerOpenAL::checkAlError(string message) {
	int err = alGetError();
	if(err != AL_NO_ERROR) {
		char szBuf[4096]="";
		sprintf(szBuf,"%s [%s]",message.c_str(),alGetString(err));
		//std::stringstream msg;
		//msg << message.c_str() << alGetString(err);
		printf("openal error [%s]\n",szBuf);
		throw std::runtime_error(szBuf);
	}
}

}}} // end of namespace

