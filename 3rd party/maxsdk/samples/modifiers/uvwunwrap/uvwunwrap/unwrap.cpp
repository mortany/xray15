/*

Copyright 2010 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

/********************************************************************** *<
FILE: unwrap.cpp

DESCRIPTION: A UVW map modifier unwraps the UVWs onto the image

HISTORY: 12/31/96
CREATED BY: Rolf Berteig
UPDATED Sept. 16, 1998 Peter Watje



*>	Copyright (c) 1998, All Rights Reserved.

**********************************************************************/

#include "unwrap.h"
#include "modsres.h"
#include "buildver.h"
#include "modstack.h"
#include <helpsys.h>
#include "iMenuMan.h"
#include "utilityMethods.h"

#include "3dsmaxport.h"

#include "UnwrapRenderItem.h"
#include "UnwrapRenderItemDistortion.h"
#include <algorithm>
#include "MouseCursors.h"
#include <Graphics/IDisplayManager.h>
#include "MeshNormalSpec.h"

static const int NUM_PEEL_MODE_SETTING = 3; // auto-pack/live-peel/edge-to-seam in peel mode dialog

PeltPointToPointMode* UnwrapMod::peltPointToPointMode = NULL;
TweakMode* UnwrapMod::tweakMode = NULL;

class MyEnumProc : public DependentEnumProc
{
public:
	virtual int proc(ReferenceMaker *rmaker);
	INodeTab Nodes;
};

int MyEnumProc::proc(ReferenceMaker *rmaker)
{
	if (rmaker->SuperClassID() == BASENODE_CLASS_ID)
	{
		Nodes.Append(1, (INode **)&rmaker);
		return DEP_ENUM_SKIP;
	}

	return DEP_ENUM_CONTINUE;
}


static void PreSave(void *param, NotifyInfo *info)
{
	UnwrapMod *mod = (UnwrapMod*)param;
	if (mod->CurrentMap == 0)
		mod->ShowCheckerMaterial(FALSE);
}

static void PostSave(void *param, NotifyInfo *info)
{
	UnwrapMod *mod = (UnwrapMod*)param;
	if ((mod->CurrentMap == 0) && (mod->checkerWasShowing))
		mod->ShowCheckerMaterial(TRUE);
}

static int CompTable(const void* item1, const void* item2) {
	const int a = *(const int*)item1;
	const int b = *(const int*)item2;
	
	if (a > b)
		return 1;
	else if (a < b)
		return -1;
	return 0;
}

static GenSubObjType SOT_SelFace(19);
static GenSubObjType SOT_SelVerts(1);
static GenSubObjType SOT_SelEdges(2);
static GenSubObjType SOT_SelGizmo(14);

HCURSOR ZoomRegMode::GetXFormCur()
{
	return mod->zoomRegionCur;
}

HCURSOR ZoomMode::GetXFormCur()
{
	return mod->zoomCur;
}

HCURSOR PanMode::GetXFormCur()
{
	return mod->panCur;
}

HCURSOR RotateMode::GetXFormCur()
{
	return mod->rotCur;
}

HCURSOR WeldMode::GetXFormCur()
{
	return mod->weldCur;
}

HCURSOR MoveMode::GetXFormCur()
{
	if (mod->move == 1)
		return mod->moveXCur;
	else if (mod->move == 2) return mod->moveYCur;
	return mod->moveCur;
}

HCURSOR ScaleMode::GetXFormCur()
{
	if (mod->scale == 1)
		return mod->scaleXCur;
	else if (mod->scale == 2) return mod->scaleYCur;
	return mod->scaleCur;

}

HCURSOR FreeFormMode::GetXFormCur()
{
	if (mod->freeFormSubMode == ID_MOVE)
		return mod->moveCur;
	else if (mod->freeFormSubMode == ID_SCALE)
		return mod->scaleCur;
	else if (mod->freeFormSubMode == ID_ROTATE)
		return mod->rotCur;
	else if (mod->freeFormSubMode == ID_MOVEPIVOT)
		return LoadCursor(NULL, IDC_SIZEALL);

	else return mod->selCur;
}

class UVWUnwrapDeleteEvent : public EventUser {
public:
	UnwrapMod *m;

	void Notify() {
		if (m)
		{
			m->DeleteSelected();
			m->InvalidateView();

		}
	}
	void SetEditMeshMod(UnwrapMod *im) { m = im; }
};

UVWUnwrapDeleteEvent delEvent;

BOOL		UnwrapMod::executedStartUIScript = FALSE;

HWND            UnwrapMod::hOptionshWnd = NULL;
HWND            UnwrapMod::hSelRollup = NULL;
HWND            UnwrapMod::hParams = NULL;
HWND            UnwrapMod::hSelParams = NULL;
HWND		UnwrapMod::hMatIdParams = NULL;
HWND            UnwrapMod::hDialogWnd = NULL;
HWND            UnwrapMod::hView = NULL;
int		UnwrapMod::iToolBarHeight = 30;

HWND UnwrapMod::stitchHWND = NULL;
HWND UnwrapMod::flattenHWND = NULL;
HWND UnwrapMod::unfoldHWND = NULL;
HWND UnwrapMod::sketchHWND = NULL;
HWND UnwrapMod::normalHWND = NULL;
HWND UnwrapMod::packHWND = NULL;
HWND UnwrapMod::relaxHWND = NULL;
HWND UnwrapMod::renderUVWindow = NULL;
HWND UnwrapMod::hTextures = NULL;
HWND UnwrapMod::hMatIDs = NULL;
HWND UnwrapMod::hConfigureParams = NULL;
HWND UnwrapMod::hMapParams = NULL;
HWND UnwrapMod::hEditUVParams = NULL;
HWND UnwrapMod::hPeelParams = NULL;
HWND UnwrapMod::hWrapParams = NULL;

IObjParam      *UnwrapMod::ip = nullptr;

ISpinnerControl *UnwrapMod::iMapID = nullptr;
ISpinnerControl *UnwrapMod::iSetMatIDSpinnerEdit = nullptr;
ISpinnerControl *UnwrapMod::iSelMatIDSpinnerEdit = nullptr;


MouseManager    UnwrapMod::mouseMan;
CopyPasteBuffer UnwrapMod::copyPasteBuffer;
int             UnwrapMod::mode = ID_FREEFORMMODE;
int             UnwrapMod::oldMode = ID_FREEFORMMODE;
int				UnwrapMod::closingMode = ID_FREEFORMMODE;

MoveMode       *UnwrapMod::moveMode = nullptr;
RotateMode     *UnwrapMod::rotMode = nullptr;
ScaleMode      *UnwrapMod::scaleMode = nullptr;
PanMode        *UnwrapMod::panMode = nullptr;
ZoomMode       *UnwrapMod::zoomMode = nullptr;
ZoomRegMode    *UnwrapMod::zoomRegMode = nullptr;
WeldMode       *UnwrapMod::weldMode = nullptr;

//PELT
PeltStraightenMode       *UnwrapMod::peltStraightenMode = nullptr;
RightMouseMode *UnwrapMod::rightMode = nullptr;
MiddleMouseMode *UnwrapMod::middleMode = nullptr;
FreeFormMode   *UnwrapMod::freeFormMode = nullptr;
SketchMode		*UnwrapMod::sketchMode = nullptr;

BOOL            UnwrapMod::viewValid = FALSE;
UnwrapMod      *UnwrapMod::editMod = nullptr;

BOOL			UnwrapMod::floaterWindowActive = FALSE;

UnwrapSelectModBoxCMode *UnwrapMod::selectMode = nullptr;
MoveModBoxCMode   *UnwrapMod::moveGizmoMode = nullptr;
RotateModBoxCMode *UnwrapMod::rotGizmoMode = nullptr;
UScaleModBoxCMode *UnwrapMod::uscaleGizmoMode = nullptr;
NUScaleModBoxCMode *UnwrapMod::nuscaleGizmoMode = nullptr;
SquashModBoxCMode *UnwrapMod::squashGizmoMode = nullptr;


PaintSelectMode *UnwrapMod::paintSelectMode = nullptr;
PaintMoveMode* UnwrapMod::paintMoveMode = nullptr;
RelaxMoveMode* UnwrapMod::relaxMoveMode = nullptr;


COLORREF UnwrapMod::lineColor = RGB(255, 255, 255);
COLORREF UnwrapMod::selColor = RGB(255, 0, 0);
COLORREF UnwrapMod::selPreviewColor = RGB(239, 154, 72);
COLORREF UnwrapMod::openEdgeColor = RGB(0, 255, 0);
COLORREF UnwrapMod::handleColor = RGB(255, 255, 0);
COLORREF UnwrapMod::freeFormColor = RGB(255, 100, 50);
COLORREF UnwrapMod::sharedColor = RGB(0, 0, 255);
COLORREF UnwrapMod::peelColor = RGB(181, 109, 218);
COLORREF UnwrapMod::gridColor = RGB(150, 150, 150);
COLORREF UnwrapMod::backgroundColor = RGB(120, 120, 120);

int UnwrapMod::RenderUVCurUTile = 0;
int UnwrapMod::RenderUVCurVTile = 0;

float UnwrapMod::weldThreshold = 0.01f;
BOOL UnwrapMod::update = FALSE;
int UnwrapMod::showVerts = 0;

BOOL		   UnwrapMod::tileValid = FALSE;

HCURSOR UnwrapMod::selCur = NULL;
HCURSOR UnwrapMod::moveCur = NULL;
HCURSOR UnwrapMod::moveXCur = NULL;
HCURSOR UnwrapMod::moveYCur = NULL;
HCURSOR UnwrapMod::rotCur = NULL;
HCURSOR UnwrapMod::scaleCur = NULL;
HCURSOR UnwrapMod::scaleXCur = NULL;
HCURSOR UnwrapMod::scaleYCur = NULL;

HCURSOR UnwrapMod::circleCur[] = { NULL };

HCURSOR UnwrapMod::zoomCur = NULL;
HCURSOR UnwrapMod::zoomRegionCur = NULL;
HCURSOR UnwrapMod::panCur = NULL;
HCURSOR UnwrapMod::weldCur = NULL;
HCURSOR UnwrapMod::weldCurHit = NULL;
HCURSOR UnwrapMod::sketchCur = NULL;
HCURSOR UnwrapMod::sketchPickCur = NULL;
HCURSOR UnwrapMod::sketchPickHitCur = NULL;
HWND UnwrapMod::hRelaxDialog = NULL;
HWND UnwrapMod::hSnapSettingDialog = NULL;

WINDOWPLACEMENT UnwrapMod::sketchWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::packWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::stitchWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::flattenWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::unfoldWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::normalWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::windowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::relaxWindowPos = { 0 };
WINDOWPLACEMENT UnwrapMod::peltWindowPos = { 0 };

int	UnwrapMod::iOriginalMode = 0;
int UnwrapMod::iOriginalOldMode = -1;
bool UnwrapMod::bCustomizedZoomMode = false;
bool UnwrapMod::bCustomizedPanMode = false;
WPARAM	UnwrapMod::wParam = 0;
LPARAM	UnwrapMod::lParam = 0;

float UnwrapMod::abfErrorBound = 1e-5f;
int UnwrapMod::abfMaxItNum = 20;
BOOL UnwrapMod::showPins = TRUE;
bool UnwrapMod::useSimplifyModel = false;

TSTR UnwrapMod::mToolBarIniFileName = _T("\\UnwrapToolBar.ini");

//--- UnwrapMod methods -----------------------------------------------

UnwrapMod::~UnwrapMod()
{
	UnRegisterNotification(OnModSelChanged, this, NOTIFY_MODPANEL_SEL_CHANGED);
	ClearMeshTopoDataContainer();

	ClearImagesContainer();
	DeleteAllRefsFromMe();
	FreeCancelBuffer();
}

void UnwrapMod::ClearMeshTopoDataContainer()
{
	// unregister mesh topo data listener.
	for (int i = 0; i < mMeshTopoData.Count(); ++i)
	{
		if (mMeshTopoData[i])
		{
			mMeshTopoData[i]->UnRegisterNotification(this);
		}
	}
	mMeshTopoData.SetCount(0);
}

static INT_PTR CALLBACK UnwrapConfigureRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	BOOL processed = mod->GetUIManager()->MessageProc(hWnd, msg, wParam, lParam);
	if (processed)
		return TRUE;
	switch (msg) {
	case WM_INITDIALOG:
	{
		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
		CheckDlgButton(hWnd, IDC_DONOTREFLATTEN_CHECK, mod->fnGetPreventFlattening());

		BOOL thickSeams = mod->fnGetThickOpenEdges();
		BOOL showSeams = mod->fnGetViewportOpenEdges();

		CheckDlgButton(hWnd, IDC_SHOWMAPSEAMS_CHECK, FALSE);
		CheckDlgButton(hWnd, IDC_THINSEAM, FALSE);
		CheckDlgButton(hWnd, IDC_THICKSEAM, FALSE);


		if (thickSeams)
			CheckDlgButton(hWnd, IDC_THICKSEAM, TRUE);
		else CheckDlgButton(hWnd, IDC_THINSEAM, TRUE);

		if (showSeams)
		{
			CheckDlgButton(hWnd, IDC_SHOWMAPSEAMS_CHECK, TRUE);
		}

		CheckDlgButton(hWnd, IDC_NORMALIZEMAP_CHECK2, mod->fnGetNormalizeMap());

		CheckDlgButton(hWnd, IDC_ALWAYSSHOWPELTSEAMS_CHECK, mod->fnGetAlwayShowPeltSeams());
	}
	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDC_ALWAYSSHOWPELTSEAMS_CHECK:
		{
			theHold.Begin();
			{
				mod->fnSetAlwayShowPeltSeams(IsDlgButtonChecked(hWnd, IDC_ALWAYSSHOWPELTSEAMS_CHECK));
			}
			theHold.Accept(GetString(IDS_PELT_ALWAYSSHOWSEAMS));

			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap5.setPeltAlwaysShowSeams"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_bool, mod->fnGetAlwayShowPeltSeams());
			macroRecorder->EmitScript();

			break;
		}
		case IDC_DONOTREFLATTEN_CHECK:
		{
			//set element mode swtich 
			//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
			theHold.Begin();
			{
				mod->fnSetPreventFlattening(IsDlgButtonChecked(hWnd, IDC_DONOTREFLATTEN_CHECK));
			}
			theHold.Accept(GetString(IDS_PW_PREVENTREFLATTENING));


			//send macro message
			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.setPreventFlattening"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_bool, mod->fnGetPreventFlattening());
			macroRecorder->EmitScript();

			if (mod->hDialogWnd)
			{
				IMenuBarContext* pContext = (IMenuBarContext*)GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
				if (pContext)
					pContext->UpdateWindowsMenu();
			}


			break;
		}
		case IDC_SHOWMAPSEAMS_CHECK:
		{
			BOOL showMapSeams = IsDlgButtonChecked(hWnd, IDC_SHOWMAPSEAMS_CHECK);
			theHold.Begin();
			{

				if (showMapSeams)
				{
					mod->fnSetViewportOpenEdges(TRUE);
				}
				else
				{
					mod->fnSetViewportOpenEdges(FALSE);
				}
			}
			theHold.Accept(GetString(IDS_PELT_ALWAYSSHOWSEAMS));

			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap5.setShowMapSeams"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_bool, mod->fnGetViewportOpenEdges());
			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());

			break;
		}

		case IDC_THINSEAM:
		case IDC_THICKSEAM:
		{
			BOOL thickSeams = IsDlgButtonChecked(hWnd, IDC_THICKSEAM);

			theHold.Begin();
			{
				if (thickSeams)
					mod->fnSetThickOpenEdges(TRUE);
				else mod->fnSetThickOpenEdges(FALSE);
			}
			theHold.Accept(GetString(IDS_THICKSEAMS));

			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());

			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap4.setThickOpenEdges"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_bool, mod->fnGetThickOpenEdges());



			break;
		}
		case IDC_NORMALIZEMAP_CHECK2:
		{
			//set element mode swtich 
			theHold.Begin();
			{
				mod->fnSetNormalizeMap(IsDlgButtonChecked(hWnd, IDC_NORMALIZEMAP_CHECK2));
			}
			theHold.Accept(GetString(IDS_NORMAL));
			//send macro message
			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap5.setNormalizeMap"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_bool, mod->fnGetNormalizeMap());
			macroRecorder->EmitScript();

			break;

		}

		}
	}
	return FALSE;
}

static INT_PTR CALLBACK UnwrapEditUVsRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	BOOL processed = mod->GetUIManager()->MessageProc(hWnd, msg, wParam, lParam);
	if (processed)
		return TRUE;
	switch (msg) {
	case WM_INITDIALOG:
	{
		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
	}
	}
	return FALSE;
}

static INT_PTR CALLBACK UnwrapWrapRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	BOOL processed = mod->GetUIManager()->MessageProc(hWnd, msg, wParam, lParam);
	if (processed)
		return TRUE;
	switch (msg) {
	case WM_INITDIALOG:
	{
		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
	}
	}
	return FALSE;
}


static INT_PTR CALLBACK UnwrapPeelRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	BOOL processed = mod->GetUIManager()->MessageProc(hWnd, msg, wParam, lParam);
	if (processed)
		return TRUE;
	switch (msg) {
	case WM_INITDIALOG:
	{
		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
		mod->GetUIManager()->UpdateCheckButtons();
	}
	}
	return FALSE;
}

static INT_PTR CALLBACK UnwrapMatIdRollupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static BOOL inEnter = FALSE;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			mod = (UnwrapMod*)lParam;
			DLSetWindowLongPtr(hWnd, lParam);
			mod->hParams = hWnd;

			mod->iSelMatIDSpinnerEdit = GetISpinner(GetDlgItem(hWnd, ID_MATIDSPIN));
			mod->iSelMatIDSpinnerEdit->LinkToEdit(GetDlgItem(hWnd, ID_MATIDEDIT), EDITTYPE_INT);
			mod->iSelMatIDSpinnerEdit->SetLimits(1, 256, FALSE);
			mod->iSelMatIDSpinnerEdit->SetAutoScale();

			mod->iSetMatIDSpinnerEdit = GetISpinner(GetDlgItem(hWnd, ID_SETMATIDSPIN));
			mod->iSetMatIDSpinnerEdit->LinkToEdit(GetDlgItem(hWnd, ID_SETMATIDEDIT), EDITTYPE_INT);
			mod->iSetMatIDSpinnerEdit->SetLimits(1, 256, FALSE);
			mod->iSetMatIDSpinnerEdit->SetAutoScale();
			break;
		}
		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP:
		{
			if (LOWORD(wParam) == ID_SETMATIDEDIT
				|| LOWORD(wParam) == ID_SETMATIDSPIN)
			{
				if (!inEnter)
				{
					inEnter = TRUE;
					int matId = mod->iSetMatIDSpinnerEdit->GetIVal();
					mod->fnSetSelectionMatID(matId);
					if (mod->ip)
						mod->ip->RedrawViews(mod->ip->GetTime());
					mod->InvalidateView();
				}
				inEnter = FALSE;
			}

			if (LOWORD(wParam) == ID_MATIDSPIN
				|| LOWORD(wParam) == ID_MATIDEDIT)
			{
				if (!inEnter)
				{
					inEnter = TRUE;
					int matId = mod->iSelMatIDSpinnerEdit->GetIVal();
					mod->fnSelectByMatID(matId);
				}
				inEnter = FALSE;
			}
		}
		break;
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_UNWRAP_SELECT_MAT_ID)
			{
				int matId = mod->iSelMatIDSpinnerEdit->GetIVal();
				mod->fnSelectByMatID(matId);
			}
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}

static INT_PTR CALLBACK UnwrapSelRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	BOOL processed = mod->GetUIManager()->MessageProc(hWnd, msg, wParam, lParam);
	if (processed)
		return TRUE;
	switch (msg) {
	case WM_INITDIALOG:
	{
		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
		mod->hSelRollup = hWnd;
		break;
	}
	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
	{
		if (LOWORD(wParam) == ID_SET_MIRROR_THRESHOLD_SPINNER ||
			LOWORD(wParam) == ID_SET_MIRROR_THRESHOLD_EDIT)
		{
			float threshold = mod->GetUIManager()->GetSpinFValue(ID_SET_MIRROR_THRESHOLD_SPINNER);
			if (threshold > 0.0f)
				mod->fnSetMirrorThreshold(threshold);
		}
	}
	break;
	default:
		return FALSE;
	}
	return TRUE;
}

static INT_PTR CALLBACK WarningProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg) {
	case WM_INITDIALOG:
	{
		CenterWindow(hWnd, NULL);
		break;
	}


	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MOVE:
		{
			EndDialog(hWnd, 0);
			break;
		}
		case IDC_ABANDON:
		{
			EndDialog(hWnd, 1);
			break;
		}

		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static INT_PTR CALLBACK UnwrapRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static BOOL inEnter = FALSE;

	switch (msg) {
	case WM_INITDIALOG:
	{

		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
		mod->hParams = hWnd;
		/*
		mod->iApplyButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_APPLY));
		mod->iApplyButton->SetType(CBT_PUSH);
		mod->iApplyButton->Enable(TRUE);
		*/
		mod->iMapID = GetISpinner(GetDlgItem(hWnd, IDC_MAP_CHAN_SPIN));
		mod->iMapID->LinkToEdit(GetDlgItem(hWnd, IDC_MAP_CHAN), EDITTYPE_INT);
		mod->iMapID->SetLimits(1, 99, FALSE);
		mod->iMapID->SetScale(0.1f);

		mod->SetupChannelButtons();
		mod->GetUIManager()->UpdateCheckButtons();


		break;
	}


	case WM_CUSTEDIT_ENTER:
	{
		if (!inEnter)
		{
			inEnter = TRUE;
			TSTR buf1 = GetString(IDS_RB_SHOULDRESET);
			TSTR buf2 = GetString(IDS_RB_UNWRAPMOD);
			int tempChannel = mod->iMapID->GetIVal();
			if (tempChannel == 1) tempChannel = 0;
			if (tempChannel != mod->channel)
			{
				int res = 0;
				res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_WARNINGDIALOG), mod->ip->GetMAXHWnd(), WarningProc);


				theHold.Begin();
				if (res == 1)
					mod->Reset();
				mod->channel = mod->iMapID->GetIVal();
				if (mod->channel == 1) mod->channel = 0;
				theHold.Accept(GetString(IDS_RB_SETCHANNEL));

				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.setMapChannel"));
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_int, mod->channel);
				macroRecorder->EmitScript();

				mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
				if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
				mod->InvalidateView();
			}
			inEnter = FALSE;
		}

	}

	break;
	case CC_SPINNER_BUTTONUP:
	{
		TSTR buf1 = GetString(IDS_RB_SHOULDRESET);
		TSTR buf2 = GetString(IDS_RB_UNWRAPMOD);
		int tempChannel = mod->iMapID->GetIVal();
		if (tempChannel == 1) tempChannel = 0;
		if (tempChannel != mod->channel)
		{
			int res = 0;
			res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_WARNINGDIALOG), mod->ip->GetMAXHWnd(), WarningProc);


			theHold.Begin();
			if (res == 1)
				mod->Reset();
			mod->channel = mod->iMapID->GetIVal();
			if (mod->channel == 1) mod->channel = 0;
			theHold.Accept(GetString(IDS_RB_SETCHANNEL));
			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.setMapChannel"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_int, mod->channel);
			macroRecorder->EmitScript();

			mod->SetCheckerMapChannel();

			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			mod->InvalidateView();

		}

	}

	break;


	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_UNWRAP_SAVE:
		{
			mod->fnSave();
			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.save"));
			macroRecorder->FunctionCall(mstr, 0, 0);
			macroRecorder->EmitScript();

			break;
		}
		case IDC_UNWRAP_LOAD:
		{
			mod->fnLoad();
			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.load"));
			macroRecorder->FunctionCall(mstr, 0, 0);
			macroRecorder->EmitScript();
			break;
		}
		case IDC_MAP_CHAN1:
		case IDC_MAP_CHAN2:
		{

			TSTR buf1 = GetString(IDS_RB_SHOULDRESET);
			TSTR buf2 = GetString(IDS_RB_UNWRAPMOD);
			int res = 0;
			res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_WARNINGDIALOG), mod->ip->GetMAXHWnd(), WarningProc);


			theHold.Begin();
			if (res == 1)
				mod->Reset();
			mod->channel = IsDlgButtonChecked(hWnd, IDC_MAP_CHAN2);
			if (mod->channel == 1)
				mod->iMapID->Enable(FALSE);
			else
			{
				int ival = mod->iMapID->GetIVal();
				if (ival == 1) mod->channel = 0;
				else mod->channel = ival;
				mod->iMapID->Enable(TRUE);
			}
			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			mod->InvalidateView();
			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.setVC"));
			if (mod->channel == 1)
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_bool, TRUE);
			else macroRecorder->FunctionCall(mstr, 1, 0,
				mr_bool, FALSE);
			macroRecorder->EmitScript();

			theHold.Accept(GetString(IDS_RB_SETCHANNEL));


			break;
		}

		case IDC_UNWRAP_RESET: {
			mod->fnReset();
			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.reset"));
			macroRecorder->FunctionCall(mstr, 0, 0);
			macroRecorder->EmitScript();
			break;
		}

		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

void UnwrapMod::SetupChannelButtons()
{
	if (hParams && editMod == this) {
		if (channel == 0)
		{
			iMapID->Enable(TRUE);
			iMapID->SetValue(1, TRUE);
			CheckDlgButton(hParams, IDC_MAP_CHAN1, TRUE);
			CheckDlgButton(hParams, IDC_MAP_CHAN2, FALSE);

		}
		else if (channel == 1)
		{
			CheckDlgButton(hParams, IDC_MAP_CHAN1, FALSE);
			CheckDlgButton(hParams, IDC_MAP_CHAN2, TRUE);
			iMapID->Enable(FALSE);
			//			iMapID->SetValue(0,TRUE);
		}
		else
		{
			CheckDlgButton(hParams, IDC_MAP_CHAN1, TRUE);
			CheckDlgButton(hParams, IDC_MAP_CHAN2, FALSE);
			iMapID->Enable(TRUE);
			iMapID->SetValue(channel, TRUE);
		}
	}
}

void UnwrapMod::Reset()
{
	if (theHold.Holding())
	{
		HoldPointsAndFaces();
		theHold.Put(new ResetRestore(this));
	}

	for (int i = 0; i < mUVWControls.cont.Count(); i++)
		if (mUVWControls.cont[i]) DeleteReference(i + 11 + 100);

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->FreeCache();
	}

	updateCache = TRUE;

	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE);  //fix for 894778 MZ	
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

BOOL UnwrapMod::AssignController(Animatable *control, int subAnim)
{
	ReplaceReference(subAnim + 11 + 100, (RefTargetHandle)control);
	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE);  //fix for 894778 MZ	
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	return TRUE;
}


Object *GetBaseObject(Object *obj)
{
	if (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject* dobj;
		dobj = (IDerivedObject*)obj;
		while (dobj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			dobj = (IDerivedObject*)dobj->GetObjRef();
		}

		return (Object *)dobj;

	}
	return obj;

}


class IsOnStack : public GeomPipelineEnumProc
{
public:
	IsOnStack(ReferenceTarget *me) : mRef(me), mOnStack(FALSE), mModData(NULL) {}
	PipeEnumResult proc(ReferenceTarget *object, IDerivedObject *derObj, int index);
	ReferenceTarget *mRef;
	BOOL mOnStack;
	MeshTopoData *mModData;
protected:
	IsOnStack(); // disallowed
	IsOnStack(IsOnStack& rhs); // disallowed
	IsOnStack& operator=(const IsOnStack& rhs); // disallowed
};

PipeEnumResult IsOnStack::proc(
	ReferenceTarget *object,
	IDerivedObject *derObj,
	int index)
{
	if ((derObj != NULL) && object == mRef) //found it!
	{
		mOnStack = TRUE;
		ModContext *mc = derObj->GetModContext(index);
		mModData = (MeshTopoData*)mc->localData;
		return PIPE_ENUM_STOP;
	}
	return PIPE_ENUM_CONTINUE;
}

bool UnwrapMod::CollectInStackMeshTopoData(INode* pNode, Object* pObj)
{
	IsOnStack pipeEnumProc(this);
	EnumGeomPipeline(&pipeEnumProc, pObj);
	if (pipeEnumProc.mOnStack == TRUE)
	{
		MeshTopoData *localData = pipeEnumProc.mModData;
		if (localData && mMeshTopoData.Append(1, localData, pNode, 10))
		{
			localData->RegisterNotification(this);
		}
		return true;
	}
	return false;
}

BOOL UnwrapMod::IsInStack(INode *node)
{
	SClass_ID		sc = 0;
	IDerivedObject* dobj = NULL;

	if ((dobj = node->GetWSMDerivedObject()) != NULL)
	{
		if (CollectInStackMeshTopoData(node, dobj))
		{
			return TRUE;
		}
	}

	Object* obj = node->GetObjectRef();
	if ((sc = obj->SuperClassID()) == GEN_DERIVOB_CLASS_ID)
	{
		if (CollectInStackMeshTopoData(node, obj))
		{
			return TRUE;
		}
	}
	return FALSE;
}

void UnwrapAction::GetCategoryText(TSTR& catText)
{
	catText.printf(_T("%s"), GetString(IDS_RB_UNWRAPMOD));
}

void UnwrapMod::GetClassName(TSTR& s)
{
	s = UNWRAP_NAME;
}
const TCHAR *UnwrapMod::GetObjectName()
{
	return UNWRAP_NAME;
}

void UnwrapMod::LoadDefaultSettingsIfNotYet()
{
	if (loadDefaults)
	{
		fnLoadDefaults();
		loadDefaults = FALSE;
	}
}

void UnwrapMod::BeginEditParams(
	IObjParam  *ip, ULONG flags, Animatable *prev)
{
	bMeshTopoDataContainerDirty = TRUE;
	LoadDefaultSettingsIfNotYet();

	selCur = ip->GetSysCursor(SYSCUR_SELECT);
	moveCur = ip->GetSysCursor(SYSCUR_MOVE);

	if (moveXCur == NULL)
		moveXCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::HorizontalMove);
	if (moveYCur == NULL)
		moveYCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::VerticalMove);

	if (scaleXCur == NULL)
		scaleXCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ScaleX);
	if (scaleYCur == NULL)
		scaleYCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ScaleY);

	if (zoomCur == NULL)
		zoomCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Magnify);
	if (zoomRegionCur == NULL)
		zoomRegionCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Region);
	if (panCur == NULL)
		panCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::PanHand);
	if (weldCur == NULL)
		weldCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::MoveW);

	if (weldCurHit == NULL)
		weldCurHit = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::MoveH);

	if (sketchCur == NULL)
		sketchCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SketchPick);

	if (sketchPickCur == NULL)
		sketchPickCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SketchPick);

	if (sketchPickHitCur == NULL)
		sketchPickHitCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SketchPick);


	if (circleCur[0] == NULL)
		circleCur[0] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle1);
	if (circleCur[1] == NULL)
		circleCur[1] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle2);
	if (circleCur[2] == NULL)
		circleCur[2] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle3);
	if (circleCur[3] == NULL)
		circleCur[3] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle4);
	if (circleCur[4] == NULL)
		circleCur[4] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle5);
	if (circleCur[5] == NULL)
		circleCur[5] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle6);
	if (circleCur[6] == NULL)
		circleCur[6] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle7);
	if (circleCur[7] == NULL)
		circleCur[7] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle8);
	if (circleCur[8] == NULL)
		circleCur[8] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle9);
	if (circleCur[9] == NULL)
		circleCur[9] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle10);
	if (circleCur[10] == NULL)
		circleCur[10] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle11);
	if (circleCur[11] == NULL)
		circleCur[11] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle12);
	if (circleCur[12] == NULL)
		circleCur[12] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle13);
	if (circleCur[13] == NULL)
		circleCur[13] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle14);
	if (circleCur[14] == NULL)
		circleCur[14] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle15);
	if (circleCur[15] == NULL)
		circleCur[15] = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Circle16);



	rotCur = ip->GetSysCursor(SYSCUR_ROTATE);
	scaleCur = ip->GetSysCursor(SYSCUR_USCALE);

	selectMode = new UnwrapSelectModBoxCMode(this, ip);
	moveGizmoMode = new MoveModBoxCMode(this, ip);
	rotGizmoMode = new RotateModBoxCMode(this, ip);
	uscaleGizmoMode = new UScaleModBoxCMode(this, ip);
	nuscaleGizmoMode = new NUScaleModBoxCMode(this, ip);
	squashGizmoMode = new SquashModBoxCMode(this, ip);
	mSplineMap.CreateCommandModes(this, ip);

	this->ip = ip;
	editMod = this;
	TimeValue t = ip->GetTime();
	//aszabo|feb.05.02
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	hSelParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_SELPARAMS),
		UnwrapSelRollupWndProc,
		GetString(IDS_PW_SELPARAMS),
		(LPARAM)this);

	hMatIdParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_MATID_PARAMS),
		UnwrapMatIdRollupWndProc,
		GetString(IDS_SEL_MATID_PARAMS),
		(LPARAM)this);
	

	hEditUVParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_EDITUVPARAMS),
		UnwrapEditUVsRollupWndProc,
		GetString(IDS_PW_EDITUVPARAMS),
		(LPARAM)this);

	hParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_PARAMS),
		UnwrapRollupWndProc,
		GetString(IDS_CHANNEL),
		(LPARAM)this);

	hPeelParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_PEELPARAMS),
		UnwrapPeelRollupWndProc,
		GetString(IDS_PEEL),
		(LPARAM)this);
	//PELT
	hMapParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_MAPPARAMS),
		UnwrapMapRollupWndProc,
		GetString(IDS_PROJECTION),
		(LPARAM)this);

	hWrapParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_WRAPPARAMS),
		UnwrapWrapRollupWndProc,
		GetString(IDS_RB_WRAPPARAMETERS),
		(LPARAM)this);

	hConfigureParams = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_CONFIGUREPARAMS),
		UnwrapConfigureRollupWndProc,
		GetString(IDS_CONFIGURE),
		(LPARAM)this);

	peltData.SetRollupHandle(hMapParams);
	peltData.SetSelRollupHandle(hSelRollup);
	peltData.SetParamRollupHandle(hParams);


	ip->RegisterTimeChangeCallback(this);

	SetNumSelLabel();

	peltPointToPointMode = new PeltPointToPointMode(this, ip);
	tweakMode = new TweakMode(this, ip);

	RegisterNotification(PreSave, this, NOTIFY_FILE_PRE_SAVE);
	RegisterNotification(PostSave, this, NOTIFY_FILE_POST_SAVE);

	mUnwrapNodeDisplayCallback.mod = this;
	INodeDisplayControl* ndc = GetNodeDisplayControl(ip);
	ndc->RegisterNodeDisplayCallback(&mUnwrapNodeDisplayCallback);
	ndc->SetNodeCallback(&mUnwrapNodeDisplayCallback);

	ActivateActionTable();

	if ((alwaysEdit) && (!popUpDialog))
		fnEdit();

	mModifierPanelUI.Setup(hInstance, GetParent(hParams), mToolBarIniFileName);
	mUIManager.AddButton(ID_EDIT, GetDlgItem(hEditUVParams, ID_EDIT), FALSE);
	mUIManager.AddButton(ID_TWEAKUVW, GetDlgItem(hEditUVParams, ID_TWEAKUVW), TRUE);

	mUIManager.AddButton(ID_MAPPING_FIT, GetDlgItem(hMapParams, ID_MAPPING_FIT), FALSE);
	mUIManager.AddButton(ID_MAPPING_CENTER, GetDlgItem(hMapParams, ID_MAPPING_CENTER), FALSE);
	mUIManager.Enable(ID_MAPPING_CENTER, FALSE);
	mUIManager.Enable(ID_MAPPING_FIT, FALSE);

	ip->SetShowEndResult(FALSE);

	// restore the previous sub obj level.
	if (ip && mTVSubObjectMode != ip->GetSubObjectLevel() && mTVSubObjectMode > 0)
	{
		fnSetTVSubMode(mTVSubObjectMode);
	}
	else if(ip)
	{
		XFormModes modes;
		ActivateSubobjSel(ip->GetSubObjectLevel(), modes); // update the UI.
	}
}


void UnwrapMod::EndEditParams(
	IObjParam *ip, ULONG flags, Animatable *next)
{
	if (!this->ip)
		this->ip = ip;

	peltData.SubObjectUpdateSuspend();
	fnSetMapMode(0);
	peltData.SubObjectUpdateResume();

	ClearAFlag(A_MOD_BEING_EDITED);

	//	ip->GetActionManager()->DeactivateActionTable(actionTable, kUnwrapActions);
	//	delete actionTable;

	if (hDialogWnd)
	{
		TearDownToolBarUIs();
		DestroyWindow(hDialogWnd);
	}

	mUIManager.Save(mToolBarIniFileName);
	mModifierPanelUI.TearDown();
	mUIManager.FreeButtons();


	ip->UnRegisterTimeChangeCallback(this);

	if (hSelParams) ip->DeleteRollupPage(hSelParams);
	hSelParams = NULL;

	if (hMatIdParams) ip->DeleteRollupPage(hMatIdParams);
	hMatIdParams = NULL;

	if (hEditUVParams) ip->DeleteRollupPage(hEditUVParams);
	hEditUVParams = NULL;


	if (hParams) ip->DeleteRollupPage(hParams);
	hParams = NULL;

	if (hPeelParams) ip->DeleteRollupPage(hPeelParams);
	hPeelParams = NULL;

	//PELT
	if (hMapParams) ip->DeleteRollupPage(hMapParams);
	hMapParams = NULL;

	if (hWrapParams) ip->DeleteRollupPage(hWrapParams);
	hWrapParams = NULL;

	if (hConfigureParams) ip->DeleteRollupPage(hConfigureParams);
	hConfigureParams = NULL;

	peltData.SetRollupHandle(NULL);
	peltData.SetSelRollupHandle(NULL);
	peltData.SetParamRollupHandle(NULL);

	ip->DeleteMode(selectMode);
	if (selectMode) delete selectMode;
	selectMode = NULL;

	ip->DeleteMode(moveGizmoMode);
	ip->DeleteMode(rotGizmoMode);
	ip->DeleteMode(uscaleGizmoMode);
	ip->DeleteMode(nuscaleGizmoMode);
	ip->DeleteMode(squashGizmoMode);

	if (moveGizmoMode) delete moveGizmoMode;
	moveGizmoMode = NULL;
	if (rotGizmoMode) delete rotGizmoMode;
	rotGizmoMode = NULL;
	if (uscaleGizmoMode) delete uscaleGizmoMode;
	uscaleGizmoMode = NULL;
	if (nuscaleGizmoMode) delete nuscaleGizmoMode;
	nuscaleGizmoMode = NULL;
	if (squashGizmoMode) delete squashGizmoMode;
	squashGizmoMode = NULL;

	fnSplineMap_EndMapMode();
	mSplineMap.DestroyCommandModes();

	if (peltData.GetPointToPointSeamsMode())
		peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);

	ip->DeleteMode(peltPointToPointMode);
	if (peltPointToPointMode) delete peltPointToPointMode;
	peltPointToPointMode = NULL;

	peltData.SetPeltMapMode(this, FALSE);
	mapMapMode = NOMAP;


	ip->DeleteMode(tweakMode);
	if (tweakMode) delete tweakMode;
	tweakMode = NULL;

	ReleaseISpinner(iMapID); iMapID = NULL;
	ReleaseISpinner(iSetMatIDSpinnerEdit); iSetMatIDSpinnerEdit = NULL;
	ReleaseISpinner(iSelMatIDSpinnerEdit); iSelMatIDSpinnerEdit = NULL;

	TimeValue t = ip->GetTime();
	//aszabo|feb.05.02
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
	this->ip = NULL;

	editMod = NULL;

	if (CurrentMap == 0)
		ShowCheckerMaterial(FALSE);

	INodeDisplayControl* ndc = GetNodeDisplayControl(ip);
	if (ndc)
	{
		ndc->SetNodeCallback(NULL);
		ndc->UnRegisterNodeDisplayCallback(&mUnwrapNodeDisplayCallback);
	}

	UnRegisterNotification(PreSave, this, NOTIFY_FILE_PRE_SAVE);
	UnRegisterNotification(PostSave, this, NOTIFY_FILE_POST_SAVE);
	DeActivateActionTable();

	GetToolTipExtender().RemoveToolTips();
}




class NullView : public View {
public:
	Point2 ViewToScreen(Point3 p) { return Point2(p.x, p.y); }
	NullView() { worldToView.IdentityMatrix(); screenW = 640.0f; screenH = 480.0f; }
};

/*
Box3 UnwrapMod::BuildBoundVolume(Object *obj)

{
Box3 b;
b.Init();
if (objType == IS_PATCH)
{
PatchObject *pobj = (PatchObject*)obj;
for (int i = 0; i < pobj->patch.patchSel.GetSize(); i++)
{
if (pobj->patch.patchSel[i])
{
int pcount = 3;
if (pobj->patch.patches[i].type == PATCH_QUAD) pcount = 4;
for (int j = 0; j < pcount; j++)
{
int index = pobj->patch.patches[i].v[j];

b+= pobj->patch.verts[index].p;
}
}
}

}
else if (objType == IS_MESH)
{
TriObject *tobj = (TriObject*)obj;
for (int i = 0; i < tobj->GetMesh().faceSel.GetSize(); i++)
{
if (tobj->GetMesh().faceSel[i])
{
for (int j = 0; j < 3; j++)
{
int index = tobj->GetMesh().faces[i].v[j];

b+= tobj->GetMesh().verts[index];
}
}
}
}
else if (objType == IS_MNMESH)
{
PolyObject *tobj = (PolyObject*)obj;
BitArray sel;
tobj->GetMesh().getFaceSel (sel);
for (int i = 0; i < sel.GetSize(); i++)
{
if (sel[i])
{
int ct;
ct = tobj->GetMesh().f[i].deg;
for (int j = 0; j < ct; j++)
{
int index = tobj->GetMesh().f[i].vtx[j];

b+= tobj->GetMesh().v[index].p;
}
}
}
}
return b;
}
*/

void UnwrapMod::InitControl(TimeValue t)
{
	Box3 box;
	Matrix3 tm(1);

	if (tmControl == NULL)
	{
		ReplaceReference(0, NewDefaultMatrix3Controller());
		NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE);
	}

	if (flags&CONTROL_INIT) {
		SuspendAnimate();
		AnimateOff();

		// get our bounding box

		Box3 bounds;
		bounds.Init();

		// get our center
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];

			Matrix3 tm = mMeshTopoData.GetNodeTM(t, ldID);
			for (int i = 0; i < ld->GetNumberGeomVerts(); i++)//TVMaps.geomPoints.Count(); i++)
			{
				bounds += ld->GetGeomVert(i) * tm; //TVMaps.geomPoints[i] * tm;
			}
		}

		if (!bounds.IsEmpty())
		{
			Point3 center = bounds.Center();
			// build the scale
			Point3 scale(bounds.Width().x, bounds.Width().y, bounds.Width().z);
			if (scale.x < 0.01f)
				scale.x = 1.0f;
			if (scale.y < 0.01f)
				scale.y = 1.0f;
			if (scale.z < 0.01f)
				scale.z = 1.0f;
			tm.SetRow(0, Point3(scale.x, 0.0f, 0.0f));
			tm.SetRow(1, Point3(0.0f, scale.y, 0.0f));
			tm.SetRow(2, Point3(0.0f, 0.0f, scale.z));
			tm.SetRow(3, center);
		}

		Matrix3 ptm(1), id(1);
		SetXFormPacket tmpck(tm, ptm);
		tmControl->SetValue(t, &tmpck, TRUE, CTRL_RELATIVE);


		ResumeAnimate();
	}
	flags = 0;
}



Matrix3 UnwrapMod::GetMapGizmoMatrix(TimeValue t)
{
	Matrix3 tm(1);
	Interval valid;

	//	int type = GetMapType();

	if (tmControl)
	{
		tmControl->GetValue(t, &tm, valid, CTRL_RELATIVE);
	}

	return tm;
}

Matrix3& UnwrapMod::GetMapGizmoTM()
{
	if (bMapGizmoTMDirty)
	{
		ComputeSelectedFaceData();
		bMapGizmoTMDirty = FALSE;
	}
	return mMapGizmoTM;
}

static int lStart[12] = { 0,1,3,2,4,5,7,6,0,1,2,3 };
static int lEnd[12] = { 1,3,2,0,5,7,6,4,4,5,6,7 };

void UnwrapMod::ComputeSelectedFaceData()
{
	TimeValue t = GetCOREInterface()->GetTime();
	mMapGizmoTM = Matrix3(1);
	int dir = GetQMapAlign();
	if (dir == 0) //x
	{
		mMapGizmoTM.SetRow(1, Point3(0.0f, 0.0f, 1.0f));
		mMapGizmoTM.SetRow(0, Point3(0.0f, 1.0f, 0.0f));
		mMapGizmoTM.SetRow(2, Point3(1.0f, 0.0f, 0.0f));
	}
	else if (dir == 1) //y
	{
		mMapGizmoTM.SetRow(0, Point3(1.0f, 0.0f, 0.0f));
		mMapGizmoTM.SetRow(1, Point3(0.0f, 0.0f, 1.0f));
		mMapGizmoTM.SetRow(2, Point3(0.0f, -1.0f, 0.0f));
	}
	else if (dir == 2) //z
	{
		mMapGizmoTM.SetRow(0, Point3(1.0f, 0.0f, 0.0f));
		mMapGizmoTM.SetRow(1, Point3(0.0f, 1.0f, 0.0f));
		mMapGizmoTM.SetRow(2, Point3(0.0f, 0.0f, 1.0f));
	}
	else
	{
		//compute our normal
		Point3 norm(0.0f, 0.0f, 0.0f);
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			Matrix3 toWorld = mMeshTopoData.GetNodeTM(t, ldID);
			Point3 pnorm(0.0f, 0.0f, 0.0f);
			int ct = 0;
			if (ld->GetFaceSel().AnyBitSet())
			{
				for (int k = 0; k < ld->GetNumberFaces(); k++)
				{
					if (ld->GetFaceSelected(k))
					{
						// Grap the three points, xformed
						int pcount = 3;
						pcount = ld->GetFaceDegree(k);

						Point3 temp_point[4];
						for (int j = 0; j < pcount; j++) {
							int index = ld->GetFaceGeomVert(k, j);
							Point3 p = ld->GetGeomVert(index);
							if (j < 4)
								temp_point[j] = p;
						}
						pnorm += VectorTransform(Normalize(temp_point[1] - temp_point[0] ^ temp_point[2] - temp_point[1]), toWorld);
						ct++;
					}
				}
			}
			
			if (ct > 0)
			{
				pnorm = pnorm / (float)ct;
				pnorm = Normalize(pnorm);
				UnwrapUtilityMethods::UnwrapMatrixFromNormal(pnorm, mMapGizmoTM);
			}
		}
	}

	//compute the center
	Box3 bounds;
	bounds.Init();

	Box3 worldBounds;
	worldBounds.Init();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray vSel;
		ld->GetGeomVertSelFromFace(vSel);
		Matrix3 toWorld = mMeshTopoData.GetNodeTM(t, ldID);
		Matrix3 toGizmoSpace = Inverse(mMapGizmoTM);
		if (vSel.AnyBitSet())
		{
			for (int i = 0; i < ld->GetNumberGeomVerts(); i++)
			{
				if (vSel[i])
				{
					Point3 p = ld->GetGeomVert(i) * toWorld;
					worldBounds += p;
					p = p * toGizmoSpace;
					bounds += p;
				}
			}
		}
	}
	float xScale = 10.0f;
	float yScale = 10.0f;
	if (!bounds.IsEmpty())
	{
		xScale = (bounds.pmax.x - bounds.pmin.x);
		yScale = (bounds.pmax.y - bounds.pmin.y);
		if (xScale > yScale)
			yScale = xScale;
		else xScale = yScale;
	}

	mMapGizmoTM.SetRow(3, worldBounds.Center());
	mMapGizmoTM.SetRow(0, mMapGizmoTM.GetRow(0) * xScale);
	mMapGizmoTM.SetRow(1, mMapGizmoTM.GetRow(1) * yScale);
}

void Draw3dEdge(GraphicsWindow *gw, float size, Point3 a, Point3 b, Color c)

{
	Matrix3 tm;
	Point3 vec = Normalize(a - b);
	MatrixFromNormal(vec, tm);
	Point3 vecA, vecB, vecC;
	vecA = tm.GetRow(0)*size;
	vecB = tm.GetRow(1)*size;
	Point3 p[3];
	Point3 color[3];
	Point3 ca = Point3(c);
	color[0] = ca;
	color[1] = ca;
	color[2] = ca;

	p[0] = a + vecA + vecB;
	p[1] = b + vecA + vecB;
	p[2] = a - vecA + vecB;
	gw->triangle(p, color);

	p[0] = b - vecA + vecB;
	p[1] = a - vecA + vecB;
	p[2] = b + vecA + vecB;
	gw->triangle(p, color);

	p[2] = a + vecA - vecB;
	p[1] = b + vecA - vecB;
	p[0] = a - vecA - vecB;
	gw->triangle(p, color);

	p[2] = b - vecA - vecB;
	p[1] = a - vecA - vecB;
	p[0] = b + vecA - vecB;
	gw->triangle(p, color);


	p[2] = a + vecB + vecA;
	p[1] = b + vecB + vecA;
	p[0] = a - vecB + vecA;
	gw->triangle(p, color);

	p[2] = b - vecB + vecA;
	p[1] = a - vecB + vecA;
	p[0] = b + vecB + vecA;
	gw->triangle(p, color);

	p[0] = a + vecB - vecA;
	p[1] = b + vecB - vecA;
	p[2] = a - vecB - vecA;
	gw->triangle(p, color);

	p[0] = b - vecB - vecA;
	p[1] = a - vecB - vecA;
	p[2] = b + vecB - vecA;
	gw->triangle(p, color);
}

template <typename SubHitList, typename ObjectType, typename SubHitRec>
void UnwrapMod::_BuildVisibleFaces(MeshTopoData * ld, GraphicsWindow * gw, BitArray &visibleFaces, ObjectType &object, DWORD hitFlags)
{
	visibleFaces.SetSize(ld->GetNumberFaces());
	visibleFaces.ClearAll();

	gw->clearHitCode();

	HitRegion hr;
	hr.type = RECT_RGN;
	hr.crossing = true;
	hr.rect.left = 0;
	hr.rect.top = 0;
	hr.rect.right = gw->getWinSizeX();
	hr.rect.bottom = gw->getWinSizeY();

	SubHitList hitList;
	object.SubObjectHitTest(gw, gw->getMaterial(), &hr,
		hitFlags, hitList);

	for (auto& rec : hitList) {
		int hitface = rec.index;
		if (hitface < visibleFaces.GetSize() && ld->DoesFacePassFilter(hitface))
			visibleFaces.Set(hitface, true);
	}
}

void UnwrapMod::BuildVisibleFaces(GraphicsWindow *gw, MeshTopoData *ld, BitArray &visibleFaces, int flags)
{
	if (ld->GetMesh())
	{
		_BuildVisibleFaces<SubObjHitList, Mesh, MeshSubHitRec>(ld, gw, visibleFaces, *ld->GetMesh(), flags | SUBHIT_FACES | SUBHIT_SELSOLID);
	}
	else if (ld->GetPatch())
	{
		_BuildVisibleFaces<SubPatchHitList, PatchMesh, PatchSubHitRec>(ld, gw, visibleFaces, *ld->GetPatch(), flags | SUBHIT_PATCH_PATCHES | SUBHIT_SELSOLID | SUBHIT_PATCH_IGNORE_BACKFACING);
	}
	else if (ld->GetMNMesh())
	{
		_BuildVisibleFaces<SubObjHitList, MNMesh, MeshSubHitRec>(ld, gw, visibleFaces, *ld->GetMNMesh(), flags | SUBHIT_MNFACES | SUBHIT_SELSOLID);
	}
}

unsigned long UnwrapMod::GetObjectDisplayRequirement() const
{
	return MaxSDK::Graphics::ObjectDisplayRequireLegacyDisplayMode;
}

bool UnwrapMod::PerpareDisplay(
	const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	return true;
}

bool UnwrapMod::UpdatePerNodeItems(
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer)
{
	using namespace MaxSDK::Graphics;

	//Under DX11 mode, we will use the render item to display 
	//the preview of seam edge,open edge,seam edge and selected edges
	if (Painter2D::Singleton().CheckDX11Mode())
	{
		ICustomRenderItemPtr	pUnwrapRenderItem;
		INode* pNode = nodeContext.GetRenderNode().GetMaxNode();
		auto it = mRenderItemsContainer.find(pNode);
		if (it == mRenderItemsContainer.end())
		{
			pUnwrapRenderItem = new UnwrapRenderItem(this, pNode);
			mRenderItemsContainer.insert(std::pair<INode*, MaxSDK::Graphics::ICustomRenderItemPtr>(pNode, pUnwrapRenderItem));
		}
		else
		{
			pUnwrapRenderItem = it->second;
		}
		CustomRenderItemHandle	newItem;
		newItem.Initialize();
		newItem.SetCustomImplementation(pUnwrapRenderItem);
		newItem.SetVisibilityGroup(RenderItemVisible_Gizmo);
		newItem.SetZBias(ZBiasPresets_Gizmo);
		targetRenderItemContainer.AddRenderItem(newItem);
	}

	//If the option is to display the angle distortion or area distortion
	if (hDialogWnd &&
		(GetDistortionType() == eAngleDistortion || GetDistortionType() == eAreaDistortion))
	{
		INode* pNode = nodeContext.GetRenderNode().GetMaxNode();
		ICustomRenderItemPtr	pUnwrapRenderItemDistortion = new UnwrapRenderItemDistortion(this, pNode);
		CustomRenderItemHandle	newItem;
		newItem.Initialize();
		newItem.SetCustomImplementation(pUnwrapRenderItemDistortion);
		newItem.SetVisibilityGroup(RenderItemVisible_Shaded);
		newItem.SetZBias(ZBiasPresets_Shaded);
		targetRenderItemContainer.AddRenderItem(newItem);
	}

	return true;
}

int UnwrapMod::Display(
	TimeValue t, INode* inode, ViewExp *vpt, int flags,
	ModContext *mc)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here?
		DbgAssert(!_T("Doing Display() on invalid viewport!"));
		return FALSE;
	}
	int iret = 0;

	if (mc->localData == NULL)
		return 1;

	if (!inode->Selected())
		return iret;

	Matrix3 modmat, ntm = inode->GetObjectTM(t);

	GraphicsWindow *gw = vpt->getGW();
	Matrix3 viewTM;
	vpt->GetAffineTM(viewTM);
	Point3 nodeCenter = ntm.GetRow(3);
	nodeCenter = nodeCenter;

	float thickLineSize = vpt->GetVPWorldWidth(nodeCenter) / gw->getWinSizeX() *0.5f;
	Point3 sizeVec(thickLineSize, thickLineSize, thickLineSize);
	sizeVec = VectorTransform(Inverse(ntm), sizeVec);
	thickLineSize = Length(sizeVec);

	MeshTopoData *ld = (MeshTopoData*)mc->localData;

	Matrix3 vtm(1), nodeTM(1);
	Interval iv;
	if (inode)
	{
		vtm = inode->GetObjectTM(t, &iv);
		nodeTM = inode->GetNodeTM(t, &iv);
	}

	DWORD limits = gw->getRndLimits();
	Matrix3 holdTM = gw->getTransform();

	if (fnGetMirrorSelectionStatus())
	{
		Point3 pivotTrans = nodeTM.GetTrans() - vtm.GetTrans();
		int mirrorAxis = fnGetMirrorAxis();
		pivotTrans[mirrorAxis] = 0;
		Point3 newTrans = nodeTM.GetTrans() - pivotTrans;
		nodeTM.SetTrans(newTrans);
		gw->setTransform(nodeTM);

		Color mirrorPlaneColor(0.0f, 0.2f, 1.0f); // designer want it to be blue
		ld->DisplayMirrorPlane(gw, fnGetMirrorAxis(), mirrorPlaneColor);
	}

	gw->setTransform(vtm);

	//DRAW SPLINE MAPPING INFO
	INode *splineNode = NULL;
	pblock->GetValue(unwrap_splinemap_node, t, splineNode, FOREVER);
	BOOL displaySplineMap = FALSE;
	pblock->GetValue(unwrap_splinemap_display, t, displaySplineMap, FOREVER);

	if (splineNode && displaySplineMap && (fnGetMapMode() == SPLINEMAP))
	{
		Tab<Point3> faceCenters;
		faceCenters.SetCount(ld->GetNumberFaces());
		for (int i = 0; i < ld->GetNumberFaces(); i++)
		{
			faceCenters[i] = Point3(0.0f, 0.0f, 0.0f);
			int deg = ld->GetFaceDegree(i);
			if (deg)
			{
				for (int j = 0; j < deg; j++)
				{
					int index = ld->GetFaceGeomVert(i, j);
					Point3 p = ld->GetGeomVert(index);
					faceCenters[i] += p;
				}
				faceCenters[i] /= deg;
			}
		}

		SplineMapProjectionTypes projType = kCircular;
		int val = 0;
		pblock->GetValue(unwrap_splinemap_projectiontype, t, val, FOREVER);
		if (val == 1)
			projType = kPlanar;
		mSplineMap.Display(gw, ld, vtm, faceCenters, projType);
	}

	gw->setTransform(vtm);

	BOOL boxMode = vpt->getGW()->getRndLimits() & GW_BOX_MODE;

	//draw our selected vertices
	if (ip && (ip->GetSubObjectLevel() == 1))
	{

		COLORREF vertColor = ColorMan()->GetColor(GEOMVERTCOLORID);

		Color c(vertColor);

		if (ld->HasIncomingFaceSelection())
			c = GetUIColor(COLOR_SEL_GIZMOS);

		gw->setColor(LINE_COLOR, c);
		gw->startMarkers();

		BOOL useDot = getUseVertexDots();
		int mType = getVertexDotType();
		for (int i = 0; i < ld->GetNumberGeomVerts(); i++)
		{
			if (ld->GetGeomVertSelected(i) &&
				ld->DoesGeomVertexPassFilter(i))
			{
				Point3 p = ld->GetGeomVert(i);
				if (useDot)
					gw->marker(&p, VERTEX_DOT_MARKER(mType));
				else gw->marker(&p, PLUS_SIGN_MRKR);
			}
		}
		gw->endMarkers();
		iret = 1;
	}

	float size = 0.5f;
	size = thickLineSize;

	gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL | GW_TWO_SIDED | GW_FLAT & ~GW_WIREFRAME | GW_ILLUM);
	COLORREF egdeSelectedColor = ColorMan()->GetColor(GEOMEDGECOLORID);
	COLORREF seamColorRef = ColorMan()->GetColor(PELTSEAMCOLORID);

	Color seamColor(seamColorRef);
	Color selectedEdgeColor(egdeSelectedColor);
	Color gizmoColor = GetUIColor(COLOR_SEL_GIZMOS);
	Color oEdgeColor(openEdgeColor);

	Material selectedEdgeMaterial;
	selectedEdgeMaterial.Kd = selectedEdgeColor;
	selectedEdgeMaterial.opacity = 1.0f;
	selectedEdgeMaterial.selfIllum = 1.0f;

	Material gizmoMaterial;
	gizmoMaterial.Kd = gizmoColor;
	gizmoMaterial.opacity = 1.0f;
	gizmoMaterial.selfIllum = 1.0f;

	Material openEdgeMaterial;
	openEdgeMaterial.Kd = oEdgeColor;
	openEdgeMaterial.opacity = 1.0f;
	openEdgeMaterial.selfIllum = 1.0f;

	Material seamMaterial;
	seamMaterial.Kd = seamColor;
	seamMaterial.opacity = 1.0f;
	seamMaterial.selfIllum = 1.0f;


	BOOL hasIncSelection = ld->HasIncomingFaceSelection();

	//now draw our selected edges, open edges, seams, and selected faces if in subobject mode
	//loop through edges

	//Under DX11 mode, the edges will be displayed in the UnwrapRenderItem.
	//Under !DX11 mode,using the gw to draw edges.
	if (!Painter2D::Singleton().CheckDX11Mode())
	{
		if (hasIncSelection)
		{
			gw->setMaterial(gizmoMaterial);
			gw->setColor(LINE_COLOR, gizmoColor);
		}
		else
		{
			gw->setMaterial(selectedEdgeMaterial);
			gw->setColor(LINE_COLOR, selectedEdgeColor);
		}
		if (thickOpenEdges)
			gw->startTriangles();
		else gw->startSegments();
		if (ip && (ip->GetSubObjectLevel() == 2) && !GetLivePeelModeEnabled())
		{

			for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
			{
				//if selected draw it and in edge mode	
				if (ld->GetGeomEdgeSelected(i))
					ld->DisplayGeomEdge(gw, i, size, thickOpenEdges, selectedEdgeColor);
			}
		}
		if (thickOpenEdges)
			gw->endTriangles();
		else gw->endSegments();
	}

	if (Painter2D::Singleton().CheckDX11Mode())
	{
		//check the seam edges if they are dirty
		if (ld->mSeamEdgesRef == ld->mSeamEdges)
		{
			;
		}
		else
		{
			ld->mSeamEdgesRef = ld->mSeamEdges;
			ld->mbSeamEdgesDirty = true;
		}

		//check the open TV edges if they are dirty
		BitArray openTVEdges = ld->GetOpenTVEdges();
		if (ld->mOpenTVEdgesRef == openTVEdges)
		{
			;
		}
		else
		{
			ld->mOpenTVEdgesRef = openTVEdges;
			ld->mbOpenTVEdgesDirty = true;
		}
	}
	else
	{
		//draw our seams
		gw->setMaterial(seamMaterial);
		gw->setColor(LINE_COLOR, seamColor);
		if (thickOpenEdges)
			gw->startTriangles();
		else gw->startSegments();
		if (!boxMode && fnGetAlwayShowPeltSeams())
		{
			for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
			{
				if ((i < ld->mSeamEdges.GetSize()) && ld->mSeamEdges[i])
				{
					ld->DisplayGeomEdge(gw, i, size, thickOpenEdges, seamColor);
				}
			}
		}
		if (thickOpenEdges)
			gw->endTriangles();
		else gw->endSegments();

		//if open edge draw it
		DisplayOpenEdges(gw, openEdgeMaterial, oEdgeColor, boxMode, ld, size);
	}

	//debug code below to visualize clusters, helps with verifing clusters
	//only displayed if switched on with a param block entry
	BOOL displayClusterColor = 0;
	pblock->GetValue(unwrap_group_display, 0, displayClusterColor, FOREVER);
	if (!boxMode && displayClusterColor)
	{
		gw->startSegments();
		//loop through our tv edges looking for open faces
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
		{
			int ct = ld->GetGeomEdgeNumberOfConnectedFaces(i);
			BOOL skip = FALSE;
			if (ip && (ip->GetSubObjectLevel() == 2))
			{
				if (ld->GetGeomEdgeSelected(i))
					skip = TRUE;
			}
			if ((ct == 2) && (!skip) && (ld->GetGeomEdgeHidden(i) == FALSE))
			{
				int fa = ld->GetGeomEdgeConnectedFace(i, 0);
				int fb = ld->GetGeomEdgeConnectedFace(i, 0);
				Color c(0, 0, 0);
				BOOL draw = FALSE;
				int clusterID1 = ld->GetToolGroupingData()->GetGroupID(fa);
				int clusterID2 = ld->GetToolGroupingData()->GetGroupID(fb);

				if (clusterID1 == clusterID2)
				{
					int id = clusterID1;
					if (id == -1)
						id = clusterID2;
					if (id != -1)
					{
						draw = TRUE;
						c = mColors[id%mColors.Count()];
					}
				}

				if (draw)
				{
					gw->setColor(LINE_COLOR, c);
					ld->DisplayGeomEdge(gw, i, 0, FALSE, c);
				}
			}
		}

		gw->endSegments();
	}


	//if in face mode and has incoming edge draw and face selected draw it
	//draw our seams
	gw->setMaterial(gizmoMaterial);
	gw->setColor(LINE_COLOR, gizmoColor);
	if (thickOpenEdges)
		gw->startTriangles();
	else gw->startSegments();
	if (!boxMode && hasIncSelection && ip && (ip->GetSubObjectLevel() == 3))
	{
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
		{
			int ct = ld->GetGeomEdgeNumberOfConnectedFaces(i);
			BOOL sel = FALSE;
			for (int j = 0; j < ct; j++)
			{
				int faceIndex = ld->GetGeomEdgeConnectedFace(i, j);
				if (ld->GetFaceSelected(faceIndex))
					sel = TRUE;
			}
			if (sel)
			{
				ld->DisplayGeomEdge(gw, i, size, thickOpenEdges, gizmoColor);
			}
		}
	}

	if (thickOpenEdges)
		gw->endTriangles();
	else gw->endSegments();

	if (Painter2D::Singleton().CheckDX11Mode())
	{
		//check the preview of the seam edges
		if (ld->mSeamEdgesPreviewRef == ld->GetSeamEdgePreiveBitArray())
		{
			;
		}
		else
		{
			ld->mSeamEdgesPreviewRef = ld->GetSeamEdgePreiveBitArray();
			ld->mbSeamEdgesPreviewDirty = true;
		}
	}
	else
	{
		// Display the Point-to-Point Preview lines
		if (!boxMode && fnGetAlwayShowPeltSeams())
		{
			for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
			{
				if (ld->GetSeamEdgesPreview(i))
				{
					ld->DisplayGeomEdge(gw, i, size, thickOpenEdges, seamColor);
				}
			}
		}
	}



	//draw the mapping gizmo
	if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP)))
	{
		DrawGizmo(t, inode, gw);
		iret = 1;
	}
	//draw the quick mapping gizmo	
	else if ((ip && (ip->GetSubObjectLevel() == 3) && (GetQMapPreview())))
	{
		gw->setTransform(GetMapGizmoTM());
		Point3 zero(0.0f, 0.0f, 0.0f);
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_SEL_GIZMOS));

		gw->marker(&zero, DOT_MRKR);

		Point3 line[3];
		gw->startSegments();

		line[0] = zero;
		line[1] = Point3(0.0f, 0.0f, 10.0f);
		gw->segment(line, 1);

		line[0] = Point3(-0.5f, -0.5f, 0.0f);
		line[1] = Point3(0.5f, -0.5f, 0.0f);
		gw->segment(line, 1);

		line[0] = Point3(-0.5f, 0.5f, 0.0f);
		line[1] = Point3(0.5f, 0.5f, 0.0f);
		gw->segment(line, 1);

		line[0] = Point3(-0.5f, -0.5f, 0.0f);
		line[1] = Point3(-0.5f, 0.5f, 0.0f);
		gw->segment(line, 1);

		line[0] = Point3(0.5f, -0.5f, 0.0f);
		line[1] = Point3(0.5f, 0.5f, 0.0f);
		gw->segment(line, 1);

		line[0] = Point3(0.0f, 0.0f, 0.0f);
		line[1] = Point3(0.0f, 0.5f, 0.0f);
		gw->segment(line, 1);

		line[0] = Point3(0.0f, 0.0f, 0.0f);
		line[1] = Point3(0.25f, 0.0f, 0.0f);
		gw->segment(line, 1);
		gw->endSegments();
	}
	if (ip && (ip->GetSubObjectLevel() == 3) && ip->GetShowEndResult() && inode->Selected())
	{
		gw->setTransform(vtm);
		gw->startSegments();
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
		{
			//if selected draw it and in edge mode	
			int ct = ld->GetGeomEdgeNumberOfConnectedFaces(i);
			BOOL selected = FALSE;
			for (int j = 0; j < ct; j++)
			{
				int index = ld->GetGeomEdgeConnectedFace(i, j);
				if (ld->GetFaceSelected(index))
					selected = TRUE;

			}
			if (selected)
				ld->DisplayGeomEdge(gw, i, size, FALSE, selectedEdgeColor);
		}
		gw->endSegments();
	}

	mRegularMap.Display(gw);

	gw->setRndLimits(limits);
	gw->setTransform(holdTM);

	if (fnGetTVSubMode() == TVFACEMODE && ld->GetFaceSelChanged())
	{
		UpdateMatIDUI();
		ld->ResetFaceSelChanged();
	}

	return iret;
}

int UnwrapMod::GetCurrentMatID()
{
	bool determined = false;
	MtlID mat = 0;

	//should put back to WM_PAINT
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData* data = GetMeshTopoData(ldID);
		if (!data)
			continue;
		for (int i = 0; i < data->GetNumberFaces(); i++)
		{
			if (data->GetFaceSelected(i))
			{
				if (!(determined)) {
					mat = data->GetFaceMatID(i);
					determined = true;
				}
				else if (data->GetFaceMatID(i) != mat) {
					determined = false;
					break;
				}
			}
		}
	}
	if (determined)
	{
		return mat;
	}
	return -1;
}

void UnwrapMod::UpdateMatIDUI()
{
	int id = GetCurrentMatID();
	
	if (iSetMatIDSpinnerEdit && iSelMatIDSpinnerEdit)
	{
		if (id < 0)
		{
			iSetMatIDSpinnerEdit->SetIndeterminate(TRUE);
			iSelMatIDSpinnerEdit->SetIndeterminate(TRUE);
		}
		else
		{
			iSetMatIDSpinnerEdit->SetIndeterminate(FALSE);
			iSetMatIDSpinnerEdit->SetValue(int(id + 1), TRUE);
			iSelMatIDSpinnerEdit->SetIndeterminate(FALSE);
			iSelMatIDSpinnerEdit->SetValue(int(id + 1), TRUE);
		}
	}
	
}
void UnwrapMod::DisplayOpenEdges(GraphicsWindow * gw, Material openEdgeMaterial, Color oEdgeColor, BOOL boxMode, MeshTopoData * ld, float size)
{
	gw->setMaterial(openEdgeMaterial);
	gw->setColor(LINE_COLOR, oEdgeColor);
	if (thickOpenEdges)
		gw->startTriangles();
	else gw->startSegments();

	if (!boxMode && viewportOpenEdges)
	{
		BitArray openGeoEdges = ld->GetOpenGeomEdges();
		for (int geoEIndex = 0; geoEIndex < openGeoEdges.GetSize(); ++geoEIndex)
		{
			if (openGeoEdges[geoEIndex]) ld->DisplayGeomEdge(gw, geoEIndex, size, thickOpenEdges, oEdgeColor);
		}
	}

	if (thickOpenEdges)
		gw->endTriangles();
	else gw->endSegments();
}

void UnwrapMod::GetWorldBoundBox(
	TimeValue t, INode* inode, ViewExp *vpt, Box3& box,
	ModContext *mc)
{
	if (!vpt || !vpt->IsAlive())
	{
		box.Init();
		return;
	}

	gizmoBounds.Init();

	if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() == SPLINEMAP)))
	{
		Box3 bounds;
		bounds.Init();
		bounds = mSplineMap.GetWorldsBounds();

		box = bounds;

	}
	else if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP)))
	{
		Box3 bounds;
		bounds.Init();
		bounds += Point3(1.0f, 1.0f, 1.0f);
		bounds += Point3(-1.0f, -1.0f, -1.0f);
		box.Init();
		Matrix3 ntm(1);
		Matrix3 tm = GetMapGizmoMatrix(t);
		for (int i = 0; i < 8; i++)
		{
			box += bounds[i] * tm;
		}
		gizmoBounds = box;

		//		cb->TM(modmat,0);
	}
	else if (ip && (ip->GetSubObjectLevel() > 0))
	{
		Box3 bounds;
		bounds.Init();
		bounds += Point3(1.0f, 1.0f, 1.0f);
		bounds += Point3(-1.0f, -1.0f, -1.0f);
		box.Init();


		for (int i = 0; i < 8; i++)
		{
			box += bounds[i] * GetMapGizmoTM();
		}
		gizmoBounds = box;
	}
}

void UnwrapMod::GetSubObjectCenters(
	SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc)
{

	if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() == SPLINEMAP)))
	{
		Tab<int> selSplines;
		Tab<int> selCrossSections;
		mSplineMap.GetSelectedCrossSections(selSplines, selCrossSections);
		//see if we have a cross selection selected
		//if not use the face selection
		if (selCrossSections.Count() == 0)
		{
			BOOL hasSelection = FALSE;
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				BitArray faceSel = ld->GetFaceSel();

				if ((int)faceSel.AnyBitSet())
				{
					hasSelection = TRUE;
				}
			}
			Box3 bounds;
			bounds.Init();
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				Matrix3 tm = mMeshTopoData.GetNodeTM(t, ldID);
				MeshTopoData *ld = mMeshTopoData[ldID];
				if (hasSelection)
				{
					bounds += ld->GetSelFaceBoundingBox()* tm;
				}
				else
				{
					bounds += ld->GetBoundingBox() * tm;
				}
			}

			cb->Center(bounds.Center(), 0);
		}
		else
		{
			SplineCrossSection *section = mSplineMap.GetCrossSection(selSplines[0], selCrossSections[0]);
			Point3 p = section->mTM.GetTrans();

			cb->Center(p, 0);
		}
	}
	else if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP)) && (fnGetMapMode() != UNFOLDMAP) && (fnGetMapMode() != LSCMMAP))
	{
		cb->Center(mGizmoTM.GetTrans(), 0);
	}
	else
	{
		int ct = 0;
		Box3 box;
		if (ip && (ip->GetSubObjectLevel() == 1))
		{
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				Matrix3 tm = GetMeshTopoDataNode(ldID)->GetObjectTM(t);
				MeshTopoData *ld = mMeshTopoData[ldID];
				if (ld->GetGeomVertSel().AnyBitSet())
				{
					for (int i = 0; i < ld->GetNumberGeomVerts(); i++)
					{
						if (ld->GetGeomVertSelected(i))
						{
							box += ld->GetGeomVert(i) * tm;
							ct++;
						}
					}
				}
			}
		}
		else if (ip && (ip->GetSubObjectLevel() == 2))
		{
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				Matrix3 tm = GetMeshTopoDataNode(ldID)->GetObjectTM(t);
				if (ld->GetGeomEdgeSel().AnyBitSet())
				{
					for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
					{
						if (ld->GetGeomEdgeSelected(i))
						{
							int a = ld->GetGeomEdgeVert(i, 0);
							box += ld->GetGeomVert(a) * tm;
							a = ld->GetGeomEdgeVert(i, 1);
							box += ld->GetGeomVert(a) * tm;
							ct++;
						}
					}
				}
			}
		}
		else if (ip && (ip->GetSubObjectLevel() == 3))
		{
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				Matrix3 tm = GetMeshTopoDataNode(ldID)->GetObjectTM(t);
				if (ld->GetFaceSel().AnyBitSet())
				{
					for (int i = 0; i < ld->GetNumberFaces(); i++)
					{
						if (ld->GetFaceSelected(i))
						{
							int degree = ld->GetFaceDegree(i);
							for (int j = 0; j < degree; j++)
							{
								int id = ld->GetFaceGeomVert(i, j);//TVMaps.f[i]->v[j];
								box += ld->GetGeomVert(id) * tm;
								ct++;
							}
						}
					}
				}
			}
		}

		if (ct == 0) return;

		mUnwrapNodeDisplayCallback.mBounds = box;

		cb->Center(box.Center(), 0);
	}

}

void UnwrapMod::GetSubObjectTMs(
	SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc)
{

	if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() == SPLINEMAP)))
	{
		Tab<int> selSplines;
		Tab<int> selCrossSections;
		mSplineMap.GetSelectedCrossSections(selSplines, selCrossSections);
		//see if we have a cross selection selected
		//if not use the face selection
		if (selCrossSections.Count() == 0)
		{
			BOOL hasSelection = FALSE;
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				BitArray faceSel = ld->GetFaceSel();

				if ((int)faceSel.AnyBitSet())
				{
					hasSelection = TRUE;
				}
			}
			Box3 bounds;
			bounds.Init();
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				Matrix3 tm = mMeshTopoData.GetNodeTM(t, ldID);
				MeshTopoData *ld = mMeshTopoData[ldID];
				if (hasSelection)
				{
					bounds += ld->GetSelFaceBoundingBox()* tm;
				}
				else
				{
					bounds += ld->GetBoundingBox() * tm;
				}
			}
			Matrix3 tm(1);
			tm.SetTrans(bounds.Center());
			cb->TM(tm, 0);
		}
		else
		{
			SplineCrossSection *section = mSplineMap.GetCrossSection(selSplines[0], selCrossSections[0]);
			Point3 p = section->mTM.GetTrans();

			Matrix3 tm(1);
			tm = section->mTM;
			tm.NoScale();
			//			tm.SetTrans(p);
			cb->TM(tm, 0);
		}
	}

	else if ((ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP)))
	{
		Matrix3 ntm = node->GetObjectTM(t), modmat;
		modmat = GetMapGizmoMatrix(t);
		cb->TM(modmat, 0);
	}
}

static void LogHitEdgesInViewport(INode* inode, ViewExp *vpt, ModContext* mc, MeshTopoData* ld,
	int indexHitEdge, int flags, bool bPeelMode = false)
{
	if (inode == nullptr || vpt == nullptr || mc == nullptr || ld == nullptr)
	{
		return;
	}

	BOOL bGeomEdgeSelected = ld->GetGeomEdgeSelected(indexHitEdge) || (bPeelMode && ld->IsOpenGeomEdge(indexHitEdge));
	BOOL bSelOnly = flags&HIT_SELONLY;
	BOOL bUnSelOnly = flags&HIT_UNSELONLY;
	BOOL bOthers = !(flags&(HIT_UNSELONLY | HIT_SELONLY));
	if ((bGeomEdgeSelected && bSelOnly) || bOthers)
		vpt->LogHit(inode, mc, 0.0f, indexHitEdge, NULL);
	else if ((!bGeomEdgeSelected && bUnSelOnly) || bOthers)
		vpt->LogHit(inode, mc, 0.0f, indexHitEdge, NULL);

}

int UnwrapMod::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc) {

	if (!vpt || !vpt->IsAlive())
	{
		// why are we here?
		DbgAssert(!_T("Doing HitTest() on invalid viewport!"));
		return FALSE;
	}

	Interval valid;
	int savedLimits = 0, res = 0;
	GraphicsWindow *gw = vpt->getGW();
	HitRegion hr;
	MeshTopoData *ld = (MeshTopoData *)mc->localData;
	mRegularMap.SetPreviewFace(-1);

	if ((fnGetMapMode() == UNFOLDMAP) && fnRegularMapGetPickStartFace() &&
		(ld->GetMesh() || ld->GetMNMesh()))
	{
		if (ld == fnRegularMapGetLocalData())
		{
			Tab<int> hitFaces;
			HitRegion hr;
			MakeHitRegion(hr, type, crossing, 4, p);
			int hitface = ld->HitTestFace(gw, inode, hr);

			if (hitface > 0)
			{
				vpt->LogHit(inode, mc, 0.0f, hitface, NULL);
				mRegularMap.SetPreviewFace(hitface);
				if (ip)
				{
					NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
					ip->RedrawViews(ip->GetTime());
				}
			}


			return hitFaces.Count();
		}
		return 0;
	}

	if (fnGetMapMode() == SPLINEMAP)
	{
		INode *splineNode = NULL;
		pblock->GetValue(unwrap_splinemap_node, t, splineNode, FOREVER);
		BOOL displaySplineMap = FALSE;
		pblock->GetValue(unwrap_splinemap_display, t, displaySplineMap, FOREVER);
		if (splineNode && displaySplineMap)
		{
			SplineMapProjectionTypes projType = kCircular;
			int val = 0;
			pblock->GetValue(unwrap_splinemap_projectiontype, t, val, FOREVER);
			if (val == 1)
				projType = kPlanar;
			MakeHitRegion(hr, type, crossing, 4, p);
			gw->setHitRegion(&hr);
			Matrix3 tm = inode->GetObjectTM(t);
			res = mSplineMap.HitTest(vpt, inode, mc, tm, hr, flags, projType, TRUE);
			return res;
		}
	}
	else if ((peltData.GetEditSeamsMode()))
	{
		MakeHitRegion(hr, type, crossing, 4, p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else 
				gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) 
				gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL);
			else 
				gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		Tab<UVWHitData> hitEdges;
		res = peltData.HitTestEdgeMode(this, ld, *vpt, gw, hitEdges, hr, inode, mc, flags, type);
		gw->setRndLimits(savedLimits);
		return res;
	}
	else if (ip && (ip->GetSubObjectLevel() == 1))
	{
		

		if (mode == ID_WELD)
			type = HITTYPE_POINT;
		MakeHitRegion(hr, type, crossing, 4, p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		
		int hitFlags = 0;
		if (mat.Parity())
			hitFlags |= HIT_MIRROREDTM;

		gw->setTransform(mat);
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else 
				gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) 
				gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL);
			else 
				gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		gw->clearHitCode();

		BOOL add = GetKeyState(VK_CONTROL) < 0;
		BOOL sub = GetKeyState(VK_MENU) < 0;
		BOOL polySelect = !(GetKeyState(VK_SHIFT) < 0);

		if (add)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_ADD));
		else if (sub)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (!polySelect)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTTRI));
		else ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTFACE));

		//build our visible face
		BitArray visibleFaces;
		BuildVisibleFaces(gw, ld, visibleFaces, hitFlags);

		Tab<UVWHitData> hitVerts;
		HitGeomVertData(ld, hitVerts, gw, hr);


		BitArray visibleVerts;
		visibleVerts.SetSize(ld->GetNumberGeomVerts());


		if (fnGetBackFaceCull())
		{
			visibleVerts.ClearAll();

			for (int i = 0; i < ld->GetNumberFaces(); i++)
			{
				if (visibleFaces[i])
				{
					int deg = ld->GetFaceDegree(i);
					for (int j = 0; j < deg; j++)
					{
						visibleVerts.Set(ld->GetFaceGeomVert(i, j), TRUE);
					}
				}
			}

		}
		else
		{
			visibleVerts.SetAll();
		}



		for (int hi = 0; hi < hitVerts.Count(); hi++)
		{
			int i = hitVerts[hi].index;
			//						if (hitEdges[i])
			{



				if (fnGetBackFaceCull())
				{
					if (visibleVerts[i])
					{
						if ((ld->GetGeomVertSelected(i) && (flags&HIT_SELONLY)) ||
							!(flags&(HIT_UNSELONLY | HIT_SELONLY)))
							vpt->LogHit(inode, mc, 0.0f, i, NULL);
						else if ((!ld->GetGeomVertSelected(i) && (flags&HIT_UNSELONLY)) ||
							!(flags&(HIT_UNSELONLY | HIT_SELONLY)))
							vpt->LogHit(inode, mc, 0.0f, i, NULL);

					}
				}
				else
				{

					if ((ld->GetGeomVertSelected(i) && (flags&HIT_SELONLY)) ||
						!(flags&(HIT_UNSELONLY | HIT_SELONLY)))
						vpt->LogHit(inode, mc, 0.0f, i, NULL);
					else if ((!ld->GetGeomVertSelected(i) && (flags&HIT_UNSELONLY)) ||
						!(flags&(HIT_UNSELONLY | HIT_SELONLY)))
						vpt->LogHit(inode, mc, 0.0f, i, NULL);

				}
			}
		}

	}
	if (ip && (ip->GetSubObjectLevel() == 2))
	{

		MakeHitRegion(hr, type, crossing, 4, p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);

		int hitFlags = 0;
		if (mat.Parity())
			hitFlags |= HIT_MIRROREDTM;

		gw->setTransform(mat);
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		gw->clearHitCode();

		BOOL add = GetKeyState(VK_CONTROL) < 0;
		BOOL sub = GetKeyState(VK_MENU) < 0;
		BOOL polySelect = !(GetKeyState(VK_SHIFT) < 0);

		if (add)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_ADD));
		else if (sub)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (!polySelect)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTTRI));
		else ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTFACE));

		//build our visible face


		if (peltData.GetPointToPointSeamsMode())
		{
			res = peltData.HitTestPointToPointMode(this, ld, *vpt, gw, p, hr, inode, mc);
		}
		else
		{
			BitArray visibleFaces;

			BuildVisibleFaces(gw, ld, visibleFaces, hitFlags);

			//hit test our geom edges
			Tab<UVWHitData> hitEdges;
			HitGeomEdgeData(ld, hitEdges, gw, hr);
			res = hitEdges.Count();
			if (type == HITTYPE_POINT)
			{
				int closestIndex = -1;
				float closest = 0.0f;
				Matrix3 toView(1);
				vpt->GetAffineTM(toView);
				toView = mat * toView;
				for (int hi = 0; hi < hitEdges.Count(); hi++)
				{
					int eindex = hitEdges[hi].index;
					int a = ld->GetGeomEdgeVert(eindex, 0);

					Point3 p = ld->GetGeomVert(a) * toView;
					if ((p.z > closest) || (closestIndex == -1))
					{
						closest = p.z;
						closestIndex = hi;
					}
				}
				if (closestIndex != -1)
				{
					Tab<UVWHitData> tempHitEdge;
					tempHitEdge.Append(1, &hitEdges[closestIndex], 1);
					hitEdges = tempHitEdge;
				}
			}

			for (int hi = 0; hi < hitEdges.Count(); hi++)
			{
				int i = hitEdges[hi].index;
				{
					BOOL visibleFace = FALSE;
					BOOL selFace = FALSE;
					int ct = ld->GetGeomEdgeNumberOfConnectedFaces(i);
					for (int j = 0; j < ct; j++)
					{
						int faceIndex = ld->GetGeomEdgeConnectedFace(i, j);
						if ((faceIndex < ld->GetNumberFaces()))
							selFace = TRUE;
						if ((faceIndex < visibleFaces.GetSize()) && (visibleFaces[faceIndex]))
							visibleFace = TRUE;

					}

					if (fnGetBackFaceCull())
					{
						if (selFace && visibleFace)
						{
							LogHitEdgesInViewport(inode, vpt, mc, ld, i, flags, GetLivePeelModeEnabled());
						}
					}
					else
					{
						if (selFace)
						{
							LogHitEdgesInViewport(inode, vpt, mc, ld, i, flags, GetLivePeelModeEnabled());
						}
					}
				}
			}
		}


	}
	else if (ip && (ip->GetSubObjectLevel() == 3))
	{
		MakeHitRegion(hr, type, crossing, 4, p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		gw->clearHitCode();

		BOOL add = GetKeyState(VK_CONTROL) < 0;
		BOOL sub = GetKeyState(VK_MENU) < 0;
		BOOL polySelect = !(GetKeyState(VK_SHIFT) < 0);

		if (add)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_ADD));
		else if (sub)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (!polySelect)
			ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTTRI));
		else ip->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTFACE));

		MeshTopoData *ld = dynamic_cast<MeshTopoData*>(mc->localData);

		if ((fnGetMapMode() != NOMAP) && (fnGetMapMode() != SPLINEMAP) && (fnGetMapMode() != UNFOLDMAP) && (fnGetMapMode() != LSCMMAP))
		{
			if (!peltData.GetPeltMapMode()
				|| (!peltData.GetPointToPointSeamsMode() && !peltData.GetEditSeamsMode())
				)
			{
				DrawGizmo(t, inode, gw);
				if (gw->checkHitCode())
				{
					vpt->LogHit(inode, mc, gw->getHitDistance(), 0, NULL);
					res = 1;
				}
			}
		}
		else if (ld && ld->GetMesh())
		{
			SubObjHitList hitList;

			Mesh &mesh = *ld->GetMesh();
			mesh.faceSel = ld->GetFaceSel();

			if (MapModesThatCanSelect())
			{
				res = mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags | SUBHIT_FACES | SUBHIT_SELSOLID, hitList);

				for (auto& rec : hitList) {
					if (ld->DoesFacePassFilter(rec.index)) vpt->LogHit(inode, mc, rec.dist, rec.index, NULL);
				}
			}
		}
		else if (ld && ld->GetPatch())
		{
			SubPatchHitList hitList;

			PatchMesh &patch = *ld->GetPatch();
			patch.patchSel = ld->GetFaceSel();

			if (MapModesThatCanSelect())
			{
				if (ignoreBackFaceCull)
					res = patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
						flags | SUBHIT_PATCH_PATCHES | SUBHIT_SELSOLID | SUBHIT_PATCH_IGNORE_BACKFACING, hitList);
				else res = patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags | SUBHIT_PATCH_PATCHES | SUBHIT_SELSOLID, hitList);

				PatchSubHitRec *rec = hitList.First();
				while (rec) {
					if (ld->DoesFacePassFilter(rec->index)) vpt->LogHit(inode, mc, rec->dist, rec->index, NULL);
					rec = rec->Next();
				}
			}
		}
		else if (ld && ld->GetMNMesh())
		{
			SubObjHitList hitList;

			MNMesh &mesh = *ld->GetMNMesh();
			mesh.FaceSelect(ld->GetFaceSel());

			if (MapModesThatCanSelect())
			{
				res = mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags | SUBHIT_MNFACES | SUBHIT_SELSOLID, hitList);

				for (auto& rec : hitList) {
					if (ld->DoesFacePassFilter(rec.index)) vpt->LogHit(inode, mc, rec.dist, rec.index, NULL);
				}
			}
		}
	}
	gw->setRndLimits(savedLimits);

	return res;
}




void UnwrapMod::SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	BitArray set;
	//BitArray tempset;
	BOOL sub = GetKeyState(VK_MENU) < 0;
	BOOL polySelect = !(GetKeyState(VK_SHIFT) < 0);

	GetCOREInterface()->ClearCurNamedSelSet();
	MeshTopoData* ld = NULL;

	if (theHold.Holding())
		HoldSelection();

	if ((fnGetMapMode() == UNFOLDMAP) &&
		fnRegularMapGetPickStartFace())
	{
		while (hitRec)
		{
			int faceIndex = hitRec->hitInfo;

			if (hitRec->Next() == NULL)
			{
				MeshTopoData *ld = (MeshTopoData*)hitRec->modContext->localData;
				INode *node = NULL;
				for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
				{
					if (ld == mMeshTopoData[ldID])
						node = mMeshTopoData.GetNode(ldID);
				}

				TSTR mstr = _T("$.modifiers[#unwrap_uvw].unwrap6.RegularMapStartNewCluster");
				macroRecorder->FunctionCall(mstr, 2, 0, mr_reftarg, node, mr_int, faceIndex + 1);
				macroRecorder->EmitScript();

				fnRegularMapStartNewCluster(node, faceIndex);
				fnRegularMapSetPickStartFace(false);
			}

			hitRec = hitRec->Next();
		}

		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());
		return;
	}
	//spline map selection
	else if (fnGetMapMode() == SPLINEMAP)
	{
		if (theHold.Holding())
			mSplineMap.HoldData();

		while (hitRec)
		{
			int splineIndex = hitRec->hitInfo;
			int index = hitRec->distance;
			BOOL state = selected;

			if (invert)
				state = !mSplineMap.CrossSectionIsSelected(splineIndex, index);
			bool newState = false;
			if (state)
				newState = true;
			else
				newState = false;
			mSplineMap.CrossSectionSelect(splineIndex, index, newState);
			TSTR functionCall;
			functionCall.printf(_T("$.modifiers[#unwrap_uvw].unwrap6.splineMap_selectCrossSection"));
			macroRecorder->FunctionCall(functionCall, 3, 0,
				mr_int, splineIndex + 1,
				mr_int, index + 1,
				mr_bool, newState);
			macroRecorder->EmitScript();
			if (!all) break;

			hitRec = hitRec->Next();
		}

		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());
		return;
	}
	//PELT
	if ((peltData.GetEditSeamsMode()))
	{

		MeshTopoData *ld = (MeshTopoData*)hitRec->modContext->localData;
		if (ld->mSeamEdges.GetSize() != ld->GetNumberGeomEdges())//TVMaps.gePtrList.Count())
		{
			ld->mSeamEdges.SetSize(ld->GetNumberGeomEdges());//TVMaps.gePtrList.Count());
			ld->mSeamEdges.ClearAll();
		}

		if (theHold.Holding())
			theHold.Put(new UnwrapPeltSeamRestore(this, ld));

		{
			while (hitRec)
			{
				set.ClearAll();
				int index = hitRec->hitInfo;
				if ((index >= 0) && (index < ld->mSeamEdges.GetSize()))
				{

					BOOL state = selected;

					if (invert)
					{
						state = !ld->mSeamEdges[hitRec->hitInfo];
					}

					if (state)
					{
						ld->mSeamEdges.Set(hitRec->hitInfo);
					}
					else
					{
						ld->mSeamEdges.Clear(hitRec->hitInfo);
					}

					if (fnGetMirrorSelectionStatus())
					{
						int mirrorIndex = ld->GetGeomEdgeMirrorIndex(hitRec->hitInfo);
						if (mirrorIndex >= 0 && mirrorIndex < ld->mSeamEdges.GetSize())
						{
							if (state)
							{
								ld->mSeamEdges.Set(mirrorIndex);
							}
							else
							{
								ld->mSeamEdges.Clear(mirrorIndex);
							}
						}
					}
					if (!all) break;
				}

				hitRec = hitRec->Next();

			}
		}


		if (fnGetMapMode() == LSCMMAP)
		{
			if (theHold.Holding())
			{
				HoldPointsAndFaces();
			}
			LSCMForceResolve();
			fnLSCMInvalidateTopo(ld);
		}

		TSTR functionCall;
		INode *node = NULL;
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			if (ld == mMeshTopoData[ldID])
				node = mMeshTopoData.GetNode(ldID);
		}
		functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.setPeltSelectedSeamsByNode"), node->GetName());
		macroRecorder->FunctionCall(functionCall, 2, 0,
			mr_bitarray, &(ld->mSeamEdges),
			mr_reftarg, node
			);
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());

	}
	else
	{
		if ((ip && (ip->GetSubObjectLevel() == 1)))
		{
			HitRecord *fistHitRec = hitRec;
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				BitArray holdSel = mMeshTopoData[ldID]->GetGeomVertSel();
				BOOL hit = FALSE;
				while (hitRec)
				{
					ld = (MeshTopoData*)hitRec->modContext->localData;

					if (ld == mMeshTopoData[ldID])
					{
						BOOL state = selected;
						//6-29--99 watje
						if (hitRec->hitInfo < ld->GetNumberGeomVerts())
						{
							if (invert)
								state = !ld->GetGeomVertSelected(hitRec->hitInfo);
							ld->SetGeomVertSelected(hitRec->hitInfo, state, mirrorGeoSelectionStatus);
							hit = TRUE;
						}

						if (!all) break;
					}

					hitRec = hitRec->Next();
				}

				if (geomElemMode && hit)
					SelectGeomElement(mMeshTopoData[ldID], !sub, &holdSel);
				hitRec = fistHitRec;
				MeshTopoData *ld = mMeshTopoData[ldID];
				TSTR functionCall;
				INode *node = mMeshTopoData.GetNode(ldID);
				functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.setSelectedGeomVertsByNode"), node->GetName());
				macroRecorder->FunctionCall(functionCall, 2, 0,
					mr_bitarray, ld->GetGeomVertSelectionPtr(),
					mr_reftarg, node
					);

			}
		}

		else if ((ip && (ip->GetSubObjectLevel() == 2)))
		{
			int flags = 0;
			if (GetKeyState(VK_SHIFT) & 0x8000) flags |= MOUSE_SHIFT;
			if (GetKeyState(VK_CONTROL) & 0x8000) flags |= MOUSE_CTRL;
			if (GetKeyState(VK_MENU) & 0x8000) flags |= MOUSE_ALT;

			// only when we select a single element.
			if (flags & MOUSE_SHIFT && hitRec && !hitRec->Next())
			{
				MouseGuidedEdgeRingSelect(hitRec, flags);
			}
			else
			{
				HitRecord *fistHitRec = hitRec;
				for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
				{
					BOOL hit = FALSE;

					BitArray holdSel;
					while (hitRec)
					{
						ld = (MeshTopoData*)hitRec->modContext->localData;
						if (ld == mMeshTopoData[ldID])
						{
							if (!hit)
								holdSel = ld->GetGeomEdgeSel();

							BOOL state = selected;
							//6-29--99 watje
							if (invert)
								state = !ld->GetGeomEdgeSelected(hitRec->hitInfo);//gesel[hitRec->hitInfo];
							if (state)
								ld->SetGeomEdgeSelected(hitRec->hitInfo, TRUE, fnGetMirrorSelectionStatus());
							else
								ld->SetGeomEdgeSelected(hitRec->hitInfo, FALSE, fnGetMirrorSelectionStatus());
							hit = TRUE;
							if (!all) break;
						}
						hitRec = hitRec->Next();
					}
					if (geomElemMode && hit)
						SelectGeomElement(mMeshTopoData[ldID], !sub, &holdSel);

					MeshTopoData *ld = mMeshTopoData[ldID];
					TSTR functionCall;
					INode *node = mMeshTopoData.GetNode(ldID);
					functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.setSelectedGeomEdgesByNode"), node->GetName());
					macroRecorder->FunctionCall(functionCall, 2, 0,
						mr_bitarray, ld->GetGeomEdgeSelectionPtr(),
						mr_reftarg, node
						);

					hitRec = fistHitRec;
				}
			}
		}
		else if ((ip && (ip->GetSubObjectLevel() == 3)) && (MapModesThatCanSelect()))
		{
			//we need to collect like LD together
			HitRecord *fistHitRec = hitRec;
			MeshTopoData *firstLD = (MeshTopoData *)fistHitRec->modContext->localData;

			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				INode *node = mMeshTopoData.GetNode(ldID);
				MeshTopoData *ld = NULL;

				BitArray holdFaceSel = mMeshTopoData[ldID]->GetFaceSel();

				BitArray saveSel;

				BOOL hit = FALSE;
				bool bSingleHit = (hitRec && !hitRec->Next()) ? true : false;
				while (hitRec)
				{
					if ((ld == NULL) && (mMeshTopoData[ldID] == hitRec->modContext->localData) || (!all))
					{
						if (all)
							ld = mMeshTopoData[ldID];
						else
							ld = firstLD;
						if (ld->GetMesh() && polySelect)
						{
							set.SetSize(ld->GetMesh()->numFaces);
						}

						if ((planarMode) && (sub))  //hmm hack alert since we are doing a planar selection removal we need to trick the system
						{
							saveSel.SetSize(ld->GetFaceSel().GetSize());  //save our current sel
							saveSel = ld->GetFaceSel();					  //then clear it ans set the system to normal select
							BitArray clearSel = ld->GetFaceSel();
							clearSel.ClearAll();				  //we will then fix the selection after the hitrecs
							ld->SetFaceSel(clearSel);
							selected = TRUE;
							hit = TRUE;
						}
					}
					if (ld && (mMeshTopoData[ldID] == hitRec->modContext->localData) || (!all))
					{
						if (ld->GetMesh() && polySelect)
						{
							set.ClearAll();

							BOOL degenerateFace = FALSE;
							for (int k = 0; k < 3; k++)
							{
								int idA = ld->GetFaceGeomVert(hitRec->hitInfo, k);
								int idB = ld->GetFaceGeomVert(hitRec->hitInfo, (k + 1) % 3);
								if (idA == idB)
									degenerateFace = TRUE;
							}

							if (degenerateFace)
							{
								set.SetSize(ld->GetNumberFaces());
								set.ClearAll();
								set.Set(hitRec->hitInfo);
							}
							else
							{
								ld->GetPolyFromFaces(hitRec->hitInfo, set, fnGetMirrorSelectionStatus());
							}
							BitArray faceSel = ld->GetFaceSel();

							if (invert)
								faceSel ^= set;
							else if (selected)
								faceSel |= set;
							else
								faceSel &= ~set;
							ld->SetFaceSelectionByRef(faceSel, FALSE);

							hit = TRUE;
							if (!all)
							{
								ldID = mMeshTopoData.Count();
								break;
							}
						}
						else
						{
							BOOL state = selected;
							bool shift_pressed = GetKeyState(VK_SHIFT) & 0x8000 ? true : false;
							if (shift_pressed && bSingleHit)
							{
								MNMesh* pMNMesh = ld->GetMNMesh();
								if (pMNMesh)
								{
									IMNMeshUtilities13* l_mesh13 = static_cast<IMNMeshUtilities13*>(pMNMesh->GetInterface(IMNMESHUTILITIES13_INTERFACE_ID));
									BitArray curSel = ld->GetFaceSel();
									BitArray newSel;
									auto arraySize = curSel.GetSize();
									newSel.SetSize(arraySize);
									newSel.ClearAll();
									newSel.Set(hitRec->hitInfo);
									if (l_mesh13->FindLoopFace(newSel, curSel))
									{
										if (fnGetFilteredSelected())
										{
											for (auto i = 0; i < arraySize; ++i)
											{
												if (newSel[i] && !ld->DoesFacePassFilter(i))
												{
													newSel.Clear(i);
												}
											}
										}
										if (fnGetMirrorSelectionStatus()) //only mirror the newly selected selections
										{
											ld->SetFaceSelectionByRef(newSel, true);
											newSel = ld->GetFaceSel();
										}
										curSel |= newSel;
										ld->SetFaceSelectionByRef(curSel, fnGetMirrorSelectionStatus());
									}
								}
							}
							else
							{
								//6-29--99 watje
								if (invert)
									state = !ld->GetFaceSelected(hitRec->hitInfo);
								ld->SetFaceSelected(hitRec->hitInfo, state, fnGetMirrorSelectionStatus());
								hit = TRUE;
								if (!all)
								{
									ldID = mMeshTopoData.Count();
									break;
								}
							}
						}
					}
					hitRec = hitRec->Next();
				}

				hitRec = fistHitRec;

				if (ld)
				{
					if (planarMode)
						SelectGeomFacesByAngle(ld);

					if ((planarMode) && (sub))
					{
						saveSel &= ~ld->GetFaceSel();//faceSel;
						ld->SetFaceSelectionByRef(saveSel);
						//					d->faceSel = saveSel;
					}

					if (geomElemMode && hit)
						SelectGeomElement(ld, !sub, &holdFaceSel);


					TSTR functionCall;
					CleanUpGeoSelection(ld);
					functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode"), node->GetName());
					macroRecorder->FunctionCall(functionCall, 2, 0,
						mr_bitarray, ld->GetFaceSelectionPtr(),
						mr_reftarg, node
						);
				}
			}
			InvalidateView();
		}

		if (fnGetSyncSelectionMode())
			fnSyncTVSelection();
		RebuildDistCache();
		theHold.Accept(GetString(IDS_DS_SELECT));

		if (sub == TRUE)
		{
			// Alt + Selection would be de-selection and to erase the open edges in peel mode.
			EraseOpenEdgesByDeSelection(hitRec);
		}
		else
		{
			AutoEdgeToSeamAndLSCMResolve();
		}

		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		SetNumSelLabel();
	}
}

void UnwrapMod::ClearSelection(int selLevel)
{

	if (fnGetMapMode() == SPLINEMAP)
	{
		INode *splineNode = NULL;
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->GetValue(unwrap_splinemap_node, t, splineNode, FOREVER);
		BOOL displaySplineMap = FALSE;
		pblock->GetValue(unwrap_splinemap_display, t, displaySplineMap, FOREVER);
		if (splineNode && displaySplineMap)
		{
			if (theHold.Holding())
				mSplineMap.HoldData();

			fnSplineMap_ClearCrossSectionSelection();
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap6.SplineMap_ClearSelectCrossSection"), 0, 0);


			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			if (ip) ip->RedrawViews(ip->GetTime());
			return;
		}
	}

	//PELT
	//we dont clear the selection when in seams mode since the user can easily wipe out 
	//his previous seams
	if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
	{
		return;
	}
	if ((peltData.GetPointToPointSeamsMode()))
	{
		return;
	}

	// in 3dsmax mode, shift is used to select loop/ring. in maya mode shift is used to invert selection.
	// in neither case should we clear the current selection.
	// TO FIX: unwrap actually doesn't support MAYA mode, a legacy issue...
	if ((GetKeyState(VK_SHIFT) & 0x8000) && selLevel == 3)
		return;

	//loop through the local data list
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (!ld)
			continue;


		//hold our current selection
		if (theHold.Holding())
			theHold.Put(new TSelRestore(ld));

		switch (selLevel)
		{
		case 1:
		{
			BitArray gvsel = ld->GetGeomVertSel();
			gvsel.ClearAll();
			ld->SetGeomVertSel(gvsel);


			BitArray vsel = ld->GetTVVertSel();
			vsel.ClearAll();
			ld->SetTVVertSel(vsel);
			break;
		}
		case 2:
		{
			bool shift_pressed = GetKeyState(VK_SHIFT) & 0x8000 ? true : false;
			if (!shift_pressed)
			{
				BitArray gesel = ld->GetGeomEdgeSel();
				gesel.ClearAll();
				ld->SetGeomEdgeSel(gesel);
			}

			break;
		}
		case 3:
		{
			if (MapModesThatCanSelect())
			{
				BitArray fsel = ld->GetFaceSel();
				fsel.ClearAll();
				ld->SetFaceSel(fsel);
			}
			break;
		}
		}
	}


	theHold.Suspend();
	if (fnGetSyncSelectionMode())
		fnSyncTVSelection();
	theHold.Resume();


	GetCOREInterface()->ClearCurNamedSelSet();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	SetNumSelLabel();
	//update our views to show new faces
	InvalidateView();


}
void UnwrapMod::SelectAll(int selLevel)
{
	if (fnGetMapMode() == SPLINEMAP)
	{
		INode *splineNode = NULL;
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->GetValue(unwrap_splinemap_node, t, splineNode, FOREVER);
		BOOL displaySplineMap = FALSE;
		pblock->GetValue(unwrap_splinemap_display, t, displaySplineMap, FOREVER);
		if (splineNode && displaySplineMap)
		{
			{
				if (theHold.Holding())
					mSplineMap.HoldData();
				for (int i = 0; i < mSplineMap.NumberOfSplines(); i++)
				{
					if (mSplineMap.IsSplineSelected(i))
					{
						{
							for (int j = 0; j < mSplineMap.NumberOfCrossSections(i); j++)
								mSplineMap.CrossSectionSelect(i, j, TRUE);
						}
					}
				}
				NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
				TimeValue t = GetCOREInterface()->GetTime();
				if (ip) ip->RedrawViews(t);
				return;
			}
		}
	}

	//PELT
	if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			ld->mSeamEdges.SetAll();
		}
		//		peltData.seamEdges.SetAll();
		return;
	}
	if ((peltData.GetPeltMapMode()) && (peltData.GetPointToPointSeamsMode()))
	{
		return;
	}

	if (theHold.Holding())
		HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (!ld)
			continue;


		switch (selLevel)
		{
		case 1:
			ld->AllSelection(TVVERTMODE);
			break;
		case 2:
			ld->AllSelection(TVEDGEMODE);
			break;
		case 3:
			ld->AllSelection(TVFACEMODE);
			break;
		}

	}

	GetCOREInterface()->ClearCurNamedSelSet();
	if (fnGetSyncSelectionMode())
		fnSyncTVSelection();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	SetNumSelLabel();
	//update our views to show new faces

	InvalidateView();
}

void UnwrapMod::InvertSelection(int selLevel)
{
	if (fnGetMapMode() == SPLINEMAP)
	{
		INode *splineNode = NULL;
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->GetValue(unwrap_splinemap_node, t, splineNode, FOREVER);
		BOOL displaySplineMap = FALSE;
		pblock->GetValue(unwrap_splinemap_display, t, displaySplineMap, FOREVER);
		if (splineNode && displaySplineMap)
		{
			if (theHold.Holding())
				mSplineMap.HoldData();
			for (int i = 0; i < mSplineMap.NumberOfSplines(); i++)
			{
				if (mSplineMap.IsSplineSelected(i))
				{
					{
						for (int j = 0; j < mSplineMap.NumberOfCrossSections(i); j++)
						{
							if (mSplineMap.CrossSectionIsSelected(i, j))
								mSplineMap.CrossSectionSelect(i, j, FALSE);
							else
								mSplineMap.CrossSectionSelect(i, j, TRUE);

						}
					}
				}
			}
			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			TimeValue t = GetCOREInterface()->GetTime();
			if (ip) ip->RedrawViews(t);

			return;
		}
	}

	//PELT
	if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
		return;

	if (theHold.Holding())
		HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (!ld)
			continue;
		switch (selLevel)
		{
		case 1:
		{
			ld->InvertSelection(TVVERTMODE);
			break;
		}
		case 2:
		{
			ld->InvertSelection(TVEDGEMODE);
			break;
		}
		case 3:
		{
			ld->InvertSelection(TVFACEMODE);
			break;
		}
		}
	}

	GetCOREInterface()->ClearCurNamedSelSet();
	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();// invertion above affect tv data, so here sync geom data.
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	SetNumSelLabel();
	//update our views to show new faces
	InvalidateView();


}

bool UnwrapMod::ClearTVSelectionPreview()
{
	bool bCleared = false;
	int subLevel = GetSubObjectLevel();
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData* ld = GetMeshTopoData(ldID);
		if (ld && (!ld->GetTVVertSelectionPreview().IsEmpty() || !ld->GetTVEdgeSelectionPreview().IsEmpty()
			|| !ld->GetFaceSelectionPreview().IsEmpty()))
		{
			ld->ClearSelectionPreview(subLevel);
			bCleared = true;
		}
	}
	return bCleared;
}

bool UnwrapMod::ConvertPreviewSelToSelected()
{
	int subLevel = GetSubObjectLevel();
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData* ld = GetMeshTopoData(ldID);
		if (subLevel == TVVERTMODE)
		{
			//vertex
			if (ld && !ld->GetTVVertSelectionPreview().IsEmpty())
			{
				BitArray selectionArray = ld->GetTVVertSel();
				BitArray selectionPreviewArray = ld->GetTVVertSelectionPreview();
				selectionArray |= selectionPreviewArray;
				ld->SetTVVertSel(selectionArray);
			}
		}
		else if (subLevel == TVEDGEMODE)
		{
			// edge
			if (ld && !ld->GetTVEdgeSelectionPreview().IsEmpty())
			{
				BitArray selectionArray = ld->GetTVEdgeSel();
				BitArray selectionPreviewArray = ld->GetTVEdgeSelectionPreview();
				selectionArray |= selectionPreviewArray;
				ld->SetTVEdgeSel(selectionArray);
			}
		}
		else if (subLevel == TVFACEMODE)
		{
			// face
			if (ld && !ld->GetFaceSelectionPreview().IsEmpty())
			{
				BitArray selectionArray = ld->GetFaceSel();
				BitArray selectionPreviewArray = ld->GetFaceSelectionPreview();
				selectionArray |= selectionPreviewArray;
				ld->SetFaceSelectionByRef(selectionArray);
			}
		}

	}
	return true;
}


void UnwrapMod::ActivateSubobjSel(int level, XFormModes& modes) {
	// Fill in modes with our sub-object modes

	if (level == 0)
	{
		fnSetTVSubMode(TVOBJECTMODE);
		if ((fnGetMapMode() != PELTMAP) && (fnGetMapMode() != UNFOLDMAP) && (fnGetMapMode() != LSCMMAP))
			fnSetMapMode(NOMAP);
		if (peltData.GetPointToPointSeamsMode())
			peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);

		modes = XFormModes(NULL, NULL, NULL, NULL, NULL, selectMode);
		EnableMapButtons(FALSE);
		EnableFaceSelectionUI(FALSE);
		EnableEdgeSelectionUI(FALSE);
		EnableSubSelectionUI(FALSE);
		if (fnGetMapMode() == PELTMAP)
			EnableAlignButtons(FALSE);
		peltData.EnablePeltButtons(this, hMapParams, FALSE);
		peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);
		peltData.SetEditSeamsMode(this, FALSE);


	}
	else if (level == 1)
	{

		fnSetTVSubMode(TVVERTMODE);
		if (peltData.GetPointToPointSeamsMode())
			peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);

		SetupNamedSelDropDown();
		if ((fnGetMapMode() != PELTMAP) && (fnGetMapMode() != UNFOLDMAP) && (fnGetMapMode() != LSCMMAP))
			fnSetMapMode(NOMAP);
		modes = XFormModes(NULL, NULL, NULL, NULL, NULL, selectMode);
		EnableMapButtons(FALSE);
		EnableFaceSelectionUI(FALSE);
		EnableEdgeSelectionUI(FALSE);
		EnableSubSelectionUI(TRUE);
		if (fnGetMapMode() == PELTMAP)
			EnableAlignButtons(FALSE);
		peltData.EnablePeltButtons(this, hMapParams, TRUE);

		peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);
		peltData.SetEditSeamsMode(this, FALSE);

	}
	else if (level == 2)
	{

		fnSetTVSubMode(TVEDGEMODE);
		SetupNamedSelDropDown();
		if ((fnGetMapMode() != PELTMAP) && (fnGetMapMode() != UNFOLDMAP) && (fnGetMapMode() != LSCMMAP))
			fnSetMapMode(NOMAP);
		modes = XFormModes(NULL, NULL, NULL, NULL, NULL, selectMode);
		EnableMapButtons(FALSE);
		EnableFaceSelectionUI(FALSE);
		EnableEdgeSelectionUI(TRUE);
		EnableSubSelectionUI(TRUE);
		if (fnGetMapMode() == PELTMAP)
			EnableAlignButtons(FALSE);
		peltData.EnablePeltButtons(this, hMapParams, TRUE);

		peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);
		peltData.SetEditSeamsMode(this, FALSE);

	}
	else if (level == 3)
	{
		if (peltData.GetPointToPointSeamsMode())
			peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);

		fnSetTVSubMode(TVFACEMODE);
		SetupNamedSelDropDown();
		//face select
		if (fnGetMapMode() == NOMAP)
			modes = XFormModes(NULL, NULL, NULL, NULL, NULL, selectMode);
		else if (fnGetMapMode() == SPLINEMAP)
		{
			modes = XFormModes(moveGizmoMode, rotGizmoMode, nuscaleGizmoMode, uscaleGizmoMode, squashGizmoMode, NULL);
			EnableAlignButtons(FALSE);
		}
		else modes = XFormModes(moveGizmoMode, rotGizmoMode, nuscaleGizmoMode, uscaleGizmoMode, squashGizmoMode, NULL);
		EnableMapButtons(TRUE);
		EnableFaceSelectionUI(TRUE);
		EnableEdgeSelectionUI(FALSE);
		EnableSubSelectionUI(TRUE);

		if (fnGetMapMode() == PELTMAP)
		{
			if (peltData.peltDialog.hWnd)
				EnableAlignButtons(FALSE);
			else EnableAlignButtons(TRUE);
		}
		peltData.EnablePeltButtons(this, hMapParams, TRUE);

		peltData.SetPointToPointSeamsMode(this, FALSE, TRUE);
		peltData.SetEditSeamsMode(this, FALSE);

	}

	SetNumSelLabel();

	UpdatePivot();
	freeFormPivotOffset = Point3(0, 0, 0);


	InvalidateView();
	NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mUIManager.UpdateCheckButtons();
}


Mtl* UnwrapMod::GetMultiMaterials(Mtl *base)
{
	Tab<Mtl*> materialStack;
	materialStack.Append(1, &base);
	while (materialStack.Count() != 0)
	{
		Mtl* topMaterial = materialStack[0];
		materialStack.Delete(0, 1);
		//append any mtl
		for (int i = 0; i < topMaterial->NumSubMtls(); i++)
		{
			Mtl* addMat = topMaterial->GetSubMtl(i);
			if (addMat)
				materialStack.Append(1, &addMat, 100);
		}


		if (topMaterial->ClassID() == Class_ID(MULTI_CLASS_ID, 0))
			return topMaterial;
	}
	return NULL;
}

void UnwrapMod::BuildMatIDList()
{
	filterMatID.ZeroCount();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		for (int i = 0; i < ld->GetNumberFaces(); i++)
		{
			int found = 0;
			if (!ld->GetFaceDead(i))
			{
				for (int j = 0; j < filterMatID.Count(); j++)
				{
					if (filterMatID[j] == ld->GetFaceMatID(i)) 
					{
						found = 1;
						j = filterMatID.Count();
					}
				}
			}
			else
			{
				found = 1;
			}

			if (found == 0)
			{
				int id = ld->GetFaceMatID(i);
				filterMatID.Append(1, &id);
			}
		}
	}

	filterMatID.Sort(CompTable);
}


void UnwrapMod::ComputeFalloff(float &u, int ftype)

{
	if (u <= 0.0f) u = 0.0f;
	else if (u >= 1.0f) u = 1.0f;
	else switch (ftype)
	{
	case (3) : u = u*u*u; break;
		//	case (BONE_FALLOFF_X2_FLAG) : u = u*u; break;
	case (0) : u = u; break;
	case (1) : u = 1.0f - ((float)cos(u*PI) + 1.0f)*0.5f; break;
		//	case (BONE_FALLOFF_2X_FLAG) : u = (float) sqrt(u); break;
	case (2) : u = (float)pow(u, 0.3f); break;

	}

}


void UnwrapMod::DisableUI()
{
	EnableMapButtons(FALSE);
	EnableFaceSelectionUI(FALSE);
	EnableEdgeSelectionUI(FALSE);
	EnableSubSelectionUI(FALSE);
	EnableAlignButtons(FALSE);

	peltData.EnablePeltButtons(this, hMapParams, FALSE);


	mUIManager.Enable(ID_QUICKMAP_ALIGN, FALSE);
	mUIManager.Enable(ID_EDGELOOPSELECTION, FALSE);
	mUIManager.Enable(ID_EDIT, FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_UNWRAP_RESET), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_UNWRAP_SAVE), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_UNWRAP_LOAD), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_MAP_CHAN1), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_MAP_CHAN2), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_DONOTREFLATTEN_CHECK), FALSE);

	EnableWindow(GetDlgItem(hParams, IDC_ALWAYSSHOWPELTSEAMS_CHECK), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_SHOWMAPSEAMS_CHECK), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_THINSEAM), FALSE);
	EnableWindow(GetDlgItem(hParams, IDC_THICKSEAM), FALSE);
}

void UnwrapMod::ModifyObject(
	TimeValue t, ModContext &mc, ObjectState *os, INode *node)
{
	//if we have the apply to whole object flip the subobject state
	if (applyToWholeObject)
	{
		if (os->obj->IsSubClassOf(patchObjectClassID))
		{
			PatchObject *pobj = (PatchObject*)os->obj;
			pobj->patch.selLevel = PATCH_OBJECT;
			pobj->patch.patchSel.ClearAll();
		}
		else
			if (os->obj->IsSubClassOf(triObjectClassID))
			{
				//				isMesh = TRUE;
				TriObject *tobj = (TriObject*)os->obj;
				tobj->GetMesh().selLevel = MESH_OBJECT;
				tobj->GetMesh().faceSel.ClearAll();
			}
			else if (os->obj->IsSubClassOf(polyObjectClassID))
			{
				PolyObject *pobj = (PolyObject*)os->obj;
				pobj->GetMesh().selLevel = MNM_SL_OBJECT;
				BitArray s;
				pobj->GetMesh().getFaceSel(s);
				s.ClearAll();
				pobj->GetMesh().FaceSelect(s);
			}
	}

	//poll for material on mesh
	int CurrentChannel = 0;

	if (channel == 0)
	{
		CurrentChannel = 1;
		//should be from scroller;
	}
	else if (channel == 1)
	{
		CurrentChannel = 0;
	}
	else CurrentChannel = channel;

	currentTime = t;

	// Prepare the controller and set up mats
	// Get our local data
	MeshTopoData *d = (MeshTopoData*)mc.localData;
	//if we dont have local data we need to build it and initialize it
	if (d == NULL)
	{
		d = new MeshTopoData(os, CurrentChannel);
		mc.localData = d;
		bMeshTopoDataContainerDirty = TRUE;
	}

	//if we are not a mesh, poly mesh or patch convert and copy it into our object
	if (
		(!os->obj->IsSubClassOf(patchObjectClassID)) &&
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)))
	{
		//neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TriObject *tri = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			os->obj = tri;
			os->obj->UnlockObject();
		}
	}

	//this just makes sure we have some cache geo data since onload the cache meshes 
	//are null which triggers a reset
	d->SetGeoCache(os);

	//next check to make sure we have a valid mesh cache in the local data
	//we need this the mesh topology and or type can change
	BOOL reset = d->HasCacheChanged(os);

	//if the topochanges and the user has forceUpdate off do nothing
	//this is for cases when they apply unwrap to an object that has changed topology but
	//will come back at render time	
	if ((reset) && (!forceUpdate))
	{
		os->obj->UpdateValidity(TEXMAP_CHAN_NUM, GetValidity(t));
		return;
	}
	else if (reset)
	{
		//this will rebuild all the caches and put back the original mapping or apply a default one if one does not exist
		d->Reset(os, CurrentChannel);
	}

	//update our geo list since this can change so we need to update
	//this includes hidden faces and geometry point position
	bool geoVertChanged = d->UpdateGeoData(t, os);
	//this pushes the data on any of the controller back into the local data
	UpdateControllers(t, d);

	// this fills out local data list
	if (bMeshTopoDataContainerDirty && !mMeshTopoData.Lock())
	{
		MyEnumProc dep;
		DoEnumDependents(&dep);

		ClearMeshTopoDataContainer();
		for (int i = 0; i < dep.Nodes.Count(); i++)
		{
			if (dep.Nodes[i]->Selected())
				IsInStack(dep.Nodes[i]);
		}

		if (dep.Nodes.Count() == 0) //running  Unwrap not hooked up to a node
			mMeshTopoData.Append(1, d, nullptr, 10);

		mMeshTopoData.Unlock();
		bMeshTopoDataContainerDirty = FALSE;
	}

	if (reset || geoVertChanged)
	{
		d->InvalidateMirrorData();
		d->ReleaseAdjFaceList();
		d->InvalidateMeshPolyData();
	}

	if (mirrorGeoSelectionStatus)
	{
		INode* node = nullptr;
		for (auto i = 0; i < mMeshTopoData.Count() && node == nullptr; ++i)
		{
			if (mMeshTopoData[i] == d)
			{
				node = mMeshTopoData.GetNode(i);
			}
		}
		d->BuildMirrorData(mirrorGeoSelectionAxis, fnGetTVSubMode(), fnGetMirrorThreshold(), node);
	}

	//see if we old Max data in the modifier and if so move it to the local data
	if (mTVMaps_Max9.f.Count() || mTVMaps_Max9.v.Count() && d->IsLoaded())
	{
		d->ResolveOldData(mTVMaps_Max9);
		//fix up the controllers now since we dont keep a full list any more
		for (int i = 0; i < mTVMaps_Max9.v.Count(); i++)
		{
			if (i < mUVWControls.cont.Count())
			{
				if (mUVWControls.cont[i])
					d->SetTVVertControlIndex(i, i);
			}
		}
		d->ClearLoaded();
	}

	//check to see if all the tv are cleared and if so clear the max9 data
	BOOL clearOldData = TRUE;
	for (int i = 0; i < mMeshTopoData.Count(); i++)
	{
		if (mMeshTopoData[i]->IsLoaded())
			clearOldData = FALSE;
	}
	if (clearOldData)
	{
		mTVMaps_Max9.FreeEdges();
		mTVMaps_Max9.FreeFaces();
		mTVMaps_Max9.v.SetCount(0);
		mTVMaps_Max9.f.SetCount(0);
	}

	//if the user flipped this flag get the selection from the stack and copy it to our cache and uvw faces
	if (getFaceSelectionFromStack)
	{
		if (os->obj->IsSubClassOf(patchObjectClassID))
		{
			GetFaceSelectionFromPatch(os, mc, t);
		}
		else
			if (os->obj->IsSubClassOf(triObjectClassID))
			{
				GetFaceSelectionFromMesh(os, mc, t);
			}
			else if (os->obj->IsSubClassOf(polyObjectClassID))
			{
				GetFaceSelectionFromMNMesh(os, mc, t);
			}

		getFaceSelectionFromStack = FALSE;
	}

	//this inits our tm control for the mapping gizmos
	if (!tmControl || (flags&CONTROL_OP) || (flags&CONTROL_INITPARAMS))
		InitControl(t);

	//now copy our selection into the object flowing up the stack for display purposes
	d->CopyFaceSelection(os);
	d->CopyMatID(os, t);
	d->ApplyMapping(os, CurrentChannel, t);
	// since unwrap mod use os.obj to display sub objs, and nitrous displays them based on selLevels
	// while the ideal way should be based on DISP_FLAGs, so we have to hack the selLevel if the current modifier
	// is the active one.
	if (this == editMod)
	{
		d->ApplyMeshSelectionLevel(os);
	}

	//Read to refill the render items in the container
	mRenderItemsContainer.clear();

	Interval iv = GetValidity(t);
	iv = iv & os->obj->ChannelValidity(t, GEOM_CHAN_NUM);
	iv = iv & os->obj->ChannelValidity(t, TOPO_CHAN_NUM);
	os->obj->UpdateValidity(TEXMAP_CHAN_NUM, iv);

	// support specified normal since unwrap mod doesn't change any normal info
	if (os->obj->IsSubClassOf(triObjectClassID))
	{
		TriObject *triOb = (TriObject*)os->obj;
		MeshNormalSpec *pNormals = triOb->GetMesh().GetSpecifiedNormals();
		if (pNormals && pNormals->GetNumFaces())
		{
			pNormals->SetParent(&(triOb->GetMesh()));
			pNormals->CheckNormals(); // rebuild if necessary.
			// inform the system that this modifier has supported the specified normals.
			pNormals->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT);
		}
	}
	
	if ((popUpDialog) && (alwaysEdit))
		fnEdit();
	popUpDialog = FALSE;

	if (IsWindow(hDialogWnd) && filterMatID.Count() == 0)// if uv editor is open
	{
		UpdateMatFilters();
	}
}

Interval UnwrapMod::LocalValidity(TimeValue t)
{
	// aszabo|feb.05.02 If we are being edited, 
	// return NEVER to forces a cache to be built after previous modifier.
	if (TestAFlag(A_MOD_BEING_EDITED))
		return NEVER;

	return GetValidity(t);
}

//aszabo|feb.06.02 - When LocalValidity is called by ModifyObject,
// it returns NEVER and thus the object channels are marked non valid
// As a result, the mod stack enters and infinite evaluation of the modifier
// ModifyObject now calls GetValidity and CORE calls LocalValidity to
// allow for building a cache on the input of this modifier when it's 
// being edited 
Interval UnwrapMod::GetValidity(TimeValue t)
{

	Interval iv = FOREVER;
	for (int i = 0; i < mUVWControls.cont.Count(); i++) {
		if (mUVWControls.cont[i])
		{
			Point3 p;
			mUVWControls.cont[i]->GetValue(t, &p, iv);
		}
	}
	return iv;
}



RefTargetHandle UnwrapMod::GetReference(int i)
{
	if (i == 0) return tmControl;
	else if (i < 91) return  map[i - 1];
	else if (i == 95) return pblock;
	else if (i > 110) return mUVWControls.cont[i - 11 - 100];
	else if (i == 100) return checkerMat;
	else if (i == 101) return originalMat;
	return NULL;
}



void UnwrapMod::SetReference(int i, RefTargetHandle rtarg)
{
	if (i == 0) tmControl = (Control*)rtarg;
	else if (i < 91) map[i - 1] = (Texmap*)rtarg;
	else if (i == 95)  pblock = (IParamBlock2*)rtarg;
	else if (i == 100) checkerMat = (StdMat*)rtarg;
	else if (i == 101) originalMat = (ReferenceTarget*)rtarg;
	else
	{
		int index = i - 11 - 100;
		if (index >= mUVWControls.cont.Count())
		{
			int startIndex = mUVWControls.cont.Count();
			mUVWControls.cont.SetCount(index + 1);
			for (int id = startIndex; id < mUVWControls.cont.Count(); id++)
				mUVWControls.cont[id] = NULL;
		}
		mUVWControls.cont[index] = (Control*)rtarg;
	}


}
// ref 0 - the tm control for gizmo
// ref 1-90 - map references
// ref 100 the checker texture
// ref 111+ the vertex controllers

int UnwrapMod::RemapRefOnLoad(int iref)
{
	if (version == 1)
	{
		if (iref == 0)
			return 1;
		else if (iref > 0)
			return iref + 10 + 100;

	}
	else if (version < 8)
	{
		if (iref > 10)
			return iref + 100;
	}
	return iref;
}

Animatable* UnwrapMod::SubAnim(int i)
{
	return mUVWControls.cont[i];
}

TSTR UnwrapMod::SubAnimName(int i)
{
	TSTR buf;
	//	buf.printf(_T("Point %d"),i+1);
	buf.printf(_T("%s %d"), GetString(IDS_PW_POINT), i + 1);
	return buf;
}


RefTargetHandle UnwrapMod::Clone(RemapDir& remap)
{
	UnwrapMod *mod = new UnwrapMod;

	mod->zoom = zoom;
	mod->aspect = aspect;
	mod->xscroll = xscroll;
	mod->yscroll = yscroll;
	mod->uvw = uvw;
	mod->showMap = showMap;
	mod->update = update;
	mod->lineColor = lineColor;
	mod->openEdgeColor = openEdgeColor;
	mod->selColor = selColor;
	mod->selPreviewColor = selPreviewColor;
	mod->peelColor = peelColor;
	mod->rendW = rendW;
	mod->rendH = rendH;
	mod->isBitmap = isBitmap;
	mod->useBitmapRes = useBitmapRes;
	mod->displayPixelUnits = displayPixelUnits;
	mod->channel = channel;
	mod->ReplaceReference(PBLOCK_REF, remap.CloneRef(pblock));
	mod->ReplaceReference(0, remap.CloneRef(tmControl));


	mod->mUVWControls.cont.SetCount(mUVWControls.cont.Count());
	for (int i = 0; i < mUVWControls.cont.Count(); i++) {
		mod->mUVWControls.cont[i] = NULL;
		if (mUVWControls.cont[i]) mod->ReplaceReference(i + 11 + 100, remap.CloneRef(mUVWControls.cont[i]));
	}

	/*
	if (instanced)
	{
	for (int i=0; i<mod->TVMaps.cont.Count(); i++) mod->DeleteReference(i+11+100);
	mod->TVMaps.v.Resize(0);
	mod->TVMaps.f.Resize(0);
	mod->TVMaps.cont.Resize(0);
	mod->vsel.SetSize(0);
	mod->updateCache = TRUE;
	mod->instanced = FALSE;
	}
	*/
	mod->loadDefaults = FALSE;
	mod->mirrorGeoSelectionStatus = mirrorGeoSelectionStatus;
	mod->mirrorGeoSelectionAxis = mirrorGeoSelectionAxis;

	mod->showMultiTile = showMultiTile;

	BaseClone(this, mod, remap);
	return mod;
}

#define NAMEDSEL_STRING_CHUNK	0x2809
#define NAMEDSEL_ID_CHUNK		0x2810

#define NAMEDVSEL_STRING_CHUNK	0x2811
#define NAMEDVSEL_ID_CHUNK		0x2812

#define NAMEDESEL_STRING_CHUNK	0x2813
#define NAMEDESEL_ID_CHUNK		0x2814


RefResult UnwrapMod::NotifyRefChanged(
	const Interval& changeInt,
	RefTargetHandle hTarget,
	PartID& partID,
	RefMessage message,
	BOOL propagate)
{

	if (suspendNotify) return REF_STOP;

	switch (message)
	{
	case REFMSG_CHANGE:
		if (editMod == this && hView)
		{
			InvalidateView();
		}
		break;



	}
	return REF_SUCCEED;
}




IOResult UnwrapMod::Save(ISave *isave)
{
	ULONG nb = 0;
	Modifier::Save(isave);
	version = CURRENTVERSION;


	isave->BeginChunk(ZOOM_CHUNK);
	isave->Write(&zoom, sizeof(zoom), &nb);
	isave->EndChunk();

	isave->BeginChunk(ASPECT_CHUNK);
	isave->Write(&aspect, sizeof(aspect), &nb);
	isave->EndChunk();

	isave->BeginChunk(XSCROLL_CHUNK);
	isave->Write(&xscroll, sizeof(xscroll), &nb);
	isave->EndChunk();

	isave->BeginChunk(YSCROLL_CHUNK);
	isave->Write(&yscroll, sizeof(yscroll), &nb);
	isave->EndChunk();

	isave->BeginChunk(IWIDTH_CHUNK);
	isave->Write(&rendW, sizeof(rendW), &nb);
	isave->EndChunk();

	isave->BeginChunk(IHEIGHT_CHUNK);
	isave->Write(&rendH, sizeof(rendH), &nb);
	isave->EndChunk();

	isave->BeginChunk(UVW_CHUNK);
	isave->Write(&uvw, sizeof(uvw), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWMAP_CHUNK);
	isave->Write(&showMap, sizeof(showMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(UPDATE_CHUNK);
	isave->Write(&update, sizeof(update), &nb);
	isave->EndChunk();

	isave->BeginChunk(LINECOLOR_CHUNK);
	isave->Write(&lineColor, sizeof(lineColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(SELCOLOR_CHUNK);
	isave->Write(&selColor, sizeof(selColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(CHANNEL_CHUNK);
	isave->Write(&channel, sizeof(channel), &nb);
	isave->EndChunk();

	isave->BeginChunk(PREFS_CHUNK);
	isave->Write(&lineColor, sizeof(lineColor), &nb);
	isave->Write(&selColor, sizeof(selColor), &nb);
	isave->Write(&weldThreshold, sizeof(weldThreshold), &nb);
	isave->Write(&update, sizeof(update), &nb);
	isave->Write(&showVerts, sizeof(showVerts), &nb);
	isave->Write(&pixelCenterSnap, sizeof(pixelCenterSnap), &nb);
	isave->EndChunk();

	if (namedSel.Count()) {
		isave->BeginChunk(0x2806);
		for (int i = 0; i < namedSel.Count(); i++)
		{
			isave->BeginChunk(NAMEDSEL_STRING_CHUNK);
			isave->WriteWString(*namedSel[i]);
			isave->EndChunk();

			isave->BeginChunk(NAMEDSEL_ID_CHUNK);
			isave->Write(&ids[i], sizeof(DWORD), &nb);
			isave->EndChunk();
		}

		for (int i = 0; i < namedVSel.Count(); i++)
		{
			isave->BeginChunk(NAMEDVSEL_STRING_CHUNK);
			isave->WriteWString(*namedVSel[i]);
			isave->EndChunk();

			isave->BeginChunk(NAMEDVSEL_ID_CHUNK);
			isave->Write(&idsV[i], sizeof(DWORD), &nb);
			isave->EndChunk();
		}

		for (int i = 0; i < namedESel.Count(); i++)
		{
			isave->BeginChunk(NAMEDESEL_STRING_CHUNK);
			isave->WriteWString(*namedESel[i]);
			isave->EndChunk();

			isave->BeginChunk(NAMEDESEL_ID_CHUNK);
			isave->Write(&idsE[i], sizeof(DWORD), &nb);
			isave->EndChunk();
		}

		isave->EndChunk();
	}
	if (useBitmapRes)
	{
		isave->BeginChunk(USEBITMAPRES_CHUNK);
		isave->EndChunk();
	}



	isave->BeginChunk(LOCKASPECT_CHUNK);
	isave->Write(&lockAspect, sizeof(lockAspect), &nb);
	isave->EndChunk();

	isave->BeginChunk(MAPSCALE_CHUNK);
	isave->Write(&mapScale, sizeof(mapScale), &nb);
	isave->EndChunk();

	isave->BeginChunk(WINDOWPOS_CHUNK);
	isave->Write(&windowPos, sizeof(WINDOWPLACEMENT), &nb);
	isave->EndChunk();

	isave->BeginChunk(FORCEUPDATE_CHUNK);
	isave->Write(&forceUpdate, sizeof(forceUpdate), &nb);
	isave->EndChunk();



	if (fnGetTile())
	{
		isave->BeginChunk(TILE_CHUNK);
		isave->EndChunk();
	}

	isave->BeginChunk(TILECONTRAST_CHUNK);
	isave->Write(&fContrast, sizeof(fContrast), &nb);
	isave->EndChunk();

	isave->BeginChunk(TILELIMIT_CHUNK);
	isave->Write(&iTileLimit, sizeof(iTileLimit), &nb);
	isave->EndChunk();

	isave->BeginChunk(SOFTSELLIMIT_CHUNK);
	isave->Write(&limitSoftSel, sizeof(limitSoftSel), &nb);
	isave->Write(&limitSoftSelRange, sizeof(limitSoftSelRange), &nb);
	isave->EndChunk();


	isave->BeginChunk(FLATTENMAP_CHUNK);
	isave->Write(&flattenAngleThreshold, sizeof(flattenAngleThreshold), &nb);
	isave->Write(&flattenSpacing, sizeof(flattenSpacing), &nb);
	isave->Write(&flattenNormalize, sizeof(flattenNormalize), &nb);
	isave->Write(&flattenRotate, sizeof(flattenRotate), &nb);
	isave->Write(&flattenCollapse, sizeof(flattenCollapse), &nb);
	isave->EndChunk();

	isave->BeginChunk(NORMALMAP_CHUNK);
	isave->Write(&normalMethod, sizeof(normalMethod), &nb);
	isave->Write(&normalSpacing, sizeof(normalSpacing), &nb);
	isave->Write(&normalNormalize, sizeof(normalNormalize), &nb);
	isave->Write(&normalRotate, sizeof(normalRotate), &nb);
	isave->Write(&normalAlignWidth, sizeof(normalAlignWidth), &nb);
	isave->EndChunk();


	isave->BeginChunk(UNFOLDMAP_CHUNK);
	isave->Write(&unfoldMethod, sizeof(unfoldMethod), &nb);
	isave->EndChunk();


	isave->BeginChunk(STITCH_CHUNK);
	isave->Write(&bStitchAlign, sizeof(bStitchAlign), &nb);
	isave->Write(&fStitchBias, sizeof(fStitchBias), &nb);
	isave->EndChunk();


	isave->BeginChunk(GEOMELEM_CHUNK);
	isave->Write(&geomElemMode, sizeof(geomElemMode), &nb);
	isave->EndChunk();


	isave->BeginChunk(PLANARTHRESHOLD_CHUNK);
	isave->Write(&planarThreshold, sizeof(planarThreshold), &nb);
	isave->Write(&planarMode, sizeof(planarMode), &nb);
	isave->EndChunk();

	isave->BeginChunk(BACKFACECULL_CHUNK);
	isave->Write(&ignoreBackFaceCull, sizeof(ignoreBackFaceCull), &nb);
	isave->Write(&oldSelMethod, sizeof(oldSelMethod), &nb);
	isave->EndChunk();

	isave->BeginChunk(TVELEMENTMODE_CHUNK);
	isave->Write(&tvElementMode, sizeof(tvElementMode), &nb);
	isave->EndChunk();

	isave->BeginChunk(ALWAYSEDIT_CHUNK);
	isave->Write(&alwaysEdit, sizeof(alwaysEdit), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWCONNECTION_CHUNK);
	isave->Write(&showVertexClusterList, sizeof(showVertexClusterList), &nb);
	isave->EndChunk();

	isave->BeginChunk(PACK_CHUNK);
	isave->Write(&packMethod, sizeof(packMethod), &nb);
	isave->Write(&packSpacing, sizeof(packSpacing), &nb);
	isave->Write(&packNormalize, sizeof(packNormalize), &nb);
	isave->Write(&packRotate, sizeof(packRotate), &nb);
	isave->Write(&packFillHoles, sizeof(packFillHoles), &nb);
	isave->EndChunk();


	isave->BeginChunk(TVSUBOBJECTMODE_CHUNK);
	isave->Write(&mTVSubObjectMode, sizeof(mTVSubObjectMode), &nb);
	isave->EndChunk();


	isave->BeginChunk(FILLMODE_CHUNK);
	isave->Write(&fillMode, sizeof(fillMode), &nb);
	isave->EndChunk();

	isave->BeginChunk(OPENEDGECOLOR_CHUNK);
	isave->Write(&openEdgeColor, sizeof(openEdgeColor), &nb);
	isave->Write(&displayOpenEdges, sizeof(displayOpenEdges), &nb);
	isave->EndChunk();

	isave->BeginChunk(UVEDGEMODE_CHUNK);
	isave->Write(&uvEdgeMode, sizeof(uvEdgeMode), &nb);
	isave->Write(&openEdgeMode, sizeof(openEdgeMode), &nb);
	isave->Write(&displayHiddenEdges, sizeof(displayHiddenEdges), &nb);

	isave->EndChunk();

	isave->BeginChunk(MISCCOLOR_CHUNK);
	isave->Write(&handleColor, sizeof(handleColor), &nb);
	isave->Write(&freeFormColor, sizeof(freeFormColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(HITSIZE_CHUNK);
	isave->Write(&hitSize, sizeof(hitSize), &nb);
	isave->EndChunk();

	isave->BeginChunk(PIVOT_CHUNK);
	isave->Write(&resetPivotOnSel, sizeof(resetPivotOnSel), &nb);
	isave->EndChunk();

	isave->BeginChunk(GIZMOSEL_CHUNK);
	isave->Write(&allowSelectionInsideGizmo, sizeof(allowSelectionInsideGizmo), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHARED_CHUNK);
	isave->Write(&showShared, sizeof(showShared), &nb);
	isave->Write(&sharedColor, sizeof(sharedColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWICON_CHUNK);
	BitArray tmpIconList;
	tmpIconList.SetSize(32);
	tmpIconList.SetAll();
	tmpIconList.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(SYNCSELECTION_CHUNK);
	isave->Write(&syncSelection, sizeof(syncSelection), &nb);
	isave->EndChunk();

	isave->BeginChunk(BRIGHTCENTER_CHUNK);
	isave->Write(&brightCenterTile, sizeof(brightCenterTile), &nb);
	isave->Write(&blendTileToBackGround, sizeof(blendTileToBackGround), &nb);
	isave->EndChunk();

	isave->BeginChunk(FILTERMAP_CHUNK);
	isave->Write(&filterMap, sizeof(filterMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(CURSORSIZE_CHUNK);
	int sketchCursorSize100 = MaxSDK::UIUnScaled(sketchCursorSize);
	isave->Write(&sketchCursorSize100, sizeof(sketchCursorSize100), &nb);
	int paintSize100 = MaxSDK::UIUnScaled(paintSize);
	isave->Write(&paintSize100, sizeof(paintSize100), &nb);
	isave->EndChunk();

	isave->BeginChunk(TICKSIZE_CHUNK);
	int tickSize100 = MaxSDK::UIUnScaled(tickSize);
	isave->Write(&tickSize100, sizeof(tickSize100), &nb);
	isave->EndChunk();

	//new
	isave->BeginChunk(GRID_CHUNK);
	isave->Write(&gridSize, sizeof(gridSize), &nb);
	isave->Write(&snapToggle, sizeof(snapToggle), &nb);
	isave->Write(&gridVisible, sizeof(gridVisible), &nb);
	isave->Write(&gridColor, sizeof(gridColor), &nb);
	isave->Write(&snapStrength, sizeof(snapStrength), &nb);
	isave->Write(&autoMap, sizeof(autoMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(TILEGRIDVISIBLE_CHUNK);
	isave->Write(&tileGridVisible, sizeof(tileGridVisible), &nb);
	isave->EndChunk();

	isave->BeginChunk(PREVENTFLATTENING_CHUNK);
	isave->Write(&preventFlattening, sizeof(preventFlattening), &nb);
	isave->EndChunk();

	isave->BeginChunk(ENABLESOFTSELECTION_CHUNK);
	isave->Write(&enableSoftSelection, sizeof(enableSoftSelection), &nb);
	isave->EndChunk();


	isave->BeginChunk(CONSTANTUPDATE_CHUNK);
	isave->Write(&update, sizeof(update), &nb);
	isave->Write(&loadDefaults, sizeof(loadDefaults), &nb);
	isave->EndChunk();

	isave->BeginChunk(APPLYTOWHOLEOBJECT_CHUNK);
	isave->Write(&applyToWholeObject, sizeof(applyToWholeObject), &nb);
	isave->EndChunk();

	isave->BeginChunk(THICKOPENEDGE_CHUNK);
	isave->Write(&thickOpenEdges, sizeof(thickOpenEdges), &nb);
	isave->EndChunk();

	isave->BeginChunk(VIEWPORTOPENEDGE_CHUNK);
	isave->Write(&viewportOpenEdges, sizeof(viewportOpenEdges), &nb);
	isave->EndChunk();

	isave->BeginChunk(ABSOLUTETYPEIN_CHUNK);
	isave->Write(&absoluteTypeIn, sizeof(absoluteTypeIn), &nb);
	isave->EndChunk();

	isave->BeginChunk(STITCHSCALE_CHUNK);
	isave->Write(&bStitchScale, sizeof(bStitchScale), &nb);
	isave->EndChunk();

	isave->BeginChunk(VERSION_CHUNK);
	isave->Write(&version, sizeof(version), &nb);
	isave->EndChunk();

	isave->BeginChunk(CURRENTMAP_CHUNK);
	isave->Write(&CurrentMap, sizeof(CurrentMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(RELAX_CHUNK);
	isave->Write(&relaxAmount, sizeof(relaxAmount), &nb);
	isave->Write(&relaxStretch, sizeof(relaxStretch), &nb);
	isave->Write(&relaxIteration, sizeof(relaxIteration), &nb);
	isave->Write(&relaxType, sizeof(relaxType), &nb);
	isave->Write(&relaxBoundary, sizeof(relaxBoundary), &nb);
	isave->Write(&relaxSaddle, sizeof(relaxSaddle), &nb);
	isave->EndChunk();

	isave->BeginChunk(FALLOFFSPACE_CHUNK);
	isave->Write(&falloffSpace, sizeof(falloffSpace), &nb);
	isave->Write(&falloffStr, sizeof(falloffStr), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWPELTSEAMS_CHUNK);
	isave->Write(&alwaysShowSeams, sizeof(alwaysShowSeams), &nb);
	isave->EndChunk();

	isave->BeginChunk(SPLINEMAP_V2_CHUNK);
	mSplineMap.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(CONTROLLER_COUNT_CHUNK);
	int controllerCount = mUVWControls.cont.Count();
	isave->Write(&controllerCount, sizeof(controllerCount), &nb);
	isave->EndChunk();


	isave->BeginChunk(PACKTEMP_CHUNK);
	isave->Write(&mPackTempPadding, sizeof(mPackTempPadding), &nb);
	isave->Write(&mPackTempRescale, sizeof(mPackTempRescale), &nb);
	isave->Write(&mPackTempRotate, sizeof(mPackTempRotate), &nb);
	isave->EndChunk();

	isave->BeginChunk(PEELCOLOR_CHUNK);
	isave->Write(&peelColor, sizeof(peelColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(MIRROR_GEOM_SEL_AXIS);
	isave->WriteVoid(&mirrorGeoSelectionAxis, sizeof(mirrorGeoSelectionAxis), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWMULTITILE_CHUNK);
	isave->Write(&showMultiTile, sizeof(showMultiTile), &nb);
	isave->EndChunk();

	isave->BeginChunk(CHECKERTILING_CHUNK);
	isave->Write(&fCheckerTiling, sizeof(fCheckerTiling), &nb);
	isave->EndChunk();

	isave->BeginChunk(DISPLAYPIXELUNITS_CHUNK);
	isave->Write(&displayPixelUnits, sizeof(displayPixelUnits), &nb);
	isave->EndChunk();

	return IO_OK;
}

#define UVWVER	4

void UnwrapMod::LoadUVW(HWND hWnd)
{

	theHold.Begin();
	HoldPointsAndFaces();

	static TCHAR fname[256] = { '\0' };
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	FilterList fl;
	fl.Append(GetString(IDS_PW_UVWFILES));
	fl.Append(_T("*.uvw"));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		TSTR title;
		title.printf(_T("%s : %s"), mMeshTopoData.GetNode(ldID)->GetName(), GetString(IDS_PW_LOADOBJECT));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetCOREInterface()->GetMAXHWnd();
		ofn.lpstrFilter = fl;
		ofn.lpstrFile = fname;
		ofn.nMaxFile = 256;
		//ofn.lpstrInitialDir = ip->GetDir(APP_EXPORT_DIR);
		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
		ofn.lpstrDefExt = _T("uvw");
		ofn.lpstrTitle = title;


		if (GetOpenFileName(&ofn))
		{

			//load stuff here  stuff here
			FILE *file = _tfopen(fname, _T("rb"));

			for (int i = 0; i < mUVWControls.cont.Count(); i++)
				if (mUVWControls.cont[i]) DeleteReference(i + 11 + 100);


			int ver = ld->LoadTVVerts(file);
			ld->LoadFaces(file, ver);
			fclose(file);
		}
	}
	theHold.Accept(GetString(IDS_PW_LOADOBJECT));

}
void UnwrapMod::SaveUVW(HWND hWnd)
{
	static TCHAR fname[256] = { '\0' };
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	FilterList fl;
	fl.Append(GetString(IDS_PW_UVWFILES));
	fl.Append(_T("*.uvw"));
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		TSTR title;
		title.printf(_T("%s : %s"), mMeshTopoData.GetNode(ldID)->GetName(), GetString(IDS_PW_SAVEOBJECT));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetCOREInterface()->GetMAXHWnd();
		ofn.lpstrFilter = fl;
		ofn.lpstrFile = fname;
		ofn.nMaxFile = 256;
		//ofn.lpstrInitialDir = ip->GetDir(APP_EXPORT_DIR);
		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
		ofn.lpstrDefExt = _T("uvw");
		ofn.lpstrTitle = title;

	tryAgain:
		if (GetSaveFileName(&ofn)) {
			if (DoesFileExist(fname)) {
				TSTR buf1;
				TSTR buf2 = GetString(IDS_PW_SAVEOBJECT);
				buf1.printf(GetString(IDS_PW_FILEEXISTS), fname);
				if (IDYES != MessageBox(
					hParams,
					buf1, buf2, MB_YESNO | MB_ICONQUESTION)) {
					goto tryAgain;
				}
			}
			//save stuff here
			// this is timed slice so it will not save animation not sure how to save controller info but will neeed to later on in other plugs

			// for a360 support - allows binary diff syncing
			MaxSDK::Util::Path storageNamePath(fname);
			storageNamePath.SaveBaseFile();

			FILE *file = _tfopen(fname, _T("wb"));

			int ver = -1;
			fwrite(&ver, sizeof(ver), 1, file);
			ver = UVWVER;
			fwrite(&ver, sizeof(ver), 1, file);


			int vct = ld->GetNumberTVVerts();//TVMaps.v.Count(), 
			int fct = ld->GetNumberFaces();//TVMaps.f.Count();

			fwrite(&vct, sizeof(vct), 1, file);

			if (vct)
			{
				ld->SaveTVVerts(file);
			}

			fwrite(&fct, sizeof(fct), 1, file);

			if (fct)
			{
				ld->SaveFaces(file);//				TVMaps.SaveFaces(file);
			}

			fclose(file);
		}
	}

}


IOResult UnwrapMod::LoadNamedSelChunk(ILoad *iload) {
	IOResult res;
	DWORD ix = 0;
	ULONG nb;

	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case NAMEDSEL_STRING_CHUNK:
		{
			TCHAR *name;
			res = iload->ReadWStringChunk(&name);
			//AddSet(TSTR(name),level+1);
			TSTR *newName = new TSTR(name);
			namedSel.Append(1, &newName);
			ids.Append(1, &ix);
			ix++;
			break;
		}
		case NAMEDSEL_ID_CHUNK:
			iload->Read(&ids[ids.Count() - 1], sizeof(DWORD), &nb);
			break;

		case NAMEDVSEL_STRING_CHUNK:
		{
			TCHAR *name;
			res = iload->ReadWStringChunk(&name);
			//AddSet(TSTR(name),level+1);
			TSTR *newName = new TSTR(name);
			namedVSel.Append(1, &newName);
			idsV.Append(1, &ix);
			ix++;
			break;
		}
		case NAMEDVSEL_ID_CHUNK:
			iload->Read(&idsV[ids.Count() - 1], sizeof(DWORD), &nb);
			break;

		case NAMEDESEL_STRING_CHUNK:
		{
			TCHAR *name;
			res = iload->ReadWStringChunk(&name);
			//AddSet(TSTR(name),level+1);
			TSTR *newName = new TSTR(name);
			namedESel.Append(1, &newName);
			idsE.Append(1, &ix);
			ix++;
			break;
		}
		case NAMEDESEL_ID_CHUNK:
			iload->Read(&idsE[ids.Count() - 1], sizeof(DWORD), &nb);
			break;

		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	return IO_OK;
}


IOResult UnwrapMod::Load(ILoad *iload)
{
	popUpDialog = FALSE;
	version = 2;
	IOResult res = IO_OK;
	ULONG nb = 0;
	Modifier::Load(iload);
	//check for backwards compatibility
	useBitmapRes = FALSE;

	Tab<UVW_TVFaceOldClass> oldData;

	fnSetTile(FALSE);
	
	//5.1.05
	this->autoBackground = FALSE;


	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case CURRENTMAP_CHUNK:
			iload->Read(&CurrentMap, sizeof(CurrentMap), &nb);
			break;

		case VERSION_CHUNK:
			iload->Read(&version, sizeof(version), &nb);
			break;
			//5.1.05
		case AUTOBACKGROUND_CHUNK:
			this->autoBackground = TRUE;
			break;

		case 0x2806:
			res = LoadNamedSelChunk(iload);
			break;

		case ZOOM_CHUNK:
			iload->Read(&zoom, sizeof(zoom), &nb);
			break;
		case ASPECT_CHUNK:
			iload->Read(&aspect, sizeof(aspect), &nb);
			break;
		case LOCKASPECT_CHUNK:
			iload->Read(&lockAspect, sizeof(lockAspect), &nb);
			break;
		case MAPSCALE_CHUNK:
			iload->Read(&mapScale, sizeof(mapScale), &nb);
			break;

		case XSCROLL_CHUNK:
			iload->Read(&xscroll, sizeof(xscroll), &nb);
			break;
		case YSCROLL_CHUNK:
			iload->Read(&yscroll, sizeof(yscroll), &nb);
			break;
		case IWIDTH_CHUNK:
			iload->Read(&rendW, sizeof(rendW), &nb);
			break;
		case IHEIGHT_CHUNK:
			iload->Read(&rendH, sizeof(rendH), &nb);
			break;
		case SHOWMAP_CHUNK:
			iload->Read(&showMap, sizeof(showMap), &nb);
			break;
		case UPDATE_CHUNK:
			iload->Read(&update, sizeof(update), &nb);
			break;
		case LINECOLOR_CHUNK:
			iload->Read(&lineColor, sizeof(lineColor), &nb);
			break;
		case SELCOLOR_CHUNK:
			iload->Read(&selColor, sizeof(selColor), &nb);
			break;
		case UVW_CHUNK:
			iload->Read(&uvw, sizeof(uvw), &nb);
			break;
		case CHANNEL_CHUNK:
			iload->Read(&channel, sizeof(channel), &nb);
			break;
		case WINDOWPOS_CHUNK:
			iload->Read(&windowPos, sizeof(WINDOWPLACEMENT), &nb);

			windowPos.flags = 0;
			windowPos.showCmd = SW_SHOWNORMAL;
			break;
		case PREFS_CHUNK:
			iload->Read(&lineColor, sizeof(lineColor), &nb);
			iload->Read(&selColor, sizeof(selColor), &nb);
			iload->Read(&weldThreshold, sizeof(weldThreshold), &nb);
			iload->Read(&update, sizeof(update), &nb);
			iload->Read(&showVerts, sizeof(showVerts), &nb);
			iload->Read(&pixelCenterSnap, sizeof(pixelCenterSnap), &nb);
			break;
		case USEBITMAPRES_CHUNK:
			useBitmapRes = TRUE;
			break;
		case DISPLAYPIXELUNITS_CHUNK:
			iload->Read(&displayPixelUnits, sizeof(displayPixelUnits), &nb);
			break;

		case FORCEUPDATE_CHUNK:
			break;
			//tile stuff
		case TILE_CHUNK:
			fnSetTile(TRUE);
			break;
		case TILECONTRAST_CHUNK:
			iload->Read(&fContrast, sizeof(fContrast), &nb);
			break;
		case TILELIMIT_CHUNK:
			iload->Read(&iTileLimit, sizeof(iTileLimit), &nb);
			break;
		case SOFTSELLIMIT_CHUNK:
			iload->Read(&limitSoftSel, sizeof(limitSoftSel), &nb);
			iload->Read(&limitSoftSelRange, sizeof(limitSoftSelRange), &nb);
			break;
		case FLATTENMAP_CHUNK:
			iload->Read(&flattenAngleThreshold, sizeof(flattenAngleThreshold), &nb);
			iload->Read(&flattenSpacing, sizeof(flattenSpacing), &nb);
			iload->Read(&flattenNormalize, sizeof(flattenNormalize), &nb);
			iload->Read(&flattenRotate, sizeof(flattenRotate), &nb);
			iload->Read(&flattenCollapse, sizeof(flattenCollapse), &nb);
			break;
		case NORMALMAP_CHUNK:
			iload->Read(&normalMethod, sizeof(normalMethod), &nb);
			iload->Read(&normalSpacing, sizeof(normalSpacing), &nb);
			iload->Read(&normalNormalize, sizeof(normalNormalize), &nb);
			iload->Read(&normalRotate, sizeof(normalRotate), &nb);
			iload->Read(&normalAlignWidth, sizeof(normalAlignWidth), &nb);
			break;

		case UNFOLDMAP_CHUNK:
			iload->Read(&unfoldMethod, sizeof(unfoldMethod), &nb);
			break;

		case STITCH_CHUNK:
			iload->Read(&bStitchAlign, sizeof(bStitchAlign), &nb);
			iload->Read(&fStitchBias, sizeof(fStitchBias), &nb);
			break;

		case STITCHSCALE_CHUNK:
			iload->Read(&bStitchScale, sizeof(bStitchScale), &nb);
			break;

		case GEOMELEM_CHUNK:
			iload->Read(&geomElemMode, sizeof(geomElemMode), &nb);
			break;
		case PLANARTHRESHOLD_CHUNK:
			iload->Read(&planarThreshold, sizeof(planarThreshold), &nb);
			iload->Read(&planarMode, sizeof(planarMode), &nb);
			break;
		case BACKFACECULL_CHUNK:
			iload->Read(&ignoreBackFaceCull, sizeof(ignoreBackFaceCull), &nb);
			iload->Read(&oldSelMethod, sizeof(oldSelMethod), &nb);
			break;
		case TVELEMENTMODE_CHUNK:
			iload->Read(&tvElementMode, sizeof(tvElementMode), &nb);
			break;
		case ALWAYSEDIT_CHUNK:
			iload->Read(&alwaysEdit, sizeof(alwaysEdit), &nb);
			break;

		case SHOWCONNECTION_CHUNK:
			iload->Read(&showVertexClusterList, sizeof(showVertexClusterList), &nb);
			break;
		case PACK_CHUNK:
			iload->Read(&packMethod, sizeof(packMethod), &nb);
			iload->Read(&packSpacing, sizeof(packSpacing), &nb);
			iload->Read(&packNormalize, sizeof(packNormalize), &nb);
			iload->Read(&packRotate, sizeof(packRotate), &nb);
			iload->Read(&packFillHoles, sizeof(packFillHoles), &nb);
			break;
		case TVSUBOBJECTMODE_CHUNK:
			iload->Read(&mTVSubObjectMode, sizeof(mTVSubObjectMode), &nb);
			if (mTVSubObjectMode < 0)
				mTVSubObjectMode = TVFACEMODE;
			break;
		case FILLMODE_CHUNK:
			iload->Read(&fillMode, sizeof(fillMode), &nb);
			break;

		case OPENEDGECOLOR_CHUNK:
			iload->Read(&openEdgeColor, sizeof(openEdgeColor), &nb);
			iload->Read(&displayOpenEdges, sizeof(displayOpenEdges), &nb);
			break;
		case THICKOPENEDGE_CHUNK:
			iload->Read(&thickOpenEdges, sizeof(thickOpenEdges), &nb);
			break;
		case VIEWPORTOPENEDGE_CHUNK:
			iload->Read(&viewportOpenEdges, sizeof(viewportOpenEdges), &nb);
			break;
		case UVEDGEMODE_CHUNK:
			iload->Read(&uvEdgeMode, sizeof(uvEdgeMode), &nb);
			iload->Read(&openEdgeMode, sizeof(openEdgeMode), &nb);
			iload->Read(&displayHiddenEdges, sizeof(displayHiddenEdges), &nb);
			break;
		case MISCCOLOR_CHUNK:
			iload->Read(&handleColor, sizeof(handleColor), &nb);
			iload->Read(&freeFormColor, sizeof(freeFormColor), &nb);
			break;

		case HITSIZE_CHUNK:
			iload->Read(&hitSize, sizeof(hitSize), &nb);
			break;
		case PIVOT_CHUNK:
			iload->Read(&resetPivotOnSel, sizeof(resetPivotOnSel), &nb);
			break;
		case GIZMOSEL_CHUNK:
			iload->Read(&allowSelectionInsideGizmo, sizeof(allowSelectionInsideGizmo), &nb);
			break;
		case SHARED_CHUNK:
			iload->Read(&showShared, sizeof(showShared), &nb);
			iload->Read(&sharedColor, sizeof(sharedColor), &nb);
			break;
		case SHOWICON_CHUNK:
			{
				BitArray tmpBitArray; //load the data but no longer used in Kirin and afterwards
				tmpBitArray.SetSize(32);
				tmpBitArray.Load(iload);
			}
			break;
		case SYNCSELECTION_CHUNK:
			iload->Read(&syncSelection, sizeof(syncSelection), &nb);
			break;
		case BRIGHTCENTER_CHUNK:
			iload->Read(&brightCenterTile, sizeof(brightCenterTile), &nb);
			iload->Read(&blendTileToBackGround, sizeof(blendTileToBackGround), &nb);
			break;
		case FILTERMAP_CHUNK:
			iload->Read(&filterMap, sizeof(filterMap), &nb);
			break;

		case CURSORSIZE_CHUNK:
		{
			int sketchCursorSize100 = 8;
			iload->Read(&sketchCursorSize100, sizeof(sketchCursorSize100), &nb);
			sketchCursorSize = MaxSDK::UIScaled(sketchCursorSize100);
			int paintSize100 = 8;
			iload->Read(&paintSize100, sizeof(paintSize100), &nb);
			paintSize = MaxSDK::UIScaled(paintSize100);
		}
		break;


		case TICKSIZE_CHUNK:
		{
			int tickSize100 = 2;
			iload->Read(&tickSize100, sizeof(tickSize100), &nb);
			tickSize = MaxSDK::UIScaled(tickSize100);
		}

			break;
			//new
		case GRID_CHUNK:
			iload->Read(&gridSize, sizeof(gridSize), &nb);
			iload->Read(&snapToggle, sizeof(snapToggle), &nb);
			iload->Read(&gridVisible, sizeof(gridVisible), &nb);
			iload->Read(&gridColor, sizeof(gridColor), &nb);
			iload->Read(&snapStrength, sizeof(snapStrength), &nb);
			iload->Read(&autoMap, sizeof(autoMap), &nb);
			break;

		case TILEGRIDVISIBLE_CHUNK:
			iload->Read(&tileGridVisible, sizeof(tileGridVisible), &nb);
			break;

		case PREVENTFLATTENING_CHUNK:
			iload->Read(&preventFlattening, sizeof(preventFlattening), &nb);
			break;

		case ENABLESOFTSELECTION_CHUNK:
			iload->Read(&enableSoftSelection, sizeof(enableSoftSelection), &nb);
			break;

		case CONSTANTUPDATE_CHUNK:
			iload->Read(&update, sizeof(update), &nb);
			iload->Read(&loadDefaults, sizeof(loadDefaults), &nb);
			break;
		case APPLYTOWHOLEOBJECT_CHUNK:
			iload->Read(&applyToWholeObject, sizeof(applyToWholeObject), &nb);
			break;

		case ABSOLUTETYPEIN_CHUNK:
			iload->Read(&absoluteTypeIn, sizeof(absoluteTypeIn), &nb);
			break;

		case RELAX_CHUNK:
			iload->Read(&relaxAmount, sizeof(relaxAmount), &nb);
			iload->Read(&relaxStretch, sizeof(relaxStretch), &nb);
			iload->Read(&relaxIteration, sizeof(relaxIteration), &nb);
			iload->Read(&relaxType, sizeof(relaxType), &nb);
			iload->Read(&relaxBoundary, sizeof(relaxBoundary), &nb);
			iload->Read(&relaxSaddle, sizeof(relaxSaddle), &nb);
			break;
		case FALLOFFSPACE_CHUNK:
			iload->Read(&falloffSpace, sizeof(falloffSpace), &nb);
			iload->Read(&falloffStr, sizeof(falloffStr), &nb);
			break;

		case SHOWPELTSEAMS_CHUNK:
			iload->Read(&alwaysShowSeams, sizeof(alwaysShowSeams), &nb);
			break;
		case SPLINEMAP_CHUNK:
			mSplineMap.Load(iload, 1);
			break;
		case SPLINEMAP_V2_CHUNK:
			mSplineMap.Load(iload, 2);
			break;
		case CONTROLLER_COUNT_CHUNK:
		{
			int controllerCount = 0;
			iload->Read(&controllerCount, sizeof(controllerCount), &nb);
			mUVWControls.cont.SetCount(controllerCount);
			for (int i = 0; i < mUVWControls.cont.Count(); i++)
				mUVWControls.cont[i] = NULL;
		}
		break;
		case PACKTEMP_CHUNK:
			iload->Read(&mPackTempPadding, sizeof(mPackTempPadding), &nb);
			iload->Read(&mPackTempRescale, sizeof(mPackTempRescale), &nb);
			iload->Read(&mPackTempRotate, sizeof(mPackTempRotate), &nb);
			break;
		case PEELCOLOR_CHUNK:
			iload->Read(&peelColor, sizeof(peelColor), &nb);
			break;
		case MIRROR_GEOM_SEL_AXIS:
		{
			int axis = 0;
			if (IO_OK == iload->Read(&axis, sizeof(axis), &nb))
			{
				fnSetMirrorAxis(axis);
			}
		}
		break;
		case SHOWMULTITILE_CHUNK:
			iload->Read(&showMultiTile, sizeof(showMultiTile), &nb);
			break;
		case CHECKERTILING_CHUNK:
			iload->Read(&fCheckerTiling, sizeof(fCheckerTiling), &nb);
			break;
		default:
			mTVMaps_Max9.LoadOlderVersions(iload);
			//load the older versions here now
			mUVWControls.cont.SetCount(mTVMaps_Max9.v.Count());

			for (int i = 0; i < mUVWControls.cont.Count(); i++)
				mUVWControls.cont[i] = NULL;
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK)
			return res;
	}

	return IO_OK;
}

#define FACESEL_CHUNKID			0x0210
#define FSELSET_CHUNK			0x2846


#define ESELSET_CHUNK			0x2847


#define VSELSET_CHUNK			0x2848

IOResult UnwrapMod::SaveLocalData(ISave *isave, LocalModData *ld) {
	MeshTopoData *d = (MeshTopoData*)ld;

	if (d == NULL)
		return IO_OK;

	/*
	isave->BeginChunk(FACESEL_CHUNKID);
	BitArray fsel = d->GetFaceSel();
	fsel.Save(isave);
	isave->EndChunk();
	*/

	if (d->fselSet.Count()) {
		isave->BeginChunk(FSELSET_CHUNK);
		d->fselSet.Save(isave);
		isave->EndChunk();
	}



	if (d->vselSet.Count()) {
		isave->BeginChunk(VSELSET_CHUNK);
		d->vselSet.Save(isave);
		isave->EndChunk();
	}

	if (d->eselSet.Count()) {
		isave->BeginChunk(ESELSET_CHUNK);
		d->eselSet.Save(isave);
		isave->EndChunk();
	}


	//	int vct = d->GetNumberTVVerts(),//TVMaps.v.Count(), 
	//		fct = d->GetNumberFaces();//TVMaps.f.Count();


	//	isave->BeginChunk(VERTCOUNT_CHUNK);
	//	isave->Write(&vct, sizeof(vct), &nb);
	//	isave->EndChunk();

	if (d->GetNumberTVVerts())
	{
		isave->BeginChunk(VERTS3_CHUNK);
		d->SaveTVVerts(isave);
		//		isave->Write(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*vct, &nb);
		isave->EndChunk();
	}


	if (d->GetNumberFaces()) {
		isave->BeginChunk(FACE5_CHUNK);
		d->SaveFaces(isave);
		isave->EndChunk();
	}



	if (d->GetNumberGeomVerts()) {
		isave->BeginChunk(GEOMPOINTS_CHUNK);
		d->SaveGeoVerts(isave);
		isave->EndChunk();
	}

	isave->BeginChunk(VERTSEL_CHUNK);
	d->SaveTVVertSelection(isave);
	isave->EndChunk();

	isave->BeginChunk(GVERTSEL_CHUNK);
	d->SaveGeoVertSelection(isave);
	isave->EndChunk();



	isave->BeginChunk(UEDGESELECTION_CHUNK);
	d->SaveTVEdgeSelection(isave);
	isave->EndChunk();

	isave->BeginChunk(GEDGESELECTION_CHUNK);
	d->SaveGeoEdgeSelection(isave);
	isave->EndChunk();

	isave->BeginChunk(FACESELECTION_CHUNK);
	d->SaveFaceSelection(isave);
	isave->EndChunk();

	isave->BeginChunk(SEAM_CHUNK);
	d->mSeamEdges.Save(isave);
	isave->EndChunk();


	isave->BeginChunk(GROUPING_CHUNK);
	d->GetToolGroupingData()->Save(isave);
	isave->EndChunk();




	return IO_OK;
}

IOResult UnwrapMod::LoadLocalData(ILoad *iload, LocalModData **pld) {
	MeshTopoData *d = new MeshTopoData();
	bMeshTopoDataContainerDirty = TRUE;
	*pld = d;
	IOResult res;
	//need to invalidate the TVs edges since they are not saved and need to be regenerated
	d->SetTVEdgeInvalid();
	d->SetGeoEdgeInvalid();
	d->SetLoaded();
	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case FSELSET_CHUNK:
			res = d->fselSet.Load(iload);
			break;
		case VSELSET_CHUNK:
			res = d->vselSet.Load(iload);
			break;
		case ESELSET_CHUNK:
			res = d->eselSet.Load(iload);
			break;
		case FACE5_CHUNK:
			d->LoadFaces(iload);
			break;

		case VERTS3_CHUNK:
			d->LoadTVVerts(iload);
			break;
		case GEOMPOINTS_CHUNK:
			d->LoadGeoVerts(iload);
			break;
		case VERTSEL_CHUNK:
		{
			d->LoadTVVertSelection(iload);
			break;
		}
		case GVERTSEL_CHUNK:
		{
			d->LoadGeoVertSelection(iload);
			break;
		}
		case GEDGESELECTION_CHUNK:
		{
			d->LoadGeoEdgeSelection(iload);
			break;
		}
		case UEDGESELECTION_CHUNK:
		{

			d->LoadTVEdgeSelection(iload);
			break;
		}
		case FACESELECTION_CHUNK:
		{
			d->LoadFaceSelection(iload);
			break;
		}

		case SEAM_CHUNK:
		{
			d->mSeamEdges.Load(iload);
			break;
		}
		case GROUPING_CHUNK:
		{
			d->GetToolGroupingData()->Load(iload);
			break;
		}

		}

		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	d->FixUpLockedFlags();
	return IO_OK;
}


/*
void UnwrapMod::SynchWithMesh(Mesh &mesh)
{
int ct=0;
if (mesh.selLevel==MESH_FACE) {
for (int i=0; i<mesh.getNumFaces(); i++) {
if (mesh.faceSel[i]) ct++;
}
} else {
ct = mesh.getNumFaces();
}
if (ct != tvFace.Count()) {
DeleteAllRefsFromMe();
tvert.Resize(0);
cont.Resize(0);
tvFace.SetCount(ct);

TVFace *tvFaceM = mesh.tvFace;
Point3 *tVertsM = mesh.tVerts;
int numTV = channel ? mesh.getNumVertCol() : mesh.getNumTVerts();
if (channel) {
tvFaceM = mesh.vcFace;
tVertsM = mesh.vertCol;
}

if (mesh.selLevel==MESH_FACE) {
// Mark tverts that are used by selected faces
BitArray used;
if (tvFaceM) used.SetSize(numTV);
else used.SetSize(mesh.getNumVerts());
for (int i=0; i<mesh.getNumFaces(); i++) {
if (mesh.faceSel[i]) {
if (tvFaceM) {
for (int j=0; j<3; j++)
used.Set(tvFaceM[i].t[j]);
} else {
for (int j=0; j<3; j++)
used.Set(mesh.faces[i].v[j]);
}
}
}

// Now build a vmap
Tab<DWORD> vmap;
vmap.SetCount(used.GetSize());
int ix=0;
for (int i=0; i<used.GetSize(); i++) {
if (used[i]) vmap[i] = ix++;
else vmap[i] = UNDEFINED;
}

// Copy in tverts
tvert.SetCount(ix);
cont.SetCount(ix);
vsel.SetSize(ix);
ix = 0;
Box3 box = mesh.getBoundingBox();
for (int i=0; i<used.GetSize(); i++) {
if (used[i]) {
cont[ix] = NULL;
if (tvFaceM) tvert[ix++] = tVertsM[i];
else {
// Do a planar mapping if there are no tverts
tvert[ix].x = mesh.verts[i].x/box.Width().x + 0.5f;
tvert[ix].y = mesh.verts[i].y/box.Width().y + 0.5f;
tvert[ix].z = mesh.verts[i].z/box.Width().z + 0.5f;
ix++;
}
}
}

// Copy in face and remap indices
ix = 0;
for (int i=0; i<mesh.getNumFaces(); i++) {
if (mesh.faceSel[i]) {
if (tvFaceM) tvFace[ix] = tvFaceM[i];
else {
for (int j=0; j<3; j++)
tvFace[ix].t[j] = mesh.faces[i].v[j];
}

for (int j=0; j<3; j++) {
tvFace[ix].t[j] = vmap[tvFace[ix].t[j]];
}
ix++;
}
}
} else {
// Just copy all the tverts and faces
if (tvFaceM) {
tvert.SetCount(numTV);
cont.SetCount(numTV);
vsel.SetSize(numTV);
for (int i=0; i<numTV; i++) {
tvert[i] = tVertsM[i];
cont[i]  = NULL;
}
for (int i=0; i<mesh.getNumFaces(); i++) {
tvFace[i] = tvFaceM[i];
}
} else {
Box3 box = mesh.getBoundingBox();
tvert.SetCount(mesh.getNumVerts());
cont.SetCount(mesh.getNumVerts());
vsel.SetSize(mesh.getNumVerts());
for (int i=0; i<mesh.getNumVerts(); i++) {
// Do a planar mapping if there are no tverts
tvert[i].x = mesh.verts[i].x/box.Width().x + 0.5f;
tvert[i].y = mesh.verts[i].y/box.Width().y + 0.5f;
tvert[i].z = mesh.verts[i].z/box.Width().z + 0.5f;
cont[i]  = NULL;
}
for (int i=0; i<mesh.getNumFaces(); i++) {
for (int j=0; j<3; j++)
tvFace[i].t[j] = mesh.faces[i].v[j];
}
}
}
if (hView && editMod==this) {
InvalidateView();
}
}
}
*/
void UnwrapMod::GetUVWIndices(int &i1, int &i2)
{
	switch (uvw) {
	case 0: i1 = 0; i2 = 1; break;
	case 1: i1 = 1; i2 = 2; break;
	case 2: i1 = 0; i2 = 2; break;
	}
}


//--- Floater Dialog -------------------------------------------------
#define WM_SETUPMOD	WM_USER+0x18de

static HIMAGELIST hToolImages = NULL;
static HIMAGELIST hOptionImages = NULL;
static HIMAGELIST hViewImages = NULL;
static HIMAGELIST hVertexImages = NULL;

class DeleteResources {
public:
	~DeleteResources() {
		if (hToolImages) ImageList_Destroy(hToolImages);
		if (hOptionImages) ImageList_Destroy(hOptionImages);
		if (hViewImages) ImageList_Destroy(hViewImages);
		if (hVertexImages) ImageList_Destroy(hVertexImages);
	}
};
static DeleteResources	theDelete;





INT_PTR CALLBACK UnwrapFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	// process mouse event,etc.
	BOOL processed = mod->GetUIManager()->MessageProc(hWnd, msg, wParam, lParam);

	if (processed == FALSE)
	{
		switch (msg) {
		case WM_INITDIALOG:

			mod = (UnwrapMod*)lParam;
			DLSetWindowLongPtr(hWnd, lParam);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, GetClassLongPtr(GetCOREInterface()->GetMAXHWnd(), GCLP_HICONSM)); // mjm - 3.12.99
			::SetWindowContextHelpId(hWnd, idh_unwrap_edit);
			GetCOREInterface()->RegisterDlgWnd(hWnd);
			mod->SetupDlg(hWnd);

			delEvent.SetEditMeshMod(mod);
			GetCOREInterface()->RegisterDeleteUser(&delEvent);

			mod->UpdateMapListBox();
			SendMessage(mod->hMatIDs, CB_SETCURSEL, mod->matid + 1, 0);
			
			mod->GetUIManager()->SetSpinFValue(ID_SOFTSELECTIONSTR_SPINNER, mod->falloffStr);
			mod->GetUIManager()->SetFlyOut(ID_FALLOFF, mod->falloff, FALSE);
			mod->GetUIManager()->SetFlyOut(ID_FALLOFF_SPACE, mod->falloffSpace, FALSE);
			mod->GetUIManager()->SetFlyOut(ID_MIRROR, mod->mirror, FALSE);
			mod->GetUIManager()->SetFlyOut(ID_UVW, mod->uvw, FALSE);

			mod->GetUIManager()->SetFlyOut(ID_HIDE, mod->hide, FALSE);
			mod->GetUIManager()->SetFlyOut(ID_FREEZE, mod->freeze, FALSE);
			mod->GetUIManager()->SetSpinIValue(ID_SOFTSELECTIONLIMIT_SPINNER, mod->fnGetLimitSoftSelRange());

			mod->GetUIManager()->SetSpinIValue(ID_BRUSH_FALLOFFSPINNER, mod->fnGetPaintFallOffSize());
			mod->GetUIManager()->SetSpinIValue(ID_BRUSH_STRENGTHSPINNER, mod->fnGetPaintFullStrengthSize());
			mod->GetUIManager()->SetFlyOut(ID_BRUSH_FALLOFF_TYPE, mod->fnGetPaintFallOffType(), FALSE);
			mod->GetUIManager()->SetFlyOut(ID_RELAX_MOVE_BRUSH, mod->fnGetRelaxType(), FALSE);

			if (mod->windowPos.length != 0) {
				mod->windowPos.flags = 0;
				mod->windowPos.showCmd = SW_SHOWNORMAL;
				SetWindowPlacement(hWnd, &mod->windowPos);
			}

			mod->GetUIManager()->UpdateCheckButtons();
			break;
		case WM_SHOWWINDOW:
		{
			if (!wParam)
			{
				if (mod->peltData.peltDialog.hWnd)
					ShowWindow(mod->peltData.peltDialog.hWnd, SW_HIDE);
				if (mod->hOptionshWnd)
					ShowWindow(mod->hOptionshWnd, SW_HIDE);
				if (mod->hRelaxDialog)
					ShowWindow(mod->hRelaxDialog, SW_HIDE);
				if (mod->renderUVWindow)
					ShowWindow(mod->renderUVWindow, SW_HIDE);

				mod->GetUIManager()->DisplayFloaters(FALSE);

			}
			else
			{
				if (mod->peltData.peltDialog.hWnd)
					ShowWindow(mod->peltData.peltDialog.hWnd, SW_SHOW);
				if (mod->hOptionshWnd)
					ShowWindow(mod->hOptionshWnd, SW_SHOW);
				if (mod->hRelaxDialog)
					ShowWindow(mod->hRelaxDialog, SW_SHOW);
				if (mod->renderUVWindow)
					ShowWindow(mod->renderUVWindow, SW_SHOW);

				mod->GetUIManager()->DisplayFloaters(TRUE);

			}
			return FALSE;
			break;
		}

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_CONTEXTHELP)
			{
				MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_edit);
			}
			return FALSE;
			break;

		case WM_SIZE:
			if (mod)
			{
				mod->minimized = FALSE;
				if (wParam == SIZE_MAXIMIZED)
				{
					WINDOWPLACEMENT floaterPos;
					floaterPos.length = sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(hWnd, &floaterPos);

					mod->bringUpPanel = TRUE;
				}
				mod->SizeDlg();

				if (wParam == SIZE_MINIMIZED)
				{
					if (mod->peltData.peltDialog.hWnd)
						ShowWindow(mod->peltData.peltDialog.hWnd, SW_HIDE);
					if (mod->hOptionshWnd)
						ShowWindow(mod->hOptionshWnd, SW_HIDE);
					if (mod->hRelaxDialog)
						ShowWindow(mod->hRelaxDialog, SW_HIDE);
					if (mod->renderUVWindow)
						ShowWindow(mod->renderUVWindow, SW_HIDE);

					mod->GetUIManager()->DisplayFloaters(FALSE);
					mod->minimized = TRUE;
				}
				else
				{
					if (mod->peltData.peltDialog.hWnd)
						ShowWindow(mod->peltData.peltDialog.hWnd, SW_SHOW);
					if (mod->hOptionshWnd)
						ShowWindow(mod->hOptionshWnd, SW_SHOW);
					if (mod->hRelaxDialog)
						ShowWindow(mod->hRelaxDialog, SW_SHOW);
					if (mod->renderUVWindow)
						ShowWindow(mod->renderUVWindow, SW_SHOW);

					mod->GetUIManager()->DisplayFloaters(TRUE);
					mod->bringUpPanel = TRUE;
				}
			}
			break;
		case WM_MOVE:
			if (mod)
			{
				mod->UpdateToolBars();
			}
			break;

		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				HWND newActiveWindow = (HWND)lParam;

				//see it is one of our child window getting activated, if so we still
				//want to be active to process action items
				bool done = false;
				while (!done)
				{
					HWND parent = GetParent(newActiveWindow);
					if (parent == NULL)
					{
						mod->floaterWindowActive = FALSE;
						done = TRUE;
					}
					else if (parent == mod->hDialogWnd)
					{
						mod->floaterWindowActive = TRUE;
						done = TRUE;
					}
					else if (parent == GetCOREInterface()->GetMAXHWnd())
					{
						mod->floaterWindowActive = FALSE;
						done = TRUE;
					}
					newActiveWindow = parent;
				}

			}
			else
			{
				if ((LOWORD(wParam) == WA_CLICKACTIVE) && (!mod->floaterWindowActive))
				{
					mod->bringUpPanel = TRUE;
				}

				mod->floaterWindowActive = TRUE;

			}

			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			Rect rect;
			GetClientRect(hWnd, &rect);
			SelectObject(hdc, GetStockObject(WHITE_BRUSH));
			WhiteRect3D(hdc, rect, TRUE);
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			int delta = (short)HIWORD(wParam);

			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);


			mod->WheelZoom(xPos, yPos, delta);
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
			case IDCANCEL:
				break;
			default:
				IMenuBarContext* pContext = (IMenuBarContext*)GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
				if (pContext)
				{
					int id = LOWORD(wParam);
					int hid = HIWORD(wParam);
					if (hid == 0)
						pContext->ExecuteAction(id);
				}
			}
			return TRUE;
		}

		case WM_CLOSE:
		{
			mod->TearDownToolBarUIs();
			if (mod->mode == ID_WELD)
				mod->SetMode(mod->oldMode, FALSE);
			HWND maxHwnd = GetCOREInterface()->GetMAXHWnd();
			mod->fnSetMapMode(NOMAP);
			SetFocus(maxHwnd);
			DestroyWindow(hWnd);
			if (mod->hRelaxDialog)
			{
				DestroyWindow(mod->hRelaxDialog);
				mod->hRelaxDialog = NULL;
			}
			break;
		}

		case WM_DESTROY:
		{
			if (mod)
			{
				mod->closingMode = mod->mode;
				mod->SetMode(ID_FREEFORMMODE, FALSE);
				mod->DestroyDlg();
				GetCOREInterface()->UnRegisterDeleteUser(&delEvent);
				if (mod->hRelaxDialog)
				{
					DestroyWindow(mod->hRelaxDialog);
					mod->hRelaxDialog = NULL;
				}
			}
			break;
		}

		default:
			return FALSE;
		}
	}

	return TRUE;
}

static LRESULT CALLBACK UnwrapViewProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	switch (msg) {
	case WM_CREATE:
		break;

	case WM_SIZE:
		if (mod)
		{
			//watje tile
			mod->tileValid = FALSE;
			mod->InvalidateView();

			if (!MaxSDK::Graphics::IsRetainedModeEnabled())
			{
				//under the legacy mode, let the off-screen buffer resize with the view's resizing.
				if (Painter2DLegacy::Singleton().iBuf)
				{
					Painter2DLegacy::Singleton().iBuf->Resize();
				}

				if (Painter2DLegacy::Singleton().iTileBuf)
				{
					Painter2DLegacy::Singleton().iTileBuf->Resize();
				}
			}

		}
		break;
	case WM_PAINT:
		if (mod) mod->fnPaintView();
		break;
	case WM_MOUSEWHEEL:
	{

		int delta = (short)HIWORD(wParam);

		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		mod->WheelZoom(xPos, yPos, delta);

	}
	break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		SetFocus(mod->hView);
		if (mod->CanEnterCustomizedMode())
		{
			UnwrapMod::EnterCustomizedMode(mod);
			mod->mouseMan.MouseWinProc(hWnd, WM_LBUTTONDOWN, wParam, lParam);
			break;
		}
		return mod->mouseMan.MouseWinProc(hWnd, msg, wParam, lParam);
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
		SetFocus(mod->hView);
		return mod->mouseMan.MouseWinProc(hWnd, msg, wParam, lParam);

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if (mod->CanExitCustomizedMode())
		{
			mod->mouseMan.MouseWinProc(hWnd, WM_LBUTTONUP, wParam, lParam);
			UnwrapMod::ExitCustomizedMode(mod);
			break;
		}
		return mod->mouseMan.MouseWinProc(hWnd, msg, wParam, lParam);
	case WM_MOUSEMOVE:
		UnwrapMod::wParam = wParam;
		UnwrapMod::lParam = lParam;
		return mod->mouseMan.MouseWinProc(hWnd, msg, wParam, lParam);
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (mod->CanEnterCustomizedMode())
		{
			//Original mode invokes one mouse button up
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
				mod->mouseMan.MouseWinProc(hWnd, WM_LBUTTONUP, UnwrapMod::wParam, UnwrapMod::lParam);
			if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
				mod->mouseMan.MouseWinProc(hWnd, WM_MBUTTONUP, UnwrapMod::wParam, UnwrapMod::lParam);
			if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
				mod->mouseMan.MouseWinProc(hWnd, WM_RBUTTONUP, UnwrapMod::wParam, UnwrapMod::lParam);

			UnwrapMod::EnterCustomizedMode(mod);
			mod->mouseMan.MouseWinProc(hWnd, WM_LBUTTONDOWN, UnwrapMod::wParam, UnwrapMod::lParam);
		}
		return 0;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (mod->CanExitCustomizedMode())
		{
			mod->mouseMan.MouseWinProc(hWnd, WM_LBUTTONUP, wParam, lParam);
			UnwrapMod::ExitCustomizedMode(mod);
		}
		return 0;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

void UnwrapMod::DestroyDlg()
{
	UnwrapMod* mod = DLGetWindowLongPtr<UnwrapMod*>(hDialogWnd);
	if (mod)
	{
		mod->FreeSnapBuffer();
	}

	if (renderUVWindow != NULL)
	{
		DestroyWindow(renderUVWindow);
	}
	renderUVWindow = NULL;

	ColorMan()->SetColor(LINECOLORID, lineColor);
	ColorMan()->SetColor(SELCOLORID, selColor);
	ColorMan()->SetColor(OPENEDGECOLORID, openEdgeColor);
	ColorMan()->SetColor(HANDLECOLORID, handleColor);
	ColorMan()->SetColor(FREEFORMCOLORID, freeFormColor);
	ColorMan()->SetColor(SHAREDCOLORID, sharedColor);
	ColorMan()->SetColor(BACKGROUNDCOLORID, backgroundColor);
	ColorMan()->SetColor(PEELCOLORID, peelColor);
	ColorMan()->SetColor(GRIDCOLORID, gridColor);

	//get windowpos
	windowPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hDialogWnd, &windowPos);

	//watje tile
	tileValid = FALSE;

	delete moveMode; moveMode = NULL;
	delete freeFormMode; freeFormMode = NULL;
	delete sketchMode; sketchMode = NULL;

	delete paintSelectMode; paintSelectMode = NULL;

	delete rotMode; rotMode = NULL;
	delete scaleMode; scaleMode = NULL;
	delete panMode; panMode = NULL;
	delete zoomMode; zoomMode = NULL;
	delete zoomRegMode; zoomRegMode = NULL;
	delete weldMode; weldMode = NULL;

	//PELT
	delete peltStraightenMode; peltStraightenMode = NULL;

	delete rightMode; rightMode = NULL;
	delete middleMode; middleMode = NULL;
	mouseMan.SetMouseProc(NULL, LEFT_BUTTON, 0);
	mouseMan.SetMouseProc(NULL, RIGHT_BUTTON, 0);
	mouseMan.SetMouseProc(NULL, MIDDLE_BUTTON, 0);
	GetCOREInterface()->UnRegisterDlgWnd(hDialogWnd);


	if (mode == ID_SKETCHMODE)
		SetMode(ID_MOVE);

	hDialogWnd = NULL;

	if (CurrentMap == 0 || CurrentMap == TEXTURECHECKERINDEX)
		ShowCheckerMaterial(FALSE);
}

void UnwrapMod::SetupDlg(HWND hWnd)
{
	if ((CurrentMap < TEXTURECHECKERINDEX) || (CurrentMap >= pblock->Count(unwrap_texmaplist)))
		CurrentMap = 0;

	lineColor = ColorMan()->GetColor(LINECOLORID);
	selColor = ColorMan()->GetColor(SELCOLORID);
	openEdgeColor = ColorMan()->GetColor(OPENEDGECOLORID);
	handleColor = ColorMan()->GetColor(HANDLECOLORID);
	freeFormColor = ColorMan()->GetColor(FREEFORMCOLORID);
	sharedColor = ColorMan()->GetColor(SHAREDCOLORID);
	backgroundColor = ColorMan()->GetColor(BACKGROUNDCOLORID);
	peelColor = ColorMan()->GetColor(PEELCOLORID);
	gridColor = ColorMan()->GetColor(GRIDCOLORID);

	hDialogWnd = hWnd;

	hView = GetDlgItem(hWnd, IDC_UNWRAP_VIEW);
	DLSetWindowLongPtr(hView, this);
	viewValid = FALSE;

	tileValid = FALSE;

	moveMode = new MoveMode(this);
	rotMode = new RotateMode(this);
	scaleMode = new ScaleMode(this);
	panMode = new PanMode(this);
	zoomMode = new ZoomMode(this);
	zoomRegMode = new ZoomRegMode(this);
	weldMode = new WeldMode(this);

	iOriginalMode = -1;
	iOriginalOldMode = -1;
	bCustomizedZoomMode = false;
	bCustomizedPanMode = false;

	//PELT
	peltStraightenMode = new PeltStraightenMode(this);

	rightMode = new RightMouseMode(this);
	middleMode = new MiddleMouseMode(this);
	freeFormMode = new FreeFormMode(this);
	sketchMode = new SketchMode(this);
	paintSelectMode = new PaintSelectMode(this);
	paintMoveMode = new PaintMoveMode(this);
	relaxMoveMode = new RelaxMoveMode(this, 0);

	mouseMan.SetMouseProc(rightMode, RIGHT_BUTTON, 1);
	mouseMan.SetMouseProc(middleMode, MIDDLE_BUTTON, 2);


	SetupToolBarUIs();
	SizeDlg();
	SetMode(closingMode, FALSE);


	ResetEditorMatIDList();

	if (!MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		//under the legacy mode, should destroy the off-screen buffer that is created with the previous window handle.
		Painter2DLegacy::Singleton().DestroyOffScreenBuffer();
	}
}

void UnwrapMod::ResetEditorMatIDList()
{
	matid = -1;
	BuildMatIDList();
	SetMatFilters();

	UpdateEditorMatListBox();

	if (pblock->Count(unwrap_texmaplist) == 0)
		ResetMaterialList();
}

void UnwrapMod::SizeDlg()
{
	Rect rect;

	GetClientRect(hDialogWnd, &rect);

	mUIManager.UpdatePositions();

	HWND hwndSideBar = GetDlgItem(hDialogWnd, IDC_SIDEBAR_ROLLOUT);
	IRollupWindow *irollup = GetIRollup(hwndSideBar);
	// Stop here if there is no rollup or if there are no panels
	if (irollup)
	{
		if (irollup->GetNumPanels() > 0)
		{
			HWND hwndRect = irollup->GetPanelDlg(0);
			int winWidth = rect.w();

			// Get size of panel to resize the control holding the rollup
			RECT panelRect;
			GetWindowRect(hwndRect, &panelRect);
			int panelWidth = panelRect.right - panelRect.left;

			// add 16 to the width (magic number: scroll bar does not scale)
			int sideBarWidth = MaxSDK::UIScaled(16) + panelWidth;
			int sideBarXPos = winWidth - sideBarWidth - MaxSDK::UIScaled(2);

			// Resize view (6 pixels for borders)
			int viewWidth = sideBarXPos - MaxSDK::UIScaled(6);
			MoveWindow(GetDlgItem(hDialogWnd, IDC_UNWRAP_VIEW), 2, 2, viewWidth, rect.h() - 5, FALSE);

			int topBars = mUIManager.GetCornerHeight(1);
			int bottomBars = mUIManager.GetCornerHeight(3);
			int height = rect.h() - 5;
			height -= topBars;
			height -= bottomBars;

			// resize sidebar
			MoveWindow(hwndSideBar, sideBarXPos, topBars, sideBarWidth, height, TRUE);

			InvalidateRect(hwndSideBar, NULL, TRUE);
			InvalidateRect(hDialogWnd, NULL, TRUE);
		}
		ReleaseIRollup(irollup); 
	}
}

Point2 UnwrapMod::UVWToScreen(Point3 pt, float xzoom, float yzoom, int w, int h)
{
	int i1, i2;
	GetUVWIndices(i1, i2);
	int tx = (w - int(xzoom)) / 2;
	int ty = (h - int(yzoom)) / 2;
	return Point2(pt[i1] * xzoom + xscroll + tx, (float(h) - pt[i2] * yzoom) + yscroll - ty);
}

void UnwrapMod::ComputeZooms(
	HWND hWnd, float &xzoom, float &yzoom, int &width, int &height)
{
	Rect rect;
	GetClientRect(hWnd, &rect);
	width = rect.w() - 1;
	height = rect.h() - 1;

	if (zoom < 0.000001f) zoom = 0.000001f;
	if (zoom > 100000.0f) zoom = 100000.0f;

	if (lockAspect)
		xzoom = zoom*aspect*float(height);
	else xzoom = zoom*aspect*float(width);
	yzoom = zoom*float(height);

}


void UnwrapMod::SetMatFilters()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		int id = -1;
		if ((matid >= 0) && matid < filterMatID.Count())
			id = filterMatID[matid];
		mMeshTopoData[ldID]->BuildMatIDFilter(id);
	}
}

void UnwrapMod::UpdateEditorMatListBox()
{
	if (!IsWindow(hMatIDs))
		return;

	Tab<int> matIDs;
	Tab<Mtl*> matList;
	MultiMtl* mtl = NULL;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		INode *node = mMeshTopoData.GetNode(ldID);
		Mtl *baseMtl = node->GetMtl();
		if (baseMtl)
		{
			mtl = (MultiMtl*)GetMultiMaterials(baseMtl);
			if (mtl)
			{
				IParamBlock2 *pb = mtl->GetParamBlock(0);
				if (pb)
				{

					int numMaterials = pb->Count(multi_mtls);
					for (int i = 0; i < numMaterials; i++)
					{
						int id = 0;
						Mtl *mat = NULL;
						pb->GetValue(multi_mtls, 0, mat, FOREVER, i);
						pb->GetValue(multi_ids, 0, id, FOREVER, i);

						BOOL dupe = FALSE;
						for (int m = 0; m < matIDs.Count(); m++)
						{
							if (matIDs[m] == id)
								dupe = TRUE;
						}
						if (!dupe)
						{
							matIDs.Append(1, &id, 100);
							matList.Append(1, &mat, 100);
						}
					}
				}
			}
		}
	}
	SendMessage(hMatIDs, CB_RESETCONTENT, 0, 0);
	// add the default "All IDs" as first item.
	SendMessage(hMatIDs, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_ID_ALLID));
	for (int i = 0; i < filterMatID.Count(); i++)
	{
		TSTR st;
		if (mtl)
		{
			int id = filterMatID[i];

			int matchMat = -1;
			for (int j = 0; j < matIDs.Count(); j++)
			{
				if (id == matIDs[j])
					matchMat = j;
			}
			if ((matchMat == -1) || (matList[matchMat] == NULL))
				st.printf(_T("%d"), filterMatID[i] + 1);
			else
				st.printf(_T("%d:%s"), filterMatID[i] + 1, matList[matchMat]->GetFullName());

		}
		else
			st.printf(_T("%d"), filterMatID[i] + 1);
		SendMessage(hMatIDs, CB_ADDSTRING, 0, (LPARAM)(TCHAR*)(st.data()));
	}

	SendMessage(hMatIDs, CB_SETCURSEL, matid + 1, 0);
}

void UnwrapMod::UpdateMatFilters()
{
	int oldFilterID = -1;
	if ((matid >= 0) && matid < filterMatID.Count())
		oldFilterID = filterMatID[matid];

	Tab<int> oldFilter;
	int filterCount = filterMatID.Count();
	oldFilter.SetCount(filterCount);
	for (auto i = 0; i < filterCount; ++i)
	{
		oldFilter[i] = filterMatID[i];
	}
	BuildMatIDList();
	matid = -1;//relocate the previous filter mat id in the new list.
	for (auto i = 0; i < filterMatID.Count(); ++i)
	{
		if (filterMatID[i] == oldFilterID)
		{
			matid = i;
			break;
		}
	}
	
	UpdateEditorMatListBox();

	SetMatFilters();
}

bool UnwrapMod::GetAutoPackEnabled()
{
	return sAutoPack;
}

void UnwrapMod::SetAutoPackEnabled(bool bAutoPack)
{
	sAutoPack = bAutoPack;
}

bool UnwrapMod::GetLivePeelEnabled() const
{
	return PeelModeDialog::GetSingleton().GetLivePeel();
}

void UnwrapMod::SetLivePeelEnabled(bool bLivePeel)
{
	PeelModeDialog::GetSingleton().SetLivePeel(bLivePeel);
}

void UnwrapMod::SyncPeelModeSettingToIniFile()
{
	PeelModeDialog::GetSingleton().SaveIniFile();
}

void UnwrapMod::InvalidateView()
{
	viewValid = FALSE;
	if (hView) {
		InvalidateRect(hView, NULL, TRUE);
	}
}

void UnwrapMod::SetMode(int m, BOOL updateMenuBar)
{
	BOOL invalidView = FALSE;
	if ((mode == ID_SKETCHMODE) && (m != ID_SKETCHMODE))
	{

		if (theHold.Holding())
			theHold.Accept(GetString(IDS_PW_MOVE_UVW));

		invalidView = TRUE;
		sketchSelMode = restoreSketchSelMode;
		sketchType = restoreSketchType;
		sketchDisplayPoints = restoreSketchDisplayPoints;
		sketchInteractiveMode = restoreSketchInteractiveMode;

		sketchCursorSize = restoreSketchCursorSize;
	}

	if ((mode == ID_FREEFORMMODE) || (mode == ID_TV_PAINTSELECTMODE))
		invalidView = TRUE;

	if (mode == m)
	{
		if ((m == ID_WELD) ||
			(m == ID_ZOOMTOOL) ||
			(m == ID_ZOOMREGION) ||
			(m == ID_TV_PAINTSELECTMODE) ||
			(m == ID_PAN) || 
			(m == ID_PAINT_MOVE_BRUSH) ||
			(m == ID_RELAX_MOVE_BRUSH))
		{
			m = ID_MOVE;
			mode = ID_MOVE;
		}
	}

	oldMode = mode;
	mode = m;

	if (oldMode == ID_PAINT_MOVE_BRUSH)
	{
		fnSetEnableSoftSelection(paintMoveMode->bSoftSelEnabled);
		invalidView = TRUE;
	}

	if (oldMode == ID_RELAX_MOVE_BRUSH)
	{
		fnSetEnableSoftSelection(relaxMoveMode->bSoftSelEnabled);
		invalidView = TRUE;
	}

	switch (mode) {
	case ID_PAINT_MOVE_BRUSH:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.PaintMoveBrush"));
		macroRecorder->Assign(mstr, mr_bool, TRUE);
		mouseMan.SetMouseProc(paintMoveMode, LEFT_BUTTON);
		paintMoveMode->bSoftSelEnabled = fnGetEnableSoftSelection();
		fnSetEnableSoftSelection(FALSE);
		paintMoveMode->first = TRUE;
		break;
	}
	case ID_RELAX_MOVE_BRUSH:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.RelaxMoveBrush"));
		macroRecorder->Assign(mstr, mr_bool, TRUE);
		mouseMan.SetMouseProc(relaxMoveMode, LEFT_BUTTON);
		relaxMoveMode->bSoftSelEnabled = fnGetEnableSoftSelection();
		fnSetEnableSoftSelection(FALSE);
		relaxMoveMode->first = TRUE;
		break;
	}
	case ID_TV_PAINTSELECTMODE:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.setPaintSelectMode"));
		macroRecorder->FunctionCall(mstr, 1, 0, mr_bool, TRUE);
		macroRecorder->EmitScript();
		mouseMan.SetMouseProc(paintSelectMode, LEFT_BUTTON);
		paintSelectMode->first = TRUE;
		break;
	}
	case ID_SKETCHMODE:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.sketchNoParams"));
		macroRecorder->FunctionCall(mstr, 0, 0);
		macroRecorder->EmitScript();
		sketchMode->pointCount = 0;
		mouseMan.SetMouseProc(sketchMode, LEFT_BUTTON);
		break;
	}
	case ID_FREEFORMMODE:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.setFreeFormMode"));
		macroRecorder->FunctionCall(mstr, 1, 0,
			mr_bool, TRUE);
		macroRecorder->EmitScript();
		freeFormSubMode = ID_MOVE;
		SetupTypeins();
		mouseMan.SetMouseProc(freeFormMode, LEFT_BUTTON);
		invalidView = TRUE;
		break;
	}
	case ID_MOVE:
	{
		move = mUIManager.GetFlyOut(ID_MOVE);
		if (move == 0)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.move"));
			macroRecorder->FunctionCall(mstr, 0, 0);
		}
		else if (move == 1)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.moveh"));
			macroRecorder->FunctionCall(mstr, 0, 0);
		}
		else if (move == 2)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.movev"));
			macroRecorder->FunctionCall(mstr, 0, 0);
		}
		macroRecorder->EmitScript();

		SetupTypeins();
		mouseMan.SetMouseProc(moveMode, LEFT_BUTTON);
		break;
	}

	case ID_ROTATE:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.rotate"));
		macroRecorder->FunctionCall(mstr, 0, 0);
		macroRecorder->EmitScript();

		SetupTypeins();
		mouseMan.SetMouseProc(rotMode, LEFT_BUTTON);
		break;
	}

	case ID_SCALE:
	{
		scale = mUIManager.GetFlyOut(ID_SCALE);

		if (scale == 0)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.scale"));
			macroRecorder->FunctionCall(mstr, 0, 0);
		}
		else if (scale == 1)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.scaleh"));
			macroRecorder->FunctionCall(mstr, 0, 0);
		}
		else if (scale == 2)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.scalev"));
			macroRecorder->FunctionCall(mstr, 0, 0);
		}
		macroRecorder->EmitScript();

		SetupTypeins();
		mouseMan.SetMouseProc(scaleMode, LEFT_BUTTON);
		break;
	}
	case ID_WELD:
	{
		if (fnGetTVSubMode() != TVFACEMODE)
		{
			TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.weld"));
			macroRecorder->FunctionCall(mstr, 0, 0);
			macroRecorder->EmitScript();

			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				if (fnGetTVSubMode() == TVVERTMODE)
					ld->ClearSelection(TVVERTMODE);//vsel.ClearAll();			
				else if (fnGetTVSubMode() == TVEDGEMODE)
					ld->ClearSelection(TVEDGEMODE);//esel.ClearAll();			
			}

			if (ip) ip->RedrawViews(ip->GetTime());

			mouseMan.SetMouseProc(weldMode, LEFT_BUTTON);
		}
		break;
	}

	case ID_TOOL_PELTSTRAIGHTEN:
	{
		peltData.peltDialog.SetStraightenButton(TRUE);
		if (ip) ip->RedrawViews(ip->GetTime());
		mouseMan.SetMouseProc(peltStraightenMode, LEFT_BUTTON);
		break;
	}
	case ID_PAN:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.pan"));
		macroRecorder->FunctionCall(mstr, 0, 0);
		macroRecorder->EmitScript();

		mouseMan.SetMouseProc(panMode, LEFT_BUTTON);
		break;
	}

	case ID_ZOOMTOOL:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.zoom"));
		macroRecorder->FunctionCall(mstr, 0, 0);
		macroRecorder->EmitScript();

		mouseMan.SetMouseProc(zoomMode, LEFT_BUTTON);
		break;
	}
	case ID_ZOOMREGION:
	{
		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.zoomRegion"));
		macroRecorder->FunctionCall(mstr, 0, 0);
		macroRecorder->EmitScript();

		mouseMan.SetMouseProc(zoomRegMode, LEFT_BUTTON);
		break;
	}
	}

	if (updateMenuBar)
	{
		if (hDialogWnd)
		{
			IMenuBarContext* pContext = (IMenuBarContext*)GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
			if (pContext)
				pContext->UpdateWindowsMenu();
		}
	}
	if (invalidView) 
		InvalidateView();
	mUIManager.UpdateCheckButtons();

}

void UnwrapMod::RegisterClasses()
{
	if (!hToolImages) {
		HBITMAP hBitmap, hMask;
		hToolImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_TRANSFORM));
		hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_TRANSFORM_MASK));
		ImageList_Add(hToolImages, hBitmap, hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	if (!hOptionImages) {
		HBITMAP hBitmap, hMask;
		hOptionImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_OPTION));
		hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_OPTION_MASK));
		ImageList_Add(hOptionImages, hBitmap, hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	if (!hViewImages) {
		HBITMAP hBitmap, hMask;
		hViewImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_VIEW));
		hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_VIEW_MASK));
		ImageList_Add(hViewImages, hBitmap, hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	if (!hVertexImages) {
		HBITMAP hBitmap, hMask;
		hVertexImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_VERT));
		hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNWRAP_VERT_MASK));
		ImageList_Add(hVertexImages, hBitmap, hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	static BOOL registered = FALSE;
	if (!registered) {
		registered = TRUE;
		WNDCLASS  wc;
		wc.style = CS_DBLCLKS;
		wc.hInstance = hInstance;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL; //(HBRUSH)GetStockObject(WHITE_BRUSH);	
		wc.lpszMenuName = NULL;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.lpfnWndProc = UnwrapViewProc;
		wc.lpszClassName = GetString(IDS_PW_UNWRAPVIEW);
		RegisterClass(&wc);
	}
}

/*************************************************************

Modified from Graphics Gem 3 Fast Liner intersection by Franklin Antonio

**************************************************************/



BOOL UnwrapMod::LineIntersect(Point3 p1, Point3 p2, Point3 q1, Point3 q2)
{


	float a, b, c, d, det;  /* parameter calculation variables */
	float max1, max2, min1, min2; /* bounding box check variables */

	/*  First make the bounding box test. */
	max1 = maxmin(p1.x, p2.x, min1);
	max2 = maxmin(q1.x, q2.x, min2);
	if ((max1 < min2) || (min1 > max2)) return(FALSE); /* no intersection */
	max1 = maxmin(p1.y, p2.y, min1);
	max2 = maxmin(q1.y, q2.y, min2);
	if ((max1 < min2) || (min1 > max2)) return(FALSE); /* no intersection */

	/* See if the endpoints of the second segment lie on the opposite
	sides of the first.  If not, return 0. */
	a = (q1.x - p1.x) * (p2.y - p1.y) -
		(q1.y - p1.y) * (p2.x - p1.x);
	b = (q2.x - p1.x) * (p2.y - p1.y) -
		(q2.y - p1.y) * (p2.x - p1.x);
	if (a != 0.0f && b != 0.0f && SAME_SIGNS(a, b)) return(FALSE);

	/* See if the endpoints of the first segment lie on the opposite
	sides of the second.  If not, return 0.  */
	c = (p1.x - q1.x) * (q2.y - q1.y) -
		(p1.y - q1.y) * (q2.x - q1.x);
	d = (p2.x - q1.x) * (q2.y - q1.y) -
		(p2.y - q1.y) * (q2.x - q1.x);
	if (c != 0.0f && d != 0.0f && SAME_SIGNS(c, d)) return(FALSE);

	/* At this point each segment meets the line of the other. */
	det = a - b;
	if (det == 0.0f) return(FALSE); /* The segments are colinear.  Determining */
	return(TRUE);
}

BOOL UnwrapMod::HitTestCircle(IPoint2& center, float radius, Tab<TVHitData>& hits)
{
	float xzoom, yzoom;
	int width, height;
	ComputeZooms(hView, xzoom, yzoom, width, height);

	float radiusSquare = radius*radius;
	//loop through all our local data
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			//check to see if we hit a vertex
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (ld->GetTVVertHidden(i)) continue;
				if (ld->GetTVVertFrozen(i)) continue;
				if (!ld->IsTVVertVisible(i)) continue;

				Point2 p2 = UVWToScreen(ld->GetTVVert(i), xzoom, yzoom, width, height);

				float lengthSquare = (p2.x - center.x)*(p2.x - center.x) + (p2.y - center.y)*(p2.y - center.y);

				if (lengthSquare < radiusSquare)
				{
					TVHitData d;
					d.mLocalDataID = ldID;
					d.mID = i;
					hits.Append(1, &d, 100);
				}
			}
		}
	}
	return hits.Count()>0 ? TRUE : FALSE;
}

BOOL UnwrapMod::HitTest(Rect rect, Tab<TVHitData> &hits, BOOL selOnly, BOOL circleMode)
{
	//we dont let hit testing happen during the inertactive mapping so they cannot change there selection
	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		return FALSE;

	//if you are in pelt mode but dont have the dialog up also prevent the selection
	if ((fnGetMapMode() == PELTMAP) && (peltData.peltDialog.hWnd == NULL))
		return FALSE;

	Point2 pt;
	float xzoom, yzoom;
	int width, height;
	ComputeZooms(hView, xzoom, yzoom, width, height);


	Point2 center = UVWToScreen(Point3(0.0f, 0.0f, 0.0f), xzoom, yzoom, width, height);
	Point3 p1(10000.f, 10000.f, 10000.f), p2(-10000.f, -10000.f, -10000.f);

	Rect smallRect = rect;

	if (circleMode)
	{

	}
	else
	{
		//just builds a quick vertex bounding box
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			if ((abs(rect.left - rect.right) <= 4) && (abs(rect.bottom - rect.top) <= 4))
			{
				rect.left -= hitSize;
				rect.right += hitSize;
				rect.top -= hitSize;
				rect.bottom += hitSize;

				smallRect.left -= 1;
				smallRect.right += 1;
				smallRect.top -= 1;
				smallRect.bottom += 1;
			}
		}
	}

	Rect rectFF = rect;
	if ((abs(rect.left - rect.right) <= 4) && (abs(rect.bottom - rect.top) <= 4))
	{
		rectFF.left -= hitSize;
		rectFF.right += hitSize;
		rectFF.top -= hitSize;
		rectFF.bottom += hitSize;
	}

	int i1, i2;
	GetUVWIndices(i1, i2);

	p1[i1] = ((float)rectFF.left - center.x) / xzoom;
	p2[i1] = ((float)rectFF.right - center.x) / xzoom;

	p1[i2] = -((float)rectFF.top - center.y) / yzoom;
	p2[i2] = -((float)rectFF.bottom - center.y) / yzoom;

	Box3 boundsFF;
	boundsFF.Init();
	boundsFF += p1;
	boundsFF += p2;

	p1[i1] = ((float)rect.left - center.x) / xzoom;
	p2[i1] = ((float)rect.right - center.x) / xzoom;

	p1[i2] = -((float)rect.top - center.y) / yzoom;
	p2[i2] = -((float)rect.bottom - center.y) / yzoom;

	Box3 bounds;
	bounds.Init();
	bounds += p1;
	bounds += p2;

	//compute a screen rect bounding box which we use to do the mouse pick
	Point3 smallp1(10000.f, 10000.f, 10000.f), smallp2(-10000.f, -10000.f, -10000.f);
	smallp1[i1] = ((float)smallRect.left - center.x) / xzoom;
	smallp2[i1] = ((float)smallRect.right - center.x) / xzoom;

	smallp1[i2] = -((float)smallRect.top - center.y) / yzoom;
	smallp2[i2] = -((float)smallRect.bottom - center.y) / yzoom;

	Box3 smallBounds;
	smallBounds.Init();
	smallBounds += smallp1;
	smallBounds += smallp2;

	mMouseHitLocalData = NULL;

	// if we are doing a grid snap we need to find the anchor vertex which is the first vertex
	// that use clicked on
	if (snapToggle)
	{
		//loop through all our local data
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld)
			{
				//check to see if we hit a vertex
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if (selOnly && !ld->GetTVVertSelected(i)) continue;
					if (ld->GetTVVertHidden(i)) continue;
					if (ld->GetTVVertFrozen(i)) continue;

					if (!ld->IsTVVertVisible(i)) continue;

					if (bounds.Contains(ld->GetTVVert(i)))
					{
						mMouseHitLocalData = ld;
						mMouseHitVert = i;
						i = ld->GetNumberTVVerts();
						ldID = mMeshTopoData.Count();
					}
				}
			}
		}
	}
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		//loop through all our local data
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld)
			{
				//check to see if we hit a vertex
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if (selOnly && !ld->GetTVVertSelected(i)) continue;
					if (ld->GetTVVertHidden(i)) continue;
					if (ld->GetTVVertFrozen(i)) continue;
					if (!ld->IsTVVertVisible(i)) continue;

					Point3 p(0.0f, 0.0f, 0.0f);
					p[i1] = ld->GetTVVert(i)[i1];
					p[i2] = ld->GetTVVert(i)[i2];

					if (bounds.Contains(p))
					{
						TVHitData d;
						d.mLocalDataID = ldID;
						d.mID = i;
						hits.Append(1, &d, 1000);
					}
				}
			}
		}
	}
	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		//loop through all our local data
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld)
			{
				//check if single click
				if ((abs(rect.left - rect.right) <= 4) && (abs(rect.bottom - rect.top) <= 4))
				{
					Point3 center = bounds.Center();
					float threshold = (1.0f / xzoom) * 2.0f;
					int edgeIndex = ld->EdgeIntersect(center, threshold, i1, i2);

					if (edgeIndex >= 0)
					{
						BitArray sel;
						sel.SetSize(ld->GetNumberTVVerts());
						sel.ClearAll();

						BitArray tempVSel;
						if (selOnly)
							ld->GetVertSelFromEdge(tempVSel);

						int index1 = ld->GetTVEdgeVert(edgeIndex, 0);
						if (selOnly && !tempVSel[index1]) edgeIndex = -1;
						if (ld->GetTVVertHidden(index1)) edgeIndex = -1;
						if (ld->GetTVVertFrozen(index1)) edgeIndex = -1;
						if (!ld->IsTVVertVisible(index1)) edgeIndex = -1;

						if (edgeIndex >= 0)
						{
							int index2 = ld->GetTVEdgeVert(edgeIndex, 1);

							if (selOnly && !tempVSel[index2]) edgeIndex = -1;
							if (ld->GetTVVertHidden(index2)) edgeIndex = -1;
							if (ld->GetTVVertFrozen(index2)) edgeIndex = -1;
							if (!ld->IsTVVertVisible(index2)) edgeIndex = -1;
						}
					}

					if (edgeIndex >= 0)
					{
						if (selOnly && !ld->GetTVEdgeSelected(edgeIndex))
						{
						}
						else
						{
							TVHitData d;
							d.mLocalDataID = ldID;
							d.mID = edgeIndex;
							hits.Append(1, &d, 10);
						}
					}
				}
				//else it is a drag rect
				else
				{
					BitArray sel;
					sel.SetSize(ld->GetNumberTVVerts());
					sel.ClearAll();

					BitArray tempVSel;
					if (selOnly)
						ld->GetVertSelFromEdge(tempVSel);
					Tab<Point2> screenSpaceList;
					screenSpaceList.SetCount(ld->GetNumberTVVerts());
					for (int i = 0; i < screenSpaceList.Count(); i++)
						screenSpaceList[i] = UVWToScreen(ld->GetTVVert(i), xzoom, yzoom, width, height);

					Point3 rectPoints[4];

					rectPoints[0].x = rect.left;
					rectPoints[0].y = rect.top;
					rectPoints[0].z = 0.0f;

					rectPoints[1].x = rect.right;
					rectPoints[1].y = rect.top;
					rectPoints[1].z = 0.0f;

					rectPoints[2].x = rect.left;
					rectPoints[2].y = rect.bottom;
					rectPoints[2].z = 0.0f;

					rectPoints[3].x = rect.right;
					rectPoints[3].y = rect.bottom;
					rectPoints[3].z = 0.0f;

					BOOL cross = GetCOREInterface()->GetCrossing();

					for (int i = 0; i < ld->GetNumberTVEdges(); i++)
					{
						int index1 = ld->GetTVEdgeVert(i, 0);

						if (selOnly && !tempVSel[index1]) continue;
						if (ld->GetTVVertHidden(index1)) continue;
						if (ld->GetTVVertFrozen(index1)) continue;
						if (!ld->IsTVVertVisible(index1)) continue;

						int index2 = ld->GetTVEdgeVert(i, 1);

						if (selOnly && !tempVSel[index2]) continue;
						if (ld->GetTVVertHidden(index2)) continue;
						if (ld->GetTVVertFrozen(index2)) continue;
						if (!ld->IsTVVertVisible(index2)) continue;

						Point2 a = screenSpaceList[index1];
						Point2 b = screenSpaceList[index2];

						BOOL hitEdge = FALSE;
						if ((a.x >= rect.left) && (a.x <= rect.right) &&
							(a.y <= rect.bottom) && (a.y >= rect.top) &&
							(b.x >= rect.left) && (b.x <= rect.right) &&
							(b.y <= rect.bottom) && (b.y >= rect.top))
						{
							TVHitData d;
							d.mLocalDataID = ldID;
							d.mID = i;
							hits.Append(1, &d, 500);
							hitEdge = TRUE;
						}

						Point3 ap, bp;
						ap.x = a.x;
						ap.y = a.y;
						ap.z = 0.0f;
						bp.x = b.x;
						bp.y = b.y;
						bp.z = 0.0f;

						if ((cross) && (!hitEdge))
						{
							if (LineIntersect(ap, bp, rectPoints[0], rectPoints[1]))
							{
								TVHitData d;
								d.mLocalDataID = ldID;
								d.mID = i;
								hits.Append(1, &d, 500);
							}
							else if (LineIntersect(ap, bp, rectPoints[2], rectPoints[3]))
							{
								TVHitData d;
								d.mLocalDataID = ldID;
								d.mID = i;
								hits.Append(1, &d, 500);
							}
							else if (LineIntersect(ap, bp, rectPoints[0], rectPoints[2]))
							{
								TVHitData d;
								d.mLocalDataID = ldID;
								d.mID = i;
								hits.Append(1, &d, 500);
							}
							else if (LineIntersect(ap, bp, rectPoints[1], rectPoints[3]))
							{
								TVHitData d;
								d.mLocalDataID = ldID;
								d.mID = i;
								hits.Append(1, &d, 500);
							}
						}
					}
				}
			}
		}
	}
	else if (fnGetTVSubMode() == TVFACEMODE)
	{
		//check if single click
		if ((abs(rect.left - rect.right) <= 4) && (abs(rect.bottom - rect.top) <= 4))
		{
			Point3 center(0.0f, 0.0f, 0.0f);

			Point3 tcent = bounds.Center();
			center.x = tcent[i1];
			center.y = tcent[i2];

			int faceIndex = -1;

			//loop through all our local data
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				if (ld)
				{
					if (selOnly)
					{
						BitArray ignoreFaces;
						ignoreFaces = ~ld->GetFaceSel();
						faceIndex = ld->PolyIntersect(center, i1, i2, &ignoreFaces);
					}
					else faceIndex = ld->PolyIntersect(center, i1, i2);

					if ((faceIndex >= 0) && !ld->GetFaceHidden(faceIndex))
					{
						if (selOnly && !ld->GetFaceSelected(faceIndex))
						{
						}
						else
						{
							TVHitData d;
							d.mLocalDataID = ldID;
							d.mID = faceIndex;
							hits.Append(1, &d, 500);
						}
					}
				}
			}
		}
		//else it is a drag rect
		else
		{
			//loop through all our local data
			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				if (ld)
				{
					/*
					BitArray sel;
					sel.SetSize(TVMaps.v.Count());
					sel.ClearAll();
					*/
					BitArray tempVSel;
					if (selOnly)
						ld->GetVertSelFromFace(tempVSel);

					Tab<Point2> screenSpaceList;
					screenSpaceList.SetCount(ld->GetNumberTVVerts());
					for (int i = 0; i < screenSpaceList.Count(); i++)
						screenSpaceList[i] = UVWToScreen(ld->GetTVVert(i), xzoom, yzoom, width, height);

					Point3 rectPoints[4];

					rectPoints[0].x = rect.left;
					rectPoints[0].y = rect.top;
					rectPoints[0].z = 0.0f;

					rectPoints[1].x = rect.right;
					rectPoints[1].y = rect.top;
					rectPoints[1].z = 0.0f;

					rectPoints[2].x = rect.left;
					rectPoints[2].y = rect.bottom;
					rectPoints[2].z = 0.0f;

					rectPoints[3].x = rect.right;
					rectPoints[3].y = rect.bottom;
					rectPoints[3].z = 0.0f;

					BOOL cross = GetCOREInterface()->GetCrossing();

					for (int i = 0; i < ld->GetNumberFaces(); i++)
					{
						int faceIndex = i;
						int total = 0;
						int pcount = ld->GetFaceDegree(faceIndex);
						BOOL frozen = FALSE;
						for (int k = 0; k < pcount; k++)
						{
							int index = ld->GetFaceTVVert(faceIndex, k);
							if (ld->GetTVVertFrozen(index))
								frozen = TRUE;
							if (!ld->IsTVVertVisible(index))
								frozen = TRUE;
							Point2 a = screenSpaceList[index];
							if ((a.x >= rect.left) && (a.x <= rect.right) &&
								(a.y <= rect.bottom) && (a.y >= rect.top))
								total++;
						}
						if (frozen)
						{
							total = 0;
						}
						else if (total == pcount)
						{
							TVHitData d;
							d.mLocalDataID = ldID;
							d.mID = i;
							hits.Append(1, &d, 500);
						}
						else if (cross)
						{
							for (int k = 0; k < pcount; k++)
							{
								int index = ld->GetFaceTVVert(faceIndex, k);
								int index2;
								if (k == (pcount - 1))
									index2 = ld->GetFaceTVVert(faceIndex, 0);
								else index2 = ld->GetFaceTVVert(faceIndex, k + 1);

								Point2 a = screenSpaceList[index];
								Point2 b = screenSpaceList[index2];

								Point3 ap, bp;
								ap.x = a.x;
								ap.y = a.y;
								ap.z = 0.0f;
								bp.x = b.x;
								bp.y = b.y;
								bp.z = 0.0f;
								if (LineIntersect(ap, bp, rectPoints[0], rectPoints[1]))
								{
									TVHitData d;
									d.mLocalDataID = ldID;
									d.mID = i;
									hits.Append(1, &d, 500);
									k = pcount;
								}
								else if (LineIntersect(ap, bp, rectPoints[2], rectPoints[3]))
								{
									TVHitData d;
									d.mLocalDataID = ldID;
									d.mID = i;
									hits.Append(1, &d, 500);
									k = pcount;
								}
								else if (LineIntersect(ap, bp, rectPoints[0], rectPoints[2]))
								{
									TVHitData d;
									d.mLocalDataID = ldID;
									d.mID = i;
									hits.Append(1, &d, 500);
									k = pcount;
								}
								else if (LineIntersect(ap, bp, rectPoints[1], rectPoints[3]))
								{
									TVHitData d;
									d.mLocalDataID = ldID;
									d.mID = i;
									hits.Append(1, &d, 500);
									k = pcount;
								}
							}
						}
					}
				}
			}
		}
	}

	BOOL bail = FALSE;
	int vselSet = 0;
	int eselSet = 0;
	int fselSet = 0;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			vselSet += ld->GetTVVertSel().NumberSet();
			eselSet += ld->GetTVEdgeSel().NumberSet();
			fselSet += ld->GetFaceSel().NumberSet();
		}
	}
	int prevFreeFormSubMode = freeFormSubMode;
	if (fnGetTVSubMode() == TVVERTMODE && (mode == ID_FREEFORMMODE))
	{
		if (vselSet == 0)
			bail = TRUE;

		freeFormSubMode = ID_SELECT;
	}
	else if (fnGetTVSubMode() == TVEDGEMODE && (mode == ID_FREEFORMMODE))
	{
		if (eselSet == 0)
			bail = TRUE;
		freeFormSubMode = ID_SELECT;
	}
	else if (fnGetTVSubMode() == TVFACEMODE && (mode == ID_FREEFORMMODE))
	{
		if (fselSet == 0)
			bail = TRUE;
		freeFormSubMode = ID_SELECT;
	}

	if (!bail && mode == ID_FREEFORMMODE)
	{
		//check if cursor is inside bounds
		Point3 center = bounds.Center();
		Point3 hold1, hold2;
		hold1 = p1;
		hold2 = p2;
		p1 = Point3(0.0f, 0.0f, 0.0f);
		p2 = Point3(0.0f, 0.0f, 0.0f);

		p1[i1] = hold1[i1];
		p1[i2] = hold1[i2];

		p2[i1] = hold2[i1];
		p2[i2] = hold2[i2];

		Point2 min = UVWToScreen(freeFormCorners[0], xzoom, yzoom, width, height);
		Point2 max = UVWToScreen(freeFormCorners[3], xzoom, yzoom, width, height);

		bounds = boundsFF;

		if ((fabs(max.x - min.x) < 40.0f) || (fabs(min.y - max.y) < 40.0f))
		{
			bounds = smallBounds;
		}

		Box3 quickDragBounds;
		quickDragBounds.Init();
		quickDragBounds += freeFormCorners[0];
		quickDragBounds += freeFormCorners[1];
		quickDragBounds += freeFormCorners[2];
		quickDragBounds += freeFormCorners[3];
		BOOL allowScaleRotate = TRUE;
		if (Length(quickDragBounds.Width()) < 0.0000001f)
			allowScaleRotate = FALSE;

		BOOL forceMove = FALSE;
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			if (vselSet <= 1)
				forceMove = TRUE;
		}
		else if (fnGetTVSubMode() == TVEDGEMODE)
		{
		}
		else if (fnGetTVSubMode() == TVFACEMODE)
		{
		}
		if ((!forceMove) && (smallBounds.Contains(selCenter + freeFormPivotOffset))
			&& (mode == ID_FREEFORMMODE))
		{
			freeFormSubMode = ID_MOVEPIVOT;
			return TRUE;
		}
		else if ((bounds.Contains(freeFormCorners[0]) || bounds.Contains(freeFormCorners[1]) ||
			bounds.Contains(freeFormCorners[2]) || bounds.Contains(freeFormCorners[3])) && allowScaleRotate)
		{
			freeFormSubMode = ID_SCALE;
			if (bounds.Contains(freeFormCorners[0]))
			{

				scaleCorner = 0;
				scaleCornerOpposite = 2;
			}
			else if (bounds.Contains(freeFormCorners[1]))
			{

				scaleCorner = 3;
				scaleCornerOpposite = 1;
			}
			else if (bounds.Contains(freeFormCorners[2]))
			{

				scaleCorner = 1;
				scaleCornerOpposite = 3;
			}
			else if (bounds.Contains(freeFormCorners[3]))
			{

				scaleCorner = 2;
				scaleCornerOpposite = 0;
			}
			return TRUE;
		}
		BOOL useMoveMode = FALSE;

		if (allowSelectionInsideGizmo)
		{
			if ((hits.Count() == 0) || forceMove)
				useMoveMode = TRUE;
			else useMoveMode = FALSE;
		}
		else
		{
			useMoveMode = TRUE;
		}
		
		Box3 freeFormBox;
		Box3 boundXForMove;
		Box3 boundYForMove;		
		Box3 boundBothForXYMove;
		ConstructFreeFormBoxesForMovingDirection(freeFormBox,boundXForMove,boundYForMove, boundBothForXYMove);

		//The mouse position is in the UVW space, not in the screen space.
		Point3 mouseP = (p1 + p2)*0.5f;

		if ((bounds.Contains(freeFormEdges[0]) || bounds.Contains(freeFormEdges[1]) ||
			bounds.Contains(freeFormEdges[2]) || bounds.Contains(freeFormEdges[3])) && allowScaleRotate)
		{
			if (mode == ID_FREEFORMMODE)
			{
				freeFormSubMode = ID_ROTATE;
				return TRUE;
			}
		}
		else if ((boundXForMove.Contains(mouseP) || 
				boundYForMove.Contains(mouseP) ||
				boundBothForXYMove.Contains(mouseP)) && useMoveMode)
		{
			if (mode == ID_FREEFORMMODE)
			{
				if (freeFormSubMode != ID_MOVE)
				{
					freeFormSubMode = ID_MOVE;
					freeFromViewValidate = FALSE;

					if (boundBothForXYMove.Contains(mouseP))
					{
						freeFormMoveAxisForDisplay = eMoveBoth;
					}
					else if(boundXForMove.Contains(mouseP))
					{
						freeFormMoveAxisForDisplay = eMoveX;
					}
					else
					{
						freeFormMoveAxisForDisplay = eMoveY;
					}
				}

				return TRUE;
			}
		}
		else if((GetKeyState(VK_CONTROL) & 0x8000) ||
			((GetKeyState(VK_MENU) & 0x8000)))
		{
			freeFormSubMode = ID_SELECT;
		}
		else
		{
			if(hits.Count() > 0 && freeFormBox.Contains(mouseP))
			{
				freeFormSubMode = ID_MOVE;
				freeFormMoveAxisForDisplay = freeFormMoveAxisLocked;
			}
			else
			{
				freeFormSubMode = ID_SELECT;
			}
		}

		if (prevFreeFormSubMode != freeFormSubMode)
			SetupTypeins();
	}

	return hits.Count();
}

void UnwrapMod::SetupTypeins()
{
	// TO DO: Performance.  Calling this here isn't always needed,
	// might degrade performance.  Instead could call manually,
	// before SetupTypeins(), on as-needed basis.
	// But, leaving it here minimizes bugs, eases testing.
	SetupPixelUnitSpinners();

	Point3 uv(0, 0, 0);
	BOOL found = FALSE;
	BOOL bSingleVertex = TRUE;

	Box3 bounds;
	bounds.Init();

	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld->GetTVVertSel().AnyBitSet())
		{
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (!ld->GetTVVertSelected(i)) continue;

				if (found)
				{
					Point3 p = ld->GetTVVert(i);
					if (uv.x != p.x ||
						uv.y != p.y ||
						uv.z != p.z)
					{
						bSingleVertex = FALSE;
						bounds += p;
					}
				}
				else
				{
					uv = ld->GetTVVert(i);
					found = TRUE;
					bounds += uv;
				}
			}
		}
	}
	TransferSelectionEnd(FALSE, FALSE);

	if (!found) 
	{
		mUIManager.Enable(ID_SPINNERU, FALSE);
		mUIManager.Enable(ID_SPINNERV, FALSE);
		mUIManager.Enable(ID_SPINNERW, FALSE);
	}
	else
	{
		if (absoluteTypeIn)
		{
			if(mode == ID_MOVE || (mode == ID_FREEFORMMODE && (freeFormSubMode == ID_MOVE || freeFormSubMode == ID_SELECT)))
			{
				mUIManager.Enable(ID_SPINNERU, TRUE);
				mUIManager.Enable(ID_SPINNERV, TRUE);
				mUIManager.Enable(ID_SPINNERW, TRUE);

				if (bSingleVertex)
				{
					mUIManager.SetSpinFValue(ID_SPINNERU, uv.x);
				}
				else
				{
					mUIManager.SetSpinFValue(ID_SPINNERU, bounds.Center()[0]);
				}

				if (bSingleVertex)
				{
					mUIManager.SetSpinFValue(ID_SPINNERV, uv.y);
				}
				else
				{
					mUIManager.SetSpinFValue(ID_SPINNERV, bounds.Center()[1]);
				}

				if (bSingleVertex)
				{
					mUIManager.SetSpinFValue(ID_SPINNERW, uv.z);
				}
				else
				{
					mUIManager.SetSpinFValue(ID_SPINNERW, bounds.Center()[2]);
				}
			}
			else if(mode == ID_SCALE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_SCALE))
			{
				mUIManager.Enable(ID_SPINNERU, FALSE);
				mUIManager.Enable(ID_SPINNERV, FALSE);
				mUIManager.Enable(ID_SPINNERW, FALSE);
			}
			else if(mode == ID_ROTATE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_ROTATE))
			{
				mUIManager.Enable(ID_SPINNERU, FALSE);
				mUIManager.Enable(ID_SPINNERV, FALSE);
				mUIManager.Enable(ID_SPINNERW, FALSE);
			}
		}
		else
		{
			if(mode == ID_MOVE || (mode == ID_FREEFORMMODE && (freeFormSubMode == ID_MOVE || freeFormSubMode == ID_SELECT)))
			{
				mUIManager.Enable(ID_SPINNERU, TRUE);
				mUIManager.Enable(ID_SPINNERV, TRUE);
				mUIManager.Enable(ID_SPINNERW, TRUE);

				mUIManager.SetSpinFValue(ID_SPINNERU, 0.0f);
				mUIManager.SetSpinFValue(ID_SPINNERV, 0.0f);
				mUIManager.SetSpinFValue(ID_SPINNERW, 0.0f);
			}
			else if(mode == ID_SCALE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_SCALE))
			{
				mUIManager.Enable(ID_SPINNERU, TRUE);
				mUIManager.Enable(ID_SPINNERV, TRUE);
				mUIManager.Enable(ID_SPINNERW, FALSE);

				mUIManager.SetSpinFValue(ID_SPINNERU, 100.0f);
				mUIManager.SetSpinFValue(ID_SPINNERV, 100.0f);
				mUIManager.SetSpinFValue(ID_SPINNERW, 100.0f);

				tempCenter.x = freeFormPivotScreenSpace.x;
				tempCenter.y = freeFormPivotScreenSpace.y;
			}
			else if(mode == ID_ROTATE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_ROTATE))
			{
				mUIManager.Enable(ID_SPINNERU, TRUE);
				mUIManager.Enable(ID_SPINNERV, FALSE);
				mUIManager.Enable(ID_SPINNERW, FALSE);
				
				mUIManager.SetSpinFValue(ID_SPINNERU, 0.0f);
				mUIManager.SetSpinFValue(ID_SPINNERV, 0.0f);
				mUIManager.SetSpinFValue(ID_SPINNERW, 0.0f);

				tempCenter.x = freeFormPivotScreenSpace.x;
				tempCenter.y = freeFormPivotScreenSpace.y;
			}
		}
	}

	UpdateGroupUI();
}

void UnwrapMod::DoubleClickSelect(Tab<TVHitData> &hits, BOOL toggle)
{
	if (fnGetPaintMode() || !hits.Count()) return;

	if (fnGetTVSubMode() != TVEDGEMODE && fnGetTVSubMode() != TVFACEMODE) return;

	DbgAssert(hits.Count() == 1);

	int objectID = hits[0].mID;
	int ldID = hits[0].mLocalDataID;

	MeshTopoData *ld = mMeshTopoData[ldID];
	if (!ld) return;

	switch (fnGetTVSubMode())
	{
	case TVEDGEMODE:
	{
		// hold the old selection
		BitArray holdSelection = ld->GetTVEdgeSel();

		// only select the current hit edge
		ld->ClearTVEdgeSelection();
		ld->SetTVEdgeSelected(objectID, TRUE);

		// select the loop of current hit edge
		SelectUVEdge(TRUE);

		if (toggle)
		{
			holdSelection |= ld->GetTVEdgeSel();
			ld->SetTVEdgeSel(holdSelection);
		}

		INode *node = mMeshTopoData.GetNode(ldID);

		TSTR functionCall;
		functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.selectEdgesByNode"), node->GetName());
		macroRecorder->FunctionCall(functionCall, 2, 0,
			mr_bitarray, ld->GetTVEdgeSelectionPtr(),
			mr_reftarg, node
			);

		macroRecorder->EmitScript();
		SyncSelect();
		// In UV edge double-click selection, the auto-pack should be forcely disabled as design.
		AutoPackDisabledGuard guard(this);
		AutoEdgeToSeamAndLSCMResolve();

	}
	break;
	case TVFACEMODE:
	{
		fnSelectElement();
	}
	break;
	}
}

void UnwrapMod::SyncSelect() {
	freeFormPivotOffset.x = 0.0f;
	freeFormPivotOffset.y = 0.0f;
	freeFormPivotOffset.z = 0.0f;

	if (fnGetSyncSelectionMode() && !fnGetPaintMode())
		fnSyncGeomSelection();

	RebuildDistCache();
}

AutoPackDisabledGuard::AutoPackDisabledGuard(UnwrapMod* pMod)
{
	mpMod = pMod;
	if (mpMod)
	{
		mTempAutoPackEnabled = mpMod->GetAutoPackEnabled();
		mpMod->SetAutoPackEnabled(false);
	}
}

AutoPackDisabledGuard::~AutoPackDisabledGuard()
{
	if (mpMod)
	{
		mpMod->SetAutoPackEnabled(mTempAutoPackEnabled);
	}
}

void UnwrapMod::Select(Tab<TVHitData> &hits, BOOL subtract, BOOL all)
{

	if (!fnGetPaintMode())
		HoldSelection();

	BitArray usedLDs;
	usedLDs.SetSize(mMeshTopoData.Count());
	usedLDs.ClearAll();


	if (fnGetTVSubMode() == TVVERTMODE)
	{
		BOOL filterPin = pblock->GetInt(unwrap_filterpin);
		for (int i = 0; i < hits.Count(); i++)
		{
			int vID = hits[i].mID;
			int ldID = hits[i].mLocalDataID;

			MeshTopoData *ld = mMeshTopoData[ldID];
			if (!ld)
				continue;

			if (filterPin)
			{
				if (ld->IsTVVertPinned(vID) == FALSE)
					continue;
			}

			usedLDs.Set(ldID, TRUE);
			if (ld->IsTVVertVisible(vID) && (!ld->GetTVVertFrozen(vID)) && (!ld->GetTVVertHidden(vID)))
			{
				if (subtract)
					ld->SetTVVertSelected(vID, FALSE);
				else
					ld->SetTVVertSelected(vID, TRUE);
				if (!all) break;
			}
		}
		if ((!subtract) && (mode != ID_WELD))
			SelectHandles(0);
	}

	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		for (int i = 0; i < hits.Count(); i++)
		{
			int eID = hits[i].mID;
			int ldID = hits[i].mLocalDataID;

			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld)
			{
				usedLDs.Set(ldID, TRUE);
				if (subtract)
					ld->SetTVEdgeSelected(eID, FALSE);
				else
					ld->SetTVEdgeSelected(eID, TRUE);

				if (!all)
					break;
			}
		}
	}

	else if (fnGetTVSubMode() == TVFACEMODE)
	{
		for (int i = 0; i < hits.Count(); i++)
		{
			int fID = hits[i].mID;
			int ldID = hits[i].mLocalDataID;

			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld)
			{
				usedLDs.Set(ldID, TRUE);
				if ((ld->IsFaceVisible(fID)) && (!ld->GetFaceFrozen(fID)))
				{
					if (subtract)
						ld->SetFaceSelected(fID, FALSE);
					else
						ld->SetFaceSelected(fID, TRUE);
					if (!all)
						break;
				}
			}
		}
		//		ld->BuilFilterSelectedFaces(filterSelectedFaces);
	}


	if ((tvElementMode) && (mode != ID_WELD))
	{
		if (subtract)
			SelectElement(FALSE);
		else SelectElement(TRUE);
	}

	if ((uvEdgeMode) && (fnGetTVSubMode() == TVEDGEMODE))
	{
		if (!fnGetPaintMode())
			SelectUVEdge(FALSE);
	}
	if ((openEdgeMode) && (fnGetTVSubMode() == TVEDGEMODE))
	{
		if (!fnGetPaintMode())
			SelectOpenEdge();
	}
	if ((polyMode) && (fnGetTVSubMode() == TVFACEMODE))
	{
		theHold.Suspend();
		if (subtract)
			fnPolySelect2(FALSE);
		else fnPolySelect();
		theHold.Resume();
	}



	if (!fnGetPaintMode())
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			if (usedLDs[ldID])
			{
				MeshTopoData *ld = mMeshTopoData[ldID];
				INode *node = mMeshTopoData.GetNode(ldID);

				TSTR functionCall;

				if (fnGetTVSubMode() == TVVERTMODE)
				{
					functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.selectVerticesByNode"), node->GetName());
					macroRecorder->FunctionCall(functionCall, 2, 0,
						mr_bitarray, ld->GetTVVertSelectionPtr(),
						mr_reftarg, node
						);
				}
				else if (fnGetTVSubMode() == TVEDGEMODE)
				{
					functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.selectEdgesByNode"), node->GetName());
					macroRecorder->FunctionCall(functionCall, 2, 0,
						mr_bitarray, ld->GetTVEdgeSelectionPtr(),
						mr_reftarg, node
						);
				}
				else if (fnGetTVSubMode() == TVFACEMODE)
				{
					functionCall.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode"), node->GetName());
					macroRecorder->FunctionCall(functionCall, 2, 0,
						mr_bitarray, ld->GetFaceSelectionPtr(),
						mr_reftarg, node
						);
				}

				macroRecorder->EmitScript();

			}
		}
	}

	SyncSelect();

	if (fnGetTVSubMode() == TVEDGEMODE)
	{
		// In UV edge selection, the auto-pack should be forcely disabled as design.
		AutoPackDisabledGuard guard(this);
		AutoEdgeToSeamAndLSCMResolve();
	}
}

void UnwrapMod::ClearSelect()
{
	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		return;

	if ((fnGetMapMode() == PELTMAP) && (peltData.peltDialog.hWnd == NULL))
		return;

	HoldSelection();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->ClearSelection(fnGetTVSubMode());
	}
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

}

void UnwrapMod::PackFull(bool isAutoPack, bool useSel)
{
	BOOL normalize = packNormalize;
	BOOL rotate = packRotate;
	BOOL fill = packFillHoles;
	float padding = packSpacing;
	mPackTempPadding = GetUIManager()->GetSpinFValue(ID_PACK_PADDINGSPINNER);
	BOOL rescale = packRescaleCluster;
	int localMethod = packMethod;

	packNormalize = TRUE;
	packRotate = mPackTempRotate;
	packFillHoles = TRUE;
	packSpacing = mPackTempPadding;
	packRescaleCluster = mPackTempRescale;
	if (isAutoPack)
		packMethod = (packMethod >= unwrap_non_convex_pack) ? unwrap_recursive_pack : packMethod;// don't use Non-Convex way, since it is a little slow for live packing.
	if (GetAutoPackEnabled())
	{
		Pack(packMethod, packSpacing, packNormalize, packRotate, packFillHoles, TRUE, useSel);
	}
	else
	{
		fnPackNoParams();
	}

	packNormalize = normalize;
	packRotate = rotate;
	packFillHoles = fill;
	packSpacing = padding;
	packRescaleCluster = rescale;
	packMethod = localMethod;
}

class MeshTopoDataHitEdgesInfo
{
public:
	INode* mpNode;
	MeshTopoData* mpLD;
	std::vector<int> mArrayHitEdgeIndex;
	MeshTopoDataHitEdgesInfo() : mpNode(nullptr), mpLD(nullptr)
	{
	}

	~MeshTopoDataHitEdgesInfo()
	{
		mArrayHitEdgeIndex.clear();
	}
};

class VectorFinder
{
public:
	typedef std::vector<std::unique_ptr<MeshTopoDataHitEdgesInfo>> VectorHitEdgesInfoPtr;
	VectorFinder(INode* pNode)
	{
		mpNode = pNode;
	}
	bool operator ()(const VectorHitEdgesInfoPtr::value_type& value)
	{
		return value->mpNode == mpNode;
	}
private:
	INode* mpNode;
};

static void UpdateSeamEdge(const BitArray& geOpenEdges, MeshTopoData* pLD, int i)
{
	if (pLD && i >= 0 && i < geOpenEdges.GetSize() && geOpenEdges[i])
	{
		if (!(pLD->GetSewingPending()))
		{
			pLD->SetSewingPending();
		}
		pLD->mSeamEdges.Set(i, TRUE);
	}
}

void UnwrapMod::EraseOpenEdgesByDeSelection(HitRecord *hitRec)
{
	// Erase open edges can only be used in peel mode condition
	if (fnGetMapMode() != LSCMMAP || hitRec == nullptr || fnGetTVSubMode() != TVEDGEMODE) return;
	HoldSuspendedOffGuard guard(theHold.IsSuspended() ? true : false);

	theHold.SuperBegin(); // Here we need to combine erase and Pack restore as one.
	theHold.Begin();

	// we need to collect the hit edges for each meshTopoData and each node in one loop
	// The hit edges map would be set up: one node-->one meshtopodata-->multiple hit edges in region de-selction.
	// Note: in regeion selection, multiple nodes would be de-selected.
	std::vector<std::unique_ptr<MeshTopoDataHitEdgesInfo>> arrayHitNodes;
	HitRecord *hitRecordForNodes = hitRec;
	while (hitRecordForNodes)
	{
		INode *hitNode = hitRecordForNodes->nodeRef;
		int hitEdgeIndex = hitRecordForNodes->hitInfo;
		bool bEdgeInArray = false;
		for (auto& it : arrayHitNodes)
		{
			if (hitNode == it->mpNode)
			{
				it->mArrayHitEdgeIndex.push_back(hitEdgeIndex);
				bEdgeInArray = true;
				break;
			}
		}

		if (bEdgeInArray)
		{
			hitRecordForNodes = hitRecordForNodes->Next();
			continue;
		}

		std::unique_ptr<MeshTopoDataHitEdgesInfo> ptrHitNode(new MeshTopoDataHitEdgesInfo());
		ptrHitNode->mpNode = hitNode;
		ptrHitNode->mArrayHitEdgeIndex.push_back(hitEdgeIndex);
		arrayHitNodes.push_back(std::move(ptrHitNode));
		hitRecordForNodes = hitRecordForNodes->Next();
	}

	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		INode *hitNode = GetMeshTopoDataNode(ldID);
		MeshTopoData *hitLD = GetMeshTopoData(ldID);
		auto findResult = std::find_if(arrayHitNodes.begin(), arrayHitNodes.end(), VectorFinder(hitNode));
		if (findResult != arrayHitNodes.end())
		{
			(*findResult)->mpLD = hitLD;
		}
	}

	if (arrayHitNodes.empty())
	{
		if (theHold.Holding())
		{
			theHold.Cancel();
			theHold.SuperCancel();
		}
		return;
	}


	for (auto& it : arrayHitNodes)
	{
		if (theHold.Holding())
		{
			theHold.Put(new UnwrapPeltSeamRestore(this, it->mpLD));
		}
	}

	HoldPointsAndFaces();

	theHold.Accept(_T("Erase"));

	// clear the seam edges.
	for (auto& it : arrayHitNodes)
	{
		if (it->mpLD->mSeamEdges.GetSize() != it->mpLD->GetNumberGeomEdges())
		{
			it->mpLD->mSeamEdges.SetSize(it->mpLD->GetNumberGeomEdges());
		}
		it->mpLD->mSeamEdges.ClearAll();
	}

	for (auto& it : arrayHitNodes)
	{
		BitArray geOpenEdges = it->mpLD->GetOpenGeomEdges();
		// find the hit edges and judge whether they are open edges
		for (int i : it->mArrayHitEdgeIndex)
		{
			UpdateSeamEdge(geOpenEdges, it->mpLD, i);
			int mirrorIndex = fnGetMirrorSelectionStatus() ? it->mpLD->GetGeomEdgeMirrorIndex(i) : -1;
			UpdateSeamEdge(geOpenEdges, it->mpLD, mirrorIndex);
		}

		if (it->mpLD->GetSewingPending())
		{
			fnLSCMInvalidateTopo(it->mpLD);
		}
	}

	LSCMForceResolve();
	theHold.SuperAccept(GetString(IDS_ERASE_OPEN_EDGES)); // Here we need to combine erase and Pack restore as one.
	InvalidateView();

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}

void UnwrapMod::AutoEdgeToSeamAndLSCMResolve()
{
	if (fnGetTVSubMode() != TVEDGEMODE) return;

	if (GetLivePeelModeEnabled() && !fnGetTVElementMode())
	{
		HoldSuspendedOffGuard guard(theHold.IsSuspended() ? true : false);
		theHold.SuperBegin(); // Here we need to combine LSCM and Pack restore as one.
		fnPeltSelToSeams(FALSE);
		theHold.SuperAccept(GetString(IDS_LIVE_PEEL));
	}
}

void UnwrapMod::HoldPoints()
{
	mMeshTopoData.HoldPoints();
}

void UnwrapMod::HoldSelection()
{
	mMeshTopoData.HoldSelection();
}

void UnwrapMod::HoldPointsAndFaces()
{
	mMeshTopoData.HoldPointsAndFaces();
}

void UnwrapMod::TypeInChanged(int which)
{
	theHold.Restore();

	TimeValue t = GetCOREInterface()->GetTime();
	HoldPoints();

	Point3 uvw(0.0f, 0.0f, 0.0f);
	uvw[0] = mUIManager.GetSpinFValue(ID_SPINNERU);
	uvw[1] = mUIManager.GetSpinFValue(ID_SPINNERV);
	uvw[2] = mUIManager.GetSpinFValue(ID_SPINNERW);

	TransferSelectionStart();

	//check if only one vertex selected or more are selected
	Point3 uv(0, 0, 0);
	BOOL found = FALSE;
	BOOL bSingleVertex = TRUE;

	Box3 bounds;
	bounds.Init();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld->GetTVVertSel().AnyBitSet())
		{
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (!ld->GetTVVertSelected(i)) continue;

				if (found)
				{
					Point3 p = ld->GetTVVert(i);
					if (uv.x != p.x ||
						uv.y != p.y ||
						uv.z != p.z)
					{
						bSingleVertex = FALSE;
						bounds += p;
					}
				}
				else
				{
					uv = ld->GetTVVert(i);
					found = TRUE;
					bounds += uv;
				}
			}
		}
	}

	if(mode == ID_MOVE || (mode == ID_FREEFORMMODE && (freeFormSubMode == ID_MOVE || freeFormSubMode == ID_SELECT) ))
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];

			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (absoluteTypeIn)
				{
					if (ld->GetTVVertSelected(i))
					{
						Point3 p = ld->GetTVVert(i);
							if(bSingleVertex)
							{						
						p[which] = uvw[which];
							}
							else
							{
								p[which] += (uvw[which] - bounds.Center()[which]);
							}
						ld->SetTVVert(t, i, p);
					}
					else if (ld->GetTVVertInfluence(i) != 0.0f)
					{
						float infl = ld->GetTVVertInfluence(i);
						Point3 p = ld->GetTVVert(i);
							if(bSingleVertex)
							{
						p[which] += (uvw[which] - p[which])*infl;
							}
							else
							{
								p[which] += (uvw[which] - bounds.Center()[which] - p[which])*infl;
							}
						ld->SetTVVert(t, i, p);
					}
				}
				else
				{
					if (ld->GetTVVertSelected(i))
					{
						Point3 p = ld->GetTVVert(i);
						p[which] += uvw[which];
						ld->SetTVVert(t, i, p);
					}
					else if (ld->GetTVVertInfluence(i) != 0.0f)
					{
						float infl = ld->GetTVVertInfluence(i);
						Point3 p = ld->GetTVVert(i);
						p[which] += uvw[which] * infl;;
						ld->SetTVVert(t, i, p);
					}
				}

			}
		}
		tempAmount = uvw[which];
	}
	else if(mode == ID_SCALE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_SCALE))
	{
		if (centeron)
		{
			int dir = 0;
			ScalePoints(hView, uvw[0]/100.0, dir);
		}
		else
		{
			center.x = tempCenter.x;
			center.y = tempCenter.y;

			float fXScale = uvw[0]/100.0;
			float fYScale = uvw[1]/100.0;

			if(mTypeInLinkUV)
			{
				if(which == 0)
				{
					fYScale = fXScale;
				}
				else if(which == 1)
				{
					fXScale = fYScale;
				}
			}

			ScalePointsXY(hView, fXScale, fYScale);
			RecomputePivotOffset();
		}
		tempAmount = uvw[0];
		tempDir = 0;
	}
	else if(mode == ID_ROTATE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_ROTATE))
	{
		center.x = tempCenter.x;
		center.y = tempCenter.y;

		float fAngleRadians = uvw[0]*PI/180.0;
		RotatePoints(hView, fAngleRadians,false);
		RecomputePivotOffset();
	}

	TransferSelectionEnd(FALSE, FALSE);

	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::ChannelChanged(int which, float x)
{
	TimeValue t = GetCOREInterface()->GetTime();
	theHold.Begin();
	HoldPoints();
	Point3 uvw;
	uvw[0] = 0.0f;
	uvw[1] = 0.0f;
	uvw[2] = 0.0f;

	uvw[which] = x;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];


		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) {
		{
			if (ld->GetTVVertSelected(i))//vsel[i]) 
			{
				Point3 p = ld->GetTVVert(i);
				p[which] = uvw[which];
				ld->SetTVVert(t, i, p);
			}
			else if (ld->GetTVVertInfluence(i) != 0.0f)
			{
				float infl = ld->GetTVVertInfluence(i);
				Point3 p = ld->GetTVVert(i);
				p[which] += (uvw[which] - p[which])*infl;
				ld->SetTVVert(t, i, p);
			}
		}
	}

	theHold.Accept(GetString(IDS_PW_MOVE_UVW));

	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::SetVertexPosition(MeshTopoData *ld, TimeValue t, int which, Point3 pos, BOOL hold, BOOL update)
{
	if (hold)
	{
		theHold.Begin();
		HoldPoints();
	}

	if (ld)
	{
		if ((which >= 0) && (which < ld->GetNumberTVVerts()))
		{
			ld->SetTVVert(t, which, pos);
		}

	}

	if (hold)
	{
		theHold.Accept(GetString(IDS_PW_MOVE_UVW));
	}

	if (update)
	{
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		InvalidateView();
		if (ip) ip->RedrawViews(ip->GetTime());
	}
}


void UnwrapMod::DeleteSelected()
{

	/*
	theHold.Begin();

	HoldPointsAndFaces();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{

	MeshTopoData *ld = mMeshTopoData[ldID];

	for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++)
	{
	if ( ld->GetTVVertSelected(i) && (!ld->GetTVVertDead(i)))//(vsel[i]) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
	{
	ld->SetTVVertDead(i,TRUE);//TVMaps.v[i].flags |= FLAG_DEAD;
	ld->SetTVVertSelected(i,FALSE);//vsel.Set(i,FALSE);
	}
	}

	for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count(); i++)
	{
	if (!ld->GetFaceDead(i))//(!(TVMaps.f[i]->flags & FLAG_DEAD))
	{
	for (int j=0; j<3; j++)
	{
	int index = ld->GetFaceTVVert(i,j);//TVMaps.f[i]->t[j];
	if ((index < 0) || (index >= ld->GetNumberTVVerts()))//TVMaps.v.Count()))
	{
	DbgAssert(1);
	}
	else if (ld->GetTVVertDead(index))//TVMaps.v[index].flags & FLAG_DEAD)
	{
	ld->SetFaceDead(i,TRUE);//TVMaps.f[i]->flags |= FLAG_DEAD;
	}
	}
	}
	}
	}
	// loop through faces all
	theHold.Accept(GetString(IDS_PW_DELETE_SELECTED));
	*/
}

void UnwrapMod::HideSelected()
{
	theHold.Begin();
	HoldPoints();
	theHold.Accept(GetString(IDS_PW_HIDE_SELECTED));

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld->GetTVVertSel().AnyBitSet())
		{
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
			{
				if ((ld->GetTVVertSelected(i) && (!ld->GetTVVertDead(i))))
				{

					ld->SetTVVertHidden(i, TRUE);//TVMaps.v[i].flags |= FLAG_HIDDEN;
					ld->SetTVVertSelected(i, FALSE);//	vsel.Set(i,FALSE);
				}
			}
		}
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE, FALSE);
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (fnGetTVSubMode() == TVVERTMODE)
			ld->ClearTVVertSelection();//vsel.ClearAll();
		else if (fnGetTVSubMode() == TVEDGEMODE)
			ld->ClearTVEdgeSelection();//esel.ClearAll();
		if (fnGetTVSubMode() == TVFACEMODE)
			ld->ClearFaceSelection();//fsel.ClearAll();
	}
}

void UnwrapMod::UnHideAll()
{
	theHold.Begin();
	HoldPoints();
	theHold.Accept(GetString(IDS_PW_UNHIDEALL));

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
		{
			if ((ld->GetTVVertHidden(i) && (!ld->GetTVVertDead(i))))//if ( (TVMaps.v[i].flags & FLAG_HIDDEN) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
			{
				ld->SetTVVertHidden(i, FALSE);//TVMaps.v[i].flags -= FLAG_HIDDEN;
			}
		}
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE, FALSE);


}


void UnwrapMod::FreezeSelected()
{

	theHold.Begin();
	HoldPoints();
	theHold.Accept(GetString(IDS_PW_FREEZE_SELECTED));

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld->GetTVVertSel().AnyBitSet())
		{
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
			{
				if (ld->GetTVVertSelected(i) && (!ld->GetTVVertDead(i)))
				{
					ld->SetTVVertFrozen(i, TRUE);//TVMaps.v[i].flags |= FLAG_FROZEN;
					ld->SetTVVertSelected(i, FALSE);//vsel.Set(i,FALSE);
				}
			}
		}
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE, FALSE);

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (fnGetTVSubMode() == TVVERTMODE)
			ld->ClearTVVertSelection();//vsel.ClearAll();
		else if (fnGetTVSubMode() == TVEDGEMODE)
			ld->ClearTVEdgeSelection();//esel.ClearAll();
		if (fnGetTVSubMode() == TVFACEMODE)
			ld->ClearFaceSelection();//fsel.ClearAll();

		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
		{
			if (ld->GetTVVertSelected(i) && ld->GetTVVertFrozen(i))//( (vsel[i]) && (TVMaps.v[i].flags & FLAG_FROZEN) )
			{
				ld->SetTVVertSelected(i, TRUE);//	vsel.Set(i,FALSE);
			}
		}

		for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count(); i++)
		{
			int deg = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
			BOOL frozen = FALSE;
			for (int j = 0; j < deg; j++)
			{
				int index = ld->GetFaceTVVert(i, j);//TVMaps.f[i]->t[j];
				if (ld->GetTVVertFrozen(index))//(TVMaps.v[index].flags & FLAG_FROZEN)
					frozen = TRUE;
			}
			if (frozen)
			{
				ld->SetFaceSelected(i, FALSE);//fsel.Set(i,FALSE);
			}
		}

		for (int i = 0; i < ld->GetNumberTVEdges(); i++)//TVMaps.ePtrList.Count(); i++)
		{
			int a = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
			int b = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
			if (ld->GetTVVertFrozen(a) || ld->GetTVVertFrozen(b))
				//				(TVMaps.v[a].flags & FLAG_FROZEN) ||
					//				(TVMaps.v[b].flags & FLAG_FROZEN) )
			{
				ld->SetTVEdgeSelected(i, FALSE);//esel.Set(i,FALSE);
			}
		}

	}
}

void UnwrapMod::UnFreezeAll()
{

	theHold.Begin();
	HoldPoints();
	theHold.Accept(GetString(IDS_PW_UNFREEZEALL));

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
		{
			if (!ld->GetTVVertDead(i))//(TVMaps.v[i].flags & FLAG_DEAD)) 
			{
				if (ld->GetTVVertFrozen(i))//(TVMaps.v[i].flags & FLAG_FROZEN)) 
					ld->SetTVVertFrozen(i, FALSE);//TVMaps.v[i].flags -= FLAG_FROZEN;
			}
		}
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE, FALSE);

}

void UnwrapMod::WeldSelected(BOOL hold, BOOL notify)
{

	if (hold)
	{
		theHold.Begin();
		HoldPointsAndFaces();
	}

	//convert our sub selection type to vertex selection
	TransferSelectionStart();
	BOOL weldOnlyShared = FALSE;
	pblock->GetValue(unwrap_weldonlyshared, 0, weldOnlyShared, FOREVER);
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->WeldSelectedVerts(weldThreshold, weldOnlyShared);
	}

	if (hold)
	{
		theHold.Accept(GetString(IDS_PW_WELDSELECTED));
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE, TRUE);

	if (notify)
	{
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());
		InvalidateView();
	}
}

BOOL UnwrapMod::WeldPoints(HWND h, IPoint2 m)
{

	BOOL holdNeeded = FALSE;


	Point3 p(0.0f, 0.0f, 0.0f);
	Point2 mp;
	mp.x = (float)m.x;
	mp.y = (float)m.y;

	float xzoom, yzoom;
	int width, height;
	ComputeZooms(h, xzoom, yzoom, width, height);



	Rect rect;
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		rect.left = m.x - 2;
		rect.right = m.x + 2;
		rect.top = m.y - 2;
		rect.bottom = m.y + 2;
	}
	else
	{
		rect.left = m.x - 3;
		rect.right = m.x + 3;
		rect.top = m.y - 3;
		rect.bottom = m.y + 3;
	}

	mode = 0;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		Tab<TVHitData> hits;
		int index = -1;

		MeshTopoData *ld = mMeshTopoData[ldID];
		if (HitTest(rect, hits, FALSE))
		{
			MeshTopoData *selID = NULL;
			for (int i = 0; i < hits.Count(); i++)
			{
				MeshTopoData *testLD = mMeshTopoData[hits[i].mLocalDataID];
				int index = hits[i].mID;
				if (fnGetTVSubMode() == TVVERTMODE)
				{
					if (testLD->GetTVVertSelected(index) && (testLD == ld))
						selID = ld;

				}
				else if (fnGetTVSubMode() == TVEDGEMODE)
				{
					if (testLD->GetTVEdgeSelected(index) && (testLD == ld))
					{
						selID = ld;
					}
				}
			}

			for (int i = 0; i < hits.Count(); i++)
			{
				int hindex = hits[i].mID;
				MeshTopoData *hitLD = mMeshTopoData[hits[i].mLocalDataID];
				if (selID && (hitLD == selID))
				{
					if (fnGetTVSubMode() == TVVERTMODE)
					{

						if (!ld->GetTVVertSelected(hindex))//vsel[hits[i]]) 
						{
							index = hindex;//hits[i];
							i = hits.Count();
						}
					}
					else if (fnGetTVSubMode() == TVEDGEMODE)
					{
						if (ld == tWeldHitLD)
							index = tWeldHit;

					}

				}
			}
		}
		mode = ID_WELD;


		Tab<int> selected, hit;
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			selected.SetCount(1);
			hit.SetCount(1);
			hit[0] = index;
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)//vsel.GetSize(); i++)
			{
				if (ld->GetTVVertSelected(i))//vsel[i]) 
					selected[0] = i;
			}
		}

		else if ((fnGetTVSubMode() == TVEDGEMODE) && (index != -1))
		{
			selected.SetCount(2);
			hit.SetCount(2);
			hit[0] = ld->GetTVEdgeVert(index, 0);//TVMaps.ePtrList[index]->a;
			hit[1] = ld->GetTVEdgeVert(index, 1);//TVMaps.ePtrList[index]->b;

			int otherEdgeIndex = 0;

			for (int i = 0; i < ld->GetNumberTVEdges(); i++)//esel.GetSize(); i++)
			{
				if (ld->GetTVEdgeSelected(i))//esel[i]) 
				{
					otherEdgeIndex = i;
					selected[0] = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
					selected[1] = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
				}
			}
			int face1, face2;
			int firstVert1, firstVert2;
			int nextVert1, nextVert2;
			int prevVert1 = -1, prevVert2 = -1;
			face1 = ld->GetTVEdgeConnectedTVFace(index, 0);//TVMaps.ePtrList[index]->faceList[0];
			face2 = ld->GetTVEdgeConnectedTVFace(otherEdgeIndex, 0);//TVMaps.ePtrList[otherEdgeIndex]->faceList[0];
			firstVert1 = hit[0];
			firstVert2 = selected[0];
			//fix up points
			int pcount = 3;
			pcount = ld->GetFaceDegree(face1);//TVMaps.f[face1]->count;
			for (int i = 0; i < pcount; i++)
			{
				int tvfIndex = ld->GetFaceTVVert(face1, i);//TVMaps.f[face1]->t[i];
				if (tvfIndex == firstVert1)
				{
					if (i != (pcount - 1))
						nextVert1 = ld->GetFaceTVVert(face1, i + 1);//TVMaps.f[face1]->t[i+1];
					else nextVert1 = ld->GetFaceTVVert(face1, 0);//TVMaps.f[face1]->t[0];
					if (i != 0)
						prevVert1 = ld->GetFaceTVVert(face1, i - 1);//TVMaps.f[face1]->t[i-1];
					else prevVert1 = ld->GetFaceTVVert(face1, pcount - 1);//TVMaps.f[face1]->t[pcount-1];
				}
			}

			pcount = ld->GetFaceDegree(face2);//TVMaps.f[face2]->count;
			for (int i = 0; i < pcount; i++)
			{
				int tvfIndex = ld->GetFaceTVVert(face2, i);//TVMaps.f[face2]->t[i];
				if (tvfIndex == firstVert2)
				{
					if (i != (pcount - 1))
						nextVert2 = ld->GetFaceTVVert(face2, i + 1);//TVMaps.f[face2]->t[i+1];
					else nextVert2 = ld->GetFaceTVVert(face2, 0);//TVMaps.f[face2]->t[0];
					if (i != 0)
						prevVert2 = ld->GetFaceTVVert(face2, i - 1);//TVMaps.f[face2]->t[i-1];
					else prevVert2 = ld->GetFaceTVVert(face2, pcount - 1);//TVMaps.f[face2]->t[pcount-1];

				}
			}

			if (prevVert1 == hit[1])
			{
				int temp = hit[0];
				hit[0] = hit[1];
				hit[1] = temp;
			}
			if (prevVert2 == selected[1])
			{
				int temp = selected[0];
				selected[0] = selected[1];
				selected[1] = temp;
			}

			int tempSel = selected[0];
			selected[0] = selected[1];
			selected[1] = tempSel;


			if (hit[0] == selected[1])
			{
				hit.Delete(0, 1);
				selected.Delete(1, 1);
			}

			else if (hit[1] == selected[0])
			{
				hit.Delete(1, 1);
				selected.Delete(0, 1);
			}

		}


		BOOL weldOnlyShared = FALSE;
		pblock->GetValue(unwrap_weldonlyshared, 0, weldOnlyShared, FOREVER);

		for (int selIndex = 0; selIndex < selected.Count(); selIndex++)
		{
			int index = hit[selIndex];
			if (index != -1)
			{
				for (int i = 0; i < ld->GetNumberFaces(); i++)
				{
					int pcount = 3;
					pcount = ld->GetFaceDegree(i);

					for (int j = 0; j < pcount; j++)
					{
						int tvfIndex = ld->GetFaceTVVert(i, j);
						if (tvfIndex == selected[selIndex])
						{
							if (ld->OkToWeld(weldOnlyShared, index, tvfIndex))
							{
								ld->SetFaceTVVert(i, j, index);
								ld->DeleteTVVert(tvfIndex);
								holdNeeded = TRUE;
							}
						}
						if (ld->GetFaceHasVectors(i))
						{
							tvfIndex = ld->GetFaceTVHandle(i, j * 2);
							if (tvfIndex == selected[selIndex])
							{
								if (ld->OkToWeld(weldOnlyShared, index, tvfIndex))
								{
									ld->SetFaceTVHandle(i, j * 2, index);
									ld->DeleteTVVert(tvfIndex);
									holdNeeded = TRUE;
								}
							}

							tvfIndex = ld->GetFaceTVHandle(i, j * 2 + 1);
							if (tvfIndex == selected[selIndex])
							{
								if (ld->OkToWeld(weldOnlyShared, index, tvfIndex))
								{
									ld->SetFaceTVHandle(i, j * 2 + 1, index);
									ld->DeleteTVVert(tvfIndex);
									holdNeeded = TRUE;
								}
							}

							if (ld->GetFaceHasInteriors(i))
							{
								tvfIndex = tvfIndex = ld->GetFaceTVHandle(i, j);
								if (tvfIndex == selected[selIndex])
								{
									if (ld->OkToWeld(weldOnlyShared, index, tvfIndex))
									{
										ld->SetFaceTVInterior(i, j, index);
										ld->DeleteTVVert(tvfIndex);
										holdNeeded = TRUE;
									}
								}

							}
						}
					}
				}
			}
		}
		ld->SetTVEdgeInvalid();
	}


	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();


	return holdNeeded;
}

void UnwrapMod::BreakSelected()
{
	if (fnGetTVSubMode() == TVOBJECTMODE)
		return;
	if (fnGetTVSubMode() == TVFACEMODE)
	{
		DetachEdgeVerts();
	}
	else
	{
		theHold.Begin();
		HoldPointsAndFaces();
		theHold.Accept(GetString(IDS_PW_BREAK));

		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{

			MeshTopoData *ld = mMeshTopoData[ldID];

			if (fnGetTVSubMode() != TVEDGEMODE)
				TransferSelectionStart();
			if (fnGetTVSubMode() == TVEDGEMODE)
			{
				ld->BreakEdges();
			}
			else
			{
				ld->BreakVerts();
			}
			if (fnGetTVSubMode() != TVEDGEMODE)
				TransferSelectionEnd(FALSE, TRUE);

			//After break the selected edges, no edge is selected, so clear the flags of the  
			//share TV edges (display as blue in the 2d view)
			if (fnGetTVSubMode() == TVEDGEMODE)
			{
				ld->mSharedTVEdges.ClearAll();
			}
			else
			{
				UpdateShowSharedTVEdges(ld);
			}

			SetMatFilters();
			NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
			if (ip) ip->RedrawViews(ip->GetTime());
			InvalidateView();

		}

	}

}

void UnwrapMod::SnapPoint(Point3 &p)
{
	int i1 = 0;
	int i2 = 1;
	GetUVWIndices(i1, i2);
	float fx = 0.0;
	float fy = 0.0;
	double dx = 0.0;
	double dy = 0.0;
	//compute in pixel space
	//find closest whole pixel
	fx = (float)modf(((float)p[i1] * (float)(bitmapWidth)), &dx);
	fy = (float)modf(((float)p[i2] * (float)(bitmapHeight)), &dy);
	
	if(pixelCenterSnap && pixelCornerSnap)
	{
		dx += 0.5f;
		dy += 0.5f;

		if (fx > 0.5f) dx += 0.5f;
		if (fy > 0.5f) dy += 0.5f;
	}
	else if (!pixelCornerSnap && pixelCenterSnap)
	{
		dx += 0.5f;
		dy += 0.5f;
	}
	else if(!pixelCenterSnap && pixelCornerSnap)
	{
		if (fx > 0.5f) dx += 1.0f;
		if (fy > 0.5f) dy += 1.0f;
	}
	//put back in UVW space
	p[i1] = (float)dx / (float)(bitmapWidth);
	p[i2] = (float)dy / (float)(bitmapHeight);
}

Point3 UnwrapMod::SnapPoint(Point3 snapPoint, MeshTopoData *snapLD, int snapIndex)
{
	int i1 = 0;
	int i2 = 1;
	GetUVWIndices(i1, i2);
	suspendNotify = TRUE;
	Point3 p = snapPoint;

	//VSNAP
	if (snapToggle)
	{
		BOOL vSnap = 0, eSnap = 0, gSnap = 0;
		pblock->GetValue(unwrap_vertexsnap, 0, vSnap, FOREVER);
		pblock->GetValue(unwrap_edgesnap, 0, eSnap, FOREVER);
		pblock->GetValue(unwrap_gridsnap, 0, gSnap, FOREVER);

		BOOL snapped = FALSE;
		if (vSnap)
		{
			//convert to screen space
			//get our window width height
			float xzoom = 1.0;
			float yzoom = 1.0;
			int width = 1;
			int height = 1;
			ComputeZooms(hView, xzoom, yzoom, width, height);
			Point3 tvPoint = UVWToScreen(p, xzoom, yzoom, width, height);
			//look up that point in our list
			//get our pixel distance
			int snapStr = (int)(fnGetSnapStrength()*60.0f);
			int x = (int)tvPoint.x;
			int y = (int)tvPoint.y;
			int startX = 0;
			int startY = 0;
			int endX = 0;
			int endY = 0;

			int vertID = -1;
			MeshTopoData *hitLD = NULL;
			for (int i = 0; i < snapStr; i++)
			{

				startX = x - i;
				endX = x + i;

				startY = y - i;
				endY = y + i;

				if (startX < 0) startX = 0;
				if (startY < 0) startY = 0;

				if (endX < 0) endX = 0;
				if (endY < 0) endY = 0;

				if (startX >= width) startX = width - 1;
				if (startY >= height) startY = height - 1;

				if (endX >= width) endX = width - 1;
				if (endY >= height) endY = height - 1;
				for (int iy = startY; iy <= endY; iy++)
				{
					for (int ix = startX; ix <= endX; ix++)
					{
						int index = iy * width + ix;
						for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
						{
							MeshTopoData *ld = mMeshTopoData[ldID];
							int ID = ld->GetVertexSnapBuffer(index);//vertexSnapBuffer[index];
							if ((ID == snapIndex) && (ld == snapLD))
							{
							}
							else if ((ID != -1))
							{
								vertID = ID;
								hitLD = ld;
								ix = endX;
								iy = endY;
								i = snapStr;
							}
						}
					}
				}
			}

			if ((vertID != -1) && hitLD)
			{
				p = hitLD->GetTVVert(vertID);//TVMaps.v[vertID].p;
				snapped = TRUE;
			}
		}

		if (eSnap && (!snapped))
		{

			//convert to screen space
			//get our window width height
			float xzoom = 1.0;
			float yzoom = 1.0;
			int width = 1;
			int height = 1;
			ComputeZooms(hView, xzoom, yzoom, width, height);
			Point3 tvPoint = UVWToScreen(p, xzoom, yzoom, width, height);
			//look up that point in our list
			//get our pixel distance
			int snapStr = (int)(fnGetSnapStrength()*60.0f);
			int x = (int)tvPoint.x;
			int y = (int)tvPoint.y;
			int startX = 0;
			int startY = 0;
			int endX = 0;
			int endY = 0;

			int egdeID = -1;
			MeshTopoData *hitLD = NULL;
			for (int i = 0; i < snapStr; i++)
			{

				startX = x - i;
				endX = x + i;

				startY = y - i;
				endY = y + i;

				if (startX < 0) startX = 0;
				if (startY < 0) startY = 0;

				if (endX < 0) endX = 0;
				if (endY < 0) endY = 0;

				if (startX >= width) startX = width - 1;
				if (startY >= height) startY = height - 1;

				if (endX >= width) endX = width - 1;
				if (endY >= height) endY = height - 1;
				for (int iy = startY; iy <= endY; iy++)
				{
					for (int ix = startX; ix <= endX; ix++)
					{
						int index = iy * width + ix;
						for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
						{

							int ID = mMeshTopoData[ldID]->GetEdgeSnapBuffer(index);

							if (ID >= 0)
							{
								if ((ID == snapIndex) && (mMeshTopoData[ldID] == snapLD))
								{
									//hit self skip
								}
								else  if (!mMeshTopoData[ldID]->GetEdgesConnectedToSnapvert(ID))//edgesConnectedToSnapvert[ID])
								{
									egdeID = ID;
									hitLD = mMeshTopoData[ldID];
									ix = endX;
									iy = endY;
									i = snapStr;
								}
							}
						}
					}
				}
			}

			if ((egdeID >= 0) && hitLD)
			{
				int a = hitLD->GetTVEdgeVert(egdeID, 0);
				int b = hitLD->GetTVEdgeVert(egdeID, 1);

				Point3 pa = hitLD->GetTVVert(a);
				Point3 aPoint = UVWToScreen(pa, xzoom, yzoom, width, height);

				Point3 pb = hitLD->GetTVVert(b);
				Point3 bPoint = UVWToScreen(pb, xzoom, yzoom, width, height);

				float screenFLength = Length(aPoint - bPoint);
				float screenSLength = Length(tvPoint - aPoint);
				float per = screenSLength / screenFLength;


				Point3 vec = (pb - pa) * per;
				p = pa + vec;

				snapped = TRUE;
			}
		}

		if ((gSnap) && (!snapped))
		{
			float rem = fmod(p[i1], gridSize);
			float per = rem / gridSize;

			per = gridSize * fnGetSnapStrength();

			float snapPos = 0.0;
			if (p[i1] >= 0)
				snapPos = (int)((p[i1] + (gridSize*0.5f)) / gridSize) * gridSize;
			else snapPos = (int)((p[i1] - (gridSize*0.5f)) / gridSize) * gridSize;

			if (fabs(p[i1] - snapPos) < per)
				p[i1] = snapPos;

			if (p[i2] >= 0)
				snapPos = (int)((p[i2] + (gridSize*0.5f)) / gridSize) * gridSize;
			else snapPos = (int)((p[i2] - (gridSize*0.5f)) / gridSize) * gridSize;

			if (fabs(p[i2] - snapPos) < per)
				p[i2] = snapPos;


		}
	}

	if (IsPixelSnapOn())
	{
		SnapPoint(p);
	}

	return p;

}

void UnwrapMod::MovePaintPoints(Tab<TVHitData>& hits, Point2 pt, Tab<float>& hitsInflu)
{
	suspendNotify = TRUE;
	MeshTopoData* mtd = nullptr;
	int count = hits.Count();
	int index;
	int i1, i2;
	GetUVWIndices(i1, i2);
	TimeValue t = GetCOREInterface()->GetTime();
	for (int i = 0; i < count; ++i)
	{
		mtd = mMeshTopoData[hits[i].mLocalDataID];
		if (mtd)
		{
			index = hits[i].mID;
			if (hitsInflu[i] > 0.0f)
			{
				//check snap and bitmap
				Point3 p = mtd->GetTVVert(index);
				Point3 NewPoint = p;
				NewPoint[i1] += pt.x;
				NewPoint[i2] += pt.y;
				Point3 vec;
				vec = (NewPoint - p) * hitsInflu[i];
				p += vec;
				if(IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				mtd->SetTVVert(t, index, p);
				// indicate the soft selection color
				mtd->SetTVVertInfluence(index, hitsInflu[i]);
			}
		}
	}

	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
}

void UnwrapMod::MovePoints(Point2 pt)
{
	int i1 = 0;
	int i2 = 1;
	GetUVWIndices(i1, i2);
	HoldPoints();
	TimeValue t = GetCOREInterface()->GetTime();


	suspendNotify = TRUE;
	//VSNAP

	if (snapToggle)
	{
		BOOL vSnap = 0, eSnap = 0, gSnap = 0;
		pblock->GetValue(unwrap_vertexsnap, 0, vSnap, FOREVER);
		pblock->GetValue(unwrap_edgesnap, 0, eSnap, FOREVER);
		pblock->GetValue(unwrap_gridsnap, 0, gSnap, FOREVER);

		BOOL snapped = FALSE;

		if (vSnap && (mMouseHitVert != -1) && mMouseHitLocalData)
		{
			Point3 p = mMouseHitLocalData->GetTVVert(mMouseHitVert);

			p[i1] += pt.x;
			p[i2] += pt.y;

			//convert to screen space
			//get our window width height
			float xzoom = 1.0;
			float yzoom = 1.0;
			int width = 1;
			int height = 1;
			ComputeZooms(hView, xzoom, yzoom, width, height);
			Point3 tvPoint = UVWToScreen(p, xzoom, yzoom, width, height);
			//look up that point in our list
			//get our pixel distance
			int snapStr = (int)(fnGetSnapStrength()*60.0f);
			int x = (int)tvPoint.x;
			int y = (int)tvPoint.y;
			int startX = 0;
			int startY = 0;
			int endX = 0;
			int endY = 0;

			int vertID = -1;
			MeshTopoData *snapLD = NULL;
			for (int i = 0; i < snapStr; i++)
			{
				startX = x - i;
				endX = x + i;

				startY = y - i;
				endY = y + i;

				if (startX < 0) startX = 0;
				if (startY < 0) startY = 0;

				if (endX < 0) endX = 0;
				if (endY < 0) endY = 0;

				if (startX >= width) startX = width - 1;
				if (startY >= height) startY = height - 1;

				if (endX >= width) endX = width - 1;
				if (endY >= height) endY = height - 1;
				for (int iy = startY; iy <= endY; iy++)
				{
					for (int ix = startX; ix <= endX; ix++)
					{
						int index = iy * width + ix;
						for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
						{
							MeshTopoData *ld = mMeshTopoData[ldID];
							int ID = ld->GetVertexSnapBuffer(index);
							if ((ID != -1))
							{
								if ((ld == mMouseHitLocalData) && (ID == mMouseHitVert))
								{
									//skip, dont want to snap to self
								}
								else
								{
									snapLD = ld;
									vertID = ID;
									ix = endX;
									iy = endY;
									i = snapStr;
								}
							}
						}
					}
				}
			}

			if ((vertID != -1) && snapLD)
			{
				Point3 p = snapLD->GetTVVert(vertID);
				pt.x = p[i1] - mMouseHitLocalData->GetTVVert(mMouseHitVert)[i1];
				pt.y = p[i2] - mMouseHitLocalData->GetTVVert(mMouseHitVert)[i2];
				snapped = TRUE;
			}
		}

		if (eSnap && (!snapped) && (mMouseHitVert != -1) && mMouseHitLocalData)
		{
			Point3 p = mMouseHitLocalData->GetTVVert(mMouseHitVert);

			p[i1] += pt.x;
			p[i2] += pt.y;

			//convert to screen space
			//get our window width height
			float xzoom = 1.0;
			float yzoom = 1.0;
			int width = 1;
			int height = 1;
			ComputeZooms(hView, xzoom, yzoom, width, height);
			Point3 tvPoint = UVWToScreen(p, xzoom, yzoom, width, height);
			//look up that point in our list
			//get our pixel distance
			int snapStr = (int)(fnGetSnapStrength()*60.0f);
			int x = (int)tvPoint.x;
			int y = (int)tvPoint.y;
			int startX = 0;
			int startY = 0;
			int endX = 0;
			int endY = 0;

			int egdeID = -1;
			MeshTopoData *snapLD = NULL;
			for (int i = 0; i < snapStr; i++)
			{
				startX = x - i;
				endX = x + i;

				startY = y - i;
				endY = y + i;

				if (startX < 0) startX = 0;
				if (startY < 0) startY = 0;

				if (endX < 0) endX = 0;
				if (endY < 0) endY = 0;

				if (startX >= width) startX = width - 1;
				if (startY >= height) startY = height - 1;

				if (endX >= width) endX = width - 1;
				if (endY >= height) endY = height - 1;
				for (int iy = startY; iy <= endY; iy++)
				{
					for (int ix = startX; ix <= endX; ix++)
					{
						int index = iy * width + ix;
						for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
						{

							int ID = mMeshTopoData[ldID]->GetEdgeSnapBuffer(index);

							if (ID >= 0)
							{
								if ((ID == mMouseHitVert) && (mMeshTopoData[ldID] == mMouseHitLocalData))
								{
									//hit self skip
								}
								else if (!mMeshTopoData[ldID]->GetEdgesConnectedToSnapvert(ID))
								{
									snapLD = mMeshTopoData[ldID];
									egdeID = ID;
									ix = endX;
									iy = endY;
									i = snapStr;
								}
							}
						}
					}
				}
			}

			if (egdeID >= 0)
			{
				Point3 p(0.0f, 0.0f, 0.0f);

				int a = snapLD->GetTVEdgeVert(egdeID, 0);
				int b = snapLD->GetTVEdgeVert(egdeID, 1);

				Point3 pa = snapLD->GetTVVert(a);
				Point3 aPoint = UVWToScreen(pa, xzoom, yzoom, width, height);

				Point3 pb = snapLD->GetTVVert(b);
				Point3 bPoint = UVWToScreen(pb, xzoom, yzoom, width, height);

				float screenFLength = Length(aPoint - bPoint);
				float screenSLength = Length(tvPoint - aPoint);
				float per = screenSLength / screenFLength;


				Point3 vec = (pb - pa) * per;
				p = pa + vec;


				pt.x = p[i1] - mMouseHitLocalData->GetTVVert(mMouseHitVert)[i1];
				pt.y = p[i2] - mMouseHitLocalData->GetTVVert(mMouseHitVert)[i2];
				snapped = TRUE;
			}
		}

		if ((gSnap) && (mMouseHitVert != -1) && (!snapped) && mMouseHitLocalData)
		{

			Point3 p = mMouseHitLocalData->GetTVVert(mMouseHitVert);

			p[i1] += pt.x;
			p[i2] += pt.y;

			float rem = fmod(p[i1], gridSize);
			float per = rem / gridSize;

			per = gridSize * fnGetSnapStrength();

			float snapPos = 0.0;
			if (p[i1] >= 0)
				snapPos = (int)((p[i1] + (gridSize*0.5f)) / gridSize) * gridSize;
			else snapPos = (int)((p[i1] - (gridSize*0.5f)) / gridSize) * gridSize;

			if (fabs(p[i1] - snapPos) < per)
				p[i1] = snapPos;

			if (p[i2] >= 0)
				snapPos = (int)((p[i2] + (gridSize*0.5f)) / gridSize) * gridSize;
			else snapPos = (int)((p[i2] - (gridSize*0.5f)) / gridSize) * gridSize;

			if (fabs(p[i2] - snapPos) < per)
				p[i2] = snapPos;

			pt.x = p[i1] - mMouseHitLocalData->GetTVVert(mMouseHitVert)[i1];
			pt.y = p[i2] - mMouseHitLocalData->GetTVVert(mMouseHitVert)[i2];
		}
	}

	bool resolve = false;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld == NULL)
		{
			DbgAssert(0);
			continue;
		}

		for (int i = 0; i < ld->GetNumberTVVerts(); i++) 
		{
			if (ld->GetTVVertSelected(i))
			{
				//check snap and bitmap
				Point3 p = ld->GetTVVert(i);
				p[i1] += pt.x;
				p[i2] += pt.y;
				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				if ((fnGetMapMode() == LSCMMAP) && (pblock->GetInt(unwrap_autopin)) && (fnGetTVSubMode() != TVFACEMODE))
					ld->TVVertPin(i);

				ld->SetTVVert(t, i, p);

				resolve = true;
			}
			else if ((ld->GetTVVertInfluence(i) != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
			{
				//check snap and bitmap
				Point3 p = ld->GetTVVert(i);
				Point3 NewPoint = p;
				NewPoint[i1] += pt.x;
				NewPoint[i2] += pt.y;
				Point3 vec = (NewPoint - p) * ld->GetTVVertInfluence(i);
				p += vec;
				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				ld->SetTVVert(t, i, p);
			}
		}
	}
	if (resolve && GetLivePeelModeEnabled())
	{
		AutoPackDisabledGuard guard(this);
		mLSCMTool.Solve(true);
	}

	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
}

void UnwrapMod::RotatePoints(HWND h, float ang,bool bAngleSnap)
{
	HoldPoints();
	TimeValue t = GetCOREInterface()->GetTime();

	Point3 cent(0, 0, 0);


	if (centeron)
	{
		float xzoom, yzoom;
		int width, height;

		ComputeZooms(h, xzoom, yzoom, width, height);


		int tx = (width - int(xzoom)) / 2;
		int ty = (height - int(yzoom)) / 2;
		int i1, i2;
		GetUVWIndices(i1, i2);

		cent[i1] = (center.x - tx - xscroll) / xzoom;
		cent[i2] = (center.y + ty - yscroll - height) / -yzoom;
	}
	else
	{

		int ct = 0;
		Box3 bbox;

		bbox.Init();

		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{

			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld == NULL)
			{
				DbgAssert(0);
				continue;
			}
			if (ld->GetTVVertSel().AnyBitSet())
			{
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if (ld->GetTVVertSelected(i)) {
						cent += ld->GetTVVert(i);
						bbox += ld->GetTVVert(i);
						ct++;
					}
				}
			}
		}

		if (!ct) return;
		cent /= float(ct);
		cent = bbox.Center();
	}

	axisCenter.x = cent.x;
	axisCenter.y = cent.y;
	axisCenter.z = 0.0f;
	Matrix3 mat(1);
	if(bAngleSnap)
	{
		ang = GetCOREInterface()->SnapAngle(ang, FALSE);
	}	

	BOOL respectAspectRatio = rotationsRespectAspect;

	if (aspect == 1.0f)
		respectAspectRatio = FALSE;


	if (respectAspectRatio)
	{
		cent[uvw] *= aspect;
	}

	mat.Translate(-cent);

	currentRotationAngle = ang * 180.0f / PI;
	switch (uvw) {
	case 0: mat.RotateZ(ang); break;
	case 1: mat.RotateX(ang); break;
	case 2: mat.RotateY(ang); break;
	}
	mat.Translate(cent);

	suspendNotify = TRUE;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{

		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld == NULL)
		{
			DbgAssert(0);
			continue;
		}
		for (int i = 0; i < ld->GetNumberTVVerts(); i++) {
			if (ld->GetTVVertSelected(i))
			{
				//check snap and bitmap
				Point3 p = ld->GetTVVert(i);
				if (respectAspectRatio)
				{

					p[uvw] *= aspect;
					p = mat * p;
					p[uvw] /= aspect;

				}
				else
					p = mat * p;

				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				ld->SetTVVert(t, i, p);
			}
			else if ((ld->GetTVVertInfluence(i) != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
			{
				Point3 p = ld->GetTVVert(i);
				//check snap and bitmap
				Point3 NewPoint = ld->GetTVVert(i);
				NewPoint = mat * ld->GetTVVert(i);

				Point3 vec;
				vec = (NewPoint - p) * ld->GetTVVertInfluence(i);
				p += vec;

				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				ld->SetTVVert(t, i, p);
			}
		}
	}

	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);

	InvalidateView();

}

void UnwrapMod::RotateAroundAxis(HWND h, float ang, Point3 axis)
{
	HoldPoints();
	TimeValue t = GetCOREInterface()->GetTime();

	Point3 cent(0, 0, 0);

	cent = axis;
	Matrix3 mat(1);
	ang = GetCOREInterface()->SnapAngle(ang, FALSE);

	BOOL respectAspectRatio = rotationsRespectAspect;

	if (aspect == 1.0f)
		respectAspectRatio = FALSE;


	if (respectAspectRatio)
	{
		cent[uvw] *= aspect;
	}

	mat.Translate(-cent);

	currentRotationAngle = ang * 180.0f / PI;
	switch (uvw)
	{
	case 0: mat.RotateZ(ang); break;
	case 1: mat.RotateX(ang); break;
	case 2: mat.RotateY(ang); break;
	}
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
		{
			if (ld->GetTVVertSelected(i))//vsel[i]) 
			{
				Point3 p = ld->GetTVVert(i);
				//check snap and bitmap
				if (respectAspectRatio)
				{
					p[uvw] *= aspect;
					p = mat * p;
					p[uvw] /= aspect;
				}
				else p = mat * p;

				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				ld->SetTVVert(t, i, p);
			}
			else if ((ld->GetTVVertInfluence(i) != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
			{
				//check snap and bitmap
				Point3 NewPoint = ld->GetTVVert(i);//TVMaps.v[i].p;
				Point3 op = NewPoint;
				NewPoint = mat * NewPoint;

				Point3 vec;
				vec = (NewPoint - op) * ld->GetTVVertInfluence(i);//TVMaps.v[i].influence;
				op += vec;

				if (IsPixelSnapOn())
				{
					SnapPoint(op);
				}
				ld->SetTVVert(t, i, op);
			}

		}
	}
	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);

	InvalidateView();

}

void UnwrapMod::ScalePoints(HWND h, float scale, int direction)
{

	HoldPoints();
	TimeValue t = GetCOREInterface()->GetTime();

	Point3 cent(0, 0, 0);
	int i;

	int i1, i2;
	GetUVWIndices(i1, i2);

	if (centeron)
	{
		float xzoom, yzoom;
		int width, height;

		ComputeZooms(h, xzoom, yzoom, width, height);

		int tx = (width - int(xzoom)) / 2;
		int ty = (height - int(yzoom)) / 2;
		cent[i1] = (center.x - tx - xscroll) / xzoom;
		cent[i2] = (center.y + ty - yscroll - height) / -yzoom;
		cent.z = 0.0f;
	}
	else
	{
		int ct = 0;
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{

			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld == NULL)
			{
				DbgAssert(0);
				continue;
			}
			if (ld->GetTVVertSel().AnyBitSet())
			{
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if (ld->GetTVVertSelected(i) && !ld->GetTVVertDead(i))
					{
						cent += ld->GetTVVert(i);
						ct++;
					}
				}
			}
		}
		if (!ct) return;
		cent /= float(ct);
	}

	Matrix3 mat(1);
	mat.Translate(-cent);
	Point3 sc(1, 1, 1);

	if (direction == 0)
	{
		sc[i1] = scale;
		sc[i2] = scale;
	}
	else if (direction == 1)
	{
		sc[i1] = scale;
	}
	else if (direction == 2)
	{
		sc[i2] = scale;
	}


	axisCenter.x = cent.x;
	axisCenter.y = cent.y;
	axisCenter.z = 0.0f;

	mat.Scale(sc, TRUE);
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld == NULL)
		{
			DbgAssert(0);
			continue;
		}
		for (i = 0; i < ld->GetNumberTVVerts(); i++)
		{
			if (ld->GetTVVertSelected(i))
			{
				Point3 p = ld->GetTVVert(i);
				//check snap and bitmap
				p = mat * p;
				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				ld->SetTVVert(t, i, p);
			}
			else if ((ld->GetTVVertInfluence(i) != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
			{
				Point3 p = ld->GetTVVert(i);
				//check snap and bitmap
				Point3 NewPoint = ld->GetTVVert(i);
				NewPoint = mat * p;
				Point3 vec;
				vec = (NewPoint - p) * ld->GetTVVertInfluence(i);
				p += vec;

				if (IsPixelSnapOn())
				{
					SnapPoint(p);
				}
				ld->SetTVVert(t, i, p);
			}
		}
	}
	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();

}

void UnwrapMod::ScalePointsXY(HWND h, float scaleX, float scaleY)
{

	HoldPoints();
	TimeValue t = GetCOREInterface()->GetTime();

	Point3 cent(0, 0, 0);
	int i;

	float xzoom, yzoom;
	int width, height;

	ComputeZooms(h, xzoom, yzoom, width, height);

	int i1, i2;
	GetUVWIndices(i1, i2);

	int tx = (width - int(xzoom)) / 2;
	int ty = (height - int(yzoom)) / 2;
	cent[i1] = (center.x - tx - xscroll) / xzoom;
	cent[i2] = (center.y + ty - yscroll - height) / -yzoom;

	Matrix3 mat(1);

	axisCenter.x = cent.x;
	axisCenter.y = cent.y;
	axisCenter.z = 0.0f;

	mat.Translate(-cent);
	Point3 sc(1, 1, 1);
	sc[i1] = scaleX;
	sc[i2] = scaleY;

	mat.Scale(sc, TRUE);
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			for (i = 0; i < ld->GetNumberTVVerts(); i++) {
				if (ld->GetTVVertSelected(i)) {
					Point3 p = ld->GetTVVert(i);
					//check snap and bitmap
					p = mat * p;
					if (IsPixelSnapOn())
					{
						SnapPoint(p);
					}
					ld->SetTVVert(t, i, p);
				}
				else if (ld->GetTVVertInfluence(i) != 0.0f)
				{
					Point3 p = ld->GetTVVert(i);
					Point3 NewPoint = p;
					NewPoint = mat * p;
					Point3 vec;
					vec = (NewPoint - p) * ld->GetTVVertInfluence(i);
					p += vec;

					//check snap and bitmap
					if (IsPixelSnapOn())
					{
						SnapPoint(p);
					}
					ld->SetTVVert(t, i, p);
				}
			}
		}
	}
	suspendNotify = FALSE;

	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
}

void UnwrapMod::ScaleAroundAxis(HWND h, float scaleX, float scaleY, Point3 axis)
{

	HoldPoints();
	TimeValue t = GetCOREInterface()->GetTime();

	Point3 cent(0, 0, 0);
	int i;

	float xzoom, yzoom;
	int width, height;

	ComputeZooms(h, xzoom, yzoom, width, height);

	int i1, i2;
	GetUVWIndices(i1, i2);

	cent = axis;

	Matrix3 mat(1);
	mat.Translate(-cent);
	Point3 sc(1, 1, 1);
	sc[i1] = scaleX;
	sc[i2] = scaleY;

	mat.Scale(sc, TRUE);
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			for (i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (ld->GetTVVertSelected(i))//vsel[i]) 
				{
					//check snap and bitmap
					Point3 p = ld->GetTVVert(i);
					p = mat * p;
					if (IsPixelSnapOn())
					{
						SnapPoint(p);
					}

					ld->SetTVVert(t, i, p);
				}
				else if (ld->GetTVVertInfluence(i) != 0.0f)
				{
					//check snap and bitmap
					//			TVMaps.v[i].p = mat * TVMaps.v[i].p;
					Point3 NewPoint = ld->GetTVVert(i);//TVMaps.v[i].p;
					Point3 p = NewPoint;
					NewPoint = mat * p;
					Point3 vec;
					vec = (NewPoint - p) * ld->GetTVVertInfluence(i);//TVMaps.v[i].influence;
					p += vec;

					if (IsPixelSnapOn())
					{
						SnapPoint(p);
					}

					ld->SetTVVert(t, i, p);
				}
			}
		}
	}
	suspendNotify = FALSE;

	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();

}


void UnwrapMod::DetachEdgeVerts(BOOL hold)
{
	if (fnGetTVSubMode() == TVOBJECTMODE)
		return;

	if (hold)
	{
		theHold.Begin();
		HoldPointsAndFaces();
	}

	//convert our sub selection type to vertex selection
	if (fnGetTVSubMode() != TVFACEMODE)
		TransferSelectionStart();

	//we differeniate between faces and verts/edges since with edges we can just split along a seam
	//versus detaching a whole set of faces
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (fnGetTVSubMode() == TVFACEMODE)
		{
			//get our shared verts
			BitArray selVerts;
			BitArray unSelVerts;
			BitArray fsel = ld->GetFaceSel();

			selVerts.SetSize(ld->GetNumberTVVerts());
			unSelVerts.SetSize(ld->GetNumberTVVerts());
			selVerts.ClearAll();
			unSelVerts.ClearAll();
			//loop though our face selection get an array of vertices that the face selection uses
			//loop through our non selected verts get an array of those vertices
			for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count(); i++)
			{
				int deg = ld->GetFaceDegree(i);
				for (int j = 0; j < deg; j++)
				{
					int index = ld->GetFaceTVVert(i, j);
					if (fsel[i])
						selVerts.Set(index, TRUE);
					else unSelVerts.Set(index, TRUE);
					if (ld->GetFaceHasVectors(i))
					{
						int index = ld->GetFaceTVHandle(i, j * 2);
						if (index != -1)
						{
							if (fsel[i])
								selVerts.Set(index, TRUE);
							else unSelVerts.Set(index, TRUE);
						}
						index = ld->GetFaceTVHandle(i, j * 2 + 1);
						if (index != -1)
						{
							if (fsel[i])
								selVerts.Set(index, TRUE);
							else unSelVerts.Set(index, TRUE);
						}
						index = ld->GetFaceTVInterior(i, j);
						if (index != -1)
						{
							if (fsel[i])
								selVerts.Set(index, TRUE);
							else unSelVerts.Set(index, TRUE);
						}
					}

				}
			}

			//loop through for matching verts they are shared
			//create clone of those
			//store a look up
			Tab<int> oldToNewIndex;
			oldToNewIndex.SetCount(ld->GetNumberTVVerts());
			for (int i = 0; i < oldToNewIndex.Count(); i++)
			{
				oldToNewIndex[i] = -1;
				if (selVerts[i] && unSelVerts[i])
				{
					Point3 p = ld->GetTVVert(i);
					int newIndex = ld->AddTVVert(0, p);
					oldToNewIndex[i] = newIndex;
				}
			}

			//go back and fix and faces that use the look up vertices
			for (int i = 0; i < ld->GetNumberFaces(); i++)
			{
				if (fsel[i])
				{
					int deg = ld->GetFaceDegree(i);
					for (int j = 0; j < deg; j++)
					{
						int index = ld->GetFaceTVVert(i, j);
						int newIndex = oldToNewIndex[index];
						if (newIndex != -1)
							ld->SetFaceTVVert(i, j, newIndex);
						if (ld->GetFaceHasVectors(i))
						{
							int index = ld->GetFaceTVHandle(i, j * 2);
							if (index != -1)
							{
								newIndex = oldToNewIndex[index];
								if (newIndex != -1)
									ld->SetFaceTVHandle(i, j * 2, newIndex);
								//									TVMaps.f[i]->vecs->handles[j*2] = newIndex;
							}
							index = ld->GetFaceTVHandle(i, j * 2 + 1);//TVMaps.f[i]->vecs->handles[j*2+1];
							if (index != -1)
							{
								newIndex = oldToNewIndex[index];
								if (newIndex != -1)
									ld->SetFaceTVHandle(i, j * 2 + 1, newIndex);
								//									TVMaps.f[i]->vecs->handles[j*2+1] = newIndex;
							}
							index = ld->GetFaceTVInterior(i, j);//TVMaps.f[i]->vecs->interiors[j];
							if (index != -1)
							{
								newIndex = oldToNewIndex[index];
								if (newIndex != -1)
									ld->SetFaceTVInterior(i, j, newIndex);
								//									TVMaps.f[i]->vecs->interiors[j] = newIndex;
							}
						}
					}
				}
			}
		}
		else
		{
			//convert our selectoin to faces and then back to verts	
			//this will clean out any invalid vertices
			BitArray fsel = ld->GetFaceSel();
			BitArray vsel = ld->GetTVVertSel();
			BitArray holdFace(fsel);
			ld->GetFaceSelFromVert(fsel, FALSE);
			ld->SetFaceSelectionByRef(fsel);
			ld->GetVertSelFromFace(vsel);
			ld->SetTVVertSel(vsel);
			fsel = holdFace;

			//loop through verts 
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
			{
				//check if selected
				if (vsel[i])
				{
					//if selected loop through faces that have this vert
					BOOL first = TRUE;
					int newID = -1;
					Point3 p;

					first = TRUE;
					for (int j = 0; j < ld->GetNumberFaces(); j++)//TVMaps.f.Count(); j++) 
					{
						//if this vert is not selected  create a new vert and point it to it
						int ct = 0;
						int whichVert = -1;
						int degree = ld->GetFaceDegree(j);
						for (int k = 0; k < degree; k++)
						{
							int id;
							id = ld->GetFaceTVVert(j, k);//TVMaps.f[j]->t[k];
							if (vsel[id])
								ct++;
							if (id == i)
							{
								whichVert = k;
								p = ld->GetTVVert(id);//TVMaps.v[id].p;
							}
						}
						//this face contains the vert
						if (whichVert != -1)
						{
							//checkif all selected;
							if (ct != degree)//TVMaps.f[j]->count)
							{
								if (first)
								{
									first = FALSE;
									ld->AddTVVert(0, p, j, whichVert, FALSE);
									vsel = ld->GetTVVertSel();
									newID = ld->GetFaceTVVert(j, whichVert);//TVMaps.f[j]->t[whichVert];
								}
								else
								{
									ld->SetFaceTVVert(j, whichVert, newID);
									//TVMaps.f[j]->t[whichVert] = newID;
								}
							}
						}
					}
					//now do handles if need be
					first = TRUE;
					BOOL removeIsolatedFaces = FALSE;
					int isoVert = -1;
					for (int j = 0; j < ld->GetNumberFaces(); j++)//TVMaps.f.Count(); j++) 
					{
						if (ld->GetFaceHasVectors(j))//(TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[j]->vecs) )
						{
							int whichVert = -1;
							int ct = 0;
							int degree = ld->GetFaceDegree(j);
							for (int k = 0; k < degree * 2; k++)
							{
								int id;
								id = ld->GetFaceTVHandle(j, k);//TVMaps.f[j]->vecs->handles[k];
								if (vsel[id])
									ct++;
								if (id == i)
								{
									whichVert = k;
									p = ld->GetTVVert(id);//TVMaps.v[id].p;
								}
							}
							//this face contains the vert
							if (whichVert != -1)
							{
								//checkif all selected;
								//if owner selected break
								if (ct != degree * 2)
								{
									if (first)
									{
										first = FALSE;
										isoVert = ld->GetFaceTVHandle(j, whichVert);//TVMaps.f[j]->vecs->handles[whichVert];
										ld->AddTVHandle(0, p, j, whichVert, FALSE);
										vsel = ld->GetTVVertSel();
										newID = ld->GetFaceTVHandle(j, whichVert);//TVMaps.f[j]->vecs->handles[whichVert];
										removeIsolatedFaces = TRUE;
									}
									else
									{
										ld->SetFaceTVHandle(j, whichVert, newID);
										//										TVMaps.f[j]->vecs->handles[whichVert] = newID;
									}
								}
							}
						}
					}

					if (removeIsolatedFaces)
					{
						BOOL hit = FALSE;
						for (int j = 0; j < ld->GetNumberFaces(); j++)//TVMaps.f.Count(); j++) 
						{
							int degree = ld->GetFaceDegree(j);
							for (int k = 0; k < degree * 2; k++)
							{
								int id;
								id = ld->GetFaceTVHandle(j, k);//TVMaps.f[j]->vecs->handles[k];
								if (id == isoVert)
								{
									hit = TRUE;
								}
							}
						}
						if (!hit)
						{
							ld->DeleteTVVert(isoVert);
							//							TVMaps.v[isoVert].flags |= FLAG_DEAD;
							vsel.Set(isoVert, FALSE);
						}
					}
				}
			}
		}

		ld->BuildTVEdges();
		ld->BuildVertexClusterList();
	}

	//convert our sub selection type to vertex selection
	if (fnGetTVSubMode() != TVFACEMODE)
		TransferSelectionEnd(FALSE, TRUE);

	if (hold)
		theHold.Accept(GetString(IDS_PW_DETACH));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			UpdateShowSharedTVEdges(ld);
		}
	}

	SetMatFilters();
	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();
}

void UnwrapMod::FlipPoints(int direction)
{
	//loop through faces
	theHold.Begin();

	HoldPointsAndFaces();
	theHold.Accept(GetString(IDS_PW_FLIP));


	DetachEdgeVerts(FALSE);
	MirrorPoints(direction, FALSE);

	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();


}

void UnwrapMod::MirrorPoints(int direction, BOOL hold)
{

	TimeValue t = GetCOREInterface()->GetTime();


	if (hold)
	{
		theHold.SuperBegin();
		theHold.Begin();
		HoldPoints();
	}

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	int ct = 0;
	Box3 bounds;
	bounds.Init();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		int i;

		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray vsel = ld->GetTVVertSel();
		if (vsel.AnyBitSet())
		{
			for (i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
			{
				if (vsel[i])
				{
					bounds += ld->GetTVVert(i);//TVMaps.v[i].p;
					ct++;
				}
			}
		}
	}
	if (!ct)
	{
		if (hold)
		{
			theHold.Cancel();
			theHold.SuperCancel();
		}
		TransferSelectionEnd(FALSE, FALSE);
		return;
	}
	Point3 cent(0, 0, 0);
	cent = bounds.Center();

	Matrix3 mat(1);
	mat.Translate(-cent);
	Point3 sc(1.0f, 1.0f, 1.0f);
	int i1, i2;
	GetUVWIndices(i1, i2);
	if (direction == 0)
	{
		sc[i1] = -1.0f;
	}
	else if (direction == 1)
	{
		sc[i2] = -1.0f;
	}

	//	sc[i1] = scale;
	//	sc[i2] = scale;

	mat.Scale(sc, TRUE);
	mat.Translate(cent);

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		int i;

		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray vsel = ld->GetTVVertSel();
		if (vsel.AnyBitSet())
		{
			for (i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (vsel[i])
				{
					//check snap and bitmap
					Point3 p = mat * ld->GetTVVert(i);
					if (IsPixelSnapOn())
					{
						SnapPoint(p);
					}

					ld->SetTVVert(t, i, p);
				}
			}
		}
	}

	if (hold)
	{
		theHold.Accept(GetString(IDS_TH_MIRROR));
		theHold.SuperAccept(GetString(IDS_TH_MIRROR));
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE, FALSE);


	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();
}



int UnwrapMod::GetAxis()
{
	return 2;
}

void UnwrapMod::ZoomExtents()
{

	for (int k = 0; k < 2; k++)
	{
		Rect brect;
		Point2 pt;
		float xzoom, yzoom;
		int width, height;
		ComputeZooms(hView, xzoom, yzoom, width, height);
		brect.SetEmpty();

		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
			{
				if (!ld->GetTVVertDead(i))//!(TVMaps.v[i].flags & FLAG_DEAD))
				{
					if (ld->IsTVVertVisible(i))
					{
						pt = UVWToScreen(ld->GetTVVert(i)/*GetPoint(t,i)*/, xzoom, yzoom, width, height);
						IPoint2 ipt(int(pt.x), int(pt.y));
						brect += ipt;
					}
				}
			}
		}

		if (brect.IsEmpty()) return;

		if ((brect.w() == 1) || (brect.h() == 1))
		{
			pt = UVWToScreen(Point3(0.0f, 0.0f, 0.0f), xzoom, yzoom, width, height);
			IPoint2 ipt(int(pt.x), int(pt.y));
			brect += ipt;

			pt = UVWToScreen(Point3(1.0f, 1.0f, 1.0f), xzoom, yzoom, width, height);
			IPoint2 ipt2(int(pt.x), int(pt.y));
			brect += ipt2;

		}

		tileValid = FALSE;

		Rect srect;
		GetClientRect(hView, &srect);
		srect.top += 30;		//toolbar fudges
		srect.bottom -= 60;


		float rat1, rat2;
		double bw, bh;
		double sw, sh;

		if (brect.w() == 1)
		{

			brect.left--;
			brect.right++;
		}

		if (brect.h() == 1)
		{

			brect.top--;
			brect.bottom++;
		}


		bw = brect.w();
		bh = brect.h();

		sw = srect.w();
		sh = srect.h();




		rat1 = float(sw - 1.0f) / float(fabs(double(bw - 1.0f)));
		rat2 = float(sh - 1.0f) / float(fabs(double(bh - 1.0f)));
		float rat = (rat1 < rat2 ? rat1 : rat2) * 0.9f;

		BOOL redo = FALSE;
		if (_isnan(rat))
		{
			rat = 1.0f;
			redo = TRUE;
		}
		if (!_finite(rat))
		{
			rat = 1.0f;
			redo = TRUE;
		}

		zoom *= rat;



		IPoint2 delta = srect.GetCenter() - brect.GetCenter();
		xscroll += delta.x;
		yscroll += delta.y;
		xscroll *= rat;
		yscroll *= rat;
	}




	InvalidateView();

}

void UnwrapMod::FrameSelectedElement()
{



	Rect brect;
	Point2 pt;
	float xzoom, yzoom;
	int width, height;
	ComputeZooms(hView, xzoom, yzoom, width, height);
	brect.SetEmpty();
	int found = 0;
	BOOL doAll = TRUE;


	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray vsel = ld->GetTVVertSel();
		if (vsel.AnyBitSet())
			doAll = FALSE;
	}

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray faceHasSelectedVert;

		BitArray vsel = ld->GetTVVertSel();
		BitArray tempVSel = vsel;

		faceHasSelectedVert.SetSize(ld->GetNumberFaces());//TVMaps.f.Count());
		faceHasSelectedVert.ClearAll();

		int count = -1;

		while (count != vsel.NumberSet())
		{
			count = vsel.NumberSet();
			for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count();i++)
			{
				if (!ld->GetFaceDead(i))//(TVMaps.f[i]->flags & FLAG_DEAD))
				{
					int pcount = 3;
					pcount = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
					int totalSelected = 0;
					for (int k = 0; k < pcount; k++)
					{
						int index = ld->GetFaceTVVert(i, k);//TVMaps.f[i]->t[k];
						if (vsel[index])
						{
							totalSelected++;
						}
					}

					if ((totalSelected != pcount) && (totalSelected != 0))
					{
						faceHasSelectedVert.Set(i);
					}
				}
			}
			for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count();i++)
			{
				if (faceHasSelectedVert[i])
				{
					int pcount = 3;
					pcount = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
					for (int k = 0; k < pcount; k++)
					{
						int index = ld->GetFaceTVVert(i, k);//TVMaps.f[i]->t[k];
						vsel.Set(index, 1);
					}
				}

			}
		}


		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
		{
			if (!ld->GetTVVertDead(i))//(TVMaps.v[i].flags & FLAG_DEAD))
			{
				if ((vsel[i]) || doAll)
				{
					pt = UVWToScreen(ld->GetTVVert(i)/*GetPoint(t,i)*/, xzoom, yzoom, width, height);
					IPoint2 ipt(int(pt.x), int(pt.y));
					brect += ipt;
					found++;
				}
			}
		}
	}

	//	vsel = tempVSel;
	TransferSelectionEnd(FALSE, FALSE);

	if (brect.w() < 5)
	{
		brect.left -= 5;
		brect.right += 5;
	}

	if (brect.h() < 5)
	{
		brect.top -= 5;
		brect.bottom += 5;
	}


	if (found <= 1) return;
	Rect srect;
	GetClientRect(hView, &srect);
	srect.top += 30;		//toolbar fudges
	srect.bottom -= 60;

	float rat1 = 1.0f, rat2 = 1.0f;
	if (brect.w() > 2.0f)
		rat1 = float(srect.w() - 1) / float(fabs(double(brect.w() - 1)));
	if (brect.h() > 2.0f)
		rat2 = float(srect.h() - 1) / float(fabs(double(brect.h() - 1)));
	float rat = (rat1 < rat2 ? rat1 : rat2) * 0.9f;
	float tempZoom = zoom *rat;
	if (tempZoom <= 1000.0f)
	{
		zoom *= rat;
		IPoint2 delta = srect.GetCenter() - brect.GetCenter();
		xscroll += delta.x;
		yscroll += delta.y;
		xscroll *= rat;
		yscroll *= rat;
		InvalidateView();
	}

}

void UnwrapMod::fnFrameSelectedElement()
{
	FrameSelectedElement();
}

void UnwrapMod::ZoomSelected()
{
	TransferSelectionStart();

	Rect brect;
	Point2 pt;
	float xzoom, yzoom;
	int width, height;
	ComputeZooms(hView, xzoom, yzoom, width, height);
	brect.SetEmpty();
	int found = 0;
	BOOL doAll = TRUE;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray vsel = ld->GetTVVertSel();
		if (vsel.AnyBitSet())
			doAll = FALSE;
	}

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
		{
			if (!ld->GetTVVertDead(i))//(TVMaps.v[i].flags & FLAG_DEAD))
			{
				if ((ld->GetTVVertSelected(i)/*vsel[i]*/) || doAll)
				{
					pt = UVWToScreen(ld->GetTVVert(i)/*GetPoint(t,i)*/, xzoom, yzoom, width, height);
					IPoint2 ipt(int(pt.x), int(pt.y));
					brect += ipt;
					found++;
				}
			}
		}
	}

	TransferSelectionEnd(FALSE, FALSE);

	if (brect.w() < 5)
	{
		brect.left -= 5;
		brect.right += 5;
	}

	if (brect.h() < 5)
	{
		brect.top -= 5;
		brect.bottom += 5;
	}

	if (found <= 1) return;

	tileValid = FALSE;

	const int topToolFudge = 30;
	const int bottomToolFudge = 60;

	Rect srect;
	GetClientRect(hView, &srect);
	srect.top += topToolFudge;		//toolbar fudges
	srect.bottom -= bottomToolFudge;


	float rat1 = 1.0f, rat2 = 1.0f;
	if (brect.w() > 2.0f)
		rat1 = float(srect.w() - 1) / float(fabs(double(brect.w() - 1)));
	if (brect.h() > 2.0f)
		rat2 = float(srect.h() - 1) / float(fabs(double(brect.h() - 1)));
	float rat = (rat1 < rat2 ? rat1 : rat2) * 0.9f;
	float tempZoom = zoom *rat;
	float centerBias = 0.5 * (bottomToolFudge - topToolFudge);
	if (tempZoom <= 1000.0f)
	{
		zoom *= rat;
		IPoint2 delta = srect.GetCenter() - brect.GetCenter();
		xscroll += delta.x;
		yscroll += (delta.y + centerBias);
		xscroll *= rat;
		yscroll *= rat;
		yscroll -= centerBias;
		InvalidateView();
	}
}



void UnwrapMod::RebuildFreeFormData()
{
	//compute your zooms and scale
	float xzoom, yzoom;
	int width, height;
	ComputeZooms(hView, xzoom, yzoom, width, height);

	int count = 0;

	TransferSelectionStart();

	count = 0;//vsel.NumberSet();


	freeFormBounds.Init();
	if (!inRotation)
		selCenter = Point3(0.0f, 0.0f, 0.0f);
	int i1, i2;
	GetUVWIndices(i1, i2);

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		int vselCount = ld->GetNumberTVVerts();//vsel.GetSize();
		if (ld->GetTVVertSel().AnyBitSet())
		{
			for (int i = 0; i < vselCount; i++)
			{
				if (ld->GetTVVertSelected(i))//vsel[i])
				{
					//get bounds
					Point3 p(0.0f, 0.0f, 0.0f);
					Point3 tv = ld->GetTVVert(i);
					p[i1] = tv[i1];
					p[i2] = tv[i2];
					//			p.z = 0.0f;
					freeFormBounds += p;
					count++;
				}
			}
		}
	}
	Point3 tempCenter;
	if (!inRotation)
		selCenter = freeFormBounds.Center();
	else tempCenter = freeFormBounds.Center();

	if (count > 0)
	{

		//draw gizmo bounds
		Point2 prect[4];
		prect[0] = UVWToScreen(freeFormBounds.pmin, xzoom, yzoom, width, height);
		prect[1] = UVWToScreen(freeFormBounds.pmax, xzoom, yzoom, width, height);
		float xexpand = 15.0f / xzoom;
		float yexpand = 15.0f / yzoom;

		if (freeFormMode && !freeFormMode->dragging)
		{
			if ((prect[1].x - prect[0].x) < 30)
			{
				prect[1].x += 15;
				prect[0].x -= 15;
				//expand bounds
				freeFormBounds.pmax.x += xexpand;
				freeFormBounds.pmin.x -= xexpand;
			}
			if ((prect[0].y - prect[1].y) < 30)
			{
				prect[1].y -= 15;
				prect[0].y += 15;
				freeFormBounds.pmax.y += yexpand;
				freeFormBounds.pmin.y -= yexpand;

			}
		}
	}
	TransferSelectionEnd(FALSE, FALSE);
}


static INT_PTR CALLBACK PropDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static IColorSwatch *iSelColor, *iLineColor, *iOpenEdgeColor, *iHandleColor, *iGizmoColor, *iSharedColor, *iBackgroundColor;
	static IColorSwatch *iPeelColor;
	static ISpinnerControl *spinW, *spinH;


	static ISpinnerControl *spinTileLimit;
	static ISpinnerControl *spinTileContrast;

	static ISpinnerControl *spinLimitSoftSel;
	static ISpinnerControl *spinHitSize;
	static ISpinnerControl *spinTickSize;
	//new
	static IColorSwatch *iGridColor;
	static ISpinnerControl *spinGridSize;
	static ISpinnerControl *spinCheckerTiling;
	
	static BOOL uiDestroyed = FALSE;

	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static COLORREF selColor, lineColor, openEdgeColor, handleColor, sharedColor, backgroundColor, freeFormColor, gridColor;
	static COLORREF peelColor;

	static BOOL update = FALSE;
	static BOOL showVerts = FALSE;
	
	static BOOL useBitmapRes = FALSE;
	static BOOL displayPixelUnits = FALSE;
	static BOOL fnGetTile = FALSE;
	static BOOL fnGetDisplayOpenEdges = FALSE;

	static BOOL fnGetThickOpenEdges = FALSE;
	static BOOL fnGetViewportOpenEdges = FALSE;

	static BOOL fnGetDisplayHiddenEdges = FALSE;
	static BOOL fnGetShowShared = FALSE;
	static BOOL fnGetBrightCenterTile = FALSE;
	static BOOL fnGetBlendToBack = FALSE;
	static BOOL fnGetFilterMap = FALSE;
	static BOOL fnGetGridVisible = FALSE;
	static BOOL fnGetTileGridVisible = FALSE;
	static BOOL fnGetLimitSoftSel = FALSE;

	static int rendW = 256;
	static int rendH = 256;
	static int fnGetHitSize = 3;

	static int fnGetTickSize = 2;
	static int fnGetLimitSoftSelRange = 8;
	static int fnGetFillMode = 0;
	static int fnGetTileLimit = 2;

	static float fnGetTileContrast = 0.0f;
	static float weldThreshold = 0.05f;

	static float fnGetGridSize = .2f;
	
	static BOOL fnShowAlpha = FALSE;
	static float sfnCheckerTiling = CHECKERTILINGDEFAULT;

	BOOL updateUI = FALSE;
	BOOL updatePixelUnits = FALSE;
	BOOL oldDisplayPixelUnits = FALSE;
	static BOOL fnNonSquareApplyBitmapRatio = TRUE;

	switch (msg) {
	case WM_INITDIALOG: {
		uiDestroyed = FALSE;
		mod = (UnwrapMod*)lParam;
		DLSetWindowLongPtr(hWnd, lParam);

		::SetWindowContextHelpId(hWnd, idh_unwrap_options);

		mod->optionsDialogActive = TRUE;
		mod->hOptionshWnd = hWnd;

		iSelColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_SELCOLOR),
			mod->selColor, GetString(IDS_PW_LINECOLOR));
		iLineColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_LINECOLOR),
			mod->lineColor, GetString(IDS_PW_LINECOLOR));
		iPeelColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_PEELCOLOR),
			mod->peelColor, GetString(IDS_PEEL_COLOR));

		CheckDlgButton(hWnd, IDC_UNWRAP_CONSTANTUPDATE, mod->update);

		// Not exposed in the UI
		//CheckDlgButton(hWnd, IDC_UNWRAP_SELECT_VERTS, mod->showVerts);
		

		CheckDlgButton(hWnd, IDC_UNWRAP_USEBITMAPRES, !mod->useBitmapRes);
		Texmap *activeMap = mod->GetActiveMap();
		if (activeMap && (activeMap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0)))
			EnableWindow(GetDlgItem(hWnd, IDC_UNWRAP_USEBITMAPRES), TRUE);

		CheckDlgButton(hWnd, IDC_UNWRAP_DISPLAYPIXELUNITS, mod->displayPixelUnits);

		spinW = SetupIntSpinner(
			hWnd, IDC_UNWRAP_WIDTHSPIN, IDC_UNWRAP_WIDTH,
			2, 2048, mod->rendW);
		spinH = SetupIntSpinner(
			hWnd, IDC_UNWRAP_HEIGHTSPIN, IDC_UNWRAP_HEIGHT,
			2, 2048, mod->rendH);

		CheckDlgButton(hWnd, IDC_UNWRAP_TILEMAP, mod->fnGetTile());

		spinTileLimit = SetupIntSpinner(
			hWnd, IDC_UNWRAP_TILELIMITSPIN, IDC_UNWRAP_TILELIMIT,
			0, 50, mod->fnGetTileLimit());
		spinTileContrast = SetupFloatSpinner(
			hWnd, IDC_UNWRAP_TILECONTRASTSPIN, IDC_UNWRAP_TILECONTRAST,
			0.0f, 1.0f, mod->fnGetTileContrast());

		CheckDlgButton(hWnd, IDC_UNWRAP_LIMITSOFTSEL, mod->fnGetLimitSoftSel());
		spinLimitSoftSel = SetupIntSpinner(
			hWnd, IDC_UNWRAP_LIMITSOFTSELRANGESPIN, IDC_UNWRAP_LIMITSOFTSELRANGE,
			0, 100, mod->fnGetLimitSoftSelRange());

		HWND hFill = GetDlgItem(hWnd, IDC_FILL_COMBO);
		SendMessage(hFill, CB_RESETCONTENT, 0, 0);

		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_NOFILL));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_SOLID));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_CROSS));
		if (mod->fnGetFillMode() > 3)
			mod->fnSetFillMode(3);
		SendMessage(hFill, CB_SETCURSEL, mod->fnGetFillMode() - 1, 0L);

		iOpenEdgeColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_OPENEDGECOLOR),
			mod->openEdgeColor, GetString(IDS_PW_OPENEDGECOLOR));
		CheckDlgButton(hWnd, IDC_DISPLAYOPENEDGES_CHECK, mod->fnGetDisplayOpenEdges());

		// Not exposed in the UI
		//CheckDlgButton(hWnd, IDC_THICKOPENEDGES_CHECK, mod->fnGetThickOpenEdges());
		//CheckDlgButton(hWnd, IDC_VIEWSEAMSCHECK, mod->fnGetViewportOpenEdges());

		CheckDlgButton(hWnd, IDC_UNWRAP_DISPLAYHIDDENEDGES, mod->fnGetDisplayHiddenEdges());
		if (!mod->fnIsMesh())
			EnableWindow(GetDlgItem(hWnd, IDC_UNWRAP_DISPLAYHIDDENEDGES), FALSE);

		iHandleColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_HANDLECOLOR),
			mod->handleColor, GetString(IDS_PW_HANDLECOLOR));

		iGizmoColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_GIZMOCOLOR),
			mod->freeFormColor, GetString(IDS_PW_HANDLECOLOR));

		spinHitSize = SetupIntSpinner(
			hWnd, IDC_UNWRAP_HITSIZESPIN, IDC_UNWRAP_HITSIZE,
			1, 10, mod->fnGetHitSize());

		iSharedColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_SHARECOLOR),
			mod->sharedColor, GetString(IDS_PW_HANDLECOLOR));
		CheckDlgButton(hWnd, IDC_SHOWSHARED_CHECK, mod->fnGetShowShared());

		iBackgroundColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_BACKGROUNDCOLOR),
			mod->backgroundColor, GetString(IDS_PW_BACKGROUNDCOLOR));

		CheckDlgButton(hWnd, IDC_UNWRAP_AFFECTCENTERTILE, mod->fnGetBrightCenterTile());
		CheckDlgButton(hWnd, IDC_UNWRAP_BLENDTOBACK, mod->fnGetBlendToBack());

		CheckDlgButton(hWnd, IDC_UNWRAP_FILTERMAP, mod->fnGetFilterMap());

		spinTickSize = SetupIntSpinner(
			hWnd, IDC_UNWRAP_TICKSIZESPIN, IDC_UNWRAP_TICKSIZE,
			1, 10, mod->fnGetTickSize());

		//new
		CheckDlgButton(hWnd, IDC_SHOWGRID_CHECK, mod->fnGetGridVisible());
		spinGridSize = SetupFloatSpinner(
			hWnd, IDC_UNWRAP_GRIDSIZESPIN, IDC_UNWRAP_GRIDSIZE,
			0.00001f, 1.0f, mod->fnGetGridSize());

		CheckDlgButton(hWnd, IDC_UNWRAP_SHOWTILEGRID_CHECK, mod->fnGetTileGridVisible());

		iGridColor = GetIColorSwatch(GetDlgItem(hWnd, IDC_UNWRAP_GRIDCOLOR),
			mod->gridColor, GetString(IDS_PW_OPENEDGECOLOR));

		spinCheckerTiling = SetupFloatSpinner(
			hWnd, IDC_UNWRAP_CHECKER_TILING_SPIN, IDC_UNWRAP_CHECKER_TILING,
			1.0f, 500.0f, mod->fnGetCheckerTiling());
		//record the initial value that can be used for restore if the user then click the "cancel" button 
		//after input some other values in the edit box.
		sfnCheckerTiling = mod->fnGetCheckerTiling();

		selColor = mod->selColor;
		lineColor = mod->lineColor;
		openEdgeColor = mod->openEdgeColor;
		handleColor = mod->handleColor;
		sharedColor = mod->sharedColor;
		backgroundColor = mod->backgroundColor;
		freeFormColor = mod->freeFormColor;
		gridColor = mod->gridColor;
		peelColor = mod->peelColor;

		update = mod->update;
		showVerts = mod->showVerts;
		
		useBitmapRes = mod->useBitmapRes;
		displayPixelUnits = mod->displayPixelUnits;
		fnGetTile = mod->fnGetTile();
		fnGetDisplayOpenEdges = mod->fnGetDisplayOpenEdges();

		fnGetThickOpenEdges = mod->fnGetThickOpenEdges();
		fnGetViewportOpenEdges = mod->fnGetViewportOpenEdges();

		fnGetDisplayHiddenEdges = mod->fnGetDisplayHiddenEdges();
		fnGetShowShared = mod->fnGetShowShared();
		fnGetBrightCenterTile = mod->fnGetBrightCenterTile();
		fnGetBlendToBack = mod->fnGetBlendToBack();
		fnGetFilterMap = mod->fnGetFilterMap();
		fnGetGridVisible = mod->fnGetGridVisible();
		fnGetTileGridVisible = mod->fnGetTileGridVisible();
		fnGetLimitSoftSel = mod->fnGetLimitSoftSel();

		rendW = mod->rendW;
		rendH = mod->rendH;
		fnGetHitSize = mod->fnGetHitSize();

		fnGetTickSize = mod->fnGetTickSize();
		fnGetLimitSoftSelRange = mod->fnGetLimitSoftSelRange();
		fnGetFillMode = mod->fnGetFillMode();
		fnGetTileLimit = mod->fnGetTileLimit();

		fnGetTileContrast = mod->fnGetTileContrast();
		weldThreshold = mod->weldThreshold;

		fnGetGridSize = mod->fnGetGridSize();
		
		fnShowAlpha = mod->GetShowImageAlpha();

		CheckDlgButton(hWnd, IDC_UNWRAP_SHOWIMAGEALPHA, mod->GetShowImageAlpha());
		CheckDlgButton(hWnd, IDC_UNWRAP_SEL_PREVIEW, mod->fnGetSelectionPreview() ? TRUE : FALSE);

		CheckDlgButton(hWnd, IDC_UNWRAP_NONSQUARE_APPLY_BITMAP_RATIO, mod->fnGetNonSquareApplyBitmapRatio());
		fnNonSquareApplyBitmapRatio = mod->fnGetNonSquareApplyBitmapRatio();

		break;
	}

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_CONTEXTHELP)
		{
			MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_options);
		}
		return FALSE;
		break;
	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
	case CC_COLOR_CHANGE:
	{
		updateUI = TRUE;
		break;
	}
	case WM_DESTROY:
	{
		mod->optionsDialogActive = FALSE;
		mod->hOptionshWnd = NULL;
		updateUI = FALSE;
		if (!uiDestroyed)
		{
			ReleaseIColorSwatch(iSelColor);
			ReleaseIColorSwatch(iLineColor);
			ReleaseIColorSwatch(iOpenEdgeColor);
			ReleaseIColorSwatch(iHandleColor);
			ReleaseIColorSwatch(iGizmoColor);
			ReleaseIColorSwatch(iSharedColor);
			ReleaseIColorSwatch(iBackgroundColor);
			ReleaseIColorSwatch(iPeelColor);

			ReleaseISpinner(spinW);
			ReleaseISpinner(spinH);

			ReleaseISpinner(spinTileLimit);
			ReleaseISpinner(spinTileContrast);

			ReleaseISpinner(spinLimitSoftSel);
			ReleaseISpinner(spinHitSize);

			//new
			ReleaseISpinner(spinGridSize);
			ReleaseIColorSwatch(iGridColor);

			ReleaseISpinner(spinCheckerTiling);
			uiDestroyed = TRUE;
			DestroyWindow(hWnd);
		}
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_FILL_COMBO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				updateUI = TRUE;
			break;

		case IDC_DISPLAYOPENEDGES_CHECK:
		//case IDC_THICKOPENEDGES_CHECK:	// Not exposed in the UI
		//case IDC_VIEWSEAMSCHECK:			// Not exposed in the UI
		case IDC_SHOWGRID_CHECK:
		case IDC_UNWRAP_SHOWTILEGRID_CHECK:
		case IDC_UNWRAP_USEBITMAPRES:
		case IDC_UNWRAP_DISPLAYPIXELUNITS:
		case IDC_UNWRAP_TILEMAP:
		case IDC_UNWRAP_CONSTANTUPDATE:
		case IDC_UNWRAP_AFFECTCENTERTILE:
		//case IDC_UNWRAP_SELECT_VERTS:		// Not exposed in the UI
		case IDC_UNWRAP_DISPLAYHIDDENEDGES:
		case IDC_UNWRAP_BLENDTOBACK:
		case IDC_UNWRAP_LIMITSOFTSEL:
		case IDC_UNWRAP_SHOWIMAGEALPHA:
		case IDC_UNWRAP_SEL_PREVIEW:
		case IDC_UNWRAP_CHECKER_TILING:
		case IDC_UNWRAP_NONSQUARE_APPLY_BITMAP_RATIO:
		case IDC_UNWRAP_FILTERMAP:
			updateUI = TRUE;
			break;
		case IDOK:
		{
			mod->hOptionshWnd = NULL;
			mod->lineColor = iLineColor->GetColor();
			mod->selColor = iSelColor->GetColor();
			mod->peelColor = iPeelColor->GetColor();
			mod->update = IsDlgButtonChecked(hWnd, IDC_UNWRAP_CONSTANTUPDATE);

			// Not exposed in the UI
			//mod->showVerts = IsDlgButtonChecked(hWnd, IDC_UNWRAP_SELECT_VERTS);
			
			//watje 5-3-99
			BOOL oldRes = mod->useBitmapRes;
			oldDisplayPixelUnits = mod->displayPixelUnits;

			mod->SetShowImageAlpha(IsDlgButtonChecked(hWnd, IDC_UNWRAP_SHOWIMAGEALPHA));
			mod->fnSetSelectionPreview(IsDlgButtonChecked(hWnd, IDC_UNWRAP_SEL_PREVIEW) ? true : false);

			mod->useBitmapRes = !IsDlgButtonChecked(hWnd, IDC_UNWRAP_USEBITMAPRES);
			mod->displayPixelUnits = IsDlgButtonChecked(hWnd, IDC_UNWRAP_DISPLAYPIXELUNITS);

			mod->rendW = spinW->GetIVal();
			mod->rendH = spinH->GetIVal();
			//watje 5-3-99
			if (mod->rendW != mod->iw ||
				mod->rendH != mod->ih || oldRes != mod->useBitmapRes) {
				mod->SetupImage();
			}

			if( oldDisplayPixelUnits!=mod->displayPixelUnits )
				updatePixelUnits = TRUE;

			mod->fnSetTileLimit(spinTileLimit->GetIVal());
			mod->fnSetTileContrast(spinTileContrast->GetFVal());

			mod->fnSetTile(IsDlgButtonChecked(hWnd, IDC_UNWRAP_TILEMAP));


			mod->fnSetLimitSoftSel(IsDlgButtonChecked(hWnd, IDC_UNWRAP_LIMITSOFTSEL));
			mod->fnSetLimitSoftSelRange(spinLimitSoftSel->GetIVal());

			HWND hFill = GetDlgItem(hWnd, IDC_FILL_COMBO);
			mod->fnSetFillMode(SendMessage(hFill, CB_GETCURSEL, 0L, 0L) + 1);

			mod->fnSetDisplayOpenEdges(IsDlgButtonChecked(hWnd, IDC_DISPLAYOPENEDGES_CHECK));

			// Not exposed in the UI
			//mod->fnSetThickOpenEdges(IsDlgButtonChecked(hWnd, IDC_THICKOPENEDGES_CHECK));
			//mod->fnSetViewportOpenEdges(IsDlgButtonChecked(hWnd, IDC_VIEWSEAMSCHECK));

			mod->fnSetDisplayHiddenEdges(IsDlgButtonChecked(hWnd, IDC_UNWRAP_DISPLAYHIDDENEDGES));
			mod->openEdgeColor = iOpenEdgeColor->GetColor();
			mod->handleColor = iHandleColor->GetColor();
			mod->freeFormColor = iGizmoColor->GetColor();
			mod->sharedColor = iSharedColor->GetColor();

			mod->backgroundColor = iBackgroundColor->GetColor();

			mod->fnSetHitSize(spinHitSize->GetIVal());
			mod->fnSetTickSize(spinTickSize->GetIVal());


			mod->fnSetShowShared(IsDlgButtonChecked(hWnd, IDC_SHOWSHARED_CHECK));
			mod->fnSetBrightCenterTile(IsDlgButtonChecked(hWnd, IDC_UNWRAP_AFFECTCENTERTILE));
			mod->fnSetBlendToBack(IsDlgButtonChecked(hWnd, IDC_UNWRAP_BLENDTOBACK));

			mod->fnSetFilterMap(IsDlgButtonChecked(hWnd, IDC_UNWRAP_FILTERMAP));

			//new			
			mod->fnSetGridVisible(IsDlgButtonChecked(hWnd, IDC_SHOWGRID_CHECK));
			mod->fnSetGridSize(spinGridSize->GetFVal());
			mod->fnSetTileGridVisible(IsDlgButtonChecked(hWnd, IDC_UNWRAP_SHOWTILEGRID_CHECK));
			
			mod->gridColor = iGridColor->GetColor();

			mod->fnSetCheckerTiling(spinCheckerTiling->GetFVal());

			mod->fnSetNonSquareApplyBitmapRatio(IsDlgButtonChecked(hWnd, IDC_UNWRAP_NONSQUARE_APPLY_BITMAP_RATIO));

			mod->tileValid = FALSE;
			mod->RebuildDistCache();
			mod->InvalidateView();

			ReleaseIColorSwatch(iSelColor);
			ReleaseIColorSwatch(iLineColor);
			ReleaseIColorSwatch(iOpenEdgeColor);
			ReleaseIColorSwatch(iHandleColor);
			ReleaseIColorSwatch(iGizmoColor);
			ReleaseIColorSwatch(iSharedColor);
			ReleaseIColorSwatch(iBackgroundColor);
			ReleaseIColorSwatch(iPeelColor);


			ReleaseISpinner(spinW);
			ReleaseISpinner(spinH);

			ReleaseISpinner(spinTileLimit);
			ReleaseISpinner(spinTileContrast);

			ReleaseISpinner(spinLimitSoftSel);
			ReleaseISpinner(spinHitSize);

			//new
			ReleaseISpinner(spinGridSize);
			
			ReleaseIColorSwatch(iGridColor);

			ReleaseISpinner(spinCheckerTiling);

			mod->optionsDialogActive = FALSE;
			updateUI = FALSE;

			uiDestroyed = TRUE;

			DestroyWindow(hWnd);

			//fall through to release controls'
			break;
		}
		case IDCANCEL:

			mod->hOptionshWnd = NULL;
			ReleaseIColorSwatch(iSelColor);
			ReleaseIColorSwatch(iLineColor);
			ReleaseIColorSwatch(iOpenEdgeColor);
			ReleaseIColorSwatch(iHandleColor);
			ReleaseIColorSwatch(iGizmoColor);
			ReleaseIColorSwatch(iSharedColor);
			ReleaseIColorSwatch(iBackgroundColor);
			ReleaseIColorSwatch(iPeelColor);


			ReleaseISpinner(spinW);
			ReleaseISpinner(spinH);

			ReleaseISpinner(spinTileLimit);
			ReleaseISpinner(spinTileContrast);

			ReleaseISpinner(spinLimitSoftSel);
			ReleaseISpinner(spinHitSize);

			//new
			ReleaseISpinner(spinGridSize);
			
			ReleaseIColorSwatch(iGridColor);

			ReleaseISpinner(spinCheckerTiling);

			//Restore the old value.
			mod->fnSetCheckerTiling(sfnCheckerTiling);

			mod->selColor = selColor;
			mod->lineColor = lineColor;
			mod->peelColor = peelColor;
			mod->openEdgeColor = openEdgeColor;
			mod->handleColor = handleColor;
			mod->sharedColor = sharedColor;
			mod->backgroundColor = backgroundColor;
			mod->freeFormColor = freeFormColor;
			mod->gridColor = gridColor;

			mod->update = update;
			mod->showVerts = showVerts;
			
			oldDisplayPixelUnits = mod->displayPixelUnits;

			mod->useBitmapRes = useBitmapRes;
			mod->displayPixelUnits = displayPixelUnits;
			if( oldDisplayPixelUnits!=mod->displayPixelUnits )
				updatePixelUnits = TRUE;

			mod->fnSetTile(fnGetTile);

			mod->fnSetDisplayOpenEdges(fnGetDisplayOpenEdges);

			mod->fnSetThickOpenEdges(fnGetThickOpenEdges);
			mod->fnSetViewportOpenEdges(fnGetViewportOpenEdges);

			mod->fnSetDisplayHiddenEdges(fnGetDisplayHiddenEdges);
			mod->fnSetShowShared(fnGetShowShared);
			mod->fnSetBrightCenterTile(fnGetBrightCenterTile);
			mod->fnSetBlendToBack(fnGetBlendToBack);
			mod->fnSetFilterMap(fnGetFilterMap);
			mod->fnSetGridVisible(fnGetGridVisible);
			mod->fnSetTileGridVisible(fnGetTileGridVisible);
			mod->fnSetLimitSoftSel(fnGetLimitSoftSel);


			mod->rendW = rendW;
			mod->rendH = rendH;
			mod->fnSetHitSize(fnGetHitSize);

			mod->fnSetTickSize(fnGetTickSize);
			mod->fnSetLimitSoftSelRange(fnGetLimitSoftSelRange);
			mod->fnSetFillMode(fnGetFillMode);
			mod->fnSetTileLimit(fnGetTileLimit);

			mod->fnSetTileContrast(fnGetTileContrast);
			mod->weldThreshold = weldThreshold;

			mod->fnSetGridSize(fnGetGridSize);
			
			mod->SetShowImageAlpha(fnShowAlpha);

			mod->fnSetNonSquareApplyBitmapRatio(fnNonSquareApplyBitmapRatio);

			mod->optionsDialogActive = FALSE;
			updateUI = FALSE;

			mod->tileValid = FALSE;
			mod->RebuildDistCache();
			mod->InvalidateView();

			uiDestroyed = TRUE;

			DestroyWindow(hWnd);
			break;

		case IDC_UNWRAP_DEFAULTS:
			
			oldDisplayPixelUnits = mod->displayPixelUnits;

			//load the preference section that appear in this dialog 
			mod->fnLoadDefaults(PREFERENCESSECTION);

			mod->lineColor = RGB(255, 255, 255);
			mod->selColor = RGB(255, 0, 0);
			mod->openEdgeColor = RGB(0, 255, 0);
			mod->handleColor = RGB(255, 255, 0);
			mod->freeFormColor = RGB(255, 100, 50);
			mod->sharedColor = RGB(0, 0, 255);
			mod->peelColor = RGB(181, 109, 218);

			mod->backgroundColor = RGB(60, 60, 60);

			selColor = mod->selColor;
			lineColor = mod->lineColor;
			openEdgeColor = mod->openEdgeColor;
			handleColor = mod->handleColor;
			sharedColor = mod->sharedColor;
			backgroundColor = mod->backgroundColor;
			freeFormColor = mod->freeFormColor;
			gridColor = mod->gridColor;
			peelColor = mod->peelColor;

			update = mod->update;
			showVerts = mod->showVerts;
			
			useBitmapRes = mod->useBitmapRes;
			displayPixelUnits = mod->displayPixelUnits;
			if( oldDisplayPixelUnits!=mod->displayPixelUnits )
				updatePixelUnits = TRUE;

			fnGetTile = mod->fnGetTile();
			fnGetDisplayOpenEdges = mod->fnGetDisplayOpenEdges();

			fnGetThickOpenEdges = mod->fnGetThickOpenEdges();
			fnGetViewportOpenEdges = mod->fnGetViewportOpenEdges();


			fnGetDisplayHiddenEdges = mod->fnGetDisplayHiddenEdges();
			fnGetShowShared = mod->fnGetShowShared();
			fnGetBrightCenterTile = mod->fnGetBrightCenterTile();
			fnGetBlendToBack = mod->fnGetBlendToBack();
			fnGetFilterMap = mod->fnGetFilterMap();
			fnGetGridVisible = mod->fnGetGridVisible();
			fnGetTileGridVisible = mod->fnGetTileGridVisible();
			fnGetLimitSoftSel = mod->fnGetLimitSoftSel();

			rendW = mod->rendW;
			rendH = mod->rendH;
			fnGetHitSize = mod->fnGetHitSize();

			fnGetTickSize = mod->fnGetTickSize();
			fnGetLimitSoftSelRange = mod->fnGetLimitSoftSelRange();
			fnGetFillMode = mod->fnGetFillMode();
			fnGetTileLimit = mod->fnGetTileLimit();

			fnGetTileContrast = mod->fnGetTileContrast();
			weldThreshold = mod->weldThreshold;

			fnGetGridSize = mod->fnGetGridSize();

			iSelColor->SetColor(mod->selColor);
			iLineColor->SetColor(mod->lineColor);
			iPeelColor->SetColor(mod->peelColor);

			spinCheckerTiling->SetValue(CHECKERTILINGDEFAULT, FALSE);
			mod->fnSetCheckerTiling(CHECKERTILINGDEFAULT);

			CheckDlgButton(hWnd, IDC_UNWRAP_CONSTANTUPDATE, mod->update);


			// Not exposed in the UI
			//CheckDlgButton(hWnd, IDC_UNWRAP_SELECT_VERTS, mod->showVerts);
			

			CheckDlgButton(hWnd, IDC_UNWRAP_USEBITMAPRES, !mod->useBitmapRes);
			Texmap *activeMap = mod->GetActiveMap();
			if (activeMap && (activeMap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0)))
				EnableWindow(GetDlgItem(hWnd, IDC_UNWRAP_USEBITMAPRES), TRUE);

			CheckDlgButton(hWnd, IDC_UNWRAP_DISPLAYPIXELUNITS, mod->displayPixelUnits);

			spinW->SetValue(mod->rendW, FALSE);
			spinH->SetValue(mod->rendH, FALSE);

			CheckDlgButton(hWnd, IDC_UNWRAP_TILEMAP, mod->fnGetTile());

			spinTileLimit->SetValue(mod->fnGetTileLimit(), FALSE);
			spinTileContrast->SetValue(mod->fnGetTileContrast(), FALSE);

			CheckDlgButton(hWnd, IDC_UNWRAP_LIMITSOFTSEL, mod->fnGetLimitSoftSel());
			spinLimitSoftSel->SetValue(mod->fnGetLimitSoftSelRange(), FALSE);

			HWND hFill = GetDlgItem(hWnd, IDC_FILL_COMBO);
			SendMessage(hFill, CB_RESETCONTENT, 0, 0);

			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_NOFILL));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_SOLID));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_BDIAG));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_CROSS));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_DIAGCROSS));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_FDIAG));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_HORIZONTAL));
			SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_PW_FILL_VERTICAL));

			SendMessage(hFill, CB_SETCURSEL, mod->fnGetFillMode() - 1, 0L);

			iOpenEdgeColor->SetColor(mod->openEdgeColor);
			CheckDlgButton(hWnd, IDC_DISPLAYOPENEDGES_CHECK, mod->fnGetDisplayOpenEdges());

			// Not exposed in the UI
			//CheckDlgButton(hWnd, IDC_THICKOPENEDGES_CHECK, mod->fnGetThickOpenEdges());
			//CheckDlgButton(hWnd, IDC_VIEWSEAMSCHECK, mod->fnGetViewportOpenEdges());

			CheckDlgButton(hWnd, IDC_UNWRAP_DISPLAYHIDDENEDGES, mod->fnGetDisplayHiddenEdges());

			iHandleColor->SetColor(mod->handleColor);

			iGizmoColor->SetColor(mod->freeFormColor);

			spinHitSize->SetValue(mod->fnGetHitSize(), FALSE);

			iSharedColor->SetColor(mod->sharedColor);
			CheckDlgButton(hWnd, IDC_SHOWSHARED_CHECK, mod->fnGetShowShared());

			iBackgroundColor->SetColor(mod->backgroundColor);

			CheckDlgButton(hWnd, IDC_UNWRAP_AFFECTCENTERTILE, mod->fnGetBrightCenterTile());
			CheckDlgButton(hWnd, IDC_UNWRAP_BLENDTOBACK, mod->fnGetBlendToBack());

			CheckDlgButton(hWnd, IDC_UNWRAP_FILTERMAP, mod->fnGetFilterMap());

			spinTickSize->SetValue(mod->fnGetTickSize(), FALSE);

			//new
			CheckDlgButton(hWnd, IDC_SHOWGRID_CHECK, mod->fnGetGridVisible());
			spinGridSize->SetValue(mod->fnGetGridSize(), FALSE);
			iGridColor->SetColor(mod->gridColor);
			CheckDlgButton(hWnd, IDC_UNWRAP_SHOWTILEGRID_CHECK, mod->fnGetTileGridVisible());

			mod->SetShowImageAlpha(FALSE);
			CheckDlgButton(hWnd, IDC_UNWRAP_SHOWIMAGEALPHA, mod->GetShowImageAlpha());
			mod->fnSetSelectionPreview(false);
			CheckDlgButton(hWnd, IDC_UNWRAP_SEL_PREVIEW, mod->fnGetSelectionPreview() ? TRUE : FALSE);

			fnNonSquareApplyBitmapRatio = mod->fnGetNonSquareApplyBitmapRatio();
			CheckDlgButton(hWnd, IDC_UNWRAP_NONSQUARE_APPLY_BITMAP_RATIO, mod->fnGetNonSquareApplyBitmapRatio());

			break;
		}
		break;

	default:
		return FALSE;
	}
	if ((updateUI) && mod->optionsDialogActive)
	{
		mod->lineColor = iLineColor->GetColor();
		mod->selColor = iSelColor->GetColor();
		mod->peelColor = iPeelColor->GetColor();
		mod->update = IsDlgButtonChecked(hWnd, IDC_UNWRAP_CONSTANTUPDATE);

		// Not exposed in the UI
		//mod->showVerts = IsDlgButtonChecked(hWnd, IDC_UNWRAP_SELECT_VERTS);
		
		//watje 5-3-99
		BOOL oldRes = mod->useBitmapRes;
		oldDisplayPixelUnits = mod->displayPixelUnits;

		mod->useBitmapRes = !IsDlgButtonChecked(hWnd, IDC_UNWRAP_USEBITMAPRES);
		mod->displayPixelUnits = IsDlgButtonChecked(hWnd, IDC_UNWRAP_DISPLAYPIXELUNITS);

		mod->rendW = spinW->GetIVal();
		mod->rendH = spinH->GetIVal();
		//watje 5-3-99
		if (mod->rendW != mod->iw ||
			mod->rendH != mod->ih || oldRes != mod->useBitmapRes) {
			mod->SetupImage();
		}

		if( oldDisplayPixelUnits!=mod->displayPixelUnits )
			updatePixelUnits = TRUE;

		mod->fnSetTileLimit(spinTileLimit->GetIVal());
		mod->fnSetTileContrast(spinTileContrast->GetFVal());

		mod->fnSetTile(IsDlgButtonChecked(hWnd, IDC_UNWRAP_TILEMAP));


		mod->fnSetLimitSoftSel(IsDlgButtonChecked(hWnd, IDC_UNWRAP_LIMITSOFTSEL));
		mod->fnSetLimitSoftSelRange(spinLimitSoftSel->GetIVal());

		HWND hFill = GetDlgItem(hWnd, IDC_FILL_COMBO);
		mod->fnSetFillMode(SendMessage(hFill, CB_GETCURSEL, 0L, 0L) + 1);

		mod->fnSetDisplayOpenEdges(IsDlgButtonChecked(hWnd, IDC_DISPLAYOPENEDGES_CHECK));

		// Not exposed in the UI
		//mod->fnSetThickOpenEdges(IsDlgButtonChecked(hWnd, IDC_THICKOPENEDGES_CHECK));
		//mod->fnSetViewportOpenEdges(IsDlgButtonChecked(hWnd, IDC_VIEWSEAMSCHECK));

		mod->fnSetDisplayHiddenEdges(IsDlgButtonChecked(hWnd, IDC_UNWRAP_DISPLAYHIDDENEDGES));
		mod->openEdgeColor = iOpenEdgeColor->GetColor();
		mod->handleColor = iHandleColor->GetColor();
		mod->freeFormColor = iGizmoColor->GetColor();
		mod->sharedColor = iSharedColor->GetColor();

		mod->backgroundColor = iBackgroundColor->GetColor();

		mod->fnSetHitSize(spinHitSize->GetIVal());
		mod->fnSetTickSize(spinTickSize->GetIVal());


		mod->fnSetShowShared(IsDlgButtonChecked(hWnd, IDC_SHOWSHARED_CHECK));
		mod->fnSetBrightCenterTile(IsDlgButtonChecked(hWnd, IDC_UNWRAP_AFFECTCENTERTILE));
		mod->fnSetBlendToBack(IsDlgButtonChecked(hWnd, IDC_UNWRAP_BLENDTOBACK));

		mod->fnSetFilterMap(IsDlgButtonChecked(hWnd, IDC_UNWRAP_FILTERMAP));

		mod->SetShowImageAlpha(IsDlgButtonChecked(hWnd, IDC_UNWRAP_SHOWIMAGEALPHA));

		//new			
		mod->fnSetGridVisible(IsDlgButtonChecked(hWnd, IDC_SHOWGRID_CHECK));
		mod->fnSetGridSize(spinGridSize->GetFVal());
		mod->gridColor = iGridColor->GetColor();
		mod->fnSetTileGridVisible(IsDlgButtonChecked(hWnd, IDC_UNWRAP_SHOWTILEGRID_CHECK));
		mod->fnSetSelectionPreview(IsDlgButtonChecked(hWnd, IDC_UNWRAP_SEL_PREVIEW) ? true : false);

		mod->fnSetCheckerTiling(spinCheckerTiling->GetFVal());

		mod->fnSetNonSquareApplyBitmapRatio(IsDlgButtonChecked(hWnd, IDC_UNWRAP_NONSQUARE_APPLY_BITMAP_RATIO));

		mod->tileValid = FALSE;
		mod->RebuildDistCache();
		mod->InvalidateView();
	}

	if( updatePixelUnits )
	{   // Refresh transform spinners if display pixel units have changed
		mod->SetupTypeins();
	}

	return TRUE;
}

void UnwrapMod::PropDialog()
{
	if (!optionsDialogActive)
	{
		CreateDialogParam(
			hInstance,
			MAKEINTRESOURCE(IDD_UNWRAP_PROP),
			hDialogWnd,
			PropDlgProc,
			(LONG_PTR)this);
	}
}

//--- Named selection sets -----------------------------------------

int UnwrapMod::FindSet(TSTR &setName)
{
	if (ip && ip->GetSubObjectLevel() == 1)
	{
		for (int i = 0; i < namedVSel.Count(); i++)
		{
			if (setName == *namedVSel[i]) return i;
		}
		return -1;
	}
	else if (ip && ip->GetSubObjectLevel() == 2)
	{
		for (int i = 0; i < namedESel.Count(); i++)
		{
			if (setName == *namedESel[i]) return i;
		}
		return -1;
	}
	else if (ip && ip->GetSubObjectLevel() == 3)
	{
		for (int i = 0; i < namedSel.Count(); i++)
		{
			if (setName == *namedSel[i]) return i;
		}
		return -1;
	}
	return -1;
}

DWORD UnwrapMod::AddSet(TSTR &setName) {
	DWORD id = 0;
	TSTR *name = new TSTR(setName);

	if (ip && ip->GetSubObjectLevel() == 1)
	{
		namedVSel.Append(1, &name);
		BOOL found = FALSE;
		while (!found) {
			found = TRUE;
			for (int i = 0; i < idsV.Count(); i++) {
				if (idsV[i] != id) continue;
				id++;
				found = FALSE;
				break;
			}

		}
		idsV.Append(1, &id);
		return id;
	}
	else if (ip && ip->GetSubObjectLevel() == 2)
	{
		namedESel.Append(1, &name);
		BOOL found = FALSE;
		while (!found) {
			found = TRUE;
			for (int i = 0; i < idsE.Count(); i++) {
				if (idsE[i] != id) continue;
				id++;
				found = FALSE;
				break;
			}

		}
		idsE.Append(1, &id);
		return id;
	}
	else if (ip && ip->GetSubObjectLevel() == 3)
	{
		namedSel.Append(1, &name);
		BOOL found = FALSE;
		while (!found) {
			found = TRUE;
			for (int i = 0; i < ids.Count(); i++) {
				if (ids[i] != id) continue;
				id++;
				found = FALSE;
				break;
			}

		}
		ids.Append(1, &id);
		return id;
	}
	return (DWORD)-1;
}

void UnwrapMod::RemoveSet(TSTR &setName) {
	int i = FindSet(setName);
	if (i < 0) return;

	if (ip && ip->GetSubObjectLevel() == 1)
	{
		delete namedVSel[i];
		namedVSel.Delete(i, 1);
		idsV.Delete(i, 1);
	}
	else if (ip && ip->GetSubObjectLevel() == 2)
	{
		delete namedESel[i];
		namedESel.Delete(i, 1);
		idsE.Delete(i, 1);
	}
	else if (ip && ip->GetSubObjectLevel() == 3)
	{
		delete namedSel[i];
		namedSel.Delete(i, 1);
		ids.Delete(i, 1);
	}
}

void UnwrapMod::ClearSetNames()
{

	if (ip && ip->GetSubObjectLevel() == 1)
	{
		for (int j = 0; j < namedVSel.Count(); j++)
		{
			delete namedVSel[j];
			namedVSel[j] = NULL;
		}
	}

	else if (ip && ip->GetSubObjectLevel() == 2)
	{
		for (int j = 0; j < namedESel.Count(); j++)
		{
			delete namedESel[j];
			namedESel[j] = NULL;
		}
	}

	else if (ip && ip->GetSubObjectLevel() == 3)
	{
		for (int j = 0; j < namedSel.Count(); j++)
		{
			delete namedSel[j];
			namedSel[j] = NULL;
		}
	}
}

void UnwrapMod::ActivateSubSelSet(TSTR &setName) {
	ModContextList mcList;
	INodeTab nodes;
	int index = FindSet(setName);
	if (index < 0 || !ip) return;

	ip->GetModContexts(mcList, nodes);
	for (int i = 0; i < mcList.Count(); i++)
	{
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		BitArray *set = NULL;

		if (ip->GetSubObjectLevel() == 1)
		{

			set = meshData->vselSet.GetSet(idsV[index]);
			if (set)
			{
				meshData->SetGeomVertSel(*set);
				SyncTVFromGeomSelection(meshData);
				RebuildDistCache();
				InvalidateView();
				UpdateWindow(hDialogWnd);
			}
		}
		else if (ip->GetSubObjectLevel() == 2)
		{

			set = meshData->eselSet.GetSet(idsE[index]);
			if (set)
			{
				meshData->SetGeomEdgeSel(*set);
				SyncTVFromGeomSelection(meshData);
				InvalidateView();
				UpdateWindow(hDialogWnd);
			}
		}
		if (ip->GetSubObjectLevel() == 3)
		{

			set = meshData->fselSet.GetSet(ids[index]);
			if (set)
			{
				meshData->SetFaceSelectionByRef(*set);
				fnSyncTVSelection();
				InvalidateView();
				UpdateWindow(hDialogWnd);
			}
		}
	}

	nodes.DisposeTemporary();
	LocalDataChanged();
	ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::NewSetFromCurSel(TSTR &setName)
{
	if (!ip) return;
	ModContextList mcList;
	INodeTab nodes;
	DWORD id = 0;
	int index = FindSet(setName);
	if (index < 0) id = AddSet(setName);
	else id = ids[index];

	ip->GetModContexts(mcList, nodes);

	for (int i = 0; i < mcList.Count(); i++) {
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		BitArray *set = NULL;
		if (ip->GetSubObjectLevel() == 1)
		{
			if (index >= 0 && (set = meshData->vselSet.GetSet(id)) != NULL)
			{
				*set = meshData->GetGeomVertSel();//gvsel;
			}
			else meshData->vselSet.AppendSet(meshData->GetGeomVertSel(), id); 
		}
		else if (ip->GetSubObjectLevel() == 2)
		{
			if (index >= 0 && (set = meshData->eselSet.GetSet(id)) != NULL)
			{
				*set = meshData->GetGeomEdgeSel();//gesel;
			}
			else meshData->eselSet.AppendSet(meshData->GetGeomEdgeSel(), id);
		}
		else if (ip->GetSubObjectLevel() == 3)
		{
			if (index >= 0 && (set = meshData->fselSet.GetSet(id)) != NULL)
			{
				*set = meshData->GetFaceSel();//meshData->faceSel;
			}
			else meshData->fselSet.AppendSet(meshData->GetFaceSel(), id);
		}
	}

	nodes.DisposeTemporary();
}

void UnwrapMod::RemoveSubSelSet(TSTR &setName) {
	int index = FindSet(setName);
	if (index < 0 || !ip) return;

	ModContextList mcList;
	INodeTab nodes;
	ip->GetModContexts(mcList, nodes);

	DWORD id = ids[index];

	for (int i = 0; i < mcList.Count(); i++)
	{
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		//				if (theHold.Holding()) theHold.Put(new DeleteSetRestore(&meshData->fselSet,id));
		if (ip->GetSubObjectLevel() == 1)
			meshData->vselSet.RemoveSet(id);
		else if (ip->GetSubObjectLevel() == 2)
			meshData->eselSet.RemoveSet(id);
		else if (ip->GetSubObjectLevel() == 3)
			meshData->fselSet.RemoveSet(id);
	}

	//	if (theHold.Holding()) theHold.Put(new DeleteSetNameRestore(&(namedSel[nsl]),this,&(ids[nsl]),id));
	RemoveSet(setName);
	ip->ClearCurNamedSelSet();
	nodes.DisposeTemporary();
}

void UnwrapMod::SetupNamedSelDropDown() {
	if (!ip) return;

	ip->ClearSubObjectNamedSelSets();
	if (ip->GetSubObjectLevel() == 1)
	{
		for (int i = 0; i < namedVSel.Count(); i++)
		{
			ip->AppendSubObjectNamedSelSet(*namedVSel[i]);
		}
	}
	if (ip->GetSubObjectLevel() == 2)
	{
		for (int i = 0; i < namedESel.Count(); i++)
		{
			ip->AppendSubObjectNamedSelSet(*namedESel[i]);
		}
	}
	if (ip->GetSubObjectLevel() == 3)
	{
		for (int i = 0; i < namedSel.Count(); i++)
		{
			ip->AppendSubObjectNamedSelSet(*namedSel[i]);
		}
	}
}

int UnwrapMod::NumNamedSelSets()
{
	if (ip && ip->GetSubObjectLevel() == 1)
		return namedVSel.Count();
	else if (ip && ip->GetSubObjectLevel() == 2)
		return namedESel.Count();
	else if (ip && ip->GetSubObjectLevel() == 3)
		return namedSel.Count();
	return -1;
}

TSTR UnwrapMod::GetNamedSelSetName(int i)
{
	if (ip && ip->GetSubObjectLevel() == 1)
		return *namedVSel[i];
	if (ip && ip->GetSubObjectLevel() == 2)
		return *namedESel[i];
	if (ip && ip->GetSubObjectLevel() == 3)
		return *namedSel[i];
	return _T(" ");
}


void UnwrapMod::SetNamedSelSetName(int i, TSTR &newName)
{
	if (ip && ip->GetSubObjectLevel() == 1)
		*namedVSel[i] = newName;
	if (ip && ip->GetSubObjectLevel() == 2)
		*namedESel[i] = newName;
	if (ip && ip->GetSubObjectLevel() == 3)
		*namedSel[i] = newName;
}

void UnwrapMod::NewSetByOperator(TSTR &newName, Tab<int> &sets, int op) {
	ModContextList mcList;
	INodeTab nodes;

	if (!ip) return;
	DWORD id = AddSet(newName);
	//	if (theHold.Holding()) theHold.Put(new AppendSetNameRestore(this,&namedSel,&ids));

	BOOL delSet = TRUE;
	ip->GetModContexts(mcList, nodes);
	for (int i = 0; i < mcList.Count(); i++) {
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData)
			continue;

		BitArray bits;
		GenericNamedSelSetList *setList;

		setList = &meshData->fselSet;

		bits = (*setList)[sets[0]];

		for (int i = 1; i < sets.Count(); i++) {
			switch (op) {
			case NEWSET_MERGE:
				bits |= (*setList)[sets[i]];
				break;

			case NEWSET_INTERSECTION:
				bits &= (*setList)[sets[i]];
				break;

			case NEWSET_SUBTRACT:
				bits &= ~((*setList)[sets[i]]);
				break;
			}
		}
		if (bits.NumberSet()) delSet = FALSE;

		setList->AppendSet(bits, id);
		//		if (theHold.Holding()) theHold.Put(new AppendSetRestore(setList));
	}
	if (delSet) RemoveSubSelSet(newName);
}

void UnwrapMod::LocalDataChanged() {
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip && editMod == this) {
		//	SetNumSelLabel();
		ip->ClearCurNamedSelSet();
	}
}

void UnwrapMod::SetNumSelLabel() {
	TSTR buf;
	int num = 0, which;

	if (!hParams || !ip) return;

	ModContextList mcList;
	INodeTab nodes;

	ip->GetModContexts(mcList, nodes);
	for (int i = 0; i < mcList.Count(); i++) {
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		BitArray fsel = meshData->GetFaceSel();
		num += fsel.NumberSet();
		if (fsel.NumberSet() == 1) {
			for (which = 0; which < fsel.GetSize(); which++)
				if (fsel[which])
					break;
		}
	}

}

int UnwrapMod::NumSubObjTypes()
{
	//return based on where
	return subObjCount;
}

bool UnwrapMod::GetLivePeelModeEnabled()
{
	return fnGetMapMode() == LSCMMAP;
}

ISubObjType *UnwrapMod::GetSubObjType(int i)
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		SOT_SelFace.SetName(GetString(IDS_PW_SELECTFACE));
		SOT_SelVerts.SetName(GetString(IDS_PW_SELECTVERTS));
		SOT_SelEdges.SetName(GetString(IDS_PW_SELECTEDGES));
		SOT_SelGizmo.SetName(GetString(IDS_PW_SELECTGIZMO));
		//		SOT_FaceMap.SetName(GetString(IDS_PW_FACEMAP));
		//		SOT_Planar.SetName(GetString(IDS_PW_PLANAR));
	}

	switch (i)
	{
	case 0:
		return &SOT_SelVerts;

	case 1:
		return &SOT_SelEdges;
	case 2:
		return &SOT_SelFace;
	case 3:
		return &SOT_SelGizmo;
	}

	return NULL;
}



//Pelt
void UnwrapMod::HitGeomEdgeData(MeshTopoData *ld, Tab<UVWHitData> &hitEdges, GraphicsWindow *gw, HitRegion hr)
{
	hitEdges.ZeroCount();

	gw->setHitRegion(&hr);
	gw->setRndLimits((GW_PICK)& ~GW_ILLUM);

	for (int i = 0; i < ld->GetNumberGeomEdges()/*TVMaps.gePtrList.Count()*/; i++)
	{
		if (!(ld->GetGeomEdgeHidden(i)))
		{
			if (ld->GetPatch())
			{
				int a, b;
				a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
				b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;

				Point3 avec, bvec;

				int faceIndex = ld->GetGeomEdgeConnectedFace(i, 0);
				int deg = ld->GetFaceDegree(faceIndex);
				PatchMesh *patch = ld->GetPatch();
				for (int k = 0; k < deg; k++)
				{
					int testa = k;
					int testb = (k + 1) % deg;
					int pa = ld->GetPatch()->patches[faceIndex].v[testa];
					int pb = ld->GetPatch()->patches[faceIndex].v[testb];
					if ((pa == a) && (pb == b))
					{
						int va = patch->patches[faceIndex].vec[testa * 2];
						int vb = patch->patches[faceIndex].vec[testa * 2 + 1];
						avec = patch->vecs[va].p;
						bvec = patch->vecs[vb].p;
					}
					else if ((pb == a) && (pa == b))
					{
						int va = patch->patches[faceIndex].vec[testa * 2];
						int vb = patch->patches[faceIndex].vec[testa * 2 + 1];
						avec = patch->vecs[va].p;
						bvec = patch->vecs[vb].p;
					}
				}

				//				avec = ld->GetGeomVert(vb);//TVMaps.geomPoints[vb]; <- is that right?
				//				bvec = ld->GetGeomVert(va);//TVMaps.geomPoints[va];
				Point3 pa, pb;
				pa = ld->GetGeomVert(a);//TVMaps.geomPoints[a];
				pb = ld->GetGeomVert(b);//TVMaps.geomPoints[b];

				Spline3D sp;
				SplineKnot ka(KTYPE_BEZIER_CORNER, LTYPE_CURVE, pa, avec, avec);
				SplineKnot kb(KTYPE_BEZIER_CORNER, LTYPE_CURVE, pb, bvec, bvec);
				sp.NewSpline();
				sp.AddKnot(ka);
				sp.AddKnot(kb);
				sp.SetClosed(0);
				sp.InvalidateGeomCache();
				Point3 ip1, ip2;
				Point3 plist[3];
				//										Draw3dEdge(gw,size, plist[0], plist[1], c);
				gw->clearHitCode();
				for (int k = 0; k < 8; k++)
				{
					float per = k / 7.0f;
					ip1 = sp.InterpBezier3D(0, per);
					if (k > 0)
					{
						plist[0] = ip1;
						plist[1] = ip2;
						gw->segment(plist, 1);
					}
					ip2 = ip1;
				}
				if (gw->checkHitCode())
				{
					UVWHitData d;
					d.index = i;
					d.dist = gw->getHitDistance();
					hitEdges.Append(1, &d, 500);
				}

			}
			else
			{

				Point3 plist[3];
				int a, b;
				a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
				b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;

				plist[0] = ld->GetGeomVert(a);//TVMaps.geomPoints[a];
				plist[1] = ld->GetGeomVert(b);//TVMaps.geomPoints[b];

				gw->clearHitCode();
				gw->segment(plist, 1);
				if (gw->checkHitCode())
				{
					UVWHitData d;
					d.index = i;
					d.dist = gw->getHitDistance();
					hitEdges.Append(1, &d, 500);
				}
			}
		}
	}


}


void UnwrapMod::HitGeomVertData(MeshTopoData *ld, Tab<UVWHitData> &hitVerts, GraphicsWindow *gw, HitRegion hr)
{
	hitVerts.ZeroCount();

	gw->setHitRegion(&hr);
	gw->setRndLimits((GW_PICK)& ~GW_ILLUM);

	for (int i = 0; i < ld->GetNumberGeomVerts(); i++)
	{
		Point3 p = ld->GetGeomVert(i);//TVMaps.geomPoints[i];

		gw->clearHitCode();

		gw->marker(&p, POINT_MRKR);
		if (gw->checkHitCode())
		{
			UVWHitData d;
			d.index = i;
			d.dist = gw->getHitDistance();
			hitVerts.Append(1, &d, 500);
		}
	}
}


void UnwrapMod::Move(
	TimeValue t, Matrix3& partm, Matrix3& tmAxis,
	Point3& val, BOOL localOrigin)
{

	assert(tmControl);


	if (fnGetMapMode() != SPLINEMAP)
	{
		SetXFormPacket pckt(val, Matrix3(1)/*partm*/, tmAxis);
		tmControl->SetValue(t, &pckt, TRUE, CTRL_RELATIVE);
	}
	else if (fnGetMapMode() == SPLINEMAP)
	{
		mSplineMap.MoveSelectedCrossSections(val);
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap6.splineMap_moveSelectedCrossSection"), 1, 0,
			mr_point3, &val);

		if (fnGetConstantUpdate())
			fnSplineMap();

		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());
		InvalidateView();
	}

	if (fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		{
			ApplyGizmo();
		}
	}

}


void UnwrapMod::Rotate(
	TimeValue t, Matrix3& partm, Matrix3& tmAxis,
	Quat& val, BOOL localOrigin)
{
	if (fnGetMapMode() != SPLINEMAP)
	{
		assert(tmControl);
		SetXFormPacket pckt(val, localOrigin, Matrix3(1), tmAxis);
		tmControl->SetValue(t, &pckt, TRUE, CTRL_RELATIVE);
	}
	else if (fnGetMapMode() == SPLINEMAP)
	{




		Tab<int> selSplines;
		Tab<int> selCrossSections;
		mSplineMap.GetSelectedCrossSections(selSplines, selCrossSections);

		if (selSplines.Count() > 0)
		{
			Quat localQuat = val;

			mSplineMap.RotateSelectedCrossSections(localQuat);

			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap6.splineMap_rotateSelectedCrossSection"), 1, 0,
				mr_quat, &localQuat);

			if (fnGetConstantUpdate())
				fnSplineMap();

			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			if (ip) ip->RedrawViews(ip->GetTime());
			InvalidateView();
		}

	}

	if (fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}
}

void UnwrapMod::Scale(
	TimeValue t, Matrix3& partm, Matrix3& tmAxis,
	Point3& val, BOOL localOrigin)
{

	assert(tmControl);
	if (fnGetMapMode() != SPLINEMAP)
	{
		SetXFormPacket pckt(val, localOrigin, Matrix3(1), tmAxis);
		tmControl->SetValue(t, &pckt, TRUE, CTRL_RELATIVE);
	}
	else if (fnGetMapMode() == SPLINEMAP)
	{
		Matrix3 scaleTM = tmAxis;

		Tab<int> selSplines;
		Tab<int> selCrossSections;
		mSplineMap.GetSelectedCrossSections(selSplines, selCrossSections);

		if (selSplines.Count() > 0)
		{
			SplineCrossSection *selCrossSection = mSplineMap.GetCrossSection(selSplines[0], selCrossSections[0]);


			Matrix3 crossSectionTM = selCrossSection->mTM;
			crossSectionTM.NoScale();
			crossSectionTM.NoTrans();

			Matrix3 toWorldSpace = crossSectionTM;


			Point3 xScale(1.0f, 0.0f, 0.0f);
			Point3 yScale(0.0f, 1.0f, 0.0f);
			xScale = xScale * toWorldSpace;
			yScale = yScale * toWorldSpace;
			xScale = VectorTransform(xScale, Inverse(scaleTM)) * val;
			yScale = VectorTransform(yScale, Inverse(scaleTM)) * val;


			float xs = Length(xScale);
			float xy = Length(yScale);


			Point2 scale(xs, xy);
			if (scale.x < 0.0f)
				scale.x *= -1.0f;
			if (scale.y < 0.0f)
				scale.y *= -1.0f;
			mSplineMap.ScaleSelectedCrossSections(scale);
			TSTR emitString;
			emitString.printf(_T("$.modifiers[#unwrap_uvw].unwrap6.splineMap_scaleSelectedCrossSection [%f,%f]"), scale.x, scale.y);
			macroRecorder->ScriptString(emitString);

			if (fnGetConstantUpdate())
				fnSplineMap();

			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			if (ip) ip->RedrawViews(ip->GetTime());
			InvalidateView();
		}
	}
	if (fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}
}

void UnwrapMod::TransformHoldingStart(TimeValue t)
{
	if (fnGetMapMode() == SPLINEMAP)
	{
		//hold our cross section data
		mSplineMap.HoldData();
	}
}

void UnwrapMod::TransformHoldingFinish(TimeValue t)
{
	if (!fnGetConstantUpdate())
	{
		if (fnGetMapMode() == SPLINEMAP)
		{
			fnSplineMap();
		}
		else if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}
	Matrix3 macroTM = *fnGetGizmoTM();
	TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap5.setGizmoTM"));
	macroRecorder->FunctionCall(mstr, 1, 0, mr_matrix3, &macroTM);

}

void UnwrapMod::EnableMapButtons(BOOL enable)
{

	mUIManager.Enable(ID_PELT_MAP, enable);
	mUIManager.Enable(ID_PLANAR_MAP, enable);
	mUIManager.Enable(ID_CYLINDRICAL_MAP, enable);
	mUIManager.Enable(ID_SPHERICAL_MAP, enable);
	mUIManager.Enable(ID_BOX_MAP, enable);
	mUIManager.Enable(ID_SPLINE_MAP, enable);
	mUIManager.Enable(ID_UNFOLD_MAP, enable);

	if (mapMapMode != NOMAP)
		enable = FALSE;

	mUIManager.Enable(ID_QMAP, enable);
	mUIManager.Enable(ID_QUICKMAP_DISPLAY, enable);
	mUIManager.Enable(ID_QUICKMAP_ALIGN, enable);

}

void UnwrapMod::EnableAlignButtons(BOOL enable)
{
	mUIManager.Enable(ID_MAPPING_ALIGNX, enable);
	mUIManager.Enable(ID_MAPPING_ALIGNY, enable);
	mUIManager.Enable(ID_MAPPING_ALIGNZ, enable);
	mUIManager.Enable(ID_MAPPING_NORMALALIGN, enable);

	mUIManager.Enable(ID_MAPPING_FIT, enable);
	mUIManager.Enable(ID_MAPPING_ALIGNTOVIEW, enable);
	mUIManager.Enable(ID_MAPPING_CENTER, enable);

	ICustButton* iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_RESET));
	if (iButton) {
		iButton->Enable(enable);
		ReleaseICustButton(iButton);
	}

}



void UnwrapMod::EnableFaceSelectionUI(BOOL enable)
{
	if (ip == NULL) return;

	mUIManager.Enable(ID_PLANARSPIN, enable);
	// update the UI status of material IDs controls
	if (iSelMatIDSpinnerEdit)
		iSelMatIDSpinnerEdit->Enable(enable);
	if (iSetMatIDSpinnerEdit)
		iSetMatIDSpinnerEdit->Enable(enable);
	if (hMatIdParams)
	{
		EnableWindow(GetDlgItem(hMatIdParams, IDC_STATIC), enable);
		EnableWindow(GetDlgItem(hMatIdParams, IDC_UNWRAP_SELECT_MAT_ID), enable);
	}

	mUIManager.Enable(ID_SMGRPSPIN, enable);
	mUIManager.Enable(ID_SELECTBY_SMGRP, enable);
	mUIManager.Enable(ID_PLANARMODE, enable);
}

void UnwrapMod::EnableEdgeSelectionUI(BOOL enable)
{
	mUIManager.Enable(ID_EDGERINGSELECTION, enable);
	mUIManager.Enable(ID_EDGELOOPSELECTION, enable);
	mUIManager.Enable(ID_POINT_TO_POINT_SEL, enable);

	mUIManager.Enable(ID_PELT_EDITSEAMS, enable);
	mUIManager.Enable(ID_PELT_POINTTOPOINTSEAMS, enable);
	mUIManager.Enable(ID_PW_SELTOSEAM2, enable);
}

void UnwrapMod::EnableSubSelectionUI(BOOL enable)
{
	mUIManager.Enable(ID_GEOMEXPANDFACESEL, enable);
	mUIManager.Enable(ID_GEOMCONTRACTFACESEL, enable);
	mUIManager.Enable(ID_IGNOREBACKFACE, enable);
	mUIManager.Enable(ID_GEOM_ELEMENT, enable);

	mUIManager.Enable(ID_MIRROR_SEL_X_AXIS, enable);
	mUIManager.Enable(ID_MIRROR_SEL_Y_AXIS, enable);
	mUIManager.Enable(ID_MIRROR_SEL_Z_AXIS, enable);
	mUIManager.Enable(ID_MIRROR_GEOM_SELECTION, enable);
	mUIManager.Enable(ID_PAINT_MOVE_BRUSH, enable);
	mUIManager.Enable(ID_RELAX_MOVE_BRUSH, enable);
}

void UnwrapMod::fnSetWindowXOffset(int offset, bool applyUIScaling)
{
	if (applyUIScaling)
		offset = MaxSDK::UIScaled(offset);

	xWindowOffset = offset;

	WINDOWPLACEMENT floaterPos;
	floaterPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hDialogWnd, &floaterPos);

	if (floaterPos.showCmd == SW_MAXIMIZE)
	{
		Rect rect;
		GetWindowRect(hDialogWnd, &rect);

		SetWindowPos(hDialogWnd, NULL, rect.left, rect.top, maximizeWidth - 2 - xWindowOffset, maximizeHeight - yWindowOffset, SWP_NOZORDER);
		SizeDlg();
	}

}
void UnwrapMod::fnSetWindowYOffset(int offset, bool applyUIScaling)
{
	if (applyUIScaling)
		offset = MaxSDK::UIScaled(offset);

	yWindowOffset = offset;

	WINDOWPLACEMENT floaterPos;
	floaterPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hDialogWnd, &floaterPos);

	if (floaterPos.showCmd == SW_MAXIMIZE)
	{
		Rect rect;
		GetWindowRect(hDialogWnd, &rect);
		SetWindowPos(hDialogWnd, NULL, rect.left, rect.top, maximizeWidth - 2 - xWindowOffset, maximizeHeight - yWindowOffset, SWP_NOZORDER);
		SizeDlg();
	}

}

void UnwrapMod::SetCheckerMapChannel()
{
	Mtl *checkMat = GetCheckerMap();
	if (checkMat)
	{
		Texmap *checkMap = NULL;
		checkMap = (Texmap *)checkMat->GetSubTexmap(1);
		if (checkMap)
		{
			StdUVGen *uvGen = (StdUVGen*)checkMap->GetTheUVGen();
			uvGen->SetMapChannel(channel);
		}

	}
}


BOOL UnwrapMod::GetShowImageAlpha()
{
	BOOL show = 0;
	pblock->GetValue(unwrap_showimagealpha, 0, show, FOREVER);
	return show;
}

void UnwrapMod::SetShowImageAlpha(BOOL show)
{
	pblock->SetValue(unwrap_showimagealpha, 0, show);
	SetupImage();
}




int UnwrapMod::GetQMapAlign()
{
	int align = 0;
	pblock->GetValue(unwrap_qmap_align, 0, align, FOREVER);
	return align;
}

void UnwrapMod::SetQMapAlign(int align)
{
	if (theHold.Holding())
		theHold.Put(new UnwrapMapAttributesRestore(this));
	pblock->SetValue(unwrap_qmap_align, 0, align);

	mUIManager.SetFlyOut(ID_QUICKMAP_ALIGN, align, FALSE);
	bMapGizmoTMDirty = TRUE;
}

BOOL UnwrapMod::GetQMapPreview()
{
	BOOL preview = TRUE;
	pblock->GetValue(unwrap_qmap_preview, 0, preview, FOREVER);
	return preview;
}
void UnwrapMod::SetQMapPreview(BOOL preview)
{
	if (theHold.Holding())
		theHold.Put(new UnwrapMapAttributesRestore(this));
	pblock->SetValue(unwrap_qmap_preview, 0, preview);

	mUIManager.UpdateCheckButtons();
}

int UnwrapMod::GetMeshTopoDataCount()
{

	return mMeshTopoData.Count();
}

MeshTopoData *UnwrapMod::GetMeshTopoData(int index)
{
	if ((index < 0) || (index >= mMeshTopoData.Count()))
	{
		DbgAssert(0);
		return NULL;
	}
	return mMeshTopoData[index];
}

INode *UnwrapMod::GetMeshTopoDataNode(int index)
{
	if ((index < 0) || (index >= mMeshTopoData.Count()))
	{
		DbgAssert(0);
		return NULL;
	}
	return mMeshTopoData.GetNode(index);
}


int UnwrapMod::ControllerAdd()
{
	int controlIndex = -1;
	for (int i = 0; i < mUVWControls.cont.Count(); i++)
	{
		if (mUVWControls.cont[i] == NULL)
		{
			controlIndex = i;
			i = mUVWControls.cont.Count();
		}
	}
	if (controlIndex == -1)
	{
		controlIndex = mUVWControls.cont.Count();
		mUVWControls.cont.SetCount(controlIndex + 1);
	}
	mUVWControls.cont[controlIndex] = NULL;
	ReplaceReference(controlIndex + 11 + 100, NewDefaultPoint3Controller());
	return controlIndex;
}
int UnwrapMod::ControllerCount()
{
	return mUVWControls.cont.Count();
}

void UnwrapMod::UpdateControllers(TimeValue t, MeshTopoData* pData)
{
	if (pData == nullptr) return;
	//loop through our tvs looking for ones with controlls on them
	for (int i = 0; i < pData->GetNumberTVVerts(); i++)
	{
		//push the data off the control into our tv data
		int controllerIndex = pData->GetTVVertControlIndex(i);
		Point3 p;
		if (controllerIndex != -1)
		{
			Control* cont = Controller(controllerIndex);
			if (cont)
			{
				cont->GetValue(t, &p, FOREVER);
				pData->SetTVVert(t, i, p);
			}
		}
	}
}

Control* UnwrapMod::Controller(int index)
{
	if ((index < 0) || (index >= mUVWControls.cont.Count()))
	{
		DbgAssert(0);
		return NULL;
	}
	return  mUVWControls.cont[index];
}

void UnwrapMod::BuildAllFilters()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->BuildAllFilters(filterSelectedFaces != 0);
	}
}

MeshTopoData *UnwrapMod::GetMeshTopoData(INode *node)
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		if (mMeshTopoData.GetNode(ldID) == node)
			return mMeshTopoData[ldID];
	}
	return NULL;
}

void UnwrapMod::CleanUpGeoSelection(MeshTopoData *ld)
{
	//	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		//		MeshTopoData *ld = mMeshTopoData[ldID];
		BOOL hasSelection = ld->HasIncomingFaceSelection();

		if (hasSelection)
		{
			int numFaces = ld->GetNumberFaces();
			BitArray fsel = ld->GetIncomingFaceSelection();
			//loop through the face
			for (int i = 0; i < numFaces; i++)
			{
				BOOL faceSelected = ld->GetFaceSelected(i);
				BOOL faceDead = ld->GetFaceDead(i);
				if (faceDead && faceSelected)
					ld->SetFaceSelected(i, FALSE);

				int degree = ld->GetFaceDegree(i);
				for (int j = 0; j < degree; j++)
				{
					int index = ld->GetFaceGeomVert(i, j);
					if (ld->GetGeomVertSelected(index) && faceDead)
						ld->SetGeomVertSelected(index, FALSE, mirrorGeoSelectionStatus);

					if (ld->GetFaceCurvedMaping(i) && ld->GetFaceHasVectors(i))
					{
						int index = ld->GetFaceGeomHandle(i, j * 2);
						if (ld->GetGeomVertSelected(index) && faceDead)
							ld->SetGeomVertSelected(index, FALSE, mirrorGeoSelectionStatus);
						index = ld->GetFaceGeomHandle(i, j * 2 + 1);
						if (ld->GetGeomVertSelected(index) && faceDead)
							ld->SetGeomVertSelected(index, FALSE, mirrorGeoSelectionStatus);
						if (ld->GetFaceHasInteriors(i))
						{
							index = ld->GetFaceGeomInterior(i, j);
							if (ld->GetGeomVertSelected(index) && faceDead)
								ld->SetGeomVertSelected(index, FALSE, mirrorGeoSelectionStatus);
						}
					}

				}
			}
		}
	}
}

TSTR    UnwrapMod::GetMacroStr(const TCHAR *macroStr)
{
	TSTR macroCommand;
	if (mMeshTopoData.Count() < 2)
	{
		macroCommand.printf(_T("$.%s"), macroStr);
		return macroCommand;
	}
	else
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			INode *node = mMeshTopoData.GetNode(0);
			if (node)
			{
				macroCommand.printf(_T("$%s.%s"), node->GetName(), macroStr);
				return macroCommand;
			}
		}
	}

	macroCommand.printf(_T("$.%s"), macroStr);
	return macroCommand;
}

void UnwrapMod::GetFaceSelectionFromMesh(ObjectState *os, ModContext &mc, TimeValue t)
{
	TriObject *tobj = (TriObject*)os->obj;
	MeshTopoData *d = (MeshTopoData*)mc.localData;
	if (d)
	{
		d->SetFaceSel(tobj->GetMesh().faceSel);
	}
}

void UnwrapMod::GetFaceSelectionFromPatch(ObjectState *os, ModContext &mc, TimeValue t)
{
	PatchObject *pobj = (PatchObject*)os->obj;
	MeshTopoData *d = (MeshTopoData*)mc.localData;
	if (d)
	{
		d->SetFaceSel(pobj->patch.patchSel);
	}
}

void UnwrapMod::GetFaceSelectionFromMNMesh(ObjectState *os, ModContext &mc, TimeValue t)
{
	PolyObject *tobj = (PolyObject*)os->obj;

	MeshTopoData *d = (MeshTopoData*)mc.localData;
	if (d)
	{
		BitArray s;
		tobj->GetMesh().getFaceSel(s);
		d->SetFaceSel(s);
	}
}

void UnwrapMod::HoldCancelBuffer()
{
	mCancelBuffer.SetCount(GetMeshTopoDataCount());
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData *ld = GetMeshTopoData(ldID);
		mCancelBuffer[ldID] = new TVertAndTFaceRestore(ld);
	}
}

void UnwrapMod::RestoreCancelBuffer()
{
	theHold.Begin();
	HoldPointsAndFaces();
	theHold.Accept(GetString(IDS_PW_CANCEL));

	for (int ldID = 0; ldID < mCancelBuffer.Count(); ldID++)
	{
		mCancelBuffer[ldID]->Restore(FALSE);
	}
}

void UnwrapMod::FreeCancelBuffer()
{
	for (int ldID = 0; ldID < mCancelBuffer.Count(); ldID++)
	{
		delete mCancelBuffer[ldID];
	}
	mCancelBuffer.SetCount(0);
}

void UnwrapMod::PlugControllers()
{
	if (!Animating())
		return;

	TransferSelectionStart();
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData *ld = GetMeshTopoData(ldID);
		if (ld != NULL)
		{
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (ld->GetTVVertSelected(i))
				{
					int controlIndex = ld->GetTVVertControlIndex(i);
					if (controlIndex == -1)
					{
						Point3 p = ld->GetTVVert(i);
						//create a new control
						controlIndex = ControllerAdd();
						//remember its id
						ld->SetTVVertControlIndex(i, controlIndex);
						//set its value
						Controller(controlIndex)->SetValue(0, p);
					}
				}
			}
		}
	}
	TransferSelectionEnd(FALSE, FALSE);
}

//we need to override the default bounding box which is the object or subobject selection
//flowing up the stack
void UnwrapNodeDisplayCallback::AddNodeCallbackBox(TimeValue t, INode *node, ViewExp *vpt, Box3& box, Object *pObj)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here?
		DbgAssert(!_T("Doing HitTest() on invalid viewport!"));
		return;
	}

	if (mod && mod->ip && (mod->ip->GetSubObjectLevel() > 0))
	{
		if (mod->ip->GetExtendedDisplayMode()& EXT_DISP_ZOOMSEL_EXT)
		{
			Interface *ip = GetCOREInterface();
			//if we are in a mapping mode use the map gizmo bounds
			if ((ip->GetSubObjectLevel() == 3) && (mod->fnGetMapMode() != NOMAP))
			{
				box = mod->gizmoBounds;
			}
			//otherwise just use the selection set bounds
			else if (mod->AnyThingSelected())
				box = mBounds;
		}

	}
}

void UnwrapMod::UpdateViewAndModifier()
{
	TimeValue t = GetCOREInterface()->GetTime();
	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(t);
	InvalidateView();
}

BOOL UnwrapMod::MapModesThatCanSelect()
{
	if ((fnGetMapMode() == NOMAP) || (fnGetMapMode() == SPLINEMAP) || (fnGetMapMode() == UNFOLDMAP) || (fnGetMapMode() == LSCMMAP))
		return TRUE;
	return FALSE;
}

UnwrapCustomUI*   UnwrapMod::GetUIManager()
{
	return &mUIManager;
}

bool UnwrapMod::GetDisplayPixelUnits() const
{
	if( (scalePixelUnitsU==0) || (scalePixelUnitsV==0) )
		return false; // never display pixel units unless valid scaling is set
	return (displayPixelUnits != FALSE);
}
void UnwrapMod::SetDisplayPixelUnits( bool bDisplayPixelUnits, bool update )
{
	this->displayPixelUnits = bDisplayPixelUnits;
	if( update )
	{
		SetupTypeins();
		InvalidateView();
	}
}
int UnwrapMod::GetScalePixelUnits( int axis )
{
	switch( axis )
	{
	case 1:	return scalePixelUnitsU;
	case 2:	return scalePixelUnitsV;
	}
	return 0; // default
}
void UnwrapMod::SetScalePixelUnits( int s, int axis )
{
	switch( axis )
	{
	case 1:	scalePixelUnitsU = s; break;
	case 2:	scalePixelUnitsV = s; break;
	}
}
bool UnwrapMod::IsSpinnerPixelUnits(int spinnerID, int* axis)
{
	// Only support pixel units in Move mode, not Rotate or Scale
	bool isMove = (mode == ID_MOVE);
	bool isFreeform = (mode == ID_FREEFORMMODE);
	bool isFreeformMove = ((freeFormSubMode == ID_MOVE) || (freeFormSubMode == ID_SELECT));
	bool isValidMode = (isMove || (isFreeform && isFreeformMove));

	if( GetDisplayPixelUnits() )
	{
		bool checkMode = false;
		int axisVal = 0;
		int& axisRef = (axis==NULL? axisVal:(*axis));
		axisRef = 0;

		switch( spinnerID )
		{
		case IDC_UNWRAP_SPACINGSPIN:	axisRef = 1; break;
		case ID_WELDTHRESH_SPINNER:		axisRef = 1; break;
		case ID_PACK_PADDINGSPINNER:	axisRef = 1; break;
		case ID_SPINNERU:				axisRef = 1, checkMode=true; break;
		case ID_SPINNERV:				axisRef = 2, checkMode=true; break;
		}
		if( (!checkMode) || isValidMode )
			return (axisRef!=0); // false by default
		// else required to check mode validity, and mode isn't valid, so fall through...
	}
	return false; // default
}

void UnwrapMod::WheelZoom(int xPos, int yPos, int delta)
{
	RECT rect;
	GetWindowRect(hView, &rect);
	xPos = xPos - rect.left;
	yPos = yPos - rect.top;

	float z;
	if (delta < 0)
		z = (1.0f / (1.0f - 0.0025f*delta));
	else z = (1.0f + 0.0025f*delta);
	zoom = zoom * z;

	if (middleMode->inDrag)
	{
		xscroll = xscroll*z;
		yscroll = yscroll*z;
	}
	else
	{

		Rect rect;
		GetClientRect(hView, &rect);
		int centerX = (rect.w() - 1) / 2 - xPos;
		int centerY = (rect.h() - 1) / 2 - yPos;

		xscroll = (xscroll + centerX)*z;
		yscroll = (yscroll + centerY)*z;


		xscroll -= centerX;
		yscroll -= centerY;
	}

	middleMode->ResetInitialParams();

	//need reset When a MMB Scroll in Pan Mode.
	if (bCustomizedPanMode)
	{
		panMode->ResetInitialParams();
	}

	InvalidateView();

	if (MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		//Under the Nitrous mode,let the flag keeps true that will not trigger the data of vertex,edge and face update.
		//The world matrix will use the new zoom and scroll value to update the display result.
		viewValid = TRUE;
	}
	else
	{
		//Under the legacy mode, need to dirty this flag to trigger the background redraw.
		tileValid = FALSE;
	}
}

bool UnwrapMod::CanEnterCustomizedMode()
{
	bool bCustomizedPan = false;
	bool bCustomizedZoom = false;
	GetMatchResult(bCustomizedPan, bCustomizedZoom);

	if (bCustomizedPan && !bCustomizedPanMode)
		return true;

	if (bCustomizedZoom && !bCustomizedZoomMode)
		return true;

	return false;
}

void UnwrapMod::EnterCustomizedMode(UnwrapMod* pMod)
{
	bool bCustomizedPan = false;
	bool bCustomizedZoom = false;
	GetMatchResult(bCustomizedPan, bCustomizedZoom);

	if (bCustomizedPan && !bCustomizedPanMode)
	{
		if (iOriginalMode < 0)
		{
			iOriginalMode = mode;
			iOriginalOldMode = oldMode;
		}

		if (mode != ID_PAN && pMod)
			pMod->SetMode(ID_PAN);

		bCustomizedZoomMode = false;
		bCustomizedPanMode = true;
		return;
	}

	if (bCustomizedZoom && !bCustomizedZoomMode)
	{
		if (iOriginalMode < 0)
		{
			iOriginalMode = mode;
			iOriginalOldMode = oldMode;
		}
			
		if (mode != ID_ZOOMTOOL && pMod)
			pMod->SetMode(ID_ZOOMTOOL);

		bCustomizedZoomMode = true;
		bCustomizedPanMode = false;
		return;
	}
}

bool UnwrapMod::CanExitCustomizedMode()
{
	if (iOriginalMode > 0 && (bCustomizedZoomMode || bCustomizedPanMode))
	{
		bool bCustomizedPan = true;
		bool bCustomizedZoom = true;
		GetMatchResult(bCustomizedPan, bCustomizedZoom);
		if (bCustomizedPan == false && bCustomizedZoom == false)
			return true;
	}

	return false;
}

void UnwrapMod::ExitCustomizedMode(UnwrapMod* pMod)
{
	if (iOriginalMode > 0 && (bCustomizedZoomMode || bCustomizedPanMode))
	{
		bool bCustomizedPan = false;
		bool bCustomizedZoom = false;
		GetMatchResult(bCustomizedPan, bCustomizedZoom);

		if (bCustomizedPan == false && bCustomizedZoom == false)
		{
			if (mode != iOriginalMode && pMod)
				pMod->SetMode(iOriginalMode);
			
			oldMode = iOriginalOldMode>0?iOriginalOldMode:ID_FREEFORMMODE;

			iOriginalMode = -1;
			iOriginalOldMode = -1;
			bCustomizedZoomMode = false;
			bCustomizedPanMode = false;
		}
	}
}

int UnwrapMod::GetButtonsStates()
{
	int buttonStates = 0;
	if (GetAsyncKeyState(VK_MENU) & 0x8000)
	{
		buttonStates |= MOUSE_ALT;
	}
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		buttonStates |= MOUSE_CTRL;
	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		buttonStates |= MOUSE_SHIFT;
	}

	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		buttonStates |= MOUSE_LBUTTON;
	}
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
	{
		buttonStates |= MOUSE_RBUTTON;
	}
	if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
	{
		buttonStates |= MOUSE_MBUTTON;
	}
	return buttonStates;
}

int UnwrapMod::KeyOption2ButtonFlag(MaxSDK::CUI::KeyOption modifierKeys)
{
	using namespace MaxSDK::CUI;
	int flag = 0;
	if (modifierKeys & EKey_Shift)
	{
		flag |= MOUSE_SHIFT;
	}
	if (modifierKeys & EKey_Ctrl)
	{
		flag |= MOUSE_CTRL;
	}
	if (modifierKeys & EKey_Alt)
	{
		flag |= MOUSE_ALT;
	}
	return flag;
}

int UnwrapMod::MouseButtonOption2ButtonFlag(MaxSDK::CUI::MouseButtonOption mouseButtons)
{
	using namespace MaxSDK::CUI;
	int flag = 0;
	if (mouseButtons & EMouseButton_Left)
	{
		flag |= MOUSE_LBUTTON;
	}
	if (mouseButtons & EMouseButton_Right)
	{
		flag |= MOUSE_RBUTTON;
	}
	if (mouseButtons & EMouseButton_Middle)
	{
		flag |= MOUSE_MBUTTON;
	}
	return flag;
}

void UnwrapMod::GetMatchResult(bool &bCustomizedPan, bool &bCustomizedZoom)
{
	bCustomizedPan = false;
	bCustomizedZoom = false;

	using namespace MaxSDK::CUI;
	KeyOption ePanKey = EKey_Null;
	KeyOption eZoomKey = EKey_Null;
	MouseButtonOption ePanButton = EMouseButton_Null;
	MouseButtonOption eZoomButton = EMouseButton_Null;
	GetOperationShortcut(EOperationMode_Pan, ePanKey, ePanButton);
	GetOperationShortcut(EOperationMode_Zoom, eZoomKey, eZoomButton);

	int panFlag = KeyOption2ButtonFlag(ePanKey) | MouseButtonOption2ButtonFlag(ePanButton);
	int zoomFlag = KeyOption2ButtonFlag(eZoomKey) | MouseButtonOption2ButtonFlag(eZoomButton);

	int ButtonFlags = GetButtonsStates();
	//Try the exact match firstly
	if (ButtonFlags == panFlag)
	{
		bCustomizedPan = true;
		ButtonFlags ^= panFlag;
	}
	else if (ButtonFlags == zoomFlag)
	{
		bCustomizedZoom = true;
		ButtonFlags ^= zoomFlag;
	}
	else
	{
		//if no exact match, use & instead of ==
		if ((ButtonFlags & zoomFlag) == zoomFlag)
		{
			bCustomizedZoom = true;
			ButtonFlags ^= zoomFlag;
		}
		else if ((ButtonFlags & panFlag) == panFlag)
		{
			bCustomizedPan = true;
			ButtonFlags ^= panFlag;
		}
	}
}

bool UnwrapMod::fnGetSelectionPreview() const
{
	return sSelectionPreview;
}

void UnwrapMod::fnSetSelectionPreview(bool bPreview)
{
	sSelectionPreview = bPreview;
	if (!sSelectionPreview)
	{
		if (ClearTVSelectionPreview())
		{
			InvalidateView();
		}
	}
}

bool UnwrapMod::LockMeshTopoData()
{
	return mMeshTopoData.Lock();
}

void UnwrapMod::UnlockMeshTopoData()
{
	mMeshTopoData.Unlock();
}

BOOL UnwrapMod::fnGetMirrorSelectionStatus()
{
	return mirrorGeoSelectionStatus;
}

void UnwrapMod::fnSetMirrorSelectionStatus(BOOL newStatus)
{
	if (newStatus != mirrorGeoSelectionStatus)
	{
		mirrorGeoSelectionStatus = newStatus;
		BuildMirrorDataForEachMeshTopoData();
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
		if (ip)
			ip->RedrawViews(ip->GetTime());
	}
}

int UnwrapMod::fnGetMirrorAxis()
{
	return mirrorGeoSelectionAxis;
}

float UnwrapMod::fnGetMirrorThreshold()
{
	return mirrorGeoThreshold;
}

void UnwrapMod::fnSetMirrorThreshold(float newValue)
{
	if (newValue != mirrorGeoThreshold)
	{
		mirrorGeoThreshold = newValue;
		mUIManager.SetSpinFValue(ID_SET_MIRROR_THRESHOLD_SPINNER, mirrorGeoThreshold);
		InvalidateMirrorDataForEachMeshTopoData();
		BuildMirrorDataForEachMeshTopoData();
	}
}

void UnwrapMod::fnSetMirrorAxis(int axis)
{
	if (axis != mirrorGeoSelectionAxis && axis >= 0 && axis <= 2)
	{
		mirrorGeoSelectionAxis = static_cast<MirrorAxisOptions>(axis);
		InvalidateMirrorDataForEachMeshTopoData();
		BuildMirrorDataForEachMeshTopoData();
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
		if (ip)
			ip->RedrawViews(ip->GetTime());
	}
}

void UnwrapMod::InvalidateMirrorDataForEachMeshTopoData()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->InvalidateMirrorData();
	}
}

void UnwrapMod::BuildMirrorDataForEachMeshTopoData()
{
	if (fnGetMirrorSelectionStatus())
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			mMeshTopoData[ldID]->BuildMirrorData(mirrorGeoSelectionAxis, fnGetTVSubMode(), fnGetMirrorThreshold(), mMeshTopoData.GetNode(ldID));
		}
	}
}

void UnwrapMod::SelectMirrorGeomVerts()
{
	if (fnGetMirrorSelectionStatus())
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			mMeshTopoData[ldID]->MirrorGeomVertSel();
		}
	}
}

void UnwrapMod::fnSetPaintMoveBrush(bool bPaint)
{
	if (bPaint)
	{
		if (ip)
			ip->ReplacePrompt(GetString(IDS_PAINT_MOVE_BRUSH));
		SetMode(ID_PAINT_MOVE_BRUSH);
	}
	else
	{
		SetMode(oldMode);
	}
}

bool UnwrapMod::fnGetPaintMoveBrush() const
{
	if (mode == ID_PAINT_MOVE_BRUSH)
		return true;
	else return false;
}

void UnwrapMod::fnSetRelaxMoveBrush(bool bPaint)
{
	if (bPaint)
	{
		if (ip)
			ip->ReplacePrompt(GetString(IDS_RELAX_MOVE_BRUSH));
		SetMode(ID_RELAX_MOVE_BRUSH);
	}
	else
	{
		SetMode(oldMode);
	}
}

bool UnwrapMod::fnGetRelaxMoveBrush() const
{
	if (mode == ID_RELAX_MOVE_BRUSH)
		return true;
	else return false;
}

void UnwrapMod::fnSetPaintFullStrengthSize(float val)
{
	if (val != sPaintFullStrengthSize && val > 0.0f)
	{
		sPaintFullStrengthSize = val;
		mUIManager.SetSpinFValue(ID_BRUSH_STRENGTHSPINNER, sPaintFullStrengthSize);
		if (sPaintFullStrengthSize > fnGetPaintFallOffSize())
		{
			fnSetPaintFallOffSize(sPaintFullStrengthSize);
		}
	}
}

float UnwrapMod::fnGetPaintFullStrengthSize() const
{
	return sPaintFullStrengthSize;
}

void UnwrapMod::fnSetPaintFallOffSize(float val)
{
	if (val != sPaintFallOffSize && val > 0.0f)
	{
		sPaintFallOffSize = val;
		mUIManager.SetSpinFValue(ID_BRUSH_FALLOFFSPINNER, sPaintFallOffSize);
		if (sPaintFallOffSize < fnGetPaintFullStrengthSize())
		{
			fnSetPaintFullStrengthSize(sPaintFallOffSize);
		}
	}
}

float UnwrapMod::fnGetPaintFallOffSize() const
{
	return sPaintFallOffSize;
}

void UnwrapMod::fnSetPaintFallOffType(int val)
{
	if (val != sPaintFallOffType && val >= 0 && val <= 3)
	{
		sPaintFallOffType = val;
		mUIManager.SetFlyOut(ID_BRUSH_FALLOFF_TYPE, fnGetPaintFallOffType(), FALSE);
	}
}

int UnwrapMod::fnGetPaintFallOffType() const
{
	return sPaintFallOffType;
}

void UnwrapMod::RecomputePivotOffset()
{
	Box3 bounds;
	bounds.Init();
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData *ld = GetMeshTopoData(ldID);
		if (ld == NULL)
		{
			DbgAssert(0);
		}
		else
		{
			int vselCount = ld->GetNumberTVVerts();

			int i1 = 0;
			int i2 = 1;
			GetUVWIndices(i1, i2);
			for (int i = 0; i < vselCount; i++)
			{
				if (ld->GetTVVertSelected(i))
				{
					//get bounds
					Point3 p = Point3(0.0f, 0.0f, 0.0f);
					p[i1] = ld->GetTVVert(i)[i1];
					p[i2] = ld->GetTVVert(i)[i2];
					bounds += p;
				}
			}
		}
	}

	Point3 originalPt = (selCenter + freeFormPivotOffset);
	freeFormPivotOffset = originalPt - bounds.Center();
}

void UnwrapMod::ConstructFreeFormBoxesForMovingDirection(Box3& freeFormBox,Box3& boundXForMove,Box3& boundYForMove, Box3& boundBothForXYMove)
{
	int i1 = 0;
	int i2 = 1;
	GetUVWIndices(i1, i2);
	Point2 aPivot(freeFormPivotOffset[i1],freeFormPivotOffset[i2]);
	Point3 centerP = freeFormBounds.Center() + Point3(aPivot.x,aPivot.y,freeFormBounds.Center().z);

	float xzoom = 1.0;
	float yzoom = 1.0; 
	int width = 1;
	int height = 1;
	ComputeZooms(hView, xzoom, yzoom, width, height);

	freeFormBox.Init();
	freeFormBox += freeFormBounds.pmin;
	freeFormBox += freeFormBounds.pmax;

	//Construct 2 boxes that come from the 2 lines' enlarging.
	//One horizontal line is from the center of the free form and extend by the length freeFormMoveAxisLength.
	//Another vertical line is from the center of the free form and and extend by the length freeFormMoveAxisLength.
	//freeFormMoveAxisLength is in the windows coordination, it can be converted into the UVW coordination by dividing xzoom and the yzoom
	float fExtensionX = freeFormMoveAxisLength/xzoom;
	float fExtensionY = freeFormMoveAxisLength/yzoom;
	float fQUarterExtensionX = fExtensionX*FREEMODEMOVERECTANGLE;
	float fQUarterExtensionY = fExtensionY*FREEMODEMOVERECTANGLE;
	float fQUarterHalfExtensionX = fQUarterExtensionX*0.5;
	float fQUarterHalfExtensionY = fQUarterExtensionY*0.5;

	boundXForMove.Init();
	boundXForMove += Point3(centerP.x+ fQUarterHalfExtensionX,centerP.y,centerP.z);
	boundXForMove += Point3(centerP.x+fExtensionX, centerP.y, centerP.z);
	
	boundYForMove.Init();
	boundYForMove += Point3(centerP.x, centerP.y+fQUarterHalfExtensionY, centerP.z);
	boundYForMove += Point3(centerP.x, centerP.y+fExtensionY, centerP.z);
	
	boundBothForXYMove.Init();
	boundBothForXYMove += centerP;
	boundBothForXYMove += Point3(centerP.x+fQUarterExtensionX, centerP.y, centerP.z);
	boundBothForXYMove += Point3(centerP.x, centerP.y + fQUarterExtensionY, centerP.z);
	boundBothForXYMove += Point3(centerP.x+fQUarterExtensionX, centerP.y+fQUarterExtensionY, centerP.z);

	float threshold = (1.0f / xzoom) * 4.0f;
	boundXForMove.EnlargeBy(threshold);
	boundYForMove.EnlargeBy(threshold);	
}

int UnwrapMod::OnTVDataChanged(IMeshTopoData*, BOOL bUpdateView)
{
	if (fnGetSyncSelectionMode())
	{
		theHold.Suspend();
		SyncGeomSelection(bUpdateView?TRUE:FALSE);
		theHold.Resume();
	}

	if (bUpdateView)
	{
		SetupTypeins();

		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (UnwrapMod::editMod == this && UnwrapMod::hView)
			InvalidateView();
	}
	return 1;
}

int UnwrapMod::OnTVertFaceChanged(IMeshTopoData*, BOOL bUpdateView)
{
	UpdateMatFilters();
	if (bUpdateView)
	{
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (UnwrapMod::editMod == this && UnwrapMod::hView)
			InvalidateView();
	}
	return 1;
}

int UnwrapMod::OnSelectionChanged(IMeshTopoData*, BOOL bUpdateView)
{
	RebuildDistCache();

	if (fnGetSyncSelectionMode())
	{
		theHold.Suspend();
		fnSyncGeomSelection();
		theHold.Resume();
	}
	
	if (bUpdateView)
	{
		if (UnwrapMod::editMod == this && UnwrapMod::hView)
			InvalidateView();
	}
	return 1;
}

int UnwrapMod::OnPinAddedOrDeleted(IMeshTopoData* pTopoData, int index)
{
	if (fnGetMapMode() == LSCMMAP)
		mLSCMTool.InvalidatePinAddDelete(dynamic_cast<MeshTopoData*>(pTopoData), index);
	return 1;
}

int UnwrapMod::OnPinInvalidated(IMeshTopoData* pTopoData, int index)
{
	if (fnGetMapMode() == LSCMMAP)
		mLSCMTool.InvalidatePin(dynamic_cast<MeshTopoData*>(pTopoData), index);
	return 1;
}

int UnwrapMod::OnTopoInvalidated(IMeshTopoData* pTopoData)
{
	if (fnGetMapMode() == LSCMMAP)
		mLSCMTool.InvalidateTopo(dynamic_cast<MeshTopoData*>(pTopoData));
	return 1;
}

int UnwrapMod::OnTVVertChanged(IMeshTopoData* pTopoData, TimeValue t, int index, const Point3& p)
{
	if (!pTopoData)
		return 1;

	int controlIndex = pTopoData->GetTVVertControlIndex(index);
	Point3 inputP = p;
	//first check if there is a control on it and if so set the value for
	if ((controlIndex != -1) && (controlIndex < ControllerCount()) &&
		Controller(controlIndex))
	{
		Point3 curVal;
		Controller(controlIndex)->GetValue(t, &curVal, FOREVER);
		if (curVal != p)
		{
			Controller(controlIndex)->SetValue(t, inputP);
		}
	}
	//next check if we are animating and no control
	else if (Animating())
	{
		//create a new control
		controlIndex = ControllerAdd();
		//remember its id
		pTopoData->SetTVVertControlIndex(index, controlIndex);
		//set its value
		Controller(controlIndex)->SetValue(t, inputP);
		Controller(controlIndex)->SetValue(0, inputP);
	}
	return 1;
}

int UnwrapMod::OnTVVertDeleted(IMeshTopoData* pTopoData, int index)
{
	if (!pTopoData)
		return 1;

	int controlID = pTopoData->GetTVVertControlIndex(index); 
	if (controlID >= 0)
		DeleteReference(controlID);
	return 1;
}

int UnwrapMod::OnMeshTopoDataDeleted(IMeshTopoData* pTopoData)
{
	if (!pTopoData)
		return 1;
	pTopoData->UnRegisterNotification(this);
	mMeshTopoData.ClearTopoData(dynamic_cast<MeshTopoData*>(pTopoData));
	return 1;
}

int UnwrapMod::OnNotifyUpdateTexmap(IMeshTopoData* pTopoData)
{
	if (!pTopoData)
		return 1;

	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	return 1;
}

int UnwrapMod::OnNotifyUIInvalidation(IMeshTopoData* pTopoData, BOOL bRedrawAtOnce)
{
	if (!pTopoData)
		return 1;

	InvalidateView();
	if (bRedrawAtOnce)
	{
		UpdateWindow(hDialogWnd);
		if (ip) ip->RedrawViews(ip->GetTime());
		SetupTypeins();
	}
	return 1;
}

int UnwrapMod::OnNotifyFaceSelectionChanged(IMeshTopoData* )
{
	bMapGizmoTMDirty = TRUE;
	return 1;
}
