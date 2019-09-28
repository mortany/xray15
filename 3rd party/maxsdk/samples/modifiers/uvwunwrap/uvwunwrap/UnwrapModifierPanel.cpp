/*

Copyright [YEAR*] Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/


#include "UnwrapModifierPanelUI.h"
#include "unwrap.h"
#include "modsres.h"

INT_PTR CALLBACK ModifierPanelRollupDialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{

	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	if ( !mod && message != WM_INITDIALOG ) 
		return FALSE;

	BOOL processed = mod->GetUIManager()->MessageProc(hWnd,message,wParam,lParam);
	if (processed)
		return TRUE;

	switch ( message ) 
	{
	case WM_INITDIALOG:
		{
			DLSetWindowLongPtr(hWnd, lParam);
			mod = (UnwrapMod*)lParam;
		}

		return TRUE;

	case WM_DESTROY:
	case WM_MOUSEACTIVATE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:            
		return FALSE;
/*
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{

		}
*/
	}
	return FALSE;
}


ModifierPanelUI::ModifierPanelUI() : mRollupHwnd(NULL)
{
	mMod = NULL;
}
ModifierPanelUI::~ModifierPanelUI()
{

}

void ModifierPanelUI::Init(UnwrapMod *mod)
{
	mMod = mod;
}

namespace 
{
	//Utility function for the modifier panel, when the toolbar window is shorter than the toolbar icons, adjust the size
	void AdjustToolbarFrame(ToolBarFrame* bar, int expectedWidth)
	{
		int width = bar->Width();
		
		if (width < expectedWidth)
		{
			HWND hWnd = bar->GetToolbarWindow();
			if (hWnd)
			{
				WINDOWPLACEMENT p;
				p.length = sizeof(WINDOWPLACEMENT);

				GetWindowPlacement(hWnd, &p);
				int delta = (expectedWidth - width) / 2;
				int x = p.rcNormalPosition.left - delta;
				//Start X is at least 4 pixels from the parent widget's left edge otherwise it is covered the groupbox
				int startX = MaxSDK::UIScaled(4);
				if (x < startX)
					x = startX;
				int y = p.rcNormalPosition.top;
				bar->ResizeWindow(x, y, expectedWidth, bar->Height());
			}
		}
	}
}


