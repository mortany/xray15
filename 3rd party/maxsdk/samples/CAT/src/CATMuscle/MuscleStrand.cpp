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

#include "MuscleStrand.h"
#include "HdlTrans.h"
#include "HdlObj.h"
#include "SegTrans.h"
#include "../CATObjects/CATDotIni.h"

#include "notify.h"
#include "MaxIcon.h"
#include <maxscript/maxwrapper/mxsobjects.h>
#include <Graphics/IDisplayManager.h>

#include "../CATAPI.h"

MuscleStrand* muscleStrandCopy = NULL;

FPInterfaceDesc* GetStrandFPInterface();

//keeps track of whether an FP interface desc has been added to the CATClipMatrix3 ClassDesc
static bool imusclestrandInterfaceAdded = false;

class MuscleStrandClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) {
		if (!imusclestrandInterfaceAdded) {
			// So we can have different muscle types sharing one Interface, we add the
			// Function publishing interface here. usually FPInterfaces are explicitly tied
			// to a ClassDesc. In our case we didn't want them to be tied.
			AddInterface(GetStrandFPInterface());
			imusclestrandInterfaceAdded = true;
		}
		g_bIsResetting = FALSE;
		return new MuscleStrand(loading);
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_MUSCLESTRAND); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return MUSCLESTRAND_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("MuscleStrand"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};

static MuscleStrandClassDesc MuscleStrandDesc;
ClassDesc2* GetMuscleStrandDesc() { return &MuscleStrandDesc; }

/**********************************************************************
 * User Interface...
 */

class MuscleStrandDlgCallBack : public TimeChangeCallback
{
private:
	MuscleStrand *muscle;
	HWND hWnd;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnShowCC;
	ICustButton *btnResetRelaxedLength;

	ISpinnerControl *spnHandleSize;
	ISpinnerControl *spnNumSpheres;

	int currsphere;
	ISpinnerControl *spnCurrSphere;

	ISpinnerControl *spnUStart;
	ISpinnerControl *spnUEnd;
	ISpinnerControl *spnRadius;

	ISpinnerControl *spnCurrLength;
	ISpinnerControl *spnCurrScale;

	ISpinnerControl *spnLengthShort;
	ISpinnerControl *spnScaleShort;
public:

	MuscleStrandDlgCallBack() : currsphere(0) {
		muscle = NULL;

		btnCopy = NULL;
		btnPaste = NULL;
		btnShowCC = NULL;
		btnResetRelaxedLength = NULL;

		spnNumSpheres = NULL;
		spnCurrSphere = NULL;
		spnUStart = NULL;
		spnUEnd = NULL;
		spnRadius = NULL;

		spnCurrLength = NULL;
		spnCurrScale = NULL;

		spnLengthShort = NULL;
		spnScaleShort = NULL;

		spnHandleSize = NULL;
		hWnd = NULL;
	}

	HWND GetHWnd() { return hWnd; }

	void Refresh()
	{
		if (muscle == NULL)
			return;

		SET_CHECKED(hWnd, IDC_RDO_PATCH_MUSCLE, muscle->GetDeformerType() == MuscleStrand::DEFORMER_MESH);
		SET_CHECKED(hWnd, IDC_RDO_BONES_MUSCLE, muscle->GetDeformerType() == MuscleStrand::DEFORMER_BONES);

		SET_CHECKED(hWnd, IDC_RDO_LEFT, muscle->GetLMR() == -1);
		SET_CHECKED(hWnd, IDC_RDO_MIDDLE, muscle->GetLMR() == 0);
		SET_CHECKED(hWnd, IDC_RDO_RIGHT, muscle->GetLMR() == 1);

		SET_CHECKED(hWnd, IDC_RDO_X, muscle->GetMirrorAxis() == X);
		SET_CHECKED(hWnd, IDC_RDO_Y, muscle->GetMirrorAxis() == Y);
		SET_CHECKED(hWnd, IDC_RDO_Z, muscle->GetMirrorAxis() == Z);

		SET_CHECKED(hWnd, IDC_CHK_HDLVISIBLE, muscle->GetHandlesVisible());

		spnHandleSize->SetValue(muscle->GetHandleSize(), FALSE);

		spnNumSpheres->SetValue(muscle->GetNumSpheres(), FALSE);

		SET_CHECKED(hWnd, IDC_CHK_ENABLESQUASHSTRETCH, muscle->GetSquashStretch());

		spnCurrLength->SetValue(muscle->GetLength(), FALSE);
		spnCurrScale->SetValue(muscle->scaleCurrent, FALSE);

		spnLengthShort->SetValue(muscle->lengthShort, FALSE);
		spnScaleShort->SetValue(muscle->scaleShort, FALSE);
	}
	void TimeChanged(TimeValue)
	{
	}

	void InitControls(HWND hDlg, MuscleStrand *muscle)
	{
		hWnd = hDlg;
		this->muscle = muscle;
		CatDotIni catini;

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hWnd, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnCopyStrand"), GetString(IDS_TT_CPYSTRAND)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnPasteStrand"), GetString(IDS_TT_PASTESTRAND)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		btnShowCC = GetICustButton(GetDlgItem(hWnd, IDC_BTN_SHOW_PROFILE_CC));
		btnShowCC->SetType(CBT_CHECK);
		btnShowCC->SetButtonDownNotify(TRUE);
		btnShowCC->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnShowMuscleStrandCC"), GetString(IDS_TT_DISPLAYSTRANDCURVE)));
		btnShowCC->SetCheck(muscle->mpCCtl->IsActive());

		spnHandleSize = SetupFloatSpinner(hWnd, IDC_SPIN_HANDLESIZE, IDC_EDIT_HANDLESIZE, 0.001f, 100.0f, muscle->GetHandleSize());
		spnHandleSize->SetAutoScale(TRUE);

		spnNumSpheres = SetupIntSpinner(hWnd, IDC_SPIN_NUMSPHERES, IDC_EDIT_NUMSPHERES, 1, 100, muscle->GetNumSpheres());
		currsphere = 0;
		spnCurrSphere = SetupIntSpinner(hWnd, IDC_SPIN_CURRSPHERE, IDC_EDIT_CURRSPHERE, 1, muscle->GetNumSpheres(), currsphere + 1);

		spnUStart = SetupFloatSpinner(hWnd, IDC_SPIN_USTART, IDC_EDIT_USTART, 0.0f, 1.0f, muscle->GetSphereUStart(currsphere), 0.01f);
		spnUStart->SetAutoScale(TRUE);
		spnUEnd = SetupFloatSpinner(hWnd, IDC_SPIN_UEND, IDC_EDIT_UEND, 0.0f, 1.0f, muscle->GetSphereUEnd(currsphere), 0.01f);
		spnUEnd->SetAutoScale(TRUE);
		spnRadius = SetupFloatSpinner(hWnd, IDC_SPIN_RADIUS, IDC_EDIT_RADIUS, 0.0f, 1000.0f, muscle->GetSphereRadius(currsphere), 0.01f);
		spnRadius->SetAutoScale(TRUE);
		spnRadius->Enable(FALSE);

		spnCurrLength = SetupFloatSpinner(hWnd, IDC_SPIN_CURRLENGTH, IDC_EDIT_CURRLENGTH, 0.0f, 1000000.0f, muscle->GetCurrentLength(), 0.01f);
		spnCurrLength->Enable(FALSE);
		spnCurrLength->SetAutoScale(TRUE);

		// btnResetRelaxedLength button
		btnResetRelaxedLength = GetICustButton(GetDlgItem(hWnd, IDC_BTN_RESET_RELAXED));
		btnResetRelaxedLength->SetType(CBT_PUSH);
		btnResetRelaxedLength->SetButtonDownNotify(TRUE);

		spnCurrScale = SetupFloatSpinner(hWnd, IDC_SPIN_CURRSCALE, IDC_EDIT_CURRSCALE2, 0.0f, 1000000.0f, muscle->GetCurrentScale(), 0.01f);
		spnCurrScale->Enable(FALSE);
		spnCurrScale->SetAutoScale(TRUE);

		spnLengthShort = SetupFloatSpinner(hWnd, IDC_SPIN_SHORTLENGTH, IDC_EDIT_SHORTLENGTH, 0.0f, 1000.0f, muscle->lengthShort, 0.01f);
		spnScaleShort = SetupFloatSpinner(hWnd, IDC_SPIN_SHORTSCALE, IDC_EDIT_SHORTSCALE, 0.0f, 1000.0f, muscle->scaleShort, 0.01f);

		spnLengthShort->SetAutoScale(TRUE);
		spnScaleShort->SetAutoScale(TRUE);

		Refresh();
		TimeChanged(GetCOREInterface()->GetTime());

