//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "NodeInstancingPool.h"

// max sdk
#include <inode.h>
#include <maxapi.h>
#include <object.h>

#include "3dsmax_banned.h"

namespace
{
    inline GeomObject* to_GeomObject(Object* obj)
    {
        if((obj != nullptr) 
            && ((obj->SuperClassID() == GEOMOBJECT_CLASS_ID) || (obj->SuperClassID() == SHAPE_CLASS_ID)))
        {
            return static_cast<GeomObject*>(obj);
        }
        else
        {
            return nullptr;
        }
    }
}

namespace MaxSDK
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

//==================================================================================================
// class INodeInstancingPool::IManager
//==================================================================================================

std::unique_ptr<INodeInstancingPool::IManager> INodeInstancingPool::IManager::AllocateInstance(NotificationAPI::IImmediateInteractiveRenderingClient* notification_client)
{
    return std::unique_ptr<IManager>(new Max::RenderingAPI::TranslationHelpers::NodeInstancingPool::Manager(notification_client));
}

}}}     // namespace

namespace Max
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

//==================================================================================================
// class NodeInstancingPool
//==================================================================================================

NodeInstancingPool::NodeInstancingPool(
    Manager& manager, 
    IImmediateNotificationClient* notification_client,
    INode& initial_node,
    const NotifierType node_notification_type)
    : m_node_notification_type(node_notification_type),
    m_manager(manager),
    m_notification_client(notification_client),
    m_referenced_object(initial_node.GetObjOrWSMRef())
{
    add_node(initial_node);
}

NodeInstancingPool::~NodeInstancingPool()
{
    // Unregister all notifications
    if(m_notification_client != nullptr)
    {
        for(INode* node : m_nodes)
        {
            m_notification_client->StopMonitoringNode(*node, ~size_t(0), *this, nullptr);
        }
    }
}

INode* NodeInstancingPool::GetRepresentativeNode() const
{
    // Just return the first node in the pool (any node would do, though)
    INode* const node = !m_nodes.empty() ? *(m_nodes.begin()) : nullptr;

    // Sanity check - makes sure that we monitor nodes properly
    DbgAssert((node == nullptr) || (node->GetObjOrWSMRef() == m_referenced_object));

    return node;
}

size_t NodeInstancingPool::GetSize() const 
{
    return m_nodes.size();
}

INode* NodeInstancingPool::GetNode(const size_t index) const 
{
    if(DbgVerify(index < m_nodes.size()))
    {
        INode* const node = *std::next(m_nodes.begin(), index);
        return node;
    }
    else
    {
        return nullptr;
    }
}

void NodeInstancingPool::remove_node(INode& node)
{
    // Find the node
    auto found_it = m_nodes.find(&node);
    if(found_it != m_nodes.end())
    {
        // Remove node from list
        m_nodes.erase(found_it);
        // Stop monitoring the node
        if(m_notification_client != nullptr)
        {
            m_notification_client->StopMonitoringNode(node, ~size_t(0), *this, nullptr);
        }

        // Notify that the pool contents changed
        for(INotifier* notifier : m_notifiers)
        {
            notifier->NotifyPoolContentMaybeChanged(*this);
        }

        // If the pool is now empty, signal the manager
        if(m_nodes.empty())
        {
            m_manager.discard_empty_pool(*this);
        }
    }
    else
    {
        // Not part of the pool? Probably a bug
        DbgAssert(false);
    }
}

void NodeInstancingPool::MonitorNodePool(INotifier& notifier) const
{
    m_notifiers.push_back(&notifier);
}

void NodeInstancingPool::StopMonitoringNodePool(INotifier& notifier) const
{
    for(auto it = m_notifiers.begin(); it != m_notifiers.end(); ++it)
    {
        if(*it == &notifier)
        {
            m_notifiers.erase(it);
            return;
        }
    }
    // Not found: bug?
    DbgAssert(false);
}

void NodeInstancingPool::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/) 
{
    const INodeEvent* const node_event = dynamic_cast<const INodeEvent*>(&genericEvent);
    DbgAssert(node_event != nullptr);
    INode* const changed_node = (node_event != nullptr) ? node_event->GetNode() : nullptr;

    if(changed_node != nullptr)
    {
        bool pooled_object_changed = false;

        switch(genericEvent.GetEventType())
        {
        case EventType_Node_Deleted:
            // A deleted node is automatically kicked out of the pool.
            remove_node(*changed_node);
            if(m_nodes.empty())
            {
                // Notify that the last node in the pool was deleted
                pooled_object_changed = true;
            }
            break;
        default:

            // All other changes: a node changing results in it being potentially kicked out of the pool - except if it's the last node in the pool.
            // This way, we're forcing all pool users to re-acquire the pools for changed nodes, enabling the pool creation process to make these
            // nodes re-join the pool if appropriate, or join another/new pool.
            if(m_nodes.size() > 1)
            {
                remove_node(*changed_node);
            }
            else
            {
                // Last node in the pool has changed: check if the object reference has changed
                INode* const last_node = !m_nodes.empty() ? *(m_nodes.begin()) : nullptr;
                Object* const new_object_reference = (last_node != nullptr) ? last_node->GetObjOrWSMRef() : nullptr;
                if(new_object_reference != m_referenced_object)
                {
                    remove_node(*last_node);
                }
                else
                {
                    // Last node in the pool has changed: the instanced object has to be re-translated
                    pooled_object_changed = true;
                }
            }
        }

        // Notify
        if(pooled_object_changed)
        {
            for(INotifier* notifier : m_notifiers)
            {
                notifier->NotifyPoolInstancedObjectChanged(*this, static_cast<NodeEventType>(genericEvent.GetEventType()));
            }
        }
    }
}

