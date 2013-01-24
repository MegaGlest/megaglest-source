// ==============================================================
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

#include "gui.h"

#include <cassert>
#include <algorithm>

#include "world.h"
#include "renderer.h"
#include "game.h"
#include "upgrade.h"
#include "unit.h"
#include "metrics.h"
#include "display.h"
#include "platform_util.h"
#include "sound_renderer.h"
#include "util.h"
#include "faction.h"
#include "leak_dumper.h"

#include "network_types.h"
#include "network_manager.h"


using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Mouse3d
// =====================================================

const float Mouse3d::fadeSpeed= 1.f/50.f;

static const int queueCommandKey = vkShift;

Mouse3d::Mouse3d(){
	enabled= false;
	rot= 0;
	fade= 0.f;
}

void Mouse3d::enable(){
	enabled= true;
	fade= 0.f;
}

void Mouse3d::update(){
	if(enabled){
		rot= (rot + 3) % 360;
		fade+= fadeSpeed;
		if(fade>1.f) fade= 1.f;
	}
}

// ===============================
// 	class SelectionQuad
// ===============================

SelectionQuad::SelectionQuad(){
	enabled= false;
	posDown= Vec2i(0);
	posUp= Vec2i(0);
}

void SelectionQuad::setPosDown(const Vec2i &posDown){
	enabled= true;
	this->posDown= posDown;
	this->posUp= posDown;
}

void SelectionQuad::setPosUp(const Vec2i &posUp){
	this->posUp= posUp;
}

void SelectionQuad::disable(){
	enabled= false;
}

// =====================================================
// 	class Gui
// =====================================================

