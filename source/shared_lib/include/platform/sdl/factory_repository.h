// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_FACTORYREPOSITORY_H_
#define _SHARED_PLATFORM_FACTORYREPOSITORY_H_

#include <string>

#include "graphics_factory.h"
#include "sound_factory.h"

#include "graphics_factory_gl.h"
#include "sound_factory_openal.h"

using std::string;

using Shared::Graphics::GraphicsFactory;
using Shared::Sound::SoundFactory;
using Shared::Graphics::Gl::GraphicsFactoryGl;
using Shared::Sound::OpenAL::SoundFactoryOpenAL;

namespace Shared{ namespace Platform{

// =====================================================
//	class FactoryRepository
// =====================================================

class FactoryRepository{
private:
	FactoryRepository(){};
	FactoryRepository(const FactoryRepository& );
	void operator=(const FactoryRepository& );

private:
	GraphicsFactoryGl graphicsFactoryGl;
	SoundFactoryOpenAL soundFactoryOpenAL;

public:
	static FactoryRepository &getInstance();

	GraphicsFactory *getGraphicsFactory(const string &name);
	SoundFactory *getSoundFactory(const string &name);
};

}}//end namespace

#endif
