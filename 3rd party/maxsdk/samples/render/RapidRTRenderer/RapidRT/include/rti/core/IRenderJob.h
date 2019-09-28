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
* @file    IRenderJob.h
* @brief   Encapsulates a render job.
*
* @author  Henrik Edstrom
* @date    2013-03-07
*
*/

#ifndef __RTI_I_RENDER_JOB_H__
#define __RTI_I_RENDER_JOB_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"
#include "rti/scene/Scene.h"


///////////////////////////////////////////////////////////////////////////////
// rti::IRenderJob ////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Forward declarations -----------------------------

class IDevice;


/**
* @class   IRenderJob
* @brief   Encapsulates a render job.
*
* @author  Henrik Edstrom
* @date    2013-03-07
*/
class RTAPI IRenderJob {
public:
  virtual ~IRenderJob() {}

  virtual RTIResult addDevice(IDevice* device) = 0;
  virtual RTIResult removeDevice(IDevice* device) = 0;

  virtual RTIResult sync(int timeoutMsecs = RTI_MAX_TIMEOUT, bool flush = false) = 0;
  virtual RTIResult frame(SceneHandle sceneHandle, bool buffered = true) = 0;
  virtual RTIResult abort() = 0;

  virtual RTIResult query1b(EQuery query, bool& value) const = 0;
  virtual RTIResult query1i(EQuery query, int& value) const = 0;
  virtual RTIResult query1f(EQuery query, float& value) const = 0;

};  // IDevice


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_I_RENDER_JOB_H__
