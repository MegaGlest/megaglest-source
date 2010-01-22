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

#ifndef _GLEST_GAME_MENUBACKGROUND_H_
#define _GLEST_GAME_MENUBACKGROUND_H_

#include "particle.h"
#include "camera.h"
#include "vec.h"
#include "texture.h"
#include "model.h"
#include "random.h"

using Shared::Graphics::RainParticleSystem;
using Shared::Graphics::FireParticleSystem;
using Shared::Graphics::Camera;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Texture2D;
using Shared::Graphics::Model;
using Shared::Util::Random;

namespace Glest{ namespace Game{

// ===========================================================
// 	class MenuBackground  
//
///	Holds the data to display the 3D environment 
/// in the MenuState
// ===========================================================

class MenuBackground{
public:
	static const int meshSize= 32;
	static const int raindropCount= 1000;
	static const int characterCount= 5;

private:
	Model *mainModel;
	
	//water
	bool water;
	float waterHeight;
	Texture2D *waterTexture;
	
	//fog
	bool fog;
	float fogDensity;
	
	//rain
	bool rain;
	Vec2f raindropPos[raindropCount];
	float raindropStates[raindropCount];

	//camera
	Camera camera;
	Camera lastCamera;
	const Camera *targetCamera;
	float t;

	//misc
	Random random;
	Model *characterModels[characterCount];
	float anim;
	float fade;
	Vec3f aboutPosition;

public:
	MenuBackground();

	bool getWater() const						{return water;}
	float getWaterHeight() const				{return waterHeight;}
	bool getFog() const							{return fog;}
	float getFogDensity() const					{return fogDensity;}
	bool getRain() const						{return rain;}
	Texture2D *getWaterTexture() const			{return waterTexture;}
	const Camera *getCamera() const				{return &camera;}
	const Model *getCharacterModel(int i) const	{return characterModels[i];}
	const Model *getMainModel() const			{return mainModel;}
	float getFade() const						{return fade;}
	Vec2f getRaindropPos(int i) const			{return raindropPos[i];}
	float getRaindropState(int i) const			{return raindropStates[i];}
	float getAnim() const						{return anim;}
	const Vec3f &getAboutPosition() const		{return aboutPosition;}
	
	void setTargetCamera(const Camera *targetCamera);
	void update();

private:
	Vec2f computeRaindropPos();
};

}} //end namespace

#endif

