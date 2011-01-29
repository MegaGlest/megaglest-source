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

#ifndef _GLEST_GAME_UNIT_H_
#define _GLEST_GAME_UNIT_H_

#include "model.h"
#include "upgrade_type.h"
#include "particle.h"
#include "skill_type.h"
#include "game_constants.h"
#include <set>
#include "platform_common.h"
#include "leak_dumper.h"

namespace Glest { namespace Game {

using Shared::Graphics::ParticleSystem;
using Shared::Graphics::UnitParticleSystem;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Model;
using Shared::PlatformCommon::Chrono;
using Shared::PlatformCommon::ValueCheckerVault;

using std::set;

class Map;
class Faction;
class Unit;
class Command;
class SkillType;
class ResourceType;
class CommandType;
class SkillType;
class UnitType;
class TotalUpgrade;
class UpgradeType;
class Level;
class MorphCommandType;
class Game;

enum CommandResult{
	crSuccess,
	crFailRes,
	crFailReqs,
	crFailUnitCount,
	crFailUndefined,
	crSomeFailed
};

enum InterestingUnitType{
	iutIdleHarvester,
	iutBuiltBuilding,
	iutProducer,
	iutDamaged,
	iutStore
};

// =====================================================
// 	class UnitObserver
// =====================================================

class UnitObserver{
public:
	enum Event{
		eKill
	};

public:
	virtual ~UnitObserver() {}
	virtual void unitEvent(Event event, const Unit *unit)=0;
};

// =====================================================
// 	class UnitReference
// =====================================================

class UnitReference{
private:
	int id;
	Faction *faction;

public:
	UnitReference();

	void operator=(const Unit *unit);
	Unit *getUnit() const;

	int getUnitId() const			{ return id; }
	Faction *getUnitFaction() const	{ return faction; }
};

class UnitPathInterface {

public:

	virtual bool isBlocked() const = 0;
	virtual bool isEmpty() const = 0;
	virtual bool isStuck() const = 0;

	virtual void clear() = 0;
	virtual void clearBlockCount() = 0;
	virtual void incBlockCount() = 0;
	virtual void add(const Vec2i &path) = 0;
	//virtual Vec2i pop() = 0;
	virtual int getBlockCount() const = 0;
	virtual int getQueueCount() const = 0;

	virtual vector<Vec2i> getQueue() const = 0;

	virtual std::string toString() const = 0;
};

class UnitPathBasic : public UnitPathInterface {
private:
	static const int maxBlockCount;

private:
	int blockCount;
	vector<Vec2i> pathQueue;

public:
	UnitPathBasic();
	virtual bool isBlocked() const;
	virtual bool isEmpty() const;
	virtual bool isStuck() const;

	virtual void clear();
	virtual void clearBlockCount() { blockCount = 0; }
	virtual void incBlockCount();
	virtual void add(const Vec2i &path);
	Vec2i pop();
	virtual int getBlockCount() const { return blockCount; }
	virtual int getQueueCount() const { return pathQueue.size(); }

	virtual vector<Vec2i> getQueue() const { return pathQueue; }

	virtual std::string toString() const;
};

// =====================================================
// 	class UnitPath
// =====================================================
/** Holds the next cells of a Unit movement
  * @extends std::list<Shared::Math::Vec2i>
  */
class UnitPath : public list<Vec2i>, public UnitPathInterface {
private:
	static const int maxBlockCount = 10; /**< number of command updates to wait on a blocked path */

private:
	int blockCount;		/**< number of command updates this path has been blocked */

public:
	UnitPath() : blockCount(0) {} /**< Construct path object */
	virtual bool isBlocked() const	{return blockCount >= maxBlockCount;} /**< is this path blocked	   */
	virtual bool isEmpty() const	{return list<Vec2i>::empty();}	/**< is path empty				  */
	virtual bool isStuck() const	{return false; }

	int  size() const		{return list<Vec2i>::size();}	/**< size of path				 */
	virtual void clear()			{list<Vec2i>::clear(); blockCount = 0;} /**< clear the path		*/
	virtual void clearBlockCount() { blockCount = 0; }
	virtual void incBlockCount()	{++blockCount;}		   /**< increment block counter			   */
	virtual void push(Vec2i &pos)	{push_front(pos);}	  /**< push onto front of path			  */
	bool empty() const		{return list<Vec2i>::empty();}	/**< is path empty				  */
	virtual void add(const Vec2i &pos)	{ push_front(pos);}	  /**< push onto front of path			  */


#if 0
	// old style, to work with original PathFinder
	Vec2i peek()			{return back();}	 /**< peek at the next position			 */
	void pop()				{this->pop_back();}	/**< pop the next position off the path */
#else
	// new style, for the new RoutePlanner
	Vec2i peek()			{return front();}	 /**< peek at the next position			 */
	//virtual Vec2i pop()		{ Vec2i p= front(); erase(begin()); return p; }	/**< pop the next position off the path */
	void pop()		{ erase(begin()); }	/**< pop the next position off the path */
#endif
	virtual int getBlockCount() const { return blockCount; }
	virtual int getQueueCount() const { return this->size(); }

