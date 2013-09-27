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
#include "randomgen.h"
#include "xml_parser.h"
#include "leak_dumper.h"

using std::list;
using Shared::Util::RandomGen;
using Shared::Xml::XmlNode;

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
	Vec3d pos;
	Vec3d lastPos;
	Vec3d speed;
	Vec3d accel;
	Vec4f color;
	double size;
	int energy;

public:
	Particle() {
		size = 0;
		energy = 0;
	}
	//get
	Vec3d getPos() const		{return pos;}
	Vec3d getLastPos() const	{return lastPos;}
	Vec3d getSpeed() const		{return speed;}
	Vec3d getAccel() const		{return accel;}
	Vec4f getColor() const		{return color;}
	double getSize() const		{return size;}
	int getEnergy()	const		{return energy;}

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);
};

// =====================================================
//	class ParticleObserver
// =====================================================

class ParticleObserver {
public:
	virtual ~ParticleObserver(){};
	virtual void update(ParticleSystem *particleSystem)= 0;
	virtual void saveGame(XmlNode *rootNode) = 0;
	virtual void loadGame(const XmlNode *rootNode, void *genericData) = 0;
};

class ParticleOwner {
public:
	virtual void end(ParticleSystem *particleSystem)= 0;
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
	
	string textureFileLoadDeferred;
	int textureFileLoadDeferredSystemId;
	Texture::Format textureFileLoadDeferredFormat;
	int textureFileLoadDeferredComponents;

	Texture *texture;
	Vec3d pos;
	Vec4f color;
	Vec4f colorNoEnergy;
	double emissionRate;
	double emissionState;
	int maxParticleEnergy;
	int varParticleEnergy;
	double particleSize;
	double speed;
	Vec3f factionColor;
    bool teamcolorNoEnergy;
    bool teamcolorEnergy;
	int alternations;
	int particleSystemStartDelay;
	ParticleObserver *particleObserver;
	ParticleOwner *particleOwner;

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
	Vec3d getPos() const						{return pos;}
	Particle *getParticle(int i)				{return &particles[i];}
	const Particle *getParticle(int i) const	{return &particles[i];}
	int getAliveParticleCount() const			{return aliveParticleCount;}
	bool getActive() const						{return active;}
	virtual bool getVisible() const				{return visible;}

	virtual string getTextureFileLoadDeferred();
	virtual int getTextureFileLoadDeferredSystemId();
	virtual Texture::Format getTextureFileLoadDeferredFormat();
	virtual int getTextureFileLoadDeferredComponents();

	//set
	virtual void setState(State state);
	void setTexture(Texture *texture);
	virtual void setPos(Vec3d pos);
	void setColor(Vec4f color);
	void setColorNoEnergy(Vec4f color);
	void setEmissionRate(double emissionRate);
	void setMaxParticleEnergy(int maxParticleEnergy);
	void setVarParticleEnergy(int varParticleEnergy);
	void setParticleSize(double particleSize);
	void setSpeed(double speed);
	virtual void setActive(bool active);
	void setObserver(ParticleObserver *particleObserver);
	virtual void setVisible(bool visible);
	void setBlendMode(BlendMode blendMode)				{this->blendMode= blendMode;}
	void setTeamcolorNoEnergy(bool teamcolorNoEnergy)	{this->teamcolorNoEnergy= teamcolorNoEnergy;}
	void setTeamcolorEnergy(bool teamcolorEnergy)		{this->teamcolorEnergy= teamcolorEnergy;}
	void setAlternations(int alternations)				{this->alternations= alternations;}
	void setParticleSystemStartDelay(int delay)			{this->particleSystemStartDelay= delay;}
	virtual void setFactionColor(Vec3f factionColor);

	static BlendMode strToBlendMode(const string &str);
	//misc
	virtual void fade();
	int isEmpty() const;
	
	virtual void setParticleOwner(ParticleOwner *particleOwner) { this->particleOwner = particleOwner;}
	virtual ParticleOwner * getParticleOwner() { return this->particleOwner;}
	virtual void callParticleOwnerEnd(ParticleSystem *particleSystem);

	//children
	virtual int getChildCount() { return 0; }
	virtual ParticleSystem* getChild(int i);

	virtual string toString() const;

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual Checksum getCRC();

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
	double radius;
	Vec3d windSpeed;

public:
	FireParticleSystem(int particleCount= 2000);

