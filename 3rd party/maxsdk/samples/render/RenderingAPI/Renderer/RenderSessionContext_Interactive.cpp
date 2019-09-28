//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "RenderSessionContext_Interactive.h"

#include <RenderingAPI/Renderer/UnifiedRenderer.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

RenderSessionContext_Interactive::RenderSessionContext_Interactive(
    UnifiedRenderer& renderer, 
    IIRenderMgr* const irender_manager,
    const IRenderMessageManager::MessageSource initial_message_source,
    RendProgressCallback* const progress_callback,
    IIRenderMgr* pIIRenderMgr,
    const Box2& region,
    Bitmap* const bitmap,
    INode* const camera_node,
    const bool use_camera,
    const ViewExp* view_exp,
    INode* const root_node,
    const DefaultLight* const default_lights, const size_t num_default_lights)

    : RenderSessionContext_Base(renderer, initial_message_source, progress_callback),
    m_render_settings_container(renderer, GetNotificationClient()),
    m_camera_container(renderer, m_render_settings_container, GetNotificationClient(), pIIRenderMgr, region, bitmap, camera_node, use_camera, view_exp),
    m_scene_container(GetNotificationClient(), m_render_settings_container, root_node),
    m_irender_manager(irender_manager)
{
    // Initialize context
    set_default_lights(default_lights, num_default_lights);

    // Initialize the environment parameters
    setup_activeshade_environment();

    // Monitor the environment parameters
    if(GetNotificationClient() != nullptr)
    {
        GetNotificationClient()->MonitorRenderEnvironment(~size_t(0), *this, nullptr);
    }

    // Let the base class monitor its dependencies
    monitor_dependencies();
}

RenderSessionContext_Interactive::~RenderSessionContext_Interactive()
{
    // Let the base class stop monitoring  its dependencies
    stop_monitoring_dependencies();

    // Stop monitoring the environment parameters 
    if(GetNotificationClient() != nullptr)
    {
        GetNotificationClient()->StopMonitoringRenderEnvironment(~size_t(0), *this, nullptr);
    }
}

const ISceneContainer& RenderSessionContext_Interactive::GetScene() const
{
    return m_scene_container;
}

const ICameraContainer& RenderSessionContext_Interactive::GetCamera() const
{
    return m_camera_container;
}

const IRenderSettingsContainer& RenderSessionContext_Interactive::GetRenderSettings() const
{
    return m_render_settings_container;
}

SceneContainer_Interactive& RenderSessionContext_Interactive::get_scene_container()
{
    return m_scene_container;
}

const SceneContainer_Interactive& RenderSessionContext_Interactive::get_scene_container() const
{
    return m_scene_container;
}

CameraContainer_Interactive& RenderSessionContext_Interactive::get_camera_container()
{
    return m_camera_container;
}

const CameraContainer_Interactive& RenderSessionContext_Interactive::get_camera_container() const
{
    return m_camera_container;
}

RenderSettingsContainer_Interactive& RenderSessionContext_Interactive::get_render_settings_container()
{
    return m_render_settings_container;
}

const RenderSettingsContainer_Interactive& RenderSessionContext_Interactive::get_render_settings_container() const
{
    return m_render_settings_container;
}

void RenderSessionContext_Interactive::set_irender_manager(IIRenderMgr* const irender_manager)
{
    m_irender_manager = irender_manager;
    m_camera_container.set_irender_mgr(irender_manager);
}

IIRenderMgr* RenderSessionContext_Interactive::get_irender_manager() const
{
    return m_irender_manager;
}

CameraContainer_Base& RenderSessionContext_Interactive::get_camera_container_base()
{
    return m_camera_container;
}

std::vector<IRenderElement*> RenderSessionContext_Interactive::GetRenderElements() const 
{
    // Not supported in ActiveShade, for now.
    return std::vector<IRenderElement*>();
}

std::vector<Atmospheric*> RenderSessionContext_Interactive::GetAtmospherics() const 
{
    // Not supported in ActiveShade, for now.
    return std::vector<Atmospheric*>();
}

Effect* RenderSessionContext_Interactive::GetEffect() const 
{
    // Not supported in ActiveShade, for now.
    return nullptr;
}

void RenderSessionContext_Interactive::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData)
{
    switch(genericEvent.GetNotifierType())
    {
    case NotifierType_RenderEnvironment:
        // Environment changed: set it up again
        setup_activeshade_environment();
        break;
    default:
        break;
    }

    __super::NotificationCallback_NotifyEvent(genericEvent, userData);
}

void RenderSessionContext_Interactive::setup_activeshade_environment()
{
    // Setup the environment using the global settings
    Texmap* const env_tex = GetCOREInterface13()->GetUseEnvironmentMap() ? GetCOREInterface13()->GetEnvironmentMap() : nullptr;
    //!! FIXME: we won't support animation this way, since we're not propagating the validity interval to the environment object.
    // Should we care about this, or is it an acceptable limitation until we have full support for the new environment object (which will solve
    // this problem intrinsically)?
    Interval env_col_validity = FOREVER;
    const Color env_col = GetCOREInterface13()->GetBackGround(GetCOREInterface()->GetTime(), env_col_validity);

    set_legacy_environment(env_tex, env_col);
}

}}	// namespace 

