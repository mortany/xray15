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

// Implements CATClipValue -- the superclass of CAT Clip Hierarchy value branches.

#include "CATPlugins.h"
#include <CATAPI/CATClassID.h>

#include "CATRigPresets.h"
#include "ICATParent.h"
#include "CATNodeControl.h"

#include "CATClipValue.h"
#include "CATClipRoot.h"
#include "CATClipWeights.h"

#include "../DataRestoreObj.h"

 // New method sets up th daddy pointer
CATClipValue* CreateClipValueController(ClassDesc2* desc, CATClipWeights* weights, ICATParentTrans* catparenttrans, BOOL loading)
{
	CATClipValue *layers = (CATClipValue*)CreateInstance(desc->SuperClassID(), desc->ClassID());
	DbgAssert(layers);
	DbgAssert(catparenttrans != NULL);
	layers->SetCATParentTrans(catparenttrans);
	layers->weights = weights;

	layers->ResetSetupVal();

	// Initialize to the current number of layers
	CATClipRoot* pRoot = nullptr;
	if (catparenttrans)
		pRoot = (CATClipRoot*)catparenttrans->GetLayerRoot();
	if (pRoot != NULL)
		layers->ResizeList(pRoot->NumLayers());

	layers->CATMessage(0, CLIP_LAYER_SELECT, layers->GetSelectedIndex());
	return layers;
}

// New method sets up th daddy pointer
CATClipWeights* CreateClipWeightsController(CATClipWeights* parentweights, ICATParentTrans* catparenttrans, BOOL loading)
{
	UNREFERENCED_PARAMETER(loading);
	DbgAssert(catparenttrans != NULL);
	CATClipWeights *weights = (CATClipWeights*)CreateInstance(GetCATClipWeightsDesc()->SuperClassID(), GetCATClipWeightsDesc()->ClassID());
	DbgAssert(weights);

	weights->SetCATParentTrans(catparenttrans);
	weights->weights = parentweights;

	weights->ResetSetupVal();

	// Update to the correct number of layers.
	CATClipRoot* pClipRoot = (CATClipRoot*)catparenttrans->GetLayerRoot();
	if (pClipRoot != NULL)
		weights->ResizeList(pClipRoot->NumLayers());

	weights->CATMessage(0, CLIP_LAYER_SELECT, weights->GetSelectedIndex());
	return weights;
}

BaseInterface* CATClipValue::GetInterface(Interface_ID id) {
	if (id == LAYERROOT_INTERFACE_FP) return m_pClipRoot ? m_pClipRoot->GetInterface(id) : NULL;
	if (id == I_LAYERCONTROL_FP) return (ILayerControlFP*)this;
	return Control::GetInterface(id);
}

void* CATClipValue::GetInterface(ULONG id) {
	return Control::GetInterface(id);
}

int CATClipValue::flagsBegin = NULL;

