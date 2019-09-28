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

// Transform Controller for the hub Rig Node. Hubs hold together limbs, tails and spines.

 // CATProject stuff
#include "CatPlugins.h"
#include "macrorec.h"
#include <CATAPI/CATClassID.h>
#include "CATMessages.h"
#include "CATRigPresets.h"

// Max Stuff
#include "iparamm2.h"
#include "decomp.h"

// CATRig Hierarchy
#include "../CATObjects/ICATObject.h"
#include "CATParentTrans.h"

// Layer System
#include "CATClipRoot.h"
#include "CATClipValues.h"
#include "CATClipWeights.h"
#include "HDPivotTrans.h"

// Rig Components
#include "Hub.h"
#include "SpineData2.h"
#include "SpineTrans2.h"
#include "ArbBoneTrans.h"
#include "LimbData2.h"
#include "TailData2.h"

#include "CATCharacterRemap.h"

//
//	Hub
//
//	Our class implementation.
//

//
//	HubTransClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class HubClassDesc : public CATNodeControlClassDesc
{
public:
	HubClassDesc()
	{
		AddInterface(IHubFP::GetFnPubDesc());
	}

	CATControl*	DoCreate(BOOL loading = FALSE)
	{
		Hub* hub = new Hub(loading);
		return hub;
	}

	const TCHAR *	ClassName() { return GetString(IDS_CL_HUBTRANS); }
	Class_ID		ClassID() { return HUB_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("Hub"); }			// returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
ClassDesc2* GetHubDesc()
{
	static HubClassDesc HubDesc;
	return &HubDesc;
}

void Hub::GetClassName(TSTR& s) { s = GetString(IDS_CL_HUBTRANS); }

enum { HubPBlock_params };

class HubMotionDlgCallBack {

	Hub *hub;
	HWND hWnd;

public:

	HWND GetHWnd() { return hWnd; }

	HubMotionDlgCallBack() : hWnd(NULL)
	{
		hub = NULL;
	}

	void InitControls(HWND hDlg, Hub *hub)
	{
		hWnd = hDlg;

		this->hub = hub;
		DbgAssert(hub);

		SET_CHECKED(hDlg, IDC_CHK_ALLOWCONSTAINED_IKROTATIONS, hub->TestCCFlag(HUBFLAG_ALLOW_IKCONTRAINT_ROT));
		SET_CHECKED(hDlg, IDC_CHK_PINBONE, hub->TestCCFlag(CCFLAG_FB_IK_LOCKED));
		SET_CHECKED(hDlg, IDC_CHK_INSPINE_LIMITS_MOVEMENT, hub->TestCCFlag(HUBFLAG_INSPINE_RESTRICTS_MOVEMENT));
	}

	void ReleaseControls(HWND hDlg)
	{
		if (hWnd != hDlg) return;
		hWnd = NULL;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			InitControls(hWnd, (Hub*)lParam);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
			{
				switch (LOWORD(wParam)) {
				case IDC_CHK_ALLOWCONSTAINED_IKROTATIONS:	hub->SetCCFlag(HUBFLAG_ALLOW_IKCONTRAINT_ROT, IS_CHECKED(hWnd, IDC_CHK_ALLOWCONSTAINED_IKROTATIONS));	break;
				case IDC_CHK_PINBONE:						hub->SetCCFlag(CCFLAG_FB_IK_LOCKED, IS_CHECKED(hWnd, IDC_CHK_PINBONE));									break;
				case IDC_CHK_INSPINE_LIMITS_MOVEMENT:		hub->SetCCFlag(HUBFLAG_INSPINE_RESTRICTS_MOVEMENT, IS_CHECKED(hWnd, IDC_CHK_INSPINE_LIMITS_MOVEMENT));	break;
				}
			}
			}
			break;
		case WM_NOTIFY:
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }
};

static HubMotionDlgCallBack HubMotionCallback;
static INT_PTR CALLBACK HubMotionProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return HubMotionCallback.DlgProc(hWnd, message, wParam, lParam);
};

class HubParamDlgCallBack : public ParamMap2UserDlgProc
{

	Hub* hub;
	ICustEdit		*edtName;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;

	ISpinnerControl *ccSpinLength;
	ISpinnerControl *ccSpinWidth;
	ISpinnerControl *ccSpinHeight;

	ICustButton* btnAddLeg;
	ICustButton* btnAddArm;
	ICustButton* btnAddSpine;
	ICustButton* btnAddTail;
	ICustButton* btnAddArbBone;
	ICustButton *btnAddERN;

public:

	HubParamDlgCallBack()
	{
		edtName = NULL;
		btnCopy = NULL;
		btnPaste = NULL;
		btnPasteMirrored = NULL;

		ccSpinLength = NULL;;
		ccSpinWidth = NULL;
		ccSpinHeight = NULL;

		hub = NULL;

		btnAddLeg = NULL;
		btnAddArm = NULL;
		btnAddSpine = NULL;
		btnAddTail = NULL;
		btnAddArbBone = NULL;
		btnAddERN = NULL;
	}

	BOOL IsDlgOpen() { return (hub != NULL); }

	void InitControls(HWND hDlg, IParamMap2 *map)
	{
		hub = (Hub*)map->GetParamBlock()->GetOwner();
		DbgAssert(hub);
		ICATObject* iobj = hub->GetICATObject();

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;
		edtName = GetICustEdit(GetDlgItem(hDlg, IDC_EDIT_NAME));
		edtName->SetText(hub->GetName().data());

		if (iobj->CustomMeshAvailable()) {
			SET_CHECKED(hDlg, IDC_CHK_USECUSTOMMESH, iobj->UsingCustomMesh());
		}
		else SendMessage(GetDlgItem(hDlg, IDC_CHK_USECUSTOMMESH), WM_ENABLE, (WPARAM)FALSE, 0);

		ccSpinLength = SetupFloatSpinner(hDlg, IDC_SPIN_LENGTH, IDC_EDIT_LENGTH, 0.0f, 1000.0f, hub->GetObjY());
		ccSpinLength->SetAutoScale();
		SetFocus(ccSpinLength->GetHwnd());

		ccSpinWidth = SetupFloatSpinner(hDlg, IDC_SPIN_WIDTH, IDC_EDIT_WIDTH, 0.0f, 1000.0f, hub->GetObjX());
		ccSpinWidth->SetAutoScale();

		ccSpinHeight = SetupFloatSpinner(hDlg, IDC_SPIN_HEIGHT, IDC_EDIT_HEIGHT, 0.0f, 1000.0f, hub->GetObjZ());
		ccSpinHeight->SetAutoScale();

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyHub"), GetString(IDS_TT_COPYHUB)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteHub"), GetString(IDS_TT_PASTEHUB)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		// btnPasteMirrored button
		btnPasteMirrored = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE_MIRRORED));
		btnPasteMirrored->SetType(CBT_PUSH);
		btnPasteMirrored->SetButtonDownNotify(TRUE);

		btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

		btnAddLeg = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDLEG));		btnAddLeg->SetType(CBT_PUSH);		btnAddLeg->SetButtonDownNotify(TRUE);
		btnAddLeg->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddLeg"), GetString(IDS_TT_ADDLEG)));

		btnAddArm = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDARM));		btnAddArm->SetType(CBT_PUSH);		btnAddArm->SetButtonDownNotify(TRUE);
		btnAddArm->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddArm"), GetString(IDS_TT_ADDARM)));

		btnAddSpine = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDSPINE));		btnAddSpine->SetType(CBT_PUSH);		btnAddSpine->SetButtonDownNotify(TRUE);
		btnAddSpine->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddSpine"), GetString(IDS_TT_ADDSPINE)));

		btnAddTail = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDTAIL));		btnAddTail->SetType(CBT_PUSH);		btnAddTail->SetButtonDownNotify(TRUE);
		btnAddTail->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddTail"), GetString(IDS_TT_ADDTAIL)));

		btnAddArbBone = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDARBBONE));	btnAddArbBone->SetType(CBT_PUSH);	btnAddArbBone->SetButtonDownNotify(TRUE);
		btnAddArbBone->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddExtraBone"), GetString(IDS_TT_ADDBONE)));

		btnAddERN = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDRIGGING));		btnAddERN->SetType(CBT_CHECK);		btnAddERN->SetButtonDownNotify(TRUE);
		btnAddERN->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddERN"), GetString(IDS_TT_ADDRIGOBJS)));

		if (hub->GetCATParentTrans()->GetCATMode() != SETUPMODE)
		{
			btnAddLeg->Disable();
			btnAddArm->Disable();
			btnAddSpine->Disable();
			btnAddTail->Disable();
			btnAddArbBone->Disable();

			btnCopy->Disable();
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}
		else if (!hub->CanPasteControl())
		{
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}
	}
	void ReleaseControls(HWND) {

		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnPasteMirrored);

		SAFE_RELEASE_SPIN(ccSpinLength);
		SAFE_RELEASE_SPIN(ccSpinWidth);
		SAFE_RELEASE_SPIN(ccSpinHeight);

		SAFE_RELEASE_BTN(btnAddLeg);
		SAFE_RELEASE_BTN(btnAddArm);

		SAFE_RELEASE_BTN(btnAddSpine);
		SAFE_RELEASE_BTN(btnAddTail);

		SAFE_RELEASE_BTN(btnAddArbBone);
		SAFE_RELEASE_BTN(btnAddERN);
		ExtraRigNodes *ern = (ExtraRigNodes*)hub->GetInterface(I_EXTRARIGNODES_FP);
		DbgAssert(ern);
		ern->IDestroyERNWindow();

		hub = NULL;
	}

	void Update()
	{
		if (ccSpinLength != NULL)
			ccSpinLength->SetValue(hub->GetObjY(), FALSE);
		if (ccSpinWidth != NULL)
			ccSpinWidth->SetValue(hub->GetObjX(), FALSE);
		if (ccSpinHeight != NULL)
			ccSpinHeight->SetValue(hub->GetObjZ(), FALSE);
	}
	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, map);
			break;

		case WM_DESTROY:
			ReleaseControls(hWnd);
			return FALSE;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_ADDLEG:		hub->AddLeg();																break;
				case IDC_BTN_ADDARM:		hub->AddArm();																break;
				case IDC_BTN_ADDSPINE:		hub->AddSpine();															break;
				case IDC_BTN_ADDTAIL:		hub->AddTail();																break;
				case IDC_BTN_COPY:			CATControl::SetPasteControl(hub);														break;
				case IDC_BTN_ADDARBBONE:
				{
					HoldActions actions(IDS_HLD_ADDBONE);
					hub->AddArbBone();
					break;
				}
				case IDC_BTN_PASTE:
				{
					theHold.Begin();
					hub->PasteFromCtrl(CATControl::GetPasteControl(), false);
					theHold.Accept(GetString(IDS_HUBPASTE));
				}
				break;
				case IDC_BTN_PASTE_MIRRORED:
				{
					theHold.Begin();
					hub->PasteFromCtrl(CATControl::GetPasteControl(), true);
					theHold.Accept(GetString(IDS_HUBPASTEMIRROR));
				}
				break;
				case IDC_BTN_ADDRIGGING: {
					ExtraRigNodes* ern = (ExtraRigNodes*)hub->GetInterface(I_EXTRARIGNODES_FP);
					DbgAssert(ern);
					if (btnAddERN->IsChecked())
						ern->ICreateERNWindow(hWnd);
					else ern->IDestroyERNWindow();
					break;
				}
				}
				break;
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_USECUSTOMMESH:
				{
					ICATObject* iobj = hub->GetICATObject();
					if (iobj)
						iobj->SetUseCustomMesh(IS_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH));
				}
				break;
				}
			}
			break;
		case WM_CUSTEDIT_ENTER:
		{
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_NAME: {	TCHAR strbuf[128];	edtName->GetText(strbuf, 64);// max 64 characters
				TSTR name(strbuf);	hub->SetName(name);				break;
			}
			}
			break;
		}
		case CC_SPINNER_BUTTONDOWN: 	theHold.Begin();	break;
		case CC_SPINNER_CHANGE: {
			if (theHold.Holding()) {
				DisableRefMsgs();
				theHold.Restore();
				EnableRefMsgs();
			}
			ISpinnerControl *ccSpinner = (ISpinnerControl*)lParam;
			float fSpinnerVal = ccSpinner->GetFVal();
			switch (LOWORD(wParam)) { // TODO put the enum into the Hub class
			case IDC_SPIN_LENGTH:
				if (!hub->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL))
					hub->SetObjY(fSpinnerVal);
				break;
			case IDC_SPIN_WIDTH:
				if (!hub->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL))
					hub->SetObjX(fSpinnerVal);
				break;
			case IDC_SPIN_HEIGHT:
				if (!hub->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL))
					hub->SetObjZ(fSpinnerVal);
				break;
			}
			break;
		}
		break;
		case CC_SPINNER_BUTTONUP: {
			if (theHold.Holding()) {
				if (HIWORD(wParam)) {
					switch (LOWORD(wParam)) {
					case IDC_SPIN_LENGTH:	theHold.Accept(GetString(IDS_HLD_OBJLEN));	break;
					case IDC_SPIN_WIDTH:	theHold.Accept(GetString(IDS_HLD_OBJWID));	break;
					case IDC_SPIN_HEIGHT:	theHold.Accept(GetString(IDS_HLD_OBJHGT));	break;
					}
				}
				else theHold.Cancel();
			}
			break;
		}
		case WM_NOTIFY:
			break;

		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }//delete this; }
};

