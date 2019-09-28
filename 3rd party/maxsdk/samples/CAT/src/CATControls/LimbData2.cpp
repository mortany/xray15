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
	This controller is used to hold all limb related stuff
	like IKTargets. It is also responsible for creating/destroying limb
	bones, collarbones, palms and IKTargets
 **********************************************************************/

#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>

#include "math.h"
#include "iparamm2.h"
#include "notify.h"
#include "MacroRec.h"

#include "CATRigPresets.h"

 // CATRig Structure
#include "LimbData2.h"
#include "FootTrans2.h"
#include "IKTargController.h"
#include "ICATParent.h"
#include "CollarBoneTrans.h"
#include "BoneData.h"
#include "PalmTrans2.h"
#include "Hub.h"

// Layer System
#include "CATClipRoot.h"
#include "CATClipHierarchy.h"
#include "CATClipValue.h"
#include "CATClipWeights.h"
#include "CATMotionLayer.h"

// CATMotion controllers
#include "CATHierarchyRoot.h"
#include "CATHierarchyBranch2.h"
#include "CATMotionRot.h"
#include "CATMotionPlatform.h"
#include "LiftPlantMod.h"
#include "FootBend.h"
#include "CATGraph.h"
#include "CATMotionLimb.h"
#include "ease.h"

#include "CATCharacterRemap.h"
class LimbData2ClassDesc : public CATControlClassDesc
{
public:
	LimbData2ClassDesc()
	{
		AddInterface(ILimbFP::GetFnPubDesc());
		AddInterface(IBoneGroupManagerFP::GetFnPubDesc());
	}

	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		LimbData2* limbdata = new LimbData2(loading);
		return limbdata;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_LIMBDATA2); }
	Class_ID		ClassID() { return LIMBDATA2_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("LimbData2"); }	// returns fixed parsable name (scripter-visible name)
};

// SimCity - Add in a second class desc for LimbData.  This is to support loading of old files
// Old files will reference this ClassDesc to create instances of LimbData (under the old SClassID)
// The actual class of LimbData was irrelevant from CATs POV, and its methods were never called.
class LimbData2ClassDescLegacy : public LimbData2ClassDesc {
public:
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// Previous versions called this class a float controller
};

// Exported function to return the LimbData2 class description
static LimbData2ClassDesc LimbData2Desc;
ClassDesc2* GetLimbData2Desc() { return &LimbData2Desc; }

static LimbData2ClassDescLegacy LimbData2DescLegacy;
ClassDesc2* GetLimbData2DescLegacy() { return &LimbData2DescLegacy; }

class LimbMotionRollout : TimeChangeCallback
{
	HWND hWnd;
	LimbData2* limb;

	ICustButton		*btnCreateIKtarget;
	ICustButton		*btnMatchIKFK;
	ICustButton		*btnMoveIKtoPalm;
	ICustButton		*btnSelectIKTarg;

	ISliderControl *sldIKFK;
	ISliderControl *sldIKPos;
	ISliderControl *sldForceFeedback;

public:

	LimbMotionRollout() : hWnd(NULL), btnSelectIKTarg(NULL), btnMatchIKFK(NULL), btnMoveIKtoPalm(NULL)
	{
		limb = NULL;

		btnCreateIKtarget = NULL;

		sldIKFK = NULL;
		sldIKPos = NULL;
		sldForceFeedback = NULL;
	}

	HWND GetHWnd() { return hWnd; }
	LimbData2* GetLimb() { return limb; }

	void UpdateIKFK(TimeValue t)
	{
		float val;
		if (limb && limb->layerIKFKRatio)
		{
			Interval iv = FOREVER;
			limb->layerIKFKRatio->GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
			sldIKFK->SetValue(val, FALSE);
			sldIKFK->SetKeyBrackets(limb->layerIKFKRatio->IsKeyAtTime(t, 0));
		}
	}

	void UpdateLimbIKPos(TimeValue t)
	{
		float val;
		if (limb && limb->layerLimbIKPos)
		{
			Interval iv = FOREVER;
			limb->layerLimbIKPos->GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
			sldIKPos->SetValue(val, FALSE);
			sldIKPos->SetKeyBrackets(limb->layerLimbIKPos->IsKeyAtTime(t, 0));
		}
	}

	void UpdateLimbLayerRetargetting(TimeValue t)
	{
		float val;
		if (limb && limb->layerRetargeting)
		{
			Interval iv = FOREVER;
			limb->layerRetargeting->GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
			sldForceFeedback->SetValue(val, FALSE);
			sldForceFeedback->SetKeyBrackets(limb->layerRetargeting->IsKeyAtTime(t, 0));
		}
	}

	void TimeChanged(TimeValue t)
	{
		// Split out the following updates so we can update them
		// 1 at a time, this allows the limb to process undos
		// without throwing errors.
		UpdateIKFK(t);
		UpdateLimbIKPos(t);
		UpdateLimbLayerRetargetting(t);
		SET_CHECKED(hWnd, IDC_CHK_DISPLAY_FK_LIMB, limb->TestFlag(LIMBFLAG_DISPAY_FK_LIMB_IN_IK));
	}

	BOOL InitControls(HWND hWnd, LimbData2* ctrlLimbData2)
	{
		// We shouldn't double create this...
		DbgAssert(this->hWnd == NULL);

		this->hWnd = hWnd;
		limb = ctrlLimbData2;
		Interface *ip = GetCOREInterface();
		TimeValue t = ip->GetTime();

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		//////////////////////////////////////////////////
		// IK System Group

		btnCreateIKtarget = GetICustButton(GetDlgItem(hWnd, IDC_BTN_CREATEIKTARGET));
		btnCreateIKtarget->SetType(CBT_PUSH);
		btnCreateIKtarget->SetButtonDownNotify(TRUE);
		btnCreateIKtarget->SetTooltip(TRUE, GetString(IDS_TT_CREATEIKTGT));
		if (limb->GetIKTarget())	btnCreateIKtarget->Disable();

		btnMatchIKFK = GetICustButton(GetDlgItem(hWnd, IDC_BTN_KEYLIMB));
		btnMatchIKFK->SetType(CBT_PUSH);
		btnMatchIKFK->SetButtonDownNotify(TRUE);
		btnMatchIKFK->SetTooltip(TRUE, GetString(IDS_TT_KEYFK));

		btnMoveIKtoPalm = GetICustButton(GetDlgItem(hWnd, IDC_BTN_IKTOLIMBEND));
		btnMoveIKtoPalm->SetType(CBT_PUSH);
		btnMoveIKtoPalm->SetButtonDownNotify(TRUE);
		btnMoveIKtoPalm->SetTooltip(TRUE, GetString(IDS_TT_MOVEIKPALM));

		btnSelectIKTarg = GetICustButton(GetDlgItem(hWnd, IDC_BTN_SELECTIKTARGET));
		btnSelectIKTarg->SetType(CBT_PUSH);
		btnSelectIKTarg->SetButtonDownNotify(TRUE);
		if (limb->GetIKTarget() && limb->GetIKTarget()->Selected()) {
			btnSelectIKTarg->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSelectLastLimbBone"), GetString(IDS_TT_SELLASTBONE)));
			btnSelectIKTarg->SetText(catCfg.Get(_T("ButtonTexts"), _T("btnSelectLastLimbBone"), GetString(IDS_BTN_SELLIMBEND)));
		}
		else {
			btnSelectIKTarg->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSelectIKTarg"), GetString(IDS_TT_SELIK)));
			btnSelectIKTarg->SetText(catCfg.Get(_T("ButtonTexts"), _T("btnSelectIKTarget"), GetString(IDS_TT_SELIK)));
			if (!limb->GetIKTarget())	btnSelectIKTarg->Disable();
		}

		float dIKFK = 0.0f;
		Interval iv = FOREVER;
		DbgAssert(limb->layerIKFKRatio);
		limb->layerIKFKRatio->GetValue(t, (void*)&dIKFK, iv, CTRL_ABSOLUTE);
		sldIKFK = SetupFloatSlider(hWnd, IDC_SLIDER_IKFK2, IDC_EDIT_IKFK, 0.0f, 1.0f, dIKFK, 4);
		sldIKFK->SetKeyBrackets(limb->layerIKFKRatio->IsKeyAtTime(t, 0));

		float dIKPos = 0.0f;
		iv = FOREVER;
		DbgAssert(limb->layerLimbIKPos);
		limb->layerLimbIKPos->GetValue(t, (void*)&dIKPos, iv, CTRL_ABSOLUTE);
		float maxIKPos = limb->GetNumBones() + (limb->GetPalmTrans() ? 1.0f : 0.0f);
		sldIKPos = SetupFloatSlider(hWnd, IDC_SLIDER_IKPOS, IDC_EDIT_IKPOS, 0.0f, maxIKPos, dIKPos, (int)maxIKPos);
		sldIKPos->SetKeyBrackets(limb->layerLimbIKPos->IsKeyAtTime(t, 0));

		float dForceFeedback = 0.0f;
		iv = FOREVER;
		DbgAssert(limb->layerRetargeting);
		limb->layerRetargeting->GetValue(t, (void*)&dForceFeedback, iv, CTRL_ABSOLUTE);
		sldForceFeedback = SetupFloatSlider(hWnd, IDC_SLIDER_FORCEFEEDBACK, IDC_EDIT_FORCEFEEDBACK, 0.0f, 1.0f, dForceFeedback, 4);
		sldForceFeedback->SetKeyBrackets(limb->layerRetargeting->IsKeyAtTime(t, 0));

		// If we have no IK target, disable the appropriate sliders.
		if (limb->layerIKTargetTrans == NULL && limb->iktarget == NULL)
		{
			sldIKFK->Disable();
			sldIKPos->Disable();
			sldForceFeedback->Disable();
		}
		TimeChanged(GetCOREInterface()->GetTime());
		GetCOREInterface()->RegisterTimeChangeCallback(this);
		return TRUE;
	}

	void ReleaseControls()
	{
		GetCOREInterface()->UnRegisterTimeChangeCallback(this);
		GetCOREInterface()->ClearPickMode();

		hWnd = NULL;
		limb = NULL;

		SAFE_RELEASE_BTN(btnCreateIKtarget);
		SAFE_RELEASE_BTN(btnMatchIKFK);
		SAFE_RELEASE_BTN(btnSelectIKTarg);
		SAFE_RELEASE_BTN(btnMoveIKtoPalm);

		SAFE_RELEASE_SLIDER(sldIKFK);
		SAFE_RELEASE_SLIDER(sldIKPos);
		SAFE_RELEASE_SLIDER(sldForceFeedback);
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TimeValue t = GetCOREInterface()->GetTime();

		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (LimbData2*)lParam);
			break;
		case WM_DESTROY:
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_DISPLAY_FK_LIMB:
					limb->SetFlag(LIMBFLAG_DISPAY_FK_LIMB_IN_IK, IS_CHECKED(hWnd, IDC_CHK_DISPLAY_FK_LIMB));
					break;
				}
				break;
			case BN_BUTTONUP:
				switch (LOWORD(wParam))
				{
				case IDC_BTN_CREATEIKTARGET:
				{
					HoldActions actions(IDS_HLD_IKTARGCREATE);
					limb->CreateIKTarget();

					btnCreateIKtarget->Disable();
					sldForceFeedback->Enable();
					sldIKFK->Enable();
					sldIKPos->Enable();

					btnMoveIKtoPalm->Enable();
					btnSelectIKTarg->Enable();
					TimeChanged(GetCOREInterface()->GetTime());
					break;
				}

				case IDC_BTN_KEYLIMB:
				{
					HoldActions actions(IDS_HLD_IKFKMATCH);
					limb->MatchIKandFK(GetCOREInterface()->GetTime());
					break;
				}
				case IDC_BTN_SELECTIKTARGET:
				{
					// Initialise the INI files so we can read button text and tooltips
					CatDotIni catCfg;
					HoldActions actions(IDS_HLD_IKTARGSEL);
					if (limb->GetIKTarget() && limb->GetIKTarget()->Selected()) {
						limb->SelectLimbEnd();
						btnSelectIKTarg->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSelectIKTarg"), GetString(IDS_TT_SELIK)));
						btnSelectIKTarg->SetText(catCfg.Get(_T("ButtonTexts"), _T("btnSelectIKTarget"), GetString(IDS_TT_SELIK)));
					}
					else {
						limb->SelectIKTarget();
						btnSelectIKTarg->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSelectLastLimbBone"), GetString(IDS_TT_SELLASTBONE)));
						btnSelectIKTarg->SetText(catCfg.Get(_T("ButtonTexts"), _T("btnSelectLastLimbBone"), GetString(IDS_BTN_SELLIMBEND)));
						if (!limb->GetIKTarget())	btnSelectIKTarg->Disable();
					}
					break;
				}
				case IDC_BTN_IKTOLIMBEND:
					HoldActions actions(IDS_HLD_IKTARGMOVE);
					limb->MoveIKTargetToEndOfLimb(GetCOREInterface()->GetTime());
					break;
				}
				// Returning redraw views apparently doesn't really do anything...
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
				return REDRAW_VIEWS;

			}
			break;

		case CC_SLIDER_BUTTONDOWN:
			theHold.Begin();
			break;
		case CC_SLIDER_CHANGE: {
			//TODO Check undos
			BOOL newundo = FALSE;
			if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
			ISliderControl *ccSlider = (ISliderControl*)lParam;
			float val = ccSlider->GetFVal();
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SLIDER_IKFK2:			limb->SetIKFKRatio(t, val);			break;
			case IDC_SLIDER_IKPOS:			limb->SetLimbIKPos(t, val);			break;
			case IDC_SLIDER_FORCEFEEDBACK:	limb->SetForceFeedback(t, val);		break;
			}
			TimeChanged(t);
			if (newundo && theHold.Holding())
				theHold.Accept(GetString(IDS_HLD_CHANGE));
		}
		break;
		case CC_SLIDER_BUTTONUP:
		{
			if (HIWORD(wParam))
			{
				switch (LOWORD(wParam))
				{
				case IDC_SLIDER_IKFK2:			theHold.Accept(GetString(IDS_HLD_IKFKRATIO));		break;
				case IDC_SLIDER_IKPOS:			theHold.Accept(GetString(IDS_HLD_IKPOSRATIO));		break;
				case IDC_SLIDER_FORCEFEEDBACK:	theHold.Accept(GetString(IDS_HLD_FORCEFEEDBACK));		break;
				}
			}
			else	theHold.Cancel();
			break;
		}
		default:
			return FALSE;
		}
		return TRUE;
	}
};

static LimbMotionRollout staticLimbMotionRollout;

static INT_PTR CALLBACK LimbMotionRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticLimbMotionRollout.DlgProc(hWnd, message, wParam, lParam);
};

class LimbHierarchyDlgCallBack
{
private:
	HWND			hWnd;
	LimbData2		*limb;
public:

	LimbHierarchyDlgCallBack() {
		hWnd = NULL;
		limb = NULL;
	};
	BOOL IsDlgOpen() { return (limb != NULL); }
	HWND GetHWnd() { return hWnd; }

	virtual void InitControls(HWND hWnd, LimbData2 *owner) {
		this->hWnd = hWnd;
		limb = owner;
		DbgAssert(limb);

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		Update();
	};
	virtual void ReleaseControls(HWND) {
		limb = NULL;
	};
	virtual void Update() {
		if (limb->TestFlag(LIMBFLAG_FFB_WORLDZ))
			SET_CHECKED(hWnd, IDC_RDO_FF_WORLDZ, TRUE);
		else SET_CHECKED(hWnd, IDC_RDO_FF_LIMB_TO_IKTARGET, TRUE);
	};

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void DeleteThis() { }
};

INT_PTR CALLBACK LimbHierarchyDlgCallBack::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		InitControls(hWnd, (LimbData2*)lParam);
		break;
	case WM_DESTROY:
		ReleaseControls(hWnd);
		return FALSE;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			switch (LOWORD(wParam)) {
			case IDC_RDO_FF_LIMB_TO_IKTARGET:
				limb->ClearFlag(LIMBFLAG_FFB_WORLDZ);
				SET_CHECKED(hWnd, IDC_RDO_FF_WORLDZ, FALSE);
				break;
			case IDC_RDO_FF_WORLDZ:
				limb->SetFlag(LIMBFLAG_FFB_WORLDZ);
				SET_CHECKED(hWnd, IDC_RDO_FF_LIMB_TO_IKTARGET, FALSE);
				break;
			}
			break;
		}
		}
		break;

	case WM_NOTIFY:
		break;

	default:
		return FALSE;
	}
	return TRUE;
};

static LimbHierarchyDlgCallBack LimbHierarchyCallBack;

static INT_PTR CALLBACK LimbHierarchyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return LimbHierarchyCallBack.DlgProc(hWnd, message, wParam, lParam);
};

/* ------------------ CommandMode classes for node picking ----------------------- */

//**********************************************************************//
// LimbData Setup Rollout                                               //
//**********************************************************************//
class LimbSetupParamDlgCallBack : public  ParamMap2UserDlgProc {

	LimbData2* limb;
	//	ICustEdit *edtLimbName;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;

	ISpinnerControl *spnNumBones;
	ICustButton		*btnUpVecNode;
#ifdef SOFTIK
	ISpinnerControl *spnIKSoftLimitMin;
	ISpinnerControl *spnIKSoftLimitMax;
	ISpinnerControl *spnIKlength;
#endif
public:

