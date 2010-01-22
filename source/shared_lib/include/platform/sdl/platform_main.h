// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORM_MAIN_H_
#define _SHARED_PLATFORM_MAIN_H_

#include <SDL.h>
#include <iostream>

#define MAIN_FUNCTION(X) int main(int argc, char **argv)                     \
{                                                                            \
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)  {                                 \
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";  \
        return 1;                                                            \
    }                                                                        \
	SDL_EnableUNICODE(1);													 \
    int result = X(argc, argv);                                              \
    SDL_Quit();                                                              \
    return result;                                                           \
}

#endif
