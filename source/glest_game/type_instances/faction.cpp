// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "faction.h"

#include <algorithm>
#include <cassert>

#include "resource_type.h"
#include "unit.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tech_tree.h"
#include "leak_dumper.h"
#include "game.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Faction
// =====================================================

Faction::Faction() {
	texture = NULL;
}

Faction::~Faction() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Renderer &renderer= Renderer::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//renderer.endTexture(rsGame,texture);
	//texture->end();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//delete texture;
	texture = NULL;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Faction::init(
	const FactionType *factionType, ControlType control, TechTree *techTree, Game *game,
	int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction, bool giveResources)
{
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->control= control;
	this->factionType= factionType;
	this->startLocationIndex= startLocationIndex;
	this->index= factionIndex;
	this->teamIndex= teamIndex;
	this->thisFaction= thisFaction;
	this->world= game->getWorld();
	this->scriptManager= game->getScriptManager();
	

	resources.resize(techTree->getResourceTypeCount());
	store.resize(techTree->getResourceTypeCount());
	for(int i=0; i<techTree->getResourceTypeCount(); ++i){
		const ResourceType *rt= techTree->getResourceType(i);
		int resourceAmount= giveResources? factionType->getStartingResourceAmount(rt): 0;
		resources[i].init(rt, resourceAmount);
		store[i].init(rt, 0);
	}

	texture= Renderer::getInstance().newTexture2D(rsGame);
	texture->load("data/core/faction_textures/faction"+intToStr(index)+".tga");

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Faction::end(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	deleteValues(units.begin(), units.end());
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ================== get ==================

const Resource *Faction::getResource(const ResourceType *rt) const{
	for(int i=0; i<resources.size(); ++i){
		if(rt==resources[i].getType()){
			return &resources[i];
		}
	}
	assert(false);
	return NULL;
}

int Faction::getStoreAmount(const ResourceType *rt) const{
	for(int i=0; i<store.size(); ++i){
		if(rt==store[i].getType()){
			return store[i].getAmount();
		}
	}
	assert(false);
	return 0;
}

bool Faction::getCpuControl(bool enableServerControlledAI,bool isNetworkGame, NetworkRole role) const {
	bool result = false;
	if(enableServerControlledAI == false || isNetworkGame == false) {
			result = (control == ctCpuEasy ||control == ctCpu || control == ctCpuUltra || control == ctCpuMega);
	}
	else {
		if(isNetworkGame == true) {
			if(role == nrServer) {
				result = (control == ctCpuEasy ||control == ctCpu || control == ctCpuUltra || control == ctCpuMega);
			}
			else {
				result = (control == ctNetworkCpuEasy ||control == ctNetworkCpu || control == ctNetworkCpuUltra || control == ctNetworkCpuMega);
			}
		}
	}

	return result;
}

bool Faction::getCpuControl() const {
	return 	control == ctCpuEasy 		||control == ctCpu 			|| control == ctCpuUltra 		|| control == ctCpuMega ||
			control == ctNetworkCpuEasy ||control == ctNetworkCpu 	|| control == ctNetworkCpuUltra || control == ctNetworkCpuMega;
}

// ==================== upgrade manager ====================

void Faction::startUpgrade(const UpgradeType *ut){
	upgradeManager.startUpgrade(ut, index);
}

void Faction::cancelUpgrade(const UpgradeType *ut){
	upgradeManager.cancelUpgrade(ut);
}

void Faction::finishUpgrade(const UpgradeType *ut){
	upgradeManager.finishUpgrade(ut);
	for(int i=0; i<getUnitCount(); ++i){
		getUnit(i)->applyUpgrade(ut);
	}
}

// ==================== reqs ====================

//checks if all required units and upgrades are present and maxUnitCount is within limit
bool Faction::reqsOk(const RequirableType *rt) const{
	assert(rt != NULL);
	//required units
    for(int i=0; i<rt->getUnitReqCount(); ++i){
        bool found=false;
        for(int j=0; j<getUnitCount(); ++j){
			Unit *unit= getUnit(j);
            const UnitType *ut= unit->getType();
            if(rt->getUnitReq(i)==ut && unit->isOperative()){
                found= true;
                break;
            }
        }
		if(!found){
            return false;
		}
    }

	//required upgrades
    for(int i=0; i<rt->getUpgradeReqCount(); ++i) {
		if(upgradeManager.isUpgraded(rt->getUpgradeReq(i)) == false) {
			return false;
		}
    }
    
    if(dynamic_cast<const UnitType *>(rt) != NULL ) {
    	const UnitType *producedUnitType=(UnitType *) rt;		
   		if(producedUnitType != NULL && producedUnitType->getMaxUnitCount() > 0) {
			if(producedUnitType->getMaxUnitCount() <= getCountForMaxUnitCount(producedUnitType)) {
		        return false;
			}
   		}
    }
    
	return true;
}

int Faction::getCountForMaxUnitCount(const UnitType *unitType) const{
	int count=0;
	//calculate current unit count
   	for(int j=0; j<getUnitCount(); ++j){
		Unit *unit= getUnit(j);
        const UnitType *currentUt= unit->getType();
        if(unitType==currentUt && unit->isOperative()){
            count++;
        }
        //check if there is any command active which already produces this unit
        count=count+unit->getCountOfProducedUnits(unitType);
    }
	return count;
}


bool Faction::reqsOk(const CommandType *ct) const{
	assert(ct != NULL);
	if(ct->getProduced()!=NULL && !reqsOk(ct->getProduced())){
		return false;
	}

	if(ct->getClass()==ccUpgrade){
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
		if(upgradeManager.isUpgradingOrUpgraded(uct->getProducedUpgrade())){
			return false;
		}
	}

	return reqsOk(static_cast<const RequirableType*>(ct));
}

// ================== cost application ==================

//apply costs except static production (start building/production)
bool Faction::applyCosts(const ProducibleType *p){

	if(!checkCosts(p)){
		return false;
	}

	assert(p != NULL);
	//for each unit cost spend it
    //pass 2, decrease resources, except negative static costs (ie: farms)
	for(int i=0; i<p->getCostCount(); ++i)
	{
        const ResourceType *rt= p->getCost(i)->getType();
		int cost= p->getCost(i)->getAmount();
		if((cost > 0 || (rt->getClass() != rcStatic)) && rt->getClass() != rcConsumable)
		{
            incResourceAmount(rt, -(cost));
		}

    }
    return true;
}

//apply discount (when a morph ends)
void Faction::applyDiscount(const ProducibleType *p, int discount)
{
	assert(p != NULL);
	//increase resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
        int cost= p->getCost(i)->getAmount();
		if((cost > 0 || (rt->getClass() != rcStatic)) && rt->getClass() != rcConsumable)
		{
            incResourceAmount(rt, cost*discount/100);
		}
    }
}

//apply static production (for starting units)
void Faction::applyStaticCosts(const ProducibleType *p)
{
	assert(p != NULL);
	//decrease static resources
    for(int i=0; i<p->getCostCount(); ++i)
    {
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
        if(rt->getClass() == rcStatic)
        {
            int cost= p->getCost(i)->getAmount();
			if(cost > 0)
			{
				incResourceAmount(rt, -cost);
			}
        }
    }
}

//apply static production (when a mana source is done)
void Faction::applyStaticProduction(const ProducibleType *p)
{
	assert(p != NULL);
	//decrease static resources
    for(int i=0; i<p->getCostCount(); ++i)
    {
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
        if(rt->getClass() == rcStatic)
        {
            int cost= p->getCost(i)->getAmount();
			if(cost < 0)
			{
				incResourceAmount(rt, -cost);
			}
        }
    }
}

//deapply all costs except static production (usually when a building is cancelled)
void Faction::deApplyCosts(const ProducibleType *p)
{
	assert(p != NULL);
	//increase resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
        int cost= p->getCost(i)->getAmount();
		if((cost > 0 || (rt->getClass() != rcStatic)) && rt->getClass() != rcConsumable)
		{
            incResourceAmount(rt, cost);
		}

    }
}

