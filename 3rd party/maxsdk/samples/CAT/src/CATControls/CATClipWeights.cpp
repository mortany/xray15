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
		The ClipWeights controller is a special little ditty.  It
		keeps a float controller for each layer and provides a
		function to get the weight for a particular layer.  The
		value is multiplied by weights all up the hierarchy until
		we reach the root.  The weight for each layer at an instant
		in time is cached in each weight controller.  This prevents
		a massive number of function calls when we have even a
		modest number of clip keyframes.

		GetValue() on a weights controller has a special function.
		It adds up the weights for all absolute layers and returns
		the 1.0 minus that number.  If the difference is less than
		zero, zero is returned.  The purpose of this is to inform
		the CAT Motion Hierarchy whether it should evaluate or not
 **********************************************************************/

#include "CATPlugins.h"
#include "CATClipWeights.h"
#include "CATClipRoot.h"

 //keeps track of whether an FP interface desc has been added to the CATClipMatrix3 ClassDesc
static bool catclipWeightInterfacesAdded = false;

class CATClipWeightsClassDesc : public ClassDesc2 {
public:
	int IsPublic() { return FALSE; }
	void *Create(BOOL loading = FALSE) {
		CATClipWeights* ctrl = new CATClipWeights(loading);
		if (!catclipWeightInterfacesAdded) {
			AddInterface(ctrl->GetDescByID(I_LAYERWEIGHTSCONTROL_FP));
			catclipWeightInterfacesAdded = true;
		}
		return ctrl;
	}

	const TCHAR *ClassName() { return GetString(IDS_CL_CATCLIPWEIGHTS); }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID ClassID() { return CATCLIPWEIGHTS_CLASS_ID; }
	const TCHAR *Category() { return GetString(IDS_CATEGORY); }

	const TCHAR *InternalName() { return _T("CATClipWeights"); }
	HINSTANCE HInstance() { return hInstance; }
};

static CATClipWeightsClassDesc CATClipWeightsDesc;
ClassDesc2* GetCATClipWeightsDesc() { return &CATClipWeightsDesc; }

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc layer_weights_FPinterface(
	I_LAYERWEIGHTSCONTROL_FP, _T("ILayerWeightsFPInterface"), 0, NULL, FP_MIXIN,

	CATClipWeights::fnGetCombinedWeight, _T("GetCombinedWeight"), 0, TYPE_FLOAT, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	properties,

	CATClipWeights::propGetParentWeightController, FP_NO_FUNCTION, _T("ParentWeightController"), 0, TYPE_CONTROL,

	p_end
);

FPInterfaceDesc* CATClipWeights::GetDescByID(Interface_ID id) {
	if (id == I_LAYERWEIGHTSCONTROL_FP) return &layer_weights_FPinterface;
	return CATClipValue::GetDescByID(id);
}

Control* CATClipWeights::NewWhateverIAmController() {
	float one = 1.0f;
	Control *ctrl = NewDefaultFloatController();
	ctrl->SetValue(0, (void*)&one, 1, CTRL_ABSOLUTE);
	return ctrl;
}

// When the layers list is resized we also have to resize
// our cache tables.  We don't have to initialise the
// values cache, but do anyway in case some drongo asks
// for the weight at negative infinity.  Also set all
// new weights to 1.
//
void CATClipWeights::ResizeList(int n, BOOL loading/*=FALSE*/)
{
	int i = tabLayers.Count();
	CATClipValue::ResizeList(n, loading);

	tabCacheIntervals.SetCount(n);
	tabCacheValues.SetCount(n);

	float one = 1.0f;

	for (; i < n; i++) {
		tabCacheIntervals[i] = NEVER;
		tabCacheValues[i] = 0.0f;
		if (!loading) {
			SetFlag(CLIP_FLAG_DONT_INVALIDATE_CACHE);
			tabLayers[i]->SetValue(0, (void*)&one, 1, CTRL_ABSOLUTE);
			ClearFlag(CLIP_FLAG_DONT_INVALIDATE_CACHE);
		}
	}

	// Weights weren't being recached when a layer was deleted.
	// The following line was added to cure it. =)
	InvalidateCache(-1);
}

