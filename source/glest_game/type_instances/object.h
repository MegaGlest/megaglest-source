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
#ifndef _GLEST_GAME_OBJECT_H_
#define _GLEST_GAME_OBJECT_H_

#include "model.h"
#include "vec.h"

namespace Glest{ namespace Game{

class ObjectType;
class ResourceType;
class Resource;

using Shared::Graphics::Model;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec3f;

// =====================================================
// 	class Object
//
///	A map object: tree, stone...
// =====================================================

class Object{
private:
	ObjectType *objectType;
	Resource *resource;
	Vec3f pos;
	float rotation;
	int variation;

public:
	Object(ObjectType *objectType, const Vec3f &pos);
	~Object();

	void setHeight(float height)		{pos.y= height;}
	
	const ObjectType *getType() const	{return objectType;}
	Resource *getResource() const		{return resource;}
	Vec3f getPos() const				{return pos;}
	const Vec3f & getConstPos() const	{return pos;}
	float getRotation()					{return rotation;}	
	const Model *getModel() const;
	bool getWalkable() const;

	void setResource(const ResourceType *resourceType, const Vec2i &pos);
};

}}//end namespace

#endif
