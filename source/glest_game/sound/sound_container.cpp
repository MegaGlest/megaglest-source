// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "sound_container.h"

#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class SoundContainer
// =====================================================

SoundContainer::SoundContainer(){
	lastSound= -1;
}

StaticSound *SoundContainer::getRandSound() const{
	switch(sounds.size()){
	case 0:
		return NULL;
	case 1:
		return sounds[0];
	default:
		int soundIndex= random.randRange(0, sounds.size()-1);
		if(soundIndex==lastSound){
			soundIndex= (lastSound+1) % sounds.size();
		}
		lastSound= soundIndex;
		return sounds[soundIndex];
	}
}

}}//end namespace
