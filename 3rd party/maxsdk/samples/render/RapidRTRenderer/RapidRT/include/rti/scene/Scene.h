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
* @file    Scene.h
* @brief   Public interface for scenes.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*
*/

#ifndef __RTI_SCENE_H__
#define __RTI_SCENE_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/scene/Camera.h"
#include "rti/scene/Framebuffer.h"
#include "rti/scene/Group.h"
#include "rti/scene/RenderOptions.h"
#include "rti/scene/Shader.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Scene /////////////////////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   Scene
* @brief   Public interface for scenes.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*/
class RTAPI Scene : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_SCENE)

  //---------- Methods ----------------------------------------

  void                  setSceneRoot(GroupHandle sceneRoot);
  void                  setCamera(CameraHandle camera);
  void                  setFramebuffer(FramebufferHandle framebuffer);
  void                  setEnvironmentShader(ShaderHandle shader);
  void                  setRenderOptions(RenderOptionsHandle options);

  GroupHandle           getSceneRoot() const;
  CameraHandle          getCamera() const;
  FramebufferHandle     getFramebuffer() const;
  ShaderHandle          getEnvironmentShader() const;
  RenderOptionsHandle   getRenderOptions() const;

};  // Scene


//---------- Handle type --------------------------------------

typedef THandle<Scene, ClassHandle> SceneHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_SCENE_H__
