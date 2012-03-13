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

#include "element_type.h"

#include <cassert>

#include "resource_type.h" 
#include "upgrade_type.h"
#include "unit_type.h"
#include "resource.h"
#include "tech_tree.h"
#include "logger.h"
#include "lang.h"
#include "renderer.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class DisplayableType
// =====================================================

DisplayableType::DisplayableType(){
	image= NULL;
}

//void DisplayableType::saveGame(XmlNode *rootNode) const {
//	std::map<string,string> mapTagReplacements;
//	XmlNode *displayableTypeNode = rootNode->addChild("DisplayableType");
//
//	displayableTypeNode->addAttribute("name",name, mapTagReplacements);
//}

// =====================================================
// 	class RequirableType
// =====================================================

string RequirableType::getReqDesc() const{
	bool anyReqs= false;

	string reqString="";
	for(int i=0; i<getUnitReqCount(); ++i){
		if(getUnitReq(i) == NULL) {
			throw runtime_error("getUnitReq(i) == NULL");
		}
        reqString+= getUnitReq(i)->getName();
        reqString+= "\n";
		anyReqs= true;
    }

    for(int i=0; i<getUpgradeReqCount(); ++i){
		if(getUpgradeReq(i) == NULL) {
			throw runtime_error("getUpgradeReq(i) == NULL");
		}

    	reqString+= getUpgradeReq(i)->getName();
        reqString+= "\n";
		anyReqs= true;
    }

	string str= getName();
	if(anyReqs){
		return str + " " + Lang::getInstance().get("Reqs") + ":\n" + reqString;
	}
	else{
		return str;
	}
}  

//void RequirableType::saveGame(XmlNode *rootNode) const {
//	DisplayableType::saveGame(rootNode);
//
//	std::map<string,string> mapTagReplacements;
//	XmlNode *requirableTypeNode = rootNode->addChild("RequirableType");
//
////	UnitReqs unitReqs;			//needed units
//	for(unsigned int i = 0; i < unitReqs.size(); ++i) {
//		const UnitType *ut = unitReqs[i];
//
//		XmlNode *unitReqsNode = requirableTypeNode->addChild("unitReqs");
//		unitReqsNode->addAttribute("name",ut->getName(), mapTagReplacements);
//	}
////	UpgradeReqs upgradeReqs;	//needed upgrades
//	for(unsigned int i = 0; i < upgradeReqs.size(); ++i) {
//		const UpgradeType* ut = upgradeReqs[i];
//
//		ut->saveGame(requirableTypeNode);
//	}
//
//}

// =====================================================
// 	class ProducibleType
// =====================================================

ProducibleType::ProducibleType(){
	cancelImage= NULL;
	productionTime=0;
}

ProducibleType::~ProducibleType(){
}

const Resource *ProducibleType::getCost(const ResourceType *rt) const{
	for(int i=0; i<costs.size(); ++i){
		if(costs[i].getType()==rt){
			return &costs[i];
		}
	}
	return NULL;
}

string ProducibleType::getReqDesc() const{
    string str= getName()+" "+Lang::getInstance().get("Reqs")+":\n";
    for(int i=0; i<getCostCount(); ++i){
        if(getCost(i)->getAmount()!=0){
            str+= getCost(i)->getType()->getName();
            str+= ": "+ intToStr(getCost(i)->getAmount());
            str+= "\n";
        }
    }

    for(int i=0; i<getUnitReqCount(); ++i){
        str+= getUnitReq(i)->getName();
        str+= "\n";
    }

    for(int i=0; i<getUpgradeReqCount(); ++i){
        str+= getUpgradeReq(i)->getName();
        str+= "\n";
    }

    return str;
}   

//void ProducibleType::saveGame(XmlNode *rootNode) const {
//	RequirableType::saveGame(rootNode);
//
//	std::map<string,string> mapTagReplacements;
//	XmlNode *producibleTypeNode = rootNode->addChild("ProducibleType");
//
////	Costs costs;
//	for(unsigned int i = 0; i < costs.size(); ++i) {
//		const Resource &res = costs[i];
//		res.saveGame(producibleTypeNode);
//	}
////    Texture2D *cancelImage;
////	int productionTime;
//	producibleTypeNode->addAttribute("productionTime",intToStr(productionTime), mapTagReplacements);
//}

//void ProducibleType::loadGame(const XmlNode *rootNode) {
//	const XmlNode *producibleTypeNode = rootNode->getChild("ProducibleType");
//
//	//int newUnitId = producibleTypeNode->getAttribute("id")->getIntValue();
//}

}}//end namespace
