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
#include "CATClipValue.h"
#include "decomp.h"
#include "CustAttrib.h"
#include "icustattribcontainer.h"

#include "CATControl.h"
#include "../CATObjects/ICATObject.h"
#include "ArbBoneTrans.h"
#include "CATClipWeights.h"
#include "CATMotionController.h"
#include "PalmTrans2.h"
#include "LimbData2.h"

// Get handle to listener
#include <maxscript\maxscript.h>
#include <maxscript\editor\scripteditor.h>

#include "../DataRestoreObj.h"
// Layer System
#include "CATClipRoot.h"
#include "RootNodeController.h"
#include "CATCharacterRemap.h"
#include "CATHierarchyBranch2.h"
#include "CATHierarchyLeaf.h"

CATControl::SuppressLinkInfoUpdate::SuppressLinkInfoUpdate()
{
	CATNodeControl::suppressLinkInfoUpdate = true;
}

CATControl::SuppressLinkInfoUpdate::~SuppressLinkInfoUpdate()
{
	CATNodeControl::suppressLinkInfoUpdate = false;
}

////////////////////////////////////////////
// Little helper class to trigger KeyFreeform flag...
CATControl::KeyFreeformMode::KeyFreeformMode(CATClipValue* pKeyingVal)
	: m_pKeyingVal(pKeyingVal)
{
	DbgAssert(m_pKeyingVal != NULL && !m_pKeyingVal->TestFlag(CLIP_FLAG_KEYFREEFORM));

	// Allow undo for this op.  If a following
	// operation modifies the flag value and
	// attempts to Hold the data, it may end
	// up holding on to data that has KeyFreeform
	// specified.
	m_pKeyingVal->SetFlag(CLIP_FLAG_KEYFREEFORM);
}
CATControl::KeyFreeformMode::~KeyFreeformMode()
{
	DbgAssert(m_pKeyingVal != NULL && m_pKeyingVal->TestFlag(CLIP_FLAG_KEYFREEFORM));

	m_pKeyingVal->ClearFlag(CLIP_FLAG_KEYFREEFORM);
}

// RAII class forces the hold to accept undo actions is suspended
// See ClearParentCATControl for usage
class ForceResumeHold
{
	int mHoldLevel;

public:
	ForceResumeHold()
		: mHoldLevel(0)
	{
		// If the hold has been suspended for whatever reason,
		// force it back to a ready state

		if (!GetCOREInterface10()->SceneResetting() && !g_bLoadingCAT3Rig)
		{
			if (theHold.IsSuspended() && theHold.GetBeginDepth() > 0)
			{
				while (theHold.IsSuspended())
				{
					theHold.Resume();
					mHoldLevel++;
				}
			}
		}
	}

	~ForceResumeHold()
	{
		// restore hack hack hack system to its previous state.
		while (mHoldLevel > 0)
		{
			theHold.Suspend();
			mHoldLevel--;
		}
	}
};

////////////////////////////////////////////
// Little helper class to trigger KeyFreeform flag...
CATControl::BlockEvaluation::BlockEvaluation(CATControl* pBlockedClass)
	: mBlockedClass(pBlockedClass)
{
	DbgAssert(mBlockedClass != NULL && !mBlockedClass->TestCCFlag(CNCFLAG_LOCK_STOP_EVALUATING));

	HoldSuspend hs;
	mBlockedClass->SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, TRUE);
}

CATControl::BlockEvaluation::~BlockEvaluation()
{
	DbgAssert(mBlockedClass != NULL && mBlockedClass->TestCCFlag(CNCFLAG_LOCK_STOP_EVALUATING));

	HoldSuspend hs;
	mBlockedClass->SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, FALSE);
}

/////////////////////////////////////////////
// CATControl methods

int CATControl::flagsbegin = 0;
bool CATControl::suppressLinkInfoUpdate = false;
IObjParam* CATControl::ipbegin = NULL;

#ifdef _DEBUG
// cheap-ass mem-leak checking
class CheckCATControlLeak {
private:
	int mNumCATControlClasses;

public:
	CheckCATControlLeak() : mNumCATControlClasses(0) {};
	~CheckCATControlLeak() { DbgAssert(mNumCATControlClasses == 0); }

	void operator++() { mNumCATControlClasses++; }
	void operator--() { mNumCATControlClasses--; }
};

// This static object will be destructed when
// the Dll is unloaded.  At this point we can
// see if we've leaked any classes.
static CheckCATControlLeak leakCheck;
#endif;

CATControl::CATControl()
	: ccflags(0)
	, mpCATParentTrans(NULL)
	, dwFileSaveVersion(0)
	, miBoneID(-1)
{
#ifdef _DEBUG
	++leakCheck;
#endif
}

CATControl::~CATControl()
{
#ifdef _DEBUG
	--leakCheck;
#endif

	// Unsure if this is necessary, but a little
	// triple check never hurt anyone
	if (GetPasteControl() == this)
		SetPasteControl(NULL);
}

void CATControl::SetCCFlag(ULONG f, BOOL on) {
	if ((ccflags & f) != (on ? f : 0)) {
		HoldActions hold(IDS_HLD_BONESETTING);
		HoldData(ccflags);

		if (on) ccflags |= f;
		else ccflags &= ~f;

		if (f & (~(CNCFLAG_LOCK_STOP_EVALUATING | CNCFLAG_RETURN_EXTRA_KEYS | CNCFLAG_EVALUATING))) {
			UpdateUI();
		}
	}
}

void CATControl::SetBoneID(int id)
{
	// We may be holding, we need to be able to restore this operation.
	HoldData(miBoneID);
	miBoneID = id;
}

CATClipWeights* CATControl::GetClipWeights()
{
	CATGroup* pGroup = GetGroup();
	if (pGroup != NULL)
		return pGroup->GetWeights();

	return NULL;
}

