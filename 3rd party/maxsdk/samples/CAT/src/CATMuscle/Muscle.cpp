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

#include "Muscle.h"
#include "HdlObj.h"
#include "MaxIcon.h"
#include <Util/StaticAssert.h>
#include <maxscript/maxwrapper/mxsobjects.h>

#include "SegTrans.h"
#include "HdlTrans.h"
#include "../CATObjects/CATDotIni.h"

class PickColObjNodeFilter : public PickNodeCallback
{
private:
	Muscle* picky;
public:
	void SetMuscle(Muscle* m) { picky = m; }
	BOOL Filter(INode *node)
	{
		// don't allow selection if it would cause a reference cycle - SA 9/09
		Interval ival;
		ival.SetInfinite();
		RefResult res = node->TestForLoop(ival, picky);
		if (res != REF_SUCCEED)
			return FALSE;

		//	if (node->GetObjectRef()->ClassID() == Class_ID(SPHERE_CLASS_ID, 0)){
	/*			if  ((node->GetObjectRef()->ClassID() == HDLOBJ_CLASS_ID)||
					(node->GetObjectRef()->ClassID() == MUSCLEPATCH_CLASS_ID)||
					(node->GetObjectRef()->ClassID() == MUSCLEBONES_CLASS_ID))
					return FALSE;
				if(!node->EvalWorldState(GetCOREInterface()->GetTime()).obj->CanConvertToType(triObjectClassID))
					return FALSE;
				for(int i=0; i<picky->GetNumColObjs(); i++)
					if(picky->GetColObj(i)==node)
						return FALSE;
	*/			return TRUE;
	//	}
	//	return FALSE;
	};
};

static PickColObjNodeFilter ColObjFilter;

#define LBL_LENGTH 64

class MuscleDlgCallBack : public PickModeCallback, public TimeChangeCallback
{
private:
	Muscle *muscle;

	ICustEdit *edtName;
	IColorSwatch *swColour;

	ISpinnerControl *spnNumVSegs;
	ISpinnerControl *spnNumUSegs;
	ISpinnerControl *spnHandleSize;

	ICustButton *btnCopy;
	ICustButton *btnPaste;

	HWND lbxColObjs;
	int collisionobj_index;

	ICustButton *btnAddColObj;
	ICustButton *btnRemoveColObj;

	ISliderControl *sldMuscleUp;
	ISliderControl *sldColObjHardness;

	ICustButton *btnMoveUp;
	ICustButton *btnMoveDown;

	HWND hWnd;

public:

	HWND GetHWnd() { return hWnd; }

	BOOL HitTest(IObjParam *ip, HWND hWnd, ViewExp *, IPoint2 m, int flags)
	{
		UNREFERENCED_PARAMETER(flags);
		return ip->PickNode(hWnd, m, &ColObjFilter) ? TRUE : FALSE;
	}
	BOOL Pick(IObjParam *, ViewExp *vpt)
	{
		INode *node = vpt->GetClosestHit();
		//		if(node->GetObjectRef()->ClassID() == Class_ID(SPHERE_CLASS_ID, 0))
		//		{
		DbgAssert(muscle);
		muscle->AddColObj(node);
		btnAddColObj->SetCheck(FALSE);
		RefreshColObjsList();
		return TRUE;
		//		}
		//		return FALSE;
	}
	PickNodeCallback *GetFilter() { return &ColObjFilter; }
	BOOL	RightClick(IObjParam *, ViewExp *) { return TRUE; }

	void RefreshColObjsList()
	{
		if (!muscle) return;

		collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
		SendMessage(lbxColObjs, LB_RESETCONTENT, 0, 0);
		for (int i = 0; i < muscle->GetNumColObjs(); i++) {
			INode* pColObj = muscle->GetColObj(i);
			if (pColObj != NULL)
			{
				TSTR strLimbName = pColObj->GetName();
				SendMessage(lbxColObjs, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)strLimbName.data());
			}
		}
		if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0) && (collisionobj_index < muscle->GetNumColObjs()))
			SendMessage(lbxColObjs, LB_SETCURSEL, collisionobj_index, 0);
	}

	void Refresh(TimeValue t)
	{
		if (!muscle) return;

		SET_CHECKED(hWnd, IDC_RDO_PATCH_MUSCLE, muscle->GetDeformerType() == Muscle::DEFORMER_MESH);
		SET_CHECKED(hWnd, IDC_RDO_BONES_MUSCLE, muscle->GetDeformerType() == Muscle::DEFORMER_BONES);
		EnableWindow(GetDlgItem(hWnd, IDC_CHK_REMOVE_SKEW), muscle->GetDeformerType() == Muscle::DEFORMER_BONES);
		SET_CHECKED(hWnd, IDC_CHK_REMOVE_SKEW, muscle->GetRemoveSkew());

		int lmr = muscle->GetLMR();
		SET_CHECKED(hWnd, IDC_RDO_LEFT, lmr == -1);
		SET_CHECKED(hWnd, IDC_RDO_MIDDLE, lmr == 0);
		SET_CHECKED(hWnd, IDC_RDO_RIGHT, lmr == 1);

		int mirroraxis = muscle->GetMirrorAxis();
		SET_CHECKED(hWnd, IDC_RDO_X, mirroraxis == 0);
		SET_CHECKED(hWnd, IDC_RDO_Y, mirroraxis == 1);
		SET_CHECKED(hWnd, IDC_RDO_Z, mirroraxis == 2);

		edtName->SetText(this->muscle->GetName());
		EnableWindow(GetDlgItem(hWnd, IDC_BTN_PASTE), muscleCopy != NULL/* && muscleCopy!=muscle*/);

		spnNumVSegs->SetValue(muscle->GetNumVSegs(), FALSE);
		spnNumUSegs->SetValue(muscle->GetNumUSegs(), FALSE);
		spnHandleSize->SetValue(muscle->GetHandleSize(), FALSE);

		SET_CHECKED(hWnd, IDC_CHK_HDLVISIBLE, muscle->GetHandlesVisible());
		SET_CHECKED(hWnd, IDC_CHK_MIDDLE_HDLS, muscle->GetMiddleHandles());
		EnableWindow(GetDlgItem(hWnd, IDC_CHK_HDLVISIBLE), muscle->IsCreated());
		EnableWindow(GetDlgItem(hWnd, IDC_SPIN_HANDLESIZE), muscle->GetHandlesVisible());
		EnableWindow(GetDlgItem(hWnd, IDC_CHK_MIDDLE_HDLS), muscle->IsCreated() && muscle->GetHandlesVisible());

		Color *clr = muscle->GetColour();
		swColour->SetColor(*clr, FALSE);

		collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
		int nNumColObjs = muscle->GetNumColObjs();
		if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0) && (collisionobj_index < nNumColObjs)) {

			btnRemoveColObj->Enable(TRUE);

			// VS doesn't seem able to resolve to the correct overload here: force it to find the correct function
			IMuscle* pMuscle = muscle;
			float dHardness = pMuscle->GetColObjHardness(collisionobj_index, t);
			sldColObjHardness->Enable(TRUE);
			sldColObjHardness->SetValue(dHardness, FALSE);

			sldMuscleUp->Enable(TRUE);
			sldMuscleUp->SetValue(pMuscle->GetColObjDistortion(collisionobj_index, t), FALSE);

			EnableWindow(GetDlgItem(hWnd, IDC_RDO_VERT_NORMAL), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_RDO_OBJX), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHK_SMOOTH), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHK_INVERT), TRUE);

			SET_CHECKED(hWnd, IDC_RDO_VERT_NORMAL, !muscle->GetObjXDistortion(collisionobj_index));
			SET_CHECKED(hWnd, IDC_RDO_OBJX, muscle->GetObjXDistortion(collisionobj_index));

			SET_CHECKED(hWnd, IDC_CHK_SMOOTH, muscle->GetSmoothCollision(collisionobj_index));
			SET_CHECKED(hWnd, IDC_CHK_INVERT, muscle->GetInvertCollision(collisionobj_index));

			if (collisionobj_index > 0)				btnMoveUp->Enable(TRUE);
			else						btnMoveUp->Enable(FALSE);
			if (collisionobj_index < (nNumColObjs - 1))	btnMoveDown->Enable(TRUE);
			else						btnMoveDown->Enable(FALSE);
		}
		else {
			btnRemoveColObj->Enable(FALSE);
			sldColObjHardness->Enable(FALSE);
			sldMuscleUp->Enable(FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RDO_VERT_NORMAL), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RDO_OBJX), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHK_SMOOTH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHK_INVERT), FALSE);
			btnMoveUp->Enable(FALSE);
			btnMoveDown->Enable(FALSE);
		}

	}
	void Refresh()
	{
		RefreshColObjsList();
		Refresh(GetCOREInterface()->GetTime());
	}
	void TimeChanged(TimeValue t) {
		Refresh(t);
	}

	void InitControls(HWND hDlg, Muscle *muscle)
	{
		hWnd = hDlg;
		this->muscle = muscle;
		CatDotIni catini;

		edtName = GetICustEdit(GetDlgItem(hWnd, IDC_EDIT_NAME));

		Color *clr = muscle->GetColour();
		swColour = GetIColorSwatch(GetDlgItem(hWnd, IDC_COLOURSWATCH_START), RGB((int)(clr->r*255.0), (int)(clr->g*255.0), (int)(clr->g*255.0)), *muscle->GetName() + GetString(IDS_START1));
		swColour->SetModal();

		spnNumVSegs = SetupIntSpinner(hWnd, IDC_SPIN_NUMVSEGS, IDC_EDIT_NUMVSEGS, 1, 100, muscle->GetNumVSegs());
		spnNumUSegs = SetupIntSpinner(hWnd, IDC_SPIN_NUMUSEGS, IDC_EDIT_NUMUSEGS, 1, 100, muscle->GetNumUSegs());
		spnHandleSize = SetupFloatSpinner(hWnd, IDC_SPIN_HANDLESIZE, IDC_EDIT_HANDLESIZE, 0.001f, 100.0f, muscle->GetHandleSize());

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hWnd, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnCopy"), GetString(IDS_TT_COPYMCL)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnPaste"), GetString(IDS_TT_PASTEMCL)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		lbxColObjs = GetDlgItem(hWnd, IDC_LIST_SPHERES);

		btnAddColObj = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADD_SPHERE));
		btnAddColObj->SetType(CBT_CHECK);
		btnAddColObj->SetButtonDownNotify(TRUE);
		btnAddColObj->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnAddColObj"), GetString(IDS_TT_PICKMSCLOBJ)));
		btnAddColObj->SetHighlightColor(BLUE_WASH);

		// Initialise the Delete Button
		btnRemoveColObj = GetICustButton(GetDlgItem(hWnd, IDC_BTN_REMOVE_SPHERE));
		btnRemoveColObj->SetType(CBT_PUSH);
		btnRemoveColObj->SetButtonDownNotify(TRUE);
		btnRemoveColObj->SetTooltip(TRUE, catini.Get(_T("ToolTips"), _T("btnRemoveColObj"), GetString(IDS_TT_REMCLSNOBJ)));
		btnRemoveColObj->SetImage(hIcons, 7, 7, 7 + 25, 7 + 25, 24, 24);

		sldMuscleUp = SetupFloatSlider(hWnd, IDC_SLD_MUSCLE_UP, IDC_EDT_MUSCLE_UP, 0.0f, 1.0f, 1.0f, 4);
		sldMuscleUp->SetResetValue(0.0f);

		sldColObjHardness = SetupFloatSlider(hWnd, IDC_SLD_HARDNESS, IDC_EDT_HARDNESS, 0.0f, 1.0f, 1.0f, 4);
		sldColObjHardness->SetResetValue(1.0f);

		// Setup the MoveUp button
		btnMoveUp = GetICustButton(GetDlgItem(hWnd, IDC_BTN_MOVEUP));
		btnMoveUp->SetType(CBT_PUSH);
		btnMoveUp->SetButtonDownNotify(TRUE);
		btnMoveUp->SetTooltip(TRUE, GetString(IDS_TT_MOVESPHUP));
		btnMoveUp->SetImage(hIcons, 14, 14, 14 + 25, 14 + 25, 24, 24);

		// Setup the MoveDown button
		btnMoveDown = GetICustButton(GetDlgItem(hWnd, IDC_BTN_MOVEDOWN));
		btnMoveDown->SetType(CBT_PUSH);
		btnMoveDown->SetButtonDownNotify(TRUE);
		btnMoveDown->SetTooltip(TRUE, GetString(IDS_TT_MOVESPHDN));
		btnMoveDown->SetImage(hIcons, 15, 15, 15 + 25, 15 + 25, 24, 24);

		Refresh();

		GetCOREInterface()->RegisterTimeChangeCallback(this);
	}

	void ReleaseControls()
	{
		GetCOREInterface()->UnRegisterTimeChangeCallback(this);
		GetCOREInterface()->ClearPickMode();
		this->hWnd = NULL;
		muscle = NULL;

		SAFE_RELEASE_EDIT(edtName);
		SAFE_RELEASE_COLORSWATCH(swColour);

		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);

		SAFE_RELEASE_SPIN(spnNumVSegs);
		SAFE_RELEASE_SPIN(spnNumUSegs);
		SAFE_RELEASE_SPIN(spnHandleSize);

		SAFE_RELEASE_BTN(btnAddColObj);
		SAFE_RELEASE_BTN(btnRemoveColObj);

		SAFE_RELEASE_SLIDER(sldMuscleUp);
		SAFE_RELEASE_SLIDER(sldColObjHardness);

		SAFE_RELEASE_BTN(btnMoveUp);
		SAFE_RELEASE_BTN(btnMoveDown);

	}

	BOOL CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (Muscle*)lParam);
			break;
		case WM_DESTROY:
			if (this->hWnd != hWnd) break;
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) { // Notification codes
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) { // Switch on ID
				case IDC_BTN_COPY:
					muscleCopy = muscle;
					Refresh(GetCOREInterface()->GetTime());
					break;
				case IDC_BTN_PASTE:
					if (muscleCopy) {
						muscle->PasteMuscle(muscleCopy);
						Refresh(GetCOREInterface()->GetTime());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_BTN_ADD_SPHERE:
					if (btnAddColObj->IsChecked())
					{
						GetCOREInterface()->ClearPickMode();
						ColObjFilter.SetMuscle(muscle);
						GetCOREInterface()->SetPickMode(this);
					}
					else GetCOREInterface()->ClearPickMode();
					break;
				case IDC_BTN_REMOVE_SPHERE:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0) && (collisionobj_index < muscle->GetNumColObjs())) {
						muscle->RemoveColObj(collisionobj_index);
						Refresh();
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_BTN_MOVEUP:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0) && (collisionobj_index < muscle->GetNumColObjs())) {
						HoldActions hold(IDS_TT_MOVESPHUP);
						muscle->MoveColObjUp(collisionobj_index);
						SendMessage(lbxColObjs, LB_SETCURSEL, collisionobj_index - 1, 0);
						Refresh();
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_BTN_MOVEDOWN:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					int nNumColObjs = muscle->GetNumColObjs();
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0) && (collisionobj_index < nNumColObjs)) {
						HoldActions hold(IDS_TT_MOVESPHDN);
						muscle->MoveColObjDown(collisionobj_index);
						SendMessage(lbxColObjs, LB_SETCURSEL, collisionobj_index + 1, 0);
						Refresh();
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				}
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_RDO_PATCH_MUSCLE:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_BONES_MUSCLE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetDeformerType(Muscle::DEFORMER_MESH);
					Refresh(GetCOREInterface()->GetTime());
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				case IDC_RDO_BONES_MUSCLE:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_PATCH_MUSCLE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					muscle->SetDeformerType(Muscle::DEFORMER_BONES);
					Refresh(GetCOREInterface()->GetTime());
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
				case IDC_RDO_VERT_NORMAL:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0)) {
						SendMessage(GetDlgItem(hWnd, IDC_RDO_OBJX), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
						muscle->SetObjXDistortion(collisionobj_index, FALSE);
						Refresh(GetCOREInterface()->GetTime());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_RDO_OBJX:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0)) {
						SendMessage(GetDlgItem(hWnd, IDC_RDO_VERT_NORMAL), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
						muscle->SetObjXDistortion(collisionobj_index, TRUE);
						Refresh(GetCOREInterface()->GetTime());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_CHK_SMOOTH:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0)) {
						BOOL on = (BOOL)SendMessage(GetDlgItem(hWnd, IDC_CHK_SMOOTH), BM_GETCHECK, (WPARAM)BST_UNCHECKED, 0);
						muscle->SetSmoothCollision(collisionobj_index, on);
						Refresh(GetCOREInterface()->GetTime());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_CHK_INVERT:
					collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
					if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0)) {
						BOOL on = (BOOL)SendMessage(GetDlgItem(hWnd, IDC_CHK_INVERT), BM_GETCHECK, (WPARAM)BST_UNCHECKED, 0);
						muscle->SetInvertCollision(collisionobj_index, on);
						Refresh(GetCOREInterface()->GetTime());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					break;
				case IDC_CHK_HDLVISIBLE:
					muscle->SetHandlesVisible(IS_CHECKED(hWnd, IDC_CHK_HDLVISIBLE));
					Refresh();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				case IDC_CHK_MIDDLE_HDLS:
					muscle->SetMiddleHandles(IS_CHECKED(hWnd, IDC_CHK_MIDDLE_HDLS));
					Refresh();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
				case IDC_CHK_REMOVE_SKEW:
				{
					HoldActions hold(IDS_HLD_MSCLSETTING);
					muscle->SetRemoveSkew(IS_CHECKED(hWnd, IDC_CHK_REMOVE_SKEW));
					muscle->UpdateMuscle();
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
				}
				break;
				}
				break;
			case LBN_SELCHANGE:
				Refresh(GetCOREInterface()->GetTime());
				break;
			}
			break;
		case WM_CUSTEDIT_ENTER:
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_NAME:
				TCHAR strbuf[LBL_LENGTH];	edtName->GetText(strbuf, LBL_LENGTH);
				muscle->SetName(strbuf);
				break;
			}
			break;
		case CC_SLIDER_BUTTONDOWN:
			theHold.Begin();
			break;
		case CC_SLIDER_BUTTONUP:
			if (HIWORD(wParam) == TRUE)
			{
				int iEndId = -1;
				switch (LOWORD(wParam)) {
				case IDC_SLD_MUSCLE_UP: iEndId = IDS_HLD_DISTORTION; break;
				case IDC_SLD_HARDNESS: iEndId = IDS_HLD_HARDNESS; break;
				default:
					DbgAssert(!_T("ERROR: Unhandled case here"));
					break;
				}
				theHold.Accept(GetString(iEndId));
			}
			else
				theHold.Cancel();
			break;
		case CC_SLIDER_CHANGE:
			collisionobj_index = (int)SendMessage(lbxColObjs, LB_GETCURSEL, 0, 0);
			if ((collisionobj_index != LB_ERR) && (collisionobj_index >= 0) && (collisionobj_index < muscle->GetNumColObjs())) {
				switch (LOWORD(wParam)) {
				case IDC_SLD_MUSCLE_UP:
					muscle->SetColObjDistortion(collisionobj_index, sldMuscleUp->GetFVal(), GetCOREInterface()->GetTime());
					break;
				case IDC_SLD_HARDNESS:
					muscle->SetColObjHardness(collisionobj_index, sldColObjHardness->GetFVal(), GetCOREInterface()->GetTime());
					break;
				}
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			}
			break;
		case CC_SPINNER_BUTTONDOWN:
			if (!theHold.Holding())
				theHold.Begin();
			break;
		case CC_SPINNER_CHANGE: {
			ISpinnerControl *ccSpinner = (ISpinnerControl*)lParam;
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_NUMVSEGS:		muscle->SetNumVSegs(ccSpinner->GetIVal());		break;
			case IDC_SPIN_NUMUSEGS:		muscle->SetNumUSegs(ccSpinner->GetIVal());		break;
			case IDC_SPIN_HANDLESIZE:	muscle->SetHandleSize(ccSpinner->GetFVal());	break;
			default:
				DbgAssert(!_T("FixThis"));
				break;
			}
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			break;
		}
		case CC_SPINNER_BUTTONUP:
			if (theHold.Holding()) {
				if (HIWORD(wParam) == FALSE) {
					theHold.Cancel();
				}
				else {
					switch (LOWORD(wParam)) { // Switch on ID
					case IDC_SPIN_NUMVSEGS:		theHold.Accept(GetString(IDS_HLD_NUMVSEGS));	break;
					case IDC_SPIN_NUMUSEGS:		theHold.Accept(GetString(IDS_HLD_NUMUSEGS));	break;
					case IDC_SPIN_HANDLESIZE:	theHold.Accept(GetString(IDS_HLD_HANDLESIZE)); break;
					}
				}
			}
			muscle->UpdateMuscle();
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			break;

		case CC_COLOR_SEL:
			// Reset the NotifyAfterAccept.  Our colour
			// swatch will only notify before or after, it
			// seems incapable of doing both.
			swColour->SetNotifyAfterAccept(FALSE);
			theHold.Begin();
			break;
		case CC_COLOR_CHANGE:
			if (LOWORD(wParam) == IDC_COLOURSWATCH_START)
			{
				Color clr = Color(swColour->GetColor());
				muscle->SetColour(&clr);
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			}
			break;
		case CC_COLOR_CLOSE:
		{
			Color clr = Color(swColour->GetColor());

			// Have we been cancelled or accepted?  If only there
			// were some way to tell!!
			// We work around this by simply canceling whatever has
			// been set
			theHold.Cancel();
			// Now we need to find the final value.  We -could- take
			// the current value and set it, except that if the value
			// was cancelled, the swatch hasn't been updated yet.
			// So we dodge this by changing the NotifyAfterAccept to true
			//.Now we get one more message to actually set the value.
			// If we didn't set this, we'd also get one more message
			// to cancel the color set, but notifying that the color has changed
			// back to the original.  Freaking brilliant.
			swColour->SetNotifyAfterAccept(TRUE);

			// The cancel above will cause the muscle to refresh the UI,
			// which will refresh the colour swatch with the undone colour.
			// Re-set the user selected colour back to the colour swatch,
			// so that the final notify will get the correct colour.
			swColour->SetColor(clr, FALSE);
		}
		break;
		default:
			return FALSE;
		}
		return TRUE;
	}
};

