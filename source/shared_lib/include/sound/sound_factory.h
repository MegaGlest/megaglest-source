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

#ifndef _SHARED_SOUND_SOUNDFACTORY_H_
#define _SHARED_SOUND_SOUNDFACTORY_H_

#include "sound_player.h"
#include "leak_dumper.h"

namespace Shared{ namespace Sound{

// =====================================================
//	class SoundFactory
// =====================================================

class SoundFactory{
public:
	virtual ~SoundFactory(){}
	virtual SoundPlayer *newSoundPlayer()	{return NULL;}	
};

}}//end namespace

#endif