static HubParamDlgCallBack HubParamsCallback;

static ParamBlockDesc2 Hub_param_blk(HubPBlock_params, _T("Hub params"), 0, GetHubDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI, Hub::PBLOCK,
	IDD_HUB_SETUP_CAT3, IDS_HUBPARAMS, 0, 0, &HubParamsCallback,
	Hub::PB_CATPARENT, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	//	Object Parameters
	Hub::PB_NAME, _T(""), TYPE_STRING, 0, 0,
		p_default, _T("Hub"),
		p_end,
	Hub::PB_COLOUR, _T("Colour"), TYPE_RGBA, 0, NULL,
		p_ui, TYPE_COLORSWATCH, IDC_SWATCH_COLOR,
		p_end,

	//	Animation Parameters

	Hub::PB_INSPINE, _T("InSpine"), TYPE_REFTARG, P_NO_REF, IDS_INSPINE,
		p_end,
	Hub::PB_SPINE_TAB, _T("Spines"), TYPE_REFTARG_TAB, 0, P_VARIABLE_SIZE + P_NO_REF, IDS_SPINEDATA,
		p_end,
	Hub::PB_TAIL_TAB, _T("Tails"), TYPE_REFTARG_TAB, 0, P_VARIABLE_SIZE + P_NO_REF, IDS_TAILDATA,
		p_end,
	Hub::PB_LIMB_TAB, _T("Limbs"), TYPE_REFTARG_TAB, 0, P_VARIABLE_SIZE + P_NO_REF, IDS_CL_LIMBDATA2,	// (Requires) intial size of tab
		p_end,
	p_end
);

/*
 * End published functions.
 ********************************************************************/

Hub::Hub(BOOL /*loading*/)
{
	pblock = NULL;
	layerDangleRatio = NULL;
	weights = NULL;

	m_tmInSpineTarget.IdentityMatrix();
	///////////////////////////////////////////

	GetHubDesc()->MakeAutoParamBlocks(this);
}

class HubCloneRestore : public RestoreObj {
public:
	SysNodeContext  ctxt;
	Hub*			basehub;
	SpineData2*			pInSpine;

	HubCloneRestore(Hub *hub, SysNodeContext ctxt) {
		this->ctxt = ctxt;
		pInSpine = hub->GetInSpine();
		basehub = pInSpine->GetBaseHub();
	}
	void Restore(int isUndo) {
		UNREFERENCED_PARAMETER(isUndo);
		//	if (isUndo) {
		switch (ctxt) {
		case kSNCClone:		basehub->RemoveSpine(pInSpine->GetSpineID(), FALSE);		break;
		case kSNCDelete:	basehub->InsertSpine(pInSpine->GetSpineID(), pInSpine);	break;
		}
		//	}
	}
	void Redo() {
		switch (ctxt) {
		case kSNCClone:		basehub->InsertSpine(pInSpine->GetSpineID(), pInSpine);		break;
		case kSNCDelete:	basehub->RemoveSpine(pInSpine->GetSpineID(), FALSE);			break;
		}
	}
	int Size() { return 2; }
};

static void HubBoneCloneNotify(void *param, NotifyInfo *info)
{
	Hub *hub = (Hub*)param;
	INode *node = (INode*)info->callParam;
	if (hub->GetNode() == node) {
		if (hub->GetInSpine() && hub->GetInSpine()->GetBaseHub()->GetSpine(hub->GetInSpine()->GetSpineID()) != hub->GetInSpine()) {
			Hub *basehub = hub->GetInSpine()->GetBaseHub();
			hub->GetInSpine()->SetSpineID(basehub->GetNumSpines());
			basehub->InsertSpine(hub->GetInSpine()->GetSpineID(), hub->GetInSpine());

			if (theHold.Holding()) {
				theHold.Put(new HubCloneRestore(hub, kSNCClone));
			}
		}
		UnRegisterNotification(HubBoneCloneNotify, hub, NOTIFY_NODE_CLONED);
	}
}

void Hub::PostCloneNode()
{
	if (GetInSpine()) {
		RegisterNotification(HubBoneCloneNotify, this, NOTIFY_NODE_CLONED);
	}
};

RefTargetHandle Hub::Clone(RemapDir& remap)
{
	// make a new Hub object to be the clone
	// call true for loading so the new Hub doesn't
	// make new default subcontrollers.
	Hub *pNewHub = new Hub(TRUE);

	pNewHub->ReplaceReference(PBLOCK, CloneParamBlock(pblock, remap));

	if (mLayerTrans)			pNewHub->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));
	if (layerDangleRatio)	pNewHub->ReplaceReference(LAYERDANGLERATIO, remap.CloneRef(layerDangleRatio));
	if (weights)				pNewHub->ReplaceReference(WEIGHTS, remap.CloneRef(weights));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(pNewHub, remap);

	// We add this entry now so that
	remap.AddEntry(this, pNewHub);

	int i;
	// pblocks refuse to remap thier pointers so here I force it to happen.
	// I can't fathom why they didn't patch pointers during cloning!
	for (i = 0; i < GetNumLimbs(); i++) {
		if (GetLimb(i)) {
			LimbData2* pNewlimb = static_cast<LimbData2*>(remap.CloneRef(GetLimb(i)));
			if (pNewlimb) {
				pNewHub->SetLimb(GetLimb(i)->GetLimbID(), pNewlimb);
				pNewlimb->SetHub(pNewHub);
			}
		}
	}

	for (i = 0; i < GetNumSpines(); i++) {
		if (GetSpine(i)) {
			pNewHub->pblock->SetValue(PB_SPINE_TAB, 0, remap.CloneRef(GetSpine(i)), i);
			pNewHub->GetSpine(i)->SetBaseHub(pNewHub);
		}
	}

	for (i = 0; i < GetNumTails(); i++) {
		if (GetTail(i)) {
			pNewHub->pblock->SetValue(PB_TAIL_TAB, 0, remap.CloneRef(GetTail(i)), i);
			pNewHub->GetTail(i)->SetHub(pNewHub);
		}
	}

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, pNewHub, remap);

	// now return the new object.
	return pNewHub;
}

Hub::~Hub()
{
	// Ensure we have NULL'ed the CATParent pointer to us
	CATParentTrans* pCATParentTrans = static_cast<CATParentTrans*>(GetCATParentTrans(false));
	if (pCATParentTrans != NULL)
	{
		DbgAssert(pCATParentTrans->GetRootHub() != this);
		if (pCATParentTrans->GetRootHub() == this)
			pCATParentTrans->SetRootHub(NULL);
	}

	// Now free all the actual classes.
	DeleteAllRefs();
}

void Hub::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		Hub *newctrl = (Hub*)from;

		if (newctrl->mLayerTrans) ReplaceReference(LAYERTRANS, newctrl->mLayerTrans);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

RefResult Hub::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
	{
		if (!pblock) return REF_SUCCEED;

		if (pblock == hTarg)
		{
			int ParamTabIndex = 0;
			ParamID ParamIndex = pblock->LastNotifyParamID(ParamTabIndex);
			switch (ParamIndex) {
			case PB_NAME:
				UpdateName();
				break;
			case PB_COLOUR:
			{
				TimeValue t = GetCOREInterface()->GetTime();
				SpineData2* pInSpine = GetInSpine();
				if (pInSpine)	pInSpine->CATMessage(t, CAT_COLOUR_CHANGED);
				else		CATMessage(t, CAT_COLOUR_CHANGED);
				break;
			}
			}
		}
		else if (mLayerTrans == hTarg) {
			InvalidateTransform();
			SpineData2* pInSpine = GetInSpine();
			if (pInSpine)	pInSpine->Update();
			break;
		}
		else if (layerDangleRatio == hTarg) {
			// call KeyFreeform to maintain the shape of the character.
			int data = -1;
			TimeValue t = GetCOREInterface()->GetTime();
			CATMessage(t, CAT_UPDATE, data);
			break;
		}
		else if (weights == hTarg) {
			CATMessage(GetCurrentTime(), CAT_UPDATE, -1);
		}
	} break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (mLayerTrans == hTarg)				mLayerTrans = NULL;
		else if (layerDangleRatio == hTarg)	layerDangleRatio = NULL;
		break;
	}
	return REF_SUCCEED;
}