static MuscleDlgCallBack muscledlgcallBack;

static INT_PTR CALLBACK MuscleDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return muscledlgcallBack.DlgProc(hWnd, message, wParam, lParam);
};

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc catmuscle_interface(
	CATMUSCLE_INTERFACE_ID, _T("CATMuscle Functions"), 0, NULL, FP_MIXIN,

	Muscle::fnAddColObj, _T("AddCollisionObject"), 0, TYPE_VOID, 0, 1,
		_T("sphere"), 0, TYPE_INODE,

	Muscle::fnRemoveColObj, _T("RemoveCollisionObject"), 0, TYPE_VOID, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	Muscle::fnGetColObj, _T("GetCollisionObject"), 0, TYPE_INODE, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	Muscle::fnGetColObjDistortion, _T("GetCollisionObjectDistortion"), 0, TYPE_FLOAT, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	Muscle::fnSetColObjDistortion, _T("SetCollisionObjectDistortion"), 0, TYPE_VOID, 0, 2,
		_T("index"), 0, TYPE_INDEX,
		_T("value"), 0, TYPE_FLOAT,

	Muscle::fnGetColObjHardness, _T("GetCollisionObjectHardness"), 0, TYPE_FLOAT, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	Muscle::fnSetColObjHardness, _T("SetCollisionObjectHardness"), 0, TYPE_VOID, 0, 2,
		_T("index"), 0, TYPE_INDEX,
		_T("value"), 0, TYPE_FLOAT,

	Muscle::fnMoveColObjUp, _T("MoveCollisionObjectUp"), 0, TYPE_VOID, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	Muscle::fnMoveColObjDown, _T("MoveCollisionObjectDown"), 0, TYPE_VOID, 0, 1,
		_T("index"), 0, TYPE_INDEX,

	Muscle::fnPasteMuscle, _T("PasteMuscle"), 0, TYPE_VOID, 0, 1,
		_T("SourceMuscle"), 0, TYPE_REFTARG,

	properties,

	Muscle::fnGetDeformerType, Muscle::fnSetDeformerType, _T("DeformerType"), 0, TYPE_TSTR_BV,
	Muscle::fnGetName, Muscle::fnSetName, _T("MuscleName"), 0, TYPE_TSTR_BV,
	Muscle::fnGetColour, Muscle::fnSetColour, _T("Colour"), 0, TYPE_COLOR,
	Muscle::fnGetLMR, Muscle::fnSetLMR, _T("LMR"), 0, TYPE_INT,
	Muscle::fnGetNumVSegs, Muscle::fnSetNumVSegs, _T("NumVSegs"), 0, TYPE_INT,
	Muscle::fnGetNumUSegs, Muscle::fnSetNumUSegs, _T("NumUSegs"), 0, TYPE_INT,
	Muscle::fnGetHandleSize, Muscle::fnSetHandleSize, _T("HandleSize"), 0, TYPE_FLOAT,
	Muscle::fnGetHandleVis, Muscle::fnSetHandleVis, _T("HandlesVisible"), 0, TYPE_BOOL,
	Muscle::fnGetMiddleHandle, Muscle::fnSetMiddleHandle, _T("MiddleHandles"), 0, TYPE_BOOL,

	Muscle::fnGetHandles, FP_NO_FUNCTION, _T("Handles"), 0, TYPE_INODE_TAB_BV,
	Muscle::fnGetNumColObjs, FP_NO_FUNCTION, _T("NumCollisionObjects"), 0, TYPE_INT,

	p_end
);

FPInterfaceDesc* GetCATMuscleFPInterface() {
	return &catmuscle_interface;
}

//  Get Descriptor method for Mixin Interface
//  *****************************************
FPInterfaceDesc* Muscle::GetDescByID(Interface_ID id) {
	if (id == CATMUSCLE_INTERFACE_ID) return &catmuscle_interface;
	return &nullInterface;
}

static void ResetStartEndCallback(void *, NotifyInfo *)
{
	g_bIsResetting = TRUE;
	muscleCopy = NULL;
}

static void MuscleNodeCreateNotify(void *param, NotifyInfo *info)
{
	INode *node = (INode*)info->callParam;
	Muscle *muscle = (Muscle*)param;
	if (node->GetObjectRef()->FindBaseObject() == muscle) {
		UnRegisterNotification(MuscleNodeCreateNotify, muscle, NOTIFY_NODE_CREATED);
		muscle->Create(node);
	}
}
void Muscle::Create(INode* node)
{
	if (!IsCreated() && node->GetObjectRef()->FindBaseObject() == this) {
		ivalid = FOREVER;
		// force the creation of the corner handles and tangents
		CreateHandles();
		AssignSegTransTMController(node);

		UpdateColours();
		UpdateName();
		ivalid = NEVER;
	}
}
void Muscle::RegisterCallbacks()
{
	g_bIsResetting = FALSE;
	RegisterNotification(ResetStartEndCallback, this, NOTIFY_SYSTEM_PRE_RESET);
	RegisterNotification(ResetStartEndCallback, this, NOTIFY_FILE_PRE_OPEN);
	RegisterNotification(MuscleNodeCreateNotify, this, NOTIFY_NODE_CREATED);
}

void Muscle::UnRegisterCallbacks()
{
	UnRegisterNotification(ResetStartEndCallback, this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(ResetStartEndCallback, this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(MuscleNodeCreateNotify, this, NOTIFY_NODE_CREATED);
}

Muscle::Muscle()
{
	Init();
}

Muscle::~Muscle()
{
	UnRegisterCallbacks();
}

void Muscle::Init()
{
	dVersion = 0;
	handlesize = 10.0f;

	flags = 0;
	lmr = 0;
	mirroraxis = kXAxis;

	strName = _T("");
	handlesvalid = NEVER;

	tabHandles.SetCount(NUMNODEREFS);
	for (int i = 0; i < NUMNODEREFS; i++) {
		tabHandles[i] = (INode*)NULL;
		ws_hdl_pos[i].Set(0.0f, 0.0f, 0.0f);
	}
	tabColObjs.SetCount(0);
	tabDistortionByColObj.SetCount(0);
	tabColObjHardness.SetCount(0);
	tabColObjFlags.SetCount(0);

	tmMuscleUp = Matrix3(1);
	iMuscleUpValid = NEVER;
	handlesvalid = NEVER;

	flagsBegin = 0;
	ipRollout = NULL;

	RegisterCallbacks();

	tabStrips.SetCount(0);
	nNumUSegs = 0;
	nNumUSegs = 0;
	node = NULL;

	CatDotIni catini;
	nTempNumUSegs = catini.GetInt(_T("Defaults"), _T("NumUSegs"), _T("3"));
	nTempNumVSegs = catini.GetInt(_T("Defaults"), _T("NumVSegs"), _T("2"));

}

//////////////////////////////////////////////////////////////////////////////////////////
// Class Animateable

void Muscle::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	UNREFERENCED_PARAMETER(prev);

	this->flagsBegin = flags;
	this->ipRollout = ip;
	if (!TestFlag(MUSCLEFLAG_KEEP_ROLLOUTS) && (flagsBegin == 0 || flagsBegin&BEGIN_EDIT_MOTION || flagsBegin&BEGIN_EDIT_CREATE)) {
		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_MUSCLE_ROLLOUT), MuscleDlgProc, GetString(IDS_CL_MUSCLE), (LPARAM)this);
	}
}

void Muscle::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(next);

	if (!TestFlag(MUSCLEFLAG_KEEP_ROLLOUTS) && (flagsBegin == 0 || flagsBegin&BEGIN_EDIT_MOTION || flagsBegin&BEGIN_EDIT_CREATE)) {
		ip->DeleteRollupPage(muscledlgcallBack.GetHWnd());
	}
	this->flagsBegin = 0;
	this->ipRollout = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// From Object
// vertices ( a b c d ) are in counter clockwise order when viewd from
// outside the surface unless bias!=0 in which case they are clockwise
static void MakeQuad(int nverts, Face *f, int a, int b, int c, int d, int sg, int bias) {
	UNREFERENCED_PARAMETER(sg); UNREFERENCED_PARAMETER(nverts);
	int sm = 1;
	DbgAssert(a < nverts);
	DbgAssert(b < nverts);
	DbgAssert(c < nverts);
	DbgAssert(d < nverts);
	if (bias) {
		f[0].setVerts(b, a, c);
		f[0].setSmGroup(sm);
		f[0].setEdgeVisFlags(EDGE_VIS, EDGE_INVIS, EDGE_VIS);
		f[1].setVerts(d, c, a);
		f[1].setSmGroup(sm);
		f[1].setEdgeVisFlags(EDGE_VIS, EDGE_INVIS, EDGE_VIS);
	}
	else {
		f[0].setVerts(a, b, c);
		f[0].setSmGroup(sm);
		f[0].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_INVIS);
		f[1].setVerts(c, d, a);
		f[1].setSmGroup(sm);
		f[1].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_INVIS);
	}
}
#define MAKE_QUAD(na,nb,nc,nd,sm,b) {MakeQuad(nverts,&(mesh.faces[nf]),na, nb, nc, nd, sm, b);nf+=2;}

