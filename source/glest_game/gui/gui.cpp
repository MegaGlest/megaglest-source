// ==============================================================
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
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

    posObjWorld= Vec2i(54, 14);
	validPosObjWorld= false;
    activeCommandType= NULL;
    activeCommandClass= ccStop;
	selectingBuilding= false;
	selectedBuildingFacing = CardinalDir::NORTH;
	selectingPos= false;
	selectingMeetingPoint= false;
	activePos= invalidPos;
	lastQuadCalcFrame=0;
	selectionCalculationFrameSkip=10;
	minQuadSize=20;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void Gui::init(Game *game){
	this->commander= game->getCommander();
	this->gameCamera= game->getGameCamera();
	this->console= game->getConsole();
	this->world= game->getWorld();
	this->game=game;
	selection.init(this, world->getThisFactionIndex());
}

void Gui::end(){
	selection.clear();
}

// ==================== get ====================

const UnitType *Gui::getBuilding() const{
    assert(selectingBuilding);
    return choosenBuildingType;
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
bool Gui::mouseValid(int x, int y){
	return computePosDisplay(x, y) != invalidPos;
}

void Gui::mouseDownLeftDisplay(int x, int y){
	if(!selectingPos && !selectingMeetingPoint){
		int posDisplay= computePosDisplay(x, y);
		if(posDisplay!= invalidPos){
			if(selection.isCommandable()){
				if(selectingBuilding){

				    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] selectingBuilding == true\n",__FILE__,__FUNCTION__);

					mouseDownDisplayUnitBuild(posDisplay);
				}
				else{
				    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] selectingBuilding == false\n",__FILE__,__FUNCTION__);

					mouseDownDisplayUnitSkills(posDisplay);
				}
			}
			else{
				resetState();
			}
		}
		computeDisplay();
	}
}

void Gui::mouseMoveDisplay(int x, int y){
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	computeInfoString(computePosDisplay(x, y));
}

void Gui::mouseDownLeftGraphics(int x, int y, bool prepared){

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	if(selectingPos){
	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] selectingPos == true\n",__FILE__,__FUNCTION__);

		//give standard orders
		giveTwoClickOrders(x, y, prepared);
		resetState();
	}
	//set meeting point
	else if(selectingMeetingPoint){
	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] selectingMeetingPoint == true\n",__FILE__,__FUNCTION__);

		if(selection.isCommandable()){

		    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] selection.isComandable() == true\n",__FILE__,__FUNCTION__);

			Vec2i targetPos;
			if(Renderer::getInstance().computePosition(Vec2i(x, y), targetPos)){

			    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] computePosition() == true\n",__FILE__,__FUNCTION__);

				commander->trySetMeetingPoint(selection.getFrontUnit(), targetPos);
			}
		}
		resetState();
	}
	else{
	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] selectionQuad()\n",__FILE__,__FUNCTION__);

		selectionQuad.setPosDown(Vec2i(x, y));
		computeSelected(false,false);
	}
	computeDisplay();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void Gui::mouseDownRightGraphics(int x, int y , bool prepared){

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	if(selectingPos || selectingMeetingPoint){
		resetState();
	}
	else if(selection.isCommandable()){
		if(prepared){
			givePreparedDefaultOrders(x, y);
		}
		else{
			giveDefaultOrders(x, y);
		}
	}
	computeDisplay();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void Gui::mouseUpLeftGraphics(int x, int y){
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	if(!selectingPos && !selectingMeetingPoint){
		if(selectionQuad.isEnabled()){
			selectionQuad.setPosUp(Vec2i(x, y));
			if(selectionQuad.getPosUp().dist(selectionQuad.getPosDown())>minQuadSize)
			{
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

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void Gui::mouseMoveGraphics(int x, int y){

	//compute selection
	if(selectionQuad.isEnabled()){
		selectionQuad.setPosUp(Vec2i(x, y));
		computeSelected(false,false);
	}

	//compute position for building
	if(isPlacingBuilding()){
	    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] isPlacingBuilding() == true\n",__FILE__,__FUNCTION__);

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

void Gui::groupKey(int groupIndex){
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] groupIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,groupIndex);

	if(isKeyDown(vkControl)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] groupIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,groupIndex);

		selection.assignGroup(groupIndex);
	}
	else{
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] groupIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,groupIndex);

		selection.recallGroup(groupIndex);
	}
}

