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

#include "ai_interface.h"

#include "ai.h"
#include "command_type.h"
#include "faction.h"
#include "unit.h"
#include "unit_type.h"
#include "object.h"
#include "game.h"
#include "config.h"
#include "network_manager.h"
#include "platform_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

// =====================================================
// 	class AiInterface
// =====================================================

namespace Glest{ namespace Game{

AiInterface::AiInterface(Game &game, int factionIndex, int teamIndex, int useStartLocation) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->world= game.getWorld();
	this->commander= game.getCommander();
	this->console= game.getConsole();
	this->gameSettings = game.getGameSettings();

	this->factionIndex= factionIndex;
	this->teamIndex= teamIndex;
	timer= 0;

	//init ai
	ai.init(this,useStartLocation);

	//config
	logLevel= Config::getInstance().getInt("AiLog");
	redir= Config::getInstance().getBool("AiRedir");

	//clear log file
	if(logLevel>0){
#ifdef WIN32
		FILE *f= _wfopen(Shared::Platform::utf8_decode(getLogFilename()).c_str(), L"wt");
#else
		FILE *f= fopen(getLogFilename().c_str(), "wt");
#endif
		if(f==NULL){
			throw runtime_error("Can't open file: "+getLogFilename());
		}
		fprintf(f, "%s", "MegaGlest AI log file\n\n");
		fclose(f);
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

AiInterface::~AiInterface() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI factionIndex = %d, teamIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,this->factionIndex,this->teamIndex);
    cacheUnitHarvestResourceLookup.clear();
}
// ==================== main ====================

void AiInterface::update() {
	timer++;
	ai.update();
}

// ==================== misc ====================

void AiInterface::printLog(int logLevel, const string &s){
    if(this->logLevel>=logLevel){
		string logString= "(" + intToStr(factionIndex) + ") " + s;

		//print log to file
#ifdef WIN32
		FILE *f= _wfopen(utf8_decode(getLogFilename()).c_str(), L"at");
#else
		FILE *f= fopen(getLogFilename().c_str(), "at");
#endif
		if(f==NULL){
			throw runtime_error("Can't open file: "+getLogFilename());
		}
		fprintf(f, "%s\n", logString.c_str());
		fclose(f);

		//redirect to console
		if(redir) {
			console->addLine(logString);
		}
    }
}

// ==================== interaction ====================

Faction *AiInterface::getMyFaction() {
	return world->getFaction(factionIndex);
}

bool AiInterface::executeCommandOverNetwork() {
	bool enableServerControlledAI 	= gameSettings->getEnableServerControlledAI();
	bool isNetworkGame 				= gameSettings->isNetworkGame();
	NetworkRole role 				= NetworkManager::getInstance().getNetworkRole();
	Faction *faction 				= world->getFaction(factionIndex);
	return faction->getCpuControl(enableServerControlledAI,isNetworkGame,role);
}

CommandResult AiInterface::giveCommand(int unitIndex, CommandClass commandClass, const Vec2i &pos){
	assert(this->gameSettings != NULL);

	if(executeCommandOverNetwork() == true) {
		const Unit *unit = getMyUnit(unitIndex);
		CommandResult result = commander->tryGiveCommand(unit, unit->getType()->getFirstCtOfClass(commandClass), pos, unit->getType(),CardinalDir::NORTH);

		return result;
	}
	else {
		Command *c= new Command (world->getFaction(factionIndex)->getUnit(unitIndex)->getType()->getFirstCtOfClass(commandClass), pos);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(c);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
}

CommandResult AiInterface::giveCommand(const Unit *unit, const CommandType *commandType, const Vec2i &pos) {
	assert(this->gameSettings != NULL);

	if(unit == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unit in AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,factionIndex);
		throw runtime_error(szBuf);
	}
    const UnitType* unitType= unit->getType();
	if(unitType == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unittype with unit id: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unit->getId(),factionIndex);
		throw runtime_error(szBuf);
	}
	if(commandType == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] commandType == NULL, unit id: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unit->getId(),factionIndex);
		throw runtime_error(szBuf);
	}
    const CommandType* ct= unit->getType()->findCommandTypeById(commandType->getId());
	if(ct == NULL) {
	    char szBuf[4096]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nCan not find AI command type for:\nunit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,
            unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),
            unit->getFaction()->getIndex());

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);

	    std::string worldLog = world->DumpWorldToLog();
	    std::string sError = "worldLog = " + worldLog + " " + string(szBuf);
		throw runtime_error(sError);
	}

	if(executeCommandOverNetwork() == true) {
		CommandResult result = commander->tryGiveCommand(unit, commandType, pos, unit->getType(),CardinalDir::NORTH);
		return result;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Faction *faction = world->getFaction(unit->getFactionIndex());
		Unit *unitToCommand = faction->findUnit(unit->getId());
		CommandResult result = unitToCommand->giveCommand(new Command(commandType, pos));

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		return result;
	}
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos){
	assert(this->gameSettings != NULL);

	const Unit *unit = getMyUnit(unitIndex);
	if(unit == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unit with index: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unitIndex,factionIndex);
		throw runtime_error(szBuf);
	}
    const UnitType* unitType= unit->getType();
	if(unitType == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unittype with unit index: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unitIndex,factionIndex);
		throw runtime_error(szBuf);
	}
    const CommandType* ct= unit->getType()->findCommandTypeById(commandType->getId());
	if(ct == NULL) {
	    char szBuf[4096]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nCan not find AI command type for:\nunit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,
            unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),
            unit->getFaction()->getIndex());

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);

	    std::string worldLog = world->DumpWorldToLog();
	    std::string sError = "worldLog = " + worldLog + " " + string(szBuf);
		throw runtime_error(sError);
	}

	if(executeCommandOverNetwork() == true) {
		const Unit *unit = getMyUnit(unitIndex);
		CommandResult result = commander->tryGiveCommand(unit, commandType, pos, unit->getType(),CardinalDir::NORTH);
		return result;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, pos));

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		return result;
	}
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos, const UnitType *ut) {
	assert(this->gameSettings != NULL);

	const Unit *unit = getMyUnit(unitIndex);
	if(unit == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unit with index: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unitIndex,factionIndex);
		throw runtime_error(szBuf);
	}
    const UnitType* unitType= unit->getType();
	if(unitType == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unittype with unit index: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unitIndex,factionIndex);
		throw runtime_error(szBuf);
	}
    const CommandType* ct= unit->getType()->findCommandTypeById(commandType->getId());
	if(ct == NULL) {
	    char szBuf[4096]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nCan not find AI command type for:\nunit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,
            unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),
            unit->getFaction()->getIndex());

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);

	    std::string worldLog = world->DumpWorldToLog();
	    std::string sError = "worldLog = " + worldLog + " " + string(szBuf);
		throw runtime_error(sError);
	}

	if(executeCommandOverNetwork() == true) {
		const Unit *unit = getMyUnit(unitIndex);
		CommandResult result = commander->tryGiveCommand(unit, commandType, pos, ut,CardinalDir::NORTH);
		return result;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, pos, ut, CardinalDir::NORTH));

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, Unit *u){
	assert(this->gameSettings != NULL);
	assert(this->commander != NULL);

	const Unit *unit = getMyUnit(unitIndex);
	if(unit == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unit with index: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unitIndex,factionIndex);
		throw runtime_error(szBuf);
	}
    const UnitType* unitType= unit->getType();
	if(unitType == NULL) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s Line: %d] Can not find AI unittype with unit index: %d, AI factionIndex = %d. Game out of synch.",__FILE__,__FUNCTION__,__LINE__,unitIndex,factionIndex);
		throw runtime_error(szBuf);
	}
    const CommandType* ct= (commandType != NULL ? unit->getType()->findCommandTypeById(commandType->getId()) : NULL);
	if(ct == NULL) {
	    char szBuf[4096]="";
	    sprintf(szBuf,"In [%s::%s Line: %d]\nCan not find AI command type for:\nunit = %d\n[%s]\n[%s]\nactual local factionIndex = %d.\nGame out of synch.",
            __FILE__,__FUNCTION__,__LINE__,
            unit->getId(), unit->getFullName().c_str(),unit->getDesc().c_str(),
            unit->getFaction()->getIndex());

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",szBuf);

	    std::string worldLog = world->DumpWorldToLog();
	    std::string sError = "worldLog = " + worldLog + " " + string(szBuf);
		throw runtime_error(sError);
	}

	if(executeCommandOverNetwork() == true) {
		Unit *targetUnit = u;
		const Unit *unit = getMyUnit(unitIndex);

		CommandResult result = commander->tryGiveCommand(unit, commandType, Vec2i(0), unit->getType(),CardinalDir::NORTH,false,targetUnit);

		return result;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, u));

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
}

