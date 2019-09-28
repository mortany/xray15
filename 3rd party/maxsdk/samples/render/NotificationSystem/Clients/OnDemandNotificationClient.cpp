//**************************************************************************/
// Copyright (c) 1998-2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Implementation
// AUTHOR: David Lanier
//***************************************************************************/

#include "OnDemandNotificationClient.h"

// src/include
#include "../NotificationAPIUtils.h"
// Max SDK
#include <inode.h>
#include <imtl.h>

namespace Max{

namespace NotificationAPI
{;

namespace
{
    void debug_print_event(const IGenericEvent& event, const size_t indent, FILE* const file)
    {
        Utils::DebugPrintToFileNotifierType(file, event.GetNotifierType(), NULL, indent + 1);
        Utils::DebugPrintToFileEventType(file, event.GetNotifierType(), event.GetEventType(), indent + 1);
    }

    void debug_print_event(const INodeEvent& event, const size_t indent, FILE* const file)
    {
        Utils::DebugPrintToFileNotifierType(file, event.GetNotifierType(), event.GetNode(), indent + 1);
        Utils::DebugPrintToFileEventType(file, event.GetNotifierType(), event.GetEventType(), indent + 1);
    }

    void debug_print_event(const MaterialEvent& event, const size_t indent, FILE* const file)
    {
        Utils::DebugPrintToFileNotifierType(file, event.GetNotifierType(), event.GetMtl(), indent + 1);
        Utils::DebugPrintToFileEventType(file, event.GetNotifierType(), event.GetEventType(), indent + 1);
    }

