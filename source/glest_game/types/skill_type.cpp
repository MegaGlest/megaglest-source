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

#include "skill_type.h"

#include <cassert>

#include "sound.h"
#include "util.h"
#include "lang.h"
#include "renderer.h"
#include "particle_type.h"
#include "unit_particle_type.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

int SkillType::nextAttackBoostId = 0;

AttackBoost::AttackBoost() {
	enabled = false;
	allowMultipleBoosts = false;
	radius = 0;
	targetType = abtFaction;
	unitParticleSystemTypeForSourceUnit = NULL;
	unitParticleSystemTypeForAffectedUnit = NULL;
	includeSelf 	= false;
}

AttackBoost::~AttackBoost() {
	delete unitParticleSystemTypeForSourceUnit;
	unitParticleSystemTypeForSourceUnit = NULL;

	delete unitParticleSystemTypeForAffectedUnit;
	unitParticleSystemTypeForAffectedUnit = NULL;
}

bool AttackBoost::isAffected(const Unit *source, const Unit *dest) const {
	bool result = false;
	if(enabled == true &&
			source != NULL && dest != NULL &&
			(includeSelf == true || source != dest)) {
		bool destUnitMightApply = false;
		if(source == dest && includeSelf == true) {
			destUnitMightApply = true;
		}
		else {
			// All units are affected (including enemies)
			if(targetType == abtAll) {
				destUnitMightApply = (boostUnitList.empty() == true);

				// Specify which units are affected
				for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
					const UnitType *ut = boostUnitList[i];
					if(dest->getType()->getId() == ut->getId()) {
						destUnitMightApply = true;
						break;
					}
				}

			}
			// Only same faction units are affected
			else if(targetType == abtFaction) {
				//if(boostUnitList.empty() == true) {
				if(source->getFactionIndex() == dest->getFactionIndex()) {
					//destUnitMightApply = true;
					destUnitMightApply = (boostUnitList.empty() == true);

					// Specify which units are affected
					for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
						const UnitType *ut = boostUnitList[i];
						if(dest->getType()->getId() == ut->getId()) {
							destUnitMightApply = true;
							break;
						}
					}

				}
				//}
			}
			// Only ally units are affected
			else if(targetType == abtAlly) {
				//if(boostUnitList.empty() == true) {
				if(source->isAlly(dest) == true) {
					//destUnitMightApply = true;
					destUnitMightApply = (boostUnitList.empty() == true);

					// Specify which units are affected
					for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
						const UnitType *ut = boostUnitList[i];
						if(dest->getType()->getId() == ut->getId()) {
							destUnitMightApply = true;
							break;
						}
					}
				}
				//}
			}
			// Only foe units are affected
			else if(targetType == abtFoe) {
				//if(boostUnitList.empty() == true) {
				if(source->isAlly(dest) == false) {
					//destUnitMightApply = true;
					destUnitMightApply = (boostUnitList.empty() == true);

					// Specify which units are affected
					for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
						const UnitType *ut = boostUnitList[i];
						if(dest->getType()->getId() == ut->getId()) {
							destUnitMightApply = true;
							break;
						}
					}
				}
				//}
			}
			else if(targetType == abtUnitTypes) {
				// Specify which units are affected
				for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
					const UnitType *ut = boostUnitList[i];
					if(dest->getType()->getId() == ut->getId()) {
						destUnitMightApply = true;
						break;
					}
				}
			}
		}

		if(destUnitMightApply == true) {
			float distance = source->getCenteredPos().dist(dest->getCenteredPos());
			if(distance <= radius) {
				result = true;
			}
		}
	}

	return result;
}

string AttackBoost::getDesc() const{
	Lang &lang= Lang::getInstance();
    string str= "";
    string indent="  ";
    if(enabled) {
    	if(boostUnitList.empty() == false) {
    		str+= "\n"+ lang.get("Effects")+":\n";
    	}

    	str += indent+lang.get("effectRadius") + ": " + intToStr(radius) +"\n";
    	if(allowMultipleBoosts==false) {
    		string allowIt=lang.get("No");
    		if(allowMultipleBoosts==true)
    			allowIt=lang.get("False");
    		str += indent+lang.get("allowMultiBoost") + ": " + allowIt +"\n";
    	}
    	str+=boostUpgrade.getDesc();

    	if(targetType==abtAlly)
    	{
    		str+= lang.get("AffectedUnitsFromTeam") +":\n";
    	}
    	else if(targetType==abtFoe)
    	{
    		str+= lang.get("AffectedUnitsFromFoe") +":\n";
    	}
    	else if(targetType==abtFaction)
    	{
    		str+= lang.get("AffectedUnitsFromYourFaction") +":\n";
    	}
    	else if(targetType==abtUnitTypes)
    	{
    		str+= lang.get("AffectedUnitsFromAll") +":\n";
    	}
    	else if(targetType==abtAll)
    	{
    		str+= lang.get("AffectedUnitsFromAll") +":\n";
    	}

    	if(boostUnitList.empty() == false) {
			for(int i=0; i<boostUnitList.size(); ++i){
				str+= "  "+boostUnitList[i]->getName(true)+"\n";
			}
    	}
    	else
    	{
    		str+= lang.get("All")+"\n";
    	}

    	return str;
    }
    else
    	return "";
}

