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

using std::string;

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

class UnitParticleSystemType{
protected:
	string type;
	Texture2D *texture;
	string primitive;
	Vec3f offset;
	Vec3f direction;
	Vec4f color;
	Vec4f colorNoEnergy;
	float radius;
	float size;
	float sizeNoEnergy;
	float speed;
	float gravity;
	int emissionRate;
	int energyMax;
	int energyVar;
    bool relative;
    bool relativeDirection;
    bool fixed;
    bool teamcolorNoEnergy;
    bool teamcolorEnergy;
    string mode;

public:
	UnitParticleSystemType();
	void load(const XmlNode *particleSystemNode, const string &dir);
	void load(const string &dir, const string &path);
	void setValues(UnitParticleSystem *uts);
};

}}//end namespace

#endif
