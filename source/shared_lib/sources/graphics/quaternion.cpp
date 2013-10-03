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

#include "math_wrapper.h"
#include "quaternion.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class AxisAngle
// =====================================================

AxisAngle::AxisAngle(const Vec3f &axis, float angle){
	this->axis= axis;
	this->angle= angle;
}

// =====================================================
//	class EulerAngles
// =====================================================

EulerAngles::EulerAngles(float x, float y, float z){
	this->x= x;
	this->y= y;
	this->z= z;
}

// =====================================================
//	class Quaternion
// =====================================================

Quaternion::Quaternion(){
	setMultIdentity();
}

Quaternion::Quaternion(float w, const Vec3f &v){
	this->w= w;
	this->v= v;
}

Quaternion::Quaternion(const EulerAngles &eulerAngles){
	setEuler(eulerAngles);
}

//Quaternion::Quaternion(const AxisAngle &axisAngle){
//	setAxisAngle(axisAngle);
//}

void Quaternion::setMultIdentity(){
	w= 1.0f;
	v= Vec3f(0.0f);
}

void Quaternion::setAddIdentity(){
	w= 0.0f;
	v= Vec3f(0.0f);
}

void Quaternion::setEuler(const EulerAngles &eulerAngles){
	Quaternion qx, qy, qz, qr;

	qx.w= std::cos(eulerAngles.x/2.0f);
	qx.v= Vec3f(std::sin(eulerAngles.x/2.0f), 0.0f, 0.0f);

	qy.w= std::cos(eulerAngles.y/2.0f);
	qy.v= Vec3f(0.0f, std::sin(eulerAngles.y/2.0f), 0.0f);

	qz.w= std::cos(eulerAngles.z/2.0f);
	qz.v= Vec3f(0.0f, 0.0f, std::sin(eulerAngles.z/2.0f));
	
	qr= qx*qy*qz;

	w= qr.w;
	v= qr.v;
}

Quaternion Quaternion::conjugate(){
	return Quaternion(w, -v);
}

Quaternion Quaternion::operator + (const Quaternion &q) const{
	return Quaternion(w +q.w, v+q.v);
}
Quaternion Quaternion::operator * (const Quaternion &q) const{
	return Quaternion(
		w*q.w - v.x*q.v.x - v.y*q.v.y - v.z*q.v.z,
		Vec3f(
		w*q.v.x + v.x*q.w + v.y*q.v.z - v.z*q.v.y,
		w*q.v.y + v.y*q.w + v.z*q.v.x - v.x*q.v.z,
		w*q.v.z + v.z*q.w + v.x*q.v.y - v.y*q.v.x));
}

void Quaternion::operator += (const Quaternion &q){
	*this= *this + q;
}

void Quaternion::operator *= (const Quaternion &q){
	*this= *this * q;
}

Quaternion Quaternion::lerp(float t, const Quaternion &q) const{
	return Quaternion(
		w * (1.0f-t) + q.w * t,
		v * (1.0f-t) + q.v * t);
}

Matrix3f Quaternion::toMatrix3() const{
	Matrix3f rm;

	//row1
	rm[0]= 1.0f - 2*v.y*v.y - 2*v.z*v.z;
	rm[3]= 2*v.x*v.y - 2*w*v.z;
	rm[6]= 2*v.x*v.z + 2*w*v.y;

	//row2
	rm[1]= 2*v.x*v.y + 2*w*v.z;
	rm[4]= 1.0f - 2*v.x*v.x - 2*v.z*v.z;
	rm[7]= 2*v.y*v.z - 2*w*v.x;

	//row3
	rm[2]= 2*v.x*v.z - 2*w*v.y;
	rm[5]= 2*v.y*v.z + 2*w*v.x;
	rm[8]= 1.0f - 2*v.x*v.x - 2*v.y*v.y;

	return rm;
}

Matrix4f Quaternion::toMatrix4() const{
	Matrix4f rm;

	//row1
	rm[0]= 1.0f - 2*v.y*v.y - 2*v.z*v.z;
	rm[4]= 2*v.x*v.y - 2*w*v.z;
	rm[8]= 2*v.x*v.z + 2*w*v.y;
	rm[12]= 0.0f;

	//row2
	rm[1]= 2*v.x*v.y + 2*w*v.z;
	rm[5]= 1.0f - 2*v.x*v.x - 2*v.z*v.z;
	rm[9]= 2*v.y*v.z - 2*w*v.x;
	rm[13]= 0.0f;

	//row3
	rm[2]= 2*v.x*v.z - 2*w*v.y;
	rm[6]= 2*v.y*v.z + 2*w*v.x;
	rm[10]= 1.0f - 2*v.x*v.x - 2*v.y*v.y;
	rm[14]= 0.0f;

	//row4
	rm[3]= 0.0f; 
	rm[7]= 0.0f;
	rm[11]= 0.0f;
	rm[15]= 1.0f;

	return rm;
}

Vec3f Quaternion::getLocalXAxis() const{
	return Vec3f(	
		1.0f - 2*v.y*v.y - 2*v.z*v.z,
		2*v.x*v.y + 2*w*v.z,
		2*v.x*v.z - 2*w*v.y);
}

Vec3f Quaternion::getLocalYAxis() const{
	return Vec3f(	
		2*v.x*v.y - 2*w*v.z,
		1.0f - 2*v.x*v.x - 2*v.z*v.z,
		2*v.y*v.z + 2*w*v.x);
}

Vec3f Quaternion::getLocalZAxis() const{
	return Vec3f(	
		2*v.x*v.z + 2*w*v.y,
		2*v.y*v.z - 2*w*v.x,
		1.0f - 2*v.x*v.x - 2*v.y*v.y);
}

}}//end namespace
