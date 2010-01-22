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

#include "upgrade.h"

#include <stdexcept>

#include "unit.h"
#include "util.h"
#include "upgrade_type.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Upgrade
// =====================================================

Upgrade::Upgrade(const UpgradeType *type, int factionIndex){
	state= usUpgrading;
	this->factionIndex= factionIndex;
	this->type= type;
}

// ============== get ==============

UpgradeState Upgrade::getState() const{
	return state;
}

int Upgrade::getFactionIndex() const{
	return factionIndex;
}

const UpgradeType * Upgrade::getType() const{
	return type;
}

// ============== set ==============

void Upgrade::setState(UpgradeState state){
     this->state= state;
}


// =====================================================
// 	class UpgradeManager
// =====================================================

UpgradeManager::~UpgradeManager(){
	deleteValues(upgrades.begin(), upgrades.end());
}

void UpgradeManager::startUpgrade(const UpgradeType *upgradeType, int factionIndex){
	upgrades.push_back(new Upgrade(upgradeType, factionIndex));
}

void UpgradeManager::cancelUpgrade(const UpgradeType *upgradeType){
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
}

void UpgradeManager::finishUpgrade(const UpgradeType *upgradeType){
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
}

bool UpgradeManager::isUpgradingOrUpgraded(const UpgradeType *upgradeType) const{
	Upgrades::const_iterator it;
	
	for(it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType){
			return true;
		}
	}

	return false;
}

bool UpgradeManager::isUpgraded(const UpgradeType *upgradeType) const{
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType && (*it)->getState()==usUpgraded){
			return true;
		}
	}
	return false;
}

bool UpgradeManager::isUpgrading(const UpgradeType *upgradeType) const{
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getType()==upgradeType && (*it)->getState()==usUpgrading){
			return true;
		}
	}
	return false;
}

void UpgradeManager::computeTotalUpgrade(const Unit *unit, TotalUpgrade *totalUpgrade) const{
	totalUpgrade->reset();
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); it++){
		if((*it)->getFactionIndex()==unit->getFactionIndex()
			&& (*it)->getType()->isAffected(unit->getType())
			&& (*it)->getState()==usUpgraded)
			totalUpgrade->sum((*it)->getType());	
	}

}

}}// end namespace
