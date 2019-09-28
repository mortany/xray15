//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "RapidRenderSession.h"

// Local includes
#include "resource.h"
#include "RapidRTRendererPlugin.h"
#include "RapidRenderSettingsWrapper.h"
#include "plugins/NoiseFilter_RenderElement.h"
#include "Translators/Translator_RTIScene.h"
#include "Util.h"
// 3ds max sdk
#include <StopWatch.h>
#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Translator/TranslatorStatistics.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
// rti
#include <rti/util/util.h>
// std
#include <algorithm>

#undef min
#undef max

namespace Max 
{;
namespace RapidRTTranslator 
{;

namespace
{
    template<typename T>
    T max(const T& v1, const T& v2, const T& v3)
    {
        return std::max(std::max(v1, v2), v3);
    }

    template<typename T>
    T lerp(const T& val0, const T& val1, const T& lerp_amount)
    {
        return val0 + (lerp_amount * (val1 - val0));
    }
}
RapidRenderSession::RapidRenderSession(
    IRenderSessionContext& sessionContext,
    const bool is_interactive_session)

    : m_scene_root_translator(nullptr),
    m_rendering_logger(sessionContext.GetLogger()),
    m_rendering_process(sessionContext.GetRenderingProcess()),
    m_render_session_context(sessionContext),
    m_abort_requested(false),
    m_stop_requested(false),
    m_renderer_paused(false),
    m_is_interactive_renderer(sessionContext.GetNotificationClient() != nullptr),
    // Free a thread for both MEdit and interactive rendering, to enable smooth UI updates.
    m_render_job(IRapidAPIManager::get_instance().create_render_job(sessionContext.GetRenderSettings().GetIsMEditRender() || is_interactive_session)),
    m_translation_time(0),
    m_termination_iterations(0),
    m_termination_seconds(0),
    m_termination_quality_db(0.0f),
    m_noise_filter_strength(0.0f),
    m_rendered_iterations(0),
    m_down_res_full_resolution(0, 0),
    m_next_down_res_factor(1),
    m_rendered_seconds(0),
    m_rendered_quality_db(0.0f),
    m_termination_criteria_reached(false),
    m_num_iterations_active(0),
    m_noise_filtering_buffer_width(0),
    m_noise_filtering_buffer_height(0),
    m_noise_filtering_buffer_format(rti::FB_FORMAT_RGBA_F32)
{
    // Register for change notifications against the render settings (to be notified when the tone operator changes)
    m_render_session_context.GetRenderSettings().RegisterChangeNotifier(*this);
}

RapidRenderSession::~RapidRenderSession(void)
{
    // Unregister for change notifications against the render settings
    m_render_session_context.GetRenderSettings().UnregisterChangeNotifier(*this);

    // Release the root translator
    if(m_scene_root_translator != nullptr)
    {
        m_render_session_context.GetTranslationManager().ReleaseSceneTranslator(*m_scene_root_translator);
        m_scene_root_translator = nullptr;
    }
}

bool RapidRenderSession::translate_or_update_scene(const TimeValue t, bool& scene_updated)
{
    scene_updated = false;

    ITranslationManager& translation_manager = m_render_session_context.GetTranslationManager();

    // Down-res first frame of ActiveShade to improve navigation performance, we adaptively down-res the first iteration while doing ActiveShade.
    const bool enable_down_res = 
        m_is_interactive_renderer 
        && ((m_scene_root_translator == nullptr) || translation_manager.DoesSceneNeedUpdate(*m_scene_root_translator, t));
    m_down_res_full_resolution = m_render_session_context.SetDownResolutionFactor(enable_down_res ? m_next_down_res_factor : 1);

    if((m_scene_root_translator == nullptr) || translation_manager.DoesSceneNeedUpdate(*m_scene_root_translator, t))
    {
        TranslationResult result;

        m_rendering_process.TranslationStarted();
        if(m_scene_root_translator == nullptr)
        {
            // Acquire the scene translator initially
            m_scene_root_translator = translation_manager.TranslateScene<Translator_RTIScene>(Translator_RTIScene::Key(), t, result);
        }
        else
        {
            // Update the scene translator which we already acquired
            result = translation_manager.UpdateScene(*m_scene_root_translator, t);
        }
        m_rendering_process.TranslationFinished();

        scene_updated = true;
        return (result == TranslationResult::Success);
    }
    else
    {
        // Scene is up-to-date: nothing to do
        return true;
    }
}

bool RapidRenderSession::setup_for_new_render(const TimeValue t)
{
    m_translation_time = t;

    if(m_render_job != nullptr)
    {
        // Reset rendering flags
        m_abort_requested = false;
        m_stop_requested = false;
        m_renderer_paused = false;
        RapidRenderSettingsWrapper settingsWrapper(m_render_session_context.GetRenderSettings());

        // Reset rendering stats
        m_rendered_iterations = 0;
        m_rendered_seconds = 0;
        m_rendered_quality_db = 0.0f;
        DbgAssert(m_num_iterations_active == 0);        // should be zero, we shouldn't be currently rendering when calling this method
        m_num_iterations_active = 0;
        m_termination_criteria_reached = false;

        // Initialize termination criteria
        m_termination_iterations = settingsWrapper.get_termination_iterations();
        m_termination_seconds = settingsWrapper.get_termination_seconds();
        m_termination_quality_db = settingsWrapper.get_termination_quality();
        
        // Initialize noise filter
        m_noise_filter_strength = settingsWrapper.get_main_frame_buffer_noise_filtering_strength();

        // Maximum down-res factor
        m_max_down_res_factor = settingsWrapper.get_max_down_res_factor();

        // Translate the scene
        m_rendering_process.SetRenderingProgressTitle(MaxSDK::GetResourceStringAsMSTR(IDS_RENDER_INIT_SCENE));
        bool scene_updated = false;
        if(translate_or_update_scene(t, scene_updated))
        {
            // Report scene statistics
            {
                TranslatorStatistics statistics;
                if(m_scene_root_translator != nullptr)
                {
                    m_scene_root_translator->AccumulateGraphStatistics(statistics);
                }
                m_rendering_process.SetSceneStats(
                    static_cast<int>(statistics.GetNumLights()), 
                    static_cast<int>(statistics.GetNumLights()), 
                    0, 
                    static_cast<int>(statistics.GetNumGeomObjects()), 
                    static_cast<int>(statistics.GetNumFaces()));
            }

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_INIT_RAPID));
        return false;
    }
}

bool RapidRenderSession::InitiateInteractiveSession(const TimeValue t)
{
    // Setup Rapid and translate initial scene
    return setup_for_new_render(t);
}

bool RapidRenderSession::render_rti_frame(const TimeValue t, bool& done_rendering)
{
    // Do not process frame updates, or request new frames, when the renderer is paused.
    if(!m_renderer_paused)
    {
        rti::IRenderJob* const render_job = (m_render_job != nullptr) ? &(m_render_job->get_render_job()) : nullptr;

        // Sync the Rapid frame buffer: this waits for a frame update to be ready, or until a timeout occurs.
        // Use a timeout of 100ms - meaning we'll check for aborts at least 10 times a second, which should be enough.
        const bool flush_job = m_termination_criteria_reached;      // flush any buffered frames if termination criteria reached, as we won't be submitting any more frame requests
        const rti::RTIResult sync_result = (render_job != nullptr) ? render_job->sync(100, flush_job) : rti::RTI_ERROR;

        if((sync_result == rti::RTI_SUCCESS) && DbgVerify(m_scene_root_translator != nullptr))
        {
            // Update the scene if necessary
            bool scene_updated = false;
            if(!translate_or_update_scene(t, scene_updated))
            {
                return false;
            }
            DbgAssert(!scene_updated || m_is_interactive_renderer);       // scene should not be updated for offline render

            // Request a new frame to be rendered, if either:
            // * Termination criteria not reached
            // * Scene was just updated (i.e. we're restarting render even if termination critiera were previously reached)
            // * Need frame buffer update for tone operator update
            const bool frame_buffer_update_needed = m_frame_buffer_update_needed.exchange(false);
            if(!m_termination_criteria_reached || scene_updated || frame_buffer_update_needed)
            {
                // Signal (re)start of rendering
                if(((m_rendered_iterations == 0) && (m_num_iterations_active == 0))     // First iteration
                    || scene_updated)       // Re-start after scene change
                {
                    m_rendering_process.RenderingStarted();
                }

                // Enable buffering if there's more than 1 frame left to render
                const unsigned int total_iterations_submitted = m_rendered_iterations + m_num_iterations_active;
                const bool enable_buffering = 
                    !m_termination_criteria_reached 
                    && ((m_termination_iterations == 0) || ((m_termination_iterations - total_iterations_submitted) > 1));

                // Start the iteration stopwatch on first frame (further frames get the stopwatch restarted in the swap callback)
                if(!m_iteration_stopwatch.IsRunning())
                {
                    m_iteration_stopwatch.Start();
                }

                // Request that an iteration be rendered
                ++m_num_iterations_active;
                switch(render_job->frame(m_scene_root_translator->get_output_scene(), enable_buffering))
                {
                case rti::RTI_SUCCESS:
                    break;
                case rti::RTI_OUT_OF_MEMORY:
                    m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_RENDERING_OUT_OF_MEMORY));
                    return false;
                default:
                    m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_RENDERING_ERROR));
                    return false;
                }
            }
            else
            {
                // Signal end of rendering
                if(m_termination_criteria_reached && (m_num_iterations_active == 0))
                {
                    m_rendering_process.RenderingFinished();
                }
            }
        }
        else if(sync_result == rti::RTI_SYNC_TIMEOUT)
        {
            // No frame update ready
        }
        else
        {
            // Error
            return false;
        }
    }

    done_rendering = (m_termination_criteria_reached && (m_num_iterations_active == 0));
    return true;
}

