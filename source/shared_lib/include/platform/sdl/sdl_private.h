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
#ifndef _SHARED_PLATFORM_SDL_GLOBALS_H_
#define _SHARED_PLATFORM_SDL_GLOBALS_H_

// This header contains things that should not be used outside the platform/sdl
// directory

namespace Shared{ namespace Platform{ namespace Private{

extern bool shouldBeFullscreen;
extern int ScreenWidth;
extern int ScreenHeight;

}}}

#endif