void Muscle::BuildMesh(TimeValue t)
{
	switch (GetDeformerType()) {
	case DEFORMER_BONES: {
		float dSize = 1.0f;

		// Initialise the mesh...
		mesh.setNumVerts(4);
		mesh.setNumFaces(2);
		mesh.InvalidateTopologyCache();

		// Vertices
		mesh.setVert(0, -dSize / 2.0f, dSize / 2.0f, 0.0f);
		mesh.setVert(1, dSize / 2.0f, dSize / 2.0f, 0.0f);
		mesh.setVert(2, -dSize / 2.0f, -dSize / 2.0f, 0.0f);
		mesh.setVert(3, dSize / 2.0f, -dSize / 2.0f, 0.0f);

		// Faces
		MakeFace(mesh.faces[0], 0, 2, 3, 1, 2, 0, 1, 1);
		MakeFace(mesh.faces[1], 3, 1, 0, 1, 2, 0, 1, 1);

		ivalid.SetInfinite();
		break;
	};
	case DEFORMER_MESH:
	{
		// Start the validity interval at forever and widdle it down.
		ivalid = FOREVER;

		Matrix3 tm;
		GetTrans(t, 0, 0, tm, ivalid);

		int ix, iy;
		Point3 p;
		int genUVs = 0;
		BOOL bias = 1;
		Matrix3 tmInv = Inverse(tm);

		int lsegs = GetNumVSegs();
		int wsegs = GetNumUSegs();
		if (lsegs < 1) lsegs = 1;
		if (wsegs < 1) lsegs = 1;
		int nverts = (lsegs + 1)*(wsegs + 1);
		int nfaces = 2 * (lsegs * wsegs);

		mesh.setNumVerts(nverts);
		mesh.setNumFaces(nfaces);

		//	UpdateHandles(t, ivalid);

			// do vertices.
		int nv = 0;
		for (iy = 0; iy <= lsegs; iy++) {
			for (ix = 0; ix <= wsegs; ix++) {
				//	p = GetSegPos(t, iy, ix, ivalid);
				p = tabPosCache[ix][iy];
				// we generate a coordinate system for this object, so we need to put the handle
				// positions into object space
				p = p * tmInv;
				mesh.setVert(nv++, p);
			}
		}
		// do faces (lsegs*wsegs);
		int kv = 0;
		int nf = 0;
		for (iy = 0; iy < lsegs; iy++) {
			kv = iy*(wsegs + 1);
			for (ix = 0; ix < wsegs; ix++) {
				MAKE_QUAD(kv, kv + 1, kv + wsegs + 2, kv + wsegs + 1, 2, bias);
				kv++;
			}
		}
		// UV coordinates
		if (genUVs) {
			mesh.setNumTVerts(nverts);
			mesh.setNumTVFaces(nfaces);

			float dw = 1.0f / float(wsegs);
			float dl = 1.0f / float(lsegs);

			float u, v;

			nv = 0;
			u = 0.0f;
			// X axis face
			for (iy = 0; iy < (lsegs + 1); iy++) {
				v = 0.0f;
				for (ix = 0; ix < (wsegs + 1); ix++) {
					mesh.setTVert(nv, u, v, 0.0f);
					nv++; v += dl;
				}
				u += dw;
			}
			DbgAssert(nv == nverts);

			for (nf = 0; nf < nfaces; nf++) {
				Face& f = mesh.faces[nf];
				DWORD* nv = f.getAllVerts();
				Point3 v[3];
				for (int ix = 0; ix < 3; ix++)
					v[ix] = mesh.getVert(nv[ix]);

				mesh.tvFace[nf].setTVerts(nv[0], nv[1], nv[2]);
				mesh.setFaceMtlIndex(nf, 1);
			}
		}
		else {
			mesh.setNumTVerts(0);
			mesh.setNumTVFaces(0);
		}
		break;
	}
	}

	mesh.InvalidateTopologyCache();
}

void Muscle::RefreshUI()
{
	if (ipRollout) muscledlgcallBack.Refresh();
}

BOOL Muscle::OKtoDisplay(TimeValue)
{
	if (IsCreated())	return TRUE;
	return FALSE;
}

// From Object
int Muscle::IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm)
{
	BuildMesh(t);
	return mesh.IntersectRay(ray, at, norm);
}

int Muscle::CanConvertToType(Class_ID obtype)
{
	return SimpleObject2::CanConvertToType(obtype);;
}
Object* Muscle::ConvertToType(TimeValue t, Class_ID obtype)
{
	return SimpleObject2::ConvertToType(t, obtype);
}

void Muscle::GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist)
{
	SimpleObject2::GetCollapseTypes(clist, nlist);
};

BaseInterface* Muscle::GetInterface(Interface_ID id)
{
	if (id == CATMUSCLE_INTERFACE_ID) return (IMuscle*)this;
	if (id == CATMUSCLE_SYSTEMMASTER_INTERFACE_ID) return (ICATMuscleSystemMaster*)this;

	BaseInterface* pInterface = FPMixinInterface::GetInterface(id);
	if (pInterface != NULL)
	{
		return pInterface;
	}
	return SimpleObject2::GetInterface(id);
}

//Class for interactive creation of the object using the mouse
class MuscleCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;		//First point in screen coordinates
	Muscle *ob;			//Pointer to the object
	Point3 p0, p1, p2;	//First point in world coordinates
	Matrix3 tm1, tm2;

	Point3 p3YAxis;		// Position/Rotation helpy things
	Point3 p3LastPos;	// Position/Rotation helpy things

	INode *node;

public:
	BOOL SupportAutoGrid() { return TRUE; }
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(Muscle *obj) { ob = obj; }

	void ArrangeHandles(TimeValue t);
};

void MuscleCreateCallBack::ArrangeHandles(TimeValue t)
{
	Matrix3 tm = ob->tabHandles[Muscle::HDL_A]->GetNodeTM(t);
	Point3 p3A = ob->tabHandles[Muscle::HDL_A]->GetNodeTM(t).GetTrans();
	Point3 p3B = ob->tabHandles[Muscle::HDL_B]->GetNodeTM(t).GetTrans();
	Point3 p3C = ob->tabHandles[Muscle::HDL_C]->GetNodeTM(t).GetTrans();
	Point3 p3D = ob->tabHandles[Muscle::HDL_D]->GetNodeTM(t).GetTrans();

	tm.SetTrans(p3A + ((p3B - p3A)*0.3f));	ob->tabHandles[Muscle::HDL_AB]->SetNodeTM(t, tm);
	tm.SetTrans(p3A + ((p3C - p3A)*0.3f));	ob->tabHandles[Muscle::HDL_AC]->SetNodeTM(t, tm);
	if (ob->tabHandles[Muscle::HDL_ACB]) { tm.SetTrans(p3A + (((p3B - p3A)*0.3f) + ((p3C - p3A)*0.3f)));	ob->tabHandles[Muscle::HDL_ACB]->SetNodeTM(t, tm); }
	//////////////////////////////////////////////////////////////////////////
	tm.SetTrans(p3B + ((p3A - p3B)*0.3f));	ob->tabHandles[Muscle::HDL_BA]->SetNodeTM(t, tm);
	tm.SetTrans(p3B + ((p3D - p3B)*0.3f));	ob->tabHandles[Muscle::HDL_BD]->SetNodeTM(t, tm);
	if (ob->tabHandles[Muscle::HDL_BAD]) { tm.SetTrans(p3B + (((p3A - p3B)*0.3f) + ((p3D - p3B)*0.3f)));	ob->tabHandles[Muscle::HDL_BAD]->SetNodeTM(t, tm); }
	//////////////////////////////////////////////////////////////////////////
	tm.SetTrans(p3C + ((p3A - p3C)*0.3f));	ob->tabHandles[Muscle::HDL_CA]->SetNodeTM(t, tm);
	tm.SetTrans(p3C + ((p3D - p3C)*0.3f));	ob->tabHandles[Muscle::HDL_CD]->SetNodeTM(t, tm);
	if (ob->tabHandles[Muscle::HDL_CAD]) { tm.SetTrans(p3C + (((p3A - p3C)*0.3f) + ((p3D - p3C)*0.3f)));	ob->tabHandles[Muscle::HDL_CAD]->SetNodeTM(t, tm); }
	//////////////////////////////////////////////////////////////////////////
	tm.SetTrans(p3D + ((p3C - p3D)*0.3f));	ob->tabHandles[Muscle::HDL_DC]->SetNodeTM(t, tm);
	tm.SetTrans(p3D + ((p3B - p3D)*0.3f));	ob->tabHandles[Muscle::HDL_DB]->SetNodeTM(t, tm);
	if (ob->tabHandles[Muscle::HDL_DBC]) { tm.SetTrans(p3D + (((p3C - p3D)*0.3f) + ((p3B - p3D)*0.3f)));	ob->tabHandles[Muscle::HDL_DBC]->SetNodeTM(t, tm); }
}

int MuscleCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	UNREFERENCED_PARAMETER(flags);
	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();
	if (msg == MOUSE_FREEMOVE) {
		vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
	}
	else if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch (point)
		{
		case 0: // only happens with MOUSE_POINT msg
		{
			ob->suspendSnap = TRUE;
			vpt->TrackImplicitGrid(m, &tm1, SNAP_IN_3D);

			ob->tabHandles[Muscle::HDL_A]->SetNodeTM(t, mat);
			ob->tabHandles[Muscle::HDL_B]->SetNodeTM(t, mat);
			ob->tabHandles[Muscle::HDL_C]->SetNodeTM(t, mat);
			ob->tabHandles[Muscle::HDL_D]->SetNodeTM(t, mat);
			ArrangeHandles(t);
			break;
		}
		case 1:
		{
			vpt->TrackImplicitGrid(m, &tm2, SNAP_IN_3D);

			Matrix3 tm = tm2 * Inverse(tm1);
			DebugPrint(_T("x:%f	y:%f\n"), tm.GetTrans()[0], tm.GetTrans()[1]);

			Matrix3 tmTL = tm;
			Matrix3 tmTR = tm;
			Matrix3 tmBL = tm;
			Matrix3 tmBR = tm;

			Point3 bl = Point3(min(tm.GetTrans()[0], 0.0f), min(tm.GetTrans()[1], 0.0f), 0.0f);
			Point3 br = Point3(max(tm.GetTrans()[0], 0.0f), min(tm.GetTrans()[1], 0.0f), 0.0f);
			Point3 tl = Point3(min(tm.GetTrans()[0], 0.0f), max(tm.GetTrans()[1], 0.0f), 0.0f);
			Point3 tr = Point3(max(tm.GetTrans()[0], 0.0f), max(tm.GetTrans()[1], 0.0f), 0.0f);

			tmBL.SetTrans(bl);
			tmBL = tmBL * tm1;
			tmBR.SetTrans(br);
			tmBR = tmBR * tm1;
			tmTL.SetTrans(tl);
			tmTL = tmTL * tm1;
			tmTR.SetTrans(tr);
			tmTR = tmTR * tm1;

			ob->tabHandles[Muscle::HDL_A]->SetNodeTM(t, tmBL);
			ob->tabHandles[Muscle::HDL_B]->SetNodeTM(t, tmBR);
			ob->tabHandles[Muscle::HDL_C]->SetNodeTM(t, tmTL);
			ob->tabHandles[Muscle::HDL_D]->SetNodeTM(t, tmTR);
			ArrangeHandles(t);

			float hdl_size = Length(tm.GetTrans()) / 10.0f;
			ob->SetHandleSize(hdl_size);

			if (msg == MOUSE_POINT) {
				return CREATE_STOP;
			}

			break;
		}
		case 2:
		{
			if (msg == MOUSE_POINT)
				return CREATE_STOP;
		}

		}
	}
	else {
		if (msg == MOUSE_ABORT) return CREATE_ABORT;
	}

	return CREATE_CONTINUE;
}

static MuscleCreateCallBack MuscleCreateCB;

//From BaseObject
CreateMouseCallBack* Muscle::GetCreateMouseCallBack()
{
	MuscleCreateCB.SetObj(this);
	return(&MuscleCreateCB);
}

//////////////////////////////////////////////////////////////////////////////////////////
// From ReferenceTarget

RefTargetHandle Muscle::GetReference(int i)
{
	int numhandles = NUMNODEREFS;
	if (dVersion < CATMUSCLE_VERSION_0860)
		numhandles = 12;

	if (i < numhandles)
		return tabHandles[i];
	i -= numhandles;
	if (i < (tabColObjs.Count() * 3)) {
		int index = (int)floor(i / 3.0);
		if (i % 3 == 0)	return tabColObjs[index];
		if (i % 3 == 1)	return tabDistortionByColObj[index];
		if (i % 3 == 2)	return tabColObjHardness[index];
	}
	return NULL;
}
void Muscle::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	int numhandles = NUMNODEREFS;
	if (dVersion < CATMUSCLE_VERSION_0860)
		numhandles = 12;
	if (i < numhandles)
	{
		DbgAssert(!rtarg || rtarg->SuperClassID() == BASENODE_CLASS_ID);
		tabHandles[i] = (INode*)rtarg;
	}
	else if (i < (numhandles + (tabColObjs.Count() * 3)))
	{
		i -= numhandles;
		int index = (int)floor(i / 3.0);
		if (i % 3 == 0) {
			DbgAssert(!rtarg || rtarg->SuperClassID() == BASENODE_CLASS_ID);
			tabColObjs[index] = (INode*)rtarg;
		}
		if (i % 3 == 1)	tabDistortionByColObj[index] = (Control*)rtarg;
		if (i % 3 == 2)	tabColObjHardness[index] = (Control*)rtarg;
		RefreshUI();
	}
}

Animatable* Muscle::SubAnim(int i)
{
	if (i < NUMNODEREFS) return GetReference(i);
	i -= NUMNODEREFS;

	if (i < (tabColObjs.Count() * 2)) {
		int index = (int)floor(i / 2.0);
		if (i % 2 == 0)	return tabDistortionByColObj[index];
		if (i % 2 == 1)	return tabColObjHardness[index];
	}
	return NULL;
}
TSTR Muscle::SubAnimName(int i)
{
	if (i < NUMNODEREFS) {
		if (tabHandles[i])	return tabHandles[i]->GetName();
		else				return _T("");
	}
	i -= NUMNODEREFS;

	int index = (int)floor(i / 2.0);
	TSTR spherename = tabColObjs[index]->GetName();
	if (i < (tabColObjs.Count() * 2)) {
		if (i % 2 == 0)	return (spherename + GetString(IDS_HLD_DISTORTION));
		if (i % 2 == 1)	return (spherename + GetString(IDS_HLD_HARDNESS));
	}
	return _T("");
}
// this is all very confusing because ColObjs come in groups of 3 references
// 1 for the sphere and one for the hardness and one for the Distortion,
// but we only display the subanims in groups of 2, because the sphre node is
// not a subanim
BOOL Muscle::AssignController(Animatable *control, int subAnim) {
	// given the subindex, what sphere's controlers are we assigning
	int index = (int)floor(subAnim / 2.0);
	if (subAnim < (tabColObjs.Count() * 2)) {
		if (subAnim % 2 == 0)	ReplaceReference(NUMNODEREFS + index + 1, (RefTargetHandle)control, TRUE);
		if (subAnim % 2 == 1)	ReplaceReference(NUMNODEREFS + index + 2, (RefTargetHandle)control, TRUE);
		return TRUE;
	}
	return FALSE;
};

