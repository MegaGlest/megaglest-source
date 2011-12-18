// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Mathlib.inl -- Copyright (c) 2005-2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda//
// -------------------------------------------------------------------
// Portions Copyright (c) Dante Treglia II and Mark A. DeLoura, 2000.
// Portions Copyright (c) Fletcher Dunn and Ian Parberry, 2002.
// -------------------------------------------------------------------
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
// Implementation of a math library to use with OpenGL.
//
// Provide vector, matrix and quaternion operations.
//
/////////////////////////////////////////////////////////////////////////////

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
//
// Global functions
//
/////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------------------------
// wrapPi
//
// "Wrap" an angle in range -pi...pi by adding the correct multiple
// of 2 pi
// --------------------------------------------------------------------------

template <typename Real>
inline Real
wrapPi (Real theta)
{
  theta += kPi;
  theta -= std::floor (theta * k1Over2Pi) * k2Pi;
  theta -= kPi;
  return theta;
}


// --------------------------------------------------------------------------
// safeAcos
//
// Same as acos(x), but if x is out of range, it is "clamped" to the nearest
// valid value. The value returned is in range 0...pi, the same as the
// standard C acos() function
// --------------------------------------------------------------------------

template <typename Real>
inline Real
safeAcos (Real x)
{
  // Check limit conditions
  if (x <= -1.0)
    return kPi;

  if (x >= 1.0)
    return 0.0;

  // value is in the domain - use standard C function
  return std::acos (x);
}


// --------------------------------------------------------------------------
// canonizeEulerAngles
//
// Set the Euler angle triple to its "canonical" value.  This does not change
// the meaning of the Euler angles as a representation of Orientation in 3D,
// but if the angles are for other purposes such as angular velocities, etc,
// then the operation might not be valid.
// --------------------------------------------------------------------------

template <typename Real>
inline void
canonizeEulerAngles (Real &roll, Real &pitch, Real &yaw)
{
  // First, wrap pitch in range -pi ... pi
  pitch = wrapPi (pitch);

  // Now, check for "the back side" of the matrix, pitch outside
  // the canonical range of -pi/2 ... pi/2
  if (pitch < -kPiOver2)
    {
      roll += kPi;
      pitch = -kPi - pitch;
      yaw += kPi;
    }
  else if (pitch > kPiOver2)
    {
      roll += kPi;
      pitch = kPi - pitch;
      yaw += kPi;
    }

  // OK, now check for the Gimbal lock case (within a slight
  // tolerance)

  if (std::fabs (pitch) > kPiOver2 - 1e-4)
    {
      // We are in gimbal lock.  Assign all rotation
      // about the vertical axis to heading
      yaw += roll;
      roll = 0.0;
    }
  else
    {
      // Not in gimbal lock.  Wrap the bank angle in
      // canonical range
      roll = wrapPi (roll);
    }

  // Wrap heading in canonical range
  yaw = wrapPi (yaw);
}


/////////////////////////////////////////////////////////////////////////////
//
// class Vector3<Real> implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Vector3::isZero
//
// Return true if is zero vector.
// --------------------------------------------------------------------------

template <typename Real>
inline bool
Vector3<Real>::isZero ()
{
  return (_x == 0.0) && (_y == 0.0) && (_z == 0.0);
}


// --------------------------------------------------------------------------
// Vector3::normalize
//
// Set vector length to 1.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Vector3<Real>::normalize()
{
  Real magSq = (_x * _x) + (_y * _y) + (_z * _z);

  if (magSq > 0.0)
    {
      // check for divide-by-zero
      Real oneOverMag = 1.0 / std::sqrt (magSq);
      _x *= oneOverMag;
      _y *= oneOverMag;
      _z *= oneOverMag;
    }
}


// --------------------------------------------------------------------------
// Vector3 operators
//
// Operator overloading for basic vector operations.
// --------------------------------------------------------------------------

template <typename Real>
inline bool
Vector3<Real>::operator== (const Vector3<Real> &v) const
{
  return ((_x == v._x) && (_y == v._y) && (_z == v._z));
}

template <typename Real>
inline bool
Vector3<Real>::operator!= (const Vector3<Real> &v) const
{
  return ((_x != v._x) || (_y != v._y) || (_z != v._z));
}

template <typename Real>
inline Vector3<Real>
Vector3<Real>::operator- () const
{
  return Vector3<Real> (-_x, -_y, -_z);
}

template <typename Real>
inline Vector3<Real>
Vector3<Real>::operator+ (const Vector3<Real> &v) const
{
  return Vector3<Real> (_x + v._x, _y + v._y, _z + v._z);
}

template <typename Real>
inline Vector3<Real>
Vector3<Real>::operator- (const Vector3<Real> &v) const
{
  return Vector3<Real> (_x - v._x, _y - v._y, _z - v._z);
}

template <typename Real>
inline Vector3<Real>
Vector3<Real>::operator* (Real s) const
{
  return Vector3<Real> (_x * s, _y * s, _z * s);
}

template <typename Real>
inline Vector3<Real>
Vector3<Real>::operator/ (Real s) const
{
  Real oneOverS = 1.0 / s; // Note: no check for divide by zero
  return Vector3<Real> (_x * oneOverS, _y * oneOverS, _z * oneOverS);
}

template <typename Real>
inline Vector3<Real> &
Vector3<Real>::operator+= (const Vector3<Real> &v)
{
  _x += v._x; _y += v._y; _z += v._z;
  return *this;
}

template <typename Real>
inline Vector3<Real> &
Vector3<Real>::operator-= (const Vector3<Real> &v)
{
  _x -= v._x; _y -= v._y; _z -= v._z;
  return *this;
}

template <typename Real>
inline Vector3<Real> &
Vector3<Real>::operator*= (Real s)
{
  _x *= s; _y *= s; _z *= s;
  return *this;
}

template <typename Real>
inline Vector3<Real> &
Vector3<Real>::operator/= (Real s)
{
  Real oneOverS = 1.0 / s; // Note: no check for divide by zero!
  _x *= oneOverS; _y *= oneOverS ; _z *= oneOverS;
  return *this;
}


// --------------------------------------------------------------------------
//
// Nonmember Vector3<Real> functions
//
// --------------------------------------------------------------------------

// Scalar on the left multiplication, for symmetry
template <typename Real>
inline Vector3<Real>
operator* (Real k, Vector3<Real> v)
{
  return Vector3<Real> (k * v._x, k * v._y, k * v._z);
}

// Compute vector lenght
template <typename Real>
inline Real
VectorMag (const Vector3<Real> &v)
{
  return std::sqrt ((v._x * v._x) +  (v._y * v._y) +  (v._z * v._z));
}

// Vector3 dot product
template <typename Real>
inline Real
DotProduct (const Vector3<Real> &a, const Vector3<Real> &b)
{
  return ((a._x * b._x) +  (a._y * b._y) +  (a._z * b._z));
}

// Vector3 cross product
template <typename Real>
inline Vector3<Real>
CrossProduct (const Vector3<Real> &a, const Vector3<Real> &b)
{
  return Vector3<Real> (
	(a._y * b._z) - (a._z * b._y),
	(a._z * b._x) - (a._x * b._z),
	(a._x * b._y) - (a._y * b._x)
   );
}

// Compute normal plane given three points
template <typename Real>
inline Vector3<Real>
ComputeNormal (const Vector3<Real> &p1, const Vector3<Real> &p2, const Vector3<Real> &p3)
{
  Vector3<Real> vec1 (p1 - p2);
  Vector3<Real> vec2 (p1 - p3);

  Vector3<Real> result (CrossProduct (vec1, vec2));
  result.normalize ();

  return result;
}

