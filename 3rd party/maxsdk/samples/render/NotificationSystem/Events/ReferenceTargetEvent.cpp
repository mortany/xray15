//**************************************************************************/
// Copyright (c) 1998-2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//***************************************************************************/
#include "ReferenceTargetEvent.h"

// src/include
#include "../NotificationAPIUtils.h"
// Max sdk
#include <imtl.h>
// std includes
#include <time.h>


namespace Max{

namespace NotificationAPI{

//---------------------------------------------------------------------------
//----------			class ReferenceTargetEvent							---------
//---------------------------------------------------------------------------

ReferenceTargetEvent::ReferenceTargetEvent(ReferenceTargetEventType updateType, ReferenceTarget* refTarg, const ParamBlockData* paramblockData /*= NULL*/)
{
	m_EventType	                = updateType;
	DbgAssert(NULL != refTarg);
	m_refTarg		            = refTarg;
    m_ParamsChangedInPBlock.removeAll();
    if (NULL != paramblockData){
        m_ParamsChangedInPBlock.append(*paramblockData);
    }
}

void ReferenceTargetEvent::DebugPrintToFile(FILE* file)const
{
    if (! file){
        DbgAssert(0 && _T("file is NULL"));
        return;
    }
    
    time_t ltime;
	time(&ltime);

    _ftprintf(file, _T("\n** ReferenceTargetEvent parameters : %s **\n"), _tctime(&ltime) );

    size_t indent = 1;
    Utils::DebugPrintToFileNotifierType(file, GetNotifierType(), m_refTarg, indent);
    Utils::DebugPrintToFileEventType(file, GetNotifierType(), m_EventType, indent);

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }

    const size_t NumParams = m_ParamsChangedInPBlock.length();
    _ftprintf(file, indentString);
    _ftprintf(file, _T("** Number of Param IDs changed from pblock : %llu **\n"), NumParams);
    TSTR indentStringPlus = indentString + TSTR(_T("\t"));
    for (size_t iParam=0;iParam<NumParams;++iParam){
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** ParamblockData #%llu : **\n"), iParam);
        Utils::DebugPrintParamBlockData(file, m_ParamsChangedInPBlock[iParam], indent+1);
    }
}

ReferenceTargetEvent::MergeResult ReferenceTargetEvent::merge_from(IMergeableEvent& generic_old_event)
{
    // Check if both events refer to the same node
    ReferenceTargetEvent* old_event = dynamic_cast<ReferenceTargetEvent*>(&generic_old_event);
    if((old_event != nullptr) && (GetReferenceTarget() == old_event->GetReferenceTarget()) && (GetEventType() == old_event->GetEventType()))
    {
        if(GetEventType() == EventType_ReferenceTarget_ParamBlock)
        {
            // Merge all param block change data into the old/existing event
            Utils::MergeParamBlockDatas(old_event->GetParamBlockData(), GetParamBlockData());

            // Keep the old/existing event
            return MergeResult::Merged_KeepOld;
        }
        else
        {
            //Same node and same update type, so only store the last update.
            return MergeResult::Merged_KeepNew;
        }
    }

    return MergeResult::NotMerged;
}

};//end of namespace NotificationAPI
};//end of namespace Max
