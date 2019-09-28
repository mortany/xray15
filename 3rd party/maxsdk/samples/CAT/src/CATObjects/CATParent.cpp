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

// This guy sholds all character related information and is a general character manager

// CAT Stuff
#include "../CATControls/CatPlugins.h"
#include "CATParent.h"

// Max Stuff
#include "notify.h"
#include <direct.h>
#include "MaxIcon.h"

#include "../CATControls/CATDialog.h"
#include "../CATControls/CATFilePaths.h"

// Layers
#include "../CATControls/CATClipRoot.h"
#include "../CATControls/LayerTransform.h"

// CATMotion
#include "../CATControls/CATHierarchyRoot.h"
#include "../CATControls/CATMotionLayer.h"
#include "../CATControls/ease.h"

// Rig structure
#include "../CATControls/CATParentTrans.h"
#include "../CATControls/CATNodeControl.h"

/**********************************************************************
* Class description
*/

//keeps track of whether an FP interface desc has been added to the CATParents ClassDesc
static bool catparentInterfacesAdded = false;

class CATParentClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) {
		CATParent *catparent = new CATParent(loading);
		if (!catparentInterfacesAdded) {
			// here we add the clip operations to the CATParent
			AddInterface(ICATParentFP::GetFnPubDesc());
			AddInterface(GetCATClipRootDesc()->GetInterface(LAYERROOT_INTERFACE_FP));
			catparentInterfacesAdded = true;
		}
		return catparent;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATPARENT); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return CATPARENT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATParent"); }		// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};

static CATParentClassDesc CATParentDesc;
ClassDesc2* GetCATParentDesc() { return &CATParentDesc; }

//
// The Main CAT ui
//
class CATRollout
{
private:
	CATParent		*mpCATParent;
	HWND			mhWnd;

	ICustEdit		*mEdtCATName;
	ICustButton		*mBtnCreateRootNode;
	ISpinnerControl	*mSpnCATUnits;

public:

	HWND GetHWnd() { return mhWnd; }

	CATParent* GetCATParent() { return mpCATParent; }
	void SetCATParent(CATParent *newcatparent)
	{
		// Do not take ownership of something that doesn't exist
		if (mhWnd == NULL)
			return;

		mpCATParent = newcatparent;

		Refresh();
	}

	CATClipRoot* GetLayerRoot()
	{
		if (mpCATParent != NULL)
		{
			ICATParentTrans* catParentTrans = mpCATParent->GetCATParentTrans();
			if (catParentTrans)
				return static_cast<CATClipRoot*>(catParentTrans->GetLayerRoot());
		}
		return NULL;
	}

	CATRollout()
	{
		mpCATParent = NULL;

		mhWnd = NULL;

		mEdtCATName = NULL;
		mBtnCreateRootNode = NULL;
		mSpnCATUnits = NULL;
	};

	void Refresh() {
		if (mpCATParent == NULL || mhWnd == NULL)
			return;

		ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
		bool bHasLayers = false;
		if (pCATParentTrans)
		{
			CATClipRoot* layerroot = GetLayerRoot();

			mEdtCATName->Enable();
			mSpnCATUnits->Enable();
			mBtnCreateRootNode->Enable();

			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_ACTIVEAYER), TRUE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_ANY_CONTRIBUTING_LAYER), TRUE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_ALL_LAYERS), TRUE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_X_AXIS), TRUE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_Z_AXIS), TRUE);

			mEdtCATName->SetText(pCATParentTrans->GetCATName());
			mSpnCATUnits->SetValue(pCATParentTrans->GetCATUnits(), FALSE);
			mBtnCreateRootNode->SetCheck(pCATParentTrans->GetRootNode() == NULL ? FALSE : TRUE);

			int tdm = layerroot->GetTrackDisplayMethod();
			SET_CHECKED(mhWnd, IDC_RDO_ACTIVEAYER, tdm == TRACK_DISPLAY_METHOD_ACTIVE);
			SET_CHECKED(mhWnd, IDC_RDO_ANY_CONTRIBUTING_LAYER, tdm == TRACK_DISPLAY_METHOD_CONTRIBUTING);
			SET_CHECKED(mhWnd, IDC_RDO_ALL_LAYERS, tdm == TRACK_DISPLAY_METHOD_ALL);

			int lengthaxis = pCATParentTrans->GetLengthAxis();
			SET_CHECKED(mhWnd, IDC_RDO_X_AXIS, lengthaxis == X);
			SET_CHECKED(mhWnd, IDC_RDO_Z_AXIS, lengthaxis == Z);

			bHasLayers = layerroot->NumLayers() > 0;
		}
		else // We havn't been created yetCreatePel
		{
			mEdtCATName->Disable();
			mBtnCreateRootNode->Disable();

			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_ACTIVEAYER), TRUE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_ANY_CONTRIBUTING_LAYER), TRUE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_ALL_LAYERS), TRUE);
		}

		if (!pCATParentTrans || bHasLayers)
		{
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_X_AXIS), FALSE);
			EnableWindow(GetDlgItem(mhWnd, IDC_RDO_Z_AXIS), FALSE);
		}
	}

	BOOL InitControls(HWND hDlg, CATParent *catParent)
	{
		if (this->mhWnd) return FALSE;
		this->mpCATParent = catParent;
		ICATParentTrans* catparenttrans = mpCATParent->GetCATParentTrans();

		mhWnd = hDlg;

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		mEdtCATName = GetICustEdit(GetDlgItem(hDlg, IDC_EDIT_CATNAME));
		mSpnCATUnits = SetupFloatSpinner(hDlg, IDC_SPIN_CATUNITS, IDC_EDIT_CATUNITS, 0.0f, 1000.0f, catparenttrans ? catparenttrans->GetCATUnits() : 1.0f);
		mSpnCATUnits->SetAutoScale(TRUE);

		mBtnCreateRootNode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_CREATE_ROOT_NODE));
		mBtnCreateRootNode->SetType(CBT_CHECK);
		mBtnCreateRootNode->SetButtonDownNotify(TRUE);

		Refresh();
		return TRUE;
	}

	void ReleaseControls()
	{
		mpCATParent = NULL;
		mhWnd = NULL;

		SAFE_RELEASE_EDIT(mEdtCATName);
		SAFE_RELEASE_BTN(mBtnCreateRootNode);

		SAFE_RELEASE_SPIN(mSpnCATUnits);
	};

	//	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	//	{
	//		switch(msg)
	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		BOOL ret = TRUE;
		switch (message)
		{
		case WM_INITDIALOG:
			if (!InitControls(hWnd, (CATParent*)lParam)) DestroyWindow(hWnd);
			// Return FALSE to prevent the system from setting the default keyboard focus
			// which would be the name edit control. It's to avoid the name being changed
			// accidently.
			ret = FALSE;
			break;
		case WM_DESTROY:
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_RDO_ACTIVEAYER:
				{
					CATClipRoot* layerroot = GetLayerRoot();
					if (layerroot)
					{
						layerroot->SetTrackDisplayMethod(TRACK_DISPLAY_METHOD_ACTIVE);
						Refresh();
					}
				}
				break;
				case IDC_RDO_ANY_CONTRIBUTING_LAYER:
				{
					CATClipRoot* layerroot = GetLayerRoot();
					if (layerroot)
					{
						layerroot->SetTrackDisplayMethod(TRACK_DISPLAY_METHOD_CONTRIBUTING);
						Refresh();
					}
				}
				break;
				case IDC_RDO_ALL_LAYERS:
				{
					CATClipRoot* layerroot = GetLayerRoot();
					if (layerroot)
					{
						layerroot->SetTrackDisplayMethod(TRACK_DISPLAY_METHOD_ALL);
						Refresh();
					}
				}
				break;
				case IDC_RDO_X_AXIS:
				{
					if (mpCATParent != NULL)
					{
						ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
						if (pCATParentTrans != NULL)
						{
							pCATParentTrans->SetLengthAxis(X);
							Refresh();
						}
					}
				}
				break;
				case IDC_RDO_Z_AXIS:
				{
					if (mpCATParent != NULL)
					{
						ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
						if (pCATParentTrans != NULL)
						{
							pCATParentTrans->SetLengthAxis(Z);
							Refresh();
						}
					}
				}
				break;
				case IDC_BTN_CREATE_ROOT_NODE:
				{
					if (mpCATParent != NULL)
					{
						ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
						if (pCATParentTrans != NULL)
						{
							if (mBtnCreateRootNode->IsChecked()) {
								pCATParentTrans->AddRootNode();
							}
							else {
								pCATParentTrans->RemoveRootNode();
							}
							Refresh();
						}
					}
				}
				break;
				}
				break;
			}
			break;
		case CC_SPINNER_BUTTONDOWN:
			if (!theHold.Holding())	theHold.Begin();
			break;
		case CC_SPINNER_CHANGE:
			switch (LOWORD(wParam)) {
			case IDC_SPIN_CATUNITS:
				if (mpCATParent != NULL)
				{
					ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
					if (pCATParentTrans != NULL)
					{
						HoldActions hold(IDS_HLD_CATUNITS);
						pCATParentTrans->SetCATUnits(mSpnCATUnits->GetFVal());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
				}
				break;
			}
			break;
		case CC_SPINNER_BUTTONUP:
			if (theHold.Holding())
			{
				if (HIWORD(wParam)) {
					theHold.Accept(GetString(IDS_HLD_CATUNITS));
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
				}
				else				theHold.Cancel();
			}
			break;
		case WM_CUSTEDIT_ENTER:
		{
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_CATNAME:
				if (mEdtCATName != NULL)
				{
					if (mpCATParent != NULL)
					{
						ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
						if (pCATParentTrans != NULL)
						{
							TCHAR strbuf[128];
							mEdtCATName->GetText(strbuf, 128);
							pCATParentTrans->SetCATName(TSTR(strbuf));
						}
					}
				}
			}
		}

		case WM_NOTIFY:
			break;

		default:
			return FALSE;

		} // end switch

		return ret;
	};
	virtual void DeleteThis() { }//delete this;
};

// CATParentParams
static CATRollout staticCATRollout;

static INT_PTR CALLBACK CATRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticCATRollout.DlgProc(hWnd, message, wParam, lParam);
};

static ParamBlockDesc2 CATParentParams(CATParent::ID_CATPARENT_PARAMS, _T("CAT Character Parameters"), 0, &CATParentDesc,
	P_AUTO_CONSTRUCT, CATParent::REF_CATPARENT_PARAMS,
	CATParent::PB_CATNAME, _T(""), TYPE_STRING, P_RESET_DEFAULT, 0, p_default, _T("CATRig"), p_end,
	CATParent::PB_CATMODE, _T(""), TYPE_INT, P_RESET_DEFAULT, 0, p_end,
	CATParent::PB_CATUNITS, _T(""), TYPE_FLOAT, P_RESET_DEFAULT, 0, p_ms_default, 1.0f, p_end,
	CATParent::PB_CATVISIBILITY, _T(""), TYPE_FLOAT, P_RESET_DEFAULT, 0, p_end,
	CATParent::PB_CATRENDERABLE, _T(""), TYPE_BOOL, P_RESET_DEFAULT, 0, p_end,
	CATParent::PB_CATFROZEN, _T(""), TYPE_BOOL, P_RESET_DEFAULT, 0, p_end,
	CATParent::PB_CATHIERARCHY, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0, p_end,
	CATParent::PB_CLIPHIERARCHY, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0, p_end,
	p_end
);

/************************************************************************/
/*   CATRigParams: pblock used to display the CATRig presets            */
/************************************************************************/

enum CATRigParams {
	PB_RIG_FILENAME
};

/**********************************************************************
* Dialog callback for CATRigParams.
* Allows saving and loading of rig presets (*.cat).
*/

