//////////////////////////////////////////////////////////////////////////////
//  Copyright 2014 by Autodesk, Inc.  All rights reserved.
//
// This computer source code and related instructions and comments 
// are the unpublished confidential and proprietary information of 
// Autodesk, Inc. and are protected under applicable copyright and 
// trade secret law.  They may not be disclosed to, copied or used 
// by any third party without the prior written consent of Autodesk, Inc.
//////////////////////////////////////////////////////////////////////////////
//
// This header defines everything needed in order to include the RapidSL shader
// for the physical sun sky in c++ source code.
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <texutil.h>

#undef min
#undef max

namespace Max
{;
namespace RapidSLShaders    // Namespace is important to isolate the definitions below
{;

class float3 : public Point3
{
public:

    float3()
    {
    }

    float3(const Point3& from)
        : Point3(from)
    {
    }

    float3(const Color& from)
        : Point3(from)
    {
    }

    float3(const float x, const float y, const float z)
        : Point3(x, y, z)
    {
    }

    explicit float3(const float val)
        : Point3(val, val, val)
    {
    }

};

class float3x3 : public Matrix3
{
public:
    float3x3(
        float x0, float x1, float x2,
        float y0, float y1, float y2,
        float z0, float z1, float z2)
        : Matrix3(Point3(x0, y0, z0), Point3(x1, y1, z1), Point3(x2, y2, z2), Point3(0.0f, 0.0f, 0.0f))
    {
    }
};

float dot(const float3& a, const float3& b)
{
    return DotProd(a, b);
}

float3 normalize(const float3& vector)
{
    return vector.Normalize();
}

float3 pow(const float3& value, const float power)
{
    return float3(powf(value.x, power), powf(value.y, power), powf(value.z, power));
}

float3 exp(const float3& value)
{
    return float3(expf(value.x), expf(value.y), expf(value.z));
}

float max(const float a, const float b)
{
    return (a < b) ? b : a;
}

float3 max(const float3& a, const float3& b)
{
    return float3(
        max(a.x, b.x),
        max(a.y, b.y),
        max(a.z, b.z));
}

// Avoid hiding global versions
using ::exp;
using ::pow;

// Include the RapidSL shader, with special #define that disables the Rapid-specific stuff.
#define RTSL_INCLUDED_BY_CPP_CODE
#include "PhysSunSky.rtsl"
#undef RTSL_INCLUDED_BY_CPP_CODE

}}	// namespace 
