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

#include "camera.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class Camera
// =====================================================

Camera::Camera(){
	position= Vec3f(0.0f);
}

void Camera::moveLocalX(float amount){
	position= position + orientation.getLocalXAxis()*amount;
}

void Camera::moveLocalY(float amount){
	position= position + orientation.getLocalYAxis()*amount;
}

void Camera::moveLocalZ(float amount){
	position= position + orientation.getLocalZAxis()*amount;
}

void Camera::addYaw(float amount){
	Quaternion q(EulerAngles(0, amount, 0));
	orientation*= q;
}

void Camera::addPitch(float amount){
	Quaternion q(EulerAngles(amount, 0, 0));
	orientation*= q;
}

void Camera::addRoll(float amount){
	Quaternion q(EulerAngles(0, 0, amount));
	orientation*= q;
}

}}//end namespace