bool RapidRenderSession::UpdateInteractiveSession(const TimeValue t, bool& done_rendering)
{
    return render_rti_frame(t, done_rendering);
}

bool RapidRenderSession::TranslateScene(const TimeValue t)
{
    // Force the renderer to restart by invalidating the root translator.
    // This wouldn't be necessary if we could force Rapid to call the swap callback again with the last rendered image.
    if(m_scene_root_translator != nullptr)
    {
        m_scene_root_translator->force_invalidate();
    }

    return setup_for_new_render(t);
}

bool RapidRenderSession::RenderOfflineFrame(const TimeValue t)
{
    bool done_rendering = false;
    while(render_rti_frame(t, done_rendering) && !done_rendering && !m_abort_requested)
    {
        // Keep looping so long as we're not done
        if(m_rendering_process.HasAbortBeenRequested())
        {
            m_abort_requested = true;
        }
    }

    DbgAssert(done_rendering || m_abort_requested);
    return (done_rendering || m_abort_requested);
}

bool RapidRenderSession::StoreRenderElementResult(const TimeValue t, IRenderElement& render_element, IFrameBufferProcessor& frame_buffer_processor) 
{
    bool any_element_failed = false;
    NoiseFilter_RenderElement* const noise_filter_render_element = dynamic_cast<NoiseFilter_RenderElement*>(&render_element);
    if(noise_filter_render_element != nullptr)
    {
        // Get the mix amount
        const float filtered_mix_amount = noise_filter_render_element->get_filtered_mix_amount();
        FrameBufferReader frame_buffer_reader(
            m_noise_filtering_unfiltered_buffer.data(), 
            m_noise_filtering_filtered_buffer.data(),
            m_noise_filtering_buffer_width,
            m_noise_filtering_buffer_height,
            m_noise_filtering_region,
            m_noise_filtering_buffer_format,
            filtered_mix_amount);
        const Box2 region = m_render_session_context.GetCamera().GetRegion();

        if(!frame_buffer_processor.ProcessFrameBuffer(true, t, frame_buffer_reader))
        {
            any_element_failed = true;
        }
    }

    return !any_element_failed;
}

