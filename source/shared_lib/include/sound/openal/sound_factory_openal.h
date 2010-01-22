//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under 
//the terms of the GNU General Public License as published by the Free Software 
//Foundation; either version 2 of the License, or (at your option) any later 
//version.

#ifndef _SHARED_SOUND_SOUNDFACTORYSDL_H_
#define _SHARED_SOUND_SOUNDFACTORYSDL_H_

#include "sound_factory.h"
#include "sound_player_openal.h"

namespace Shared{ namespace Sound{ namespace OpenAL{

// ===============================
//	class SoundFactoryOpenAL
// ===============================

class SoundFactoryOpenAL : public SoundFactory{
public:
	virtual SoundPlayer* newSoundPlayer()	{return new SoundPlayerOpenAL();}	
};

}}}//end namespace

#endif
