// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_SOUND_SOUNDPLAYERDS8_H_
#define _SHARED_SOUND_SOUNDPLAYERDS8_H_

#include "sound_player.h"
#include "platform_util.h"

#include <dsound.h>

#include <vector>

using std::vector;

namespace Shared{ namespace Sound{ namespace Ds8{

// =====================================================
//	class SoundBuffer
// =====================================================

class SoundBuffer{
protected:
	IDirectSoundBuffer8 *dsBuffer;
	Sound *sound;
	DWORD size;

public:
	SoundBuffer();
	virtual ~SoundBuffer(){};
	virtual void end()=0;

	IDirectSoundBuffer8 *getDsBuffer() const	{return dsBuffer;}
	Sound *getSound() const						{return sound;}

	void setDsBuffer(IDirectSoundBuffer8 *dsBuffer)			{this->dsBuffer= dsBuffer;}
	void setSound(IDirectSound8 *dsObject, Sound *sound)	{this->sound= sound;}

	bool isFree();
	bool isReady();

protected:
	void createDsBuffer(IDirectSound8 *dsObject);
};

// =====================================================
//	class StaticSoundBuffer
// =====================================================

class StaticSoundBuffer: public SoundBuffer{
public:
	StaticSound *getStaticSound() const	{return static_cast<StaticSound*>(sound);}
	void init(IDirectSound8 *dsObject, Sound *sound);
	void end();
	void play();
private:
	void fillDsBuffer();
};

// =====================================================
//	class StrSoundBuffer
// =====================================================

class StrSoundBuffer: public SoundBuffer{
private:
	enum State{sFree, sFadingOn, sPlaying, sFadingOff, sStopped};

private:
	DWORD lastPlayCursor;
	State state;
	Chrono chrono;	//delay-fade chrono
	int64 fade;		//fade on fade off delay

public:
	StrSoundBuffer();
	StrSound *getStrSound() const	{return static_cast<StrSound*>(sound);}
	
	void init(IDirectSound8 *dsObject, Sound *sound, uint32 strBufferSize);
	void end();
	void play(int64 fadeOn);
	void update();
	void stop(int64 fadeOff);
	

private:
	void fillDsBuffer();
	void refreshDsBuffer();
	void readChunk(void *writePointer, uint32 size);
};

// =====================================================
//	class SoundPlayerDs8  
//
///	SoundPlayer implementation using Direct Sound 8
// =====================================================

class SoundPlayerDs8: public SoundPlayer{
private:
	IDirectSound8 *dsObject;
	vector<StaticSoundBuffer> staticSoundBuffers;
	vector<StrSoundBuffer> strSoundBuffers;
	SoundPlayerParams params;

public:
	SoundPlayerDs8();
	virtual void init(const SoundPlayerParams *params);
	virtual void end();
	virtual void play(StaticSound *staticSound);
	virtual void play(StrSound *strSound, int64 fadeOn=0);
	virtual void stop(StrSound *strSound, int64 fadeOff=0);
	virtual void stopAllSounds();
	virtual void updateStreams();	//updates str buffers if needed

private:
	bool findStaticBuffer(Sound *sound, int *bufferIndex);
	bool findStrBuffer(Sound *sound, int *bufferIndex);
};

// =====================================================
//	Misc
// =====================================================

long dsVolume(float floatVolume);

}}}//end namespace

#endif