// to reduce confusion
#define CANLOAD  FALSE
#define CANSAVE  TRUE

class CATRigRollout {
	HWND hWnd;
	CATParent *catparent;

	HWND hListPresets;
	ICustButton* btnLoadRig;
	ICustButton* btnSaveRig;
	ICustButton* btnCreatePelvis;
	ICustButton* btnERN;

	std::vector<TCHAR*> vecFileNames;
	std::vector<TCHAR*> vecFolderNames;

	TCHAR lastSelectedFilename[MAX_PATH];
	TCHAR strCurrentFolder[MAX_PATH];

	bool bShowFolders;

	TSTR	selectedRigFile;

public:

	CATRigRollout() {
		hWnd = NULL;
		catparent = NULL;
		hListPresets = NULL;
		btnLoadRig = NULL;
		btnSaveRig = NULL;
		btnCreatePelvis = NULL;
		btnERN = NULL;

		bShowFolders = true;
		_tcscpy(strCurrentFolder, _T(""));
	}

	HWND GetHWnd() { return hWnd; }

	CATParent* GetCATParent() { return catparent; }
	void SetCATParent(CATParent *newcatparent) {
		catparent = newcatparent;
		Refresh();
	};

	TSTR	GetSelectedRigFilename() { return selectedRigFile; }
	void	SetSelectedRigFilename(TSTR name) { selectedRigFile = name; }

	void Refresh() {

		if (catparent == NULL || hWnd == NULL)
			return;

		RefreshPresetList();

		ICATParentTrans* catparenttrans = catparent->GetCATParentTrans();
		if (!catparenttrans) // this only happens in the create panel when nothing has been created yet
		{
			btnCreatePelvis->SetText(GetString(IDS_BTN_CREATEPELVIS));
			btnLoadRig->Disable();
			btnSaveRig->Disable();
			btnCreatePelvis->Disable();
		}
		else
		{
			BOOL pelvisExists = catparenttrans->GetRootHub() ? TRUE : FALSE;
			if (!pelvisExists) // no pelvis
			{
				btnCreatePelvis->Enable();
				btnCreatePelvis->SetText(GetString(IDS_BTN_CREATEPELVIS));

				btnLoadRig->Enable();
				btnSaveRig->Disable();
			}
			else // pelvis exists
			{
				btnCreatePelvis->SetText(GetString(IDS_BTN_RELOAD));
				TSTR path, filename, extension;
				SplitFilename(catparent->GetRigFilename(), &path, &filename, &extension);
				extension = extension.Substr(1, extension.Length() - 1);	// Remove the '.' from '.rg3'
				if (MatchPattern(extension, TSTR(CAT3_RIG_PRESET_EXT), TRUE)) // a .rg3 file is associated with this rig
				{
					btnCreatePelvis->Enable();
				}
				else
				{
					btnCreatePelvis->Disable();
				}

				btnSaveRig->Enable();
			}
		}
	}

	BOOL InitControls(HWND hDlg, CATParent *catParent)
	{
		// NOTE - I've got weird issues with this dialog being
		// closed without destructing properly (or rather, without
		// all my pointers being NULL'ed).  Fix it.  Till
		// then, just assert if we don't seem to be in the right place.
		DbgAssert(this->hWnd == NULL);
		this->hWnd = hDlg;
		catparent = catParent;
		ICATParentTrans* catparenttrans = catparent->GetCATParentTrans();

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		hListPresets = GetDlgItem(hWnd, IDC_PRESETS_LIST);

		btnLoadRig = GetICustButton(GetDlgItem(hWnd, IDC_BTN_LOADRIG));
		btnLoadRig->SetImage(hIcons, 8, 8, 8 + 25, 8 + 25, 24, 24);
		btnLoadRig->SetType(CBT_PUSH);
		btnLoadRig->SetButtonDownNotify(TRUE);
		btnLoadRig->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnLoadRig"), GetString(IDS_TT_OPENRIGPRESET)));

		btnSaveRig = GetICustButton(GetDlgItem(hWnd, IDC_BTN_SAVERIG));
		btnSaveRig->SetImage(hIcons, 6, 6, 6 + 25, 6 + 25, 24, 24);
		btnSaveRig->SetType(CBT_PUSH);
		btnSaveRig->SetButtonDownNotify(TRUE);
		btnSaveRig->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSaveRig"), GetString(IDS_TT_SAVERIGPRESET)));

		btnCreatePelvis = GetICustButton(GetDlgItem(hWnd, IDC_BTN_CREATEPELVIS));
		btnCreatePelvis->SetType(CBT_PUSH);
		btnCreatePelvis->SetButtonDownNotify(TRUE);

		btnERN = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ERN));
		btnERN->SetType(CBT_CHECK);
		btnERN->SetButtonDownNotify(TRUE);
		if (!catparenttrans) btnERN->Disable();

		SET_CHECKED(hWnd, IDC_CHK_RELOAD_RIG, catparent->GetReloadRigOnSceneLoad());
		// initializes directory and listbox selection to the rig filename, if there is one, or the most recently selected filename
		BOOL dirfound = FALSE;
		if (catparent->GetRigFilename() != _T(""))
		{
			// the rig has a filename (could be .rig or .rg3) => remember it and change dir to it
			_tcsnccpy(lastSelectedFilename, catparent->GetRigFilename(), catparent->GetRigFilename().Length());
			TSTR path, filename, extension;
			SplitFilename(catparent->GetRigFilename(), &path, &filename, &extension);
			dirfound = ChangeDirectory(path);
		}
		else if (_tcscmp(lastSelectedFilename, _T("")) != 0 && _tcscmp(lastSelectedFilename, (GetString(IDS_NONE))) != 0)
		{
			// the rig doesn't have a filename, but there is a most recently used filename => change dir to it
			TSTR path, filename, extension;
			SplitFilename(TSTR(lastSelectedFilename), &path, &filename, &extension);
			dirfound = ChangeDirectory(path);
		}
		// if still no directory found, then change to default dirs
		if (!dirfound) dirfound = ChangeDirectory(catCfg.Get(INI_RIG_PRESET_PATH));
		if (!dirfound) ChangeDirectory(GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));

		Refresh();

		return TRUE;
	}

	void ReleaseControls(HWND hDlg) {
		if (hWnd != hDlg) return;

		ICATParentTrans* catparenttrans = catparent->GetCATParentTrans();
		if (catparenttrans) {
			ExtraRigNodes *ern = (ExtraRigNodes*)catparenttrans->GetInterface(I_EXTRARIGNODES_FP);
			// In the case of loading a rig preset after a CATParent has been created, the
			// catparenttrans controller gets muddled. We cannot assume that it is an ern controller
			if (ern) {
				ern->IDestroyERNWindow();
			}
		}

		hWnd = NULL;
		ClearFileList();
		SAFE_RELEASE_BTN(btnLoadRig);
		SAFE_RELEASE_BTN(btnSaveRig);
		SAFE_RELEASE_BTN(btnCreatePelvis);
		SAFE_RELEASE_BTN(btnERN);

		catparent = NULL;
	}

	void DoSelectionChanged() {
		int item = (int)SendMessage(hListPresets, LB_GETCURSEL, 0, 0);
		if (item != LB_ERR) {
			int nIndex = (int)SendMessage(hListPresets, LB_GETITEMDATA, item, 0);
			if (nIndex == 0) {
				SetSelectedRigFilename(_T(""));
				_tcscpy(lastSelectedFilename, GetString(IDS_NONE));
				return;
			}
			else if (nIndex > 0) {
				SetSelectedRigFilename(vecFileNames[nIndex]);
				_tcsnccpy(lastSelectedFilename, vecFileNames[nIndex], MAX_PATH);
			}
		}
	}

	void ClearFileList() {
		ULONG i;
		SendMessage(hListPresets, LB_RESETCONTENT, 0, 0);
		for (i = 0; i < vecFolderNames.size(); i++) delete[] vecFolderNames[i];
		for (i = 0; i < vecFileNames.size(); i++) delete[] vecFileNames[i];
		vecFolderNames.resize(0);
		vecFileNames.resize(0);
	}

	BOOL ChangeDirectory(const TCHAR *szNewDir) {
		TCHAR szWorkingDir[MAX_PATH];
		_tgetcwd(szWorkingDir, MAX_PATH);

		if (!szNewDir) return FALSE;
		if (_tcscmp(szNewDir, _T("")) == 0) {
			_tcscpy(strCurrentFolder, _T(""));
			return TRUE;
		}
		else if (_tchdir(szNewDir) != -1) {
			// Set the current folder to be the directory we just changed to
			// (so we get its full, correct name).
			_tgetcwd(strCurrentFolder, MAX_PATH);
			_tchdir(szWorkingDir);
			return TRUE;
		}

		return FALSE;
	}

	void RefreshPresetList(TCHAR *currentFile = NULL)
	{
		UNREFERENCED_PARAMETER(currentFile);
		WIN32_FIND_DATA find;
		HANDLE hFind;

		TCHAR searchName[MAX_PATH] = { 0 };
		TCHAR fileName[MAX_PATH] = { 0 };
		int id;

		ClearFileList();

		// ST - You cannot unselect the currently selected rig,
		// and if you have created a rig and now want to create a blank
		// rig its not possible without some sort of special case
		// Here I am creating the initial string of the rig table
		// to be "none", which will not load a rig
		TCHAR *emptyRig = new TCHAR[MAX_PATH]; // SA - globalize - allocate enough for any language
		_stprintf(emptyRig, GetString(IDS_NONE));
		id = (int)SendMessage(hListPresets, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)emptyRig);
		SendMessage(hListPresets, LB_SETITEMDATA, id, vecFileNames.size());
		SendMessage(hListPresets, LB_SETCURSEL, id, 0);
		vecFileNames.push_back(emptyRig);

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

						id = (int)SendMessage(hListPresets, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)driveName);
						SendMessage(hListPresets, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
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
				id = (int)SendMessage(hListPresets, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)_T("<..>"));
				SendMessage(hListPresets, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
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
								id = (int)SendMessage(hListPresets, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)fileName);
								SendMessage(hListPresets, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
								vecFolderNames.push_back(folder);
							}
						}
					}
				} while (FindNextFile(hFind, &find));

				FindClose(hFind);
			}
		}
		if (!catparent->GetCATParentTrans() || !(catparent->GetCATParentTrans() && catparent->GetCATParentTrans()->GetRootHub())) {

			//////////////////////////////////////////////////////////////////////////////////
			// Search for all rig files in the current directory and
			// insert them into the list box, while also maintaining them
			// in a vector.
			_stprintf(searchName, _T("%s\\*.%s"), strCurrentFolder, CAT_RIG_PRESET_EXT);
			hFind = FindFirstFile(searchName, &find);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (find.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
						// Copy the filename and remove the extension.
						_tcscpy(fileName, find.cFileName);
						fileName[_tcslen(fileName) - _tcslen(CAT_RIG_PRESET_EXT) - 1] = _T('\0');

						// We need to save the full path, so allocate some
						// memory.  If this fails, don't even bother adding
						// it to the list box.
						TCHAR *file = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];

						if (file) {
							_stprintf(file, _T("%s\\%s"), strCurrentFolder, find.cFileName);
							id = (int)SendMessage(hListPresets, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)fileName);
							SendMessage(hListPresets, LB_SETITEMDATA, id, vecFileNames.size());
							vecFileNames.push_back(file);

							// Select the most recently used file.
							if (!_tcsncicmp(catparent->GetRigFilename(), file, MAX_PATH) || !_tcsncicmp(lastSelectedFilename, file, MAX_PATH))
								SendMessage(hListPresets, LB_SETCURSEL, id, 0);
						}
					}
				} while (FindNextFile(hFind, &find));
			}

		}

		//////////////////////////////////////////////////////////////////////////////////
		// Now search for any CAT3 rigs
		_stprintf(searchName, _T("%s\\*.%s"), strCurrentFolder, CAT3_RIG_PRESET_EXT);
		hFind = FindFirstFile(searchName, &find);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (find.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
					// Copy the filename and remove the extension.
					_tcscpy(fileName, find.cFileName);
					fileName[_tcslen(fileName) - _tcslen(CAT3_RIG_PRESET_EXT) - 1] = _T('\0');

					// We need to save the full path, so allocate some
					// memory.  If this fails, don't even bother adding
					// it to the list box.
					TCHAR *file = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];

					if (file) {
						_stprintf(file, _T("%s\\%s"), strCurrentFolder, find.cFileName);
						id = (int)SendMessage(hListPresets, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)fileName);
						SendMessage(hListPresets, LB_SETITEMDATA, id, vecFileNames.size());
						vecFileNames.push_back(file);

						// Select the most recently used file.
						if (!_tcsncicmp(catparent->GetRigFilename(), file, MAX_PATH) || !_tcsncicmp(lastSelectedFilename, file, MAX_PATH))
							SendMessage(hListPresets, LB_SETCURSEL, id, 0);
					}
				}
			} while (FindNextFile(hFind, &find));
		}

		FindClose(hFind);
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		ICATParentTrans* catparenttrans = (catparent != NULL) ? catparent->GetCATParentTrans() : NULL;

		switch (message) {
		case WM_INITDIALOG:
			if (!InitControls(hWnd, (CATParent*)lParam)) DestroyWindow(hWnd);
			RefreshPresetList();
			DoSelectionChanged();
			return FALSE;

		case WM_DESTROY:
			ReleaseControls(hWnd);
			return FALSE;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				switch (LOWORD(wParam)) {
				case IDC_PRESETS_LIST:
					DoSelectionChanged();
					break;
				}
				break;
			case LBN_DBLCLK:
				switch (LOWORD(wParam)) {
				case IDC_PRESETS_LIST:
				{
					int nItem = (int)SendMessage(hListPresets, LB_GETCURSEL, 0, 0);
					if (nItem != LB_ERR) {
						int nIndex = (int)SendMessage(hListPresets, LB_GETITEMDATA, nItem, 0);
						if (nIndex < 0) {
							// A negative index means this is a directory.
							HCURSOR hCurrentCursor = ::GetCursor();
							SetCursor(LoadCursor(NULL, IDC_WAIT));
							BOOL bSuccess = ChangeDirectory(vecFolderNames[-nIndex - 1]);
							SetCursor(hCurrentCursor);

							if (!bSuccess) {
								::MessageBox(
									hWnd,
									GetString(IDS_ERR_NOACCESS),
									GetString(IDS_ERROR),
									MB_OK | MB_ICONSTOP);
							}
							RefreshPresetList();
						}
						else if (catparenttrans/* && !catparenttrans->GetRootHub()*/)
						{
							// We actually support Undo's for this operation
							DbgAssert(!theHold.Holding());

							theHold.Begin();
							// Otherwise we load the requested preset
							MSTR sCurrentRigFile = GetSelectedRigFilename();
							INode* pNode = catparenttrans->LoadRig(sCurrentRigFile);
							if (NULL != pNode)
							{
								// If we just loaded an RG3 file, the existing
								// catparenttrans would have been deleted.
								catparenttrans = dynamic_cast<CATParentTrans*>(pNode->GetTMController());
								DbgAssert(catparenttrans != NULL);

								if (catparenttrans == NULL)
								{
									theHold.Cancel();
									return FALSE;
								}

								MSTR sUndoAction = GetString(IDS_HLD_LOAD_RIG);
								sUndoAction += _T(" \"");
								sUndoAction += catparenttrans->GetCATName();
								sUndoAction += _T("\"");
								theHold.Accept(sUndoAction);
							}
							else
							{
								// TODO: Messagebox?
								theHold.Cancel();
							}
							Refresh();
							return REDRAW_VIEWS;
						}
					}
				}
				break;
				}
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_LOADRIG:
					if (catparenttrans/* && !catparenttrans->GetRootHub()*/)
					{
						TCHAR filename[MAX_PATH] = { 0 };
						BOOL result;
						TSTR file = catparent->GetRigFilename();
						TSTR path, fname, extension;
						SplitFilename(file, &path, &fname, &extension);
						extension = extension.Substr(1, extension.Length() - 1);	// Remove the '.' from '.rg3'
						if (MatchPattern(extension, TSTR(CAT3_RIG_PRESET_EXT), TRUE))
						{
							// rig is associated with rg3 preset, so initialize file open dialog with associated rg3 file
							_stprintf(filename, file.data());
							result = DialogOpenRigPreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH);
						}
						else
						{
							// rig is not associated with an rg3 preset, so initialize to most recent directory
							filename[0] = _T('\0');
							result = DialogOpenRigPreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH);
						}
						if (result)
						{
							if (catparenttrans->LoadRig(filename))
							{
								//InitControls(hWnd, catparent);
								// the rig has a .rg3 filename => remember it and change dir to it
								_tcsnccpy(lastSelectedFilename, catparent->GetRigFilename(), catparent->GetRigFilename().Length());
								TSTR path, filename, extension;
								SplitFilename(catparent->GetRigFilename(), &path, &filename, &extension);
								ChangeDirectory(path);
								Refresh();
								DoSelectionChanged();
							}
							return REDRAW_VIEWS;
						}
					}
					break;
				case IDC_BTN_SAVERIG:
				{
					// DialogSaveRigPreset: 1st string param is initial file name w/ path and extension, 2nd string param should be NULL
					TCHAR filename[MAX_PATH] = { 0 };
					BOOL result;
					TSTR file = catparent->GetRigFilename();
					TSTR path, fname, extension;
					SplitFilename(file, &path, &fname, &extension);
					extension = extension.Substr(1, extension.Length() - 1);	// Remove the '.' from '.rg3'
					if (MatchPattern(extension, TSTR(CAT3_RIG_PRESET_EXT), TRUE))
					{
						// rig is associated with rg3 preset, so initialize file save dialog with associated rg3 file
						_stprintf(filename, file.data());
						result = DialogSaveRigPreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH, NULL);
					}
					else
					{
						// rig is not associated with an rg3 preset, so initialize to most recent directory
						filename[0] = _T('\0');
						result = DialogSaveRigPreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH);
					}
					if (result)
					{
						if (!catparenttrans->SaveRig(filename))
						{
							::MessageBox(hWnd, GetString(IDS_ERR_FILESAVE), GetString(IDS_ERR_RIGEXPORT), MB_OK);
						}
						// the rig has a .rg3 filename => remember it and change dir to it
						_tcsnccpy(lastSelectedFilename, catparent->GetRigFilename(), catparent->GetRigFilename().Length());
						TSTR path, filename, extension;
						SplitFilename(catparent->GetRigFilename(), &path, &filename, &extension);
						ChangeDirectory(path);
						Refresh();
						DoSelectionChanged();
					}
				}
				break;
				case IDC_BTN_CREATEPELVIS:
					if (catparenttrans->GetRootHub() && catparent->GetRigFilename() != _T("")) {
						catparenttrans->LoadRig(catparent->GetRigFilename());
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					else if (catparenttrans->AddHub())
					{
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					}
					Refresh();
					break;
				case IDC_BTN_ERN:
					ExtraRigNodes *ern = (ExtraRigNodes*)catparenttrans->GetInterface(I_EXTRARIGNODES_FP);
					DbgAssert(ern);
					if (btnERN->IsChecked())
						ern->ICreateERNWindow(hWnd);
					else ern->IDestroyERNWindow();
				}// switch switch (LOWORD(wParam)) {
				break;
			case BN_CLICKED:
			{
				switch (LOWORD(wParam)) {
				case IDC_CHK_RELOAD_RIG:	catparent->SetReloadRigOnSceneLoad(IS_CHECKED(hWnd, IDC_CHK_RELOAD_RIG));		break;
				}
			}

			} //switch (HIWORD(wParam))
			return FALSE;
		}
		return FALSE;
	}
};

