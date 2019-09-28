//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license
//  agreement provided at the time of installation or download, or which
//  otherwise accompanies this software in either electronic or hard copy form.
//
//////////////////////////////////////////////////////////////////////////////

/**
* @file    Math.h
* @brief   Basic vectors and matrices.
*
* @author  Henrik Edstrom
* @date    2008-01-24
*
*/

#ifndef __RTI_MATH_H__
#define __RTI_MATH_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// C++ / STL
#include <math.h>

// rti
#include "rti/core/Common.h"

///////////////////////////////////////////////////////////////////////////////
// rti ////////////////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Constants ----------------------------------------

const float RTI_INFINITY      = 1e38f;
const float RTI_EPSILON       = 1e-6f;
const float RTI_PI            = 3.14159265358979323846f;
const float RTI_TWO_PI        = 2.0f * RTI_PI;
const float RTI_HALF_PI       = 0.5f * RTI_PI;
const float RTI_INV_PI        = 1.0f / RTI_PI;
const float RTI_INV_TWO_PI    = 1.0f / RTI_TWO_PI;
const float RTI_DEG_TO_RAD    = RTI_PI / 180.0f;
const float RTI_RAD_TO_DEG    = 180.0f / RTI_PI;

const double RTI_D_PI         = 3.14159265358979323846;   // d_pi
const double RTI_D_INV_PI     = 0.318309886183790671538;  // d_1_pi
const double RTI_D_TWO_PI     = 2.0 * RTI_D_PI;           // d_2pi
const double RTI_D_INV_TWO_PI = 0.5 * RTI_D_INV_PI;       // d_1_2pi
const double RTI_D_HALF_PI    = 0.5 * RTI_D_PI;           // d_pi_2


//---------- Forward declarations -----------------------------

struct Mat2f;
struct Mat3f;
struct Mat4f;


/**
* @class   Vec2f
* @brief   2D float vector.
*
* @author  Henrik Edstrom
* @date    2008-01-24
*/
struct Vec2f {

  //---------- Constructors -----------------------------------

  Vec2f() {}
  explicit Vec2f(const float* p);
  explicit Vec2f(float s);
  Vec2f(float x, float y);


  //---------- Operators --------------------------------------

  Vec2f         operator + (const Vec2f& rhs) const;
  Vec2f         operator - (const Vec2f& rhs) const;
  Vec2f         operator * (const Vec2f& rhs) const;
  Vec2f         operator / (const Vec2f& rhs) const;
  Vec2f         operator * (float rhs) const;
  Vec2f         operator / (float rhs) const;
  friend Vec2f  operator * (float lhs, const Vec2f& rhs);

  Vec2f&        operator += (const Vec2f& rhs);
  Vec2f&        operator -= (const Vec2f& rhs);
  Vec2f&        operator *= (const Vec2f& rhs);
  Vec2f&        operator /= (const Vec2f& rhs);
  Vec2f&        operator *= (float rhs);
  Vec2f&        operator /= (float rhs);

  Vec2f         operator - () const;

  bool          operator == (const Vec2f& rhs) const;
  bool          operator != (const Vec2f& rhs) const;

  // casting
  operator float*();
  operator const float*() const;


  //---------- Attributes -------------------------------------
  float x, y;

};  // Vec2f


/**
* @class   Vec3f
* @brief   3D float vector.
*
* @author  Henrik Edstrom
* @date    2008-01-24
*/
struct Vec3f {

  //---------- Constructors -----------------------------------

  Vec3f() {}
  explicit Vec3f(const float* p);
  explicit Vec3f(float s);
  Vec3f(float x, float y, float z);


  //---------- Operators --------------------------------------

  Vec3f         operator + (const Vec3f& rhs) const;
  Vec3f         operator - (const Vec3f& rhs) const;
  Vec3f         operator * (const Vec3f& rhs) const;
  Vec3f         operator / (const Vec3f& rhs) const;
  Vec3f         operator * (float rhs) const;
  Vec3f         operator / (float rhs) const;
  friend Vec3f  operator * (float lhs, const Vec3f& rhs);

