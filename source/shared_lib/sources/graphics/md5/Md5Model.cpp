// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Md5Model.h -- Copyright (c) 2006-2007 David Henry
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
// Implementation of MD5 Model classes.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>

#include "md5Texture.h"
#include "ArbProgram.h"
#include "ShaderManager.h"
#include "Md5Model.h"

namespace Shared { namespace Graphics { namespace md5 {


using std::cout;
using std::cerr;
using std::endl;

/////////////////////////////////////////////////////////////////////////////
//
// global stuff
//
/////////////////////////////////////////////////////////////////////////////

render_path_e Md5Model::renderPath;
ShaderProgram *Md5Model::shader=NULL;
ArbVertexProgram *Md5Model::vp=NULL;
ArbFragmentProgram *Md5Model::fp=NULL;
// Tangent uniform's location
GLint Md5Model::tangentLoc = -1;
bool Md5Model::bDrawNormals = false;

const int kMd5Version = 10;

// Sort functor for joints
struct SortByDepth :
  public std::binary_function<Md5Joint_t, Md5Joint_t, bool>
{
  bool operator() (const Md5Joint_t &j1, const Md5Joint_t &j2) const {
    return (j1.pos._z < j2.pos._z);
  }
};

/////////////////////////////////////////////////////////////////////////////
//
// class Md5Skeleton implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Md5Skeleton::Md5Skeleton
//
// Constructor.  Load skeleton data from a <.md5mesh> file.
// --------------------------------------------------------------------------
Md5Skeleton::Md5Skeleton (std::ifstream &ifs, int numJoints)
  throw (Md5Exception) {
  string token, buffer;

  if (!ifs.is_open())
    throw Md5Exception ("Input stream not opened!", "skeleton");

  // Read all joints
  for (int i = 0; i < numJoints; ++i)
    {
      // NOTE: hope there isn't any comment between
      // two lines of joints data...
      Md5JointPtr j (new Md5Joint_t);

      ifs >> j->name;
      ifs >> j->parent;
      ifs >> token; // "("
      ifs >> j->pos._x;
      ifs >> j->pos._y;
      ifs >> j->pos._z;
      ifs >> token; // ")"
      ifs >> token; // "("
      ifs >> j->orient._x;
      ifs >> j->orient._y;
      ifs >> j->orient._z;
      ifs >> token; // ")"

      // Eat up rest of the line
      std::getline (ifs, buffer);

      // Compute orient quaternion's w value
      j->orient.computeW ();

      // Add joint to joints vector
      _joints.push_back (j);
    }
}

// --------------------------------------------------------------------------
// Md5Skeleton::~Md5Skeleton
//
// Destructor.  Free all data allocated for skeleton's joints.
// --------------------------------------------------------------------------
Md5Skeleton::~Md5Skeleton () {
}

// --------------------------------------------------------------------------
// Md5Skeleton::draw
//
// Draw skeleton's bones and joints.
// --------------------------------------------------------------------------
void Md5Skeleton::draw (const Matrix4x4f &modelView, bool labelJoints) {
  // Draw each joint
  glPointSize (5.0f);
  glColor3f (1.0f, 0.0f, 0.0f);
  glBegin (GL_POINTS);
    for (unsigned int i = 0; i < _joints.size (); ++i)
      glVertex3fv (_joints[i]->pos);
  glEnd ();
  glPointSize (1.0f);

  // Draw each bone
  glColor3f (0.0f, 1.0f, 0.0f);
  glBegin (GL_LINES);
    for (unsigned int i = 0; i < _joints.size (); ++i)
      {
	if (_joints[i]->parent != -1)
	  {
	    glVertex3fv (_joints[ _joints[i]->parent ]->pos);
	    glVertex3fv (_joints[i]->pos);
	  }
      }
  glEnd ();

  // Label each joint
  /*
  if (labelJoints && font)
    {
      vector<Md5Joint_t> jointlist (_joints.size ());

      // Copy joint's position and name
      for (unsigned int i = 0; i < _joints.size (); ++i)
	jointlist.push_back (*_joints[i]);

      // Sort joints about depth because of alpha blending
      std::sort (jointlist.begin (), jointlist.end (), SortByDepth ());

      glActiveTexture (GL_TEXTURE0);
      glMatrixMode (GL_TEXTURE);
      glLoadIdentity ();

      GLfloat mat[16];
      glMatrixMode (GL_MODELVIEW);
      glGetFloatv (GL_MODELVIEW_MATRIX, mat);

      glPushMatrix ();
	// Setup billboard matrix
	mat[0] = 1.0f; mat[1] = 0.0f; mat[2] = 0.0f;
	mat[4] = 0.0f; mat[5] = 1.0f; mat[6] = 0.0f;
	mat[8] = 0.0f; mat[9] = 0.0f; mat[10]= 1.0f;

	glLoadMatrixf (mat);

	glPushAttrib (GL_POLYGON_BIT);
	glFrontFace (GL_CCW);
	glPolygonMode (GL_FRONT, GL_FILL);
	glColor3f (1.0f, 1.0f, 1.0f);

	glLoadIdentity ();
	glScalef (0.1f, 0.1f, 0.1f);

	for (unsigned int i = 0; i < _joints.size (); ++i)
	  {
	    glPushMatrix ();
	      // Move to joint's position
	      glTranslatef (jointlist[i].pos._x * 10.0f,
			    jointlist[i].pos._y * 10.0f,
			    jointlist[i].pos._z * 10.0f);

	      font->printText (jointlist[i].name.c_str ());
	    glPopMatrix();
	  }

	// GL_POLYGON_BIT
	glPopAttrib ();
      glPopMatrix ();
  }
  */
}

// --------------------------------------------------------------------------
// Md5Skeleton::setNumJoints
//
// Reserve memory to hold numJoints joints.
// --------------------------------------------------------------------------
void Md5Skeleton::setNumJoints (int numJoints) {
  _joints.reserve (numJoints);
}

// --------------------------------------------------------------------------
// Md5Skeleton::addJoint
//
// Add a joint to the skeleton.
// --------------------------------------------------------------------------
void Md5Skeleton::addJoint (Md5Joint_t *thisJoint) {
  _joints.push_back (Md5JointPtr (thisJoint));
}

// --------------------------------------------------------------------------
// Md5Skeleton::clone
//
// Dupplicate the skeleton.
// --------------------------------------------------------------------------
Md5Skeleton* Md5Skeleton::clone () const {
  Md5Skeleton *cpy = new Md5Skeleton;
  cpy->setNumJoints (_joints.size ());

  for (size_t i = 0; i < _joints.size (); ++i)
    cpy->addJoint (new Md5Joint_t (*_joints[i].get ()));

  return cpy;
}

/////////////////////////////////////////////////////////////////////////////
//
// class Md5Mesh implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Md5Mesh::Md5Mesh
//
// Constructor.  Load mesh data from a <.md5mesh> file.
// --------------------------------------------------------------------------
Md5Mesh::Md5Mesh (std::ifstream &ifs)
  throw (Md5Exception)
  : _renderState (kShow), _numVerts (0), _numTris (0), _numWeights (0),
    _decal (NULL), _specMap (NULL), _normalMap (NULL), _heightMap (NULL) {
  string token, buffer;

  if (!ifs.is_open())
    throw Md5Exception ("Input stream not opened!", "mesh");

  do
    {
      // Read first token line from file
      ifs >> token;

      if (token == "shader")
	{
	  ifs >> _shader;

	  // Remove quote marks from the shader string
	  _shader = _shader.substr (_shader.find_first_of ('\"') + 1,
				    _shader.find_last_of ('\"') - 1);

	  // Get mesh name from shader string
	  _name = _shader.c_str() + _shader.find_last_of ('/') + 1;
	}
      else if (token == "numverts")
	{
	  ifs >> _numVerts;
	  _verts.reserve (_numVerts);
	}
      else if (token == "numtris")
	{
	  ifs >> _numTris;
	  _tris.reserve (_numTris);
	}
      else if (token == "numweights")
	{
	  ifs >> _numWeights;
	  _weights.reserve (_numWeights);
	}
      else if (token == "vert")
	{
	  Md5VertexPtr vert (new Md5Vertex_t);

	  // Read vertex data
	  ifs >> token; // index
	  ifs >> token; // "("
	  ifs >> vert->st[0];
	  ifs >> vert->st[1];
	  ifs >> token; // ")"
	  ifs >> vert->startWeight;
	  ifs >> vert->countWeight;

	  _verts.push_back (vert);
	}
      else if (token == "tri")
	{
	  Md5TrianglePtr tri (new Md5Triangle_t);

	  // Read triangle data
	  ifs >> token; // index
	  ifs >> tri->index[0];
	  ifs >> tri->index[1];
	  ifs >> tri->index[2];

	  _tris.push_back (tri);
	}
      else if (token == "weight")
	{
	  Md5WeightPtr weight (new Md5Weight_t);

	  // Read weight data
	  ifs >> token; // index
	  ifs >> weight->joint;
	  ifs >> weight->bias;
	  ifs >> token; // "("
	  ifs >> weight->pos._x;
	  ifs >> weight->pos._y;
	  ifs >> weight->pos._z;
	  ifs >> token; // ")"

	  _weights.push_back (weight);
	}

      // Eat up rest of the line
      std::getline (ifs, buffer);

    }
  while ((token != "}") && !ifs.eof ());

  // Memory allocation for vertex arrays
  allocVertexArrays ();

  // Setup texture coordinates array
  setupTexCoordArray ();
}

// --------------------------------------------------------------------------
// Md5Mesh::~Md5Mesh
//
// Destructor.  Free all data allocated for the mesh, i.e. vertices,
// triangles, weights and vertex arrays.
// --------------------------------------------------------------------------
Md5Mesh::~Md5Mesh () {
}

// --------------------------------------------------------------------------
// Md5Mesh::setupVertexArray
//
// Compute vertices' position, normal and tangent.
// --------------------------------------------------------------------------
void Md5Mesh::setupVertexArrays (Md5Skeleton *skel) {
  for (int i = 0; i < _numVerts; ++i)
    {
      Vector3f finalVertex = kZeroVectorf;
      Vector3f finalNormal = kZeroVectorf;
      Vector3f finalTangent = kZeroVectorf;

      // Calculate final vertex to draw with weights
      for (int j = 0; j < _verts[i]->countWeight; ++j)
	{
	  const Md5Weight_t *pWeight
	    = _weights[_verts[i]->startWeight + j].get ();
	  const Md5Joint_t *pJoint
	    = skel->joint (pWeight->joint);

	  // Calculate transformed vertex for this weight
	  Vector3f wv = pWeight->pos;
	  pJoint->orient.rotate (wv);

	  // The sum of all pWeight->bias should be 1.0
	  finalVertex += (pJoint->pos + wv) * pWeight->bias;

	  // Calculate transformed normal for this weight
	  Vector3f wn = pWeight->norm;
	  pJoint->orient.rotate (wn);

	  finalNormal += wn * pWeight->bias;

	  // Calculate transformed tangent for this weight
	  Vector3f wt = pWeight->tan;
	  pJoint->orient.rotate (wt);

	  finalTangent += wt * pWeight->bias;
	}

      // We can omit to normalize normal and tangent,
      // because they should have been already normalized
      // when they were computed. We can gain some time
      // avoiding some heavy calculus.

      //finalNormal.normalize ();
      //finalTangent.normalize ();

      {
	// Fill in the vertex arrays with the freshly vertex, normal
	// and tangent computed.

	GLfloat *vertexPointer = &_vertexArray[i * 3];
	GLfloat *normalPointer = &_normalArray[i * 3];
	GLfloat *tangentPointer = &_tangentArray[i * 3];

	vertexPointer[0] = finalVertex._x;
	vertexPointer[1] = finalVertex._y;
	vertexPointer[2] = finalVertex._z;

	normalPointer[0] = finalNormal._x;
	normalPointer[1] = finalNormal._y;
	normalPointer[2] = finalNormal._z;

	tangentPointer[0] = finalTangent._x;
	tangentPointer[1] = finalTangent._y;
	tangentPointer[2] = finalTangent._z;
      }
    }
}


// --------------------------------------------------------------------------
// Md5Mesh::computeWeightNormals
//
// der_ton said:
//
// * First you have to get the bind-pose model-space normals by calculating
//   them from the model geometry in bind-pose.
//
// * Then you calculate the weight's normal (which is in bone-space) by
//   invert-transforming the normal by the bone-space matrix.
//
// * So afterwards when animating, you'll transform the weight normal with
//   the animated bone-space matrix and add them all up and you'll get
//   back your animated vertex normal.
// --------------------------------------------------------------------------

void
Md5Mesh::computeWeightNormals (Md5Skeleton *skel)
{
  vector<Vector3f> bindposeVerts (_numVerts);
  vector<Vector3f> bindposeNorms (_numVerts);

  for (int i = 0; i < _numVerts; ++i)
    {
      // Zero out final vertex position and final vertex normal
      bindposeVerts[i] = kZeroVectorf;
      bindposeNorms[i] = kZeroVectorf;

      for (int j = 0; j < _verts[i]->countWeight; ++j)
	{
	  const Md5Weight_t *pWeight
	    = _weights[_verts[i]->startWeight + j].get ();
	  const Md5Joint_t *pJoint
	    = skel->joint (pWeight->joint);

	  // Calculate transformed vertex for this weight
	  Vector3f wv = pWeight->pos;
	  pJoint->orient.rotate (wv);

	  bindposeVerts[i] += (pJoint->pos + wv) * pWeight->bias;
	}
    }

  // Compute triangle normals
  for (int i = 0; i < _numTris; ++i)
    {
      const Md5Triangle_t *pTri = _tris[i].get ();

      Vector3f triNorm (-ComputeNormal (bindposeVerts[pTri->index[0]],
	bindposeVerts[pTri->index[1]], bindposeVerts[pTri->index[2]]));

      for (int j = 0; j < 3; ++j)
	bindposeNorms[pTri->index[j]] += triNorm;
    }

  // "Average" the surface normals, by normalizing them
  for (int i = 0; i < _numVerts; ++i)
    bindposeNorms[i].normalize ();

  //
  // At this stage we have all vertex normals computed
  // for the model geometry in bind-pos
  //

  // Zero out all weight normals
  for (int i = 0; i < _numWeights; ++i)
    _weights[i]->norm = kZeroVectorf;

  // Compute weight normals by invert-transforming the normal
  // by the bone-space matrix
  for (int i = 0; i < _numVerts; ++i)
    {
      for (int j = 0; j < _verts[i]->countWeight; ++j)
	{
	  Md5Weight_t *pWeight
	    = _weights[_verts[i]->startWeight + j].get ();
	  const Md5Joint_t *pJoint
	    = skel->joint (pWeight->joint);

	  Vector3f wn = bindposeNorms[i];

	  // Compute inverse quaternion rotation
	  Quaternionf invRot = Inverse (pJoint->orient);
	  invRot.rotate (wn);

	  pWeight->norm += wn;
	}
    }

  // Normalize all weight normals
  for (int i = 0; i < _numWeights; ++i)
    _weights[i]->norm.normalize ();
}


// --------------------------------------------------------------------------
// Md5Mesh::computeWeightTangents
//
// Compute per-vertex tangent vectors and then, calculate the weight
// tangent.
// --------------------------------------------------------------------------

void
Md5Mesh::computeWeightTangents (Md5Skeleton *skel)
{
  vector<Vector3f> bindposeVerts (_numVerts);
  vector<Vector3f> bindposeNorms (_numVerts);
  vector<Vector3f> bindposeTans (_numVerts);

  vector<Vector3f> sTan (_numVerts);
  vector<Vector3f> tTan (_numVerts);

  // Zero out all weight tangents (thank you Valgrind)
  for (int i = 0; i < _numWeights; ++i)
    _weights[i]->tan = kZeroVectorf;

  // Compute bind-pose vertices and normals
  for (int i = 0; i < _numVerts; ++i)
    {
      // Zero out final vertex position, normal and tangent
      bindposeVerts[i] = kZeroVectorf;
      bindposeNorms[i] = kZeroVectorf;
      bindposeTans[i] = kZeroVectorf;

      // Zero s-tangents and t-tangents
      sTan[i] = kZeroVectorf;
      tTan[i] = kZeroVectorf;

      for (int j = 0; j < _verts[i]->countWeight; ++j)
	{
	  const Md5Weight_t *pWeight
	    = _weights[_verts[i]->startWeight + j].get ();
	  const Md5Joint_t *pJoint
	    = skel->joint (pWeight->joint);

	  // Calculate transformed vertex for this weight
	  Vector3f wv = pWeight->pos;
	  pJoint->orient.rotate (wv);

	  bindposeVerts[i] += (pJoint->pos + wv) * pWeight->bias;

	  // Calculate transformed normal for this weight
	  Vector3f wn = pWeight->norm;
	  pJoint->orient.rotate (wn);

	  bindposeNorms[i] += wn * pWeight->bias;
	}
    }

  // Calculate s-tangeants and t-tangeants at triangle level
  for (int i = 0; i < _numTris; ++i)
    {
      const Md5Triangle_t *pTri = _tris[i].get ();

      const Vector3f &v0 = bindposeVerts[pTri->index[0]];
      const Vector3f &v1 = bindposeVerts[pTri->index[1]];
      const Vector3f &v2 = bindposeVerts[pTri->index[2]];

      const vec2_t &w0 = _verts[pTri->index[0]]->st;
      const vec2_t &w1 = _verts[pTri->index[1]]->st;
      const vec2_t &w2 = _verts[pTri->index[2]]->st;

      float x1 = v1._x - v0._x;
      float x2 = v2._x - v0._x;
      float y1 = v1._y - v0._y;
      float y2 = v2._y - v0._y;
      float z1 = v1._z - v0._z;
      float z2 = v2._z - v0._z;

      float s1 = w1[0] - w0[0];
      float s2 = w2[0] - w0[0];
      float t1 = w1[1] - w0[1];
      float t2 = w2[1] - w0[1];

      float r = (s1 * t2) - (s2 * t1);

      if (r == 0.0f)
	// Prevent division by zero
	r = 1.0f;

      float oneOverR = 1.0f / r;

      Vector3f sDir ((t2 * x1 - t1 * x2) * oneOverR,
		     (t2 * y1 - t1 * y2) * oneOverR,
		     (t2 * z1 - t1 * z2) * oneOverR);
      Vector3f tDir ((s1 * x2 - s2 * x1) * oneOverR,
		     (s1 * y2 - s2 * y1) * oneOverR,
		     (s1 * z2 - s2 * z1) * oneOverR);

      for (int j = 0; j < 3; ++j)
	{
	  sTan[pTri->index[j]] += sDir;
	  tTan[pTri->index[j]] += tDir;
	}
    }

  // Calculate vertex tangent
  for (int i = 0; i < _numVerts; ++i)
    {
      const Vector3f &n = bindposeNorms[i];
      const Vector3f &t = sTan[i];

      // Gram-Schmidt orthogonalize
      bindposeTans[i] = (t - n * DotProduct (n, t));
      bindposeTans[i].normalize ();

      // Calculate handedness
      if (DotProduct (CrossProduct (n, t), tTan[i]) < 0.0f)
	bindposeTans[i] = -bindposeTans[i];

      // Compute weight tangent
      for (int j = 0; j < _verts[i]->countWeight; ++j)
	{
	  Md5Weight_t *pWeight
	    = _weights[_verts[i]->startWeight + j].get ();
	  const Md5Joint_t *pJoint
	    = skel->joint (pWeight->joint);

	  Vector3f wt = bindposeTans[i];

	  // Compute inverse quaternion rotation
	  Quaternionf invRot = Inverse (pJoint->orient);
	  invRot.rotate (wt);

	  pWeight->tan += wt;
	}
    }

  // Normalize all weight tangents
  for (int i = 0; i < _numWeights; ++i)
    _weights[i]->tan.normalize ();
}


// --------------------------------------------------------------------------
// Md5Mesh::computeBoundingBox
//
// Compute mesh bounding box for a given skeleton.
// --------------------------------------------------------------------------

void Md5Mesh::computeBoundingBox (Md5Skeleton *skel) {
  Vector3f maxValue(-99999.0f, -99999.0f, -99999.0f);
  Vector3f minValue( 99999.0f,  99999.0f,  99999.0f);

  for (int i = 0; i < _numVerts; ++i) {
      Vector3f finalVertex = kZeroVectorf;

      // Calculate final vertex to draw with weights
      for (int j = 0; j < _verts[i]->countWeight; ++j) {
	  const Md5Weight_t *pWeight
	    = _weights[_verts[i]->startWeight + j].get ();
	  const Md5Joint_t *pJoint
	    = skel->joint (pWeight->joint);

	  // Calculate transformed vertex for this weight
	  Vector3f wv = pWeight->pos;
	  pJoint->orient.rotate (wv);

	  // The sum of all pWeight->bias should be 1.0
	  finalVertex += (pJoint->pos + wv) * pWeight->bias;
	}

      if (finalVertex._x > maxValue._x)
	maxValue._x = finalVertex._x;

      if (finalVertex._x < minValue._x)
	minValue._x = finalVertex._x;

      if (finalVertex._y > maxValue._y)
	maxValue._y = finalVertex._y;

      if (finalVertex._y < minValue._y)
	minValue._y = finalVertex._y;

      if (finalVertex._z > maxValue._z)
	maxValue._z = finalVertex._z;

      if (finalVertex._z < minValue._z)
	minValue._z = finalVertex._z;
    }

  _boundingBox.min = minValue;
  _boundingBox.max = maxValue;
}

// --------------------------------------------------------------------------
// Md5Mesh::preRenderVertexArrays
//
// Pre-rendering preparation.  Setup some render-path dependent stuff,
// like tangent arrays for ARB programs and shaders.
// --------------------------------------------------------------------------

void
Md5Mesh::preRenderVertexArrays () const
{
  switch (Md5Model::renderPath)
    {
    case R_normal:
      break;

    case R_ARBfp_diffuse:
    case R_ARBfp_diffuse_specular:
    case R_ARBfp_ds_parallax:
      {
    	  Md5Model::vp->use ();
    	  Md5Model::fp->use ();

	glEnableVertexAttribArrayARB (TANGENT_LOC);
	glVertexAttribPointerARB (TANGENT_LOC, 3, GL_FLOAT, GL_FALSE,
				  0, &_tangentArray.front ());
	break;
      }

    case R_shader:
      {
	if (Md5Model::tangentLoc == -1)
	  break;

	Md5Model::shader->use ();

	if (GLEW_VERSION_2_0)
	  {
	    glEnableVertexAttribArray (Md5Model::tangentLoc);
	    glVertexAttribPointer (Md5Model::tangentLoc, 3, GL_FLOAT,
				   GL_FALSE, 0, &_tangentArray.front ());
	  }
	else
	  {
	    glEnableVertexAttribArrayARB (Md5Model::tangentLoc);
	    glVertexAttribPointerARB (Md5Model::tangentLoc, 3, GL_FLOAT,
				      GL_FALSE, 0, &_tangentArray.front ());
	  }
	break;
      }
    }
}


// --------------------------------------------------------------------------
// Md5Mesh::postRenderVertexArrays
//
// Post-rendering operations.  Render-path dependent cleaning up after
// mesh rendering, like disabling tangent arrays for ARB programs and
// shaders.
// --------------------------------------------------------------------------

void
Md5Mesh::postRenderVertexArrays () const
{
  switch (Md5Model::renderPath)
    {
    case R_normal:
      break;

    case R_ARBfp_diffuse:
    case R_ARBfp_diffuse_specular:
    case R_ARBfp_ds_parallax:
      {
    	  Md5Model::vp->unuse ();
    	  Md5Model::fp->unuse ();

	glDisableVertexAttribArrayARB (TANGENT_LOC);
	break;
      }

    case R_shader:
      {
	if (Md5Model::tangentLoc == -1)
	  break;

	Md5Model::shader->unuse ();

	if (GLEW_VERSION_2_0)
	  glDisableVertexAttribArray (Md5Model::tangentLoc);
	else
	  glDisableVertexAttribArrayARB (Md5Model::tangentLoc);

	break;
      }
    }
}


// --------------------------------------------------------------------------
// Md5Mesh::renderVertexArray
//
// Render mesh with vertex array.
// --------------------------------------------------------------------------

void Md5Mesh::renderVertexArrays () const {
  // Ensable shader/program's stuff
  preRenderVertexArrays ();

  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_NORMAL_ARRAY);
  glEnableClientState (GL_TEXTURE_COORD_ARRAY);