void AttackBoost::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *attackBoostNode = rootNode->addChild("AttackBoost");

//	bool enabled;
	attackBoostNode->addAttribute("enabled",intToStr(enabled), mapTagReplacements);
//	bool allowMultipleBoosts;
	attackBoostNode->addAttribute("allowMultipleBoosts",intToStr(allowMultipleBoosts), mapTagReplacements);
//	int radius;
	attackBoostNode->addAttribute("radius",intToStr(radius), mapTagReplacements);
//	AttackBoostTargetType targetType;
	attackBoostNode->addAttribute("targetType",intToStr(targetType), mapTagReplacements);
//	vector<const UnitType *> boostUnitList;
	for(unsigned int i = 0; i < boostUnitList.size(); ++i) {
		const UnitType *ut = boostUnitList[i];
		XmlNode *unitTypeNode = attackBoostNode->addChild("UnitType");
		unitTypeNode->addAttribute("name",ut->getName(), mapTagReplacements);
	}
//	UpgradeTypeBase boostUpgrade;
	boostUpgrade.saveGame(attackBoostNode);
//	UnitParticleSystemType *unitParticleSystemTypeForSourceUnit;
	if(unitParticleSystemTypeForSourceUnit != NULL) {
		unitParticleSystemTypeForSourceUnit->saveGame(attackBoostNode);
	}
//	UnitParticleSystemType *unitParticleSystemTypeForAffectedUnit;
	if(unitParticleSystemTypeForAffectedUnit != NULL) {
		unitParticleSystemTypeForAffectedUnit->saveGame(attackBoostNode);
	}

//	bool includeSelf;
	attackBoostNode->addAttribute("includeSelf",intToStr(includeSelf), mapTagReplacements);
//	string name;
	attackBoostNode->addAttribute("name",name, mapTagReplacements);
}

// =====================================================
// 	class SkillType
// =====================================================

SkillType::~SkillType() {
	deleteValues(sounds.getSounds().begin(), sounds.getSounds().end());
	//remove unitParticleSystemTypes
	while(!unitParticleSystemTypes.empty()) {
		delete unitParticleSystemTypes.back();
		unitParticleSystemTypes.pop_back();
	}
}

const XmlNode * SkillType::findAttackBoostDetails(string attackBoostName,
		const XmlNode *attackBoostsNode,const XmlNode *attackBoostNode) {
	const XmlNode *result = attackBoostNode;

	if(attackBoostsNode != NULL && attackBoostName != "") {
		for(int i = 0; i < attackBoostsNode->getChildCount(); ++i) {
			const XmlNode *abn= attackBoostsNode->getChild("attack-boost", i);

			string sharedName = abn->getAttribute("name")->getRestrictedValue();
			if(sharedName == attackBoostName) {
				result = abn;
				break;
			}
		}

	}

	return result;
}

