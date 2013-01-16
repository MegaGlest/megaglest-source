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
#include "conversion.h"
#include "lang.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

void AttackType::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *attackTypeNode = rootNode->addChild("AttackType");

//	string name;
	attackTypeNode->addAttribute("name",name, mapTagReplacements);
//	int id;
	attackTypeNode->addAttribute("id",intToStr(id), mapTagReplacements);
}

string AttackType::getName(bool translatedValue) const	{
	if(translatedValue == false) return name;

	Lang &lang = Lang::getInstance();
	return lang.getTechTreeString("AttackTypeName_" + name,name.c_str());
}

void ArmorType::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *armorTypeNode = rootNode->addChild("ArmorType");

//	string name;
	armorTypeNode->addAttribute("name",name, mapTagReplacements);
//	int id;
	armorTypeNode->addAttribute("id",intToStr(id), mapTagReplacements);
}

string ArmorType::getName(bool translatedValue) const	{
	if(translatedValue == false) return name;

	Lang &lang = Lang::getInstance();
	return lang.getTechTreeString("ArmorTypeName_" + name,name.c_str());
}
// =====================================================
// 	class DamageMultiplierTable
// =====================================================

DamageMultiplierTable::DamageMultiplierTable(){
	values= NULL;
	attackTypeCount=0;
	armorTypeCount=0;
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

void DamageMultiplierTable::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *damageMultiplierTableNode = rootNode->addChild("DamageMultiplierTable");

//	float *values;
//	int attackTypeCount;
//	int armorTypeCount;
	damageMultiplierTableNode->addAttribute("attackTypeCount",intToStr(attackTypeCount), mapTagReplacements);
	damageMultiplierTableNode->addAttribute("armorTypeCount",intToStr(armorTypeCount), mapTagReplacements);

	int valueCount= attackTypeCount * armorTypeCount;
	for(unsigned int i=0; i < valueCount; ++i) {
		XmlNode *valuesNode = damageMultiplierTableNode->addChild("values");
		valuesNode->addAttribute("value",intToStr(values[i]), mapTagReplacements);
	}
}

}}//end namespaces
