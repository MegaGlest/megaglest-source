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
#include "faction.h"

#include <cassert>

#include "unit.h"
#include "unit_particle_type.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "game.h"
#include "socket.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;


namespace Glest{ namespace Game{

const int UnitPathBasic::maxBlockCount= GameConstants::updateFps / 2;

UnitPathBasic::UnitPathBasic() {
	this->blockCount = 0;
	this->pathQueue.clear();
}

bool UnitPathBasic::isEmpty() const {
	return pathQueue.empty();
}

bool UnitPathBasic::isBlocked() const {
	return blockCount >= maxBlockCount;
}

bool UnitPathBasic::isStuck() const {
	return (isBlocked() == true && blockCount >= (maxBlockCount * 2));
}

void UnitPathBasic::clear() {
	pathQueue.clear();
	blockCount= 0;
}

void UnitPathBasic::incBlockCount() {
	pathQueue.clear();
	blockCount++;
}

void UnitPathBasic::add(const Vec2i &path){
	pathQueue.push_back(path);
}

Vec2i UnitPathBasic::pop() {
	Vec2i p= pathQueue.front();
	pathQueue.erase(pathQueue.begin());
	return p;
}
std::string UnitPathBasic::toString() const {
	std::string result = "";

	result = "unit path blockCount = " + intToStr(blockCount) + " pathQueue size = " + intToStr(pathQueue.size());
	for(int idx = 0; idx < pathQueue.size(); idx++) {
		result += " index = " + intToStr(idx) + " " + pathQueue[idx].getString();
	}

	return result;
}
// =====================================================
// 	class UnitPath
// =====================================================

void WaypointPath::condense() {
	if (size() < 2) {
		return;
	}
	iterator prev, curr;
	prev = curr = begin();
	while (++curr != end()) {
		if (prev->dist(*curr) < 3.f) {
			prev = erase(prev);
		} else {
			++prev;
		}
	}
}

std::string UnitPath::toString() const {
	std::string result = "";

	result = "unit path blockCount = " + intToStr(blockCount) + " pathQueue size = " + intToStr(size());
	result += " path = ";
	for (const_iterator it = begin(); it != end(); ++it) {
		result += " [" + intToStr(it->x) + "," + intToStr(it->y) + "]";
	}

	return result;
}

// =====================================================
// 	class UnitReference
// =====================================================

UnitReference::UnitReference(){
	id= -1;
	faction= NULL;
}

void UnitReference::operator=(const Unit *unit){
	if(unit==NULL){
		id= -1;
		faction= NULL;
	}
	else{
		id= unit->getId();
		faction= unit->getFaction();
	}
}

Unit *UnitReference::getUnit() const{
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

	if(faction!=NULL){
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

		return faction->findUnit(id);
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
	return NULL;
}

// =====================================================
// 	class Unit
// =====================================================

const float Unit::speedDivider= 100.f;
const int Unit::maxDeadCount= 1000;	//time in until the corpse disapears - should be about 40 seconds
const float Unit::highlightTime= 0.5f;
const int Unit::invalidId= -1;

set<int> Unit::livingUnits;
set<Unit*> Unit::livingUnitsp;

// ============================ Constructor & destructor =============================

Game *Unit::game = NULL;

Unit::Unit(int id, UnitPathInterface *unitpath, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, CardinalDir placeFacing):id(id) {

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	modelFacing = CardinalDir::NORTH;

    RandomGen random;

    this->unitPath = unitpath;
	this->pos=pos;
	this->type=type;
    this->faction=faction;
	this->map= map;
	this->targetRef = NULL;
	this->targetField = fLand;
	this->targetVec   = Vec3f(0.0);
	this->targetPos   = Vec2i(0);
	this->lastRenderFrame = 0;
	this->visible = true;
	this->retryCurrCommandCount=0;
	this->screenPos = Vec3f(0.0);
	this->inBailOutAttempt = false;
	this->lastHarvestResourceTarget.first = Vec2i(0);
	this->lastBadHarvestListPurge = 0;

	level= NULL;
	loadType= NULL;

    setModelFacing(placeFacing);

    Config &config= Config::getInstance();
	showUnitParticles= config.getBool("UnitParticles");

	lastPos= pos;
    progress= 0;
	lastAnimProgress= 0;
    animProgress= 0;
    progress2= 0;
	kills= 0;
	loadCount= 0;
    ep= 0;
	deadCount= 0;
	hp= type->getMaxHp()/20;
	toBeUndertaken= false;

	highlight= 0.f;
	meetingPos= pos;
	alive= true;

	if (type->hasSkillClass(scBeBuilt) == false) {
		float rot= 0.f;
		random.init(id);
		rot+= random.randRange(-5, 5);
		rotation= rot;
		lastRotation= rot;
		targetRotation= rot;
	}
	// else it was set appropriately in setModelFacing()

    if(getType()->getField(fAir)) {
    	currField = fAir;
    }
    if(getType()->getField(fLand)) {
    	currField = fLand;
    }

    fire= NULL;

	computeTotalUpgrade();

	//starting skill
	this->currSkill = getType()->getFirstStOfClass(scStop);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Unit ID = %d [%s], this->currSkill = %s\n",__FILE__,__FUNCTION__,__LINE__,this->getId(),this->getFullName().c_str(), (this->currSkill == NULL ? "" : this->currSkill->toString().c_str()));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
	livingUnits.insert(id);
	livingUnitsp.insert(this);

	logSynchData(string(__FILE__) + string("::") + string(__FUNCTION__) + string(" Line: ") + intToStr(__LINE__));
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

Unit::~Unit(){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] delete unitid = %d\n",__FILE__,__FUNCTION__,__LINE__,id);

	badHarvestPosList.clear();

	//Just to be sure, should already be removed
	if (livingUnits.erase(id)) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//Only an error if not called at end
	}

	//Just to be sure, should already be removed
	if (livingUnitsp.erase(this)) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//remove commands
	while(commands.empty() == false) {
		delete commands.back();
		commands.pop_back();
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// If the unit is not visible we better make sure we cleanup associated particles
	if(this->getVisible() == false) {
		Renderer::getInstance().cleanupUnitParticleSystems(unitParticleSystems,rsGame);

		Renderer::getInstance().cleanupParticleSystems(fireParticleSystems,rsGame);
		// Must set this to null of it will be used below in stopDamageParticles()

		if(Renderer::getInstance().validateParticleSystemStillExists(fire,rsGame) == false) {
			fire = NULL;
		}
	}

	// fade(and by this remove) all unit particle systems
	while(unitParticleSystems.empty() == false) {
		unitParticleSystems.back()->fade();
		unitParticleSystems.pop_back();
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	stopDamageParticles();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete this->unitPath;
	this->unitPath = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Renderer &renderer= Renderer::getInstance();
	//renderer.setQuadCacheDirty(true);
	renderer.removeUnitFromQuadCache(this);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Unit::setModelFacing(CardinalDir value) { 
	modelFacing = value;
	lastRotation = targetRotation = rotation = value * 90.f;
}

// ====================================== get ======================================

int Unit::getFactionIndex() const{
	return faction->getIndex();
}

int Unit::getTeam() const{
	return faction->getTeam();
}

Vec2i Unit::getCenteredPos() const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

    return pos + Vec2i(type->getSize()/2, type->getSize()/2);
}

Vec2f Unit::getFloatCenteredPos() const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	return Vec2f(pos.x-0.5f+type->getSize()/2.f, pos.y-0.5f+type->getSize()/2.f);
}

Vec2i Unit::getCellPos() const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(type->hasCellMap()) {
		if( type->hasEmptyCellMap() == false ||
			type->getAllowEmptyCellMap() == true) {

			//find nearest pos to center that is free
			Vec2i centeredPos= getCenteredPos();
			float nearestDist= -1.f;
			Vec2i nearestPos= pos;

			for(int i=0; i<type->getSize(); ++i){
				for(int j=0; j<type->getSize(); ++j){
					if(type->getCellMapCell(i, j, modelFacing)){
						Vec2i currPos= pos + Vec2i(i, j);
						float dist= currPos.dist(centeredPos);
						if(nearestDist==-1.f || dist<nearestDist){
							nearestDist= dist;
							nearestPos= currPos;
						}
					}
				}
			}
			return nearestPos;
		}
	}
	return pos;
}

float Unit::getVerticalRotation() const{
	/*if(type->getProperty(UnitType::pRotatedClimb) && currSkill->getClass()==scMove){
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		float dist= pos.dist(targetPos);
		return radToDeg(streflop::atan2(heightDiff, dist));
	}*/
	return 0.f;
}

int Unit::getProductionPercent() const{
	if(anyCommand()){
		const ProducibleType *produced= commands.front()->getCommandType()->getProduced();
		if(produced!=NULL){
			return clamp(progress2*100/produced->getProductionTime(), 0, 100);
		}
	}
	return -1;
}

float Unit::getHpRatio() const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	return clamp(static_cast<float>(hp)/type->getTotalMaxHp(&totalUpgrade), 0.f, 1.f);
}

float Unit::getEpRatio() const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(type->getMaxHp()==0){
		return 0.f;
	}
	else{
		return clamp(static_cast<float>(ep)/type->getTotalMaxEp(&totalUpgrade), 0.f, 1.f);
	}
}

