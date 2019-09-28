//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_TextureOutput_to_RTITexture.h"

// Max SDK
#include <imtl.h>
#include <iparamb2.h>
#include <icurvctl.h>
#include <stdmat.h>
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

Translator_TextureOutput_to_RTITexture::Translator_TextureOutput_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_MtlBase(key, true, translator_graph_node),
    BaseTranslator_to_RTITexture(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_TextureOutput_to_RTITexture::~Translator_TextureOutput_to_RTITexture()
{

}

StdTexoutGen *Translator_TextureOutput_to_RTITexture::CreateFromOutputParameter(ReferenceTarget& ref_targ, const MSTR param_name, const TimeValue t, Interval& validity)
{
    // Search for the appropriately named parameter in all the param blocks
    const int num_param_blocks = ref_targ.NumParamBlocks();
    for(int param_block_index = 0; (param_block_index < num_param_blocks); ++param_block_index)
    {
        IParamBlock2* const param_block = ref_targ.GetParamBlock(param_block_index);
        if(param_block != nullptr)
        {
            for(const ParamDef& param_def : *param_block)
            {
				if (param_name ==  param_def.int_name)
				{
					if(param_def.type == TYPE_REFTARG)
					{
						// Is this of the right type?
						return dynamic_cast<StdTexoutGen *>(param_block->GetReferenceTarget(param_def.ID, t, validity)); 
					}
					else
					{
						return nullptr; // Parameter found, but not of our type. Tgis is normal, healthy "not what we were looking for" return
					}
				}
            }
        }
    }

    // Not found! A bug? The parameter should always be found!
	DbgAssert(false);
	return nullptr;
}


TranslationResult Translator_TextureOutput_to_RTITexture::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    MtlBase* const mtl = GetMtlBase();
	if(mtl != nullptr)
	{
        // Bake the texture
        IPoint2 resolution(256, 1);
        std::vector<BMM_Color_fl> data;

		StdTexoutGen *texOut = dynamic_cast<StdTexoutGen *>(mtl); 

		if (texOut)
		{
			ICurveCtl   *pCCRGB  = (ICurveCtl*) texOut->GetReference(1); // RGB Control
			ICurveCtl   *pCCMono = (ICurveCtl*) texOut->GetReference(2); // Mono Control

			if ((pCCRGB  != nullptr || pCCMono != nullptr) && texOut->GetFlag(TEXOUT_COLOR_MAP))
			{
				data.resize(resolution.x * resolution.y);
				texOut->Update(t, validity);

				for (int i=0; i < resolution.x; i++)
				{
					AColor result;

					if (texOut->GetFlag(TEXOUT_COLOR_MAP_RGB))
					{
						if (pCCRGB != nullptr)
						{
							data[i].r = pCCRGB->GetControlCurve(0)->GetValue(t,i/255.0,validity,true);
							data[i].g = pCCRGB->GetControlCurve(1)->GetValue(t,i/255.0,validity,true);
							data[i].b = pCCRGB->GetControlCurve(2)->GetValue(t,i/255.0,validity,true);
						}
						else
						{
							data[i].r = i/255.0;
							data[i].g = i/255.0;
							data[i].b = i/255.0;
						}
					}
					else
					{
						if (pCCMono != nullptr)
						{
							data[i].r = pCCMono->GetControlCurve(0)->GetValue(t,i/255.0,validity,true);
							data[i].g = pCCMono->GetControlCurve(0)->GetValue(t,i/255.0,validity,true);
							data[i].b = pCCMono->GetControlCurve(0)->GetValue(t,i/255.0,validity,true);
						}
						else
						{
							data[i].r = i/255.0;
							data[i].g = i/255.0;
							data[i].b = i/255.0;
						}
					}
				}
			}
			else
			{
				resolution.x  = 1;
				resolution.y  = 1;
				data.resize(resolution.x * resolution.y);
			}

			const int width  = resolution.x;
			const int height = resolution.y;

			rti::TextureDescriptor textureDesc;
			textureDesc.m_type = rti::TEXTURE_2D;
			textureDesc.m_pixelDataType = rti::TEXTURE_FLOAT32;
			textureDesc.m_numComponents = 4;
			textureDesc.m_filter  = rti::TEXTURE_LINEAR;
			textureDesc.m_repeatU = rti::TEXTURE_CLAMP;
			textureDesc.m_repeatV = rti::TEXTURE_CLAMP;
			textureDesc.m_repeatW = rti::TEXTURE_CLAMP;
			textureDesc.m_width   = width;
			textureDesc.m_height  = height;
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
	}

	return TranslationResult::Failure;
}

rti::TextureHandle Translator_TextureOutput_to_RTITexture::get_texture_handle() const
{
    return get_output_texture(0);
}

IPoint2 Translator_TextureOutput_to_RTITexture::get_texture_resolution() const
{
    return IPoint2(get_output_texture_resolution(0));
}

}}	// namespace 
