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

#include "tileset.h"

#include <cassert>
#include <ctime>

#include "logger.h"
#include "util.h"
#include "renderer.h"
#include "game_util.h"
#include "leak_dumper.h"
#include "properties.h"
#include "lang.h"
#include "platform_util.h"

using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class AmbientSounds
// =====================================================

void AmbientSounds::load(const string &dir, const XmlNode *xmlNode,
		std::map<string,vector<pair<string, string> > > &loadedFileList, string parentLoader) {
	string path="";

	//day
	const XmlNode *dayNode= xmlNode->getChild("day-sound");
	enabledDay= dayNode->getAttribute("enabled")->getBoolValue();
	if(enabledDay) {
		string currentPath = dir;
		endPathWithSlash(currentPath);

		path= dayNode->getAttribute("path")->getRestrictedValue(currentPath);
		day.open(path);
		loadedFileList[path].push_back(make_pair(parentLoader,dayNode->getAttribute("path")->getRestrictedValue()));

		alwaysPlayDay= dayNode->getAttribute("play-always")->getBoolValue();
	}

	//night
	const XmlNode *nightNode= xmlNode->getChild("night-sound");
	enabledNight= nightNode->getAttribute("enabled")->getBoolValue();
	if(enabledNight) {
		string currentPath = dir;
		endPathWithSlash(currentPath);

		path= nightNode->getAttribute("path")->getRestrictedValue(currentPath);
		night.open(path);
		loadedFileList[path].push_back(make_pair(parentLoader,nightNode->getAttribute("path")->getRestrictedValue()));

		alwaysPlayNight= nightNode->getAttribute("play-always")->getBoolValue();
	}

	//rain
	const XmlNode *rainNode= xmlNode->getChild("rain-sound");
	enabledRain= rainNode->getAttribute("enabled")->getBoolValue();
	if(enabledRain) {
		string currentPath = dir;
		endPathWithSlash(currentPath);

		path= rainNode->getAttribute("path")->getRestrictedValue(currentPath);
		rain.open(path);
		loadedFileList[path].push_back(make_pair(parentLoader,rainNode->getAttribute("path")->getRestrictedValue()));
	}

	//snow
	const XmlNode *snowNode= xmlNode->getChild("snow-sound");
	enabledSnow= snowNode->getAttribute("enabled")->getBoolValue();
	if(enabledSnow) {
		string currentPath = dir;
		endPathWithSlash(currentPath);

		path= snowNode->getAttribute("path")->getRestrictedValue(currentPath);
		snow.open(path);
		loadedFileList[path].push_back(make_pair(parentLoader,snowNode->getAttribute("path")->getRestrictedValue()));
	}

	//dayStart
	const XmlNode *dayStartNode= xmlNode->getChild("day-start-sound");
	enabledDayStart= dayStartNode->getAttribute("enabled")->getBoolValue();
	if(enabledDayStart) {
		string currentPath = dir;
		endPathWithSlash(currentPath);

		path= dayStartNode->getAttribute("path")->getRestrictedValue(currentPath);
		dayStart.load(path);
		loadedFileList[path].push_back(make_pair(parentLoader,dayStartNode->getAttribute("path")->getRestrictedValue()));
	}

	//nightStart
	const XmlNode *nightStartNode= xmlNode->getChild("night-start-sound");
	enabledNightStart= nightStartNode->getAttribute("enabled")->getBoolValue();
	if(enabledNightStart) {
		string currentPath = dir;
		endPathWithSlash(currentPath);

		path= nightStartNode->getAttribute("path")->getRestrictedValue(currentPath);
		nightStart.load(path);
		loadedFileList[path].push_back(make_pair(parentLoader,nightStartNode->getAttribute("path")->getRestrictedValue()));
	}
}

// =====================================================
// 	class Tileset
// =====================================================

