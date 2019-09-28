//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_LightscaleLightWeb_to_RTITexture.h"

// Local includes
#include "TypeUtil.h"
#include "../Util.h"
#include "../resource.h"
// Max SDK
#include <object.h>
#include <lslights.h>
#include <dllutilities.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_LightscaleLightWeb_to_RTITexture::Translator_LightscaleLightWeb_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Object(key, translator_graph_node),
    BaseTranslator_to_RTITexture(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_LightscaleLightWeb_to_RTITexture::~Translator_LightscaleLightWeb_to_RTITexture()
{

}

TranslationResult Translator_LightscaleLightWeb_to_RTITexture::Translate(const TimeValue t, Interval& /*validity*/, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    INode* const node = GetNode();
    if(node != nullptr)
    {
        const ObjectState& object_state = node->EvalWorldState(t);
        LightscapeLight2* ls_light = TypeUtil::to_LightscapeLight(object_state.obj);
        if(ls_light != nullptr)
        {
            // Bake the web data
            MaxSDK::Array<float> baked_distribution;
            size_t baked_width = 0;
            size_t baked_height = 0;
            if(ls_light->BakeWebDistribution(baked_distribution, baked_width, baked_height))
            {
                DbgAssert(baked_distribution.lengthUsed() == (baked_width * baked_height));

                // Create a Rapid Texture
                rti::TextureDescriptor desc;
                desc.m_type = rti::TEXTURE_2D;
                desc.m_pixelDataType = rti::TEXTURE_FLOAT32;
                desc.m_numComponents = 1;
                desc.m_filter = rti::TEXTURE_LINEAR;
                desc.m_repeatU = rti::TEXTURE_REPEAT;      // Repeat in U to enable smooth filtering across the seam
                desc.m_repeatV = rti::TEXTURE_CLAMP;       // Clamp in V to disable smoothing across poles
                desc.m_repeatW = rti::TEXTURE_CLAMP;
                desc.m_width = static_cast<int>(baked_width);
                desc.m_height = static_cast<int>(baked_height);
                desc.m_depth = 0;
                desc.m_pixelData = !baked_distribution.isEmpty() ? reinterpret_cast<const unsigned char*>(baked_distribution.asArrayPtr()) : nullptr;
                desc.m_borderColor = rti::Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
                desc.m_invertRows = false;
                const rti::TextureHandle texture_handle = initialize_output_texture(0, desc);
                if(texture_handle.isValid())
                {
                    return TranslationResult::Success;
                }
            }
        }

        return TranslationResult::Failure;
    }
    else
    {
        // Nothing to translate
        SetNumOutputs(0);
        return TranslationResult::Success;
    }
}

rti::TextureHandle Translator_LightscaleLightWeb_to_RTITexture::get_web_texture_handle() const
{
    return get_output_texture(0);
}

MSTR Translator_LightscaleLightWeb_to_RTITexture::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_LIGHTS);
}

bool Translator_LightscaleLightWeb_to_RTITexture::CareAboutMaterialDisplacement() const
{
    return false;
}

}}	// namespace 
