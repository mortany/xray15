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
* @file    IMessageCallback.h
* @brief   Interface for client callbacks.
*
* @author  Henrik Edstrom
* @date    2008-01-09
*
*/

#ifndef __RTI_I_MESSAGE_CALLBACK_H__
#define __RTI_I_MESSAGE_CALLBACK_H__

// rti
#include "rti/core/Common.h"

///////////////////////////////////////////////////////////////////////////////
// rti::IMessageCallback //////////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   IMessageCallback
* @brief   Interface for client callbacks.
*
* @author  Henrik Edstrom
* @date    2008-01-09
*/
class IMessageCallback {
public:
  virtual ~IMessageCallback() {}

  virtual void message(int severity, const char* text) = 0;
  virtual void warning(const char* text) = 0;
  virtual void error(const char* text) = 0;
  virtual void fatal(const char* text) = 0;

};  // IMessageCallback

END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_I_MESSAGE_CALLBACK_H__
