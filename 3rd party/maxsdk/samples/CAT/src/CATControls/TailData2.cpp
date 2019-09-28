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

// Central Tail/Trunk/FloppyAppendage controller

 // Cat Project stuff
#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>
#include "CATRigPresets.h"
#include "ease.h"

// Max includes
#include "MaxIcon.h"

// Rig Structure
#include "ICATParent.h"
#include "Hub.h"
#include "TailData2.h"
#include "TailTrans.h"

// Layer System
#include "CATClipRoot.h"
#include "CATClipValues.h"
#include "CATClipWeights.h"
#include "CATClipValue.h"

// CATMotion
#include "CATWeight.h"
#include "CATMotionTail.h"
#include "CATMotionTailRot.h"
#include "CATHierarchyBranch2.h"

class TailData2ClassDesc : public CATControlClassDesc
{
public:
	TailData2ClassDesc()
	{
		AddInterface(IBoneGroupManagerFP::GetFnPubDesc());
		AddInterface(ITailFP::GetFnPubDesc());
	}

	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		TailData2* taildata = new TailData2(loading);
		return taildata;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_TAILDATA); }
	Class_ID		ClassID() { return TAILDATA2_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("TailData2"); }	// returns fixed parsable name (scripter-visible name)
};

static TailData2ClassDesc TailData2Desc;
ClassDesc2* GetTailData2Desc() { return &TailData2Desc; }

#include "iparamm2.h"
class TailData2ParamDlgCallBack : public ParamMap2UserDlgProc
{
	TailData2*	tail;
	CATNodeControl*	tailbone;

	ISpinnerControl *spnLength;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;

	ICustButton* btnAddArbBone;
	ICustButton *btnAddERN;

public:

	TailData2ParamDlgCallBack() : tail(NULL), tailbone(NULL), spnLength(NULL) {
		btnCopy = NULL;
		btnPaste = NULL;
		btnPasteMirrored = NULL;
		btnAddArbBone = NULL;
		btnAddERN = NULL;
	}

