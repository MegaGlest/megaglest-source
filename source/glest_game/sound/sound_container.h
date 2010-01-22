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

#ifndef _GLEST_GAME_SOUNDCONTAINER_H_
#define _GLEST_GAME_SOUNDCONTAINER_H_

#include <vector>

#include "sound.h"
#include "random.h"

using std::vector;
using Shared::Util::Random;
using Shared::Sound::StaticSound;

namespace Glest{ namespace Game{

// =====================================================
// 	class SoundContainer
//
/// Holds a list of sounds that are usually played at random
// =====================================================

class SoundContainer{
public:
	typedef vector<StaticSound*> Sounds;

private:
	Sounds sounds;
	mutable Random random;
	mutable int lastSound;

public:
	SoundContainer();

	void resize(int size)			{sounds.resize(size);}
	StaticSound *&operator[](int i)	{return sounds[i];}

	const Sounds &getSounds() const	{return sounds;}
	StaticSound *getRandSound() const;
};

}}//end namespace

#endif
