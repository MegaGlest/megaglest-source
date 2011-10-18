// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// GlErrors.h  -- Copyright (c) 2006-2007 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
//
// OpenGL error management.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __GLERRORS_H__
#define __GLERRORS_H__

namespace Shared { namespace Graphics { namespace md5 {

GLenum checkOpenGLErrors (const char *file, int line);

}}} //end namespace

#endif // __GLERRORS_H__
