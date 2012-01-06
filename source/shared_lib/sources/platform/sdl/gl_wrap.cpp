//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.
#include "gl_wrap.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

#include "opengl.h"
#include "sdl_private.h"
#include "noimpl.h"
#include "util.h"
#include "window.h"
#include <vector>
//#include <SDL_image.h>
#include "model.h"
#include "texture.h"
#include "graphics_interface.h"
#include "graphics_factory.h"
#include "platform_common.h"
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::PlatformCommon;

namespace Shared{ namespace Platform{

// ======================================
//	class PlatformContextGl
// ======================================
PlatformContextGl::PlatformContextGl() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	icon = NULL;
	screen = NULL;
}

PlatformContextGl::~PlatformContextGl() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	end();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

/* Quick utility function for texture creation */
static int powerOfTwo(int input) {
        int value = 1;

        while (value < input) {
                value <<= 1;
        }
        return value;
}

SDL_Surface *prepGLTexture(SDL_Surface *surface, GLfloat *texCoords = NULL, const bool
freeSource = false) {
        /* Use the surface width and height expanded to powers of 2 */
        int w = powerOfTwo(surface->w);
        int h = powerOfTwo(surface->h);
        if (texCoords != 0) {
                texCoords[0] = 0.0f;                                    /* Min
X */
                texCoords[1] = 0.0f;                                    /* Min
Y */
                texCoords[2] = (GLfloat)surface->w / w; /* Max X */
                texCoords[3] = (GLfloat)surface->h / h; /* Max Y */
        }

        SDL_Surface *image = SDL_CreateRGBSurface(
                SDL_SWSURFACE,
                w, h,
                32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
                0x000000FF,
                0x0000FF00,
                0x00FF0000,
                0xFF000000
#else
                0xFF000000,
                0x00FF0000,
                0x0000FF00,
                0x000000FF
#endif
        );
        if ( image == NULL ) {
                return 0;
        }

        /* Save the alpha blending attributes */
        Uint32 savedFlags = surface->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
        Uint8  savedAlpha = surface->format->alpha;
        if ( (savedFlags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
                SDL_SetAlpha(surface, 0, 0);
        }

        SDL_Rect srcArea, destArea;
        /* Copy the surface into the GL texture image */
        srcArea.x = 0; destArea.x = 0;
        /* Copy it in at the bottom, because we're going to flip
           this image upside-down in a moment
        */
        srcArea.y = 0; destArea.y = h - surface->h;
        srcArea.w = surface->w;
        srcArea.h = surface->h;
        SDL_BlitSurface(surface, &srcArea, image, &destArea);

        /* Restore the alpha blending attributes */
        if ((savedFlags & SDL_SRCALPHA) == SDL_SRCALPHA) {
                SDL_SetAlpha(surface, savedFlags, savedAlpha);
        }

        /* Turn the image upside-down, because OpenGL textures
           start at the bottom-left, instead of the top-left
        */
#ifdef _MSC_VER
        Uint8 *line = new Uint8[image->pitch];
#else
        Uint8 line[image->pitch];
#endif
        /* These two make the following more readable */
        Uint8 *pixels = static_cast<Uint8*>(image->pixels);
        Uint16 pitch = image->pitch;
        int ybegin = 0;
        int yend = image->h - 1;

        // TODO: consider if this lock is legal/appropriate
        if (SDL_MUSTLOCK(image)) { SDL_LockSurface(image); }
        while (ybegin < yend) {
                memcpy(line, pixels + pitch*ybegin, pitch);
                memcpy(pixels + pitch*ybegin, pixels + pitch*yend, pitch);
                memcpy(pixels + pitch*yend, line, pitch);
                ybegin++;
                yend--;
        }
        if (SDL_MUSTLOCK(image)) { SDL_UnlockSurface(image); }

        if (freeSource) {
                SDL_FreeSurface(surface);
        }

#ifdef _MSC_VER
        delete[] line;
#endif

        return image;
}

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits,
		                     bool hardware_acceleration,
		                     bool fullscreen_anti_aliasing, float gammaValue) {
	
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Window::setupGraphicsScreen(depthBits, stencilBits, hardware_acceleration, fullscreen_anti_aliasing);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	int flags = SDL_OPENGL;
	if(PlatformCommon::Private::shouldBeFullscreen) {
		flags |= SDL_FULLSCREEN;
		Window::setIsFullScreen(true);
	}
	else {
		Window::setIsFullScreen(false);
	}

	//flags |= SDL_HWSURFACE

	int resW = PlatformCommon::Private::ScreenWidth;
	int resH = PlatformCommon::Private::ScreenHeight;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {

	#ifndef WIN32
		string mg_icon_file = "";
	#if defined(CUSTOM_DATA_INSTALL_PATH_VALUE)
		if(fileExists(CUSTOM_DATA_INSTALL_PATH_VALUE + "megaglest.png")) {
			mg_icon_file = CUSTOM_DATA_INSTALL_PATH_VALUE + "megaglest.png";
		}
		else if(fileExists(CUSTOM_DATA_INSTALL_PATH_VALUE + "megaglest.bmp")) {
			mg_icon_file = CUSTOM_DATA_INSTALL_PATH_VALUE + "megaglest.bmp";
		}

	#endif

		if(mg_icon_file == "" && fileExists("megaglest.png")) {
			mg_icon_file = "megaglest.png";
		}
		else if(mg_icon_file == "" && fileExists("megaglest.bmp")) {
			mg_icon_file = "megaglest.bmp";
		}
		else if(mg_icon_file == "" && fileExists("/usr/share/pixmaps/megaglest.png")) {
			mg_icon_file = "/usr/share/pixmaps/megaglest.png";
		}
		else if(mg_icon_file == "" && fileExists("/usr/share/pixmaps/megaglest.bmp")) {
			mg_icon_file = "/usr/share/pixmaps/megaglest.bmp";
		}

		if(mg_icon_file != "") {

			if(icon != NULL) {
				SDL_FreeSurface(icon);
				icon = NULL;
			}

			//printf("Loading icon [%s]\n",mg_icon_file.c_str());
			if(extractExtension(mg_icon_file) == "bmp") {
				icon = SDL_LoadBMP(mg_icon_file.c_str());
				icon = prepGLTexture(icon);
			}
			else {
				//printf("Loadng png icon\n");
				Texture2D *texture2D = GraphicsInterface::getInstance().getFactory()->newTexture2D();
				texture2D->load(mg_icon_file);
				icon = texture2D->CreateSDLSurface(true);
				delete texture2D;
			}

		//SDL_Surface *icon = IMG_Load("megaglest.ico");


	//#if !defined(MACOSX)
		// Set Icon (must be done before any sdl_setvideomode call)
		// But don't set it on OS X, as we use a nicer external icon there.
	//#if WORDS_BIGENDIAN
	//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,8,128,0xff000000,0x00ff0000,0x0000ff00,0);
	//#else
	//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,32,128,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
	//#endif

			//printf("In [%s::%s Line: %d] icon = %p\n",__FILE__,__FUNCTION__,__LINE__,icon);
			if(icon == NULL) {
				printf("Error: %s\n", SDL_GetError());
			}
			if(icon != NULL) {

				//uint32 colorkey = SDL_MapRGB(icon->format, 255, 0, 255);
				//SDL_SetColorKey(icon, SDL_SRCCOLORKEY, colorkey);
				SDL_SetColorKey(icon, SDL_SRCCOLORKEY, SDL_MapRGB(icon->format, 255, 0, 255));

				SDL_WM_SetIcon(icon, NULL);
			}
		}
	#endif

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to set resolution: %d x %d, colorBits = %d.\n",__FILE__,__FUNCTION__,__LINE__,resW,resH,colorBits);

		if(screen != NULL) {
			SDL_FreeSurface(screen);
			screen = NULL;
		}


		screen = SDL_SetVideoMode(resW, resH, colorBits, flags);
		if(screen == 0) {
			std::ostringstream msg;
			msg << "Couldn't set video mode "
				<< resW << "x" << resH << " (" << colorBits
				<< "bpp " << stencilBits << " stencil "
				<< depthBits << " depth-buffer). SDL Error is: " << SDL_GetError();

			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,msg.str().c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,msg.str().c_str());

			for(int i = 32; i >= 8; i-=8) {
				// try different color bits
				screen = SDL_SetVideoMode(resW, resH, i, flags);
				if(screen != 0) {
					break;
				}
			}

			if(screen == 0) {
				for(int i = 32; i >= 8; i-=8) {
					// try to revert to 800x600
					screen = SDL_SetVideoMode(800, 600, i, flags);
					if(screen != 0) {
						resW = 800;
						resH = 600;
						break;
					}
				}
			}
			if(screen == 0) {
				for(int i = 32; i >= 8; i-=8) {
					// try to revert to 640x480
					screen = SDL_SetVideoMode(640, 480, i, flags);
					if(screen != 0) {
						resW = 640;
						resH = 480;
						break;
					}
				}
			}

			if(screen == 0) {
				throw std::runtime_error(msg.str());
			}
		}

		SDL_WM_GrabInput(SDL_GRAB_OFF);

		GLuint err = glewInit();
		if (GLEW_OK != err) {
			fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
			//return 1;
			throw std::runtime_error((char *)glewGetErrorString(err));
		}
		//fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

		int bufferSize = (resW * resH * BaseColorPickEntity::COLOR_COMPONENTS);
		BaseColorPickEntity::init(bufferSize);

		if(gammaValue != 0.0) {
			//printf("Attempting to call SDL_SetGamma using value %f\n", gammaValue);

			if (SDL_SetGamma(gammaValue, gammaValue, gammaValue) < 0) {
				printf("WARNING, SDL_SetGamma failed using value %f [%s]\n", gammaValue,SDL_GetError());
			}
		}
	}
}

void PlatformContextGl::end() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(icon != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SDL_FreeSurface(icon);
		icon = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(screen != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SDL_FreeSurface(screen);
		screen = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void PlatformContextGl::makeCurrent() {
}

void PlatformContextGl::swapBuffers() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		SDL_GL_SwapBuffers();
	}
}
	
	
const char *getPlatformExtensions(const PlatformContextGl *pcgl) {
		return "";
}
	
void *getGlProcAddress(const char *procName) {
		void* proc = SDL_GL_GetProcAddress(procName);
		assert(proc!=NULL);
		return proc;
}

}}//end namespace 
