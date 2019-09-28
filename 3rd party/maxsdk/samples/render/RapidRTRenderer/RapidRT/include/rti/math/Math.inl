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
* @file    Math.inl
* @brief   Inline section for Math.h
*
* @author  Henrik Edstrom
* @date    2008-01-24
*
*/

#ifndef __RTI_MATH_INL__
#define __RTI_MATH_INL__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// C++ / STL
#include <math.h>
#include <string.h>


#ifndef RTI_INLINE
  #if defined(_WIN32)
    #define RTI_INLINE __forceinline
  #elif defined(__clang__)
    #define RTI_INLINE __inline__ __attribute__((always_inline))
  #elif defined(__linux__)
    #define RTI_INLINE inline
  #else
    #define RTI_INLINE inline
  #endif
#endif

#ifdef __INTEL_COMPILER
#pragma warning(disable : 522)   // ICC: function redeclared inline
#pragma warning(disable : 981)   // ICC: operands are evaluated in unspecified order
#pragma warning(disable : 1572)  // ICC: floating-point equality
#endif // __INTEL_COMPILER

///////////////////////////////////////////////////////////////////////////////
// rti ////////////////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- rti::Vec2f ---------------------------------------

RTI_INLINE
Vec2f::Vec2f(const float* p) : x(p[0]), y(p[1]) {}


RTI_INLINE
Vec2f::Vec2f(float s) : x(s), y(s) {}


RTI_INLINE
Vec2f::Vec2f(float _x, float _y) : x(_x), y(_y) {}


RTI_INLINE
Vec2f Vec2f::operator+(const Vec2f& rhs) const {
  return Vec2f(x + rhs.x, y + rhs.y);
}


RTI_INLINE
Vec2f Vec2f::operator-(const Vec2f& rhs) const {
  return Vec2f(x - rhs.x, y - rhs.y);
}


RTI_INLINE
Vec2f Vec2f::operator*(const Vec2f& rhs) const {
  return Vec2f(x * rhs.x, y * rhs.y);
}


RTI_INLINE
Vec2f Vec2f::operator/(const Vec2f& rhs) const {
  return Vec2f(x / rhs.x, y / rhs.y);
}


RTI_INLINE
Vec2f Vec2f::operator*(float rhs) const {
  return Vec2f(x * rhs, y * rhs);
}


RTI_INLINE
Vec2f Vec2f::operator/(float rhs) const {
  const float r = 1.0f / rhs;
  return Vec2f(x * r, y * r);
}


RTI_INLINE
Vec2f operator*(float lhs, const Vec2f& rhs) {
  return Vec2f(lhs * rhs.x, lhs * rhs.y);
}


RTI_INLINE
Vec2f& Vec2f::operator+=(const Vec2f& rhs) {
  x += rhs.x;
  y += rhs.y;
  return *this;
}


RTI_INLINE
Vec2f& Vec2f::operator-=(const Vec2f& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  return *this;
}


RTI_INLINE
Vec2f& Vec2f::operator*=(const Vec2f& rhs) {
  x *= rhs.x;
  y *= rhs.y;
  return *this;
}


RTI_INLINE
Vec2f& Vec2f::operator/=(const Vec2f& rhs) {
  x /= rhs.x;
  y /= rhs.y;
  return *this;
}


RTI_INLINE
Vec2f& Vec2f::operator*=(float rhs) {
  x *= rhs;
  y *= rhs;
  return *this;
}


RTI_INLINE
Vec2f& Vec2f::operator/=(float rhs) {
  const float r = 1.0f / rhs;
  x *= r;
  y *= r;
  return *this;
}


RTI_INLINE
Vec2f Vec2f::operator-() const {
  return Vec2f(-x, -y);
}


RTI_INLINE
bool Vec2f::operator==(const Vec2f& rhs) const {
  return (x == rhs.x) && (y == rhs.y);
}


RTI_INLINE
bool Vec2f::operator!=(const Vec2f& rhs) const {
  return (x != rhs.x) || (y != rhs.y);
}


