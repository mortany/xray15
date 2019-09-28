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

#include "CATPlugins.h"
#include "BezierInterp.h"
#include "CATWindow.h"
#include "CATDialog.h"

#include <winuser.h>
#include <vector>
#include <direct.h>

#include "CATHierarchyRoot.h"
#include "CATHierarchyBranch2.h"
#include "CATHierarchyLeaf.h"
#include "ease.h"

#include "CATFilePaths.h"

#include "TrackViewFunctions.h"
#include "itreevw.h"
#include "tvnode.h"

#include "ICATParent.h"

// NavigatorData holds stuff we need to store in the navigator
// list box using LB_SETITEMDATA.  It is implemented as a double-
// linked list.
//
// To add an item or a list to the current node, there are two
// functions available called Insert() and Append().
//
// To unlink the current node just call delete.
//
// To destroy the entire list call DestroyEntireList().
//
#define STRING_BUFFER_SIZE 256

class NavigatorData {
	NavigatorData *next, *prev;

public:
	NavigatorData(CATHierarchyBranch *pBranch, const TCHAR* pName, int nLevel = 0) {
		next = prev = NULL;
		branch = pBranch;
		level = nLevel;

		name = new TCHAR[_tcslen(pName) + 4];
		if (name) _tcscpy(name, pName);
	}

	~NavigatorData() {
		if (prev) prev->next = this->next;
		if (next) next->prev = this->prev;
		if (name) delete[] name;
	}

	// Inserts a list before this node.
	void Insert(NavigatorData *head) {
		DbgAssert(head->prev == NULL);

		// Find the tail of the list being added
		NavigatorData *tail = head;
		while (tail->next) tail = tail->next;

		// Link
		if (this->prev) this->prev->next = head;
		head->prev = this->prev;
		tail->next = this;
		this->prev = tail;
	}

	// Appends a list after this node.
	void Append(NavigatorData *head) {
		DbgAssert(head->prev == NULL);

		// Find the tail of the list being added
		NavigatorData *tail = head;
		while (tail->next) tail = tail->next;

		// Link
		if (this->next) this->next->prev = tail;
		tail->next = this->next;
		head->prev = this;
		this->next = head;
	}

	// This function destroys the entire list (surprisingly
	// enough), regardless of which node you call it from.
	void DestroyEntireList() {
		NavigatorData *l, *dead;

		// Destroy to head
		for (l = this->prev; l; ) {
			dead = l;
			l = l->prev;
			delete dead;
		}

		// Destroy to tail
		for (l = this->next; l; ) {
			dead = l;
			l = l->next;
			delete dead;
		}

		// Destroy this!
		delete this;
	}

	// These allow someone else to iterate over the list.
	NavigatorData *Next() const { return next; }
	NavigatorData *Prev() const { return prev; }

	// And all this so we can store a handful of
	// things in the list box item data!!
	CATHierarchyBranch *branch;
	TCHAR *name;
	int level;
};

////////////////////////////////////////////////////////////////////////////////
// CAT Window Panes go here.  The pane is the region to the right of the
// hierarchy navigator.  Selecting an item in the navigator will cause
// a certain pane to be shown.
////////////////////////////////////////////////////////////////////////////////

//
// Superclass for any CAT Window Pane.
//
class CATWindowPane : public CATDialog, public TimeChangeCallback, public ReferenceMaker
{
protected:

	NavigatorData *listNavigatorCurrent;

public:
	CATHierarchyRoot *root;

	CATWindowPane() : CATDialog() {
		listNavigatorCurrent = NULL;
		root = NULL;
	}

	~CATWindowPane() {
	}

	virtual void UpdateCATRig() { if (root)	root->GetCATParentTrans()->UpdateCharacter(); };

protected:

	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate)
	{
		UNREFERENCED_PARAMETER(changeInt);
		UNREFERENCED_PARAMETER(hTarget);
		UNREFERENCED_PARAMETER(partID);
		UNREFERENCED_PARAMETER(message);
		TimeChanged(GetCOREInterface()->GetTime());
		UpdateCATRig();
		return REF_SUCCEED;
	}
};

////////////////////////////////////////////////////////////////////////////////
// Spinner Management Functions

void ControlToSpinner(ISpinnerControl *spn, Control* ctrl, TimeValue t, float mult = 1.0f) {
	if (ctrl) {
		float val;
		Interval iv = FOREVER;
		ctrl->GetValue(t, (void*)&val, iv);
		spn->SetValue(val * mult, FALSE);
		spn->SetKeyBrackets(ctrl->IsKeyAtTime(t, NULL));
	}
	else {
		spn->Disable();
	}
}

void SpinnerToControl(ISpinnerControl *spn, Control* ctrl, TimeValue t, CATWindowPane *pane) {
	if (ctrl) {
		BOOL newundo = FALSE;
		if (!theHold.Holding()) {
			theHold.Begin();
			newundo = TRUE;
		}
		float val = spn->GetFVal();
		ctrl->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
		spn->SetKeyBrackets(ctrl->IsKeyAtTime(t, NULL));

		if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_SETTINGCHANGE)); }
	}
}

void SpinnerButtonUp(BOOL accept, TCHAR* undo_msg) {
	if (accept)	theHold.Accept(undo_msg);
	else		theHold.Cancel();
}

void ValuetoSpinnerAndControl(ISpinnerControl *spn, Control* ctrl, TimeValue t, float val, CATWindowPane *pane) {
	spn->SetValue(val, FALSE);
	SpinnerToControl(spn, ctrl, t, pane);
}
////////////////////////////////////////////////////////////////////////////////
// Slider Management Functions

void ControlToSlider(ISliderControl *sld, Control* ctrl, TimeValue t) {
	if (ctrl) {
		float val;
		Interval iv = FOREVER;
		ctrl->GetValue(t, (void*)&val, iv);
		sld->SetValue(val, FALSE);
		sld->SetKeyBrackets(ctrl->IsKeyAtTime(t, NULL));
	}
	else {
		sld->Disable();
	}
}

void SliderToControl(ISliderControl *sld, Control* ctrl, TimeValue t, CATWindowPane *pane) {
	if (ctrl) {
		BOOL newundo = FALSE;
		if (!theHold.Holding()) {
			theHold.Begin();
			newundo = TRUE;
		}
		float val = sld->GetFVal();
		ctrl->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
		sld->SetKeyBrackets(ctrl->IsKeyAtTime(t, NULL));
		if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_SETTINGCHANGE)); }
	}
}

void SliderButtonUp(BOOL accept, TCHAR* undo_msg) {
	if (accept)	theHold.Accept(undo_msg);
	else		theHold.Cancel();
}

////////////////////////////////////////////////////////////////////////////////
// Welcome Pane: Displays a CAT splash screen.
////////////////////////////////////////////////////////////////////////////////
class CATWindowWelcome : public CATWindowPane
{
	WORD GetDialogTemplateID() const { return IDD_CATWINDOW_WELCOME; }

public:

	CATWindowWelcome() : CATWindowPane() { }

	// We have no controls.
	void InitControls() {};
	void ReleaseControls() {};

	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void TimeChanged(TimeValue) {};
};

INT_PTR CALLBACK CATWindowWelcome::DialogProc(HWND, UINT uMsg, WPARAM, LPARAM)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		return TRUE;

	}
	return FALSE;
}

/*************************
 * LoadClip dialogue
*/
class LoadCATMotionDlg
{
private:
	CATHierarchyRoot* root;
	//	ECATParent* catparent;
	HWND hWnd;

	TSTR filename, presetname;

public:

	LoadCATMotionDlg() : hWnd(NULL), root(NULL) {}

	void InitControls() {
		SendMessage(GetDlgItem(hWnd, IDC_RDO_LOADNEW), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	}

	void ReleaseControls()
	{
		hWnd = NULL;
	}

	void SetCATHierarchyRoot(CATHierarchyRoot *pClipRoot, TSTR filename, TSTR presetname) {
		this->root = pClipRoot;
		this->filename = filename;
		this->presetname = presetname;

	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM)
	{
		static HWND hwndOldMouseCapture;
		switch (uMsg) {
		case WM_INITDIALOG:
			this->hWnd = hWnd;

			// Take away any mouse capture that was active.
			hwndOldMouseCapture = GetCapture();
			SetCapture(NULL);

			// Centre the dialog in its parent window and set the
			// focus to the OK button, if it exists.
			CenterWindow(hWnd, GetParent(hWnd));
			if (GetDlgItem(hWnd, IDC_BTN_LOAD)) SetFocus(GetDlgItem(hWnd, IDC_BTN_LOAD));
			InitControls();

			return FALSE;
		case WM_DESTROY:
			ReleaseControls();

			// Restore the mouse capture.
			SetCapture(hwndOldMouseCapture);
			return TRUE;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
			{
				switch (LOWORD(wParam)) {
				case IDC_RDO_LOADEXISTING:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_LOADNEW), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					break;

				case IDC_RDO_LOADNEW:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_LOADEXISTING), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					break;

				case IDC_BTN_LOAD:
				{
					BOOL bNewLayer = TRUE;
					if (SendMessage(GetDlgItem(hWnd, IDC_RDO_LOADEXISTING), BM_GETCHECK, 0, 0) == BST_CHECKED)
						bNewLayer = FALSE;
					root->LoadPreset(filename, presetname, bNewLayer);
				}
				case IDC_BTN_CANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					return TRUE;
				}
			}
			break;
			}
		}
		return FALSE;
	}
};

static LoadCATMotionDlg LoadCATMotionDlgClass;

static INT_PTR CALLBACK LoadCATMotionDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return LoadCATMotionDlgClass.DlgProc(hWnd, message, wParam, lParam);
};

