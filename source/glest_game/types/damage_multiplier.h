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

#ifndef _GLEST_GAME_DAMAGEMULTIPLIER_H_
#define _GLEST_GAME_DAMAGEMULTIPLIER_H_

#include <string>

using std::string;

namespace Glest{ namespace Game{

// ===============================
// 	class AttackType  
// ===============================

class AttackType{
private:
	string name;
	int id;

public:
	int getId() const					{return id;}
	const string &getName() const		{return name;}

	void setName(const string &name)	{this->name= name;}
	void setId(int id)					{this->id= id;}
};

// ===============================
// 	class ArmorType  
// ===============================

class ArmorType{
private:
	string name;
	int id;

public:
	int getId() const					{return id;}
	const string &getName() const		{return name;}

	void setName(const string &name)	{this->name= name;}
	void setId(int id)					{this->id= id;}
};

// =====================================================
// 	class DamageMultiplierTable  
//
///	Some attack types have bonuses against some 
/// armor types and vice-versa
// =====================================================

class DamageMultiplierTable{
private:
	float *values;
	int attackTypeCount;
	int armorTypeCount;

public:
	DamageMultiplierTable();
	~DamageMultiplierTable();

	void init(int attackTypeCount, int armorTypeCount);
	float getDamageMultiplier(const AttackType *att, const ArmorType *art) const; 
	void setDamageMultiplier(const AttackType *att, const ArmorType *art, float value);
};

}}//end namespace

#endif
