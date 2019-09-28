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

// rendering api
#include <RenderingAPI/Renderer/ISceneContainer.h>
// max sdk
#include <strclass.h>
#include <Noncopyable.h>
// rti
#include <rti/scene/RenderOptions.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI;

// Wraps the IRenderSettingsContainer to expose render settings specific to RapidRT
class RapidRenderSettingsWrapper :
    public MaxSDK::Util::Noncopyable
{
public:

    RapidRenderSettingsWrapper(const IRenderSettingsContainer& render_settings_container);

    bool get_enable_caustics() const;
    rti::EKernel get_rapid_kernel_to_use() const;
    float get_point_light_radius() const;

    // Number of iterations to be rendered, or 0 if not terminating based on iterations
    unsigned int get_termination_iterations() const;
    // Time, in seconds, to render, or 0 if not terminating based on time
    unsigned int get_termination_seconds() const;
    // Quality target, in decibels, or 0 if not terminating based on quality
    float get_termination_quality() const;

    bool get_enabled_outlier_clamp() const;
    float get_filter_diameter() const;
    bool get_enable_animated_noise() const;

    unsigned int get_max_down_res_factor() const;

    IPoint2 get_texture_bake_resolution() const;

    // Returns the noise filter strength to use, for the main frame buffer, 0.0 if it's disabled.
    float get_main_frame_buffer_noise_filtering_strength() const;

private:

    const IRenderSettingsContainer& m_render_settings_container;
    IParamBlock2* const m_param_block;
};

}}	// namespace 

