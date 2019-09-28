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
	This is a float controller that acts like Max's float list...
	sort of.  It contains one controller that is always in the
	first subanim position.  The rest of the subanim positions
	can be expanded.  There are no weights.  Instead, weights are
	found in the CATHierarchyBranch controller pointed to by
	'weights'.  This controller is the weights of the CAT hierarchy
	tree.

	The layers and their names are all stored in the weights
	controller.  This controller serves as both a layer list
	and a weights list.  The distinction is that a weights list
	has its 'weights' set to NULL.
 **********************************************************************/

#include "CATPlugins.h"

#include "CATHierarchyFunctions.h"

#include "Hub.h"
#include "CATHierarchyLeaf.h"
#include "CATHierarchyBranch2.h"

 //	CATHierarchyLeaf  Implementation.
 //
 //	Make it work
 //
class CATHierarchyLeafClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading);  return new CATHierarchyLeaf(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATHIERARCHYLEAF); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return CATHIERARCHYLEAF_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATHierarchyLeaf"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static CATHierarchyLeafClassDesc CATHierarchyLeafDesc;
ClassDesc2* GetCATHierarchyLeafDesc() { return &CATHierarchyLeafDesc; }

void CATHierarchyLeaf::GetClassName(TSTR& s) { s = GetString(IDS_CL_CATHIERARCHYLEAF); }

//
// Destruct the leaf?
void CATHierarchyLeaf::MaybeDestructLeaf(LimbData2* pLimb)
{
	// We need to detect if this value is still necessary
	// If the value is shared among multiple limbs, then
	// do not remove it (unless there is only 1 limb)
	// NOTE!  This runs under the assumption that the
	// CATHierarchy looks something like
	// Attribute (OffsetRot, FootLift, etc) <-- This value can be welded
	//		-> Value (eg "X", "LiftHeight" etc)
	//			-> CATHierarchyLeaf
	// To see if we are used on multiple limbs, check
	// our parents parent (the attribute).
	CATHierarchyBranch2* pBranch = GetLeafParent();
	if (pBranch != NULL)
	{
		// Remove the limb pointer from our owning parent.  This will trigger a reaction,
		// and if we need to be removed, we will be.

		CATHierarchyBranch* pAttr = pBranch->GetBranchParent();
		if (pAttr != NULL)
		{
			if (pAttr->GetNumLimbs() > 0)
				pAttr->RemoveLimb(pLimb);
			else
				pAttr->SelfDestruct();
		}
	}
}
//
// This function gets called when the layer list is
// changing.  We're either appending, inserting, or
// removing a layer.
//
void CATHierarchyLeaf::NotifyLeaf(UINT msg, int &data)
{
	switch (msg) {
	case CAT_LAYER_ADD:
		data = AddLayer();
		break;

	case CAT_LAYER_REMOVE:
		RemoveLayer(data);
		break;

	case CAT_LAYER_SELECT:
		// This affects which layer is currently active.
		// It is used for keyframe ranges so we can use
		// TrackView to move and stretch keys for one
		// layer.
//			nActiveLayer = data;
		break;
	}
}

RefTargetHandle CATHierarchyLeaf::GetReference(int i)
{
	UNREFERENCED_PARAMETER(i);
	return pblock;
}

// Calculate layer names for the CATMotion layers stored in the parameter block
class CATHierarchyLeafLayerNames : public PBAccessor
{
	TSTR GetLocalName(ReferenceMaker* owner, ParamID id, int tabIndex)
	{
		// The below assumption is a constraint of max
		DbgAssert(owner != NULL && owner->ClassID() == CATHIERARCHYLEAF_CLASS_ID);
		CATHierarchyLeaf* pLeaf = static_cast<CATHierarchyLeaf*>(owner);

		CATHierarchyLeaf* weights = pLeaf->GetWeights();
		if (weights)
			return weights->GetLayerName(tabIndex);

		return pLeaf->GetLayerName(tabIndex);
	}

public:
	static CATHierarchyLeafLayerNames* GetInstance()
	{
		static CATHierarchyLeafLayerNames instance;
		return &instance;
	}
};