  Vec3f&        operator += (const Vec3f& rhs);
  Vec3f&        operator -= (const Vec3f& rhs);
  Vec3f&        operator *= (const Vec3f& rhs);
  Vec3f&        operator /= (const Vec3f& rhs);
  Vec3f&        operator *= (float rhs);
  Vec3f&        operator /= (float rhs);

  Vec3f         operator - () const;

  // casting
  operator float*();
  operator const float*() const;

  bool          operator == (const Vec3f& rhs) const;
  bool          operator != (const Vec3f& rhs) const;

  //---------- Attributes -------------------------------------
  float x, y, z;

};  // Vec3f


/**
* @class   Vec4f
* @brief   4D float vector.
*
* @author  Henrik Edstrom
* @date    2008-01-24
*/
struct Vec4f {
  
  //---------- Constructors -----------------------------------

  Vec4f() {}
  explicit Vec4f(const float* p);
  explicit Vec4f(float s);
  Vec4f(float x, float y, float z, float w);


  //---------- Operators --------------------------------------

  Vec4f         operator + (const Vec4f& rhs) const;
  Vec4f         operator - (const Vec4f& rhs) const;
  Vec4f         operator * (const Vec4f& rhs) const;
  Vec4f         operator / (const Vec4f& rhs) const;
  Vec4f         operator * (float rhs) const;
  Vec4f         operator / (float rhs) const;
  friend Vec4f  operator * (float lhs, const Vec4f& rhs);

  Vec4f&        operator += (const Vec4f& rhs);
  Vec4f&        operator -= (const Vec4f& rhs);
  Vec4f&        operator *= (const Vec4f& rhs);
  Vec4f&        operator /= (const Vec4f& rhs);
  Vec4f&        operator *= (float rhs);
  Vec4f&        operator /= (float rhs);

  Vec4f         operator - () const;

  bool          operator == (const Vec4f& rhs) const;
  bool          operator != (const Vec4f& rhs) const;

  // casting
  operator float*();
  operator const float*() const;


  //---------- Attributes -------------------------------------
  float x, y, z, w;

};  // Vec4f