ICATParentTrans*	CATControl::GetCATParentTrans(bool bSearch/*=true*/)
{
	if (mpCATParentTrans == NULL && bSearch)
		mpCATParentTrans = FindCATParentTrans();

	DbgAssert(mpCATParentTrans != NULL || !bSearch);
	return mpCATParentTrans;
}

ICATParentTrans* CATControl::FindCATParentTrans()
{
	if (mpCATParentTrans != NULL)
		return mpCATParentTrans;

	CATControl* pParentCtrl = GetParentCATControl();
	if (pParentCtrl != NULL)
		return pParentCtrl->FindCATParentTrans();

	DbgAssert("We should have found a parent by now");
	return NULL;
}

DWORD CATControl::GetFileSaveVersion() {
	// On older builds we only stored the version onthe CATParent.
	// Now we store it on every controller.
	if (dwFileSaveVersion > 0) return dwFileSaveVersion;
	assert(mpCATParentTrans);
	return mpCATParentTrans->GetFileSaveVersion();

}
void CATControl::SetName(TSTR newname, BOOL quiet/*=FALSE*/)
{
	if (newname != name)
	{
		name = newname;
		if (!quiet) CATMessage(GetCOREInterface()->GetTime(), CAT_NAMECHANGE);
	}
};

CATMode CATControl::GetCATMode() const
{
	const ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent != NULL)
	{
		return pCATParent->GetCATMode();
	}
	return SETUPMODE;
}

TSTR	CATControl::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));

	CATControl *parentcatcontrol = GetParentCATControl();
	if (parentcatcontrol)
		return (parentcatcontrol->GetBoneAddress() + _T(".") + bonerigname);
	else return (TSTR(IdentName(idSceneRootNode)) + _T(".") + bonerigname);
};

USHORT GetNextRigID(TSTR &address, int &boneid) {
	int id_index = 0;
	boneid = -1;
	while (id_index < address.Length() && address[id_index] != _T('.') && address[id_index] != _T('[')) id_index++;

	TSTR rig_id = address.Substr(0, id_index);
	address = address.remove(0, id_index); //address.Substr(id_index+1, address.Length() - (id_index+1));

	if (address[0] == _T('[')) {
		id_index = 0;
		while (id_index < address.Length() && address[id_index] != _T(']')) id_index++;
		boneid = _ttoi(address.Substr(1, id_index - 1).data());
		address = address.remove(0, id_index + 1);
	}
	if (address[0] == _T('.')) {
		address = address.remove(0, 1);
	}
	return StringIdent(rig_id);
}

INode*	CATControl::GetBoneByAddress(TSTR address)
{
	// we cannot terminate on a CATControl
	if (address.Length() == 0) return NULL;

	int boneid;
	USHORT rig_id = GetNextRigID(address, boneid);
	// we cannot terminate on a CATControl
	// If an arb bone contains another arb bone
	// then they both have the same rig id and this bails prematurely
	//	if(rig_id==GetRigID()){
	//		return NULL;
	//	}
	if (boneid >= 0) {
		if (boneid >= NumChildCATControls()) return NULL;
		if (GetChildCATControl(boneid)->GetRigID() == rig_id)
			return GetChildCATControl(boneid)->GetBoneByAddress(address);
		return NULL;
	}
	for (int i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child && child->GetRigID() == rig_id)
			return child->GetBoneByAddress(address);
	}
	// We didn't find a bone that matched the address given
	// this may be because this rigs structure is different
	// from that of the rig that generated this address
	return NULL;
};

// this will only propogate the messages
void CATControl::CATMessage(TimeValue t, UINT msg, int data)
{
	switch (msg)
	{
	case CAT_KEYFREEFORM:
		KeyFreeform(t);
		break;
	}

	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *nla = GetLayerController(i);
		if (nla) nla->CATMessage(t, msg, data);
	}

	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child) child->CATMessage(t, msg, data);
	}
}

void CATControl::KeyFreeform(TimeValue t, ULONG /*flags*/)
{
	int i;
	Interval iv;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *nla = GetLayerController(i);
		if (nla) {
			switch (nla->SuperClassID()) {
			case CTRL_FLOAT_CLASS_ID:
				float val;
				iv = FOREVER;
				nla->GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
				KeyFreeformMode SetMode(nla);
				nla->SetValue(t, (void*)&val, TRUE, CTRL_ABSOLUTE);
				break;
			}
		}
	}
}

void CATControl::GetAllChildCATControls(Tab<CATControl*>& dAllChilds)
{
	int nChildren = NumChildCATControls();
	dAllChilds.SetCount(nChildren);
	for (int i = 0; i < nChildren; i++)
		dAllChilds[i] = GetChildCATControl(i);
}

Interval CATControl::GetLayerTimeRange(int index, DWORD flags) {

	Interval range, iv;
	BOOL init = FALSE;
	int i;

	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *layer = GetLayerController(i);
		if (layer) {
			iv = layer->GetLayerTimeRange(index, flags);
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
	}
	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl *ctrl = GetChildCATControl(i);
		if (ctrl) {
			iv = ctrl->GetLayerTimeRange(index, flags);
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
	}
	return range;
}

void CATControl::EditLayerTimeRange(int index, Interval range, DWORD flags) {
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) ctrl->EditLayerTimeRange(index, range, flags);
	}

	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child) child->EditLayerTimeRange(index, range, flags);
	}
}

void CATControl::MapLayerKeys(int index, TimeMap *map, DWORD flags)
{
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) ctrl->MapLayerKeys(index, map, flags);
	}
	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child) child->MapLayerKeys(index, map, flags);
	}
}

static int CompInts(const void *elem1, const void *elem2) {
	int a = *((int*)elem1);
	int b = *((int*)elem2);
	return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

// this function isint as fast as it could be but its a trickey problem
int CATControl::GetKeyTimes(Tab<TimeValue>& times, Interval range, DWORD flags)
{
	Tab<TimeValue> layertimes;
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) {
			layertimes.SetCount(0);
			ctrl->GetKeyTimes(layertimes, range, flags);

			int prevtimecount = times.Count();
			times.SetCount(prevtimecount + layertimes.Count());
			for (int j = 0; j < layertimes.Count(); j++)		times[prevtimecount + j] = layertimes[j];
		}
	}
	times.Sort(CompInts);
	return 0;
}

