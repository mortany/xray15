//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Translator_PhysicalCameraAperture_to_RTITexture.h"

// local
#include "../resource.h"
// Max SDK
#include <dllutilities.h>
#include <Scene/IPhysicalCamera_BitmapApertureSampler.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK;
using namespace MaxSDK::NotificationAPI;

namespace
{
    // Accessor used to provide aperture bitmap pixel values to the sampler
    class ApertureBitmapAccessor : public IPhysicalCamera_BitmapApertureSampler::IApertureBitmapAccess
    {
    public:
        ApertureBitmapAccessor(Texmap& texmap, const IPoint2& camera_resolution, const TimeValue t, Interval& validity)
        {
            // Bake the texture
            // Default bake resolution is that of the camera
            BaseTranslator_Texmap::BakeTexmap(texmap, t, validity, camera_resolution, m_resolution, m_pixel_data);
        }
        virtual ~ApertureBitmapAccessor()
        {
        }
        virtual unsigned int get_x_resolution() const override
        {
            return m_resolution.x;
        }
        virtual unsigned int get_y_resolution() const override
        {
            return m_resolution.y;
        }
        virtual IPhysicalCamera_BitmapApertureSampler::RGBValue get_pixel_value(const unsigned int x, const unsigned int y) const override
        {
            if(DbgVerify((x < m_resolution.x) && (y < m_resolution.y)))
            {
                const BMM_Color_fl& pixel = m_pixel_data[(y * m_resolution.x) + x];
                return IPhysicalCamera_BitmapApertureSampler::RGBValue(pixel.r, pixel.g, pixel.b);
            }
            else
            {
                return IPhysicalCamera_BitmapApertureSampler::RGBValue(0.0f, 0.0f, 0.0f);
            }
        }
    private:
        std::vector<BMM_Color_fl> m_pixel_data;
        IPoint2 m_resolution;
    };
}

//==================================================================================================
// class Translator_PhysicalCameraAperture_to_RTITexture
//==================================================================================================

Translator_PhysicalCameraAperture_to_RTITexture::Translator_PhysicalCameraAperture_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Texmap(key.GetData().aperture_texmap, translator_graph_node),
    BaseTranslator_to_RTITexture(translator_graph_node),
    Translator(translator_graph_node),
    m_key(key.GetData())
{

}

Translator_PhysicalCameraAperture_to_RTITexture::~Translator_PhysicalCameraAperture_to_RTITexture()
{

}

TranslationResult Translator_PhysicalCameraAperture_to_RTITexture::Translate(const TimeValue translationTime, Interval& newValidity, ITranslationProgress& /*translationProgress*/, KeyframeList& /*keyframesNeeded*/)
{
    if(m_key.aperture_texmap != nullptr)
    {
        // Initialize the bitmap sampler data
        ApertureBitmapAccessor bitmap_aperture_accessor(*m_key.aperture_texmap, m_key.camera_resolution, translationTime, newValidity);
        IPhysicalCamera_BitmapApertureSampler bitmap_aperture_sampler(bitmap_aperture_accessor, m_key.center_bias, m_key.affects_exposure);

        // Create rti::Textures to hold the sampler data
        if(bitmap_aperture_sampler.is_sampler_valid())
        {
            const std::vector<IPhysicalCamera_BitmapApertureSampler::RGBValue>& pixel_weights = bitmap_aperture_sampler.get_pixel_weights();
            const std::vector<float>& x_selection_cdf = bitmap_aperture_sampler.get_x_selection_cdf();
            const std::vector<float>& y_selection_cdf = bitmap_aperture_sampler.get_y_selection_cdf();
            const IPoint2 resolution = IPoint2(bitmap_aperture_sampler.get_x_resolution(), bitmap_aperture_sampler.get_y_resolution());

            if(DbgVerify((pixel_weights.size() == (resolution.x * resolution.y))
                && (x_selection_cdf.size() == (resolution.x * resolution.y))
                && (y_selection_cdf.size() == resolution.y)))
            {
                // Texture for pixel weights
                rti::TextureDescriptor pixel_weights_texture_desc;
                pixel_weights_texture_desc.m_type = rti::TEXTURE_2D;
                pixel_weights_texture_desc.m_pixelDataType = rti::TEXTURE_FLOAT32;
                pixel_weights_texture_desc.m_numComponents = 3;
                pixel_weights_texture_desc.m_filter = rti::TEXTURE_NEAREST;
                pixel_weights_texture_desc.m_repeatU = rti::TEXTURE_CLAMP;
                pixel_weights_texture_desc.m_repeatV = rti::TEXTURE_CLAMP;
                pixel_weights_texture_desc.m_repeatW = rti::TEXTURE_CLAMP;
                pixel_weights_texture_desc.m_width = resolution.x;
                pixel_weights_texture_desc.m_height = resolution.y;
                pixel_weights_texture_desc.m_depth = 0;
                pixel_weights_texture_desc.m_pixelData = reinterpret_cast<const unsigned char*>(pixel_weights.data());
                pixel_weights_texture_desc.m_borderColor = rti::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
                pixel_weights_texture_desc.m_invertRows = false;
                const rti::TextureHandle pixel_weights_texture_handle = initialize_output_texture(TextureIndex_PixelWeightsTexture, pixel_weights_texture_desc);

                // Texture for X selection CDF
                rti::TextureDescriptor x_selection_texture_desc;
                x_selection_texture_desc.m_type = rti::TEXTURE_2D;
                x_selection_texture_desc.m_pixelDataType = rti::TEXTURE_FLOAT32;
                x_selection_texture_desc.m_numComponents = 1;
                x_selection_texture_desc.m_filter = rti::TEXTURE_NEAREST;
                x_selection_texture_desc.m_repeatU = rti::TEXTURE_CLAMP;
                x_selection_texture_desc.m_repeatV = rti::TEXTURE_CLAMP;
                x_selection_texture_desc.m_repeatW = rti::TEXTURE_CLAMP;
                x_selection_texture_desc.m_width = resolution.x;
                x_selection_texture_desc.m_height = resolution.y;
                x_selection_texture_desc.m_depth = 0;
                x_selection_texture_desc.m_pixelData = reinterpret_cast<const unsigned char*>(x_selection_cdf.data());
                x_selection_texture_desc.m_borderColor = rti::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
                x_selection_texture_desc.m_invertRows = false;
                const rti::TextureHandle x_selection_texture_handle = initialize_output_texture(TextureIndex_XSelectionCDFTexture, x_selection_texture_desc);

                // Texture for Y selection CDF. We use a texture2d even though we have a single row: this enables consistency with the x selection table,
                // allowing code sharing in the shader.
                rti::TextureDescriptor y_selection_texture_desc;
                y_selection_texture_desc.m_type = rti::TEXTURE_2D;
                y_selection_texture_desc.m_pixelDataType = rti::TEXTURE_FLOAT32;
                y_selection_texture_desc.m_numComponents = 1;
                y_selection_texture_desc.m_filter = rti::TEXTURE_NEAREST;
                y_selection_texture_desc.m_repeatU = rti::TEXTURE_CLAMP;
                y_selection_texture_desc.m_repeatV = rti::TEXTURE_CLAMP;
                y_selection_texture_desc.m_repeatW = rti::TEXTURE_CLAMP;
                y_selection_texture_desc.m_width = resolution.y;
                y_selection_texture_desc.m_height = 1;
                y_selection_texture_desc.m_depth = 0;
                y_selection_texture_desc.m_pixelData = reinterpret_cast<const unsigned char*>(y_selection_cdf.data());
                y_selection_texture_desc.m_borderColor = rti::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
                y_selection_texture_desc.m_invertRows = false;
                const rti::TextureHandle y_selection_texture_handle = initialize_output_texture(TextureIndex_YSelectionCDFTexture, y_selection_texture_desc);                    

                if(pixel_weights_texture_handle.isValid() && x_selection_texture_handle.isValid() && y_selection_texture_handle.isValid())
                {
                    return TranslationResult::Success;
                }
            }
        }

        // Failed to translate the texture
        return TranslationResult::Failure;
    }

    // No texture to translate
    return TranslationResult::Failure;
}

