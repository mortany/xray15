//**************************************************************************/
// Copyright (c) 2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//***************************************************************************/

#include "NotificationManager.h"

#include "Clients/ImmediateNotificationClient.h"
#include "Clients/OnDemandNotificationClient.h"

#include "Notifiers/MaxMaterialNotifier.h"
#include "Notifiers/MaxNodeNotifier.h"
#include "Notifiers/MaxRenderEnvironmentNotifier.h"
#include "Notifiers/MaxRenderSettingsNotifier.h"
#include "Notifiers/MaxSceneNotifier.h"
#include "Notifiers/MaxTexmapNotifier.h"
#include "Notifiers/MaxRefTargNotifier.h"
#include "Notifiers/MaxViewNotifier.h"

namespace MaxSDK
{;
namespace NotificationAPI
{;

//-------------------------------------------------------------------
// ----------			class INotificationManager			---------
//-------------------------------------------------------------------

INotificationManager* INotificationManager::GetManager()
{
    return &(Max::NotificationAPI::NotificationManager::GetInstance());
}

}}      // namespace

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

//-------------------------------------------------------------------
// ----------			class NotificationManager			---------
//-------------------------------------------------------------------

NotificationManager::NotificationManager()
    : m_RenderEnvironmentNotifier(nullptr),
    m_RenderSettingsNotifier(nullptr),
    m_SceneNotifier(nullptr)
{

}
		
NotificationManager& NotificationManager::GetInstance()
{
    static NotificationManager theInstance;
    return theInstance;
}

IImmediateNotificationClient* NotificationManager::RegisterNewImmediateClient(int version /*= -1*/)
{
    ImmediateNotificationClient* engine  = NULL;
    switch(version){
        //Handle there any versionning for the notification engine
        case -1:
        default:
            //Create latest version
			engine = new ImmediateNotificationClient();
        break;
    }
    DbgAssert(NULL != engine && _T("client is NULL"));

    m_Clients.append(engine);

    return engine;
}

IOnDemandNotificationClient* NotificationManager::RegisterNewOnDemandClient(int version /*= -1*/)
{
    OnDemandNotificationClient* engine  = NULL;
    switch(version){
        //Handle there any versionning for the notification engine
        case -1:
        default:
            //Create latest version
			engine = new OnDemandNotificationClient();
        break;
    }
    DbgAssert(NULL != engine && _T("client is NULL"));

    m_Clients.append(engine);

    return engine;
}

bool NotificationManager::RemoveClient(INotificationClient* client)
{
    DbgAssert(client && _T("client is NULL !"));
    if (client){
        const size_t index = m_Clients.find(client);
        if (index == (size_t)-1){
            DbgAssert(0 && _T("client pointer not found in m_Clients !"));
            return false;
        }

        INotificationClient* pClient = m_Clients[index];

        delete pClient;
		pClient = NULL;

        m_Clients.removeAt(index);
        
        return true;
    }

    return false;
}

void NotificationManager::UnregisterNotificationClient(ImmediateNotificationClient& client)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    RemoveClientFromNotifiers(client, m_NodeNotifiers);
    RemoveClientFromNotifiers(client, m_MaterialNotifiers);
    RemoveClientFromNotifiers(client, m_TexmapNotifiers);
    RemoveClientFromNotifiers(client, m_ViewNotifiers);
    if((m_RenderEnvironmentNotifier != nullptr) && RemoveClientFromNotifier(client, *m_RenderEnvironmentNotifier))
        m_RenderEnvironmentNotifier = nullptr;
    if((m_RenderSettingsNotifier != nullptr) && RemoveClientFromNotifier(client, *m_RenderSettingsNotifier))
        m_RenderSettingsNotifier = nullptr;
    if((m_SceneNotifier != nullptr) && RemoveClientFromNotifier(client, *m_SceneNotifier))
        m_SceneNotifier = nullptr;
}

bool NotificationManager::RemoveClientFromNotifier(ImmediateNotificationClient& client, MaxNotifier& notifier)
{
    notifier.UnregisterAllCallbacksForClient(client);
    if(!notifier.IsAnyCallbackRegistered())
    {
        notifier.delete_this();
        return true;
    }
    else
    {
        return false;
    }
}

template<typename MonitoredType, typename NotifierType>
void NotificationManager::RemoveClientFromNotifiers(ImmediateNotificationClient& client, std::map<MonitoredType, NotifierType>& notifiers_map)
{
    for(auto it = notifiers_map.begin(); it != notifiers_map.end(); )
    {
        MaxNotifier* notif = it->second;
        if((notif != nullptr) && RemoveClientFromNotifier(client, *notif))
        {
            // Notifier was deleted: remove it from the list
            it = notifiers_map.erase(it);
        }
        else
        {
            // Notifier wasn't deleted: move on to the next one
            ++it;
        }
    }
}