		GetCOREInterface()->RegisterTimeChangeCallback(this);
	}

	void ShowCurveControl(BOOL tf)
	{
		ICurveCtl		*mpCCtl = muscle->mpCCtl;
		if (!mpCCtl || (tf && mpCCtl->IsActive())) return;

		if (!tf) {
			mpCCtl->SetActive(FALSE);
			return;
		}

		mpCCtl->SetCustomParentWnd(GetDlgItem(hWnd, IDC_CURVE));
		mpCCtl->SetMessageSink(hWnd);
		mpCCtl->SetActive(TRUE);

		// This must be called after the SetActive call above, or GetHWND will return NULL
		lpfnOldWndProc = (WNDPROC)SetWindowLongPtr(mpCCtl->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubClassFunc);

	}

	void ReleaseControls()
	{
		GetCOREInterface()->UnRegisterTimeChangeCallback(this);

		this->hWnd = NULL;
		muscle = NULL;

		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnShowCC);

		SAFE_RELEASE_SPIN(spnHandleSize);

		SAFE_RELEASE_SPIN(spnNumSpheres);
		SAFE_RELEASE_SPIN(spnCurrSphere);
		SAFE_RELEASE_SPIN(spnUStart);
		SAFE_RELEASE_SPIN(spnUEnd);
		SAFE_RELEASE_SPIN(spnRadius);

		SAFE_RELEASE_SPIN(spnCurrLength);
		SAFE_RELEASE_SPIN(spnCurrScale);
		SAFE_RELEASE_SPIN(spnLengthShort);
		SAFE_RELEASE_SPIN(spnScaleShort);
	}

	BOOL CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (MuscleStrand*)lParam);
			break;
		case WM_DESTROY:
			if (this->hWnd != hWnd) break;
			ShowCurveControl(FALSE);
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) { // Notification codes
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) { // Switch on ID
				case IDC_BTN_COPY:
					muscleStrandCopy = muscle;
					break;
				case IDC_BTN_PASTE:
					if (muscleStrandCopy) {
						muscle->PasteMuscleStrand(muscleStrandCopy);
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				}
				break;
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_RDO_PATCH_MUSCLE:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_BONES_MUSCLE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					{
						HoldActions hold(IDS_HLD_MSCLSETTING);
						muscle->SetDeformerType(MuscleStrand::DEFORMER_MESH);
					}
					Refresh();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				case IDC_RDO_BONES_MUSCLE:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_PATCH_MUSCLE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					{
						HoldActions hold(IDS_HLD_MSCLSETTING);
						muscle->SetDeformerType(MuscleStrand::DEFORMER_BONES);
					}
					Refresh();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				case IDC_RDO_LEFT:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_MIDDLE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendMessage(GetDlgItem(hWnd, IDC_RDO_RIGHT), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetLMR(-1);
					break;
				case IDC_RDO_MIDDLE:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_LEFT), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendMessage(GetDlgItem(hWnd, IDC_RDO_RIGHT), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetLMR(0);
					break;
				case IDC_RDO_RIGHT:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_LEFT), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendMessage(GetDlgItem(hWnd, IDC_RDO_MIDDLE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetLMR(1);
					break;
				case IDC_RDO_X:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_Y), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendMessage(GetDlgItem(hWnd, IDC_RDO_Z), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetMirrorAxis(kXAxis);
					break;
				case IDC_RDO_Y:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_X), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendMessage(GetDlgItem(hWnd, IDC_RDO_Z), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetMirrorAxis(kYAxis);
					break;
				case IDC_RDO_Z:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_X), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendMessage(GetDlgItem(hWnd, IDC_RDO_Y), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetMirrorAxis(kZAxis);
					break;
				case IDC_CHK_HDLVISIBLE:
					muscle->SetHandlesVisible(IS_CHECKED(hWnd, IDC_CHK_HDLVISIBLE));
					Refresh();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				case IDC_CHK_ENABLESQUASHSTRETCH:
				{
					HoldActions hold(IDS_HLD_MSCLSETTING);
					muscle->SetSquashStretch(IS_CHECKED(hWnd, IDC_CHK_ENABLESQUASHSTRETCH));
				}
				Refresh();
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
				break;
				case IDC_BTN_SHOW_PROFILE_CC:
					//	ShowCurveControl(IS_CHECKED(hWnd, IDC_BTN_SHOW_PROFILE_CC));
					ShowCurveControl(btnShowCC->IsChecked());
					break;
				case IDC_BTN_RESET_RELAXED:
					muscle->SetDefaultLength(muscle->GetLength());
					muscle->UpdateMuscleStrand();
					Refresh();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				}
				break;
			}
			break;
		case CC_SPINNER_BUTTONDOWN:
			if (!theHold.Holding()) theHold.Begin();
			break;
		case CC_SPINNER_BUTTONUP:
			if (HIWORD(wParam)) {
				if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_STRAND));
			}
			else theHold.Cancel();
			break;
		case CC_SPINNER_CHANGE: {
			ISpinnerControl *ccSpinner = (ISpinnerControl*)lParam;

			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_NUMSPHERES:
				muscle->SetNumSpheres(ccSpinner->GetIVal());
				break;
			case IDC_SPIN_CURRSPHERE:
				currsphere = ccSpinner->GetIVal() - 1;
				spnUStart->SetValue(muscle->GetSphereUStart(currsphere), FALSE);
				spnUEnd->SetValue(muscle->GetSphereUEnd(currsphere), FALSE);
				spnRadius->SetValue(muscle->GetSphereRadius(currsphere), FALSE);
				break;
			case IDC_SPIN_USTART:
				muscle->SetSphereUStart(currsphere, ccSpinner->GetFVal());
				muscle->UpdateMuscleStrand();
				break;
			case IDC_SPIN_UEND:
				muscle->SetSphereUEnd(currsphere, ccSpinner->GetFVal());
				muscle->UpdateMuscleStrand();
				break;
			case IDC_SPIN_SHORTLENGTH:
				muscle->SetDefaultLength(ccSpinner->GetFVal());
				muscle->UpdateMuscleStrand();
				break;
			case IDC_SPIN_SHORTSCALE:
				muscle->SetSquashStretchScale(ccSpinner->GetFVal());
				muscle->UpdateMuscleStrand();
				break;
			case IDC_SPIN_HANDLESIZE:
				muscle->SetHandleSize(ccSpinner->GetFVal());
				break;
			}
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			break;
		}
		case WM_CC_CHANGE_CURVEPT:
		case WM_CC_CHANGE_CURVETANGENT:
		case WM_CC_DEL_CURVEPT:
		case WM_CC_INSERT_CURVEPT:
			muscle->UpdateMuscleStrand();
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	// The following WndProc is directly subclassing the CurveCtl WndProc,
	// and is used for un-checking the button when the window is closed
	// via the (X) button.  When this happens, we aren't notified by
	// the regular callback above.
	static WNDPROC lpfnOldWndProc;
	static LRESULT FAR SubClassFunc(HWND hWnd, UINT Message, WPARAM wParam,
		LPARAM lParam)
	{
		if (Message == WM_DESTROY)
		{
			if (GetInstance()->btnShowCC != NULL)
				GetInstance()->btnShowCC->SetCheck(FALSE);
		}
		return CallWindowProc(lpfnOldWndProc, hWnd, Message, wParam,
			lParam);
	}

	static MuscleStrandDlgCallBack* GetInstance() {
		static MuscleStrandDlgCallBack musclestranddlgcallBack;
		return &musclestranddlgcallBack;
	}
};
WNDPROC MuscleStrandDlgCallBack::lpfnOldWndProc = NULL;

static INT_PTR CALLBACK MuscleStrandDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return MuscleStrandDlgCallBack::GetInstance()->DlgProc(hWnd, message, wParam, lParam);
};

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc stand_interface(
	CATMUSCLESTRAND_INTERFACE_ID, _T("CATMuscleStrand Functions"), 0, NULL, FP_MIXIN,

	MuscleStrand::fnPasteStrand, _T("PasteStrand"), 0, TYPE_VOID, 0, 1,
		_T("SourceStrand"), 0, TYPE_REFTARG,

	MuscleStrand::fnGetSphereRadius, _T("GetSphereRadius"), 0, TYPE_FLOAT, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	MuscleStrand::fnSetSphereRadius, _T("SetSphereRadius"), 0, TYPE_VOID, 0, 2,
		_T("index"), 0, TYPE_INDEX,
		_T("value"), 0, TYPE_FLOAT,

	MuscleStrand::fnGetSphereUStart, _T("GetSphereUStart"), 0, TYPE_FLOAT, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	MuscleStrand::fnSetSphereUStart, _T("SetSphereUStart"), 0, TYPE_VOID, 0, 2,
		_T("index"), 0, TYPE_INDEX,
		_T("value"), 0, TYPE_FLOAT,

	MuscleStrand::fnGetSphereUEnd, _T("GetSphereUEnd"), 0, TYPE_FLOAT, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	MuscleStrand::fnSetSphereUEnd, _T("SetSphereUEnd"), 0, TYPE_VOID, 0, 2,
		_T("index"), 0, TYPE_INDEX,
		_T("value"), 0, TYPE_FLOAT,

	properties,

	MuscleStrand::fnGetLMR, MuscleStrand::fnSetLMR, _T("LMR"), 0, TYPE_INT,

	MuscleStrand::fnGetHandleSize, MuscleStrand::fnSetHandleSize, _T("HandleSize"), 0, TYPE_FLOAT,
	MuscleStrand::fnGetHandleVis, MuscleStrand::fnSetHandleVis, _T("HandlesVisible"), 0, TYPE_BOOL,

	MuscleStrand::fnGetHandles, FP_NO_FUNCTION, _T("Handles"), 0, TYPE_INODE_TAB_BV,

	MuscleStrand::fnGetNumSpheres, MuscleStrand::fnSetNumSpheres, _T("NumSpheres"), 0, TYPE_INT,

	MuscleStrand::fnGetCurrentLength, FP_NO_FUNCTION, _T("CurrentLength"), 0, TYPE_FLOAT,
	MuscleStrand::fnGetCurrentScale, FP_NO_FUNCTION, _T("CurrentScale"), 0, TYPE_FLOAT,

	MuscleStrand::fnGetSquashStretch, MuscleStrand::fnSetSquashStretch, _T("SquashStretch"), 0, TYPE_BOOL,
	MuscleStrand::fnGetDefaultLength, MuscleStrand::fnSetDefaultLength, _T("DefaultLength"), 0, TYPE_FLOAT,
	MuscleStrand::fnGetSquashStretchScale, MuscleStrand::fnSetSquashStretchScale, _T("SquashStretchScale"), 0, TYPE_FLOAT,

	p_end
);

FPInterfaceDesc* GetStrandFPInterface() {
	return &stand_interface;
}

//  Get Descriptor method for Mixin Interface
//  *****************************************

FPInterfaceDesc* MuscleStrand::GetDescByID(Interface_ID id) {
	if (id == CATMUSCLESTRAND_INTERFACE_ID) return &stand_interface;
	return &nullInterface;
}

// Note: We need a custom RestoreObj to handle
// data kept in an array.  See DataRestoreObj
// comments for more information.
class SphereParamsRestore : public RestoreObj {
public:
	MuscleStrand *mMuscleStrand;
	int mIdx;
	SphereParams mUndo, mRedo;

	SphereParamsRestore(MuscleStrand *muscle, int idx)
		: mMuscleStrand(muscle)
		, mIdx(idx)
	{
		if (mMuscleStrand != NULL && idx >= 0 && idx < mMuscleStrand->GetNumSpheres())
			mUndo = mMuscleStrand->mTabSphereParams[idx];
		mMuscleStrand->SetAFlag(A_HELD);
	}

	~SphereParamsRestore() {}

	void Restore(int isUndo) {
		if (isUndo)
			mRedo = mMuscleStrand->mTabSphereParams[mIdx];

		mMuscleStrand->mTabSphereParams[mIdx] = mUndo;
		mMuscleStrand->UpdateMuscleStrand();
		mMuscleStrand->RefreshUI();
	}

	void Redo() {
		mMuscleStrand->mTabSphereParams[mIdx] = mRedo;
		mMuscleStrand->UpdateMuscleStrand();
		mMuscleStrand->RefreshUI();
	}
	int Size() { return sizeof(this); }
	void EndHold() { mMuscleStrand->ClearAFlag(A_HELD); }
};

float MuscleStrand::GetSphereRadius(int i)
{
	float radius = 0.0f;
	if (i < GetNumSpheres())
	{
		SphereParams& aSphere = mTabSphereParams[i];
		float u = (aSphere.UStart + aSphere.UEnd) / 2.0f;
		radius = mpCCtl->GetControlCurve(0)->GetValue(0, u, ivalid);
	}
	return radius;
}

void MuscleStrand::SetSphereUStart(int id, float v) {
	if (id >= 0 && id < GetNumSpheres())
	{
		HoldActions holdThis(IDS_HLD_USTART);
		if (theHold.Holding())
			theHold.Put(new SphereParamsRestore(this, id));
		mTabSphereParams[id].UStart = v;
	}
}

void MuscleStrand::SetSphereUEnd(int id, float v) {
	if (0 <= id && id < GetNumSpheres())
	{
		HoldActions holdThis(IDS_HLD_UEND);
		if (theHold.Holding())
			theHold.Put(new SphereParamsRestore(this, id));
		mTabSphereParams[id].UEnd = v;
	}
}

void	MuscleStrand::SetDefaultLength(float v) {
	HoldActions holdThis(IDS_HLD_LENSHORT);
	HoldData(lengthShort);
	lengthShort = v;
}

void	MuscleStrand::SetSquashStretchScale(float v) {
	HoldActions holdThis(IDS_HLD_SCLSHORT);
	HoldData(scaleShort);
	scaleShort = v;
};