// This is only used by the PLCB for backwards compatiblity
//void CATClipWeights::SetWeightController(CATClipThing* dad){ if(dad->daddy) weights = dad->daddy->weights; }

CATClipWeights* CATClipWeights::GetWeightsCtrl() {
	return this;
}

CATClipWeights* CATClipWeights::ParentWeights() {
	//	return weights ? weights : ((daddy&&daddy->daddy) ? daddy->daddy->weights : NULL);
	return weights;
}

// The GetWeight function calculates the weight for a layer
// at this level, multiplied by the weights at all levels
// above.  We access the level above by getting our daddy's
// daddy, which is the parent branch of our owning branch.
// If that happens to be NULL, then this is the root weights
// controller.
//
float CATClipWeights::GetWeight(TimeValue t, int index, Interval& valid)
{
	// ST 18/12/03 - CATMotion is ALWAYS at weight 1
	// GB 11/03/04 - modified this test to include an index less than
	// zero (in cases where someone queries the weight for the active
	// layer, and no active layer is selected (index -1)).
	if (index <= 0 || index >= GetNumLayers()) return 1.0f;

	// Check our cached value first, before doing more
	// work than we need to.
	if (tabCacheIntervals[index].InInterval(t))
	{
		// Our ClipValues need to have thier validity intervals pruned by the weight view intervals
		valid &= tabCacheIntervals[index];
		return tabCacheValues[index];
	}

	ICATParentTrans* pParentTrans = GetCATParentTrans();
	if (pParentTrans == NULL)
		return 1.0f;

	// GB 27-Jan-2004: Update the rig colours, which may depend
	// on weights (in certain modes).
	switch (pParentTrans->GetEffectiveColourMode()) {
	case COLOURMODE_ACTIVE:
	case COLOURMODE_BLEND:
		// The CATParent will only update colours once per frame
		pParentTrans->UpdateColours(FALSE, FALSE);
		break;
	}

	// Get our local weight.
	float dWeight = 1.0f;
	/*	Special case for the weight view intervals
	 *	our weights validity interval does not depend on the ClipValues validity
	 *  intervals, or the intervals of any children in the weight view hierarchy,
	 *  but they depend on us because the GetValues go up the hierarchy towards the root
	 *  instead of the other way round. We & the two intervals at the end to give a
	 *  smaller interval to the controller who asked us. This means
	 *  that if a leaf controller is changing all the time and we aren't we can
	 *  still be valid and make use of our caches. If we are never valid we must
	 *  stop any of the children or ClipValues from being valid.
	 */
	Interval weightValid = FOREVER;

	if (tabLayers[index])
		tabLayers[index]->GetValue(t, (void*)&dWeight, weightValid, CTRL_ABSOLUTE);

	// If we're not at the root level we have to work up the
	// hierarchy.  Call GetWeight on the next-level-up weights
	// controller and hope it has a cached value...

	// PT - 1/05/2004 here we can stop the traversal up the hierarchy if this flag is set
	// This flag needs to be  local to this weights controller and per layer to be useful
//	if (daddy->daddy && root->WeightHierarchyEnabled(index)) {
//	if(root->WeightHierarchyEnabled(index)){
	if (weights)
		dWeight *= weights->GetWeight(t, index, weightValid);
	// in future, hubs weights controllers will not have a pointer to a parent weights controller
	else
	{
		CATClipRoot* pClipRoot = GetRoot();
		if (pClipRoot != NULL)
			dWeight *= pClipRoot->GetWeight(t, index, weightValid);
	}
	//	}

		// Cache and clamp the weight and return it.
	tabCacheIntervals[index] = weightValid;//valid;

	// Our ClipValues need to have thier validity intervals pruned by the weight view intervals
	valid &= weightValid;
	return (tabCacheValues[index] = (dWeight > 1.0f) ? 1.0f : (dWeight < 0.0f ? 0.0f : dWeight));
}