void CATClipValue::SetFlag(ULONG f, BOOL on) {
	HoldData(flags);
	if (on)	flags |= f;
	else	flags &= ~f;
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void CATClipValue::ClearFlag(ULONG f) {
	SetFlag(f, FALSE);
}

void CATClipValue::SetFlags(ULONG f) {
	HoldData(flags);
	flags = f;
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void CATClipValue::SetCATParentTrans(ICATParentTrans* pCATParent)
{
	m_pCATParentTrans = pCATParent;
	if (m_pCATParentTrans != NULL)
		m_pClipRoot = (CATClipRoot*)m_pCATParentTrans->GetLayerRoot();
	else
		m_pClipRoot = NULL;
}

CATClipWeights* CATClipValue::GetWeightsCtrl() { return weights; };
CATMode			CATClipValue::GetCATMode() { return (m_pCATParentTrans != NULL) ? m_pCATParentTrans->GetCATMode() : SETUPMODE; }
float			CATClipValue::GetCATUnits() { return (m_pCATParentTrans != NULL) ? m_pCATParentTrans->GetCATUnits() : 1.0f; }
Control*		CATClipValue::GetLayerRoot() { return GetRoot(); };
Control*		CATClipValue::GetLocalWeights() { return GetWeightsCtrl(); };
int				CATClipValue::GetTrackDisplayMethod() { return (m_pClipRoot != NULL) ? m_pClipRoot->GetTrackDisplayMethod() : -1; }
Control*		CATClipValue::GetSelectedLayerCtrl()
{
	int nSelected = GetSelectedIndex();
	return (nSelected >= 0) ? tabLayers[nSelected] : NULL;
};
//Control*		CATClipValue::IGetCATControl(){		return catcontrol;			};
//ECATParent*	CATClipValue::GetCATParent(){		return catparent ? catparent : root->GetCATParent(); };

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc layer_control_FPinterface(
	I_LAYERCONTROL_FP, _T("ILayerControlFPInterface"), 0, NULL, FP_MIXIN,

	ILayerControlFP::fnBakeCurrentLayerSettings, _T("BakeCurrentLayerSettings"), 0, TYPE_VOID, 0, 0,

	properties,

	ILayerControlFP::propGetLayerRoot, FP_NO_FUNCTION, _T("LayerRoot"), 0, TYPE_CONTROL,
	ILayerControlFP::propGetLocalWeights, FP_NO_FUNCTION, _T("Weights"), 0, TYPE_CONTROL,
	ILayerControlFP::propGetUseSetupController, ILayerControlFP::propSetUseSetupController, _T("UseSetupController"), 0, TYPE_BOOL,
	ILayerControlFP::propGetSetupController, FP_NO_FUNCTION, _T("SetupController"), 0, TYPE_CONTROL,
	ILayerControlFP::propGetRelativeBone, ILayerControlFP::propSetRelativeBone, _T("AdditiveToSetupValue"), 0, TYPE_BOOL,
	ILayerControlFP::propSelectedLayerCtrl, FP_NO_FUNCTION, _T("SelectedLayerCtrl"), 0, TYPE_CONTROL,
	ILayerControlFP::propNumLayers, FP_NO_FUNCTION, _T("NumLayers"), 0, TYPE_INT,
	ILayerControlFP::propLayers, FP_NO_FUNCTION, _T("LayerControllers"), 0, TYPE_CONTROL_TAB_BV,

	p_end
);

FPInterfaceDesc* CATClipValue::GetDescByID(Interface_ID id) {
	if (id == I_LAYERCONTROL_FP) return &layer_control_FPinterface;
	return &nullInterface;
}

// Gets the subanim parameter dimension based on the
// value flags.
ParamDimension* CATClipValue::GetParamDimension(int)
{
	if (TestFlag(CLIP_FLAG_ANGLE_DIM)) return stdAngleDim;
	return defaultDim;
}

// Grab the weight from daddy's weight controller and return it.
// The weight controller caches the value for each index at an
// instant in time, so successive calls with the same 't' should
// be quite speedy.
//
float CATClipValue::GetWeight(TimeValue t, int index, Interval& valid)
{
	if (index < 0 || index >= GetNumLayers())
		return 0.0f;

	if (weights)
		return weights->GetWeight(t, index, valid);
	else {
		return m_pClipRoot->GetWeight(t, index, valid);
	}
}

// Resizes our layers list.  Any new items get initialised with
// NewWhateverIAmController(), which returns a new instance of
// whatever sort of controller this value branch is supposed to
// hold.  If 'loading' is TRUE, the items are just initialised
// to NULL.  It's the loader's responsibility to set the
// references.
void CATClipValue::ResizeList(int n, BOOL loading /*=FALSE*/)
{
	int iOldNum = tabLayers.Count();

	// remove any layers that should be there
	for (int i = iOldNum - 1; i >= n; i--)
	{
		DeleteReference(i);
		DbgAssert(tabLayers[i] == NULL);
	}

	tabLayers.SetCount(n);

	// init to NULL first
	for (int i = iOldNum; i < n; i++)
		tabLayers[i] = NULL;

	// Now create the reference
	for (int i = iOldNum; i < n; i++)
	{
		if (!loading)
			ReplaceReference(i, NewWhateverIAmController());
	}
}

int CATClipValue::GetNumLayers() const
{
	int nLayers = tabLayers.Count();

	// In normal operation - our local list and the central
	// list _have_ to match up.  The only exception is if
	// we are changing the number of layers, or if we are being deleted.
#ifdef _DEBUG
	CATClipValue* pThisDeConst = const_cast<CATClipValue*>(this);
	if (m_pClipRoot != NULL && nLayers != m_pClipRoot->NumLayers())
	{
		DbgAssert(
			m_pClipRoot->bIsModifyingLayers ||
			theHold.RestoreOrRedoing() ||
			pThisDeConst->TestAFlag(A_BEING_AUTO_DELETED) ||
			(
				FindReferencingClass<CATNodeControl>(pThisDeConst, 1) == NULL ||
				FindReferencingClass<CATNodeControl>(pThisDeConst, 1)->TestAFlag(A_LOCK_TARGET) // This flag is set on our owner when being deleted.
				)
		);
	}
#endif
	return nLayers;
}

class LayerChangeRestore : public RestoreObj {
public:
	CATClipValue	*layers;
	int				index;
	Control			*ctrl;
	int				msg;

	LayerChangeRestore(CATClipValue *l, int m, int i) : ctrl(NULL) {
		layers = l;
		index = i;
		msg = m;
	}

	void Restore(int isUndo) {
		UNREFERENCED_PARAMETER(isUndo);
		//	if (isUndo) {
		switch (msg) {
		case CLIP_LAYER_INSERT:
			layers->RemoveController(index);
			break;
		case CLIP_LAYER_REMOVE:
			layers->InsertController(index, FALSE);
			break;
		case CLIP_LAYER_MOVEUP:
			//	index--;
			layers->MoveControllerUp(index);
			break;
		case CLIP_LAYER_MOVEDOWN:
			//	index++;
			layers->MoveControllerDown(index);
			break;
		}
		//	}
	}

	void Redo() {
		switch (msg) {
		case CLIP_LAYER_INSERT:
			layers->InsertController(index, FALSE);
			break;
		case CLIP_LAYER_REMOVE:
			layers->RemoveController(index);
			break;
		case CLIP_LAYER_MOVEUP:
			//	index++;
			layers->MoveControllerUp(index);
			break;
		case CLIP_LAYER_MOVEDOWN:
			//	index--;
			layers->MoveControllerDown(index);
			break;
		}
	}

	int Size() { return 4; }
	void EndHold() { layers->ClearAFlag(A_HELD); }
};

// Here, data is the id of the slot we're inserting
// into.  We first resize the list, which puts a new
// entry at the end.  Then we save a pointer to the
// new entry and set A_LOCK_TARGET to stop it from
// being auto-deleted.  This allows us to shuffle
// everything along and then stick it in the new gap
// we've created.  Yaay.
void CATClipValue::InsertController(int n, BOOL makenew)
{
	if (theHold.Holding())
		theHold.Put(new LayerChangeRestore(this, CLIP_LAYER_INSERT, n));

	int nCurrLayers = tabLayers.Count();
	int nNewLayers = max(n + 1, nCurrLayers + 1);
	ResizeList(nNewLayers, TRUE);

	// Move our reference pointers up 1 index (from n upwards)
	for (int i = nNewLayers - 2; i >= n; i--)
		tabLayers[i + 1] = tabLayers[i];

	// DONT FORGET TO NULL THIS POINTER!!!  It has been
	// copied to a higher index, so the reference will be preserved.
	tabLayers[n] = NULL;

	if (makenew) {
		Control *ctrl = NewWhateverIAmController();
		ReplaceReference(n, ctrl);
	}
}

// To remove, we replace each reference in the list
// with its successor.  At the end of the list, we
// replace the reference with NULL.  Now we have
// one reference for each item, except for the one
// we deleted, which should be gone =)
void CATClipValue::RemoveController(int n)
{
	int nCurrLayers = tabLayers.Count();
	if (n >= nCurrLayers)
		return;

	DeleteReference(n);

	for (int i = n + 1; i < nCurrLayers; i++)
		tabLayers[i - 1] = tabLayers[i];

	tabLayers[nCurrLayers - 1] = NULL;

	if (theHold.Holding())
		theHold.Put(new LayerChangeRestore(this, CLIP_LAYER_REMOVE, n));

	ResizeList(nCurrLayers - 1);

	NotifyDependents(FOREVER, PART_TM, REFMSG_CHANGE);
}

void CATClipValue::MoveControllerUp(int n)
{
	if (n > 0 && n < GetNumLayers()) {
		Control *ctrlTarget = tabLayers[n - 1];
		tabLayers[n - 1] = tabLayers[n];
		tabLayers[n] = ctrlTarget;

		if (theHold.Holding())
			theHold.Put(new LayerChangeRestore(this, CLIP_LAYER_MOVEUP, n));
	}

	NotifyDependents(FOREVER, PART_TM, REFMSG_CHANGE);
}

void CATClipValue::MoveControllerDown(int n)
{
	MoveControllerUp(n + 1);
}

void CATClipValue::CATMessage(TimeValue t, UINT msg, int data)
{
	TCHAR buf[256] = { 0 };
	switch (msg) {
	case CLIP_LAYER_INSERT:
		InsertController(data, TRUE);
		break;
	case CLIP_LAYER_CALL_PLACB:
		LoadPostLayerCallback();
		break;
	case CLIP_LAYER_REMOVE:
		// If we are being displayed in the motion panel kill our
		// rollout. I pass in BEGIN_EDIT_CREATE because its the only way
		// I know to force the removal of a rollout without changing panels
//			if (ipClip != NULL)
//				EndEditParams(ipClip, flagsBegin);
		RemoveController(data);
		break;
	case CLIP_LAYER_MOVEUP:
		MoveControllerUp(data);
		break;
	case CLIP_LAYER_MOVEDOWN:
		MoveControllerDown(data);
		break;
	case CLIP_LAYER_SELECT:
		// This is easier than appending a layer.  We just
		// plug 'data' straight into our selected channel.
#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
		if (ipClip != NULL && GetCATMode() != SETUPMODE && m_pClipRoot->TestFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS)) {
#else
		if (ipClip != NULL && GetCATMode() != SETUPMODE) {
#endif
			// If we're displaying a rollout for the selected layer,
			// switch it to display the rollout for the new selected
			// layer (index passed in the data parameter).
			int oldSelected = GetSelectedIndex();
			EndEditLayers(oldSelected, ipClip, END_EDIT_REMOVEUI, GetCATMode());
			BeginEditLayers(data, ipClip, flagsBegin, GetCATMode());
		}
		// Make the timeline redraw the keys
		NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
		break;
	case CLIP_LAYER_CHECK_NUM_LAYERS:
		if (m_pClipRoot && GetNumLayers() != tabLayers.Count()) {

			TSTR message = GetString(IDS_ERR_CORRUPTION);
			TSTR summary = GetString(IDS_ERR_CORRUPT1);
			_tcscpy(buf, GetString(IDS_ERR_COR2));
			_tcscat(buf, GetString(IDS_ERR_COR3));
			TSTR errordetails = buf;

			::MessageBox(GetCOREInterface()->GetMAXHWnd(),
				message + errordetails + summary,
				GetString(IDS_ERR_FILELOAD), MB_OK);

			ResizeList(m_pClipRoot->NumLayers());
		}
		break;

	case CAT_CATMODE_CHANGED:
#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
		if (ipClip != NULL && m_pClipRoot->TestFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS)) {
#else
		if (ipClip != NULL) {
#endif
			if (data == SETUPMODE || GetCATMode() == SETUPMODE) {
				// remove the existing catmode rollouts
				EndEditLayers(GetSelectedIndex(), ipClip, flagsBegin | END_EDIT_REMOVEUI, data);
				// Add the rollouts for the new mode
				BeginEditLayers(GetSelectedIndex(), ipClip, flagsBegin, GetCATMode());
			}
		}
		// continue on and update the controller if we are an extra controller
	case CAT_UPDATE:
		if (TestFlag(CLIP_FLAG_ARB_CONTROLLER)) {
			// Extra controllers need to send out special messages
			// So that the objects they are attached to update.
			ClearFlag(CLIP_FLAG_ARB_CONTROLLER);
			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			SetFlag(CLIP_FLAG_ARB_CONTROLLER);
		}
		break;
	case CAT_KEYFREEFORM:
		// These controllers are attached to objects in the scene,not CATObjects.
		// That means taht they have to keyframe thier own new layers
		if (TestFlag(CLIP_FLAG_ARB_CONTROLLER)) {
			Interval iv = FOREVER;
			switch (SuperClassID())
			{
			case CTRL_FLOAT_CLASS_ID:
			{
				float val;
				GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
				CATControl::KeyFreeformMode SetMode(this);
				SetValue(t, (void*)&val, TRUE, CTRL_ABSOLUTE);
				break;
			}
			default:
				break;
			}
		}
		break;
	case CLIP_WEIGHTS_CHANGED:
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		break;
	case CAT_TDM_CHANGED:
		NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
		break;
		}
		}

void CATClipValue::SavePostLayerCallback()
{
	Control* pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl == NULL)
		return;

	TSTR rigdatafile = GetPlugCFGPath(_T("newlayercallback.txt"));
	CATRigWriter save(_T(""), GetCATParentTrans());
	if (!save.ok()) {
		return;
	}
	DWORD flags = CLIPFLAG_SKIP_KEYFRAMES | CLIPFLAG_CLIP;

	DWORD dwCurrVer = CAT_VERSION_CURRENT;
	save.Write(idCATVersion, dwCurrVer);

	float dCATUnits = GetCATUnits();
	save.Write(idCatUnits, dCATUnits);

	NLAInfo* pInfo = GetSelectedNLAInfo();
	DbgAssert(pInfo != NULL);
	if (pInfo != NULL)
	{
		TSTR method = pInfo->GetLayerType();
		save.Write(idLayerMethod, method);
	}
	save.BeginGroup(idController);

	save.WriteController(pSelectedCtrl, flags, FOREVER);
	save.EndGroup();

	save.GetStreamBuffer(newlayercallback);
}

void CATClipValue::LoadPostLayerCallback()
{
	int nSelected = GetSelectedIndex();
	if (nSelected < 0 || (newlayercallback.Length() < 2)) return;
	// Create our rig loader and begin...
	CATRigReader load(newlayercallback, GetCATParentTrans());
	bool done = false;
	bool ok = true;
	float dScale = 1.0f;
	DWORD flags = CLIPFLAG_SKIP_KEYFRAMES | CLIPFLAG_CLIP | CLIPFLAG_SCALE_DATA;

	while (load.ok() && !done && ok) {
		load.NextClause();
		switch (load.CurClauseID()) {
		case rigBeginGroup:
			switch (load.CurIdentifier()) {
			case idController:
				LoadClip(&load, FOREVER, nSelected, dScale, flags);
				break;
			}
			done = TRUE;
			break;
		case rigAssignment:
			switch (load.CurIdentifier())
			{
			case idLayerMethod: {
				TSTR method;
				load.GetValue(method);

				NLAInfo* pInfo = GetSelectedNLAInfo();
				DbgAssert(pInfo != NULL);
				if (pInfo != NULL && pInfo->GetLayerType() != method)
				{
					done = true;
					ok = false;
				}
				break;
			}
			case idCatUnits: {
				float dCATUnits;
				load.GetValue(dCATUnits);
				dScale = GetCATUnits() / dCATUnits;
				break;
			}
			}
			break;
		case rigAbort:
		case rigEnd:
			done = true;
			break;
		case rigEndGroup:
			ok = FALSE;
			break;
		}
	}
}

int CATClipValue::FindLayerIndexByAddr(Control* ptr) {

	for (int i = 0; i < tabLayers.Count(); i++)
		if (tabLayers[i] == ptr)
			return i;
	return -1;
}

// Construction...  I thought we could call the polymorphic
// Init() here, but we can't because it hasn't been created
// yet.  So any derived class that overrides Init() must call
// it from their own constructor.
//
CATClipValue::CATClipValue(BOOL loading/*=FALSE*/)
	: dwFileSaveVersion(0)
	, m_pClipRoot(NULL)
	, m_pCATParentTrans(NULL)
	, flags(CLIP_FLAG_INHERIT_POS | CLIP_FLAG_INHERIT_ROT | CLIP_FLAG_INHERIT_SCL)
	, layerbranch(NULL)
	, weights(NULL)
	, ctrlSetup(NULL)
	, ipClip(NULL)
	// this is the ID for the transform controllers on the CATNodeControl nodes
	// some controllers will have these changed
	, rigID(idController)
{
	UNUSED_PARAM(loading);
}

// Just delete our references when destructing.
//
CATClipValue::~CATClipValue()
{
	if (TestFlag(CLIP_FLAG_ARB_CONTROLLER))
	{
		if (m_pCATParentTrans != NULL)
		{
			// We cannot pass ourselves in this argument,
			// as
			AnimHandle myHdl = GetHandleByAnim(this);
			// TODO: Fix this.  Its unlikely to be a problem (thats a lot of Anims)
			// but still needs to be done sooner rather than later.
			DbgAssert(!_T("Warning: x64 failure here"));
			m_pCATParentTrans->CATMessage(0, CLIP_ARB_LAYER_DELETED, myHdl);
		}
	}

	DeleteAllRefs();
}

// Copy / Clone -- No thanks...
void CATClipValue::Copy(Control *from) {

	if (SuperClassID() == CTRL_FLOAT_CLASS_ID && from->SuperClassID() == CTRL_FLOAT_CLASS_ID) {
		float val = 0.0;
		Interval iv = FOREVER;
		from->GetValue(GetCOREInterface()->GetTime(), (void*)&val, iv, CTRL_ABSOLUTE);
		GetSetupVal((void*)&val);
	}
}

RefTargetHandle CATClipValue::Clone(RemapDir& remap)
{
	if (TestFlag(CLIP_FLAG_DISABLE_LAYERS)) {
		return NULL;
	}
	CATClipValue *ctrl = (CATClipValue*)CreateInstance(SuperClassID(), ClassID());
	DbgAssert(ctrl);

	if (m_pCATParentTrans)	remap.PatchPointer((RefTargetHandle*)&ctrl->m_pCATParentTrans, m_pCATParentTrans);
	if (weights)			remap.PatchPointer((RefTargetHandle*)&ctrl->weights, weights);
	if (m_pClipRoot)			remap.PatchPointer((RefTargetHandle*)&ctrl->m_pClipRoot, m_pClipRoot);

	if (SuperClassID() == CTRL_MATRIX3_CLASS_ID) {
		Matrix3 val;
		GetSetupVal((void*)&val);
		ctrl->SetSetupVal((void*)&val);
	}
	else if (SuperClassID() == CTRL_FLOAT_CLASS_ID) {
		float val;
		GetSetupVal((void*)&val);
		ctrl->SetSetupVal((void*)&val);
	}

	int numlayers = m_pClipRoot->NumLayers();
	ctrl->ResizeList(numlayers);
	for (int i = 0; i < numlayers; i++) {
		ctrl->ReplaceReference(i, remap.CloneRef(GetReference(i)));
	}

	ctrl->flags = flags;
	ctrl->rigID = rigID;

	BaseClone(this, ctrl, remap);

	return ctrl;
}

//////////////////////////////////////////////////////////////////////////
// Load/Save stuff

// Backwards compatibility
//

class CATClipValuePLCB : public PostLoadCallback {
protected:
	CATClipValue *ctrl;

	// Allow patching of old files (2.5) to newer
	ReferenceTarget* mpCATParentPtr;

public:

	CATClipValuePLCB(CATClipValue *pOwner) { ctrl = pOwner; mpCATParentPtr = NULL; }

	IOResult PatchCATParentPtr(ILoad* pLoad)
	{
		DWORD refID = -1;
		DWORD nb;
		IOResult res = pLoad->Read(&refID, sizeof(DWORD), &nb);
		if (res == IO_OK && refID != (DWORD)-1)
			pLoad->RecordBackpatch(refID, (void**)&(mpCATParentPtr));
		return res;
	}

	DWORD GetFileSaveVersion() {
		if (ctrl->dwFileSaveVersion > CAT_VERSION_1151) return ctrl->dwFileSaveVersion;
		if (ctrl->GetRoot()) return ctrl->GetRoot()->GetFileSaveVersion();
		return CAT_VERSION_1200;
	}

	// This PLCB must be called after the reference
	// hierarchy has been loaded back in.
	int Priority() { return 7; }

	void proc(ILoad *iload)
	{
		if (!ctrl || ctrl->TestAFlag(A_IS_DELETED) || !DependentIterator(ctrl).Next())
		{
			delete this;
			return;
		}

		// We are having some issues in getting our CATParent pointer to cast correctly.
		// I suspect this is a compiler bug, or something, but if we try and directly load
		// the pointer as a CATParent, the memory is offset slightly.  If instead we load
		// to a ReferenceTarget (which is the pointer that the loading code actually has)
		// then cast to a CATParent, we get the correct value.
		ECATParent* pCATParent = dynamic_cast<ECATParent*>(mpCATParentPtr);
		DbgAssert(pCATParent != NULL || mpCATParentPtr == NULL);

		if (GetFileSaveVersion() < CAT_VERSION_1700) {
			::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_NOLOAD_C1C3), GetString(IDS_ERR_CAT1LOAD), MB_OK);
			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_1700) {
			ctrl->ClearFlag(CLIP_FLAG_DISABLE_LAYERS);
		}

		if (GetFileSaveVersion() < CAT_VERSION_2420) {
			// any ArbBones that have been added to the BoneData controller
			//  need to be moved off onto the  1st bone segment
			if (ctrl->SuperClassID() != CTRL_FLOAT_CLASS_ID) {
				ctrl->ClearFlag(CLIP_FLAG_ARB_CONTROLLER);
			}
		}

		if (GetFileSaveVersion() < CAT_VERSION_2435)
		{
			if (pCATParent != NULL)
			{
				ICATParentTrans* pCATParentTrans = pCATParent->GetCATParentTrans();
				ctrl->SetCATParentTrans(pCATParentTrans);
			}
		}

		// in some cases, the setup controller was lost, somehow.
		// So we need to clear this flag so that
		// our scripts can keep working with the layer system.
		if (!ctrl->ctrlSetup && ctrl->TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
			ctrl->ClearFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER);
		}

		// Check that our layer stack is in tact.
		ctrl->CATMessage(0, CLIP_LAYER_CHECK_NUM_LAYERS, -1);
		ctrl->ClearFlag(CLIP_FLAG_DISABLE_LAYERS);

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