void Muscle::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	if (muscleCopy == this) muscleCopy = NULL;
	int u, v, i;
	switch (ctxt) {
		// if we are merging, force all the collision spheres to be merged also
	case kSNCFileMerge:
		for (i = 0; i < tabColObjs.Count(); i++)
			nodes.Append(1, &tabColObjs[i]);
		// fall through to next case
	case kSNCDelete:
		if (ctxt == kSNCDelete) {
			SetFlag(MUSCLEFLAG_CREATED, FALSE);
		}
		// fall through to next case
	case kSNCFileSave:
	case kSNCClone:
		if (TestFlag(MUSCLEFLAG_HANDLES_VISIBLE)) {
			for (i = 0; i < NUMNODEREFS; i++)
				if (tabHandles[i]) nodes.Append(1, &tabHandles[i]);
		}

		switch (deformer_type) {
		case DEFORMER_BONES:
			if (!TestFlag(MUSCLEFLAG_CLONING)) {
				for (v = 0; v < tabStrips.Count(); v++) {
					MuscleBonesStrip *tabStrip = tabStrips[v];
					for (u = 0; u < tabStrip->tabSegs.Count(); u++)
						nodes.Append(1, &tabStrip->tabSegs[u]);
				}
			}
			break;
		case DEFORMER_MESH:
			nodes.Append(1, &node);
			break;
		}
		break;
	}
}
// The master controller of a system plug-in should implement this
// method to give MAX a list of nodes that are part of the system.
// The master controller should fill in the given table with the
// INode pointers of the nodes that are part of the system. This
// will ensure that operations like cloning and deleting affect
// the whole system.  MAX will use GetInterface() in the
// tmController of each selected node to retrieve the master
// controller and then call GetSystemNodes() on the master
// controller to get the list of nodes.

// this callback tells us that a node has been cloned.
// now we need to find out if it was out IKTarget node.
// We can assume that because the node has been cloned and
// this notification has been registered, then the limb pointer
// is patched.
typedef struct { INodeTab* origNodes; INodeTab* clonedNodes; CloneType cloneType; } ClonedNodesInfo;
struct ClonedMuscleParam
{
	Muscle* pOrig;
	Muscle* pClone;
};

void MuscleCloneNotify(void *param, NotifyInfo *info)
{
	ClonedMuscleParam *muscles = static_cast<ClonedMuscleParam*>(param);
	ClonedNodesInfo* pClonedNodes = static_cast<ClonedNodesInfo*>(info->callParam);
	DbgAssert(muscles->pOrig != NULL && muscles->pClone != NULL);

	// Iterate all the muscles that have been cloned, and ensure the pointers
	// are transferred properly from the original muscle to the new one.
	// This is mainly to catch the collision objects, as this is the only
	// place we are guaranteed to actually have a pointer to the new
	// collision object (if it was, indeed, cloned!).
	// Confused... like a hungry baby in a topless bar...
	if (muscles->pOrig != NULL && muscles->pClone != NULL)
	{
		DbgAssert(muscles->pOrig->NumRefs() == muscles->pClone->NumRefs());
		for (int i = 0; i < muscles->pOrig->NumRefs(); i++)
		{
			INode* pOrigNode = dynamic_cast<INode*>(muscles->pOrig->GetReference(i));
			if (pOrigNode == NULL)
				continue;

			int idx = pClonedNodes->origNodes->IndexOf(pOrigNode);
			if (idx >= 0)
			{
				muscles->pClone->ReplaceReference(i, (*pClonedNodes->clonedNodes)[idx]);
			}
		}
	}

	// This callbacks job is done.  Unregister yourself, and delete the muscles.
	UnRegisterNotification(MuscleCloneNotify, muscles, NOTIFY_POST_NODES_CLONED);
	// Delete the muscles struct.  It was new'ed in CloneMuscle below
	delete muscles;
}

void Muscle::AssignSegTransTMController(INode *node) {

	if (IsCreated()) return;

	Interface *ip = GetCOREInterface();
	if (!node || node->GetObjectRef() != this) {
		node = ip->CreateObjectNode(this);
	}
	else {
		clrColour = Color(node->GetWireColor());
		strName = node->GetName();
	}

	SegTrans* seg = (SegTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, SEGTRANS_CLASS_ID);
	// instead of calling BuildSeg(which creates another INode and we already have one)
	// just put the correct values into the SegTrans
	seg->mWeakRefToObject.SetRef(this);
	//	seg->node = node;
	seg->nStrandID = 0;
	seg->nSegID = 0;
	node->SetTMController(seg);

	switch (deformer_type) {
	case DEFORMER_BONES:
		nNumUSegs = 1;
		// here we plug ourselves into the first slot in the muscle
		tabStrips.SetCount(1);
		tabStrips[0] = new MuscleBonesStrip();
		tabStrips[0]->tabSegs.SetCount(1);
		tabStrips[0]->tabSegs[0] = node;
		break;
	case DEFORMER_MESH:
		this->node = node;
		break;
	}

	SetFlag(MUSCLEFLAG_CREATED); // LAM - changed from direct set of flag so that RestoreObj is created
	CatDotIni catini;
	catini.SetInt(_T("Defaults"), _T("NumUSegs"), nTempNumUSegs);
	catini.SetInt(_T("Defaults"), _T("NumVSegs"), nTempNumVSegs);

	// Use the temp values to build our muscle
	SetNumVSegs(nTempNumVSegs);
	SetNumUSegs(nTempNumUSegs);
}

// From ReferenceTarget
void Muscle::CloneMuscle(Muscle* newob, RemapDir& remap)
{
	// References: this has 4 tabs of references, two of which are Nodes which need to be cloned
	// The referenced nodesTabs are: tabHandles and tabColObjs

	newob->clrColour = clrColour;
	newob->strName = strName;
	newob->lmr = lmr;
	newob->handlesvalid = NEVER;
	newob->handlesize = handlesize;
	newob->iMuscleUpValid = NEVER;

	newob->dVersion = dVersion;
	newob->flags = flags;

	newob->tabColObjs.SetCount(tabColObjs.Count());
	newob->tabDistortionByColObj.SetCount(tabColObjs.Count());
	newob->tabColObjHardness.SetCount(tabColObjs.Count());
	newob->tabColObjFlags.SetCount(tabColObjs.Count());
	for (int i = 0; i < tabColObjs.Count(); i++) {
		newob->tabColObjs[i] = NULL;
		newob->tabDistortionByColObj[i] = NULL;
		newob->tabColObjHardness[i] = NULL;

		// Do not clone the collision objects.  Unless we are supposed to.
		INode* pColObject = tabColObjs[i];
		// WORK_3 is code for "Clone Node".
		// If the node is not scheduled to be cloned,
		// then we just take the pointer to the original.
		// If the node is being cloned, it will be hooked
		// up in the MuscleCloneNotify callback
		if (!pColObject->TestAFlag(A_WORK3))
		{
			newob->ReplaceReference(NUMNODEREFS + (i * 3), pColObject);
		}

		// Always clone the hardness etc.
		newob->ReplaceReference(NUMNODEREFS + (i * 3) + 1, remap.CloneRef(GetReference(NUMNODEREFS + (i * 3) + 1)), TRUE);
		newob->ReplaceReference(NUMNODEREFS + (i * 3) + 2, remap.CloneRef(GetReference(NUMNODEREFS + (i * 3) + 2)), TRUE);
		newob->tabColObjFlags[i] = tabColObjFlags[i];
	}

	ClonedMuscleParam* cloneMuscleBits = new ClonedMuscleParam;
	cloneMuscleBits->pClone = newob;
	cloneMuscleBits->pOrig = this;
	RegisterNotification(MuscleCloneNotify, cloneMuscleBits, NOTIFY_POST_NODES_CLONED);
}

// From ReferenceTarget
RefTargetHandle Muscle::Clone(RemapDir& remap)
{
	Muscle* newob = (Muscle*)CreateInstance(SuperClassID(), ClassID());

	CloneMuscle(newob, remap);

	switch (deformer_type) {
	case DEFORMER_BONES:
		newob->tabStrips.SetCount(tabStrips.Count());
		for (int v = 0; v < tabStrips.Count(); v++) {
			newob->tabStrips[v] = new MuscleBonesStrip();
			newob->tabStrips[v]->tabSegs.SetCount(GetNumUSegs());
			for (int j = 0; j < GetNumUSegs(); j++) {
				newob->tabStrips[v]->tabSegs[j] = NULL;
				remap.PatchPointer((RefTargetHandle*)&newob->tabStrips[v]->tabSegs[j], (RefTargetHandle)tabStrips[v]->tabSegs[j]);
			}
		}
		newob->nNumUSegs = nNumUSegs;
		break;
	case DEFORMER_MESH:
		newob->nNumUSegs = nNumUSegs;
		newob->nNumVSegs = nNumVSegs;
		newob->ivalid = NEVER;

		remap.PatchPointer((RefTargetHandle*)&newob->node, (RefTargetHandle)node);
		break;
	}

	BaseClone(this, newob, remap);

	return newob;
}

int  Muscle::GetNumUSegs() {
	if (!IsCreated()) return nTempNumUSegs;
	return nNumUSegs;
};

int  Muscle::GetNumVSegs() {
	if (!IsCreated()) return nTempNumVSegs;
	switch (deformer_type) {
	case DEFORMER_BONES:	return tabStrips.Count();
	case DEFORMER_MESH:	return nNumVSegs;
	}
	return 0;
};

class SetNumUSegsRestore : public RestoreObj {
public:
	Muscle *muscle;
	int val_undo, val_redo;
	bool refresh_ui;
	bool capture_redo_value;
	SetNumUSegsRestore(Muscle *c, int n, bool refresh_ui) : val_redo(0) {
		muscle = c;
		val_undo = n;
		this->refresh_ui = refresh_ui;
		muscle->SetAFlag(A_HELD);
		capture_redo_value = true;
	}
	void Restore(int isUndo) {
		if (capture_redo_value) {
			val_redo = muscle->GetNumUSegs();
			capture_redo_value = false;
		}
		muscle->SetNumUSegs(val_undo);
		if (isUndo) {
			muscle->UpdateMuscle();
			muscle->RefreshUI();
		}
	}
	void Redo() {
		muscle->SetNumUSegs(val_redo);
		muscle->UpdateMuscle();
		if (refresh_ui) {
			muscle->RefreshUI();
		}
	}
	int Size() { return sizeof(SetNumUSegsRestore); }
	void EndHold() { muscle->ClearAFlag(A_HELD); }
};

class INodePointerRestore : public RestoreObj {
public:
	Muscle	*muscle;
	INode	*node;
	int u, v;
	INodePointerRestore(Muscle *m, int u, int v) {
		muscle = m;
		this->u = u;
		this->v = v;
		node = muscle->tabStrips[v]->tabSegs[u];
	}
	void Restore(int isUndo) {
		if (isUndo) {
			muscle->tabStrips[v]->tabSegs[u] = node;
		}
	}
	void Redo() {}
	int Size() { return sizeof(INodePointerRestore); }
	void EndHold() {}
};

void Muscle::SetNumUSegs(int n)
{
	if (!IsCreated()) {
		// do something so that when we really do create it make all the segs
		nTempNumUSegs = n;
		return;
	}

	// register an Undo
	bool newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = true; }
	int oldNumUSegs = GetNumUSegs();

	switch (deformer_type) {
	case DEFORMER_BONES:
		handlesvalid = NEVER;

		SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

		for (int v = 0; v < tabStrips.Count(); v++) {
			MuscleBonesStrip* strand = tabStrips[v];

			// remove excessive segs
			if (n < strand->tabSegs.Count()) {
				if (!theHold.RestoreOrRedoing()) {
					for (int u = strand->tabSegs.Count() - 1; u >= n; u--) {
						if (theHold.Holding())	theHold.Put(new INodePointerRestore(this, u, v));
						if (DeSelectAndDelete(strand->tabSegs[u])) {
							GetCOREInterface()->SelectNode(tabStrips[0]->tabSegs[0]);
						}
					}
				}
				strand->tabSegs.SetCount(n);
			}
			// add new segs
			else if (n > strand->tabSegs.Count()) {
				int nOldNumUSegs = strand->tabSegs.Count();
				strand->tabSegs.SetCount(n);

				for (int j = nOldNumUSegs; j < n; j++) {
					if (!theHold.RestoreOrRedoing()) {
						SegTrans* seg = (SegTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, SEGTRANS_CLASS_ID);
						if (j > 0)	strand->tabSegs[j] = seg->BuildNode(this, v, j, strand->tabSegs[j - 1]);
						else	strand->tabSegs[j] = seg->BuildNode(this, v, j, NULL);
					}
					else {
						strand->tabSegs[j] = NULL;
					}
				}
			}
		}
		nNumUSegs = n;
		UpdateColours();
		ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);
		break;
	case DEFORMER_MESH:
		nNumUSegs = n;
		break;
	}
	UpdateMuscle();
	if (theHold.Holding())		theHold.Put(new SetNumUSegsRestore(this, oldNumUSegs, newundo));
	if (newundo && theHold.Holding()) {
		theHold.Accept(GetString(IDS_HLD_NUMUSEGS));
	}
}

class SetNumVSegsRestore : public RestoreObj {
public:
	Muscle *muscle;
	int val_undo, val_redo;
	bool refresh_ui;
	bool capture_redo_value;
	SetNumVSegsRestore(Muscle *c, int n, bool refresh_ui) : val_redo(0) {
		muscle = c;
		val_undo = n;
		this->refresh_ui = refresh_ui;
		muscle->SetAFlag(A_HELD);
		capture_redo_value = true;
	}
	void Restore(int isUndo) {
		if (capture_redo_value) {
			val_redo = muscle->GetNumVSegs();
			capture_redo_value = false;
		}
		muscle->SetNumVSegs(val_undo);
		if (isUndo) {
			muscle->UpdateMuscle();
			muscle->RefreshUI();
		}
	}
	void Redo() {
		muscle->SetNumVSegs(val_redo);
		muscle->UpdateMuscle();
		muscle->RefreshUI();
	}
	int Size() { return sizeof(SetNumVSegsRestore); }
	void EndHold() { muscle->ClearAFlag(A_HELD); }
};

void Muscle::SetNumVSegs(int n)
{
	if (!IsCreated()) {
		// do something so that when we really do create it make all the segs
		nTempNumVSegs = n;
		return;
	}
	// register an Undo
	bool newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = true; }
	int oldNumVSegs = GetNumVSegs();

	switch (deformer_type) {
	case DEFORMER_BONES: {
		SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

		if (n < tabStrips.Count()) {
			// remove the segs from the strands we about to delete
			// if we remove all the segs from any strand we austomaticly delete the handle
			if (!theHold.RestoreOrRedoing()) {
				for (int v = tabStrips.Count() - 1; v >= n; v--) {
					MuscleBonesStrip* strand = tabStrips[v];
					for (int u = strand->tabSegs.Count() - 1; u >= 0; u--) {
						if (theHold.Holding())	theHold.Put(new INodePointerRestore(this, u, v));
						if (DeSelectAndDelete(strand->tabSegs[u])) {
							GetCOREInterface()->SelectNode(tabStrips[0]->tabSegs[0]);
						}
					}
				}
			}
			tabStrips.SetCount(n);
		}
		else {
			int oldcount = tabStrips.Count();
			tabStrips.SetCount(n);
			for (int v = oldcount; v < n; v++)
				tabStrips[v] = new MuscleBonesStrip();
			SetNumUSegs(nNumUSegs);
			if (newundo && theHold.Holding())	UpdateName();
		}
		ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);
		break;
	}
	case DEFORMER_MESH:
		nNumVSegs = n;
		break;
	}
	UpdateMuscle();
	if (theHold.Holding())		theHold.Put(new SetNumVSegsRestore(this, oldNumVSegs, newundo));
	if (newundo && theHold.Holding()) {
		theHold.Accept(GetString(IDS_HLD_NUMVSEGS));
	}
}