/************************************************************************/
/* End LoadCATMotionRollout stuff                                            */
/************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// Presets: A Dialog for managing presets
////////////////////////////////////////////////////////////////////////////////
class CATWindowPresets : public CATWindowPane //, public TimeChangeCallback
{
	HWND hAvailable;
	HWND hLoaded;

	//	CATHierarchyRoot* root;
	CATHierarchyLeaf* mpWeights;

	ICustEdit *ccEditWeight;
	ISpinnerControl *spnWeight;
	Control* ctrlWeight;

	// I need to know how to manage buttons
	ICustButton *btnBrowseCATMotionPreset;
	ICustButton *btnCATMotionWeightsView;
	ICustButton *btnSaveCATMotionPreset;
	ICustButton *btnAddCATMotionLayer;
	ICustButton *btnRemoveCATMotionLayer;

	ICustEdit *edtLayerName;

	HIMAGELIST hImageBrowseBtn;
	HIMAGELIST hImageSaveBtn;
	HIMAGELIST hImageAddBtn;
	HIMAGELIST hImageRemoveBtn;

	std::vector<TCHAR*> vecFileNames;
	std::vector<TCHAR*> vecPresetNames;
	std::vector<TCHAR*> vecFolderNames;

	WORD GetDialogTemplateID() const { return IDD_CATWINDOW_PRESETS; }
	void InitControls();
	void ReleaseControls();

	void ClearFileList();
	BOOL ChangeDirectory(const TCHAR *szNewDir);
	void RefreshAvailablePresets();
	void RefreshLoadedPresets();
	void RefreshButtons();

	TCHAR strCurrentFolder[MAX_PATH];
	bool bShowFolders;

	enum CONTROLLIST {
		WEIGHT,
		NUM_CONTROLS,
	};

public:

	CATWindowPresets(CATHierarchyRoot* pRoot)
		: CATWindowPane(), vecFileNames(), vecPresetNames(), vecFolderNames(),
		ccEditWeight(NULL), hImageAddBtn(NULL), hImageRemoveBtn(NULL), hImageSaveBtn(NULL), hImageBrowseBtn(NULL)
	{
		root = pRoot;

		mpWeights = root->GetWeights();
		DbgAssert(mpWeights);

		hAvailable = NULL;
		hLoaded = NULL;
		spnWeight = NULL;
		ctrlWeight = NULL;

		btnBrowseCATMotionPreset = NULL;
		btnCATMotionWeightsView = NULL;
		btnSaveCATMotionPreset = NULL;
		btnAddCATMotionLayer = NULL;
		btnRemoveCATMotionLayer = NULL;

		edtLayerName = NULL;

		bShowFolders = true;
		_tcscpy(strCurrentFolder, _T(""));

		CatDotIni catCfg;
		if (!ChangeDirectory(catCfg.Get(INI_MOTION_PRESET_PATH)))
			ChangeDirectory(GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	}
	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void TimeChanged(TimeValue t);

	virtual int NumRefs() { return NUM_CONTROLS; }
	virtual RefTargetHandle GetReference(int i)
	{
		DbgAssert(i == WEIGHT);
		return (i == WEIGHT) ? ctrlWeight : nullptr;
	}

private:

	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		DbgAssert(i == WEIGHT);
		if (i == WEIGHT)
			ctrlWeight = dynamic_cast<Control*>(rtarg);
	}
};

// Initialises all our controls.
void CATWindowPresets::InitControls()
{
	hAvailable = GetDlgItem(hDlg, IDC_LIST_AVAILABLE);
	hLoaded = GetDlgItem(hDlg, IDC_LIST_LOADED);

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	// Initialise custom Max controls
	edtLayerName = GetICustEdit(GetDlgItem(hDlg, IDC_EDIT_CATLAYERNAME));
	spnWeight = SetupFloatSpinner(hDlg, IDC_SPIN_WEIGHT, IDC_EDIT_WEIGHT, 0.0f, 100.0f, 0.0f);

	// Initialise the Browse Button
	btnBrowseCATMotionPreset = GetICustButton(GetDlgItem(hDlg, IDC_BTN_BROWSE));
	btnBrowseCATMotionPreset->SetType(CBT_PUSH);
	btnBrowseCATMotionPreset->SetButtonDownNotify(TRUE);
	btnBrowseCATMotionPreset->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnBrowseCATMotionPreset"), GetString(IDS_TT_CMPRESET)));
	btnBrowseCATMotionPreset->SetImage(hIcons, 8, 8, 8 + 25, 8 + 25, 24, 24);

	// Initialise the TrackView Button
	btnCATMotionWeightsView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_TRACKVIEW));
	btnCATMotionWeightsView->SetType(CBT_PUSH);
	btnCATMotionWeightsView->SetButtonDownNotify(TRUE);
	btnCATMotionWeightsView->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCATMotionWeightsView"), GetString(IDS_TT_CURVEEDT_CMWGTS)));
	btnCATMotionWeightsView->SetImage(hIcons, 13, 13, 13 + 25, 13 + 25, 24, 24);

	// Initialise the Save Button
	btnSaveCATMotionPreset = GetICustButton(GetDlgItem(hDlg, IDC_BTN_SAVE));
	btnSaveCATMotionPreset->SetType(CBT_PUSH);
	btnSaveCATMotionPreset->SetButtonDownNotify(TRUE);
	btnSaveCATMotionPreset->SetTooltip(TRUE, (catCfg.Get(_T("ToolTips"), _T("btnSaveCATMotionPreset"), GetString(IDS_TT_SAVECMPRESET))));
	btnSaveCATMotionPreset->SetImage(hIcons, 6, 6, 6 + 25, 6 + 25, 24, 24);

	// Initialise the Add Button
	btnAddCATMotionLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADD));
	btnAddCATMotionLayer->SetType(CBT_PUSH);
	btnAddCATMotionLayer->SetButtonDownNotify(TRUE);
	btnAddCATMotionLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddCATMotionLayer"), GetString(IDS_TT_ADDCMLAYER)));
	btnAddCATMotionLayer->SetImage(hIcons, 11, 11, 11 + 25, 11 + 25, 24, 24);

	// Initialise the Remove Button
	btnRemoveCATMotionLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_REMOVE));
	btnRemoveCATMotionLayer->SetType(CBT_PUSH);
	btnRemoveCATMotionLayer->SetButtonDownNotify(TRUE);
	btnRemoveCATMotionLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnRemoveCATMotionLayer"), GetString(IDS_TT_DELCMLAYER)));
	if (root->GetNumLayers() > 1) btnRemoveCATMotionLayer->Enable(TRUE);
	else								   btnRemoveCATMotionLayer->Enable(FALSE);
	btnRemoveCATMotionLayer->SetImage(hIcons, 7, 7, 7 + 25, 7 + 25, 24, 24);

	hAvailable = GetDlgItem(hDlg, IDC_LIST_AVAILABLE);

	hLoaded = GetDlgItem(hDlg, IDC_LIST_LOADED);
	// Get the width of the list box in screen coordinates, and set
	// the tab stops using that information.
	RECT rcListBoxRect;
	GetClientRect(hLoaded, &rcListBoxRect);
	POINT pos = { rcListBoxRect.right, rcListBoxRect.bottom };
	ClientToScreen(hLoaded, &pos);

	TEXTMETRIC text;
	HDC hdcListBox = GetDC(hLoaded);
	GetTextMetrics(hdcListBox, &text);
	ReleaseDC(hLoaded, hdcListBox);

	int tabstops[1] = {
		MulDiv(text.tmAveCharWidth * 8, 4, LOWORD(GetDialogBaseUnits()))
	};
	SendMessage(hLoaded, LB_SETTABSTOPS, 2, (LPARAM)tabstops);

	RefreshAvailablePresets();
	RefreshLoadedPresets();
	RefreshButtons();

	root->SetCurWindowPtr(this);
	GetCOREInterface()->RegisterTimeChangeCallback(this);
}

// Releases all our custom Max controls
void CATWindowPresets::ReleaseControls()
{
	DbgAssert(hDlg != NULL);

	GetCOREInterface()->UnRegisterTimeChangeCallback(this);

	ClearFileList();

	SAFE_RELEASE_EDIT(edtLayerName);
	SAFE_RELEASE_SPIN(spnWeight);

	SAFE_RELEASE_BTN(btnBrowseCATMotionPreset);
	SAFE_RELEASE_BTN(btnCATMotionWeightsView);
	SAFE_RELEASE_BTN(btnSaveCATMotionPreset);
	SAFE_RELEASE_BTN(btnAddCATMotionLayer);
	SAFE_RELEASE_BTN(btnRemoveCATMotionLayer);

	hDlg = NULL;

	if (*root->GetCurWindowPtrAddr() == this) root->SetCurWindowPtr(NULL);
	DeleteAllRefsFromMe();
}

void CATWindowPresets::ClearFileList() {
	unsigned int i;
	SendMessage(hAvailable, LB_RESETCONTENT, 0, 0);

	for (i = 0; i < vecFileNames.size(); i++) {
		delete[] vecFileNames[i];
		delete[] vecPresetNames[i];
	}

	for (i = 0; i < vecFolderNames.size(); i++) {
		delete[] vecFolderNames[i];
	}

	vecFileNames.resize(0);
	vecPresetNames.resize(0);
	vecFolderNames.resize(0);
}

BOOL CATWindowPresets::ChangeDirectory(const TCHAR *szNewDir)
{
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

#define LBL_LENGTH 64

// Load up all existing preset names into the ListBox
void CATWindowPresets::RefreshAvailablePresets()
{
	WIN32_FIND_DATA find;
	HANDLE hFind;

	TCHAR searchName[MAX_PATH] = { 0 };
	TCHAR fileName[MAX_PATH] = { 0 };

	int id;

	ClearFileList();

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

					id = (int)SendMessage(hAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)driveName);
					SendMessage(hAvailable, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
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
			id = (int)SendMessage(hAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)_T("<..>"));
			SendMessage(hAvailable, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
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
							id = (int)SendMessage(hAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)fileName);
							SendMessage(hAvailable, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
							vecFolderNames.push_back(folder);
						}
					}
				}
			} while (FindNextFile(hFind, &find));

			FindClose(hFind);
		}
	}

	// Search for all motion presets in the current directory and
	// insert them into the list box, while also maintaining them
	// in a vector.
	_stprintf(searchName, _T("%s\\*.%s"), strCurrentFolder, CAT_MOTION_PRESET_EXT);
	hFind = FindFirstFile(searchName, &find);
	if (hFind == INVALID_HANDLE_VALUE) return;

	do {
		if (find.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
			// Copy the filename and remove the extension.
			_tcscpy(fileName, find.cFileName);
			fileName[_tcslen(fileName) - _tcslen(CAT_MOTION_PRESET_EXT) - 1] = _T('\0');

			// We need to save the full path, so allocate some
			// memory.  If this fails, don't even bother adding
			// it to the list box.
			TCHAR *file = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];
			TCHAR *preset = new TCHAR[_tcslen(fileName) + 4];

			if (file && preset) {
				_stprintf(file, _T("%s\\%s"), strCurrentFolder, find.cFileName);
				_tcscpy(preset, fileName);
				id = (int)SendMessage(hAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)preset);
				SendMessage(hAvailable, LB_SETITEMDATA, id, (LPARAM)vecFileNames.size());
				vecFileNames.push_back(file);
				vecPresetNames.push_back(preset);
			}
			else {
				// One of these must have failed, for some stupid reason.
				// May as well clean up even though there's no point, cos
				// the system will have died horribly before now.
				delete[] file;
				delete[] preset;
			}
		}
	} while (FindNextFile(hFind, &find));

	FindClose(hFind);
}

// Load up all existing preset names into the ListBox
void CATWindowPresets::RefreshLoadedPresets()
{
	DbgAssert(mpWeights != NULL);
	if (mpWeights == NULL)
		return;

	SendMessage(hLoaded, LB_RESETCONTENT, 0, 0);
	TSTR strPresetLabel = _T("");
	TSTR strPresetName;

	float dWeight;
	TCHAR strbuf[128];

	TimeValue t = GetCOREInterface()->GetTime();
	int nActiveLayer = mpWeights->GetActiveLayer();

	int i;
	// so we dont break our current weight
	Control* ctrlEnumWeight = NULL;
	for (i = 0; i < mpWeights->GetNumLayers(); i++)
	{

		ctrlEnumWeight = mpWeights->GetLayer(i);
		Interval iv = FOREVER;
		ctrlEnumWeight->GetValue(t, (void*)&dWeight, iv, CTRL_ABSOLUTE);
		if (i == 0) dWeight = 1.0f;

		dWeight *= 100.0f;
		strPresetName = mpWeights->GetLayerName(i);

		_sntprintf(strbuf, 128, _T("(%d%%)\t%s"),
			(int)dWeight,
			strPresetName.data());
		int result = (int)SendMessage(hLoaded, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)strbuf);
		UNREFERENCED_PARAMETER(result);
		DbgAssert(result != LB_ERR);
	}
	if (nActiveLayer >= 0)
		SendMessage(hLoaded, LB_SETCURSEL, nActiveLayer, 0);
}

void CATWindowPresets::RefreshButtons()
{
	DbgAssert(mpWeights != NULL);
	if (mpWeights == NULL)
		return;

	int nNumLayers = mpWeights->GetNumLayers();
	int nActiveLayer = mpWeights->GetActiveLayer();
	TimeValue t = GetCOREInterface()->GetTime();
	float dWeight = 0.0f;

	if ((nNumLayers > 1) && (nActiveLayer > 0) && (nActiveLayer < nNumLayers)) {
		spnWeight->Enable();
		ReplaceReference(WEIGHT, mpWeights->GetLayer(nActiveLayer));
		Interval iv = FOREVER;
		ctrlWeight->GetValue(t, (void*)&dWeight, iv);
		dWeight *= 100.0f;
		spnWeight->SetValue(dWeight, FALSE);

		if (ctrlWeight->IsKeyAtTime(t, NULL))
			spnWeight->SetKeyBrackets(TRUE);
		else spnWeight->SetKeyBrackets(FALSE);

		btnRemoveCATMotionLayer->Enable(TRUE);
	}
	else {
		DeleteReference(WEIGHT);
		spnWeight->SetValue(100.0f, FALSE);
		spnWeight->Disable();

		TSTR strPresetName = _T("");
		edtLayerName->SetText(strPresetName.data());

		btnRemoveCATMotionLayer->Disable();
	}

	if ((nActiveLayer >= 0) && (nActiveLayer < nNumLayers))
	{
		TSTR strPresetName = mpWeights->GetLayerName(nActiveLayer);
		edtLayerName->SetText(strPresetName.data());
	}
}

void CATWindowPresets::TimeChanged(TimeValue t) {
	RefreshLoadedPresets();

	ControlToSpinner(spnWeight, ctrlWeight, t, 100.0f);
}

INT_PTR CALLBACK CATWindowPresets::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM)
{
	TimeValue t = GetCOREInterface()->GetTime();

	switch (uMsg)
	{
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
			// Commands to the hierarchy navigator...  Possible commands are:
			//
			//  * single-click to select the item and display its rollout.
			//  * double-click to select the item, display its rollout,
			//    and expand or collapse it.
			//
		case IDC_LIST_AVAILABLE:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE: {
				int item;
				item = (int)SendMessage(hAvailable, LB_GETCURSEL, 0, 0);
				if (item >= 0) {

				}
				break;
			}
			case LBN_DBLCLK: {
				int item;
				item = (int)SendMessage(hAvailable, LB_GETCURSEL, 0, 0);
				if (item >= 0) {
					// User double-clicked on a preset.  Get the index within
					// vecFileNames out of the list box item data.  If it is
					// negative, it refers to a directory.  If positive or zero,
					// it is the index of a preset.
					int nFileID = (int)SendMessage(hAvailable, LB_GETITEMDATA, item, 0);

					if (nFileID < 0) {
						HCURSOR hCurrentCursor = ::GetCursor();
						SetCursor(LoadCursor(NULL, IDC_WAIT));
						BOOL bSuccess = ChangeDirectory(vecFolderNames[-nFileID - 1]);
						SetCursor(hCurrentCursor);

						if (!bSuccess) {
							::MessageBox(
								hWnd,
								GetString(IDS_ERR_NOACCESS),
								GetString(IDS_ERROR),
								MB_OK | MB_ICONSTOP);
						}
						RefreshAvailablePresets();
					}
					else {
						if (root->GetActiveLayer() < root->GetNumLayers()) {
							// pop up the options dialogue
							LoadCATMotionDlgClass.SetCATHierarchyRoot(root, TSTR(vecFileNames[nFileID]), TSTR(vecPresetNames[nFileID]));
							DialogBox(hInstance, MAKEINTRESOURCE(IDD_LOAD_CATMOTION_OPTIONS_MODAL), GetCOREInterface()->GetMAXHWnd(), LoadCATMotionDlgProc);
						}
						else root->LoadPreset(TSTR(vecFileNames[nFileID]), TSTR(vecPresetNames[nFileID]), TRUE);

						RefreshLoadedPresets();
						RefreshButtons();
					}
				}
				break;
			}
			}
			break;

		case IDC_LIST_LOADED: {
			int item = (int)SendMessage(hLoaded, LB_GETCURSEL, 0, 0);
			if (item != LB_ERR) {
				switch (HIWORD(wParam)) {
				case LBN_SELCHANGE: {
					if ((item >= 0) && (item < root->GetNumLayers()))
						// Make the Selected item active for manipulation
						root->GetWeights()->SetActiveLayer(item);
					RefreshLoadedPresets();
					RefreshButtons();
				}
				} break;
			}
		}
		}
	}

						  switch (HIWORD(wParam))
						  {
						  case BN_BUTTONUP:
							  switch (LOWORD(wParam))
							  {
							  case IDC_BTN_BROWSE:
							  {
								  TCHAR filename[MAX_PATH];
								  if (DialogOpenMotionPreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH)) {
									  int i = (int)_tcslen(filename) - 1;
									  while (i >= 0 && filename[i] != _T('\\')) i--;

									  if (root->GetActiveLayer() < root->GetNumLayers()) {
										  // pop up the options dialogue
										  LoadCATMotionDlgClass.SetCATHierarchyRoot(root, TSTR(filename), TSTR(&filename[i + 1]));
										  DialogBox(hInstance, MAKEINTRESOURCE(IDD_LOAD_CATMOTION_OPTIONS_MODAL), GetCOREInterface()->GetMAXHWnd(), LoadCATMotionDlgProc);
										  //	root->LoadPreset(TSTR(filename), TSTR(&filename[i+1]), FALSE);
									  }
									  else
										  root->LoadPreset(TSTR(filename), TSTR(&filename[i + 1]), TRUE);

									  RefreshLoadedPresets();
									  RefreshButtons();
								  }
							  }
							  break;
							  case IDC_BTN_SAVE:
							  {
								  TCHAR filename[MAX_PATH];
								  if (DialogSaveMotionPreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH, _T("somefilename"))) {
									  root->SavePreset(TSTR(filename));
									  //root->GetCATParentTrans()->SaveClip(TSTR(filename), FALSE, 0);
									  RefreshAvailablePresets();
								  }
								  break;
							  }
							  case IDC_BTN_ADD:
							  {
								  root->AddLayer();
								  RefreshLoadedPresets();
								  RefreshButtons();
								  break;
							  }
							  case IDC_BTN_REMOVE:
							  {
								  DeleteAllRefsFromMe();
								  int nActiveLayer = root->GetActiveLayer();
								  root->RemoveLayer(nActiveLayer);

								  RefreshLoadedPresets();
								  RefreshButtons();
								  break;
							  }
							  case IDC_BTN_TRACKVIEW:
							  {
								  DbgAssert(root);

								  root->pCATTreeView = GetTrackView(_T("CATMotion Tracks")); // globalize? - no, need to get this global track name
								  if (root->pCATTreeView) {
									  root->pCATTreeView->SetEditMode(MODE_EDITFCURVE);
									  ITreeViewUI_ShowControllerWindow(root->pCATTreeView);

									  ITrackViewNode *pTrackViewNode = CreateITrackViewNode(CAT_HIDE_TV_ROOT_TRACKS);
									  root->pCATTreeView->SetRootTrack(pTrackViewNode);
									  pTrackViewNode->AddController(root->GetWeights(), _T("CATMotion Weights"), CAT_TRACKVIEW_TVCLSID); // globalize? - no, need to get this global track name

									  root->pCATTreeView->ExpandTracks();
									  root->pCATTreeView->ZoomOn(root->GetWeights(), 0);
									  root->pCATTreeView->SelectTrack(root->GetWeights());
								  }
								  break;
							  }
							  }
							  break;
						  }
						  return TRUE;
						  // WM_COMMAND

				  //	}
	case CC_SPINNER_BUTTONDOWN:
		if (!theHold.Holding()) {
			theHold.Begin();
		}
		break;
	case CC_SPINNER_CHANGE:
	{
		if (ctrlWeight) {
			BOOL newundo = FALSE;
			if (!theHold.Holding()) {
				theHold.Begin();
				newundo = TRUE;
			}
			float val = (float)spnWeight->GetFVal() / 100.0f;
			ctrlWeight->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
			spnWeight->SetKeyBrackets(ctrlWeight->IsKeyAtTime(t, NULL));

			if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_HLD_CMSETTING)); }

			//	SpinnerToControl((ISpinnerControl*)lParam,		ctrlWeight,	t, this);
			RefreshLoadedPresets();
		}
		break;
	}
	case CC_SPINNER_BUTTONUP:
		if (HIWORD(wParam))	theHold.Accept(GetString(IDS_HLD_PRESETWGT));
		else				theHold.Cancel();
		// make sure we update steps because weights affects stridelength and steptime
		root->IUpdateSteps();
		break;
	case WM_CUSTEDIT_ENTER:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_WEIGHT:
			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_PRESETWGT));
			break;
		case IDC_EDIT_CATLAYERNAME: {
			int nActiveLayer = root->GetWeights()->GetActiveLayer();

			if ((nActiveLayer >= 0) && (nActiveLayer < mpWeights->GetNumLayers()))
			{
				TCHAR strbuf[128];
				edtLayerName->GetText(strbuf, LBL_LENGTH);
				root->GetWeights()->SetLayerName(root->GetActiveLayer(), strbuf);
				RefreshLoadedPresets();
			}
			else
			{
				edtLayerName->SetText(_T(""));
			}
		}
											 break;
		}
		break;
	}
	return FALSE;
}

/* ------------------ CommandMode classes for node picking ----------------------- */
class PickNodeButtonFilter : public PickNodeCallback
{
public:
	BOOL geom;
	BOOL Filter(INode *node);
	void Setup(BOOL selGeom) { geom = selGeom; }
};
BOOL PickNodeButtonFilter::Filter(INode *node)
{
	if (geom == TRUE) {
		ObjectState os = node->EvalWorldState(GetCOREInterface()->GetTime());
		return os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0));
	}
	return TRUE;
}

static PickNodeButtonFilter pick_filter;

enum PICKMODES {
	PICK_PATHNODE,
	PICK_GROUND
};
static BOOL bOnlySelected = FALSE;

class PickMode : public PickModeCallback
{
private:
	CATHierarchyRoot *root;
	INode*			node;

	ICustButton*	pnb;
	TCHAR*			msg;
	int				pickid;

public:

	PickNodeCallback *GetFilter() { return &pick_filter; }
	BOOL	RightClick(IObjParam *ip, ViewExp *vpt) { UNREFERENCED_PARAMETER(ip); UNREFERENCED_PARAMETER(vpt); return TRUE; }

	void EnterMode(IObjParam *ip)
	{
		if (pnb != NULL)	pnb->SetCheck(TRUE);
		if (msg != NULL)	ip->PushPrompt(msg);
	}

	void ExitMode(IObjParam *ip)
	{
		if (pnb != NULL)	pnb->SetCheck(FALSE);
		if (msg != NULL)	ip->PopPrompt();
	}

	// the user is moving the mouse over the window
	BOOL HitTest(IObjParam *ip, HWND hWnd, ViewExp *, IPoint2 m, int)
	{
		return ip->PickNode(hWnd, m, &pick_filter) ? TRUE : FALSE;
	}

	// The user has picked something
	BOOL Pick(IObjParam *, ViewExp *vpt)
	{
		node = vpt->GetClosestHit();
		TSTR name = node->GetName();
		if (pickid == PICK_PATHNODE) {
			root->ISetPathNode(node);
		}
		else if (pickid == PICK_GROUND) {
			root->ISnapToGround(node, bOnlySelected);
		}
		return TRUE;
	}

	void Setup(CATHierarchyRoot *root, TCHAR* msg, ICustButton* pnb, PICKMODES pickid)
	{
		this->root = root;
		this->pickid = pickid;
		this->msg = msg;
		this->pnb = pnb;
		if (pickid == PICK_PATHNODE)			pick_filter.Setup(FALSE);
		else if (pickid == PICK_GROUND)		pick_filter.Setup(TRUE);
	}
};

static PickMode pick_mode;

////////////////////////////////////////////////////////////////////////////////
// Presets: A Dialog for managing Globals Params
////////////////////////////////////////////////////////////////////////////////

class CATWindowGlobals : public CATWindowPane
{
private:
	INode*				node;
	void	TimeChanged(TimeValue t);
	WORD GetDialogTemplateID() const { return IDD_CATWINDOW_GLOBALS; }
	void InitControls();
	void ReleaseControls();

	enum CONTROLLIST {
		ROOT,
		MAX_STEP_T,
		MAX_STRIDE_L,
		DIRECTION,
		GRADIENT,
		KA_LIFTING,
		PATH_FACING,
		NUM_CONTROLS
	};

public:

	CATHierarchyBranch2* ctrlGlobalsBranch;
	ICATParentTrans* catparenttrans;

	ICustButton *btnTrackView;

	ISpinnerControl *spnMaxStepTime;
	ISpinnerControl *spnMaxStrideLength;
	ISpinnerControl *spnVelocity;
	ISpinnerControl *spnDirection;
	ISpinnerControl *spnGradient;

	Control* ctrlMaxStepT;
	Control* ctrlMaxStrideL;
	Control* ctrlDirection;
	Control* ctrlGradient;

	ISpinnerControl *spnStartTime;
	ISpinnerControl *spnEndTime;

	ISliderControl *sldPathFacing;
	Control* ctrlPathFacing;

	ISliderControl *sldKALifting;
	Control* ctrlKALifting;
	Control* ctrlIKKA;

	ICustButton *btnPickPathNode;

	CATWindowGlobals(CATHierarchyRoot* pRoot, CATHierarchyBranch* ctrlGlobals) : CATWindowPane() {
		UNREFERENCED_PARAMETER(pRoot);

		root = nullptr;
		ctrlMaxStepT = nullptr;
		ctrlMaxStrideL = nullptr;
		ctrlDirection = nullptr;
		ctrlGradient = nullptr;
		ctrlKALifting = nullptr;
		ctrlPathFacing = nullptr;

		ctrlGlobalsBranch = (CATHierarchyBranch2*)ctrlGlobals;
		ReplaceReference(ROOT, ctrlGlobalsBranch->GetCATRoot());
		catparenttrans = root->GetCATParentTrans();

		btnTrackView = NULL;

		// globalize: This is an internal name, used for debugging. It is NOT stored in a file, so globalization is irrelevant here.  (SA 10/09)
		ReplaceReference(MAX_STEP_T, ctrlGlobalsBranch->GetLeaf(GetString(IDS_MAX_STEP_TIME)));
		ReplaceReference(MAX_STRIDE_L, ctrlGlobalsBranch->GetLeaf(GetString(IDS_MAX_STEP_LENGTH)));
		ReplaceReference(DIRECTION, ctrlGlobalsBranch->GetLeaf(GetString(IDS_DIRECTION)));
		ReplaceReference(GRADIENT, ctrlGlobalsBranch->GetLeaf(GetString(IDS_GRADIENT)));
		ReplaceReference(KA_LIFTING, ctrlGlobalsBranch->GetLeaf(GetString(IDS_KABODYLIFTING)));
		ReplaceReference(PATH_FACING, ctrlGlobalsBranch->GetLeaf(GetString(IDS_PATHFACING)));
		ctrlIKKA = NULL;

		spnMaxStepTime = NULL;
		spnMaxStrideLength = NULL;
		spnVelocity = NULL;

		spnGradient = NULL;
		spnDirection = NULL;

		spnStartTime = NULL;
		spnEndTime = NULL;

		sldPathFacing = NULL;
		sldKALifting = NULL;

		node = NULL;

		btnPickPathNode = NULL;
	}

	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual int NumRefs() { return NUM_CONTROLS; }
	virtual RefTargetHandle GetReference(int i)
	{
		switch (i)
		{
		case ROOT: return root;
		case MAX_STEP_T: return ctrlMaxStepT;
		case MAX_STRIDE_L: return ctrlMaxStrideL;
		case DIRECTION: return ctrlDirection;
		case GRADIENT: return ctrlGradient;
		case KA_LIFTING: return ctrlKALifting;
		case PATH_FACING: return ctrlPathFacing;
		default: DbgAssert(false); return nullptr;
		}
	}

protected:

	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		switch (i)
		{
		case ROOT: root = dynamic_cast<CATHierarchyRoot*>(rtarg); break;
		case MAX_STEP_T: ctrlMaxStepT = dynamic_cast<Control*>(rtarg); break;
		case MAX_STRIDE_L: ctrlMaxStrideL = dynamic_cast<Control*>(rtarg); break;
		case DIRECTION: ctrlDirection = dynamic_cast<Control*>(rtarg); break;
		case GRADIENT: ctrlGradient = dynamic_cast<Control*>(rtarg); break;
		case KA_LIFTING: ctrlKALifting = dynamic_cast<Control*>(rtarg); break;
		case PATH_FACING: ctrlPathFacing = dynamic_cast<Control*>(rtarg); break;
		default: DbgAssert(false);
		}
	}
};

