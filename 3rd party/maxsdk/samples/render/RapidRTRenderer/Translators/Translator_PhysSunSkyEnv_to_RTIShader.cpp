//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_PhysSunSkyEnv_to_RTIShader.h"

// Max SDK
#include <DaylightSimulation/IPhysicalSunSky.h>
#include <RenderingAPI/Renderer/IEnvironmentContainer.h>

using namespace MaxSDK;

namespace
{
    double square(const double val)
    {
        return val * val;
    }

    double cube(const double val)
    {
        return val * val * val;
    }
}

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_PhysSunSkyEnv_to_RTIShader::Translator_PhysSunSkyEnv_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Environment_to_RTIShader(key, translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_PhysSunSkyEnv_to_RTIShader::~Translator_PhysSunSkyEnv_to_RTIShader()
{

}

bool Translator_PhysSunSkyEnv_to_RTIShader::can_translate(IEnvironmentContainer* const environment)
{
    // We support both the mr physical sun & sky and the new (non-mr) physical sun & sky
    IEnvironmentContainer* const textured_env = environment;
    const IEnvironmentContainer::EnvironmentMode env_mode = (textured_env != nullptr) ? textured_env->GetEnvironmentMode() : IEnvironmentContainer::EnvironmentMode::None;
    Texmap* const environment_texture = (env_mode == IEnvironmentContainer::EnvironmentMode::Texture) ? textured_env->GetEnvironmentTexture() : nullptr;

    // Check if the environment map has the physical sun & sky interface
    return (dynamic_cast<IPhysicalSunSky*>(environment_texture) != nullptr);
}

TranslationResult Translator_PhysSunSkyEnv_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    IEnvironmentContainer* const textured_env = GetRenderSessionContext().GetEnvironment();
    const IEnvironmentContainer::EnvironmentMode env_mode = (textured_env != nullptr) ? textured_env->GetEnvironmentMode() : IEnvironmentContainer::EnvironmentMode::None;
    Texmap* const environment_map = (env_mode == IEnvironmentContainer::EnvironmentMode::Texture) ? textured_env->GetEnvironmentTexture() : nullptr;

    IPhysicalSunSky* const physical_sun_sky = dynamic_cast<IPhysicalSunSky*>(environment_map);
    if(physical_sun_sky != nullptr)
    {
        // Instantiate the shader
        const rti::ShaderHandle shader_handle = initialize_output_shader("PhysSunSky");
        if(shader_handle.isValid())
        {
            // Fetch the shader parameters
            const IPhysicalSunSky::ShadingParameters params = physical_sun_sky->EvaluateShadingParameters(t, validity);

            // Setup the shader parameters
            rti::TEditPtr<rti::Shader> shader(shader_handle);

            set_shader_float(*shader, "m_horizon_height", params.horizon_height);
            set_shader_float(*shader, "m_horizon_blur", params.horizon_blur);
            set_shader_float3(*shader, "m_global_multiplier", params.global_multiplier);
            set_shader_float3(*shader, "m_sun_illuminance", params.sun_illuminance);
            set_shader_float3(*shader, "m_sun_luminance", params.sun_luminance);
            set_shader_float(*shader, "m_sun_glow_multiplier", params.sun_glow_intensity);
            set_shader_float(*shader, "m_sky_contribution_multiplier", params.sky_contribution_multiplier);
            set_shader_float3(*shader, "m_sky_ground_contribution", params.sky_ground_contribution);
            set_shader_float(*shader, "m_sun_disc_angular_radius", params.sun_disc_angular_radius);
            set_shader_float(*shader, "m_sun_smooth_angular_radius", params.sun_smooth_angular_radius);
            set_shader_float(*shader, "m_sun_glow_angular_radius", params.sun_glow_angular_radius);
            set_shader_float(*shader, "m_saturation", params.color_saturation);
            set_shader_float3(*shader, "m_color_tint", params.color_tint);
            set_shader_float3(*shader, "m_ground_color", params.ground_color);
            set_shader_float3(*shader, "m_night_luminance", params.night_luminance);
            set_shader_float(*shader, "m_night_falloff", params.night_falloff);
            set_shader_float3(*shader, "m_sun_direction", params.sun_direction);
            set_shader_float3(*shader, "m_sun_direction_for_sky_contribution", params.sun_direction_for_sky_contribution);
            set_shader_float3(*shader, "m_perez_A", params.perez_A);
            set_shader_float3(*shader, "m_perez_B", params.perez_B);
            set_shader_float3(*shader, "m_perez_C", params.perez_C);
            set_shader_float3(*shader, "m_perez_D", params.perez_D);
            set_shader_float3(*shader, "m_perez_E", params.perez_E);
            set_shader_float3(*shader, "m_perez_Z", params.perez_Z);

            return TranslationResult::Success;
        }
        else
        {
            return TranslationResult::Failure;
        }
    }
    else
    {
        return TranslationResult::Failure;
    }
}

}}	// namespace 
