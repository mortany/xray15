//
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.   
//
//

#include "UnwrapCustomToolBars.h"
#include "modsres.h"
#include "unwrap.h"

#define _CRT_SECURE_NO_WARNINGS

PeelModeDialog& PeelModeDialog::GetSingleton()
{
	static PeelModeDialog sPeelModeDialog;
	return sPeelModeDialog;
}

PeelModeDialog::PeelModeDialog() : mbAutoPack(false), mbLivePeel(false), mbEdgeToSeam(false),mhDlg(nullptr),
	mpUnwrapMod(nullptr)
{
	if (GetCOREInterface())
	{
		TCHAR cfgFile[1024] = { 0 };
		wcscpy_s(cfgFile, GetCOREInterface()->GetDir(APP_PLUGCFG_DIR));
		mIniFileName.printf(_T("%s\\UnwrapPeelMode.ini"), cfgFile);
	}
}

PeelModeDialog::~PeelModeDialog()
{}

static INT_PTR CALLBACK PeelModeDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static PeelModeDialog* spPeelModeDialog = nullptr;
	switch (msg) {
	case WM_INITDIALOG:
		{
			spPeelModeDialog = (PeelModeDialog*)lParam;
			if(nullptr == spPeelModeDialog) return FALSE;
			spPeelModeDialog->LoadIniFile();
			spPeelModeDialog->SetDlgWnd(hWnd);
			spPeelModeDialog->UpdateUI();
			spPeelModeDialog->GetGuard().IncrementPeelModeDialogPopup();
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CHECK_AUTO_PACK:
			spPeelModeDialog->SetAutoPack(GetCheckBox(hWnd, IDC_CHECK_AUTO_PACK) ? true : false);
			break;
		case IDC_CHECK_LIVE_PEEL:
			spPeelModeDialog->SetLivePeel(GetCheckBox(hWnd, IDC_CHECK_LIVE_PEEL) ? true : false);
			break;
		case IDC_CHECK_EDGE_TO_SEAM:
			spPeelModeDialog->SetEdgeToSeam(GetCheckBox(hWnd, IDC_CHECK_EDGE_TO_SEAM) ? true : false);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		spPeelModeDialog->GetGuard().DecrementPeelModeDialogPopup();
		spPeelModeDialog->SaveIniFile();
		spPeelModeDialog->SetDlgWnd(nullptr);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL PeelModeDialog::CreatePeelModeDialog(HINSTANCE instance, HWND parentWnd)
{
	if(mGuard.CanPopupPeelModeDialog())
	{
		HWND hPeelModeDialog = CreateDialogParam(
			instance,
			MAKEINTRESOURCE(IDD_DLG_PEEL_MODE),
			parentWnd,
			PeelModeDlgProc,
			(LONG_PTR)this);
		ShowWindow(hPeelModeDialog, SW_SHOW);
		return TRUE;
	}
	return FALSE;
}

bool PeelModeDialog::GetAutoPack() const
{
	return mbAutoPack;
}

void PeelModeDialog::SetAutoPack(bool bAutoPack)
{
	if(mbAutoPack != bAutoPack)
	{
		mbAutoPack = bAutoPack;
		UpdateUI();
	}
}

bool PeelModeDialog::GetLivePeel() const
{
	return mbLivePeel;
}

void PeelModeDialog::SetLivePeel(bool bLivePeel)
{
	if(mbLivePeel != bLivePeel)
	{
		mbLivePeel = bLivePeel;
		if(!mbLivePeel && mpUnwrapMod && (mpUnwrapMod->fnGetMapMode() == LSCMMAP))
		{
			mpUnwrapMod->fnSetMapMode(NOMAP);
		}
		else if(mbLivePeel && mpUnwrapMod && (mpUnwrapMod->fnGetTVSubMode() == TVVERTMODE || mpUnwrapMod->fnGetTVSubMode() == TVFACEMODE))
		{
			// when we use live peel, edge selection would be auto transformed into from non-edge sub object level.
			mpUnwrapMod->fnSetTVSubMode(TVEDGEMODE);
		}
		UpdateUI();
	}
}

bool PeelModeDialog::GetEdgeToSeam() const
{
	return mbEdgeToSeam;
}

void PeelModeDialog::SetEdgeToSeam(bool bEdgeToSeam)
{
	if(mbEdgeToSeam != bEdgeToSeam)
	{
		mbEdgeToSeam = bEdgeToSeam;
		UpdateUI();
	}
}

void PeelModeDialog::SetMod(UnwrapMod* pMod)
{
	if(pMod != nullptr)
	{
		mpUnwrapMod = pMod;
	}
}

UnwrapMod* PeelModeDialog::GetMod()
{
	return mpUnwrapMod;
}

HWND PeelModeDialog::GetDlgWnd() const
{
	return mhDlg;
}

void PeelModeDialog::SetDlgWnd(HWND hDlg)
{
	mhDlg = hDlg;
}

bool PeelModeDialog::LoadIniFile()
{
	if(mIniFileName.length() == 0)
	{
		return false;
	}
	
	if(mIniFileIO.InitCacheFromIniFile(mIniFileName))
	{
		int avoidOverlap = mIniFileIO.GetIntFromSectionKey(_T("UnwrapLivePeel"), _T("AvoidOverlap"));
		int livePeel = mIniFileIO.GetIntFromSectionKey(_T("UnwrapLivePeel"), _T("LivePeel"));
		int edgeToSeam = mIniFileIO.GetIntFromSectionKey(_T("UnwrapLivePeel"), _T("EdgeToSeam"));
		mbAutoPack = (avoidOverlap == 0) ? false : true;
		mbLivePeel = (livePeel == 0) ? false : true;
		mbEdgeToSeam = (edgeToSeam == 0) ? false : true;
		return true;
	}
	return false;
}

bool PeelModeDialog::SaveIniFile()
{
	if(mIniFileName.length() == 0)
	{
		return false;
	}

	int avoidOverlap = mbAutoPack ? 1 : 0;
	int livePeel = mbLivePeel ? 1 : 0;
	int edgeToSeam = mbEdgeToSeam ? 1 : 0;
	TSTR strAvoidOverlap, strLivePeel, strEdgeToSeam;
	strAvoidOverlap.printf(_T("%d"), avoidOverlap);
	strLivePeel.printf(_T("%d"), livePeel);
	strEdgeToSeam.printf(_T("%d"), edgeToSeam);
	mIniFileIO.AddSectionKeyValue(_T("UnwrapLivePeel"), _T("AvoidOverlap"), strAvoidOverlap);
	mIniFileIO.AddSectionKeyValue(_T("UnwrapLivePeel"), _T("LivePeel"), strLivePeel);
	mIniFileIO.AddSectionKeyValue(_T("UnwrapLivePeel"), _T("EdgeToSeam"), strEdgeToSeam);
	mIniFileIO.SaveAs(mIniFileName);
	return true;
}

void PeelModeDialog::UpdateUI()
{
	if(mhDlg)
	{
		SetCheckBox(mhDlg, IDC_CHECK_AUTO_PACK, mbAutoPack ? TRUE : FALSE);
		SetCheckBox(mhDlg, IDC_CHECK_LIVE_PEEL, mbLivePeel ? TRUE : FALSE);
		SetCheckBox(mhDlg, IDC_CHECK_EDGE_TO_SEAM, mbEdgeToSeam ? TRUE : FALSE);
		if(mbLivePeel)
		{
			SetEdgeToSeam(true);
			EnableWindow(GetDlgItem(mhDlg, IDC_CHECK_EDGE_TO_SEAM), FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(mhDlg, IDC_CHECK_EDGE_TO_SEAM), TRUE);
			SetCheckBox(mhDlg, IDC_CHECK_EDGE_TO_SEAM, mbEdgeToSeam ? TRUE : FALSE);
		}
	}
}

PeelModeDialog::PeelModeDialogPopupGuard& PeelModeDialog::GetGuard()
{
	return mGuard;
}

PeelModeDialog::PeelModeDialogPopupGuard::PeelModeDialogPopupGuard() : mGuardPeelModeDialogPopup(0)
{
}

PeelModeDialog::PeelModeDialogPopupGuard::~PeelModeDialogPopupGuard()
{
}

void PeelModeDialog::PeelModeDialogPopupGuard::IncrementPeelModeDialogPopup()
{
	mGuardPeelModeDialogPopup++;
}

void PeelModeDialog::PeelModeDialogPopupGuard::DecrementPeelModeDialogPopup()
{
	--mGuardPeelModeDialogPopup;
}

bool PeelModeDialog::PeelModeDialogPopupGuard::CanPopupPeelModeDialog()
{
	return mGuardPeelModeDialogPopup == 0 ;
}



