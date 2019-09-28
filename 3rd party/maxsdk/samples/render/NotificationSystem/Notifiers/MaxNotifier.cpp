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

#include "MaxNotifier.h"

#include "../Clients/ImmediateNotificationClient.h"


namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

;

//==================================================================================================
// struct MaxNotifier::CallbackInfo
//==================================================================================================

MaxNotifier::CallbackInfo::CallbackInfo(ImmediateNotificationClient& p_client, INotificationCallback& p_callback, void* const p_callback_user_data, const size_t p_monitored_events)
    : notification_client(&p_client),
    callback(&p_callback),
    callback_user_data(p_callback_user_data),
    monitored_events(p_monitored_events)
{

}

//==================================================================================================
// class MaxNotifier
//==================================================================================================

MaxNotifier::MaxNotifier() 
    : m_currently_notifying_callbacks(false),
    m_callbacks_to_be_deferred_deleted(false),
    m_deleting_this(false)
{
}

MaxNotifier::~MaxNotifier()
{
}

void MaxNotifier::delete_this()
{
    m_deleting_this = true;
    theHold.Suspend(); //suspend undo, we don't want our notifier to be stored in the undo stack
    this->DeleteMe();
    theHold.Resume();
}

int MaxNotifier::NumRefs()
{
    return 0;
}

RefTargetHandle MaxNotifier::GetReference(int )
{
    // No referenecs, shouldn't be called
    DbgAssert(false);
    return NULL;
}

RefResult MaxNotifier::NotifyRefChanged(
			const Interval& /*changeInt*/, 
			RefTargetHandle /*hTarget*/, 
			PartID& /*partID*/, 
			RefMessage /*message*/,
            BOOL /*propagate */)
{
    return REF_SUCCEED;
}

void MaxNotifier::SetReference(int , RefTargetHandle )
{
    // No references, shouldn't be called
    DbgAssert(false);
}

void MaxNotifier::DebugPrintToFile(FILE* file, size_t indent)const
{
    if (NULL == file){
		DbgAssert(0 && _T("file is NULL"));
		return;
	}

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    _ftprintf(file, indentString);
    _ftprintf(file, _T("** Max Notifier : **\n"));

    ++indent;
    indentString += TSTR(_T("\t"));

    const size_t numListeners = m_callbacks.size();
    _ftprintf(file, indentString);
    _ftprintf(file, _T("** Number of callbacks : %llu **\n"), numListeners);
}   

BOOL MaxNotifier::IsRealDependency(ReferenceTarget * )
{
    // Weak reference
    return FALSE;
} 

bool MaxNotifier::IsAnyCallbackRegistered() const
{
    return !m_callbacks.empty();
}