//////////////////////////////////////////////////////////////////////////
// These functions used to reside in CATClipBranch,
// but they make just as much sense here, and I am
// trying to purge CAT of the whole hierarchy

// Returns the colour of the active layer, scaled by the effective
// weight of the active layer.
Color CATClipWeights::GetActiveLayerColour(TimeValue t)
{
	NLAInfo* pInfo = GetSelectedNLAInfo();
	Color col(1.0f, 0.0f, 0.0f);
	if (pInfo != NULL)
	{
		col = pInfo->GetColour();
		Interval iv = FOREVER;
		col *= GetWeight(t, GetSelectedIndex(), iv);
	}
	return col;
}

// Blends the colour of each layer that contributes to a blend.
Color CATClipWeights::GetBlendedLayerColour(TimeValue t)
{
	BOOL bWeighted = TRUE;
	float dClipWeight = 0.0f;
	Color col(1.0f, 0.0f, 0.0f);
	CATClipRoot* pRoot = GetRoot();
	if (pRoot == NULL)
		return col;

	int nSoloLayer = pRoot->GetSoloLayer();
	if (nSoloLayer >= 0)
	{
		NLAInfo* pSoloInfo = pRoot->GetLayer(nSoloLayer);
		if (pSoloInfo != NULL)
			col = pSoloInfo->GetColour();
		return col;
	}

	int nNumLayers = pRoot->NumLayers();
	Color colBlended;

	// This is the guts...
	for (int i = 0; i < nNumLayers; i++) {
		NLAInfo *layerinfo = pRoot->GetLayer(i);
		if (!layerinfo) continue;
		if (i == 0) { colBlended = layerinfo->GetColour(); continue; }

		Interval iv = FOREVER;
		if (nSoloLayer == i) bWeighted = FALSE;
		else if (nSoloLayer >= 0 && nSoloLayer != layerinfo->GetParentLayer()) continue;
		else if (!layerinfo->LayerEnabled()) continue;
		else dClipWeight = GetWeight(t, i, iv);

		if ((dClipWeight > 0.0f) || !bWeighted)
		{
			switch (layerinfo->GetMethod()) {
			case LAYER_RELATIVE:
			case LAYER_RELATIVE_WORLD:
				//colBlended += root->GetLayerColour(i) * dClipWeight;
				break;

			case LAYER_ABSOLUTE:
			case LAYER_CATMOTION:
				col = Color(layerinfo->GetColour());
				colBlended += (col - colBlended) * (bWeighted ? dClipWeight : 1.0f);
				break;
			}
		}
	}

	return colBlended;
}

// Override CATMessage() so that the cache gets invalidated
// if a weight at this level or any level above has changed.
//
void CATClipWeights::CATMessage(TimeValue t, UINT msg, int data)
{
	switch (msg) {
	case CLIP_WEIGHTS_CHANGED:
		InvalidateCache(data);
		break;

	case CLIP_LAYER_APPEND:
	case CLIP_LAYER_INSERT:
	case CLIP_LAYER_CLONE:
	case CLIP_LAYER_MOVEUP:
	case CLIP_LAYER_MOVEDOWN:
		CATClipValue::CATMessage(t, msg, data);
		InvalidateCache();
		break;

	default:
		CATClipValue::CATMessage(t, msg, data);
	}
}

void CATClipWeights::InvalidateCache(int index)
{
	int nNumLayers = tabCacheIntervals.Count();
	if (index == -1 || index >= nNumLayers) {
		for (index = 0; index < nNumLayers; index++) {
			tabCacheIntervals[index] = NEVER;
		}
	}
	else {
		tabCacheIntervals[index] = NEVER;
	}
}

CATClipWeights::CATClipWeights(BOOL loading/*=FALSE*/)
	: CATClipValue(loading),
	tabCacheIntervals(), tabCacheValues()
{
	InvalidateCache(-1);
}

