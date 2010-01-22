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

#ifndef _SHARED_GRAPHICS_PARTICLE_H_
#define _SHARED_GRAPHICS_PARTICLE_H_

#include <list>
#include <cassert>

#include "vec.h"
#include "pixmap.h"
#include "texture_manager.h"
#include "random.h"

using std::list;
using Shared::Util::Random;

namespace Shared{ namespace Graphics{

class ParticleSystem;
class FireParticleSystem;
class RainParticleSystem;
class SnowParticleSystem;
class ProjectileParticleSystem;
class SplashParticleSystem;
class ParticleRenderer;
class ModelRenderer;
class Model;

// =====================================================
//	class Particle
// =====================================================

class Particle{
public:	
	//attributes
	Vec3f pos;
	Vec3f lastPos;
	Vec3f speed;
	Vec3f accel;
	Vec4f color;
	float size;
	int energy;

public:
	//get
	Vec3f getPos() const		{return pos;}
	Vec3f getLastPos() const	{return lastPos;}
	Vec3f getSpeed() const		{return speed;}
	Vec3f getAccel() const		{return accel;}
	Vec4f getColor() const		{return color;}
	float getSize() const		{return size;}
	int getEnergy()	const		{return energy;}
};

class ParticleSystem;

// =====================================================
//	class ParticleObserver
// =====================================================

class ParticleObserver{
public:
	virtual ~ParticleObserver(){};
	virtual void update(ParticleSystem *particleSystem)= 0;
};

// =====================================================
//	class ParticleSystem
// =====================================================

class ParticleSystem{
public:
	enum BlendMode{
		bmOne,
		bmOneMinusAlpha
	};

protected:
	enum State{
		sPause,	// No updates
		sPlay,	
		sFade	// No new particles
	};

protected:
	
	Particle *particles;
	Random random;

	BlendMode blendMode;
	State state;
	bool active;
	bool visible;
	int aliveParticleCount;
	int particleCount;
	

	Texture *texture;
	Vec3f pos;
	Vec4f color;
	Vec4f colorNoEnergy;
	int emissionRate;
	int maxParticleEnergy;
	int varParticleEnergy;
	float particleSize;
	float speed;

	ParticleObserver *particleObserver;

public:
	//conmstructor and destructor
	ParticleSystem(int particleCount);
	virtual ~ParticleSystem();

	//public
	virtual void update();
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	//get
	State getState() const						{return state;}
	BlendMode getBlendMode() const				{return blendMode;}
	Texture *getTexture() const					{return texture;}
	Vec3f getPos() const						{return pos;}
	Particle *getParticle(int i)				{return &particles[i];}
	const Particle *getParticle(int i) const	{return &particles[i];}
	int getAliveParticleCount() const			{return aliveParticleCount;}
	bool getActive() const						{return active;}
	bool getVisible() const						{return visible;}

	//set
	void setState(State state);
	void setTexture(Texture *texture);
	void setPos(Vec3f pos);
	void setColor(Vec4f color);
	void setColorNoEnergy(Vec4f color);
	void setEmissionRate(int emissionRate);
	void setMaxParticleEnergy(int maxParticleEnergy);
	void setVarParticleEnergy(int varParticleEnergy);
	void setParticleSize(float particleSize);
	void setSpeed(float speed);
	void setActive(bool active);
	void setObserver(ParticleObserver *particleObserver);
	void setVisible(bool visible);

	//misc
	void fade();
	int isEmpty() const;

protected:
	//protected
	Particle *createParticle();
	void killParticle(Particle *p);

	//virtual protected
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	virtual bool deathTest(Particle *p);
};

// =====================================================
//	class FireParticleSystem
// =====================================================

class FireParticleSystem: public ParticleSystem{
private:
	float radius;
	Vec3f windSpeed;

public:
	FireParticleSystem(int particleCount= 2000);

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	//set params
	void setRadius(float radius);
	void setWind(float windAngle, float windSpeed);
};

// =====================================================
//	class RainParticleSystem
// =====================================================

class RainParticleSystem: public ParticleSystem{
private:
	Vec3f windSpeed;
	float radius;

public:
	RainParticleSystem(int particleCount= 4000);

	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	virtual void initParticle(Particle *p, int particleIndex);
	virtual bool deathTest(Particle *p);