	void InitControls(HWND hDlg, IParamMap2 *map)
	{
		tail = (TailData2*)map->GetParamBlock()->GetOwner();
		DbgAssert(tail);
		tailbone = (CATNodeControl*)GetCOREInterface()->GetSelNode(0)->GetTMController()->GetInterface(I_CATNODECONTROL);

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		float val = tail->GetTailLength();
		spnLength = SetupFloatSpinner(hDlg, IDC_SPIN_TAIL_LENGTH, IDC_EDIT_TAIL_LENGTH, 0.0f, 1000.0f, val);
		spnLength->SetAutoScale();

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyTail"), GetString(IDS_TT_COPYTAIL)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteTail"), GetString(IDS_TT_PASTETAIL)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		// btnPasteMirrored button
		btnPasteMirrored = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE_MIRRORED));
		btnPasteMirrored->SetType(CBT_PUSH);
		btnPasteMirrored->SetButtonDownNotify(TRUE);
		btnPasteMirrored->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteTailMirrored"), GetString(IDS_TT_PASTETAILMIRROR)));
		btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

		btnAddArbBone = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDARBBONE));
		btnAddArbBone->SetType(CBT_PUSH);
		btnAddArbBone->SetButtonDownNotify(TRUE);

		btnAddERN = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDRIGGING));		btnAddERN->SetType(CBT_CHECK);		btnAddERN->SetButtonDownNotify(TRUE);
		btnAddERN->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddERN"), GetString(IDS_TT_ADDOBJS)));

		if (tail->GetCATParentTrans()->GetCATMode() != SETUPMODE) {
			btnCopy->Disable();
			btnPaste->Disable();
			btnPasteMirrored->Disable();
			btnAddArbBone->Disable();
		}
		else if (!tail->CanPasteControl())
		{
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}
	}

	void ReleaseControls()
	{
		SAFE_RELEASE_SPIN(spnLength);

		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnPasteMirrored);

		SAFE_RELEASE_BTN(btnAddArbBone);

		SAFE_RELEASE_BTN(btnAddERN);
		ExtraRigNodes *ern = (ExtraRigNodes*)tailbone->GetInterface(I_EXTRARIGNODES_FP);
		DbgAssert(ern);
		ern->IDestroyERNWindow();

		tail = NULL;
		tailbone = NULL;
	}

	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, map);
			break;
		case WM_DESTROY:
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_BUTTONDOWN:
				switch (LOWORD(wParam)) {
				case IDC_BTN_COPY:		CATControl::SetPasteControl(tail);		break;
				case IDC_BTN_PASTE:
				{
					theHold.Begin();
					tail->PasteFromCtrl(CATControl::GetPasteControl(), false);
					theHold.Accept(GetString(IDS_HLD_TAILPASTEMIRROR));
				}
				break;
				case IDC_BTN_PASTE_MIRRORED:
				{
					theHold.Begin();
					tail->PasteFromCtrl(CATControl::GetPasteControl(), true);
					theHold.Accept(GetString(IDS_HLD_TAILPASTEMIRROR));
				}
				break;
				}
				break;
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_ADDARBBONE:
					if (tailbone)
					{
						HoldActions actions(IDS_HLD_ADDBONE);
						tailbone->AddArbBone();
					}
					break;
				case IDC_BTN_ADDRIGGING: {
					ExtraRigNodes* ern = (ExtraRigNodes*)tailbone->GetInterface(I_EXTRARIGNODES_FP);
					DbgAssert(ern);
					if (btnAddERN->IsChecked())
						ern->ICreateERNWindow(hWnd);
					else ern->IDestroyERNWindow();
					break;
				}
				}
				break;
			}
			break;
		case CC_SPINNER_BUTTONDOWN:
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_TAIL_LENGTH:		if (!theHold.Holding()) theHold.Begin();			break;
			}
			break;
		case CC_SPINNER_CHANGE:
			switch (LOWORD(wParam)) {
			case IDC_SPIN_TAIL_LENGTH:
				if (theHold.Holding()) {
					DisableRefMsgs();
					theHold.Restore();
					EnableRefMsgs();
				}
				tail->SetTailLength(spnLength->GetFVal());
				break;
			}
			break;
		case CC_SPINNER_BUTTONUP:
			if (theHold.Holding()) {
				if (HIWORD(wParam))	theHold.Accept(GetString(IDS_HLD_TAILLEN));
				else				theHold.Cancel();
			}
			break;
		case WM_NOTIFY:
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }//delete this; }
};

static TailData2ParamDlgCallBack TailData2ParamsCallback;

static ParamBlockDesc2 TailData2_param_blk(TailData2::PBLOCK_REF, _T("TailData2 Params"), 0, &TailData2Desc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, TailData2::PBLOCK_REF,
	IDD_TAIL_SETUP_CAT3, IDS_TAILPARAMS, 0, 0, &TailData2ParamsCallback,

	TailData2::PB_HUB, _T("Hub"), TYPE_REFTARG, P_NO_REF /* This is the wrong way around, but to load old files, it has to be this way */, 0,
		p_end,
	TailData2::PB_TAILID_DEPRECATED, _T("TailID"), TYPE_INDEX, P_OBSOLETE, 0,
		p_default, -1,
		p_end,
	TailData2::PB_NUMBONES, _T("NumBones"), TYPE_INT, P_RESET_DEFAULT, IDS_NUMBONES,
		p_default, 0,
		p_range, 1, 100,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_TAIL_NUMLINKS, IDC_SPIN_TAIL_NUMLINKS, SPIN_AUTOSCALE,
		p_end,

	TailData2::PB_TAILTRANS_TAB, _T("Bones"), TYPE_REFTARG_TAB, 0, P_NO_REF, 0,
		p_end,
	// Added in parameter to control whether
	// tail aligns itself to the path or something else
	TailData2::PB_TAIL_STIFFNESS, _T("TailStiffness"), TYPE_FLOAT, P_ANIMATABLE, IDS_TAIL_STIFFNESS,
		p_end,
	TailData2::PB_NAME, _T(""), TYPE_STRING, 0, 0,
		p_default, _T("Tail"),
		p_ui, TYPE_EDITBOX, IDC_EDIT_NAME,
		p_end,

	TailData2::PB_COLOUR, _T("Colour"), TYPE_RGBA, 0, NULL,
		p_default, Color(Point3(1.0f, 0.0f, 0.0f)),
		p_ui, TYPE_COLORSWATCH, IDC_SWATCH_COLOR,
		p_end,

	TailData2::PB_LENGTH, _T("Length"), TYPE_FLOAT, 0, IDS_LENGTH,
		p_default, 60.0f,
		p_end,
	TailData2::PB_SIZE, _T("Size"), TYPE_FLOAT, 0, IDS_SIZE,
		p_default, 8.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TAIL_SIZE, IDC_SPIN_TAIL_SIZE, SPIN_AUTOSCALE,
		p_end,

	TailData2::PB_TAPER, _T("Taper"), TYPE_FLOAT, P_ANIMATABLE, IDS_TAPER,
		p_default, 0.5f,
		p_range, -1000.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TAIL_TAPER, IDC_SPIN_TAIL_TAPER, SPIN_AUTOSCALE,
		p_end,
	TailData2::PB_HEIGHT, _T("Height"), TYPE_FLOAT, 0, IDS_HEIGHT,
	p_default, 8.0f,
	p_range, 0.0f, 1000.0f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TAIL_HEIGHT, IDC_SPIN_TAIL_HEIGHT, SPIN_AUTOSCALE,
	p_end,

	p_end
);

