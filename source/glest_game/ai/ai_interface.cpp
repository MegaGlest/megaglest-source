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
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

// =====================================================
// 	class AiInterface
// =====================================================

namespace Glest{ namespace Game{

bool AiInterface::enableServerControlledAI = false;

AiInterface::AiInterface(Game &game, int factionIndex, int teamIndex){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->world= game.getWorld();
	this->commander= game.getCommander();
	this->console= game.getConsole();
	this->gameSettings = game.getGameSettings();

	this->factionIndex= factionIndex;
	this->teamIndex= teamIndex;
	timer= 0;

	AiInterface::enableServerControlledAI = Config::getInstance().getBool("ServerControlledAI","false");

	//init ai
	ai.init(this);

	//config
	logLevel= Config::getInstance().getInt("AiLog");
	redir= Config::getInstance().getBool("AiRedir");

	//clear log file
	if(logLevel>0){
		FILE *f= fopen(getLogFilename().c_str(), "wt");
		if(f==NULL){
			throw runtime_error("Can't open file: "+getLogFilename());
		}
		fprintf(f, "%s", "Glest AI log file\n\n");
		fclose(f);
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== main ==================== 

void AiInterface::update(){
	timer++;
	ai.update();
}

// ==================== misc ==================== 

void AiInterface::printLog(int logLevel, const string &s){
    if(this->logLevel>=logLevel){
		string logString= "(" + intToStr(factionIndex) + ") " + s;

		//print log to file
		FILE *f= fopen(getLogFilename().c_str(), "at");
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

CommandResult AiInterface::giveCommand(int unitIndex, CommandClass commandClass, const Vec2i &pos){
	assert(this->gameSettings != NULL);

	if(enableServerControlledAI == true &&
		this->gameSettings->isNetworkGame() == true &&
		NetworkManager::getInstance().getNetworkRole() == nrServer) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = commander->tryGiveCommand(world->getFaction(factionIndex)->getUnit(unitIndex), world->getFaction(factionIndex)->getUnit(unitIndex)->getType()->getFirstCtOfClass(commandClass), pos, world->getFaction(factionIndex)->getUnit(unitIndex)->getType(),CardinalDir::NORTH);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
	else {
		Command *c= new Command (world->getFaction(factionIndex)->getUnit(unitIndex)->getType()->getFirstCtOfClass(commandClass), pos);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(c);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos){
	assert(this->gameSettings != NULL);

	if(enableServerControlledAI == true &&
		this->gameSettings->isNetworkGame() == true &&
		NetworkManager::getInstance().getNetworkRole() == nrServer) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = commander->tryGiveCommand(world->getFaction(factionIndex)->getUnit(unitIndex), commandType, pos, world->getFaction(factionIndex)->getUnit(unitIndex)->getType(),CardinalDir::NORTH);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, pos));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos, const UnitType *ut){
	assert(this->gameSettings != NULL);

	if(enableServerControlledAI == true &&
		this->gameSettings->isNetworkGame() == true &&
		NetworkManager::getInstance().getNetworkRole() == nrServer) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = commander->tryGiveCommand(world->getFaction(factionIndex)->getUnit(unitIndex), commandType, pos, ut,CardinalDir::NORTH);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, pos, ut, CardinalDir::NORTH));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, Unit *u){
	assert(this->gameSettings != NULL);
	assert(this->commander != NULL);
	//assert(u != NULL);
	//assert(u->getType() != NULL);

	if(enableServerControlledAI == true &&
		this->gameSettings->isNetworkGame() == true &&
		NetworkManager::getInstance().getNetworkRole() == nrServer) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//assert(u != NULL);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(u == NULL) {
			u = world->getFaction(factionIndex)->getUnit(unitIndex);
		}
		CommandResult result = commander->tryGiveCommand(u, commandType, Vec2i(0), u->getType(),CardinalDir::NORTH);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		return result;
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		CommandResult result = world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, u));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

int AiInterface::onSightUnitCount(){
    int count=0;
	Map *map= world->getMap();
	for(int i=0; i<world->getFactionCount(); ++i){
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
			SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(world->getFaction(i)->getUnit(j)->getPos()));
			if(sc->isVisible(teamIndex)){     
				count++;     
			}
		}	
	}
    return count;
}

const Resource *AiInterface::getResource(const ResourceType *rt){
	return world->getFaction(factionIndex)->getResource(rt);
}

const Unit *AiInterface::getMyUnit(int unitIndex){
    return world->getFaction(factionIndex)->getUnit(unitIndex);
}

const Unit *AiInterface::getOnSightUnit(int unitIndex){
     
    int count=0;
	Map *map= world->getMap();

	for(int i=0; i<world->getFactionCount(); ++i){
        for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
            Unit *u= world->getFaction(i)->getUnit(j);
            if(map->getSurfaceCell(Map::toSurfCoords(u->getPos()))->isVisible(teamIndex)){
				if(count==unitIndex){
					return u;
				}
				else{
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

bool AiInterface::getNearestSightedResource(const ResourceType *rt, const Vec2i &pos, Vec2i &resultPos){
	float tmpDist;

	float nearestDist= infinity;
	bool anyResource= false;

	const Map *map= world->getMap();

	for(int i=0; i<map->getW(); ++i){
		for(int j=0; j<map->getH(); ++j){
			Vec2i surfPos= Map::toSurfCoords(Vec2i(i, j));
			
			//if explored cell
			if(map->getSurfaceCell(surfPos)->isExplored(teamIndex)){
				Resource *r= map->getSurfaceCell(surfPos)->getResource(); 
				
				//if resource cell
				if(r!=NULL && r->getType()==rt){
					tmpDist= pos.dist(Vec2i(i, j));
					if(tmpDist<nearestDist){
						anyResource= true;
						nearestDist= tmpDist;
						resultPos= Vec2i(i, j);
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

}}//end namespace
