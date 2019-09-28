#pragma once

//**************************************************************************/
// Copyright (c) 2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// AUTHOR: David Lanier
//***************************************************************************/


#include "ImmediateNotificationClient.h"
#include "Events/MaterialEvent.h"
#include "Events/NodeEvent.h"
#include "Events/TexmapEvent.h"
#include "Events/SceneNodeEvent.h"
#include "Events/ViewEvent.h"
#include "Events/GenericEvent.h"
#include "Notifiers/MaxNotifier.h"
#include "NotificationManager.h"

// maxsdk includes
#include <NotificationAPI/NotificationAPIUtils.h>

// std includes
#include <list>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;


    //Sorts out all referencing messages and callback the subscribers
	class OnDemandNotificationClient : 
        public IOnDemandNotificationClient,
        // INotificationCallback is used to received notifications from the immediate notification client, which are then stored as on-demand notifications.
        private INotificationCallback
	{
    private:
        template<typename EventType>
        struct EventAndUserData
        {
            EventAndUserData(const EventType& event, void* const userData);
            EventType event;    // Store a copy of the event, which we own
            void* const userData;
        private:
            // Disable copy operator
            EventAndUserData& operator=(const EventAndUserData&);
        };

        // Stores a mergeable event in the list of notifications. Mergeable events enable eliminating duplicates from the list before they are added to it.
        // The class of EventType must implement a method with the following signature:
        // EventMergeResult merge_new_event(EventType& new_event);
        // This method tries to merge a new event into an old/existing event.
        template<typename EventType>
        static void StoreMergeableEvent(const EventType& new_event, void* const new_user_data, std::list<EventAndUserData<EventType>>& event_list);

        template<typename EventType>
        static void process_event_list(INotificationCallback& notificationCallback, std::list<EventAndUserData<EventType>>& event_list);

        template<typename EventType>
        static void debug_print_event_list(const std::list<EventAndUserData<EventType>>& event_list, const size_t indent, FILE* const file, const MCHAR* event_type_string);

    private:

        // These lists store events until they get processed. They never contain any null entries.
        std::list<EventAndUserData<NodeEvent>>    m_DirtyUpdateNodeEvents;
        std::list<EventAndUserData<MaterialEvent>> m_DirtyUpdateMaterialEvents;
        std::list<EventAndUserData<TexmapEvent>>   m_DirtyUpdateTexmapEvents;
        std::list<EventAndUserData<SceneNodeEvent>> m_DirtyUpdateSceneEvents;
        std::list<EventAndUserData<ViewEvent>> m_DirtyUpdateViewEvents;
        std::list<EventAndUserData<GenericEvent>> m_DirtyUpdateRenderEnvironmentEvents;
        std::list<EventAndUserData<GenericEvent>> m_DirtyUpdateRenderSettingsEvents;

        mutable NotificationSystemCriticalSectionObject m_lock;

	public:
        OnDemandNotificationClient();
		virtual ~OnDemandNotificationClient();

        //From INotificationclient
        virtual int                     VersionNumber()const{return 1;};
        virtual void                    EnableNotifications(bool enable);
        virtual bool                    NotificationsEnabled(void)const;
		virtual void                    DebugPrintToFile(FILE* file)const;

        //From IOnDemandNotificationClient
		virtual bool MonitorNode				    (INode& node,		NotifierType type, size_t monitoredEvents, void* userData) override;
        virtual bool MonitorMaterial			    (Mtl& mtl,			size_t monitoredEvents, void* userData) override;
		virtual bool MonitorTexmap				    (Texmap& texmap,	size_t monitoredEvents, void* userData) override;
        virtual bool MonitorReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, void* userData) override;
		virtual bool MonitorViewport                (int viewID, size_t monitoredEvents, void* userData) override;
		virtual bool MonitorRenderEnvironment	    (size_t monitoredEvents, void* userData) override;
        virtual bool MonitorRenderSettings          (size_t monitoredEvents, void* userData) override;
		virtual bool MonitorScene			        (size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringNode(INode& node, size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringMaterial(Mtl& mtl, size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringTexmap(Texmap& texmap, size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringViewport(int viewID, size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringRenderEnvironment(size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringRenderSettings(size_t monitoredEvents, void* userData) override;
        virtual bool StopMonitoringScene(size_t monitoredEvents, void* userData) override;
        virtual size_t NumberOfQueuedEvents() override;
        virtual void ProcessEvents(INotificationCallback& notificationCallback) override;

        virtual void GetClassName(MSTR& s) { s = _M("Graphics.OnDemandNotificationClient"); }

    private:

        // These methods store events in the list to be processed on demand. They also eliminate duplicate or redundant events.
        virtual void StoreThisDirtyNodeEvent(const NodeEvent& nodeevent, void* userData);
        virtual void StoreThisDirtyMaterialEvent(const MaterialEvent& matEvent, void* userData);
        virtual void StoreThisDirtyTexmapEvent(const TexmapEvent& texmapEvent, void* userData);
        virtual void StoreThisDirtySceneEvent(const SceneNodeEvent& sceneEvent, void* userData);
        virtual void StoreThisDirtyViewportEvent(const ViewEvent& viewportEvent, void* userData);
        virtual void StoreThisDirtyRenderSettingsEvent(const GenericEvent& renderSettingsEvent, void* userData);
        virtual void StoreThisDirtyRenderEnvironmentEvent(const GenericEvent& renderEnvironmentEvent, void* userData);

        // -- inherited from INotificationCallback
        virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData);

    private:

        // We use an immediate notification callback for all our needs, and use the immediate callback to store the events in a list.
        ImmediateNotificationClient m_immediate_notification_client;
	};


	
};//end of namespace NotificationAPI
};//end of namespace Max