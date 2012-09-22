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

#include <cstdlib>

#include "config.h"
#include "game_constants.h"
#include "xml_parser.h"
#include "conversion.h"
#include "leak_dumper.h"


using namespace Shared::Graphics;
using Shared::Xml::XmlNode;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class GameCamera
// =====================================================

//static std::map<float, std::map<float, std::map<Vec3f, Quad2i> > > cacheVisibleQuad;

// ================== PUBLIC =====================

const float GameCamera::startingVAng= -60.f;
const float GameCamera::startingHAng= 0.f;
const float GameCamera::vTransitionMult= 0.125f;
const float GameCamera::hTransitionMult= 0.125f;
const float GameCamera::defaultHeight= 20.f;
const float GameCamera::centerOffsetZ= 8.0f;

// ================= Constructor =================

GameCamera::GameCamera() : pos(0.f, defaultHeight, 0.f),
		destPos(0.f, defaultHeight, 0.f), destAng(startingVAng, startingHAng) {
	Config &config = Config::getInstance();
	calculatedDefault=defaultHeight;
    state= sGame;

    cacheVisibleQuad.clear();
    MaxVisibleQuadItemCache = config.getInt("MaxVisibleQuadItemCache",intToStr(-1).c_str());
    if(Config::getInstance().getBool("DisableCaching","false") == true) {
    	MaxVisibleQuadItemCache = 0;
    }

	//config
	speed= 15.f / GameConstants::cameraFps;
	clampBounds= !Config::getInstance().getBool("PhotoMode");
	clampDisable = false;

	vAng= startingVAng;
    hAng= startingHAng;

    rotate=0;

	move= Vec3f(0.f);

	//maxRenderDistance = Config::getInstance().getFloat("RenderDistanceMax","64");
	maxHeight = Config::getInstance().getFloat("CameraMaxDistance","20");
	minHeight = Config::getInstance().getFloat("CameraMinDistance","7");
	//maxCameraDist = maxHeight;
	//minCameraDist = minHeight;
	minVAng = -Config::getInstance().getFloat("CameraMaxYaw","77.5");
	maxVAng = -Config::getInstance().getFloat("CameraMinYaw","20");
	fov = Config::getInstance().getFloat("CameraFov","45");

	lastHAng=0;
	lastVAng=0;
	limitX=0;
	limitY=0;
}

GameCamera::~GameCamera() {
	cacheVisibleQuad.clear();
}

std::string GameCamera::getCameraMovementKey() const {
	char szBuf[1024]="";
	sprintf(szBuf,"%s_%f_%f_%f_%s,%f",pos.getString().c_str(),hAng,vAng,rotate,move.getString().c_str(),fov);
	return szBuf;
}

void GameCamera::setMaxHeight(float value) {
	if(value < 0) {
		maxHeight = Config::getInstance().getFloat("CameraMaxDistance","20");
	}
	else {
		maxHeight = value;
	}
}

void GameCamera::setCalculatedDefault(float calculatedDefault){
	this->calculatedDefault= calculatedDefault;
	if(maxHeight>0 && maxHeight<calculatedDefault){
		setMaxHeight(calculatedDefault);
	}
	resetPosition();
}

void GameCamera::init(int limitX, int limitY){
	this->limitX= limitX;
	this->limitY= limitY;
}

// ==================== Misc =====================

void GameCamera::setPos(Vec2f pos){
	this->pos= Vec3f(pos.x, this->pos.y, pos.y);
	clampPosXZ(0.0f, (float)limitX, 0.0f, (float)limitY);
	destPos.x = pos.x;
	destPos.z = pos.y;
}

void GameCamera::setPos(Vec3f pos){
	this->pos= pos;
	//clampPosXZ(0.0f, (float)limitX, 0.0f, (float)limitY);
	destPos.x = pos.x;
	destPos.y = pos.y;
	destPos.z = pos.z;
}

