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
#ifndef _GLEST_GAME_OBJECTTYPE_H_
#define _GLEST_GAME_OBJECTTYPE_H_

#include <vector>

#include "model.h"
#include "vec.h"

using std::vector;

namespace Glest{ namespace Game{

using Shared::Graphics::Model;
using Shared::Graphics::Vec3f;

// =====================================================
// 	class ObjectType  
//
///	Each of the possible objects of the map: trees, stones ...
// =====================================================

class ObjectType{
private:
	typedef vector<Model*> Models;

private:
	static const int tree1= 0;
	static const int tree2= 1;
	static const int choppedTree= 2;

private:
	Models models;
	Vec3f color;
	int objectClass;
	bool walkable;

public:
	void init(int modelCount, int objectClass, bool walkable);

	void loadModel(const string &path);

	Model *getModel(int i)			{return models[i];}
	int getModelCount() const		{return models.size();}
	const Vec3f &getColor() const	{return color;} 
	int getClass() const			{return objectClass;}
	bool getWalkable() const		{return walkable;}
	bool isATree() const			{return objectClass==tree1 || objectClass==tree2;}
};

}}//end namespace

#endif