const INotificationClient* NotificationManager::GetClient(size_t index)const
{
    const size_t NumClients = m_Clients.length();

    if (index > NumClients){
        DbgAssert(0 && _T("Invalid index"));
        return NULL;
    }
    
    DbgAssert(NULL != m_Clients[index] &&_T("client is null"));
    return m_Clients[index];
}

template<typename TableKeyType, typename MonitoredObjectType, typename NotifierType>
void NotificationManager::RegisterWitNotifier(
    MonitoredObjectType& monitored_object, 
    TableKeyType table_key, 
    ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events,
    std::map<TableKeyType, NotifierType*>& notifier_table)
{
    // See if a notifier already exists
    auto found_it = notifier_table.find(table_key);

    // Create a new notifier if necessary
    if(found_it == notifier_table.end())
    {
        NotifierType* const new_notifier = new NotifierType(monitored_object);
        found_it = notifier_table.insert(std::map<TableKeyType, NotifierType*>::value_type(table_key, new_notifier)).first;
    }

    DbgAssert(found_it->second != nullptr);     // That would be a bug
    if(found_it->second != nullptr)
    {
        found_it->second->RegisterEventCallback(notification_client, callback, callback_user_data, monitored_events);
    }
}

void NotificationManager::RegisterWithNodeNotifier(INode& node, const NotifierType notifier_type, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    // See if a notifier already exists
    auto found_it = m_NodeNotifiers.find(&node);

    // Create a new notifier if necessary
    if(found_it == m_NodeNotifiers.end())
    {
        MaxNodeNotifier* const new_notifier = new MaxNodeNotifier(node, notifier_type);
        found_it = m_NodeNotifiers.insert(std::map<INode*, MaxNodeNotifier*>::value_type(&node, new_notifier)).first;
    }

    DbgAssert(found_it->second != nullptr);     // That would be a bug
    if(found_it->second != nullptr)
    {
        DbgAssert(found_it->second->GetNotifierType() == notifier_type);
        found_it->second->RegisterEventCallback(notification_client, callback, callback_user_data, monitored_events);
    }
}

void NotificationManager::RegisterWithMaterialNotifier(Mtl& mtl, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    RegisterWitNotifier(mtl, &mtl, notification_client, callback, callback_user_data, monitored_events, m_MaterialNotifiers);
}

void NotificationManager::RegisterWithTexmapNotifier(Texmap& texmap, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    RegisterWitNotifier(texmap, &texmap, notification_client, callback, callback_user_data, monitored_events, m_TexmapNotifiers);
}

void NotificationManager::RegisterWithReferenceTargetNotifier(ReferenceTarget& refTarg, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    RegisterWitNotifier(refTarg, &refTarg, notification_client, callback, callback_user_data, monitored_events, m_RefTargNotifiers);
}

void NotificationManager::RegisterWithViewNotifier(const int viewID, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    RegisterWitNotifier(viewID, viewID, notification_client, callback, callback_user_data, monitored_events, m_ViewNotifiers);
}

void NotificationManager::RegisterWithRenderEnvironmentNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    // Create the notifier as necessary
    if(m_RenderEnvironmentNotifier == nullptr)
    {
        m_RenderEnvironmentNotifier = new MaxRenderEnvironmentNotifier();
    }

    m_RenderEnvironmentNotifier->RegisterEventCallback(notification_client, callback, callback_user_data, monitored_events);
}

void NotificationManager::RegisterWithRenderSettingsNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    // Create the notifier as necessary
    if(m_RenderSettingsNotifier == nullptr)
    {
        m_RenderSettingsNotifier = new MaxRenderSettingsNotifier();
    }

    m_RenderSettingsNotifier->RegisterEventCallback(notification_client, callback, callback_user_data, monitored_events);
}

void NotificationManager::RegisterWithSceneNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    // Create the notifier as necessary
    if(m_SceneNotifier == nullptr)
    {
        m_SceneNotifier = new MaxSceneNotifier();
    }

    m_SceneNotifier->RegisterEventCallback(notification_client, callback, callback_user_data, monitored_events);
}

