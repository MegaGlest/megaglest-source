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
#include "game.h"
#include "config.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Faction
// =====================================================

Faction::Faction() {
	texture = NULL;
	//lastResourceTargettListPurge = 0;
	cachingDisabled=false;
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
	FactionType *factionType, ControlType control, TechTree *techTree, Game *game,
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
	cachingDisabled = (Config::getInstance().getBool("DisableCaching","false") == true);

	resources.resize(techTree->getResourceTypeCount());
	store.resize(techTree->getResourceTypeCount());
	for(int i=0; i<techTree->getResourceTypeCount(); ++i){
		const ResourceType *rt= techTree->getResourceType(i);
		int resourceAmount= giveResources? factionType->getStartingResourceAmount(rt): 0;
		resources[i].init(rt, resourceAmount);
		store[i].init(rt, 0);
	}

	texture= Renderer::getInstance().newTexture2D(rsGame);
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	texture->load(data_path + "data/core/faction_textures/faction"+intToStr(startLocationIndex)+".tga");

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
bool Faction::reqsOk(const RequirableType *rt) const {
	assert(rt != NULL);
	//required units
    for(int i = 0; i < rt->getUnitReqCount(); ++i) {
        bool found = false;
        for(int j = 0; j < getUnitCount(); ++j) {
			Unit *unit= getUnit(j);
            const UnitType *ut= unit->getType();
            if(rt->getUnitReq(i) == ut && unit->isOperative()) {
                found= true;
                break;
            }
        }
		if(found == false) {
			SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
            return false;
		}
    }

	//required upgrades
    for(int i = 0; i < rt->getUpgradeReqCount(); ++i) {
		if(upgradeManager.isUpgraded(rt->getUpgradeReq(i)) == false) {
			SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
			return false;
		}
    }

    if(dynamic_cast<const UnitType *>(rt) != NULL ) {
    	const UnitType *producedUnitType=(UnitType *) rt;
   		if(producedUnitType != NULL && producedUnitType->getMaxUnitCount() > 0) {
			if(producedUnitType->getMaxUnitCount() <= getCountForMaxUnitCount(producedUnitType)) {
				SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
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


bool Faction::reqsOk(const CommandType *ct) const {
	assert(ct != NULL);
	if(ct == NULL) {
	    throw runtime_error("In [Faction::reqsOk] ct == NULL");
	}

	if(ct->getProduced() != NULL && reqsOk(ct->getProduced()) == false) {
		SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] reqsOk FAILED\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}

	if(ct->getClass() == ccUpgrade) {
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
		if(upgradeManager.isUpgradingOrUpgraded(uct->getProducedUpgrade())) {
			SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] upgrade check FAILED\n",__FILE__,__FUNCTION__,__LINE__);
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
void Faction::applyCostsOnInterval(const ResourceType *rtApply) {

	// For each Resource type we store in the int a total consumed value, then
	// a vector of units that consume the resource type
	std::map<const ResourceType *, std::pair<int, std::vector<Unit *> > > resourceIntervalUsage;

	// count up consumables usage for the interval
	for(int j = 0; j < getUnitCount(); ++j) {
		Unit *unit = getUnit(j);
		if(unit->isOperative() == true) {
			for(int k = 0; k < unit->getType()->getCostCount(); ++k) {
				const Resource *resource = unit->getType()->getCost(k);
				if(resource->getType() == rtApply && resource->getType()->getClass() == rcConsumable && resource->getAmount() != 0) {
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] resource->getType() [%s] store = %d, unit cost = %d\n",__FILE__,__FUNCTION__,__LINE__,resource->getType()->getName().c_str(), getResource(resource->getType())->getAmount(),resource->getAmount());

					if(resourceIntervalUsage.find(resource->getType()) == resourceIntervalUsage.end()) {
						resourceIntervalUsage[resource->getType()] = make_pair<int, std::vector<Unit *> >(0,std::vector<Unit *>());
					}
					// Negative cost means accumulate the resource type
					resourceIntervalUsage[resource->getType()].first += -resource->getAmount();

					// If the cost > 0 then the unit is a consumer
					if(resource->getAmount() > 0) {
						resourceIntervalUsage[resource->getType()].second.push_back(unit);
					}

					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] resourceIntervalUsage[resource->getType()].first = %d, consumerCount = %d\n",__FILE__,__FUNCTION__,__LINE__,resourceIntervalUsage[resource->getType()].first,resourceIntervalUsage[resource->getType()].second.size());
				}
			}
		}
	}

	// Apply consumable resource usage
	if(resourceIntervalUsage.size() > 0) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] resourceIntervalUsage.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,resourceIntervalUsage.size());

		for(std::map<const ResourceType *, std::pair<int, std::vector<Unit *> > >::iterator iter = resourceIntervalUsage.begin();
																							iter != resourceIntervalUsage.end();
																							++iter) {
			// Apply resource type usage to faction resource store
			const ResourceType *rt = iter->first;
			int resourceTypeUsage = iter->second.first;
			incResourceAmount(rt, resourceTypeUsage);

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] rt [%s] resourceTypeUsage = %d store = %d\n",__FILE__,__FUNCTION__,__LINE__,rt->getName().c_str(), resourceTypeUsage,getResource(rt)->getAmount());

			// Check if we have any unit consumers
			if(getResource(rt)->getAmount() < 0) {
				resetResourceAmount(rt);

				// Apply consequences to consumer units of this resource type
				std::vector<Unit *> &resourceConsumers = iter->second.second;

				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] resourceConsumers.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,resourceConsumers.size());

				for(int i = 0; i < resourceConsumers.size(); ++i) {
					Unit *unit = resourceConsumers[i];

					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] getResource(resource->getType())->getAmount() = %d\n",__FILE__,__FUNCTION__,__LINE__,getResource(rt)->getAmount());
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] consume setting for faction index = %d, consume = %d, getResource(resource->getType())->getAmount() = %d, Unit = [%s] - [%d]\n",__FILE__,__FUNCTION__,__LINE__,this->index,scriptManager->getPlayerModifiers(this->index)->getConsumeEnabled(),getResource(rt)->getAmount(),unit->getFullName().c_str(),unit->getId());

					//decrease unit hp
					if(scriptManager->getPlayerModifiers(this->index)->getConsumeEnabled() == true) {
						//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

						bool decHpResult = unit->decHp(unit->getType()->getMaxHp() / 3);

						//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] decHpResult = %d, unit->getType()->getMaxHp() = %d, hp = %d\n",__FILE__,__FUNCTION__,__LINE__,decHpResult,unit->getType()->getMaxHp(),unit->getHp());

						if(decHpResult) {
							//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

							world->getStats()->die(unit->getFactionIndex());
							scriptManager->onUnitDied(unit);
						}
						StaticSound *sound= unit->getType()->getFirstStOfClass(scDie)->getSound();
						if(sound != NULL && thisFaction) {
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

bool Faction::isAlly(const Faction *faction) {
	assert(faction != NULL);
	return teamIndex==faction->getTeam();
}

// ================== misc ==================

void Faction::incResourceAmount(const ResourceType *rt, int amount) {
	for(int i=0; i<resources.size(); ++i) {
		Resource *r= &resources[i];
		if(r->getType()==rt) {
			r->setAmount(r->getAmount()+amount);
			if(r->getType()->getClass() != rcStatic && r->getAmount()>getStoreAmount(rt)) {
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

bool Faction::isResourceTargetInCache(const Vec2i &pos, bool incrementUseCounter) {
	bool result = false;

	if(cachingDisabled == false) {
		if(cacheResourceTargetList.size() > 0) {
			std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.find(pos);

			result = (iter != cacheResourceTargetList.end());
			if(result == true && incrementUseCounter == true) {
				iter->second++;
			}
		}
	}

	return result;
}

void Faction::addResourceTargetToCache(const Vec2i &pos,bool incrementUseCounter) {
	if(cachingDisabled == false) {

		bool duplicateEntry = isResourceTargetInCache(pos,incrementUseCounter);
		//bool duplicateEntry = false;

		if(duplicateEntry == false) {
			cacheResourceTargetList[pos] = 1;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
				char szBuf[4096]="";
				sprintf(szBuf,"[addResourceTargetToCache] pos [%s]cacheResourceTargetList.size() [%ld]",
								pos.getString().c_str(),cacheResourceTargetList.size());

				//unit->logSynchData(szBuf);
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"[%s::%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s\n",szBuf);
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
			}
		}
	}
}

void Faction::removeResourceTargetFromCache(const Vec2i &pos) {
	if(cachingDisabled == false) {
		if(cacheResourceTargetList.size() > 0) {
			std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.find(pos);

			if(iter != cacheResourceTargetList.end()) {
				cacheResourceTargetList.erase(pos);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
					char szBuf[4096]="";
					sprintf(szBuf,"[removeResourceTargetFromCache] pos [%s]cacheResourceTargetList.size() [%ld]",
									pos.getString().c_str(),cacheResourceTargetList.size());

					//unit->logSynchData(szBuf);
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"[%s::%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s\n",szBuf);
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
				}
			}
		}
	}
}

void Faction::addCloseResourceTargetToCache(const Vec2i &pos) {
	if(cachingDisabled == false) {
		if(cachedCloseResourceTargetLookupList.find(pos) == cachedCloseResourceTargetLookupList.end()) {
			const Map *map = world->getMap();
			const int harvestDistance = 5;

			for(int j = -harvestDistance; j <= harvestDistance; ++j) {
				for(int k = -harvestDistance; k <= harvestDistance; ++k) {
					Vec2i newPos = pos + Vec2i(j,k);
					if(isResourceTargetInCache(newPos) == false) {
						if(map->isInside(newPos.x, newPos.y)) {
							Resource *r = map->getSurfaceCell(map->toSurfCoords(newPos))->getResource();
							if(r != NULL) {
								addResourceTargetToCache(newPos);
								//cacheResourceTargetList[newPos] = 1;
							}
						}
					}
				}
			}

			cachedCloseResourceTargetLookupList[pos] = true;
		}
	}
}

Vec2i Faction::getClosestResourceTypeTargetFromCache(Unit *unit, const ResourceType *type) {
	Vec2i result(-1);

	if(cachingDisabled == false) {
		if(cacheResourceTargetList.size() > 0) {
			std::vector<Vec2i> deleteList;

			const int harvestDistance = 5;
			const Map *map = world->getMap();
			Vec2i pos = unit->getPos();

			bool foundCloseResource = false;
			// First look immediately around the unit's position
			for(int j = -harvestDistance; j <= harvestDistance && foundCloseResource == false; ++j) {
				for(int k = -harvestDistance; k <= harvestDistance && foundCloseResource == false; ++k) {
					Vec2i newPos = pos + Vec2i(j,k);
					if(map->isInside(newPos) == true && isResourceTargetInCache(newPos) == false) {
						const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(newPos));
						if( sc != NULL && sc->getResource() != NULL) {
							const Resource *resource = sc->getResource();
							if(resource->getType() != NULL && resource->getType() == type) {
								if(result.x < 0 || unit->getPos().dist(newPos) < unit->getPos().dist(result)) {
									if(unit->isBadHarvestPos(newPos) == false) {
										result = newPos;
										foundCloseResource = true;
										break;
									}
								}
							}
						}
						else {
							deleteList.push_back(newPos);
						}
					}
				}
			}

			if(foundCloseResource == false) {
				// Now check the whole cache
				for(std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.begin();
											  iter != cacheResourceTargetList.end() && foundCloseResource == false;
											  ++iter) {
					const Vec2i &cache = iter->first;
					if(map->isInside(cache) == true) {
						const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(cache));
						if( sc != NULL && sc->getResource() != NULL) {
							const Resource *resource = sc->getResource();
							if(resource->getType() != NULL && resource->getType() == type) {
								if(result.x < 0 || unit->getPos().dist(cache) < unit->getPos().dist(result)) {
									if(unit->isBadHarvestPos(cache) == false) {
										result = cache;
										// Close enough to our position, no more looking
										if(unit->getPos().dist(result) <= (harvestDistance * 2)) {
											foundCloseResource = true;
											break;
										}
									}
								}
							}
						}
						else {
							deleteList.push_back(cache);
						}
					}
					else {
						deleteList.push_back(cache);
					}
				}
			}

			if(deleteList.size() > 0) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
					char szBuf[4096]="";
								sprintf(szBuf,"[cleaning old resource targets] deleteList.size() [%ld] cacheResourceTargetList.size() [%ld] result [%s]",
										deleteList.size(),cacheResourceTargetList.size(),result.getString().c_str());

					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}

				cleanupResourceTypeTargetCache(&deleteList);
			}
		}
	}

	return result;
}