//deapply static costs (usually when a unit dies)
void Faction::deApplyStaticCosts(const ProducibleType *p)
{
	assert(p != NULL);
    //decrease resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
		if(rt->getClass() == rcStatic)
		{
		    if(rt->getRecoup_cost() == true)
		    {
                int cost= p->getCost(i)->getAmount();
                incResourceAmount(rt, cost);
		    }
        }
    }
}

//deapply static costs, but not negative costs, for when building gets killed
void Faction::deApplyStaticConsumption(const ProducibleType *p)
{
	assert(p != NULL);
    //decrease resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
		if(rt->getClass() == rcStatic)
		{
            int cost= p->getCost(i)->getAmount();
			if(cost>0)
			{
				incResourceAmount(rt, cost);
			}
        }
    }
}

//apply resource on interval (cosumable resouces)
void Faction::applyCostsOnInterval(){

	//increment consumables
	for(int j=0; j<getUnitCount(); ++j){
		Unit *unit= getUnit(j);
		if(unit->isOperative()){
			for(int k=0; k<unit->getType()->getCostCount(); ++k){
				const Resource *resource= unit->getType()->getCost(k);
				if(resource->getType()->getClass() == rcConsumable && resource->getAmount() < 0) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getResource(resource->getType())->getAmount() = %d\n",__FILE__,__FUNCTION__,__LINE__,getResource(resource->getType())->getAmount());

					incResourceAmount(resource->getType(), -resource->getAmount());

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getResource(resource->getType())->getAmount() = %d\n",__FILE__,__FUNCTION__,__LINE__,getResource(resource->getType())->getAmount());
				}
			}
		}
	}
	
	//decrement consumables
	for(int j=0; j<getUnitCount(); ++j){
		Unit *unit= getUnit(j);
		assert(unit != NULL);
		if(unit->isOperative()) {
			for(int k = 0; k < unit->getType()->getCostCount(); ++k) {
				const Resource *resource= unit->getType()->getCost(k);
				if(resource->getType()->getClass() == rcConsumable && resource->getAmount() > 0){
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getResource(resource->getType())->getAmount() = %d\n",__FILE__,__FUNCTION__,__LINE__,getResource(resource->getType())->getAmount());

					incResourceAmount(resource->getType(), -resource->getAmount());

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getResource(resource->getType())->getAmount() = %d\n",__FILE__,__FUNCTION__,__LINE__,getResource(resource->getType())->getAmount());
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] consume setting for faction index = %d, consume = %d, getResource(resource->getType())->getAmount() = %d, Unit = [%s] - [%d]\n",__FILE__,__FUNCTION__,__LINE__,this->index,scriptManager->getPlayerModifiers(this->index)->getConsumeEnabled(),getResource(resource->getType())->getAmount(),unit->getFullName().c_str(),unit->getId());

					//decrease unit hp
					if(scriptManager->getPlayerModifiers(this->index)->getConsumeEnabled() == true &&
						getResource(resource->getType())->getAmount() < 0) {
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						resetResourceAmount(resource->getType());
						bool decHpResult=unit->decHp(unit->getType()->getMaxHp()/3);

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] decHpResult = %d, unit->getType()->getMaxHp() = %d, hp = %d\n",__FILE__,__FUNCTION__,__LINE__,decHpResult,unit->getType()->getMaxHp(),unit->getHp());

						if(decHpResult) {
							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							world->getStats()->die(unit->getFactionIndex());
							scriptManager->onUnitDied(unit);
						}
						StaticSound *sound= unit->getType()->getFirstStOfClass(scDie)->getSound();
						if(sound!=NULL && thisFaction){
							SoundRenderer::getInstance().playFx(sound);
						}
					}
				}
			}
		}
	}
}