Object* NodeInstancingPool::get_referenced_object() const
{
    return m_referenced_object;
}


void NodeInstancingPool::add_node(INode& node)
{
    DbgAssert(node.GetObjOrWSMRef() == m_referenced_object);

    // Add the node
    const auto insert_result = m_nodes.insert(&node);

    // Upon insertion, monitor the node for changes
    if(insert_result.second && (m_notification_client != nullptr))
    {
        m_notification_client->MonitorNode(node, m_node_notification_type, ~size_t(0), *this, nullptr);

        // Notify that the pool contents changed
        for(INotifier* notifier : m_notifiers)
        {
            notifier->NotifyPoolContentMaybeChanged(*this);
        }
    }
}

bool NodeInstancingPool::is_node_compatible(INode& node, const TimeValue t) const
{
    // Node instancing can be disabled here, for testing purposes
    const bool enable_node_instancing = true;
    if(enable_node_instancing 
        // Do the nodes point to the same object?
        && (node.GetObjOrWSMRef() == m_referenced_object))
    {
        // If the objects have different displacement properties, then they can't share an instance (because the meshes will
        // be generated differently)
        INode* const this_node = GetRepresentativeNode();
        Mtl* const this_material = (this_node != nullptr) ? this_node->GetMtl() : nullptr;
        Mtl* const new_material = node.GetMtl();
        if(this_material != new_material) 
        {
            const ULONG this_mtaterial_req = (this_material != nullptr) ? this_material->Requirements(-1) : 0;
            const ULONG new_material_req = (new_material != nullptr) ? new_material->Requirements(-1) : 0;
            if((this_mtaterial_req & MTLREQ_DISPLACEMAP) || (new_material_req & MTLREQ_DISPLACEMAP))
            {
                // Nodes have different materials and at least one of them has a displacement map: they cannot share the mesh.
                //!! TODO: What do we do in the case of view-dependent displacement? It doesn't seem to be possible to detect this for now,
                // and I'm afraid it might be overkill to disallow instancing displaced objects just in case there might be view-dependent displacement.
                return false;
            }
        }

        // Check whether the object is explicitly instance-dependent
        const ObjectState& os = node.EvalWorldState(t);
        GeomObject* geom_object = to_GeomObject(os.obj);
        if(geom_object != nullptr)
        {
            return !geom_object->IsInstanceDependent();
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

//==================================================================================================
// class NodeInstancingPool::Manager
//==================================================================================================

NodeInstancingPool::Manager::Manager(IImmediateNotificationClient* notification_client)
    : m_notification_client(notification_client)
{

}

NodeInstancingPool::Manager::~Manager()
{

}

std::shared_ptr<const INodeInstancingPool> NodeInstancingPool::Manager::GetPoolForNode(INode& node, const TimeValue t, const NotifierType node_notification_type)
{
    // Check if we already have a pool for the object referenced by this node
    Object* const object = node.GetObjOrWSMRef();

    // See if we've got a compatible pool for this node
    for(auto it = m_pool_lookup.lower_bound(object); (it != m_pool_lookup.end()) && (it->first == object); ++it)
    {
        if(it->second->is_node_compatible(node, t))
        {
            // Found a compatible pool: add the node to it
            it->second->add_node(node);
            return it->second;
        }
    }

    // No compatible pool was found: create a new pool
    auto insert_it = m_pool_lookup.insert(PoolLookup::value_type(object, std::shared_ptr<NodeInstancingPool>(new NodeInstancingPool(*this, m_notification_client, node, node_notification_type))));
    return insert_it->second;
}

void NodeInstancingPool::Manager::discard_empty_pool(NodeInstancingPool& pool)
{
    Object* const referenced_object = pool.get_referenced_object();
    for(auto it = m_pool_lookup.lower_bound(referenced_object); (it != m_pool_lookup.end()) && (it->first == referenced_object); ++it)
    {
        if(it->second.get() == &pool)
        {
            m_pool_lookup.erase(it);
            return;
        }
    }

    // Couldn't find the pool: that's a bug
    DbgAssert(false);
}

}}}	// namespace 

