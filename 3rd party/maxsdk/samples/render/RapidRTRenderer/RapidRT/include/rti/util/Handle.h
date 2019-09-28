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
* @file    Handle.h
* @brief   Handle types.
*
* @author  Henrik Edstrom
* @date    2008-01-11
*
*/

#ifndef __RTI_HANDLE_H__
#define __RTI_HANDLE_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Handle ////////////////////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   HandleBase
* @brief   Base class for handles.
*
* @author  Henrik Edstrom
* @date    2008-01-11
*/
class HandleBase {
public:
  //---------- Friends ----------------------------------------

  template <class T>
  friend T castHandle(const HandleBase& h);


  //---------- Copy constructor and assignment ----------------

  HandleBase(const HandleBase& rhs);
  HandleBase& operator=(const HandleBase& rhs);


  //---------- Methods ----------------------------------------

  bool      isValid() const;
  int       getType() const;
  bool      isOfType(EClassType type) const;
  int       getInstance() const;
  void*     beginEdit();
  RTIResult endEdit() const;
  RTIResult destroy();

  bool operator==(const HandleBase& rhs) const;
  bool operator!=(const HandleBase& rhs) const;

protected:
  //---------- Constructors (protected) -----------------------

  HandleBase(int type);


  //---------- Methods ----------------------------------------

  bool createInstance();
  bool createInstance(int instance);

private:
  friend class HandleUtil;

  //---------- Types ------------------------------------------

  enum {
    TYPE_BITS       = 4,
    INSTANCE_BITS   = 32 - TYPE_BITS,
    INSTANCE_MASK   = ((0x1 << INSTANCE_BITS) - 1),
    INVALID_INSTACE = INSTANCE_MASK
  };


  //---------- Attributes -------------------------------------

  unsigned int m_type : 4;
  unsigned int m_instance : 28;
};


namespace handle_internal {
  RTAPI bool        isValid(HandleBase h);
  RTAPI void*       beginEdit(HandleBase h);
  RTAPI RTIResult   endEdit(HandleBase h);
  RTAPI RTIResult   destroy(HandleBase h);
  RTAPI int         createInstance(HandleBase h);
  RTAPI int         createInstance(HandleBase h, int instance);
}


//---------- HandleBase inline section ------------------------

inline HandleBase::HandleBase(const HandleBase& rhs)
    : m_type(rhs.m_type), m_instance(rhs.m_instance) {}


inline HandleBase::HandleBase(int type) : m_type(type), m_instance(INVALID_INSTACE) {}


inline HandleBase& HandleBase::operator=(const HandleBase& rhs) {
  m_type     = rhs.m_type;
  m_instance = rhs.m_instance;
  return (*this);
}


inline bool HandleBase::isValid() const {
  if (m_instance == INVALID_INSTACE) {
    return false;
  }
  return handle_internal::isValid(*this);
}


inline int HandleBase::getType() const {
  return int(m_type);
}


inline bool HandleBase::isOfType(EClassType type) const {
  return getType() == type;
}


inline int HandleBase::getInstance() const {
  return int(m_instance);
}


inline void* HandleBase::beginEdit() {
  return handle_internal::beginEdit(*this);
}


inline RTIResult HandleBase::endEdit() const {
  return handle_internal::endEdit(*this);
}


inline RTIResult HandleBase::destroy() {
  RTIResult result = handle_internal::destroy(*this);

  m_instance = INVALID_INSTACE;

  return result;
}


inline bool HandleBase::operator==(const HandleBase& rhs) const {
  return m_type == rhs.m_type && m_instance == rhs.m_instance;
}

inline bool HandleBase::operator!=(const HandleBase& rhs) const {
  return m_type != rhs.m_type || m_instance != rhs.m_instance;
}


inline bool HandleBase::createInstance() {
  m_instance = handle_internal::createInstance(getType());
  return isValid();
}


inline bool HandleBase::createInstance(int instance) {
  m_instance = handle_internal::createInstance(getType(), instance);
  return isValid();
}


//---------- END: HandleBase inline section -------------------


/**
* @class   TAbstractHandle
* @brief   Template for abstract object handles.
*
* @author  Henrik Edstrom
* @date    2008-01-12
*/
template <class T, class Parent>
class TAbstractHandle : public Parent {
public:
  //---------- Default constructor (creates invalid handle) ---

  TAbstractHandle() : Parent(T::TypeId) {}


  //---------- Methods ----------------------------------------

  T* beginEdit() { return (T*)HandleBase::beginEdit(); }

protected:
  //---------- Constructors -----------------------------------

  TAbstractHandle(int type) : Parent(type) {}


};  // TAbstractHandle


/**
* @class   THandle
* @brief   Template for object handles. Wraps casting to the correct interface.
*
* @author  Henrik Edstrom
* @date    2008-01-11
*/
template <class T, class Parent>
class THandle : public Parent {
public:
  //---------- Types ------------------------------------------

  typedef T ObjectType;


  //---------- Static methods ---------------------------------

  static THandle create() {
    THandle h(T::TypeId);
    h.createInstance();
    return h;
  }


  //---------- Default constructor (creates invalid handle) ---

  THandle() : Parent(T::TypeId) {}


  //---------- Methods ----------------------------------------

  T* beginEdit() { return (T*)HandleBase::beginEdit(); }


  RTIResult destroy() { return HandleBase::destroy(); }

protected:
  friend class HandleUtil;

  //---------- Constructors -----------------------------------

  THandle(int type) : Parent(type) {}


};  // THandle


template <class T>
inline T castHandle(const HandleBase& h) {
  T res;
  if (h.getType() == T::ObjectType::TypeId) {
    res.m_instance = h.m_instance;
  }
  return res;
}


/**
* @class   TEditPtr
* @brief   Helper template for scoping begin/end edit of handles.
*
* @author  Henrik Edstrom
* @date    2008-01-11
*/
template <class T>
class TEditPtr {
public:
  //---------- Types ------------------------------------------

  typedef T* pointer;
  typedef T& reference;


  //---------- Constructor ------------------------------------

  template <class H>
  explicit TEditPtr(H handle) : m_handle(handle) {
    m_pointer = (pointer)handle.beginEdit();
  }


  //---------- Destructor -------------------------------------

  ~TEditPtr() { m_handle.endEdit(); }


  //---------- Operators --------------------------------------

  pointer operator->() const { return m_pointer; }
  reference operator*() const { return *m_pointer; }
  operator pointer() const { return m_pointer; }

  TEditPtr& operator=(const T& rhs) {
    m_handle  = rhs.m_handle;
    m_pointer = rhs.m_pointer;
    return *this;
  }


private:
  //---------- Copy and assignment (undefined) ----------------

  TEditPtr(const TEditPtr& rhs);
  TEditPtr& operator=(const TEditPtr& rhs);


  //---------- Attributes -------------------------------------

  HandleBase m_handle;
  T*         m_pointer;

};  // TEditPtr


/**
* @class   HandleUtil
* @brief   Handle utility class. Only intended to be used by internal classes.
*
* @author  Henrik Edstrom
* @date    2013-02-22
*/
class HandleUtil {
public:
  template <typename T>
  static inline T create(int instance) {
    T h(T::ObjectType::TypeId);
    h.m_instance = instance;
    return h;
  }


  static inline HandleBase create(int type, int instance) {
    HandleBase h(type);
    h.m_instance = instance;
    return h;
  }


  template <typename T>
  static inline T instantiate(int instance) {
    T h(T::ObjectType::TypeId);
    h.createInstance(instance);
    return h;
  }
};

END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_HANDLE_H__
