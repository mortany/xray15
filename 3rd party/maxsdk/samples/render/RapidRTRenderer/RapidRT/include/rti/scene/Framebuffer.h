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
* @file    Framebuffer.h
* @brief   Public interface for framebuffers.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*
*/

#ifndef __RTI_FRAMEBUFFER_H__
#define __RTI_FRAMEBUFFER_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"

///////////////////////////////////////////////////////////////////////////////
// rti::Framebuffer ///////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Types --------------------------------------------

enum EFramebufferFormat { 
  FB_FORMAT_RGBA_F32, 
  FB_FORMAT_RGBA_8, 
  FB_FORMAT_LOGYUV_16_8_8 
};


enum EFramebufferMode { 
  FB_MODE_PROGRESSIVE, 
  FB_MODE_FINAL, 
  FB_MODE_FINAL_TONE_MAPPED 
};


enum EFramebufferOutputType {
  FB_OUTPUT_INVALID  = -1,
  FB_OUTPUT_POSITION = 0,
  FB_OUTPUT_SHADING_NORMAL,
  FB_OUTPUT_GEOMETRY_NORMAL,
  FB_OUTPUT_DEPTH,
  FB_OUTPUT_ALPHA,

  FB_OUTPUT_DIFFUSE_ALBEDO,
  FB_OUTPUT_EMISSION,
  FB_OUTPUT_DIFFUSE_ODD,
  FB_OUTPUT_DIFFUSE_EVEN,
  FB_OUTPUT_SPECULAR_ODD,
  FB_OUTPUT_SPECULAR_EVEN,
  FB_OUTPUT_EXTRA,

  FB_NUM_OUTPUT_TYPES
};


enum EFramebufferOutputFormat { 
  FB_OUTPUT_FORMAT_FLOAT 
};


struct SwapParams;


/**
* @class   IFramebufferCallback
* @brief   Swap callback.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*/
class IFramebufferCallback {
public:
  virtual ~IFramebufferCallback() {}
  virtual void swap(const SwapParams& params) = 0;
};


/**
* @class   Framebuffer
* @brief   Public interface for framebuffers.
*
* @author  Henrik Edstrom
* @date    2008-01-10
*/
class RTAPI Framebuffer : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_FRAMEBUFFER)

  //---------- Methods ----------------------------------------

  void                    setFormat(EFramebufferFormat format);
  void                    setMode(EFramebufferMode mode);
  void                    setSwapCallback(IFramebufferCallback* callback);

  int                     addOutput(EFramebufferOutputType outputType);
  RTIResult               setOutput(int i, EFramebufferOutputType outputType);
  RTIResult               removeOutput(int i);
  RTIResult               removeAllOutputs();

  int                     getNumOutputs() const;
  EFramebufferOutputType  getOutputType(int i) const;

};  // Framebuffer


/**
* @class   FramebufferOutput
* @brief   Data associated with an output pass / AOV.
*
* @author  Henrik Edstrom
* @date    2015-10-14
*/
struct FramebufferOutput {
  EFramebufferOutputType    m_type;
  EFramebufferOutputFormat  m_format;
  int                       m_numChannels;
  void*                     m_data;
};


/**
* @class   FilteringParams
* @brief   Parameters for the noise filtering block of the SwapParams.
*
* @author  Peter Rundberg
* @date    2015-03-31
*/
struct FilteringParams {
  int                 m_width;
  int                 m_height;
  EFramebufferFormat  m_format;
  EFramebufferMode    m_mode;
  int                 m_toneOpType;
  const void*         m_camera;

  // Histogram data layout
  int                 m_numFilterColorChannels;
  int                 m_numFilterBuckets;

  float               m_convergencePsnr;

  // Exposure level at where the image is
  // meant to be displayed.
  float               m_exposure;

  // The outlier clamp used to render the image
  float               m_outlierClamp;

  // Un-tonemapped pixel data
  int                 m_pixelDataBufferSize;
  const void*         m_pixelData;

  // Buffer of samples per pixel
  int                 m_sampleCountBufferSize;
  const float*        m_sampleCountData;

  // Buffer of histogram data used for filtering
  int                 m_filterDataBufferSize;
  const void*         m_filterData;
};


/**
* @class   ConvergenceParams
* @brief   Contains convergence statistics.
*
* @author  Henrik Edstrom
* @date    2015-05-12
*/
struct ConvergenceParams {
  // Noise metrics. Negative values indicate invalid data.
  float m_avgPSNR;
  float m_minPSNR;
  float m_convergencePSNR;

  // Gives Convergence as an index. Some values of the
  // index is given in the namespace rti::ConvergenceIndex.
  // Zero indicates total chaos and 100 indicates excellent
  // convergence. An index of 200 indicates that the refined
  // image has half the noise as an image with index 100.
  // The error is linear. For example, an image with the
  // index of 37.5f has a noise level which is the mean value
  // of draft and medium convergence.
  float m_convergence;

  // Gives the progress towards the set Convergence Target.
  // Please note that this can be used to calculate the estimated
  // time to a completed image with the set convergence target.
  // Progress is near linear up to the convergence target. This
  // equals 1.0 on the scale. The progress over 1.0 measures how
  // much the image has improved over the target, so 2.0 would
  // indicate an image that has the noise reduced by half.
  // The progress starts at 0.0.
  float m_progress;

  // Estimated time until the image reaches the convergence target.
  float m_timeToImage;

  // Internal data to reconstruct the error metrics.
  struct Internal {
    int         m_targetType;
    int         m_iterationTarget;
    float       m_convergenceTarget;  // dB
    int         m_kernel;
    const void* m_pixelBuffer;
    const void* m_extraBuffer;
  } m_internal;
};


/**
* @class   SwapParams
* @brief   Parameters to the swap callback.
*
* @author  Henrik Edstrom
* @date    2008-11-28
*/
struct SwapParams {
  int                   m_frameId;

  int                   m_width;
  int                   m_height;
  EFramebufferFormat    m_format;

  EFramebufferMode      m_mode;
  bool                  m_progressiveDone;
  int                   m_iteration;
  float                 m_avgSamplesPerPixel;

  int                   m_interlaceFactor;
  int                   m_interlaceOffset;

  int                   m_regionOriginX;
  int                   m_regionOriginY;
  int                   m_regionWidth;
  int                   m_regionHeight;

  int                   m_bytesPerPixel;
  int                   m_bufferSize;

  bool                  m_filteringEnabled;
  FilteringParams       m_filteringParams;
  ConvergenceParams     m_convergenceParams;

  const void*           m_pixelData;
  const float*          m_sampleCountData;

  int                   m_numOutputs;
  FramebufferOutput*    m_outputs;
};


//---------- Handle type --------------------------------------

typedef THandle<Framebuffer, ClassHandle> FramebufferHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_FRAMEBUFFER_H__