class MuscleConvertRestore : public RestoreObj {
public:
	Muscle *muscle;
	Muscle::DEFORMER_TYPE muscle_type_undo, muscle_type_redo;
	bool capture_redo_value;
	MuscleConvertRestore(Muscle *c) : muscle_type_redo(ICATMuscleClass::DEFORMER_MESH) {
		muscle = c;
		muscle_type_undo = muscle->deformer_type;
		capture_redo_value = true;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			if (capture_redo_value) {
				muscle_type_redo = muscle->deformer_type;
				capture_redo_value = false;
			}
			muscle->deformer_type = muscle_type_undo;
		}
	}
	void Redo() {
		muscle->deformer_type = muscle_type_redo;
	}
	int Size() { return sizeof(MuscleConvertRestore); }
	void EndHold() {}
};

void Muscle::SetDeformerType(DEFORMER_TYPE type)
{
	if (deformer_type == type)	return;

	CatDotIni *catini = new CatDotIni();
	catini->SetInt(KEY_MUSCLE_TYPE, type);
	delete catini;

	if (!IsCreated()) {
		deformer_type = type;
		return;
	}
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }

	SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

	switch (deformer_type) {
	case DEFORMER_BONES: {
		nTempNumUSegs = GetNumUSegs();
		nTempNumVSegs = GetNumVSegs();
		SetNumUSegs(1);
		SetNumVSegs(1);
		UpdateMuscle();
		if (theHold.Holding())	theHold.Put(new MuscleConvertRestore(this));

		deformer_type = type;
		node = tabStrips[0]->tabSegs[0];
		nNumVSegs = 1;

		SetNumUSegs(nTempNumUSegs);
		SetNumVSegs(nTempNumVSegs);
		break;
	}
	case DEFORMER_MESH: {
		nTempNumUSegs = GetNumUSegs();
		nTempNumVSegs = GetNumVSegs();
		SetNumUSegs(1);
		SetNumVSegs(1);
		UpdateMuscle();
		if (theHold.Holding())	theHold.Put(new MuscleConvertRestore(this));
		deformer_type = type;

		// here we plug ourselves into the first slot in the muscle
		tabStrips.SetCount(1);
		tabStrips[0] = new MuscleBonesStrip();
		tabStrips[0]->tabSegs.SetCount(1);
		tabStrips[0]->tabSegs[0] = node;

		SetNumUSegs(nTempNumUSegs);
		SetNumVSegs(nTempNumVSegs);
		break;
	}
	}
	UpdateColours();
	UpdateName();

	ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);
	if (newundo && theHold.Holding()) {
		theHold.Accept(TSTR(GetString(IDS_HLD_CONVERSION)) + IGetDeformerType() + GetString(IDS_HLD_MSCL));
		UpdateMuscle();
	}
}

void Muscle::UpdateMuscle()
{
	if (!IsCreated() || g_bIsResetting) return;
	handlesvalid = NEVER;

	switch (deformer_type) {
	case DEFORMER_BONES:
		for (int v = 0; v < tabStrips.Count(); v++) {
			for (int j = 0; j < tabStrips[v]->tabSegs.Count(); j++) {
				if (tabStrips[v]->tabSegs[j])
					tabStrips[v]->tabSegs[j]->InvalidateTM();
			}
		}
		break;
	case DEFORMER_MESH:
		ivalid = NEVER;
		if (node) {
			node->InvalidateWS();
			node->InvalidateTM();
		}
		break;
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

RefResult Muscle::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE: {
		if (!IsCreated()) return REF_SUCCEED;

		for (int j = 0; j < GetNumColObjs(); j++)
		{
			INode* pColObj = GetColObj(j);
			if (hTarg == pColObj)
			{
				// collisions with spheres
				if (pColObj->GetObjectRef()->ClassID() == Class_ID(SPHERE_CLASS_ID, 0))
				{
					//////////////////////////////////////////////////////////////////////
					// user Properties.
					// these will be used by the runtime libraries
					TSTR key = _T("CATMuscleProp_CollisionSphereRadius"); // globalize?  I don't think so - looks like a prop string?
					IParamBlock2* sphereParams = ((GenSphere*)pColObj->GetObjectRef())->GetParamBlockByID(SPHERE_PARAMBLOCK_ID);
					DbgAssert(sphereParams);
					if (sphereParams == nullptr)
						break;
					float radius;
					sphereParams->GetValue(SPHERE_RADIUS, 0, radius, FOREVER);
					pColObj->SetUserPropFloat(key, radius);
				}
				break;
			}
		}

		// we only update the muscle if we are in interactive manipulation mode
		handlesvalid = NEVER;
		iMuscleUpValid = NEVER;
		RefreshUI();
		UpdateMuscle();
		break;
	}
	case REFMSG_TARGET_DELETED:
		for (int j = 0; j < GetNumColObjs(); j++)
			if (hTarg == GetColObj(j)) {
				RemoveColObj(j);
				// Update Rollout
				break;
			}
		for (int i = 0; i < NUMNODEREFS; i++)
		{
			if (hTarg == tabHandles[i])
				tabHandles[i] = NULL;
		}
		// if any of our handles gets deleted clean up everything
		if (muscleCopy == this)
			muscleCopy = NULL;
		break;
	}

	return REF_SUCCEED;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CreateHandleNode
// This function creates a new Hadnle trans and initialises it.

void Muscle::CreateHandleNode(int refid, INode* nodeparent/*=NULL*/)
{
	// create the handle controller
	HdlTrans *hdltrans = (HdlTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HDLTRANS_CLASS_ID);

	INode *node = hdltrans->BuildNode(this, nodeparent, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE);

	if (IsCreated()) {
		Matrix3 tm(1);
		TimeValue t = GetCOREInterface()->GetTime();
		if (tabHandles[refid]) {
			tm = tabHandles[refid]->GetNodeTM(t);
			if (!nodeparent) {
				tabHandles[refid]->AttachChild(node, FALSE);
			}
		}
		if (refid < ls_hdl_tm.Count())
			tm = ls_hdl_tm[refid] * tm;
		node->SetNodeTM(t, tm);
	}
	ReplaceReference(refid, node);
}

void Muscle::CreateMiddleHandles()
{
	if (TestFlag(MUSCLEFLAG_MIDDLE_HANDLES)) return;

	if (!IsCreated() || !TestFlag(MUSCLEFLAG_HANDLES_VISIBLE)) {
		SetFlag(MUSCLEFLAG_MIDDLE_HANDLES);
		UpdateMuscle();
		return;
	}

	// register an Undo
	HoldActions hold(IDS_HLD_MIDHANDLECREATE);
	SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

	CreateHandleNode(HDL_ACB, tabHandles[HDL_A]);
	CreateHandleNode(HDL_BAD, tabHandles[HDL_B]);
	CreateHandleNode(HDL_CAD, tabHandles[HDL_C]);
	CreateHandleNode(HDL_DBC, tabHandles[HDL_D]);

	Matrix3 tm;
	TimeValue t = GetCOREInterface()->GetTime();
	tm = tabHandles[HDL_A]->GetNodeTM(t);	tm.SetTrans(ws_hdl_pos[HDL_ACB]);	tabHandles[HDL_ACB]->SetNodeTM(t, tm);
	tm = tabHandles[HDL_B]->GetNodeTM(t);	tm.SetTrans(ws_hdl_pos[HDL_BAD]);	tabHandles[HDL_BAD]->SetNodeTM(t, tm);
	tm = tabHandles[HDL_C]->GetNodeTM(t);	tm.SetTrans(ws_hdl_pos[HDL_CAD]);	tabHandles[HDL_CAD]->SetNodeTM(t, tm);
	tm = tabHandles[HDL_D]->GetNodeTM(t);	tm.SetTrans(ws_hdl_pos[HDL_DBC]);	tabHandles[HDL_DBC]->SetNodeTM(t, tm);

	SetFlag(MUSCLEFLAG_MIDDLE_HANDLES);
	ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

	if (IsCreated()) {
		UpdateName();
		UpdateColours();
	}
}

void Muscle::RemoveMiddleHandles()
{
	if (!TestFlag(MUSCLEFLAG_MIDDLE_HANDLES)) return;
	if (!TestFlag(MUSCLEFLAG_HANDLES_VISIBLE)) {
		ClearFlag(MUSCLEFLAG_MIDDLE_HANDLES);
		UpdateMuscle();
		return;
	}
	// register an Undo
	HoldActions hold(IDS_HLD_MIDHANDLEREM);

	if (tabHandles[HDL_ACB]) DeSelectAndDelete(tabHandles[HDL_ACB]);
	if (tabHandles[HDL_BAD]) DeSelectAndDelete(tabHandles[HDL_BAD]);
	if (tabHandles[HDL_CAD]) DeSelectAndDelete(tabHandles[HDL_CAD]);
	if (tabHandles[HDL_DBC]) DeSelectAndDelete(tabHandles[HDL_DBC]);

	ClearFlag(MUSCLEFLAG_MIDDLE_HANDLES);
}

void Muscle::CreateHandles()
{
	if (TestFlag(MUSCLEFLAG_HANDLES_VISIBLE))
		return;

	// register an Undo
	HoldActions hold(IDS_HLD_HANDLECREATE);
	SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

	CreateHandleNode(HDL_A);
	CreateHandleNode(HDL_AB, tabHandles[HDL_A]);
	CreateHandleNode(HDL_AC, tabHandles[HDL_A]);

	CreateHandleNode(HDL_B);
	CreateHandleNode(HDL_BA, tabHandles[HDL_B]);
	CreateHandleNode(HDL_BD, tabHandles[HDL_B]);

	CreateHandleNode(HDL_C);
	CreateHandleNode(HDL_CA, tabHandles[HDL_C]);
	CreateHandleNode(HDL_CD, tabHandles[HDL_C]);

	CreateHandleNode(HDL_D);
	CreateHandleNode(HDL_DB, tabHandles[HDL_D]);
	CreateHandleNode(HDL_DC, tabHandles[HDL_D]);

	if (TestFlag(MUSCLEFLAG_MIDDLE_HANDLES)) {
		CreateHandleNode(HDL_ACB, tabHandles[HDL_A]);
		CreateHandleNode(HDL_BAD, tabHandles[HDL_B]);
		CreateHandleNode(HDL_CAD, tabHandles[HDL_C]);
		CreateHandleNode(HDL_DBC, tabHandles[HDL_D]);
	}

	SetFlag(MUSCLEFLAG_HANDLES_VISIBLE);
	ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

	if (IsCreated()) {
		UpdateName();
		UpdateColours();
	}

	// Remove the handles
	ls_hdl_tm.SetCount(0);
}

void Muscle::RemoveHandles() {
	if (!TestFlag(MUSCLEFLAG_HANDLES_VISIBLE))
		return;

	// register an Undo
	HoldActions hold(IDS_HLD_HANDLEREM);
	HoldData(ls_hdl_tm);

	// Ensure we have enough matrices
	// to store the offset from each node
	ls_hdl_tm.SetCount(NUMNODEREFS);

	// Handles
	INode*	tabHandlesTemp[NUMNODEREFS];

	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();
	INode *parent_node;
	int i, idx;

	const REFS removeOrder[NUMNODEREFS] = {
		HDL_AC, HDL_AB, HDL_BD, HDL_BA,
		HDL_CA, HDL_CD, HDL_DB, HDL_DC,
		HDL_ACB, HDL_BAD, HDL_CAD, HDL_DBC,
		HDL_A, HDL_B, HDL_C, HDL_D // Make sure root handles are in the end
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

	for (i = 0; i < NUMNODEREFS; i++)
		if (tabHandlesTemp[i])	DeSelectAndDelete(tabHandlesTemp[i]);
	ClearFlag(MUSCLEFLAG_HANDLES_VISIBLE);
}

void Muscle::SetHandleSize(float sz)
{
	if (!TestFlag(MUSCLEFLAG_HANDLES_VISIBLE))
		return;

	HoldActions hold(IDS_HLD_HANDLESIZE);
	HoldData(handlesize, this);

	handlesize = sz;
	for (int i = 0; i < NUMNODEREFS; i++)
	{
		if (tabHandles[i])
		{
			HdlObj* pObj = dynamic_cast<HdlObj*>(tabHandles[i]->GetObjectRef());
			if (pObj != NULL)
				pObj->SetSize(handlesize);
		}
	}
}

void Muscle::SetColour(Color* clr)
{
	if (clr == NULL || *clr == clrColour)
		return;

	HoldActions hold(IDS_HLD_MSCLCOLOR);
	HoldData(clrColour, this);
	clrColour = *clr;
	UpdateColours();
};

void Muscle::UpdateColours() {
	DWORD dwColour = asRGB(clrColour);
	int i, u, v;

	for (i = 0; i < NUMNODEREFS; i++) {
		if (tabHandles[i])tabHandles[i]->SetWireColor(dwColour);
	}
	switch (deformer_type) {
	case DEFORMER_BONES:
		for (v = 0; v < tabStrips.Count(); v++)
			for (u = 0; u < nNumUSegs; u++)
				if (tabStrips[v]->tabSegs[u])
					tabStrips[v]->tabSegs[u]->SetWireColor(dwColour);
		break;
	case DEFORMER_MESH:
		node->SetWireColor(asRGB(clrColour));
		break;
	}

}

TSTR Muscle::IGetDeformerType()
{
	switch (deformer_type) {
	case DEFORMER_BONES:	return _T("Bones");	break;  // don't globalize - used for mxs  SA 10/09
	case DEFORMER_MESH:	return _T("Patch");	break;
	default: return _T("");
	}
}

void Muscle::ISetDeformerType(const TSTR& s)
{
	if (s == muscle_bones_str)	SetDeformerType(DEFORMER_BONES);
	if (s == muscle_patch_str)	SetDeformerType(DEFORMER_MESH);
}

void Muscle::SetName(const TSTR& newname) {
	if (strName == newname) return;
	HoldActions hold(IDS_HLD_MSCLNAME);
	HoldData(strName);
	strName = newname;
	UpdateName();
};

void SetUserPropLocalOffset(TimeValue t, INode* node, INode* hdl, TSTR key) {
	INode* hdl_parent_node = FindParent(node);
	Point3 offset;
	if (!hdl_parent_node) {
		node->SetUserPropString(key, _T("Root"));
		offset = hdl->GetNodeTM(t).GetTrans();
	}
	else {
		node->SetUserPropString(key, hdl_parent_node->GetName());
		offset = (hdl->GetNodeTM(t) * Inverse(hdl_parent_node->GetNodeTM(t))).GetTrans();
	}
	TSTR offsetstring;
	key += _T("_Offset");
	offsetstring.printf(_T("%f %f %f"), offset.x, offset.y, offset.z);
	node->SetUserPropString(key, offsetstring);
}

void Muscle::UpdateName()
{
	SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

	TSTR newname = strName;
	switch (GetLMR()) {
	case -1:	newname = newname + _T("L");		break;
	case 0:		newname = newname + _T("M");		break;
	case 1:		newname = newname + _T("R");		break;
	default:	DbgAssert(0);
	}
	if (tabHandles[HDL_A])	tabHandles[HDL_A]->SetName(newname + _T("_A"));
	if (tabHandles[HDL_AC])	tabHandles[HDL_AC]->SetName(newname + _T("_AC"));
	if (tabHandles[HDL_AB])	tabHandles[HDL_AB]->SetName(newname + _T("_AB"));
	if (tabHandles[HDL_ACB])	tabHandles[HDL_ACB]->SetName(newname + _T("_ACB"));

	if (tabHandles[HDL_B])	tabHandles[HDL_B]->SetName(newname + _T("_B"));
	if (tabHandles[HDL_BD])	tabHandles[HDL_BD]->SetName(newname + _T("_BD"));
	if (tabHandles[HDL_BA])	tabHandles[HDL_BA]->SetName(newname + _T("_BA"));
	if (tabHandles[HDL_BAD])	tabHandles[HDL_BAD]->SetName(newname + _T("_BAD"));

	if (tabHandles[HDL_C])	tabHandles[HDL_C]->SetName(newname + _T("_C"));
	if (tabHandles[HDL_CA])	tabHandles[HDL_CA]->SetName(newname + _T("_CA"));
	if (tabHandles[HDL_CD])	tabHandles[HDL_CD]->SetName(newname + _T("_CD"));
	if (tabHandles[HDL_CAD])	tabHandles[HDL_CAD]->SetName(newname + _T("_CAD"));

	if (tabHandles[HDL_D])	tabHandles[HDL_D]->SetName(newname + _T("_D"));
	if (tabHandles[HDL_DB])	tabHandles[HDL_DB]->SetName(newname + _T("_DB"));
	if (tabHandles[HDL_DC])	tabHandles[HDL_DC]->SetName(newname + _T("_DC"));
	if (tabHandles[HDL_DBC])	tabHandles[HDL_DBC]->SetName(newname + _T("_DBC"));

	switch (deformer_type) {
	case DEFORMER_BONES: {
		TSTR strStrandNum = _T("");
		TSTR strSegNum = _T("");
		for (int v = 0; v < tabStrips.Count(); v++)
		{
			strStrandNum.printf(_T("%d"), v);
			for (int j = 0; j < nNumUSegs; j++) {
				INode *segnode = tabStrips[v]->tabSegs[j];
				if (!segnode) continue;

				strSegNum.printf(_T("%d"), j);
				segnode->SetName(newname + strStrandNum + strSegNum);
				//////////////////////////////////////////////////////////////////////
				// user Properties.
				// these will be used by the runtime libraries
				TSTR key = _T("CATMuscleProp_MuscleName");
				segnode->SetUserPropString(key, newname);
				key = _T("CATMuscleProp_numUSegs");
				segnode->SetUserPropInt(key, GetNumUSegs());

				key = _T("CATMuscleProp_numVSegs");
				segnode->SetUserPropInt(key, GetNumVSegs());

				key = _T("CATMuscleProp_Uindex");
				segnode->SetUserPropInt(key, j);

				key = _T("CATMuscleProp_Vindex");
				segnode->SetUserPropInt(key, v);

				Point3 offset;
				TSTR offsetstring;
				TimeValue t = GetCOREInterface()->GetTime();

				if (tabHandles[HDL_A])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_A], _T("CATMuscleProp_A"));
				if (tabHandles[HDL_AB])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_AB], _T("CATMuscleProp_AB"));
				if (tabHandles[HDL_AC])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_AC], _T("CATMuscleProp_AC"));
				if (tabHandles[HDL_ACB])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_ACB], _T("CATMuscleProp_ACB"));

				if (tabHandles[HDL_B])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_B], _T("CATMuscleProp_B"));
				if (tabHandles[HDL_BA])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_BA], _T("CATMuscleProp_BA"));
				if (tabHandles[HDL_BD])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_BD], _T("CATMuscleProp_BD"));
				if (tabHandles[HDL_BAD])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_BAD], _T("CATMuscleProp_BAD"));

				if (tabHandles[HDL_C])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_C], _T("CATMuscleProp_C"));
				if (tabHandles[HDL_CA])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_CA], _T("CATMuscleProp_CA"));
				if (tabHandles[HDL_CD])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_CD], _T("CATMuscleProp_CD"));
				if (tabHandles[HDL_CAD])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_CAD], _T("CATMuscleProp_CAD"));

				if (tabHandles[HDL_D])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_D], _T("CATMuscleProp_D"));
				if (tabHandles[HDL_DC])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_DC], _T("CATMuscleProp_DC"));
				if (tabHandles[HDL_DB])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_DB], _T("CATMuscleProp_DB"));
				if (tabHandles[HDL_DBC])	SetUserPropLocalOffset(t, segnode, tabHandles[HDL_DBC], _T("CATMuscleProp_DBC"));
			}
		}
		break;
	}
	case DEFORMER_MESH: {
		if (node)
			node->SetName(newname);
		break;
	}
	}
	ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);
}