// CANNOT MODIFY the cache here since the AI calls this method and the AI is only controlled
// by the server for network games and it would cause out of synch since clients do not call
// this method so DO NOT modify the cache here!
Vec2i Faction::getClosestResourceTypeTargetFromCache(const Vec2i &pos, const ResourceType *type) {
	Vec2i result(-1);
	if(cachingDisabled == false) {
		if(cacheResourceTargetList.size() > 0) {
			//std::vector<Vec2i> deleteList;

			const int harvestDistance = 5;
			const Map *map = world->getMap();

			bool foundCloseResource = false;
			// First look immediately around the given position
			for(int j = -harvestDistance; j <= harvestDistance && foundCloseResource == false; ++j) {
				for(int k = -harvestDistance; k <= harvestDistance && foundCloseResource == false; ++k) {
					Vec2i newPos = pos + Vec2i(j,k);
					if(map->isInside(newPos) == true && isResourceTargetInCache(newPos) == false) {
						const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(newPos));
						if( sc != NULL && sc->getResource() != NULL) {
							const Resource *resource = sc->getResource();
							if(resource->getType() != NULL && resource->getType() == type) {
								if(result.x < 0 || pos.dist(newPos) < pos.dist(result)) {
									result = newPos;
									foundCloseResource = true;
									break;
								}
							}
						}
						//else {
						//	deleteList.push_back(newPos);
						//}
					}
				}
			}

			if(foundCloseResource == false) {
				// Now check the whole cache
				for(std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.begin();
											  iter != cacheResourceTargetList.end() && foundCloseResource == false;
											  ++iter) {
					const Vec2i &cache = iter->first;
					if(map->isInside(cache) == true) {
						const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(cache));
						if( sc != NULL && sc->getResource() != NULL) {
							const Resource *resource = sc->getResource();
							if(resource->getType() != NULL && resource->getType() == type) {
								if(result.x < 0 || pos.dist(cache) < pos.dist(result)) {
									result = cache;
									// Close enough to our position, no more looking
									if(pos.dist(result) <= (harvestDistance * 2)) {
										foundCloseResource = true;
										break;
									}
								}
							}
						}
						//else {
						//	deleteList.push_back(cache);
						//}
					}
					//else {
					//	deleteList.push_back(cache);
					//}
				}
			}

		  	//char szBuf[4096]="";
		  	//sprintf(szBuf,"[%s::%s Line: %d] [looking for resource targets] result [%s] deleteList.size() [%ld] cacheResourceTargetList.size() [%ld] foundCloseResource [%d]",
			//	    			__FILE__,__FUNCTION__,__LINE__,result.getString().c_str(),deleteList.size(),cacheResourceTargetList.size(),foundCloseResource);

		    //unit->logSynchData(szBuf);
			//SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
			//SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s",szBuf);
			//SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());

			//if(deleteList.size() > 0) {
			//	cleanupResourceTypeTargetCache(&deleteList);
			//}
		}
	}

	return result;
}

