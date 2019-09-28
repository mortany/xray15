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
* @file    ShaderStdLib.inl
* @brief   Shaders standard library functions inlines.
*
* @author  Henrik Edstrom
* @date    2008-04-09
*
*/

#ifndef __RTI_SHADER_STD_LIB_INL__
#define __RTI_SHADER_STD_LIB_INL__

#ifndef DOXYGEN_SHOULD_SKIP_THIS

///////////////////////////////////////////////////////////////////////////////
// RapidSL Standard Library ///////////////////////////////////////////////////

BEGIN_RTI

//---------- Angle and trigonometry functions ---------------

RTI_INLINE
float sl_lib_acos(float x) {
  return acosf(x);
}


RTI_INLINE
Vec2f sl_lib_acos(const Vec2f& x) {
  return Vec2f(acosf(x[0]), acosf(x[1]));
}


RTI_INLINE
Vec3f sl_lib_acos(const Vec3f& x) {
  return Vec3f(acosf(x[0]), acosf(x[1]), acosf(x[2]));
}


RTI_INLINE
Vec4f sl_lib_acos(const Vec4f& x) {
  return Vec4f(acosf(x[0]), acosf(x[1]), acosf(x[2]), acosf(x[3]));
}


RTI_INLINE
float sl_lib_asin(float x) {
  return asinf(x);
}


RTI_INLINE
Vec2f sl_lib_asin(const Vec2f& x) {
  return Vec2f(asinf(x[0]), asinf(x[1]));
}


RTI_INLINE
Vec3f sl_lib_asin(const Vec3f& x) {
  return Vec3f(asinf(x[0]), asinf(x[1]), asinf(x[2]));
}


RTI_INLINE
Vec4f sl_lib_asin(const Vec4f& x) {
  return Vec4f(asinf(x[0]), asinf(x[1]), asinf(x[2]), asinf(x[3]));
}


RTI_INLINE
float sl_lib_atan(float x) {
  return atanf(x);
}


RTI_INLINE
Vec2f sl_lib_atan(const Vec2f& x) {
  return Vec2f(atanf(x[0]), atanf(x[1]));
}


RTI_INLINE
Vec3f sl_lib_atan(const Vec3f& x) {
  return Vec3f(atanf(x[0]), atanf(x[1]), atanf(x[2]));
}


RTI_INLINE
Vec4f sl_lib_atan(const Vec4f& x) {
  return Vec4f(atanf(x[0]), atanf(x[1]), atanf(x[2]), atanf(x[3]));
}


RTI_INLINE
float sl_lib_atan2(float y, float x) {
  return atan2f(y, x);
}


RTI_INLINE
Vec2f sl_lib_atan2(const Vec2f& y, const Vec2f& x) {
  return Vec2f(atan2f(y[0], x[0]), atan2f(y[1], x[1]));
}


RTI_INLINE
Vec3f sl_lib_atan2(const Vec3f& y, const Vec3f& x) {
  return Vec3f(atan2f(y[0], x[0]), atan2f(y[1], x[1]), atan2f(y[2], x[2]));
}


RTI_INLINE
Vec4f sl_lib_atan2(const Vec4f& y, const Vec4f& x) {
  return Vec4f(atan2f(y[0], x[0]), atan2f(y[1], x[1]), atan2f(y[2], x[2]), atan2f(y[3], x[3]));
}


RTI_INLINE
float sl_lib_cos(float x) {
  return cosf(x);
}


RTI_INLINE
Vec2f sl_lib_cos(const Vec2f& x) {
  return Vec2f(cosf(x[0]), cosf(x[1]));
}


RTI_INLINE
Vec3f sl_lib_cos(const Vec3f& x) {
  return Vec3f(cosf(x[0]), cosf(x[1]), cosf(x[2]));
}


RTI_INLINE
Vec4f sl_lib_cos(const Vec4f& x) {
  return Vec4f(cosf(x[0]), cosf(x[1]), cosf(x[2]), cosf(x[3]));
}


RTI_INLINE
float sl_lib_degrees(float x) {
  return x * RTI_RAD_TO_DEG;
}


RTI_INLINE
Vec2f sl_lib_degrees(const Vec2f& x) {
  return x * RTI_RAD_TO_DEG;
}


RTI_INLINE
Vec3f sl_lib_degrees(const Vec3f& x) {
  return x * RTI_RAD_TO_DEG;
}


RTI_INLINE
Vec4f sl_lib_degrees(const Vec4f& x) {
  return x * RTI_RAD_TO_DEG;
}


RTI_INLINE
float sl_lib_radians(float x) {
  return x * RTI_DEG_TO_RAD;
}


RTI_INLINE
Vec2f sl_lib_radians(const Vec2f& x) {
  return x * RTI_DEG_TO_RAD;
}


RTI_INLINE
Vec3f sl_lib_radians(const Vec3f& x) {
  return x * RTI_DEG_TO_RAD;
}


RTI_INLINE
Vec4f sl_lib_radians(const Vec4f& x) {
  return x * RTI_DEG_TO_RAD;
}


RTI_INLINE
float sl_lib_sin(float x) {
  return sinf(x);
}


RTI_INLINE
Vec2f sl_lib_sin(const Vec2f& x) {
  return Vec2f(sinf(x[0]), sinf(x[1]));
}