	LimbSetupParamDlgCallBack() : limb(NULL), btnUpVecNode(NULL) {
		btnCopy = NULL;
		btnPaste = NULL;
		btnPasteMirrored = NULL;

		spnNumBones = NULL;
#ifdef SOFTIK
		spnIKSoftLimitMin = NULL;
		spnIKSoftLimitMax = NULL;
		spnIKlength = NULL;
#endif
	}
	void InitControls(HWND hWnd)
	{
		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		int iLMR = limb->GetLMR();
		switch (iLMR) {
		case -1:	SET_CHECKED(hWnd, IDC_RADIO_L, TRUE);	break;
		case 0:		SET_CHECKED(hWnd, IDC_RADIO_M, TRUE);	break;
		case 1:		SET_CHECKED(hWnd, IDC_RADIO_R, TRUE);	break;
		}

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hWnd, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyLimb"), GetString(IDS_TT_COPYLIMB)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteLimb"), GetString(IDS_TT_PASTELIMB)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		// btnPasteMirrored button
		btnPasteMirrored = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE_MIRRORED));
		btnPasteMirrored->SetType(CBT_PUSH);
		btnPasteMirrored->SetButtonDownNotify(TRUE);
		btnPasteMirrored->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteLimbMirrored"), GetString(IDS_TT_PASTELIMBMIRROR)));
		btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

		if (limb->GetCATParentTrans()->GetCATMode() != SETUPMODE) {
			btnCopy->Disable();
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}
		else if (!limb->CanPasteControl())
		{
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}

		spnNumBones = SetupIntSpinner(hWnd, IDC_SPIN_NUMBONES, IDC_EDIT_NUMBONES, 1, 20, limb->GetNumBones());

		if (!limb->GetisLeg())	SET_TEXT(hWnd, IDC_CHK_PALM_ANKLE, catCfg.Get(_T("ButtonTexts"), _T("chkPalm"), GetString(IDS_PALM)));
		else					SET_TEXT(hWnd, IDC_CHK_PALM_ANKLE, catCfg.Get(_T("ButtonTexts"), _T("chkAnkle"), GetString(IDS_ANKLE)));

		if (limb->GetPalmTrans())	SET_CHECKED(hWnd, IDC_CHK_PALM_ANKLE, TRUE);
		if (limb->GetCollarbone())	SET_CHECKED(hWnd, IDC_CHK_COLLARBONE, TRUE);

		btnUpVecNode = GetICustButton(GetDlgItem(hWnd, IDC_BTN_UPVECNODE));
		btnUpVecNode->SetType(CBT_CHECK);
		btnUpVecNode->SetCheck(limb->GetUpNode() ? TRUE : FALSE);
		btnUpVecNode->SetButtonDownNotify(TRUE);
		btnUpVecNode->SetText(GetString(IDS_USE_UP_NODE));
		btnUpVecNode->SetTooltip(TRUE, GetString(IDS_TT_PICKNODEUP));

#ifdef SOFTIK
		SET_CHECKED(hWnd, IDC_CHK_SOFTIK_LIMITS, limb->GetIKSoftLimitEnabled());
		SET_CHECKED(hWnd, IDC_CHK_LIMBSTRETCH, limb->GetStretchyLimbEnabled());
		SET_CHECKED(hWnd, IDC_CHK_RAMP_RETARGETTING, limb->GetRampReachEnabled());
		spnIKSoftLimitMin = SetupFloatSpinner(hWnd, IDC_SPIN_IKSOFTLIMIT_MIN, IDC_EDIT_IKSOFTLIMIT_MIN, 0.0f, 9999.0f, limb->GetIKSoftLimitMin());
		spnIKSoftLimitMax = SetupFloatSpinner(hWnd, IDC_SPIN_IKSOFTLIMIT_MAX, IDC_EDIT_IKSOFTLIMIT_MAX, 0.0f, 9999.0f, limb->GetIKSoftLimitMax());

		spnIKlength = SetupFloatSpinner(hWnd, IDC_SPIN_IKLENGTH, IDC_EDIT_IKLENGTH, 0.0f, 9999.0f, 0.0f);
#endif
	}
	void ReleaseControls()
	{
		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnPasteMirrored);
		SAFE_RELEASE_BTN(btnUpVecNode);
		SAFE_RELEASE_SPIN(spnNumBones);
	}
	void Update(TimeValue, Interval&, IParamMap2*)
	{
#ifdef SOFTIK
		spnIKlength->SetValue(limb->GetDistToTarg(t), FALSE);
		spnIKSoftLimitMin->SetValue(limb->GetIKSoftLimitMin(), FALSE);
		spnIKSoftLimitMax->SetValue(limb->GetIKSoftLimitMax(), FALSE);
#endif
	}

	virtual INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		limb = (LimbData2*)map->GetParamBlock()->GetOwner();
		if (!limb) return FALSE;

		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd);
			break;

		case WM_DESTROY:
			ReleaseControls();
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_RADIO_L:
					SET_CHECKED(hWnd, IDC_RADIO_M, FALSE);
					SET_CHECKED(hWnd, IDC_RADIO_R, FALSE);
					limb->SetLMR(-1);
					break;
				case IDC_RADIO_M:
					SET_CHECKED(hWnd, IDC_RADIO_L, FALSE);
					SET_CHECKED(hWnd, IDC_RADIO_R, FALSE);
					limb->SetLMR(0);
					break;
				case IDC_RADIO_R:
					SET_CHECKED(hWnd, IDC_RADIO_L, FALSE);
					SET_CHECKED(hWnd, IDC_RADIO_M, FALSE);
					limb->SetLMR(1);
					break;
				case IDC_CHK_COLLARBONE:
					if (IS_CHECKED(hWnd, IDC_CHK_COLLARBONE))
					{
						DbgAssert(!theHold.Holding());
						theHold.Begin();
						limb->CreateCollarbone();
						theHold.Accept(GetString(IDS_HLD_CLRBONEADD));
					}
					else
					{
						DbgAssert(!theHold.Holding());
						theHold.Begin();
						limb->RemoveCollarbone();
						theHold.Accept(GetString(IDS_HLD_CLRBONEREM));
					}
					break;
				case IDC_CHK_PALM_ANKLE:
					if (IS_CHECKED(hWnd, IDC_CHK_PALM_ANKLE))	limb->CreatePalmAnkle();
					else										limb->RemovePalmAnkle();
					break;
				case IDC_BTN_UPVECNODE:			if (btnUpVecNode->IsChecked()) limb->CreateUpNode(); else limb->RemoveUpNode();	break;
#ifdef SOFTIK
				case IDC_CHK_SOFTIK_LIMITS:		limb->SetIKSoftLimitEnabled(IS_CHECKED(hWnd, IDC_CHK_SOFTIK_LIMITS));			break;
				case IDC_CHK_LIMBSTRETCH:		limb->SetStretchyLimbEnabled(IS_CHECKED(hWnd, IDC_CHK_LIMBSTRETCH));			break;
				case IDC_CHK_RAMP_RETARGETTING:	limb->SetRampReachEnabled(IS_CHECKED(hWnd, IDC_CHK_RAMP_RETARGETTING));			break;
#endif
				}
				break;
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_COPY:				CATControl::SetPasteControl(limb);													break;
				case IDC_BTN_PASTE:					limb->PasteFromCtrl(CATControl::GetPasteControl(), false);			break;
				case IDC_BTN_PASTE_MIRRORED:		limb->PasteFromCtrl(CATControl::GetPasteControl(), true);				break;
				}
				break;
				break;// WM_COMMAND
			}
		case CC_SPINNER_BUTTONDOWN: {
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_NUMBONES:
				if (!theHold.Holding()) theHold.Begin();
				break;
			}
			break;
		}
		case CC_SPINNER_CHANGE:
			if (theHold.Holding()) theHold.Restore();
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_NUMBONES:			limb->SetNumBones(spnNumBones->GetIVal(), FALSE);				break;
#ifdef SOFTIK
			case IDC_SPIN_IKSOFTLIMIT_MIN:	limb->SetIKSoftLimitMin(spnIKSoftLimitMin->GetFVal());	break;
			case IDC_SPIN_IKSOFTLIMIT_MAX:	limb->SetIKSoftLimitMax(spnIKSoftLimitMax->GetFVal());	break;
#endif
			}
			GetCOREInterface()->RedrawViews(t);
			break;
		case CC_SPINNER_BUTTONUP:
			if (theHold.Holding()) {
				if (HIWORD(wParam)) {
					switch (LOWORD(wParam)) { // Switch on ID
					case IDC_SPIN_NUMBONES:			theHold.Accept(GetString(IDS_HLD_NUMBONES));				break;
#ifdef SOFTIK
					case IDC_SPIN_IKSOFTLIMIT_MIN:	theHold.Accept(_T("Soft IK Min Limit Changed"));	break;
					case IDC_SPIN_IKSOFTLIMIT_MAX:	theHold.Accept(_T("Soft IK Max Limit Changed"));	break;
#endif
					}
				}
				else theHold.Cancel();
			}
			break;
		case WM_CUSTEDIT_ENTER:
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_NUMBONES:			limb->SetNumBones(spnNumBones->GetIVal(), FALSE);			break;
#ifdef SOFTIK
			case IDC_EDIT_IKSOFTLIMIT_MIN:	limb->SetIKSoftLimitMin(spnIKSoftLimitMin->GetFVal());	break;
			case IDC_EDIT_IKSOFTLIMIT_MAX:	limb->SetIKSoftLimitMax(spnIKSoftLimitMax->GetFVal());	break;
#endif
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }//delete this; }
};

// Dumb-ass check no 1
class LimbParamAccessor : PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker *owner, ParamID id, int tabIndex)
	{
		UNREFERENCED_PARAMETER(owner); UNREFERENCED_PARAMETER(tabIndex);
		switch (id)
		{
		case LimbData2::PB_NUMBONES:
			v.i = max(v.i, 1);
			break;
		default:
			break;
		}
	}
};

static LimbSetupParamDlgCallBack limbSetupParamDlgCallBack;

// I think we need two pblocks, one public that everyone can see with
// IKFK ratios, and IKTarget, and Name and colour.
// and one with all the info that the user doesn't need to see.
// we just don't return is as a subanim
static ParamBlockDesc2 LimbData2_t_param_blk(LimbData2::pb_idParams, _T("LimbData2Params"), 0, &LimbData2Desc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, LimbData2::LIMBDATAPB_REF,
	IDD_LIMBDATA_SETUP_CAT3, IDS_LIMBPARAMS, 0, 0, &limbSetupParamDlgCallBack,

	////////////////////////////////////////////////////////////////////////////////////////////
	// params
	LimbData2::PB_CATPARENT, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	LimbData2::PB_HUB, _T("Hub"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	LimbData2::PB_CATHIERARCHY, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	LimbData2::PB_LAYERHIERARCHY, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,

	LimbData2::PB_COLOUR, _T("LimbColour"), TYPE_RGBA, P_RESET_DEFAULT, NULL,
		p_ui, TYPE_COLORSWATCH, IDC_SWATCH_COLOR,
		p_default, Point3(0.0f, 0.5f, 1.0f),
		p_end,
	LimbData2::PB_NAME, _T(""), TYPE_STRING, P_RESET_DEFAULT, NULL,
		p_default, _T("Limb"),
		p_ui, TYPE_EDITBOX, IDC_EDIT_NAME,
		p_end,
	LimbData2::PB_LIMBID_DEPRECATED, _T("LimbID"), TYPE_INDEX, P_OBSOLETE, NULL,
		p_default, -1,
		p_end,
	LimbData2::PB_LIMBFLAGS, _T(""), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 0,
		p_end,

	LimbData2::PB_CTRLCOLLARBONE, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	LimbData2::PB_CTRLPALM, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,

	LimbData2::PB_P3ROOTPOS, _T(""), TYPE_POINT3, P_RESET_DEFAULT, 0,
		p_end,
	/////////////////////////// All our pointers to nodes and references ///////////////////////////
	LimbData2::PB_NUMBONES, _T("NumBones"), TYPE_INT, P_RESET_DEFAULT, IDS_NUMBONES,
		p_default, 0,
		p_end,
	// The BoneDatas are float controllers, and they maintain the nodes
	LimbData2::PB_BONEDATATAB, _T("Bones"), TYPE_REFTARG_TAB, 0, P_NO_REF + P_VARIABLE_SIZE, IDS_BONES,
		p_end,

	// This 'may' only be used in mocap where
	// we have a symmetrical character and we want to force KA's

	// Now the palm will maintain the platform. The palm and platform will be tied closely together

	////////////////////////////////////////////////////////////////////////////////////////////
	LimbData2::PB_LEGWEIGHT, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_end,
	// These all need to be layerControllers
	LimbData2::PB_LAYERBENDANGLE, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_end,
	LimbData2::PB_LAYERSWIVEL, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_end,
	////////////////////////////////////////////////////////////////////////////////////////////
	// A limb scale that is used on every seg.
	// No scale controllers needed on each segment
	LimbData2::PB_LIMBSCALE, _T(""), TYPE_POINT3, P_ANIMATABLE | P_RESET_DEFAULT, 0,
		p_default, Point3(1.0f, 1.0f, 1.0f),
		p_end,
	// aNiMaTeD bouncey/wouncey scale.
	// this gets passed to the pelvis/Ribcage and is interpolated up the spine
	LimbData2::PB_CATSCALE, _T(""), TYPE_POINT3, P_ANIMATABLE | P_RESET_DEFAULT, 0,
		p_default, Point3(1.0f, 1.0f, 1.0f),
		p_end,
	////////////////////////////////////////////////////////////////////////////////////////////
	LimbData2::PB_PHASEOFFSET, _T(""), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, 0,
		p_end,
	// These all need to be layerControllers
	LimbData2::PB_LIFTPLANTMOD, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5f,
		p_end,

	// Left = -1; Middle = 0; Right = 1;
	// LMR
	LimbData2::PB_SYMLIMB_SETUP, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	LimbData2::PB_SYMLIMBID, _T(""), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, -1,
		p_end,
	LimbData2::PB_SYMLIMB_MOTION, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,

	////////////////////////////////////////////////////////////////////////////////////////////
	LimbData2::PB_SETUPTARGETALIGN, _T(""), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_end,

	#ifdef SOFTIK
	LimbData2::PB_SOFTIKWEIGHT, _T("SoftIKWeight"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_end,
	#endif
	p_end
);

BaseInterface* LimbData2::GetInterface(Interface_ID id)
{
	if (id == I_CATGROUP) return static_cast<CATGroup*>(this);
	if (id == LIMB_INTERFACE_FP) return static_cast<ILimbFP*>(this);
	if (id == BONEGROUPMANAGER_INTERFACE_FP) return static_cast<IBoneGroupManagerFP*>(this);
	else return CATControl::GetInterface(id);
}

FPInterfaceDesc* LimbData2::GetDescByID(Interface_ID id) {
	if (id == LIMB_INTERFACE_FP) return ILimbFP::GetDesc();
	return CATControl::GetDescByID(id);
}

/*
 * End published functions.
 ********************************************************************/

LimbData2::LimbData2(BOOL loading)
	: mStretch(0.0f)
{
	///////////////////////////////////////////////////////////////////////////
	// References
	pblock = NULL;
	layerRetargeting = NULL;
	layerIKFKRatio = NULL;
	layerLimbIKPos = NULL;
	layerIKTargetTrans = NULL;
	weights = NULL;
	iktarget = NULL;

	//////////////////////////////////////////////////////////////////////////
	flags = 0;

	mp3LimbRoot = Point3::Origin;
	mp3PalmLocalVec = Point3::Origin;

	mtmIKTarget = mtmFKTarget = mFKTargetLocalTM = mFKPalmLocalTM = Matrix3(1);
	mFKTargetValid = mIKSystemValid = NEVER;

	//////////////////////////////////////////////////////////////////////////

	upvectornode = NULL;

#ifdef SOFTIK
	iksoftlimit_min = iksoftlimit_max = 99999.0f;
#endif

	// Always create new parameter block
	LimbData2Desc.MakeAutoParamBlocks(this);

	if (!loading) pblock->ResetAll(TRUE, FALSE);
}

LimbData2::~LimbData2()
{
	DeleteAllRefs();
}

SClass_ID LimbData2::SuperClassID()
{
	return LimbData2Desc.SuperClassID();
}

void LimbData2::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	if (ctxt == kSNCDelete) {
		// The LOCK flag will stop us destroying our rollouts, so we need to do that now
		if (ipbegin) {
			EndEditParams(ipbegin, END_EDIT_REMOVEUI, NULL);
		}
		SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, TRUE);
	}

	CATControl::AddSystemNodes(nodes, ctxt);
}

static void LimbBoneCloneNotify(void *param, NotifyInfo *info)
{
	LimbData2 *limb = (LimbData2*)param;
	INode *node = (INode*)info->callParam;
	if (limb->GetBoneData(0)->GetNode() == node) {
		Hub *hub = limb->GetHub();
		if (hub->GetLimb(limb->GetLimbID()) != limb) {

			limb->SetLimbID(hub->GetNumLimbs());
			hub->SetLimb(limb->GetLimbID(), limb);
		}
		UnRegisterNotification(LimbBoneCloneNotify, limb, NOTIFY_NODE_CLONED);
	}
}

void LimbData2::PostCloneManager()
{
	// if we have just been cloned, ensure we are hooked
	// into our Hub. (TODO: Move this into the PostPatch below)
	RegisterNotification(LimbBoneCloneNotify, this, NOTIFY_NODE_CLONED);
};

class PostPatchLimbDataClone : public PostPatchProc
{
private:
	LimbData2* mpClonedOwner;

	INode* mpOrigUpVector;
	INode* mpOrigIKTarget;

public:

	PostPatchLimbDataClone(LimbData2* pClonedOwner, INode* pIKTarget, INode* pUpVector)
		: mpClonedOwner(pClonedOwner)
		, mpOrigIKTarget(pIKTarget)
		, mpOrigUpVector(pUpVector)
	{
		DbgAssert(pClonedOwner != NULL);
	}

	~PostPatchLimbDataClone() {};

	// The proc needs to
	int Proc(RemapDir& remap)
	{
		mpClonedOwner->ReplaceReference(LimbData2::REF_IKTARGET, remap.FindMapping(mpOrigIKTarget));

		mpClonedOwner->ReplaceReference(LimbData2::REF_UPVECTORNODE, remap.FindMapping(mpOrigUpVector));

		return TRUE;
	}
};