const Level *Unit::getNextLevel() const{
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(level==NULL && type->getLevelCount()>0){
		return type->getLevel(0);
	}
	else{
		for(int i=1; i<type->getLevelCount(); ++i){
			if(type->getLevel(i-1)==level){
				return type->getLevel(i);
			}
		}
	}
	return NULL;
}

string Unit::getFullName() const{
	string str="";
	if(level != NULL){
		str += (level->getName() + " ");
	}
	if(type == NULL) {
	    throw runtime_error("type == NULL in Unit::getFullName()!");
	}
	str += type->getName();
	return str;
}

// ====================================== is ======================================

bool Unit::isOperative() const{
    return isAlive() && isBuilt();
}

bool Unit::isBeingBuilt() const{
	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

    return (currSkill->getClass() == scBeBuilt);
}

bool Unit::isBuilt() const{
    return (isBeingBuilt() == false);
}

bool Unit::isPutrefacting() const{
	return deadCount!=0;
}

bool Unit::isAlly(const Unit *unit) const {
	if(unit == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: unit == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	return faction->isAlly(unit->getFaction());
}

bool Unit::isDamaged() const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	return hp < type->getTotalMaxHp(&totalUpgrade);
}

bool Unit::isInteresting(InterestingUnitType iut) const {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	switch(iut){
	case iutIdleHarvester:
		if(type->hasCommandClass(ccHarvest)){
			if(!commands.empty()){
				const CommandType *ct= commands.front()->getCommandType();
				if(ct!=NULL){
					return ct->getClass()==ccStop;
				}
			}
		}
		return false;

	case iutBuiltBuilding:
		return type->hasSkillClass(scBeBuilt) && isBuilt();
	case iutProducer:
		return type->hasSkillClass(scProduce);
	case iutDamaged:
		return isDamaged();
	case iutStore:
		return type->getStoredResourceCount()>0;
	default:
		return false;
	}
}

// ====================================== set ======================================

void Unit::setCurrSkill(const SkillType *currSkill) {
	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}
	if(this->currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: this->currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Unit ID = %d [%s], this->currSkill = %s\n currSkill = %s\n",__FILE__,__FUNCTION__,__LINE__,this->getId(), this->getFullName().c_str(), this->currSkill->toString().c_str(),currSkill->toString().c_str());

	if(currSkill->getClass() != this->currSkill->getClass()) {
		animProgress= 0;
		lastAnimProgress= 0;

		while(unitParticleSystems.empty() == false) {
			unitParticleSystems.back()->fade();
			unitParticleSystems.pop_back();
		}
	}
	if(showUnitParticles && (currSkill->unitParticleSystemTypes.empty() == false) &&
		(unitParticleSystems.empty() == true) ) {
		for(UnitParticleSystemTypes::const_iterator it= currSkill->unitParticleSystemTypes.begin(); it != currSkill->unitParticleSystemTypes.end(); ++it) {
			UnitParticleSystem *ups = new UnitParticleSystem(200);
			(*it)->setValues(ups);
			ups->setPos(getCurrVector());
			ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
			unitParticleSystems.push_back(ups);
			Renderer::getInstance().manageParticleSystem(ups, rsGame);
		}
	}
	progress2= 0;
	this->currSkill= currSkill;
}