int Hub::NumChildCATControls()
{
	return tabArbBones.Count() + GetNumLimbs() + GetNumSpines() + GetNumTails();
}

CATControl* Hub::GetChildCATControl(int i)
{
	if (i < tabArbBones.Count())	return (tabArbBones[i] && !tabArbBones[i]->TestAFlag(A_IS_DELETED)) ? (CATControl*)tabArbBones[i] : NULL;
	i -= tabArbBones.Count();
	if (i < GetNumLimbs())		return (GetLimb(i) && !((CATControl*)GetLimb(i))->TestAFlag(A_IS_DELETED)) ? (CATControl*)GetLimb(i) : NULL;
	i -= GetNumLimbs();
	if (i < GetNumSpines())		return (GetSpine(i) && !((CATControl*)GetSpine(i))->TestAFlag(A_IS_DELETED)) ? (CATControl*)GetSpine(i) : NULL;
	i -= GetNumSpines();
	if (i < GetNumTails())		return (GetTail(i) && !((CATControl*)GetTail(i))->TestAFlag(A_IS_DELETED)) ? (CATControl*)GetTail(i) : NULL;

	return NULL;
}

CATControl* Hub::GetParentCATControl()
{
	return GetInSpine();
}

INode*	Hub::GetBoneByAddress(TSTR address) {
	if (address.Length() == 0) return GetNode();
	int boneindex;
	USHORT rig_id = GetNextRigID(address, boneindex);
	DbgAssert(boneindex >= 0);// all child bones are indexed on the hub
	switch (rig_id) {
	case idArbBone:
		if (boneindex < GetNumArbBones())
		{
			CATNodeControl* pArbBone = static_cast<CATNodeControl*>(GetArbBone(boneindex));
			if (pArbBone != NULL)
				return pArbBone->GetBoneByAddress(address);
			break;
		}
	case idLimb:		if (boneindex < GetNumLimbs())		return GetLimb(boneindex)->GetBoneByAddress(address);		break;
	case idSpine:		if (boneindex < GetNumSpines())	return GetSpine(boneindex)->GetBoneByAddress(address);		break;
	case idTail:		if (boneindex < GetNumTails())		return GetTail(boneindex)->GetBoneByAddress(address);		break;
	}
	// We didn't find a bone that matched the address given
	// this may be because this rigs structure is different
	// from that of the rig that generated this address
	return NULL;
};

ISpine* Hub::AddSpine(int numbones, BOOL loading)
{
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	int numSpines = GetNumSpines();
	numSpines++;
	SpineData2 *spine = (SpineData2*)CreateInstance(REF_TARGET_CLASS_ID, SPINEDATA2_CLASS_ID);
	DbgAssert(spine);
	spine->Initialise(this, numSpines - 1, loading, numbones);
	pblock->SetCount(PB_SPINE_TAB, numSpines);
	pblock->SetValue(PB_SPINE_TAB, 0, spine, numSpines - 1);

	if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_HLD_SPINEADD)); }
	return spine;
}

void Hub::RemoveSpine(int id, BOOL deletenodes)
{
	BOOL newundo = FALSE;;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	SpineData2* spine = GetSpine(id);
	if (spine && deletenodes) {
		INodeTab nodes;
		spine->AddSystemNodes(nodes, kSNCDelete);
		for (int i = (nodes.Count() - 1); i >= 0; i--)	nodes[i]->Delete(0, FALSE);
	}

	int numspines = GetNumSpines();
	// shuffle the remaining bones down the list
	for (int i = id; i < (numspines - 1); i++) {
		SetSpine(i, GetSpine(i + 1));
		if (GetSpine(i)) GetSpine(i)->SetSpineID(i);
	}

	// remove the last limb from the array
	pblock->SetCount(PB_SPINE_TAB, numspines - 1);

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_SPINEREM));
	}
};

void Hub::InsertSpine(int id, SpineData2 *spine) {

	int numspines = GetNumSpines();
	pblock->SetCount(PB_SPINE_TAB, numspines + 1);
	SetSpine(numspines, NULL);

	// shuffle the remaining up a knotch to make room for the new limb
	if ((id < numspines - 1)) {
		for (int i = (numspines - 1); i > (id - 1); --i) {
			SetSpine(i, GetSpine(i - 1));
			if (GetSpine(i)) GetSpine(i)->SetSpineID(i);
		}
	}
	SetSpine(id, spine);
};

ITail* Hub::GetITail(int id)
{
	return GetTail(id);
}

TailData2* Hub::GetTail(int id)
{
	if (id < GetNumTails() && id >= 0)
		return static_cast<TailData2*>(pblock->GetReferenceTarget(PB_TAIL_TAB, 0, id));

	return NULL;
}

void Hub::SetTail(int id, TailData2 *newTail)
{
	pblock->SetValue(PB_TAIL_TAB, 0, newTail, id);
}

ITail* Hub::AddTail(int numbones, BOOL loading)
{
	BOOL newundo = FALSE;;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	int numTails = GetNumTails();
	numTails++;

	TailData2 *tail = (TailData2*)CreateInstance(REF_TARGET_CLASS_ID, TAILDATA2_CLASS_ID);
	DbgAssert(tail);

	tail->Initialise(this, numTails - 1, loading, numbones);
	pblock->SetCount(PB_TAIL_TAB, numTails);
	pblock->SetValue(PB_TAIL_TAB, 0, tail, numTails - 1);

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_TAILADD));
	}
	return tail;
}

void Hub::RemoveTail(int id, BOOL deletenodes)
{
	BOOL newundo = FALSE;;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	TailData2* tail = GetTail(id);
	if (tail && deletenodes) {
		INodeTab nodes;
		tail->AddSystemNodes(nodes, kSNCDelete);
		int i;
		for (i = (nodes.Count() - 1); i >= 0; i--)	nodes[i]->Delete(0, FALSE);
	}

	int numtails = GetNumTails();
	// shuffle the remaining bones down the list
	for (int i = id; i < (numtails - 1); i++) {
		SetTail(i, GetTail(i + 1));
		if (GetTail(i)) GetTail(i)->SetTailID(i);
	}

	// remove the last limb from the array
	pblock->SetCount(PB_TAIL_TAB, numtails - 1);

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_TAILREM));
	}
};

void Hub::InsertTail(int id, TailData2 *tail)
{

	int numtails = GetNumTails();
	pblock->SetCount(PB_TAIL_TAB, numtails + 1);
	SetTail(numtails, NULL);

	// shuffle the remaining up a knotch to make room for the new limb
	if ((id < numtails - 1)) {
		for (int i = (numtails - 1); i > (id - 1); --i) {
			SetTail(i, GetTail(i - 1));
			if (GetTail(i)) GetTail(i)->SetTailID(i);
		}
	}
	SetTail(id, tail);
};

//////////////////////////////////////////////////////////////////////////
// Limb-related methods

int Hub::GetNumLimbs()
{
	return pblock->Count(PB_LIMB_TAB);
}

ILimb* Hub::GetILimb(int id)
{
	return GetLimb(id);
}

void Hub::SetLimb(int id, ReferenceTarget* limb)
{
	if (id >= GetNumLimbs())
		pblock->SetCount(PB_LIMB_TAB, id + 1);
	pblock->SetValue(PB_LIMB_TAB, 0, (ReferenceTarget*)limb, id);
}

LimbData2* Hub::AddLimb(BOOL loading, ULONG flags)
{
	BOOL newundo = FALSE;;
	if (!loading && !theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	LimbData2 *newLimb = NULL;
	int numlimbs = GetNumLimbs();
	numlimbs++;

	// Macro-recorder support
	MACRO_DISABLE;
	pblock->SetCount(PB_LIMB_TAB, numlimbs);

	Random r;
	int id = numlimbs - 1;
	// Create and initialise a new limb.
	newLimb = (LimbData2*)CreateInstance(REF_TARGET_CLASS_ID, LIMBDATA2_CLASS_ID);
	newLimb->Initialise(this, id, loading, flags);
	// Generate a random colour for the limb
	Color limbColour(r.getf(1.0f, 0.0f), r.getf(1.0f, 0.0f), r.getf(1.0f, 0.0f));
	newLimb->SetGroupColour(limbColour);

	// Save to own paramblock
	pblock->SetValue(PB_LIMB_TAB, 0, newLimb, id);

	// Macro-recorder support
	MACRO_ENABLE;

	if (!loading && theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_LIMBADD));
	}
	return newLimb;
}

void Hub::RemoveLimb(int id, BOOL deletenodes/*=TRUE*/)
{
	theHold.Begin();

	LimbData2* limb = GetLimb(id);
	if (limb && deletenodes) {
		INodeTab nodes;
		limb->AddSystemNodes(nodes, kSNCDelete);
		for (int i = (nodes.Count() - 1); i >= 0; i--)	nodes[i]->Delete(0, FALSE);
	}
	// SA Note:  Calling node->Delete() resets the limb id
	// and also decrements the result of GetNumLimbs()
	// because the limbs get removed from the param block

	int numlimbs = GetNumLimbs();  // now we have one less limb than when we entered this function
	if (numlimbs > 0)
	{
		// shuffle the remaining bones down the list
		for (int i = id; i < (numlimbs - 1); i++) {
			SetLimb(i, GetLimb(i + 1));
			if (GetLimb(i)) GetLimb(i)->SetLimbID(i);
		}
	}

	theHold.Accept(GetString(IDS_HLD_REMBONE));
};

