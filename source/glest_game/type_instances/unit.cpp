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
#include "sound_renderer.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;


namespace Glest{ namespace Game{

const int UnitPathBasic::maxBlockCount= GameConstants::updateFps / 2;

UnitPathBasic::UnitPathBasic() {
	this->blockCount = 0;
	this->pathQueue.clear();
	this->lastPathCacheQueue.clear();
	this->map = NULL;
}

UnitPathBasic::~UnitPathBasic() {
	this->blockCount = 0;
	this->pathQueue.clear();
	this->lastPathCacheQueue.clear();
	this->map = NULL;
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
	lastPathCacheQueue.clear();
	blockCount= 0;
}

void UnitPathBasic::incBlockCount() {
	pathQueue.clear();
	lastPathCacheQueue.clear();
	blockCount++;
}

void UnitPathBasic::add(const Vec2i &path) {
	if(this->map != NULL) {
		if(this->map->isInside(path) == false) {
			throw runtime_error("Invalid map path position = " + path.getString() + " map w x h = " + intToStr(map->getW()) + " " + intToStr(map->getH()));
		}
		else if(this->map->isInsideSurface(this->map->toSurfCoords(path)) == false) {
			throw runtime_error("Invalid map surface path position = " + path.getString() + " map surface w x h = " + intToStr(map->getSurfaceW()) + " " + intToStr(map->getSurfaceH()));
		}
	}

	pathQueue.push_back(path);
}

void UnitPathBasic::addToLastPathCache(const Vec2i &path) {
	if(this->map != NULL) {
		if(this->map->isInside(path) == false) {
			throw runtime_error("Invalid map path position = " + path.getString() + " map w x h = " + intToStr(map->getW()) + " " + intToStr(map->getH()));
		}
		else if(this->map->isInsideSurface(this->map->toSurfCoords(path)) == false) {
			throw runtime_error("Invalid map surface path position = " + path.getString() + " map surface w x h = " + intToStr(map->getSurfaceW()) + " " + intToStr(map->getSurfaceH()));
		}
	}

	lastPathCacheQueue.push_back(path);
}

Vec2i UnitPathBasic::pop(bool removeFrontPos) {
	if(pathQueue.size() <= 0) {
		throw runtime_error("pathQueue.size() = " + intToStr(pathQueue.size()));
	}
	Vec2i p= pathQueue.front();
	if(removeFrontPos == true) {
		pathQueue.erase(pathQueue.begin());
	}
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
	if(faction!=NULL){
		return faction->findUnit(id);
	}
	return NULL;
}

UnitAttackBoostEffect::UnitAttackBoostEffect() {
	boost = NULL;
	source = NULL;
	ups = NULL;
	upst = NULL;
}

UnitAttackBoostEffect::~UnitAttackBoostEffect() {
	if(ups != NULL) {
		//ups->fade();

		vector<UnitParticleSystem *> particleSystemToRemove;
		particleSystemToRemove.push_back(ups);

		Renderer::getInstance().cleanupUnitParticleSystems(particleSystemToRemove,rsGame);
		ups = NULL;
	}

	delete upst;
	upst = NULL;
}

UnitAttackBoostEffectOriginator::UnitAttackBoostEffectOriginator() {
	skillType = NULL;
	currentAppliedEffect = NULL;
}

UnitAttackBoostEffectOriginator::~UnitAttackBoostEffectOriginator() {
	delete currentAppliedEffect;
	currentAppliedEffect = NULL;
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
	modelFacing = CardinalDir::NORTH;
	lastStuckFrame = 0;
	lastStuckPos = Vec2i(0,0);
	lastPathfindFailedFrame = 0;
	lastPathfindFailedPos = Vec2i(0,0);
	usePathfinderExtendedMaxNodes = false;

    RandomGen random;

	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		throw runtime_error("#2 Invalid path position = " + pos.getString());
	}

    this->unitPath = unitpath;
    this->unitPath->setMap(map);
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
	this->ignoreCheckCommand = false;
	this->inBailOutAttempt = false;
	this->lastHarvestResourceTarget.first = Vec2i(0);
	//this->lastBadHarvestListPurge = 0;

	level= NULL;
	loadType= NULL;

    setModelFacing(placeFacing);

    Config &config= Config::getInstance();
	showUnitParticles				= config.getBool("UnitParticles","true");
	maxQueuedCommandDisplayCount 	= config.getInt("MaxQueuedCommandDisplayCount","15");

	lastPos= pos;
    progress= 0;
	lastAnimProgress= 0;
    animProgress= 0;
    progress2= 0;
	kills= 0;
	enemyKills = 0;
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
	this->lastModelIndexForCurrSkillType = -1;
	this->animationRandomCycleCount = 0;
	this->currSkill = getType()->getFirstStOfClass(scStop);
	this->currentAttackBoostOriginatorEffect.skillType = this->currSkill;

	livingUnits.insert(id);
	livingUnitsp.insert(this);

	addItemToVault(&this->hp,this->hp);
	addItemToVault(&this->ep,this->ep);

	logSynchData(__FILE__,__LINE__);
}

Unit::~Unit() {
	badHarvestPosList.clear();

	//Just to be sure, should already be removed
	if (livingUnits.erase(id)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//Only an error if not called at end
	}

	//Just to be sure, should already be removed
	if (livingUnitsp.erase(this)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	}
	//remove commands
	while(commands.empty() == false) {
		delete commands.back();
		commands.pop_back();
	}

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
	stopDamageParticles(true);

	while(currentAttackBoostEffects.empty() == false) {
		//UnitAttackBoostEffect &effect = currentAttackBoostEffects.back();
		currentAttackBoostEffects.pop_back();
	}

	delete currentAttackBoostOriginatorEffect.currentAppliedEffect;
	currentAttackBoostOriginatorEffect.currentAppliedEffect = NULL;

	delete this->unitPath;
	this->unitPath = NULL;

	Renderer &renderer= Renderer::getInstance();
	renderer.removeUnitFromQuadCache(this);
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

	switch(iut) {
	case iutIdleHarvester:
		if(type->hasCommandClass(ccHarvest)) {
			if(commands.empty() == false) {
				const CommandType *ct= commands.front()->getCommandType();
				if(ct != NULL){
					return ct->getClass() == ccStop;
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
			ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
			unitParticleSystems.push_back(ups);
			Renderer::getInstance().manageParticleSystem(ups, rsGame);
		}
	}
	progress2= 0;
	if(this->currSkill != currSkill) {
		this->lastModelIndexForCurrSkillType = -1;
		this->animationRandomCycleCount = 0;
	}
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

void Unit::setPos(const Vec2i &pos) {
	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		throw runtime_error("#3 Invalid path position = " + pos.getString());
	}

	this->lastPos= this->pos;
	this->pos= pos;
	map->clampPos(this->pos);
	this->meetingPos= pos - Vec2i(1);
	map->clampPos(this->meetingPos);

	// Attempt to improve performance
	this->exploreCells();

	logSynchData(__FILE__,__LINE__);
}

void Unit::setTargetPos(const Vec2i &targetPos) {

	if(map->isInside(targetPos) == false || map->isInsideSurface(map->toSurfCoords(targetPos)) == false) {
		throw runtime_error("#4 Invalid path position = " + targetPos.getString());
	}

	Vec2i relPos= targetPos - pos;
	//map->clampPos(relPos);

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
	map->clampPos(this->targetPos);

	logSynchData(__FILE__,__LINE__);
}

void Unit::setVisible(const bool visible) {
	this->visible = visible;

	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		(*it)->setVisible(visible);
	}
	for(UnitParticleSystems::iterator it= damageParticleSystems.begin(); it != damageParticleSystems.end(); ++it) {
		(*it)->setVisible(visible);
	}

	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		UnitAttackBoostEffect &effect = currentAttackBoostEffects[i];
		if(effect.ups != NULL) {
			effect.ups->setVisible(visible);
		}
	}
	if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
		if(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups != NULL) {
			currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setVisible(visible);
		}
	}
}