RefTargetHandle LimbData2::Clone(RemapDir& remap)
{
	// make a new LimbData2 object to be the clone
	LimbData2 *newLimbData2 = new LimbData2();
	// We add this entry now so that we don't get cloned again by anyone cloning a reference to us
	remap.AddEntry(this, newLimbData2);

	BlockEvaluation block(newLimbData2);

	if (pblock)				newLimbData2->ReplaceReference(LIMBDATAPB_REF, CloneParamBlock(pblock, remap));
	if (weights)				newLimbData2->ReplaceReference(REF_WEIGHTS, remap.CloneRef(weights));
	if (layerRetargeting)	newLimbData2->ReplaceReference(REF_LAYERRETARGETING, remap.CloneRef(layerRetargeting));
	if (layerIKFKRatio)		newLimbData2->ReplaceReference(REF_LAYERIKFKRATIO, remap.CloneRef(layerIKFKRatio));
	if (layerLimbIKPos)		newLimbData2->ReplaceReference(REF_LAYERLIMBIKPOS, remap.CloneRef(layerLimbIKPos));
	if (layerIKTargetTrans)	newLimbData2->ReplaceReference(REF_LAYERIKTARGETRANS, remap.CloneRef(layerIKTargetTrans));

	// Cloning nodes is a little more difficult.
	// A node cannot be cloned by us, so we cannot use
	// the CloneRef function. We need to create a reference
	// to the node, and cannot use Max re-mapping ability
	// so use a post-patch proc to handle them.
	remap.AddPostPatchProc(new PostPatchLimbDataClone(newLimbData2, iktarget, upvectornode), true);

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATControl(newLimbData2, remap);

	// Clone all the LimbStuff
	newLimbData2->flags = flags;

	// If our hub was cloned, set the new pointer,
	// otherwise we are just cloning the limb
	Hub* pNewHub = static_cast<Hub*>(remap.FindMapping(GetHub()));
	int id = GetLimbID();
	if (pNewHub == NULL)
	{
		// If we are cloning on to the original hub, we need
		// to find a new LimbId (because hte original
		// hub still has the pointer to this LimbData
		pNewHub = GetHub();
		// Add limb to hub
		while (pNewHub->GetLimb(id) != NULL)
			id++;
	}

	DbgAssert(pNewHub != NULL);

	// Make connections.
	// NOTE: Do NOT set the limb on the hub.
	// While we are mid-clone, we CANNOT modify
	// existing classes.  theHold is suspended,
	// and any changes will not be undone in case
	// of a cancel/undo.
	newLimbData2->SetHub(pNewHub);
	newLimbData2->SetLimbID(id);

	// Clone collarbone
	CATNodeControl* collarbone = GetCollarbone();
	if (collarbone) {
		CollarBoneTrans* pNewCollarbone = (CollarBoneTrans*)remap.CloneRef(collarbone);
		newLimbData2->pblock->SetValue(PB_CTRLCOLLARBONE, 0, pNewCollarbone);
		pNewCollarbone->SetLimbData(newLimbData2);
	}
	for (int i = 0; i < GetNumBones(); i++)
	{
		newLimbData2->pblock->SetValue(PB_BONEDATATAB, 0, remap.CloneRef(GetBoneData(i)), i);
		newLimbData2->GetBoneData(i)->SetLimbData(newLimbData2);
	}
	if (GetPalmTrans()) {
		PalmTrans2* palm = (PalmTrans2*)remap.CloneRef(GetPalmTrans());
		newLimbData2->pblock->SetValue(PB_CTRLPALM, 0, palm);
		palm->SetLimb(newLimbData2);
	}

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newLimbData2, remap);

	// now return the new object.
	return newLimbData2;
}

ReferenceTarget* LimbData2::GetReference(int i)
{
	switch (i)
	{
	case LIMBDATAPB_REF:			return pblock;
	case REF_LAYERRETARGETING:		return layerRetargeting;
	case REF_LAYERIKFKRATIO:		return layerIKFKRatio;
	case REF_LAYERLIMBIKPOS:		return layerLimbIKPos;
	case REF_IKTARGET:				return iktarget;
	case REF_WEIGHTS:				return (Control*)weights;
	case REF_LAYERIKTARGETRANS:		return layerIKTargetTrans;
	case REF_UPVECTORNODE:			return upvectornode;
	default:						return NULL;
	}
}

// A small helper function sets the Limb pointer
// on the nodes that don't reference us
void SetLimbPointer(INode* pANode, LimbData2* pSetLimb)
{
	if (pANode == NULL)
		return;

	Control* pMtxCtrl = pANode->GetTMController();
	if (pMtxCtrl != NULL)
	{
		Class_ID cid = pMtxCtrl->ClassID();
		if (cid == IKTARGTRANS_CLASS_ID)
		{
			IKTargTrans* pTargTrans = static_cast<IKTargTrans*>(pMtxCtrl);
			pTargTrans->SetLimbData(pSetLimb);
		}
		else if (cid == FOOTTRANS2_CLASS_ID)
		{
			FootTrans2* pFootTrans = static_cast<FootTrans2*>(pMtxCtrl);
			pFootTrans->SetLimbData(pSetLimb);
		}
		else
		{
			DbgAssert(FALSE && "FALSE: WTF is this?");
		}
	}
}

void LimbData2::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i)
	{
	case LIMBDATAPB_REF:			pblock = (IParamBlock2*)rtarg;					break;
	case REF_LAYERRETARGETING:		layerRetargeting = (CATClipValue*)rtarg;		break;
	case REF_LAYERIKFKRATIO:		layerIKFKRatio = (CATClipValue*)rtarg;			break;
	case REF_LAYERLIMBIKPOS:		layerLimbIKPos = (CATClipValue*)rtarg;			break;
	case REF_IKTARGET:
	{
		// If we are removing our reference to our IKTarget, NULL the pointer it holds to us.
		if (iktarget != NULL)
			SetLimbPointer(iktarget, NULL);
		iktarget = static_cast<INode*>(rtarg);
		// If we are adding a reference to a new IKTarget, add the pointer to us.
		// This is mostly to ensure that connections are maintained
		// when cloning things
		if (iktarget != NULL)
			SetLimbPointer(iktarget, this);
		break;
	}
	case REF_WEIGHTS:				weights = (CATClipWeights*)rtarg;				break;
	case REF_LAYERIKTARGETRANS:		layerIKTargetTrans = (CATClipMatrix3*)rtarg;	break;
	case REF_UPVECTORNODE:
	{
		// If we are removing our reference to our IKTarget, NULL the pointer it holds to us.
		if (upvectornode != NULL)
			SetLimbPointer(upvectornode, NULL);
		upvectornode = static_cast<INode*>(rtarg);
		// If we are adding a reference to a new IKTarget, add the pointer to us.
		// This is mostly to ensure that connections are maintained
		// when cloning things
		if (upvectornode != NULL)
			SetLimbPointer(upvectornode, this);
		break;
	}
	}
}

Animatable* LimbData2::SubAnim(int i)
{
	switch (i)
	{
	case SUB_WEIGHTS:				return weights;
	case SUB_LAYERIKFKRATIO:		return layerIKFKRatio;
	case SUB_LAYERLIMBIKPOS:		return layerLimbIKPos;
	case SUB_LAYERFORCEFEEDBACK:	return layerRetargeting;
	case SUB_LAYERIKTRANS:			return layerIKTargetTrans;
	default:						return NULL;
	}
}

TSTR LimbData2::SubAnimName(int i)
{
	switch (i)
	{
	case SUB_WEIGHTS:				return GetString(IDS_WEIGHTS);
	case SUB_LAYERIKFKRATIO:		return GetString(IDS_LYR_IKFKRATIO);
	case SUB_LAYERLIMBIKPOS:		return GetString(IDS_LYR_LIMBIKPOS);
	case SUB_LAYERFORCEFEEDBACK:	return GetString(IDS_LYR_RETARG);
	case SUB_LAYERIKTRANS:			return GetString(IDS_LYR_IKTARGTRANS);
	default:						return _T("");
	}
}

float LimbData2::GetIKFKRatio(TimeValue t, Interval& valid, int boneid/*=-1*/)
{
	if (TestFlag(LIMBFLAG_LOCKED_FK))
		return 1.0f;
	if (TestFlag(LIMBFLAG_LOCKED_IK))
		return 0.0f;

	// If we have no IK data, force FK.
	if (layerIKTargetTrans == NULL && iktarget == NULL)
		return 1.0f;

	float ikfkratio = 0.0f;
	if (layerIKFKRatio) {
		layerIKFKRatio->GetValue(t, (void*)&ikfkratio, valid, CTRL_ABSOLUTE);
	}
	ikfkratio = min(1.0f, max(ikfkratio, 0.0f));
	// return unmodified ikfkratio
	if (boneid < 0) return ikfkratio;
	//////////////////////////////////////////////////////////
	// Given a bone ID, is that bone in IK or FK.
	int numbones = GetNumBones() + (GetPalmTrans() ? 1 : 0);
	if (boneid < 0)
		boneid = numbones - 1;

	float ikpos = GetLimbIKPos(t, valid);

	if ((boneid + 1.0f) <= ikpos) return ikfkratio;
	if ((boneid) >= ikpos) return 1.0f;

	ikpos -= boneid;
	ikfkratio = BlendFloat(ikfkratio, 1.0f, 1.0f - ikpos);
	//	ikfkratio *= (1.0f - ikpos);
		//////////////////////////////////////////////////////////

	return ikfkratio;
}

void LimbData2::SetIKFKRatio(TimeValue t, float val) {
	val = min(1.0f, max(val, 0.0f));

	if (layerIKFKRatio) {
		// I could not stop max from creating keyframes on the ikfk layer controller when the
		// rollout got refreshed. I could not figure out why this issue only effected the ikfk slider,
		// so I put this little test in here to stop keys being automaticly create when someone selectes a
		// limb bone with the motion panel open.
		float ikfkratio = 0.0f;
		Interval iv = FOREVER;
		layerIKFKRatio->GetValue(t, (void*)&ikfkratio, iv, CTRL_ABSOLUTE);
		if (fabs(ikfkratio - val) > 0.001) {
			layerIKFKRatio->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
		}
	}
}

float LimbData2::GetForceFeedback(TimeValue t, Interval& valid) {
	if (TestFlag(LIMBFLAG_LOCKED_IK) || GetCATMode() == SETUPMODE) return 0.0f;

	float ffeedback = 0.0f;
	if (layerRetargeting) layerRetargeting->GetValue(t, (void*)&ffeedback, valid, CTRL_ABSOLUTE);

	ffeedback = min(1.0f, max(ffeedback, 0.0f));
	return ffeedback;
}
void LimbData2::SetForceFeedback(TimeValue t, float val) {
	val = min(1.0f, max(val, 0.0f));
	if (layerRetargeting) layerRetargeting->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
}

float LimbData2::GetLimbIKPos(TimeValue t, Interval &valid) {
	float ikposratio = 0.0f;
	if (layerLimbIKPos) layerLimbIKPos->GetValue(t, (void*)&ikposratio, valid, CTRL_ABSOLUTE);

	// limit low
	ikposratio = max(ikposratio, 0.0f);
	// Limit high
	if (GetPalmTrans() != NULL)
		ikposratio = min(ikposratio, GetNumBones() + 1);
	else
		ikposratio = min(ikposratio, GetNumBones());
	return ikposratio;
}

void LimbData2::SetLimbIKPos(TimeValue t, float val) {
	val = max(val, 0.0f);
	if (layerLimbIKPos) layerLimbIKPos->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
}

INode* LimbData2::CreateIKTarget(BOOL loading)
{
	if (iktarget)
		return iktarget;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }

	if (!GetisLeg()) {
		// If we are a brand new character, we probably don't have a controller for the IKTarget yet
		// We should make one now.
		BOOL bIsNewTarget = false;
		if (!layerIKTargetTrans) {
			bIsNewTarget = true;
			CATClipValue* layerIKTrans = CreateClipValueController(GetCATClipMatrix3Desc(), GetWeights(), GetCATParentTrans(), loading);
			layerIKTrans->SetFlag(CLIP_FLAG_HAS_TRANSFORM);
			ReplaceReference(REF_LAYERIKTARGETRANS, layerIKTrans);
		}
		DbgAssert(layerIKTargetTrans);
		IKTargTrans* ikcontroller = (IKTargTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, IKTARGTRANS_CLASS_ID);
		INode* nodeIKTarget = ikcontroller->Initialise(this, layerIKTargetTrans, idIKTargetValues, loading);
		ReplaceReference(REF_IKTARGET, nodeIKTarget);
		nodeIKTarget->InvalidateTreeTM();

		// If its a new target, move it to the appropriate place (end of the limb)
		if (bIsNewTarget && !loading)
		{
			// Just in case the user has set the IK
			// ratio to 0 before creating the IK target,
			// lock the ratio to FK.
			SetFlag(LIMBFLAG_LOCKED_FK);
			MoveIKTargetToEndOfLimb(0);
			ClearFlag(LIMBFLAG_LOCKED_FK);
		}
	}
	else {
		// Create the IK Target controller
		FootTrans2* foot = (FootTrans2*)CreateInstance(CTRL_MATRIX3_CLASS_ID, FOOTTRANS2_CLASS_ID);
		INode* nodeIKTarget = foot->Initialise(this, loading);
		ReplaceReference(REF_IKTARGET, nodeIKTarget);
	}

	if (newundo && theHold.Holding()) theHold.Accept(GetString(IDS_HLD_IKTARGCREATE));

	return iktarget;
}

INode* LimbData2::CreateIKTarget()
{
	return CreateIKTarget(FALSE);
}

BOOL LimbData2::RemoveIKTarget()
{
	if (!iktarget)
		return true;

	// start at the leaves and work up
	if (!theHold.Holding())
		theHold.Begin();

	GetCOREInterface()->DisableSceneRedraw();
	{
		MaxReferenceMsgLock lockThis;
		iktarget->Delete(0, FALSE);
		DeleteReference(REF_IKTARGET);
	}

	theHold.Accept(GetString(IDS_HLD_IKTARGREM));

	UpdateLimb();
	GetCOREInterface()->EnableSceneRedraw();
	return true;
}

INode* LimbData2::CreateUpNode(BOOL loading)
{
	if (upvectornode)
		return upvectornode;

	HoldActions hold(IDS_HLD_UPNODECREATE);
	BlockEvaluation block(this);

	CATClipMatrix3* layerUpNodeTrans = (CATClipMatrix3*)CreateClipValueController(GetCATClipMatrix3Desc(), GetWeights(), GetCATParentTrans(), loading);
	DbgAssert(layerUpNodeTrans);
	IKTargTrans* ikcontroller = (IKTargTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, IKTARGTRANS_CLASS_ID);
	INode* nodeUpnode = ikcontroller->Initialise(this, layerUpNodeTrans, idUpVectorValues, loading);
	INode* nodeHub = GetHub()->GetNode();
	nodeHub->AttachChild(nodeUpnode, false);
	ikcontroller->SetName(GetString(IDS_UPVECNODE));

	ReplaceReference(REF_UPVECTORNODE, nodeUpnode);

	TimeValue t = GetCOREInterface()->GetTime();
	Matrix3 tmUpNode = GetHub()->GetNodeTM(t);
	if (GetNumBones() > 1) tmUpNode.SetTrans(GetBoneData(1)->GetNodeTM(t).GetTrans());
	nodeUpnode->SetNodeTM(t, tmUpNode);

	GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	return upvectornode;
}

INode* LimbData2::CreateUpNode()
{
	return CreateUpNode(FALSE);
}
BOOL LimbData2::RemoveUpNode()
{
	if (!upvectornode)
		return true;

	// start at the leaves and work up
	if (!theHold.Holding()) theHold.Begin();

	GetCOREInterface()->DisableSceneRedraw();

	upvectornode->Delete(0, FALSE);
	DeleteReference(REF_UPVECTORNODE);

	theHold.Accept(GetString(IDS_HLD_UPNODEREM));

	UpdateLimb();
	GetCOREInterface()->EnableSceneRedraw();
	return true;
}

// With the new IKpos feature of the IK system we need to calculate the remaining limb length
// given that not the entire limb may be in IK. The IK chain may be terminated early by the
// ikpos value.
float LimbData2::GetLimbIKLength(TimeValue t, Interval& valid, int frombone, float dLimbIKPos/*=-1*/)
{
	int		numbones = GetNumBones();
	float	ikpos = (dLimbIKPos > 0) ? dLimbIKPos : GetLimbIKPos(t, valid);
	int		iLastFullBoneID = int(ikpos);
	float	dLength = 0;

	// Include the last fraction as a whole,
	// the effect of ikpos on the bone has
	// already been calculated (in GetFKTarget)
	if (iLastFullBoneID < ikpos)
		iLastFullBoneID++;

	// Ensure our data is valid
	if (!mFKTargetValid.InInterval(t) || (iLastFullBoneID >= mdFKBonePosition.Count()))
	{
		Matrix3 tmWeDontCare(1);
		GettmFKTarget(t, valid, NULL, tmWeDontCare);
	}

	// The ankle and the last bone in the chain have a special
	// relationship.  The rest of the chain solves to a IK Target
	// that is either at the base or the tip of the Palm, and
	// treat the palm and last bone as fused.
	// The last bone solves all the way to the IK target,
	// and so includes the full palm length in the LimbIK Length
	if (ikpos > numbones)
	{
		PalmTrans2* pPalm = GetPalmTrans();
		if (pPalm != NULL)
		{
			// If we are not the last bone, we use the
			// length the Palm calculates for itself to
			// generate an IK solution that solves with
			// the correct FK angle (when TargetAlign != 1)
			// NOTE: The last bone solves with the actual length.
			// the actual length will still vary from the
			// physical size of the palm, as its FK position
			// is still modulated based on TargetAlign in ModifyFKTM
			if (frombone < numbones - 1)
			{
				dLength = pPalm->GetExtensionLength(t, mFKPalmLocalTM);
				iLastFullBoneID--;
			}
			else
				return pPalm->GetBoneLength();
		}
	}

	// Get the length of full bones
	DbgAssert(iLastFullBoneID < mdFKBonePosition.Count());
	for (int i = frombone + 1; i < iLastFullBoneID; i++)
	{
		Point3 p3Offset = mdFKBonePosition[i + 1].GetTrans() - mdFKBonePosition[i].GetTrans();
		dLength += p3Offset.Length();
	}
	return dLength;
}

void LimbData2::MatchIKandFK(TimeValue t)
{
	DbgAssert(layerIKFKRatio);

	// Cache the Palms transform
	PalmTrans2* pPalm = GetPalmTrans();
	Matrix3 tmPalm, tmParent;
	if (pPalm != NULL)
	{
		tmPalm = pPalm->GetNodeTM(t);
		tmParent = pPalm->GetParentTM(t);
	}

	KeyLimbBones(t);

	// Now, with the limb bones cached,
	// reset the palms transform.  The
	// limb might have changed in the previous
	// call, due to the differences in the FK
	// solution affecting IK
	if (pPalm != NULL)
	{
		KeyFreeformMode kf(pPalm->GetLayerTrans());
		SetXFormPacket pk(tmPalm, tmParent);
		pPalm->SetValue(t, &pk, 1, CTRL_RELATIVE);
	}

	Interval iv = FOREVER;
	if (!(GetCATMode() == SETUPMODE || GetisLeg()) && GetIKFKRatio(t, iv) > 0.5f)
		MoveIKTargetToEndOfLimb(t);

	UpdateLimb();
}

