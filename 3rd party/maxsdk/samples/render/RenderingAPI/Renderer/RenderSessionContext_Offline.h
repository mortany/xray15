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
#include "SceneContainer_Offline.h"
#include "CameraContainer_Offline.h"
#include "RenderSettingsContainer_Offline.h"
#include "RenderingLogger.h"
#include "RenderingProgress.h"
#include "RenderSessionContext_Base.h"

// std
#include <memory>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

// The offline (production) version of IRenderSessionContext
class RenderSessionContext_Offline :
    public RenderSessionContext_Base
{
public:

    RenderSessionContext_Offline(
        UnifiedRenderer& renderer, 
        INode* const root_node,
        const DefaultLight* const default_lights, const size_t num_default_lights,
        INode* const camera_node,
        const ViewParams* view_params,
        RendParams& rend_params);
    ~RenderSessionContext_Offline();

    // Re-initializes the context for a new render operation (these are the parameters passed to Renderer::Open())
    void reinitialize(
        INode* const root_node,
        const DefaultLight* const default_lights, const size_t num_default_lights,
        INode* const camera_node,
        const ViewParams* view_params,
        RendParams& rend_params);
    
    // Sets up parameters specific to each frame
    void setup_frame_params(Bitmap* const bitmap, const RendParams& rend_params, const FrameRendParams& frame_rend_params);

    SceneContainer_Offline& get_scene_container();
    const SceneContainer_Offline& get_scene_container() const;
    CameraContainer_Offline& get_camera_container();
    const CameraContainer_Offline& get_camera_container() const;
    RenderSettingsContainer_Offline& get_render_settings_container();
    const RenderSettingsContainer_Offline& get_render_settings_container() const;

    // -- inherited from IRenderSessionContext
    virtual const ISceneContainer& GetScene() const override;
    virtual const ICameraContainer& GetCamera() const override;
    virtual const IRenderSettingsContainer& GetRenderSettings() const override;
    virtual std::vector<IRenderElement*> GetRenderElements() const override;
    virtual std::vector<Atmospheric*> GetAtmospherics() const override;
    virtual Effect* GetEffect() const override;

    // -- inherited from IRenderSessionContext
    virtual IIRenderMgr* get_irender_manager() const;

protected:

    // -- inherited from RenderSessionContext_Base
    virtual CameraContainer_Base& get_camera_container_base() override;

private:

    // Must be declared FIRST as it's passed to the constructor of other members
    RenderSettingsContainer_Offline m_render_settings_container;

    SceneContainer_Offline m_scene_container;
    CameraContainer_Offline m_camera_container;

    // Render elements, effects, etc. from RendParams.
    IRenderElementMgr* m_render_element_manager;
    Atmospheric* m_atmospheric;
    Effect* m_effect;

};

}}	// namespace 

