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

#include "resource_type.h"

#include "util.h"
#include "element_type.h"
#include "logger.h"
#include "renderer.h"
#include "xml_parser.h"
#include "game_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class ResourceType
// =====================================================

ResourceType::ResourceType() {
    resourceClass=rcTech;
    tilesetObject=0;
    resourceNumber=0;
    interval=0;
	defResPerPatch=0;
	recoup_cost = false;
    model = NULL;
}

ResourceType::~ResourceType(){
	while(!(particleTypes.empty())){
		delete particleTypes.back();
		particleTypes.pop_back();
	}
}

void ResourceType::load(const string &dir, Checksum* checksum, Checksum *techtreeChecksum,
		std::map<string,vector<string> > &loadedFileList) {

	string path, str;
	Renderer &renderer= Renderer::getInstance();

	try
	{
	    recoup_cost = true;

		name= lastDir(dir);

		Logger::getInstance().add("Resource type: "+ formatString(name), true);
		string currentPath = dir;
		endPathWithSlash(currentPath);
		path= currentPath + name + ".xml";
		checksum->addFile(path);
		techtreeChecksum->addFile(path);

		//tree
		XmlTree xmlTree;
		xmlTree.load(path);
		loadedFileList[path].push_back(currentPath);

		const XmlNode *resourceNode= xmlTree.getRootNode();

		//image
		const XmlNode *imageNode= resourceNode->getChild("image");
		image= renderer.newTexture2D(rsGame);
		image->load(imageNode->getAttribute("path")->getRestrictedValue(currentPath));
		loadedFileList[imageNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(currentPath);

		//type
		const XmlNode *typeNode= resourceNode->getChild("type");
		resourceClass = strToRc(typeNode->getAttribute("value")->getRestrictedValue());

		switch(resourceClass)
		{
            case rcTech:
            {
                //model
                const XmlNode *modelNode= typeNode->getChild("model");
                string modelPath= modelNode->getAttribute("path")->getRestrictedValue(currentPath);

                model= renderer.newModel(rsGame);
                model->load(modelPath, false, &loadedFileList, &currentPath);
                loadedFileList[modelPath].push_back(currentPath);

                if(modelNode->hasChild("particles")){
					const XmlNode *particleNode= modelNode->getChild("particles");
					bool particleEnabled= particleNode->getAttribute("value")->getBoolValue();
					if(particleEnabled == true) {
						for(int k= 0; k < particleNode->getChildCount(); ++k) {
							const XmlNode *particleFileNode= particleNode->getChild("particle-file", k);
							string particlePath= particleFileNode->getAttribute("path")->getRestrictedValue();

							ObjectParticleSystemType *objectParticleSystemType= new ObjectParticleSystemType();
							objectParticleSystemType->load(dir,  currentPath + particlePath,
									&Renderer::getInstance(), loadedFileList);
							loadedFileList[currentPath + particlePath].push_back(currentPath);

							particleTypes.push_back(objectParticleSystemType);
						}
					}
				}

                //default resources
                const XmlNode *defaultAmountNode= typeNode->getChild("default-amount");
                defResPerPatch= defaultAmountNode->getAttribute("value")->getIntValue();

                //resource number
                const XmlNode *resourceNumberNode= typeNode->getChild("resource-number");
                resourceNumber= resourceNumberNode->getAttribute("value")->getIntValue();
			}
			break;

            case rcTileset:
            {
                //resource number
                const XmlNode *defaultAmountNode= typeNode->getChild("default-amount");
                defResPerPatch= defaultAmountNode->getAttribute("value")->getIntValue();

                //resource number
                const XmlNode *tilesetObjectNode= typeNode->getChild("tileset-object");
                tilesetObject= tilesetObjectNode->getAttribute("value")->getIntValue();
			}
			break;

            case rcConsumable:
            {
                //interval
                const XmlNode *intervalNode= typeNode->getChild("interval");
                interval= intervalNode->getAttribute("value")->getIntValue();
			}
			break;

            case rcStatic:
            {
                //recoup_cost
                if(typeNode->hasChild("recoup_cost") == true) {
                    const XmlNode *recoup_costNode= typeNode->getChild("recoup_cost");
                    if(recoup_costNode != NULL) {
                        recoup_cost= recoup_costNode->getAttribute("value")->getBoolValue();
                    }
                }
			}
			break;

		default:
			break;
		}
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error loading resource type: " + path + "\n" + e.what());
	}
}

// ==================== misc ====================

ResourceClass ResourceType::strToRc(const string &s){
	if(s=="tech"){
        return rcTech;
	}
	if(s=="tileset"){
        return rcTileset;
	}
	if(s=="static"){
        return rcStatic;
	}
	if(s=="consumable"){
        return rcConsumable;
	}
	throw runtime_error("Error converting from string ro resourceClass, found: " + s);
}

void ResourceType::deletePixels() {
	if(model != NULL) {
		model->deletePixels();
	}
}

}}//end namespace