void LimbData2::MoveIKTargetToEndOfLimb(TimeValue t)
{
	Matrix3 tmIKTarg;
	PalmTrans2* palm = (PalmTrans2*)GetPalmTrans();
	if (palm) {
		tmIKTarg = palm->GetNodeTM(t);
		int lengthaxis = GetLengthAxis();

		float palmLength = palm->GetBoneLength();
		tmIKTarg.PreTranslate(FloatToVec(palmLength, lengthaxis));

		if (GetisLeg())
		{
			Interval iv = FOREVER;
			Matrix3 tmPalmLocal(1);
			tmPalmLocal = palm->CalcPalmRelToIKTarget(t, iv, tmPalmLocal);

			// Apply inverse to take us away from the tip of the
			// palm back to where the foot platform could be.
			tmPalmLocal.Invert();
			tmIKTarg = tmPalmLocal * tmIKTarg;
		}
	}
	else
	{
		BoneData* pLastBone = GetBoneData(GetNumBones() - 1);
		DbgAssert(pLastBone != NULL);
		tmIKTarg = pLastBone->GetNodeTM(t);
		pLastBone->ApplyChildOffset(t, tmIKTarg);
	}

	tmIKTarg.PreRotateY(PI);

	if (iktarget)
		iktarget->SetNodeTM(t, tmIKTarg);
	else if (layerIKTargetTrans)
	{
		SetXFormPacket ptr(tmIKTarg);
		layerIKTargetTrans->SetValue(t, (void*)&ptr, 1, CTRL_ABSOLUTE);
	}

	if (upvectornode) {
		Matrix3 tmUpNode;
		if (GetNumBones() > 1) tmUpNode = GetBoneData(1)->GetNodeTM(t);
		else				tmUpNode = GetBoneData(0)->GetNodeTM(t);
		upvectornode->SetNodeTM(t, tmUpNode);
	}
}

void LimbData2::SelectIKTarget()
{
	if (iktarget)
	{
		BOOL newundo = FALSE;
		if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }

		Interface* ip = GetCOREInterface();
		// this stops the rollouts being destroyed and created
		SetCCFlag(CNCFLAG_KEEP_ROLLOUTS);
		// deselect any selected nodes.
		// there should be only 1
		int selectioncount = ip->GetSelNodeCount();
		for (int i = 0; i < selectioncount; i++)
			ip->DeSelectNode(ip->GetSelNode(i));

		ip->SelectNode(iktarget);
		ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);

		if (newundo && theHold.Holding()) theHold.Accept(GetString(IDS_HLD_IKTARGSEL));
		ip->RedrawViews(ip->GetTime());
	}
}

void LimbData2::SelectLimbEnd()
{
	if (iktarget)
	{
		Interface* ip = GetCOREInterface();
		// this stops the rollouts being destroyed and created
	//	BlockAllEvaluation(TRUE);
		SetCCFlag(CNCFLAG_KEEP_ROLLOUTS);
		// deselect any selected nodes.
		// there should be only 1
		int selectioncount = ip->GetSelNodeCount();
		for (int i = 0; i < selectioncount; i++)
			ip->DeSelectNode(ip->GetSelNode(i));

		CATNodeControl* palm = GetPalmTrans();
		if (palm)
			ip->SelectNode(palm->GetNode());
		else GetBoneData(GetNumBones() - 1)->GetNode();
		//	BlockAllEvaluation(FALSE);
		ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);
		ip->RedrawViews(ip->GetTime());
	}
}

Matrix3 LimbData2::GettmIKTarget(TimeValue t, Interval &valid)
{
	// Don't modify the cached mtmIKTarget, because it may be
	// blended between IK & FK, and we don't wanna screw with that
	// This function should always just get the -real- IK target.
	// The function GetIKTM's will update the mtmIKTarget cache.
	Matrix3 tmIKTarget(1);
	if (!TestFlag(LIMBFLAG_LOCKED_IK))
	{
		if (iktarget)
		{
			if (iktarget->IsHidden())
				iktarget->EvalWorldState(t, TRUE);

			tmIKTarget = iktarget->GetObjTMAfterWSM(t, &valid);

			CATNodeControl *ipar = (CATNodeControl*)iktarget->GetTMController()->GetInterface(I_CATNODECONTROL);
			if (ipar)
				ipar->ApplyChildOffset(t, tmIKTarget);
		}
		else
		{
			tmIKTarget.IdentityMatrix();
			Point3 tempscale(P3_IDENTITY_SCALE);
			if (layerIKTargetTrans)
				layerIKTargetTrans->GetTransformation(t, tmIKTarget, tmIKTarget, valid, tempscale, tempscale);
			else
				tmIKTarget = mtmFKTarget;
		}

		// Flip the IK targets matrix upsideDown so that it aligns with the limb bone matrix
		tmIKTarget.PreRotateY((float)M_PI);
	}
	else
	{
		// If we are in locked IK mode, we always solve directly to the
		// set IK target (which will be set by the palm)
		tmIKTarget = mtmIKTarget;
	}
	return tmIKTarget;
}

Matrix3 LimbData2::GettmPalmTarget(TimeValue t, Interval &valid, const Matrix3& tmFKPalmLocal)
{
	int numbones = GetNumBones();
	float ikpos = GetLimbIKPos(t, valid);
	int lastIKbone = (int)floor(ikpos);

	Matrix3 tmIKTarget = GettmIKTarget(t, valid);

	PalmTrans2* palm = GetPalmTrans();
	if (palm)
	{
		if (lastIKbone >= numbones)
		{
			if (lastIKbone >= (numbones + 1))
			{
				palm->ModifyIKTM(t, valid, tmFKPalmLocal, tmIKTarget);
			}
			else
			{
				Matrix3 tempIK = tmIKTarget;
				palm->ModifyIKTM(t, valid, tmFKPalmLocal, tempIK);
				// blend the above calculated FKTarget with the previous bones FKTarget
				BlendMat(tmIKTarget, tempIK, (ikpos - (float)lastIKbone));
			}
		}
	}

	return tmIKTarget;
}

Matrix3 LimbData2::GettmFKTarget(TimeValue t, Interval &valid, const Matrix3* tmIKChainParent, Matrix3& tmFKPalmLocal)
{
	// Get our parent matrix.
	Matrix3 tmParent;
	if (tmIKChainParent != NULL)
		tmParent = *tmIKChainParent;
	else
	{
		BoneData* pBone = GetBoneData(0);
		if (pBone != NULL)
			tmParent = pBone->GetParentTM(t);
		else
			return Matrix3(1);
	}

	if (mFKTargetValid.InInterval(t) && mtmFKParent == tmParent)
	{
		tmFKPalmLocal = mFKPalmLocalTM;
		return mFKTargetLocalTM * tmParent;
	}

	//////////////////////////////////////////////////////////////////////
	// We must reset the scale so that the following GetValues work on the unscaled length of the limb.
	mtmFKParent = tmParent;

	//////////////////////////////////////////////////////////////////////
	// calculate where our FK chain is terminated.
	// force an evaluation of the FK hierarchy to make
	// sure our scale system is up to date

	// Cache the validity of our FK target
	Interval fkTargetValid = FOREVER;

	int numbones = GetNumBones();
	float ikpos = GetLimbIKPos(t, fkTargetValid);
	int lastIKbone = (int)floor(ikpos);
	Matrix3 tmLastBoneWorld;
	Matrix3 tmWorld = tmParent;

	// Force a full FK evaluation
	// Do not trigger an update with this call.  It should be unnecessary.
	{
		MaxReferenceMsgLock lockThis;
		CATMessage(t, CAT_LOCK_LIMB_FK, TRUE);
	}

	// While we are in FK, cache our child lengths.
	// This array should be (for 3 bones with palm)
	// [Bone1FK, Bone2FK, Bone3FK, PalmFK, PalmInterpFK, PalmTipFK]
	// Without palm
	// [Bone1FK, Bone2FK, Bone3FK, Bone3FKTip]
	PalmTrans2* pPalm = GetPalmTrans();
	int nLimbBones = numbones + ((pPalm != NULL) ? 3 : 1);
	mdFKBonePosition.SetCount(nLimbBones);
	for (int i = 0; i < nLimbBones; i++)
		mdFKBonePosition[i].IdentityMatrix();

	DbgAssert(ikpos <= nLimbBones);
	DbgAssert(lastIKbone < nLimbBones);
	int nBonesToCalc = numbones + ((pPalm != NULL) ? 2 : 1);
	for (int i = 0; i < nBonesToCalc; i++)
	{
		GetBoneTM(i, t, tmWorld, fkTargetValid);
		mdFKBonePosition[i] = tmWorld;

		// For the IK solution to solve correctly, we need the FK transform
		// of the palm (see Palm::ModifyIKTM).  We calculate it here
		// because we are conventiently entirely in FK.
		if (i == numbones - 1)
			tmLastBoneWorld = tmWorld;
		else if (i == numbones)
		{
			// tmWorld = palm.  Calculate its local bits.
			// If no palm exists, this is the last bone
			// just offset to the end of the limb
			tmFKPalmLocal = tmWorld * Inverse(tmLastBoneWorld);

			PalmTrans2* pPalm = GetPalmTrans();
			if (pPalm != NULL)
			{
				// Calculate the point at the end of the palm
				Matrix3 tmTerribleHack = tmWorld;
				pPalm->CATNodeControl::ApplyChildOffset(t, tmTerribleHack);
				mdFKBonePosition[nLimbBones - 1] = tmTerribleHack;
			}
		}
	}

	// If our ikpos is half way down the bone, then
	// move tmWorld back by the appropriate amount.
	float dOffset = ikpos - lastIKbone;
	if (dOffset != 0)
	{
		DbgAssert(lastIKbone < nBonesToCalc);
		// Do not let the offset shrink to 0.  If we did this,
		// we end up trying to solve IK for a 0-length bone.
		dOffset = max(dOffset, 0.01f);
		Point3 p3Offset = mdFKBonePosition[lastIKbone + 1].GetTrans() - mdFKBonePosition[lastIKbone].GetTrans();
		p3Offset *= dOffset;

		// Offset the final bone (and tmWorld).
		Point3 finalFKPos = mdFKBonePosition[lastIKbone].GetTrans() + p3Offset;
		mdFKBonePosition[lastIKbone + 1].SetTrans(finalFKPos);
		tmWorld.SetTrans(finalFKPos);
	}

	// Leave locked FK mode
	// Do not trigger an update with this call.  It should be unnecessary.
	{
		MaxReferenceMsgLock lockThis;
		CATMessage(t, CAT_LOCK_LIMB_FK, FALSE);
	}

	// Cache the local FK result from the root of the hierarchy
	// Our mFKTargetValid was be reset by the CATMessage above, so we kept a cache of the
	// validity in a local variable instead (until here).
	mFKTargetValid = fkTargetValid;
	mFKTargetLocalTM = tmWorld * Inverse(tmParent);
	mFKPalmLocalTM = tmFKPalmLocal;

	// Calculate our bend ratios...
	// We need these ratios on all bones except the last 2
	Point3 p3FKTarget = mdFKBonePosition[lastIKbone].GetTrans();
	for (int i = 0; i < lastIKbone - 2; i++)
	{
		BoneData* pBone = GetBoneData(i);
		if (pBone == NULL)
			continue;
		// Get the combined length of all children, excluding the palm
		double child_lengths = GetLimbIKLength(t, valid, i);

		// How long am I?
		double bone_length = (GetBoneFKPosition(t, i + 1) - GetBoneFKPosition(t, i)).Length();

		// How far to the FK Target?
		double this_fk_bone_to_fk_target_length = (GetBoneFKPosition(t, i) - p3FKTarget).Length();
		// shortest possible child length is where we look at the target
		double fk_short_child = this_fk_bone_to_fk_target_length - bone_length;
		// the longest is where we look in the other direction
		double fk_long_child = min(child_lengths, this_fk_bone_to_fk_target_length + bone_length);

		// How far from my child (my tip position) to the target when calculated in FK?
		double child_bone_to_fk_target_length = Length(GetBoneFKPosition(t, i + 1) - p3FKTarget);
		double fk_child_diff = fk_long_child - fk_short_child;
		float bend_ratio = 0;
		if (fk_child_diff > 0.0f)
		{
			bend_ratio = float((child_bone_to_fk_target_length - fk_short_child) / (fk_child_diff));
			DbgAssert(bone_length < 0.0001 || bend_ratio <= 1.0f);
		}
		pBone->SetBendRatio(bend_ratio);
	}

	valid &= mFKTargetValid;
	mtmFKTarget = tmWorld;
	return tmWorld;
}

void LimbData2::GetBoneTM(int iBoneID, TimeValue t, Matrix3& tmWorld, Interval& ivValid)
{
	int nBones = GetNumBones();

	// We have been passed the matrix of the parent bone, however
	// each bone expects to receive the matrix of the parent segment.
	// This is because an BoneData's parent is not the parent BoneData,
	// but its last ILimbSeg.  If an BoneData has multiple segments, each
	// segment is only 1/nSegs long, we need to offset the tmWorld through
	// each of these segments.
	// This is why I originally implemented this function to use GetNodeTM,
	// it seems very messy to have LimbData 'know' so much about BoneData's
	// construction.
	if (iBoneID > 0 && iBoneID <= nBones)
	{
		BoneData* pParentBone = GetBoneData(iBoneID - 1);
		if (pParentBone != NULL)
		{
			CATNodeControl* pBoneSeg = pParentBone->GetBone(0);
			for (int i = 1; i < pParentBone->GetNumBones(); i++)
			{
				pBoneSeg->ApplyChildOffset(t, tmWorld);
			}
		}
	}

	if (iBoneID >= nBones)
		return GetPalmTM(iBoneID, t, tmWorld, ivValid);

	// Find which bone will give us our values
	CATNodeControl* pBone = GetBoneData(iBoneID);
	// We cannot evaluate via GetNodeTM here,
	// because it is possible we are evaluating from
	// a 'theoretical' parent position (when applying
	// reach to the hub?)
	if (pBone != NULL)
		pBone->GetValue(t, &tmWorld, ivValid, CTRL_RELATIVE);
}

void LimbData2::GetPalmTM(int iBoneID, TimeValue t, Matrix3& tmWorld, Interval& ivValid)
{
	int nBones = GetNumBones();
	DbgAssert(iBoneID >= nBones);

	PalmTrans2* pPalm = GetPalmTrans();

	// If we do not have a palm, then we simply
	// offset to the end of the last bone
	if (pPalm == NULL)
	{
		BoneData* pBone = GetBoneData(nBones - 1);
		if (pBone != NULL)
		{
			int nChildBones = pBone->GetNumBones();
			CATNodeControl* pLastBone = pBone->GetBone(nChildBones - 1);
			if (pLastBone != NULL)
				pLastBone->ApplyChildOffset(t, tmWorld);
		}
	}
	else
	{
		// the transform at nBones is the Palm itself,
		if (iBoneID == nBones)
		{
			pPalm->GetValue(t, &tmWorld, ivValid, CTRL_RELATIVE);
		}
		else // iBoneID > nBones - this is the FK target
			pPalm->ModifyFKTM(t, tmWorld);
	}
};

void LimbData2::SetTemporaryIKTM(const Matrix3& tmIKTarget, const Matrix3& tmFKTarget)
{
	// It is REQUIRED for this function to work, that the hold be holding already
	DbgAssert(theHold.Holding());
	DbgAssert(TestFlag(LIMBFLAG_LOCKED_IK));

	mtmIKTarget = tmIKTarget;
	mtmFKTarget = tmFKTarget;
}

void LimbData2::GetIKTMs(TimeValue t, Interval &valid, const Point3& p3LimbRoot, const Matrix3& tmFKParent, Matrix3 &IKTarget, Matrix3 &FKTarget, int boneid/*=0*/)
{
	if (IsEvaluationBlocked()) return;

	// If we are locked in IK mode, just return the target.
	if (TestFlag(LIMBFLAG_LOCKED_IK))
	{
		IKTarget = mtmIKTarget;
		FKTarget = mtmFKTarget;
		return;
	}

	// Cache this before calling GettmFKTarget below.
	bool bIKValid = mIKSystemValid.InInterval(t) && mFKTargetValid.InInterval(t);

	Matrix3 tmPalmLocal(1);
	const Matrix3* ptmFKParent = (boneid == 0) ? &tmFKParent : NULL;
	mtmFKTarget = GettmFKTarget(t, mIKSystemValid, ptmFKParent, tmPalmLocal);

	// Check to see if our IK system is still valid.
	// Because the IK is dependent on the FK, if the FK target changes, we need to update the IK system
	// TODO: Verify that this caching is (1) effective, and (2) complete.
	if (!bIKValid) {
		mIKSystemValid = FOREVER;
		mp3LimbRoot = p3LimbRoot;

		// Cache our palm length axis (we'll need it later
		//  to figure out which way to bend the last bone)
		mp3PalmLocalVec = tmPalmLocal.GetRow(GetLengthAxis());

		mtmIKTarget = GettmPalmTarget(t, mIKSystemValid, tmPalmLocal);

		float dIKFKRatio = GetIKFKRatio(t, mIKSystemValid, -1);
		if (dIKFKRatio > 0)
		{
			// Now blend our IK target to the FK target, but in a circular arc to prevent collapsing limbs
			Point3 p3Root = p3LimbRoot;
			Point3 p3IKPos = mtmIKTarget.GetTrans();
			Point3 p3FKPos = mtmFKTarget.GetTrans();

			Point3 p3ToIKPos = p3IKPos - p3Root;
			Point3 p3ToFKPos = p3FKPos - p3Root;

			float dIKLength = p3ToIKPos.LengthUnify();
			float dFKLength = p3ToFKPos.LengthUnify();

			Point3 axis = Normalize(CrossProd(p3ToIKPos, p3ToFKPos));
			float angle = acos(min(DotProd(p3ToIKPos, p3ToFKPos), 1.0f));
			angle *= dIKFKRatio;

			// Convert to Matrix3
			Matrix3 tmIKBlended = RotAngleAxisMatrix(axis, angle);

			Point3 p3ToIKBlended = tmIKBlended.VectorTransform(p3ToIKPos);
			p3ToIKBlended *= BlendFloat(dIKLength, dFKLength, dIKFKRatio);

			BlendMat(mtmIKTarget, mtmFKTarget, dIKFKRatio);
			mtmIKTarget.SetTrans(p3Root + p3ToIKBlended);
		}
	}

	// This should have been evaluated in GettmFKTarget
	DbgAssert(boneid < mdFKBonePosition.Count());

	IKTarget = mtmIKTarget;
	FKTarget = mtmFKTarget;
	valid &= mIKSystemValid;
}