// Compute distance between two points
template <typename Real>
inline Real
Distance (const Vector3<Real> &a, const Vector3<Real> &b)
{
  Real dx = a._x - b._x;
  Real dy = a._y - b._y;
  Real dz = a._z - b._z;
  return std::sqrt ((dx * dx) + (dy * dy) + (dz * dz));
}

// Compute squared distance between two points.
// Useful when comparing distances, since we don't need
// to square the result.
template <typename Real>
inline Real
DistanceSquared (const Vector3<Real> &a, const Vector3<Real> &b)
{
  Real dx = a._x - b._x;
  Real dy = a._y - b._y;
  Real dz = a._z - b._z;
  return ((dx * dx) + (dy * dy) + (dz * dz));
}


/////////////////////////////////////////////////////////////////////////////
//
// class Matrix4x4<Real> implementation.
//
// --------------------------------------------------------------------------
//
// MATRIX ORGANIZATION
//
// The purpose of this class is so that a user might perform transformations
// without fiddling with plus or minus signs or transposing the matrix
// until the output "looks right".  But of course, the specifics of the
// internal representation is important.  Not only for the implementation
// in this file to be correct, but occasionally direct access to the
// matrix variables is necessary, or beneficial for optimization.  Thus,
// we document our matrix conventions here.
//
// Strict adherance to linear algebra rules dictates that the
// multiplication of a 4x4 matrix by a 3D vector is actually undefined.
// To circumvent this, we can consider the input and output vectors as
// having an assumed fourth coordinate of 1.  Also, since the rightmost
// column is [ 0 0 0 1 ], we can simplificate calculations ignoring
// this last column. This is shown below:
//
//         | m11 m12 m13 0 | | x |   | x'|
//         | m21 m22 m23 0 | | y | = | y'|
//         | m31 m32 m33 0 | | z |   | z'|
//         | tx  ty  tz  1 | | 1 |   | 1 |
//
// We use row vectors to represent the right, up and forward vectors
// in the 4x4 matrix.  OpenGL uses column vectors, but the elements of
// an OpenGL matrix are ordered in columns so that m[i][j] is in row j
// and column i.  This is the reverse of the standard C convention in
// which m[i][j] is in row i and column j.  The matrix should be
// transposed before being sent to OpenGL.
//
//   | m11 m21 m31 tx |    |  m0  m4  m8 m12 |    |  m0  m1  m2  m3 |
//   | m12 m22 m32 ty |    |  m1  m5  m9 m13 |    |  m4  m5  m6  m7 |
//   | m13 m23 m33 tz |    |  m2  m6 m10 m14 |    |  m8  m9 m10 m11 |
//   |  0   0   0  tw |    |  m3  m7 m11 m15 |    | m12 m13 m14 m15 |
//
//      OpenGL style          OpenGL matrix            standard C
//                             arrangement             convention
//
// Fortunately, accessing to the raw matrix data via the _m[] array gives
// us the transpose matrix; i.e. in OpenGL form, so that we can directly use
// it with glLoadMatrix{fd}() or glMultMatrix{fd}().
//
// Also, since the rightmost column (in standard C form) should always
// be [ 0 0 0 1 ], and sice these values (_h14, _h24, _h34 and _tw) are
// initialized in constructors, we don't need to modify them in our
// matrix operations, so we don't perform useless calculations...
//
// The right-hand rule is used to define "positive" rotation.
//
//               +y                          +x'
//                |                           |
//                |                           |
//                |______ +x        +y' ______|
//               /                            /
//              /                            /
//             +z                          +z'
//
//          initial position      Positive rotation of
//                                 pi/2 around z-axis
//
/////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------------
// Matrix4x4::identity
//
// Set matrix to identity.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::identity ()
{
  _m11 = 1.0; _m21 = 0.0; _m31 = 0.0; _tx = 0.0;
  _m12 = 0.0; _m22 = 1.0; _m32 = 0.0; _ty = 0.0;
  _m13 = 0.0; _m23 = 0.0; _m33 = 1.0; _tz = 0.0;
  _h14 = 0.0; _h24 = 0.0; _h34 = 0.0; _tw = 1.0;
}


// --------------------------------------------------------------------------
// Matrix4x4::transpose
//
// Transpose the current matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::transpose ()
{
  *this = Transpose (*this);
}


// --------------------------------------------------------------------------
// Matrix4x4::invert
//
// Invert the current matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::invert ()
{
  *this = Invert (*this);
}


// --------------------------------------------------------------------------
// Matrix4x4::setTranslation
//
// Set the translation portion of the matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::setTranslation (const Vector3<Real> &v)
{
  _tx = v._x; _ty = v._y; _tz = v._z;
}


// --------------------------------------------------------------------------
// Matrix4x4::transform
//
// Transform a point by the matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::transform (Vector3<Real> &v) const
{
  // Grind through the linear algebra.
  v = Vector3<Real> (
	(v._x * _m11) + (v._y * _m21) + (v._z * _m31) + _tx,
	(v._x * _m12) + (v._y * _m22) + (v._z * _m32) + _ty,
	(v._x * _m13) + (v._y * _m23) + (v._z * _m33) + _tz
   );
}


// --------------------------------------------------------------------------
// Matrix4x4::rotate
//
// Rotate a point by the 3x3 upper left portion of the matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::rotate (Vector3<Real> &v) const
{
  v = Vector3<Real> (
	(v._x * _m11) + (v._y * _m21) + (v._z * _m31),
	(v._x * _m12) + (v._y * _m22) + (v._z * _m32),
	(v._x * _m13) + (v._y * _m23) + (v._z * _m33)
   );
}


// --------------------------------------------------------------------------
// Matrix4x4::inverseRotate
//
// Rotate a point by the inverse 3x3 upper left portion of the matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::inverseRotate (Vector3<Real> &v) const
{
  v = Vector3<Real> (
	(v._x * _m11) + (v._y * _m12) + (v._z * _m13),
	(v._x * _m21) + (v._y * _m22) + (v._z * _m23),
	(v._x * _m31) + (v._y * _m32) + (v._z * _m33)
   );
}


// --------------------------------------------------------------------------
// Matrix4x4::inverseRotate
//
// Translate a point by the inverse matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::inverseTranslate (Vector3<Real> &v) const
{
  v._x -= _tx;
  v._y -= _ty;
  v._z -= _tz;
}


// --------------------------------------------------------------------------
// Matrix4x4::fromQuaternion
//
// Convert a quaternion to a matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::fromQuaternion (const Quaternion<Real> &q)
{
  // Compute a few values to optimize common subexpressions
  Real ww = 2.0 * q._w;
  Real xx = 2.0 * q._x;
  Real yy = 2.0 * q._y;
  Real zz = 2.0 * q._z;

  // Set the matrix elements.  There is still a little more
  // opportunity for optimization due to the many common
  // subexpressions.  We'll let the compiler handle that...
  _m11 = 1.0 - (yy * q._y) - (zz * q._z);
  _m12 = (xx * q._y) + (ww * q._z);
  _m13 = (xx * q._z) - (ww * q._y);

  _m21 = (xx * q._y) - (ww * q._z);
  _m22 = 1.0 - (xx * q._x) - (zz * q._z);
  _m23 = (yy * q._z) + (ww * q._x);

  _m31 = (xx * q._z) + (ww * q._y);
  _m32 = (yy * q._z) - (ww * q._x);
  _m33 = 1.0 - (xx * q._x) - (yy * q._y);

  // Reset the translation portion
  _tx = _ty = _tz = 0.0;
}


