//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

// Includes the standard math header, implements VC98's missing
// definitition of M_PI, and a function sq() to square a value.

#pragma once

#include <cmath>

#ifndef M_E
#define M_E        2.7182818284590452354E0  /*Hex  2^ 0 * 1.5bf0a8b145769 */
#define M_LOG2E    1.4426950408889634074E0  /*Hex  2^ 0 * 1.71547652B82FE */
#define M_LOG10E   4.3429448190325182765E-1 /*Hex  2^-2 * 1.BCB7B1526E50E */
#define M_LN2      6.9314718055994530942E-1 /*Hex  2^-1 * 1.62E42FEFA39EF */
#define M_LN10     2.3025850929940456840E0  /*Hex  2^ 1 * 1.26bb1bbb55516 */
#define M_PI       3.1415926535897932385E0  /*Hex  2^ 1 * 1.921FB54442D18 */
#define M_PI_2     1.5707963267948966192E0  /*Hex  2^ 0 * 1.921FB54442D18 */
#define M_PI_4     7.8539816339744830962E-1 /*Hex  2^-1 * 1.921FB54442D18 */
#define M_1_PI     3.1830988618379067154E-1 /*Hex  2^-2 * 1.45f306dc9c883 */
#define M_2_PI     6.3661977236758134308E-1 /*Hex  2^-1 * 1.45f306dc9c883 */
#define M_2_SQRTPI 1.1283791670955125739E0  /*Hex  2^ 0 * 1.20dd750429b6d */
#define M_SQRT2    1.4142135623730950488E0  /*Hex  2^ 0 * 1.6A09E667F3BCD */
#define M_SQRT1_2  7.0710678118654752440E-1 /*Hex  2^-1 * 1.6a09e667f3bcd */
#endif

#include "cat.h"

template<class Num> inline Num sq(const Num& val) { return val*val; }
#define clamp( val, minval, maxval )       ( min( maxval, max( val, minval ) ) )

const static Point3 P3_IDENTITY = Point3(0.0f, 0.0f, 0.0f);
const static Point3 P3_IDENTITY_SCALE = Point3(1.0f, 1.0f, 1.0f);
const static Point3 P3_UP_VEC = Point3(0.0f, 0.0f, 1.0f);

// Matrix3 row numbers -- to reduce confusion
#define X     kXAxis
#define Y     kYAxis
#define Z     kZAxis
#define TRANS kTrans

const static AngAxis AX_IDENTITIY = AngAxis(P3_UP_VEC, 0.0f);

// The maximum useful value we can pass to acos.  Any higher than this and we simply return 0
// Found by trial and error :-)
#define ACOS_LIMIT		0.99999998

// Math functions used in CAT
void RaySphereCollision(Ray &ray, Point3 spherecenter, float radius);

float BlendFloat(const float &val1, const float &Vec2, const float &ratio);
Point3 BlendVector(Point3 Vec1, Point3 Vec2, float ratio = 0.0f);
void BlendRot(Matrix3 &Rot1, const Matrix3 &Rot2, float ratio, BOOL useQuat = TRUE);
void BlendMat(Matrix3 &Rot1, const Matrix3 &Rot2, float ratio);
void MirrorQuat(Quat &qt, Axis mirroraxis);
void MirrorAngAxis(AngAxis &ax, int mirroraxis);
void MirrorPoint(Point3 &pt, int mirroraxis, BOOL eulerrot);
void BlendRotByAA(Matrix3 &Mat1, const Matrix3 &Mat2, float ratio);

void RotMat(Matrix3 &Mat1, const AngAxis &ax);
void RotMatToLookAtPoint(Matrix3 &tm, int axis, BOOL neg, const Point3 &target);
void RotateMatrixToAlignWithVector(Matrix3 &tm, const Point3 &vector, int axis);

BOOL RemoveTableNode(Tab<INode*> *nodes, INode* node);

BOOL MirrorCtrl(Control* ctrl, int mirrorplane);
void WriteControlToUserProps(INode* node, Control* ctrl, TSTR keytype);

BOOL MAXScriptEvaluate(Interface* ip, TCHAR *s, ReferenceTarget *this_ref, int i);

inline void dLine2Pts(GraphicsWindow *gw, Point3& r, Point3& q);
void DrawCross(GraphicsWindow *gw, Matrix3 tm, const float &size, Box3 &bbox);
inline void DrawArc10Segs(GraphicsWindow *gw, Matrix3 tm, const Point3 &axis, const float &degstart, const float &degend, const Point3 &spoke, Box3 &bbox, bool markers_at_ends = false, MarkerType marker_type = X_MRKR);
inline void DrawBox(GraphicsWindow *gw, Matrix3 tm, const Point3 &blr, const Point3 &trf, Box3 &bbox);

//////////////////////////////////////////////////////////////////////////
// CATMuscle maths.

// each collision object holds one of these flags
#define COLLISIONFLAG_OBJ_X_DISTORTION				(1<<3)
#define COLLISIONFLAG_ALWAYS_COLLIDE				(1<<4)
#define COLLISIONFLAG_INVERT						(1<<5)
#define COLLISIONFLAG_SMOOTH						(1<<6)

Point3 DetectCol(Point3 p3Up, INode *node, Interval &valid, Point3 point, TimeValue t, float distortion, float hardness, int flags);

Point3 CalcBezier(const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3, const float &mu, const Point3 *p4 = NULL);
void MakeFace(Face &face, int a, int b, int c, int ab, int bc, int ca, DWORD sg, MtlID id);

INode* FindNodeAtPos(INode* node, Point3 p, TimeValue t);
void ReparentNode(INode *oldnode, INode* newnode, Axis mirroraxis);