ILimb* Hub::AddArm(BOOL bCollarbone, BOOL bAnkle)
{
	UNREFERENCED_PARAMETER(bAnkle); UNREFERENCED_PARAMETER(bCollarbone);

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	int limbFlags = 0;

	limbFlags |= LIMBFLAG_ISARM;
	// arms are in fK by default
	limbFlags |= LIMBFLAG_SETUP_FK;

	// if we only have one other limb, create as opposite.
	// otherwise who cares. (this is because I just confused myself
	// deleting and creating limbs
	int iNumLimbs = GetNumLimbs();
	if (iNumLimbs == 1)
	{
		LimbData2 *aLimb = GetLimb(0);
		DbgAssert(aLimb);
		if (aLimb->GetLMR() == -1) limbFlags |= LIMBFLAG_RIGHT;
		else					  limbFlags |= LIMBFLAG_LEFT;
	}
	else
	{
		if ((iNumLimbs % 2) == 0) limbFlags |= LIMBFLAG_LEFT;
		else					 limbFlags |= LIMBFLAG_RIGHT;
	}

	LimbData2 *newLimb = static_cast<LimbData2*>(AddLimb(FALSE, limbFlags));

	int None = -1;
	newLimb->CATMessage(0, CAT_NAMECHANGE, None);
	newLimb->UpdateLimb();

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_ARMADD));
	}

	return newLimb;
}

ILimb* Hub::AddLeg(BOOL bCollarbone, BOOL bAnkle)
{
	UNREFERENCED_PARAMETER(bAnkle); UNREFERENCED_PARAMETER(bCollarbone);

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	int limbFlags = 0;

	// add an ankle to the new leg
	limbFlags |= LIMBFLAG_ISLEG;
	limbFlags |= LIMBFLAG_FFB_WORLDZ;

	// if we only have one other limb, create as opposite.
	// otherwise who cares. (this is because I just confused myself
	// deleting and creating limbs
	int iNumLimbs = GetNumLimbs();
	if (iNumLimbs == 1 && GetLimb(0))
	{
		LimbData2 *prevlimb = GetLimb(0);
		DbgAssert(prevlimb);
		if (prevlimb->GetLMR() == -1) limbFlags |= LIMBFLAG_RIGHT;
		else						 limbFlags |= LIMBFLAG_LEFT;
	}
	else
	{
		if ((iNumLimbs % 2) == 0) limbFlags |= LIMBFLAG_LEFT;
		else					 limbFlags |= LIMBFLAG_RIGHT;
	}

	LimbData2 *newLimb = static_cast<LimbData2*>(AddLimb(FALSE, limbFlags));

	int None = -1;
	newLimb->CATMessage(0, CAT_NAMECHANGE, None);

	if (GetNumLimbs() == 1 && GetInSpine())
	{
		// Calculate a new SetupTM
		// Do this before changing the parameters below,
		// because that change will cause our GetNodeTM
		// to return a different value.
		Matrix3 tmCATParent = GetCATParentTrans()->ApproxCharacterTransform(GetCOREInterface()->GetTime());
		Matrix3 tmSetup = GetNodeTM(0) * Inverse(tmCATParent);
		tmSetup.SetTrans(tmSetup.GetTrans() / GetCATParentTrans()->GetCATUnits());

		// convert the pInSpine to an absolute spine
		// and clear our inheritance flags
		GetInSpine()->SetAbsRel(0, 0.0f);

		SetLayerTransFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT, TRUE);
		SetLayerTransFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT, TRUE);

		mLayerTrans->SetSetupVal((void*)&tmSetup);
	}
	newLimb->UpdateLimb();

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_LEGADD));
	}

	return newLimb;
}

void Hub::ISetInheritsRot(BOOL b)
{
	if (b)	mLayerTrans->ClearFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
	else	mLayerTrans->SetFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
	Update();
}

void Hub::ISetInheritsPos(BOOL b)
{
	if (b)	mLayerTrans->ClearFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT);
	else	mLayerTrans->SetFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT);
	Update();
}

void Hub::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	SetXFormPacket *ptr = (SetXFormPacket*)val;

	// If we are a tip hub (if we have an in-spine), we
	// will need to modify absolute setvalues to include
	// the offset the spine will add back in on evaluation.
	bool rotateInAnimMode = (GetCATMode() == NORMAL) && (ptr->command == XFORM_ROTATE);
	SpineData2* pParentSpine = NULL;
	CATNodeControl* pParentSpineBone = GetParentCATNodeControl();
	if (pParentSpineBone != NULL)
		pParentSpine = dynamic_cast<SpineData2*>(pParentSpineBone->GetManager());

	if (pParentSpine != NULL)
	{
		// In animation mode, spine stretch is evaluated in the transformaion
		// stack instead of the set method. User doesn't expect spine stretchy
		// when rotating tip hub. Here's the mechanism to handle this:
		//    - Disable stretchy temporarily
		//    - Apply rotation
		//    - Get the new hub transform (which is the desired result)
		//    - Enable stretchy (to restore the normal transform stack)
		//    - Apply the desired transform again
		if (rotateInAnimMode)
			pParentSpine->HoldAnimStretchy(true);

		// Allow the Spine to modify our SetValue.  This allows
		// the spine to stretch in response.
		pParentSpine->SetValueHubTip(t, ptr, commit, method);
	}

	if (!GetCATParentTrans()->GetCATLayerRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS))
	{
		// the spines can restrict the movement of the
		// base hub to maintain the tip hubs transform
		int numspines = GetNumSpines();
		for (int i = 0; i < numspines; i++)
		{
			SpineData2 *spine = GetSpine(i);
			if (spine) spine->SetValueHubBase(t, ptr, commit, method);
		}
	}

	CATNodeControl::SetValue(t, val, commit, method);

	if ((pParentSpine != NULL) && rotateInAnimMode)
	{
		// Get the current transform which is evaluated with stretchy off
		// (which is the result user expected when doing rotation).
		Matrix3 tmDesired = GetNodeTM(t);

		// Re-enable stretchy to restore the normal transformation stack.
		// Set the desired transform again which would generate the value
		// that works with the normal transformation stack.
		pParentSpine->HoldAnimStretchy(false);
		Matrix3 tmParent = GetParentTM(t);
		SetXFormPacket setPkt(tmDesired, tmParent);
		SetValue(t, &setPkt, commit, method);
	}
}

void Hub::CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	// If we have a procedural in-spine, reset our tmParent
	CATNodeControl* pParent = GetParentCATNodeControl();
	if (pParent != NULL)
	{
		// FK Spines simply inherit from the spine - don't reset.
		SpineTrans2* pSpineTrans = dynamic_cast<SpineTrans2*>(pParent);
		if (pSpineTrans != NULL)
		{
			SpineData2* pSpine = static_cast<SpineData2*>(pSpineTrans->GetParentCATControl());
			if (pSpine != NULL && !pSpine->GetSpineFK())
			{
				tmParent = pSpine->GetSpineTipTM();
				CalcInheritance(t, tmParent, p3ParentScale);
				return;
			}
		}
	}
	CATNodeControl::CalcParentTransform(t, tmParent, p3ParentScale);
}

void Hub::CalcWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid)
{
	CATNodeControl::CalcWorldTransform(t, tmOrigParent, p3ParentScale, tmWorld, p3LocalScale, ivLocalValid);

	// The reach (force feedback from the limbs) is applied here as a limit for the hubs transform
	// It is calculated differently for pelvis and for ribcages. The ribcage reach
	// is calculated when the spine is calculating so the spine can end at the required position.
	// This means the ribcage is already at the correct position and only the cached rotation
	// needs to be applied.
	// For pelvii, we need to calculate the reach (knee-angle lifting, in CAT1 parlance) here entirely.

	SpineTrans2* pParent = dynamic_cast<SpineTrans2*>(GetParentCATNodeControl());
	if (pParent != NULL)
	{
		// Reaching only works on procedural spines.
		SpineData2* pSpine = pParent->GetSpine();
		if (pSpine != NULL && !pSpine->GetSpineFK())
		{
			// Cache our in target, so that we can draw our little cross thingy
			m_tmInSpineTarget = tmWorld;

			// Apply any rotations the reach may have forced.
			ApplyReachCache(tmWorld);

			// If we are attached to the end of a spine,
			// then we prevent the hub from detaching
			// TODO: Make this an actual limit - old scenes
			// could be patched to use the CNCFLAG_IMPOSE_POS_LIMITS
			// flag to prevent the ribcage from flying off, and
			// this line removed (also, CNCFLAG_IMPOSE_POS_LIMITS
			// could be optimized quite a lot...)
			Matrix3 tmSpineTip = tmOrigParent;
			pParent->CATNodeControl::ApplyChildOffset(t, tmSpineTip);
			tmWorld.SetTrans(tmSpineTip.GetTrans());
		}
	}
	else
	{
		// If we are not part of a spine, here we still need to calculate our
		// potential reach.  It is possible for the legs to lift the pelvis
		ApplyReach(t, ivLocalValid, tmWorld, tmWorld, p3LocalScale);
		ApplyReachCache(tmWorld);
	}
}
float Hub::GetDangleRatio(TimeValue t)
{
	if (!layerDangleRatio || GetCATParentTrans()->GetCATLayerRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS))
		return 0.0f;

	if (layerDangleRatio->TestFlag(CLIP_FLAG_KEYFREEFORM))
		return 0.0f;

	float dDangle;
	Interval iv = FOREVER;
	layerDangleRatio->GetValue(t, (void*)&dDangle, iv, CTRL_ABSOLUTE);
	return dDangle;
}

// This code is almost obsolete. CATMotion doesn't call this any more
void Hub::GettmOrient(TimeValue t, Matrix3& tm)
{
	DbgAssert(GetCATParentTrans() != NULL);

	if (GetCATMode() == SETUPMODE && !GetCATParentTrans()->GetCATLayerRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS)) {
		tm = GetCATParentTrans()->GettmCATParent(t);
		return;
	}

	if (!layerDangleRatio) return;

	float dDangleRatio = GetDangleRatio(t);
	if (dDangleRatio > 0.0f)
	{
		int lengthaxis = GetLengthAxis();
		AngAxis ax;
		ax.angle = acos(min(1.0f, DotProd(P3_UP_VEC, tm.GetRow(lengthaxis)))) * dDangleRatio;
		ax.axis = Normalize(CrossProd(P3_UP_VEC, tm.GetRow(lengthaxis)));
		RotateMatrix(tm, ax);
		tm.SetTrans(GetNodeTM(t).GetTrans());
	}
}

