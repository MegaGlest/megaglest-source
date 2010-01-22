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

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Mouse3d
// =====================================================

const float Mouse3d::fadeSpeed= 1.f/50.f;

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
    posObjWorld= Vec2i(54, 14);
    computeSelection= false;
	validPosObjWorld= false;
    activeCommandType= NULL;
    activeCommandClass= ccStop;
	selectingBuilding= false;
	selectingPos= false;
	selectingMeetingPoint= false;
	activePos= invalidPos;
}

void Gui::init(Game *game){
	this->commander= game->getCommander();
	this->gameCamera= game->getGameCamera();
	this->console= game->getConsole();
	this->world= game->getWorld();
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

void Gui::setComputeSelectionFlag(){
	computeSelection= true;
}


// ==================== reset state ==================== 

void Gui::resetState(){
    selectingBuilding= false;
	selectingPos= false;
	selectingMeetingPoint= false;
    activePos= invalidPos;
	activeCommandClass= ccStop;
	activeCommandType= NULL;
}

// ==================== events ==================== 

void Gui::update(){
    setComputeSelectionFlag();
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
			if(selection.isComandable()){
				if(selectingBuilding){
					mouseDownDisplayUnitBuild(posDisplay);
				}
				else{
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
	computeInfoString(computePosDisplay(x, y));
}

void Gui::mouseDownLeftGraphics(int x, int y){
	if(selectingPos){
		//give standard orders
		giveTwoClickOrders(x, y);
		resetState();
	}
	//set meeting point
	else if(selectingMeetingPoint){
		if(selection.isComandable()){
			Vec2i targetPos;
			if(Renderer::getInstance().computePosition(Vec2i(x, y), targetPos)){
				commander->trySetMeetingPoint(selection.getFrontUnit(), targetPos);
			}
		}
		resetState();
	}
	else{
		selectionQuad.setPosDown(Vec2i(x, y));
		computeSelected(false);
	}
	computeDisplay();
}

void Gui::mouseDownRightGraphics(int x, int y){

	if(selectingPos || selectingMeetingPoint){
		resetState();
	}
	else if(selection.isComandable()){
		giveDefaultOrders(x, y);
	}
	computeDisplay();
}

void Gui::mouseUpLeftGraphics(int x, int y){
	if(!selectingPos && !selectingMeetingPoint){
		if(selectionQuad.isEnabled()){
			selectionQuad.setPosUp(Vec2i(x, y));
			if(selection.isComandable() && random.randRange(0, 1)==0){
				SoundRenderer::getInstance().playFx(
					selection.getFrontUnit()->getType()->getSelectionSound(),
					selection.getFrontUnit()->getCurrVector(), 
					gameCamera->getPos());
			}
			selectionQuad.disable();
		}
	}
}

void Gui::mouseMoveGraphics(int x, int y){
   
	//compute selection
	if(selectionQuad.isEnabled()){
		selectionQuad.setPosUp(Vec2i(x, y));
		if(computeSelection){
			computeSelection= false;
			computeSelected(false);
		}
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
		computeSelected(true);
		computeDisplay();
	}
}

void Gui::groupKey(int groupIndex){
	if(isKeyDown(vkControl)){
		selection.assignGroup(groupIndex);
	}
	else{
		selection.recallGroup(groupIndex);
	}
}

void Gui::hotKey(char key){
	if(key==' '){
		centerCameraOnSelection();
	}
	else if(key=='I'){
		selectInterestingUnit(iutIdleHarvester);
	}
	else if(key=='B'){
		selectInterestingUnit(iutBuiltBuilding);
	}
	else if(key=='R'){
		selectInterestingUnit(iutProducer);
	}
	else if(key=='D'){
		selectInterestingUnit(iutDamaged);
	}
	else if(key=='T'){
		selectInterestingUnit(iutStore);
	}
	else if(key=='A'){
		clickCommonCommand(ccAttack);
	}
	else if(key=='S'){
		clickCommonCommand(ccStop);
	}
	else if(key=='M'){
		clickCommonCommand(ccMove);
	}
}

void Gui::onSelectionChanged(){
	resetState();
	computeDisplay();
}

// ================= PRIVATE =================

void Gui::giveOneClickOrders(){
	CommandResult result;
	if(selection.isUniform()){
		result= commander->tryGiveCommand(&selection, activeCommandType);  
	}
	else{
		result= commander->tryGiveCommand(&selection, activeCommandClass); 
	}
	addOrdersResultToConsole(activeCommandClass, result);
    activeCommandType= NULL;
    activeCommandClass= ccStop;
}

void Gui::giveDefaultOrders(int x, int y){

	//compute target
	const Unit *targetUnit= NULL;
	Vec2i targetPos;
	if(!computeTarget(Vec2i(x, y), targetPos, targetUnit)){
		console->addStdMessage("InvalidPosition");
		return;
	}

	//give order
	CommandResult result= commander->tryGiveCommand(&selection, targetPos, targetUnit);

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result == crSuccess || result == crSomeFailed){
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

void Gui::giveTwoClickOrders(int x, int y){
   
	CommandResult result;

	//compute target
	const Unit *targetUnit= NULL;
	Vec2i targetPos;
	if(!computeTarget(Vec2i(x, y), targetPos, targetUnit)){
		console->addStdMessage("InvalidPosition");
		return;
	}

    //give orders to the units of this faction
	if(!selectingBuilding){
		if(selection.isUniform()){
			result= commander->tryGiveCommand(&selection, activeCommandType, targetPos, targetUnit);  
		}
		else{
			result= commander->tryGiveCommand(&selection, activeCommandClass, targetPos, targetUnit);        
        }
	}
	else{
		//selecting building
		result= commander->tryGiveCommand( selection.getFrontUnit(), activeCommandType, posObjWorld, choosenBuildingType ); 
    }
          
	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result == crSuccess || result == crSomeFailed){
		mouse3d.enable();
	
		if(random.randRange(0, 1)==0){
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(), 
				gameCamera->getPos());
		}
	}
}

void Gui::centerCameraOnSelection(){
	if(!selection.isEmpty()){
		Vec3f refPos= selection.getRefPos();
		gameCamera->centerXZ(refPos.x, refPos.z);
	}
}

void Gui::selectInterestingUnit(InterestingUnitType iut){
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
				selectingPos= true;;
				activePos= posDisplay;
			}   
		}
	}
}

