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

namespace Max
{;
namespace NotificationAPI
{;
    // This class exposes an internal interface to allow merging events, for the purpose of:
    // 1. Eliminating duplicate events
    // 2. Merging events which may be combined, for efficiency
    // 3. Eliminate events which conflict with each other
    class IMergeableEvent
    {
    public:

        enum class MergeResult
        {
            // Events weren't merged
            NotMerged,
            // Events were merged, the new event should be preserved and the old event should be discarded
            Merged_KeepNew,
            // Events were merged, the old event should be preserved and the new event should be discarded
            Merged_KeepOld,
            // Events nullify each other, both should be discarded
            DiscardBoth
        };

        // Merges an old event into 'this' new event (this == new event)
        virtual MergeResult merge_from(IMergeableEvent& old_event) = 0;
    };
	
};//end of namespace NotificationAPI
};//end of namespace Max