static ParamBlockDesc2 cathierarchyleaf_param_blk(CATHierarchyLeaf::PBLOCK_REF, _T("Leaf Params"), 0, &CATHierarchyLeafDesc,
	P_AUTO_CONSTRUCT, CATHierarchyLeaf::CATLEAFPB,

	CATHierarchyLeaf::PB_LAYERTAB, _T("Layers"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_VARIABLE_SIZE + P_COMPUTED_NAME, IDS_LAYERNAME,
		p_end,
	CATHierarchyLeaf::PB_LAYERNAMESTAB, _T("LayerNames"), TYPE_STRING_TAB, 0, P_VARIABLE_SIZE, IDS_LAYERNAMES,
		p_accessor, CATHierarchyLeafLayerNames::GetInstance(),
		p_end,

	CATHierarchyLeaf::PB_LEAFPARENT, _T("LeafParent"), TYPE_REFTARG, P_NO_REF + P_INVISIBLE, 0,
		p_end,

	CATHierarchyLeaf::PB_WEIGHTS, _T("Weights"), TYPE_REFTARG, P_NO_REF + P_INVISIBLE, 0,
		p_end,
	CATHierarchyLeaf::PB_LIMBDATA_DEPRECATED, _T(""), TYPE_REFTARG, P_NO_REF + P_INVISIBLE, IDS_CL_LIMBDATA,
		p_end,
	CATHierarchyLeaf::PB_DEFAULTVAL, _T("DefaultVal"), TYPE_FLOAT, 0, 0,
		p_default, 0.0f,
		p_end,
	CATHierarchyLeaf::PB_ACTIVELAYER, _T("ActiveLayer"), TYPE_INT, 0, 0,
		p_default, 0,
		p_end,
	p_end
);

//	Steve T. 12 Nov 2002
void CATHierarchyLeaf::Init()
{
	pblock = NULL;
}

//
// We don't allow copying or cloning, so just initialise and
// do nothing else.
CATHierarchyLeaf::CATHierarchyLeaf(const CATHierarchyLeaf &ctrl)
{
	UNREFERENCED_PARAMETER(ctrl);
	Init();
	CATHierarchyLeafDesc.MakeAutoParamBlocks(this);
}

//
//	If we're loading then we don't want to make new default controllers
//	as they already exist (in the file).  Max will hook them up later
//  for us. (probably using SetReference())
//
CATHierarchyLeaf::CATHierarchyLeaf(BOOL loading)
{
	Init();
	CATHierarchyLeafDesc.MakeAutoParamBlocks(this);
}

// When deleting, set our weights pointer to NULL,
// just in case it for some stupid reason gets
// used after we've deleted.
CATHierarchyLeaf::~CATHierarchyLeaf()
{
	DeleteAllRefs();
}

// Copy / Clone
void CATHierarchyLeaf::Copy(Control *from)
{
	UNREFERENCED_PARAMETER(from);
}

//
// Creates and adds a new layer and returns its subanim index.
// The layer has a default name
int CATHierarchyLeaf::AddLayer(int newLayers/*=1*/)
{
	// this might happen because leaves are created with
	// 1 default layer.
	int nOldNumLayers = pblock->Count(PB_LAYERTAB);
	if (newLayers == 0) return nOldNumLayers;

	pblock->EnableNotifications(FALSE);

	int nNewNumLayers = nOldNumLayers + newLayers;
	pblock->SetCount(PB_LAYERTAB, nNewNumLayers);
	float defaultval = GetDefaultVal();

	for (int i = nOldNumLayers; i < nNewNumLayers; i++)
	{
		Control* ctrlNewLayer = NewDefaultFloatController();
		ctrlNewLayer->SetValue(GetCOREInterface()->GetTime(), (void*)&defaultval, 1);

		// initialise the layer to the default
		pblock->SetControllerByID(PB_LAYERTAB, i, ctrlNewLayer, FALSE);
		//	pblock->SetValue(PB_LAYERTAB,0, GetDefaultVal(), i);
	}
	// If we are the weights branch maintain the tabLayerNames
	if (!GetWeights())
	{
		pblock->SetCount(PB_LAYERNAMESTAB, nNewNumLayers);
		for (int i = nOldNumLayers; i < nNewNumLayers; i++)
		{
			TSTR layername;
			// the name of the first layer is never actually used,
			// but we need to maintain the table
			layername.printf(_T("%s%02d"), GetString(IDS_LYR), i + 1);
			pblock->SetValue(PB_LAYERNAMESTAB, 0, layername, i);
		}
	}

	pblock->EnableNotifications(TRUE);
	return nNewNumLayers - 1;
}

void CATHierarchyLeaf::RemoveLayer(const int nLayerIndex)
{
	CATHierarchyLeaf* weights = GetWeights();
	int nNumLayers = GetNumLayers();

	pblock->EnableNotifications(FALSE);

	for (int i = nLayerIndex; i < (nNumLayers - 1); i++) {
		Control *ctrlLayer = pblock->GetControllerByID(PB_LAYERTAB, i + 1);
		pblock->SetControllerByID(PB_LAYERTAB, i, ctrlLayer, FALSE);

		if (!weights) SetLayerName(i, GetLayerName(i + 1));
	}

	nNumLayers--;

	Control *nullCtrl = NULL;
	pblock->SetControllerByID(PB_LAYERTAB, nNumLayers, nullCtrl, FALSE);
	pblock->SetCount(PB_LAYERTAB, nNumLayers);
	pblock->SetCount(PB_LAYERNAMESTAB, nNumLayers);

	pblock->EnableNotifications(TRUE);
}

int CATHierarchyLeaf::NumSubs()
{
	return 1;
}

Animatable* CATHierarchyLeaf::SubAnim(int i)
{
	DbgAssert(i == 0);
	return pblock;
}

TSTR CATHierarchyLeaf::SubAnimName(int i)
{
	return _T("");
}

void CATHierarchyLeaf::SetReference(int i, RefTargetHandle rtarg)
{
	if (pblock != NULL && rtarg == NULL)
	{
		// Clean our parent pointer
		CATHierarchyBranch2* pParent = GetLeafParent();
		if (pParent != NULL)
		{
			pParent->RemoveLeaf(this);
		}
	}
	pblock = (IParamBlock2*)rtarg;
}

RefResult CATHierarchyLeaf::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	UNREFERENCED_PARAMETER(partID);
	UNREFERENCED_PARAMETER(hTarget);
	UNREFERENCED_PARAMETER(changeInt);
	UNREFERENCED_PARAMETER(message);

	return REF_SUCCEED;
};