void SkillType::loadAttackBoost(const XmlNode *attackBoostsNode, const XmlNode *attackBoostNode,
		const FactionType *ft, string parentLoader, const string & dir,
		string currentPath, std::map<string,vector<pair<string,string> > > & loadedFileList,
		const TechTree *tt) {

	attackBoost.enabled = true;
    if(attackBoostNode->hasAttribute("name") == true) {
        attackBoost.name = attackBoostNode->getAttribute("name")->getRestrictedValue();

        attackBoostNode = findAttackBoostDetails(attackBoost.name,attackBoostsNode,attackBoostNode);
    }
    else {
        attackBoost.name = "attack-boost-autoname-" + intToStr(getNextAttackBoostId());
    }
    string targetType = attackBoostNode->getChild("target")->getAttribute("value")->getValue();

    attackBoost.allowMultipleBoosts = false;
    if(attackBoostNode->hasChild("allow-multiple-boosts") == true) {
    	attackBoost.allowMultipleBoosts = attackBoostNode->getChild("allow-multiple-boosts")->getAttribute("value")->getBoolValue();
    }

    attackBoost.radius = attackBoostNode->getChild("radius")->getAttribute("value")->getIntValue();

    attackBoost.includeSelf = false;
    if(attackBoostNode->getChild("target")->hasAttribute("include-self") == true) {
        attackBoost.includeSelf = attackBoostNode->getChild("target")->getAttribute("include-self")->getBoolValue();
    }

    if(targetType == "ally") {
        attackBoost.targetType = abtAlly;
        for(int i = 0;i < attackBoostNode->getChild("target")->getChildCount();++i) {
            const XmlNode *boostUnitNode = attackBoostNode->getChild("target")->getChild("unit-type", i);
            attackBoost.boostUnitList.push_back(ft->getUnitType(boostUnitNode->getAttribute("name")->getRestrictedValue()));
        }
    }
    else if(targetType == "foe") {
		attackBoost.targetType = abtFoe;
		for(int i = 0;i < attackBoostNode->getChild("target")->getChildCount();++i) {
			const XmlNode *boostUnitNode = attackBoostNode->getChild("target")->getChild("unit-type", i);
			attackBoost.boostUnitList.push_back(ft->getUnitType(boostUnitNode->getAttribute("name")->getRestrictedValue()));
		}
	}
	else if(targetType == "faction") {
		attackBoost.targetType = abtFaction;
		for(int i = 0;i < attackBoostNode->getChild("target")->getChildCount();++i) {
			const XmlNode *boostUnitNode = attackBoostNode->getChild("target")->getChild("unit-type", i);
			attackBoost.boostUnitList.push_back(ft->getUnitType(boostUnitNode->getAttribute("name")->getRestrictedValue()));
		}
	}
	else if(targetType == "unit-types") {
		attackBoost.targetType = abtUnitTypes;
		for(int i = 0;i < attackBoostNode->getChild("target")->getChildCount();++i) {
			const XmlNode *boostUnitNode = attackBoostNode->getChild("target")->getChild("unit-type", i);
			attackBoost.boostUnitList.push_back(ft->getUnitType(boostUnitNode->getAttribute("name")->getRestrictedValue()));
		}
	}
	else if(targetType == "all") {
		attackBoost.targetType = abtAll;
		for(int i = 0;i < attackBoostNode->getChild("target")->getChildCount();++i) {
			const XmlNode *boostUnitNode = attackBoostNode->getChild("target")->getChild("unit-type", i);
			attackBoost.boostUnitList.push_back(ft->getUnitType(boostUnitNode->getAttribute("name")->getRestrictedValue()));
		}
	}
	else {
		char szBuf[8096] = "";
		snprintf(szBuf, 8096,"Unsupported target [%s] specified for attack boost for skill [%s] in [%s]", targetType.c_str(), name.c_str(), parentLoader.c_str());
		throw megaglest_runtime_error(szBuf);
	}

    attackBoost.boostUpgrade.load(attackBoostNode,attackBoost.name);
    if(attackBoostNode->hasChild("particles") == true) {
        const XmlNode *particleNode = attackBoostNode->getChild("particles");
        bool particleEnabled = particleNode->getAttribute("value")->getBoolValue();
        if(particleEnabled == true) {
            if(particleNode->hasChild("originator-particle-file") == true){
                const XmlNode *particleFileNode = particleNode->getChild("originator-particle-file");
                string path = particleFileNode->getAttribute("path")->getRestrictedValue();
                attackBoost.unitParticleSystemTypeForSourceUnit = new UnitParticleSystemType();
                attackBoost.unitParticleSystemTypeForSourceUnit->load(particleFileNode, dir, currentPath + path, &Renderer::getInstance(), loadedFileList, parentLoader, tt->getPath());
                loadedFileList[currentPath + path].push_back(make_pair(parentLoader, particleFileNode->getAttribute("path")->getRestrictedValue()));
            }
            if(particleNode->hasChild("affected-particle-file") == true) {
                const XmlNode *particleFileNode = particleNode->getChild("affected-particle-file");
                string path = particleFileNode->getAttribute("path")->getRestrictedValue();
                attackBoost.unitParticleSystemTypeForAffectedUnit = new UnitParticleSystemType();
                attackBoost.unitParticleSystemTypeForAffectedUnit->load(particleFileNode, dir, currentPath + path, &Renderer::getInstance(), loadedFileList, parentLoader, tt->getPath());
                loadedFileList[currentPath + path].push_back(make_pair(parentLoader, particleFileNode->getAttribute("path")->getRestrictedValue()));
            }
        }
    }
}

void SkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt, const FactionType *ft,
		std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	//name
	name= sn->getChild("name")->getAttribute("value")->getRestrictedValue();

	//ep cost
	if(sn->hasChild("ep-cost") == true) {
		mpCost = sn->getChild("ep-cost")->getAttribute("value")->getIntValue();
	}
	else {
		mpCost = 0;
	}

	if (sn->hasChild("hp-cost") == true) {
		hpCost = sn->getChild("hp-cost")->getAttribute("value")->getIntValue();
	}
	else {
		hpCost = 0;
	}

	//speed
	if(sn->hasChild("speed") == true) {
		speed = sn->getChild("speed")->getAttribute("value")->getIntValue();
	}
	else {
		speed = 0;
	}

	//anim speed
	if(sn->hasChild("anim-speed") == true) {
		animSpeed = sn->getChild("anim-speed")->getAttribute("value")->getIntValue();
	}
	else {
		animSpeed = 0;
	}

	//model
	string currentPath = dir;
	endPathWithSlash(currentPath);

	animationRandomCycleMaxcount = -1;
	if(sn->hasChild("animation-random-cycle-maxcount") == true) {
		const XmlNode *randomCycleCountNode = sn->getChild("animation-random-cycle-maxcount");
		animationRandomCycleMaxcount = randomCycleCountNode->getAttribute("value")->getIntValue();
	}

	if(sn->hasChild("animation") == true) {
		//string path= sn->getChild("animation")->getAttribute("path")->getRestrictedValue(currentPath);
		vector<XmlNode *> animationList = sn->getChildList("animation");
		for(unsigned int i = 0; i < animationList.size(); ++i) {
			string path= animationList[i]->getAttribute("path")->getRestrictedValue(currentPath);
			if(fileExists(path) == true) {
				Model *animation= Renderer::getInstance().newModel(rsGame);
				if(animation) {
					animation->load(path, false, &loadedFileList, &parentLoader);
				}
				loadedFileList[path].push_back(make_pair(parentLoader,animationList[i]->getAttribute("path")->getRestrictedValue()));

				animations.push_back(animation);
				//printf("**FOUND ANIMATION [%s]\n",path.c_str());

				AnimationAttributes animationAttributeList;
				if(animationList[i]->getAttribute("minHp",false) != NULL && animationList[i]->getAttribute("maxHp",false) != NULL) {
					animationAttributeList.fromHp = animationList[i]->getAttribute("minHp")->getIntValue();
					animationAttributeList.toHp = animationList[i]->getAttribute("maxHp")->getIntValue();
				}
				animationAttributes.push_back(animationAttributeList);
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] WARNING CANNOT LOAD MODEL [%s] for parentLoader [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),parentLoader.c_str());
			}
		}
		if(animations.empty() == true) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"Error no animations found for skill [%s] for parentLoader [%s]",name.c_str(),parentLoader.c_str());
			throw megaglest_runtime_error(szBuf);
		}
	}

	//particles
	if(sn->hasChild("particles")) {
		const XmlNode *particleNode= sn->getChild("particles");
		bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
		if(particleEnabled) {
			for(int i = 0; i < particleNode->getChildCount(); ++i) {
				const XmlNode *particleFileNode= particleNode->getChild("particle-file", i);
				string path= particleFileNode->getAttribute("path")->getRestrictedValue();
				UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();
				unitParticleSystemType->load(particleFileNode, dir, currentPath + path, &Renderer::getInstance(),
						loadedFileList,parentLoader,tt->getPath());

				if(particleNode->getAttribute("start-time",false) != NULL) {
					//printf("*NOTE particle system type has start-time [%f]\n",particleNode->getAttribute("start-time")->getFloatValue());
					unitParticleSystemType->setStartTime(particleNode->getAttribute("start-time")->getFloatValue());
				}
				if(particleNode->getAttribute("end-time",false) != NULL) {
					//printf("*NOTE particle system type has end-time [%f]\n",particleNode->getAttribute("end-time")->getFloatValue());
					unitParticleSystemType->setEndTime(particleNode->getAttribute("end-time")->getFloatValue());
				}

				loadedFileList[currentPath + path].push_back(make_pair(parentLoader,particleFileNode->getAttribute("path")->getRestrictedValue()));
				unitParticleSystemTypes.push_back(unitParticleSystemType);
			}
		}
	}

	//sound
	if(sn->hasChild("sound")) {
		const XmlNode *soundNode= sn->getChild("sound");
		if(soundNode->getAttribute("enabled")->getBoolValue()) {
			soundStartTime= soundNode->getAttribute("start-time")->getFloatValue();
			sounds.resize(soundNode->getChildCount());
			for(int i = 0; i < soundNode->getChildCount(); ++i) {
				const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue(currentPath, true);

				StaticSound *sound= new StaticSound();
				sound->load(path);
				loadedFileList[path].push_back(make_pair(parentLoader,soundFileNode->getAttribute("path")->getRestrictedValue()));
				sounds[i]= sound;
			}
		}
	}


	// attack-boost
	if(sn->hasChild("attack-boost") == true) {
		//printf("$$FOUND ATTACK BOOST FOR [%s]\n",parentLoader.c_str());
		const XmlNode *attackBoostNode = sn->getChild("attack-boost");
		loadAttackBoost(attackBoostsNode, attackBoostNode, ft, parentLoader, dir, currentPath, loadedFileList, tt);
	}
}

bool SkillType::CanCycleNextRandomAnimation(const int *animationRandomCycleCount) const {
	bool result = true;
	if(animations.size() > 1) {
		if( animationRandomCycleMaxcount >= 0 &&
				animationRandomCycleCount != NULL &&
			*animationRandomCycleCount >= animationRandomCycleMaxcount) {
			result = false;
		}
	}
	return result;
}

const AnimationAttributes SkillType::getAnimationAttribute(int index) const {
	return animationAttributes[index];
}