void RapidRenderSession::StopRendering()
{
    m_stop_requested = true;
    
    // Resume rendering if paused, to let rendering finish.
    ResumeRendering();
}

void RapidRenderSession::PauseRendering() 
{
    // Don't pause if rendering is being stopped
    if(!m_stop_requested)
    {
        const bool was_paused = m_renderer_paused.exchange(true);
        if(!was_paused)
        {
            m_rendering_process.RenderingPaused();
        }
    }
}

void RapidRenderSession::ResumeRendering() 
{
    const bool was_paused = m_renderer_paused.exchange(false);
    if(was_paused)
    {
        m_rendering_process.RenderingResumed();
    }
}

void RapidRenderSession::swap(const	rti::SwapParams	&params)
{
    // Reset the rendering stats upon receiving the first iteration after translating/updating the scene.
    if(params.m_iteration == 0)
    {
        m_rendered_iterations = 0;
        m_rendered_seconds = 0;
        m_rendered_quality_db = 0.0f;
        m_termination_criteria_reached = false;
    }

    // Determine how many iterations have been rendered so far
    const unsigned int new_iterations_rendered = (params.m_iteration + 1) - m_rendered_iterations;
    // Determine if the rendered frame is a down-res'ed frame for interactive rendering
    const bool is_down_res_frame = (m_rendered_iterations == 0) && (params.m_width < m_down_res_full_resolution.x) && (params.m_height < m_down_res_full_resolution.y);

    // Update rendering statistics & progress
    if(new_iterations_rendered > 0)
    {
        DbgAssert(m_num_iterations_active >= new_iterations_rendered);
        m_num_iterations_active -= new_iterations_rendered;
        m_rendered_iterations += new_iterations_rendered;
        m_rendered_seconds = m_rendering_process.GetRenderingTimeInSeconds();
        m_rendered_quality_db = params.m_convergenceParams.m_convergencePSNR;

        if(!is_down_res_frame)
        {
            // Report progress information
            {
                // We report a multi-line title string with various useful information
                TSTR temp;
                TSTR progress_title_string;

                // Report iteration progress
                {
                    if(m_termination_iterations > 0)
                    {
                    temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_TERMINATION_ITERATION), m_rendered_iterations, m_termination_iterations);
                    }
                    else
                    {
                    temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_RENDERED_ITERATION), m_rendered_iterations);
                    }
                    progress_title_string += temp;
                }

                // Report quality progress
                {
                    const TSTR quality_preset_reached = RapidRTRendererPlugin::get_matching_quality_preset_name(m_rendered_quality_db);
                    if(!_finite(m_rendered_quality_db))
                    {
                        // Infinite quality happens when rendering a perfectly black image (and probably in other cases, too)
                    temp = MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_RENDERED_QUALITY_INFINITE);
                    }
                    else if(m_termination_quality_db > 0.0f)
                    {
                        // Report quality against target
                    temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_TERMINATION_QUALITY), m_rendered_quality_db, quality_preset_reached.data(), m_termination_quality_db);
                    }
                    else
                    {
                        // Report quality
                    temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_RENDERED_QUALITY), m_rendered_quality_db, quality_preset_reached.data());
                    }

                    progress_title_string += _M("\n");
                    progress_title_string += temp;
                }

                // Report time progress
                {
                    if(m_termination_seconds > 0)
                    {
                        const RRTUtil::Time_HMS rendered_time = RRTUtil::Time_HMS::from_seconds(m_rendered_seconds);
                        const RRTUtil::Time_HMS termination_time = RRTUtil::Time_HMS::from_seconds(m_termination_seconds);
                    temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_TERMINATION_TIME),
                            rendered_time.hours, rendered_time.minutes, rendered_time.seconds,
                            termination_time.hours, termination_time.minutes, termination_time.seconds);
                    }
                    else
                    {
                        const RRTUtil::Time_HMS rendered_time = RRTUtil::Time_HMS::from_seconds(m_rendered_seconds);
                    temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_RENDERED_TIME), rendered_time.hours, rendered_time.minutes, rendered_time.seconds);
                    }
                    progress_title_string += _M("\n");
                    progress_title_string += temp;
                }

                const bool m_infinite_render = (m_termination_iterations == 0) && (m_termination_seconds == 0) && (m_termination_quality_db == 0);

                // Report progress percentage
                if(m_infinite_render)
                {
                    m_rendering_process.SetRenderingProgressTitle(progress_title_string);
                    m_rendering_process.SetInfiniteProgress(m_rendered_iterations, IRenderingProcess::ProgressType::Rendering);
                }
                else 
                {
                    // Calculate progress
                    const float iteration_progress = (m_termination_iterations > 0) ? (float(m_rendered_iterations) / float(m_termination_iterations)) : 0.0f;
                    const float time_progress = (m_termination_seconds > 0) ? (float(m_rendered_seconds) / float(m_termination_seconds)) : 0.0f;
                    const float quality_progress = (m_termination_quality_db > 0.0f) ? params.m_convergenceParams.m_progress : 0.0f;
                    // Clamp overall progress to 1.0, as it may surpass that because we will always render 1 more iteration after the time or quality 
                    // criteria are reached.
                    const float overall_progress = std::min(max(iteration_progress, time_progress, quality_progress), 1.0f);

                    // Calculate and report time estimate if possible
                    if((overall_progress > 0.0f) && (overall_progress < 1.0f))
                    {
                        // Calculate the time estimate by extrapolating from render time and current progress
                        unsigned int time_estimate = static_cast<unsigned int>(m_rendered_seconds * (1.0f - overall_progress) / overall_progress);
                        if(overall_progress == quality_progress)
                        {
                            // If using quality as the progress, then use the time estimate provided by Rapid. It tends to be more accurate, 
                            // seems to include some useful heuristics that adjust the estimate if progress stalls.
                            time_estimate = static_cast<unsigned int>(std::max(0.0f, params.m_convergenceParams.m_timeToImage));
                        }

                        const RRTUtil::Time_HMS time_estimate_hms = RRTUtil::Time_HMS::from_seconds(time_estimate);

                        // Report "exact time" versus "approx. time" if time is the only selected criterion
                        const bool report_exact_time = (m_termination_iterations == 0) && (m_termination_quality_db == 0.0f);

                    temp.printf(MaxSDK::GetResourceStringAsMSTR(report_exact_time ? IDS_PROGRESS_TIME_EXACT : IDS_PROGRESS_TIME_ESTIMATE), time_estimate_hms.hours, time_estimate_hms.minutes, time_estimate_hms.seconds);
                        progress_title_string += _M("\n");
                        progress_title_string += temp;
                    }

                    m_rendering_process.SetRenderingProgressTitle(progress_title_string);
                    m_rendering_process.SetRenderingProgress(static_cast<size_t>(overall_progress * 1000.0f), 1000, IRenderingProcess::ProgressType::Rendering);
                }
            }

            // Update termination criteria
            if(!m_termination_criteria_reached)
            {
                // Iterations
                if((m_termination_iterations > 0)
                    && (m_rendered_iterations >= m_termination_iterations))
                {
                    DbgAssert(m_rendered_iterations == m_termination_iterations);   // check against rendering more iterations than supposed
                    m_termination_criteria_reached = true;
                m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_TERMINATING_ITERATIONS));
                }

                // Time
                if((m_termination_seconds > 0)
                    && (m_rendered_seconds >= m_termination_seconds))
                {
                    m_termination_criteria_reached = true;
                m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_TERMINATING_TIME));
                }

                // Quality
                if((m_termination_quality_db > 0.0f)
                    && (m_rendered_quality_db >= m_termination_quality_db))
                {
                    m_termination_criteria_reached = true;
                m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_TERMINATING_QUALITY));
                }

                // Explicit request to stop rendering (which triggers an artificial termination that's not an abort)
                if(m_stop_requested)
                {
                    m_termination_criteria_reached = true;
                m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_TERMINATING_USER_REQUEST));
                }
            }
        }
    }
    else
    {
        // No new iterations rendered, we're just updating the frame buffer - likely as a result of the tone operator changing
        TSTR temp;
        temp.printf(MaxSDK::GetResourceStringAsMSTR(IDS_RENDER_UPDATING_ITERATION), m_rendered_iterations);
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Info, temp);
    }

    // Update the bitmap display
    {
        // Run the noise filter, on the last iteration to be rendered
        const bool enable_noise_filter = (m_num_iterations_active == 0);
        if(enable_noise_filter)
        {
            execute_noise_filter(params);
        }

        // For MEdit, don't bother updating the bitmap interactively - only update on the final iteration
        if(!m_render_session_context.GetRenderSettings().GetIsMEditRender() || (m_num_iterations_active == 0))
        {
            const Box2 region(
                IPoint2(params.m_regionOriginX, params.m_regionOriginY), 
                IPoint2(params.m_regionOriginX + params.m_regionWidth - 1, params.m_regionOriginY + params.m_regionHeight - 1));

            // Store the frame buffer
            FrameBufferReader frame_buffer_access(
                params.m_pixelData, 
                enable_noise_filter ? m_noise_filtering_filtered_buffer.data() : nullptr, 
                params.m_width, 
                params.m_height, 
                region,
                params.m_format, 
                enable_noise_filter ? m_noise_filter_strength : 0.0f);
            DbgVerify(m_render_session_context.GetMainFrameBufferProcessor().ProcessFrameBuffer(true, m_translation_time, frame_buffer_access));

            // Update the bitmap window
            m_render_session_context.UpdateBitmapDisplay();
        }
    }

    // Report time taken to render & process this last iteration.
    {
        m_iteration_stopwatch.Stop();
        const float iteration_time_seconds = m_iteration_stopwatch.GetElapsedTime() / 1000.0;
        m_iteration_stopwatch.Reset();

        if(is_down_res_frame)
        {
            // We just rendered a down-res'ed frame. We don't report those normally; and we especially don't care about termination criteria
            // for those. 
            const unsigned int currend_down_res_factor = (m_down_res_full_resolution.x / params.m_width);

            TSTR msg;
            msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_RENDERED_DOWN_RES_ITERATION), currend_down_res_factor);
            m_rendering_process.SetRenderingProgressTitle(msg);
        }

        // Adjust or enable down-res'ing, as needed
        if(is_down_res_frame
            || ((m_next_down_res_factor <= 1) && (m_rendered_iterations == new_iterations_rendered) && m_is_interactive_renderer))
        {
            // Adjust the down-res factor as needed. 
            // This is the range within which we're happy, where we don't try to adjust the down-res factor. Using a range should provide
            // some stability, avoid too much jitter in the factor.
            const float target_frame_time_max = (1.0f / 10.0f);
            const float target_frame_time_min = (1.0f / 20.0f);
            const unsigned int currend_down_res_factor = (m_down_res_full_resolution.x / params.m_width);

            if((iteration_time_seconds > target_frame_time_max) && (currend_down_res_factor < m_max_down_res_factor))
            {
                // Not fast enough: increment down-res factor
                m_next_down_res_factor = currend_down_res_factor + 1;
            }
            else if((iteration_time_seconds < target_frame_time_min) && (currend_down_res_factor > 1))
            {
                // Too fast: decrement down-res factor
                m_next_down_res_factor = currend_down_res_factor - 1;
            }
        }

        // Restart timer immediately if more iterations are already queued
        if(m_num_iterations_active > 0)
        {
            m_iteration_stopwatch.Start();
        }

        TSTR msg;
        msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ITERATION_RENDER_TIME), iteration_time_seconds);
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Info, msg);
    }
}

