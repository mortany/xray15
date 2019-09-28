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

/**********************************************************************
	This is a float controller, but it doesn't have GetValue()
	or SetValue().  It represents a CAT hierarchy tree branch
	as a growable list of items, where an item is another tree
	branch or a hierarchy leaf list.  A branch can act as
	a root or an ordinary branch.  This controller cannot be
	cloned or copied, or have controllers assigned to it.

	If this is the tree root it's called 'catroot'.  All branches
	it creates get a catroot pointer.  All branches that come off
	a non-catroot get their creator's catroot pointer.
 **********************************************************************/

#include "CATPlugins.h"
#include "BezierInterp.h"
#include "macrorec.h"

#include "CATHierarchyBranch2.h"
#include "LimbData2.h"
#include "CATMotionLimb.h"
#include "Hub.h"
#include <CATAPI/CATClassID.h>

#include "CATWindow.h"

 //
 //	CATHierarchyBranch2ClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class CATHierarchyBranch2ClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading);  return new CATHierarchyBranch2(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATHIERARCHYBRANCH); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return CATHIERARCHYBRANCH_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATHierarchyBranch2"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static CATHierarchyBranch2ClassDesc CATHierarchyBranch2Desc;
ClassDesc2* GetCATHierarchyBranch2Desc() { return &CATHierarchyBranch2Desc; }

