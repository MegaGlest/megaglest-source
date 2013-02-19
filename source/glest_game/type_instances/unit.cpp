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

#define NOMINMAX

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

const float CHANGE_COMMAND_SPEED = 325.0;

//Mutex Unit::mutexDeletedUnits;
//map<void *,bool> Unit::deletedUnits;

const int UnitPathBasic::maxBlockCount= GameConstants::updateFps / 2;

#ifdef LEAK_CHECK_UNITS
std::map<UnitPathBasic *,bool> UnitPathBasic::mapMemoryList;
std::map<Unit *,bool> Unit::mapMemoryList;
std::map<UnitPathInterface *,int> Unit::mapMemoryList2;
#endif

UnitPathBasic::UnitPathBasic() : UnitPathInterface() {
#ifdef LEAK_CHECK_UNITS
	UnitPathBasic::mapMemoryList[this]=true;
#endif

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

#ifdef LEAK_CHECK_UNITS
	UnitPathBasic::mapMemoryList.erase(this);
#endif
}

#ifdef LEAK_CHECK_UNITS
void UnitPathBasic::dumpMemoryList() {
	printf("===== START report of Unfreed UnitPathBasic pointers =====\n");
	for(std::map<UnitPathBasic *,bool>::iterator iterMap = UnitPathBasic::mapMemoryList.begin();
		iterMap != UnitPathBasic::mapMemoryList.end(); ++iterMap) {
		printf("************** ==> Unfreed UnitPathBasic pointer [%p]\n",iterMap->first);

		if(Unit::mapMemoryList2.find(iterMap->first) != Unit::mapMemoryList2.end()) {
			printf("Found owner unit id [%d]\n",Unit::mapMemoryList2[iterMap->first]);
		}
	}
}
#endif

void UnitPathBasic::clearCaches() {
	this->blockCount = 0;
	this->pathQueue.clear();
	this->lastPathCacheQueue.clear();
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
			throw megaglest_runtime_error("Invalid map path position = " + path.getString() + " map w x h = " + intToStr(map->getW()) + " " + intToStr(map->getH()));
		}
		else if(this->map->isInsideSurface(this->map->toSurfCoords(path)) == false) {
			throw megaglest_runtime_error("Invalid map surface path position = " + path.getString() + " map surface w x h = " + intToStr(map->getSurfaceW()) + " " + intToStr(map->getSurfaceH()));
		}
	}

	pathQueue.push_back(path);
}

void UnitPathBasic::addToLastPathCache(const Vec2i &path) {
	if(this->map != NULL) {
		if(this->map->isInside(path) == false) {
			throw megaglest_runtime_error("Invalid map path position = " + path.getString() + " map w x h = " + intToStr(map->getW()) + " " + intToStr(map->getH()));
		}
		else if(this->map->isInsideSurface(this->map->toSurfCoords(path)) == false) {
			throw megaglest_runtime_error("Invalid map surface path position = " + path.getString() + " map surface w x h = " + intToStr(map->getSurfaceW()) + " " + intToStr(map->getSurfaceH()));
		}
	}

	const bool tryLastPathCache = Config::getInstance().getBool("EnablePathfinderCache","false");
	if(tryLastPathCache == true) {
		lastPathCacheQueue.push_back(path);
	}
}

Vec2i UnitPathBasic::pop(bool removeFrontPos) {
	if(pathQueue.empty() == true) {
		throw megaglest_runtime_error("pathQueue.size() = " + intToStr(pathQueue.size()));
	}
	Vec2i p= pathQueue.front();
	if(removeFrontPos == true) {
		pathQueue.erase(pathQueue.begin());
	}
	return p;
}
std::string UnitPathBasic::toString() const {
	std::string result = "unit path blockCount = " + intToStr(blockCount) + " pathQueue size = " + intToStr(pathQueue.size());
	for(int idx = 0; idx < pathQueue.size(); ++idx) {
		result += " index = " + intToStr(idx) + " " + pathQueue[idx].getString();
	}

	return result;
}

void UnitPathBasic::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *unitPathBasicNode = rootNode->addChild("UnitPathBasic");

//	int blockCount;
	unitPathBasicNode->addAttribute("blockCount",intToStr(blockCount), mapTagReplacements);
//	vector<Vec2i> pathQueue;
	for(unsigned int i = 0; i < pathQueue.size(); ++i) {
		Vec2i &vec = pathQueue[i];

		XmlNode *pathQueueNode = unitPathBasicNode->addChild("pathQueue");
		pathQueueNode->addAttribute("vec",vec.getString(), mapTagReplacements);
	}
//	vector<Vec2i> lastPathCacheQueue;
	for(unsigned int i = 0; i < lastPathCacheQueue.size(); ++i) {
		Vec2i &vec = lastPathCacheQueue[i];

		XmlNode *lastPathCacheQueueNode = unitPathBasicNode->addChild("lastPathCacheQueue");
		lastPathCacheQueueNode->addAttribute("vec",vec.getString(), mapTagReplacements);
	}
}

void UnitPathBasic::loadGame(const XmlNode *rootNode) {
	const XmlNode *unitPathBasicNode = rootNode->getChild("UnitPathBasic");

	blockCount = unitPathBasicNode->getAttribute("blockCount")->getIntValue();

	pathQueue.clear();
	vector<XmlNode *> pathqueueNodeList = unitPathBasicNode->getChildList("pathQueue");
	for(unsigned int i = 0; i < pathqueueNodeList.size(); ++i) {
		XmlNode *node = pathqueueNodeList[i];

		Vec2i vec = Vec2i::strToVec2(node->getAttribute("vec")->getValue());
		pathQueue.push_back(vec);
	}

	lastPathCacheQueue.clear();
	vector<XmlNode *> lastpathqueueNodeList = unitPathBasicNode->getChildList("lastPathCacheQueue");
	for(unsigned int i = 0; i < lastpathqueueNodeList.size(); ++i) {
		XmlNode *node = lastpathqueueNodeList[i];

		Vec2i vec = Vec2i::strToVec2(node->getAttribute("vec")->getValue());
		lastPathCacheQueue.push_back(vec);
	}
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
	std::string result = "unit path blockCount = " + intToStr(blockCount) + " pathQueue size = " + intToStr(size());
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

UnitReference & UnitReference::operator=(const Unit *unit){
	if(unit==NULL){
		id= -1;
		faction= NULL;
	}
	else{
		id= unit->getId();
		faction= unit->getFaction();
	}

	return *this;
}

Unit *UnitReference::getUnit() const{
	if(faction!=NULL){
		return faction->findUnit(id);
	}
	return NULL;
}

void UnitReference::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *unitRefNode = rootNode->addChild("UnitReference");

	unitRefNode->addAttribute("id",intToStr(id), mapTagReplacements);
	if(faction != NULL) {
		unitRefNode->addAttribute("factionIndex",intToStr(faction->getIndex()), mapTagReplacements);
	}
}

void UnitReference::loadGame(const XmlNode *rootNode,World *world) {
	const XmlNode *unitRefNode = rootNode->getChild("UnitReference");

	id = unitRefNode->getAttribute("id")->getIntValue();
	if(unitRefNode->hasAttribute("factionIndex") == true) {
		int factionIndex = unitRefNode->getAttribute("factionIndex")->getIntValue();
		if(factionIndex >= world->getFactionCount()) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"factionIndex >= world->getFactionCount() [%d] : [%d]",factionIndex,world->getFactionCount());
			throw megaglest_runtime_error(szBuf);
		}
		faction = world->getFaction(factionIndex);
	}
}

const bool checkMemory = false;
static map<void *,int> memoryObjectList;

UnitAttackBoostEffect::UnitAttackBoostEffect() {
	if(checkMemory) {
		printf("++ Create UnitAttackBoostEffect [%p] before count = %d\n",this,memoryObjectList[this]);
		memoryObjectList[this]++;
		printf("++ Create UnitAttackBoostEffect [%p] after count = %d\n",this,memoryObjectList[this]);
	}

	boost = NULL;
	source = NULL;
	ups = NULL;
	upst = NULL;
}

UnitAttackBoostEffect::~UnitAttackBoostEffect() {
	if(checkMemory) {
		printf("-- Delete UnitAttackBoostEffect [%p] count = %d\n",this,memoryObjectList[this]);
		memoryObjectList[this]--;
		assert(memoryObjectList[this] == 0);
	}

	if(ups != NULL) {
		bool particleValid = Renderer::getInstance().validateParticleSystemStillExists(ups,rsGame);
		if(particleValid == true) {
			ups->fade();

			vector<UnitParticleSystem *> particleSystemToRemove;
			particleSystemToRemove.push_back(ups);

			Renderer::getInstance().cleanupUnitParticleSystems(particleSystemToRemove,rsGame);
			ups = NULL;
		}
	}

	delete upst;
	upst = NULL;
}

void UnitAttackBoostEffect::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *unitAttackBoostEffectNode = rootNode->addChild("UnitAttackBoostEffect");

//	const AttackBoost *boost;
	if(boost != NULL) {
		boost->saveGame(unitAttackBoostEffectNode);
	}
//	const Unit *source;
	if(source != NULL) {
		unitAttackBoostEffectNode->addAttribute("source",intToStr(source->getId()), mapTagReplacements);
	}
//	UnitParticleSystem *ups;
	if(ups != NULL && Renderer::getInstance().validateParticleSystemStillExists(ups,rsGame) == true) {
		ups->saveGame(unitAttackBoostEffectNode);
	}

//	UnitParticleSystemType *upst;
	if(upst != NULL) {
		upst->saveGame(unitAttackBoostEffectNode);
	}
}

UnitAttackBoostEffectOriginator::UnitAttackBoostEffectOriginator() {
	skillType = NULL;
	currentAppliedEffect = NULL;
}

UnitAttackBoostEffectOriginator::~UnitAttackBoostEffectOriginator() {
	delete currentAppliedEffect;
	currentAppliedEffect = NULL;
}

void UnitAttackBoostEffectOriginator::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *unitAttackBoostEffectOriginatorNode = rootNode->addChild("UnitAttackBoostEffectOriginator");

//	const SkillType *skillType;
	if(skillType != NULL) {
		unitAttackBoostEffectOriginatorNode->addAttribute("skillType",skillType->getName(), mapTagReplacements);
	}
//	std::vector<int> currentAttackBoostUnits;
	for(unsigned int i = 0; i < currentAttackBoostUnits.size(); ++i) {
		XmlNode *currentAttackBoostUnitsNode = unitAttackBoostEffectOriginatorNode->addChild("currentAttackBoostUnits");
		currentAttackBoostUnitsNode->addAttribute("value",intToStr(currentAttackBoostUnits[i]), mapTagReplacements);
	}
//	UnitAttackBoostEffect *currentAppliedEffect;
	if(currentAppliedEffect != NULL) {
		currentAppliedEffect->saveGame(unitAttackBoostEffectOriginatorNode);
	}
}

// =====================================================
// 	class Unit
// =====================================================

const float Unit::speedDivider= 100.f;
const int Unit::maxDeadCount= 1000;	//time in until the corpse disapears - should be about 40 seconds
const int Unit::invalidId= -1;

//set<int> Unit::livingUnits;
//set<Unit*> Unit::livingUnitsp;

// ============================ Constructor & destructor =============================

Game *Unit::game = NULL;

Unit::Unit(int id, UnitPathInterface *unitpath, const Vec2i &pos,
		   const UnitType *type, Faction *faction, Map *map, CardinalDir placeFacing) : BaseColorPickEntity(), id(id) {
#ifdef LEAK_CHECK_UNITS
	Unit::mapMemoryList[this]=true;
#endif

	mutexCommands = new Mutex();
	changedActiveCommand = false;
	lastSynchDataString="";
	modelFacing = CardinalDir::NORTH;
	lastStuckFrame = 0;
	lastStuckPos = Vec2i(0,0);
	lastPathfindFailedFrame = 0;
	lastPathfindFailedPos = Vec2i(0,0);
	usePathfinderExtendedMaxNodes = false;
	this->currentAttackBoostOriginatorEffect.skillType = NULL;
	lastAttackerUnitId = -1;
	lastAttackedUnitId = -1;
	causeOfDeath = ucodNone;
	pathfindFailedConsecutiveFrameCount = 0;

	targetRotationZ=.0f;
	targetRotationX=.0f;
	rotationZ=.0f;
	rotationX=.0f;

	this->fire= NULL;
    this->unitPath = unitpath;
    this->unitPath->setMap(map);

    //RandomGen random;
    random.init(id);
	pathFindRefreshCellCount = random.randRange(10,20);

	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		throw megaglest_runtime_error("#2 Invalid path position = " + pos.getString());
	}

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
	this->morphFieldsBlocked=false;
	//this->lastBadHarvestListPurge = 0;

	level= NULL;
	loadType= NULL;

    setModelFacing(placeFacing);

    Config &config= Config::getInstance();
	showUnitParticles				= config.getBool("UnitParticles","true");
	maxQueuedCommandDisplayCount 	= config.getInt("MaxQueuedCommandDisplayCount","15");

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		showUnitParticles = false;
	}

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

	computeTotalUpgrade();

	//starting skill
	this->lastModelIndexForCurrSkillType = -1;
	this->animationRandomCycleCount = 0;
	this->currSkill = getType()->getFirstStOfClass(scStop);
	this->currentAttackBoostOriginatorEffect.skillType = this->currSkill;

	this->faction->addLivingUnits(id);
	this->faction->addLivingUnitsp(this);

	addItemToVault(&this->hp,this->hp);
	addItemToVault(&this->ep,this->ep);

	calculateFogOfWarRadius();

//	if(isUnitDeleted(this) == true) {
//		MutexSafeWrapper safeMutex(&mutexDeletedUnits,string(__FILE__) + "_" + intToStr(__LINE__));
//		deletedUnits.erase(this);
//	}

	logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
}

Unit::~Unit() {
	badHarvestPosList.clear();

	this->faction->deleteLivingUnits(id);
	this->faction->deleteLivingUnitsp(this);

	//remove commands
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	changedActiveCommand = false;
	while(commands.empty() == false) {
		delete commands.back();
		commands.pop_back();
	}
	safeMutex.ReleaseLock();

	// If the unit is not visible we better make sure we cleanup associated particles
	if(this->getVisible() == false) {
		Renderer::getInstance().cleanupUnitParticleSystems(unitParticleSystems,rsGame);

		Renderer::getInstance().cleanupParticleSystems(fireParticleSystems,rsGame);
		// Must set this to null of it will be used below in stopDamageParticles()

		if(Renderer::getInstance().validateParticleSystemStillExists(this->fire,rsGame) == false) {
			this->fire = NULL;
		}
	}

	// fade(and by this remove) all unit particle systems
	queuedUnitParticleSystemTypes.clear();
	while(unitParticleSystems.empty() == false) {
		if(Renderer::getInstance().validateParticleSystemStillExists(unitParticleSystems.back(),rsGame) == true) {
			unitParticleSystems.back()->fade();
		}
		unitParticleSystems.pop_back();
	}
	stopDamageParticles(true);

	while(currentAttackBoostEffects.empty() == false) {
		//UnitAttackBoostEffect &effect = currentAttackBoostEffects.back();
		UnitAttackBoostEffect *ab = currentAttackBoostEffects.back();
		delete ab;
		currentAttackBoostEffects.pop_back();
	}

	delete currentAttackBoostOriginatorEffect.currentAppliedEffect;
	currentAttackBoostOriginatorEffect.currentAppliedEffect = NULL;

#ifdef LEAK_CHECK_UNITS
	Unit::mapMemoryList2[this->unitPath] = this->getId();
#endif

	delete this->unitPath;
	this->unitPath = NULL;

	Renderer &renderer= Renderer::getInstance();
	renderer.removeUnitFromQuadCache(this);
	if(game != NULL) {
		game->removeUnitFromSelection(this);
	}

	//MutexSafeWrapper safeMutex1(&mutexDeletedUnits,string(__FILE__) + "_" + intToStr(__LINE__));
	//deletedUnits[this]=true;

	delete mutexCommands;
	mutexCommands=NULL;

	recycleUniqueColor();

#ifdef LEAK_CHECK_UNITS
	Unit::mapMemoryList.erase(this);
#endif
}

