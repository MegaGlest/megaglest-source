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

#include "texture.h"
#include "util.h"
#include <SDL.h>
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// =====================================================
//	class Texture
// =====================================================

const int Texture::defaultSize       = 256;
const int Texture::defaultComponents = 4;
bool Texture::useTextureCompression  = false;

/* Quick utility function for texture creation */
static int powerOfTwo(int input) {
		int value = 1;

		while (value < input) {
				value <<= 1;
		}
		return value;
}

Texture::Texture() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	mipmap= true;
	pixmapInit= true;
	wrapMode= wmRepeat;
	format= fAuto;

	inited= false;
	forceCompressionDisabled=false;
}


// =====================================================
//	class Texture1D
// =====================================================

void Texture1D::load(const string &path){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getComponents() == -1) { //TODO: look where you really need that
		pixmap.init(defaultComponents);
	}
	pixmap.load(path);
	this->path= path;
}

string Texture1D::getPath() const {
	return (pixmap.getPath() != "" ? pixmap.getPath() : path);
}

void Texture1D::deletePixels() {
	//printf("+++> Texture1D pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

// =====================================================
//	class Texture2D
// =====================================================

SDL_Surface* Texture2D::CreateSDLSurface(bool newPixelData) const {
	SDL_Surface* surface = NULL;

	unsigned char* surfData = NULL;
	if (newPixelData == true) {
		// copy pixel data
		surfData = new unsigned char[pixmap.getW() * pixmap.getH() * pixmap.getComponents()];
		memcpy(surfData, pixmap.getPixels(), pixmap.getW() * pixmap.getH() * pixmap.getComponents());
	} else {
		surfData = pixmap.getPixels();
	}

	// This will only work with 24bit RGB and 32bit RGBA pictures
	surface = SDL_CreateRGBSurfaceFrom(surfData, pixmap.getW(), pixmap.getH(), 8 * pixmap.getComponents(), pixmap.getW() * pixmap.getComponents(), 0x000000FF, 0x0000FF00, 0x00FF0000, (pixmap.getComponents() == 4) ? 0xFF000000 : 0);
	if ((surface == NULL) && newPixelData == true) {
		// cleanup when we failed to the create surface
		delete[] surfData;
	}

//	SDL_Surface *prepGLTexture(SDL_Surface *surface, GLfloat *texCoords = NULL, const bool
//	freeSource = false) {
	        /* Use the surface width and height expanded to powers of 2 */
	        int w = powerOfTwo(surface->w);
	        int h = powerOfTwo(surface->h);
//	        if (texCoords != 0) {
//	                texCoords[0] = 0.0f;                                    /* Min
//	X */
//	                texCoords[1] = 0.0f;                                    /* Min
//	Y */
//	                texCoords[2] = (GLfloat)surface->w / w; /* Max X */
//	                texCoords[3] = (GLfloat)surface->h / h; /* Max Y */
//	        }

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

//	        if (freeSource) {
//	                SDL_FreeSurface(surface);
//	        }

	#ifdef _MSC_VER
	        delete[] line;
	#endif

	        return image;
//	}

	return surface;
}

void Texture2D::load(const string &path){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.load(path);
	this->path= path;
}

string Texture2D::getPath() const {
	return (pixmap.getPath() != "" ? pixmap.getPath() : path);
}

void Texture2D::deletePixels() {
	//printf("+++> Texture2D pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

// =====================================================
//	class Texture3D
// =====================================================

void Texture3D::loadSlice(const string &path, int slice){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.loadSlice(path, slice);
	this->path= path;
}

string Texture3D::getPath() const {
	return (pixmap.getPath() != "" ? pixmap.getPath() : path);
}

void Texture3D::deletePixels() {
	//printf("+++> Texture3D pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

// =====================================================
//	class TextureCube
// =====================================================

void TextureCube::loadFace(const string &path, int face){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getFace(0)->getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.loadFace(path, face);
	this->path= path;
}

string TextureCube::getPath() const {
	string result = "";
	for(int i = 0; i < 6; ++i) {
		if(pixmap.getPath(i) != "") {
			if(result != "") {
				result += ",";
			}
			result += pixmap.getPath(i);
		}
	}
	if(result == "") {
		result = path;
	}
	return result;
}

void TextureCube::deletePixels() {
	//printf("+++> TextureCube pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

}}//end namespace