// --------------------------------------------------------------------------
// Matrix4x4::fromEulerAngles
//
// Setup a rotation matrix, given three X-Y-Z rotation angles. The
// rotations are performed first on x-axis, then y-axis and finaly z-axis.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::fromEulerAngles (Real x, Real y, Real z)
{
  // Fetch sine and cosine of angles
  Real cx = std::cos (x);
  Real sx = std::sin (x);
  Real cy = std::cos (y);
  Real sy = std::sin (y);
  Real cz = std::cos (z);
  Real sz = std::sin (z);

  Real sxsy = sx * sy;
  Real cxsy = cx * sy;

  // Fill in the matrix elements
  _m11 =  (cy * cz);
  _m12 =  (sxsy * cz) + (cx * sz);
  _m13 = -(cxsy * cz) + (sx * sz);

  _m21 = -(cy * sz);
  _m22 = -(sxsy * sz) + (cx * cz);
  _m23 =  (cxsy * sz) + (sx * cz);

  _m31 =  (sy);
  _m32 = -(sx * cy);
  _m33 =  (cx * cy);

  // Reset the translation portion
  _tx = _ty = _tz = 0.0;
}


// --------------------------------------------------------------------------
// Matrix4x4::toEulerAngles
//
// Setup the euler angles in radians, given a rotation matrix. The rotation
// matrix could have been obtained from euler angles given the expression:
//   M = X.Y.Z
// where X, Y and Z are rotation matrices about X, Y and Z axes.
// This is the "opposite" of the fromEulerAngles function.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Matrix4x4<Real>::toEulerAngles (Real &x, Real &y, Real &z) const
{
  // Compute Y-axis angle
  y = std::asin (_m31);

  // Compute cos and one over cos for optimization
  Real cy = std::cos (y);
  Real oneOverCosY = 1.0 / cy;

  if (std::fabs (cy) > 0.001)
    {
      // No gimball lock
      x = std::atan2 (-_m32 * oneOverCosY, _m33 * oneOverCosY);
      z = std::atan2 (-_m21 * oneOverCosY, _m11 * oneOverCosY);
    }
  else
    {
      // Gimbal lock case
      x = 0.0;
      z = std::atan2 (_m12, _m22);
    }
}


// --------------------------------------------------------------------------
// Matrix4x4::rightVector
// Matrix4x4::upVector
// Matrix4x4::forwardVector
// Matrix4x4::translationVector
//
// Return a base vector from the matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline Vector3<Real>
Matrix4x4<Real>::rightVector () const
{
  return Vector3<Real> (_m11, _m12, _m13);
}

template <typename Real>
inline Vector3<Real>
Matrix4x4<Real>::upVector () const
{
  return Vector3<Real> (_m21, _m22, _m23);
}

template <typename Real>
inline Vector3<Real>
Matrix4x4<Real>::forwardVector () const
{
  return Vector3<Real> (_m31, _m32, _m33);
}

template <typename Real>
inline Vector3<Real>
Matrix4x4<Real>::translationVector () const
{
  return Vector3<Real> (_tx, _ty, _tz);
}


// --------------------------------------------------------------------------
//
// Nonmember Matrix4x4<Real> functions
//
// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
// Matrix4x4 * Matrix4x4
//
// Matrix concatenation.
//
// We also provide a *= operator, as per C convention.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
operator* (const Matrix4x4<Real> &a, const Matrix4x4<Real> &b)
{
  Matrix4x4<Real> res;

  // Compute the left 4x3 (linear transformation) portion
  res._m11 = (a._m11 * b._m11) + (a._m21 * b._m12) + (a._m31 * b._m13);
  res._m12 = (a._m12 * b._m11) + (a._m22 * b._m12) + (a._m32 * b._m13);
  res._m13 = (a._m13 * b._m11) + (a._m23 * b._m12) + (a._m33 * b._m13);

  res._m21 = (a._m11 * b._m21) + (a._m21 * b._m22) + (a._m31 * b._m23);
  res._m22 = (a._m12 * b._m21) + (a._m22 * b._m22) + (a._m32 * b._m23);
  res._m23 = (a._m13 * b._m21) + (a._m23 * b._m22) + (a._m33 * b._m23);

  res._m31 = (a._m11 * b._m31) + (a._m21 * b._m32) + (a._m31 * b._m33);
  res._m32 = (a._m12 * b._m31) + (a._m22 * b._m32) + (a._m32 * b._m33);
  res._m33 = (a._m13 * b._m31) + (a._m23 * b._m32) + (a._m33 * b._m33);

  // Compute the translation portion
  res._tx = (a._m11 * b._tx) + (a._m21 * b._ty) + (a._m31 * b._tz) + a._tx;
  res._ty = (a._m12 * b._tx) + (a._m22 * b._ty) + (a._m32 * b._tz) + a._ty;
  res._tz = (a._m13 * b._tx) + (a._m23 * b._ty) + (a._m33 * b._tz) + a._tz;

  return res;
}

template <typename Real>
inline Matrix4x4<Real> &
operator*= (Matrix4x4<Real> &a, const Matrix4x4<Real> &b)
{
  a = a * b;
  return a;
}


// --------------------------------------------------------------------------
// Matrix4x4 * Vector3
//
// Transform a point by a matrix.  This makes using the vector class look
// like it does with linear algebra notation on paper.
// --------------------------------------------------------------------------

template <typename Real>
inline Vector3<Real>
operator* (const Matrix4x4<Real> &m, const Vector3<Real> &p)
{
  Vector3<Real> res (p);
  m.transform (res);
  return res;
}


// --------------------------------------------------------------------------
// Transpose
//
// Return the transpose matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
Transpose (const Matrix4x4<Real> &m)
{
  Matrix4x4<Real> res;

  res._m11 = m._m11; res._m21 = m._m12; res._m31 = m._m13; res._tx = m._h14;
  res._m12 = m._m21; res._m22 = m._m22; res._m32 = m._m23; res._ty = m._h24;
  res._m13 = m._m31; res._m23 = m._m32; res._m33 = m._m33; res._tz = m._h34;
  res._h14 = m._tx;  res._h24 = m._ty;  res._h34 = m._tz;  res._tw = m._tw;

  return res;
}


// --------------------------------------------------------------------------
// Determinant
//
// Compute the determinant of the 3x3 portion of the matrix.
// --------------------------------------------------------------------------

template <typename Real>
inline static Real
Determinant3x3 (const Matrix4x4<Real> &m)
{
  return m._m11 * ((m._m22 * m._m33) - (m._m23 * m._m32))
    + m._m12 * ((m._m23 * m._m31) - (m._m21 * m._m33))
    + m._m13 * ((m._m21 * m._m32) - (m._m22 * m._m31));
}


