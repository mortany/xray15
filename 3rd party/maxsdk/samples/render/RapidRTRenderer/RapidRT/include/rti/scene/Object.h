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
* @file    Object.h
* @brief   Public interface for scene graph leaf objects.
*
* @author  Henrik Edstrom
* @date    2008-01-15
*
*/

#ifndef __RTI_OBJECT_H__
#define __RTI_OBJECT_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"
#include "rti/scene/Entity.h"
#include "rti/scene/Geometry.h"

///////////////////////////////////////////////////////////////////////////////
// rti::Object ////////////////////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   Object
* @brief   Public interface for scene graph leaf objects.
*
* @author  Henrik Edstrom
* @date    2008-01-15
*/
class RTAPI Object : public Entity {
public:
  DECL_INTERFACE_CLASS(CTYPE_OBJECT)

  //---------- Methods ----------------------------------------

  RTIResult       setGeometry(GeometryHandle geometry);
  GeometryHandle  getGeometry() const;

};  // Object


//---------- Handle type --------------------------------------

typedef THandle<Object, EntityHandle> ObjectHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_OBJECT_H__
