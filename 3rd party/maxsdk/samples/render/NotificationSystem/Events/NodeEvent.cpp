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

#include "NodeEvent.h"

// src/include
#include "../NotificationAPIUtils.h"
// Max sdk
#include <inode.h>


namespace Max{

namespace NotificationAPI{
;


//==================================================================================================
// class NodeEvent
//==================================================================================================

NodeEvent::NodeEvent(NotifierType notifierType, NodeEventType updateType, INode* pNode)
{
	m_NotififerType             = notifierType;
	m_EventType	                = updateType;
	DbgAssert(NULL != pNode);
	m_pNode			            = pNode;
}

void NodeEvent::DebugPrintToFile(FILE* /*file*/)const
{
}

IMergeableEvent::MergeResult NodeEvent::merge_from(IMergeableEvent& generic_old_event)
{
    // Check if both events refer to the same node
    NodeEvent* old_event = dynamic_cast<NodeEvent*>(&generic_old_event);
    if((old_event != nullptr) && (GetNode() == old_event->GetNode()) && (GetEventType() == old_event->GetEventType()))
    {
        if(GetEventType() == EventType_Node_ParamBlock)
        {
            NodeParamBlockEvent* const this_pb_event = dynamic_cast<NodeParamBlockEvent*>(this);
            NodeParamBlockEvent* const old_pb_event = dynamic_cast<NodeParamBlockEvent*>(old_event);
            if(DbgVerify((this_pb_event != nullptr) && (old_pb_event != nullptr)))
            {
                // Merge all param block change data into the old/existing event
                Utils::MergeParamBlockDatas(old_pb_event->GetParamBlockData(), this_pb_event->GetParamBlockData());
            }

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

//==================================================================================================
// class NodeParamBlockEvent
//==================================================================================================

NodeParamBlockEvent::NodeParamBlockEvent(NotifierType notifierType, NodeEventType updateType, INode* pNode, const ParamBlockData& paramblockData)
    : NodeEvent(notifierType, updateType, pNode)
{
    m_ParamsChangedInPBlock.append(paramblockData);
}

const MaxSDK::Array<ParamBlockData>& NodeParamBlockEvent::GetParamBlockData() const
{
    return m_ParamsChangedInPBlock;
}

MaxSDK::Array<ParamBlockData>& NodeParamBlockEvent::GetParamBlockData()
{
    return m_ParamsChangedInPBlock;
}

//==================================================================================================
// class NodePropertyEvent
//==================================================================================================

NodePropertyEvent::NodePropertyEvent(NotifierType notifierType, NodeEventType eventType, INode* pNode, const PartID part_id)
    : NodeEvent(notifierType, eventType, pNode),
    m_part_id(part_id)
{

}

PartID NodePropertyEvent::GetPropertyID() const 
{
    return m_part_id;
}


};//end of namespace NotificationAPI
};//end of namespace Max