//constructor
Gui::Gui(){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	lastGroupRecall = -1;
    posObjWorld= Vec2i(54, 14);
	validPosObjWorld= false;
    activeCommandType= NULL;
    activeCommandClass= ccStop;
	selectingBuilding= false;
	selectedBuildingFacing = CardinalDir::NORTH;
	selectingPos= false;
	selectingMeetingPoint= false;
	activePos= invalidPos;
	lastPosDisplay= invalidPos;
	lastQuadCalcFrame=0;
	selectionCalculationFrameSkip=10;
	minQuadSize=20;
	selectedResourceObjectPos=Vec2i(-1,-1);
	highlightedResourceObjectPos=Vec2i(-1,-1);
	highlightedUnitId=-1;
	hudTexture=NULL;
	commander=NULL;
	world=NULL;
	game=NULL;
	gameCamera=NULL;
	console=NULL;
	choosenBuildingType=NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void Gui::init(Game *game){
	this->commander= game->getCommander();
	this->gameCamera= game->getGameCameraPtr();
	this->console= game->getConsole();
	this->world= game->getWorld();
	this->game=game;
	selection.init(this, world->getThisFactionIndex(), world->getThisTeamIndex());
}

void Gui::end(){
	selection.clear();
	if(hudTexture != NULL) {
		Renderer::getInstance().endTexture(rsGlobal, hudTexture, false);
	}
	hudTexture = NULL;
}

// ==================== get ====================

const UnitType *Gui::getBuilding() const{
    assert(selectingBuilding);
    return choosenBuildingType;
}
const Object *Gui::getSelectedResourceObject()	const{
	if(selectedResourceObjectPos.x==-1){
		return NULL;
	}
	else {
		return world->getMap()->getSurfaceCell(selectedResourceObjectPos)->getObject();
	}
}
Object *Gui::getHighlightedResourceObject()	const{
	if(highlightedResourceObjectPos.x==-1){
		return NULL;
	}
	else {
		return world->getMap()->getSurfaceCell(highlightedResourceObjectPos)->getObject();
	}
}

Unit* Gui::getHighlightedUnit()	const{
	if(highlightedUnitId==-1){
		return NULL;
	}
	else {
		return world->findUnitById(highlightedUnitId);
	}
}

// ==================== is ====================

bool Gui::isPlacingBuilding() const{
	return isSelectingPos() && activeCommandType!=NULL && activeCommandType->getClass()==ccBuild;
}

// ==================== set ====================

void Gui::invalidatePosObjWorld(){
    validPosObjWorld= false;
}

// ==================== reset state ====================

void Gui::resetState(){
    selectingBuilding= false;
	selectedBuildingFacing = CardinalDir::NORTH;
	selectingPos= false;
	selectingMeetingPoint= false;
    activePos= invalidPos;
	activeCommandClass= ccStop;
	activeCommandType= NULL;
}

// ==================== events ====================

void Gui::update(){

    if(selectionQuad.isEnabled() && selectionQuad.getPosUp().dist(selectionQuad.getPosDown())>minQuadSize){
    	computeSelected(false,false);
    }
	mouse3d.update();
}

void Gui::tick(){
	computeDisplay();
}

//in display coords
bool Gui::mouseValid(int x, int y) {
	return computePosDisplay(x, y) != invalidPos;
}

void Gui::mouseDownLeftDisplay(int x, int y) {
	if(!selectingPos && !selectingMeetingPoint) {
		int posDisplay= computePosDisplay(x, y);
		if(posDisplay!= invalidPos) {
			if(selection.isCommandable()) {
				if(selectingBuilding) {
					mouseDownDisplayUnitBuild(posDisplay);
				}
				else {
					mouseDownDisplayUnitSkills(posDisplay);
				}
			}
			else {
				resetState();
			}
		}
		computeDisplay();
	}
}

void Gui::mouseMoveDisplay(int x, int y) {
	computeInfoString(computePosDisplay(x, y));
}

void Gui::mouseMoveOutsideDisplay() {
	computeInfoString(invalidPos);
}

void Gui::mouseDownLeftGraphics(int x, int y, bool prepared) {
	if(selectingPos) {
		//give standard orders
		Vec2i targetPos;
		if(Renderer::getInstance().computePosition(Vec2i(x, y), targetPos) &&
			world->getMap()->isInsideSurface(world->getMap()->toSurfCoords(targetPos)) == true) {
			giveTwoClickOrders(x, y, prepared);
		}
		resetState();
	}
	//set meeting point
	else if(selectingMeetingPoint) {
		if(selection.isCommandable()) {
			Vec2i targetPos;
			if(Renderer::getInstance().computePosition(Vec2i(x, y), targetPos) &&
				world->getMap()->isInsideSurface(world->getMap()->toSurfCoords(targetPos)) == true) {
				commander->trySetMeetingPoint(selection.getFrontUnit(), targetPos);
			}
		}
		resetState();
	}
	else {
		selectionQuad.setPosDown(Vec2i(x, y));
		computeSelected(false,false);
	}
	computeDisplay();
}

void Gui::mouseDownRightGraphics(int x, int y , bool prepared) {
	if(selectingPos || selectingMeetingPoint) {
		resetState();
	}
	else if(selection.isCommandable()) {
		if(prepared) {
			Vec2i targetPos;
			if(Renderer::getInstance().computePosition(Vec2i(x, y), targetPos) &&
				world->getMap()->isInsideSurface(world->getMap()->toSurfCoords(targetPos)) == true) {
				givePreparedDefaultOrders(x, y);
			}
		}
		else {
			Vec2i targetPos;
			if(Renderer::getInstance().computePosition(Vec2i(x, y), targetPos) &&
				world->getMap()->isInsideSurface(world->getMap()->toSurfCoords(targetPos)) == true) {
				giveDefaultOrders(x, y);
			}
		}
	}
	computeDisplay();
}

void Gui::mouseUpLeftGraphics(int x, int y) {
	if(!selectingPos && !selectingMeetingPoint) {
		if(selectionQuad.isEnabled()){
			selectionQuad.setPosUp(Vec2i(x, y));
			if(selectionQuad.getPosUp().dist(selectionQuad.getPosDown()) > minQuadSize) {
				computeSelected(false,true);
			}
			if(selection.isCommandable() && random.randRange(0, 1)==0){
				SoundRenderer::getInstance().playFx(
					selection.getFrontUnit()->getType()->getSelectionSound(),
					selection.getFrontUnit()->getCurrVector(),
					gameCamera->getPos());
			}
			selectionQuad.disable();
		}
	}
}

void Gui::mouseMoveGraphics(int x, int y) {
	//compute selection
	if(selectionQuad.isEnabled()){
		selectionQuad.setPosUp(Vec2i(x, y));
		computeSelected(false,false);
	}

	//compute position for building
	if(isPlacingBuilding()){
		validPosObjWorld= Renderer::getInstance().computePosition(Vec2i(x,y), posObjWorld);
	}

	display.setInfoText("");
}

void Gui::mouseDoubleClickLeftGraphics(int x, int y){
	if(!selectingPos && !selectingMeetingPoint){
		selectionQuad.setPosDown(Vec2i(x, y));
		computeSelected(true,true);
		computeDisplay();
	}
}

void Gui::groupKey(int groupIndex) {
	if(isKeyDown(vkControl)){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] groupIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,groupIndex);

		selection.assignGroup(groupIndex);
	}
	else{
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] groupIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,groupIndex);

		Config &config = Config::getInstance();
		int recallGroupCenterCameraTimeout = config.getInt("RecallGroupCenterCameraTimeoutMilliseconds","1500");
		if(lastGroupRecall == groupIndex &&
			lastGroupRecallTime.getMillis() > 0 &&
			lastGroupRecallTime.getMillis() <= recallGroupCenterCameraTimeout) {
			centerCameraOnSelection();
		}
		else {
			selection.recallGroup(groupIndex);
		}
		lastGroupRecallTime.start();
		lastGroupRecall = groupIndex;
	}
}

void Gui::hotKey(SDL_KeyboardEvent key) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] key = [%c][%d]\n",__FILE__,__FUNCTION__,key,key);

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	//if(key == configKeys.getCharKey("HotKeyCenterCameraOnSelection")) {
	if(isKeyPressed(configKeys.getSDLKey("HotKeyCenterCameraOnSelection"),key) == true) {
		centerCameraOnSelection();
	}
	//else if(key == configKeys.getCharKey("HotKeySelectIdleHarvesterUnit")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeySelectIdleHarvesterUnit"),key) == true) {
		selectInterestingUnit(iutIdleHarvester);
	}
	//else if(key == configKeys.getCharKey("HotKeySelectBuiltBuilding")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeySelectBuiltBuilding"),key) == true) {
		selectInterestingUnit(iutBuiltBuilding);
	}
	//else if(key == configKeys.getCharKey("HotKeyDumpWorldToLog")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeyDumpWorldToLog"),key) == true) {
		std::string worldLog = world->DumpWorldToLog();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] worldLog dumped to [%s]\n",__FILE__,__FUNCTION__,__LINE__,worldLog.c_str());
	}
	//else if(key == configKeys.getCharKey("HotKeyRotateUnitDuringPlacement")){
	else if(isKeyPressed(configKeys.getSDLKey("HotKeyRotateUnitDuringPlacement"),key) == true) {
	    // Here the user triggers a unit rotation while placing a unit
	    if(isPlacingBuilding()) {
	    	if(getBuilding()->getRotationAllowed()){
				++selectedBuildingFacing;
	    	}
	    }
	}
	//else if(key == configKeys.getCharKey("HotKeySelectDamagedUnit")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeySelectDamagedUnit"),key) == true) {
		selectInterestingUnit(iutDamaged);
	}
	//else if(key == configKeys.getCharKey("HotKeySelectStoreUnit")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeySelectStoreUnit"),key) == true) {
		selectInterestingUnit(iutStore);
	}
	//else if(key == configKeys.getCharKey("HotKeySelectedUnitsAttack")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeySelectedUnitsAttack"),key) == true) {
		clickCommonCommand(ccAttack);
	}
	//else if(key == configKeys.getCharKey("HotKeySelectedUnitsStop")) {
	else if(isKeyPressed(configKeys.getSDLKey("HotKeySelectedUnitsStop"),key) == true) {
		clickCommonCommand(ccStop);
	}
}

