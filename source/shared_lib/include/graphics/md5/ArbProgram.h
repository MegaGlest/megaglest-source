// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// ArbProgram.h  -- Copyright (c) 2007 David Henry
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
// Definitions of ARB program related classes.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __ARB_PROGRAM_H__
#define __ARB_PROGRAM_H__

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/glew.h>
#include <stdexcept>
#include <string>

namespace Shared { namespace Graphics { namespace md5 {

using std::string;

/////////////////////////////////////////////////////////////////////////////
// ARB Program class diagram:
//
//                    +---------- (abs)
//                    |  ArbProgram  |
//                    +--------------+
//                           ^
//                           |
//             +-------------+-------------+
//             |                           |
//   +------------------+        +--------------------+
//   | ArbVertexProgram |        | ArbFragmentProgram |
//   +------------------+        +--------------------+
//
/////////////////////////////////////////////////////////////////////////////

// Global functions for initializing ARB program extensions and query for
// vertex and fragment program support on the host.
GLboolean hasArbVertexProgramSupport();
GLboolean hasArbFragmentProgramSupport();

void initArbProgramHandling();

/////////////////////////////////////////////////////////////////////////////
//
// class ArbProgram -- ARB Program abstract object.  Can be a vertex program
// or a fragment program.
//
/////////////////////////////////////////////////////////////////////////////
class ArbProgram
{
protected:
  // Constructor
  ArbProgram (const string &filename);

public:
  // Destructor
  virtual ~ArbProgram ();

public:
  // Accessors
  const string &name () const { return _name; }
  const string &code () const { return _code; }
  GLuint handle () const { return _handle; }
  bool fail () const { return _fail; }

  virtual GLenum programType () const = 0;

public:
  // Public interface
  void use () const;
  void unuse () const;

protected:
  // Internal functions
  void printProgramString (int errPos);
  void load ()
    throw (std::runtime_error);
  void loadProgramFile (const string &filename)
    throw (std::runtime_error);

protected:
  // Member variables
  string _name;
  string _code;
  GLuint _handle;
  GLboolean _fail;
};

/////////////////////////////////////////////////////////////////////////////
//
// class ArbVertexProgram -- ARB vertex program object.
//
/////////////////////////////////////////////////////////////////////////////
class ArbVertexProgram : public ArbProgram
{
public:
  // Constructor
  ArbVertexProgram (const string &filename);

public:
  // Return the program enum type
  virtual GLenum programType () const {
    return GL_VERTEX_PROGRAM_ARB;
  }
};


/////////////////////////////////////////////////////////////////////////////
//
// class ArbFragmentProgram -- ARB fragment program object.
//
/////////////////////////////////////////////////////////////////////////////
class ArbFragmentProgram : public ArbProgram
{
public:
  // Constructor
  ArbFragmentProgram (const string &filename);

public:
  // Return the program enum type
  virtual GLenum programType () const {
    return GL_FRAGMENT_PROGRAM_ARB;
  }
};

}}} //end namespace

#endif // __ARB_PROGRAM_H__
