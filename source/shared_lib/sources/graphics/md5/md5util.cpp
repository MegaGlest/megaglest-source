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

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <GL/glew.h>

#include "GlErrors.h"
#include "Mathlib.h"
#include "Md5Model.h"
#include "TextureManager.h"
#include "ArbProgram.h"
#include "Shader.h"
#include "md5util.h"

namespace Shared { namespace Graphics { namespace md5 {


using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

// All vertex and fragment programs
ArbVertexProgram *vp_bump = NULL;
ArbVertexProgram *vp_bump_parallax = NULL;

ArbFragmentProgram *fp_diffuse = NULL;
ArbFragmentProgram *fp_diffuse_specular = NULL;
ArbFragmentProgram *fp_ds_parallax = NULL;

// Tangent uniform's location
GLint tangentLoc = -1;

int renderFlags = Md5Object::kDrawModel;

bool bAnimate = true;
bool bTextured = true;
bool bCullFace = true;
bool bBounds = false;
bool bParallax = false;
bool bLight = true;
bool bSmooth = true;
bool bWireframe = false;
bool bDrawNormals = false;

vector<string> animations;

// Camera
Vector3f rot, eye;

// -------------------------------------------------------------------------
// cleanupMD5OpenGL
//
// Application cleanup.
// -------------------------------------------------------------------------
void cleanupMD5OpenGL() {
  //delete model;
  //delete object;
  //delete font;
  //delete shader;

  delete Md5Model::shader;
  Md5Model::shader=NULL;
  delete vp_bump;
  delete vp_bump_parallax;
  delete fp_diffuse;
  delete fp_diffuse_specular;
  delete fp_ds_parallax;

  Texture2DManager::kill ();
}

// -------------------------------------------------------------------------
// initShader
//
// Shader's uniform variables initialization.
// -------------------------------------------------------------------------
void initShader () {
  if (NULL == Md5Model::shader)
    return;

  Md5Model::shader->use();

  if (GLEW_VERSION_2_0) {
      GLuint prog = Md5Model::shader->handle ();

      // Set uniform parameters
      glUniform1i (glGetUniformLocation (prog, "decalMap"), 0);
      glUniform1i (glGetUniformLocation (prog, "glossMap"), 1);
      glUniform1i (glGetUniformLocation (prog, "normalMap"), 2);
      glUniform1i (glGetUniformLocation (prog, "heightMap"), 3);
      glUniform1i (glGetUniformLocation (prog, "parallaxMapping"), bParallax);

      // Get attribute location
      Md5Model::tangentLoc = glGetAttribLocation (prog, "tangent");
  }
  else {
      GLhandleARB prog = Md5Model::shader->handle();

      // Set uniform parameters
      glUniform1iARB (glGetUniformLocationARB (prog, "decalMap"), 0);
      glUniform1iARB (glGetUniformLocationARB (prog, "glossMap"), 1);
      glUniform1iARB (glGetUniformLocationARB (prog, "normalMap"), 2);
      glUniform1iARB (glGetUniformLocationARB (prog, "heightMap"), 3);
      glUniform1iARB (glGetUniformLocationARB (prog, "parallaxMapping"), bParallax);

      // Get attribute location
      Md5Model::tangentLoc = glGetAttribLocationARB (prog, "tangent");
  }

  Md5Model::shader->unuse();

  // Warn ff we fail to get tangent location... We'll can still use
  // the shader, but without tangents
  if(Md5Model::tangentLoc == -1)
    cerr << "Warning! No \"tangent\" uniform found in shader!" << endl;
}

// -------------------------------------------------------------------------
// announceRenderPath
//
// Print info about a render path.
// -------------------------------------------------------------------------
void announceRenderPath (render_path_e path) {
  cout << "Render path: ";
  switch (path)
    {
    case R_normal:
      cout << "no bump mapping (fixed pipeline)" << endl;
      break;

    case R_ARBfp_diffuse:
      cout << "bump mapping, diffuse only "
	   << "(ARB vp & fp)" << endl;
      break;

    case R_ARBfp_diffuse_specular:
      cout << "bump mapping, diffuse and specular "
	   << "(ARB vp & fp)" << endl;
      break;

    case R_ARBfp_ds_parallax:
      cout << "bump mapping with parallax "
	   << "(ARB fp & fp)" << endl;
      break;

    case R_shader:
      cout << "bump mapping with parallax "
	   << "(GLSL)" << endl;
      break;
    }
}

// -------------------------------------------------------------------------
// initMD5OpenGL
//
// OpenGL initialization.
// -------------------------------------------------------------------------
void initMD5OpenGL(string shaderPath) {
  //glClearColor (0.5f, 0.5f, 0.5f, 0.0f);
  //glShadeModel (GL_SMOOTH);
  //glCullFace (GL_BACK);
  //glEnable (GL_DEPTH_TEST);

  // Initialize GLEW
  GLenum err = glewInit ();
  if(GLEW_OK != err) {
      // Problem: glewInit failed, something is seriously wrong.
      cerr << "Error: " << glewGetErrorString (err) << endl;
      cleanupMD5OpenGL();
  }

  // Print some infos about user's OpenGL implementation
  cout << "OpenGL Version String: " << glGetString (GL_VERSION) << endl;
  cout << "GLU Version String: " << gluGetString (GLU_VERSION) << endl;
  cout << "GLEW Version String: " << glewGetString (GLEW_VERSION) << endl;

  // Initialize ARB vertex/fragment program support
  initArbProgramHandling();

  // Initialize GLSL shader support
  initShaderHandling();

  if(hasArbVertexProgramSupport () &&
      hasArbFragmentProgramSupport ()) {
      // Load ARB programs
      vp_bump = new ArbVertexProgram(shaderPath + "bump.vp");
      vp_bump_parallax = new ArbVertexProgram(shaderPath + "bumpparallax.vp");

      fp_diffuse = new ArbFragmentProgram(shaderPath + "bumpd.fp");
      fp_diffuse_specular = new ArbFragmentProgram(shaderPath + "bumpds.fp");
      fp_ds_parallax = new ArbFragmentProgram(shaderPath + "bumpdsp.fp");

      // Current ARB programs will be bump mapping with diffuse
      // and specular components
      Md5Model::vp = vp_bump;
      Md5Model::fp = fp_diffuse_specular;
  }

  if(hasShaderSupport ()) {
      // Load shader
      VertexShader vs(shaderPath + "bump.vert");
      FragmentShader fs(shaderPath + "bump.frag");
      Md5Model::shader = new ShaderProgram(shaderPath + "bump mapping", vs, fs);

      // Initialize shader's uniforms
      initShader();
  }

  // Announce avalaible render paths, select the best
  cout << endl << "Available render paths:" << endl;

  cout << " [F3] - No bump mapping (fixed pipeline)" << endl;
  Md5Model::renderPath = R_normal;

  if (vp_bump && fp_diffuse) {
      cout << " [F4] - Bump mapping, diffuse only "
	   << "(ARB vp & fp)" << endl;
      Md5Model::renderPath = R_ARBfp_diffuse;
  }

  if (vp_bump && fp_diffuse_specular) {
      cout << " [F5] - Bump mapping, diffuse and specular "
	   << "(ARB vp & fp)" << endl;
      Md5Model::renderPath = R_ARBfp_diffuse_specular;
  }

  if (vp_bump_parallax && fp_ds_parallax) {
      cout << " [F6] - Bump mapping with parallax "
	   << "(ARB vp & fp)" << endl;
  }

  if (Md5Model::shader) {
      cout << " [F7] - Bump mapping with parallax "
	   << "(GLSL)" << endl;
      Md5Model::renderPath = R_shader;
  }

  // Announce which path has been chosen by default
  cout << endl;
  announceRenderPath(Md5Model::renderPath);

  checkOpenGLErrors (__FILE__, __LINE__);
}

// -------------------------------------------------------------------------
// extractFromQuotes
//
// Extract a string from quotes.
// -------------------------------------------------------------------------
inline const string extractFromQuotes (const string &str) {
  string::size_type start = str.find_first_of ('\"') + 1;
  string::size_type end = str.find_first_of ('\"', start) - 2;
  return str.substr (start, end);
}

// -------------------------------------------------------------------------
// getMD5ObjectFromLoaderScript
//
// Parse a script file for loading md5mesh and animations.
// -------------------------------------------------------------------------
Md5Object * getMD5ObjectFromLoaderScript(const string &filename) {
  // Open the file to parse
  std::ifstream file (filename.c_str(), std::ios::in);

  if (file.fail ()) {
      cerr << "Couldn't open " << filename << endl;
      cleanupMD5OpenGL();
  }

  // Get texture manager
  Texture2DManager *texMgr = Texture2DManager::getInstance();

  Md5Model *model = NULL;
  Md5Object *object = NULL;

  while (!file.eof ()) {
      string token, buffer;
      string meshFile, animFile, textureFile;
      string meshName, animName;

      // Peek next token
      file >> token;

      if (token == "model") {
		  std::getline (file, buffer);
		  meshFile = extractFromQuotes (buffer);

		  // Delete previous model and object if existing
		  delete model;
		  delete object;

		  // Load mesh model
		  model = new Md5Model(meshFile);
		  object = new Md5Object(model);
      }
      else if (token == "anim") {
		  std::getline (file, buffer);
		  animFile = extractFromQuotes (buffer);

		  try {
			  // Load animation
			  if (model) {
				  model->addAnim(animFile);
			  }
		  }
		  catch (Md5Exception &err) {
			  cerr << "Failed to load animation "
			   << animFile << endl;
			  cerr << "Reason: " << err.what ()
			   << " (" << err.which () << ")" << endl;
		  }
      }
      else if (token == "hide")	{
		  std::getline (file, buffer);
		  meshName = extractFromQuotes (buffer);

		  // Set mesh's render state
		  if (model) {
			  model->setMeshRenderState (meshName, Md5Mesh::kHide);
		  }
      }
      else if ((token == "decalMap") ||
	       (token == "specularMap") ||
	       (token == "normalMap") ||
	       (token == "heightMap")) {
		  // Get the next token and extract the mesh name
		  file >> buffer;
		  long start = buffer.find_first_of ('\"') + 1;
		  long end = buffer.find_first_of ('\"', start) - 1;
		  meshName = buffer.substr (start, end);

		  // Get the rest of line and extract texture's filename
		  std::getline (file, buffer);
		  textureFile = extractFromQuotes (buffer);

		  // If the model has been loaded, setup
		  // the texture to the desired mesh
		  if (model) {
			  Texture2D *tex = texMgr->load (textureFile);
			  if (tex->fail ())
				  cerr << "failed to load " << textureFile << endl;

			  if (token == "decalMap")
				  model->setMeshDecalMap (meshName, tex);
			  else if (token == "specularMap")
				  model->setMeshSpecularMap (meshName, tex);
			  else if (token == "normalMap")
				  model->setMeshNormalMap (meshName, tex);
			  else if (token == "heightMap")
				  model->setMeshHeightMap (meshName, tex);
		  }
      }
      else if (token == "setAnim") {
		  std::getline (file, buffer);
		  animName = extractFromQuotes (buffer);

		  // Set object's default animation
		  object->setAnim (animName);
      }
    }

  	file.close ();

  	if (!model || !object)
  		throw Md5Exception ("No mesh found!", filename);

  	return object;
}

// -------------------------------------------------------------------------
// setupLight
//
// Setup light position and enable light0.
// -------------------------------------------------------------------------
void setupLight(GLfloat x, GLfloat y, GLfloat z) {
  GLfloat lightPos[4];
  lightPos[0] = x;
  lightPos[1] = y;
  lightPos[2] = z;
  lightPos[3] = 1.0f;

  glDisable (GL_LIGHTING);
  glDisable (GL_LIGHT0);

  if (bLight)
    {
      glPushMatrix ();
	glLoadIdentity ();
	glLightfv (GL_LIGHT0, GL_POSITION, lightPos);
      glPopMatrix ();

      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);
    }
}


// -------------------------------------------------------------------------
// drawObb
//
// Draw an Oriented Bouding Box.
// -------------------------------------------------------------------------
void drawObb(const OBBox_t &obb) {
  Vector3f corners[8];

  corners[0] = Vector3f (-obb.extent._x, -obb.extent._y, -obb.extent._z);
  corners[1] = Vector3f ( obb.extent._x, -obb.extent._y, -obb.extent._z);
  corners[2] = Vector3f ( obb.extent._x,  obb.extent._y, -obb.extent._z);
  corners[3] = Vector3f (-obb.extent._x,  obb.extent._y, -obb.extent._z);
  corners[4] = Vector3f (-obb.extent._x, -obb.extent._y,  obb.extent._z);
  corners[5] = Vector3f ( obb.extent._x, -obb.extent._y,  obb.extent._z);
  corners[6] = Vector3f ( obb.extent._x,  obb.extent._y,  obb.extent._z);
  corners[7] = Vector3f (-obb.extent._x,  obb.extent._y,  obb.extent._z);

  glPushAttrib (GL_ENABLE_BIT);
  glDisable (GL_TEXTURE_2D);
  glDisable (GL_LIGHTING);

  for (int i = 0; i < 8; ++i) {
      corners[i] += obb.center;
      obb.world.transform (corners[i]);
  }

  GLuint indices[24] =
    {
      0, 1, 1, 2, 2, 3, 3, 0,
      4, 5, 5, 6, 6, 7, 7, 4,
      0, 4, 1, 5, 2, 6, 3, 7
    };

  glColor3f (1.0, 0.0, 0.0);

  glEnableClientState (GL_VERTEX_ARRAY);
  glVertexPointer (3, GL_FLOAT, 0, corners);
  glDrawElements (GL_LINES, 24, GL_UNSIGNED_INT, indices);
  glDisableClientState (GL_VERTEX_ARRAY);

  // GL_ENABLE_BIT
  glPopAttrib();
}

// -------------------------------------------------------------------------
// drawAxes
//
// Draw the X, Y and Z axes at the center of world.
// -------------------------------------------------------------------------
void drawAxes(const Matrix4x4f &modelView) {
  // Setup world model view matrix
  glLoadIdentity ();
  glMultMatrixf (modelView._m);

  // Draw the three axes
  glBegin (GL_LINES);
    // X-axis in red
    glColor3f (1.0f, 0.0f, 0.0f);
    glVertex3fv (kZeroVectorf._v);
    glVertex3fv (kZeroVectorf + Vector3f (10.0f, 0.0f, 0.0));

    // Y-axis in green
    glColor3f (0.0f, 1.0f, 0.0f);
    glVertex3fv (kZeroVectorf._v);
    glVertex3fv (kZeroVectorf + Vector3f (0.0f, 10.0f, 0.0));

    // Z-axis in blue
    glColor3f (0.0f, 0.0f, 1.0f);
    glVertex3fv (kZeroVectorf._v);
    glVertex3fv (kZeroVectorf + Vector3f (0.0f, 0.0f, 10.0));
  glEnd ();
}

// -------------------------------------------------------------------------
// renderMD5Object
//
// Render the 3D part of the scene.
// -------------------------------------------------------------------------
void renderMD5Object(Md5Object *object, double anim, Matrix4x4f *modelViewMatrix) {
  if(!object) {
	  return;
  }

  // Clear the window
  //glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  // Camera rotation
  Matrix4x4f camera;

#if 0
  camera.identity ();

  glTranslated (-eye._x, -eye._y, -eye._z);
  glRotated (rot._x, 1.0f, 0.0f, 0.0f);
  glRotated (rot._y, 0.0f, 1.0f, 0.0f);
  glRotated (rot._z, 0.0f, 0.0f, 1.0f);
#else
  camera.fromEulerAngles (degToRad (rot._x),
			  degToRad (rot._y),
			  degToRad (rot._z));
  camera.setTranslation (-eye);
#endif

  Matrix4x4f axisRotation
    = RotationMatrix (kXaxis, -kPiOver2)
    * RotationMatrix (kZaxis, -kPiOver2);

  Matrix4x4f final = camera * axisRotation;
  //glMultMatrixf (final._m);
  if(modelViewMatrix) {
	  final = *modelViewMatrix;
  }

  // Setup scene lighting
  setupLight (0.0f, 20.0f, 100.0f);

  // Enable/disable texture mapping (fixed pipeline)
  if (bTextured)
    glEnable (GL_TEXTURE_2D);
  else
    glDisable (GL_TEXTURE_2D);

  // Enable/disable backface culling
  if (bCullFace)
    glEnable (GL_CULL_FACE);
  else
    glDisable (GL_CULL_FACE);

  // Setup polygon mode and shade model
  glPolygonMode (GL_FRONT_AND_BACK, bWireframe ? GL_LINE : GL_FILL);
  glShadeModel (bSmooth ? GL_SMOOTH : GL_FLAT);

  // Draw object
  object->setModelViewMatrix (final);
  object->setRenderFlags (renderFlags);
  //object->animate (bAnimate ? timer.deltaTime () : 0.0f);
  object->animate (anim);
  object->computeBoundingBox ();
  object->prepare (false);
  object->render ();

  if (bBounds)
    drawObb (object->boundingBox ());

  glDisable (GL_LIGHTING);
  glDisable (GL_TEXTURE_2D);

  drawAxes (final);
}

}}} //end namespace
