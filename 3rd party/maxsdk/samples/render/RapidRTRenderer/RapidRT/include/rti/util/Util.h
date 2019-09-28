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
* @file    Util.h
* @brief   Utility functions.
*
* @author  Henrik Edstrom
* @date    2008-01-27
*
*/

#ifndef __RTI_UTIL_H__
#define __RTI_UTIL_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Common.h"
#include "rti/math/Math.h"
#include "rti/scene/Framebuffer.h"


///////////////////////////////////////////////////////////////////////////////
// rti ////////////////////////////////////////////////////////////////////////

BEGIN_RTI

struct FilteringParams;


/**
* @class   BloomFilteringParams
* @brief   Input parameters to the bloom filter.
*
* @author  Nicolas Savva
* @date    2016-12-05
*/
struct BloomFilteringParams {
  // Image buffer dimensions.
  int width;
  int height;
  int components;

  // The image buffer should contain interleaved 3-component RGB floating point image data in
  // row-major order. I.e. Image(x, y).g can be found at Image[3*width*y + 3*x + 1].

  // Input Image
  const float* inputImage;

  // Amount of bloom component to be added.
  // Valid range: [0.0, 1.0]
  float alpha;

  // Conversion factor between pixels and bloom filter kernel angle units.
  float angleToPx;
};

/**
 * @class   FeatureAwareFilteringParams
 * @brief   Input parameters to the feature-aware denoising filter.
 *
 * @author  Karl Schmidt
 * @date    2016-09-21
 */
struct FeatureAwareFilteringParams {
  // Image and feature buffer dimensions.
  int width;
  int height;

  // The feature buffers should contain interleaved 3-component RGB floating point image data in
  // row-major order. I.e. Albedo(x, y).g can be found at albedo[3*width*y + 3*x + 1].

  // Diffuse material albedo.
  const float* albedo;

  // Radiance from emission.
  const float* emission;

  // Radiance from diffuse BSDFs, even samples.
  const float* diffuse0;

  // Radiance from diffuse BSDFs, odd samples.
  const float* diffuse1;

  // Radiance from non-diffuse BSDFs, even samples.
  const float* specular0;

  // Radiance from non-diffuse BSDFs, odd samples.
  const float* specular1;

  // World space position.
  const float* position;

  // World space normal.
  const float* normal;

  // Extra features. Currently the first channel is roughness.
  const float* extra;

  // Size of the feature window. A bigger window investigates more surrounding pixels for
  // similarity in the final filter. This has a high impact on performance.
  // Valid range: [0, 40]
  int windowRadius = 8;

  // Size of the variance averaging window. A bigger window averages the feature variance over a
  // larger area.
  // Valid range: [0, 40]
  int bilateralWindowRadius = 10;

  // Size of the comparison tile. A bigger comparison tile uses a larger region for determining
  // the characteristics of a pixel. This has a high impact on performance.
  // Valid range: [0, 10]
  int tileRadius = 3;

  // Strength of the feature variance filtering. A higher value indicates more aggressive
  // bilateral filtering of the diffuse and specular variance buffers.
  // Valid range: [0.0, 1.0]
  float featureFilterStrength = 0.1f;

  // Strength of the final radiance reconstruction filter. A higher value indicates
  // more aggressive filtering, which may remove more noise at the cost of more blur.
  // Valid range: [0.0, 1.0]
  float colorFilterStrength = 0.4f;

  // Strength of the roughness feature (essentially the standard deviation of the Gaussian
  // that judges the similarity of two roughness values).
  // Valid range: [0.0, 1.0]
  float roughnessFilterStrength = 0.001f;

  // Strength of the bilateral filter for variance. This is meant for preventing overblurring
  // of the variance estimate on very bright highlights (e.g. sun).
  float bilateralFilterParam = 5000;
};

/**
* @class   PhotographicToneOpParams
* @brief   Input parameters for the Photographic ToneOp.
*
* @author  Nicolas Savva
* @date    2017-02-27
*/
struct PhotographicToneOpParams {
  // Photographic Tonemap parameters

  // Radial vignetting factor  Valid range: [0.0, 1.0]
  float vignetting;

  // Numerator parameter in Reinhard style compression curve
  // Photographic default 0.0. Set to 1.0 for linear curve compression.
  float burnHighlights;