// ==================== get data ====================

int AiInterface::getMapMaxPlayers(){
     return world->getMaxPlayers();
}

Vec2i AiInterface::getHomeLocation(){
	return world->getMap()->getStartLocation(world->getFaction(factionIndex)->getStartLocationIndex());
}

Vec2i AiInterface::getStartLocation(int loactionIndex){
	return world->getMap()->getStartLocation(loactionIndex);
}

int AiInterface::getFactionCount(){
     return world->getFactionCount();
}

int AiInterface::getMyUnitCount() const{
	return world->getFaction(factionIndex)->getUnitCount();
}

int AiInterface::getMyUpgradeCount() const{
	return world->getFaction(factionIndex)->getUpgradeManager()->getUpgradeCount();
}

int AiInterface::onSightUnitCount() {
    int count=0;
	Map *map= world->getMap();
	for(int i=0; i<world->getFactionCount(); ++i) {
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = world->getFaction(i)->getUnit(j);
			SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
			bool cannotSeeUnit = (unit->getType()->hasCellMap() == true &&
								  unit->getType()->getAllowEmptyCellMap() == true &&
								  unit->getType()->hasEmptyCellMap() == true);
			if(sc->isVisible(teamIndex) && cannotSeeUnit == false) {
				count++;
			}
		}
	}
    return count;
}

