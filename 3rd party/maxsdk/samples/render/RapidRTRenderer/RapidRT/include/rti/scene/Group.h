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
* @file    Group.h
* @brief   Public interface for scene graph group nodes.
*
* @author  Henrik Edstrom
* @date    2008-01-15
*
*/

#ifndef __RTI_GROUP_H__
#define __RTI_GROUP_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"
#include "rti/scene/Entity.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Group /////////////////////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   Group
* @brief   Public interface for scene graph group nodes.
*
* @author  Henrik Edstrom
* @date    2008-01-15
*/
class RTAPI Group : public Entity {
public:
  DECL_INTERFACE_CLASS(CTYPE_GROUP)

  //---------- Methods ----------------------------------------

  void                  addChild(EntityHandle child);
  RTIResult             removeChild(EntityHandle child);
  RTIResult             setChild(int i, EntityHandle child);
  RTIResult             replaceChild(EntityHandle oldChild, EntityHandle newChild);
  void                  removeAllChildren();

  EntityConstIterator   getChildren() const;
  int                   getNumChildren() const;
  EntityHandle          getChild(int i) const;
  int                   findChild(EntityHandle child) const;


};  // Group


//---------- Handle type --------------------------------------

typedef THandle<Group, EntityHandle> GroupHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_GROUP_H__
