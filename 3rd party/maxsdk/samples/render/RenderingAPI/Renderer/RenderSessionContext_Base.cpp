//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "RenderSessionContext_Offline.h"

// Local
#include "EnvironmentContainer.h"
#include "../Translator/TranslationManager.h"
#include "../Translator/Helpers/NodeInstancingPool.h"
#include "Utils.h"

// max sdk
#include <bitmap.h>
#include <interactiverender.h>
#include <Rendering/CommonRendererUI.h>
#include <RenderingAPI/Renderer/IEnvironmentContainer.h>

#include "3dsmax_banned.h"

#undef min      // pif
#undef max      // paf

namespace
{
    class MissingFileAccumulator : public AssetEnumCallback
    {
    public:

        const std::set<MSTR> get_missing_files() const
        {
            return m_missing_files;
        }

        // -- inherited from AssetEnumCallback
        virtual void RecordAsset(const MaxSDK::AssetManagement::AssetUser& asset)
        {
            m_missing_files.insert(asset.GetFileName());
        }

    private:

        std::set<MSTR> m_missing_files;
    };
}

namespace Max
{;
namespace RenderingAPI
{;

RenderSessionContext_Base::RenderSessionContext_Base(
    const UnifiedRenderer& renderer,
    const IRenderMessageManager::MessageSource initial_message_source, 
    RendProgressCallback* const progress_callback)

