// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// GlErrors.cpp  -- Copyright (c) 2006-2007 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licensed under the MIT license:
// http://www.opensource.org/licenses/mit-license.php
//
// Open Source Initiative OSI - The MIT License (MIT):Licensing
//
// The MIT License (MIT)
// Copyright (c) <year> <copyright holders>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.//
//
// OpenGL error management.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/glew.h>

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
