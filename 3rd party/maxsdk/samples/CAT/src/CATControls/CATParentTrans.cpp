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

// Transform for the CATParent. Doesnt actually do anything, its sole purpose in life is to display UI.

#include "CatPlugins.h"

#include "iparamm2.h"
#include <SingleWeakRefMaker.h>

#include <map>
#include <vector>
#include <direct.h>

#include "CATParentTrans.h"
#include "Callback.h"
#include "../CATObjects/CATParent.h"
#include "../DataRestoreObj.h"

// Layers
#include "CATClipRoot.h"
#include "RootNodeController.h"
#include "LayerTransform.h"

// CATMotion
#include "CATHierarchyRoot.h"
#include "CATMotionLayer.h"
#include "ease.h"
// CATMotion
#include "CATHierarchyBranch2.h"

//Rig Structure
#include "Hub.h"
#include "LimbData2.h"
#include "PalmTrans2.h"
#include "DigitData.h"
#include "BoneData.h"
#include "SpineData2.h"

// HIK Export
#include <CATAPI/HIKDefinition.h>
#include <CATAPI/ILimb.h>

// Reserve two bits in the flags to hold four colour modes
#define CATFLAG_COLOURMODE_SHIFT	18								// 0x000c0000 = Colour mode
#define CATFLAG_COLOURMODE_MASK		(3<<CATFLAG_COLOURMODE_SHIFT)

#include "CATCharacterRemap.h"

//////////////////////////////////////////////////////////////////////////
// Utility functions
void KeyRotation(CATClipValue *layerctrl, TimeValue prevkey, TimeValue newt);
void FindMidKeys(Tab < TimeValue > &keytimes, float ratio);
void FindNextKeyForThisController(Tab < TimeValue > keytimes, int &keyid, TimeValue &nextkeyt, TimeValue &t);
void FindNextKeyTime(Tab< Tab < TimeValue > > poskeytimes, Tab< Tab < TimeValue > > rotkeytimes, Tab< Tab < TimeValue > > sclkeytimes, Tab< Tab < TimeValue > > keytimes, \
	Tab<int> &poskeyid, Tab<int> &rotkeyid, Tab<int> &sclkeyid, Tab<int> &keyid, \
	TimeValue start_t, TimeValue end_t, TimeValue &t);
CATNodeControl* FindCATNodeControl(CATClipValue* pCATClipValue);

RefResult TransferAlmostAllReferences(ICATParentTrans* pOldParent, CATCharacterRemap& remap);

// Because there is not a hard connection between the CATParent and child nodes
// it is possible for the rig nodes to be deleted after the CATNodeControl.
// If this happens, we have rig elements in the scene that contain dead (and potentially
// rather dangerous) pointers.  We cannot use find these nodes using normal means because
// the links may be broken before this.
// This function is a fool-proof cleaner - iterate all CATNodeControl entities
// and remove their CATParentTrans pointer.  Called in the CATParentTrans destructor/
void CleanCATParentTransPointer(CATParentTrans* pDeadCATParent);

//////////////////////////////////////////////////////////////////////////

//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class CATParentTransClassDesc : public ICATParentTransClassDesc
{
public:

	CATParentTransClassDesc()
	{
		AddInterface(GetCATClipRootDesc()->GetInterface(LAYERROOT_INTERFACE_FP));
		AddInterface(ICATParentFP::GetFnPubDesc());
		AddInterface(ExtraRigNodes::GetFnPubDesc());
	}

	int 			IsPublic() { return FALSE; }						// Show this in create branch?

	void* Create(BOOL loading = FALSE) {
		CATParentTrans *cpt = new CATParentTrans(loading);
		return cpt;
	}
	void* CreateNew(ECATParent *catparent, INode* node) {
		CATParentTrans *cpt = new CATParentTrans(catparent, node);
		return cpt;
	}

	const TCHAR *	ClassName() { return GetString(IDS_CL_CATPARENTTRANS); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return CATPARENTTRANS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CAT CATParentTrans"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static CATParentTransClassDesc CATParentTransDesc;
ICATParentTransClassDesc* GetCATParentTransDesc() { return &CATParentTransDesc; }

static void PostFileOpenCallback(void *param, NotifyInfo *)
{
	CATParentTrans *catparenttrans = (CATParentTrans*)param;
	ECATParent* catparent = catparenttrans->GetCATParent();

	// we may have been deleted and are on the undo queue
	if (!catparent || catparent->TestAFlag(A_IS_DELETED) || !DependentIterator(catparent).Next())
	{
		return;
	}

	if (catparent->GetReloadRigOnSceneLoad()) {
		// Now we only want to reload the rig if the modified date is different to the one we have saved
		TSTR file = catparent->GetRigFilename();
		TSTR time;
		GetFileModifedTime(file, time);
		if (time != catparent->GetRigFileModifiedTime()) {
			catparenttrans->LoadRig(file);
		}
	}
}

static void ExportStartCallback(void *param, NotifyInfo *)
{
	CATParentTrans *catparenttrans = (CATParentTrans*)param;
	if (!catparenttrans->GetCATLayerRoot())
		return;

	theHold.Suspend();
	// Save the pre-export selected layer.
	catparenttrans->pre_export_selected_layer = catparenttrans->GetCATLayerRoot()->GetSelectedLayer();
	// this stops our layers from returning pos and rot controllers which forces most
	// exporters to re-sample the animation based on actual movement. (FBX)
	catparenttrans->GetCATLayerRoot()->SelectNone();
	theHold.Resume();
}

static void ExportEndCallback(void *param, NotifyInfo *)
{
	CATParentTrans *catparenttrans = (CATParentTrans*)param;
	if (!catparenttrans->GetCATLayerRoot())
		return;

	theHold.Suspend();
	// Now exporting is finished, reselect layer
	catparenttrans->GetCATLayerRoot()->SelectLayer(catparenttrans->pre_export_selected_layer);
	theHold.Resume();
}

static void PreFileResetCallback(void *param, NotifyInfo *)
{
	CATParentTrans *catparenttrans = (CATParentTrans*)param;

	// after a File>Reset, the cached node, if any, is certainly invalid
	if (catparenttrans) {
		catparenttrans->SetNode(NULL);
	}
}

/*************************
 * LoadClip dialogue
 */
class LoadClipDlg : public MaxHeapOperators
{
private:
	ISpinnerControl *spnTimeStart;
	ReferenceTarget* loadingref;
	CATClipRoot *layerroot;
	HWND hWnd;
	BOOL bIsClip;
	ULONG CATControlInterfaceID;

	const TCHAR *pStr;

public:

	LoadClipDlg() : hWnd(NULL)
		, pStr(NULL)
		, bIsClip(0)
		, loadingref(NULL)
	{
		spnTimeStart = NULL;
		layerroot = NULL;
		CATControlInterfaceID = 0;
	}

	void InitControls()
	{
		spnTimeStart = SetupIntSpinner(hWnd, IDC_SPN_START_T, IDC_EDIT_START_T, -1000000, 1000000, GetCOREInterface()->GetAnimRange().Start() / GetTicksPerFrame());

		SET_CHECKED(hWnd, IDC_CHK_CLIP_SCALE_DATA, TRUE);
		SET_CHECKED(hWnd, IDC_RDO_CHAR_SPACE, TRUE);

		// Show/Hide the re-assign controllers dlg box
		if (!bIsClip)
		{
			ShowWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_REASSIGNCOTROLLERS), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_STATIC_RANGE), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_EDIT_START_T), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_SPN_START_T), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_NEWLAYER), SW_HIDE);
		}
		else
		{
			ShowWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_REASSIGNCOTROLLERS), SW_SHOW);
			ShowWindow(GetDlgItem(hWnd, IDC_STATIC_RANGE), SW_SHOW);
			ShowWindow(GetDlgItem(hWnd, IDC_EDIT_START_T), SW_SHOW);
			ShowWindow(GetDlgItem(hWnd, IDC_SPN_START_T), SW_SHOW);

			// If the user has the CATMotion or available layer selected,
			// then me MUST create a new layer to load the clip into
		//	if(layerroot->GetSelectedLayer() == 0  || layerroot->GetSelectedLayer() == -1){
		//		SendMessage(GetDlgItem(hWnd, IDC_CHK_CLIP_NEWLAYER), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		//		EnableWindow(GetDlgItem(hWnd,IDC_CHK_CLIP_NEWLAYER), FALSE);
		//	}
		}

		Class_ID loadingClid = loadingref->ClassID();
		if (loadingClid == HUB_CLASS_ID ||
			loadingClid == LIMBDATA2_CLASS_ID ||
			loadingClid == PALMTRANS2_CLASS_ID ||
			loadingClid == TAILDATA2_CLASS_ID)
		{
			ShowWindow(GetDlgItem(hWnd, IDC_RDO_CHAR_SPACE), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_RDO_WORLD_SPACE_X), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_RDO_WORLD_SPACE_Y), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_SCALE_DATA), SW_HIDE);
		}

		if (loadingClid == LIMBDATA2_CLASS_ID ||
			loadingClid == PALMTRANS2_CLASS_ID) {
			ShowWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_MIRROR), SW_HIDE);
		}
	}

	void SetLoadinfRef(ReferenceTarget *loadingref, CATClipRoot *layerroot, BOOL bIsClip, const TCHAR *pStr, BOOL bIsLimb = FALSE)
	{
		UNREFERENCED_PARAMETER(bIsClip); UNREFERENCED_PARAMETER(bIsLimb);
		this->loadingref = loadingref;
		this->layerroot = layerroot;
		this->bIsClip = bIsClip;
		this->pStr = pStr;
	}

	void ReleaseControls()
	{
		SAFE_RELEASE_SPIN(spnTimeStart);
	}

	//	void SetClipRoot(CATClipRoot *layerroot) { this->layerroot=layerroot; }

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM)
	{
		static HWND hwndOldMouseCapture;
		switch (uMsg) {
		case WM_INITDIALOG:

			this->hWnd = hWnd;

			// Take away any mouse capture that was active.
			hwndOldMouseCapture = GetCapture();
			SetCapture(NULL);

			// Centre the dialog in its parent window and set the
			// focus to the OK button, if it exists.
			CenterWindow(hWnd, GetParent(hWnd));
			InitControls();
			if (GetDlgItem(hWnd, IDC_SPN_START_T)) SetFocus(GetDlgItem(hWnd, IDC_SPN_START_T));
			return FALSE;

		case WM_DESTROY:
			ReleaseControls();

			// Restore the mouse capture.
			SetCapture(hwndOldMouseCapture);
			return TRUE;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_CLIP_MIRROR: {
					BOOL on = IS_CHECKED(hWnd, IDC_CHK_CLIP_MIRROR);
					EnableWindow(GetDlgItem(hWnd, IDC_RDO_CHAR_SPACE), on);
					EnableWindow(GetDlgItem(hWnd, IDC_RDO_WORLD_SPACE_X), on);
					EnableWindow(GetDlgItem(hWnd, IDC_RDO_WORLD_SPACE_Y), on);
					// Default to Char Space
					SET_CHECKED(hWnd, IDC_RDO_CHAR_SPACE, TRUE);
				}
				case IDC_RDO_CHAR_SPACE:
					SET_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_X, FALSE);
					SET_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_Y, FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS), TRUE);
					break;

				case IDC_RDO_WORLD_SPACE_X:
					SET_CHECKED(hWnd, IDC_RDO_CHAR_SPACE, FALSE);
					SET_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_Y, FALSE);
					// Turn of Apply Transforms
					EnableWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS), FALSE);
					SET_CHECKED(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS, FALSE);
					break;

				case IDC_RDO_WORLD_SPACE_Y:
					SET_CHECKED(hWnd, IDC_RDO_CHAR_SPACE, FALSE);
					SET_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_X, FALSE);
					// Turn of Apply Transforms
					EnableWindow(GetDlgItem(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS), FALSE);
					SET_CHECKED(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS, FALSE);
					break;
				case IDC_BTN_CANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					return FALSE;
				case IDC_BTN_LOAD:
					if (loadingref)
					{
						BOOL success = TRUE;
						if (bIsClip) {
							DbgAssert(loadingref->GetInterface(I_CATPARENTTRANS));
							((ICATParentTrans*)loadingref->GetInterface(I_CATPARENTTRANS))->GetCATLayerRoot()->LoadClip(
								TSTR(pStr),
								(TimeValue)spnTimeStart->GetFVal()*GetTicksPerFrame(),
								IS_CHECKED(hWnd, IDC_CHK_CLIP_SCALE_DATA),
								IS_CHECKED(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS),
								IS_CHECKED(hWnd, IDC_CHK_CLIP_MIRROR),
								IS_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_X),
								IS_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_Y)
							);
						}
						else
						{
							DbgAssert(loadingref->GetInterface(I_CATPARENTTRANS));
							((ICATParentTrans*)loadingref)->GetCATLayerRoot()->LoadPose(
								TSTR(pStr),
								GetCOREInterface()->GetTime(),
								IS_CHECKED(hWnd, IDC_CHK_CLIP_SCALE_DATA),
								IS_CHECKED(hWnd, IDC_CHK_CLIP_APPLYTRANSFORMS),
								IS_CHECKED(hWnd, IDC_CHK_CLIP_MIRROR),
								IS_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_X),
								IS_CHECKED(hWnd, IDC_RDO_WORLD_SPACE_Y)
							);
						}

						if (success) layerroot->RefreshLayerRollout();
						EndDialog(hWnd, LOWORD(wParam));
						return FALSE;
					}
				}
				break;
			}
			break;
		}
		return FALSE;
	}
};

static LoadClipDlg LoadClipDlgClass;

static INT_PTR CALLBACK LoadClipDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return LoadClipDlgClass.DlgProc(hWnd, message, wParam, lParam);
};

BOOL MakeLoadClipPoseDlg(ReferenceTarget* loadingref, CATClipRoot* cliproot, BOOL bIsClip, const TCHAR *pStr, BOOL bIsLimb) {
	LoadClipDlgClass.SetLoadinfRef(loadingref, cliproot, bIsClip, pStr, bIsLimb);
	return (BOOL)DialogBox(hInstance, MAKEINTRESOURCE(IDD_LOAD_CLIP_OPTIONS_MODAL), GetCOREInterface()->GetMAXHWnd(), LoadClipDlgProc);
}

/************************************************************************/
/* End LoadClipRollout stuff                                            */
/************************************************************************/
class SaveClipRollout : public MaxHeapOperators
{
private:
	HWND hWnd;
	ReferenceTarget* savingref;
	CATClipRoot *layerroot;
	const TCHAR* filename;

	ISpinnerControl *spnTimeRangeStart;
	ISpinnerControl *spnTimeRangeEnd;

	HWND hComboLayerRangeStart;
	HWND hComboLayerRangeEnd;

public:
	void InitControls()
	{
		Interval ivRange = GetCOREInterface()->GetAnimRange();
		spnTimeRangeStart = SetupIntSpinner(hWnd, IDC_SPN_CLIP_START, IDC_EDT_CLIP_START, -1000000, 1000000, ivRange.Start() / GetTicksPerFrame());
		spnTimeRangeStart->Disable();
		spnTimeRangeEnd = SetupIntSpinner(hWnd, IDC_SPN_CLIP_END, IDC_EDT_CLIP_END, -1000000, 1000000, ivRange.End() / GetTicksPerFrame());
		spnTimeRangeEnd->Disable();

		SET_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_TIME_RANGE, TRUE);

		// Set up the layer range combo boxes.  Each is filled with
		// a list of all layer names (in order of course), and is
		// initialised with the current layer selected, or if no
		// layer is selected, the full layer range.  We do not
		// include the CATMotion layer in the list box.
		hComboLayerRangeStart = GetDlgItem(hWnd, IDC_COMBO_LAYERRANGE_START);
		hComboLayerRangeEnd = GetDlgItem(hWnd, IDC_COMBO_LAYERRANGE_END);

		for (int i = 0; i < layerroot->NumLayers(); i++) {
			SendMessage(hComboLayerRangeStart, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)layerroot->GetLayer(i)->GetName().data());
			SendMessage(hComboLayerRangeEnd, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)layerroot->GetLayer(i)->GetName().data());
		}

		int nActiveLayer = layerroot->GetSelectedLayer();
		SendMessage(hComboLayerRangeStart, CB_SETCURSEL, (nActiveLayer >= 0 ? nActiveLayer : 0), 0);
		SendMessage(hComboLayerRangeEnd, CB_SETCURSEL, (nActiveLayer >= 0 ? nActiveLayer : layerroot->NumLayers() - 2), 0);

		if (savingref->ClassID() == CATPARENTTRANS_CLASS_ID) {
			SET_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_LAYER_STACK, TRUE);
			EnableWindow(hComboLayerRangeStart, FALSE);
			EnableWindow(hComboLayerRangeEnd, FALSE);
		}
		else {
			EnableWindow(hComboLayerRangeStart, FALSE);
			EnableWindow(hComboLayerRangeEnd, FALSE);

			EnableWindow(GetDlgItem(hWnd, IDC_CHK_SAVE_ENTIRE_LAYER_STACK), FALSE);
			EnableWindow(hComboLayerRangeStart, FALSE);
			EnableWindow(hComboLayerRangeEnd, FALSE);
		}
	}

	void ReleaseControls()
	{
		SAFE_RELEASE_SPIN(spnTimeRangeStart);
		SAFE_RELEASE_SPIN(spnTimeRangeEnd);
	}

	void SetSavingRef(ReferenceTarget *savingref, CATClipRoot *layerroot, const TCHAR* filename)
	{
		this->savingref = savingref;
		this->layerroot = layerroot;
		this->filename = filename;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		static HWND hwndOldMouseCapture;
		switch (uMsg)
		{
		case WM_INITDIALOG:
			this->hWnd = hWnd;
			// Take away any mouse capture that was active.
			hwndOldMouseCapture = GetCapture();
			SetCapture(NULL);

			// Centre the dialog in its parent window and set the
			// focus to the OK button, if it exists.
			CenterWindow(hWnd, GetParent(hWnd));
			InitControls();
			if (GetDlgItem(hWnd, IDC_SPN_START_T)) SetFocus(GetDlgItem(hWnd, IDC_SPN_START_T));
			return FALSE;

		case WM_DESTROY:
			ReleaseControls();
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return FALSE;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam))
				{
				case IDC_CHK_SAVE_ENTIRE_TIME_RANGE:
					spnTimeRangeStart->Enable(!IS_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_TIME_RANGE));
					spnTimeRangeEnd->Enable(!IS_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_TIME_RANGE));
					break;
				case IDC_CHK_SAVE_ENTIRE_LAYER_STACK:
					EnableWindow(hComboLayerRangeStart, !IS_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_LAYER_STACK));
					EnableWindow(hComboLayerRangeEnd, !IS_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_LAYER_STACK));
					break;
				case IDC_CLP_OK:
				{
					//	int flags = CLIPFLAG_CLIP;
					int tpf = GetTicksPerFrame();
					Interval timerange, layerrange;
					//		CATClipRoot *layerroot = catparent->GetClipRootInterface();

					if (IS_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_TIME_RANGE))
						timerange = FOREVER;
					else {
						timerange.SetStart(spnTimeRangeStart->GetIVal() * tpf);
						timerange.SetEnd(spnTimeRangeEnd->GetIVal() * tpf);
					}

					if (IS_CHECKED(hWnd, IDC_CHK_SAVE_ENTIRE_LAYER_STACK)) {
						layerrange.SetStart(0);
						layerrange.SetEnd(layerroot->NumLayers());
					}
					else {
						// I have multiple layers being saved out
						int start = (int)SendMessage(hComboLayerRangeStart, CB_GETCURSEL, 0, 0);
						int end = (int)SendMessage(hComboLayerRangeEnd, CB_GETCURSEL, 0, 0);
						layerrange.Set(start, end);
					}

					// this will all be replaced with a nice interface for CATV2
					// TODO: the above
					if (savingref->ClassID() == CATPARENTTRANS_CLASS_ID)
						((ICATParentTrans*)savingref)->GetCATLayerRoot()->SaveClip(TSTR(filename), timerange.Start(), timerange.End(), layerrange.Start(), layerrange.End());
					else if (savingref->GetInterface(I_CATCONTROL))
						((CATControl*)savingref->GetInterface(I_CATCONTROL))->SaveClip(TSTR(filename), timerange.Start(), timerange.End());
					EndDialog(hWnd, LOWORD(wParam));
				}
				break;

				case IDC_CLP_CANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					return TRUE;

				case IDC_COMBO_LAYERRANGE_START:
				case IDC_COMBO_LAYERRANGE_END:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						// If one of our layer ranges have changed, make sure we maintain
						// a valid range, adjusting the other range if necessary.
						int nStartID = (int)SendMessage(hComboLayerRangeStart, CB_GETCURSEL, 0, 0);
						int nEndID = (int)SendMessage(hComboLayerRangeEnd, CB_GETCURSEL, 0, 0);

						if (savingref->ClassID() == CATPARENTTRANS_CLASS_ID) {
							if (lParam == (LPARAM)hComboLayerRangeStart && nStartID > nEndID)
								SendMessage(hComboLayerRangeEnd, CB_SETCURSEL, nStartID, 0);

							if (lParam == (LPARAM)hComboLayerRangeEnd && nStartID > nEndID)
								SendMessage(hComboLayerRangeStart, CB_SETCURSEL, nEndID, 0);
						}
						else {
							// We can only save out 1 layer at a time on the limb and tail clip savers
							if (lParam == (LPARAM)hComboLayerRangeStart)
								SendMessage(hComboLayerRangeEnd, CB_SETCURSEL, nStartID, 0);

							if (lParam == (LPARAM)hComboLayerRangeEnd)
								SendMessage(hComboLayerRangeStart, CB_SETCURSEL, nEndID, 0);
						}
					}
					break;
				}
				break;
			}
			break;

		case CC_SPINNER_CHANGE: {
			switch (LOWORD(wParam)) {
			case IDC_SPN_CLIP_START:
			case IDC_SPN_CLIP_END:
				if (spnTimeRangeStart->GetIVal() > spnTimeRangeEnd->GetIVal())
					spnTimeRangeEnd->SetValue(spnTimeRangeStart->GetIVal() + 1, FALSE);
				break;
			}
		}
		default:
			return FALSE;
		}
		return TRUE;
	}
};

static SaveClipRollout SaveClipRolloutClass;

static INT_PTR CALLBACK SaveClipDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return SaveClipRolloutClass.DlgProc(hWnd, message, wParam, lParam);
};