static ParamBlockDesc2 weightshift_param_blk(CATHierarchyBranch2::PBLOCK_REF, _T("params"), 0, &CATHierarchyBranch2Desc,
	P_AUTO_CONSTRUCT, CATHierarchyBranch2::CATBRANCHPB,

	CATHierarchyBranch2::PB_BRANCHTAB, _T("Branches"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_VARIABLE_SIZE, IDS_LAYERNAME,
		p_end,
	CATHierarchyBranch2::PB_BRANCHNAMESTAB, _T("BranchNames"), TYPE_STRING_TAB, 0, P_VARIABLE_SIZE, IDS_LAYERNAMES,
		p_end,
	CATHierarchyBranch2::PB_EXPANDABLE, _T("Expandable"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 1,
		p_end,
	CATHierarchyBranch2::PB_BRANCHPARENT, _T("BranchParent"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATHierarchyBranch2::PB_BRANCHINDEX, _T("BranchIndex"), TYPE_INT, 0, 0,
		p_end,

	// this is the guy who who is responsible for
	// I.E. hub, palm, collarbone
	CATHierarchyBranch2::PB_BRANCHOWNER, _T("BranchOwner"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,

	CATHierarchyBranch2::PB_LEAFTAB, _T("Leaves"), TYPE_REFTARG_TAB, 0, P_NO_REF + P_VARIABLE_SIZE, IDS_LEAVES,
		p_end,
	CATHierarchyBranch2::PB_LEAFNAMESTAB, _T("LeafNames"), TYPE_STRING_TAB, 0, P_VARIABLE_SIZE, IDS_LEAFNAMES,
		p_end,

	CATHierarchyBranch2::PB_UITYPE, _T("UIType"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 0,
		p_end,
	CATHierarchyBranch2::PB_ISWELDED, _T("isWelded"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 1,
		p_end,

	CATHierarchyBranch2::PB_CONTROLLERTAB, _T("Controller"), TYPE_REFTARG_TAB, 0, P_NO_REF + P_VARIABLE_SIZE, 0,
		p_end,
	CATHierarchyBranch2::PB_SUBANIMINDEX, _T("OwnerSubNum"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 0,
		p_end,
	CATHierarchyBranch2::PB_LIMBS_TAB, _T("Limbs"), TYPE_REFTARG_TAB, 0, P_NO_REF + P_VARIABLE_SIZE, IDS_CL_LIMBDATA,
		p_end,

	CATHierarchyBranch2::PB_ROOT, _T("CATHierarchyRoot"), TYPE_REFTARG, P_NO_REF, IDS_CL_CATHIERARCHYROOT,
		p_end,
	p_end
);

CATHierarchyLeaf* CATHierarchyBranch2::NewCATHierarchyLeafController()
{
	CATHierarchyLeaf* ctrl = static_cast<CATHierarchyLeaf*>(CreateInstance(CTRL_FLOAT_CLASS_ID, CATHIERARCHYLEAF_CLASS_ID));

	CATHierarchyLeaf* weights = GetWeights();
	DbgAssert(weights != NULL);

	ctrl->SetLeafParent(this);
	if (weights)
	{
		ctrl->SetWeights(weights);
		ctrl->AddLayer(weights->GetNumLayers());
		ctrl->SetActiveLayer(weights->GetActiveLayer());
	}
	return ctrl;
}

bool CATHierarchyBranch2::MaybeDestructBranch()
{
	if (GetNumBranches() == 0 &&
		GetNumLeaves() == 0)
	{
		SelfDestruct();
		return true;
	}
	return false;
}

// This class searches a dependent tree to find multiple
// instances of a CATClipMatrix3 reference the same source controller.
class CATGraphFinder : public DependentEnumProc
{
public:
	CATGraph* mGraph;
	CATGraphFinder()
		: mGraph(NULL)
	{
	}

	int proc(ReferenceMaker *rmaker)
	{
		// If we find a node, job done.
		if ((mGraph = dynamic_cast<CATGraph*>(rmaker)) != NULL)
			return DEP_ENUM_HALT;

		return DEP_ENUM_CONTINUE;
	}
};

CATGraph* FindReferencingCATGraph(CATHierarchyLeaf* pLeaf)
{
	if (pLeaf == NULL)
		return NULL;

	CATGraphFinder finder;
	pLeaf->DoEnumDependents(&finder);
	return finder.mGraph;
}

void CATHierarchyBranch2::SelfDestruct()
{
	// If we are destructing, there is no need to notify
	// anyone.  The leaves are affected, but we don't want
	// to notify anyone
	MaxReferenceMsgLock lock;

	// Clean our pointer off our owner
	// I suspect this code is irrelevant.
	ReferenceTarget* pOwner = GetBranchOwner();
	if (pOwner != NULL)
	{
		CATGraph* pOwnerAsGraph = dynamic_cast<CATGraph*>(pOwner);
		if (pOwnerAsGraph != NULL)
			pOwnerAsGraph->SetBranch(NULL);
		else
		{
			// Double-check - we don't have any other pointers now, do we?
			bool bAPointerWasCleaned = NullPointerInParamBlock(pOwner->GetParamBlock(0), this);
			DbgAssert(bAPointerWasCleaned == false);
		}
	}

	// Break the link from us to CATGraph
	int nControllers = GetNumControllerRefs();
	for (int i = 0; i < nControllers; i++)
	{
		CATGraph* pCtrl = dynamic_cast<CATGraph*>(GetControllerRef(i));
		if (pCtrl != NULL)
			pCtrl->SetBranch(NULL);
	}
	pblock->SetCount(PB_CONTROLLERTAB, 0);

	// NULL our own pointer to our owner (just for safeties sake)
	SetBranchOwner(NULL);

	int nLeaves = GetNumLeaves();
	// Remove leaves pointers to us
	for (int i = 0; i < nLeaves; i++)
	{
		CATHierarchyLeaf* pLeaf = GetLeaf(i);
		if (pLeaf != NULL)
			pLeaf->SetLeafParent(NULL);
	}
	// Remove all pointers to leaves
	pblock->SetCount(PB_LEAFTAB, 0);

	// Allow the root to handle destructing branches.
	CATHierarchyBranch::SelfDestruct();
}

// Gets an existing leaf if it exists
CATHierarchyLeaf* CATHierarchyBranch2::GetLeaf(const TSTR& name)
{	// do we already have a leaf branch with this name?
	for (int i = 0; i < GetNumLeaves(); i++) {
		if (0 == _tcsicmp(GetLeafName(i).data(), name.data()))
			return GetLeaf(i);
	}
	return NULL;
}

int	 CATHierarchyBranch2::GetLeafIndex(CATHierarchyLeaf* leaf)
{
	for (int i = 0; i < GetNumLeaves(); i++) {
		if (GetLeaf(i) == leaf)
			return i;
	}
	return -1;
}

TSTR CATHierarchyBranch2::GetLeafName(int index)
{
	CATHierarchyBranch2* branchParent = (CATHierarchyBranch2*)GetBranchParent();
	if (branchParent)
	{
		CATHierarchyRoot* pHierarchyRoot = GetCATRoot();
		if (!pHierarchyRoot)
			return _T("ERROR - no Root");

		int nNumParentLimbs = branchParent->GetNumLimbs();
		if ((nNumParentLimbs > 0) && (index < nNumParentLimbs))
		{
			LimbData2* limb = branchParent->GetLimb(index);
			if (GetisWelded()) return (GetString(IDS_LIMBS));
			else if (limb)
				return limb->GetName();
		}

		// special case for phase offsets branch
		else if ((this == pHierarchyRoot->GetLimbPhasesBranch()) && (index < GetNumLimbs()))
		{
			LimbData2* limb = GetLimb(index);
			if (limb) return limb->GetName();
		}
	}

	return pblock->GetStr(PB_LEAFNAMESTAB, 0, index);
}

// Adds a new leaf branch.
CATHierarchyLeaf* CATHierarchyBranch2::AddLeaf(const TSTR& name)
{
	CATHierarchyLeaf* oldLeaf = GetLeaf(name);
	if (oldLeaf) return oldLeaf;

	CATHierarchyLeaf* ctrl = (CATHierarchyLeaf*)NewCATHierarchyLeafController();// GetCATHierarchyLeafDesc()->Create(FALSE, this);
	if (ctrl) {
		int NumLeaves = GetNumLeaves();
		pblock->SetCount(PB_LEAFTAB, NumLeaves + 1);
		pblock->SetCount(PB_LEAFNAMESTAB, NumLeaves + 1);

		SetLeaf(NumLeaves, ctrl);
		SetLeafName(NumLeaves, name);

		return ctrl;
	}
	return NULL;
}

BOOL CATHierarchyBranch2::RemoveLeaf(CATHierarchyLeaf* leaf)
{
	if (leaf == NULL)
		return FALSE;

	int numleaves = GetNumLeaves();
	int numlimbs = GetNumLimbs();

	for (int i = 0; i < numleaves; i++)
	{
		// Once we find the leaf, remove it.
		if (leaf == GetLeaf(i))
		{
			pblock->Delete(PB_LEAFTAB, i, 1);
			pblock->Delete(PB_LEAFNAMESTAB, i, 1);

			// If our limbs are not welded, then our
			// limb/leaf tabs need to be synchronized.
			if (numlimbs == numleaves)
				pblock->Delete(PB_LIMBS_TAB, i, 1);

			// Remove back pointer
			leaf->SetLeafParent(NULL);

			// if we have just removed our last sub branch,
			// kill ourselves as  we have no reason to live
			MaybeDestructBranch();

			return TRUE;
		}
	}

	return FALSE;
}

CATHierarchyLeaf* CATHierarchyBranch2::AddLimbPhasesLimb(LimbData2 *ctrlLMData)
{
	CATHierarchyRoot* pHierarchyRoot = GetCATRoot();
	if (!pHierarchyRoot)
		return NULL;

	if (this != pHierarchyRoot->GetLimbPhasesBranch()) return NULL;

	CATHierarchyLeaf* newLeaf = NewCATHierarchyLeafController();
	if (newLeaf) {

		int NumLeaves = GetNumLeaves();
		pblock->SetCount(PB_LEAFTAB, NumLeaves + 1);
		pblock->SetCount(PB_LEAFNAMESTAB, NumLeaves + 1);

		SetLeaf(NumLeaves, newLeaf);
		AddLimb(ctrlLMData);
	}

	return newLeaf;
}

bool CATHierarchyBranch2::RemoveLimb(LimbData2* limb)
{
	// We could delete ourselves in this iteration
	MaxAutoDeleteLock lock(this);

	// First, iterate th hierarchy
	CATHierarchyBranch::RemoveLimb(limb);

	// Now remove our own pointer.
	int nLimbs = pblock->Count(PB_LIMBS_TAB);
	for (int i = 0; i < nLimbs; i++)
	{
		if (pblock->GetReferenceTarget(PB_LIMBS_TAB, 0, i) == limb)
		{
			// Remove the pointer to the any graphs.
			if (GetNumControllerRefs() > i)
			{
				CATGraph* pGraph = dynamic_cast<CATGraph*>(GetControllerRef(i));
				if (pGraph != NULL)
				{
					pGraph->SetBranch(NULL);
					pGraph->ReplaceReference(CATGraph::REF_CATMOTIONLIMB, NULL);
				}
				pblock->Delete(PB_CONTROLLERTAB, i, 1);
			}

			// If we still have limbs left, and we are not welded,
			// we need to remove those extra leaves.
			if (!GetisWelded() && nLimbs > 1 && GetNumLeaves() == nLimbs)
			{
				// The following call will remove the leaf and hte limb
				RemoveLeaf(i);
			}
			else if (nLimbs == 1)
			{
				// Was that our last limb?
				SelfDestruct();
				return true;
			}
			else
			{
				// Remove our pointer to the limb, and to the
				// controller that we reference
				pblock->Delete(PB_LIMBS_TAB, i, 1);
			}
			// We have cleaned the limb - break out of the loop!
			break;
		}
	}
	return false;

}

void CATHierarchyBranch2::ResetToDefaults(TimeValue t) {
	int j;
	for (j = 0; j < GetNumBranches(); j++)
		((CATHierarchyBranch2*)GetBranch(j))->ResetToDefaults(t);

	int nNumLeaves = GetNumLeaves();
	CATHierarchyLeaf* leaf;
	if (nNumLeaves > 0)
	{
		float dDefaultVal;
		for (j = 0; j < nNumLeaves; j++)
		{
			leaf = GetLeaf(j);
			dDefaultVal = leaf->GetDefaultVal();
			leaf->SetValue(t, (void*)&dDefaultVal, TRUE, CTRL_ABSOLUTE);
		}
	}
}

CATHierarchyBranch* CATHierarchyBranch2::AddAttribute(Control *ctrl, const TSTR& string, CATMotionLimb *catmotionlimb, int nNumDefaultVals/*=0*/, float *dDefaultVals/*=NULL*/)
{

	// Adds a Attribute Graph controller to the hierarchy.
	// First we add a branch using AddBranch, that will return
	// the branch that we will build up to look like our attribute

	//	if(!(stringID > 0) || !ctrl)
	//		return NULL;

	pblock->EnableNotifications(FALSE);
	// Macro-recorder support
	MACRO_DISABLE;

	// create the root of this new Sub-Hierarchy
	// we are now inserting these branches at the top of the hierarchy to
	// try and shuffle all the sub-brach hierarchuies to the bottom
	CATHierarchyBranch2 *attributeBranch = (CATHierarchyBranch2*)this->AddBranch(string, 0);
	DbgAssert(attributeBranch);
	attributeBranch->SetExpandable(FALSE);
	// store the guy who will be used to build the hierarchy
	attributeBranch->AddControllerRef(ctrl);

	if (catmotionlimb)
	{
		// we seem to be storing a limbs table on too many places
		// the attributebranch and all its leaves
		// but the ui uses the attributebranch's limbs
		// and the 'leaf' branches also need limb pointers for
		// subanim names etc.
		// defaulting to welded limbs
		attributeBranch->WeldLeaves();
		//	attributeBranch->SetisWelded(TRUE);

		attributeBranch->AddLimb(catmotionlimb->GetLimb());

		if (ctrl->GetInterface(I_CATGRAPH)) {
			((CATGraph*)ctrl)->SetCATMotionLimb(catmotionlimb);
		}
	}

	CATHierarchyBranch2 *ctrlBranch;
	CATHierarchyLeaf *ctrlLeaf;
	int iAssignedVals = 0;

	//***********************************************************************
	// Set the UI flags                                                     *
	//***********************************************************************
	if (ctrl->GetInterface(I_CATGRAPH)) {
		attributeBranch->SetUIType(UI_GRAPH);
	}
	else if (ctrl->ClassID() == IPOINT3_CONTROL_CLASS_ID) {
		attributeBranch->SetUIType(UI_POINT3);
	}

	if (ctrl->NumParamBlocks())
	{
		IParamBlock2 *ctrlParamBlock = ctrl->GetParamBlock(0);
		// All of our Attribute controllers have parameter blocks
		// if not the they should!!
		if (ctrlParamBlock)
		{
			for (int i = 0; i < ctrlParamBlock->NumParams(); i++)
			{
				ParamID pid = ctrlParamBlock->IndextoID(i);
				ParamType2 attributeType = ctrlParamBlock->GetParameterType(pid);
				TSTR strParamName = ctrlParamBlock->GetLocalName(pid);

				float attributeVal = 0.0f;

				if (attributeType == TYPE_FLOAT || attributeType == TYPE_TIMEVALUE || attributeType == TYPE_ANGLE)
				{
					if ((nNumDefaultVals > 0) && (iAssignedVals < nNumDefaultVals))
					{
						attributeVal = dDefaultVals[iAssignedVals];
					}
					else
					{

						attributeVal = ctrlParamBlock->GetFloat(pid, 0);
					}

					if (catmotionlimb) {
						CATHierarchyBranch2 *ctrlBranch = (CATHierarchyBranch2*)attributeBranch->AddBranch(strParamName);
						ctrlBranch->SetSubAnimIndex(i);
						// we have a doubly linked Hierarchy
						//	ctrlBranch->SetBranchParent((ReferenceTarget*)attributeBranch);

						// Defaulting to welded branches
						ctrlBranch->SetisWelded(TRUE);
						ctrlLeaf = (CATHierarchyLeaf*)ctrlBranch->AddLeaf(GetString(IDS_LIMBS));

						// Default val is stored on the branch so we need a pointer
						ctrlLeaf->SetLeafParent(ctrlBranch);
					}
					else {
						ctrlLeaf = (CATHierarchyLeaf*)attributeBranch->AddLeaf(strParamName);
						ctrlLeaf->SetLeafParent(attributeBranch);
					}

					ctrlLeaf->SetDefaultVal(attributeVal);
					ctrlLeaf->SetValue(0, (void*)&attributeVal, TRUE, CTRL_ABSOLUTE);
					ctrlParamBlock->SetControllerByIndex(i, 0, (Control*)ctrlLeaf, FALSE);
					iAssignedVals++;
				}
				else
				{
					// CATGraph can access the limb thru catmotionlimb
					if (!_tcscmp(strParamName, GetString(IDS_CATBRANCH)))
						ctrlParamBlock->SetValue(pid, 0, (ReferenceTarget*)attributeBranch);
				}
			}
		}

		if (attributeBranch->GetUIType() == UI_GRAPH)
			((CATGraph*)ctrl)->InitControls();
	}
	else if (ctrl->NumSubs() > 0)
	{
		for (int i = 0; i < ctrl->NumSubs(); i++)
		{
			if (ctrl->SubAnim(i) && ctrl->SubAnim(i)->SuperClassID() == CTRL_FLOAT_CLASS_ID)
			{
				Control *attributeCtrl = (Control*)ctrl->SubAnim(i);
				float attributeVal;
				Interval iv = FOREVER;
				attributeCtrl->GetValue(0, (void*)&attributeVal, iv, CTRL_ABSOLUTE);

				TSTR name = ctrl->SubAnimName(i);
				if (catmotionlimb)
				{
					ctrlBranch = (CATHierarchyBranch2*)attributeBranch->AddBranch(ctrl->SubAnimName(i));
					ctrlBranch->SetSubAnimIndex(i);

					// Defaulting to welded branches
					ctrlBranch->SetisWelded(TRUE);
					ctrlLeaf = (CATHierarchyLeaf*)ctrlBranch->AddLeaf(GetString(IDS_LIMBS));

					ctrl->AssignController((Control*)ctrlLeaf, i);
				}
				else
				{
					ctrlLeaf = (CATHierarchyLeaf*)attributeBranch->AddLeaf(ctrl->SubAnimName(i));
					ctrl->AssignController((Control*)ctrlLeaf, i);
				}
			}
		}
	}

	pblock->EnableNotifications(TRUE);
	// Macro-recorder support
	MACRO_ENABLE;

	return attributeBranch;
}

void CATHierarchyBranch2::WeldLeaves()
{
	int nNumBranches = GetNumBranches();
	if (nNumBranches > 0)
		for (int i = 0; i < nNumBranches; i++)
			((CATHierarchyBranch2*)GetBranch(i))->WeldLeaves();

	int nNumLeaves = GetNumLeaves();
	if (!GetisWelded() && (nNumLeaves > 0))
	{
		Control* ctrl = NULL;
		CATHierarchyBranch2 *branchParent = (CATHierarchyBranch2*)GetBranchParent();
		DbgAssert(branchParent);

		int nNumControllerRefs = branchParent->GetNumControllerRefs();
		int nNumLimbs = branchParent->GetNumLimbs();
		DbgAssert(nNumControllerRefs == nNumLimbs);
		UNREFERENCED_PARAMETER(nNumControllerRefs);

		CATHierarchyLeaf *leaf = (CATHierarchyLeaf*)GetLeaf(0);
		int subindex = GetSubAnimIndex();
		for (int i = 0; i < nNumLimbs; i++)
		{
			ctrl = (Control*)branchParent->GetControllerRef(i);
			ctrl->AssignController(leaf, subindex);
		}
		pblock->SetCount(PB_LEAFTAB, 1);
	}
	SetisWelded(TRUE);
}

void CATHierarchyBranch2::UnWeldLeaves()
{
	for (int i = 0; i < GetNumBranches(); i++)
		((CATHierarchyBranch2*)GetBranch(i))->UnWeldLeaves();

	int nNumLeaves = GetNumLeaves();
	if (GetisWelded() && (nNumLeaves > 0))
	{
		Control* ctrl = NULL;
		CATHierarchyBranch2 *branchParent = (CATHierarchyBranch2*)GetBranchParent();
		DbgAssert(branchParent);
		CATHierarchyLeaf *leaf;
		LimbData2* limb;

		int nNumControllerRefs = branchParent->GetNumControllerRefs();
		int nNumLimbs = branchParent->GetNumLimbs();
		DbgAssert(nNumControllerRefs == nNumLimbs);
		UNREFERENCED_PARAMETER(nNumControllerRefs);

		TimeValue t = GetCOREInterface()->GetTime();
		CATHierarchyLeaf *baseleaf = GetLeaf(0);
		DbgAssert(baseleaf);
		float dDefaultVal = baseleaf->GetDefaultVal();
		float currVal = 0.0f;
		//	leaf->GetValue(t, (void*)&currVal, FOREVER, CTRL_ABSOLUTE);
		int subindex = GetSubAnimIndex();
		for (int i = 0; i < nNumLimbs; i++)
		{
			limb = branchParent->GetLimb(i);
			if (i == 0)
			{
				leaf = baseleaf;
				//	leaf->SetLimbData(limb);
				SetLeafName(i, limb->GetName());
			}
			else
			{
				leaf = (CATHierarchyLeaf*)AddLeaf(limb->GetName());//(limb);
				leaf->SetDefaultVal(dDefaultVal);
				Interval iv;
				for (int j = 0; j < baseleaf->GetNumLayers(); j++)
				{
					iv = FOREVER;
					baseleaf->GetLayer(j)->GetValue(t, (void*)&currVal, iv, CTRL_ABSOLUTE);
					leaf->GetLayer(j)->SetValue(t, (void*)&currVal, TRUE, CTRL_ABSOLUTE);
				}
			}
			// put our new leaves onto the CATGraphs subanims
			ctrl = branchParent->GetControllerRef(i);
			ctrl->AssignController(leaf, subindex);
		}
	}
	SetisWelded(FALSE);
}

void CATHierarchyBranch2::SetActiveLeaf(int index)
{
	nActiveLeaf = index;
	if (GetNumBranches() > 0)
		for (int i = 0; i < GetNumBranches(); i++)
			((CATHierarchyBranch2*)GetBranch(i))->SetActiveLeaf(index);
}

int CATHierarchyBranch2::GetNumGraphKeys()
{
	if (GetNumControllerRefs() > 0)
	{
		CATGraph* pGraph = static_cast<CATGraph*>(GetControllerRef(0));
		if (pGraph != NULL)
			return pGraph->GetNumGraphKeys();
	}
	return 0;
}

void CATHierarchyBranch2::GetGraphKey(int iKeyNum,
	Control**	ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
	Control**	ctrlValue, float &fValueVal, float &minVal, float &maxVal,
	Control**	ctrlTangent, float &fTangentVal,
	Control**	ctrlInTanLen, float &fInTanLenVal,
	Control**	ctrlOutTanLen, float &fOutTanLenVal,
	Control**	ctrlSlider)
{
	// this function passes ourselves to a Graph controller, and the Graph controller
	// knows how to hook it up. This is based on the knowledge that the branch was built to
	// represent the Graph controller.

	if (GetNumControllerRefs()) ((CATGraph*)GetControllerRef(0))->GetGraphKey(
		iKeyNum, this,// pass in ourselves
		ctrlTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
		ctrlValue, fValueVal, minVal, maxVal,
		ctrlTangent, fTangentVal,
		ctrlInTanLen, fInTanLenVal,
		ctrlOutTanLen, fOutTanLenVal,
		ctrlSlider);

}

void CATHierarchyBranch2::PasteGraph()
{
	CATHierarchyRoot* pHierarchyRoot = GetCATRoot();
	if (!pHierarchyRoot)
		return;

	if (GetNumControllerRefs() > 0)
	{
		CATGraph* pCtrl = static_cast<CATGraph*>(GetControllerRef(0));
		if (pCtrl != NULL)
			pCtrl->PasteGraph(pHierarchyRoot->GetCopiedBranch());
	}
}

// We want to display a Point3 Branch in the ui
void CATHierarchyBranch2::GetP3Ctrls(Control** ctrlX, Control** ctrlY, Control** ctrlZ)
{
	// Limb offset of some kind
	if (GetNumBranches() == 3)
	{
		*ctrlX = (Control*)GetBranch(0);
		*ctrlY = (Control*)GetBranch(1);
		*ctrlZ = (Control*)GetBranch(2);
	}
	// Hub offset
	else if (GetNumLeaves() == 3)
	{
		*ctrlX = (Control*)GetLeaf(0);
		*ctrlY = (Control*)GetLeaf(1);
		*ctrlZ = (Control*)GetLeaf(2);
	}
}

/* Empty namespace ********************************************************** */
namespace
{

	/* ************************************************************************** **
	** DESCRIPTION: Draws in hGraphDC a square at position 'center'.			  **
	** CONTEXT: CATHierarchyBranch2::BranchDrawHandles                             **
	** ************************************************************************** */
	void	DrawKeyLine(HDC			&hGraphDC,
		const Point2	start,
		const Point2	end,
		const int iGraphWidth,
		const bool hot)
	{
		UNREFERENCED_PARAMETER(hot);
		if (end.x > iGraphWidth) {
			// the whole line is outside the graph area
			if (start.x > iGraphWidth) {
				MoveToEx(hGraphDC, (int)(start.x - iGraphWidth), (int)start.y, NULL);
				LineTo(hGraphDC, (int)(end.x - iGraphWidth), (int)end.y);
				return;
			}
			float ratio = (start.x - iGraphWidth) / (end.x - start.x);
			float BreakPointY = start.y + ((start.y - end.y) * ratio);//start.x/(end.x - start.x));

			MoveToEx(hGraphDC, (int)start.x, (int)start.y, NULL);
			LineTo(hGraphDC, iGraphWidth, (int)BreakPointY);

			MoveToEx(hGraphDC, 0, (int)BreakPointY, NULL);
			LineTo(hGraphDC, (int)(end.x - iGraphWidth), (int)end.y);

/*			float BreakPointY = start.y + ((start.y - end.y) * ((end.x - iGraphWidth)/(end.x - start.x)));

			MoveToEx(hGraphDC, (int)start.x, (int)start.y, NULL);
			LineTo(hGraphDC, iGraphWidth, (int)BreakPointY);

			MoveToEx(hGraphDC, 0, (int)BreakPointY, NULL);
			LineTo(hGraphDC, (int)(end.x - iGraphWidth), (int)end.y);
*/
			return;
		}

		if (start.x < 0.0f) {
			// the whole line is outside the graph area
			if (end.x < 0.0f) {
				MoveToEx(hGraphDC, (int)(start.x + iGraphWidth), (int)start.y, NULL);
				LineTo(hGraphDC, (int)(end.x + iGraphWidth), (int)end.y);
				return;
			}

/*			float BreakPointY = start.y + ((start.y + end.y) * ((-end.x)/(end.x + start.x)));

			MoveToEx(hGraphDC, (int)(start.x + iGraphWidth), (int)start.y, NULL);
			LineTo(hGraphDC, iGraphWidth, (int)BreakPointY);

			MoveToEx(hGraphDC, 0, (int)BreakPointY, NULL);
			LineTo(hGraphDC, (int)end.x, (int)end.y);
*/
			float ratio = start.x / (end.x - start.x);
			float BreakPointY = start.y + ((start.y - end.y) * ratio);//start.x/(end.x - start.x));

			MoveToEx(hGraphDC, (int)(start.x + iGraphWidth), (int)start.y, NULL);
			LineTo(hGraphDC, iGraphWidth, (int)BreakPointY);

			MoveToEx(hGraphDC, 0, (int)BreakPointY, NULL);
			LineTo(hGraphDC, (int)end.x, (int)end.y);
			return;
		}

		MoveToEx(hGraphDC, (int)start.x, (int)start.y, NULL);
		LineTo(hGraphDC, (int)end.x, (int)end.y);

	}

	/* ************************************************************************** **
	** DESCRIPTION: Draws in hGraphDC a square at position 'center'.			  **
	** CONTEXT: CATHierarchyBranch2::BranchDrawHandles                             **
	** ************************************************************************** */
	void	DrawKeyKnob(Point2 center,
		HDC			&hGraphDC,
		const int iGraphWidth,
		const int		size,
		const bool hot)
	{
		UNREFERENCED_PARAMETER(hot);
		if (center.x < 0) center.x += iGraphWidth;
		else if (center.x > iGraphWidth) center.x -= iGraphWidth;

		MoveToEx(hGraphDC, (int)(center.x - size), (int)(center.y - size), NULL);
		LineTo(hGraphDC, (int)(center.x - size), (int)(center.y + size));
		LineTo(hGraphDC, (int)(center.x + size), (int)(center.y + size));
		LineTo(hGraphDC, (int)(center.x + size), (int)(center.y - size));
		LineTo(hGraphDC, (int)(center.x - size), (int)(center.y - size));
	}

	/* ************************************************************************** **
	** DESCRIPTION: Draws in hGraphDC a tangent at position 'center'.			  **
	** CONTEXT: CATHierarchyBranch2::BranchDrawHandles                             **
	** ************************************************************************** */
	/*void	DrawKey (Point2	&center,
					bool	&intan,
					Point2	&intanPos,
					bool	&outtan,
					Point2	&outtanPos,
					HDC		&hGraphDC,
					const float iGraphWidth)*/
	void	DrawKey(
		const CATKey key1,
		const bool		isOutTan1,
		const Point2	outTan,
		const bool		key1Hot,
		const CATKey key2,
		const bool		isInTan2,
		const Point2	inTan,
		const bool		key2Hot,
		HDC		&hGraphDC,
		HPEN	&keyPen,
		HPEN	&hotKeyPen,
		const int iGraphWidth)
	{
		Point2 center1(key1.x, key1.y);
		Point2 center2(key2.x, key2.y);

		if (key2Hot)
			SelectObject(hGraphDC, hotKeyPen);
		else SelectObject(hGraphDC, keyPen);

		if (isInTan2) {
			DrawKeyLine(hGraphDC, inTan, center2, iGraphWidth, key2Hot);
			DrawKeyKnob(inTan, hGraphDC, iGraphWidth, 1, key2Hot);
		}

		if (key1Hot)
			SelectObject(hGraphDC, hotKeyPen);
		else SelectObject(hGraphDC, keyPen);

		if (isOutTan1) {
			DrawKeyLine(hGraphDC, center1, outTan, iGraphWidth, key1Hot);
			DrawKeyKnob(outTan, hGraphDC, iGraphWidth, 1, key1Hot);
		}
		DrawKeyKnob(center1, hGraphDC, iGraphWidth, 2, key1Hot);
	}

	/* Empty namespace ********************************************************** */
}

/* ************************************************************************** **
** DESCRIPTION: Returns the controlPoint which is under mouse position		  **
**   'clickPos.																  **
** ************************************************************************** */
bool	CATHierarchyBranch2::GetClickedKey(const Point2	&clickPos,
	const Point2	&clickRange,
	int			&iKeynum,
	int			&iLimb,
	BOOL			&bKey,
	BOOL			&bInTan,
	BOOL			&bOutTan)
{
	const TimeValue t(GetCOREInterface()->GetTime());

	CATKey	key, key1st, key1, key2;
	// Flags telling us whether we have a controller for the intan or the outtan values.
	bool	isOutTan1, isInTan1, isOutTan2, isInTan2;
	Point2	outTan, inTan;
	float dDeltax;
	float dDeltay;

	isOutTan2 = isInTan2 = false;
	for (int i = 0; i < GetNumControllerRefs(); i++)
	{
		CATGraph	*ctrlCATGraph = (CATGraph*)GetControllerRef(i);
		int			numKeys = ctrlCATGraph->GetNumGraphKeys();

		//////////////////////////////////////////////////////////////////////
		// 1st check for clicking on the actual key knob.
		for (int j = 0; j <= numKeys; ++j)
		{
			ctrlCATGraph->GetCATKey(j, t, key, isInTan1, isOutTan1);

			if (key.x < 0.0f) key.x += STEPTIME100;
			else key.x = fmod(key.x, (float)STEPTIME100);
			dDeltax = fabs(clickPos.x - key.x);
			dDeltay = fabs(clickPos.y - key.y);
			if ((dDeltax < clickRange.x) &&
				(dDeltay < clickRange.y))
			{
				// for some reason that everyb dy has forgotten, keynum is a 1 base index to the keys.
				iKeynum = (j + 1);
				iLimb = i;
				bKey = true;
				return true;
			}
		}

		//////////////////////////////////////////////////////////////////////
		// Now using pairs of keys(key1 and key2), we evaluate the tangents
		// and see iof they we clicked on
		ctrlCATGraph->GetCATKey(0, t, key1st, isInTan1, isOutTan1);
		key1 = key1st;

		for (int j = 1; j <= numKeys; ++j)
		{
			if (j == numKeys) {
				key2 = key1st;
				// Make sure key2 is ahead of key1 in time by adding on a loop
				key2.x += STEPTIME100;
			}
			else
				ctrlCATGraph->GetCATKey(j, t, key2, isInTan2, isOutTan2);

			if (isInTan2 || isOutTan1)
			{
				// Given 2 keys, calculate the out and in tangents between them
				computeControlKeys(key1, key2, outTan, inTan);

				// Ensure the tangent handle position we check is actually on the screen
				if (outTan.x < 0.0f) outTan.x += (float)STEPTIME100;
				else outTan.x = fmod(outTan.x, (float)STEPTIME100);

				// Now see if the screenspace position is within range of our click positon
				if ((fabs(clickPos.x - outTan.x) < clickRange.x) &&
					(fabs(clickPos.y - outTan.y) < clickRange.y))
				{
					iKeynum = j;
					iLimb = i;
					bOutTan = true;
					return true;
				}

				// Ensure the tangent handle position we check is actually on the screen
				if (inTan.x < 0.0f) inTan.x += (float)STEPTIME100;
				else inTan.x = fmod(inTan.x, (float)STEPTIME100);

				// Now see if the screenspace position is within range of our click positon
				if ((fabs(clickPos.x - inTan.x) < clickRange.x) &&
					(fabs(clickPos.y - inTan.y) < clickRange.y))
				{
					iKeynum = ((j == numKeys) ? 1 : j + 1);
					iLimb = i;
					bInTan = true;
					return true;
				}
			}

			key1 = key2;
			isOutTan1 = isOutTan2;
			isInTan1 = isInTan2;
		}
	}

	return false;
}

/* ************************************************************************** **
** DESCRIPTION: Calculates the handles for all the keys,finds the max and min **
** ************************************************************************** */
void CATHierarchyBranch2::GetYRange(TimeValue t, float &minY, float &maxY)
{
	int			numKeys;
	Point2		point1, outTan, inTan, point2;

	CATKey		key1st, key1, key2;//, TempKey1, TempKey2;
	bool		isOutTan1, isInTan1, isOutTan2, isInTan2;

	isOutTan2 = isInTan2 = false;
	for (int i = 0; i < GetNumControllerRefs(); i++)
	{
		CATGraph	*ctrlCATGraph = (CATGraph*)GetControllerRef(i);
		numKeys = ctrlCATGraph->GetNumGraphKeys();

		ctrlCATGraph->GetCATKey(0, t, key1st, isInTan1, isOutTan1);
		key1 = key1st;

		for (int j(1); j <= numKeys; ++j)
		{
			if (j == numKeys)
				key2 = key1st;
			else
				ctrlCATGraph->GetCATKey(j, t, key2, isInTan2, isOutTan2);

			if (key2.x < key1.x) key2.x += STEPTIME100;

			if (isInTan2 || isOutTan1)
			{
				computeControlKeys(key1, key2, outTan, inTan);
				minY = min(minY, (min(outTan.y, inTan.y)));
				maxY = max(maxY, (max(outTan.y, inTan.y)));
			}
			else
			{
				minY = min(minY, (min(key1.y, key2.y)));
				maxY = max(maxY, (max(key1.y, key2.y)));
			}
			key1 = key2;
			isOutTan1 = isOutTan2;
			isInTan1 = isInTan2;
		}
	}

}

/* ************************************************************************** **
** DESCRIPTION: Draws the handles.											  **
**   numSegments..... number of bezier segments, equal to the number of keys. **
** NOTE: in the loop, each Bezier segment draws 2 tangents + 1 point.         **
** ************************************************************************** */
void CATHierarchyBranch2::DrawKeys(const TimeValue	t,
	const int	iGraphWidth,
	const int	iGraphHeight,
	const float		alpha,
	const float		beta,
	HDC				&hGraphDC,
	const int		nSelectedKey)
{
	int			numKeys;
	Point2		point1, outTan, inTan, point2;

	CATKey		key1st, key1, key2, TempKey1, TempKey2;
	bool		isOutTan1st, isInTan1st, isOutTan1, isInTan1, isOutTan2, isInTan2, key1Hot, key2Hot, key1stHot;

	float		widthScaleCoeff(iGraphWidth / float(STEPTIME100));
	HPEN		keyPen = CreatePen(PS_SOLID, 1, RGB(250, 250, 250));
	HPEN		hotKeyPen = CreatePen(PS_SOLID, 1, RGB(250, 0, 0));

	for (int i = 0; i < GetNumControllerRefs(); i++)
	{
		CATGraph	*ctrlCATGraph = (CATGraph*)GetControllerRef(i);
		numKeys = ctrlCATGraph->GetNumGraphKeys();

		ctrlCATGraph->GetCATKey(0, t, key1st, isInTan1st, isOutTan1st);
		//	key1st.x *= widthScaleCoeff;
		//	key1st.y = iGraphHeight - (alpha + (beta * key1st.y));
		key1 = key1st;
		isOutTan1 = isOutTan1st;
		isInTan1 = isInTan1st;
		if (1 == nSelectedKey) {
			key1stHot = true;
			key1Hot = true;
		}
		else {
			key1Hot = false;
			key1stHot = false;
		}

		for (int j(1); j <= numKeys; ++j)
		{
			if (j == numKeys) {
				key2 = key1st;
				key2.x += STEPTIME100;
				key2Hot = key1stHot;
				isOutTan2 = isOutTan1st;
				isInTan2 = isInTan1st;
			}
			else {
				ctrlCATGraph->GetCATKey(j, t, key2, isInTan2, isOutTan2);

				if ((j + 1) == nSelectedKey)
					key2Hot = true;
				else key2Hot = false;
			}

			if (isInTan2 || isOutTan1)
			{
				computeControlKeys(key1, key2, outTan, inTan);

				if (isOutTan1)
				{
					outTan.x = widthScaleCoeff * outTan.x;
					outTan.y = iGraphHeight - (alpha + (beta * outTan.y));
				}
				if (isInTan2)
				{
					inTan.x = widthScaleCoeff * inTan.x;
					inTan.y = iGraphHeight - (alpha + (beta * inTan.y));
				}

			}
			TempKey1 = key1;
			TempKey1.x *= widthScaleCoeff;
			TempKey1.y = iGraphHeight - (alpha + (beta * key1.y));
			TempKey2 = key2;
			TempKey2.x *= widthScaleCoeff;
			TempKey2.y = iGraphHeight - (alpha + (beta * key2.y));

			DrawKey(TempKey1, isOutTan1, outTan, key1Hot, TempKey2, isInTan2, inTan, key2Hot, hGraphDC, keyPen, hotKeyPen, iGraphWidth);

			key1 = key2;
			isOutTan1 = isOutTan2;
			isInTan1 = isInTan2;
			key1Hot = key2Hot;

		}
	}

	DeleteObject(keyPen);
	DeleteObject(hotKeyPen);
}

/* ************************************************************************** */
// Tell this branchs Controllers to draw themselves
void CATHierarchyBranch2::DrawGraph(TimeValue t,
	bool	isActive,
	int		nSelectedKey,
	HDC		hGraphDC,
	int		iGraphWidth,
	int		iGraphHeight,
	float	alpha,
	float	beta)
{

	int		LoopT, i;
	float	posX, oldX = 0.0f;//, avgY, oldAvgY;
	int		LineLength = (iGraphWidth / 120);

	// Create the Pens
	Tab <float> tabLimbYPos;
	Tab <HPEN> tabLimbPens;
	Tab <float> tabVals;
	float totVal;

	tabLimbYPos.SetCount(GetNumControllerRefs());
	tabLimbPens.SetCount(GetNumControllerRefs());

	Tab <float> tabOldY;
	Tab <float> tabNewY;
	tabOldY.SetCount(GetNumControllerRefs());
	tabNewY.SetCount(GetNumControllerRefs());

	for (i = 0; i < GetNumControllerRefs(); i++)
		tabLimbPens[i] = CreatePen(PS_SOLID, 2, ((CATGraph*)GetControllerRef(i))->GetGraphColour());

	for (posX = 0; posX < iGraphWidth; posX += LineLength)
	{
		LoopT = (int)(((float)posX / (float)iGraphWidth) * STEPTIME100);
		totVal = 0.0f;

		for (i = 0; i < GetNumControllerRefs(); i++)
		{
			// build a table of all the values
			tabNewY[i] = ((CATGraph*)GetControllerRef(i))->GetGraphYval(t, LoopT);
			tabNewY[i] = iGraphHeight - (alpha + (beta * tabNewY[i]));

			if (posX && (tabNewY[i] > 0) && (tabNewY[i] < iGraphHeight))
			{
				SelectObject(hGraphDC, tabLimbPens[i]);
				MoveToEx(hGraphDC, (int)oldX, (int)tabOldY[i], NULL);
				LineTo(hGraphDC, (int)posX, (int)tabNewY[i]);
			}

			// total the values
			totVal += tabNewY[i];
			tabOldY[i] = tabNewY[i];
		}
		oldX = posX;
	}

	if (isActive)
		DrawKeys(t,
			iGraphWidth,
			iGraphHeight,
			alpha,
			beta,
			hGraphDC,
			nSelectedKey);

	// Select the old Object
//	SelectObject(hGraphDC, hOldObject);
	// delete all our new ones
	for (i = 0; i < GetNumControllerRefs(); i++)
		DeleteObject(tabLimbPens[i]);
}

int CATHierarchyBranch2::PaintFCurves(ParamDimensionBase *dim, HDC hdc, Rect& rcGraph, Rect& rcPaint,
	float tzoom, int tscroll, float vzoom, int vscroll, DWORD flags)
{
	// This paints a kid.
	for (int i = 0; i < GetNumControllerRefs(); i++)
		((CATGraph*)GetControllerRef(i))->PaintFCurves(dim, hdc, rcGraph, rcPaint, tzoom, tscroll, vzoom, vscroll, flags);

	return 0;
}

int CATHierarchyBranch2::GetFCurveExtents(ParamDimensionBase *dim, float &min, float &max, DWORD flags)
{
	for (int i = 0; i < GetNumControllerRefs(); i++)
		((CATGraph*)GetControllerRef(i))->GetFCurveExtents(dim, min, max, flags);

	return 1;
}

BOOL CATHierarchyBranch2::SavePreset(CATPresetWriter *save, TimeValue t)
{
	int i;
	if (GetNumLimbs() > 0) {
		float val = (float)GetisWelded();
		save->WriteValue(_T("WeldedLeaves"), val);
	}

	// First, save out our motion values
	for (i = 0; i < GetNumLeaves(); i++)
		save->FromController(GetLeaf(i), t, SubAnimName(i));

	// Now save each branch
	for (i = 0; i < GetNumBranches(); i++) {
		save->BeginBranch(GetBranchName(i));
		((CATHierarchyBranch2*)GetBranch(i))->SavePreset(save, t);
		save->EndBranch();
	}

	return TRUE;
}

BOOL CATHierarchyBranch2::LoadPreset(CATPresetReader *load, TimeValue t, int slot)
{
	CATHierarchyRoot* pCATRoot = GetCATRoot();
	if (pCATRoot == NULL)
		return FALSE;

	int branchesLoaded = 0;
	int leavesLoaded = 0;
	int nNumBranches = GetNumBranches();
	int nNumLeaves = GetNumLeaves();
	for (int i = 0; i < nNumBranches; i++)
		GetBranch(i)->loadedPreset = FALSE;

	BOOL ok = TRUE;
	bool done = false;
	CATHierarchyLeaf *leaf;
	CATHierarchyBranch2 *branch;

	while (!done && load->ok() && ok) {
		ok = load->NextClause();

		switch (load->CurClauseID()) {
			// This is end of input.
		case motionEnd:
			done = true;
			continue;

			// This is a value.  If the value controller exists,
			// load it with the requested value.  Otherwise spit
			// out a warning and continue.
		case motionValue:
			if (_tcscmp(load->CurName().data(), _T("WeldedLeaves")) == 0) {
				int val = (int)load->GetValue();
				if (val) WeldLeaves();
				else	UnWeldLeaves();
				break;
			}

			if (this == pCATRoot->GetLimbPhasesBranch()) {
				if (leavesLoaded < GetNumLeaves())
					leaf = GetLeaf(leavesLoaded);
				else break;
			}
			else
			{
				leaf = GetLeaf(load->CurName());

				// this is trying to handle loading
				// unwelded onto welded and vice versa
				if (!leaf)
				{
					if ((_tcscmp(load->CurName().data(), _T("Arms")) == 0 ||
						_tcscmp(load->CurName().data(), _T("Legs")) == 0) &&
						!GetisWelded())
					{
						float val = load->GetValue();
						for (int j = 0; j < nNumLeaves; j++)
						{
							CATHierarchyLeaf* pLeaf = GetLeaf(j);
							DbgAssert(pLeaf);
							if (pLeaf != NULL)
							{
								Control* pCtrl = pLeaf->GetLayer(slot);
								DbgAssert(pCtrl != NULL);
								if (pCtrl != NULL)
									pCtrl->SetValue(t, (void*)&val);
							}
						}
						leavesLoaded = nNumLeaves;
						break;
					}
					// if our branch is 'welded', and our file is not
					if (GetisWelded() && (GetNumLeaves() == 1))//
						leaf = GetLeaf(0);
					//	else if(leavesLoaded<nNumLeaves)
						//	leaf = GetLeaf(leavesLoaded);
				}
			}

			if (leaf) {
				float val = load->GetValue();
				Control* pCtrl = leaf->GetLayer(slot);
				DbgAssert(pCtrl != NULL);
				if (pCtrl != NULL)
					pCtrl->SetValue(t, (void*)&val);
				leavesLoaded++;
			}
			break;

			// This is a branch.  If the branch controller exists,
			// call its loader.  Otherwise spit out a warning and
			// skip the entire branch.
		case motionBeginBranch:
		{
			branch = (CATHierarchyBranch2*)GetBranch(load->CurName());

			// Hack so that I could load old presets
			//TSTR strTail = _T("Tail");
			// are we the tail branch?
			if (0 == _tcsicmp(GetBranchName().data(), _T("Tail")))
			{
				if (0 == _tcsicmp(load->CurName().data(), GetString(IDS_TWIST)))
					branch = (CATHierarchyBranch2*)GetBranch(0);

				if (0 == _tcsicmp(load->CurName().data(), GetString(IDS_PITCH)))
					branch = (CATHierarchyBranch2*)GetBranch(2);
			}

			// keeping track of the branches that have already been processed
			// Very bad idea!
			// if the presets change at all this starts loading values into whatever branch it wants
		//	if((!branch && (branchesLoaded < nNumBranches)&&(GetBranch(branchesLoaded)->loadedPreset != TRUE)))
		//		branch = (CATHierarchyBranch2*)GetBranch(branchesLoaded);

			if (!branch)
				for (int i = 0; i < GetNumBranches(); i++)
				{
					if (GetBranch(i) && GetBranch(i)->loadedPreset == FALSE)
					{
						branch = (CATHierarchyBranch2*)GetBranch(i);
						break;
					}
				}

			if (branch) {
				branch->LoadPreset(load, t, slot);
				branchesLoaded++;
				branch->loadedPreset = TRUE;
			}
			else {
				//					load->WarnNoSuchBranch();
				load->SkipBranch();
			}
		}	break;

		// This is end of branch.  We are done.  If this is the
		// root, there's no branch to end, but that will be
		// picked up by the parser, as it counts branch levels,
		// so we don't worry about it.  Yaay!
		case motionEndBranch:
			done = true;
			continue;

			// Whoah, we've aborted for some reason!
		case motionAbort:
			return FALSE;
		}
	}

	return ok;
}

//	CATHierarchyBranch2  Implementation.
//
//	Make it work
//
//	Steve T. 12 Nov 2002
void CATHierarchyBranch2::Init()
{
	pblock = NULL;
	nActiveLeaf = -1;
}

//
// This function gets called when the leafs list is
// changing.  We pass it on to our leafs and our
// branches.
//
void CATHierarchyBranch2::NotifyLeaves(UINT msg, int &data)
{
	int i;
	for (i = 0; i < GetNumLeaves(); i++)
		if (GetLeaf(i)) GetLeaf(i)->NotifyLeaf(msg, data);

	for (i = 0; i < GetNumBranches(); i++)
		if (GetBranch(i)) ((CATHierarchyBranch2*)GetBranch(i))->NotifyLeaves(msg, data);

}
//
// Whether we're loading or not, we never start with any
// references so there's nothing to initialise.
//
CATHierarchyBranch2::CATHierarchyBranch2(BOOL loading)
	: CATHierarchyBranch(loading)
{
	Init();
	CATHierarchyBranch2Desc.MakeAutoParamBlocks(this);
}

// When deleting, set our catroot and weights pointers
// to NULL, just in case they for some stupid reason
// get used after we've deleted.
CATHierarchyBranch2::~CATHierarchyBranch2()
{
	DeleteAllRefs();
}

BOOL CATHierarchyBranch2::IsKeyAtTime(TimeValue t, DWORD flags)
{
	BOOL isKey = FALSE;

	//	int nActiveLeaf = GetCATRoot()->GetActiveLeaf();
		// This code wascommented out between 2.4 and 2.444
		// But it is importatn for all the spinners on the CATWindow
		// So I put it back in. Next time leave a note!
	int nNumLeaves = GetNumLeaves();
	if ((nActiveLeaf >= 0) && (nActiveLeaf < nNumLeaves))
	{
		CATHierarchyLeaf* leaf = GetLeaf(nActiveLeaf);
		DbgAssert(leaf);
		isKey = (leaf != NULL) ? leaf->IsKeyAtTime(t, flags) : FALSE;
	}
	else for (int i = 0; i < nNumLeaves; i++)
	{
		CATHierarchyLeaf* leaf = GetLeaf(i);
		if (leaf != NULL && leaf->IsKeyAtTime(t, flags)) return TRUE;
	}

	return isKey;
}

// SetValue() sets all leaf branches to this value if there is
// no active leaf branch.  If there is, SetValue() is called
// only on that leaf branch.
//
void CATHierarchyBranch2::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	int nNumLeaves = GetNumLeaves();
	if (nActiveLeaf == -1)
	{
		for (int i = 0; i < nNumLeaves; i++)
			if (GetLeaf(i))
				GetLeaf(i)->SetValue(t, val, commit, method);
	}
	else
	{
		DbgAssert(nActiveLeaf >= 0 && nActiveLeaf < nNumLeaves);
		CATHierarchyLeaf* leaf = GetLeaf(nActiveLeaf);
		if (leaf)
			leaf->SetValue(t, val, commit, method);
	}
}

// GetValue() returns the average of all leaf branches if there
// is no active leaf branch.  If there is, GetValue() is called
// only on that leaf branch.
//
void CATHierarchyBranch2::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod)
{
	/*
	 *	Branches arn't usually quiered for thier value. Only when selected in TrackView
	 */
	*(float*)val = 0.0f;

	int nNumLeaves = GetNumLeaves();
	if ((nNumLeaves > 0) && (nActiveLeaf < nNumLeaves))
	{
		if (nActiveLeaf == -1) {
			float leafVal = 0.0f;
			for (int i = 0; i < nNumLeaves; i++) {
				CATHierarchyLeaf* leaf = GetLeaf(i);
				if (leaf && leaf->pblock) leaf->GetValue(t, (void*)&leafVal, valid, CTRL_ABSOLUTE);
				*(float*)val += leafVal;
			}
			*(float*)val /= (float)nNumLeaves;
		}
		else
		{
			DbgAssert((nActiveLeaf >= 0) && (nActiveLeaf < nNumLeaves));
			CATHierarchyLeaf* leaf = GetLeaf(nActiveLeaf);
			if (leaf && leaf->pblock)
				leaf->GetValue(t, val, valid, CTRL_RELATIVE);
		}
	}

	valid.SetInstant(t);
}

void CATHierarchyBranch2::AddLimb(LimbData2* limb)
{
	int numLimbs = pblock->Count(PB_LIMBS_TAB);
	pblock->SetCount(PB_LIMBS_TAB, numLimbs + 1);
	pblock->SetValue(PB_LIMBS_TAB, 0, (ReferenceTarget*)limb, numLimbs);
}

TSTR CATHierarchyBranch2::GetLimbName(int index)
{
	return GetLimb(index)->GetName();
}

void CATHierarchyBranch2::AddControllerRef(Control* ctrl)
{
	int nNumControllers = pblock->Count(PB_CONTROLLERTAB);
	pblock->SetCount(PB_CONTROLLERTAB, nNumControllers + 1);
	pblock->SetValue(PB_CONTROLLERTAB, 0, (ReferenceTarget*)ctrl, nNumControllers);
}

int CATHierarchyBranch2::NumSubs() {
	return GetNumLeaves() + GetNumBranches();
}

Animatable* CATHierarchyBranch2::SubAnim(int i)
{
	if (i < GetNumLeaves())
		return GetLeaf(i);
	else if (i - GetNumLeaves() < GetNumBranches())
		return GetBranch(i - GetNumLeaves());
	return NULL;
}

TSTR CATHierarchyBranch2::SubAnimName(int i)
{
	if (g_bSavingCAT3Rig) GetString(IDS_SAVINGRIG);
	int nNumLeaves = GetNumLeaves();
	if (i < nNumLeaves) return GetLeafName(i);
	else if ((i - nNumLeaves) < GetNumBranches()) return GetBranchName(i - nNumLeaves);

	return _T("");
}

class BranchAndRemap
{
public:
	CATHierarchyBranch2* branch;
	CATHierarchyBranch2* clonedbranch;
	RemapDir* remap;

	BranchAndRemap(CATHierarchyBranch2* branch, CATHierarchyBranch2* clonedbranch, RemapDir* remap) {
		this->branch = branch;
		this->clonedbranch = clonedbranch;
		this->remap = remap;
	}
};

void CATHierarchyBranch2CloneNotify(void *param, NotifyInfo *info)
{
	UNREFERENCED_PARAMETER(info);
	BranchAndRemap *branchAndremap = (BranchAndRemap*)param;
	TSTR branchname = branchAndremap->branch->GetBranchName();

	//	void	 SetCATRoot(CATHierarchyRoot* newcatroot);
	CATHierarchyRoot *oldroot = branchAndremap->branch->GetCATRoot();
	CATHierarchyRoot *clonedroot = branchAndremap->clonedbranch->GetCATRoot();
	CATHierarchyRoot *newroot = (CATHierarchyRoot*)branchAndremap->remap->FindMapping(oldroot);
	if (newroot && (newroot != clonedroot)) {
		branchAndremap->clonedbranch->SetCATRoot(newroot);
	}
	else {
		//		DbgAssert(0);
	}

	/////////////////////////////////////////////////////////////////////////////

	CATHierarchyBranch *oldparent = branchAndremap->branch->GetBranchParent();
	CATHierarchyBranch *clonedparent = branchAndremap->clonedbranch->GetBranchParent();
	CATHierarchyBranch *newparent = (CATHierarchyBranch*)branchAndremap->remap->FindMapping(oldparent);
	if (newparent && (newparent != clonedparent)) {
		branchAndremap->clonedbranch->SetBranchParent(newparent);
	}

	/////////////////////////////////////////////////////////////////////////////

	//	for(int i=0; i< branchAndremap->clonedbranch()->GetNumLeaves(); i++)
	int i = 0;
	int deletedleaves = 0;
	while (i < branchAndremap->branch->GetNumLeaves())
	{
		CATHierarchyLeaf *oldbranchleaf = branchAndremap->branch->GetLeaf(i);
		CATHierarchyLeaf *clonedbranchleaf = branchAndremap->clonedbranch->GetLeaf(i - deletedleaves);
		//TSTR leafname = branchAndremap->branch->GetLeafName(i);

		CATHierarchyLeaf *newleaf = (CATHierarchyLeaf*)branchAndremap->remap->FindMapping(oldbranchleaf);

		if (newleaf && (newleaf != clonedbranchleaf)) {
			branchAndremap->clonedbranch->SetLeaf(i - deletedleaves, newleaf);
		}
		else if (!newleaf && (oldbranchleaf == clonedbranchleaf)) {
			// Our leaf was not cloned, so we still point to our old leaf
			branchAndremap->clonedbranch->RemoveLeaf(clonedbranchleaf);
			deletedleaves++;
		}
		i++;
	}
	/////////////////////////////////////////////////////////////////////////////
	i = 0;
	int deletedctrls = 0;
	while (i < branchAndremap->branch->GetNumControllerRefs())
	{
		Control *oldcatgraphctrl = branchAndremap->branch->GetControllerRef(i);
		Control *clonedoldcatgraphctrl = branchAndremap->clonedbranch->GetControllerRef(i - deletedctrls);
		Control *newcatgraphctrl = (Control*)branchAndremap->remap->FindMapping(oldcatgraphctrl);

		if (newcatgraphctrl && (newcatgraphctrl != clonedoldcatgraphctrl)) {
			branchAndremap->clonedbranch->SetControllerRef(i - deletedctrls, newcatgraphctrl);
		}
		else if (!newcatgraphctrl) {
			// Our leaf was not cloned, so we still point to our old leaf
			branchAndremap->clonedbranch->SetControllerRef(i - deletedctrls, NULL);
			deletedctrls++;
		}
		i++;
	}

	UnRegisterNotification(CATHierarchyBranch2CloneNotify, branchAndremap, NOTIFY_NODE_CLONED);
	delete branchAndremap;
}

RefTargetHandle CATHierarchyBranch2::Clone(RemapDir& remap)
{

	CATHierarchyBranch2 *newCATHierarchyBranch = new CATHierarchyBranch2(TRUE);
	remap.AddEntry(this, newCATHierarchyBranch);

	newCATHierarchyBranch->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	RegisterNotification(CATHierarchyBranch2CloneNotify, new BranchAndRemap(this, newCATHierarchyBranch, &remap), NOTIFY_NODE_CLONED);

	BaseClone(this, newCATHierarchyBranch, remap);
	return newCATHierarchyBranch;
};

BOOL CATHierarchyBranch2::CleanupTree()
{
	int i = 0;
	while (i < GetNumBranches()) {
		if (!GetBranch(i)->CleanupTree()) {
			i++;
		}
	}

	// if we have just removed our last sub branch,
	// kill ourselves as  we have no reason to live
	BOOL selfdestructing = (GetNumLeaves() == 0 && GetNumBranches() == 0);
	if (selfdestructing)
	{
		SelfDestruct();
	}
	return selfdestructing;
}