Model *SkillType::getAnimation(float animProgress, const Unit *unit,
		int *lastAnimationIndex, int *animationRandomCycleCount) const {
	int modelIndex = 0;
	//printf("Count [%d] animProgress = [%f] for skill [%s] animationRandomCycleCount = %d\n",animations.size(),animProgress,name.c_str(),*animationRandomCycleCount);
	if(animations.size() > 1) {
		//printf("animProgress = [%f] for skill [%s] animationRandomCycleCount = %d\n",animProgress,name.c_str(),*animationRandomCycleCount);

		if(lastAnimationIndex) {
			modelIndex = *lastAnimationIndex;
		}
		if(modelIndex < 0 || animProgress > 1.0f) {
			bool canCycle = CanCycleNextRandomAnimation(animationRandomCycleCount);
			if(canCycle == true) {
				vector<int> filteredAnimations;
				bool foundSpecificAnimation = false;
				if(unit != NULL) {
					for(unsigned int i = 0; i < animationAttributes.size(); ++i) {
						const AnimationAttributes &attributes = animationAttributes[i];
						if(attributes.fromHp != 0 || attributes.toHp != 0) {
							if(unit->getHp() >= attributes.fromHp && unit->getHp() <= attributes.toHp) {
								modelIndex = i;
								foundSpecificAnimation = true;
								filteredAnimations.push_back(i);
								//printf("SELECTING Model index = %d [%s] model attributes [%d to %d] for unit [%s - %d] with HP = %d\n",i,animations[modelIndex]->getFileName().c_str(),attributes.fromHp,attributes.toHp,unit->getType()->getName().c_str(),unit->getId(),unit->getHp());
								//break;
							}
						}
					}
					// Need to make sure the filtered list does NOT include any
					// models with min/max hp
					if(foundSpecificAnimation == false) {
						for(unsigned int i = 0; i < animationAttributes.size(); ++i) {
							const AnimationAttributes &attributes = animationAttributes[i];
							if(attributes.fromHp == 0 && attributes.toHp == 0) {
								modelIndex = i;
								foundSpecificAnimation = true;
								filteredAnimations.push_back(i);
									//printf("SELECTING Model index = %d [%s] model attributes [%d to %d] for unit [%s - %d] with HP = %d\n",i,animations[modelIndex]->getFileName().c_str(),attributes.fromHp,attributes.toHp,unit->getType()->getName().c_str(),unit->getId(),unit->getHp());
									//break;
							}
						}
					}
				}

				if(foundSpecificAnimation == false) {
					//int modelIndex = random.randRange(0,animations.size()-1);
					Chrono seed(true);
					srand((unsigned int)seed.getCurTicks() + (unit != NULL ? unit->getId() : 0));

					modelIndex = rand() % animations.size();

					//const AnimationAttributes &attributes = animationAttributes[modelIndex];
					//printf("SELECTING RANDOM Model index = %d [%s] model attributes [%d to %d] for unit [%s - %d] with HP = %d\n",modelIndex,animations[modelIndex]->getFileName().c_str(),attributes.fromHp,attributes.toHp,unit->getType()->getName().c_str(),unit->getId(),unit->getHp());
				}
				else {
					Chrono seed(true);
					srand((unsigned int)seed.getCurTicks() + unit->getId());

					int filteredModelIndex = rand() % filteredAnimations.size();
					modelIndex = filteredAnimations[filteredModelIndex];
				}
			}
		}
	}
	if(lastAnimationIndex) {
		if(*lastAnimationIndex != modelIndex) {
			//printf("Switching model from [%s] to [%s]\n",(*lastAnimationIndex >= 0 ? animations[*lastAnimationIndex]->getFileName().c_str() : "none"),animations[modelIndex]->getFileName().c_str());
		}
		*lastAnimationIndex = modelIndex;
	}

	//printf("!!RETURN ANIMATION [%d / %d]\n",modelIndex,animations.size()-1);
	return animations[modelIndex];
}

string SkillType::skillClassToStr(SkillClass skillClass) {
	switch(skillClass){
	case scStop: return "Stop";
	case scMove: return "Move";
	case scAttack: return "Attack";
	case scHarvest: return "Harvest";
	case scRepair: return "Repair";
	case scBuild: return "Build";
	case scDie: return "Die";
	case scBeBuilt: return "Be Built";
	case scProduce: return "Produce";
	case scUpgrade: return "Upgrade";
	case scFogOfWar: return "Fog Of War";
	default:
		assert(false);
		break;
	}
	return "";
}

string SkillType::fieldToStr(Field field) {
	Lang &lang= Lang::getInstance();
	string fieldName = "";
	switch(field) {
		case fLand:
			if(lang.hasString("FieldLand") == true) {
				fieldName = lang.get("FieldLand");
			}
			else {
				fieldName = "Land";
			}
			//return "Land";
			return lang.getTilesetString("FieldLandName",fieldName.c_str());

		case fAir:
			if(lang.hasString("FieldAir") == true) {
				fieldName = lang.get("FieldAir");
			}
			else {
				fieldName = "Air";
			}

			//return "Air";
			return lang.getTilesetString("FieldAirName",fieldName.c_str());
		default:
			assert(false);
			break;
	}
	return "";
}

void SkillType::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *skillTypeNode = rootNode->addChild("SkillType");

//    SkillClass skillClass;
	skillTypeNode->addAttribute("skillClass",intToStr(skillClass), mapTagReplacements);
//	string name;
	skillTypeNode->addAttribute("name",name, mapTagReplacements);
//	int mpCost;
	skillTypeNode->addAttribute("mpCost",intToStr(mpCost), mapTagReplacements);
//	int hpCost;
	skillTypeNode->addAttribute("hpCost",intToStr(hpCost), mapTagReplacements);
//    int speed;
	skillTypeNode->addAttribute("speed",intToStr(speed), mapTagReplacements);
//    int animSpeed;
	skillTypeNode->addAttribute("animSpeed",intToStr(animSpeed), mapTagReplacements);
//    int animationRandomCycleMaxcount;
	skillTypeNode->addAttribute("animationRandomCycleMaxcount",intToStr(animationRandomCycleMaxcount), mapTagReplacements);
//    vector<Model *> animations;
//    vector<AnimationAttributes> animationAttributes;
//
//    SoundContainer sounds;
//	float soundStartTime;
	skillTypeNode->addAttribute("soundStartTime",floatToStr(soundStartTime), mapTagReplacements);
//	RandomGen random;
	skillTypeNode->addAttribute("random",intToStr(random.getLastNumber()), mapTagReplacements);
//	AttackBoost attackBoost;
	attackBoost.saveGame(skillTypeNode);
