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

#include "damage_multiplier.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class DamageMultiplierTable
// =====================================================

DamageMultiplierTable::DamageMultiplierTable(){
	values= NULL;
}

DamageMultiplierTable::~DamageMultiplierTable(){
	delete [] values;
}

void DamageMultiplierTable::init(int attackTypeCount, int armorTypeCount){
	this->attackTypeCount= attackTypeCount;
	this->armorTypeCount= armorTypeCount;
	
	int valueCount= attackTypeCount*armorTypeCount;
	values= new float[valueCount];
	for(int i=0; i<valueCount; ++i){
		values[i]= 1.f;
	}
}

float DamageMultiplierTable::getDamageMultiplier(const AttackType *att, const ArmorType *art) const{
	return values[attackTypeCount*art->getId()+att->getId()];
}

void DamageMultiplierTable::setDamageMultiplier(const AttackType *att, const ArmorType *art, float value){
	values[attackTypeCount*art->getId()+att->getId()]= value;
}

}}//end namespaces
