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

#ifndef _GLEST_GAME_TILESET_H_
#define _GLEST_GAME_TILESET_H_

#include <map>
#include "graphics_interface.h"
#include "xml_parser.h"
#include "object_type.h"
#include "sound.h"
#include "randomgen.h"
#include "surface_atlas.h"
#include "checksum.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Xml::XmlNode;
using Shared::Sound::StaticSound;
using Shared::Sound::StrSound;
using Shared::Graphics::Model;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Texture2D;
using Shared::Graphics::Texture3D;
using Shared::Util::RandomGen;
using Shared::Util::Checksum;

using std::map;

enum FogMode{
	fmExp,
	fmExp2
};

enum Weather{
	wSunny,
	wRainy,
	wSnowy
};

class Renderer;
class SurfaceCell;

// =====================================================
//	class AmbientSounds
// =====================================================

class AmbientSounds{
private:
	bool enabledDay;
	bool enabledNight;
	bool enabledRain;
	bool enabledSnow;
	bool enabledDayStart;
	bool enabledNightStart;
	bool alwaysPlayDay;
	bool alwaysPlayNight;
	StrSound day;
	StrSound night;
	StrSound rain;
	StrSound snow;
	StaticSound dayStart;
	StaticSound nightStart;

public:
	AmbientSounds() {
		enabledDay = false;
		enabledNight = false;
		enabledRain = false;
		enabledSnow = false;
		enabledDayStart = false;
		enabledNightStart = false;
		alwaysPlayDay = false;
		alwaysPlayNight = false;
	}

	bool isEnabledDay() const			{return enabledDay;}
	bool isEnabledNight() const			{return enabledNight;}
	bool isEnabledRain() const			{return enabledRain;}
	bool isEnabledSnow() const			{return enabledSnow;}
	bool isEnabledDayStart() const		{return enabledDayStart;}
	bool isEnabledNightStart() const	{return enabledNightStart;}
	bool getAlwaysPlayDay() const		{return alwaysPlayDay;}
	bool getAlwaysPlayNight() const		{return alwaysPlayNight;}

	StrSound *getDay() 				{return &day;}
	StrSound *getNight() 			{return &night;}
	StrSound *getRain() 			{return &rain;}
	StrSound *getSnow() 			{return &snow;}
	StaticSound *getDayStart()		{return &dayStart;}
	StaticSound *getNightStart()	{return &nightStart;}

	void load(const string &dir, const XmlNode *xmlNode,
			std::map<string,vector<pair<string, string> > > &loadedFileList,string parentLoader);
};

// =====================================================
// 	class Tileset
//
///	Containt textures, models and parameters for a tileset
// =====================================================

class Tileset {
public:
	static const int waterTexCount= 1;
	static const int surfCount= 6;
	static const int objCount= 10;
	static const int transitionVars= 2; //number or different transition textures

public:
	typedef vector<float> SurfProbs;
	typedef vector<Pixmap2D *> SurfPixmaps;

private:
	SurfaceAtlas surfaceAtlas;
    ObjectType objectTypes[objCount];

	SurfProbs surfProbs[surfCount];
	SurfPixmaps surfPixmaps[surfCount];

	RandomGen random;
    Texture3D *waterTex;
    bool waterEffects;
    bool fog;
    int fogMode;
	float fogDensity;
	Vec3f fogColor;
	Vec3f sunLightColor;
	Vec3f moonLightColor;
	Weather weather;

	AmbientSounds ambientSounds;
	Checksum checksumValue;

public:
	Tileset() {
		waterTex = NULL;
	    waterEffects = false;
	    fog = false;
	    fogMode = 0;
		fogDensity = 0.0f;
		weather= wSunny;
	}
    ~Tileset();
    Checksum loadTileset(const vector<string> pathList, const string &tilesetName,
    		Checksum* checksum, std::map<string,vector<pair<string, string> > > &loadedFileList);
	void load(const string &dir, Checksum *checksum, Checksum *tilesetChecksum,
			std::map<string,vector<pair<string, string> > > &loadedFileList);
	Checksum * getChecksumValue() { return &checksumValue; }

    //get
	const SurfaceAtlas *getSurfaceAtlas() const		{return &surfaceAtlas;}
	ObjectType *getObjectType(int i)				{return &objectTypes[i];}
	float getSurfProb(int surf, int var) const		{return surfProbs[surf][var];}
	Texture3D *getWaterTex() const					{return waterTex;}
	bool getWaterEffects() const					{return waterEffects;}
	bool getFog() const								{return fog;}
	int getFogMode() const							{return fogMode;}
	float getFogDensity() const						{return fogDensity;}
	const Vec3f &getFogColor() const				{return fogColor;}
	const Vec3f &getSunLightColor() const			{return sunLightColor;}
	const Vec3f &getMoonLightColor() const			{return moonLightColor;}
	Weather getWeather() const						{return weather;}
	void setWeather(Weather value) { weather = value; }

	//surface textures
	const Pixmap2D *getSurfPixmap(int type, int var) const;
	void addSurfTex(int leftUp, int rightUp, int leftDown, int rightDown, Vec2f &coord, const Texture2D *&texture);

	//sounds
	AmbientSounds *getAmbientSounds() {return &ambientSounds;}
};

}} //end namespace

#endif