BOOL CATControl::IsKeyAtTime(TimeValue t, DWORD flags) {
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		if (GetLayerController(i) && GetLayerController(i)->IsKeyAtTime(t, flags))
			return TRUE;
	}
	return FALSE;
}

BOOL CATControl::GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) {
	TimeValue at, tnear = 0;
	BOOL tnearInit = FALSE;
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		if (GetLayerController(i) && GetLayerController(i)->GetNextKeyTime(t, flags, at)) {
			if (!tnearInit) {
				tnear = at;
				tnearInit = TRUE;
			}
			else 	if (ABS(at - t) < ABS(tnear - t)) tnear = at;
		}
	}
	if (tnearInit) {
		nt = tnear;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////

void CATControl::SetLayerORT(int index, int ort, int type)
{
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) ctrl->SetLayerORT(index, ort, type);
	}
	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child) child->SetLayerORT(index, ort, type);
	}
}

void CATControl::EnableLayerORTs(int index, BOOL enable)
{
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) ctrl->GetLayer(index)->EnableORTs(enable);
	}
	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child) child->EnableLayerORTs(index, enable);
	}
}

void CATControl::CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags)
{
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) ctrl->CopyKeysFromTime(src, dst, flags);
	}
}

void CATControl::AddNewKey(TimeValue t, DWORD flags)
{
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue *ctrl = GetLayerController(i);
		if (ctrl) ctrl->AddNewKey(t, flags);
	}
}

void CATControl::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	switch (ctxt) {
		// if we are merging, force the whole character to be merged
	case kSNCFileSave:
	case kSNCFileMerge:
		GetCATParentTrans()->AddSystemNodes(nodes, ctxt);
		break;
	case kSNCClone:
	case kSNCDelete:
		// In SETUP mode, we can clone a rig element, but
		// in Animation mode just clone the whole rig. (we
		// can't update the animation that easily).
		if (GetCATMode() == SETUPMODE)
			AddSystemNodes(nodes, ctxt);
		else
			GetCATParentTrans()->AddSystemNodes(nodes, ctxt);
		break;
	default:
		DbgAssert(FALSE && _T("Deal with new method"));
	}
}

BOOL CATControl::PasteLayer(CATControl* pPasteCtrl, int fromindex, int toindex, DWORD flags, RemapDir &remap)
{
	if (pPasteCtrl == NULL)
		return FALSE;

	DbgAssert(ClassID() == pPasteCtrl->ClassID());
	if (ClassID() != pPasteCtrl->ClassID())
		return FALSE;

	// you cannot paste a layer onto its self
	if ((pPasteCtrl == this) && (fromindex == toindex))
		return FALSE;

	// Copy the actual animation information.
	for (int i = 0; i < NumLayerControllers(); i++)
	{
		CATClipValue *pLayerCtrl = GetLayerController(i);
		CATClipValue* pPastedLayerCtrl = pPasteCtrl->GetLayerController(i);
		if (pLayerCtrl && pPastedLayerCtrl)
			pLayerCtrl->PasteLayer(pPastedLayerCtrl, fromindex, toindex, flags, remap);
	}

	// Paste children
	Tab<CATControl*> dAllMyChildren;
	GetAllChildCATControls(dAllMyChildren);

	Tab<CATControl*> dAllPasteChildren;
	pPasteCtrl->GetAllChildCATControls(dAllPasteChildren);

	// Remember that not all the children are necessarily the same
	// type - and we cannot rely on the order (in dAllMyChildren/GetLayerController) to
	// give us matching children.  We need to ensure we only
	// paste matching children to matching children.
	for (int i = 0; i < dAllMyChildren.Count(); i++)
	{
		CATControl* pMyChild = dAllMyChildren[i];
		if (pMyChild == NULL)
			continue;

		Class_ID idMatching = pMyChild->ClassID();

		// Find a matching pasted controller, one that has the same ClassID as pMyChild
		// We can't guarantee order, so we look through the entire list to find matching items
		// Only un-used items are stored in the dAllPasteChildren list, so there is no
		// concern of using the same child to paste 2x
		for (int iPastedIdx = 0; iPastedIdx < dAllPasteChildren.Count(); iPastedIdx++)
		{
			CATControl* pPasteChild = dAllPasteChildren[iPastedIdx];
			if (pPasteChild != NULL && pPasteChild->ClassID() == idMatching)
			{
				pMyChild->PasteLayer(pPasteChild, fromindex, toindex, flags, remap);
				// Now the pasted child has been matched, NULL the pointer so
				// it doesn't get re-pasted to something else.
				dAllPasteChildren[iPastedIdx] = NULL;
				break;
			}
		}
	}

	return TRUE;
}

