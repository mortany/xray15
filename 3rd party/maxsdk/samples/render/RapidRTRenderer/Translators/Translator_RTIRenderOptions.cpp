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

#include "Translator_RTIRenderOptions.h"

// Local
#include "../RapidRenderSettingsWrapper.h"
#include "../Util.h"
#include "../resource.h"
#include "../plugins/NoiseFilter_RenderElement.h"

// Max sdk
#include <Rendering/ToneOperator.h>
#include <dllutilities.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>
#include <algorithm>

#undef min
#undef max

namespace
{
    inline float log_base_2(const float val)
    {
        // log base 2 = ln(val) / ln(2.0)
        // M_LN2 == ln(2.0)
        return log(val) * (1.0f / M_LN2);        
    }
}

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_RTIRenderOptions::Translator_RTIRenderOptions(const Key& /*key*/, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_RTIRenderOptions::~Translator_RTIRenderOptions()
{

}

rti::RenderOptionsHandle Translator_RTIRenderOptions::get_output_render_options() const
{
    return get_output_handle<rti::RenderOptionsHandle>(0);
}

TranslationResult Translator_RTIRenderOptions::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    // Initialize the render options
    const rti::RenderOptionsHandle render_options_handle = initialize_output_handle<rti::RenderOptionsHandle>(0);
    if(render_options_handle.isValid())
    {
        const IRenderSettingsContainer& render_settings_container = GetRenderSessionContext().GetRenderSettings();
        const RapidRenderSettingsWrapper settings_wrapper(render_settings_container);

        rti::TEditPtr<rti::RenderOptions> render_options(render_options_handle);

        // We check the success state of these calls, but always return success - we wouldn't want to abort rendering just because a render
        // setting couldn't be set (I choose not to consider that a fatal error).
        int num_iterations = settings_wrapper.get_termination_iterations();
        if(num_iterations <= 0) // infinite
        {
            num_iterations = std::numeric_limits<int>::max() / 4;
        }
        RRTUtil::check_rti_result(render_options->setOption1i(rti::OPTION_PROGRESSIVE_MAX_ITERATIONS, num_iterations));
        RRTUtil::check_rti_result(render_options->setOption1i(rti::OPTION_KERNEL, settings_wrapper.get_rapid_kernel_to_use()));
        
        // Enable alpha channel generation at all times
        RRTUtil::check_rti_result(render_options->setOption1i(rti::OPTION_ALPHA_CHANNEL, rti::ALPHA_PREMULTIPLIED_WITH_MATTE_ILLUMINATION)); // ZAP CHECK THIS !!

        // The scene type controls what acceleration structure is used in Rapid. Dynamic is optimized for fast changes (but slower renders),
        // whereas static is optimized for faster renders (but slower builds).
        bool use_dynamic_scene = false;
        switch(render_settings_container.GetRenderTargetType())
        {
        default:
            DbgAssert(false);
            // Fall into...
        case IRenderSettingsContainer::RenderTargetType::MaterialEditor:
        case IRenderSettingsContainer::RenderTargetType::AnimationOrStill:
        case IRenderSettingsContainer::RenderTargetType::ToneOperatorPreview:
            use_dynamic_scene = false;
            break;
        case IRenderSettingsContainer::RenderTargetType::Interactive:
            use_dynamic_scene = true;
            break;
        }

        RRTUtil::check_rti_result(render_options->setOption1i(rti::OPTION_SCENE_TYPE, use_dynamic_scene ? rti::SCENE_DYNAMIC : rti::SCENE_STATIC));

        // Set quality convergence target
        const float quality_target = settings_wrapper.get_termination_quality();
        RRTUtil::check_rti_result(render_options->setOption1i(rti::OPTION_TARGET_TYPE, rti::TARGET_CONVERGENCE));
        RRTUtil::check_rti_result(render_options->setOption1f(rti::OPTION_CONVERGENCE_TARGET, quality_target));

        // Set filter
        RRTUtil::check_rti_result(render_options->setOption1i(rti::OPTION_FILTER_TYPE, rti::FILTER_GAUSSIAN));
        // Valid range is 1.0 to 10.0 (values outside of this get rejected outright)
        const float filter_diameter = std::max(std::min(settings_wrapper.get_filter_diameter(), 10.0f), 1.0f);
        RRTUtil::check_rti_result(render_options->setOption1f(rti::OPTION_FILTER_WIDTH, filter_diameter));
        RRTUtil::check_rti_result(render_options->setOption1f(rti::OPTION_FILTER_HEIGHT, filter_diameter));

        // Setup outlier clamp
        float exposure_multiplier = 0.0f;
        if(settings_wrapper.get_enabled_outlier_clamp() && get_physical_value_for_white(exposure_multiplier, t, validity))
        {
            // Calculate the exposure value from exposure multiplier
            const float outlier_clamp_ev = -log_base_2(exposure_multiplier);
            render_options->setOption1b(rti::OPTION_AUTOMATIC_OUTLIER_CLAMP, true);
            render_options->setOption1f(rti::OPTION_OUTLIER_CLAMP, 0.0f);            // not used, just set to 0
            render_options->setOption1f(rti::OPTION_EXPOSURE_HINT, outlier_clamp_ev);
        }
        else
        {
            // Disable outlier clamping
            render_options->setOption1b(rti::OPTION_AUTOMATIC_OUTLIER_CLAMP, false);
            render_options->setOption1f(rti::OPTION_OUTLIER_CLAMP, 0.0f);       
            render_options->setOption1f(rti::OPTION_EXPOSURE_HINT, 0.0f);
        }

        // Noise filtering
        {
            bool enable_noise_filtering = (settings_wrapper.get_main_frame_buffer_noise_filtering_strength() > 0.0f);

            if(!enable_noise_filtering)
            {
                // Enable noise filtering if the render element is present
                const std::vector<IRenderElement*> render_elements = GetRenderSessionContext().GetRenderElements();
                for(IRenderElement* const element : render_elements)
                {
                    if(dynamic_cast<NoiseFilter_RenderElement*>(element) != nullptr)
                    {
                        enable_noise_filtering = true;
                    }
                }
            }

            RRTUtil::check_rti_result(render_options->setOption1b(rti::OPTION_NOISE_FILTERING, enable_noise_filtering));
        }

        return TranslationResult::Success;
    }

    return TranslationResult::Failure;
}

