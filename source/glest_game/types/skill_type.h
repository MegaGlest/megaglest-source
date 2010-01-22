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

#ifndef _GLEST_GAME_SKILLTYPE_H_
#define _GLEST_GAME_SKILLTYPE_H_

#include "sound.h"
#include "vec.h"
#include "model.h"
#include "xml_parser.h"
#include "util.h"
#include "damage_multiplier.h"
#include "element_type.h"
#include "factory.h"
#include "sound_container.h"

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Model;

namespace Glest{ namespace Game{

using Shared::Util::MultiFactory;

class ParticleSystemTypeProjectile;
class ParticleSystemTypeSplash;
class FactionType;
class TechTree;
class Lang;
class TotalUpgrade;

enum Field{
     fLand,
     fAir,

     fieldCount
};

enum SkillClass{
    scStop,
    scMove,
    scAttack,
    scBuild,
    scHarvest,
    scRepair,
    scBeBuilt,
    scProduce,
    scUpgrade,
	scMorph,
	scDie,

    scCount
};

// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
// =====================================================

class SkillType{
protected:
    SkillClass skillClass;
	string name;
	int mpCost;
    int speed;
    int animSpeed;
    Model *animation;
    SoundContainer sounds;
	float soundStartTime;

public:
    //varios
    virtual ~SkillType();
    virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);	
		
    //get
	const string &getName() const		{return name;}
	SkillClass getClass() const			{return skillClass;}
	int getEpCost() const				{return mpCost;}
	int getSpeed() const				{return speed;}
	int getAnimSpeed() const			{return animSpeed;}
	const Model *getAnimation() const	{return animation;}
	StaticSound *getSound() const		{return sounds.getRandSound();}
	float getSoundStartTime() const		{return soundStartTime;}

	//other
	virtual string toString() const= 0;	
	virtual int getTotalSpeed(const TotalUpgrade *) const	{return speed;}
	static string skillClassToStr(SkillClass skillClass); 
	static string fieldToStr(Field field);
};

// ===============================
// 	class StopSkillType  
// ===============================

class StopSkillType: public SkillType{
public:
    StopSkillType();
    virtual string toString() const;
};

// ===============================
// 	class MoveSkillType  
// ===============================

class MoveSkillType: public SkillType{
public:
    MoveSkillType();
    virtual string toString() const;

	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;
};

// ===============================
// 	class AttackSkillType  
// ===============================

class AttackSkillType: public SkillType{
private:
    int attackStrength;
    int attackVar;
    int attackRange;
	const AttackType *attackType;
	bool attackFields[fieldCount];
	float attackStartTime;

    bool projectile;
    ParticleSystemTypeProjectile* projectileParticleSystemType;
	SoundContainer projSounds;
	
    bool splash;
    int splashRadius;
    bool splashDamageAll;
    ParticleSystemTypeSplash* splashParticleSystemType;

public:
    AttackSkillType();
    ~AttackSkillType();
    virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual string toString() const;
    
	//get
	int getAttackStrength() const				{return attackStrength;}
	int getAttackVar() const					{return attackVar;}
	int getAttackRange() const					{return attackRange;}
	const AttackType *getAttackType() const		{return attackType;}
	bool getAttackField(Field field) const		{return attackFields[field];}
	float getAttackStartTime() const			{return attackStartTime;}

	//get proj
	bool getProjectile() const									{return projectile;}
	ParticleSystemTypeProjectile * getProjParticleType() const	{return projectileParticleSystemType;}
	StaticSound *getProjSound() const							{return projSounds.getRandSound();}

	//get splash
	bool getSplash() const										{return splash;}
	int getSplashRadius() const									{return splashRadius;}
	bool getSplashDamageAll() const								{return splashDamageAll;}
	ParticleSystemTypeSplash * getSplashParticleType() const	{return splashParticleSystemType;}
	
	//misc
	int getTotalAttackStrength(const TotalUpgrade *totalUpgrade) const;
	int getTotalAttackRange(const TotalUpgrade *totalUpgrade) const;
};


// ===============================
// 	class BuildSkillType  
// ===============================

class BuildSkillType: public SkillType{
public:
    BuildSkillType();
    virtual string toString() const;
};

// ===============================
// 	class HarvestSkillType  
// ===============================

class HarvestSkillType: public SkillType{
public:
    HarvestSkillType();
	virtual string toString() const;
};

// ===============================
// 	class RepairSkillType  
// ===============================

class RepairSkillType: public SkillType{
public:
    RepairSkillType();
    virtual string toString() const;
};

// ===============================
// 	class ProduceSkillType  
// ===============================

class ProduceSkillType: public SkillType{
public:
    ProduceSkillType();
    virtual string toString() const;

	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;
};

// ===============================
// 	class UpgradeSkillType  
// ===============================

class UpgradeSkillType: public SkillType{
public:
    UpgradeSkillType();
	virtual string toString() const;

	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;
};


// ===============================
// 	class BeBuiltSkillType  
// ===============================

class BeBuiltSkillType: public SkillType{
public:
    BeBuiltSkillType();
    virtual string toString() const;
};

// ===============================
// 	class MorphSkillType  
// ===============================

class MorphSkillType: public SkillType{
public:
    MorphSkillType();
    virtual string toString() const;

	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;
};

// ===============================
// 	class DieSkillType  
// ===============================

class DieSkillType: public SkillType{
private:
	bool fade;

public:
    DieSkillType();
    bool getFade() const	{return fade;}
	
	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual string toString() const;
};

// ===============================
// 	class SkillFactory  
// ===============================

class SkillTypeFactory: public MultiFactory<SkillType>{
private:
	SkillTypeFactory();
public:
	static SkillTypeFactory &getInstance();
};

}}//end namespace

#endif