// --------------------------------------------------------------------------
// Invert
//
// Compute the inverse of a matrix.  We use the classical adjoint divided
// by the determinant method.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
Invert (const Matrix4x4<Real> &m)
{
  // Compute the determinant of the 3x3 portion
  Real det = Determinant3x3 (m);

  // If we're singular, then the determinant is zero and there's
  // no inverse
  assert (std::fabs (det) > 0.000001);

  // Compute one over the determinant, so we divide once and
  // can *multiply* per element
  Real oneOverDet = 1.0 / det;

  // Compute the 3x3 portion of the inverse, by
  // dividing the adjoint by the determinant
  Matrix4x4<Real> res;

  res._m11 = ((m._m22 * m._m33) - (m._m23 * m._m32)) * oneOverDet;
  res._m12 = ((m._m13 * m._m32) - (m._m12 * m._m33)) * oneOverDet;
  res._m13 = ((m._m12 * m._m23) - (m._m13 * m._m22)) * oneOverDet;

  res._m21 = ((m._m23 * m._m31) - (m._m21 * m._m33)) * oneOverDet;
  res._m22 = ((m._m11 * m._m33) - (m._m13 * m._m31)) * oneOverDet;
  res._m23 = ((m._m13 * m._m21) - (m._m11 * m._m23)) * oneOverDet;

  res._m31 = ((m._m21 * m._m32) - (m._m22 * m._m31)) * oneOverDet;
  res._m32 = ((m._m12 * m._m31) - (m._m11 * m._m32)) * oneOverDet;
  res._m33 = ((m._m11 * m._m22) - (m._m12 * m._m21)) * oneOverDet;

  // Compute the translation portion of the inverse
  res._tx = -((m._tx * res._m11) + (m._ty * res._m21) + (m._tz * res._m31));
  res._ty = -((m._tx * res._m12) + (m._ty * res._m22) + (m._tz * res._m32));
  res._tz = -((m._tx * res._m13) + (m._ty * res._m23) + (m._tz * res._m33));

  // Return it.
  return res;
}


// --------------------------------------------------------------------------
// RotationMatrix
//
// Setup the matrix to perform a rotation about one of the three cardinal
// X-Y-Z axes.
//
// The axis of rotation is specified by the 1-based "axis" index.
//
// theta is the amount of rotation, in radians.  The right-hand rule is
// used to define "positive" rotation.
//
// The translation portion is reset.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
RotationMatrix (Axis axis, Real theta)
{
  Matrix4x4<Real> res;

  // Get sin and cosine of rotation angle
  Real s = std::sin (theta);
  Real c = std::cos (theta);

  // Check which axis they are rotating about
  switch (axis)
    {
    case kXaxis: // Rotate about the x-axis
      res._m11 = 1.0; res._m21 = 0.0; res._m31 = 0.0;
      res._m12 = 0.0; res._m22 = c;   res._m32 = -s;
      res._m13 = 0.0; res._m23 = s;   res._m33 =  c;
      break;

    case kYaxis: // Rotate about the y-axis
      res._m11 = c;   res._m21 = 0.0; res._m31 = s;
      res._m12 = 0.0; res._m22 = 1.0; res._m32 = 0.0;
      res._m13 = -s;  res._m23 = 0.0; res._m33 = c;
      break;

    case kZaxis: // Rotate about the z-axis
      res._m11 = c;   res._m21 = -s;  res._m31 = 0.0;
      res._m12 = s;   res._m22 =  c;  res._m32 = 0.0;
      res._m13 = 0.0; res._m23 = 0.0; res._m33 = 1.0;
      break;

    default:
      // bogus axis index
      assert (false);
    }

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.0;

  return res;
}


//---------------------------------------------------------------------------
// AxisRotationMatrix
//
// Setup the matrix to perform a rotation about an arbitrary axis.
// The axis of rotation must pass through the origin.
//
// axis defines the axis of rotation, and must be a unit vector.
//
// theta is the amount of rotation, in radians.  The right-hand rule is
// used to define "positive" rotation.
//
// The translation portion is reset.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
RotationMatrix (const Vector3<Real> &axis, Real theta)
{
  Matrix4x4<Real> res;

  // Quick sanity check to make sure they passed in a unit vector
  // to specify the axis
  assert (std::fabs (DotProduct (axis, axis) - 1.0) < 0.001);

  // Get sin and cosine of rotation angle
  Real s = std::sin (theta);
  Real c = std::cos (theta);

  // Compute 1 - cos(theta) and some common subexpressions
  Real a = 1.0 - c;
  Real ax = a * axis._x;
  Real ay = a * axis._y;
  Real az = a * axis._z;

  // Set the matrix elements.  There is still a little more
  // opportunity for optimization due to the many common
  // subexpressions.  We'll let the compiler handle that...
  res._m11 = (ax * axis._x) + c;
  res._m12 = (ax * axis._y) + (axis._z * s);
  res._m13 = (ax * axis._z) - (axis._y * s);

  res._m21 = (ay * axis._x) - (axis._z * s);
  res._m22 = (ay * axis._y) + c;
  res._m23 = (ay * axis._z) + (axis._x * s);

  res._m31 = (az * axis._x) + (axis._y * s);
  res._m32 = (az * axis._y) - (axis._x * s);
  res._m33 = (az * axis._z) + c;

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.0;

  return res;
}


// --------------------------------------------------------------------------
// TranslationMatrix
//
// Build a translation matrix given a translation vector.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
TranslationMatrix (Real x, Real y, Real z)
{
  return TranslationMatrix (Vector3<Real> (x, y, z));
}

template <typename Real>
inline Matrix4x4<Real>
TranslationMatrix (const Vector3<Real> &v)
{
  Matrix4x4<Real> res;

  res._m11 = 1.0; res._m21 = 0.0; res._m31 = 0.0; res._tx = v._x;
  res._m12 = 0.0; res._m22 = 1.0; res._m32 = 0.0; res._ty = v._y;
  res._m13 = 0.0; res._m23 = 0.0; res._m33 = 1.0; res._tz = v._z;

  return res;
}


// --------------------------------------------------------------------------
// ScaleMatrix
//
// Setup the matrix to perform scale on each axis.  For uniform scale by k,
// use a vector of the form Vector3( k, k, k ).
//
// The translation portion is reset.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
ScaleMatrix (const Vector3<Real> &s)
{
  Matrix4x4<Real> res;

  // Set the matrix elements.  Pretty straightforward
  res._m11 = s._x; res._m21 = 0.0;  res._m31 = 0.0;
  res._m12 = 0.0;  res._m22 = s._y; res._m32 = 0.0;
  res._m13 = 0.0;  res._m23 = 0.0;  res._m33 = s._z;

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.0;

  return res;
}


// --------------------------------------------------------------------------
// ScaleAlongAxisMatrix
//
// Setup the matrix to perform scale along an arbitrary axis.
//
// The axis is specified using a unit vector.
//
// The translation portion is reset.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
ScaleAlongAxisMatrix (const Vector3<Real> &axis, Real k)
{
  Matrix4x4<Real> res;

  // Quick sanity check to make sure they passed in a unit vector
  // to specify the axis
  assert (std::fabs (DotProduct (axis, axis) - 1.0) < 0.001);

  // Compute k-1 and some common subexpressions
  Real a = k - 1.0;
  Real ax = a * axis._x;
  Real ay = a * axis._y;
  Real az = a * axis._z;

  // Fill in the matrix elements.  We'll do the common
  // subexpression optimization ourselves here, since diagonally
  // opposite matrix elements are equal
  res._m11 = (ax * axis._x) + 1.0;
  res._m22 = (ay * axis._y) + 1.0;
  res._m32 = (az * axis._z) + 1.0;

  res._m12 = res._m21 = (ax * axis._y);
  res._m13 = res._m31 = (ax * axis._z);
  res._m23 = res._m32 = (ay * axis._z);

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.0;

  return res;
}


