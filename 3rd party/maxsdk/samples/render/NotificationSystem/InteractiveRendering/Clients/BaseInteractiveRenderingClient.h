/******************************************************************************
* Copyright 2014 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
#pragma once

#include "InteractiveRendering/InteractiveRenderingViewData.h"

#include "Events/ViewEvent.h"

#include <NotificationAPI/NotificationAPI_Subscription.h>

namespace Max
{
namespace NotificationAPI
{;

// Base class for both interactive and on-demand interactive clients
class BaseInteractiveRenderingClient : public Max::NotificationAPI::INotificationCallback
{
public:

    // The constructor is given a reference to a notification client which is used to convert certain scene notifications to active shade notifications.
    // The client passed here IS NOT TO BE USED IN THE DESTRUCTOR as it may no longer be valid at that point (the sub-class might have deleted it
    // in its own destructor).
    BaseInteractiveRenderingClient(NotificationAPI::IImmediateNotificationClient& notification_client);

    // -- inherited From NotificationCallback
    virtual void NotificationCallback_NotifyEvent(const Max::NotificationAPI::IGenericEvent& genericEvent, void* userData);

protected:

    // Implementation-specific processing of a ViewEvent. The event being passed is a temporary object and musn't be retained past this call.
    virtual void ProcessEvent(const NotificationAPI::ViewEvent& event) = 0;

    // Used by MonitorActiveShadeView_Base() to determine if already monitoring the view
    virtual bool IsMonitoringActiveView() const = 0;

    // Common functionality for both implementations
    bool MonitorActiveShadeView_Base();
    bool StopMonitoringActiveView_Base();

private:

    //When the view lock/unlock state has changed from UI
    void LockViewStateChanged(const NotificationAPI::IGenericEvent& genericEvent);
    //When the active viewport has changed, check the lock/unlock active shade workflow and monitor new view if necessary
    void ActiveViewportChanged(const NotificationAPI::ViewEvent& view_event);
    //When the active camera we were rendering was deleted
    void ActiveCameraWasDeleted(const NotificationAPI::INodeEvent& node_event);
    //Common code to several functions to monitor active view (whatever it is viewport, camera or light node), when viewIDToUse is >= 0 we use it to monitor only that viewport and not the active viewport (lockworkflow)
    void MonitorViewportOrCameraOrLightNode(bool bUseOldViewID = false);
    //common code to several functions to stop monitoring active view (whatever it is viewport, camera or light node)
    bool StopMonitoringViewportOrCameraOrLightNode(void);

protected:

    NotificationAPI::IImmediateNotificationClient& m_NotificationClient;

private:

    InteractiveRenderingViewData m_ViewData;

};

}}  // namespace