void CATHierarchyLeaf::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	if (method == CTRL_ABSOLUTE) *(float*)val = 0.0f;
	CATHierarchyLeaf* weights = GetWeights();
	if (!weights) {
		valid = FOREVER;
		return;
	}

	int numWeights = weights->GetNumLayers();
	int numLayers = GetNumLayers();
	DbgAssert(numWeights == numLayers);
	UNREFERENCED_PARAMETER(numWeights);

	float dLayerVal, dWeight = 1.0f;

	// add on the default value first.
	// We only store the deviation from the default, and
	// this means we can add many layers together and it won't destroy
	// the walkcycle...

	if (numLayers < 1)
	{
		*(float*)val = GetDefaultVal();
		return;
	}

	for (int i = 0; i < numLayers; i++)
	{
		if (i == 0)
			dWeight = 1.0f;		// Our BaseLayer is always one
		else
		{
			Control* pLayerWeight = weights->GetLayer(i);
			DbgAssert(pLayerWeight != NULL);
			if (pLayerWeight != NULL)
				pLayerWeight->GetValue(t, (void*)&dWeight, valid, CTRL_ABSOLUTE);
		}
		dLayerVal = pblock->GetFloat(PB_LAYERTAB, t, i);

		// this is now absolute weightings.
		// as you turn up the weight on any layer, it overrides previous layers
		*(float*)val = *(float*)val + ((dLayerVal - *(float*)val) * dWeight);
	}

	valid.SetInstant(t);
}
// SetValue pipes through to the first subanim, or the selected subanim.
//
void CATHierarchyLeaf::SetValue(TimeValue t, void *val, int, GetSetMethod)
{
	int nNumLayers = GetNumLayers();
	if (nNumLayers < 1) return;

	int nActiveLayer = -1;
	float dWeight = 0.0f;
	CATHierarchyLeaf* weights = GetWeights();
	if (weights)
		nActiveLayer = weights->GetActiveLayer();
	else return;
	//		nActiveLayer = GetActiveLayer();

	if ((nActiveLayer >= 0) && (nActiveLayer < nNumLayers))
	{
		if (nActiveLayer == 0)
			dWeight = 1.0f;		// Our BaseLayer is always one
		else
		{
			Interval iv = FOREVER;
			Control* pLayerWeight = weights->GetLayer(nActiveLayer);
			DbgAssert(pLayerWeight != NULL);
			if (pLayerWeight != NULL)
				pLayerWeight->GetValue(t, (void*)&dWeight, iv, CTRL_ABSOLUTE);
		}

		if (dWeight > 0.5f)
			pblock->SetValue(PB_LAYERTAB, t, *(float*)val, nActiveLayer);
	}
}

