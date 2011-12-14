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

#ifndef _SHARED_GRAPHICS_CAMERA_H_
#define _SHARED_GRAPHICS_CAMERA_H_

#include "vec.h"
#include "quaternion.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class Camera
// =====================================================

class Camera{
private:
	Quaternion orientation;
	Vec3f position;
public:
	Camera();

	Vec3f getPosition() const			{return position;}
	Quaternion getOrientation() const	{return orientation;}

	const Vec3f & getConstPosition() const			{return position;}
	const Quaternion & getConstOrientation() const	{return orientation;}

	void setPosition(const Vec3f &position)				{this->position= position;}
	void setOrientation(const Quaternion &orientation)	{this->orientation= orientation;}

	void moveLocalX(float amount);
	void moveLocalY(float amount);
	void moveLocalZ(float amount);

	void addYaw(float amount);
	void addPitch(float amount);
	void addRoll(float amount);
};

}}//end namespace

#endif