rti::TextureHandle Translator_PhysicalCameraAperture_to_RTITexture::get_pixel_weights_texture_handle() const
{
    return get_output_texture(TextureIndex_PixelWeightsTexture);
}

rti::TextureHandle Translator_PhysicalCameraAperture_to_RTITexture::get_x_selection_cdf_texture_handle() const
{
    return get_output_texture(TextureIndex_XSelectionCDFTexture);
}

rti::TextureHandle Translator_PhysicalCameraAperture_to_RTITexture::get_y_selection_cdf_texture_handle() const
{
    return get_output_texture(TextureIndex_YSelectionCDFTexture);
}

IPoint2 Translator_PhysicalCameraAperture_to_RTITexture::get_resolution() const
{
    return IPoint2(get_output_texture_resolution(TextureIndex_PixelWeightsTexture));
}

MSTR Translator_PhysicalCameraAperture_to_RTITexture::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_CAMERA);
}

//==================================================================================================
// class Translator_PhysicalCameraAperture_to_RTITexture::KeyStruct
//==================================================================================================

Translator_PhysicalCameraAperture_to_RTITexture::KeyStruct::KeyStruct(Texmap* p_aperture_texmap, const IPoint2& p_camera_resolution, float p_center_bias, bool p_affects_exposure)
    : aperture_texmap(p_aperture_texmap),
    camera_resolution(p_camera_resolution),
    center_bias(p_center_bias),
    affects_exposure(p_affects_exposure)
{

}

bool Translator_PhysicalCameraAperture_to_RTITexture::KeyStruct::operator==(const KeyStruct& rhs) const
{
    return (aperture_texmap == rhs.aperture_texmap)
        && (camera_resolution == rhs.camera_resolution)
        && (center_bias == rhs.center_bias)
        && (affects_exposure == rhs.affects_exposure);
}

//==================================================================================================
// class Translator_PhysicalCameraAperture_to_RTITexture::KeyStructHash
//==================================================================================================
size_t Translator_PhysicalCameraAperture_to_RTITexture::KeyStructHash::operator()(const KeyStruct& data) const
{
    return std::hash<Texmap*>()(data.aperture_texmap)
        ^ (std::hash<int>()(data.camera_resolution.x) << 1)
        ^ (std::hash<int>()(data.camera_resolution.x) << 2)
        ^ (std::hash<float>()(data.center_bias) << 3)
        ^ (std::hash<bool>()(data.affects_exposure) << 4);
}

}}	// namespace 