BOOL MakeClipPoseDlg(ReferenceTarget* savingref, CATClipRoot* cliproot, const TCHAR* filename) {
	SaveClipRolloutClass.SetSavingRef(savingref, cliproot, filename);
	return (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SAVESTUFF_MODAL), GetCOREInterface()->GetMAXHWnd(), SaveClipDlgProc) != 0);
}

/************************************************************************/
/*  Clip and Pose Rollout                                                */
/************************************************************************/
class CATParentTransParamDlgCallBack : public ParamMap2UserDlgProc, public CATActiveLayerChangeCallback
{
	HWND hWnd;

	CATParentTrans* catparenttrans;
	CATClipRoot *layerroot;

	ICustButton *btnSetClip;
	ICustButton *btnSetPose;
	ICustButton *btnSave;
	ICustButton *btnBrowse;
	ICustButton *btnDelete;

	BOOL bIsClip;

	HWND lbxClips;

	std::vector<TCHAR*> vecFileNames;
	std::vector<TCHAR*> vecNames;
	std::vector<TCHAR*> vecFolderNames;

	TCHAR strClipsFolder[MAX_PATH];
	TCHAR strPosesFolder[MAX_PATH];

	bool bShowFolders;

	void ClearFileList() {
		unsigned int i;
		SendMessage(lbxClips, LB_RESETCONTENT, 0, 0);
		for (i = 0; i < vecFolderNames.size(); i++) delete[] vecFolderNames[i];
		for (i = 0; i < vecFileNames.size(); i++) {
			delete[] vecFileNames[i];
			delete[] vecNames[i];
		}
		vecFolderNames.resize(0);
		vecFileNames.resize(0);
		vecNames.resize(0);
	}

	BOOL ChangeDirectory(const TCHAR *szNewDir) {
		TCHAR szWorkingDir[MAX_PATH];
		_tgetcwd(szWorkingDir, MAX_PATH);

		if (_tcscmp(szNewDir, _T("")) == 0) {
			_tcscpy(bIsClip ? strClipsFolder : strPosesFolder, _T(""));
			return TRUE;
		}
		else if (_tchdir(szNewDir) != -1) {
			// Set the current folder to be the directory we just changed to
			// (so we get its full, correct name).
			_tgetcwd(bIsClip ? strClipsFolder : strPosesFolder, MAX_PATH);
			_tchdir(szWorkingDir);
			return TRUE;
		}

		return FALSE;
	}

public:

	CATParentTransParamDlgCallBack() : bIsClip(0)
	{
		hWnd = NULL;
		lbxClips = NULL;

		catparenttrans = NULL;
		layerroot = NULL;

		btnSave = NULL;

		btnSetClip = NULL;
		btnSetPose = NULL;
		btnBrowse = NULL;
		btnDelete = NULL;

		bShowFolders = true;

	}

	// Load up all existing clip/pose names into the ListBox
	void RefreshListBox()
	{
		WIN32_FIND_DATA find;
		HANDLE hFind;

		TCHAR searchName[MAX_PATH] = { 0 };
		TCHAR fileName[MAX_PATH] = { 0 };
		TCHAR *strCurrentFolder = bIsClip ? strClipsFolder : strPosesFolder;

		int id;

		ClearFileList();

		// Search for all subdirectories in the current directory and
		// insert them into the list box (unless instructed not to),
		// while also maintaining them in a vector.  The listbox stores
		// a negative value in its ITEMDATA field to specify that the
		// entry is a folder, not a rig preset.  To convert to an index
		// into the folders vector, use abs(ITEMDATA)-1.
		if (bShowFolders) {
			if (!_tcscmp(strCurrentFolder, _T(""))) {
				int nOldDrive;
				nOldDrive = _getdrive();

				for (int nDrive = 1; nDrive <= 26; nDrive++) {
					if (nDrive <= 2 || !_chdrive(nDrive)) {
						TCHAR driveName[5] = { 0 };
						TCHAR *drive = new TCHAR[4];
						DbgAssert(drive);
						_stprintf(drive, _T("%c:\\"), nDrive + 'A' - 1);
						_stprintf(driveName, _T("<%c:>"), nDrive + 'A' - 1);

						id = (int)SendMessage(lbxClips, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)driveName);
						SendMessage(lbxClips, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
						vecFolderNames.push_back(drive);
					}
				}

				_chdrive(nOldDrive);
			}
			else {
				// Find the parent directory.  If we are in the root directory
				// for the particular drive, store our parent directory as an
				// empty string.  When the user chooses to navigate to that, we
				// display all the drives instead of directories.
				TCHAR szWorkingDir[MAX_PATH];
				TCHAR szParentDir[MAX_PATH];
				_tgetcwd(szWorkingDir, MAX_PATH);

				_tchdir(strCurrentFolder);
				_tgetcwd(strCurrentFolder, MAX_PATH);
				_tchdir(_T(".."));
				_tgetcwd(szParentDir, MAX_PATH);

				_tchdir(szWorkingDir);

				if (0 == _tcscmp(strCurrentFolder, szParentDir)) {
					// We are in the root directory of the drive.  The parent
					// folder is an immitation of "My Computer".
					_tcscpy(szParentDir, _T(""));
				}

				// Insert the '..' directory.
				TCHAR *folder = new TCHAR[_tcslen(szParentDir) + 4];
				DbgAssert(folder);

				_tcscpy(folder, szParentDir);
				id = (int)SendMessage(lbxClips, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)_T("<..>"));
				SendMessage(lbxClips, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
				vecFolderNames.push_back(folder);

				// Insert the subdirectories
				_stprintf(searchName, _T("%s\\*.*"), strCurrentFolder);
				hFind = FindFirstFile(searchName, &find);
				if (hFind == INVALID_HANDLE_VALUE) return;

				do {
					if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (_tcscmp(find.cFileName, _T(".")) != 0 && _tcscmp(find.cFileName, _T("..")) != 0) {
							// Create the display name for the directory.
							_stprintf(fileName, _T("<%s>"), find.cFileName);

							// We need to save the full path, so allocate some
							// memory.  If this fails, don't even bother adding
							// it to the list box.
							TCHAR *folder = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];

							if (folder) {
								_stprintf(folder, _T("%s\\%s"), strCurrentFolder, find.cFileName);
								id = (int)SendMessage(lbxClips, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)fileName);
								SendMessage(lbxClips, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
								vecFolderNames.push_back(folder);
							}
						}
					}
				} while (FindNextFile(hFind, &find));

				FindClose(hFind);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Create a search string for clips
		_sntprintf(searchName, MAX_PATH, _T("%s\\*.%s"), strCurrentFolder, (bIsClip ? CAT_CLIP_EXT : CAT_POSE_EXT));
		hFind = FindFirstFile(searchName, &find);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (find.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
					// Copy the filename and remove the extension.
					_tcscpy(fileName, find.cFileName);

					// We need to save the full path, so allocate some
					// memory.  If this fails, don't even bother adding
					// it to the list box.
					TCHAR *file = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];
					TCHAR *clip = new TCHAR[_tcslen(fileName) + 4];

					if (file && clip) {
						_stprintf(file, _T("%s\\%s"), strCurrentFolder, find.cFileName);
						_tcscpy(clip, fileName);
						id = (int)SendMessage(lbxClips, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)clip);
						SendMessage(lbxClips, LB_SETITEMDATA, id, (LPARAM)vecNames.size());
						vecFileNames.push_back(file);
						vecNames.push_back(clip);
					}
					else {
						// One of these must have failed, for some stupid reason.
						// May as well clean up even though there's no point, cos
						// the system will have died horribly before now.
						delete[] file;
						delete[] clip;
					}
				}
			} while (FindNextFile(hFind, &find));
		}

		// Mocap import is disabled for Max5 because the Capture animaiton Script iuses UI controls
		// not available pre Max6
#ifndef VISUALSTUDIO6
		if (bIsClip) {
			for (int filetype = 0; filetype < 4; filetype++) {
				//////////////////////////////////////////////////////////////////////////
				// Create a search string for HTR mocapfiles
				if (filetype == 0)
					_sntprintf(searchName, MAX_PATH, _T("%s\\*.%s"), strCurrentFolder, CAT_HTR_EXT);
				else if (filetype == 1)
					_sntprintf(searchName, MAX_PATH, _T("%s\\*.%s"), strCurrentFolder, CAT_BVH_EXT);
				else if (filetype == 2)
					_sntprintf(searchName, MAX_PATH, _T("%s\\*.%s"), strCurrentFolder, CAT_BIP_EXT);
				else if (filetype == 3)
					_sntprintf(searchName, MAX_PATH, _T("%s\\*.%s"), strCurrentFolder, CAT_FBX_EXT);

				hFind = FindFirstFile(searchName, &find);
				if (hFind != INVALID_HANDLE_VALUE) {
					do {
						if (find.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
							// Copy the filename and remove the extension.
							_tcscpy(fileName, find.cFileName);

							// We need to save the full path, so allocate some
							// memory.  If this fails, don't even bother adding
							// it to the list box.
							TCHAR *file = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];
							TCHAR *clip = new TCHAR[_tcslen(fileName) + 4];

							if (file && clip) {
								_stprintf(file, _T("%s\\%s"), strCurrentFolder, find.cFileName);
								_tcscpy(clip, fileName);
								id = (int)SendMessage(lbxClips, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)clip);
								SendMessage(lbxClips, LB_SETITEMDATA, id, (LPARAM)vecNames.size());
								vecFileNames.push_back(file);
								vecNames.push_back(clip);
							}
							else {
								// One of these must have failed, for some stupid reason.
								// May as well clean up even though there's no point, cos
								// the system will have died horribly before now.
								delete[] file;
								delete[] clip;
							}
						}
					} while (FindNextFile(hFind, &find));
				}
			}
		}
#endif;
		FindClose(hFind);
	}

	void InitControls(HWND hDlg, IParamMap2 *map)
	{
		hWnd = hDlg;

		catparenttrans = (CATParentTrans*)map->GetParamBlock()->GetOwner();
		layerroot = catparenttrans->GetCATLayerRoot();

		CatDotIni catCfg;

		// Initialise the clip folder
		bIsClip = false;
		_tcscpy(strPosesFolder, _T(""));
		if (!ChangeDirectory(catCfg.Get(INI_POSES_PATH)))
			ChangeDirectory(GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));

		// Initialise the clips folder
		bIsClip = true;
		_tcscpy(strClipsFolder, _T(""));
		if (!ChangeDirectory(catCfg.Get(INI_CLIPS_PATH)))
			ChangeDirectory(GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));

		// Initialise the switch buttons
		btnSetClip = GetICustButton(GetDlgItem(hDlg, IDC_BTN_SET_CLIP));
		btnSetClip->SetType(CBT_CHECK);
		btnSetClip->SetTooltip(TRUE, GetString(IDS_TT_SETCLIPS));
		btnSetClip->SetCheck(bIsClip);

		btnSetPose = GetICustButton(GetDlgItem(hDlg, IDC_BTN_SET_POSE));
		btnSetPose->SetType(CBT_CHECK);
		btnSetPose->SetTooltip(TRUE, GetString(IDS_TT_SETPOSES));
		btnSetPose->SetCheck(!bIsClip);

		// Initialise the Save Button
		btnSave = GetICustButton(GetDlgItem(hDlg, IDC_BTN_SAVE));
		btnSave->SetType(CBT_PUSH);
		btnSave->SetButtonDownNotify(TRUE);
		btnSave->SetTooltip(TRUE, bIsClip ? GetString(IDS_TT_SAVECLIP) : GetString(IDS_TT_SAVEPOSE));
		btnSave->SetImage(hIcons, 6, 6, 6 + 25, 6 + 25, 24, 24);

		// Initialise the Browse Button
		btnBrowse = GetICustButton(GetDlgItem(hDlg, IDC_BTN_BROWSE_FILE));
		btnBrowse->SetType(CBT_PUSH);
		btnBrowse->SetButtonDownNotify(TRUE);
		btnBrowse->SetTooltip(TRUE, bIsClip ? GetString(IDS_TT_BROWSECM) : GetString(IDS_TT_BROWSEPOSE));
		btnBrowse->SetImage(hIcons, 8, 8, 8 + 25, 8 + 25, 24, 24);

		// Initialise the Delete Button
		btnDelete = GetICustButton(GetDlgItem(hDlg, IDC_BTN_DELETE));
		btnDelete->SetType(CBT_PUSH);
		btnDelete->SetButtonDownNotify(TRUE);
		btnDelete->SetTooltip(TRUE, GetString(IDS_TT_DELFILE));
		btnDelete->SetImage(hIcons, 7, 7, 7 + 25, 7 + 25, 24, 24);

		lbxClips = GetDlgItem(hDlg, IDC_FILE_LIST);

		RefreshListBox();
		RefreshButtonStates();
	}

	void ReleaseControls()
	{
		SAFE_RELEASE_BTN(btnSetClip);
		SAFE_RELEASE_BTN(btnSetPose);
		SAFE_RELEASE_BTN(btnSave);
		SAFE_RELEASE_BTN(btnBrowse);
		SAFE_RELEASE_BTN(btnDelete);

		ClearFileList();
	}

	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, map);
			catparenttrans->GetCATLayerRoot()->RegisterActiveLayerChangeCallback(this);
			break;

		case WM_DESTROY:
			catparenttrans->GetCATLayerRoot()->UnRegisterActiveLayerChangeCallback(this);
			ReleaseControls();
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				if (LOWORD(wParam) == IDC_FILE_LIST)
				{
					int item = (int)SendMessage(lbxClips, LB_GETCURSEL, 0, 0);
					if (item != LB_ERR)
					{
						int nFileID = (int)SendMessage(lbxClips, LB_GETITEMDATA, item, 0);

						if (nFileID < 0) {
							// A negative index means this item is a directory.
							HCURSOR hCurrentCursor = ::GetCursor();
							SetCursor(LoadCursor(NULL, IDC_WAIT));
							BOOL bSuccess = ChangeDirectory(vecFolderNames[-nFileID - 1]);
							SetCursor(hCurrentCursor);

							if (!bSuccess) {
								::MessageBox(
									hWnd,
									GetString(IDS_ERR_NOACCESS),
									GetString(IDS_ERROR),
									MB_OK | MB_ICONSTOP);
							}
							RefreshListBox();
						}
						else if (layerroot && catparenttrans) {
							// extract the extension
							int nExt = (int)_tcslen(vecFileNames[nFileID]);
							while (nExt > 0 && vecFileNames[nFileID][nExt - 1] != _T('.')) nExt--;

							if (!_tcsicmp(&vecFileNames[nFileID][nExt], CAT_BVH_EXT))
								layerroot->LoadBVH(vecFileNames[nFileID]);
							else if (!_tcsicmp(&vecFileNames[nFileID][nExt], CAT_HTR_EXT))
								layerroot->LoadHTR(vecFileNames[nFileID]);
							else if (!_tcsicmp(&vecFileNames[nFileID][nExt], CAT_BIP_EXT))
								layerroot->LoadBIP(vecFileNames[nFileID]);
							else if (!_tcsicmp(&vecFileNames[nFileID][nExt], CAT_FBX_EXT))
								layerroot->LoadFBX(vecFileNames[nFileID]);
							else if (!_tcsicmp(&vecFileNames[nFileID][nExt], CAT_CLIP_EXT))
							{
								MakeLoadClipPoseDlg(catparenttrans, layerroot, TRUE, vecFileNames[nFileID], FALSE);

								//	LoadClipDlgClass.SetLoadingRef(catparent, TRUE, vecFileNames[nFileID]);
								//	INT result = DialogBox(hInstance, MAKEINTRESOURCE(IDD_LOAD_CLIP_OPTIONS_MODAL), GetCOREInterface()->GetMAXHWnd(), LoadClipDlgProc);
							}
							else if (!_tcsicmp(&vecFileNames[nFileID][nExt], CAT_POSE_EXT))
							{
								if (layerroot->GetSelectedLayer() != -1) {
									MakeLoadClipPoseDlg(catparenttrans, layerroot, FALSE, vecFileNames[nFileID], FALSE);
									//		LoadClipDlgClass.SetSavingRef(catparent, bIsClip, vecFileNames[nFileID]);
									//		INT result = DialogBox(hInstance, MAKEINTRESOURCE(IDD_LOAD_CLIP_OPTIONS_MODAL), GetCOREInterface()->GetMAXHWnd(), LoadClipDlgProc);
								}
							}
							RefreshButtonStates();
						}
					}
				}
				break;
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_BTN_SET_CLIP:
				{
					ResetLastFreeformFilterIndex();
					btnSetClip->SetCheck(TRUE);
					btnSetPose->SetCheck(FALSE);
					if (!bIsClip)
					{
						bIsClip = TRUE;

						// Initialise the INI files so we can read button text and tooltips
						CatDotIni catCfg;

						btnBrowse->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnBrowseClipFile"), GetString(IDS_TT_BROWSECM)));
						btnSave->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSaveClipFile"), GetString(IDS_TT_SAVECLIP)));

						ChangeDirectory(strClipsFolder);
						RefreshListBox();
						RefreshButtonStates();
					}
				}
				break;
				case IDC_BTN_SET_POSE:
				{
					ResetLastFreeformFilterIndex();
					btnSetClip->SetCheck(FALSE);
					btnSetPose->SetCheck(TRUE);
					if (bIsClip)
					{
						bIsClip = FALSE;
						// Initialise the INI files so we can read button text and tooltips
						CatDotIni catCfg;

						btnBrowse->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnBrowsePoseFile"), GetString(IDS_TT_BROWSEPOSE)));
						btnSave->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSavePoseFile"), GetString(IDS_TT_SAVEPOSE)));

						ChangeDirectory(strPosesFolder);
						RefreshListBox();
						RefreshButtonStates();
					}
				}
				break;
				case IDC_BTN_SAVE:
				{
					// this idea was so that the system would detect what extension the user had selected, and
					// automaticly choose to display the UI or not. I had problems with it because of the defal
					TCHAR filename[MAX_PATH];
					if (DialogSaveClipOrPose(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH, bIsClip))
					{
						int nExt = (int)_tcslen(filename);
						while (nExt > 0 && filename[nExt - 1] != _T('.')) nExt--;

						if (!_tcsicmp(&filename[nExt], CAT_CLIP_EXT)) {
							MakeClipPoseDlg(catparenttrans, layerroot, filename);
						}
						else if (!_tcsicmp(&filename[nExt], CAT_POSE_EXT))
							if (catparenttrans) catparenttrans->SavePose(TSTR(filename));
					}
					RefreshListBox();
				}
				break;
				case IDC_BTN_DELETE:
				{
					int item = (int)SendMessage(lbxClips, LB_GETCURSEL, 0, 0);
					if (item != LB_ERR) {
						int nFileID = (int)SendMessage(lbxClips, LB_GETITEMDATA, item, 0);
						if (nFileID >= 0 && nFileID < vecFileNames.size())
						{
							DeleteFile(vecFileNames[nFileID]);
							RefreshListBox();
						}
					}
				}
				break;
				case IDC_BTN_BROWSE_FILE:
				{
					TCHAR filename[MAX_PATH];
					if (DialogOpenFreeform(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH, bIsClip))
					{
						// Load the motion data file based on its extension.
						BOOL success = FALSE;
						DbgAssert(catparenttrans);
						int nExt = (int)_tcslen(filename);
						while (nExt > 0 && filename[nExt - 1] != _T('.')) nExt--;

						if (!_tcsicmp(&filename[nExt], CAT_BVH_EXT))
							success = layerroot->LoadBVH(filename);
						else if (!_tcsicmp(&filename[nExt], CAT_HTR_EXT))
							success = layerroot->LoadHTR(filename);
						else if (!_tcsicmp(&filename[nExt], CAT_BIP_EXT))
							success = layerroot->LoadBIP(filename);
						else if (!_tcsicmp(&filename[nExt], CAT_FBX_EXT))
							success = layerroot->LoadFBX(filename);
						else if (!_tcsicmp(&filename[nExt], CAT_CLIP_EXT))
							MakeLoadClipPoseDlg(catparenttrans, layerroot, TRUE, filename, FALSE);
						else if (!_tcsicmp(&filename[nExt], CAT_POSE_EXT))
							MakeLoadClipPoseDlg(catparenttrans, layerroot, FALSE, filename, FALSE);
						RefreshButtonStates();
					}
				}
				break;
				}
			}
		default:
			return FALSE;
		}
		return TRUE;
	}

	void RefreshButtonStates() {
		if (!bIsClip) {
			if (catparenttrans->GetCATMode() == SETUPMODE) {
				EnableWindow(lbxClips, FALSE);
				btnBrowse->Enable(FALSE);
				btnSave->Enable(FALSE);
			}
			else {
				btnSave->Enable(TRUE);
				EnableWindow(lbxClips, layerroot->GetSelectedLayer() >= 0);
				btnBrowse->Enable(layerroot->GetSelectedLayer() >= 0);
			}
		}
		else {
			btnSave->Enable(TRUE);
			EnableWindow(lbxClips, TRUE);
			btnBrowse->Enable(TRUE);
		}
	}

	virtual void ActiveLayerChanged(int nLayer) {
		UNREFERENCED_PARAMETER(nLayer);
		RefreshButtonStates();
	}

	virtual void DeleteThis() { }//delete this; }
};

static CATParentTransParamDlgCallBack CATParentTransParamCallBack;

static ParamBlockDesc2 catparenttrans_param_blk(CATParentTrans::PBLOCK_REF, _T("CATParent params"), 0, &CATParentTransDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, CATParentTrans::PBLOCK_REF,
	IDD_CLIPMANAGER, IDS_CLIPMANAGER, 0, 0, &CATParentTransParamCallBack,
	CATParentTrans::PB_CATUNITS, _T(""), TYPE_WORLD, P_RESET_DEFAULT, 0,
		p_default, 0.1f,
		p_end,
	p_end
);

//
//	CATParentTrans  Implementation.
//
//
//	Steve T. 21 Nov 2002
void CATParentTrans::Init(BOOL loading)
{
	prs = NULL;
	pblock = NULL;
	flagsBegin = 0;
	ipbegin = NULL;

	isResetting = false;

	dwFileSaveVersion = 0;
	layerroot = NULL;
	mRootHub = NULL;
	node = NULL;
	catunits = 0.0f;
	flags = 0;
	catmode = SETUPMODE;
	lengthaxis = X;
	colourmode = COLOURMODE_CLASSIC;
	catname = GetString(IDS_CATNAME);
	rootnode = NULL;

	bRigCreating = FALSE;
	ivalid = NEVER;
	tmCATValid = NEVER;
	p3CATParentScale = P3_IDENTITY_SCALE;
	tmCATParent = Matrix3(1);

	rootnodehdl = -1;

	// GB 27-Jan-2004: To prevent multiple colour updates at one point in time.
	tvLastColourUpdateTime = ivalid.Start();

	CATParentTransDesc.MakeAutoParamBlocks(this);

	if (!loading)
	{
		ReplaceReference(PRS, NewDefaultMatrix3Controller());
	}

	RegisterNotification(ExportStartCallback, this, NOTIFY_PRE_EXPORT);	// PT: Added 14/02/06
	RegisterNotification(ExportEndCallback, this, NOTIFY_POST_EXPORT);	// PT: Added 14/02/06

	RegisterNotification(PostFileOpenCallback, this, NOTIFY_FILE_POST_OPEN);	// PT: Added 02/08/07
	RegisterNotification(PreFileResetCallback, this, NOTIFY_SYSTEM_PRE_RESET);
};