RTI_INLINE
Vec3f sl_lib_sin(const Vec3f& x) {
  return Vec3f(sinf(x[0]), sinf(x[1]), sinf(x[2]));
}


RTI_INLINE
Vec4f sl_lib_sin(const Vec4f& x) {
  return Vec4f(sinf(x[0]), sinf(x[1]), sinf(x[2]), sinf(x[3]));
}


RTI_INLINE
float sl_lib_tan(float x) {
  return tanf(x);
}


RTI_INLINE
Vec2f sl_lib_tan(const Vec2f& x) {
  return Vec2f(tanf(x[0]), tanf(x[1]));
}


RTI_INLINE
Vec3f sl_lib_tan(const Vec3f& x) {
  return Vec3f(tanf(x[0]), tanf(x[1]), tanf(x[2]));
}


RTI_INLINE
Vec4f sl_lib_tan(const Vec4f& x) {
  return Vec4f(tanf(x[0]), tanf(x[1]), tanf(x[2]), tanf(x[3]));
}


//---------- Exponential functions --------------------------

RTI_INLINE
float sl_lib_exp(float x) {
  return expf(x);
}


RTI_INLINE
Vec2f sl_lib_exp(const Vec2f& x) {
  return Vec2f(sl_lib_exp(x[0]), sl_lib_exp(x[1]));
}


RTI_INLINE
Vec3f sl_lib_exp(const Vec3f& x) {
  return Vec3f(sl_lib_exp(x[0]), sl_lib_exp(x[1]), sl_lib_exp(x[2]));
}


RTI_INLINE
Vec4f sl_lib_exp(const Vec4f& x) {
  return Vec4f(sl_lib_exp(x[0]), sl_lib_exp(x[1]), sl_lib_exp(x[2]), sl_lib_exp(x[3]));
}


RTI_INLINE
float sl_lib_exp2(float x) {
  return sl_lib_pow(2.0f, x);
}


RTI_INLINE
Vec2f sl_lib_exp2(const Vec2f& x) {
  return Vec2f(sl_lib_exp2(x[0]), sl_lib_exp2(x[1]));
}


RTI_INLINE
Vec3f sl_lib_exp2(const Vec3f& x) {
  return Vec3f(sl_lib_exp2(x[0]), sl_lib_exp2(x[1]), sl_lib_exp2(x[2]));
}


RTI_INLINE
Vec4f sl_lib_exp2(const Vec4f& x) {
  return Vec4f(sl_lib_exp2(x[0]), sl_lib_exp2(x[1]), sl_lib_exp2(x[2]), sl_lib_exp2(x[3]));
}


RTI_INLINE
float sl_lib_log(float x) {
  return logf(x);
}


RTI_INLINE
Vec2f sl_lib_log(const Vec2f& x) {
  return Vec2f(sl_lib_log(x[0]), sl_lib_log(x[1]));
}


RTI_INLINE
Vec3f sl_lib_log(const Vec3f& x) {
  return Vec3f(sl_lib_log(x[0]), sl_lib_log(x[1]), sl_lib_log(x[2]));
}


RTI_INLINE
Vec4f sl_lib_log(const Vec4f& x) {
  return Vec4f(sl_lib_log(x[0]), sl_lib_log(x[1]), sl_lib_log(x[2]), sl_lib_log(x[3]));
}


RTI_INLINE
float sl_lib_log2(float x) {
  static const float fInvLog2 = 1.0f / logf(2.0f);
  return logf(x) * fInvLog2;
}


RTI_INLINE
Vec2f sl_lib_log2(const Vec2f& x) {
  return Vec2f(sl_lib_log2(x[0]), sl_lib_log2(x[1]));
}


RTI_INLINE
Vec3f sl_lib_log2(const Vec3f& x) {
  return Vec3f(sl_lib_log2(x[0]), sl_lib_log2(x[1]), sl_lib_log2(x[2]));
}


RTI_INLINE
Vec4f sl_lib_log2(const Vec4f& x) {
  return Vec4f(sl_lib_log2(x[0]), sl_lib_log2(x[1]), sl_lib_log2(x[2]), sl_lib_log2(x[3]));
}


RTI_INLINE
float sl_lib_log10(float x) {
  static const float fInvLog10 = 1.0f / logf(10.0f);
  return logf(x) * fInvLog10;
}


RTI_INLINE
Vec2f sl_lib_log10(const Vec2f& x) {
  return Vec2f(sl_lib_log10(x[0]), sl_lib_log10(x[1]));
}


RTI_INLINE
Vec3f sl_lib_log10(const Vec3f& x) {
  return Vec3f(sl_lib_log10(x[0]), sl_lib_log10(x[1]), sl_lib_log10(x[2]));
}


RTI_INLINE
Vec4f sl_lib_log10(const Vec4f& x) {
  return Vec4f(sl_lib_log10(x[0]), sl_lib_log10(x[1]), sl_lib_log10(x[2]), sl_lib_log10(x[3]));
}


RTI_INLINE
int sl_lib_pow(int x, int y) {
  return int(powf(float(x), float(y)));
}


RTI_INLINE
float sl_lib_pow(float x, int y) {
  return powf(x, float(y));
}


RTI_INLINE
float sl_lib_pow(float x, float y) {
  return powf(x, y);
}


RTI_INLINE
Vec2f sl_lib_pow(const Vec2f& x, int y) {
  return Vec2f(sl_lib_pow(x[0], y), sl_lib_pow(x[1], y));
}


