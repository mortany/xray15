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

#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_INode.h>

// Max SDK
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>
#include <maxscript/mxsplugin/mxsPlugin.h>
#include <IRefTargWrappingRefTarg.h>
#include <inode.h>
#include <imtl.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;

using namespace MaxSDK::NotificationAPI;
using namespace TranslationHelpers;

BaseTranslator_INode::BaseTranslator_INode(
    INode& node, 
    const NotifierType notifierType, 
    TranslatorGraphNode& graphNode)
    : Translator(graphNode),
    m_node(node),
    m_notifierType(notifierType),
    m_node_pool_invalid(true),
    m_resolved_material(nullptr),
    m_monitored_node_material(nullptr)
{
    // Monitor the node for interactive changes
    IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
    if(notifications != nullptr)
    {
        notifications->MonitorNode(GetNode(), notifierType, ~size_t(0), *this, nullptr);
    }

    // Monitor the camera (for motion blur property changes)
    GetRenderSessionContext().GetCamera().RegisterChangeNotifier(*this);
}

BaseTranslator_INode::~BaseTranslator_INode()
{
    // Stop monitoring the node for changes
    IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
    if(notifications != nullptr)
    {
        notifications->StopMonitoringNode(GetNode(), ~size_t(0), *this, nullptr);
    }

    // Stop monitoring the camera
    GetRenderSessionContext().GetCamera().UnregisterChangeNotifier(*this);

    // Stop monitoring the node pool
    set_node_pool(std::shared_ptr<const INodeInstancingPool>());

    // Stop monitoring the material
    set_monitored_material(nullptr);
}

MotionTransforms BaseTranslator_INode::EvaluateTransforms(const TimeValue t, Interval& validity) const
{
    const MotionTransforms transforms = GetRenderSessionContext().EvaluateMotionTransforms(m_node, t, validity);
    return transforms;
}

bool BaseTranslator_INode::CareAboutNotificationEvent(const NodeEventType eventType) const
{
    switch(eventType)
    {
    case EventType_Node_Transform:
    case EventType_Node_Hide:
    case EventType_Node_Material_Replaced:
        // All event types lead to re-translation of the object (which should be relatively cheap in any case)
        return true;
    case EventType_Node_Uncategorized:
        // Includes changes to motion blur properties
        return true;
    case EventType_Node_RenderProperty:
    case EventType_Node_GIProperty:
    case EventType_Node_DisplayProperty:
        return true;
    default:
        // Don't care about other notifications
        return false;
    }
}

bool BaseTranslator_INode::CareAboutRenderProperty(const PartID /*render_property_id*/) const
{
    return true;
}

bool BaseTranslator_INode::CareAboutDisplayProperty(const PartID /*display_property_id*/) const
{
    return false;
}

bool BaseTranslator_INode::CareAboutGIProperty(const PartID /*gi_property_id*/) const
{
    return false;
}

void BaseTranslator_INode::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/) 
{
    if(dynamic_cast<const INodeEvent*>(&genericEvent) != nullptr)
    {
        switch(genericEvent.GetEventType())
        {
        case EventType_Node_Deleted:
            // Tell the graph node the translated object has been deleted.
            TranslatedObjectDeleted();
            break;
        default:
            if(CareAboutNotificationEvent(static_cast<NodeEventType>(genericEvent.GetEventType())))
            {
                const INodePropertyEvent* prop_event = dynamic_cast<const INodePropertyEvent*>(&genericEvent);
                if(prop_event != nullptr)
                {
                    // Check if we care about the specific property that changed
                    switch(prop_event->GetEventType())
                    {
                    case EventType_Node_RenderProperty:
                        if(CareAboutRenderProperty(prop_event->GetPropertyID()))
                            Invalidate();
                        break;
                    case EventType_Node_GIProperty:
                        if(CareAboutGIProperty(prop_event->GetPropertyID()))
                            Invalidate();
                        break;
                    case EventType_Node_DisplayProperty:
                        if(CareAboutDisplayProperty(prop_event->GetPropertyID()))
                            Invalidate();
                        break;
                    default:
                        DbgAssert(false);
                        break;
                    }
                }
                else
                {
                    Invalidate();
                }
            }
            break;
        }
    }
    else if(dynamic_cast<const IMaterialEvent*>(&genericEvent) != nullptr)
    {
        // Material has changed, which might result in the resolved material having changed. 
        Invalidate(true);
    }
    else
    {
        // Unhandled event type?
        DbgAssert(false);
    }
}