void Unit::setCurrSkill(SkillClass sc) {
	if(getType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: getType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

    setCurrSkill(getType()->getFirstStOfClass(sc));
}

void Unit::setTarget(const Unit *unit){

	if(unit == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: unit == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//find a free pos in cellmap
	setTargetPos(unit->getCellPos());

	//ser field and vector
	targetField= unit->getCurrField();
	targetVec= unit->getCurrVector();
	targetRef= unit;
}

void Unit::setPos(const Vec2i &pos){
	this->lastPos= this->pos;
	this->pos= pos;
	this->meetingPos= pos - Vec2i(1);

	// Attempt to improve performance
	this->exploreCells();

	logSynchData(string(__FILE__) + string("::") + string(__FUNCTION__) + string(" Line: ") + intToStr(__LINE__));
}

void Unit::setTargetPos(const Vec2i &targetPos){

	Vec2i relPos= targetPos - pos;
	Vec2f relPosf= Vec2f((float)relPos.x, (float)relPos.y);
#ifdef USE_STREFLOP
	targetRotation= radToDeg(streflop::atan2(relPosf.x, relPosf.y));
#else
	targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));
#endif
	targetRef= NULL;

	//this->targetField = fLand;
	//this->targetVec   = Vec3f(0.0);
	//this->targetPos   = Vec2i(0);

	this->targetPos= targetPos;

	logSynchData(string(__FILE__) + string("::") + string(__FUNCTION__) + string(" Line: ") + intToStr(__LINE__));
}

void Unit::setVisible(const bool visible) {
	this->visible = visible;

	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		(*it)->setVisible(visible);
	}
	for(UnitParticleSystems::iterator it= damageParticleSystems.begin(); it != damageParticleSystems.end(); ++it) {
		(*it)->setVisible(visible);
	}
}

// =============================== Render related ==================================

const Model *Unit::getCurrentModel() const{
	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

    return currSkill->getAnimation();
}

Vec3f Unit::getCurrVector() const{
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	return getCurrVectorFlat() + Vec3f(0.f, type->getHeight()/2.f, 0.f);
}

Vec3f Unit::getCurrVectorFlat() const{
    Vec3f v;

	float y1= computeHeight(lastPos);
	float y2= computeHeight(pos);

    if(currSkill->getClass() == scMove) {
        v.x= lastPos.x + progress * (pos.x-lastPos.x);
        v.z= lastPos.y + progress * (pos.y-lastPos.y);
		v.y= y1+progress*(y2-y1);
    }
    else{
        v.x= static_cast<float>(pos.x);
        v.z= static_cast<float>(pos.y);
        v.y= y2;
    }
    v.x+= type->getSize()/2.f-0.5f;
    v.z+= type->getSize()/2.f-0.5f;

    return v;
}

// =================== Command list related ===================

//any command
bool Unit::anyCommand(bool validateCommandtype) const {
	bool result = false;
	if(validateCommandtype == false) {
		result = (commands.empty() == false);
	}
	else {
		for(Commands::const_iterator it= commands.begin(); it != commands.end(); ++it) {
			const CommandType *ct = (*it)->getCommandType();
			if(ct != NULL && ct->getClass() != ccStop) {
				result = true;
				break;
			}
		}
	}

	return result;
}

//return current command, assert that there is always one command
Command *Unit::getCurrCommand() const{
	if(commands.empty() == false) {
		return commands.front();
	}
	return NULL;
}

void Unit::replaceCurrCommand(Command *cmd) {
	assert(!commands.empty());
	commands.front() = cmd;
	this->setCurrentUnitTitle("");
}

//returns the size of the commands
unsigned int Unit::getCommandSize() const{
	return commands.size();
}

//return current command, assert that there is always one command
int Unit::getCountOfProducedUnits(const UnitType *ut) const{
	int count=0;
	for(Commands::const_iterator it= commands.begin(); it!=commands.end(); ++it){
			const CommandType* ct=(*it)->getCommandType();
        	if(ct->getClass()==ccProduce || ct->getClass()==ccMorph ){
        		const UnitType *producedUnitType= static_cast<const UnitType*>(ct->getProduced());
        		if(producedUnitType==ut)
        		{
        			count++;
        		}
        	}
        	if(ct->getClass()==ccBuild){
        		const UnitType *builtUnitType= (*it)->getUnitType();
        		if(builtUnitType==ut)
        		{
        			count++;
        		}
        	}
	}
	return count;
}

#define deleteSingleCommand(command) {\
	undoCommand(command);\
	delete command;\
}

//give one command (clear, and push back)
CommandResult Unit::giveCommand(Command *command, bool tryQueue) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);

    SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"\n======================\nUnit Command tryQueue = %d\nUnit Info:\n%s\nCommand Info:\n%s\n",tryQueue,this->toString().c_str(),command->toString().c_str());

    assert(command != NULL);

    assert(command->getCommandType() != NULL);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Unit id = %d name = %s, Command [%s] tryQueue = %d\n",
    		__FILE__,__FUNCTION__, __LINE__,this->id,(this->type != NULL ? this->type->getName().c_str() : "null"), (command != NULL ? command->toString().c_str() : "null"),tryQueue);

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    const int command_priority = command->getCommandType()->getPriority();

	if(command->getCommandType()->isQueuable(tryQueue) && (commands.empty() || (command_priority >= commands.back()->getCommandType()->getPriority()))){
		//Delete all lower-prioirty commands
		for (list<Command*>::iterator i = commands.begin(); i != commands.end();) {
			if ((*i)->getCommandType()->getPriority() < command_priority) {
				deleteSingleCommand(*i);
				i = commands.erase(i);
			}
			else {
				++i;
			}
		}
		//cancel current command if it is not queuable
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(!commands.empty() && !commands.back()->getCommandType()->isQueueAppendable()){
		    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			cancelCommand();
		}
	}
	else {
		//empty command queue
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		clearCommands();
		this->unitPath->clear();
	}

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] A\n",__FILE__,__FUNCTION__);

	//check command
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	CommandResult result= checkCommand(command);
	if(result == crSuccess) {
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		applyCommand(command);
	}

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] B\n",__FILE__,__FUNCTION__);

	//push back command
	if(result == crSuccess) {
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		commands.push_back(command);
	}
	else {
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		delete command;
	}

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] result = %d\n",__FILE__,__FUNCTION__,__LINE__,result);

	return result;
}

