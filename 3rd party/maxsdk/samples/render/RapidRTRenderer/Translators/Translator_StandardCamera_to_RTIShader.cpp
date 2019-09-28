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

#include "Translator_StandardCamera_to_RTIShader.h"

#include "../resource.h"

#include <dllutilities.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_StandardCamera_to_RTIShader::Translator_StandardCamera_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Camera_to_RTIShader(key,translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_StandardCamera_to_RTIShader::~Translator_StandardCamera_to_RTIShader()
{

}

TranslationResult Translator_StandardCamera_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translationProgress*/, KeyframeList& /*keyframesNeeded*/)
{
    // Evaluate the parameters
    const rti::ShaderHandle shader_handle = initialize_output_shader("StandardCamera");
    if(shader_handle.isValid())
    {
        EvaluateCameraParameters(m_last_translated_camera_parameters, t, GetRenderSessionContext());
        validity &= m_last_translated_camera_parameters.validity;

        rti::TEditPtr<rti::Shader> shader(shader_handle);
        set_shader_float(*shader, "m_near_clip_distance", m_last_translated_camera_parameters.clip_near);

        return TranslationResult::Success;
    }
    else
    {
        // Failed to instantiate shader
        return TranslationResult::Failure;
    }
}

MSTR Translator_StandardCamera_to_RTIShader::get_shader_name() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_STANDARD_CAMERA_SHADER_NAME);
}

Interval Translator_StandardCamera_to_RTIShader::CheckValidity(const TimeValue t, const Interval& previous_validity) const 
{
    // Check if any of the camera parameters, which we use and care about, has changed
    // For example, we don't care if the exposure parameters of the physical camera change - and we don't want to react to those changes.
    // We evaluate at the same time as was last evaluated, to make the comparison meaningful.
    CameraParameters camera_parameters;
    EvaluateCameraParameters(camera_parameters, t, GetRenderSessionContext());

    // We compare for equality of the struct, including the validity interval it contains, as a change there may be meaningful
    if(camera_parameters != m_last_translated_camera_parameters)
    {
        return NEVER;
    }
    else
    {
        return __super::CheckValidity(t, previous_validity);
    }
}

void Translator_StandardCamera_to_RTIShader::NotifyCameraChanged() 
{
    // The camera parameters may (or may not) have changed in a way which is meaningful to us. Delay the check as it's not safe to evaluate the camera
    // node within the notification callback.
    Invalidate(true);   
}

void Translator_StandardCamera_to_RTIShader::EvaluateCameraParameters(CameraParameters& parameters, const TimeValue t, const IRenderSessionContext& context)
{
    const ICameraContainer& camera_container = context.GetCamera();

    // Reset the parameters
    parameters = CameraParameters();
    Interval& validity = parameters.validity;
    validity = FOREVER;

    parameters.clip_near = camera_container.GetClipNear(t, validity);
}

//==================================================================================================
// struct Translator_StandardCamera_to_RTIShader::CameraParameters
//==================================================================================================

Translator_StandardCamera_to_RTIShader::CameraParameters::CameraParameters()
    : validity(NEVER),
    clip_near(0.0f)
{

}

bool Translator_StandardCamera_to_RTIShader::CameraParameters::operator==(const CameraParameters& other) const
{
    return validity == other.validity
        && clip_near == other.clip_near;
}

bool Translator_StandardCamera_to_RTIShader::CameraParameters::operator!=(const CameraParameters& other) const
{
    return !(*this == other);
}


}}	// namespace 