void Gui::switchToNextDisplayColor(){
	display.switchColor();
}

void Gui::onSelectionChanged(){
	resetState();
	computeDisplay();
}

// ================= PRIVATE =================

void Gui::giveOneClickOrders(){
	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	std::pair<CommandResult,string> result(crFailUndefined,"");
	bool queueKeyDown = isKeyDown(queueCommandKey);
	if(selection.isUniform()){
		result= commander->tryGiveCommand(&selection, activeCommandType, Vec2i(0), (Unit*)NULL,  queueKeyDown);
	}
	else{
		result= commander->tryGiveCommand(&selection, activeCommandClass, Vec2i(0), (Unit*)NULL, queueKeyDown);
	}
	addOrdersResultToConsole(activeCommandClass, result);
    activeCommandType= NULL;
    activeCommandClass= ccStop;
}

void Gui::giveDefaultOrders(int x, int y) {
	//compute target
	//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	const Unit *targetUnit= NULL;
	Vec2i targetPos;
	if(computeTarget(Vec2i(x, y), targetPos, targetUnit) == false) {
		console->addStdMessage("InvalidPosition");
		return;
	}
	giveDefaultOrders(targetPos.x,targetPos.y,targetUnit,true);
	//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Gui::givePreparedDefaultOrders(int x, int y){
	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	giveDefaultOrders(x, y, NULL,false);
}

void Gui::giveDefaultOrders(int x, int y,const Unit *targetUnit, bool paintMouse3d) {
	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	bool queueKeyDown = isKeyDown(queueCommandKey);
	Vec2i targetPos=Vec2i(x, y);

	//give order
	//printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	std::pair<CommandResult,string> result= commander->tryGiveCommand(&selection, targetPos, targetUnit, queueKeyDown);

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result.first == crSuccess || result.first == crSomeFailed) {
		if(paintMouse3d)
			mouse3d.enable();

		if(random.randRange(0, 1)==0){
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(),
				gameCamera->getPos());
		}
	}

	//reset
	resetState();
}

void Gui::giveTwoClickOrders(int x, int y , bool prepared) {
	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	std::pair<CommandResult,string> result(crFailUndefined,"");

	//compute target
	const Unit *targetUnit= NULL;
	Vec2i targetPos;
	if(prepared){
		targetPos=Vec2i(x, y);
	}
	else {
		if(computeTarget(Vec2i(x, y), targetPos, targetUnit) == false) {
			console->addStdMessage("InvalidPosition");
			return;
		}
	}

	bool queueKeyDown = isKeyDown(queueCommandKey);
    //give orders to the units of this faction
	if(selectingBuilding == false) {
		if(selection.isUniform()) {
			result= commander->tryGiveCommand(&selection, activeCommandType,
											targetPos, targetUnit,queueKeyDown);
		}
		else {
			result= commander->tryGiveCommand(&selection, activeCommandClass,
											targetPos, targetUnit,queueKeyDown);
        	}
	}
	else {
		//selecting building
		result= commander->tryGiveCommand(&selection,
						activeCommandType, posObjWorld, choosenBuildingType,
						selectedBuildingFacing,queueKeyDown);
    }

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result.first == crSuccess || result.first == crSomeFailed) {
		if(prepared == false) {
			mouse3d.enable();
		}

		if(random.randRange(0, 1) == 0) {
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(),
				gameCamera->getPos());
		}
	}
}

void Gui::centerCameraOnSelection() {
	if(selection.isEmpty() == false) {
		Vec3f refPos= selection.getRefPos();
		gameCamera->centerXZ(refPos.x, refPos.z);
	}
}

void Gui::selectInterestingUnit(InterestingUnitType iut) {
	const Faction* thisFaction= world->getThisFaction();
	const Unit* previousUnit= NULL;
	bool previousFound= true;

	//start at the next harvester
	if(selection.getCount()==1){
		const Unit* refUnit= selection.getFrontUnit();

		if(refUnit->isInteresting(iut)){
			previousUnit= refUnit;
			previousFound= false;
		}
	}

	//clear selection
	selection.clear();

	//search
	for(int i= 0; i<thisFaction->getUnitCount(); ++i){
		Unit* unit= thisFaction->getUnit(i);

		if(previousFound){
			if(unit->isInteresting(iut)){
				selection.select(unit);
				break;
			}
		}
		else{
			if(unit==previousUnit){
				previousFound= true;
			}
		}
	}

	//search again if we have a previous
	if(selection.isEmpty() && previousUnit!=NULL && previousFound==true){
		for(int i= 0; i<thisFaction->getUnitCount(); ++i){
			Unit* unit= thisFaction->getUnit(i);

			if(unit->isInteresting(iut)){
				selection.select(unit);
				break;
			}
		}
	}
}

