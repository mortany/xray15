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

#include "NotificationAPI/NotificationAPI_Subscription.h"
#include "NotificationSystemCriticalSectionObject.h"

// maxsdk includes
#include <NotificationAPI/NotificationAPIUtils.h>

// std includes
#include <map>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

    class MaxNotifier;
    class MaxRenderEnvironmentNotifier;
    class MaxRenderSettingsNotifier;
    class MaxSceneNotifier;
    class MaxNodeNotifier;
    class MaxMaterialNotifier;
    class MaxTexmapNotifier;
    class MaxRefTargNotifier;
    class MaxViewNotifier;
    class ImmediateNotificationClient;

    //The manager
    class NotificationManager : public INotificationManager
	{
    private:

        // Constructor and destructor are private to enforce singleton pattern
        NotificationManager();
        virtual ~NotificationManager(){};

        // Removes the given client from the given notifier, and deletes the notifier if this is the last client.
        // Returns true iff the notifier was deleted.
        static bool RemoveClientFromNotifier(ImmediateNotificationClient& client, MaxNotifier& notifier);
        template<typename MonitoredType, typename NotifierType>
        static void RemoveClientFromNotifiers(ImmediateNotificationClient& client, std::map<MonitoredType, NotifierType>& notifiers_map);
   
    protected:
        MaxSDK::Array<INotificationClient*>             m_Clients;//Clients are customers of this API listening to some events
        // These containers map an object being monitored to a notifier. The key is the object being monitored for two reasons: efficient searching,
        // and identifying registered notifiers even after they might have nulled their own pointer to the object being monitored (for cause
        // of that object having been deleted). The key of the map must only be used for searching; it must never be de-rerefenced, as it is kept
        // around even after the object in question has been deleted by the system.
        std::map<INode*, MaxNodeNotifier*>              m_NodeNotifiers;
        std::map<Mtl*, MaxMaterialNotifier*>            m_MaterialNotifiers;
        std::map<Texmap*, MaxTexmapNotifier*>           m_TexmapNotifiers;
        std::map<ReferenceTarget*, MaxRefTargNotifier*>           m_RefTargNotifiers;
        std::map<int, MaxViewNotifier*>                 m_ViewNotifiers;       // maps "view ID" to notifier
        // These notifiers exist only once, as they only have one object to monitor
        MaxRenderEnvironmentNotifier* m_RenderEnvironmentNotifier;
        MaxRenderSettingsNotifier* m_RenderSettingsNotifier;
        MaxSceneNotifier* m_SceneNotifier;

	public:

        // Returns the singleton of this class
        static NotificationManager& GetInstance();

        virtual IImmediateNotificationClient* RegisterNewImmediateClient(int version = -1);
        virtual IOnDemandNotificationClient*  RegisterNewOnDemandClient(int version = -1);
        virtual bool RemoveClient(INotificationClient* listener);

        virtual size_t NumClients()const{return m_Clients.length();};
        virtual const INotificationClient* GetClient(size_t index)const;

        // Register the given notification client and callback with the given object.
        void RegisterWithNodeNotifier(INode& node, const NotifierType notifier_type, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithMaterialNotifier(Mtl& mtl, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithTexmapNotifier(Texmap& texmap, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithReferenceTargetNotifier(ReferenceTarget& refTarg, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithViewNotifier(const int viewID, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithRenderEnvironmentNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithRenderSettingsNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void RegisterWithSceneNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        
        void UnRegisterWithNodeNotifier(INode& node, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithMaterialNotifier(Mtl& mtl, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithTexmapNotifier(Texmap& texmap, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithReferenceTargetNotifier(ReferenceTarget& refTarg, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithViewNotifier(const int viewID, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithRenderEnvironmentNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithRenderSettingsNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);
        void UnRegisterWithSceneNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events);

        // Unregisters a notification client from all notifiers (useful when the notification client is being deleted
        void UnregisterNotificationClient(ImmediateNotificationClient& notification_client);

        // -- inherited from INotificationManager
        virtual void DebugPrintToFile(FILE* file) const override;
        virtual void NotifyRenderSettingsEvent(const RenderSettingsEventType eventType) const override;
        virtual void NotifyEnvironmentEvent(const RenderEnvironmentEventType eventType) const override;

        void Notify_SceneNodeAdded(INode& node);
        void Notify_SceneNodeRemoved(INode& node);

    private:

        // Generic methods for registering/unregistering a notification client with a notifier.
        template<typename TableKeyType, typename MonitoredObjectType, typename NotifierType>
        static void RegisterWitNotifier(
            MonitoredObjectType& monitored_object, 
            TableKeyType table_key, 
            ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events,
            std::map<TableKeyType, NotifierType*>& notifier_table);
        template<typename TableKeyType, typename NotifierType>
        static void UnRegisterWitNotifiers(
            TableKeyType table_key, 
            ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events,
            std::map<TableKeyType, NotifierType*>& notifier_table);
        template<typename NotifierType>
        static void UnRegisterWitNotifier(
            ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events,
            NotifierType*& notifier);

    private:

        // This locks controls registration and processing of notifiers
        mutable NotificationSystemCriticalSectionObject m_notifiers_lock;
    };

};//end of namespace NotificationAPI
};//end of namespace Max