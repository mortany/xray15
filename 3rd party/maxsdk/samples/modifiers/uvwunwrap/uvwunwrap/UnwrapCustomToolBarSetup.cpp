/*

Copyright 2010 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

/*

add method to get corner heights
add missing actions

*/

#include "Unwrap.h"
#include "modsres.h"

void UnwrapMod::fnAddToolBar(int owner, const MCHAR* name, int pos, int x, int y, int width, BOOL popup)
{
	bool found = false;
	int ct = mUIManager.NumberToolBars();
	for (int i = 0; i < ct; i++)
	{
		const MCHAR* testName = mUIManager.Name(i);
		if (_tcscmp(name, testName) == 0)
		{
			found = true;
			i = ct;
		}
	}

	if (!found)
	{
		if (hDialogWnd)
		{
			AppendCustomToolBar(owner, name, pos, x, y, width, popup, false);
		}
	}
}

void UnwrapMod::AppendCustomToolBar(int owner, const MCHAR* name, int pos, int x, int y, int width, BOOL popup, bool bRescale)
{
	HWND parentHWND = hDialogWnd;
	width = bRescale?MaxSDK::UIScaled(width):width;
	int height = bRescale ? MaxSDK::UIScaled(iToolBarHeight):iToolBarHeight;
	if ((owner >= 100) && (owner <= 199))
	{
		int ownerID = owner - 100;
		IRollupWindow *irollup = GetIRollup(GetDlgItem(hDialogWnd, IDC_SIDEBAR_ROLLOUT));
		if (irollup)
		{
			parentHWND = irollup->GetPanelDlg(ownerID);
			ReleaseIRollup(irollup);
		}
		else
			parentHWND = NULL;
	}
	else if ((owner >= 200) && (owner <= 299))
	{
		parentHWND = GetParent(hSelParams);
	}

	if (parentHWND)
	{
		if (popup)
			mUIManager.AppendToolBar(owner, parentHWND, name, pos, x, y, width, height, true);
		else
			mUIManager.AppendToolBar(owner, parentHWND, name, pos, x, y, width, height, false);

		SetupDefaultWindows();


		if (hDialogWnd)
		{
			SizeDlg();
			InvalidateView();
		}
	}
}

void UnwrapMod::fnShowToolBar(BOOL visible)
{}

void UnwrapMod::UpdateToolBars()
{
	mUIManager.UpdatePositions();
}

void	UnwrapMod::SetupTransformToolBar(ToolBarFrame *toolBarFrame)
{
	AddActionToToolbar(toolBarFrame, ID_MOVE, true);
	AddActionToToolbar(toolBarFrame, ID_ROTATE, true);
	AddActionToToolbar(toolBarFrame, ID_SCALE, true);
	AddActionToToolbar(toolBarFrame, ID_FREEFORMMODE, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);
	AddActionToToolbar(toolBarFrame, ID_MIRROR, true);
}

void	UnwrapMod::SetupOptionToolBar(ToolBarFrame *toolBarFrame)
{
	AddActionToToolbar(toolBarFrame, ID_SHOWMULTITILE, true);
	AddActionToToolbar(toolBarFrame, ID_SHOWMAP, true);
	AddActionToToolbar(toolBarFrame, ID_UVW, true);
	AddActionToToolbar(toolBarFrame, ID_PROPERTIES, true);
	AddActionToToolbar(toolBarFrame, ID_TEXTURE_COMBO, true);
}


void	UnwrapMod::SetupTypeInToolBar(ToolBarFrame *toolBarFrame)
{
	AddActionToToolbar(toolBarFrame, ID_ABSOLUTETYPEIN, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);
	AddActionToToolbar(toolBarFrame, ID_ABSOLUTETYPEIN_SPINNERS, true);
}

void	UnwrapMod::SetupViewToolBar(ToolBarFrame *toolBarFrame)
{
	AddActionToToolbar(toolBarFrame, ID_LOCKSELECTED, true);
	AddActionToToolbar(toolBarFrame, ID_FILTERSELECTED, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);
	AddActionToToolbar(toolBarFrame, ID_HIDE, true);
	AddActionToToolbar(toolBarFrame, ID_FREEZE, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);

	AddActionToToolbar(toolBarFrame, ID_FILTER_MATID, true);


	AddActionToToolbar(toolBarFrame, ID_PAN, true);
	AddActionToToolbar(toolBarFrame, ID_ZOOMTOOL, true);
	AddActionToToolbar(toolBarFrame, ID_ZOOMREGION, true);
	AddActionToToolbar(toolBarFrame, ID_ZOOMEXTENT, true);
	AddActionToToolbar(toolBarFrame, ID_SNAP, true);
}