RTI_INLINE
Vec2f sl_lib_pow(const Vec2f& x, float y) {
  return Vec2f(sl_lib_pow(x[0], y), sl_lib_pow(x[1], y));
}


RTI_INLINE
Vec2f sl_lib_pow(const Vec2f& x, const Vec2f& y) {
  return Vec2f(sl_lib_pow(x[0], y[0]), sl_lib_pow(x[1], y[1]));
}


RTI_INLINE
Vec3f sl_lib_pow(const Vec3f& x, int y) {
  return Vec3f(sl_lib_pow(x[0], y), sl_lib_pow(x[1], y), sl_lib_pow(x[2], y));
}


RTI_INLINE
Vec3f sl_lib_pow(const Vec3f& x, float y) {
  return Vec3f(sl_lib_pow(x[0], y), sl_lib_pow(x[1], y), sl_lib_pow(x[2], y));
}


RTI_INLINE
Vec3f sl_lib_pow(const Vec3f& x, const Vec3f& y) {
  return Vec3f(sl_lib_pow(x[0], y[0]), sl_lib_pow(x[1], y[1]), sl_lib_pow(x[2], y[2]));
}


RTI_INLINE
Vec4f sl_lib_pow(const Vec4f& x, int y) {
  return Vec4f(sl_lib_pow(x[0], y), sl_lib_pow(x[1], y), sl_lib_pow(x[2], y), sl_lib_pow(x[3], y));
}


RTI_INLINE
Vec4f sl_lib_pow(const Vec4f& x, float y) {
  return Vec4f(sl_lib_pow(x[0], y), sl_lib_pow(x[1], y), sl_lib_pow(x[2], y), sl_lib_pow(x[3], y));
}


RTI_INLINE
Vec4f sl_lib_pow(const Vec4f& x, const Vec4f& y) {
  return Vec4f(sl_lib_pow(x[0], y[0]), sl_lib_pow(x[1], y[1]), sl_lib_pow(x[2], y[2]),
               sl_lib_pow(x[3], y[3]));
}


RTI_INLINE
float sl_lib_rsqrt(float x) {
  return 1.0f / sqrtf(x);
}


RTI_INLINE
Vec2f sl_lib_rsqrt(const Vec2f& x) {
  return Vec2f(sl_lib_rsqrt(x[0]), sl_lib_rsqrt(x[1]));
}


RTI_INLINE
Vec3f sl_lib_rsqrt(const Vec3f& x) {
  return Vec3f(sl_lib_rsqrt(x[0]), sl_lib_rsqrt(x[1]), sl_lib_rsqrt(x[2]));
}


RTI_INLINE
Vec4f sl_lib_rsqrt(const Vec4f& x) {
  return Vec4f(sl_lib_rsqrt(x[0]), sl_lib_rsqrt(x[1]), sl_lib_rsqrt(x[2]), sl_lib_rsqrt(x[3]));
}


RTI_INLINE
float sl_lib_sqrt(float x) {
  return sqrtf(x);
}


RTI_INLINE
Vec2f sl_lib_sqrt(const Vec2f& x) {
  return Vec2f(sl_lib_sqrt(x[0]), sl_lib_sqrt(x[1]));
}


RTI_INLINE
Vec3f sl_lib_sqrt(const Vec3f& x) {
  return Vec3f(sl_lib_sqrt(x[0]), sl_lib_sqrt(x[1]), sl_lib_sqrt(x[2]));
}


RTI_INLINE
Vec4f sl_lib_sqrt(const Vec4f& x) {
  return Vec4f(sl_lib_sqrt(x[0]), sl_lib_sqrt(x[1]), sl_lib_sqrt(x[2]), sl_lib_sqrt(x[3]));
}


//---------- Math functions -----------------------------------

RTI_INLINE
int sl_lib_abs(int x) {
  return x < 0 ? -x : x;
}


RTI_INLINE
float sl_lib_abs(float x) {
  return fabsf(x);
}


RTI_INLINE
Vec2f sl_lib_abs(const Vec2f& x) {
  return Vec2f(sl_lib_abs(x[0]), sl_lib_abs(x[1]));
}


RTI_INLINE
Vec3f sl_lib_abs(const Vec3f& x) {
  return Vec3f(sl_lib_abs(x[0]), sl_lib_abs(x[1]), sl_lib_abs(x[2]));
}


RTI_INLINE
Vec4f sl_lib_abs(const Vec4f& x) {
  return Vec4f(sl_lib_abs(x[0]), sl_lib_abs(x[1]), sl_lib_abs(x[2]), sl_lib_abs(x[3]));
}


RTI_INLINE
float sl_lib_ceil(float x) {
  return ceilf(x);
}


RTI_INLINE
Vec2f sl_lib_ceil(const Vec2f& x) {
  return Vec2f(sl_lib_ceil(x[0]), sl_lib_ceil(x[1]));
}


RTI_INLINE
Vec3f sl_lib_ceil(const Vec3f& x) {
  return Vec3f(sl_lib_ceil(x[0]), sl_lib_ceil(x[1]), sl_lib_ceil(x[2]));
}


RTI_INLINE
Vec4f sl_lib_ceil(const Vec4f& x) {
  return Vec4f(sl_lib_ceil(x[0]), sl_lib_ceil(x[1]), sl_lib_ceil(x[2]), sl_lib_ceil(x[3]));
}


