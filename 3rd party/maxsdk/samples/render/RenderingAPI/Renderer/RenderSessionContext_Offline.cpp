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

#include <RenderingAPI/Renderer/UnifiedRenderer.h>
#include <Rendering/IAtmosphericContainer.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

RenderSessionContext_Offline::RenderSessionContext_Offline(
    UnifiedRenderer& renderer, 
    INode* const root_node,
    const DefaultLight* const default_lights, const size_t num_default_lights,
    INode* const camera_node,
    const ViewParams* view_params,
    RendParams& rend_params)

    : RenderSessionContext_Base(renderer, rend_params.inMtlEdit ? IRenderMessageManager::kSource_MEditRenderer : IRenderMessageManager::kSource_ProductionRenderer, nullptr),
    m_render_settings_container(renderer, GetNotificationClient()),
    m_scene_container(m_render_settings_container, GetNotificationClient()),
    m_camera_container(renderer, m_render_settings_container),
    m_render_element_manager(nullptr),
    m_atmospheric(nullptr),
    m_effect(nullptr)
{
    reinitialize(root_node, default_lights, num_default_lights, camera_node, view_params, rend_params);
}

RenderSessionContext_Offline::~RenderSessionContext_Offline()
{

}

void RenderSessionContext_Offline::reinitialize(
    INode* const root_node,
    const DefaultLight* const default_lights, const size_t num_default_lights,
    INode* const camera_node,
    const ViewParams* view_params,
    RendParams& rend_params)
{
    get_logger().set_message_source(rend_params.inMtlEdit ? IRenderMessageManager::kSource_MEditRenderer : IRenderMessageManager::kSource_ProductionRenderer);

    m_scene_container.set_scene_root_node(root_node);
    set_default_lights(default_lights, num_default_lights);
    m_camera_container.set_camera_node(camera_node);
    m_camera_container.set_use_camera(true);
    if(view_params != nullptr)
    {
        m_camera_container.set_view(*view_params);
   }
    m_render_settings_container.set_rend_params(rend_params);
    m_render_element_manager = rend_params.GetRenderElementMgr();
    m_atmospheric = rend_params.atmos;
    m_effect = rend_params.effect;

    // Call RenderBegin on the render elements
    const std::vector<IRenderElement*> elements = GetRenderElements();
    for(IRenderElement* const element : elements)
    {
        if(element != nullptr)
        {
            CallRenderBegin(*element, rend_params.firstFrame);
        }
    }

    // Call render begin on various things
    if(rend_params.atmos != nullptr)
        CallRenderBegin(*rend_params.atmos, rend_params.firstFrame);
    if(rend_params.envMap != nullptr)
        CallRenderBegin(*rend_params.envMap, rend_params.firstFrame);
    if(rend_params.effect != nullptr)
        CallRenderBegin(*rend_params.effect, rend_params.firstFrame);
}

const ISceneContainer& RenderSessionContext_Offline::GetScene() const
{
    return m_scene_container;
}

const ICameraContainer& RenderSessionContext_Offline::GetCamera() const
{
    return m_camera_container;
}

const IRenderSettingsContainer& RenderSessionContext_Offline::GetRenderSettings() const
{
    return m_render_settings_container;
}

SceneContainer_Offline& RenderSessionContext_Offline::get_scene_container()
{
    return m_scene_container;
}

const SceneContainer_Offline& RenderSessionContext_Offline::get_scene_container() const
{
    return m_scene_container;
}

CameraContainer_Offline& RenderSessionContext_Offline::get_camera_container()
{
    return m_camera_container;
}

const CameraContainer_Offline& RenderSessionContext_Offline::get_camera_container() const
{
    return m_camera_container;
}

RenderSettingsContainer_Offline& RenderSessionContext_Offline::get_render_settings_container()
{
    return m_render_settings_container;
}

const RenderSettingsContainer_Offline& RenderSessionContext_Offline::get_render_settings_container() const
{
    return m_render_settings_container;
}

IIRenderMgr* RenderSessionContext_Offline::get_irender_manager() const
{
    // No interactive rendering managed for offline rendering
    return nullptr;
}

CameraContainer_Base& RenderSessionContext_Offline::get_camera_container_base()
{
    return m_camera_container;
}

std::vector<IRenderElement*> RenderSessionContext_Offline::GetRenderElements() const 
{
    // Extract the set of render elements from the manager
    std::vector<IRenderElement*> elements;
    if((m_render_element_manager != nullptr) && m_render_element_manager->GetElementsActive())
    {
        const int num_render_elements = m_render_element_manager->NumRenderElements();
        for(int i = 0; i < num_render_elements; ++i)
        {
            IRenderElement* const render_element = m_render_element_manager->GetRenderElement(i);
            if((render_element != nullptr) && render_element->IsEnabled())
            {
                elements.push_back(render_element);
            }
        }
    }

    return elements;
}

std::vector<Atmospheric*> RenderSessionContext_Offline::GetAtmospherics() const 
{
    std::vector<Atmospheric*> atmospherics;
    if(m_atmospheric != nullptr)
    {
        // RendParams::atmos generally contains a pointer to an IAtmosphericContainer
        IAtmosphericContainer* const container = dynamic_cast<IAtmosphericContainer*>(m_atmospheric);
        if(container != nullptr)
        {
            const int num_atmospherics = container->NumAtmospheric();
            for(int i = 0; i < num_atmospherics; ++i)
            {
                Atmospheric* const atmospheric = container->GetAtmospheric(i);
                if(atmospheric != nullptr)
                {
                    atmospherics.push_back(atmospheric);
                }
            }
        }
        else
        {
            atmospherics.push_back(m_atmospheric);
        }
    }

    return atmospherics;
}

Effect* RenderSessionContext_Offline::GetEffect() const 
{
    return m_effect;
}

void RenderSessionContext_Offline::setup_frame_params(Bitmap* const bitmap, const RendParams& rend_params, const FrameRendParams& frame_rend_params)
{
    // Setup the camera's per-frame parameters
    m_camera_container.setup_frame_params(bitmap, rend_params, frame_rend_params);

    // Setup the environment's per-frame parameters
    set_legacy_environment(rend_params.envMap, frame_rend_params.background);

    m_render_element_manager = rend_params.GetRenderElementMgr();
    m_atmospheric = rend_params.atmos;
    m_effect = rend_params.effect;
}

}}	// namespace 

