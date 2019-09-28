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

#include "RapidRenderSettingsWrapper.h"
#include "RapidRTRendererPlugin.h"

// max sdk
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <iparamb2.h>

// rti
#include <rti/scene/RenderOptions.h>
#include <algorithm>

#undef min      // ugly
#undef max      // duckling

namespace Max
{;
namespace RapidRTTranslator
{;

RapidRenderSettingsWrapper::RapidRenderSettingsWrapper(const IRenderSettingsContainer& render_settings_container)
    : m_render_settings_container(render_settings_container),
	 m_param_block(m_render_settings_container.GetRenderer().GetParamBlockByID(0))
{
    DbgAssert(m_param_block != nullptr);
}

bool RapidRenderSettingsWrapper::get_enable_caustics() const
{
    // [dl | 25aug2014] Disabling caustics in all cases, as the feature is apparently experimental and unreliable. See notes in https://jira.autodesk.com/browse/RRT-1050.
    return false;
}

rti::EKernel RapidRenderSettingsWrapper::get_rapid_kernel_to_use() const
{
    if(m_param_block != nullptr)
    {
        return static_cast<rti::EKernel>(m_param_block->GetInt(RapidRTRendererPlugin::ParamID_RenderMethod));
    }
    else
    {
        return rti::KERNEL_LOW_NOISE;
    }
}

unsigned int RapidRenderSettingsWrapper::get_termination_iterations() const
{
    if(m_render_settings_container.GetIsMEditRender())
    {
        return 100;
    }
    else if(m_render_settings_container.GetRenderTargetType() == IRenderSettingsContainer::RenderTargetType::ToneOperatorPreview)
    {
        // No iteration limit - rely on time and quality limits for tone op preview
        return 0;
    }
    else if(m_param_block != nullptr)
    {
        if(m_param_block->GetInt(RapidRTRendererPlugin::ParamID_Termination_EnableIterations))
        {
            // Minimum is 1
            return std::max(1, m_param_block->GetInt(RapidRTRendererPlugin::ParamID_Termination_NumIterations));
        }
    }

    return 0;   // infinite
}

unsigned int RapidRenderSettingsWrapper::get_termination_seconds() const
{
    if(m_render_settings_container.GetIsMEditRender())
    {
        return 1;       // Time limit on MEdit
    }
    else if(m_render_settings_container.GetRenderTargetType() == IRenderSettingsContainer::RenderTargetType::ToneOperatorPreview)
    {
        return 5;       // 5 seconds for tone op preview
    }
    else if(m_param_block != nullptr)
    {
        if(m_param_block->GetInt(RapidRTRendererPlugin::ParamID_Termination_EnableTime))
        {
            // Minimum is 1
            return std::max(1, m_param_block->GetInt(RapidRTRendererPlugin::ParamID_Termination_TimeInSeconds));
        }
    }

    return 0;   // infinite
}

float RapidRenderSettingsWrapper::get_termination_quality() const
{
    if(m_render_settings_container.GetIsMEditRender())
    {
        return rti::ConvergenceLevel::Medium;
    }
    else if(m_render_settings_container.GetRenderTargetType() == IRenderSettingsContainer::RenderTargetType::ToneOperatorPreview)
    {
        return rti::ConvergenceLevel::Medium;
    }
    else if(m_param_block != nullptr)
    {
        return m_param_block->GetFloat(RapidRTRendererPlugin::ParamID_Termination_Quality_dB);
    }

    return 0.0f;   // infinite
}

float RapidRenderSettingsWrapper::get_point_light_radius() const
{
    if(m_param_block != nullptr)
    {
        return std::max(m_param_block->GetFloat(RapidRTRendererPlugin::ParamID_PointLightDiameter) * 0.5f, 1e-7f);     // Disallow 0-size lights
    }
    else
    {
        return 0.5f;
    }
}

bool RapidRenderSettingsWrapper::get_enabled_outlier_clamp() const
{
    // Disable outlier clamp for tone operator preview, to avoid clamping the results as we don't know what the final tone mapping will be.
    if(m_render_settings_container.GetRenderTargetType() == IRenderSettingsContainer::RenderTargetType::ToneOperatorPreview)
    {
        return false;
    }
    else
    {
        return (m_param_block != nullptr) && (m_param_block->GetInt(RapidRTRendererPlugin::ParamID_EnableOutlierClamp) != 0);
    }
}

float RapidRenderSettingsWrapper::get_filter_diameter() const
{
    return (m_param_block != nullptr) ? m_param_block->GetFloat(RapidRTRendererPlugin::ParamID_FilterDiameter) : 1.0f;
}

bool RapidRenderSettingsWrapper::get_enable_animated_noise() const
{
    return (m_param_block != nullptr) ? !!m_param_block->GetInt(RapidRTRendererPlugin::ParamID_EnableAnimatedNoise) : false;
}

unsigned int RapidRenderSettingsWrapper::get_max_down_res_factor() const
{
    return (m_param_block != nullptr) ? m_param_block->GetInt(RapidRTRendererPlugin::ParamID_Maximum_DownResFactor) : 1;
}

IPoint2 RapidRenderSettingsWrapper::get_texture_bake_resolution() const
{
    int resolution = (m_param_block != nullptr) ? m_param_block->GetInt(RapidRTRendererPlugin::ParamID_Texture_Bake_Resolution) : 256;
    resolution = std::min(std::max(resolution, 1), 16000);        // clamp to sane range
    return IPoint2(resolution, resolution);
}

float RapidRenderSettingsWrapper::get_main_frame_buffer_noise_filtering_strength() const
{
    if(m_param_block != nullptr) 
    {
        const bool enable_noise_filter = !!m_param_block->GetInt(RapidRTRendererPlugin::ParamID_EnableNoiseFilter);
        const float strength = enable_noise_filter ? m_param_block->GetFloat(RapidRTRendererPlugin::ParamID_NoiseFilterStrength) : 0.0f;
        return strength;
    }
    else
    {
        return 0.0f;
    }
}

}}	// namespace 