void RapidRenderSession::execute_noise_filter(const	rti::SwapParams	&params)
{
    if(params.m_filteringEnabled)
    {
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Info, MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_STARTED));
        MaxSDK::Util::StopWatch noise_filter_stopwatch;
        noise_filter_stopwatch.Start();

        // sanity check
        DbgAssert(params.m_bufferSize == (params.m_height * params.m_width * params.m_bytesPerPixel));

        // Exception handling for memory allocation failure
        try
        {
            DbgAssert(params.m_bufferSize == (params.m_width * params.m_height * 4 * sizeof(float)));
            m_noise_filtering_unfiltered_buffer.resize(params.m_bufferSize);
            m_noise_filtering_filtered_buffer.resize(params.m_bufferSize);
            m_noise_filtering_buffer_width = params.m_width;
            m_noise_filtering_buffer_height = params.m_height;
            m_noise_filtering_region = Box2(
                IPoint2(params.m_regionOriginX, params.m_regionOriginY),
                IPoint2(params.m_regionOriginX + params.m_regionWidth - 1, params.m_regionOriginY + params.m_regionHeight - 1));
            m_noise_filtering_buffer_format = params.m_format;

#pragma warning( disable: 4996 )

            // Eventual TODO: Support abort (will require running the filter in a separate thread)
            const rti::RTIResult result = rti::Util::applyDenoiseFilter(m_noise_filtering_filtered_buffer.data(), params.m_filteringParams);
            if(result == rti::RTI_SUCCESS)
            {
                // Save the unfiltered buffer, for later mixing with the filtered one
                memcpy(m_noise_filtering_unfiltered_buffer.data(), params.m_pixelData, params.m_bufferSize);
                TSTR msg;
                msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_SUCCEED), noise_filter_stopwatch.GetElapsedTime() * 1e-3);
                m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Info, msg);
            }
            else  
            {
                // Report error
                switch(result)
                {
                case rti::RTI_OUT_OF_MEMORY:
                    m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_OUT_OF_MEMORY));
                    break;
                case rti::RTI_FILTER_ABORTED:
                    m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Info, MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_ABORTED));
                    break;
                default:
                    m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_ERROR));
                    break;
                }
            }
        }
        catch(...)
        {
            // Memory allocation failed - can't run the noise filter
            m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_OUT_OF_MEMORY));
        }
    }
    else
    {
        m_noise_filtering_unfiltered_buffer.clear();
        m_noise_filtering_filtered_buffer.clear();
    }
}

