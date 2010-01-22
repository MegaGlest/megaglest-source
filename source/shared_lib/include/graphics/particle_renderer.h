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

#ifndef _SHARED_GRAPHICS_PARTICLERENDERER_H_
#define _SHARED_GRAPHICS_PARTICLERENDERER_H_

#include "particle.h"

namespace Shared{ namespace Graphics{

class ModelRenderer;

// =====================================================
//	class ParticleRenderer
// =====================================================

class ParticleRenderer{
public:
	//particles
	virtual ~ParticleRenderer(){};
	virtual void renderManager(ParticleManager *pm, ModelRenderer *mr)=0;
	virtual void renderSystem(ParticleSystem *ps)=0;
	virtual void renderSystemLine(ParticleSystem *ps)=0;
	virtual void renderSystemLineAlpha(ParticleSystem *ps)=0;
	virtual void renderSingleModel(AttackParticleSystem *ps, ModelRenderer *mr)=0;
};

}}//end namespace

#endif