ParticleSystem * Unit::getFire() const	{
	if(Renderer::getInstance().validateParticleSystemStillExists(this->fire,rsGame) == false) {
		return NULL;
	}
	return this->fire;
}

#ifdef LEAK_CHECK_UNITS
void Unit::dumpMemoryList() {
	printf("===== START report of Unfreed Unit pointers =====\n");
	for(std::map<Unit *,bool>::iterator iterMap = Unit::mapMemoryList.begin();
		iterMap != Unit::mapMemoryList.end(); ++iterMap) {
		printf("************** ==> Unfreed Unit pointer [%p]\n",iterMap->first);
	}
}
#endif

//bool Unit::isUnitDeleted(void *unit) {
//	bool result = false;
//	MutexSafeWrapper safeMutex(&mutexDeletedUnits,string(__FILE__) + "_" + intToStr(__LINE__));
//	if(deletedUnits.find(unit) != deletedUnits.end()) {
//		result = true;
//	}
//	return result;
//}

void Unit::setModelFacing(CardinalDir value) {
	modelFacing = value;
	lastRotation = targetRotation = rotation = value * 90.f;
}

// ====================================== get ======================================

Vec2i Unit::getCenteredPos() const {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

    return pos + Vec2i(type->getSize()/2, type->getSize()/2);
}

Vec2f Unit::getFloatCenteredPos() const {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	return Vec2f(pos.x-0.5f+type->getSize()/2.f, pos.y-0.5f+type->getSize()/2.f);
}

Vec2i Unit::getCellPos() const {
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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



void Unit::calculateXZRotation(){
	//if(type->getProperty(UnitType::pRotatedClimb) && currSkill->getClass()==scMove){
	//if(currSkill->getClass()==scMove)
	if(lastPos != pos){ // targetPosCalc ( maybe also sometimes needed if no move ? terrain flatting... )
		SurfaceCell* sc= map->getSurfaceCell(Map::toSurfCoords(pos));
		const Vec3f normal= sc->getNormal();

#ifdef USE_STREFLOP
		targetRotationZ= radToDeg(streflop::atan2(static_cast<streflop::Simple>(abs(normal.x)), static_cast<streflop::Simple>(abs(normal.y))));
#else
		targetRotationZ= radToDeg(atan2(abs(normal.x), abs(normal.y)));
#endif

		if((normal.y < 0 || normal.x < 0) && !(normal.y < 0 && normal.x < 0)){
			targetRotationZ= targetRotationZ * -1;
		}
		targetRotationZ= targetRotationZ * -1;

#ifdef USE_STREFLOP
		targetRotationX= radToDeg(streflop::atan2(static_cast<streflop::Simple>(abs(normal.z)), static_cast<streflop::Simple>(abs(normal.y))));
#else
		targetRotationX= radToDeg(atan2(abs(normal.z), abs(normal.y)));
#endif

		if((normal.y < 0 || normal.z < 0) && !(normal.y < 0 && normal.z < 0)){
			targetRotationX= targetRotationX * -1;
		}
	}

	//For smooth rotation we now softly adjust the angle
	int adjustStep= 1;
	if(rotationZ < targetRotationZ){
		if(rotationZ + adjustStep > targetRotationZ){
			rotationZ= targetRotationZ;
		}
		else{
			rotationZ= rotationZ + adjustStep;
		}
	}
	else if(rotationZ > targetRotationZ){
		if(rotationZ - adjustStep < targetRotationZ){
			rotationZ= targetRotationZ;
		}
		else{
			rotationZ= rotationZ - adjustStep;
		}
	}

	if(rotationX < targetRotationX){
		if(rotationX + adjustStep > targetRotationX){
			rotationX= targetRotationX;
		}
		else{
			rotationX= rotationX + adjustStep;
		}
	}
	else if(rotationX > targetRotationX){
		if(rotationX - adjustStep < targetRotationX){
			rotationX= targetRotationX;
		}
		else{
			rotationX= rotationX - adjustStep;
		}
	}
}

float Unit::getRotationZ() const{
	return rotationZ;
}

float Unit::getRotationX() const{
	return rotationX;
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

float Unit::getProgressRatio() const{
	if(anyCommand()){
		const ProducibleType *produced= commands.front()->getCommandType()->getProduced();
		if(produced!=NULL){
			float help=progress2;
			return clamp(help/produced->getProductionTime(), 0.f, 1.f);
		}
	}
	return -1;
}

float Unit::getHpRatio() const {
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	return clamp(static_cast<float>(hp)/type->getTotalMaxHp(&totalUpgrade), 0.f, 1.f);
}

float Unit::getEpRatio() const {
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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
		str += (level->getName(true) + " ");
	}
	if(type == NULL) {
	    throw megaglest_runtime_error("type == NULL in Unit::getFullName()!");
	}
	str += type->getName(true);
	return str;
}

// ====================================== is ======================================

bool Unit::isOperative() const{
    return isAlive() && isBuilt();
}

bool Unit::isAnimProgressBound() const{
	if(currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	bool result = false;
    if(currSkill->getClass() == scBeBuilt) {
    	const BeBuiltSkillType *skill = dynamic_cast<const BeBuiltSkillType*>(currSkill);
    	if(skill != NULL) {
    		result = skill->getAnimProgressBound();
    	}
    }
    else if(currSkill->getClass() == scProduce) {
    	const ProduceSkillType *skill = dynamic_cast<const ProduceSkillType*>(currSkill);
    	if(skill != NULL) {
    		result = skill->getAnimProgressBound();
    	}
    }
    else if(currSkill->getClass() == scUpgrade) {
    	const UpgradeSkillType *skill = dynamic_cast<const UpgradeSkillType*>(currSkill);
    	if(skill != NULL) {
    		result = skill->getAnimProgressBound();
    	}
    }
    else if(currSkill->getClass() == scMorph) {
    	const MorphSkillType *skill = dynamic_cast<const MorphSkillType*>(currSkill);
    	if(skill != NULL) {
    		result = skill->getAnimProgressBound();
    	}
    }
    return result;
}

bool Unit::isBeingBuilt() const{
	if(currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

    return (currSkill->getClass() == scBeBuilt);
}

bool Unit::isBuilt() const{
    return (isBeingBuilt() == false);
}

bool Unit::isBuildCommandPending() const {
	bool result = false;

	Command *command= this->getCurrCommand();
	if(command != NULL) {
		const BuildCommandType *bct= dynamic_cast<const BuildCommandType*>(command->getCommandType());
		if(bct != NULL) {
			if(this->getCurrSkill()->getClass() != scBuild) {
				result = true;
			}
		}
	}

	return result;
}

UnitBuildInfo Unit::getBuildCommandPendingInfo() const {
	UnitBuildInfo result;

	Command *command= this->getCurrCommand();
	if(command != NULL) {
		const BuildCommandType *bct= dynamic_cast<const BuildCommandType*>(command->getCommandType());
		if(bct != NULL) {
			result.pos = command->getOriginalPos();
			result.facing = command->getFacing();
			result.buildUnit = command->getUnitType();
			result.unit = this;
		}
	}

	return result;
}

bool Unit::isAlly(const Unit *unit) const {
	if(unit == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: unit == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	return faction->isAlly(unit->getFaction());
}

bool Unit::isDamaged() const {
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	return hp < type->getTotalMaxHp(&totalUpgrade);
}

bool Unit::isInteresting(InterestingUnitType iut) const {
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}
	if(this->currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: this->currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	if(this->currSkill->getClass() == scMove &&
		currSkill->getClass() != scMove) {
		faction->removeUnitFromMovingList(this->getId());
	}
	else if(this->currSkill->getClass() != scMove &&
		currSkill->getClass() == scMove) {
		faction->addUnitToMovingList(this->getId());
	}

	changedActiveCommand = false;
	if( currSkill->getClass() != this->currSkill->getClass() ||
		currSkill->getName() != this->currSkill->getName()) {
		animProgress= 0;
		lastAnimProgress= 0;

		queuedUnitParticleSystemTypes.clear();
		while(unitParticleSystems.empty() == false) {
			if(Renderer::getInstance().validateParticleSystemStillExists(unitParticleSystems.back(),rsGame) == true) {
				unitParticleSystems.back()->fade();
			}
			unitParticleSystems.pop_back();
		}

		Command *cmd = getCurrrentCommandThreadSafe();

		// Set mew fog of war skill type if need be
		if(cmd != NULL && cmd->getCommandType() != NULL &&
				cmd->getCommandType()->hasFogOfWarSkillType(currSkill->getName())) {
			const FogOfWarSkillType *fowst = cmd->getCommandType()->getFogOfWarSkillType();

			// Remove old fog of war skill type if need be
			game->getWorld()->removeFogOfWarSkillTypeFromList(this);

			game->getWorld()->addFogOfWarSkillType(this,fowst);
		}
		else {
			// Remove old fog of war skill type if need be
			game->getWorld()->removeFogOfWarSkillType(this);
		}
	}
	if(showUnitParticles == true  &&
		currSkill->unitParticleSystemTypes.empty() == false &&
		unitParticleSystems.empty() == true) {
		//printf("START - particle system type\n");

		for(UnitParticleSystemTypes::const_iterator it= currSkill->unitParticleSystemTypes.begin(); it != currSkill->unitParticleSystemTypes.end(); ++it) {
			if((*it)->getStartTime() == 0.0) {
				//printf("Adding NON-queued particle system type [%s] [%f] [%f]\n",(*it)->getType().c_str(),(*it)->getStartTime(),(*it)->getEndTime());

				UnitParticleSystem *ups = new UnitParticleSystem(200);
				(*it)->setValues(ups);
				ups->setPos(getCurrVector());
				if(getFaction()->getTexture()) {
					ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
				}
				unitParticleSystems.push_back(ups);
				Renderer::getInstance().manageParticleSystem(ups, rsGame);
			}
			else {
				//printf("Adding queued particle system type [%s] [%f] [%f]\n",(*it)->getType().c_str(),(*it)->getStartTime(),(*it)->getEndTime());

				queuedUnitParticleSystemTypes.push_back(*it);
			}
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
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: getType() == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

    setCurrSkill(getType()->getFirstStOfClass(sc));
}

void Unit::setTarget(const Unit *unit){

	if(unit == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: unit == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//find a free pos in cellmap
	setTargetPos(unit->getCellPos());

	//ser field and vector
	targetField= unit->getCurrField();
	targetVec= unit->getCurrVector();
	targetRef= unit;
}

void Unit::setPos(const Vec2i &pos, bool clearPathFinder) {
	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		throw megaglest_runtime_error("#3 Invalid path position = " + pos.getString());
	}

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	if(clearPathFinder == true && this->unitPath != NULL) {
		this->unitPath->clear();
	}
	Vec2i oldLastPos = this->lastPos;
	this->lastPos= this->pos;
	this->pos= pos;

//  	// This code is initial code to make the camera 'follow' a unit
//	//if(this->getId() == 5) {
//	if(cameraFollowUnit == true) {
//		//printf("A fov [%f] [H [%f] [V [%f]\n",game->getGameCameraPtr()->getFov(),game->getGameCameraPtr()->getHAng(),game->getGameCameraPtr()->getVAng());
//
//		//game->getGameCameraPtr()->setClampDisabled(true);
//
//		//game->getGameCameraPtr()->setFov(45);
//		if(oldLastPos.x > pos.x) {
//			game->getGameCameraPtr()->setHAng(270);
//			game->getGameCameraPtr()->setVAng(-7.6);
//		}
//		else if(oldLastPos.x < pos.x) {
//			game->getGameCameraPtr()->setHAng(90);
//			game->getGameCameraPtr()->setVAng(1.4);
//		}
//		else if(oldLastPos.y > pos.y) {
//			game->getGameCameraPtr()->setHAng(180);
//			game->getGameCameraPtr()->setVAng(4.2);
//		}
//		else {
//			game->getGameCameraPtr()->setHAng(-2.4);
//			game->getGameCameraPtr()->setVAng(-1.4);
//		}
//
//		game->getGameCameraPtr()->setPos(getCurrVector());
//		game->getGameCameraPtr()->stop();
//
//		//printf("B fov [%f] [H [%f] [V [%f]\n",game->getGameCameraPtr()->getFov(),game->getGameCameraPtr()->getHAng(),game->getGameCameraPtr()->getVAng());
//	}

	map->clampPos(this->pos);
	this->meetingPos= pos - Vec2i(1);
	map->clampPos(this->meetingPos);

	safeMutex.ReleaseLock();

	// Attempt to improve performance
	this->exploreCells();

	calculateFogOfWarRadius();

	logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
}

FowAlphaCellsLookupItem Unit::getFogOfWarRadius(bool useCache) const {
	if(useCache == true && Config::getInstance().getBool("EnableFowCache","true") == true) {
		return cachedFow;
	}

	FowAlphaCellsLookupItem result;
	//iterate through all cells
	int sightRange= this->getType()->getSight();
	PosCircularIterator pci(map, this->getPosNotThreadSafe(), sightRange + World::indirectSightRange);
	while(pci.next()){
		const Vec2i sightpos= pci.getPos();
		Vec2i surfPos= Map::toSurfCoords(sightpos);

		//compute max alpha
		float maxAlpha= 0.0f;
		if(surfPos.x > 1 && surfPos.y > 1 && surfPos.x < map->getSurfaceW() -2 && surfPos.y < map->getSurfaceH() -2) {
			maxAlpha= 1.f;
		}
		else if(surfPos.x > 0 && surfPos.y > 0 && surfPos.x < map->getSurfaceW() -1 && surfPos.y < map->getSurfaceH() -1) {
			maxAlpha= 0.3f;
		}

		//compute alpha
		float alpha = maxAlpha;
		float dist = this->getPosNotThreadSafe().dist(sightpos);
		if(dist > sightRange) {
			alpha= clamp(1.f-(dist - sightRange) / (World::indirectSightRange), 0.f, maxAlpha);
		}
		result.surfPosList.push_back(surfPos);
		result.alphaList.push_back(alpha);
	}
	return result;
}

void Unit::calculateFogOfWarRadius() {
	if(game->getWorld()->getFogOfWar() == true) {
		if(Config::getInstance().getBool("EnableFowCache","true") == true && this->pos != this->cachedFowPos) {
			cachedFow = getFogOfWarRadius(false);

			static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
			MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);
			this->cachedFowPos = this->pos;
		}
	}
}

void Unit::setTargetPos(const Vec2i &targetPos) {

	if(map->isInside(targetPos) == false || map->isInsideSurface(map->toSurfCoords(targetPos)) == false) {
		throw megaglest_runtime_error("#4 Invalid path position = " + targetPos.getString());
	}

	Vec2i relPos= targetPos - pos;
	//map->clampPos(relPos);

	Vec2f relPosf= Vec2f((float)relPos.x, (float)relPos.y);
#ifdef USE_STREFLOP
	targetRotation= radToDeg(streflop::atan2(static_cast<streflop::Simple>(relPosf.x), static_cast<streflop::Simple>(relPosf.y)));
#else
	targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));
#endif
	targetRef= NULL;

	//this->targetField = fLand;
	//this->targetVec   = Vec3f(0.0);
	//this->targetPos   = Vec2i(0);

	this->targetPos= targetPos;
	map->clampPos(this->targetPos);

	logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
}

void Unit::setVisible(const bool visible) {
	this->visible = visible;

	if(unitParticleSystems.empty() == false) {
		for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
			if(Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame) == true) {
				(*it)->setVisible(visible);
			}
		}
	}
	if(damageParticleSystems.empty() == false) {
		for(UnitParticleSystems::iterator it= damageParticleSystems.begin(); it != damageParticleSystems.end(); ++it) {
			if(Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame) == true) {
				(*it)->setVisible(visible);
			}
		}
	}
	if(smokeParticleSystems.empty() == false) {
		for(UnitParticleSystems::iterator it= smokeParticleSystems.begin(); it != smokeParticleSystems.end(); ++it) {
			if(Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame) == true) {
				if((*it)->getVisible() != visible) {
					//printf("Changing visibility for smoke particle system to: %d\n",visible);
					(*it)->setVisible(visible);
				}
			}
		}
	}
	if(currentAttackBoostEffects.empty() == false) {
		for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
			UnitAttackBoostEffect *effect = currentAttackBoostEffects[i];
			if(effect != NULL && effect->ups != NULL) {
				bool particleValid = Renderer::getInstance().validateParticleSystemStillExists(effect->ups,rsGame);
				if(particleValid == true) {
					effect->ups->setVisible(visible);
				}
			}
		}
	}
	if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
		if(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups != NULL) {
			bool particleValid = Renderer::getInstance().validateParticleSystemStillExists(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups,rsGame);
			if(particleValid == true) {
				currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setVisible(visible);
			}
		}
	}
}