const Resource *AiInterface::getResource(const ResourceType *rt){
	return world->getFaction(factionIndex)->getResource(rt);
}

Unit *AiInterface::getMyUnitPtr(int unitIndex) {
	if(unitIndex >= world->getFaction(factionIndex)->getUnitCount()) {
		char szBuf[1024]="";
		sprintf(szBuf,"In [%s::%s Line: %d] unitIndex >= world->getFaction(factionIndex)->getUnitCount(), unitIndex = %d, world->getFaction(factionIndex)->getUnitCount() = %d",__FILE__,__FUNCTION__,__LINE__,unitIndex,world->getFaction(factionIndex)->getUnitCount());
		throw runtime_error(szBuf);
	}

	return world->getFaction(factionIndex)->getUnit(unitIndex);
}

const Unit *AiInterface::getMyUnit(int unitIndex) {
	return getMyUnitPtr(unitIndex);
}

const Unit *AiInterface::getOnSightUnit(int unitIndex) {

    int count=0;
	Map *map= world->getMap();

	for(int i=0; i<world->getFactionCount(); ++i) {
        for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j) {
            Unit * unit= world->getFaction(i)->getUnit(j);
            SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
			bool cannotSeeUnit = (unit->getType()->hasCellMap() == true &&
								  unit->getType()->getAllowEmptyCellMap() == true &&
								  unit->getType()->hasEmptyCellMap() == true);

            if(sc->isVisible(teamIndex)  && cannotSeeUnit == false) {
				if(count==unitIndex) {
					return unit;
				}
				else {
					count ++;
				}
            }
        }
	}
    return NULL;
}

const FactionType * AiInterface::getMyFactionType(){
	return world->getFaction(factionIndex)->getType();
}

const ControlType AiInterface::getControlType(){
	return world->getFaction(factionIndex)->getControlType();
}

const TechTree *AiInterface::getTechTree(){
	return world->getTechTree();
}