bool Faction::checkCosts(const ProducibleType *pt){
	assert(pt != NULL);
	//for each unit cost check if enough resources
	for(int i=0; i<pt->getCostCount(); ++i){
		const ResourceType *rt= pt->getCost(i)->getType();
		int cost= pt->getCost(i)->getAmount();
		if(cost > 0) {
			int available= getResource(rt)->getAmount();
			if(cost > available){
				return false;
			}
		}
    }

	return true;
}

// ================== diplomacy ==================

bool Faction::isAlly(const Faction *faction){
	assert(faction != NULL);
	return teamIndex==faction->getTeam();
}

// ================== misc ==================

void Faction::incResourceAmount(const ResourceType *rt, int amount)
{
	for(int i=0; i<resources.size(); ++i)
	{
		Resource *r= &resources[i];
		if(r->getType()==rt)
		{
			r->setAmount(r->getAmount()+amount);
			if(r->getType()->getClass() != rcStatic && r->getAmount()>getStoreAmount(rt))
			{
				r->setAmount(getStoreAmount(rt));
			}
			return;
		}
	}
	assert(false);
}

void Faction::setResourceBalance(const ResourceType *rt, int balance){
	for(int i=0; i<resources.size(); ++i){
		Resource *r= &resources[i];
		if(r->getType()==rt){
			r->setBalance(balance);
			return;
		}
	}
	assert(false);
}

