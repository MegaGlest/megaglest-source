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

#include "sound_interface.h"
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Shared{ namespace Sound{

// =====================================================
//	class SoundInterface
// =====================================================

SoundInterface &SoundInterface::getInstance(){
	static SoundInterface soundInterface;
	return soundInterface;
}

void SoundInterface::setFactory(SoundFactory *soundFactory){
	this->soundFactory= soundFactory;
}

SoundPlayer *SoundInterface::newSoundPlayer(){

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return soundFactory->newSoundPlayer();
}

}}//end namespace