  // Power exponent parameter used for non-linear compression of darker color values
  float crushBlacks;
  // Saturation parameter Valid range: [0.0, 1.0]. Output grayscale image when set to zero.
  float saturation;
  // Gamma-like power scaling parameter.
  float midtones;
  // Exposure scale to apply initially after all other toneop operations.
  float postMultiply;
  // Exposure scale to apply before any other toneop operation.
  float preMultiply;
  // Toggle on to scale all channels by the same luminance dependent value
  // instead of just the equivalent per color channel computation. The ratio of a linear
  // mix of the latter to the former smoothly increases with pixel overexposure.
  bool colorPreserving;
};

// Enum for selecting different color correction modes.
enum EChromaticAdaptation { E_DIAGONAL_SCALE = 0, E_BRADFORD_ADAPTATION };



// TODO (savvan): Move BufferViews to a different place
/**
 * @class   ConstBufferView
 * @brief   A non-owning representation of an immutable image buffer.
 *
 * @author  Karl Schmidt
 * @date    2017-05-26
 */
template <class T>
class ConstBufferView {
public:
  ConstBufferView(const T* data, int width, int height, int components)
      : m_data(data), m_width(width), m_height(height), m_components(components){};

  const T* buffer() const { return m_data; };

  int getWidth() const { return m_width; };
  int getHeight() const { return m_height; };
  int getComponents() const { return m_components; };
  int getNumPixels() const { return m_width * m_height; };

private:
  const T* m_data;

  int m_width;
  int m_height;
  int m_components;
};

typedef ConstBufferView<float> ConstFloatBufferView;


/**
 * @class   BufferView
 * @brief   A non-owning representation of a mutable image buffer.
 *
 * @author  Karl Schmidt
 * @date    2017-05-26
 */
template <class T>
class BufferView : public ConstBufferView<T> {
public:
  BufferView(T* data, int width, int height, int components)
      : ConstBufferView<T>(data, width, height, components), m_data(data){};

  T* buffer() { return m_data; };

private:
  T* m_data;
};

typedef BufferView<float> FloatBufferView;


class RTAPI Util {
public:
  //---------- Methods ----------------------------------------

  // Filter the data from a swap callback to remove sampling noise in the image.
  // This call is blocking and can take some time to execute (several seconds).
  // This call should not be made on every swap callback as that will slow down
  // rendering (lots of CPU will be devoted to filtering). It should ideally be
  // used when you have a somewhat noisy image that you want to denoise before
  // presenting to the user.
  RTI_DEPRECATED static RTIResult applyDenoiseFilter(void*                  dstBuffer,
                                                     const FilteringParams& filteringParams);

  // Abort a currently running denoise filter
  RTI_DEPRECATED static RTIResult abortDenoiseFilter(bool blocking = true);

  // Denoise filtering using the feature aware denoise filtering kernel. This
  // requires that the FeatureAwareFilteringParams struct be filled out
  // manually using the appropriate set of AOV outputs.
  // Note: pUnfilteredDstBuffer is currently not written to.
  RTI_DEPRECATED static RTIResult applyFeatureAwareFilter(
      FeatureAwareFilteringParams* filteringParams, void* pUnfilteredDstBuffer,
      void* pFilteredDstBuffer);

  // Bloom filtering using four gaussian fit Spencer kernel approximation. This
  // requires that the BloomFilteringParams struct be filled out
  // manually using the appropriate set of AOV outputs.
  RTI_DEPRECATED static RTIResult applyBloomFilter(BloomFilteringParams* filteringParams,
                                                   void*                 pFilteredDstBuffer);

  // Adjust exposure of floating point RGBA buffer. In place application is supported.
  RTI_DEPRECATED static void adjustExposure(const void* srcBuffer, void* dstBuffer, int numPixels,
                                            float exposure);

  // Apply gamma 2.2 to buffer. In place application is supported.
  RTI_DEPRECATED static void applyGamma(const void* srcBuffer, void* dstBuffer, int numPixels);

  // Apply Canon tone operator to buffer. In place application is supported.
  RTI_DEPRECATED static void applyCanonToneOp(EFramebufferFormat srcFormat,
                                              EFramebufferFormat dstFormat, const void* srcBuffer,
                                              void* dstBuffer, int width, int height,
                                              bool colorPreserving, float exposureBase,
                                              float exposureOffset);