Unit *Faction::findUnit(int id) const {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] id = %d\n",__FILE__,__FUNCTION__, __LINE__,id);

	UnitMap::const_iterator itFound = unitMap.find(id);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	if(itFound == unitMap.end()) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
		return NULL;
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] it->second = %p\n",__FILE__,__FUNCTION__, __LINE__,it->second);
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] it->second->id = %d\n",__FILE__,__FUNCTION__, __LINE__,it->second->getId());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] it->second = %s\n",__FILE__,__FUNCTION__, __LINE__,it->second->toString().c_str());
	return itFound->second;
}

void Faction::addUnit(Unit *unit){
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] id = %d\n",__FILE__,__FUNCTION__, __LINE__,unit->getId());

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] it->second = %p\n",__FILE__,__FUNCTION__, __LINE__,unit);
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] it->second->id = %d\n",__FILE__,__FUNCTION__, __LINE__,unit->getId());
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] it->second->str = %s\n",__FILE__,__FUNCTION__, __LINE__,unit->toString().c_str());

	units.push_back(unit);
	//unitMap.insert(make_pair(unit->getId(), unit));
	unitMap[unit->getId()] = unit;

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] id = %d\n",__FILE__,__FUNCTION__, __LINE__,unit->getId());
}

void Faction::removeUnit(Unit *unit){
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] id = %d\n",__FILE__,__FUNCTION__, __LINE__,unit->getId());

	assert(units.size()==unitMap.size());

	int unitId = unit->getId();
	for(int i=0; i<units.size(); ++i) {
		if(units[i]->getId() == unitId) {
			units.erase(units.begin()+i);
			unitMap.erase(unitId);
			assert(units.size() == unitMap.size());

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] id = %d\n",__FILE__,__FUNCTION__, __LINE__,unitId);

			return;
		}
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] id = %d\n",__FILE__,__FUNCTION__, __LINE__,unitId);

	throw runtime_error("Could not remove unit from faction!");
	assert(false);
}

void Faction::addStore(const UnitType *unitType){
	assert(unitType != NULL);
	for(int i=0; i<unitType->getStoredResourceCount(); ++i){
		const Resource *r= unitType->getStoredResource(i);
		for(int j=0; j<store.size(); ++j){
			Resource *storedResource= &store[j];
			if(storedResource->getType() == r->getType()){
				storedResource->setAmount(storedResource->getAmount() + r->getAmount());
			}
		}
	}
}

void Faction::removeStore(const UnitType *unitType){
	assert(unitType != NULL);
	for(int i=0; i<unitType->getStoredResourceCount(); ++i){
		const Resource *r= unitType->getStoredResource(i);
		for(int j=0; j<store.size(); ++j){
			Resource *storedResource= &store[j];
			if(storedResource->getType() == r->getType()){
				storedResource->setAmount(storedResource->getAmount() - r->getAmount());
			}
		}
	}
	limitResourcesToStore();
}

void Faction::limitResourcesToStore()
{
	for(int i=0; i<resources.size(); ++i)
	{
		Resource *r= &resources[i];
		Resource *s= &store[i];
		if(r->getType()->getClass() != rcStatic && r->getAmount()>s->getAmount())
		{
			r->setAmount(s->getAmount());
		}
	}
}

void Faction::resetResourceAmount(const ResourceType *rt){
	for(int i=0; i<resources.size(); ++i){
		if(resources[i].getType()==rt){
			resources[i].setAmount(0);
			return;
		}
	}
	assert(false);
}

std::vector<Vec2i> Faction::findCachedPath(const Vec2i &target, Unit *unit) {
	std::vector<Vec2i> result;
	if(successfulPathFinderTargetList.find(target) == successfulPathFinderTargetList.end()) {
		// Lets find the shortest and most successful path already taken by a
		// similar sized unit

		bool foundCachedPath = false;

		std::vector<FactionPathSuccessCache> &cacheList = successfulPathFinderTargetList[target];
		int unitSize = unit->getType()->getSize();
		for(int i = 0; i < cacheList.size(); ++i) {
			FactionPathSuccessCache &cache = cacheList[i];
			if(cache.unitSize <= unitSize) {
				vector<std::pair<vector<Vec2i>, int> > &pathQueue = cache.pathQueue;

				for(int j = 0; j < pathQueue.size(); ++j) {
					// Now start at the end of the path and see how many nodes
					// until we reach a cell near the unit's current position
					std::pair<vector<Vec2i>, int> &path = pathQueue[j];

					for(int k = path.first.size() - 1; k >= 0; --k) {
						if(world->getMap()->canMove(unit, unit->getPos(), path.first[k]) == true) {
							if(foundCachedPath == false) {
								for(int l = k; l < path.first.size(); ++l) {
									result.push_back(path.first[l]);
								}
							}
							else {
								if(result.size() > (path.first.size() - k)) {
									for(int l = k; l < path.first.size(); ++l) {
										result.push_back(path.first[l]);
									}
								}
							}
							foundCachedPath = true;
							break;
						}
					}
				}
			}
		}
	}
	return result;
}