//
//	initialize ourselves by copying data and controls
//	from another instance of our class
//

CATParentTrans::CATParentTrans(BOOL loading)
{
	Init(loading);
}

CATParentTrans::CATParentTrans(ECATParent *catparent, INode *node)
{
	Init(FALSE);

	this->node = node;

	// We don't store the CATParent any more. We pull it off the node if we need it.
	ReplaceReference(LAYERROOT, (CATClipRoot*)GetCATClipRootDesc()->CreateNew(this));

}

//////////////////////////////////////////////////////////////////////////
// ICATParent functions

BOOL CATParentTrans::SaveRig(const MSTR& file)
{
	TSTR path, filename, extension;
	SplitFilename(file, &path, &filename, &extension);

	//	Interface *ip = GetCOREInterface();
	Interface9	*ip = GetCOREInterface9();

	// Remove the '.' from '.rg3'
	extension = extension.Substr(1, extension.Length() - 1);
	if (MatchPattern(extension, TSTR(CAT3_RIG_PRESET_EXT), TRUE)) {
		SetFlag(PARENTFLAG_SAVING_RIG);
		g_bSavingCAT3Rig = true;
		theHold.Suspend();

		Tab <BOOL> transformnodes;
		transformnodes.SetCount(GetCATLayerRoot()->NumLayers());
		for (int i = 0; i < GetCATLayerRoot()->NumLayers(); i++) {
			transformnodes[i] = GetCATLayerRoot()->GetLayer(i)->TransformNodeOn();
			GetCATLayerRoot()->GetLayer(i)->DestroyTransformNode();
		}

		INodeTab nodes;
		AddSystemNodes(nodes, kSNCFileSave);

		CATMessage(0, CAT_PRE_RIG_SAVE);

		// The problem with this idea is that it is not possible to stop the Saver from saving nodes that we dont want.
		// One thing we could do is rip through the rig and unlink any bones from thier parents if they have them.
		// Otherwise, if a character was riding in a car, then the saver would save the car also.
		// During saving we need to stop the saver saving the layers. we could do this in the same way we used to cripple the demo.
		// simply make the layer system not return layers during saving as references during rig saving.
		ip->FileSaveNodes(&nodes, (MCHAR*)file.data());

		for (int i = 0; i < GetCATLayerRoot()->NumLayers(); i++) {
			if (transformnodes[i]) {
				GetCATLayerRoot()->GetLayer(i)->CreateTransformNode();
			}
		}

		CATMessage(0, CAT_POST_RIG_SAVE);

		theHold.Resume();
		g_bSavingCAT3Rig = false;
		ClearFlag(PARENTFLAG_SAVING_RIG);
		GetCATParent()->SetRigFilename(file);
		return TRUE;
	}

	// Create our rig saver and begin...
	CATRigWriter save(file.data(), GetCATParentTrans());

	if (!save.ok()) {
		::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_FILENOCREATE), GetString(IDS_ERR_FILECREATE), MB_OK);
		return FALSE;
	}

	// Insert a comment with a timestamp at the top of the file.
	time_t ltime;
	time(&ltime);
	save.Comment(GetString(IDS_MSG_EXPORT_RIG), _tctime(&ltime));

	// Go bananas!
	SaveRig(&save);

	GetCATParent()->SetRigFilename(file);

	// (stop banana's)
	return TRUE;
}

INode* CATParentTrans::LoadRig(const MSTR& file)
{
	return LoadRig(file, NULL);
}

BOOL CATParentTrans::SaveClip(TSTR filename, TimeValue start_t, TimeValue end_t, int from_layer, int to_layer)
{
	int flags = CLIPFLAG_CLIP;
	Interval timerange(start_t, end_t);
	if (start_t > end_t) timerange = FOREVER;
	Interval layerrange(from_layer, to_layer);
	return SaveClip(filename, flags, timerange, layerrange, NULL);
}

// this is the function that is called by script and the rollout
INode* CATParentTrans::LoadClip(TSTR filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY)
{

	CATClipRoot *layerroot = GetCATLayerRoot();
	int selected = layerroot->GetSelectedLayer();

	int flags = CLIPFLAG_CLIP | \
		CLIPFLAG_LOADPELVIS | \
		CLIPFLAG_LOADFEET | \
		(scale_data ? CLIPFLAG_SCALE_DATA : FALSE) | \
		(transformData ? CLIPFLAG_APPLYTRANSFORMS : FALSE) | \
		((mirrorData || mirrorWorldX || mirrorWorldY) ? CLIPFLAG_MIRROR : FALSE) | \
		(mirrorWorldX ? CLIPFLAG_MIRROR_WORLD_X : FALSE) | \
		(mirrorWorldY ? CLIPFLAG_MIRROR_WORLD_Y : FALSE);

	if (selected < 0 || selected > layerroot->NumLayers())
		flags |= CLIPFLAG_CREATENEWLAYER;

	// this flag is turned off for body parts that are not evaluated in world space
	// at least the pelvis will be in world space
	flags |= CLIPFLAG_WORLDSPACE;

	return LoadClip(filename, selected, flags, t/*GetTicksPerFrame()*/, NULL);
}

BOOL CATParentTrans::SavePose(TSTR filename)
{
	CATClipRoot *layerroot = GetCATLayerRoot();
	int flags = 0;
	Interval clipLength(GetCOREInterface()->GetTime(), GetCOREInterface()->GetTime());
	Interval layerrange(layerroot->GetSelectedLayer(), layerroot->GetSelectedLayer());

	return SaveClip(TSTR(filename), flags, clipLength, layerrange, NULL);
}

// this is the function that is called by script and the rollout
//BOOL CATParent::ILoadPose(TSTR filename, BOOL loadWSBodyParts, BOOL only_weighted, BOOL scale_data, BOOL mirrorData, BOOL transformData)
INode* CATParentTrans::LoadPose(TSTR filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY)
{
	//	TimeValue t = GetCOREInterface()->GetTime();
	CATClipRoot *layerroot = GetCATLayerRoot();
	int selected = layerroot->GetSelectedLayer();
	if (selected < 0 || selected > layerroot->NumLayers()) return FALSE;

	int flags = \
		CLIPFLAG_LOADPELVIS | \
		CLIPFLAG_LOADFEET | \
		(scale_data ? CLIPFLAG_SCALE_DATA : FALSE) | \
		(transformData ? CLIPFLAG_APPLYTRANSFORMS : FALSE) | \
		((mirrorData || mirrorWorldX || mirrorWorldY) ? CLIPFLAG_MIRROR : FALSE) | \
		(mirrorWorldX ? CLIPFLAG_MIRROR_WORLD_X : FALSE) | \
		(mirrorWorldY ? CLIPFLAG_MIRROR_WORLD_Y : FALSE);

	// this flag is turned off for body parts that are not evaluated in world space
	// at least the pelvis will be in world space
	flags |= CLIPFLAG_WORLDSPACE;

	return LoadClip(filename, selected, flags, t/*GetTicksPerFrame()*/, NULL);
}

//
// During keyfreeform, the flag CLIP_FLAG_KEYFREEFORM must be set on
// the clip root controller.  This gives special status to the currently
// selected layer, to make the GetValues and SetValues work.
//
void CATParentTrans::CollapsePoseToCurLayer(TimeValue t)
{
	if (!mRootHub) return;

	CATClipRoot *layerroot = GetCATLayerRoot();

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	layerroot->SetFlag(CLIP_FLAG_COLLAPSINGLAYERS);
	//layerroot->SetFlag(CLIP_FLAG_KEYFREEFORM);
	CATMessage(t, CAT_KEYFREEFORM);
	//layerroot->ClearFlag(CLIP_FLAG_KEYFREEFORM);
	layerroot->ClearFlag(CLIP_FLAG_COLLAPSINGLAYERS);

	if (newundo) theHold.Accept(GetString(IDS_HLD_POSECOLLAPSE));
}

//	Run through given range, keying the character at the selected
//	intervals. Pls note that this function takes TICKS, not FRAMES
//	as input.
//	TODO: make the frequency a timevalue too
BOOL CATParentTrans::CollapseTimeRangeToLayer(TimeValue start_t, TimeValue end_t, TimeValue iKeyFreq, BOOL regularplot, int numpasses, float posdelta, float rotdelta)
{
	if (!mRootHub || iKeyFreq <= 0 || start_t > end_t) return FALSE;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	TimeValue t;
	Interface* ip = GetCOREInterface();
	Interval iv;

	//////////////////////////////////////////////////////////////////////////
	CATClipRoot *layerroot = GetCATLayerRoot();

	// If a layer isn't selected, then make a new one
	int nLastSelected = layerroot->GetSelectedLayer();
	int nLayer = -1;

	// We should always add a new layer. We had this option before we had the 'Remember KLayer Settings' feature.
	// Never Mind. People seem to use this feature and there is no reason to take it out.
	BOOL addlayer = (nLastSelected < 0) || (layerroot->GetLayer(nLastSelected)->GetMethod() == LAYER_CATMOTION);
	if (addlayer) {
		// Add the new layer
		layerroot->AppendLayer(GetString(IDS_LYR_COLLAPSED), LAYER_ABSOLUTE);
		nLayer = layerroot->NumLayers() - 1;
		layerroot->SelectLayer(nLayer);
		//	CATMessage(CLIP_LAYER_SETROT_CLASS, TCBINTERP_ROTATION_CLASS_ID);
	}
	else {
		nLayer = nLastSelected;
	}

	//////////////////////////////////////////////////////////////////////////
	layerroot->GetLayer(nLayer)->EnableLayer(FALSE);

	layerroot->SetFlag(CLIP_FLAG_COLLAPSINGLAYERS);
	//	layerroot->SetFlag(CLIP_FLAG_KEYFREEFORM);
	BOOL currentAnimState = Animating();
	ip->SetAnimateButtonState(TRUE);
	//	ip->BeginProgressiveMode();

	if (!regularplot) {
		Tab <Control*> ctrls = GetCATRigLayerCtrls();
		Tab< Tab < TimeValue > > poskeytimes;
		Tab< Tab < TimeValue > > rotkeytimes;
		Tab< Tab < TimeValue > > sclkeytimes;
		Tab< Tab < TimeValue > > keytimes;

		poskeytimes.SetCount(ctrls.Count());
		rotkeytimes.SetCount(ctrls.Count());
		sclkeytimes.SetCount(ctrls.Count());
		keytimes.SetCount(ctrls.Count());

		Interval timerange(start_t, end_t);
		int tdm = layerroot->GetTrackDisplayMethod();
		layerroot->SetTrackDisplayMethod(TRACK_DISPLAY_METHOD_ALL);

		BOOL updateviewsduringplot = TRUE;

		for (int i = 0; i < ctrls.Count(); i++) {
			poskeytimes[i].Init();
			rotkeytimes[i].Init();
			sclkeytimes[i].Init();
			keytimes[i].Init();

			if (ctrls[i]->SuperClassID() == CTRL_MATRIX3_CLASS_ID) {
				ctrls[i]->GetKeyTimes(poskeytimes[i], timerange, KEYAT_POSITION);
				ctrls[i]->GetKeyTimes(rotkeytimes[i], timerange, KEYAT_ROTATION);
				ctrls[i]->GetKeyTimes(sclkeytimes[i], timerange, KEYAT_SCALE);
			}
			else {
				ctrls[i]->GetKeyTimes(keytimes[i], timerange, KEYAT_POSITION | KEYAT_ROTATION | KEYAT_SCALE);
			}
		}

		Tab<int> poskeyid;
		Tab<int> rotkeyid;
		Tab<int> sclkeyid;
		Tab<int> keyid;
		poskeyid.SetCount(ctrls.Count());
		rotkeyid.SetCount(ctrls.Count());
		sclkeyid.SetCount(ctrls.Count());
		keyid.SetCount(ctrls.Count());
		for (int i = 0; i < ctrls.Count(); i++) {
			poskeyid[i] = 0;
			rotkeyid[i] = 0;
			sclkeyid[i] = 0;
			keyid[i] = 0;
		}
		t = start_t;
		BOOL firstloop = TRUE;

		while (t < end_t) {

			SHORT esc = GetAsyncKeyState(VK_ESCAPE);
			if (esc & 1) {
				break;
			}

			if (firstloop) {
				firstloop = FALSE;
			}
			else {
				// Find the next time any controller needs a key placed.
				FindNextKeyTime(poskeytimes, rotkeytimes, sclkeytimes, keytimes, \
					poskeyid, rotkeyid, sclkeyid, keyid, \
					start_t, end_t, t);
			}

			ip->SetTime(t, updateviewsduringplot);

			{ // Scope for lockThis
				MaxReferenceMsgLock lockThis;
				for (int i = 0; i < ctrls.Count(); i++)
				{
					CATClipValue *layerctrl = (CATClipValue*)ctrls[i];

					if (layerctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID)
					{
						if (t == start_t && (poskeytimes[i].Count() > 0)) layerctrl->CopyKeysFromTime(start_t, start_t, COPYKEY_POS);
						if (t == start_t && (rotkeytimes[i].Count() > 0)) layerctrl->CopyKeysFromTime(start_t, start_t, COPYKEY_ROT);
						if (t == start_t && (sclkeytimes[i].Count() > 0)) layerctrl->CopyKeysFromTime(start_t, start_t, COPYKEY_SCALE);

						ULONG keyposflag = ((poskeytimes[i].Count() > 0 && ((t == start_t) || (t == end_t) || t == poskeytimes[i][poskeyid[i]])) ? KEY_POSITION : 0);
						ULONG keyrotflag = ((rotkeytimes[i].Count() > 0 && ((t == start_t) || (t == end_t) || t == rotkeytimes[i][rotkeyid[i]])) ? KEY_ROTATION : 0);
						ULONG keysclflag = ((sclkeytimes[i].Count() > 0 && ((t == start_t) || (t == end_t) || t == sclkeytimes[i][sclkeyid[i]])) ? KEY_SCALE : 0);

						CATNodeControl* catnodecontrol = FindCATNodeControl(layerctrl);
						DbgAssert(catnodecontrol != NULL);
						if (catnodecontrol)
						{
							if (keyposflag | keyrotflag | keysclflag) {
								if (keyrotflag && rotkeyid[i] > 0) {
									KeyRotation(layerctrl, rotkeytimes[i][rotkeyid[i] - 1], t);
									keyrotflag = 0;
								}
								if (keyposflag | keyrotflag | keysclflag) {
									catnodecontrol->KeyFreeform(t, keyposflag | keyrotflag | keysclflag);
								}
							}
						}
						else {
							// This section of code only gets entered in very special cases.
							// When the Rig has had its IKTarget deleted. this controller will be the iktarget.
							Matrix3 tm(1);
							iv = FOREVER;
							layerctrl->GetValue(t, (void*)&tm, iv, CTRL_RELATIVE);

							SetXFormPacket ptr;
							if (keyposflag && keyrotflag) {
								ptr.command = XFORM_SET;
								ptr.tmAxis = tm;
								ptr.tmParent = Matrix3(1);
							}
							else if (keyposflag) {
								ptr.command = XFORM_MOVE;
								ptr.p = tm.GetTrans();
								ptr.tmAxis = Matrix3(1);
								ptr.tmParent = Matrix3(1);
							}
							else if (keyrotflag) {
								ptr.command = XFORM_ROTATE;
								// this is not strictly correct. We should be passing in an AngleAxis or a quat,
								// but I can't get the SetValue to Set an absolute rotation. I know Keying a layer has always worked
								// So here I pass in a full transform and CATClipMatrix3::SetValue will generate the correct rotation
								ptr.tmAxis = tm;
								ptr.tmParent = Matrix3(1);
							}
							layerctrl->SetValue(t, (void*)&ptr, 1, CTRL_ABSOLUTE);
						}
					}
					else {
						if (keytimes[i].Count() > 0 && ((t == start_t) || (t == end_t) || t == keytimes[i][keyid[i]])) {
							if (t == start_t) layerctrl->CopyKeysFromTime(start_t, start_t, 0);
							float val;
							iv = FOREVER;
							layerctrl->GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
							layerctrl->SetValue(t, (void*)&val, TRUE, CTRL_ABSOLUTE);
						}
					}
				}
			} // RefMsgLock
		}

		/////////////////////////////////////////////////////////////////
		// Now do a second pass and check that the results are good.
		// We sample the controller value between the keyframes we have created
		// and then we do another pass to check if these values are within our threasholds
		for (int refinements = 0; refinements < numpasses; refinements++)
		{
			std::vector<std::vector<Matrix3>> posmidkeymatrix3;
			std::vector<std::vector<Matrix3>> rotmidkeymatrix3;
			posmidkeymatrix3.resize(ctrls.Count());
			rotmidkeymatrix3.resize(ctrls.Count());

			float ratio = 0.3f;

			for (int i = 0; i < poskeytimes.Count(); i++) {
				FindMidKeys(poskeytimes[i], ratio);
				FindMidKeys(rotkeytimes[i], ratio);
				FindMidKeys(keytimes[i], ratio);

				// Now make sure all our arrays are ready to store all the mide key values.
				posmidkeymatrix3[i].resize(poskeytimes[i].Count());
				rotmidkeymatrix3[i].resize(rotkeytimes[i].Count());
			}

			////////////////////////////////
			layerroot->GetLayer(nLayer)->EnableLayer(TRUE);
			//			layerroot->ClearFlag(CLIP_FLAG_KEYFREEFORM);

			for (int i = 0; i < ctrls.Count(); i++) {
				poskeyid[i] = 0;
				rotkeyid[i] = 0;
				sclkeyid[i] = 0;
				keyid[i] = 0;
			}

			t = start_t;
			firstloop = TRUE;
			while (t < end_t) {

				SHORT esc = GetAsyncKeyState(VK_ESCAPE);
				if (esc & 1) {
					break;
				}
				FindNextKeyTime(poskeytimes, rotkeytimes, sclkeytimes, keytimes, \
					poskeyid, rotkeyid, sclkeyid, keyid, \
					start_t, end_t, t);

				ip->SetTime(t, updateviewsduringplot);

				for (int i = 0; i < ctrls.Count(); i++) {
					CATClipValue *layerctrl = (CATClipValue*)ctrls[i];

					if (layerctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID)
					{
						ULONG keyposflag = ((poskeytimes[i].Count() > 0 && t == poskeytimes[i][poskeyid[i]]) ? KEY_POSITION : 0);
						ULONG keyrotflag = ((rotkeytimes[i].Count() > 0 && t == rotkeytimes[i][rotkeyid[i]]) ? KEY_ROTATION : 0);
						ULONG keysclflag = ((sclkeytimes[i].Count() > 0 && t == sclkeytimes[i][sclkeyid[i]]) ? KEY_SCALE : 0);

						if (keyposflag | keyrotflag | keysclflag) {
							CATNodeControl* catnodecontrol = FindCATNodeControl(layerctrl);
							if (catnodecontrol)
							{
								Matrix3 tm = catnodecontrol->GetNode()->GetNodeTM(t);
								if (keyposflag) posmidkeymatrix3[i][poskeyid[i]] = tm;
								if (keyrotflag) rotmidkeymatrix3[i][rotkeyid[i]] = tm;
							}
							else {
								//		midkeymatrix[i].IdentityMatrix();
								//		layerctrl->GetValue(t, (void*)&(midkeymatrix[i]), FOREVER, CTRL_RELATIVE);
							}
						}
					}
				}
			}
			layerroot->GetLayer(nLayer)->EnableLayer(FALSE);
			//			layerroot->SetFlag(CLIP_FLAG_KEYFREEFORM);

			////////////////////////////////
			for (int i = 0; i < ctrls.Count(); i++) {
				poskeyid[i] = 0;
				rotkeyid[i] = 0;
				sclkeyid[i] = 0;
				keyid[i] = 0;
			}

			t = start_t;
			firstloop = TRUE;
			while (t < end_t) {

				SHORT esc = GetAsyncKeyState(VK_ESCAPE);
				if (esc & 1) {
					break;
				}

				// Find the next time any controller needs a key placed.
				FindNextKeyTime(poskeytimes, rotkeytimes, sclkeytimes, keytimes, \
					poskeyid, rotkeyid, sclkeyid, keyid, \
					start_t, end_t, t);

				ip->SetTime(t, updateviewsduringplot);

				{ // Scope for LockThis
					MaxReferenceMsgLock lockThis;

					for (int i = 0; i < ctrls.Count(); i++) {
						CATClipValue *layerctrl = (CATClipValue*)ctrls[i];

						if (layerctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID)
						{
							ULONG keyposflag = ((poskeytimes[i].Count() > 0 && t == poskeytimes[i][poskeyid[i]]) ? KEY_POSITION : 0);
							ULONG keyrotflag = ((rotkeytimes[i].Count() > 0 && t == rotkeytimes[i][rotkeyid[i]]) ? KEY_ROTATION : 0);
							ULONG keysclflag = ((sclkeytimes[i].Count() > 0 && t == sclkeytimes[i][sclkeyid[i]]) ? KEY_SCALE : 0);

							CATNodeControl* catnodecontrol = FindCATNodeControl(layerctrl);
							if (catnodecontrol)
							{
								if (keyposflag | keyrotflag | keysclflag) {
									Matrix3 oldtm = catnodecontrol->GetNode()->GetNodeTM(t);

									float delatapos = 0.0f;
									if (keyposflag && (delatapos = Length(oldtm.GetTrans() - posmidkeymatrix3[i][poskeyid[i]].GetTrans())) < (GetCATUnits()*posdelta))
										keyposflag = 0;

									float deltaangle = 0.0f;
									if (keyrotflag && (deltaangle = RadToDeg(AngAxis(rotmidkeymatrix3[i][rotkeyid[i]] * Inverse(oldtm)).angle)) < rotdelta)
										keyrotflag = 0;
									keysclflag = 0;

									if (keyposflag | keyrotflag | keysclflag)
									{
										catnodecontrol->KeyFreeform(t, keyposflag | keyrotflag | keysclflag);
									}
								}
							}
							else {
								Matrix3 tm(1);
								iv = FOREVER;
								layerctrl->GetValue(t, (void*)&tm, iv, CTRL_RELATIVE);

								SetXFormPacket ptr;
								if (keyposflag && keyrotflag) {
									ptr.command = XFORM_SET;
									ptr.tmAxis = tm;
									ptr.tmParent = Matrix3(1);
								}
								else if (keyposflag) {
									ptr.command = XFORM_MOVE;
									ptr.p = tm.GetTrans();
									ptr.tmAxis = Matrix3(1);
									ptr.tmParent = Matrix3(1);
								}
								else if (keyrotflag) {
									ptr.command = XFORM_ROTATE;
									// this is not strictly correct. We should be passing in an AngleAxis or a quat,
									// but I can't get the SetValue to Set an absolute rotation. I know Keying a layer has always worked
									// So here I pass in a full transform and CATClipMatrix3::SetValue will generate the correct rotation
									ptr.tmAxis = tm;
									ptr.tmParent = Matrix3(1);
								}
								layerctrl->SetValue(t, (void*)&ptr, 1, CTRL_ABSOLUTE);
							}
						}
						else {
							if (keytimes[i].Count() > 0 && keytimes[i][keyid[i]] == t) {
								float val;
								iv = FOREVER;
								layerctrl->GetValue(t, (void*)&val, iv, CTRL_ABSOLUTE);
								layerctrl->SetValue(t, (void*)&val, TRUE, CTRL_ABSOLUTE);
							}
						}
					}
				} // RefMsgLock
			}
		}
		///////////////////////

		layerroot->SetTrackDisplayMethod(tdm);

	}
	else {

		// save the initial pose at the start of the collapse
		{
			MaxReferenceMsgLock lockThis;
			CATMessage(start_t, CAT_KEYFREEFORM);
		}

		for (t = start_t; t <= end_t; t += iKeyFreq)
		{
			SHORT esc = GetAsyncKeyState(VK_ESCAPE);
			if (esc & 1) {
				break;
			}

			// ST - Trying to fix our abysmal
			// collapsing performance
			{
				MaxReferenceMsgLock lockThis;
				CATMessage(t, CAT_KEYFREEFORM);
			}
		}
	}

	// Put things back the way you found them
	ip->SetAnimateButtonState(currentAnimState);

	layerroot->GetLayer(nLayer)->EnableLayer(TRUE);
	layerroot->ClearFlag(CLIP_FLAG_COLLAPSINGLAYERS);

	UpdateCharacter();

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_LYRCOLLAPSE));
	}
	return TRUE;
}