// --------------------------------------------------------------------------
// ShearMatrix
//
// Setup the matrix to perform a shear
//
// The type of shear is specified by the 1-based "axis" index.  The effect
// of transforming a point by the matrix is described by the pseudocode
// below:
//
//	xAxis  =>  y += s * x, z += t * x
//	yAxis  =>  x += s * y, z += t * y
//	zAxis  =>  x += s * z, y += t * z
//
// The translation portion is reset.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
ShearMatrix (Axis axis, Real s, Real t)
{
  Matrix4x4<Real> res;

  // Check which type of shear they want
  switch (axis)
    {
    case kXaxis: // Shear y and z using x
      res._m11 = 1.0; res._m21 = 0.0; res._m31 = 0.0;
      res._m12 = s;   res._m22 = 1.0; res._m32 = 0.0;
      res._m13 = t;   res._m23 = 0.0; res._m33 = 1.0;
      break;

    case kYaxis: // Shear x and z using y
      res._m11 = 1.0; res._m21 = s;   res._m31 = 0.0;
      res._m12 = 0.0; res._m22 = 1.0; res._m32 = 0.0;
      res._m13 = 0.0; res._m23 = t;   res._m33 = 1.0;
      break;

    case kZaxis: // Shear x and y using z
      res._m11 = 1.0; res._m21 = 0.0; res._m31 = s;
      res._m12 = 0.0; res._m22 = 1.0; res._m32 = t;
      res._m13 = 0.0; res._m23 = 0.0; res._m33 = 1.0;
      break;

    default:
      // bogus axis index
      assert (false);
    }

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.0;

  return res;
}


// --------------------------------------------------------------------------
// ProjectionMatrix
//
// Setup the matrix to perform a projection onto a plane passing
// through the origin.  The plane is perpendicular to the
// unit vector n.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
ProjectionMatrix (const Vector3<Real> &n)
{
  Matrix4x4<Real> res;

  // Quick sanity check to make sure they passed in a unit vector
  // to specify the axis
  assert (std::fabs (DotProduct (n, n) - 1.0) < 0.001);

  // Fill in the matrix elements.  We'll do the common
  // subexpression optimization ourselves here, since diagonally
  // opposite matrix elements are equal
  res._m11 = 1.0 - (n._x * n._x);
  res._m22 = 1.0 - (n._y * n._y);
  res._m33 = 1.0 - (n._z * n._z);

  res._m12 = res._m21 = -(n._x * n._y);
  res._m13 = res._m31 = -(n._x * n._z);
  res._m23 = res._m32 = -(n._y * n._z);

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.0;

  return res;
}


// --------------------------------------------------------------------------
// ReflectionMatrix
//
// Setup the matrix to perform a reflection about a plane parallel
// to a cardinal plane.
//
// axis is a 1-based index which specifies the plane to project about:
//
//	xAxis => reflect about the plane x=k
//	yAxis => reflect about the plane y=k
//	zAxis => reflect about the plane z=k
//
// The translation is set appropriately, since translation must occur if
// k != 0
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
ReflectionMatrix (Axis axis, Real k)
{
  Matrix4x4<Real> res;

  // Check which plane they want to reflect about
  switch (axis)
    {
    case kXaxis: // Reflect about the plane x=k
      res._m11 = -1.0; res._m21 =  0.0; res._m31 =  0.0; res._tx = 2.0 * k;
      res._m12 =  0.0; res._m22 =  1.0; res._m32 =  0.0; res._ty = 0.0;
      res._m13 =  0.0; res._m23 =  0.0; res._m33 =  1.0; res._tz = 0.0;
      break;

    case kYaxis: // Reflect about the plane y=k
      res._m11 =  1.0; res._m21 =  0.0; res._m31 =  0.0; res._tx = 0.0;
      res._m12 =  0.0; res._m22 = -1.0; res._m32 =  0.0; res._ty = 2.0 * k;
      res._m13 =  0.0; res._m23 =  0.0; res._m33 =  1.0; res._tz = 0.0;
      break;

    case kZaxis: // Reflect about the plane z=k
      res._m11 =  1.0; res._m21 =  0.0; res._m31 =  0.0; res._tx = 0.0;
      res._m12 =  0.0; res._m22 =  1.0; res._m32 =  0.0; res._ty = 0.0;
      res._m13 =  0.0; res._m23 =  0.0; res._m33 = -1.0; res._tz = 2.0 * k;
      break;

    default:
      // bogus axis index
      assert (false);
    }

  return res;
}


// --------------------------------------------------------------------------
// AxisReflectionMatrix
//
// Setup the matrix to perform a reflection about an arbitrary plane
// through the origin.  The unit vector n is perpendicular to the plane.
//
// The translation portion is reset.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
AxisReflectionMatrix (const Vector3<Real> &n)
{
  Matrix4x4<Real> res;

  // Quick sanity check to make sure they passed in a unit vector
  // to specify the axis
  assert (std::fabs (DotProduct (n, n) - 1.0) < 0.001);

  // Compute common subexpressions
  Real ax = -2.0 * n._x;
  Real ay = -2.0 * n._y;
  Real az = -2.0 * n._z;

  // Fill in the matrix elements.  We'll do the common
  // subexpression optimization ourselves here, since diagonally
  // opposite matrix elements are equal
  res._m11 = 1.0 + (ax * n._x);
  res._m22 = 1.0 + (ay * n._y);
  res._m32 = 1.0 + (az * n._z);

  res._m12 = res._m21 = (ax * n._y);
  res._m13 = res._m31 = (ax * n._z);
  res._m23 = res._m32 = (ay * n._z);

  // Reset the translation portion
  res._tx = res._ty = res._tz = 0.00;

  return res;
}


// --------------------------------------------------------------------------
// LookAtMatrix
//
// Setup the matrix to perform a "Look At" transformation like a first
// person camera.
// --------------------------------------------------------------------------
template <typename Real>
inline Matrix4x4<Real>
LookAtMatrix (const Vector3<Real> &camPos, const Vector3<Real> &target,
	      const Vector3<Real> &camUp)
{
  Matrix4x4<Real> rot, trans;

  Vector3<Real> forward (camPos - target);
  forward.normalize ();

  Vector3<Real> right (CrossProduct (camUp, forward));
  Vector3<Real> up (CrossProduct (forward, right));

  right.normalize ();
  up.normalize ();

  rot._m11 = right._x;
  rot._m21 = right._y;
  rot._m31 = right._z;

  rot._m12 = up._x;
  rot._m22 = up._y;
  rot._m32 = up._z;

  rot._m13 = forward._x;
  rot._m23 = forward._y;
  rot._m33 = forward._z;

  rot._tx  = 0.0;
  rot._ty  = 0.0;
  rot._tz  = 0.0;

  trans = TranslationMatrix (-camPos);

  return (rot * trans);
}


// --------------------------------------------------------------------------
// FrustumMatrix
//
// Setup a frustum matrix given the left, right, bottom, top, near, and far
// values for the frustum boundaries.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
FrustumMatrix (Real l, Real r, Real b, Real t, Real n, Real f)
{
  assert (n >= 0.0);
  assert (f >= 0.0);

  Matrix4x4<Real> res;

  Real width  = r - l;
  Real height = t - b;
  Real depth  = f - n;

  res._m[0] = (2 * n) / width;
  res._m[1] = 0.0;
  res._m[2] = 0.0;
  res._m[3] = 0.0;

  res._m[4] = 0.0;
  res._m[5] = (2 * n) / height;
  res._m[6] = 0.0;
  res._m[7] = 0.0;

  res._m[8] = (r + l) / width;
  res._m[9] = (t + b) / height;
  res._m[10]= -(f + n) / depth;
  res._m[11]= -1.0;

  res._m[12]= 0.0;
  res._m[13]= 0.0;
  res._m[14]= -(2 * f * n) / depth;
  res._m[15]= 0.0;

  return res;
}


