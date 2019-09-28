//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_BitmapFile_to_RTITexture.h"

// local
#include "../resource.h"
// Max sdk
#include <ref.h>
#include <Util/fnv1a.hpp>
#include <pbbitmap.h>
#include <iparamb2.h>
#include <imtl.h>
#include <stdmat.h>
#include <dllutilities.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Texmap.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

namespace
{
    struct BitmapDeleter
    {
        void operator()(Bitmap* const bitmap)
        {
            if(bitmap != nullptr)
            {
                bitmap->DeleteThis();
            }
        }
    };
}

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

Translator_BitmapFile_to_RTITexture::Translator_BitmapFile_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTITexture(translator_graph_node),
    Translator(translator_graph_node),
    m_key(key)
{

}

Translator_BitmapFile_to_RTITexture::~Translator_BitmapFile_to_RTITexture()
{

}

MSTR Translator_BitmapFile_to_RTITexture::GetTimingCategory() const
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMER_BITMAP);
}

TranslationResult Translator_BitmapFile_to_RTITexture::Translate(const TimeValue /*t*/, Interval& /*validity*/, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    // Don't try to load empty filenames
    if(m_key.file_name.length() > 0)
    {
        // Setup a BitmapInfo
        BitmapInfo bi;
        bi.SetName(m_key.file_name);
        bi.SetCurrentFrame(m_key.frame_number);
        if(m_key.custom_file_gamma_flag)
        {
            bi.SetCustomFlag(BMM_CUSTOM_FILEGAMMA);
        }
        else
        {
            bi.ResetCustomFlag(BMM_CUSTOM_FILEGAMMA);
        }
        if(m_key.custom_gamma_flag)
        {
            bi.SetCustomFlag(BMM_CUSTOM_GAMMA);
            bi.SetCustomGamma(m_key.custom_gamma);
        }
        else
        {
            bi.ResetCustomFlag(BMM_CUSTOM_GAMMA);
        }

        // Initialize PiData
        const size_t pi_data_size = m_key.pi_data.size();
        if(pi_data_size > 0)
        {
            if(DbgVerify(bi.AllocPiData(DWORD(pi_data_size))))
            {
                void* const pi_data = bi.GetPiData();
                if(DbgVerify(pi_data != nullptr))
                {
                    memcpy(pi_data, m_key.pi_data.data(), pi_data_size);
                }
            }
        }

        // Try loading the bitmap
        const BOOL old_silent_mode = TheManager->SetSilentMode(TRUE);
        BMMRES status = BMMRES_SUCCESS;
        const std::unique_ptr<Bitmap, BitmapDeleter> bitmap = std::unique_ptr<Bitmap, BitmapDeleter>(TheManager->Load(&bi, &status));
        TheManager->SetSilentMode(old_silent_mode);

        if(bitmap != nullptr)
        {
            // Extract the pixel data
            IPoint2 resolution(0, 0);
            std::vector<BMM_Color_fl> data;
            if(DbgVerify(BaseTranslator_Texmap::ExtractBitmap(*bitmap, resolution, data)))
            {
                // Create the RTI texture
                rti::TextureDescriptor textureDesc;
                textureDesc.m_type = rti::TEXTURE_2D;
                textureDesc.m_pixelDataType = rti::TEXTURE_FLOAT32;
                textureDesc.m_numComponents = 4;
                textureDesc.m_filter  = rti::TEXTURE_LINEAR;
                textureDesc.m_repeatU = rti::TEXTURE_REPEAT;
                textureDesc.m_repeatV = rti::TEXTURE_REPEAT;
                textureDesc.m_repeatW = rti::TEXTURE_REPEAT;
                textureDesc.m_width = resolution.x;
                textureDesc.m_height = resolution.y;
                textureDesc.m_depth = 0;
                textureDesc.m_pixelData = (unsigned char*)(&data[0]);
                textureDesc.m_borderColor = rti::Vec4f(1.0f, 1.0f, 0.0f, 1.0f);
                textureDesc.m_invertRows = true;
                const rti::TextureHandle texture_handle = initialize_output_texture(0, textureDesc);
                if(DbgVerify(texture_handle.isValid()))
                {
                    return TranslationResult::Success;
                }
                else
                {
                    // Error: texture creation failed
                }
            }
            else
            {
                // Error: bitmap extraction failed
            }
           
            // Output error
            MSTR msg;
            msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_TRANSLATING_BITMAP_FAILED), bi.Name());
            GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, msg);
        }
        else
        {
            // On failure, report the error.
            if(status == BMMRES_FILENOTFOUND)
            {
                MSTR msg;
                msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_LOADING_BITMAP_MISSING), bi.Name());
                // Missing file is a warning because it was already reported in the missing bitmaps dialog.
                GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Warning, msg);
            }
            else
            {
                MSTR msg;
                msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_LOADING_BITMAP_FAILED), bi.Name());
                GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, msg);
            }
        }

        return TranslationResult::Failure;
    }
    else
    {
        // Nothing to translate
        ResetAllOutputs();
        return TranslationResult::Success;
    }
}

