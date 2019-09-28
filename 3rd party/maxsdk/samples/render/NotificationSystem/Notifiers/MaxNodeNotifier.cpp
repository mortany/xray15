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

#include "MaxNodeNotifier.h"

#include "Events/NodeEvent.h"
#include "MaxSceneNotifier.h"
#include "NotificationManager.h"

#include "../NotificationAPIUtils.h"

// Max SDK
#include <inode.h>
#include <notify.h>
#include <object.h>
#include <iparamb.h>
#include <xref/iXrefItem.h>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;



MaxNodeNotifier::MaxNodeNotifier(INode& pNode, const NotifierType notifier_type) 
    : m_notifier_type(notifier_type)
{
    // Only a few types of notifiers map to node notifiers
    DbgAssert(
        (m_notifier_type == NotifierType_Node_Camera) 
        || (m_notifier_type == NotifierType_Node_Light) 
        || (m_notifier_type == NotifierType_Node_Geom) 
        || (m_notifier_type == NotifierType_Node_Helper));

    m_Node = NULL;
    m_ReplacingReference = true;
    {
        // Suspend undo to prevent an undo operation from setting our reference to null, making us believe that the referenced object has been deleted
        HoldSuspend hold_suspend;
        ReplaceReference(0, &pNode);//Does at some point m_Node = pNode;
    }
    m_ReplacingReference = false;
    m_TransformUpdateHasJustBeenCalled = false;

    // Register for notifications that potentially affect this node
    RegisterNotification(ProcessNotifications, this, NOTIFY_NODE_HIDE);
    RegisterNotification(ProcessNotifications, this, NOTIFY_NODE_UNHIDE);
}

MaxNodeNotifier::~MaxNodeNotifier()
{
    // Unregister the notifications
    UnRegisterNotification(ProcessNotifications, this, NOTIFY_NODE_HIDE);
    UnRegisterNotification(ProcessNotifications, this, NOTIFY_NODE_UNHIDE);
}

INode* MaxNodeNotifier::GetMaxNode() const
{
    return m_Node;
}

Object* MaxNodeNotifier::s_GetObject(INode *node, const SClass_ID& superClassID)
{
	if (NULL == node) {
		return NULL;
	}

    //Almost copied from src\app\View3D.cpp
    //Function : CameraObject *GetCameraObject(INode *node)

	ReferenceTarget* rt = node->GetObjectRef();

	//see if the camera is an xref
	if (rt && IXRefItem::IsIXRefItem(*rt)) 
	{
		//iterate until we are not an xref
		while (rt && IXRefItem::IsIXRefItem(*rt))
		{
			//if we are derived we need to dive down to the base object
			if (rt && rt->SuperClassID()==GEN_DERIVOB_CLASS_ID) 			
				rt = ((Object*)rt)->FindBaseObject();

			//check to see if we are an xref if so go down a level
			if (rt && IXRefItem::IsIXRefItem(*rt)) 
			{
				IXRefItem* xi = IXRefItem::GetInterface(*rt);
				rt = xi->GetSrcItem();
			}			

		}
	}
	else //it is not we just dive to the base objects
	{
		if (rt && rt->SuperClassID()==GEN_DERIVOB_CLASS_ID) 
		{
			rt = ((Object*)rt)->FindBaseObject();
		}
	}


	if (rt && rt->SuperClassID()==superClassID) 
	{
		return (Object*)rt;
	} 
	else 
	{
		return NULL;
	}
}