// --------------------------------------------------------------------------
// PerspectiveMatrix
//
// Setup a perspective matrix given the field-of-view in the Y direction
// in degrees, the aspect ratio of Y/X, and near and far plane distances.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
PerspectiveMatrix (Real fovY, Real aspect, Real n, Real f)
{
  Matrix4x4<Real> res;

  Real angle;
  Real cot;

  angle = fovY / 2.0;
  angle = degToRad (angle);

  cot = std::cos (angle) / std::sin (angle);

  res._m[0] = cot / aspect;
  res._m[1] = 0.0;
  res._m[2] = 0.0;
  res._m[3] = 0.0;

  res._m[4] = 0.0;
  res._m[5] = cot;
  res._m[6] = 0.0;
  res._m[7] = 0.0;

  res._m[8] = 0.0;
  res._m[9] = 0.0;
  res._m[10]= -(f + n) / (f - n);
  res._m[11]= -1.0;

  res._m[12]= 0.0;
  res._m[13]= 0.0;
  res._m[14]= -(2 * f * n) / (f - n);
  res._m[15]= 0.0;

  return res;
}


// --------------------------------------------------------------------------
// OrthoMatrix
//
// Setup an orthographic Matrix4x4 given the left, right, bottom, top, near,
// and far values for the frustum boundaries.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
OrthoMatrix (Real l, Real r, Real b, Real t, Real n, Real f)
{
  Matrix4x4<Real> res;

  Real width  = r - l;
  Real height = t - b;
  Real depth  = f - n;

  res._m[0] =  2.0 / width;
  res._m[1] =  0.0;
  res._m[2] =  0.0;
  res._m[3] =  0.0;

  res._m[4] =  0.0;
  res._m[5] =  2.0 / height;
  res._m[6] =  0.0;
  res._m[7] =  0.0;

  res._m[8] =  0.0;
  res._m[9] =  0.0;
  res._m[10]= -2.0 / depth;
  res._m[11]=  0.0;

  res._m[12]= -(r + l) / width;
  res._m[13]= -(t + b) / height;
  res._m[14]= -(f + n) / depth;
  res._m[15]=  1.0;

  return res;
}


// --------------------------------------------------------------------------
// OrthoNormalMatrix
//
// Setup an orientation matrix using 3 basis normalized vectors.
// --------------------------------------------------------------------------

template <typename Real>
inline Matrix4x4<Real>
OrthoNormalMatrix (const Vector3<Real> &xdir, const Vector3<Real> &ydir,
		   const Vector3<Real> &zdir)
{
  Matrix4x4<Real> res;

  res._m[0] = xdir._x; res._m[4] = ydir._x; res._m[8] = zdir._x; res._m[12] = 0.0;
  res._m[1] = xdir._y; res._m[5] = ydir._y; res._m[9] = zdir._y; res._m[13] = 0.0;
  res._m[2] = xdir._z; res._m[6] = ydir._z; res._m[10]= zdir._z; res._m[14] = 0.0;
  res._m[3] = 0.0;     res._m[7] = 0.0;     res._m[11]= 0.0;     res._m[15] = 1.0;

  return res;
}


/////////////////////////////////////////////////////////////////////////////
//
// class Quaternion<Real> implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Quaternion::identity
//
// Set to identity
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::identity ()
{
  _w = 1.0; _x = _y = _z = 0.0;
}


// --------------------------------------------------------------------------
// Quaternion::normalize
//
// "Normalize" a quaternion.  Note that normally, quaternions
// are always normalized (within limits of numerical precision).
//
// This function is provided primarily to combat floating point "error
// creep", which can occur when many successive quaternion operations
// are applied.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::normalize ()
{
  // Compute magnitude of the quaternion
  Real mag = std::sqrt ((_w * _w) + (_x * _x) + (_y * _y) + (_z * _z));

  // Check for bogus length, to protect against divide by zero
  if (mag > 0.0)
    {
      // Normalize it
      Real oneOverMag = 1.0 / mag;

      _w *= oneOverMag;
      _x *= oneOverMag;
      _y *= oneOverMag;
      _z *= oneOverMag;
    }
}


// --------------------------------------------------------------------------
// Quaternion<Real>::computeW
//
// Compute the W component of a unit quaternion given its x,y,z components.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::computeW ()
{
  Real t = 1.0 - (_x * _x) - (_y * _y) - (_z * _z);

  if (t < 0.0)
    _w = 0.0;
  else
    _w = -std::sqrt (t);
}


// --------------------------------------------------------------------------
// Quaternion<Real>::rotate
//
// Rotate a point by quaternion.  v' = q.p.q*, where p = <0, v>.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::rotate (Vector3<Real> &v) const
{
  Quaternion<Real> qf = *this * v * ~(*this);
  v._x = qf._x;
  v._y = qf._y;
  v._z = qf._z;
}


// --------------------------------------------------------------------------
// Quaternion::fromMatrix
//
// Setup the quaternion to perform a rotation, given the angular displacement
// in matrix form.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::fromMatrix (const Matrix4x4<Real> &m)
{
  Real trace = m._m11 + m._m22 + m._m33 + 1.0;

  if (trace > 0.0001)
    {
      Real s = 0.5 / std::sqrt (trace);
      _w = 0.25 / s;
      _x = (m._m23 - m._m32) * s;
      _y = (m._m31 - m._m13) * s;
      _z = (m._m12 - m._m21) * s;
    }
  else
    {
      if ((m._m11 > m._m22) && (m._m11 > m._m33))
	{
	  Real s = 0.5 / std::sqrt (1.0 + m._m11 - m._m22 - m._m33);
	  _x = 0.25 / s;
	  _y = (m._m21 + m._m12) * s;
	  _z = (m._m31 + m._m13) * s;
	  _w = (m._m32 - m._m23) * s;
	}
      else if (m._m22 > m._m33)
	{
	  Real s = 0.5 / std::sqrt (1.0 + m._m22 - m._m11 - m._m33);
	  _x = (m._m21 + m._m12) * s;
	  _y = 0.25 / s;
	  _z = (m._m32 + m._m23) * s;
	  _w = (m._m31 - m._m13) * s;
	}
      else
	{
	  Real s = 0.5 / std::sqrt (1.0 + m._m33 - m._m11 - m._m22);
	  _x = (m._m31 + m._m13) * s;
	  _y = (m._m32 + m._m23) * s;
	  _z = 0.25 / s;
	  _w = (m._m21 - m._m12) * s;
	}
    }
}


// --------------------------------------------------------------------------
// Quaternion::fromEulerAngles
//
// Setup the quaternion to perform an object->inertial rotation, given the
// orientation in XYZ-Euler angles format.  x,y,z parameters must be in
// radians.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::fromEulerAngles (Real x, Real y, Real z)
{
  // Compute sine and cosine of the half angles
  Real sr = std::sin (x * 0.5);
  Real cr = std::cos (x * 0.5);
  Real sp = std::sin (y * 0.5);
  Real cp = std::cos (y * 0.5);
  Real sy = std::sin (z * 0.5);
  Real cy = std::cos (z * 0.5);

  // Compute values
  _w =  (cy * cp * cr) + (sy * sp * sr);
  _x = -(sy * sp * cr) + (cy * cp * sr);
  _y =  (cy * sp * cr) + (sy * cp * sr);
  _z = -(cy * sp * sr) + (sy * cp * cr);
}


