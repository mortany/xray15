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

// rendering API
#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Translator/Helpers/INodeInstancingPool.h>
#include <RenderingAPI/Renderer/IInteractiveRenderSession.h>
#include <RenderingAPI/Renderer/IOfflineRenderSession.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/IFrameBufferProcessor.h>
// max sdk
#include <RenderingAPI/Translator/Translator.h>
#include <Rendering\IRenderMessageManager.h>
#include <bmmlib.h>
#include <Max.h>
#include <stdmat.h>
#include <interactiverender.h>
#include <StopWatch.h>
// std
#include <map>
#include <atomic>
// local
#include "IRapidAPIManager.h"

namespace Max 
{;
namespace RapidRTTranslator 
{;

using namespace RapidRTCore;
using namespace MaxSDK;
using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::RenderingAPI::TranslationHelpers;

class RootTranslator;
class Translator_RTIScene;
class RRTRenderParams;

class RapidRenderSession : 
    public Util::Noncopyable, 
    public rti::IFramebufferCallback, 
    public IOfflineRenderSession,
    public IInteractiveRenderSession,
    // This is to keep track of changes in the tone operator
    private IRenderSettingsContainer::IChangeNotifier
{
public:

	//We need to store what created us to call it back to set the NULL pointer on the RapidRenderSession.
	// Either pRenderer or pInteractiveRenderer is non-null, they can't be both NULL or both non-null.
	RapidRenderSession(IRenderSessionContext& sessionContext, const bool is_interactive_session);
	virtual ~RapidRenderSession(void);

    // -- inherited from IOfflineRenderSession
    virtual bool TranslateScene(const TimeValue t) override;
    virtual bool RenderOfflineFrame(const TimeValue t) override;
    virtual bool StoreRenderElementResult(const TimeValue t, IRenderElement& render_element, IFrameBufferProcessor& frame_buffer_processor) override;
    virtual void StopRendering() override;
    virtual void PauseRendering() override;
    virtual void ResumeRendering() override;

    // -- inherited from IInteractiveRenderSession 
    virtual bool InitiateInteractiveSession(const TimeValue t) override;
    virtual bool UpdateInteractiveSession(const TimeValue t, bool& done_rendering) override;
    virtual void TerminateInteractiveSession() override;
    virtual bool IsInteractiveSessionUpToDate(const TimeValue t) override;

private:

    class FrameBufferReader;

    // Initializes RapidRT core stuff
	bool initialize_rti();

    // Initially translates the scene and setups RTI for rendering
    bool setup_for_new_render(const TimeValue t);

    // Translates or updates the scene, as needed. Returns false on error.
    bool translate_or_update_scene(const TimeValue t, bool& scene_updated);

    // This method renders a single frame if possible, updating the scene as necessary in between frames.
    // This method is used for both interactive and offline rendering. For offline rendering, of course, the scene updates shouldn't occur as the
    // scene isn't supposed to change.
    // Returns false on error.
    bool RapidRenderSession::render_rti_frame(
        // The time at which the scene is to be updated, in between frames
        const TimeValue t, 
        // Returns whether all desired iterations have been rendered
        bool& done_rendering);

    // Runs the noise filter on the given frame buffer parameters, saving the result for later use
    void execute_noise_filter(const	rti::SwapParams	&params);

    // -- inherited from rti::IFramebufferCallback
    void swap(const rti::SwapParams &params);

    // -- inherited from IRenderSettingsContainer::IIChangeNotifier
    virtual void NotifyToneOperatorSwapped() override;
    virtual void NotifyToneOperatorParamsChanged() override;
    virtual void NotifyPhysicalScaleChanged() override;

private:

    // This data member must come first - as it needs to be constructed before all the other stuff, since the other stuff may have dependencies
    // on the Rapid API being initialized first.
    std::unique_ptr<IRapidAPIManager::IRenderJob> m_render_job;

    // Termination criteria. We stop when the first one is reached.
    // Each may be disabled by setting it to 0. All disabled means infinite render.
    unsigned int m_termination_iterations;
    unsigned int m_termination_seconds;
    float m_termination_quality_db;
    float m_noise_filter_strength;

    // The maximum factor at which we down-res for initial frames of interactive rendering
    unsigned int m_max_down_res_factor = 1;

    // Statistics accumulated by the last update to the frame buffer. These represent the rendering work that has been done, and which is 
    // fully committed. It does not represent any work in progress but not committed to the frame buffer bitmap.
    unsigned int m_rendered_iterations;
    unsigned int m_rendered_seconds;
    float m_rendered_quality_db;
    unsigned int m_num_iterations_active;       // num iterations currently rendering (submitted but not done)
    // Whether the termination criteria have been reached (thus whether rendering should stop)
    bool m_termination_criteria_reached;

    // Used to measure time lapse between rendering iterations, effectively measuring the rate at which iterations are incoming.
    MaxSDK::Util::StopWatch m_iteration_stopwatch;
    // The full-frame resolution without any down-res'ing, used to adapt the down-res factor
    IPoint2 m_down_res_full_resolution;
    // Divider for resolution when down-res'ing for interactivity
    unsigned int m_next_down_res_factor;

    // Translation components which are allocated by the calling renderer
    IRenderSessionContext& m_render_session_context;

    // Translator components which are allocated and managed by this class
	std::unique_ptr<ITranslationManager> m_translation_manager;
    Translator_RTIScene* m_scene_root_translator;

    IRenderingLogger& m_rendering_logger;
    IRenderingProcess& m_rendering_process;

    std::atomic<bool> m_abort_requested;
    // Request to stop the render at the next opportunity. This is not an abort, it's equivalent to a manually-triggered termination criterion.
    // (i.e. behaviour should be the same as if any termination criterion had been reached).
    std::atomic<bool> m_stop_requested;
    // Set whenever the renderer should be paused (i.e. new frame requests will not be sent while paused)
    std::atomic<bool> m_renderer_paused;

    // Whether this manager was created for interactive or offline rendering
    const bool m_is_interactive_renderer;
    
    // The time at which the scene was last translated
    TimeValue m_translation_time;

    // This boolean is used to request a frame buffer update whenever the tone operator needs re-processing.
    std::atomic_bool m_frame_buffer_update_needed;

    // Buffers where we store the Rapid frame buffer as well as the result of the noise filter, for eventual output into render element buffers
    std::vector<unsigned char> m_noise_filtering_unfiltered_buffer;
    std::vector<unsigned char> m_noise_filtering_filtered_buffer;
    int m_noise_filtering_buffer_width;
    int m_noise_filtering_buffer_height;
    Box2 m_noise_filtering_region;
    rti::EFramebufferFormat m_noise_filtering_buffer_format;
};

// Implementation of interface used to provide frame buffer pixels to the rendering API's frame buffer processing interface.
class RapidRenderSession::FrameBufferReader :
    public IFrameBufferProcessor::IFrameBufferReader
{
public:

    FrameBufferReader(
        const void* unfiltered_pixels, 
        const void* filtered_pixels, 
        const int width,
        const int height,
        // The region, as expressed by RapidRT in its swap params (i.e. y is inversed from max)
        const Box2& region,
        const rti::EFramebufferFormat format,
        // If non-zero, mixes filtered and unfiltered buffers
        const float filtered_mix_amount);

    // -- from IFrameBufferReader
    virtual bool GetPixelLine(const unsigned int y, const unsigned int x_start, const unsigned int num_pixels, BMM_Color_fl* const target_pixels) override;
    virtual IPoint2 GetResolution() const override;
    virtual Box2 GetRegion() const override;

private:

    FrameBufferReader& operator=(const FrameBufferReader&);

private:

    const void* const m_unfiltered_pixels;
    const void* const m_filtered_pixels;
    const int m_width;
    const int m_height;
    const Box2 m_region;
    const rti::EFramebufferFormat m_format;
    const float m_filtered_mix_amount;
};

}}  // namespace