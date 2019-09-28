//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_Mtl_to_RTIShader.h"

// local
#include "../resource.h"
#include "Translator_AMGMtl_to_RTIShader.h"
#include "Translator_LegacyMatteMtl_to_RTIShader.h"
#include "Translator_UnsupportedMtl_to_RTIShader.h"
// max sdk
#include <dllutilities.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

using namespace MaxSDK;

namespace Max
{;
namespace RapidRTTranslator
{;

BaseTranslator_Mtl_to_RTIShader::BaseTranslator_Mtl_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_AMG_to_RTIShader(translator_graph_node),
    BaseTranslator_Mtl(key, translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_Mtl_to_RTIShader::~BaseTranslator_Mtl_to_RTIShader()
{

}

MSTR BaseTranslator_Mtl_to_RTIShader::get_shader_name() const
{
    Mtl* const mtl = GetMaterial();
    if(mtl != nullptr)
    {
        return mtl->GetName();
    }
    else
    {
        return _T("");
    }
}

//==================================================================================================
// struct BaseTranslator_Mtl_to_RTIShader::Allocator
//==================================================================================================

std::unique_ptr<Translator> BaseTranslator_Mtl_to_RTIShader::Allocator::operator()(const Key& key, TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue /*initial_time*/) const
{
    // Find the appropriate translator for this material
    if(Translator_AMGMtl_to_RTIShader::can_translate(key))
    {
        return std::unique_ptr<Translator>(new Translator_AMGMtl_to_RTIShader(key, translator_graph_node));
    }
    else if(Translator_LegacyMatteMtl_to_RTIShader::can_translate(key))  // ZAP TODO: Remove later
    {
        return std::unique_ptr<Translator>(new Translator_LegacyMatteMtl_to_RTIShader(key, translator_graph_node));
    }
    else
    {
        // Unsupported material: output an appropriate error message
        Mtl* const material = key;
        if(material != nullptr)
        {
            TSTR error_message;
            error_message.printf(MaxSDK::GetResourceStringAsMSTR(IDS_UNSUPPORTED_MATERIAL), material->GetName().data(), material->ClassName().data());
            render_session_context.GetLogger().LogMessage(IRenderingLogger::MessageType::Error, error_message);
        }

        return std::unique_ptr<Translator>(new Translator_UnsupportedMtl_to_RTIShader(key, translator_graph_node));
    }
}

}}	// namespace 