// ST -28-01-04 Please note that CATClipTransforms
// overwrites this class and so any changes to the next
// two functions should be mirrored there.
#define CLIP_NUMLAYERS_CHUNK		1
#define CLIP_SELECTED_CHUNK			2
#define CLIP_FLAGS_CHUNK			3
#define CLIP_DADDY_CHUNK			4
#define CLIP_VERSION_CHUNK			5
#define CLIP_CATPARENT_CHUNK		6
#define CLIP_WEIGHTS_CHUNK			7
#define CLIP_RIG_ID_CHUNK			9
#define CLIP_SETUPVAL_CHUNK			8
#define CLIP_ROOT_CHUNK				10
//#define CLIP_CATCONTROL_CHUNK		11
#define CLIP_CATPARENTTRANS_CHUNK	14
#define CLIP_NEWLAYERCALLBACK_CHUNK	16
#define CLIP_TDM_CHUNK				18

IOResult CATClipValue::Save(ISave *isave)
{
	DWORD nb, refID;

	// The file save version
	dwFileSaveVersion = CAT_VERSION_CURRENT;
	isave->BeginChunk(CLIP_VERSION_CHUNK);
	isave->Write(&dwFileSaveVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	// We are saving a CAT3 rig file (RG3)
	if (g_bSavingCAT3Rig) {
		// Stores the number of layers.
		isave->BeginChunk(CLIP_NUMLAYERS_CHUNK);
		int numlayers = 0;
		isave->Write(&numlayers, sizeof(int), &nb);
		isave->EndChunk();

		// Stores the current layer selection.
		isave->BeginChunk(CLIP_SELECTED_CHUNK);
		int sel = -1;
		isave->Write(&sel, sizeof(int), &nb);
		isave->EndChunk();

	}
	else
	{
		isave->BeginChunk(CLIP_NUMLAYERS_CHUNK);
		int nLayers = GetNumLayers();
		isave->Write(&nLayers, sizeof(int), &nb);
		isave->EndChunk();

		if (isave->SavingVersion() <= MAX_RELEASE_R13)
		{
			// For old files only, preserve the currently
			// selected layuer
			int nSelected = GetSelectedIndex();
			isave->BeginChunk(CLIP_SELECTED_CHUNK);
			isave->Write(&nSelected, sizeof(int), &nb);
			isave->EndChunk();
		}
	}

	// Stores the flags.
	isave->BeginChunk(CLIP_FLAGS_CHUNK);
	isave->Write(&flags, sizeof(ULONG), &nb);
	isave->EndChunk();

	// Stores handle to daddy.
	refID = isave->GetRefID((void*)m_pClipRoot);
	isave->BeginChunk(CLIP_ROOT_CHUNK);
	isave->Write(&refID, sizeof(DWORD), &nb);
	isave->EndChunk();

	// Stores handle to catparenttrans.
	refID = isave->GetRefID((void*)m_pCATParentTrans);
	isave->BeginChunk(CLIP_CATPARENTTRANS_CHUNK);
	isave->Write(&refID, sizeof(DWORD), &nb);
	isave->EndChunk();

	//////////////////////////////////////////////////////////////////////////
	// Stores handle to our weights controller.
	refID = isave->GetRefID((void*)weights);
	isave->BeginChunk(CLIP_WEIGHTS_CHUNK);
	isave->Write(&refID, sizeof(DWORD), &nb);
	isave->EndChunk();

	// Stores our rig ID as a string
//	refID = isave->GetRefID((void*)weights);
//	isave->BeginChunk(CLIP_RIG_ID_CHUNK);
//	const char* idname = IdentName(rigID);
//	isave->WriteCString(idname);
//	isave->EndChunk();

	isave->BeginChunk(CLIP_SETUPVAL_CHUNK);
	SaveSetupval(isave);
	isave->EndChunk();

	//refID = isave->GetRefID((void*)catcontrol);
	//isave->BeginChunk(CLIP_CATCONTROL_CHUNK);
	//isave->Write( &refID, sizeof DWORD, &nb);
	//isave->EndChunk();

	isave->BeginChunk(CLIP_NEWLAYERCALLBACK_CHUNK);
	isave->WriteCString(newlayercallback);
	isave->EndChunk();

	int tdm = GetTrackDisplayMethod();
	isave->BeginChunk(CLIP_TDM_CHUNK);
	isave->Write(&tdm, sizeof(int), &nb);
	isave->EndChunk();

	return IO_OK;
}

// ST -28-01-04 Please note that CATClipTransforms
// overwrites this class and so any changes to the next
// function should be mirrored there.
IOResult CATClipValue::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L, refID = 0L;

	// Default file save version to the version just prior to
	// when it was implemented.
	dwFileSaveVersion = CAT_VERSION_1151;
	CATClipValuePLCB* pValuePLCB = new CATClipValuePLCB(this);

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CLIP_VERSION_CHUNK:
			res = iload->Read(&dwFileSaveVersion, sizeof DWORD, &nb);
			break;

		case CLIP_NUMLAYERS_CHUNK: {
			// We read into a temporary variable rather than into
			// nLayers because correct initialisation within
			// ResizeList() requires nLayers to initially be
			// be zero.  This little cunt had me for a while...
			int size = 0;
			res = iload->Read(&size, sizeof(int), &nb);
			ResizeList(size, TRUE);
			break;
		}
		case CLIP_FLAGS_CHUNK:
			if (dwFileSaveVersion < CAT_VERSION_1700)
				res = iload->Read(&flags, sizeof USHORT, &nb);
			else res = iload->Read(&flags, sizeof ULONG, &nb);
			break;

			//////////////////////////////////////////////////////////////////////////
		case CLIP_CATPARENT_CHUNK:
			res = pValuePLCB->PatchCATParentPtr(iload);
			break;
		case CLIP_CATPARENTTRANS_CHUNK:
			res = iload->Read(&refID, sizeof DWORD, &nb);
			if (res == IO_OK && refID != (DWORD)-1)
				iload->RecordBackpatch(refID, (void**)&m_pCATParentTrans);
			break;

		case CLIP_ROOT_CHUNK:
			res = iload->Read(&refID, sizeof DWORD, &nb);
			if (res == IO_OK && refID != (DWORD)-1)
				iload->RecordBackpatch(refID, (void**)&m_pClipRoot);
			break;

		case CLIP_WEIGHTS_CHUNK:
			res = iload->Read(&refID, sizeof DWORD, &nb);
			if (res == IO_OK && refID != (DWORD)-1)
				iload->RecordBackpatch(refID, (void**)&weights);
			break;

			//	case CLIP_RIG_ID_CHUNK:
			//		char* name;
			//		res = iload->ReadCStringChunk(&name);
			//		if(res == IO_OK) rigID = StringIdent(name);
			//		break;

		case CLIP_SETUPVAL_CHUNK:
			LoadSetupval(iload);
			break;
			//case CLIP_CATCONTROL_CHUNK:
			//	res = iload->Read(&refID, sizeof DWORD, &nb);
			//	if (res == IO_OK && refID != (DWORD)-1)
			//		iload->RecordBackpatch(refID, (void**)&catcontrol);
			//	break;
		case CLIP_NEWLAYERCALLBACK_CHUNK: {
			TCHAR *strBuf;
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) newlayercallback = strBuf;
			break;
		}
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(pValuePLCB);

	return IO_OK;
}

