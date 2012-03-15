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

#ifndef _GLEST_GAME_GUI_H_
#define _GLEST_GAME_GUI_H_

#include "resource.h"
#include "command_type.h"
#include "display.h"
#include "commander.h"
#include "console.h"
#include "selection.h"
#include "randomgen.h"
#include <map>
#include "object.h"
#include "platform_common.h"
#include "leak_dumper.h"

using Shared::Util::RandomGen;

namespace Glest{ namespace Game{

class Unit;
class World;
class CommandType;
class GameCamera;
class Game;

enum DisplayState{
	dsEmpty,
	dsUnitSkills,
	dsUnitBuild,
	dsEnemy
};

// =====================================================
//	class Mouse3d
// =====================================================

class Mouse3d{
public:
	static const float fadeSpeed;

private:
	bool enabled;
	int rot;
	float fade;

public:
	Mouse3d();

	void enable();
	void update();

	bool isEnabled() const	{return enabled;}
	float getFade() const	{return fade;}
	int getRot() const		{return rot;}
};

// =====================================================
//	class SelectionQuad
// =====================================================

class SelectionQuad{
private:
	Vec2i posDown;
	Vec2i posUp;
	bool enabled;

public:
	SelectionQuad();

	bool isEnabled() const		{return enabled;}
	Vec2i getPosDown() const	{return posDown;}
	Vec2i getPosUp() const		{return posUp;}

	void setPosDown(const Vec2i &posDown);
	void setPosUp(const Vec2i &posUp);
	void disable();
};

// =====================================================
// 	class Gui
//
///	In game GUI
// =====================================================

class Gui : public ObjectStateInterface {
public:
	static const int maxSelBuff= 128*5;
	static const int upgradeDisplayIndex= 8;

	static const int meetingPointPos= 14;
	static const int cancelPos= 15;
	static const int imageCount= 16;

	static const int invalidPos= -1;
	static const int doubleClickSelectionRadius= 20;

private:
	//External objects
	RandomGen random;
	const Commander *commander;
	const World *world;
	const Game *game;
	GameCamera *gameCamera;
	Console *console;

	//Positions
	Vec2i posObjWorld;		//world coords
	bool validPosObjWorld;

	//display
	const UnitType *choosenBuildingType;
	const CommandType *activeCommandType;
	CommandClass activeCommandClass;
	int activePos;
	int lastPosDisplay;

	//composite
	Display display;
	Mouse3d mouse3d;
	Selection selection;
	SelectionQuad selectionQuad;
	int lastQuadCalcFrame;
	int selectionCalculationFrameSkip;
	int minQuadSize;

	Chrono lastGroupRecallTime;
	int lastGroupRecall;

	//states
	bool selectingBuilding;
	bool selectingPos;
	bool selectingMeetingPoint;

	CardinalDir selectedBuildingFacing;
	const Object *selectedResourceObject;

	Texture2D* hudTexture;

public:
	Gui();
	void init(Game *game);
	void end();

	// callback when tileset objects are removed from the world
	virtual void removingObjectEvent(Object* o) ;

	//get
	Vec2i getPosObjWorld() const			{return posObjWorld;}
	const UnitType *getBuilding() const;

	Texture2D *getHudTexture() const {return hudTexture;}
	void setHudTexture(Texture2D* value) { hudTexture = value;}

	const Mouse3d *getMouse3d() const				{return &mouse3d;}
	const Display *getDisplay()	const				{return &display;}
	const Selection *getSelection()	const			{return &selection;}
	const Object *getSelectedResourceObject()	const			{return selectedResourceObject;}

	const SelectionQuad *getSelectionQuad() const	{return &selectionQuad;}
	CardinalDir getSelectedFacing() const			{return selectedBuildingFacing;}
	bool isSelected(const Unit *unit) const			{return selection.hasUnit(unit);}

	bool isValidPosObjWorld() const		{return validPosObjWorld;}
	bool isSelecting() const			{return selectionQuad.isEnabled();}
	bool isSelectingPos() const			{return selectingPos;}
	bool isSelectingBuilding() const	{return selectingBuilding;}
	bool isPlacingBuilding() const;

	//set
	void invalidatePosObjWorld();

	//events
	void update();
	void tick();
	bool mouseValid(int x, int y);
	void mouseDownLeftDisplay(int x, int y);
	void mouseMoveDisplay(int x, int y);
	void mouseMoveOutsideDisplay();
	void mouseDownLeftGraphics(int x, int y, bool prepared);
	void mouseDownRightGraphics(int x, int y, bool prepared);
	void mouseUpLeftGraphics(int x, int y);
	void mouseMoveGraphics(int x, int y);
	void mouseDoubleClickLeftGraphics(int x, int y);
	void groupKey(int groupIndex);
	void hotKey(SDL_KeyboardEvent key);

	//misc
	void switchToNextDisplayColor();
	void onSelectionChanged();

	void saveGame(XmlNode *rootNode) const;
	void loadGame(const XmlNode *rootNode, World *world);

private:

	//orders
	void giveDefaultOrders(int x, int y);
	void giveDefaultOrders(int x, int y, const Unit *targetUnit, bool paintMouse3d);
	void givePreparedDefaultOrders(int x, int y);
	void giveOneClickOrders();
	void giveTwoClickOrders(int x, int y, bool prepared);

	//hotkeys
	void centerCameraOnSelection();
	void selectInterestingUnit(InterestingUnitType iut);
	void clickCommonCommand(CommandClass commandClass);

	//misc
	int computePosDisplay(int x, int y);
	void computeDisplay();
	void resetState();
	void mouseDownDisplayUnitSkills(int posDisplay);
	void mouseDownDisplayUnitBuild(int posDisplay);
	void computeInfoString(int posDisplay);
	string computeDefaultInfoString();
	void addOrdersResultToConsole(CommandClass cc, CommandResult rr);
	bool isSharedCommandClass(CommandClass commandClass);
	void computeSelected(bool doubleCkick,bool force);
	bool computeTarget(const Vec2i &screenPos, Vec2i &targetPos, const Unit *&targetUnit);
	Unit* getRelevantObjectFromSelection(Selection::UnitContainer *uc);
};

}} //end namespace

#endif