void CATWindowGlobals::TimeChanged(TimeValue t)
{

	float steplength;
	Interval iv = FOREVER;
	ctrlMaxStepT->GetValue(t, (void*)&steplength, iv);
	spnMaxStepTime->SetValue(steplength, FALSE);
	spnMaxStepTime->SetKeyBrackets(ctrlMaxStepT->IsKeyAtTime(t, 0));

	float steptime;
	iv = FOREVER;
	ctrlMaxStrideL->GetValue(t, (void*)&steptime, iv);
	spnMaxStrideLength->SetValue(steptime, FALSE);
	spnMaxStrideLength->SetKeyBrackets(ctrlMaxStrideL->IsKeyAtTime(t, 0));

	ControlToSpinner(spnDirection, ctrlDirection, t);
	ControlToSpinner(spnGradient, ctrlGradient, t);

	////////////////////////////////////////////////////////////////////////////////////////
	//
	EnableWindow(GetDlgItem(hDlg, IDC_RDO_WALK_ON_PATHNODE), root->GetPathNode() != NULL);
	SET_CHECKED(hDlg, IDC_RDO_WALK_ON_SPOT, root->GetWalkMode() == CATHierarchyRoot::WALK_ON_SPOT);
	SET_CHECKED(hDlg, IDC_RDO_WALK_ON_LINE, root->GetWalkMode() == CATHierarchyRoot::WALK_ON_LINE);
	SET_CHECKED(hDlg, IDC_RDO_WALK_ON_PATHNODE, root->GetWalkMode() == CATHierarchyRoot::WALK_ON_PATHNODE);

	spnDirection->Enable(root->GetWalkMode() != CATHierarchyRoot::WALK_ON_PATHNODE);
	spnGradient->Enable(root->GetWalkMode() != CATHierarchyRoot::WALK_ON_PATHNODE);

	if (root->GetWalkMode() != CATHierarchyRoot::WALK_ON_PATHNODE) {
		spnVelocity->SetValue((steplength * catparenttrans->GetCATUnits()) / steptime, FALSE);
	}
	else {
		Ease* distcovered = (Ease*)root->GetDistCovered();
		DbgAssert(distcovered);
		int tpf = GetTicksPerFrame();
		float d1 = distcovered->GetValue(t - (TimeValue)((float)tpf / 2.0f));
		float d2 = distcovered->GetValue(t + (TimeValue)((float)tpf / 2.0f));
		// velocity in max units per frame
		float vel = (d2 - d1);
		spnVelocity->SetValue(vel, FALSE);
	}

	ControlToSlider(sldPathFacing, ctrlPathFacing, t);
	ControlToSlider(sldKALifting, ctrlKALifting, t);

	// ST 13/04/04. These two parameters are non-animatable. We update them
	// here anyway cause this time changed could be coming from an undo or
	// sommat like that. There probably is a better way, but none simpler or
	// easier. So who cares?
	float tpf = (float)GetTicksPerFrame();
	Interval iCATMotion = root->GetCATMotionRange();
	spnStartTime->SetValue(iCATMotion.Start() / tpf, FALSE);
	spnEndTime->SetValue(iCATMotion.End() / tpf, FALSE);

	CatDotIni catCfg;
	INode *node = root->GetPathNode();
	TSTR strPathNode = catCfg.Get(_T("ButtonTexts"), _T("btnPickPathNode"), GetString(IDS_BTN_PATHNODE));
	if (node) strPathNode = strPathNode + _T(":") + node->GetName();
	btnPickPathNode->SetText(strPathNode);

	root->SetPathButtons(btnPickPathNode, NULL);
	root->SetCurWindowPtr(this);

};

// Initialises all our controls.
void CATWindowGlobals::InitControls()
{
	float spinnerVal = 0.0f;

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	// TrackView button
	btnTrackView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_TRACKVIEW));
	btnTrackView->SetType(CBT_PUSH);
	btnTrackView->SetButtonDownNotify(TRUE);
	btnTrackView->SetImage(hIcons, 13, 13, 13 + 25, 13 + 25, 24, 24);

	spnMaxStepTime = SetupFloatSpinner(hDlg, IDC_SPIN_MAXSTEPTIME, IDC_EDIT_MAXSTEPTIME, 2.0f, 1000.0f, 2.0f);
	spnMaxStrideLength = SetupFloatSpinner(hDlg, IDC_SPIN_MAXSTRIDELENGTH, IDC_EDIT_MAXSTRIDELENGTH, 2.0f, 10000.0f, 2.0f);
	spnDirection = SetupFloatSpinner(hDlg, IDC_SPIN_DIRECTION, IDC_EDIT_DIRECTION, -360.0f, 360.0f, 0.0f);
	spnGradient = SetupFloatSpinner(hDlg, IDC_SPIN_GRADIENT, IDC_EDIT_GRADIENT, -360.0f, 360.0f, 0.0f);
	sldPathFacing = SetupFloatSlider(hDlg, IDC_SLIDER_PATHFACING, IDC_EDIT_PATHFACING, 0.0f, 1.0f, 0.0f, 4);
	sldPathFacing->SetResetValue(0.0f);

	spnVelocity = SetupFloatSpinner(hDlg, IDC_SPIN_VELOCITY, IDC_EDIT_VELOCITY, 0.0f, 1000.0f, spinnerVal);
	spnVelocity->Disable();
	sldKALifting = SetupFloatSlider(hDlg, IDC_SLIDER_KALIFTING, IDC_EDIT_KALIFTING, 0.0f, 1.0f, 0.0f, 4);
	sldKALifting->SetResetValue(0.0f);

	// PathNode
	btnPickPathNode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PATHNODE));
	btnPickPathNode->SetType(CBT_CHECK);
	btnPickPathNode->SetButtonDownNotify(TRUE);
	btnPickPathNode->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPickPathNode"), GetString(IDS_TT_PICKPATHNODE)));
	node = root->GetPathNode();
	TSTR strPathNode = catCfg.Get(_T("ButtonTexts"), _T("btnPickPathNode"), GetString(IDS_BTN_PATHNODE));
	if (node) strPathNode = strPathNode + _T(":") + node->GetName();
	btnPickPathNode->SetText(strPathNode);

	spnStartTime = SetupFloatSpinner(hDlg, IDC_SPIN_STARTTIME, IDC_EDIT_STARTTIME, -999999.0f, 999999.0f, ((float)root->GetStartTime()) / ((float)GetTicksPerFrame()));
	spnEndTime = SetupFloatSpinner(hDlg, IDC_SPIN_ENDTIME, IDC_EDIT_ENDTIME, -999999.0f, 999999.0f, ((float)root->GetEndTime()) / ((float)GetTicksPerFrame()));

	root->SetCurWindowPtr(this);
	GetCOREInterface()->RegisterTimeChangeCallback(this);
	TimeChanged(GetCOREInterface()->GetTime());
}

// Releases all our custom Max controls
void CATWindowGlobals::ReleaseControls()
{
	GetCOREInterface()->UnRegisterTimeChangeCallback(this);
	GetCOREInterface()->ClearPickMode();

	SAFE_RELEASE_BTN(btnTrackView);

	//	ST - Bug fix, this window is destructed AFTER the next CATWindow is opened,
	//	meaning that setting this to NULL will not delete this windows pointer, but
	//	the current one. Check to see that the pointer still points at us (this
	//	will happen when the window is destructed) before setting it to NULL
	if (*(root->GetCurWindowPtrAddr()) == this)
		root->SetCurWindowPtr(NULL);
	// does not apply to path node, we are the only people who use it
	root->SetPathButtons(NULL, NULL);

	SAFE_RELEASE_SPIN(spnStartTime);
	SAFE_RELEASE_SPIN(spnEndTime);

	SAFE_RELEASE_SLIDER(sldPathFacing);
	SAFE_RELEASE_SLIDER(sldKALifting);

	SAFE_RELEASE_BTN(btnPickPathNode);

	SAFE_RELEASE_SPIN(spnMaxStepTime);
	SAFE_RELEASE_SPIN(spnMaxStrideLength);
	SAFE_RELEASE_SPIN(spnVelocity);

	SAFE_RELEASE_SPIN(spnGradient);
	SAFE_RELEASE_SPIN(spnDirection);

	hDlg = NULL;

	DeleteAllRefsFromMe();
}

INT_PTR CALLBACK CATWindowGlobals::DialogProc(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM)
{
	TimeValue t = GetCOREInterface()->GetTime();

	switch (uMsg) {
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{ // Notification codes
		case BN_CLICKED:
			switch (LOWORD(wParam))
			{
			case IDC_RDO_WALK_ON_SPOT:
			{
				HoldActions hold(IDS_WALK_MODECH);
				root->SetWalkMode(CATHierarchyRoot::WALK_ON_SPOT);
				break;
			}
			case IDC_RDO_WALK_ON_LINE:
			{
				HoldActions hold(IDS_WALK_MODECH);
				root->SetWalkMode(CATHierarchyRoot::WALK_ON_LINE);
				break;
			}
			case IDC_RDO_WALK_ON_PATHNODE:
			{
				HoldActions hold(IDS_WALK_MODECH);
				root->SetWalkMode(CATHierarchyRoot::WALK_ON_PATHNODE);
				break;
			}
			}
			TimeChanged(t);
			catparenttrans->UpdateCharacter();
			GetCOREInterface()->RedrawViews(t);
			break;
		case BN_BUTTONUP:
			switch (LOWORD(wParam)) {
			case IDC_BTN_TRACKVIEW:
				DbgAssert(ctrlGlobalsBranch);

				root->pCATTreeView = GetTrackView(_T("CATMotion Tracks"));// globalize?
				if (root->pCATTreeView) {
					root->pCATTreeView->SetEditMode(MODE_EDITFCURVE);
					ITreeViewUI_ShowControllerWindow(root->pCATTreeView);

					root->pCATTreeView->SetRootTrack(ctrlGlobalsBranch);
					root->pCATTreeView->ExpandTracks();
					root->pCATTreeView->ZoomOn(ctrlGlobalsBranch, 0);
					root->pCATTreeView->SelectTrack(ctrlGlobalsBranch);
				}
				break;
			case IDC_BTN_PATHNODE:
				if (btnPickPathNode->IsChecked())
				{
					TCHAR *msg = GetString(IDS_PICKPATHNODE);
					pick_mode.Setup(root, msg, btnPickPathNode, PICK_PATHNODE);
					GetCOREInterface()->ClearPickMode();
					GetCOREInterface()->SetPickMode(&pick_mode);
				}
				break;
			}
			break;
		}
		return FALSE;
		break;
	case CC_SPINNER_BUTTONDOWN:
		if (!theHold.Holding()) {
			theHold.Begin();
		}
		break;
	case CC_SPINNER_CHANGE:
		if (theHold.Holding()) {
			DisableRefMsgs();
			theHold.Restore();
			EnableRefMsgs();
		}
		// Manage all bracketing of spinners and updating of controllers
		switch (LOWORD(wParam)) { // Switch on ID
		case IDC_SPIN_MAXSTEPTIME:		SpinnerToControl(spnMaxStepTime, ctrlMaxStepT, t, this);	break;
		case IDC_SPIN_MAXSTRIDELENGTH:	SpinnerToControl(spnMaxStrideLength, ctrlMaxStrideL, t, this);	break;
		case IDC_SPIN_VELOCITY:																				break;
		case IDC_SPIN_DIRECTION:		SpinnerToControl(spnDirection, ctrlDirection, t, this);	break;
		case IDC_SPIN_GRADIENT:			SpinnerToControl(spnGradient, ctrlGradient, t, this);	break;
		case IDC_SPIN_STARTTIME:
			if (!theHold.Holding()) {
				theHold.Begin();
			}
			root->SetStartTime(spnStartTime->GetFVal());
			break;
		case IDC_SPIN_ENDTIME:
			if (!theHold.Holding()) {
				theHold.Begin();
			}
			root->SetEndTime(spnEndTime->GetFVal());
			break;
		}
		catparenttrans->UpdateCharacter();
		break;
	case CC_SPINNER_BUTTONUP:
		if (theHold.Holding())
		{
			switch (LOWORD(wParam))
			{
			case IDC_SPIN_MAXSTEPTIME:		SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_STEPTIME));		break;
			case IDC_SPIN_MAXSTRIDELENGTH:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_STRIDELEN));	break;
			case IDC_SPIN_DIRECTION:		SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_DIRECTION));		break;
			case IDC_SPIN_GRADIENT:			SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_GRADIENT));		break;
			case IDC_SPIN_STARTTIME:
			case IDC_SPIN_ENDTIME:			SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_TIMERANGE2));	break;
			}
		}
		if (!GetCOREInterface()->IsAnimPlaying())
			GetCOREInterface()->RedrawViews(t);
		break;
	case CC_SLIDER_BUTTONDOWN:
		if (!theHold.Holding()) {
			theHold.Begin();
		}
		break;
	case CC_SLIDER_CHANGE:
		switch (LOWORD(wParam)) {
		case IDC_SLIDER_PATHFACING:	SliderToControl(sldPathFacing, ctrlPathFacing, t, this);	break;
		case IDC_SLIDER_KALIFTING:	SliderToControl(sldKALifting, ctrlKALifting, t, this);	break;
		}
		if (!GetCOREInterface()->IsAnimPlaying())
			GetCOREInterface()->RedrawViews(t);
		break;
	case CC_SLIDER_BUTTONUP:
		switch (LOWORD(wParam)) {
		case IDC_SLIDER_PATHFACING:	 SliderButtonUp(HIWORD(wParam), GetString(IDS_HLD_PATHFACING));	break;
		case IDC_SLIDER_KALIFTING:	 SliderButtonUp(HIWORD(wParam), GetString(IDS_HLD_RETARGET));	break;
		}
		break;
	case WM_CUSTEDIT_ENTER:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_PATHFACING:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_PATHFACING));		break;
		case IDC_EDIT_KALIFTING:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_KALINFTING));		break;
		case IDC_EDIT_MAXSTEPTIME:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_STEPTIME));		break;
		case IDC_EDIT_MAXSTRIDELENGTH:	if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_STRIDELEN));	break;
		case IDC_EDIT_STARTTIME:
		case IDC_EDIT_ENDTIME:			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_TIMERANGE));	break;
		}
		break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// LimbPhasesScrollPane: A Dialog for managing a scroll list of limb phase
// slider controls.  The scrolling is handled by CATWindowLimbPhases.
////////////////////////////////////////////////////////////////////////////////
class CATWindowLimbPhasesScrollPane : public CATWindowPane //, public TimeChangeCallback
{
	CATHierarchyBranch2* ctrlLimbPhasesBranch;

	std::vector<ISliderControl*> vecSliderPhases;
	std::vector<ICustEdit*> vecEditPhases;
	std::vector<TSTR> vecLabels;

	ICustButton *btnStepMask;
	ICustButton *btnCreateFootPrints;
	ICustButton *btnDeleteFootPrints;
	ICustButton *btnResetFootprints;
	ICustButton *btnPickGround;

	std::vector<RefTargetHandle> ctrlLeaves;
	int nNumLimbs;
	int nViewWidth, nViewHeight;

	void InitControls();
	void ReleaseControls();

public:
	int nPaneSize;
	int nPaneStepSize;

	CATWindowLimbPhasesScrollPane(CATHierarchyBranch2* ctrlPhases)/*, ECATParent* objCATParent)*/
		: CATWindowPane(),
		vecSliderPhases(),
		vecEditPhases(),
		vecLabels()
	{
		ctrlLimbPhasesBranch = ctrlPhases;
		nNumLimbs = 0;
		nPaneSize = 0;
		nPaneStepSize = 0;
		nViewWidth = 0;
		nViewHeight = 0;

		btnStepMask = NULL;
		btnCreateFootPrints = NULL;
		btnDeleteFootPrints = NULL;
		btnResetFootprints = NULL;
		btnPickGround = NULL;

		bOnlySelected = FALSE;
	}

	void SetViewDimensions(int width, int height) {
		nViewWidth = width;
		nViewHeight = height;
	}

	WORD GetDialogTemplateID() const { return IDD_CATWINDOW_EMPTY; }

	void TimeChanged(TimeValue t);

	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int NumRefs() { return nNumLimbs; }
	virtual RefTargetHandle GetReference(int i)
	{
		DbgAssert(i < nNumLimbs);
		return  (i < ctrlLeaves.size()) ? ctrlLeaves[i] : nullptr;
	}

protected:

	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		DbgAssert(i < nNumLimbs);
		if (i >= ctrlLeaves.size())
			ctrlLeaves.resize(i + 1, nullptr);
		ctrlLeaves[i] = rtarg;
	}
};