bool AiInterface::isResourceInRegion(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos, int range) const {
	const Map *map= world->getMap();
	RandomGen random;
	int xi=1;
	int xj=1;
	if(random.randRange(0,1)==1){
		xi=-1;
	}
	if(random.randRange(0,1)==1){
		xj=-1;
	}
	for(int i = -range; i <= range; ++i) {
		for(int j = -range; j <= range; ++j) {
			int ii=xi*i;
			int jj=xj*j;
			if(map->isInside(pos.x + ii, pos.y + jj)) {
				Resource *r= map->getSurfaceCell(map->toSurfCoords(Vec2i(pos.x + ii, pos.y + jj)))->getResource();
				if(r != NULL) {
					if(r->getType() == rt) {
						resourcePos= pos + Vec2i(ii,jj);
						return true;
					}
				}
			}
		}
	}
	return false;
}



//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool AiInterface::isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos, Faction *faction, bool fallbackToPeersHarvestingSameResource) const {
	const Map *map= world->getMap();
	int size = 1;
	for(int i = -1; i <= size; ++i) {
		for(int j = -1; j <= size; ++j) {
			if(map->isInside(pos.x + i, pos.y + j)) {
				Resource *r= map->getSurfaceCell(map->toSurfCoords(Vec2i(pos.x + i, pos.y + j)))->getResource();
				if(r != NULL) {
					if(r->getType() == rt) {
						resourcePos= pos + Vec2i(i,j);

						//if(faction != NULL) {
						//	char szBuf[4096]="";
						//				sprintf(szBuf,"[%s::%s Line: %d] [isresourcenear] pos [%s] resourcePos [%s] fallbackToPeersHarvestingSameResource [%d] rt [%s]",
						//						__FILE__,__FUNCTION__,__LINE__,pos.getString().c_str(),resourcePos.getString().c_str(),fallbackToPeersHarvestingSameResource,rt->getName().c_str());

						//	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",faction->getFrameCount());
						//	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s",szBuf);
						//	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",faction->getFrameCount());
						//}
						return true;
					}
				}
			}
		}
	}

	if(fallbackToPeersHarvestingSameResource == true && faction != NULL) {
		// Look for another unit that is currently harvesting the same resource
		// type right now

		// Check the faction cache for a known position where we can harvest
		// this resource type
		Vec2i result = faction->getClosestResourceTypeTargetFromCache(pos, rt);
		if(result.x >= 0) {
			resourcePos = result;

		  	//char szBuf[4096]="";
			//	    	sprintf(szBuf,"[%s::%s Line: %d] [isresourcenear] pos [%s] resourcePos [%s] fallbackToPeersHarvestingSameResource [%d] rt [%s]",
			//	    			__FILE__,__FUNCTION__,__LINE__,pos.getString().c_str(),resourcePos.getString().c_str(),fallbackToPeersHarvestingSameResource,rt->getName().c_str());

			//SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",faction->getFrameCount());
			//SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s",szBuf);
			//SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",faction->getFrameCount());

			if(pos.dist(resourcePos) <= size) {
				return true;
			}
		}
	}

	//if(faction != NULL) {
	//	char szBuf[4096]="";
	//				sprintf(szBuf,"[%s::%s Line: %d] [isresourcenear] pos [%s] resourcePos [%s] fallbackToPeersHarvestingSameResource [%d] rt [%s] getCacheResourceTargetListSize() [%d]",
	//						__FILE__,__FUNCTION__,__LINE__,pos.getString().c_str(),resourcePos.getString().c_str(),fallbackToPeersHarvestingSameResource,rt->getName().c_str(),faction->getCacheResourceTargetListSize());

	//	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",faction->getFrameCount());
	//	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s",szBuf);
	//	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",faction->getFrameCount());
	//}

	return false;
}