void CATParentTrans::PasteLayer(INode* from, int fromindex, int toindex, BOOL instance) {

	ReferenceTarget* fromobj = from->GetTMController();
	if (fromobj->ClassID() != ClassID()) return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	CATNodeControl* fromHubRoot = ((CATParentTrans*)fromobj)->GetRootHub();
	RemapDir *remap = NewRemapDir();
	int flags = 0;
	if (instance) flags |= PASTELAYERFLAG_INSTANCE;
	if (mRootHub&&fromHubRoot) {
		mRootHub->PasteLayer(fromHubRoot, fromindex, toindex, flags, *remap);
	}
	remap->DeleteThis();

	if (newundo) theHold.Accept(GetString(IDS_HLD_LYRPASTE));
}

class TransformNode
{
	static MaxSDK::SingleWeakRefMaker mNodeRef;
public:

	static INode* Get() { return static_cast<INode*>(mNodeRef.GetRef()); }
	static void Set(INode* pNode) { mNodeRef.SetRef(pNode); }

};
MaxSDK::SingleWeakRefMaker TransformNode::mNodeRef;

INode* CATParentTrans::CreatePasteLayerTransformNode()
{
	LayerTransform* layertransform = (LayerTransform*)CreateInstance(CTRL_MATRIX3_CLASS_ID, LAYER_TRANSFORM_CLASS_ID);
	INode* transformnode = layertransform->BuildNode();
	TransformNode::Set(transformnode);

	return transformnode;
}

TSTR CATParentTrans::GetFileTagValue(TSTR file, TSTR tag)
{
	// Create our rig loader and begin...
	CATRigReader load(file.data(), GetCATParentTrans());
	BOOL done = FALSE;
	TSTR res(_T("undefined"));
	while (load.ok() && !done)
	{
		load.NextClause();
		if (_tcsicmp(IdentName(load.CurClauseID()), tag.data()) == 0) {
			load.GetValue(res);
			done = TRUE;
		}
	}
	return res;
}

void	CATParentTrans::AddRootNode()
{
	RootNodeCtrl* rootnodectrl = (RootNodeCtrl*)CreateInstance(CTRL_MATRIX3_CLASS_ID, ROOT_NODE_CTRL_CLASS_ID);
	INode* node = rootnodectrl->Initialise(this);
	rootnodehdl = node->GetHandle();

	GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());

	INodeTab nodes;
	nodes.AppendNode(node);
	GetCOREInterface()->FlashNodes(&nodes);
}

INode*	CATParentTrans::GetRootNode()
{
	if (rootnodehdl > 0) {
		INode *node = GetCOREInterface()->GetINodeByHandle(rootnodehdl);
		if (node)			return node;
		rootnodehdl = -1;
	}
	return NULL;
}

void	CATParentTrans::RemoveRootNode()
{
	INode* rootnode = GetRootNode();
	if (rootnode) {
		rootnode->Delete(0, FALSE);
		rootnodehdl = -1;
	}
	GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
}

INodeTab CATParentTrans::GetCATRigNodes()
{
	INodeTab nodes;
	AddSystemNodes(nodes, kSNCFileSave);
	return nodes;
}

Tab <Control*> CATParentTrans::GetCATRigLayerCtrls()
{
	Tab <Control*> layerctrls;
	if (!mRootHub) return layerctrls;

	INode *rootnode = GetRootNode();
	if (rootnode) ((RootNodeCtrl*)rootnode->GetTMController())->AddLayerControllers(layerctrls);

	mRootHub->AddLayerControllers(layerctrls);
	return layerctrls;
}

//////////////////////////////////////////////////////////////////////////

RefTargetHandle CATParentTrans::Clone(RemapDir& remap)
{
	// make a new LiftPlantMod object to be the clone
	// call true for loading so the new LiftPlantMod doesn't
	// make new default subcontrollers.
	CATParentTrans *newctrl = new CATParentTrans(TRUE);

	// another entry will be added to the remap system onthe conclusion of this clone, but wee need one now
	// before we can clone the CATParent. The catparent has a reference to us and therefore calls Clone
	// on us causing an infinite loop.
	remap.AddEntry(this, newctrl);

	// clone our subcontrollers and assign them to the new object.
	if (prs)	newctrl->ReplaceReference(PRS, remap.CloneRef(prs));
	if (pblock)	newctrl->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// We do not need to clone the CATParent.  This pointer is no longer used (by anyone).

	// We want to force the cloning of the CATParent object while the controllers are all cloning because
	// EVERYBODY needs this guy to be cloned. Once he is cloned, there will be a mapping in the remap system
	// to the new CATParent. Then every pointer that is patched will be patched immediately.
	//CATParent *catparent = (CATParent*)GetCATParent();
	//CATParent* pNewParentObj = (CATParent*)remap.CloneRef(catparent);

	remap.PatchPointer((RefTargetHandle*)&newctrl->node, node);
	newctrl->lengthaxis = lengthaxis;
	newctrl->catunits = catunits;
	newctrl->catmode = catmode;
	newctrl->colourmode = colourmode;
	newctrl->catname = catname;
	newctrl->flags = flags;

	if (layerroot)	newctrl->ReplaceReference(LAYERROOT, remap.CloneRef(layerroot));

	if (mRootHub) {
		newctrl->SetRootHub((Hub*)remap.CloneRef(mRootHub));
		newctrl->GetRootHub()->SetCATParentTrans(newctrl);
	}

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newctrl, remap);

	// now return the new object.
	return newctrl;
}

CATParentTrans::~CATParentTrans()
{
	// Remove all back pointers to this.
	CleanCATParentTransPointer(this);

	UnRegisterCallbacks();
	DeleteAllRefs();
}

// Will be phased out later
void CATParentTrans::Copy(Control *from)
{
	UNREFERENCED_PARAMETER(from);
	DbgAssert(0);
}

void CATParentTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	SetXFormPacket* ptr = (SetXFormPacket*)val;

	if (prs != NULL)
		prs->SetValue(t, val, commit, method);
}

// CATParentTrans takes incoming tm, Sets pos to saved pos, then applies own
// rotation and returns.
void CATParentTrans::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	if (prs)
		prs->GetValue(t, val, valid, method);
}

RefTargetHandle CATParentTrans::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:	return pblock;
	case PRS:			return prs;
	case LAYERROOT:		return layerroot;
	default:			return NULL;
	}
}

void CATParentTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:	pblock = (IParamBlock2*)rtarg;		break;
	case PRS:			prs = (Control*)rtarg;				break;
	case LAYERROOT:		layerroot = (CATClipRoot*)rtarg;	break;
	}
}

BaseInterface* CATParentTrans::GetInterface(Interface_ID id)
{
	if (id == CATPARENT_INTERFACE_FP)		return (ICATParentFP*)this;
	if (id == I_EXTRARIGNODES_FP)	return (IExtraRigNodesFP*)this;
	if (id == LAYERROOT_INTERFACE_FP)		return GetCATLayerRoot() ? GetCATLayerRoot()->GetInterface(id) : NULL;
	return ICATParentTrans::GetInterface(id);
}

FPInterfaceDesc* CATParentTrans::GetDescByID(Interface_ID id) {
	if (id == CATPARENT_INTERFACE_FP)		return ICATParentFP::GetFnPubDesc();
	if (id == LAYERROOT_INTERFACE_FP)		return GetCATLayerRoot() ? GetCATLayerRoot()->GetDescByID(id) : &nullInterface;
	if (id == I_EXTRARIGNODES_FP)	return ExtraRigNodes::GetDescByID(id);
	return &nullInterface;
}

void CATParentTrans::RefDeleted()
{

}

Animatable* CATParentTrans::SubAnim(int i)
{
	switch (i)
	{
	case PBLOCK_REF:	return pblock;
	case PRS:			return prs;
	case LAYERROOT:		return layerroot;
	default:			return NULL;
	}
}

TSTR CATParentTrans::SubAnimName(int i)
{

	switch (i)
	{
	case PBLOCK_REF:	return GetString(IDS_PARAMS);
	case PRS:			return GetString(IDS_PRS);
	case LAYERROOT:		return GetString(IDS_CLIPHIERARCHY); // "Layers"
	default:			return _T("");
	}
}

class FindLoopTest : public RefEnumProc
{
	bool mIsLoop;
	INodeTab* mAllNodesToTest;
public:
	FindLoopTest(INodeTab* pAllNodes) : mIsLoop(false), mAllNodesToTest(pAllNodes) {}

	int proc(ReferenceMaker *rm)
	{
		// Hack-job.  During a loop-test, the original entity is marked with Evaluating...
		if (rm->TestAFlag(A_EVALUATING))
		{
			mIsLoop = true;
			return REF_ENUM_HALT;
		}
		// continue, nothing to see here...
		return REF_ENUM_CONTINUE;
	}

	bool IsLoop() { return mIsLoop; }
};

RefResult CATParentTrans::NotifyDependents(const Interval& changeInt, PartID partID, RefMessage message, SClass_ID sclass /* = NOTIFY_ALL */, BOOL propagate /* = TRUE */, RefTargetHandle hTarg /* = NULL */, NotifyDependentsOption notifyDependentsOption)
{
	// If we are testing a dependency loop, include all CAT nodes in these dependencies
	// This message will not be passed to NotifyRefChanged, hence we need to overload
	// this function and process it here.
	if (message == REFMSG_LOOPTEST)
	{
		CATNodeControl* pRoot = GetRootHub();
		if (pRoot != NULL)
		{
			// Get all CAT rig nodes
			INodeTab nodes;
			pRoot->AddSystemNodes(nodes, kSNCClone);
			FindLoopTest loopTest(&nodes);

			// Iterate over all nodes & dependents, were
			// any already tested for loop?
			int count = nodes.Count();
			for (int i = 0; i < count; i++)
			{
				INode* pNode = nodes[i];
				if (pNode != NULL)
				{
					// Cannot call to StdNotifyRefChanged due to protection level
					// This looptest function couldn't be more confusingly implemented...
					pNode->EnumRefHierarchy(loopTest, true, false, false, true);

					// A loop exists, return failure!
					if (loopTest.IsLoop())
						return REF_FAIL;
				}
			}
		}
	}

	return Control::NotifyDependents(changeInt, partID, message, sclass, propagate, hTarg, notifyDependentsOption);
}

RefResult CATParentTrans::NotifyRefChanged(const Interval& iv, RefTargetHandle hTarg, PartID& partId, RefMessage msg, BOOL propagate)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if (hTarg == prs)
		{
			if (GetCATMode() == SETUPMODE)
				UpdateCharacter();
		}
		else if (hTarg == pblock)
		{
			switch (pblock->LastNotifyParamID())
			{
			case PB_CATUNITS:
				catunits = pblock->GetFloat(PB_CATUNITS);
				TimeValue t = GetCOREInterface()->GetTime();
				CATMessage(t, CAT_CATUNITS_CHANGED);
				break;
			}
		}
		// one of our subcontrollers has changed.
		// but because we keep no cached data, we don't
		// care. Data is requested directly instead of
		// looked up from existing values
		break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (pblock == hTarg)	pblock = NULL;
		else if (prs == hTarg)		prs = NULL;
		break;
	case REFMSG_GET_CONTROL_DIM:
		// one of our referenced objects have changed dimension.
		// guess what... we don't care.
		break;
	case REFMSG_LOOPTEST:
	{
		INodeTab nodes;
		GetSystemNodes(nodes, kSNCClone);
		for (int i = 0; i < nodes.Count(); i++)
		{
			INode* pNode = nodes[i];
			if (pNode != NULL)
			{
				RefResult result = pNode->NotifyDependents(iv, partId, msg);
				if (result != REF_SUCCEED)
					return result;
			}
		}
	}
	break;
	}
	return REF_SUCCEED;
}

BOOL CATParentTrans::AssignController(Animatable *control, int subAnim)
{
	if (control)
	{
		// change the controllers
	//	if(subAnim == CATParentTrans::PRS)
	//		ReplaceReference(CATParentTrans::PRS, (RefTargetHandle)control);
		if (ReplaceReference(subAnim, (RefTargetHandle)control) != REF_SUCCEED) return FALSE;
	};

	// Note: 0 here means that there is no special information we need to include
	// in this message.
	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE, TREE_VIEW_CLASS_ID, FALSE);	// this explicitly refreshes the tree view.		( the false here says for it not to propogate )
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);								// this refreshes all objects (controllers etc..) that reference us.  (and it propogates)
	return TRUE;
}

/************************************************************************/
/* Displays our Rollouts                                                */
/************************************************************************/
void CATParentTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsBegin = flags;
	if (flagsBegin&BEGIN_EDIT_MOTION)
	{
		GetCATLayerRoot()->BeginEditParams(ip, flags, prev);
		CATParentTransDesc.BeginEditParams(ip, this, flags, prev);
	}
	else if (flags == 0) {
	}
}

void CATParentTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (flagsBegin&BEGIN_EDIT_MOTION)
	{
		GetCATLayerRoot()->EndEditParams(ip, flags, next);
		CATParentTransDesc.EndEditParams(ip, this, flags, next);
	}
	else if (flagsBegin == 0) {
	}
}

void CATParentTrans::DrawGizmo(TimeValue t, GraphicsWindow *gw) {

	gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world

	bbox.Init();

	////////
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

	Point3 p1, p2;
	Point3 com = GetCOM(t);
	gw->marker(&com, HOLLOW_BOX_MRKR);
	/*
		if(FALSE) //accellertion
			Point3 comPrev = GetCOM(t-GetTicksPerFrame());
			Point3 comNext = GetCOM(t+GetTicksPerFrame());

			Point3 velocity1 = com - comPrev;
			Point3 velocity2 = comNext - com;
			Point3 acceleration = ( velocity1 - velocity2 ) * (float)GetFrameRate();

			// Draw Accellerartion
			p1 = com;
			p2 = com + acceleration;
			dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

			// Draw Gravity
			p1 = com;
			p2 = com + -9.8f;
			dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

			// COM Vec
			p1 = com;
			p2 = com + acceleration;
			dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;
		}
	*/
	//	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

	//	UpdateWindow(hWnd);
	/////////
}

int CATParentTrans::Display(TimeValue, INode*, ViewExp *, int flags)
{
	UNREFERENCED_PARAMETER(flags);
	return 1;
}

void CATParentTrans::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	AddSystemNodes(nodes, ctxt);
}

void CATParentTrans::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	if (ctxt != kSNCDelete) {
		nodes.AppendNode(GetNode());
	}

	ExtraRigNodes::AddSystemNodes(nodes, ctxt);

	INode *rootnode = GetRootNode();
	if (rootnode) nodes.AppendNode(rootnode);

	if (!g_bSavingCAT3Rig && !g_bLoadingCAT3Rig) {
		// if we are deleting the character make sure we delete all the transform nodes
		// and invalidate caches such as the copy/past layer cache
		CATClipRoot *layerroot = GetCATLayerRoot();
		if (layerroot) layerroot->AddSystemNodes(nodes, ctxt);
	}

	CATNodeControl* roothub = GetRootHub();
	if (roothub) roothub->AddSystemNodes(nodes, ctxt);

	if (ctxt == kSNCDelete) {
		nodes.AppendNode(GetNode());
	}
}

BOOL CATParentTrans::BuildMapping(ICATParentTrans* pasteicatparenttrans, CATCharacterRemap &remap, BOOL includeERNnodes)
{
	if (ClassID() != pasteicatparenttrans->ClassID()) return FALSE;

	CATParentTrans* pastecatparent = (CATParentTrans*)pasteicatparenttrans;

	remap.AddEntry(pastecatparent->GetCATLayerRoot(), GetCATLayerRoot());
	remap.AddEntry(pastecatparent->GetNode(), GetNode());

	// Add entries to the CATParent & CATParentTrans
	remap.AddEntry(pastecatparent, this);
	remap.AddEntry(pastecatparent->GetCATParent(), GetCATParent());

	//remap.AddEntry(pastecatparent->GetNode()->GetObjectRef(), GetNode()->GetObjectRef());
	//remap.AddEntry(pastecatparent->GetNode()->GetObjectRef()->FindBaseObject(), GetNode()->GetObjectRef()->FindBaseObject());

	if (GetRootHub() && pastecatparent->GetRootHub()) {
		GetRootHub()->BuildMapping(pastecatparent->GetRootHub(), remap, TRUE);
	}

	// Do this after the CAT rig mapping has been established,
	// the ERN nodes can find their match based on their
	// remapped parents.
	if (includeERNnodes) {
		// TODO: add a mapping between all ERN nodes.
		// Check that the name, object type etc are the same before mapping.
		ExtraRigNodes::BuildMapping((ExtraRigNodes*)pastecatparent, remap);
	}

	return TRUE;
}

