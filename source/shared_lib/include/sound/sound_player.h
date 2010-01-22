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

#ifndef _SHARED_SOUND_SOUNDPLAYER_H_
#define _SHARED_SOUND_SOUNDPLAYER_H_

#include "sound.h"
#include "types.h"

using Shared::Platform::uint32;

namespace Shared{ namespace Sound{

// =====================================================
//	class SoundPlayerParams
// =====================================================

class SoundPlayerParams{
public:
	uint32 strBufferSize;
	uint32 strBufferCount;
	uint32 staticBufferCount;

	SoundPlayerParams();
};

// =====================================================
//	class SoundPlayer  
//
//	Interface that every SoundPlayer will implement
// =====================================================

class SoundPlayer{
public:
	virtual ~SoundPlayer(){};
	virtual void init(const SoundPlayerParams *params)= 0;
	virtual void end()= 0;
	virtual void play(StaticSound *staticSound)= 0;
	virtual	void play(StrSound *strSound, int64 fadeOn=0)= 0;	//delay and fade in miliseconds
	virtual void stop(StrSound *strSound, int64 fadeOff=0)= 0;
	virtual void stopAllSounds()= 0;
	virtual void updateStreams()= 0;
};

}}//end namespace

#endif