int CATClipValue::NumSubs() {
	if (g_bSavingCAT3Rig) {
		return TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER) ? 1 : 0;
	}
	return GetNumLayers() + (TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER) ? 1 : 0);
}
int CATClipValue::NumRefs() {
	if (g_bSavingCAT3Rig) {
		return 1;
	}
	return GetNumLayers() + 1;
}

// References and subanims.  Easy-peesy....  Our refs are just
// our layers.  Same with sub-anims.
//
RefTargetHandle CATClipValue::GetReference(int i)
{
	if (g_bSavingCAT3Rig) {
		if (i == 0)	return ctrlSetup;
		return NULL;
	}
	int nLayers = GetNumLayers();
	if (i < nLayers)	return tabLayers[i];
	if (i == nLayers)	return ctrlSetup;
	return NULL;
}

void CATClipValue::SetReference(int i, RefTargetHandle rtarg)
{
	int nLayers = GetNumLayers();
	if (i < nLayers)	tabLayers[i] = (Control*)rtarg;
	if (i == nLayers)	ctrlSetup = (Control*)rtarg;
}

int CATClipValue::SubNumToRefNum(int subNum) {
	if (TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
		if (subNum == 0) return GetNumLayers();
		subNum--;
	}
	return subNum;
}

Animatable* CATClipValue::SubAnim(int i)
{
	int nSelected = GetSelectedIndex();

	if (TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
		if (i == 0) {
			// In Animation mode, we no longer return the setup controller as a subanim
			// because some keyframing methods in max keyframe all exposed controllers. (e.g. Big key button)
			if (GetCATMode() == SETUPMODE) {
				return ctrlSetup;
			}
			else {
				return ControlOrDummy(ctrlSetup);
			}
		}
		i--;
	}

	CATClipRoot* pRoot = GetRoot();
	if (pRoot == NULL || pRoot->TestAFlag(A_IS_DELETED)) return NULL;
	int tdm = pRoot->GetTrackDisplayMethod();
	switch (tdm) {
	case TRACK_DISPLAY_METHOD_ACTIVE:
		if (i == nSelected)	return tabLayers[i];
		break;
	case TRACK_DISPLAY_METHOD_CONTRIBUTING:
		if (pRoot->LayerHasContribution(i, GetCOREInterface()->GetTime(), LAYER_RANGE_ALL, weights))
			return tabLayers[i];
		break;
	case TRACK_DISPLAY_METHOD_ALL:
		if (i < GetNumLayers()) return tabLayers[i];
		else				return NULL;
		break;
	}
	if (i < GetNumLayers())
		return ControlOrDummy(tabLayers[i]);
	return NULL;
}