//pop front (used when order is done)
CommandResult Unit::finishCommand() {

	retryCurrCommandCount=0;
	this->setCurrentUnitTitle("");
	//is empty?
	if(commands.empty()){
		return crFailUndefined;
	}

	//pop front
	delete commands.front();
	commands.erase(commands.begin());
	this->unitPath->clear();

	while (!commands.empty()) {
		if (commands.front()->getUnit() != NULL && livingUnitsp.find(commands.front()->getUnit()) == livingUnitsp.end()) {
			delete commands.front();
			commands.erase(commands.begin());
		} else {
			break;
		}
	}

	return crSuccess;
}

//to cancel a command
CommandResult Unit::cancelCommand() {

	retryCurrCommandCount=0;
	this->setCurrentUnitTitle("");

	//is empty?
	if(commands.empty()){
		return crFailUndefined;
	}

	//undo command
	undoCommand(commands.back());

	//delete ans pop command
	delete commands.back();
	commands.pop_back();

	//clear routes
	this->unitPath->clear();

	return crSuccess;
}

// =================== route stack ===================

void Unit::create(bool startingUnit){
	faction->addUnit(this);
	map->putUnitCells(this, pos);
	if(startingUnit){
		faction->applyStaticCosts(type);
	}
}

void Unit::born() {
	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Unit ID = %d [%s], this->currSkill = %s\n",__FILE__,__FUNCTION__,__LINE__,this->getId(),this->getFullName().c_str(), (this->currSkill == NULL ? "" : this->currSkill->toString().c_str()));

	faction->addStore(type);
	faction->applyStaticProduction(type);
	setCurrSkill(scStop);

	hp= type->getMaxHp();
}

void Unit::kill() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Unit ID = %d [%s], this->currSkill = %s\n",__FILE__,__FUNCTION__,__LINE__,this->getId(),this->getFullName().c_str(), (this->currSkill == NULL ? "" : this->currSkill->toString().c_str()));

	//no longer needs static resources
	if(isBeingBuilt()) {
		faction->deApplyStaticConsumption(type);
	}
	else {
		faction->deApplyStaticCosts(type);
	}

	//do the cleaning
	map->clearUnitCells(this, pos);
	if(isBeingBuilt() == false) {
		faction->removeStore(type);
	}
    setCurrSkill(scDie);

	notifyObservers(UnitObserver::eKill);

	//clear commands
	clearCommands();
}

void Unit::undertake() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to undertake unit id = %d [%s] [%s]\n",
			__FILE__,__FUNCTION__,__LINE__,this->id, this->getFullName().c_str(),this->getDesc().c_str());

	livingUnits.erase(id);
	livingUnitsp.erase(this);
	faction->removeUnit(this);
}

// =================== Referencers ===================

void Unit::addObserver(UnitObserver *unitObserver){
	observers.push_back(unitObserver);
}

void Unit::removeObserver(UnitObserver *unitObserver){
	observers.remove(unitObserver);
}

void Unit::notifyObservers(UnitObserver::Event event){
	for(Observers::iterator it= observers.begin(); it!=observers.end(); ++it){
		(*it)->unitEvent(event, this);
	}
}

// =================== Other ===================

void Unit::resetHighlight(){
	highlight= 1.f;
}

const CommandType *Unit::computeCommandType(const Vec2i &pos, const Unit *targetUnit) const{
	const CommandType *commandType= NULL;
	SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(pos));

	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(targetUnit!=NULL){
		//attack enemies
		if(!isAlly(targetUnit)){
			commandType= type->getFirstAttackCommand(targetUnit->getCurrField());
		}

		//repair allies
		else{
			commandType= type->getFirstRepairCommand(targetUnit->getType());
		}
	}
	else{
		//check harvest command
		Resource *resource= sc->getResource();
		if(resource!=NULL){
			commandType= type->getFirstHarvestCommand(resource->getType(),this->getFaction());
		}
	}

	//default command is move command
	if(commandType==NULL){
		commandType= type->getFirstCtOfClass(ccMove);
	}

	return commandType;
}

bool Unit::update() {
	assert(progress<=1.f);

	//highlight
	if(highlight>0.f) {
		const Game *game = Renderer::getInstance().getGame();
		highlight-= 1.f / (highlightTime * game->getWorld()->getUpdateFps(this->getFactionIndex()));
	}

	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//speed
	int speed= currSkill->getTotalSpeed(&totalUpgrade);

	//speed modifier
	float diagonalFactor= 1.f;
	float heightFactor= 1.f;
	if(currSkill->getClass() == scMove) {

		//if moving in diagonal move slower
		Vec2i dest= pos-lastPos;
		if(abs(dest.x)+abs(dest.y) == 2) {
			diagonalFactor= 0.71f;
		}

		//if movig to an higher cell move slower else move faster
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		heightFactor= clamp(1.f+heightDiff/5.f, 0.2f, 5.f);
	}


	//update progresses
	lastAnimProgress= animProgress;
	const Game *game = Renderer::getInstance().getGame();
	progress += (speed * diagonalFactor * heightFactor) / (speedDivider * game->getWorld()->getUpdateFps(this->getFactionIndex()));
	animProgress += (currSkill->getAnimSpeed() * heightFactor) / (speedDivider * game->getWorld()->getUpdateFps(this->getFactionIndex()));

	//update target
	updateTarget();

	//rotation
	if(currSkill->getClass() != scStop) {
		const int rotFactor= 2;
		if(progress<1.f/rotFactor){
			if(type->getFirstStOfClass(scMove)){
				if(abs((int)(lastRotation-targetRotation)) < 180)
					rotation= lastRotation+(targetRotation-lastRotation)*progress*rotFactor;
				else{
					float rotationTerm= targetRotation>lastRotation? -360.f: +360.f;
					rotation= lastRotation+(targetRotation-lastRotation+rotationTerm)*progress*rotFactor;
				}
			}
		}
	}

	if(Renderer::getInstance().validateParticleSystemStillExists(fire,rsGame) == false) {
		fire = NULL;
	}

	if (fire != NULL) {
		fire->setPos(getCurrVector());
	}
	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		(*it)->setPos(getCurrVector());
		(*it)->setRotation(getRotation());
	}
	for(UnitParticleSystems::iterator it= damageParticleSystems.begin(); it != damageParticleSystems.end(); ++it) {
		(*it)->setPos(getCurrVector());
		(*it)->setRotation(getRotation());
	}
	//checks
	if(animProgress>1.f) {
		animProgress= currSkill->getClass()==scDie? 1.f: 0.f;
	}

    bool return_value = false;
	//checks
	if(progress>=1.f) {
		lastRotation= targetRotation;
		if(currSkill->getClass() != scDie) {
			progress= 0.f;
			return_value = true;
		}
		else {
			progress= 1.f;
			deadCount++;
			if(deadCount>=maxDeadCount) {
				toBeUndertaken= true;
				return_value = false;
			}
		}
	}

	return return_value;
}

