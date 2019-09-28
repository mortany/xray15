//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_UnsupportedMtl_to_RTIShader.h"

#include "PBUtil.h"
#include "../Util.h"

#include <inode.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_UnsupportedMtl_to_RTIShader::Translator_UnsupportedMtl_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Mtl_to_RTIShader(key, translator_graph_node),
    Translator(translator_graph_node),
    m_material(key)
{

}

Translator_UnsupportedMtl_to_RTIShader::~Translator_UnsupportedMtl_to_RTIShader()
{

}

TranslationResult Translator_UnsupportedMtl_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
	if(m_material != NULL)
	{
		// Update the material and get its validity
		m_material->Update(t, validity);

        const Color diffuse_color = m_material->GetDiffuse();
        const Color specular_color = m_material->GetSpecular();
        const float shininess = m_material->GetShininess();     // 0-1, glossiness
        const float shininess_strength = m_material->GetShinStr();    // exponent
        const float reflectivity = min(shininess_strength, 1.0f);       //!! semi-magical guesstimate
        const float transparency = m_material->GetXParency();
        const float emission_intensity = m_material->GetSelfIllumColorOn() ? m_material->GetSelfIllum() : 0.0f;
        const Color emission_color = m_material->GetSelfIllumColor();

        const rti::ShaderHandle shader_handle = initialize_output_shader("ArchDesign");
		if(shader_handle.isValid())
        {
            rti::TEditPtr<rti::Shader> shader(shader_handle);

			set_shader_float3(*shader, "diffuse", diffuse_color);
			set_shader_float3(*shader, "reflectionColor", specular_color);
            set_shader_float3(*shader, "refractionColor", Color(1.0f, 1.0f, 1.0f));
            set_shader_float(*shader, "reflectiveness", reflectivity);
            set_shader_float(*shader, "maxRef", 1.0f);
            set_shader_float(*shader, "minRef", 0.01f); // Zero yields ugly artifact
            set_shader_float(*shader, "refShape", 5.0f);
            set_shader_float(*shader, "roughness", RRTUtil::glossToRough(shininess));
            set_shader_float(*shader, "transparency", transparency);
            set_shader_float(*shader, "ior", 1.0f);
            set_shader_bool(*shader, "useIORReflectiveness", false);
            set_shader_float3(*shader, "emission_color", emission_color);
            set_shader_float(*shader, "emission_intensity", emission_intensity);

            set_output_is_emitter(Point3(emission_intensity * emission_color).MaxComponent() > 0.0f);
            return TranslationResult::Success;
		}
	}

	return TranslationResult::Failure;
}

}}	// namespace 