// Initialises all our controls.
void CATWindowLimbPhasesScrollPane::InitControls()
{
	root = ctrlLimbPhasesBranch->GetCATRoot();
	HWND hStaticWnd, hEditWnd, hSliderWnd;
	ISliderControl *sld;

	WORD wBaseUnitsX = LOWORD(GetDialogBaseUnits());
	WORD wBaseUnitsY = HIWORD(GetDialogBaseUnits());

	int yOffset = 50;

	// Setup a slider, edit box, and label for each limb phase
	// branch.  Resize the pane first, to fit all the controls.
	nPaneSize = MulDiv(yOffset + 8 + ctrlLimbPhasesBranch->GetNumLeaves() * 16, wBaseUnitsY, 8);
	nPaneStepSize = MulDiv(8, wBaseUnitsY, 8);

	SetWindowPos(
		hDlg, HWND_TOP,
		0,
		0,
		nViewWidth,
		nPaneSize,
		SWP_NOMOVE);

	nNumLimbs = ctrlLimbPhasesBranch->GetNumLimbs();
	vecSliderPhases.resize(nNumLimbs);
	vecLabels.resize(nNumLimbs);

	for (int i = 0; i < nNumLimbs; i++) {
		vecLabels[i].printf(_T("%d. %s"), i + 1, ctrlLimbPhasesBranch->GetLimbName(i).data());

		hStaticWnd = CreateWindowEx(
			0, _T("STATIC"), vecLabels[i], SS_SIMPLE | WS_CHILD | WS_VISIBLE,
			MulDiv(4, wBaseUnitsX, 4),
			MulDiv(yOffset + 10 + i * 16, wBaseUnitsY, 8),
			MulDiv(36, wBaseUnitsX, 4),
			MulDiv(8, wBaseUnitsY, 8),
			hDlg, NULL, hInstance, NULL);

		HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
		SendMessage(hStaticWnd, WM_SETFONT, (WPARAM)hFont, 0);

		hEditWnd = CreateWindowEx(
			0, _T("CustEdit"), _T(""), WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			MulDiv(42, wBaseUnitsX, 4),
			MulDiv(yOffset + 9 + i * 16, wBaseUnitsY, 8),
			MulDiv(20, wBaseUnitsX, 4),
			MulDiv(8, wBaseUnitsY, 8),
			hDlg, NULL, hInstance, NULL);

		hSliderWnd = CreateWindowEx(
			0, _T("SliderControl"), _T(""), WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			MulDiv(64, wBaseUnitsX, 4),
			MulDiv(yOffset + 7 + i * 16, wBaseUnitsY, 8),
			MulDiv(100, wBaseUnitsX, 4),
			MulDiv(12, wBaseUnitsY, 8),
			hDlg, NULL, hInstance, NULL);

		// Store the ID of the branch in the slider's window long
		// pointer.  I think it may be possible to instead set
		// GWLP_ID, but I'm not sure if this causes conflicts with
		// other window IDs if we choose bad values.  Better to just
		// let Windows handle that...
		SetWindowLongPtr(hSliderWnd, GWLP_ID, (LONG)i);

		// Setup and store the custom slider control.
		sld = GetISlider(hSliderWnd);
		if (sld) {
			sld->SetLimits(-0.5f, 0.5f);
			sld->SetNumSegs(4);
			sld->LinkToEdit(hEditWnd, EDITTYPE_FLOAT);
		}
		vecSliderPhases[i] = sld;
	}

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	// FootStepMasks  // SA: not gobalized because this button is always invisible
	btnStepMask = GetICustButton(GetDlgItem(hDlg, IDC_BTN_STEPMASKS));
	btnStepMask->SetType(CBT_CHECK);
	btnStepMask->SetButtonDownNotify(TRUE);
	btnStepMask->SetText(catCfg.Get(_T("ButtonTexts"), _T("btnStepMask"), _T("Step Mask")));
	btnStepMask->SetTooltip(TRUE, _T("Turn on and off foot step masks. Foot step masks detect when a character has stopped moving and make the character stationary"));
	btnStepMask->SetCheck(root->GetisStepMasks());

	SET_CHECKED(hDlg, IDC_RDO_ALL, TRUE);

	// CreateFootPrints
	btnCreateFootPrints = GetICustButton(GetDlgItem(hDlg, IDC_BUTTON_FP_CREATE));
	btnCreateFootPrints->SetType(CBT_PUSH);
	btnCreateFootPrints->SetButtonDownNotify(TRUE);
	btnCreateFootPrints->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCreateFootPrints"), GetString(IDS_TT_CREATEFPRINTOBJS)));

	// Delete
	btnDeleteFootPrints = GetICustButton(GetDlgItem(hDlg, IDC_BUTTON_FP_REMOVE));
	btnDeleteFootPrints->SetType(CBT_PUSH);
	btnDeleteFootPrints->SetButtonDownNotify(TRUE);
	btnDeleteFootPrints->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnDeleteFootPrints"), GetString(IDS_TT_REMCMFPRINTS)));

	// Reset
	btnResetFootprints = GetICustButton(GetDlgItem(hDlg, IDC_BUTTON_FP_RESET));
	btnResetFootprints->SetType(CBT_PUSH);
	btnResetFootprints->SetButtonDownNotify(TRUE);
	btnResetFootprints->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnResetFootprints"), GetString(IDS_TT_RESETFPRINTS)));

	// PickGround
	btnPickGround = GetICustButton(GetDlgItem(hDlg, IDC_BUTTON_FP_SNAPTOGROUND));
	btnPickGround->SetType(CBT_CHECK);
	btnPickGround->SetButtonDownNotify(TRUE);
	btnPickGround->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPickGround"), GetString(IDS_BTN_PICKGROUNDGEOM)));
	btnPickGround->SetHighlightColor(GREEN_WASH);

	INode* node = root->GetGroundNode();
	if (node) btnPickGround->SetText(node->GetName());

	GetCOREInterface()->RegisterTimeChangeCallback(this);
	TimeChanged(GetCOREInterface()->GetTime());
	root->SetCurWindowPtr(this);
}

// Releases all our custom Max controls
void CATWindowLimbPhasesScrollPane::ReleaseControls()
{
	//	ST - Bug fix, this window is destructed AFTER the next CATWindow is opened,
	//	meaning that setting this to NULL will not delete this windows pointer, but
	//	the current one. Check to see that the pointer still points at us (this
	//	will happen when the window is destructed) before setting it to NULL
	if (*(root->GetCurWindowPtrAddr()) == this)
		root->SetCurWindowPtr(NULL);

	GetCOREInterface()->UnRegisterTimeChangeCallback(this);

	for (int i = 0; i < nNumLimbs; i++) {
		SAFE_RELEASE_SLIDER(vecSliderPhases[i]);
	}
	SAFE_RELEASE_BTN(btnStepMask);
	SAFE_RELEASE_BTN(btnCreateFootPrints);
	SAFE_RELEASE_BTN(btnDeleteFootPrints);
	SAFE_RELEASE_BTN(btnResetFootprints);
	SAFE_RELEASE_BTN(btnPickGround);
	DeleteAllRefsFromMe();
}

INT_PTR CALLBACK CATWindowLimbPhasesScrollPane::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TimeValue t = GetCOREInterface()->GetTime();
	ISliderControl *sld;
	int nSliderID;

	switch (uMsg) {
	case CC_SLIDER_CHANGE:
		// Determine which slider changed.  We have stored its
		// index in GWLP_ID cos if we store it in GWLP_USERDATA
		// and then attach it to a SliderControl, we get big
		// bad crashing...
		sld = (ISliderControl*)lParam;
		nSliderID = (int)GetWindowLongPtr(sld->GetHwnd(), GWLP_ID);

		if (nSliderID >= 0 && nSliderID < nNumLimbs) {
			Control* ctrlPhase = ctrlLimbPhasesBranch->GetLeaf(nSliderID);
			ReplaceReference(nSliderID, ctrlPhase);
			if (ctrlPhase) {
				SliderToControl(sld, ctrlPhase, t, this);
				if (!GetCOREInterface()->IsAnimPlaying())
					GetCOREInterface()->RedrawViews(t);
			}
		}
		return TRUE;
	case CC_SLIDER_BUTTONDOWN:
		sld = (ISliderControl*)lParam;
		nSliderID = (int)GetWindowLongPtr(sld->GetHwnd(), GWLP_ID);
		if (nSliderID >= 0 && nSliderID < nNumLimbs) {
			if (!theHold.Holding()) {
				theHold.Begin();
			}
		}
		break;
	case CC_SLIDER_BUTTONUP:	SliderButtonUp(HIWORD(wParam), GetString(IDS_HLD_CADENCE));				break;
	case WM_CUSTEDIT_ENTER:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_CADENCE));		break;
	case WM_COMMAND:
		switch (HIWORD(wParam)) { // Notification codes
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_CHK_STEPMASK:	root->SetisStepMasks(IS_CHECKED(hWnd, IDC_CHK_STEPMASK));	break;
			case IDC_RDO_ALL:		bOnlySelected = IS_CHECKED(hWnd, IDC_RDO_ALL);				break;
			case IDC_RDO_SELECTED:	bOnlySelected = IS_CHECKED(hWnd, IDC_RDO_SELECTED);			break;
			}
			break;
		case BN_BUTTONUP:
			switch (LOWORD(wParam)) {
				//	case IDC_BTN_STEPMASKS:
				//		root->SetisStepMasks(btnStepMask->IsChecked());
				//		GetCOREInterface()->RedrawViews(t);
				//		break;
			case IDC_BUTTON_FP_CREATE:	root->ICreateFootPrints();										break;
			case IDC_BUTTON_FP_REMOVE:	root->IRemoveFootPrints(IS_CHECKED(hDlg, IDC_RDO_SELECTED));	break;
			case IDC_BUTTON_FP_RESET:	root->IResetFootPrints(IS_CHECKED(hDlg, IDC_RDO_SELECTED));		break;
			case IDC_BUTTON_FP_SNAPTOGROUND:
				if (btnPickGround->IsChecked())
				{
					TCHAR *msg = GetString(IDS_PICK_FT_ALIGN);
					pick_mode.Setup(root, msg, btnPickGround, PICK_GROUND);
					GetCOREInterface()->ClearPickMode();
					GetCOREInterface()->SetPickMode(&pick_mode);
				}
				break;
			}
			break;
		}
		break;
	}
	return FALSE;
}

void CATWindowLimbPhasesScrollPane::TimeChanged(TimeValue t)
{
	for (int i = 0; i < ctrlLimbPhasesBranch->GetNumLeaves(); i++) {
		Control* ctrlPhase = ctrlLimbPhasesBranch->GetLeaf(i);
		ControlToSlider(vecSliderPhases[i], ctrlPhase, t);
	}

	// If we don't have a path node then disable all these buttons
//	btnStepMask->Enable(root->GetPathNode()!=NULL);
//	EnableWindow(GetDlgItem(hDlg, IDC_BTN_STEPMASKS), root->GetPathNode()!=NULL);
//	SET_CHECKED(hDlg, IDC_BTN_STEPMASKS, root->GetisStepMasks());
	EnableWindow(GetDlgItem(hDlg, IDC_CHK_STEPMASK), root->GetPathNode() != NULL);
	SET_CHECKED(hDlg, IDC_CHK_STEPMASK, root->GetisStepMasks());

	EnableWindow(GetDlgItem(hDlg, IDC_RDO_ALL), root->GetPathNode() != NULL);
	EnableWindow(GetDlgItem(hDlg, IDC_RDO_SELECTED), root->GetPathNode() != NULL);

	btnCreateFootPrints->Enable(root->GetPathNode() != NULL);
	btnDeleteFootPrints->Enable(root->GetPathNode() != NULL);
	btnResetFootprints->Enable(root->GetPathNode() != NULL);
	btnPickGround->Enable(root->GetPathNode() != NULL);
}

////////////////////////////////////////////////////////////////////////////////
// LimbPhases: A Dialog for managing Phase Offsets
////////////////////////////////////////////////////////////////////////////////
class CATWindowLimbPhases : public CATWindowPane
{
private:
	CATHierarchyBranch2* ctrlLimbPhasesBranch;
	CATWindowLimbPhasesScrollPane *pScrollPane;

	HWND hScrollBar;

	SCROLLINFO siScrollInfo;

	WORD GetDialogTemplateID() const { return IDD_CATWINDOW_PHASES; }
	void InitControls();
	void ReleaseControls();

public:

	CATWindowLimbPhases(CATHierarchyBranch2* ctrlPhases)
		: CATWindowPane()
	{
		SecureZeroMemory(&siScrollInfo, sizeof(SCROLLINFO));
		ctrlLimbPhasesBranch = ctrlPhases;
		root = ctrlPhases->GetCATRoot();

		pScrollPane = NULL;
		hScrollBar = NULL;
	}

	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void TimeChanged(TimeValue) {};
};

// Initialises all our controls.
void CATWindowLimbPhases::InitControls()
{
	// Setup scroll bar and scroll pane.  The pane is the length
	// of the entire page containing all our slider controls etc.
	// The scroll bar remains parented to the dialog window and
	// is set top-most in the Z-order.  Messages to the scroll bar
	// cause the scroll pane to be repositioned accordingly.
	RECT rcClient;
	GetClientRect(hDlg, &rcClient);
	int nPaneWidth = rcClient.right - rcClient.left;
	int nPaneHeight = rcClient.bottom - rcClient.top;

	hScrollBar = hDlg;

	pScrollPane = new CATWindowLimbPhasesScrollPane(ctrlLimbPhasesBranch);
	pScrollPane->SetViewDimensions(nPaneWidth, nPaneHeight);
	pScrollPane->Create(hInstance, hDlg);

	siScrollInfo.cbSize = sizeof SCROLLINFO;
	siScrollInfo.fMask = SIF_ALL;
	siScrollInfo.nPage = nPaneHeight;
	siScrollInfo.nPos = 0;
	siScrollInfo.nMin = 0;
	siScrollInfo.nMax = pScrollPane->nPaneSize > nPaneHeight ? pScrollPane->nPaneSize : nPaneHeight;
	siScrollInfo.nTrackPos = 0;

	SetScrollInfo(hScrollBar, SB_VERT, &siScrollInfo, TRUE);
}

void CATWindowLimbPhases::ReleaseControls()
{
	// pScrollPane is automatically released when the parent dialog is released (us)
}

INT_PTR CALLBACK CATWindowLimbPhases::DialogProc(HWND, UINT uMsg, WPARAM wParam, LPARAM)
{
	switch (uMsg) {
	case WM_VSCROLL:
	{
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		GetScrollInfo(hScrollBar, SB_VERT, &si);
		int nPos = si.nPos;

		switch (LOWORD(wParam)) {
		case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
		case SB_TOP:		si.nPos = si.nMin; break;
		case SB_BOTTOM:		si.nPos = si.nMax; break;
		case SB_LINEDOWN:	si.nPos += pScrollPane->nPaneStepSize; break;
		case SB_LINEUP:		si.nPos -= pScrollPane->nPaneStepSize; break;
		case SB_PAGEDOWN:	si.nPos += si.nPage; break;
		case SB_PAGEUP:		si.nPos -= si.nPage; break;
		}
		si.fMask = SIF_POS;
		SetScrollInfo(hScrollBar, SB_VERT, &si, TRUE);
		GetScrollInfo(hScrollBar, SB_VERT, &si);

		if (nPos != si.nPos) {
			SetWindowPos(pScrollPane->GetDlg(), NULL, 0, -si.nPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
		return TRUE;
	}
	}
	return FALSE;
}

#define IMAGE_W 30
#define IMAGE_H 30

// ST 26/01/12 Copy/Paste for CATMotion graphs. This is a VERY simple clipboard, that
// stores every variable from a graph and its id. On a graph with the same id, loading
// the values in the array back in the same order will set the controller to the same
// state as the 'copied' graph.

typedef struct CATClipboard
{
	float dArrVal[40];
	Class_ID currClassID;
} Clipboard;

static Clipboard clipboard;

////////////////////////////////////////////////////////////////////////////////
// Graph Pane: Displays a graph and a bunch of controls.  The controls are
// hooked up to the selected CATHierarchyBranch
////////////////////////////////////////////////////////////////////////////////
class CATWindowGraph : public CATWindowPane //, public TimeChangeCallback
{
	HWND hLimbs;

	bool bRigAndUiDirty;
	HWND hGraphFrame;
	RECT rcGraphRect;
	HDC hGraphMemDC;
	HBITMAP hGraphBitmap;
	HGDIOBJ hGraphMemDCOldSel;

	BOOL bDragingKey;
	BOOL bkey, bInTan, bOutTan;

	// Graph Y scaling factors are calculated from these
	float minY, maxY;

	// the branch we are currently working on
	CATHierarchyBranch2* ctrlBranch;

	UIType iUIType;
	int iNumKeys;

	// for some reason that everybody has forgotten, keynum is a 1 base index to the keys.
	// The 1st key is iKeyNum=1;
	int iKeyNum;

	// We are either a WeightShift Graphs or a Monograph
	// These graphs have only one Value for the size of the occilations
	BOOL SetValueAllKeys;

	float dKeyTimeOffset;
	float fPrevKeyTime, fNextKeyTime;
	float minVal, maxVal;

	ISpinnerControl *spnScale, *spnOffsetX, *spnOffsetY;
	ISpinnerControl *spnTime, *spnValue, *spnTangent, *spnInTanLen, *spnOutTanLen;
	Control			*ctrlTime, *ctrlValue, *ctrlTangent, *ctrlInTanLen, *ctrlOutTanLen;

	ISliderControl	*slider;
	Control			*ctrlSlider;
	std::vector<RefTargetHandle> ctrlLeaves;

	ISpinnerControl *spnX, *spnY, *spnZ;
	Control			*ctrlX, *ctrlY, *ctrlZ;

	ICustButton *btnNextKey;
	ICustButton *btnPrevKey;
	ICustButton *btnWeld;
	ICustButton *btnLock;

	ICustButton *btnTrackView;
	ICustButton *btnFrameGraph;

	ICustButton *btnCopy;
	ICustButton *btnPaste;

	HWND hPrevMouseCaptureWnd;

	WORD GetDialogTemplateID() const { return IDD_CATWINDOW_GRAPH; }
	void InitControls();//CATHierarchyBranch* root);
	void ReleaseControls();

	void RefreshLimbsListbox();

	// Fetches the new controllers and sets values on the spinners
	void GetGraphKey();

	void SetModeGraph(int ShowGraph);
	void SetModeP3(int ShowP3);

	void TimeChanged(TimeValue t);

	enum CONTROLLIST {
		TIME,
		VALUE,
		TANGENT,
		IN_TAN_LEN,
		OUT_TAN_LEN,
		CTRL_X,
		CTRL_Y,
		CTRL_Z,
		BRANCH,
		NUM_CONTROLS,
		SLIDER = NUM_CONTROLS
	};

public:

	CATWindowGraph() : CATWindowPane()
		, iUIType(UI_BLANK)
		, bInTan(0)
		, hLimbs(NULL)
		, maxVal(0.0f)
		, minVal(0.0f)
		, ctrlBranch(NULL)
		, bkey(0)
		, maxY(0.0f)
		, minY(0.0f)
		, iNumKeys(0)
		, fNextKeyTime(0.0f)
		, dKeyTimeOffset(0.0f)
		, bOutTan(0)
		, fPrevKeyTime(0.0f)
	{

		iKeyNum = 0;
		SetValueAllKeys = FALSE;

		bRigAndUiDirty = false;
		hGraphFrame = NULL;
		SetRectEmpty(&rcGraphRect);
		hGraphMemDC = NULL;
		hGraphBitmap = NULL;
		hGraphMemDCOldSel = NULL;

		spnScale = NULL;
		spnOffsetX = NULL;
		spnOffsetY = NULL;

		spnTime = NULL;
		spnValue = NULL;
		spnTangent = NULL;
		spnInTanLen = NULL;
		spnOutTanLen = NULL;

		ctrlTime = NULL;
		ctrlValue = NULL;
		ctrlTangent = NULL;
		ctrlInTanLen = NULL;
		ctrlOutTanLen = NULL;

		slider = NULL;
		ctrlSlider = NULL;

		btnNextKey = NULL;
		btnPrevKey = NULL;
		btnWeld = NULL;
		btnLock = NULL;

		btnTrackView = NULL;
		btnFrameGraph = NULL;

		btnCopy = NULL;
		btnPaste = NULL;

		spnX = NULL;
		spnY = NULL;
		spnZ = NULL;

		ctrlX = NULL;
		ctrlY = NULL;
		ctrlZ = NULL;

		bDragingKey = FALSE;
		hPrevMouseCaptureWnd = FALSE;
	}

	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void ConnectSpinnersWithControllers(CATHierarchyBranch* newBranch);

	float& GetMinY() { return minY; }
	float& GetMaxY() { return maxY; }

	// GB 12-Jul-03: All graph drawing must be done via this function.
	// It calls the draw function on the hierarchy root, passing in the
	// DC.  If the hierarchy ever wants to initiate a graph draw, it
	// must call a user message on this window.  We cannot store the DC
	// in the hierarchy because it cannot be used by two threads.  Can
	// pass in the hWnd though.
	//
	void DrawGraph(CATHierarchyBranch* ctrlActiveBranch, int nSelectedKey, BOOL bInvalidate = TRUE)
	{
		root->DrawGraph(ctrlActiveBranch, nSelectedKey, hGraphMemDC, rcGraphRect);

		if (bInvalidate) InvalidateRect(hDlg, &rcGraphRect, FALSE);
		else
		{
			// Draw graph without invalidating the region,
			// create a DC, paint it, and validate (in case
			// there are paint messages further down the
			// queue).
			HDC hdc = GetDC(hDlg);
			PaintGraphNow(hdc, &rcGraphRect);
			ReleaseDC(hDlg, hdc);
			ValidateRect(hDlg, &rcGraphRect);
		}
	}

	void PaintGraphNow(HDC hdc, LPRECT lpRect);

	void CopyGraph();
	void PasteGraph();

	virtual int NumRefs()
	{
		return NUM_CONTROLS + (int)ctrlLeaves.size();
	}

	virtual RefTargetHandle GetReference(int i)
	{
		switch (i)
		{
		case TIME: return ctrlTime;
		case VALUE: return ctrlValue;
		case TANGENT: return ctrlTangent;
		case IN_TAN_LEN: return ctrlInTanLen;
		case OUT_TAN_LEN: return ctrlOutTanLen;
		case CTRL_X: return ctrlX;
		case CTRL_Y: return ctrlY;
		case CTRL_Z: return ctrlZ;
		case BRANCH: return ctrlBranch;
		default:
		{
			int leafIdx = i - SLIDER;
			if ((0 <= leafIdx) && (leafIdx < ctrlLeaves.size()))
			{
				return ctrlLeaves[leafIdx];
			}
			return nullptr;
		}
		}
	}

protected:

	virtual RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL)
	{
		bRigAndUiDirty = true;
		InvalidateRect(hGraphFrame, NULL, FALSE);
		return REF_SUCCEED;
	}

	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		switch (i)
		{
		case TIME: ctrlTime = dynamic_cast<Control*>(rtarg); break;
		case VALUE: ctrlValue = dynamic_cast<Control*>(rtarg); break;
		case TANGENT: ctrlTangent = dynamic_cast<Control*>(rtarg); break;
		case IN_TAN_LEN: ctrlInTanLen = dynamic_cast<Control*>(rtarg); break;
		case OUT_TAN_LEN: ctrlOutTanLen = dynamic_cast<Control*>(rtarg); break;
		case CTRL_X: ctrlX = dynamic_cast<Control*>(rtarg); break;
		case CTRL_Y: ctrlY = dynamic_cast<Control*>(rtarg); break;
		case CTRL_Z: ctrlZ = dynamic_cast<Control*>(rtarg); break;
		case BRANCH: ctrlBranch = dynamic_cast<CATHierarchyBranch2*>(rtarg); break;
		default:
		{
			if (i >= SLIDER)
			{
				int leafIdx = i - SLIDER;

				DbgAssert(leafIdx < ctrlLeaves.size());
				if (leafIdx >= ctrlLeaves.size())
					ctrlLeaves.resize(leafIdx + 1, nullptr);

				ctrlLeaves[leafIdx] = rtarg;
			}
		}
		}
	}
};