/**
* @class   Vec2i
* @brief   2D int vector.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Vec2i {

  //---------- Constructors -----------------------------------

  Vec2i() {}
  explicit Vec2i(const int* p);
  explicit Vec2i(int n);
  Vec2i(int x, int y);


  //---------- Operators --------------------------------------

  Vec2i         operator + (const Vec2i& rhs) const;
  Vec2i         operator - (const Vec2i& rhs) const;
  Vec2i         operator * (const Vec2i& rhs) const;
  Vec2i         operator / (const Vec2i& rhs) const;
  Vec2i         operator * (int rhs) const;
  Vec2i         operator / (int rhs) const;
  friend Vec2i  operator * (int lhs, const Vec2i& rhs);

  Vec2i&        operator += (const Vec2i& rhs);
  Vec2i&        operator -= (const Vec2i& rhs);
  Vec2i&        operator *= (const Vec2i& rhs);
  Vec2i&        operator /= (const Vec2i& rhs);
  Vec2i&        operator *= (int rhs);
  Vec2i&        operator /= (int rhs);

  Vec2i         operator - () const;

  bool          operator == (const Vec2i& rhs) const;
  bool          operator != (const Vec2i& rhs) const;

  // casting
  operator int*();
  operator const int*() const;


  //---------- Attributes -------------------------------------
   int x, y;

};  // Vec2i


/**
* @class   Vec3i
* @brief   3D int vector.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Vec3i {

  //---------- Constructors -----------------------------------

  Vec3i() {}
  explicit Vec3i(const int* p);
  explicit Vec3i(int n);
  Vec3i(int x, int y, int z);


  //---------- Operators --------------------------------------

  Vec3i         operator + (const Vec3i& rhs) const;
  Vec3i         operator - (const Vec3i& rhs) const;
  Vec3i         operator * (const Vec3i& rhs) const;
  Vec3i         operator / (const Vec3i& rhs) const;
  Vec3i         operator * (int rhs) const;
  Vec3i         operator / (int rhs) const;
  friend Vec3i  operator * (int lhs, const Vec3i& rhs);

  Vec3i&        operator += (const Vec3i& rhs);
  Vec3i&        operator -= (const Vec3i& rhs);
  Vec3i&        operator *= (const Vec3i& rhs);
  Vec3i&        operator /= (const Vec3i& rhs);
  Vec3i&        operator *= (int rhs);
  Vec3i&        operator /= (int rhs);

  Vec3i         operator - () const;

  bool          operator == (const Vec3i& rhs) const;
  bool          operator != (const Vec3i& rhs) const;

  // casting
  operator int*();
  operator const int*() const;


  //---------- Attributes -------------------------------------
  int x, y, z;

};  // Vec3i


/**
* @class   Vec4i
* @brief   4D int vector.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Vec4i {

  //---------- Constructors -----------------------------------

  Vec4i() {}
  explicit Vec4i(const int* p);
  explicit Vec4i(int n);
  Vec4i(int x, int y, int z, int w);


  //---------- Operators --------------------------------------

  Vec4i         operator + (const Vec4i& rhs) const;
  Vec4i         operator - (const Vec4i& rhs) const;
  Vec4i         operator * (const Vec4i& rhs) const;
  Vec4i         operator / (const Vec4i& rhs) const;
  Vec4i         operator * (int rhs) const;
  Vec4i         operator / (int rhs) const;
  friend Vec4i  operator * (int lhs, const Vec4i& rhs);

  Vec4i&        operator += (const Vec4i& rhs);
  Vec4i&        operator -= (const Vec4i& rhs);
  Vec4i&        operator *= (const Vec4i& rhs);
  Vec4i&        operator /= (const Vec4i& rhs);
  Vec4i&        operator *= (int rhs);
  Vec4i&        operator /= (int rhs);

  Vec4i         operator - () const;

  bool          operator == (const Vec4i& rhs) const;
  bool          operator != (const Vec4i& rhs) const;

  // casting
  operator int*();
  operator const int*() const;


  //---------- Attributes -------------------------------------
  int x, y, z, w;

};  // Vec4i


/**
* @class   Vec2b
* @brief   2D bool vector.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Vec2b {

  //---------- Constructors -----------------------------------

  Vec2b() {}
  explicit Vec2b(const bool* p);
  explicit Vec2b(bool b);
  Vec2b(bool x, bool y);


  //---------- Operators --------------------------------------

  Vec2b         operator ! () const;

  bool          operator == (const Vec2b& rhs) const;
  bool          operator != (const Vec2b& rhs) const;

  // casting
  operator bool*();
  operator const bool*() const;


  //---------- Attributes -------------------------------------
  bool x, y;

};  // Vec2b


/**
* @class   Vec3b
* @brief   3D bool vector.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Vec3b {

  //---------- Constructors -----------------------------------

  Vec3b() {}
  explicit Vec3b(const bool* p);
  explicit Vec3b(bool b);
  Vec3b(bool x, bool y, bool z);


  //---------- Operators --------------------------------------

  Vec3b         operator ! () const;

  bool          operator == (const Vec3b& rhs) const;
  bool          operator != (const Vec3b& rhs) const;

  // casting
  operator bool*();
  operator const bool*() const;


  //---------- Attributes -------------------------------------
  bool x, y, z;

};  // Vec3b


/**
* @class   Vec4b
* @brief   4D bool vector.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Vec4b {
  //---------- Constructors -----------------------------------

  Vec4b() {}
  explicit Vec4b(const bool* p);
  explicit Vec4b(bool b);
  Vec4b(bool x, bool y, bool z, bool w);


  //---------- Operators --------------------------------------

  Vec4b         operator ! () const;

  bool          operator == (const Vec4b& rhs) const;
  bool          operator != (const Vec4b& rhs) const;

  // casting
  operator bool*();
  operator const bool*() const;


  //---------- Attributes -------------------------------------
  bool x, y, z, w;

};  // Vec4b


/**
* @class   Mat2f
* @brief   2x2 float matrix.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Mat2f {

  //---------- Constructors -----------------------------------

  Mat2f() {}
  explicit Mat2f(const float* p);
  explicit Mat2f(float s);
  explicit Mat2f(const Mat3f& m3);
  explicit Mat2f(const Mat4f& m4);
  Mat2f(Vec2f col0, Vec2f col1);
  Mat2f(float c0r0, float c0r1, 
        float c1r0, float c1r1);
  Mat2f(const Mat2f& rhs);


  //---------- Operators --------------------------------------

  Mat2f&        operator = (const Mat2f& rhs);
  Vec2f&        operator [] (int i);
  const Vec2f&  operator [] (int i) const;

  Mat2f         operator * (const Mat2f& rhs) const;
  Vec2f         operator * (const Vec2f& rhs) const;
  Mat2f         operator + (const Mat2f& rhs) const;
  Mat2f         operator - (const Mat2f& rhs) const;
  Mat2f         operator * (float rhs) const;
  Mat2f         operator / (float rhs) const;
  friend Mat2f  operator * (float lhs, const Mat2f& rhs);
  friend Vec2f  operator * (const Vec2f& lhs, const Mat2f& rhs);
  friend Vec2f& operator *= (Vec2f& lhs, const Mat2f& rhs);

  Mat2f&        operator *= (const Mat2f& rhs);
  Mat2f&        operator += (const Mat2f& rhs);
  Mat2f&        operator -= (const Mat2f& rhs);
  Mat2f&        operator *= (float rhs);
  Mat2f&        operator /= (float rhs);

  Mat2f         operator - () const;

  bool          operator == (const Mat2f& rhs) const;
  bool          operator != (const Mat2f& rhs) const;

  // casting
  operator float*();
  operator const float*() const;


  //---------- Attributes -------------------------------------

  Vec2f m[2];

};  // Mat2f


/**
* @class   Mat3f
* @brief   3x3 float matrix.
*
* @author  Henrik Edstrom
* @date    2008-05-05
*/
struct Mat3f {

