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
#include "BezierInterp.h"
#include "CATRigPresets.h"

#include "CATWeight.h"
#include <CATAPI/CATClassID.h>

class CATWeightClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATWeight(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATWEIGHT); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return CATWEIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATWeight"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATWeightClassDesc CATWeightDesc;
ClassDesc2* GetCATWeightDesc() { return &CATWeightDesc; }

#include "iparamm2.h"
class CATWeightParamDlgCallBack : public ParamMap2UserDlgProc {

	CATWeight* ctrlCATWeight;
	HWND hGraphFrame;
	RECT rcGraphRect;
	HDC hGraphMemDC;
	HBITMAP hGraphBitmap;
	HGDIOBJ hGraphMemDCOldSel;

	HWND hDlg;

	BOOL bTitleChanged;

public:

	CATWeightParamDlgCallBack() : hDlg(NULL), ctrlCATWeight(NULL) {
		hGraphFrame = NULL;
		SetRectEmpty(&rcGraphRect);
		hGraphMemDC = NULL;
		hGraphBitmap = NULL;
		hGraphMemDCOldSel = NULL;
		bTitleChanged = FALSE;
	}

	void InitControls(HWND hWnd)
	{
		this->hDlg = hWnd;

		// Get the graph rectangle.  We need to hide the graph
		// frame after doing this.  It's only a guide.  If we
		// don't hide it, it'll cover our graph blits.
		hGraphFrame = GetDlgItem(hDlg, IDC_GRAPH_FRAME);

		GetClientRect(hGraphFrame, &rcGraphRect);

		// Create a memory DC for the graph and attach to device-
		// compatible bitmap.
		HDC hdc = GetDC(hDlg);
		hGraphMemDC = CreateCompatibleDC(hdc);
		hGraphBitmap = CreateCompatibleBitmap(hdc,
			rcGraphRect.right - rcGraphRect.left + 1,
			rcGraphRect.bottom - rcGraphRect.top + 1);
		hGraphMemDCOldSel = SelectObject(hGraphMemDC, hGraphBitmap);
		ReleaseDC(hDlg, hdc);

		SetFocus(hGraphFrame);

	}
	void ReleaseControls()
	{
		SelectObject(hGraphMemDC, hGraphMemDCOldSel);
		DeleteObject(hGraphBitmap);
		DeleteDC(hGraphMemDC);
	}

	void PaintGraphNow(HDC hdc, LPRECT lpRect)
	{
		RECT rcIntersection;
		if (IntersectRect(&rcIntersection, lpRect, &rcGraphRect)) {
			// Copy to screen DC
			int nWidth = rcIntersection.right - rcIntersection.left;
			int nHeight = rcIntersection.bottom - rcIntersection.top;
			BitBlt(hdc,
				rcIntersection.left, rcIntersection.top,
				nWidth, nHeight,
				hGraphMemDC,
				rcIntersection.left - rcGraphRect.left,
				rcIntersection.top - rcGraphRect.top,
				SRCCOPY);
		}
	}

	// GB 12-Jul-03: All graph drawing must be done via this function.
	// It calls the draw function on the hierarchy root, passing in the
	// DC.  If the hierarchy ever wants to initiate a graph draw, it
	// must call a user message on this window.  We cannot store the DC
	// in the hierarchy because it cannot be used by two threads.  Can
	// pass in the hWnd though.
	void DrawGraph() {

		if (!hGraphMemDC) return;
		int nGraphWidth = (rcGraphRect.right - rcGraphRect.left);
		int nGraphHeight = (rcGraphRect.bottom - rcGraphRect.top);

		// Erase the graph
		HBRUSH hbrushCyan;
		hbrushCyan = CreateSolidBrush(RGB(0, 192, 192));
		FillRect(hGraphMemDC, &rcGraphRect, hbrushCyan);
		DeleteObject(hbrushCyan);

		int stepsize = nGraphWidth / 20;
		HPEN hGraphPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));

		float xScale = (float)STEPTIME100 / nGraphWidth;
		//		float xScale = (float)nGraphWidth/STEPTIME100;
		float yScale = (float)nGraphHeight * 0.9f;
		float cushion = (nGraphHeight - yScale) / 2;

		Interval iv;
		int x, oldX = 0;
		int y, oldY = (int)((1.0f - ctrlCATWeight->GetYval(0, iv)) * yScale + cushion);

		SelectObject(hGraphMemDC, hGraphPen);
		//		MoveToEx(hGraphMemDC, oldX, oldY, NULL);
		//		LineTo(hGraphMemDC, nGraphWidth, 0);

		for (int i = stepsize; i < nGraphWidth; i += stepsize)
		{
			x = (int)(i * xScale);
			y = (int)((1.0f - ctrlCATWeight->GetYval(x, iv)) * yScale + cushion);
			MoveToEx(hGraphMemDC, oldX, oldY, NULL);
			LineTo(hGraphMemDC, i, y);
			oldX = i;
			oldY = y;
		}

		InvalidateRect(hDlg, &rcGraphRect, FALSE);
		/*
		{
			// Draw graph without invalidating the region,
			// create a DC, paint it, and validate (in case
			// there are paint messages further down the
			// queue).
			HDC hdc = GetDC(hDlg);
			PaintGraphNow(hdc, &rcGraphRect);
			ReleaseDC(hDlg, hdc);
			ValidateRect(hDlg, &rcGraphRect);
		}	*/
	}

	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		ctrlCATWeight = (CATWeight*)map->GetParamBlock()->GetOwner();
		if (!ctrlCATWeight) return FALSE;

		switch (msg) {
		case WM_INITDIALOG:
		{
			bTitleChanged = FALSE;
			InitControls(hWnd);
			DrawGraph();
		}
		break;

		case WM_DESTROY:
			ReleaseControls();
			break;

		case WM_PAINT:
		{
			if (!bTitleChanged) {
				// Replace the window caption with the desired one...
				IRollupWindow *pRollupWindow = GetCOREInterface()->GetCommandPanelRollup();
				DbgAssert(pRollupWindow);
				int nPanelIndex = pRollupWindow->GetPanelIndex(hWnd);
				if (nPanelIndex >= 0) {
					pRollupWindow->SetPanelTitle(nPanelIndex, const_cast<TCHAR*>(ctrlCATWeight->strRolloutCaption));
					bTitleChanged = TRUE;
				}
			}

			//
			// Blit our current graph bitmap to the screen if the update
			// region intersects with the graph rect.
			//
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);
			if (hdc) {
				PaintGraphNow(hdc, &ps.rcPaint);
				EndPaint(hDlg, &ps);
			}
		}
		break;

		case CC_SPINNER_CHANGE: {
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_IN_TAN_LEN:
			case IDC_SPIN_OUT_TAN_LEN:
			case IDC_SPIN_VAL1:
			case IDC_SPIN_VAL2:
				DrawGraph();
				break;
			}
			break;
		}

		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }//delete this; }
};

