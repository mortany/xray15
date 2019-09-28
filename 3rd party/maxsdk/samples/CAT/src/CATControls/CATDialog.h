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

#pragma once

class CATDialog : public MaxHeapOperators
{
protected:
	virtual WORD GetDialogTemplateID() const = 0;
	HINSTANCE hInstance;
	HWND hDlg;

	// Children must implement this to check
	virtual void InitControls() = 0;
	virtual void ReleaseControls() = 0;

public:

	CATDialog() {
		hDlg = NULL;
		hInstance = NULL;
	}

	virtual ~CATDialog() {
		DbgAssert(hDlg == NULL);
	}

	HWND GetDlg() { return hDlg; }

	void InitControls(HWND hWnd)
	{
		DbgAssert(hDlg == NULL);
		hDlg = hWnd;
		InitControls(); // Call derived version
	};
	void ReleaseControls(HWND hWnd)
	{
		UNUSED_PARAM(hWnd);
		DbgAssert(hDlg == hWnd);
		ReleaseControls(); // Call derived version.
		hDlg = NULL;
		hInstance = NULL;
	};

	// Basic window functions...
	//   Create() creates a modeless dialog and returns its handle.
	//   CreateModal() creates a modal dialog and returns the result.
	//   Show() makes the window visible.
	//   Hide() makes the window invisible.
	//
	HWND Create(HINSTANCE hInst, HWND hParentWnd);

	void Show() { if (hDlg) ::ShowWindow(hDlg, SW_SHOW); }
	void Hide() { if (hDlg) ::ShowWindow(hDlg, SW_HIDE); }

	// The dialog callback is where all the fun stuff goes down...
	virtual INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};

class CATModalDialog : public MaxHeapOperators
{
	virtual WORD GetDialogTemplateID() const = 0;

public:

	HWND hDlg;
	HINSTANCE hInstance;

	CATModalDialog() {
		hDlg = NULL;
		hInstance = NULL;
	}

	// Basic window functions...
	//   DoModal() creates a modal dialog and returns the result.
	//
	int DoModal(HINSTANCE hInst, HWND hParentWnd);

	// The dialog callback is where all the fun stuff goes down...
	virtual INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};