bool Translator_RTIRenderOptions::get_physical_value_for_white(float& value, const TimeValue t, Interval& valid) const
{
    ToneOperator* const tone_op = GetRenderSessionContext().GetRenderSettings().GetToneOperator();
    if(tone_op != nullptr)
    {
        // Initialize the tone operator
        const ICameraContainer& camera_container = GetRenderSessionContext().GetCamera();
        const IPoint2 resolution = camera_container.GetResolution();
        tone_op->Update(t, camera_container.GetCameraNode(), resolution, camera_container.GetPixelAspectRatio(), valid);
        const Point2 xy_coord(resolution.x * 0.5f, resolution.y * 0.5f);      // center of image, in case there's vignetting

        // Calculate what raw value gets remapped to 1.0 by the exposure control.
        // To avoid relying on an inverse exposure function, which may not be supported.
        float range_min = 0.0f;
        float range_max = 1e20f;        // Large enough to be pretty sure to contain the target value, but not large enough that we'll run into trouble

        // 100 iterations should be plenty (1e20 / 2^100 ~= 7.9e-11)
        for(size_t iteration = 0; iteration < 100; ++iteration)
        {
            // Calculate exposure on center of current range
            const float range_center = (range_min * 0.5f) + (range_max * 0.5f);
            const float current_exposed_value = tone_op->ScaledToRGB(range_center, xy_coord);

            // See if we're close enough to white
            const float threshold = 0.001f; // Within 0.1% of white
            if(fabsf(1.0f - current_exposed_value) <= threshold)
            {
                value = range_center;
                return true;
            }
            else if(current_exposed_value < 1.0f)
            {
                // Need brighter result: right
                range_min = range_center;
            }
            else    // current_exposed_value > 1.0f
            {
                // Need darker result: left
                range_max = range_center;
            }
        }

        // If we get here, it's almost guaranteed that our search failed because the exposure control is kinda weird. For example, it could map
        // values in negative colors (i.e. non-ascending values). Or the exposure could map to pseudo-colors, which never are white.
        // Another example is the logarithmic tone operator, which maps all values to a curve that's strictly below 1.0.
        // In those cases, we simply disable the outlier clamp.
        GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Info, MaxSDK::GetResourceStringAsMSTR(IDS_WARNING_OUTLIER_CLAMP_EXPOSURE_CONTROL));
        return false;
    }
    else
    {
        // No exposure control: direct mapping
        value = 1.0f;
        return true;
    }
}

Interval Translator_RTIRenderOptions::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const
{
    return previous_validity;
}

void Translator_RTIRenderOptions::PreTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

void Translator_RTIRenderOptions::PostTranslate(const TimeValue /*translationTime*/ , Interval& /*validity*/)
{
    // Do nothing
}

MSTR Translator_RTIRenderOptions::GetTimingCategory() const 
{
    return MSTR();
}

}}	// namespace 
