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

#ifndef _SHARED_GRAPHICS_SHADERMANAGER_H_
#define _SHARED_GRAPHICS_SHADERMANAGER_H_

#include "shader.h"

#include <vector>

using namespace std;

namespace Shared{ namespace Graphics{

// =====================================================
//	class ShaderManager
// =====================================================

class ShaderManager{
protected:
	typedef vector<ShaderProgram*> ShaderProgramContainer;
	typedef vector<Shader*> ShaderContainer;

protected:
	ShaderProgramContainer shaderPrograms;
	ShaderContainer shaders;
	string logString;

public:
	ShaderManager(){}
	virtual ~ShaderManager();

	ShaderProgram *newShaderProgram();
	VertexShader *newVertexShader();
	FragmentShader *newFragmentShader();

	void init();
	void end();

	const string &getLogString() const	{return logString;}
};

}}//end namespace

#endif