  //---------- Constructors -----------------------------------

  Mat3f() {}
  explicit Mat3f(const float* p);
  explicit Mat3f(float s);
  explicit Mat3f(const Mat4f& m4);
  Mat3f(Vec3f col0, Vec3f col1, Vec3f col2);
  Mat3f(float c0r0, float c0r1, float c0r2, 
        float c1r0, float c1r1, float c1r2, 
        float c2r0, float c2r1, float c2r2);
  Mat3f(const Mat3f& rhs);


  //---------- Operators --------------------------------------

  Mat3f&        operator = (const Mat3f& rhs);
  Vec3f&        operator [] (int i);
  const Vec3f&  operator [] (int i) const;

  Mat3f         operator * (const Mat3f& rhs) const;
  Vec3f         operator * (const Vec3f& rhs) const;
  Mat3f         operator + (const Mat3f& rhs) const;
  Mat3f         operator - (const Mat3f& rhs) const;
  Mat3f         operator * (float rhs) const;
  Mat3f         operator / (float rhs) const;
  friend Mat3f  operator * (float lhs, const Mat3f& rhs);
  friend Vec3f  operator * (const Vec3f& lhs, const Mat3f& rhs);
  friend Vec3f& operator *= (Vec3f& lhs, const Mat3f& rhs);

  Mat3f&        operator *= (const Mat3f& rhs);
  Mat3f&        operator += (const Mat3f& rhs);
  Mat3f&        operator -= (const Mat3f& rhs);
  Mat3f&        operator *= (float rhs);
  Mat3f&        operator /= (float rhs);

  Mat3f         operator - () const;

  bool          operator == (const Mat3f& rhs) const;
  bool          operator != (const Mat3f& rhs) const;

  // casting
  operator float*();
  operator const float*() const;


  //---------- Attributes -------------------------------------

  Vec3f m[3];

};  // Mat3f


/**
* @class   Mat4f
* @brief   4x4 float matrix.
*
* @author  Henrik Edstrom
* @date    2008-01-24
*/
struct Mat4f {

  //---------- Constructors -----------------------------------