RefTargetHandle TailData2::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case WEIGHTS:			return weights;
	default:				return NULL;
	}
}

void TailData2::SetReference(int i, RefTargetHandle rtarg)\
{
	switch (i)
	{
	case PBLOCK_REF:		pblock = (IParamBlock2*)rtarg; break;
	case WEIGHTS:			weights = (CATClipWeights*)rtarg; break;
	}
}

TSTR TailData2::SubAnimName(int i) {
	switch (i)
	{
	case PBLOCK_REF:	return GetString(IDS_PARAMS);
	case WEIGHTS:		return GetString(IDS_WEIGHTS);
	default:			return _T("");
	}
}

Animatable* TailData2::SubAnim(int i) {
	switch (i)
	{
	case PBLOCK_REF:	return pblock;
	case WEIGHTS:		return weights;
	default:			return NULL;
	}
}

void TailData2::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	if (bNumSegmentsChanging) return;
	flagsBegin = flags;
	this->ipbegin = ip;

	if (flagsBegin == 0) {
		// ST 17/02/04 This is to stop the tail renaming
		// every time it's UI is displayed.
		pblock->EnableNotifications(FALSE);
		TailData2Desc.BeginEditParams(ip, this, flags, prev);
		pblock->EnableNotifications(TRUE);

		CATWeight* tailstiffness = (CATWeight*)pblock->GetControllerByID(PB_TAIL_STIFFNESS);
		if (tailstiffness) {
			tailstiffness->DisplayRollout(ip, flagsBegin, NULL, GetString(IDS_TAIL_STIFFNESS1));
		}
	}
}

void TailData2::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (bNumSegmentsChanging) return;
	this->ipbegin = NULL;
	if (flagsBegin == 0) {
		CATWeight* tailstiffness = (CATWeight*)pblock->GetControllerByID(PB_TAIL_STIFFNESS);
		if (tailstiffness) tailstiffness->EndEditParams(ip, END_EDIT_REMOVEUI, NULL);

		TailData2Desc.EndEditParams(ip, this, flags, next);
	}
}

void TailData2::UpdateUI() {
	if (!ipbegin) return;
	if (flagsBegin == 0) {
		IParamMap2* pmap = pblock->GetMap();
		if (pmap) pmap->Invalidate();
	}
}

void TailData2::DestructAllCATMotionLayers()
{
	// We need to ensure that our Bone0 CATMotion
	// layers are deleted first.
	CATNodeControl* pBone = GetBone(0);
	if (pBone != NULL)
		pBone->DestructAllCATMotionLayers();
}

