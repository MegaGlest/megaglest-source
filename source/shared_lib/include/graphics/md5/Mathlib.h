// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Mathlib.h -- Copyright (c) 2005-2006 David Henry
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
// Declarations for 3D maths object and functions to use with OpenGL.
//
// Provide vector, matrix and quaternion operations.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include <cmath>

namespace Shared { namespace Graphics { namespace md5 {

// Forward declarations
template <typename Real> class Vector3;
template <typename Real> class Matrix4x4;
template <typename Real> class Quaternion;

// Type definitions
enum Axis {
  kXaxis, kYaxis, kZaxis
};

// Declare a global constant for pi and a few multiples.
const float kPi = 3.14159265358979323846f;
const float k2Pi = kPi * 2.0f;
const float kPiOver2 = kPi / 2.0f;
const float k1OverPi = 1.0f / kPi;
const float k1Over2Pi = 1.0f / k2Pi;
const float kPiOver180 = kPi / 180.0f;
const float k180OverPi = 180.0f / kPi;

// "Wrap" an angle in range -pi...pi by adding the correct multiple
// of 2 pi
template <typename Real>
Real wrapPi (Real theta);

// "Safe" inverse trig functions
template <typename Real>
Real safeAcos (Real x);

// Set the Euler angle triple to its "canonical" value
template <typename Real>
void canonizeEulerAngles (Real &roll, Real &pitch, Real &yaw);

// Convert between degrees and radians
template <typename Real>
inline Real degToRad (Real deg) { return deg * kPiOver180; }

template <typename Real>
inline Real radToDeg (Real rad) { return rad * k180OverPi; }

// Convert between "field of view" and "zoom".
// The FOV angle is specified in radians.
template <typename Real>
inline Real fovToZoom (Real fov) { return 1.0f / std::tan (fov * 0.5f); }

template <typename Real>
inline Real zoomToFov (Real zoom) { return 2.0f * std::atan (1.0f / zoom); }


/////////////////////////////////////////////////////////////////////////////
//
// class Vector3<Real> - A simple 3D vector class.
//
/////////////////////////////////////////////////////////////////////////////

template <typename Real>
class Vector3
{
public:
  // Constructors
  Vector3 () { }
  Vector3 (Real x, Real y, Real z)
    : _x (x), _y (y), _z (z) { }

public:
  // Vector comparison
  bool operator== (const Vector3<Real> &v) const;
  bool operator!= (const Vector3<Real> &v) const;

  // Vector negation
  Vector3<Real> operator- () const;

  // Vector operations
  Vector3<Real> operator+ (const Vector3<Real> &v) const;
  Vector3<Real> operator- (const Vector3<Real> &v) const;
  Vector3<Real> operator* (Real s) const;
  Vector3<Real> operator/ (Real s) const;

  // Combined assignment operators to conform to
  // C notation convention
  Vector3<Real> &operator+= (const Vector3<Real> &v);
  Vector3<Real> &operator-= (const Vector3<Real> &v);
  Vector3<Real> &operator*= (Real s);
  Vector3<Real> &operator/= (Real s);

  // Accessor.  This allows to use the vector object
  // like an array of Real. For example:
  // Vector3<float> v (...);
  // float f = v[1]; // access to _y
  operator const Real *() { return _v; }

public:
  // Other vector operations
  bool isZero ();
  void normalize ();

public:
  // Member variables
  union
  {
    struct
    {
      Real _x, _y, _z;
    };

    Real _v[3];
  };
};


// Predefined Vector3 types
typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

// We provide a global constant zero vector
static const Vector3f kZeroVectorf (0.0f, 0.0f, 0.0f);
static const Vector3d kZeroVectord (0.0, 0.0, 0.0);


//
// Nonmember Vector3 functions
//

template <typename Real>
Vector3<Real> operator* (Real k, Vector3<Real> v);

template <typename Real>
Real VectorMag (const Vector3<Real> &v);

template <typename Real>
Real DotProduct (const Vector3<Real> &a, const Vector3<Real> &b);

template <typename Real>
Vector3<Real> CrossProduct (const Vector3<Real> &a, const Vector3<Real> &b);

template <typename Real>
Vector3<Real> ComputeNormal (const Vector3<Real> &p1,
     const Vector3<Real> &p2, const Vector3<Real> &p3);

template <typename Real>
Real Distance (const Vector3<Real> &a, const Vector3<Real> &b);

template <typename Real>
Real DistanceSquared (const Vector3<Real> &a, const Vector3<Real> &b);


/////////////////////////////////////////////////////////////////////////////
//
// class Matrix4x4<Real> - Implement a 4x4 Matrix class that can represent
// any 3D affine transformation.
//
/////////////////////////////////////////////////////////////////////////////

template <typename Real>
class Matrix4x4
{
public:
  // Constructor - Initialize the last (never used) row of the matrix
  // so that we can do any operation on matrices on the 3x4 portion
  // and forget that line which will (and should) never change.
  Matrix4x4 ()
    : _h14 (0.0f), _h24 (0.0f), _h34 (0.0f), _tw (1.0f) { }