// =============================== Render related ==================================

Model *Unit::getCurrentModelPtr() {
	if(currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	int currentModelIndexForCurrSkillType = lastModelIndexForCurrSkillType;
	Model *result = currSkill->getAnimation(animProgress,this,&lastModelIndexForCurrSkillType, &animationRandomCycleCount);
	if(currentModelIndexForCurrSkillType != lastModelIndexForCurrSkillType) {
		animationRandomCycleCount++;
		if(currSkill != NULL && animationRandomCycleCount >= currSkill->getAnimationCount()) {
			animationRandomCycleCount = 0;
		}
	}
	return result;
}

const Model *Unit::getCurrentModel() {
	if(currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	int currentModelIndexForCurrSkillType = lastModelIndexForCurrSkillType;
	const Model *result = currSkill->getAnimation(animProgress,this,&lastModelIndexForCurrSkillType, &animationRandomCycleCount);
	if(currentModelIndexForCurrSkillType != lastModelIndexForCurrSkillType) {
		animationRandomCycleCount++;
		if(currSkill != NULL && animationRandomCycleCount >= currSkill->getAnimationCount()) {
			animationRandomCycleCount = 0;
		}
	}
	return result;
}

bool Unit::checkModelStateInfoForNewHpValue() {
	bool result = false;
	if(currSkill != NULL && currSkill->getAnimationCount() > 1) {
		if(lastModelIndexForCurrSkillType >= 0) {
			const AnimationAttributes attributes = currSkill->getAnimationAttribute(lastModelIndexForCurrSkillType);
			if(attributes.fromHp != 0 || attributes.toHp != 0) {
				//printf("Check for RESET model state for [%d - %s] HP = %d [%d to %d]\n",this->id,this->getType()->getName().c_str(),this->getHp(),attributes.fromHp,attributes.toHp);
				//if(this->getHp() >= attributes.fromHp && this->getHp() <= attributes.toHp) {
				if(this->getHp() < attributes.fromHp || this->getHp() > attributes.toHp) {
					//printf("RESET model state for [%d - %s] HP = %d [%d to %d]\n",this->id,this->getType()->getName().c_str(),this->getHp(),attributes.fromHp,attributes.toHp);

					lastModelIndexForCurrSkillType = -1;
					animationRandomCycleCount = 0;
					result = true;
				}
			}
			else {
				//printf("Check for RESET #2 model state for [%d - %s] HP = %d [%d to %d] for skill [%s]\n",this->id,this->getType()->getName().c_str(),this->getHp(),attributes.fromHp,attributes.toHp,currSkill->getName().c_str());
			}
		}
	}

	return result;
}

Vec3f Unit::getCurrVector() const{
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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
Command *Unit::getCurrrentCommandThreadSafe() {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	if(commands.empty() == false) {
		return commands.front();
	}

	return NULL;
}

void Unit::replaceCurrCommand(Command *cmd) {
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

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
std::pair<CommandResult,string> Unit::giveCommand(Command *command, bool tryQueue) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"\n======================\nUnit Command tryQueue = %d\nUnit Info:\n%s\nCommand Info:\n%s\n",tryQueue,this->toString().c_str(),command->toString().c_str());

	std::pair<CommandResult,string> result(crFailUndefined,"");
	changedActiveCommand = false;

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

    assert(command != NULL);
    assert(command->getCommandType() != NULL);

    const int command_priority = command->getPriority();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

    //printf("In [%s::%s] Line: %d unit [%d - %s] command [%s] tryQueue = %d command->getCommandType()->isQueuable(tryQueue) = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->getId(),this->getType()->getName().c_str(), command->getCommandType()->getName().c_str(), tryQueue,command->getCommandType()->isQueuable(tryQueue));


	if(command->getCommandType()->isQueuable(tryQueue)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Command is Queable\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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
						SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Deleting lower priority command [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(*i)->toString().c_str());

					static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
					MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

					deleteQueuedCommand(*i);
					i= commands.erase(i);

					safeMutex.ReleaseLock();
				}
				else{
					++i;
				}
			}
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

		//cancel current command if it is not queuable
		if(commands.empty() == false &&
          commands.back()->getCommandType()->isQueueAppendable() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Cancel command because last one is NOT queable [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,commands.back()->toString().c_str());

		    cancelCommand();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	}
	else {
		//empty command queue
		if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] Clear commands because current is NOT queable.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		bool willChangedActiveCommand= (commands.empty() == false);
		if(willChangedActiveCommand){
			CommandClass currCommandClass=getCurrCommand()->getCommandType()->getClass();
			CommandClass commandClass=command->getCommandType()->getClass();
			if(currCommandClass
			        == commandClass){
				willChangedActiveCommand= false;
			}
			else if(currCommandClass==ccAttack || currCommandClass==ccAttackStopped ||
					commandClass==ccAttack || commandClass==ccAttackStopped){
				willChangedActiveCommand= true;
			}
			else{
				willChangedActiveCommand= false;
			}
		}
		clearCommands();
		changedActiveCommand= willChangedActiveCommand;



		//printf("In [%s::%s] Line: %d cleared existing commands\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	//check command
	result= checkCommand(command);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled) SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"In [%s::%s Line: %d] checkCommand returned: [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,result.first);

	//printf("In [%s::%s] Line: %d check command returned %d, commands.size() = %d\n[%s]\n\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,result,commands.size(),command->toString().c_str());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	if(result.first == crSuccess) {
		applyCommand(command);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	//push back command
	if(result.first == crSuccess) {
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

		commands.push_back(command);

		safeMutex.ReleaseLock();
	}
	else {
		delete command;
		changedActiveCommand = false;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	return result;
}

//pop front (used when order is done)
CommandResult Unit::finishCommand() {
	changedActiveCommand = false;
	retryCurrCommandCount=0;
	this->setCurrentUnitTitle("");
	//is empty?
	if(commands.empty()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__, __LINE__);
		return crFailUndefined;
	}

	//pop front
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	delete commands.front();
	commands.erase(commands.begin());

	safeMutex.ReleaseLock(true);

	this->unitPath->clear();

	while (commands.empty() == false) {
		if (commands.front()->getUnit() != NULL && this->faction->isUnitInLivingUnitsp(commands.front()->getUnit()) == false) {
			safeMutex.Lock();
			delete commands.front();
			commands.erase(commands.begin());
			safeMutex.ReleaseLock(true);
		}
		else {
			break;
		}
	}

	return crSuccess;
}

//to cancel a command
CommandResult Unit::cancelCommand() {
	changedActiveCommand = false;
	retryCurrCommandCount=0;
	this->setCurrentUnitTitle("");

	//is empty?
	if(commands.empty()){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__, __LINE__);
		return crFailUndefined;
	}

	//undo command
	undoCommand(commands.back());

	//delete ans pop command
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

	delete commands.back();
	commands.pop_back();

	safeMutex.ReleaseLock();

	//clear routes
	this->unitPath->clear();



	return crSuccess;
}

// =================== route stack ===================

void Unit::create(bool startingUnit) {
	faction->addUnit(this);
	map->putUnitCells(this, pos);
	if(startingUnit) {
		faction->applyStaticCosts(type,NULL);
	}
}

void Unit::born(const CommandType *ct) {
	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	faction->addStore(type);
	faction->applyStaticProduction(type,ct);
	setCurrSkill(scStop);

	checkItemInVault(&this->hp,this->hp);
	hp= type->getMaxHp();
	addItemToVault(&this->hp,this->hp);
}

void Unit::kill() {
	//no longer needs static resources
	if(isBeingBuilt()) {
		faction->deApplyStaticConsumption(type,(getCurrCommand() != NULL ? getCurrCommand()->getCommandType() : NULL));
	}
	else {
		faction->deApplyStaticCosts(type,(getCurrCommand() != NULL ? getCurrCommand()->getCommandType() : NULL));
	}

	//do the cleaning
	//clear commands ( and their blocking fields )
	clearCommands();

	map->clearUnitCells(this, pos, true);
	if(isBeingBuilt() == false) {
		faction->removeStore(type);
	}
    setCurrSkill(scDie);

	notifyObservers(UnitObserver::eKill);


	UnitUpdater *unitUpdater = game->getWorld()->getUnitUpdater();
	//unitUpdater->clearUnitPrecache(this);
	unitUpdater->removeUnitPrecache(this);
}

void Unit::undertake() {
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to undertake unit id = %d [%s] [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->id, this->getFullName().c_str(),this->getDesc().c_str());

		// Remove any units that were previously in attack-boost range
		if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.empty() == false && currentAttackBoostOriginatorEffect.skillType != NULL) {
			for(unsigned int i = 0; i < currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size(); ++i) {
				// Remove attack boost upgrades from unit
				int findUnitId = currentAttackBoostOriginatorEffect.currentAttackBoostUnits[i];
				Unit *affectedUnit = game->getWorld()->findUnitById(findUnitId);
				if(affectedUnit != NULL) {
					affectedUnit->deapplyAttackBoost(currentAttackBoostOriginatorEffect.skillType->getAttackBoost(), this);
				}

				//printf("!!!! DE-APPLY ATTACK BOOST from unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
			}
			currentAttackBoostOriginatorEffect.currentAttackBoostUnits.clear();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		UnitUpdater *unitUpdater = game->getWorld()->getUnitUpdater();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		//unitUpdater->clearUnitPrecache(this);
		unitUpdater->removeUnitPrecache(this);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		this->faction->deleteLivingUnits(id);
		this->faction->deleteLivingUnitsp(this);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		faction->removeUnit(this);
	}
	catch(const megaglest_runtime_error &ex) {
		string sErrBuf = "";
		if(ex.wantStackTrace() == true) {
			char szErrBuf[8096]="";
			snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			sErrBuf = string(szErrBuf) + string("\nerror [") + string(ex.what()) + string("]\n");
		}
		else {
			sErrBuf = ex.what();
		}
		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());

		throw;
	}
	catch(const std::exception &ex) {
		char szErrBuf[8096]="";
		snprintf(szErrBuf,8096,"In [%s::%s %d]",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		string sErrBuf = string(szErrBuf) + string("\nerror [") + string(ex.what()) + string("]\n");
		SystemFlags::OutputDebug(SystemFlags::debugError,sErrBuf.c_str());

		throw;
	}
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
		throw megaglest_runtime_error("#6 Invalid path position = " + pos.getString());
	}

	SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(pos));

	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//printf("Line: %d Unit::computeCommandType pos [%s] targetUnit [%s]\n",__LINE__,pos.getString().c_str(),(targetUnit != NULL ? targetUnit->getType()->getName().c_str() : "(null)"));
	if(targetUnit != NULL) {
		//attack enemies
		if(isAlly(targetUnit) == false) {
			commandType= type->getFirstAttackCommand(targetUnit->getCurrField());
		}
		//repair allies
		else {
			if(targetUnit->isBuilt() == false || targetUnit->isDamaged() == true) {
				commandType= type->getFirstRepairCommand(targetUnit->getType());
			}

			//printf("Line: %d Unit::computeCommandType pos [%s] targetUnit [%s] commandType [%p]\n",__LINE__,pos.getString().c_str(),(targetUnit != NULL ? targetUnit->getType()->getName().c_str() : "(null)"),commandType);

			if(commandType == NULL && targetUnit != NULL) {
				Command *command= this->getCurrCommand();
			    const HarvestCommandType *hct= dynamic_cast<const HarvestCommandType*>((command != NULL ? command->getCommandType() : NULL));

				// Check if we can return whatever resources we have
				if(targetUnit->getFactionIndex() == this->getFactionIndex() &&
					targetUnit->isOperative() == true &&
					this->getLoadType() != NULL && this->getLoadCount() != 0 &&
					targetUnit->getType() != NULL &&
					targetUnit->getType()->getStore(this->getLoadType()) > 0) {

					const HarvestCommandType *previousHarvestCmd = this->getType()->getFirstHarvestCommand(this->getLoadType(),this->getFaction());
					if(previousHarvestCmd != NULL) {
						//printf("\n#1 return harvested resources\n\n");
						//this->setCurrSkill(previousHarvestCmd->getStopLoadedSkillType()); // make sure we use the right harvest animation
						commandType = type->getFirstHarvestEmergencyReturnCommand();
					}
					else {
						//printf("\n#2 return harvested resources\n\n");
						//this->setCurrSkill(hct->getStopLoadedSkillType());
						commandType = type->getFirstHarvestEmergencyReturnCommand();
					}
				}
			}
		}
	}
	else {
		//check harvest command
		Resource *resource= sc->getResource();
		if(resource != NULL) {
			commandType= type->getFirstHarvestCommand(resource->getType(),this->getFaction());
		}
	}

	if(targetUnit == NULL && commandType == NULL) {
		const Vec2i unitTargetPos = pos;
		Cell *cell= map->getCell(unitTargetPos);
		if(cell != NULL && cell->getUnit(this->getCurrField()) != NULL) {
			Unit *targetUnit = cell->getUnit(this->getCurrField());
			if(targetUnit != NULL) {
				// Check if we want to help build (repair) any buildings instead of just moving
				if(targetUnit->getFactionIndex() == this->getFactionIndex() &&
					(targetUnit->isBuilt() == false || targetUnit->isDamaged() == true)) {
					const RepairCommandType *rct= this->getType()->getFirstRepairCommand(targetUnit->getType());
					if(rct != NULL) {
						commandType= type->getFirstRepairCommand(targetUnit->getType());
						//printf("************ Unit will repair building built = %d, repair = %d\n",targetUnit->isBuilt(),targetUnit->isDamaged());
					}
				}
			}
		}
	}

	//default command is move command
	if(commandType == NULL) {
		commandType= type->getFirstCtOfClass(ccMove);
	}

	return commandType;
}


bool Unit::needToUpdate() {
	//assert(progress <= 1.f);
	if(progress > 1.f) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: progress > 1.f, progress = [%f]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,progress);
		throw megaglest_runtime_error(szBuf);
	}

	if(currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	bool return_value = false;
	if(currSkill->getClass() != scDie) {
		//speed
		int speed = currSkill->getTotalSpeed(&totalUpgrade);

		if(changedActiveCommand) {
			speed = CHANGE_COMMAND_SPEED;
		}

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
			Cell *unitCell = map->getCell(pos);
			if(unitCell == NULL) {
				throw megaglest_runtime_error("unitCell == NULL");
			}

			Cell *targetCell = map->getCell(targetPos);
			if(targetCell == NULL) {
				throw megaglest_runtime_error("targetCell == NULL");
			}

			float heightDiff= unitCell->getHeight() - targetCell->getHeight();
			heightFactor= clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
		}

		//update progresses
		const Game *game = Renderer::getInstance().getGame();
		if(game == NULL) {
			throw megaglest_runtime_error("game == NULL");
		}
		if(game->getWorld() == NULL) {
			throw megaglest_runtime_error("game->getWorld() == NULL");
		}

		float speedDenominator = (speedDivider * game->getWorld()->getUpdateFps(this->getFactionIndex()));
		float newProgress = progress;
		newProgress += (speed * diagonalFactor * heightFactor) / speedDenominator;

		if(newProgress >= 1.f) {
			return_value = true;
		}
	}
	return return_value;
}

