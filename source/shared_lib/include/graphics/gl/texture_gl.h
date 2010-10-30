// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_GL_TEXTUREGL_H_
#define _SHARED_GRAPHICS_GL_TEXTUREGL_H_

#include "texture.h"
#include "opengl.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class TextureGl
// =====================================================

class TextureGl {
protected:
	GLuint handle;

public:
	TextureGl();
	GLuint getHandle() const	{return handle;}

	void OutputTextureDebugInfo(Texture::Format format, int components, const string path,uint64 rawSize,GLenum texType);
};

// =====================================================
//	class Texture1DGl
// =====================================================

class Texture1DGl: public Texture1D, public TextureGl {
public:
	Texture1DGl();
	virtual ~Texture1DGl();

	virtual void init(Filter filter, int maxAnisotropy= 1);
	virtual void end(bool deletePixelBuffer=true);
};

// =====================================================
//	class Texture2DGl
// =====================================================

class Texture2DGl: public Texture2D, public TextureGl{
public:
	Texture2DGl();
	virtual ~Texture2DGl();

	virtual void init(Filter filter, int maxAnisotropy= 1);
	virtual void end(bool deletePixelBuffer=true);
};

// =====================================================
//	class Texture3DGl
// =====================================================

class Texture3DGl: public Texture3D, public TextureGl{
public:

	Texture3DGl();
	virtual ~Texture3DGl();

	virtual void init(Filter filter, int maxAnisotropy= 1);
	virtual void end(bool deletePixelBuffer=true);
};

// =====================================================
//	class TextureCubeGl
// =====================================================

class TextureCubeGl: public TextureCube, public TextureGl{
public:

	TextureCubeGl();
	virtual ~TextureCubeGl();

	virtual void init(Filter filter, int maxAnisotropy= 1);
	virtual void end(bool deletePixelBuffer=true);
};

}}}//end namespace

#endif