//	static int nextAttackBoostId;
	skillTypeNode->addAttribute("nextAttackBoostId",intToStr(nextAttackBoostId), mapTagReplacements);
//	UnitParticleSystemTypes unitParticleSystemTypes;
	for(UnitParticleSystemTypes::iterator it = unitParticleSystemTypes.begin(); it != unitParticleSystemTypes.end(); ++it) {
		(*it)->saveGame(skillTypeNode);
	}
}

// =====================================================
// 	class StopSkillType
// =====================================================

StopSkillType::StopSkillType(){
    skillClass= scStop;
}

string StopSkillType::toString() const{
	return Lang::getInstance().get("Stop");
}

// =====================================================
// 	class MoveSkillType
// =====================================================

MoveSkillType::MoveSkillType(){
	skillClass= scMove;
}

string MoveSkillType::toString() const{
	return Lang::getInstance().get("Move");
}

int MoveSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getMoveSpeed(this);
	result = max(0,result);
	return result;
}

// =====================================================
// 	class AttackSkillType
// =====================================================

//varios
AttackSkillType::AttackSkillType() {
    skillClass= scAttack;
	attackType= NULL;
    projectile= false;
    splash= false;
    splashRadius= 0;
    spawnUnit="";
    spawnUnitcount=0;
	projectileParticleSystemType= NULL;
	splashParticleSystemType= NULL;

	for(int i = 0; i < fieldCount; ++i) {
		attackFields[i]= false;
	}

	attackStrength=0;
	attackVar=0;
	attackRange=0;
	attackStartTime=0;
	splashDamageAll=false;
}

AttackSkillType::~AttackSkillType() {
	delete projectileParticleSystemType;
	projectileParticleSystemType = NULL;

	delete splashParticleSystemType;
	splashParticleSystemType = NULL;
	deleteValues(projSounds.getSounds().begin(), projSounds.getSounds().end());
	projSounds.clearSounds();
}

void AttackSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
    SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	string currentPath = dir;
	endPathWithSlash(currentPath);

	//misc
	attackStrength= sn->getChild("attack-strenght")->getAttribute("value")->getIntValue();
    attackVar= sn->getChild("attack-var")->getAttribute("value")->getIntValue();

    if(attackVar < 0) {
        char szBuf[8096]="";
        snprintf(szBuf,8096,"The attack skill has an INVALID attack var value which is < 0 [%d] in file [%s]!",attackVar,dir.c_str());
        throw megaglest_runtime_error(szBuf);
    }

    attackRange= sn->getChild("attack-range")->getAttribute("value")->getIntValue();
	string attackTypeName= sn->getChild("attack-type")->getAttribute("value")->getRestrictedValue();
	attackType= tt->getAttackType(attackTypeName);
	attackStartTime= sn->getChild("attack-start-time")->getAttribute("value")->getFloatValue();

	if (sn->hasChild("unit")) {
		spawnUnit = sn->getChild("unit")->getAttribute("value")->getValue();
		spawnUnitcount
				= sn->getChild("unit")->getAttribute("amount")->getIntValue();
	} else {
		spawnUnit = "";
		spawnUnitcount = 0;
	}
	//attack fields
	const XmlNode *attackFieldsNode= sn->getChild("attack-fields");
	for(int i=0; i<attackFieldsNode->getChildCount(); ++i){
		const XmlNode *fieldNode= attackFieldsNode->getChild("field", i);
		string fieldName= fieldNode->getAttribute("value")->getRestrictedValue();
		if(fieldName=="land"){
			attackFields[fLand]= true;
		}
		else if(fieldName=="air"){
			attackFields[fAir]= true;
		}
		else{
			throw megaglest_runtime_error("Not a valid field: "+fieldName+": "+ dir);
		}
	}

	//projectile
	const XmlNode *projectileNode= sn->getChild("projectile");
	projectile= projectileNode->getAttribute("value")->getBoolValue();
	if(projectile){

		//proj particle
		const XmlNode *particleNode= projectileNode->getChild("particle");
		bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
		if(particleEnabled){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(particleNode, dir, currentPath + path,
					&Renderer::getInstance(), loadedFileList, parentLoader,
					tt->getPath());
		}

		//proj sounds
		const XmlNode *soundNode= projectileNode->getChild("sound");
		if(soundNode->getAttribute("enabled")->getBoolValue()){

			projSounds.resize(soundNode->getChildCount());
			for(int i=0; i<soundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue(currentPath, true);
				//printf("\n\n\n\n!@#$ ---> parentLoader [%s] path [%s] nodeValue [%s] i = %d",parentLoader.c_str(),path.c_str(),soundFileNode->getAttribute("path")->getRestrictedValue().c_str(),i);

				StaticSound *sound= new StaticSound();
				sound->load(path);
				loadedFileList[path].push_back(make_pair(parentLoader,soundFileNode->getAttribute("path")->getRestrictedValue()));
				projSounds[i]= sound;
			}
		}
	}

	//splash
	const XmlNode *splashNode= sn->getChild("splash");
	splash= splashNode->getAttribute("value")->getBoolValue();
	if(splash){
		splashRadius= splashNode->getChild("radius")->getAttribute("value")->getIntValue();
		splashDamageAll= splashNode->getChild("damage-all")->getAttribute("value")->getBoolValue();

		//splash particle
		const XmlNode *particleNode= splashNode->getChild("particle");
		bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
		if(particleEnabled){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(particleNode, dir, currentPath + path,
					&Renderer::getInstance(),loadedFileList, parentLoader,
					tt->getPath());
		}
	}
}

