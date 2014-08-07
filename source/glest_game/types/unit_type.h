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

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

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

	string getName(bool translatedValue=false) const;
	int getKills() const			{return kills;}

	void saveGame(XmlNode *rootNode) const ;
	static const Level * loadGame(const XmlNode *rootNode, const UnitType *ut);
};

// ===============================
// 	class LootResource
//
///	Stores information about a lootable resource. Lootable resources are stolen by the attacker on death.
// ===============================

class LootableResource {
private:
	const ResourceType *type;
	int amountValue;
	int amountFactionPercent;
	int lossValue;
	int lossFactionPercent;
	bool negativeAllowed;

public:
	const ResourceType* getResourceType() const {return type;}
	void setResourceType(const ResourceType *type) {this->type=type;}

	int getAmountValue() const {return amountValue;}
	void setAmountValue(int amountValue) {this->amountValue=amountValue;}

	int getAmountFactionPercent() const {return amountFactionPercent;}
	void setAmountFactionPercent(int amountPercentage) {this->amountFactionPercent=amountPercentage;}

	int getLossValue() const {return lossValue;}
	void setLossValue(int lossValue) {this->lossValue=lossValue;}

	int getLossFactionPercent() const {return lossFactionPercent;}
	void setLossFactionPercent(int lossPercentage) {this->lossFactionPercent=lossPercentage;}

	bool isNegativeAllowed() const {return negativeAllowed;}
	void setNegativeAllowed(bool negativeAllowed) {this->negativeAllowed=negativeAllowed;}

	bool operator==(const LootableResource& other) {
		return type == other.getResourceType();
	}
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

enum UnitCountsInVictoryConditions {
	ucvcNotSet,
	ucvcTrue,
	ucvcFalse
};

class UnitType: public ProducibleType, public ValueCheckerVault {
public:
	enum Property {
		pBurnable,
		pRotatedClimb,

		pCount
	};

	enum StartType {
		stValue,
		stPercentage
	};

	static const char *propertyNames[];
	DamageParticleSystemTypes damageParticleSystemTypes;
private:
    typedef vector<SkillType*> SkillTypes;
    typedef vector<CommandType*> CommandTypes;
    typedef vector<Resource> StoredResources;
	typedef vector<Level> Levels;
	typedef vector<LootableResource> LootableResources;

private:
	//basic
	int id;
	int maxHp;
    int startHpValue;
    int startHpPercentage;
	StartType startHpType;
	int hpRegeneration;
    int maxEp;
    int startEpValue;
    int startEpPercentage;
	StartType startEpType;
	int epRegeneration;
	int maxUnitCount;


	// remove fields, multiple fields are not supported by the engine
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
    int renderSize;						//size to render in cells
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
	LootableResources lootableResources;
	std::set<string> tags;

	//meeting point
	bool meetingPoint;
	Texture2D *meetingPointImage;

	// for dummy units and units used as shots and so on ....
	bool countUnitDeathInStats;
	bool countUnitProductionInStats;
	bool countUnitKillInStats;
	bool countKillForUnitUpgrade;

    //OPTIMIZATION: store first command type and skill type of each class
	const CommandType *firstCommandTypeOfClass[ccCount];
    const SkillType *firstSkillTypeOfClass[scCount];

    UnitCountsInVictoryConditions countInVictoryConditions;

    static auto_ptr<CommandType> ctHarvestEmergencyReturnCommandType;

public:
	//creation and loading
    UnitType();
    virtual ~UnitType();
	void preLoad(const string &dir);
    void loaddd(int id, const string &dir, const TechTree *techTree,
    		const string &techTreePath,
    		const FactionType *factionType, Checksum* checksum,
    		Checksum* techtreeChecksum,
    		std::map<string,vector<pair<string, string> > > &loadedFileList,
    		bool validationMode=false);

    virtual string getName(bool translatedValue=false) const;

