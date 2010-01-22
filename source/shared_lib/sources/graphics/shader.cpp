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

#include "shader.h"

#include <stdexcept>
#include <fstream>

#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{

// =====================================================
//	class ShaderSource
// =====================================================

void ShaderSource::load(const string &path){
	pathInfo+= path + " ";

	//open file
	ifstream ifs(path.c_str());
	if(ifs.fail()){
		throw runtime_error("Can't open shader file: " + path);
	}

	//read source
	while(true){
		fstream::int_type c= ifs.get();
		if(ifs.eof() || ifs.fail() || ifs.bad()){
			break;
		}
		code+= c;
	}
}

}}//end namespace