BaseInterface* MuscleStrand::GetInterface(Interface_ID id) {
	if (id == CATMUSCLESTRAND_INTERFACE_ID) return (IMuscleStrand*)this;
	if (id == CATMUSCLE_SYSTEMMASTER_INTERFACE_ID) return (ICATMuscleSystemMaster*)this;

	BaseInterface* pInterface = FPMixinInterface::GetInterface(id);
	if (pInterface != NULL)
	{
		return pInterface;
	}
	return SimpleObject2::GetInterface(id);
}

static void StrandNodeRenameNotify(void *param, NotifyInfo *)
{
	MuscleStrand *strand = (MuscleStrand*)param;
	INode* pOwningNode = FindReferencingClass<INode>(strand);
	if (pOwningNode != NULL)
	{
		strand->UpdateName(pOwningNode->GetName());
	}
}
void MuscleStrand::Create(INode* node)
{
	DbgAssert(node != NULL);
	if (node == NULL)
		return;

	DbgAssert(node->GetObjectRef()->FindBaseObject() == this);

	ivalid = FOREVER;
	// force the creation of the corner handles and tangents
	CreateHandles(node);
	AssignSegTransTMController(node);

	ivalid = NEVER;
}

void MuscleStrand::Init()
{
	dVersion = 0;
	deformer_type = DEFORMER_MESH;

	handlesize = 10.0f;
	handlesvalid = NEVER;
	tabHandles.SetCount(NUMNODEREFS);
	for (int i = 0; i < NUMNODEREFS; i++) {
		tabHandles[i] = (INode*)NULL;
		ls_hdl_tm[i].IdentityMatrix();
		ws_hdl_tm[i].IdentityMatrix();
	}

	mpCCtl = NULL;
	tm = Matrix3(1);
	subobject_level = 0;

	lengthShort = 0.0f;
	scaleShort = 1.0f;
	scaleCurrent = 1.0f;

	flags = 0;
	lmr = 0;
	mirroraxis = kXAxis;

	flagsBegin = 0;
	ipRollout = NULL;

	bDeleting = FALSE;

	RegisterNotification(StrandNodeRenameNotify, this, NOTIFY_NODE_RENAMED);
}

MuscleStrand::MuscleStrand(BOOL loading)
{
	Init();

	if (!loading) {
		ICurveCtl* pCCtl = (ICurveCtl*)CreateInstance(REF_MAKER_CLASS_ID, CURVE_CONTROL_CLASS_ID);
		ReplaceReference(CC_PROFILE, pCCtl);

		DbgAssert(mpCCtl);
		mpCCtl->SetNumCurves(1);
		mpCCtl->SetXRange(0.0f, 1.0f);
		mpCCtl->SetYRange(0.0f, 100.0f);

		BitArray ba;
		ba.SetSize(1);
		ba.Set(0);
		mpCCtl->SetDisplayMode(ba);

		ICurve *pCurve = mpCCtl->GetControlCurve(0);
		if (pCurve)
		{
			pCurve->SetNumPts(3);
			CurvePoint pt;

			// Point 3
			pt.p = Point2(1.0, 5.0);
			pt.in = Point2(-0.15, 0.0);
			pt.flags = CURVEP_CORNER | CURVEP_BEZIER;
			pCurve->SetPoint(0, 2, &pt, TRUE, FALSE);

			// Point 2
			pt.p = Point2(0.5, 20.0);
			pt.in = Point2(-0.15, 0.0);
			pt.out = Point2(0.15, 0.0);
			pt.flags = CURVEP_BEZIER;
			pCurve->SetPoint(0, 1, &pt, TRUE, FALSE);

			// Point 1
			pt.p = Point2(0.0, 5.0);
			pt.in = Point2(-0.15, 0.0);
			pt.out = Point2(0.15, 0.0);
			pt.flags = CURVEP_CORNER | CURVEP_BEZIER;
			pCurve->SetPoint(0, 0, &pt, TRUE, FALSE);

		}

		mpCCtl->SetZoomValues(500.0, 4.0);
		mpCCtl->SetScrollValues(-24, -40);

		mpCCtl->ZoomExtents();
		mpCCtl->SetTitle(GetString(IDS_MUSCLE_PROFCURVE));

		//		bCreated = TRUE;
	}
}

MuscleStrand::~MuscleStrand()
{
	UnRegisterNotification(StrandNodeRenameNotify, this, NOTIFY_NODE_RENAMED);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Class Animateable

MoveModBoxCMode*	MuscleStrand::moveMode = NULL;
SelectModBoxCMode*	MuscleStrand::selMode = NULL;

void MuscleStrand::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) {
	UNREFERENCED_PARAMETER(prev);
	this->flagsBegin = flags;
	this->ipRollout = ip;
	if (!TestFlag(STRANDFLAG_KEEP_ROLLOUTS) && (flagsBegin == 0 || flagsBegin&BEGIN_EDIT_MOTION || flagsBegin&BEGIN_EDIT_CREATE)) {

		//////////////////////////////////////////////////////
		// Initialise Sub-Object mode
		// Create sub object editing modes.
		moveMode = new MoveModBoxCMode(this, ip);
		selMode = new SelectModBoxCMode(this, ip);
		//////////////////////////////////////////////////////

		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_MUSCLESTRAND_ROLLOUT), MuscleStrandDlgProc, GetString(IDS_CL_MUSCLESTRAND), (LPARAM)this);
	}
}
void MuscleStrand::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) {
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);

	if (!TestFlag(STRANDFLAG_KEEP_ROLLOUTS) && (flagsBegin == 0 || flagsBegin&BEGIN_EDIT_MOTION || flagsBegin&BEGIN_EDIT_CREATE)) {

		///////////////////////////////////
		// End the subobject mode
		ip->DeleteMode(moveMode);	if (moveMode) { delete moveMode; }	moveMode = NULL;
		ip->DeleteMode(selMode);	if (selMode) { delete selMode; } 	selMode = NULL;
		///////////////////////////////////

		ip->DeleteRollupPage(MuscleStrandDlgCallBack::GetInstance()->GetHWnd());
	}
	this->flagsBegin = 0;
	this->ipRollout = NULL;
}

void	MuscleStrand::DrawStartGizmo(class GraphicsWindow *gw, int sphereid)
{
	if (sphereid >= 0 && sphereid < GetNumSpheres())
	{
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_SEL_GIZMOS));
		Point3 p = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), mTabSphereParams[sphereid].UStart);
		gw->marker(&p, ASTERISK_MRKR);
		bbox += p;

	}
}

void	MuscleStrand::DrawEndGizmo(class GraphicsWindow *gw, int sphereid)
{
	if (sphereid >= 0 && sphereid < GetNumSpheres())
	{
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_SEL_GIZMOS));
		Point3 p = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), mTabSphereParams[sphereid].UEnd);
		gw->marker(&p, ASTERISK_MRKR);
		bbox += p;
	}
}

void MuscleStrand::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box)
{
	if (subobject_level == 0) {
		return SimpleObject2::GetWorldBoundBox(t, inode, vpt, box);
	}
	box += bbox;
}

int MuscleStrand::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	int retval = SimpleObject2::Display(t, inode, vpt, flags);
	if (subobject_level == 0) {
		return retval;
	}
	bbox.Init();

	//////////////////////////////////////////////////////////////////////////
	GraphicsWindow *gw = vpt->getGW();
	Matrix3 modmat, ntm = inode->GetObjTMBeforeWSM(t), off, invoff;

	// I draw weverything in world coordinates.
	// I am not sure if this is a performance issue, but the maths is simpler for me
	// No modifiers can be applied to these objects
	gw->setTransform(Matrix3(1));

	for (int i = 0; i < GetNumSpheres(); i++)
	{
		DrawStartGizmo(gw, i);
		DrawEndGizmo(gw, i);
	}

	return retval;
}

static GenSubObjType SOT_Sphere_End_Points(31);

ISubObjType *MuscleStrand::GetSubObjType(int i)
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		SOT_Sphere_End_Points.SetName(GetString(IDS_SPHEREENDPTS));
	}
	if (i == -1)	return NULL;
	//	if(i==0)	return NULL;
	return &SOT_Sphere_End_Points;
}

int MuscleStrand::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	if (subobject_level == 0) {
		return SimpleObject2::HitTest(t, inode, type, crossing, flags, p, vpt);
	}
	Matrix3 tm(1);
	int res = 0;
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0, 0, 0);

	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(Matrix3(1));

	MakeHitRegion(hitRegion, type, crossing, 4, p);
	gw->setHitRegion(&hitRegion);
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	gw->clearHitCode();

	const int nSpheres = GetNumSpheres();
	for (int i = 0; i < nSpheres; i++)
	{
		DrawStartGizmo(gw, i);
		if (gw->checkHitCode()) {
			vpt->CtrlLogHit(inode, gw->getHitDistance(), i, 0);
			gw->clearHitCode();
			res = TRUE;
		}
		DrawEndGizmo(gw, i);
		if (gw->checkHitCode()) {
			vpt->CtrlLogHit(inode, gw->getHitDistance(), i + nSpheres, 0);
			gw->clearHitCode();
			res = TRUE;
		}
	}

	gw->setRndLimits(savedLimits);
	return res;
}

void	MuscleStrand::SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	UNREFERENCED_PARAMETER(hitRec); UNREFERENCED_PARAMETER(selected); UNREFERENCED_PARAMETER(all); UNREFERENCED_PARAMETER(invert);
}

void	MuscleStrand::ClearSelection(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
}

void	MuscleStrand::SelectAll(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
}

void	MuscleStrand::InvertSelection(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
}

int		MuscleStrand::SubObjectIndex(HitRecord *hitRec)
{
	return hitRec->hitInfo;
}

void MuscleStrand::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue, INode *, ModContext *)
{
	Point3 p;

	// Compute a the transformation out of lattice space into world space
//	if (mc && mc->box) lbox = *mc->box;

	if (subobject_level == 0) {
		cb->Center(tm.GetTrans(), 0);
	}
}

void MuscleStrand::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue, INode *, ModContext *)
{
	const int nSpheres = GetNumSpheres();
	float u;
	Point3 p, p2;
	Matrix3 tm;
	for (int i = 0; i < nSpheres; i++)
	{
		SphereParams& aSphere = mTabSphereParams[i];
		u = aSphere.UStart;

		if (u <= 0.0f) {
			tm = ws_hdl_tm[NODE_ST];
		}
		else if (u >= 1.0f) {
			tm = ws_hdl_tm[NODE_EN];
		}
		else {
			p = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), u);
			p2 = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), u + 0.01f);

			tm = ws_hdl_tm[NODE_ST];
			BlendMat(tm, ws_hdl_tm[NODE_EN], u);
			tm.SetTrans(p);
			RotMatToLookAtPoint(tm, X, FALSE, p2);
		}
		cb->TM(tm, i);

		////////////////////////////////////////////////////////
		u = aSphere.UEnd;

		if (u <= 0.0f) {
			tm = ws_hdl_tm[NODE_ST];
		}
		else if (u >= 1.0f) {
			tm = ws_hdl_tm[NODE_EN];
		}
		else {
			p = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), u);
			p2 = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), u + 0.01f);

			tm = ws_hdl_tm[NODE_ST];
			BlendMat(tm, ws_hdl_tm[NODE_EN], u);
			tm.SetTrans(p);
			RotMatToLookAtPoint(tm, X, FALSE, p2);
		}
		cb->TM(tm, i + nSpheres);

	}
}