// =============================== Render related ==================================

Model *Unit::getCurrentModelPtr() {
	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	int currentModelIndexForCurrSkillType = lastModelIndexForCurrSkillType;
	Model *result = currSkill->getAnimation(animProgress,this,&lastModelIndexForCurrSkillType, &animationRandomCycleCount);
	if(currentModelIndexForCurrSkillType != lastModelIndexForCurrSkillType) {
		animationRandomCycleCount++;
	}
	return result;
}

const Model *Unit::getCurrentModel() {
	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	int currentModelIndexForCurrSkillType = lastModelIndexForCurrSkillType;
	const Model *result = currSkill->getAnimation(animProgress,this,&lastModelIndexForCurrSkillType, &animationRandomCycleCount);
	if(currentModelIndexForCurrSkillType != lastModelIndexForCurrSkillType) {
		animationRandomCycleCount++;
	}
	return result;
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
	return getVectorFlat(lastPos, pos);
/*
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
*/
}

Vec3f Unit::getVectorFlat(const Vec2i &lastPosValue, const Vec2i &curPosValue) const {
    Vec3f v;

	float y1= computeHeight(lastPosValue);
	float y2= computeHeight(curPosValue);

    if(currSkill->getClass() == scMove) {
        v.x= lastPosValue.x + progress * (curPosValue.x-lastPosValue.x);
        v.z= lastPosValue.y + progress * (curPosValue.y-lastPosValue.y);
		v.y= y1+progress*(y2-y1);
    }
    else{
        v.x= static_cast<float>(curPosValue.x);
        v.z= static_cast<float>(curPosValue.y);
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
Command *Unit::getCurrCommand() const {
	if(commands.empty() == false) {
		return commands.front();
	}
	return NULL;
}

void Unit::replaceCurrCommand(Command *cmd) {
	assert(commands.empty() == false);
	commands.front() = cmd;
	this->setCurrentUnitTitle("");
}

//returns the size of the commands
unsigned int Unit::getCommandSize() const {
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

//give one command (clear, and push back)
CommandResult Unit::giveCommand(Command *command, bool tryQueue) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"\n======================\nUnit Command tryQueue = %d\nUnit Info:\n%s\nCommand Info:\n%s\n",tryQueue,this->toString().c_str(),command->toString().c_str());

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    assert(command != NULL);
    assert(command->getCommandType() != NULL);

    const int command_priority = command->getPriority();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(command->getCommandType()->isQueuable(tryQueue)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Command is Queable\n",__FILE__,__FUNCTION__,__LINE__);

		if(command->getCommandType()->isQueuable() == qAlways && tryQueue){
			// Its a produce or upgrade command called without queued key
			// in this case we must NOT delete lower priority commands!
			// we just queue it!

		}
		else{
			//Delete all lower-prioirty commands
			for(list<Command*>::iterator i= commands.begin(); i != commands.end();){
				if((*i)->getPriority() < command_priority){
					if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled)
						SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Deleting lower priority command [%s]\n",__FILE__,__FUNCTION__,__LINE__,(*i)->toString().c_str());

					deleteQueuedCommand(*i);
					i= commands.erase(i);
				}
				else{
					++i;
				}
			}
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		//cancel current command if it is not queuable
		if(commands.empty() == false &&
          commands.back()->getCommandType()->isQueueAppendable() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Cancel command because last one is NOT queable [%s]\n",__FILE__,__FUNCTION__,__LINE__,commands.back()->toString().c_str());

		    cancelCommand();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}
	else {
		//empty command queue
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Clear commands because current is NOT queable.\n",__FILE__,__FUNCTION__,__LINE__);

		clearCommands();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//check command
	CommandResult result= checkCommand(command);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] checkCommand returned: [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);


	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(result == crSuccess) {
		applyCommand(command);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	//push back command
	if(result == crSuccess) {
		commands.push_back(command);
	}
	else {
		delete command;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	return result;
}

//pop front (used when order is done)
CommandResult Unit::finishCommand() {

	retryCurrCommandCount=0;
	this->setCurrentUnitTitle("");
	//is empty?
	if(commands.empty()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
		return crFailUndefined;
	}

	//pop front
	delete commands.front();
	commands.erase(commands.begin());
	this->unitPath->clear();

	while (commands.empty() == false) {
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
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
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

	faction->addStore(type);
	faction->applyStaticProduction(type);
	setCurrSkill(scStop);

	checkItemInVault(&this->hp,this->hp);
	hp= type->getMaxHp();
	addItemToVault(&this->hp,this->hp);
}

void Unit::kill() {
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

	UnitUpdater *unitUpdater = game->getWorld()->getUnitUpdater();
	//unitUpdater->clearUnitPrecache(this);
	unitUpdater->removeUnitPrecache(this);
}

void Unit::undertake() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to undertake unit id = %d [%s] [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->id, this->getFullName().c_str(),this->getDesc().c_str());

	// Remove any units that were previously in attack-boost range
	if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size() > 0) {
		for(unsigned int i = 0; i < currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size(); ++i) {
			// Remove attack boost upgrades from unit
			Unit *affectedUnit = currentAttackBoostOriginatorEffect.currentAttackBoostUnits[i];
			affectedUnit->deapplyAttackBoost(currentAttackBoostOriginatorEffect.skillType->getAttackBoost(), this);

			//printf("!!!! DE-APPLY ATTACK BOOST from unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
		}
		currentAttackBoostOriginatorEffect.currentAttackBoostUnits.clear();
	}

	UnitUpdater *unitUpdater = game->getWorld()->getUnitUpdater();
	//unitUpdater->clearUnitPrecache(this);
	unitUpdater->removeUnitPrecache(this);

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

	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		throw runtime_error("#6 Invalid path position = " + pos.getString());
	}

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


bool Unit::needToUpdate() {
	assert(progress <= 1.f);
	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	bool return_value = false;
	if(currSkill->getClass() != scDie) {
		//speed
		int speed = currSkill->getTotalSpeed(&totalUpgrade);

		//speed modifier
		float diagonalFactor= 1.f;
		float heightFactor= 1.f;
		if(currSkill->getClass() == scMove) {
			//if moving in diagonal move slower
			Vec2i dest= pos - lastPos;
			if(abs(dest.x) + abs(dest.y) == 2) {
				diagonalFactor = 0.71f;
			}

			//if moving to an higher cell move slower else move faster
			float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
			heightFactor= clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
		}

		//update progresses
		const Game *game = Renderer::getInstance().getGame();
		float speedDenominator = (speedDivider * game->getWorld()->getUpdateFps(this->getFactionIndex()));
		float newProgress = progress;
		newProgress += (speed * diagonalFactor * heightFactor) / speedDenominator;

		if(newProgress >= 1.f) {
			return_value = true;
		}
	}
	return return_value;
}

bool Unit::update() {
	assert(progress <= 1.f);

	//highlight
	if(highlight > 0.f) {
		const Game *game = Renderer::getInstance().getGame();
		highlight -= 1.f / (highlightTime * game->getWorld()->getUpdateFps(this->getFactionIndex()));
	}

	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//speed
	int speed= currSkill->getTotalSpeed(&totalUpgrade);

	//speed modifier
	float diagonalFactor = 1.f;
	float heightFactor   = 1.f;
	if(currSkill->getClass() == scMove) {
		//if moving in diagonal move slower
		Vec2i dest = pos - lastPos;
		if(abs(dest.x) + abs(dest.y) == 2) {
			diagonalFactor = 0.71f;
		}

		//if moving to an higher cell move slower else move faster
		float heightDiff = map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		heightFactor     = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
	}

	//update progresses
	lastAnimProgress= animProgress;
	const Game *game = Renderer::getInstance().getGame();

	float speedDenominator = (speedDivider * game->getWorld()->getUpdateFps(this->getFactionIndex()));
	progress += (speed * diagonalFactor * heightFactor) / speedDenominator;

	if(currSkill->getClass() == scBeBuilt && static_cast<const BeBuiltSkillType*>(currSkill)->getAnimHpBound()==true ){
		animProgress=this->getHpRatio();
	}
	else{
		animProgress += (currSkill->getAnimSpeed() * heightFactor) / speedDenominator;
	}
	//update target
	updateTarget();

	//rotation
	if(currSkill->getClass() != scStop) {
		const int rotFactor= 2;
		if(progress < 1.f / rotFactor) {
			if(type->getFirstStOfClass(scMove)){
				if(abs((int)(lastRotation-targetRotation)) < 180)
					rotation= lastRotation + (targetRotation - lastRotation) * progress * rotFactor;
				else {
					float rotationTerm = targetRotation > lastRotation ? -360.f: +360.f;
					rotation           = lastRotation + (targetRotation - lastRotation + rotationTerm) * progress * rotFactor;
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

	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		UnitAttackBoostEffect &effect = currentAttackBoostEffects[i];
		if(effect.ups != NULL) {
			effect.ups->setPos(getCurrVector());
			effect.ups->setRotation(getRotation());
		}
	}
	if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
		if(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups != NULL) {
			currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setPos(getCurrVector());
			currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setRotation(getRotation());
		}
	}

	//checks
	if(animProgress > 1.f) {
		bool canCycle = currSkill->CanCycleNextRandomAnimation(&animationRandomCycleCount);
		animProgress = currSkill->getClass() == scDie? 1.f: 0.f;
		if(canCycle == true) {
			this->lastModelIndexForCurrSkillType = -1;
		}
	}

    bool return_value = false;
	//checks
	if(progress >= 1.f) {
		lastRotation= targetRotation;
		if(currSkill->getClass() != scDie) {
			progress     = 0.f;
			return_value = true;
		}
		else {
			progress= 1.f;
			deadCount++;
			if(deadCount >= maxDeadCount) {
				toBeUndertaken= true;
				return_value = false;
			}
		}
	}

	if(currSkill != currentAttackBoostOriginatorEffect.skillType) {
		if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
			delete currentAttackBoostOriginatorEffect.currentAppliedEffect;
			currentAttackBoostOriginatorEffect.currentAppliedEffect = NULL;

			//printf("- #1 DE-APPLY ATTACK BOOST SELF PARTICLE to unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId());
		}

		// Remove any units that were previously in range
		if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size() > 0) {
			for(unsigned int i = 0; i < currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size(); ++i) {
				// Remove attack boost upgrades from unit
				Unit *affectedUnit = currentAttackBoostOriginatorEffect.currentAttackBoostUnits[i];
				affectedUnit->deapplyAttackBoost(currentAttackBoostOriginatorEffect.skillType->getAttackBoost(), this);

				//printf("- #1 DE-APPLY ATTACK BOOST from unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
			}
			currentAttackBoostOriginatorEffect.currentAttackBoostUnits.clear();
		}
		currentAttackBoostOriginatorEffect.skillType = currSkill;

		if(currSkill->isAttackBoostEnabled() == true) {
			// Search for units in range of this unit which apply to the
			// attack-boost and temporarily upgrade them
			UnitUpdater *unitUpdater = this->game->getWorld()->getUnitUpdater();

			const AttackBoost *attackBoost = currSkill->getAttackBoost();
			vector<Unit *> candidates = unitUpdater->findUnitsInRange(this, attackBoost->radius);
			for(unsigned int i = 0; i < candidates.size(); ++i) {
				Unit *affectedUnit = candidates[i];
				if(attackBoost->isAffected(this,affectedUnit) == true) {
					if(affectedUnit->applyAttackBoost(attackBoost, this) == true) {
						currentAttackBoostOriginatorEffect.currentAttackBoostUnits.push_back(affectedUnit);

						//printf("+ #1 APPLY ATTACK BOOST to unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
					}
				}
			}

			if(showUnitParticles == true) {
				if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size() > 0) {
					if(attackBoost->unitParticleSystemTypeForSourceUnit != NULL) {
						currentAttackBoostOriginatorEffect.currentAppliedEffect = new UnitAttackBoostEffect();
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = new UnitParticleSystemType();
						*currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = *attackBoost->unitParticleSystemTypeForSourceUnit;
						//effect.upst = boost->unitParticleSystemTypeForAffectedUnit;

						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups = new UnitParticleSystem(200);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst->setValues(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setPos(getCurrVector());
						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
						Renderer::getInstance().manageParticleSystem(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups, rsGame);

						//printf("+ #1 APPLY ATTACK BOOST SELF PARTICLE to unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId());
					}
				}
			}
		}
	}
	else {
		if(currSkill->isAttackBoostEnabled() == true) {
			// Search for units in range of this unit which apply to the
			// attack-boost and temporarily upgrade them
			UnitUpdater *unitUpdater = this->game->getWorld()->getUnitUpdater();

			const AttackBoost *attackBoost = currSkill->getAttackBoost();
			vector<Unit *> candidates = unitUpdater->findUnitsInRange(this, attackBoost->radius);
			for(unsigned int i = 0; i < candidates.size(); ++i) {
				Unit *affectedUnit = candidates[i];

				std::vector<Unit *>::iterator iterFound = std::find(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.begin(), currentAttackBoostOriginatorEffect.currentAttackBoostUnits.end(), affectedUnit);

				if(attackBoost->isAffected(this,affectedUnit) == true) {
					if(iterFound == currentAttackBoostOriginatorEffect.currentAttackBoostUnits.end()) {
						if(affectedUnit->applyAttackBoost(attackBoost, this) == true) {
							currentAttackBoostOriginatorEffect.currentAttackBoostUnits.push_back(affectedUnit);

							//printf("+ #2 APPLY ATTACK BOOST to unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
						}
					}
				}
				else {
					if(iterFound != currentAttackBoostOriginatorEffect.currentAttackBoostUnits.end()) {
						affectedUnit->deapplyAttackBoost(currentAttackBoostOriginatorEffect.skillType->getAttackBoost(), this);
						currentAttackBoostOriginatorEffect.currentAttackBoostUnits.erase(iterFound);

						//printf("- #2 DE-APPLY ATTACK BOOST from unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
					}
				}
			}

			if(showUnitParticles == true) {
				if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size() > 0) {
					if( attackBoost->unitParticleSystemTypeForSourceUnit != NULL &&
						currentAttackBoostOriginatorEffect.currentAppliedEffect == NULL) {

						currentAttackBoostOriginatorEffect.currentAppliedEffect = new UnitAttackBoostEffect();
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = new UnitParticleSystemType();
						*currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = *attackBoost->unitParticleSystemTypeForSourceUnit;
						//effect.upst = boost->unitParticleSystemTypeForAffectedUnit;

						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups = new UnitParticleSystem(200);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst->setValues(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setPos(getCurrVector());
						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
						Renderer::getInstance().manageParticleSystem(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups, rsGame);

						//printf("+ #2 APPLY ATTACK BOOST SELF PARTICLE to unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId());
					}
				}
				else if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size() <= 0) {
					if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
						delete currentAttackBoostOriginatorEffect.currentAppliedEffect;
						currentAttackBoostOriginatorEffect.currentAppliedEffect = NULL;

						//printf("- #2 DE-APPLY ATTACK BOOST SELF PARTICLE to unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId());
					}
				}
			}
		}
	}

	return return_value;
}

bool Unit::unitHasAttackBoost(const AttackBoost *boost, const Unit *source) const {
	bool result = false;
	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		const UnitAttackBoostEffect &effect = currentAttackBoostEffects[i];
		if( effect.boost == boost &&
			effect.source->getType()->getId() == source->getType()->getId()) {
			result = true;
			break;
		}
	}
	return result;
}