// The IK system in CAT can apply a force to the hub. This force vector
// is calcualted to enable the limb to maintain its FK length. All the limb
// contribute a force vector and the hub tries to accomadate for all the limbs
bool Hub::ApplyReach(TimeValue t, Interval& ivValid, Matrix3 &tmSpineTip, Matrix3& tmSpineResult, const Point3& p3HubScale)
{
	int numlimbs = GetNumLimbs();
	if (GetCATMode() == SETUPMODE || numlimbs == 0) return false;

	// We are basing the reach vec on the matrix passed in
	// We will be triggering evaluations that will pull tmBoneWorld.
	Point3 old_hub_pos;
	LimbData2* limb;

	float currWeight, totWeight;
	Tab <Ray> shiftDists;
	Tab <float> weights;
	shiftDists.SetCount(numlimbs);
	weights.SetCount(numlimbs);

	old_hub_pos = tmSpineResult.GetTrans();

	totWeight = 0.0f;
	int i;
	for (i = 0; i < numlimbs; i++)
	{
		limb = GetLimb(i);
		if (limb)
		{
			shiftDists[i] = limb->GetReachVec(t, ivValid, tmSpineResult, p3HubScale, currWeight);
			weights[i] = currWeight;
			totWeight += currWeight;
		}
		else
		{
			shiftDists[i].p = old_hub_pos;
			shiftDists[i].dir = P3_UP_VEC;
		}
	}

	if (totWeight <= 0.0f)
		return false;

	Point3 avgShiftDist(0.0f, 0.0f, 0.0f);
	// we are basically adding a spring here to tie the hub to its original location
	if (TestCCFlag(HUBFLAG_ALLOW_IKCONTRAINT_ROT)) totWeight = max(2.0f, totWeight);

	for (i = 0; i < numlimbs; i++)
		if (weights[i] > 0.0f)
			avgShiftDist += shiftDists[i].dir * (weights[i] / totWeight);

	tmSpineResult.Translate(avgShiftDist);

	CacheReachRotations(weights, shiftDists, old_hub_pos, tmSpineResult.GetTrans());

	tmSpineTip = tmSpineResult;
	// Return true - we did modify the transformation...

	return true;
}

void Hub::CacheReachRotations(Tab<float> weights, Tab<Ray> &shiftDists, const Point3& p3OldHubPos, const Point3& p3NewHubPos)
{
	// KALifting rotations
	if (TestCCFlag(HUBFLAG_ALLOW_IKCONTRAINT_ROT))
	{
		int nLimbs = weights.Count();
		DbgAssert(nLimbs == shiftDists.Count());

		AngAxis ax;
		Point3 vec1(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < nLimbs; i++)
		{
			if (weights[i] > 0.0f)
			{
				Point3 old_limbroot_to_old_hub_pos = Normalize(shiftDists[i].p - p3OldHubPos);
				Point3 new_limbroot_to_new_hub_pos = Normalize((shiftDists[i].p + shiftDists[i].dir) - p3NewHubPos);
				Point3 limb_rox_axis = CrossProd(new_limbroot_to_new_hub_pos, old_limbroot_to_old_hub_pos);
				limb_rox_axis *= (float)GetLimb(i)->GetLMR();

				// In the example of a hip, then these 2 vectors would be pointing up
				// along the length axis of the pelvis.
				vec1 += Normalize(CrossProd(limb_rox_axis, old_limbroot_to_old_hub_pos));
				vec2 += Normalize(CrossProd(limb_rox_axis, new_limbroot_to_new_hub_pos));

			}
		}
		// Rotate to keep as many legs as possible happy
		ax.angle = acos(max(-1.0f, min(1.0f, DotProd(Normalize(vec1), Normalize(vec2)))));
		ax.axis = Normalize(CrossProd(vec2, vec1));

		mqReachCache += ax;
	}
}

void Hub::ResetReachCache()
{
	mqReachCache.Identity();
}

void Hub::ApplyReachCache(Matrix3& tmWorld)
{
	if (!mqReachCache.IsIdentity())
	{
		Point3 pos = tmWorld.GetTrans();
		RotateMatrix(tmWorld, mqReachCache);
		tmWorld.SetTrans(pos);
	}
}

Point3 BlendVectorRadial(Point3 vec1, Point3 vec2, float ratio)
{
	if (ratio == 1.0f) return vec2;
	if (ratio == 0.0f) return vec1;

	Matrix3 tm(1);
	tm.SetAngleAxis(Normalize(CrossProd(vec1, vec2)), (float)acos(DotProd(vec1, vec2)) * ratio);
	return tm.VectorTransform(vec1);
}

Matrix3 Hub::ApproxCharacterTransform(TimeValue t)
{
	// force and evaluation of the hub at time t
	Matrix3 tmBoneWorld = GetNodeTM(t);
	int lengthaxis = GetLengthAxis();

	// Make sure do not include scale in the tm
	Matrix3 tmPathNodeGuess = tmBoneWorld;
	RotateMatrixToAlignWithVector(tmPathNodeGuess, P3_UP_VEC, lengthaxis);

	// If the character is bending over, we would rather use the length axis as the forwards direction
	// Here we rotate another matrix so that the tm is closer to the correct 'lengthaxis' == World Z
	Matrix3 tmPathNodeGuess2 = tmBoneWorld;
	if (lengthaxis == X) {
		tmPathNodeGuess2.PreRotateZ(-HALFPI);
	}
	else {
		tmPathNodeGuess2.PreRotateX(-HALFPI);
	}
	RotateMatrixToAlignWithVector(tmPathNodeGuess2, P3_UP_VEC, lengthaxis);

	// Blend the matricies based on how verticle the character is.
	float dCosRatio = DotProd(P3_UP_VEC, tmBoneWorld.GetRow(lengthaxis));
	dCosRatio = min(dCosRatio, 1.0f);
	float dRatio = acos(dCosRatio) / HALFPI;
	BlendMat(tmPathNodeGuess, tmPathNodeGuess2, dRatio);

	// Now double check that the Martics is still upright
	RotateMatrixToAlignWithVector(tmPathNodeGuess, P3_UP_VEC, lengthaxis);
	// Now take out the Forwards Vector
	Point3 forwards = tmPathNodeGuess.GetRow(Y);

	// look up through the rest of the character averaging
	// the rotations of hubs that have limbs attached
	int numspines = GetNumSpines();
	for (int i = 0; i < numspines; i++) {
		SpineData2 *spine = GetSpine(i);
		if (spine&&spine->GetTipHub()->GetNumLimbs() > 0) {
			forwards += (spine->GetTipHub()->ApproxCharacterTransform(t).GetRow(Y) / ((float)numspines + 1.0f));
		}
	}

	// we average the feet transforms here
	int numlimbs = GetNumLimbs();
	for (int i = 0; i < numlimbs; i++) {
		LimbData2 *limb = GetLimb(i);
		if (limb&&limb->GetisLeg() && limb->GetIKTarget()) {
			CATNodeControl* iktarget = ((CATNodeControl*)limb->GetIKTarget()->GetTMController()->GetInterface(I_CATNODECONTROL));
			if (iktarget != NULL)
			{
				CATClipMatrix3* pLayerTrans = iktarget->GetLayerTrans();
				DbgAssert(pLayerTrans != NULL);
				if (pLayerTrans->TestFlag(CLIP_FLAG_HAS_TRANSFORM))
					forwards += iktarget->GetNodeTM(t).GetRow(Y);
			}
		}
	}

	// Align the transform with the new forwards vector
	forwards.z = 0.0f;
	forwards = Normalize(forwards);
	RotateMatrixToAlignWithVector(tmPathNodeGuess, forwards, Y);

	tmPathNodeGuess.SetTrans(tmBoneWorld.GetTrans());
	// we don't apply the tmSetup Rotations

	if (!GetInSpine()) {

		Matrix3 tmSetup = GetSetupMatrix();

		// if we are the root hub, and we are linked to another object that is a CAT Bone
		// that is part of our own rig, then use that nodes tmSetup instead.
		// PT 02/12/08 Took out, as it is too risky to introduce for CAT 3.3.
		//	if(ipar && ipar->GetCATParentTrans()==catparenttrans){
		//		tmSetup = ipar->GetSetupMatrix();
		//	}
		tmPathNodeGuess.PreTranslate(-tmSetup.GetTrans()*GetCATUnits());
	}

	return tmPathNodeGuess;
}
/*
 *	Class Control
 */

RefTargetHandle Hub::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK:			return pblock;
	case LAYERTRANS:		return mLayerTrans;
	case LAYERDANGLERATIO:	return layerDangleRatio;
	case WEIGHTS:			return weights;
	default:				return NULL;
	}
}

void Hub::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK:			pblock = (IParamBlock2*)rtarg; break;
	case LAYERTRANS:		mLayerTrans = (CATClipMatrix3*)rtarg; break;
	case LAYERDANGLERATIO:	layerDangleRatio = (CATClipValue*)rtarg; break;
	case WEIGHTS:			weights = (CATClipWeights*)rtarg; break;
	}
}

Animatable* Hub::SubAnim(int i)
{
	switch (i)
	{
	case SUBTRANS:		return mLayerTrans;
	case SUBWEIGHTS:	return (Control*)weights;
	default:			return NULL;
	}
}

TSTR Hub::SubAnimName(int i)
{
	switch (i)
	{
	case SUBTRANS:		return GetString(IDS_LAYERTRANS);
	case SUBWEIGHTS:	return GetString(IDS_WEIGHTS);
	default:			return _T("");
	}
}

void Hub::KeyFreeform(TimeValue t, ULONG flags)//, Matrix3 tmParent)
{
	CATNodeControl::KeyFreeform(t, flags);

	// This code has been disabled because the whole dangle
	// tails feature is messed up and I finally got rid of it!!!!
	// From now on only very old layers will still require
	// this feature.
	if (layerDangleRatio) {
		KeyFreeformMode mode(layerDangleRatio);

		float dDangle = 0.0f;
		layerDangleRatio->SetValue(t, (void*)&dDangle, 1, CTRL_ABSOLUTE);
	}
}

//	Called by the limb when it needs to update everybody
void Hub::Update()
{
	// If we have re-targeting turned on,
	// then we don't want our children
	// updating to cause us to cause them to update again.
	if (!IsEvaluationBlocked()) {
		BlockEvaluation block(this);

		if (GetInSpine())
			GetInSpine()->Update();
		else {
			CATNodeControl::Update();
		}
	}
};

void Hub::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;

	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY) {

		if (flagsbegin & BEGIN_EDIT_LINKINFO) {
			ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_HUB_HIERARCHY), HubMotionProc, GetString(IDS_HUB_HIER), (LPARAM)this);
		}

		// Let the CATNode manage the UIs for motino and hierarchy panel
		CATNodeControl::BeginEditParams(ip, flags, prev);
	}
	else if (flagsbegin == 0) {
		GetHubDesc()->BeginEditParams(ip, this, flags, prev);
	}
	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void Hub::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	next = NULL;

	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY)
	{
		if (flagsbegin & BEGIN_EDIT_LINKINFO) {
			if (HubMotionCallback.GetHWnd()) {
				ip->DeleteRollupPage(HubMotionCallback.GetHWnd());
			}
		}

		// Let the CATNode manage the UIs for motion and hierarchy panel
		CATNodeControl::EndEditParams(ip, flags, next);
	}
	else if (flagsbegin == 0) {
		GetHubDesc()->EndEditParams(ip, this, flags, next);
	}

	ipbegin = NULL;
}

