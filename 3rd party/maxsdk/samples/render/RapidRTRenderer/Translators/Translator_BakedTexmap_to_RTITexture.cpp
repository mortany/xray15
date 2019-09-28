//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_BakedTexmap_to_RTITexture.h"

// local
#include "../resource.h"
#include "../RapidRenderSettingsWrapper.h"
// Max SDK
#include <imtl.h>
#include <dllutilities.h>
// std
#include <vector>

#undef min      // madness!!!
#undef max      // when will it stop?

namespace Max
{;
namespace RapidRTTranslator
{;

namespace
{
	inline float colorAmount(BMM_Color_fl &c)
	{
		return (c.r + c.g + c.b)*(1.0f/3.0f);
	}
}

Translator_BakedTexmap_to_RTITexture::Translator_BakedTexmap_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Texmap(key, translator_graph_node),
    BaseTranslator_to_RTITexture(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_BakedTexmap_to_RTITexture::~Translator_BakedTexmap_to_RTITexture()
{

}

MSTR Translator_BakedTexmap_to_RTITexture::GetTimingCategory() const
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMER_BAKED_TEXMAP);
}

TranslationResult Translator_BakedTexmap_to_RTITexture::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    Texmap* const texmap = GetTexmap();
	if(texmap != nullptr)
	{
        // Bake the texture
        IPoint2 resolution(0, 0);
        std::vector<BMM_Color_fl> data;
        const IPoint2 bake_resolution = RapidRenderSettingsWrapper(GetRenderSessionContext().GetRenderSettings()).get_texture_bake_resolution();
        BakeTexmap(*texmap, t, validity, bake_resolution, resolution, data);

        const int width = resolution.x;
        const int height = resolution.y;

        rti::TextureDescriptor textureDesc;
        textureDesc.m_type = rti::TEXTURE_2D;
        textureDesc.m_pixelDataType = rti::TEXTURE_FLOAT32;
        textureDesc.m_numComponents = 4;
        textureDesc.m_filter  = rti::TEXTURE_LINEAR;
        textureDesc.m_repeatU = rti::TEXTURE_REPEAT;
        textureDesc.m_repeatV = rti::TEXTURE_REPEAT;
        textureDesc.m_repeatW = rti::TEXTURE_REPEAT;
        textureDesc.m_width = width;
        textureDesc.m_height = height;
        textureDesc.m_depth = 0;
        textureDesc.m_pixelData = (unsigned char*)(&data[0]);
        textureDesc.m_borderColor = rti::Vec4f(1.0f, 1.0f, 0.0f, 1.0f);
        textureDesc.m_invertRows = true;
        const rti::TextureHandle texture_handle = initialize_output_texture(0, textureDesc);
        if(texture_handle.isValid())
        {
            return TranslationResult::Success;
		}
	}

	return TranslationResult::Failure;
}

rti::TextureHandle Translator_BakedTexmap_to_RTITexture::get_texture_handle() const
{
    return get_output_texture(0);
}

IPoint2 Translator_BakedTexmap_to_RTITexture::get_texture_resolution() const
{
    return IPoint2(get_output_texture_resolution(0));
}

}}	// namespace 