TSTR CATClipValue::SubAnimName(int i)
{
	if (TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
		if (i == 0) return GetString(IDS_SETUP);
		i--;
	}
	CATClipRoot* pRoot = GetRoot();
	if (pRoot != NULL)
	{
		NLAInfo* pInfo = pRoot->GetLayer(i);
		if (pInfo != NULL)
			return pInfo->GetName();
	}
	return _T(""); // Perhaps we should return an error message
}

// TODO: Probably something here...
RefResult CATClipValue::NotifyRefChanged(const Interval&, RefTargetHandle hTarget, PartID&, RefMessage message, BOOL)
{
	switch (message) {
	case REFMSG_TARGET_DELETED:
		for (int i = 0; i < tabLayers.Count(); i++)
			if (tabLayers[i] == hTarget)	tabLayers[i] = NULL;
		break;
	case REFMSG_KEY_SELECTION_CHANGED:
	case REFMSG_LOOPTEST:
	case REFMSG_CHANGE:
	case REFMSG_NODE_LINK:
	case REFMSG_SUBANIM_STRUCTURE_CHANGED:
	case REFMSG_CONTROLREF_CHANGE:
	case REFMSG_REF_DELETED:
	case REFMSG_REF_ADDED:
	case REFMSG_GET_NODE_HANDLE:
	case REFMSG_NODE_HANDLE_CHANGED:
	case REFMSG_NUM_SUBOBJECTTYPES_CHANGED:
	case REFMSG_TARGET_SELECTIONCHANGE:
	case REFMSG_LOOKAT_TARGET_DELETED:
	case REFMSG_GET_NODE_NAME:
		return REF_SUCCEED;
	default:
		return REF_STOP;
	}
	return REF_SUCCEED;
}

Control* CATClipValue::GetLayer(int n) const
{
	if (n >= 0 && n < tabLayers.Count())
		return tabLayers[n];;
	return NULL;
}
//
// Replaces the controller for the specified layer with a new one.
//
void CATClipValue::SetLayer(int n, Control *ctrl)
{
	DbgAssert(n >= 0 && n < GetNumLayers());
	DbgAssert(ctrl != NULL && ctrl->SuperClassID() == SuperClassID());
	if (n >= 0 && n < GetNumLayers())
		ReplaceReference(n, (RefTargetHandle)ctrl);
	//	AssignController(ctrl, n);
}

//
// This gives a value branch some control over how each layer will
// be evaluated, by letting it catch the layer method from the root
// and adjust it if necessary.
//
ClipLayerMethod CATClipValue::GetLayerMethod(int layer)
{
	// The CATClipRoot implimentation of this method handles that case where
	// the ClipRoot has not completed its upgrade
	CATClipRoot* pRoot = GetRoot();
	if (pRoot != NULL)
		return pRoot->GetLayerMethod(layer);
	return LAYER_IGNORE;
}

Control* CATClipValue::GetSelectedLayer() const
{
	int nSelected = GetSelectedIndex();
	return (nSelected >= 0 && nSelected < tabLayers.Count()) ? tabLayers[nSelected] : NULL;
}

ClipLayerMethod CATClipValue::GetSelectedLayerMethod()
{
	NLAInfo* pInfo = GetSelectedNLAInfo();
	if (pInfo != NULL)
		return pInfo->GetMethod();
	return LAYER_IGNORE;
}

NLAInfo* CATClipValue::GetSelectedNLAInfo()
{
	CATClipRoot* pRoot = GetRoot();
	if (pRoot != NULL)
		return pRoot->GetSelectedLayerInfo();
	return NULL;
}

// ST - This cumbersomely labeled function returns
// the first or last (depending on how you look at it)
// absolute layer with 100% weighting
int CATClipValue::GetFirstActiveAbsLayer(TimeValue t, LayerRange range)
{
	DbgAssert(m_pClipRoot);
	int i;
	// Go down the stack and stop
	for (i = range.nLast; i > range.nFirst; i--)
	{
		// If the layer stack ever gets out of synch,
		// then this mothod is the 1st method to catch the error
		if (i < 0 || i >= m_pClipRoot->NumLayers()) {
			DbgAssert(0);
		}

		NLAInfo *layerinfo = m_pClipRoot->GetLayer(i);
		float dClipWeight = 0.0f;
		if (!layerinfo || !tabLayers[i]) continue;
		if (layerinfo->ApplyAbsolute() &&
			layerinfo->LayerEnabled())
		{
			// We no longer assume that a layer controller has a local weights controller since the introduction of
			// the new root node controller for games. This controller is not associated with any body part
			// and so has no local weight controller
			Interval iv = FOREVER;
			if (GetWeightsCtrl())
				dClipWeight = GetWeightsCtrl()->GetWeight(t, i, iv);
			else {
				dClipWeight = layerinfo->GetWeight(t, iv);
			}
			if (dClipWeight == 1.0f)	break;
		}
	}

	return i;
}

// Assignment of individual layer controllers (GB 21-Jul-03).
//
// Steve has pointed out that the user needs to assign controllers to
// layers because users might want things such as look-ats, scripts
// etc.
//
// AssignController() allows them to assign inside the clip hierarchy.
//
// GetSelectedLayerOrDummy() and AssignToSelectedLayer() provide a
// way to get and set the selected layer controller.  If there is no
// layer selected, GetSelectedLayerOrDummy() returns a dummy, and
// AssignToSelectedLayer() does nothing and returns FALSE.
//
Control* CATClipValue::GetSelectedLayerOrDummy()
{
	Control* pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl;

	return this->ControlOrDummy(this);
}

int CATClipValue::GetSelectedIndex() const
{
	const CATClipRoot* pRoot = GetRoot();
	if (pRoot != NULL)
		return pRoot->GetSelectedLayer();
	return -1;
}

// Note:nobody a
BOOL CATClipValue::AssignToSelectedLayer(Control *ctrl)
{
	int nSelected = GetSelectedIndex();
	if (nSelected >= 0) {
		ReplaceReference(SubNumToRefNum(nSelected), (RefTargetHandle)ctrl);
		return TRUE;
	}
	return FALSE;
}

BOOL CATClipValue::AssignController(Animatable *control, int subAnim)
{
	//	if(nSelected == subAnim)
	//	{
	ReplaceReference(SubNumToRefNum(subAnim), (RefTargetHandle)control, TRUE);
	return TRUE;
	//	}
	//	return FALSE;
}