void Hub::Initialise(ICATParentTrans* newcatparent, BOOL loading, SpineData2* newinspine/*=NULL*/)
{
	DbgAssert(newcatparent);

	SetCATParentTrans(newcatparent);
	pblock->SetValue(PB_INSPINE, 0, newinspine);

	///////////////////////////////////////////////////////////////////////////
	// Create our Layer controllers
	// because this weights controller is not passed a parent weights controller,it will access the root directly using GetLayerWeight()
	CATClipWeights* newweights = CreateClipWeightsController((CATClipWeights*)NULL, GetCATParentTrans(), loading);
	CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), newweights, GetCATParentTrans(), loading);

	//////////////////////////////////////////////////////////////////////////
	// here we set some flags for making the layer trans inherit properly while in setup mode
	// TODO needs to be tweaked a bit for hubs that are children of non spines
	// actually this whole initialize function should look like the one on ARBTrans
	if (newinspine)
	{
		if (!newinspine->GetSpineFK())
		{
			// all hubs inherit rot fromm the CATParent
			layerController->SetFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	ReplaceReference(LAYERTRANS, (RefTargetHandle)layerController);
	ReplaceReference(WEIGHTS, (RefTargetHandle)newweights);

	Interface *ip = GetCOREInterface();
	Object *obj = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATHUB_CLASS_ID);
	DbgAssert(obj);
	INode* node = CreateNode(obj);

	if (!loading) {
		// make sure our colour swatch matches the node colour
		// this is iff we are creating hubs manually, not loading a rig preset
		Color hubColour(node->GetWireColor());
		pblock->SetValue(PB_COLOUR, 0, hubColour);

		if (!newinspine) {
			// Configure a default value
			Matrix3 tmSetup(1);
			if (GetLengthAxis() == Z)
				tmSetup.SetTrans(Point3(0.0f, 0.0f, 100.0f));
			else tmSetup.SetTrans(Point3(100.0f, 0.0f, 0.0f));
			layerController->SetSetupVal((void*)&tmSetup);
		}

		// Force the spine to figure out its name
		UpdateName();

		// Make sure this name hasn't been used.
		TSTR bonename = GetRigName();
		TSTR uniquename = bonename;
		ip->MakeNameUnique(uniquename);
		if (bonename != uniquename) {
			// Take the CATName back off
			TSTR catname = GetCATParentTrans()->GetCATName();
			bonename = uniquename.Substr(catname.Length(), uniquename.Length() - catname.Length());
			SetName(bonename);
		}
	}
	UpdateCATUnits();
}

BOOL Hub::LoadRig(CATRigReader *load)
{
	IParamBlock2 *obj_pblock = ((SimpleObject2*)GetObject())->GetParamBlockByID(HUBOBJ_PARAMBLOCK_ID);
	ICATObject* iobj = GetICATObject();

	assert(obj_pblock && iobj);

	BOOL done = FALSE;
	BOOL ok = TRUE;

	float val;
	float dangleratio = 0.0f;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idHub) &&
				(load->CurGroupID() != idHubParams)) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idObjectParams:	if (iobj) iobj->LoadRig(load);				break;
			case idController:		if (mLayerTrans) mLayerTrans->LoadRig(load);	break;
			case idArbBones:		LoadRigArbBones(load);						break;
			case idLimbParams:
			case idLimb:
			{
				LimbData2* pLimb = static_cast<LimbData2*>(AddLimb(TRUE));
				if (pLimb != NULL)
					pLimb->LoadRig(load);
				break;
			}
			case idSpineParams:
			case idSpine:
			{
				SpineData2* pNewSpine = static_cast<SpineData2*>(AddSpine(DEFAULT_NUM_CAT_BONES, TRUE));
				if (pNewSpine != NULL)
					pNewSpine->LoadRig(load);
				break;
			}
			case idTailParams:
			case idTail:
			{
				TailData2* pNewTail = static_cast<TailData2*>(AddTail(DEFAULT_NUM_CAT_BONES, TRUE));
				if (pNewTail != NULL)
					pNewTail->LoadRig(load);
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
			case idFlags:		load->GetValue(ccflags);								break;
			case idBoneName:	load->ToParamBlock(pblock, PB_NAME);					break;
			case idBoneColour:
			{
				int hubColour;
				load->GetValue(hubColour);
				Color hColour = Color(hubColour);
				pblock->SetValue(PB_COLOUR, 0, hColour);
				UpdateColour(0);
				break;
			}
			case idParentNode: {
				TSTR parent_address;
				load->GetValue(parent_address);
				load->AddParent(GetNode(), parent_address);
				break;
			}
			case idWidth:		load->GetValue(val);	SetObjX(val);		break;
			case idLength:		load->GetValue(val);	SetObjY(val);		break;
			case idHeight:		load->GetValue(val);	SetObjZ(val);		break;
			case idPivotPosY:	load->ToParamBlock(obj_pblock, HUBOBJ_PB_PIVOTPOSY);	break;
			case idPivotPosZ:	load->ToParamBlock(obj_pblock, HUBOBJ_PB_PIVOTPOSZ);	break;
			case idSetupTM: {
				Matrix3 tmSetup(1);
				load->GetValue(tmSetup);
				mLayerTrans->SetSetupVal((void*)&tmSetup);
				break;
			}
			case idDangleRatioVal:
				load->GetValue(dangleratio);
				break;
			default:			load->AssertOutOfPlace();
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
	if (ok) {

		if (load->GetVersion() < CAT_VERSION_1700) {
			if (GetInSpine()) {

				SpineData2 *sp = GetInSpine();
				float inspinelength = sp->GetSpineLength();
				BOOL doit = TRUE;
				while (doit) {
					sp = sp->GetBaseHub()->GetInSpine();
					if (!sp) break;
					inspinelength += sp->GetSpineLength();
					if (!sp->GetBaseHub()->GetInSpine()) {
						inspinelength += sp->GetBaseHub()->GetSetupMatrix().GetTrans()[Y];
					}
				}

				Matrix3 tmSetup(1);
				mLayerTrans->GetSetupVal((void*)&tmSetup);
				Point3 pos = tmSetup.GetTrans();
				pos[Y] = inspinelength;
				tmSetup.SetTrans(pos);
				mLayerTrans->SetSetupVal((void*)&tmSetup);
			}
		}

		if (load->GetVersion() < CAT_VERSION_2100) {
			if (GetNumTails() > 0 && layerDangleRatio) {
				float dangle = 0.0;
				for (int i = 0; i < GetNumTails(); i++) {
					if (GetTail(i)->GetNumBones() > 1)
						dangle = 1.0f;
				}
				// on older rigs, tails always dangled in setupmode
				layerDangleRatio->SetSetupVal((void*)&dangle);
			}
		}
		if (load->GetVersion() < CAT3_VERSION_2707) {
			HoldSuspend hs;
			SetCCFlag(CNCFLAG_INHERIT_ANIM_POS);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL);
			ClearCCFlag(CCFLAG_ANIM_STRETCHY);
		}

		if (dangleratio > 0.0 && GetNumTails() > 0 && !layerDangleRatio) {
			CATClipValue* layerDangle = CreateClipValueController(GetCATClipFloatDesc(), weights, GetCATParentTrans(), true);
			ReplaceReference(LAYERDANGLERATIO, (RefTargetHandle)layerDangle);
			layerDangleRatio->SetSetupVal((void*)&dangleratio);
		}
	}

	return ok &&load->ok();
}

BOOL Hub::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idHub);

	INode *node = GetNode();
	if (!node)
	{
		save->EndGroup();
		return save->ok();
	}

	save->FromParamBlock(pblock, idBoneName, PB_NAME);
	save->Write(idFlags, ccflags);

	Color clr = GetBonesRigColour();
	DWORD dwColour = asRGB(clr);
	save->Write(idBoneColour, dwColour);

	if (mLayerTrans) mLayerTrans->SaveRig(save);
	if (layerDangleRatio) {
		float val;
		layerDangleRatio->GetSetupVal((void*)&val);
		save->Write(idDangleRatioVal, val);
	}

	INode* parentnode = GetParentNode();
	save->Write(idParentNode, parentnode);

	ICATObject* iobj = GetICATObject();
	if (iobj) iobj->SaveRig(save);

	SaveRigArbBones(save);
	int i;
	for (i = 0; i < GetNumSpines(); i++) { if (GetSpine(i))		GetSpine(i)->SaveRig(save); }
	for (i = 0; i < GetNumTails(); i++) { if (GetTail(i))		GetTail(i)->SaveRig(save); }
	for (i = 0; i < GetNumLimbs(); i++) { if (GetLimb(i))		GetLimb(i)->SaveRig(save); }

	save->EndGroup();
	return save->ok();
}