Point3 CATParentTrans::GetCOM(TimeValue t)
{
	Point3 com(P3_IDENTITY);
	INodeTab nodes;
	CATNodeControl* roothub = GetRootHub();
	if (!roothub) return com;
	roothub->AddSystemNodes(nodes, kSNCFileSave);

	float mass = 0.0f;
	for (int i = 0; i < nodes.Count(); i++) {
		CATNodeControl *catnodecontrol = (CATNodeControl*)nodes[i]->GetTMController()->GetInterface(I_CATNODECONTROL);
		if (catnodecontrol) {
			com += nodes[i]->GetNodeTM(t).GetTrans();
			Point3 dim = catnodecontrol->GetBoneDimensions();
			mass += (dim.x * dim.y * dim.z);
		}
	}
	com /= mass;
	return com;
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATParentTransPLCB : public PostLoadCallback {
protected:
	CATParentTrans *catparenttrans;
public:
	CATParentTransPLCB(CATParentTrans *pOwner) { catparenttrans = pOwner; }

	DWORD GetFileSaveVersion() {
		if (catparenttrans->GetFileSaveVersion() > 0) return catparenttrans->GetFileSaveVersion();
		return catparenttrans->GetCATParent()->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		// ----------
		// CAT v1.700 - no can load!
		// ----------
		if (GetFileSaveVersion() < CAT_VERSION_1700) {
			::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_NOLOAD_C1C3), GetString(IDS_ERR_CAT1LOAD), MB_OK);
		}

		if (GetFileSaveVersion() < CAT3_VERSION_2802) {
			// We are storing CATUnits in the pblock so that it handl;es changes in world units for us automaticly
			catparenttrans->pblock->EnableNotifications(FALSE);
			catparenttrans->pblock->SetValue(CATParentTrans::PB_CATUNITS, 0, catparenttrans->catunits);
			catparenttrans->pblock->EnableNotifications(TRUE);
			//	iload->SetObsolete();
		}

		// We need to restore the link between CATParent & Hub
		if (catparenttrans->mRootHub != NULL)
			catparenttrans->mRootHub->SetCATParentTrans(catparenttrans);

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

/**********************************************************************
 * Loading and saving....
 */
enum {
	CPTCHUNK_NUMCATCHARS,
	CPTCHUNK_SAVE_VERSION,
	CPTCHUNK_FLAGS,
	CPTCHUNK_CATUNITS,
	CPTCHUNK_CATMODE,
	// Cannot remove this parameter, would change enum values below
	CPTCHUNK_NUMSAVES_OBSOLETE,
	CPTCHUNK_NODE,
	CPTCHUNK_CATPARENTTRANS,
	CPTCHUNK_ROOTHUB,
	CPTCHUNK_LENGTHAXIS,
	CPTCHUNK_ERN,
	CPTCHUNK_CATNAME,
	CPTCHUNK_ROOTNODEHANDLE,
	// Cannot remove the below parameter, or future enum values may conflict
	CPTCHUNK_BUILDNUMBER_OBSOLETE,
};

IOResult CATParentTrans::Save(ISave *isave)
{
	DWORD nb;
	ULONG id;

	// This stores the version of CAT used to save the file.
	dwFileSaveVersion = CAT_VERSION_CURRENT;
	isave->BeginChunk(CPTCHUNK_SAVE_VERSION);
	isave->Write(&dwFileSaveVersion, sizeof DWORD, &nb);
	isave->EndChunk();

	isave->BeginChunk(CPTCHUNK_CATNAME);
	isave->WriteCString(catname);
	isave->EndChunk();

	isave->BeginChunk(CPTCHUNK_CATUNITS);
	isave->Write(&catunits, sizeof(float), &nb);
	isave->EndChunk();

	if (g_bSavingCAT3Rig) {
		int temp = 0;
		isave->BeginChunk(CPTCHUNK_CATMODE);
		isave->Write(&temp, sizeof(int), &nb);
		isave->EndChunk();

		DWORD tempflags;
		tempflags = (flags&~CATFLAG_COLOURMODE_MASK) | (COLOURMODE_CLASSIC&CATFLAG_COLOURMODE_MASK);
		isave->BeginChunk(CPTCHUNK_FLAGS);
		isave->Write(&tempflags, sizeof DWORD, &nb);
		isave->EndChunk();
	}
	else {
		isave->BeginChunk(CPTCHUNK_CATMODE);
		isave->WriteEnum(&catmode, sizeof(int), &nb);
		isave->EndChunk();

		isave->BeginChunk(CPTCHUNK_FLAGS);
		isave->Write(&flags, sizeof DWORD, &nb);
		isave->EndChunk();

	}

	if (node) {
		isave->BeginChunk(CPTCHUNK_NODE);
		id = isave->GetRefID(node);
		isave->Write(&id, sizeof(ULONG), &nb);
		isave->EndChunk();
	}
	if (mRootHub) {
		isave->BeginChunk(CPTCHUNK_ROOTHUB);
		id = isave->GetRefID(mRootHub);
		isave->Write(&id, sizeof(ULONG), &nb);
		isave->EndChunk();
	}
	isave->BeginChunk(CPTCHUNK_LENGTHAXIS);
	isave->Write(&lengthaxis, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(CPTCHUNK_ROOTNODEHANDLE);
	isave->Write(&rootnodehdl, sizeof(ULONG), &nb);
	isave->EndChunk();

	isave->BeginChunk(CPTCHUNK_ERN);
	ExtraRigNodes::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult CATParentTrans::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L;
	ULONG id = 0L;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case CPTCHUNK_SAVE_VERSION: {

			TCHAR buf[256] = { 0 };
			res = iload->Read(&dwFileSaveVersion, sizeof DWORD, &nb);
			// GB 14-Oct-03: Don't set obsolete flag here, as a new version might
			// not ever cause a problem.  It is up to the PostLoadCallbacks to set
			// the flag if necessary.
		//	if (dwFileSaveVersion != CAT_VERSION_CURRENT)
		//		iload->SetObsolete();

			// If the file was saved with a newer version of CAT, display a
			// warning message, giving the option to continue or balk.
			if (dwFileSaveVersion > CAT_VERSION_CURRENT && !g_bOutOfDateWarningDisplayed) {
				_tcscpy(buf, GetString(IDS_ERR_FUTUREFILE1));
				_tcscat(buf, GetString(IDS_ERR_FUTUREFILE2));
				_tcscat(buf, GetString(IDS_ERR_FUTUREFILE3));
				DWORD dwResult = ::MessageBox(GetCOREInterface()->GetMAXHWnd(), buf,
					GetString(IDS_ERR_FUTUREVER4), MB_YESNO | MB_ICONASTERISK);

				g_bOutOfDateWarningDisplayed = true;

				if (dwResult == IDNO)
					res = IO_END;
			}
			if (dwFileSaveVersion < CAT_VERSION_1300) {
				_tcscpy(buf, GetString(IDS_ERR_OLDFILE1));
				_tcscat(buf, GetString(IDS_ERR_OLDFILE2));
				_tcscat(buf, GetString(IDS_ERR_OLDFILE3));
				_tcscat(buf, GetString(IDS_ERR_OLDFILE4));
				_tcscat(buf, GetString(IDS_ERR_OLDFILE5));
				DWORD dwResult = ::MessageBox(GetCOREInterface()->GetMAXHWnd(), buf,
					GetString(IDS_ERR_UPGRADEPROB), MB_YESNO | MB_ICONASTERISK);

				g_bOldFileWarningDisplayed = true;

				if (dwResult == IDNO)
					res = IO_END;
			}
		}
											 break;
		case CPTCHUNK_CATNAME:
			TCHAR *strBuf;
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) catname = strBuf;
			break;
		case CPTCHUNK_FLAGS:			res = iload->Read(&flags, sizeof DWORD, &nb);			break;
		case CPTCHUNK_CATUNITS:			res = iload->Read(&catunits, sizeof(float), &nb);		break;
		case CPTCHUNK_CATMODE:			res = iload->ReadEnum(&catmode, sizeof(int), &nb);		break;
		case CPTCHUNK_NODE:
			iload->Read(&id, sizeof(ULONG), &nb);
			if (id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&node);
			break;
		case CPTCHUNK_ROOTHUB:
			iload->Read(&id, sizeof(ULONG), &nb);
			if (id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&mRootHub);
			break;
		case CPTCHUNK_LENGTHAXIS:		iload->Read(&lengthaxis, sizeof(int), &nb);						break;
		case CPTCHUNK_ROOTNODEHANDLE:	iload->Read(&rootnodehdl, sizeof(ULONG), &nb);					break;
		case CPTCHUNK_ERN:				ExtraRigNodes::Load(iload);										break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new CATParentTransPLCB(this));
	return IO_OK;
}

////////////////////////////////////////////////////////////////////
// ICATParent

class CATParentTransChangeRestore : public RestoreObj {
public:
	CATParentTrans	*catparenttrans;
	int				index;
	int				msg;
	TSTR			catname_undo, catname_redo;
	float			catunits_undo, catunits_redo;
	CATMode			catmode_undo, catmode_redo;;
	CATClipRoot	*root;

	CATParentTransChangeRestore(CATParentTrans *c, int m) : catmode_redo(SETUPMODE), index(0), catunits_redo(0.0f) {
		catparenttrans = c;
		msg = m;
		root = c->GetCATLayerRoot();
		catname_undo = c->catname;
		catunits_undo = c->catunits;
		catmode_undo = c->catmode;
	}

	void Restore(int isUndo) {
		TimeValue t = GetCOREInterface()->GetTime();
		switch (msg) {
		case CAT_CATMODE_CHANGED:
			catmode_redo = catparenttrans->catmode;
			catparenttrans->catmode = catmode_undo;
			catparenttrans->CATMessage(t, CAT_CATMODE_CHANGED);
			break;
		case CAT_NAMECHANGE:
			catname_redo = catparenttrans->catname;
			catparenttrans->catname = catname_undo;
			catparenttrans->CATMessage(t, CAT_NAMECHANGE);
			break;
		case CAT_COLOUR_CHANGED:	break;
		case CAT_CATUNITS_CHANGED:
			catunits_redo = catparenttrans->catunits;
			catparenttrans->catunits = catunits_undo;
			catparenttrans->CATMessage(t, CAT_CATUNITS_CHANGED);
			break;
		}
		if (isUndo) {
			root->RefreshLayerRollout();
		}
	}

	void Redo() {
		TimeValue t = GetCOREInterface()->GetTime();
		switch (msg) {
		case CAT_CATMODE_CHANGED:
			catparenttrans->catmode = catmode_redo;
			catparenttrans->CATMessage(t, CAT_CATMODE_CHANGED);
			break;
		case CAT_NAMECHANGE:
			catparenttrans->catname = catname_redo;
			catparenttrans->CATMessage(t, CAT_NAMECHANGE);
			break;
		case CAT_COLOUR_CHANGED:	break;
		case CAT_CATUNITS_CHANGED:
			catparenttrans->catunits = catunits_redo;
			catparenttrans->CATMessage(t, CAT_CATUNITS_CHANGED);
			break;
		}
		root->RefreshLayerRollout();
	}

	int Size() { return 4; }
	void EndHold() { }
};

float CATParentTrans::GetCATUnits() const {
	//float catunits = (pblock) ? pblock->GetFloat(PB_CATUNITS) : 0.0f;
	// During loading of old files, we refresh the obj_dim variable.
	// In some cases, we are also in the middle of transferring our
	// data from the CATParent to the CATParentTrans controller.
	// CATUnits is still 0 if we have not finished.
	//CATParent* pParent = (CATParent*)const_cast<CATParentTrans*>(this)->GetCATParent();
	//if (catunits <= 0)
	//	catunits = pParent->catparentparams->GetFloat(CATParent::PB_CATUNITS);
	return max(catunits, 0.0000001f);
}

void CATParentTrans::SetCATUnits(float val)
{
	// To change our CATUnits, we can utilize the Max RescaleWorldUnits function.
	// As well as rescaling our rig via CATUnits, we also rescale any animations
	// or non-CAT objects attached to this rig using RescaleWorldUnits
	float catunits = GetCATUnits();
	if (catunits == val)
		return;

	// prevent divide by 0
	val = max(val, 0.000001f);
	float scaleval = val / catunits;

	INodeTab catNodes;
	AddSystemNodes(catNodes, kSNCFileSave);

	// Clear the rescaling flag, otherwise the
	// rescaling will work only once per session
	for (int i = 0; i < catNodes.Count(); i++)
		ClearAFlagInHierarchy(catNodes[i], A_WORK1);

	// Set CATUnits, this resizes the CAT nodes
	pblock->SetValue(PB_CATUNITS, 0, val);

	// Iterate over the non-CAT nodes and animations
	for (int i = 0; i < catNodes.Count(); i++)
	{
		INode* pNode = catNodes[i];
		if (pNode == NULL || pNode == GetNode())
			continue;

		CATNodeControl* pNodeCtrl = dynamic_cast<CATNodeControl*>(pNode->GetTMController());
		if (pNodeCtrl != NULL)
		{
			// For CATNodes, rescale animations
			Control* pLayerTrans = pNodeCtrl->GetLayerTrans();
			if (pLayerTrans != NULL)
				pLayerTrans->RescaleWorldUnits(scaleval);
		}
		else
		{
			// This is _Not_ a CATNodeControl - scale the whole node.
			pNode->RescaleWorldUnits(scaleval);
		}
	}
}

void CATParentTrans::SetCATMode(CATMode mode) {
	if (catmode == mode) return;
	if (theHold.Holding())
		theHold.Put(new CATParentTransChangeRestore(this, CAT_CATMODE_CHANGED));

	CATMode oldcatmode = catmode;
	catmode = (CATMode)mode;
	CATMessage(GetCOREInterface()->GetTime(), CAT_CATMODE_CHANGED, oldcatmode);

	UpdateColours(FALSE);
	CATClipRoot* pLayerRoot = GetCATLayerRoot();
	if (pLayerRoot != NULL)
		pLayerRoot->RefreshLayerRollout();
}

void CATParentTrans::CATMessage(TimeValue t, UINT msg, int data /*= -1*/)
{
	// we might have been deleted but be on the undo queue
	// so for now don't do nothing
	if (/*isResetting || */bRigCreating) return;

	// GB 27-Jan-2004: The CAT_COLOUR_CHANGED message is sent whenever
	// the character's colour needs to be updated.  This is either during
	// user interaction (such as changing a layer colour) or during animation
	// (eg, where a changing layer weight may cause the character's colour
	// to change).  The 'data' passed in is a boolean saying whether to
	// always recalculate the colour (TRUE) or to only do it once for any
	// particular time (FALSE).  The first is used for interactive updates;
	// the second is used for animation updates.
	switch (msg) {
	case CAT_CATMODE_CHANGED:		break;
	case CAT_SHOWHIDE:				break;
	case CAT_VISIBILITY:			break;
	case CAT_NAMECHANGE:			break;
	case CAT_COLOUR_CHANGED:
		if ((BOOL)data == FALSE) {
			if (tvLastColourUpdateTime == t) return;
			tvLastColourUpdateTime = t;
		}
		break;
	case CAT_SET_LENGTH_AXIS:
	case CAT_CATUNITS_CHANGED:
		// invalidate our buildmeshes validity
		// interval so it redraws...
		ECATParent* catparent = GetCATParent();
		if (catparent) {
			catparent->CATMessage(t, msg, data);
		}
		break;
	}

	INode* rootnode = GetRootNode();
	if (rootnode) ((RootNodeCtrl*)rootnode->GetTMController())->CATMessage(t, msg, data);
	if (mRootHub) mRootHub->CATMessage(t, msg, data);
}

// This method is likely to be called during a PLCB so we can't assume that the file has finnished
// upgrading and our node pointer has been set by the CATParent
INode*	CATParentTrans::GetNode()
{
	if (!node)
	{
		// Find the node using the handlemessage system
		node = FindReferencingClass<INode>(this);
	}
	return node;
}

ECATParent*		CATParentTrans::GetCATParent()
{
	INode* pNode = GetNode();
	if (pNode != NULL)
	{
		Object* pObject = pNode->GetObjectRef();
		if (pObject != NULL)
		{
			Object* pBaseObject = pObject->FindBaseObject();
			if (pBaseObject != NULL)
				return static_cast<ECATParent*>(pBaseObject->GetInterface(E_CATPARENT));
		}
	}
	return NULL;
};

ILayerRoot* CATParentTrans::GetLayerRoot()
{
	return layerroot;
};

Control* CATParentTrans::GetRootIHub()
{
	return mRootHub;
};

CATNodeControl* CATParentTrans::GetRootHub()
{
	return mRootHub;
}

void CATParentTrans::SetRootHub(Hub* hub)
{
	HoldData(mRootHub);
	mRootHub = hub;
}

void  CATParentTrans::SetCATName(const MSTR& strCATName) {
	if (strCATName == catname) return;
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	if (theHold.Holding())   theHold.Put(new CATParentTransChangeRestore(this, CAT_NAMECHANGE));

	catname = strCATName;
	if (strCATName.Length() > 0) {
		node->SetName((MCHAR*)strCATName.data());
	}
	TimeValue t = GetCOREInterface()->GetTime();
	CATMessage(t, CAT_NAMECHANGE);

	if (theHold.Holding() && newundo == TRUE) {
		theHold.Accept(GetString(IDS_HLD_CATNAME));
	}
}

BOOL CATParentTrans::LoadRig(CATRigReader* load)
{
	BOOL ok = TRUE;
	bool done = false;

	// Ensure we have our node pointer.
	if (!node)
		node = GetNode();

	bRigCreating = TRUE;
	int newlengthaxis = 0;
	Interface *ip = GetCOREInterface();
	BOOL scene_redraw_disabled = ip->IsSceneRedrawDisabled();
	if (!scene_redraw_disabled) ip->DisableSceneRedraw();
	SuspendAnimate();
	AnimateOff();

	while (load->ok() && !done && ok) {
		if (!load->NextClause()) {
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idParentParams) &&
				(load->CurGroupID() != idCATParent))
			{
				ok = FALSE;
				continue;
			}
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idHubParams:
			case idHub:
			{
				Hub* pNewHub = (Hub*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HUB_CLASS_ID);
				pNewHub->Initialise(this, TRUE);
				SetRootHub(pNewHub);
				ok = pNewHub->LoadRig(load);
				break;
			}
			case idMeshParams:
				GetCATParent()->LoadMesh(load);
				GetCATParent()->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
				break;
			default:	load->AssertOutOfPlace();
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idCatUnits: {
				float filecatunits = 0.0f;
				load->GetValue(filecatunits);
				SetCATUnits(filecatunits);
				// So if we are in the middle of a load, and
				CATParentCreateCallBack* pCATParentCreate = (CATParentCreateCallBack*)GetCATParent()->GetCreateMouseCallBack();
				pCATParentCreate->SetDefaultCATUnits(filecatunits);
				break;
			}
			case idCharacterName:
				//					TSTR parentName;
				//					load->GetValue(parentName);
				//					node->SetName(parentName);
				break;
			case idCATVersion: {
				// kinda lame, but this tells the rigloader what version it is loading
				DWORD nCATPresetVersion;
				load->GetValue(nCATPresetVersion);
				load->SetVersion((DWORD)nCATPresetVersion);
				break;
			}
			case idLengthAxis:
				load->GetValue(newlengthaxis);
				SetLengthAxis(newlengthaxis);
				break;
			default:
				load->AssertOutOfPlace();
			}
			break;
		case rigEndGroup:
			done = true;
			if (ok)
			{
				load->ResolveParentChilds();
			}
			break;
		default:
			load->AssertOutOfPlace();
		}
	}
	bRigCreating = FALSE;

	// from now on all rigs should be X Aligned by default
	if (load->GetVersion() < CAT_VERSION_1730) {
		SetLengthAxis(Z);
		SetLengthAxis(X);
	}

	TimeValue t = ip->GetTime();
	CATMessage(t, CAT_CATUNITS_CHANGED);
	CATMessage(t, CAT_POST_RIG_LOAD);

	ResumeAnimate();

	if (!scene_redraw_disabled) ip->EnableSceneRedraw();
	UpdateCharacter();
	return ok && load->ok();
}

BOOL CATParentTrans::SaveRig(CATRigWriter* save)
{
	// Save parent stuff
	save->BeginGroup(idCATParent);
	TSTR charName = GetNode()->GetName();
	save->Write(idCharacterName, charName);

	float dCATUnits = GetCATUnits();
	save->Write(idCatUnits, dCATUnits);

	DWORD dwVersion = CAT_VERSION_CURRENT;
	save->Write(idCATVersion, dwVersion);

	int lengthaxis = GetLengthAxis();
	save->Write(idLengthAxis, lengthaxis);

	GetCATParent()->SaveMesh(save);

	if (mRootHub)
		mRootHub->SaveRig(save);

	save->EndGroup();
	return save->ok();
}