class AddColObjRestore : public RestoreObj {
public:
	INode* mpNode;
	Muscle *muscle;
	int n;

	AddColObjRestore(Muscle *c, int index, INode* pNode) {
		muscle = c;
		n = index;
		mpNode = pNode;
		muscle->SetAFlag(A_HELD);
	}
	~AddColObjRestore() {}
	void Restore(int isUndo) { muscle->RemoveColObj(n); }
	void Redo() { muscle->AddColObj(mpNode); }
	int Size() { return sizeof(AddColObjRestore); }
	void EndHold() { muscle->ClearAFlag(A_HELD); }
};

void Muscle::AddColObj(INode *node) {

	if (node && (node->SuperClassID() == BASENODE_CLASS_ID))
	{
		for (int i = 0; i < GetNumColObjs(); i++)
		{
			if (GetColObj(i) == node)
				return;
		}

		HoldActions hold(IDS_HLD_ADDCOLOBJ);
		int nNumColObjs = tabColObjs.Count();
		if (theHold.Holding())
			theHold.Put(new AddColObjRestore(this, nNumColObjs, node));

		tabColObjs.SetCount(nNumColObjs + 1);
		tabColObjHardness.SetCount(nNumColObjs + 1);
		tabDistortionByColObj.SetCount(nNumColObjs + 1);
		tabColObjFlags.SetCount(nNumColObjs + 1);

		tabColObjs[nNumColObjs] = NULL;
		tabColObjHardness[nNumColObjs] = NULL;
		tabDistortionByColObj[nNumColObjs] = NULL;
		tabColObjFlags[nNumColObjs] = 0;

		// Setting references will happen automatically in the case of an Undo or Redo
		if (!theHold.RestoreOrRedoing())
		{
			ReplaceReference(NUMNODEREFS + (nNumColObjs * 3), node);
			ReplaceReference(NUMNODEREFS + (nNumColObjs * 3) + 1, NewDefaultFloatController());
			ReplaceReference(NUMNODEREFS + (nNumColObjs * 3) + 2, NewDefaultFloatController());

			// collisions with spheres
			if (node->GetObjectRef()->ClassID() == Class_ID(SPHERE_CLASS_ID, 0)) {
				//////////////////////////////////////////////////////////////////////
				// user Properties.
				// these will be used by the runtime libraries
				TSTR key = _T("CATMuscleProp_CollisionSphereRadius");
				IParamBlock2* sphereParams = ((GenSphere*)node->GetObjectRef())->GetParamBlockByID(SPHERE_PARAMBLOCK_ID);
				DbgAssert(sphereParams);
				if (sphereParams)
				{
					float radius;
					sphereParams->GetValue(SPHERE_RADIUS, 0, radius, FOREVER);
					node->SetUserPropFloat(key, radius);
				}
			}

			SetColObjHardness(nNumColObjs, 1.0f, GetCOREInterface()->GetTime());
			SetColObjDistortion(nNumColObjs, 0.0f, GetCOREInterface()->GetTime());
		}

		UpdateMuscle();
	}
}

void Muscle::RemoveColObj(int n) {

	HoldActions hold(IDS_TT_REMCLSNOBJ);
	int refindex = NUMNODEREFS + (n * 3);
	DeleteReference(refindex);
	DeleteReference(refindex + 1);
	DeleteReference(refindex + 2);

	// Now all our pointers have been nulled
	HoldData(tabColObjs);
	HoldData(tabColObjHardness);
	HoldData(tabDistortionByColObj);
	HoldData(tabColObjFlags);

	tabColObjs.Delete(n, 1);
	tabColObjHardness.Delete(n, 1);
	tabDistortionByColObj.Delete(n, 1);
	tabColObjFlags.Delete(n, 1);
}

void Muscle::MoveColObjUp(int n) {
	if (n<1 || n >(GetNumColObjs() - 1))
		return;

	MoveColObjDown(n - 1);
}

void Muscle::MoveColObjDown(int n) {
	if (n<0 || n >(GetNumColObjs() - 2))
		return;

	// Save the data at n and n + 1
	// Ensure that the first and last items registered
	// have a callback to refresh the UI
	HoldTabData(tabColObjFlags, n, static_cast<IDataRestoreOwner<int>*>(this));
	HoldTabData(tabColObjs, n);
	HoldTabData(tabColObjHardness, n);
	HoldTabData(tabDistortionByColObj, n);

	HoldTabData(tabColObjs, n + 1);
	HoldTabData(tabColObjHardness, n + 1);
	HoldTabData(tabDistortionByColObj, n + 1);
	HoldTabData(tabColObjFlags, n + 1, static_cast<IDataRestoreOwner<int>*>(this));

	INode* sph = tabColObjs[n + 1];
	Control* hrd = tabColObjHardness[n + 1];
	Control* dist = tabDistortionByColObj[n + 1];
	int fl = tabColObjFlags[n + 1];

	tabColObjs[n + 1] = tabColObjs[n];
	tabColObjHardness[n + 1] = tabColObjHardness[n];
	tabDistortionByColObj[n + 1] = tabDistortionByColObj[n];
	tabColObjFlags[n + 1] = tabColObjFlags[n];

	tabColObjs[n] = sph;
	tabColObjHardness[n] = hrd;
	tabDistortionByColObj[n] = dist;
	tabColObjFlags[n] = fl;

	RefreshUI();
	UpdateMuscle();
}

float Muscle::GetColObjDistortion(int n, TimeValue t, Interval& valid)
{
	float v = 0.0f;
	if (tabDistortionByColObj[n] != NULL)
		tabDistortionByColObj[n]->GetValue(t, (void*)&v, valid);
	return v;
}

void Muscle::SetColObjDistortion(int n, float newVal, TimeValue t)
{
	if (tabDistortionByColObj[n] != NULL)
	{
		HoldActions hold(IDS_HLD_DISTORTION);
		tabDistortionByColObj[n]->SetValue(t, (void*)&newVal);
	}
}

float Muscle::GetColObjHardness(int n, TimeValue t, Interval& valid)
{
	float v = 0.0f;
	if (tabColObjHardness[n] != NULL)
		tabColObjHardness[n]->GetValue(t, (void*)&v, valid);
	return v;
}

void Muscle::SetColObjHardness(int n, float newVal, TimeValue t)
{
	if (tabColObjHardness[n] != NULL) { HoldActions hold(IDS_HLD_HARDNESS); tabColObjHardness[n]->SetValue(t, (void*)&newVal); }
}

void Muscle::UpdateHandles(TimeValue t, Interval&valid)
{
	if (!handlesvalid.InInterval(t)) {
		//		if(g_nNumSaves>CATDEMO_MAXSAVES) t=0;

		handlesvalid = FOREVER;

		Matrix3 tm;
		for (int i = 0; i < NUMNODEREFS; i++) {
			//	if(i==HDL_ACB||i==HDL_BAD||i==HDL_CAD||i==HDL_DBC) continue;
			if (tabHandles[i])	tm = tabHandles[i]->GetNodeTM(t, &handlesvalid);
			else				tm.IdentityMatrix();
			if (TestFlag(MUSCLEFLAG_HANDLES_VISIBLE))
				ws_hdl_pos[i] = tm.GetTrans();
			else if (ls_hdl_tm.Count() > i)
				ws_hdl_pos[i] = tm.GetTrans() + tm.VectorTransform(ls_hdl_tm[i].GetTrans());
			else // This is technically an error condition, but it can occur legally during file open/close.
				ws_hdl_pos[i] = tm.GetTrans();
		}

		// estimate the imaginary handles. To calculate a bezier patch we need 16 handles
		// but we only get given 12, this means we have to guess the 4 handles that would
		// have been in the middle of the patch
		if (!TestFlag(MUSCLEFLAG_MIDDLE_HANDLES)) {
			if (!tabHandles[HDL_ACB]) ws_hdl_pos[HDL_ACB] = ws_hdl_pos[HDL_A] + ((ws_hdl_pos[HDL_AB] - ws_hdl_pos[HDL_A])*0.7f) + ((ws_hdl_pos[HDL_CD] - ws_hdl_pos[HDL_C])*0.3f)/**/ + ((ws_hdl_pos[HDL_AC] - ws_hdl_pos[HDL_A])*0.7f) + ((ws_hdl_pos[HDL_BD] - ws_hdl_pos[HDL_B])*0.3f)/**/;
			if (!tabHandles[HDL_BAD]) ws_hdl_pos[HDL_BAD] = ws_hdl_pos[HDL_B] + ((ws_hdl_pos[HDL_BA] - ws_hdl_pos[HDL_B])*0.7f) + ((ws_hdl_pos[HDL_DC] - ws_hdl_pos[HDL_D])*0.3f)/**/ + ((ws_hdl_pos[HDL_BD] - ws_hdl_pos[HDL_B])*0.7f) + ((ws_hdl_pos[HDL_AC] - ws_hdl_pos[HDL_A])*0.3f)/**/;
			if (!tabHandles[HDL_CAD]) ws_hdl_pos[HDL_CAD] = ws_hdl_pos[HDL_C] + ((ws_hdl_pos[HDL_CD] - ws_hdl_pos[HDL_C])*0.7f) + ((ws_hdl_pos[HDL_AB] - ws_hdl_pos[HDL_A])*0.3f)/**/ + ((ws_hdl_pos[HDL_CA] - ws_hdl_pos[HDL_C])*0.7f) + ((ws_hdl_pos[HDL_DB] - ws_hdl_pos[HDL_D])*0.3f)/**/;
			if (!tabHandles[HDL_DBC]) ws_hdl_pos[HDL_DBC] = ws_hdl_pos[HDL_D] + ((ws_hdl_pos[HDL_DC] - ws_hdl_pos[HDL_D])*0.7f) + ((ws_hdl_pos[HDL_BA] - ws_hdl_pos[HDL_B])*0.3f)/**/ + ((ws_hdl_pos[HDL_DB] - ws_hdl_pos[HDL_D])*0.7f) + ((ws_hdl_pos[HDL_CA] - ws_hdl_pos[HDL_C])*0.3f)/**/;
		}

		//TODO: here I plan on caching all the positions of all the points on fht esurface of the bezier patch
		// then I can do more clever things with collisiion detection.

		// I am pretty sure all this stuff goes, its just
		// that I have disbled it untill it has been tested

		int usegs = GetNumUSegs();
		int vsegs = GetNumVSegs();
		int u, v;
		for (u = 0; u <= usegs; u++) {
			// here we allocate usegs+1 and vsegs+1 space in the array
			// this is because each seg has a start and and end
			if (tabPosCache.size() <= usegs) {
				tabPosCache.resize(usegs + 1);
			}
			float floatu = (float)u / (float)usegs;
			Point3 p1 = CalcBezier(ws_hdl_pos[HDL_A], ws_hdl_pos[HDL_AC], ws_hdl_pos[HDL_CA], ws_hdl_pos[HDL_C], floatu);
			Point3 p2 = CalcBezier(ws_hdl_pos[HDL_AB], ws_hdl_pos[HDL_ACB], ws_hdl_pos[HDL_CAD], ws_hdl_pos[HDL_CD], floatu);
			Point3 p3 = CalcBezier(ws_hdl_pos[HDL_BA], ws_hdl_pos[HDL_BAD], ws_hdl_pos[HDL_DBC], ws_hdl_pos[HDL_DC], floatu);
			Point3 p4 = CalcBezier(ws_hdl_pos[HDL_B], ws_hdl_pos[HDL_BD], ws_hdl_pos[HDL_DB], ws_hdl_pos[HDL_D], floatu);

			for (v = 0; v <= vsegs; v++) {

				if (tabPosCache[u].size() <= vsegs)
					tabPosCache[u].resize(vsegs + 1);
				float floatv = (float)v / (float)vsegs;
				tabPosCache[u][v] = CalcBezier(p1, p2, p3, p4, floatv);
			}
		}

		if (tabColObjs.Count() > 0) {
			// do collision detection
			std::vector<std::vector<Point3>> tabCollisionCache;
			tabCollisionCache.resize(usegs + 1);
			for (u = 0; u <= usegs; u++) {
				tabCollisionCache[u].resize(vsegs + 1);
			}

			for (u = 0; u <= usegs; u++) {
				for (v = 0; v <= vsegs; v++) {
					Point3 vec1, vec2, up, p = tabPosCache[u][v];
					if (u > 0 && u < usegs)	vec1 = tabPosCache[u + 1][v] - tabPosCache[u - 1][v];
					else if (u > 0)		vec1 = tabPosCache[u][v] - tabPosCache[u - 1][v];
					else if (u < usegs)	vec1 = tabPosCache[u + 1][v] - tabPosCache[u][v];

					if (v > 0 && v < vsegs)	vec2 = tabPosCache[u][v + 1] - tabPosCache[u][v - 1];
					else if (v > 0)		vec2 = tabPosCache[u][v] - tabPosCache[u][v - 1];
					else if (v < vsegs)	vec2 = tabPosCache[u][v + 1] - tabPosCache[u][v];

					up = Normalize(CrossProd(vec2, vec1));

					for (int i = 0; i < tabColObjs.Count(); i++) {
						float spherehardness;
						tabColObjHardness[i]->GetValue(t, (void*)&spherehardness, handlesvalid);;
						if (spherehardness > 0.0f) {
							// find a colllion with the sphere in question
							// each time a collision is found, the result is fed into the next sphere's DetectCol
							float distortion;
							tabDistortionByColObj[i]->GetValue(t, (void*)&distortion, handlesvalid);;
							if (GetObjXDistortion(i))
								p = DetectCol(tabColObjs[i]->GetNodeTM(t).GetRow(0), tabColObjs[i], handlesvalid, p, t, distortion, spherehardness, tabColObjFlags[i]);
							else p = DetectCol(up, tabColObjs[i], handlesvalid, p, t, distortion, spherehardness, tabColObjFlags[i]);
						}
					}
					tabCollisionCache[u][v] = p;
				}
			}
			// put the positions back into
			for (u = 0; u <= usegs; u++) {
				for (v = 0; v <= vsegs; v++)
					tabPosCache[u][v] = tabCollisionCache[u][v];
			}
		}

	}
	// every bone segment shares this one validity interval
	valid = handlesvalid;
}
Point3 Muscle::GetSegPos(TimeValue t, int vindex, int uindex, Interval&valid)
{
	// each muscle holds a cache of all the vertex positions.
	// this cache is only filled out once per evaluation
	UpdateHandles(t, valid);

	if (uindex < tabPosCache.size())
	{
		std::vector<Point3>& vTab = tabPosCache[uindex];
		if (vindex < vTab.size())
		{
			return vTab[vindex];
		}
	}

#ifdef _DEBUG
	// During a Undo/Redo we can sometimes re-evaluate while not
	// all nodes are present (and cache is incomplete)
	if (!theHold.RestoreOrRedoing())
	{
		DbgAssert(!_T("ERROR: Invalid indicies here"));
	}
#endif // _DEBUG
	return Point3::Origin;
};

