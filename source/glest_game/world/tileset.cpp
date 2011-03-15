// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "tileset.h"

#include <cassert>
#include <ctime>

#include "logger.h"
#include "util.h"
#include "renderer.h"
#include "game_util.h"
#include "leak_dumper.h"
#include "platform_util.h"

using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class AmbientSounds
// =====================================================

void AmbientSounds::load(const string &dir, const XmlNode *xmlNode,
		std::map<string,int> &loadedFileList) {
	string path;

	//day
	const XmlNode *dayNode= xmlNode->getChild("day-sound");
	enabledDay= dayNode->getAttribute("enabled")->getBoolValue();
	if(enabledDay) {
		path= dayNode->getAttribute("path")->getRestrictedValue();
		string currentPath = dir;
		endPathWithSlash(currentPath);
		day.open(currentPath + path);
		alwaysPlayDay= dayNode->getAttribute("play-always")->getBoolValue();
	}

	//night
	const XmlNode *nightNode= xmlNode->getChild("night-sound");
	enabledNight= nightNode->getAttribute("enabled")->getBoolValue();
	if(enabledNight){
		path= nightNode->getAttribute("path")->getRestrictedValue();
		string currentPath = dir;
		endPathWithSlash(currentPath);
		night.open(currentPath + path);
		alwaysPlayNight= nightNode->getAttribute("play-always")->getBoolValue();
	}

	//rain
	const XmlNode *rainNode= xmlNode->getChild("rain-sound");
	enabledRain= rainNode->getAttribute("enabled")->getBoolValue();
	if(enabledRain){
		path= rainNode->getAttribute("path")->getRestrictedValue();
		string currentPath = dir;
		endPathWithSlash(currentPath);
		rain.open(currentPath + path);
	}

	//snow
	const XmlNode *snowNode= xmlNode->getChild("snow-sound");
	enabledSnow= snowNode->getAttribute("enabled")->getBoolValue();
	if(enabledSnow){
		path= snowNode->getAttribute("path")->getRestrictedValue();
		string currentPath = dir;
		endPathWithSlash(currentPath);
		snow.open(currentPath + path);
	}

	//dayStart
	const XmlNode *dayStartNode= xmlNode->getChild("day-start-sound");
	enabledDayStart= dayStartNode->getAttribute("enabled")->getBoolValue();
	if(enabledDayStart){
		path= dayStartNode->getAttribute("path")->getRestrictedValue();
		string currentPath = dir;
		endPathWithSlash(currentPath);
		dayStart.load(currentPath + path);
		loadedFileList[currentPath + path]++;
	}

	//nightStart
	const XmlNode *nightStartNode= xmlNode->getChild("night-start-sound");
	enabledNightStart= nightStartNode->getAttribute("enabled")->getBoolValue();
	if(enabledNightStart){
		path= nightStartNode->getAttribute("path")->getRestrictedValue();
		string currentPath = dir;
		endPathWithSlash(currentPath);
		nightStart.load(currentPath + path);
		loadedFileList[currentPath + path]++;
	}
}

// =====================================================
// 	class Tileset
// =====================================================

Checksum Tileset::loadTileset(const vector<string> pathList, const string &tilesetName,
		Checksum* checksum, std::map<string,int> &loadedFileList) {
    Checksum tilesetChecksum;
    for(int idx = 0; idx < pathList.size(); idx++) {
		string currentPath = pathList[idx];
		endPathWithSlash(currentPath);
        string path = currentPath + tilesetName;
        if(isdir(path.c_str()) == true) {
            load(path, checksum, &tilesetChecksum, loadedFileList);
            break;
        }
    }
    return tilesetChecksum;
}


