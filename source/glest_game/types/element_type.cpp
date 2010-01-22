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

// =====================================================
// 	class RequirableType
// =====================================================

string RequirableType::getReqDesc() const{
	bool anyReqs= false;

	string reqString;
	for(int i=0; i<getUnitReqCount(); ++i){
        reqString+= getUnitReq(i)->getName();
        reqString+= "\n";
		anyReqs= true;
    }

    for(int i=0; i<getUpgradeReqCount(); ++i){
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

// =====================================================
// 	class ProducibleType
// =====================================================

ProducibleType::ProducibleType(){
	cancelImage= NULL;
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

}}//end namespace
