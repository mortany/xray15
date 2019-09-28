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

// local includes
#include "FrameBufferProcessor.h"
// max sdk
#include <toneop.h>
#include <Noncopyable.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/UnifiedRenderer.h>
#include <NotificationAPI/InteractiveRenderingAPI_Subscription.h>
#include <NotificationAPI/NotificationAPI_Subscription.h>
// std
#include <memory>

class IIRenderMgr;

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;

// Common base class for both offline and interactive render session contexts
class RenderSessionContext_Base :
    public IRenderSessionContext,
    private FrameBufferProcessor,
    // To detect changes in default lighting
    protected ISceneContainer::IChangeNotifier,
    protected INotificationCallback
{
public:

    using IRenderSessionContext::IChangeNotifier;

    RenderSessionContext_Base(
        const UnifiedRenderer& m_renderer,
        const IRenderMessageManager::MessageSource initial_message_source, 
        RendProgressCallback* const progress_callback);
    ~RenderSessionContext_Base();

    RenderingLogger& get_logger();
    RenderingProgress& get_progress();

    void set_render_session(const std::shared_ptr<IRenderSession>& render_session);

    // Returns the interactive rendering manager, only relevant for active shade
    virtual IIRenderMgr* get_irender_manager() const = 0;

    // Sets up the internal environment texture with the given environment texture and background color.
    // This translates the legacy 3ds max background texture/color to the new environment system
    void set_legacy_environment(Texmap* const env_tex, const Color background_color) const;

    // Resets the default lights to the given set
    void set_default_lights(const DefaultLight* const default_lights, const size_t num_default_lights);
    
    // Checks the scene for missing files (textures and other support files), allowing the user to fix them by adding new search paths.
    // Returns false iff the user hits cancel, in which case the render operation should be canceled.
    bool do_missing_maps_dialog(const TimeValue t, const HWND parent_hwnd) const;

    // -- inherited from IRenderSessionContext
    virtual IEnvironmentContainer* GetEnvironment() const override;
    virtual IFrameBufferProcessor& GetMainFrameBufferProcessor() override;
    virtual void UpdateBitmapDisplay() override;
    virtual IRenderingProcess& GetRenderingProcess() const override;
    virtual IRenderingLogger& GetLogger() const override;
    virtual IRenderSession* GetRenderSession() const override;
    virtual void CallRenderBegin(ReferenceMaker& refMaker, const TimeValue t) override;
    virtual void CallRenderEnd(const TimeValue t) override;
    virtual TranslationHelpers::INodeInstancingPool::IManager& GetNodeInstancingManager() const override;
    virtual ITranslationManager& GetTranslationManager() const override;
    virtual IPoint2 SetDownResolutionFactor(const unsigned int factor) override;
    virtual bool GetDefaultLightingEnabled(const bool considerEnvironmentLighting, const TimeValue t, Interval& validity) const override;
    virtual std::vector<DefaultLight> GetDefaultLights() const override;
    virtual MotionTransforms EvaluateMotionTransforms(INode& node, const TimeValue t, Interval& validity, const MotionBlurSettings* node_motion_blur_settings) const override;
    virtual MotionBlurSettings GetMotionBlurSettingsForNode(INode& node, const TimeValue t, Interval& validity, const MotionBlurSettings* global_motion_blur_settings) const override;
    virtual void RegisterChangeNotifier(IChangeNotifier& notifier) const  override final;
    virtual void UnregisterChangeNotifier(IChangeNotifier& notifier) const  override final;
    virtual IImmediateInteractiveRenderingClient* GetNotificationClient() const override final;

    // -- inherited from FrameBufferProcessor
    virtual Bitmap* get_frame_buffer_bitmap() override;
    virtual IRenderSessionContext& get_render_session_context() override;

protected:

    // To be called by the subclass to allow this class to register change notifiers. This is needed to avoid problems with
    // calling virtual functions in the constructor and destructor
    void monitor_dependencies();
    void stop_monitoring_dependencies();

    // -- inherited from ISceneContainer::IChangeNotifier
    virtual void NotifySceneNodesChanged() override;
    virtual void NotifySceneBoundingBoxChanged() override;

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) override;

protected:

    const UnifiedRenderer& m_renderer;

    virtual CameraContainer_Base& get_camera_container_base() = 0;