BOOL CATControl::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;
	if (pastectrl->GetCATParentTrans()->GetLengthAxis() != GetCATParentTrans()->GetLengthAxis()) return FALSE;

	BOOL firstpastedbone = FALSE;
	if (!(flags&PASTERIGFLAG_FIRST_PASTED_BONE)) {
		flags |= PASTERIGFLAG_FIRST_PASTED_BONE;
		firstpastedbone = TRUE;
	}

	if (pastectrl->GetParentCATControl() != GetParentCATControl())
		SetName(pastectrl->GetName());

	// paste all our flags at once
	if (!(flags&PASTERIGFLAG_DONT_PASTE_FLAGS)) {
		HoldData(ccflags);
		ccflags = pastectrl->ccflags;
	}

	if (!(flags&PASTERIGFLAG_DONT_PASTE_CHILDREN)) {
		// some bones may have more or less arbitrary bones attached, adn so we can only paste the minimum
		int numchildcontrols = min(NumChildCATControls(), pastectrl->NumChildCATControls());
		for (int i = 0; i < numchildcontrols; i++) {
			CATControl* child = GetChildCATControl(i);
			CATControl* pastechild = pastectrl->GetChildCATControl(i);
			if (child && !child->TestAFlag(A_IS_DELETED) &&
				pastechild && !pastechild->TestAFlag(A_IS_DELETED)) {
				child->PasteRig(pastechild, flags, scalefactor);
			}
		}
	}

	if (firstpastedbone) {
		CATCharacterRemap remap;

		// Add Required Mapping
		remap.AddEntry(pastectrl->GetCATParentTrans(), GetCATParentTrans());
		remap.AddEntry(pastectrl->GetCATParentTrans()->GetNode(), GetCATParentTrans()->GetNode());

		// Build a compete mapping of the CATRig nodes in this rig
		BuildMapping(pastectrl, remap, FALSE);
		PasteERNNodes(pastectrl, remap);
	}

	return TRUE;
}

void CATControl::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	// If we currently have the weights open for this bone,
	// we need to close it.  Our node is getting deleted,
	// so make sure that we don't just leave an empty window.
	CATClipWeights* weights = NULL;
	CATGroup* pGroup = GetGroup();
	if (pGroup != NULL)
		weights = pGroup->GetWeights();

	if (weights != NULL)
	{
		ICATParentTrans* pCPTrans = GetCATParentTrans();
		if (pCPTrans != NULL)
		{
			CATClipRoot* pClipRoot = pCPTrans->GetCATLayerRoot();
			if (pClipRoot != NULL)
			{
				if (pClipRoot->GetLayerWeightLocalControl() == weights)
					pClipRoot->CloseLocalWeightsView();
			}
		}
	}

	if (CATControl::GetPasteControl() == this && (ctxt == kSNCDelete))
		CATControl::SetPasteControl(NULL);

	for (int i = 0; i < NumChildCATControls(); i++) {
		CATControl* child = GetChildCATControl(i);
		if (child && !child->TestAFlag(A_IS_DELETED)) child->AddSystemNodes(nodes, ctxt);
	}
}

void CATControl::AddLayerControllers(Tab <Control*>	 &layerctrls)
{
	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		Control *ctrl = GetLayerController(i);
		if (ctrl) layerctrls.Append(1, &ctrl);
	}
	for (i = 0; i < NumChildCATControls(); i++) {
		CATControl* child = GetChildCATControl(i);
		if (child) child->AddLayerControllers(layerctrls);
	}
}

void CATControl::DeleteBoneHierarchy() {
	// Delete the Bone Node, and any extra bones
	Interface *ip = GetCOREInterface();
	int selectioncount = ip->GetSelNodeCount();
	INodeTab nodes;
	AddSystemNodes(nodes, kSNCDelete);
	for (int i = (nodes.Count() - 1); i >= 0; i--) {
		if (nodes[i]) {
			// we must be being called from the modifier panel
			if ((selectioncount == 1) && (nodes[i] == ip->GetSelNode(0)))
				ip->DeSelectNode(nodes[i]);
			nodes[i]->Delete(0, FALSE);
		}
	}
}

void CATControl::SetLengthAxis(int axis) {
	UNREFERENCED_PARAMETER(axis);
};

BOOL CATControl::SavePose(TSTR filename)
{
	CATClipRoot *pClipRoot = GetCATParentTrans()->GetCATLayerRoot();

	int flags = 0;
	// Hubs shouldn't Save child hubs
	if (ClassID() == HUB_CLASS_ID)
		flags |= CLIPFLAG_SKIP_SPINES;

	Interval clipLength(GetCOREInterface()->GetTime(), GetCOREInterface()->GetTime());
	Interval layerrange(pClipRoot->GetSelectedLayer(), pClipRoot->GetSelectedLayer());

	return GetCATParentTrans()->SaveClip(filename, flags, clipLength, layerrange, this);
}

// this is the function that is called by script and the rollout
INode* CATControl::LoadPose(TSTR filename, TimeValue t, BOOL bMirrorData)
{
	CATClipRoot *pClipRoot = GetCATParentTrans()->GetCATLayerRoot();
	int selected = pClipRoot->GetSelectedLayer();
	if (selected < 0 || selected > pClipRoot->NumLayers()) return FALSE;

	int flags = \
		CLIPFLAG_LOADPELVIS | \
		CLIPFLAG_LOADFEET | \
		CLIPFLAG_SCALE_DATA | \
		CLIPFLAG_APPLYTRANSFORMS;

	if (bMirrorData) {
		flags |= CLIPFLAG_MIRROR;
		// We assume we are loading onto the correct body part now.
		// We do not need to swap at any point
		flags |= CLIPFLAG_MIRROR_DATA_SWAPPED;
	}

	// this flag is turned off for body parts that are not evaluated in world space
	// at least the pelvis will be in world space.
	CATNodeControl* thisAsNodeCtrl = dynamic_cast<CATNodeControl*>(this);
	if (thisAsNodeCtrl != NULL)
	{
		CATClipMatrix3* pLayer = thisAsNodeCtrl->GetLayerTrans();
		if (pLayer != NULL && pLayer->TestFlag(CLIP_FLAG_HAS_TRANSFORM))
			flags |= CLIPFLAG_WORLDSPACE;
	}

	return GetCATParentTrans()->LoadClip(filename, selected, flags, t, this);
}

