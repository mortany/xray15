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
* @file    ShaderStdLib.h
* @brief   Shaders standard library functions.
*
* @author  Henrik Edstrom
* @date    2008-04-09
*
*/

#ifndef __RTI_SHADER_STD_LIB_H__
#define __RTI_SHADER_STD_LIB_H__

#ifndef DOXYGEN_SHOULD_SKIP_THIS

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// Math
#include "rti/math/Math.h"


///////////////////////////////////////////////////////////////////////////////
// RapidSL Standard Library ///////////////////////////////////////////////////

BEGIN_RTI

//---------- Angle and trigonometry functions -----------------

float sl_lib_acos(float x);
Vec2f sl_lib_acos(const Vec2f& x);
Vec3f sl_lib_acos(const Vec3f& x);
Vec4f sl_lib_acos(const Vec4f& x);

float sl_lib_asin(float x);
Vec2f sl_lib_asin(const Vec2f& x);
Vec3f sl_lib_asin(const Vec3f& x);
Vec4f sl_lib_asin(const Vec4f& x);

float sl_lib_atan(float x);
Vec2f sl_lib_atan(const Vec2f& x);
Vec3f sl_lib_atan(const Vec3f& x);
Vec4f sl_lib_atan(const Vec4f& x);

float sl_lib_atan2(float y, float x);
Vec2f sl_lib_atan2(const Vec2f& y, const Vec2f& x);
Vec3f sl_lib_atan2(const Vec2f& y, const Vec3f& x);
Vec4f sl_lib_atan2(const Vec2f& y, const Vec4f& x);

float sl_lib_cos(float x);
Vec2f sl_lib_cos(const Vec2f& x);
Vec3f sl_lib_cos(const Vec3f& x);
Vec4f sl_lib_cos(const Vec4f& x);

float sl_lib_degrees(float x);
Vec2f sl_lib_degrees(const Vec2f& x);
Vec3f sl_lib_degrees(const Vec3f& x);
Vec4f sl_lib_degrees(const Vec4f& x);

float sl_lib_radians(float x);
Vec2f sl_lib_radians(const Vec2f& x);
Vec3f sl_lib_radians(const Vec3f& x);
Vec4f sl_lib_radians(const Vec4f& x);

float sl_lib_sin(float x);
Vec2f sl_lib_sin(const Vec2f& x);
Vec3f sl_lib_sin(const Vec3f& x);
Vec4f sl_lib_sin(const Vec4f& x);

float sl_lib_tan(float x);
Vec2f sl_lib_tan(const Vec2f& x);
Vec3f sl_lib_tan(const Vec3f& x);
Vec4f sl_lib_tan(const Vec4f& x);


//---------- Exponential functions ----------------------------

float sl_lib_exp(float x);
Vec2f sl_lib_exp(const Vec2f& x);
Vec3f sl_lib_exp(const Vec3f& x);
Vec4f sl_lib_exp(const Vec4f& x);

float sl_lib_exp2(float x);
Vec2f sl_lib_exp2(const Vec2f& x);
Vec3f sl_lib_exp2(const Vec3f& x);
Vec4f sl_lib_exp2(const Vec4f& x);

float sl_lib_log(float x);
Vec2f sl_lib_log(const Vec2f& x);
Vec3f sl_lib_log(const Vec3f& x);
Vec4f sl_lib_log(const Vec4f& x);

float sl_lib_log2(float x);
Vec2f sl_lib_log2(const Vec2f& x);
Vec3f sl_lib_log2(const Vec3f& x);
Vec4f sl_lib_log2(const Vec4f& x);

float sl_lib_log10(float x);
Vec2f sl_lib_log10(const Vec2f& x);
Vec3f sl_lib_log10(const Vec3f& x);
Vec4f sl_lib_log10(const Vec4f& x);

int sl_lib_pow(int x, int y);
float sl_lib_pow(float x, int y);
float sl_lib_pow(float x, float y);
Vec2f sl_lib_pow(const Vec2f& x, int y);
Vec2f sl_lib_pow(const Vec2f& x, float y);
Vec2f sl_lib_pow(const Vec2f& x, const Vec2f& y);
Vec3f sl_lib_pow(const Vec3f& x, int y);
Vec3f sl_lib_pow(const Vec3f& x, float y);
Vec3f sl_lib_pow(const Vec3f& x, const Vec3f& y);
Vec4f sl_lib_pow(const Vec4f& x, int y);
Vec4f sl_lib_pow(const Vec4f& x, float y);
Vec4f sl_lib_pow(const Vec4f& x, const Vec4f& y);

float sl_lib_rsqrt(float x);
Vec2f sl_lib_rsqrt(const Vec2f& x);
Vec3f sl_lib_rsqrt(const Vec3f& x);
Vec4f sl_lib_rsqrt(const Vec4f& x);