// Initialises all our controls.
void CATWindowGraph::InitControls()//CATHierarchyBranch* root)
{
	// Get the graph rectangle.  We need to hide the graph
	// frame after doing this.  It's only a guide.  If we
	// don't hide it, it'll cover our graph blits.
	hGraphFrame = GetDlgItem(hDlg, IDC_GRAPH_FRAME);

	GetClientRect(hGraphFrame, &rcGraphRect);

	root->SetGraphRect(rcGraphRect);
	root->SetCurWindowPtr(this);

	// Create a memory DC for the graph and attach to device-
	// compatible bitmap.
	HDC hdc = GetDC(hDlg);
	hGraphMemDC = CreateCompatibleDC(hdc);
	hGraphBitmap = CreateCompatibleBitmap(hdc,
		rcGraphRect.right - rcGraphRect.left + 1,
		rcGraphRect.bottom - rcGraphRect.top + 1);
	hGraphMemDCOldSel = SelectObject(hGraphMemDC, hGraphBitmap);
	ReleaseDC(hDlg, hdc);

	// We must start with no Hierarchy Branch.
	// One will be given to us soon
	ctrlBranch = NULL;

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	hLimbs = GetDlgItem(hDlg, IDC_LIST_LIMBS);

	//////////////////////////////////////////////////////////////////////////
	// Scale and offset spinners
	spnScale = SetupFloatSpinner(hDlg, IDC_SPIN_SCALE2, IDC_EDIT_SCALE2, -500.0f, 500.0f, 100.0f);
	spnOffsetX = SetupFloatSpinner(hDlg, IDC_SPIN_OFFSET_X, IDC_EDIT_OFFSET_X, -500.0f, 500.0f, 0.0f);
	spnOffsetY = SetupFloatSpinner(hDlg, IDC_SPIN_OFFSET_Y, IDC_EDIT_OFFSET_Y, -500.0f, 500.0f, 0.0f);

	// Initialise custom Max controls
	spnTime = SetupFloatSpinner(hDlg, IDC_SPIN_TIME, IDC_EDIT_TIME, -100.0f, 100.0f, 50.0f);
	spnValue = SetupFloatSpinner(hDlg, IDC_SPIN_VALUE, IDC_EDIT_VALUE, -500.0f, 500.0f, 50.0f);

	spnTangent = SetupFloatSpinner(hDlg, IDC_SPIN_TANGENT, IDC_EDIT_TANGENT, -500.0f, 500.0f, 50.0f);
	spnTangent->SetResetValue(0.0f);
	spnTangent->SetScale(0.01f);

	spnInTanLen = SetupFloatSpinner(hDlg, IDC_SPIN_INTANLEN, IDC_EDIT_INTANLEN, 0.0f, 1.0f, 0.333f);
	spnInTanLen->SetResetValue(0.333f);
	spnInTanLen->SetScale(0.01f);

	spnOutTanLen = SetupFloatSpinner(hDlg, IDC_SPIN_OUTTANLEN, IDC_EDIT_OUTTANLEN, 0.0f, 1.0f, 0.333f);
	spnOutTanLen->SetResetValue(0.333f);
	spnOutTanLen->SetScale(0.01f);

	slider = SetupFloatSlider(hDlg, IDC_SLIDER, IDC_SLIDEREDIT, 0.0f, 1.0f, 0.0f, 4);
	slider->SetResetValue(0.0f);

	// the Point3 Controls
	spnX = SetupFloatSpinner(hDlg, IDC_SPIN_X, IDC_EDIT_X, -500.0f, 500.0f, 50.0f);
	spnX->SetResetValue(0.0f);

	spnY = SetupFloatSpinner(hDlg, IDC_SPIN_Y, IDC_EDIT_Y, -500.0f, 500.0f, 50.0f);
	spnY->SetResetValue(0.0f);

	spnZ = SetupFloatSpinner(hDlg, IDC_SPIN_Z, IDC_EDIT_Z, -500.0f, 500.0f, 50.0f);
	spnZ->SetResetValue(0.0f);

	// Weld
	btnWeld = GetICustButton(GetDlgItem(hDlg, IDC_BTN_WELD));
	btnWeld->SetType(CBT_CHECK);
	btnWeld->SetButtonDownNotify(TRUE);
	// Set tooltip to use text based on program state
	btnWeld->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnWeld"), GetString(IDS_TT_INDEPLEGSETS)));
	btnWeld->SetImage(hIcons, 17, 18, 17 + 25, 18 + 25, 24, 24);

	// Lock
	btnLock = GetICustButton(GetDlgItem(hDlg, IDC_BTN_LOCK));
	btnLock->SetType(CBT_CHECK);
	btnLock->SetButtonDownNotify(TRUE);
	// Set tooltip to use text based on program state
	btnLock->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnLock"), GetString(IDS_TT_UNLOCKLEGS)));
	btnLock->SetImage(hIcons, 19, 19, 19 + 25, 19 + 25, 24, 24);

	// NextKey
	btnNextKey = GetICustButton(GetDlgItem(hDlg, IDC_BTN_KEYNEXT));
	btnNextKey->SetType(CBT_PUSH);
	btnNextKey->SetButtonDownNotify(TRUE);
	btnNextKey->SetImage(hIcons, 21, 21, 21 + 25, 21 + 25, 24, 24);

	// PrevKey
	btnPrevKey = GetICustButton(GetDlgItem(hDlg, IDC_BTN_KEYPREV));
	btnPrevKey->SetType(CBT_PUSH);
	btnPrevKey->SetButtonDownNotify(TRUE);
	btnPrevKey->SetImage(hIcons, 20, 20, 20 + 25, 20 + 25, 24, 24);

	// TrackView button
	btnTrackView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_TRACKVIEW));
	btnTrackView->SetType(CBT_PUSH);
	btnTrackView->SetButtonDownNotify(TRUE);
	btnTrackView->SetImage(hIcons, 13, 13, 13 + 25, 13 + 25, 24, 24);

	// FrameGraph button
	btnFrameGraph = GetICustButton(GetDlgItem(hDlg, IDC_BTN_FRAMEGRAPH));
	btnFrameGraph->SetType(CBT_PUSH);
	btnFrameGraph->SetButtonDownNotify(TRUE);
	btnFrameGraph->SetImage(hIcons, 23, 23, 23 + 25, 23 + 25, 24, 24);

	// Copy button
	btnCopy = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COPY));
	btnCopy->SetType(CBT_PUSH);
	btnCopy->SetButtonDownNotify(TRUE);
	btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

	// Paste button
	btnPaste = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE));
	btnPaste->SetType(CBT_PUSH);
	btnPaste->SetButtonDownNotify(TRUE);
	btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

	GetCOREInterface()->RegisterTimeChangeCallback(this);

	dKeyTimeOffset = 0.0f;
}

// Releases all our custom Max controls
void CATWindowGraph::ReleaseControls()
{
	//	ST - Bug fix, this window is destructed AFTER the next CATWindow is opened,
	//	meaning that setting this to NULL will not delete this windows pointer, but
	//	the current one. Check to see that the pointer still points at us (this
	//	will happen when the window is destructed) before setting it to NULL
	if (*(root->GetCurWindowPtrAddr()) == this)
		root->SetCurWindowPtr(NULL);

	if (GetCapture() == hDlg) ReleaseCapture();

	// Clean up graphing stuff.
	SelectObject(hGraphMemDC, hGraphMemDCOldSel);
	DeleteObject(hGraphBitmap);
	DeleteDC(hGraphMemDC);

	SAFE_RELEASE_SPIN(spnScale);
	SAFE_RELEASE_SPIN(spnOffsetX);
	SAFE_RELEASE_SPIN(spnOffsetY);

	SAFE_RELEASE_SPIN(spnTime);
	SAFE_RELEASE_SPIN(spnValue);
	SAFE_RELEASE_SPIN(spnTangent);
	SAFE_RELEASE_SPIN(spnInTanLen);
	SAFE_RELEASE_SPIN(spnOutTanLen);

	// the Point3 Controls
	SAFE_RELEASE_SPIN(spnX);
	SAFE_RELEASE_SPIN(spnY);
	SAFE_RELEASE_SPIN(spnZ);

	SAFE_RELEASE_SLIDER(slider);

	SAFE_RELEASE_BTN(btnNextKey);
	SAFE_RELEASE_BTN(btnPrevKey);
	SAFE_RELEASE_BTN(btnLock);

	SAFE_RELEASE_BTN(btnTrackView);
	SAFE_RELEASE_BTN(btnFrameGraph);

	SAFE_RELEASE_BTN(btnCopy);
	SAFE_RELEASE_BTN(btnPaste);

	root = NULL;

	// ST - Dont forget to unregister anything
	// registered with GetCOREInterface()
	GetCOREInterface()->UnRegisterTimeChangeCallback(this);
	DeleteAllRefsFromMe();
}

void CATWindowGraph::TimeChanged(TimeValue t)
{
	ControlToSpinner(spnTime, ctrlTime, t);
	ControlToSpinner(spnValue, ctrlValue, t);
	ControlToSpinner(spnTangent, ctrlTangent, t);
	ControlToSpinner(spnInTanLen, ctrlInTanLen, t);
	ControlToSpinner(spnOutTanLen, ctrlOutTanLen, t);

	ControlToSlider(slider, ctrlSlider, t);

	ControlToSpinner(spnX, ctrlX, t);
	ControlToSpinner(spnY, ctrlY, t);
	ControlToSpinner(spnZ, ctrlZ, t);

	DrawGraph(ctrlBranch, iKeyNum, FALSE);
}

void CATWindowGraph::GetGraphKey()
{
	TimeValue t = GetCOREInterface()->GetTime();

	TSTR lblKeyNum;
	lblKeyNum.printf(_T("%d"), iKeyNum);
	SetWindowText(GetDlgItem(hDlg, IDC_STATIC_KEYNUM), lblKeyNum);

	float fTimeVal = 50.0f;
	fPrevKeyTime = 0.0f;	fNextKeyTime = 100.0f;
	minVal = -1000.0f;		maxVal = 1000.0f;
	float fValueVal = 0.0f, fTangentVal = 0.0f, fInTanLenVal = 0.333f, fOutTanLenVal = 0.333f;

	ctrlTime = NULL;
	ctrlValue = NULL;
	ctrlTangent = NULL;
	ctrlInTanLen = NULL;
	ctrlOutTanLen = NULL;

	Interval iv;

	if (ctrlBranch->GetNumLayers() > 0)
		ctrlBranch->GetGraphKey(iKeyNum,
			&ctrlTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
			&ctrlValue, fValueVal, minVal, maxVal,
			&ctrlTangent, fTangentVal,
			&ctrlInTanLen, fInTanLenVal,
			&ctrlOutTanLen, fOutTanLenVal,
			&ctrlSlider);

	//	CATGraphKey *key = ctrlBranch->GetCATGraphKey(iKeyNum);

		// Time
	//	if(key->ctrlTime){
	ReplaceReference(TIME, ctrlTime);
	if (ctrlTime) {
		iv = FOREVER;
		spnTime->SetLimits(fPrevKeyTime, fNextKeyTime, FALSE);
		ctrlTime->GetValue(t, (void*)&fTimeVal, iv);
		spnTime->SetValue(fTimeVal, FALSE);
		spnTime->Enable();
	}
	else {
		spnTime->SetLimits(fPrevKeyTime, fNextKeyTime, FALSE);
		spnTime->Disable();
		spnTime->SetValue(fTimeVal, FALSE);
	}
	// Value
//	if(key->ctrlValue){
	ReplaceReference(VALUE, ctrlValue);
	if (ctrlValue) {
		iv = FOREVER;
		spnValue->SetLimits(minVal, maxVal, FALSE);
		ctrlValue->GetValue(t, (void*)&fValueVal, iv);
		spnValue->SetValue(fValueVal, FALSE);
		spnValue->Enable();
	}
	else {
		spnValue->Disable();
		spnValue->SetValue(fValueVal, FALSE);
	}
	// Tangent
//	if(key->ctrlTangent){
	ReplaceReference(TANGENT, ctrlTangent);
	if (ctrlTangent) {
		iv = FOREVER;
		ctrlTangent->GetValue(t, (void*)&fTangentVal, iv);
		spnTangent->SetValue(fTangentVal, FALSE);
		spnTangent->Enable();
	}
	else {
		spnTangent->Disable();
		spnTangent->SetValue(fTangentVal, FALSE);
	}
	// InTanLen
//	if(key->ctrlInTanLen){
	ReplaceReference(IN_TAN_LEN, ctrlInTanLen);
	if (ctrlInTanLen) {
		iv = FOREVER;
		ctrlInTanLen->GetValue(t, (void*)&fInTanLenVal, iv);
		spnInTanLen->SetValue(fInTanLenVal, FALSE);
		spnInTanLen->Enable();
	}
	else {
		spnInTanLen->Disable();
		spnInTanLen->SetValue(fInTanLenVal, FALSE);
	}
	// OutTanLen
//	if(key->ctrlOutTanLen){
	ReplaceReference(OUT_TAN_LEN, ctrlOutTanLen);
	if (ctrlOutTanLen) {
		iv = FOREVER;
		ctrlOutTanLen->GetValue(t, (void*)&fOutTanLenVal, iv);
		spnOutTanLen->SetValue(fOutTanLenVal, FALSE);
		spnOutTanLen->Enable();
	}
	else {
		spnOutTanLen->Disable();
		spnOutTanLen->SetValue(fOutTanLenVal, FALSE);
	}

	for (int i = (int)ctrlLeaves.size() - 1; i >= 0; --i)
	{
		DeleteReference(SLIDER + i);
	}
	ctrlLeaves.clear();

	if (ctrlSlider) {
		iv = FOREVER;
		ShowWindow(slider->GetHwnd(), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_SLIDEREDIT), SW_SHOW);
		ctrlSlider->GetValue(t, (void*)&fValueVal, iv);

		slider->SetValue(fValueVal, FALSE);
		slider->Enable();

		// hide all the other controls
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TIME), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_TIME), SW_HIDE);
		ShowWindow(spnTime->GetHwnd(), SW_HIDE);

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_VALUE), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), SW_HIDE);
		ShowWindow(spnValue->GetHwnd(), SW_HIDE);

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TANGENT), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_TANGENT), SW_HIDE);
		ShowWindow(spnTangent->GetHwnd(), SW_HIDE);

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TANLENGTH), SW_HIDE);

		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_INTANLEN), SW_HIDE);
		ShowWindow(spnInTanLen->GetHwnd(), SW_HIDE);

		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_OUTTANLEN), SW_HIDE);
		ShowWindow(spnOutTanLen->GetHwnd(), SW_HIDE);

		CATHierarchyBranch2* sliderBranch = nullptr;
		if (ctrlSlider->ClassID() == CATHIERARCHYBRANCH_CLASS_ID)
			sliderBranch = (CATHierarchyBranch2*)ctrlSlider;
		DbgAssert(sliderBranch != nullptr);

		if (sliderBranch != nullptr)
		{
			for (int i = 0; i < sliderBranch->GetNumLeaves(); ++i)
				ReplaceReference(SLIDER + i, sliderBranch->GetLeaf(i));
		}
	}
	else {
		ShowWindow(slider->GetHwnd(), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_SLIDEREDIT), SW_HIDE);

		// show all the other controls
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TIME), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_TIME), SW_SHOW);
		ShowWindow(spnTime->GetHwnd(), SW_SHOW);

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_VALUE), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), SW_SHOW);
		ShowWindow(spnValue->GetHwnd(), SW_SHOW);

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TANGENT), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_TANGENT), SW_SHOW);
		ShowWindow(spnTangent->GetHwnd(), SW_SHOW);

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TANLENGTH), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_INTANLEN), SW_SHOW);
		ShowWindow(spnInTanLen->GetHwnd(), SW_SHOW);

		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_OUTTANLEN), SW_SHOW);
		ShowWindow(spnOutTanLen->GetHwnd(), SW_SHOW);
	}
}