template<typename TableKeyType, typename NotifierType>
void NotificationManager::UnRegisterWitNotifiers(
    TableKeyType table_key, 
    ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events,
    std::map<TableKeyType, NotifierType*>& notifier_table)
{
    // See if a notifier already exists
    auto found_it = notifier_table.find(table_key);
    if(found_it != notifier_table.end())
    {
        MaxNotifier* notifier = found_it->second;
        UnRegisterWitNotifier(notification_client, callback, callback_user_data, monitored_events, notifier);

        // Remove the list entry if the notifier was deleted
        if(notifier == nullptr)
        {
            notifier_table.erase(found_it);
        }
    }
    else
    {
        // Notifier not found: bug?
        DbgAssert(false);
    }
}

template<typename NotifierType>
void NotificationManager::UnRegisterWitNotifier(
    ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events,
    NotifierType*& notifier)
{
    // See if a notifier already exists
    if(notifier != nullptr)
    {
        // Unregister the callback
        notifier->UnregisterEventCallback(notification_client, callback, callback_user_data, monitored_events);

        // Delete the notifier if it no longer has any clients registered
        if(!notifier->IsAnyCallbackRegistered())
        {
            notifier->delete_this();
            notifier = nullptr;
        }
    }
    else
    {
        // Notifier not found: bug?
        DbgAssert(false);
    }
}

void NotificationManager::UnRegisterWithNodeNotifier(INode& node, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);    
    UnRegisterWitNotifiers(&node, notification_client, callback, callback_user_data, monitored_events, m_NodeNotifiers);
}

void NotificationManager::UnRegisterWithMaterialNotifier(Mtl& mtl, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifiers(&mtl, notification_client, callback, callback_user_data, monitored_events, m_MaterialNotifiers);
}

void NotificationManager::UnRegisterWithTexmapNotifier(Texmap& texmap, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifiers(&texmap, notification_client, callback, callback_user_data, monitored_events, m_TexmapNotifiers);
}

void NotificationManager::UnRegisterWithReferenceTargetNotifier(ReferenceTarget& refTarg, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifiers(&refTarg, notification_client, callback, callback_user_data, monitored_events, m_RefTargNotifiers);
}

void NotificationManager::UnRegisterWithViewNotifier(const int viewID, ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifiers(viewID, notification_client, callback, callback_user_data, monitored_events, m_ViewNotifiers);
}

void NotificationManager::UnRegisterWithRenderEnvironmentNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifier(notification_client, callback, callback_user_data, monitored_events, m_RenderEnvironmentNotifier);
}

void NotificationManager::UnRegisterWithRenderSettingsNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifier(notification_client, callback, callback_user_data, monitored_events, m_RenderSettingsNotifier);
}

void NotificationManager::UnRegisterWithSceneNotifier(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);
    UnRegisterWitNotifier(notification_client, callback, callback_user_data, monitored_events, m_SceneNotifier);
}

void NotificationManager::DebugPrintToFile(FILE* /*file*/)const
{
    // Note: I delete the code as it would have to be re-written, but let's worry about that if/when we need it.
}

void NotificationManager::Notify_SceneNodeAdded(INode& node)
{
    if(m_SceneNotifier != nullptr)
    {
        m_SceneNotifier->Notify_SceneNodeAdded(node);
    }
}

void NotificationManager::Notify_SceneNodeRemoved(INode& node)
{
    if(m_SceneNotifier != nullptr)
    {
        m_SceneNotifier->Notify_SceneNodeRemoved(node);
    }
}

void NotificationManager::NotifyRenderSettingsEvent(const RenderSettingsEventType eventType) const 
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    switch(eventType) {
    case EventType_RenderSettings_LockView:
        //Lock view state has changed from common render settings dialog (renddlg.cpp SetActiveView function)
        if(m_RenderSettingsNotifier != nullptr) 
        {
            m_RenderSettingsNotifier->NotifyEvent_LockView();
        }
        break;
    default:
        DbgAssert(false);
        break;
    }
}

void NotificationManager::NotifyEnvironmentEvent(const RenderEnvironmentEventType eventType) const 
{
    NotificationSystemAutoCriticalSection lock_guard(m_notifiers_lock);

    if(m_RenderEnvironmentNotifier != nullptr)
    {
        switch(eventType)
        {
        case EventType_RenderEnvironment_BackgroundColor:
            m_RenderEnvironmentNotifier->NotifyEvent_BackgroundColor();
            break;
        case EventType_RenderEnvironment_EnvironmentMap:
            m_RenderEnvironmentNotifier->NotifyEvent_EnvironmentMap();
            break;
        case EventType_RenderEnvironment_EnvironmentMapState:
            m_RenderEnvironmentNotifier->NotifyEvent_EnvironmentMapState();
            break;
        default:
            DbgAssert(false);
            break;
        }
    }
}

}} //End of namespaces