void ModifierPanelUI::SetDefaults(int index, HWND parentWindow)
{
	if (index == 200) //select1
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_SELPARAM1_TOOLBAR),index,_M("__InternalSelect1"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect1"));

		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_TV_VERTMODE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_TV_EDGEMODE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_TV_FACEMODE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_GEOM_ELEMENT);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}

	if (index == 201) //select2
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_SELPARAM2_TOOLBAR),index,_M("__InternalSelect2"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect2"));

		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_GEOMEXPANDFACESEL);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_GEOMCONTRACTFACESEL);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_EDGELOOPSELECTION);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_EDGERINGSELECTION);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}

	if (index == 202) //select2
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_SELPARAM3_TOOLBAR),index,_M("__InternalSelect3"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect3"));
		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_PLANARMODE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SELECTBY_SMGRP);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}
	if (index == 204) //mirror sel
	{
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_SELMIRROR_TOOLBAR),index,_M("__InternalSelect5"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect5"));
		int expectedWidth = mMod->AddActionToToolbar(bar1, ID_MIRROR_GEOM_SELECTION);
		expectedWidth += mMod->AddActionToToolbar(bar1, ID_MIRROR_SEL_X_AXIS);
		expectedWidth += mMod->AddActionToToolbar(bar1, ID_MIRROR_SEL_Y_AXIS);
		expectedWidth += mMod->AddActionToToolbar(bar1, ID_MIRROR_SEL_Z_AXIS);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}

	if (index == 205) 
	{
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_SELPARAM5_TOOLBAR),index,_M("__InternalSelect6"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect6"));
		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_IGNOREBACKFACE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_POINT_TO_POINT_SEL);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}

	if (index == 206) //set mirror threshold
	{
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow, IDC_MIRROR_THRESHOLD_TOOLBAR), index, _M("__InternalSelect7"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect7"));
		int expectedWidth = mMod->AddActionToToolbar(bar1, ID_SET_MIRROR_THRESHOLD_EDIT);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}

	if (index == 210) //EditUVs1
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_EDITPARAM1_TOOLBAR),index,_M("__InternalEditUVs1"));
	}
	
	if (index == 211) //EditUVs2
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_EDITPARAM2_TOOLBAR),index,_M("__InternalEditUVs2"));
	}
	if (index == 212) //EditUVs3
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_EDITPARAM3_TOOLBAR),index,_M("__InternalEditUVs3"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalEditUVs3"));

		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_QMAP);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_QUICKMAP_DISPLAY);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_QUICKMAP_ALIGN);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}
	if (index == 230) //Peel1
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_PEELPARAM1_TOOLBAR),index,_M("__InternalMPeel1"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalMPeel1"));
		
		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_SEPARATORHALF);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_LSCM_SOLVE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_LSCM_INTERACTIVE);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_LSCM_RESET);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_PELT_MAP);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}
	if (index == 231) //Peel2
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_PEELPARAM2_TOOLBAR),index,_M("__InternalMPeel2"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalMPeel2"));

		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_SEPARATORHALF);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_PELT_EDITSEAMS);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_PELT_POINTTOPOINTSEAMS);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_PW_SELTOSEAM2);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_PELT_EXPANDSELTOSEAM);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}
	if (index == 240) //map
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_MAPPARAM1_TOOLBAR),index,_M("__InternalProjection1"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalProjection1"));
		
		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_SEPARATORHALF);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_PLANAR_MAP);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_CYLINDRICAL_MAP);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SPHERICAL_MAP);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_BOX_MAP);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
		
	}
	if (index == 241) //map
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_MAPPARAM2_TOOLBAR),index,_M("__InternalProjection2"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalProjection2"));

		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_MAPPING_ALIGNX);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_MAPPING_ALIGNY);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_MAPPING_ALIGNZ);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_MAPPING_NORMALALIGN);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_MAPPING_ALIGNTOVIEW);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}
	if (index == 242) //map
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_MAPPARAM3_TOOLBAR),index,_M("__InternalProjection3"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalProjection3"));		
//		mMod->AddActionToToolbar(bar1,ID_MAPPING_CENTER);
//		mMod->AddActionToToolbar(bar1,ID_MAPPING_FIT);		
		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_MAPPING_RESET);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
		
	}
	if (index == 250) //wrap
	{		
		mMod->GetUIManager()->AppendToolBar(GetDlgItem(parentWindow,IDC_WRAPPARAM1_TOOLBAR),index,_M("__InternalWrap1"));
		ToolBarFrame* bar1 = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalWrap1"));
				
		int expectedWidth = mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SPLINE_MAP);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_SEPARATOR1);
		expectedWidth += mMod->AddActionToToolbar(bar1,ID_UNFOLD_EDGE);
		AdjustToolbarFrame(bar1, MaxSDK::UIScaled(expectedWidth));
	}

}

void ModifierPanelUI::LoadInActions(int index)
{
	ToolBarFrame *bar = NULL;
	if (index == 200)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect1"));
	else if (index == 201)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect2"));
	else if (index == 202)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect3"));
	else if (index == 204)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect5"));
	else if (index == 205)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect6"));
	else if (index == 206)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalSelect7"));
	else if (index == 210)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalEditUVs1"));
	else if (index == 211)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalEditUVs2"));
	else if (index == 212)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalEditUVs3"));
	else if (index == 230)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalMPeel1"));
	else if (index == 231)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalMPeel2"));
	else if (index == 240)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalProjection1"));
	else if (index == 241)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalProjection2"));
	else if (index == 242)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalProjection3"));
	else if (index == 250)
		bar = mMod->GetUIManager()->GetToolBarFrame(_M("__InternalWrap1"));

	if (bar)
	{
		Tab<int> ids;
		ids = bar->LoadIDs();
		for (int j = 0; j < ids.Count(); j++)
		{
			int id = ids[j];
			mMod->AddActionToToolbar(bar,id);
		}
		 bar->LoadIDsClear();
	}
}




