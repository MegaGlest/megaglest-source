// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "shader_manager.h"

#include <stdexcept>

#include "graphics_interface.h"
#include "graphics_factory.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class ShaderManager
// =====================================================

ShaderManager::~ShaderManager(){
}

void ShaderManager::init(){
	for(int i=0; i<shaders.size(); ++i){
		shaders[i]->init();
		if(!shaders[i]->compile(logString)){
			throw runtime_error("Can't compile shader\n");
		}
	}	
	for(int i=0; i<shaderPrograms.size(); ++i){
		shaderPrograms[i]->init();
		if(!shaderPrograms[i]->link(logString)){
			throw runtime_error("Can't link shader\n");
		}
	}	
}

void ShaderManager::end(){
	for(int i=0; i<shaderPrograms.size(); ++i){
		shaderPrograms[i]->end();
		delete shaderPrograms[i];
	}	
	shaderPrograms.clear();
	for(int i=0; i<shaders.size(); ++i){
		shaders[i]->end();
		delete shaders[i];
	}	
	shaders.clear();
}

ShaderProgram *ShaderManager::newShaderProgram(){
	ShaderProgram *sp= GraphicsInterface::getInstance().getFactory()->newShaderProgram();
	shaderPrograms.push_back(sp);
	return sp;
}

VertexShader *ShaderManager::newVertexShader(){
	VertexShader *vs= GraphicsInterface::getInstance().getFactory()->newVertexShader();
	shaders.push_back(vs);
	return vs;
}

FragmentShader *ShaderManager::newFragmentShader(){
	FragmentShader *fs= GraphicsInterface::getInstance().getFactory()->newFragmentShader();
	shaders.push_back(fs);
	return fs;
}

}}//end namespace