void Muscle::SetColObjFlag(int flag, BOOL tf, int n) {
	HoldActions hold(IDS_HLD_COLOBJSETTINGS);
	HoldTabData(tabColObjFlags, n, this);
	if (tf) { tabColObjFlags[n] |= flag; }
	else { tabColObjFlags[n] &= ~flag; }
	UpdateMuscle();
}

void	Muscle::SetLMR(int newlmr) {
	HoldActions hold(IDS_HLD_LRMSETTINGS);
	HoldData(lmr, this);
	lmr = newlmr;
	UpdateName();
};

void	Muscle::SetMirrorAxis(Axis newmirroraxis) {
	HoldActions hold(IDS_HLD_MIRRORAXIS);
	HoldData(mirroraxis, this);
	mirroraxis = newmirroraxis;
};

void	Muscle::SetFlag(USHORT f, BOOL tf/*=TRUE*/) {
	if ((tf && TestFlag(f)) || (!tf && !TestFlag(f))) return;
	// register an Undo
	HoldData(flags, this);
	if (tf) flags |= f; else flags &= ~f;
}

Matrix3 Muscle::GetLSHandleTM(int id, TimeValue t) {
	if (TestFlag(MUSCLEFLAG_HANDLES_VISIBLE) && tabHandles[id]) {
		if (tabHandles[id]->GetParentNode()->IsRootNode())
			return tabHandles[id]->GetNodeTM(t);
		else return tabHandles[id]->GetNodeTM(t) * Inverse(tabHandles[id]->GetParentNode()->GetNodeTM(t));
	}
	else if (ls_hdl_tm.Count() > id)
		return ls_hdl_tm[id];
	return Matrix3(1);
}

void Muscle::SetLSHandleTM(int id, TimeValue t, Matrix3 &tm, BOOL bMirror) {
	if (bMirror)
		MirrorMatrix(tm, mirroraxis);

	if (TestFlag(MUSCLEFLAG_HANDLES_VISIBLE) && tabHandles[id]) {
		if (!tabHandles[id]->GetParentNode()->IsRootNode())
			tm = tm * tabHandles[id]->GetParentNode()->GetNodeTM(t);
		tabHandles[id]->SetNodeTM(t, tm);
	}
	else
	{
		// Fall through here if no handles present.
		if (ls_hdl_tm.Count() <= id)
			ls_hdl_tm.SetCount(id + 1);

		ls_hdl_tm[id] = tm;;
	}
}

void Muscle::PasteMuscle(ReferenceTarget* pasteRef)
{
	Muscle* pasteMuscle = nullptr;

	if (pasteRef != nullptr)
	{
		if (GetCATMuscleInterface(pasteRef) != nullptr)
		{
			pasteMuscle = (Muscle*)GetCATMuscleInterface(pasteRef);
		}
		else
		{
			// In the case that the function is called by MAXScript,
			// it's the node that contains the Muscle passed in.
			INode* node = static_cast<INode*>(pasteRef->GetInterface(INODE_INTERFACE));
			if (node != nullptr)
			{
				ObjectState os = node->EvalWorldState(MAXScript_time());
				if (os.obj != nullptr)
				{
					pasteMuscle = (Muscle*)GetCATMuscleInterface(os.obj);
				}
			}
		}
	}

	if (pasteMuscle == nullptr) return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	Interface *ip = GetCOREInterface();

	if (pasteRef == this) {
		// Keep track of the previously selcted nodes.
		// We don't want the current rollout to be destroyed before we are done here
		INodeTab selectednodes;
		for (int i = 0; i < ip->GetSelNodeCount(); i++) {
			INode *selnode = ip->GetSelNode(i);
			selectednodes.Append(1, &selnode);
		}
		SetFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

		INodeTab nodes;
		switch (deformer_type) {
		case DEFORMER_BONES:
			if (tabStrips.Count() > 0 && tabStrips[0] &&
				tabStrips[0]->tabSegs.Count() > 0 && tabStrips[0]->tabSegs[0])
				nodes.Append(1, &tabStrips[0]->tabSegs[0]);
			break;
		case DEFORMER_MESH:
			if (node)
				nodes.Append(1, &node);
			break;
		}
		if (nodes.Count() == 0) {
			theHold.Cancel();
			return;
		}

		Point3 offset(0.0f, 0.0f, 0.0f);
		bool expandHierarchies = false;
		CloneType cloneType = NODE_COPY;
		INodeTab resultSource;
		INodeTab resultTarget;

		g_bKeepRollouts = TRUE;

		ip->CloneNodes(nodes, offset, expandHierarchies, cloneType, &resultSource, &resultTarget);
		DbgAssert(resultTarget.Count() > 0);
		if (resultTarget.Count() == 0) {
			theHold.Cancel();
			return;
		}
		pasteMuscle = (Muscle*)GetCATMuscleInterface(resultTarget[0]->GetObjectRef()->FindBaseObject());

		// Make sure our rollout is still being displayed
		ip->ClearNodeSelection(FALSE);
		ip->SelectNodeTab(selectednodes, TRUE, FALSE);
		ClearFlag(MUSCLEFLAG_KEEP_ROLLOUTS);

		if (!pasteMuscle) {
			theHold.Cancel();
			return;
		}
		pasteMuscle->SetLMR(-GetLMR());

		if ((GetLMR() != pasteMuscle->GetLMR()) && (GetLMR() + pasteMuscle->GetLMR() == 0))
		{
			ReparentNode(tabHandles[HDL_A], pasteMuscle->tabHandles[HDL_B], mirroraxis);
			ReparentNode(tabHandles[HDL_AB], pasteMuscle->tabHandles[HDL_BA], mirroraxis);
			ReparentNode(tabHandles[HDL_BA], pasteMuscle->tabHandles[HDL_AB], mirroraxis);
			ReparentNode(tabHandles[HDL_B], pasteMuscle->tabHandles[HDL_A], mirroraxis);

			ReparentNode(tabHandles[HDL_AC], pasteMuscle->tabHandles[HDL_BD], mirroraxis);
			ReparentNode(tabHandles[HDL_ACB], pasteMuscle->tabHandles[HDL_BAD], mirroraxis);
			ReparentNode(tabHandles[HDL_BAD], pasteMuscle->tabHandles[HDL_ACB], mirroraxis);
			ReparentNode(tabHandles[HDL_BD], pasteMuscle->tabHandles[HDL_AC], mirroraxis);

			ReparentNode(tabHandles[HDL_CA], pasteMuscle->tabHandles[HDL_DB], mirroraxis);
			ReparentNode(tabHandles[HDL_CAD], pasteMuscle->tabHandles[HDL_DBC], mirroraxis);
			ReparentNode(tabHandles[HDL_DBC], pasteMuscle->tabHandles[HDL_CAD], mirroraxis);
			ReparentNode(tabHandles[HDL_DB], pasteMuscle->tabHandles[HDL_CA], mirroraxis);

			ReparentNode(tabHandles[HDL_C], pasteMuscle->tabHandles[HDL_D], mirroraxis);
			ReparentNode(tabHandles[HDL_CD], pasteMuscle->tabHandles[HDL_DC], mirroraxis);
			ReparentNode(tabHandles[HDL_DC], pasteMuscle->tabHandles[HDL_CD], mirroraxis);
			ReparentNode(tabHandles[HDL_D], pasteMuscle->tabHandles[HDL_C], mirroraxis);

			// We actually want the settings to be pasted onto the new muscle
			pasteMuscle->PasteMuscle(this);
		}
		pasteMuscle->UpdateMuscle();
		if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_MSCLCOPYPASTE));

		return;
	}

	BOOL bMirror = FALSE;
	// if this is a left paste onto a right then mirror
	if ((GetLMR() != pasteMuscle->GetLMR()) && (GetLMR() + pasteMuscle->GetLMR() == 0))
		bMirror = TRUE;

	SetNumVSegs(pasteMuscle->GetNumVSegs());
	SetNumUSegs(pasteMuscle->GetNumUSegs());
	SetHandleSize(pasteMuscle->GetHandleSize());

	SetHandlesVisible(pasteMuscle->GetHandlesVisible());
	SetMiddleHandles(pasteMuscle->GetMiddleHandles());

	Matrix3 tm;
	TimeValue t = ip->GetTime();
	SetXFormPacket SetVal;

	/////////////////////////////////////////////////////////////////////////
	// Copy valuse from the left side of the muscle to the right side
	Matrix3 m;
	m = pasteMuscle->GetLSHandleTM(HDL_B, t); SetLSHandleTM(HDL_A, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_BA, t); SetLSHandleTM(HDL_AB, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_AB, t); SetLSHandleTM(HDL_BA, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_A, t); SetLSHandleTM(HDL_B, t, m, bMirror);

	m = pasteMuscle->GetLSHandleTM(HDL_BD, t); SetLSHandleTM(HDL_AC, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_BAD, t); SetLSHandleTM(HDL_ACB, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_ACB, t); SetLSHandleTM(HDL_BAD, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_AC, t); SetLSHandleTM(HDL_BD, t, m, bMirror);

	m = pasteMuscle->GetLSHandleTM(HDL_DB, t); SetLSHandleTM(HDL_CA, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_DBC, t); SetLSHandleTM(HDL_CAD, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_CAD, t); SetLSHandleTM(HDL_DBC, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_CA, t); SetLSHandleTM(HDL_DB, t, m, bMirror);

	m = pasteMuscle->GetLSHandleTM(HDL_D, t); SetLSHandleTM(HDL_C, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_DC, t); SetLSHandleTM(HDL_CD, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_CD, t); SetLSHandleTM(HDL_DC, t, m, bMirror);
	m = pasteMuscle->GetLSHandleTM(HDL_C, t); SetLSHandleTM(HDL_D, t, m, bMirror);

	//////////////////////////////////////////////////////////////////////////
	// Copy Colours
	SetColour(pasteMuscle->GetColour());

	/////////////////////////////////////////////////////
	// Copy Name
	strName = pasteMuscle->GetName();
	SetName(strName);

	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_MSCLPASTE));
}

void Muscle::GetTrans(TimeValue t, int v, int u, Matrix3 &tm, Interval &valid)
{
	switch (deformer_type) {
	case DEFORMER_BONES: {
		Point3 bl = GetSegPos(t, v, u, valid);
		Point3 br = GetSegPos(t, v, (u + 1), valid);
		Point3 tl = GetSegPos(t, (v + 1), u, valid);
		Point3 tr = GetSegPos(t, (v + 1), (u + 1), valid);

		tm.SetTrans((bl + br + tr + tl) / 4.0f);
		tm.SetRow(0, (((tr + tl) / 2.0f) - ((br + bl) / 2.0f)));
		tm.SetRow(1, (((br + tr) / 2.0f) - ((bl + tl) / 2.0f)));
		tm.SetRow(2, Normalize(CrossProd(tm.GetRow(0), tm.GetRow(1))));

		// dirty hack to make the muscles export to mayhem.
		float xlength = Length(tm.GetRow(0));

		if (GetRemoveSkew()) {
			tm.SetRow(0, Normalize(CrossProd(tm.GetRow(1), tm.GetRow(2))) * xlength);
		}
		break;
	}
	case DEFORMER_MESH: {
		UpdateHandles(t, valid);

		Point3 vecY = Normalize((ws_hdl_pos[HDL_C] - ws_hdl_pos[HDL_A]) + (ws_hdl_pos[HDL_D] - ws_hdl_pos[HDL_B]));
		Point3 vecZ = Normalize(CrossProd((ws_hdl_pos[HDL_B] - ws_hdl_pos[HDL_A]) + (ws_hdl_pos[HDL_D] - ws_hdl_pos[HDL_C]), vecY));
		Point3 vecX = Normalize(CrossProd(vecY, vecZ));

		tm.SetTrans(((ws_hdl_pos[HDL_A] + ((ws_hdl_pos[HDL_D] - ws_hdl_pos[HDL_A]) / 2.0f)) + (ws_hdl_pos[HDL_B] + ((ws_hdl_pos[HDL_C] - ws_hdl_pos[HDL_B]) / 2.0f))) / 2.0f);
		tm.SetRow(0, vecX);
		tm.SetRow(1, vecY);
		tm.SetRow(2, vecZ);

		//	this->tm = tm;
		break;
	}
	}
};