void Gui::hotKey(char key) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] key = [%c][%d]\n",__FILE__,__FUNCTION__,key,key);

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	if(key == configKeys.getCharKey("HotKeyCenterCameraOnSelection")) {
		centerCameraOnSelection();
	}
	else if(key == configKeys.getCharKey("HotKeySelectIdleHarvesterUnit")) {
		selectInterestingUnit(iutIdleHarvester);
	}
	else if(key == configKeys.getCharKey("HotKeySelectBuiltBuilding")) {
		selectInterestingUnit(iutBuiltBuilding);
	}
	else if(key == configKeys.getCharKey("HotKeyDumpWorldToLog")) {
		std::string worldLog = world->DumpWorldToLog();
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] worldLog dumped to [%s]\n",__FILE__,__FUNCTION__,__LINE__,worldLog.c_str());
	}
	else if(key == configKeys.getCharKey("HotKeyRotateUnitDuringPlacement")){
	    // Here the user triggers a unit rotation while placing a unit
	    if(isPlacingBuilding()) {
	    	if(getBuilding()->getRotationAllowed()){
				++selectedBuildingFacing;
	    	}
	    }
	}
	else if(key == configKeys.getCharKey("HotKeySelectDamagedUnit")) {
		selectInterestingUnit(iutDamaged);
	}
	else if(key == configKeys.getCharKey("HotKeySelectStoreUnit")) {
		selectInterestingUnit(iutStore);
	}
	else if(key == configKeys.getCharKey("HotKeySelectedUnitsAttack")) {
		clickCommonCommand(ccAttack);
	}
	else if(key == configKeys.getCharKey("HotKeySelectedUnitsStop")) {
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
	CommandResult result;
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
	const Unit *targetUnit= NULL;
	Vec2i targetPos;
	if(computeTarget(Vec2i(x, y), targetPos, targetUnit) == false) {
		console->addStdMessage("InvalidPosition");
		return;
	}
	giveDefaultOrders(targetPos.x,targetPos.y,targetUnit,true);
}

void Gui::givePreparedDefaultOrders(int x, int y){
	giveDefaultOrders(x, y, NULL,false);
}

