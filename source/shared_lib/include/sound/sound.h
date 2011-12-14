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

#ifndef _SHARED_SOUND_SOUND_H_
#define _SHARED_SOUND_SOUND_H_

#include <string>
#include "sound_file_loader.h" 
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;

namespace Shared{ namespace Sound{

// =====================================================
//	class SoundInfo
// =====================================================

class SoundInfo{
private:
	uint32 channels;
	uint32 samplesPerSecond;
	uint32 bitsPerSample;
	uint32 size;
	uint32 bitRate;

public:
	SoundInfo();
	virtual ~SoundInfo(){};

	uint32 getChannels() const			{return channels;}
	uint32 getSamplesPerSecond() const	{return samplesPerSecond;}
	uint32 getBitsPerSample() const		{return bitsPerSample;}
	uint32 getSize() const				{return size;}
	uint32 getBitRate() const			{return bitRate;}

	void setChannels(uint32 channels)					{this->channels= channels;}
	void setsamplesPerSecond(uint32 samplesPerSecond)	{this->samplesPerSecond= samplesPerSecond;}
	void setBitsPerSample(uint32 bitsPerSample)			{this->bitsPerSample= bitsPerSample;}
	void setSize(uint32 size)							{this->size= size;}
	void setBitRate(uint32 value) 						{this->bitRate = value;}
};

// =====================================================
//	class Sound
// =====================================================

class Sound {
protected:
	SoundFileLoader *soundFileLoader;
	SoundInfo info;
	float volume;
	string fileName;
	
	//static bool masterserverMode;

public:
	Sound();
	virtual ~Sound(){};

	//static void setMasterserverMode(bool value) { masterserverMode=value; }

	const SoundInfo *getInfo() const	{return &info;}
	float getVolume() const				{return volume;}
	
	void setVolume(float volume)		{this->volume= volume;}
	string getFileName() 				{return fileName; }
};

// =====================================================
//	class StaticSound
// =====================================================

class StaticSound: public Sound{
private:
	int8 * samples;

public:
	StaticSound();
	virtual ~StaticSound();

	int8 *getSamples() const		{return samples;}
	
	void load(const string &path);
	void close();
};

// =====================================================
//	class StrSound
// =====================================================

class StrSound: public Sound{
private:
	StrSound *next;

public:
	StrSound();
	virtual ~StrSound();

	StrSound *getNext() const		{return next;}
	void setNext(StrSound *next)	{this->next= next;}

	void open(const string &path);
	uint32 read(int8 *samples, uint32 size);
	void close();
	void restart();
};

}}//end namespace

#endif