BOOL CATClipValue::IsAnimated()
{
	return TRUE;
	// The following code has been commented because it caused problems with the
	// Gambryo exporter. 99.9% of the time this should return true anyway, and even then
	// it only matter on the root node. i.e. the pelvis. Also, it doesn't take into account
	// local weights, or animated transform nodes.
/*	BOOL ani = FALSE;
	for(int i = 0; i < nLayers; i++){
		if(root->LayerHasContribution(i, GetCOREInterface()->GetTime(), LayerRange(i,nLayers), weights))
			ani |= tabLayers[i]->IsAnimated();
	}
	return ani;
*/
}

int CATClipValue::IsKeyable() {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->IsKeyable();
	return FALSE;
}

void CATClipValue::CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) {

	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl == NULL)
		return;

	NLAInfo* pInfo = GetSelectedNLAInfo();
	if (pInfo != NULL)
	{
		// make sure the time values are transformed into layer time
		src = pInfo->GetTime(src);
		dst = pInfo->GetTime(dst);
	}
	pSelectedCtrl->CopyKeysFromTime(src, dst, flags);
}

void CATClipValue::AddNewKey(TimeValue t, DWORD flags) {
	if (GetCATMode() != SETUPMODE)
	{
		Control *pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			return pSelectedCtrl->AddNewKey(t, flags);
	}
}

BOOL CATClipValue::IsKeyAtTime(TimeValue t, DWORD flags)
{
	if (GetCATMode() != SETUPMODE)
	{
		// PT 13-12-05 This method is mostly used to tell spinners whether to draw keyframe brackets.
		// Therefore we should only check the active layer.
		Control *pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			return pSelectedCtrl->IsKeyAtTime(t, flags);
	}
	return FALSE;
}

BOOL CATClipValue::GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) {

	/*	DbgAssert(root);

		int tdm = root->GetTrackDisplayMethod();
		if(tdm==TRACK_DISPLAY_METHOD_ACTIVE){
				if (nSelected < 0) return 0;
				return GetLayer(nSelected)->GetNextKeyTime(root->GetLayerTime(nSelected, t), flags, nt);
		}

		if( tdm == TRACK_DISPLAY_METHOD_CONTRIBUTING ||
			tdm == TRACK_DISPLAY_METHOD_ALL){
			TimeValue at,tnear = 0;
			BOOL tnearInit = FALSE;
			for(int i=0; i<NumLayers(); i++){
				if( tdm == TRACK_DISPLAY_METHOD_CONTRIBUTING &&
					root->LayerHasContribution(i, GetCOREInterface()->GetTime(), LAYER_RANGE_ALL, weights))
					continue;
				if (GetLayer(i) && GetLayer(i)->GetNextKeyTime(root->GetLayerTime(i, t), flags,at)) {
					if (!tnearInit) {
						tnear = at;
						tnearInit = TRUE;
					} else 	if (ABS(at-t) < ABS(tnear-t)) tnear = at;
				}
			}
			if (tnearInit) {
				nt = tnear;
				return TRUE;
			} else{
				return FALSE;
			}
		}
		return FALSE;
	*/
	// PT 13-12-05 This method is mostly used to update Schematic View.
	// Therefore we should only check the active layer.
	if (GetCATMode() != SETUPMODE)
	{
		Control *pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			return pSelectedCtrl->GetNextKeyTime(t, flags, nt);
	}
	return FALSE;
}

static void GetSubAnims(Animatable *anim, Tab<Animatable *>&anims)
{
	if (anim != NULL)
	{
		anims.Append(1, &anim);
		for (int i = 0; i < anim->NumSubs(); ++i)
		{
			Animatable *subAnim = anim->SubAnim(i);
			if (subAnim)
			{
				GetSubAnims(subAnim, anims);
			}
		}
	}
}

static int CompInts(const void *elem1, const void *elem2) {
	int a = *((int*)elem1);
	int b = *((int*)elem2);
	return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

static void GetControllerHierarchyKeyTimes(Animatable *anim, Tab<TimeValue> &keyTimes, Interval &range, bool getSubTimes = TRUE)
{
	Tab<Animatable*> anims;
	if (getSubTimes)
		GetSubAnims(anim, anims);
	else
		anims.Append(1, &anim);
	anims.Shrink();
	Tab<TimeValue> tempTimes;
	TimeValue t;
	for (int i = 0; i < anims.Count(); ++i)
	{
		if (anims[i] && anims[i]->NumKeys() > 0)
		{
			anims[i]->GetKeyTimes(tempTimes, range, 0);

			for (int j = 0; j < tempTimes.Count(); j++) {
				BOOL addkey = TRUE;
				for (int k = 0; addkey && k < keyTimes.Count(); k++) {
					if (tempTimes[j] == keyTimes[k]) addkey = FALSE;
				}
				t = tempTimes[j];
				if (addkey) keyTimes.Append(1, &t);
			}
		}
	}
	keyTimes.Sort(CompInts);
}

// this function isint as fast as it could be but its a trickey problem
// we unable to filter the keyframe time values using the time warp curve
int CATClipValue::GetKeyTimes(Tab<TimeValue>& times, Interval range, DWORD flags)
{
	int tdm = GetTrackDisplayMethod();

	if (GetCATMode() == SETUPMODE)
		return 0;

	if (tdm == TRACK_DISPLAY_METHOD_ACTIVE) {
		Control *pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			return pSelectedCtrl->GetKeyTimes(times, range, flags);
		return 0;
	}

	if (tdm == TRACK_DISPLAY_METHOD_CONTRIBUTING ||
		tdm == TRACK_DISPLAY_METHOD_ALL) {

		Tab<TimeValue> layertimes;
		for (int i = 0; i < GetNumLayers(); i++) {
			if (tdm == TRACK_DISPLAY_METHOD_CONTRIBUTING &&
				m_pClipRoot->LayerHasContribution(i, range.Start(), LAYER_RANGE_ALL, weights))
				continue;
			if (!m_pClipRoot->GetLayer(i)->LayerEnabled())
				continue;

			if (SuperClassID() == CTRL_MATRIX3_CLASS_ID)
			{
				if (flags&KEYAT_POSITION) {
					Control *pos = GetLayer(i)->GetPositionController();
					GetControllerHierarchyKeyTimes(pos, times, range, (flags != 0));
				}
				if (flags&KEYAT_ROTATION) {
					Control *rot = GetLayer(i)->GetRotationController();
					GetControllerHierarchyKeyTimes(rot, times, range, (flags != 0));
				}
				if (flags&KEYAT_SCALE) {
					Control *scl = GetLayer(i)->GetScaleController();
					GetControllerHierarchyKeyTimes(scl, times, range, (flags != 0));
				}
			}
			else {
				GetLayer(i)->GetKeyTimes(layertimes, range, flags);
			}

			int prevtimecount = times.Count();
			times.SetCount(prevtimecount + layertimes.Count());
			for (int j = 0; j < layertimes.Count(); j++)
				times[prevtimecount + j] = layertimes[j];
		}

		times.Sort(CompInts);
	}

	return 0;
}

int CATClipValue::GetKeySelState(BitArray &sel, Interval range, DWORD flags) {
	if (GetCATMode() != SETUPMODE)
	{
		Control *pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			return pSelectedCtrl->GetKeySelState(sel, range, flags);
	}
	return FALSE;
}

Interval GetLeafTimeRange(Control* ctrl, DWORD flags)
{
	Interval range, iv;
	BOOL init = FALSE;

	for (int i = 0; i < ctrl->NumSubs(); i++)
	{
		Control* sub = (Control*)ctrl->SubAnim(i)->GetInterface(I_CONTROL);
		if (sub && !sub->IsLeaf())
			iv = GetLeafTimeRange(sub, flags);
		else iv = sub->GetTimeRange(flags);
		if (!iv.Empty()) {
			if (init) {
				range += iv.Start();
				range += iv.End();
			}
			else {
				range.Set(iv.Start(), iv.End());
				init = TRUE;
			}
		}
	}
	return range;
}
// We override these methods to do special things with
// key ranges.  This allows us to use the track view as
// a non-linear animation editor.  Isn't that cool?!
Interval CATClipValue::GetLayerTimeRange(int index, DWORD flags)
{
	Control *pLayerCtrl = GetLayer(index);
	if (pLayerCtrl == NULL)
		return NEVER;

	Interval range, iv;
	BOOL init = FALSE;
	iv = pLayerCtrl->GetTimeRange(flags);
	if (!iv.Empty()) {
		if (init) {
			range += iv.Start();
			range += iv.End();
		}
		else {
			range.Set(iv.Start(), iv.End());
			init = TRUE;
		}
	}
	return range;
}

void EditLeafTimeRange(Animatable* anim, Interval range, DWORD flags)
{
	if (anim->GetInterface(I_CONTROL))
		((Control*)anim)->EditTimeRange(range, flags);
	for (int i = 0; i < anim->NumSubs(); i++) {
		Animatable* sub = anim->SubAnim(i);
		if (sub)	EditLeafTimeRange(sub, range, flags);
	}
}

// We override these methods to do special things with
// key ranges.  This allows us to use the track view as
// a non-linear animation editor.  Isn't that cool?!
void CATClipValue::EditLayerTimeRange(int index, Interval range, DWORD flags)
{
	//	tabLayers[index]->EditTimeRange(range, flags);
	EditLeafTimeRange(tabLayers[index], range, flags);
}

void CATClipValue::MapLayerKeys(int index, TimeMap *map, DWORD flags)
{
	tabLayers[index]->MapKeys(map, flags);
}

void SetLeafORT(Animatable* anim, int ort, int type)
{
	if (anim->GetInterface(I_CONTROL))
		((Control*)anim)->SetORT(ort, type);
	for (int i = 0; i < anim->NumSubs(); i++) {
		Animatable* sub = anim->SubAnim(i);
		if (sub) SetLeafORT(sub, ort, type);
	}
}

void CATClipValue::SetLayerORT(int index, int ort, int type)
{
	// the relative repeat cycle option doesn't work very well on non-worldspace controllers
	// here we simply set non-WS controllers to loop because then even if the characters
	// graphs are not prefectly looped, the character won't fly apart.
	if (type == ORT_RELATIVE_REPEAT && !TestFlag(CLIP_FLAG_HAS_TRANSFORM)) type = ORT_LOOP;

	Control* ctrl = tabLayers[index];
	SetLeafORT(ctrl, ort, type);
}

Interval CATClipValue::GetTimeRange(DWORD flags)
{
	UNREFERENCED_PARAMETER(flags);
	return NEVER;
}

void CATClipValue::EditTimeRange(Interval range, DWORD flags)
{
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->EditTimeRange(range, flags);
	Control::EditTimeRange(range, flags);
}

void CATClipValue::MapKeys(TimeMap *map, DWORD flags)
{
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL && (flags & TRACK_DOSUBANIMS))
		return pSelectedCtrl->MapKeys(map, flags);
	Control::MapKeys(map, flags);
}