void	MuscleStrand::Move(TimeValue, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
{
	UNREFERENCED_PARAMETER(localOrigin);
	Point3 p = VectorTransform(tmAxis*Inverse(partm), val);

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void MuscleStrand::ActivateSubobjSel(int level, XFormModes& modes)
{
	subobject_level = level;
	if (level > 0) {
		modes = XFormModes(moveMode, NULL, NULL, NULL, NULL, selMode);
	}
	NotifyDependents(FOREVER, PART_SUBSEL_TYPE | PART_SELECT | PART_DISPLAY, REFMSG_CHANGE);
}

//////////////////////////////////////////////////////////////////////////////////////////
// From Object
void MuscleStrand::RefreshUI()
{
	if (ipRollout) {
		MuscleStrandDlgCallBack::GetInstance()->TimeChanged(GetCOREInterface()->GetTime());
		MuscleStrandDlgCallBack::GetInstance()->Refresh();
	}
}

//Class for interactive creation of the object using the mouse
class MuscleStrandCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;		//First point in screen coordinates
	MuscleStrand *ob;			//Pointer to the object
	Point3 p0, p1, p2;	//First point in world coordinates
	Matrix3 tm1, tm2, tm;

	Point3 p3YAxis;		// Position/Rotation helpy things
	Point3 p3LastPos;	// Position/Rotation helpy things

public:
	BOOL SupportAutoGrid() { return TRUE; }
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(MuscleStrand *obj) { ob = obj; }

	int override(int mode) { UNREFERENCED_PARAMETER(mode); return CLICK_DOWN_POINT; }

};

int MuscleStrandCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3&)
{
	UNREFERENCED_PARAMETER(flags);

	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();
	if (msg == MOUSE_FREEMOVE) {
		vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
	}
	else if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {

		// If a click has already been recorded for this MuscleStrand,
		// then we have created our handles, and need to resize them
		// to match the scaling of the muscle as its stretched out.
		if (point > 0)
		{
			ob->lengthShort = ob->GetLength();
			ob->SetHandleSize(ob->lengthShort / 25.0f);

			ICurve *pCurve = ob->mpCCtl->GetControlCurve(0);
			if (pCurve)
			{
				CurvePoint pt;
				// Point 3
				pt = pCurve->GetPoint(0, 2);
				pt.p.y = 5.0f * (ob->lengthShort / 250.0f);
				pCurve->SetPoint(0, 2, &pt, TRUE, FALSE);

				// Point 2
				pt = pCurve->GetPoint(0, 1);
				pt.p.y = 15.0f * (ob->lengthShort / 250.0f);
				pCurve->SetPoint(0, 1, &pt, TRUE, FALSE);

				// Point 1
				pt = pCurve->GetPoint(0, 0);
				pt.p.y = 5.0f * (ob->lengthShort / 250.0f);
				pCurve->SetPoint(0, 0, &pt, TRUE, FALSE);
			}
		}

		switch (point)
		{
		case 0: // only happens with MOUSE_POINT msg
		{
			ob->suspendSnap = TRUE;
			vpt->TrackImplicitGrid(m, &tm1, SNAP_IN_3D);

			INode* pNode = FindReferencingClass<INode>(ob);
			ob->Create(pNode);
			ob->SetNumSpheres(7);

			HoldSuspend hs;
			ob->tabHandles[MuscleStrand::NODE_ST]->SetNodeTM(t, tm1);
			ob->tabHandles[MuscleStrand::NODE_ST_HDL]->SetNodeTM(t, tm1);
			ob->tabHandles[MuscleStrand::NODE_EN]->SetNodeTM(t, tm1);
			ob->tabHandles[MuscleStrand::NODE_EN_HDL]->SetNodeTM(t, tm1);
			break;
		}
		case 1:
		{
			HoldSuspend hs;
			vpt->TrackImplicitGrid(m, &tm2, SNAP_IN_3D);

			tm = tm1;
			RotMatToLookAtPoint(tm, X, FALSE, tm2.GetTrans());
			ob->tabHandles[MuscleStrand::NODE_ST]->SetNodeTM(t, tm);

			tm = tm2;
			RotMatToLookAtPoint(tm, X, TRUE, tm1.GetTrans());
			ob->tabHandles[MuscleStrand::NODE_ST_HDL]->SetNodeTM(t, tm);
			ob->tabHandles[MuscleStrand::NODE_EN]->SetNodeTM(t, tm);
			ob->tabHandles[MuscleStrand::NODE_EN_HDL]->SetNodeTM(t, tm);
			break;
		}
		case 2:
		{
			HoldSuspend hs;
			vpt->TrackImplicitGrid(m, &tm1, SNAP_IN_3D);

			tm = tm1;
			RotMatToLookAtPoint(tm, X, FALSE, tm2.GetTrans());
			ob->tabHandles[MuscleStrand::NODE_EN]->SetNodeTM(t, tm);
			ob->tabHandles[MuscleStrand::NODE_EN_HDL]->SetNodeTM(t, tm);
			break;
		}
		case 3:
		{
			HoldSuspend hs;
			vpt->TrackImplicitGrid(m, &tm2, SNAP_IN_3D);

			tm = tm2;
			RotMatToLookAtPoint(tm, X, TRUE, tm1.GetTrans());
			ob->tabHandles[MuscleStrand::NODE_EN]->SetNodeTM(t, tm);

			tm = tm1;
			RotMatToLookAtPoint(tm, X, FALSE, tm2.GetTrans());
			ob->tabHandles[MuscleStrand::NODE_EN_HDL]->SetNodeTM(t, tm);

			if (msg == MOUSE_POINT) 		return CREATE_STOP;
			break;
		}

		case 4:
		{
			if (msg == MOUSE_POINT) {
				// we don't need this one anymore
			//	ip->DeleteNode(node, FALSE);
				return CREATE_STOP;
			}
		}
		}
	}
	else {
		if (msg == MOUSE_ABORT) {
			return CREATE_ABORT;
		}
	}

	return CREATE_CONTINUE;
}

static MuscleStrandCreateCallBack MuscleStrandCreateCB;

//From BaseObject
CreateMouseCallBack* MuscleStrand::GetCreateMouseCallBack()
{
	MuscleStrandCreateCB.SetObj(this);
	return(&MuscleStrandCreateCB);
}

//////////////////////////////////////////////////////////////////////////////////////////
// From ReferenceTarget

RefTargetHandle MuscleStrand::GetReference(int i)
{
	if (i <= NODE_EN_HDL)	return tabHandles[i];
	if (i == CC_PROFILE)	return mpCCtl;
	return NULL;
}
void MuscleStrand::SetReference(int i, RefTargetHandle rtarg)
{
	if (i <= NODE_EN_HDL)	tabHandles[i] = (INode*)rtarg;
	if (i == CC_PROFILE)	mpCCtl = (ICurveCtl*)rtarg;
}

// From ReferenceTarget
RefTargetHandle MuscleStrand::Clone(RemapDir& remap)
{
	MuscleStrand* newob = new MuscleStrand();

	newob->lmr = lmr;
	newob->handlesvalid = NEVER;
	newob->handlesize = handlesize;

	newob->dVersion = dVersion;
	newob->flags = flags;
	newob->deformer_type = deformer_type;

	if (mpCCtl)	newob->ReplaceReference(CC_PROFILE, remap.CloneRef(mpCCtl));

	// Do not clone the handles, the references will be set by HdlTrans on clone.

	// Does a shallow copy - will copy over all the basic data.
	newob->mTabSphereParams = mTabSphereParams;
	// Once the shallow copy is performed, we need to patch the pointers
	switch (deformer_type) {
	case DEFORMER_BONES:
	{
		const int nSpheres = GetNumSpheres();
		for (int i = 0; i < nSpheres; i++)
		{
			newob->mTabSphereParams[i].pSphereNode = NULL;
			remap.PatchPointer((RefTargetHandle*)&newob->mTabSphereParams[i].pSphereNode, (RefTargetHandle)mTabSphereParams[i].pSphereNode);
		}
	}
	break;
	case DEFORMER_MESH:
		break;
	}

	newob->lengthShort = lengthShort;
	newob->scaleShort = scaleShort;

	newob->ivalid = NEVER;

	BaseClone(this, newob, remap);
	return newob;
}

RefResult MuscleStrand::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		UpdateMuscleStrand();
		if (hTarg == mpCCtl)
		{
			BuildMesh(GetCOREInterface()->GetTime());
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
		}
		break;
	case REFMSG_TARGET_DELETED:
		for (int i = 0; i < NUMNODEREFS; i++) { if (hTarg == tabHandles[i])	tabHandles[i] = NULL; }
		if (muscleStrandCopy == this) { muscleStrandCopy = NULL; }
		break;
	}
	return REF_SUCCEED;
}

// From Object
int MuscleStrand::IntersectRay(TimeValue, Ray& ray, float& at, Point3& norm)
{
	float a, b, c, ac4, b2, at1, at2;
	float root;
	BOOL neg1, neg2;
	Ray rayInSphereSpace;

	Point3 sphereNorm;
	float currAt = 0.0f;
	int spherehit = 0;

	// collisions with spheres
	const int nSpheres = GetNumSpheres();
	for (int i = 0; i < nSpheres; i++) {

		Point3 hitpoint = ray.p;

		Matrix3 tmInv = Inverse(mTabSphereParams[i].tmLocalSpace);

		rayInSphereSpace.p = ray.p * tmInv;
		tmInv.NoTrans();
		rayInSphereSpace.dir = ray.dir * tmInv;

		a = DotProd(rayInSphereSpace.dir, rayInSphereSpace.dir);
		b = DotProd(rayInSphereSpace.dir, rayInSphereSpace.p) * 2.0f;
		c = DotProd(rayInSphereSpace.p, rayInSphereSpace.p) - 1.0f;//radius*radius;

		ac4 = 4.0f * a * c;
		b2 = b*b;

		if (ac4 > b2) continue;

		// We want the smallest positive root
		root = Sqrt(b2 - ac4);
		at1 = (-b + root) / (2.0f * a);
		at2 = (-b - root) / (2.0f * a);

		neg1 = at1 < 0.0f;
		neg2 = at2 < 0.0f;
		// no intersections
		if (neg1 && neg2) continue;
		// 1 intersection
		else if (neg1 && !neg2) at = at2;
		else if (!neg1 && neg2) at = at1;
		// 2 intersections
		// choose the fartherest away point
		else if (at1 < at2) at = at2;
		else at = at1;

		if (at > currAt) {
			norm = Normalize(ray.p + at*ray.dir);
			spherehit = 1;
			currAt = at;
		}
	}
	at = currAt;
	return spherehit;
}

void SetVert(Mesh &mesh, const int &id, const float &x, const float &y, const float &z, const Matrix3 &tm)
{
	Point3 p = tm.PointTransform(Point3(x, y, z));
	mesh.setVert(id, p.x, p.y, p.z);
}