TailData2::TailData2(BOOL loading)
{
	weights = NULL;
	pblock = NULL;
	bNumSegmentsChanging = FALSE;
	ipbegin = NULL;

	TailData2Desc.MakeAutoParamBlocks(this);

	if (!loading)
	{
		CATWeight *tailstiffness = (CATWeight*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATWEIGHT_CLASS_ID);
		pblock->SetControllerByID(PB_TAIL_STIFFNESS, 0, tailstiffness, FALSE);
	}
}

TailData2::~TailData2()
{
	DeleteAllRefs();
}

static void TailBoneCloneNotify(void *param, NotifyInfo *info)
{
	TailData2 *tail = (TailData2*)param;
	INode *node = (INode*)info->callParam;
	if (tail->GetBone(0) != NULL &&
		tail->GetBone(0)->GetNode() == node)
	{
		Hub *hub = tail->GetHub();
		if (hub->GetTail(tail->GetTailID()) != tail)
		{
			tail->SetTailID(hub->GetNumTails());
			hub->InsertTail(tail->GetTailID(), tail);
		}
		UnRegisterNotification(TailBoneCloneNotify, tail, NOTIFY_NODE_CLONED);
	}
}

void TailData2::PostCloneManager()
{
	RegisterNotification(TailBoneCloneNotify, this, NOTIFY_NODE_CLONED);
};

RefTargetHandle TailData2::Clone(RemapDir& remap)
{
	// make a new TailData2 object to be the clone
	TailData2 *newInstance = new TailData2();
	// We add this entry now so that from now on pointers to us will be patched
	remap.AddEntry(this, newInstance);

	newInstance->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));
	newInstance->ReplaceReference(WEIGHTS, remap.CloneRef(weights));

	// CloneCATControl can handle cloning the arbitrary bones and things
	CloneCATControl(newInstance, remap);

	for (int i = 0; i < GetNumBones(); i++) {
		TailTrans *tailtrans = (TailTrans*)remap.CloneRef(GetBone(i));
		newInstance->pblock->SetValue(PB_TAILTRANS_TAB, 0, tailtrans, i);
		tailtrans->pblock->SetValue(TailTrans::PB_TAILDATA, 0, newInstance);
	}

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newInstance, remap);

	// now return the new object.
	return newInstance;
}

RefResult TailData2::NotifyRefChanged(const Interval&, RefTargetHandle hTarget, PartID&, RefMessage message, BOOL)
{
	switch (message)
	{
	case REFMSG_CHANGE:
		if (!pblock) return REF_SUCCEED;

		if (pblock == hTarget)
		{

			ParamID lastChangingParam = pblock->LastNotifyParamID();
			switch (lastChangingParam)
			{
			case PB_NUMBONES:
				CreateTailBones();
				break;
			case PB_SIZE:
			case PB_TAPER:
			case PB_HEIGHT:
				UpdateObjDim();
				break;
			case PB_COLOUR:
				UpdateTailColours(GetCOREInterface()->GetTime());
				break;
			case PB_NAME: {
				int data = -1;
				CATMessage(0, CAT_NAMECHANGE, data);
				break;
			}
			case PB_TAIL_STIFFNESS:
				UpdateObjDim();
				UpdateTailColours(GetCOREInterface()->GetTime());
				break;
			}
			break;
		}
		else if (weights == hTarget) {
			CATMessage(GetCurrentTime(), CAT_UPDATE, -1);
		}
	}

	return REF_SUCCEED;
}

int TailData2::NumLayerControllers()
{
	return 1;
}

CATClipValue* TailData2::GetLayerController(int i)
{
	return (i == 0) ? (CATClipValue*)weights : NULL;
}

int TailData2::NumChildCATControls()
{
	return GetNumBones();
}

CATControl* TailData2::GetChildCATControl(int i)
{
	if (i < GetNumBones())	return (CATControl*)GetBone(i);
	else					return NULL;
}

CATControl* TailData2::GetParentCATControl()
{
	return (CATControl*)GetHub();
}

float TailData2::GetTailStiffness(int index)
{
	int numbones = GetNumBones();
	float BoneRatio = ((float)index + 0.5f) / numbones;

	// Magic 10000 number, Taildata generates the weight graphs 1000 ticks long
	// these are only used in interpolating Quad spine Offset p3s
	int BoneIndex = (int)(BoneRatio * STEPTIME100);
	return pblock->GetFloat(PB_TAIL_STIFFNESS, BoneIndex);
}

