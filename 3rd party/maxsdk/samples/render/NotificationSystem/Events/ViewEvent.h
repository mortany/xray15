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

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

class ViewEvent : public IViewEvent, public IMergeableEvent
{
public:

    // All view events should be created through these helper methods
    static ViewEvent MakeViewPropertiesEvent(INode* const view_node, const int view_id);
    static ViewEvent MakeViewActiveEvent(const int view_id);
    static ViewEvent MakeViewDeletedEvent(INode* const view_node, const int view_id) ;
    static ViewEvent MakeViewTypeEvent(const int view_id);
        
    ViewEvent(const ViewEvent& from);
	virtual ~ViewEvent();

    //From IGenericEvent
	virtual NotifierType    GetNotifierType	(void)const{return NotifierType_View;};
	virtual size_t		    GetEventType	(void)const{return m_EventType;};
    virtual void		    DebugPrintToFile(FILE* f)const;

    //From IViewEvent
    virtual bool ViewIsACameraOrLightNode()const override;
    virtual INode* GetViewCameraOrLightNode()const override;
    virtual ViewExp* GetView() const override;
    virtual int GetViewID() const override;

    // -- inherited from IMergeableEvent
    virtual MergeResult merge_from(IMergeableEvent& old_event);

protected:

    // Protected constructor so only derived classes may build this
    ViewEvent(const ViewEventType event_type, INode* const view_node, const int view_id);

private:

    // Disable assignment operator
    ViewEvent& operator=(const ViewEvent&);

private:

    const ViewEventType m_EventType;
    INode* const m_CameraOrLightNode;//May be NULL if it's a viewport and not a camera or a light view
    const int m_view_id; //If >=0 then you must use this ID to update active shade rendering to this viewport, if -1 then ou can get the current active viewport
};

};//end of namespace NotificationAPI
};//end of namespace Max