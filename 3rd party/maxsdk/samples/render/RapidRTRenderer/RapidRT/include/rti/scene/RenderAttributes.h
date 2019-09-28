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
* @file    RenderAttributes.h
* @brief   Public interface for render attributes.
*
* @author  Henrik Edstrom
* @date    2008-02-05
*
*/

#ifndef __RTI_RENDER_ATTRIBUTES_H__
#define __RTI_RENDER_ATTRIBUTES_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"


///////////////////////////////////////////////////////////////////////////////
// rti::RenderAttributes //////////////////////////////////////////////////////

BEGIN_RTI

//---------- Types --------------------------------------------

enum ERenderAttribute {
  ATTRIB_VISIBILITY_PRIMARY,

  ATTRIB_LIGHT,
  ATTRIB_LIGHT_ILLUMINATES_MATTE,
  ATTRIB_CAUSTICS,
  ATTRIB_MATTE_OBJECT,
  ATTRIB_TRANSPARENT_MATTE,
  ATTRIB_BACKGROUND,
  ATTRIB_NESTING_PRIORITY,

  NUM_RENDER_ATTRIBUTES
};


/**
* @class   RenderAttributes
* @brief   Public interface for render attributes.
*
* @author  Henrik Edstrom
* @date    2008-02-05
*/
class RTAPI RenderAttributes : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_RENDER_ATTRIBUTES)

  //---------- Methods ----------------------------------------

  RTIResult setAttribute1b(ERenderAttribute attribute, bool value);
  RTIResult setAttribute1f(ERenderAttribute attribute, float value);
  RTIResult setAttribute1i(ERenderAttribute attribute, int value);
  RTIResult setAttribute1iv(ERenderAttribute attribute, int numValues, int* values);
  RTIResult removeAttribute(ERenderAttribute attribute);

};  // RenderAttributes


//---------- Handle type --------------------------------------

typedef THandle<RenderAttributes, ClassHandle> RenderAttributesHandle;


END_RTI

///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_RENDER_ATTRIBUTES_H__