void Gui::clickCommonCommand(CommandClass commandClass){
	for(int i= 0; i<Display::downCellCount; ++i){
		const CommandType* ct= display.getCommandType(i);
		if((ct!=NULL && ct->getClass()==commandClass) || display.getCommandClass(i)==commandClass){
			mouseDownDisplayUnitSkills(i);
			break;
		}
	}
	computeDisplay();
}

void Gui::mouseDownDisplayUnitSkills(int posDisplay) {
	if(!selection.isEmpty()) {
		if(posDisplay != cancelPos) {
			if(posDisplay!=meetingPointPos) {
				const Unit *unit= selection.getFrontUnit();

				//uniform selection
				if(selection.isUniform()) {
				    const CommandType *ct = display.getCommandType(posDisplay);
					if(ct != NULL && unit->getFaction()->reqsOk(ct)) {
						activeCommandType= display.getCommandType(posDisplay);
						activeCommandClass= activeCommandType->getClass();
					}
					else {
						posDisplay= invalidPos;
						activeCommandType= NULL;
						activeCommandClass= ccStop;
						return;
					}
				}

				//non uniform selection
				else {
					activeCommandType= NULL;
					activeCommandClass= display.getCommandClass(posDisplay);
				}

				//give orders depending on command type
				if(!selection.isEmpty()){
					const CommandType *ct= selection.getUnit(0)->getType()->getFirstCtOfClass(activeCommandClass);
					if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
						assert(selection.isUniform());
						selectingBuilding= true;
					}
					else if(ct->getClicks()==cOne){
						invalidatePosObjWorld();
						giveOneClickOrders();
					}
					else{
						selectingPos= true;
						activePos= posDisplay;
					}
				}
			}
			else{
				activePos= posDisplay;
				selectingMeetingPoint= true;
			}
		}
		else{
			commander->tryCancelCommand(&selection);
		}
	}
}

void Gui::mouseDownDisplayUnitBuild(int posDisplay){
	int factionIndex= world->getThisFactionIndex();

	if(posDisplay==cancelPos){
		resetState();
	}
	else{
		if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
			const BuildCommandType *bct= static_cast<const BuildCommandType*>(activeCommandType);
			const UnitType *ut= bct->getBuilding(posDisplay);
			if(world->getFaction(factionIndex)->reqsOk(ut)){
				choosenBuildingType= ut;
				assert(choosenBuildingType!=NULL);
				selectingPos= true;
				selectedBuildingFacing = CardinalDir::NORTH;
				activePos= posDisplay;
			}
		}
	}
}


string Gui::computeDefaultInfoString() {
	//Lang &lang= Lang::getInstance();

	string result="";
	//printf("\n\n\n\n********* selection.isCommandable() [%d] selection.isUniform() [%d]\n\n",selection.isCommandable(),selection.isUniform());

	if(selection.isUniform()) {
		if(selection.isObserver() || selection.isCommandable()) {
			// default is the description extension
			result = selection.getFrontUnit()->getDescExtension();
		}
	}
	return result;
}


void Gui::computeInfoString(int posDisplay){

	Lang &lang= Lang::getInstance();

	lastPosDisplay = posDisplay;

	display.setInfoText(computeDefaultInfoString());

	if(posDisplay!=invalidPos && selection.isCommandable()){
		if(!selectingBuilding){
			if(posDisplay==cancelPos){
				display.setInfoText(lang.get("Cancel"));
			}
			else if(posDisplay==meetingPointPos){
				display.setInfoText(lang.get("MeetingPoint"));
			}
			else{
				//uniform selection
				if(selection.isUniform()){
					const Unit *unit= selection.getFrontUnit();
					const CommandType *ct= display.getCommandType(posDisplay);

					if(ct!=NULL){
						if(unit->getFaction()->reqsOk(ct)){
							display.setInfoText(ct->getDesc(unit->getTotalUpgrade()));
						}
						else{
							if(ct->getClass()==ccUpgrade){
								string text="";
								const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
								if(unit->getFaction()->getUpgradeManager()->isUpgrading(uct->getProducedUpgrade())){
									text=lang.get("Upgrading")+"\n\n";
								}
								else if(unit->getFaction()->getUpgradeManager()->isUpgraded(uct->getProducedUpgrade())){
									text=lang.get("AlreadyUpgraded")+"\n\n";
								}
								display.setInfoText(text+ct->getReqDesc());
							}
							else{
								display.setInfoText(ct->getReqDesc());
							}
						}
					}
				}

				//non uniform selection
				else{
					const UnitType *ut= selection.getFrontUnit()->getType();
					CommandClass cc= display.getCommandClass(posDisplay);
					if(cc!=ccNull){
						display.setInfoText(lang.get("CommonCommand") + ": " + ut->getFirstCtOfClass(cc)->toString());
					}
				}
			}
		}
		else{
			if(posDisplay==cancelPos){
				display.setInfoText(lang.get("Return"));
			}
			else{
				if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
					const BuildCommandType *bct= static_cast<const BuildCommandType*>(activeCommandType);
					display.setInfoText(bct->getBuilding(posDisplay)->getReqDesc());
				}
			}
		}
	}
}