bool LimbData2::GetIKAngleFlip(TimeValue t, float dDesiredAngle, const Point3& p3Axis, const Matrix3& tmIkBase, int iBoneID)
{
	// If we are the last,
	int iNumBones = GetNumBones();
	if (iBoneID == (iNumBones - 1))
	{
		// and we have some target align offset
		PalmTrans2* pPalm = GetPalmTrans();
		Interval iv;
		if (pPalm != NULL && pPalm->GetTargetAlign(t, iv) < 1)
		{
			DbgAssert(FALSE);  // Not sure, but I believe this is obsolete (2010)
			// And we are up to date
			DbgAssert(mIKSystemValid.InInterval(t));

			// Test to ensure that the desired angle will rotate around
			// the axis in the correct direction.  The final bone (only) in a limb has
			// 2 equally correct solutions at dDesiredAngle & -dDesiredAngle, and it
			// is possible for it to chose the wrong one.  We need to ensure that
			// the rotation given by dDesiredAngle allows the palm to rotate
			// in the correct direction.
			//

			 // Create a basis out of our axis and length vecs

			int iLengthAxis = GetLengthAxis();

			// This is sideways axis, basically
			//int iDiscriminantAxis = ; // Fancy name, cause it sounds smarter that way...
			// The IK base discriminant tells us that positive rotations go the same way on IK Base & axis
			bool bIkBaseDiscriminant = DotProd(tmIkBase.GetRow((iLengthAxis + 2) % 3), p3Axis) > 0;
			// The local discriminant tells us if we are either a positive or negative rotation relative to the IK base
			bool bPalmLocalDiscriminant = mp3PalmLocalVec[(iLengthAxis + 1) % 3] > 0;
			bool bAngleDiscriminant = dDesiredAngle > 0;

			// If the IKBase & the axis rotate the same way, then the palmlocal discriminant & the sign of the angle
			// should agree.  If IKBase and axis are opposite, then the either the local or the angle should be negative
			return (bIkBaseDiscriminant) == (bAngleDiscriminant == bPalmLocalDiscriminant);
		}
	}
	else if (iBoneID > 0)
	{
		// An IK system can't know which way (+ or 1) to rotate a limb bone, as they are both correct.
		// However, the method that is most similar to FK is the one we want.  However, because the IK
		// solution has no world orientation we can rely on, we simply test if the same calculated
		// rotation axis on the FK solution would be on the same side of the matrix as a perpendicular
		// row.
		// Guess which axis should be perpendicular to the plane created by the vector
		// looking directly at the FK target and the tmFK length axis.
		int iLengthAxis = GetLengthAxis();
		int iPerpAxis = (iLengthAxis + 2) % 3;

		Matrix3& tmFK = mdFKBonePosition[iBoneID];
		Point3 p3FKToTarg = mtmFKTarget.GetTrans() - tmFK.GetTrans();
		Point3 p3FKAxis = CrossProd(tmFK.GetRow(iLengthAxis), p3FKToTarg);

		float dIsPerp = DotProd(p3FKAxis, tmFK.GetRow(iPerpAxis));
		// If the DotProd of the 2 axis less than half the length of the
		// FK Axis, this means we are not parallel.
		if ((dIsPerp * dIsPerp * 2) < (p3FKAxis.LengthSquared()))
		{
			// If the first axis wasn't parrallel, this one will be!
			iPerpAxis = (iLengthAxis + 1) % 3;
			dIsPerp = DotProd(p3FKAxis, tmFK.GetRow(iPerpAxis));

			// Our IK matrix should have similar layout WRT the perp axis
			DbgAssert(DotProd(p3Axis, tmIkBase.GetRow(iPerpAxis)) > 0.3f);
		}
		bool bReverseFK = dIsPerp < 0;
		bool bReverseIK = DotProd(p3Axis, tmIkBase.GetRow(iPerpAxis)) < 0;

		// If our calculated axis are on opposite side sides of their
		// respective world matrices, then we need to reverse the angle
		// in order to make sure we bend the right way
		return bReverseFK != bReverseIK;

		//Point3 p3FKAxis = CrossProd(tmIkBase.GetRow(iLengthAxis), tmIKParent.GetRow(iLengthAxis));
//		bool iSignDet = DotProd(p3FKAxis, p3Axis) > 0;

//		bool iAngleDet = DotProd(p3Axis, tmIkBase.GetRow(X)) > 0;

		// I hate relying on child info, but I can't see any way out here.
		//Point3 p3ParFKPos = GetBoneFKPosition(t, iBoneID - 1);
		//Point3 p3BoneFKPos = GetBoneFKPosition(t, iBoneID);
		//Point3 p3BoneFKTarget = GetBoneFKPosition(t, iNumBones);
		//Point3 p3ChildFKPos = GetBoneFKPosition(t, iBoneID + 1);

		//Point3 p3ToFKTarg = p3BoneFKPos - p3BoneFKTarget;
		//Point3 p3ToChild = p3BoneFKPos - p3ChildFKPos;

		//Point3 p3ParToFKTarg = p3ParFKPos - p3BoneFKTarget;
		//Point3 p3ParToMe = p3ParFKPos - p3BoneFKPos;

		//Point3 p3ParFKAxis = CrossProd(p3ParToFKTarg, p3ParToMe);
		//Point3 p3MyFKAxis = CrossProd(p3ToFKTarg, p3ToChild);

		//bool bAxisPointSameWay = DotProd(p3ParFKAxis, p3MyFKAxis) > 0;

		//return !bAxisPointSameWay;
		//// Does our FKAxis point the same way as our IKAxis?
		////bool bIKAxisDet =
		////float dAngleToFK = -acos(min(1.0f, DotProd(p3BoneFKPos, p3ToChild)));
		////return (dAngleToFK > 0 && dDesiredAngle > 0) || (dAngleToFK < 0 && dDesiredAngle < 0);

	}
	return false;
}

bool LimbData2::IsLimbStretching(TimeValue t, const Point3& ikTarget)
{
	// To be stretching, all members must be stretchy.
	int nBones = GetNumBones();
	CATMode mode = GetCATMode();
	for (int i = 0; i < nBones; i++)
	{
		CATNodeControl* pBone = GetBoneData(i);
		if (pBone == NULL || !pBone->IsStretchy(mode))
			return false;
	}

	if (GetNumBones() == 0)
		return false;

	// Now the limb is stretching if its target is out of reach
	Interval validIK;
	return ((ikTarget - GetBoneFKPosition(t, 0)).Length() > GetLimbIKLength(t, validIK, -1));
}

Point3 LimbData2::GetLimbRoot(TimeValue t)
{
	UNUSED_PARAM(t);
	DbgAssert(mIKSystemValid.InInterval(t));
	return mp3LimbRoot;
}

Point3 LimbData2::GetBoneFKPosition(TimeValue t, int iBoneID)
{
	UNUSED_PARAM(t);
	DbgAssert(iBoneID < mdFKBonePosition.Count());
	DbgAssert(mFKTargetValid.InInterval((t)));
	if (iBoneID < mdFKBonePosition.Count())
		return mdFKBonePosition[iBoneID].GetTrans();
	return Point3::Origin;
}

bool LimbData2::GetBoneFKTM(TimeValue t, int iBoneID, Matrix3& tm)
{
	UNUSED_PARAM(t);
	DbgAssert(iBoneID < mdFKBonePosition.Count());
	DbgAssert(mFKTargetValid.InInterval((t)));
	if (iBoneID < mdFKBonePosition.Count())
	{
		tm = mdFKBonePosition[iBoneID];
		return true;
	}
	return false;
}

void LimbData2::UpdateLimb()
{
	if (!pblock) return;
	int isUndoing = 0;
	if (IsEvaluationBlocked() && (!theHold.Restoring(isUndoing) && isUndoing)) return;

	// If ForceFeedback is on, changes in the limb can affect the
	// hubs position, force an upate on it.
	Interval iv = FOREVER;
	TimeValue t = GetCOREInterface()->GetTime();
	if (GetIKFKRatio(t, iv) < 1.0f && (GetForceFeedback(t, iv) > 0.0f || TestFlag(LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS)) && GetHub())
		GetHub()->Update();
	else
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

	// Make sure we re-evaluate our FK
	InvalidateFKSolution();

	if (iktarget) {
		//	iktarget->GetTMController()->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
		iktarget->InvalidateTreeTM();
	}
}

RefResult LimbData2::NotifyRefChanged(
	const Interval&,
	RefTargetHandle hTarget,
	PartID&,
	RefMessage message,
	BOOL)
{
	//	if(theHold.RestoreOrRedoing()) return REF_SUCCEED;
	TimeValue t = GetCOREInterface()->GetTime();
	switch (message)
	{
	case REFMSG_CHANGE:
	{
		if (hTarget == layerIKFKRatio || hTarget == layerRetargeting || hTarget == layerIKTargetTrans || hTarget == iktarget)
		{
			Interval iv = FOREVER;
			staticLimbMotionRollout.UpdateIKFK(t);
			if (GetIKFKRatio(t, iv) < 1.0f && GetForceFeedback(t, iv) > 0.0f || TestFlag(LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS)) {
				GetHub()->Update();
			}
		}
		else if (hTarget == layerLimbIKPos)
		{
			staticLimbMotionRollout.UpdateLimbIKPos(t);
			InvalidateFKSolution();
			Interface* coreInterface = GetCOREInterface();
			if (coreInterface != NULL && !coreInterface->IsAnimPlaying())
				coreInterface->RedrawViews(coreInterface->GetTime());
		}
		else if (hTarget == layerRetargeting)		staticLimbMotionRollout.UpdateLimbLayerRetargetting(t);

		if (hTarget == iktarget ||
			hTarget == layerIKTargetTrans ||
			hTarget == layerIKFKRatio ||
			hTarget == layerRetargeting ||
			hTarget == weights)
		{
			mIKSystemValid.SetEmpty();

			// Invalidate transforms.
			BoneData* pBone = GetBoneData(0);
			if (pBone != NULL)
				pBone->InvalidateTransform();

			// the following if statement is a fix to a bug that popped up
			// very close to the release of CAT 2.0.
			// The limb stopped updating when the IKFK slider was moved,
			// I don't think I had made any recent changes to the update system.!?!
			// The notifications are getting thru, and this used to be enough.
			// ST (BUG 1351481) Turns out we still need this?!?  Not sure why this is necessary
			if (hTarget == layerIKFKRatio ||
				hTarget == layerRetargeting ||
				hTarget == weights) {
				if (!GetCOREInterface()->IsAnimPlaying())
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			}
			break;
		}

		if (!pblock) return REF_SUCCEED;

		if (pblock == hTarget)
		{
			int tabIndex = 0;
			ParamID index = pblock->LastNotifyParamID(tabIndex);
			switch (index)
			{
			case PB_LIMBFLAGS:
				flags = pblock->GetInt(PB_LIMBFLAGS);
				if (!IsEvaluationBlocked()) {
					UpdateLimb();
				}
				break;
			case PB_NUMBONES:
				if (!IsEvaluationBlocked()) {
					CreateLimbBones(FALSE);
					mIKSystemValid.SetEmpty();
					InvalidateFKSolution();
				}
				break;
			case PB_NAME:
				CATMessage(0, CAT_NAMECHANGE);
				break;
			case PB_COLOUR:
				if (!IsEvaluationBlocked()) {
					CATMessage(GetCOREInterface()->GetTime(), CAT_COLOUR_CHANGED, 0);
				}
				break;
			}
		}
	}
	break;
	case REFMSG_TARGET_DELETED:
	{
		if (hTarget == layerRetargeting)	layerRetargeting = NULL;
		if (hTarget == layerIKFKRatio)		layerIKFKRatio = NULL;
		if (hTarget == layerLimbIKPos)		layerLimbIKPos = NULL;
		if (hTarget == iktarget)
		{
			// If we are losing our pointer to iktarget, then
			// NULL it's pointer to us (if we didn't, we'd have
			// no way to notify foottrans when we were deleted).
			CATNodeControl* pLimbCtrl = dynamic_cast<CATNodeControl*>(iktarget->GetTMController());
			if (pLimbCtrl != NULL)
			{
				MaxAutoDeleteLock lock(pLimbCtrl, false);
				pLimbCtrl->UnlinkCATHierarchy();
				//pLimbCtrl->SetLimb(NULL);
			}
			iktarget = NULL;
		}
		if (hTarget == upvectornode)
		{
			CATNodeControl* pUpVecCtrl = dynamic_cast<CATNodeControl*>(upvectornode->GetTMController());
			if (pUpVecCtrl != NULL)
			{
				MaxAutoDeleteLock lock(pUpVecCtrl, false);
				pUpVecCtrl->UnlinkCATHierarchy();
				//pUpVecCtrl->SetLimb(NULL);
			}
			upvectornode = NULL;
		}
		break;
	}
	}
	return REF_SUCCEED;
}

CATClipValue* LimbData2::GetLayerController(int i) {
	switch (i) {
	case 0:return (CATClipValue*)weights;
		//		case 1:return layerSwivel;
	case 2:return layerRetargeting;
	case 3:return layerIKFKRatio;
	case 4:return layerLimbIKPos;
	case 5:return (iktarget) ? NULL : static_cast<CATClipValue*>(layerIKTargetTrans);
	default:return NULL;
	}
}

int LimbData2::NumChildCATControls()
{
	return GetNumBones() + (GetPalmTrans() ? 1 : 0) + (GetCollarbone() ? 1 : 0) + (iktarget ? 1 : 0) + (upvectornode ? 1 : 0);
}

CATControl* LimbData2::GetChildCATControl(int i)
{
	// NOTE - children need to be returned in order of dependence (because it
	// is assumed that any values you depend on will be keyed first in a KeyFreeform)
	// In other words, if we returned the palm before we returned the IK target, then
	// the Palm would be relying on the value of the IK target (which is not yet
	// keyed) when calculating its own value.
	if (iktarget) {
		if (i == 0)				return (CATControl*)iktarget->GetTMController()->GetInterface(I_CATNODECONTROL);
		i--;
	}
	if (upvectornode) {
		if (i == 0)				return (CATControl*)upvectornode->GetTMController()->GetInterface(I_CATNODECONTROL);
		i--;
	}

	CATNodeControl* collarbone = GetCollarbone();
	if (collarbone) {
		if (i == 0)				return (CATControl*)collarbone;
		i--;
	}
	if (i < GetNumBones())		return (CATControl*)GetBoneData(i);
	i -= GetNumBones();
	if (GetPalmTrans()) {
		if (i == 0)				return (CATControl*)GetPalmTrans();
		i--;
	}
	return NULL;
}

void LimbData2::DestructAllCATMotionLayers()
{
	// This handles removing us from all layer-related issues
	ICATParentTrans* pCPTrans = GetCATParentTrans();
	if (pCPTrans != NULL)
	{
		CATClipRoot* pLayerRoot = pCPTrans->GetCATLayerRoot();
		if (pLayerRoot != NULL)
		{
			for (int i = 0; i < pLayerRoot->NumLayers(); i++)
			{
				CATMotionLayer* info = dynamic_cast<CATMotionLayer*>(pLayerRoot->GetLayer(i));
				if (info)
				{
					// Remove from the CATHierarchy
					CATHierarchyRoot* pRoot = static_cast<CATHierarchyRoot*>(info->GetReference(CATMotionLayer::REF_CATHIERARHYROOT));
					if (pRoot != NULL)
						pRoot->RemoveLimb(this);
				}
			}
		}
	}
}

CATControl* LimbData2::GetParentCATControl()
{
	return GetHub();
}

TSTR	LimbData2::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetLimbID());

	Hub* pHub = GetHub();
	assert(pHub);
	return (pHub->GetBoneAddress() + _T(".") + bonerigname + boneid);
};

INode*	LimbData2::GetBoneByAddress(TSTR address) {
	if (address.Length() == 0) return NULL;
	int boneid;
	USHORT rig_id = GetNextRigID(address, boneid);
	// we cannot terminate on a CATControl
	if (rig_id == GetRigID())  return NULL;
	if (boneid >= 0 && boneid < GetNumBones()) {
		DbgAssert(GetBoneData(boneid)->GetRigID() == rig_id);
		return GetBoneData(boneid)->GetBoneByAddress(address);
	}

	CATNodeControl* collarbone = GetCollarbone();
	if (collarbone && collarbone->GetRigID() == rig_id)
		return collarbone->GetBoneByAddress(address);

	PalmTrans2* palm = GetPalmTrans();
	if (palm && palm->GetRigID() == rig_id)
		return palm->GetBoneByAddress(address);
	if (iktarget && ((CATNodeControl*)iktarget->GetTMController())->GetRigID() == rig_id)
		return ((CATNodeControl*)iktarget->GetTMController())->GetBoneByAddress(address);

	// We didn't find a bone that matched the address given
	// this may be because this rigs structure is different
	// from that of the rig that generated this address
	return NULL;
};

// CATs Internal messaging system, cause using
// Max's system is like chasing mice with a mallet
void LimbData2::CATMessage(TimeValue t, UINT msg, int data)
{
	//	if(GetLimbCreating()) return;
	switch (msg)
	{
	case CAT_CATUNITS_CHANGED:
		InvalidateFKSolution();
		break;
	case CLIP_LAYER_INSERT:
	case CLIP_LAYER_APPEND:
		if (GetCATMode() == SETUPMODE &&
			!GetisLeg())
		{
			// If we are creating a new layer, we only
			// want to reset the position of the transform
			// node if we are adding an absolute layer
			if (layerLimbIKPos && layerLimbIKPos->GetLayerMethod(data) == CTRL_ABSOLUTE)
			{
				Interval iv = FOREVER;
				if (GetIKFKRatio(t, iv) > 0.5f)
					MoveIKTargetToEndOfLimb(t);
			}
		}
		break;
	case CAT_CATMODE_CHANGED:
		UpdateUI();
	case CAT_UPDATE:
		UpdateLimb();
		//	return;
		break;
	case CAT_LOCK_LIMB_FK:
		if (data)	SetFlag(LIMBFLAG_LOCKED_FK);
		else		ClearFlag(LIMBFLAG_LOCKED_FK);
		break;
	}

	CATControl::CATMessage(t, msg, data);
}