private:

    class RenderBeginCaller;
    class FrameBufferProcessor;
    class UpdateBitmap_MainThreadJob;
    class DownResFactor_MainThreadJob;

    class NotificationClientDeleter
    {
    public:
        void operator()(IImmediateNotificationClient* const client);
        void operator()(IImmediateInteractiveRenderingClient* const client);
    };

    // The notification API clients, used to monitor scene. 
    // WARNING: These are declared FIRST to ensure they're deleted last (because they're used by many other things).
    std::unique_ptr<IImmediateNotificationClient, NotificationClientDeleter> m_immediate_notification_client;
    std::unique_ptr<IImmediateInteractiveRenderingClient, NotificationClientDeleter> m_immediate_interactive_rendering_client;

    // The handler for log messages
    mutable RenderingLogger m_rendering_logger;
    // The handler for progress reporting
    mutable RenderingProgress m_rendering_process;

    // The set of things on which RenderBegin() has been called. We use an AnimHandle to address mysterious crashes that resulted from certain
    // references being deleted between calling RenderBegin() and RendEnd().
    std::set<AnimHandle> m_render_begin_callees;

    // Lazily-allocated node instancing manager
    mutable std::unique_ptr<TranslationHelpers::INodeInstancingPool::IManager> m_node_instancing_manager;
    // Lazily-allocated translation manager
    mutable std::unique_ptr<ITranslationManager> m_translation_manager;

    // Weak pointer to the render session, which is ultimately owned by the renderer plugin
    std::weak_ptr<IRenderSession> m_render_session;

    // Holds the reference to the environment object, used temporary to emulate the upcoming environment object plugin class
    mutable SingleRefMaker m_env_object_ref_holder;

    // Stores the default lights currently being used
    std::vector<DefaultLight> m_default_lights;

    // The change notifiers, currently registered
    mutable std::vector<IChangeNotifier*> m_change_notifiers;
};

// This class is responsible for updating render output bitmap from the main thread.
class RenderSessionContext_Base::UpdateBitmap_MainThreadJob : 
    public IRenderingProcess::IMainThreadJob,
    public MaxSDK::Util::Noncopyable
{
public:

    UpdateBitmap_MainThreadJob(const Box2* const update_region, const RenderSessionContext_Base& render_session_context);
    virtual ~UpdateBitmap_MainThreadJob();

    // -- inherited from IMainThreadJob
    virtual void ExecuteMainThreadJob();

private:

    const Box2 m_update_region;
    const bool m_update_region_provided;
    const RenderSessionContext_Base& m_render_session_context;
};

// This class is responsible for setting the down-res factor from the main thread
class RenderSessionContext_Base::DownResFactor_MainThreadJob :
    public IRenderingProcess::IMainThreadJob,
    public MaxSDK::Util::Noncopyable
{
public:

    DownResFactor_MainThreadJob(const unsigned int down_res_factor, RenderSessionContext_Base& render_session_context);
    virtual ~DownResFactor_MainThreadJob();

    IPoint2 get_full_camera_resolution() const;

    // -- inherited from IMainThreadJob
    virtual void ExecuteMainThreadJob();

private:

    const unsigned int m_down_res_factor;
    RenderSessionContext_Base& m_render_session_context;
    IPoint2 m_full_camera_resolution;
};

//=================================================================================================
// class RenderSessionContext_Base::RenderBeginCaller
//
// Encapsulates the reference enumeration logic for calling RenderBegin()
class RenderSessionContext_Base::RenderBeginCaller :
    private RefEnumProc,
    private MaxSDK::Util::Noncopyable
{
public:

    RenderBeginCaller(
        // Time at which to call RenderBegin()
        const TimeValue t,
        // Lookup table used to avoid duplicate calls
        std::set<AnimHandle>& render_begin_callees,
        // Whether rendering for MEdit
        const bool is_material_editor);

    void call_render_begin(ReferenceMaker& ref_maker);

private:

    // -- inherited from RefEnumProc
    virtual int proc(ReferenceMaker *rm) override;

private:

    // Time at which to call RenderBegin()
    const TimeValue m_time;
    // Whether rendering for MEdit
    const bool m_is_material_editor;

    // Lookup table used to avoid duplicate calls
    std::set<AnimHandle>& m_render_begin_callees;
};

}}	// namespace 

