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

#include "MaxTexmapNotifier.h"

#include "Events/TexmapEvent.h"

#include "../NotificationAPIUtils.h"

// Max SDK
#include <imtl.h>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

;

MaxTexmapNotifier::MaxTexmapNotifier(Texmap& pTexmap)
{
    m_Texmap = NULL;
    {
        // Suspend undo to prevent an undo operation from setting our reference to null, making us believe that the referenced object has been deleted
        HoldSuspend hold_suspend;
        ReplaceReference(0, &pTexmap);//Does at some point m_BaseMaterial = pMaterial;
    }
}

MaxTexmapNotifier::~MaxTexmapNotifier()
{
    // Reference should have been set to null by MaxNotifier::delete_this()
    DbgAssert(m_Texmap == nullptr);
}

Texmap* MaxTexmapNotifier::GetMaxTexmap() const
{
    return m_Texmap;
}

int MaxTexmapNotifier::NumRefs()
{
    return 1;
}

RefTargetHandle MaxTexmapNotifier::GetReference(int )
{
    return m_Texmap; //May be NULL
}

RefResult MaxTexmapNotifier::NotifyRefChanged(
			const Interval& /*changeInt*/, 
			RefTargetHandle hTarget, 
			PartID& partID, 
			RefMessage message,
            BOOL /*propagate */)
{
    //Is this what we are monitoring ?
    if (hTarget != m_Texmap || ! m_Texmap){
        return REF_DONTCARE;
    }

    switch(message){
	case REFMSG_TARGET_DELETED:
        // The reference system expects us to react to REFMSG_TARGET_DELETED by setting the reference to null.
        SetReference(0, nullptr);
    	break;
    case REFMSG_CHANGE:
        if (partID == PART_ALL)
        {
            ParamBlockData pblockData;
            pblockData.m_ParamBlockType = ParamBlockData::UNKNOWN_PB_TYPE;
            pblockData.m_ParamBlockIndexPath.removeAll();
            pblockData.m_ParametersIDsUpdatedInThatParamBlock.removeAll();

            bool bRes = false;
            Utils::GetLastUpdatedParamFromObject(bRes, *m_Texmap, pblockData);//bRes is filled by the function

            if (bRes && pblockData.m_ParamBlockIndexPath.length() > 0 && pblockData.m_ParametersIDsUpdatedInThatParamBlock.length() > 0)
            {
                //We know what changed in the pblock
                NotifyEvent(TexmapEvent(EventType_Texmap_ParamBlock, m_Texmap, &pblockData));
            }
            else
            {
                // We don't know what changed: send a generic event
                NotifyEvent(TexmapEvent(EventType_Texmap_Uncategorized, m_Texmap, nullptr));
            }
        }
        else
        {
            NotifyEvent(TexmapEvent(EventType_Texmap_Uncategorized, m_Texmap, nullptr));
        }
	    break;
	}

	return REF_SUCCEED;	
}

void MaxTexmapNotifier::SetReference(int , RefTargetHandle rtarg)
{
    if(m_Texmap != rtarg)
    {
        Texmap* const old_texmap = m_Texmap;
        DbgAssert((m_Texmap == nullptr) || (rtarg == nullptr));        // not supposed to change the node to which we're referencing
        m_Texmap = dynamic_cast<Texmap*>(rtarg);

        if(m_Texmap == nullptr)
        {
            // Non-null to null: The reference was destroyed - notify that this object was deleted from the scene.
            // Use the old texmap in the notification, to avoid sending out a null value.
            NotifyEvent(TexmapEvent(EventType_Texmap_Deleted, old_texmap, nullptr));
        }
    }
}

void MaxTexmapNotifier::DebugPrintToFile(FILE* file, size_t indent)const
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
    _ftprintf(file, _T("** Texmap Notifier data : **\n"));
    
    //Print base class
   __super::DebugPrintToFile(file, ++indent);
}

} } // namespaces
