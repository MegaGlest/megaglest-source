// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GAMECAMERA_H_
#define _GLEST_GAME_GAMECAMERA_H_

#include "vec.h"
#include "math_util.h"
#include <map>
#include <string>
#include "leak_dumper.h"

namespace Shared { namespace Xml {
	class XmlNode;
}}

namespace Glest{ namespace Game{

using Shared::Graphics::Quad2i;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2f;
using Shared::Xml::XmlNode;

class Config;

// =====================================================
// 	class GameCamera
//
/// A basic camera that holds information about the game view
// =====================================================

class GameCamera {
public:
	static const float startingVAng;
	static const float startingHAng;
	static const float vTransitionMult;
	static const float hTransitionMult;
	static const float defaultHeight;
	static const float centerOffsetZ;

public:
	enum State{
		sGame,
		sFree
	};

private:
	Vec3f pos;
	Vec3f destPos;

    float hAng;	//YZ plane positive -Z axis
    float vAng;	//XZ plane positive +Z axis
	float lastHAng;
    float lastVAng;
	Vec2f destAng;

	float rotate;

	Vec3f move;

	State state;

	int limitX;
	int limitY;

	//config
	float speed;
	bool clampBounds;
	//float maxRenderDistance;
	float maxHeight;
	float minHeight;
	//float maxCameraDist;
	//float minCameraDist;
	float minVAng;
	float maxVAng;
	float fov;

	int MaxVisibleQuadItemCache;

public:
    GameCamera();
    ~GameCamera();

	void init(int limitX, int limitY);

    //get
	float getHAng() const		{return hAng;};
	float getVAng() const		{return vAng;}
	State getState() const		{return state;}
	const Vec3f &getPos() const	{return pos;}

    //set
	void setRotate(float rotate){this->rotate= rotate;}
	void setPos(Vec2f pos);

	void setMoveX(float f)		{this->move.x= f;}
	void setMoveY(float f)		{this->move.y= f;}
	void setMoveZ(float f)		{this->move.z= f;}

	void stop() {
		destPos = pos;
		destAng.x = vAng;
		destAng.y = hAng;
	}

    //other
    void update();
    Quad2i computeVisibleQuad() const;
	void switchState();
	void resetPosition();

	void centerXZ(float x, float z);
	void rotateHV(float h, float v);
	void transitionXYZ(float x, float y, float z);
	void transitionVH(float v, float h);
	void zoom(float dist);
	void moveForwardH(float dist, float response);	// response: 1.0 for immediate, 0 for full inertia
	void moveSideH(float dist, float response);

	void load(const XmlNode *node);
	void save(XmlNode *node) const;

	void setClampBounds(bool value) { clampBounds = value; }
	void setMaxHeight(float value);
	void setFov(float value) { fov = value; }
	void setMinVAng(float value) { minVAng = value; }
	void setMaxVAng(float value) { maxVAng = value; }

private:
	void clampPosXYZ(float x1, float x2, float y1, float y2, float z1, float z2);
	void clampPosXZ(float x1, float x2, float z1, float z2);
	void clampAng();
	void moveUp(float dist);
};

}} //end namespace

#endif