static CATRigRollout staticCATRigRollout;

static INT_PTR CALLBACK CATRigRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticCATRigRollout.DlgProc(hWnd, message, wParam, lParam);
};

/**********************************************************************
* Object creation callback class, for interactive creation of the
* object using the mouse.
*/

int CATParentCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	UNREFERENCED_PARAMETER(flags);
	Point3 p1;
	float r;
	if (!ob) return TRUE;

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch (point) {
		case 0: // only happens with MOUSE_POINT msg
		{
			iscreating = TRUE;
			ob->suspendSnap = TRUE;
			ob->bRigCreating = FALSE;

			sp0 = m;
			p0 = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);

			ob->BuildMesh(0);

			CATParentTrans* pCATParentTrans = (CATParentTrans*)ob->GetCATParentTrans();
			DbgAssert(pCATParentTrans != NULL);
			if (pCATParentTrans == NULL)
				return CREATE_STOP;

			// If we load a new rig here, we need to capture the new node
			INode* pNewNode = pCATParentTrans->LoadRig(TSTR(staticCATRigRollout.GetSelectedRigFilename()), &mat);
			if (pNewNode != NULL)
				pCATParentTrans = dynamic_cast<CATParentTrans*>(pNewNode->GetTMController());

			DbgAssert(pCATParentTrans != NULL);
			if (pCATParentTrans == NULL)
				return CREATE_ABORT;  // That would be a failure - how to warn?

			// This needs to happen after the LoadRig call, because that will set the length axis.
			if (pCATParentTrans->GetLengthAxis() == Z)
				mat.PreRotateZ(PI);
			else
			{
				mat.PreRotateY(-HALFPI);
				mat.RotateZ(PI);
			}
			mat.SetTrans(p0);

			// If we are loading an RG3 file, Max will
			// already have backed out from the CreateMode,
			// and will be unable to set the transform on the
			// current node.  We compensate for this here
			// by setting the transform on whatever has come
			// back from the LoadRig fn call
			if (pNewNode != NULL)
				pNewNode->SetNodeTM(0, mat);

			// Refresh our UI
			staticCATRollout.Refresh();
			staticCATRigRollout.Refresh();
			break;
		}
		case 1:
			sp1 = m;
			//			mat.IdentityMatrix();
			p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D) - p0;
			CATParentTrans* pCATParentTrans = (CATParentTrans*)ob->GetCATParentTrans();
			DbgAssert(pCATParentTrans != NULL);
			if (pCATParentTrans == NULL)
				return CREATE_STOP;

			r = (float)(sqrt(sq(p1.x) + sq(p1.y))) / 200; ///100.0f;	// ST - Making CATUnits 10 times smaller, just too fiddly right now
			if (r > 0.01) {
				pCATParentTrans->SetCATUnits(r);
			}
			if (msg == MOUSE_POINT) {
				ob->suspendSnap = FALSE;
				// CAT2.5 Rigs will load a CATUnits setting meaning that if a user
				// clicks and releases without dragging then they will get their
				// rig at their orriginal size.
				if (pCATParentTrans->GetCATUnits() < 0.01)
					pCATParentTrans->SetCATUnits(defaultCATUnits);
				iscreating = FALSE;
				return CREATE_STOP;
			}
			break;
		}
	}
	else {
		if (msg == MOUSE_ABORT) {
			iscreating = FALSE;
			return CREATE_ABORT;
		}
	}

	return TRUE;
}

static CATParentCreateCallBack CATParentCreateCB;

//From BaseObject
CreateMouseCallBack* CATParent::GetCreateMouseCallBack()
{
	CATParentCreateCB.SetObj(this);
	return(&CATParentCreateCB);
}
// end new insert