void RapidRenderSession::TerminateInteractiveSession()
{
    m_abort_requested = true;
    if(m_render_job != nullptr)
    {
        m_render_job->get_render_job().abort();
    }
}

bool RapidRenderSession::IsInteractiveSessionUpToDate(const TimeValue t)
{
    return !m_render_session_context.GetTranslationManager().DoesSceneNeedUpdate(*m_scene_root_translator, t);
}

void RapidRenderSession::NotifyToneOperatorSwapped()
{
    // Update the frame buffer to re-process the tone operator
    m_frame_buffer_update_needed = true;
}

void RapidRenderSession::NotifyToneOperatorParamsChanged()
{
    // Update the frame buffer to re-process the tone operator
    m_frame_buffer_update_needed = true;
}

void RapidRenderSession::NotifyPhysicalScaleChanged()
{
    // Don't care
}

//==================================================================================================
// class RapidRenderSession::FrameBufferReader
//==================================================================================================

RapidRenderSession::FrameBufferReader::FrameBufferReader(
    const void* unfiltered_pixels, 
    const void* filtered_pixels, 
    const int width,
    const int height,
    const Box2& region,
    const rti::EFramebufferFormat format,
    const float filtered_mix_amount)
    
    : m_unfiltered_pixels(unfiltered_pixels),
    m_filtered_pixels(filtered_pixels),
    m_width(width),
    m_height(height),
    m_format(format),
    m_region(region),
    m_filtered_mix_amount(filtered_mix_amount)
{

}

