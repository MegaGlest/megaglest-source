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

#ifndef _SHARED_SOUND_SOUNDFILELOADER_H_
#define _SHARED_SOUND_SOUNDFILELOADER_H_

#include <string>
#include <fstream>

#include "types.h"
#include "factory.h"

struct OggVorbis_File;

using std::string;
using std::ifstream;

namespace Shared{ namespace Sound{

using Platform::uint32;
using Platform::int8;
using Util::MultiFactory;

class SoundInfo;

// =====================================================
//	class SoundFileLoader  
//
///	Interface that all SoundFileLoaders will implement
// =====================================================

class SoundFileLoader{
public:
	virtual ~SoundFileLoader(){}

	virtual void open(const string &path, SoundInfo *soundInfo)= 0;
	virtual uint32 read(int8 *samples, uint32 size)= 0;
	virtual void close()= 0;
	virtual void restart()= 0;
};

// =====================================================
//	class WavSoundFileLoader  
//
///	Wave file loader
// =====================================================

class WavSoundFileLoader: public SoundFileLoader{
private:
	static const int maxDataRetryCount= 10;

private:
	uint32 dataOffset;
	uint32 dataSize;
	uint32 bytesPerSecond;
	ifstream f;

public:
	virtual void open(const string &path, SoundInfo *soundInfo);
	virtual uint32 read(int8 *samples, uint32 size);
	virtual void close();
	virtual void restart();
};

// =====================================================
//	class OggSoundFileLoader 
//
///	OGG sound file loader, uses ogg-vorbis library
// =====================================================

class OggSoundFileLoader: public SoundFileLoader{
private:
	OggVorbis_File *vf;
	FILE *f;

public:
	virtual void open(const string &path, SoundInfo *soundInfo);
	virtual uint32 read(int8 *samples, uint32 size);
	virtual void close();
	virtual void restart();
};

// =====================================================
//	class SoundFileLoaderFactory
// =====================================================

class SoundFileLoaderFactory: public MultiFactory<SoundFileLoader>{
private:
	SoundFileLoaderFactory();
public:
	static SoundFileLoaderFactory * getInstance();
};

}}//end namespace

#endif