// CATStuffParams
// I really want to remove this ParamBlock form CAT, but deleteing param blocks seems impossible
// If I simply don't make it, then I can't load old files. I can't make it simply so I can load old files
// because I don't have version information until I have loaded the CATParent, and References get loaded
// before the ReferenceMaker.
static ParamBlockDesc2 CATStuffParams(
	CATParent::ID_CATSTUFF_PARAMS, _T("CAT Stuff"), 0, &CATParentDesc,
	P_AUTO_CONSTRUCT, CATParent::REF_CATSTUFF_PARAMS,

	CATParent::PB_PARENT_NODE, _T(""), TYPE_INODE, P_NO_REF, 0, p_end,
	CATParent::PB_PARENT_CTRL, _T(""), TYPE_REFTARG, 0, 0, p_end,
	CATParent::PB_ROOTHUB, _T(""), TYPE_REFTARG, P_NO_REF, 0, p_end,

	CATParent::PB_STEP_EASE, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0, p_end,
	CATParent::PB_DISTCOVERED, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0, p_end,
	p_end
);

BaseInterface* CATParent::GetInterface(Interface_ID id)
{
	// We return CATParentTrans's interfaces for it, because then we can
	// access these interfaces without accessing the controller directly
	// (Max will treat object properties like node properties, but not controllers)
	if (catparenttrans != NULL)
	{
		if (id == LAYERROOT_INTERFACE_FP)	return catparenttrans->GetInterface(id);
		if (id == CATPARENT_INTERFACE_FP)	return catparenttrans->GetInterface(id);
	}
	return ECATParent::GetInterface(id);
}

FPInterfaceDesc* CATParent::GetDescByID(Interface_ID id)
{
	// We return CATParentTrans's interfaces for it, because then we can
	// access these interfaces without accessing the controller directly
	// (Max will treat object properties like node properties, but not controllers)
	if (catparenttrans != NULL)
	{
		if (id == LAYERROOT_INTERFACE_FP)	return catparenttrans->GetDescByID(id);
		if (id == CATPARENT_INTERFACE_FP)	return catparenttrans->GetDescByID(id);
	}
	return NULL;
}

//--- CATParent -------------------------------------------------------

void CATParent::GetClassName(TSTR& s) { s = GetString(IDS_CL_CATPARENT); }

void CATParent::CATParentNodeCreateNotify(void *param, NotifyInfo *info)
{
	INode *node = (INode*)info->callParam;
	CATParent *pCATParent = (CATParent*)param;
	if (node->GetObjectRef()->FindBaseObject() == pCATParent) {
		UnRegisterNotification(CATParentNodeCreateNotify, pCATParent, NOTIFY_NODE_CREATED);
		if (!pCATParent->TestFlag(PARENTFLAG_CREATED)) {
			// this bit is here to ensure our node has the correct controller applied.
			INode *pCATParentNode = pCATParent->catparenttrans ? pCATParent->catparenttrans->GetNode() : NULL;
			if (!pCATParentNode) {
				pCATParent->catparenttrans = (CATParentTrans*)node->GetTMController();
				if (pCATParent->catparenttrans->ClassID() != CATPARENTTRANS_CLASS_ID) {
					pCATParent->catparenttrans = (CATParentTrans*)GetCATParentTransDesc()->CreateNew(pCATParent, node);
					node->SetTMController(pCATParent->catparenttrans);
				}
			}
			float catunits = pCATParent->GetCATParentTrans()->GetCATUnits();
			pCATParent->catparentparams->SetValue(PB_CATUNITS, 0, catunits);
			pCATParent->lengthaxis = pCATParent->GetCATParentTrans()->GetLengthAxis();
			pCATParent->SetFlag(PARENTFLAG_CREATED);
			// Allow the rig to start functioning normally
			pCATParent->bRigCreating = FALSE;
		}
	}
}

/**********************************************************************
* Initialisation, construction and destruction...
*/
void CATParent::Init()
{
	ipbegin = NULL;

	// Our references are initialised here
	catparentparams = NULL;
	catstuffparams = NULL;
	flagsBegin = 0;

	bRigCreating = TRUE;

	catparenttrans = NULL;

	catmesh = Mesh();

	flags = 0;

	dwFileSaveVersion = CAT_VERSION_0360;

	RegisterCallbacks();

	/////////////////////////////////////////////////////////////
	// Obsolete variables. All moved to CATParentTrans
	roothub_deprecated = NULL;
	node_deprecated = NULL;

	// New Characters default to X, old ones to Z
	lengthaxis = X;

	catmode = SETUPMODE;
	/////////////////////////////////////////////////////////////
}

CATParent::CATParent(BOOL loading)
{
	InitCATParentGlobalCallbacks();

	UNREFERENCED_PARAMETER(loading);

	Init();

	CATParentDesc.MakeAutoParamBlocks(this);
}

void CATParent::RegisterCallbacks()
{
	RegisterNotification(CATParentNodeCreateNotify, this, NOTIFY_NODE_CREATED);
}

void CATParent::UnRegisterCallbacks()
{
	UnRegisterNotification(CATParentNodeCreateNotify, this, NOTIFY_NODE_CREATED);
}

CATParent::~CATParent()
{
	// Ensure any other links out there are removed.
	DbgAssert(staticCATRigRollout.GetCATParent() != this && staticCATRollout.GetCATParent() != this);
	if (staticCATRigRollout.GetCATParent() == this)
		staticCATRigRollout.SetCATParent(NULL);
	if (staticCATRollout.GetCATParent() == this)
		staticCATRollout.SetCATParent(NULL);

	UnRegisterCallbacks();
	DeleteAllRefs();
}

/**********************************************************************
* Loading and saving....
*/
enum {
	CATPARENTCHUNK_NUMCATCHARS,
	CATPARENTCHUNK_SAVE_VERSION,
	CATPARENTCHUNK_FLAGS,
	CATPARENTCHUNK_NUMSAVES,
	CATPARENTCHUNK_NODE,
	CATPARENTCHUNK_CATPARENTTRANS,
	CATPARENTCHUNK_ROOTHUB,
	CATPARENTCHUNK_LENGTHAXIS,
	CATPARENTCHUNK_CATMODE,
	CATOBJECT_NUMVERTICIES_CHUNK,
	CATOBJECT_NUMFACES_CHUNK,
	CATOBJECT_VERTICIES_CHUNK,
	CATOBJECT_FACES_CHUNK,
	CATPARENTCHUNK_PRESETFILE,
	CATPARENTCHUNK_PRESETFILE_MODIFIED_TIME
};

IOResult CATParent::Save(ISave *isave)
{
	DWORD nb;
	ULONG id;
	int i;

	if (g_bSavingCAT3Rig) {

		// Save our flags
		isave->BeginChunk(CATPARENTCHUNK_CATMODE);
		int tempcatmode = SETUPMODE;
		isave->Write(&tempcatmode, sizeof(int), &nb);
		isave->EndChunk();

		// Save our flags
		isave->BeginChunk(CATPARENTCHUNK_FLAGS);
		isave->Write(&flags, sizeof DWORD, &nb);
		isave->EndChunk();

	}
	else {
		// Save our catmode param
		isave->BeginChunk(CATPARENTCHUNK_CATMODE);
		isave->WriteEnum(&catmode, sizeof(int), &nb);
		isave->EndChunk();

		// Save our flags
		isave->BeginChunk(CATPARENTCHUNK_FLAGS);
		isave->Write(&flags, sizeof DWORD, &nb);
		isave->EndChunk();

	}

	// This stores the version of CAT used to save the file.
	DWORD dwCurrentVersion = CAT_VERSION_CURRENT;
	isave->BeginChunk(CATPARENTCHUNK_SAVE_VERSION);
	isave->Write(&dwCurrentVersion, sizeof DWORD, &nb);
	isave->EndChunk();

	if (catparenttrans) {
		isave->BeginChunk(CATPARENTCHUNK_CATPARENTTRANS);
		id = isave->GetRefID(catparenttrans);
		isave->Write(&id, sizeof(ULONG), &nb);
		isave->EndChunk();
	}

	// We will keep saving these values so that if people really need to they can downgrade from 2.5

	if (node_deprecated) {
		isave->BeginChunk(CATPARENTCHUNK_NODE);
		id = isave->GetRefID(node_deprecated);
		isave->Write(&id, sizeof(ULONG), &nb);
		isave->EndChunk();
	}
	if (roothub_deprecated) {
		isave->BeginChunk(CATPARENTCHUNK_ROOTHUB);
		id = isave->GetRefID(roothub_deprecated);
		isave->Write(&id, sizeof(ULONG), &nb);
		isave->EndChunk();
	}
	isave->BeginChunk(CATPARENTCHUNK_LENGTHAXIS);
	isave->Write(&lengthaxis, sizeof(int), &nb);
	isave->EndChunk();

	// Stores the number of verticies.
	isave->BeginChunk(CATOBJECT_NUMVERTICIES_CHUNK);
	isave->Write(&catmesh.numVerts, sizeof(int), &nb);
	isave->EndChunk();

	// Stores the number of faces.
	isave->BeginChunk(CATOBJECT_NUMFACES_CHUNK);
	isave->Write(&catmesh.numFaces, sizeof(int), &nb);
	isave->EndChunk();

	// Stores the verticies.
	isave->BeginChunk(CATOBJECT_VERTICIES_CHUNK);
	for (i = 0; i < catmesh.getNumVerts(); i++)
		isave->Write(&catmesh.getVert(i), sizeof(Point3), &nb);
	isave->EndChunk();

	// Stores the faces.
	isave->BeginChunk(CATOBJECT_FACES_CHUNK);
	for (i = 0; i < catmesh.getNumFaces(); i++)
		isave->Write(&catmesh.faces[i], sizeof(Face), &nb);
	isave->EndChunk();

	isave->BeginChunk(CATPARENTCHUNK_PRESETFILE);
	isave->WriteCString(rigpresetfile);
	isave->EndChunk();

	isave->BeginChunk(CATPARENTCHUNK_PRESETFILE_MODIFIED_TIME);
	isave->WriteCString(rigpresetfile_modifiedtime);
	isave->EndChunk();

	return IO_OK;
}

//[GB 15-Mar-2004]
// The function ResetFileLoadWarnings() is a callback, registered to
// respond to NOTIFY_FILE_PRE_MERGE and NOTIFY_FILE_PRE_OPEN.  This
// registration is set up by the function InitCATParentGlobalCallbacks(),
// which is called in the DllEntry.  The purpose of it is to maintain
// flags that allow us to do something once during loading, such as
// displaying a message warning the user that their version of CAT is
// older than that which saved the file.
//

void ResetFileLoadWarnings(void *, NotifyInfo *)
{
	g_bOutOfDateWarningDisplayed = false;
}

//static callback function that is called whenever the color changes in the GUI.
static void ColorChangeNotifyProc(void*, NotifyInfo*)
{
	if (hIcons) ImageList_RemoveAll(hIcons);
	LoadMAXFileIcon(_T("CAT"), hIcons, kBackground, FALSE);
}

// When changing parents, recalc the HasTransform flag
static void OnChangeParentsNotifyProc(void* arg, NotifyInfo* pInfo)
{
	if (pInfo != NULL)
	{
		INode* pChild = reinterpret_cast<INode*>(pInfo->callParam);
		if (pChild != NULL)
		{
			CATNodeControl* pNodeCtrl = dynamic_cast<CATNodeControl*>(pChild->GetTMController());
			if (pNodeCtrl != NULL)
				pNodeCtrl->CalculateHasTransformFlag();
		}
	}
}