Interval BaseTranslator_INode::CheckValidity(const TimeValue t, const Interval& previous_validity) const 
{
    Interval evaluation_interval = FOREVER;

    // Check if the motion blur properties changed
    {
        const MotionBlurSettings current_motion_blur_settings = GetRenderSessionContext().GetMotionBlurSettingsForNode(m_node, t, evaluation_interval);
        if(current_motion_blur_settings != m_last_translated_motion_settings)
        {
            return NEVER;
        }
    }

    // Check whether node pool, to which our node belongs, has changed, in which case we need to re-acquire the translator for the pool.
    if(m_node_pool_invalid)
    {
        std::shared_ptr<const TranslationHelpers::INodeInstancingPool> new_node_pool = GetRenderSessionContext().GetNodeInstancingManager().GetPoolForNode(GetNode(), t, m_notifierType);
        if(new_node_pool != m_node_pool)
        {
            return NEVER;
        }
    }

    // Check whether the resolved material changed
    if(CareAboutNotificationEvent(EventType_Node_Material_Replaced))
    {
        Mtl* const new_resolved_mtl = ResolveMaterial(m_node.GetMtl());
        if(new_resolved_mtl != m_resolved_material)
        {
            return NEVER;
        }
    }

    // Combine the existing interval with that of the parameters we just evaluated
    return (evaluation_interval & previous_validity);
}

void BaseTranslator_INode::NotifyCameraChanged()
{
    // It only makes sense to invalidate on transform changes if we care about transform matrix changes.
    if(CareAboutNotificationEvent(EventType_Node_Transform))
    {
        // The camera motion blur parameters may (or may not) have changed in a way which is meaningful to us. Delay the check as it's not safe to evaluate the camera
        // node within the notification callback.
        Invalidate(true);
    }
}

INode& BaseTranslator_INode::GetNode() const
{
    return m_node;
}

Mtl* BaseTranslator_INode::GetMaterial() const
{
    return m_resolved_material;
}

Mtl* BaseTranslator_INode::ResolveMaterial(Mtl* const originalMtl) const
{
    return BaseTranslator_INode::StaticResolveMaterial(originalMtl);
}

Mtl* BaseTranslator_INode::StaticResolveMaterial(Mtl* const originalMtl) 
{
    // Use the core, common resolution functionality
    Mtl* const resolvedMaterial = (originalMtl != nullptr) ? originalMtl->ResolveWrapperMaterials(false) : nullptr;
    
    // Perform additional resolution which is specific to renderers
    if(resolvedMaterial != NULL) 
    {
        // Resolve DX Material
        if(originalMtl->ClassID() == DXMATERIAL_CLASS_ID)
        {
            if(resolvedMaterial->NumSubMtls() > 0 )
            {
                Mtl* const software_mtl = resolvedMaterial->GetSubMtl(0);
                return StaticResolveMaterial(software_mtl);
            }
        }
        else 
        {
            // Resolve scripted materials
            MSPlugin* msPlugin = static_cast<MSPlugin*>(resolvedMaterial->GetInterface(I_MAXSCRIPTPLUGIN));
            if(msPlugin != NULL) 
            {
                ReferenceTarget* delegate = msPlugin->get_delegate();
                if((delegate != NULL) && IsMtl(delegate)) 
                {
                    Mtl* const delegateMtl = static_cast<Mtl*>(delegate);
                    return StaticResolveMaterial(delegateMtl);
                }
            }

            // Resolve IRefTargWrappingRefTarg materials
            ReferenceTarget* delegate = IRefTargWrappingRefTarg::GetWrappedObject(resolvedMaterial, true);
            if((delegate != NULL) && IsMtl(delegate)) 
            {
                Mtl* const delegateMtl = static_cast<Mtl*>(delegate);
                return StaticResolveMaterial(delegateMtl);
            }
        }
    }

    return resolvedMaterial;
}


void BaseTranslator_INode::set_node_pool(const std::shared_ptr<const INodeInstancingPool>& new_pool)
{
    if(new_pool != m_node_pool)
    {
        // Unregister notifications against the old pool
        if(m_node_pool != nullptr)
        {
            m_node_pool->StopMonitoringNodePool(*this);
        }

        m_node_pool = new_pool;

        // Register notifications against the new pool
        if(m_node_pool != nullptr)
        {
            m_node_pool->MonitorNodePool(*this);
        }
    }
}

void BaseTranslator_INode::set_monitored_material(Mtl* const mtl)
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
}

std::shared_ptr<const INodeInstancingPool> BaseTranslator_INode::GetNodePool(const TimeValue t) 
{
    // Update the node pool if necessary
    if(m_node_pool_invalid)
    {
        set_node_pool(GetRenderSessionContext().GetNodeInstancingManager().GetPoolForNode(GetNode(), t, m_notifierType));
        m_node_pool_invalid = false;
    }

    DbgAssert(m_node_pool != nullptr);
    return m_node_pool;
}

