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
// DESCRIPTION: Notification API internal (private header)
// AUTHOR: David Lanier
//***************************************************************************/


#include "NotificationAPI/NotificationAPI_Subscription.h"
#include "NotificationAPI/NotificationAPI_Events.h"
#include "IMergeableEvent.h"

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

    class GenericEvent : public IGenericEvent, public IMergeableEvent
	{	
		NotifierType	m_NotififerType;
		size_t		    m_EventType;

	public:
		GenericEvent(NotifierType notififerType, size_t eventType);
		virtual ~GenericEvent(){};

		//Gives access to the type of notifier and the user data the plugin (usually renderer has added)
		virtual NotifierType	GetNotifierType	(void)const{return m_NotififerType;};
		virtual size_t		    GetEventType	(void)const{return m_EventType;};
        virtual void		    DebugPrintToFile(FILE* f)const;

        // -- inherited from IMergeableEvent
        virtual MergeResult merge_from(IMergeableEvent& old_event);
	};

};//end of namespace NotificationAPI
};//end of namespace Max