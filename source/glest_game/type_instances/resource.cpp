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

#include "resource.h"

#include "conversion.h"
#include "resource_type.h"
#include "checksum.h"
#include <stdexcept>
#include "util.h"
#include "tech_tree.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Resource
// =====================================================

Resource::Resource() {
    this->type= NULL;
    this->amount= 0;
	pos= Vec2i(0);
	balance= 0;

	addItemToVault(&this->amount,this->amount);
	addItemToVault(&this->balance,this->balance);
}

void Resource::init(const ResourceType *rt, int amount) {
    this->type= rt;
    this->amount= amount;        
	pos= Vec2i(0);
	balance= 0;

	addItemToVault(&this->amount,this->amount);
	addItemToVault(&this->balance,this->balance);
}

void Resource::init(const ResourceType *rt, const Vec2i &pos) {
	this->type=rt;
	amount=rt->getDefResPerPatch();
	this->pos= pos;

	addItemToVault(&this->amount,this->amount);
	addItemToVault(&this->balance,this->balance);
}

string Resource::getDescription() const {
     string str;

     str+= type->getName(true);
     str+="\n";
     str+= intToStr(amount);
     str+="/";
     str+= intToStr(type->getDefResPerPatch());

     return str;
}

int Resource::getAmount() const {
	checkItemInVault(&this->amount,this->amount);
	return amount;
}

int Resource::getBalance() const {
	checkItemInVault(&this->balance,this->balance);
	return balance;
}

void Resource::setAmount(int amount) {
	checkItemInVault(&this->amount,this->amount);
	this->amount= amount;
	addItemToVault(&this->amount,this->amount);
}

void Resource::setBalance(int balance) {
	checkItemInVault(&this->balance,this->balance);
	this->balance= balance;
	addItemToVault(&this->balance,this->balance);
}

bool Resource::decAmount(int i) {
	checkItemInVault(&this->amount,this->amount);
	amount -= i;
	addItemToVault(&this->amount,this->amount);

    if(amount > 0) {
         return false;
    }
    return true;
}

void Resource::saveGame(XmlNode *rootNode) const {
	std::map<string,string> mapTagReplacements;
	XmlNode *resourceNode = rootNode->addChild("Resource");

//    int amount;
	resourceNode->addAttribute("amount",intToStr(amount), mapTagReplacements);
//    const ResourceType *type;
	resourceNode->addAttribute("type",type->getName(), mapTagReplacements);
//	Vec2i pos;
	resourceNode->addAttribute("pos",pos.getString(), mapTagReplacements);
//	int balance;
	resourceNode->addAttribute("balance",intToStr(balance), mapTagReplacements);
}

void Resource::loadGame(const XmlNode *rootNode, int index,const TechTree *techTree) {
	vector<XmlNode *> resourceNodeList = rootNode->getChildList("Resource");

	if(index < resourceNodeList.size()) {
		XmlNode *resourceNode = resourceNodeList[index];

		amount = resourceNode->getAttribute("amount")->getIntValue();
		type = techTree->getResourceType(resourceNode->getAttribute("type")->getValue());
		pos = Vec2i::strToVec2(resourceNode->getAttribute("pos")->getValue());
		balance = resourceNode->getAttribute("balance")->getIntValue();
	}
}

}}//end namespace
