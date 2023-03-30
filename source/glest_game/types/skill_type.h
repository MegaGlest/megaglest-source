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

#ifndef _GLEST_GAME_SKILLTYPE_H_
#define _GLEST_GAME_SKILLTYPE_H_

#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
#endif

#include "damage_multiplier.h"
#include "element_type.h"
#include "factory.h"
#include "leak_dumper.h"
#include "model.h"
#include "particle.h"
#include "projectile_type.h"
#include "sound.h"
#include "sound_container.h"
#include "upgrade_type.h"
#include "util.h"
#include "vec.h"
#include "xml_parser.h"
#include <set>

using Shared::Graphics::Model;
using Shared::Graphics::Vec3f;
using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;

namespace Glest {
namespace Game {

using Shared::Util::MultiFactory;

class ParticleSystemTypeProjectile;
class ParticleSystemTypeSplash;
class UnitParticleSystemType;
class FactionType;
class TechTree;
class Lang;
class TotalUpgrade;
class Unit;

enum Field {
  fLand,
  fAir,

  fieldCount
};

enum SkillClass {
  scStop,
  scMove,
  scAttack,
  scBuild,
  scHarvest,
  scRepair,
  scBeBuilt,
  scProduce,
  scUpgrade,
  scMorph,
  scDie,
  scFogOfWar,

  scCount
};

typedef list<UnitParticleSystemType *> UnitParticleSystemTypes;
typedef list<ProjectileType *> ProjectileTypes;
// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
// =====================================================

enum AttackBoostTargetType {
  abtAlly,      // Only ally units are affected
  abtFoe,       // Only foe units are affected
  abtFaction,   // Only same faction units are affected
  abtUnitTypes, // Specify which units are affected ( in general same as abtAll
                // )
  abtAll        // All units are affected (including enemies)
};

class AttackBoost {
public:
  AttackBoost();
  virtual ~AttackBoost();
  bool enabled;
  bool allowMultipleBoosts;
  int radius;
  AttackBoostTargetType targetType;
  std::set<const UnitType *> boostUnitList;
  std::set<string> tags;
  UpgradeTypeBase boostUpgrade;

  UnitParticleSystemType *unitParticleSystemTypeForSourceUnit;
  UnitParticleSystemType *unitParticleSystemTypeForAffectedUnit;

  bool includeSelf;
  string name;

  bool isAffected(const Unit *source, const Unit *dest) const;
  virtual string getDesc(bool translatedValue) const;
  string getTagName(string tag, bool translatedValue = false) const;

  virtual void saveGame(XmlNode *rootNode) const;
  virtual void loadGame(const XmlNode *rootNode, Faction *faction,
                        const SkillType *skillType);

private:
  /**
   * Checks if a unit is affected by the attack boost by checking if either the
   * UnitType is in the #boostUnitList or shares a tag with #tags.
   * @param unitType The unit type to check.
   * @return True if the unit *might* be affected by the attack boost (still
   * have to check if it's in range), false otherwise.
   */
  bool isInUnitListOrTags(const UnitType *unitType) const;
};

class AnimationAttributes {
public:
  AnimationAttributes() {
    fromHp = 0;
    toHp = 0;
  }

  int fromHp;
  int toHp;
};

// =====================================================
// 	class SkillSound
// 	holds the start time and a SoundContainer
// =====================================================

class SkillSound {
private:
  SoundContainer soundContainer;
  float startTime;

public:
  SkillSound();
  ~SkillSound();

  SoundContainer *getSoundContainer() { return &soundContainer; }
  float getStartTime() const { return startTime; }
  void setStartTime(float value) { startTime = value; }
};

typedef list<SkillSound *> SkillSoundList;

class SkillType {

protected:
  SkillClass skillClass;
  string name;
  int mpCost;
  int hpCost;
  int speed;
  int animSpeed;

  bool shake;
  int shakeIntensity;
  int shakeDuration;
  float shakeStartTime;
  bool shakeSelfEnabled;
  bool shakeSelfVisible;
  bool shakeSelfInCameraView;
  bool shakeSelfCameraAffected;
  bool shakeTeamEnabled;
  bool shakeTeamVisible;
  bool shakeTeamInCameraView;
  bool shakeTeamCameraAffected;
  bool shakeEnemyEnabled;
  bool shakeEnemyVisible;
  bool shakeEnemyInCameraView;
  bool shakeEnemyCameraAffected;

  int animationRandomCycleMaxcount;
  vector<Model *> animations;
  vector<AnimationAttributes> animationAttributes;

  SkillSoundList skillSoundList;
  RandomGen random;
  AttackBoost attackBoost;

  static int nextAttackBoostId;
  static int getNextAttackBoostId() { return ++nextAttackBoostId; }

