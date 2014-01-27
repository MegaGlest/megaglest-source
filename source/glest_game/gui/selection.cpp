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

#include "selection.h"

#include <algorithm>

#include "unit_type.h"
#include "gui.h"
#include "config.h"
#include "world.h"
#include "leak_dumper.h"

using namespace std;

namespace Glest{ namespace Game{

// =====================================================
// 	class Selection
// =====================================================

void Selection::init(Gui *gui, int factionIndex, int teamIndex, bool allowSharedTeamUnits) {
	this->factionIndex 			= factionIndex;
	this->teamIndex 			= teamIndex;
	this->allowSharedTeamUnits	= allowSharedTeamUnits;
	this->gui					= gui;
	clear();
}

Selection::~Selection(){
	clear();
}

bool Selection::canSelectUnitFactionCheck(const Unit *unit) const {
	//check if enemy
	if(unit->getFactionIndex() != factionIndex) {
		if(this->allowSharedTeamUnits == false ||
			unit->getFaction()->getTeam() != teamIndex) {
			return false;
		}
	}

	return true;
}

bool Selection::select(Unit *unit) {
	bool result = false;
	if((int)selectedUnits.size() >= Config::getInstance().getInt("MaxUnitSelectCount",intToStr(maxUnits).c_str())) {
		return result;
	}

	// Fix Bug reported on sourceforge.net: Glest::Game::Selection::select crash with NULL pointer - ID: 3608835
	if(unit != NULL) {
		//check if already selected
		for(int index = 0; index < (int)selectedUnits.size(); ++index) {
			if(selectedUnits[index] == unit) {
				return true;
			}
		}

		//check if dead
		if(unit->isDead() == true) {
			return false;
		}

		//check if multisel
		if(unit->getType()->getMultiSelect() == false && isEmpty() == false) {
			return false;
		}

		//check if enemy
		if(canSelectUnitFactionCheck(unit) == false && isEmpty() == false) {
			return false;
		}

		//check existing enemy
		//if(selectedUnits.size() == 1 && selectedUnits.front()->getFactionIndex() != factionIndex) {
		if(selectedUnits.size() == 1 && canSelectUnitFactionCheck(selectedUnits.front()) == false) {
			clear();
		}

		//check existing multisel
		if(selectedUnits.size() == 1 &&
				selectedUnits.front()->getType()->getMultiSelect() == false) {
			clear();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unit selected [%s]\n",__FILE__,__FUNCTION__,__LINE__,unit->toString().c_str());

		unit->addObserver(this);

		int unitTypeId = unit->getType()->getId();
		bool inserted = false;
		for(int index = 0; index < (int)selectedUnits.size(); ++index) {

			int currentTypeId = selectedUnits[index]->getType()->getId();
			if(unitTypeId <= currentTypeId) {

				//place unit here
				selectedUnits.insert(selectedUnits.begin() + index,unit);
				inserted = true;
				break;
			}
		}
		if(inserted == false) {
			selectedUnits.push_back(unit);
		}
		result = true;
		gui->onSelectionChanged();
	}

	return result;
}

void Selection::select(const UnitContainer &units){

	//add units to gui
	for(UnitIterator it = units.begin(); it != units.end(); ++it) {
		select(*it);
	}
}

void Selection::unSelect(const UnitContainer &units) {

	//add units to gui
	for(UnitIterator it = units.begin(); it != units.end(); ++it) {
		for(int i = 0; i < (int)selectedUnits.size(); ++i) {
			if(selectedUnits[i] == *it) {
				unSelect(i);
			}
		}
	}
}

void Selection::unSelect(int i) {
	selectedUnits.erase(selectedUnits.begin() + i);
	gui->onSelectionChanged();
}

void Selection::clear(){
	selectedUnits.clear();
}

bool Selection::isUniform() const{
	if(selectedUnits.empty() == true) {
		return true;
	}

	const UnitType *ut= selectedUnits.front()->getType();

	for(int i = 0; i < (int)selectedUnits.size(); ++i) {
		if(selectedUnits[i]->getType() != ut) {
            return false;
		}
    }
    return true;
}

bool Selection::isEnemy() const {
	return selectedUnits.size() == 1 &&
			//selectedUnits.front()->getFactionIndex() != factionIndex;
			canSelectUnitFactionCheck(selectedUnits.front()) == false;
}

bool Selection::isObserver() const {
	return selectedUnits.size() == 1 &&
			(teamIndex == (GameConstants::maxPlayers -1 + fpt_Observer));
}

bool Selection::isCommandable() const {
	//printf("\n\n\n\n********* selection.isCommandable() ---> isEmpty() [%d] isEnemy() [%d] selectedUnits.size() [%d]\n\n",isEmpty(),isEnemy(),(int)selectedUnits.size());

	return
		isEmpty() == false &&
		isEnemy() == false &&
		(selectedUnits.size() == 1 && selectedUnits.front()->isAlive() == false) == false;
}

bool Selection::isCancelable() const {
	return
		selectedUnits.size() > 1 ||
		(selectedUnits.size() == 1 && selectedUnits[0]->anyCommand(true));
}

bool Selection::isMeetable() const{
	return
		isUniform() &&
		isCommandable() &&
		selectedUnits.front()->getType()->getMeetingPoint();
}

Vec3f Selection::getRefPos() const{
	return getFrontUnit()->getCurrVector();
}

bool Selection::hasUnit(const Unit* unit) const {
	return find(selectedUnits.begin(), selectedUnits.end(), unit) != selectedUnits.end();
}

void Selection::assignGroup(int groupIndex,const UnitContainer *pUnits) {
	if(groupIndex < 0 || groupIndex >= maxGroups) {
		throw megaglest_runtime_error("Invalid value for groupIndex = " + intToStr(groupIndex));
	}

	//clear group
	groups[groupIndex].clear();

	//assign new group
	const UnitContainer *addUnits = &selectedUnits;
	if(pUnits != NULL) {
		addUnits = pUnits;
	}
	for(unsigned int i = 0; i < addUnits->size(); ++i) {
		groups[groupIndex].push_back((*addUnits)[i]);
	}
}

void Selection::addUnitToGroup(int groupIndex,Unit *unit) {
	if(groupIndex < 0 || groupIndex >= maxGroups) {
		throw megaglest_runtime_error("Invalid value for groupIndex = " + intToStr(groupIndex));
	}

	if(unit != NULL) {
		groups[groupIndex].push_back(unit);
	}
}

void Selection::removeUnitFromGroup(int groupIndex,int unitId) {
	if(groupIndex < 0 || groupIndex >= maxGroups) {
		throw megaglest_runtime_error("Invalid value for groupIndex = " + intToStr(groupIndex));
	}

	for(unsigned int i = 0; i < groups[groupIndex].size(); ++i) {
		Unit *unit = groups[groupIndex][i];
		if(unit != NULL && unit->getId() == unitId) {
			groups[groupIndex].erase(groups[groupIndex].begin() + i);
			break;
		}
	}
}

vector<Unit*> Selection::getUnitsForGroup(int groupIndex) {
	if(groupIndex < 0 || groupIndex >= maxGroups) {
		throw megaglest_runtime_error("Invalid value for groupIndex = " + intToStr(groupIndex));
	}
	return groups[groupIndex];
}

void Selection::recallGroup(int groupIndex){
	if(groupIndex < 0 || groupIndex >= maxGroups) {
		throw megaglest_runtime_error("Invalid value for groupIndex = " + intToStr(groupIndex));
	}

	clear();
	for(int i = 0; i < (int)groups[groupIndex].size(); ++i) {
		select(groups[groupIndex][i]);
	}
}

void Selection::unitEvent(UnitObserver::Event event, const Unit *unit) {

	if(event == UnitObserver::eKill) {

		//remove from selection
		for(int index = 0; index < (int)selectedUnits.size(); ++index) {
			if(selectedUnits[index] == unit){
				selectedUnits.erase(selectedUnits.begin() + index);
				break;
			}
		}

		//remove from groups
		for(int index = 0; index < maxGroups; ++index) {
			for(int index2 = 0; index2 < (int)groups[index].size(); ++index2) {
				if(groups[index][index2] == unit) {
					groups[index].erase(groups[index].begin() + index2);
					break;
				}
			}
		}

		//notify gui only if no more units to execute the command
		//of course the selection changed, but this doesn't matter in this case.
		if( selectedUnits.empty() == true) {
			gui->onSelectionChanged();
		}
	}
}

void Selection::saveGame(XmlNode *rootNode) const {

	std::map<string,string> mapTagReplacements;
	XmlNode *selectionNode = rootNode->addChild("Selection");

	selectionNode->addAttribute("factionIndex",intToStr(factionIndex), mapTagReplacements);
	selectionNode->addAttribute("teamIndex",intToStr(teamIndex), mapTagReplacements);
	selectionNode->addAttribute("allowSharedTeamUnits",intToStr(allowSharedTeamUnits), mapTagReplacements);

	for(unsigned int i = 0; i < selectedUnits.size(); i++) {
		Unit *unit = selectedUnits[i];

		XmlNode *selectedUnitsNode = selectionNode->addChild("selectedUnits");
		selectedUnitsNode->addAttribute("unitId",intToStr(unit->getId()), mapTagReplacements);
	}

	for(unsigned int x = 0; x < (unsigned int)maxGroups; ++x) {
		XmlNode *groupsNode = selectionNode->addChild("groups");
		for(unsigned int i = 0; i < (unsigned int)groups[x].size(); ++i) {
			Unit *unit = groups[x][i];

			XmlNode *selectedUnitsNode = groupsNode->addChild("selectedUnits");
			selectedUnitsNode->addAttribute("unitId",intToStr(unit->getId()), mapTagReplacements);
		}
	}
}

void Selection::loadGame(const XmlNode *rootNode, World *world) {

	const XmlNode *selectionNode = rootNode->getChild("Selection");

	factionIndex = selectionNode->getAttribute("factionIndex")->getIntValue();
	teamIndex = selectionNode->getAttribute("teamIndex")->getIntValue();
	if(selectionNode->hasAttribute("allowSharedTeamUnits") == true) {
		allowSharedTeamUnits = selectionNode->getAttribute("allowSharedTeamUnits")->getIntValue();
	}

	vector<XmlNode *> selectedUnitsNodeList = selectionNode->getChildList("selectedUnits");
	for(unsigned int i = 0; i < selectedUnitsNodeList.size(); ++i) {
		XmlNode *selectedUnitsNode = selectedUnitsNodeList[i];

		int unitId = selectedUnitsNode->getAttribute("unitId")->getIntValue();
		Unit *unit = world->findUnitById(unitId);
		//assert(unit != NULL);
		//printf("#1 Unit [%s], group: %d\n",unit->getType()->getName().c_str(),i);
		selectedUnits.push_back(unit);
	}

	vector<XmlNode *> groupsNodeList = selectionNode->getChildList("groups");
	for(unsigned int i = 0; i < groupsNodeList.size(); ++i) {
		XmlNode *groupsNode = groupsNodeList[i];

		vector<XmlNode *> selectedGroupsUnitsNodeList = groupsNode->getChildList("selectedUnits");
		for(unsigned int j = 0; j < selectedGroupsUnitsNodeList.size(); ++j) {
			XmlNode *selectedGroupsUnitsNode = selectedGroupsUnitsNodeList[j];

			int unitId = selectedGroupsUnitsNode->getAttribute("unitId")->getIntValue();
			Unit *unit = world->findUnitById(unitId);
			//assert(unit != NULL);
			//printf("Unit #2 [%s], group: %d\n",unit->getType()->getName().c_str(),i);
			groups[i].push_back(unit);
		}
	}
}


}}//end namespace