void Unit::tick() {

	if(isAlive()) {
		if(type == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

		//regenerate hp
		hp+= type->getHpRegeneration();
		if(hp>type->getTotalMaxHp(&totalUpgrade)){
			hp= type->getTotalMaxHp(&totalUpgrade);
		}
		//stop DamageParticles
		if(hp>type->getTotalMaxHp(&totalUpgrade)/2 ){
			stopDamageParticles();
		}
		//regenerate ep
		ep+= type->getEpRegeneration();
		if(ep>type->getTotalMaxEp(&totalUpgrade)){
			ep= type->getTotalMaxEp(&totalUpgrade);
		}
	}
}

int Unit::update2(){
     progress2++;
     return progress2;
}

bool Unit::computeEp() {

	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//if not enough ep
    if(ep-currSkill->getEpCost() < 0) {
        return true;
    }

	//decrease ep
    ep-= currSkill->getEpCost();

	if(getType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: getType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(ep>getType()->getTotalMaxEp(&totalUpgrade)){
        ep= getType()->getTotalMaxEp(&totalUpgrade);
	}

    return false;
}

bool Unit::repair(){

	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//increase hp
	hp+= getType()->getMaxHp()/type->getProductionTime() + 1;
    if(hp>(getType()->getTotalMaxHp(&totalUpgrade))){
        hp= getType()->getTotalMaxHp(&totalUpgrade);
        return true;
    }

	//stop DamageParticles
	if(hp>type->getTotalMaxHp(&totalUpgrade)/2 ){
		stopDamageParticles();
	}
    return false;
}

//decrements HP and returns if dead
bool Unit::decHp(int i){
	if(hp==0){
		return false;
	}

	hp-=i;

	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//startDamageParticles
	if(hp<type->getMaxHp()/2 ){
		startDamageParticles();
	}

	//stop DamageParticles on death
    if(hp<=0){
		alive= false;
        hp=0;
		stopDamageParticles();
		return true;
    }
    return false;
}

string Unit::getDesc() const{

    Lang &lang= Lang::getInstance();

    //pos
    //str+="Pos: "+v2iToStr(pos)+"\n";

	//hp
	string str= "\n";
	
	//maxUnitCount
	if(type->getMaxUnitCount()>0){
		str += lang.get("MaxUnitCount")+ ": " + intToStr(faction->getCountForMaxUnitCount(type)) + "/" + intToStr(type->getMaxUnitCount());
	}
	
	str += "\n"+lang.get("Hp")+ ": " + intToStr(hp) + "/" + intToStr(type->getTotalMaxHp(&totalUpgrade));
	if(type->getHpRegeneration()!=0){
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(type->getHpRegeneration()) + ")";
	}

	//ep
	if(getType()->getMaxEp()!=0){
		str+= "\n" + lang.get("Ep")+ ": " + intToStr(ep) + "/" + intToStr(type->getTotalMaxEp(&totalUpgrade));
	}
	if(type->getEpRegeneration()!=0){
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(type->getEpRegeneration()) + ")";
	}

	//armor
	str+= "\n" + lang.get("Armor")+ ": " + intToStr(getType()->getArmor());
	if(totalUpgrade.getArmor()!=0){
		str+="+"+intToStr(totalUpgrade.getArmor());
	}
	str+= " ("+getType()->getArmorType()->getName()+")";

	//sight
	str+="\n"+ lang.get("Sight")+ ": " + intToStr(getType()->getSight());
	if(totalUpgrade.getSight()!=0){
		str+="+"+intToStr(totalUpgrade.getSight());
	}

	//kills
	const Level *nextLevel= getNextLevel();
	if(kills>0 || nextLevel!=NULL){
		str+= "\n" + lang.get("Kills") +": " + intToStr(kills);
		if(nextLevel!=NULL){
			str+= " (" + nextLevel->getName() + ": " + intToStr(nextLevel->getKills()) + ")";
		}
	}

	//str+= "\nskl: "+scToStr(currSkill->getClass());

	//load
	if(loadCount!=0){
		str+= "\n" + lang.get("Load")+ ": " + intToStr(loadCount) +"  " + loadType->getName();
	}

	//consumable production
	for(int i=0; i<getType()->getCostCount(); ++i){
		const Resource *r= getType()->getCost(i);
		if(r->getType()->getClass()==rcConsumable){
			str+= "\n";
			str+= r->getAmount()<0? lang.get("Produce")+": ": lang.get("Consume")+": ";
			str+= intToStr(abs(r->getAmount())) + " " + r->getType()->getName();
		}
	}

	//command info
	if(!commands.empty()){
		str+= "\n" + commands.front()->getCommandType()->getName();
		if(commands.size()>1){
			str+="\n"+lang.get("OrdersOnQueue")+": "+intToStr(commands.size());
		}
	}
	else{
		//can store
		if(getType()->getStoredResourceCount()>0){
			for(int i=0; i<getType()->getStoredResourceCount(); ++i){
				const Resource *r= getType()->getStoredResource(i);
				str+= "\n"+lang.get("Store")+": ";
				str+= intToStr(r->getAmount()) + " " + r->getType()->getName();
			}
		}
	}

    return str;
}

void Unit::applyUpgrade(const UpgradeType *upgradeType){
	if(upgradeType == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: upgradeType == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(upgradeType->isAffected(type)){
		totalUpgrade.sum(upgradeType);
		hp+= upgradeType->getMaxHp();
	}
}

void Unit::computeTotalUpgrade(){
	faction->getUpgradeManager()->computeTotalUpgrade(this, &totalUpgrade);
}

void Unit::incKills(){
	++kills;

	const Level *nextLevel= getNextLevel();
	if(nextLevel!=NULL && kills>= nextLevel->getKills()){
		level= nextLevel;
		int maxHp= totalUpgrade.getMaxHp();
		totalUpgrade.incLevel(type);
		hp+= totalUpgrade.getMaxHp()-maxHp;
	}
}

bool Unit::morph(const MorphCommandType *mct){

	if(mct == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: mct == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	const UnitType *morphUnitType= mct->getMorphUnit();

	if(morphUnitType == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: morphUnitType == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

    Field morphUnitField=fLand;
    if(morphUnitType->getField(fAir)) morphUnitField=fAir;
    if(morphUnitType->getField(fLand)) morphUnitField=fLand;
    if(map->isFreeCellsOrHasUnit(pos, morphUnitType->getSize(), morphUnitField, this)){
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		hp+= morphUnitType->getMaxHp() - type->getMaxHp();
		type= morphUnitType;
		level= NULL;
		currField=morphUnitField;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(morphUnitType, mct->getDiscount());
		faction->applyStaticProduction(morphUnitType);
		return true;
	}
	else{
		return false;
	}
}


// ==================== PRIVATE ====================

float Unit::computeHeight(const Vec2i &pos) const{
	float height= map->getCell(pos)->getHeight();

	if(currField == fAir) {
		height += World::airHeight;

		Unit *unit = map->getCell(pos)->getUnit(fLand);
		if(unit != NULL && unit->getType()->getHeight() > World::airHeight) {
			height += (std::min((float)unit->getType()->getHeight(),World::airHeight * 3) - World::airHeight);
		}
		else {
			SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(pos));
			if(sc != NULL && sc->getObject() != NULL && sc->getObject()->getType() != NULL) {
				if(sc->getObject()->getType()->getHeight() > World::airHeight) {
					height += (std::min((float)sc->getObject()->getType()->getHeight(),World::airHeight * 3) - World::airHeight);
				}
			}
		}
	}

	return height;
}

void Unit::updateTarget(){
	Unit *target= targetRef.getUnit();
	if(target!=NULL){

		//update target pos
		targetPos= target->getCellPos();
		Vec2i relPos= targetPos - pos;
		Vec2f relPosf= Vec2f((float)relPos.x, (float)relPos.y);
#ifdef USE_STREFLOP
		targetRotation= radToDeg(streflop::atan2(relPosf.x, relPosf.y));
#else
		targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));
#endif
		//update target vec
		targetVec= target->getCurrVector();

		//if(getFrameCount() % 40 == 0) {
			//logSynchData(string(__FILE__) + string("::") + string(__FUNCTION__) + string(" Line: ") + intToStr(__LINE__));
			//logSynchData();
		//}
	}
}

void Unit::clearCommands() {
	this->setCurrentUnitTitle("");
	while(!commands.empty()){
		undoCommand(commands.back());
		delete commands.back();
		commands.pop_back();
	}
}

CommandResult Unit::checkCommand(Command *command) const {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(command == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}
	//if not operative or has not command type => fail
	if(!isOperative() || command->getUnit()==this || !getType()->hasCommandType(command->getCommandType())|| !this->getFaction()->reqsOk(command->getCommandType())){
        return crFailUndefined;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//if pos is not inside the world (if comand has not a pos, pos is (0, 0) and is inside world
	if(!map->isInside(command->getPos())){
        return crFailUndefined;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//check produced
	if(command->getCommandType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		if(!faction->reqsOk(produced)){
            return crFailReqs;
		}
		if(!faction->checkCosts(produced)){
			return crFailRes;
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //build command specific, check resources and requirements for building
    if(command->getCommandType()->getClass()==ccBuild){
		const UnitType *builtUnit= command->getUnitType();

		if(builtUnit == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: builtUnit == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

		if(!faction->reqsOk(builtUnit)){
            return crFailReqs;
		}
		if(!faction->checkCosts(builtUnit)){
			return crFailRes;
		}		
    }

    //upgrade command specific, check that upgrade is not upgraded
    else if(command->getCommandType()->getClass()==ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

		if(uct == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

		if(faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())){
            return crFailUndefined;
		}
	}

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    return crSuccess;
}

void Unit::applyCommand(Command *command){

	if(command == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}
	else if(command->getCommandType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//check produced
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->applyCosts(produced);
	}

    //build command specific
    if(command->getCommandType()->getClass()==ccBuild){
		faction->applyCosts(command->getUnitType());
    }

    //upgrade command specific
    else if(command->getCommandType()->getClass()==ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

		if(uct == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

        faction->startUpgrade(uct->getProducedUpgrade());
	}
}

CommandResult Unit::undoCommand(Command *command){

	if(command == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}
	else if(command->getCommandType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//return cost
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->deApplyCosts(produced);
	}

	//return building cost if not already building it or dead
	if(command->getCommandType()->getClass() == ccBuild){
		if(currSkill->getClass() != scBuild && currSkill->getClass() != scDie) {
			faction->deApplyCosts(command->getUnitType());
		}
	}

	//upgrade command cancel from list
	if(command->getCommandType()->getClass() == ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

        if(uct == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

        faction->cancelUpgrade(uct->getProducedUpgrade());
	}

	retryCurrCommandCount=0;
	this->setCurrentUnitTitle("");

	return crSuccess;
}

void Unit::stopDamageParticles() {

	if(Renderer::getInstance().validateParticleSystemStillExists(fire,rsGame) == false) {
		fire = NULL;
	}

	// stop fire
	if(fire != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		fire->fade();
		fire = NULL;
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	// stop additional particles
	while(damageParticleSystems.empty() == false) {
		damageParticleSystems.back()->fade();
		damageParticleSystems.pop_back();
	}
}

void Unit::startDamageParticles(){
	//start additional particles
	if( showUnitParticles && (!type->damageParticleSystemTypes.empty())
		&& (damageParticleSystems.empty()) ){
		for(UnitParticleSystemTypes::const_iterator it= type->damageParticleSystemTypes.begin(); it!=type->damageParticleSystemTypes.end(); ++it){
			UnitParticleSystem *ups;
			ups= new UnitParticleSystem(200);
			(*it)->setValues(ups);
			ups->setPos(getCurrVector());
			ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
			damageParticleSystems.push_back(ups);
			Renderer::getInstance().manageParticleSystem(ups, rsGame);
		}
	}
	// start fire
	if(type->getProperty(UnitType::pBurnable) && fire == NULL) {
		FireParticleSystem *fps = new FireParticleSystem(200);
		const Game *game = Renderer::getInstance().getGame();
		fps->setSpeed(2.5f / game->getWorld()->getUpdateFps(this->getFactionIndex()));
		fps->setPos(getCurrVector());
		fps->setRadius(type->getSize()/3.f);
		fps->setTexture(CoreData::getInstance().getFireTexture());
		fps->setParticleSize(type->getSize()/3.f);
		fire= fps;
		fireParticleSystems.push_back(fps);

		Renderer::getInstance().manageParticleSystem(fps, rsGame);
		if(showUnitParticles) {
			// smoke
			UnitParticleSystem *ups= new UnitParticleSystem(400);
			ups->setColorNoEnergy(Vec4f(0.0f, 0.0f, 0.0f, 0.13f));
			ups->setColor(Vec4f(0.115f, 0.115f, 0.115f, 0.22f));
			ups->setPos(getCurrVector());
			ups->setBlendMode(ups->strToBlendMode("black"));
			ups->setOffset(Vec3f(0,2,0));
			ups->setDirection(Vec3f(0,1,-0.2f));
			ups->setRadius(type->getSize()/3.f);
			ups->setTexture(CoreData::getInstance().getFireTexture());
			const Game *game = Renderer::getInstance().getGame();
			ups->setSpeed(2.0f / game->getWorld()->getUpdateFps(this->getFactionIndex()));
			ups->setGravity(0.0004f);
			ups->setEmissionRate(1);
			ups->setMaxParticleEnergy(150);
			ups->setSizeNoEnergy(type->getSize()*0.6f);
			ups->setParticleSize(type->getSize()*0.8f);
			damageParticleSystems.push_back(ups);
			Renderer::getInstance().manageParticleSystem(ups, rsGame);
		}

	}
}

void Unit::setTargetVec(const Vec3f &targetVec)	{
	this->targetVec= targetVec;
	logSynchData(string(__FILE__) + string("::") + string(__FUNCTION__) + string(" Line: ") + intToStr(__LINE__));
}

void Unit::setMeetingPos(const Vec2i &meetingPos) {
	this->meetingPos= meetingPos;
	logSynchData(string(__FILE__) + string("::") + string(__FUNCTION__) + string(" Line: ") + intToStr(__LINE__));
}

bool Unit::isMeetingPointSettable() const {
	return (type != NULL ? type->getMeetingPoint() : false);
}

int Unit::getFrameCount() {
	int frameCount = 0;
	const Game *game = Renderer::getInstance().getGame();
	if(game != NULL && game->getWorld() != NULL) {
		frameCount = game->getWorld()->getFrameCount();
	}

	return frameCount;
}

void Unit::exploreCells() {
	if(this->isOperative()) {
		const Vec2i &newPos = this->getCenteredPos();
		int sightRange = this->getType()->getSight();
		int teamIndex = this->getTeam();

		Chrono chrono;
		chrono.start();

		if(game == NULL) {
			throw runtime_error("game == NULL");
		}
		else if(game->getWorld() == NULL) {
			throw runtime_error("game->getWorld() == NULL");
		}

		game->getWorld()->exploreCells(newPos, sightRange, teamIndex);

		if(chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}
}

void Unit::logSynchData(string source) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {

	    char szBuf[1024]="";
	    sprintf(szBuf,
	    		//"Unit = %d [%s] [%s] pos = %s, lastPos = %s, targetPos = %s, targetVec = %s, meetingPos = %s, lastRotation [%f], targetRotation [%f], rotation [%f], progress [%f], progress2 [%f]\n",
	    		"FrameCount [%d] Unit = %d [%s] pos = %s, lastPos = %s, targetPos = %s, targetVec = %s, meetingPos = %s, lastRotation [%f], targetRotation [%f], rotation [%f], progress [%f], progress2 [%d]\n",
	    		getFrameCount(),
	    		id,
				getFullName().c_str(),
				//getDesc().c_str(),
				pos.getString().c_str(),
				lastPos.getString().c_str(),
				targetPos.getString().c_str(),
				targetVec.getString().c_str(),
				meetingPos.getString().c_str(),
				lastRotation,
				targetRotation,
				rotation,
				progress,
				progress2);

	    if(lastSynchDataString != string(szBuf)) {
	    	lastSynchDataString = string(szBuf);

			SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,
					"%s %s",
					source.c_str(),
					szBuf);
	    }
	}
}

void Unit::addBadHarvestPos(const Vec2i &value) {
	Chrono chron;
	chron.start();
	//badHarvestPosList.push_back(std::pair<Vec2i,Chrono>(value,chron));
	badHarvestPosList[value] = chron;
	cleanupOldBadHarvestPos();
}

void Unit::removeBadHarvestPos(const Vec2i &value) {
	std::map<Vec2i,Chrono>::iterator iter = badHarvestPosList.find(value);
	if(iter != badHarvestPosList.end()) {
		badHarvestPosList.erase(value);
	}
	cleanupOldBadHarvestPos();

/*
	for(int i = 0; i < badHarvestPosList.size(); ++i) {
		const std::pair<Vec2i,Chrono> &item = badHarvestPosList[i];
		if(item.first == value) {
			badHarvestPosList.erase(badHarvestPosList.begin() + i);
			break;
		}
	}
	cleanupOldBadHarvestPos();
*/

}

bool Unit::isBadHarvestPos(const Vec2i &value, bool checkPeerUnits) const {
	//cleanupOldBadHarvestPos();

	bool result = false;

	std::map<Vec2i,Chrono>::const_iterator iter = badHarvestPosList.find(value);
	if(iter != badHarvestPosList.end()) {
		result = true;
	}

/*
	for(int i = 0; i < badHarvestPosList.size(); ++i) {
		const std::pair<Vec2i,Chrono> &item = badHarvestPosList[i];
		if(item.first == value) {
			result = true;
			break;
		}
	}
*/
	if(result == false && checkPeerUnits == true) {
		// Check if any other units of similar type have this position tagged
		// as bad?
		for(int i = 0; i < this->getFaction()->getUnitCount(); ++i) {
			Unit *peerUnit = this->getFaction()->getUnit(i);
			if( peerUnit != NULL && peerUnit->getId() != this->getId() &&
				peerUnit->getType()->getSize() <= this->getType()->getSize()) {
				if(peerUnit->isBadHarvestPos(value,false) == true) {
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

void Unit::cleanupOldBadHarvestPos() {
	if(difftime(time(NULL),lastBadHarvestListPurge) >= 240) {
		lastBadHarvestListPurge = time(NULL);
		std::vector<Vec2i> purgeList;
		for(std::map<Vec2i,Chrono>::iterator iter = badHarvestPosList.begin(); iter != badHarvestPosList.end(); iter++) {
			if(iter->second.getMillis() >= 2400000) {
				purgeList.push_back(iter->first);
			}
		}
		for(int i = 0; i < purgeList.size(); ++i) {
			const Vec2i &item = purgeList[i];
			badHarvestPosList.erase(item);
		}
	}

/*
	for(int i = badHarvestPosList.size() - 1; i >= 0; --i) {
		const std::pair<Vec2i,Chrono> &item = badHarvestPosList[i];

		// If this position has been is the list for longer than 240
		// seconds remove it so the unit could potentially try it again
		if(item.second.getMillis() >= 2400000) {
			badHarvestPosList.erase(badHarvestPosList.begin() + i);
		}
	}
*/
}

void Unit::setLastHarvestResourceTarget(const Vec2i *pos) {
	if(pos == NULL) {
		lastHarvestResourceTarget.first = Vec2i(0);
		//lastHarvestResourceTarget.second = 0;
	}
	else {
		const Vec2i resourceLocation = *pos;
		if(resourceLocation != lastHarvestResourceTarget.first) {
			lastHarvestResourceTarget.first = resourceLocation;

			Chrono chron;
			chron.start();
			lastHarvestResourceTarget.second = chron;
		}
		else {
			// If we cannot harvest for > 10 seconds tag the position
			// as a bad one
			if(lastHarvestResourceTarget.second.getMillis() > 10000) {
				addBadHarvestPos(resourceLocation);
			}
		}
	}
}

void Unit::addCurrentTargetPathTakenCell(const Vec2i &target,const Vec2i &cell) {
	if(currentTargetPathTaken.first != target) {
		currentTargetPathTaken.second.clear();
	}
	currentTargetPathTaken.first = target;
	currentTargetPathTaken.second.push_back(cell);
}

std::string Unit::toString() const {
	std::string result = "";

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	result += "id = " + intToStr(this->id);
	if(this->type != NULL) {
		result += " name [" + this->type->getName() + "]";
	}

	if(this->faction != NULL) {
	    result += "\nFactionIndex = " + intToStr(this->faction->getIndex()) + "\n";
	    result += "teamIndex = " + intToStr(this->faction->getTeam()) + "\n";
	    result += "startLocationIndex = " + intToStr(this->faction->getStartLocationIndex()) + "\n";
	    result += "thisFaction = " + intToStr(this->faction->getThisFaction()) + "\n";
	    result += "control = " + intToStr(this->faction->getControlType()) + "\n";
	    if(this->faction->getType() != NULL) {
	    	result += "factionName = " + this->faction->getType()->getName() + "\n";
	    }
	}
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	result += " hp = " + intToStr(this->hp);
	result += " ep = " + intToStr(this->ep);
	result += " loadCount = " + intToStr(this->loadCount);
	result += " deadCount = " + intToStr(this->deadCount);
	result += " progress = " + floatToStr(this->progress);
	result += "\n";
	result += " lastAnimProgress = " + floatToStr(this->lastAnimProgress);
	result += " animProgress = " + floatToStr(this->animProgress);
	result += " highlight = " + floatToStr(this->highlight);
	result += " progress2 = " + intToStr(this->progress2);
	result += " kills = " + intToStr(this->kills);
	result += "\n";
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// WARNING!!! Don't access the Unit pointer in this->targetRef in this method or it causes
	// a stack overflow
	if(this->targetRef.getUnitId() >= 0) {
		//result += " targetRef = " + this->targetRef.getUnit()->toString();
		result += " targetRef = " + intToStr(this->targetRef.getUnitId()) + " - factionIndex = " + intToStr(this->targetRef.getUnitFaction()->getIndex());
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	result += " currField = " + intToStr(this->currField);
	result += " targetField = " + intToStr(this->targetField);
	if(level != NULL) {
		result += " level = " + level->getName();
	}
	result += "\n";
	result += " pos = " + pos.getString();
	result += " lastPos = " + lastPos.getString();
	result += "\n";
	result += " targetPos = " + targetPos.getString();
	result += " targetVec = " + targetVec.getString();
	result += " meetingPos = " + meetingPos.getString();
	result += "\n";

	result += " lastRotation = " + floatToStr(this->lastRotation);
	result += " targetRotation = " + floatToStr(this->targetRotation);
	result += " rotation = " + floatToStr(this->rotation);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(loadType != NULL) {
    	result += " loadType = " + loadType->getName();
    }

    if(currSkill != NULL) {
    	result += " currSkill = " + currSkill->getName();
    }
    result += "\n";

    result += " toBeUndertaken = " + intToStr(this->toBeUndertaken);
    result += " alive = " + intToStr(this->alive);
    result += " showUnitParticles = " + intToStr(this->showUnitParticles);

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    result += " totalUpgrade = " + totalUpgrade.toString();
    result += " " + this->unitPath->toString() + "\n";
    result += "\n";

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    result += "Command count = " + intToStr(commands.size()) + "\n";

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    int cmdIdx = 0;
    for(Commands::const_iterator iterList = commands.begin(); iterList != commands.end(); ++iterList) {
    	result += " index = " + intToStr(cmdIdx) + " ";
    	const Command *cmd = *iterList;
    	if(cmd != NULL) {
    		result += cmd->toString() + "\n";
    	}
    	cmdIdx++;
    }
    result += "\n";

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    result += "modelFacing = " + intToStr(modelFacing.asInt()) + "\n";

    result += "retryCurrCommandCount = " + intToStr(retryCurrCommandCount) + "\n";

    result += "screenPos = " + screenPos.getString() + "\n";

    result += "currentUnitTitle = " + currentUnitTitle + "\n";

    result += "inBailOutAttempt = " + intToStr(inBailOutAttempt) + "\n";

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return result;
}

}}//end namespace