	virtual ParticleSystemType getParticleSystemType() const { return pst_FireParticleSystem;}

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	//set params
	void setRadius(double radius);
	void setWind(double windAngle, double windSpeed);

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual string toString() const;

	virtual Checksum getCRC();
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
	void setPos(Vec3d pos);
	void setOffset(Vec3d offset);
	void setModel(Model *model) {this->model= model;}
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);
	double getTween() { return tween; }  // 0.0 -> 1.0 for animation of model
	Model *getModel() const {return model;}
	virtual string getModelFileLoadDeferred();

	void setPrimitive(Primitive primitive) {this->primitive= primitive;}
	Vec3d getDirection() const {return direction;}
	void setModelCycle(double modelCycle) {this->modelCycle= modelCycle;}

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual string toString() const;

	virtual Checksum getCRC();

protected:
	typedef std::vector<UnitParticleSystem*> Children;
	Children children;
	Primitive primitive;

	string modelFileLoadDeferred;
	Model *model;
	double modelCycle;
	Vec3d offset;
	Vec3d direction;
	double tween;
	
	GameParticleSystem(int particleCount);
	void positionChildren();
	void setTween(double relative,double absolute);
};

// =====================================================
//	class UnitParticleSystem
// =====================================================

class UnitParticleSystem: public GameParticleSystem{
public:
	static bool isNight;
	static Vec3f lightColor;
private:
	double radius;
	double minRadius;
	Vec3d windSpeed;
	Vec3d cRotation;
	Vec3d fixedAddition;
    Vec3d oldPosition;
    bool energyUp;
    double startTime;
    double endTime;
    
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
	double angle;
	double sizeNoEnergy;
	double gravity;
	double rotation;
	bool isVisibleAtNight;
	bool isVisibleAtDay;
	bool isDaylightAffected;
	bool radiusBasedStartenergy;
	int staticParticleCount;
	int delay;
	int lifetime;
	double emissionRateFade;
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

	virtual void setStartTime(double startTime) { this->startTime = startTime; }
	virtual double getStartTime() const { return this->startTime; }
	virtual void setEndTime(double endTime) { this->endTime = endTime; }
	virtual double getEndTime() const { return this->endTime; }

	//set params
	void setRadius(double radius)					{this->radius= radius;}
	void setMinRadius(double minRadius)				{this->minRadius= minRadius;}
	void setEmissionRateFade(double emissionRateFade)		{this->emissionRateFade= emissionRateFade;}

	void setWind(double windAngle, double windSpeed);
	
	void setDirection(Vec3d direction)				{this->direction= direction;}
	void setSizeNoEnergy(double sizeNoEnergy)			{this->sizeNoEnergy= sizeNoEnergy;}
	void setGravity(double gravity)						{this->gravity= gravity;}
	void setRotation(double rotation);
	void setRelative(bool relative)						{this->relative= relative;}
	void setRelativeDirection(bool relativeDirection)	{this->relativeDirection= relativeDirection;}
	void setFixed(bool fixed)							{this->fixed= fixed;}
	void setPrimitive(Primitive primitive)				{this->primitive= primitive;}
	void setStaticParticleCount(int staticParticleCount){this->staticParticleCount= staticParticleCount;}
	void setIsVisibleAtNight(bool value)				{this->isVisibleAtNight= value;}
	void setIsDaylightAffected(bool value)				{this->isDaylightAffected= value;}
	void setIsVisibleAtDay(bool value)					{this->isVisibleAtDay= value;}
	void setRadiusBasedStartenergy(bool value)			{this->radiusBasedStartenergy= value;}
	void setShape(Shape shape)					{this->shape= shape;}
	void setAngle(double angle)					{this->angle= angle;}
	void setDelay(int delay) 					{this->delay= delay;}
	void setLifetime(int lifetime)					{this->lifetime= lifetime;}
	void setParent(GameParticleSystem* parent)			{this->parent= parent;}
	GameParticleSystem* getParent() const				{return parent;}
	void setParentDirection(Vec3d parentDirection);
	
	static Shape strToShape(const string& str);

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual string toString() const;

	virtual Checksum getCRC();
};

// =====================================================
//	class RainParticleSystem
// =====================================================

