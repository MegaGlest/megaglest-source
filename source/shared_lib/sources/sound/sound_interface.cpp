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

#include "leak_dumper.h"

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
	return soundFactory->newSoundPlayer();
}

}}//end namespace