void	UnwrapMod::SetupSelectToolBar(ToolBarFrame *toolBarFrame)
{
	AddActionToToolbar(toolBarFrame, ID_TV_VERTMODE, true);
	AddActionToToolbar(toolBarFrame, ID_TV_EDGEMODE, true);
	AddActionToToolbar(toolBarFrame, ID_TV_FACEMODE, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);
	AddActionToToolbar(toolBarFrame, ID_TV_ELEMENTMODE, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);
	//subobject modes

	AddActionToToolbar(toolBarFrame, ID_TV_INCSEL, true);
	AddActionToToolbar(toolBarFrame, ID_TV_DECSEL, true);

	AddActionToToolbar(toolBarFrame, ID_TV_LOOP, true);
	AddActionToToolbar(toolBarFrame, ID_TV_LOOPGROW, true);
	AddActionToToolbar(toolBarFrame, ID_TV_LOOPSHRINK, true);

	AddActionToToolbar(toolBarFrame, ID_TV_RING, true);
	AddActionToToolbar(toolBarFrame, ID_TV_RINGGROW, true);
	AddActionToToolbar(toolBarFrame, ID_TV_RINGSHRINK, true);


	AddActionToToolbar(toolBarFrame, ID_TV_PAINTSELECTMODE, true);
	AddActionToToolbar(toolBarFrame, ID_TV_PAINTSELECTINC, true);
	AddActionToToolbar(toolBarFrame, ID_TV_PAINTSELECTDEC, true);
}
void	UnwrapMod::SetupSoftSelectToolBar(ToolBarFrame *toolBarFrame)
{
	AddActionToToolbar(toolBarFrame, ID_SOFTSELECTION, true);
	AddActionToToolbar(toolBarFrame, ID_SOFTSELECTIONSTR, true);
	AddActionToToolbar(toolBarFrame, ID_FALLOFF, true);
	AddActionToToolbar(toolBarFrame, ID_FALLOFF_SPACE, true);
	AddActionToToolbar(toolBarFrame, ID_SEPARATORBAR, true);
	AddActionToToolbar(toolBarFrame, ID_LIMITSOFTSEL, true);
	AddActionToToolbar(toolBarFrame, ID_SOFTSELECTIONLIMIT, true);
}


void    UnwrapMod::SetupDefaultWindows()
{
	ToolBarFrame *bar = mUIManager.GetToolBarFrame(_T("_InternalTransform"));
	if (bar)
	{
		if (bar->NumItems() == 0)
			SetupTransformToolBar(bar);
	}
	bar = mUIManager.GetToolBarFrame(_T("_InternalOption"));
	if (bar)
	{
		if (bar->NumItems() == 0)
			SetupOptionToolBar(bar);
	}

	bar = mUIManager.GetToolBarFrame(_T("_InternalTypeIn"));
	if (bar)
	{
		if (bar->NumItems() == 0)
			SetupTypeInToolBar(bar);
	}

	bar = mUIManager.GetToolBarFrame(_T("_InternalView"));
	if (bar)
	{
		if (bar->NumItems() == 0)
			SetupViewToolBar(bar);
	}

	bar = mUIManager.GetToolBarFrame(_T("_InternalSelect"));
	if (bar)
	{
		if (bar->NumItems() == 0)
			SetupSelectToolBar(bar);
	}

	bar = mUIManager.GetToolBarFrame(_T("_InternalSoftSelect"));
	if (bar)
	{
		if (bar->NumItems() == 0)
			SetupSoftSelectToolBar(bar);
	}
}

void    UnwrapMod::FillOutToolBars()
{
	int numBars = mUIManager.NumberToolBars();
	if (mUIManager.GetToolBarFrame(_T("_InternalTransform")) == NULL)
	{
		//add our default bars
		AppendCustomToolBar(0, _T("_InternalTransform"), 0, 0, 0, 148, TRUE, true);
		AppendCustomToolBar(0, _T("_InternalOption"), 1, 0, 0, 255 + 16, TRUE, true);

		AppendCustomToolBar(0, _T("_InternalTypeIn"), 2, 0, 0, 232, TRUE, true);
		AppendCustomToolBar(0, _T("_InternalSelect"), 2, 0, 0, 420, TRUE, true);

		AppendCustomToolBar(0, _T("_InternalView"), 3, 0, 0, 400 + 16, TRUE, true);
		AppendCustomToolBar(0, _T("_InternalSoftSelect"), 3, 0, 0, 246 - 16, TRUE, true);
	}
	else
	{
		for (int i = 0; i < numBars; i++)
		{
			const MCHAR *name = mUIManager.Name(i);
			ToolBarFrame *bar = mUIManager.GetToolBarFrame(name);
			Tab<int> ids;
			ids = bar->LoadIDs();
			for (int j = 0; j < ids.Count(); j++)
			{
				int id = ids[j];

				AddActionToToolbar(bar, id);
			}
			bar->LoadIDsClear();
		}
	}
}

void    UnwrapMod::SetupToolBarUIs()
{
	//set up toolbar	
	mUIManager.Load(mToolBarIniFileName, 0, hDialogWnd);
	FillOutToolBars();
	SetupDefaultWindows();

	mSideBarUI.Setup(hInstance, GetDlgItem(hDialogWnd, IDC_SIDEBAR_ROLLOUT), mToolBarIniFileName);
}
void     UnwrapMod::TearDownToolBarUIs()
{
	//save our toolbar data out to an ini file
	mUIManager.Save(mToolBarIniFileName);
	mUIManager.Free(hDialogWnd);

	mSideBarUI.TearDown();
}