void Gui::computeInfoString(int posDisplay){

	Lang &lang= Lang::getInstance();

	display.setInfoText("");
	if(posDisplay!=invalidPos && selection.isComandable()){
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

void Gui::computeDisplay(){

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

	if(selectingPos || selectingMeetingPoint){
		display.setDownSelectedPos(activePos);
	}

	if(selection.isComandable()){
		if(!selectingBuilding){

			//cancel button
			const Unit *u= selection.getFrontUnit();
			const UnitType *ut= u->getType();
			if(selection.isCancelable()){
				display.setDownImage(cancelPos, ut->getCancelImage());
				display.setDownLighted(cancelPos, true);
			}

			//meeting point
			if(selection.isMeetable()){
				display.setDownImage(meetingPointPos, ut->getMeetingPointImage());
				display.setDownLighted(meetingPointPos, true);
			}

			if(selection.isUniform()){
				//uniform selection
				if(u->isBuilt()){
					int morphPos= 8;
					for(int i=0; i<ut->getCommandTypeCount(); ++i){
						int displayPos= i;
						const CommandType *ct= ut->getCommandType(i);	
						if(ct->getClass()==ccMorph){
							displayPos= morphPos++;
						}
						display.setDownImage(displayPos, ct->getImage());
						display.setCommandType(displayPos, ct);
						display.setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
					}
				}
			}

			else{
				//non uniform selection
				int lastCommand= 0;
				for(int i=0; i<ccCount; ++i){
					CommandClass cc= static_cast<CommandClass>(i);
					if(isSharedCommandClass(cc) && cc!=ccBuild){
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
	else if(selection.isComandable()){
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

void Gui::computeSelected(bool doubleClick){
	Selection::UnitContainer units;
	Renderer::getInstance().computeSelected(units, selectionQuad.getPosDown(), selectionQuad.getPosUp());
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
		selection.clear();
	}

	if(!controlDown){
		selection.select(units);
	}
	else{
		selection.unSelect(units);
	}
}

bool Gui::computeTarget(const Vec2i &screenPos, Vec2i &targetPos, const Unit *&targetUnit){
	Selection::UnitContainer uc;
	Renderer &renderer= Renderer::getInstance();
	renderer.computeSelected(uc, screenPos, screenPos);
	validPosObjWorld= false;
			
	if(!uc.empty()){
		targetUnit= uc.front();
		targetPos= targetUnit->getPos();
		return true;
	}
	else{
		targetUnit= NULL;
		if(renderer.computePosition(screenPos, targetPos)){
			validPosObjWorld= true;
			posObjWorld= targetPos;
			return true;
		}
		else{
			return false;
		}
	}
}

}}//end namespace