    void debug_print_event(const TexmapEvent& event, const size_t indent, FILE* const file)
    {
        Utils::DebugPrintToFileNotifierType(file, event.GetNotifierType(), event.GetTexmap(), indent + 1);
        Utils::DebugPrintToFileEventType(file, event.GetNotifierType(), event.GetEventType(), indent + 1);
    }
};

//=================================================================================================
// struct OnDemandNotificationClient::EventAndUserData
//=================================================================================================

template<typename EventType>
inline OnDemandNotificationClient::EventAndUserData<EventType>::EventAndUserData(const EventType& p_event, void* const p_userData)
    : event(p_event),
    userData(p_userData)
{

}

//-------------------------------------------------------------------
// ----------			class OnDemandNotificationClient	---------
//-------------------------------------------------------------------

OnDemandNotificationClient::OnDemandNotificationClient()
{

}

OnDemandNotificationClient::~OnDemandNotificationClient()
{

}

bool OnDemandNotificationClient::MonitorNode(INode& node, NotifierType type, size_t monitoredEvents, void* userDataAssociatedtoThisNode )
{
    return m_immediate_notification_client.MonitorNode(node, type, monitoredEvents, *this, userDataAssociatedtoThisNode);
}

bool OnDemandNotificationClient::MonitorMaterial(Mtl& mtl, size_t monitoredEvents, void* userDataAssociatedtoThisMtl)
{
    return m_immediate_notification_client.MonitorMaterial(mtl, monitoredEvents, *this, userDataAssociatedtoThisMtl);
}

bool OnDemandNotificationClient::MonitorTexmap(Texmap& texmap,	size_t monitoredEvents, void* userDataAssociatedtoThisTexmap)
{	
    return m_immediate_notification_client.MonitorTexmap(texmap, monitoredEvents, *this, userDataAssociatedtoThisTexmap);
}

bool OnDemandNotificationClient::MonitorReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, void* userDataAssociatedtoThisTexmap)
{	
    return m_immediate_notification_client.MonitorReferenceTarget(refTarg, monitoredEvents, *this, userDataAssociatedtoThisTexmap);
}

bool OnDemandNotificationClient::MonitorViewport(int viewID, size_t monitoredEvents, void* userDataAssociatedtoThisView)
{
    return m_immediate_notification_client.MonitorViewport(viewID, monitoredEvents, *this, userDataAssociatedtoThisView);
}

bool OnDemandNotificationClient::MonitorRenderEnvironment(size_t monitoredEvents, void* userDataAssociatedToRenderEnv)
{
    return m_immediate_notification_client.MonitorRenderEnvironment(monitoredEvents, *this, userDataAssociatedToRenderEnv);
}

bool OnDemandNotificationClient::MonitorRenderSettings(size_t monitoredEvents, void* userDataAssociatedToRenderEnv)
{
    return m_immediate_notification_client.MonitorRenderSettings(monitoredEvents, *this, userDataAssociatedToRenderEnv);
}

bool OnDemandNotificationClient::MonitorScene(size_t monitoredEvents, void* userData)
{
    return m_immediate_notification_client.MonitorScene(monitoredEvents, *this, userData);
}

template<typename EventType>
void OnDemandNotificationClient::StoreMergeableEvent(const EventType& const_new_event, void* const new_user_data, std::list<EventAndUserData<EventType>>& event_list)
{
    // Create a non-const copy of the event, which we can merge into
    EventType new_event = const_new_event;

    // Loop through all events, trying to merge the new event with any existing one
    for(auto it = event_list.begin(); it != event_list.end(); )
    {
        bool erase_entry = false;
        EventType& old_event = it->event;
        // See if the new event can be merged with the current one
        const IMergeableEvent::MergeResult merge_result = new_event.merge_from(old_event);
        switch(merge_result)
        {
        default:
            DbgAssert(false);
            // Fall through...
        case IMergeableEvent::MergeResult::NotMerged:
            // Events weren't merged: continue looking at the rest of the list
            break;
        case IMergeableEvent::MergeResult::Merged_KeepOld:
            // New event to be discard
            // Stop looking, since we discarded the new event
            return;
        case IMergeableEvent::MergeResult::Merged_KeepNew:
            // Old event to be discarded
            erase_entry = true;
            break;
        case IMergeableEvent::MergeResult::DiscardBoth:
            // Discard both events
            event_list.erase(it);
            return;
        }

        it = erase_entry ? event_list.erase(it) : ++it;
    }

    // If we got here, it's because the new event wasn't discarded: so add it to the list
    event_list.push_back(EventAndUserData<EventType>(new_event, new_user_data));
}

void OnDemandNotificationClient::StoreThisDirtyNodeEvent(const NodeEvent& nodeevent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    // If the node was deleted, we need to get rid of any existing notification that references this node
    if(nodeevent.GetEventType() == EventType_Node_Deleted)
    {
        INode* const deleted_node = nodeevent.GetNode();
        if(deleted_node != nullptr)
        {
            // Lock
            NotificationSystemAutoCriticalSection nested_lock_guard(m_lock);

            // Remove matching node notifications
            for(auto it = m_DirtyUpdateNodeEvents.begin(); it != m_DirtyUpdateNodeEvents.end(); )
            {
                if(it->event.GetNode() == deleted_node)
                {
                    it = m_DirtyUpdateNodeEvents.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // Remove potential scene addition notifications
            for(auto it = m_DirtyUpdateSceneEvents.begin(); it != m_DirtyUpdateSceneEvents.end(); )
            {
                if(it->event.GetNode() == deleted_node)
                {
                    // This event is now ineffective and can be removed
                    it = m_DirtyUpdateSceneEvents.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    StoreMergeableEvent(nodeevent, userData, m_DirtyUpdateNodeEvents);
}

void OnDemandNotificationClient::StoreThisDirtyMaterialEvent(const MaterialEvent& matEvent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    // If the material is being deleted, remove any event that makes reference to that material
    if(matEvent.GetEventType() == EventType_Material_Deleted)
    {
        // Lock
        NotificationSystemAutoCriticalSection nested_lock_guard(m_lock);

        for(auto it = m_DirtyUpdateMaterialEvents.begin(); it != m_DirtyUpdateMaterialEvents.end(); )
        {
            if(it->event.GetMtl() == matEvent.GetMtl())
            {
                it = m_DirtyUpdateMaterialEvents.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    StoreMergeableEvent(matEvent, userData, m_DirtyUpdateMaterialEvents);
}

void OnDemandNotificationClient::StoreThisDirtyTexmapEvent(const TexmapEvent& texmapEvent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    // If the texmap is being deleted, delete any event that makes a reference to it to avoid dangling pointers
    if(texmapEvent.GetEventType() == EventType_Texmap_Deleted)
    {
        // Lock
        NotificationSystemAutoCriticalSection nested_lock_guard(m_lock);

        for(auto it = m_DirtyUpdateTexmapEvents.begin(); it != m_DirtyUpdateTexmapEvents.end(); )
        {
            if(it->event.GetTexmap() == texmapEvent.GetTexmap())
            {
                it = m_DirtyUpdateTexmapEvents.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    StoreMergeableEvent(texmapEvent, userData, m_DirtyUpdateTexmapEvents);
}

void OnDemandNotificationClient::StoreThisDirtySceneEvent(const SceneNodeEvent& sceneEvent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    StoreMergeableEvent(sceneEvent, userData, m_DirtyUpdateSceneEvents);
}

void OnDemandNotificationClient::StoreThisDirtyViewportEvent(const ViewEvent& viewportEvent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    StoreMergeableEvent(viewportEvent, userData, m_DirtyUpdateViewEvents);
}

void OnDemandNotificationClient::StoreThisDirtyRenderSettingsEvent(const GenericEvent& renderSettingsEvent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    StoreMergeableEvent(renderSettingsEvent, userData, m_DirtyUpdateRenderSettingsEvents);
}

void OnDemandNotificationClient::StoreThisDirtyRenderEnvironmentEvent(const GenericEvent& renderEnvironmentEvent, void* userData)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    StoreMergeableEvent(renderEnvironmentEvent, userData, m_DirtyUpdateRenderEnvironmentEvents);
}

size_t OnDemandNotificationClient::NumberOfQueuedEvents(void)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    return static_cast<int>(m_DirtyUpdateNodeEvents.size()
        + m_DirtyUpdateMaterialEvents.size()
        + m_DirtyUpdateTexmapEvents.size()
        + m_DirtyUpdateSceneEvents.size()
        + m_DirtyUpdateViewEvents.size()
        + m_DirtyUpdateRenderEnvironmentEvents.size()
        + m_DirtyUpdateRenderSettingsEvents.size());
}

void OnDemandNotificationClient::ProcessEvents(INotificationCallback& notificationCallback)
{
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    process_event_list(notificationCallback, m_DirtyUpdateNodeEvents);
    process_event_list(notificationCallback, m_DirtyUpdateMaterialEvents);
    process_event_list(notificationCallback, m_DirtyUpdateTexmapEvents);
    process_event_list(notificationCallback, m_DirtyUpdateSceneEvents);
    process_event_list(notificationCallback, m_DirtyUpdateViewEvents);
    process_event_list(notificationCallback, m_DirtyUpdateRenderEnvironmentEvents);
    process_event_list(notificationCallback, m_DirtyUpdateRenderSettingsEvents);

}

void OnDemandNotificationClient::DebugPrintToFile(FILE* file)const
{
	if (NULL == file ){
		DbgAssert(0 && _T("file is NULL"));
		return;
	}

    size_t indent = 1;
    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    _ftprintf(file, _T("** NotificationClient debugging **\n\n"));

	_ftprintf(file, _T("** NotificationClient version : %d **\n"), VersionNumber());
    _ftprintf(file, _T("** NotificationClient type : OnDemand **\n"));
	
    //Print the data this Client is monitoring from the manager
    m_immediate_notification_client.DebugPrintToFile(file);

       
    // Lock
    NotificationSystemAutoCriticalSection lock_guard(m_lock);

    debug_print_event_list(m_DirtyUpdateNodeEvents, indent, file, _T("Node"));
    debug_print_event_list(m_DirtyUpdateMaterialEvents, indent, file, _T("Material"));
    debug_print_event_list(m_DirtyUpdateTexmapEvents, indent, file, _T("Texmap"));
    debug_print_event_list(m_DirtyUpdateSceneEvents, indent, file, _T("Scene"));
    debug_print_event_list(m_DirtyUpdateViewEvents, indent, file, _T("View"));
    debug_print_event_list(m_DirtyUpdateRenderEnvironmentEvents, indent, file, _T("Render Environment"));
    debug_print_event_list(m_DirtyUpdateRenderSettingsEvents, indent, file, _T("Render Settings"));
}

bool OnDemandNotificationClient::StopMonitoringNode(INode& node, size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringNode(node, monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringMaterial(Mtl& mtl, size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringMaterial(mtl, monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringTexmap(Texmap& texmap, size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringTexmap(texmap, monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringReferenceTarget(refTarg, monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringViewport(int viewID, size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringViewport(viewID, monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringRenderEnvironment(size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringRenderEnvironment(monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringRenderSettings(size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringRenderSettings(monitoredEvents, *this, userData);
}

bool OnDemandNotificationClient::StopMonitoringScene(size_t monitoredEvents, void* const userData)
{
    return m_immediate_notification_client.StopMonitoringScene(monitoredEvents, *this, userData);
}

void OnDemandNotificationClient::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData)
{
    if(dynamic_cast<const NodeEvent*>(&genericEvent) != nullptr)
    {
        StoreThisDirtyNodeEvent(dynamic_cast<const NodeEvent&>(genericEvent), userData);
    }
    else if(dynamic_cast<const MaterialEvent*>(&genericEvent) != nullptr)
    {
        StoreThisDirtyMaterialEvent(static_cast<const MaterialEvent&>(genericEvent), userData);
    }
    else if(dynamic_cast<const TexmapEvent*>(&genericEvent) != nullptr)
    {
        StoreThisDirtyTexmapEvent(static_cast<const TexmapEvent&>(genericEvent), userData);
    }
    else if(dynamic_cast<const SceneNodeEvent*>(&genericEvent) != nullptr)
    {
        StoreThisDirtySceneEvent(static_cast<const SceneNodeEvent&>(genericEvent), userData);
    }
    else if(dynamic_cast<const ViewEvent*>(&genericEvent) != nullptr)
    {
        StoreThisDirtyViewportEvent(static_cast<const ViewEvent&>(genericEvent), userData);
    }
    else if(dynamic_cast<const GenericEvent*>(&genericEvent) != nullptr)
    {
        switch(genericEvent.GetNotifierType())
        {
        case NotifierType_RenderEnvironment:
            StoreThisDirtyRenderEnvironmentEvent(static_cast<const GenericEvent&>(genericEvent), userData);
            break;
        case NotifierType_RenderSettings:
            StoreThisDirtyRenderSettingsEvent(static_cast<const GenericEvent&>(genericEvent), userData);
            break;
        default:
            DbgAssert(false);   // unhandled event type
            break;
        }
    }
    else
    {
        DbgAssert(false);       // unhandled event type
    }
}


void OnDemandNotificationClient::EnableNotifications(bool enable)
{
    // Let the immediate notification client handle this behaviour
    m_immediate_notification_client.EnableNotifications(enable);
}

bool OnDemandNotificationClient::NotificationsEnabled(void)const
{
    // Let the immediate notification client handle this behaviour
    return m_immediate_notification_client.NotificationsEnabled();
}

template<typename EventType>
void OnDemandNotificationClient::process_event_list(INotificationCallback& notificationCallback, std::list<EventAndUserData<EventType>>& event_list)
{
    for(const auto& entry : event_list)
    {
        notificationCallback.NotificationCallback_NotifyEvent(entry.event, entry.userData);
    }
    event_list.clear();
}

template<typename EventType>
void OnDemandNotificationClient::debug_print_event_list(const std::list<EventAndUserData<EventType>>& event_list, const size_t indent, FILE* const file, const MCHAR* event_type_string)
{
    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    _ftprintf(file, indentString);
    _ftprintf(file, TSTR(_T("** ")) + TSTR(event_type_string) + TSTR(_T(" dirty events : **\n")));

    for(const auto& entry : event_list)
    {
        debug_print_event(entry.event, indent, file);
    }
    _ftprintf(file, _T("\n\n"));
}

};//end of namespace NotificationAPI
};//end of namespace Max