string AttackSkillType::toString() const{
	return Lang::getInstance().get("Attack");
}

//get totals
int AttackSkillType::getTotalAttackStrength(const TotalUpgrade *totalUpgrade) const{
	int result = attackStrength + totalUpgrade->getAttackStrength(this);
	result = max(0,result);
	return result;
}

int AttackSkillType::getTotalAttackRange(const TotalUpgrade *totalUpgrade) const{
	int result = attackRange + totalUpgrade->getAttackRange(this);
	result = max(0,result);
	return result;
}

void AttackSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *attackSkillTypeNode = rootNode->addChild("AttackSkillType");

//    int attackStrength;
	attackSkillTypeNode->addAttribute("attackStrength",intToStr(attackStrength), mapTagReplacements);
//    int attackVar;
	attackSkillTypeNode->addAttribute("attackVar",intToStr(attackVar), mapTagReplacements);
//    int attackRange;
	attackSkillTypeNode->addAttribute("attackRange",intToStr(attackRange), mapTagReplacements);
//	const AttackType *attackType;
	if(attackType != NULL) {
		attackSkillTypeNode->addAttribute("attackType",attackType->getName(), mapTagReplacements);
	}
//	bool attackFields[fieldCount];
	for(unsigned int i = 0; i < fieldCount; ++i) {
		XmlNode *attackFieldsNode = attackSkillTypeNode->addChild("attackFields");
		attackFieldsNode->addAttribute("key",intToStr(i), mapTagReplacements);
		attackFieldsNode->addAttribute("value",intToStr(attackFields[i]), mapTagReplacements);
	}
//	float attackStartTime;
	attackSkillTypeNode->addAttribute("attackStartTime",floatToStr(attackStartTime), mapTagReplacements);
//	string spawnUnit;
	attackSkillTypeNode->addAttribute("spawnUnit",spawnUnit, mapTagReplacements);
//	int spawnUnitcount;
	attackSkillTypeNode->addAttribute("spawnUnitcount",intToStr(spawnUnitcount), mapTagReplacements);
//    bool projectile;
	attackSkillTypeNode->addAttribute("projectile",intToStr(projectile), mapTagReplacements);
//    ParticleSystemTypeProjectile* projectileParticleSystemType;
	if(projectileParticleSystemType != NULL) {
		projectileParticleSystemType->saveGame(attackSkillTypeNode);
	}
//	SoundContainer projSounds;
//
//    bool splash;
	attackSkillTypeNode->addAttribute("splash",intToStr(splash), mapTagReplacements);
//    int splashRadius;
	attackSkillTypeNode->addAttribute("splashRadius",intToStr(splashRadius), mapTagReplacements);
//    bool splashDamageAll;
	attackSkillTypeNode->addAttribute("splashDamageAll",intToStr(splashDamageAll), mapTagReplacements);
//    ParticleSystemTypeSplash* splashParticleSystemType;
	if(splashParticleSystemType != NULL) {
		splashParticleSystemType->saveGame(attackSkillTypeNode);
	}
}
// =====================================================
// 	class BuildSkillType
// =====================================================

BuildSkillType::BuildSkillType(){
    skillClass= scBuild;
}

string BuildSkillType::toString() const{
	return Lang::getInstance().get("Build");
}

// =====================================================
// 	class HarvestSkillType
// =====================================================

HarvestSkillType::HarvestSkillType(){
    skillClass= scHarvest;
}

string HarvestSkillType::toString() const{
	return Lang::getInstance().get("Harvest");
}

// =====================================================
// 	class RepairSkillType
// =====================================================

RepairSkillType::RepairSkillType(){
    skillClass= scRepair;
}

string RepairSkillType::toString() const{
	return Lang::getInstance().get("Repair");
}

// =====================================================
// 	class ProduceSkillType
// =====================================================

ProduceSkillType::ProduceSkillType(){
    skillClass= scProduce;
    animProgressBound=false;
}

void ProduceSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	if(sn->hasChild("anim-progress-bound")){
		animProgressBound= sn->getChild("anim-progress-bound")->getAttribute("value")->getBoolValue();
	}
	else {
		animProgressBound=false;
	}
}


string ProduceSkillType::toString() const{
	return Lang::getInstance().get("Produce");
}

int ProduceSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getProdSpeed(this);
	result = max(0,result);
	return result;
}

void ProduceSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *produceSkillTypeNode = rootNode->addChild("ProduceSkillType");

	produceSkillTypeNode->addAttribute("animProgressBound",intToStr(animProgressBound), mapTagReplacements);
}
// =====================================================
// 	class UpgradeSkillType
// =====================================================

UpgradeSkillType::UpgradeSkillType(){
    skillClass= scUpgrade;
    animProgressBound=false;
}

void UpgradeSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	if(sn->hasChild("anim-progress-bound")){
		animProgressBound= sn->getChild("anim-progress-bound")->getAttribute("value")->getBoolValue();
	}
	else {
		animProgressBound=false;
	}
}