// --------------------------------------------------------------------------
// Quaternion::toEulerAngles
//
// Setup the Euler angles, given an object->inertial rotation quaternion.
// Returned x,y,z are in radians.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Quaternion<Real>::toEulerAngles (Real &x, Real &y, Real &z) const
{
  // Compute Y-axis angle
  y = std::asin (2.0 * ((_x * _z) + (_w * _y)));

  // Compute cos and one over cos for optimization
  Real cy = std::cos (y);
  Real oneOverCosY = 1.0 / cy;

  if (std::fabs (cy) > 0.001)
    {
      // No gimball lock
      x = std::atan2 (2.0 * ((_w * _x) - (_y * _z)) * oneOverCosY,
		      (1.0 - 2.0 * (_x*_x + _y*_y)) * oneOverCosY);
      z = std::atan2 (2.0 * ((_w * _z) - (_x * _y)) * oneOverCosY,
		      (1.0 - 2.0 * (_y*_y + _z*_z)) * oneOverCosY);
    }
  else
    {
      // Gimbal lock case
      x = 0.0;
      z = std::atan2 (2.0 * ((_x * _y) + (_w * _z)),
		      1.0 - 2.0 * (_x*_x + _z*_z));
    }
}


// --------------------------------------------------------------------------
// Quaternion::getRotationAngle
//
// Return the rotation angle theta (in radians).
// --------------------------------------------------------------------------

template <typename Real>
inline Real
Quaternion<Real>::rotationAngle () const
{
  // Compute the half angle.  Remember that w = cos(theta / 2)
  Real thetaOver2 = safeAcos (_w);

  // Return the rotation angle
  return thetaOver2 * 2.0;
}


// --------------------------------------------------------------------------
// Quaternion::getRotationAxis
//
// Return the rotation axis.
// --------------------------------------------------------------------------

template <typename Real>
inline Vector3<Real>
Quaternion<Real>::rotationAxis () const
{
  // Compute sin^2(theta/2).  Remember that w = cos(theta/2),
  // and sin^2(x) + cos^2(x) = 1
  Real sinThetaOver2Sq = 1.0 - (_w * _w);

  // Protect against numerical imprecision
  if (sinThetaOver2Sq <= 0.0)
    {
      // Identity quaternion, or numerical imprecision.  Just
      // return any valid vector, since it doesn't matter

      return Vector3<Real> (1.0, 0.0, 0.0);
    }

  // Compute 1 / sin(theta/2)
  Real oneOverSinThetaOver2 = 1.0 / std::sqrt (sinThetaOver2Sq);

  // Return axis of rotation
  return Vector3<Real> (
	_x * oneOverSinThetaOver2,
	_y * oneOverSinThetaOver2,
	_z * oneOverSinThetaOver2
   );
}


// --------------------------------------------------------------------------
// Quaternion operators
//
// Operator overloading for basic quaternion operations.
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator+ (const Quaternion<Real> &q) const
{
  return Quaternion<Real> (_w + q._w, _x + q._x, _y + q._y, _z + q._z);
}

template <typename Real>
inline Quaternion<Real> &
Quaternion<Real>::operator+= (const Quaternion<Real> &q)
{
  _w += q._w; _x += q._x; _y += q._y; _z += q._z;
  return *this;
}

template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator- (const Quaternion<Real> &q) const
{
  return Quaternion<Real> (_w - q._w, _x - q._x, _y - q._y, _z - q._z);
}

template <typename Real>
inline Quaternion<Real> &
Quaternion<Real>::operator-= (const Quaternion<Real> &q)
{
  _w -= q._w; _x -= q._x; _y -= q._y; _z -= q._z;
  return *this;
}

// Quaternion multiplication.  The order of multiplication, from left
// to right, corresponds to the order of concatenated rotations.
// NOTE: Quaternion multiplication is NOT commutative, p * q != q * p
template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator* (const Quaternion<Real> &q) const
{
  // We use the Grassman product formula:
  // pq = <w1w2 - dot(v1, v2), w1v2 + w2v1 + cross(v1, v2)>
  return Quaternion<Real> (
	(_w * q._w) - (_x * q._x) - (_y * q._y) - (_z * q._z),
	(_x * q._w) + (_w * q._x) + (_y * q._z) - (_z * q._y),
	(_y * q._w) + (_w * q._y) + (_z * q._x) - (_x * q._z),
	(_z * q._w) + (_w * q._z) + (_x * q._y) - (_y * q._x)
   );
}

template <typename Real>
inline Quaternion<Real> &
Quaternion<Real>::operator*= (const Quaternion<Real> &q)
{
  *this = *this * q;
  return *this;
}

template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator* (const Vector3<Real> &v) const
{
  // q * v = q * p where p = <0,v>
  // Thus, we can simplify the operations.
  return Quaternion<Real> (
	- (_x * v._x) - (_y * v._y) - (_z * v._z),
	  (_w * v._x) + (_y * v._z) - (_z * v._y),
	  (_w * v._y) + (_z * v._x) - (_x * v._z),
	  (_w * v._z) + (_x * v._y) - (_y * v._x)
   );
}

template <typename Real>
inline Quaternion<Real> &
Quaternion<Real>::operator*= (const Vector3<Real> &v)
{
  *this = *this * v;
  return *this;
}

template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator* (Real k) const
{
  return Quaternion<Real> (_w * k, _x * k, _y * k, _z * k);
}

template <typename Real>
inline Quaternion<Real> &
Quaternion<Real>::operator*= (Real k)
{
  _w *= k; _x *= k; _y *= k; _z *= k;
  return *this;
}

template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator/ (Real k) const
{
  Real oneOverK = 1.0 / k;
  return Quaternion<Real> (_w * oneOverK, _x * oneOverK, _y * oneOverK, _z * oneOverK);
}

template <typename Real>
inline Quaternion<Real> &
Quaternion<Real>::operator/= (Real k)
{
  Real oneOverK = 1.0 / k;
  _w *= oneOverK; _x *= oneOverK; _y *= oneOverK; _z *= oneOverK;
  return *this;
}

// Quaternion conjugate
template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator~ () const
{
  return Quaternion<Real> (_w, -_x, -_y, -_z);
}


// Quaternion negation
template <typename Real>
inline Quaternion<Real>
Quaternion<Real>::operator- () const
{
  return Quaternion<Real> (-_w, -_x, -_y, -_z);
}


// --------------------------------------------------------------------------
//
// Nonmember Quaternion functions
//
// --------------------------------------------------------------------------

// Scalar on left multiplication
template <typename Real>
inline Quaternion<Real>
operator* (Real k, const Quaternion<Real> &q)
{
  return Quaternion<Real> (q._w * k, q._x * k, q._y * k, q._z * k);
}

// Quaternion dot product
template <typename Real>
inline Real
DotProduct (const Quaternion<Real> &a, const Quaternion<Real> &b)
{
  return ((a._w * b._w) + (a._x * b._x) + (a._y * b._y) + (a._z * b._z));
}

// Compute the quaternion conjugate.  This is the quaternian
// with the opposite rotation as the original quaternion.
template <typename Real>
inline Quaternion<Real>
Conjugate (const Quaternion<Real> &q)
{
  return Quaternion<Real> (q._w, -q._x, -q._y, -q._z);
}


// Compute the inverse quaternion (for unit quaternion only).
template <typename Real>
inline Quaternion<Real>
Inverse (const Quaternion<Real> &q)
{
  // Assume this is a unit quaternion! No check for this!
  Quaternion<Real> res (q._w, -q._x, -q._y, -q._z);
  res.normalize ();
  return res;
}