void Unit::updateTimedParticles() {
	//!!!
	// Start new particle systems based on start time
	if(queuedUnitParticleSystemTypes.empty() == false) {
		for(int i = queuedUnitParticleSystemTypes.size() - 1; i >= 0; i--) {
			UnitParticleSystemType *pst = queuedUnitParticleSystemTypes[i];
			if(pst != NULL) {
				if(truncateDecimal<float>(pst->getStartTime()) <= truncateDecimal<float>(animProgress)) {
					//printf("STARTING queued particle system type [%s] [%f] [%f] [%f] [%f]\n",pst->getType().c_str(),truncateDecimal<float>(pst->getStartTime()),truncateDecimal<float>(pst->getEndTime()),truncateDecimal<float>(animProgress),truncateDecimal<float>(lastAnimProgress));

					UnitParticleSystem *ups = new UnitParticleSystem(200);
					pst->setValues(ups);
					ups->setPos(getCurrVector());
					if(getFaction()->getTexture()) {
						ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
					}
					unitParticleSystems.push_back(ups);
					Renderer::getInstance().manageParticleSystem(ups, rsGame);

					queuedUnitParticleSystemTypes.erase(queuedUnitParticleSystemTypes.begin() + i);
				}
			}
			else {
				queuedUnitParticleSystemTypes.erase(queuedUnitParticleSystemTypes.begin() + i);
			}
		}
	}

	// End existing systems based on end time
	if(unitParticleSystems.empty() == false) {
		for(int i = unitParticleSystems.size() - 1; i >= 0; i--) {
			UnitParticleSystem *ps = unitParticleSystems[i];
			if(ps != NULL) {
				if(Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
					if(truncateDecimal<float>(ps->getStartTime()) != 0.0 || truncateDecimal<float>(ps->getEndTime()) != 1.0) {
						//printf("Checking for end particle system #%d [%d] [%f] [%f] [%f] [%f]\n",i,ps->shape,truncateDecimal<float>(ps->getStartTime()),truncateDecimal<float>(ps->getEndTime()),truncateDecimal<float>(animProgress),truncateDecimal<float>(lastAnimProgress));

						if(truncateDecimal<float>(animProgress) >= 0.99 ||
								truncateDecimal<float>(animProgress) >= truncateDecimal<float>(ps->getEndTime())) {
							//printf("ENDING particle system [%d]\n",ps->shape);

							ps->fade();
							unitParticleSystems.erase(unitParticleSystems.begin() + i);
						}
					}
				}
			}
		}
	}
}

bool Unit::update() {
	assert(progress <= 1.f);

	//highlight
	if(highlight > 0.f) {
		const Game *game = Renderer::getInstance().getGame();
		highlight -= 1.f / (Game::highlightTime * game->getWorld()->getUpdateFps(this->getFactionIndex()));
	}

	if(currSkill == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//speed
	int speed= currSkill->getTotalSpeed(&totalUpgrade);

	if(changedActiveCommand) {
		speed = CHANGE_COMMAND_SPEED;
	}

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

	if(isAnimProgressBound() == true) {
		float targetProgress=0;
		if(currSkill->getClass() == scBeBuilt) {
			targetProgress = this->getHpRatio();
		}
		if(currSkill->getClass() == scProduce) {
			targetProgress = this->getProgressRatio();
		}
		if(currSkill->getClass() == scUpgrade) {
			targetProgress = this->getProgressRatio();
		}
		if(currSkill->getClass() == scMorph) {
			targetProgress = this->getProgressRatio();
		}
		if(animProgress<targetProgress){
			float diff=targetProgress-animProgress;
			animProgress=animProgress+diff/(GameConstants::updateFps);
		}
	}
	else {
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

	if(type->getProperty(UnitType::pRotatedClimb)){
		calculateXZRotation();
	}
	else {
		 rotationZ=.0f;
		 rotationX=.0f;
	}

	if(Renderer::getInstance().validateParticleSystemStillExists(this->fire,rsGame) == false) {
		this->fire = NULL;
	}

	if (this->fire != NULL) {
		this->fire->setPos(getCurrVector());
	}
	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		if(Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame) == true) {
			(*it)->setPos(getCurrVector());
			(*it)->setRotation(getRotation());
		}
	}
	for(UnitParticleSystems::iterator it= damageParticleSystems.begin(); it != damageParticleSystems.end(); ++it) {
		if(Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame) == true) {
			(*it)->setPos(getCurrVector());
			(*it)->setRotation(getRotation());
		}
	}

	for(UnitParticleSystems::iterator it= smokeParticleSystems.begin(); it != smokeParticleSystems.end(); ++it) {
		if(Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame) == true) {
			(*it)->setPos(getCurrVector());
			(*it)->setRotation(getRotation());
		}
	}

	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		UnitAttackBoostEffect *effect = currentAttackBoostEffects[i];
		if(effect != NULL && effect->ups != NULL) {
			bool particleValid = Renderer::getInstance().validateParticleSystemStillExists(effect->ups,rsGame);
			if(particleValid == true) {
				effect->ups->setPos(getCurrVector());
				effect->ups->setRotation(getRotation());
			}
		}
	}
	if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
		if(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups != NULL) {
			bool particleValid = Renderer::getInstance().validateParticleSystemStillExists(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups,rsGame);
			if(particleValid == true) {
				currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setPos(getCurrVector());
				currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setRotation(getRotation());
			}
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
		if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.empty() == false && currentAttackBoostOriginatorEffect.skillType != NULL) {
			for(unsigned int i = 0; i < currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size(); ++i) {
				// Remove attack boost upgrades from unit

				int findUnitId = currentAttackBoostOriginatorEffect.currentAttackBoostUnits[i];
				Unit *affectedUnit = game->getWorld()->findUnitById(findUnitId);
				if(affectedUnit != NULL) {
					affectedUnit->deapplyAttackBoost(currentAttackBoostOriginatorEffect.skillType->getAttackBoost(), this);
				}

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
						currentAttackBoostOriginatorEffect.currentAttackBoostUnits.push_back(affectedUnit->getId());

						//printf("+ #1 APPLY ATTACK BOOST to unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
					}
				}
			}

			if(showUnitParticles == true) {
				if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.empty() == false) {
					if(attackBoost->unitParticleSystemTypeForSourceUnit != NULL) {
						currentAttackBoostOriginatorEffect.currentAppliedEffect = new UnitAttackBoostEffect();
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = new UnitParticleSystemType();
						*currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = *attackBoost->unitParticleSystemTypeForSourceUnit;
						//effect.upst = boost->unitParticleSystemTypeForAffectedUnit;

						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups = new UnitParticleSystem(200);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst->setValues(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setPos(getCurrVector());
						if(getFaction()->getTexture()) {
							currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
						}
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

				std::vector<int>::iterator iterFound = std::find(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.begin(), currentAttackBoostOriginatorEffect.currentAttackBoostUnits.end(), affectedUnit->getId());

				if(attackBoost->isAffected(this,affectedUnit) == true) {
					if(iterFound == currentAttackBoostOriginatorEffect.currentAttackBoostUnits.end()) {
						if(affectedUnit->applyAttackBoost(attackBoost, this) == true) {
							currentAttackBoostOriginatorEffect.currentAttackBoostUnits.push_back(affectedUnit->getId());

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
				if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.empty() == false) {
					if( attackBoost->unitParticleSystemTypeForSourceUnit != NULL &&
						currentAttackBoostOriginatorEffect.currentAppliedEffect == NULL) {

						currentAttackBoostOriginatorEffect.currentAppliedEffect = new UnitAttackBoostEffect();
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = new UnitParticleSystemType();
						*currentAttackBoostOriginatorEffect.currentAppliedEffect->upst = *attackBoost->unitParticleSystemTypeForSourceUnit;
						//effect.upst = boost->unitParticleSystemTypeForAffectedUnit;

						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups = new UnitParticleSystem(200);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->upst->setValues(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups);
						currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setPos(getCurrVector());
						if(getFaction()->getTexture()) {
							currentAttackBoostOriginatorEffect.currentAppliedEffect->ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
						}
						Renderer::getInstance().manageParticleSystem(currentAttackBoostOriginatorEffect.currentAppliedEffect->ups, rsGame);

						//printf("+ #2 APPLY ATTACK BOOST SELF PARTICLE to unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId());
					}
				}
				else if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.empty() == true) {
					if(currentAttackBoostOriginatorEffect.currentAppliedEffect != NULL) {
						delete currentAttackBoostOriginatorEffect.currentAppliedEffect;
						currentAttackBoostOriginatorEffect.currentAppliedEffect = NULL;

						//printf("- #2 DE-APPLY ATTACK BOOST SELF PARTICLE to unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId());
					}
				}
			}
		}
	}

	if(return_value) {
		changedActiveCommand = false;
	}

	return return_value;
}

bool Unit::unitHasAttackBoost(const AttackBoost *boost, const Unit *source) const {
	bool result = false;
	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		const UnitAttackBoostEffect *effect = currentAttackBoostEffects[i];
		if( effect != NULL && effect->boost->name == boost->name &&
			effect->source->getType()->getId() == source->getType()->getId()) {
			result = true;
			break;
		}
	}
	return result;
}

bool Unit::applyAttackBoost(const AttackBoost *boost, const Unit *source) {
	if(boost == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: boost == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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
		//printf("APPLYING ATTACK BOOST START to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());

		UnitAttackBoostEffect *effect = new UnitAttackBoostEffect();
		effect->boost = boost;
		effect->source = source;

		bool wasAlive = alive;
		int originalHp = hp;
		int prevMaxHp = totalUpgrade.getMaxHp();
		int prevMaxHpRegen = totalUpgrade.getMaxHpRegeneration();
		//printf("#1 wasAlive = %d hp = %d boosthp = %d\n",wasAlive,hp,boost->boostUpgrade.getMaxHp());

		totalUpgrade.apply(&boost->boostUpgrade, this);

		checkItemInVault(&this->hp,this->hp);
		//hp += boost->boostUpgrade.getMaxHp();
		hp += (totalUpgrade.getMaxHp() - prevMaxHp);
		addItemToVault(&this->hp,this->hp);

		//regenerate hp upgrade / or boost
		if(totalUpgrade.getMaxHpRegeneration() != 0) {
			checkItemInVault(&this->hp,this->hp);

			//printf("BEFORE Apply Hp Regen max = %d, prev = %d, hp = %d\n",totalUpgrade.getMaxHpRegeneration(),prevMaxHpRegen,hp);

			hp += (totalUpgrade.getMaxHpRegeneration() - prevMaxHpRegen);
			//if(hp > type->getTotalMaxHp(&totalUpgrade)) {
			//	hp = type->getTotalMaxHp(&totalUpgrade);
			//}
			addItemToVault(&this->hp,this->hp);

			//printf("AFTER Apply Hp Regen max = %d, prev = %d, hp = %d\n",totalUpgrade.getMaxHpRegeneration(),prevMaxHpRegen,hp);
		}

		checkModelStateInfoForNewHpValue();

		if(originalHp < hp) {
			this->setLastAttackerUnitId(source->getId());
		}
		//printf("#2 wasAlive = %d hp = %d boosthp = %d\n",wasAlive,hp,boost->boostUpgrade.getMaxHp());

		if(showUnitParticles == true) {
			if(boost->unitParticleSystemTypeForAffectedUnit != NULL) {
				effect->upst = new UnitParticleSystemType();
				*effect->upst = *boost->unitParticleSystemTypeForAffectedUnit;
				//effect.upst = boost->unitParticleSystemTypeForAffectedUnit;

				effect->ups = new UnitParticleSystem(200);
				effect->upst->setValues(effect->ups);
				effect->ups->setPos(getCurrVector());
				if(getFaction()->getTexture()) {
					effect->ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
				}
				Renderer::getInstance().manageParticleSystem(effect->ups, rsGame);
			}
		}
		currentAttackBoostEffects.push_back(effect);

		if(wasAlive == true) {
			//startDamageParticles

			if(originalHp > hp) {
				startDamageParticles();
			}

			//stop DamageParticles on death
			if(hp <= 0) {
				alive= false;
				hp=0;
				addItemToVault(&this->hp,this->hp);
				checkModelStateInfoForNewHpValue();

				stopDamageParticles(true);

				this->setLastAttackerUnitId(source->getId());
				this->setCauseOfDeath(ucodAttackBoost);
				Unit::game->getWorld()->getStats()->die(getFactionIndex(),getType()->getCountUnitDeathInStats());
				game->getScriptManager()->onUnitDied(this);

				StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
				if(sound != NULL &&
					(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
					 (game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true))) {
					SoundRenderer::getInstance().playFx(sound);
				}

				if(this->isDead() && this->getCurrSkill()->getClass() != scDie) {
					this->kill();
				}
			}
		}

		//printf("APPLYING ATTACK BOOST END to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());
	}
	return shouldApplyAttackBoost;
}

void Unit::deapplyAttackBoost(const AttackBoost *boost, const Unit *source) {
	if(boost == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: boost == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//printf("DE-APPLYING ATTACK BOOST START to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());

	bool wasAlive = alive;
	int originalHp = hp;
	int prevMaxHp = totalUpgrade.getMaxHp();
	int prevMaxHpRegen = totalUpgrade.getMaxHpRegeneration();
	totalUpgrade.deapply(&boost->boostUpgrade, this);

	checkItemInVault(&this->hp,this->hp);
	//hp -= boost->boostUpgrade.getMaxHp();
	hp -= (prevMaxHp - totalUpgrade.getMaxHp());
	addItemToVault(&this->hp,this->hp);

	//regenerate hp upgrade / or boost
	if(totalUpgrade.getMaxHpRegeneration() != 0) {
		checkItemInVault(&this->hp,this->hp);

		//printf("BEFORE DeApply Hp Regen max = %d, prev = %d, hp = %d\n",totalUpgrade.getMaxHpRegeneration(),prevMaxHpRegen,hp);

		hp -= (totalUpgrade.getMaxHpRegeneration() - prevMaxHpRegen);
		//if(hp > totalUpgrade.getMaxHp()) {
		//	hp = totalUpgrade.getMaxHp();
		//}
		addItemToVault(&this->hp,this->hp);

		//printf("AFTER DeApply Hp Regen max = %d, prev = %d, hp = %d\n",totalUpgrade.getMaxHpRegeneration(),prevMaxHpRegen,hp);
	}

	if(originalHp < hp) {
		this->setLastAttackerUnitId(source->getId());
	}

	if(wasAlive == true) {
		//printf("DE-APPLYING ATTACK BOOST wasalive = true to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());

		//startDamageParticles
		if(originalHp > hp) {
			startDamageParticles();
		}

		//stop DamageParticles on death
		if(hp <= 0) {
			alive= false;
			hp=0;
			addItemToVault(&this->hp,this->hp);

			checkModelStateInfoForNewHpValue();

			stopDamageParticles(true);

			this->setLastAttackerUnitId(source->getId());
			this->setCauseOfDeath(ucodAttackBoost);

			Unit::game->getWorld()->getStats()->die(getFactionIndex(),getType()->getCountUnitDeathInStats());
			game->getScriptManager()->onUnitDied(this);

			StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
			if(sound != NULL &&
				(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
				 (game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true))) {
				SoundRenderer::getInstance().playFx(sound);
			}

			if(this->isDead() && this->getCurrSkill()->getClass() != scDie) {
				this->kill();
			}
		}
	}

	checkModelStateInfoForNewHpValue();

	//printf("DE-APPLYING ATTACK BOOST BEFORE END to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());

	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		UnitAttackBoostEffect *effect = currentAttackBoostEffects[i];
		if(effect != NULL && effect->boost == boost && effect->source == source) {
			delete effect;
			currentAttackBoostEffects.erase(currentAttackBoostEffects.begin() + i);
			break;
		}
	}

	//printf("DE-APPLYING ATTACK BOOST END to unit [%s - %d] from unit [%s - %d]\n",this->getType()->getName().c_str(),this->getId(),source->getType()->getName().c_str(),source->getId());
}

