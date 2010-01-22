//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under 
//the terms of the GNU General Public License as published by the Free Software 
//Foundation; either version 2 of the License, or (at your option) any later 
//version.
#ifndef _SHARED_SOUND_SOUNDPLAYEROPENAL_H_
#define _SHARED_SOUND_SOUNDPLAYEROPENAL_H_

#include "sound_player.h"
#include "platform_util.h"

#include <SDL.h>
#include <AL/alc.h>
#include <AL/al.h>

#include <vector>

using std::vector;

namespace Shared{ namespace Sound{ namespace OpenAL{

class SoundSource {
public:
	SoundSource();
	virtual ~SoundSource();

	bool playing();
	void stop();

protected:
	friend class SoundPlayerOpenAL;
	ALenum getFormat(Sound* sound);
		
	ALuint source;	
};

class StaticSoundSource : public SoundSource {
public:
	StaticSoundSource();
	virtual ~StaticSoundSource();

	void play(StaticSound* sound);

protected:
	friend class SoundPlayerOpenAL;
	bool bufferAllocated;
	ALuint buffer;
};

class StreamSoundSource : public SoundSource {
public:
	StreamSoundSource();
	virtual ~StreamSoundSource();

	void play(StrSound* sound, int64 fade);
	void update();
	void stop();
	void stop(int64 fade);

protected:
	friend class SoundPlayerOpenAL;
	static const size_t STREAMBUFFERSIZE = 1024 * 500;
	static const size_t STREAMFRAGMENTS = 5;
	static const size_t STREAMFRAGMENTSIZE 
		= STREAMBUFFERSIZE / STREAMFRAGMENTS;
	
	bool fillBufferAndQueue(ALuint buffer);
	
	StrSound* sound;
	ALuint buffers[STREAMFRAGMENTS];
	ALenum format;

	enum FadeState { NoFading, FadingOn, FadingOff };
	FadeState fadeState;
	Chrono chrono; // delay-fade chrono
	int64 fade;
};

// ==============================================================
//	class SoundPlayerSDL
//
///	SoundPlayer implementation using SDL_mixer
// ==============================================================

class SoundPlayerOpenAL : public SoundPlayer {
public:
	SoundPlayerOpenAL();
	virtual ~SoundPlayerOpenAL();
	virtual void init(const SoundPlayerParams *params);
	virtual void end();
	virtual void play(StaticSound *staticSound);
	virtual void play(StrSound *strSound, int64 fadeOn=0);
	virtual void stop(StrSound *strSound, int64 fadeOff=0);
	virtual void stopAllSounds();
	virtual void updateStreams();	//updates str buffers if needed

private:
	friend class SoundSource;
	friend class StaticSoundSource;
	friend class StreamSoundSource;

	void printOpenALInfo();
	
	StaticSoundSource* findStaticSoundSource();
	StreamSoundSource* findStreamSoundSource();
	void checkAlcError(const char* message);
	static void checkAlError(const char* message);
	
	ALCdevice* device;
	ALCcontext* context;

	typedef std::vector<StaticSoundSource*> StaticSoundSources;
	StaticSoundSources staticSources;
	typedef std::vector<StreamSoundSource*> StreamSoundSources;
	StreamSoundSources streamSources;

	SoundPlayerParams params;
};

}}}//end namespace

#endif