    : m_immediate_notification_client(INotificationManager::GetManager()->RegisterNewImmediateClient()),
    m_immediate_interactive_rendering_client((m_immediate_notification_client != nullptr) ? IInteractiveRenderingManager::GetManager()->RegisterNewImmediateClient(m_immediate_notification_client.get()) : nullptr),
    m_rendering_logger(initial_message_source),
    m_rendering_process(m_rendering_logger, progress_callback),
    m_renderer(renderer)
{
    DbgAssert((m_immediate_notification_client != nullptr) && (m_immediate_interactive_rendering_client != nullptr));
}

RenderSessionContext_Base::~RenderSessionContext_Base()
{
    // RenderEnd() should have been called already
    DbgAssert(m_render_begin_callees.empty());

    // All notifiers should have unregistered themselves
    DbgAssert(m_change_notifiers.empty());

    // Stop monitoring the environment
    // could call stop_monitoring_dependencies(), but then get DbgAssert under UnregisterChangeNotifier call 
    // since monitor_dependencies() has not been called or stop_monitoring_dependencies() has already been called
    IEnvironmentContainer* env = dynamic_cast<IEnvironmentContainer*>(m_env_object_ref_holder.GetRef());
    if (env != nullptr)
    {
        IImmediateInteractiveRenderingClient* const notification_client = GetNotificationClient();
        if (notification_client != nullptr)
        {
            notification_client->StopMonitoringReferenceTarget(*env, ~size_t(0), *this, nullptr);
        }
     }
}

void RenderSessionContext_Base::monitor_dependencies()
{
    // Watch for scene changes, in case default lights should be enabled or disabled
    GetScene().RegisterChangeNotifier(*this);
}

void RenderSessionContext_Base::stop_monitoring_dependencies()
{
    // Unregister for notifications from the scene
    GetScene().UnregisterChangeNotifier(*this);

    // Stop monitoring the environment
    IEnvironmentContainer* env = dynamic_cast<IEnvironmentContainer*>(m_env_object_ref_holder.GetRef());
    if(env != nullptr)
    {
        IImmediateInteractiveRenderingClient* const notification_client = GetNotificationClient();
        if(notification_client != nullptr)
        {
            notification_client->StopMonitoringReferenceTarget(*env, ~size_t(0), *this, nullptr);
        }
    }
}

RenderingLogger& RenderSessionContext_Base::get_logger()
{
    return m_rendering_logger;
}

RenderingProgress& RenderSessionContext_Base::get_progress()
{
    return m_rendering_process;
}

void RenderSessionContext_Base::set_render_session(const std::shared_ptr<IRenderSession>& render_session)
{
    m_render_session = render_session;
}

IRenderSession* RenderSessionContext_Base::GetRenderSession() const 
{
    return m_render_session.lock().get();
}

IFrameBufferProcessor& RenderSessionContext_Base::GetMainFrameBufferProcessor() 
{
    return *this;
}

void RenderSessionContext_Base::UpdateBitmapDisplay() 
{
    // Report the bitmap update to the progress reporting interface
    m_rendering_process.report_bitmap_update();

    // Fetch the region to be updated
    const Box2 region = get_camera_container_base().get_region_ignoring_down_res_factor();

    // Update the bitmap from the main thread
    UpdateBitmap_MainThreadJob main_thread_job(&region, *this);
    if(!m_rendering_process.RunJobFromMainThread(main_thread_job))
    {
        // Abort requested - but don't care. The bitmap won't update, but that's OK - we'll let the render abort normally after this.
    }
}

Bitmap* RenderSessionContext_Base::get_frame_buffer_bitmap() 
{
    return GetCamera().GetBitmap();
}

IRenderSessionContext& RenderSessionContext_Base::get_render_session_context() 
{
    return *this;
}

IRenderingProcess& RenderSessionContext_Base::GetRenderingProcess() const
{
    return m_rendering_process;
}

IRenderingLogger& RenderSessionContext_Base::GetLogger() const
{
    return m_rendering_logger;
}

void RenderSessionContext_Base::CallRenderBegin(ReferenceMaker& refMaker, const TimeValue t) 
{
    // Disable notification events before calling RenderBegin() and RenderEnd().
    // Some plugins send a notification from RenderBegin() and RenderEnd(), to force the viewport to update. This notification is undesirable
    // for ActiveShade rendering as it causes an endless loop of invalidation/re-translation.
    IImmediateInteractiveRenderingClient* notification_client = GetNotificationClient();
    const bool re_enable_notifications = (notification_client != nullptr) && notification_client->NotificationsEnabled();
    if(notification_client != nullptr)
    {
        notification_client->EnableNotifications(false);
    }

    // Call RenderBegin()
    RenderBeginCaller caller(t, m_render_begin_callees, GetRenderSettings().GetIsMEditRender());
    caller.call_render_begin(refMaker);

    // Re-enable notifications
    if(re_enable_notifications && (notification_client != nullptr))
    {
        notification_client->EnableNotifications(true);
    }
}

void RenderSessionContext_Base::CallRenderEnd(const TimeValue t)
{
    // Disable notification events before calling RenderBegin() and RenderEnd().
    // Some plugins send a notification from RenderBegin() and RenderEnd(), to force the viewport to update. This notification is undesirable
    // for ActiveShade rendering as it causes an endless loop of invalidation/re-translation.
    IImmediateInteractiveRenderingClient* notification_client = GetNotificationClient();
    const bool re_enable_notifications = (notification_client != nullptr) && notification_client->NotificationsEnabled();
    if(notification_client != nullptr)
    {
        notification_client->EnableNotifications(false);
    }

    // Call RenderEnd()
    for(const AnimHandle anim_handle : m_render_begin_callees)
    {
        Animatable* const anim = Animatable::GetAnimByHandle(anim_handle);
        if(anim != nullptr)
        {
            anim->RenderEnd(t);
        }
    }
    m_render_begin_callees.clear();

    // Re-enable notifications
    if(re_enable_notifications && (notification_client != nullptr))
    {
        notification_client->EnableNotifications(true);
    }
}

TranslationHelpers::INodeInstancingPool::IManager& RenderSessionContext_Base::GetNodeInstancingManager() const
{
    // Allocate on first call
    if(m_node_instancing_manager == nullptr)
    {
        m_node_instancing_manager = std::unique_ptr<TranslationHelpers::INodeInstancingPool::IManager>(
            new Max::RenderingAPI::TranslationHelpers::NodeInstancingPool::Manager(GetNotificationClient()));
    }

    return *m_node_instancing_manager;
}

ITranslationManager& RenderSessionContext_Base::GetTranslationManager() const 
{
    // Allocate on first call
    if(m_translation_manager == nullptr)
    {
        m_translation_manager= std::unique_ptr<ITranslationManager>(new TranslationManager(const_cast<RenderSessionContext_Base&>(*this)));
    }

    return *m_translation_manager;
}

IPoint2 RenderSessionContext_Base::SetDownResolutionFactor(const unsigned int factor) 
{
    // Set the down-res factor from the main thread
    DownResFactor_MainThreadJob main_thread_job(factor, *this);
    if(!m_rendering_process.RunJobFromMainThread(main_thread_job))
    {
        // Abort requested - but don't care. We'll let the render abort normally after this.
        return IPoint2(1, 1);
    }
    else
    {
        return main_thread_job.get_full_camera_resolution();
    }
}

IEnvironmentContainer* RenderSessionContext_Base::GetEnvironment() const 
{
    // Instantiate the standard environment object, if necessary
    IEnvironmentContainer* env = dynamic_cast<IEnvironmentContainer*>(m_env_object_ref_holder.GetRef());
    DbgAssert(env == m_env_object_ref_holder.GetRef());
    return env;
}

void RenderSessionContext_Base::set_legacy_environment(Texmap* const env_tex, const Color background_color) const
{
    // Create the environment container if needed
    EnvironmentContainer* env = dynamic_cast<EnvironmentContainer*>(m_env_object_ref_holder.GetRef());
    if(env == nullptr)
    {
        env = new EnvironmentContainer(false);
        if(DbgVerify(env != nullptr))
        {
            m_env_object_ref_holder.SetRef(env);

            // Monitor the environment
            IImmediateInteractiveRenderingClient* const notification_client = GetNotificationClient();
            if(notification_client != nullptr)
            {
                notification_client->MonitorReferenceTarget(*env, ~size_t(0), const_cast<RenderSessionContext_Base&>(*this), nullptr);
            }
        }
    }

    // Setup the environment using the legacy parameters
    if(DbgVerify(env != nullptr))
    {
        env->SetLegacyEnvironment(env_tex, background_color);
    }
}

bool RenderSessionContext_Base::do_missing_maps_dialog(const TimeValue t, const HWND parent_hwnd) const
{
    // No missing maps dialog for MEdit
    if(!GetRenderSettings().GetIsMEditRender())
    {
        Interval dummy_interval;

        // Get the list of scene nodes
        const ISceneContainer& scene_container = GetScene();
        const std::vector<INode*> geom_nodes = scene_container.GetGeometricNodes(t, dummy_interval);
        const std::vector<INode*> light_nodes = scene_container.GetLightNodes(t, dummy_interval);

        // Clear the enumeration flag on all the references on which we'll call EnumAuxFiles()
        ClearAFlagInAllAnimatables(A_WORK1); // this flag is used by EnumAuxFiles

        // Call EnumAuxFiles() on all the references used by the renderer
        MissingFileAccumulator missing_files_accumulator;
        {
            const DWORD enum_flags = (FILE_ENUM_MISSING_ONLY|FILE_ENUM_1STSUB_MISSING|FILE_ENUM_CHECK_AWORK1|FILE_ENUM_SKIP_VPRENDER_ONLY|FILE_ENUM_RENDER);
            for(INode* const node : geom_nodes)
            {
                node->EnumAuxFiles(missing_files_accumulator, enum_flags);
            }
            for(INode* const node : light_nodes)
            {
                node->EnumAuxFiles(missing_files_accumulator, enum_flags);
            }

            IEnvironmentContainer* const env = GetEnvironment();
            if(env != nullptr)
            {
                env->EnumAuxFiles(missing_files_accumulator, enum_flags);
            }
        }

        // Show the missing files dialog, if necessary
        const std::set<MSTR>& missing_files = missing_files_accumulator.get_missing_files();
        if(!missing_files.empty())
        {
            // Construct a NameTab
            NameTab name_tab;
            for(const MSTR& file_name : missing_files)
            {
                name_tab.AddName(file_name.data());
            }

            return CommonRendererUI::DoMissingMapsDialog(name_tab, parent_hwnd);
        }
        else
        {
            // No missing files: no dialog to show
            return true;
        }
    }
    else
    {
        // No missing maps dialog for MEdit
        return true;
    }
}

void RenderSessionContext_Base::set_default_lights(const DefaultLight* const default_lights, const size_t num_default_lights)
{
    if(default_lights != nullptr)
    {
        m_default_lights.resize(num_default_lights);
        for(size_t i = 0; i < num_default_lights; ++i)
        {
            m_default_lights[i] = default_lights[i];
        }
    }
    else
    {
        DbgAssert(num_default_lights == 0);
        m_default_lights.clear();
    }

    for(IChangeNotifier* const notifier : m_change_notifiers)
    {
        if(notifier != nullptr)
        {
            notifier->NotifyDefaultLightingMaybeChanged();
        }
    }
}

std::vector<DefaultLight> RenderSessionContext_Base::GetDefaultLights() const 
{
    return m_default_lights;
}

bool RenderSessionContext_Base::GetDefaultLightingEnabled(const bool considerEnvironmentLighting, const TimeValue t, Interval& validity) const 
{
    // Enable default lights iff there are no lights in the scene, nor any environment map or color
    bool is_environment_present = false;
    if(considerEnvironmentLighting)
    {
        IEnvironmentContainer* const env = dynamic_cast<IEnvironmentContainer*>(GetEnvironment());
        if(env != nullptr)
        {
            switch(env->GetEnvironmentMode())
            {
            case IEnvironmentContainer::EnvironmentMode::None:
                is_environment_present = false;
                break;
            case IEnvironmentContainer::EnvironmentMode::Color:
                {
                    const AColor env_color = env->GetEnvironmentColor(t, validity);
                    is_environment_present = (env_color.r != 0.0f) || (env_color.g != 0.0f) || (env_color.b != 0.0f);
                }
                break;
            case IEnvironmentContainer::EnvironmentMode::Texture:
                is_environment_present = (env->GetEnvironmentTexture() != nullptr);
                break;
            }
        }
    }

    // In MEdit, we enable default lights even if an environment is present
    if(!is_environment_present || GetRenderSettings().GetIsMEditRender())
    {
        // Check if there are any lights in the scene
        const ISceneContainer& scene_container = GetScene();
        const std::vector<INode*> light_nodes = scene_container.GetLightNodes(t, validity);
        if(light_nodes.empty())
        {
            return true;
        }
    }

    return false;
}

void RenderSessionContext_Base::NotifySceneNodesChanged() 
{
    // Default lighting may have changed, as it's based on the presence of lights in the scene
    for(IChangeNotifier* const notifier : m_change_notifiers)
    {
        if(notifier != nullptr)
        {
            notifier->NotifyDefaultLightingMaybeChanged();
        }
    }
}

void RenderSessionContext_Base::NotifySceneBoundingBoxChanged() 
{
    // don't care
}

void RenderSessionContext_Base::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/) 
{
    const IReferenceTargetEvent* ref_targ_event = dynamic_cast<const IReferenceTargetEvent*>(&genericEvent);
    if((ref_targ_event != nullptr) && (ref_targ_event->GetReferenceTarget() == m_env_object_ref_holder.GetRef()))
    {
        // The environment has changed, which may cause the default lighting situation to change as well
        for(IChangeNotifier* const notifier : m_change_notifiers)
        {
            if(notifier != nullptr)
            {
                notifier->NotifyDefaultLightingMaybeChanged();
            }
        }
    }
}

MotionBlurSettings RenderSessionContext_Base::GetMotionBlurSettingsForNode(INode& node, const TimeValue t, Interval& validity, const MotionBlurSettings* global_motion_blur_settings) const 
{
    return Utils::ApplyMotionBlurSettingsFromNode(
        node,
        t,
        validity,
        (global_motion_blur_settings != nullptr) ? *global_motion_blur_settings : GetCamera().GetGlobalMotionBlurSettings(t, validity),
        m_renderer);
}

MotionTransforms RenderSessionContext_Base::EvaluateMotionTransforms(INode& node, const TimeValue t, Interval& validity, const MotionBlurSettings* node_motion_blur_settings) const
{
    return Utils::EvaluateMotionTransformsForNode(
        node,
        t,
        validity,
        (node_motion_blur_settings != nullptr) ? *node_motion_blur_settings : GetMotionBlurSettingsForNode(node, t, validity, nullptr));
}

void RenderSessionContext_Base::RegisterChangeNotifier(IChangeNotifier& notifier) const 
{
    m_change_notifiers.push_back(&notifier);
}

void RenderSessionContext_Base::UnregisterChangeNotifier(IChangeNotifier& notifier) const 
{
    for(auto it = m_change_notifiers.begin(); it != m_change_notifiers.end(); ++it)
    {
        if(&notifier == *it)
        {
            m_change_notifiers.erase(it);
            return;
        }
    }

    // Shouldn't get here
    DbgAssert(false);
}

//==================================================================================================
// class RenderSessionContext_Base::UpdateBitmap_MainThreadJob
//==================================================================================================

RenderSessionContext_Base::UpdateBitmap_MainThreadJob::UpdateBitmap_MainThreadJob(
    const Box2* const update_region, 
    const RenderSessionContext_Base& render_session_context)
    : m_update_region((update_region != nullptr) ? *update_region : Box2()),        // Keep a copy of the region, since it'd be unsafe to hold a pointer to the region allocated by the caller
    m_update_region_provided(update_region != nullptr),
    m_render_session_context(render_session_context)
{

}

RenderSessionContext_Base::UpdateBitmap_MainThreadJob::~UpdateBitmap_MainThreadJob()
{

}

void RenderSessionContext_Base::UpdateBitmap_MainThreadJob::ExecuteMainThreadJob()
{
    Bitmap* const bitmap = m_render_session_context.GetCamera().GetBitmap();

    // Update the bitmap display
    IIRenderMgr* const irender_manager = m_render_session_context.get_irender_manager();
    DbgAssert((irender_manager != nullptr) || (bitmap!= nullptr));
    if(irender_manager != nullptr)
    {
        // Update active shade window
        irender_manager->UpdateDisplay();
    }
    if(bitmap != nullptr)
    {
        // Update offline rendering VFB
        bitmap->RefreshWindow(m_update_region_provided ? const_cast<Box2*>(&m_update_region) : nullptr);
    }

    // To ensure the immediate refresh of the bitmap window, we need to process the pending window messages 
    // (as Bitmap::RefreshWindow() merely invalidates the bitmap window, posting a message on the queue).
    // An artificial/hacky way of doing this is checking for abort.    
    m_render_session_context.GetRenderingProcess().HasAbortBeenRequested();
}

//==================================================================================================
// class RenderSessionContext_Base::DownResFactor_MainThreadJob
//==================================================================================================

RenderSessionContext_Base::DownResFactor_MainThreadJob::DownResFactor_MainThreadJob(const unsigned int down_res_factor, RenderSessionContext_Base& render_session_context)
    : m_down_res_factor(down_res_factor),
    m_render_session_context(render_session_context)
{

}

RenderSessionContext_Base::DownResFactor_MainThreadJob::~DownResFactor_MainThreadJob()
{

}

void RenderSessionContext_Base::DownResFactor_MainThreadJob::ExecuteMainThreadJob()
{
    CameraContainer_Base& camera_container = m_render_session_context.get_camera_container_base();
    camera_container.set_down_res_factor(m_down_res_factor);
    
    m_full_camera_resolution = camera_container.get_resolution_ignoring_down_res_factor();
}

IPoint2 RenderSessionContext_Base::DownResFactor_MainThreadJob::get_full_camera_resolution() const
{
    return m_full_camera_resolution;
}

//==================================================================================================
// class RenderSessionContext_Base::RenderBeginCaller
//==================================================================================================

RenderSessionContext_Base::RenderBeginCaller::RenderBeginCaller(
    const TimeValue t,
    std::set<AnimHandle>& render_begin_callees,
    const bool is_material_editor)

