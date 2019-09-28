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

#include "NotificationAPI/InteractiveRenderingAPI_Subscription.h"
#include "InteractiveRendering/InteractiveRenderingViewData.h"
#include "BaseInteractiveRenderingClient.h"
#include "NotificationSystemCriticalSectionObject.h"

// maxsdk includes
#include <NotificationAPI/NotificationAPIUtils.h>

// std includes
#include <set>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

//Implementation of IInteractiveRenderingClient
class ImmediateInteractiveRenderingClient : 
    public IImmediateInteractiveRenderingClient, 
    // Implements the functionality that OnDemandInteractiveRenderingClient and ImmediateInteractiveRenderingClient have in common
    private BaseInteractiveRenderingClient
{

protected:

    // -- inherited from BaseInteractiveRenderingClient
    virtual void ProcessEvent(const NotificationAPI::ViewEvent& event);
    virtual bool IsMonitoringActiveView() const;

public:
    ImmediateInteractiveRenderingClient(Max::NotificationAPI::IImmediateNotificationClient& notificationClient);//!< Constructor
    virtual ~ImmediateInteractiveRenderingClient();//!< Destructor
     
    //From IImmediateInteractiveRenderingClient
    virtual bool MonitorActiveShadeView(IInteractiveRenderingCallback& callback, void* userData);
    virtual bool StopMonitoringActiveView(IInteractiveRenderingCallback& callback, void* userData);

    //from IInteractiveRenderingClient
    virtual int                     VersionNumber() const;//!< returns the version number of the notification client

    virtual void DebugPrintToFile(FILE* file)const;

    // -- inherited from IImmediateNotificationClient
    virtual bool MonitorNode(INode& node, NotifierType type, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool MonitorMaterial(Mtl& mtl, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool MonitorTexmap(Texmap& texmap,	size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
    virtual bool MonitorReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool MonitorViewport(int viewID, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool MonitorRenderEnvironment(size_t monitoredEvents, INotificationCallback& callback, void* userData)   override;
    virtual bool MonitorRenderSettings          (size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool MonitorScene(size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool StopMonitoringNode(INode& pNode, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool StopMonitoringMaterial(Mtl& pMtl, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
	virtual bool StopMonitoringTexmap(Texmap& pTexmap, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
    virtual bool StopMonitoringReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
    virtual bool StopMonitoringViewport(int viewID, size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
    virtual bool StopMonitoringRenderEnvironment(size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
    virtual bool StopMonitoringRenderSettings(size_t monitoredEvents, INotificationCallback& callback, void* userData) override;
    virtual bool StopMonitoringScene(size_t monitoredEvents, INotificationCallback& callback, void* userData) override;

    // -- inherited from INotificationClient
    virtual void EnableNotifications(bool enable) override;
    virtual bool NotificationsEnabled(void)const override;


private:

    // The list of registered callbacks
    typedef std::pair<IInteractiveRenderingCallback* /*callback*/, void* /*user_data*/> CallbackInfo;
    typedef std::set<CallbackInfo> CallbackSet;
    CallbackSet m_registered_callbacks;

    // This locks controls registration and processing of events: it ensures that these cannot happen in parallel.
    mutable NotificationSystemCriticalSectionObject m_event_lock;
};


}} //end of namespace 