// This is basicly the buildmesh for the standard Sphere object stolen from Sphere.cpp
float uval[3] = { 1.0f,0.0f,1.0f };
void BuildSphereMesh(int i, int nSpheres, TimeValue, Mesh &mesh, float radius, Matrix3 tm)
{
	radius = 1.0f;

	tm.PreRotateY(HALFPI);

	Point3 p;
	int ix, na, nb, nc, nd, jx, kx;
	int nf = 0, nv = 0;
	float delta, delta2;
	float a, alt, secrad, secang, b, c;
	int segs = 8;
	int smooth = TRUE;
	BOOL noHemi = FALSE;
	BOOL genUVs = FALSE;//TRUE;
	float startAng = 0.0f;
	int doPie = FALSE;

	noHemi = TRUE;
	float basedelta = 2.0f*PI / (float)segs;
	delta2 = basedelta;
	delta = basedelta;
	int rows = (segs / 2 - 1);

	int realsegs = segs;
	int nverts = rows * realsegs + 2;
	int nfaces = rows * realsegs * 2;

	///////////////////////////////////////////
	int initNumVerts = 0;
	int initNumFaces = 0;
	if (i == 0) {
		mesh.setNumVerts(nverts * nSpheres);
		mesh.setNumFaces(nfaces * nSpheres);
	}
	else {
		initNumVerts = i * nverts;
		initNumFaces = i * nfaces;
	}

	///////////////////////////////////////////

	mesh.setSmoothFlags(smooth != 0);
	int lastvert = nverts - 1;

	// Top vertex
//	mesh.setVert( initNumVerts + nv, 0.0f, 0.0f, radius);
	SetVert(mesh, initNumVerts + nv, 0.0f, 0.0f, radius, tm);
	nv++;

	// Middle vertices
	alt = delta;
	for (ix = 1; ix <= rows; ix++) {
		//	if (!noHemi && ix==rows) alt = hemi;
		a = (float)cos(alt)*radius;
		secrad = (float)sin(alt)*radius;
		secang = startAng; //0.0f
		for (jx = 0; jx < segs; ++jx) {
			b = (float)cos(secang)*secrad;
			c = (float)sin(secang)*secrad;
			//	mesh.setVert( initNumVerts + nv++, b,c,a);
			SetVert(mesh, initNumVerts + nv++, b, c, a, tm);
			secang += delta2;
		}
		//	if (doPie &&(noHemi ||(ix<rows))) {
		//	//	mesh.setVert( initNumVerts + nv++,0.0f,0.0f,a);
		//		SetVert(mesh, initNumVerts + nv++,0.0f,0.0f,a, tm);
		//	}
		alt += delta;
	}

	// Bottom vertex
//	mesh.setVert( initNumVerts + nv++, 0.0f, 0.0f,-radius);
	SetVert(mesh, initNumVerts + nv++, 0.0f, 0.0f, -radius, tm);

	BOOL issliceface;
	// Now make faces
//	if (doPie) segs++;

	// Make top conic cap
	for (ix = 1; ix <= segs; ++ix) {
		issliceface = FALSE; //(doPie && (ix>=segs-1));
		nc = (ix == segs) ? 1 : ix + 1;
		mesh.faces[initNumFaces + nf].setEdgeVisFlags(1, 1, 1);
		if ((issliceface) && (ix == segs - 1))
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 4 : 0);
			mesh.faces[initNumFaces + nf].setMatID(2);
		}
		else if ((issliceface) && (ix == segs))
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 8 : 0);
			mesh.faces[initNumFaces + nf].setMatID(3);
		}
		else
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 1 : 0);
			mesh.faces[initNumFaces + nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
//			mesh.faces[ initNumFaces + nf].setMatID(0); // mjm - 3.2.99 - was set to 1
		}
		mesh.faces[initNumFaces + nf].setVerts(initNumVerts + 0, initNumVerts + ix, initNumVerts + nc);
		nf++;
	}

	// Make midsection
	int lastseg = segs - 1, almostlast = lastseg - 1;
	BOOL weirdpt = FALSE;//doPie && !noHemi;
	BOOL weirdmid = FALSE;//weirdpt && (rows==2);
	for (ix = 1; ix < rows; ++ix) {
		jx = (ix - 1)*segs + 1;
		for (kx = 0; kx < segs; ++kx) {
			issliceface = (doPie && (kx >= almostlast));

			na = jx + kx;
			nb = na + segs;
			nb = (weirdmid && (kx == lastseg) ? lastvert : na + segs);
			if ((weirdmid) && (kx == almostlast)) nc = lastvert; else
				nc = (kx == lastseg) ? jx + segs : nb + 1;
			nd = (kx == lastseg) ? jx : na + 1;

			mesh.faces[initNumFaces + nf].setEdgeVisFlags(1, 1, 0);

			if ((issliceface) && ((kx == almostlast - 2) || (kx == almostlast)))
			{
				mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 4 : 0);
				mesh.faces[initNumFaces + nf].setMatID(2);
			}
			else if ((issliceface) && ((kx == almostlast - 1) || (kx == almostlast + 1)))
			{
				mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 8 : 0);
				mesh.faces[initNumFaces + nf].setMatID(3);
			}
			else
			{
				mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 1 : 0);
				mesh.faces[initNumFaces + nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
//				mesh.faces[ initNumFaces + nf].setMatID(0); // mjm - 3.2.99 - was set to 1
			}

			mesh.faces[initNumFaces + nf].setVerts(initNumVerts + na, initNumVerts + nb, initNumVerts + nc);
			nf++;

			mesh.faces[initNumFaces + nf].setEdgeVisFlags(0, 1, 1);

			if ((issliceface) && ((kx == almostlast - 2) || (kx == almostlast)))
			{
				mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 4 : 0);
				mesh.faces[initNumFaces + nf].setMatID(2);
			}
			else if ((issliceface) && ((kx == almostlast - 1) || (kx == almostlast + 1)))
			{
				mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 8 : 0);
				mesh.faces[initNumFaces + nf].setMatID(3);
			}
			else
			{
				mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 1 : 0);
				mesh.faces[initNumFaces + nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
//				mesh.faces[ initNumFaces + nf].setMatID(0); // mjm - 3.2.99 - was set to 1
			}

			mesh.faces[initNumFaces + nf].setVerts(initNumVerts + na, initNumVerts + nc, initNumVerts + nd);
			nf++;
		}
	}

	// Make bottom conic cap
	na = nverts - 1;//mesh.getNumVerts()-1;
	int botsegs = (weirdpt ? segs - 2 : segs);
	jx = (rows - 1)*segs + 1; lastseg = botsegs - 1;
	for (ix = 0; ix < botsegs; ++ix) {
		issliceface = (doPie && (ix >= botsegs - 2));
		nc = ix + jx;
		nb = (!weirdpt && (ix == lastseg) ? jx : nc + 1);
		mesh.faces[initNumFaces + nf].setEdgeVisFlags(1, 1, 1);

		if ((issliceface) && (noHemi) && (ix == botsegs - 2))
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 4 : 0);
			mesh.faces[initNumFaces + nf].setMatID(2);
		}
		else if ((issliceface) && (noHemi) && (ix == botsegs - 1))
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 8 : 0);
			mesh.faces[initNumFaces + nf].setMatID(3);
		}
		else if ((!issliceface) && (noHemi))
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 1 : 0);
			mesh.faces[initNumFaces + nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
//			mesh.faces[ initNumFaces + nf].setMatID(0); // mjm - 3.2.99 - was set to 1
		}
		else if (!noHemi)
		{
			mesh.faces[initNumFaces + nf].setSmGroup(smooth ? 2 : 0);
			mesh.faces[initNumFaces + nf].setMatID(0); // mjm - 5.5.99 - rollback change - should be fixed in later release
//			mesh.faces[ initNumFaces + nf].setMatID(1); // mjm - 3.2.99 - was set to 0
		}
		//		else
		//		{	mesh.faces[ initNumFaces + nf].setSmGroup(0);
		//			mesh.faces[ initNumFaces + nf].setMatID(noHemi?1:0); // mjm - 5.5.99 - rollback change - should be fixed in later release
		//			mesh.faces[ initNumFaces + nf].setMatID(noHemi?0:1); // mjm - 3.2.99 - was commented out but set to 1:0
		//		}

		mesh.faces[initNumFaces + nf].setVerts(initNumVerts + na, initNumVerts + nb, initNumVerts + nc);

		nf++;
	}

	if (genUVs) {
		int tvsegs = segs;
		int tvpts = (doPie ? segs + 1 : segs);
		int ntverts = (rows + 2)*(tvpts + 1);
		//		if (doPie) {ntverts-=6; if (weirdpt) ntverts-3;}
		mesh.setNumTVerts(ntverts);
		mesh.setNumTVFaces(nfaces);
		nv = 0;
		delta = basedelta;  // make the texture squash too
		alt = 0.0f; // = delta;
		int dsegs = (doPie ? 3 : 0), midsegs = tvpts - dsegs, t1 = tvpts + 1;
		for (ix = 0; ix < rows + 2; ix++) {
			//	if (!noHemi && ix==rows) alt = hemi;
			secang = 0.0f; //angle;
			float yang = 1.0f - alt / PI;
			for (jx = 0; jx <= midsegs; ++jx) {
				mesh.setTVert(nv++, secang / TWOPI, yang, 0.0f);
				secang += delta2;
			}
			for (jx = 0; jx < dsegs; jx++) mesh.setTVert(nv++, uval[jx], yang, 0.0f);
			alt += delta;
		}

		nf = 0; dsegs = (doPie ? 2 : 0), midsegs = segs - dsegs;
		// Make top conic cap
		for (ix = 0; ix < midsegs; ++ix) {
			mesh.tvFace[nf++].setTVerts(ix, ix + t1, ix + t1 + 1);
		} ix = midsegs + 1; int topv = ix + 1;
		for (jx = 0; jx < dsegs; jx++)
		{
			mesh.tvFace[nf++].setTVerts(topv, ix + t1, ix + t1 + 1); ix++;
		}
		int cpt;
		// Make midsection
		for (ix = 1; ix < rows; ++ix) {
			cpt = ix*t1;
			for (kx = 0; kx < tvsegs; ++kx) {
				if (kx == midsegs) cpt++;
				na = cpt + kx;
				nb = na + t1;
				nc = nb + 1;
				nd = na + 1;
				DbgAssert(nc < ntverts);
				DbgAssert(nd < ntverts);
				mesh.tvFace[nf++].setTVerts(na, nb, nc);
				mesh.tvFace[nf++].setTVerts(na, nc, nd);
			}
		}
		// Make bottom conic cap
		int lastv = rows*t1, jx = lastv + t1;
		if (weirdpt) dsegs = 0;
		int j1;
		for (j1 = lastv; j1 < lastv + midsegs; j1++) {
			mesh.tvFace[nf++].setTVerts(jx, j1 + 1, j1); jx++;
		}
		j1 = lastv + midsegs + 1; topv = j1 + t1 + 1;
		for (ix = 0; ix < dsegs; ix++)
		{
			mesh.tvFace[nf++].setTVerts(topv, j1 + 1, j1); j1++;
		}
		DbgAssert(nf == nfaces);
	}
	else {
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}
}

// Triangular patch layout:
//
//   A---> ac ----- ca <---C
//   |                    /
//   |                  /
//   v    i1    i3    /
//   ab            cb
//
//   |           /
//   |    i2   /
//
//   ba     bc
//   ^     /
//   |   /
//   | /
//   B
//
// vertices ( a b c d ) are in counter clockwise order when viewed from
// outside the surface