RefResult MaxNodeNotifier::NotifyRefChanged(
    const Interval& /*changeInt*/, 
    RefTargetHandle hTarget, 
    PartID& partID, 
    RefMessage message,
    BOOL /*propagate */)
{
    // MakeRefRestore::DoRestore() sets the reference to null before broadcasting a change notification, so this is a valid call to be ignored.
    // (although I don't understand why MaxRefRestore behaves like that... it sure seems weird).
    if(m_Node == nullptr)
    {
        return REF_DONTCARE;
    }

    Control* pTMControl  = m_Node->GetTMController();
    Mtl* pMtl            = m_Node->GetMtl();
    Object* pObjRef      = m_Node->GetObjectRef();

    if ( hTarget != m_Node && hTarget != (RefTargetHandle)pTMControl && hTarget != pMtl && hTarget != pObjRef){
        return REF_DONTCARE;
    }

    switch(message)
    {
    case REFMSG_TARGET_DELETED:
        // The reference system expects us to react to REFMSG_TARGET_DELETED by setting the reference to null.
        SetReference(0, nullptr);
        break;
    case REFMSG_NODE_MATERIAL_CHANGED:
        NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Material_Replaced, m_Node));
        break;

    case REFMSG_DISPLAY_MATERIAL_CHANGE:
        switch(partID)
        {
        case PART_TEXMAP:
        case PART_MTL:
            NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Material_Updated, m_Node));
            break;
        case PART_DISPLAY:
        default:
            //Ignore
            break;
        }
        break;

    case REFMSG_SUBANIM_STRUCTURE_CHANGED:
        if (hTarget == (RefTargetHandle)pTMControl)
            NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Transform, m_Node));
        else if (hTarget == pObjRef)
            NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Uncategorized, m_Node));
        else if (hTarget == pMtl)
            NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Material_Updated, m_Node));

        //Default, subanim structure changed
        NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Reference, m_Node));
        break;

    case REFMSG_CHANGE:
        if (partID  == PART_ALL)
        {
            if (m_TransformUpdateHasJustBeenCalled)
            {
                //We have just received a PART_TM message, so ignore this PART_ALL that we always receive just after
                m_TransformUpdateHasJustBeenCalled = false; 
            }
            else if(pObjRef != nullptr)
            {
                ParamBlockData pblockData;
                pblockData.m_ParamBlockType = ParamBlockData::UNKNOWN_PB_TYPE;
                pblockData.m_ParamBlockIndexPath.removeAll();
                pblockData.m_ParametersIDsUpdatedInThatParamBlock.removeAll();

                bool bRes = false;
                Utils::GetLastUpdatedParamFromObject(bRes, *pObjRef, pblockData);//bRes is filled by the function

                if (bRes && pblockData.m_ParamBlockIndexPath.length() > 0 && pblockData.m_ParametersIDsUpdatedInThatParamBlock.length() > 0)
                {
                    //We know what changed in the pblock
                    NotifyEvent(NodeParamBlockEvent(m_notifier_type, EventType_Node_ParamBlock, m_Node, pblockData));
                }
                else
                {
                    // We don't know what changed: send a generic event
                    NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Uncategorized, m_Node));
                }
            }
        }
        else
        {
            m_TransformUpdateHasJustBeenCalled = false;//Reset our variable

            if (partID & PART_TM)
            {
                NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Transform, m_Node));
                m_TransformUpdateHasJustBeenCalled = true; //We are going to receive a PART_ALL msg after the PART_TM, so we will ignore it
            }

            if (partID & PART_GEOM)
            {
                NotifyEvent(NodeEvent(m_notifier_type, EventType_Mesh_Vertices, m_Node));
            }

            if (partID  & PART_TOPO)
            {
                NotifyEvent(NodeEvent(m_notifier_type, EventType_Mesh_Faces, m_Node));
            }

            if (partID & TEXMAP_CHANNEL)
            {
                NotifyEvent(NodeEvent(m_notifier_type, EventType_Mesh_UVs, m_Node));
            }
        }
        break;
    case REFMSG_NODE_WIRECOLOR_CHANGED:
        NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_WireColor, m_Node));
        break;
    case REFMSG_TARGET_SELECTIONCHANGE:
        NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Selection, m_Node));
        break;
    case REFMSG_NODE_GI_PROP_CHANGED:
        NotifyEvent(NodePropertyEvent(m_notifier_type, EventType_Node_GIProperty, m_Node, partID));
        break;
    case REFMSG_NODE_RENDERING_PROP_CHANGED:
        NotifyEvent(NodePropertyEvent(m_notifier_type, EventType_Node_RenderProperty, m_Node, partID));
        break;
    case REFMSG_NODE_DISPLAY_PROP_CHANGED:
        NotifyEvent(NodePropertyEvent(m_notifier_type, EventType_Node_DisplayProperty, m_Node, partID));
        // Handle 'hidden' property with specific event
        if(partID & PART_DISP_PROP_IS_HIDDEN)
        {
            NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Hide, m_Node));
        }
        break;
    default:
        break;
    }

    return REF_SUCCEED;	
}

int MaxNodeNotifier::GetUpdatedParamIDFromCamera()const
{
    int lastParamID = -1;

    Object* obj = s_GetObject(m_Node, CAMERA_CLASS_ID);
    if (obj){
        IParamBlock* pblock1 = dynamic_cast<IParamBlock*>(obj->GetReference(0));
        if (pblock1){
            lastParamID = pblock1->LastNotifyParamNum();
        }
    }

    return lastParamID;
}

int MaxNodeNotifier::NumRefs()
{
    return 1;
}

RefTargetHandle MaxNodeNotifier::GetReference(int )
{
    return m_Node; //May be NULL
}

void MaxNodeNotifier::SetReference(int , RefTargetHandle rtarg)
{
    if(m_Node != rtarg)
    {
        INode* const old_node = m_Node;
        DbgAssert((m_Node == nullptr) || (rtarg == nullptr));        // not supposed to change the node to which we're referencing
        m_Node = dynamic_cast<INode*>(rtarg);

        // Don't propagate notifications that result from deleting the references made by this notifier
        if(!DeletingThis())
        {
            if(m_Node == nullptr)
            {
                // Non-null to null: The reference was destroyed - notify that this object was deleted from the scene.
                // Use the old node in the notification, to avoid sending out a null value.
                NotifyEvent(NodeEvent(m_notifier_type, EventType_Node_Deleted, old_node));
                NotificationManager::GetInstance().Notify_SceneNodeRemoved(*old_node);
            }
            else if(!m_ReplacingReference)
            {
                // Null to non-null: Node reference was restored - probably the result of an undo operation. Notify that a node was (re)added to the scene.
                if(m_Node != nullptr)
                {
                    NotificationManager::GetInstance().Notify_SceneNodeAdded(*m_Node);
                }
            }
        }
    }
}

void MaxNodeNotifier::DebugPrintToFile(FILE* file, size_t indent)const
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
    _ftprintf(file, _T("** Node Notifier data : **\n"));

    if (m_Node){
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Node : \"%s\" **\n"), m_Node->GetName() );
    }else{
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Node : None **\n"));
    }

     //Print base class first
    __super::DebugPrintToFile(file, ++indent);

}

void MaxNodeNotifier::ProcessNotifications(void* const param, NotifyInfo* const info)
{
    MaxNodeNotifier* const notifier = static_cast<MaxNodeNotifier*>(param);
    switch(info->intcode)
    {
    case NOTIFY_NODE_HIDE:
    case NOTIFY_NODE_UNHIDE:
        if(info->callParam == notifier->m_Node)
        {
            notifier->NotifyEvent(NodeEvent(notifier->m_notifier_type, EventType_Node_Hide, notifier->m_Node));
        }
        break;
    default:
        DbgAssert(false);   // unhandled notification?
        break;
    }
}

NotifierType MaxNodeNotifier::GetNotifierType() const
{
    return m_notifier_type;
}

} } // namespaces
