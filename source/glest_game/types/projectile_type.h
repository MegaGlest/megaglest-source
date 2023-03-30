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

#ifndef _GLEST_GAME_PROJEKTILETYPE_H_
#define _GLEST_GAME_PROJEKTILETYPE_H_

#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
#endif

#include "sound.h"
#include "vec.h"
// #include "xml_parser.h"
#include "util.h"
// #include "element_type.h"
#include "factory.h"
#include "leak_dumper.h"
#include "particle_type.h"
#include "sound_container.h"

using std::string;
using std::vector;

namespace Glest {
namespace Game {
// =====================================================
// 	class ProjectileType
// =====================================================

class ProjectileType {
protected:
  ParticleSystemTypeProjectile *projectileParticleSystemType;
  SoundContainer hitSounds;
  float attackStartTime;

  string spawnUnit;
  int spawnUnitcount;
  bool spawnUnitAtTarget;

  bool shake;
  int shakeIntensity;
  int shakeDuration;

  bool shakeVisible;
  bool shakeInCameraView;
  bool shakeCameraDistanceAffected;
  int damagePercentage;

public:
  ProjectileType();
  virtual ~ProjectileType();

  void load(const XmlNode *projectileNode, const string &dir,
            const string &techtreepath,
            std::map<string, vector<pair<string, string>>> &loadedFileList,
            string parentLoader);

  // get/set
  inline StaticSound *getHitSound() const { return hitSounds.getRandSound(); }
  ParticleSystemTypeProjectile *getProjectileParticleSystemType() const {
    return projectileParticleSystemType;
  }
  float getAttackStartTime() const { return attackStartTime; }
  void setAttackStartTime(float value) { attackStartTime = value; }

  string getSpawnUnit() const { return spawnUnit; }
  int getSpawnUnitcount() const { return spawnUnitcount; }
  bool getSpawnUnitAtTarget() const { return spawnUnitAtTarget; }

  bool isShake() const { return shake; }
  bool isShakeCameraDistanceAffected() const {
    return shakeCameraDistanceAffected;
  }
  int getShakeDuration() const { return shakeDuration; }
  bool isShakeInCameraView() const { return shakeInCameraView; }
  int getShakeIntensity() const { return shakeIntensity; }
  bool isShakeVisible() const { return shakeVisible; }
  int getDamagePercentage() const { return damagePercentage; }
  void setDamagePercentage(int value) { damagePercentage = value; }

  void setProjectileParticleSystemType(ParticleSystemTypeProjectile *pointer) {
    projectileParticleSystemType = pointer;
  }
  ParticleSystemTypeProjectile *getProjectileParticleSystemType() {
    return projectileParticleSystemType;
  }
};

} // namespace Game
} // namespace Glest

#endif