RTI_INLINE
int sl_lib_clamp(int x, int l, int h) {
  return sl_lib_min(h, sl_lib_max(x, l));
}


RTI_INLINE
float sl_lib_clamp(float x, float l, float h) {
  return sl_lib_min(h, sl_lib_max(x, l));
}


RTI_INLINE
Vec2f sl_lib_clamp(const Vec2f& x, float l, float h) {
  return Vec2f(sl_lib_min(h, sl_lib_max(x[0], l)), sl_lib_min(h, sl_lib_max(x[1], l)));
}


RTI_INLINE
Vec2f sl_lib_clamp(const Vec2f& x, const Vec2f& l, const Vec2f& h) {
  return Vec2f(sl_lib_min(h[0], sl_lib_max(x[0], l[0])), sl_lib_min(h[1], sl_lib_max(x[1], l[1])));
}


RTI_INLINE
Vec3f sl_lib_clamp(const Vec3f& x, float l, float h) {
  return Vec3f(sl_lib_min(h, sl_lib_max(x[0], l)), sl_lib_min(h, sl_lib_max(x[1], l)),
               sl_lib_min(h, sl_lib_max(x[2], l)));
}


RTI_INLINE
Vec3f sl_lib_clamp(const Vec3f& x, const Vec3f& l, const Vec3f& h) {
  return Vec3f(sl_lib_min(h[0], sl_lib_max(x[0], l[0])), sl_lib_min(h[1], sl_lib_max(x[1], l[1])),
               sl_lib_min(h[2], sl_lib_max(x[2], l[2])));
}


RTI_INLINE
Vec4f sl_lib_clamp(const Vec4f& x, float l, float h) {
  return Vec4f(sl_lib_min(h, sl_lib_max(x[0], l)), sl_lib_min(h, sl_lib_max(x[1], l)),
               sl_lib_min(h, sl_lib_max(x[2], l)), sl_lib_min(h, sl_lib_max(x[3], l)));
}


RTI_INLINE
Vec4f sl_lib_clamp(const Vec4f& x, const Vec4f& l, const Vec4f& h) {
  return Vec4f(sl_lib_min(h[0], sl_lib_max(x[0], l[0])), sl_lib_min(h[1], sl_lib_max(x[1], l[1])),
               sl_lib_min(h[2], sl_lib_max(x[2], l[2])), sl_lib_min(h[3], sl_lib_max(x[3], l[3])));
}


RTI_INLINE
float sl_lib_floor(float x) {
  return floorf(x);
}


RTI_INLINE
Vec2f sl_lib_floor(const Vec2f& x) {
  return Vec2f(sl_lib_floor(x[0]), sl_lib_floor(x[1]));
}


RTI_INLINE
Vec3f sl_lib_floor(const Vec3f& x) {
  return Vec3f(sl_lib_floor(x[0]), sl_lib_floor(x[1]), sl_lib_floor(x[2]));
}


RTI_INLINE
Vec4f sl_lib_floor(const Vec4f& x) {
  return Vec4f(sl_lib_floor(x[0]), sl_lib_floor(x[1]), sl_lib_floor(x[2]), sl_lib_floor(x[3]));
}


RTI_INLINE
float sl_lib_fmod(float x, float y) {
  return x - y * sl_lib_floor(x / y);
}


RTI_INLINE
Vec2f sl_lib_fmod(const Vec2f& x, float y) {
  return Vec2f(x[0] - y * sl_lib_floor(x[0] / y), x[1] - y * sl_lib_floor(x[1] / y));
}


RTI_INLINE
Vec2f sl_lib_fmod(const Vec2f& x, const Vec2f& y) {
  return Vec2f(x[0] - y[0] * sl_lib_floor(x[0] / y[0]), x[1] - y[1] * sl_lib_floor(x[1] / y[1]));
}


RTI_INLINE
Vec3f sl_lib_fmod(const Vec3f& x, float y) {
  return Vec3f(x[0] - y * sl_lib_floor(x[0] / y), x[1] - y * sl_lib_floor(x[1] / y),
               x[2] - y * sl_lib_floor(x[2] / y));
}


RTI_INLINE
Vec3f sl_lib_fmod(const Vec3f& x, const Vec3f& y) {
  return Vec3f(x[0] - y[0] * sl_lib_floor(x[0] / y[0]), x[1] - y[1] * sl_lib_floor(x[1] / y[1]),
               x[2] - y[2] * sl_lib_floor(x[2] / y[2]));
}


RTI_INLINE
Vec4f sl_lib_fmod(const Vec4f& x, float y) {
  return Vec4f(x[0] - y * sl_lib_floor(x[0] / y), x[1] - y * sl_lib_floor(x[1] / y),
               x[2] - y * sl_lib_floor(x[2] / y), x[3] - y * sl_lib_floor(x[3] / y));
}


RTI_INLINE
Vec4f sl_lib_fmod(const Vec4f& x, const Vec4f& y) {
  return Vec4f(x[0] - y[0] * sl_lib_floor(x[0] / y[0]), x[1] - y[1] * sl_lib_floor(x[1] / y[1]),
               x[2] - y[2] * sl_lib_floor(x[2] / y[2]), x[3] - y[3] * sl_lib_floor(x[3] / y[3]));
}


