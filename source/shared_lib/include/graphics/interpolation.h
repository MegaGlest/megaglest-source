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
#include <map>
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class InterpolationData
// =====================================================

class InterpolationData{
private:
	const Mesh *mesh;

	Vec3f *vertices;
	Vec3f *normals;

	int raw_frame_ofs;

	static bool enableInterpolation;
	
	void update(const Vec3f* src, Vec3f* &dest, float t, bool cycle);

public:
	InterpolationData(const Mesh *mesh);
	~InterpolationData();

	static void setEnableInterpolation(bool enabled) { enableInterpolation = enabled; }

	const Vec3f *getVertices() const	{return !vertices || !enableInterpolation? mesh->getVertices()+raw_frame_ofs: vertices;}
	const Vec3f *getNormals() const		{return !normals || !enableInterpolation? mesh->getNormals()+raw_frame_ofs: normals;}
	
	void update(float t, bool cycle);
	void updateVertices(float t, bool cycle);
	void updateNormals(float t, bool cycle);
};

}}//end namespace

#endif
