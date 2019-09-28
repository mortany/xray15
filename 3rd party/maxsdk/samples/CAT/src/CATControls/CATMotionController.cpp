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

#include "CATMotionController.h"
#include "CATNodeControl.h"

#include "CATHierarchyLeaf.h"

RefResult CATMotionController::AutoDelete()
{
	DestructCATMotionHierarchy(NULL);
	return Control::AutoDelete();
}

void CATMotionController::DestructCATMotionHierarchy(LimbData2* pLimb)
{
	Control* pParentCtrl = GetOwningCATMotionController();
	if (pParentCtrl == NULL)
		return;

	// THE FOLLOWING LINES SHOULD BE MADE OBSOLETE BY P_NO_REF WORK
	//IParamBlock2* pParentParams = pParentCtrl->GetParamBlock(0);
	//if (pParentParams != NULL)
	//	NullPointerInParamBlock(pParentParams, this);

	// NULL our limb pointer, as it's not referenced either...
	//NullNoRefInParamBlock();

	IParamBlock2* pb2 = GetParamBlock(0);
	int nParams = pb2->NumSubs();
	for (int i = 0; i < nParams; i++)
	{
		Animatable* pTarg = pb2->SubAnim(i);
		CATHierarchyLeaf* pLeaf = dynamic_cast<CATHierarchyLeaf*>(pTarg);
		if (pLeaf != NULL)
			pLeaf->MaybeDestructLeaf(pLimb);

		CATMotionController* pSubCtrl = dynamic_cast<CATMotionController*>(pTarg);
		if (pSubCtrl != NULL)
			pSubCtrl->DestructCATMotionHierarchy(pLimb);
	}
}

void CATMotionController::NullNoRefInParamBlock()
{
	// Find the layer that
	// We are only cleaning it's unref'ed pointers
	IParamBlock2* pMyParams = GetParamBlock(0);
	DbgAssert(pMyParams != NULL);
	if (pMyParams == NULL)
		return;

	for (int i = 0; i < pMyParams->NumParams(); i++)
	{
		ParamID pid = pMyParams->IndextoID(i);
		ParamDef& def = pMyParams->GetParamDef(pid);

		if (reftarg_type(def.type) && (def.flags&P_NO_REF))
		{
			if (is_tab(def.type))
				pMyParams->SetCount(pid, 0);
			else
				pMyParams->SetValue(pid, 0, (ReferenceTarget*)NULL);
		}
	}
}