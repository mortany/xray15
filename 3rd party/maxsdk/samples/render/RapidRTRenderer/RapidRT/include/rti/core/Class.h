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
* @file    Class.h
* @brief   Public interface to the root class.
*
* @author  Henrik Edstrom
* @date    2008-01-12
*
*/

#ifndef __RTI_CLASS_H__
#define __RTI_CLASS_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"
#include "rti/util/Handle.h"

///////////////////////////////////////////////////////////////////////////////
// rti::Class /////////////////////////////////////////////////////////////////

BEGIN_RTI


/**
* @class   Class
* @brief   Public interface to the root class.
*
* @author  Henrik Edstrom
* @date    2008-01-12
*/
class RTAPI Class {
public:
  DECL_ROOT_INTERFACE_CLASS(CTYPE_CLASS)

  //---------- Methods ----------------------------------------


};  // Class


//---------- Handle type --------------------------------------

typedef TAbstractHandle<Class, HandleBase> ClassHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_CLASS_H__
