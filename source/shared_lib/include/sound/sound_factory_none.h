//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2010 Mark Vejvoda <mark_vejvoda@hotmail.com>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.

#ifndef _SHARED_SOUND_SOUNDFACTORY_NONEL_H_
#define _SHARED_SOUND_SOUNDFACTORY_NONEL_H_

#include "sound_factory.h"
//#include "sound_player_openal.h"

namespace Shared{ namespace Sound{

// ===============================
//	class SoundFactoryOpenAL
// ===============================

class SoundFactoryNone : public SoundFactory{
public:
	virtual SoundPlayer* newSoundPlayer()	{return NULL;}
};

}}//end namespace

#endif
