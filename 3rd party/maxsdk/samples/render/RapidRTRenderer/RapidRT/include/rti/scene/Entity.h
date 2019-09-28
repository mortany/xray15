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
* @file    Entity.h
* @brief   Public interface for scene entities.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*
*/

#ifndef __RTI_ENTITY_H__
#define __RTI_ENTITY_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/scene/RenderAttributes.h"
#include "rti/scene/Shader.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Entity ////////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Forward declarations -----------------------------

struct Mat4f;


/**
* @class   Entity
* @brief   Public interface for scene entities.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*/
class RTAPI Entity : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_ENTITY)

  //---------- Methods ----------------------------------------

  void                    setTransform(const Mat4f& m);
  void                    setMotionTransform(const Mat4f& m);
  RTIResult               setShader(ShaderHandle shader);
  void                    setRenderAttributes(RenderAttributesHandle attributes);

  void                    getTransform(Mat4f* m) const;
  void                    getMotionTransform(Mat4f* m) const;
  ShaderHandle            getShader() const;
  RenderAttributesHandle  getRenderAttributes() const;

};  // Entity


//---------- Handle type --------------------------------------

typedef TAbstractHandle<Entity, ClassHandle> EntityHandle;


/**
* @class   EntityConstIterator
* @brief   Iterator (const) for entities.
*
* @author  Henrik Edstrom
* @date    2008-05-13
*/
class EntityConstIterator {
public:
  //---------- Constructors -----------------------------------

  EntityConstIterator();
  EntityConstIterator(const EntityHandle* begin, const EntityHandle* end);
  EntityConstIterator(const EntityConstIterator& rhs);


  //---------- Operators --------------------------------------

  bool operator==(const EntityConstIterator& rhs) const;
  bool operator!=(const EntityConstIterator& rhs) const;
  EntityConstIterator& operator=(const EntityConstIterator& rhs);
  EntityConstIterator& operator++();
  const EntityHandle& operator*() const;

  operator bool() const;

private:
  //---------- Attributes -------------------------------------

  const EntityHandle* m_curr;
  const EntityHandle* m_end;

};  // EntityConstIterator


//---------- EntityConstIterator inline section ---------------

inline EntityConstIterator::EntityConstIterator() : m_curr(0), m_end(0) {}


inline EntityConstIterator::EntityConstIterator(const EntityHandle* begin,
                                                const EntityHandle* end)
    : m_curr(begin), m_end(end) {}


inline EntityConstIterator::EntityConstIterator(const EntityConstIterator& rhs)
    : m_curr(rhs.m_curr), m_end(rhs.m_end) {}


inline bool EntityConstIterator::operator==(const EntityConstIterator& rhs) const {
  return m_curr == rhs.m_curr;
}


inline bool EntityConstIterator::operator!=(const EntityConstIterator& rhs) const {
  return m_curr != rhs.m_curr;
}


inline EntityConstIterator& EntityConstIterator::operator=(const EntityConstIterator& rhs) {
  m_curr = rhs.m_curr;
  m_end  = rhs.m_end;
  return *this;
}


inline EntityConstIterator& EntityConstIterator::operator++() {
  m_curr++;
  return *this;
}


inline const EntityHandle& EntityConstIterator::operator*() const {
  return *m_curr;
}


inline EntityConstIterator::operator bool() const {
  return m_curr != m_end;
}


//---------- END: EntityConstIterator inline section ----------

END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_ENTITY_H__
