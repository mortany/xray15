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

// local 
#include "SceneContainer_Interactive.h"
#include "CameraContainer_Interactive.h"
#include "RenderSettingsContainer_Interactive.h"
#include "RenderingLogger.h"
#include "RenderingProgress.h"
#include "RenderSessionContext_Base.h"

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;

// The interactive (active shade) version of IRenderSessionContext
class RenderSessionContext_Interactive :
    public RenderSessionContext_Base
{
public:

    RenderSessionContext_Interactive(
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
        const DefaultLight* const default_lights, const size_t num_default_lights);
    ~RenderSessionContext_Interactive();

    void set_irender_manager(IIRenderMgr* const irender_manager);

    SceneContainer_Interactive& get_scene_container();
    const SceneContainer_Interactive& get_scene_container() const;
    CameraContainer_Interactive& get_camera_container();
    const CameraContainer_Interactive& get_camera_container() const;
    RenderSettingsContainer_Interactive& get_render_settings_container();
    const RenderSettingsContainer_Interactive& get_render_settings_container() const;

    // -- inherited from IRenderSessionContext
    virtual const ISceneContainer& GetScene() const override;
    virtual const ICameraContainer& GetCamera() const override;
    virtual const IRenderSettingsContainer& GetRenderSettings() const override;
    virtual std::vector<IRenderElement*> GetRenderElements() const override;
    virtual std::vector<Atmospheric*> GetAtmospherics() const override;
    virtual Effect* GetEffect() const override;

    // -- inherited from RenderSessionContext_Base
    virtual IIRenderMgr* get_irender_manager() const;

protected:

    // -- inherited from RenderSessionContext_Base
    virtual CameraContainer_Base& get_camera_container_base() override;

private:

    // Sets up the active shade environment
    void setup_activeshade_environment();

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) override;

private:

    // Must be declared FIRST as it's passed to the constructor of other members
    RenderSettingsContainer_Interactive m_render_settings_container;

    CameraContainer_Interactive m_camera_container;
    SceneContainer_Interactive m_scene_container;

    // The interactive rendering manager currently being used
    IIRenderMgr* m_irender_manager;
};

}}	// namespace 