    UnitCountsInVictoryConditions getCountInVictoryConditions() const { return countInVictoryConditions; }
	//get
    inline int getId() const									{return id;}
    inline int getMaxHp() const								{return maxHp;}
    inline int getHpRegeneration() const						{return hpRegeneration;}
    inline int getStartHpValue() const						{return startHpValue;}
    inline int getStartHpPercentage() const						{return startHpPercentage;}
    inline StartType getStartHpType() const						{return startHpType;}
    inline int getMaxEp() const								{return maxEp;}
    inline int getEpRegeneration() const						{return epRegeneration;}
    inline int getStartEpValue() const						{return startEpValue;}
    inline int getStartEpPercentage() const						{return startEpPercentage;}
    inline StartType getStartEpType() const						{return startEpType;}
    inline int getMaxUnitCount() const							{return maxUnitCount;}
    inline bool getField(Field field) const					{return fields[field];}
    inline Field getField() const								{return field;}
    inline bool getProperty(Property property) const			{return properties[property];}
    inline int getArmor() const								{return armor;}
    inline const ArmorType *getArmorType() const				{return armorType;}
    inline const SkillType *getSkillType(int i) const			{return skillTypes[i];}
	const CommandType *getCommandType(int i) const;
	inline const Level *getLevel(int i) const					{return &levels[i];}
	const Level *getLevel(string name) const;
	inline int getSkillTypeCount() const						{return (int)skillTypes.size();}
	inline int getCommandTypeCount() const						{return (int)commandTypes.size();}
	inline int getLevelCount() const							{return (int)levels.size();}
	inline bool getLight() const								{return light;}
	inline bool getRotationAllowed() const						{return rotationAllowed;}
	inline Vec3f getLightColor() const							{return lightColor;}
	inline bool getMultiSelect() const							{return multiSelect;}
	inline int getSight() const								{return sight;}
	inline int getSize() const									{return size;}
	inline int getRenderSize() const								{return renderSize;}
	int getHeight() const								{return height;}
	int getStoredResourceCount() const					{return (int)storedResources.size();}
	inline const Resource *getStoredResource(int i) const		{return &storedResources[i];}
	int getLootableResourceCount() const					{return lootableResources.size();}
	inline const LootableResource getLootableResource(int i) const		{return lootableResources.at(i);}
	const set<string> &getTags() const	{return tags;}
	bool getCellMapCell(int x, int y, CardinalDir facing) const;
	inline bool getMeetingPoint() const						{return meetingPoint;}
	inline bool getCountUnitDeathInStats() const				{return countUnitDeathInStats;}
	inline bool getCountUnitProductionInStats() const			{return countUnitProductionInStats;}
	inline bool getCountUnitKillInStats() const				{return countUnitKillInStats;}
	inline bool getCountKillForUnitUpgrade() const				{return countKillForUnitUpgrade;}
	inline bool isMobile() const								{return (firstSkillTypeOfClass[scMove] != NULL);}
	inline Texture2D *getMeetingPointImage() const				{return meetingPointImage;}
	inline StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	inline StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}

	inline const SoundContainer & getSelectionSounds() const { return selectionSounds; }
	inline const SoundContainer & getCommandSounds() const   { return commandSounds; }

	int getStore(const ResourceType *rt) const;
	const SkillType *getSkillType(const string &skillName, SkillClass skillClass) const;
	const SkillType *getFirstStOfClass(SkillClass skillClass) const;
    const CommandType *getFirstCtOfClass(CommandClass commandClass) const;
    const HarvestCommandType *getFirstHarvestCommand(const ResourceType *resourceType,const Faction *faction) const;
    const HarvestEmergencyReturnCommandType *getFirstHarvestEmergencyReturnCommand() const;
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
	inline bool hasCommandClass(CommandClass commandClass) const {
		return firstCommandTypeOfClass[commandClass]!=NULL;
	}
    bool hasSkillType(const SkillType *skillType) const;
    bool hasSkillClass(SkillClass skillClass) const;
    inline bool hasCellMap() const										{return cellMap!=NULL;}
    inline bool getAllowEmptyCellMap() const {return allowEmptyCellMap;}
	bool hasEmptyCellMap() const;
	Vec2i getFirstOccupiedCellInCellMap(Vec2i currentPos) const;

	//is
	bool isOfClass(UnitClass uc) const;

	//find
	const CommandType* findCommandTypeById(int id) const;
	string getCommandTypeListDesc() const;

	inline float getRotatedBuildPos() { return rotatedBuildPos; }
	inline void setRotatedBuildPos(float value) { rotatedBuildPos = value; }

	//other
    virtual string getReqDesc(bool translatedValue) const;

    std::string toString() const;

private:
    void computeFirstStOfClass();
    void computeFirstCtOfClass();
};

/**
 * Used to sort UnitType. Sorts by *translated* unit name. Sorting is case sensitive and done in
 * lexical order.
 */
struct UnitTypeSorter
{
    bool operator()( const UnitType *left, const UnitType *right ) const {
    	return left->getName(true) < right->getName(true);
    }
};

}}//end namespace


#endif