void Unit::tick() {

	if(isAlive()) {
		if(type == NULL) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
			throw megaglest_runtime_error(szBuf);
		}

		//if(this->getType()->getName() == "spearman") printf("Unit [%d - %s] start tick hp = %d\n",this->getId(),this->getType()->getName().c_str(),hp);


		//regenerate hp upgrade / or boost
		if(type->getTotalMaxHpRegeneration(&totalUpgrade) != 0) {
			if( currSkill->getClass() != scBeBuilt){
				if(type->getTotalMaxHpRegeneration(&totalUpgrade) >= 0) {
					checkItemInVault(&this->hp,this->hp);
					hp += type->getTotalMaxHpRegeneration(&totalUpgrade);
					if(hp > type->getTotalMaxHp(&totalUpgrade)) {
						hp = type->getTotalMaxHp(&totalUpgrade);
					}
					addItemToVault(&this->hp,this->hp);

					checkModelStateInfoForNewHpValue();
					//if(this->getType()->getName() == "spearman") printf("tick hp#2 [type->getTotalMaxHpRegeneration(&totalUpgrade)] = %d type->getTotalMaxHp(&totalUpgrade) [%d] newhp = %d\n",type->getTotalMaxHpRegeneration(&totalUpgrade),type->getTotalMaxHp(&totalUpgrade),hp);
				}
				// If we have negative regeneration then check if the unit should die
				else {
					bool decHpResult = decHp(-type->getTotalMaxHpRegeneration(&totalUpgrade));
					if(decHpResult) {
						this->setCauseOfDeath(ucodStarvedRegeneration);

						Unit::game->getWorld()->getStats()->die(getFactionIndex(),getType()->getCountUnitDeathInStats());
						game->getScriptManager()->onUnitDied(this);
					}
					StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
					if(sound != NULL &&
							(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
									(game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true))) {
						SoundRenderer::getInstance().playFx(sound);
					}
				}
			}
		}
		//regenerate hp
		else {
			if(type->getHpRegeneration() >= 0) {
				if( currSkill->getClass() != scBeBuilt){
					checkItemInVault(&this->hp,this->hp);

					hp += type->getHpRegeneration();
					if(hp > type->getTotalMaxHp(&totalUpgrade)) {
						hp = type->getTotalMaxHp(&totalUpgrade);
					}
					addItemToVault(&this->hp,this->hp);

					checkModelStateInfoForNewHpValue();
					//if(this->getType()->getName() == "spearman") printf("tick hp#1 [type->getHpRegeneration()] = %d type->getTotalMaxHp(&totalUpgrade) [%d] newhp = %d\n",type->getHpRegeneration(),type->getTotalMaxHp(&totalUpgrade),hp);
				}
			}
			// If we have negative regeneration then check if the unit should die
			else {
				bool decHpResult = decHp(-type->getHpRegeneration());
				if(decHpResult) {
					this->setCauseOfDeath(ucodStarvedRegeneration);

					Unit::game->getWorld()->getStats()->die(getFactionIndex(),getType()->getCountUnitDeathInStats());
					game->getScriptManager()->onUnitDied(this);
				}
				StaticSound *sound= this->getType()->getFirstStOfClass(scDie)->getSound();
				if(sound != NULL &&
					(this->getFactionIndex() == Unit::game->getWorld()->getThisFactionIndex() ||
					 (game->getWorld()->showWorldForPlayer(game->getWorld()->getThisTeamIndex()) == true))) {
					SoundRenderer::getInstance().playFx(sound);
				}
			}
		}

		//stop DamageParticles
		stopDamageParticles(false);

		checkItemInVault(&this->ep,this->ep);
		//regenerate ep
//		ep += type->getEpRegeneration();
//		if(ep > type->getTotalMaxEp(&totalUpgrade)){
//			ep = type->getTotalMaxEp(&totalUpgrade);
//		}
//		addItemToVault(&this->ep,this->ep);

		//regenerate ep upgrade / or boost
		checkItemInVault(&this->ep,this->ep);
		//regenerate ep
		ep += type->getTotalMaxEpRegeneration(&totalUpgrade);
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
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: currSkill == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: getType() == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	if(ep > getType()->getTotalMaxEp(&totalUpgrade)){
        ep = getType()->getTotalMaxEp(&totalUpgrade);
	}
	addItemToVault(&this->ep,this->ep);

    return false;
}

bool Unit::repair(){

	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
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

	checkModelStateInfoForNewHpValue();

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

	checkModelStateInfoForNewHpValue();

	if(type == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: type == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//startDamageParticles
	startDamageParticles();

	//stop DamageParticles on death
    if(hp <= 0) {
		alive= false;
        hp=0;
    	addItemToVault(&this->hp,this->hp);

    	checkModelStateInfoForNewHpValue();

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
			str+= "\n#" + intToStr(i + 1) + " " + ct->getName(true);
			++it;
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

	//str += "\n"+lang.get("Hp")+ ": " + intToStr(hp) + "/" + intToStr(type->getTotalMaxHp(&totalUpgrade)) + " [" + floatToStr(getHpRatio()) + "] [" + floatToStr(animProgress) + "]";
	str += "\n"+lang.get("Hp")+ ": " + intToStr(hp) + "/" + intToStr(type->getTotalMaxHp(&totalUpgrade));
	if(type->getHpRegeneration() != 0 || totalUpgrade.getMaxHpRegeneration() != 0) {
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(type->getHpRegeneration());
		if(totalUpgrade.getMaxHpRegeneration() != 0) {
			str+= "+" + intToStr(totalUpgrade.getMaxHpRegeneration());
		}
		str+= ")";
	}

	//ep
	if(getType()->getMaxEp()!=0){
		str+= "\n" + lang.get("Ep")+ ": " + intToStr(ep) + "/" + intToStr(type->getTotalMaxEp(&totalUpgrade));
	}
	if(type->getEpRegeneration() != 0 || totalUpgrade.getMaxEpRegeneration() != 0) {
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(type->getEpRegeneration());
		if(totalUpgrade.getMaxEpRegeneration() != 0) {
			str += "+" + intToStr(totalUpgrade.getMaxEpRegeneration());
		}
		str+= ")";
	}

	//armor
	str+= "\n" + lang.get("Armor")+ ": " + intToStr(getType()->getArmor());
	if(totalUpgrade.getArmor()!=0){
		str+="+"+intToStr(totalUpgrade.getArmor());
	}
	str+= " ("+getType()->getArmorType()->getName(true)+")";

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
			str+= " (" + nextLevel->getName(true) + ": " + intToStr(nextLevel->getKills()) + ")";
		}
	}

	//str+= "\nskl: "+scToStr(currSkill->getClass());

	//load
	if(loadCount!=0){
		str+= "\n" + lang.get("Load")+ ": " + intToStr(loadCount) +"  " + loadType->getName(true);
	}

	//consumable production
	for(int i=0; i < getType()->getCostCount(); ++i) {
		const Resource *r= getType()->getCost(i);
		if(r->getType()->getClass() == rcConsumable) {
			str+= "\n";
			str+= r->getAmount() < 0 ? lang.get("Produce")+": ": lang.get("Consume")+": ";
			str+= intToStr(abs(r->getAmount())) + " " + r->getType()->getName(true);
		}
	}

	//command info
    if(commands.empty() == false) {
		str+= "\n" + commands.front()->getCommandType()->getName(true);
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
				str+= intToStr(r->getAmount()) + " " + r->getType()->getName(true);
			}
		}
	}

    return str;
}

void Unit::applyUpgrade(const UpgradeType *upgradeType){
	if(upgradeType == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: upgradeType == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	if(upgradeType->isAffected(type)){
		totalUpgrade.sum(upgradeType, this);

		checkItemInVault(&this->hp,this->hp);
		hp += upgradeType->getMaxHp();
		hp = max(0,hp);
		addItemToVault(&this->hp,this->hp);

		checkModelStateInfoForNewHpValue();
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

	checkUnitLevel();
}

void Unit::checkUnitLevel() {
	const Level *nextLevel= getNextLevel();
	if(nextLevel != NULL && enemyKills >= nextLevel->getKills()) {
		level= nextLevel;
		int maxHp= totalUpgrade.getMaxHp();
		totalUpgrade.incLevel(type);

		checkItemInVault(&this->hp,this->hp);
		hp += totalUpgrade.getMaxHp() - maxHp;
		addItemToVault(&this->hp,this->hp);

		checkModelStateInfoForNewHpValue();
	}
}

void Unit::morphAttackBoosts(Unit *unit) {
	// Remove any units that were previously in range
	if(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.empty() == false && currentAttackBoostOriginatorEffect.skillType != NULL) {
		for(int i = currentAttackBoostOriginatorEffect.currentAttackBoostUnits.size() - 1; i >= 0; --i) {
			// Remove attack boost upgrades from unit

			int findUnitId = currentAttackBoostOriginatorEffect.currentAttackBoostUnits[i];
			Unit *affectedUnit = game->getWorld()->findUnitById(findUnitId);
			if(affectedUnit != NULL && affectedUnit->getId() == unit->getId()) {
				affectedUnit->deapplyAttackBoost(currentAttackBoostOriginatorEffect.skillType->getAttackBoost(), this);

				currentAttackBoostOriginatorEffect.currentAttackBoostUnits.erase(currentAttackBoostOriginatorEffect.currentAttackBoostUnits.begin() + i);
			}

			//printf("- #1 DE-APPLY ATTACK BOOST from unit [%s - %d]\n",affectedUnit->getType()->getName().c_str(),affectedUnit->getId());
		}
	}
}

bool Unit::morph(const MorphCommandType *mct){

	if(mct == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: mct == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	const UnitType *morphUnitType= mct->getMorphUnit();

	if(morphUnitType == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: morphUnitType == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

    Field morphUnitField=fLand;
    if(morphUnitType->getField(fAir)) morphUnitField=fAir;
    if(morphUnitType->getField(fLand)) morphUnitField=fLand;
    map->clearUnitCells(this, pos, false);
    if(map->isFreeCellsOrHasUnit(pos, morphUnitType->getSize(), morphUnitField, this,morphUnitType)) {
		map->clearUnitCells(this, pos, true);
		faction->deApplyStaticCosts(type,mct);

		//printf("Now unapply attack-boost for unit [%d - %s]\n",this->getId(),this->getType()->getName().c_str());
		// De apply attack boosts for morphed unit
		for(int i = currentAttackBoostEffects.size() - 1; i >= 0; --i) {
			UnitAttackBoostEffect *effect = currentAttackBoostEffects[i];
			if(effect != NULL) {
				Unit *sourceUnit = game->getWorld()->findUnitById(effect->source->getId());
				sourceUnit->morphAttackBoosts(this);
			}
		}


		checkItemInVault(&this->hp,this->hp);
		hp += morphUnitType->getMaxHp() - type->getMaxHp();
		addItemToVault(&this->hp,this->hp);

		checkModelStateInfoForNewHpValue();

		type= morphUnitType;
		currField=morphUnitField;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(morphUnitType, mct->getDiscount());
		faction->addStore(type);
		faction->applyStaticProduction(morphUnitType,mct);

		level= NULL;
		checkUnitLevel();

		return true;
	}
	else{
		return false;
	}
}


// ==================== PRIVATE ====================

float Unit::computeHeight(const Vec2i &pos) const{
	//printf("CRASHING FOR UNIT: %d alive = %d\n",this->getId(),this->isAlive());
	//printf("[%s]\n",this->getType()->getName().c_str());
	if(map->isInside(pos) == false || map->isInsideSurface(map->toSurfCoords(pos)) == false) {
		//printf("CRASHING FOR UNIT: %d [%s] alive = %d\n",this->getId(),this->getType()->getName().c_str(),this->isAlive());
		//abort();
		throw megaglest_runtime_error("#7 Invalid path position = " + pos.getString());
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
		targetRotation= radToDeg(streflop::atan2(static_cast<streflop::Simple>(relPosf.x), static_cast<streflop::Simple>(relPosf.y)));
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

		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);

		delete commands.back();
		commands.pop_back();

		safeMutex.ReleaseLock();
	}
	changedActiveCommand = false;
}

void Unit::deleteQueuedCommand(Command *command) {
	if(getCurrCommand() == command)	{
		this->setCurrentUnitTitle("");
		this->unitPath->clear();
	}
	undoCommand(command);
	delete command;
}


std::pair<CommandResult,string> Unit::checkCommand(Command *command) const {
	std::pair<CommandResult,string> result(crSuccess,"");

	if(command == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//if not operative or has not command type => fail
	if(isOperative() == false ||
       command->getUnit() == this ||
       getType()->hasCommandType(command->getCommandType()) == false ||
       (ignoreCheckCommand == false && this->getFaction()->reqsOk(command->getCommandType()) == false)) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] isOperative() = %d, command->getUnit() = %p, getType()->hasCommandType(command->getCommandType()) = %d, this->getFaction()->reqsOk(command->getCommandType()) = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__, __LINE__,isOperative(),command->getUnit(),getType()->hasCommandType(command->getCommandType()),this->getFaction()->reqsOk(command->getCommandType()));

		// Allow self healing if able to heal own unit type
		if(	command->getUnit() == this &&
			command->getCommandType()->getClass() == ccRepair &&
			this->getType()->getFirstRepairCommand(this->getType()) != NULL) {

		}
		else {
			//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);

			result.first = crFailUndefined;
			return result;
		}
	}

	//if pos is not inside the world (if comand has not a pos, pos is (0, 0) and is inside world
	if(map->isInside(command->getPos()) == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__, __LINE__);
		//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);

		result.first = crFailUndefined;
		return result;
	}

	//check produced
	if(command->getCommandType() == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced != NULL) {
		if(ignoreCheckCommand == false && faction->reqsOk(produced) == false) {
			//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);
			//printf("To produce this unit you need:\n%s\n",produced->getUnitAndUpgradeReqDesc().c_str());
			result.first = crFailReqs;

			Lang &lang= Lang::getInstance();
			result.second = " - " + lang.get("Reqs") + " : " + produced->getUnitAndUpgradeReqDesc(false);
			return result;
		}

		if(ignoreCheckCommand == false &&
				faction->checkCosts(produced,command->getCommandType()) == false) {
			//printf("To produce this unit you need:\n%s\n",produced->getResourceReqDesc().c_str());
			//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);
			result.first = crFailRes;
			Lang &lang= Lang::getInstance();
			result.second = " - " + lang.get("Reqs") + " : " + produced->getResourceReqDesc(false);
			return result;
		}
	}

    //build command specific, check resources and requirements for building
    if(command->getCommandType()->getClass() == ccBuild) {
		const UnitType *builtUnit= command->getUnitType();

		if(builtUnit == NULL) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: builtUnit == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
			throw megaglest_runtime_error(szBuf);
		}

		if(faction->reqsOk(builtUnit) == false) {
			//printf("To build this unit you need:\n%s\n",builtUnit->getUnitAndUpgradeReqDesc().c_str());
			//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);
			result.first = crFailReqs;
			Lang &lang= Lang::getInstance();
			result.second = " - " + lang.get("Reqs") + " : " + builtUnit->getUnitAndUpgradeReqDesc(false);
			return result;
		}
		if(faction->checkCosts(builtUnit,NULL) == false) {
			//printf("To build this unit you need:\n%s\n",builtUnit->getResourceReqDesc().c_str());
			//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);
			result.first = crFailRes;
			Lang &lang= Lang::getInstance();
			result.second = " - " + lang.get("Reqs") + " : " + builtUnit->getResourceReqDesc(false);
			return result;
		}
    }
    //upgrade command specific, check that upgrade is not upgraded
    else if(command->getCommandType()->getClass() == ccUpgrade) {
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

		if(uct == NULL) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
			throw megaglest_runtime_error(szBuf);
		}

		if(faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())){
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__, __LINE__);
			//printf("In [%s::%s Line: %d] command = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,command);
			result.first = crFailUndefined;
			return result;
		}
	}

    return result;
}

void Unit::applyCommand(Command *command){
	if(command == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}
	else if(command->getCommandType() == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//check produced
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL) {
		faction->applyCosts(produced,command->getCommandType());
	}

    //build command specific
    if(command->getCommandType()->getClass()==ccBuild){
		faction->applyCosts(command->getUnitType(),command->getCommandType());
    }
    //upgrade command specific
    else if(command->getCommandType()->getClass()==ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

		if(uct == NULL) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
			throw megaglest_runtime_error(szBuf);
		}

        faction->startUpgrade(uct->getProducedUpgrade());
	}
}

