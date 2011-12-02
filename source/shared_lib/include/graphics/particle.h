// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
#include "randomgen.h"
#include "leak_dumper.h"

using std::list;
using Shared::Util::RandomGen;

namespace Shared{ namespace Graphics{

class ParticleSystem;
class FireParticleSystem;
class UnitParticleSystem;
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

class Particle {
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
	Particle() {
		size = 0;
		energy = 0;
	}
	//get
	Vec3f getPos() const		{return pos;}
	Vec3f getLastPos() const	{return lastPos;}
	Vec3f getSpeed() const		{return speed;}
	Vec3f getAccel() const		{return accel;}
	Vec4f getColor() const		{return color;}
	float getSize() const		{return size;}
	int getEnergy()	const		{return energy;}
};

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

class ParticleSystem {

public:

enum State {
	sPause,	// No updates
	sPlay,
	sFade	// No new particles
};

enum BlendMode {
	bmOne,
	bmOneMinusAlpha
};

enum ParticleSystemType {
	pst_All,
	pst_FireParticleSystem,
	pst_UnitParticleSystem,
	pst_RainParticleSystem,
	pst_SnowParticleSystem,
	pst_ProjectileParticleSystem,
	pst_SplashParticleSystem,
};

protected:
	
	std::vector<Particle> particles;
	RandomGen random;

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
	float emissionRate;
	float emissionState;
	int maxParticleEnergy;
	int varParticleEnergy;
	float particleSize;
	float speed;
	Vec3f factionColor;
    bool teamcolorNoEnergy;
    bool teamcolorEnergy;
	int alternations;
	ParticleObserver *particleObserver;

public:
	//conmstructor and destructor
	ParticleSystem(int particleCount);
	virtual ~ParticleSystem();
	virtual ParticleSystemType getParticleSystemType() const = 0;

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
	virtual bool getVisible() const				{return visible;}

	//set
	virtual void setState(State state);
	void setTexture(Texture *texture);
	virtual void setPos(Vec3f pos);
	void setColor(Vec4f color);
	void setColorNoEnergy(Vec4f color);
	void setEmissionRate(float emissionRate);
	void setMaxParticleEnergy(int maxParticleEnergy);
	void setVarParticleEnergy(int varParticleEnergy);
	void setParticleSize(float particleSize);
	void setSpeed(float speed);
	virtual void setActive(bool active);
	void setObserver(ParticleObserver *particleObserver);
	virtual void setVisible(bool visible);
	void setBlendMode(BlendMode blendMode)				{this->blendMode= blendMode;}
	void setTeamcolorNoEnergy(bool teamcolorNoEnergy)	{this->teamcolorNoEnergy= teamcolorNoEnergy;}
	void setTeamcolorEnergy(bool teamcolorEnergy)		{this->teamcolorEnergy= teamcolorEnergy;}
	void setAlternations(int alternations)				{this->alternations= alternations;}
	virtual void setFactionColor(Vec3f factionColor);

	static BlendMode strToBlendMode(const string &str);
	//misc
	virtual void fade();
	int isEmpty() const;
	
	//children
	virtual int getChildCount() { return 0; }
	virtual ParticleSystem* getChild(int i);

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

	virtual ParticleSystemType getParticleSystemType() const { return pst_FireParticleSystem;}

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	//set params
	void setRadius(float radius);
	void setWind(float windAngle, float windSpeed);
};

// =====================================================
//	class GameParticleSystem
/// base-class for unit and attack systems
// =====================================================

class UnitParticleSystem;

class GameParticleSystem: public ParticleSystem{
public:
	enum Primitive{
		pQuad,
		pLine,
		pLineAlpha
	};
	static Primitive strToPrimitive(const string &str);
	virtual ~GameParticleSystem();
	int getChildCount();
	ParticleSystem* getChild(int i);
	void addChild(UnitParticleSystem* child);
	void removeChild(UnitParticleSystem* child);
	void setPos(Vec3f pos);
	void setOffset(Vec3f offset);
	void setModel(Model *model) {this->model= model;}
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);
	float getTween() { return tween; }  // 0.0 -> 1.0 for animation of model
	Model *getModel() const {return model;}
	void setPrimitive(Primitive primitive) {this->primitive= primitive;}
	Vec3f getDirection() const {return direction;}
	void setModelCycle(float modelCycle) {this->modelCycle= modelCycle;}
protected:
	typedef std::vector<UnitParticleSystem*> Children;
	Children children;
	Primitive primitive;
	Model *model;
	float modelCycle;
	Vec3f offset;
	Vec3f direction;
	float tween;
	
	GameParticleSystem(int particleCount);
	void positionChildren();
	void setTween(float relative,float absolute);
};

// =====================================================
//	class UnitParticleSystem
// =====================================================

class UnitParticleSystem: public GameParticleSystem{
public:
	static bool isNight;
private:
	float radius;
	float minRadius;
	Vec3f windSpeed;
	Vec3f cRotation;
	Vec3f fixedAddition;
    Vec3f oldPosition;
    bool energyUp;
	float startTime;
	float endTime;
    
public:
	enum Shape{
		sLinear, // generated in a sphere, flying in direction
		sSpherical, // generated in a sphere, flying away from center
		sConical, // generated in a cone at angle from direction
	};
	bool relative;
	bool relativeDirection;
    bool fixed;
	Shape shape;
	float angle;
	float sizeNoEnergy;
	float gravity;
	float rotation;
	bool isVisibleAtNight;
	bool isVisibleAtDay;
	bool radiusBasedStartenergy;
	int staticParticleCount;
	int delay;
	int lifetime;
	float emissionRateFade;
	GameParticleSystem* parent;

public:
	UnitParticleSystem(int particleCount= 2000);
	~UnitParticleSystem();