    : m_time(t),
    m_render_begin_callees(render_begin_callees),
    m_is_material_editor(is_material_editor)
{

}

void RenderSessionContext_Base::RenderBeginCaller::call_render_begin(ReferenceMaker& ref_maker)
{
    // Set the 'preventDuplicatesViaFlag' argument to false, as having it true would result in the visited flag being reset on every animatable
    // in the system at every call to EnumRefHierarchy, turning this process into an O(N^2), very slow on scenes with large numbers of objects.
    ref_maker.EnumRefHierarchy(*this, true, true, true, false);
}

int RenderSessionContext_Base::RenderBeginCaller::proc(ReferenceMaker *rm) 
{
    if(rm != nullptr)
    {
        const AnimHandle anim_handle = Animatable::GetHandleByAnim(rm);

        // Check whether we've already called RenderBegin() on this object
        const bool inserted = m_render_begin_callees.insert(anim_handle).second;
        if(inserted)
        {
            rm->RenderBegin(m_time, m_is_material_editor ? RENDERBEGIN_IN_MEDIT : 0);
            return DEP_ENUM_CONTINUE;
        }
        else
        {
            // This reference gets ignored: Skip this part of the reference tree
            return DEP_ENUM_SKIP;
        }
    }
    else
    {
        return DEP_ENUM_CONTINUE;
    }
}

IImmediateInteractiveRenderingClient* RenderSessionContext_Base::GetNotificationClient() const
{
    return m_immediate_interactive_rendering_client.get();
}

//==================================================================================================
// class RenderSessionContext_Base::NotificationClientDeleter
//==================================================================================================

void RenderSessionContext_Base::NotificationClientDeleter::operator()(IImmediateNotificationClient* const client)
{
    INotificationManager::GetManager()->RemoveClient(client);
}

void RenderSessionContext_Base::NotificationClientDeleter::operator()(IImmediateInteractiveRenderingClient* const client)
{
    IInteractiveRenderingManager::GetManager()->RemoveClient(client);
}

}}	// namespace 