void Tileset::load(const string &dir, Checksum *checksum, Checksum *tilesetChecksum,
		std::map<string,int> &loadedFileList) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	random.init(time(NULL));

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	string name= lastDir(dir);
	string currentPath = dir;
	endPathWithSlash(currentPath);
	string path= currentPath + name + ".xml";

	checksum->addFile(path);
	tilesetChecksum->addFile(path);
	checksumValue.addFile(path);

	try {
		Logger::getInstance().add("Tileset: "+formatString(name), true);
		Renderer &renderer= Renderer::getInstance();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//parse xml
		XmlTree xmlTree;
		xmlTree.load(path);
		loadedFileList[path]++;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		const XmlNode *tilesetNode= xmlTree.getRootNode();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//surfaces
		const XmlNode *surfacesNode= tilesetNode->getChild("surfaces");
		for(int i=0; i<surfCount; ++i){
			const XmlNode *surfaceNode;
			if(surfacesNode->hasChildAtIndex("surface",i)){
				surfaceNode= surfacesNode->getChild("surface", i);
			}
			else {
				// cliff texture does not exist, use texture 2 instead
				surfaceNode= surfacesNode->getChild("surface", 2);
			}


			int childCount= surfaceNode->getChildCount();
			surfPixmaps[i].resize(childCount);
			surfProbs[i].resize(childCount);
			for(int j=0; j<childCount; ++j){
				const XmlNode *textureNode= surfaceNode->getChild("texture", j);
				surfPixmaps[i][j].init(3);
				surfPixmaps[i][j].load(currentPath + textureNode->getAttribute("path")->getRestrictedValue());
				loadedFileList[currentPath + textureNode->getAttribute("path")->getRestrictedValue()]++;

				surfProbs[i][j]= textureNode->getAttribute("prob")->getFloatValue();
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//object models
		const XmlNode *objectsNode= tilesetNode->getChild("objects");
		for(int i=0; i<objCount; ++i){
			const XmlNode *objectNode= objectsNode->getChild("object", i);
			int childCount= objectNode->getChildCount();

			int objectHeight = 0;
			bool walkable = objectNode->getAttribute("walkable")->getBoolValue();
			if(walkable == false) {
				const XmlAttribute *heightAttribute = objectNode->getAttribute("height",false);
				if(heightAttribute != NULL) {
					objectHeight = heightAttribute->getIntValue();
				}
			}

			objectTypes[i].init(childCount, i, walkable,objectHeight);
			for(int j=0; j<childCount; ++j) {
				const XmlNode *modelNode= objectNode->getChild("model", j);
				const XmlAttribute *pathAttribute= modelNode->getAttribute("path");
				objectTypes[i].loadModel(currentPath + pathAttribute->getRestrictedValue(),&loadedFileList);
				loadedFileList[currentPath + pathAttribute->getRestrictedValue()]++;

				if(modelNode->hasChild("particles")){
					const XmlNode *particleNode= modelNode->getChild("particles");
					bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
					if(particleEnabled){
						for(int k=0; k<particleNode->getChildCount(); ++k){
							const XmlNode *particleFileNode= particleNode->getChild("particle-file", k);
							string path= particleFileNode->getAttribute("path")->getRestrictedValue();
							ObjectParticleSystemType *objectParticleSystemType= new ObjectParticleSystemType();
							objectParticleSystemType->load(dir,  currentPath + path,
									&Renderer::getInstance(), loadedFileList);
							loadedFileList[currentPath + path]++;

							objectTypes[i].addParticleSystem((objectParticleSystemType));
						}
					}
				}
			}
		}

		// Now free up the pixmap memory
		for(int i=0; i<objCount; ++i){
			objectTypes[i].deletePixels();
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//ambient sounds
		ambientSounds.load(dir, tilesetNode->getChild("ambient-sounds"), loadedFileList);

		//parameters
		const XmlNode *parametersNode= tilesetNode->getChild("parameters");

		//water
		const XmlNode *waterNode= parametersNode->getChild("water");
		waterTex= renderer.newTexture3D(rsGame);
		waterTex->setMipmap(false);
		waterTex->setWrapMode(Texture::wmRepeat);
		waterEffects= waterNode->getAttribute("effects")->getBoolValue();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		int waterFrameCount= waterNode->getChildCount();
		waterTex->getPixmap()->init(waterFrameCount, 4);
		for(int i=0; i<waterFrameCount; ++i){
			const XmlNode *waterFrameNode= waterNode->getChild("texture", i);
			waterTex->getPixmap()->loadSlice(currentPath + waterFrameNode->getAttribute("path")->getRestrictedValue(), i);
			loadedFileList[currentPath + waterFrameNode->getAttribute("path")->getRestrictedValue()]++;
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//fog
		const XmlNode *fogNode= parametersNode->getChild("fog");
		fog= fogNode->getAttribute("enabled")->getBoolValue();
		if(fog){
			fogMode= fogNode->getAttribute("mode")->getIntValue(1, 2);
			fogDensity= fogNode->getAttribute("density")->getFloatValue();
			fogColor.x= fogNode->getAttribute("color-red")->getFloatValue(0.f, 1.f);
			fogColor.y= fogNode->getAttribute("color-green")->getFloatValue(0.f, 1.f);
			fogColor.z= fogNode->getAttribute("color-blue")->getFloatValue(0.f, 1.f);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//sun and moon light colors
		const XmlNode *sunLightColorNode= parametersNode->getChild("sun-light");
		sunLightColor.x= sunLightColorNode->getAttribute("red")->getFloatValue();
		sunLightColor.y= sunLightColorNode->getAttribute("green")->getFloatValue();
		sunLightColor.z= sunLightColorNode->getAttribute("blue")->getFloatValue();

		const XmlNode *moonLightColorNode= parametersNode->getChild("moon-light");
		moonLightColor.x= moonLightColorNode->getAttribute("red")->getFloatValue();
		moonLightColor.y= moonLightColorNode->getAttribute("green")->getFloatValue();
		moonLightColor.z= moonLightColorNode->getAttribute("blue")->getFloatValue();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//weather
		const XmlNode *weatherNode= parametersNode->getChild("weather");
		float sunnyProb= weatherNode->getAttribute("sun")->getFloatValue(0.f, 1.f);
		float rainyProb= weatherNode->getAttribute("rain")->getFloatValue(0.f, 1.f) + sunnyProb;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

#ifdef USE_STREFLOP
		float rnd= streflop::fabs(random.randRange(-1.f, 1.f));
#else
		float rnd= fabs(random.randRange(-1.f, 1.f));
#endif

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(rnd < sunnyProb) {
			weather= wSunny;
		}
		else if(rnd < rainyProb) {
			weather= wRainy;
		}
		else {
			weather= wSnowy;
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	}
	//Exception handling (conversions and so on);
	catch(const exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error: " + path + "\n" + e.what());
	}
}

Tileset::~Tileset() {
	Logger::getInstance().add("Tileset", true);
}

const Pixmap2D *Tileset::getSurfPixmap(int type, int var) const{
	int vars= surfPixmaps[type].size();
	return &surfPixmaps[type][var % vars];
}

void Tileset::addSurfTex(int leftUp, int rightUp, int leftDown, int rightDown, Vec2f &coord, const Texture2D *&texture) {
	//center textures
	if(leftUp == rightUp && leftUp == leftDown && leftUp == rightDown) {
		//texture variation according to probability
		float r= random.randRange(0.f, 1.f);
		int var= 0;
		float max= 0.f;
		for(int i=0; i < surfProbs[leftUp].size(); ++i) {
			max+= surfProbs[leftUp][i];
			if(r <= max) {
				var= i;
				break;
			}
		}
		SurfaceInfo si(getSurfPixmap(leftUp, var));
		surfaceAtlas.addSurface(&si);
		coord= si.getCoord();
		texture= si.getTexture();
	}
	//spatted textures
	else {
		int var= random.randRange(0, transitionVars);

		SurfaceInfo si( getSurfPixmap(leftUp, var),
						getSurfPixmap(rightUp, var),
						getSurfPixmap(leftDown, var),
						getSurfPixmap(rightDown, var));
		surfaceAtlas.addSurface(&si);
		coord= si.getCoord();
		texture= si.getTexture();
	}
}

}}// end namespace