/************************************************************************/
/* UI Rollouts                                                          */
/************************************************************************/
void CATClipValue::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	DbgAssert(m_pClipRoot);
	// We save this stuff so that we know this controller is having its
	// rollouts displayed.  These are used inside CATMessage().  Have
	// a look there for more info.
	ipClip = ip;
	flagsBegin = flags;

	// show the layer manager Rollout passing a pointer to ourselves so
	// the layer UI can be relevant to us.
	if (flags&BEGIN_EDIT_MOTION)
		m_pClipRoot->BeginEditLayers(ip, flags, this, prev);

#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
	if (m_pClipRoot->TestFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS))
#endif
	{

		// display the ui of the current layer
		BeginEditLayers(GetSelectedIndex(), ip, flags, GetCATMode(), prev);
	}
}

void CATClipValue::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	DbgAssert(m_pClipRoot);

	if (flagsBegin&BEGIN_EDIT_MOTION) {
		m_pClipRoot->EndEditLayers(ip, flags, this, next);
	}

#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
	if (m_pClipRoot->TestFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS))
#endif
	{

		EndEditLayers(GetSelectedIndex(), ipClip, flags, GetCATMode(), next);
	}
	ipClip = NULL;
}

// These methods are just for managing the rollouts of our layers.
// These methods get called to display and remove rollouts whne
// things like the active layer changes, or the CATMode changes.
// During CATMode changes we need to be able to destroy rollouts and
// create new ones based on the mode change.
void CATClipValue::BeginEditLayers(int nLayer, IObjParam *ip, ULONG flags, int catmode, Animatable *prev/*=NULL*/)
{
	if (catmode == SETUPMODE) {
		if (ctrlSetup)ctrlSetup->BeginEditParams(ip, flags, prev);
		return;
	}
	Control* pLayer = GetLayer(nLayer);
	if (pLayer == NULL) return;
	pLayer->BeginEditParams(ip, flags, prev);
}

void CATClipValue::EndEditLayers(int nLayer, IObjParam *ip, ULONG flags, int catmode, Animatable *next/*=NULL*/)
{
	if (catmode == SETUPMODE) {
		if (ctrlSetup) ctrlSetup->EndEditParams(ip, flags, next);
		return;
	}

	Control* pLayer = GetLayer(nLayer);
	if (pLayer == NULL) return;
	pLayer->EndEditParams(ip, flags, next);
}

//**********************************************************************//
// From control -- for apparatus manipulation			                //
//**********************************************************************//
int		CATClipValue::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->Display(GetSelectedNLAInfo()->GetTime(t), inode, vpt, flags);
	return 0;
};
void	CATClipValue::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->GetWorldBoundBox(GetSelectedNLAInfo()->GetTime(t), inode, vpt, box);
};

int		CATClipValue::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->HitTest(GetSelectedNLAInfo()->GetTime(t), inode, type, crossing, flags, p, vpt);
	return 0;
};
void	CATClipValue::ActivateSubobjSel(int level, XFormModes& modes) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->ActivateSubobjSel(level, modes);
};
void	CATClipValue::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->SelectSubComponent(hitRec, selected, all);
};
void	CATClipValue::ClearSelection(int selLevel) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->ClearSelection(selLevel);
};
int		CATClipValue::SubObjectIndex(CtrlHitRecord *hitRec) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->SubObjectIndex(hitRec);
	return 0;
};
void	CATClipValue::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->SelectSubComponent(hitRec, selected, all, invert);
};
void CATClipValue::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->GetSubObjectCenters(cb, GetSelectedNLAInfo()->GetTime(t), node);
};
void CATClipValue::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->GetSubObjectTMs(cb, GetSelectedNLAInfo()->GetTime(t), node);
};
void CATClipValue::SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->SubMove(GetSelectedNLAInfo()->GetTime(t), partm, tmAxis, val, localOrigin);
};
void CATClipValue::SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->SubRotate(GetSelectedNLAInfo()->GetTime(t), partm, tmAxis, val, localOrigin);
};
void CATClipValue::SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) {
	Control *pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		return pSelectedCtrl->SubScale(GetSelectedNLAInfo()->GetTime(t), partm, tmAxis, val, localOrigin);
};

/************************************************************************/
/* Here we get the clip saver going                                     */
/************************************************************************/
void CATClipValue::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	// For poses we use the result of the combined layer stack.
	if (!(flags&CLIPFLAG_CLIP)) {
		if (SuperClassID() == CTRL_MATRIX3_CLASS_ID) {

			DbgAssert(FALSE); // TODO: Fix this
			//CATNodeControl* catnodecontrol = (CATNodeControl*)catcontrol;
			//save->Write(idValMatrix3, (void*)&catnodecontrol->GettmBoneWorld());
			//save->Write(idValPoint, (void*)&catnodecontrol->Getp3BoneLocalScale());

		}
		else {
			save->WritePose(timerange.Start(), this);
		}
	}
	else {
		Control* pLayerCtrl = GetLayer(layerindex);
		if (pLayerCtrl != NULL)
		{
			// This is the best way to see if the current controller hierarchy contains keys
			// the NumKeys method is not implemented by many of the controllers including PRS.
			// they will always return -1
		//	int getkeyflags = NEXTKEY_RIGHT|NEXTKEY_POS|NEXTKEY_ROT|NEXTKEY_SCALE;
		//	TimeValue nextkeyt = 0;
		//	BOOL isanimated = GetLayer(layerindex)->GetNextKeyTime(timerange.Start(), getkeyflags, nextkeyt);

			if (SuperClassID() == CTRL_MATRIX3_CLASS_ID) {
				save->WriteController(pLayerCtrl, flags, timerange);
			}
			else {
				BOOL isanimated = pLayerCtrl->IsAnimated();

				// I can't aviod saving out at least the first level of the controller hierarchy
				if (isanimated)
					save->WriteController(pLayerCtrl, flags, timerange);
				else save->WritePose(timerange.Start(), pLayerCtrl);
			}
		}
	}
}