	virtual ParticleSystemType getParticleSystemType() const { return pst_UnitParticleSystem;}

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	virtual void update();
	virtual bool getVisible() const;
	virtual void fade();
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	virtual void setStartTime(float startTime) { this->startTime = startTime; }
	virtual float getStartTime() const { return this->startTime; }
	virtual void setEndTime(float endTime) { this->endTime = endTime; }
	virtual float getEndTime() const { return this->endTime; }

	//set params
	void setRadius(float radius)					{this->radius= radius;}
	void setMinRadius(float minRadius)				{this->minRadius= minRadius;}
	void setEmissionRateFade(float emissionRateFade)		{this->emissionRateFade= emissionRateFade;}

	void setWind(float windAngle, float windSpeed);
	
	void setDirection(Vec3f direction)				{this->direction= direction;}
	void setSizeNoEnergy(float sizeNoEnergy)			{this->sizeNoEnergy= sizeNoEnergy;}
	void setGravity(float gravity)						{this->gravity= gravity;}
	void setRotation(float rotation);
	void setRelative(bool relative)						{this->relative= relative;}
	void setRelativeDirection(bool relativeDirection)	{this->relativeDirection= relativeDirection;}
	void setFixed(bool fixed)							{this->fixed= fixed;}
	void setPrimitive(Primitive primitive)				{this->primitive= primitive;}
	void setStaticParticleCount(int staticParticleCount){this->staticParticleCount= staticParticleCount;}
	void setIsVisibleAtNight(bool value)				{this->isVisibleAtNight= value;}
	void setIsVisibleAtDay(bool value)					{this->isVisibleAtDay= value;}
	void setRadiusBasedStartenergy(bool value)			{this->radiusBasedStartenergy= value;}
	void setShape(Shape shape)					{this->shape= shape;}
	void setAngle(float angle)					{this->angle= angle;}
	void setDelay(int delay) 					{this->delay= delay;}
	void setLifetime(int lifetime)					{this->lifetime= lifetime;}
	void setParent(GameParticleSystem* parent)			{this->parent= parent;}
	GameParticleSystem* getParent() const				{return parent;}
	void setParentDirection(Vec3f parentDirection);
	
	static Shape strToShape(const string& str);
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

	virtual ParticleSystemType getParticleSystemType() const { return pst_RainParticleSystem;}

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

	virtual ParticleSystemType getParticleSystemType() const { return pst_SnowParticleSystem;}

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

class AttackParticleSystem: public GameParticleSystem{

protected:
	float sizeNoEnergy;
	float gravity;
public:
	AttackParticleSystem(int particleCount);

	virtual ParticleSystemType getParticleSystemType() const { return pst_ProjectileParticleSystem;}

	void setSizeNoEnergy(float sizeNoEnergy)	{this->sizeNoEnergy= sizeNoEnergy;}
	void setGravity(float gravity)				{this->gravity= gravity;}
	
	virtual void initParticleSystem() {} // opportunity to do any initialization when the system has been created and all settings set
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
	
	void rotateChildren();

public:
	ProjectileParticleSystem(int particleCount= 1000);
	virtual ~ProjectileParticleSystem();

	virtual ParticleSystemType getParticleSystemType() const { return pst_SplashParticleSystem;}

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

	float emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;
	
	float startEmissionRate;

public:
	SplashParticleSystem(int particleCount= 1000);
	virtual ~SplashParticleSystem();
	
	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	
	virtual void initParticleSystem();

	void setEmissionRateFade(float emissionRateFade)		{this->emissionRateFade= emissionRateFade;}
	void setVerticalSpreadA(float verticalSpreadA)		{this->verticalSpreadA= verticalSpreadA;}
	void setVerticalSpreadB(float verticalSpreadB)		{this->verticalSpreadB= verticalSpreadB;}
	void setHorizontalSpreadA(float horizontalSpreadA)	{this->horizontalSpreadA= horizontalSpreadA;}
	void setHorizontalSpreadB(float horizontalSpreadB)	{this->horizontalSpreadB= horizontalSpreadB;}
	
};

// =====================================================
//	class ParticleManager
// =====================================================

class ParticleManager {
private:
	vector<ParticleSystem *> particleSystems;

public:
	ParticleManager();
	~ParticleManager();
	void update(int renderFps=-1);
	void render(ParticleRenderer *pr, ModelRenderer *mr) const;	
	void manage(ParticleSystem *ps);
	void end();
	void cleanupParticleSystems(ParticleSystem *ps);
	void cleanupParticleSystems(vector<ParticleSystem *> &particleSystems);
	void cleanupUnitParticleSystems(vector<UnitParticleSystem *> &particleSystems);
	int findParticleSystems(ParticleSystem *psFind, const vector<ParticleSystem *> &particleSystems) const;
	bool validateParticleSystemStillExists(ParticleSystem * particleSystem) const;
	bool hasActiveParticleSystem(ParticleSystem::ParticleSystemType type) const;
}; 

}}//end namespace

#endif