void MaxNotifier::NotifyEvent(const IGenericEvent& genericEvent)
{
    // Don't propagate notifications that result from deleting the references made by this notifier
    if(!m_deleting_this)
    {
        // Set the flag that says we're currently processing an event, but restore the previous value to ensure that we correctly handle recursive calls
        // to this function (which I guess is possible, as we don't know when to expect reference notifications from Max)
        const bool old_m_currently_notifying_callbacks = m_currently_notifying_callbacks;
        m_currently_notifying_callbacks = true;

        for(const CallbackInfo& callback_info : m_callbacks)
        {
            if(callback_info.callback != nullptr)
            {
                const bool notifications_enabled = (callback_info.notification_client != nullptr) && callback_info.notification_client->NotificationsEnabled();
                if(notifications_enabled)
                {
                    callback_info.callback->NotificationCallback_NotifyEvent(genericEvent, callback_info.callback_user_data);
                }
            }
        }

        // Restore this flag to its previous value
        m_currently_notifying_callbacks = old_m_currently_notifying_callbacks;

        // If done notifying callbacks, then process any deferred registrations or unregistrations
        if(!m_currently_notifying_callbacks)
        {
            // Process deferred unregistrations
            if(m_callbacks_to_be_deferred_deleted)
            {
                for(CallbackList::iterator it = m_callbacks.begin(); it != m_callbacks.end(); )
                {
                    if(it->callback == nullptr)
                    {
                        it = m_callbacks.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }

                m_callbacks_to_be_deferred_deleted = false;
            }

            // Process deferred registrations
            for(const CallbackInfo& deferred_callback : m_deferred_added_callbacks)
            {
                RegisterEventCallback(*deferred_callback.notification_client, *deferred_callback.callback, deferred_callback.callback_user_data, deferred_callback.monitored_events);
            }
            m_deferred_added_callbacks.clear();
        }
    }
}

void MaxNotifier::RegisterEventCallback(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    if(!m_currently_notifying_callbacks)
    {
        // Check if the given callback is already registered
        for(CallbackInfo& callback_info : m_callbacks)
        {
            if((callback_info.notification_client == &notification_client)
                && (callback_info.callback == &callback)
                && (callback_info.callback_user_data == callback_user_data))
            {
                // Callback already registered - add new event types to monitor
                callback_info.monitored_events |= monitored_events;
                return;
            }
        }

        // The callback wasn't already registered: so register it
        m_callbacks.push_back(CallbackInfo(notification_client, callback, callback_user_data, monitored_events));
    }
    else
    {
        // An event is currently being processed: we can't add the new callback to the list, as the new registration shouldn't be taken
        // into account by the current event. Instead, we save it here and add it once the event is done being processed.
        m_deferred_added_callbacks.push_back(CallbackInfo(notification_client, callback, callback_user_data, monitored_events));
    }
}

void MaxNotifier::UnregisterEventCallback(ImmediateNotificationClient& notification_client, INotificationCallback& callback, void* const callback_user_data, const size_t monitored_events)
{
    // Deletions need to be deferred if the list of callbacks is currently being processed by a method lower in the call stack. Otherwise
    // we'd corrupt the loop being processed by that method.
    const bool defer_deletions = m_currently_notifying_callbacks;

    const bool callback_found = 
        // Unregister callback from the normal callbacks list
        UnregisterEventCallbackFromList(m_callbacks, defer_deletions, notification_client, callback, callback_user_data, monitored_events)
        // Also unregister callbacks from the deferred additions list, just in case we get into a weird corner case where the callback is deleted before
        // the notification event is done being processed.
        || UnregisterEventCallbackFromList(m_deferred_added_callbacks, false, notification_client, callback, callback_user_data, monitored_events);      

    // If the callback weren't found, we'd be trying to unregister a non-existing callback, and that would probably be a bug.
    DbgAssert(callback_found);

    if(callback_found)
    {
        // Remember that we need to processed the deferred deletions, once the event is done being processed.
        m_callbacks_to_be_deferred_deleted = true;
    }
}

bool MaxNotifier::UnregisterEventCallbackFromList(
    CallbackList& callback_list, 
    const bool defer_deletions,
    ImmediateNotificationClient& notification_client, 
    INotificationCallback& callback, 
    void* const callback_user_data, 
    const size_t monitored_events)
{
    bool callback_found = false;

    // Check if the given callback is already registered
    for(CallbackList::iterator it = callback_list.begin(); it != callback_list.end(); )
    {
        bool erase_callback = false;
        CallbackInfo& callback_info = *it;
        if((callback_info.notification_client == &notification_client)
            && (callback_info.callback == &callback)
            && (callback_info.callback_user_data == callback_user_data))
        {
            callback_found = true;

            // Callback already registered - remove event types to un-monitor
            callback_info.monitored_events &= ~monitored_events;

            // If no events left, then unregister this callback completely
            if(callback_info.monitored_events == 0)
            {
                if(!defer_deletions)
                {
                    erase_callback = true;
                }
                else
                {
                    // We are currently in the process of iterating through the list of callbacks to send out an event notification.
                    // We therefore can't delete the entry from the list, so instead we flag it to be deleted after the event processing is finished.
                    callback_info.callback = nullptr;
                }
            }
        }

        it = erase_callback ? callback_list.erase(it) : ++it;
    }

    return callback_found;
}

void MaxNotifier::UnregisterAllCallbacksForClient(ImmediateNotificationClient& notification_client)
{
    // It's assumed that this method will never be called while there are deferred callbacks left unregistered. If that were the case, then 
    // these deferred callbacks may be left registered (and dangling), so that would be wrong.
    DbgAssert(m_deferred_added_callbacks.empty());

    for(CallbackList::iterator it = m_callbacks.begin(); it != m_callbacks.end(); )
    {
        if(it->notification_client == &notification_client)
        {
            it = m_callbacks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool MaxNotifier::DeletingThis() const
{
    return m_deleting_this;
}

} } // namespaces
