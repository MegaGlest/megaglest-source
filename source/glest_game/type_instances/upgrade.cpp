// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "upgrade.h"

#include <stdexcept>

#include "unit.h"
#include "util.h"
#include "upgrade_type.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Upgrade
// =====================================================

Upgrade::Upgrade(const UpgradeType *type, int factionIndex) {
	state= usUpgrading;
	this->factionIndex= factionIndex;
	this->type= type;
}

// ============== get ==============

UpgradeState Upgrade::getState() const {
	return state;
}

int Upgrade::getFactionIndex() const {
	return factionIndex;
}

const UpgradeType * Upgrade::getType() const {
	return type;
}

// ============== set ==============

void Upgrade::setState(UpgradeState state) {
     this->state= state;
}

std::string Upgrade::toString() const {
	std::string result = "";

	result += " state = " + intToStr(state) + " factionIndex = " + intToStr(factionIndex);
	if(type != NULL) {
		result += " type = " + type->getReqDesc();
	}

	return result;
}


// =====================================================
// 	class UpgradeManager
// =====================================================

UpgradeManager::~UpgradeManager() {
	upgradesLookup.clear();
	deleteValues(upgrades.begin(), upgrades.end());
}

void UpgradeManager::startUpgrade(const UpgradeType *upgradeType, int factionIndex) {
	Upgrade *upgrade = new Upgrade(upgradeType, factionIndex);
	upgrades.push_back(upgrade);
	upgradesLookup[upgradeType] = upgrades.size()-1;
}

void UpgradeManager::cancelUpgrade(const UpgradeType *upgradeType) {
	map<const UpgradeType *,int>::iterator iterFind = upgradesLookup.find(upgradeType);
	if(iterFind != upgradesLookup.end()) {
		if(iterFind->second >= upgrades.size()) {
			char szBuf[1024]="";
			sprintf(szBuf,"Error canceling upgrade, iterFind->second >= upgrades.size() - [%d] : [%d]",iterFind->second,(int)upgrades.size());
			throw runtime_error("Error canceling upgrade, upgrade not found in upgrade manager");
		}
		int eraseIndex = iterFind->second;
		upgrades.erase(upgrades.begin() + eraseIndex);
		upgradesLookup.erase(upgradeType);

		for(map<const UpgradeType *,int>::iterator iterMap = upgradesLookup.begin();
			iterMap != upgradesLookup.end(); iterMap++) {
			if(iterMap->second >= upgrades.size()) {
				iterMap->second--;
			}
			if(iterMap->second < 0) {
				upgradesLookup.erase(iterMap->first);
			}
		}
	}
	else {
		throw runtime_error("Error canceling upgrade, upgrade not found in upgrade manager");
	}

/*
	Upgrades::iterator it;

	for(it=upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType){
			break;
		}
	}

	if(it!=upgrades.end()){
		upgrades.erase(it);
	}
	else{
		throw runtime_error("Error canceling upgrade, upgrade not found in upgrade manager");
	}
*/
}

void UpgradeManager::finishUpgrade(const UpgradeType *upgradeType) {
	map<const UpgradeType *,int>::iterator iterFind = upgradesLookup.find(upgradeType);
	if(iterFind != upgradesLookup.end()) {
		upgrades[iterFind->second]->setState(usUpgraded);
	}
	else {
		throw runtime_error("Error finishing upgrade, upgrade not found in upgrade manager");
	}


/*
	Upgrades::iterator it;

	for(it=upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType){
			break;
		}
	}

	if(it!=upgrades.end()){
		(*it)->setState(usUpgraded);
	}
	else{
		throw runtime_error("Error finishing upgrade, upgrade not found in upgrade manager");
	}
*/
}

bool UpgradeManager::isUpgradingOrUpgraded(const UpgradeType *upgradeType) const {
	if(upgradesLookup.find(upgradeType) != upgradesLookup.end()) {
		return true;
	}

	return false;

/*
	Upgrades::const_iterator it;
	
	for(it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType){
			return true;
		}
	}

	return false;
*/
}

bool UpgradeManager::isUpgraded(const UpgradeType *upgradeType) const {
	map<const UpgradeType *,int>::const_iterator iterFind = upgradesLookup.find(upgradeType);
	if(iterFind != upgradesLookup.end()) {
		return (upgrades[iterFind->second]->getState() == usUpgraded);
	}
	return false;

/*
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType && (*it)->getState()==usUpgraded){
			return true;
		}
	}
	return false;
*/
}

bool UpgradeManager::isUpgrading(const UpgradeType *upgradeType) const {
	map<const UpgradeType *,int>::const_iterator iterFind = upgradesLookup.find(upgradeType);
	if(iterFind != upgradesLookup.end()) {
		return (upgrades[iterFind->second]->getState() == usUpgrading);
	}
	return false;

/*
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType && (*it)->getState()==usUpgrading){
			return true;
		}
	}
	return false;
*/
}

void UpgradeManager::computeTotalUpgrade(const Unit *unit, TotalUpgrade *totalUpgrade) const {
	totalUpgrade->reset();
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); it++) {
		if((*it)->getFactionIndex() == unit->getFactionIndex()
			&& (*it)->getType()->isAffected(unit->getType())
			&& (*it)->getState()==usUpgraded)
			totalUpgrade->sum((*it)->getType(), unit);
	}

}

std::string UpgradeManager::toString() const {
	std::string result = "UpgradeCount: " + intToStr(this->getUpgradeCount());
	for(int idx = 0; idx < upgrades.size(); idx++) {
		result += " index = " + intToStr(idx) + " " + upgrades[idx]->toString();
	}
	return result;
}

}}// end namespace