void CATWindowGraph::SetModeGraph(int ShowGraph)
{
	//////////////////////////////////////////////////////////////////////////
	// New Scale and Offset controls
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_SCALE2), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_SCALE2), ShowGraph);
	ShowWindow(spnScale->GetHwnd(), ShowGraph);

	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OFFSET), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_OFFSET_X), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_OFFSET_Y), ShowGraph);
	ShowWindow(spnOffsetX->GetHwnd(), ShowGraph);
	ShowWindow(spnOffsetY->GetHwnd(), ShowGraph);
	//////////////////////////////////////////////////////////////////////////

	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TIME), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_TIME), ShowGraph);
	ShowWindow(spnTime->GetHwnd(), ShowGraph);

	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_VALUE), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), ShowGraph);
	ShowWindow(spnValue->GetHwnd(), ShowGraph);

	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TANGENT), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_TANGENT), ShowGraph);
	ShowWindow(spnTangent->GetHwnd(), ShowGraph);

	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TANLENGTH), ShowGraph);

	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_INTANLEN), ShowGraph);
	ShowWindow(spnInTanLen->GetHwnd(), ShowGraph);

	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_OUTTANLEN), ShowGraph);
	ShowWindow(spnOutTanLen->GetHwnd(), ShowGraph);

	ShowWindow(slider->GetHwnd(), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_SLIDEREDIT), ShowGraph);

	ShowWindow(btnNextKey->GetHwnd(), ShowGraph);
	ShowWindow(btnPrevKey->GetHwnd(), ShowGraph);
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_KEYNUM), ShowGraph);

	//	ShowWindow(btnLock->GetHwnd(), ShowGraph);
}

void CATWindowGraph::SetModeP3(int ShowP3)
{
	ShowWindow(spnX->GetHwnd(), ShowP3);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_X), ShowP3);
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_X), ShowP3);

	ShowWindow(spnY->GetHwnd(), ShowP3);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_Y), ShowP3);
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_Y), ShowP3);

	ShowWindow(spnZ->GetHwnd(), ShowP3);
	ShowWindow(GetDlgItem(hDlg, IDC_EDIT_Z), ShowP3);
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_Z), ShowP3);

	if (ShowP3 == SW_HIDE) return;

	/*	// not very tidy at all
		ShowWindow(sldPivotPosLift->GetHwnd(), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_PIVOTPOSLIFT), SW_HIDE);
		ShowWindow(sldPivotPosPlant->GetHwnd(), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_PIVOTPOSPLANT), SW_HIDE);
	*/
	TimeValue t = GetCOREInterface()->GetTime();
	ctrlBranch->GetP3Ctrls(&ctrlX, &ctrlY, &ctrlZ);

	Interval iv;
	float spnVal;
	// X
	ReplaceReference(CTRL_X, ctrlX);
	if (ctrlX) {
		iv = FOREVER;
		ctrlX->GetValue(t, (void*)&spnVal, iv);
		spnX->SetValue(spnVal, FALSE);
		spnX->Enable();
	}
	else {
		spnX->Disable();
		spnX->SetValue(0.0f, FALSE);
	}
	// Y
	ReplaceReference(CTRL_Y, ctrlY);
	if (ctrlY) {
		iv = FOREVER;
		ctrlY->GetValue(t, (void*)&spnVal, iv);
		spnY->SetValue(spnVal, FALSE);
		spnY->Enable();
	}
	else {
		spnY->Disable();
		spnY->SetValue(0.0f, FALSE);
	}
	// Z
	ReplaceReference(CTRL_Z, ctrlZ);
	if (ctrlZ) {
		iv = FOREVER;
		ctrlZ->GetValue(t, (void*)&spnVal, iv);
		spnZ->SetValue(spnVal, FALSE);
		spnZ->Enable();
	}
	else {
		spnZ->Disable();
		spnZ->SetValue(0.0f, FALSE);
	}
}

void CATWindowGraph::RefreshLimbsListbox()
{
	SendMessage(hLimbs, LB_RESETCONTENT, 0, 0);
	int iLimbs = ctrlBranch->GetNumLimbs();
	if (iLimbs > 0)
	{
		btnWeld->Enable();
		btnLock->Enable();

		if (ctrlBranch->GetisWelded())
		{
			TSTR strLimbs = GetString(IDS_LIMBSWELDED);
			int result = (int)SendMessage(hLimbs, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)strLimbs.data());
			UNREFERENCED_PARAMETER(result);
			DbgAssert(result != LB_ERR);
			EnableWindow(hLimbs, FALSE);

			btnWeld->SetCheck(FALSE);
			btnLock->SetCheck(TRUE);
			btnLock->Disable();
		}
		else
		{
			EnableWindow(hLimbs, TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_STATIC_LIMBS), TRUE);

			btnWeld->SetCheck(TRUE);
			btnLock->Enable();

			// fill out the list box with all the limbs names
			for (int i = 0; i < iLimbs; i++) {
				TSTR strlLimbName = ctrlBranch->GetLimbName(i);
				int result = (int)SendMessage(hLimbs, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)strlLimbName.data());
				UNREFERENCED_PARAMETER(result);
				DbgAssert(result != LB_ERR);
			}

			if (ctrlBranch->GetActiveLeaf() == -1) {
				btnLock->SetCheck(TRUE);
				EnableWindow(hLimbs, FALSE);
			}
			else {
				btnLock->SetCheck(FALSE);
				EnableWindow(hLimbs, TRUE);
				SendMessage(hLimbs, LB_SETCURSEL, ctrlBranch->GetActiveLeaf(), 0);
			}
		}
	}
	else
	{
		TSTR strLimbs = GetString(IDS_NOT_APPLICABLE);
		int result = (int)SendMessage(hLimbs, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)strLimbs.data());
		UNREFERENCED_PARAMETER(result);
		DbgAssert(result != LB_ERR);
		EnableWindow(hLimbs, FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_STATIC_LIMBS), FALSE);

		btnWeld->Disable();
		btnLock->Disable();
		//	EnableWindow(GetDlgItem(hDlg, IDC_BTN_LOCK), FALSE);
		//	EnableWindow(GetDlgItem(hDlg, IDC_BTN_WELD), FALSE);
	}
}

void CATWindowGraph::ConnectSpinnersWithControllers(CATHierarchyBranch* newBranch)
{
	// the branch we will in interactively modifying
	ReplaceReference(BRANCH, newBranch);
	iNumKeys = 0;
	RefreshLimbsListbox();

	switch (ctrlBranch->GetUIType()) {
	case UI_LIMBLESSGRAPH: {
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_LIMBS), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_WELD), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_LOCK), SW_HIDE);
		SetModeP3(SW_HIDE);
		SetModeGraph(SW_SHOW);

		iNumKeys = ctrlBranch->GetNumGraphKeys();

		iKeyNum = 1;
		if (iNumKeys > 1)
		{
			btnNextKey->Enable(TRUE);
			btnPrevKey->Enable(TRUE);
		}

		GetGraphKey();
		break;
	}
	case UI_GRAPH:
		SetModeP3(SW_HIDE);
		SetModeGraph(SW_SHOW);

		iNumKeys = ctrlBranch->GetNumGraphKeys();

		iKeyNum = 1;
		if (iNumKeys > 1)
		{
			btnNextKey->Enable(TRUE);
			btnPrevKey->Enable(TRUE);
		}
		GetGraphKey();
		break;
	case UI_POINT3:
		SetModeGraph(SW_HIDE);
		SetModeP3(SW_SHOW);
		break;
	case UI_FLOAT:
		SetModeGraph(SW_HIDE);
		SetModeP3(SW_HIDE);
		btnNextKey->Disable();
		btnPrevKey->Disable();
		btnLock->Disable();

		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_VALUE), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), SW_SHOW);
		ShowWindow(spnValue->GetHwnd(), SW_SHOW);
		ctrlValue = ctrlBranch->GetLeaf(0);
		break;

		//		default:
	}

	if (ctrlBranch->GetControllerRef(0) && (ctrlBranch->GetControllerRef(0)->ClassID() == clipboard.currClassID))
		btnPaste->Enable();
	else btnPaste->Disable();
}

void CATWindowGraph::PaintGraphNow(HDC hdc, LPRECT lpRect)
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

void CATWindowGraph::CopyGraph()
{
	Control *copyGraph = ctrlBranch->GetControllerRef(0);
	if (!ctrlBranch || !copyGraph)
		return;

	clipboard.currClassID = copyGraph->ClassID();

	if (clipboard.currClassID == CATHIERARCHYBRANCH_CLASS_ID)
	{
		DbgAssert(0);
		return;
	}

	TimeValue t = GetCOREInterface()->GetTime();
	int iNumGraphSubs = ctrlBranch->GetNumBranches();
	int iTargetValue = max(ctrlBranch->GetActiveLeaf(), 0);

	Interval iv;
	if (iNumGraphSubs > 0)
	{
		for (int i = 0; i < iNumGraphSubs; i++)
		{
			Control *ctrlValue = ((CATHierarchyBranch2*)ctrlBranch->GetBranch(i))->GetLeaf(iTargetValue);
			DbgAssert(ctrlValue);
			iv = FOREVER;
			ctrlValue->GetValue(t, (void*)&clipboard.dArrVal[i], iv, CTRL_ABSOLUTE);
		}
	}
	else // its a p3 controller
	{
		iNumGraphSubs = ctrlBranch->GetNumLeaves();
		for (int i = 0; i < iNumGraphSubs; i++)
		{
			Control *ctrlValue = ctrlBranch->GetLeaf(i);
			DbgAssert(ctrlValue);
			iv = FOREVER;
			ctrlValue->GetValue(t, (void*)&clipboard.dArrVal[i], iv, CTRL_ABSOLUTE);
		}
	}
	// We have just copied the current graph. Now enable the paste, so we can scrub time and
	// happily paste.
	if (btnPaste) btnPaste->Enable(TRUE);
}

void CATWindowGraph::PasteGraph()
{
	if (!ctrlBranch || !(ctrlBranch->GetControllerRef(0)->ClassID() == clipboard.currClassID))
		return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	TimeValue t = GetCOREInterface()->GetTime();
	int iNumGraphSubs = ctrlBranch->GetNumBranches();
	int iTargetValue = max(ctrlBranch->GetActiveLeaf(), 0);

	if (iNumGraphSubs > 0)
	{
		for (int i = 0; i < iNumGraphSubs; i++)
		{
			Control *ctrlValue = ((CATHierarchyBranch2*)ctrlBranch->GetBranch(i))->GetLeaf(iTargetValue);
			DbgAssert(ctrlValue);
			ctrlValue->SetValue(t, (void*)&clipboard.dArrVal[i], 1, CTRL_ABSOLUTE);
		}
	}
	else // its a p3 controller
	{
		iNumGraphSubs = ctrlBranch->GetNumLeaves();
		for (int i = 0; i < iNumGraphSubs; i++)
		{
			Control *ctrlValue = ctrlBranch->GetLeaf(i);
			DbgAssert(ctrlValue);
			ctrlValue->SetValue(t, (void*)&clipboard.dArrVal[i], 1, CTRL_ABSOLUTE);
		}
	}

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_GRAPHPASTE));
	}

	// This TimeChanged seems to only be needed by the P3 graphs... if anyone cares?
	TimeChanged(t);
}

