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

void Faction::init(
	const FactionType *factionType, ControlType control, TechTree *techTree, Game *game,
	int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction, bool giveResources)
{
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
}

void Faction::end(){
	deleteValues(units.begin(), units.end());
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

bool Faction::getCpuControl() const{
	return control==ctCpuEasy ||control==ctCpu || control==ctCpuUltra|| control==ctCpuMega;
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

//checks if all required units and upgrades are present
bool Faction::reqsOk(const RequirableType *rt) const{

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
    for(int i=0; i<rt->getUpgradeReqCount(); ++i){
		if(!upgradeManager.isUpgraded(rt->getUpgradeReq(i))){
			return false;
		}
    }
	return true;

}

bool Faction::reqsOk(const CommandType *ct) const{

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
	//increase resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
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
	//decrease static resources
    for(int i=0; i<p->getCostCount(); ++i)
    {
		const ResourceType *rt= p->getCost(i)->getType();
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
	//decrease static resources
    for(int i=0; i<p->getCostCount(); ++i)
    {
		const ResourceType *rt= p->getCost(i)->getType();
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
	//increase resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
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
    //decrease resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
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
    //decrease resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
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
				if(resource->getType()->getClass()==rcConsumable && resource->getAmount()<0){
					incResourceAmount(resource->getType(), -resource->getAmount());
				}
			}
		}
	}
	
	//decrement consumables
	if((getCpuControl() == true) ||
		(getCpuControl() == true && scriptManager->getPlayerModifiers(this->thisFaction)->getAiEnabled() == false))
	{
		for(int j=0; j<getUnitCount(); ++j){
			Unit *unit= getUnit(j);
			assert(unit != NULL);
			if(unit->isOperative()){
				for(int k=0; k<unit->getType()->getCostCount(); ++k){
					const Resource *resource= unit->getType()->getCost(k);
					if(resource->getType()->getClass()==rcConsumable && resource->getAmount()>0){
						incResourceAmount(resource->getType(), -resource->getAmount());
	
						//decrease unit hp
						if(getResource(resource->getType())->getAmount()<0){
							resetResourceAmount(resource->getType());
							bool decHpResult=unit->decHp(unit->getType()->getMaxHp()/3);
							if(decHpResult){
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
}

bool Faction::checkCosts(const ProducibleType *pt){

	//for each unit cost check if enough resources
	for(int i=0; i<pt->getCostCount(); ++i){
		const ResourceType *rt= pt->getCost(i)->getType();
		int cost= pt->getCost(i)->getAmount();
		if(cost>0){
			int available= getResource(rt)->getAmount();
			if(cost>available){
				return false;
			}
		}
    }

	return true;
}

// ================== diplomacy ==================

bool Faction::isAlly(const Faction *faction){
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

Unit *Faction::findUnit(int id){
	UnitMap::iterator it= unitMap.find(id);

	if(it==unitMap.end()){
		return NULL;
	}
	return it->second;
}

void Faction::addUnit(Unit *unit){
	units.push_back(unit);
	unitMap.insert(make_pair(unit->getId(), unit));
}

void Faction::removeUnit(Unit *unit){
	for(int i=0; i<units.size(); ++i){
		if(units[i]==unit){
			units.erase(units.begin()+i);
			unitMap.erase(unit->getId());
			assert(units.size()==unitMap.size());
			return;
		}
	}
	assert(false);
}

void Faction::addStore(const UnitType *unitType){
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

}}//end namespace