  const XmlNode *findAttackBoostDetails(string attackBoostName,
                                        const XmlNode *attackBoostsNode,
                                        const XmlNode *attackBoostNode);
  void loadAttackBoost(
      const XmlNode *attackBoostsNode, const XmlNode *attackBoostNode,
      const FactionType *ft, string parentLoader, const string &dir,
      string currentPath,
      std::map<string, vector<pair<string, string>>> &loadedFileList,
      const TechTree *tt);

public:
  UnitParticleSystemTypes unitParticleSystemTypes;

public:
  // varios
  virtual ~SkillType();
  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);

  bool CanCycleNextRandomAnimation(const int *animationRandomCycleCount) const;

  static void resetNextAttackBoostId() { nextAttackBoostId = 0; }

  const AnimationAttributes getAnimationAttribute(int index) const;
  int getAnimationCount() const { return (int)animations.size(); }

  // get
  const string &getName() const { return name; }
  SkillClass getClass() const { return skillClass; }
  int getEpCost() const { return mpCost; }
  int getHpCost() const { return hpCost; }
  int getSpeed() const { return speed; }
  int getAnimSpeed() const { return animSpeed; }
  Model *getAnimation(float animProgress = 0, const Unit *unit = NULL,
                      int *lastAnimationIndex = NULL,
                      int *animationRandomCycleCount = NULL) const;

  float getShakeStartTime() const { return shakeStartTime; }
  bool getShake() const { return shake; }
  int getShakeIntensity() const { return shakeIntensity; }
  int getShakeDuration() const { return shakeDuration; }

  const SkillSoundList *getSkillSoundList() const { return &skillSoundList; }

  bool getShakeSelfEnabled() const { return shakeSelfEnabled; }
  bool getShakeSelfVisible() const { return shakeSelfVisible; }
  bool getShakeSelfInCameraView() const { return shakeSelfInCameraView; }
  bool getShakeSelfCameraAffected() const { return shakeSelfCameraAffected; }
  bool getShakeTeamEnabled() const { return shakeTeamEnabled; }
  bool getShakeTeamVisible() const { return shakeTeamVisible; }
  bool getShakeTeamInCameraView() const { return shakeTeamInCameraView; }
  bool getShakeTeamCameraAffected() const { return shakeTeamCameraAffected; }
  bool getShakeEnemyEnabled() const { return shakeEnemyEnabled; }
  bool getShakeEnemyVisible() const { return shakeEnemyVisible; }
  bool getShakeEnemyInCameraView() const { return shakeEnemyInCameraView; }
  bool getShakeEnemyCameraAffected() const { return shakeEnemyCameraAffected; }

  bool isAttackBoostEnabled() const { return attackBoost.enabled; }
  const AttackBoost *getAttackBoost() const { return &attackBoost; }
  // virtual string getDesc(const TotalUpgrade *totalUpgrade) const= 0;

  // other
  virtual string toString(bool translatedValue) const = 0;
  virtual int getTotalSpeed(const TotalUpgrade *) const { return speed; }
  static string skillClassToStr(SkillClass skillClass);
  static string fieldToStr(Field field);
  virtual string getBoostDesc(bool translatedValue) const {
    return attackBoost.getDesc(translatedValue);
  }

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class StopSkillType
// ===============================

class StopSkillType : public SkillType {
public:
  StopSkillType();
  virtual string toString(bool translatedValue) const;
};

// ===============================
// 	class MoveSkillType
// ===============================

class MoveSkillType : public SkillType {
public:
  MoveSkillType();
  virtual string toString(bool translatedValue) const;

  virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;
};

// ===============================
// 	class AttackSkillType
// ===============================

class AttackSkillType : public SkillType {
public:
  ProjectileTypes projectileTypes;

private:
  int attackStrength;
  int attackVar;
  int attackRange;
  const AttackType *attackType;
  bool attackFields[fieldCount];
  float attackStartTime;

  string spawnUnit;
  int spawnUnitcount;
  bool spawnUnitAtTarget;
  bool projectile;
  // ParticleSystemTypeProjectile* projectileParticleSystemType;
  SoundContainer projSounds;

  bool splash;
  int splashRadius;
  bool splashDamageAll;
  ParticleSystemTypeSplash *splashParticleSystemType;

public:
  AttackSkillType();
  ~AttackSkillType();
  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);
  virtual string toString(bool translatedValue) const;

  // get
  inline int getAttackStrength() const { return attackStrength; }
  inline int getAttackVar() const { return attackVar; }
  inline int getAttackRange() const { return attackRange; }
  inline const AttackType *getAttackType() const { return attackType; }
  inline bool getAttackField(Field field) const { return attackFields[field]; }
  inline float getAttackStartTime() const { return attackStartTime; }
  inline string getSpawnUnit() const { return spawnUnit; }
  inline int getSpawnUnitCount() const { return spawnUnitcount; }
  inline bool getSpawnUnitAtTarget() const { return spawnUnitAtTarget; }