void TailData2::ScaleTailSize(TimeValue t, const Point3& p3Scale)
{
	DbgAssert(GetCATParentTrans() != NULL);

	int lengthaxis = GetLengthAxis();
	int swaxis = (lengthaxis == Z) ? X : Z;

	float tailSize = pblock->GetFloat(PB_SIZE) * p3Scale[swaxis];
	float tailHeight = pblock->GetFloat(PB_HEIGHT) * p3Scale[Y];
	pblock->SetValue(PB_SIZE, t, tailSize);
	pblock->SetValue(PB_HEIGHT, t, tailHeight);
}

float TailData2::GetTailLength()
{
	float taillength = 0.0f;
	for (int i = 0; i < GetNumBones(); i++)
	{
		CATNodeControl* pTail = GetBone(i);
		if (pTail != NULL)
			taillength += pTail->GetObjZ();
	}
	return taillength;
};

void  TailData2::SetTailLength(float val) {
	if (val < 1.0f) return;

	HoldActions hold(IDS_HLD_LENSIZE);

	float ratio = val / GetTailLength();
	for (int i = 0; i < GetNumBones(); i++)
	{
		CATNodeControl *bone = GetBone(i);
		if (!bone || bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL) || bone->IsXReffed()) continue;

		bone->SetObjZ(bone->GetObjZ() * ratio);
	}

	Update();
}

void TailData2::CreateTailBones(BOOL loading)
{
	if (theHold.RestoreOrRedoing()) return;

	IHub *hub = GetHub();
	if (!hub) return;

	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();

	int newnBones = pblock->GetInt(PB_NUMBONES);
	int oldnBones = pblock->Count(PB_TAILTRANS_TAB);
	if (newnBones < 1)
	{
		newnBones = 1;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(PB_NUMBONES, 0, 1);
		pblock->EnableNotifications(TRUE);
	}
	if (newnBones == oldnBones)
		return;

	BlockEvaluation block(this);
	// If there were previously no tail links, the base node is the hub
	// node.  Otherwise it is the first tail link's node.
	//INode *baseNode = hub->GetNode();
	bNumSegmentsChanging = TRUE;

	float oldlength = GetTailLength();

	//
	// The tail is shrinking.  Remove the surplus links from the end and
	// destroy their associated nodes.
	//
	if (oldnBones > newnBones)
	{
		int SelCount = ip->GetSelNodeCount();

		// Remove surplus segments
		for (int nSeg = (oldnBones - 1); nSeg >= newnBones; nSeg--)
		{
			CATNodeControl* pTail = GetBone(nSeg);
			if (pTail == NULL)
				continue;

			INode *node = pTail->GetNode();
			if (node)
			{
				if (SelCount == 1 && node == ip->GetSelNode(0) && nSeg > 0)
				{
					// Deselect this node, as its about to be deleted
					// Do this to prevent crash when useing Object panel to delete stuff
					CATNodeControl* pParTail = GetBone(nSeg - 1);
					if (pParTail != NULL)
						ip->SelectNode(pParTail->GetNode());
					ip->DeSelectNode(node);
				}
				// Delete the Bone Node, and any extra bones
				pTail->DeleteBoneHierarchy();
				pblock->SetCount(PB_TAILTRANS_TAB, nSeg);
			}
		}
		pblock->EnableNotifications(TRUE);
	}

	// Resize all the lists associated with tail segments.
	pblock->SetCount(PB_TAILTRANS_TAB, newnBones);

	//
	// The tail is growing.  Create new TailTrans links on the end of the
	// tail, adopting the last tail segment's setup TM, or if there were
	// no tail segments just use the identity.
	//
	if (oldnBones < newnBones)
	{
		TailTrans* ctrl;

		// Add the new segments
		for (int i = oldnBones; i < newnBones; i++)
		{
			ctrl = (TailTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, TAILTRANS_CLASS_ID);
			ctrl->Initialise(this, i, loading);

			pblock->SetValue(PB_TAILTRANS_TAB, 0, ctrl, i);
		}
	}

	bNumSegmentsChanging = FALSE;

	UpdateObjDim();
	UpdateTailColours(t);
	RenameTail();
	pblock->EnableNotifications(TRUE);

	if (!loading) {
		SetTailLength(oldlength);
	}

	return;
}