void Faction::cleanupResourceTypeTargetCache(std::vector<Vec2i> *deleteListPtr) {
	if(cachingDisabled == false) {
		if(cacheResourceTargetList.size() > 0) {
			const int cleanupInterval = (GameConstants::updateFps * 5);
			bool needToCleanup = (getFrameCount() % cleanupInterval == 0);

			if(deleteListPtr != NULL || needToCleanup == true) {
				std::vector<Vec2i> deleteList;

				if(deleteListPtr != NULL) {
					deleteList = *deleteListPtr;
				}
				else {
					for(std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.begin();
												  iter != cacheResourceTargetList.end(); ++iter) {
						const Vec2i &cache = iter->first;

						if(world->getMap()->getSurfaceCell(world->getMap()->toSurfCoords(cache)) != NULL) {
							Resource *resource = world->getMap()->getSurfaceCell(world->getMap()->toSurfCoords(cache))->getResource();
							if(resource == NULL) {
								deleteList.push_back(cache);
							}
						}
						else {
							deleteList.push_back(cache);
						}
					}
				}

				if(deleteList.size() > 0) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
						char szBuf[4096]="";
									sprintf(szBuf,"[cleaning old resource targets] deleteList.size() [%ld] cacheResourceTargetList.size() [%ld], needToCleanup [%d]",
											deleteList.size(),cacheResourceTargetList.size(),needToCleanup);
						//unit->logSynchData(szBuf);
						SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
						SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"[%s::%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
						SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s\n",szBuf);
						SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
					}

					for(int i = 0; i < deleteList.size(); ++i) {
						Vec2i &cache = deleteList[i];
						cacheResourceTargetList.erase(cache);
					}
				}
			}
		}
	}
}

