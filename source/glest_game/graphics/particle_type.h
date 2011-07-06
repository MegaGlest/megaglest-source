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

#ifndef _GLEST_GAME_PARTICLETYPE_H_
#define _GLEST_GAME_PARTICLETYPE_H_

#include <string>
#include <list>

#include "particle.h"
#include "factory.h"
#include "texture.h"
#include "vec.h"
#include "xml_parser.h"
#include "graphics_interface.h"
#include "leak_dumper.h"

using std::string;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

using Shared::Graphics::ParticleSystem;
using Shared::Graphics::UnitParticleSystem;
using Shared::Graphics::AttackParticleSystem;
using Shared::Graphics::ProjectileParticleSystem;
using Shared::Graphics::SplashParticleSystem;
using Shared::Graphics::Texture2D;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Model;
using Shared::Util::MultiFactory;
using Shared::Xml::XmlNode;

class UnitParticleSystemType;

// ===========================================================
//	class ParticleSystemType 
//
///	A type of particle system
// ===========================================================

class ParticleSystemType {
protected:
	string type;
	Texture2D *texture;
	Model *model;
	float modelCycle;
	string primitive;
	Vec3f offset;
	Vec4f color;
	Vec4f colorNoEnergy;
	float size;
	float sizeNoEnergy;
	float speed;
	float gravity;
	float emissionRate;
	int energyMax;
	int energyVar;
	string mode;
	bool teamcolorNoEnergy;
    bool teamcolorEnergy;
    int alternations;
	typedef std::list<UnitParticleSystemType*> Children;
	Children children;

    bool minmaxEnabled;
    int minHp;
    int maxHp;
    bool minmaxIsPercent;

public:
	ParticleSystemType();
	virtual ~ParticleSystemType();
	void load(const XmlNode *particleSystemNode, const string &dir,
			RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
			string parentLoader, string techtreePath);
	void setValues(AttackParticleSystem *ats);
	bool hasTexture() const { return(texture != NULL); }
	bool hasModel() const { return(model != NULL); }

    bool getMinmaxEnabled() const { return minmaxEnabled;}
    int getMinHp() const { return minHp;}
    int getMaxHp() const { return maxHp;}
    bool getMinmaxIsPercent() const { return minmaxIsPercent; }

    void setMinmaxEnabled(bool value) { minmaxEnabled=value;}
    void setMinHp(int value) { minHp=value;}
    void setMaxHp(int value) { maxHp=value;}
    void setMinmaxIsPercent(bool value) { minmaxIsPercent=value; }

protected:

};

// ===========================================================
//	class ParticleSystemTypeProjectile
// ===========================================================

class ParticleSystemTypeProjectile: public ParticleSystemType{
private:
	string trajectory;
	float trajectorySpeed;
	float trajectoryScale;
	float trajectoryFrequency;

public:
	void load(const XmlNode *particleFileNode, const string &dir, const string &path,
			RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
			string parentLoader, string techtreePath);
	ProjectileParticleSystem *create();

};

// ===========================================================
//	class ParticleSystemTypeSplash
// ===========================================================

class ParticleSystemTypeSplash: public ParticleSystemType {
public:
	void load(const XmlNode *particleFileNode, const string &dir, const string &path,
			RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
			string parentLoader, string techtreePath);
	SplashParticleSystem *create();

private:
	float emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;
};

}}//end namespace

#endif
