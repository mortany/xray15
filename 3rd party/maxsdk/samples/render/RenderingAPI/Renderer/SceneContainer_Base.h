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

#include <interval.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <NotificationAPI/NotificationAPI_Subscription.h>
#include <NotificationAPI/InteractiveRenderingAPI_Subscription.h>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;

// The interactive (active shade) version of ISceneContainer
class SceneContainer_Base :
    public ISceneContainer,
    public MaxSDK::Util::Noncopyable,
    private INotificationCallback
{
public:

    explicit SceneContainer_Base(const IRenderSettingsContainer& render_settings_container, IImmediateInteractiveRenderingClient* const notification_client);
    virtual ~SceneContainer_Base();

    void set_scene_root_node(INode* const root_node);

    // -- inherited from ISceneContainer
    virtual INode* GetRootNode() const override;
    virtual std::vector<INode*> GetGeometricNodes(const TimeValue t, Interval& validity) const override;
    virtual std::vector<INode*> GetLightNodes(const TimeValue t, Interval& validity) const override;
    virtual std::vector<INode*> GetHelperNodes(const TimeValue t, Interval& validity) const override;
    virtual void RegisterChangeNotifier(IChangeNotifier& notifier) const override final;
    virtual void UnregisterChangeNotifier(IChangeNotifier& notifier) const override final;

    virtual Box3 GetSceneBoundingBox(const TimeValue t, Interval& validity) const override;

protected:

    // Invalidates cached scene data and notifies listeners of the change
    void invalidate_scene_contents();   // Includes invalidation of bounding box
    void invalidate_scene_bounding_box();

    // These are called after the scene contents are re-built, to allow the interactive sub-class to monitor the scene according to the new contents.
    void scene_content_rebuilt(
        const std::vector<INode*>& geom_nodes, 
        const std::vector<INode*>& light_nodes, 
        const std::vector<INode*>& helper_nodes,
        const std::vector<INode*>& excluded_geom_nodes,
        const std::vector<INode*>& excluded_light_nodes) const;

    // To be called when the properties of a monitored (excluded) node have changed.
    void excluded_node_properties_changed(INode& node);

protected:

    const IRenderSettingsContainer& m_render_settings_container;

private:

    // Recursively flattens the node hierarchy into the given vector.
    void flatten_node_hierarchy_recursively(INode& node, const TimeValue t) const;

    // Rebuilds the list of scene nodes, if needed
    void rebuild_scene_nodes_lists_if_needed(const TimeValue t) const;

    // Returns whether the ndoe is hidden, for our rendering purposes
    bool is_node_hidden(INode& node) const;

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) override;

private:

    INode* m_scene_root_node;

    // Cached scene content - all mutable because it has no impact on th exact content on the scene
    mutable Interval m_scene_contents_validity;
    mutable std::vector<INode*> m_scene_geom_nodes;
    mutable std::vector<INode*> m_scene_light_nodes;
    mutable std::vector<INode*> m_scene_helper_nodes;
    mutable std::vector<INode*> m_scene_excluded_geom_nodes;
    mutable std::vector<INode*> m_scene_excluded_light_nodes;
    mutable Interval m_scene_bounding_box_validity;
    mutable Box3 m_scene_bounding_box;

    // mutable as registering notifiers doesn't change the contents of the scene container
    mutable std::vector<IChangeNotifier*> m_change_notifiers;

    IImmediateNotificationClient* const m_notification_client;

    // All nodes which are now monitoring
    mutable std::vector<INode*> m_monitored_nodes_list;
};

}}	// namespace