CommandResult Unit::undoCommand(Command *command){

	if(command == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: command == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}
	else if(command->getCommandType() == NULL) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: command->getCommandType() == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
		throw megaglest_runtime_error(szBuf);
	}

	if(getCurrCommand() == command && command->getCommandType()->getClass()==ccMorph
			&& this->currSkill->getClass() == scMorph ){
		// clear cells of morphed unit and set those of current unit!
		map->clearUnitCells(this, this->getPos());
		map->putUnitCells(this, this->getPos(),true);
	}
	//return cost
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->deApplyCosts(produced,command->getCommandType());
	}

	//return building cost if not already building it or dead
	if(command->getCommandType()->getClass() == ccBuild){
		if(currSkill->getClass() != scBuild && currSkill->getClass() != scDie) {
			faction->deApplyCosts(command->getUnitType(),command->getCommandType());
		}
	}

	//upgrade command cancel from list
	if(command->getCommandType()->getClass() == ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());
        if(uct == NULL) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s Line: %d] ERROR: uct == NULL, Unit = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,this->toString().c_str());
			throw megaglest_runtime_error(szBuf);
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

		if(Renderer::getInstance().validateParticleSystemStillExists(this->fire,rsGame) == false) {
			this->fire = NULL;
		}

		// stop fire
		if(this->fire != NULL) {
			this->fire->fade();
			this->fire = NULL;
		}
		// stop additional particles


		if(smokeParticleSystems.empty() == false) {
			//printf("Checking to stop smokeparticles for unit [%s - %d] hp = %d\n",this->getType()->getName().c_str(),this->getId(),hp);

			for(int i = smokeParticleSystems.size()-1; i >= 0; --i) {
				UnitParticleSystem *ps = smokeParticleSystems[i];
				if(Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
					ps->fade();
				}
				smokeParticleSystems.pop_back();
			}
		}

		if(damageParticleSystems.empty() == false) {
			//printf("Checking to stop damageparticles for unit [%s - %d] hp = %d\n",this->getType()->getName().c_str(),this->getId(),hp);

			for(int i = damageParticleSystems.size()-1; i >= 0; --i) {
				UnitParticleSystem *ps = damageParticleSystems[i];
				UnitParticleSystemType *pst = NULL;
				int foundParticleIndexType = -2;
				if(Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
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
				}
				if(force == true || ( pst !=NULL && pst->getMinmaxEnabled() == false )) {
					damageParticleSystemsInUse.erase(foundParticleIndexType);
					if(Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
						ps->fade();
					}
					damageParticleSystems.pop_back();
				}
			}
		}
	}

	checkCustomizedParticleTriggers(force);
}

void Unit::checkCustomizedParticleTriggers(bool force) {
	// Now check if we have special hp triggered particles
	if(damageParticleSystems.empty() == false) {
		for(int i = damageParticleSystems.size()-1; i >= 0; --i) {
			UnitParticleSystem *ps = damageParticleSystems[i];
			UnitParticleSystemType *pst = NULL;
			int foundParticleIndexType = -2;
			if(Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
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
					if(Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
						ps->fade();
					}
					damageParticleSystems.pop_back();
				}
			}
		}
	}

	// Now check if we have special hp triggered particles
	//start additional particles
	if(showUnitParticles &&
		type->damageParticleSystemTypes.empty() == false  &&
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
					if(getFaction()->getTexture()) {
						ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
					}
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
		if( showUnitParticles &&
			type->damageParticleSystemTypes.empty() == false ) {
			for(unsigned int i = 0; i < type->damageParticleSystemTypes.size(); ++i) {
				UnitParticleSystemType *pst = type->damageParticleSystemTypes[i];

				if(pst->getMinmaxEnabled() == false && damageParticleSystemsInUse.find(i) == damageParticleSystemsInUse.end()) {
					UnitParticleSystem *ups = new UnitParticleSystem(200);
					pst->setValues(ups);
					ups->setPos(getCurrVector());
					if(getFaction()->getTexture()) {
						ups->setFactionColor(getFaction()->getTexture()->getPixmapConst()->getPixel3f(0,0));
					}
					damageParticleSystems.push_back(ups);
					damageParticleSystemsInUse[i] = ups;
					Renderer::getInstance().manageParticleSystem(ups, rsGame);
				}
			}
		}

		// start fire
		if(type->getProperty(UnitType::pBurnable) && this->fire == NULL) {
			FireParticleSystem *fps = new FireParticleSystem(200);
			const Game *game = Renderer::getInstance().getGame();
			fps->setSpeed(2.5f / game->getWorld()->getUpdateFps(this->getFactionIndex()));
			fps->setPos(getCurrVector());
			fps->setRadius(type->getSize()/3.f);
			fps->setTexture(CoreData::getInstance().getFireTexture());
			fps->setParticleSize(type->getSize()/3.f);
			this->fire= fps;
			fireParticleSystems.push_back(fps);

			Renderer::getInstance().manageParticleSystem(fps, rsGame);
			if(showUnitParticles == true) {
				// smoke
				UnitParticleSystem *ups= new UnitParticleSystem(400);
				ups->setColorNoEnergy(Vec4f(0.0f, 0.0f, 0.0f, 0.13f));
				ups->setColor(Vec4f(0.115f, 0.115f, 0.115f, 0.22f));
				ups->setPos(getCurrVector());
				ups->setBlendMode(ups->strToBlendMode("black"));
				ups->setOffset(Vec3f(0,2,0));
				ups->setDirection(Vec3f(0,1,-0.2f));
				ups->setRadius(type->getSize()/3.f);
				ups->setShape(Shared::Graphics::UnitParticleSystem::sLinear);
				ups->setTexture(CoreData::getInstance().getFireTexture());
				const Game *game = Renderer::getInstance().getGame();
				ups->setSpeed(2.0f / game->getWorld()->getUpdateFps(this->getFactionIndex()));
				ups->setGravity(0.0004f);
				ups->setEmissionRate(1);
				ups->setMaxParticleEnergy(150);
				ups->setSizeNoEnergy(type->getSize()*0.6f);
				ups->setParticleSize(type->getSize()*0.8f);
				smokeParticleSystems.push_back(ups);
				//damageParticleSystemsInUse[-1] = ups;
				Renderer::getInstance().manageParticleSystem(ups, rsGame);
			}
		}
	}

	checkCustomizedParticleTriggers(false);
}

void Unit::setTargetVec(const Vec3f &targetVec)	{
	this->targetVec= targetVec;
	logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
}

void Unit::setMeetingPos(const Vec2i &meetingPos) {
	this->meetingPos= meetingPos;
	map->clampPos(this->meetingPos);

	if(map->isInside(this->meetingPos) == false || map->isInsideSurface(map->toSurfCoords(this->meetingPos)) == false) {
		throw megaglest_runtime_error("#8 Invalid path position = " + this->meetingPos.getString());
	}

	logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
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
			throw megaglest_runtime_error("game == NULL");
		}
		else if(game->getWorld() == NULL) {
			throw megaglest_runtime_error("game->getWorld() == NULL");
		}

		game->getWorld()->exploreCells(newPos, sightRange, teamIndex);
	}
}