  // Note that we don't define the copy constructor and let the compiler
  // doing it itself because such initialization is not necessary
  // since the source matrix has its last row already initialized...

public:
  // Public interface
  void identity ();
  void transpose ();
  void invert ();
  void setTranslation (const Vector3<Real> &v);

  void transform (Vector3<Real> &v) const;
  void rotate (Vector3<Real> &v) const;
  void inverseRotate (Vector3<Real> &v) const;
  void inverseTranslate (Vector3<Real> &v) const;

  void fromQuaternion (const Quaternion<Real> &q);

  // Matrix <-> Euler conversions; XYZ rotation order; angles in radians
  void fromEulerAngles (Real x, Real y, Real z);
  void toEulerAngles (Real &x, Real &y, Real &z) const;

  // Return a base vector from the matrix
  Vector3<Real> rightVector () const;
  Vector3<Real> upVector () const;
  Vector3<Real> forwardVector () const;
  Vector3<Real> translationVector () const;

  // Accessor.  This allows to use the matrix object
  // like an array of Real. For example:
  // Matrix4x4<float> mat;
  // float f = mat[4]; // access to _m21
  operator const Real *() { return _m; }

public:
  // Member variables

  // The values of the matrix.  Basically the upper 3x3 portion
  // contains a linear transformation, and the last column is the
  // translation portion. Here data is transposed, see the Mathlib.inl
  // for more details.
  union
  {
    struct
    {
      Real _m11, _m12, _m13, _h14;
      Real _m21, _m22, _m23, _h24;
      Real _m31, _m32, _m33, _h34;
      Real _tx,  _ty,  _tz,  _tw;
    };

    // Access to raw packed matrix data (usefull for
    // glLoadMatrixf () and glMultMatrixf ())
    Real _m[16];
  };
};


// Predefined Matrix4x4 types
typedef Matrix4x4<float> Matrix4x4f;
typedef Matrix4x4<double> Matrix4x4d;


//
// Nonmember Matrix4x4 functions
//

// Matrix concatenation
template <typename Real>
Matrix4x4<Real> operator* (const Matrix4x4<Real> &a, const Matrix4x4<Real> &b);

template <typename Real>
Matrix4x4<Real> &operator*= (Matrix4x4<Real> &a, const Matrix4x4<Real> &b);

// Vector transformation
template <typename Real>
Vector3<Real> operator* (const Matrix4x4<Real> &m, const Vector3<Real> &p);

// Transpose matrix
template <typename Real>
Matrix4x4<Real> Transpose (const Matrix4x4<Real> &m);

// Invert matrix
template <typename Real>
Matrix4x4<Real> Invert (const Matrix4x4<Real> &m);

//
// Matrix-builder functions
//

template <typename Real> Matrix4x4<Real> RotationMatrix (Axis axis, Real theta);
template <typename Real> Matrix4x4<Real> RotationMatrix (const Vector3<Real> &axis, Real theta);
template <typename Real> Matrix4x4<Real> TranslationMatrix (Real x, Real y, Real z);
template <typename Real> Matrix4x4<Real> TranslationMatrix (const Vector3<Real> &v);
template <typename Real> Matrix4x4<Real> ScaleMatrix (const Vector3<Real> &s);
template <typename Real> Matrix4x4<Real> ScaleAlongAxisMatrix (const Vector3<Real> &axis, Real k);
template <typename Real> Matrix4x4<Real> ShearMatrix (Axis axis, Real s, Real t);
template <typename Real> Matrix4x4<Real> ProjectionMatrix (const Vector3<Real> &n);
template <typename Real> Matrix4x4<Real> ReflectionMatrix (Axis axis, Real k);
template <typename Real> Matrix4x4<Real> AxisReflectionMatrix (const Vector3<Real> &n);

template <typename Real>
Matrix4x4<Real> LookAtMatrix (const Vector3<Real> &camPos,
	const Vector3<Real> &target, const Vector3<Real> &camUp);
template <typename Real>
Matrix4x4<Real> FrustumMatrix (Real l, Real r, Real b, Real t, Real n, Real f);
template <typename Real>
Matrix4x4<Real> PerspectiveMatrix (Real fovY, Real aspect, Real n, Real f);
template <typename Real>
Matrix4x4<Real> OrthoMatrix (Real l, Real r, Real b, Real t, Real n, Real f);
template <typename Real>
Matrix4x4<Real> OrthoNormalMatrix (const Vector3<Real> &xdir,
	const Vector3<Real> &ydir, const Vector3<Real> &zdir);


/////////////////////////////////////////////////////////////////////////////
//
// class Quaternion<Real> - Implement a quaternion, for purposes of
// representing an angular displacement (orientation) in 3D.
//
/////////////////////////////////////////////////////////////////////////////

template <typename Real>
class Quaternion
{
public:
  // Constructors
  Quaternion () { }
  Quaternion (Real w, Real x, Real y, Real z)
    : _w (w), _x (x), _y (y), _z (z) { }

public:
  // Public interface
  void identity ();
  void normalize ();
  void computeW ();
  void rotate (Vector3<Real> &v) const;