	virtual vector<Vec2i> getQueue() const {
		vector<Vec2i> result;
		for(list<Vec2i>::const_iterator iter = this->begin(); iter != this->end(); ++iter) {
			result.push_back(*iter);
		}
		return result;
	}

	virtual std::string toString() const;
};

class WaypointPath : public list<Vec2i> {
public:
	WaypointPath() {}
	void push(const Vec2i &pos)	{ push_front(pos); }
	Vec2i peek() const			{return front();}
	void pop()					{erase(begin());}
	void condense();
};


// ===============================
// 	class Unit
//
///	A game unit or building
// ===============================

class Unit : public ValueCheckerVault {
private:
    typedef list<Command*> Commands;
	typedef list<UnitObserver*> Observers;
	typedef vector<UnitParticleSystem*> UnitParticleSystems;

public:
	static const float speedDivider;
	static const int maxDeadCount;
	static const float highlightTime;
	static const int invalidId;

	static set<int> livingUnits;
	static set<Unit*> livingUnitsp;


private:
	const int id;
    int hp;
    int ep;
    int loadCount;
    int deadCount;
    float progress;			//between 0 and 1
	float lastAnimProgress;	//between 0 and 1
	float animProgress;		//between 0 and 1
	float highlight;
	int progress2;
	int kills;

	UnitReference targetRef;

	Field currField;
    Field targetField;
	const Level *level;

    Vec2i pos;
	Vec2i lastPos;
    Vec2i targetPos;		//absolute target pos
	Vec3f targetVec;
	Vec2i meetingPos;

	float lastRotation;		//in degrees
	float targetRotation;
	float rotation;

    const UnitType *type;
    const ResourceType *loadType;
    const SkillType *currSkill;

    bool toBeUndertaken;
	bool alive;
	bool showUnitParticles;

    Faction *faction;
	ParticleSystem *fire;
	TotalUpgrade totalUpgrade;
	Map *map;

	UnitPathInterface *unitPath;
	WaypointPath waypointPath;

    Commands commands;
	Observers observers;
	vector<UnitParticleSystem*> unitParticleSystems;
	UnitParticleSystems damageParticleSystems;
	vector<ParticleSystem*> fireParticleSystems;

	CardinalDir modelFacing;

	std::string lastSynchDataString;
	std::string lastFile;
	int lastLine;
	std::string lastSource;
	int lastRenderFrame;
	bool visible;

	int retryCurrCommandCount;

	Vec3f screenPos;
	string currentUnitTitle;

	bool inBailOutAttempt;
	// This buffer stores a list of bad harvest cells, along with the start
	// time of when it was detected. Typically this may be due to a unit
	// constantly getting blocked from getting to the resource so this
	// list may be used to tell areas of the game to ignore those cells for a
	// period of time
	//std::vector<std::pair<Vec2i,Chrono> > badHarvestPosList;
	std::map<Vec2i,int> badHarvestPosList;
	//time_t lastBadHarvestListPurge;
	std::pair<Vec2i,int> lastHarvestResourceTarget;

	std::pair<Vec2i,std::vector<Vec2i> > currentTargetPathTaken;

	static Game *game;

	bool ignoreCheckCommand;

public:
    Unit(int id, UnitPathInterface *path, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, CardinalDir placeFacing);
    ~Unit();

    static void setGame(Game *value) { game=value;}

    //queries
    void setIgnoreCheckCommand(bool value)      { ignoreCheckCommand=value;}
    bool getIgnoreCheckCommand() const			{return ignoreCheckCommand;}
	int getId() const							{return id;}
	Field getCurrField() const					{return currField;}
	int getLoadCount() const					{return loadCount;}
	float getLastAnimProgress() const			{return lastAnimProgress;}
	float getProgress() const					{return progress;}
	float getAnimProgress() const				{return animProgress;}
	float getHightlight() const					{return highlight;}
	int getProgress2() const					{return progress2;}
	int getFactionIndex() const;
	int getTeam() const;
	int getHp() const							{return hp;}
	int getEp() const							{return ep;}
	int getProductionPercent() const;
	float getHpRatio() const;
	float getEpRatio() const;
	bool getToBeUndertaken() const				{return toBeUndertaken;}
	Vec2i getTargetPos() const					{return targetPos;}
	Vec3f getTargetVec() const					{return targetVec;}
	Field getTargetField() const				{return targetField;}
	Vec2i getMeetingPos() const					{return meetingPos;}
	Faction *getFaction() const					{return faction;}
    const ResourceType *getLoadType() const		{return loadType;}
	const UnitType *getType() const				{return type;}
	const SkillType *getCurrSkill() const		{return currSkill;}
	const TotalUpgrade *getTotalUpgrade() const	{return &totalUpgrade;}
	float getRotation() const					{return rotation;}
	float getVerticalRotation() const;
	ParticleSystem *getFire() const				{return fire;}
	int getKills()								{return kills;}
	const Level *getLevel() const				{return level;}
	const Level *getNextLevel() const;
	string getFullName() const;
	const UnitPathInterface *getPath() const	{return unitPath;}
	UnitPathInterface *getPath()				{return unitPath;}
	WaypointPath *getWaypointPath()				{return &waypointPath;}

