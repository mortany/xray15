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
* @file    Geometry.h
* @brief   Public interface for geometries.
*
* @author  Henrik Edstrom
* @date    2008-02-04
*
*/

#ifndef __RTI_GEOMETRY_H__
#define __RTI_GEOMETRY_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/math/Math.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Geometry //////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Types --------------------------------------------

enum EGeometryType {
  GEOM_TRIANGLE_MESH,
  GEOM_ON_DEMAND_TRIANGLE_MESH,

  NUM_GEOMETRY_TYPES
};


enum EGeometryAttribute {
  GEOM_ATTRIB_INDEX_ARRAY,
  GEOM_ATTRIB_COORD_ARRAY,
  GEOM_ATTRIB_NORMAL_ARRAY,
  GEOM_ATTRIB_BOUNDS_MIN,
  GEOM_ATTRIB_BOUNDS_MAX,

  GEOM_ATTRIB_SPHERE_RADIUS
};


/**
* @class   Geometry
* @brief   Public interface for geometries.
*
* @author  Henrik Edstrom
* @date    2008-02-04
*/
class RTAPI Geometry : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_GEOMETRY)

  //---------- Methods ----------------------------------------

  RTIResult init(EGeometryType geometryType);

  RTIResult setAttribute1i(EGeometryAttribute attribute, int value);
  RTIResult setAttribute1f(EGeometryAttribute attribute, float value);
  RTIResult setAttribute3f(EGeometryAttribute attribute, Vec3f value);

  RTIResult setAttribute1iv(EGeometryAttribute attribute, int size, const int* data);
  RTIResult setAttribute3fv(EGeometryAttribute attribute, int size, const Vec3f* data);

  RTIResult setAttribute1fv(const char* name, int size, const float* data);
  RTIResult setAttribute2fv(const char* name, int size, const Vec2f* data);
  RTIResult setAttribute3fv(const char* name, int size, const Vec3f* data);
  RTIResult setAttribute4fv(const char* name, int size, const Vec4f* data);
  RTIResult setAttribute2x2fv(const char* name, bool transpose, int size, const Mat2f* data);
  RTIResult setAttribute3x3fv(const char* name, bool transpose, int size, const Mat3f* data);
  RTIResult setAttribute4x4fv(const char* name, bool transpose, int size, const Mat4f* data);

  RTIResult setUserData(const void* data);

  RTIResult removeAttribute(EGeometryAttribute attrib);
  RTIResult removeAttribute(const char* name);

};  // Geometry


//---------- Handle type --------------------------------------

typedef THandle<Geometry, ClassHandle> GeometryHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_GEOMETRY_H__