  Mat4f() {}
  explicit Mat4f(const float* p);
  explicit Mat4f(float s);
  Mat4f(Vec4f col0, Vec4f col1, Vec4f col2, Vec4f col3);
  Mat4f(float c0r0, float c0r1, float c0r2, float c0r3, 
        float c1r0, float c1r1, float c1r2, float c1r3,
        float c2r0, float c2r1, float c2r2, float c2r3, 
        float c3r0, float c3r1, float c3r2, float c3r3);

  Mat4f(const Mat4f& rhs);


  //---------- Operators --------------------------------------

  Mat4f&        operator = (const Mat4f& rhs);
  Vec4f&        operator [] (int i);
  const Vec4f&  operator [] (int i) const;

  Mat4f         operator * (const Mat4f& rhs) const;
  Vec4f         operator * (const Vec4f& rhs) const;
  Mat4f         operator + (const Mat4f& rhs) const;
  Mat4f         operator - (const Mat4f& rhs) const;
  Mat4f         operator * (float rhs) const;
  Mat4f         operator / (float rhs) const;
  friend Mat4f  operator * (float lhs, const Mat4f& rhs);
  friend Vec4f  operator * (const Vec4f& lhs, const Mat4f& rhs);
  friend Vec4f& operator *= (Vec4f& lhs, const Mat4f& rhs);

  Mat4f&        operator *= (const Mat4f& rhs);
  Mat4f&        operator += (const Mat4f& rhs);
  Mat4f&        operator -= (const Mat4f& rhs);
  Mat4f&        operator *= (float rhs);
  Mat4f&        operator /= (float rhs);

  Mat4f         operator - () const;

  bool          operator == (const Mat4f& rhs) const;
  bool          operator != (const Mat4f& rhs) const;

  // casting
  operator float*();
  operator const float*() const;


  //---------- Attributes -------------------------------------

  Vec4f m[4];

};  // Mat4f


//---------- Scalar functions ---------------------------------

float clamp(float value, float low, float high);
int clamp(int value, int low, int high);


//---------- Vector functions ---------------------------------

float dot(const Vec2f& lhs, const Vec2f& rhs);
float dot(const Vec3f& lhs, const Vec3f& rhs);
float dot(const Vec4f& lhs, const Vec4f& rhs);

Vec3f cross(const Vec3f& lhs, const Vec3f& rhs);

float length(const Vec2f& v);
float length(const Vec3f& v);
float length(const Vec4f& v);

float normalize(Vec2f& v);
float normalize(Vec3f& v);
float normalize(Vec4f& v);

Vec3f reflect(const Vec3f& i, const Vec3f& n);

Vec3f transformPoint(const Vec3f& v, const Mat4f& m);
Vec3f transformVector(const Vec3f& v, const Mat4f& m);
Vec3f transformVector(const Vec3f& v, const Mat3f& m);


//---------- Matrix functions ---------------------------------

Mat2f& makeIdentity(Mat2f& mat);
Mat3f& makeIdentity(Mat3f& mat);
Mat4f& makeIdentity(Mat4f& mat);

bool isIdentity(const Mat2f& mat);
bool isIdentity(const Mat3f& mat);
bool isIdentity(const Mat4f& mat);

float determinant(const Mat2f& mat);
float determinant(const Mat3f& mat);
float determinant(const Mat4f& mat);

Mat2f& transpose(Mat2f& res, const Mat2f& mat);
Mat3f& transpose(Mat3f& res, const Mat3f& mat);
Mat4f& transpose(Mat4f& res, const Mat4f& mat);

Mat2f& inverse(Mat2f& res, const Mat2f& mat);
Mat3f& inverse(Mat3f& res, const Mat3f& mat);
Mat4f& inverse(Mat4f& res, const Mat4f& mat);

Mat3f& normalMatrix(Mat3f& res, const Mat4f& mat);

Mat4f translate(float dx, float dy, float dz);
Mat4f translate(const Vec3f& translation);
Mat4f scale(float s);
Mat4f scale(float sx, float sy, float sz);
Mat4f rotate(Vec3f axis, float angle);
Mat4f rotateX(float angle);
Mat4f rotateY(float angle);
Mat4f rotateZ(float angle);
void axisToAxisRotation(Vec3f from, Vec3f into, Mat4f& mat);

END_RTI


#include "rti/math/Math.inl"

///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_MATH_H__