INT_PTR CALLBACK CATWindowGraph::DialogProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TimeValue t = GetCOREInterface()->GetTime();

	switch (uMsg) {
	case WM_NOTIFY:
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam)) { // Notification codes
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_BTN_WELD:
				if (btnWeld->IsChecked())
				{
					if (ctrlBranch->GetisWelded())
					{
						ctrlBranch->UnWeldLeaves();
						ctrlBranch->SetActiveLeaf(0);
						RefreshLimbsListbox();
					}
				}
				else
				{
					if (!ctrlBranch->GetisWelded())
					{
						ctrlBranch->WeldLeaves();
						ctrlBranch->SetActiveLeaf(-1);
						RefreshLimbsListbox();
					}
				}
				break;
			case IDC_BTN_LOCK:
				if (btnLock->IsChecked()) {
					EnableWindow(hLimbs, FALSE);
					ctrlBranch->SetActiveLeaf(-1);
					if (ctrlBranch->GetUIType() == UI_GRAPH) GetGraphKey();
				}
				else {
					EnableWindow(hLimbs, TRUE);
					ctrlBranch->SetActiveLeaf(0);
					SendMessage(hLimbs, LB_SETCURSEL, ctrlBranch->GetActiveLeaf(), 0);
					if (ctrlBranch->GetUIType() == UI_GRAPH) GetGraphKey();
				}
				break;
			case IDC_BTN_KEYNEXT:
				iKeyNum++;
				if (iKeyNum > iNumKeys) iKeyNum = 1;
				GetGraphKey();
				break;
			case IDC_BTN_KEYPREV:
				iKeyNum--;
				if (iKeyNum < 1) iKeyNum = iNumKeys;
				GetGraphKey();
				break;
			case IDC_BTN_COPY:
				CopyGraph();
				break;
			case IDC_BTN_PASTE:
				PasteGraph();
				break;
			case IDC_BTN_RESET:
				if (!theHold.Holding()) theHold.Begin();
				ctrlBranch->ResetToDefaults(t);
				theHold.Accept(GetString(IDS_HLD_GRAPHRESET));
				TimeChanged(t);
				GetCOREInterface()->RedrawViews(t);
				break;
			case IDC_BTN_FRAMEGRAPH: {
				root->GetYRange(minY, maxY);
				break;
			}

			case IDC_BTN_TRACKVIEW:
			{
				DbgAssert(ctrlBranch);

				root->pCATTreeView = GetTrackView(_T("CATMotion Tracks"));// globalize?
				if (root->pCATTreeView) {
					root->pCATTreeView->SetEditMode(MODE_EDITFCURVE);
					ITreeViewUI_ShowControllerWindow(root->pCATTreeView);

					ITrackViewNode *pTrackViewNode = CreateITrackViewNode(TRUE);

					root->pCATTreeView->SetRootTrack(pTrackViewNode);
					pTrackViewNode->AddController(ctrlBranch, ctrlBranch->GetBranchName(), CATPARENT_CLASS_ID);

					root->pCATTreeView->ExpandTracks();
					root->pCATTreeView->ZoomOn(ctrlBranch, 0);
					root->pCATTreeView->SelectTrack(ctrlBranch);
				}
			}
			break;
			}
			// after any button clicks we need to refresh the graph
			DrawGraph(ctrlBranch, iKeyNum);
			break;
		case LBN_SELCHANGE: {
			int item = (int)SendMessage(hLimbs, LB_GETCURSEL, 0, 0);
			ctrlBranch->SetActiveLeaf(item);          //ISetActive(item + 1);// make all indexes 1 based
			if (ctrlBranch->GetUIType() == UI_GRAPH) GetGraphKey();
			TimeChanged(t);
			break;
		}
		}
	case WM_PAINT: {
		if (bRigAndUiDirty)
		{
			UpdateCATRig();
			TimeChanged(GetCOREInterface()->GetTime());
			bRigAndUiDirty = false;
		}
		//
		// Blit our current graph bitmap to the screen if the update
		// region intersects with the graph rect.
		//
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);
		if (hdc) PaintGraphNow(hdc, &ps.rcPaint);
		EndPaint(hDlg, &ps);
		break;
	}
	case CC_SPINNER_BUTTONDOWN:
		if (!theHold.Holding()) {
			theHold.Begin();
		}
		break;
	case CC_SPINNER_CHANGE:
		if (theHold.Holding()) {
			DisableRefMsgs();
			theHold.Restore();
			EnableRefMsgs();
		}
		else {
			// This situation happens when a user types into the edit box and presses enter.
			theHold.Begin();
		}
		// Manage all bracketing of spinners and updating of controllers
		switch (LOWORD(wParam)) { // Switch on ID
		case IDC_SPIN_TIME:			SpinnerToControl(spnTime, ctrlTime, t, this);	break;
		case IDC_SPIN_VALUE:		SpinnerToControl(spnValue, ctrlValue, t, this);	break;
		case IDC_SPIN_TANGENT:		SpinnerToControl(spnTangent, ctrlTangent, t, this);	break;
		case IDC_SPIN_INTANLEN:		SpinnerToControl(spnInTanLen, ctrlInTanLen, t, this);	break;
		case IDC_SPIN_OUTTANLEN:	SpinnerToControl(spnOutTanLen, ctrlOutTanLen, t, this);	break;
		case IDC_SPIN_X:			SpinnerToControl(spnX, ctrlX, t, this);	break;
		case IDC_SPIN_Y:			SpinnerToControl(spnY, ctrlY, t, this);	break;
		case IDC_SPIN_Z:			SpinnerToControl(spnZ, ctrlZ, t, this);	break;
		case IDC_SPIN_SCALE2: {
			float scale = spnScale->GetFVal();
			if (ctrlBranch && ctrlBranch->GetNumLayers() > 0) {
				for (int i = 1; i <= ctrlBranch->GetNumGraphKeys(); i++) {
					float fTimeVal = 50.0f, fValueVal = 0.0f, fTangentVal = 0.0f, fInTanLenVal = 0.333f, fOutTanLenVal = 0.333f;
					Control			*ctrlTime, *ctrlValue, *ctrlTangent, *ctrlInTanLen, *ctrlOutTanLen;
					ctrlTime = ctrlValue = ctrlTangent = ctrlInTanLen = ctrlOutTanLen = NULL;
					ctrlBranch->GetGraphKey(i,
						&ctrlTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
						&ctrlValue, fValueVal, minVal, maxVal,
						&ctrlTangent, fTangentVal,
						&ctrlInTanLen, fInTanLenVal,
						&ctrlOutTanLen, fOutTanLenVal,
						&ctrlSlider);
					Interval iv;
					if (ctrlValue) {
						float val = 0.0;
						iv = FOREVER;
						ctrlValue->GetValue(t, (void*)&val, iv);
						val *= (scale / 100.0f);
						ctrlValue->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
						if (i == iKeyNum) {
							spnValue->SetKeyBrackets(ctrlValue->IsKeyAtTime(t, NULL));
						}
					}
					if (ctrlTangent) {
						float val = 0.0;
						iv = FOREVER;
						ctrlTangent->GetValue(t, (void*)&val, iv);
						val *= (scale / 100.0f);
						ctrlTangent->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
						if (i == iKeyNum) {
							spnTangent->SetKeyBrackets(ctrlTangent->IsKeyAtTime(t, NULL));
						}
					}
				}
				TimeChanged(t);
			}
			break;
		}
		case IDC_SPIN_OFFSET_X: {
			float offset = spnOffsetX->GetFVal();
			if (ctrlBranch && ctrlBranch->GetNumLayers() > 0) {
				for (int i = 1; i <= ctrlBranch->GetNumGraphKeys(); i++) {
					float fTimeVal = 50.0f, fValueVal = 0.0f, fTangentVal = 0.0f, fInTanLenVal = 0.333f, fOutTanLenVal = 0.333f;
					Control			*ctrlTime, *ctrlValue, *ctrlTangent, *ctrlInTanLen, *ctrlOutTanLen;
					ctrlTime = ctrlValue = ctrlTangent = ctrlInTanLen = ctrlOutTanLen = NULL;
					ctrlBranch->GetGraphKey(i,
						&ctrlTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
						&ctrlValue, fValueVal, minVal, maxVal,
						&ctrlTangent, fTangentVal,
						&ctrlInTanLen, fInTanLenVal,
						&ctrlOutTanLen, fOutTanLenVal,
						&ctrlSlider);
					if (ctrlTime) {
						float val = 0.0;
						Interval iv = FOREVER;
						ctrlTime->GetValue(t, (void*)&val, iv);
						val += offset;
						ctrlTime->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
						if (i == iKeyNum) {
							spnTime->SetKeyBrackets(ctrlTime->IsKeyAtTime(t, NULL));
						}
					}
				}
				TimeChanged(t);
			}
			break;
		}
		case IDC_SPIN_OFFSET_Y: {
			float offset = spnOffsetY->GetFVal();
			if (ctrlBranch && ctrlBranch->GetNumLayers() > 0) {
				for (int i = 1; i <= ctrlBranch->GetNumGraphKeys(); i++) {
					float fTimeVal = 50.0f, fValueVal = 0.0f, fTangentVal = 0.0f, fInTanLenVal = 0.333f, fOutTanLenVal = 0.333f;
					Control			*ctrlTime, *ctrlValue, *ctrlTangent, *ctrlInTanLen, *ctrlOutTanLen;
					ctrlTime = ctrlValue = ctrlTangent = ctrlInTanLen = ctrlOutTanLen = NULL;
					ctrlBranch->GetGraphKey(i,
						&ctrlTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
						&ctrlValue, fValueVal, minVal, maxVal,
						&ctrlTangent, fTangentVal,
						&ctrlInTanLen, fInTanLenVal,
						&ctrlOutTanLen, fOutTanLenVal,
						&ctrlSlider);
					if (ctrlValue) {
						float val = 0.0;
						Interval iv = FOREVER;
						ctrlValue->GetValue(t, (void*)&val, iv);
						val += offset;
						ctrlValue->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
						if (i == iKeyNum) {
							spnValue->SetKeyBrackets(ctrlValue->IsKeyAtTime(t, NULL));
						}
					}
				}
				TimeChanged(t);
			}
			break;
		}

		}
		if (!GetCOREInterface()->IsAnimPlaying())
			GetCOREInterface()->RedrawViews(t);
		DrawGraph(ctrlBranch, iKeyNum, FALSE);
		break;
	case CC_SPINNER_BUTTONUP:
		switch (LOWORD(wParam))
		{
		case IDC_SPIN_TIME:		SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_KEYTIME));		break;
		case IDC_SPIN_VALUE:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_KEYVAL));		break;
		case IDC_SPIN_TANGENT:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_KEYTAN));	break;
		case IDC_SPIN_INTANLEN:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_KEYTANLEN));	break;
		case IDC_SPIN_OUTTANLEN:SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_KEYTANLEN));	break;
		case IDC_SPIN_X:		SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_XVAL));		break;
		case IDC_SPIN_Y:		SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_YVAL));		break;
		case IDC_SPIN_Z:		SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_ZVAL));		break;
		case IDC_SPIN_SCALE2:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_GRAPHKEYSCL));	spnScale->SetValue(100.0f, FALSE);		break;
		case IDC_SPIN_OFFSET_X:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_GRAPHKEYOFFSET));	spnOffsetX->SetValue(0.0f, FALSE);		break;
		case IDC_SPIN_OFFSET_Y:	SpinnerButtonUp(HIWORD(wParam), GetString(IDS_HLD_GRAPHKEYOFFSET));	spnOffsetY->SetValue(0.0f, FALSE);		break;
		}
		break;
	case CC_SLIDER_BUTTONDOWN:
		if (!theHold.Holding()) {
			theHold.Begin();
		}
		break;
	case CC_SLIDER_CHANGE: {
		// GetThe spinner and its value
		ISliderControl *slider = (ISliderControl*)lParam;
		switch (LOWORD(wParam)) { // Switch on ID
		case IDC_SLIDER:			SliderToControl(slider, ctrlSlider, t, this);	break;
		}
		if (!GetCOREInterface()->IsAnimPlaying())
			GetCOREInterface()->RedrawViews(t);
		DrawGraph(ctrlBranch, iKeyNum, FALSE);
		break;
	}
	case CC_SLIDER_BUTTONUP:
		switch (LOWORD(wParam))
		{
		case IDC_SLIDER:		SliderButtonUp(HIWORD(wParam), _T("CATWindow Slider Changed"));			break;
		}
		break;
	case WM_CUSTEDIT_ENTER:
	{
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_TIME:			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_KEYTIME));			break;
		case IDC_EDIT_VALUE:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_KEYVAL));		break;
		case IDC_EDIT_TANGENT:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_KEYTAN));		break;
		case IDC_EDIT_INTANLEN:
		case IDC_EDIT_OUTTANLEN:	if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_KEYTANLEN));		break;
		case IDC_EDIT_X:			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_XVAL));			break;
		case IDC_EDIT_Y:			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_YVAL));			break;
		case IDC_EDIT_Z:			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_ZVAL));			break;
		case IDC_EDIT_SCALE2:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_GRAPHKEYSCL));		spnScale->SetValue(100.0f, FALSE);	break;
		case IDC_EDIT_OFFSET_X:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_GRAPHKEYOFFSET));	spnOffsetX->SetValue(0.0f, FALSE);	break;
		case IDC_EDIT_OFFSET_Y:		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_GRAPHKEYOFFSET));	spnOffsetY->SetValue(0.0f, FALSE);	break;
		default:	// Catch the slider?
			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_PARAM));
			break;
		}
		break;
	}
	case WM_LBUTTONDOWN: {
		float xPos = (float)GET_X_LPARAM(lParam);  // horizontal position of cursor
		float yPos = (float)GET_Y_LPARAM(lParam);  // vertical position of cursor
		if ((xPos > rcGraphRect.left) &&
			(xPos < rcGraphRect.right) &&
			(yPos > rcGraphRect.top) &&
			(yPos < rcGraphRect.bottom) &&
			(ctrlBranch->GetUIType() == UI_GRAPH))
		{
			float clickRangeX = 12.0f;
			float clickRangeY = 12.0f;
			// convert the screen coordinates to graph coordinates
			float dGraphWidth = (float)(rcGraphRect.right - rcGraphRect.left);
			xPos = (((float)xPos - (float)rcGraphRect.left) / (float)dGraphWidth) * STEPTIME100;
			clickRangeX = (clickRangeX / dGraphWidth) * (float)STEPTIME100;
			float dGraphHeight = (float)(rcGraphRect.bottom - rcGraphRect.top);
			float verticalMargins = dGraphHeight / 10.0f;
			float heightMin = verticalMargins;
			float heightMax = dGraphHeight - verticalMargins;
			float beta = (heightMax - heightMin) / (maxY - minY);
			float alpha = heightMin - beta * minY;
			yPos = (((dGraphHeight - yPos) - alpha) / beta);
			clickRangeY /= beta;

			int iLimb;
			bkey = FALSE; bInTan = FALSE; bOutTan = FALSE;
			if (ctrlBranch->GetClickedKey(Point2(xPos, yPos), Point2(clickRangeX, clickRangeY), iKeyNum, iLimb, bkey, bInTan, bOutTan))
			{
				if (!theHold.Holding()) {
					theHold.Begin();
				}

				if (!btnLock->IsChecked())
				{	//root->SetActiveLeaf(iLimb);
					SendMessage(hLimbs, LB_SETCURSEL, iLimb, 0);
					ctrlBranch->SetActiveLeaf(iLimb);
				}

				bDragingKey = TRUE;
				GetGraphKey();

				float time;
				if (ctrlTime)
				{
					// this means you can't drag kes more than 2 page
					Interval iv = FOREVER;
					ctrlTime->GetValue(GetCOREInterface()->GetTime(), (void*)&time, iv);
					if ((time < 0.0f)/*||(fPrevKeyTime>xPos)*/)
						dKeyTimeOffset = -STEPRATIO100;
					else if ((time > STEPRATIO100)/*||(fNextKeyTime<xPos)*/)
						dKeyTimeOffset = STEPRATIO100;
					else dKeyTimeOffset = 0.0f;
				}
				else dKeyTimeOffset = 0.0f;

				SetCapture(hDlg);

				// Redraw the graph with thte selected key red
				DrawGraph(ctrlBranch, iKeyNum);
			}
			else bDragingKey = FALSE;
		}

		break;
	}
	case WM_LBUTTONUP:
		ReleaseCapture();
		bDragingKey = FALSE;
		if (theHold.Holding()) {
			theHold.Accept(GetString(IDS_HLD_GRAPHDRAG));
		}
		break;
	case WM_RBUTTONUP:
		ReleaseCapture();
		bDragingKey = FALSE;
		if (theHold.Holding()) {
			theHold.Cancel();
		}
		break;
	case WM_MOUSEMOVE: {
		int fwKeys = (int)wParam;        // key flags
		float xPos = (float)GET_X_LPARAM(lParam);  // horizontal position of cursor
		float yPos = (float)GET_Y_LPARAM(lParam);  // vertical position of cursor

		if ((xPos > rcGraphRect.left) &&
			(xPos < rcGraphRect.right) &&
			(yPos > rcGraphRect.top) &&
			(yPos < rcGraphRect.bottom) &&
			(ctrlBranch->GetUIType() == UI_GRAPH)) {
			SetCursor(LoadCursor(NULL, IDC_CROSS));
		}
		else if (!bDragingKey) SetCursor(LoadCursor(NULL, IDC_ARROW));

#ifdef SKIP_REDUNDANT_MOUSEMOVES
		// Look ahead for subsequent WM_MOUSEMOVE messages in
		// the queue.  When we encounter a message that is not
		// a WM_MOUSEMOVE, we set a flag to cause it to be
		// dispatched after finishing the processing of the
		// most recent WM_MOUSEMOVE.
		//
		// Note: After implementing this I found that only one
		// in every 20 or so messages was ever skipped.  And
		// that's with excessive mouse movement (GB 21-Jul-03).
		MSG msg;
		BOOL bDispatchMessageWhenDone = FALSE;
		while (PeekMessage(&msg, hDlg, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)) {
			if (msg.message == WM_MOUSEMOVE) {
				xPos = GET_X_LPARAM(msg.lParam);
				yPos = GET_Y_LPARAM(msg.lParam);
				fwKeys = msg.wParam;
				DebugPrint(_T("Skipped a WM_MOUSEMOVE\n"));
			}
			else {
				bDispatchMessageWhenDone = TRUE;
				break;
			}
		}
#endif

		if ((fwKeys == MK_LBUTTON) && bDragingKey)
		{
			DebugPrint(_T("WM_MOUSEMOVE (%d,%d)\n"), xPos, yPos);
			TimeValue t = GetCOREInterface()->GetTime();

			//	Convert the screen coordinates to key values
			float dGraphWidth = (float)(rcGraphRect.right - rcGraphRect.left);
			xPos = (((float)xPos - (float)rcGraphRect.left) / (float)dGraphWidth) * STEPRATIO100;

			xPos += dKeyTimeOffset;

			float dGraphHeight = (float)(rcGraphRect.bottom - rcGraphRect.top);
			float verticalMargins = dGraphHeight / 10.0f;
			float heightMin = verticalMargins;
			float heightMax = dGraphHeight - verticalMargins;
			float beta = (heightMax - heightMin) / (maxY - minY);
			float alpha = heightMin - beta * minY;
			yPos = (((dGraphHeight - yPos) - alpha) / beta);
			yPos = min(max(minVal, yPos), maxVal);

			if (!theHold.Holding()) {
				theHold.Begin();
			}

			if (bkey) {
				// SetKeyTime
				if (ctrlTime)
				{
					xPos = min(fNextKeyTime - 3.0f, max(fPrevKeyTime + 3.0f, xPos));
					ValuetoSpinnerAndControl(spnTime, ctrlTime, t, xPos, this);
				}
				// SetKeyVal
				if (ctrlValue)
				{
					ValuetoSpinnerAndControl(spnValue, ctrlValue, t, yPos, this);
				}
			}
			else if (bInTan)
			{
				// convert to Tangent values
				float keytime = spnTime->GetFVal();
				float keyvalue = spnValue->GetFVal();
				if (keytime < xPos)xPos -= STEPRATIO100;
				double baseLength = fabs(keytime - fPrevKeyTime);
				float tanLen = (float)(fabs(keytime - xPos) / baseLength);
				if (ctrlInTanLen)
				{
					tanLen = (float)max(min(tanLen, 1.0f), 0.0f);

					ctrlInTanLen->SetValue(t, (void*)&tanLen, 1, CTRL_ABSOLUTE);
					spnInTanLen->SetValue(tanLen, FALSE);
					if (ctrlInTanLen->IsKeyAtTime(t, NULL)) spnInTanLen->SetKeyBrackets(TRUE);
					else spnInTanLen->SetKeyBrackets(FALSE);
				}
				baseLength *= GetTicksPerFrame() * tanLen;
				yPos = (keyvalue - yPos);
				double angle = atan(yPos / baseLength);
				float Tangent = (float)tan(angle) * 25;//GetFrameRate();
				if (ctrlTangent)
				{
					ctrlTangent->SetValue(t, (void*)&Tangent, 1, CTRL_ABSOLUTE);
					spnTangent->SetValue(Tangent, FALSE);
					if (ctrlTangent->IsKeyAtTime(t, NULL)) spnTangent->SetKeyBrackets(TRUE);
					else spnTangent->SetKeyBrackets(FALSE);
				}
			}
			else if (bOutTan)
			{
				//	convert to Tangent values
				float keytime = spnTime->GetFVal();
				float keyvalue = spnValue->GetFVal();
				if (keytime > xPos) xPos += STEPRATIO100;
				double baseLength = fabs(keytime - fNextKeyTime);
				float tanLen = (float)(fabs(xPos - keytime) / baseLength);
				if (ctrlOutTanLen)
				{
					tanLen = (float)max(min(tanLen, 1.0f), 0.0f);

					ctrlOutTanLen->SetValue(t, (void*)&tanLen, 1, CTRL_ABSOLUTE);
					spnOutTanLen->SetValue(tanLen, FALSE);
					if (ctrlOutTanLen->IsKeyAtTime(t, NULL)) spnOutTanLen->SetKeyBrackets(TRUE);
					else spnOutTanLen->SetKeyBrackets(FALSE);
				}
				baseLength *= GetTicksPerFrame() * tanLen;
				yPos = yPos - keyvalue;
				float angle = (float)atan(yPos / baseLength);
				float Tangent = (float)tan(angle) * 25;// * GetFrameRate();
				if (ctrlTangent)
				{
					ctrlTangent->SetValue(t, (void*)&Tangent, 1, CTRL_ABSOLUTE);
					spnTangent->SetValue(Tangent, FALSE);
					if (ctrlTangent->IsKeyAtTime(t, NULL)) spnTangent->SetKeyBrackets(TRUE);
					else spnTangent->SetKeyBrackets(FALSE);
				}
			}

			// Draw graph without invalidating the region,
			// create a DC, paint it, and validate (in case
			// there are paint messages further down the
			// queue).
			if (!GetCOREInterface()->IsAnimPlaying())
				GetCOREInterface()->RedrawViews(t);
			DrawGraph(ctrlBranch, iKeyNum, FALSE);
		}

#ifdef SKIP_REDUNDANT_MOUSEMOVES
		if (bDispatchMessageWhenDone) {
			DispatchMessage(&msg);
		}
#endif
		break;
		}
	}
	return FALSE;
}