void TailData2::UpdateObjDim()
{
	float tailWidth = pblock->GetFloat(PB_SIZE);
	float tailHeight = pblock->GetFloat(PB_HEIGHT);
	float tailtaper = pblock->GetFloat(PB_TAPER);

	// Because each bone has its own length,
	// we don't set it here.
	int numbones = GetNumBones();
	float bonewidth = tailWidth;
	float boneheight = tailHeight;

	for (int i = 0; i < numbones; i++)
	{
		CATNodeControl *bone = GetBone(i);
		if (bone == NULL || bone->IsXReffed())
			continue;

		ICATObject* pObj = bone->GetICATObject();

		bonewidth = tailWidth - (tailWidth * tailtaper * GetTailStiffness(i));
		boneheight = tailHeight - (tailHeight * tailtaper * GetTailStiffness(i));

		pObj->SetX(bonewidth);
		pObj->SetY(boneheight);
		bone->UpdateCATUnits();
	}
}

void TailData2::UpdateTailColours(TimeValue t)
{
	int nnBones = GetNumBones();
	if (nnBones)
	{
		for (int j = 0; j < nnBones; j++)
		{
			CATNodeControl* pTailBone = GetBone(j);
			if (pTailBone != NULL)
				pTailBone->UpdateColour(t);
		}
	}
}
void TailData2::RenameTail()
{
	int data = 13;
	CATMessage(0, CAT_NAMECHANGE, data);
}

BOOL TailData2::LoadRig(CATRigReader *load)
{
	IHub *hub = (IHub*)pblock->GetReferenceTarget(PB_HUB);
	if (!hub) return FALSE;

	BOOL done = FALSE;
	BOOL ok = TRUE;

	int bone_number = 0;
	float taillength = 0.0f;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idTailParams) &&
				(load->CurGroupID() != idTail))return FALSE;
		}

		// Now check the clause linkID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idTailBone:
			{
				CATNodeControl* pTailBone = GetBone(bone_number);
				if (pTailBone != NULL)
				{
					pTailBone->LoadRig(load);
					bone_number++;
				}
				else
				{
					load->AssertOutOfPlace();
					load->SkipGroup();
				}
				break;
			}
			case idCATWeightParams: {

				Control *ctrlTwistWeight = pblock->GetControllerByID(PB_TAIL_STIFFNESS);
				if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
					((CATWeight*)ctrlTwistWeight)->LoadRig(load);
				break;
			}
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idBoneName:	load->ToParamBlock(pblock, PB_NAME);		break;
			case idWidth:
				load->ToParamBlock(pblock, PB_SIZE);				// tails now have a height parameter
				if (load->GetVersion() < CAT_VERSION_1150)
					pblock->SetValue(TailData2::PB_HEIGHT, 0, pblock->GetFloat(TailData2::PB_SIZE));
				break;
			case idLength:		load->GetValue(taillength);					break;
			case idHeight:		load->ToParamBlock(pblock, PB_HEIGHT);		break;
			case idTaper:		load->ToParamBlock(pblock, PB_TAPER);		break;
			case idNumLinks:
				pblock->EnableNotifications(FALSE);
				load->ToParamBlock(pblock, PB_NUMBONES);
				CreateTailBones();
				pblock->EnableNotifications(TRUE);

				if (load->GetVersion() < CAT_VERSION_1700)
				{
					for (int i = 0; i < GetNumBones(); i++) {
						CATNodeControl* pTail = GetBone(i);
						if (pTail != NULL) {
							TSTR boneid;
							boneid.printf(_T("%d"), (i + 1));
							pTail->SetName(boneid);
						}
					}
				}
				break;

			case idWeightOutTan:
			case idWeightInTan:
			case idWeightOutVal:
			case idWeightInVal:
			{
				Control *ctrlTwistWeight = pblock->GetControllerByID(PB_TAIL_STIFFNESS);
				if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
				{
					IParamBlock2 *twistBlock = ctrlTwistWeight->GetParamBlock(0);
					switch (load->CurIdentifier()) {
					case idWeightOutTan: load->ToParamBlock(twistBlock, CATWeight::PB_KEY1OUTTANLEN); break;
					case idWeightInTan:  load->ToParamBlock(twistBlock, CATWeight::PB_KEY2INTANLEN); break;
					case idWeightOutVal: load->ToParamBlock(twistBlock, CATWeight::PB_KEY1VAL); break;
					case idWeightInVal:  load->ToParamBlock(twistBlock, CATWeight::PB_KEY2VAL); break;
					}
				}
				else
					load->SkipNextTokenOrValue();
				break;
			}
			case idSetupTM:
			{
				int numTailBones = GetNumBones();
				if (numTailBones <= bone_number) break;

				Matrix3 tmSetup;
				load->GetValue(tmSetup);
				CATNodeControl *tail_bone = GetBone(bone_number);
				if (tail_bone)
					tail_bone->GetLayerTrans()->SetSetupVal((void*)&tmSetup);

				bone_number++;
				break;
			}
			default:
				load->AssertOutOfPlace();
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	if (ok && load->GetVersion() < CAT_VERSION_2100) {
		SetTailLength(taillength);
	}

	return ok && load->ok();
}

