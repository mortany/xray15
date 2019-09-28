//**************************************************************************/
// Copyright (c) 1998-2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Implementation
// AUTHOR: David Lanier
//***************************************************************************/
#include "ViewEvent.h"

// src/include
#include "../NotificationAPIUtils.h"
// Max sdk
#include <inode.h>
#include <NotificationAPI/NotificationAPIUtils.h>
// std includes
#include <time.h>

namespace Max{
namespace NotificationAPI{
;

//---------------------------------------------------------------------------
//----------			class ViewEvent				---------
//---------------------------------------------------------------------------

ViewEvent::ViewEvent(const ViewEventType event_type, INode* const view_node, const int view_id)
    : m_EventType(event_type),
    m_CameraOrLightNode(view_node),
    m_view_id(view_id)
{

}

ViewEvent::ViewEvent(const ViewEvent& from)
    : m_EventType(from.m_EventType),
    m_CameraOrLightNode(from.m_CameraOrLightNode),
    m_view_id(from.m_view_id)
{

}

ViewEvent::~ViewEvent()
{

}

void ViewEvent::DebugPrintToFile(FILE* file)const
{
    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }
    
    time_t ltime;
	time(&ltime);

    TSTR indentString = _T("");
    size_t indent = 1;
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    _ftprintf(file, indentString);
    _ftprintf(file, _T("\n** ViewEvent parameters : %s **\n"), _tctime(&ltime) );
    
    Utils::DebugPrintToFileEventType(file, GetNotifierType(), m_EventType, indent);
    
    _ftprintf(file, indentString);
    if (m_CameraOrLightNode){
		_ftprintf(file, _T("** CameraOrLightNode name : \"%s\" **\n"), m_CameraOrLightNode->GetName());
	}else{
		_ftprintf(file, _T("** No CameraOrLightNode (it's a viewport)**\n"));
	}

    _ftprintf(file, indentString);
    _ftprintf(file, _T("** view_id : %d **\n"), m_view_id);
}

ViewEvent ViewEvent::MakeViewPropertiesEvent(INode* const view_node, const int view_id)
{
    return ViewEvent(EventType_View_Properties, view_node, view_id);
}

ViewEvent ViewEvent::MakeViewActiveEvent(const int view_id)
{
    bool dummy;
    return ViewEvent(EventType_View_Active, Utils::GetViewNode(MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(dummy)), view_id);
}

ViewEvent ViewEvent::MakeViewTypeEvent(const int view_id)
{
    bool dummy;
    return ViewEvent(EventType_View_Type, Utils::GetViewNode(MaxSDK::NotificationAPIUtils::GetViewExpFromUndoIDIncludingExtendedViews(view_id, dummy)), view_id);
}


ViewEvent ViewEvent::MakeViewDeletedEvent(INode* const view_node, const int view_id) 
{
    return ViewEvent(EventType_View_Deleted, view_node, view_id);
}

ViewEvent::MergeResult ViewEvent::merge_from(IMergeableEvent& generic_old_event)
{
    ViewEvent* old_event = dynamic_cast<ViewEvent*>(&generic_old_event);
    if(old_event != nullptr) 
    {
        switch(m_EventType)
        {
        case EventType_View_Properties:
            // New events precede old ones
            return (old_event->m_EventType == m_EventType) ? MergeResult::Merged_KeepNew : MergeResult::NotMerged;
        case EventType_View_Active:
            // New events precede old ones
            return (old_event->m_EventType == m_EventType) ? MergeResult::Merged_KeepNew : MergeResult::NotMerged;
        case EventType_View_Deleted:
            // View deletion invalidates all previous events
            return MergeResult::Merged_KeepNew;
        default:
            DbgAssert(false);
            return MergeResult::NotMerged;
        }
    }
    else
    {
        return MergeResult::NotMerged;
    }
}

bool ViewEvent::ViewIsACameraOrLightNode() const
{
    return NULL != m_CameraOrLightNode;
}

INode* ViewEvent::GetViewCameraOrLightNode() const
{
    return m_CameraOrLightNode;
}

ViewExp* ViewEvent::GetView() const 
{
    bool dummy_bool = false;
    ViewExp* const view_exp = 
        (m_view_id < 0)
        ? MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(dummy_bool)
        : MaxSDK::NotificationAPIUtils::GetViewExpFromUndoIDIncludingExtendedViews(m_view_id, dummy_bool);
    return view_exp;
}

int ViewEvent::GetViewID() const 
{
    return m_view_id;
}


};//end of namespace NotificationAPI
};//end of namespace Max