void GameCamera::update(){

	//move XZ
	if(move.z){
        moveForwardH(speed * move.z, 0.9f);
	}
	if(move.x){
        moveSideH(speed * move.x, 0.9f);
	}

	//free state
	if(state==sFree){
#ifdef USE_STREFLOP
		if(streflop::fabs(static_cast<streflop::Simple>(rotate)) == 1){
#else
		if(fabs(rotate) == 1){
#endif
			rotateHV(speed*5*rotate, 0);
		}
		if(move.y>0){
			moveUp(speed * move.y);
			if(clampDisable == false && clampBounds && pos.y<maxHeight){
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
		if(move.y<0){
			moveUp(speed * move.y);
			if(clampDisable == false && clampBounds && pos.y>minHeight){
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
	}

	//game state
	if(abs(destAng.x - vAng) > 0.01f) {
		vAng+= (destAng.x - vAng) * hTransitionMult;
	}
	if(abs(destAng.y - hAng) > 0.01f) {
		if(abs(destAng.y - hAng) > 180) {
			if(destAng.y > hAng) {
				hAng+= (destAng.y - hAng - 360) * vTransitionMult;
			} else {
				hAng+= (destAng.y - hAng + 360) * vTransitionMult;
			}
		} else {
			hAng+= (destAng.y - hAng) * vTransitionMult;
		}
	}
	if(abs(destPos.x - pos.x) > 0.01f) {
		pos.x += (destPos.x - pos.x) / 32.0f;
	}
	if(abs(destPos.y - pos.y) > 0.01f) {
		pos.y += (destPos.y - pos.y) / 32.0f;
	}
	if(abs(destPos.z - pos.z) > 0.01f) {
		pos.z += (destPos.z - pos.z) / 32.0f;
	}

	clampAng();

	if(clampDisable == false && clampBounds){
		clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
	}
}

Quad2i GameCamera::computeVisibleQuad() {
	//printf("\n@@@ hAng [%f] vAng [%f] fov [%f]\n",hAng,vAng,fov);

	if(MaxVisibleQuadItemCache != 0) {
		std::map<float, std::map<float, std::map<Vec3f, Quad2i> > >::const_iterator iterFind = cacheVisibleQuad.find(fov);
		if(iterFind != cacheVisibleQuad.end()) {
			std::map<float, std::map<Vec3f, Quad2i> >::const_iterator iterFind2 = iterFind->second.find(hAng);
			if(iterFind2 != iterFind->second.end()) {
				std::map<Vec3f, Quad2i>::const_iterator iterFind3 = iterFind2->second.find(pos);
				if(iterFind3 != iterFind2->second.end()) {
					return iterFind3->second;
				}
			}
		}
	}

	//const float nearDist= 20.f;
	//const float farDist= 90.f;
	//const float dist= 20.f;

//	float nearDist = 20.f;
//	float dist = pos.y > 20.f ? pos.y * 1.2f : 20.f;
//	float farDist = 90.f * (pos.y > 20.f ? pos.y / 15.f : 1.f);
	float nearDist = 15.f;
	float dist = pos.y > nearDist ? pos.y * 1.2f : nearDist;
	float farDist = 90.f * (pos.y > nearDist ? pos.y / 15.f : 1.f);
	const float viewDegree = 180.f;

#ifdef USE_STREFLOP
	Vec2f v(streflop::sinf(static_cast<streflop::Simple>(degToRad(viewDegree - hAng))), streflop::cosf(static_cast<streflop::Simple>(degToRad(viewDegree - hAng))));
	Vec2f v1(streflop::sinf(static_cast<streflop::Simple>(degToRad(viewDegree - hAng - fov))), streflop::cosf(static_cast<streflop::Simple>(degToRad(viewDegree - hAng - fov))));
	Vec2f v2(streflop::sinf(static_cast<streflop::Simple>(degToRad(viewDegree - hAng + fov))), streflop::cosf(static_cast<streflop::Simple>(degToRad(viewDegree - hAng + fov))));
#else
	Vec2f v(sinf(degToRad(viewDegree - hAng)), cosf(degToRad(viewDegree - hAng)));
	Vec2f v1(sinf(degToRad(viewDegree - hAng - fov)), cosf(degToRad(viewDegree - hAng - fov)));
	Vec2f v2(sinf(degToRad(viewDegree - hAng + fov)), cosf(degToRad(viewDegree - hAng + fov)));
#endif

	v.normalize();
	v1.normalize();
	v2.normalize();

	Vec2f p = Vec2f(pos.x, pos.z) - v * dist;
	Vec2i p1(static_cast<int>(p.x + v1.x * nearDist), static_cast<int>(p.y + v1.y * nearDist));
	Vec2i p2(static_cast<int>(p.x + v1.x * farDist), static_cast<int>(p.y + v1.y * farDist));
	Vec2i p3(static_cast<int>(p.x + v2.x * nearDist), static_cast<int>(p.y + v2.y * nearDist));
	Vec2i p4(static_cast<int>(p.x + v2.x * farDist), static_cast<int>(p.y + v2.y * farDist));

	const bool debug = false;

	Quad2i result;
	if (hAng >= 135 && hAng <= 225) {
		if(debug) printf("Line %d  hAng [%f] fov [%f]\n",__LINE__,hAng,fov);

		result = Quad2i(p1, p2, p3, p4);
		if(MaxVisibleQuadItemCache != 0 &&
		   (MaxVisibleQuadItemCache < 0 || cacheVisibleQuad[fov][hAng].size() <= MaxVisibleQuadItemCache)) {
			cacheVisibleQuad[fov][hAng][pos] = result;
		}
	}
	else if (hAng >= 45 && hAng <= 135) {
		if(debug) printf("Line %d  hAng [%f] fov [%f]\n",__LINE__,hAng,fov);

		result = Quad2i(p3, p1, p4, p2);
		if(MaxVisibleQuadItemCache != 0 &&
		   (MaxVisibleQuadItemCache < 0 || cacheVisibleQuad[fov][hAng].size() <= MaxVisibleQuadItemCache)) {
			cacheVisibleQuad[fov][hAng][pos] = result;
		}
	}
	else if (hAng >= 225 && hAng <= 315) {
		if(debug) printf("Line %d  hAng [%f] fov [%f]\n",__LINE__,hAng,fov);

		result = Quad2i(p2, p4, p1, p3);
		if(MaxVisibleQuadItemCache != 0 &&
		   (MaxVisibleQuadItemCache < 0 || cacheVisibleQuad[fov][hAng].size() <= MaxVisibleQuadItemCache)) {
			cacheVisibleQuad[fov][hAng][pos] = result;
		}
	}
	else {
		if(debug) printf("Line %d  hAng [%f] fov [%f]\n",__LINE__,hAng,fov);

		result = Quad2i(p4, p3, p2, p1);
		if(MaxVisibleQuadItemCache != 0 &&
		   (MaxVisibleQuadItemCache < 0 || cacheVisibleQuad[fov][hAng].size() <= MaxVisibleQuadItemCache)) {
			cacheVisibleQuad[fov][hAng][pos] = Quad2i(p4, p3, p2, p1);
		}
	}

	return result;
}

void GameCamera::switchState(){
	if(state==sGame){
		state= sFree;
	}
	else{
		state= sGame;
		destAng.x = startingVAng;
		destAng.y = startingHAng;
		destPos.y = calculatedDefault;
	}
}

void GameCamera::resetPosition(){
	state= sGame;
	destAng.x = startingVAng;
	destAng.y = startingHAng;
	destPos.y = calculatedDefault;
}

void GameCamera::centerXZ(float x, float z){
	destPos.x = pos.x= x;
	destPos.z = pos.z= z+centerOffsetZ;
}

void GameCamera::transitionXYZ(float x, float y, float z) {
	destPos.x += x;
	destPos.y += y;
	destPos.z += z;
	clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
}

void GameCamera::transitionVH(float v, float h) {
	destAng.x -= v;
	//destPos.y -= v * destPos.y / 100.f;
	destAng.y -= h;
	clampAng();
}

void GameCamera::zoom(float dist) {
#ifdef USE_STREFLOP
	float flatDist = dist * streflop::cosf(static_cast<streflop::Simple>(degToRad(vAng)));
	Vec3f offset(flatDist * streflop::sinf(static_cast<streflop::Simple>(degToRad(hAng))), dist * streflop::sinf(static_cast<streflop::Simple>(degToRad(vAng))), flatDist  * -streflop::cosf(static_cast<streflop::Simple>(degToRad(hAng))));
#else
	float flatDist = dist * cosf(degToRad(vAng));
	Vec3f offset(flatDist * sinf(degToRad(hAng)), dist * sinf(degToRad(vAng)), flatDist  * -cosf(degToRad(hAng)));
#endif

	destPos += offset;
}

void GameCamera::load(const XmlNode *node) {
	//destPos = node->getChildVec3fValue("pos");
	//destAng = node->getChildVec2fValue("angle");
}

void GameCamera::save(XmlNode *node) const {
	//node->addChild("pos", pos);
	//node->addChild("angle", Vec2f(vAng, hAng));
}

// ==================== PRIVATE ====================

void GameCamera::clampPosXZ(float x1, float x2, float z1, float z2){
	if(clampDisable == true) {
		return;
	}

	if(pos.x < x1)		pos.x = x1;
	if(destPos.x < x1)	destPos.x = x1;
	if(pos.z < z1)		pos.z = z1;
	if(destPos.z < z1)	destPos.z = z1;
	if(pos.x > x2)		pos.x = x2;
	if(destPos.x > x2)	destPos.x = x2;
	if(pos.z > z2)		pos.z = z2;
	if(destPos.z > z2)	destPos.z = z2;
}

void GameCamera::clampPosXYZ(float x1, float x2, float y1, float y2, float z1, float z2){
	if(clampDisable == true) {
		return;
	}

	if(pos.x < x1)		pos.x = x1;
	if(destPos.x < x1)	destPos.x = x1;
	if(pos.y < y1)		pos.y = y1;
	if(destPos.y < y1)	destPos.y = y1;
	if(pos.z < z1)		pos.z = z1;
	if(destPos.z < z1)	destPos.z = z1;
	if(pos.x > x2)		pos.x = x2;
	if(destPos.x > x2)	destPos.x = x2;
	if(pos.y > y2)		pos.y = y2;
	if(destPos.y > y2)	destPos.y = y2;
	if(pos.z > z2)		pos.z = z2;
	if(destPos.z > z2)	destPos.z = z2;
}

void GameCamera::rotateHV(float h, float v){
	destAng.x = vAng += v;
	destAng.y = hAng += h;
	clampAng();
}

void GameCamera::clampAng() {
	if(clampDisable == true) {
		return;
	}

	if(vAng > maxVAng)		vAng = maxVAng;
	if(destAng.x > maxVAng)	destAng.x = maxVAng;
	if(vAng < minVAng)		vAng = minVAng;
	if(destAng.x < minVAng)	destAng.x = minVAng;
	if(hAng > 360.f)		hAng -= 360.f;
	if(destAng.y > 360.f)	destAng.y -= 360.f;
	if(hAng < 0.f)			hAng += 360.f;
	if(destAng.y < 0.f)		destAng.y = 360.f;
}

//move camera forwad but never change heightFactor
void GameCamera::moveForwardH(float d, float response) {
#ifdef USE_STREFLOP
	Vec3f offset(streflop::sinf(static_cast<streflop::Simple>(degToRad(hAng))) * d, 0.f, -streflop::cosf(static_cast<streflop::Simple>(degToRad(hAng))) * d);
#else
	Vec3f offset(sinf(degToRad(hAng)) * d, 0.f, -cosf(degToRad(hAng)) * d);
#endif
	destPos += offset;
	pos.x += offset.x * response;
	pos.z += offset.z * response;
}

//move camera to a side but never change heightFactor
void GameCamera::moveSideH(float d, float response){
#ifdef USE_STREFLOP
	Vec3f offset(streflop::sinf(static_cast<streflop::Simple>(degToRad(hAng+90))) * d, 0.f, -streflop::cosf(static_cast<streflop::Simple>(degToRad(hAng+90))) * d);
#else
	Vec3f offset(sinf(degToRad(hAng+90)) * d, 0.f, -cosf(degToRad(hAng+90)) * d);
#endif
	destPos += offset;
	pos.x += (destPos.x - pos.x) * response;
	pos.z += (destPos.z - pos.z) * response;
}

void GameCamera::moveUp(float d){
//	pos.y+= d;
	destPos.y += d;
}

void GameCamera::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *gamecameraNode = rootNode->addChild("GameCamera");

//	Vec3f pos;
	gamecameraNode->addAttribute("pos",pos.getString(), mapTagReplacements);
//	Vec3f destPos;
	gamecameraNode->addAttribute("destPos",destPos.getString(), mapTagReplacements);
//
//    float hAng;	//YZ plane positive -Z axis
	gamecameraNode->addAttribute("hAng",floatToStr(hAng), mapTagReplacements);
//    float vAng;	//XZ plane positive +Z axis
	gamecameraNode->addAttribute("vAng",floatToStr(vAng), mapTagReplacements);
//	float lastHAng;
	gamecameraNode->addAttribute("lastHAng",floatToStr(lastHAng), mapTagReplacements);

//    float lastVAng;
	gamecameraNode->addAttribute("lastVAng",floatToStr(lastVAng), mapTagReplacements);
//	Vec2f destAng;
	gamecameraNode->addAttribute("destAng",destAng.getString(), mapTagReplacements);
//	float rotate;
	gamecameraNode->addAttribute("rotate",floatToStr(rotate), mapTagReplacements);
//	Vec3f move;
	gamecameraNode->addAttribute("move",move.getString(), mapTagReplacements);
//	State state;
	gamecameraNode->addAttribute("state",intToStr(state), mapTagReplacements);
//	int limitX;
	gamecameraNode->addAttribute("limitX",intToStr(limitX), mapTagReplacements);
//	int limitY;
	gamecameraNode->addAttribute("limitY",intToStr(limitY), mapTagReplacements);
//	//config
//	float speed;
	gamecameraNode->addAttribute("speed",floatToStr(speed), mapTagReplacements);
//	bool clampBounds;
	gamecameraNode->addAttribute("clampBounds",intToStr(clampBounds), mapTagReplacements);
//	//float maxRenderDistance;
//	float maxHeight;
	gamecameraNode->addAttribute("maxHeight",floatToStr(maxHeight), mapTagReplacements);
//	float minHeight;
	gamecameraNode->addAttribute("minHeight",floatToStr(minHeight), mapTagReplacements);
//	//float maxCameraDist;
//	//float minCameraDist;
//	float minVAng;
	gamecameraNode->addAttribute("minVAng",floatToStr(minVAng), mapTagReplacements);
//	float maxVAng;
	gamecameraNode->addAttribute("maxVAng",floatToStr(maxVAng), mapTagReplacements);
//	float fov;
	gamecameraNode->addAttribute("fov",floatToStr(fov), mapTagReplacements);
//	float calculatedDefault;
	gamecameraNode->addAttribute("calculatedDefault",floatToStr(calculatedDefault), mapTagReplacements);
//	std::map<float, std::map<float, std::map<Vec3f, Quad2i> > > cacheVisibleQuad;
//	int MaxVisibleQuadItemCache;
	gamecameraNode->addAttribute("MaxVisibleQuadItemCache",intToStr(MaxVisibleQuadItemCache), mapTagReplacements);
}

void GameCamera::loadGame(const XmlNode *rootNode) {
	const XmlNode *gamecameraNode = rootNode->getChild("GameCamera");

	//firstTime = timeflowNode->getAttribute("firstTime")->getFloatValue();

	//	Vec3f pos;
	pos = Vec3f::strToVec3(gamecameraNode->getAttribute("pos")->getValue());
	//	Vec3f destPos;
	destPos = Vec3f::strToVec3(gamecameraNode->getAttribute("destPos")->getValue());
	//
	//    float hAng;	//YZ plane positive -Z axis
	hAng = gamecameraNode->getAttribute("hAng")->getFloatValue();
	//    float vAng;	//XZ plane positive +Z axis
	vAng = gamecameraNode->getAttribute("vAng")->getFloatValue();
	//	float lastHAng;
	lastHAng = gamecameraNode->getAttribute("lastHAng")->getFloatValue();

	//    float lastVAng;
	lastVAng = gamecameraNode->getAttribute("lastVAng")->getFloatValue();
	//	Vec2f destAng;
	destAng = Vec2f::strToVec2(gamecameraNode->getAttribute("destAng")->getValue());
	//	float rotate;
	rotate = gamecameraNode->getAttribute("rotate")->getFloatValue();
	//	Vec3f move;
	move = Vec3f::strToVec3(gamecameraNode->getAttribute("move")->getValue());
	//	State state;
	state = static_cast<State>(gamecameraNode->getAttribute("state")->getIntValue());
	//	int limitX;
	limitX = gamecameraNode->getAttribute("limitX")->getIntValue();
	//	int limitY;
	limitY = gamecameraNode->getAttribute("limitY")->getIntValue();
	//	//config
	//	float speed;
	speed = gamecameraNode->getAttribute("speed")->getFloatValue();
	//	bool clampBounds;
	clampBounds = gamecameraNode->getAttribute("clampBounds")->getIntValue() != 0;
	//	//float maxRenderDistance;
	//	float maxHeight;
	maxHeight = gamecameraNode->getAttribute("maxHeight")->getFloatValue();
	//	float minHeight;
	minHeight = gamecameraNode->getAttribute("minHeight")->getFloatValue();
	//	//float maxCameraDist;
	//	//float minCameraDist;
	//	float minVAng;
	minVAng = gamecameraNode->getAttribute("minVAng")->getFloatValue();
	//	float maxVAng;
	maxVAng = gamecameraNode->getAttribute("maxVAng")->getFloatValue();
	//	float fov;
	fov = gamecameraNode->getAttribute("fov")->getFloatValue();
	//	float calculatedDefault;
	calculatedDefault = gamecameraNode->getAttribute("calculatedDefault")->getFloatValue();
	//	std::map<float, std::map<float, std::map<Vec3f, Quad2i> > > cacheVisibleQuad;
	//	int MaxVisibleQuadItemCache;
	MaxVisibleQuadItemCache = gamecameraNode->getAttribute("MaxVisibleQuadItemCache")->getIntValue();

}

}}//end namespace
