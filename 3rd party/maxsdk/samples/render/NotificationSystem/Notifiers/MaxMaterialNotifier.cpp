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

#include "MaxMaterialNotifier.h"

#include "Events/MaterialEvent.h"

#include <imtl.h>
#include "../NotificationAPIUtils.h"

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;


MaxMaterialNotifier::MaxMaterialNotifier(Mtl& pMaterial) 
{
    m_Material = NULL;
    {
        // Suspend undo to prevent an undo operation from setting our reference to null, making us believe that the referenced object has been deleted
        HoldSuspend hold_suspend;
        ReplaceReference(0, &pMaterial);//Does at some point m_Material = pMaterial;
    }
}

MaxMaterialNotifier::~MaxMaterialNotifier()
{
    // Reference should have been set to null by MaxNotifier::delete_this()
    DbgAssert(m_Material == nullptr);
}

Mtl* MaxMaterialNotifier::GetMaxMaterial() const
{
    return m_Material;
}

int MaxMaterialNotifier::NumRefs()
{
    return 1;
}

RefTargetHandle MaxMaterialNotifier::GetReference(int )
{
    return m_Material; //May be NULL
}

RefResult MaxMaterialNotifier::NotifyRefChanged(
			const Interval& /*changeInt*/, 
			RefTargetHandle hTarget, 
			PartID& partID, 
			RefMessage message,
            BOOL /*propagate */)
{
    //Is this what we are monitoring ?
    if (hTarget != m_Material || ! m_Material){
        return REF_DONTCARE;
    }

    switch(message)
    {
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
                Utils::GetLastUpdatedParamFromObject(bRes, *m_Material, pblockData);//bRes is filled by the function
                
                if (bRes && pblockData.m_ParamBlockIndexPath.length() > 0 && pblockData.m_ParametersIDsUpdatedInThatParamBlock.length() > 0)
                {
                    //We know what changed in the pblock
                    NotifyEvent(MaterialEvent(EventType_Material_ParamBlock, m_Material, &pblockData));
                }
                else
                {
                    // We don't know what changed: send a generic event
                    NotifyEvent(MaterialEvent(EventType_Material_Uncategorized, m_Material, nullptr));
                }
            }
            else
            {
                NotifyEvent(MaterialEvent(EventType_Material_Uncategorized, m_Material, nullptr));
            }
		    break;
        case REFMSG_SUBANIM_STRUCTURE_CHANGED:
			//Default, subanim structure changed
            if (0 == partID){
                NotifyEvent(MaterialEvent(EventType_Material_Reference, m_Material, nullptr));
            }
        break;
	}

	return REF_SUCCEED;	
}

void MaxMaterialNotifier::SetReference(int , RefTargetHandle rtarg)
{
    if(m_Material != rtarg)
    {
        Mtl* const old_mtl = m_Material;
        DbgAssert((m_Material == nullptr) || (rtarg == nullptr));        // not supposed to change the node to which we're referencing
        m_Material = dynamic_cast<Mtl*>(rtarg);

        if(m_Material == nullptr)
        {
            // Non-null to null: The reference was destroyed - notify that this object was deleted from the scene.
            // Use the old material in the notification, to avoid sending out a null value.
            NotifyEvent(MaterialEvent(EventType_Material_Deleted, old_mtl, nullptr));
        }
    }
}

void MaxMaterialNotifier::DebugPrintToFile(FILE* file, size_t indent)const
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
    _ftprintf(file, _T("** Material Notifier data : **\n"));

    if (m_Material){
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Material : \"%s\" **\n"), m_Material->GetName().data() );
    }else{
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Material : None **\n"));
    }

    //Print base class first
   __super::DebugPrintToFile(file, ++indent);
}

} } // namespaces
