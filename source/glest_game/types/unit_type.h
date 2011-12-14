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

#ifndef _GLEST_GAME_UNITTYPE_H_
#define _GLEST_GAME_UNITTYPE_H_

#include "element_type.h"
#include "command_type.h"
#include "damage_multiplier.h"
#include "sound_container.h"
#include "checksum.h"
#include "game_constants.h"
#include "platform_common.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Sound::StaticSound;
using Shared::Util::Checksum;
using Shared::PlatformCommon::ValueCheckerVault;

class UpgradeType;
class UnitType;
class UnitParticleSystemType;
class ResourceType;
class TechTree;
class FactionType;
class Faction;


// ===============================
// 	class Level
// ===============================

class Level {
private:
	string name;
	int kills;

public:
	Level() {
		kills = 0;
	}
	void init(string name, int kills);

	const string &getName() const	{return name;}
	int getKills() const			{return kills;}
};

// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================

enum UnitClass {
	ucWarrior,
	ucWorker,
	ucBuilding
};

typedef vector<UnitParticleSystemType*> DamageParticleSystemTypes;

class UnitType: public ProducibleType, public ValueCheckerVault {
public:
	enum Property {
		pBurnable,
		pRotatedClimb,

		pCount
	};

	static const char *propertyNames[];
	DamageParticleSystemTypes damageParticleSystemTypes;
private:
    typedef vector<SkillType*> SkillTypes;
    typedef vector<CommandType*> CommandTypes;
    typedef vector<Resource> StoredResources;
	typedef vector<Level> Levels;

private:
	//basic
	int id;
	int maxHp;
	int hpRegeneration;
    int maxEp;
	int epRegeneration;
	int maxUnitCount;


	///@todo remove fields, multiple fields are not supported by the engine
	bool fields[fieldCount];			//fields: land, sea or air
	Field field;

    bool properties[pCount];			//properties
	int armor;							//armor
	const ArmorType *armorType;
	bool light;
    Vec3f lightColor;
    bool multiSelect;
    int sight;
    int size;							//size in cells
    int height;
    float rotatedBuildPos;
    bool rotationAllowed;

	//cellmap
	bool *cellMap;
	bool allowEmptyCellMap;

	//sounds
    SoundContainer selectionSounds;
    SoundContainer commandSounds;

	//info
    SkillTypes skillTypes;
    CommandTypes commandTypes;
    StoredResources storedResources;
	Levels levels;

	//meeting point
	bool meetingPoint;
	Texture2D *meetingPointImage;

    //OPTIMIZATION: store first command type and skill type of each class
	const CommandType *firstCommandTypeOfClass[ccCount];
    const SkillType *firstSkillTypeOfClass[scCount];

public:
	//creation and loading
    UnitType();
    virtual ~UnitType();
	void preLoad(const string &dir);
    void load(int id, const string &dir, const TechTree *techTree,
    		const FactionType *factionType, Checksum* checksum,
    		Checksum* techtreeChecksum, std::map<string,vector<pair<string, string> > > &loadedFileList);

	//get
	int getId() const									{return id;}
	int getMaxHp() const								{return maxHp;}
	int getHpRegeneration() const						{return hpRegeneration;}
	int getMaxEp() const								{return maxEp;}
	int getEpRegeneration() const						{return epRegeneration;}
	int getMaxUnitCount() const							{return maxUnitCount;}
	bool getField(Field field) const					{return fields[field];}
	Field getField() const								{return field;}
	bool getProperty(Property property) const			{return properties[property];}
	int getArmor() const								{return armor;}
	const ArmorType *getArmorType() const				{return armorType;}
	const SkillType *getSkillType(int i) const			{return skillTypes[i];}
	const CommandType *getCommandType(int i) const;
	const Level *getLevel(int i) const					{return &levels[i];}
	int getSkillTypeCount() const						{return skillTypes.size();}
	int getCommandTypeCount() const						{return commandTypes.size();}
	int getLevelCount() const							{return levels.size();}
	bool getLight() const								{return light;}
	bool getRotationAllowed() const						{return rotationAllowed;}
	Vec3f getLightColor() const							{return lightColor;}
	bool getMultiSelect() const							{return multiSelect;}
	int getSight() const								{return sight;}
	int getSize() const									{return size;}
	int getHeight() const								{return height;}
	int getStoredResourceCount() const					{return storedResources.size();}
	const Resource *getStoredResource(int i) const		{return &storedResources[i];}
	bool getCellMapCell(int x, int y, CardinalDir facing) const;
	bool getMeetingPoint() const						{return meetingPoint;}
	bool isMobile() const								{return (firstSkillTypeOfClass[scMove] != NULL);}
	Texture2D *getMeetingPointImage() const				{return meetingPointImage;}
	StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}

    const SoundContainer & getSelectionSounds() const { return selectionSounds; }
    const SoundContainer & getCommandSounds() const   { return commandSounds; }

	int getStore(const ResourceType *rt) const;
	const SkillType *getSkillType(const string &skillName, SkillClass skillClass) const;
	const SkillType *getFirstStOfClass(SkillClass skillClass) const;
    const CommandType *getFirstCtOfClass(CommandClass commandClass) const;
    const HarvestCommandType *getFirstHarvestCommand(const ResourceType *resourceType,const Faction *faction) const;
	const AttackCommandType *getFirstAttackCommand(Field field) const;
	const AttackStoppedCommandType *getFirstAttackStoppedCommand(Field field) const;
	const RepairCommandType *getFirstRepairCommand(const UnitType *repaired) const;

	//get totals
	int getTotalMaxHp(const TotalUpgrade *totalUpgrade) const;
	int getTotalMaxHpRegeneration(const TotalUpgrade *totalUpgrade) const;
	int getTotalMaxEp(const TotalUpgrade *totalUpgrade) const;
	int getTotalMaxEpRegeneration(const TotalUpgrade *totalUpgrade) const;
	int getTotalArmor(const TotalUpgrade *totalUpgrade) const;
	int getTotalSight(const TotalUpgrade *totalUpgrade) const;

	//has
    bool hasCommandType(const CommandType *commandType) const;
	bool hasCommandClass(CommandClass commandClass) const;
    bool hasSkillType(const SkillType *skillType) const;
    bool hasSkillClass(SkillClass skillClass) const;
	bool hasCellMap() const										{return cellMap!=NULL;}
	bool getAllowEmptyCellMap() const {return allowEmptyCellMap;}
	bool hasEmptyCellMap() const;
	Vec2i getFirstOccupiedCellInCellMap(Vec2i currentPos) const;

	//is
	bool isOfClass(UnitClass uc) const;

	//find
	const CommandType* findCommandTypeById(int id) const;
	string getCommandTypeListDesc() const;

    float getRotatedBuildPos() { return rotatedBuildPos; }
    void setRotatedBuildPos(float value) { rotatedBuildPos = value; }

	//other
    virtual string getReqDesc() const;

    std::string toString() const;

private:
    void computeFirstStOfClass();
    void computeFirstCtOfClass();
};

}}//end namespace


#endif