RTI_INLINE
float sl_lib_frac(float x) {
  return x - sl_lib_floor(x);
}


RTI_INLINE
Vec2f sl_lib_frac(const Vec2f& x) {
  return x - sl_lib_floor(x);
}


RTI_INLINE
Vec3f sl_lib_frac(const Vec3f& x) {
  return x - sl_lib_floor(x);
}


RTI_INLINE
Vec4f sl_lib_frac(const Vec4f& x) {
  return x - sl_lib_floor(x);
}


RTI_INLINE
bool sl_lib_inside(float x, float l, float h) {
  return x >= l && x <= h;
}


RTI_INLINE
Vec2b sl_lib_inside(const Vec2f& x, float l, float h) {
  return Vec2b(x[0] >= l && x[0] <= h, x[1] >= l && x[1] <= h);
}


RTI_INLINE
Vec2b sl_lib_inside(const Vec2f& x, const Vec2f& l, const Vec2f& h) {
  return Vec2b(x[0] >= l[0] && x[0] <= h[0], x[1] >= l[1] && x[1] <= h[1]);
}


RTI_INLINE
Vec3b sl_lib_inside(const Vec3f& x, float l, float h) {
  return Vec3b(x[0] >= l && x[0] <= h, x[1] >= l && x[1] <= h, x[2] >= l && x[2] <= h);
}


RTI_INLINE
Vec3b sl_lib_inside(const Vec3f& x, const Vec3f& l, const Vec3f& h) {
  return Vec3b(x[0] >= l[0] && x[0] <= h[0], x[1] >= l[1] && x[1] <= h[1],
               x[2] >= l[2] && x[2] <= h[2]);
}


RTI_INLINE
Vec4b sl_lib_inside(const Vec4f& x, float l, float h) {
  return Vec4b(x[0] >= l && x[0] <= h, x[1] >= l && x[1] <= h, x[2] >= l && x[2] <= h,
               x[3] >= l && x[3] <= h);
}


RTI_INLINE
Vec4b sl_lib_inside(const Vec4f& x, const Vec4f& l, const Vec4f& h) {
  return Vec4b(x[0] >= l[0] && x[0] <= h[0], x[1] >= l[1] && x[1] <= h[1],
               x[2] >= l[2] && x[2] <= h[2], x[3] >= l[3] && x[3] <= h[3]);
}


RTI_INLINE
float sl_lib_lerp(float x, float y, float t) {
  return x * (1.0f - t) + y * t;
}


RTI_INLINE
Vec2f sl_lib_lerp(const Vec2f& x, const Vec2f& y, float t) {
  return Vec2f(x[0] * (1.0f - t) + y[0] * t, x[1] * (1.0f - t) + y[1] * t);
}


RTI_INLINE
Vec2f sl_lib_lerp(const Vec2f& x, const Vec2f& y, const Vec2f& t) {
  return Vec2f(x[0] * (1.0f - t[0]) + y[0] * t[0], x[1] * (1.0f - t[1]) + y[1] * t[1]);
}


RTI_INLINE
Vec3f sl_lib_lerp(const Vec3f& x, const Vec3f& y, float t) {
  return Vec3f(x[0] * (1.0f - t) + y[0] * t, x[1] * (1.0f - t) + y[1] * t,
               x[2] * (1.0f - t) + y[2] * t);
}


RTI_INLINE
Vec3f sl_lib_lerp(const Vec3f& x, const Vec3f& y, const Vec3f& t) {
  return Vec3f(x[0] * (1.0f - t[0]) + y[0] * t[0], x[1] * (1.0f - t[1]) + y[1] * t[1],
               x[2] * (1.0f - t[2]) + y[2] * t[2]);
}


RTI_INLINE
Vec4f sl_lib_lerp(const Vec4f& x, const Vec4f& y, float t) {
  return Vec4f(x[0] * (1.0f - t) + y[0] * t, x[1] * (1.0f - t) + y[1] * t,
               x[2] * (1.0f - t) + y[2] * t, x[3] * (1.0f - t) + y[3] * t);
}


RTI_INLINE
Vec4f sl_lib_lerp(const Vec4f& x, const Vec4f& y, const Vec4f& t) {
  return Vec4f(x[0] * (1.0f - t[0]) + y[0] * t[0], x[1] * (1.0f - t[1]) + y[1] * t[1],
               x[2] * (1.0f - t[2]) + y[2] * t[2], x[3] * (1.0f - t[3]) + y[3] * t[3]);
}


RTI_INLINE
int sl_lib_max(int x, int y) {
  return x > y ? x : y;
}


RTI_INLINE
float sl_lib_max(float x, float y) {
  return x > y ? x : y;
}


RTI_INLINE
Vec2f sl_lib_max(const Vec2f& x, const Vec2f& y) {
  return Vec2f(x[0] > y[0] ? x[0] : y[0], x[1] > y[1] ? x[1] : y[1]);
}


RTI_INLINE
Vec3f sl_lib_max(const Vec3f& x, const Vec3f& y) {
  return Vec3f(x[0] > y[0] ? x[0] : y[0], x[1] > y[1] ? x[1] : y[1], x[2] > y[2] ? x[2] : y[2]);
}


RTI_INLINE
Vec4f sl_lib_max(const Vec4f& x, const Vec4f& y) {
  return Vec4f(x[0] > y[0] ? x[0] : y[0], x[1] > y[1] ? x[1] : y[1], x[2] > y[2] ? x[2] : y[2],
               x[3] > y[3] ? x[3] : y[3]);
}


