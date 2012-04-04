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
#ifndef _GLEST_GAME_OBJECT_H_
#define _GLEST_GAME_OBJECT_H_

#include "model.h"
#include "vec.h"
#include "leak_dumper.h"
#include "particle.h"
#include "object_type.h"
#include "tileset_model_type.h"

namespace Glest{ namespace Game{

class ObjectType;
class ResourceType;
class Resource;
class TechTree;

using Shared::Graphics::Model;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec3f;
using Shared::Graphics::UnitParticleSystem;

// =====================================================
// 	class Object
//
///	A map object: tree, stone...
// =====================================================

class Object;

class ObjectStateInterface {
public:
	virtual void removingObjectEvent(Object *object) = 0;
};

class Object : public BaseColorPickEntity {
private:
	typedef vector<UnitParticleSystem*> UnitParticleSystems;

private:
	ObjectType *objectType;
	vector<UnitParticleSystem*> unitParticleSystems;
	Resource *resource;
	Vec3f pos;
	float rotation;
	int variation;
	int lastRenderFrame;
	Vec2i mapPos;
	bool visible;
	float animProgress;
	float highlight;

	static ObjectStateInterface *stateCallback;

public:
	Object(ObjectType *objectType, const Vec3f &pos, const Vec2i &mapPos);
	~Object();

	void end(); //to kill particles
	void initParticles();
	void initParticlesFromTypes(const ModelParticleSystemTypes *particleTypes);
	static void setStateCallback(ObjectStateInterface *value) { stateCallback=value; }

	const ObjectType *getType() const	{return objectType;}
	Resource *getResource() const		{return resource;}
	Vec3f getPos() const				{return pos;}
	bool isVisible() const				{return visible;}
	const Vec3f & getConstPos() const	{return pos;}
	float getRotation() const			{return rotation;}
	const Model *getModel() const;
	Model *getModelPtr() const;
	bool getWalkable() const;

	float getHightlight() const			{return highlight;}
	bool isHighlighted() const			{return highlight>0.f;}
	void resetHighlight();

	void setResource(const ResourceType *resourceType, const Vec2i &pos);
	void setHeight(float height);
	void setVisible(bool visible);

	int getLastRenderFrame() const { return lastRenderFrame; }
	void setLastRenderFrame(int value) { lastRenderFrame = value; }

	const Vec2i & getMapPos() const { return mapPos; }

	void update();
	float getAnimProgress() const { return animProgress;}

	virtual string getUniquePickName() const;
	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode,const TechTree *techTree);
};

}}//end namespace

#endif
