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

#ifndef _GLEST_GAME_TECHTREE_H_
#define _GLEST_GAME_TECHTREE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <set>
#include "util.h"
#include "resource_type.h"
#include "faction_type.h"
#include "damage_multiplier.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class TechTree
//
///	A set of factions and resources
// =====================================================

class TechTree{
private:
	typedef vector<ResourceType> ResourceTypes;
	typedef vector<FactionType> FactionTypes;
	typedef vector<ArmorType> ArmorTypes;
	typedef vector<AttackType> AttackTypes;

private:

	string name;
    //string desc;
	string treePath;
	vector<string> pathList;

    ResourceTypes resourceTypes;
    FactionTypes factionTypes;
	ArmorTypes armorTypes;
	AttackTypes attackTypes;
	DamageMultiplierTable damageMultiplierTable;
	Checksum checksumValue;

	string languageUsedForCache;
	std::map<string,string> translatedTechNames;
	std::map<string,std::map<string,string> > translatedTechFactionNames;
	bool isValidationModeEnabled;

public:
    Checksum loadTech(const string &techName,
    		set<string> &factions, Checksum* checksum,
    		std::map<string,vector<pair<string, string> > > &loadedFileList,
    		bool validationMode=false);
    void load(const string &dir, set<string> &factions, Checksum* checksum,
    		Checksum *techtreeChecksum,
    		std::map<string,vector<pair<string, string> > > &loadedFileList,
    		bool validationMode=false);
    string findPath(const string &techName) const;

    static string findPath(const string &techName, const vector<string> &pathTechList);
    static bool exists(const string &techName, const vector<string> &pathTechList);

    TechTree(const vector<string> pathList);
    ~TechTree();
    Checksum * getChecksumValue() { return &checksumValue; }

    //get
	int getResourceTypeCount() const							{return (int)resourceTypes.size();}
	int getTypeCount() const									{return (int)factionTypes.size();}
	const FactionType *getType(int i) const						{return &factionTypes[i];}
	const ResourceType *getResourceType(int i) const			{return &resourceTypes[i];}
	string getName(bool translatedValue=false);
	string getNameUntranslated() const;

	string getTranslatedName(string techName, bool forceLoad=false, bool forceTechtreeActiveFile=false);
	string getTranslatedFactionName(string techName, string factionName);

	vector<string> getPathList() const					{return pathList;}
    //const string &getDesc() const								{return desc;}

	const string getPath() const								{return treePath;}

	const FactionType *getType(const string &name) const;
	FactionType *getTypeByName(const string &name);
	const ResourceType *getResourceType(const string &name) const;
    const ResourceType *getTechResourceType(int i) const;
    const ResourceType *getFirstTechResourceType() const;

    const ArmorType *getArmorType(const string &name) const;
	const AttackType *getAttackType(const string &name) const;

    int getArmorTypeCount() const { return (int)armorTypes.size(); }
    const ArmorType * getArmorTypeByIndex(int index) const { return &armorTypes[index]; }
	int getAttackTypeCount() const { return (int)attackTypes.size(); }
	const AttackType * getAttackTypeByIndex(int index) const { return &attackTypes[index]; }

	double getDamageMultiplier(const AttackType *att, const ArmorType *art) const;
	std::vector<std::string> validateFactionTypes();
	std::vector<std::string> validateResourceTypes();

	void saveGame(XmlNode *rootNode);

};

}} //end namespace

#endif
