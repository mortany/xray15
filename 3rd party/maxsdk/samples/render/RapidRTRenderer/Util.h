//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

// rti
#include <rti/math/math.h>
#include <rti/scene/geometry.h>
// windows
#include <wtypes.h>
#include <point3.h>

class Color;
class AColor;
class Matrix3;

namespace Max
{;
namespace RapidRTTranslator
{;

namespace RRTUtil
{
	rti::Vec3f convertVector(const Point3& p);
	rti::Vec4f convertVector(const Point3& p, float f);
	rti::Mat4f convertMatrix(const Matrix3& in);
	rti::Vec3f convertColor(const Color& c);
	rti::Vec3f convertColor(const DWORD c);
	rti::Vec4f convertColor(const AColor& c);
	rti::Vec2f convertUV(const UVVert& uvv);

	void CreateRapidPlane(const rti::GeometryHandle &hGeometry, float width, float height);
	void CreateRapidSphere(const rti::GeometryHandle &hGeometry, float radius);
	void CreateRapidDisc(const rti::GeometryHandle &hGeometry, float diameter);
	void CreateRapidCylinder(const rti::GeometryHandle &hGeometry, float diameter, float height);

	size_t GenerateHashID(const TCHAR* s);

	float glossToRough(float gloss);

    // Checks the rti result for success: asserts on error. Returns true iff no error.
    bool check_rti_result(const rti::RTIResult rti_result);

    // Splits a time into hours, minutes, seconds
    struct Time_HMS
    {
        unsigned int hours;
        unsigned int minutes;
        unsigned int seconds;

        int get_total_seconds() const;
        static Time_HMS from_seconds(const unsigned int seconds);
    };
};

}}

#include "Util.inline.h"