  void fromMatrix (const Matrix4x4<Real> &m);

  // Quaternion <-> Euler conversions; XYZ rotation order; angles in radians
  void fromEulerAngles (Real x, Real y, Real z);
  void toEulerAngles (Real &x, Real &y, Real &z) const;

  Real rotationAngle () const;
  Vector3<Real> rotationAxis () const;

  // Quaternion operations
  Quaternion<Real> operator+ (const Quaternion<Real> &q) const;
  Quaternion<Real> &operator+= (const Quaternion<Real> &q);

  Quaternion<Real> operator- (const Quaternion<Real> &q) const;
  Quaternion<Real> &operator-= (const Quaternion<Real> &q);

  Quaternion<Real> operator* (const Quaternion<Real> &q) const;
  Quaternion<Real> &operator*= (const Quaternion<Real> &q);

  Quaternion<Real> operator* (Real k) const;
  Quaternion<Real> &operator*= (Real k);

  Quaternion<Real> operator* (const Vector3<Real> &v) const;
  Quaternion<Real> &operator*= (const Vector3<Real> &v);

  Quaternion<Real> operator/ (Real k) const;
  Quaternion<Real> &operator/= (Real k);

  Quaternion<Real> operator~ () const; // Quaternion conjugate
  Quaternion<Real> operator- () const; // Quaternion negation

public:
  // Member variables

  // The 4 values of the quaternion.  Normally, it will not
  // be necessary to manipulate these directly.  However,
  // we leave them public, since prohibiting direct access
  // makes some operations, such as file I/O, unnecessarily
  // complicated.

  union
  {
    struct
    {
      Real _w, _x, _y, _z;
    };

    Real _q[4];
  };
};


// Predefined Quaternion types
typedef Quaternion<float> Quaternionf;
typedef Quaternion<double> Quaterniond;

// A global "identity" quaternion constant
static const Quaternionf kQuaternionIdentityf (1.0f, 0.0f, 0.0f, 0.0f);
static const Quaterniond kQuaternionIdentityd (1.0f, 0.0f, 0.0f, 0.0f);


//
// Nonmember Matrix4x functions
//

template <typename Real>
Quaternion<Real> operator* (Real k, const Quaternion<Real> &q);

template <typename Real>
Real DotProduct (const Quaternion<Real> &a, const Quaternion<Real> &b);

template <typename Real>
Quaternion<Real> Conjugate (const Quaternion<Real> &q);

template <typename Real>
Quaternion<Real> Inverse (const Quaternion<Real> &q);

template <typename Real>
Quaternion<Real> RotationQuaternion (Axis axis, Real theta);

template <typename Real>
Quaternion<Real> RotationQuaternion (const Vector3<Real> &axis, Real theta);

template <typename Real>
Quaternion<Real> Log (const Quaternion<Real> &q);
template <typename Real>
Quaternion<Real> Exp (const Quaternion<Real> &q);
template <typename Real>
Quaternion<Real> Pow (const Quaternion<Real> &q, Real exponent);

template <typename Real>
Quaternion<Real> Slerp (const Quaternion<Real> &q0, const Quaternion<Real> &q1, Real t);
template <typename Real>
Quaternion<Real> Squad (const Quaternion<Real> &q0, const Quaternion<Real> &qa,
			const Quaternion<Real> &qb, const Quaternion<Real> &q1, Real t);
template <typename Real>
inline void Intermediate (const Quaternion<Real> &qprev, const Quaternion<Real> &qcurr,
			  const Quaternion<Real> &qnext, Quaternion<Real> &qa,
			  Quaternion<Real> &qb);

// Include inline function definitions
#include "Mathlib.inl"

}}} //end namespace

#endif // __MATHLIB_H__