BOOL CATClipValue::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	Interval iv = FOREVER;
	if ((flags&CLIPFLAG_ONLYWEIGHTED) && (GetWeight(range.Start(), layerindex, iv) == 0.0f))
	{
		load->SkipGroup();
		// even though we are skipping this group, it is not due to an error
		return TRUE;
	}

	BOOL done = FALSE;
	BOOL ok = TRUE;
	load->SetLayerController(this);

	// Load a clip into the passed layer
	Control* pLayerCtrl = GetLayer(layerindex);
	if (pLayerCtrl != NULL)
	{
		while (load->ok() && !done && ok) {
			if (!load->NextClause())
				return FALSE;

			// Now check the clause ID and act accordingly.
			switch (load->CurClauseID()) {
			case rigAssignment:
				// Here we find out what our base controller is
				switch (load->CurIdentifier()) {
				case idValClassIDs:
				{
					TokClassIDs classid;
					load->GetValue(classid);

					// we don't need to check anything for poses, just use the CATClipValues SetValue
					// since 1.3 Poses wouldn't be sotered for a particulatr layer. Instead the first
					// Tag in the CATClipValue would have been idValMatrix3, or idValFloat.
					// This line is here for Legacy reasons only
					if (!(flags&CLIPFLAG_CLIP)) return load->ReadController(this, range, dScale, flags);

					Class_ID newClassID(classid.usClassIDA, classid.usClassIDB);
					Control *oldLayer = pLayerCtrl;

					// are we loading an old file, onto a PRS, that used to be a rotation.
					if (load->GetVersion() < CAT_VERSION_1700 &&
						SuperClassID() == CTRL_MATRIX3_CLASS_ID &&
						classid.usSuperClassID == CTRL_ROTATION_CLASS_ID)
					{// instead load the clip into the rotation controller
						pLayerCtrl = oldLayer->GetRotationController();
					}

					if (!oldLayer || oldLayer->ClassID() != newClassID)
					{
						/*	if(flags&CLIPFLAG_DONTREASSIGN)
							{
								load->SkipGroup();
								return TRUE;
							}
						*/
						Control *newLayer = (Control*)CreateInstance(classid.usSuperClassID, newClassID);
						DbgAssert(newLayer);
						if (pLayerCtrl) newLayer->Copy(pLayerCtrl);
						// Ensure we set the controller back on the right owner!
						if (oldLayer == pLayerCtrl)
							ReplaceReference(layerindex, newLayer);
						else
							pLayerCtrl->SetRotationController(newLayer);
						pLayerCtrl = newLayer;
					}

					if (!((SuperClassID() == CTRL_MATRIX3_CLASS_ID) || (SuperClassID() == CTRL_POSITION_CLASS_ID)))
						flags = flags&~CLIPFLAG_SCALE_DATA;
					// If loading a clip, dont modify it. Set it directly into the layer.
					ok = load->ReadController(oldLayer, range, dScale, flags);
					done = TRUE;
					break;
				}
				case idFlags: {
					DWORD inheritflags = 0;
					load->GetValue(inheritflags);
					// for some retarded reason, the flags are inverted when SETTING the inheritance flags
					// when 'Getting' a clear bit means it DOES inherit,
					// while 'Setting' a clear bit means it DOESN'T.... logical aye...
					inheritflags = ~inheritflags;
					pLayerCtrl->SetInheritanceFlags(inheritflags, FALSE);
					break;
				}
								  // maybe we are a pose, in which case we really don't need a classID
				case idValFloat:
				case idValPoint:
					if (SuperClassID() == CTRL_MATRIX3_CLASS_ID) {
						DbgAssert(FALSE); // FIX THIS?
			/*			CATNodeControl* catnodecontrol = (CATNodeControl*)catcontrol;
						Point3 scale(1, 1, 1);
						if(load->GetValue(scale)){
							// Make sure our caches are up to date
							catnodecontrol->GetParentBoneScaleAndChildParent(range.Start());
							CATClipMatrix3::SetScale(range.Start(), scale, catnodecontrol->Getp3BoneParentScale(), CTRL_ABSOLUTE);
						}
			*/			break;
					}
				case idValQuat:
				case idValMatrix3:
					// GetValuePose now just processes the pose data and returns it.
					// this new method is for putting it into the controller.
					// For Clips we always want the valu eplugged straight into the correct layer
					// ignoring things like the weighting system
					if (flags&CLIPFLAG_CLIP)
						load->ReadPoseIntoController(pLayerCtrl, range.Start(), dScale, flags);
					else load->ReadPoseIntoController(this, range.Start(), dScale, flags);
					break;
				}
				break;
			case rigEndGroup:
				done = TRUE;
				break;
			case rigAbort:
			case rigEnd:
				return FALSE;
			}
		}
	}
	load->SetLayerController(NULL);
	return ok;
}

INode* RemapNode(INode *node, ICATParent* fromcatparent, ICATParent* tocatparent)
{
	if (node) {
		CATNodeControl* catnodecontrol = (CATNodeControl*)node->GetTMController()->GetInterface(I_CATNODECONTROL);
		// Make sure that our node is being constrained to a node that is our own hierarchy befre we remap it.
		if (catnodecontrol && catnodecontrol->GetCATParentTrans()->GetNode() == fromcatparent->GetNode()) {
			TSTR address = catnodecontrol->GetBoneAddress();
			return tocatparent->GetBoneByAddress(address);

		}
		else if (node == fromcatparent->GetNode()) {
			return tocatparent->GetNode();
		}
	}
	return node;
}

BOOL RemapNodePointers(Control* fromctrl, Control* toctrl, ICATParent* fromcatparent, ICATParent* tocatparent)
{
	if (fromctrl->GetInterface(LINK_CONSTRAINT_INTERFACE) && toctrl->GetInterface(LINK_CONSTRAINT_INTERFACE)) {
		ILinkCtrl* fromconstr = (ILinkCtrl*)fromctrl->GetInterface(LINK_CONSTRAINT_INTERFACE);
		ILinkCtrl* toconstr = (ILinkCtrl*)toctrl->GetInterface(LINK_CONSTRAINT_INTERFACE);
		// Remove ALL the old targets and reconfigure them all
		for (int i = 0; i < fromconstr->GetNumTargets(); i++) {
			toconstr->DeleteTarget(fromconstr->GetFrameNumber(i));
		}
		// Now add them all again
		for (int i = 0; i < fromconstr->GetNumTargets(); i++) {
			INode *node = fromconstr->GetNode(i);
			node = RemapNode(node, fromcatparent, tocatparent);
			toconstr->AddTarget(node, fromconstr->GetFrameNumber(i));
		}
	}
	else {
		int fromnumSubs = fromctrl->NumSubs();
		int tonumSubs = toctrl->NumSubs();
		int minimum_from_to = min(fromnumSubs, tonumSubs);
		for (int i = 0; i < minimum_from_to; i++)
		{
			Animatable* fromsub = fromctrl->SubAnim(i);
			Animatable* tosub = toctrl->SubAnim(i);
			if (GetControlInterface(fromsub) && GetControlInterface(tosub))
			{
				RemapNodePointers(GetControlInterface(fromsub), GetControlInterface(tosub), fromcatparent, tocatparent);
			}
			else if (fromsub->ClassID().PartA() == 130 && tosub->ClassID().PartA() == 130)
			{
				IParamBlock2* frompblock = (IParamBlock2*)fromsub;
				IParamBlock2* topblock = (IParamBlock2*)tosub;
				int numParamsFrom = frompblock->NumParams();
				for (int j = 0; j < numParamsFrom; j++)
				{
					ParamID pid_j = frompblock->IndextoID(j);
					ParamType2 type = frompblock->GetParameterType(pid_j);
					if (type == TYPE_INODE)
					{
						INode* node = frompblock->GetINode(pid_j);
						node = RemapNode(node, fromcatparent, tocatparent);
						topblock->SetValue(pid_j, 0, node);
					}
					else if (type == TYPE_INODE_TAB)
					{
						int numfromParams = frompblock->Count(pid_j);
						for (int k = 0; k < numfromParams; k++)
						{
							INode *node = frompblock->GetINode(pid_j, 0, k);
							node = RemapNode(node, fromcatparent, tocatparent);
							topblock->SetValue(pid_j, 0, node, k);
						}
					}
				}
			}
		}
	}

	return TRUE;
};

BOOL CATClipValue::PasteLayer(CATClipValue* fromctrl, int fromindex, int toindex, DWORD flags, RemapDir &remap)
{
	if (flags&PASTELAYERFLAG_INSTANCE) {
		SetLayer(toindex, fromctrl->GetLayer(fromindex));
	}
	else {
		SetLayer(toindex, (Control*)remap.CloneRef(fromctrl->GetLayer(fromindex)));

		//	RemapNodePointers(fromctrl->GetLayer(fromindex), GetLayer(toindex), fromctrl->GetRoot()->GetCATParentTrans(), GetRoot()->GetCATParentTrans());
	}
	return TRUE;
}
