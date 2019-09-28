/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#pragma once

// Local includes
#include "BaseInteractiveRenderingClient.h"
#include "NotificationAPI/InteractiveRenderingAPI_Subscription.h"
#include "InteractiveRendering/InteractiveRenderingViewData.h"
#include "Events/ViewEvent.h"
#include "NotificationSystemCriticalSectionObject.h"

// maxsdk includes
#include <NotificationAPI/NotificationAPIUtils.h>

// std includes
#include <list>
#include <set>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

//Implementation of IOnDemandInteractiveRenderingClient
class OnDemandInteractiveRenderingClient : 
    public IOnDemandInteractiveRenderingClient, 
    // Implements the functionality that OnDemandInteractiveRenderingClient and ImmediateInteractiveRenderingClient have in common
    private BaseInteractiveRenderingClient
{
private:

    void StoreThisDirtyViewEvent(const NotificationAPI::ViewEvent& viewEvent);

    // -- inherited from BaseInteractiveRenderingClient
    virtual void ProcessEvent(const NotificationAPI::ViewEvent& event);
    virtual bool IsMonitoringActiveView()const;

public:

    // This constructor is passed an immediate notification client which is used to  monitor the scene (to convert certain notifications into active shade ones).
    // The client passed here is then OWNED by this class - and will be released by its destructor.
    OnDemandInteractiveRenderingClient(NotificationAPI::IImmediateNotificationClient& immediate_notification_client);
    virtual ~OnDemandInteractiveRenderingClient();//!< Destructor
     
    //From IOnDemandInteractiveRenderingClient
    virtual size_t NumberOfQueuedEvents();
    virtual void ProcessEvents(IInteractiveRenderingCallback& notificationCallback);
	virtual bool MonitorActiveShadeView(void* userData);
    virtual bool StopMonitoringActiveView(void* userData);
    
    //From IInteractiveRenderingClient
    virtual int                     VersionNumber() const;//!< returns the version number of the notification client
	virtual void DebugPrintToFile(FILE* theFile)const;

private:

    //We need an immediate client and we are going to store events, we can't use a on demand client for notificationAPI as we need to 
    //monitor viewports changes in real time not only when the user asks for it
    Max::NotificationAPI::IImmediateNotificationClient& m_NotificationClient; 

    // The list of registered user data callbacks
    std::set<void*> m_registered_callbacks;

    // The list of events waiting to be processed
    std::list<NotificationAPI::ViewEvent> m_events_queue;

    // This locks controls registration and processing of events: it ensures that these cannot happen in parallel.
    mutable NotificationSystemCriticalSectionObject m_event_lock;
};


}}; //end of namespace 