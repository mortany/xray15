//
// Copyright 2013 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#pragma once

// src/include
#include <NotificationAPI/NotificationAPI_Subscription.h>
// 3ds max sdk
#include <ref.h>
// std
#include <list>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

;

class ImmediateNotificationClient;

// The notifier is responsible for listening to events in the 3ds Max world and broadcasting those events to clients of the Notification API.
class MaxNotifier : 
    // We derive from ReferenceMaker as many notifiers need to listen to reference messages on a ReferenceTarget.
    public ReferenceMaker
{
public:

	MaxNotifier();

    // Use this method to delete this object
    void delete_this();

    //Print debug info to file
    virtual void DebugPrintToFile(FILE* , size_t indent)const  = 0;

    // Registration of event callbacks. A set of events to be monitored is associated with every callback. The callback is only fully
    // unregistered when all its monitored events are unregistered.
    void RegisterEventCallback(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, size_t monitored_events);
    void UnregisterEventCallback(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, size_t monitored_events);

    // Unregisters all callbacks associated with the given notification client
    void UnregisterAllCallbacksForClient(ImmediateNotificationClient& notification_client);

    // Returns whether there are any callbacks currently registered
    bool IsAnyCallbackRegistered() const;

    //-- From ReferenceMaker
    virtual RefResult NotifyRefChanged(
        const Interval& changeInt, 
        RefTargetHandle hTarget, 
        PartID& partID, 
        RefMessage message,
        BOOL propagate );
    virtual int NumRefs();
    virtual RefTargetHandle GetReference(int i);
    virtual BOOL IsRealDependency(ReferenceTarget * );
    virtual void SetReference(int , RefTargetHandle rtarg);

    //-- From Animatable
	MSTR ClassName() const { return _T("NotificationAPI::MaxNotifier");  }

protected:

    // Protected destructor forces going through delete_this()
    virtual ~MaxNotifier();

    // This notifiers all callbacks of the given event
    void NotifyEvent(const IGenericEvent& genericEvent);

    // Returns whether this notifier is currently in the process of being deleted
    bool DeletingThis() const;

private:

    // This structure stores all the information associated with the registration of a notification callback
    struct CallbackInfo
    {
        CallbackInfo(ImmediateNotificationClient& client, INotificationCallback& callback, void* callback_user_data, const size_t monitored_events);
        
        // The client for which this callback is registered. This is needed because callbacks are always associated with an instance of a notification client.
        ImmediateNotificationClient* notification_client;
        // The callback to be called upon receiving an event
        INotificationCallback* callback;
        // Arbitrary user data passed by the user
        void* callback_user_data;   
        // A bit-field, its values being specific to the event type
        size_t monitored_events;
    };

    typedef std::list<CallbackInfo> CallbackList;

private:

    // Unregisters an event callback from a specific callback list
    // Returns true iff the callback was found and unregistered.
    static bool UnregisterEventCallbackFromList(
        CallbackList& callback_list, 
        const bool defer_deletions, 
        ImmediateNotificationClient& notification_client, 
        INotificationCallback& callback, 
        void* const callback_user_data, 
        size_t monitored_events);

private:

    // The list of callbacks which are currently registered on this notifier
    CallbackList m_callbacks;

    // This is set whenever the list of callbacks is currently being traversed. It's used to avoid modifying the list of callbacks while it
    // is being used.
    // When registering notifications, we add the new notification to a separate (deferred) list to ensure that notification doesn't get processed by the current notfication loop.
    // When unregistering notifications, however, we take care not to delete any element of the callback list. Instead, we flag the list entry and defer the deletion of the element.
    bool m_currently_notifying_callbacks;
    // This is set whenever a callback is waiting to be deleted (after the current notification has finished processing)
    bool m_callbacks_to_be_deferred_deleted;
    // This is the list of notification callbacks which are to be added after the current event is done processing.
    CallbackList m_deferred_added_callbacks;

    // Set when this object is currently being deleted, to avoid sending notifications that result from deleting the references
    bool m_deleting_this;
};

}} // namespaces
