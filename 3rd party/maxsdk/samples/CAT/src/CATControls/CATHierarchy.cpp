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

#include "CatPlugins.h"
#include "macrorec.h"

#include "CATHierarchyRoot.h"
#include "CATHierarchyBranch2.h"

#include "Hub.h"
#include "LimbData2.h"
#include "TailData2.h"
#include "PalmTrans2.h"

void CATHierarchyBranch::SetReference(int, RefTargetHandle rtarg)
{
	pblock = dynamic_cast<IParamBlock2*>(rtarg);
}

CATHierarchyBranch2* CATHierarchyBranch::NewCATHierarchyBranch2Controller()
{
	CATHierarchyBranch2* ctrl = (CATHierarchyBranch2*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATHIERARCHYBRANCH_CLASS_ID);
	ctrl->SetCATRoot(GetCATRoot());
	return ctrl;
}

// Gets an existing leaf if it exists
CATHierarchyBranch* CATHierarchyBranch::GetBranch(const TSTR& name)
{
	// do we already have a branch with this name?
	for (int i = 0; i < GetNumBranches(); i++)
	{
		if (0 == _tcsicmp(GetBranchName(i).data(), name.data()))
			return GetBranch(i);
	}
	return NULL;
}

// Gets an existing leaf if it exists
int CATHierarchyBranch::GetBranchIndex(CATHierarchyBranch* branch)
{
	for (int i = 0; i < GetNumBranches(); i++)
		if (branch == GetBranch(i))
			return i;
	return -1;
}

Control* CATHierarchyBranch::AddBranch(const TSTR& name, int index/*=-1*/)
{
	Control* oldBranch = GetBranch(name);
	if (oldBranch) return oldBranch;

	CATHierarchyBranch2* ctrl = (CATHierarchyBranch2*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATHIERARCHYBRANCH_CLASS_ID);
	if (ctrl) {
		int NumBranches = GetNumBranches();
		pblock->SetCount(PB_BRANCHTAB, NumBranches + 1);
		pblock->SetCount(PB_BRANCHNAMESTAB, NumBranches + 1);

		// shuffle everything else down
		if ((index >= 0) && (index < NumBranches))
		{
			for (int i = NumBranches - 1; i >= index; i--)
			{
				CATHierarchyBranch* branch = GetBranch(i);
				branch->SetBranchIndex(i + 1);
				SetBranch(i + 1, branch);
				SetBranchName(i + 1, pblock->GetStr(PB_BRANCHNAMESTAB, 0, i));
			}
		}
		else index = NumBranches;

		ctrl->SetCATRoot(GetCATRoot());
		ctrl->SetBranchParent(this);
		ctrl->SetBranchIndex(index);

		SetBranchName(index, name);
		SetBranch(index, ctrl);

		return ctrl;
	}

	return NULL;
}

Control* CATHierarchyBranch::AddBranch(ReferenceTarget* owner, int index/*=-1*/)
{
	// do we already have a branch for this hub?
	for (int i = 0; i < GetNumBranches(); i++)
	{
		CATHierarchyBranch* pBranch = GetBranch(i);
		if (pBranch != NULL && pBranch->GetBranchOwner() == owner)
			return (Control*)GetBranch(i);
	}

	CATHierarchyBranch2* ctrl = (CATHierarchyBranch2*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATHIERARCHYBRANCH_CLASS_ID);
	if (ctrl) {
		int NumBranches = GetNumBranches();
		pblock->SetCount(PB_BRANCHTAB, NumBranches + 1);
		pblock->SetCount(PB_BRANCHNAMESTAB, NumBranches + 1);

		// shuffle everything else down
		if ((index >= 0) && (index < NumBranches))
		{
			for (int i = NumBranches - 1; i >= index; i--)
			{
				CATHierarchyBranch* branch = GetBranch(i);
				branch->SetBranchIndex(i + 1);
				SetBranch(i + 1, branch);
				SetBranchName(i + 1, pblock->GetStr(PB_BRANCHNAMESTAB, 0, i));
			}
		}
		else index = NumBranches;

		ctrl->SetCATRoot(GetCATRoot());
		ctrl->SetBranchParent(this);
		ctrl->SetBranchIndex(index);
		ctrl->SetBranchOwner(owner);
		TSTR strBranchName = _T("");
		ctrl->SetBranchName(strBranchName);

		SetBranch(index, ctrl);
		return ctrl;
	}

	return NULL;
}

Control* CATHierarchyBranch::AddBranch(ReferenceTarget* owner, const TSTR& groupName, int index/*=-1*/)
{
	// do we already have a branch for this hub?
	CATHierarchyBranch2* ctrl = (CATHierarchyBranch2*)AddBranch(owner, index);
	if (ctrl) {
		ctrl->SetBranchOwner(owner);
		ctrl->SetBranchName(groupName);
		return ctrl;
	}
	return NULL;
}