float sl_lib_sqrt(float x);
Vec2f sl_lib_sqrt(const Vec2f& x);
Vec3f sl_lib_sqrt(const Vec3f& x);
Vec4f sl_lib_sqrt(const Vec4f& x);


//---------- Math functions -----------------------------------

int sl_lib_abs(int x);
float sl_lib_abs(float x);
Vec2f sl_lib_abs(const Vec2f& x);
Vec3f sl_lib_abs(const Vec3f& x);
Vec4f sl_lib_abs(const Vec4f& x);

float sl_lib_ceil(float x);
Vec2f sl_lib_ceil(const Vec2f& x);
Vec3f sl_lib_ceil(const Vec3f& x);
Vec4f sl_lib_ceil(const Vec4f& x);

int sl_lib_clamp(int x, int l, int h);
float sl_lib_clamp(float x, float l, float h);
Vec2f sl_lib_clamp(const Vec2f& x, float l, float h);
Vec2f sl_lib_clamp(const Vec2f& x, const Vec2f& l, const Vec2f& h);
Vec3f sl_lib_clamp(const Vec3f& x, float l, float h);
Vec3f sl_lib_clamp(const Vec3f& x, const Vec3f& l, const Vec3f& h);
Vec4f sl_lib_clamp(const Vec4f& x, float l, float h);
Vec4f sl_lib_clamp(const Vec4f& x, const Vec4f& l, const Vec4f& h);

float sl_lib_floor(float x);
Vec2f sl_lib_floor(const Vec2f& x);
Vec3f sl_lib_floor(const Vec3f& x);
Vec4f sl_lib_floor(const Vec4f& x);

float sl_lib_fmod(float x, float y);
Vec2f sl_lib_fmod(const Vec2f& x, float y);
Vec2f sl_lib_fmod(const Vec2f& x, const Vec2f& y);
Vec3f sl_lib_fmod(const Vec3f& x, float y);
Vec3f sl_lib_fmod(const Vec3f& x, const Vec3f& y);
Vec4f sl_lib_fmod(const Vec4f& x, float y);
Vec4f sl_lib_fmod(const Vec4f& x, const Vec4f& y);

float sl_lib_frac(float x);
Vec2f sl_lib_frac(const Vec2f& x);
Vec3f sl_lib_frac(const Vec3f& x);
Vec4f sl_lib_frac(const Vec4f& x);

bool sl_lib_inside(float x, float l, float h);
Vec2b sl_lib_inside(const Vec2f& x, float l, float h);
Vec2b sl_lib_inside(const Vec2f& x, const Vec2f& l, const Vec2f& h);
Vec3b sl_lib_inside(const Vec3f& x, float l, float h);
Vec3b sl_lib_inside(const Vec3f& x, const Vec3f& l, const Vec3f& h);
Vec4b sl_lib_inside(const Vec4f& x, float l, float h);
Vec4b sl_lib_inside(const Vec4f& x, const Vec4f& l, const Vec4f& h);

float sl_lib_lerp(float x, float y, float t);
Vec2f sl_lib_lerp(const Vec2f& x, const Vec2f& y, float t);
Vec2f sl_lib_lerp(const Vec2f& x, const Vec2f& y, const Vec2f& t);
Vec3f sl_lib_lerp(const Vec3f& x, const Vec3f& y, float t);
Vec3f sl_lib_lerp(const Vec3f& x, const Vec3f& y, const Vec3f& t);
Vec4f sl_lib_lerp(const Vec4f& x, const Vec4f& y, float t);
Vec4f sl_lib_lerp(const Vec4f& x, const Vec4f& y, const Vec4f& t);

int sl_lib_max(int x, int y);
float sl_lib_max(float x, float y);
Vec2f sl_lib_max(const Vec2f& x, const Vec2f& y);
Vec3f sl_lib_max(const Vec3f& x, const Vec3f& y);
Vec4f sl_lib_max(const Vec4f& x, const Vec4f& y);

float sl_lib_max(const Vec2f& x);
float sl_lib_max(const Vec3f& x);
float sl_lib_max(const Vec4f& x);

int sl_lib_min(int x, int y);
float sl_lib_min(float x, float y);
Vec2f sl_lib_min(const Vec2f& x, const Vec2f& y);
Vec3f sl_lib_min(const Vec3f& x, const Vec3f& y);
Vec4f sl_lib_min(const Vec4f& x, const Vec4f& y);

float sl_lib_min(const Vec2f& x);
float sl_lib_min(const Vec3f& x);
float sl_lib_min(const Vec4f& x);

float sl_lib_round(float x);
Vec2f sl_lib_round(const Vec2f& x);
Vec3f sl_lib_round(const Vec3f& x);
Vec4f sl_lib_round(const Vec4f& x);