/**********************************************************************
 * Loading and saving....
 */

 //////////////////////////////////////////////////////////////////////
 // Backwards compatibility
 //

class LimbPLCB : public PostLoadCallback {
protected:
	LimbData2 *limb;

public:
	LimbPLCB(LimbData2 *pOwner) { limb = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(limb);
		if (limb->dwFileSaveVersion > 0) return limb->dwFileSaveVersion;

		ICATParentTrans *catparent = limb->GetCATParentTrans();
		if (catparent) return catparent->GetFileSaveVersion();
		return (DWORD)-1;
	}

	int Priority() { return 1; }

	void proc(ILoad *iload) {

		// New scale controllers were added tot he paremeter blcos for BoneDatas
		// this is just to make people realise that they cant
		// go backwards after they have saved over thier files with this version
		if (GetFileSaveVersion() < CAT_VERSION_1150)
			iload->SetObsolete();

		if (GetFileSaveVersion() < CAT_VERSION_1200) {
			if (!limb->GetisLeg()) {
				limb->pblock->EnableNotifications(FALSE);
				limb->SetFlag(LIMBFLAG_SETUP_FK);
				limb->pblock->EnableNotifications(TRUE);
			}
			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_1730) {
			if (limb->pblock->GetControllerByID(LimbData2::PB_LEGWEIGHT)) {
				// removing the branch will stop this controller from evaluating
				((CATGraph*)limb->pblock->GetControllerByID(LimbData2::PB_LEGWEIGHT))->SetBranch(NULL);
				limb->pblock->SetControllerByID(LimbData2::PB_LEGWEIGHT, 0, (Control*)NULL, FALSE);
			}
			iload->SetObsolete();
		}
		if (GetFileSaveVersion() < CAT_VERSION_2000) {
			limb->weights->weights = NULL;
			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_2435) {
			// Initialise the INI file
			CatDotIni catCfg;
			catCfg.Set(_T("ButtonTexts"), _T("sldLimbIKPos"), GetString(IDS_BTN_NUMIKBONES));
		}

		// Let the limb start evaluating
		limb->SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, FALSE);

		// Get everything ready to go

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define		LIMBCHUNK_CATCONTROL			16
#define		LIMBCHUNK_LIMBFLAGS				1
#define		LIMBCHUNK_LIMBID_OBSOLETE		2
#define		LIMBCHUNK_COLOUR				3
#define		LIMBCHUNK_NAME_OBSOLETE			4

#define		LIMBCHUNK_CATPARENT				10
#define		LIMBCHUNK_HUB					11
#define		LIMBCHUNK_COLLARBONE			12
#define		LIMBCHUNK_PALM					13
#define		LIMBCHUNK_NUMBONES_OBSOLETE		14
#define		LIMBCHUNK_BONES_OBSOLETE		15

#define		LIMBCHUNK_SETUPTARGETALIGN		20

#define		LIMBCHUNK_IKSOFTLIMIT_MIN		21
#define		LIMBCHUNK_IKSOFTLIMIT_MAX		22

IOResult LimbData2::Save(ISave *isave)
{
	DWORD nb;
	IOResult res = IO_OK;

	isave->BeginChunk(LIMBCHUNK_CATCONTROL);
	res = CATControl::Save(isave);
	isave->EndChunk();

	isave->BeginChunk(LIMBCHUNK_LIMBFLAGS);
	isave->Write(&flags, sizeof ULONG, &nb);
	isave->EndChunk();

	//////////////////////////////////////////////////////////////////////////
	// a few parameters for animation

#ifdef SOFTIK
	isave->BeginChunk(LIMBCHUNK_IKSOFTLIMIT_MIN);
	isave->Write(&iksoftlimit_min, sizeof(float), &nb);
	isave->EndChunk();

	isave->BeginChunk(LIMBCHUNK_IKSOFTLIMIT_MAX);
	isave->Write(&iksoftlimit_max, sizeof(float), &nb);
	isave->EndChunk();
#endif

	return IO_OK;
}

IOResult LimbData2::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case LIMBCHUNK_CATCONTROL:
			CATControl::Load(iload);
			break;
		case LIMBCHUNK_LIMBFLAGS:	res = iload->Read(&flags, sizeof(DWORD), &nb);			break;
#ifdef SOFTIK
		case LIMBCHUNK_IKSOFTLIMIT_MIN:		res = iload->Read(&iksoftlimit_min, sizeof(float), &nb);		break;
		case LIMBCHUNK_IKSOFTLIMIT_MAX:		res = iload->Read(&iksoftlimit_max, sizeof(float), &nb);		break;
#endif
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, TRUE);
	iload->RegisterPostLoadCallback(new LimbPLCB(this));
	iload->RegisterPostLoadCallback(new BoneGroupPostLoadIDPatcher(this, PB_LIMBID_DEPRECATED));

	return IO_OK;
}

BOOL LimbData2::LoadRig(CATRigReader *load)
{
	// Note: Flags are loaded by the hub because we need the flags to initialise the limb
	// The only reason
	// we used to have a flag for legs that were in FK
	// during SetupMode. Now we need to convert this flag
	if (((load->GetVersion() < CAT_VERSION_1700) && TestFlag(LIMBFLAG_SETUP_FK)) || GetisArm()) {
		float ikfkratio = 1.0f;
		layerIKFKRatio->SetSetupVal((void*)&ikfkratio);
		ClearFlag(LIMBFLAG_SETUP_FK);
	}

	BOOL done = FALSE;
	BOOL ok = TRUE;

	int boneid = 0;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idLimbParams) &&
				(load->CurGroupID() != idLimb))
				return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idLimbBone: {
				if (boneid >= pblock->GetInt(PB_NUMBONES)) {
					int numSegs = boneid + 1; // 0 based array, 1 based number
					pblock->SetValue(PB_NUMBONES, 0, numSegs);	// Change handler catches this
				}
				BoneData *bone = GetBoneData(boneid);
				DbgAssert(bone);
				ok = bone->LoadRig(load);

				// Apply the twist weights after loading the bone.
//						if (ok) bone->UpdateTwistWeights();
				boneid++;
				break;
			}
			case idPalm: {
				CATNodeControl* palm = CreatePalmAnkle(TRUE);
				ok = palm->LoadRig(load);
				break;
			}
			case idCollarbone: {
				CATNodeControl* collarbone = CreateCollarbone(TRUE);
				ok = collarbone->LoadRig(load);
				break;
			}
			case idUpVectorValues:
				if (!upvectornode) CreateUpNode();
				if (upvectornode) {
					CATNodeControl* ctrlupv = dynamic_cast<CATNodeControl*>(upvectornode->GetTMController());
					if (ctrlupv) ctrlupv->LoadRig(load);
				}
				else load->SkipGroup();
				break;
			case idPlatform:
			case idIKTargetValues: {
				if (!iktarget) CreateIKTarget();

				if (iktarget) {
					CATNodeControl* ctrliktarget = dynamic_cast<CATNodeControl*>(iktarget->GetTMController());
					if (ctrliktarget) ctrliktarget->LoadRig(load);
				}
				else load->SkipGroup();
				break;
			}
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idFlags:
			case idLimbFlags:
			{
				// currently this code is never called. Soon I'll remove any
				// dependance on flags in the Initialise function(isLeg/isArm)
				// and then we can load the flags here

				load->GetValue(flags);
				pblock->SetValue(PB_LIMBFLAGS, 0, (int)flags);

				if (load->GetVersion() < CAT_VERSION_1700) {
					if (GetisArm()) CreateCollarbone();
					if ((GetisArm() && !GetisLeg()) || (GetisLeg() && TestFlag(LIMBFLAG_SETUP_FK))) {
						float ikfk = 1.0f;
						if (layerIKFKRatio) layerIKFKRatio->SetSetupVal((void*)&ikfk);
					}
				}
				if (load->GetVersion() < CAT_VERSION_2100) {
					if (GetisLeg()) SetFlag(LIMBFLAG_FFB_WORLDZ);
				}

				if (GetisLeg()) {
					CreateIKTarget();
				}
				break;
			}
			case idIKFKValue: {
				//As of V1.7 the IKFK controller is stored on the limb.
				float ikfk;
				load->GetValue(ikfk);
				if (layerIKFKRatio) layerIKFKRatio->SetSetupVal((void*)&ikfk);
				break;
			}
			case idBoneName:
				load->ToParamBlock(pblock, PB_NAME);
				if (load->GetVersion() < CAT_VERSION_1720) {
					// New rigs bake the LMR into limbname
					TSTR name = pblock->GetStr(PB_NAME);
					TSTR side = _T("");
					switch (GetLMR()) {
					case -1:	side = _T("L");		break;
					case 0:		side = _T("M");		break;
					case 1:		side = _T("R");		break;
					}
					pblock->SetValue(PB_NAME, 0, side + name);
				}
				break;
			case idBoneColour:
			{
				int intlegColour;
				load->GetValue(intlegColour);
				Color legColour(intlegColour);
				pblock->SetValue(PB_COLOUR, 0, legColour);
				break;
			}
			case idRootPos:
			{
				load->ToParamBlock(pblock, PB_P3ROOTPOS);
				break;
			}

			// for backwards compatability
			case idLeftRight:
			{
				//	load->ToParamBlock(pblock, PB_LEFTRIGHT);
				int nLMR;
				load->GetValue(nLMR);
				SetLMR(nLMR);
				break;
			}
			case idNumBones: {
				int numbones = 1;
				load->GetValue(numbones);
				SetNumBones(numbones, TRUE);

				if (load->GetVersion() < CAT_VERSION_1700) {
					for (int i = 0; i < GetNumBones(); i++) {
						TSTR boneid;
						boneid.printf(_T("%d"), (i + 1));
						GetBoneData(i)->SetName(boneid);
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

	//  Havent figured out why, but CATUnits dosent update
	//	on the palm without this call. Everything else does
	//	though
	return ok && load->ok();
}

BOOL LimbData2::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idLimb);

	Color limbColour = pblock->GetColor(PB_COLOUR);
	DWORD intLimbColour = asRGB(limbColour);

	save->FromParamBlock(pblock, idFlags, PB_LIMBFLAGS);
	save->FromParamBlock(pblock, idBoneName, PB_NAME);

	float ikfk;
	layerIKFKRatio->GetSetupVal((void*)&ikfk);
	save->Write(idIKFKValue, ikfk);
	save->Write(idBoneColour, intLimbColour);

	if (upvectornode) {
		CATNodeControl* ctrlupnode = (CATNodeControl*)upvectornode->GetTMController()->GetInterface(I_CATNODECONTROL);
		if (ctrlupnode) ctrlupnode->SaveRig(save);
	}

	if (iktarget) {
		CATNodeControl* ctrliktarget = (CATNodeControl*)iktarget->GetTMController()->GetInterface(I_CATNODECONTROL);
		if (ctrliktarget) ctrliktarget->SaveRig(save);
	}

	CATNodeControl* collarbone = GetCollarbone();
	if (collarbone) collarbone->SaveRig(save);

	int numbones = pblock->GetInt(PB_NUMBONES);
	save->Write(idNumBones, numbones);
	for (int i = 0; i < numbones; i++)
	{
		BoneData *bone = GetBoneData(i);
		DbgAssert(bone);
		bone->SaveRig(save);
	}

	PalmTrans2* palm = GetPalmTrans();
	if (palm) palm->SaveRig(save);

	save->EndGroup();
	return TRUE;
}

/*
// TODO: enable for CAT2.4
BOOL LimbData2::PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap)
{
	if((flags&PASTELAYERFLAG_MIRROR)||(flags&PASTELAYERFLAG_MIRROR_WORLD_X)||(flags&PASTELAYERFLAG_MIRROR_WORLD_Y)){
		LimbData2 *symlimb = (LimbData2*)GetSymLimb();
		// if we now have a sym limb, have another go at loading it
		if(symlimb)	return symlimb->CatControl::PasteLayer(pastectrl, fromindex, toindex, flags, remap);

		// something went wrong so
		// remove the mirror from the flag, and continue as normal
		flags &= ~PASTELAYERFLAG_MIRROR;
		flags &= ~PASTELAYERFLAG_MIRROR_WORLD_X;
		flags &= ~PASTELAYERFLAG_MIRROR_WORLD_Y;
	}
	return CatControl::PasteLayer(pastectrl, fromindex, toindex, flags, remap);
}
*/

/************************************************************************/
/* Just a short wee function for managing the loading of symmetry limbs */
/************************************************************************/
BOOL LimbData2::LoadClip(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	if (flags&CLIPFLAG_MIRROR)
	{
		LimbData2 *symlimb = (LimbData2*)GetSymLimb();
		// if we now have a sym limb, have another go at loading it
		if (symlimb) {
			// Set this flag so that we don't try to re-swap on a sub part of this limb
			flags |= CLIPFLAG_MIRROR_DATA_SWAPPED;
			return symlimb->LoadClipData(load, timerange, layerindex, dScale, flags);
		}

		// something went wrong so
		// remove the mirror from the flag
		flags = flags & ~CLIPFLAG_MIRROR;
	}
	// we either
	return LoadClipData(load, timerange, layerindex, dScale, flags);
}