// We need to process NotifyRefChanged() to detect if one of
// our weights has changed.  If so, we invalidate our cache
// and the caches of all weight controllers in our branch's
// sub-tree.  Don't forget to call the superclass version
// of this function, because Max won't.
//
RefResult CATClipWeights::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	switch (message) {
	case REFMSG_CHANGE:
		// I discovered that when I set a default weight when adding
		// a new layer in ResizeList(), this sneaky message handler
		// said "wooh yaay the weight changed so tell everyone else
		// to invalidate".  Good, yeah, but we hadn't yet passed the
		// message down the tree to add a new layer...  So it ended
		// up invalidating a cache for a value that didn't exist...
		// And you know what that means..... =(
		if (!TestFlag(CLIP_FLAG_DONT_INVALIDATE_CACHE))
		{
			ICATParentTrans* pParentTrans = GetCATParentTrans();
			if (pParentTrans == NULL)
				break;

			// Find which layer weight changed and invalidate caches.
			// This is handled by the CATMessages thingy.
			int nNumLayers = GetNumLayers();
			for (int i = 0; i < nNumLayers; i++)
			{
				if (hTarget == (RefTargetHandle)tabLayers[i])
				{

					pParentTrans->CATMessage(GetCOREInterface()->GetTime(), CLIP_WEIGHTS_CHANGED, i);
					pParentTrans->UpdateCharacter();
					pParentTrans->UpdateColours(FALSE);
					break;
				}
			}
		}
		break;
	}
	return CATClipValue::NotifyRefChanged(changeInt, hTarget, partID, message, propagate);
}

// These Get, and Set Values needed to be set to get the current layer weight because
// the clip saver has been
void CATClipWeights::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	Control* pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		pSelectedCtrl->GetValue(t, val, valid, method);
	else
		*(float*)val = 0.0f;
}

void CATClipWeights::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	Control* pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		pSelectedCtrl->SetValue(t, val, commit, method);
}

float	CATClipWeights::GetCombinedWeight(int index, TimeValue t)
{

	BOOL bWeighted = TRUE;
	float dClipWeight = 0.0f;

	CATClipRoot* pRoot = GetRoot();
	if (pRoot == NULL)
		return 0.0f;

	int nSoloLayer = pRoot->GetSoloLayer();

	Interval iv = FOREVER;
	if (index == 0) dClipWeight = 1.0f;
	else		 dClipWeight = GetWeight(t, index, iv); ;

	if (nSoloLayer == index) return dClipWeight;

	int nNumLayers = pRoot->NumLayers();

	// This is the guts...
	for (int i = (nNumLayers - 1); i > index; i--) {
		NLAInfo *layerinfo = pRoot->GetLayer(i);
		if (!layerinfo) continue;
		if (i == 0) { dClipWeight = 1.0f; continue; }

		if (nSoloLayer == i) return 0.0f;
		else if (!layerinfo->LayerEnabled()) continue;

		if ((dClipWeight > 0.0f) || !bWeighted)
		{
			Interval iv = FOREVER;
			switch (layerinfo->GetMethod()) {
			case LAYER_RELATIVE:
			case LAYER_RELATIVE_WORLD:
				break;
			case LAYER_ABSOLUTE:
			case LAYER_CATMOTION:
				dClipWeight *= 1.0f - GetWeight(t, i, iv);
				break;
			}
		}
	}
	return dClipWeight;
};

// always show the weights controllers in the track view.
// Removed 14-2-06.
// The keyframes for the weighting controlers would show up on the trackbar - annoying
// This would cause the local weights to be be deleted when other keyfrmes were deleted.
// I think this was added when CAT didn't have the option to show all layers in the CATClipValue::SubAnim
/*
Animatable* CATClipWeights::SubAnim(int i)
{
	if (i < nNumLayers)
		return tabLayers[i];
	return NULL;
}
*/
