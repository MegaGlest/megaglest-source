// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_UNITPARTICLETYPE_H_
#define _GLEST_GAME_UNITPARTICLETYPE_H_

#include <string>

#include "particle.h"
#include "factory.h"
#include "texture.h"
#include "vec.h"
#include "xml_parser.h"
#include "graphics_interface.h"
#include "leak_dumper.h"
#include "particle_type.h"

using std::string;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

using Shared::Graphics::ParticleSystem;
using Shared::Graphics::UnitParticleSystem;
using Shared::Graphics::Texture2D;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec4f;
using Shared::Util::MultiFactory;
using Shared::Xml::XmlNode;

// ===========================================================
//	class ParticleSystemType 
//
///	A type of particle system
// ===========================================================

class UnitParticleSystemType: public ParticleSystemType{
protected:

	float radius;
	Vec3f direction;
    bool relative;
    bool relativeDirection;
    bool fixed;
    int staticParticleCount;

public:
	void load(const XmlNode *particleSystemNode, const string &dir, RendererInterface *newTexture);
	void load(const string &dir, const string &path, RendererInterface *newTexture);
	const void setValues (UnitParticleSystem *uts);
	bool hasTexture() const { return(texture != NULL); }
};

class ObjectParticleSystemType: public UnitParticleSystemType{
};

}}//end namespace

#endif