/************************************************************************/
/* This function actually does the loading of the clip                  */
/************************************************************************/
BOOL LimbData2::LoadClipData(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	// by default bones are not in world space
	flags &= ~CLIPFLAG_WORLDSPACE;

	int boneid = 0;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idLimbParams) &&
				(load->CurGroupID() != idLimb))return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idGroupWeights: {
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				GetWeights()->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idBendAngleValues: {
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			case idSwivelValues: {
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			case idIKFKValues: {
				//As of V1.7 the IKFK controller is stored on the limb.
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				if (layerIKFKRatio) layerIKFKRatio->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idLimbIKPos: {
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				if (layerLimbIKPos) layerLimbIKPos->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idRetargetting: {
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				if (layerRetargeting) layerRetargeting->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idUpVectorValues:
				if (!upvectornode) CreateUpNode();
				if (upvectornode) {
					CATNodeControl* ctrlupv = dynamic_cast<CATNodeControl*>(upvectornode->GetTMController());
					if (ctrlupv) ctrlupv->LoadClip(load, timerange, layerindex, dScale, flags);
				}
				else load->SkipGroup();
				break;
			case idIKTargetValues:
				if (!iktarget) CreateIKTarget();
				if (iktarget) {
					CATNodeControl* ctrliktarget = dynamic_cast<CATNodeControl*>(iktarget->GetTMController());
					if (ctrliktarget) ctrliktarget->LoadClip(load, timerange, layerindex, dScale, flags);
					//	((CATNodeControl*)iktarget->GetTMController())->LoadClip(load, timerange, layerindex, dScale, flags);
				}
				else load->SkipGroup();
				break;
			case idLimbBone: {
				if (boneid >= GetNumBones()) {
					load->SkipGroup();
				}
				else {
					BoneData *ctrlBone = GetBoneData(boneid);
					if (ctrlBone)	ok = ctrlBone->LoadClip(load, timerange, layerindex, dScale, flags);
					else			load->SkipGroup();
					boneid++;
					break;
				}
			}
			case idPalm: {
				PalmTrans2* palm = GetPalmTrans();
				if (palm)		ok = palm->LoadClip(load, timerange, layerindex, dScale, flags);
				else			load->SkipGroup();
				break;
			}
			case idCollarbone: {
				CATNodeControl* collarbone = GetCollarbone();
				if (collarbone)	ok = collarbone->LoadClip(load, timerange, layerindex, dScale, flags);
				else			load->SkipGroup();
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
			case idLimbFlags: {
				// if the limb this clip was saved from was on the
				// opposite side of the body then mirror the data
				int filelimbflags;
				load->GetValue(filelimbflags);
				int lmr = (filelimbflags&LIMBFLAG_LEFT) ? -1 : ((filelimbflags&LIMBFLAG_RIGHT) ? 1 : 0);
				if ((lmr + GetLMR()) == 0) {
					flags |= CLIPFLAG_MIRROR;
					flags |= CLIPFLAG_MIRROR_DATA_SWAPPED;
				}
			}
			break;
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

BOOL LimbData2::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(idLimb);

	save->Comment(GetName());

	// if we know what kind of limb saved the clip, then we can automatically
	save->FromParamBlock(pblock, idLimbFlags, PB_LIMBFLAGS);

	if (flags&CLIPFLAG_CLIP) {
		save->BeginGroup(idGroupWeights);
		GetWeights()->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	if (layerIKFKRatio) {
		save->BeginGroup(idIKFKValues);
		layerIKFKRatio->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	if (layerLimbIKPos) {
		save->BeginGroup(idLimbIKPos);
		layerLimbIKPos->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	if (layerRetargeting) {
		save->BeginGroup(idRetargetting);
		layerRetargeting->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	if (upvectornode) {
		((CATNodeControl*)upvectornode->GetTMController())->SaveClip(save, flags, timerange, layerindex);
	}
	if (iktarget) {
		((CATNodeControl*)iktarget->GetTMController())->SaveClip(save, flags, timerange, layerindex);
	}
	else if (layerIKTargetTrans)
	{
		save->BeginGroup(idIKTargetValues);
		// if we are saving out a pose then we can
		// just save out the world space matrix for this bone
		if (!(flags&CLIPFLAG_CLIP)) {
			Interval iv = FOREVER;
			mtmIKTarget.IdentityMatrix();
			layerIKTargetTrans->GetValue(timerange.Start(), (void*)&mtmIKTarget, iv, CTRL_RELATIVE);
			save->Write(idValMatrix3, mtmIKTarget);
		}
		else {
			save->BeginGroup(idController);
			layerIKTargetTrans->SaveClip(save, flags, timerange, layerindex);
			save->EndGroup();
		}
		save->EndGroup();
	}

	CATNodeControl* collarbone = GetCollarbone();
	if (collarbone) collarbone->SaveClip(save, flags, timerange, layerindex);

	int numbones = GetNumBones();
	BoneData *bone;
	for (int i = 0; i < numbones; i++)
	{
		bone = GetBoneData(i);
		DbgAssert(bone);
		bone->SaveClip(save, flags, timerange, layerindex);
	}

	PalmTrans2* palm = GetPalmTrans();
	if (palm) palm->SaveClip(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

void LimbData2::SetNumBones(int i, BOOL loading/*=TRUE*/)
{
	if (i == pblock->Count(PB_BONEDATATAB)) return;
	// when we are deleting limb bones, we may be forced to
	// deselect the deleting bone and reselect bone 0.
	// This happens when the selected boneid is > than the new num bones.
	// When we re-select a new bone, the UI gets refreshed, and then the numbones spinner
	// sends out another change message and we find outselves back in this function.
	//We should bail if the bone counter has already been updated.
	if (i == pblock->GetInt(PB_NUMBONES)) return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	//if(theHold.Holding()) theHold.Put(new SetNumBonesRestoreUndo(this));

	pblock->EnableNotifications(FALSE);
	pblock->SetValue(PB_NUMBONES, 0, i);
	pblock->EnableNotifications(TRUE);
	CreateLimbBones(loading);

	//if(theHold.Holding()) theHold.Put(new SetNumBonesRestoreRedo(this));
	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_NUMBONES));
	}
}

void LimbData2::CreateLimbBones(BOOL loading)
{
	if (theHold.RestoreOrRedoing()) return;
	DbgAssert(!IsEvaluationBlocked());

	int j;
	int oldNumBones = pblock->Count(PB_BONEDATATAB);
	int newNumBones = pblock->GetInt(PB_NUMBONES);
	if (newNumBones < 1)
	{
		newNumBones = 1;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(PB_NUMBONES, 0, 1);
		pblock->EnableNotifications(TRUE);
	}
	if (newNumBones == oldNumBones)
		return;

	// Scope our evaluation block
	{
		BlockEvaluation block(this);
		SetCCFlag(CNCFLAG_KEEP_ROLLOUTS);
		BoneData* bone;

		float oldLimbLength = 0.0f;

		//	std::vector<float> BoneLengths(oldNumBones);
		for (j = 0; j < oldNumBones; j++)
		{
			// Get the bone's length
			bone = GetBoneData(j);
			oldLimbLength += bone->GetBoneLength();
		}

		if (newNumBones < oldNumBones)
		{
			// count down, it makes so much more sense
			// If newNumBones < oldNumBones, delete some,
			// But only if we are not in an undo
			for (int j = (oldNumBones - 1); j >= newNumBones; j--)
			{
				GetBoneData(j)->DeleteBone();
				pblock->SetCount(PB_BONEDATATAB, j);
			}
			pblock->SetCount(PB_BONEDATATAB, newNumBones);
		}
		else
		{
			pblock->SetCount(PB_BONEDATATAB, newNumBones);

			// What is the parent of this bone?
			CATNodeControl* pParent = NULL;
			if (oldNumBones != 0) pParent = GetBoneData(oldNumBones - 1);
			else if (GetCollarbone() != NULL) pParent = GetCollarbone();
			else pParent = GetHub();

			for (j = oldNumBones; j < newNumBones; j++)
			{
				bone = (BoneData*)CreateInstance(CTRL_MATRIX3_CLASS_ID, BONEDATA_CLASS_ID);
				DbgAssert(bone != NULL);

				// we are going to 0 based Bone IDs now from version 1.7 onwards
				bone->Initialise(this, j, loading);
				pblock->SetValue(PB_BONEDATATAB, 0, (ReferenceTarget*)bone, j);

				pParent = bone;
			}
		}

		// make sure the layerLimbIKPos defaults to the end of the limb
		float ikpos = (float)newNumBones;
		PalmTrans2* palm = GetPalmTrans();
		if (palm) ikpos += 1.0f;
		if (layerLimbIKPos)
			layerLimbIKPos->SetSetupVal((void*)&ikpos);
		/////////////////////////////////////////////
		// Finished making bones

		// re-enable this method calling
		ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);
	} // End evaluation lock

	if (!loading) {
		CATMessage(0, CAT_CATUNITS_CHANGED);
		UpdateLimb();
	}
}

void LimbData2::SetLMR(int nLMR)
{
	switch (nLMR) {
	case -1:
		SetFlag(LIMBFLAG_LEFT);
		ClearFlag(LIMBFLAG_MIDDLE);
		ClearFlag(LIMBFLAG_RIGHT);
		break;
	case 0:
		ClearFlag(LIMBFLAG_LEFT);
		SetFlag(LIMBFLAG_MIDDLE);
		ClearFlag(LIMBFLAG_RIGHT);
		break;
	case 1:
		ClearFlag(LIMBFLAG_LEFT);
		ClearFlag(LIMBFLAG_MIDDLE);
		SetFlag(LIMBFLAG_RIGHT);
		break;
	}
};

#ifdef SOFTIK

void	LimbData2::SetIKSoftLimitMin(float val)
{
	iksoftlimit_min = val;
	if (val > iksoftlimit_max) iksoftlimit_max = val;
	UpdateLimb();
	UpdateUI();
};

void	LimbData2::SetIKSoftLimitMax(float val)
{
	iksoftlimit_max = val;
	if (val < iksoftlimit_min) iksoftlimit_min = val;
	UpdateLimb();
	UpdateUI();
};

float LimbData2::GetDistToTarg(TimeValue t)
{
	GetBone(0)->Evaluate(t);

	Matrix3 tmIKTarget, tmFKTarget;
	GetIKTMs(t, tmIKTarget, tmFKTarget);

	Point3 rootpos = GetBone(0)->GetFKTM().GetTrans();
	Point3 ik_limb_vec = rootpos - tmIKTarget.GetTrans();

	return Length(ik_limb_vec);
}

#endif

// Return the length between the Root and the ankle, calculated using Ratios and BendAngles
// Used for Root repositioning. This function does NOT represent the state of the Limb. The Limb
// can be in IK, KA, or FK even, and this will only return the height of the lift AS IF the
// system was fully in KA lifting mode
Ray LimbData2::GetReachVec(TimeValue t, Interval& valid, const Matrix3& tmHub, const Point3& p3HubScale, float &weight)
{
	UNREFERENCED_PARAMETER(p3HubScale); UNREFERENCED_PARAMETER(tmHub);
	Ray ray;
	float ikfkratio = GetIKFKRatio(t, valid);
	weight = GetForceFeedback(t, valid) * (1.0f - ikfkratio);
#ifndef SOFTIK
	if (ikfkratio >= 1.0f || weight <= 0.0f) {
#else
	if (ikfkratio >= 1.0f || (weight <= 0.0f && !TestFlag(LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS))) {
#endif
		ray.p = P3_IDENTITY;
		ray.dir = ray.p;
		weight = 0.0f;
		return ray;
	}

	Matrix3 tmParent = tmHub;
	CATNodeControl* pCollarbone = GetCollarbone();
	if (pCollarbone != NULL)
		pCollarbone->GetValue(t, &tmParent, valid, CTRL_RELATIVE);

	Matrix3 tmPalmLocalFK;
	Point3 p3FKTarget = GettmFKTarget(t, valid, &tmParent, tmPalmLocalFK).GetTrans();
	Point3 p3IKTarget = GettmPalmTarget(t, valid, tmPalmLocalFK).GetTrans();

	Point3 rootpos = GetBoneFKPosition(t, 0);
	Point3 ik_limb_vec = rootpos - p3IKTarget;
	float curr_limblength = Length(ik_limb_vec);
	float fk_limblength = Length(rootpos - p3FKTarget);

#ifdef SOFTIK
	if (TestFlag(LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS)) {
		// If ramp is on then as the iktarget goes out of reach,
		// then we turn on retargetting so we lean forwards to reach for it.
		float reachramp = min(1.0f, max((curr_limblength - iksoftlimit_min) / (iksoftlimit_max - iksoftlimit_min), 0.0f));
		fk_limblength = BlendFloat(BlendFloat(iksoftlimit_min, iksoftlimit_max, reachramp), fk_limblength, weight);
		weight = BlendFloat(weight, 1.0f, reachramp * (1.0f - ikfkratio));
	}
#endif

	if (TestFlag(LIMBFLAG_FFB_WORLDZ)) {
		ray.p = rootpos;
		ray.dir = P3_UP_VEC;
		RaySphereCollision(ray, p3IKTarget, fk_limblength);
		ray.dir = (ray.p - rootpos);
		PalmTrans2* palm = GetPalmTrans();
		if (palm && GetisLeg())
		{
			// PT 04-12-05
			// This will bias the lifting to the leg that is planting
			weight *= palm->GetTargetAlign(t, valid);
			if (rootpos[Z] < mtmIKTarget.GetTrans()[Z])
				weight = 0.0f;
			else {
				// PT 26-8-06
				// This will turn off retargetting if the footis above the hips. i.e. Rolling
				float dotprod = DotProd(Normalize(ray.p - p3IKTarget), P3_UP_VEC);
				dotprod *= dotprod;
				ray.dir *= dotprod;
			}
		}
		ray.dir *= weight;
	}
	else {
		ray.p = rootpos;
		ray.dir = Normalize(ik_limb_vec) * ((fk_limblength - curr_limblength) * weight);
	}

	// here we could make it so that the force feedback system only pushes or pulls.
//	if(DotProd(ray.dir, ik_limb_vec)>0.0f){	// pushing
//		if(!pushing) ray.dir = Point3(0.0f, 0.0f, 0.0f);
//	}else{									// pulling
//		if(!pulling) ray.dir = Point3(0.0f, 0.0f, 0.0f);
//	}
	return ray;
	}

/*
 *	Called by the limbs creator, telling the limb
 *	how it should treat end points collarbone/ankle
 */
void LimbData2::SetIsArmIsLeg(BOOL isArm, BOOL isLeg)
{
	SetisArm(isArm);
	SetisArm(isLeg);
}

void LimbData2::SetLeftRight(Matrix3 &inVal)
{
	inVal.SetRow(0, (inVal.GetRow(0) * (float)GetLMR()));
}

void LimbData2::SetLeftRightRot(Point3 &inVal)
{
	if (GetLengthAxis() == Z) {
		inVal[Y] *= GetLMR();
		inVal[Z] *= GetLMR();
	}
	else {
		inVal[X] *= GetLMR();
		inVal[Y] *= GetLMR();
	}
}
void LimbData2::SetLeftRightPos(Point3 &inVal)
{
	if (GetLengthAxis() == Z)
		inVal[X] *= GetLMR();
	else inVal[Z] *= GetLMR();
};

void LimbData2::SetLeftRight(Quat &inVal)
{
	//	if(GetLMR() == 0)return;
	//	AngAxis inValAA(inVal);
	//	SetLeftRightRot(inValAA.axis);
	//	inVal = Quat(inValAA);

	if (GetLengthAxis() == Z) {
		if (GetLMR() == -1) {
			inVal.y *= -1.0f;
			inVal.z *= -1.0f;
		}
	}
	else {
		if (GetLMR() == -1) {
			inVal.y *= -1.0f;
			inVal.z *= -1.0f;
		}/* else{
			inVal.x *= -1.0f;
			inVal.z *= -1.0f;
		}*/
	}
};

void LimbData2::SetLeftRight(float &inVal)
{
	inVal *= GetLMR();
};

CATNodeControl* LimbData2::CreateCollarbone(BOOL loading)
{
	CATNodeControl* pExistingCollar = GetCollarbone();
	if (pExistingCollar != NULL)
		return pExistingCollar;

	CollarBoneTrans* pNewCollar = (CollarBoneTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, COLLARBONETRANS_CLASS_ID);
	pNewCollar->Initialise(this, loading);
	pblock->SetValue(PB_CTRLCOLLARBONE, 0, pNewCollar);

	return pNewCollar;
}

CatAPI::INodeControl* LimbData2::CreateCollarbone()
{
	return CreateCollarbone(FALSE);
}

BOOL LimbData2::RemoveCollarbone()
{
	// we must do the collarbone first because if there isn't one
	// then our last reference will have already been removed and we will die
	// a nasty horrible death trying to kill the collarbone
	CATNodeControl* collarbone = GetCollarbone();
	bool bSuccess = true;
	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();

	if (collarbone)
	{
		BlockEvaluation block(this);

		ip->DisableSceneRedraw();
		if (collarbone->IsThisBoneSelected()) ip->DeSelectNode(collarbone->GetNode());

		if (GetNumBones() != 0)
		{
			BoneData* bone = GetBoneData(0);
			if (bone)
			{

				// this bone does not need a positionally offset any more
				Matrix3 tmCollarboneSetup = collarbone->GetSetupMatrix();
				Matrix3 tmBone0Setup = bone->GetSetupMatrix();

				ICATObject* iobj = collarbone->GetICATObject();
				if (GetLengthAxis() == Z)
					tmCollarboneSetup.PreTranslate(Point3(0.0f, 0.0f, iobj->GetZ()));
				else tmCollarboneSetup.PreTranslate(Point3(iobj->GetZ(), 0.0f, 0.0f));

				if (GetLMR() == 1)
					tmBone0Setup.SetTrans(tmCollarboneSetup.GetTrans()*RotateYMatrix((float)-M_PI_2));
				else tmBone0Setup.SetTrans(tmCollarboneSetup.GetTrans()*RotateYMatrix((float)M_PI_2));

				// fix up any layers that have been assigned too
				Point3 pos = tmBone0Setup.GetTrans() * GetCATUnits();
				for (int i = 0; i < bone->GetLayerTrans()->GetNumLayers(); i++)
				{
					if (bone->GetLayerTrans()->GetLayerMethod(i) == LAYER_ABSOLUTE &&
						bone->GetLayerTrans()->GetLayer(i)->GetPositionController()) {
						bone->GetLayerTrans()->GetLayer(i)->GetPositionController()->SetValue(0, (void*)&pos, 1, CTRL_ABSOLUTE);
					}
				}
				bone->SetSetupMatrix(tmBone0Setup);
				bone->ClearCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
			}
		}
		INodeTab nodes;
		collarbone->AddSystemNodes(nodes, kSNCDelete);
		for (int i = 0; i < nodes.Count(); i++)	nodes[i]->Delete(0, FALSE);
		collarbone = NULL;
		pblock->SetValue(PB_CTRLCOLLARBONE, 0, (ReferenceTarget*)NULL);

		ip->EnableSceneRedraw();
	}

	UpdateLimb();
	ip->RedrawViews(t);

	return bSuccess;
}

CATNodeControl* LimbData2::CreatePalmAnkle(BOOL loading)
{
	// No double create, thank you!
	PalmTrans2* palm = GetPalmTrans();
	if (palm != NULL)
		return palm;

	HoldActions hold(IDS_HLD_PALMADD);
	// create the palm

	// Stop the limb from updating for a sec
	{
		BlockEvaluation block(this);
		palm = (PalmTrans2*)CreateInstance(CTRL_MATRIX3_CLASS_ID, PALMTRANS2_CLASS_ID);
		palm->Initialise(this, loading);

		pblock->SetValue(PB_CTRLPALM, 0, (ReferenceTarget*)palm);

		// make sure the layerLimbIKPos defaults to the end of the limb
		float ikpos = (float)(GetNumBones() + 1);
		layerLimbIKPos->SetSetupVal((void*)&ikpos);
	}
	return palm;
}

CatAPI::INodeControl* LimbData2::CreatePalmAnkle()
{
	return CreatePalmAnkle(FALSE);
}

BOOL LimbData2::RemovePalmAnkle()
{
	PalmTrans2* palm = GetPalmTrans();
	if (!palm)
		return true;
	// start at the leaves and work up
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();  newundo = TRUE; }

	Interface *ip = GetCOREInterface();
	if (palm->IsThisBoneSelected())
		ip->DeSelectNode(palm->GetNode());

	ip->DisableSceneRedraw();
	DisableRefMsgs();

	INodeTab nodes;
	palm->AddSystemNodes(nodes, kSNCDelete);
	for (int i = (nodes.Count() - 1); i >= 0; i--)	nodes[i]->Delete(0, FALSE);
	pblock->SetValue(PB_CTRLPALM, 0, (ReferenceTarget*)NULL);
	palm = NULL;

	EnableRefMsgs();

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_PALMREM));
	}

	UpdateLimb();
	ip->EnableSceneRedraw();

	return true;
}

CATNodeControl* LimbData2::GetCollarbone()
{
	return static_cast<CATNodeControl*>(pblock->GetReferenceTarget(PB_CTRLCOLLARBONE));
}

PalmTrans2* LimbData2::GetPalmTrans()
{
	return static_cast<PalmTrans2*>(pblock->GetReferenceTarget(PB_CTRLPALM));
}

void LimbData2::SetPalmTrans(PalmTrans2* pPalm)
{
	pblock->SetValue(PB_CTRLPALM, 0, pPalm);
}

BoneData* LimbData2::GetBoneData(int boneid)
{
	if (boneid >= GetNumBones() || boneid < 0)
		return NULL;

	BoneData *pBone = static_cast<BoneData*>(pblock->GetReferenceTarget(PB_BONEDATATAB, 0, boneid));
	return pBone;
}

CatAPI::INodeControl*		LimbData2::GetBoneINodeControl(int boneid)
{
	return GetBoneData(boneid);
}

CatAPI::INodeControl*		LimbData2::GetPalmAnkleINodeControl()
{
	return GetPalmTrans();
};
CatAPI::INodeControl*		LimbData2::GetCollarboneINodeControl()
{
	return GetCollarbone();
};

CatAPI::IHub* LimbData2::GetIHub()
{
	ReferenceTarget* pHub = pblock->GetReferenceTarget(PB_HUB);
	if (pHub != NULL)
		return GetIHubInterface(pHub);
	return NULL;
}

Hub* LimbData2::GetHub()
{
	return static_cast<Hub*>(pblock->GetReferenceTarget(PB_HUB));
}

void LimbData2::SetHub(Hub* h)
{
	pblock->SetValue(PB_HUB, 0, h);
}

void LimbData2::Initialise(Hub *hub, int id, BOOL loading, ULONG flags)
{
	if (!hub) return;

	// Block evalutating while creating
	{
		BlockEvaluation block(this);

		this->flags = flags;
		pblock->SetValue(PB_LIMBFLAGS, 0, (int)flags);

		// Set the Limb ID
		SetLimbID(id);
		pblock->SetValue(PB_HUB, 0, hub);

		CATClipWeights* newweights = CreateClipWeightsController((CATClipWeights*)NULL, GetCATParentTrans(), FALSE);
		CATClipValue* layerFFeedback = CreateClipValueController(GetCATClipFloatDesc(), newweights, GetCATParentTrans(), FALSE);
		CATClipValue* layerIKFK = CreateClipValueController(GetCATClipFloatDesc(), newweights, GetCATParentTrans(), FALSE);
		CATClipValue* layerIKPos = CreateClipValueController(GetCATClipFloatDesc(), newweights, GetCATParentTrans(), FALSE);

		ReplaceReference(REF_WEIGHTS, newweights);
		ReplaceReference(REF_LAYERRETARGETING, layerFFeedback);
		ReplaceReference(REF_LAYERIKFKRATIO, layerIKFK);
		ReplaceReference(REF_LAYERLIMBIKPOS, layerIKPos);

#ifdef SOFTIK
		Control* ctrlSoftIKWeight = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATWEIGHT_CLASS_ID);
		pblock->SetControllerByID(PB_SOFTIKWEIGHT, 0, ctrlSoftIKWeight, FALSE);
#endif
	}

	// if we are not loading data then we'd better make up some defaults
	if (!loading)
	{
		TSTR name;
		if (GetisArm()) {
			SetFlag(LIMBFLAG_ISARM);
			name = GetString(IDS_ARM);
		}
		if (GetisLeg()) {
			SetFlag(LIMBFLAG_ISLEG);
			name = GetString(IDS_LEG);
		}
		TSTR side = _T("");
		switch (GetLMR()) {
		case -1:	side = _T("L");		break;
		case 0:		side = _T("M");		break;
		case 1:		side = _T("R");		break;
		}
		pblock->SetValue(PB_NAME, 0, side + name);

		ILimb *symlimb = GetSymLimb();
		if (symlimb)
		{
			PasteRig(static_cast<LimbData2*>(symlimb), PASTERIGFLAG_MIRROR, 1.0f);
		}
		else
		{
			if (GetisArm()) {
				float dDefaultVal = 1.0f;
				layerIKFKRatio->SetSetupVal((void*)&dDefaultVal);

				CreateCollarbone();
				CreatePalmAnkle();
			}
			if (GetisLeg()) {
				// default legs to IK
				float dDefaultVal = 0.0f;
				layerIKFKRatio->SetSetupVal((void*)&dDefaultVal);

				CreatePalmAnkle();
				CreateIKTarget();
			}
			// The number of bones in the limb may now be set.  For
			// both arms and legs, this value is 2.
			SetNumBones(2, loading);
		}
	}
	else
	{
		// We must create at least 1 bone to stop the
		// limb data being deleted by having no refs to it
		SetNumBones(1, loading);
	}
}

void LimbData2::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	// If a Limb UI is open and we undo a delete, this flag has not been cleared yet.
	if (TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) return;

	flagsbegin = flags;
	ipbegin = ip;

	if (flags&BEGIN_EDIT_MOTION) {
		TSTR limbmotionrolloutname = GetString(IDS_LIMB_ANIM);
		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_LIMB_MOTION_CAT3), LimbMotionRolloutProc, limbmotionrolloutname, (LPARAM)this, ROLLUP_SAVECAT, 3);
	}
	else if (flags&BEGIN_EDIT_LINKINFO)
		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_LIMBDATA_HIERARCHY), LimbHierarchyProc, GetString(IDS_LIMB_HIER), (LPARAM)this, ROLLUP_SAVECAT, 3);
	else if (flagsbegin == 0) {
		LimbData2Desc.BeginEditParams(ip, this, flags, prev);
	}
}

