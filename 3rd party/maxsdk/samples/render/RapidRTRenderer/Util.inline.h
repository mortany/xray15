//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

// max sdk
#include <color.h>
#include <acolor.h>
#include <matrix3.h>

namespace Max
{;
namespace RapidRTTranslator
{;

namespace RRTUtil
{
    inline rti::Vec3f convertVector(const Point3& p)
    {
        return rti::Vec3f(p.x, p.y, p.z);
    }

    inline rti::Vec4f convertVector(const Point3& p, float f)
    {
        return rti::Vec4f(p.x, p.y, p.z, f);
    }

    inline rti::Mat4f convertMatrix(const Matrix3& in)
    {
        rti::Vec4f row0 = convertVector(in.GetRow(0), 0.0f);
        rti::Vec4f row1 = convertVector(in.GetRow(1), 0.0f);
        rti::Vec4f row2 = convertVector(in.GetRow(2), 0.0f);
        rti::Vec4f row3 = convertVector(in.GetRow(3), 1.0f);

        rti::Mat4f out = rti::Mat4f(row0, row1, row2, row3);

        return out;
    }

    inline rti::Vec3f convertColor(const Color& c)
    {
        return rti::Vec3f(c.r, c.g, c.b);
    }

    inline rti::Vec3f convertColor(const DWORD c)
    {
        return rti::Vec3f(GetRValue(c)/255.0f, GetGValue(c)/255.0f, GetBValue(c)/255.0f);
    }

    inline rti::Vec4f convertColor(const AColor& c)
    {
        return rti::Vec4f(c.r, c.g, c.b, c.a);
    }

    inline rti::Vec2f convertUV(const UVVert& uvv)
    {
        return rti::Vec2f(uvv.x, uvv.y);
    }
    
    inline bool check_rti_result(const rti::RTIResult rti_result)
    {
        DbgAssert(rti_result == rti::RTI_SUCCESS);
        return (rti_result == rti::RTI_SUCCESS);
    }
}

}}


