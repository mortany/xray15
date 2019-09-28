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
* @file    Common.h
* @brief
*
* @author  Henrik Edstrom
* @date    2008-01-11
*
*/

#ifndef __RTI_COMMON_H__
#define __RTI_COMMON_H__


///////////////////////////////////////////////////////////////////////////////
// Macros /////////////////////////////////////////////////////////////////////

#ifdef _WIN32
  #define RTI_DEPRECATED __declspec(deprecated)
#else
  #define RTI_DEPRECATED __attribute__((deprecated))
#endif

// FIXME: move this to platform.h
#ifdef RTAPI_BUILDING_LIB
  #ifdef _WIN32
    #define RTAPI __declspec(dllexport)
  #else
    #define RTAPI __attribute__((visibility("default")))
  #endif
#else
  #ifndef RTAPI_INTERNAL_ONLY
    #ifdef _WIN32
      #define RTAPI __declspec(dllimport)
    #else
      #define RTAPI __attribute__((visibility("default")))
    #endif
  #else
    #define RTAPI
  #endif
#endif

#define BEGIN_RTI namespace rti {
#define END_RTI }


#define DECL_INTERFACE_CLASS(ClassId) \
public:                               \
  static const EClassType TypeId = ClassId;

#define DECL_ROOT_INTERFACE_CLASS(ClassId) \
protected:                                 \
  friend class ClassPrivate;               \
                                           \
class ClassPrivate* m_classPrivate;        \
  DECL_INTERFACE_CLASS(ClassId)


///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// rti ////////////////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Return status type for all functions -------------

typedef int RTIResult;

enum {
  RTI_ERROR                    = -1,  // generic error
  RTI_SUCCESS                  = 0,
  RTI_INVALID_HANDLE           = 0x1000,
  RTI_INVALID_ENUM             = 0x1001,
  RTI_INVALID_VALUE            = 0x1002,
  RTI_INVALID_OPERATION        = 0x1003,
  RTI_OUT_OF_MEMORY            = 0x1004,
  RTI_SYNC_TIMEOUT             = 0x2000,
  RTI_CLUSTER_CONNECTION_ERROR = 0x3000,
  RTI_FILTER_ABORTED           = 0x4000
};

#define RTI_FAILED(res) (res != RTI_SUCCESS)


//---------- Class Type Ids -----------------------------------

enum EClassType {
  CTYPE_UNDEFINED         = 0,
  CTYPE_CLASS             = 1,
  CTYPE_SCENE             = 2,
  CTYPE_FRAMEBUFFER       = 3,
  CTYPE_ENTITY            = 4,
  CTYPE_CAMERA            = 5,
  CTYPE_GROUP             = 6,
  CTYPE_OBJECT            = 7,
  CTYPE_SHADER            = 8,
  CTYPE_GEOMETRY          = 9,
  CTYPE_TEXTURE           = 10,
  CTYPE_RENDER_OPTIONS    = 11,
  CTYPE_RENDER_ATTRIBUTES = 12
};


//---------- Queries ------------------------------------------

enum EQuery {
  QUERY_RENDER_PROGRESS,
  QUERY_FRAME_DIRTY,
  QUERY_LAST_FRAME_ID,
  QUERY_ACTIVE_FRAMES,
  QUERY_MEMORY_USAGE  // query RapidRT memory usage in megabytes
};


//---------- Notifications ------------------------------------

enum EMessageLevel {
  MSG_IMPORTANT = 0,
  MSG_DEFAULT   = 1,
  MSG_MINOR     = 2,
  MSG_DEBUG_1   = 3,
  MSG_DEBUG_2   = 4,
  MSG_DEBUG_3   = 5
};


//---------- Status of memory ---------------------------------

enum EMemoryStatus {
  RTI_MEMORY_OK                     = 0,  // Memory is fine
  RTI_OUT_OF_MEMORY_SOFT_LIMIT      = 1,  // User set soft memory limit reached
  RTI_OUT_OF_MEMORY_HARD_LIMIT      = 2,  // Hard memory limit reached, fatal error
  RTI_OUT_OF_MEMORY_ON_DEMAND_LIMIT = 3   // User set on demand geometry limit reached
};


//---------- Constants ----------------------------------------

const int RTI_MAX_TIMEOUT = 2147483647;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_COMMON_H__
