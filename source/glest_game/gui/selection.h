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

#ifndef _GLEST_GAME_SELECTION_
#define _GLEST_GAME_SELECTION_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "unit.h"
#include <vector>
#include "leak_dumper.h"

using std::vector;

namespace Glest{ namespace Game{

class Gui;
class World;

class HighlightSpecialUnitInfo {
public:
	float radius;
	float thickness;
	Vec4f color;
};

// =====================================================
// 	class Selection 
//
///	List of selected units and groups
// =====================================================

class Selection: public UnitObserver {
public:
	typedef vector<Unit*> UnitContainer;
	typedef UnitContainer::const_iterator UnitIterator;

public:
	static const int maxGroups	= 10;
	static const int maxUnits	= 36;

private:
	int factionIndex;
	int teamIndex;
	bool allowSharedTeamUnits;
	UnitContainer selectedUnits;
	UnitContainer groups[maxGroups];
	Gui *gui;

public:
	Selection() : UnitObserver() {
		factionIndex 			= 0;
		teamIndex 				= 0;
		allowSharedTeamUnits	= false;
		gui 					= NULL;
	}
	void init(Gui *gui, int factionIndex, int teamIndex, bool allowSharedTeamUnits);
	virtual ~Selection();

	bool select(Unit *unit, bool addToSelection);
	void select(const UnitContainer &units, bool addToSelection);
	void unSelect(const UnitContainer &units);
	void unSelect(int unitIndex);
	void clear();
	
	bool isEmpty() const				{return selectedUnits.empty();}
	bool isUniform() const;
	bool isEnemy() const;
	bool isObserver() const;

	bool isCommandable() const;
	bool isCancelable() const;
	bool isMeetable() const;
	int getCount() const				{return (int)selectedUnits.size();}
	const Unit *getUnit(int i) const	{return selectedUnits[i];}
	Unit *getUnitPtr(int i) 			{return selectedUnits[i];}
	const Unit *getFrontUnit() const	{return selectedUnits.front();}
	const Unit *getUnitFromCC(CommandClass commandClass);
	Vec3f getRefPos() const;
	bool hasUnit(const Unit* unit) const;
	
	bool assignGroup(int groupIndex, bool clearGroup=true, const UnitContainer *pUnits=NULL);
	bool addUnitToGroup(int groupIndex,Unit *unit);
	void removeUnitFromGroup(int groupIndex,int UnitId);
	void recallGroup(int groupIndex, bool clearSelection=true);

	//vector<Unit*> getUnitsForGroup(int groupIndex);

	virtual void unitEvent(UnitObserver::Event event, const Unit *unit);
	bool canSelectUnitFactionCheck(const Unit *unit) const;

	virtual void saveGame(XmlNode *rootNode) const;
	void loadGame(const XmlNode *rootNode, World *world);

};

}}//end namespace

#endif
