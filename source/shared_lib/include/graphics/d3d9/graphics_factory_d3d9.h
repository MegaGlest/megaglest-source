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

#ifndef _SHARED_D3D9_GRAPHICSFACTORYD3D9_H_
#define _SHARED_D3D9_GRAPHICSFACTORYD3D9_H_

#include "context_d3d9.h"
#include "model_renderer_d3d9.h"
#include "texture_d3d9.h"
#include "font_d3d9.h"
#include "text_renderer_d3d9.h"
#include "shader_d3d9.h"
#include "graphics_factory.h"

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class GraphicsFactoryD3d9  
// ===============================

class GraphicsFactoryD3d9: public GraphicsFactory{
public:
	virtual TextRenderer2D *newTextRenderer2D()		{return new TextRenderer2DD3d9();}
	virtual ModelRenderer *newModelRenderer()		{return NULL;}
	virtual Context *newContext()					{return new ContextD3d9();}
	virtual Texture2D *newTexture2D()				{return new Texture2DD3d9();}
	virtual TextureCube *newTextureCube()			{return new TextureCubeD3d9();}
	virtual Font2D *newFont2D()						{return new Font2DD3d9();}
	virtual ShaderProgram *newShaderProgram()		{return new ShaderProgramD3d9();}
	virtual VertexShader *newVertexShader()			{return new VertexShaderD3d9();}
	virtual FragmentShader *newFragmentShader()		{return new PixelShaderD3d9();}
};

}}}//end namespace

#endif
