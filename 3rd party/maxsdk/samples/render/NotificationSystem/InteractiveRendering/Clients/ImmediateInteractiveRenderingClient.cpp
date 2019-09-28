/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#include "ImmediateInteractiveRenderingClient.h"

using namespace Max::NotificationAPI;

ImmediateInteractiveRenderingClient::ImmediateInteractiveRenderingClient(Max::NotificationAPI::IImmediateNotificationClient& notifClient)
    : BaseInteractiveRenderingClient(notifClient)
{

}

ImmediateInteractiveRenderingClient::~ImmediateInteractiveRenderingClient()
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    // Stop monitoring if still monitoring. We must do this because we're borrowing someone else's notification client.
    if(!m_registered_callbacks.empty())
    {
        DbgVerify(StopMonitoringActiveView_Base());
    }
}

bool ImmediateInteractiveRenderingClient::MonitorActiveShadeView(IInteractiveRenderingCallback& callback, void* userData)
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    // Check if callback already registered
    if(m_registered_callbacks.find(CallbackInfo(&callback, userData)) == m_registered_callbacks.end())
    {
        if(MonitorActiveShadeView_Base())
        {
            m_registered_callbacks.insert(CallbackInfo(&callback, userData));
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // Already registered: fail
        return false;
    }
}

bool ImmediateInteractiveRenderingClient::StopMonitoringActiveView(IInteractiveRenderingCallback& callback, void* userData)
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    const auto it = m_registered_callbacks.find(CallbackInfo(&callback, userData));
    if(it != m_registered_callbacks.end())
    {
        // Remove from the list
        m_registered_callbacks.erase(it);

        // Stop monitoring on last removal
        if(m_registered_callbacks.empty())
        {
            return StopMonitoringActiveView_Base();
        }
        else
        {
            return true;
        }
    }
    else
    {
        // Callback not found: fail
        return false;
    }
}

bool ImmediateInteractiveRenderingClient::IsMonitoringActiveView(void)const
{
    return !m_registered_callbacks.empty();
}

int ImmediateInteractiveRenderingClient::VersionNumber() const
{
    return 1;
}

void ImmediateInteractiveRenderingClient::DebugPrintToFile(FILE* file)const
{
    if (! file){
        DbgAssert(0 &&_T("file is NULL"));
        return;
    }

    _ftprintf(file, _T("** ImmediateInteractiveRenderingClient debugging **\n"));

	_ftprintf(file, _T("** NotificationClient version : %d **\n"), VersionNumber());
    _ftprintf(file, _T("** NotificationClient type : Immediate **\n"));
}

void ImmediateInteractiveRenderingClient::ProcessEvent(const ViewEvent& event)
{
    // Call all the callbacks
    for(const CallbackInfo& callback : m_registered_callbacks)
    {
        callback.first->InteractiveRenderingCallback_NotifyEvent(event, callback.second);
    }
}

bool ImmediateInteractiveRenderingClient::MonitorNode(INode& node, NotifierType type, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorNode(node, type, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorMaterial(Mtl& mtl, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorMaterial(mtl, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorTexmap(Texmap& texmap,	size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorTexmap(texmap, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorReferenceTarget(refTarg, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorViewport(int viewID, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorViewport(viewID, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorRenderEnvironment(size_t monitoredEvents, INotificationCallback& callback, void* userData)  
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorRenderEnvironment(monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorRenderSettings          (size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorRenderSettings(monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::MonitorScene(size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.MonitorScene(monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringNode(INode& pNode, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringNode(pNode, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringMaterial(Mtl& pMtl, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringMaterial(pMtl, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringTexmap(Texmap& pTexmap, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringTexmap(pTexmap, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringReferenceTarget(refTarg, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringViewport(int viewID, size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringViewport(viewID, monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringRenderEnvironment(size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringRenderEnvironment(monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringRenderSettings(size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringRenderSettings(monitoredEvents, callback, userData);
}

bool ImmediateInteractiveRenderingClient::StopMonitoringScene(size_t monitoredEvents, INotificationCallback& callback, void* userData)
{
    // Forward to the immediate client
    return m_NotificationClient.StopMonitoringScene(monitoredEvents, callback, userData);
}

void ImmediateInteractiveRenderingClient::EnableNotifications(bool enable) 
{
    // Forward to the immediate client
    return m_NotificationClient.EnableNotifications(enable);
}

bool ImmediateInteractiveRenderingClient::NotificationsEnabled(void)const 
{
    // Forward to the immediate client
    return m_NotificationClient.NotificationsEnabled();
}

