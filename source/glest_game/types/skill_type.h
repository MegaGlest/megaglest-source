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
#include "particle.h"
#include "upgrade_type.h"
#include "leak_dumper.h"

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Model;

namespace Glest{ namespace Game{

using Shared::Util::MultiFactory;

class ParticleSystemTypeProjectile;
class ParticleSystemTypeSplash;
class UnitParticleSystemType;
class FactionType;
class TechTree;
class Lang;
class TotalUpgrade;
class Unit;


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

typedef list<UnitParticleSystemType*> UnitParticleSystemTypes;
// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
// =====================================================

enum AttackBoostTargetType {
	abtAlly, // Only ally units are affected
	abtFoe, // Only foe units are affected
	abtFaction, // Only same faction units are affected
	abtUnitTypes, // Specify which units are affected ( in general same as abtAll )
	abtAll // All units are affected (including enemies)
};

class AttackBoost {
public:
	AttackBoost();
	~AttackBoost();
	bool enabled;
	bool allowMultipleBoosts;
	int radius;
	AttackBoostTargetType targetType;
	vector<const UnitType *> boostUnitList;
	UpgradeTypeBase boostUpgrade;

	UnitParticleSystemType *unitParticleSystemTypeForSourceUnit;
	UnitParticleSystemType *unitParticleSystemTypeForAffectedUnit;

	bool includeSelf;
	string name;

	bool isAffected(const Unit *source, const Unit *dest) const;
	virtual string getDesc() const;

	virtual void saveGame(XmlNode *rootNode) const;
};

class AnimationAttributes {
public:
	AnimationAttributes() {
		fromHp = 0;
		toHp = 0;
	}

	int fromHp;
	int toHp;
};

class SkillType {
    
protected:
    SkillClass skillClass;
	string name;
	int mpCost;
	int hpCost;
    int speed;
    int animSpeed;

    int animationRandomCycleMaxcount;
    vector<Model *> animations;
    vector<AnimationAttributes> animationAttributes;

    SoundContainer sounds;
	float soundStartTime;
	RandomGen random;
	AttackBoost attackBoost;

	static int nextAttackBoostId;
	static int getNextAttackBoostId() { return ++nextAttackBoostId; }

	const XmlNode * findAttackBoostDetails(string attackBoostName,
			const XmlNode *attackBoostsNode,const XmlNode *attackBoostNode);
	void loadAttackBoost(const XmlNode *attackBoostsNode,
			const XmlNode *attackBoostNode, const FactionType *ft,
			string parentLoader, const string & dir, string currentPath,
			std::map<string,vector<pair<string,string> > > & loadedFileList, const TechTree *tt);

public:
	UnitParticleSystemTypes unitParticleSystemTypes;

public:
    //varios
    virtual ~SkillType();
    virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
    		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
    		string parentLoader);
		
    bool CanCycleNextRandomAnimation(const int *animationRandomCycleCount) const;

    static void resetNextAttackBoostId() { nextAttackBoostId=0; }

    //get
	const string &getName() const		{return name;}
	SkillClass getClass() const			{return skillClass;}
	int getEpCost() const				{return mpCost;}
	int getHpCost() const				{return hpCost;}
	int getSpeed() const				{return speed;}
	int getAnimSpeed() const			{return animSpeed;}
	Model *getAnimation(float animProgress=0, const Unit *unit=NULL, int *lastAnimationIndex=NULL, int *animationRandomCycleCount=NULL) const;
	StaticSound *getSound() const		{return sounds.getRandSound();}
	float getSoundStartTime() const		{return soundStartTime;}
	
	bool isAttackBoostEnabled() const { return attackBoost.enabled; }
	const AttackBoost * getAttackBoost() const { return &attackBoost; }
	//virtual string getDesc(const TotalUpgrade *totalUpgrade) const= 0;

	//other
	virtual string toString() const= 0;	
	virtual int getTotalSpeed(const TotalUpgrade *) const	{return speed;}
	static string skillClassToStr(SkillClass skillClass); 
	static string fieldToStr(Field field);
	virtual string getBoostDesc() const {return attackBoost.getDesc();}

	virtual void saveGame(XmlNode *rootNode);
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

	string spawnUnit;
	int spawnUnitcount;
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
    virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
    		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
    		string parentLoader);
	virtual string toString() const;
    
	//get
	int getAttackStrength() const				{return attackStrength;}
	int getAttackVar() const					{return attackVar;}
	int getAttackRange() const					{return attackRange;}
	const AttackType *getAttackType() const		{return attackType;}
	bool getAttackField(Field field) const		{return attackFields[field];}
	float getAttackStartTime() const			{return attackStartTime;}
	string getSpawnUnit() const					{return spawnUnit;}
	int getSpawnUnitCount() const				{return spawnUnitcount;}

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

	virtual void saveGame(XmlNode *rootNode);
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
private:
	bool animProgressBound;
public:
    ProduceSkillType();
    bool getAnimProgressBound() const	{return animProgressBound;}
    virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
    			const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
    			string parentLoader);

    virtual string toString() const;

	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;

	virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class UpgradeSkillType  
// ===============================

class UpgradeSkillType: public SkillType{
private:
	bool animProgressBound;
public:
    UpgradeSkillType();
    bool getAnimProgressBound() const	{return animProgressBound;}
    virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
    			const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
    			string parentLoader);

	virtual string toString() const;

	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;

	virtual void saveGame(XmlNode *rootNode);
};


// ===============================
// 	class BeBuiltSkillType  
// ===============================

class BeBuiltSkillType: public SkillType{
private:
	bool animProgressBound;

public:
    BeBuiltSkillType();
	bool getAnimProgressBound() const	{return animProgressBound;}

    virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
    			const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
    			string parentLoader);
    virtual string toString() const;

    virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class MorphSkillType  
// ===============================

class MorphSkillType: public SkillType{
private:
	bool animProgressBound;

public:
    MorphSkillType();
	bool getAnimProgressBound() const	{return animProgressBound;}

    virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
    			const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
    			string parentLoader);

    virtual string toString() const;
	virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;

	virtual void saveGame(XmlNode *rootNode);
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
	
	virtual void load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir, const TechTree *tt,
			const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
			string parentLoader);
	virtual string toString() const;

	virtual void saveGame(XmlNode *rootNode);
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