  // get proj
  inline bool getProjectile() const { return projectile; }
  inline StaticSound *getProjSound() const { return projSounds.getRandSound(); }

  // get splash
  inline bool getSplash() const { return splash; }
  inline int getSplashRadius() const { return splashRadius; }
  inline bool getSplashDamageAll() const { return splashDamageAll; }
  inline ParticleSystemTypeSplash *getSplashParticleType() const {
    return splashParticleSystemType;
  }

  // misc
  int getTotalAttackStrength(const TotalUpgrade *totalUpgrade) const;
  int getTotalAttackRange(const TotalUpgrade *totalUpgrade) const;
  virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;
  virtual int getAnimSpeedBoost(const TotalUpgrade *totalUpgrade) const;

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class BuildSkillType
// ===============================

class BuildSkillType : public SkillType {
public:
  BuildSkillType();
  virtual string toString(bool translatedValue) const;
};

// ===============================
// 	class HarvestSkillType
// ===============================

class HarvestSkillType : public SkillType {
public:
  HarvestSkillType();
  virtual string toString(bool translatedValue) const;
};

// ===============================
// 	class RepairSkillType
// ===============================

class RepairSkillType : public SkillType {
public:
  RepairSkillType();
  virtual string toString(bool translatedValue) const;
};

// ===============================
// 	class ProduceSkillType
// ===============================

class ProduceSkillType : public SkillType {
private:
  bool animProgressBound;

public:
  ProduceSkillType();
  bool getAnimProgressBound() const { return animProgressBound; }
  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);

  virtual string toString(bool translatedValue) const;

  virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class UpgradeSkillType
// ===============================

class UpgradeSkillType : public SkillType {
private:
  bool animProgressBound;

public:
  UpgradeSkillType();
  bool getAnimProgressBound() const { return animProgressBound; }
  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);

  virtual string toString(bool translatedValue) const;

  virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class BeBuiltSkillType
// ===============================

class BeBuiltSkillType : public SkillType {
private:
  bool animProgressBound;

public:
  BeBuiltSkillType();
  bool getAnimProgressBound() const { return animProgressBound; }

  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);
  virtual string toString(bool translatedValue) const;

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class MorphSkillType
// ===============================

class MorphSkillType : public SkillType {
private:
  bool animProgressBound;

public:
  MorphSkillType();
  bool getAnimProgressBound() const { return animProgressBound; }

  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);

  virtual string toString(bool translatedValue) const;
  virtual int getTotalSpeed(const TotalUpgrade *totalUpgrade) const;

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class DieSkillType
// ===============================

class DieSkillType : public SkillType {
private:
  bool fade;
  bool spawn;
  float spawnStartTime;
  string spawnUnit;
  int spawnUnitcount;
  int spawnUnitHealthPercentMin;
  int spawnUnitHealthPercentMax;
  int spawnProbability;

public:
  DieSkillType();

  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);
  virtual string toString(bool translatedValue) const;

  bool getFade() const { return fade; }
  bool getSpawn() const { return spawn; }
  inline int getSpawnStartTime() const { return spawnStartTime; }
  inline string getSpawnUnit() const { return spawnUnit; }
  inline int getSpawnUnitCount() const { return spawnUnitcount; }
  inline int getSpawnUnitHealthPercentMin() const {
    return spawnUnitHealthPercentMin;
  }
  inline int getSpawnUnitHealthPercentMax() const {
    return spawnUnitHealthPercentMax;
  }
  inline int getSpawnProbability() const { return spawnProbability; }

  virtual void saveGame(XmlNode *rootNode);
  StaticSound *getSound() const;
};

// ===============================
// 	class FogOfWarSkillType
// ===============================

class FogOfWarSkillType : public SkillType {
private:
  bool fowEnable;
  bool applyToTeam;
  float durationTime;

public:
  FogOfWarSkillType();
  bool getFowEnable() const { return fowEnable; }
  bool getApplyToTeam() const { return applyToTeam; }
  float getDurationTime() const { return durationTime; }

  virtual void
  load(const XmlNode *sn, const XmlNode *attackBoostsNode, const string &dir,
       const TechTree *tt, const FactionType *ft,
       std::map<string, vector<pair<string, string>>> &loadedFileList,
       string parentLoader);
  virtual string toString(bool translatedValue) const;

  virtual void saveGame(XmlNode *rootNode);
};

// ===============================
// 	class SkillFactory
// ===============================

class SkillTypeFactory : public MultiFactory<SkillType> {
private:
  SkillTypeFactory();

public:
  static SkillTypeFactory &getInstance();
};

} // namespace Game
} // namespace Glest

#endif
