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

#ifndef _SHARED_GRAPHICS_QUATERNION_H_
#define _SHARED_GRAPHICS_QUATERNION_H_ 

#include <string>

#include "vec.h"
#include "matrix.h"

using namespace std;

namespace Shared{ namespace Graphics{

// =====================================================
//	class AxisAngle
// =====================================================

class AxisAngle{
public:
	Vec3f axis;
	float angle;

	AxisAngle(){};
	AxisAngle(const Vec3f &axis, float angle);
};

// =====================================================
//	class EulerAngles
// =====================================================

class EulerAngles{
public:
	float x, y, z;

	EulerAngles(){};
	EulerAngles(float x, float y, float z);
};

// =====================================================
//	class Quaternion
// =====================================================

class Quaternion{
private:
	float w;
	Vec3f v;

public:
	Quaternion();
	Quaternion(float w, const Vec3f &v);
	Quaternion(const EulerAngles &eulerAngles);
	Quaternion(const AxisAngle &axisAngle);

	//initializers
	void setMultIdentity();
	void setAddIdentity();
	void setAxisAngle(const AxisAngle &axisAngle);
	void setEuler(const EulerAngles &eulerAngles);
	
	//unary operators
	float length();
	Quaternion conjugate();
	void normalize();

	//binary operators
	Quaternion operator + (const Quaternion &q) const;
	Quaternion operator * (const Quaternion &q) const;
	void operator += (const Quaternion &q);
	void operator *= (const Quaternion &q);

	//ternary operators
	Quaternion lerp(float t, const Quaternion &q) const;

	//conversions
	Matrix3f toMatrix3() const;
	Matrix4f toMatrix4() const;
	AxisAngle toAxisAngle() const;

	//local axis
	Vec3f getLocalXAxis() const;
	Vec3f getLocalYAxis() const;
	Vec3f getLocalZAxis() const;
};

}}//end namespace

#endif