void CATHierarchyBranch::SelfDestruct() {

	// We need this pointer...
	DbgAssert(pblock != NULL);
	if (pblock == NULL)
		return;

	// For safeties sake, remove any pointers from our children to us.
	for (int i = 0; i < GetNumBranches(); i++)
	{
		CATHierarchyBranch* pBranch = GetBranch(i);
		if (pBranch != NULL)
			pBranch->SetBranchParent(NULL);
	}
	pblock->SetCount(PB_BRANCHTAB, 0);

	// These iterations can delete the reference to ourselves.
	MaxAutoDeleteLock lock(this);

	CATHierarchyBranch* parent = GetBranchParent();
	// first, check to see if we have any siblings.  If not, then
	// we are the only reason our parent exists.  In this case,
	// just destruct our parent to prune the empty branch
	CATHierarchyBranch2* branchParent = dynamic_cast<CATHierarchyBranch2*>(parent);
	if (branchParent != NULL)
	{
		// If our parent has no children, then we might as well remove it from the list as well.
		if (branchParent->GetNumBranches() <= 1)
		{
			if (branchParent->GetNumLeaves() == 0)		// No values on this item
			{
				// Only 1 (or less) branches in the tree,
				// and that final branch is this branch
				// because we don't NULL our parent pointer,
				// it is possible for a parent continue
				// to receive Destruct calls after its
				// already destructed.
				if (branchParent->GetNumBranches() == 1 && branchParent->GetBranch(0) == this)
					branchParent->SelfDestruct();

				// Even if we don't destruct the parent, we know
				// that we have already been destructed, so just return
				return;
			}
		}
	}

	// remove this control from our parents branches
	// This is legal (we could be destructed twice)
	if (parent == NULL)
		return;

	int nNumParentBranches = parent->GetNumBranches();
	if (nNumParentBranches > 0) {
		for (int i = GetBranchIndex(); i < (nNumParentBranches - 1); i++)
		{
			CATHierarchyBranch* branch = parent->GetBranch(i + 1);
			if (branch != NULL)
				branch->SetBranchIndex(i);
		}
		DbgAssert(parent->GetBranchIndex(this) == GetBranchIndex());
		int mySafeIndex = parent->GetBranchIndex(this); // Cause I don't trust the local cache
		if (mySafeIndex >= 0)
		{
			parent->pblock->Delete(PB_BRANCHTAB, mySafeIndex, 1);
			parent->pblock->Delete(PB_BRANCHNAMESTAB, mySafeIndex, 1);
		}

		DbgAssert(parent->GetBranchIndex(this) == -1);
		// NULL our pointer to our parent...
		SetBranchParent(NULL);
	}
}

bool CATHierarchyBranch::RemoveLimb(LimbData2* limb)
{
	int nBranches = GetNumBranches();
	for (int j = nBranches - 1; j >= 0; j--)
	{
		CATHierarchyBranch* branch = GetBranch(j);
		branch->RemoveLimb(limb);
	}
	return true;
}

TSTR CATHierarchyBranch::GetBranchName(int index)
{
	if (index < GetNumBranches())
	{
		TSTR strBranchName = pblock->GetStr(PB_BRANCHNAMESTAB, 0, index);

		CATHierarchyBranch* ctrlBranch = (CATHierarchyBranch*)GetBranch(index);
		if (ctrlBranch == NULL)
			return strBranchName;

		ReferenceTarget* owner = ctrlBranch->GetBranchOwner();
		if (owner)
		{
			TSTR strOwnerName;

			if (owner->ClassID() == HUB_CLASS_ID)
				return (((CATControl*)owner->GetInterface(I_CATCONTROL))->GetName() + strBranchName);
			else if (owner->ClassID() == COLLARBONE_CLASS_ID)
				strOwnerName = GetString(IDS_COLLARBONE);
			else if (owner->ClassID() == PALMTRANS2_CLASS_ID)
			{
				PalmTrans2* pPalm = static_cast<PalmTrans2*>(owner);
				LimbData2* pLimb = pPalm->GetLimb();
				if (pLimb != NULL && pLimb->GetisArm())
					return strOwnerName = GetString(IDS_PALM);
				else
					return strOwnerName = GetString(IDS_ANKLE);
			}
			else if (owner->ClassID() == LIMBDATA2_CLASS_ID)
				return (((CATControl*)owner->GetInterface(I_CATCONTROL))->GetName() + strBranchName);
			else if (owner->ClassID() == TAILDATA2_CLASS_ID)
				return (((CATControl*)owner->GetInterface(I_CATCONTROL))->GetName() + strBranchName);

			return (strOwnerName + strBranchName);
		}

		CATHierarchyBranch2* branchParent = (CATHierarchyBranch2*)GetBranchParent();
		if (branchParent && !((CATHierarchyBranch2*)this)->GetisWelded())
		{
			int nNumParentLimbs = branchParent->GetNumLimbs();
			if ((nNumParentLimbs > 0) && (index < nNumParentLimbs))
			{
				TSTR strOwnerName = branchParent->GetLimbName(index);
				return (strOwnerName + strBranchName);
			}
		}

		return strBranchName;
	}
	return _T("");
};

int	CATHierarchyBranch::GetNumLayers()
{
	return GetWeights()->GetNumLayers();
}

BOOL CATHierarchyBranch::IsKeyAtTime(TimeValue, DWORD flags)
{
	UNREFERENCED_PARAMETER(flags);
	BOOL isKey = FALSE;

	/*	int nNumBranches =  GetNumBranches();
		CATHierarchyBranch* branch;

		else for (int i=0; i < nNumBranches; i++)
		{
			branch = GetBranch(j);
			if(branch->IsKeyAtTime(t, flags)) return TRUE;
		}
	*/
	return isKey;
}

// The clip saver needs to be able to turn a pointer into an index.
int	CATHierarchyBranch::SubAnimIndex(Control* sub)
{
	for (int i = 0; i < NumSubs(); i++) {
		if (SubAnim(i) == sub) return i;
	}
	return -1;
}

BOOL CATHierarchyBranch::CleanupTree() {

	BOOL selfdestructing = FALSE;

	// We self destruct if all out children are self destructing
	int i = 0;
	while (i < GetNumBranches()) {
		if (!GetBranch(i)->CleanupTree()) {
			i++;
		}
	}

	// if we have just removed our last sub branch,
	// kill ourselves as  we have no reason to live
//	BOOL selfdestructing = GetNumBranches()==0;
//	if(selfdestructing)
//	{
//		SelfDestruct();
//	}
	return selfdestructing;
}
