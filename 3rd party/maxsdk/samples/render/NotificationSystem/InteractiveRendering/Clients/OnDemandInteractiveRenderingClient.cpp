/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#include "OnDemandInteractiveRenderingClient.h"

using namespace Max::NotificationAPI;

OnDemandInteractiveRenderingClient::OnDemandInteractiveRenderingClient(IImmediateNotificationClient& immediate_notification_client)
    : m_NotificationClient(immediate_notification_client),
    BaseInteractiveRenderingClient(immediate_notification_client)
{

}

OnDemandInteractiveRenderingClient::~OnDemandInteractiveRenderingClient()
{
    // Stop monitoring if still monitoring. We must do this because we're borrowing someone else's notification client.
    if(!m_registered_callbacks.empty())
    {
        DbgVerify(StopMonitoringActiveView_Base());
    }

    // Release the notification client (as its owned by 'this' class)
    DbgVerify(INotificationManager::GetManager()->RemoveClient(&m_NotificationClient));
}

bool OnDemandInteractiveRenderingClient::MonitorActiveShadeView(void* userData)
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    // Check if callback already registered
    if(m_registered_callbacks.find(userData) == m_registered_callbacks.end())
    {
        if(MonitorActiveShadeView_Base())
        {
            m_registered_callbacks.insert(userData);
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

bool OnDemandInteractiveRenderingClient::IsMonitoringActiveView(void)const
{
    return !m_registered_callbacks.empty();
}


bool OnDemandInteractiveRenderingClient::StopMonitoringActiveView(void* userData)
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    const auto it = m_registered_callbacks.find(userData);
    if(it != m_registered_callbacks.end())
    {
        // Remove from the list
        m_registered_callbacks.erase(it);

        // Stop monitoring on last removal
        if(m_registered_callbacks.empty())
        {
            // Clear the queue - as the events may become invalid while we're not watching for new events
            m_events_queue.clear();
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


int OnDemandInteractiveRenderingClient::VersionNumber() const
{
    return 1;
}

void OnDemandInteractiveRenderingClient::StoreThisDirtyViewEvent(const ViewEvent& newEvent)
{
    // Bit field of events to be deleted
    size_t delete_event_types = 0;

    switch(newEvent.GetEventType())
    {
    case EventType_View_Deleted:
        delete_event_types = (EventType_View_Properties | EventType_View_Active | EventType_View_Deleted);//Delete all others events
        break;
    case EventType_View_Active:
        delete_event_types = (EventType_View_Properties | EventType_View_Active);//Delete all past transform or active view changed events as view has changed, so we don't care about them
        break;
    case EventType_View_Properties:
        delete_event_types = (EventType_View_Properties);//Delete all others transform events
        break;
    }

    // Clean up redundant/obsolete events
    if(delete_event_types != 0)
    {
        for(auto it = m_events_queue.begin(); it != m_events_queue.end(); )
        {
            if(it->GetEventType() & delete_event_types)
            {
                it = m_events_queue.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Store the new event
    m_events_queue.push_back(newEvent);
}

void OnDemandInteractiveRenderingClient::DebugPrintToFile(FILE* file)const
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    if (! file){
        DbgAssert(0 &&_T("file is NULL"));
        return;
    }

    _ftprintf(file, _T("** OnDemandInteractiveRenderingClient debugging **\n"));

    _ftprintf(file, _T("** NotificationClient version : %d **\n"), VersionNumber());
    _ftprintf(file, _T("** NotificationClient type : OnDemand **\n"));

    size_t indent = 1; //Start with one tab char
    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    {
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** View dirty events : **\n"));
        for(const auto& event : m_events_queue)
        {
            event.DebugPrintToFile(file);
        }

        --indent;
        _ftprintf(file, _T("\n\n"));
    }
}

void OnDemandInteractiveRenderingClient::ProcessEvent(const ViewEvent& event)
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    // Store the event in the queue
    StoreThisDirtyViewEvent(event);
}

size_t OnDemandInteractiveRenderingClient::NumberOfQueuedEvents()
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    return m_events_queue.size();
}

void OnDemandInteractiveRenderingClient::ProcessEvents(IInteractiveRenderingCallback& notificationCallback)
{
    NotificationSystemAutoCriticalSection lock_guard(m_event_lock);

    // Process every event with every registered user data
    for(const NotificationAPI::ViewEvent& event : m_events_queue)
    {
        for(void* const user_data : m_registered_callbacks)
        {
            notificationCallback.InteractiveRenderingCallback_NotifyEvent(event, user_data);
        }
    }

    // Clear the events, now that they've been processed
    m_events_queue.clear();
}

