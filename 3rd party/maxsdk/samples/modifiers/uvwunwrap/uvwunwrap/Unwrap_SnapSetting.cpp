//
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.   
//
//

#include "unwrap.h"
#include "modsres.h"
#include <helpsys.h>

int UnwrapMod::pixelCenterSnap = 0;
BOOL UnwrapMod::snapToggle = FALSE;
int UnwrapMod::pixelCornerSnap = 0;
float UnwrapMod::snapStrength = 0.1f;

INT_PTR CALLBACK UnwrapSnapSettingDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static BOOL sPixelCornerSnap = FALSE;
	static BOOL sPixelCenterSnap = FALSE;
	static ISpinnerControl *spinSnapStrength = nullptr;
	static float sSnapStrength = .2f;

	switch (msg) {
	  case WM_INITDIALOG:
		  {
			  mod = (UnwrapMod*)lParam;
			  UnwrapMod::hSnapSettingDialog = hWnd;

			  DLSetWindowLongPtr(hWnd, lParam);
			  ::SetWindowContextHelpId(hWnd, idh_unwrap_options);
			  
			  CheckDlgButton(hWnd, IDC_UNWRAP_PIXEL_CORNER_SNAP, mod->pixelCornerSnap);
			  sPixelCornerSnap = mod->pixelCornerSnap;
			  CheckDlgButton(hWnd, IDC_UNWRAP_PIXEL_CENTER_SNAP, mod->pixelCenterSnap);
			  sPixelCenterSnap = mod->pixelCenterSnap;
			  CheckDlgButton(hWnd, IDC_UNWRAP_GRID_SNAP, mod->GetGridSnap());
			  CheckDlgButton(hWnd, IDC_UNWRAP_VERTEX_SNAP, mod->GetVertexSnap());
			  CheckDlgButton(hWnd, IDC_UNWRAP_EDGE_SNAP, mod->GetEdgeSnap());

			  spinSnapStrength = SetupFloatSpinner(
				  hWnd, IDC_UNWRAP_GRIDSTRSPIN, IDC_UNWRAP_GRIDSTR,
				  0.0f, 0.5f, mod->fnGetSnapStrength());
			  sSnapStrength = mod->fnGetSnapStrength();

			  break;
		  }
	  case WM_SYSCOMMAND:
		  if ((wParam & 0xfff0) == SC_CONTEXTHELP) 
		  {
			  MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_options); 
		  }
		  return FALSE;
	  case WM_CLOSE:
		  {
			  if(spinSnapStrength)
			  { 
				  ReleaseISpinner(spinSnapStrength);
				  spinSnapStrength = nullptr;
			  }
			  mod->hSnapSettingDialog = NULL;
			  EndDialog(hWnd,0);
			  break;
		  }
	  case WM_COMMAND:
		  switch (LOWORD(wParam)) 
		  {
		  case IDOK:
			  {
				  mod->pixelCornerSnap = IsDlgButtonChecked(hWnd, IDC_UNWRAP_PIXEL_CORNER_SNAP);
				  mod->pixelCenterSnap = IsDlgButtonChecked(hWnd, IDC_UNWRAP_PIXEL_CENTER_SNAP);
				  mod->SetGridSnap(IsDlgButtonChecked(hWnd, IDC_UNWRAP_GRID_SNAP));
				  mod->SetEdgeSnap(IsDlgButtonChecked(hWnd, IDC_UNWRAP_EDGE_SNAP));
				  mod->SetVertexSnap(IsDlgButtonChecked(hWnd, IDC_UNWRAP_VERTEX_SNAP));
				  mod->fnSetSnapStrength(spinSnapStrength->GetFVal());

				  if(spinSnapStrength)
				  { 
					  ReleaseISpinner(spinSnapStrength);
					  spinSnapStrength = nullptr;
				  }
				  mod->hSnapSettingDialog = NULL;
				  EndDialog(hWnd, 0);
				  break;
			  }

		  case IDCANCEL:
			  {
				 mod->pixelCornerSnap = sPixelCornerSnap;
				 mod->pixelCenterSnap = sPixelCenterSnap;
				 mod->fnSetSnapStrength(sSnapStrength);

				 if(spinSnapStrength)
				 { 
					 ReleaseISpinner(spinSnapStrength);
					 spinSnapStrength = nullptr;
				 }
				
				 mod->hSnapSettingDialog = NULL;
				 EndDialog(hWnd, 0);
				 break;
			  }
		  }
		  break;

	  default:
		  return FALSE;
	}
	return TRUE;
}


void  UnwrapMod::fnSnapSettingDialog()
{
	//bring up the dialog
	if (hSnapSettingDialog == NULL)
	{
		hSnapSettingDialog = CreateDialogParam(hInstance,
			MAKEINTRESOURCE(IDD_SNAPSETTINGDIALOG),
			hDialogWnd,
			UnwrapSnapSettingDlgProc,
			(LPARAM)this );
		ShowWindow(hSnapSettingDialog,SW_SHOW);
	}
}