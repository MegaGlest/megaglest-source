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

#include "sound_player.h"

#include "leak_dumper.h"

namespace Shared{ namespace Sound{

// =====================================================
//	class SoundPlayerParams
// =====================================================

SoundPlayerParams::SoundPlayerParams(){
	staticBufferCount= 8;
	strBufferCount= 4;
	strBufferSize= 44050*2*2*2;	//2 second buffer
}

}}//end namespace
