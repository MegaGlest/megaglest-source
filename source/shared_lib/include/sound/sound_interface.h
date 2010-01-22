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

#ifndef _SHARED_SOUND_SOUNDINTERFACE_H_
#define _SHARED_SOUND_SOUNDINTERFACE_H_

#include "sound_factory.h"

namespace Shared{ namespace Sound{

// =====================================================
//	class SoundInterface
// =====================================================

class SoundInterface{
private:
	SoundFactory *soundFactory;

private:
	SoundInterface(){}
	SoundInterface(SoundInterface &);
	void operator=(SoundInterface &);

public:
	static SoundInterface &getInstance();

	void setFactory(SoundFactory *soundFactory);

	SoundPlayer *newSoundPlayer();
};

}}//end namespace

#endif
