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

#ifndef _GLEST_GAME_UNITPARTICLETYPE_H_
#define _GLEST_GAME_UNITPARTICLETYPE_H_

#include <string>
#include <list>

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

using Shared::Graphics::ParticleManager;
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

class UnitParticleSystemType: public ParticleSystemType {
protected:
	UnitParticleSystem::Shape shape;
	float angle;
	float radius;
	float minRadius;
	float emissionRateFade;
	Vec3f direction;
    bool relative;
    bool relativeDirection;
    bool fixed;
    int staticParticleCount;
	bool isVisibleAtNight;
	bool isVisibleAtDay;
	bool radiusBasedStartenergy;
	int delay;
	int lifetime;
	float startTime;
	float endTime;

public:
	UnitParticleSystemType();

	void load(const XmlNode *particleSystemNode, const string &dir,
			RendererInterface *newTexture, std::map<string,vector<pair<string, string> > > &loadedFileList,
			string parentLoader, string techtreePath);
	void load(const XmlNode *particleFileNode, const string &dir, const string &path, RendererInterface *newTexture,
			std::map<string,vector<pair<string, string> > > &loadedFileList,string parentLoader,
			string techtreePath);

	void setStartTime(float startTime) { this->startTime = startTime; }
	float getStartTime() const { return this->startTime; }
	void setEndTime(float endTime) { this->endTime = endTime; }
	float getEndTime() const { return this->endTime; }

	const void setValues (UnitParticleSystem *uts);
	bool hasTexture() const { return(texture != NULL); }
};

class ObjectParticleSystemType: public UnitParticleSystemType{
};

}}//end namespace

#endif