BOOL CATControl::SaveClip(TSTR filename, TimeValue start_t, TimeValue end_t)
{
	int selectdlayer = GetCATParentTrans()->GetCATLayerRoot()->GetSelectedLayer();
	if (selectdlayer<0 || start_t>end_t) {

		if (!GetCOREInterface()->GetQuietMode())
		{
			// Pop Up the Load Clip dialogue
			MakeClipPoseDlg(this, static_cast<CATClipRoot*>(GetCATParentTrans()->GetLayerRoot()), filename);
			return TRUE;
		}
		else
		{
			DbgAssert(MAX_VERSION_MAJOR < 16 && _M("NOTE: Localize the below string in Tekken"));
			the_listener->edit_stream->printf(_M("ERROR: Invalid parameters"));
			// No recovery possible here.  bail.
			return FALSE;
		}
	}
	int flags = CLIPFLAG_CLIP;
	// Hubs shouldn't Save child hubs
	if (ClassID() == HUB_CLASS_ID)
		flags |= CLIPFLAG_SKIP_SPINES;

	Interval timerange(start_t, end_t);
	Interval layerrange(selectdlayer, selectdlayer);
	return GetCATParentTrans()->SaveClip(filename, flags, timerange, layerrange, this);
}

// this is the function that is called by script and the rollout
INode* CATControl::LoadClip(TSTR filename, TimeValue t, BOOL bMirrorData)
{
	CATClipRoot *pClipRoot = GetCATParentTrans()->GetCATLayerRoot();
	int selected = pClipRoot->GetSelectedLayer();
	if (selected < 0 || selected > pClipRoot->NumLayers()) return FALSE;

	int flags = CLIPFLAG_CLIP | \
		CLIPFLAG_LOADPELVIS | \
		CLIPFLAG_LOADFEET | \
		CLIPFLAG_SCALE_DATA;

	if (bMirrorData) {
		flags |= CLIPFLAG_MIRROR;
		// We assume we are loading onto the correct body part now.
		// We do not need to swap at any point
		flags |= CLIPFLAG_MIRROR_DATA_SWAPPED;
	}

	CATNodeControl* thisAsNodeCtrl = dynamic_cast<CATNodeControl*>(this);
	if (thisAsNodeCtrl != NULL)
	{
		CATClipMatrix3* pLayer = thisAsNodeCtrl->GetLayerTrans();
		if (pLayer != NULL && pLayer->TestFlag(CLIP_FLAG_HAS_TRANSFORM))
		{
			flags |= CLIPFLAG_SCALE_DATA;
			flags |= CLIPFLAG_WORLDSPACE;
			flags |= CLIPFLAG_APPLYTRANSFORMS;
		}
	}

	return GetCATParentTrans()->LoadClip(filename, selected, flags, t, this);
}

void CATControl::PasteLayer(Control* ctrl, int fromindex, int toindex, BOOL instance)
{
	CATControl* pastectrl = (CATControl*)ctrl->GetInterface(I_CATCONTROL);
	if (!pastectrl) return;
	RemapDir *remap = NewRemapDir();
	DWORD flags = 0;
	if (instance) flags |= PASTELAYERFLAG_INSTANCE;
	theHold.Begin();
	PasteLayer(pastectrl, fromindex, toindex, flags, *remap);
	remap->DeleteThis();
	theHold.Accept(GetString(IDS_HLD_LYRPASTE));
	GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
}

BOOL CATControl::PasteFromCtrl(ReferenceTarget* ctrl, BOOL bMirrorData)
{
	CATControl* pastectrl = (CATControl*)ctrl->GetInterface(I_CATCONTROL);
	if (!pastectrl)
		return FALSE;
	if (pastectrl->GetCATParentTrans()->GetLengthAxis() != GetLengthAxis())
		return FALSE;

	float scalefactor = pastectrl->GetCATParentTrans()->GetCATUnits() / GetCATUnits();
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	DWORD flags = bMirrorData ? PASTERIGFLAG_MIRROR : 0;
	BOOL success = PasteRig(pastectrl, flags, scalefactor);

	if (theHold.Holding() && newundo) {
		if (bMirrorData)
			theHold.Accept(GetString(IDS_HLD_BONEPASTEMIRROR));
		else theHold.Accept(GetString(IDS_HLD_BONEPASTE));
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	}

	return success;
}

void CATControl::CollapsePoseToCurrLayer(TimeValue t)
{
	KeyFreeform(t);
};

void CATControl::CollapseTimeRangeToCurrLayer(TimeValue start_t, TimeValue end_t, int freq)
{
	int data = -1;
	TimeValue t;
	for (t = start_t; t <= end_t; t += freq)
		CATMessage(t, CAT_KEYFREEFORM, data);
};

void CATControl::ResetTransforms(TimeValue t)
{
	CATClipRoot *pClipRoot = GetCATParentTrans()->GetCATLayerRoot();
	int data = pClipRoot->GetSelectedLayer();
	if (data >= 0) {
		CATMessage(t, CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER, data);
	}
}

FPInterfaceDesc* CATControl::GetDescByID(Interface_ID id) {
	if (id == CATCONTROL_INTERFACE_FP) return ICATControlFP::GetFnPubDesc();
	return &nullInterface;
}