INode* CATParentTrans::LoadClip(TSTR file, int layerindex, int flags, TimeValue startT, CATControl* bodypart)
{
	if (file.isNull())	return NULL;

	// Can't Load Poses while in Setupmode
	if (!(flags&CLIPFLAG_CLIP) && GetCATMode() == SETUPMODE) return NULL;

	BOOL bNewLayerCreated = FALSE;
	BOOL ok = TRUE;
	BOOL done = FALSE;
	Interval layerrange(-1, -1);
	TCHAR buf[256] = { 0 };

	CATClipRoot *layerroot = GetCATLayerRoot();
	if (!layerroot || !mRootHub) return NULL;

	// TODO: check the extension is the right kind
	// Create our rig loader and begin...
	CATRigReader load(file.data(), GetCATParentTrans());
	if (!load.ok()) return NULL;

	// This is the interval the clip runs for,
	// any existing keys in this period will be deleted.
	// Initialise it to an instant, for no particular reason
	Interval timerange;
	timerange.SetInstant(startT);
	TSTR path, filename, extension;
	SplitFilename(file, &path, &filename, &extension);

	load.ShowProgress(GetString(IDS_PROG_LOADCLIP));

	Interface *ip = GetCOREInterface();
	ip->DisableSceneRedraw();

	// When the right-click menu system is being used, the undo is started when the right click menu is executed
	BOOL newundo = FALSE;
	if (!theHold.Holding()) {
		theHold.Begin(); newundo = TRUE;
	}

	// This is so gawd awful slow, disable messages to speed it up.
	{
		MaxReferenceMsgLock lock;

		float dScale = 1.0f;
		Matrix3 tmTransform(1);
		Matrix3 tmFilePathNodeGuess(1);
		Matrix3 tmCurrPathNodeGuess(1);

		int filelengthaxis = Z;

		// NEVER EVER DO THIS in C++
		//	int commandpanel = ip->GetCommandPanelTaskMode();
		//	ip->SetCommandPanelTaskMode(TASK_MODE_DISPLAY);

		while (load.ok() && !done && ok)
		{
			load.NextClause();
			switch (load.CurClauseID())
			{
			case rigAbort:
			case rigEnd:
				done = true;
				break;

			case rigEndGroup:
				// Well we shouldn't get one here!!  Need error msg.
				ok = FALSE;
				break;

			case rigBeginGroup:
			{
				switch (load.CurIdentifier())
				{
				case idCATParent:
				case idParentParams:
					// There is a tag, but it is not
					// used for anything currently (but if
					// we dont have a case, then we throw up error
					break;
				case idLayerInfo: {
					// do we need a new layer to load the data into -
					// if we are loading more than one layer the layer info
					// will load first and remove this flag.
					if (flags&CLIPFLAG_CREATENEWLAYER && !bNewLayerCreated) {
						LayerInfo layerinfo;
						load.GetValue(layerinfo);
						// new layers are automaticly keyed with the current pose
						if (!layerroot->AppendLayer(filename, (ClipLayerMethod)(layerinfo.dwFlags&LAYER_METHOD_MASK)))
						{
							ok = false;
							break;
						}

						layerindex = layerroot->NumLayers() - 1;

						layerroot->GetLayer(layerindex)->SetFlags(layerinfo.dwFlags);
						layerroot->GetLayer(layerindex)->SetColour(Color(layerinfo.dwLayerColour));

						if (layerroot->GetLayer(layerindex)->GetMethod() == LAYER_ABSOLUTE) {
							// Make the Transform controler that wasn't created by default
							BOOL bIsAnimating = Animating();
							if (bIsAnimating) AnimateOff();
							layerroot->GetLayer(layerindex)->ReplaceReference(NLAInfo::REF_TRANSFORM, NewDefaultMatrix3Controller());
							CATMessage(startT, CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER, layerindex);
							if (bIsAnimating) AnimateOn();
						}

						flags &= ~CLIPFLAG_CREATENEWLAYER;
					}
					break;
				}
				case idNLAInfo: {
					if (layerrange.Start() >= 0 && layerrange.End() >= 0 && layerrange.Start() == layerrange.End() && layerroot->GetSelectedLayer() >= 0) {
						flags &= ~CLIPFLAG_CREATENEWLAYER;
						load.SkipGroup();
						break;
					}

					// We set the initial type of the layer to IGNORE, the correct
					// layer type is set after in the pLayer->LoadClip call
					layerindex = layerroot->AppendLayer(filename, LAYER_IGNORE);
					NLAInfo* pLayer = layerroot->GetLayer(layerindex);
					if (pLayer != NULL)
						pLayer->LoadClip(&load, timerange, dScale, flags);

					// At least one layer has been created. Remove the flag
					flags &= ~CLIPFLAG_CREATENEWLAYER;
					break;
				}
				case idWeights: {

					// do we need a new layer to load the data into -
					// if we are loading more than one layer the layer info
					// will load first and remove this flag.
					if (flags&CLIPFLAG_CREATENEWLAYER && !bNewLayerCreated) {
						// new layers are automaticly keyed with the current pose
						layerroot->AppendLayer(filename, LAYER_RELATIVE);
						layerindex = layerroot->NumLayers() - 1;

						layerroot->GetLayer(layerindex)->SetMethod(LAYER_ABSOLUTE);
						// Make the Transform controler that wasn't created by default
						BOOL bIsAnimating = Animating();
						if (bIsAnimating) AnimateOff();
						layerroot->GetLayer(layerindex)->ReplaceReference(NLAInfo::REF_TRANSFORM, NewDefaultMatrix3Controller());
						CATMessage(startT, CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER, layerindex);
						if (bIsAnimating) AnimateOn();

						flags &= ~CLIPFLAG_CREATENEWLAYER;
					}

					// We are loading an old clip so we need to used out special loader
					layerroot->GetLayer(layerindex)->LoadPreCAT2Weights(&load, timerange, dScale, flags);
					break;
				}
				case idRootBone: {
					INode* rootnode = GetRootNode();
					if (rootnode) {
						((RootNodeCtrl*)rootnode->GetTMController())->LoadClip(&load, timerange, layerindex, dScale, flags);
					}
					else {
						load.SkipGroup();
					}
					break;
				}
				case idHub:
				case idHubParams:
				case idLimb:
				case idLimbParams:
				case idTail:
				case idPalm:
				case idDigit:
				case idArbBone:
				{
					///////////////////////////////////////////////////////
					// the idClipLength is the tag that follows length axis
					// if we did not find an idLengthAxis tag, then we can
					// assume that the file is a Z aixs aligned file
					if (filelengthaxis != lengthaxis) {
						_tcscpy(buf, GetString(IDS_ERR_LENAXIS1));
						_tcscat(buf, GetString(IDS_ERR_LENAXIS2));
						_tcscat(buf, GetString(IDS_ERR_LENAXIS3));
						::MessageBox(ip->GetMAXHWnd(), buf,
							GetString(IDS_ERR_FILELOAD), MB_OK);
						//	theHold.Restore();
						done = TRUE;
						ok = FALSE;
						break;
					}
					if (bodypart && (load.CurIdentifier() == idHub) && !bodypart->GetInterface(HUB_INTERFACE_FP)) {
						_tcscpy(buf, GetString(IDS_ERR_PARHUB1));
						_tcscat(buf, GetString(IDS_ERR_PARHUB2));
						_tcscat(buf, GetString(IDS_ERR_PARHUB3));
						::MessageBox(ip->GetMAXHWnd(), buf,
							GetString(IDS_ERR_FILELOAD), MB_OK);

						done = TRUE;
						ok = FALSE;
						break;
					}
					if (bodypart && (load.CurIdentifier() == idLimb || load.CurIdentifier() == idLimbParams) &&
						bodypart->ClassID() != LIMBDATA2_CLASS_ID) {
						_tcscpy(buf, GetString(IDS_ERR_LIMBLOAD1));
						_tcscat(buf, GetString(IDS_ERR_LIMBLOAD2));
						_tcscat(buf, GetString(IDS_ERR_LIMBLOAD3));
						_tcscat(buf, GetString(IDS_ERR_LIMBLOAD4));
						::MessageBox(ip->GetMAXHWnd(), buf,
							GetString(IDS_ERR_FILELOAD), MB_OK);

						done = TRUE;
						ok = FALSE;
						break;
					}
					if (bodypart && (load.CurIdentifier() == idTail) && bodypart->ClassID() != TAILDATA2_CLASS_ID) {
						::MessageBox(ip->GetMAXHWnd(),
							GetString(IDS_ERR_TAIL),
							GetString(IDS_ERR_FILELOAD), MB_OK);

						done = TRUE;
						ok = FALSE;
						break;
					}

					// do we need a new layer to load the data into
					if (flags&CLIPFLAG_CREATENEWLAYER) {
						_tcscpy(buf, GetString(IDS_ERR_LRYSEL1));
						_tcscat(buf, GetString(IDS_ERR_LRYSEL2));
						::MessageBox(ip->GetMAXHWnd(), buf, GetString(IDS_ERR_FILELOAD), MB_OK);

						done = TRUE;
						ok = FALSE;
						break;
					}

					if (layerroot->GetLayer(layerindex)->CanTransform())
						load.SetClipTransforms(/*tmCurrPathNodeGuess, */tmFilePathNodeGuess, tmTransform, dScale);
					else	// we don't want relative layers getting thier data transformed around cause it just makes a mess
						load.SetClipTransforms(/*Matrix3(1), */Matrix3(1), Matrix3(1), dScale);

					if (bodypart) {
						bodypart->LoadClip(&load, timerange, layerindex, dScale, flags);
					}
					else {
						GetRootHub()->LoadClip(&load, timerange, layerindex, dScale, flags);
					}

					break;
				}
				case idCATMotionLayer: {

					layerroot->AppendLayer(GetString(IDS_LYR_CATMOTION), LAYER_CATMOTION);
					layerindex = layerroot->NumLayers() - 1;

					int newflags = flags;
					newflags &= ~CLIPFLAG_MIRROR;
					layerroot->GetLayer(layerindex)->LoadClip(&load, timerange, dScale, newflags);

					// At least one layer has been created. Remove the flag
					flags &= ~CLIPFLAG_CREATENEWLAYER;
					break;
				}
				default:
					load.AssertOutOfPlace();
				}
				break;
			}
			case rigAssignment:
				switch (load.CurIdentifier())
				{
				case idClipLength:
				{
					int clipLength;
					load.GetValue(clipLength);
					timerange.SetEnd(timerange.Start() + clipLength);

					INode *pTransformNode = TransformNode::Get();
					if (pTransformNode != NULL) {
						((LayerTransform*)pTransformNode->GetTMController())->SetGhostTimeRange(timerange);
					}
					break;
				}
				case idLayerRange:
					load.GetValue(layerrange);
					break;

					// this tag is for when we are lading in multiple layers
					// it implies that we will be getting a few of these
				case idValLayerInfo: {
					LayerInfo layerinfo;
					load.GetValue(layerinfo);
					// we need a new layer, so we give it default variables
					layerroot->AppendLayer(layerinfo.strName, LAYER_RELATIVE);

					// now we are going to load into this lyer
					layerindex = layerroot->NumLayers() - 1;
					layerroot->GetLayer(layerindex)->SetFlags(layerinfo.dwFlags);
					layerroot->GetLayer(layerindex)->SetColour(Color(layerinfo.dwLayerColour));

					if (layerroot->GetLayer(layerindex)->GetMethod() == LAYER_ABSOLUTE) {
						// Make the Transform controler that wasn't created by default
						BOOL bIsAnimating = Animating();
						if (bIsAnimating) AnimateOff();
						layerroot->GetLayer(layerindex)->ReplaceReference(NLAInfo::REF_TRANSFORM, NewDefaultMatrix3Controller());
						CATMessage(startT, CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER, layerindex);
						if (bIsAnimating) AnimateOn();
					}

					// make sure we don't create the extra ne one the user asked for
					flags &= ~CLIPFLAG_CREATENEWLAYER;

					// These two instructions do :Sliterally the same thing, but we are about
					// to release 1.2 so I dont wanna change anything. If anyone reads this
					// then take a second to remove all references to bNewLayerCreated (and this comment).
					bNewLayerCreated = TRUE;
					break;
				}
				case idBodyPartID: {
					TSTR filebodypartid, currbodypartid = bodypart ? IdentName(bodypart->GetRigID()) : _T("");
					load.GetValue(filebodypartid);
					if (!bodypart || (0 != _tcsicmp(currbodypartid.data(), filebodypartid.data()))) {
						TSTR msg;
						msg.printf(GetString(IDS_ERR_SAVELOAD), filebodypartid, filebodypartid);
						::MessageBox(ip->GetMAXHWnd(), msg, GetString(IDS_ERR_FILELOAD), MB_OK);
						done = TRUE;
						ok = FALSE;
						break;
					}
					break;
				}
				case idValMatrix3: {
					Matrix3 tmTrans;
					load.GetValue(tmTrans);
					//	Control *ctrlLayerTrans = layerroot->GetLayer(layerindex)->GetTransformController();
					//	SetXFormPacket xfTrans(tmTrans);

					// Lets not key this set
					BOOL bIsAnimating = Animating();
					if (bIsAnimating) AnimateOff();
					layerroot->GetLayer(layerindex)->SetTransform(ip->GetTime(), tmTrans);
					//	ctrlLayerTrans->SetValue(ip->GetTime(), (void*)&xfTrans);
					if (bIsAnimating) AnimateOn();
					break;
				}
				case idCatUnits:
					load.GetValue(dScale);
					dScale = GetCATUnits() / dScale;
					break;
				case idCATVersion:
					DWORD nCATPresetVersion;
					// kinda lame, but this tells the rigloader what version it is loading
					load.GetValue(nCATPresetVersion);
					load.SetVersion((DWORD)nCATPresetVersion);

					///////////////////////////////////////////////////////
					// the idClipLength is the tag that follows length axis
					// if we did not find an idLengthAxis tag, then we can
					// assume that the file is a Z aixs aligned file
					if (nCATPresetVersion < 1200) {
						_tcscpy(buf, GetString(IDS_ERR_VOLDFILE1));
						_tcscat(buf, GetString(IDS_ERR_VOLDFILE2));
						_tcscat(buf, GetString(IDS_ERR_VOLDFILE3));
						_tcscat(buf, GetString(IDS_ERR_VOLDFILE4));
						::MessageBox(ip->GetMAXHWnd(), buf,
							GetString(IDS_ERR_FILELOAD), MB_OK);
						done = TRUE;
						ok = FALSE;
						break;
					}
					break;
				case idLengthAxis:
					load.GetValue(filelengthaxis);
					break;
				case idPathNodeGuess:
					load.GetValue(tmFilePathNodeGuess);

					// We need to scale the translation our file pathnode guess,
					// when we are reloading the keyframes, this will keep our
					// character in place if the tnTransform is used.
					if (flags&CLIPFLAG_SCALE_DATA)
						tmFilePathNodeGuess.SetTrans(tmFilePathNodeGuess.GetTrans() * dScale);

					tmCurrPathNodeGuess = mRootHub->ApproxCharacterTransform(timerange.Start());
					tmTransform = Inverse(tmFilePathNodeGuess) * tmCurrPathNodeGuess;

					// If we have been provided with a transform node, then we should get our values off of that
					{
						INode* pTransformNode = TransformNode::Get();
						if (pTransformNode != NULL)
						{
							((LayerTransform*)pTransformNode->GetTMController())->Initialise(GetCATParentTrans(), filename, file, layerindex, flags, startT, bodypart);
							if (!((LayerTransform*)pTransformNode->GetTMController())->TransformsLoaded()) {
								((LayerTransform*)pTransformNode->GetTMController())->SetTransforms(tmFilePathNodeGuess, tmTransform);
								pTransformNode->InvalidateTM();
								Matrix3 tm = pTransformNode->GetNodeTM(0);
							}
							else {
								((LayerTransform*)pTransformNode->GetTMController())->GetTransforms(tmFilePathNodeGuess, tmTransform);
								load.GetValue(tmFilePathNodeGuess);
							}
						}
					}
					break;
				case idWeights:
					DbgAssert(0);
					// TODO: rewrite the Clip Loading to use the root to load these clips
					//					((CATClipValue*)layerroot->GetWeights())->LoadClip(&load, timerange, layerindex, dScale, flags, tmTransform);
					break;
				default:
					load.AssertOutOfPlace();
					break;
				}
				break;
			}
		}

		if (ok) {
			if (theHold.Holding() && newundo) {
				theHold.Accept(GetString(IDS_HLD_ANIMLOAD));
			}
			layerroot->RefreshLayerRollout();
		}
		else {
			if (theHold.Holding() && newundo) {
				theHold.Cancel();
				TransformNode::Set(NULL);
			}
		}
	} // End max msg lock

	ip->EnableSceneRedraw();

	// No Idea why, but we aint appearing after a Button load
	// so this will force that
	ip->RedrawViews(ip->GetTime());

	UpdateCharacter();

	return TransformNode::Get();
}

BOOL CATParentTrans::SaveClip(TSTR filename, int flags, Interval timerange, Interval layerrange, CATControl* bodypart)
{
	CATClipRoot *layerroot = GetCATLayerRoot();
	if (!layerroot) return FALSE;

	// Create our rig saver and begin...
	CATRigWriter save(filename.data(), GetCATParentTrans());
	if (!save.ok()) {
		::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_FILENOCREATE), GetString(IDS_ERR_FILECREATEERR), MB_OK);
		return FALSE;
	}

	// Insert a comment with a timestamp at the top of the file.
	time_t ltime;
	time(&ltime);
	save.Comment(GetString(IDS_MSG_EXPORT_CLIP), _tctime(&ltime));

	DWORD dwCurrVer = CAT_VERSION_CURRENT;
	save.Write(idCATVersion, dwCurrVer);
	save.Write(idLengthAxis, lengthaxis);

	TSTR charName = GetNode()->GetName();
	save.Write(idCharacterName, charName);

	// save out the length of the animation
	int animLength;
	if (timerange.Start() != FOREVER.Start() && timerange.End() != FOREVER.End()) {
		animLength = timerange.End() - timerange.Start();
	}
	else {
		Interval range = GetCOREInterface()->GetAnimRange();
		animLength = range.End() - range.Start();
	}
	save.Write(idClipLength, animLength);
	save.Write(idLayerRange, layerrange);

	// We expand our timerange by 1 to ensure allkeys within this range get saved.
	// If a key lands exactly on the start or end time, then it should still be saved
	// These 1 tick offsets offset all loaded keys by one tick wich was annoying when
	// the keys were loaded back in because they didn't align with whole key times.
	// Removing this didn't effect the saving of keys that lanede on the boundary times.
	//	if(timerange.Start() > FOREVER.Start()) timerange.SetStart( timerange.Start()-1 );
	//	if(timerange.End() < FOREVER.End()) timerange.SetEnd( timerange.End()+1 );

	// write out CATUnits, so we can rescale
	// on read.
	float dCATUnits = GetCATUnits();
	save.Write(idCatUnits, dCATUnits);

	if (mRootHub)
	{
		Matrix3 tmPathNode;
		tmPathNode = mRootHub->ApproxCharacterTransform(timerange.Start());
		save.Write(idPathNodeGuess, tmPathNode);

		if (bodypart) {
			TSTR rigID(IdentName(bodypart->GetRigID()));
			save.Write(idBodyPartID, rigID);
		}

		// here we start saving out lots of layers
		for (int i = layerrange.Start(); i <= layerrange.End(); i++)
		{
			if (i < 0 || i >= layerroot->NumLayers()) break; // i can be -1

			//	if(layerroot->GetLayer(i)->GetMethod()==LAYER_CATMOTION) continue;
			// we select the layer so that commands
			layerroot->SelectLayer(i);

			// if we are saving out a whole body clip then save the NLA infos, if we are
			// saving out a body part then only save the layer infos if we are saving out
			// multiple layers
			if (!bodypart || (layerrange.Start() != layerrange.End()))
			{
				// Write out the Layer Info Data.
				// On load this will trigger a new layer to be created,
				// and the layerindex value to be defines.
				layerroot->GetLayer(i)->SaveClip(&save, flags, timerange);

				// Write out the rootnode animation data. The root node is the node that defines
				// the character transform when exporting to game engines like Unreal.
				INode* rootnode = GetRootNode();
				if (rootnode) ((RootNodeCtrl*)rootnode->GetTMController())->SaveClip(&save, flags, timerange, i);
			}
			if (bodypart)	bodypart->SaveClip(&save, flags, timerange, i);
			else			GetRootHub()->SaveClip(&save, flags, timerange, i);
		}
	}
	return save.ok();
}

void CATParentTrans::UpdateCharacter()
{
	TimeValue t = GetCOREInterface()->GetTime();
	CATMessage(t, CAT_UPDATE);
	//	if(!GetCOREInterface()->IsAnimPlaying())
	//		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CATColourMode CATParentTrans::GetEffectiveColourMode() {
	return (GetCATMode() == SETUPMODE) ? COLOURMODE_CLASSIC : GetColourMode();
}

CATColourMode CATParentTrans::GetColourMode()
{
	DWORD catColourFlags = GetMaskFlag(CATFLAG_COLOURMODE_MASK);
	catColourFlags = catColourFlags >> CATFLAG_COLOURMODE_SHIFT;
	// catColourFlags are still the power of the actual mode
	// (because catColourFlags are 1, 2, 3, 4, and our flag
	// values are 1, 2, 4, 8).  I think there is a way to easily
	// find the root, but I can't think of it now, so I'm hardcoding
	// it (I'm cheating).  At less than 3, the flag == the mode.
	DbgAssert(catColourFlags < 3);
	return (CATColourMode)catColourFlags;
}

void CATParentTrans::SetColourMode(CATColourMode colmode, BOOL bRedraw/*=TRUE*/)
{
	// Shift from our colmode up to where it is stored in the flags variable.
	DWORD catColMode = 0;
	if (colmode > 0)
	{
		catColMode = 1 << ((CATFLAG_COLOURMODE_SHIFT - 1) + colmode);
		DbgAssert((CATFLAG_COLOURMODE_MASK&catColMode) != 0);
	}
	SetMaskFlag(CATFLAG_COLOURMODE_MASK, catColMode);
	UpdateColours(bRedraw);
	GetCATLayerRoot()->UpdateUI();
}

void CATParentTrans::UpdateColours(BOOL bRedraw/*=TRUE*/, BOOL bRecalculate/*=TRUE*/)
{
	// we have often been getting update colour calls from within PLCBs and we need to
	// stop this untill the whole character is loaded.
	if (bRigCreating || g_bLoadingCAT3Rig) return;
	Interface* pCore = GetCOREInterface();
	CATMessage(pCore->GetTime(), CAT_COLOUR_CHANGED, (int)bRecalculate);
	if (bRedraw)
		pCore->ForceCompleteRedraw();
}

int  CATParentTrans::GetTrackDisplayMethod() { return GetCATLayerRoot()->GetTrackDisplayMethod(); };
void CATParentTrans::SetTrackDisplayMethod(int n) { GetCATLayerRoot()->SetTrackDisplayMethod(n); };

Matrix3 CATParentTrans::GettmCATParent(TimeValue t)
{
	INode* pMyNode = GetNode();
	if (pMyNode != NULL)
	{
		// TODO: How should we deal with scale?  Should we support it at all?
		return pMyNode->GetNodeTM(t);
	}
	return Matrix3(1);
}

Point3 CATParentTrans::GetCATParentScale(TimeValue t) {
	if (!tmCATValid.InInterval(t))
	{
		tmCATParent.IdentityMatrix();
		p3CATParentScale = Point3(1, 1, 1);

		ICATParentTrans *catparenttrans = GetCATParentTrans();
		if (catparenttrans)
		{
			tmCATValid.SetInfinite();

			Control *ctrlPosition = catparenttrans->GetPositionController();
			if (ctrlPosition) ctrlPosition->GetValue(t, (void*)&tmCATParent, tmCATValid, CTRL_RELATIVE);

			Control *ctrlRotation = catparenttrans->GetRotationController();
			if (ctrlRotation) ctrlRotation->GetValue(t, (void*)&tmCATParent, tmCATValid, CTRL_RELATIVE);

			Control *ctrlCatScale = catparenttrans->GetScaleController();
			if (ctrlCatScale && GetCATMode() != SETUPMODE)
			{
				ScaleValue CATScale;
				ctrlCatScale->GetValue(t, (void*)&CATScale, tmCATValid, CTRL_ABSOLUTE);
				p3CATParentScale = CATScale.s;
			}
		}
	}
	return p3CATParentScale;
}

INode* CATParentTrans::LoadRig(const MSTR& filename, Matrix3* tm/*=NULL*/)
{
	if (filename.isNull())
		return NULL;

	TSTR path, name, extension;
	SplitFilename(filename, &path, &name, &extension);

	// Remove the '.' from '.rg3'
	extension = extension.Substr(1, extension.Length() - 1);
	INode* pResult = NULL;
	if (MatchPattern(extension, TSTR(CAT3_RIG_PRESET_EXT), TRUE))
		pResult = LoadCATBinaryRig(filename, name, tm);
	else
		pResult = LoadCATAsciiRig(filename);

	DbgAssert(pResult != NULL);
	if (pResult == NULL)
		return NULL;

	// Create a unique name for this rig
	Interface* ip = GetCOREInterface();
	INode* pNamedNode = ip->GetINodeByName(name);
	if (pNamedNode != NULL && pNamedNode != pResult) {
		ip->MakeNameUnique(name);
	}

	// This class has been potentially deleted (if we called LoadBinaryRig)
	// In this case we need to access the 'right' CATParentTrans of the
	// returned INode
	if (pResult != NULL)
	{
		HoldSuspend hs; // We don't want to add an extra undo just for this action
		CATParentTrans* pNewCATParent = dynamic_cast<CATParentTrans*>(pResult->GetTMController());
		DbgAssert(pNewCATParent != NULL);
		if (pNewCATParent != NULL)
			pNewCATParent->SetCATName(name);

	}
	return pResult;
}