bool Unit::applyAttackBoost(const AttackBoost *boost, const Unit *source) {
	if(boost == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: boost == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//printf("APPLYING ATTACK BOOST to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());

	bool shouldApplyAttackBoost = true;
	if(boost->allowMultipleBoosts == false) {
		// Check if we already have this boost from this unit type and multiples
		// are not allowed
		bool alreadyHasAttackBoost = this->unitHasAttackBoost(boost, source);
		if(alreadyHasAttackBoost == true) {
			shouldApplyAttackBoost = false;
		}
	}

	if(shouldApplyAttackBoost == true) {
		currentAttackBoostEffects.push_back(UnitAttackBoostEffect());
		UnitAttackBoostEffect &effect = currentAttackBoostEffects[currentAttackBoostEffects.size()-1];
		effect.boost = boost;
		effect.source = source;

		bool wasAlive = alive;
		int prevMaxHp = totalUpgrade.getMaxHp();
		//printf("#1 wasAlive = %d hp = %d boosthp = %d\n",wasAlive,hp,boost->boostUpgrade.getMaxHp());

		totalUpgrade.apply(&boost->boostUpgrade, this);

		checkItemInVault(&this->hp,this->hp);
		//hp += boost->boostUpgrade.getMaxHp();
		hp += (totalUpgrade.getMaxHp() - prevMaxHp);
		addItemToVault(&this->hp,this->hp);

		//printf("#2 wasAlive = %d hp = %d boosthp = %d\n",wasAlive,hp,boost->boostUpgrade.getMaxHp());

		if(wasAlive == true) {
			//startDamageParticles
			startDamageParticles();

			//stop DamageParticles on death
			if(hp <= 0) {
				alive= false;
				hp=0;
				addItemToVault(&this->hp,this->hp);

				stopDamageParticles(true);

				Unit::game->getWorld()->getStats()->die(getFactionIndex());
				game->getScriptManager()->onUnitDied(this);

				StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
				if(sound != NULL &&
					(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
					 (game->getWorld()->getThisTeamIndex() == GameConstants::maxPlayers -1 + fpt_Observer))) {
					SoundRenderer::getInstance().playFx(sound);
				}

				if(this->isDead() && this->getCurrSkill()->getClass() != scDie) {
					this->kill();
				}
			}
		}

		if(showUnitParticles == true) {
			effect.upst = new UnitParticleSystemType();
			*effect.upst = *boost->unitParticleSystemTypeForAffectedUnit;
			//effect.upst = boost->unitParticleSystemTypeForAffectedUnit;

			effect.ups = new UnitParticleSystem(200);
			effect.upst->setValues(effect.ups);
			effect.ups->setPos(getCurrVector());
			effect.ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
			Renderer::getInstance().manageParticleSystem(effect.ups, rsGame);
		}
		//currentAttackBoostEffects.push_back(effect);
	}
	return shouldApplyAttackBoost;
}

void Unit::deapplyAttackBoost(const AttackBoost *boost, const Unit *source) {
	if(boost == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: boost == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//printf("DE-APPLYING ATTACK BOOST to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());

	bool wasAlive = alive;
	int prevMaxHp = totalUpgrade.getMaxHp();
	totalUpgrade.deapply(&boost->boostUpgrade, this);

	checkItemInVault(&this->hp,this->hp);
	//hp -= boost->boostUpgrade.getMaxHp();
	hp -= (prevMaxHp - totalUpgrade.getMaxHp());
	addItemToVault(&this->hp,this->hp);

	if(wasAlive == true) {
		//startDamageParticles
		startDamageParticles();

		//stop DamageParticles on death
		if(hp <= 0) {
			alive= false;
			hp=0;
			addItemToVault(&this->hp,this->hp);

			stopDamageParticles(true);

			Unit::game->getWorld()->getStats()->die(getFactionIndex());
			game->getScriptManager()->onUnitDied(this);

			StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
			if(sound != NULL &&
				(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
				 (game->getWorld()->getThisTeamIndex() == GameConstants::maxPlayers -1 + fpt_Observer))) {
				SoundRenderer::getInstance().playFx(sound);
			}

			if(this->isDead() && this->getCurrSkill()->getClass() != scDie) {
				this->kill();
			}
		}
	}


	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		UnitAttackBoostEffect &effect = currentAttackBoostEffects[i];
		if(effect.boost == boost && effect.source == source) {
			currentAttackBoostEffects.erase(currentAttackBoostEffects.begin() + i);
			break;
		}
	}
}

void Unit::tick() {

	if(isAlive()) {
		if(type == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

		//regenerate hp
		if(type->getHpRegeneration() >= 0) {
			checkItemInVault(&this->hp,this->hp);
            hp += type->getHpRegeneration();
            if(hp > type->getTotalMaxHp(&totalUpgrade)) {
                hp = type->getTotalMaxHp(&totalUpgrade);
            }
        	addItemToVault(&this->hp,this->hp);
		}
		// If we have negative regeneration then check if the unit should die
		else {
            bool decHpResult = decHp(-type->getHpRegeneration());
            if(decHpResult) {
                Unit::game->getWorld()->getStats()->die(getFactionIndex());
                game->getScriptManager()->onUnitDied(this);
            }
            StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
            if(sound != NULL &&
            	(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
            	 (game->getWorld()->getThisTeamIndex() == GameConstants::maxPlayers -1 + fpt_Observer))) {
                SoundRenderer::getInstance().playFx(sound);
            }
		}

		//stop DamageParticles
		stopDamageParticles(false);

		checkItemInVault(&this->ep,this->ep);
		//regenerate ep
		ep += type->getEpRegeneration();
		if(ep > type->getTotalMaxEp(&totalUpgrade)){
			ep = type->getTotalMaxEp(&totalUpgrade);
		}
		addItemToVault(&this->ep,this->ep);
	}
}

int Unit::update2() {
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
    if(ep - currSkill->getEpCost() < 0) {
        return true;
    }

	checkItemInVault(&this->ep,this->ep);
	//decrease ep
    ep -= currSkill->getEpCost();
	addItemToVault(&this->ep,this->ep);

	if(getType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: getType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(ep > getType()->getTotalMaxEp(&totalUpgrade)){
        ep = getType()->getTotalMaxEp(&totalUpgrade);
	}
	addItemToVault(&this->ep,this->ep);

    return false;
}
bool Unit::computeHp() {

	if(currSkill == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	if(!isBeingBuilt()){
			//cost hp
		if(currSkill->getHpCost() > 0) {
			bool decHpResult = decHp(currSkill->getHpCost());
            if(decHpResult) {
                Unit::game->getWorld()->getStats()->die(getFactionIndex());
                game->getScriptManager()->onUnitDied(this);
            }
		}
		// If we have negative costs then add life
		else {
			checkItemInVault(&this->hp,this->hp);
            hp += -currSkill->getHpCost();
            if(hp > type->getTotalMaxHp(&totalUpgrade)) {
                hp = type->getTotalMaxHp(&totalUpgrade);
            }
        	addItemToVault(&this->hp,this->hp);
            
		}
	}
	

    return true;
}

bool Unit::repair(){

	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//increase hp
	checkItemInVault(&this->hp,this->hp);
	hp += getType()->getMaxHp()/type->getProductionTime() + 1;
    if(hp > (getType()->getTotalMaxHp(&totalUpgrade))) {
        hp = getType()->getTotalMaxHp(&totalUpgrade);
    	addItemToVault(&this->hp,this->hp);
        return true;
    }
	addItemToVault(&this->hp,this->hp);

	//stop DamageParticles
	stopDamageParticles(false);

    return false;
}

//decrements HP and returns if dead
bool Unit::decHp(int i) {
	if(hp == 0) {
		return false;
	}

	checkItemInVault(&this->hp,this->hp);
	hp -= i;
	addItemToVault(&this->hp,this->hp);

	if(type == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//startDamageParticles
	startDamageParticles();

	//stop DamageParticles on death
    if(hp <= 0) {
		alive= false;
        hp=0;
    	addItemToVault(&this->hp,this->hp);

		stopDamageParticles(true);
		return true;
    }
    return false;
}

string Unit::getDescExtension() const{
	Lang &lang= Lang::getInstance();
	string str= "\n";

	if(commands.empty() == false && commands.size() > 1 ){
		Commands::const_iterator it= commands.begin();
		for(unsigned int i= 0; i < min((size_t) maxQueuedCommandDisplayCount, commands.size()); ++i){
			const CommandType *ct= (*it)->getCommandType();
			if(i == 0){
				str+= "\n" + lang.get("OrdersOnQueue") + ": ";
			}
			str+= "\n#" + intToStr(i + 1) + " " + ct->getName();
			it++;
		}
	}

	return str;
}

string Unit::getDesc() const {

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
	if(enemyKills > 0 || nextLevel != NULL) {
		str+= "\n" + lang.get("Kills") +": " + intToStr(enemyKills);
		if(nextLevel != NULL) {
			str+= " (" + nextLevel->getName() + ": " + intToStr(nextLevel->getKills()) + ")";
		}
	}

	//str+= "\nskl: "+scToStr(currSkill->getClass());

	//load
	if(loadCount!=0){
		str+= "\n" + lang.get("Load")+ ": " + intToStr(loadCount) +"  " + loadType->getName();
	}

	//consumable production
	for(int i=0; i < getType()->getCostCount(); ++i) {
		const Resource *r= getType()->getCost(i);
		if(r->getType()->getClass() == rcConsumable) {
			str+= "\n";
			str+= r->getAmount() < 0 ? lang.get("Produce")+": ": lang.get("Consume")+": ";
			str+= intToStr(abs(r->getAmount())) + " " + r->getType()->getName();
		}
	}

	//command info
    if(commands.empty() == false) {
		str+= "\n" + commands.front()->getCommandType()->getName();
		if(commands.size() > 1) {
			str+= "\n" + lang.get("OrdersOnQueue") + ": " + intToStr(commands.size());
		}
	}
	else{
		//can store
		if(getType()->getStoredResourceCount() > 0) {
			for(int i = 0; i < getType()->getStoredResourceCount(); ++i) {
				const Resource *r= getType()->getStoredResource(i);
				str+= "\n" + lang.get("Store") + ": ";
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
		totalUpgrade.sum(upgradeType, this);

		checkItemInVault(&this->hp,this->hp);
		hp += upgradeType->getMaxHp();
		hp = max(0,hp);
		addItemToVault(&this->hp,this->hp);
	}
}

void Unit::computeTotalUpgrade(){
	faction->getUpgradeManager()->computeTotalUpgrade(this, &totalUpgrade);
}

void Unit::incKills(int team) {
	++kills;
	if(team != this->getTeam()) {
		++enemyKills;
	}

	const Level *nextLevel= getNextLevel();
	if(nextLevel != NULL && enemyKills >= nextLevel->getKills()) {
		level= nextLevel;
		int maxHp= totalUpgrade.getMaxHp();
		totalUpgrade.incLevel(type);

		checkItemInVault(&this->hp,this->hp);
		hp += totalUpgrade.getMaxHp() - maxHp;
		addItemToVault(&this->hp,this->hp);
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
    if(map->isFreeCellsOrHasUnit(pos, morphUnitType->getSize(), morphUnitField, this,morphUnitType)){
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);

		checkItemInVault(&this->hp,this->hp);
		hp += morphUnitType->getMaxHp() - type->getMaxHp();
		addItemToVault(&this->hp,this->hp);

		type= morphUnitType;
		level= NULL;
		currField=morphUnitField;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(morphUnitType, mct->getDiscount());
		faction->addStore(type);
		faction->applyStaticProduction(morphUnitType);
		return true;
	}
	else{
		return false;
	}
}


// ==================== PRIVATE ====================

float Unit::computeHeight(const Vec2i &pos) const{
	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		throw runtime_error("#7 Invalid path position = " + pos.getString());
	}

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
	this->unitPath->clear();
	while(commands.empty() == false) {
		undoCommand(commands.back());
		delete commands.back();
		commands.pop_back();
	}
}

void Unit::deleteQueuedCommand(Command *command) {
	if(getCurrCommand() == command)	{
		this->setCurrentUnitTitle("");
		this->unitPath->clear();
	}
	undoCommand(command);
	delete command;
}


CommandResult Unit::checkCommand(Command *command) const {
	if(command == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	//if not operative or has not command type => fail
	if(isOperative() == false ||
       command->getUnit() == this ||
       getType()->hasCommandType(command->getCommandType()) == false ||
       (ignoreCheckCommand == false && this->getFaction()->reqsOk(command->getCommandType()) == false)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] isOperative() = %d, command->getUnit() = %p, getType()->hasCommandType(command->getCommandType()) = %d, this->getFaction()->reqsOk(command->getCommandType()) = %d\n",__FILE__,__FUNCTION__, __LINE__,isOperative(),command->getUnit(),getType()->hasCommandType(command->getCommandType()),this->getFaction()->reqsOk(command->getCommandType()));

		// Allow self healing if able to heal own unit type
		if(	command->getUnit() == this &&
			command->getCommandType()->getClass() == ccRepair &&
			this->getType()->getFirstRepairCommand(this->getType()) != NULL) {

		}
		else {
			return crFailUndefined;
		}
	}

	//if pos is not inside the world (if comand has not a pos, pos is (0, 0) and is inside world
	if(map->isInside(command->getPos()) == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
        return crFailUndefined;
	}

	//check produced
	if(command->getCommandType() == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
		throw runtime_error(szBuf);
	}

	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL) {
		if(ignoreCheckCommand == false && faction->reqsOk(produced) == false) {
            return crFailReqs;
		}

		if(ignoreCheckCommand == false && faction->checkCosts(produced) == false) {
			return crFailRes;
		}
	}

    //build command specific, check resources and requirements for building
    if(command->getCommandType()->getClass() == ccBuild) {
		const UnitType *builtUnit= command->getUnitType();

		if(builtUnit == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: builtUnit == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

		if(faction->reqsOk(builtUnit) == false) {
            return crFailReqs;
		}
		if(faction->checkCosts(builtUnit) == false) {
			return crFailRes;
		}
    }
    //upgrade command specific, check that upgrade is not upgraded
    else if(command->getCommandType()->getClass() == ccUpgrade) {
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

		if(uct == NULL) {
			char szBuf[4096]="";
			sprintf(szBuf,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->toString().c_str());
			throw runtime_error(szBuf);
		}

		if(faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())){
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
            return crFailUndefined;
		}
	}

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
	if(produced!=NULL) {
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

void Unit::stopDamageParticles(bool force) {
	if(force == true || (hp > type->getTotalMaxHp(&totalUpgrade) / 2) ) {
		//printf("Checking to stop damageparticles for unit [%s - %d] hp = %d\n",this->getType()->getName().c_str(),this->getId(),hp);

		if(Renderer::getInstance().validateParticleSystemStillExists(fire,rsGame) == false) {
			fire = NULL;
		}

		// stop fire
		if(fire != NULL) {
			fire->fade();
			fire = NULL;
		}
		// stop additional particles

		if(damageParticleSystems.size() > 0) {
			for(int i = damageParticleSystems.size()-1; i >= 0; --i) {
				UnitParticleSystem *ps = damageParticleSystems[i];
				UnitParticleSystemType *pst = NULL;
				int foundParticleIndexType = -2;
				for(std::map<int, UnitParticleSystem *>::iterator iterMap = damageParticleSystemsInUse.begin();
					iterMap != damageParticleSystemsInUse.end(); ++iterMap) {
					if(iterMap->second == ps) {
						foundParticleIndexType = iterMap->first;
						if(foundParticleIndexType >= 0) {
							pst = type->damageParticleSystemTypes[foundParticleIndexType];
							break;
						}
					}
				}
				if(force == true || ( pst !=NULL && pst->getMinmaxEnabled() == false )) {
					damageParticleSystemsInUse.erase(foundParticleIndexType);
					ps->fade();
					damageParticleSystems.pop_back();
				}
			}
		}
	}

	checkCustomizedParticleTriggers(force);
}

void Unit::checkCustomizedParticleTriggers(bool force) {
	// Now check if we have special hp triggered particles
	if(damageParticleSystems.size() > 0) {
		for(int i = damageParticleSystems.size()-1; i >= 0; --i) {
			UnitParticleSystem *ps = damageParticleSystems[i];
			UnitParticleSystemType *pst = NULL;
			int foundParticleIndexType = -2;
			for(std::map<int, UnitParticleSystem *>::iterator iterMap = damageParticleSystemsInUse.begin();
				iterMap != damageParticleSystemsInUse.end(); ++iterMap) {
				if(iterMap->second == ps) {
					foundParticleIndexType = iterMap->first;
					if(foundParticleIndexType >= 0) {
						pst = type->damageParticleSystemTypes[foundParticleIndexType];
						break;
					}
				}
			}

			if(force == true || (pst != NULL && pst->getMinmaxEnabled() == true)) {
				bool stopParticle = force;
				if(force == false) {
					if(pst->getMinmaxIsPercent() == false) {
						if(hp < pst->getMinHp() || hp > pst->getMaxHp()) {
							stopParticle = true;
						}
					}
					else {
						int hpPercent = (hp / type->getTotalMaxHp(&totalUpgrade) * 100);
						if(hpPercent < pst->getMinHp() || hpPercent > pst->getMaxHp()) {
							stopParticle = true;
						}
					}
				}

				//printf("CHECKING to STOP customized particle trigger by HP [%d to %d percentbased = %d] current hp = %d stopParticle = %d\n",pst->getMinHp(),pst->getMaxHp(),pst->getMinmaxIsPercent(),hp,stopParticle);

				if(stopParticle == true) {
					//printf("STOPPING customized particle trigger by HP [%d to %d] current hp = %d\n",pst->getMinHp(),pst->getMaxHp(),hp);

					damageParticleSystemsInUse.erase(foundParticleIndexType);
					ps->fade();
					damageParticleSystems.pop_back();
				}
			}
		}
	}

	// Now check if we have special hp triggered particles
	//start additional particles
	if(showUnitParticles && (type->damageParticleSystemTypes.empty() == false)  &&
		force == false && alive == true) {
		for(unsigned int i = 0; i < type->damageParticleSystemTypes.size(); ++i) {
			UnitParticleSystemType *pst = type->damageParticleSystemTypes[i];

			if(pst->getMinmaxEnabled() == true && damageParticleSystemsInUse.find(i) == damageParticleSystemsInUse.end()) {
				bool showParticle = false;
				if(pst->getMinmaxIsPercent() == false) {
					if(hp >= pst->getMinHp() && hp <= pst->getMaxHp()) {
						showParticle = true;
					}
				}
				else {
					int hpPercent = (hp / type->getTotalMaxHp(&totalUpgrade) * 100);
					if(hpPercent >= pst->getMinHp() && hpPercent <= pst->getMaxHp()) {
						showParticle = true;
					}
				}

				//printf("CHECKING to START customized particle trigger by HP [%d to %d percentbased = %d] current hp = %d showParticle = %d\n",pst->getMinHp(),pst->getMaxHp(),pst->getMinmaxIsPercent(),hp,showParticle);

				if(showParticle == true) {
					//printf("STARTING customized particle trigger by HP [%d to %d] current hp = %d\n",pst->getMinHp(),pst->getMaxHp(),hp);

					UnitParticleSystem *ups = new UnitParticleSystem(200);
					pst->setValues(ups);
					ups->setPos(getCurrVector());
					ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
					damageParticleSystems.push_back(ups);
					damageParticleSystemsInUse[i] = ups;
					Renderer::getInstance().manageParticleSystem(ups, rsGame);
				}
			}
		}
	}
}

void Unit::startDamageParticles() {
	if(hp < type->getMaxHp() / 2 && hp > 0 && alive == true) {
		//start additional particles
		if( showUnitParticles && (!type->damageParticleSystemTypes.empty()) ) {
			for(unsigned int i = 0; i < type->damageParticleSystemTypes.size(); ++i) {
				UnitParticleSystemType *pst = type->damageParticleSystemTypes[i];

				if(pst->getMinmaxEnabled() == false && damageParticleSystemsInUse.find(i) == damageParticleSystemsInUse.end()) {
					UnitParticleSystem *ups = new UnitParticleSystem(200);
					pst->setValues(ups);
					ups->setPos(getCurrVector());
					ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
					damageParticleSystems.push_back(ups);
					damageParticleSystemsInUse[i] = ups;
					Renderer::getInstance().manageParticleSystem(ups, rsGame);
				}
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
				damageParticleSystemsInUse[-1] = ups;
				Renderer::getInstance().manageParticleSystem(ups, rsGame);
			}
		}
	}

	checkCustomizedParticleTriggers(false);
}

void Unit::setTargetVec(const Vec3f &targetVec)	{
	this->targetVec= targetVec;
	logSynchData(__FILE__,__LINE__);
}

void Unit::setMeetingPos(const Vec2i &meetingPos) {
	this->meetingPos= meetingPos;
	map->clampPos(this->meetingPos);

	if(map->isInside(this->meetingPos) == false || map->isInsideSurface(map->toSurfCoords(this->meetingPos)) == false) {
		throw runtime_error("#8 Invalid path position = " + this->meetingPos.getString());
	}

	logSynchData(__FILE__,__LINE__);
}

bool Unit::isMeetingPointSettable() const {
	return (type != NULL ? type->getMeetingPoint() : false);
}

int Unit::getFrameCount() const {
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

		if(game == NULL) {
			throw runtime_error("game == NULL");
		}
		else if(game->getWorld() == NULL) {
			throw runtime_error("game->getWorld() == NULL");
		}

		game->getWorld()->exploreCells(newPos, sightRange, teamIndex);
	}
}

void Unit::logSynchData(string file,int line,string source) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {

	    char szBuf[4096]="";

	    sprintf(szBuf,
	    		"FrameCount [%d] Unit = %d [%s][%s] pos = %s, lastPos = %s, targetPos = %s, targetVec = %s, meetingPos = %s, progress [%f], progress2 [%d]\nUnit Path [%s]\n",
	    		getFrameCount(),
	    		id,
				getFullName().c_str(),
				faction->getType()->getName().c_str(),
				//getDesc().c_str(),
				pos.getString().c_str(),
				lastPos.getString().c_str(),
				targetPos.getString().c_str(),
				targetVec.getString().c_str(),
				meetingPos.getString().c_str(),
//				lastRotation,
//				targetRotation,
//				rotation,
				progress,
				progress2,
				(unitPath != NULL ? unitPath->toString().c_str() : "NULL"));

/*
	    sprintf(szBuf,
	    		"FrameCount [%d] Unit = %d [%s][%s] pos = %s, lastPos = %s, targetPos = %s, targetVec = %s, meetingPos = %s, lastRotation [%f], targetRotation [%f], rotation [%f], progress [%f], progress2 [%d]\nUnit Path [%s]\n",
	    		getFrameCount(),
	    		id,
				getFullName().c_str(),
				faction->getType()->getName().c_str(),
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
				progress2,
				(unitPath != NULL ? unitPath->toString().c_str() : "NULL"));
*/
	    if( lastSynchDataString != string(szBuf) ||
	    	lastFile != file ||
	    	lastLine != line ||
	    	lastSource != source) {
	    	lastSynchDataString = string(szBuf);
	    	lastFile = file;
	    	lastSource = source;

	    	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
	    	SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"[%s::%d]\n",extractFileFromDirectoryPath(file).c_str(),line);
			if(source != "") {
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s ",source.c_str());
			}
			SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s\n",szBuf);
			SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
	    }
	}
}

void Unit::addBadHarvestPos(const Vec2i &value) {
	//Chrono chron;
	//chron.start();
	badHarvestPosList[value] = getFrameCount();
	cleanupOldBadHarvestPos();
}

void Unit::removeBadHarvestPos(const Vec2i &value) {
	std::map<Vec2i,int>::iterator iter = badHarvestPosList.find(value);
	if(iter != badHarvestPosList.end()) {
		badHarvestPosList.erase(value);
	}
	cleanupOldBadHarvestPos();
}

bool Unit::isBadHarvestPos(const Vec2i &value, bool checkPeerUnits) const {
	bool result = false;

	std::map<Vec2i,int>::const_iterator iter = badHarvestPosList.find(value);
	if(iter != badHarvestPosList.end()) {
		result = true;
	}
	else if(checkPeerUnits == true) {
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
	const int cleanupInterval = (GameConstants::updateFps * 5);
	bool needToCleanup = (getFrameCount() % cleanupInterval == 0);

	//printf("========================> cleanupOldBadHarvestPos() [%d] badHarvestPosList.size [%ld] cleanupInterval [%d] getFrameCount() [%d] needToCleanup [%d]\n",getFrameCount(),badHarvestPosList.size(),cleanupInterval,getFrameCount(),needToCleanup);

	if(needToCleanup == true) {
		//printf("========================> cleanupOldBadHarvestPos() [%d] badHarvestPosList.size [%ld]\n",getFrameCount(),badHarvestPosList.size());

		std::vector<Vec2i> purgeList;
		for(std::map<Vec2i,int>::iterator iter = badHarvestPosList.begin(); iter != badHarvestPosList.end(); iter++) {
			if(getFrameCount() - iter->second >= cleanupInterval) {
				//printf("cleanupOldBadHarvestPos() [%d][%d]\n",getFrameCount(),iter->second);
				purgeList.push_back(iter->first);
			}
		}

		if(purgeList.size() > 0) {
			char szBuf[4096]="";
			sprintf(szBuf,"[cleaning old bad harvest targets] purgeList.size() [%ld]",purgeList.size());
			logSynchData(__FILE__,__LINE__,szBuf);

			for(int i = 0; i < purgeList.size(); ++i) {
				const Vec2i &item = purgeList[i];
				badHarvestPosList.erase(item);
			}
		}
	}
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

			//Chrono chron;
			//chron.start();
			lastHarvestResourceTarget.second = getFrameCount();
		}
		else {
			// If we cannot harvest for > 10 seconds tag the position
			// as a bad one
			const int addInterval = (GameConstants::updateFps * 5);
			if(lastHarvestResourceTarget.second - getFrameCount() >= addInterval) {
				//printf("-----------------------> setLastHarvestResourceTarget() [%d][%d]\n",getFrameCount(),lastHarvestResourceTarget.second);
				addBadHarvestPos(resourceLocation);
			}
		}
	}
}

//void Unit::addCurrentTargetPathTakenCell(const Vec2i &target,const Vec2i &cell) {
//	if(currentTargetPathTaken.first != target) {
//		currentTargetPathTaken.second.clear();
//	}
//	currentTargetPathTaken.first = target;
//	currentTargetPathTaken.second.push_back(cell);
//}

void Unit::setLastPathfindFailedFrameToCurrentFrame() {
	lastPathfindFailedFrame = getFrameCount();
}

bool Unit::isLastPathfindFailedFrameWithinCurrentFrameTolerance() const {
	static const bool enablePathfinderEnlargeMaxNodes = Config::getInstance().getBool("EnablePathfinderEnlargeMaxNodes","false");
	bool result = enablePathfinderEnlargeMaxNodes;
	if(enablePathfinderEnlargeMaxNodes) {
		const int MIN_FRAME_ELAPSED_RETRY = 960;
		result = (getFrameCount() - lastPathfindFailedFrame >= MIN_FRAME_ELAPSED_RETRY);
	}
	return result;
}

void Unit::setLastStuckFrameToCurrentFrame() {
	lastStuckFrame = getFrameCount();
}

bool Unit::isLastStuckFrameWithinCurrentFrameTolerance() const {
	const int MIN_FRAME_ELAPSED_RETRY = 300;
	bool result (getFrameCount() - lastStuckFrame <= MIN_FRAME_ELAPSED_RETRY);
	return result;
}

Vec2i Unit::getPosWithCellMapSet() const {
	Vec2i cellMapPos = this->getType()->getFirstOccupiedCellInCellMap(pos);
	return cellMapPos;
}

std::string Unit::toString() const {
	std::string result = "";

	result += "id = " + intToStr(this->id);
	if(this->type != NULL) {
		result += " name [" + this->type->getName() + "][" + intToStr(this->type->getId()) + "]";
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
	result += " enemyKills = " + intToStr(this->enemyKills);
	result += "\n";

	// WARNING!!! Don't access the Unit pointer in this->targetRef in this method or it causes
	// a stack overflow
	if(this->targetRef.getUnitId() >= 0) {
		//result += " targetRef = " + this->targetRef.getUnit()->toString();
		result += " targetRef = " + intToStr(this->targetRef.getUnitId()) + " - factionIndex = " + intToStr(this->targetRef.getUnitFaction()->getIndex());
	}

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

    result += " totalUpgrade = " + totalUpgrade.toString();
    result += " " + this->unitPath->toString() + "\n";
    result += "\n";

    result += "Command count = " + intToStr(commands.size()) + "\n";

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

    result += "modelFacing = " + intToStr(modelFacing.asInt()) + "\n";

    result += "retryCurrCommandCount = " + intToStr(retryCurrCommandCount) + "\n";

    result += "screenPos = " + screenPos.getString() + "\n";

    result += "currentUnitTitle = " + currentUnitTitle + "\n";

    result += "inBailOutAttempt = " + intToStr(inBailOutAttempt) + "\n";

	return result;
}

}}//end namespace