class RainParticleSystem: public ParticleSystem{
private:
	Vec3d windSpeed;
	double radius;

public:
	RainParticleSystem(int particleCount= 4000);

	virtual ParticleSystemType getParticleSystemType() const { return pst_RainParticleSystem;}

	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	virtual void initParticle(Particle *p, int particleIndex);
	virtual bool deathTest(Particle *p);

	void setRadius(double radius);
	void setWind(double windAngle, double windSpeed);

	virtual string toString() const;

	virtual Checksum getCRC();
};

// =====================================================
//	class SnowParticleSystem
// =====================================================

class SnowParticleSystem: public ParticleSystem{
private:
	Vec3d windSpeed;
	double radius;

public:
	SnowParticleSystem(int particleCount= 4000);

	virtual ParticleSystemType getParticleSystemType() const { return pst_SnowParticleSystem;}

	virtual void initParticle(Particle *p, int particleIndex);
	virtual bool deathTest(Particle *p);

	void setRadius(double radius);
	void setWind(double windAngle, double windSpeed);

	virtual string toString() const;

	virtual Checksum getCRC();
};

// ===========================================================================
//  AttackParticleSystem
//
/// Base class for Projectiles and Splashes
// ===========================================================================

class AttackParticleSystem: public GameParticleSystem {

protected:
	double sizeNoEnergy;
	double gravity;
public:
	AttackParticleSystem(int particleCount);

	virtual ParticleSystemType getParticleSystemType() const { return pst_ProjectileParticleSystem;}

	void setSizeNoEnergy(double sizeNoEnergy)	{this->sizeNoEnergy= sizeNoEnergy;}
	void setGravity(double gravity)				{this->gravity= gravity;}
	
	virtual void initParticleSystem() {} // opportunity to do any initialization when the system has been created and all settings set

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual string toString() const;

	virtual Checksum getCRC();
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

	Vec3d lastPos;
	Vec3d startPos;
	Vec3d endPos;
	Vec3d flatPos;

	Vec3d xVector;
	Vec3d yVector;
	Vec3d zVector;

	Trajectory trajectory;
	double trajectorySpeed;

	//parabolic
	double trajectoryScale;
	double trajectoryFrequency;

	double arriveDestinationDistance;
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
	void setTrajectorySpeed(double trajectorySpeed)			{this->trajectorySpeed= trajectorySpeed;}
	void setTrajectoryScale(double trajectoryScale)			{this->trajectoryScale= trajectoryScale;}
	void setTrajectoryFrequency(double trajectoryFrequency)	{this->trajectoryFrequency= trajectoryFrequency;}

	void setPath(Vec3d startPos, Vec3d endPos);

	static Trajectory strToTrajectory(const string &str);

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual string toString() const;

	virtual Checksum getCRC();
};

// =====================================================
//	class SplashParticleSystem
// =====================================================

class SplashParticleSystem: public AttackParticleSystem{
public:
	friend class ProjectileParticleSystem;

private:
	ProjectileParticleSystem *prevParticleSystem;

	double emissionRateFade;
	double verticalSpreadA;
	double verticalSpreadB;
	double horizontalSpreadA;
	double horizontalSpreadB;
	
	double startEmissionRate;

public:
	SplashParticleSystem(int particleCount= 1000);
	virtual ~SplashParticleSystem();
	
	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	
	virtual void initParticleSystem();

	void setEmissionRateFade(double emissionRateFade)		{this->emissionRateFade= emissionRateFade;}
	void setVerticalSpreadA(double verticalSpreadA)		{this->verticalSpreadA= verticalSpreadA;}
	void setVerticalSpreadB(double verticalSpreadB)		{this->verticalSpreadB= verticalSpreadB;}
	void setHorizontalSpreadA(double horizontalSpreadA)	{this->horizontalSpreadA= horizontalSpreadA;}
	void setHorizontalSpreadB(double horizontalSpreadB)	{this->horizontalSpreadB= horizontalSpreadB;}

	virtual void saveGame(XmlNode *rootNode);
	virtual void loadGame(const XmlNode *rootNode);

	virtual string toString() const;
	
	virtual Checksum getCRC();
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
	void removeParticleSystemsForParticleOwner(ParticleOwner * particleOwner);
	bool hasActiveParticleSystem(ParticleSystem::ParticleSystemType type) const;
}; 

}}//end namespace

#endif