void InitCATParentGlobalCallbacks()
{
	static bool hasRegistered = false;
	if (!hasRegistered)
	{
		RegisterNotification(ResetFileLoadWarnings, NULL, NOTIFY_FILE_PRE_MERGE);
		RegisterNotification(ResetFileLoadWarnings, NULL, NOTIFY_FILE_PRE_OPEN);
		RegisterNotification(ColorChangeNotifyProc, NULL, NOTIFY_COLOR_CHANGE);

		// Register for node link notifications.  This allows us
		// to auto-magically recalculate our HAS_TRANSFORMS flag
		// whenever one of our nodes is re-parented.  We tried
		// using the ChangeParents function, but it doesn't fire
		// when called with the keepPos argument as false
		RegisterNotification(OnChangeParentsNotifyProc, NULL, NOTIFY_NODE_LINKED);

		hasRegistered = true;
	}
}

void ReleaseCATParentGlobalCallbacks()
{
	UnRegisterNotification(ResetFileLoadWarnings, NULL);
	UnRegisterNotification(ColorChangeNotifyProc, NULL);
}
//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//

class CATParentPLCB : public PostLoadCallback {
protected:
	CATParent *catparent;
public:
	CATParentPLCB(CATParent *pOwner) { catparent = pOwner; }

	DWORD GetFileSaveVersion() {
		return catparent->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		if (GetFileSaveVersion() < CAT_VERSION_2436) {
			catparent->catmode = (CATMode)catparent->catparentparams->GetInt(CATParent::PB_CATMODE);
		}

		if (GetFileSaveVersion() < CAT3_VERSION_3000 && catparent->node_deprecated && !catparent->TestFlag(PARENTFLAG_LOADED_IN_CAT3)) {
			catparent->catparentparams->EnableNotifications(FALSE);
			catparent->catparenttrans->catmode = catparent->catmode;
			catparent->catparenttrans->catunits = catparent->catparentparams->GetFloat(CATParent::PB_CATUNITS);
			catparent->catparenttrans->pblock->SetValue(CATParentTrans::PB_CATUNITS, 0, catparent->catparenttrans->catunits);
			catparent->catparenttrans->node = catparent->node_deprecated;
			catparent->catparenttrans->flags = catparent->flags;
			catparent->catparenttrans->lengthaxis = catparent->lengthaxis;
			catparent->catparenttrans->mRootHub = catparent->roothub_deprecated;
			catparent->catparenttrans->ReplaceReference(CATParentTrans::LAYERROOT, catparent->catparentparams->GetControllerByID(CATParent::PB_CLIPHIERARCHY));

			catparent->node_deprecated = NULL;
			catparent->roothub_deprecated = NULL;
			int index = catparent->catparentparams->IDtoIndex(CATParent::PB_CLIPHIERARCHY);
			catparent->catparentparams->RemoveControllerByIndex(index, 0); // a real bug fix!

			catparent->catparentparams->EnableNotifications(TRUE);
			catparent->SetFlag(PARENTFLAG_LOADED_IN_CAT3);
			iload->SetObsolete();
		}
		// we are done loading now
		catparent->bRigCreating = FALSE;

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

IOResult CATParent::Load(ILoad *iload) {

	IOResult res = IO_OK;
	DWORD nb;
	ULONG id = 0L;
	TCHAR *strBuf;
	TCHAR buf[256] = { 0 };

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case CATPARENTCHUNK_SAVE_VERSION:
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
					GetString(IDS_ERR_OODATEVER), MB_YESNO | MB_ICONASTERISK);

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
			break;
		case CATPARENTCHUNK_CATPARENTTRANS:
			iload->Read(&id, sizeof(ULONG), &nb);
			if (id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&catparenttrans);
			break;
		case CATPARENTCHUNK_CATMODE:
			res = iload->ReadEnum(&catmode, sizeof(DWORD), &nb);
			break;
		case CATPARENTCHUNK_FLAGS:
			res = iload->Read(&flags, sizeof(DWORD), &nb);
			break;
		case CATPARENTCHUNK_NODE:
			iload->Read(&id, sizeof(ULONG), &nb);
			if (id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&node_deprecated);
			break;
		case CATPARENTCHUNK_ROOTHUB:
			iload->Read(&id, sizeof(ULONG), &nb);
			if (id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&roothub_deprecated);
			break;
		case CATPARENTCHUNK_LENGTHAXIS:
			iload->Read(&lengthaxis, sizeof(int), &nb);
			break;
		case CATOBJECT_NUMVERTICIES_CHUNK: {
			int nNumVerticies = 0;
			res = iload->Read(&nNumVerticies, sizeof DWORD, &nb);
			catmesh.setNumVerts(nNumVerticies);
			break;
		}
		case CATOBJECT_NUMFACES_CHUNK: {
			int nNumFaces = 0;
			res = iload->Read(&nNumFaces, sizeof DWORD, &nb);
			catmesh.setNumFaces(nNumFaces);
			break;
		}
		case CATOBJECT_VERTICIES_CHUNK: {
			for (int i = 0; i < catmesh.getNumVerts(); i++)
				res = iload->Read(&catmesh.verts[i], sizeof Point3, &nb);
			break;
		}
		case CATOBJECT_FACES_CHUNK: {
			for (int i = 0; i < catmesh.getNumFaces(); i++)
				res = iload->Read(&catmesh.faces[i], sizeof Face, &nb);
			break;
		}
		case CATPARENTCHUNK_PRESETFILE:
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) rigpresetfile = strBuf;
			break;
		case CATPARENTCHUNK_PRESETFILE_MODIFIED_TIME:
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) rigpresetfile_modifiedtime = strBuf;
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new CATParentPLCB(this));

	return IO_OK;
}

BOOL CATParent::SaveMesh(CATRigWriter* save)
{
	return save->SaveMesh(catmesh);
}
BOOL CATParent::LoadMesh(CATRigReader* load)
{
	return load->ReadMesh(catmesh);
}

void CATParent::SetOwnsUI()
{
	staticCATRollout.SetCATParent(this);
	staticCATRigRollout.SetCATParent(this);
}

void CATParent::RefreshUI()
{
	if (staticCATRollout.GetHWnd())		staticCATRollout.Refresh();
	if (staticCATRigRollout.GetHWnd())	staticCATRigRollout.Refresh();
}

void CATParent::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	DbgAssert(ipbegin == NULL);
	this->ipbegin = ip;
	this->flagsBegin = flags;

	// We may already own this UI
	if (staticCATRollout.GetCATParent() == this)
	{
		staticCATRigRollout.Refresh();
		staticCATRollout.Refresh();
		return;
	}

	SimpleObject2::BeginEditParams(ip, flags, prev);

	if (flags&BEGIN_EDIT_CREATE || flags == 0) {
		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_CATROLLOUT), CATRolloutProc, GetString(IDS_RO_CATRIGPARAMS), (LPARAM)this);

		//	if(!(flags&BEGIN_EDIT_CREATE)){
		//		if(catparenttrans) catparenttrans->BeginEditParams(ip,flags, prev);
		//	}

		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_CATRIG_LOADSAVE_CAT3), CATRigRolloutProc, GetString(IDS_RO_CATRIG_LDSV), (LPARAM)this);
	}
}

void CATParent::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (ipbegin == NULL)
		return;

	// If we don't own the UI, then bail
	if (staticCATRollout.GetCATParent() != this)
		return;

	//TODO: Save plugin parameter values into class variables, if they are not hosted in ParamBlocks.
	SimpleObject2::EndEditParams(ip, flags, next);

	if (flagsBegin == BEGIN_EDIT_CREATE || flagsBegin == 0) {
		//	CATParentDesc.EndEditParams(ip, this, flags, next);
		if (staticCATRollout.GetHWnd()) {
			ip->DeleteRollupPage(staticCATRollout.GetHWnd());
		}

		if (catparenttrans) catparenttrans->EndEditParams(ip, flags, next);

		if (staticCATRigRollout.GetHWnd()) {
			ip->DeleteRollupPage(staticCATRigRollout.GetHWnd());
		}
	}
	this->ipbegin = NULL;
	this->flagsBegin = 0;
}

/**********************************************************************
* Parameter block functions...
*/
IParamBlock2* CATParent::GetParamBlock(int i)
{
	switch (i) {
	case 0:		return catparentparams;
	case 1:		return catstuffparams;
	default:	return NULL;
	}
}

//***********************************************************************/
// This method is called by begin edit params to display each parameter */
// blocks rollout. So I have used it to mask the rollout I don't want   */
// appearing                                                            */
//***********************************************************************/
IParamBlock2* CATParent::GetParamBlockByID(BlockID id)
{
	if (catparentparams && catparentparams->ID() == id) return catparentparams;
	if (catstuffparams && catstuffparams->ID() == id) return catstuffparams;

	return NULL;
}

ICATParentTrans* CATParent::GetCATParentTrans()
{
	if (catparenttrans == NULL)
	{
		ReferenceTarget* pPossibleCPTrans = catstuffparams->GetReferenceTarget(CATParent::PB_PARENT_CTRL);
		if (pPossibleCPTrans != NULL && pPossibleCPTrans->ClassID() == CATPARENTTRANS_CLASS_ID)
			catparenttrans = static_cast<CATParentTrans*>(pPossibleCPTrans);
		else
		{
			// Find my node:
			INode* pANode = FindReferencingClass<INode>(this, 1);
			if (pANode != NULL && pANode->GetObjectRef() == this)
			{
				Control* pACtrl = pANode->GetTMController();
				DbgAssert(pACtrl->ClassID() == CATPARENTTRANS_CLASS_ID);
				if (pACtrl->ClassID() == CATPARENTTRANS_CLASS_ID)
				{
					catparenttrans = static_cast<CATParentTrans*>(pACtrl);
				}
			}
		}
	}

	// We need this to return a CATParentTrans during PLCBs
	DbgAssert(catparenttrans != NULL || bRigCreating);
	return catparenttrans;
}

const ICATParentTrans* CATParent::GetCATParentTrans() const {
	return const_cast<const ICATParentTrans*>(const_cast<CATParent*>(this)->GetCATParentTrans());
}

/**********************************************************************
* Functions to handle references and subanims...
* We must take care to preserve SimpleObject2's references and
* subanims.  At this point it has only one of each.  We can put our
* references first so that our enumerations work properly, but it's
* probably a good idea to put SimpleObject2's subanims ahead of ours.
* In any case we must remap the subanims as well as the references.
*/
RefTargetHandle CATParent::GetReference(int i)
{
	switch (i) {
	case REF_CATPARENT_PARAMS:	return catparentparams;
	case REF_CATSTUFF_PARAMS:	return catstuffparams;
	default:					return NULL;
	}
}

void CATParent::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
	case REF_CATPARENT_PARAMS:	catparentparams = (IParamBlock2*)rtarg;		break;
	case REF_CATSTUFF_PARAMS:	catstuffparams = (IParamBlock2*)rtarg; 		break;
	}
}

Animatable* CATParent::SubAnim(int i)
{
	if (bRigCreating) return NULL;
	switch (i) {
	case 0:		return catparenttrans ? static_cast<CATClipRoot*>(catparenttrans->GetLayerRoot()) : NULL;
	default:	return NULL;
	}
}

TSTR CATParent::SubAnimName(int i)
{
	switch (i) {
	case 0:		return GetString(IDS_CLIPHIERARCHY); // "Layers"
	default:	return _T("");
	}
}

/**********************************************************************
* Message handler.  In particular we are looking for the message that
* gets posted when CATUnits changes.  When we receive this message,
* we need to send a CAT_CATUNITS out to our dependents telling
* them to update their local copy of CATUnits.
*/
RefResult CATParent::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
	{
		break;
	}
	case REFMSG_TARGET_DELETED:
		if (catstuffparams == hTarg)	catstuffparams = NULL;
		else if (catparentparams == hTarg)	catparentparams = NULL;
		break;
	}
	return REF_SUCCEED;
}

