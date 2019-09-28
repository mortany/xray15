//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Object.h>

// Max sdk
#include <inode.h>
#include <Materials/Mtl.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;

using namespace NotificationAPI;
using namespace TranslationHelpers;

BaseTranslator_Object::BaseTranslator_Object(const std::shared_ptr<const INodeInstancingPool>& nodePool, TranslatorGraphNode& graphNode)
    : Translator(graphNode),
    m_node_pool(nodePool)
{
    // Monitor interactive changes
    if(m_node_pool != nullptr)
    {
        m_node_pool->MonitorNodePool(*this);
    }
}

BaseTranslator_Object::~BaseTranslator_Object()
{
    // Stop monitoring changes
    if(m_node_pool != nullptr)
    {
        m_node_pool->StopMonitoringNodePool(*this);
    }

    // Stop monitoring the material
    set_monitored_material(nullptr);
}


void BaseTranslator_Object::NotifyPoolInstancedObjectChanged(const INodeInstancingPool& /*node_pool*/, const NodeEventType event_type)
{
    switch(event_type)
    {
    case EventType_Node_Deleted:
        // Tell the graph node the translated object has been deleted.
        TranslatedObjectDeleted();
        break;
    case EventType_Node_Transform:
        // Don't care about transform changes when translating the object
        break;
    case EventType_Node_ParamBlock:
    case EventType_Node_Uncategorized:
        // Object parameters may have changed
        Invalidate();
        break;
    case EventType_Mesh_Vertices:
    case EventType_Mesh_Faces:
    case EventType_Mesh_UVs:
        // Mesh has changed
        Invalidate();
        break;
    case EventType_Node_Material_Replaced:
        // Material was replaced: we may need invalidation if the displacement properties of the material changed
        if(CareAboutMaterialDisplacement())
        {
            INode* const node = GetNode();
            Mtl* const new_material = (node != nullptr) ? node->GetMtl() : nullptr;
            const ULONG new_mtl_req = (new_material != nullptr) ? new_material->Requirements(-1) : 0;
            const bool new_mtl_has_displacement = ((new_mtl_req & MTLREQ_DISPLACEMAP) != 0);
            if(new_mtl_has_displacement || m_last_translation_has_material_displacement)
            {
                // Material has changed and displacement is involved: need to re-process displacement
                Invalidate();
            }
        }
        break;
    default:
        // Don't care about other notifications
        break;
    }

}

void BaseTranslator_Object::NotifyPoolContentMaybeChanged(const INodeInstancingPool& /*node_pool*/)
{
    // Don't care: the object being translated isn't affected
}

INode* BaseTranslator_Object::GetNode() const
{
    if(DbgVerify(m_node_pool != nullptr))
    {
        return m_node_pool->GetRepresentativeNode();
    }
    else
    {
        return nullptr;
    }
}

std::shared_ptr<const INodeInstancingPool> BaseTranslator_Object::GetNodePool() const
{
    return m_node_pool;
}

Interval BaseTranslator_Object::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const 
{
    return previous_validity;
}

void BaseTranslator_Object::PreTranslate(const TimeValue translationTime, Interval& /*validity*/) 
{
    // Call RenderBegin on the node
    INode* const node = GetNode();
    if(node != nullptr)
    {
        GetRenderSessionContext().CallRenderBegin(*node, translationTime);

        // Monitor the material in case the displacement properties change.
        set_monitored_material(node->GetMtl());
    }
}

void BaseTranslator_Object::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Nothing to do
}

void BaseTranslator_Object::set_monitored_material(Mtl* const mtl)
{
    if(mtl != m_monitored_node_material)
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            // Stop monitoring the old material
            if(m_monitored_node_material != nullptr)
            {
                notifications->StopMonitoringMaterial(*m_monitored_node_material, ~size_t(0), *this, nullptr);
            }
            // Monitor the new material
            if(mtl != nullptr)
            {
                notifications->MonitorMaterial(*mtl, ~size_t(0), *this, nullptr);
            }
        }

        m_monitored_node_material = mtl;
    }

    // Remember whether the last translated material had displacement or not. This way we can determine
    // when displacement is enabled/disabled.
    const ULONG new_mtl_req = (mtl != nullptr) ? mtl->Requirements(-1) : 0;
    m_last_translation_has_material_displacement = ((new_mtl_req & MTLREQ_DISPLACEMAP) != 0);
}

void BaseTranslator_Object::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/)
{
    if(dynamic_cast<const IMaterialEvent*>(&genericEvent) != nullptr)
    {
        const IMaterialEvent& mtl_event = static_cast<const IMaterialEvent&>(genericEvent);
        switch(mtl_event.GetEventType())
        {
        case EventType_Material_Deleted:
            // Ignore this notification, we'll get notified by the node that its material has changed.
            break;
        case EventType_Material_ParamBlock:
        case EventType_Material_Reference:
        case EventType_Material_Uncategorized:
            // Material  has changed: if displacement was/is enabled, then we may need to re-translate
            if(CareAboutMaterialDisplacement())
            {
                Mtl* const mtl = mtl_event.GetMtl();
                const ULONG new_mtl_req = (mtl != nullptr) ? mtl->Requirements(-1) : 0;
                const bool new_displacement_enabled = ((new_mtl_req & MTLREQ_DISPLACEMAP) != 0);
                if(new_displacement_enabled || m_last_translation_has_material_displacement)
                {
                    // Either...
                    // * Displacement newly enabled
                    // * Displacement newly disabled
                    // * Properties of material with displacement have changed (NOTE: we can't tell whether the displacement properties themselves have changed!!!)
                    Invalidate();
                }
            }
            break;
        }
    }
}

}}	// namespace 