void Gui::giveDefaultOrders(int x, int y,const Unit *targetUnit, bool paintMouse3d) {
	bool queueKeyDown = isKeyDown(queueCommandKey);
	Vec2i targetPos=Vec2i(x, y);
	//give order
	CommandResult result= commander->tryGiveCommand(&selection, targetPos, targetUnit, queueKeyDown);

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result == crSuccess || result == crSomeFailed) {
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

	CommandResult result;

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
	if(result == crSuccess || result == crSomeFailed) {
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
}

void Gui::mouseDownDisplayUnitSkills(int posDisplay){
	if(!selection.isEmpty()){
		if(posDisplay != cancelPos){
			if(posDisplay!=meetingPointPos){
				const Unit *unit= selection.getFrontUnit();

				//uniform selection
				if(selection.isUniform()){
					if(unit->getFaction()->reqsOk(display.getCommandType(posDisplay))){
						activeCommandType= display.getCommandType(posDisplay);
						activeCommandClass= activeCommandType->getClass();
					}
					else{
						posDisplay= invalidPos;
						activeCommandType= NULL;
						activeCommandClass= ccStop;
						return;
					}
				}

				//non uniform selection
				else{
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

void Gui::computeInfoString(int posDisplay){

	Lang &lang= Lang::getInstance();

	display.setInfoText("");
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
								const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
								if(unit->getFaction()->getUpgradeManager()->isUpgrading(uct->getProducedUpgrade())){
									display.setInfoText(lang.get("Upgrading"));
								}
								else if(unit->getFaction()->getUpgradeManager()->isUpgraded(uct->getProducedUpgrade())){
									display.setInfoText(lang.get("AlreadyUpgraded"));
								}
								else{
									display.setInfoText(ct->getReqDesc());
								}
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

void Gui::computeDisplay() {

	//printf("Start ===> computeDisplay()\n");

	//init
	display.clear();

    // ================ PART 1 ================

	//title, text and progress bar
    if(selection.getCount()==1){
		display.setTitle(selection.getFrontUnit()->getFullName());
		display.setText(selection.getFrontUnit()->getDesc());
		display.setProgressBar(selection.getFrontUnit()->getProductionPercent());
    }

    //portraits
	for(int i=0; i<selection.getCount(); ++i){
		display.setUpImage(i, selection.getUnit(i)->getType()->getImage());
	}

    // ================ PART 2 ================

	if(selectingPos || selectingMeetingPoint) {
		//printf("selectingPos || selectingMeetingPoint\n");
		display.setDownSelectedPos(activePos);
	}

	if(selection.isCommandable()) {
		//printf("selection.isComandable()\n");

		if(selectingBuilding == false) {

			//cancel button
			const Unit *u= selection.getFrontUnit();
			const UnitType *ut= u->getType();
			if(selection.isCancelable()) {
				//printf("selection.isCancelable() commandcount = %d\n",selection.getUnit(0)->getCommandSize());
				if(selection.getUnit(0)->getCommandSize() > 0) {
					//printf("Current Command [%s]\n",selection.getUnit(0)->getCurrCommand()->toString().c_str());
				}

				display.setDownImage(cancelPos, ut->getCancelImage());
				display.setDownLighted(cancelPos, true);
			}

			//meeting point
			if(selection.isMeetable()) {
				//printf("selection.isMeetable()\n");

				display.setDownImage(meetingPointPos, ut->getMeetingPointImage());
				display.setDownLighted(meetingPointPos, true);
			}

			if(selection.isUniform()) {
				//printf("selection.isUniform()\n");

				//uniform selection
				if(u->isBuilt()) {
					//printf("u->isBuilt()\n");

					int morphPos= 8;
					for(int i=0; i<ut->getCommandTypeCount(); ++i) {
						int displayPos= i;
						const CommandType *ct= ut->getCommandType(i);
						if(ct->getClass()==ccMorph) {
							displayPos= morphPos++;
						}
						display.setDownImage(displayPos, ct->getImage());
						display.setCommandType(displayPos, ct);
						display.setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
					}
				}
			}
			else {
				//printf("selection.isUniform() == FALSE\n");

				//non uniform selection
				int lastCommand= 0;
				for(int i=0; i<ccCount; ++i) {
					CommandClass cc= static_cast<CommandClass>(i);
					if(isSharedCommandClass(cc) && cc!=ccBuild) {
						display.setDownLighted(lastCommand, true);
						display.setDownImage(lastCommand, ut->getFirstCtOfClass(cc)->getImage());
						display.setCommandClass(lastCommand, cc);
						lastCommand++;
					}
				}
			}
		}
		else {

			//selecting building
			const Unit *unit= selection.getFrontUnit();
			if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
				const BuildCommandType* bct= static_cast<const BuildCommandType*>(activeCommandType);
				for(int i=0; i<bct->getBuildingCount(); ++i){
					display.setDownImage(i, bct->getBuilding(i)->getImage());
					display.setDownLighted(i, unit->getFaction()->reqsOk(bct->getBuilding(i)));
				}
				display.setDownImage(cancelPos, selection.getFrontUnit()->getType()->getCancelImage());
				display.setDownLighted(cancelPos, true);
			}
		}
    }
}

int Gui::computePosDisplay(int x, int y){
	int posDisplay= display.computeDownIndex(x, y);

	if(posDisplay<0 || posDisplay>=Display::downCellCount){
		posDisplay= invalidPos;
	}
	else if(selection.isCommandable()){
		if(posDisplay!=cancelPos){
			if(posDisplay!=meetingPointPos){
				if(!selectingBuilding){
					//standard selection
					if(display.getCommandClass(posDisplay)==ccNull && display.getCommandType(posDisplay)==NULL){
						posDisplay= invalidPos;
					}
				}
				else{
					//building selection
					if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
						const BuildCommandType *bct= static_cast<const BuildCommandType*>(activeCommandType);
						if(posDisplay>=bct->getBuildingCount()){
							posDisplay= invalidPos;
						}
					}
				}
			}
			else{
				//check meeting point
				if(!selection.isMeetable()){
					posDisplay= invalidPos;
				}
			}
		}
		else{
			//check cancel button
			if(!selection.isCancelable()){
				posDisplay= invalidPos;
			}
		}
	}
	else{
        posDisplay= invalidPos;
    }

	return posDisplay;
}

void Gui::addOrdersResultToConsole(CommandClass cc, CommandResult result){

    switch(result){
	case crSuccess:
		break;
	case crFailReqs:
        switch(cc){
        case ccBuild:
            console->addStdMessage("BuildingNoReqs");
		    break;
        case ccProduce:
            console->addStdMessage("UnitNoReqs");
            break;
        case ccUpgrade:
            console->addStdMessage("UpgradeNoReqs");
            break;
        default:
            break;
        }
        break;
	case crFailRes:
        switch(cc){
        case ccBuild:
            console->addStdMessage("BuildingNoRes");
		    break;
        case ccProduce:
            console->addStdMessage("UnitNoRes");
            break;
        case ccUpgrade:
            console->addStdMessage("UpgradeNoRes");
            break;
		default:
			break;
        }
		break;

    case crFailUndefined:
        console->addStdMessage("InvalidOrder");
        break;

    case crSomeFailed:
        console->addStdMessage("SomeOrdersFailed");
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
		if(selectionQuad.isEnabled() && selectionQuad.getPosUp().dist(selectionQuad.getPosDown())<minQuadSize){
			Renderer::getInstance().computeSelected(units, selectionQuad.getPosDown(), selectionQuad.getPosDown());
		}
		else{
			Renderer::getInstance().computeSelected(units, selectionQuad.getPosDown(), selectionQuad.getPosUp());
		}
		selectingBuilding= false;
		activeCommandType= NULL;

		//select all units of the same type if double click
		if(doubleClick && units.size()==1){
			const Unit *refUnit= units.front();
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
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to call selection.clear()\n",__FILE__,__FUNCTION__,__LINE__);
			selection.clear();
		}

		if(!controlDown){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to call selection.select(units)\n",__FILE__,__FUNCTION__,__LINE__);
			selection.select(units);
		}
		else{
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] selection.unSelect(units)\n",__FILE__,__FUNCTION__,__LINE__);
			selection.unSelect(units);
		}
	}
}

bool Gui::computeTarget(const Vec2i &screenPos, Vec2i &targetPos, const Unit *&targetUnit) {
	Selection::UnitContainer uc;
	Renderer &renderer= Renderer::getInstance();
	renderer.computeSelected(uc, screenPos, screenPos);
	validPosObjWorld= false;

	if(uc.empty() == false) {
		targetUnit= uc.front();
		targetPos= targetUnit->getPos();
		return true;
	}
	else {
		targetUnit= NULL;
		if(renderer.computePosition(screenPos, targetPos)) {
			validPosObjWorld= true;
			posObjWorld= targetPos;
			return true;
		}
		else {
			return false;
		}
	}
}

}}//end namespace
