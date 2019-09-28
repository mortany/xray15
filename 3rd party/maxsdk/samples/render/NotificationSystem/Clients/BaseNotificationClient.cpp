//**************************************************************************/
// Copyright (c) 2014 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/

#include "Precomp/PrecompHeader_Notification.h"

namespace Max
{
namespace NotificationAPI
{;

bool BaseNotificationClient::MonitorNode(INode& node, const NotifierType type, const size_t monitoredEvents)
{
    if(monitoredEvents != 0)
    {
        MaxNodeNotifier* pMaxNodeNotifier = GetNotificationManager().GetNodeNotifier(node);
        if (! pMaxNodeNotifier){
            //No Notifier yet for this node, create one
            pMaxNodeNotifier = new MaxNodeNotifier(&node);
            GetNotificationManager().AddNodeNotifier(*pMaxNodeNotifier);
        }

        //!! TODO: This check is problematic, it prevents multiple monitors of the same node. 
        const NotificationDataListener* pExistingListener =  pMaxNodeNotifier->GetByClientListener(*this);
        if (NULL != pExistingListener){
            const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
            if (monitoredEventsFromListener != monitoredEvents){
                NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                    return false;
            }

            //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
            return true;
        }

        NodeNotificationDataListener* pNodeNotificationDataListener = NULL;

        switch(type){
        case NotifierType_Camera:
            pNodeNotificationDataListener = new CameraNotificationDataListener(*this, &node, type, monitoredEvents);
            break;
        case NotifierType_Light:
            pNodeNotificationDataListener = new LightNotificationDataListener(*this, &node, type, monitoredEvents);
            break;
        case NotifierType_MNMesh:
        case NotifierType_TriMesh:
            pNodeNotificationDataListener = new MeshNotificationDataListener(*this, &node, type, monitoredEvents);
            break;
        default:
            pNodeNotificationDataListener = new NodeNotificationDataListener(*this, &node, type, monitoredEvents);
            break;
        }

        //Sort listeners by internal msg supported
        GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxNodeNotifier, pNodeNotificationDataListener);

        return true;
    }
    else
    {
        // Potentially a bug?
        DbgAssert(false);
        return false;
    }
}

bool BaseNotificationClient::MonitorMaterial(Mtl& mtl, const NotifierType type, const size_t monitoredEvents)
{
    if(type != NotifierType_Unknown)
    {
        MaxMaterialNotifier* pMaxBaseMtlNotifier = GetNotificationManager().GetMaterialNotifier(mtl);
        if (! pMaxBaseMtlNotifier){
            //No Notifier yet for this Mtl, create one
            pMaxBaseMtlNotifier = new MaxMaterialNotifier(&mtl);
            GetNotificationManager().AddMaterialNotifier(*pMaxBaseMtlNotifier);
        }

        const NotificationDataListener* pExistingListener =  pMaxBaseMtlNotifier->GetByClientListener(*this);
        if (NULL != pExistingListener){
            const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
            if (monitoredEventsFromListener != monitoredEvents){
                NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                    return false;
            }

            //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
            return true;
        }

        MaterialNotificationDataListener* pNotificationDataListener = new MaterialNotificationDataListener(*this, &mtl, type, monitoredEvents);

        //Sort listeners by internal msg supported
        GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxBaseMtlNotifier, pNotificationDataListener);

        return true;
    }
    else
    {
        DbgAssert(false);   // Probably a bug
        return false;
    }
}

bool BaseNotificationClient::MonitorTexmap(Texmap& texmap, const NotifierType type, const size_t monitoredEvents)
{	
    if(type != NotifierType_Unknown)
    {
        MaxTexmapNotifier* pMaxTexmapNotifier = GetNotificationManager().GetTexmapNotifier(texmap);
        if (! pMaxTexmapNotifier){
            //No Notifier yet for this Texmap, create one
            pMaxTexmapNotifier = new MaxTexmapNotifier(&texmap);
            GetNotificationManager().AddTexmapNotifier(*pMaxTexmapNotifier);
        }

        const NotificationDataListener* pExistingListener =  pMaxTexmapNotifier->GetByClientListener(*this);
        if (NULL != pExistingListener){
            const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
            if (monitoredEventsFromListener != monitoredEvents){
                NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                    return false;
            }

            //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
            return true;
        }

        TexmapNotificationDataListener* pNotificationDataListener = new TexmapNotificationDataListener(*this, &texmap, type, monitoredEvents);

        //Sort listeners by internal msg supported
        GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxTexmapNotifier, pNotificationDataListener);

        return true;
    }
    else
    {
        DbgAssert(false);   // probably a bug
        return false;
    }
}

bool BaseNotificationClient::MonitorViewport(const int viewID, const size_t monitoredEvents)
{
    if(viewID >= 0)
    {
        //Create or get the MaxViewNotifier
        MaxViewNotifier* pMaxNotifier = GetNotificationManager().GetViewNotifier(viewID);
        if (! pMaxNotifier){
            //No Notifier yet for this Texmap, create one
            pMaxNotifier = new MaxViewNotifier(viewID);
            GetNotificationManager().AddViewNotifier(*pMaxNotifier);
        }

        const NotificationDataListener* pExistingListener =  pMaxNotifier->GetByClientListener(*this);
        if (NULL != pExistingListener){
            const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
            if (monitoredEventsFromListener != monitoredEvents){
                NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                    return false;
            }

            //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
            return true;
        }

        ViewNotificationDataListener* pNotificationDataListener = new ViewNotificationDataListener(*this, NULL, NotifierType_View, monitoredEvents);

        //Sort listeners by internal msg supported
        GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxNotifier, pNotificationDataListener);

        return true;
    }
    else
    {
        DbgAssert(false);   // probably a bug
        return false;
    }
}