RTI_INLINE
Vec2f::operator float*() {
  static_assert(sizeof(Vec2f) == 2 * sizeof(float), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec2f::operator const float*() const {
  static_assert(sizeof(Vec2f) == 2 * sizeof(float), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec3f ---------------------------------------

RTI_INLINE
Vec3f::Vec3f(const float* p) : x(p[0]), y(p[1]), z(p[2]) {}


RTI_INLINE
Vec3f::Vec3f(float s) : x(s), y(s), z(s) {}


RTI_INLINE
Vec3f::Vec3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}


RTI_INLINE
Vec3f Vec3f::operator+(const Vec3f& rhs) const {
  return Vec3f(x + rhs.x, y + rhs.y, z + rhs.z);
}


RTI_INLINE
Vec3f Vec3f::operator-(const Vec3f& rhs) const {
  return Vec3f(x - rhs.x, y - rhs.y, z - rhs.z);
}


RTI_INLINE
Vec3f Vec3f::operator*(const Vec3f& rhs) const {
  return Vec3f(x * rhs.x, y * rhs.y, z * rhs.z);
}


RTI_INLINE
Vec3f Vec3f::operator/(const Vec3f& rhs) const {
  return Vec3f(x / rhs.x, y / rhs.y, z / rhs.z);
}


RTI_INLINE
Vec3f Vec3f::operator*(float rhs) const {
  return Vec3f(x * rhs, y * rhs, z * rhs);
}


RTI_INLINE
Vec3f Vec3f::operator/(float rhs) const {
  const float r = 1.0f / rhs;
  return Vec3f(x * r, y * r, z * r);
}


RTI_INLINE
Vec3f operator*(float lhs, const Vec3f& rhs) {
  return Vec3f(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
}


RTI_INLINE
Vec3f& Vec3f::operator+=(const Vec3f& rhs) {
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
  return *this;
}


RTI_INLINE
Vec3f& Vec3f::operator-=(const Vec3f& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;
  return *this;
}


RTI_INLINE
Vec3f& Vec3f::operator*=(const Vec3f& rhs) {
  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;
  return *this;
}


RTI_INLINE
Vec3f& Vec3f::operator/=(const Vec3f& rhs) {
  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;
  return *this;
}


RTI_INLINE
Vec3f& Vec3f::operator*=(float rhs) {
  x *= rhs;
  y *= rhs;
  z *= rhs;
  return *this;
}


RTI_INLINE
Vec3f& Vec3f::operator/=(float rhs) {
  const float r = 1.0f / rhs;
  x *= r;
  y *= r;
  z *= r;
  return *this;
}


RTI_INLINE
Vec3f Vec3f::operator-() const {
  return Vec3f(-x, -y, -z);
}


RTI_INLINE
Vec3f::operator float*() {
  static_assert(sizeof(Vec3f) == 3 * sizeof(float), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec3f::operator const float*() const {
  static_assert(sizeof(Vec3f) == 3 * sizeof(float), "Vector is not packed.");
  return &x;
}


RTI_INLINE
bool Vec3f::operator==(const Vec3f& rhs) const {
  return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
}


RTI_INLINE
bool Vec3f::operator!=(const Vec3f& rhs) const {
  return (x != rhs.x) || (y != rhs.y) || (z != rhs.z);
}


//---------- rti::Vec4f ---------------------------------------

RTI_INLINE
Vec4f::Vec4f(const float* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}


RTI_INLINE
Vec4f::Vec4f(float s) : x(s), y(s), z(s), w(s) {}


RTI_INLINE
Vec4f::Vec4f(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}


RTI_INLINE
Vec4f Vec4f::operator+(const Vec4f& rhs) const {
  return Vec4f(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}


RTI_INLINE
Vec4f Vec4f::operator-(const Vec4f& rhs) const {
  return Vec4f(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}


RTI_INLINE
Vec4f Vec4f::operator*(const Vec4f& rhs) const {
  return Vec4f(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}


RTI_INLINE
Vec4f Vec4f::operator/(const Vec4f& rhs) const {
  return Vec4f(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}


RTI_INLINE
Vec4f Vec4f::operator*(float rhs) const {
  return Vec4f(x * rhs, y * rhs, z * rhs, w * rhs);
}


RTI_INLINE
Vec4f Vec4f::operator/(float rhs) const {
  const float r = 1.0f / rhs;
  return Vec4f(x * r, y * r, z * r, w * r);
}


RTI_INLINE
Vec4f operator*(float lhs, const Vec4f& rhs) {
  return Vec4f(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w);
}


RTI_INLINE
Vec4f& Vec4f::operator+=(const Vec4f& rhs) {
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
  w += rhs.w;
  return *this;
}


RTI_INLINE
Vec4f& Vec4f::operator-=(const Vec4f& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;
  w -= rhs.w;
  return *this;
}


RTI_INLINE
Vec4f& Vec4f::operator*=(const Vec4f& rhs) {
  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;
  w *= rhs.w;
  return *this;
}


RTI_INLINE
Vec4f& Vec4f::operator/=(const Vec4f& rhs) {
  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;
  w /= rhs.w;
  return *this;
}


RTI_INLINE
Vec4f& Vec4f::operator*=(float rhs) {
  x *= rhs;
  y *= rhs;
  z *= rhs;
  w *= rhs;
  return *this;
}


RTI_INLINE
Vec4f& Vec4f::operator/=(float rhs) {
  const float r = 1.0f / rhs;
  x *= r;
  y *= r;
  z *= r;
  w *= r;
  return *this;
}


RTI_INLINE
Vec4f Vec4f::operator-() const {
  return Vec4f(-x, -y, -z, -w);
}


RTI_INLINE
bool Vec4f::operator==(const Vec4f& rhs) const {
  return (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w);
}


RTI_INLINE
bool Vec4f::operator!=(const Vec4f& rhs) const {
  return (x != rhs.x) || (y != rhs.y) || (z != rhs.z) || (w != rhs.w);
}


RTI_INLINE
Vec4f::operator float*() {
  static_assert(sizeof(Vec4f) == 4 * sizeof(float), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec4f::operator const float*() const {
  static_assert(sizeof(Vec4f) == 4 * sizeof(float), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec2i ---------------------------------------

RTI_INLINE
Vec2i::Vec2i(const int* p) : x(p[0]), y(p[1]) {}


RTI_INLINE
Vec2i::Vec2i(int n) : x(n), y(n) {}


RTI_INLINE
Vec2i::Vec2i(int _x, int _y) : x(_x), y(_y) {}


RTI_INLINE
Vec2i Vec2i::operator+(const Vec2i& rhs) const {
  return Vec2i(x + rhs.x, y + rhs.y);
}


RTI_INLINE
Vec2i Vec2i::operator-(const Vec2i& rhs) const {
  return Vec2i(x - rhs.x, y - rhs.y);
}


RTI_INLINE
Vec2i Vec2i::operator*(const Vec2i& rhs) const {
  return Vec2i(x * rhs.x, y * rhs.y);
}


RTI_INLINE
Vec2i Vec2i::operator/(const Vec2i& rhs) const {
  return Vec2i(x / rhs.x, y / rhs.y);
}


RTI_INLINE
Vec2i Vec2i::operator*(int rhs) const {
  return Vec2i(x * rhs, y * rhs);
}


RTI_INLINE
Vec2i Vec2i::operator/(int rhs) const {
  return Vec2i(x / rhs, y / rhs);
}


RTI_INLINE
Vec2i operator*(int lhs, const Vec2i& rhs) {
  return Vec2i(lhs * rhs.x, lhs * rhs.y);
}


RTI_INLINE
Vec2i& Vec2i::operator+=(const Vec2i& rhs) {
  x += rhs.x;
  y += rhs.y;
  return *this;
}


RTI_INLINE
Vec2i& Vec2i::operator-=(const Vec2i& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  return *this;
}


RTI_INLINE
Vec2i& Vec2i::operator*=(const Vec2i& rhs) {
  x *= rhs.x;
  y *= rhs.y;
  return *this;
}


RTI_INLINE
Vec2i& Vec2i::operator/=(const Vec2i& rhs) {
  x /= rhs.x;
  y /= rhs.y;
  return *this;
}


RTI_INLINE
Vec2i& Vec2i::operator*=(int rhs) {
  x *= rhs;
  y *= rhs;
  return *this;
}


RTI_INLINE
Vec2i& Vec2i::operator/=(int rhs) {
  x /= rhs;
  y /= rhs;
  return *this;
}


RTI_INLINE
Vec2i Vec2i::operator-() const {
  return Vec2i(-x, -y);
}


RTI_INLINE
bool Vec2i::operator==(const Vec2i& rhs) const {
  return (x == rhs.x) && (y == rhs.y);
}


RTI_INLINE
bool Vec2i::operator!=(const Vec2i& rhs) const {
  return (x != rhs.x) || (y != rhs.y);
}


RTI_INLINE
Vec2i::operator int*() {
  static_assert(sizeof(Vec2i) == 2 * sizeof(int), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec2i::operator const int*() const {
  static_assert(sizeof(Vec2i) == 2 * sizeof(int), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec3i ---------------------------------------

RTI_INLINE
Vec3i::Vec3i(const int* p) : x(p[0]), y(p[1]), z(p[2]) {}


RTI_INLINE
Vec3i::Vec3i(int n) : x(n), y(n), z(n) {}


RTI_INLINE
Vec3i::Vec3i(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}


RTI_INLINE
Vec3i Vec3i::operator+(const Vec3i& rhs) const {
  return Vec3i(x + rhs.x, y + rhs.y, z + rhs.z);
}


RTI_INLINE
Vec3i Vec3i::operator-(const Vec3i& rhs) const {
  return Vec3i(x - rhs.x, y - rhs.y, z - rhs.z);
}


RTI_INLINE
Vec3i Vec3i::operator*(const Vec3i& rhs) const {
  return Vec3i(x * rhs.x, y * rhs.y, z * rhs.z);
}


RTI_INLINE
Vec3i Vec3i::operator/(const Vec3i& rhs) const {
  return Vec3i(x / rhs.x, y / rhs.y, z / rhs.z);
}


RTI_INLINE
Vec3i Vec3i::operator*(int rhs) const {
  return Vec3i(x * rhs, y * rhs, z * rhs);
}


RTI_INLINE
Vec3i Vec3i::operator/(int rhs) const {
  return Vec3i(x / rhs, y / rhs, z / rhs);
}


RTI_INLINE
Vec3i operator*(int lhs, const Vec3i& rhs) {
  return Vec3i(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
}


RTI_INLINE
Vec3i& Vec3i::operator+=(const Vec3i& rhs) {
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
  return *this;
}


RTI_INLINE
Vec3i& Vec3i::operator-=(const Vec3i& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;
  return *this;
}


RTI_INLINE
Vec3i& Vec3i::operator*=(const Vec3i& rhs) {
  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;
  return *this;
}


RTI_INLINE
Vec3i& Vec3i::operator/=(const Vec3i& rhs) {
  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;
  return *this;
}


RTI_INLINE
Vec3i& Vec3i::operator*=(int rhs) {
  x *= rhs;
  y *= rhs;
  z *= rhs;
  return *this;
}


RTI_INLINE
Vec3i& Vec3i::operator/=(int rhs) {
  x /= rhs;
  y /= rhs;
  z /= rhs;
  return *this;
}


RTI_INLINE
Vec3i Vec3i::operator-() const {
  return Vec3i(-x, -y, -z);
}


RTI_INLINE
bool Vec3i::operator==(const Vec3i& rhs) const {
  return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
}


RTI_INLINE
bool Vec3i::operator!=(const Vec3i& rhs) const {
  return (x != rhs.x) || (y != rhs.y) || (z != rhs.z);
}


RTI_INLINE
Vec3i::operator int*() {
  static_assert(sizeof(Vec3i) == 3 * sizeof(int), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec3i::operator const int*() const {
  static_assert(sizeof(Vec3i) == 3 * sizeof(int), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec4i ---------------------------------------

RTI_INLINE
Vec4i::Vec4i(const int* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}


RTI_INLINE
Vec4i::Vec4i(int n) : x(n), y(n), z(n), w(n) {}


RTI_INLINE
Vec4i::Vec4i(int _x, int _y, int _z, int _w) : x(_x), y(_y), z(_z), w(_w) {}


RTI_INLINE
Vec4i Vec4i::operator+(const Vec4i& rhs) const {
  return Vec4i(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}


RTI_INLINE
Vec4i Vec4i::operator-(const Vec4i& rhs) const {
  return Vec4i(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}


RTI_INLINE
Vec4i Vec4i::operator*(const Vec4i& rhs) const {
  return Vec4i(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}


RTI_INLINE
Vec4i Vec4i::operator/(const Vec4i& rhs) const {
  return Vec4i(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}


RTI_INLINE
Vec4i Vec4i::operator*(int rhs) const {
  return Vec4i(x * rhs, y * rhs, z * rhs, w * rhs);
}


RTI_INLINE
Vec4i Vec4i::operator/(int rhs) const {
  return Vec4i(x / rhs, y / rhs, z / rhs, w / rhs);
}


RTI_INLINE
Vec4i operator*(int lhs, const Vec4i& rhs) {
  return Vec4i(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w);
}


RTI_INLINE
Vec4i& Vec4i::operator+=(const Vec4i& rhs) {
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
  w += rhs.w;
  return *this;
}


RTI_INLINE
Vec4i& Vec4i::operator-=(const Vec4i& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;
  w -= rhs.w;
  return *this;
}


RTI_INLINE
Vec4i& Vec4i::operator*=(const Vec4i& rhs) {
  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;
  w *= rhs.w;
  return *this;
}


RTI_INLINE
Vec4i& Vec4i::operator/=(const Vec4i& rhs) {
  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;
  w /= rhs.w;
  return *this;
}


RTI_INLINE
Vec4i& Vec4i::operator*=(int rhs) {
  x *= rhs;
  y *= rhs;
  z *= rhs;
  w *= rhs;
  return *this;
}


RTI_INLINE
Vec4i& Vec4i::operator/=(int rhs) {
  x /= rhs;
  y /= rhs;
  z /= rhs;
  w /= rhs;
  return *this;
}


RTI_INLINE
Vec4i Vec4i::operator-() const {
  return Vec4i(-x, -y, -z, -w);
}


RTI_INLINE
bool Vec4i::operator==(const Vec4i& rhs) const {
  return (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w);
}


RTI_INLINE
bool Vec4i::operator!=(const Vec4i& rhs) const {
  return (x != rhs.x) || (y != rhs.y) || (z != rhs.z) || (w != rhs.w);
}


RTI_INLINE
Vec4i::operator int*() {
  static_assert(sizeof(Vec4i) == 4 * sizeof(int), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec4i::operator const int*() const {
  static_assert(sizeof(Vec4i) == 4 * sizeof(int), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec2b ---------------------------------------

RTI_INLINE
Vec2b::Vec2b(const bool* p) : x(p[0]), y(p[1]) {}


RTI_INLINE
Vec2b::Vec2b(bool b) : x(b), y(b) {}


RTI_INLINE
Vec2b::Vec2b(bool _x, bool _y) : x(_x), y(_y) {}


RTI_INLINE
Vec2b Vec2b::operator!() const {
  return Vec2b(!x, !y);
}


RTI_INLINE
bool Vec2b::operator==(const Vec2b& rhs) const {
  return (x == rhs.x) && (y == rhs.y);
}


RTI_INLINE
bool Vec2b::operator!=(const Vec2b& rhs) const {
  return (x != rhs.x) || (y != rhs.y);
}


RTI_INLINE
Vec2b::operator bool*() {
  static_assert(sizeof(Vec2b) == 2 * sizeof(bool), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec2b::operator const bool*() const {
  static_assert(sizeof(Vec2b) == 2 * sizeof(bool), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec3b ---------------------------------------

RTI_INLINE
Vec3b::Vec3b(const bool* p) : x(p[0]), y(p[1]), z(p[2]) {}


RTI_INLINE
Vec3b::Vec3b(bool b) : x(b), y(b), z(b) {}


RTI_INLINE
Vec3b::Vec3b(bool _x, bool _y, bool _z) : x(_x), y(_y), z(_z) {}


RTI_INLINE
Vec3b Vec3b::operator!() const {
  return Vec3b(!x, !y, !z);
}


RTI_INLINE
bool Vec3b::operator==(const Vec3b& rhs) const {
  return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
}


RTI_INLINE
bool Vec3b::operator!=(const Vec3b& rhs) const {
  return (x != rhs.x) || (y != rhs.y) || (z != rhs.z);
}


RTI_INLINE
Vec3b::operator bool*() {
  static_assert(sizeof(Vec3b) == 3 * sizeof(bool), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec3b::operator const bool*() const {
  static_assert(sizeof(Vec3b) == 3 * sizeof(bool), "Vector is not packed.");
  return &x;
}


//---------- rti::Vec4b ---------------------------------------

RTI_INLINE
Vec4b::Vec4b(const bool* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}


RTI_INLINE
Vec4b::Vec4b(bool b) : x(b), y(b), z(b), w(b) {}


RTI_INLINE
Vec4b::Vec4b(bool _x, bool _y, bool _z, bool _w) : x(_x), y(_y), z(_z), w(_w) {}


RTI_INLINE
Vec4b Vec4b::operator!() const {
  return Vec4b(!x, !y, !z, !w);
}


RTI_INLINE
bool Vec4b::operator==(const Vec4b& rhs) const {
  return (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w);
}


RTI_INLINE
bool Vec4b::operator!=(const Vec4b& rhs) const {
  return (x != rhs.x) || (y != rhs.y) || (z != rhs.z) || (w != rhs.w);
}


RTI_INLINE
Vec4b::operator bool*() {
  static_assert(sizeof(Vec4b) == 4 * sizeof(bool), "Vector is not packed.");
  return &x;
}


RTI_INLINE
Vec4b::operator const bool*() const {
  static_assert(sizeof(Vec4b) == 4 * sizeof(bool), "Vector is not packed.");
  return &x;
}


//---------- rti::Mat2f ---------------------------------------

RTI_INLINE
Mat2f::Mat2f(const float* p) {
  
  m[0][0] = p[0];
  m[0][1] = p[1];

  m[1][0] = p[2];
  m[1][1] = p[3];
}


RTI_INLINE
Mat2f::Mat2f(float s) {
  
  m[0][0] = s;
  m[0][1] = 0.0f;
  
  m[1][0] = 0.0f;
  m[1][1] = s;
}


RTI_INLINE
Mat2f::Mat2f(const Mat3f& m3) {
  
  m[0][0] = m3[0][0];
  m[0][1] = m3[0][1];
  
  m[1][0] = m3[1][0];
  m[1][1] = m3[1][1];
}


RTI_INLINE
Mat2f::Mat2f(const Mat4f& m4) {
  
  m[0][0] = m4[0][0];
  m[0][1] = m4[0][1];
  
  m[1][0] = m4[1][0];
  m[1][1] = m4[1][1];
}


RTI_INLINE
Mat2f::Mat2f(Vec2f col0, Vec2f col1) {
  m[0] = col0;
  m[1] = col1;
}


RTI_INLINE
Mat2f::Mat2f(float c0r0, float c0r1, 
             float c1r0, float c1r1) {

  m[0][0] = c0r0;
  m[0][1] = c0r1;

  m[1][0] = c1r0;
  m[1][1] = c1r1;
}


RTI_INLINE
Mat2f::Mat2f(const Mat2f& rhs) {
  *this = rhs;
}


RTI_INLINE
Mat2f& Mat2f::operator=(const Mat2f& rhs) {
  m[0] = rhs[0];
  m[1] = rhs[1];
  return *this;
}


RTI_INLINE
Vec2f& Mat2f::operator[](int i) {
  return m[i];
}


RTI_INLINE
const Vec2f& Mat2f::operator[](int i) const {
  return m[i];
}


RTI_INLINE
Mat2f Mat2f::operator*(const Mat2f& rhs) const {
  Mat2f res;

  res[0][0] = m[0][0] * rhs[0][0] + m[1][0] * rhs[0][1];
  res[0][1] = m[0][1] * rhs[0][0] + m[1][1] * rhs[0][1];

  res[1][0] = m[0][0] * rhs[1][0] + m[1][0] * rhs[1][1];
  res[1][1] = m[0][1] * rhs[1][0] + m[1][1] * rhs[1][1];

  return res;
}


RTI_INLINE
Vec2f Mat2f::operator*(const Vec2f& v) const {
  return Vec2f(m[0][0] * v.x + m[1][0] * v.y, 
               m[0][1] * v.x + m[1][1] * v.y);
}


RTI_INLINE
Mat2f Mat2f::operator+(const Mat2f& rhs) const {
  return Mat2f(m[0][0] + rhs[0][0], m[0][1] + rhs[0][1], 
               m[1][0] + rhs[1][0], m[1][1] + rhs[1][1]);
}


RTI_INLINE
Mat2f Mat2f::operator-(const Mat2f& rhs) const {
  return Mat2f(m[0][0] - rhs[0][0], m[0][1] - rhs[0][1], 
               m[1][0] - rhs[1][0], m[1][1] - rhs[1][1]);
}


RTI_INLINE
Mat2f Mat2f::operator*(float rhs) const {
  return Mat2f(m[0][0] * rhs, m[0][1] * rhs, 
               m[1][0] * rhs, m[1][1] * rhs);
}


RTI_INLINE
Mat2f Mat2f::operator/(float rhs) const {
  const float r = 1.0f / rhs;
  return Mat2f(m[0][0] * r, m[0][1] * r, 
               m[1][0] * r, m[1][1] * r);
}


RTI_INLINE
Mat2f operator*(float lhs, const Mat2f& rhs) {
  return rhs * lhs;
}


RTI_INLINE
Vec2f operator*(const Vec2f& v, const Mat2f& m) {
  return Vec2f(v.x * m[0][0] + v.y * m[0][1], 
               v.x * m[1][0] + v.y * m[1][1]);
}


RTI_INLINE
Vec2f& operator*=(Vec2f& lhs, const Mat2f& rhs) {
  lhs = lhs * rhs;
  return lhs;
}


RTI_INLINE
Mat2f& Mat2f::operator*=(const Mat2f& rhs) {
  Mat2f tmp      = (*this) * rhs;
  return (*this) = tmp;
}


RTI_INLINE
Mat2f& Mat2f::operator+=(const Mat2f& rhs) {
  m[0][0] += rhs[0][0];
  m[0][1] += rhs[0][1];
  m[1][0] += rhs[1][0];
  m[1][1] += rhs[1][1];
  return *this;
}


RTI_INLINE
Mat2f& Mat2f::operator-=(const Mat2f& rhs) {
  m[0][0] -= rhs[0][0];
  m[0][1] -= rhs[0][1];
  m[1][0] -= rhs[1][0];
  m[1][1] -= rhs[1][1];
  return *this;
}


RTI_INLINE
Mat2f& Mat2f::operator*=(float rhs) {
  m[0][0] *= rhs;
  m[0][1] *= rhs;
  m[1][0] *= rhs;
  m[1][1] *= rhs;
  return *this;
}


RTI_INLINE
Mat2f& Mat2f::operator/=(float rhs) {
  const float r = 1.0f / rhs;
  m[0][0] *= r;
  m[0][1] *= r;
  m[1][0] *= r;
  m[1][1] *= r;
  return *this;
}


RTI_INLINE
Mat2f Mat2f::operator-() const {
  return Mat2f(-m[0][0], -m[0][1], 
               -m[1][0], -m[1][1]);
}


RTI_INLINE
bool Mat2f::operator==(const Mat2f& rhs) const {
  return memcmp(this, &rhs, sizeof(Mat2f)) == 0;
}


RTI_INLINE
bool Mat2f::operator!=(const Mat2f& rhs) const {
  return memcmp(this, &rhs, sizeof(Mat2f)) != 0;
}


RTI_INLINE
Mat2f::operator float*() {
  return (float*)m[0];
}


RTI_INLINE
Mat2f::operator const float*() const {
  return (const float*)m[0];
}


//---------- rti::Mat3f ---------------------------------------

RTI_INLINE
Mat3f::Mat3f(const float* p) {
  
  m[0][0] = p[0];
  m[0][1] = p[1];
  m[0][2] = p[2];
  
  m[1][0] = p[3];
  m[1][1] = p[4];
  m[1][2] = p[5];
  
  m[2][0] = p[6];
  m[2][1] = p[7];
  m[2][2] = p[8];
}


RTI_INLINE
Mat3f::Mat3f(float s) {
  
  m[0][0] = s;
  m[0][1] = 0.0f;
  m[0][2] = 0.0f;
  
  m[1][0] = 0.0f;
  m[1][1] = s;
  m[1][2] = 0.0f;
  
  m[2][0] = 0.0f;
  m[2][1] = 0.0f;
  m[2][2] = s;
}


RTI_INLINE
Mat3f::Mat3f(const Mat4f& m4) {
  
  m[0][0] = m4[0][0];
  m[0][1] = m4[0][1];
  m[0][2] = m4[0][2];
  
  m[1][0] = m4[1][0];
  m[1][1] = m4[1][1];
  m[1][2] = m4[1][2];
  
  m[2][0] = m4[2][0];
  m[2][1] = m4[2][1];
  m[2][2] = m4[2][2];
}


RTI_INLINE
Mat3f::Mat3f(Vec3f col0, Vec3f col1, Vec3f col2) {
  m[0] = col0;
  m[1] = col1;
  m[2] = col2;
}


RTI_INLINE
Mat3f::Mat3f(float c0r0, float c0r1, float c0r2,
             float c1r0, float c1r1, float c1r2,
             float c2r0, float c2r1, float c2r2) {

  m[0][0] = c0r0;
  m[0][1] = c0r1;
  m[0][2] = c0r2;

  m[1][0] = c1r0;
  m[1][1] = c1r1;
  m[1][2] = c1r2;

  m[2][0] = c2r0;
  m[2][1] = c2r1;
  m[2][2] = c2r2;
}


RTI_INLINE
Mat3f::Mat3f(const Mat3f& rhs) {
  *this = rhs;
}


RTI_INLINE
Mat3f& Mat3f::operator=(const Mat3f& rhs) {
  m[0] = rhs[0];
  m[1] = rhs[1];
  m[2] = rhs[2];
  return *this;
}



RTI_INLINE
Vec3f& Mat3f::operator[](int i) {
  return m[i];
}


RTI_INLINE
const Vec3f& Mat3f::operator[](int i) const {
  return m[i];
}


RTI_INLINE
Mat3f Mat3f::operator*(const Mat3f& rhs) const {
  Mat3f res;

  res[0][0] = m[0][0] * rhs[0][0] + m[1][0] * rhs[0][1] + m[2][0] * rhs[0][2];
  res[0][1] = m[0][1] * rhs[0][0] + m[1][1] * rhs[0][1] + m[2][1] * rhs[0][2];
  res[0][2] = m[0][2] * rhs[0][0] + m[1][2] * rhs[0][1] + m[2][2] * rhs[0][2];

  res[1][0] = m[0][0] * rhs[1][0] + m[1][0] * rhs[1][1] + m[2][0] * rhs[1][2];
  res[1][1] = m[0][1] * rhs[1][0] + m[1][1] * rhs[1][1] + m[2][1] * rhs[1][2];
  res[1][2] = m[0][2] * rhs[1][0] + m[1][2] * rhs[1][1] + m[2][2] * rhs[1][2];

  res[2][0] = m[0][0] * rhs[2][0] + m[1][0] * rhs[2][1] + m[2][0] * rhs[2][2];
  res[2][1] = m[0][1] * rhs[2][0] + m[1][1] * rhs[2][1] + m[2][1] * rhs[2][2];
  res[2][2] = m[0][2] * rhs[2][0] + m[1][2] * rhs[2][1] + m[2][2] * rhs[2][2];

  return res;
}


RTI_INLINE
Vec3f Mat3f::operator*(const Vec3f& v) const {
  return Vec3f(m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
               m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
               m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z);

}


RTI_INLINE
Mat3f Mat3f::operator+(const Mat3f& rhs) const {
  return Mat3f(m[0][0] + rhs[0][0], m[0][1] + rhs[0][1], m[0][2] + rhs[0][2], 
               m[1][0] + rhs[1][0], m[1][1] + rhs[1][1], m[1][2] + rhs[1][2], 
               m[2][0] + rhs[2][0], m[2][1] + rhs[2][1], m[2][2] + rhs[2][2]);
}


RTI_INLINE
Mat3f Mat3f::operator-(const Mat3f& rhs) const {
  return Mat3f(m[0][0] - rhs[0][0], m[0][1] - rhs[0][1], m[0][2] - rhs[0][2], 
               m[1][0] - rhs[1][0], m[1][1] - rhs[1][1], m[1][2] - rhs[1][2], 
               m[2][0] - rhs[2][0], m[2][1] - rhs[2][1], m[2][2] - rhs[2][2]);
}


RTI_INLINE
Mat3f Mat3f::operator*(float rhs) const {
  return Mat3f(m[0][0] * rhs, m[0][1] * rhs, m[0][2] * rhs, 
               m[1][0] * rhs, m[1][1] * rhs, m[1][2] * rhs, 
               m[2][0] * rhs, m[2][1] * rhs, m[2][2] * rhs);
}


RTI_INLINE
Mat3f Mat3f::operator/(float rhs) const {
  const float r = 1.0f / rhs;
  return Mat3f(m[0][0] * r, m[0][1] * r, m[0][2] * r, 
               m[1][0] * r, m[1][1] * r, m[1][2] * r,
               m[2][0] * r, m[2][1] * r, m[2][2] * r);
}


RTI_INLINE
Mat3f operator*(float lhs, const Mat3f& rhs) {
  return rhs * lhs;
}


RTI_INLINE
Vec3f operator*(const Vec3f& v, const Mat3f& m) {
  return Vec3f(m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
               m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
               m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
}


RTI_INLINE
Vec3f& operator*=(Vec3f& lhs, const Mat3f& rhs) {
  lhs = lhs * rhs;
  return lhs;
}


RTI_INLINE
Mat3f& Mat3f::operator*=(const Mat3f& rhs) {
  Mat3f tmp      = (*this) * rhs;
  return (*this) = tmp;
}


RTI_INLINE
Mat3f& Mat3f::operator+=(const Mat3f& rhs) {
  
  m[0][0] += rhs[0][0];
  m[0][1] += rhs[0][1];
  m[0][2] += rhs[0][2];
  
  m[1][0] += rhs[1][0];
  m[1][1] += rhs[1][1];
  m[1][2] += rhs[1][2];
  
  m[2][0] += rhs[2][0];
  m[2][1] += rhs[2][1];
  m[2][2] += rhs[2][2];
  
  return *this;
}


RTI_INLINE
Mat3f& Mat3f::operator-=(const Mat3f& rhs) {
  
  m[0][0] -= rhs[0][0];
  m[0][1] -= rhs[0][1];
  m[0][2] -= rhs[0][2];
  
  m[1][0] -= rhs[1][0];
  m[1][1] -= rhs[1][1];
  m[1][2] -= rhs[1][2];
  
  m[2][0] -= rhs[2][0];
  m[2][1] -= rhs[2][1];
  m[2][2] -= rhs[2][2];
  
  return *this;
}


RTI_INLINE
Mat3f& Mat3f::operator*=(float rhs) {
  
  m[0][0] *= rhs;
  m[0][1] *= rhs;
  m[0][2] *= rhs;
  
  m[1][0] *= rhs;
  m[1][1] *= rhs;
  m[1][2] *= rhs;
  
  m[2][0] *= rhs;
  m[2][1] *= rhs;
  m[2][2] *= rhs;
  
  return *this;
}


RTI_INLINE
Mat3f& Mat3f::operator/=(float rhs) {
  const float r = 1.0f / rhs;
  
  m[0][0] *= r;
  m[0][1] *= r;
  m[0][2] *= r;
  
  m[1][0] *= r;
  m[1][1] *= r;
  m[1][2] *= r;
  
  m[2][0] *= r;
  m[2][1] *= r;
  m[2][2] *= r;
  
  return *this;
}


RTI_INLINE
Mat3f Mat3f::operator-() const {
  return Mat3f(-m[0][0], -m[0][1], -m[0][2], 
               -m[1][0], -m[1][1], -m[1][2], 
               -m[2][0], -m[2][1], -m[2][2]);
}


RTI_INLINE
bool Mat3f::operator==(const Mat3f& rhs) const {
  return memcmp(this, &rhs, sizeof(Mat3f)) == 0;
}


RTI_INLINE
bool Mat3f::operator!=(const Mat3f& rhs) const {
  return memcmp(this, &rhs, sizeof(Mat3f)) != 0;
}


RTI_INLINE
Mat3f::operator float*() {
  return (float*)m[0];
}


RTI_INLINE
Mat3f::operator const float*() const {
  return (const float*)m[0];
}


//---------- rti::Mat4f ---------------------------------------

RTI_INLINE
Mat4f::Mat4f(const float* p) {
  
  m[0][0] = p[0];
  m[0][1] = p[1];
  m[0][2] = p[2];
  m[0][3] = p[3];
  
  m[1][0] = p[4];
  m[1][1] = p[5];
  m[1][2] = p[6];
  m[1][3] = p[7];
  
  m[2][0] = p[8];
  m[2][1] = p[9];
  m[2][2] = p[10];
  m[2][3] = p[11];
  
  m[3][0] = p[12];
  m[3][1] = p[13];
  m[3][2] = p[14];
  m[3][3] = p[15];
}


RTI_INLINE
Mat4f::Mat4f(float s) {
  
  m[0][0] = s;
  m[0][1] = 0.0f;
  m[0][2] = 0.0f;
  m[0][3] = 0.0f;
  
  m[1][0] = 0.0f;
  m[1][1] = s;
  m[1][2] = 0.0f;
  m[1][3] = 0.0f;
  
  m[2][0] = 0.0f;
  m[2][1] = 0.0f;
  m[2][2] = s;
  m[2][3] = 0.0f;
  
  m[3][0] = 0.0f;
  m[3][1] = 0.0f;
  m[3][2] = 0.0f;
  m[3][3] = s;
}


RTI_INLINE
Mat4f::Mat4f(Vec4f col0, Vec4f col1, Vec4f col2, Vec4f col3) {
  m[0] = col0;
  m[1] = col1;
  m[2] = col2;
  m[3] = col3;
}


RTI_INLINE
Mat4f::Mat4f(float c0r0, float c0r1, float c0r2, float c0r3,
             float c1r0, float c1r1, float c1r2, float c1r3,
             float c2r0, float c2r1, float c2r2, float c2r3,
             float c3r0, float c3r1, float c3r2, float c3r3) {

  m[0][0] = c0r0;
  m[0][1] = c0r1;
  m[0][2] = c0r2;
  m[0][3] = c0r3;

  m[1][0] = c1r0;
  m[1][1] = c1r1;
  m[1][2] = c1r2;
  m[1][3] = c1r3;

  m[2][0] = c2r0;
  m[2][1] = c2r1;
  m[2][2] = c2r2;
  m[2][3] = c2r3;

  m[3][0] = c3r0;
  m[3][1] = c3r1;
  m[3][2] = c3r2;
  m[3][3] = c3r3;
}


RTI_INLINE
Mat4f::Mat4f(const Mat4f& rhs) {
  *this = rhs;
}


RTI_INLINE
Mat4f& Mat4f::operator=(const Mat4f& rhs) {
  m[0] = rhs[0];
  m[1] = rhs[1];
  m[2] = rhs[2];
  m[3] = rhs[3];
  return *this;
}


RTI_INLINE
Vec4f& Mat4f::operator[](int i) {
  return m[i];
}


RTI_INLINE
const Vec4f& Mat4f::operator[](int i) const {
  return m[i];
}


RTI_INLINE
Mat4f Mat4f::operator*(const Mat4f& rhs) const {
  Mat4f res;

  res[0] = m[0] * rhs[0][0] + m[1] * rhs[0][1] + m[2] * rhs[0][2] + m[3] * rhs[0][3];
  res[1] = m[0] * rhs[1][0] + m[1] * rhs[1][1] + m[2] * rhs[1][2] + m[3] * rhs[1][3];
  res[2] = m[0] * rhs[2][0] + m[1] * rhs[2][1] + m[2] * rhs[2][2] + m[3] * rhs[2][3];
  res[3] = m[0] * rhs[3][0] + m[1] * rhs[3][1] + m[2] * rhs[3][2] + m[3] * rhs[3][3];

  return res;
}


RTI_INLINE
Vec4f Mat4f::operator*(const Vec4f& v) const {
  return Vec4f(m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3],
               m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3],
               m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3],
               m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3]);
}


RTI_INLINE
Mat4f Mat4f::operator+(const Mat4f& rhs) const {
  return Mat4f(m[0][0] + rhs[0][0], m[0][1] + rhs[0][1], m[0][2] + rhs[0][2], m[0][3] + rhs[0][3],
               m[1][0] + rhs[1][0], m[1][1] + rhs[1][1], m[1][2] + rhs[1][2], m[1][3] + rhs[1][3],
               m[2][0] + rhs[2][0], m[2][1] + rhs[2][1], m[2][2] + rhs[2][2], m[2][3] + rhs[2][3],
               m[3][0] + rhs[3][0], m[3][1] + rhs[3][1], m[3][2] + rhs[3][2], m[3][3] + rhs[3][3]);
}


RTI_INLINE
Mat4f Mat4f::operator-(const Mat4f& rhs) const {
  return Mat4f(m[0][0] - rhs[0][0], m[0][1] - rhs[0][1], m[0][2] - rhs[0][2], m[0][3] - rhs[0][3],
               m[1][0] - rhs[1][0], m[1][1] - rhs[1][1], m[1][2] - rhs[1][2], m[1][3] - rhs[1][3],
               m[2][0] - rhs[2][0], m[2][1] - rhs[2][1], m[2][2] - rhs[2][2], m[2][3] - rhs[2][3],
               m[3][0] - rhs[3][0], m[3][1] - rhs[3][1], m[3][2] - rhs[3][2], m[3][3] - rhs[3][3]);
}


RTI_INLINE
Mat4f Mat4f::operator*(float rhs) const {
  return Mat4f(m[0][0] * rhs, m[0][1] * rhs, m[0][2] * rhs, m[0][3] * rhs, m[1][0] * rhs,
               m[1][1] * rhs, m[1][2] * rhs, m[1][3] * rhs, m[2][0] * rhs, m[2][1] * rhs,
               m[2][2] * rhs, m[2][3] * rhs, m[3][0] * rhs, m[3][1] * rhs, m[3][2] * rhs,
               m[3][3] * rhs);
}


RTI_INLINE
Mat4f Mat4f::operator/(float rhs) const {
  const float r = 1.0f / rhs;
  return Mat4f(m[0][0] * r, m[0][1] * r, m[0][2] * r, m[0][3] * r, 
               m[1][0] * r, m[1][1] * r, m[1][2] * r, m[1][3] * r, 
               m[2][0] * r, m[2][1] * r, m[2][2] * r, m[2][3] * r,
               m[3][0] * r, m[3][1] * r, m[3][2] * r, m[3][3] * r);
}


RTI_INLINE
Mat4f operator*(float lhs, const Mat4f& rhs) {
  return rhs * lhs;
}


RTI_INLINE
Vec4f operator*(const Vec4f& v, const Mat4f& m) {
  return Vec4f(m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * v[3],
               m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * v[3],
               m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * v[3],
               m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] * v[3]);

}


RTI_INLINE
Vec4f& operator*=(Vec4f& lhs, const Mat4f& rhs) {
  lhs = lhs * rhs;
  return lhs;
}


RTI_INLINE
Mat4f& Mat4f::operator*=(const Mat4f& rhs) {
  Mat4f tmp      = (*this) * rhs;
  return (*this) = tmp;
}


RTI_INLINE
Mat4f& Mat4f::operator+=(const Mat4f& rhs) {
  
  m[0][0] += rhs[0][0];
  m[0][1] += rhs[0][1];
  m[0][2] += rhs[0][2];
  m[0][3] += rhs[0][3];
  
  m[1][0] += rhs[1][0];
  m[1][1] += rhs[1][1];
  m[1][2] += rhs[1][2];
  m[1][3] += rhs[1][3];
  
  m[2][0] += rhs[2][0];
  m[2][1] += rhs[2][1];
  m[2][2] += rhs[2][2];
  m[2][3] += rhs[2][3];
  
  m[3][0] += rhs[3][0];
  m[3][1] += rhs[3][1];
  m[3][2] += rhs[3][2];
  m[3][3] += rhs[3][3];
  
  return *this;
}


RTI_INLINE
Mat4f& Mat4f::operator-=(const Mat4f& rhs) {
  
  m[0][0] -= rhs[0][0];
  m[0][1] -= rhs[0][1];
  m[0][2] -= rhs[0][2];
  m[0][3] -= rhs[0][3];
  
  m[1][0] -= rhs[1][0];
  m[1][1] -= rhs[1][1];
  m[1][2] -= rhs[1][2];
  m[1][3] -= rhs[1][3];
  
  m[2][0] -= rhs[2][0];
  m[2][1] -= rhs[2][1];
  m[2][2] -= rhs[2][2];
  m[2][3] -= rhs[2][3];
  
  m[3][0] -= rhs[3][0];
  m[3][1] -= rhs[3][1];
  m[3][2] -= rhs[3][2];
  m[3][3] -= rhs[3][3];
  
  return *this;
}


RTI_INLINE
Mat4f& Mat4f::operator*=(float rhs) {
  
  m[0][0] *= rhs;
  m[0][1] *= rhs;
  m[0][2] *= rhs;
  m[0][3] *= rhs;
  
  m[1][0] *= rhs;
  m[1][1] *= rhs;
  m[1][2] *= rhs;
  m[1][3] *= rhs;
  
  m[2][0] *= rhs;
  m[2][1] *= rhs;
  m[2][2] *= rhs;
  m[2][3] *= rhs;
  
  m[3][0] *= rhs;
  m[3][1] *= rhs;
  m[3][2] *= rhs;
  m[3][3] *= rhs;
  
  return *this;
}


RTI_INLINE
Mat4f& Mat4f::operator/=(float rhs) {
  
  const float r = 1.0f / rhs;
  
  m[0][0] *= r;
  m[0][1] *= r;
  m[0][2] *= r;
  m[0][3] *= r;
  
  m[1][0] *= r;
  m[1][1] *= r;
  m[1][2] *= r;
  m[1][3] *= r;
  
  m[2][0] *= r;
  m[2][1] *= r;
  m[2][2] *= r;
  m[2][3] *= r;
  
  m[3][0] *= r;
  m[3][1] *= r;
  m[3][2] *= r;
  m[3][3] *= r;
  
  return *this;
}


RTI_INLINE
Mat4f Mat4f::operator-() const {
  return Mat4f(-m[0][0], -m[0][1], -m[0][2], -m[0][3], 
               -m[1][0], -m[1][1], -m[1][2], -m[1][3],
               -m[2][0], -m[2][1], -m[2][2], -m[2][3], 
               -m[3][0], -m[3][1], -m[3][2], -m[3][3]);
}


RTI_INLINE
bool Mat4f::operator==(const Mat4f& rhs) const {
  return memcmp(this, &rhs, sizeof(Mat4f)) == 0;
}


RTI_INLINE
bool Mat4f::operator!=(const Mat4f& rhs) const {
  return memcmp(this, &rhs, sizeof(Mat4f)) != 0;
}


RTI_INLINE
Mat4f::operator float*() {
  return (float*)m[0];
}


RTI_INLINE
Mat4f::operator const float*() const {
  return (const float*)m[0];
}


//-------------------------------------------------------------

RTI_INLINE
float clamp(float value, float low, float high) {
  return (value < low) ? low : ((value > high) ? high : value);
}


RTI_INLINE
int clamp(int value, int low, int high) {
  return (value < low) ? low : ((value > high) ? high : value);
}


RTI_INLINE
float dot(const Vec2f& lhs, const Vec2f& rhs) {
  return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}


RTI_INLINE
float dot(const Vec3f& lhs, const Vec3f& rhs) {
  return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}


RTI_INLINE
float dot(const Vec4f& lhs, const Vec4f& rhs) {
  return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
}


RTI_INLINE
Vec3f cross(const Vec3f& lhs, const Vec3f& rhs) {
  return Vec3f(lhs[1] * rhs[2] - lhs[2] * rhs[1], 
               lhs[2] * rhs[0] - lhs[0] * rhs[2],
               lhs[0] * rhs[1] - lhs[1] * rhs[0]);
}


RTI_INLINE
float length(const Vec2f& v) {
  return sqrtf((v.x * v.x) + (v.y * v.y));
}


RTI_INLINE
float length(const Vec3f& v) {
  return sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}


RTI_INLINE
float length(const Vec4f& v) {
  return sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w));
}


RTI_INLINE
float normalize(Vec2f& v) {
  const float fLength = length(v);
  v /= fLength;
  return fLength;
}


RTI_INLINE
float normalize(Vec3f& v) {
  const float fLength = length(v);
  v /= fLength;
  return fLength;
}


RTI_INLINE
float normalize(Vec4f& v) {
  const float fLength = length(v);
  v /= fLength;
  return fLength;
}


RTI_INLINE
Vec3f reflect(const Vec3f& i, const Vec3f& n) {
  return i - 2.0f * dot(n, i) * n;
}


RTI_INLINE
Vec3f transformPoint(const Vec3f& v, const Mat4f& m) {
  Vec3f res;

  res[0] = (v[0] * m[0][0]) + (v[1] * m[1][0]) + (v[2] * m[2][0]) + m[3][0];
  res[1] = (v[0] * m[0][1]) + (v[1] * m[1][1]) + (v[2] * m[2][1]) + m[3][1];
  res[2] = (v[0] * m[0][2]) + (v[1] * m[1][2]) + (v[2] * m[2][2]) + m[3][2];

  return res;
}


RTI_INLINE
Vec3f transformVector(const Vec3f& v, const Mat4f& m) {
  Vec3f res;

  res[0] = (v[0] * m[0][0]) + (v[1] * m[1][0]) + (v[2] * m[2][0]);
  res[1] = (v[0] * m[0][1]) + (v[1] * m[1][1]) + (v[2] * m[2][1]);
  res[2] = (v[0] * m[0][2]) + (v[1] * m[1][2]) + (v[2] * m[2][2]);

  return res;
}


RTI_INLINE
Vec3f transformVector(const Vec3f& v, const Mat3f& m) {
  Vec3f res;

  res[0] = (v[0] * m[0][0]) + (v[1] * m[1][0]) + (v[2] * m[2][0]);
  res[1] = (v[0] * m[0][1]) + (v[1] * m[1][1]) + (v[2] * m[2][1]);
  res[2] = (v[0] * m[0][2]) + (v[1] * m[1][2]) + (v[2] * m[2][2]);

  return res;
}


//---------- Matrix functions -------------------------------

RTI_INLINE
Mat2f& makeIdentity(Mat2f& mat) {
  mat[0][1] = mat[1][0] = 0.0f;
  mat[0][0] = mat[1][1] = 1.0f;
  return mat;
}


RTI_INLINE
Mat3f& makeIdentity(Mat3f& mat) {
  mat[0][1] = mat[0][2] = 0.0f;
  mat[1][0] = mat[1][2] = 0.0f;
  mat[2][0] = mat[2][1] = 0.0f;

  mat[0][0] = mat[1][1] = mat[2][2] = 1.0f;

  return mat;
}


RTI_INLINE
Mat4f& makeIdentity(Mat4f& mat) {
  mat[0][1] = mat[0][2] = mat[0][3] = 0.0f;
  mat[1][0] = mat[1][2] = mat[1][3] = 0.0f;
  mat[2][0] = mat[2][1] = mat[2][3] = 0.0f;
  mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;

  mat[0][0] = mat[1][1] = mat[2][2] = mat[3][3] = 1.0f;

  return mat;
}


RTI_INLINE
bool isIdentity(const Mat2f& mat) {
  return (mat[0][0] == 1.0f) && (mat[0][1] == 0.0f) && (mat[1][0] == 0.0f) && (mat[1][1] == 1.0f);
}


RTI_INLINE
bool isIdentity(const Mat3f& mat) {
  return (mat[0][0] == 1.0f) && (mat[0][1] == 0.0f) && (mat[0][2] == 0.0f) && 
         (mat[1][0] == 0.0f) && (mat[1][1] == 1.0f) && (mat[1][2] == 0.0f) && 
         (mat[2][0] == 0.0f) && (mat[2][1] == 0.0f) && (mat[2][2] == 1.0f);
}


RTI_INLINE
bool isIdentity(const Mat4f& mat) {
  return (mat[0][0] == 1.0f) && (mat[0][1] == 0.0f) && (mat[0][2] == 0.0f) && (mat[0][3] == 0.0f) &&
         (mat[1][0] == 0.0f) && (mat[1][1] == 1.0f) && (mat[1][2] == 0.0f) && (mat[1][3] == 0.0f) &&
         (mat[2][0] == 0.0f) && (mat[2][1] == 0.0f) && (mat[2][2] == 1.0f) && (mat[2][3] == 0.0f) &&
         (mat[3][0] == 0.0f) && (mat[3][1] == 0.0f) && (mat[3][2] == 0.0f) && (mat[3][3] == 1.0f);
}


RTI_INLINE
float determinant(const Mat2f& m) {
  return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}


RTI_INLINE
float determinant(const Mat3f& m) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) +
         m[0][1] * (m[1][2] * m[2][0] - m[1][0] * m[2][2]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}


RTI_INLINE
float determinant(const Mat4f& m) {

  const float a0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
  const float a1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
  const float a2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
  const float a3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
  const float a4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
  const float a5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
  const float b0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
  const float b1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
  const float b2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
  const float b3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
  const float b4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
  const float b5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];

  return a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
}


RTI_INLINE
Mat2f& transpose(Mat2f& r, const Mat2f& m) {

  r[0][0] = m[0][0];
  r[0][1] = m[1][0];

  r[1][0] = m[0][1];
  r[1][1] = m[1][1];

  return r;
}


RTI_INLINE
Mat3f& transpose(Mat3f& r, const Mat3f& m) {

  r[0][0] = m[0][0];
  r[0][1] = m[1][0];
  r[0][2] = m[2][0];

  r[1][0] = m[0][1];
  r[1][1] = m[1][1];
  r[1][2] = m[2][1];

  r[2][0] = m[0][2];
  r[2][1] = m[1][2];
  r[2][2] = m[2][2];

  return r;
}


RTI_INLINE
Mat4f& transpose(Mat4f& r, const Mat4f& m) {

  r[0][0] = m[0][0];
  r[0][1] = m[1][0];
  r[0][2] = m[2][0];
  r[0][3] = m[3][0];

  r[1][0] = m[0][1];
  r[1][1] = m[1][1];
  r[1][2] = m[2][1];
  r[1][3] = m[3][1];

  r[2][0] = m[0][2];
  r[2][1] = m[1][2];
  r[2][2] = m[2][2];
  r[2][3] = m[3][2];

  r[3][0] = m[0][3];
  r[3][1] = m[1][3];
  r[3][2] = m[2][3];
  r[3][3] = m[3][3];

  return r;
}


RTI_INLINE
Mat2f& inverse(Mat2f& r, const Mat2f& m) {
  const float MATRIX_INVERSE_EPSILON = 1e-14f;

  const float det = determinant(m);

  if (::fabsf(det) <= MATRIX_INVERSE_EPSILON) {
    memset(r.m, 0, sizeof(r));
    return r;
  }

  const float invDet = 1.0f / det;

  r[0][0] = m[1][1] * invDet;
  r[0][1] = -m[0][1] * invDet;
  r[1][0] = -m[1][0] * invDet;
  r[1][1] = m[0][0] * invDet;

  return r;
}


RTI_INLINE
Mat3f& inverse(Mat3f& r, const Mat3f& m) {
  const float MATRIX_INVERSE_EPSILON = 1e-14f;

  r[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
  r[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
  r[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];

  r[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
  r[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
  r[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];

  r[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
  r[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
  r[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];

  const float det = m[0][0] * r[0][0] + m[0][1] * r[1][0] + m[0][2] * r[2][0];

  if (::fabsf(det) <= MATRIX_INVERSE_EPSILON) {
    memset(r.m, 0, sizeof(r));
    return r;
  }

  const float invDet = 1.0f / det;

  r[0][0] *= invDet;
  r[0][1] *= invDet;
  r[0][2] *= invDet;

  r[1][0] *= invDet;
  r[1][1] *= invDet;
  r[1][2] *= invDet;

  r[2][0] *= invDet;
  r[2][1] *= invDet;
  r[2][2] *= invDet;

  return r;
}


RTI_INLINE
Mat4f& inverse(Mat4f& r, const Mat4f& m) {
  const float MATRIX_INVERSE_EPSILON = 1e-14f;

  const float a0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
  const float a1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
  const float a2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
  const float a3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
  const float a4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
  const float a5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
  const float b0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
  const float b1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
  const float b2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
  const float b3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
  const float b4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
  const float b5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];

  const float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;

  if (::fabsf(det) <= MATRIX_INVERSE_EPSILON) {
    memset(r.m, 0, sizeof(r));
    return r;
  }

  const float invDet = 1.0f / det;

  r[0][0] = (+m[1][1] * b5 - m[1][2] * b4 + m[1][3] * b3) * invDet;
  r[1][0] = (-m[1][0] * b5 + m[1][2] * b2 - m[1][3] * b1) * invDet;
  r[2][0] = (+m[1][0] * b4 - m[1][1] * b2 + m[1][3] * b0) * invDet;
  r[3][0] = (-m[1][0] * b3 + m[1][1] * b1 - m[1][2] * b0) * invDet;
  r[0][1] = (-m[0][1] * b5 + m[0][2] * b4 - m[0][3] * b3) * invDet;
  r[1][1] = (+m[0][0] * b5 - m[0][2] * b2 + m[0][3] * b1) * invDet;
  r[2][1] = (-m[0][0] * b4 + m[0][1] * b2 - m[0][3] * b0) * invDet;
  r[3][1] = (+m[0][0] * b3 - m[0][1] * b1 + m[0][2] * b0) * invDet;
  r[0][2] = (+m[3][1] * a5 - m[3][2] * a4 + m[3][3] * a3) * invDet;
  r[1][2] = (-m[3][0] * a5 + m[3][2] * a2 - m[3][3] * a1) * invDet;
  r[2][2] = (+m[3][0] * a4 - m[3][1] * a2 + m[3][3] * a0) * invDet;
  r[3][2] = (-m[3][0] * a3 + m[3][1] * a1 - m[3][2] * a0) * invDet;
  r[0][3] = (-m[2][1] * a5 + m[2][2] * a4 - m[2][3] * a3) * invDet;
  r[1][3] = (+m[2][0] * a5 - m[2][2] * a2 + m[2][3] * a1) * invDet;
  r[2][3] = (-m[2][0] * a4 + m[2][1] * a2 - m[2][3] * a0) * invDet;
  r[3][3] = (+m[2][0] * a3 - m[2][1] * a1 + m[2][2] * a0) * invDet;

  return r;
}


RTI_INLINE
Mat3f& normalMatrix(Mat3f& res, const Mat4f& mat) {
  Mat3f tmp;
  transpose(tmp, Mat3f(mat));
  return inverse(res, tmp);
}


RTI_INLINE
Mat4f translate(float dx, float dy, float dz) {
  return Mat4f(1.0f, 0.0f, 0.0f, 0.0f, 
               0.0f, 1.0f, 0.0f, 0.0f, 
               0.0f, 0.0f, 1.0f, 0.0f, 
                 dx,   dy,   dz, 1.0f);
}


RTI_INLINE
Mat4f translate(const Vec3f& t) {
  return Mat4f(1.0f, 0.0f, 0.0f, 0.0f, 
               0.0f, 1.0f, 0.0f, 0.0f, 
               0.0f, 0.0f, 1.0f, 0.0f,
               t[0], t[1], t[2], 1.0f);
}


RTI_INLINE
Mat4f scale(float s) {
  return Mat4f(   s, 0.0f, 0.0f, 0.0f, 
               0.0f,    s, 0.0f, 0.0f, 
               0.0f, 0.0f,    s, 0.0f, 
               0.0f, 0.0f, 0.0f, 1.0f);
}


RTI_INLINE
Mat4f scale(float sx, float sy, float sz) {
  return Mat4f(  sx, 0.0f, 0.0f, 0.0f, 
               0.0f,   sy, 0.0f, 0.0f, 
               0.0f, 0.0f,   sz, 0.0f, 
               0.0f, 0.0f, 0.0f, 1.0f);
}


RTI_INLINE
Mat4f rotate(Vec3f a, float angle) {

  Mat4f m;
  normalize(a);

  const float s = sinf(RTI_DEG_TO_RAD * angle);
  const float c = cosf(RTI_DEG_TO_RAD * angle);
  const float x = a.x;
  const float y = a.y;
  const float z = a.z;
  const float t = 1.0f - c;

  m[0][0] = t * x * x + c;
  m[0][1] = t * x * y + s * z;
  m[0][2] = t * x * z - s * y;
  m[0][3] = 0.0f;

  m[1][0] = t * y * x - s * z;
  m[1][1] = t * y * y + c;
  m[1][2] = t * y * z + s * x;
  m[1][3] = 0.0f;

  m[2][0] = t * z * x + s * y;
  m[2][1] = t * z * y - s * x;
  m[2][2] = t * z * z + c;
  m[2][3] = 0.0f;

  m[3][0] = 0.0f;
  m[3][1] = 0.0f;
  m[3][2] = 0.0f;
  m[3][3] = 1.0f;

  return m;
}


RTI_INLINE
Mat4f rotateX(float angle) {

  float s = sinf(RTI_DEG_TO_RAD * angle);
  float c = cosf(RTI_DEG_TO_RAD * angle);

  return Mat4f(1.0f, 0.0f, 0.0f, 0.0f, 
               0.0f,    c,    s, 0.0f, 
               0.0f,   -s,    c, 0.0f, 
               0.0f, 0.0f, 0.0f, 1.0f);
}


RTI_INLINE
Mat4f rotateY(float angle) {

  float s = sinf(RTI_DEG_TO_RAD * angle);
  float c = cosf(RTI_DEG_TO_RAD * angle);

  return Mat4f(   c, 0.0f,   -s, 0.0f, 
               0.0f, 1.0f, 0.0f, 0.0f, 
                  s, 0.0f,    c, 0.0f, 
               0.0f, 0.0f, 0.0f, 1.0f);
}

RTI_INLINE
Mat4f rotateZ(float angle) {

  float s = sinf(RTI_DEG_TO_RAD * angle);
  float c = cosf(RTI_DEG_TO_RAD * angle);

  return Mat4f(   c,    s, 0.0f, 0.0f, 
                 -s,    c, 0.0f, 0.0f, 
               0.0f, 0.0f, 1.0f, 0.0f, 
               0.0f, 0.0f, 0.0f, 1.0f);
}

// Taken from http://cs.brown.edu/research/pubs/pdfs/1999/Moller-1999-EBA.pdf
// The paper also includes a faster solution that does not have a square root.
RTI_INLINE
void axisToAxisRotation(Vec3f from, Vec3f into, Mat4f& mat) {

  normalize(from);
  normalize(into);
  makeIdentity(mat);

  float fromDotInto = dot(from, into);

  if (fromDotInto > 0.9999f) {
    return;
  }

  Vec3f rotAxis;
  if (fromDotInto < -0.9999f) {
    // Setup axis to be the min direction of from...
    float absX = fabsf(from[0]);
    float absY = fabsf(from[1]);
    float absZ = fabsf(from[2]);

    Vec3f axis = Vec3f(((absX <= absY) && (absX <= absZ)) ? 1.0f : 0.0f,
                       ((absY < absX) && (absY <= absZ)) ? 1.0f : 0.0f,
                       ((absZ < absX) && (absZ < absY)) ? 1.0f : 0.0f);
    rotAxis = cross(from, axis);
  }
  else {
    rotAxis = cross(from, into);
  }
  normalize(rotAxis);

  float cosT = fromDotInto;
  float sinT = sqrtf(1.0f - cosT * cosT);
  float ux   = rotAxis[0];
  float uy   = rotAxis[1];
  float uz   = rotAxis[2];

  mat[0][0] = ux * ux * (1.0f - cosT) + cosT;
  mat[0][1] = ux * uy * (1.0f - cosT) + uz * sinT;
  mat[0][2] = ux * uz * (1.0f - cosT) - uy * sinT;

  mat[1][0] = uy * ux * (1.0f - cosT) - uz * sinT;
  mat[1][1] = uy * uy * (1.0f - cosT) + cosT;
  mat[1][2] = uy * uz * (1.0f - cosT) + ux * sinT;

  mat[2][0] = uz * ux * (1.0f - cosT) + uy * sinT;
  mat[2][1] = uz * uy * (1.0f - cosT) - ux * sinT;
  mat[2][2] = uz * uz * (1.0f - cosT) + cosT;
}

END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_MATH_INL__
