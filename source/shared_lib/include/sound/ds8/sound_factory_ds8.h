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

#ifndef _SHARED_SOUND_SOUNDFACTORYDS8_H_
#define _SHARED_SOUND_SOUNDFACTORYDS8_H_

#include "sound_factory.h"
#include "sound_player_ds8.h"

namespace Shared{ namespace Sound{ namespace Ds8{

// =====================================================
//	class SoundFactoryDs8
// =====================================================

class SoundFactoryDs8: public SoundFactory{
public:
	virtual SoundPlayer *newSoundPlayer()	{return new SoundPlayerDs8();}	
};

}}}//end namespace

#endif