bool RapidRenderSession::FrameBufferReader::GetPixelLine(const unsigned int y, const unsigned int x_start, const unsigned int num_pixels, BMM_Color_fl* const target_pixels) 
{
    if(DbgVerify((y < m_height) && ((x_start + num_pixels) <= m_width)))
    {
        if(DbgVerify(m_format == rti::FB_FORMAT_RGBA_F32))
        {
            // Memory layout of the RRT frame buffer is the same as an array of BMM_Color_fl, so we can just copy the buffer directly
            static_assert(sizeof(BMM_Color_fl) == (4 * sizeof(float)), "BMM_Color_fl has different memory layout than assumed");
            // Note that Max & Rapid have inverted Y coordinates
            const unsigned int y_rapid = (m_height - y - 1);

            if((m_filtered_mix_amount <= 0.0f) && DbgVerify(m_unfiltered_pixels != nullptr))
            {
                const BMM_Color_fl* source_pixels = static_cast<const BMM_Color_fl*>(m_unfiltered_pixels) + (y_rapid * m_width) + x_start;
                memcpy(target_pixels, source_pixels, num_pixels * sizeof(*source_pixels));
                return true;
            }
            else if((m_filtered_mix_amount >= 1.0f) && DbgVerify(m_filtered_pixels != nullptr))
            {
                const BMM_Color_fl* source_pixels = static_cast<const BMM_Color_fl*>(m_filtered_pixels) + (y_rapid * m_width) + x_start;
                memcpy(target_pixels, source_pixels, num_pixels * sizeof(*source_pixels));
                return true;
            }
            else if(DbgVerify((m_unfiltered_pixels != nullptr) && (m_filtered_pixels != nullptr)))
            {
                // Mix the two buffers
                const BMM_Color_fl* filtered_pixels = static_cast<const BMM_Color_fl*>(m_filtered_pixels) + (y_rapid * m_width) + x_start;
                const BMM_Color_fl* unfiltered_pixels = static_cast<const BMM_Color_fl*>(m_unfiltered_pixels) + (y_rapid * m_width) + x_start;
                for(size_t i = 0; i < num_pixels; ++i, ++filtered_pixels, ++unfiltered_pixels)
                {
                    BMM_Color_fl& target_pixel = target_pixels[i];
                    target_pixel.r = lerp(unfiltered_pixels->r, filtered_pixels->r, m_filtered_mix_amount);
                    target_pixel.g = lerp(unfiltered_pixels->g, filtered_pixels->g, m_filtered_mix_amount);
                    target_pixel.b = lerp(unfiltered_pixels->b, filtered_pixels->b, m_filtered_mix_amount);
                    target_pixel.a = unfiltered_pixels->a;
                }
                return true;
            }
        }
    }

    // error
    return false;
}

IPoint2 RapidRenderSession::FrameBufferReader::GetResolution() const
{
    return IPoint2(m_width, m_height);
}

Box2 RapidRenderSession::FrameBufferReader::GetRegion() const
{
    // Note that Max & Rapid have inverted Y coordinates
    Box2 max_region;
    max_region.left = m_region.left;
    max_region.right = m_region.right;
    max_region.top = m_height - m_region.bottom - 1;
    max_region.bottom = m_height - m_region.top - 1;

    DbgAssert((max_region.top >= 0) && (max_region.top < m_height) && (max_region.bottom >= 0) && (max_region.bottom < m_height));

    return max_region;
}

}}  // namespace