BOOL Hub::LoadClip(CATRigReader* load, Interval timerange, int layerindex, float dScale, int flags)
{
	if (!GetCATParentTrans()) return FALSE;

	BOOL done = FALSE;
	BOOL ok = TRUE;

	int legNumber = 0, spineNumber = 0, tailNumber = 0;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idHubParams &&
				load->CurGroupID() != idHub) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idGroupWeights: {
				int newflags = flags;
				newflags &= ~CLIPFLAG_WORLDSPACE;
				newflags &= ~CLIPFLAG_APPLYTRANSFORMS;
				newflags &= ~CLIPFLAG_MIRROR;
				newflags &= ~CLIPFLAG_SCALE_DATA;
				ok = GetWeights()->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idDangleRatio: {
				if (!layerDangleRatio) {
					load->SkipGroup();
					break;
				}
				int newflags = flags;
				newflags &= ~CLIPFLAG_APPLYTRANSFORMS;
				newflags &= ~CLIPFLAG_MIRROR;
				newflags &= ~CLIPFLAG_SCALE_DATA;
				ok = layerDangleRatio->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idLimbParams:
			case idLimb:
			{
				LimbData2 *limb = GetLimb(legNumber);
				if (limb) ok = limb->LoadClip(load, timerange, layerindex, dScale, flags);
				else	 load->SkipGroup();
				legNumber++;
				break;
			}
			case idSpineParams:
			case idSpine:
			{
				SpineData2 *spine = GetSpine(spineNumber);
				if (spine) ok = spine->LoadClip(load, timerange, layerindex, dScale, flags);
				else	  load->SkipGroup();
				spineNumber++;
				break;
			}
			case idTailParams:
			case idTail:
			{
				TailData2 *tail = GetTail(tailNumber);
				if (tail) ok = tail->LoadClip(load, timerange, layerindex, dScale, flags);
				else	 load->SkipGroup();
				tailNumber++;
				break;
			}
			case idController: {

				int newflags = flags;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)) newflags |= CLIPFLAG_SKIPPOS;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)) newflags |= CLIPFLAG_SKIPROT;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) newflags |= CLIPFLAG_SKIPSCL;

				if (mLayerTrans) ok = mLayerTrans->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idArbBones:
				ok = LoadClipArbBones(load, timerange, layerindex, dScale, flags);
				break;
			case idExtraControllers:
				LoadClipExtraControllers(load, timerange, layerindex, dScale, flags);
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idValMatrix3: {
				int newflags = flags;
				Matrix3 val;
				if (GetInSpine() && (load->GetVersion() >= CAT_VERSION_1330)) {
					ok = load->ReadPoseIntoController(mLayerTrans, timerange.Start(), dScale, flags);
				}
				else {
					newflags |= CLIPFLAG_WORLDSPACE;
					// this method will do all the processing of the pose for us
					// when we set the nodetm
					if (load->GetValuePose(newflags, SuperClassID(), (void*)&val)) {

						if (flags&CLIPFLAG_CLIP) {
							// make sure keys are created
							SuspendAnimate();
							AnimateOn();
						}

						GetNode()->SetNodeTM(timerange.Start(), val);

						if (flags&CLIPFLAG_CLIP) {
							AnimateOff();
							ResumeAnimate();
						}
					}
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
	pblock->EnableNotifications(TRUE);
	return ok && load->ok();
}

BOOL Hub::SaveClip(CATRigWriter* save, int flags, Interval timerange, int layerindex)
{
	int i;
	save->BeginGroup(idHub);

	save->Comment(GetNode()->GetName());

	// if we are saving out a pose then we can
	// just save out the world space matrix for this bone
	if (!(flags&CLIPFLAG_CLIP))
	{
		if (GetInSpine() != NULL)
		{
			// Get the local transformation, ignoring any effect from
			// the parenting.  The value will be re-applied the same way
			Matrix3 tmLocal(1);
			Point3 p3BaseScale(1, 1, 1);
			Interval iv = FOREVER;
			mLayerTrans->GetTransformation(timerange.Start(), tmLocal, tmLocal, iv, p3BaseScale, p3BaseScale);
			save->Write(idValMatrix3, tmLocal);
		}
		else
		{
			INode* pNode = GetNode();
			DbgAssert(pNode != NULL);
			if (pNode)
			{
				Matrix3 tm = pNode->GetNodeTM(GetCOREInterface()->GetTime());
				save->Write(idValMatrix3, tm);
			}
		}
	}
	else {
		save->BeginGroup(idGroupWeights);
		GetWeights()->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();

		if (layerDangleRatio) {
			save->BeginGroup(idDangleRatio);
			layerDangleRatio->SaveClip(save, flags, timerange, layerindex);
			save->EndGroup();
		}

		save->BeginGroup(idController);
		mLayerTrans->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}

	for (i = 0; i < GetNumLimbs(); i++) {
		LimbData2 *limb = GetLimb(i);
		if (limb) limb->SaveClip(save, flags, timerange, layerindex);
	}
	// sometimes we just want to save a clip for this hub, and not the whole Rig
	if (!(flags&CLIPFLAG_SKIP_SPINES)) {
		for (i = 0; i < GetNumSpines(); i++) {
			SpineData2* spine = GetSpine(i);
			if (spine) spine->SaveClip(save, flags, timerange, layerindex);
		}
	}
	for (i = 0; i < GetNumTails(); i++) {
		TailData2* tail = GetTail(i);
		if (tail) tail->SaveClip(save, flags, timerange, layerindex);
	}

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

//********************************************************************************************

void Hub::SetNode(INode* n)
{
	// If we have been deleted?
	if (n == NULL)
	{
		// Ensure we NULL the CATParent ptr to us
		CATParentTrans* pCATParentTrans = static_cast<CATParentTrans*>(GetCATParentTrans(false));
		if (pCATParentTrans != NULL && pCATParentTrans->GetRootHub() == this)
			pCATParentTrans->SetRootHub(NULL);
	}

	CATNodeControl::SetNode(n);
}

ICATParentTrans* Hub::FindCATParentTrans()
{
	ICATParentTrans* pParent = CATControl::FindCATParentTrans();
	if (pParent == NULL)
	{
		// Legacy pointer access (through CATParent)
		ECATParent* pParentObj = dynamic_cast<ECATParent*>(pblock->GetReferenceTarget(PB_CATPARENT));
		if (pParentObj != NULL)
			pParent = pParentObj->GetCATParentTrans();
	}

	return pParent;
}

CATNodeControl* Hub::FindParentCATNodeControl()
{
	// our parent is the last spine link
	SpineData2* pSpine = GetInSpine();
	if (pSpine != NULL)
	{
		int iNumSpineBones = pSpine->GetNumBones();
		if (iNumSpineBones > 0)
			return (CATNodeControl*)pSpine->GetSpineBone(iNumSpineBones - 1);
	}
	return NULL;
}

int Hub::NumChildCATNodeControls()
{
	int iTotalChildren = GetNumTails() + GetNumSpines() + GetNumLimbs();
	return iTotalChildren;
}

CATNodeControl* Hub::GetChildCATNodeControl(int i)
{
	// We return limbs first, because we are most likely to have them
	int iNumLimbs = GetNumLimbs();
	if (i < iNumLimbs)
	{
		LimbData2* pLimb = GetLimb(i);
		if (pLimb == NULL)
			return NULL;

		CATNodeControl* pCollarbone = pLimb->GetCollarbone();
		if (pCollarbone != NULL)
			return pCollarbone;
		return (CATNodeControl*)pLimb->GetBoneData(0);
	}
	i -= iNumLimbs;

	// Next is spines
	int iNumSpines = GetNumSpines();
	if (i < iNumSpines)
	{
		SpineData2* pSpine = GetSpine(i);
		if (pSpine != NULL)
			return (CATNodeControl*)pSpine->GetSpineBone(0);
		return NULL;
	}
	i -= iNumSpines;

	// Tails are relatively rare.
	int iNumTails = GetNumTails();
	if (i < iNumTails)
	{
		TailData2* pTail = GetTail(i);
		if (pTail != NULL)
		{
			if (pTail->GetNumBones() > 0)
				return pTail->GetBone(0);
		}

		return NULL;
	}

	return NULL;
}

BOOL Hub::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	int i;
	Hub* pastehub = (Hub*)pastectrl;

	BOOL firstpastedbone = FALSE;
	if (!(flags&PASTERIGFLAG_FIRST_PASTED_BONE)) {
		flags |= PASTERIGFLAG_FIRST_PASTED_BONE;
		firstpastedbone = TRUE;
	}

	//	CATNodeControl::PasteRig(pastectrl, flags);

	SpineData2* pInSpine = GetInSpine();
	if (pInSpine && pastehub->GetInSpine())
		pInSpine->PasteRig(pastehub->GetInSpine(), flags, scalefactor);

	ICATObject* iobj = GetICATObject();
	ICATObject* pasteiobj = ((CATNodeControl*)pastectrl)->GetICATObject();
	if (iobj && pasteiobj)	iobj->PasteRig(pasteiobj, flags, scalefactor);

	Matrix3 tmSetup = pastehub->GetSetupMatrix();
	// the goal of this code was to make sure that if you were pasting a hub into an existing rig
	// it would calculate a new tmSetup that would posiiton coorectly in front of the preceding hub
	// never finnished
//	if(pInSpine && pastehub->pInSpine && pastehub->pInSpine->GetHub() == pInSpine->GetBaseHub()){
//		TimeValue t = GetCOREInterface()->GetTime();
//		Matrix3 tmHubOffset =  pastehub->GettmBoneWorld() * Inverse(pastehub->pInSpine->GetBaseHub()->GettmBoneWorld());
//		tmSetup = (tmHubOffset * pInSpine->GetBaseHub()->GettmBoneWorld()) * Inverse(catparenttrans->GettmCATParent(t));
//		tmSetup.SetTrans((tmSetup.GetTrans() * catparenttrans->ModVec(Point3(1.0f, 1.0f, 0.0f))) / catparenttrans->GetCATUnits());
//
//	}else tmSetup = pastehub->GetSetupMatrix();

	if (flags&PASTERIGFLAG_MIRROR) {
		if (GetLengthAxis() == Z)	MirrorMatrix(tmSetup, kXAxis);
		else									MirrorMatrix(tmSetup, kZAxis);
	}
	tmSetup.SetTrans(tmSetup.GetTrans() * scalefactor);
	SetSetupMatrix(tmSetup);

	mLayerTrans->SetFlags(pastehub->mLayerTrans->GetFlags());


	/////////////////////////////////////////////////////////////////
	// just before we keep going down the hierarchy
	int numarbbones = GetNumArbBones();
	int pastenumarbbones = ((CATNodeControl*)pastectrl)->GetNumArbBones();
	if (numarbbones < pastenumarbbones) {
		for (int i = numarbbones; i < pastenumarbbones; i++)
			AddArbBone();
	}
	else {
		for (int i = pastenumarbbones; i < numarbbones; i++)
			RemoveArbBone(GetArbBone(pastenumarbbones));
	}
	for (i = GetNumArbBones() - 1; i >= 0; i--)
		GetArbBone(i)->PasteRig(pastehub->GetArbBone(i), flags, scalefactor);

	/////////////////////////////////////////////////////////////////
	int nNumPasteLimbs = pastehub->GetNumLimbs();
	int numlimbs = GetNumLimbs();
	for (i = numlimbs; i < nNumPasteLimbs; i++) {
		LimbData2* limb = AddLimb(TRUE, pastehub->GetLimb(i)->GetFlags());
		limb->SetLMR(pastehub->GetLimb(i)->GetLMR());
	}
	// Always remove the same limb id, as the limbs get shuffled back in the array
	for (i = nNumPasteLimbs; i < numlimbs; i++)
		RemoveLimb(nNumPasteLimbs);

	for (i = GetNumLimbs() - 1; i >= 0; i--)
		GetLimb(i)->PasteRig(pastehub->GetLimb(i), flags, scalefactor);

	/////////////////////////////////////////////////////////////////
	int nNumPasteTails = pastehub->GetNumTails();
	int numtails = GetNumTails();
	for (i = numtails; i < nNumPasteTails; i++)
		AddTail(DEFAULT_NUM_CAT_BONES, TRUE);
	for (i = nNumPasteTails; i < numtails; i++)
		RemoveTail(nNumPasteTails);
	for (i = GetNumTails() - 1; i >= 0; i--)
		GetTail(i)->PasteRig(pastehub->GetTail(i), flags, scalefactor);
	/////////////////////////////////////////////////////////////////

	if (firstpastedbone) {
		CATCharacterRemap remap;

		// Add Required Mapping
		remap.AddEntry(pastectrl->GetCATParentTrans(), GetCATParentTrans());
		remap.AddEntry(pastectrl->GetCATParentTrans()->GetNode(), GetCATParentTrans()->GetNode());
		remap.AddEntry(pastectrl->GetCATParent(), GetCATParent());

		BuildMapping(pastectrl, remap, FALSE);
		PasteERNNodes(pastehub, remap);
	}

	UpdateCATUnits();

	return TRUE;
}

void Hub::SetName(TSTR newname, BOOL quiet/*=FALSE*/)
{
	if (newname != GetName()) {
		if (quiet) DisableRefMsgs();
		pblock->SetValue(PB_NAME, 0, newname);
		if (quiet) EnableRefMsgs();
	};
}

void  Hub::SetLayerTransFlag(DWORD f, BOOL tf)
{
	BOOL newundo = FALSE;
	TimeValue t = GetCOREInterface()->GetTime();
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	if (f == CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT) {
		Matrix3 tmSetup = GetSetupMatrix();
		if (tf) {
			tmSetup.SetTrans(GetNodeTM(t).GetTrans() - GetCATParentTrans()->GettmCATParent(t).GetTrans());
			tmSetup.SetTrans(tmSetup.GetTrans() / GetCATUnits());
		}
		else {
			// TODO
			if (GetInSpine()) {
			}
		}
		SetSetupMatrix(tmSetup);
	}
	if (f == CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT) {
		Matrix3 tmSetup = GetSetupMatrix();
		Point3 pos = tmSetup.GetTrans();
		if (tf) {
			tmSetup = GetNodeTM(t) * Inverse(GetCATParentTrans()->GettmCATParent(GetCOREInterface()->GetTime()));
			tmSetup.SetTrans(pos);
		}
		else {
			// TODO
			if (GetInSpine()) {
			}
		}
		SetSetupMatrix(tmSetup);
	}
	if (GetLayerTrans()) GetLayerTrans()->SetFlag(f, tf);

	if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_HLD_HUBSETTING)); }
}