  class RTAPI PostProcess {
  public:
    /// @brief Denoise filtering using the feature aware denoise filtering kernel.
    /// @param filteringParams Buffers and other settings used by the feature-aware denoiser.
    /// @param[out] dstBuffer View of the output buffer.
    static RTIResult applyFeatureAwareFilter(const FeatureAwareFilteringParams& filteringParams,
                                             FloatBufferView&                   dstBuffer);

    /// @brief Apply bloom filtering using four gaussian fit Spencer kernel approximation.
    /// @param bloomFilterParams struct containing the settings and input buffer for the bloom filter
    /// @param[out] filteredDstBuffer View of the output buffer.
    static RTIResult applyBloomFilter(const BloomFilteringParams& bloomFilterParams,
                                      FloatBufferView&            filteredDstBuffer);

    /// @brief Apply exposure adjustment to buffer.
    /// @param exposureValue Exponent for 2**exposureValue scaling value.
    /// @param srcBuffer View of the input buffer.
    /// @param[out] dstBuffer View of the output buffer. This may alias \p srcBuffer for in-place application.
    static RTIResult adjustExposure(float exposureValue, const ConstFloatBufferView& srcBuffer,
                                    FloatBufferView& dstBuffer);

    /// @brief Compute log-average auto exposure value and apply to buffer.
    /// @param srcBuffer View of the input buffer.
    /// @param[out] dstBuffer View of the output buffer. This may alias \p srcBuffer for in-place application.
    static RTIResult autoExposure(const ConstFloatBufferView& srcBuffer,
                                  FloatBufferView&            dstBuffer);

    /// @brief Apply color correction to buffer.
    /// @param chromaticAdaptationType Enum to select between different color correction modes
    /// {diagonal scaling or Bradford chromatic adaptation} supported for now.
    /// @param srcBuffer View of the input buffer.
    /// @param[out] dstBuffer View of the output buffer. This may alias \p srcBuffer for in-place application.
    /// @param whitePoint Desired color to map to white during color correction.
    static RTIResult applyColorCorrection(EChromaticAdaptation chromaticAdaptationType,
                                          Vec3f whitePoint, const ConstFloatBufferView& srcBuffer,
                                          FloatBufferView& dstBuffer);

    /// @brief Apply gamma 2.2 to buffer.
    /// @param srcBuffer View of the input buffer.
    /// @param[out] dstBuffer View of the output buffer. This may alias \p srcBuffer for in-place application.
    static RTIResult applyLinearToSrgbGamma(const ConstFloatBufferView& srcBuffer,
                                            FloatBufferView&            dstBuffer);

    /// @brief Apply customizable universal curve compression tone operator to buffer.
    /// @param photographicToneOpParams struct containing the settings for the augmented photographic toneop.
    /// @param srcBuffer View of the input buffer.
    /// @param[out] dstBuffer View of the output buffer. This may alias \p srcBuffer for in-place application.
    static RTIResult applyCurveCompression(const PhotographicToneOpParams& photographicToneOpParams,
                                           const ConstFloatBufferView&     srcBuffer,
                                           FloatBufferView&                dstBuffer);

    /// @brief Compute srgb color from blackbody temperature (approximation valid 1667K to 25000K temperature range).
    /// @param colorTemperature input blackbody temperature.
    /// @param[out] color output srgb color.
    static RTIResult blackbodyKelvinToSrgb(float colorTemperature, Vec3f& color);

    /// @brief Compute log-average auto exposure value.
    /// @param srcBuffer View of the buffer.
    /// @param[out] exposureValue auto exposure value.
    static RTIResult autoExposureExponent(const ConstFloatBufferView& srcBuffer,
                                          float&                      exposureValue);
  };  // class PostProcess

  class RTAPI Convergence {
  public:
    //---------- Methods ----------------------------------------

    // Gives convergence as an index. Have a look in
    // "Frambuffer.h / ConvergenceParams / m_fConvergence"
    // for a detailed description. The input is convergence in dB.
    static float calculateConvergence(float convergence /*dB*/);

    static float calculateConvergencePSNR(float index);
  };

};  // class Util


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_UTIL_H__
