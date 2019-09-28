//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "SceneContainer_Base.h"

// max sdk
#include <Rendering/ToneOperator.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <render.h>

#include "3dsmax_banned.h"

using namespace MaxSDK;

namespace Max
{;
namespace RenderingAPI
{;

SceneContainer_Base::SceneContainer_Base(const IRenderSettingsContainer& render_settings_container, IImmediateInteractiveRenderingClient* const notification_client)

    : m_scene_root_node(nullptr),
    m_scene_contents_validity(NEVER),
    m_scene_bounding_box_validity(NEVER),
    m_render_settings_container(render_settings_container),
    m_notification_client(notification_client)
{
    // Monitor the scene for changes
    if(m_notification_client != nullptr)
    {
        m_notification_client->MonitorScene(~size_t(0), *this, nullptr);
    }
}

SceneContainer_Base::~SceneContainer_Base()
{
    // All notifiers should have been unregistered
    DbgAssert(m_change_notifiers.empty());

    // Stop monitor everything being monitored
    if(m_notification_client != nullptr)
    {
        for(INode* const node : m_monitored_nodes_list)
        {
            m_notification_client->StopMonitoringNode(*node, ~size_t(0), *this, nullptr);
        }
        m_monitored_nodes_list.clear();

        m_notification_client->StopMonitoringScene(~size_t(0), *this, nullptr);
    }
}

void SceneContainer_Base::RegisterChangeNotifier(IChangeNotifier& notifier) const
{
    m_change_notifiers.push_back(&notifier);
}

void SceneContainer_Base::UnregisterChangeNotifier(IChangeNotifier& notifier) const
{
    for(auto it = m_change_notifiers.begin(); it != m_change_notifiers.end(); ++it)
    {
        if(*it == &notifier)
        {
            m_change_notifiers.erase(it);
            return;
        }
    }

    // Shouldn't get here
    DbgAssert(false);
}

void SceneContainer_Base::set_scene_root_node(INode* const root_node)
{
    if(root_node != m_scene_root_node)
    {
        m_scene_root_node = root_node;
        invalidate_scene_contents();
    }
}

INode* SceneContainer_Base::GetRootNode() const
{
    return m_scene_root_node;
}

void SceneContainer_Base::rebuild_scene_nodes_lists_if_needed(const TimeValue t) const
{
    // Rebuild as necessary
    if(!m_scene_contents_validity.InInterval(t))
    {
        m_scene_geom_nodes.clear();
        m_scene_light_nodes.clear();
        m_scene_helper_nodes.clear();
        m_scene_excluded_geom_nodes.clear();
        m_scene_excluded_light_nodes.clear();
        m_scene_contents_validity.SetInfinite();

        if(m_scene_root_node != nullptr)
        {
            // Make sure to include xrefs while rendering
            const BOOL xres_includes_old_value = GetCOREInterface()->GetIncludeXRefsInHierarchy();
            GetCOREInterface()->SetIncludeXRefsInHierarchy(TRUE);

            flatten_node_hierarchy_recursively(*m_scene_root_node, t);

            // Restore the old value
            GetCOREInterface()->SetIncludeXRefsInHierarchy(xres_includes_old_value);
        }

        scene_content_rebuilt(m_scene_geom_nodes, m_scene_light_nodes, m_scene_helper_nodes, m_scene_excluded_geom_nodes, m_scene_excluded_light_nodes);
    }
}

void SceneContainer_Base::scene_content_rebuilt(
    const std::vector<INode*>& geom_nodes,
    const std::vector<INode*>& light_nodes,
    const std::vector<INode*>& helper_nodes,
    const std::vector<INode*>& excluded_geom_nodes,
    const std::vector<INode*>& excluded_light_nodes) const
{
    if(m_notification_client != nullptr)
    {
        // Unregister all current change notifications
        for(INode* const node : m_monitored_nodes_list)
        {
            m_notification_client->StopMonitoringNode(*node, ~size_t(0), const_cast<SceneContainer_Base&>(*this), nullptr);
        }
        m_monitored_nodes_list.clear();

        m_monitored_nodes_list.reserve(geom_nodes.size() + light_nodes.size() + helper_nodes.size());

        // Monitor geometry nodes
        for(INode* const node : geom_nodes)
        {
            m_notification_client->MonitorNode(*node, NotifierType_Node_Geom, ~size_t(0), const_cast<SceneContainer_Base&>(*this), nullptr);
            m_monitored_nodes_list.push_back(node);
        }
        for(INode* const node : excluded_geom_nodes)
        {
            m_notification_client->MonitorNode(*node, NotifierType_Node_Geom, ~size_t(0), const_cast<SceneContainer_Base&>(*this), nullptr);
            m_monitored_nodes_list.push_back(node);
        }

        // Monitor light nodes
        for(INode* const node : light_nodes)
        {
            m_notification_client->MonitorNode(*node, NotifierType_Node_Light, ~size_t(0), const_cast<SceneContainer_Base&>(*this), nullptr);
            m_monitored_nodes_list.push_back(node);
        }
        for(INode* const node : excluded_light_nodes)
        {
            m_notification_client->MonitorNode(*node, NotifierType_Node_Light, ~size_t(0), const_cast<SceneContainer_Base&>(*this), nullptr);
            m_monitored_nodes_list.push_back(node);
        }

        // Monitor helper nodes
        for(INode* const node : helper_nodes)
        {
            m_notification_client->MonitorNode(*node, NotifierType_Node_Helper, ~size_t(0), const_cast<SceneContainer_Base&>(*this), nullptr);
            m_monitored_nodes_list.push_back(node);
        }
    }
}

std::vector<INode*> SceneContainer_Base::GetGeometricNodes(const TimeValue t, Interval& validity) const 
{
    rebuild_scene_nodes_lists_if_needed(t);

    validity &= m_scene_contents_validity;
    return m_scene_geom_nodes;
}

std::vector<INode*> SceneContainer_Base::GetLightNodes(const TimeValue t, Interval& validity) const 
{
    rebuild_scene_nodes_lists_if_needed(t);

    validity &= m_scene_contents_validity;
    return m_scene_light_nodes;
}

std::vector<INode*> SceneContainer_Base::GetHelperNodes(const TimeValue t, Interval& validity) const 
{
    rebuild_scene_nodes_lists_if_needed(t);

    validity &= m_scene_contents_validity;
    return m_scene_helper_nodes;
}

// Adds the given node's bounding box to the given scene bounding box
inline void add_node_bounding_box(INode& node, Box3& bounding_box, const TimeValue t, Interval& validity)
{
    const ObjectState& object_state = node.EvalWorldState(t);
    if(object_state.obj != nullptr)
    {
        Interval tm_validity = FOREVER;
        Matrix3 node_tm = node.GetObjTMAfterWSM(t, &tm_validity);
        validity &= tm_validity;

        Box3 object_bounding_box;
        object_state.obj->GetDeformBBox(t, object_bounding_box, &node_tm);

        bounding_box += object_bounding_box;
    }
}

Box3 SceneContainer_Base::GetSceneBoundingBox(const TimeValue t, Interval& validity) const
{
    // Rebuild as necessary
    if(!m_scene_bounding_box_validity.InInterval(t))
    {
        rebuild_scene_nodes_lists_if_needed(t);
        
        // Start with an empty box
        m_scene_bounding_box = Box3();
        m_scene_bounding_box_validity = FOREVER;

        // Get the cumulative bounding boxes of the scene geometry and lights
        for(INode* const node : m_scene_geom_nodes)
        {
            add_node_bounding_box(*node, m_scene_bounding_box, t, validity);
        }
        for(INode* const node : m_scene_light_nodes)
        {
            add_node_bounding_box(*node, m_scene_bounding_box, t, validity);
        }

        // Bounding box validity is dependent on scene content validity
        m_scene_bounding_box_validity &= m_scene_contents_validity;
    }

    validity &= m_scene_bounding_box_validity;
    return m_scene_bounding_box;
}

bool SceneContainer_Base::is_node_hidden(INode& node) const
{
    if(!m_render_settings_container.GetRenderHiddenObjects())
    {
        return node.IsNodeHidden(true) 
            || (m_render_settings_container.GetHideFrozenObjects() && node.IsFrozen());
    }
    else
    {
        // We render hidden objects - so nothing is considered hidden
        return false;
    }
}
void SceneContainer_Base::flatten_node_hierarchy_recursively(INode& node, const TimeValue t) const
{
    // See what list the node fits into
    if(!const_cast<INode&>(node).IsRootNode())    // Root nodes are empty shells, we don't render them.
    {
        const ObjectState& object_state = node.EvalWorldState(t);
        if((object_state.obj != nullptr) && (object_state.obj->ClassID().PartA() != STANDIN_CLASS_ID))
        {
            switch(object_state.obj->SuperClassID())
            {
            case SHAPE_CLASS_ID:
            case GEOMOBJECT_CLASS_ID:
                // Check if node is excluded from render
                if(node.Renderable() 
                    && !is_node_hidden(node)
                    && object_state.obj->IsRenderable() 
                    && (node.Selected() || !m_render_settings_container.GetRenderOnlySelected()))
                {
                    m_scene_geom_nodes.push_back(&node);
                }
                else
                {
                    m_scene_excluded_geom_nodes.push_back(&node);
                }
                break;
            case LIGHT_CLASS_ID:
                if(node.Renderable())
                {
                    m_scene_light_nodes.push_back(&node);
                }
                else
                {
                    m_scene_excluded_light_nodes.push_back(&node);
                }
                break;
            case HELPER_CLASS_ID:
                m_scene_helper_nodes.push_back(&node);
                break;
            default:
                // Meh - don't care about other types?
                break;
            }
        }
    }

    // Traverse the child nodes
    const int node_count = const_cast<INode&>(node).NumberOfChildren();
    for(int i = 0; i < node_count; ++i) 
    {
        INode* childNode = const_cast<INode&>(node).GetChildNode(i);
        if(childNode != NULL)
        {
            flatten_node_hierarchy_recursively(*childNode, t);
        }
    }
}

void SceneContainer_Base::invalidate_scene_contents()
{
    m_scene_contents_validity.SetEmpty();

    // Scene bounding box automatically invalidated whenever nodes get added/removed to/from scene
    invalidate_scene_bounding_box();

    // Notify listeners
    for(IChangeNotifier* const notifier : m_change_notifiers)
    {
        notifier->NotifySceneNodesChanged();
    }
}

void SceneContainer_Base::invalidate_scene_bounding_box()
{
    m_scene_bounding_box_validity.SetEmpty();

    // Notify listeners
    for(IChangeNotifier* const notifier : m_change_notifiers)
    {
        notifier->NotifySceneBoundingBoxChanged();
    }
}

void SceneContainer_Base::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/)
{
    switch(genericEvent.GetNotifierType())
    {
    case NotifierType_SceneNode:
        // Invalidate the scene contents
        invalidate_scene_contents();
        break;
    case NotifierType_Node_Light:
    case NotifierType_Node_Geom:
        switch(genericEvent.GetEventType())
        {
        case EventType_Node_Hide:      // might result in scene content changing
        case EventType_Node_Deleted:
            invalidate_scene_contents();
            break;
        case EventType_Node_Transform:
        case EventType_Mesh_Vertices:
        case EventType_Mesh_Faces:
        case EventType_Mesh_UVs:
        case EventType_Node_ParamBlock:     // assume param block changes can change the geometry...?
            invalidate_scene_bounding_box();
            break;
        case EventType_Node_Selection:
            // Node selected/unselected is only relevant if we're rendering selected
            if(m_render_settings_container.GetRenderOnlySelected())
            {
                invalidate_scene_contents();
            }
            break;
        case EventType_Node_RenderProperty:
        {
            const INodePropertyEvent* node_prop_event = dynamic_cast<const INodePropertyEvent*>(&genericEvent);
            if(DbgVerify(node_prop_event != nullptr))
            {
                switch(node_prop_event->GetPropertyID())
                {
                case PART_REND_PROP_RENDERABLE:
                    invalidate_scene_contents();
                    break;
                default:
                    break;
                }
            }
        }
        break;
        default:
            // Don't care about other notifications
            break;
        }
        break;
    default:
        DbgAssert(false);
        break;
    }
}

}}	// namespace

