// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_INTERPOLATION_H_
#define _SHARED_GRAPHICS_INTERPOLATION_H_

#include "vec.h"
#include "model.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class InterpolationData
// =====================================================

class InterpolationData{
private:
	const Mesh *mesh;

	Vec3f *vertices;
	Vec3f *normals;

public:
	InterpolationData(const Mesh *mesh);
	~InterpolationData();

	const Vec3f *getVertices() const	{return vertices==NULL? mesh->getVertices(): vertices;}
	const Vec3f *getNormals() const		{return normals==NULL? mesh->getNormals(): normals;}
	
	void update(float t, bool cycle);
	void updateVertices(float t, bool cycle);
	void updateNormals(float t, bool cycle);
};

}}//end namespace

#endif