    //pos
	Vec2i getPos() const				{return pos;}
	Vec2i getLastPos() const			{return lastPos;}
	Vec2i getCenteredPos() const;
    Vec2f getFloatCenteredPos() const;
	Vec2i getCellPos() const;

    //is
	bool isHighlighted() const			{return highlight>0.f;}
	bool isDead() const					{return !alive;}
	bool isAlive() const				{return alive;}
    bool isOperative() const;
    bool isBeingBuilt() const;
    bool isBuilt() const;
    bool isPutrefacting() const;
	bool isAlly(const Unit *unit) const;
	bool isDamaged() const;
	bool isInteresting(InterestingUnitType iut) const;

    //set
	void setCurrField(Field currField)					{this->currField= currField;}
    void setCurrSkill(const SkillType *currSkill);
    void setCurrSkill(SkillClass sc);
	void setLoadCount(int loadCount)					{this->loadCount= loadCount;}
	void setLoadType(const ResourceType *loadType)		{this->loadType= loadType;}
	void setProgress2(int progress2)					{this->progress2= progress2;}
	void setPos(const Vec2i &pos);
	void setTargetPos(const Vec2i &targetPos);
	void setTarget(const Unit *unit);
	void setTargetVec(const Vec3f &targetVec);
	void setMeetingPos(const Vec2i &meetingPos);
	void setVisible(const bool visible);
	bool getVisible() const { return visible; }

	//render related
    const Model *getCurrentModel() const;
    Model *getCurrentModelPtr() const;
	Vec3f getCurrVector() const;
	Vec3f getCurrVectorFlat() const;

    //command related
	bool anyCommand(bool validateCommandtype=false) const;
	Command *getCurrCommand() const;
	void replaceCurrCommand(Command *cmd);
	int getCountOfProducedUnits(const UnitType *ut) const;
	unsigned int getCommandSize() const;
	CommandResult giveCommand(Command *command, bool tryQueue = false);		//give a command
	CommandResult finishCommand();						//command finished
	CommandResult cancelCommand();						//cancel canceled

	//lifecycle
	void create(bool startingUnit= false);
	void born();
	void kill();
	void undertake();

	//observers
	void addObserver(UnitObserver *unitObserver) ;
	void removeObserver(UnitObserver *unitObserver);
	void notifyObservers(UnitObserver::Event event);

    //other
	void resetHighlight();
	const CommandType *computeCommandType(const Vec2i &pos, const Unit *targetUnit= NULL) const;
	string getDesc() const;
    bool computeEp();
    bool repair();
    bool decHp(int i);
    int update2();
    bool update();
	void tick();
	void applyUpgrade(const UpgradeType *upgradeType);
	void computeTotalUpgrade();
	void incKills();
	bool morph(const MorphCommandType *mct);
	CommandResult checkCommand(Command *command) const;
	void applyCommand(Command *command);

	void setModelFacing(CardinalDir value);
	CardinalDir getModelFacing() const { return modelFacing; }

	bool isMeetingPointSettable() const;

	int getLastRenderFrame() const { return lastRenderFrame; }
	void setLastRenderFrame(int value) { lastRenderFrame = value; }

	int getRetryCurrCommandCount() const { return retryCurrCommandCount; }
	void setRetryCurrCommandCount(int value) { retryCurrCommandCount = value; }

	Vec3f getScreenPos() const { return screenPos; }
	void setScreenPos(Vec3f value) { screenPos = value; }

	string getCurrentUnitTitle() const {return currentUnitTitle;}
	void setCurrentUnitTitle(string value) { currentUnitTitle = value;}

	void exploreCells();

	bool getInBailOutAttempt() const { return inBailOutAttempt; }
	void setInBailOutAttempt(bool value) { inBailOutAttempt = value; }

	//std::vector<std::pair<Vec2i,Chrono> > getBadHarvestPosList() const { return badHarvestPosList; }
	//void setBadHarvestPosList(std::vector<std::pair<Vec2i,Chrono> > value) { badHarvestPosList = value; }
	void addBadHarvestPos(const Vec2i &value);
	void removeBadHarvestPos(const Vec2i &value);
	bool isBadHarvestPos(const Vec2i &value,bool checkPeerUnits=true) const;
	void cleanupOldBadHarvestPos();

	void setLastHarvestResourceTarget(const Vec2i *pos);
	std::pair<Vec2i,int> getLastHarvestResourceTarget() const { return lastHarvestResourceTarget;}

	std::pair<Vec2i,std::vector<Vec2i> > getCurrentTargetPathTaken() const { return currentTargetPathTaken; }
	void addCurrentTargetPathTakenCell(const Vec2i &target,const Vec2i &cell);

	void logSynchData(string file,int line,string source="");
	std::string toString() const;

private:
	float computeHeight(const Vec2i &pos) const;
	void updateTarget();
	void clearCommands();
	void deleteQueuedCommand(Command *command);
	CommandResult undoCommand(Command *command);
	void stopDamageParticles();
	void startDamageParticles();

	int getFrameCount();
};

}}// end namespace

#endif
