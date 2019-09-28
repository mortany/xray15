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
* @file    IOnDemandGeometryData.h
* @brief   Public interface for on demand geometries.
*
* @author  Per Svensson
* @date    2013-09-12
*
*/

#ifndef __RTI_ON_DEMAND_GEOMETRY_DATA_H__
#define __RTI_ON_DEMAND_GEOMETRY_DATA_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/math/Math.h"

///////////////////////////////////////////////////////////////////////////////
// rti::IOnDemandGeometryData /////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   IOnDemandGeometryData
* @brief   Interface class for setting on demand geomtery data
*
* @author  Per Svensson
* @date    2013-09-12
*/
class IOnDemandGeometryData {
public:
  //---------- Methods ----------------------------------------

  virtual ~IOnDemandGeometryData() {}

  virtual RTIResult setGeometryData(int numTriangles, int numVertices, const int* triangles,
                                    const Vec3f* coords, const Vec3f* normals) = 0;

  virtual RTIResult setVertexAttribute2fv(const char* name, int size, const Vec2f* data) = 0;
  virtual RTIResult setVertexAttribute3fv(const char* name, int size, const Vec3f* data) = 0;
  virtual RTIResult setVertexAttribute4fv(const char* name, int size, const Vec4f* data) = 0;

};  // IOnDemandGeometryData

END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_ON_DEMAND_GEOMETRY_DATA_H__