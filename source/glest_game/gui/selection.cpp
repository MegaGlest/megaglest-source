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
#include "leak_dumper.h"

using namespace std;

namespace Glest{ namespace Game{

// =====================================================
// 	class Selection
// =====================================================

void Selection::init(Gui *gui, int factionIndex){
	this->factionIndex= factionIndex;	
	this->gui= gui;
}

Selection::~Selection(){
	clear();
}

void Selection::select(Unit *unit){

	//check size
	if(selectedUnits.size()>=maxUnits){
		return;
	}

	//check if already selected 
	for(int i=0; i<selectedUnits.size(); ++i){
		if(selectedUnits[i]==unit){
			return;
		}
	}

	//check if dead
	if(unit->isDead()){
		return;
	}

	//check if multisel
	if(!unit->getType()->getMultiSelect() && !isEmpty()){
		return;
	}

	//check if enemy
	if(unit->getFactionIndex()!=factionIndex && !isEmpty()){
		return;
	}

	//check existing enemy
	if(selectedUnits.size()==1 && selectedUnits.front()->getFactionIndex()!=factionIndex){
		clear();
	}

	//check existing multisel
	if(selectedUnits.size()==1 && !selectedUnits.front()->getType()->getMultiSelect()){
		clear();
	}

	unit->addObserver(this);
	selectedUnits.push_back(unit);
	gui->onSelectionChanged();
}

void Selection::select(const UnitContainer &units){
	
	//add units to gui
	for(UnitIterator it= units.begin(); it!=units.end(); ++it){
		select(*it);
	}
}

void Selection::unSelect(const UnitContainer &units){
	
	//add units to gui
	for(UnitIterator it= units.begin(); it!=units.end(); ++it){
		for(int i=0; i<selectedUnits.size(); ++i){
			if(selectedUnits[i]==*it){
				unSelect(i);
			}
		}
	}
}

void Selection::unSelect(int i){
	//remove unit from list
	selectedUnits.erase(selectedUnits.begin()+i);
	gui->onSelectionChanged();
}

void Selection::clear(){   
	//clear list
	selectedUnits.clear();
}

bool Selection::isUniform() const{
	if(selectedUnits.empty()){
		return true;
	}

	const UnitType *ut= selectedUnits.front()->getType();

	for(int i=0; i<selectedUnits.size(); ++i){
		if(selectedUnits[i]->getType()!=ut){
            return false;
		}
    }
    return true;
}

bool Selection::isEnemy() const{
	return selectedUnits.size()==1 && selectedUnits.front()->getFactionIndex()!=factionIndex;
}
	
bool Selection::isComandable() const{
	return 
		!isEmpty() && 
		!isEnemy() && 
		!(selectedUnits.size()==1 && !selectedUnits.front()->isOperative());
}

bool Selection::isCancelable() const{
	return 
		selectedUnits.size()>1 || 
		(selectedUnits.size()==1 && selectedUnits[0]->anyCommand());
}

bool Selection::isMeetable() const{
	return
		isUniform() &&
		isComandable() &&
		selectedUnits.front()->getType()->getMeetingPoint();
}

Vec3f Selection::getRefPos() const{
	return getFrontUnit()->getCurrVector();
}

bool Selection::hasUnit(const Unit* unit) const{
	return find(selectedUnits.begin(), selectedUnits.end(), unit)!=selectedUnits.end();
}

void Selection::assignGroup(int groupIndex){
	//clear group
	groups[groupIndex].clear();

	//assign new group
	for(int i=0; i<selectedUnits.size(); ++i){
		groups[groupIndex].push_back(selectedUnits[i]);
	}
}

void Selection::recallGroup(int groupIndex){
	clear();
	for(int i=0; i<groups[groupIndex].size(); ++i){
		select(groups[groupIndex][i]);
	}
}

void Selection::unitEvent(UnitObserver::Event event, const Unit *unit){

	if(event==UnitObserver::eKill){

		//remove from selection
		for(int i=0; i<selectedUnits.size(); ++i){
			if(selectedUnits[i]==unit){
				selectedUnits.erase(selectedUnits.begin()+i);
				break;
			}
		}

		//remove from groups
		for(int i=0; i<maxGroups; ++i){
			for(int j=0; j<groups[i].size(); ++j){
				if(groups[i][j]==unit){
					groups[i].erase(groups[i].begin()+j);
					break;
				}
			}
		}

		//notify gui
		gui->onSelectionChanged();

	}
}

}}//end namespace 