INode* CATParentTrans::LoadCATAsciiRig(const MSTR& file)
{
	// the macro recorder refused to stop working so I put this in to really stop it
	MACRO_DISABLE;

	// Create our rig loader and begin...
	CATRigReader load(file.data(), this);
	bool done = false;
	BOOL ok = TRUE;
	while (load.ok() && !done && ok) {
		load.NextClause();
		switch (load.CurClauseID()) {
		case rigBeginGroup:
			ok = LoadRig(&load);
			done = TRUE;
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

	Interface* ip = GetCOREInterface();

	MACRO_ENABLE;
	ip->RedrawViews(GetCOREInterface()->GetTime());

	return GetNode();
}

bool SearchMappingToRG3File(CATCharacterRemap& remap, INode* pPotentialOrigNode, INode* pMergedNode)
{
	if (pMergedNode == NULL || pPotentialOrigNode == NULL)
		return false;

	// It may have been pulled in automatically if it is a system node, and one of it's other
	// system nodes were included.
	INodeTab sysNodes;
	Control* pController = pPotentialOrigNode->GetTMController();
	if (pController != NULL)
		pController->GetSystemNodes(sysNodes, kSNCFileSave);
	Object* pObject = pPotentialOrigNode->GetObjectRef();
	if (pObject != NULL)
		pObject->GetSystemNodes(sysNodes, kSNCFileSave);

	for (int i = 0; i < sysNodes.Count(); i++)
	{
		if (remap.FindMapping(sysNodes[i]) != NULL)
		{
			// Success - this node is part of a system that was
			// marked part of the orig RG3 file.  Include this in
			// our remapping!
			return true;
		}
	}

	// If we haven't found a system link, we try for parents and children...
	if (pPotentialOrigNode != NULL)
	{
		if (remap.FindMapping(pPotentialOrigNode->GetParentNode()))
			return true;
		else if (pPotentialOrigNode->GetParentNode()->IsRootNode() && pMergedNode->GetParentNode()->IsRootNode())
			return true;
		else
		{
			for (int j = 0; j < pPotentialOrigNode->NumberOfChildren(); j++)
			{
				if (remap.FindMapping(pPotentialOrigNode->GetChildNode(j)) != NULL)
					return true;
			}
		}
	}
	return false;
}

// This function will go through all the nodes that
// were loaded in the RG3 file, search for any that
// are not marked as being part of the RG3 file
// If a missing node is found, we attempt to find
// an existing one in the file that we can remap from/to
// and add the missing one to the CATParenTrans
void CleanUpMissingNodePointers(CATCharacterRemap& remap, INodeTab& dExistingNodes, INodeTab& dMarkedRG3Nodes, INodeTab& dAllMergedNodes, CATParentTrans* pMergedCATParent)
{
	for (int i = 0; i < dAllMergedNodes.Count(); i++)
	{
		INode* pMergedNode = dAllMergedNodes[i];
		// Is this node marked as being part of the RG3?
		if (dMarkedRG3Nodes.IndexOf(pMergedNode) < 0)
		{
			// Not found... we have a loose node.  We need to see if it existed in the
			// original scene, add the remap, and set it as part of the RG3.

			// This node should be a standard Max node, it should not be a CATNode.
			DbgAssert(pMergedNode->GetTMController()->GetInterface(I_NODECONTROL) == FALSE);

			// If we are not a CAT Node, we can attempt to find the original via it's name
			// A CATNode may be renamed because of its RG3 file name, but non-CAT nodes
			// should preserve the same name
			INode* pPotentialOrigNode = GetCOREInterface()->GetINodeByName(pMergedNode->GetName());
			if (pPotentialOrigNode != NULL && pPotentialOrigNode != pMergedNode)
			{
				// We have a similar named node in the scene.  Test it to see if could be linked to the RG3
				// If we cannot find any link, NULL the pointer and we'll keep looking
				if (SearchMappingToRG3File(remap, pPotentialOrigNode, pMergedNode))
				{
					// Mapping exists, add this to Remap and mark as ExtraRigNode on CATParentTrans
					remap.AddEntry(pPotentialOrigNode, pMergedNode);
					INodeTab extraNode;
					extraNode.AppendNode(pMergedNode);
					pMergedCATParent->AddExtraRigNodes(extraNode);
					// Add the original to the original rig, for now we treat it as if it part of the original rig.
					dExistingNodes.AppendNode(pPotentialOrigNode);
				}
			}
		}
	}
}

// Recursively add child and all children to this tab.
void AddNodeAndChildren(INode* pNewNode, INodeTab& dAllMergedNodes)
{
	if (pNewNode != NULL)
	{
		dAllMergedNodes.AppendNode(pNewNode);
		for (int i = 0; i < pNewNode->NumberOfChildren(); i++)
			AddNodeAndChildren(pNewNode->GetChildNode(i), dAllMergedNodes);
	}
}

void AddAllNewNodesToTab(int iNumRootNodesPreLoad, INodeTab& dAllMergedNodes)
{
	INode* pRoot = GetCOREInterface()->GetRootNode();

	for (int i = iNumRootNodesPreLoad; i < pRoot->NumberOfChildren(); i++)
	{
		INode* pNewNode = pRoot->GetChildNode(i);
		AddNodeAndChildren(pNewNode, dAllMergedNodes);
	}
}

CATParent* FindMergedCATParent(const INodeTab& dAllMergedNodes)
{
	// we should assume that the 1st CATParent that we find is the. this maybe us
	for (int i = 0; i < dAllMergedNodes.Count(); i++)
	{
		INode *pMergedNode = dAllMergedNodes[i];
		if (pMergedNode != NULL && pMergedNode->GetObjectRef()->FindBaseObject()->GetInterface(E_CATPARENT))
		{
			return static_cast<CATParent*>(pMergedNode->GetObjectRef()->FindBaseObject());
		}
	}
	DbgAssert(FALSE && !_T("ERROR: Missing CATParent in RG3 File Load"));
	return NULL;
}

INode* CATParentTrans::LoadCATBinaryRig(const MSTR& file, MSTR& name, Matrix3 * tm)
{
	INode* pNewCATParent = NULL;
	// Initialise the progress bar.
	IProgressWindow *pProgressWindow = GetProgressWindow();
	TCHAR ProgStr[256] = { 0 };
	_stprintf(ProgStr, GetString(IDS_LOADINGRIG), name);
	if (!pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS), ProgStr))
		pProgressWindow = NULL;

	// To help in debugging, we keep track of each stage reached so we can display usefull error messages.
	int stage_reached = 0;

	Interface9* ip = GetCOREInterface9();
	TimeValue t = ip->GetTime();

	// for this to work, we will need to find the CATParent in the new nodes, and replace the old one with the new one.
	// Select the new one, and delete the old one.
	CATParent* existing_catparent = NULL;
	CATParent* loaded_catparent = NULL;

	// Get the old CATParent object.
	// This method is being called form the CATPArentTrans,
	// which is not allowed to keep a pointer to the object,
	// so we need to find the catparent.
	existing_catparent = static_cast<CATParent*>(GetCATParent());
	DbgAssert(existing_catparent);

	g_bLoadingCAT3Rig = TRUE;
	BOOL scene_redraw_disabled = ip->IsSceneRedrawDisabled();
	if (!scene_redraw_disabled) ip->DisableSceneRedraw();
	theHold.Suspend();

	CATParentCreateCallBack* pCATCreateCB = (CATParentCreateCallBack*)GetCATParent()->GetCreateMouseCallBack();
	INodeTab dAllMergedNodes;
	__TRY
	{
		// Gather all rig nodes first, before the Merge changes all our stored handles

		/////////////////////////////////////////////////////////////////////////
		// Collect all the nodes from the existing rig. These nodes will be deleted
		INodeTab existing_nodes;
		AddSystemNodes(existing_nodes, kSNCDelete);

		// NOTE: When we merge, Max very kindly kills our UI, calling EndEditParams on CATControl (if posted)
		// However, in the remapping, we remap all references to the new rig, including the UI references.
		// We Cannot select the nodes on load, so we save the number of pre-load root nodes, then select all
		// the new ones after load is complete
		INode* sceneRoot = GetCOREInterface()->GetRootNode();
		int iNumRootNodesPreLoad = sceneRoot->NumberOfChildren();
		BOOL mergeAll = TRUE; BOOL selMerged = FALSE; BOOL refresh = FALSE;
		if (ip->MergeFromFile(file, mergeAll, selMerged, refresh, MERGE_DUPS_MERGE, NULL, MERGE_DUP_MTL_USE_MERGED, MERGE_REPARENT_ALWAYS) != 0)
		{
			HoldSuspend hs;
			// Find all the recently added nodes.  We will compare the nodes actually added
			// with the nodes marked by the loaded CATParentTrans and
			AddAllNewNodesToTab(iNumRootNodesPreLoad, dAllMergedNodes);
			loaded_catparent = FindMergedCATParent(dAllMergedNodes);

			DbgAssert(loaded_catparent);
			CATParentTrans* existing_cptrans = this;
			CATParentTrans* loaded_cptrans = static_cast<CATParentTrans*>(loaded_catparent->GetCATParentTrans());
			DbgAssert(loaded_cptrans != NULL);

			if (pProgressWindow) pProgressWindow->SetPercent(50);
			stage_reached++;

			// If any of these pointers don't exist, just throw.  CATCH will try and clean up.
			INode *existing_catparent_node = GetNode();
			INode *catrig_node = loaded_cptrans->GetNode();
			pNewCATParent = catrig_node;

			// Now Transfer everything to the new rig.
			bool bRigExists = false;
			if (!pCATCreateCB->IsCreating())
			{
				int i;

				// Set the current length axis on the existing parent to match
				// the newly loaded one.  We do this so that the CATParentTrans
				// modifies its transform to be equivalent in the new axis system
				// If we didn't do this, when we transfer over the controller later
				// it would not be setup correctly for the loaded CATParentTrans
				// axis system (if they are different)
				loaded_cptrans->SetLengthAxis(existing_cptrans->GetLengthAxis());

				CATClipRoot* existing_layerroot = static_cast<CATClipRoot*>(existing_cptrans->GetLayerRoot());
				CATClipRoot* loaded_layerroot = static_cast<CATClipRoot*>(loaded_cptrans->GetLayerRoot());

				/////////////////////////////////////////////////////////////////////////
				// Build a mapping between from the existing rig to the Newly loaded rig
				// Now for any part of the existing rig we can get the corresponding part on the loaded rig
				CATCharacterRemap remap;
				loaded_cptrans->BuildMapping(existing_cptrans, remap, TRUE);

				// Here we do a safety check, the loaded file might have some nodes
				// included that are not marked as part of the rig.  Here we add
				// them to the remap, and if we can find a matching node in the scene
				// we'll add that to the existing nodes (so we don't mess up the remap).
				INodeTab dMarkedRG3Nodes;
				loaded_cptrans->GetSystemNodes(dMarkedRG3Nodes, kSNCFileSave);
				CleanUpMissingNodePointers(remap, existing_nodes, dMarkedRG3Nodes, dAllMergedNodes, static_cast<CATParentTrans*>(loaded_cptrans));

				/////////////////////////////////////////////////////////////////////////
				// Now transfer all the layers across to the new rig.
				// This is only relevant if the  old rig had layers

				if (!pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS), GetString(IDS_PROG_XFER)))
					pProgressWindow = NULL;
				stage_reached++;

				int numlayers = existing_layerroot->NumLayers();
				for (i = 0; i < numlayers; i++)
				{
					existing_layerroot->CopyLayer(i);
					loaded_layerroot->PasteLayer(TRUE, TRUE, i, &remap);

					float percent = 1.0f - ((float)i / (float)numlayers);
					pProgressWindow->SetPercent((int)(percent * 100.0f));
				}

				/////////////////////////////////////////////////////////////////////////
				// Now fix up all the parenting. Any nodes in the existing rig that are
				// parented to objects that are not part of the rig( Vehicles, chars etc..)
				// We want our new rig nodes to be parented to the same objects.

				if (!pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS), GetString(IDS_PROG_FIXING)))
					pProgressWindow = NULL;
				stage_reached++;

				for (i = 0; i < existing_nodes.Count(); i++)
				{
					INode* pExistingNode = (INode*)existing_nodes[i];
					INode* pLoadedNode = static_cast<INode*>(remap.FindMapping(pExistingNode));
					if (pLoadedNode == NULL || pExistingNode == NULL)
						continue;

					INode* pExistingParent = pExistingNode->GetParentNode();

					// If our node has been reparent'ed outside of the rig,
					// we need to reparent the remapped node to the same parent
					if (pExistingParent != NULL && !pExistingParent->IsRootNode())
					{
						if (existing_nodes.IndexOf(pExistingParent) < 0)
						{
							// Our existing parent is not not part of the rig.
							// We want the loaded node to have the same relationship.
							pExistingParent->AttachChild(pLoadedNode, FALSE);

							// An open question here - what do we do with the loaded parent, if it exists?
						}
					}

					// Any nodes in the existing rig that have children that are not part of the rig,
					// we need to re-parent them to the new rig
					for (int j = 0; j < pExistingNode->NumberOfChildren(); j++)
					{
						INode *pExistingChild = pExistingNode->GetChildNode(j);
						if (existing_nodes.IndexOf(pExistingChild) < 0)
							pLoadedNode->AttachChild(pExistingChild, FALSE);
					}

					// Update our progress bar.
					float percent = (100.0f * i) / existing_nodes.Count();
					pProgressWindow->SetPercent((int)percent);
				}

				/////////////////////////////////////////////////////////////////////////
				// Make sure the new catparent is in the same state as the old one.
				loaded_cptrans->SetCATMode(existing_cptrans->GetCATMode());
				loaded_cptrans->SetColourMode(existing_cptrans->GetColourMode());
				loaded_catparent->SetReloadRigOnSceneLoad(existing_catparent->GetReloadRigOnSceneLoad());

				// Transfer references from the existing CATParent to the new one.

				// We now want to make sure that anything that was referencing the old rig is now referencing the new rig.
				// Examples of this include constraints that were constrained to the rig that we are now replacing.
				// Even scripts are now handled because script controllers make real references to objects in the scene.
				TransferAlmostAllReferences(existing_cptrans, remap);

				// The transfer should have handled selection sets, but because Selection Sets don't implement
				// references correctly, I'll manually remap them here.
				INodeTab selNodes;
				ip->GetSelNodeTab(selNodes);
				for (int i = 0; i < selNodes.Count(); i++)
					selNodes[i] = static_cast<INode*>(remap.FindMapping(selNodes[i]));
				{
					// Lock the selection set to prevent notification
					// messages removing the UI.  Doing so deletes the
					// CustButton class which is at the root of our fn stack
					MaxReferenceMsgLock lock;
					ip->SelectNodeTab(selNodes, 1, FALSE);
				}
			}
			else
			{
				existing_nodes.AppendNode(existing_catparent_node);

				// If the original is selected, select the new one.
				if (existing_catparent_node->Selected())
				{
					// Select the new node
					MaxReferenceMsgLock lock;
					ip->SelectNode(pNewCATParent, FALSE);
				}
			}
			/////////////////////////////////////////////////////////////////////////

			if (pProgressWindow && !pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS),GetString(IDS_PROG_REMAP)))
				pProgressWindow = NULL;
			if (pProgressWindow) pProgressWindow->SetPercent(50);
			stage_reached++;

			// Keep the PRS controller because it defines where we are in the scene.
			loaded_cptrans->ReplaceReference(CATParentTrans::PRS, existing_cptrans->GetReference(CATParentTrans::PRS));
			if (pCATCreateCB->IsCreating()) {
				Matrix3 rigpos = tm ? *tm : Matrix3(1);
				loaded_cptrans->GetNode()->SetNodeTM(ip->GetTime(), rigpos);

				pCATCreateCB->SetObj(loaded_catparent);
				pCATCreateCB->SetDefaultCATUnits(loaded_cptrans->GetCATUnits());
			}
			// Make sure the UI's point to the new controllers
			loaded_catparent->SetOwnsUI();

			///////////////////////////////////////////////////////////////////////////////
			// Now delete the old rig .
			if (pProgressWindow && !pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS),GetString(IDS_PROG_CLEAN)))
				pProgressWindow = NULL;
			if (pProgressWindow) pProgressWindow->SetPercent(70);
			stage_reached++;

			// We let Max expand out the systems to get all the nodes and correctly delete the old rig.
			// Simply ask max to delete the catparent, and it should delete everything else automaticly
			ip->DeleteNodes(existing_nodes, false, false, true);

			///////////////////////////////////////////////////////////////////////////////
			// Update and refresh the new CATParent

			// for some reason deleting the node breaks the CreationCallback
			pCATCreateCB->SetNotCreating();

			// Refresh the CATUnits setting
			loaded_cptrans->SetCATUnits(loaded_cptrans->GetCATUnits());
			loaded_cptrans->UpdateCharacter();
			loaded_cptrans->NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED, NOTIFY_ALL, FALSE);

			// Save the date and time that the rig file was created on.
			// If the file gets modified, then we will know and can reload it.
			loaded_catparent->SetRigFilename(file);
			TSTR rigpresetfile_modifiedtime;
			GetFileModifedTime(file, rigpresetfile_modifiedtime);
			loaded_catparent->SetRigFileModifiedTime(rigpresetfile_modifiedtime);
		}
	}
		__CATCH(...)
	{
		if (!pCATCreateCB->IsCreating())
		{
			::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_RIGUPDATE), _T("If you see this, something is not working")/*GetString(IDS_ERR_RIGUPDATE2)*/, MB_OK);
		}
		else
		{
			::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_RIGLOAD), GetString(IDS_ERR_RIGLOAD2), MB_OK);
		}

		// Attempt to cleanup by removing any loaded nodes
		__TRY
		{
			ip->DeleteNodes(dAllMergedNodes);
		}
			__CATCH(...) {}
	}

	g_bLoadingCAT3Rig = FALSE;
	theHold.Resume();
	theHold.End();

	// We cannot continue to display ourselves in the Create panel, as the
	// create mode is cancelled when we load.
	ip->SetCommandPanelTaskMode(TASK_MODE_MODIFY);

	// A merge will always flush the undo buffer on us, loading is not undoable.
	ip->FlushUndoBuffer();

	if (!scene_redraw_disabled) ip->EnableSceneRedraw();

	if (pProgressWindow) pProgressWindow->SetPercent(100);
	pProgressWindow->ProgressEnd();

	ip->RedrawViews(ip->GetTime());
	return pNewCATParent;
}

/**********************************************************************
* Loading and saving CAT Rigs.
*
* The LoadRig() function acts in the same way as the CATObject
* version.  This function gets called every time ILoadConfig()
* doesn't know what to do with a clause it has just read.  This
* is most of the time, since ILoadConfig() is just a wrapper
* and just handles the very outer-most details of loading a rig,
* such as initialising the loader, handling undo and putting
* up a pretty dialog when we're finished to display what undoubtedly
* will be heaps of errors (until I get the parser working).
*/

BOOL CATParentTrans::AddHub()
{
	if (!mRootHub)
	{
		HoldActions hold(IDS_HLD_ROOTHUBCREATE);

		Hub* pNewHub = (Hub*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HUB_CLASS_ID);
		pNewHub->Initialise(this, FALSE, NULL);
		SetRootHub(pNewHub);

		return TRUE;
	}
	return FALSE;
}

void CATParentTrans::UnRegisterCallbacks()
{
	UnRegisterNotification(ExportStartCallback, this, NOTIFY_PRE_EXPORT);		// PT: Added 14/02/06
	UnRegisterNotification(ExportEndCallback, this, NOTIFY_POST_EXPORT);		// PT: Added 14/02/06

	UnRegisterNotification(PostFileOpenCallback, this, NOTIFY_FILE_POST_OPEN);	// PT: Added 02/08/07
	UnRegisterNotification(PreFileResetCallback, this, NOTIFY_SYSTEM_PRE_RESET);
}

void	CATParentTrans::EvaluateCharacter(TimeValue)
{
	// Tell all the bones inthe rig to evaluate.
	// evalmode = FBIK_GENERATING_TMBONEWORLD
	TimeValue t = GetCOREInterface()->GetTime();
	CATMessage(t, CAT_EVALUATE_BONES);

	// Find Pinned Bones
	Tab <CATNodeControl*> tabPinnedBones;

	// Iterate pulling neighbours towards pinned bones

	// Invalidate caches.
	CATMessage(t, CAT_INVALIDATE_TM);

	// Tell all the bones in the rig to re-evaluate.
	// evalmode = FBIK_UPDATING_INODE_CACHES
	CATMessage(t, CAT_EVALUATE_BONES);
}

///////////////////////////////////////////////////////////////
// These functions were moved from CATPArent.cpp for CAT3.0

Matrix3 CATParentTrans::ApproxCharacterTransform(TimeValue t)
{
	if (GetCATMode() == SETUPMODE)	return GettmCATParent(t);
	if (mRootHub) return mRootHub->ApproxCharacterTransform(t);
	return Matrix3(1);
}

void CATParentTrans::SetLengthAxis(int a)
{
	if (!node) return;
	if ((this->lengthaxis == a) || (a != X && a != Z)) return;
	this->lengthaxis = a;
	// we need to tell the CATParent about the length axis chagne so its build mesh builds the corect mesh
	GetCATParent()->CATMessage(CAT_SET_LENGTH_AXIS, lengthaxis);

	TimeValue t = GetCOREInterface()->GetTime();
	Matrix3 tmAxis = GettmCATParent(t);
	SuspendAnimate();
	AnimateOff();
	if (lengthaxis == X)	node->Rotate(t, tmAxis, Quat(RotateYMatrix(-HALFPI)), 1, 0, 0, 1);
	else				node->Rotate(t, tmAxis, Quat(RotateYMatrix(HALFPI)), 1, 0, 0, 1);

	CATMessage(t, CAT_SET_LENGTH_AXIS, lengthaxis);
	CATMessage(t, CAT_CATUNITS_CHANGED);
	GetCOREInterface()->RedrawViews(t);

	ResumeAnimate();
}

TSTR	CATParentTrans::GetBoneAddress()
{
	TSTR rigname(IdentName(idCATParent));
	return rigname;
};

INode*	CATParentTrans::GetBoneByAddress(const MSTR& address)
{
	// If we get passed an empty address then we should return the root node
	if (address.Length() == 0) return GetCOREInterface()->GetRootNode();

	int boneid;
	USHORT rig_id;
	// we should always start with CATParent
	MSTR nextAddress = address;
	rig_id = GetNextRigID(nextAddress, boneid);

	if (rig_id != idCATParent && rig_id != idSceneRootNode) return NULL;

	if (address.Length() == 0 && rig_id == idSceneRootNode)	return NULL;
	if (address.Length() == 0 && rig_id == idCATParent)		return GetNode();

	rig_id = GetNextRigID(nextAddress, boneid);
	if (rig_id == idHub && mRootHub) return mRootHub->GetBoneByAddress(nextAddress);
	// We didn't find a bone that matched the address given
	// this may be because this rigs structure is different
	// from that of the rig that generated this address
	return NULL;
};

void	CATParentTrans::UpdateUserProps()
{
	TimeValue t = GetCOREInterface()->GetTime();
	CATMessage(t, CAT_UPDATE_USER_PROPS);
};

//////////////////////////////////////////////////////////////////////////
// Utility functions

void FindNextKeyForThisController(Tab < TimeValue > keytimes, int &keyid, TimeValue &nextkeyt, TimeValue &t)
{
	for (int j = 0; j < keytimes.Count(); j++) {
		if (keytimes[j] > t && keytimes[j] <= nextkeyt) {
			nextkeyt = keytimes[j];
			keyid = j;
			return;
		}
	}
}

void FindNextKeyTime(Tab< Tab < TimeValue > > poskeytimes, Tab< Tab < TimeValue > > rotkeytimes, Tab< Tab < TimeValue > > sclkeytimes, Tab< Tab < TimeValue > > keytimes, \
	Tab<int> &poskeyid, Tab<int> &rotkeyid, Tab<int> &sclkeyid, Tab<int> &keyid, \
	TimeValue start_t, TimeValue end_t, TimeValue &t)
{
	UNREFERENCED_PARAMETER(start_t);
	TimeValue nextpkeyt, nextrkeyt, nextskeyt, nextkeyt;
	nextpkeyt = nextrkeyt = nextskeyt = nextkeyt = end_t;
	for (int i = 0; i < poskeytimes.Count(); i++) {
		FindNextKeyForThisController(poskeytimes[i], poskeyid[i], nextpkeyt, t);
		FindNextKeyForThisController(rotkeytimes[i], rotkeyid[i], nextrkeyt, t);
		FindNextKeyForThisController(sclkeytimes[i], sclkeyid[i], nextskeyt, t);
		FindNextKeyForThisController(keytimes[i], keyid[i], nextkeyt, t);
	}

	t = min(nextpkeyt, min(nextrkeyt, min(nextskeyt, nextkeyt)));
}

