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

#include <NotificationAPI/NotificationAPI_Events.h>
#include "IMergeableEvent.h"
#include <noncopyable.h>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

    class SceneNodeEvent : 
        public ISceneNodeEvent, 
        public IMergeableEvent,
        public MaxSDK::Util::Noncopyable
	{
	public:

        static SceneNodeEvent MakeNodeAddedEvent(INode* const node);
        static SceneNodeEvent MakeNodeRemovedEvent(INode* const node);

        SceneNodeEvent(const SceneNodeEvent& from);
		virtual ~SceneNodeEvent();

        // -- inherited from IGenericEvent
		virtual NotifierType		GetNotifierType	(void)const{return NotifierType_SceneNode;};
		virtual size_t			    GetEventType	(void)const{return m_EventType;};
        virtual void		        DebugPrintToFile(FILE* f)const;
        
        // -- inherited from ISceneNodeEvent
        virtual INode* GetNode() const override;

        // -- inherited from IMergeableEvent
        virtual MergeResult merge_from(IMergeableEvent& old_event);

    protected:

        // Constructor protected: use the static methods to create an event
        SceneNodeEvent(SceneEventType eventType, INode* const node);

    private:

        const SceneEventType m_EventType;
        INode* const m_node;
	};

};//end of namespace NotificationAPI
};//end of namespace Max