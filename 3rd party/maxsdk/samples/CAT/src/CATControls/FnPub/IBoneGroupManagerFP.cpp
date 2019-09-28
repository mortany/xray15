//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "IBoneGroupManagerFP.h"
#include "../CATNodeControl.h"

FPInterfaceDesc* IBoneGroupManagerFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* IBoneGroupManagerFP::GetFnPubDesc()
{
	// Descriptor
	static FPInterfaceDesc group_manager_FPinterface(
		BONEGROUPMANAGER_INTERFACE_FP, _T("CATGroupManagerFPInterface"), 0, NULL, FP_MIXIN,

		IBoneGroupManagerFP::fnGetBone, _T("GetBone"), 0, TYPE_INTERFACE, 0, 1,
			_T("index"), 0, TYPE_INDEX,

		properties,

		IBoneGroupManagerFP::propGetNumBones, IBoneGroupManagerFP::propSetNumBones, _T("NumBones"), 0, TYPE_INT,

		p_end
	);

	return &group_manager_FPinterface;
}

void IBoneGroupManagerFP::NotifyBoneDelete(CatAPI::INodeControl* pDeletedBone)
{
	// test if there are any valid bones left.
	// If there are, leave.
	int nBones = GetNumBones();
	for (int i = 0; i < nBones; i++)
	{
		CatAPI::INodeControl* pBone = GetBoneINodeControl(i);
		if (pBone != NULL && pBone != pDeletedBone)
			return;
	}

	// There were no valid bones - unlink the manager
	// Our parent is our manager, but we cannot cast
	// directly between the classes because
	// the multiple inheritance can screw up our Fn pointers.
	CATNodeControl* pThisAsNodeCtrl = dynamic_cast<CATNodeControl*>(static_cast<BaseInterface*>(this));
	if (pThisAsNodeCtrl != NULL)
	{
		MaxAutoDeleteLock lock(pThisAsNodeCtrl);
		pThisAsNodeCtrl->UnlinkCATHierarchy();
	}
	else
	{
		CATControl* pThisAsCATCtrl = dynamic_cast<CATControl*>(static_cast<BaseInterface*>(this));
		DbgAssert(pThisAsCATCtrl != NULL);
		if (pThisAsCATCtrl != NULL)
		{
			MaxAutoDeleteLock lock(pThisAsCATCtrl);
			pThisAsCATCtrl->DestructCATControlHierarchy();
		}
	}
}

BoneGroupPostLoadIDPatcher::BoneGroupPostLoadIDPatcher(CATControl* pOwner, ParamID paramIndex)
	: mpOwner(pOwner)
	, mOldIdParam(paramIndex)
{
	DbgAssert(pOwner != NULL);
	DbgAssert(paramIndex >= 0);
}

void BoneGroupPostLoadIDPatcher::proc(ILoad* load)
{
	if (HIWORD(load->GetFileSaveVersion()) <= MAX_RELEASE_R15)
	{
		// if the owner has already loaded an Id, ignore this one
		if (mpOwner->GetBoneID() == -1)
		{
			IParamBlock2* pParams = mpOwner->GetParamBlock(0);
			DbgAssert(pParams != NULL);
			if (pParams == NULL)
				return;

			DbgAssert(mOldIdParam < pParams->NumParams());
			DbgAssert(pParams->GetParamDef(mOldIdParam).type == TYPE_INDEX);
			int oldId = pParams->GetInt(mOldIdParam);
			if (oldId != -1)
			{
				mpOwner->SetBoneID(oldId);
			}
		}
	}
	delete this;
}

