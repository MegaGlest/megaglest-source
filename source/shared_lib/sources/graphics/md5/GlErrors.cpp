// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// GlErrors.cpp  -- Copyright (c) 2006-2007 David Henry
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

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>

#include "GlErrors.h"

namespace Shared { namespace Graphics { namespace md5 {

using std::cerr;
using std::endl;

// -------------------------------------------------------------------------
// checkOpenGLErrors
//
// Print the last OpenGL error code.  @file is the filename where the
// function has been called, @line is the line number.  You should use
// this function like this:
//  checkOpenGLErrors (__FILE__, __LINE__);
// -------------------------------------------------------------------------

GLenum checkOpenGLErrors (const char *file, int line) {
  GLenum errCode = glGetError ();

  if (errCode != GL_NO_ERROR)
    cerr << "(GL) " << file << " (" << line << "): "
	 << gluErrorString (errCode) << endl;

  return errCode;
}

}}} //end namespace
