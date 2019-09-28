//**************************************************************************/
// Copyright (c) 1998-2010 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Unwrap UI classes
// AUTHOR: Peter Watje
// DATE: 2010/08/31 
//***************************************************************************/

#pragma once

#include <windowsx.h>
#include "custcont.h"
#include "IniFileIO.h"
#include <winutil.h>


class UnwrapMod;

/* 
this is a wrapper around the Max ICustToolbar

*/
class ToolBarFrame
{
public:

	enum DockPos {kDockUpperLeft,kDockUpperRight, kDockLowerLeft, kDockLowerRight, kFloat};

	ToolBarFrame( HWND hwnd, int owner, const TSTR &name);

	ToolBarFrame(HINSTANCE instance, int owner, HWND parentWindow, int x, int y, int w, int h, DockPos location, bool popup, const TSTR &name);
	ToolBarFrame(HINSTANCE instance, const MCHAR* section, IniFileIO& iniFileIO, HWND parent, HWND toolbarHWND);
	~ToolBarFrame();

	void UpdateWindow();
	
	void ResizeWindow(int x, int y, int w, int h, BOOL save = TRUE);

	HWND GetParWindow();
	HWND GetToolbarWindow();

	ICustToolbar* GetToolBar();

	DockPos Location();
	int Width();
	int Height();

	void Show(bool vis);

	void Save(const MCHAR* section, IniFileIO& iniFileIO);
	void Load(const MCHAR* section, IniFileIO& iniFileIO); 

	//Frees the CUI Frame and Toolbar
	void Free();

	bool Loaded();

	const MCHAR* Name();
	int	NumItems();

	int Owner();
	Tab<int> LoadIDs();
	void LoadIDsClear();

	void AddButton(int id, ToolButtonItem item, FlyOffIconList& iconList, int flyOffValue); // for flyout buttons
	void AddButton(int id, ToolButtonItem item, const MCHAR* toolTip); // for single icon buttons
protected:
	ToolBarFrame();

	void Init(HINSTANCE hInstance);

	//creates the toolbar if toolBarHWND is NULL a window will be created in its place
	void Create(HWND toolbarHWND);

	HWND mParentWindow;

	HWND mDummyWindow;
	HWND mPopUpWindow;

	HWND mToolBarWindow;
	ICustToolbar* mToolBar;

	int mOwner;
	TSTR mName;
	DockPos		mLocation;    
	int		mX,mY,mW,mH;
	bool	mPopup;

	bool mCenter;

	HINSTANCE mhInstance;

	bool mLoaded;
	HIMAGELIST mImages;

	Tab<int> mLoadIds;

};

class PeelModeDialog : public MaxSDK::Util::Noncopyable
{
public:
	PeelModeDialog();
	~PeelModeDialog();

	BOOL CreatePeelModeDialog(HINSTANCE instance, HWND parentWnd);
	static PeelModeDialog& GetSingleton();

	bool GetAutoPack() const;
	void SetAutoPack(bool bAutoPack);
	bool GetLivePeel() const;
	void SetLivePeel(bool bLivePeel);
	bool GetEdgeToSeam() const;
	void SetEdgeToSeam(bool bEdgeToSeam);
	void SetMod(UnwrapMod* pMod);
	UnwrapMod* GetMod();

	HWND GetDlgWnd() const;
	void SetDlgWnd(HWND hDlg);

	bool LoadIniFile();
	bool SaveIniFile();

	void UpdateUI();

	class PeelModeDialogPopupGuard : public MaxSDK::Util::Noncopyable
	{
	public:
		PeelModeDialogPopupGuard();
		~PeelModeDialogPopupGuard();
		bool CanPopupPeelModeDialog();
		void IncrementPeelModeDialogPopup();
		void DecrementPeelModeDialogPopup();
	private:
		//If the value is the initial 0, then peel mode dialog can be created
		int mGuardPeelModeDialogPopup;
	};
	PeelModeDialogPopupGuard& GetGuard();

private:
	bool mbAutoPack;
	bool mbLivePeel;
	bool mbEdgeToSeam;
	HWND mhDlg;
	PeelModeDialogPopupGuard mGuard;
	IniFileIO mIniFileIO;
	TSTR mIniFileName;
	UnwrapMod* mpUnwrapMod;
};

ToolTipExtender& GetToolTipExtender();