void Faction::addCachedPath(const Vec2i &target, Unit *unit) {
	if(successfulPathFinderTargetList.find(target) == successfulPathFinderTargetList.end()) {
		FactionPathSuccessCache cache;
		cache.unitSize = unit->getType()->getSize();
		cache.pathQueue.push_back(make_pair<vector<Vec2i>, int>(unit->getCurrentTargetPathTaken().second,1));
		successfulPathFinderTargetList[target].push_back(cache);
	}
	else {
		bool finishedAdd = false;
		std::pair<Vec2i,std::vector<Vec2i> > currentTargetPathTaken = unit->getCurrentTargetPathTaken();
		std::vector<FactionPathSuccessCache> &cacheList = successfulPathFinderTargetList[target];
		int unitSize = unit->getType()->getSize();
		for(int i = 0; i < cacheList.size() && finishedAdd == false; ++i) {
			FactionPathSuccessCache &cache = cacheList[i];
			if(cache.unitSize <= unitSize) {
				vector<std::pair<vector<Vec2i>, int> > &pathQueue = cache.pathQueue;

				for(int j = 0; j < pathQueue.size() && finishedAdd == false; ++j) {
					// Now start at the end of the path and see how many nodes are the same
					std::pair<vector<Vec2i>, int> &path = pathQueue[j];
					int minPathSize = std::min(path.first.size(),currentTargetPathTaken.second.size());
					int intersectIndex = -1;

					for(int k = 0; k < minPathSize; ++k) {
						if(path.first[path.first.size() - k - 1] != currentTargetPathTaken.second[currentTargetPathTaken.second.size() - k - 1]) {
							intersectIndex = k;
							break;
						}
					}

					// New path is same or longer than old path so replace
					// old path with new
					if(intersectIndex + 1 == path.first.size()) {
						path.first = currentTargetPathTaken.second;
						path.second++;
						finishedAdd = true;
					}
					// Old path is same or longer than new path so
					// do nothing
					else if(intersectIndex + 1 == currentTargetPathTaken.second.size()) {
						path.second++;
						finishedAdd = true;
					}
				}

				// If new path is >= 10 cells add it
				if(finishedAdd == false && currentTargetPathTaken.second.size() >= 10) {
					pathQueue.push_back(make_pair<vector<Vec2i>, int>(currentTargetPathTaken.second,1));
				}
			}
		}
	}
}

std::string Faction::toString() const {
	std::string result = "";

    result = "FactionIndex = " + intToStr(this->index) + "\n";
    result += "teamIndex = " + intToStr(this->teamIndex) + "\n";
    result += "startLocationIndex = " + intToStr(this->startLocationIndex) + "\n";
    result += "thisFaction = " + intToStr(this->thisFaction) + "\n";
    result += "control = " + intToStr(this->control) + "\n";

    if(this->factionType != NULL) {
    	result += this->factionType->toString() + "\n";
    }

	result += this->upgradeManager.toString() + "\n";

	result += "ResourceCount = " + intToStr(resources.size()) + "\n";
	for(int idx = 0; idx < resources.size(); idx ++) {
		result += "index = " + intToStr(idx) + " " + resources[idx].getDescription() + "\n";
	}

	result += "StoreCount = " + intToStr(store.size()) + "\n";
	for(int idx = 0; idx < store.size(); idx ++) {
		result += "index = " + intToStr(idx) + " " + store[idx].getDescription()  + "\n";
	}

	result += "Allies = " + intToStr(allies.size()) + "\n";
	for(int idx = 0; idx < allies.size(); idx ++) {
		result += "index = " + intToStr(idx) + " name: " + allies[idx]->factionType->getName() + " factionindex = " + intToStr(allies[idx]->index)  + "\n";
	}

	result += "Units = " + intToStr(units.size()) + "\n";
	for(int idx = 0; idx < units.size(); idx ++) {
		result += units[idx]->toString() + "\n";
	}

	return result;
}

}}//end namespace
