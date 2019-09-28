//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_to_RTITexture.h"

#include "../Util.h"

namespace Max
{;
namespace RapidRTTranslator
{;

BaseTranslator_to_RTITexture::BaseTranslator_to_RTITexture(TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_to_RTITexture::~BaseTranslator_to_RTITexture()
{

}

rti::TextureHandle BaseTranslator_to_RTITexture::get_output_texture(const size_t texture_index) const
{
    const size_t texture_output_index = texture_index * 2;
    return get_output_handle<rti::TextureHandle>(texture_output_index);
}

IPoint3 BaseTranslator_to_RTITexture::get_output_texture_resolution(const size_t texture_index) const
{
    const size_t resolution_output_index = texture_index * 2 + 1;
    return GetOutput_SimpleValue(resolution_output_index, IPoint3(0, 0, 0));
}

rti::TextureHandle BaseTranslator_to_RTITexture::initialize_output_texture(const size_t texture_index, const rti::TextureDescriptor& texture_desc)
{
    const size_t texture_output_index = texture_index * 2;
    const size_t resolution_output_index = texture_index * 2 + 1;

    // Don't bother creating an empty texture (that would trigger bogus asserts)
    if((texture_desc.m_width > 0) && (texture_desc.m_height > 0))
    {
        const rti::TextureHandle texture_handle = initialize_output_handle<rti::TextureHandle>(texture_output_index);
        SetOutput_SimpleValue(resolution_output_index, IPoint3(texture_desc.m_width, texture_desc.m_height, texture_desc.m_depth));
        if(texture_handle.isValid())
        {
            rti::TEditPtr<rti::Texture> pTexture(texture_handle);
            if(RRTUtil::check_rti_result(pTexture->create(texture_desc)))
            {
                return texture_handle;
            }
        }
    }

    // Failed: clear the output
    ResetOutput(texture_output_index);
    ResetOutput(resolution_output_index);
    return rti::TextureHandle();
}

}}	// namespace 