bool AiInterface::getNearestSightedResource(const ResourceType *rt, const Vec2i &pos,
											Vec2i &resultPos, bool usableResourceTypeOnly) {
	Faction *faction = world->getFaction(factionIndex);
	float tmpDist=0;
	float nearestDist= infinity;
	bool anyResource= false;
	resultPos.x = -1;
	resultPos.y = -1;

	bool canUseResourceType = (usableResourceTypeOnly == false);
	if(usableResourceTypeOnly == true) {
		// can any unit harvest this resource yet?
		std::map<const ResourceType *,int>::iterator iterFind = cacheUnitHarvestResourceLookup.find(rt);

		if(	iterFind != cacheUnitHarvestResourceLookup.end() &&
			faction->findUnit(iterFind->second) != NULL) {
			canUseResourceType = true;
		}
		else {
			int unitCount = getMyUnitCount();
			for(int i = 0; i < unitCount; ++i) {
				const Unit *unit = getMyUnit(i);
				const HarvestCommandType *hct= unit->getType()->getFirstHarvestCommand(rt,unit->getFaction());
				if(hct != NULL) {
					canUseResourceType = true;
					cacheUnitHarvestResourceLookup[rt] = unit->getId();
					break;
				}
			}
		}
	}

	if(canUseResourceType == true) {
		bool isResourceClose = isResourceNear(pos, rt, resultPos, faction, true);
		//bool isResourceClose = false;

		// Found a resource
		if(isResourceClose == true || resultPos.x >= 0) {
			anyResource= true;
		}
		else {
			const Map *map		= world->getMap();
			Faction *faction 	= world->getFaction(factionIndex);

			for(int i = 0; i < map->getW(); ++i) {
				for(int j = 0; j < map->getH(); ++j) {
					Vec2i resPos = Vec2i(i, j);
					Vec2i surfPos= Map::toSurfCoords(resPos);
					SurfaceCell *sc = map->getSurfaceCell(surfPos);

					//if explored cell
					if(sc != NULL && sc->isExplored(teamIndex)) {
						Resource *r= sc->getResource();

						//if resource cell
						if(r != NULL) {
							if(r->getType() == rt) {
								tmpDist= pos.dist(resPos);
								if(tmpDist < nearestDist) {
									anyResource= true;
									nearestDist= tmpDist;
									resultPos= resPos;
								}
							}

							//faction->addResourceTargetToCache(resPos,false);
						}
					}
				}
			}
		}
	}
	return anyResource;
}

bool AiInterface::isAlly(const Unit *unit) const{
	return world->getFaction(factionIndex)->isAlly(unit->getFaction());
}
bool AiInterface::reqsOk(const RequirableType *rt){
    return world->getFaction(factionIndex)->reqsOk(rt);
}

bool AiInterface::reqsOk(const CommandType *ct){
    return world->getFaction(factionIndex)->reqsOk(ct);
}

bool AiInterface::checkCosts(const ProducibleType *pt){
	return world->getFaction(factionIndex)->checkCosts(pt);
}

bool AiInterface::isFreeCells(const Vec2i &pos, int size, Field field){
    return world->getMap()->isFreeCells(pos, size, field);
}

const Unit *AiInterface::getFirstOnSightEnemyUnit(Vec2i &pos, Field &field, int radius) {
	Map *map= world->getMap();

	for(int i = 0; i < world->getFactionCount(); ++i) {
        for(int j = 0; j < world->getFaction(i)->getUnitCount(); ++j) {
            Unit * unit= world->getFaction(i)->getUnit(j);
            SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
			bool cannotSeeUnit = (unit->getType()->hasCellMap() == true &&
								  unit->getType()->getAllowEmptyCellMap() == true &&
								  unit->getType()->hasEmptyCellMap() == true);

            if(sc->isVisible(teamIndex)  && cannotSeeUnit == false &&
               isAlly(unit) == false && unit->isAlive() == true) {
                pos= unit->getPos();
    			field= unit->getCurrField();
                if(pos.dist(getHomeLocation()) < radius) {
                    printLog(2, "Being attacked at pos "+intToStr(pos.x)+","+intToStr(pos.y)+"\n");

                    return unit;
                }
            }
        }
	}
    return NULL;
}

Map * AiInterface::getMap() {
	Map *map= world->getMap();
	return map;
}

bool AiInterface::factionUsesResourceType(const FactionType *factionType, const ResourceType *rt) {
	bool factionUsesResourceType = factionType->factionUsesResourceType(rt);
	return factionUsesResourceType;
}

}}//end namespace
