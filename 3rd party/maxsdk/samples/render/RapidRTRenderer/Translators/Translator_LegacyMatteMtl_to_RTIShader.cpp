//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_LegacyMatteMtl_to_RTIShader.h"

#include <inode.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_LegacyMatteMtl_to_RTIShader::Translator_LegacyMatteMtl_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Mtl_to_RTIShader(key, translator_graph_node),
    Translator(translator_graph_node),
    m_material(key)
{
    DbgAssert(can_translate(key));
}

Translator_LegacyMatteMtl_to_RTIShader::~Translator_LegacyMatteMtl_to_RTIShader()
{

}

bool Translator_LegacyMatteMtl_to_RTIShader::can_translate(const Key& key)
{
    Mtl* material = key;
    return (material != nullptr) && (material->ClassID() == Class_ID(MATTE_CLASS_ID, 0));
}

TranslationResult Translator_LegacyMatteMtl_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
	if(m_material != NULL)
	{
		// Update the material and get its validity
		m_material->Update(t, validity);

		// Simple default grey material for now
        const rti::ShaderHandle shader_handle = initialize_output_shader("ArchDesign");
		if(shader_handle.isValid())
        {
			rti::TEditPtr<rti::Shader> shader(shader_handle);
			set_shader_float3(*shader, "diffuse", Color(0.9f, 0.9f, 0.9f));
            return TranslationResult::Success;
		}
	}

	return TranslationResult::Failure;
}

}}	// namespace 