bool BaseNotificationClient::MonitorRenderEnvironment(const size_t monitoredEvents)
{
    //Create or get the MaxNotifier
    MaxRenderEnvironmentNotifier* pMaxRenderEnvironmentNotifier = GetNotificationManager().GetRenderEnvironmentNotifier();
    if (! pMaxRenderEnvironmentNotifier){
        //No Notifier yet for this Texmap, create one
        pMaxRenderEnvironmentNotifier = new MaxRenderEnvironmentNotifier();
        GetNotificationManager().AddRenderEnvironmentNotifier(*pMaxRenderEnvironmentNotifier);
    }

    const NotificationDataListener* pExistingListener =  pMaxRenderEnvironmentNotifier->GetByClientListener(*this);
    if (NULL != pExistingListener){
        const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
        if (monitoredEventsFromListener != monitoredEvents){
            NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                return false;
        }

        //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
        return true;
    }

    RenderEnvironmentNotificationDataListener* pNotificationDataListener = new RenderEnvironmentNotificationDataListener(*this, NULL, NotifierType_RenderEnvironment, monitoredEvents);

    //Sort listeners by internal msg supported
    GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxRenderEnvironmentNotifier, pNotificationDataListener);

    return true;
}

bool BaseNotificationClient::MonitorRenderSettings(const size_t monitoredEvents)
{
    //Create or get the MaxNotifier
    MaxRenderSettingsNotifier* pMaxRenderSettingsNotifier = GetNotificationManager().GetRenderSettingsNotifier();
    if (! pMaxRenderSettingsNotifier){
        //No Notifier yet for this Texmap, create one
        pMaxRenderSettingsNotifier = new MaxRenderSettingsNotifier();
        GetNotificationManager().AddRenderSettingsNotifier(*pMaxRenderSettingsNotifier);
    }

    const NotificationDataListener* pExistingListener =  pMaxRenderSettingsNotifier->GetByClientListener(*this);
    if (NULL != pExistingListener){
        const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
        if (monitoredEventsFromListener != monitoredEvents){
            NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                return false;
        }

        //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
        return true;
    }

    RenderSettingsNotificationDataListener* pNotificationDataListener = new RenderSettingsNotificationDataListener(*this, NULL, NotifierType_RenderSettings, monitoredEvents);

    //Sort listeners by internal msg supported
    GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxRenderSettingsNotifier, pNotificationDataListener);

    return true;
}

bool BaseNotificationClient::MonitorScene(const size_t monitoredEvents)
{
    MaxSceneNotifier* pMaxSceneNotifier = GetNotificationManager().GetSceneNotifier();
    if (! pMaxSceneNotifier){
        //No Notifier yet for this Texmap, create one
        pMaxSceneNotifier = new MaxSceneNotifier();
        GetNotificationManager().AddSceneNotifier(*pMaxSceneNotifier);
    }

    const NotificationDataListener* pExistingListener =  pMaxSceneNotifier->GetByClientListener(*this);
    if (NULL != pExistingListener){
        const size_t monitoredEventsFromListener = pExistingListener->GetMonitoredEvents();
        if (monitoredEventsFromListener != monitoredEvents){
            NOTIFICATIONAPI_DBGASSERT_STUFF_ALREADY_MONITORED
                return false;
        }

        //We You are trying to monitor something that is already monitored by this Notification API client, simply ignore this.
        return true;
    }

    SceneNotificationDataListener* pNotificationDataListener = new SceneNotificationDataListener(*this, NULL, NotifierType_Scene, monitoredEvents);

    //Sort listeners by internal msg supported
    GetNotificationManager().RegisterListenerByInternalMsg(monitoredEvents, pMaxSceneNotifier, pNotificationDataListener);

    return true;
}

bool BaseNotificationClient::StopMonitoringNode(INode& node, const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringNodeEvents(*this, &node, monitoredEvents);
}

bool BaseNotificationClient::StopMonitoringMaterial(Mtl& mtl, const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringMaterialEvents(*this, &mtl, monitoredEvents);
}

bool BaseNotificationClient::StopMonitoringTexmap(Texmap& texmap, const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringTexmapEvents(*this, &texmap, monitoredEvents);
}

bool BaseNotificationClient::StopMonitoringViewport(const int viewID, const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringViewEvents(*this, viewID, monitoredEvents);
}

bool BaseNotificationClient::StopMonitoringRenderEnvironment(const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringRenderEnvironmentEvents(*this, monitoredEvents);
}

bool BaseNotificationClient::StopMonitoringRenderSettings(const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringRenderSettingsEvents(*this, monitoredEvents);
}

bool BaseNotificationClient::StopMonitoringScene(const size_t monitoredEvents)
{
    return GetNotificationManager().StopMonitoringSceneEvents(*this, monitoredEvents);
}


}}      // namespaces