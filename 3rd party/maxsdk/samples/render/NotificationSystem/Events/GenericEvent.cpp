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

#include "GenericEvent.h"

// src/include
#include "../NotificationAPIUtils.h"
// std includes
#include <time.h>

namespace Max{

namespace NotificationAPI{

//---------------------------------------------------------------------------
//----------			class GenericEvent							---------
//---------------------------------------------------------------------------

GenericEvent::GenericEvent(NotifierType notififerType, size_t updateType)
{
	m_NotififerType = notififerType;
	m_EventType	    = updateType;
}

void GenericEvent::DebugPrintToFile(FILE* file)const
{
    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }
    
    time_t ltime;
	time(&ltime);
    _ftprintf(file, _T("** GenericEvent parameters : %s**\n"), _tctime(&ltime) );

    size_t indent = 1;
    Utils::DebugPrintToFileNotifierType(file, m_NotififerType, NULL, indent); //No known notifier at this level
    Utils::DebugPrintToFileEventType(file, m_NotififerType, m_EventType, indent);

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }
}

GenericEvent::MergeResult GenericEvent::merge_from(IMergeableEvent& generic_old_event)
{
    GenericEvent* const old_event = dynamic_cast<GenericEvent*>(&generic_old_event);
    if((old_event != nullptr) && (old_event->GetEventType() == GetEventType()))
    {
        return MergeResult::Merged_KeepNew;
    }
    else
    {
        return MergeResult::NotMerged;
    }
}


};//end of namespace NotificationAPI
};//end of namespace Max
