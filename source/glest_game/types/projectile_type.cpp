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

#include "lang.h"
#include "leak_dumper.h"
#include "logger.h"
#include "renderer.h"
#include <cassert>

using namespace Shared::Util;

namespace Glest {
namespace Game {

ProjectileType::ProjectileType() {

  projectileParticleSystemType = NULL;
  attackStartTime = 0.0f;

  spawnUnit = "";
  spawnUnitcount = 0;
  spawnUnitAtTarget = false;

  shake = false;
  shakeIntensity = 0;
  shakeDuration = 0;

  shakeVisible = true;
  shakeInCameraView = true;
  shakeCameraDistanceAffected = false;
  damagePercentage = 100;
}

ProjectileType::~ProjectileType() {
  deleteValues(hitSounds.getSounds().begin(), hitSounds.getSounds().end());
  if (projectileParticleSystemType != NULL) {
    delete projectileParticleSystemType;
    projectileParticleSystemType = NULL;
  }
}

void ProjectileType::load(
    const XmlNode *projectileNode, const string &dir,
    const string &techtreepath,
    std::map<string, vector<pair<string, string>>> &loadedFileList,
    string parentLoader) {

  string currentPath = dir;
  endPathWithSlash(currentPath);

  if (projectileNode->hasAttribute("attack-start-time")) {
    attackStartTime =
        projectileNode->getAttribute("attack-start-time")->getFloatValue();
  } else {
    attackStartTime = 0.0f;
  }

  // damage percentage MUST be set!
  damagePercentage =
      projectileNode->getAttribute("damage-percentage")->getIntValue();

  // projectiles MUST have a particle system.
  const XmlNode *particleNode = projectileNode->getChild("particle");
  string path = particleNode->getAttribute("path")->getRestrictedValue();
  ParticleSystemTypeProjectile *projectileParticleSystemType =
      new ParticleSystemTypeProjectile();
  projectileParticleSystemType->load(particleNode, dir, currentPath + path,
                                     &Renderer::getInstance(), loadedFileList,
                                     parentLoader, techtreepath);
  loadedFileList[currentPath + path].push_back(make_pair(
      parentLoader, particleNode->getAttribute("path")->getRestrictedValue()));
  setProjectileParticleSystemType(projectileParticleSystemType);

  // spawnattack
  if (projectileNode->hasChild("unit")) {
    spawnUnit =
        projectileNode->getChild("unit")->getAttribute("value")->getValue();
    spawnUnitcount =
        projectileNode->getChild("unit")->getAttribute("amount")->getIntValue();
    if (projectileNode->getChild("unit")->hasAttribute("spawnAtTarget")) {
      spawnUnitAtTarget = projectileNode->getChild("unit")
                              ->getAttribute("spawnAtTarget")
                              ->getBoolValue();
    } else {
      spawnUnitAtTarget = false;
    }
  } else {
    spawnUnit = "";
    spawnUnitcount = 0;
    spawnUnitAtTarget = false;
  }

  if (projectileNode->hasChild("hitshake")) {
    const XmlNode *hitShakeNode = projectileNode->getChild("hitshake");
    shake = hitShakeNode->getAttribute("enabled")->getBoolValue();
    if (shake) {
      shakeIntensity = hitShakeNode->getAttribute("intensity")->getIntValue();
      shakeDuration = hitShakeNode->getAttribute("duration")->getIntValue();

      shakeVisible = hitShakeNode->getAttribute("visible")->getBoolValue();
      shakeInCameraView =
          hitShakeNode->getAttribute("in-camera-view")->getBoolValue();
      shakeCameraDistanceAffected =
          hitShakeNode->getAttribute("camera-distance-affected")
              ->getBoolValue();
    }
  }

  if (projectileNode->hasChild("hitsound")) {
    const XmlNode *soundNode = projectileNode->getChild("hitsound");
    if (soundNode->getAttribute("enabled")->getBoolValue()) {

      hitSounds.resize((int)soundNode->getChildCount());
      for (int i = 0; i < (int)soundNode->getChildCount(); ++i) {
        const XmlNode *soundFileNode = soundNode->getChild("sound-file", i);
        string path = soundFileNode->getAttribute("path")->getRestrictedValue(
            currentPath, true);
        // printf("\n\n\n\n!@#$ ---> parentLoader [%s] path [%s] nodeValue [%s]
        // i =
        // %d",parentLoader.c_str(),path.c_str(),soundFileNode->getAttribute("path")->getRestrictedValue().c_str(),i);

        StaticSound *sound = new StaticSound();
        sound->load(path);
        loadedFileList[path].push_back(make_pair(
            parentLoader,
            soundFileNode->getAttribute("path")->getRestrictedValue()));
        hitSounds[i] = sound;
      }
    }
  }
}

} // namespace Game
} // namespace Glest