void FindMidKeys(Tab < TimeValue > &keytimes, float ratio)
{
	UNREFERENCED_PARAMETER(ratio);
	Tab < TimeValue > midkeytimes;
	midkeytimes.Init();
	TimeValue t1;
	TimeValue t2;
	float delta;
	for (int j = 1; j < keytimes.Count(); j++) {
		if ((keytimes[j] - keytimes[j - 1]) >= (GetTicksPerFrame() * 3)) {

			delta = keytimes[j - 1] + ((keytimes[j] - keytimes[j - 1]) * 0.33f);
			t1 = (TimeValue)(floor(delta) / GetTicksPerFrame()) * GetTicksPerFrame();
			if ((t1 - keytimes[j - 1]) < GetTicksPerFrame()) t1 += GetTicksPerFrame();
			midkeytimes.Append(1, &t1);

			delta = keytimes[j - 1] + ((keytimes[j] - keytimes[j - 1]) * 0.66f);
			t2 = (TimeValue)(floor(delta) / GetTicksPerFrame()) * GetTicksPerFrame();
			if ((keytimes[j] - t2) < GetTicksPerFrame()) t2 -= GetTicksPerFrame();

			if (t1 != t2) midkeytimes.Append(1, &t2);
		}
	}
	keytimes = midkeytimes;
}

void KeyRotation(CATClipValue *layerctrl, TimeValue prevkey, TimeValue newt)
{
	if (!layerctrl->GetRotationController()) return;
	AngAxis ax, totalax(0.0f, 0.0f, 0.0f, 0.0f);
	Matrix3 prevtm, tm;
	Matrix3 tmAxis(1);
	CATNodeControl* catnodecontrol = NULL;

	int keystep = 4;
	DbgAssert(FALSE);	// TODO - Replace this with KeyFreeform
	// Some kinds of bones are really difficult to plot using the tmBone world.
	// Using the local values is much more likely to work. We sill simply use the Ankles Local value to plot its transform.
	CATControl *catcontrol = NULL; //layerctrl->GetCATControl();
	if (catcontrol && catcontrol->ClassID() != PALMTRANS2_CLASS_ID)
		catnodecontrol = (CATNodeControl*)catcontrol->GetInterface(I_CATNODECONTROL);

	if (catcontrol && catcontrol->ClassID() != PALMTRANS2_CLASS_ID && catnodecontrol)
	{

		for (TimeValue t = (prevkey + (GetTicksPerFrame()*keystep)); t <= newt; t += GetTicksPerFrame()*keystep) {
			GetCOREInterface()->SetTime(t, TRUE);
			tm = catnodecontrol->GetNode()->GetNodeTM(t);
			catnodecontrol->KeyFreeform(t, KEY_ROTATION);

			prevtm = tm;
		}

		GetCOREInterface()->SetTime(newt, TRUE);
		tm = catnodecontrol->GetNode()->GetNodeTM(newt);
		catnodecontrol->KeyFreeform(newt, KEY_ROTATION);
	}
	else {

		SetXFormPacket ptr;
		ptr.command = XFORM_ROTATE;
		ptr.tmParent = Matrix3(1);
		Interval iv;
		for (TimeValue t = prevkey; t < newt; t += GetTicksPerFrame() * 2)
		{
			Matrix3 tm(1);
			iv = FOREVER;
			layerctrl->GetValue(t, (void*)&tm, iv, CTRL_RELATIVE);
			// this is not strictly correct. We should be passing in an AngleAxis or a quat,
			// but I can't get the SetValue to Set an absolute rotation. I know Keying a layer has always worked
			// So here I pass in a full transform and CATClipMatrix3::SetValue will generate the correct rotation
			ptr.tmAxis = tm;
			layerctrl->SetValue(t, (void*)&ptr, 1, CTRL_ABSOLUTE);
		}
		Matrix3 tm(1);
		iv = FOREVER;
		layerctrl->GetValue(newt, (void*)&tm, iv, CTRL_RELATIVE);
		layerctrl->SetValue(newt, (void*)&ptr, 1, CTRL_ABSOLUTE);
	}

	//	layerctrl->GetRotationController()->DeleteTime (  Interval(prevkey+2, newt-2),  TIME_NOSLIDE);
	for (TimeValue t = (prevkey + (GetTicksPerFrame()*keystep)); t < newt; t += GetTicksPerFrame()*keystep) {
		layerctrl->GetRotationController()->DeleteKeyAtTime(t);
	}

}

class FindCATNodeControlEnum : public DependentEnumProc
{
public:
	CATClipValue* mpTarget;
	CATNodeControl* mpOwner;

	FindCATNodeControlEnum(CATClipValue* pVal)
		: mpTarget(pVal)
		, mpOwner(NULL)
	{ }

	int proc(ReferenceMaker *rmaker)
	{
		CATNodeControl* pCATNodeCtrl = dynamic_cast<CATNodeControl*>(rmaker);
		if (pCATNodeCtrl != NULL)
		{
			// Double-check; is the CATNodeControl applied here?
			int nClipCtrls = pCATNodeCtrl->NumLayerControllers();
			for (int i = 0; i < nClipCtrls; i++)
			{
				if (pCATNodeCtrl->GetLayerController(i) == mpTarget)
				{
					mpOwner = pCATNodeCtrl;
					return DEP_ENUM_HALT;
				}
			}
		}
		// Else, if we have hit a node, we are not interested in this branch
		else if (rmaker->SuperClassID() == BASENODE_CLASS_ID)
			return DEP_ENUM_SKIP;

		// else keep looking;
		return DEP_ENUM_CONTINUE;
	}
};

// Given a layer controller, find the CATNodeControl that references it
CATNodeControl* FindCATNodeControl(CATClipValue* pCATClipValue)
{
	if (pCATClipValue == NULL)
		return NULL;

	FindCATNodeControlEnum findMyCtrl(pCATClipValue);
	pCATClipValue->DoEnumDependents(&findMyCtrl);
	return findMyCtrl.mpOwner;
}

// CAT stores a lot of pointers in pblocks marked P_NO_REF (to allow circular links).
// This class transfers any of these pointers in the scene from the old target to the new.
class TransferUnReferencedPtrsIter : public Animatable::EnumAnimList
{
private:
	CATCharacterRemap*	mpRemap;
	ICATParentTrans*		mpOldParent;
	TransferUnReferencedPtrsIter(const TransferUnReferencedPtrsIter&);

public:
	TransferUnReferencedPtrsIter(CATCharacterRemap &remap, ICATParentTrans* pOldParent)
		: mpRemap(&remap)
		, mpOldParent(pOldParent)
	{
	}

	virtual	bool proc(Animatable *theAnim)
	{
		// We are only interested in updating unref'ed pointers in parameter blocks
		if (theAnim == NULL || theAnim->SuperClassID() != PARAMETER_BLOCK2_CLASS_ID)
			return true;

		IParamBlock2* pParamBlock = static_cast<IParamBlock2*>(theAnim);

		// Skip any items that were part of the old rig
		CATControl* pOwner = dynamic_cast<CATControl*>(pParamBlock->GetOwner());
		if (pOwner != NULL)
		{
			if (pOwner->GetCATParentTrans(false) == mpOldParent)
				return true;
		}

		// Skip any items that are part of the clone!
		if (mpRemap->FindMapping((ReferenceTarget*)pParamBlock->GetOwner()) != NULL)
			return true;

		// All references are handled, but we may need
		// to update any pointers stored in P_NO_REF
		// parameters.
		int nParams = pParamBlock->NumParams();
		for (int i = 0; i < nParams; i++)
		{
			ParamID pid = pParamBlock->IndextoID(i);
			ParamDef& paramDef = pParamBlock->GetParamDef(pid);
			if ((paramDef.flags&P_NO_REF) && reftarg_type(paramDef.type))
			{
				// This pointer may not have been updated by the reference transfer bits
				if (is_tab(paramDef.type)) {
					// Check the entire list for the problematic pointer
					int nItems = pParamBlock->Count(pid);
					for (int j = 0; j < nItems; j++)
					{
						// Update to point at mpNewTarget, if one was specified
						ReferenceTarget* pRetarget = mpRemap->FindMapping(pParamBlock->GetReferenceTarget(pid, 0, j));
						if (pRetarget != NULL)
						{
							pParamBlock->SetValue(pid, 0, pRetarget, j);
						}
					}
				}
				else
				{
					// Update to point at mpNewTarget, if one was specified
					ReferenceTarget* pRetarget = mpRemap->FindMapping(pParamBlock->GetReferenceTarget(pid));
					if (pRetarget != NULL)
					{
						pParamBlock->SetValue(pid, 0, pRetarget);
					}
				}
			}
		}

		return true;
	}
};

// Transfer all the references from the original CAT rig to the new one.
// The remap structure should contain the linkage between the old structure
// and the new one.
RefResult TransferAlmostAllReferences(ICATParentTrans* pOldParent, CATCharacterRemap& remap)
{
	Tab<RefMakerHandle> makers;

	ReferenceMapping::iterator itr = remap.begin();
	ReferenceMapping::iterator end = remap.end();
	for (; itr != end; itr++)
	{
		// Iterate the classes that reference the old target, and move their
		// references to the new target.
		ReferenceMaker  *maker;
		DependentIterator di(itr->first);
		while ((maker = di.Next()) != NULL)
		{
			// Skip any classes that are part of the original rig
			if (remap.FindMapping((ReferenceTarget*)maker) != NULL)
				continue;
			else if (maker->ClassID() == Class_ID(PARAMETER_BLOCK2_CLASS_ID, 0))
			{
				// If its a parameter block, test the owner of the class (they are not remapped)
				IParamBlock2* pblock = static_cast<IParamBlock2*>(maker);
				CATControl* pOwningCtrl = dynamic_cast<CATControl*>(pblock->GetOwner());
				if (pOwningCtrl != NULL && pOwningCtrl->GetCATParentTrans() == pOldParent)
				{
					continue;
				}
			}

			// This reference needs to be transfered:
			int j = maker->FindRef(itr->first);

			// [dl | 6june2003] I commented out this DbgAssert and added an 'if' instead.
			// It's possible that the reference will not be found, since the call to
			// "makr->ReplaceReference()", in a previous iteration of the 'for' loop,
			// may have affected the reference hierarchy.
			//DbgAssert(j>=0);
			if (j >= 0) {
				if (!maker->CanTransferReference(j))
					continue;
				maker->ReplaceReference(j, itr->second);
			}
		}
	}

	// Update non-reference pointers
	TransferUnReferencedPtrsIter iter(remap, pOldParent);
	Animatable::EnumerateAllAnimatables(iter);

	return REF_SUCCEED;
}

class CleanDeadCPTransRigElements : public Animatable::EnumAnimList
{
private:
	CATParentTrans* mpDeadPtr;

public:

	CleanDeadCPTransRigElements(CATParentTrans* pDeadPtr)
		: mpDeadPtr(pDeadPtr)
	{	}

	virtual	bool proc(Animatable *theAnim)
	{
		CATControl* pCATCtrl = dynamic_cast<CATControl*>(theAnim);
		if (pCATCtrl != NULL)
		{
			__TRY{
				// Clean the pointer on the CATNodeControl
				if (pCATCtrl->GetCATParentTrans(false) == mpDeadPtr)
				{
					pCATCtrl->SetCATParentTrans(NULL);

					CATNodeControl* pCATNodeCtrl = dynamic_cast<CATNodeControl*>(pCATCtrl);
					if (pCATNodeCtrl != NULL)
					{
						// Remove all arb controller pointers.  This is necessary, because there
						// is no other way at this point for the arb controller to let its CATNodeControl
						// know when it is deleted
						int nArbCtrl = pCATNodeCtrl->NumLayerFloats();
						for (int i = nArbCtrl - 1; i >= 0; --i)
							pCATNodeCtrl->RemoveLayerFloat(i);
					}
				}
			} __CATCH(...) {}
		}
		else
		{
			CATClipValue *pValue = dynamic_cast<CATClipValue*>(theAnim);
			if (pValue != NULL)
			{
				__TRY{
					// Clean the pointer on any CATClipValues
					if (pValue->GetCATParentTrans() == mpDeadPtr)
						pValue->SetCATParentTrans(NULL);
				} __CATCH(...) {}
			}
		}

		return true;
	}

};

void CleanCATParentTransPointer(CATParentTrans* pDeadCATParentTrans)
{
	CleanDeadCPTransRigElements cleaner(pDeadCATParentTrans);
	Animatable::EnumerateAllAnimatables(cleaner);
}

/////////////////////////////////////////////////////////////////////////////////
// HIK Stuff

void FillDefDigitSegs(DigitData* pDigit, INode* defStore[3])
{
	DbgAssert(pDigit != NULL);

	int nDigitSegs = min(pDigit->GetNumBones(), 3);
	for (int i = 0; i < nDigitSegs; i++)
	{
		CATNodeControl* pSeg = (CATNodeControl*)pDigit->GetBone(i);
		if (pSeg != NULL)
			defStore[i] = pSeg->GetNode();
	}
}

void FillDefDigit(PalmTrans2* pPalm, INode* defStore[5][3])
{
	DbgAssert(pPalm != NULL);

	int nDigits = min(pPalm->GetNumDigits(), 5);
	for (int i = 0; i < nDigits; i++)
	{
		DigitData* pDigit = pPalm->GetDigit(i);
		if (pDigit != NULL)
			FillDefDigitSegs(pDigit, defStore[i]);
	}
}

void FillDefLeftLeg(LimbData2* pLimb, HIKDefinition& def)
{
	DbgAssert(pLimb != NULL);
	CATNodeControl* pBone0 = pLimb->GetBoneData(0);
	if (pBone0 != NULL)
	{
		def.mLeftThigh = pBone0->GetNode();
		CATNodeControl* pBone1 = pLimb->GetBoneData(1);
		if (pBone1 != NULL)
			def.mLeftCalf = pBone1->GetNode();
	}
	PalmTrans2* pAnkle = pLimb->GetPalmTrans();
	if (pAnkle != NULL)
	{
		def.mLeftAnkle = pAnkle->GetNode();
		FillDefDigit(pAnkle, def.mLeftToes);
	}
}

void FillDefRightLeg(LimbData2* pLimb, HIKDefinition& def)
{
	DbgAssert(pLimb != NULL);
	CATNodeControl* pBone0 = pLimb->GetBoneData(0);
	if (pBone0 != NULL)
	{
		def.mRightThigh = pBone0->GetNode();
		CATNodeControl* pBone1 = pLimb->GetBoneData(1);
		if (pBone1 != NULL)
			def.mRightCalf = pBone1->GetNode();
	}
	PalmTrans2* pAnkle = pLimb->GetPalmTrans();
	if (pAnkle != NULL)
	{
		def.mRightAnkle = pAnkle->GetNode();
		FillDefDigit(pAnkle, def.mRightToes);
	}
}

void FillDefLeftArm(LimbData2* pLimb, HIKDefinition& def)
{
	DbgAssert(pLimb != NULL);

	// Limb bones (should be 2 at least).
	CATNodeControl* pBone0 = pLimb->GetBoneData(0);
	if (pBone0 != NULL)
	{
		def.mLeftUpperArm = pBone0->GetNode();
		CATNodeControl* pBone1 = pLimb->GetBoneData(1);
		if (pBone1 != NULL)
			def.mLeftForearm = pBone1->GetNode();
	}

	// collarbone (if present)
	CATNodeControl* pCollarTrans = pLimb->GetCollarbone();
	if (pCollarTrans != NULL)
		def.mLeftCollar = pCollarTrans->GetNode();

	// Palm, if present
	PalmTrans2* pAnkle = pLimb->GetPalmTrans();
	if (pAnkle != NULL)
	{
		def.mLeftPalm = pAnkle->GetNode();
		FillDefDigit(pAnkle, def.mLeftFingers);
	}
}

void FillDefRightArm(LimbData2* pLimb, HIKDefinition& def)
{
	DbgAssert(pLimb != NULL);
	CATNodeControl* pBone0 = pLimb->GetBoneData(0);
	if (pBone0 != NULL)
	{
		def.mRightUpperArm = pBone0->GetNode();
		CATNodeControl* pBone1 = pLimb->GetBoneData(1);
		if (pBone1 != NULL)
			def.mRightForearm = pBone1->GetNode();
	}

	// collarbone (if present)
	CATNodeControl* pCollarTrans = pLimb->GetCollarbone();
	if (pCollarTrans != NULL)
		def.mRightCollar = pCollarTrans->GetNode();

	PalmTrans2* pAnkle = pLimb->GetPalmTrans();
	if (pAnkle != NULL)
	{
		def.mRightPalm = pAnkle->GetNode();
		FillDefDigit(pAnkle, def.mRightFingers);
	}
}

// I cannot believe our string class does not have this fn?!?!?
bool Search(const MSTR& inString, const MSTR& substr)
{
	int subLen = substr.Length();
	int origLen = inString.Length();

	// Todo: (Make someone else) implement an efficient version of this fn!
	for (int i = 0; i <= origLen - subLen; i++)
	{
		if (inString.Substr(i, subLen) == substr)
			return true;
	}
	return false;
}
bool CATParentTrans::FillHIKDefinition(HIKDefinition& def)
{
	// NULL all pointers.
	memset(&def, 0, sizeof(def));
	Hub* pRootHub = mRootHub;
	if (pRootHub == NULL)
		return FALSE;

	def.mPelvis = pRootHub->GetNode();

	// Fill out legs.
	for (int i = 0; i < pRootHub->GetNumLimbs(); i++)
	{
		LimbData2* pLimb = pRootHub->GetLimb(i);
		if (pLimb == NULL)
			return FALSE;

		if (pLimb->GetisLeg())
		{
			if (pLimb->GetLMR() == -1)
				FillDefLeftLeg(pLimb, def);
			else
				FillDefRightLeg(pLimb, def);
		}
	}

	SpineData2* pSpine = pRootHub->GetSpine(0);
	// If we don't have a spine, we only return FALSE if
	// we are actually supposed to have a spine.
	if (pSpine == NULL)
		return pRootHub->GetNumSpines() == 0;

	for (int i = 0; i < MAX_CAT_SPINE_BONES; i++)
	{
		INodeControl* pBone = pSpine->GetBoneINodeControl(i);
		if (pBone != NULL)
			def.mSpine[i] = pBone->GetNode();
	}

	Hub* pRibcage = pSpine->GetTipHub();
	// Ribcage is required if we have a spine.
	if (pRibcage == NULL)
		return false;
	def.mRibcage = pRibcage->GetNode();

	// How many actual arms do we have
	int nArms = pRibcage->GetNumLimbs();
	LimbData2* pLeftArm = NULL;
	LimbData2* pRightArm = NULL;

	// If we have too many limbs, perhaps some of them are not
	// actually arms?  We only consider genuine arms here.
	if (nArms > 2)
	{
		// First, don't count any non-arms
		for (int i = 0; i < pRibcage->GetNumLimbs(); i++)
		{
			LimbData2* pLimb = pRibcage->GetLimb(i);
			if (pLimb == NULL)
				return FALSE;

			if (!pLimb->GetisArm())
				nArms--;
		}
	}

	// If we have too many limbs marked as arm's, try name matching to see
	// if we can find the 'most likely' match
	if (nArms > 2)
	{
		static const int nameCount = 2;
		MSTR allAllowedNames[nameCount] = {
			MSTR(_M("arm")),
			MSTR(_M("limb"))
		};

		// Iterate over the namecount first.  This ensures that the
		// names are matched in order of priority: a limb named "arm"
		// will be picked rather than a limb named "Limb"
		for (int j = 0; j < nameCount; j++)
		{
			for (int i = 0; i < pRibcage->GetNumLimbs(); i++)
			{
				LimbData2* pLimb = pRibcage->GetLimb(i);

				if (pLimb == NULL)
					return FALSE;

				if (pLimb->GetisArm())
				{
					MSTR name = pLimb->GetName();
					name.toLower();
					bool isValid = false;
					if (Search(name, allAllowedNames[j]))
						isValid = true;
					else
					{
						// If we didn't get it on the Limb name, we can check
						// the bone name as well, just to double check!
						BoneData* pFirstBone = pLimb->GetBoneData(0);
						if (pFirstBone != NULL)
						{
							name = pFirstBone->GetName();
							name.toLower();
							if (Search(name, allAllowedNames[j]))
								isValid = true;
						}
					}

					// We have a valid name: count this arm.
					if (isValid)
					{
						if (pLimb->GetLMR() == -1)
						{
							if (pLeftArm == NULL)
								pLeftArm = pLimb;
						}
						else
						{
							if (pRightArm == NULL)
								pRightArm = pLimb;
						}
					}
				}
			}
		}
	}

	// If we haven't found enough via name-matching, fill up the rest however we can.
	if (pRightArm == NULL || pLeftArm == NULL)
	{
		for (int i = 0; i < pRibcage->GetNumLimbs(); i++)
		{
			LimbData2* pLimb = pRibcage->GetLimb(i);
			if (pLimb == NULL)
				return FALSE;

			if (pLimb->GetisArm())
			{
				if (pLimb->GetLMR() == -1)
				{
					if (pLeftArm == NULL)
						pLeftArm = pLimb;
				}
				else
				{
					if (pRightArm == NULL)
						pRightArm = pLimb;
				}
			}
		}
	}

	// Now fill out the definition with what we have.
	if (pLeftArm != NULL)
		FillDefLeftArm(pLeftArm, def);
	if (pRightArm != NULL)
		FillDefRightArm(pRightArm, def);

	SpineData2* pNeck = pRibcage->GetSpine(0);
	// If we don't have a spine, we only return FALSE if
	// we are actually supposed to have a spine.
	if (pNeck == NULL)
		return pRibcage->GetNumSpines() == 0;

	for (int i = 0; i < MAX_CAT_SPINE_BONES; i++)
	{
		INodeControl* pBone = pNeck->GetBoneINodeControl(i);
		if (pBone != NULL)
			def.mNeck[i] = pBone->GetNode();
	}

	Hub* pHead = pNeck->GetTipHub();
	// Ribcage is required if we have a spine.
	if (pHead == NULL)
		return false;

	def.mHead = pHead->GetNode();

	return true;
}
