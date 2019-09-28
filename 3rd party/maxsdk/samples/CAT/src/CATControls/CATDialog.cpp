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

#include "cat.h"
#include "resource.h"
#include "CATDialog.h"

static INT_PTR CALLBACK CATDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK CATModalDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Creates a modeless dialog and registers it with Max.
HWND CATDialog::Create(HINSTANCE hInst, HWND hParentWnd)
{
	if (!hDlg) {
		hInstance = hInst;
		CreateDialogParam(hInst, MAKEINTRESOURCE(GetDialogTemplateID()), hParentWnd, CATDialogProc, (LPARAM)this);
	}
	return hDlg;
}

int CATModalDialog::DoModal(HINSTANCE hInst, HWND hParentWnd)
{
	if (!hDlg) {
		hInstance = hInst;
		return (int)DialogBoxParam(hInst, MAKEINTRESOURCE(GetDialogTemplateID()), hParentWnd, CATModalDialogProc, (LPARAM)this);
	}
	return -1;
}

//
// The CATDialog is passed through lParam on init, and subsequently
// stored in the window long pointer.  The virtual callback function
// is then called on the CATDialog instance and its result is returned.
// If WM_DESTROY is received, the CATDialog instance is deleted before
// returning.
//
static INT_PTR CALLBACK CATDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	CATDialog *dlg;

	if (uMsg == WM_INITDIALOG) {
		dlg = (CATDialog*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
		GetCOREInterface()->RegisterDlgWnd(hWnd);
	}
	else {
		dlg = (CATDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	if (dlg) {

		switch (uMsg) {
		case WM_INITDIALOG:
			dlg->InitControls(hWnd);
			break;
		case WM_DESTROY:
			dlg->ReleaseControls(hWnd);
			break;
		case WM_CLOSE:
			GetCOREInterface()->UnRegisterDlgWnd(hWnd);
			DestroyWindow(hWnd);
			return TRUE;
		case WM_NCDESTROY:
			delete dlg;
			return TRUE;
		}

		return dlg->DialogProc(hWnd, uMsg, wParam, lParam);
	}

	return FALSE;
}

//
// The CATModalDialog is passed through lParam on init, and subsequently
// stored in the window long pointer.  The virtual callback function
// is then called on the CATModalDialog instance and its result is returned.
//
static INT_PTR CALLBACK CATModalDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	CATDialog *dlg;

	if (uMsg == WM_INITDIALOG) {
		dlg = (CATDialog*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
	}
	else {
		dlg = (CATDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	if (dlg) {

		BOOL handled = FALSE;
		switch (uMsg) {
		case WM_INITDIALOG:
			dlg->InitControls(hWnd);
			handled = TRUE;
			break;
		case WM_DESTROY:
			dlg->ReleaseControls(hWnd);
			handled = TRUE;
			break;
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
		}
		handled |= dlg->DialogProc(hWnd, uMsg, wParam, lParam);
		return handled;
	}

	return FALSE;
}

// MELONG!!! :P