void CATParent::NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index)
{
	UNREFERENCED_PARAMETER(index); UNREFERENCED_PARAMETER(derObj); UNREFERENCED_PARAMETER(obj);
	DbgAssert(node);
	// grab the mesh off the node;
	ICATParentTrans* pCATParentTrans = GetCATParentTrans();
	DbgAssert(pCATParentTrans != NULL);
	if (pCATParentTrans == NULL)
		return;
	float catunits = pCATParentTrans->GetCATUnits();
	Point3 dim(catunits * 50, catunits * 50, catunits);
	CopyMeshFromNode(GetCOREInterface()->GetTime(), pCATParentTrans->GetNode(), node, catmesh, dim);

	// make ourselves the object again
	node->SetObjectRef(this);

	// Start using the modified geometry
	SetFlag(PARENTFLAG_USECUSTOMMESH);

	//	Update();
	ivalid = NEVER;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	node->EvalWorldState(0);
}

void CATParent::CATMessage(TimeValue t, UINT msg, int data/*=-1*/)
{
	UNREFERENCED_PARAMETER(t);
	UNREFERENCED_PARAMETER(data);
	// we might have been deleted but be on the undo queue
	// so for now don't do nothing
	if (!catparenttrans || bRigCreating) return;

	// GB 27-Jan-2004: The CAT_COLOUR_CHANGED message is sent whenever
	// the character's colour needs to be updated.  This is either during
	// user interaction (such as changing a layer colour) or during animation
	// (eg, where a changing layer weight may cause the character's colour
	// to change).  The 'data' passed in is a boolean saying whether to
	// always recalculate the colour (TRUE) or to only do it once for any
	// particular time (FALSE).  The first is used for interactive updates;
	// the second is used for animation updates.
	switch (msg) {
	case CAT_CATMODE_CHANGED:
	case CAT_NAMECHANGE:
		RefreshUI();
		break;
	case CAT_SHOWHIDE:				break;
	case CAT_VISIBILITY:			break;
	case CAT_CATUNITS_CHANGED:
	{
		// invalidate our buildmeshes validity
		// interval so it redraws...
		ivalid = NEVER;
		// In CAT3 we store CATUnits on the CATPArentTrans, but we also keep a local copy on the
		// CATParent. This is because if the CATParent is XRefed. we need to be able to Build our
		// mesh without being able to access the controller
		float catunits = GetCATParentTrans()->GetCATUnits();
		catparentparams->SetValue(PB_CATUNITS, 0, catunits);

		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

		// Update the CATUnits UI
		staticCATRollout.Refresh();
		break;
	}
	case CAT_SET_LENGTH_AXIS:
		lengthaxis = GetCATParentTrans()->GetLengthAxis();
		break;
	}

}

/**********************************************************************
* Texturing co-ordinate stuff...
*/
BOOL CATParent::HasUVW()
{
	//TODO: Return whether the object has UVW coordinates or not
	return TRUE;
}

void CATParent::SetGenUVW(BOOL sw)
{
	if (sw == HasUVW()) return;
	//TODO: Set the plugin's internal value to sw
}

void CATParent::RefAdded(RefMakerHandle rm)
{
	// We are not being created via the mouse! (but we are being created)

	if (!CATParentCreateCB.IsCreating() && theHold.Holding())
	{
		// We do not currently have a node yet!
		INode* pNewNode = dynamic_cast<INode*>(rm);
		ICATParentTrans* pCATParentTrans = GetCATParentTrans();

		if (pNewNode != NULL && pCATParentTrans != NULL)
		{
			// Ensure that the current node is the one assigned to
			// our CATParentTrans (may not be if we are just cloned)
			if (pCATParentTrans->GetNode() == pNewNode)
			{
				// We have just been cloned - cache the current name.
				MSTR sName = pNewNode->GetName();
				pCATParentTrans->SetCATName(sName);
			}
		}
	}
}

//#define STATUS_INSUFFICIENT_MEM       0xE0000001

