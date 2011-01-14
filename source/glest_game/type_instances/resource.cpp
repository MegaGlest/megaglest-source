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

#include "resource.h"

#include "conversion.h"
#include "resource_type.h"
#include "checksum.h"
#include <stdexcept>
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

void ValueCheckerVault::addItemToVault(const void *ptr,int value) {
	Checksum checksum;
	checksum.addString(intToStr(value));
	vaultList[ptr] = intToStr(checksum.getSum());

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] add vault key [%p] value [%s] [%d]\n",__FILE__,__FUNCTION__,__LINE__,ptr,intToStr(checksum.getSum()).c_str(),value);
}

void ValueCheckerVault::checkItemInVault(const void *ptr,int value) const {
	map<const void *,string>::const_iterator iterFind = vaultList.find(ptr);
	if(iterFind == vaultList.end()) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) {
			printf("In [%s::%s Line: %d] check vault key [%p] value [%d]\n",__FILE__,__FUNCTION__,__LINE__,ptr,value);
			for(map<const void *,string>::const_iterator iterFind = vaultList.begin();
					iterFind != vaultList.end(); iterFind++) {
				printf("In [%s::%s Line: %d] LIST-- check vault key [%p] value [%s]\n",__FILE__,__FUNCTION__,__LINE__,iterFind->first,iterFind->second.c_str());
			}
		}
		throw std::runtime_error("memory value has been unexpectedly modified (not found)!");
	}
	Checksum checksum;
	checksum.addString(intToStr(value));
	if(iterFind->second != intToStr(checksum.getSum())) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) {
			printf("In [%s::%s Line: %d] check vault key [%p] value [%s] [%d]\n",__FILE__,__FUNCTION__,__LINE__,ptr,intToStr(checksum.getSum()).c_str(),value);
			for(map<const void *,string>::const_iterator iterFind = vaultList.begin();
					iterFind != vaultList.end(); iterFind++) {
				printf("In [%s::%s Line: %d] LIST-- check vault key [%p] value [%s]\n",__FILE__,__FUNCTION__,__LINE__,iterFind->first,iterFind->second.c_str());
			}
		}
		throw std::runtime_error("memory value has been unexpectedly modified (changed)!");
	}
}

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

     str+= type->getName();
     str+="\n";
     str+= intToStr(amount);
     str+="/";
     str+= intToStr(type->getDefResPerPatch());

     return str;
}

int Resource::getAmount() const {
	checkItemInVault(&this->amount,this->amount);
	checkItemInVault(&this->balance,this->balance);

	return amount;
}

int Resource::getBalance() const {
	checkItemInVault(&this->amount,this->amount);
	checkItemInVault(&this->balance,this->balance);

	return balance;
}

void Resource::setAmount(int amount) {
	checkItemInVault(&this->amount,this->amount);
	checkItemInVault(&this->balance,this->balance);

	this->amount= amount;

	addItemToVault(&this->amount,this->amount);
}

void Resource::setBalance(int balance) {
	checkItemInVault(&this->amount,this->amount);
	checkItemInVault(&this->balance,this->balance);

	this->balance= balance;

	addItemToVault(&this->balance,this->balance);
}

bool Resource::decAmount(int i) {
	checkItemInVault(&this->amount,this->amount);
	checkItemInVault(&this->balance,this->balance);

	amount -= i;

	addItemToVault(&this->amount,this->amount);

    if(amount > 0) {
         return false;
    }
    return true;
}

}}//end namespace