  // Upload mesh data to OpenGL
  glVertexPointer (3, GL_FLOAT, 0, &_vertexArray.front ());
  glNormalPointer (GL_FLOAT, 0, &_normalArray.front ());
  glTexCoordPointer (2, GL_FLOAT, 0, &_texCoordArray.front ());

  // Bind to mesh's textures
  setupTexture(_heightMap, GL_TEXTURE3);
  setupTexture(_normalMap, GL_TEXTURE2);
  setupTexture(_specMap, GL_TEXTURE1);
  setupTexture(_decal, GL_TEXTURE0);

  // Draw the mesh
  glDrawElements (GL_TRIANGLES, _numTris * 3,
	  GL_UNSIGNED_INT, &_vertIndices.front ());

  resetReversedTexture(_heightMap, GL_TEXTURE3);
  resetReversedTexture(_normalMap, GL_TEXTURE2);
  resetReversedTexture(_specMap, GL_TEXTURE1);
  resetReversedTexture(_decal, GL_TEXTURE0);


  glDisableClientState (GL_TEXTURE_COORD_ARRAY);
  glDisableClientState (GL_NORMAL_ARRAY);
  glDisableClientState (GL_VERTEX_ARRAY);

  // Disable shader/program's stuff
  postRenderVertexArrays ();
}


// --------------------------------------------------------------------------
// Md5Mesh::drawNormals
//
// Draw mesh's vertex normals and tangents.
// --------------------------------------------------------------------------

void
Md5Mesh::drawNormals () const
{
  Vector3f thisVertex, thisNormal, thisTangent;

  glPushAttrib (GL_ENABLE_BIT);
  glDisable (GL_LIGHTING);
  glDisable (GL_TEXTURE_2D);

  // Blue
  glColor3f (0.0, 0.0, 1.0);

  // Draw normals
  glBegin (GL_LINES);
    for (int i = 0; i < _numVerts * 3; i += 3)
      {
	thisVertex._x = _vertexArray[i + 0];
	thisVertex._y = _vertexArray[i + 1];
	thisVertex._z = _vertexArray[i + 2];

	thisNormal._x = _normalArray[i + 0];
	thisNormal._y = _normalArray[i + 1];
	thisNormal._z = _normalArray[i + 2];

	Vector3f normVec (thisVertex + thisNormal);

	glVertex3fv (thisVertex);
	glVertex3fv (normVec);
      }
  glEnd ();

  // Magenta
  glColor3f (1.0, 0.0, 1.0);

  // Draw tangents
  glBegin (GL_LINES);
    for (int i = 0; i < _numVerts * 3; i += 3)
      {
	thisVertex._x = _vertexArray[i + 0];
	thisVertex._y = _vertexArray[i + 1];
	thisVertex._z = _vertexArray[i + 2];

	thisTangent._x = _tangentArray[i + 0];
	thisTangent._y = _tangentArray[i + 1];
	thisTangent._z = _tangentArray[i + 2];

	Vector3f tanVec (thisVertex + thisTangent);

	glVertex3fv (thisVertex);
	glVertex3fv (tanVec);
      }
  glEnd ();

  // GL_ENABLE_BIT
  glPopAttrib ();
}


// --------------------------------------------------------------------------
// Md5Mesh::allocVertexArrays
//
// Allocate memory for vertex arrays.  NOTE: we need to have triangle
// data first! (_tris)
// --------------------------------------------------------------------------

void
Md5Mesh::allocVertexArrays ()
{
  _vertexArray.reserve (_numVerts * 3);
  _normalArray.reserve (_numVerts * 3);
  _tangentArray.reserve (_numVerts * 3);
  _texCoordArray.reserve (_numVerts * 2);
  _vertIndices.reserve (_numTris * 3);

  // We can already initialize the vertex index array (we won't have
  // to do it each time we want to draw)
  for (int i = 0; i < _numTris; ++i)
    {
      for (int j = 0; j < 3; ++j)
	_vertIndices.push_back (_tris[i]->index[j]);
    }
}


// --------------------------------------------------------------------------
// Md5Mesh::setupTexCoordArray
//
// Compute texture coordinate array.
// --------------------------------------------------------------------------

void
Md5Mesh::setupTexCoordArray ()
{
  for (int i = 0, j = 0; i < _numVerts; ++i, j += 2)
    {
      // j = i * 2;
      _texCoordArray[j + 0] = _verts[i]->st[0];
      _texCoordArray[j + 1] = _verts[i]->st[1];
    }
}


// --------------------------------------------------------------------------
// Md5Mesh::setupTexture
//
// Setup texture 'tex' for the given texture unit.
// --------------------------------------------------------------------------

void Md5Mesh::setupTexture(const Texture2D *tex, GLenum texUnit) const {
  if(!tex) {
      // Disable texture and return
      glActiveTexture (texUnit);
      glBindTexture (GL_TEXTURE_2D, 0);
  }
  else {
      tex->bind (texUnit);

      // Doom 3 doesn't use the OpenGL standard coordinate system
      // for images, i.e. image data "starts" at the upper-left
      // corner instead of the lower-left corner.

      // We must reverse the t component if the texture
      // has been built with OpenGL's coord. system
      if (tex->stdCoordSystem ()) {
		  glMatrixMode(GL_TEXTURE);
		  glLoadIdentity();
		  glScalef(1.0f, -1.0f, 1.0f);
		  glTranslatef(0.0f, -1.0f, 0.0f);
		  glMatrixMode(GL_MODELVIEW);
      }
  }
}

void Md5Mesh::resetReversedTexture(const Texture2D *tex, GLenum texUnit) const {
  if(tex) {
	  tex->bind(texUnit);

      // Doom 3 doesn't use the OpenGL standard coordinate system
      // for images, i.e. image data "starts" at the upper-left
      // corner instead of the lower-left corner.

      // We must UNDO the reversing of the t component if the texture
      // has been built with OpenGL's coord. system
      if(tex->stdCoordSystem ()) {
		  glMatrixMode(GL_TEXTURE);
		  glLoadIdentity();
		  glTranslatef(0.0f, 0.0f, 0.0f);
		  glScalef(1.0f, 1.0f, 1.0f);
		  glMatrixMode(GL_MODELVIEW);
      }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// class Md5Model implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Md5Model::Md5Model
//
// Constructor.  Load MD5 mesh from file.
// --------------------------------------------------------------------------

Md5Model::Md5Model (const string &filename)
  throw (Md5Exception)
  : _numJoints (0), _numMeshes (0)
{
  // Open file
  std::ifstream ifs (filename.c_str(), std::ios::in);

  if (ifs.fail ())
    throw Md5Exception ("Couldn't open md5model file: " + filename, filename);

  while (!ifs.eof ())
    {
      string token, buffer;
      int version = 0;

      // Read next token
      ifs >> token;

      if (token == "//")
	{
	  // This is the begining of a comment
	  // Eat up rest of the line
	  std::getline (ifs, buffer);
	}
      else if (token == "MD5Version")
	{
	  ifs >> version;

	  if (version != kMd5Version)
	    throw Md5Exception ("Bad ifs version", filename);
	}
      else if (token == "numJoints")
	{
	  ifs >> _numJoints;
	}
      else if (token == "numMeshes")
	{
	  ifs >> _numMeshes;
	  _meshes.reserve (_numMeshes);
	}
      else if (token == "joints")
	{
	  // Base skeleton data
	  ifs >> token; // "{"

	  Md5Skeleton *skel = new Md5Skeleton (ifs, _numJoints);
	  _baseSkeleton = Md5SkeletonPtr (skel);

	  ifs >> token; // "}"
	}
      else if (token == "mesh")
	{
	  ifs >> token; // "{"

	  // Create and load a new model mesh
	  Md5MeshPtr mesh = Md5MeshPtr (new Md5Mesh (ifs));

	  // Compute bounding box in bind-pose
	  mesh->computeBoundingBox (_baseSkeleton.get ());

	  // Compute weight normals
	  mesh->computeWeightNormals (_baseSkeleton.get ());

	  // Compute weight tangents
	  mesh->computeWeightTangents (_baseSkeleton.get ());

	  _meshes.push_back (mesh);
	}
    }

  ifs.close ();

  // Compute the bounding box in bind-pose
  computeBindPoseBoundingBox ();
}


// --------------------------------------------------------------------------
// Md5Model::~Md5Model
//
// Destructor.  Free all data allocated for the model.
// --------------------------------------------------------------------------

Md5Model::~Md5Model ()
{
}


// --------------------------------------------------------------------------
// Md5Model::drawModel
//
// Draw each mesh of the model.
// --------------------------------------------------------------------------

void
Md5Model::drawModel () const
{
  for (int i = 0; i < _numMeshes; ++i)
    {
      if (_meshes[i]->show ())
	{
	  _meshes[i]->renderVertexArrays ();

	  if (Md5Model::bDrawNormals)
	    _meshes[i]->drawNormals ();
	}
    }
}


// --------------------------------------------------------------------------
// Md5Model::prepare
//
// Prepare each mesh of the model for drawing, i.e. compute final vertex
// positions, normals and other vertex's related data.
// --------------------------------------------------------------------------

void Md5Model::prepare (Md5Skeleton *skel) {
  for (int i = 0; i < _numMeshes; ++i) {
      if (!_meshes[i]->hiden ()) {
    	  // Prepare for drawing with interpolated skeleton
    	  _meshes[i]->setupVertexArrays(skel);
      }
  }
}


// --------------------------------------------------------------------------
// Md5Model::addAnim
//
// Load a MD5 animation and add it to the animation list.  Return true if
// the animation has been loaded successfully.
// --------------------------------------------------------------------------

bool
Md5Model::addAnim (const string &filename)
{
  Md5Animation *pAnim = new Md5Animation (filename);

  // Check for compatibility
  if (!validityCheck (pAnim))
    {
      cerr << filename << " isn't a valid animation"
	   << " for this model!" << endl;
      delete pAnim;
      return false;
    }

  const string name (pAnim->name ());

  // If there is already an animation with same name,
  // delete it
  AnimMap::iterator itor = _animList.find (name);
  if (itor != _animList.end())
    _animList.erase (itor);

  // Insert the new animation
  _animList.insert (AnimMap::value_type (name, Md5AnimationPtr (pAnim)));

  return true;
}


// --------------------------------------------------------------------------
// Md5Model::setMeshRenderState
// Md5Model::setMeshDecalMap
// Md5Model::setMeshSpecularMap
// Md5Model::setMeshNormalMap
// Md5Model::setMeshHeightMap
//
// Setup mesh's state or texture.
// --------------------------------------------------------------------------

void
Md5Model::setMeshRenderState (const string &name, int state)
{
  if (Md5Mesh *mesh = getMeshByName (name))
    mesh->setState (state);
}


void
Md5Model::setMeshDecalMap (const string &name, const Texture2D *tex)
{
  if (Md5Mesh *mesh = getMeshByName (name))
    mesh->setDecalMap (tex);
}


void
Md5Model::setMeshSpecularMap (const string &name, const Texture2D *tex)
{
  if (Md5Mesh *mesh = getMeshByName (name))
    mesh->setSpecularMap (tex);
}


void
Md5Model::setMeshNormalMap (const string &name, const Texture2D *tex)
{
  if (Md5Mesh *mesh = getMeshByName (name))
    mesh->setNormalMap (tex);
}


void
Md5Model::setMeshHeightMap (const string &name, const Texture2D *tex)
{
  if (Md5Mesh *mesh = getMeshByName (name))
    mesh->setHeightMap (tex);
}


// --------------------------------------------------------------------------
// Md5Model::anim
//
// Accessor.  Return animation from list.
// --------------------------------------------------------------------------

const Md5Animation*
Md5Model::anim (const string &name) const
{
  AnimMap::const_iterator itor = _animList.find (name);
  if (itor != _animList.end ())
    return itor->second.get ();

  return NULL;
}


// --------------------------------------------------------------------------
// Md5Model::computeBindPoseBoundingBox
//
// Compute model's bounding box in bind-pose.
// --------------------------------------------------------------------------

void Md5Model::computeBindPoseBoundingBox() {
  Vector3f maxValue(-99999.0f, -99999.0f, -99999.0f);
  Vector3f minValue( 99999.0f,  99999.0f,  99999.0f);

  // Get the min and the max from all mesh bounding boxes
  for (int i = 0; i < _numMeshes; ++i) {
      const BoundingBox_t &meshBox = _meshes[i]->boundingBox ();

      if (meshBox.max._x > maxValue._x)
	maxValue._x = meshBox.max._x;

      if (meshBox.min._x < minValue._x)
	minValue._x = meshBox.min._x;

      if (meshBox.max._y > maxValue._y)
	maxValue._y = meshBox.max._y;

      if (meshBox.min._y < minValue._y)
	minValue._y = meshBox.min._y;

      if (meshBox.max._z > maxValue._z)
	maxValue._z = meshBox.max._z;

      if (meshBox.min._z < minValue._z)
	minValue._z = meshBox.min._z;
    }

  _bindPoseBox.min = minValue;
  _bindPoseBox.max = maxValue;
}


// --------------------------------------------------------------------------
// Md5Model::validityCheck
//
// Check if an animation is valid for this model or not.  A valid
// animation must have a skeleton with the same number of joints with
// model's skeleton and for each skeleton joint, name and parent Id must
// match.
// --------------------------------------------------------------------------

bool
Md5Model::validityCheck (Md5Animation *anim) const
{
  if (!anim)
    return false;

  if (_numJoints != anim->frame (0)->numJoints())
    return false;

  for (int i = 0; i < _numJoints; ++i)
    {
      const Md5Joint_t *modelJoint = _baseSkeleton->joint (i);
      const Md5Joint_t *animJoint = anim->frame (0)->joint (i);

      if (modelJoint->name != animJoint->name)
	return false;

      if (modelJoint->parent != animJoint->parent)
	return false;
    }

  return true;
}


// --------------------------------------------------------------------------
// Md5Model::getMeshByName
//
// Get pointer to a mesh given its name.  Return NULL if there is no mesh
// with such a name.
// --------------------------------------------------------------------------

Md5Mesh*
Md5Model::getMeshByName (const string &name) const
{
  for (int i = 0; i < _numMeshes; ++i)
    {
      if (_meshes[i]->name () == name)
	return _meshes[i].get ();
    }

  return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Animation implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Md5Animation::Md5Animation
//
// Constructor.  Load MD5 animation from <.md5anim> file.
// --------------------------------------------------------------------------

Md5Animation::Md5Animation (const string &filename)
  throw (Md5Exception)
  : _numFrames (0), _frameRate (0)
{
  vector<JointInfo> jointInfos;
  vector<BaseFrameJoint> baseFrame;
  vector<float> animFrameData;
  int numJoints = 0;
  int numAnimatedComponents = 0;

  // Open file
  std::ifstream ifs (filename.c_str(), std::ios::in);

  if (ifs.fail())
    throw Md5Exception ("Couldn't open md5anim file: " + filename, filename);

  while (!ifs.eof ())
    {
      string token, buffer;
      int version = 0;;
      int i = 0;

      // Read next token
      ifs >> token;

      if (token == "//")
	{
	  // This is the begining of a comment
	  // Eat up rest of the line
	  std::getline (ifs, buffer);
	}
      else if (token == "MD5Version")
	{
	  ifs >> version;

	  if (version != kMd5Version)
	    throw Md5Exception ("Bad file version", filename);
	}
      else if (token == "numFrames")
	{
	  ifs >> _numFrames;
	  _skelframes.reserve (_numFrames);
	  _bboxes.reserve (_numFrames);
	}
      else if (token == "numJoints")
	{
	  ifs >> numJoints;
	  jointInfos.reserve (numJoints);
	  baseFrame.reserve (numJoints);
	}
      else if (token == "frameRate")
	{
	  ifs >> _frameRate;
	}
      else if (token == "numAnimatedComponents")
	{
	  ifs >> numAnimatedComponents;
	  animFrameData.reserve (numAnimatedComponents);
	}
      else if (token == "hierarchy")
	{
	  // Read all joint infos
	  ifs >> token; // "{"

	  // Read all joint infos
	  for (i = 0; i < numJoints; ++i)
	    {
	      JointInfo jinfo;

	      ifs >> jinfo.name;
	      ifs >> jinfo.parent;
	      ifs >> jinfo.flags.value;
	      ifs >> jinfo.startIndex;

	      jointInfos.push_back (jinfo);

	      // Eat up rest of the line
	      std::getline (ifs, buffer);
	    }

	  ifs >> token; // "}"
	}
      else if (token == "bounds")
	{
	  ifs >> token; // "{"

	  // Read frame bounds
	  for (int i = 0; i < _numFrames; ++i)
	    {
	      BoundingBoxPtr bbox (new BoundingBox_t);

	      ifs >> token; // "("
	      ifs >> bbox->min._x;
	      ifs >> bbox->min._y;
	      ifs >> bbox->min._z;
	      ifs >> token; // ")"
	      ifs >> token; // "("
	      ifs >> bbox->max._x;
	      ifs >> bbox->max._y;
	      ifs >> bbox->max._z;
	      ifs >> token; // ")"

	      _bboxes.push_back (bbox);
	    }

	  ifs >> token; // "}"
	}
      else if (token == "baseframe")
	{
	  // We should have an opening bracket for the baseframe joint list
	  ifs >> token; // "{"

	  // Read baseframe data
	  for (i = 0; i < numJoints; ++i)
	    {
	      BaseFrameJoint bfj;

	      ifs >> token; // "("
	      ifs >> bfj.pos._x;
	      ifs >> bfj.pos._y;
	      ifs >> bfj.pos._z;
	      ifs >> token; // ")"
	      ifs >> token; // "("
	      ifs >> bfj.orient._x;
	      ifs >> bfj.orient._y;
	      ifs >> bfj.orient._z;
	      ifs >> token; // ")"

	      baseFrame.push_back (bfj);

	      // Eat up rest of the line
	      std::getline (ifs, buffer);
	    }

	  ifs >> token; // "}"
	}
      else if (token == "frame")
	{
	  int frameIndex;
	  ifs >> frameIndex;
	  ifs >> token; // "{"

	  animFrameData.clear ();

	  // Read all frame data
	  float afvalue;
	  for (i = 0; i < numAnimatedComponents; ++i)
	    {
	      // NOTE about coding style: beeuuarg *vomit*
	      ifs >> afvalue;
	      animFrameData.push_back (afvalue);
	    }

	  ifs >> token; // "}"

	  // Build skeleton for this frame
	  buildFrameSkeleton (jointInfos, baseFrame, animFrameData);
	}
    }

  ifs.close ();

  // Extract animation's name
  string szfile = filename;
  string::size_type start = szfile.find_last_of ('/');
  string::size_type end = szfile.find_last_of (".md5anim");
  _name = szfile.substr (start + 1, end - start - 8);
}


// --------------------------------------------------------------------------
// Md5Animation::~Md5Animation
//
// Destructor.  Free all data allocated for the animation.
// --------------------------------------------------------------------------

Md5Animation::~Md5Animation ()
{
}


// --------------------------------------------------------------------------
// Md5Animation::buildFrameSkeleton
//
// Build a skeleton for a particular frame.  The skeleton is transformed
// by the given modelview matrix so that it is possible to obtain the
// skeleton in absolute coordinates.
// --------------------------------------------------------------------------

void
Md5Animation::buildFrameSkeleton (vector<JointInfo> &jointInfos,
				  vector<BaseFrameJoint> &baseFrame,
				  vector<float> &animFrameData)
{
  // Allocate memory for this frame
  Md5SkeletonPtr skelframe (new Md5Skeleton);
  _skelframes.push_back (skelframe);

  skelframe->setNumJoints (jointInfos.size ());

  // Setup all joints for this frame
  for (unsigned int i = 0; i < jointInfos.size (); ++i)
    {
      BaseFrameJoint *baseJoint = &baseFrame[i];
      Vector3f animatedPos = baseJoint->pos;
      Quaternionf animatedOrient = baseJoint->orient;
      int j = 0;

      if (jointInfos[i].flags.tx) // Tx
	{
	  animatedPos._x = animFrameData[jointInfos[i].startIndex + j];
	  ++j;
	}

      if (jointInfos[i].flags.ty) // Ty
	{
	  animatedPos._y = animFrameData[jointInfos[i].startIndex + j];
	  ++j;
	}

      if (jointInfos[i].flags.tz) // Tz
	{
	  animatedPos._z = animFrameData[jointInfos[i].startIndex + j];
	  ++j;
	}

      if (jointInfos[i].flags.qx) // Qx
	{
	  animatedOrient._x = animFrameData[jointInfos[i].startIndex + j];
	  ++j;
	}

      if (jointInfos[i].flags.qy) // Qy
	{
	  animatedOrient._y = animFrameData[jointInfos[i].startIndex + j];
	  ++j;
	}

      if (jointInfos[i].flags.qz) // Qz
	{
	  animatedOrient._z = animFrameData[jointInfos[i].startIndex + j];
	  ++j;
	}

      // Compute orient quaternion's w value
      animatedOrient.computeW ();

      // NOTE: we assume that this joint's parent has
      // already been calculated, i.e. joint's ID should
      // never be smaller than its parent ID.
      Md5Joint_t *thisJoint = new Md5Joint_t;
      skelframe->addJoint (thisJoint);

      int parent = jointInfos[i].parent;
      thisJoint->parent = parent;
      thisJoint->name = jointInfos[i].name;

      // has parent?
      if (thisJoint->parent < 0)
	{
	  thisJoint->pos = animatedPos;
	  thisJoint->orient = animatedOrient;
	}
      else
	{
	  const Md5Joint_t *parentJoint = skelframe->joint (parent);

	  parentJoint->orient.rotate (animatedPos);
	  thisJoint->pos = animatedPos + parentJoint->pos;

	  thisJoint->orient = parentJoint->orient * animatedOrient;
	  thisJoint->orient.normalize ();
	}
    }
}


// --------------------------------------------------------------------------
// Md5Animation::interpolate
//
// Build an interpolated skeleton given two frame indexes and an
// interpolation percentage.  'out' must be non-null.
// --------------------------------------------------------------------------

void
Md5Animation::interpolate (int frameA, int frameB,
		   float interp, Md5Skeleton *out) const
{
  for (int i = 0; i < out->numJoints (); ++i)
    {
      const Md5Joint_t *pJointA = _skelframes[frameA]->joint (i);
      const Md5Joint_t *pJointB = _skelframes[frameB]->joint (i);
      Md5Joint_t *pFinalJoint = out->joint (i);

      pFinalJoint->parent = pJointA->parent;
      pFinalJoint->pos = pJointA->pos + interp * (pJointB->pos - pJointA->pos);
      pFinalJoint->orient = Slerp (pJointA->orient, pJointB->orient, interp);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Object implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Md5Object::Md5Object
//
// Constructor.
// --------------------------------------------------------------------------

Md5Object::Md5Object (Md5Model *model)
  : _model (NULL), _animatedSkeleton (NULL), _softwareTransformation (false),
    _currAnim (NULL), _currFrame (0), _nextFrame (1), _last_time (0.0),
    _max_time (0.0), _renderFlags (kDrawModel)
{
  setMd5Model (model);
}


// --------------------------------------------------------------------------
// Md5Object::~Md5Object
//
// Destructor.  Free all data allocated for the object.
// --------------------------------------------------------------------------

Md5Object::~Md5Object ()
{
  delete _animatedSkeleton;
}


// --------------------------------------------------------------------------
// Md5Object::setMd5Model
//
// Attach MD5 Model to Object.
// --------------------------------------------------------------------------

void
Md5Object::setMd5Model (Md5Model *model)
{
  if (_model != model)
    {
      _model = model; // Link to the model

      // Delete previous skeletons because the new
      // model is different and its skeleton can hold
      // more joints.
      delete _animatedSkeleton;

      // Copy skeleton joints name
      _animatedSkeleton = _model->baseSkeleton ()->clone ();

      // Reset animation
      _currAnim = NULL;
      _currAnimName.clear ();
    }
}


// --------------------------------------------------------------------------
// Md5Object::setAnim
//
// Set the current animation to play.
// --------------------------------------------------------------------------

void
Md5Object::setAnim (const string &name)
{
  if (_model)
    {
      // Retrieve animation from model's animation list
      if ((_currAnim = _model->anim (name)))
	{
	  _currAnimName = _currAnim->name ();

	  // Compute max frame time and reset _last_time
	  _max_time =  1.0 / static_cast<double>(_currAnim->frameRate ());
	  _last_time = 0.0;

	  // Reset current and next frames
	  _currFrame = 0;
	  _nextFrame = (_currAnim->maxFrame () > 0) ? 1 : 0;
	}
      else
	{
	  delete _animatedSkeleton;
	  _currAnimName.clear ();

	  // Rebuild animated skeleton with model's base skeleton
	  _animatedSkeleton = _model->baseSkeleton ()->clone ();
	}
    }
}


// --------------------------------------------------------------------------
// Md5Object::animate
//
// Compute current and next frames for model's animation.
// --------------------------------------------------------------------------

void
Md5Object::animate (double dt)
{
  // Animate only if there is an animation...
  if (_currAnim)
    {
      _last_time += dt;

      // Move to next frame?
      if (_last_time >= _max_time)
	{
	  _currFrame++;
	  _nextFrame++;
	  _last_time = 0.0f;

	  unsigned int maxFrame = _currAnim->maxFrame ();

	  if (_currFrame > maxFrame)
	    _currFrame = 0;

	  if (_nextFrame > maxFrame)
	    _nextFrame = 0;
	}
    }
}


// --------------------------------------------------------------------------
// Md5Object::computeBoundingBox
//
// Compute object's oriented bounding box.
// --------------------------------------------------------------------------

void
Md5Object::computeBoundingBox ()
{
  BoundingBox_t bbox;

  if (_currAnim)
    {
      // Interpolate frames' bounding box in order
      // to get animated AABB in object space
      const BoundingBox_t *boxA, *boxB;
      boxA = _currAnim->frameBounds (_currFrame);
      boxB = _currAnim->frameBounds (_nextFrame);

      float interp = _last_time * _currAnim->frameRate ();

      bbox.min = boxA->min + (boxB->min - boxA->min) * interp;
      bbox.max = boxA->max + (boxB->max - boxA->max) * interp;
    }
  else
    {
      // Get bind-pose model's bouding box
      bbox = _model->bindPoseBoundingBox ();
    }

  // Compute oriented bounding box
  _bbox.world = _modelView;
  _bbox.center = Vector3f ((bbox.max._x + bbox.min._x) * 0.5f,
			   (bbox.max._y + bbox.min._y) * 0.5f,
			   (bbox.max._z + bbox.min._z) * 0.5f);
  _bbox.extent = Vector3f (bbox.max._x - _bbox.center._x,
			   bbox.max._y - _bbox.center._y,
			   bbox.max._z - _bbox.center._z);
}


// --------------------------------------------------------------------------
// Md5Object::prepare
//
// Prepare object to be drawn.  Interpolate skeleton frames if the
// object is animated, or copy model's base skeleton to current
// skeleton if not.
// Apply model view transformation if asked.
// --------------------------------------------------------------------------

void Md5Object::prepare(bool softwareTransformation) {
  _softwareTransformation = softwareTransformation;

  if (_renderFlags & kDrawModel)
    {
      if (_currAnim)
	{
	  // Interpolate current and next frame skeletons
	  float interp = _last_time * _currAnim->frameRate ();
	  _currAnim->interpolate (_currFrame, _nextFrame,
				  interp, _animatedSkeleton);
	}
      else
	{
	  // If there is no animated skeleton, fall to
	  // model's base skeleton
	  delete _animatedSkeleton;

	  _animatedSkeleton = _model->baseSkeleton()->clone ();
	}

      if (_softwareTransformation || _renderFlags & kDrawJointLabels)
	{
	  // Force software transformation if joint labels have
	  // to be drawn
	  _softwareTransformation = true;

	  Quaternionf rot;
	  rot.fromMatrix (_modelView);

	  // Applly Model-View transformation for each joint
	  for (int i = 0; i < _animatedSkeleton->numJoints (); ++i)
	    {
	      Md5Joint_t *thisJoint = _animatedSkeleton->joint (i);

	      thisJoint->pos = _modelView * thisJoint->pos;
	      thisJoint->orient = rot * thisJoint->orient;
	    }
	}

      // Setup vertex arrays
      _model->prepare (_animatedSkeleton);
    }
}

// --------------------------------------------------------------------------
// Md5Object::render
//
// Draw the object.
// --------------------------------------------------------------------------

void Md5Object::render () const {
	 glPushMatrix ();
	//glTranslatef( 0.0f, -60.0f, 0.0f );
	//glRotatef( -90.0, 1.0, 0.0, 0.0 );
	//glRotatef( -90.0, 0.0, 0.0, 1.0 );
	//glTranslatef( 0.0f, -60.0f, 0.0f );

	 //!glRotatef( -20.0, 1.0, 0.0, 0.0 );
	 //!glRotatef( -20.0, 0.0, 1.0, 0.0 );

	//glRotatef( 50.0, 1.0, 0.0, 0.0 );
	//glTranslatef( 5.0f, -2.0f, -3.0f );

	 //!glTranslatef(-1.4f, -1.4f, -7.5f);

	 //glRotatef( 90.0, 0.0, 0.0, 1.0 );
	 //glRotatef( -25.0, 0.0, 1.0, 0.0 );

	 //glRotatef( -20.0, 1.0, 0.0, 0.0 );
	 //glScalef(1.0/20.0f, 1.0/20.0f, 1.0f);

	if (!_softwareTransformation) {
		glMultMatrixf (_modelView._m);
	}
	glPushAttrib (GL_POLYGON_BIT | GL_ENABLE_BIT);
	glFrontFace (GL_CW);

	if (_renderFlags & kDrawModel) {
		_model->drawModel ();
	}
	if (_renderFlags & kDrawSkeleton) {
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_LIGHTING);

		_animatedSkeleton->draw (_modelView,_renderFlags & kDrawJointLabels);
	}

	// GL_POLYGON_BIT | GL_ENABLE_BIT
	glPopAttrib ();
	glPopMatrix ();
}

}}} //end namespace