void MuscleStrand::BuildMesh(TimeValue t)
{
	UpdateHandles(t, ivalid);

	switch (GetDeformerType()) {
	case DEFORMER_BONES: {
		BuildSphereMesh(0, 1, t, mesh, 1.0f, Matrix3(1));
		break;
	};
	case DEFORMER_MESH:
	{
		Matrix3 tmInv = Inverse(tm);
		const int nSpheres = GetNumSpheres();
		for (int i = 0; i < nSpheres; i++)
		{
			SphereParams& aSphere = mTabSphereParams[i];
			Matrix3 tmLocal = aSphere.tmWorldSpace * tmInv;
			float radius = GetSphereRadius(i);
			BuildSphereMesh(i, nSpheres, t, mesh, radius, tmLocal);
		}
		break;
	}
	}

	if (!MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		mesh.BuildStripsAndEdges();
	}

	mesh.InvalidateTopologyCache();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Class MuscleStrand

void	MuscleStrand::SetFlag(USHORT f, BOOL tf/*=TRUE*/) {
	if ((tf && TestFlag(f)) || (!tf && !TestFlag(f))) return;

	// Do not automatically register a hold for this function.
	// Not all flag changes need to be held
//	HoldActions holdThis(IDS_HLD_MSCLSETTING);
	// Ensure we catch all the ones that are supposed to be held though
	DbgAssert(theHold.Holding() || theHold.IsSuspended() || STRANDFLAG_KEEP_ROLLOUTS == f);
	HoldData(flags, this);

	if (tf) flags |= f;
	else flags &= ~f;

	UpdateMuscleStrand();
}

void MuscleStrand::CreateHandleNode(int refid, DWORD dwWireColor, INode* nodeparent/*=NULL*/)
{
	// create the handle controller
	HdlTrans *hdltrans = (HdlTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HDLTRANS_CLASS_ID);

	INode *node = hdltrans->BuildNode(this, nodeparent, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE);

	Matrix3 tm(1);
	TimeValue t = GetCOREInterface()->GetTime();
	if (tabHandles[refid]) {
		tm = tabHandles[refid]->GetNodeTM(t);
		if (!nodeparent) {
			tabHandles[refid]->AttachChild(node, FALSE);
		}
	}
	tm = ls_hdl_tm[refid] * tm;
	node->SetNodeTM(t, tm);
	node->SetWireColor(dwWireColor);
	ReplaceReference(refid, node);
}

void MuscleStrand::CreateHandles(INode* pStrandNode)
{
	if (TestFlag(STRANDFLAG_HANDLES_VISIBLE))
		return;

	// We do not need to undo this action (it is temporary anyway)
	SetFlag(STRANDFLAG_KEEP_ROLLOUTS);
	{
		// register an Undo
		HoldActions holdThis(IDS_HLD_HANDLECREATE);

		DWORD dwWireColor = pStrandNode->GetWireColor();
		CreateHandleNode(NODE_ST, dwWireColor);
		CreateHandleNode(NODE_ST_HDL, dwWireColor, tabHandles[NODE_ST]);
		CreateHandleNode(NODE_EN, dwWireColor);
		CreateHandleNode(NODE_EN_HDL, dwWireColor, tabHandles[NODE_EN]);

		UpdateName(pStrandNode->GetName());
		SetFlag(STRANDFLAG_HANDLES_VISIBLE);
	}
	ClearFlag(STRANDFLAG_KEEP_ROLLOUTS);
}

void MuscleStrand::RemoveHandles() {
	if (!TestFlag(STRANDFLAG_HANDLES_VISIBLE)) return;

	// register an Undo
	HoldActions holdActions(IDS_HLD_HANDLEREM);

	// Handles
	INode*	tabHandlesTemp[NUMNODEREFS];

	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();
	INode *parent_node;
	int i, idx;

	const REFS removeOrder[NUMNODEREFS] = {
		NODE_ST_HDL, NODE_EN_HDL,
		NODE_ST, NODE_EN // Make sure root handles are in the end
	};

	for (i = 0; i < NUMNODEREFS; i++) {
		idx = removeOrder[i];
		if (tabHandles[idx]) {
			tabHandlesTemp[idx] = tabHandles[idx];
			parent_node = FindParent(tabHandles[idx]);
			if (parent_node) {
				ls_hdl_tm[idx] = tabHandles[idx]->GetNodeTM(t) * Inverse(parent_node->GetNodeTM(t));
			}
			else {
				ls_hdl_tm[idx] = tabHandles[idx]->GetNodeTM(t);
			}
			if (parent_node) {
				ReplaceReference(idx, parent_node);
			}
		}
		else {
			tabHandlesTemp[idx] = NULL;
		}
	}
	for (i = 0; i < NUMNODEREFS; i++) {
		if (tabHandlesTemp[i])	DeSelectAndDelete(tabHandlesTemp[i]);
	}
	ClearFlag(STRANDFLAG_HANDLES_VISIBLE);
	UpdateMuscleStrand();
}

TSTR MuscleStrand::IGetDeformerType()
{
	switch (deformer_type) {
	case DEFORMER_BONES:	return _T("Bones");	break;
	case DEFORMER_MESH:	return _T("Patch");	break;
	default: return _T("");
	}
};

void MuscleStrand::ISetDeformerType(const TSTR& s)
{
	if (s == muscle_bones_str)	SetDeformerType(DEFORMER_BONES);
	if (s == muscle_patch_str)	SetDeformerType(DEFORMER_MESH);
};

void MuscleStrand::SetDeformerType(DEFORMER_TYPE type)
{
	if (deformer_type == type)
	{
		return;
	}

	// Scope the catini, its not needed past this point
	{
		CatDotIni catini;
		catini.SetInt(KEY_MUSCLE_TYPE, type);
	}

	if (GetNumSpheres() == 0) {
		deformer_type = type;
		return;
	}

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }

	SetFlag(STRANDFLAG_KEEP_ROLLOUTS);

	switch (deformer_type) {
	case DEFORMER_BONES: {
		// Changing from Bones to Mesh
		const int nSpheres = GetNumSpheres();
		//	Tab <float>	tabTempSphereRadius	= tabSphereRadius;
		Tab <float>	tabTempSphereUStart = tabTempSphereUStart;
		Tab <float>	tabTempSphereUEnd = tabTempSphereUEnd;
		SetNumSpheres(1);
		UpdateMuscleStrand();

		HoldData(deformer_type);
		deformer_type = type;
		SetNumSpheres(nSpheres);
		tabTempSphereUStart = tabTempSphereUStart;
		tabTempSphereUEnd = tabTempSphereUEnd;
		break;
	}
	case DEFORMER_MESH: {
		// Changing from Mesh to Bones
		const int nSpheres = GetNumSpheres();
		Tab <float>	tabTempSphereUStart = tabTempSphereUStart;
		Tab <float>	tabTempSphereUEnd = tabTempSphereUEnd;
		SetNumSpheres(1);
		UpdateMuscleStrand();
		HoldData(deformer_type);

		deformer_type = type;

		// here we plug ourselves into the first slot in the muscle
		mTabSphereParams[0].pSphereNode = FindReferencingClass<INode>(this);

		SetNumSpheres(nSpheres);
		tabTempSphereUStart = tabTempSphereUStart;
		tabTempSphereUEnd = tabTempSphereUEnd;
		break;
	}
	}

	INode* pOwningNode = FindReferencingClass<INode>(this);
	if (pOwningNode != NULL)
		UpdateName(pOwningNode->GetName());

	ClearFlag(STRANDFLAG_KEEP_ROLLOUTS);
	if (newundo && theHold.Holding()) {
		theHold.Accept(TSTR(GetString(IDS_HLD_CONVERSION)) + IGetDeformerType() + GetString(IDS_HLD_MSCL));
		UpdateMuscleStrand();
	}
}

void MuscleStrand::AssignSegTransTMController(INode *node)
{
	if (!node || node->GetObjectRef() != this) {
		return;
	}

	SegTrans* seg = (SegTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, SEGTRANS_CLASS_ID);
	// instead of calling BuildSeg(which creates another INode and we already have one)
	// just put the correct values into the SegTrans
	seg->mWeakRefToObject.SetRef(this);
	seg->nStrandID = 0;
	seg->nSegID = 0;
	node->SetTMController(seg);
}

void MuscleStrand::UpdateMuscleStrand()
{
	handlesvalid = NEVER;

	switch (deformer_type) {
	case DEFORMER_BONES:
		for (int i = 0; i < GetNumSpheres(); i++) {
			INode* pSphere = mTabSphereParams[i].pSphereNode;
			if (pSphere)
			{
				pSphere->InvalidateTM();
				pSphere->InvalidateWS();
				pSphere->GetObjectRef()->NotifyDependents(FOREVER, PART_TM, REFMSG_OBJECT_CACHE_DUMPED);
			}
		}
		break;
	case DEFORMER_MESH:
		ivalid = NEVER;
		INode* node = FindReferencingClass<INode>(this);
		if (node) {
			node->InvalidateWS();
			node->InvalidateTM();
			NotifyDependents(FOREVER, PART_TM, REFMSG_OBJECT_CACHE_DUMPED);
		}
		break;
	}

	RefreshUI();
}

void MuscleStrand::UpdateName(const MCHAR* nodeName)
{
	MSTR newname = nodeName;
	switch (GetLMR()) {
	case -1:	newname = newname + _T("L");		break;
	case 0:		newname = newname + _T("M");		break;
	case 1:		newname = newname + _T("R");		break;
	default:	DbgAssert(0);
	}
	if (tabHandles[NODE_ST])		tabHandles[NODE_ST]->SetName(newname + GetString(IDS_MNAME_START));
	if (tabHandles[NODE_ST_HDL])	tabHandles[NODE_ST_HDL]->SetName(newname + GetString(IDS_MNAME_STARTHDL));
	if (tabHandles[NODE_EN])		tabHandles[NODE_EN]->SetName(newname + GetString(IDS_MNAME_END));
	if (tabHandles[NODE_EN_HDL])	tabHandles[NODE_EN_HDL]->SetName(newname + GetString(IDS_MNAME_ENDHDL));
}

void MuscleStrand::SetColour(Color* clr) {
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }
	//	if (theHold.Holding())	theHold.Put(new SetColourRestore(this));
	clrColour = *clr;
	//	UpdateColours();
	if (newundo && theHold.Holding()) {
		theHold.Accept(GetString(IDS_HLD_MSCLCOLOR));
	}
};

void MuscleStrand::SetName(const TSTR& newname) {
	if (strName == newname) return;
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }
	//	if (theHold.Holding())	theHold.Put( new MuscleNameRestore(this) );
	strName = newname;
	UpdateName(strName);
	if (newundo && theHold.Holding())	theHold.Accept(GetString(IDS_HLD_MSCLNAME));
};

float MuscleStrand::GetLength()
{
	//	return  Length(ws_hdl_tm[NODE_ST].GetTrans()	-	ws_hdl_tm[NODE_ST_HDL].GetTrans()) + \
	//			Length(ws_hdl_tm[NODE_ST_HDL].GetTrans() -	ws_hdl_tm[NODE_EN_HDL].GetTrans()) + \
	//			Length(ws_hdl_tm[NODE_EN_HDL].GetTrans() - ws_hdl_tm[NODE_EN].GetTrans());

	lengthCurrent = 0.0f;
	Point3 p1, p2;
	const int nSpheres = GetNumSpheres();
	for (int i = 0; i <= nSpheres; i++) {
		Point3 p2 = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), (float)i / (float)nSpheres);
		if (i == 0) p1 = p2;
		else lengthCurrent += Length(p2 - p1);
	}
	return lengthCurrent;
}