BaseInterface* CATControl::GetInterface(Interface_ID id)
{
	if (id == CATCONTROL_INTERFACE_FP) return static_cast<ICATControlFP*>(this);
	return Control::GetInterface(id);
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATControlPLCB : public PostLoadCallback {
protected:
	CATControl *ctrl;

public:
	CATControlPLCB(CATControl *pOwner) { ctrl = pOwner; }

	DWORD GetFileSaveVersion() {
		if (ctrl->GetFileSaveVersion() > 0)	return ctrl->GetFileSaveVersion();
		if (ctrl->GetCATParentTrans())		return ctrl->GetCATParentTrans()->GetFileSaveVersion();
		return 0;
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		if (!ctrl || ctrl->TestAFlag(A_IS_DELETED) || !DependentIterator(ctrl).Next())
		{
			delete this;
			return;
		}

		// In some cases, this has been left on and has broken a rig.
		ctrl->BlockAllEvaluation(FALSE);
		ctrl->ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define		CATCTRLCHUNK_CATPARENT		1
#define		CATCTRLCHUNK_FLAGS			2
#define		CATCTRLCHUNK_NAME			3
#define		CATCTRLCHUNK_SAVE_VERSION	4
#define		CATCTRLCHUNK_CATPARENTTRANS	5
#define		CATCTRLCHUNK_BONEID			6
// The following define is no longer used, but old files may still
// contain chunks matching the ID.  Do not re-use this ID
#define		CATCTRLCHUNK_BUILDNUMBER_OBSOLETE	10

IOResult CATControl::Save(ISave *isave)
{
	DWORD nb;//, refID;
	ULONG id;

	if (mpCATParentTrans) {
		isave->BeginChunk(CATCTRLCHUNK_CATPARENTTRANS);
		id = isave->GetRefID(mpCATParentTrans);
		isave->Write(&id, sizeof(ULONG), &nb);
		isave->EndChunk();

		// We only need to save this for legacy purposes.
		ECATParent* pCATParent = mpCATParentTrans->GetCATParent();
		if (pCATParent != NULL)
		{
			isave->BeginChunk(CATCTRLCHUNK_CATPARENT);
			id = isave->GetRefID(pCATParent);
			isave->Write(&id, sizeof(ULONG), &nb);
			isave->EndChunk();
		}
	}

	isave->BeginChunk(CATCTRLCHUNK_FLAGS);
	isave->Write(&ccflags, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(CATCTRLCHUNK_NAME);
	isave->WriteCString(name);
	isave->EndChunk();

	// This stores the version of CAT used to save the file.
	dwFileSaveVersion = CAT_VERSION_CURRENT;
	isave->BeginChunk(CATCTRLCHUNK_SAVE_VERSION);
	isave->Write(&dwFileSaveVersion, sizeof DWORD, &nb);
	isave->EndChunk();

	isave->BeginChunk(CATCTRLCHUNK_BONEID);
	isave->Write(&miBoneID, sizeof miBoneID, &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult CATControl::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;//, refID;
	ULONG id = 0L;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CATCTRLCHUNK_CATPARENTTRANS:
			iload->Read(&id, sizeof(ULONG), &nb);
			if (id != 0xffffffff)	iload->RecordBackpatch(id, (void**)&mpCATParentTrans);
			break;
		case CATCTRLCHUNK_FLAGS:
			res = iload->Read(&ccflags, sizeof(int), &nb);
			break;
		case CATCTRLCHUNK_NAME:
			TCHAR *strBuf;
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) name = strBuf;
			break;
		case CATCTRLCHUNK_SAVE_VERSION:
			res = iload->Read(&dwFileSaveVersion, sizeof DWORD, &nb);
			break;
		case CATCTRLCHUNK_BONEID:
			res = iload->Read(&miBoneID, sizeof(miBoneID), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	iload->RegisterPostLoadCallback(new CATControlPLCB(this));
	return IO_OK;
}

void CATControl::CloneCATControl(CATControl* clonedctrl, RemapDir& /*remap*/)
{
	if (ClassID() != clonedctrl->ClassID()) return;

	// NOTE!!!
	// We cannot patch the mpCATParent pointer, because objects are not
	// cloned until AFTER controllers are finished (and our Remap has patched).
	// It is the responsibility of CATParentTrans to ensure that the root
	// hub has a pointer to the CATParent

	// copy the ccflags
	clonedctrl->ccflags = ccflags;
	// copy the name
	clonedctrl->name = name;
	// Copy the bone ID, although this may get changed by
	// the cloned control later (for example, when cloning
	// a single leg on a character).
	clonedctrl->SetBoneID(GetBoneID());
}

void CATControl::SetPasteControl(CATControl* pasteCATControl)
{
	mpPasteCtrl = pasteCATControl;
}

CATControl* CATControl::GetPasteControl()
{
	return mpPasteCtrl;
}

BOOL CATControl::CanPasteControl()
{
	if (mpPasteCtrl == NULL)	// Sanity
		return FALSE;
	if (mpPasteCtrl == this)	// Cannot paste to yourself
		return FALSE;
	if (mpPasteCtrl->ClassID() != ClassID())	// Cannot paste to a different type of class
		return FALSE;
	if (mpPasteCtrl->GetLengthAxis() != GetLengthAxis()) // Cannot paste to a different axis system
		return FALSE;

	// Else, can paste!
	return TRUE;
}

CATControl* CATControl::mpPasteCtrl = NULL;

RefResult CATControl::AutoDelete()
{
	DbgAssert(TestAFlag(A_LOCK_TARGET) == 0);

	{
		MaxAutoDeleteLock lock(this, false);
		DestructCATControlHierarchy();
	}

	return Control::AutoDelete();
}

// This class searches a dependent tree to find multiple
// instances of a CATClipMatrix3 reference the same source controller.
class ControlIsInstanced : public DependentEnumProc
{
	CATClipValue* mOwner;	// The first owner.
	bool mIsInstanced;		// Set to true if an instance is found.

public:
	ControlIsInstanced(CATClipValue* pOwner)
		: mOwner(pOwner)
		, mIsInstanced(false)
	{
		DbgAssert(mOwner != NULL);
	}

	int proc(ReferenceMaker *rmaker)
	{
		// Do not iterate our owners tree - we
		// know we are not going to find an instance there!
		if (rmaker == mOwner)
			return DEP_ENUM_SKIP;
		if (rmaker->SuperClassID() == mOwner->SuperClassID() &&
			rmaker->ClassID() == mOwner->ClassID())
		{
			// An instance is found.  Return true to end enumeration
			mIsInstanced = true;
			return DEP_ENUM_HALT;
		}
		return DEP_ENUM_CONTINUE;
	}

	// After enumeration, call this function to see if another
	// instance of CATClipValue referenced this class
	bool ControlWasInstanced() { return mIsInstanced; }
};

void CleanCATMotionHierarchy(ReferenceTarget* pCtrl, LimbData2* pOwningLimb, bool bHasCleanedCATMotion = false)
{
	if (pCtrl == NULL)
		return;

	// If we have a leaf, it is a value that is only used by this controller
	CATHierarchyLeaf* pLeaf = dynamic_cast<CATHierarchyLeaf*>(pCtrl);
	if (pLeaf != NULL)
		pLeaf->MaybeDestructLeaf(pOwningLimb);
	else
	{
		// Once we have iterated this class's dependents, check if it is
		// a CATMotionController.  If so, then destruct it too.
		CATMotionController* pCMCtrl = dynamic_cast<CATMotionController*>(pCtrl);
		if (pCMCtrl != NULL)
		{
			// If we are the first CATMotionController encountered,
			// then trigger the destruction of that hierarchy as well.
			// It would be nice if the CATMotionLeaves could handle this
			// but at this stage, I'm not sure I could make it reliable.
			if (!bHasCleanedCATMotion)
			{
				pCMCtrl->DestructCATMotionHierarchy(pOwningLimb);
				// Do not destruct the sub-controllers of a
				// CATMotion controller
				// NOTE - siblings are fine though
				bHasCleanedCATMotion = true;
			}
			// If we are not the first CATMotionController, then
			// this controller is not being destructed (and we need to bail)
			else
				return;
		}

		// Keep searching for the leaves
		for (int k = 0; k < pCtrl->NumSubs(); k++)
			CleanCATMotionHierarchy(static_cast<ReferenceTarget*>(pCtrl->SubAnim(k)), pOwningLimb, bHasCleanedCATMotion);

	}
}

void CATControl::DestructCATControlHierarchy()
{
	MacroRecorder::MacroRecorderDisable macroRecorderDisable;

	// If our pblock has been destructed, then cancel destruction.
	// This only really happens when the creation is cancelled.  In this
	// case, the reference to the pblock is undone before the
	// class is destructed.
	DbgAssert(NumParamBlocks() <= 1); // This assumption holds true for now.
	IParamBlock2* pMyBlock = GetParamBlock(0);
	if (NumParamBlocks() == 1 && pMyBlock == NULL)
		return;

	// Destruct any layer controllers.  This function is here
	// to unlink any CATMotion controllers assigned to us from
	// the CATHierachy.  Man - 10 years, and I'm still fixing this
	// retarded dual-layer system...
	DestructAllCATMotionLayers();

	// It is required for the caller of this function to lock
	// this class, as removing these pointers could trigger
	// deletion of this class.
	DbgAssert(TestAFlag(A_LOCK_TARGET));

	// Remove the pointers between us and our parent.
	CATControl* pParentCtrl = GetParentCATControl();
	if (pParentCtrl != NULL)
	{
		// Remove our parent's pointer to us
		pParentCtrl->ClearChildCATControl(this);

#ifdef _DEBUG
		// For safety - are we sure that we were removed from our parent?
		for (int i = 0; i < pParentCtrl->NumChildCATControls(); i++)
		{
			CATControl* pChild = pParentCtrl->GetChildCATControl(i);
			DbgAssert(pChild != this);
		}
#endif

		// Remove our pointer to our parent.
		ClearParentCATControl();
		// For safety, did we clear our own parent control?
		DbgAssert(GetParentCATControl() == NULL);
	}

	// NULL any children who might be pointing at us.
	for (int i = NumChildCATControls() - 1; i >= 0; --i)
	{
		CATControl* pChild = GetChildCATControl(i);
		if (pChild != NULL)
		{
			pChild->ClearParentCATControl();
			// For safety - check that all children no longer have a parent.
			DbgAssert(pChild->GetParentCATControl() == NULL);

			// Finally, NULL any pointers to our children
			// NULL any children who might be pointing at us.
			ClearChildCATControl(pChild);
			// Was this successful?  Note: FootTrans is legal to survive this, because it
			// is sourced from a Node reference, and is guaranteed to be valid.
			//DbgAssert(GetChildCATControl(i) == NULL || pChild->ClassID() == FOOTTRANS2_CLASS_ID || pChild->ClassID() == IKTARGTRANS_CLASS_ID);
		}
	}
}

void CATControl::DestructAllCATMotionLayers()
{
	for (int i = 0; i < NumLayerControllers(); i++)
	{
		CATClipValue* pValue = GetLayerController(i);
		if (pValue == NULL)
			continue;

		for (int j = 0; j < pValue->GetNumLayers(); j++)
		{
			// Is this a CATMotion layer?
			if (pValue->GetLayerMethod(j) != LAYER_CATMOTION)
				continue;

			Control* pCtrl = pValue->GetLayer(j);
			if (pCtrl == NULL)
				continue;

			// Do not clean a layer that is instanced across several Matrix3 controllers
			ControlIsInstanced itr(pValue);
			pCtrl->DoEnumDependents(&itr);
			if (!itr.ControlWasInstanced())
			{
				// We need the pointer to our limb, if we are on a limb
				// We can find our way back to the limb most easily by looking
				// up through our groups.
				LimbData2* pLimbData = NULL;
				CATGroup* pGroup = GetGroup();
				if (pGroup != NULL)
				{
					CATControl* pGroupCtrl = pGroup->AsCATControl();
					if (pGroupCtrl != NULL)
					{
						// If our group ctrl is a LimbData, success
						if (pGroupCtrl->ClassID() == LIMBDATA2_CLASS_ID)
						{
							pLimbData = static_cast<LimbData2*>(pGroupCtrl);
						}
						else if (pGroupCtrl->ClassID() == PALMTRANS2_CLASS_ID)
						{
							// And the one special case - PalmTrans is its own group
							PalmTrans2* pPalm = static_cast<PalmTrans2*>(pGroupCtrl);
							pLimbData = pPalm->GetLimb();
						}
					}
				}

				// We are the only CATClipValue using this controller.
				// We are safe to clean the hierarchy
				// CATMotion layers operate below the controller level.  The controller
				// has a bunch of CATHierarchyBranch controllers below this level that
				// are also descendants of the CATHierarchyRoot.  To clean out the tree,
				// we need to just need to clean that other hierarchy.
				CleanCATMotionHierarchy(pCtrl, pLimbData);
			}
		}
	}
}

void ShrinkTabParameter(IParamBlock2* pblock, ParamID paramId, int initTabIndex)
{
	// if we are removing a pointer in a list, shuffle remaining down
	int nParams = pblock->Count(paramId);
	DbgAssert(initTabIndex < nParams);
	for (int k = initTabIndex + 1; k < nParams; k++)
	{
		// Shuffle boneId's down, keeping their BoneID in synch with their index.
		CATControl* pSibling = dynamic_cast<CATControl*>(pblock->GetReferenceTarget(paramId, 0, k));
		if (pSibling != NULL)
			pSibling->SetBoneID(k - 1);
	}
	pblock->Delete(paramId, initTabIndex, 1);
}

// This function will search all the stored pointers in the
// passed pblock, and if it finds pSomePtr stored in the
// block anywhere, it will NULL that pointer.
bool NullPointerInParamBlock(IParamBlock2* pblock, Control* pSomePtr)
{
	// Not sure if this assert should be here.  Technically, that
	// could just be called success??
	DbgAssert(pblock != NULL && pSomePtr != NULL);
	if (pblock == NULL || pSomePtr == NULL)
		return true;

	// Do not notify of this change.  Whatever
	// is happening, the caches will be cleared
	// when the dust settles.  Till then...
	MaxReferenceMsgLock lock;

	// Hack hack hack...
	// The parameter block for some reason suspends theHold when setting
	// its values.  That means if we take any action in response to that
	// set value (for example, clearing our parent pointer) then the
	// undo will not be recorded.  In other words, if we are executing this
	// function because somewhere up the stack we are in the middle of a
	// SetValue call, undo is disabled.  We kinda need it, so screw whatever
	// PB is doing, re-enable the undo.
	ForceResumeHold forceHoldOpen;

	// Unfortunately, we cannot use block::FindRefParam here
	// because it does not account for P_ANIMATABLE pointers
	ParamBlockDesc2* pDesc = pblock->GetDesc();
	for (int i = 0; i < pDesc->count; i++)
	{
		ParamDef& pdef = pDesc->paramdefs[i];
		ParamType2 type = base_type(pdef.type);
		ParamID paramId = pdef.ID;
		if (reftarg_type(type))
		{
			// The type is appropriate - does it have a
			// pointer to this?
			if (is_tab(pdef.type))
			{
				int nParams = pblock->Count(paramId);
				for (int j = 0; j < nParams; j++)
				{
					if (pblock->GetReferenceTarget(paramId, 0, j) == pSomePtr)
					{
#ifdef _DEBUG
						CATControl* pAsCATCtrl = dynamic_cast<CATControl*>(pSomePtr);
						if (pAsCATCtrl != NULL)
						{
							// Assumption, all pointers stored in arrays
							// are dynamic, therefore have an ID.  This
							// assumption is not necessary for this loop,
							// but this is a good test for general consistency
							DbgAssert(pAsCATCtrl->GetBoneID() >= 0);
							// I'm not sure if the following test is valid or not
							// If valid, it could be used to shorten this operation
							DbgAssert(pAsCATCtrl->GetBoneID() == j);
						}
#endif
						pblock->SetValue(paramId, 0, (ReferenceTarget*)NULL, j);
						ShrinkTabParameter(pblock, paramId, j);
						return true;
					}
				}
			}
			else
			{
				if (pblock->GetReferenceTarget(paramId) == pSomePtr)
				{
					pblock->SetValue(paramId, 0, (ReferenceTarget*)NULL);
					return true;
				}
			}
		}
		else if (pdef.flags&P_ANIMATABLE)
		{
			// This may be a controller.  Better clean it too.
			if (is_tab(pdef.type))
			{
				for (int j = 0; j < pblock->Count(paramId); j++)
				{
					if (pblock->GetControllerByIndex(i, j) == pSomePtr)
					{
						pblock->SetControllerByIndex(i, j, NULL);
						ShrinkTabParameter(pblock, paramId, j);
						return true;
					}
				}
			}
			else
			{
				if (pblock->GetControllerByIndex(i) == pSomePtr)
				{
					pblock->SetControllerByIndex(i, 0, NULL);
					return true;
				}
			}
		}
	}
	return false;
}

void CATControl::ClearParentCATControl()
{
	// Attempt an 'automatic' clean, by looking
	// through the parameter blocks for pointers.
	CATControl* pParentCtrl = GetParentCATControl();
	if (pParentCtrl != NULL)
	{
		for (int i = 0; i < NumParamBlocks(); i++)
		{
			IParamBlock2* pMyBlock = GetParamBlock(i);
			if (pMyBlock != NULL)
			{
				if (NullPointerInParamBlock(pMyBlock, pParentCtrl))
					return;
			}
		}
		DbgAssert(!_T("ERROR: We failed to clear a parent pointer!"));
	}
}

void CATControl::ClearChildCATControl(CATControl* pDestructingClass)
{
	// Attempt an 'automatic' clean, by looking
	// through the parameter blocks for pointers.
	for (int i = 0; i < NumParamBlocks(); i++)
	{
		IParamBlock2* pMyBlock = GetParamBlock(i);
		if (pMyBlock != NULL)
		{
			if (NullPointerInParamBlock(pMyBlock, pDestructingClass))
				return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CATControl ClassDesc
CATControlClassDesc::CATControlClassDesc()
{
	AddInterface(ICATControlFP::GetFnPubDesc());
}

CATControlClassDesc::~CATControlClassDesc()
{
}

SClass_ID CATControlClassDesc::SuperClassID()
{
	return REF_TARGET_CLASS_ID;
}

int CATControlClassDesc::IsPublic()
{
	return FALSE;
}

const TCHAR* CATControlClassDesc::Category()
{
	return GetString(IDS_CATEGORY);
}

HINSTANCE CATControlClassDesc::HInstance()
{
	return hInstance;
}