static CATWeightParamDlgCallBack CATWeightParamCallBack;

static ParamBlockDesc2 CATWeight_param_blk(CATWeight::PBLOCK_REF, _T("CATWeight Params"), 0, &CATWeightDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, CATWeight::PBLOCK_REF,
	IDD_CATWEIGHT_ROLLOUT, IDS_ROTWEIGHT, 0, 0, &CATWeightParamCallBack,

	CATWeight::PB_KEY1VAL, _T("KEY1Val"), TYPE_FLOAT, P_RESET_DEFAULT, IDS_KEY1VAL,
		p_default, 0.0f,
		p_range, -1.0f, 1.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_VAL1, IDC_SPIN_VAL1, SPIN_AUTOSCALE,//0.01f,
		p_end,
	CATWeight::PB_KEY1OUTTANLEN, _T("KEY1OutTanLen"), TYPE_FLOAT, P_RESET_DEFAULT, IDS_KEY1OUTTANLEN,
		p_default, 0.333f,
		p_range, 0.0f, 1.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_OUT_TAN_LEN, IDC_SPIN_OUT_TAN_LEN, SPIN_AUTOSCALE,//0.01f,
		p_end,
	CATWeight::PB_KEY2VAL, _T("KEY2Val"), TYPE_FLOAT, P_RESET_DEFAULT, IDS_KEY2VAL,
		p_default, 1.0f,
		p_range, -1.0f, 1.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_VAL2, IDC_SPIN_VAL2, SPIN_AUTOSCALE,//0.01f,
		p_end,
	CATWeight::PB_KEY2INTANLEN, _T("KEY2InTanLen"), TYPE_FLOAT, P_RESET_DEFAULT, IDS_KEY2INTANLEN,
		p_default, 0.333f,
		p_range, 0.0f, 1.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_IN_TAN_LEN, IDC_SPIN_IN_TAN_LEN, SPIN_AUTOSCALE,//0.01f,
		p_end,

	CATWeight::PB_KEY1TAN, _T("KEY1Tan"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 0.0f,
		p_end,
	CATWeight::PB_KEY2TAN, _T("KEY2Tan"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 0.0f,
		p_end,
	p_end
);

CATWeight::CATWeight() {
	pblock = NULL;
	CATWeightDesc.MakeAutoParamBlocks(this);
	strRolloutCaption = GetString(IDS_WGT_CURVE);
	//	paramDim = defaultDim;
}

CATWeight::~CATWeight() {
	DeleteAllRefs();
}

RefTargetHandle CATWeight::Clone(RemapDir& remap)
{
	// make a new CATWeight object to be the clone
	CATWeight *newCATWeight = new CATWeight();

	newCATWeight->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATWeight, remap);

	// now return the new object.
	return newCATWeight;
}

void CATWeight::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		CATWeight *newctrl = (CATWeight*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** *
void CATWeight::GetCATKey(const int			i,
						 const TimeValue	t,
						 CATKey				&key,
						 bool				&isInTan,
						 bool				&isOutTan)
{
	switch(i)
	{
	case 0:
		{
			key = CATKey(
				0,
				pblock->GetFloat(PB_KEY1VAL, t),
		/		pblock->GetFloat(PB_KEY1TANGENT, t),
				pblock->GetFloat(PB_KEY1OUTTANLEN, t),
				0);
			isInTan = TRUE;
			isOutTan = TRUE;
			break;
		}
	case 1:
		{
			key = CATKey(
				STEPTIME100,
				pblock->GetFloat(PB_KEY2VAL, t),
				pblock->GetFloat(PB_KEY2TANGENT, t),
				0,
				pblock->GetFloat(PB_KEY2INTANLEN, t));
			isInTan = TRUE;
			isOutTan = TRUE;
			break;
		}
	default:{
			key = CATKey();
			isInTan = FALSE;
			isOutTan = FALSE;
		}
	}
}
*/

float CATWeight::GetYval(TimeValue t, Interval& valid)
{
	float KEY1Val = pblock->GetFloat(PB_KEY1VAL, t);
	float KEY1tangent = pblock->GetFloat(PB_KEY1TAN, t);
	float KEY1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);

	float KEY2Val = pblock->GetFloat(PB_KEY2VAL, t);
	float KEY2tangent = pblock->GetFloat(PB_KEY2TAN, t);
	float KEY2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);

	CATKey key1(0, KEY1Val, KEY1tangent, KEY1outlength, 0);
	CATKey key2(STEPTIME100, KEY2Val, -KEY2tangent, 0, KEY2inlength);

	pblock->GetValidity(t, valid);
	return InterpValue(key1, key2, t);

}