void Gui::computeDisplay(){

	//printf("Start ===> computeDisplay()\n");
	Lang &lang= Lang::getInstance();
	//init
	display.clear();

	// ================ PART 1 ================
	const Object *selectedResourceObject =getSelectedResourceObject();
	if(selection.isEmpty() && selectedResourceObject != NULL && selectedResourceObject->getResource() != NULL) {
		Resource *r = selectedResourceObject->getResource();
		display.setTitle(r->getType()->getName(true));
		display.setText(lang.get("Amount")+ ": "+intToStr(r->getAmount())+" / "+intToStr(r->getType()->getDefResPerPatch()));
		//display.setProgressBar(r->);
		display.setUpImage(0, r->getType()->getImage());
	}
	else {
		//title, text and progress bar
		if(selection.getCount() == 1){
			display.setTitle(selection.getFrontUnit()->getFullName());
			display.setText(selection.getFrontUnit()->getDesc());
			display.setProgressBar(selection.getFrontUnit()->getProductionPercent());
		}

		//portraits
		for(int i= 0; i < selection.getCount(); ++i){
			display.setUpImage(i, selection.getUnit(i)->getType()->getImage());
		}

		// ================ PART 2 ================

		if(selectingPos || selectingMeetingPoint){
			//printf("selectingPos || selectingMeetingPoint\n");
			display.setDownSelectedPos(activePos);
		}

		//printf("computeDisplay selection.isCommandable() = %d\n",selection.isCommandable());

		if(selection.isCommandable()) {
			//printf("selection.isComandable()\n");

			if(selectingBuilding == false){

				//cancel button
				const Unit *u= selection.getFrontUnit();
				const UnitType *ut= u->getType();
				if(selection.isCancelable()) {
					//printf("selection.isCancelable() commandcount = %d\n",selection.getUnit(0)->getCommandSize());
					if(selection.getUnit(0)->getCommandSize() > 0){
						//printf("Current Command [%s]\n",selection.getUnit(0)->getCurrCommand()->toString().c_str());
					}

					display.setDownImage(cancelPos, ut->getCancelImage());
					display.setDownLighted(cancelPos, true);
				}

				//meeting point
				if(selection.isMeetable()){
					//printf("selection.isMeetable()\n");

					display.setDownImage(meetingPointPos, ut->getMeetingPointImage());
					display.setDownLighted(meetingPointPos, true);
				}


				//printf("computeDisplay selection.isUniform() = %d\n",selection.isUniform());

				if(selection.isUniform()) {
					//printf("selection.isUniform()\n");

					//uniform selection
					if(u->isBuilt()){
						//printf("u->isBuilt()\n");

						int morphPos= 8;
						for(int i= 0; i < ut->getCommandTypeCount(); ++i){
							int displayPos= i;
							const CommandType *ct= ut->getCommandType(i);
							if(ct->getClass() == ccMorph) {
								displayPos= morphPos++;
							}

							//printf("computeDisplay i = %d displayPos = %d morphPos = %d ct->getClass() = %d [%s]\n",i,displayPos,morphPos,ct->getClass(),ct->getName().c_str());

							display.setDownImage(displayPos, ct->getImage());
							display.setCommandType(displayPos, ct);
							display.setCommandClass(displayPos, ct->getClass());
							display.setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
						}
					}
				}
				else{
					//printf("selection.isUniform() == FALSE\n");

					//non uniform selection
					int lastCommand= 0;
					for(int i= 0; i < ccCount; ++i){
						CommandClass cc= static_cast<CommandClass> (i);

						//printf("computeDisplay i = %d cc = %d isshared = %d lastCommand = %d\n",i,cc,isSharedCommandClass(cc),lastCommand);

						if(isSharedCommandClass(cc) && cc != ccBuild){
							display.setDownLighted(lastCommand, true);
							display.setDownImage(lastCommand, ut->getFirstCtOfClass(cc)->getImage());
							display.setCommandClass(lastCommand, cc);
							lastCommand++;
						}
					}
				}
			}
			else{

				//selecting building
				const Unit *unit= selection.getFrontUnit();
				if(activeCommandType != NULL && activeCommandType->getClass() == ccBuild){
					const BuildCommandType* bct= static_cast<const BuildCommandType*> (activeCommandType);
					for(int i= 0; i < bct->getBuildingCount(); ++i){
						display.setDownImage(i, bct->getBuilding(i)->getImage());
						display.setDownLighted(i, unit->getFaction()->reqsOk(bct->getBuilding(i)));
					}
					display.setDownImage(cancelPos, selection.getFrontUnit()->getType()->getCancelImage());
					display.setDownLighted(cancelPos, true);
				}
			}
		}
	}

	// refresh other things
	if(!isSelecting() && !isSelectingPos()){
		if(!isSelectingPos() && lastPosDisplay == invalidPos){
			computeInfoString(lastPosDisplay);
		}
	}

}