rti::TextureHandle Translator_BitmapFile_to_RTITexture::get_texture_handle() const
{
    return get_output_texture(0);
}

IPoint2 Translator_BitmapFile_to_RTITexture::get_texture_resolution() const
{
    return IPoint2(get_output_texture_resolution(0));
}

void Translator_BitmapFile_to_RTITexture::PreTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // do nothing
}

void Translator_BitmapFile_to_RTITexture::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // do nothing
}

Interval Translator_BitmapFile_to_RTITexture::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const 
{
    return previous_validity;
}

//==================================================================================================
// class Translator_BitmapFile_to_RTITexture::KeyStruct
//==================================================================================================

Translator_BitmapFile_to_RTITexture::KeyStruct::KeyStruct()
    : file_name(),
    frame_number(0),
    custom_file_gamma_flag(false),
    custom_gamma_flag(false),
    custom_gamma(0.0f)
{

}

Translator_BitmapFile_to_RTITexture::KeyStruct::KeyStruct(const BitmapInfo& bi)
    : file_name(bi.Name()),
    frame_number(const_cast<BitmapInfo&>(bi).CurrentFrame()),
    custom_file_gamma_flag(!!const_cast<BitmapInfo&>(bi).TestCustomFlags(BMM_CUSTOM_FILEGAMMA)),
    custom_gamma_flag(!!const_cast<BitmapInfo&>(bi).TestCustomFlags(BMM_CUSTOM_GAMMA)),
    custom_gamma(const_cast<BitmapInfo&>(bi).TestCustomFlags(BMM_CUSTOM_GAMMA) ? const_cast<BitmapInfo&>(bi).GetCustomGamma() : 0.0f)
{
    // Copy the PiData
    const DWORD pi_data_size = const_cast<BitmapInfo&>(bi).GetPiDataSize();
    const void* const bi_pi_data = const_cast<BitmapInfo&>(bi).GetPiData();
    if((pi_data_size > 0) && DbgVerify(bi_pi_data != nullptr))
    {
        pi_data.resize(pi_data_size);
        memcpy(pi_data.data(), bi_pi_data, pi_data_size);
    }
}

Translator_BitmapFile_to_RTITexture::KeyStruct 
    Translator_BitmapFile_to_RTITexture::KeyStruct::CreateFromBitmapParameter(ReferenceTarget& ref_targ, const MSTR param_name, const TimeValue t, Interval& validity)
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
                if((param_def.type == TYPE_BITMAP) && (param_name ==  param_def.int_name))
                {
                    PBBitmap* const pb_bitmap = param_block->GetBitmap(param_def.ID, t, validity);
                    return (pb_bitmap != nullptr) ? KeyStruct(pb_bitmap->bi) : KeyStruct();
                }
            }
        }
    }

    // Parameter not found - bug?
    DbgAssert(false);
    return KeyStruct();
}

bool Translator_BitmapFile_to_RTITexture::KeyStruct::operator==(const KeyStruct& rhs) const
{
    return (file_name == rhs.file_name)
        && (frame_number == rhs.frame_number)
        && (custom_file_gamma_flag == rhs.custom_file_gamma_flag)
        && (custom_gamma_flag == rhs.custom_gamma_flag)
        && (custom_gamma == rhs.custom_gamma)
        && (pi_data == rhs.pi_data);
}

//==================================================================================================
// class Translator_BitmapFile_to_RTITexture::KeyStructHash
//==================================================================================================

size_t buffer_hash(const void* const ptr, const size_t num_bytes)
{
    const size_t hash = fnv1a_hash_bytes((const unsigned char*)ptr, num_bytes);
    return hash;
}

size_t hash(const MSTR& string)
{
    return buffer_hash(string.data(), string.length() * sizeof(*(string.data())));
}

template<typename T>
size_t hash(const std::vector<T>& v)
{
    return buffer_hash(v.data(), v.size() * sizeof(*(v.data())));
}

size_t Translator_BitmapFile_to_RTITexture::KeyStructHash::operator()(const KeyStruct& data) const
{
    const size_t hash_value = 
        hash(data.file_name)
        ^ (std::hash<int>()(data.frame_number) << 1)
        ^ (std::hash<bool>()(data.custom_file_gamma_flag) << 2)
        ^ (std::hash<bool>()(data.custom_gamma_flag) << 3)
        ^ (std::hash<float>()(data.custom_gamma) << 4)
        ^ (hash(data.pi_data) << 5);

    return hash_value;
}

}}	// namespace 