RTI_INLINE
float sl_lib_max(const Vec2f& x) {
  return sl_lib_max(x[0], x[1]);
}


RTI_INLINE
float sl_lib_max(const Vec3f& x) {
  return sl_lib_max(x[0], sl_lib_max(x[1], x[2]));
}


RTI_INLINE
float sl_lib_max(const Vec4f& x) {
  return sl_lib_max(sl_lib_max(x[0], x[1]), sl_lib_max(x[2], x[3]));
}


RTI_INLINE
int sl_lib_min(int x, int y) {
  return x < y ? x : y;
}


RTI_INLINE
float sl_lib_min(float x, float y) {
  return x < y ? x : y;
}


RTI_INLINE
Vec2f sl_lib_min(const Vec2f& x, const Vec2f& y) {
  return Vec2f(x[0] < y[0] ? x[0] : y[0], x[1] < y[1] ? x[1] : y[1]);
}


RTI_INLINE
Vec3f sl_lib_min(const Vec3f& x, const Vec3f& y) {
  return Vec3f(x[0] < y[0] ? x[0] : y[0], x[1] < y[1] ? x[1] : y[1], x[2] < y[2] ? x[2] : y[2]);
}


RTI_INLINE
Vec4f sl_lib_min(const Vec4f& x, const Vec4f& y) {
  return Vec4f(x[0] < y[0] ? x[0] : y[0], x[1] < y[1] ? x[1] : y[1], x[2] < y[2] ? x[2] : y[2],
               x[3] < y[3] ? x[3] : y[3]);
}


RTI_INLINE
float sl_lib_min(const Vec2f& x) {
  return sl_lib_min(x[0], x[1]);
}


RTI_INLINE
float sl_lib_min(const Vec3f& x) {
  return sl_lib_min(x[0], sl_lib_min(x[1], x[2]));
}


RTI_INLINE
float sl_lib_min(const Vec4f& x) {
  return sl_lib_min(sl_lib_min(x[0], x[1]), sl_lib_min(x[2], x[3]));
}


RTI_INLINE
float sl_lib_round(float x) {
  return floorf(x + 0.5f);
}


RTI_INLINE
Vec2f sl_lib_round(const Vec2f& x) {
  return sl_lib_floor(x + Vec2f(0.5f));
}


RTI_INLINE
Vec3f sl_lib_round(const Vec3f& x) {
  return sl_lib_floor(x + Vec3f(0.5f));
}


RTI_INLINE
Vec4f sl_lib_round(const Vec4f& x) {
  return sl_lib_floor(x + Vec4f(0.5f));
}


RTI_INLINE
float sl_lib_saturate(float x) {
  return clamp(x, 0.0f, 1.0f);
}


RTI_INLINE
Vec2f sl_lib_saturate(const Vec2f& x) {
  return sl_lib_clamp(x, 0.0f, 1.0f);
}


RTI_INLINE
Vec3f sl_lib_saturate(const Vec3f& x) {
  return sl_lib_clamp(x, 0.0f, 1.0f);
}


RTI_INLINE
Vec4f sl_lib_saturate(const Vec4f& x) {
  return sl_lib_clamp(x, 0.0f, 1.0f);
}


RTI_INLINE
int sl_lib_sign(int x) {
  return x < 0 ? -1 : x > 0 ? 1 : 0;
}


RTI_INLINE
float sl_lib_sign(float x) {
  return x < 0.0f ? -1.0f : x > 0.0f ? 1.0f : 0.0f;
}


RTI_INLINE
Vec2f sl_lib_sign(const Vec2f& x) {
  return Vec2f(sl_lib_sign(x[0]), sl_lib_sign(x[1]));
}


RTI_INLINE
Vec3f sl_lib_sign(const Vec3f& x) {
  return Vec3f(sl_lib_sign(x[0]), sl_lib_sign(x[1]), sl_lib_sign(x[2]));
}


RTI_INLINE
Vec4f sl_lib_sign(const Vec4f& x) {
  return Vec4f(sl_lib_sign(x[0]), sl_lib_sign(x[1]), sl_lib_sign(x[2]), sl_lib_sign(x[3]));
}


RTI_INLINE
float sl_lib_smoothstep(float x, float y, float t) {
  const float tmp = sl_lib_saturate((t - x) / (y - x));
  return tmp * tmp * (3.0f - 2.0f * tmp);
}


RTI_INLINE
Vec2f sl_lib_smoothstep(const Vec2f& x, const Vec2f& y, float t) {
  return Vec2f(sl_lib_smoothstep(x[0], y[0], t), sl_lib_smoothstep(x[1], y[1], t));
}


RTI_INLINE
Vec2f sl_lib_smoothstep(const Vec2f& x, const Vec2f& y, const Vec2f& t) {
  return Vec2f(sl_lib_smoothstep(x[0], y[0], t[0]), sl_lib_smoothstep(x[1], y[1], t[1]));
}


RTI_INLINE
Vec3f sl_lib_smoothstep(const Vec3f& x, const Vec3f& y, float t) {
  return Vec3f(sl_lib_smoothstep(x[0], y[0], t), sl_lib_smoothstep(x[1], y[1], t),
               sl_lib_smoothstep(x[2], y[2], t));
}


