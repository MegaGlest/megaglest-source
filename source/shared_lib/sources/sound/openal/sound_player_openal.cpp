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
#include "leak_dumper.h"

namespace Shared{ namespace Sound{ namespace OpenAL{

using namespace Util;

SoundSource::SoundSource()
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	alGenSources(1, &source);
	SoundPlayerOpenAL::checkAlError("Couldn't create audio source: ");

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

SoundSource::~SoundSource()
{
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stop();
	alDeleteSources(1, &source);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool SoundSource::playing()
{
	ALint state = AL_PLAYING;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	return state != AL_STOPPED;
}

void SoundSource::unQueueBuffers() {
    int queued;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    while(queued--) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
    }

}

void SoundSource::stop()
{
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	alSourceStop(source);

    SoundPlayerOpenAL::checkAlError("Problem stopping audio source: ");

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    unQueueBuffers();

    SoundPlayerOpenAL::checkAlError("Problem unqueuing buffers in audio source: ");

	alSourcei(source, AL_BUFFER, AL_NONE);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	SoundPlayerOpenAL::checkAlError("Problem stopping audio source: ");

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

ALenum SoundSource::getFormat(Sound* sound)
{
	if(sound->getInfo()->getChannels() == 2) {
		if(sound->getInfo()->getBitsPerSample() == 16)
			return AL_FORMAT_STEREO16;
		else if(sound->getInfo()->getBitsPerSample() == 8)
			return AL_FORMAT_STEREO8;
		else
			throw std::runtime_error("[1] Sample format not supported in file: " + sound->getFileName());
	} else if(sound->getInfo()->getChannels() == 1) {
		if(sound->getInfo()->getBitsPerSample() == 16)
			return AL_FORMAT_MONO16;
		else if(sound->getInfo()->getBitsPerSample() == 8)
			return AL_FORMAT_MONO8;
		else
			throw std::runtime_error("[2] Sample format not supported in file: " + sound->getFileName());
	}

	throw std::runtime_error("[3] Sample format not supported in file: " + sound->getFileName());
}

//---------------------------------------------------------------------------

StaticSoundSource::StaticSoundSource() {
	bufferAllocated = false;
}

StaticSoundSource::~StaticSoundSource() {
	if(bufferAllocated) {
		stop();
		alDeleteBuffers(1, &buffer);
	}
}

void StaticSoundSource::play(StaticSound* sound)
{
	if(bufferAllocated) {
		stop();
		alDeleteBuffers(1, &buffer);
	}
	ALenum format = getFormat(sound);

	alGenBuffers(1, &buffer);
	SoundPlayerOpenAL::checkAlError("Couldn't create audio buffer: ");

	bufferAllocated = true;
	alBufferData(buffer, format, sound->getSamples(),
			static_cast<ALsizei> (sound->getInfo()->getSize()),
			static_cast<ALsizei> (sound->getInfo()->getSamplesPerSecond()));
	SoundPlayerOpenAL::checkAlError("Couldn't fill audio buffer: ");

	alSourcei(source, AL_BUFFER, buffer);
	alSourcef(source, AL_GAIN, sound->getVolume());
	alSourcePlay(source);
	SoundPlayerOpenAL::checkAlError("Couldn't start audio source: ");
}

StreamSoundSource::StreamSoundSource()
{
	sound = 0;
	alGenBuffers(STREAMFRAGMENTS, buffers);
	SoundPlayerOpenAL::checkAlError("Couldn't allocate audio buffers: ");
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
		fadeState = FadingOn;
		fade = fadeon;
		chrono.start();
	} else {
		fadeState = NoFading;
		alSourcef(source, AL_GAIN, sound->getVolume());
	}
	alSourcePlay(source);
}

void StreamSoundSource::update()
{
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(sound == 0) {
        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		return;
	}

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(fadeState == NoFading){
		alSourcef(source, AL_GAIN, sound->getVolume());
	}
	
	ALint processed = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
	while(processed > 0) {
		processed--;

		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer);
		SoundPlayerOpenAL::checkAlError("Couldn't unqueue audio buffer: ");

		if(!fillBufferAndQueue(buffer))
			break;
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

 	// we might have to restart the source if we had a buffer underrun
	if(!playing()) {
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Restarting audio source because of buffer underrun.\n",__FILE__,__FUNCTION__,__LINE__);

		std::cerr << "Restarting audio source because of buffer underrun.\n";
		alSourcePlay(source);
		SoundPlayerOpenAL::checkAlError("Couldn't restart audio source: ");
  	}

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// handle fading
	switch(fadeState) {
		case FadingOn:
			if(chrono.getMillis() > fade) {
				alSourcef(source, AL_GAIN, sound->getVolume());
				fadeState = NoFading;
			} else {
				alSourcef(source, AL_GAIN, sound->getVolume() *
						static_cast<float> (chrono.getMillis())/fade);
			}
			break;
		case FadingOff:
			if(chrono.getMillis() > fade) {
				stop();
			} else {
				alSourcef(source, AL_GAIN, sound->getVolume() *
						(1.0f - static_cast<float>(chrono.getMillis())/fade));
			}
			break;
		default:
			break;
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	device = 0;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

SoundPlayerOpenAL::~SoundPlayerOpenAL() {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	end();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void SoundPlayerOpenAL::printOpenALInfo()
{
	std::cout << "OpenAL Vendor: " << alGetString(AL_VENDOR) << "\n"
	          << "OpenAL Version: " << alGetString(AL_VERSION) << "\n"
	          << "OpenAL Renderer: " << alGetString(AL_RENDERER) << "\n"
	          << "OpenAl Extensions: " << alGetString(AL_RENDERER) << "\n";
}

void SoundPlayerOpenAL::init(const SoundPlayerParams* params) {

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->params = *params;

	device = alcOpenDevice(0);
	if(device == 0) {
		printOpenALInfo();
		throw std::runtime_error("Couldn't open audio device.");
	}
	try {
		int attributes[] = { 0 };
		context = alcCreateContext(device, attributes);
		checkAlcError("Couldn't create audio context: ");
		alcMakeContextCurrent(context);
		checkAlcError("Couldn't select audio context: ");

		checkAlError("Audio error after init: ");
	} catch(...) {

	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		printOpenALInfo();
		throw;
	}
}

void SoundPlayerOpenAL::end() {

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);


	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
        StaticSoundSource *src = (*i);
        src->stop();
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
        StreamSoundSource *src = (*i);
        src->stop();
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
		delete *i;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		delete *i;
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

    alcMakeContextCurrent( NULL );

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(context != 0) {
		alcDestroyContext(context);
		context = 0;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(device != 0) {
		alcCloseDevice(device);
		device = 0;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void SoundPlayerOpenAL::play(StaticSound* staticSound) {
	assert(staticSound != 0);

	try {
		StaticSoundSource* source = findStaticSoundSource();
		if(source == 0)
			return;
		source->play(staticSound);
	} catch(std::exception& e) {
		std::cerr << "Couldn't play static sound: " << e.what() << "\n";
	}
}

void SoundPlayerOpenAL::play(StrSound* strSound, int64 fadeOn) {
	assert(strSound != 0);

	try {
		StreamSoundSource* source = findStreamSoundSource();
		source->play(strSound, fadeOn);
	} catch(std::exception& e) {
		std::cerr << "Couldn't play streaming sound: " << e.what() << "\n";
	}
}

void SoundPlayerOpenAL::stop(StrSound* strSound, int64 fadeOff) {
	assert(strSound != 0);

	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		StreamSoundSource* source = *i;
		if(source->sound == strSound) {
			source->stop(fadeOff);
		}
	}
}

void SoundPlayerOpenAL::stopAllSounds() {
	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
		StaticSoundSource* source = *i;
		source->stop();
	}
	for(StreamSoundSources::iterator i = streamSources.begin();
			i != streamSources.end(); ++i) {
		StreamSoundSource* source = *i;
		source->stop();
	}
}

void SoundPlayerOpenAL::updateStreams() {
	assert(context != 0);
	try {
		for(StreamSoundSources::iterator i = streamSources.begin();
				i != streamSources.end(); ++i) {
			StreamSoundSource* source = *i;
			try {
				source->update();
			} catch(std::exception& e) {
				std::cerr << "Error while updating sound stream: "
					<< e.what() << "\n";
			}
		}
		alcProcessContext(context);
		checkAlcError("Error while processing audio context: ");
	} catch(...) {
		printOpenALInfo();
		throw;
	}
}

StaticSoundSource* SoundPlayerOpenAL::findStaticSoundSource() {
	// try to find a stopped source
	for(StaticSoundSources::iterator i = staticSources.begin();
			i != staticSources.end(); ++i) {
		StaticSoundSource* source = *i;
		if(! source->playing()) {
			return source;
		}
	}

	// create a new source
	if(staticSources.size() >= params.staticBufferCount) {
		return 0;
	}

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	StaticSoundSource* source = new StaticSoundSource();
	staticSources.push_back(source);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return source;
}

StreamSoundSource* SoundPlayerOpenAL::findStreamSoundSource() {
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

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	StreamSoundSource* source = new StreamSoundSource();
	streamSources.push_back(source);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return source;
}

void SoundPlayerOpenAL::checkAlcError(const char* message)
{
	int err = alcGetError(device);
	if(err != ALC_NO_ERROR) {
		std::stringstream msg;
		msg << message << alcGetString(device, err);
		throw std::runtime_error(msg.str());
	}
}

void SoundPlayerOpenAL::checkAlError(const char* message)
{
	int err = alGetError();
	if(err != AL_NO_ERROR) {
		std::stringstream msg;
		msg << message << alGetString(err);
		throw std::runtime_error(msg.str());
	}
}

}}} // end of namespace

