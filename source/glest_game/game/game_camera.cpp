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

#include "game_camera.h"

#include "config.h"
#include "game_constants.h"
#include "metrics.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class GameCamera
// =====================================================

// ================== PUBLIC =====================

const float GameCamera::startingVAng= -60.f;
const float GameCamera::startingHAng= 0.f;
const float GameCamera::maxHeight= 20.f;
const float GameCamera::minHeight= 10.f;
const float GameCamera::transitionSpeed= 0.01f;
const float GameCamera::centerOffsetZ= 8.0f;

// ================= Constructor =================

GameCamera::GameCamera(){
    this->pos= Vec3f(0.f, maxHeight, 0.f);
    state= sGame;

	//config
	speed= 15.f / GameConstants::cameraFps;
	clampBounds= !Config::getInstance().getBool("PhotoMode");
    
	vAng= startingVAng;
    hAng= startingHAng;
    rotate=0;
	stateTransition= 1.f;

	move= Vec3f(0.f);
	stopMove= Vec3b(false);
}

void GameCamera::init(int limitX, int limitY){
	this->limitX= limitX;
	this->limitY= limitY;
}

// ==================== Misc =====================

void GameCamera::setPos(Vec2f pos){
	this->pos= Vec3f(pos.x, this->pos.y, pos.y);
	clampPosXZ(0.0f, limitX, 0.0f, limitY);
}

void GameCamera::update(){
    
	//move speed XYZ
	if(stopMove.x){
		move.x *= 0.9f;
	}
	if(stopMove.y){
		move.y *= 0.9f;
	}
	if(stopMove.z){
		move.z *= 0.9f;
	}

	//move XZ
	if(move.z!=0){
        moveForwardH(speed*move.z);
	}
	if(move.x!=0){
        moveSideH(speed*move.x);
	}

	//free state
	if(state==sFree){
		if(fabs(rotate) == 1){
			rotateHV(speed*5*rotate, 0);
		}
		if(move.y>0){
			moveUp(speed * move.y);
			if(clampBounds && pos.y<maxHeight){ 
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
		if(move.y<0){
			moveUp(speed * move.y);
			if(clampBounds && pos.y>minHeight){
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
	}

	//game state
	else if(stateTransition<1.f){
		if(lastHAng<180){
			hAng= lastHAng + (stateTransition)*(startingHAng-lastHAng);
		}
		else{
			hAng= lastHAng + (stateTransition)*(startingHAng+360-lastHAng);
		}
		vAng= lastVAng*(1.f-stateTransition)+startingVAng*stateTransition;
		pos.y= lastPos.y+maxHeight*stateTransition;
		stateTransition+= transitionSpeed;
	}

	if(clampBounds){
		clampPosXYZ(0.0f, limitX, minHeight, maxHeight, 0.0f, limitY);
	}
}

Quad2i GameCamera::computeVisibleQuad() const{
	float aspectRatio = Metrics::getInstance().getAspectRatio();
	Vec2i v= Vec2i(static_cast<int>(pos.x), static_cast<int>(pos.z));
		
	//free state
	if(state==sFree || stateTransition<1.f){
		const float nearDist= 20.f;
		const float farDist= 90.f;
		const float fov= 65.0f * aspectRatio * 0.5f;
		const float dist= 20.f;
		
		Vec2f v(sin(degToRad(180-hAng)), cos(degToRad(180-hAng)));
		Vec2f v1(sin(degToRad(180-hAng-fov)), cos(degToRad(180-hAng-fov)));
		Vec2f v2(sin(degToRad(180-hAng+fov)), cos(degToRad(180-hAng+fov)));
		v.normalize();
		v1.normalize();
		v2.normalize();

		Vec2f p= Vec2f(pos.x, pos.z)-v*dist;
		Vec2i p1(static_cast<int>(p.x+v1.x*nearDist), static_cast<int>(p.y+v1.y*nearDist));
		Vec2i p2(static_cast<int>(p.x+v1.x*farDist), static_cast<int>(p.y+v1.y*farDist));
		Vec2i p3(static_cast<int>(p.x+v2.x*nearDist), static_cast<int>(p.y+v2.y*nearDist));
		Vec2i p4(static_cast<int>(p.x+v2.x*farDist), static_cast<int>(p.y+v2.y*farDist));

		if(hAng>=135 && hAng<=225){
			return Quad2i(p1, p2, p3, p4);
		}
		if(hAng>=45 && hAng<=135){
			return Quad2i(p3, p1, p4, p2);
		}	
		if(hAng>=225 && hAng<=315) {
			return Quad2i(p2, p4, p1, p3);
		}
		return Quad2i(p4, p3, p2, p1);

	}

	//game state
	else{
		static const int widthNear= 19 * aspectRatio;
		static const int widthFar= 23 * aspectRatio;
		static const int near= 5;
		static const int far= 40;

		return Quad2i(
			Vec2i(v.x-widthNear, v.y-far), 
			Vec2i(v.x-widthFar, v.y+near), 
			Vec2i(v.x+widthNear, v.y-far), 
			Vec2i(v.x+widthFar, v.y+near));
	}
}

void GameCamera::switchState(){
	if(state==sGame){
		state= sFree;
	}
	else{
		state= sGame;
		stateTransition= 0.f;
		lastHAng= hAng;
		lastVAng= vAng;
		lastPos= pos;
	}
}

void GameCamera::centerXZ(float x, float z){
	pos.x= x;
	pos.z= z+centerOffsetZ;
}

// ==================== PRIVATE ==================== 

void GameCamera::clampPosXZ(float x1, float x2, float z1, float z2){
	if(pos.x<x1) pos.x= static_cast<float>(x1);
	if(pos.z<z1) pos.z= static_cast<float>(z1);
	if(pos.x>x2) pos.x= static_cast<float>(x2);
	if(pos.z>z2) pos.z= static_cast<float>(z2);
}

void GameCamera::clampPosXYZ(float x1, float x2, float y1, float y2, float z1, float z2){
	if(pos.x<x1) pos.x= x1;
	if(pos.y<y1) pos.y= y1;
	if(pos.z<z1) pos.z= z1;
	if(pos.x>x2) pos.x= x2;
	if(pos.y>y2) pos.y= y2;
	if(pos.z>z2) pos.z= z2;
}

void GameCamera::rotateHV(float h, float v){
	vAng+=v;
	hAng+=h;
	if(hAng>360.f) hAng-=360.f;
	if(hAng<0.f) hAng+=360.f;
}

//move camera forwad but never change heightFactor
void GameCamera::moveForwardH(float d){
	pos=pos + Vec3f(sin(degToRad(hAng)), 0.f, -cos(degToRad(hAng))) * d;
}

//move camera to a side but never change heightFactor
void GameCamera::moveSideH(float d){
	pos=pos + Vec3f(sin(degToRad(hAng+90)), 0.f, -cos(degToRad(hAng+90))) * d;

}

void GameCamera::moveUp(float d){
	pos.y+= d;
}

}}//end namespace