	void setRadius(float radius);
	void setWind(float windAngle, float windSpeed);	
};

// =====================================================
//	class SnowParticleSystem
// =====================================================

class SnowParticleSystem: public ParticleSystem{
private:
	Vec3f windSpeed;
	float radius;

public:
	SnowParticleSystem(int particleCount= 4000);

	virtual void initParticle(Particle *p, int particleIndex);
	virtual bool deathTest(Particle *p);

	void setRadius(float radius);
	void setWind(float windAngle, float windSpeed);	
};

// ===========================================================================
//  AttackParticleSystem
//
/// Base class for Projectiles and Splashes
// ===========================================================================

class AttackParticleSystem: public ParticleSystem{
public:
	enum Primitive{
		pQuad,
		pLine,
		pLineAlpha
	};

protected:
	Model *model;
	Primitive primitive;
	Vec3f offset;
	float sizeNoEnergy;
	float gravity;
	
	Vec3f direction;

public:
	AttackParticleSystem(int particleCount);

	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	Model *getModel() const			{return model;}
	Vec3f getDirection() const		{return direction;}

	void setModel(Model *model)					{this->model= model;}
	void setOffset(Vec3f offset)				{this->offset= offset;}
	void setSizeNoEnergy(float sizeNoEnergy)	{this->sizeNoEnergy= sizeNoEnergy;}
	void setGravity(float gravity)				{this->gravity= gravity;}
	void setPrimitive(Primitive primitive)		{this->primitive= primitive;}

	static Primitive strToPrimitive(const string &str);
};

// =====================================================
//	class ProjectileParticleSystem
// =====================================================

class ProjectileParticleSystem: public AttackParticleSystem{
public:
	friend class SplashParticleSystem;

	enum Trajectory{
		tLinear,
		tParabolic,
		tSpiral
	};

private:
	SplashParticleSystem *nextParticleSystem;

	Vec3f lastPos;
	Vec3f startPos;
	Vec3f endPos;
	Vec3f flatPos;

	Vec3f xVector;
	Vec3f yVector;
	Vec3f zVector;

	Trajectory trajectory;
	float trajectorySpeed;

	//parabolic
	float trajectoryScale;
	float trajectoryFrequency;

public:
	ProjectileParticleSystem(int particleCount= 1000);
	virtual ~ProjectileParticleSystem();

	void link(SplashParticleSystem *particleSystem);
	
	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	
	void setTrajectory(Trajectory trajectory)				{this->trajectory= trajectory;}
	void setTrajectorySpeed(float trajectorySpeed)			{this->trajectorySpeed= trajectorySpeed;}
	void setTrajectoryScale(float trajectoryScale)			{this->trajectoryScale= trajectoryScale;}
	void setTrajectoryFrequency(float trajectoryFrequency)	{this->trajectoryFrequency= trajectoryFrequency;}
	void setPath(Vec3f startPos, Vec3f endPos);

	static Trajectory strToTrajectory(const string &str);
};

// =====================================================
//	class SplashParticleSystem
// =====================================================

class SplashParticleSystem: public AttackParticleSystem{
public:
	friend class ProjectileParticleSystem;

private:
	ProjectileParticleSystem *prevParticleSystem;

	int emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;

public:
	SplashParticleSystem(int particleCount= 1000);
	virtual ~SplashParticleSystem();
	
	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	void setEmissionRateFade(int emissionRateFade)		{this->emissionRateFade= emissionRateFade;}
	void setVerticalSpreadA(float verticalSpreadA)		{this->verticalSpreadA= verticalSpreadA;}
	void setVerticalSpreadB(float verticalSpreadB)		{this->verticalSpreadB= verticalSpreadB;}
	void setHorizontalSpreadA(float horizontalSpreadA)	{this->horizontalSpreadA= horizontalSpreadA;}
	void setHorizontalSpreadB(float horizontalSpreadB)	{this->horizontalSpreadB= horizontalSpreadB;}
	
};

// =====================================================
//	class ParticleManager
// =====================================================

class ParticleManager{
private:
	list<ParticleSystem*> particleSystems; 

public:
	~ParticleManager();
	void update();
	void render(ParticleRenderer *pr, ModelRenderer *mr) const;	
	void manage(ParticleSystem *ps);
	void end();
}; 

}}//end namespace

#endif