void ModifierPanelUI::Setup(HINSTANCE hInstance, HWND rollupHWND, const MCHAR *iniFile)
{
	//setup rollup
	mRollupHwnd = rollupHWND;

	UnwrapCustomUI* pUIManager = mMod->GetUIManager();
	if(NULL == pUIManager)
	{
		return;
	}
	bool bResult = pUIManager->GetIniFileCache().InitCacheFromIniFile(TSTR(iniFile));

	IRollupWindow *irollup = GetIRollup(rollupHWND);
	if(NULL == irollup)
	{
		return;
	}
		
	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,200,mMod->hSelParams,GetDlgItem(mMod->hSelParams,IDC_SELPARAM1_TOOLBAR)) > 0)
		LoadInActions(200);
	else
		SetDefaults(200,mMod->hSelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,201,mMod->hSelParams,GetDlgItem(mMod->hSelParams,IDC_SELPARAM2_TOOLBAR)) > 0)
		LoadInActions(201);
	else
		SetDefaults(201,mMod->hSelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,202,mMod->hSelParams,GetDlgItem(mMod->hSelParams,IDC_SELPARAM3_TOOLBAR)) > 0)
		LoadInActions(202);
	else
		SetDefaults(202,mMod->hSelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,204,mMod->hSelParams,GetDlgItem(mMod->hSelParams,IDC_SELMIRROR_TOOLBAR)) > 0)
		LoadInActions(204);
	else
		SetDefaults(204,mMod->hSelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,205,mMod->hSelParams,GetDlgItem(mMod->hSelParams,IDC_SELPARAM5_TOOLBAR)) > 0)
		LoadInActions(205);
	else
		SetDefaults(205,mMod->hSelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile, 206, mMod->hSelParams, GetDlgItem(mMod->hSelParams, IDC_MIRROR_THRESHOLD_TOOLBAR)) > 0)
		LoadInActions(206);
	else
		SetDefaults(206, mMod->hSelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,210,mMod->hEditUVParams,GetDlgItem(mMod->hEditUVParams,IDC_EDITPARAM1_TOOLBAR)) > 0)
		LoadInActions(210);
	else
		SetDefaults(210,mMod->hEditUVParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,211,mMod->hEditUVParams,GetDlgItem(mMod->hEditUVParams,IDC_EDITPARAM2_TOOLBAR)) > 0)
		LoadInActions(211);
	else
		SetDefaults(211,mMod->hEditUVParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,212,mMod->hEditUVParams,GetDlgItem(mMod->hEditUVParams,IDC_EDITPARAM3_TOOLBAR)) > 0)
		LoadInActions(212);
	else
		SetDefaults(212,mMod->hEditUVParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,230,mMod->hPeelParams,GetDlgItem(mMod->hPeelParams,IDC_PEELPARAM1_TOOLBAR)) > 0)
		LoadInActions(230);
	else
		SetDefaults(230,mMod->hPeelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,231,mMod->hPeelParams,GetDlgItem(mMod->hPeelParams,IDC_PEELPARAM2_TOOLBAR)) > 0)
		LoadInActions(231);
	else
		SetDefaults(231,mMod->hPeelParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,240,mMod->hMapParams,GetDlgItem(mMod->hMapParams,IDC_MAPPARAM1_TOOLBAR)) > 0)
		LoadInActions(240);
	else
		SetDefaults(240,mMod->hMapParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,241,mMod->hMapParams,GetDlgItem(mMod->hMapParams,IDC_MAPPARAM2_TOOLBAR)) > 0)
		LoadInActions(241);
	else
		SetDefaults(241,mMod->hMapParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,242,mMod->hMapParams,GetDlgItem(mMod->hMapParams,IDC_MAPPARAM3_TOOLBAR)) > 0)
		LoadInActions(242);
	else
		SetDefaults(242,mMod->hMapParams);

	if (bResult && pUIManager->LoadSingleFromIniFile(iniFile,250,mMod->hWrapParams,GetDlgItem(mMod->hWrapParams,IDC_WRAPPARAM1_TOOLBAR)) > 0)
		LoadInActions(250);
	else
		SetDefaults(250,mMod->hWrapParams);

	ReleaseIRollup(irollup);

	pUIManager->UpdateCheckButtons();

	//add the toolbars
}
void ModifierPanelUI::TearDown()
{
	
	IRollupWindow *irollup = GetIRollup(mRollupHwnd);

	mMod->GetUIManager()->Free(mMod->hSelParams);
	mMod->GetUIManager()->Free(mMod->hMatIdParams);
	mMod->GetUIManager()->Free(mMod->hEditUVParams);
	mMod->GetUIManager()->Free(mMod->hPeelParams);
	mMod->GetUIManager()->Free(mMod->hMapParams);
	mMod->GetUIManager()->Free(mMod->hWrapParams);

	ReleaseIRollup(irollup);
}

void ModifierPanelUI::CreateDefaultToolBars()
{
	
}
