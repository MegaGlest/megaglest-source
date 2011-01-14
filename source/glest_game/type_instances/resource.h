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
#ifndef _GLEST_GAME_RESOURCE_H_
#define _GLEST_GAME_RESOURCE_H_

#include <string>
#include "vec.h"
#include <map>
#include "leak_dumper.h"

using std::string;
using std::map;

namespace Glest{ namespace Game{

using Shared::Graphics::Vec2i;

class ResourceType;

// =====================================================
// 	class Resource  
//
/// Amount of a given ResourceType
// =====================================================

class ValueCheckerVault {

protected:
	map<const void *,string> vaultList;

	void addItemToVault(const void *ptr,int value);
	void checkItemInVault(const void *ptr,int value) const;

public:

	ValueCheckerVault() {
		vaultList.clear();
	}
};

class Resource : public ValueCheckerVault {
private:
    int amount;
    const ResourceType *type;
	Vec2i pos;	
	int balance;

public:
	Resource();
    void init(const ResourceType *rt, int amount);
    void init(const ResourceType *rt, const Vec2i &pos);

	const ResourceType * getType() const	{return type;}
	Vec2i getPos() const					{return pos;}

	int getAmount() const;
	int getBalance() const;
	string getDescription() const;

	void setAmount(int amount);
	void setBalance(int balance);

    bool decAmount(int i);
};

}}// end namespace

#endif