int Gui::computePosDisplay(int x, int y){
	int posDisplay= display.computeDownIndex(x, y);

	//printf("computePosDisplay x = %d y = %d posDisplay = %d Display::downCellCount = %d cc = %d ct = %p\n",x,y,posDisplay,Display::downCellCount,display.getCommandClass(posDisplay),display.getCommandType(posDisplay));

	if(posDisplay < 0 || posDisplay >= Display::downCellCount) {
		posDisplay= invalidPos;
		//printf("In [%s:%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else if(selection.isCommandable()) {
		if(posDisplay != cancelPos) {
			if(posDisplay != meetingPointPos) {
				if(selectingBuilding == false) {
					//standard selection
					if(display.getCommandClass(posDisplay) == ccNull && display.getCommandType(posDisplay) == NULL) {
						posDisplay= invalidPos;
						//printf("In [%s:%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
					}
				}
				else {
					//building selection
					if(activeCommandType != NULL && activeCommandType->getClass() == ccBuild){
						const BuildCommandType *bct= static_cast<const BuildCommandType*>(activeCommandType);
						if(posDisplay >= bct->getBuildingCount()) {
							posDisplay= invalidPos;
							//printf("In [%s:%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
						}
					}
				}
			}
			else{
				//check meeting point
				if(!selection.isMeetable()) {
					posDisplay= invalidPos;
					//printf("In [%s:%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
				}
			}
		}
		else {
			//check cancel button
			if(selection.isCancelable() == false) {
				posDisplay= invalidPos;
				//printf("In [%s:%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}
	}
	else {
        posDisplay= invalidPos;
        //printf("In [%s:%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
    }

	//printf("computePosDisplay returning = %d\n",posDisplay);

	return posDisplay;
}

void Gui::addOrdersResultToConsole(CommandClass cc, std::pair<CommandResult,string> result) {

    switch(result.first) {
	case crSuccess:
		break;
	case crFailReqs:
        switch(cc){
        case ccBuild:
            console->addStdMessage("BuildingNoReqs",result.second);
		    break;
        case ccProduce:
            console->addStdMessage("UnitNoReqs",result.second);
            break;
        case ccMorph:
            console->addStdMessage("MorphNoReqs",result.second);
            break;
        case ccUpgrade:
            console->addStdMessage("UpgradeNoReqs",result.second);
            break;
        default:
            break;
        }
        break;
	case crFailRes:
        switch(cc){
        case ccBuild:
            console->addStdMessage("BuildingNoRes",result.second);
		    break;
        case ccProduce:
            console->addStdMessage("UnitNoRes",result.second);
            break;
        case ccMorph:
            console->addStdMessage("MorphNoRes",result.second);
            break;
        case ccUpgrade:
            console->addStdMessage("UpgradeNoRes",result.second);
            break;
		default:
			break;
        }
		break;

    case crFailUndefined:
        console->addStdMessage("InvalidOrder",result.second);
        break;

    case crSomeFailed:
        console->addStdMessage("SomeOrdersFailed",result.second);
        break;
    }
}

bool Gui::isSharedCommandClass(CommandClass commandClass){
    for(int i=0; i<selection.getCount(); ++i){
		const Unit *unit= selection.getUnit(i);
        const CommandType *ct= unit->getType()->getFirstCtOfClass(commandClass);
		if(ct==NULL || !unit->getFaction()->reqsOk(ct))
            return false;
    }
    return true;
}

void Gui::computeSelected(bool doubleClick, bool force){
	Selection::UnitContainer units;

	if( force || ( lastQuadCalcFrame+selectionCalculationFrameSkip < game->getTotalRenderFps() ) ){
		lastQuadCalcFrame=game->getTotalRenderFps();
		const Object *selectedResourceObject=NULL;
		selectedResourceObjectPos=Vec2i(-1,-1);
		if(selectionQuad.isEnabled() && selectionQuad.getPosUp().dist(selectionQuad.getPosDown())<minQuadSize){
			//Renderer::getInstance().computeSelected(units, selectedResourceObject, true, selectionQuad.getPosDown(), selectionQuad.getPosDown());
			Renderer::getInstance().computeSelected(units, selectedResourceObject, true, selectionQuad.getPosDown(), selectionQuad.getPosDown());
			if(selectedResourceObject!=NULL){
				selectedResourceObjectPos=Map::toSurfCoords(selectedResourceObject->getMapPos());
			}
		}
		else{
			Renderer::getInstance().computeSelected(units, selectedResourceObject, false, selectionQuad.getPosDown(), selectionQuad.getPosUp());
		}
		selectingBuilding= false;
		activeCommandType= NULL;

		//select all units of the same type if double click
		if(doubleClick && units.empty() == false){
			const Unit *refUnit= getRelevantObjectFromSelection(&units);
			int factionIndex= refUnit->getFactionIndex();
			for(int i=0; i<world->getFaction(factionIndex)->getUnitCount(); ++i){
				Unit *unit= world->getFaction(factionIndex)->getUnit(i);
				if(unit->getPos().dist(refUnit->getPos())<doubleClickSelectionRadius &&
					unit->getType()==refUnit->getType())
				{
					units.push_back(unit);
				}
			}
		}

		bool shiftDown= isKeyDown(vkShift);
		bool controlDown= isKeyDown(vkControl);

		if(!shiftDown && !controlDown){
			//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to call selection.clear()\n",__FILE__,__FUNCTION__,__LINE__);
			selection.clear();
		}

		if(!controlDown){
			//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to call selection.select(units)\n",__FILE__,__FUNCTION__,__LINE__);
			selection.select(units);
			if(!selection.isEmpty()){
				selectedResourceObject=NULL;
			}
		}
		else{
			//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] selection.unSelect(units)\n",__FILE__,__FUNCTION__,__LINE__);
			selection.unSelect(units);
		}
	}
}

bool Gui::computeTarget(const Vec2i &screenPos, Vec2i &targetPos, const Unit *&targetUnit){
	Selection::UnitContainer uc;
	Renderer &renderer= Renderer::getInstance();
	const Object* obj= NULL;
	renderer.computeSelected(uc, obj, true, screenPos, screenPos);
	validPosObjWorld= false;

	if(uc.empty() == false){
		targetUnit= getRelevantObjectFromSelection(&uc);
		targetPos= targetUnit->getPos();
		highlightedUnitId=targetUnit->getId();
		getHighlightedUnit()->resetHighlight();
		return true;
	}
	else if(obj != NULL){
		targetUnit= NULL;
		highlightedResourceObjectPos=Map::toSurfCoords(obj->getMapPos());
		getHighlightedResourceObject()->resetHighlight();
		// get real click pos
		renderer.computePosition(screenPos, targetPos);

		//validPosObjWorld= true;
		//posObjWorld = targetPos;

		int tx= targetPos.x;
		int ty= targetPos.y;

		int ox= obj->getMapPos().x;
		int oy= obj->getMapPos().y;

		Resource* clickedRecource= world->getMap()->getSurfaceCell(Map::toSurfCoords(obj->getMapPos()))->getResource();

		// lets see if the click had the same Resource
		if(clickedRecource == world->getMap()->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource()){
			// same ressource is meant, so use the user selected position
			return true;
		}
		else{// calculate a valid resource position which is as near as possible to the selected position
			Vec2i testIt= Vec2i(obj->getMapPos());
			///////////////
			// test both //
			///////////////
			if(ty < oy){
				testIt.y--;
			}
			else if(ty > oy){
				testIt.y++;
			}
			if(tx < ox){
				testIt.x--;
			}
			else if(tx > ox){
				testIt.x++;
			}

			if(clickedRecource == world->getMap()->getSurfaceCell(Map::toSurfCoords(testIt))->getResource()){
				// same ressource is meant, so use this position
				targetPos= testIt;
				//posObjWorld= targetPos;
				return true;
			}
			else{
				testIt= Vec2i(obj->getMapPos());
			}

			/////////////////
			// test y-only //
			/////////////////
			if(ty < oy){
				testIt.y--;
			}
			else if(ty > oy){
				testIt.y++;
			}
			if(clickedRecource == world->getMap()->getSurfaceCell(Map::toSurfCoords(testIt))->getResource()){
				// same ressource is meant, so use this position
				targetPos= testIt;
				//posObjWorld= targetPos;
				return true;
			}
			else{
				testIt= Vec2i(obj->getMapPos());
			}

			/////////////////
			// test x-only //
			/////////////////
			if(tx < ox){
				testIt.x--;
			}
			else if(tx > ox){
				testIt.x++;
			}
			if(clickedRecource == world->getMap()->getSurfaceCell(Map::toSurfCoords(testIt))->getResource()){
				// same ressource is meant, so use this position
				targetPos= testIt;
				//posObjWorld= targetPos;
				return true;
			}
		}
		// give up and use the object position;
		targetPos= obj->getMapPos();
		posObjWorld= targetPos;
		return true;

	}
	else{
		targetUnit= NULL;
		if(renderer.computePosition(screenPos, targetPos)){
			validPosObjWorld= true;
			posObjWorld= targetPos;

			if(world->getMap()->getSurfaceCell(Map::toSurfCoords(targetPos))->getResource()!=NULL)
			{
				highlightedResourceObjectPos=Map::toSurfCoords(targetPos);
				getHighlightedResourceObject()->resetHighlight();
			}

			return true;
		}
		else{
			return false;
		}
	}
}

Unit* Gui::getRelevantObjectFromSelection(Selection::UnitContainer *uc){
	Unit *resultUnit=NULL;
	for(int i= 0; i < uc->size(); ++i){
		resultUnit= uc->at(i);
		if(resultUnit->getType()->hasSkillClass(scMove)){// moving units are more relevant than non moving ones
			break;
		}
	}
	return resultUnit;
}

void Gui::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *guiNode = rootNode->addChild("Gui");

	//External objects
//	RandomGen random;
	guiNode->addAttribute("random",intToStr(random.getLastNumber()), mapTagReplacements);
//	const Commander *commander;
//	const World *world;
//	const Game *game;
//	GameCamera *gameCamera;
//	Console *console;
//
//	//Positions
//	Vec2i posObjWorld;		//world coords
	guiNode->addAttribute("posObjWorld",posObjWorld.getString(), mapTagReplacements);
//	bool validPosObjWorld;
	guiNode->addAttribute("validPosObjWorld",intToStr(validPosObjWorld), mapTagReplacements);
//	//display
//	const UnitType *choosenBuildingType;
	if(choosenBuildingType != NULL) {
		const Faction* thisFaction= world->getThisFaction();
		guiNode->addAttribute("choosenBuildingType",choosenBuildingType->getName(), mapTagReplacements);
		guiNode->addAttribute("choosenBuildingTypeFactionIndex",intToStr(thisFaction->getIndex()), mapTagReplacements);
	}
//	const CommandType *activeCommandType;
	if(activeCommandType != NULL) {
		guiNode->addAttribute("activeCommandType",activeCommandType->getName(), mapTagReplacements);
	}

//	CommandClass activeCommandClass;
	guiNode->addAttribute("activeCommandClass",intToStr(activeCommandClass), mapTagReplacements);
//	int activePos;
	guiNode->addAttribute("activePos",intToStr(activePos), mapTagReplacements);
//	int lastPosDisplay;
	guiNode->addAttribute("lastPosDisplay",intToStr(lastPosDisplay), mapTagReplacements);
//	//composite
//	Display display;
	display.saveGame(guiNode);
//	Mouse3d mouse3d;
//	Selection selection;
	selection.saveGame(guiNode);
//	SelectionQuad selectionQuad;
//	int lastQuadCalcFrame;
	guiNode->addAttribute("lastQuadCalcFrame",intToStr(lastQuadCalcFrame), mapTagReplacements);
//	int selectionCalculationFrameSkip;
	guiNode->addAttribute("selectionCalculationFrameSkip",intToStr(selectionCalculationFrameSkip), mapTagReplacements);
//	int minQuadSize;
	guiNode->addAttribute("minQuadSize",intToStr(minQuadSize), mapTagReplacements);
//	Chrono lastGroupRecallTime;
	//guiNode->addAttribute("lastGroupRecallTime",intToStr(lastGroupRecallTime.getMillis()), mapTagReplacements);
//	int lastGroupRecall;
	guiNode->addAttribute("lastGroupRecall",intToStr(lastGroupRecall), mapTagReplacements);
//	//states
//	bool selectingBuilding;
	guiNode->addAttribute("selectingBuilding",intToStr(selectingBuilding), mapTagReplacements);
//	bool selectingPos;
	guiNode->addAttribute("selectingPos",intToStr(selectingPos), mapTagReplacements);
//	bool selectingMeetingPoint;
	guiNode->addAttribute("selectingMeetingPoint",intToStr(selectingMeetingPoint), mapTagReplacements);
//	CardinalDir selectedBuildingFacing;
	guiNode->addAttribute("selectedBuildingFacing",intToStr(selectedBuildingFacing), mapTagReplacements);
//	const Object *selectedResourceObject;
//
//	Texture2D* hudTexture;
}

void Gui::loadGame(const XmlNode *rootNode, World *world) {
	const XmlNode *guiNode = rootNode->getChild("Gui");

	//External objects
//	RandomGen random;
	random.setLastNumber(guiNode->getAttribute("random")->getIntValue());
//	const Commander *commander;
//	const World *world;
//	const Game *game;
//	GameCamera *gameCamera;
//	Console *console;
//
//	//Positions
//	Vec2i posObjWorld;		//world coords
	//guiNode->addAttribute("posObjWorld",posObjWorld.getString(), mapTagReplacements);
	posObjWorld = Vec2i::strToVec2(guiNode->getAttribute("posObjWorld")->getValue());
//	bool validPosObjWorld;
	validPosObjWorld = guiNode->getAttribute("validPosObjWorld")->getIntValue() != 0;
//	//display
//	const UnitType *choosenBuildingType;
//	if(choosenBuildingType != NULL) {
//		guiNode->addAttribute("choosenBuildingType",choosenBuildingType->getName(), mapTagReplacements);
//	}
	if(guiNode->hasAttribute("choosenBuildingType") == true) {
		string unitType = guiNode->getAttribute("choosenBuildingType")->getValue();
		int factionIndex = guiNode->getAttribute("choosenBuildingTypeFactionIndex")->getIntValue();
		choosenBuildingType = world->getFaction(factionIndex)->getType()->getUnitType(unitType);
	}
//	const CommandType *activeCommandType;
	//if(activeCommandType != NULL) {
	//	guiNode->addAttribute("activeCommandType",activeCommandType->getName(), mapTagReplacements);
	//}

//	CommandClass activeCommandClass;
	//guiNode->addAttribute("activeCommandClass",intToStr(activeCommandClass), mapTagReplacements);
//	int activePos;
	activePos = guiNode->getAttribute("activePos")->getIntValue();
//	int lastPosDisplay;
	lastPosDisplay = guiNode->getAttribute("lastPosDisplay")->getIntValue();
//	//composite
//	Display display;
	display.loadGame(guiNode);
//	Mouse3d mouse3d;
//	Selection selection;
	selection.loadGame(guiNode,world);
//	SelectionQuad selectionQuad;
//	int lastQuadCalcFrame;
	// don't load this! lastQuadCalcFrame = guiNode->getAttribute("lastQuadCalcFrame")->getIntValue();
	lastQuadCalcFrame = game->getTotalRenderFps();
//	int selectionCalculationFrameSkip;
	selectionCalculationFrameSkip = guiNode->getAttribute("selectionCalculationFrameSkip")->getIntValue();
//	int minQuadSize;
	minQuadSize = guiNode->getAttribute("minQuadSize")->getIntValue();
//	Chrono lastGroupRecallTime;
	//guiNode->addAttribute("lastGroupRecallTime",intToStr(lastGroupRecallTime.getMillis()), mapTagReplacements);
//	int lastGroupRecall;
	lastGroupRecall = guiNode->getAttribute("lastGroupRecall")->getIntValue();
//	//states
//	bool selectingBuilding;
	//selectingBuilding = guiNode->getAttribute("selectingBuilding")->getIntValue();
//	bool selectingPos;
	//guiNode->addAttribute("selectingPos",intToStr(selectingPos), mapTagReplacements);
//	bool selectingMeetingPoint;
	//guiNode->addAttribute("selectingMeetingPoint",intToStr(selectingMeetingPoint), mapTagReplacements);
//	CardinalDir selectedBuildingFacing;
	//guiNode->addAttribute("selectedBuildingFacing",intToStr(selectedBuildingFacing), mapTagReplacements);
//	const Object *selectedResourceObject;
//
//	Texture2D* hudTexture;
}

}}//end namespace