void BaseTranslator_INode::NotifyPoolInstancedObjectChanged(const INodeInstancingPool& /*node_pool*/, const NodeEventType /*event_type*/)
{
    // Ignore: we don't care about the instanced object changing
}

void BaseTranslator_INode::NotifyPoolContentMaybeChanged(const INodeInstancingPool& /*node_pool*/)
{
    // Pool content may have changed, meaning that our node may now belong to a different pool. If that's the case, we'll need to re-acquire the
    // translator for the instanced object.
    m_node_pool_invalid = true;
    Invalidate(true);
}

void BaseTranslator_INode::PreTranslate(const TimeValue translationTime, Interval& validity)
{
    // Call RenderBegin on the node
    GetRenderSessionContext().CallRenderBegin(m_node, translationTime);

    // Cache motion settings to later check against invalidation
    const MotionTransforms transforms = GetRenderSessionContext().EvaluateMotionTransforms(m_node, translationTime, validity);
    m_last_translated_motion_settings = transforms.shutter_settings;

    // Cache the resolved material, to later check against invalidation
    Mtl* const node_material = m_node.GetMtl();
    m_resolved_material = ResolveMaterial(node_material);
    // Monitor the node material to know when the resolved material might change
    set_monitored_material((node_material != m_resolved_material) ? node_material : nullptr);    
}

void BaseTranslator_INode::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    Mtl* const material = GetMaterial();
    if(material != nullptr)
    {
        // Check for missing UVW channels, reporting them as needed
        const std::vector<unsigned int> channels_present = GetMeshUVWChannelIDs();

        // If using a multi-material, then only check those materials which are actually
        if(material->IsMultiMtl())
        {
            const int num_sub_materials = material->NumSubMtls();
            if(num_sub_materials > 0)
            {
                // Get list of material IDs referenced by the geometry
                const std::vector<MtlID> material_ids_used = GetMeshMaterialIDs();
                // Individually check each material that is used by the geometry
                for(const MtlID material_id : material_ids_used)
                {
                    Mtl* const sub_material = material->GetSubMtl(material_id % num_sub_materials);
                    if(sub_material != nullptr)
                    {
                        // Check this material
                        CheckMaterialForMissingUVWChannels(*sub_material, channels_present);
                    }
                }
            }
        }
        else
        {
            // Check this material
            CheckMaterialForMissingUVWChannels(*material, channels_present);
        }
    }
}

// Check the given material for missing UVW channels, reporting the missing channels as necessary.
// Doesn't support multi/sub-object materials... assumes the sub-materials are passed here
void BaseTranslator_INode::CheckMaterialForMissingUVWChannels(
    Mtl& mtl,
    const std::vector<unsigned int>& channels_present)
{
    DbgAssert(!mtl.IsMultiMtl());

    const ULONG mtlRequirements = mtl.Requirements(0);

    // Face mapped materials don't need UV coordinates
    if ((mtlRequirements & MTLREQ_FACEMAP) == 0)
    {
        BitArray mapreq, bmpreq;
        mapreq.SetSize(MAX_MESHMAPS);
        bmpreq.SetSize(MAX_MESHMAPS);
        mapreq.ClearAll();
        bmpreq.ClearAll();
        mtl.MappingsRequired(0, mapreq, bmpreq);

        // for backwards compatibility, still use the requirements bits
        if (mtlRequirements & MTLREQ_UV)
            mapreq.Set(1);
        if (mtlRequirements & MTLREQ_UV2)
            mapreq.Set(0);
        if (mtlRequirements & MTLREQ_BUMPUV)
            bmpreq.Set(1);
        if (mtlRequirements & MTLREQ_BUMPUV2)
            bmpreq.Set(0);

        // Merge all requirements into a single list
        mapreq |= bmpreq;

        // Check whether any required channel is missing
        // Do this by clearing the bits that are present, and then checking whether any bits remain set
        for (unsigned int channel_present_id : channels_present)
        {
            mapreq.Clear(channel_present_id);
        }

        if (mapreq.AnyBitSet())
        {
            // Some channels missing... list/report them
            for (int channel_req_id = 0; channel_req_id < mapreq.GetSize(); ++channel_req_id)
            {
                if (mapreq[channel_req_id])
                {
                    // Channel is missing
                    GetRenderSessionContext().GetRenderingProcess().ReportMissingUVWChannel(m_node, channel_req_id);
                }
            }
        }
    }
}

}}	// namespace 


