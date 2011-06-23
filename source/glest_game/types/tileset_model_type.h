// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GLEST_GAME_TILESET_MODEL_TYPE_H_
#define _GLEST_GAME_TILESET_MODEL_TYPE_H_

#include <vector>

#include "model.h"
#include "vec.h"
#include "leak_dumper.h"
#include "unit_particle_type.h"

using std::vector;

namespace Glest{ namespace Game{

using Shared::Graphics::Model;
using Shared::Graphics::Vec3f;

// =====================================================
// 	class ObjectType  
//
///	Each of the possible objects of the map: trees, stones ...
// =====================================================

typedef vector<ObjectParticleSystemType*> ModelParticleSystemTypes;

class TilesetModelType{
private:
	Model *model;
	ModelParticleSystemTypes particleTypes;
	int height;
	bool rotationAllowed;

public:
	~TilesetModelType();

	void addParticleSystem(ObjectParticleSystemType *particleSystem);
	bool hasParticles()	const		{return !particleTypes.empty();}
	ModelParticleSystemTypes* getParticleTypes()  { return &particleTypes ;}


	Model * getModel() const		{return model;}
	void setModel(Model *model) 	{this->model=model;}

	int getHeight() const			{return height;}
	void setHeight(int height) 			{this->height=height;}
	bool getRotationAllowed() const			{return rotationAllowed;}
	void setRotationAllowed(bool rotationAllowed)	{this->rotationAllowed=rotationAllowed;}
};

}}//end namespace

#endif