void CATWeight::GetValue(TimeValue t, void* val, Interval& valid, GetSetMethod)
{
	//valid = FOREVER;
	*(float*)val = GetYval(t, valid);
}

void CATWeight::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	CATWeightDesc.BeginEditParams(ip, this, flags, prev);
	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void CATWeight::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	CATWeightDesc.EndEditParams(ip, this, flags, next);
}

BOOL CATWeight::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idCATWeightParams) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idWeightOutVal:	load->ToParamBlock(pblock, PB_KEY1VAL);			break;
			case idWeightOutTan:	load->ToParamBlock(pblock, PB_KEY1TAN);			break;
			case idWeightOutLen:	load->ToParamBlock(pblock, PB_KEY1OUTTANLEN);	break;

			case idWeightInVal:		load->ToParamBlock(pblock, PB_KEY2VAL);			break;
			case idWeightInTan:		load->ToParamBlock(pblock, PB_KEY2TAN);			break;
			case idWeightInLen:		load->ToParamBlock(pblock, PB_KEY2INTANLEN);	break;

			default:				load->AssertOutOfPlace();						break;
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

BOOL CATWeight::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idCATWeightParams);

	save->FromParamBlock(pblock, idWeightOutVal, PB_KEY1VAL);
	save->FromParamBlock(pblock, idWeightOutTan, PB_KEY1TAN);
	save->FromParamBlock(pblock, idWeightOutLen, PB_KEY1OUTTANLEN);

	save->FromParamBlock(pblock, idWeightInVal, PB_KEY2VAL);
	save->FromParamBlock(pblock, idWeightInTan, PB_KEY2TAN);
	save->FromParamBlock(pblock, idWeightInLen, PB_KEY2INTANLEN);

	save->EndGroup();
	return TRUE;
}

BOOL CATWeight::PasteRig(CATWeight* pastectrl)
{
	pblock->SetValue(PB_KEY1VAL, 0, pastectrl->pblock->GetFloat(CATWeight::PB_KEY1VAL));
	pblock->SetValue(PB_KEY1TAN, 0, pastectrl->pblock->GetFloat(CATWeight::PB_KEY1TAN));
	pblock->SetValue(PB_KEY1OUTTANLEN, 0, pastectrl->pblock->GetFloat(CATWeight::PB_KEY1OUTTANLEN));

	pblock->SetValue(PB_KEY2VAL, 0, pastectrl->pblock->GetFloat(CATWeight::PB_KEY2VAL));
	pblock->SetValue(PB_KEY2TAN, 0, pastectrl->pblock->GetFloat(CATWeight::PB_KEY2TAN));
	pblock->SetValue(PB_KEY2INTANLEN, 0, pastectrl->pblock->GetFloat(CATWeight::PB_KEY2INTANLEN));

	return TRUE;
}

void CATWeight::DisplayRollout(IObjParam *ip, ULONG flags, Animatable *prev, const TCHAR *caption)
{
	strRolloutCaption = caption;
	BeginEditParams(ip, flags, prev);
};

void CATWeight::DisplayWindow(const TCHAR *caption, HWND)
{
	strRolloutCaption = caption;

	/*	if (!hERNWnd) {
			hERNWnd = ::CreateERNWindow(hInstance, GetCOREInterface()->GetMAXHWnd(), hWndOwner, this);
		}
		else {
			ShowWindow(hERNWnd, SW_RESTORE);
			SetWindowPos(hERNWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		return hERNWnd;
	*/
}