// ==============================================================
//	This file is part of MegaGlest (www.glest.org)
//
//	Copyright (C) 2011-  by Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 3 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef __MD5Loader_H__
#define __MD5Loader_H__

//#include "Mathlib.h"

namespace Shared { namespace Graphics { namespace md5 {

class Md5Object;

class Real;
template <typename Real>
class Matrix4x4;
typedef Matrix4x4<float> Matrix4x4f;

void initMD5OpenGL(string shaderPath);
void cleanupMD5OpenGL();

Md5Object * getMD5ObjectFromLoaderScript(const string &filename);
void renderMD5Object(Md5Object *object, double anim, Matrix4x4f *modelViewMatrix=NULL);

}}} //end namespace

#endif // __MD5Loader_H__