RTI_INLINE
Vec3f sl_lib_smoothstep(const Vec3f& x, const Vec3f& y, const Vec3f& t) {
  return Vec3f(sl_lib_smoothstep(x[0], y[0], t[0]), sl_lib_smoothstep(x[1], y[1], t[1]),
               sl_lib_smoothstep(x[2], y[2], t[2]));
}


RTI_INLINE
Vec4f sl_lib_smoothstep(const Vec4f& x, const Vec4f& y, float t) {
  return Vec4f(sl_lib_smoothstep(x[0], y[0], t), sl_lib_smoothstep(x[1], y[1], t),
               sl_lib_smoothstep(x[2], y[2], t), sl_lib_smoothstep(x[3], y[3], t));
}


RTI_INLINE
Vec4f sl_lib_smoothstep(const Vec4f& x, const Vec4f& y, const Vec4f& t) {
  return Vec4f(sl_lib_smoothstep(x[0], y[0], t[0]), sl_lib_smoothstep(x[1], y[1], t[1]),
               sl_lib_smoothstep(x[2], y[2], t[2]), sl_lib_smoothstep(x[3], y[3], t[3]));
}


RTI_INLINE
float sl_lib_step(float e, float t) {
  return t <= e ? 0.0f : 1.0f;
}


RTI_INLINE
Vec2f sl_lib_step(float e, const Vec2f& t) {
  return Vec2f(t[0] <= e ? 0.0f : 1.0f, t[1] <= e ? 0.0f : 1.0f);
}


RTI_INLINE
Vec2f sl_lib_step(const Vec2f& e, const Vec2f& t) {
  return Vec2f(t[0] <= e[0] ? 0.0f : 1.0f, t[1] <= e[1] ? 0.0f : 1.0f);
}


RTI_INLINE
Vec3f sl_lib_step(float e, const Vec3f& t) {
  return Vec3f(t[0] <= e ? 0.0f : 1.0f, t[1] <= e ? 0.0f : 1.0f, t[2] <= e ? 0.0f : 1.0f);
}


RTI_INLINE
Vec3f sl_lib_step(const Vec3f& e, const Vec3f& t) {
  return Vec3f(t[0] <= e[0] ? 0.0f : 1.0f, t[1] <= e[1] ? 0.0f : 1.0f, t[2] <= e[2] ? 0.0f : 1.0f);
}


RTI_INLINE
Vec4f sl_lib_step(float e, const Vec4f& t) {
  return Vec4f(t[0] <= e ? 0.0f : 1.0f, t[1] <= e ? 0.0f : 1.0f, t[2] <= e ? 0.0f : 1.0f,
               t[3] <= e ? 0.0f : 1.0f);
}


RTI_INLINE
Vec4f sl_lib_step(const Vec4f& e, const Vec4f& t) {
  return Vec4f(t[0] <= e[0] ? 0.0f : 1.0f, t[1] <= e[1] ? 0.0f : 1.0f, t[2] <= e[2] ? 0.0f : 1.0f,
               t[3] <= e[3] ? 0.0f : 1.0f);
}


//---------- Logical functions --------------------------------

RTI_INLINE
bool sl_lib_all(bool x) {
  return x;
}


RTI_INLINE
bool sl_lib_all(const Vec2b& x) {
  return x[0] && x[1];
}


RTI_INLINE
bool sl_lib_all(const Vec3b& x) {
  return x[0] && x[1] && x[2];
}


RTI_INLINE
bool sl_lib_all(const Vec4b& x) {
  return x[0] && x[1] && x[2] && x[3];
}


RTI_INLINE
bool sl_lib_any(bool x) {
  return x;
}


RTI_INLINE
bool sl_lib_any(const Vec2b& x) {
  return x[0] || x[1];
}


RTI_INLINE
bool sl_lib_any(const Vec3b& x) {
  return x[0] || x[1] || x[2];
}


RTI_INLINE
bool sl_lib_any(const Vec4b& x) {
  return x[0] || x[1] || x[2] || x[3];
}


RTI_INLINE
bool sl_lib_not(bool x) {
  return !x;
}


RTI_INLINE
Vec2b sl_lib_not(const Vec2b& x) {
  return !x;
}


RTI_INLINE
Vec3b sl_lib_not(const Vec3b& x) {
  return !x;
}


RTI_INLINE
Vec4b sl_lib_not(const Vec4b& x) {
  return !x;
}


//---------- Geometric functions ------------------------------

RTI_INLINE
Vec3f sl_lib_cross(const Vec3f& x, const Vec3f& y) {
  return Vec3f(x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]);
}


RTI_INLINE
float sl_lib_distance(float x, float y) {
  return fabsf(x - y);
}


RTI_INLINE
float sl_lib_distance(const Vec2f& x, const Vec2f& y) {
  const Vec2f fvDelta = x - y;
  return sl_lib_length(fvDelta);
}


RTI_INLINE
float sl_lib_distance(const Vec3f& x, const Vec3f& y) {
  const Vec3f fvDelta = x - y;
  return sl_lib_length(fvDelta);
}


RTI_INLINE
float sl_lib_distance(const Vec4f& x, const Vec4f& y) {
  const Vec4f fvDelta = x - y;
  return sl_lib_length(fvDelta);
}


RTI_INLINE
float sl_lib_dot(const Vec2f& x, const Vec2f& y) {
  return (x[0] * y[0]) + (x[1] * y[1]);
}