void Hub::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	ICATParentTrans* pCATParentTrans = GetCATParentTrans();
	DbgAssert(pCATParentTrans != NULL);
	if (pCATParentTrans == NULL)
		return;

	switch (ctxt) {
		// if we are merging, force the whole character to be merged
	case kSNCFileSave:	// not really susre when this case is used
	case kSNCFileMerge:
		pCATParentTrans->AddSystemNodes(nodes, ctxt);
		break;
	case kSNCClone:
		if (GetCATMode() == SETUPMODE) {
			if (GetInSpine())
				GetInSpine()->AddSystemNodes(nodes, ctxt);
			else pCATParentTrans->AddSystemNodes(nodes, ctxt);
		}
		else pCATParentTrans->AddSystemNodes(nodes, ctxt);
		break;
	case kSNCDelete:
		if (GetCATMode() == SETUPMODE) {
			if (GetInSpine())
				GetInSpine()->AddSystemNodes(nodes, ctxt);
			else AddSystemNodes(nodes, ctxt);
		}
		else pCATParentTrans->AddSystemNodes(nodes, ctxt);
		break;
	}
}

// For the procedural spine sub-objet selection
int Hub::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {
	if (ipbegin && (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO) && GetInSpine() && !GetInSpine()->GetSpineFK()) {

		Interval iv;
		GraphicsWindow *gw = vpt->getGW();	// This line is here because I don't know how to initialize
											// a *gw. I will change it in the next line
		gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world

		bbox.Init();
		float dCATUnits = GetCATUnits();

		////////
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

		// Draw the axis
		Point3 p1, p2;
		float length = 0.5;
		if (GetICATObject()) length = GetICATObject()->GetX() * dCATUnits * 0.3f;
		Matrix3 tm = m_tmInSpineTarget;

		Matrix3 tmX = tm;
		tmX.PreTranslate(Point3(-length / 2.0f, 0.0f, 0.0f)); p1 = tmX.GetTrans();
		tmX.PreTranslate(Point3(length, 0.0f, 0.0f));		p2 = tmX.GetTrans();
		dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

		Matrix3 tmY = tm;
		tmY.PreTranslate(Point3(0.0f, -length / 2.0f, 0.0f)); p1 = tmY.GetTrans();
		tmY.PreTranslate(Point3(0.0f, length, 0.0f));		p2 = tmY.GetTrans();
		dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

		Matrix3 tmZ = tm;
		tmZ.PreTranslate(Point3(0.0f, 0.0f, -length / 2.0f)); p1 = tmZ.GetTrans();
		tmZ.PreTranslate(Point3(0.0f, 0.0f, length));		p2 = tmZ.GetTrans();
		dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

		// Draw a line from the base hub to us.
		SpineData2* pSpine = GetInSpine();
		Hub* pHub = pSpine->GetBaseHub();
		if (pHub != NULL)
		{
			// The base of the system
			INode* pBaseNode = pHub->GetNode();
			DbgAssert(pBaseNode != NULL);
			Matrix3 tmBase = pSpine->GetBaseTransform();
			p1 = tmBase.GetTrans() * dCATUnits;
			p1 = pBaseNode->GetNodeTM(t).PointTransform(p1);

			p2 = tm.GetTrans();

			dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;
		}
	}

	return CATNodeControl::Display(t, inode, vpt, flags);
};
void Hub::GetWorldBoundBox(TimeValue, INode *, ViewExp*, Box3& box)
{
	box += bbox;
}

// user Properties.
// these will be used by the runtime libraries
void	Hub::UpdateUserProps() {

	CATNodeControl::UpdateUserProps();

	INode* pNode = GetNode();
	DbgAssert(pNode != NULL);

	pNode->SetUserPropInt(_T("CATProp_Hub"), true);

	pNode->SetUserPropInt(_T("CATProp_GetNumLimbs"), GetNumLimbs());
	TSTR key;
	for (int i = 0; i < GetNumLimbs(); i++) {
		// Generate a unique identifier for the limb.
		// we will borrow the node handle from the 1st bone in the limb
		key.printf(_T("CATProp_LimbAddress%i"), i);
		pNode->SetUserPropString(key, GetLimb(i)->GetBoneAddress());
	}

	if (GetInSpine()) {
		// Generate a unique identifier for the limb.
		// we will borrow the node handle from the 1st bone in the limb
		pNode->SetUserPropString(_T("CATProp_InSpineAddress"), GetInSpine()->GetBoneAddress());
	}

	// now we save the Local weights key times and values to the user properties
	WriteControlToUserProps(pNode, GetWeights()->GetSelectedLayer(), _T("LocalWeight"));

}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class HubPLCB : public PostLoadCallback {
protected:
	Hub* hub;

public:
	HubPLCB(Hub *pOwner) { hub = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(hub);
		return hub->GetFileSaveVersion();
	}

	// this PLCB has a hierpriority over the ClipRoots so that we get called first
	int Priority() { return 2; }

	void proc(ILoad *) {

		if (!hub || hub->TestAFlag(A_IS_DELETED) || !DependentIterator(hub).Next())
		{
			delete this;
			return;
		}

		if (GetFileSaveVersion() < CAT_VERSION_2500) {
			// TODO: Throw Dialog (we don't support this)
			DbgAssert("Unsupported File!!!");
		}

		if (GetFileSaveVersion() < CAT_VERSION_2447) {
			hub->SetCCFlag(HUBFLAG_INSPINE_RESTRICTS_MOVEMENT);
		}

		if (GetFileSaveVersion() < CAT3_VERSION_3015) {

		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define		HUBCHUNK_CATNODECONTROL		1
#define		HUBCHUNK_CATGROUP			2
#define		ARBBONECHUNK_NUMARBBONES			5
#define		ARBBONECHUNK_ARBBONES				6

IOResult Hub::Save(ISave *isave)
{
	//	DWORD nb, refID;
	IOResult res = IO_OK;

	isave->BeginChunk(HUBCHUNK_CATNODECONTROL);
	res = CATNodeControl::Save(isave);
	isave->EndChunk();

	return res;
}

IOResult Hub::Load(ILoad *iload)
{
	//	DWORD nb, refID;
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case HUBCHUNK_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new HubPLCB(this));
	return IO_OK;
}

void Hub::UpdateUI() {
	if (!ipbegin) return;
	if (flagsbegin == 0) {
		IParamMap2* pmap = pblock->GetMap();
		if (pmap) pmap->Invalidate();
		HubParamsCallback.Update();
	}
	CATNodeControl::UpdateUI();

}

FPInterfaceDesc* Hub::GetDescByID(Interface_ID id)
{
	if (id == HUB_INTERFACE_FP) return IHubFP::GetDesc();
	return CATNodeControl::GetDescByID(id);
}

BaseInterface* Hub::GetInterface(Interface_ID id)
{
	if (id == HUB_INTERFACE_FP)
		return static_cast<IHubFP*>(this);
	else
		return CATNodeControl::GetInterface(id);
}

LimbData2* Hub::GetLimb(int id)
{
	return (id < GetNumLimbs() && id >= 0) ? static_cast<LimbData2*>(pblock->GetReferenceTarget(PB_LIMB_TAB, 0, id)) : NULL;
}

CatAPI::ISpine* Hub::GetInISpine()
{
	return GetInSpine();
}

int Hub::GetNumSpines()
{
	return pblock->Count(PB_SPINE_TAB);
}

SpineData2* Hub::GetInSpine()
{
	return (SpineData2*)pblock->GetReferenceTarget(PB_INSPINE);
}

void Hub::SetInSpine(SpineData2* sp)
{
	pblock->SetValue(PB_INSPINE, 0, (ReferenceTarget*)sp);
}

void Hub::SetSpine(int id, SpineData2 *spine)
{
	pblock->SetValue(PB_SPINE_TAB, 0, spine, id);
}

SpineData2* Hub::GetSpine(int id)
{
	return (id < GetNumSpines() && id >= 0) ? (SpineData2*)pblock->GetReferenceTarget(PB_SPINE_TAB, 0, id) : NULL;
}

CatAPI::ISpine* Hub::GetISpine(int id)
{
	return GetSpine(id);;
}

int Hub::FindSpine(SpineData2 *spine)
{
	for (int i = 0; i < GetNumSpines(); i++) { if (GetSpine(i) == spine) return i; } return -1;
}