BOOL TailData2::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idTail);

	int numbones = GetNumBones();
	if (numbones == 0) {
		save->EndGroup();
		return TRUE;
	}
	save->FromParamBlock(pblock, idBoneName, PB_NAME);
	save->FromParamBlock(pblock, idWidth, PB_SIZE);
	save->FromParamBlock(pblock, idHeight, PB_HEIGHT);
	save->FromParamBlock(pblock, idTaper, PB_TAPER);

	Control *ctrlTailStiffness = pblock->GetControllerByID(PB_TAIL_STIFFNESS);
	if (ctrlTailStiffness && ctrlTailStiffness->ClassID() == CATWEIGHT_CLASS_ID)
		((CATWeight*)ctrlTailStiffness)->SaveRig(save);

	save->Write(idNumLinks, numbones);
	for (int i = 0; i < numbones; i++)
	{
		CATNodeControl* pTail = GetBone(i);
		if (pTail != NULL)
			pTail->SaveRig(save);
	}

	save->EndGroup();
	return TRUE;
}

BOOL TailData2::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	// by default bones are not in world space
	flags &= ~CLIPFLAG_WORLDSPACE;

	int bone_number = 0;
	int numbones = GetNumBones();

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idTailParams) &&
				(load->CurGroupID() != idTail))return FALSE;
		}

		// Now check the clause linkID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idController:
			{
				if (bone_number >= numbones)
				{
					load->SkipGroup();
					break;
				}
				CATNodeControl *bone = GetBone(bone_number);
				if (bone)
					bone->GetLayerTrans()->LoadClip(load, range, layerindex, dScale, flags);
				else load->SkipGroup();
				bone_number++;
				break;
			}
			case idGroupWeights:
			{
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				ok = weights->LoadClip(load, range, layerindex, dScale, newflags);
				break;
			}
			case idTailBone:
				if (bone_number >= numbones || GetBone(bone_number) == NULL) {
					load->SkipGroup();
					break;
				}
				DbgAssert(GetBone(bone_number));
				GetBone(bone_number)->LoadClip(load, range, layerindex, dScale, flags);
				bone_number++;
				break;
			case idCATWeightParams: {
				Control *ctrlTailStiffness = pblock->GetControllerByID(PB_TAIL_STIFFNESS);
				if (ctrlTailStiffness && ctrlTailStiffness->ClassID() == CATWEIGHT_CLASS_ID)
					((CATWeight*)ctrlTailStiffness)->LoadRig(load);
				break;
			}
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}

			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idSubNum:
			{
				// Old skool method. We dont save out for this format anymore, its only here for old versions
				int subNum;
				load->GetValue(subNum);

				load->NextClause();
				if (load->CurIdentifier() == idController)
				{
					CATNodeControl* pTailBone = GetBone(subNum);
					if (pTailBone != NULL)
						pTailBone->GetLayerTrans()->LoadClip(load, range, layerindex, dScale, flags);
					else load->SkipGroup();
				}

				break;
			}
			default:
				load->AssertOutOfPlace();
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	return ok && load->ok();
}