BOOL CATHierarchyLeaf::IsKeyAtTime(TimeValue t, DWORD flags)
{
	int nNumLayers = GetNumLayers();
	if (nNumLayers < 1) return FALSE;

	int nActiveLayer;
	CATHierarchyLeaf* weights = GetWeights();
	if (weights)
		nActiveLayer = weights->GetActiveLayer();
	else
		nActiveLayer = GetActiveLayer();

	if ((nActiveLayer >= 0) && (nActiveLayer < nNumLayers))
		return pblock->GetControllerByID(PB_LAYERTAB, nActiveLayer)->IsKeyAtTime(t, flags);

	return FALSE;
}

class LeafAndRemap
{
public:
	CATHierarchyLeaf* leaf;
	CATHierarchyLeaf* clonedleaf;
	RemapDir* remap;

	LeafAndRemap(CATHierarchyLeaf* leaf, CATHierarchyLeaf* clonedleaf, RemapDir* remap) {
		this->leaf = leaf;
		this->clonedleaf = clonedleaf;
		this->remap = remap;
	}
};

void CATHierarchyLeafCloneNotify(void *param, NotifyInfo *info)
{
	UNREFERENCED_PARAMETER(info);
	LeafAndRemap *leafAndremap = (LeafAndRemap*)param;

	CATHierarchyBranch *branchparent = leafAndremap->leaf->GetLeafParent();
	if (branchparent) {
		CATHierarchyBranch *clonedbranchparent = leafAndremap->clonedleaf->GetLeafParent();
		CATHierarchyBranch *newbranchparent = (CATHierarchyBranch*)leafAndremap->remap->FindMapping(branchparent);
		if (newbranchparent && (newbranchparent != clonedbranchparent)) {
			leafAndremap->clonedleaf->SetLeafParent(newbranchparent);
		}
	}

	CATHierarchyLeaf *weightsctrl = leafAndremap->leaf->GetWeights();
	if (weightsctrl) {
		CATHierarchyLeaf *clonedweightsctrl = leafAndremap->clonedleaf->GetWeights();
		CATHierarchyLeaf *newweightsctrl = (CATHierarchyLeaf*)leafAndremap->remap->FindMapping(weightsctrl);
		if (newweightsctrl && (newweightsctrl != clonedweightsctrl)) {
			leafAndremap->clonedleaf->SetWeights(newweightsctrl);
		}
	}
	UnRegisterNotification(CATHierarchyLeafCloneNotify, leafAndremap, NOTIFY_NODE_CLONED);

	delete leafAndremap;
}

RefTargetHandle CATHierarchyLeaf::Clone(RemapDir& remap) {
	CATHierarchyLeaf *newCATHierarchyLeaf = new CATHierarchyLeaf(TRUE);
	remap.AddEntry(this, newCATHierarchyLeaf);

	newCATHierarchyLeaf->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	RegisterNotification(CATHierarchyLeafCloneNotify, new LeafAndRemap(this, newCATHierarchyLeaf, &remap), NOTIFY_NODE_CLONED);

	BaseClone(this, newCATHierarchyLeaf, remap);
	return newCATHierarchyLeaf;
};