void Unit::logSynchData(string file,int line,string source) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {

	    char szBuf[8096]="";

	    snprintf(szBuf,8096,
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

void Unit::cleanupOldBadHarvestPos() {
	const int cleanupInterval = (GameConstants::updateFps * 5);
	bool needToCleanup = (getFrameCount() % cleanupInterval == 0);

	//printf("========================> cleanupOldBadHarvestPos() [%d] badHarvestPosList.size [%ld] cleanupInterval [%d] getFrameCount() [%d] needToCleanup [%d]\n",getFrameCount(),badHarvestPosList.size(),cleanupInterval,getFrameCount(),needToCleanup);

	if(needToCleanup == true) {
		//printf("========================> cleanupOldBadHarvestPos() [%d] badHarvestPosList.size [%ld]\n",getFrameCount(),badHarvestPosList.size());

		std::vector<Vec2i> purgeList;
		for(std::map<Vec2i,int>::iterator iter = badHarvestPosList.begin(); iter != badHarvestPosList.end(); ++iter) {
			if(getFrameCount() - iter->second >= cleanupInterval) {
				//printf("cleanupOldBadHarvestPos() [%d][%d]\n",getFrameCount(),iter->second);
				purgeList.push_back(iter->first);
			}
		}

		if(purgeList.empty() == false) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"[cleaning old bad harvest targets] purgeList.size() [" MG_SIZE_T_SPECIFIER "]",purgeList.size());
			logSynchData(extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,szBuf);

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

string Unit::getUniquePickName() const {
	string result = intToStr(id) + " - " + type->getName() + " : ";
	result += pos.getString();
	return result;
}

Vec2i Unit::getPos() {
	Vec2i result;

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(mutexCommands,mutexOwnerId);
	result = this->pos;
	safeMutex.ReleaseLock();

	return result;
}

void Unit::clearCaches() {
	cachedFow.alphaList.clear();
	cachedFow.surfPosList.clear();
	cachedFowPos = Vec2i(0,0);

	//unitPath->clearCaches();

	lastHarvestedResourcePos = Vec2i(0,0);
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
	result += " progress = " + floatToStr(this->progress,16);
	result += "\n";
	result += " lastAnimProgress = " + floatToStr(this->lastAnimProgress,16);
	result += " animProgress = " + floatToStr(this->animProgress,16);
	result += " highlight = " + floatToStr(this->highlight,16);
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

	result += " lastRotation = " + floatToStr(this->lastRotation,16);
	result += " targetRotation = " + floatToStr(this->targetRotation,16);
	result += " rotation = " + floatToStr(this->rotation,16);

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

    result += "random = " + intToStr(random.getLastNumber()) + "\n";
    result += "pathFindRefreshCellCount = " + intToStr(pathFindRefreshCellCount) + "\n";

	return result;
}

void Unit::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *unitNode = rootNode->addChild("Unit");

//	const int id;
	unitNode->addAttribute("id",intToStr(id), mapTagReplacements);
	// For info purposes only
	unitNode->addAttribute("name",type->getName(), mapTagReplacements);
//    int hp;
	unitNode->addAttribute("hp",intToStr(hp), mapTagReplacements);
//    int ep;
	unitNode->addAttribute("ep",intToStr(ep), mapTagReplacements);
//    int loadCount;
	unitNode->addAttribute("loadCount",intToStr(loadCount), mapTagReplacements);
//    int deadCount;
	unitNode->addAttribute("deadCount",intToStr(deadCount), mapTagReplacements);
//    float progress;			//between 0 and 1
	unitNode->addAttribute("progress",floatToStr(progress,16), mapTagReplacements);
//	float lastAnimProgress;	//between 0 and 1
	unitNode->addAttribute("lastAnimProgress",floatToStr(lastAnimProgress,16), mapTagReplacements);
//	float animProgress;		//between 0 and 1
	unitNode->addAttribute("animProgress",floatToStr(animProgress,16), mapTagReplacements);
//	float highlight;
	unitNode->addAttribute("highlight",floatToStr(highlight,16), mapTagReplacements);
//	int progress2;
	unitNode->addAttribute("progress2",intToStr(progress2), mapTagReplacements);
//	int kills;
	unitNode->addAttribute("kills",intToStr(kills), mapTagReplacements);
//	int enemyKills;
	unitNode->addAttribute("enemyKills",intToStr(enemyKills), mapTagReplacements);
//	UnitReference targetRef;
	targetRef.saveGame(unitNode);
//
//	Field currField;
	unitNode->addAttribute("currField",intToStr(currField), mapTagReplacements);
//    Field targetField;
	unitNode->addAttribute("targetField",intToStr(targetField), mapTagReplacements);
//	const Level *level;
	if(level != NULL) {
		level->saveGame(unitNode);
	}
//    Vec2i pos;
	unitNode->addAttribute("pos",pos.getString(), mapTagReplacements);
//	Vec2i lastPos;
	unitNode->addAttribute("lastPos",lastPos.getString(), mapTagReplacements);
//    Vec2i targetPos;		//absolute target pos
	unitNode->addAttribute("targetPos",targetPos.getString(), mapTagReplacements);
//	Vec3f targetVec;
	unitNode->addAttribute("targetVec",targetVec.getString(), mapTagReplacements);
//	Vec2i meetingPos;
	unitNode->addAttribute("meetingPos",meetingPos.getString(), mapTagReplacements);
//
//	float lastRotation;		//in degrees
	unitNode->addAttribute("lastRotation",floatToStr(lastRotation,16), mapTagReplacements);
//	float targetRotation;
	unitNode->addAttribute("targetRotation",floatToStr(targetRotation,16), mapTagReplacements);
//	float rotation;
	unitNode->addAttribute("rotation",floatToStr(rotation,16), mapTagReplacements);
//	float targetRotationZ;
	unitNode->addAttribute("targetRotationZ",floatToStr(targetRotationZ,16), mapTagReplacements);
//	float targetRotationX;
	unitNode->addAttribute("targetRotationX",floatToStr(targetRotationX,16), mapTagReplacements);
//	float rotationZ;
	unitNode->addAttribute("rotationZ",floatToStr(rotationZ,16), mapTagReplacements);
//	float rotationX;
	unitNode->addAttribute("rotationX",floatToStr(rotationX,16), mapTagReplacements);
//    const UnitType *type;
	unitNode->addAttribute("type",type->getName(), mapTagReplacements);
//    const ResourceType *loadType;
	if(loadType != NULL) {
		unitNode->addAttribute("loadType",loadType->getName(), mapTagReplacements);
	}
//    const SkillType *currSkill;
	if(currSkill != NULL) {
		unitNode->addAttribute("currSkillName",currSkill->getName(), mapTagReplacements);
		unitNode->addAttribute("currSkillClass",intToStr(currSkill->getClass()), mapTagReplacements);
	}
//    int lastModelIndexForCurrSkillType;
	unitNode->addAttribute("lastModelIndexForCurrSkillType",intToStr(lastModelIndexForCurrSkillType), mapTagReplacements);
//    int animationRandomCycleCount;
	unitNode->addAttribute("animationRandomCycleCount",intToStr(animationRandomCycleCount), mapTagReplacements);
//
//    bool toBeUndertaken;
	unitNode->addAttribute("toBeUndertaken",intToStr(toBeUndertaken), mapTagReplacements);
//	bool alive;
	unitNode->addAttribute("alive",intToStr(alive), mapTagReplacements);
//	bool showUnitParticles;
	unitNode->addAttribute("showUnitParticles",intToStr(showUnitParticles), mapTagReplacements);
//    Faction *faction;
//	ParticleSystem *fire;
	int linkFireIndex = -1;
	if(this->fire != NULL && Renderer::getInstance().validateParticleSystemStillExists(this->fire,rsGame) == true) {
		//fire->saveGame(unitNode);
		bool fireInSystemList = false;
		if(fireParticleSystems.empty() == false) {
			for(unsigned int i = 0; i < fireParticleSystems.size(); ++i) {
				ParticleSystem *ps= fireParticleSystems[i];
				if(ps == this->fire) {
					linkFireIndex = i;
					fireInSystemList = true;
					break;
				}
			}
		}
		if(fireInSystemList == false) {
			this->fire->saveGame(unitNode);
		}
	}
//	TotalUpgrade totalUpgrade;
	totalUpgrade.saveGame(unitNode);
//	Map *map;
//
//	UnitPathInterface *unitPath;
	unitPath->saveGame(unitNode);
//	WaypointPath waypointPath;
//
//    Commands commands;
	for(Commands::iterator it = commands.begin(); it != commands.end(); ++it) {
		(*it)->saveGame(unitNode,faction);
	}
//	Observers observers;
	//for(Observers::iterator it = observers.begin(); it != observers.end(); ++it) {
	//	(*it)->saveGame(unitNode);
	//}

//	vector<UnitParticleSystem*> unitParticleSystems;
	if(unitParticleSystems.empty() == false) {
		XmlNode *unitParticleSystemsNode = unitNode->addChild("unitParticleSystems");

		for(unsigned int i = 0; i < unitParticleSystems.size(); ++i) {
			UnitParticleSystem *ups= unitParticleSystems[i];
			if(ups != NULL && Renderer::getInstance().validateParticleSystemStillExists(ups,rsGame) == true) {
				ups->saveGame(unitParticleSystemsNode);
			}
		}
	}
//	vector<UnitParticleSystemType*> queuedUnitParticleSystemTypes;
	if(queuedUnitParticleSystemTypes.empty() == false) {
		XmlNode *queuedUnitParticleSystemTypesNode = unitNode->addChild("queuedUnitParticleSystemTypes");
		for(unsigned int i = 0; i < queuedUnitParticleSystemTypes.size(); ++i) {
			UnitParticleSystemType *upst= queuedUnitParticleSystemTypes[i];
			if(upst != NULL) {
				upst->saveGame(queuedUnitParticleSystemTypesNode);
			}
		}
	}
//	UnitParticleSystems damageParticleSystems;
	if(damageParticleSystems.empty() == false) {
		XmlNode *damageParticleSystemsNode = unitNode->addChild("damageParticleSystems");
		for(unsigned int i = 0; i < damageParticleSystems.size(); ++i) {
			UnitParticleSystem *ups= damageParticleSystems[i];
			if(ups != NULL && Renderer::getInstance().validateParticleSystemStillExists(ups,rsGame) == true) {
				ups->saveGame(damageParticleSystemsNode);
			}
		}
	}
//	std::map<int, UnitParticleSystem *> damageParticleSystemsInUse;
	if(damageParticleSystemsInUse.empty() == false) {
		XmlNode *damageParticleSystemsInUseNode = unitNode->addChild("damageParticleSystemsInUse");

		for(std::map<int, UnitParticleSystem *>::const_iterator iterMap = damageParticleSystemsInUse.begin();
				iterMap != damageParticleSystemsInUse.end(); ++iterMap) {
			if(iterMap->second != NULL && Renderer::getInstance().validateParticleSystemStillExists(iterMap->second,rsGame) == true) {
				XmlNode *damageParticleSystemsInUseNode2 = damageParticleSystemsInUseNode->addChild("damageParticleSystemsInUse");

				damageParticleSystemsInUseNode2->addAttribute("key",intToStr(iterMap->first), mapTagReplacements);
				iterMap->second->saveGame(damageParticleSystemsInUseNode2);
			}
		}
	}

//	vector<ParticleSystem*> fireParticleSystems;
	if(fireParticleSystems.empty() == false) {
		XmlNode *fireParticleSystemsNode = unitNode->addChild("fireParticleSystems");

		if(linkFireIndex >= 0) {
			fireParticleSystemsNode->addAttribute("fireParticleLink",intToStr(linkFireIndex), mapTagReplacements);
		}

		for(unsigned int i = 0; i < fireParticleSystems.size(); ++i) {
			ParticleSystem *ps= fireParticleSystems[i];
			if(ps != NULL && Renderer::getInstance().validateParticleSystemStillExists(ps,rsGame) == true) {
				ps->saveGame(fireParticleSystemsNode);
			}
		}
	}

//	vector<UnitParticleSystem*> smokeParticleSystems;
	if(smokeParticleSystems.empty() == false) {
		XmlNode *smokeParticleSystemsNode = unitNode->addChild("smokeParticleSystems");
		for(unsigned int i = 0; i < smokeParticleSystems.size(); ++i) {
			UnitParticleSystem *ups= smokeParticleSystems[i];
			if(ups != NULL && Renderer::getInstance().validateParticleSystemStillExists(ups,rsGame) == true) {
				ups->saveGame(smokeParticleSystemsNode);
				//printf("Saving smoke particles:\n[%s]\n",ups->toString().c_str());
			}
		}
	}

//	CardinalDir modelFacing;
	unitNode->addAttribute("modelFacing",intToStr(modelFacing), mapTagReplacements);

//	std::string lastSynchDataString;
	unitNode->addAttribute("lastSynchDataString",lastSynchDataString, mapTagReplacements);
//	std::string lastFile;
	unitNode->addAttribute("lastFile",lastFile, mapTagReplacements);
//	int lastLine;
	unitNode->addAttribute("lastLine",intToStr(lastLine), mapTagReplacements);
//	std::string lastSource;
	unitNode->addAttribute("lastSource",lastSource, mapTagReplacements);
//	int lastRenderFrame;
	unitNode->addAttribute("lastRenderFrame",intToStr(lastRenderFrame), mapTagReplacements);
//	bool visible;
	unitNode->addAttribute("visible",intToStr(visible), mapTagReplacements);
//	int retryCurrCommandCount;
	unitNode->addAttribute("retryCurrCommandCount",intToStr(retryCurrCommandCount), mapTagReplacements);
//	Vec3f screenPos;
	unitNode->addAttribute("screenPos",screenPos.getString(), mapTagReplacements);
//	string currentUnitTitle;
	unitNode->addAttribute("currentUnitTitle",currentUnitTitle, mapTagReplacements);
//
//	bool inBailOutAttempt;
	unitNode->addAttribute("inBailOutAttempt",intToStr(inBailOutAttempt), mapTagReplacements);
//	//std::vector<std::pair<Vec2i,Chrono> > badHarvestPosList;
//	std::map<Vec2i,int> badHarvestPosList;
	for(std::map<Vec2i,int>::const_iterator iterMap = badHarvestPosList.begin();
			iterMap != badHarvestPosList.end(); ++iterMap) {
		XmlNode *badHarvestPosListNode = unitNode->addChild("badHarvestPosList");

		badHarvestPosListNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);
		badHarvestPosListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

//	//time_t lastBadHarvestListPurge;
//	std::pair<Vec2i,int> lastHarvestResourceTarget;
	XmlNode *lastHarvestResourceTargetNode = unitNode->addChild("lastHarvestResourceTarget");
	lastHarvestResourceTargetNode->addAttribute("key",lastHarvestResourceTarget.first.getString(), mapTagReplacements);
	lastHarvestResourceTargetNode->addAttribute("value",intToStr(lastHarvestResourceTarget.second), mapTagReplacements);

//	//std::pair<Vec2i,std::vector<Vec2i> > currentTargetPathTaken;
//	static Game *game;
//
//	bool ignoreCheckCommand;
	unitNode->addAttribute("ignoreCheckCommand",intToStr(ignoreCheckCommand), mapTagReplacements);
//	uint32 lastStuckFrame;
	unitNode->addAttribute("lastStuckFrame",intToStr(lastStuckFrame), mapTagReplacements);
//	Vec2i lastStuckPos;
	unitNode->addAttribute("lastStuckPos",lastStuckPos.getString(), mapTagReplacements);
//	uint32 lastPathfindFailedFrame;
	unitNode->addAttribute("lastPathfindFailedFrame",intToStr(lastPathfindFailedFrame), mapTagReplacements);
//	Vec2i lastPathfindFailedPos;
	unitNode->addAttribute("lastPathfindFailedPos",lastPathfindFailedPos.getString(), mapTagReplacements);
//	bool usePathfinderExtendedMaxNodes;
	unitNode->addAttribute("usePathfinderExtendedMaxNodes",intToStr(usePathfinderExtendedMaxNodes), mapTagReplacements);
//	int maxQueuedCommandDisplayCount;
	unitNode->addAttribute("maxQueuedCommandDisplayCount",intToStr(maxQueuedCommandDisplayCount), mapTagReplacements);
//	UnitAttackBoostEffectOriginator currentAttackBoostOriginatorEffect;
	currentAttackBoostOriginatorEffect.saveGame(unitNode);
//	std::vector<UnitAttackBoostEffect *> currentAttackBoostEffects;
	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
		UnitAttackBoostEffect *uabe= currentAttackBoostEffects[i];
		if(uabe != NULL) {
			uabe->saveGame(unitNode);
		}
	}

//	Mutex *mutexCommands;
//
//	//static Mutex mutexDeletedUnits;
//	//static std::map<void *,bool> deletedUnits;
//
//	bool changedActiveCommand;
	unitNode->addAttribute("changedActiveCommand",intToStr(changedActiveCommand), mapTagReplacements);
//	int lastAttackerUnitId;
	unitNode->addAttribute("lastAttackerUnitId",intToStr(lastAttackerUnitId), mapTagReplacements);
//	int lastAttackedUnitId;
	unitNode->addAttribute("lastAttackedUnitId",intToStr(lastAttackedUnitId), mapTagReplacements);
//	CauseOfDeathType causeOfDeath;
	unitNode->addAttribute("causeOfDeath",intToStr(causeOfDeath), mapTagReplacements);

	//pathfindFailedConsecutiveFrameCount
	unitNode->addAttribute("pathfindFailedConsecutiveFrameCount",intToStr(pathfindFailedConsecutiveFrameCount), mapTagReplacements);

	unitNode->addAttribute("currentPathFinderDesiredFinalPos",currentPathFinderDesiredFinalPos.getString(), mapTagReplacements);

	unitNode->addAttribute("random",intToStr(random.getLastNumber()), mapTagReplacements);
	unitNode->addAttribute("pathFindRefreshCellCount",intToStr(pathFindRefreshCellCount), mapTagReplacements);
}

Unit * Unit::loadGame(const XmlNode *rootNode, GameSettings *settings, Faction *faction, World *world) {
	const XmlNode *unitNode = rootNode;

	int newUnitId = unitNode->getAttribute("id")->getIntValue();
	Vec2i newUnitPos = Vec2i::strToVec2(unitNode->getAttribute("pos")->getValue());
	string newUnitType = unitNode->getAttribute("type")->getValue();
	const UnitType *ut = faction->getType()->getUnitType(newUnitType);
	CardinalDir newModelFacing = static_cast<CardinalDir>(unitNode->getAttribute("modelFacing")->getIntValue());

//	Unit *result = new Unit(int id, UnitPathInterface *unitpath, const Vec2i &pos,
//			   const UnitType *type, Faction *faction, Map *map, CardinalDir placeFacing) : BaseColorPickEntity(), id(id) {

	UnitPathInterface *newpath = NULL;
	switch(settings->getPathFinderType()) {
		case pfBasic:
			newpath = new UnitPathBasic();
			break;
		default:
			throw megaglest_runtime_error("detected unsupported pathfinder type!");
    }

	newpath->loadGame(unitNode);
	//Unit *result = new Unit(getNextUnitId(f), newpath, Vec2i(0), ut, f, &map, CardinalDir::NORTH);
	//Unit(int id, UnitPathInterface *path, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, CardinalDir placeFacing);
	Unit *result = new Unit(newUnitId, newpath, newUnitPos, ut, faction, world->getMapPtr(), newModelFacing);

	result->lastRotation = unitNode->getAttribute("lastRotation")->getFloatValue();
	result->targetRotation = unitNode->getAttribute("targetRotation")->getFloatValue();
	result->rotation = unitNode->getAttribute("rotation")->getFloatValue();

	//world->placeUnitAtLocation(newUnitPos, generationArea, unit, true);
	//result->setPos(newUnitPos);
    //Vec2i meetingPos = newUnitPos-Vec2i(1);
    //result->setMeetingPos(meetingPos);
	result->pos = newUnitPos;
	result->lastPos = Vec2i::strToVec2(unitNode->getAttribute("lastPos")->getValue());
	result->meetingPos = Vec2i::strToVec2(unitNode->getAttribute("meetingPos")->getValue());
	// Attempt to improve performance
	//result->exploreCells();
	//result->calculateFogOfWarRadius();
    // --------------------------

    result->hp = unitNode->getAttribute("hp")->getIntValue();
//    int ep;
	result->ep = unitNode->getAttribute("ep")->getIntValue();
//    int loadCount;
	result->loadCount = unitNode->getAttribute("loadCount")->getIntValue();
//    int deadCount;
	result->deadCount = unitNode->getAttribute("deadCount")->getIntValue();
//    float progress;			//between 0 and 1
	result->progress = unitNode->getAttribute("progress")->getFloatValue();
//	float lastAnimProgress;	//between 0 and 1
	result->lastAnimProgress = unitNode->getAttribute("lastAnimProgress")->getFloatValue();
//	float animProgress;		//between 0 and 1
	result->animProgress = unitNode->getAttribute("animProgress")->getFloatValue();
//	float highlight;
	result->highlight = unitNode->getAttribute("highlight")->getFloatValue();
//	int progress2;
	result->progress2 = unitNode->getAttribute("progress2")->getIntValue();
//	int kills;
	result->kills = unitNode->getAttribute("kills")->getIntValue();
//	int enemyKills;
	result->enemyKills = unitNode->getAttribute("enemyKills")->getIntValue();
//	UnitReference targetRef;
//	targetRef.saveGame(unitNode);
	result->targetRef.loadGame(unitNode,world);
//
//	Field currField;
	result->currField = static_cast<Field>(unitNode->getAttribute("currField")->getIntValue());
//    Field targetField;
	result->targetField = static_cast<Field>(unitNode->getAttribute("targetField")->getIntValue());
//	const Level *level;
//	if(level != NULL) {
//		level->saveGame(unitNode);
//	}
	result->level = Level::loadGame(unitNode,ut);
//    Vec2i pos;
	result->pos = Vec2i::strToVec2(unitNode->getAttribute("pos")->getValue());
//	Vec2i lastPos;
	result->lastPos = Vec2i::strToVec2(unitNode->getAttribute("lastPos")->getValue());
//    Vec2i targetPos;		//absolute target pos
	result->targetPos = Vec2i::strToVec2(unitNode->getAttribute("targetPos")->getValue());
//	Vec3f targetVec;
	result->targetVec = Vec3f::strToVec3(unitNode->getAttribute("targetVec")->getValue());
//	Vec2i meetingPos;
	result->meetingPos = Vec2i::strToVec2(unitNode->getAttribute("meetingPos")->getValue());
//
//	float lastRotation;		//in degrees
	result->lastRotation = unitNode->getAttribute("lastRotation")->getFloatValue();
//	float targetRotation;
	result->targetRotation = unitNode->getAttribute("targetRotation")->getFloatValue();
//	float rotation;
	result->rotation = unitNode->getAttribute("rotation")->getFloatValue();
//	float targetRotationZ;
	result->targetRotationZ = unitNode->getAttribute("targetRotationZ")->getFloatValue();
//	float targetRotationX;
	result->targetRotationX = unitNode->getAttribute("targetRotationX")->getFloatValue();
//	float rotationZ;
	result->rotationZ = unitNode->getAttribute("rotationZ")->getFloatValue();
//	float rotationX;
	result->rotationX = unitNode->getAttribute("rotationX")->getFloatValue();
//    const UnitType *type;
//	unitNode->addAttribute("type",type->getName(), mapTagReplacements);
//    const ResourceType *loadType;
//	if(loadType != NULL) {
//		unitNode->addAttribute("loadType",loadType->getName(), mapTagReplacements);
//	}
	if(unitNode->hasAttribute("loadType") == true) {
		string loadTypeName = unitNode->getAttribute("loadType")->getValue();
		result->loadType = world->getTechTree()->getResourceType(loadTypeName);
	}
//    const SkillType *currSkill;
//	if(currSkill != NULL) {
//		unitNode->addAttribute("currSkill",currSkill->getName(), mapTagReplacements);
//	}
	if(unitNode->hasAttribute("currSkillName") == true) {
		string skillTypeName = unitNode->getAttribute("currSkillName")->getValue();
		SkillClass skillClass = static_cast<SkillClass>(unitNode->getAttribute("currSkillClass")->getIntValue());
		result->currSkill = ut->getSkillType(skillTypeName,skillClass);
	}

//    int lastModelIndexForCurrSkillType;
	result->lastModelIndexForCurrSkillType = unitNode->getAttribute("lastModelIndexForCurrSkillType")->getIntValue();
//    int animationRandomCycleCount;
	result->animationRandomCycleCount = unitNode->getAttribute("animationRandomCycleCount")->getIntValue();
//
//    bool toBeUndertaken;
	result->toBeUndertaken = unitNode->getAttribute("toBeUndertaken")->getIntValue() != 0;
//	bool alive;
	result->alive = unitNode->getAttribute("alive")->getIntValue() != 0;
//	bool showUnitParticles;
	result->showUnitParticles = unitNode->getAttribute("showUnitParticles")->getIntValue() != 0;
//    Faction *faction;
//	ParticleSystem *fire;
//	if(fire != NULL) {
//		fire->saveGame(unitNode);
//	}
	if(unitNode->hasChild("FireParticleSystem") == true) {
		XmlNode *fireNode = unitNode->getChild("FireParticleSystem");
		result->fire = new FireParticleSystem();
		result->fire->loadGame(fireNode);
		//result->fire->setTexture(CoreData::getInstance().getFireTexture());
		result->fireParticleSystems.push_back(result->fire);

		//Renderer::getInstance().manageParticleSystem(result->fire, rsGame);
		Renderer::getInstance().addToDeferredParticleSystemList(make_pair(result->fire, rsGame));
	}

//	TotalUpgrade totalUpgrade;
	result->totalUpgrade.loadGame(unitNode);
//	Map *map;
//
//	UnitPathInterface *unitPath;
//	unitPath->saveGame(unitNode);
//	WaypointPath waypointPath;
//
//    Commands commands;
//	for(Commands::iterator it = commands.begin(); it != commands.end(); ++it) {
//		(*it)->saveGame(unitNode);
//	}
	vector<XmlNode *> commandNodeList = unitNode->getChildList("Command");
	for(unsigned int i = 0; i < commandNodeList.size(); ++i) {
		XmlNode *node = commandNodeList[i];
		Command *command = Command::loadGame(node,ut,world);

		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(result->mutexCommands,mutexOwnerId);
		result->commands.push_back(command);
		safeMutex.ReleaseLock();
	}
//	Observers observers;
	//for(Observers::iterator it = observers.begin(); it != observers.end(); ++it) {
	//	(*it)->saveGame(unitNode);
	//}

//	vector<UnitParticleSystem*> unitParticleSystems;
//	for(unsigned int i = 0; i < unitParticleSystems.size(); ++i) {
//		UnitParticleSystem *ups= unitParticleSystems[i];
//		ups->saveGame(unitNode);
//	}
	if(unitNode->hasChild("unitParticleSystems") == true) {
		XmlNode *unitParticleSystemsNode = unitNode->getChild("unitParticleSystems");
		vector<XmlNode *> unitParticleSystemNodeList = unitParticleSystemsNode->getChildList("UnitParticleSystem");
		for(unsigned int i = 0; i < unitParticleSystemNodeList.size(); ++i) {
			XmlNode *node = unitParticleSystemNodeList[i];

			UnitParticleSystem *ups = new UnitParticleSystem();
			ups->loadGame(node);
			result->unitParticleSystems.push_back(ups);

			//Renderer::getInstance().manageParticleSystem(result->fire, rsGame);
			Renderer::getInstance().addToDeferredParticleSystemList(make_pair(ups, rsGame));
		}
	}

//	vector<UnitParticleSystemType*> queuedUnitParticleSystemTypes;
//	for(unsigned int i = 0; i < queuedUnitParticleSystemTypes.size(); ++i) {
//		UnitParticleSystemType *upst= queuedUnitParticleSystemTypes[i];
//		upst->saveGame(unitNode);
//	}

//	UnitParticleSystems damageParticleSystems;
//	for(unsigned int i = 0; i < damageParticleSystems.size(); ++i) {
//		UnitParticleSystem *ups= damageParticleSystems[i];
//		ups->saveGame(unitNode);
//	}
	if(unitNode->hasChild("damageParticleSystems") == true) {
		XmlNode *damageParticleSystemsNode = unitNode->getChild("damageParticleSystems");
		vector<XmlNode *> unitParticleSystemNodeList = damageParticleSystemsNode->getChildList("UnitParticleSystem");
		for(unsigned int i = 0; i < unitParticleSystemNodeList.size(); ++i) {
			XmlNode *node = unitParticleSystemNodeList[i];

			UnitParticleSystem *ups = new UnitParticleSystem();
			ups->loadGame(node);
			result->damageParticleSystems.push_back(ups);
			result->damageParticleSystemsInUse[i]=ups;

			//Renderer::getInstance().manageParticleSystem(result->fire, rsGame);
			Renderer::getInstance().addToDeferredParticleSystemList(make_pair(ups, rsGame));
		}
	}

//	std::map<int, UnitParticleSystem *> damageParticleSystemsInUse;
//	for(std::map<int, UnitParticleSystem *>::const_iterator iterMap = damageParticleSystemsInUse.begin();
//			iterMap != damageParticleSystemsInUse.end(); ++iterMap) {
//		XmlNode *damageParticleSystemsInUseNode = unitNode->addChild("damageParticleSystemsInUse");
//
//		damageParticleSystemsInUseNode->addAttribute("key",intToStr(iterMap->first), mapTagReplacements);
//		iterMap->second->saveGame(damageParticleSystemsInUseNode);
//	}
//	if(unitNode->hasChild("damageParticleSystemsInUse") == true) {
//		XmlNode *damageParticleSystemsInUseNode = unitNode->getChild("damageParticleSystemsInUse");
//		vector<XmlNode *> damageParticleSystemsInUseNode2 = damageParticleSystemsInUseNode->getChildList("damageParticleSystemsInUse");
//		for(unsigned int i = 0; i < damageParticleSystemsInUseNode2.size(); ++i) {
//			XmlNode *d2Node = damageParticleSystemsInUseNode2[i];
//
//			vector<XmlNode *> unitParticleSystemNodeList = damageParticleSystemsInUseNode->getChildList("UnitParticleSystem");
//			for(unsigned int i = 0; i < unitParticleSystemNodeList.size(); ++i) {
//				XmlNode *node = unitParticleSystemNodeList[i];
//
//				UnitParticleSystem *ups = new UnitParticleSystem();
//				ups->loadGame(node);
//				result->unitParticleSystems.push_back(ups);
//
//				//Renderer::getInstance().manageParticleSystem(result->fire, rsGame);
//				Renderer::getInstance().addToDeferredParticleSystemList(make_pair(ups, rsGame));
//			}
//
//	}

//	vector<ParticleSystem*> fireParticleSystems;
//	for(unsigned int i = 0; i < fireParticleSystems.size(); ++i) {
//		ParticleSystem *ps= fireParticleSystems[i];
//		ps->saveGame(unitNode);
//	}
	if(unitNode->hasChild("fireParticleSystems") == true) {
		XmlNode *fireParticleSystemsNode = unitNode->getChild("fireParticleSystems");

		int linkFireIndex = -1;
		if(fireParticleSystemsNode->hasAttribute("fireParticleLink") == true) {
			linkFireIndex = fireParticleSystemsNode->getAttribute("fireParticleLink")->getIntValue();
		}

		vector<XmlNode *> unitParticleSystemNodeList = fireParticleSystemsNode->getChildList("FireParticleSystem");
		for(unsigned int i = 0; i < unitParticleSystemNodeList.size(); ++i) {
			XmlNode *node = unitParticleSystemNodeList[i];

			FireParticleSystem *ups = new FireParticleSystem();
			ups->loadGame(node);
			//ups->setTexture(CoreData::getInstance().getFireTexture());
			result->fireParticleSystems.push_back(ups);

			if(linkFireIndex >= 0 && linkFireIndex == i) {
				result->fire = ups;
			}
			//Renderer::getInstance().manageParticleSystem(result->fire, rsGame);
			Renderer::getInstance().addToDeferredParticleSystemList(make_pair(ups, rsGame));
		}
	}

//	vector<UnitParticleSystem*> smokeParticleSystems;
//	for(unsigned int i = 0; i < smokeParticleSystems.size(); ++i) {
//		UnitParticleSystem *ups= smokeParticleSystems[i];
//		ups->saveGame(unitNode);
//	}
	if(unitNode->hasChild("smokeParticleSystems") == true) {
		XmlNode *smokeParticleSystemsNode = unitNode->getChild("smokeParticleSystems");
		vector<XmlNode *> unitParticleSystemNodeList = smokeParticleSystemsNode->getChildList("UnitParticleSystem");
		for(unsigned int i = 0; i < unitParticleSystemNodeList.size(); ++i) {
			XmlNode *node = unitParticleSystemNodeList[i];

//			printf("Load Smoke particle i = %d\n",i);
			UnitParticleSystem *ups = new UnitParticleSystem();
			ups->loadGame(node);
			//ups->setTexture(CoreData::getInstance().getFireTexture());
			result->smokeParticleSystems.push_back(ups);

//			UnitParticleSystem *ups= new UnitParticleSystem(400);
//			ups->setColorNoEnergy(Vec4f(0.0f, 0.0f, 0.0f, 0.13f));
//			ups->setColor(Vec4f(0.115f, 0.115f, 0.115f, 0.22f));
//			ups->setPos(result->getCurrVector());
//			ups->setBlendMode(ups->strToBlendMode("black"));
//			ups->setOffset(Vec3f(0,2,0));
//			ups->setDirection(Vec3f(0,1,-0.2f));
//			ups->setRadius(result->type->getSize()/3.f);
//			ups->setShape(Shared::Graphics::UnitParticleSystem::sLinear);
//			ups->setTexture(CoreData::getInstance().getFireTexture());
//			const Game *game = Renderer::getInstance().getGame();
//			ups->setSpeed(2.0f / game->getWorld()->getUpdateFps(result->getFactionIndex()));
//			ups->setGravity(0.0004f);
//			ups->setEmissionRate(1);
//			ups->setMaxParticleEnergy(150);
//			ups->setSizeNoEnergy(result->type->getSize()*0.6f);
//			ups->setParticleSize(result->type->getSize()*0.8f);
//			result->smokeParticleSystems.push_back(ups);

			//Renderer::getInstance().manageParticleSystem(result->fire, rsGame);
			Renderer::getInstance().addToDeferredParticleSystemList(make_pair(ups, rsGame));

			//printf("Loading smoke particles:\n[%s]\n",ups->toString().c_str());
		}
	}

//	CardinalDir modelFacing;
//	unitNode->addAttribute("modelFacing",intToStr(modelFacing), mapTagReplacements);

//	std::string lastSynchDataString;
//	unitNode->addAttribute("lastSynchDataString",lastSynchDataString, mapTagReplacements);
//	std::string lastFile;
//	unitNode->addAttribute("lastFile",lastFile, mapTagReplacements);
//	int lastLine;
//	unitNode->addAttribute("lastLine",intToStr(lastLine), mapTagReplacements);
//	std::string lastSource;
//	unitNode->addAttribute("lastSource",lastSource, mapTagReplacements);
//	int lastRenderFrame;
	result->lastRenderFrame = unitNode->getAttribute("lastRenderFrame")->getIntValue();
//	bool visible;
	result->visible = unitNode->getAttribute("visible")->getIntValue() != 0;
//	int retryCurrCommandCount;
	result->retryCurrCommandCount = unitNode->getAttribute("retryCurrCommandCount")->getIntValue();
//	Vec3f screenPos;
	result->screenPos = Vec3f::strToVec3(unitNode->getAttribute("screenPos")->getValue());
//	string currentUnitTitle;
	result->currentUnitTitle = unitNode->getAttribute("currentUnitTitle")->getValue();
//
//	bool inBailOutAttempt;
	result->inBailOutAttempt = unitNode->getAttribute("inBailOutAttempt")->getIntValue() != 0;
//	//std::vector<std::pair<Vec2i,Chrono> > badHarvestPosList;
//	std::map<Vec2i,int> badHarvestPosList;
//	for(std::map<Vec2i,int>::const_iterator iterMap = badHarvestPosList.begin();
//			iterMap != badHarvestPosList.end(); ++iterMap) {
//		XmlNode *badHarvestPosListNode = unitNode->addChild("badHarvestPosList");
//
//		badHarvestPosListNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);
//		badHarvestPosListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
//	}

//	//time_t lastBadHarvestListPurge;
//	std::pair<Vec2i,int> lastHarvestResourceTarget;
	const XmlNode *lastHarvestResourceTargetNode = unitNode->getChild("lastHarvestResourceTarget");
//	lastHarvestResourceTargetNode->addAttribute("key",lastHarvestResourceTarget.first.getString(), mapTagReplacements);
//	lastHarvestResourceTargetNode->addAttribute("value",intToStr(lastHarvestResourceTarget.second), mapTagReplacements);

	result->lastHarvestResourceTarget =
			make_pair(Vec2i::strToVec2(lastHarvestResourceTargetNode->getAttribute("key")->getValue()),
					                  lastHarvestResourceTargetNode->getAttribute("value")->getIntValue());

//	//std::pair<Vec2i,std::vector<Vec2i> > currentTargetPathTaken;
//	static Game *game;
//
//	bool ignoreCheckCommand;
	result->ignoreCheckCommand = unitNode->getAttribute("ignoreCheckCommand")->getIntValue() != 0;
//	uint32 lastStuckFrame;
	result->lastStuckFrame = unitNode->getAttribute("lastStuckFrame")->getIntValue();
//	Vec2i lastStuckPos;
	result->lastStuckPos = Vec2i::strToVec2(unitNode->getAttribute("lastStuckPos")->getValue());
//	uint32 lastPathfindFailedFrame;
	result->lastPathfindFailedFrame = unitNode->getAttribute("lastPathfindFailedFrame")->getIntValue();
//	Vec2i lastPathfindFailedPos;
	result->lastPathfindFailedPos = Vec2i::strToVec2(unitNode->getAttribute("lastPathfindFailedPos")->getValue());
//	bool usePathfinderExtendedMaxNodes;
	result->usePathfinderExtendedMaxNodes = unitNode->getAttribute("usePathfinderExtendedMaxNodes")->getIntValue() != 0;
//	int maxQueuedCommandDisplayCount;
	result->maxQueuedCommandDisplayCount = unitNode->getAttribute("maxQueuedCommandDisplayCount")->getIntValue();
//	UnitAttackBoostEffectOriginator currentAttackBoostOriginatorEffect;
//	currentAttackBoostOriginatorEffect.saveGame(unitNode);
//	std::vector<UnitAttackBoostEffect *> currentAttackBoostEffects;
//	for(unsigned int i = 0; i < currentAttackBoostEffects.size(); ++i) {
//		UnitAttackBoostEffect *uabe= currentAttackBoostEffects[i];
//		uabe->saveGame(unitNode);
//	}

//	Mutex *mutexCommands;
//
//	//static Mutex mutexDeletedUnits;
//	//static std::map<void *,bool> deletedUnits;
//
//	bool changedActiveCommand;
	result->changedActiveCommand = unitNode->getAttribute("changedActiveCommand")->getIntValue() != 0;
//	int lastAttackerUnitId;
	result->lastAttackerUnitId = unitNode->getAttribute("lastAttackerUnitId")->getIntValue();
//	int lastAttackedUnitId;
	result->lastAttackedUnitId = unitNode->getAttribute("lastAttackedUnitId")->getIntValue();
//	CauseOfDeathType causeOfDeath;
	result->causeOfDeath = static_cast<CauseOfDeathType>(unitNode->getAttribute("causeOfDeath")->getIntValue());

	result->pathfindFailedConsecutiveFrameCount = unitNode->getAttribute("pathfindFailedConsecutiveFrameCount")->getIntValue();

	if(result->alive) {
		world->getMapPtr()->putUnitCells(result, newUnitPos);
		//result->born();
	}

	result->pos = newUnitPos;
	result->lastPos = Vec2i::strToVec2(unitNode->getAttribute("lastPos")->getValue());
	result->meetingPos = Vec2i::strToVec2(unitNode->getAttribute("meetingPos")->getValue());

	if(unitNode->hasAttribute("currentPathFinderDesiredFinalPos")) {
		result->currentPathFinderDesiredFinalPos = Vec2i::strToVec2(unitNode->getAttribute("currentPathFinderDesiredFinalPos")->getValue());
	}

	if(unitNode->hasAttribute("random")) {
		result->random.setLastNumber(unitNode->getAttribute("random")->getIntValue());
	}
	if(unitNode->hasAttribute("pathFindRefreshCellCount")) {
		result->pathFindRefreshCellCount = unitNode->getAttribute("pathFindRefreshCellCount")->getIntValue();
	}

	result->calculateFogOfWarRadius();

    return result;
}

}}//end namespace