BOOL TailData2::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	int numbones = GetNumBones();
	if (!numbones) {
		return TRUE;
	}
	save->BeginGroup(idTail);

	save->Comment(GetName());

	if (flags&CLIPFLAG_CLIP) {
		save->BeginGroup(idGroupWeights);
		weights->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}

	for (int i = 0; i < numbones; i++)
	{
		CATNodeControl* pTailBone = GetBone(i);
		if (pTailBone != NULL)
			pTailBone->SaveClip(save, flags, timerange, layerindex);
	}
	save->EndGroup();
	return TRUE;
}

CatAPI::INodeControl* TailData2::GetBoneINodeControl(int index)
{
	return GetBone(index);
}

CATNodeControl* TailData2::GetBone(int index)
{
	if (index >= 0 && index < GetNumBones())
		return static_cast<CATNodeControl*>(pblock->GetReferenceTarget(PB_TAILTRANS_TAB, 0, index));
	return NULL;
}

void TailData2::Initialise(Hub *hub, int id, BOOL loading, int numbones)
{
	SetTailID(id);
	pblock->SetValue(PB_HUB, 0, hub);

	CATClipWeights* newweights = CreateClipWeightsController((CATClipWeights*)NULL, GetCATParentTrans(), loading);
	ReplaceReference(WEIGHTS, (RefTargetHandle)newweights);

	if (!loading) {
		SetNumBones(numbones);
	}
}

CatAPI::IHub* TailData2::GetIHub()
{
	return GetHub();
}

BOOL TailData2::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	TailData2* pastetail = (TailData2*)pastectrl;

	//	Force the number of bones to be the same
	SetNumBones(pastetail->GetNumBones());

	pblock->EnableNotifications(FALSE);
	// paste the tail stiffnessgraph
	CATWeight* tailstiffness = (CATWeight*)pblock->GetControllerByID(PB_TAIL_STIFFNESS);
	CATWeight* pastetailstiffness = (CATWeight*)pastetail->pblock->GetControllerByID(PB_TAIL_STIFFNESS);
	tailstiffness->PasteRig(pastetailstiffness);

	pblock->EnableNotifications(TRUE);

	// CATControl will handle
	CATControl::PasteRig(pastectrl, flags, scalefactor);

	return TRUE;
}

BaseInterface* TailData2::GetInterface(Interface_ID id)
{
	if (id == TAIL_INTERFACE_FP) return static_cast<ITailFP*>(this);
	else return CATControl::GetInterface(id);
}

FPInterfaceDesc* TailData2::GetDescByID(Interface_ID id) {
	if (id == TAIL_INTERFACE_FP) return ITailFP::GetDesc();
	return CATControl::GetDescByID(id);
}

TSTR	TailData2::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetTailID());
	DbgAssert(GetHub());
	return (GetHub()->GetBoneAddress() + _T(".") + bonerigname + boneid);
};

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class Tail2PLCB : public PostLoadCallback {
protected:
	TailData2 *tail;

public:
	Tail2PLCB(TailData2 *pOwner) { tail = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(tail);
		if (tail->GetFileSaveVersion() > 0) return tail->GetFileSaveVersion();
		return tail->GetCATParentTrans()->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		if (GetFileSaveVersion() < CAT_VERSION_2110) {
			tail->GetWeights()->weights = NULL;
			iload->SetObsolete();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

/**********************************************************************
 * Loading and saving....
 */

#define TAILDATA_CATCONTROL		1

IOResult TailData2::Save(ISave *isave)
{
	isave->BeginChunk(TAILDATA_CATCONTROL);
	CATControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult TailData2::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	//	DWORD nb;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case TAILDATA_CATCONTROL:
			res = CATControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new Tail2PLCB(this));
	iload->RegisterPostLoadCallback(new BoneGroupPostLoadIDPatcher(this, PB_TAILID_DEPRECATED));

	return IO_OK;
}
