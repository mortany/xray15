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
* @file    RenderOptions.h
* @brief   Public interface for render options.
*
* @author  Henrik Edstrom
* @date    2008-02-05
*
*/

#ifndef __RTI_RENDER_OPTIONS_H__
#define __RTI_RENDER_OPTIONS_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/math/Math.h"


///////////////////////////////////////////////////////////////////////////////
// rti::RenderOptions /////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Types --------------------------------------------

enum EKernel {
  KERNEL_PATH_TRACE       = 0,
  KERNEL_PATH_TRACE_FAST  = 1,
  KERNEL_LOW_NOISE        = 2,
  KERNEL_LOW_NOISE_DIRECT = 3,
  NUM_KERNELS             = 4
};


enum EFilterType {
  FILTER_BOX,
  FILTER_TRIANGLE,
  FILTER_GAUSSIAN,
  FILTER_LANCZOS_CLIPPED,
  FILTER_MITCHELL_CLIPPED,

  NUM_FILTERS
};


enum EAlphaType {
  // Do not compute an alpha channel.
  ALPHA_OFF,

  // Compute a color FG and alpha that should be blended with the background BG
  // as (1-alpha)*BG + FG.
  //
  // FG includes illumination for matte objects. This is the preferred option
  // if you are compositing the rendering on a background yourself, however
  // the premultiplication cannot be reversed to arrive at an unassociated
  // alpha value to save to e.g. a PNG file.
  ALPHA_PREMULTIPLIED_WITH_MATTE_ILLUMINATION,

  // Compute a color FG and alpha that should be blended with the background BG
  // as (1-alpha)*BG + FG.
  //
  // FG does not include any illumination for matte objects, which means that
  // the premultiplication can be reversed. A PNG file storing FG/alpha as its
  // color will for instance compose correctly.
  ALPHA_PREMULTIPLIED,

  // The alpha channel will store a mask of the foreground (i.e. not ATTRIB_MATTE_OBJECT or
  // ATTRIB_BACKGROUND) objects in the rendering. The color channel will be identical to ALPHA_OFF.
  ALPHA_OBJECT_MASK
};


enum ESceneType { 
  SCENE_STATIC, 
  SCENE_DYNAMIC 
};


enum ETargetType { 
  TARGET_CONVERGENCE, 
  TARGET_ITERATION 
};


enum EIrradianceOverrideMode {
  IRRADIANCE_OVERRIDE_OFF,
  IRRADIANCE_OVERRIDE_PRIMARY_HIT,
  IRRADIANCE_OVERRIDE_SENSOR_GRID
};


enum ERenderOption {
  OPTION_KERNEL,                          // int
  OPTION_PROGRESSIVE_BASE_ITERATION,      // int
  OPTION_PROGRESSIVE_MAX_ITERATIONS,      // int

  OPTION_FILTER_TYPE,                     // int
  OPTION_FILTER_WIDTH,                    // float
  OPTION_FILTER_HEIGHT,                   // float

  OPTION_PRECISION_HINT,                  // float [0.0, 1.0]

  OPTION_ALPHA_CHANNEL,                   // int

  OPTION_INTERLACE_FACTOR,                // int

  OPTION_BLEND_KERNEL_ITERATIONS,         // int
  OPTION_LOW_NOISE_SHADOW_SAMPLES,        // int

  OPTION_SCENE_TYPE,                      // int

  // The outlier clamp input has two components.
  // The first is a clamp value and the second is an exposure hint value.
  // If the exposure hint value is set to zero, then the clamp value will
  // be the only value used. If the clamp value is zero, then no clamping
  // will be done. The clamp value is not used in automatic outlier clamp mode.
  // The automatic outlier clamp feature requires that an exposure has been set.
  OPTION_AUTOMATIC_OUTLIER_CLAMP,         // bool
  OPTION_OUTLIER_CLAMP,                   // float

  // The exposure hint is used to indicate to the renderer at around what
  // exposure the image is meant to be displayed. The exposure hint is used
  // to perform proper clamping for the outlier clamp option above as well
  // as to allow the noise filtering option below to work at the correct
  // visible range. The exposure hint doesn't actually change the exposure
  // of the rendered image.
  OPTION_EXPOSURE_HINT,                   // float

  OPTION_TARGET_TYPE,                     // int
  OPTION_CONVERGENCE_TARGET,              // float
  OPTION_ITERATION_TARGET,                // int

  OPTION_ADAPTIVE_SAMPLING,               // bool

  OPTION_NOISE_FILTERING,                 // bool

  // Enable a rougher environment approximation for early iterations. Enabling
  // this will make changes to the environment, like rotating it, altering
  // brightness or moving directional lights, more responsive. However, it will
  // introduce some bias for early iterations.
  OPTION_APPROXIMATE_ENVIRONMENT,         // bool

  OPTION_REFRACTIVE_CONTINUATION,         // bool

  OPTION_IRRADIANCE_OVERRIDE_MODE,        // int

  NUM_RENDER_OPTIONS
};

namespace ConvergenceLevel /* dB */ {
  const float Draft      = 20.0f;
  const float Medium     = 28.0f;
  const float Production = 33.0f;
  const float Excellent  = 37.0f;
}

namespace ConvergenceIndex {
  const float Draft      = 25.0f;
  const float Medium     = 50.0f;
  const float Production = 75.0f;
  const float Excellent  = 100.0f;
}

/**
* @class   RenderOptions
* @brief   Public interface for render options.
*
* @author  Henrik Edstrom
* @date    2008-02-05
*/
class RTAPI RenderOptions : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_RENDER_OPTIONS)

  //---------- Methods ----------------------------------------

  RTIResult setOption1b(ERenderOption option, bool value);
  RTIResult setOption1i(ERenderOption option, int value);
  RTIResult setOption1f(ERenderOption option, float value);

  RTIResult setOption2f(ERenderOption option, Vec2f value);

};  // RenderOptions


//---------- Handle type --------------------------------------

typedef THandle<RenderOptions, ClassHandle> RenderOptionsHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_RENDER_OPTIONS_H__