Checksum Tileset::loadTileset(const vector<string> pathList, const string &tilesetName,
		Checksum* checksum, std::map<string,vector<pair<string, string> > > &loadedFileList) {
    Checksum tilesetChecksum;

    bool found = false;
    for(int idx = 0; idx < pathList.size(); idx++) {
		string currentPath = pathList[idx];
		endPathWithSlash(currentPath);
        string path = currentPath + tilesetName;
        if(isdir(path.c_str()) == true) {
            load(path, checksum, &tilesetChecksum, loadedFileList);
            found = true;
            break;
        }
    }
    if(found == false) {
		throw megaglest_runtime_error("Error could not find tileset [" + tilesetName + "]\n");

    }
    return tilesetChecksum;
}


void Tileset::load(const string &dir, Checksum *checksum, Checksum *tilesetChecksum,
		std::map<string,vector<pair<string, string> > > &loadedFileList) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	random.init(time(NULL));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	string name= lastDir(dir);
	tileset_name = name;
	string currentPath = dir;
	endPathWithSlash(currentPath);
	string path= currentPath + name + ".xml";
	string sourceXMLFile = path;

	checksum->addFile(path);
	tilesetChecksum->addFile(path);
	checksumValue.addFile(path);

	try {
		char szBuf[1024]="";
		sprintf(szBuf,Lang::getInstance().get("LogScreenGameLoadingTileset","",true).c_str(),formatString(name).c_str());
		Logger::getInstance().add(szBuf, true);

		Renderer &renderer= Renderer::getInstance();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//printf("About to load tileset [%s]\n",path.c_str());
		//parse xml
		XmlTree xmlTree;
		xmlTree.load(path,Properties::getTagReplacementValues());
		loadedFileList[path].push_back(make_pair(currentPath,currentPath));

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		const XmlNode *tilesetNode= xmlTree.getRootNode();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//surfaces
		const XmlNode *surfacesNode= tilesetNode->getChild("surfaces");
		for(int i=0; i < surfCount; ++i) {
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

			for(int j = 0; j < childCount; ++j) {
				surfPixmaps[i][j] = NULL;
			}

			for(int j = 0; j < childCount; ++j) {
				const XmlNode *textureNode= surfaceNode->getChild("texture", j);
				if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
					surfPixmaps[i][j] = new Pixmap2D();
					surfPixmaps[i][j]->init(3);
					surfPixmaps[i][j]->load(textureNode->getAttribute("path")->getRestrictedValue(currentPath));
				}
				loadedFileList[textureNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(make_pair(sourceXMLFile,textureNode->getAttribute("path")->getRestrictedValue()));

				surfProbs[i][j]= textureNode->getAttribute("prob")->getFloatValue();
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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
				TilesetModelType* tmt=objectTypes[i].loadModel(pathAttribute->getRestrictedValue(currentPath),&loadedFileList, sourceXMLFile);
				loadedFileList[pathAttribute->getRestrictedValue(currentPath)].push_back(make_pair(sourceXMLFile,pathAttribute->getRestrictedValue()));

				if(modelNode->hasAttribute("anim-speed") == true) {
					int animSpeed= modelNode->getAttribute("anim-speed")->getIntValue();
					tmt->setAnimSpeed(animSpeed);
				}

				if(modelNode->hasChild("particles")){
					const XmlNode *particleNode= modelNode->getChild("particles");
					bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
					if(particleEnabled){
						for(int k=0; k<particleNode->getChildCount(); ++k){
							const XmlNode *particleFileNode= particleNode->getChild("particle-file", k);
							string path= particleFileNode->getAttribute("path")->getRestrictedValue();
							ObjectParticleSystemType *objectParticleSystemType= new ObjectParticleSystemType();
							objectParticleSystemType->load(particleFileNode, dir, currentPath + path,
									&Renderer::getInstance(), loadedFileList, sourceXMLFile,"");
							loadedFileList[currentPath + path].push_back(make_pair(sourceXMLFile,particleFileNode->getAttribute("path")->getRestrictedValue()));

							tmt->addParticleSystem(objectParticleSystemType);
						}
					}
				}

				//rotationAllowed
				if(modelNode->hasAttribute("rotationAllowed") == true) {
					tmt->setRotationAllowed(modelNode->getAttribute("rotationAllowed")->getBoolValue());
				}
				else if(modelNode->hasChild("rotationAllowed")){
					const XmlNode *rotationAllowedNode= modelNode->getChild("rotationAllowed");
					tmt->setRotationAllowed(rotationAllowedNode->getAttribute("value")->getBoolValue());
				}
				else{
					tmt->setRotationAllowed(true);
				}
			}
		}

		// Now free up the pixmap memory
		for(int i=0; i<objCount; ++i){
			objectTypes[i].deletePixels();
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//ambient sounds
		ambientSounds.load(dir, tilesetNode->getChild("ambient-sounds"), loadedFileList, sourceXMLFile);

		//parameters
		const XmlNode *parametersNode= tilesetNode->getChild("parameters");

		//water
		const XmlNode *waterNode= parametersNode->getChild("water");
		waterTex= renderer.newTexture3D(rsGame);
		if(waterTex) {
			waterTex->setMipmap(false);
			waterTex->setWrapMode(Texture::wmRepeat);
		}
		waterEffects= waterNode->getAttribute("effects")->getBoolValue();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		int waterFrameCount= waterNode->getChildCount();
		if(waterTex) {
			waterTex->getPixmap()->init(waterFrameCount, 4);
		}
		for(int i=0; i<waterFrameCount; ++i){
			const XmlNode *waterFrameNode= waterNode->getChild("texture", i);
			if(waterTex) {
				waterTex->getPixmap()->loadSlice(waterFrameNode->getAttribute("path")->getRestrictedValue(currentPath), i);
			}
			loadedFileList[waterFrameNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(make_pair(sourceXMLFile,waterFrameNode->getAttribute("path")->getRestrictedValue()));
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//sun and moon light colors
		const XmlNode *sunLightColorNode= parametersNode->getChild("sun-light");
		sunLightColor.x= sunLightColorNode->getAttribute("red")->getFloatValue();
		sunLightColor.y= sunLightColorNode->getAttribute("green")->getFloatValue();
		sunLightColor.z= sunLightColorNode->getAttribute("blue")->getFloatValue();

		const XmlNode *moonLightColorNode= parametersNode->getChild("moon-light");
		moonLightColor.x= moonLightColorNode->getAttribute("red")->getFloatValue();
		moonLightColor.y= moonLightColorNode->getAttribute("green")->getFloatValue();
		moonLightColor.z= moonLightColorNode->getAttribute("blue")->getFloatValue();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//weather
		const XmlNode *weatherNode= parametersNode->getChild("weather");
		float sunnyProb= weatherNode->getAttribute("sun")->getFloatValue(0.f, 1.f);
		float rainyProb= weatherNode->getAttribute("rain")->getFloatValue(0.f, 1.f) + sunnyProb;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

#ifdef USE_STREFLOP
		float rnd= streflop::fabs(static_cast<streflop::Simple>(random.randRange(-1.f, 1.f)));
#else
		float rnd= fabs(random.randRange(-1.f, 1.f));
#endif

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(rnd < sunnyProb) {
			weather= wSunny;
		}
		else if(rnd < rainyProb) {
			weather= wRainy;
		}
		else {
			weather= wSnowy;
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	}
	//Exception handling (conversions and so on);
	catch(const exception &e) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error: " + path + "\n" + e.what());
	}

    Lang &lang = Lang::getInstance();
    lang.loadTilesetStrings(name);
}

Tileset::~Tileset() {
	for(int i = 0; i < surfCount; ++i) {
		deleteValues(surfPixmaps[i].begin(),surfPixmaps[i].end());
		surfPixmaps[i].clear();
	}

	Logger::getInstance().add(Lang::getInstance().get("LogScreenGameUnLoadingTileset","",true), true);
}

const Pixmap2D *Tileset::getSurfPixmap(int type, int var) const{
	int vars= surfPixmaps[type].size();
	return surfPixmaps[type][var % vars];
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
