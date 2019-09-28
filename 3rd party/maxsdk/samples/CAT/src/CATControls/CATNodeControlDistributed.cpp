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

#include "CATNodeControlDistributed.h"
#include <CATAPI/IBoneGroupManager.h>

// When a SetValue is piped in, we need to detect if it should
// be shared among all selected items of this body part.
// (This is rotation only, other stuff can be done for this later).
void CATNodeControlDistributed::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	// If we are being called, we can guarantee
	// that we have no further ancestors selected (the SetValue is valid)
	SetXFormPacket* xForm = (SetXFormPacket*)val;
	if (IsThisBoneSelected())
	{
		switch (xForm->command)
		{
		case XFORM_MOVE:
		{
			// We only wanted to apply the effect to the
			// first selected bone in the group.
			int iBoneID = GetBoneID();

			if (iBoneID > 0)
			{
				CATNodeControl* pParent = GetParentCATNodeControl();
				if (pParent != NULL && pParent->IsThisBoneSelected())
				{
					// If we have a parent selected, the only reason to
					// process this SetValue now is to stretch to
					// meet a child (if necessary)
					if (CATNodeControl::IsStretchy(GetCATMode()))
					{
						IBoneGroupManager* pManager = GetManager();;
						// This class can only be implemented on classes that have a data class.
						DbgAssert(pManager != NULL);
						if (pManager == NULL)
							return;

						// If the following condition is true, we stretch
						INodeControl* pChild = pManager->GetBoneINodeControl(iBoneID + 1);
						if (pChild != NULL && !pChild->IsThisBoneSelected())
							break;
					}
					return;
				}
			}

			break;
		}
		case XFORM_ROTATE:
		{
			// We don't want to apply the full effect to every bone
			int nBoneSel = NumSelectedBones();
			if (nBoneSel != 0)
			{
				xForm->aa.angle /= nBoneSel;
			}
			break;
		}
		case XFORM_SCALE:
		{
			// TODO:
		}
		}
	}

	CATNodeControl::SetValue(t, val, commit, method);
}

BOOL CATNodeControlDistributed::InheritsParentTransform()
{
	// If we have a non-tail ancestor selected,
	// then yes, we do inherit their transform
	if (IsAncestorSelected(0))
		return TRUE;

	// Otherwise, the tail distributes the SetValue over
	// all tail bones.  To do this, we return that we
	// do not, in fact, inherit transforms...
	return FALSE;
}

bool CATNodeControlDistributed::IsAncestorSelected(int iBoneID)
{
	IBoneGroupManager* pManager = GetManager();;
	// This class can only be implemented on classes that have a data class.
	DbgAssert(pManager != NULL);
	if (pManager == NULL)
		return false;

	// Double check we are hooked up correctly
	DbgAssert(pManager->GetBoneINodeControl(GetBoneID()) == this);
	INodeControl* pBone = pManager->GetBoneINodeControl(iBoneID);
	INode* pNode = pBone->GetNode();

	// Are any of the ancestors of this bone selected
	while ((pNode = pNode->GetParentNode()) != NULL)
	{
		if (pNode->Selected())
			return true;
	}
	return false;
}

int CATNodeControlDistributed::NumSelectedBones(int iBoneID/*=-1*/)
{
	IBoneGroupManager* pManager = GetManager();
	// This class can only be implemented on classes that have a data class.
	DbgAssert(pManager != NULL);
	if (pManager == NULL)
		return false;

	// Keep our ID in range
	int nBones = pManager->GetNumBones();
	if (iBoneID >= nBones || iBoneID < 0)
		iBoneID = nBones - 1;

	int nBoneSel = 0;
	for (int i = 0; i <= iBoneID; i++)
	{
		INodeControl* pTailBone = pManager->GetBoneINodeControl(i);
		if (pTailBone != NULL && pTailBone->IsThisBoneSelected())
			nBoneSel++;
	}
	return nBoneSel;
}
