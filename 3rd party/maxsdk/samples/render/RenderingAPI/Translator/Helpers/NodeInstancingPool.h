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

// max sdk
#include <Noncopyable.h>
#include <RenderingAPI/Translator/Helpers/INodeInstancingPool.h>
#include <NotificationAPI/NotificationAPI_Subscription.h>
// std includes
#include <vector>
#include <set>
#include <map>
#include <memory>

class Object;

namespace Max
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::RenderingAPI::TranslationHelpers;
using namespace MaxSDK::NotificationAPI;

// This container encapsulates a collection of nodes which all instance the same geometric object. All the nodes contained in a single pool can
// therefore share a single mesh at translation time.
class NodeInstancingPool :
    public INodeInstancingPool2,
    public MaxSDK::Util::Noncopyable,
    private INotificationCallback
{
public:

    class Manager;

    NodeInstancingPool(
        Manager& manager, 
        IImmediateNotificationClient* notification_client,
        // The first node in the pool
        INode& initial_node,
        // The type of notification to be monitored
        const NotifierType node_notification_type);
    virtual ~NodeInstancingPool();

    // -- inherited from INodeInstancingPool
    virtual INode* GetRepresentativeNode() const override;
    virtual void MonitorNodePool(INotifier& notifier) const override;
    virtual void StopMonitoringNodePool(INotifier& notifier) const override;

    // -- inherited from INodeInstancingPool2
    virtual size_t GetSize() const override;
    virtual INode* GetNode(const size_t) const override;

private:

    // Returns the object to which this pool is associated.
    Object* get_referenced_object() const;

    // Returns whether the given node is compatible with this pool, which may depend on a number of factors
    // The time is required in case the object state needs to be evaluated to determine instance dependency
    bool is_node_compatible(INode& node, const TimeValue t) const;
    // Adds a node if it's not already part of this pool
    void add_node(INode& node);

    // Removes a node from the pool. 
    // IMPORTANT: this call may result in the pool being deleted. The pool should therefore no longer be used after this call.
    void remove_node(INode& node);

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) override;

private:

    // The nodes which are part of this pool
    std::set<INode*> m_nodes;
    // The notifiers to call when the nodes change, mutable to allow registering notifications without changing the content of the pool
    mutable std::vector<INotifier*> m_notifiers;
    IImmediateNotificationClient* m_notification_client;
    Manager& m_manager;
    // The notifier type to be registered with the notification wrapper
    const NotifierType m_node_notification_type;

    // The object to which this pool is associated to. Any nodes that refer to this object may be part of this pool.
    Object* const m_referenced_object;
};

class NodeInstancingPool::Manager :
    public IManager,
    public MaxSDK::Util::Noncopyable
{
public:

    Manager(IImmediateNotificationClient* notification_client);
    ~Manager();

    // -- inherited from IManager
    virtual std::shared_ptr<const INodeInstancingPool> GetPoolForNode(INode& node, const TimeValue t, const NotifierType node_notification_type);

    // To be called by the pool when it becomes empty. Discards the empty pool.
    void discard_empty_pool(NodeInstancingPool& pool);

private:

    // The lookup table, which maps the node's object reference to a corresponding pool.
    // There may be several pools for a single Object*, as nodes that refer to the same object can potentially be incompatible for the target renderer.
    typedef std::multimap<Object*, std::shared_ptr<NodeInstancingPool>> PoolLookup;
    PoolLookup m_pool_lookup;
    
    IImmediateNotificationClient* m_notification_client;
};

}}}	// namespace 

