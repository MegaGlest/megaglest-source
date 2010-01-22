// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_GL_GRAPHICSFACTORYBASICGL_H_
#define _SHARED_GRAPHICS_GL_GRAPHICSFACTORYBASICGL_H_

#include "graphics_factory.h"
#include "text_renderer_gl.h"
#include "model_renderer_gl.h"
#include "context_gl.h"
#include "model_gl.h"
#include "texture_gl.h"
#include "font_gl.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class GraphicsFactoryBasicGl
// =====================================================

class GraphicsFactoryBasicGl: public GraphicsFactory{
public:
	virtual TextRenderer2D *newTextRenderer2D()	{return new TextRenderer2DGl();}
	virtual TextRenderer3D *newTextRenderer3D()	{return new TextRenderer3DGl();}
	virtual ModelRenderer *newModelRenderer()	{return new ModelRendererGl();}
	virtual Context *newContext()				{return new ContextGl();}
	virtual Model *newModel()					{return new ModelGl();}
	virtual Texture2D *newTexture2D()			{return new Texture2DGl();}
	virtual Font2D *newFont2D()					{return new Font2DGl();}
	virtual Font3D *newFont3D()					{return new Font3DGl();}
};

}}}//end namespace

#endif
