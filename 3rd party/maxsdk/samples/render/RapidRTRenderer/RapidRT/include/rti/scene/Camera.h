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
* @file    Camera.h
* @brief   Public interface for cameras.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*
*/

#ifndef __RTI_CAMERA_H__
#define __RTI_CAMERA_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"
#include "rti/math/Math.h"
#include "rti/scene/Entity.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Camera ////////////////////////////////////////////////////////////////

BEGIN_RTI


//---------- Types --------------------------------------------

enum ECameraType { 
  CAMERA_PERSPECTIVE, 
  CAMERA_ORTHOGONAL, 
  CAMERA_PROJECTION,
  CAMERA_IRRADIANCE_SENSOR
};


enum ECameraParameter {
  CAMERA_PARAM_FRAME_NUMBER,
  CAMERA_PARAM_SHUTTER_OPEN,
  CAMERA_PARAM_SHUTTER_CLOSE,
  CAMERA_PARAM_TONEOP,
  CAMERA_PARAM_EXPOSURE,
  CAMERA_PARAM_BURN_HIGHLIGHTS,
  CAMERA_PARAM_CRUSH_BLACKS,
  CAMERA_PARAM_SATURATION,
  CAMERA_PARAM_GAMMA,
  CAMERA_PARAM_VIGNETTING,
  CAMERA_PARAM_WHITE_POINT,
  CAMERA_PARAM_PHYSICAL_SCALE,
  CAMERA_PARAM_CONTRAST,
  CAMERA_PARAM_MID_TONES,
  CAMERA_PARAM_COLOR_CORRECTION,
  CAMERA_PARAM_COLOR_DIFFERENTIATION,
  CAMERA_PARAM_COLOR_MAP_INDEPENDENT,
  CAMERA_PARAM_EXTERIOR_DAYLIGHT,
  CAMERA_PARAM_CANON_EXPOSURE_BASE,
  CAMERA_PARAM_CANON_EXPOSURE_OFFSET,
  CAMERA_PARAM_CANON_COLOR_PRESERVING
};


enum EToneOpType {
  TONEOP_NONE,
  TONEOP_REINHARD,
  TONEOP_PHOTOGRAPHIC,
  TONEOP_LOGARITHMIC,
  TONEOP_CANON
};


/**
* @class   Camera
* @brief   Public interface for cameras.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*/
class RTAPI Camera : public Entity {
public:
  DECL_INTERFACE_CLASS(CTYPE_CAMERA)

  //---------- Methods ----------------------------------------

  void          initPerspective(float fov);
  void          initOrthographic(float aperture);
  void          initProjection(Vec3f lowerLeft, Vec3f lowerRight, Vec3f upperLeft);
  void          initIrradianceSensor(Vec3f lowerLeft, Vec3f lowerRight, Vec3f upperLeft);

  void          setResolution(int width, int height);
  void          setAspectRatio(float aspectRatio);
  void          setDepthOfField(float focusDistance, float lensRadius);
  void          setOffset(float offsetX, float offsetY);
  void          setWindowRegion(int x0, int y0, int x1, int y1);
  void          setBackgroundShader(ShaderHandle backgroundShader);

  RTIResult     setParameter1b(ECameraParameter param, bool value);
  RTIResult     setParameter1i(ECameraParameter param, int value);
  RTIResult     setParameter1f(ECameraParameter param, float value);
  RTIResult     setParameter3f(ECameraParameter param, Vec3f value);


  ECameraType   getCameraType() const;
  int           getWidth() const;
  int           getHeight() const;
  float         getAspectRatio() const;
  float         getFieldOfView() const;
  float         getAperture() const;
  void          getProjection(Vec3f* lowerLeft, Vec3f* lowerRight, Vec3f* upperLeft) const;
  float         getFocusDistance() const;
  float         getLensRadius() const;
  float         getOffsetX() const;
  float         getOffsetY() const;
  void          getWindowRegion(int* x0, int* y0, int* x1, int* y1) const;

};  // Camera


//---------- Handle type --------------------------------------

typedef THandle<Camera, EntityHandle> CameraHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_CAMERA_H__