// this is for moving a muscle around using the segments.
// it allows the user to treat the whole muscle as one object when moving and rotating
// it in the viewport. When the user tries to move or rotate one segment this
// method in turn, transforms the corner handles effectively moving the whole muscle
void Muscle::SetValue(TimeValue t, SetXFormPacket *ptr, int commit, Control* pOriginator)
{
	if (ptr->command == XFORM_SCALE) return;

	// If multiple segments are set at the same time, we will have
	// redundant calls to this SetValue.  We can ignore
	// all calls after the first one, and we monitor
	// this using the A_HELD flag.  If our data is not
	// held though it is the first pass through of the
	// SetValue, and this should always be processed.
	if (TestAFlag(A_HELD) && (TestAFlag(A_OBJ_LONG_CREATE) || IsPointerHeld(&ls_hdl_tm)))
		return;

	// Hold the transforms.  This may not always be necessary,
	// but we need to hold something here so we can leverage
	// the HoldData pointer testing.
	HoldData(ls_hdl_tm, this);

	// This is cleared in the callback for ls_hdl_tm to allow subsequent set values
	if (theHold.Holding())
		SetAFlag(A_HELD);

	if (TestFlag(MUSCLEFLAG_HANDLES_VISIBLE))
	{
		for (int i = 0; i < NUMNODEREFS; i++)
		{
			if (tabHandles[i]) {

				// Do not apply the SetValue if the handle is already selected
				if (tabHandles[i]->Selected())
					continue;

				// Don't apply if the handle is parented to another handle in this
				if (!tabHandles[i]->GetParentNode()->IsRootNode() && tabHandles[i]->GetParentNode()->GetTMController()->GetInterface(I_MASTER) == this)
					continue;

				SetXFormPacket hdl_ptr = *ptr;
				// this forces all the handles to rotate around the same axis
				hdl_ptr.localOrigin = 0;
				hdl_ptr.tmParent = tabHandles[i]->GetParentTM(t);
				tabHandles[i]->GetTMController()->SetValue(t, (void*)&hdl_ptr, commit, CTRL_RELATIVE);
			}
		}
	}
	else
	{
		DbgAssert(ls_hdl_tm.Count() == NUMNODEREFS);
		if (ls_hdl_tm.Count() < NUMNODEREFS)
			ls_hdl_tm.SetCount(NUMNODEREFS);

		for (int i = 0; i < NUMNODEREFS; i++)
		{
			SetXFormPacket hdl_ptr = *ptr;
			if (tabHandles[i])	hdl_ptr.tmParent = tabHandles[i]->GetNodeTM(t);
			else				hdl_ptr.tmParent.IdentityMatrix();
			switch (ptr->command)
			{
			case XFORM_MOVE: {
				Point3 pos = ls_hdl_tm[i].GetTrans();
				pos += VectorTransform(hdl_ptr.tmAxis*Inverse(hdl_ptr.tmParent), hdl_ptr.p);
				ls_hdl_tm[i].SetTrans(pos);
				break;
			}
			case XFORM_ROTATE: {
				Matrix3 ptm = ls_hdl_tm[i] * hdl_ptr.tmParent;
				Matrix3 mat;
				hdl_ptr.q.MakeMatrix(mat);
				mat = XFormMat(ptm*Inverse(hdl_ptr.tmAxis), mat);
				ls_hdl_tm[i].SetTrans(ls_hdl_tm[i].GetTrans() + mat.GetTrans());

				hdl_ptr.aa.axis = Normalize(VectorTransform(hdl_ptr.tmAxis*Inverse(hdl_ptr.tmParent), hdl_ptr.aa.axis));
				Point3 currSetPos = ls_hdl_tm[i].GetTrans();
				RotateMatrix(ls_hdl_tm[i], hdl_ptr.aa);
				ls_hdl_tm[i].SetTrans(currSetPos);
				break;
			}
			case XFORM_SET:
				// TODO:
				break;
			}
		}
	}
	UpdateMuscle();
};

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATMusclePLCB : public PostLoadCallback {
protected:
	Muscle		*muscle;

public:
	CATMusclePLCB(Muscle *pOwner) { muscle = pOwner; }

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		if (muscle->dVersion < CATMUSCLE_VERSION_0860) {
			muscle->flags |= MUSCLEFLAG_HANDLES_VISIBLE;

			iload->SetObsolete();
		}

		muscle->dVersion = CAT_VERSION_CURRENT;

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define MUSCLE_VERSION					0
#define MUSCLE_DEFORMER_TYPE			1
#define MUSCLE_FLAGS_CHUNK				2
#define MUSCLE_NAME_CHUNK				4
#define MUSCLE_LMR_CHUNK				5
#define MUSCLE_COLOUR_CHUNK		6
#define MUSCLE_COLOUREND_CHUNK			8
#define MUSCLE_HANDLESIZE_CHUNK			9

#define MUSCLE_MUSCLEUP_CHUNK			10
#define MUSCLE_NUMSPHERES_CHUNK			12
#define MUSCLE_SPHEREFLAGS_CHUNK		14

#define MUSCLE_MIRRORAXIS_CHUNK			15
#define MUSCLE_NUMSAVES_CHUNK			18
#define MUSCLE_LS_HDL_TMS				20

IOResult Muscle::LoadMuscle(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;//, refID;
	int size = 0, i;//, j;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case MUSCLE_VERSION:
			res = iload->Read(&dVersion, sizeof(DWORD), &nb);
			break;
		case MUSCLE_DEFORMER_TYPE:
			res = iload->ReadEnum(&deformer_type, sizeof(int), &nb);
			break;
		case MUSCLE_FLAGS_CHUNK:
			res = iload->Read(&flags, sizeof USHORT, &nb);
			break;
		case MUSCLE_NAME_CHUNK: {
			TCHAR *strBuf;
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) {
				TSTR temp(strBuf);
				strName = temp;
			}
			break;
		}
		case MUSCLE_LMR_CHUNK:
			res = iload->Read(&lmr, sizeof(int), &nb);
			break;
		case MUSCLE_MIRRORAXIS_CHUNK:
			res = iload->ReadVoid((void*)&mirroraxis, sizeof(Axis), &nb);
			break;
		case MUSCLE_COLOUR_CHUNK: {
			DWORD dwColour = 0L;
			res = iload->Read(&dwColour, sizeof DWORD, &nb);
			clrColour = Color(dwColour);
			break;
		}
		case MUSCLE_HANDLESIZE_CHUNK:
			res = iload->Read(&handlesize, sizeof(float), &nb);
			break;
		case MUSCLE_NUMSPHERES_CHUNK:
			res = iload->Read(&size, sizeof(int), &nb);
			tabColObjs.SetCount(size);
			tabDistortionByColObj.SetCount(size);
			tabColObjHardness.SetCount(size);
			tabColObjFlags.SetCount(size);
			for (i = 0; i < size; i++) {
				tabColObjs[i] = NULL;
				tabDistortionByColObj[i] = NULL;
				tabColObjHardness[i] = NULL;
				tabColObjFlags[i] = 0;
			}
			break;
		case MUSCLE_MUSCLEUP_CHUNK:
			res = iload->Read(&p3MuscleUp, sizeof(float), &nb);
			break;
		case MUSCLE_SPHEREFLAGS_CHUNK:
			for (i = 0; i < tabColObjFlags.Count(); i++)
				res = iload->Read(&tabColObjFlags[i], sizeof(int), &nb);
			break;
#ifndef CAT_NO_AUTH
		case MUSCLE_NUMSAVES_CHUNK: {
			int loadednumsaves;
			res = iload->Read(&loadednumsaves, sizeof(int), &nb);
			g_nNumSaves = max(loadednumsaves, g_nNumSaves);
			break;
		}
#endif
		case MUSCLE_LS_HDL_TMS:
			ls_hdl_tm.SetCount(NUMNODEREFS);
			for (i = 0; i < NUMNODEREFS; i++)
				res = iload->Read(&ls_hdl_tm[i], sizeof(Matrix3), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// this flag should always be set for loaded files
	flags |= MUSCLEFLAG_CREATED;

	iload->RegisterPostLoadCallback(new HdlObjOwnerPLCB<Muscle>(*this));
	return IO_OK;
}

IOResult Muscle::SaveMuscle(ISave *isave)
{
	DWORD nb;//, refID;
	int i;

	isave->BeginChunk(MUSCLE_VERSION);
	dVersion = CAT_VERSION_CURRENT;
	isave->Write(&dVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_DEFORMER_TYPE);
	isave->WriteEnum(&deformer_type, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_FLAGS_CHUNK);
	isave->Write(&flags, sizeof(USHORT), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_NAME_CHUNK);
	isave->WriteCString(strName.data());
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_LMR_CHUNK);
	isave->Write(&lmr, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_MIRRORAXIS_CHUNK);
	isave->WriteVoid(&mirroraxis, sizeof(Axis), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_COLOUR_CHUNK);
	DWORD dwColour = asRGB(clrColour);
	isave->Write(&dwColour, sizeof DWORD, &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_HANDLESIZE_CHUNK);
	isave->Write(&handlesize, sizeof(float), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_NUMSPHERES_CHUNK);
	int numspheres = tabColObjs.Count();
	isave->Write(&numspheres, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_MUSCLEUP_CHUNK);
	isave->Write(&p3MuscleUp, sizeof(float), &nb);
	isave->EndChunk();

	isave->BeginChunk(MUSCLE_SPHEREFLAGS_CHUNK);
	for (i = 0; i < tabColObjFlags.Count(); i++) {
		isave->Write(&tabColObjFlags[i], sizeof(int), &nb);
	}
	isave->EndChunk();

	if (ls_hdl_tm.Count() > 0)
	{
		DbgAssert(ls_hdl_tm.Count() == NUMNODEREFS);
		isave->BeginChunk(MUSCLE_LS_HDL_TMS);
		for (i = 0; i < ls_hdl_tm.Count(); i++)
			isave->Write(&ls_hdl_tm[i], sizeof(Matrix3), &nb);
		isave->EndChunk();
	}
	return IO_OK;
}

////////////////////////////////////////////////////////////////////////////////
//

#define MUSCLE_MUSCLE					0

#define MUSCLEPATCH_NUMVSEGS_CHUNK		2
#define MUSCLEPATCH_NUMUSEGS_CHUNK		4
#define MUSCLEPATCH_NODE				6

#define MUSCLEBONES_NUMSTRANDS_CHUNK	2
#define MUSCLEBONES_NUMSEGS_CHUNK		4
#define MUSCLEBONES_SEGTRANS_CHUNK		6

IOResult Muscle::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L, refID = 0L;
	int val = 0, i = 0, j = 0;

	while (IO_OK == (res = iload->OpenChunk())) {
		if (iload->CurChunkID() == MUSCLE_MUSCLE) {
			res = Muscle::LoadMuscle(iload);
		}
		switch (deformer_type) {
		case DEFORMER_BONES: {
			switch (iload->CurChunkID()) {
			case MUSCLEBONES_NUMSTRANDS_CHUNK: {
				res = iload->Read(&val, sizeof(int), &nb);
				tabStrips.SetCount(val);
				for (int v = 0; v < val; v++)	tabStrips[v] = new MuscleBonesStrip();
				break;
			}
			case MUSCLEBONES_NUMSEGS_CHUNK:
				res = iload->Read(&val, sizeof(int), &nb);
				for (int v = 0; v < tabStrips.Count(); v++)
					tabStrips[v]->tabSegs.SetCount(val);
				nNumUSegs = val;
				break;
			case MUSCLEBONES_SEGTRANS_CHUNK:
				for (i = 0; i < tabStrips.Count(); i++) {
					for (j = 0; j < nNumUSegs; j++) {
						res = iload->Read(&refID, sizeof DWORD, &nb);
						if (res == IO_OK)
							iload->RecordBackpatch(refID, (void**)&tabStrips[i]->tabSegs[j]);
					}
				}
				break;
			}
			break;
		}
		case DEFORMER_MESH: {
			switch (iload->CurChunkID()) {
			case MUSCLEPATCH_NUMVSEGS_CHUNK:
				res = iload->Read(&nNumVSegs, sizeof(int), &nb);
				break;
			case MUSCLEPATCH_NUMUSEGS_CHUNK:
				res = iload->Read(&nNumUSegs, sizeof(int), &nb);
				break;
			case MUSCLEPATCH_NODE:
				res = iload->Read(&refID, sizeof DWORD, &nb);
				if (res == IO_OK && refID != (DWORD)-1)
					iload->RecordBackpatch(refID, (void**)&node);
				break;
			}
			break;
		}
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new CATMusclePLCB(this));
	return IO_OK;
}

IOResult Muscle::Save(ISave *isave)
{
	DWORD nb, refID;
	int i, j;

	// The muscle handle
	isave->BeginChunk(MUSCLE_MUSCLE);
	Muscle::SaveMuscle(isave);
	isave->EndChunk();

	switch (deformer_type) {
	case DEFORMER_BONES: {

		isave->BeginChunk(MUSCLEBONES_NUMSTRANDS_CHUNK);
		int nNumVSegs = tabStrips.Count();
		isave->Write(&nNumVSegs, sizeof(int), &nb);
		isave->EndChunk();

		isave->BeginChunk(MUSCLEBONES_NUMSEGS_CHUNK);
		isave->Write(&nNumUSegs, sizeof(int), &nb);
		isave->EndChunk();

		isave->BeginChunk(MUSCLEBONES_SEGTRANS_CHUNK);
		for (i = 0; i < tabStrips.Count(); i++) {
			for (j = 0; j < nNumUSegs; j++) {
				refID = isave->GetRefID((void*)tabStrips[i]->tabSegs[j]);
				isave->Write(&refID, sizeof DWORD, &nb);
			}
		}
		isave->EndChunk();
		break;
	}
	case DEFORMER_MESH: {
		// The muscle handle
		isave->BeginChunk(MUSCLEPATCH_NODE);
		refID = isave->GetRefID((void*)node);
		isave->Write(&refID, sizeof DWORD, &nb);
		isave->EndChunk();

		isave->BeginChunk(MUSCLEPATCH_NUMVSEGS_CHUNK);
		isave->Write(&nNumVSegs, sizeof(int), &nb);
		isave->EndChunk();

		isave->BeginChunk(MUSCLEPATCH_NUMUSEGS_CHUNK);
		isave->Write(&nNumUSegs, sizeof(int), &nb);
		isave->EndChunk();
		break;
	}
	}
	return IO_OK;
}

void Muscle::OnRestoreDataChanged(float /*val*/)
{
	//	UpdateMuscle();
	SetHandleSize(GetHandleSize());
	RefreshUI();
}

void Muscle::OnRestoreDataChanged(int /*val*/)
{
	UpdateMuscle();
	RefreshUI();
}

void Muscle::OnRestoreDataChanged(Axis /*val*/)
{
	UpdateMuscle();
	RefreshUI();
}

void Muscle::OnRestoreDataChanged(Color /*val*/)
{
	UpdateColours();
	RefreshUI();
}

void Muscle::OnRestoreDataChanged(Tab<Matrix3> /*val*/)
{
	ClearAFlag(A_HELD);
	UpdateMuscle();
}

