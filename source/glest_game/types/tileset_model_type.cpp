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

#include "tileset_model_type.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class TilesetModelType
// =====================================================

TilesetModelType::~TilesetModelType(){
		while(!(particleTypes.empty())){
			delete particleTypes.back();
			particleTypes.pop_back();
		}
	//Logger::getInstance().add("ObjectType", true);
}


void TilesetModelType::addParticleSystem(ObjectParticleSystemType *particleSystem){
	particleTypes.push_back(particleSystem);
}


}}//end namespace