std::vector<Vec2i> Faction::findCachedPath(const Vec2i &target, Unit *unit) {
	std::vector<Vec2i> result;
	if(cachingDisabled == false) {
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
	}

	return result;
}

void Faction::addCachedPath(const Vec2i &target, Unit *unit) {
	if(cachingDisabled == false) {
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
}

void Faction::deletePixels() {
	if(factionType != NULL) {
		factionType->deletePixels();
	}
}

Unit * Faction::findClosestUnitWithSkillClass(	const Vec2i &pos,const CommandClass &cmdClass,
												const std::vector<SkillClass> &skillClassList,
												const UnitType *unitType) {
	Unit *result = NULL;

/*
	std::map<CommandClass,std::map<int,int> >::iterator iterFind = cacheUnitCommandClassList.find(cmdClass);
	if(iterFind != cacheUnitCommandClassList.end()) {
		for(std::map<int,int>::iterator iter = iterFind->second.begin();
				iter != iterFind->second.end(); ++iter) {
			Unit *curUnit = findUnit(iter->second);
			if(curUnit != NULL) {

				const CommandType *cmdType = curUnit->getType()->getFirstCtOfClass(cmdClass);
				bool isUnitPossibleCandidate = (cmdType != NULL);
				if(skillClassList.size() > 0) {
					isUnitPossibleCandidate = false;

					for(int j = 0; j < skillClassList.size(); ++j) {
						SkillClass skValue = skillClassList[j];
						if(curUnit->getCurrSkill()->getClass() == skValue) {
							isUnitPossibleCandidate = true;
							break;
						}
					}
				}

				if(isUnitPossibleCandidate == true) {
					if(result == NULL || curUnit->getPos().dist(pos) < result->getPos().dist(pos)) {
						result = curUnit;
					}
				}
			}
		}
	}
*/

	if(result == NULL) {
		for(int i = 0; i < getUnitCount(); ++i) {
			Unit *curUnit = getUnit(i);

			bool isUnitPossibleCandidate = false;

			const CommandType *cmdType = curUnit->getType()->getFirstCtOfClass(cmdClass);
			if(cmdType != NULL) {
				const RepairCommandType *rct = dynamic_cast<const RepairCommandType *>(cmdType);
				if(rct != NULL && rct->isRepairableUnitType(unitType)) {
					isUnitPossibleCandidate = true;
				}
			}
			else {
				isUnitPossibleCandidate = false;
			}

			if(isUnitPossibleCandidate == true && skillClassList.size() > 0) {
				isUnitPossibleCandidate = false;

				for(int j = 0; j < skillClassList.size(); ++j) {
					SkillClass skValue = skillClassList[j];
					if(curUnit->getCurrSkill()->getClass() == skValue) {
						isUnitPossibleCandidate = true;
						break;
					}
				}
			}


			if(isUnitPossibleCandidate == true) {
				//cacheUnitCommandClassList[cmdClass][curUnit->getId()] = curUnit->getId();

				if(result == NULL || curUnit->getPos().dist(pos) < result->getPos().dist(pos)) {
					result = curUnit;
				}
			}
		}
	}
	return result;
}

int Faction::getFrameCount() {
	int frameCount = 0;
	const Game *game = Renderer::getInstance().getGame();
	if(game != NULL && game->getWorld() != NULL) {
		frameCount = game->getWorld()->getFrameCount();
	}

	return frameCount;
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
