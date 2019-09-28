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

#include "BaseTranslator_to_RTITexture.h"

#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Texmap.h>
#include <NotificationAPI/NotificationAPI_Subscription.h>

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: PHysical Camera Aperture Bitmap ==> rti::Texture
class Translator_PhysicalCameraAperture_to_RTITexture : 
    public BaseTranslator_Texmap,           // Translate from Texmap
    public BaseTranslator_to_RTITexture    // Translate to rti::Texture
{
public:

    // The key stores all the information used to create the aperture bitmap sampling acceleration structure. This way, we automatically
    // re-translate only when required information changes
    struct KeyStruct
    {
        KeyStruct(Texmap* p_aperture_texmap, const IPoint2& p_camera_resolution, float p_center_bias, bool p_affects_exposure);
        bool operator==(const KeyStruct& rhs) const;

        Texmap* aperture_texmap;
        IPoint2 camera_resolution;
        float center_bias;
        bool affects_exposure;
    };
    struct KeyStructHash
    {
        size_t operator()(const KeyStruct& data) const;
    };

    typedef GenericTranslatorKey_Struct<Translator_PhysicalCameraAperture_to_RTITexture, KeyStruct, KeyStructHash> Key;

    Translator_PhysicalCameraAperture_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node);
    ~Translator_PhysicalCameraAperture_to_RTITexture();

    // -- inherited from Translator
    virtual TranslationResult Translate(const TimeValue translationTime, Interval& newValidity, ITranslationProgress& translationProgress, KeyframeList& keyframesNeeded) override;
    virtual MSTR GetTimingCategory() const override;

    // Access to the outputs of this translator
    rti::TextureHandle get_pixel_weights_texture_handle() const;
    rti::TextureHandle get_x_selection_cdf_texture_handle() const;
    rti::TextureHandle get_y_selection_cdf_texture_handle() const;
    IPoint2 get_resolution() const;

private:

    // Indices for translator outputs
    enum TextureIndex
    {
        TextureIndex_PixelWeightsTexture,
        TextureIndex_XSelectionCDFTexture,
        TextureIndex_YSelectionCDFTexture
    };

    const KeyStruct m_key;
};

}}	// namespace 