RTI_INLINE
float sl_lib_dot(const Vec3f& x, const Vec3f& y) {
  return (x[0] * y[0]) + (x[1] * y[1]) + (x[2] * y[2]);
}


RTI_INLINE
float sl_lib_dot(const Vec4f& x, const Vec4f& y) {
  return (x[0] * y[0]) + (x[1] * y[1]) + (x[2] * y[2]) + (x[3] * y[3]);
}

RTI_INLINE
Vec2f sl_lib_faceforward(const Vec2f& n, const Vec2f& i, const Vec2f& nref) {
  return -n * sl_lib_sign(sl_lib_dot(i, nref));
}


RTI_INLINE
Vec3f sl_lib_faceforward(const Vec3f& n, const Vec3f& i, const Vec3f& nref) {
  return -n * sl_lib_sign(sl_lib_dot(i, nref));
}


RTI_INLINE
Vec4f sl_lib_faceforward(const Vec4f& n, const Vec4f& i, const Vec4f& nref) {
  return -n * sl_lib_sign(sl_lib_dot(i, nref));
}


RTI_INLINE
float sl_lib_length(const Vec2f& x) {
  return sqrtf((x[0] * x[0]) + (x[1] * x[1]));
}


RTI_INLINE
float sl_lib_length(const Vec3f& x) {
  return sqrtf((x[0] * x[0]) + (x[1] * x[1]) + (x[2] * x[2]));
}


RTI_INLINE
float sl_lib_length(const Vec4f& x) {
  return sqrtf((x[0] * x[0]) + (x[1] * x[1]) + (x[2] * x[2]) + (x[3] * x[3]));
}


RTI_INLINE
Vec2f sl_lib_normalize(const Vec2f& x) {
  const float length = sl_lib_length(x);
  if (length > 0.0f) {
    return x / length;
  }
  return x;
}


RTI_INLINE
Vec3f sl_lib_normalize(const Vec3f& x) {
  const float length = sl_lib_length(x);
  if (length > 0.0f) {
    return x / length;
  }
  return x;
}


RTI_INLINE
Vec4f sl_lib_normalize(const Vec4f& x) {
  const float length = sl_lib_length(x);
  if (length > 0.0f) {
    return x / length;
  }
  return x;
}


RTI_INLINE
Vec3f sl_lib_reflect(const Vec3f& i, const Vec3f& n) {
  return i - 2.0f * sl_lib_dot(n, i) * n;
}


RTI_INLINE
Vec3f sl_lib_refract(const Vec3f& i, const Vec3f& n, float eta) {
  const float cosI  = fabsf(sl_lib_dot(i, n));
  const float cosSq = cosI * cosI;
  const float cosT2 = 1.0f + (eta * eta) * (cosSq - 1.0f);

  if (cosT2 <= RTI_EPSILON) {
    return Vec3f(0.0f);
  }

  const float tmp = eta * cosI - sqrtf(cosT2);

  Vec3f result;
  result[0] = eta * i[0] + (tmp * n[0]);
  result[1] = eta * i[1] + (tmp * n[1]);
  result[2] = eta * i[2] + (tmp * n[2]);

  return result;
}


//---------- Matrix functions ---------------------------------

RTI_INLINE
float sl_lib_determinant(const Mat2f& m) {
  return rti::determinant(m);
}


RTI_INLINE
float sl_lib_determinant(const Mat3f& m) {
  return rti::determinant(m);
}


RTI_INLINE
float sl_lib_determinant(const Mat4f& m) {
  return rti::determinant(m);
}


RTI_INLINE
Mat2f sl_lib_inverse(const Mat2f& m) {
  Mat2f r;
  rti::inverse(r, m);
  return r;
}


RTI_INLINE
Mat3f sl_lib_inverse(const Mat3f& m) {
  Mat3f r;
  rti::inverse(r, m);
  return r;
}


RTI_INLINE
Mat4f sl_lib_inverse(const Mat4f& m) {
  Mat4f r;
  rti::inverse(r, m);
  return r;
}


RTI_INLINE
Mat2f sl_lib_transpose(const Mat2f& m) {
  Mat2f r;
  rti::transpose(r, m);
  return r;
}


RTI_INLINE
Mat3f sl_lib_transpose(const Mat3f& m) {
  Mat3f r;
  rti::transpose(r, m);
  return r;
}


RTI_INLINE
Mat4f sl_lib_transpose(const Mat4f& m) {
  Mat4f r;
  rti::transpose(r, m);
  return r;
}


//---------- Shading functions --------------------------------

RTI_INLINE
float sl_lib_fresnel(const Vec3f& idir, const Vec3f& normal, float eta) {
  float e = 1.0f / eta;
  float c = -sl_lib_dot(idir, normal);
  float t = (e * e) + (c * c) - 1.0f;
  if (t < RTI_EPSILON) {  // TIR
    return 1.0f;
  }
  float g = sl_lib_sqrt(t);
  float a = (g - c) / (g + c);
  float b = (c * (g + c) - 1.0f) / (c * (g - c) + 1.0f);

  return sl_lib_saturate(0.5f * (a * a) * (1.0f + (b * b)));
}


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // DOXYGEN_SHOULD_SKIP_THIS

#endif  // __RTI_SHADER_STD_LIB_INL__