void MuscleStrand::UpdateHandles(TimeValue t, Interval&valid)
{
	if (!handlesvalid.InInterval(t)) {
		handlesvalid = FOREVER;

		Matrix3 tm;
		for (int i = 0; i < NUMNODEREFS; i++) {
			if (tabHandles[i])	tm = tabHandles[i]->GetNodeTM(t, &handlesvalid);
			else				tm.IdentityMatrix();
			if (TestFlag(STRANDFLAG_HANDLES_VISIBLE))
				ws_hdl_tm[i] = tm;
			else ws_hdl_tm[i] = ls_hdl_tm[i] * tm;
		}

		// the transform for the strand object
		tm = ws_hdl_tm[NODE_ST];
		BlendMat(tm, ws_hdl_tm[NODE_EN], 0.5f);
		tm.SetTrans((ws_hdl_tm[NODE_ST].GetTrans() + ws_hdl_tm[NODE_EN].GetTrans() + ws_hdl_tm[NODE_ST_HDL].GetTrans() + ws_hdl_tm[NODE_EN_HDL].GetTrans()) / 4.0f);
		Matrix3 tmInv = Inverse(tm);
		this->tm = tm;

		if (TestFlag(STRANDFLAG_ENABLE_SQUASHSTRETCH)) {
			lengthCurrent = GetLength();
			//	scaleCurrent = 1.0 + (((lengthShort/lengthCurrent)-1.0f) * scaleShort);
			scaleCurrent = 1.0f / (lengthCurrent / lengthShort);
			scaleCurrent = ((scaleCurrent - 1.0f) * scaleShort) + 1.0f;
		}
		else {
			scaleCurrent = 1.0f;
		}

		// now calculate the transforms for the spheres
		const int nSpheres = GetNumSpheres();
		for (int i = 0; i < nSpheres; i++)
		{
			float radius = GetSphereRadius(i);
			SphereParams& aSphere = mTabSphereParams[i];
			Matrix3& tmWorldSphere = aSphere.tmWorldSpace;
			tmWorldSphere = ws_hdl_tm[NODE_ST];

			// Linearly interpollate the handles
			BlendMat(tmWorldSphere, ws_hdl_tm[NODE_EN], (aSphere.UStart + aSphere.UEnd) / 2.0f);

			// NBow plce the sphere on the bezier curve
			Point3 p1 = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), aSphere.UStart);
			Point3 p2 = CalcBezier(ws_hdl_tm[NODE_ST].GetTrans(), ws_hdl_tm[NODE_ST_HDL].GetTrans(), ws_hdl_tm[NODE_EN_HDL].GetTrans(), ws_hdl_tm[NODE_EN].GetTrans(), aSphere.UEnd);
			Point3 dir = p2 - p1;
			aSphere.tmWorldSpace.SetTrans((p1 + p2) / 2.0f);

			// Align the sphere to the bezier curve
			RotMatToLookAtPoint(tmWorldSphere, X, FALSE, p2);
			tmWorldSphere.SetRow(Y, tmWorldSphere.GetRow(Y) * radius * scaleCurrent);
			tmWorldSphere.SetRow(Z, tmWorldSphere.GetRow(Z) * radius * scaleCurrent);
			tmWorldSphere.SetRow(X, dir / 2.0f);

			aSphere.tmLocalSpace = tmWorldSphere * tmInv;
		}
	}
	// every bone segment shares this one validity interval
	valid = handlesvalid;
}

void MuscleStrand::GetTrans(TimeValue t, int nStrand, int nSeg, Matrix3 &tm, Interval &valid)
{
	UNREFERENCED_PARAMETER(nSeg);
	UpdateHandles(t, valid);

	if (nStrand >= 0 && nStrand < GetNumSpheres())
		switch (deformer_type) {
		case DEFORMER_BONES: {
			tm = mTabSphereParams[nStrand].tmWorldSpace;
			break;
		}
		case DEFORMER_MESH: {
			tm = this->tm;
			break;
		}
		}
};

void  MuscleStrand::SetHandleSize(float sz) {

	HoldData(handlesize, this);

	handlesize = sz;
	for (int i = 0; i < NUMNODEREFS; i++)
	{
		INode* pHdlNode = tabHandles[i];
		if (pHdlNode != NULL)
		{
			pHdlNode->InvalidateWS();

			HdlObj* pHdlObj = dynamic_cast<HdlObj*>(pHdlNode->GetObjectRef());
			if (pHdlObj != NULL)
				pHdlObj->SetSize(handlesize);
		}
	}
}

void  MuscleStrand::SetHandlesVisible(BOOL tf)
{
	if (tf)
	{
		INode* pMyNode = FindReferencingClass<INode>(this);
		if (pMyNode != NULL)
			CreateHandles(pMyNode);
	}
	else
		RemoveHandles();
};

void	MuscleStrand::SetNumSpheres(int n)
{
	int oldnSpheres = GetNumSpheres();
	int nSpheres = n;

	SetFlag(STRANDFLAG_KEEP_ROLLOUTS);
	{
		HoldActions holdActions(IDS_HLD_NUMSPHERES);
		HoldData(mTabSphereParams, this);

		// Increase size, if necessary
		if (nSpheres > oldnSpheres)
			mTabSphereParams.SetCount(nSpheres);

		for (int i = 0; i < nSpheres; i++)
		{
			SphereParams& aSphere = mTabSphereParams[i];
			aSphere.UStart = ((float)i) / ((float)nSpheres) - ((1.0f / (nSpheres)) / 2.0f);
			aSphere.UEnd = ((float)i + 1.0f) / ((float)nSpheres) + ((1.0f / (nSpheres)) / 2.0f);

			// If our constructor worked, we wouldn't have to do this...
			if (i >= oldnSpheres)
			{
				aSphere.pSphereNode = NULL;
				aSphere.tmLocalSpace = aSphere.tmWorldSpace = Matrix3(1);
			}
		}

		switch (deformer_type) {
		case DEFORMER_BONES:
			handlesvalid = NEVER;

			// remove excessive segs
			if (nSpheres < oldnSpheres) {
				if (!theHold.RestoreOrRedoing())
				{
					for (int i = oldnSpheres - 1; i >= nSpheres; i--)
						DeSelectAndDelete(mTabSphereParams[i].pSphereNode);
					if (nSpheres > 0)
						GetCOREInterface()->SelectNode(mTabSphereParams[0].pSphereNode);
				}
			}
			// add new segs
			else if (nSpheres > oldnSpheres)
			{
				INode* pSrcNode = (oldnSpheres > 0) ? mTabSphereParams[0].pSphereNode : NULL;
				for (int i = oldnSpheres; i < nSpheres; i++)
				{
					SegTrans* segtrans = (SegTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, SEGTRANS_CLASS_ID);
					pSrcNode = segtrans->BuildNode(this, i, pSrcNode);
					mTabSphereParams[i].pSphereNode = pSrcNode;
				}
			}

			break;
		case DEFORMER_MESH:
		{
			// We don't need to do anything here, but we do need
			// to tell Max that our object has changed
			NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			break;
		}
		}

		// De-allocate space now, if necessary
		if (oldnSpheres > nSpheres)
			mTabSphereParams.SetCount(nSpheres);
	}
	ClearFlag(STRANDFLAG_KEEP_ROLLOUTS);
};

Matrix3 MuscleStrand::GetLSHandleTM(int id, TimeValue t) {
	if (GetHandlesVisible() && tabHandles[id])
	{
		if (tabHandles[id]->GetParentNode()->IsRootNode())
			return tabHandles[id]->GetNodeTM(t);
		else return tabHandles[id]->GetNodeTM(t) * Inverse(tabHandles[id]->GetParentNode()->GetNodeTM(t));
	}
	else return ls_hdl_tm[id];
}

void MuscleStrand::SetLSHandleTM(int id, TimeValue t, Matrix3 &tm, BOOL bMirror) {
	if (bMirror)
		MirrorMatrix(tm, mirroraxis);

	if (GetHandlesVisible() && tabHandles[id])
	{
		if (!tabHandles[id]->GetParentNode()->IsRootNode())
		{
			tm = tm * tabHandles[id]->GetParentNode()->GetNodeTM(t);
		}
		tabHandles[id]->SetNodeTM(t, tm);
	}
	else ls_hdl_tm[id] = tm;;
}

Matrix3 GetWSHandleTM(MuscleStrand *musclestrand, int id, TimeValue t) {
	if (musclestrand->GetHandlesVisible() && musclestrand->GetHandles()[id])
	{
		return musclestrand->GetHandles()[id]->GetNodeTM(t);
	}
	else
	{
		if (musclestrand->GetHandles()[id])
		{
			return musclestrand->GetLSHandleTM(id, t) * musclestrand->GetHandles()[id]->GetNodeTM(t);
		}
		else
		{
			return musclestrand->GetLSHandleTM(id, t);
		}
	}
}

void SetWSHandleTM(MuscleStrand *musclestrand, int id, TimeValue t, Matrix3 &tm, BOOL bMirror) {
	if (bMirror)
		MirrorMatrix(tm, musclestrand->GetMirrorAxis());

	if (musclestrand->GetHandlesVisible() && musclestrand->GetHandles()[id])
	{
		musclestrand->GetHandles()[id]->SetNodeTM(t, tm);
	}
	else
	{
		if (musclestrand->GetHandles()[id])
		{
			tm = tm * Inverse(musclestrand->GetHandles()[id]->GetNodeTM(t));
		}
		musclestrand->SetLSHandleTM(id, t, tm, FALSE);
	}
}