void LimbData2::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(next);
	// If a Limb UI is open and we undo a delete, this flag has not been cleared yet.
	if (TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) return;

	ipbegin = NULL;

	if (flagsbegin&BEGIN_EDIT_MOTION) {
		if (staticLimbMotionRollout.GetHWnd()) {
			DbgAssert(staticLimbMotionRollout.GetLimb() == this);
			ip->DeleteRollupPage(staticLimbMotionRollout.GetHWnd());
		}
	}
	else if (flagsbegin&BEGIN_EDIT_LINKINFO) {
		if (LimbHierarchyCallBack.GetHWnd() && LimbHierarchyCallBack.IsDlgOpen()) {
			ip->DeleteRollupPage(LimbHierarchyCallBack.GetHWnd());
		}
	}
	else if (flagsbegin == 0) {
		LimbData2Desc.EndEditParams(ip, this, flags, NULL); // next);
	}
}

void LimbData2::UpdateUI() {
	if (!ipbegin) return;
	if (flagsbegin&BEGIN_EDIT_MOTION && staticLimbMotionRollout.GetHWnd()) {
		staticLimbMotionRollout.TimeChanged(GetCOREInterface()->GetTime());
	}
	else if (flagsbegin&BEGIN_EDIT_LINKINFO && LimbHierarchyCallBack.IsDlgOpen())
		LimbHierarchyCallBack.Update();
	else if (flagsbegin == 0) {
		IParamMap2* pmap = pblock->GetMap();
		if (pmap) pmap->Invalidate();
	}
}

void LimbData2::KeyFreeform(TimeValue t, ULONG flags)
{
	UNREFERENCED_PARAMETER(flags);
	// if we are locked in IK mode then we don't need to key any of this stuff
	if (TestFlag(LIMBFLAG_LOCKED_IK)) return;

	Matrix3 tmParent;
	float dIKFKRatio = 0.0f;
	Interval ivDontCare;

	// If the IKTarget exists, it will be keyed automatically.
	// Otherwise, we need to set the IK Controller directly
	// NOTE - This action needs to be done first,
	// otherwise, the FK solution will evaluate
	if (GetIKTarget() == NULL && layerIKTargetTrans != NULL)
	{
		// Just get and set the value;
		SetXFormPacket packet;
		packet.tmParent.IdentityMatrix();
		packet.tmAxis.IdentityMatrix();
		packet.command = XFORM_SET;
		{
			// Lock reference messages to prevent this from invalidating our IK
			MaxReferenceMsgLock lockThis;
			layerIKTargetTrans->GetValue(t, &packet.tmParent, &packet.tmAxis, ivDontCare, CTRL_RELATIVE);
			KeyFreeformMode SetMode(layerIKTargetTrans);
			layerIKTargetTrans->SetValue(t, &packet);
		}
	}

	if (layerIKFKRatio) {
		dIKFKRatio = GetIKFKRatio(t, ivDontCare);
		KeyFreeformMode SetMode(layerIKFKRatio);
		layerIKFKRatio->SetValue(t, (void*)&dIKFKRatio, TRUE, CTRL_ABSOLUTE);
	}
	if (layerLimbIKPos) {
		float ikpos = GetLimbIKPos(t, ivDontCare);
		KeyFreeformMode SetMode(layerLimbIKPos);
		layerLimbIKPos->SetValue(t, (void*)&ikpos, TRUE, CTRL_ABSOLUTE);
	}
	if (layerRetargeting) {
		float retargetting = GetForceFeedback(t, ivDontCare);
		KeyFreeformMode SetMode(layerRetargeting);
		layerRetargeting->SetValue(t, (void*)&retargetting, TRUE, CTRL_ABSOLUTE);
	}
}

void LimbData2::KeyLimbBones(TimeValue t, bool bKeyFreeform/*=true*/)
{
	int nBones = GetNumBones();
	Tab<Matrix3> dBoneTMs;
	dBoneTMs.SetCount(nBones);

	// Because IK bones are dependent
	// on their FK rotation for the IK
	// solution, we need to bake transforms
	// in a reverse order.  This ensures
	// that children FK is baked before
	// the parents FK changes (potentially
	// changing the childs IK solution).
	for (int i = 0; i < nBones; i++)
	{
		BoneData* pBone = GetBoneData(i);
		if (pBone != NULL)
			dBoneTMs[i] = pBone->GetNodeTM(t);
	}

	int iCATMode = GetCATMode();

	// Ignore the IK values for a moment...
	SetFlag(LIMBFLAG_LOCKED_FK);
	for (int i = 0; i < nBones; i++)
	{
		BoneData* pBone = GetBoneData(i);
		if (pBone != NULL)
		{
			// If we are not in setup mode, then make
			// sure we apply the value in KeyFreeformMode
			if (iCATMode != SETUPMODE && bKeyFreeform)
			{
				KeyFreeformMode kf(pBone->GetLayerTrans());
				pBone->SetNodeTM(t, dBoneTMs[i]);

				// Because we have KeyFreeformMode enabled,
				// Notify messages are suppressed and
				// our SetValue has not invalidated the FK
				// transform.  Explicitly invalidate it here.
				// This has proved necessary for baking animation
				// to layers.
				pBone->CATMessage(t, CAT_UPDATE);
			}
			else
				pBone->SetNodeTM(t, dBoneTMs[i]);
		}
	}
	ClearFlag(LIMBFLAG_LOCKED_FK);
}

CatAPI::ILimb* LimbData2::GetSymLimb()
{
	// If no sym limb has been assigned then try this
	// we get a limb of the hub which could be our sym-limb
	// the hub will check these limbids before returning a limb
	// 0->1 1->0 2->3 3->2
	ILimb *symlimb = NULL;
	int limb_id = GetLimbID();
	if ((limb_id % 2) == 0)	symlimb = GetHub()->GetLimb(limb_id + 1);
	if ((limb_id % 2) == 1)	symlimb = GetHub()->GetLimb(limb_id - 1);

	if (symlimb && ((GetLMR() + symlimb->GetLMR()) == 0) &&
		(GetisLeg() == symlimb->GetisLeg()) &&
		(GetisArm() == symlimb->GetisArm())) {
		return symlimb;
	}
	return NULL;
}

BOOL LimbData2::PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap)
{
	DbgAssert(ClassID() == pastectrl->ClassID());
	LimbData2* pastelimb = (LimbData2*)pastectrl;

	// Groom this limb, to be close as possible to the incoming one.
	if (pastelimb->GetUpNode() && !GetUpNode())
	{
		CreateUpNode();
	}
	if (pastelimb->GetIKTarget() != NULL)
	{
		// If paste has one, we want one too.
		if (GetIKTarget() == NULL)
			CreateIKTarget();
	}
	else if (GetIKTarget() != NULL)
	{
		// If we have an IK target, the layerIKTrans will not be returned
		// from GetLayerController, and we will need to update it manually
		CATClipValue* layerCtrl = ((CATNodeControl*)iktarget->GetTMController()->GetInterface(I_CATNODECONTROL))->GetLayerTrans();
		CATClipValue* pastelayerCtrl = pastelimb->layerIKTargetTrans;

		if (layerCtrl != NULL && pastelayerCtrl != NULL)
			layerCtrl->PasteLayer(pastelimb->layerIKTargetTrans, fromindex, toindex, flags, remap);
	}

	return CATControl::PasteLayer(pastectrl, fromindex, toindex, flags, remap);

	// some bones may have more or less arbitrary bones attached, and so we can only paste the minimum
	/*int numchildcontrols = NumChildCATControls();
	for(i = 0; i <numchildcontrols; i++){
		CATControl* child = GetChildCATControl(i);
		CATControl* pastechild = pastelimb->GetChildCATControl(i);
		if(child && !child->TestAFlag(A_IS_DELETED) && pastechild && !pastechild->TestAFlag(A_IS_DELETED))
			child->PasteLayer(pastechild, fromindex, toindex, flags, remap);
	}

	{
		if(iktarget && pastelimb->GetIKTarget()){
			((CATNodeControl*)iktarget->GetTMController()->GetInterface(I_CATNODECONTROL))->PasteLayer(((CATNodeControl*)pastelimb->GetIKTarget()->GetTMController()->GetInterface(I_CATNODECONTROL)), fromindex, toindex, flags, remap);
		}else{
			CATClipValue* layerCtrl;
			CATClipValue* pastelayerCtrl;
			if(iktarget)	layerCtrl = ((CATNodeControl*)iktarget->GetTMController()->GetInterface(I_CATNODECONTROL))->GetLayerTrans();
			else			layerCtrl = layerIKTargetTrans;
			if(pastelimb->GetIKTarget())
				pastelayerCtrl = ((CATNodeControl*)pastelimb->GetIKTarget()->GetTMController()->GetInterface(I_CATNODECONTROL))->GetLayerTrans();
			else pastelayerCtrl = pastelimb->layerIKTargetTrans;
			if(layerCtrl && pastelayerCtrl){
				layerCtrl->PasteLayer(pastelayerCtrl, fromindex, toindex, flags, remap);
			}
		}
	}
	return TRUE;*/
}

BOOL LimbData2::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	LimbData2* pastelimb = dynamic_cast<LimbData2*>(pastectrl);
	DbgAssert(pastelimb != NULL);

	BOOL firstpastedbone = FALSE;
	if (!(flags&PASTERIGFLAG_FIRST_PASTED_BONE)) {
		flags |= PASTERIGFLAG_FIRST_PASTED_BONE;
		firstpastedbone = TRUE;
	}

	//	int lmr = pastelimb->GetLMR();
	//	if(flags&PASTERIGFLAG_MIRROR) lmr *= -1;
	//	SetLMR(lmr);
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	pblock->EnableNotifications(FALSE);

	SetFlag(LIMBFLAG_FFB_WORLDZ, pastelimb->TestFlag(LIMBFLAG_FFB_WORLDZ));
	pblock->EnableNotifications(TRUE);

	int numbones = GetNumBones();
	int pasteNumBones = pastelimb->GetNumBones();
	//	Force the number of bones to be the same
	if (numbones != pasteNumBones) {
		SetNumBones(pasteNumBones, TRUE);
	}
	DbgAssert(GetNumBones() == pasteNumBones);

	// make sure both limbs have the same confiuration
	CATNodeControl* collarbone = GetCollarbone();
	if (pastelimb->GetCollarbone() && !collarbone)
		CreateCollarbone(TRUE);
	else if (!pastelimb->GetCollarbone() && collarbone)
		RemoveCollarbone();

	PalmTrans2* palm = GetPalmTrans();
	if (pastelimb->GetPalmTrans() && !palm)
		CreatePalmAnkle(TRUE);
	else if (!pastelimb->GetPalmTrans() && palm)
		RemovePalmAnkle();

	if (pastelimb->GetIKTarget() && !iktarget)		CreateIKTarget(TRUE);
	else if (pastelimb->GetIKTarget() && !iktarget)	RemoveIKTarget();

	if (pastelimb->GetUpNode() && !upvectornode)		CreateUpNode(TRUE);
	else if (pastelimb->GetUpNode() && !upvectornode)	RemoveUpNode();

	// We Can't Use CATControl to do any pasting here cause we
	// need to manually specify which controllers get mirrored
	float val = 0.0f;

	pastelimb->layerRetargeting->GetSetupVal((void*)&val);
	layerRetargeting->SetSetupVal((void*)&val);

	pastelimb->layerIKFKRatio->GetSetupVal((void*)&val);
	layerIKFKRatio->SetSetupVal((void*)&val);

	if (layerLimbIKPos) {
		pastelimb->layerLimbIKPos->GetSetupVal((void*)&val);
		layerLimbIKPos->SetSetupVal((void*)&val);
	}

	if (layerIKTargetTrans && pastelimb->layerIKTargetTrans) {
		Matrix3 tmSetup(1);
		pastelimb->layerIKTargetTrans->GetSetupVal((void*)&tmSetup);
		if (flags&PASTERIGFLAG_MIRROR) {
			if (GetLengthAxis() == Z)	MirrorMatrix(tmSetup, kXAxis);
			else									MirrorMatrix(tmSetup, kZAxis);
		}
		tmSetup.SetTrans(tmSetup.GetTrans() * scalefactor);
		layerIKTargetTrans->SetSetupVal((void*)&tmSetup);
	}

	// call PasteRig on all the children
	for (int i = 0; i < NumChildCATControls(); i++) {
		CATControl* child = GetChildCATControl(i);
		CATControl* pastechild = pastectrl->GetChildCATControl(i);
		if (child && pastechild) child->PasteRig(pastechild, flags, scalefactor);
	}

	if (firstpastedbone) {
		CATCharacterRemap remap;

		// Add Required Mapping
		remap.AddEntry(pastectrl->GetCATParentTrans(), GetCATParentTrans());
		remap.AddEntry(pastectrl->GetCATParentTrans()->GetNode(), GetCATParentTrans()->GetNode());
		remap.AddEntry(pastectrl->GetCATParent(), GetCATParent());

		BuildMapping(pastectrl, remap, FALSE);
		PasteERNNodes(pastelimb, remap);
	}

	if (newundo && theHold.Holding()) {
		theHold.Accept(GetString(IDS_HLD_LIMBPASTE));
		UpdateLimb();
	}

	return TRUE;
}

// TODO: override this method for BoneData
int LimbData2::Display(TimeValue t, INode*, ViewExp *vpt, int flags)
{
	UNREFERENCED_PARAMETER(flags);
	Interval iv;
	if (GetIKFKRatio(t, iv) < 1.0f) {

		GraphicsWindow *gw = vpt->getGW();
		gw->setTransform(Matrix3(1)); // sets the graphicsWindow to world

		if (iktarget != NULL)
		{
			if (iktarget->Selected()) {
				gw->setColor(LINE_COLOR, GetUIColor(COLOR_SELECTION));
			}
			else if (!iktarget->IsFrozen() && !iktarget->Dependent()) {
				Color color = Color(iktarget->GetWireColor());
				gw->setColor(LINE_COLOR, color);
			}
		}

		BoneData* pFirstBone = GetBoneData(0);
		if (pFirstBone != NULL)
		{
			Point3 ikTargetPos = mtmIKTarget.GetTrans();
			Point3 rootbonepos = pFirstBone->GetNodeTM(t).GetTrans();
			dLine2Pts(gw, ikTargetPos, rootbonepos);

			if (upvectornode != NULL) {
				Point3 upnodepos = upvectornode->GetNodeTM(t).GetTrans();
				dLine2Pts(gw, ikTargetPos, upnodepos);
				dLine2Pts(gw, upnodepos, rootbonepos);
			}
		}
	}
	return 0;
}