float sl_lib_saturate(float x);
Vec2f sl_lib_saturate(const Vec2f& x);
Vec3f sl_lib_saturate(const Vec3f& x);
Vec4f sl_lib_saturate(const Vec4f& x);

int sl_lib_sign(int x);
float sl_lib_sign(float x);
Vec2f sl_lib_sign(const Vec2f& x);
Vec3f sl_lib_sign(const Vec3f& x);
Vec4f sl_lib_sign(const Vec4f& x);

float sl_lib_smoothstep(float x, float y, float t);
Vec2f sl_lib_smoothstep(const Vec2f& x, const Vec2f& y, float t);
Vec2f sl_lib_smoothstep(const Vec2f& x, const Vec2f& y, const Vec2f& t);
Vec3f sl_lib_smoothstep(const Vec3f& x, const Vec3f& y, float t);
Vec3f sl_lib_smoothstep(const Vec3f& x, const Vec3f& y, const Vec3f& t);
Vec4f sl_lib_smoothstep(const Vec4f& x, const Vec4f& y, float t);
Vec4f sl_lib_smoothstep(const Vec4f& x, const Vec4f& y, const Vec4f& t);

float sl_lib_step(float e, float t);
Vec2f sl_lib_step(float e, const Vec2f& t);
Vec2f sl_lib_step(const Vec2f& e, const Vec2f& t);
Vec3f sl_lib_step(float e, const Vec3f& t);
Vec3f sl_lib_step(const Vec3f& e, const Vec3f& t);
Vec4f sl_lib_step(float e, const Vec4f& t);
Vec4f sl_lib_step(const Vec4f& e, const Vec4f& t);


//---------- Logical functions --------------------------------

bool sl_lib_all(bool x);
bool sl_lib_all(const Vec2b& x);
bool sl_lib_all(const Vec3b& x);
bool sl_lib_all(const Vec4b& x);

bool sl_lib_any(bool x);
bool sl_lib_any(const Vec2b& x);
bool sl_lib_any(const Vec3b& x);
bool sl_lib_any(const Vec4b& x);

bool sl_lib_not(bool x);
Vec2b sl_lib_not(const Vec2b& x);
Vec3b sl_lib_not(const Vec3b& x);
Vec4b sl_lib_not(const Vec4b& x);


//---------- Geometric functions ------------------------------

Vec3f sl_lib_cross(const Vec3f& lhs, const Vec3f& rhs);

float sl_lib_distance(float x, float y);
float sl_lib_distance(const Vec2f& x, const Vec2f& y);
float sl_lib_distance(const Vec3f& x, const Vec3f& y);
float sl_lib_distance(const Vec4f& x, const Vec4f& y);

float sl_lib_dot(const Vec2f& x, const Vec2f& y);
float sl_lib_dot(const Vec3f& x, const Vec3f& y);
float sl_lib_dot(const Vec4f& x, const Vec4f& y);

Vec2f sl_lib_faceforward(const Vec2f& n, const Vec2f& i, const Vec2f& nref);
Vec3f sl_lib_faceforward(const Vec3f& n, const Vec3f& i, const Vec3f& nref);
Vec4f sl_lib_faceforward(const Vec4f& n, const Vec4f& i, const Vec4f& nref);

float sl_lib_length(const Vec2f& x);
float sl_lib_length(const Vec3f& x);
float sl_lib_length(const Vec4f& x);

Vec2f sl_lib_normalize(const Vec2f& x);
Vec3f sl_lib_normalize(const Vec3f& x);
Vec4f sl_lib_normalize(const Vec4f& x);

Vec3f sl_lib_reflect(const Vec3f& i, const Vec3f& n);

Vec3f sl_lib_refract(const Vec3f& i, const Vec3f& n, float eta);


//---------- Matrix functions ---------------------------------

float sl_lib_determinant(const Mat2f& m);
float sl_lib_determinant(const Mat3f& m);
float sl_lib_determinant(const Mat4f& m);

Mat2f sl_lib_inverse(const Mat2f& m);
Mat3f sl_lib_inverse(const Mat3f& m);
Mat4f sl_lib_inverse(const Mat4f& m);

Mat2f sl_lib_transpose(const Mat2f& m);
Mat3f sl_lib_transpose(const Mat3f& m);
Mat4f sl_lib_transpose(const Mat4f& m);


//---------- Shading functions --------------------------------

float sl_lib_fresnel(const Vec3f& idir, const Vec3f& normal, float eta);

END_RTI

#include "rti/shader/ShaderStdLib.inl"

///////////////////////////////////////////////////////////////////////////////


#endif  // DOXYGEN_SHOULD_SKIP_THIS

#endif  // __RTI_SHADER_STD_LIB_H__