//
// Creates a new graph pane window inside a floating modeless
// window.  This is so we can adjust a specific graph without
// requiring the CATWindow navigator.
//
// Returns: Pointer to the new HWND on success.
//          NULL on failure.
//
HWND CreateFloatingGraphWindow(HINSTANCE hInst, HWND hwndParent,
	CATHierarchyRoot *root,
	CATHierarchyBranch *pGraphableBranch)
{
	HWND hWnd = CreateWindow(
		_T(""), GetString(IDS_GRAPH),
		WS_OVERLAPPED | WS_DLGFRAME | WS_CAPTION | WS_MINIMIZEBOX,
		0, 0, 0, 0, hwndParent, NULL, hInst, NULL);

	if (hWnd) {
		// Create the graph pane as a child to the new overlapped
		// window.  If successful, hook it to the required branch.
		// Otherwise cancel the entire operation and return NULL.
		CATWindowGraph *pGraphWnd = new CATWindowGraph();
		pGraphWnd->root = root;

		if (!pGraphWnd->Create(hInst, hWnd)) {
			DestroyWindow(hWnd);
			delete pGraphWnd;
			return NULL;
		}

		pGraphWnd->ConnectSpinnersWithControllers(pGraphableBranch);

		// Resize the window so its client area fits the graph window.
		// Then reposition the graph window so its top-left corner
		// is at (0,0) in client coords of the floating window.
		RECT rcWndRect, rcOriginal;
		GetWindowRect(hWnd, &rcOriginal);
		GetWindowRect(pGraphWnd->GetDlg(), &rcWndRect);
		AdjustWindowRect(&rcWndRect, WS_DLGFRAME | WS_CAPTION, NULL);
		OffsetRect(&rcWndRect, rcOriginal.left - rcWndRect.left, rcOriginal.top - rcWndRect.top);

		SetWindowPos(hWnd, NULL,
			rcWndRect.left, rcWndRect.top,
			rcWndRect.right - rcWndRect.left,
			rcWndRect.bottom - rcWndRect.top,
			SWP_NOZORDER | SWP_SHOWWINDOW);

		SetWindowPos(pGraphWnd->GetDlg(), NULL, 0, 0, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}

	return hWnd;
}

////////////////////////////////////////////////////////////////////////////////
// Now the CAT Window itself.  This has a hierarchy navigator that allows us
// to easily browse through the CAT Motion Hierarchy.  Selecting a single
// hierarchy branch in the navigator will bring up a CATWindowPane, which
// fills the rest of the CATWindow.  The purpose of the pane is to provide
// controls particular to that branch.
////////////////////////////////////////////////////////////////////////////////

class CATWindow : public CATDialog {
private:

	CATHierarchyRoot *root;

	RECT rcPaneRect;
	CATWindowPane *paneCurrent;

	// Maybe we will be able to work it so that we
	// can see lots of different graphs at once
	CATWindowGraph *paneGraph;

	HWND hNavigator;
	NavigatorData *listNavigatorData;
	NavigatorData *listNavigatorCurrent;

	WORD GetDialogTemplateID() const { return IDD_CATWINDOW; }

	void InitControls();
	void ReleaseControls();

	enum NavigatorBranchState {
		navExpand,
		navCollapse,
		navToggle,
		navToggleRecursive
	};

	int BuildNavigatorList(NavigatorData* data, int nItem = 0);
	void SetHierarchyBranchState(CATHierarchyBranch *branch, BOOL bExpanded);
	void SetNavigatorBranchState(int nItem, NavigatorBranchState state);
	void RefreshNavigatorList();

	void ReplaceCurrentPane(CATWindowPane* newPane, BOOL bIsGraph = FALSE);

public:

	CATWindow(CATHierarchyRoot *catHierarchyRoot) : CATDialog(), paneGraph(NULL) {
		root = catHierarchyRoot;
		SetRectEmpty(&rcPaneRect);
		paneCurrent = NULL;

		hNavigator = NULL;
		listNavigatorData = NULL;
		listNavigatorCurrent = NULL;
	}

	// The dialog callback is where all the fun stuff goes down...
	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Here is stuff that could conceivably happen from within
	// the dialog.

	// The different UI that will be available
	void ShowWelcomePage();

	void ShowPresetsUI(CATHierarchyRoot* root);
	void ShowKALiftingUI(CATHierarchyRoot* root);
	void ShowGlobalsUI(CATHierarchyRoot* root, CATHierarchyBranch* ctrlGlobals);
	void ShowLimbPhasesUI(CATHierarchyBranch* ctrlPhases);
	// Graphs will be used by all the data graphs
	void ShowGraphUI(CATHierarchyRoot* root, CATHierarchyBranch* ctrlBranch);//, CATHierarchyBranch* ctrlRoot);
//	void DrawGraph(CATHierarchyBranch* ctrlBranch, int iKeyNum, int hGraphDC, const RECT& rcGraphRect);
};

HWND CreateCATWindow(HINSTANCE hInstance, HWND hWnd, CATHierarchyRoot *catHierarchy/*=NULL*/) {
	CATWindow *dlgData = new CATWindow(catHierarchy);
	HWND hDlg = dlgData->Create(hInstance, hWnd);

	if (!hDlg) {// the window wasnt created, display an error
		LPVOID lpMsgBuf;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox(NULL, (LPCTSTR)lpMsgBuf, GetString(IDS_ERROR), MB_OK | MB_ICONINFORMATION);
		// Free the buffer.
		LocalFree(lpMsgBuf);
	}

	dlgData->Show();
	return hDlg;
}

// Initialise all the Custom Controls
void CATWindow::InitControls() {
	hNavigator = GetDlgItem(hDlg, IDC_LIST_NAVIGATOR);

	// Get the client rectangle and the navigator rectangle,
	// and calculate the rectangle to be used for the pane.
	RECT rcNavRect;
	GetWindowRect(hNavigator, &rcNavRect);
	GetClientRect(hDlg, &rcPaneRect);
	rcPaneRect.left = rcNavRect.right - rcNavRect.left + 1;

	//	pCATTreeView = NULL;
}

void CATWindow::ReleaseControls() {
	hNavigator = NULL;
}

//
// CATWindow Dialog Callback function.
// All the fun stuff happens here.
//
INT_PTR CALLBACK CATWindow::DialogProc(HWND, UINT uMsg, WPARAM wParam, LPARAM)
{
	switch (uMsg) {
	case WM_INITDIALOG: {
		//			GetCOREInterface()->RegisterDlgWnd(hDlg);
		CenterWindow(hDlg, GetParent(hDlg));
		RefreshNavigatorList();
		ShowPresetsUI(root);
		return TRUE;
	}

	case WM_DESTROY: {
		//			GetCOREInterface()->UnRegisterDlgWnd(hDlg);
		listNavigatorData->DestroyEntireList();
		root->UnlinkCATWindow();
		return TRUE;
	}

	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
			// Commands to the hierarchy navigator...  Possible commands are:
			//
			//  * single-click to select the item and display its rollout.
			//  * double-click to select the item, display its rollout,
			//    and expand or collapse it.
			//
		case IDC_LIST_NAVIGATOR: {

			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE: {
				int nNumSel = (int)SendMessage(hNavigator, LB_GETSELCOUNT, 0, 0);
				int item = (int)SendMessage(hNavigator, LB_GETCURSEL, 0, 0);
				if (nNumSel == 0) {
					ShowWelcomePage();
				}
				else {

					// Toggle the expand/collapse state of the selected item.
					// If a Shift key was held down during double-click, do
					// a recursive toggle (ie, all children get the same
					// expand/collapse state).
					if (GetKeyState(VK_SHIFT) & 0x8000) {
						SetNavigatorBranchState(item, navToggleRecursive);
					}
					else {
						SetNavigatorBranchState(item, navToggle);
					}

					// Get the navigator data for the selected item, and if the
					// selection has actually CHANGED, display its rollout page.
					//
					NavigatorData *selectionData = (NavigatorData*)SendMessage(hNavigator, LB_GETITEMDATA, (WPARAM)item, 0);
					if (selectionData != listNavigatorCurrent && (LPARAM)selectionData != LB_ERR) {
						listNavigatorCurrent = selectionData;

						// Change the window title, using the character name
						// followed by the name of the currently selected branch.
						DbgAssert(root && root->GetCATParentTrans());
						TSTR pCharacterName = root->GetCATParentTrans()->GetCATName();

						TCHAR pTitle[513];
						if (nNumSel <= 0) {
							_sntprintf(pTitle, 512, _T("%s"), pCharacterName.data());
						}
						else if (nNumSel == 1) {
							_sntprintf(pTitle, 512, _T("%s - %s"), pCharacterName.data(), listNavigatorCurrent->name);
						}
						else {
							_sntprintf(pTitle, 512, _T("%s - (multiple)"), pCharacterName.data());
						}
						SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)pTitle);

						// Display rollout page here.  This is a really basic
						// setup for testing, but I imagine the way it'll actually
						// work is completely different.
						if (item == 0)
							ShowPresetsUI(root);
						else {
							switch (((CATHierarchyBranch2*)listNavigatorCurrent->branch)->GetUIType()) {
							case UI_BLANK:
								//	ReplaceCurrentPane(NULL);
								ShowWelcomePage();
								break;

							case UI_FLOAT:
							case UI_POINT3:
							case UI_GRAPH: {
								// remove all the branches from the list
								root->ClearGraphingBranchs();

								ShowGraphUI(root, listNavigatorCurrent->branch);//, root);

								// Get a table of selected items
								if (nNumSel > 0)
								{
									LONG *tabSel = new LONG[nNumSel];
									SendMessage(hNavigator, LB_GETSELITEMS, (WPARAM)nNumSel, (LPARAM)tabSel);
									// add all the other branches to the list of graphing ones
									for (int i = 0; i < nNumSel; i++) {
										NavigatorData *selectionData = (NavigatorData*)SendMessage(hNavigator, LB_GETITEMDATA, (WPARAM)tabSel[i], 0);
										if (selectionData && (((CATHierarchyBranch2*)selectionData->branch)->GetUIType() == UI_GRAPH))
											root->AddGraphingBranch((CATHierarchyBranch2*)selectionData->branch);
									}
									delete[] tabSel;
								}

								//	root->AddGraphingBranch((CATHierarchyBranch2*)listNavigatorCurrent->branch);

									// This resets the scaling factor used in the
									// Graphing window to frame the Curve
								root->GetYRange(((CATWindowGraph*)paneCurrent)->GetMinY(), ((CATWindowGraph*)paneCurrent)->GetMaxY());
								((CATWindowGraph*)paneCurrent)->DrawGraph(listNavigatorCurrent->branch, 1);

								break;
							}

							case UI_FOOTSTEPS:
								ShowGlobalsUI(root, listNavigatorCurrent->branch);
								break;

							case UI_LIMBPHASES:
								ShowLimbPhasesUI(listNavigatorCurrent->branch);
								break;

							default:
								//												ReplaceCurrentPane(NULL);
								ShowWelcomePage();
							}
						}
					}

				}
				break;
			}
									  /*
								  case LBN_DBLCLK:
									  int item = SendMessage(hNavigator, LB_GETCURSEL, 0, 0);
									  if (item != LB_ERR) {
										  // Toggle the expand/collapse state of the selected item.
										  // If a Shift key was held down during double-click, do
										  // a recursive toggle (ie, all children get the same
										  // expand/collapse state).
										  if (GetKeyState(VK_SHIFT) & 0x8000) {
											  SetNavigatorBranchState(item, navToggleRecursive);
										  } else {
											  SetNavigatorBranchState(item, navToggle);
										  }
									  }
									  break;
									  */
			}
			break;
		}
		}
		break;
	}

	}
	return FALSE;
}

// This is a recursive function that inserts items into the
// navigator list box, beginning at 'branch'.  Each list box
// element stores a pointer to a NavigatorData instance.
//
// Branches with child branches are expandable (the user can
// double-click to expand/collapse it).  If a branch is expanded,
// its child branches are also displayed.
//
// The return value is the index AFTER the last item that was
// inserted.

int CATWindow::BuildNavigatorList(NavigatorData* data, int nItem/*=0*/)
{
	static TCHAR buffer[STRING_BUFFER_SIZE + 1];
	static TCHAR *strPrefix[3] = {
		_T("    "),		// not expandable
		_T("[  ]"),		// expanded
		_T("[+]")		// collapsed
	};

	CATHierarchyBranch *branch = data->branch;
	DbgAssert(branch);

	// Choose the correct prefix for the branch.
	int prefix;
	if (!branch->GetExpandable() || branch->GetNumBranches() == 0) prefix = 0;
	else prefix = branch->bExpanded ? 1 : 2;

	// Build the string and add it to the navigator.  Note that
	// the list box allocates its own memory and copies the string.
	_sntprintf(buffer, STRING_BUFFER_SIZE, _T("%*s%s %s"), data->level * 5, _T(""), strPrefix[prefix], data->name);
	SendMessage(hNavigator, LB_INSERTSTRING, (WPARAM)nItem, (LPARAM)buffer);
	SendMessage(hNavigator, LB_SETITEMDATA, (WPARAM)nItem, (LPARAM)data);
	nItem++;

	// Recursively add children if we are expanded.
	if (branch->bExpanded) {
		for (int i = 0; i < branch->GetNumBranches(); i++) {
			NavigatorData *childData = new NavigatorData((CATHierarchyBranch*)branch->GetBranch(i), branch->GetBranchName(i), data->level + 1);
			data->Append(childData);
			nItem = BuildNavigatorList(childData, nItem);
		}
	}

	return nItem;
}

// Recursively sets the state of the branch and all its expandable
// children to the given state.
//
void CATWindow::SetHierarchyBranchState(CATHierarchyBranch *branch, BOOL bExpanded)
{
	if (branch && branch->GetExpandable()) {
		if (branch->GetNumBranches() != 0 && branch != root)
			branch->bExpanded = bExpanded;

		for (int i = 0; i < branch->GetNumBranches(); i++) {
			SetHierarchyBranchState(((CATHierarchyBranch*)branch->GetBranch(i)), bExpanded);
		}
	}
}

// Expands or collapses a branch in the navigator list box.  This
// is achieved by removing the branch and all its children from
// the list box, setting its 'expanded' flag to the desired value,
// and calling BuildNavigatorList() to re-insert it at the correct
// position.
//
void CATWindow::SetNavigatorBranchState(int nItem, NavigatorBranchState state)
{
	NavigatorData *data = (NavigatorData*)SendMessage(hNavigator, LB_GETITEMDATA, nItem, 0);

	if (data) {
		// Make sure that this branch is expandable.
		if (data->branch->GetNumBranches() == 0 || !data->branch->GetExpandable()) return;

		// Take requested action.  If there is no change to the
		// existing state, just return.  Also, if we're trying
		// to collapse or toggle the state of the hierarchy root,
		// just return.
		switch (state) {
		case navExpand:
			if (data->branch->bExpanded) return;
			data->branch->bExpanded = FALSE;
			break;

		case navCollapse:
			if (!data->branch->bExpanded) return;
			if (data->branch == root) return;
			data->branch->bExpanded = FALSE;
			break;

		case navToggle:
			if (data->branch == root) return;
			data->branch->bExpanded = !data->branch->bExpanded;
			break;

		case navToggleRecursive:
			SetHierarchyBranchState(data->branch, !data->branch->bExpanded);
			break;

		}

		// Delete the branch's string, but don't delete its
		// NavigatorData.
		SendMessage(hNavigator, LB_DELETESTRING, nItem, 0);

		// Delete each navigator item following this one, whose
		// level is deeper.  These are its children.  If LB_ERR
		// is returned for the user data, assume that there are
		// no more items in the navigator.  We use nItem every
		// time because each delete will shuffle the remaining
		// items up one slot.
		NavigatorData *childData;
		for (;;) {
			childData = (NavigatorData*)SendMessage(hNavigator, LB_GETITEMDATA, nItem, 0);
			if ((LPARAM)childData == LB_ERR) break;
			if (childData->level <= data->level) break;
			delete childData;
			SendMessage(hNavigator, LB_DELETESTRING, nItem, 0);
		}

		// Add the branch back in at the current spot, and set the
		// current selection back to it.
		BuildNavigatorList(data, nItem);
		SendMessage(hNavigator, LB_SETSEL, (WPARAM)TRUE, (LPARAM)nItem);
	}
}

// This erases the navigator list box and fills it again.
//
void CATWindow::RefreshNavigatorList()
{
	// Erase the navigator listbox and then kill the
	// navigator data list.
	SendMessage(hNavigator, LB_RESETCONTENT, 0, 0);
	if (listNavigatorData) {
		listNavigatorData->DestroyEntireList();
		listNavigatorData = NULL;
	}

	// Reinitialise the navigator list box from the
	// root level (which is always expanded).
	root->bExpanded = TRUE;
	listNavigatorData = new NavigatorData(root, _T("CATMotion Presets"));
	BuildNavigatorList(listNavigatorData);

	// If there's no current selection, make one!  We first change
	// the selection, then force a notify (because LB_SETCURSEL
	// doesn't notify).
//	if (SendMessage(hNavigator, LB_GETCURSEL, 0, 0) == LB_ERR) {
		//GB 26-Jun-03: Don't select but still notify.  This will cause
		//              the welcome screen to be displayed.
		//SendMessage(hNavigator, LB_SETCURSEL, 0, 0);
	SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIST_NAVIGATOR, LBN_SELCHANGE), (LPARAM)hNavigator);
	//	}
}

// Replaces the current pane (if any) with a new one (if any).
// The parameter 'bIsGraph' tells us if this is a graph pane.
// This means if any non-graph pane replaces a graph pane, we
// don't still think there's a graph hanging around.
void CATWindow::ReplaceCurrentPane(CATWindowPane* newPane, BOOL bIsGraph/*=FALSE*/)
{
	if (!bIsGraph) paneGraph = NULL;

	if (newPane) {
		// Repositions the pane and sets its z-order to be on top.
		SetWindowPos(
			newPane->GetDlg(), HWND_TOP,
			rcPaneRect.left, rcPaneRect.top,
			0, 0, SWP_NOSIZE);
		newPane->Show();
	}

	if (paneCurrent) {
		// This will automatically delete the pane.
//		paneCurrent->Hide();
		SendMessage(paneCurrent->GetDlg(), WM_CLOSE, 0, 0);
	}

	paneCurrent = newPane;
}

void CATWindow::ShowWelcomePage()
{
	CATWindowWelcome *pane = new CATWindowWelcome();
	if (pane) {
		pane->Create(hInstance, hDlg);
		ReplaceCurrentPane(pane);
	}
}

void CATWindow::ShowPresetsUI(CATHierarchyRoot* root)
{
	CATWindowPresets *pane = new CATWindowPresets(root);
	if (pane) {
		pane->Create(hInstance, hDlg);
		ReplaceCurrentPane(pane);
	}
}

void CATWindow::ShowKALiftingUI(CATHierarchyRoot* root)
{
	UNREFERENCED_PARAMETER(root);
	/*	CATWindowKALifting *pane = new CATWindowKALifting(root);
		if (pane) {
			pane->Create(hInstance, hDlg);
			ReplaceCurrentPane(pane);
		}*/
}

void CATWindow::ShowGlobalsUI(CATHierarchyRoot* root, CATHierarchyBranch* ctrlGlobals)
{
	CATWindowGlobals *pane = new CATWindowGlobals(root, ctrlGlobals);
	if (pane) {
		pane->Create(hInstance, hDlg);
		ReplaceCurrentPane(pane);
	}
}

void CATWindow::ShowLimbPhasesUI(CATHierarchyBranch* ctrlPhases)
{
	CATWindowLimbPhases *pane = new CATWindowLimbPhases((CATHierarchyBranch2*)ctrlPhases);
	if (pane) {
		pane->Create(hInstance, hDlg);
		ReplaceCurrentPane(pane);
	}
}

void CATWindow::ShowGraphUI(CATHierarchyRoot* root, CATHierarchyBranch* ctrlBranch)//, CATHierarchyBranch* ctrlRoot)
{
	// GB 12-Jul-2003: Added check to allow NULL paneGraph to
	// pass through.  This fixes the bug where the first graph
	// pane doesn't get displayed.
	if (!paneGraph || paneGraph != paneCurrent)
	{
		paneGraph = new CATWindowGraph();
		if (paneGraph) {
			paneGraph->root = root;
			paneGraph->Create(hInstance, hDlg);
			ReplaceCurrentPane(paneGraph, TRUE);

			// GB 12-Jul-2003: This is done in the graph pane's InitControls(), where it should be.
			//			this->paneCurrent->root->SetGraphBuff(this->paneGraph->iGraphBuff, this->paneGraph->rcGraphRect);

						//this->paneGraph->root = ctrlRoot;
		}
	}

	// GB 12-Jul-2003: Added check for NULL, in case the graph
	// pane creation fails.
	if (paneGraph) {
		paneGraph->ConnectSpinnersWithControllers(ctrlBranch);
	}
}
