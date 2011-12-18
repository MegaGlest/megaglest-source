// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// ArbProgram.cpp  -- Copyright (c) 2007 David Henry
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
// Implementation of ARB program related classes.
//
/////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>

#include "ArbProgram.h"

namespace Shared { namespace Graphics { namespace md5 {

using std::cout;
using std::cerr;
using std::endl;

/////////////////////////////////////////////////////////////////////////////
//
// Global ARB program related functions.
//
/////////////////////////////////////////////////////////////////////////////

static GLboolean ArbVpCapable = GL_FALSE;
static GLboolean ArbFpCapable = GL_FALSE;

// --------------------------------------------------------------------------
// hasArbVertexProgramSupport
// hasArbFragmentProgramSupport
//
// Return true if the host has ARB program support (vertex or fragment).
// --------------------------------------------------------------------------

GLboolean hasArbVertexProgramSupport () {
  return ArbVpCapable;
}

GLboolean hasArbFragmentProgramSupport () {
  return ArbFpCapable;
}

// --------------------------------------------------------------------------
// initArbProgramHandling
//
// Initialize variables and extensions needed for using ARB Programs.
// This function should be called before any shader usage (at application
// initialization for example).
// --------------------------------------------------------------------------

void initArbProgramHandling () {
  // Check for extensions needed for ARB program support on host
  ArbVpCapable = glewIsSupported ("GL_ARB_vertex_program");
  ArbFpCapable = glewIsSupported ("GL_ARB_fragment_program");

  if (!hasArbVertexProgramSupport ())
    cerr << "* missing GL_ARB_vertex_program extension" << endl;

  if (!hasArbFragmentProgramSupport ())
    cerr << "* missing GL_ARB_fragment_program extension" << endl;
}

/////////////////////////////////////////////////////////////////////////////
//
// class ArbProgram implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ArbProgram::ArbProgram
//
// Constructor.
// --------------------------------------------------------------------------

ArbProgram::ArbProgram (const string &filename)
  : _name (filename), _handle (0), _fail (true) {
}

// --------------------------------------------------------------------------
// ArbProgram::~ArbProgram
//
// Destructor.  Destroy the program handle.
// --------------------------------------------------------------------------

ArbProgram::~ArbProgram () {
  if (glIsProgramARB (_handle))
    glDeleteProgramsARB (1, &_handle);
}

// -------------------------------------------------------------------------
// ArbProgram::use
// ArbProgram::unuse
//
// Bind/unbind the program.
// -------------------------------------------------------------------------

void ArbProgram::use () const {
  const GLenum target = programType ();

  glEnable (target);
  glBindProgramARB (target, _handle);
}

void ArbProgram::unuse () const {
  const GLenum target = programType ();

  glBindProgramARB (target, 0);
  glDisable (target);
}

// -------------------------------------------------------------------------
// ArbProgram::printProgramString
//
// Print the ARB program string until a given position.  This is
// usefull for printing code until error position.
// -------------------------------------------------------------------------

void ArbProgram::printProgramString (int errPos) {
  int i = 0;

  cerr << endl << " > ";
  for (i = 0; i < (errPos + 1) && _code[i]; i++) {
      cerr.put (_code[i]);
      if (_code[i] == '\n')
    	  cerr << " > ";
  }
  cerr << " <---" << endl << endl;
}

// -------------------------------------------------------------------------
// ArbProgram::load
//
// Create and load the program.
// -------------------------------------------------------------------------

void ArbProgram::load ()  throw (std::runtime_error) {
  const GLchar *code = _code.c_str ();
  const GLenum target = programType ();

  // Generate a program object handle
  glGenProgramsARB (1, &_handle);

  // Make the "current" program object progid
  glBindProgramARB (target, _handle);

  // Specify the program for the current object
  glProgramStringARB (target, GL_PROGRAM_FORMAT_ASCII_ARB,
		      _code.size (), code);

  // Check for errors and warnings...
  if (GL_INVALID_OPERATION == glGetError ()) {
      const GLubyte *errString;
      GLint errPos;

      // Find the error position
      glGetIntegerv (GL_PROGRAM_ERROR_POSITION_ARB, &errPos);

      // Print implementation-dependent program
      // errors and warnings string
      errString = glGetString (GL_PROGRAM_ERROR_STRING_ARB);

      cerr << "Error in " << ((GL_VERTEX_PROGRAM_ARB == target) ?
    		  "vertex" : "fragment");
      cerr << " program at position: " << errPos << endl << errString;

      printProgramString (errPos);

      _fail = true;
      throw std::runtime_error ("Compilation failed");
    }
}

// -------------------------------------------------------------------------
// ArbProgram::loadProgramFile
//
// Load program's code from file.  The code is stored into the
// _code string member variable.
// -------------------------------------------------------------------------

void ArbProgram::loadProgramFile (const string &filename)  throw (std::runtime_error) {
  // Open the file
  std::ifstream ifs (filename.c_str (), std::ios::in | std::ios::binary);

  if (ifs.fail ())
    throw std::runtime_error ("Couldn't open prog file: " + filename);

  // Read whole file into string
  _code.assign (std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

  // Close file
  ifs.close ();
}


/////////////////////////////////////////////////////////////////////////////
//
// class ArbVertexProgram implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ArbVertexProgram::ArbVertexProgram
//
// Constructor.  Read vertex program code from file and load it.
// --------------------------------------------------------------------------

ArbVertexProgram::ArbVertexProgram (const string &filename) : ArbProgram (filename) {
  try {
      // Load program code from file
      loadProgramFile (filename);

      // load the program from code buffer
      load ();

      cout << "* Vertex program \"" << _name << "\" loaded" << endl;
  }
  catch (std::runtime_error &err) {
      cerr << "Error: Faild to create vertex program from " << _name;
      cerr << endl << "Reason: " << err.what () << endl;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// class ArbFragmentProgram implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ArbFragmentProgram::ArbFragmentProgram
//
// Constructor.  Read fragment program code from file and load it.
// --------------------------------------------------------------------------

ArbFragmentProgram::ArbFragmentProgram (const string &filename) : ArbProgram (filename) {
  try {
      // Load program code from file
      loadProgramFile (filename);

      // load the program from code buffer
      load ();

      cout << "* Fragment program \"" << _name << "\" loaded" << endl;
  }
  catch (std::runtime_error &err) {
      cerr << "Error: Faild to create fragment program from " << _name;
      cerr << endl << "Reason: " << err.what () << endl;
  }
}

}}} //end namespace