string UpgradeSkillType::toString() const{
	return Lang::getInstance().get("Upgrade");
}

int UpgradeSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getProdSpeed(this);
	result = max(0,result);
	return result;
}

void UpgradeSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *upgradeSkillTypeNode = rootNode->addChild("UpgradeSkillType");

	upgradeSkillTypeNode->addAttribute("animProgressBound",intToStr(animProgressBound), mapTagReplacements);
}

// =====================================================
// 	class BeBuiltSkillType
// =====================================================

BeBuiltSkillType::BeBuiltSkillType(){
    skillClass= scBeBuilt;
    animProgressBound=false;
}

void BeBuiltSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	if(sn->hasChild("anim-progress-bound")){
		animProgressBound= sn->getChild("anim-progress-bound")->getAttribute("value")->getBoolValue();
	}
	else if(sn->hasChild("anim-hp-bound")){ // deprecated!!!! remove it when you see it after 15th July 2011
		animProgressBound= sn->getChild("anim-hp-bound")->getAttribute("value")->getBoolValue();
	}
	else {
		animProgressBound=false;
	}
}


string BeBuiltSkillType::toString() const{
	return "Be built";
}

void BeBuiltSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *beBuiltSkillTypeNode = rootNode->addChild("BeBuiltSkillType");

	beBuiltSkillTypeNode->addAttribute("animProgressBound",intToStr(animProgressBound), mapTagReplacements);
}

// =====================================================
// 	class MorphSkillType
// =====================================================

MorphSkillType::MorphSkillType(){
    skillClass= scMorph;
    animProgressBound=false;
}

void MorphSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	if(sn->hasChild("anim-progress-bound")){
		animProgressBound= sn->getChild("anim-progress-bound")->getAttribute("value")->getBoolValue();
	}
	else {
		animProgressBound=false;
	}
}

string MorphSkillType::toString() const{
	return "Morph";
}

int MorphSkillType::getTotalSpeed(const TotalUpgrade *totalUpgrade) const{
	int result = speed + totalUpgrade->getProdSpeed(this);
	result = max(0,result);
	return result;
}

void MorphSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *morphSkillTypeNode = rootNode->addChild("MorphSkillType");

	morphSkillTypeNode->addAttribute("animProgressBound",intToStr(animProgressBound), mapTagReplacements);
}

// =====================================================
// 	class DieSkillType
// =====================================================

DieSkillType::DieSkillType(){
    skillClass= scDie;
    fade=false;
}

void DieSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	fade= sn->getChild("fade")->getAttribute("value")->getBoolValue();
}

string DieSkillType::toString() const{
	return "Die";
}

void DieSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *dieSkillTypeNode = rootNode->addChild("DieSkillType");

	dieSkillTypeNode->addAttribute("fade",intToStr(fade), mapTagReplacements);
}


// =====================================================
// 	class FogOfWarSkillType
// =====================================================

FogOfWarSkillType::FogOfWarSkillType(){
    skillClass= scFogOfWar;

	fowEnable = false;
	applyToTeam = false;
	durationTime = 0;
}

void FogOfWarSkillType::load(const XmlNode *sn, const XmlNode *attackBoostsNode,
		const string &dir, const TechTree *tt,
		const FactionType *ft, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader) {
	SkillType::load(sn, attackBoostsNode,dir, tt, ft, loadedFileList, parentLoader);

	fowEnable = sn->getChild("enable-fog")->getAttribute("value")->getBoolValue();
	applyToTeam = sn->getChild("apply-team")->getAttribute("value")->getBoolValue();
	durationTime = sn->getChild("duration")->getAttribute("value")->getFloatValue();
}

string FogOfWarSkillType::toString() const{
	return "FogOfWar";
}

void FogOfWarSkillType::saveGame(XmlNode *rootNode) {
	SkillType::saveGame(rootNode);
	std::map<string,string> mapTagReplacements;
	XmlNode *fogSkillTypeNode = rootNode->addChild("FogOfWarSkillType");

	fogSkillTypeNode->addAttribute("enable-fog",intToStr(fowEnable), mapTagReplacements);
	fogSkillTypeNode->addAttribute("apply-team",intToStr(applyToTeam), mapTagReplacements);
	fogSkillTypeNode->addAttribute("duration",floatToStr(durationTime), mapTagReplacements);
}

// =====================================================
// 	class SkillTypeFactory
// =====================================================

SkillTypeFactory::SkillTypeFactory(){
	registerClass<StopSkillType>("stop");
	registerClass<MoveSkillType>("move");
	registerClass<AttackSkillType>("attack");
	registerClass<BuildSkillType>("build");
	registerClass<BeBuiltSkillType>("be_built");
	registerClass<HarvestSkillType>("harvest");
	registerClass<RepairSkillType>("repair");
	registerClass<ProduceSkillType>("produce");
	registerClass<UpgradeSkillType>("upgrade");
	registerClass<MorphSkillType>("morph");
	registerClass<DieSkillType>("die");
	registerClass<FogOfWarSkillType>("fog_of_war");
}

SkillTypeFactory &SkillTypeFactory::getInstance(){
	static SkillTypeFactory ctf;
	return ctf;
}

}} //end namespace