void CATParent::BuildMesh(TimeValue)
{
	float catunits = catparentparams->GetFloat(PB_CATUNITS);

	// Everything is scaled by CATUnits.
	float x = catunits * 50;
	float y = catunits * 50;
	float z = catunits;

	if (flags&PARENTFLAG_USECUSTOMMESH) {

		mesh = catmesh;

		Point3 rescale(x, y, z);
		// Scale everything up by CATUnits
		for (int i = 0; i < mesh.getNumVerts(); i++) {
			Point3 p = mesh.getVert(i);
			mesh.setVert(i, p * rescale);
		}

		mesh.InvalidateTopologyCache();

		ivalid.SetInfinite();
		return;
	}

	// Initialise the mesh...
	mesh.setNumVerts(184);
	mesh.setNumFaces(310);
	mesh.setVert(0, 0.183424f * x, 0.999876f * y, -0.000006f * z);
	mesh.setVert(1, 0.150672f * x, 1.044868f * y, -0.000006f * z);
	mesh.setVert(2, 0.100402f * x, 1.086049f * y, -0.000007f * z);
	mesh.setVert(3, 0.035729f * x, 1.110962f * y, -0.000007f * z);
	mesh.setVert(4, -0.035727f * x, 1.110962f * y, -0.000008f * z);
	mesh.setVert(5, -0.100400f * x, 1.086049f * y, -0.000008f * z);
	mesh.setVert(6, -0.150670f * x, 1.044868f * y, -0.000009f * z);
	mesh.setVert(7, -0.183422f * x, 0.999877f * y, -0.000009f * z);
	mesh.setVert(8, -0.957630f * x, -0.341088f * y, -0.000004f * z);
	mesh.setVert(9, -0.980218f * x, -0.391948f * y, -0.000003f * z);
	mesh.setVert(10, -0.990747f * x, -0.456073f * y, -0.000003f * z);
	mesh.setVert(11, -0.979985f * x, -0.524539f * y, -0.000002f * z);
	mesh.setVert(12, -0.944258f * x, -0.586421f * y, -0.000002f * z);
	mesh.setVert(13, -0.890345f * x, -0.629974f * y, -0.000001f * z);
	mesh.setVert(14, -0.829547f * x, -0.652919f * y, -0.000001f * z);
	mesh.setVert(15, -0.774208f * x, -0.658787f * y, -0.000000f * z);
	mesh.setVert(16, 0.774206f * x, -0.658787f * y, 0.000010f * z);
	mesh.setVert(17, 0.829546f * x, -0.652920f * y, 0.000010f * z);
	mesh.setVert(18, 0.890345f * x, -0.629975f * y, 0.000010f * z);
	mesh.setVert(19, 0.944257f * x, -0.586423f * y, 0.000011f * z);
	mesh.setVert(20, 0.979985f * x, -0.524540f * y, 0.000010f * z);
	mesh.setVert(21, 0.990746f * x, -0.456075f * y, 0.000010f * z);
	mesh.setVert(22, 0.980217f * x, -0.391950f * y, 0.000009f * z);
	mesh.setVert(23, 0.957630f * x, -0.341090f * y, 0.000009f * z);
	mesh.setVert(24, 0.245226f * x, 1.040052f * y, 0.245363f * z);
	mesh.setVert(25, 0.204626f * x, 1.095825f * y, 0.245361f * z);
	mesh.setVert(26, 0.137899f * x, 1.150487f * y, 0.245361f * z);
	mesh.setVert(27, 0.049416f * x, 1.184572f * y, 0.245360f * z);
	mesh.setVert(28, -0.049414f * x, 1.184572f * y, 0.245360f * z);
	mesh.setVert(29, -0.137899f * x, 1.150487f * y, 0.245360f * z);
	mesh.setVert(30, -0.204624f * x, 1.095826f * y, 0.245360f * z);
	mesh.setVert(31, -0.245225f * x, 1.040052f * y, 0.245359f * z);
	mesh.setVert(32, -1.023325f * x, -0.307653f * y, 0.245364f * z);
	mesh.setVert(33, -1.051326f * x, -0.370701f * y, 0.245364f * z);
	mesh.setVert(34, -1.065301f * x, -0.455819f * y, 0.245365f * z);
	mesh.setVert(35, -1.050578f * x, -0.549491f * y, 0.245366f * z);
	mesh.setVert(36, -1.001163f * x, -0.635080f * y, 0.245366f * z);
	mesh.setVert(37, -0.927402f * x, -0.694667f * y, 0.245367f * z);
	mesh.setVert(38, -0.846701f * x, -0.725123f * y, 0.245368f * z);
	mesh.setVert(39, -0.778099f * x, -0.732397f * y, 0.245368f * z);
	mesh.setVert(40, 0.778099f * x, -0.732398f * y, 0.245378f * z);
	mesh.setVert(41, 0.846700f * x, -0.725124f * y, 0.245378f * z);
	mesh.setVert(42, 0.927401f * x, -0.694668f * y, 0.245379f * z);
	mesh.setVert(43, 1.001162f * x, -0.635081f * y, 0.245379f * z);
	mesh.setVert(44, 1.050577f * x, -0.549492f * y, 0.245379f * z);
	mesh.setVert(45, 1.065300f * x, -0.455820f * y, 0.245378f * z);
	mesh.setVert(46, 1.051325f * x, -0.370704f * y, 0.245378f * z);
	mesh.setVert(47, 1.023324f * x, -0.307655f * y, 0.245377f * z);
	mesh.setVert(48, 0.282308f * x, 1.064157f * y, -0.000006f * z);
	mesh.setVert(49, 0.236999f * x, 1.126400f * y, -0.000007f * z);
	mesh.setVert(50, 0.160398f * x, 1.189151f * y, -0.000007f * z);
	mesh.setVert(51, 0.057629f * x, 1.228738f * y, -0.000008f * z);
	mesh.setVert(52, -0.057627f * x, 1.228738f * y, -0.000009f * z);
	mesh.setVert(53, -0.160397f * x, 1.189151f * y, -0.000009f * z);
	mesh.setVert(54, -0.236996f * x, 1.126401f * y, -0.000010f * z);
	mesh.setVert(55, -0.282307f * x, 1.064157f * y, -0.000009f * z);
	mesh.setVert(56, -1.062741f * x, -0.287593f * y, -0.000005f * z);
	mesh.setVert(57, -1.093990f * x, -0.357954f * y, -0.000005f * z);
	mesh.setVert(58, -1.110034f * x, -0.455666f * y, -0.000004f * z);
	mesh.setVert(59, -1.092933f * x, -0.564461f * y, -0.000004f * z);
	mesh.setVert(60, -1.035305f * x, -0.664275f * y, -0.000003f * z);
	mesh.setVert(61, -0.949636f * x, -0.733483f * y, -0.000002f * z);
	mesh.setVert(62, -0.856994f * x, -0.768445f * y, -0.000000f * z);
	mesh.setVert(63, -0.780434f * x, -0.776564f * y, -0.000000f * z);
	mesh.setVert(64, 0.780434f * x, -0.776564f * y, 0.000010f * z);
	mesh.setVert(65, 0.856991f * x, -0.768446f * y, 0.000011f * z);
	mesh.setVert(66, 0.949636f * x, -0.733484f * y, 0.000011f * z);
	mesh.setVert(67, 1.035304f * x, -0.664276f * y, 0.000012f * z);
	mesh.setVert(68, 1.092933f * x, -0.564462f * y, 0.000011f * z);
	mesh.setVert(69, 1.110033f * x, -0.455668f * y, 0.000010f * z);
	mesh.setVert(70, 1.093990f * x, -0.357955f * y, 0.000010f * z);
	mesh.setVert(71, 1.062741f * x, -0.287594f * y, 0.000009f * z);
	mesh.setVert(72, 0.294669f * x, 1.072192f * y, -0.490742f * z);
	mesh.setVert(73, 0.247789f * x, 1.136592f * y, -0.490743f * z);
	mesh.setVert(74, 0.167897f * x, 1.202038f * y, -0.490743f * z);
	mesh.setVert(75, 0.060366f * x, 1.243460f * y, -0.490744f * z);
	mesh.setVert(76, -0.060364f * x, 1.243460f * y, -0.490746f * z);
	mesh.setVert(77, -0.167896f * x, 1.202038f * y, -0.490746f * z);
	mesh.setVert(78, -0.247787f * x, 1.136593f * y, -0.490746f * z);
	mesh.setVert(79, -0.294667f * x, 1.072193f * y, -0.490746f * z);
	mesh.setVert(80, -1.075880f * x, -0.280905f * y, -0.490742f * z);
	mesh.setVert(81, -1.108212f * x, -0.353704f * y, -0.490741f * z);
	mesh.setVert(82, -1.124945f * x, -0.455616f * y, -0.490740f * z);
	mesh.setVert(83, -1.107052f * x, -0.569452f * y, -0.490740f * z);
	mesh.setVert(84, -1.046686f * x, -0.674007f * y, -0.490738f * z);
	mesh.setVert(85, -0.957047f * x, -0.746422f * y, -0.490738f * z);
	mesh.setVert(86, -0.860425f * x, -0.782886f * y, -0.490737f * z);
	mesh.setVert(87, -0.781212f * x, -0.791286f * y, -0.490736f * z);
	mesh.setVert(88, 0.781212f * x, -0.791286f * y, -0.490725f * z);
	mesh.setVert(89, 0.860423f * x, -0.782888f * y, -0.490725f * z);
	mesh.setVert(90, 0.957047f * x, -0.746423f * y, -0.490725f * z);
	mesh.setVert(91, 1.046686f * x, -0.674008f * y, -0.490725f * z);
	mesh.setVert(92, 1.107051f * x, -0.569453f * y, -0.490726f * z);
	mesh.setVert(93, 1.124943f * x, -0.455617f * y, -0.490726f * z);
	mesh.setVert(94, 1.108211f * x, -0.353707f * y, -0.490726f * z);
	mesh.setVert(95, 1.075880f * x, -0.280908f * y, -0.490727f * z);
	mesh.setVert(96, 0.282308f * x, 1.064157f * y, -0.981479f * z);
	mesh.setVert(97, 0.236999f * x, 1.126400f * y, -0.981480f * z);
	mesh.setVert(98, 0.160398f * x, 1.189151f * y, -0.981480f * z);
	mesh.setVert(99, 0.057629f * x, 1.228738f * y, -0.981481f * z);
	mesh.setVert(100, -0.057627f * x, 1.228738f * y, -0.981482f * z);
	mesh.setVert(101, -0.160397f * x, 1.189151f * y, -0.981482f * z);
	mesh.setVert(102, -0.236996f * x, 1.126401f * y, -0.981483f * z);
	mesh.setVert(103, -0.282307f * x, 1.064157f * y, -0.981482f * z);
	mesh.setVert(104, -1.062741f * x, -0.287593f * y, -0.981478f * z);
	mesh.setVert(105, -1.093990f * x, -0.357954f * y, -0.981477f * z);
	mesh.setVert(106, -1.110034f * x, -0.455666f * y, -0.981477f * z);
	mesh.setVert(107, -1.092933f * x, -0.564461f * y, -0.981476f * z);
	mesh.setVert(108, -1.035305f * x, -0.664275f * y, -0.981476f * z);
	mesh.setVert(109, -0.949636f * x, -0.733483f * y, -0.981474f * z);
	mesh.setVert(110, -0.856994f * x, -0.768445f * y, -0.981473f * z);
	mesh.setVert(111, -0.780434f * x, -0.776564f * y, -0.981472f * z);
	mesh.setVert(112, 0.780434f * x, -0.776564f * y, -0.981463f * z);
	mesh.setVert(113, 0.856991f * x, -0.768446f * y, -0.981462f * z);
	mesh.setVert(114, 0.949636f * x, -0.733484f * y, -0.981461f * z);
	mesh.setVert(115, 1.035304f * x, -0.664276f * y, -0.981461f * z);
	mesh.setVert(116, 1.092933f * x, -0.564462f * y, -0.981462f * z);
	mesh.setVert(117, 1.110033f * x, -0.455668f * y, -0.981463f * z);
	mesh.setVert(118, 1.093990f * x, -0.357955f * y, -0.981463f * z);
	mesh.setVert(119, 1.062741f * x, -0.287594f * y, -0.981464f * z);
	mesh.setVert(120, 0.245226f * x, 1.040052f * y, -1.226847f * z);
	mesh.setVert(121, 0.204626f * x, 1.095825f * y, -1.226848f * z);
	mesh.setVert(122, 0.137899f * x, 1.150487f * y, -1.226848f * z);
	mesh.setVert(123, 0.049416f * x, 1.184572f * y, -1.226849f * z);
	mesh.setVert(124, -0.049414f * x, 1.184572f * y, -1.226849f * z);
	mesh.setVert(125, -0.137899f * x, 1.150487f * y, -1.226851f * z);
	mesh.setVert(126, -0.204624f * x, 1.095826f * y, -1.226850f * z);
	mesh.setVert(127, -0.245225f * x, 1.040052f * y, -1.226850f * z);
	mesh.setVert(128, -1.023325f * x, -0.307653f * y, -1.226846f * z);
	mesh.setVert(129, -1.051326f * x, -0.370701f * y, -1.226846f * z);
	mesh.setVert(130, -1.065301f * x, -0.455819f * y, -1.226845f * z);
	mesh.setVert(131, -1.050578f * x, -0.549491f * y, -1.226845f * z);
	mesh.setVert(132, -1.001163f * x, -0.635080f * y, -1.226844f * z);
	mesh.setVert(133, -0.927402f * x, -0.694667f * y, -1.226843f * z);
	mesh.setVert(134, -0.846701f * x, -0.725123f * y, -1.226841f * z);
	mesh.setVert(135, -0.778099f * x, -0.732397f * y, -1.226841f * z);
	mesh.setVert(136, 0.778099f * x, -0.732398f * y, -1.226831f * z);
	mesh.setVert(137, 0.846700f * x, -0.725124f * y, -1.226831f * z);
	mesh.setVert(138, 0.927401f * x, -0.694668f * y, -1.226831f * z);
	mesh.setVert(139, 1.001162f * x, -0.635081f * y, -1.226831f * z);
	mesh.setVert(140, 1.050577f * x, -0.549492f * y, -1.226830f * z);
	mesh.setVert(141, 1.065300f * x, -0.455820f * y, -1.226831f * z);
	mesh.setVert(142, 1.051325f * x, -0.370704f * y, -1.226831f * z);
	mesh.setVert(143, 1.023324f * x, -0.307655f * y, -1.226832f * z);
	mesh.setVert(144, 0.183424f * x, 0.999876f * y, -0.981479f * z);
	mesh.setVert(145, 0.150672f * x, 1.044868f * y, -0.981479f * z);
	mesh.setVert(146, 0.100402f * x, 1.086049f * y, -0.981480f * z);
	mesh.setVert(147, 0.035729f * x, 1.110962f * y, -0.981480f * z);
	mesh.setVert(148, -0.035727f * x, 1.110962f * y, -0.981481f * z);
	mesh.setVert(149, -0.100400f * x, 1.086049f * y, -0.981481f * z);
	mesh.setVert(150, -0.150670f * x, 1.044868f * y, -0.981481f * z);
	mesh.setVert(151, -0.183422f * x, 0.999877f * y, -0.981482f * z);
	mesh.setVert(152, -0.957630f * x, -0.341088f * y, -0.981477f * z);
	mesh.setVert(153, -0.980218f * x, -0.391948f * y, -0.981476f * z);
	mesh.setVert(154, -0.990747f * x, -0.456073f * y, -0.981476f * z);
	mesh.setVert(155, -0.979985f * x, -0.524539f * y, -0.981475f * z);
	mesh.setVert(156, -0.944258f * x, -0.586421f * y, -0.981475f * z);
	mesh.setVert(157, -0.890345f * x, -0.629974f * y, -0.981474f * z);
	mesh.setVert(158, -0.829547f * x, -0.652919f * y, -0.981474f * z);
	mesh.setVert(159, -0.774208f * x, -0.658787f * y, -0.981473f * z);
	mesh.setVert(160, 0.774206f * x, -0.658787f * y, -0.981463f * z);
	mesh.setVert(161, 0.829546f * x, -0.652920f * y, -0.981463f * z);
	mesh.setVert(162, 0.890345f * x, -0.629975f * y, -0.981463f * z);
	mesh.setVert(163, 0.944257f * x, -0.586423f * y, -0.981462f * z);
	mesh.setVert(164, 0.979985f * x, -0.524540f * y, -0.981463f * z);
	mesh.setVert(165, 0.990746f * x, -0.456075f * y, -0.981463f * z);
	mesh.setVert(166, 0.980217f * x, -0.391950f * y, -0.981464f * z);
	mesh.setVert(167, 0.957630f * x, -0.341090f * y, -0.981464f * z);
	mesh.setVert(168, -0.179756f * x, 0.233797f * y, -0.000003f * z);
	mesh.setVert(169, -0.074777f * x, 0.233797f * y, -0.000002f * z);
	mesh.setVert(170, -0.074777f * x, -0.374074f * y, 0.000002f * z);
	mesh.setVert(171, 0.000019f * x, -0.374075f * y, 0.000003f * z);
	mesh.setVert(172, 0.074815f * x, -0.374075f * y, 0.000003f * z);
	mesh.setVert(173, 0.074815f * x, 0.233797f * y, -0.000001f * z);
	mesh.setVert(174, 0.179794f * x, 0.233796f * y, -0.000001f * z);
	mesh.setVert(175, 0.000019f * x, 0.547312f * y, -0.000004f * z);
	mesh.setVert(176, -0.136709f * x, 0.258735f * y, 0.935184f * z);
	mesh.setVert(177, -0.049839f * x, 0.258735f * y, 0.935184f * z);
	mesh.setVert(178, -0.049839f * x, -0.349136f * y, 0.935189f * z);
	mesh.setVert(179, 0.000019f * x, -0.349136f * y, 0.935189f * z);
	mesh.setVert(180, 0.049876f * x, -0.349136f * y, 0.935189f * z);
	mesh.setVert(181, 0.049877f * x, 0.258735f * y, 0.935185f * z);
	mesh.setVert(182, 0.136747f * x, 0.258735f * y, 0.935185f * z);
	mesh.setVert(183, 0.000019f * x, 0.497179f * y, 0.935183f * z);
	MakeFace(mesh.faces[0], 0, 24, 1, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[1], 24, 25, 1, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[2], 1, 25, 2, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[3], 25, 26, 2, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[4], 2, 26, 3, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[5], 26, 27, 3, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[6], 3, 27, 4, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[7], 27, 28, 4, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[8], 4, 28, 5, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[9], 28, 29, 5, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[10], 5, 29, 6, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[11], 29, 30, 6, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[12], 6, 30, 7, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[13], 30, 31, 7, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[14], 7, 31, 8, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[15], 31, 32, 8, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[16], 8, 32, 9, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[17], 32, 33, 9, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[18], 9, 33, 10, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[19], 33, 34, 10, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[20], 10, 34, 11, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[21], 34, 35, 11, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[22], 11, 35, 12, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[23], 35, 36, 12, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[24], 12, 36, 13, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[25], 36, 37, 13, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[26], 13, 37, 14, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[27], 37, 38, 14, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[28], 14, 38, 15, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[29], 38, 39, 15, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[30], 15, 39, 16, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[31], 39, 40, 16, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[32], 16, 40, 17, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[33], 40, 41, 17, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[34], 17, 41, 18, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[35], 41, 42, 18, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[36], 18, 42, 19, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[37], 42, 43, 19, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[38], 19, 43, 20, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[39], 43, 44, 20, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[40], 20, 44, 21, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[41], 44, 45, 21, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[42], 21, 45, 22, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[43], 45, 46, 22, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[44], 22, 46, 23, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[45], 46, 47, 23, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[46], 23, 47, 0, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[47], 47, 24, 0, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[48], 24, 48, 25, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[49], 48, 49, 25, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[50], 25, 49, 26, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[51], 49, 50, 26, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[52], 26, 50, 27, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[53], 50, 51, 27, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[54], 27, 51, 28, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[55], 51, 52, 28, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[56], 28, 52, 29, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[57], 52, 53, 29, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[58], 29, 53, 30, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[59], 53, 54, 30, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[60], 30, 54, 31, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[61], 54, 55, 31, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[62], 31, 55, 32, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[63], 55, 56, 32, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[64], 32, 56, 33, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[65], 56, 57, 33, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[66], 33, 57, 34, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[67], 57, 58, 34, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[68], 34, 58, 35, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[69], 58, 59, 35, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[70], 35, 59, 36, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[71], 59, 60, 36, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[72], 36, 60, 37, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[73], 60, 61, 37, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[74], 37, 61, 38, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[75], 61, 62, 38, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[76], 38, 62, 39, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[77], 62, 63, 39, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[78], 39, 63, 40, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[79], 63, 64, 40, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[80], 40, 64, 41, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[81], 64, 65, 41, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[82], 41, 65, 42, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[83], 65, 66, 42, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[84], 42, 66, 43, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[85], 66, 67, 43, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[86], 43, 67, 44, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[87], 67, 68, 44, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[88], 44, 68, 45, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[89], 68, 69, 45, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[90], 45, 69, 46, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[91], 69, 70, 46, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[92], 46, 70, 47, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[93], 70, 71, 47, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[94], 47, 71, 24, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[95], 71, 48, 24, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[96], 48, 72, 49, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[97], 72, 73, 49, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[98], 49, 73, 50, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[99], 73, 74, 50, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[100], 50, 74, 51, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[101], 74, 75, 51, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[102], 51, 75, 52, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[103], 75, 76, 52, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[104], 52, 76, 53, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[105], 76, 77, 53, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[106], 53, 77, 54, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[107], 77, 78, 54, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[108], 54, 78, 55, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[109], 78, 79, 55, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[110], 55, 79, 56, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[111], 79, 80, 56, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[112], 56, 80, 57, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[113], 80, 81, 57, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[114], 57, 81, 58, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[115], 81, 82, 58, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[116], 58, 82, 59, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[117], 82, 83, 59, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[118], 59, 83, 60, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[119], 83, 84, 60, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[120], 60, 84, 61, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[121], 84, 85, 61, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[122], 61, 85, 62, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[123], 85, 86, 62, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[124], 62, 86, 63, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[125], 86, 87, 63, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[126], 63, 87, 64, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[127], 87, 88, 64, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[128], 64, 88, 65, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[129], 88, 89, 65, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[130], 65, 89, 66, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[131], 89, 90, 66, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[132], 66, 90, 67, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[133], 90, 91, 67, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[134], 67, 91, 68, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[135], 91, 92, 68, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[136], 68, 92, 69, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[137], 92, 93, 69, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[138], 69, 93, 70, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[139], 93, 94, 70, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[140], 70, 94, 71, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[141], 94, 95, 71, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[142], 71, 95, 48, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[143], 95, 72, 48, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[144], 72, 96, 73, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[145], 96, 97, 73, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[146], 73, 97, 74, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[147], 97, 98, 74, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[148], 74, 98, 75, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[149], 98, 99, 75, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[150], 75, 99, 76, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[151], 99, 100, 76, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[152], 76, 100, 77, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[153], 100, 101, 77, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[154], 77, 101, 78, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[155], 101, 102, 78, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[156], 78, 102, 79, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[157], 102, 103, 79, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[158], 79, 103, 80, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[159], 103, 104, 80, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[160], 80, 104, 81, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[161], 104, 105, 81, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[162], 81, 105, 82, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[163], 105, 106, 82, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[164], 82, 106, 83, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[165], 106, 107, 83, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[166], 83, 107, 84, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[167], 107, 108, 84, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[168], 84, 108, 85, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[169], 108, 109, 85, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[170], 85, 109, 86, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[171], 109, 110, 86, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[172], 86, 110, 87, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[173], 110, 111, 87, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[174], 87, 111, 88, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[175], 111, 112, 88, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[176], 88, 112, 89, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[177], 112, 113, 89, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[178], 89, 113, 90, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[179], 113, 114, 90, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[180], 90, 114, 91, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[181], 114, 115, 91, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[182], 91, 115, 92, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[183], 115, 116, 92, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[184], 92, 116, 93, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[185], 116, 117, 93, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[186], 93, 117, 94, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[187], 117, 118, 94, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[188], 94, 118, 95, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[189], 118, 119, 95, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[190], 95, 119, 72, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[191], 119, 96, 72, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[192], 96, 120, 97, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[193], 120, 121, 97, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[194], 97, 121, 98, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[195], 121, 122, 98, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[196], 98, 122, 99, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[197], 122, 123, 99, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[198], 99, 123, 100, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[199], 123, 124, 100, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[200], 100, 124, 101, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[201], 124, 125, 101, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[202], 101, 125, 102, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[203], 125, 126, 102, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[204], 102, 126, 103, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[205], 126, 127, 103, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[206], 103, 127, 104, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[207], 127, 128, 104, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[208], 104, 128, 105, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[209], 128, 129, 105, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[210], 105, 129, 106, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[211], 129, 130, 106, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[212], 106, 130, 107, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[213], 130, 131, 107, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[214], 107, 131, 108, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[215], 131, 132, 108, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[216], 108, 132, 109, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[217], 132, 133, 109, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[218], 109, 133, 110, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[219], 133, 134, 110, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[220], 110, 134, 111, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[221], 134, 135, 111, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[222], 111, 135, 112, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[223], 135, 136, 112, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[224], 112, 136, 113, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[225], 136, 137, 113, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[226], 113, 137, 114, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[227], 137, 138, 114, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[228], 114, 138, 115, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[229], 138, 139, 115, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[230], 115, 139, 116, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[231], 139, 140, 116, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[232], 116, 140, 117, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[233], 140, 141, 117, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[234], 117, 141, 118, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[235], 141, 142, 118, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[236], 118, 142, 119, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[237], 142, 143, 119, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[238], 119, 143, 96, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[239], 143, 120, 96, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[240], 120, 144, 121, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[241], 144, 145, 121, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[242], 121, 145, 122, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[243], 145, 146, 122, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[244], 122, 146, 123, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[245], 146, 147, 123, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[246], 123, 147, 124, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[247], 147, 148, 124, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[248], 124, 148, 125, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[249], 148, 149, 125, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[250], 125, 149, 126, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[251], 149, 150, 126, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[252], 126, 150, 127, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[253], 150, 151, 127, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[254], 127, 151, 128, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[255], 151, 152, 128, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[256], 128, 152, 129, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[257], 152, 153, 129, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[258], 129, 153, 130, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[259], 153, 154, 130, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[260], 130, 154, 131, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[261], 154, 155, 131, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[262], 131, 155, 132, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[263], 155, 156, 132, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[264], 132, 156, 133, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[265], 156, 157, 133, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[266], 133, 157, 134, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[267], 157, 158, 134, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[268], 134, 158, 135, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[269], 158, 159, 135, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[270], 135, 159, 136, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[271], 159, 160, 136, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[272], 136, 160, 137, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[273], 160, 161, 137, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[274], 137, 161, 138, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[275], 161, 162, 138, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[276], 138, 162, 139, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[277], 162, 163, 139, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[278], 139, 163, 140, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[279], 163, 164, 140, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[280], 140, 164, 141, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[281], 164, 165, 141, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[282], 141, 165, 142, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[283], 165, 166, 142, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[284], 142, 166, 143, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[285], 166, 167, 143, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[286], 143, 167, 120, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[287], 167, 144, 120, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[288], 168, 169, 176, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[289], 169, 177, 176, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[290], 169, 170, 177, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[291], 170, 178, 177, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[292], 170, 171, 178, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[293], 171, 179, 178, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[294], 171, 172, 179, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[295], 172, 180, 179, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[296], 172, 173, 180, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[297], 173, 181, 180, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[298], 173, 174, 181, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[299], 174, 182, 181, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[300], 174, 175, 182, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[301], 175, 183, 182, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[302], 175, 168, 183, 1, 0, 4, 0, 2);
	MakeFace(mesh.faces[303], 168, 176, 183, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[304], 183, 176, 177, 1, 2, 0, 1, 0);
	MakeFace(mesh.faces[305], 181, 182, 183, 1, 2, 0, 1, 0);
	MakeFace(mesh.faces[306], 181, 183, 177, 0, 0, 0, 1, 0);
	MakeFace(mesh.faces[307], 180, 181, 177, 1, 0, 0, 1, 0);
	MakeFace(mesh.faces[308], 179, 180, 177, 1, 0, 0, 1, 0);
	MakeFace(mesh.faces[309], 179, 177, 178, 0, 2, 4, 1, 0);

	if (lengthaxis == X) {
		for (int i = 0; i < mesh.getNumVerts(); i++) {
			mesh.verts[i] = mesh.verts[i] * RotateYMatrix(HALFPI);
		}
	}

	mesh.InvalidateTopologyCache();
	mesh.InvalidateGeomCache();

	ivalid.SetInfinite();
}

