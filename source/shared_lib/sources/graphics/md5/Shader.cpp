// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Shader.cpp  -- Copyright (c) 2006-2007 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
//
// Implementation of GLSL shader related classes.
//
/////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>

#include "Shader.h"
#include "GlErrors.h"

namespace Shared { namespace Graphics { namespace md5 {

using std::cout;
using std::cerr;
using std::endl;


/////////////////////////////////////////////////////////////////////////////
//
// Global shader related functions.
//
/////////////////////////////////////////////////////////////////////////////

static GLboolean GLSLCapable = GL_FALSE;

// --------------------------------------------------------------------------
// hasShaderSupport
//
// Return true if the host has GLSL, so that we can use shaders.
// --------------------------------------------------------------------------
GLboolean hasShaderSupport () {
  return GLSLCapable;
}

// --------------------------------------------------------------------------
// checkExtensionPresence
//
// Check if an extension is present on the host OpenGL implementation.
// Increment @missing if the extension is missing.
// --------------------------------------------------------------------------
static void checkExtensionPresence (const string &name, int &missing) {
  if (!glewIsSupported (name.c_str ()))
    {
      cerr << "* missing " << name << " extension" << endl;
      missing++;
    }
}

// --------------------------------------------------------------------------
// initShaderHandling
//
// Initialize variables and extensions needed for using GLSL.  This
// function should be called before any shader usage (at application
// initialization for example).
// --------------------------------------------------------------------------
void initShaderHandling () {
  int missing = 0;

  // Check for extensions needed for GLSL support on host
  checkExtensionPresence ("GL_ARB_shader_objects", missing);
  checkExtensionPresence ("GL_ARB_shading_language_100", missing);
  checkExtensionPresence ("GL_ARB_vertex_shader", missing);
  checkExtensionPresence ("GL_ARB_fragment_shader", missing);

  // Disable GLSL if one extension is missing
  if (missing > 0)
    GLSLCapable = GL_FALSE;
  else
    GLSLCapable = GL_TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// class Shader implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Shader::Shader
//
// Constructor.
// --------------------------------------------------------------------------
Shader::Shader (const string &filename)
  : _name (filename), _handle (0), _compiled (0) {
}

// --------------------------------------------------------------------------
// Shader::~Shader
//
// Destructor.  Destroy the shader handle.
// --------------------------------------------------------------------------
Shader::~Shader () {
  if (GLEW_VERSION_2_0)
    {
      if (glIsShader (_handle))
	glDeleteShader (_handle);
    }
  else
    {
      GLint type;
      glGetObjectParameterivARB (_handle, GL_OBJECT_TYPE_ARB, &type);

      if (GL_SHADER_OBJECT_ARB == type)
	glDeleteObjectARB (_handle);
    }
}

// -------------------------------------------------------------------------
// Shader::printInfoLog
//
// Print log info about a vertex or a fragment shader.
// -------------------------------------------------------------------------
void Shader::printInfoLog () const {
  GLint infologLength = 0;

  // First check for previous OpenGL errors...
  checkOpenGLErrors (__FILE__, __LINE__);

  // Get log's length
  if (GLEW_VERSION_2_0)
    glGetShaderiv (_handle, GL_INFO_LOG_LENGTH, &infologLength);
  else
    glGetObjectParameterivARB (_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB,
			       &infologLength);

  // If log is empty, quit
  if (infologLength <= 1)
    return;

  try
    {
      GLchar *infoLog = new GLchar[infologLength];

      // Get the log...
      if (GLEW_VERSION_2_0)
	glGetShaderInfoLog (_handle, infologLength, NULL, infoLog);
      else
	glGetInfoLogARB (_handle, infologLength, NULL, infoLog);

      // ...and print it to standard output
      cout << "Shader \"" << _name << "\" InfoLog ("
	   << infologLength << "):" << endl << infoLog << endl;

      delete [] infoLog;
    }
  catch (std::bad_alloc &err)
    {
      cerr << "Error: memory allocation failed for shader info log"
	   << endl << " Reason: " << err.what () << endl;
    }
}

// -------------------------------------------------------------------------
// Shader::compile
//
// Create and compile the shader.
// -------------------------------------------------------------------------
void Shader::compile ()
  throw (std::runtime_error) {
  const GLchar *code = _code.c_str ();

  if (GLEW_VERSION_2_0)
    {
      // Create a shader object
      _handle = glCreateShader (shaderType ());

      // Upload shader code to OpenGL
      glShaderSource (_handle, 1, &code, NULL);

      // Compile shader
      glCompileShader (_handle);
      glGetShaderiv (_handle, GL_COMPILE_STATUS, &_compiled);
      printInfoLog ();

      // Check for success
      if (GL_FALSE == _compiled)
	throw std::runtime_error ("Compilation failed");
    }
  else
    {
      // Create a shader object
      _handle = glCreateShaderObjectARB (shaderType ());

      // Upload shader code to OpenGL
      glShaderSourceARB (_handle, 1, &code, NULL);

      // Compile shader
      glCompileShaderARB (_handle);
      glGetObjectParameterivARB (_handle, GL_OBJECT_COMPILE_STATUS_ARB, &_compiled);
      printInfoLog ();

      // Check for success
      if (GL_FALSE == _compiled)
	throw std::runtime_error ("Compilation failed");
    }
}

// -------------------------------------------------------------------------
// Shader::loadShaderFile
//
// Load shader's GLSL code from file.  The code is stored into the
// _code string member variable.
// -------------------------------------------------------------------------
void Shader::loadShaderFile (const string &filename)  throw (std::runtime_error) {
  // Open the file
  std::ifstream ifs (filename.c_str (), std::ios::in | std::ios::binary);

  if (ifs.fail ())
    throw std::runtime_error ("Couldn't open shader file: " + filename);

  // Read whole file into string
  _code.assign (std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

  // Close file
  ifs.close ();
}


/////////////////////////////////////////////////////////////////////////////
//
// class VertexShader implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// VertexShader::VertexShader
//
// Constructor.  Read vertex shader code from file and compile it.
// --------------------------------------------------------------------------
VertexShader::VertexShader (const string &filename)
  : Shader (filename) {
  try
    {
      // Load shader code from file
      loadShaderFile (filename);

      // Compile the shader
      compile ();

      cout << "* Vertex shader \"" << _name << "\" compiled" << endl;
    }
  catch (std::runtime_error &err)
    {
      cerr << "Error: Faild to create vertex shader from " << _name << endl;
      cerr << "Reason: " << err.what () << endl;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// class FragmentShader implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// FragmentShader::FragmentShader
//
// Constructor.  Read fragment shader code from file and compile it.
// --------------------------------------------------------------------------
FragmentShader::FragmentShader (const string &filename)
  : Shader (filename) {
  try
    {
      // Load shader code from file
      loadShaderFile (filename);

      // Compile the shader
      compile ();

      cout << "* Fragment shader \"" << _name << "\" compiled" << endl;
    }
  catch (std::runtime_error &err)
    {
      cerr << "Error: Faild to create fragment shader from " << _name << endl;
      cerr << "Reason: " << err.what () << endl;
    }
}
/////////////////////////////////////////////////////////////////////////////
//
// class ShaderProgram implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ShaderProgram::ShaderProgram
//
// Constructor.  Link vertex and fragment shader.  If the vertex shader
// or the fragment shader is invalid (has failed to compile), the
// shader program creation is aborted.
// --------------------------------------------------------------------------

ShaderProgram::ShaderProgram (const string &filename,
			      const VertexShader &vertexShader,
			      const FragmentShader &fragmentShader)
  : _name (filename), _handle (0), _linked (0) {
  try {
      if (vertexShader.fail ())
	throw std::runtime_error ("Invalid vertex shader");

      if (fragmentShader.fail ())
	throw std::runtime_error ("Invalid fragment shader");

      if (GLEW_VERSION_2_0)
	{
	  // Create program and attach vertex and fragment shaders
	  _handle = glCreateProgram ();
	  glAttachShader (_handle, vertexShader.handle ());
	  glAttachShader (_handle, fragmentShader.handle ());

	  // Perform link stage
	  glLinkProgram (_handle);
	  glGetProgramiv (_handle, GL_LINK_STATUS, &_linked);

	  // Validate program
	  glValidateProgram (_handle);
	  printInfoLog ();

	  // Check for success
	  if (GL_FALSE == _linked)
	    throw std::runtime_error ("Link stage failed");
	}
      else
	{
	  // Create program and attach vertex and fragment shaders
	  _handle = glCreateProgramObjectARB ();
	  glAttachObjectARB (_handle, vertexShader.handle ());
	  glAttachObjectARB (_handle, fragmentShader.handle ());

	  // Perform link stage
	  glLinkProgramARB (_handle);
	  glGetObjectParameterivARB (_handle, GL_OBJECT_LINK_STATUS_ARB, &_linked);

	  // Validate program
	  glValidateProgramARB (_handle);
	  printInfoLog ();

	  // Check for success
	  if (GL_FALSE == _linked)
	    throw std::runtime_error ("Link stage failed");
	}

      cout << "* Shader \"" << _name << "\" successfully linked" << endl;
    }
  catch (std::runtime_error &err)
    {
      cerr << "Error: Faild to create shader " << _name << endl;
      cerr << "Reason: " << err.what () << endl;
    }
}

// --------------------------------------------------------------------------
// ShaderProgram::~ShaderProgram
//
// Destructor.  Destroy the shader program handle.
// --------------------------------------------------------------------------
ShaderProgram::~ShaderProgram () {
  if (GLEW_VERSION_2_0)
    {
      if (glIsProgram (_handle))
	glDeleteProgram (_handle);
    }
  else
    {
      GLint type;
      glGetObjectParameterivARB (_handle, GL_OBJECT_TYPE_ARB, &type);

      if (GL_PROGRAM_OBJECT_ARB == type)
	glDeleteObjectARB (_handle);
    }
}


// -------------------------------------------------------------------------
// ShaderProgram::use
// ShaderProgram::unuse
//
// Bind/unbind the shader.
// -------------------------------------------------------------------------
void ShaderProgram::use () const {
  if (GLEW_VERSION_2_0)
    glUseProgram (_handle);
  else
    glUseProgramObjectARB (_handle);
}

void ShaderProgram::unuse () const {
  if (GLEW_VERSION_2_0)
    glUseProgram (0);
  else
    glUseProgramObjectARB (0);
}

// -------------------------------------------------------------------------
// ShaderProgram::printInfoLog
//
// Print log info about a shader program.
// -------------------------------------------------------------------------
void ShaderProgram::printInfoLog () const {
  GLint infologLength = 0;

  // First check for previous OpenGL errors...
  checkOpenGLErrors (__FILE__, __LINE__);

  // Get log's length
  if (GLEW_VERSION_2_0)
    glGetProgramiv (_handle, GL_INFO_LOG_LENGTH, &infologLength);
  else
    glGetObjectParameterivARB (_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB,
			       &infologLength);


  // If log is empty, quit
  if (infologLength <= 1)
    return;

  try
    {
      GLchar *infoLog = new GLchar[infologLength];

      // Get the log...
      if (GLEW_VERSION_2_0)
	glGetProgramInfoLog (_handle, infologLength, NULL, infoLog);
      else
	glGetInfoLogARB (_handle, infologLength, NULL, infoLog);

      // ...and print it to standard output
      cout << "Program \"" << _name << "\" InfoLog ("
	   << infologLength << "):" << endl << infoLog << endl;

      delete [] infoLog;
    }
  catch (std::bad_alloc &err)
    {
      cerr << "Error: memory allocation failed for shader program "
	   << "info log" << endl << " Reason: " << err.what () << endl;
    }
}

}}} //end namespace