// --------------------------------------------------------------------------
// RotationQuaternion
//
// Setup the quaternion to rotate about the specified axis.  theta must
// be in radians.
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
RotationQuaternion (Axis axis, Real theta)
{
  Quaternion<Real> res;

  // Compute the half angle
  Real thetaOver2 = theta * 0.5;

  // Set the values
  switch (axis)
    {
    case kXaxis:
      res._w = std::cos (thetaOver2);
      res._x = std::sin (thetaOver2);
      res._y = 0.0;
      res._z = 0.0;
      break;

    case kYaxis:
      res._w = std::cos (thetaOver2);
      res._x = 0.0;
      res._y = std::sin (thetaOver2);
      res._z = 0.0;
      break;

    case kZaxis:
      res._w = std::cos (thetaOver2);
      res._x = 0.0;
      res._y = 0.0;
      res._z = std::sin (thetaOver2);
      break;

    default:
      // Bad axis
      assert (false);
  }

  return res;
}

template <typename Real>
inline Quaternion<Real>
RotationQuaternion (const Vector3<Real> &axis, Real theta)
{
  Quaternion<Real> res;

  // The axis of rotation must be normalized
  assert (std::fabs (DotProduct (axis, axis) - 1.0) < 0.001);

  // Compute the half angle and its sin
  Real thetaOver2 = theta * 0.5;
  Real sinThetaOver2 = std::sin (thetaOver2);

  // Set the values
  res._w = std::cos (thetaOver2);
  res._x = axis._x * sinThetaOver2;
  res._y = axis._y * sinThetaOver2;
  res._z = axis._z * sinThetaOver2;

  return res;
}


// --------------------------------------------------------------------------
// Log
//
// Unit quaternion logarithm. log(q) = log(cos(theta) + n*sin(theta))
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
Log (const Quaternion<Real> &q)
{
  Quaternion<Real> res;
  res._w = 0.0;

  if (std::fabs (q._w) < 1.0)
    {
      Real theta = std::acos (q._w);
      Real sin_theta = std::sin (theta);

      if (std::fabs (sin_theta) > 0.00001)
	{
	  Real thetaOverSinTheta = theta / sin_theta;
	  res._x = q._x * thetaOverSinTheta;
	  res._y = q._y * thetaOverSinTheta;
	  res._z = q._z * thetaOverSinTheta;
	  return res;
	}
    }

  res._x = q._x;
  res._y = q._y;
  res._z = q._z;

  return res;
}


// --------------------------------------------------------------------------
// Exp
//
// Quaternion exponential.
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
Exp (const Quaternion<Real> &q)
{
  Real theta = std::sqrt (DotProduct (q, q));
  Real sin_theta = std::sin (theta);

  Quaternion<Real> res;
  res._w = std::cos (theta);

  if (std::fabs (sin_theta) > 0.00001)
    {
      Real sinThetaOverTheta = sin_theta / theta;
      res._x = q._x * sinThetaOverTheta;
      res._y = q._y * sinThetaOverTheta;
      res._z = q._z * sinThetaOverTheta;
    }
  else
    {
      res._x = q._x;
      res._y = q._y;
      res._z = q._z;
    }

  return res;
}


// --------------------------------------------------------------------------
// Pow
//
// Quaternion exponentiation.
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
Pow (const Quaternion<Real> &q, Real exponent)
{
  // Check for the case of an identity quaternion.
  // This will protect against divide by zero
  if (std::fabs (q._w) > 0.9999)
    return q;

  // Extract the half angle alpha (alpha = theta/2)
  Real alpha = std::acos (q._w);

  // Compute new alpha value
  Real newAlpha = alpha * exponent;

  // Compute new quaternion
  Vector3<Real> n (q._x, q._y, q._z);
  n *= std::sin (newAlpha) / std::sin (alpha);

  return Quaternion<Real> (std::cos (newAlpha), n);
}


// --------------------------------------------------------------------------
// Slerp
//
// Spherical linear interpolation.
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
Slerp (const Quaternion<Real> &q0, const Quaternion<Real> &q1, Real t)
{
  // Check for out-of range parameter and return edge points if so
  if (t <= 0.0) return q0;
  if (t >= 1.0) return q1;

  // Compute "cosine of angle between quaternions" using dot product
  Real cosOmega = DotProduct (q0, q1);

  // If negative dot, use -q1.  Two quaternions q and -q
  // represent the same rotation, but may produce
  // different slerp.  We chose q or -q to rotate using
  // the acute angle.
  Real q1w = q1._w;
  Real q1x = q1._x;
  Real q1y = q1._y;
  Real q1z = q1._z;

  if (cosOmega < 0.0)
    {
      q1w = -q1w;
      q1x = -q1x;
      q1y = -q1y;
      q1z = -q1z;
      cosOmega = -cosOmega;
    }

  // We should have two unit quaternions, so dot should be <= 1.0
  assert (cosOmega < 1.1);

  // Compute interpolation fraction, checking for quaternions
  // almost exactly the same
  Real k0, k1;

  if (cosOmega > 0.9999)
    {
      // Very close - just use linear interpolation,
      // which will protect againt a divide by zero

      k0 = 1.0 - t;
      k1 = t;
    }
  else
    {
      // Compute the sin of the angle using the
      // trig identity sin^2(omega) + cos^2(omega) = 1
      Real sinOmega = std::sqrt (1.0 - (cosOmega * cosOmega));

      // Compute the angle from its sin and cosine
      Real omega = std::atan2 (sinOmega, cosOmega);

      // Compute inverse of denominator, so we only have
      // to divide once
      Real oneOverSinOmega = 1.0 / sinOmega;

      // Compute interpolation parameters
      k0 = std::sin ((1.0 - t) * omega) * oneOverSinOmega;
      k1 = std::sin (t * omega) * oneOverSinOmega;
    }

  // Interpolate and return new quaternion
  return Quaternion<Real> (
	(k0 * q0._w) + (k1 * q1w),
	(k0 * q0._x) + (k1 * q1x),
	(k0 * q0._y) + (k1 * q1y),
	(k0 * q0._z) + (k1 * q1z)
   );
}


// --------------------------------------------------------------------------
// Squad
//
// Spherical cubic interpolation.
// squad = slerp (slerp (q0, q1, t), slerp (qa, qb, t), 2t(1 - t)).
// --------------------------------------------------------------------------

template <typename Real>
inline Quaternion<Real>
Squad (const Quaternion<Real> &q0, const Quaternion<Real> &qa,
       const Quaternion<Real> &qb, const Quaternion<Real> &q1, Real t)
{
  Real slerp_t = 2.0 * t * (1.0 - t);

  Quaternion<Real> slerp_q0 = Slerp (q0, q1, t);
  Quaternion<Real> slerp_q1 = Slerp (qa, qb, t);

  return Slerp (slerp_q0, slerp_q1, slerp_t);
}


// --------------------------------------------------------------------------
// Intermediate
//
// Compute intermediate quaternions for building spline segments.
// --------------------------------------------------------------------------

template <typename Real>
inline void
Intermediate (const Quaternion<Real> &qprev, const Quaternion<Real> &qcurr,
	      const Quaternion<Real> &qnext, Quaternion<Real> &qa, Quaternion<Real> &qb)
{
  // We should have unit quaternions
  assert (DotProduct (qprev, qprev) <= 1.0001);
  assert (DotProduct (qcurr, qcurr) <= 1.0001);

  Quaternion<Real> inv_prev = Conjugate (qprev);
  Quaternion<Real> inv_curr = Conjugate (qcurr);

  Quaternion<Real> p0 = inv_prev * qcurr;
  Quaternion<Real> p1 = inv_curr * qnext;

  Quaternion<Real> arg = (Log (p0) - Log (p1)) * 0.25;

  qa = qcurr * Exp ( arg);
  qb = qcurr * Exp (-arg);
}