BOOL CATParent::OKtoDisplay(TimeValue)
{
	return TRUE;
}

void CATParent::InvalidateUI()
{
	// Hey! Update the param blocks
}

const TCHAR* CATParent::GetObjectName()
{
	return GetString(IDS_CATOBJECT);
}

Object* CATParent::ConvertToType(TimeValue t, Class_ID obtype)
{
	return SimpleObject2::ConvertToType(t, obtype);
}

int CATParent::CanConvertToType(Class_ID obtype)
{
	//TODO: Check for any additional types the plugin supports
	if (obtype == defObjectClassID ||
		obtype == triObjectClassID) {
		return 1;
	}
	else {
		return SimpleObject2::CanConvertToType(obtype);
	}
}

/**********************************************************************
* Raytracing functions
*/
int CATParent::IntersectRay(TimeValue, Ray& ray, float& at, Point3& norm)
{
	UNREFERENCED_PARAMETER(ray); UNREFERENCED_PARAMETER(at); UNREFERENCED_PARAMETER(norm);
	//TODO: Return TRUE after you implement this method
	return TRUE;
}

/*
void CATParent::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
Object::GetCollapseTypes(clist, nlist);
//TODO: Append any any other collapse type the plugin supports
}
*/

/**********************************************************************
* Duplication....
*/
RefTargetHandle CATParent::Clone(RemapDir& remap)
{
	CATParent* newob = new CATParent(TRUE);
	// thie entry will allow the CATclip root to patch itpointer immedieately
	remap.AddEntry(this, newob);
	newob->ivalid.SetEmpty();

	newob->catmesh = catmesh;

	newob->ReplaceReference(REF_CATPARENT_PARAMS, remap.CloneRef(catparentparams));

	// I can't leave cloning the pointeers to the parameter blocks
	// If the pblock clones itsself it simply copes the old pointer
	// it doesn't even try to Patch the pointer!
	//	remap.PatchPointer((RefTargetHandle*)&newob->roothub, roothub);
	remap.PatchPointer((RefTargetHandle*)&newob->catparenttrans, catparenttrans);

	BaseClone(this, newob, remap);
	newob->bRigCreating = FALSE;

	return newob;
}