void MuscleStrand::PasteMuscleStrand(ReferenceTarget* pasteRef)
{
	MuscleStrand* pasteMuscleStrand = nullptr;

	if (pasteRef != nullptr)
	{
		if (pasteRef->ClassID() == ClassID())
		{
			pasteMuscleStrand = (MuscleStrand*)pasteRef;
		}
		else
		{
			// In the case that the function is called by MAXScript,
			// it's the node that contains the MuscleStrand passed in.
			INode* node = static_cast<INode*>(pasteRef->GetInterface(INODE_INTERFACE));
			if (node != nullptr)
			{
				ObjectState os = node->EvalWorldState(MAXScript_time());
				if ((os.obj != nullptr) && (os.obj->ClassID() == ClassID()))
				{
					pasteMuscleStrand = (MuscleStrand*)os.obj;
				}
			}
		}
	}

	if (pasteMuscleStrand == nullptr) return;

	HoldActions holdThis(IDS_HLD_STRANDPASTE);
	TimeValue t = GetCOREInterface()->GetTime();
	BOOL bMirror = FALSE;

	// if this is a left paste onto a right then mirror
	if ((GetLMR() != pasteMuscleStrand->GetLMR()) && (GetLMR() + pasteMuscleStrand->GetLMR() == 0))
		bMirror = TRUE;

	int nSpheres = pasteMuscleStrand->GetNumSpheres();
	SetNumSpheres(nSpheres);
	SetHandleSize(pasteMuscleStrand->GetHandleSize());

	for (int i = 0; i < nSpheres; i++) {
		SetSphereUStart(i, pasteMuscleStrand->GetSphereUStart(i));
		SetSphereUEnd(i, pasteMuscleStrand->GetSphereUEnd(i));
	}

	SetDefaultLength(pasteMuscleStrand->GetDefaultLength());
	SetSquashStretchScale(pasteMuscleStrand->GetSquashStretchScale());

	if (mpCCtl && pasteMuscleStrand->mpCCtl) {
		ICurve *pCurve = pasteMuscleStrand->mpCCtl->GetControlCurve(0);
		if (pCurve)
		{
			mpCCtl->GetControlCurve(0)->SetNumPts(pCurve->GetNumPts());
			for (int i = 0; i < mpCCtl->GetControlCurve(0)->GetNumPts(); i++) {
				CurvePoint pt = pCurve->GetPoint(0, i);
				mpCCtl->GetControlCurve(0)->SetPoint(0, i, &pt, TRUE, FALSE);
			}
		}
	}

	Matrix3 m;
	m = GetWSHandleTM(pasteMuscleStrand, NODE_ST, t); SetWSHandleTM(this, NODE_ST, t, m, bMirror);
	m = GetWSHandleTM(pasteMuscleStrand, NODE_ST_HDL, t); SetWSHandleTM(this, NODE_ST_HDL, t, m, bMirror);
	m = GetWSHandleTM(pasteMuscleStrand, NODE_EN, t); SetWSHandleTM(this, NODE_EN, t, m, bMirror);
	m = GetWSHandleTM(pasteMuscleStrand, NODE_EN_HDL, t); SetWSHandleTM(this, NODE_EN_HDL, t, m, bMirror);

	// Paste the Squash and sterch values.
	SetFlag(STRANDFLAG_ENABLE_SQUASHSTRETCH, pasteMuscleStrand->TestFlag(STRANDFLAG_ENABLE_SQUASHSTRETCH));
}

// all this undo really does is stop lots of different muscle segments all
// calling setvalue at once and accumulating thier results. When the first set value
// somes through the flag A_HELD is set, and then no more SetValues can get through
// untill the restore has happened. The restores happen constantly throughtout and undo
class MuscleStrandMoveRestore : public RestoreObj {
public:
	MuscleStrand *muscle;

	MuscleStrandMoveRestore() { muscle = NULL; };
	MuscleStrandMoveRestore(MuscleStrand *c) {
		muscle = c;
	}
	~MuscleStrandMoveRestore() {}
	void Restore(int isUndo) {
		UNREFERENCED_PARAMETER(isUndo);
		if (muscle) muscle->ClearAFlag(A_HELD);
	}
	void Redo() {}
	int Size() { return sizeof(MuscleStrandMoveRestore); }
	void EndHold() { muscle->ClearAFlag(A_HELD); }
};

// this is for moving a muscle around using the segments.
// it allows the user to treat the whole musle as one object when moving and rotating
// it in the viewport. When the user tries to move or rotate one segment this
// method in turn, transforms the corner handles effectively moving the whole muscle
void MuscleStrand::SetValue(TimeValue t, SetXFormPacket *ptr, int commit, Control* /*pOriginator*/)
{
	// the undo just manages removing the A_HELD flag
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		for (int i = 0; i < NUMNODEREFS; i += 2) {
			if (tabHandles[i]) {
				SetXFormPacket setval = *ptr;
				// this forces all the handles to rotate around the same axis
				setval.localOrigin = 0;
				setval.tmParent = tabHandles[i]->GetParentTM(t);
				tabHandles[i]->GetTMController()->SetValue(t, (void*)&setval, commit, CTRL_RELATIVE);
			}
		}
		theHold.Put(new MuscleStrandMoveRestore(this));
		SetAFlag(A_HELD);
		UpdateMuscleStrand();
	}
};

void MuscleStrand::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	int i;
	switch (ctxt) {
		// if we are merging, force all the collision spheres to be merged also
	case kSNCFileMerge:
	case kSNCFileSave:
	case kSNCDelete:
	case kSNCClone:
		// if we add these nodes to the mapping for the Clone method, and they do
		// not get automaticly cloned before the muslce does(and this almost never happens)
		// then we are forceed to clone them manually and then we find that they get cloned twice.
		if (GetHandlesVisible())
		{
			for (i = 0; i < NUMNODEREFS; i++)
			{
				if (tabHandles[i])
				{
					nodes.AppendNode(tabHandles[i], false);
				}
			}
		}

		switch (deformer_type)
		{
		case DEFORMER_BONES:
			for (int i = 0; i < GetNumSpheres(); i++)
				nodes.AppendNode(mTabSphereParams[i].pSphereNode);
			break;
		case DEFORMER_MESH:
			break;
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//

#define MUSCLESTRAND_VERSION					0
#define MUSCLESTRAND_FLAGS_CHUNK				2
#define MUSCLESTRAND_DEFORMER_TYPE				3
#define MUSCLESTRAND_LMR_CHUNK					4
#define MUSCLESTRAND_COLOUR_CHUNK				6
#define MUSCLESTRAND_HANDLESIZE_CHUNK			8
#define MUSCLESTRAND_NODE						10

#define MUSCLESTRAND_NUMSPHERES_CHUNK			12
#define MUSCLESTRAND_SPHERERADIUS_CHUNK			14
#define MUSCLESTRAND_SPHEREUSTART_CHUNK			16
#define MUSCLESTRAND_SPHEREUEND_CHUNK			18
#define	MUSCLESTRAND_SHPERE_POINTERS_CHUNK		19

#define MUSCLESTRAND_SHORT_LENGTH_CHUNK			20
#define MUSCLESTRAND_SHORT_SCALE_CHUNK			22
#define MUSCLESTRAND_LONG_LENGTH_CHUNK			24
#define MUSCLESTRAND_LONG_SCALE_CHUNK			26

#define MUSCLESTRAND_MIRRORAXIS_CHUNK			28
#define MUSCLESTRAND_NUMSAVES_CHUNK				30
#define	MUSCLESTRAND_LS_HDL_TMS					32

IOResult MuscleStrand::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L, refID = 0L;
	int i;//, j;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case MUSCLESTRAND_VERSION:
			res = iload->Read(&dVersion, sizeof(DWORD), &nb);
			break;
		case MUSCLESTRAND_FLAGS_CHUNK:
			res = iload->Read(&flags, sizeof USHORT, &nb);
			break;
		case MUSCLESTRAND_DEFORMER_TYPE:
			res = iload->ReadEnum(&deformer_type, sizeof(int), &nb);
			break;
		case MUSCLESTRAND_LMR_CHUNK:
			res = iload->Read(&lmr, sizeof(int), &nb);
			break;
		case MUSCLESTRAND_MIRRORAXIS_CHUNK:
			res = iload->ReadVoid(&mirroraxis, sizeof(Axis), &nb);
			break;
		case MUSCLESTRAND_HANDLESIZE_CHUNK:
			res = iload->Read(&handlesize, sizeof(float), &nb);
			break;

		case MUSCLESTRAND_NUMSPHERES_CHUNK:
		{
			int nSpheres = 0;
			res = iload->Read(&nSpheres, sizeof(int), &nb);
			mTabSphereParams.SetCount(nSpheres);
			break;
		}
		case MUSCLESTRAND_SHPERE_POINTERS_CHUNK:
		{
			int nSpheres = mTabSphereParams.Count();
			for (i = 0; i < nSpheres; i++) {
				res = iload->Read(&refID, sizeof DWORD, &nb);
				if (res == IO_OK && refID != (DWORD)-1)
					iload->RecordBackpatch(refID, (void**)&(mTabSphereParams[i].pSphereNode));
			}
		}
		break;
		case MUSCLESTRAND_SPHEREUSTART_CHUNK:
		{
			int nSpheres = mTabSphereParams.Count();
			for (i = 0; i < nSpheres && res == IO_OK; i++)
				res = iload->Read(&mTabSphereParams[i].UStart, sizeof(float), &nb);
			break;
		}
		case MUSCLESTRAND_SPHEREUEND_CHUNK:
		{
			int nSpheres = mTabSphereParams.Count();
			for (i = 0; i < nSpheres && res == IO_OK; i++)
				res = iload->Read(&mTabSphereParams[i].UEnd, sizeof(float), &nb);
		}
		break;
		case MUSCLESTRAND_SHORT_LENGTH_CHUNK:
			res = iload->Read(&lengthShort, sizeof(float), &nb);
			break;
		case MUSCLESTRAND_SHORT_SCALE_CHUNK:
			res = iload->Read(&scaleShort, sizeof(float), &nb);
			break;
		case MUSCLESTRAND_LS_HDL_TMS:
			for (i = 0; i < NUMNODEREFS; i++)
				res = iload->Read(&ls_hdl_tm[i], sizeof(Matrix3), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new HdlObjOwnerPLCB<MuscleStrand>(*this));

	// this flag should always be set for loaded files
	return IO_OK;
}

IOResult MuscleStrand::Save(ISave *isave)
{
	DWORD nb, refID;
	int i;

	isave->BeginChunk(MUSCLESTRAND_VERSION);
	dVersion = CAT_VERSION_CURRENT;
	isave->Write(&dVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_FLAGS_CHUNK);
	isave->Write(&flags, sizeof(USHORT), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_DEFORMER_TYPE);
	isave->WriteEnum(&deformer_type, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_LMR_CHUNK);
	isave->Write(&lmr, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_MIRRORAXIS_CHUNK);
	isave->WriteVoid(&mirroraxis, sizeof(Axis), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_HANDLESIZE_CHUNK);
	isave->Write(&handlesize, sizeof(float), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_SHORT_LENGTH_CHUNK);
	isave->Write(&lengthShort, sizeof(float), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_SHORT_SCALE_CHUNK);
	isave->Write(&scaleShort, sizeof(float), &nb);
	isave->EndChunk();

	const int nSpheres = GetNumSpheres();
	isave->BeginChunk(MUSCLESTRAND_NUMSPHERES_CHUNK);
	isave->Write(&nSpheres, sizeof(int), &nb);
	isave->EndChunk();

	if (deformer_type == DEFORMER_BONES) {
		isave->BeginChunk(MUSCLESTRAND_SHPERE_POINTERS_CHUNK);
		for (i = 0; i < nSpheres; i++)
		{
			INode* pNode = mTabSphereParams[i].pSphereNode;
			if (pNode != NULL) {
				refID = isave->GetRefID(pNode);
				isave->Write(&refID, sizeof DWORD, &nb);
			}
		}
		isave->EndChunk();
	}

	isave->BeginChunk(MUSCLESTRAND_SPHEREUSTART_CHUNK);
	for (i = 0; i < nSpheres; i++)
	{
		isave->Write(&mTabSphereParams[i].UStart, sizeof(float), &nb);
	}
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_SPHEREUEND_CHUNK);
	for (i = 0; i < nSpheres; i++)
	{
		isave->Write(&mTabSphereParams[i].UEnd, sizeof(float), &nb);
	}
	isave->EndChunk();

	isave->BeginChunk(MUSCLESTRAND_LS_HDL_TMS);
	for (i = 0; i < NUMNODEREFS; i++)
		isave->Write(&ls_hdl_tm[i], sizeof(Matrix3), &nb);
	isave->EndChunk();

	return IO_OK;
}

