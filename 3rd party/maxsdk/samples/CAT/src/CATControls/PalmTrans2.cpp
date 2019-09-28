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

// Takes Matrix passed and uses it to find PALMPB_REF. Also has
// IK/FK blending, passed in from lowerLimbTrans

// CATProject Stuff
#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>
#include "CATFilePaths.h"

// Max Stuff
#include "iparamm2.h"
#include "decomp.h"
#include <direct.h>
#include "math.h"
#include "Locale.h"
#include "MaxIcon.h"

// CATRig Hierarchy
#include "ICATParent.h"

#include "../CATObjects/ICATObject.h"
#include "LimbData2.h"
#include "FootTrans2.h"
#include "DigitData.h"
#include "DigitSegTrans.h"

#include "IKTargController.h"
#include "PalmTrans2.h"
#include "BoneSegTrans.h"

// Layers
#include "CATClipRoot.h"
#include "CATClipHierarchy.h"
#include "CATClipValue.h"
#include "CATClipWeights.h"
#include "CATClipValues.h"

// CATMotion
#include "CATHierarchyBranch2.h"
#include "PivotPosData.h"
#include "PivotRot.h"
#include "CATMotionRot.h"
#include "CATMotionDigitRot.h"
#include "MonoGraph.h"
#include "maxtextfile.h"

//
//	PalmTransClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class PalmTrans2ClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		PalmTrans2* bone = new PalmTrans2(loading);
		return bone;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_PALMTRANS2); }
	Class_ID		ClassID() { return PALMTRANS2_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("PalmTrans"); } // returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
static PalmTrans2ClassDesc PalmTrans2Desc;
ClassDesc2* GetPalmTrans2Desc() { return &PalmTrans2Desc; }

// More undo stuff.
#define UNDO 0
#define REDO 1

class SetObjDimRestoreUndo : public RestoreObj {
public:
	CATNodeControl	*ctrl;
	SetObjDimRestoreUndo(CATNodeControl *c) { ctrl = c; }
	void Restore(int isUndo) { ctrl->UpdateObjDim();	if (isUndo) { ctrl->Update();	ctrl->UpdateUI(); } }
	void Redo() {	}
	int Size() { return 1; }
};
class SetObjDimRestoreRedo : public RestoreObj {
public:
	CATNodeControl	*ctrl;
	SetObjDimRestoreRedo(CATNodeControl *c) { ctrl = c; }
	void Restore(int isUndo) { UNREFERENCED_PARAMETER(isUndo); }
	void Redo() { ctrl->UpdateObjDim();	ctrl->Update();	ctrl->UpdateUI(); }
	int Size() { return 1; }
};

class PalmMotionParamDlgCallBack : public TimeChangeCallback {

	PalmTrans2* palm;
	LimbData2* pLimb;

	HWND hWnd;

	HWND lbxAvailable;
	HWND lbxDigits;
	BOOL handposeloaded;

	//	std::vector<std::vector> vecSelectedDigits;

	std::vector<TCHAR*> vecFileNames;
	std::vector<TCHAR*> vecPoseNames;
	std::vector<TCHAR*> vecFolderNames;

	TCHAR strCurrentFolder[MAX_PATH];
	bool bShowFolders;

	std::vector<std::vector<Quat> > vecOriginalSegRots;
	std::vector<std::vector<Quat> > vecHandPoseSegRots;

	TCHAR* selectedHandPose;

	ISpinnerControl *spnPoseWeight;
	ICustButton *btnSavePose;
	ICustButton *btnDelete;

	ISpinnerControl *spnCurlDigits;
	ISpinnerControl *spnSpreadDigits;
	ISpinnerControl *spnBendDigits;
	ISpinnerControl *spnRollDigits;

public:

	void AcceptCallback(int data) { UNREFERENCED_PARAMETER(data); };
	void RestoreCallback(BOOL isUndo, int data) { if (isUndo && data == UNDO) TimeChanged(GetCOREInterface()->GetTime()); };
	void RedoCallback(int data) { if (data == REDO) TimeChanged(GetCOREInterface()->GetTime()); };

	HWND GetHWnd() { return hWnd; }

	PalmMotionParamDlgCallBack()
		//	: vecSelectedDigits(),
		: vecOriginalSegRots(),
		vecHandPoseSegRots(),
		vecFileNames(),
		vecPoseNames(),
		vecFolderNames(),
		hWnd(NULL),
		palm(NULL),
		handposeloaded(0),
		selectedHandPose(NULL),
		pLimb(NULL)
	{
		lbxAvailable = NULL;
		lbxDigits = NULL;
		spnPoseWeight = NULL;

		btnSavePose = NULL;
		btnDelete = NULL;
		//	hImageSavePoseBtn =	NULL;
		//	hImageDeleteBtn =	NULL;

		spnCurlDigits = NULL;
		spnSpreadDigits = NULL;
		spnBendDigits = NULL;
		spnRollDigits = NULL;

		bShowFolders = true;
		_tcscpy(strCurrentFolder, _T(""));

	}

	//////////////////////////////////////////////////////////////////////////
	// Applies a rotations around the x axis of the original finger angles  //
	// and then sets this in the finger                                     //
	//////////////////////////////////////////////////////////////////////////
	void ModifyDigits(AngAxis ax, BOOL onlyFirstBone, BOOL spread = FALSE)
	{
		//	if(theHold.Holding()) theHold.Restore();

		TimeValue t = GetCOREInterface()->GetTime();
		int lengthaxis = palm->GetCATParentTrans()->GetLengthAxis();

		int nNumSel = (int)SendMessage(lbxDigits, LB_GETSELCOUNT, 0, 0);
		LONG *tabSelectedDigits = new LONG[nNumSel];
		SendMessage(lbxDigits, LB_GETSELITEMS, (WPARAM)nNumSel, (LPARAM)tabSelectedDigits);

		for (int i = 0; i < nNumSel; i++)
		{
			// is this finger selected
			DigitData *digit = palm->GetDigit(tabSelectedDigits[i]);
			if (digit)
			{
				int numSegs = 1;
				if (!onlyFirstBone) numSegs = digit->GetNumBones();

				for (int j = 0; j < numSegs; j++)
				{
					DigitSegTrans *digitbone = digit->GetBone(j);
					if (digitbone)
					{
						INode* digitbonenode = digitbone->GetNode();
						AngAxis digitboneax = ax;
						if (spread) {
							if (lengthaxis == Z)	digitboneax.angle *= digit->GetRootPos().x;
							else				digitboneax.angle *= -digit->GetRootPos().z;
						}
						else		digitboneax.angle *= digitbone->GetBendWeight();
						SetXFormPacket ptr(digitboneax, 1, digitbonenode->GetParentNode()->GetNodeTM(t), digitbonenode->GetNodeTM(t));
						digitbone->GetLayerTrans()->SetValue(t, (void*)&ptr, 1, CTRL_ABSOLUTE);
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Builds the array which represents the hand pose stored in the file   //
	//////////////////////////////////////////////////////////////////////////
	void LoadHandPose(const TCHAR* HandPoseFilePath)
	{
		// GB 24-Oct-03: Save current numeric locale and set to standard C.
		// This cures our number translation problems in central europe.
		TCHAR *strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));
		MaxSDK::Util::TextFile::Reader fpHandPose;
		// Original open mode was "r" - assuming Text
		if (!fpHandPose.Open(HandPoseFilePath)) return;
		GetCATMessages().Debug(_T("Begin reading in HandPreset:\n"));

		//	TCHAR *strBuf[512];
		TCHAR strBuf[512];
		int nCurrentDigit = 0, nNumDigits = 0;
		int nCurrentSeg = 0, nNumSegs = 0;
		int pos;
		float qx, qy, qz, qw;

		// Kill previous contents of hand pose stuff.
		vecHandPoseSegRots.resize(0);

		// This is a big bad loop that runs through the file reading
		// finger seg rotations and names.
		MaxSDK::Util::Char str;
		while (!fpHandPose.IsEndOfFile())
		{
			// Read to the end of the line and see what we end up with.
			// redo it late for efficiency, read in full, and parse by a stringstream?
			for (pos = 0; !fpHandPose.IsEndOfFile() && pos < 511; ) {
				str = fpHandPose.ReadChar();
				TCHAR c;
				str.ToMCHAR(&c, 1);
				if ((c == _T('\r')) || (c == _T('\n'))) break;
				if (c != _T('\f'))
					strBuf[pos++] = (TCHAR)tolower(c);
			}
			strBuf[pos] = _T('\0');

			// Currently we just handle a quat.
			if (0 == _tcsncicmp(strBuf, _T("(quat"), 5)) { // SA - don't globalize - file io
				strBuf[pos - 1] = _T('\0');
				if (4 == _stscanf(&strBuf[5], _T("%f%f%f%f"), &qx, &qy, &qz, &qw)) {
					// The quat was read successfully.  Pump it into the
					// a new finger segment rotation.
					nCurrentSeg = nNumSegs++;
					vecHandPoseSegRots[nCurrentDigit].resize(nNumSegs);

					Quat &q = vecHandPoseSegRots[nCurrentDigit][nCurrentSeg];
					q.x = qx;
					q.y = qy;
					q.z = qz;
					q.w = qw;

				}
				else {
					GetCATMessages().Debug(_T("Failed to read a quat in the string \"%s\"\n"), strBuf);
				}
			}
			if (0 == _tcsncicmp(strBuf, _T("digit"), 5)) {
				// Add a new finger.
				nNumSegs = 0;
				nCurrentDigit = nNumDigits++;
				vecHandPoseSegRots.resize(nNumDigits);
				GetCATMessages().Debug(_T("Reading new digit - \n"));
			}
		}

		// Don't forget to close
		fpHandPose.Close();

		// Restore the locale
		RestoreLocale(LC_NUMERIC, strOldNumericLocale);

		GetCATMessages().Debug(_T("Finished reading preset \n"));
	}

	//////////////////////////////////////////////////////////////////////////
	// Saves a hand pose preset out to the given file, overwriting the file //
	// if it already exists.  We store less information than the original   //
	// hand pose files...                                                   //
	//////////////////////////////////////////////////////////////////////////
	void SaveHandPose(const TCHAR* HandPoseFilePath)
	{
		MaxSDK::Util::TextFile::Writer fpHandPose;
		MaxSDK::Util::TextFile::Writer outfileWriter;
		Interface14 *iface = GetCOREInterface14();
		unsigned int encoding = 0;
		if (iface->LegacyFilesCanBeStoredUsingUTF8())
			encoding = CP_UTF8 | MaxSDK::Util::TextFile::Writer::WRITE_BOM;
		else
		{
			LANGID langID = iface->LanguageToUseForFileIO();
			encoding = iface->CodePageForLanguage(langID);
		}

		// Original open mode was "w" - assuming Text
		if (!fpHandPose.Open(HandPoseFilePath, false, encoding)) return;
		// GB 24-Oct-03: Save current numeric locale and set to standard C.
		// This cures our number translation problems in central europe.
		TCHAR *strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));
		TimeValue t = GetCOREInterface()->GetTime();
		const int lengthaxis = palm->GetCATParentTrans()->GetLengthAxis();
		DigitData *digit;
		DigitSegTrans *digitbone;

		//		TCHAR strBuf[512];
		int numDigits = palm->pblock->Count(PalmTrans2::PB_DIGITDATATAB);
		for (int nDigit = 0; nDigit < numDigits; nDigit++) {
			digit = palm->GetDigit(nDigit);
			if (digit) {
				fpHandPose.Printf(_T("Digit Digit%02d\n"), nDigit + 1);
				int numSegs = digit->pblock->Count(DigitData::PB_SEGTRANSTAB);
				for (int nSeg = 0; nSeg < numSegs; nSeg++) {
					digitbone = digit->GetBone(nSeg);
					if (digitbone)
					{
						Matrix3 segRot;
						digitbone->GetRotation(t, &segRot);

						Quat qtSegRot(segRot);

						// All hand poses are saved and loaded as if it is
						// on a right hand of a Z aligned character.
						if (lengthaxis == X)
						{
							qtSegRot.x *= -1.0f;
							qtSegRot.z *= -1.0f;

							qtSegRot = (Quat(RotateYMatrix(-HALFPI))  * qtSegRot) * Quat(RotateYMatrix(HALFPI));
						}

						palm->GetLimb()->SetLeftRight(qtSegRot);

						fpHandPose.Printf(_T("(quat %f %f %f %f)\n"), qtSegRot.x, qtSegRot.y, qtSegRot.z, qtSegRot.w);
					}
				}
			}
		}
		fpHandPose.Close();

		// Restore the locale
		RestoreLocale(LC_NUMERIC, strOldNumericLocale);
	}

	//////////////////////////////////////////////////////////////////////////
	// Blends on the currently loaded hand pose                             //
	//////////////////////////////////////////////////////////////////////////
	void ApplyHandPose(float weight)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		int iCATMode = palm->GetCATMode();
		const int lengthaxis = palm->GetCATParentTrans()->GetLengthAxis();
		DigitData *digit;

		int numPalmDigits = palm->pblock->Count(PalmTrans2::PB_DIGITDATATAB);
		int numFileDigits = (int)vecHandPoseSegRots.size();
		int numDigits = min(numPalmDigits, numFileDigits);

		for (int i = 0; i < numDigits; i++)
		{

			digit = palm->GetDigit(i);
			if (digit) {
				int numSegs = digit->pblock->Count(DigitData::PB_SEGTRANSTAB);
				int numFileSegs = (int)vecHandPoseSegRots[i].size();
				numSegs = min(numSegs, numFileSegs);

				DigitSegTrans *digitbone;
				for (int j = 0; j < numSegs; j++)
				{
					digitbone = digit->GetBone(j);
					if (digitbone)
					{
						Matrix3 tmRotation;
						digitbone->GetRotation(t, &tmRotation);
						Quat original(tmRotation);
						Quat newRot = vecHandPoseSegRots[i][j];
						// All hand poses are saved and loaded as if it is
						// on a right hand of a Z aligned character.
						if (lengthaxis == Z) {
							//		if(palm->GetLimb()->GetLMR()==-1){
							//			newRot.y *= -1.0f;
							//			newRot.z *= -1.0f;
							//		}
						}
						else {
							newRot = (Quat(RotateYMatrix(HALFPI))  * newRot) * Quat(RotateYMatrix(-HALFPI));
							if (palm->GetLimb()->GetLMR() == -1) {
								//			newRot.y *= -1.0f;
								//			newRot.z *= -1.0f;
							}
							else {
								newRot.x *= -1.0f;
								newRot.z *= -1.0f;
							}
						}
						palm->GetLimb()->SetLeftRight(newRot);
						newRot.MakeClosest(original);

						Quat segRot = Slerp(original, newRot, weight);
						Matrix3 tmNewRotation;
						segRot.MakeMatrix(tmNewRotation);
						digitbone->SetRotation(t, tmNewRotation);
					}
				}
			}
		}

		// the rest of the digits can just pose like the last digit from the file
		if (numPalmDigits > numFileDigits)
		{
			digit = palm->GetDigit(numFileDigits - 1);

			if (!digit) return;

			for (int i = numFileDigits; i < numPalmDigits; i++)
			{
				digit->PoseLikeMe(t, iCATMode, palm->GetDigit(i));
			}
		}
	}

	// This erases the hand pose preset list
	void ClearFileList() {
		unsigned int i;
		SendMessage(lbxAvailable, LB_RESETCONTENT, 0, 0);
		for (i = 0; i < vecFolderNames.size(); i++) delete[] vecFolderNames[i];
		for (i = 0; i < vecFileNames.size(); i++) {
			delete[] vecFileNames[i];
			delete[] vecPoseNames[i];
		}
		vecFolderNames.resize(0);
		vecFileNames.resize(0);
		vecPoseNames.resize(0);
	}

	// This attempts to change the current directory to the specified one.
	// If the attempt fails, we return FALSE.
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

	// Load up all existing preset names into the ListBox
	void RefreshAvailableHandPoses()
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

						id = (int)SendMessage(lbxAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)driveName);
						SendMessage(lbxAvailable, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
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
				id = (int)SendMessage(lbxAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)_T("<..>"));
				SendMessage(lbxAvailable, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
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
								id = (int)SendMessage(lbxAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)fileName);
								SendMessage(lbxAvailable, LB_SETITEMDATA, id, -(int)vecFolderNames.size() - 1);
								vecFolderNames.push_back(folder);
							}
						}
					}
				} while (FindNextFile(hFind, &find));

				FindClose(hFind);
			}
		}

		// Search for all rig files in the current directory and
		// insert them into the list box, while also maintaining them
		// in a vector.
		_stprintf(searchName, _T("%s\\*.%s"), strCurrentFolder, CAT_HANDPOSE_EXT);
		hFind = FindFirstFile(searchName, &find);
		if (hFind == INVALID_HANDLE_VALUE) return;

		do {
			if (find.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
				// Copy the filename and remove the extension.
				_tcscpy(fileName, find.cFileName);
				fileName[_tcslen(fileName) - _tcslen(CAT_HANDPOSE_EXT) - 1] = _T('\0');

				// We need to save the full path, so allocate some
				// memory.  If this fails, don't even bother adding
				// it to the list box.
				TCHAR *file = new TCHAR[_tcslen(strCurrentFolder) + _tcslen(find.cFileName) + 8];
				TCHAR *preset = new TCHAR[_tcslen(fileName) + 4];

				if (file && preset) {
					_stprintf(file, _T("%s\\%s"), strCurrentFolder, find.cFileName);
					_tcscpy(preset, fileName);
					id = (int)SendMessage(lbxAvailable, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)preset);
					SendMessage(lbxAvailable, LB_SETITEMDATA, id, (LPARAM)vecFileNames.size());
					vecFileNames.push_back(file);
					vecPoseNames.push_back(preset);
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

	// Load up all existing digit names into the ListBox
	void RefreshDigits()
	{
		SendMessage(lbxDigits, LB_RESETCONTENT, 0, 0);
		TSTR strDigitName;

		DigitData *digit;
		for (int i = 0; i < palm->pblock->Count(PalmTrans2::PB_DIGITDATATAB); i++)
		{
			digit = palm->GetDigit(i);
			if (digit) {
				strDigitName = digit->GetName();
				int msg = (int)SendMessage(lbxDigits, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)strDigitName.data());
				DbgAssert(msg != LB_ERR);
				UNREFERENCED_PARAMETER(msg);
			}
		}
	}

	void InitControls(HWND hDlg, PalmTrans2 *palm)
	{
		hWnd = hDlg;

		this->palm = palm;
		DbgAssert(this->palm);
		pLimb = (LimbData2*)palm->GetLimb();
		assert(pLimb);
		handposeloaded = FALSE;

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		lbxAvailable = GetDlgItem(hDlg, IDC_LIST_AVAILABLE);
		lbxDigits = GetDlgItem(hDlg, IDC_LIST_DIGITS);

		spnPoseWeight = SetupFloatSpinner(hDlg, IDC_SPIN_WEIGHT, IDC_EDIT_WEIGHT, 0.0f, 100.0f, 0.0f);

		// Initialise the SavePose Button
		btnSavePose = GetICustButton(GetDlgItem(hDlg, IDC_BTN_SAVEPOSE));
		btnSavePose->SetType(CBT_PUSH);
		btnSavePose->SetButtonDownNotify(TRUE);
		btnSavePose->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnSaveHandPose"), GetString(IDS_TT_SAVEPOSE)));
		btnSavePose->SetImage(hIcons, 6, 6, 6 + 25, 6 + 25, 24, 24);

		// Initialise the Delete Button
		btnDelete = GetICustButton(GetDlgItem(hDlg, IDC_BTN_DELETEPOSE));
		btnDelete->SetType(CBT_PUSH);
		btnDelete->SetButtonDownNotify(TRUE);
		btnDelete->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnDeleteHandPose"), GetString(IDS_TT_DELCLIP)));
		btnDelete->SetImage(hIcons, 7, 7, 7 + 25, 7 + 25, 24, 24);

		spnCurlDigits = SetupFloatSpinner(hDlg, IDC_SPIN_CURL, IDC_EDIT_CURL, -200.0f, 200.0f, 0.0f);
		spnSpreadDigits = SetupFloatSpinner(hDlg, IDC_SPIN_SPREAD, IDC_EDIT_SPREAD, -200.0f, 200.0f, 0.0f);
		spnBendDigits = SetupFloatSpinner(hDlg, IDC_SPIN_BEND, IDC_EDIT_BEND, -200.0f, 200.0f, 0.0f);
		spnRollDigits = SetupFloatSpinner(hDlg, IDC_SPIN_ROLL, IDC_EDIT_ROLL, -200.0f, 200.0f, 0.0f);

		if (!ChangeDirectory(catCfg.Get(INI_HAND_POSE_PRESET_PATH)))
			ChangeDirectory(GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));

		RefreshAvailableHandPoses();
		RefreshDigits();

		/* not part of IDD_PALM_DIGITMANAGER  - SA 10/09 (part of removed dialog)
		if(palm->TestCCFlag(CNCFLAG_RETURN_EXTRA_KEYS))
		SET_CHECKED(hDlg, IDC_DISP_EXTRA_KEYFRAMES, TRUE);
		*/

		TimeChanged(GetCOREInterface()->GetTime());
		GetCOREInterface()->RegisterTimeChangeCallback(this);
	}

	void ReleaseControls(HWND hDlg)
	{
		if (hWnd != hDlg) return;
		GetCOREInterface()->UnRegisterTimeChangeCallback(this);

		ClearFileList();

		SAFE_RELEASE_SPIN(spnPoseWeight);
		SAFE_RELEASE_SPIN(spnCurlDigits);
		SAFE_RELEASE_SPIN(spnSpreadDigits);
		SAFE_RELEASE_SPIN(spnBendDigits);
		SAFE_RELEASE_SPIN(spnRollDigits);

		// ST 1/05/04 The following buttons have
		// not been releasing before now
		SAFE_RELEASE_BTN(btnSavePose);
		SAFE_RELEASE_BTN(btnDelete);

		hWnd = NULL;

	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (PalmTrans2*)lParam);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				/* not part of IDD_PALM_DIGITMANAGER  - SA 10/09 (part of removed dialog)
				case BN_CLICKED:
				switch(LOWORD(wParam)) {
				case IDC_DISP_EXTRA_KEYFRAMES:
				palm->SetCCFlag(CNCFLAG_RETURN_EXTRA_KEYS);
				break;
				}
				break;*/
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_SAVEPOSE:
				{
					TCHAR filename[MAX_PATH];
					if (DialogSaveHandPosePreset(hInstance, GetCOREInterface()->GetMAXHWnd(), filename, MAX_PATH/*, _T("somefilename"), basePath*/)) {
						SaveHandPose(filename);
						RefreshAvailableHandPoses();
					}
					break;
				}
				case IDC_BTN_DELETEPOSE:
				{
					int item = (int)SendMessage(lbxAvailable, LB_GETCURSEL, 0, 0);
					if (item != LB_ERR) {
						int nIndex = (int)SendMessage(lbxAvailable, LB_GETITEMDATA, item, 0);
						// Dont forget that
						if (nIndex >= 0) {
							DeleteFile(vecFileNames[nIndex]);
							RefreshAvailableHandPoses();
						}
					}
					break;
				}
				}
				break;
			case LBN_SELCHANGE: {
				switch (LOWORD(wParam)) {
				case IDC_LIST_AVAILABLE:
				{
					int item = (int)SendMessage(lbxAvailable, LB_GETCURSEL, 0, 0);
					if (item != LB_ERR) {
						int nIndex = (int)SendMessage(lbxAvailable, LB_GETITEMDATA, item, 0);
						// Dont forget that
						if (nIndex >= 0) {
							LoadHandPose(vecFileNames[nIndex]);
							handposeloaded = TRUE;
						}
					}
					break;
				}
				}
				break;
			}
			case LBN_DBLCLK:
				switch (LOWORD(wParam)) {
				case IDC_LIST_AVAILABLE:
				{
					int item = (int)SendMessage(lbxAvailable, LB_GETCURSEL, 0, 0);
					if (item != LB_ERR) {
						int nIndex = (int)SendMessage(lbxAvailable, LB_GETITEMDATA, item, 0);
						if (nIndex < 0) {
							// Open the requested directory
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
							RefreshAvailableHandPoses();
						}
						else {
							// Apply the loaded hand pose preset.
							if (!theHold.Holding()) theHold.Begin();
							ApplyHandPose(1.0f);
							theHold.Accept(GetString(IDS_HLD_DIGITPOSE));
							GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
						}
					}
					break;
				}
				}
				break;
			}
		case WM_NOTIFY:
			break;
		case CC_SPINNER_BUTTONDOWN: {
			//			if(!theHold.Holding()) theHold.Begin();
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_WEIGHT:
			case IDC_SPIN_CURL:
			case IDC_SPIN_SPREAD:
			case IDC_SPIN_BEND:
			case IDC_SPIN_ROLL:
				if (!theHold.Holding()) theHold.Begin();
				break;
			}
			break;
		}

		case CC_SPINNER_CHANGE: {
			ISpinnerControl *ccSpinner = (ISpinnerControl*)lParam;
			float fSpinnerVal = ccSpinner->GetFVal();

			if (theHold.Holding()) theHold.Restore();

			if (palm->GetCATParentTrans()->GetLengthAxis() == Z) {
				switch (LOWORD(wParam)) { // Switch on ID
				case IDC_SPIN_WEIGHT:	if (handposeloaded) ApplyHandPose(fSpinnerVal / 100.0f);		break;
				case IDC_SPIN_SPREAD:	ModifyDigits(AngAxis(Point3(0.0f, 1.0f, 0.0f), -DegToRad(fSpinnerVal)), TRUE, TRUE);		break;
				case IDC_SPIN_CURL:		ModifyDigits(AngAxis(Point3(1.0f, 0.0f, 0.0f), -DegToRad(fSpinnerVal)), FALSE);				break;
				case IDC_SPIN_BEND:		ModifyDigits(AngAxis(Point3(1.0f, 0.0f, 0.0f), -DegToRad(fSpinnerVal)), TRUE);				break;
				case IDC_SPIN_ROLL:		ModifyDigits(AngAxis(Point3(0.0f, 0.0f, 1.0f), -DegToRad(fSpinnerVal)), TRUE, TRUE);				break;
				}
			}
			else {
				switch (LOWORD(wParam)) { // Switch on ID
				case IDC_SPIN_WEIGHT:	if (handposeloaded) ApplyHandPose(fSpinnerVal / 100.0f);		break;
				case IDC_SPIN_SPREAD:	ModifyDigits(AngAxis(Point3(0.0f, 1.0f, 0.0f), -DegToRad(fSpinnerVal)), TRUE, TRUE);		break;
				case IDC_SPIN_CURL:		ModifyDigits(AngAxis(Point3(0.0f, 0.0f, -1.0f), -DegToRad(fSpinnerVal)), FALSE);				break;
				case IDC_SPIN_BEND:		ModifyDigits(AngAxis(Point3(0.0f, 0.0f, -1.0f), -DegToRad(fSpinnerVal)), TRUE);				break;
				case IDC_SPIN_ROLL:		ModifyDigits(AngAxis(Point3(-1.0f, 0.0f, 0.0f), -DegToRad(fSpinnerVal)), TRUE, TRUE);				break;
				}
			}

			GetCOREInterface()->RedrawViews(t);
			break;
		}
		case CC_SPINNER_BUTTONUP: {

			if (theHold.Holding()) {
				if (HIWORD(wParam)) theHold.Accept(GetString(IDS_HLD_DIGITS));
				else					   theHold.Cancel();
			}

			//	Reset all the spinners when we have finnished with them
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_WEIGHT:	spnPoseWeight->SetValue(0.0f, FALSE);	break;
			case IDC_SPIN_CURL:	spnCurlDigits->SetValue(0.0f, FALSE);	break;
			case IDC_SPIN_SPREAD:	spnSpreadDigits->SetValue(0.0f, FALSE);	break;
			case IDC_SPIN_BEND:		spnBendDigits->SetValue(0.0f, FALSE);	break;
			case IDC_SPIN_ROLL:		spnRollDigits->SetValue(0.0f, FALSE);	break;
			}
			break;
		}
		case WM_CUSTEDIT_ENTER:
			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_DIGITS));
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_WEIGHT:	spnPoseWeight->SetValue(0.0f, FALSE);	break;
			case IDC_EDIT_CURL:	spnCurlDigits->SetValue(0.0f, FALSE);	break;
			case IDC_EDIT_SPREAD:	spnSpreadDigits->SetValue(0.0f, FALSE);	break;
			case IDC_EDIT_BEND:		spnBendDigits->SetValue(0.0f, FALSE);	break;
			case IDC_EDIT_ROLL:		spnRollDigits->SetValue(0.0f, FALSE);	break;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	void TimeChanged(TimeValue)
	{
	}

	virtual void DeleteThis() { }//delete this; }
};

static INT_PTR CALLBACK PalmObjRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class PalmObjRolloutData
{
	HWND hDlg;
	ICATObject* iobj;
	PalmTrans2* palm;

	ICustEdit	*edtName;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;

	ICustButton *btnAddArbBone;
	ICustButton *btnAddERN;

	ISpinnerControl *ccSpinLength;
	ISpinnerControl *ccSpinWidth;
	ISpinnerControl *ccSpinDepth;

	ISpinnerControl *ccSpinNumDigits;
	HWND lbxDigits;

public:

	PalmObjRolloutData() : hDlg(NULL), iobj(NULL), palm(NULL)
	{
		edtName = NULL;

		btnCopy = NULL;
		btnPaste = NULL;
		btnPasteMirrored = NULL;

		btnAddArbBone = NULL;

		lbxDigits = NULL;
		ccSpinLength = NULL;
		ccSpinWidth = NULL;
		ccSpinDepth = NULL;

		ccSpinNumDigits = NULL;
		btnAddERN = NULL;
	}

	HWND GetHWnd() { return hDlg; }

	BOOL InitControls(HWND hWnd, PalmTrans2* PlmTrans)
	{
		palm = (PalmTrans2*)PlmTrans;
		if (!palm) return FALSE;
		iobj = palm->GetICATObject();
		if (!iobj) return FALSE;

		hDlg = hWnd;

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		edtName = GetICustEdit(GetDlgItem(hDlg, IDC_EDIT_NAME));
		edtName->SetText(palm->GetName().data());

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyBone"), GetString(IDS_TT_COPYBONE)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteBone"), GetString(IDS_TT_PASTERIG)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		// btnPasteMirrored button
		btnPasteMirrored = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE_MIRRORED));
		btnPasteMirrored->SetType(CBT_PUSH);
		btnPasteMirrored->SetButtonDownNotify(TRUE);
		btnPasteMirrored->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteBoneMirrored"), GetString(IDS_TT_PASTERIGMIRROR)));
		btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

		if (iobj->CustomMeshAvailable()) {
			SET_CHECKED(hDlg, IDC_CHK_USECUSTOMMESH, iobj->UsingCustomMesh());
		}
		else SendMessage(GetDlgItem(hDlg, IDC_CHK_USECUSTOMMESH), WM_ENABLE, (WPARAM)FALSE, 0);

		// btnAddArbBone button
		btnAddArbBone = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDARBBONE));	btnAddArbBone->SetType(CBT_PUSH);	btnAddArbBone->SetButtonDownNotify(TRUE);
		btnAddArbBone->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddExtraBone"), GetString(IDS_TT_ADDBONE)));

		btnAddERN = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADDRIGGING));		btnAddERN->SetType(CBT_CHECK);		btnAddERN->SetButtonDownNotify(TRUE);
		btnAddERN->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddERN"), GetString(IDS_TT_ADDOBJS)));

		ccSpinLength = SetupFloatSpinner(hDlg, IDC_SPIN_LENGTH, IDC_EDIT_LENGTH, 0.0f, 100.0f, iobj->GetZ());
		ccSpinWidth = SetupFloatSpinner(hDlg, IDC_SPIN_WIDTH2, IDC_EDIT_WIDTH, 0.0f, 100.0f, iobj->GetX()*2.0f);
		ccSpinDepth = SetupFloatSpinner(hDlg, IDC_SPIN_HEIGHT, IDC_EDIT_HEIGHT, 0.0f, 100.0f, iobj->GetY()*2.0f);

		ccSpinNumDigits = SetupIntSpinner(hDlg, IDC_SPIN_NUMDIGITS, IDC_EDIT_NUMDIGITS, 0, 100, palm->GetNumDigits());
		lbxDigits = GetDlgItem(hDlg, IDC_LIST_DIGITS);
		EnableWindow(lbxDigits, FALSE);

		if (palm->GetCATParentTrans()->GetCATMode() != SETUPMODE) {
			btnCopy->Disable();
			btnPaste->Disable();
			btnPasteMirrored->Disable();
			btnAddArbBone->Disable();
		}
		else if (!palm->CanPasteControl())
		{
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}

		RefreshDigitsList();
		SetFocus(GetDlgItem(hWnd, IDC_BTN_ADDARBBONE));

		return TRUE;
	}
	void Update()
	{
		ccSpinLength->SetValue(iobj->GetZ(), FALSE);
		ccSpinWidth->SetValue(iobj->GetX()*2.0f, FALSE);
		ccSpinDepth->SetValue(iobj->GetY()*2.0f, FALSE);

		ccSpinNumDigits->SetValue(palm->GetNumDigits(), FALSE);
		RefreshDigitsList();
	}
	void ReleaseControls(HWND hWnd) {
		UNUSED_PARAM(hWnd);
		DbgAssert(hDlg == hWnd);
		SAFE_RELEASE_EDIT(edtName);

		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnPasteMirrored);

		SAFE_RELEASE_BTN(btnAddArbBone);
		SAFE_RELEASE_BTN(btnAddERN);
		ExtraRigNodes *ern = (ExtraRigNodes*)palm->GetInterface(I_EXTRARIGNODES_FP);
		DbgAssert(ern);
		ern->IDestroyERNWindow();

		SAFE_RELEASE_SPIN(ccSpinLength);
		SAFE_RELEASE_SPIN(ccSpinWidth);
		SAFE_RELEASE_SPIN(ccSpinDepth);

		SAFE_RELEASE_SPIN(ccSpinNumDigits);

		hDlg = NULL;
		palm = NULL;
		iobj = NULL;
	}

	// Load up all existing digit names into the ListBox
	void RefreshDigitsList()
	{
		SendMessage(lbxDigits, LB_RESETCONTENT, 0, 0);
		TSTR strDigitName;

		DigitData *digit;
		for (int i = 0; i < palm->pblock->Count(PalmTrans2::PB_DIGITDATATAB); i++)
		{
			digit = palm->GetDigit(i);
			if (digit) {
				strDigitName = digit->GetName();
				int msg = (int)SendMessage(lbxDigits, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)strDigitName.data());
				DbgAssert(msg != LB_ERR);
				UNREFERENCED_PARAMETER(msg);
			}
		}
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TimeValue t = GetCOREInterface()->GetTime();

		switch (msg) {
		case WM_INITDIALOG:
			if (!InitControls(hWnd, (PalmTrans2*)lParam)) DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_USECUSTOMMESH:	if (iobj) iobj->SetUseCustomMesh(IS_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH));	break;
				}
				break;
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_COPY:				CATControl::SetPasteControl(palm);											break;
				case IDC_BTN_PASTE:				 palm->PasteFromCtrl(CATControl::GetPasteControl(), false);	break;
				case IDC_BTN_PASTE_MIRRORED:		palm->PasteFromCtrl(CATControl::GetPasteControl(), true);		break;
				case IDC_BTN_ADDARBBONE:
				{
					HoldActions actions(IDS_HLD_ADDBONE);
					palm->AddArbBone();
					break;
				}
				case IDC_BTN_ADDRIGGING: {
					ExtraRigNodes* ern = (ExtraRigNodes*)palm->GetInterface(I_EXTRARIGNODES_FP);
					DbgAssert(ern);
					if (btnAddERN->IsChecked())
						ern->ICreateERNWindow(hWnd);
					else ern->IDestroyERNWindow();
					break;
				}
				}
				break;
			}
			break;
		case CC_SPINNER_BUTTONDOWN:
			if (!theHold.Holding())
				theHold.Begin();
			theHold.Put(new SetObjDimRestoreUndo(palm));
			break;
		case CC_SPINNER_CHANGE: {
			ISpinnerControl *ccSpinner = (ISpinnerControl*)lParam;
			float fSpinnerVal = ccSpinner->GetFVal();
			ICATObject* iobj = palm->GetICATObject();
			switch (LOWORD(wParam)) {
			case IDC_SPIN_LENGTH:
				iobj->SetZ(fSpinnerVal);
				palm->UpdateObjDim();
				palm->Update();
				break;
			case IDC_SPIN_WIDTH2:
				iobj->SetX(fSpinnerVal / 2.0f);
				palm->UpdateObjDim();
				palm->Update();
				break;
			case IDC_SPIN_HEIGHT:
				iobj->SetY(fSpinnerVal / 2.0f);
				palm->UpdateObjDim();
				palm->Update();
				break;
			case IDC_SPIN_NUMDIGITS:
				int numDigits = ccSpinner->GetIVal();
				palm->SetNumDigits(numDigits);
				RefreshDigitsList();
				break;
			}
			GetCOREInterface()->RedrawViews(t);
			break;
		}
		break;
		case CC_SPINNER_BUTTONUP:
			if (theHold.Holding()) {
				if (HIWORD(wParam)) {
					theHold.Put(new SetObjDimRestoreRedo(palm));
					theHold.Accept(GetString(IDS_HLD_PALMOBJEDIT));
				}
				else					   theHold.Cancel();
			}
			break;
		case WM_CUSTEDIT_ENTER:
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_NAME: {
				TCHAR strbuf[128];
				edtName->GetText(strbuf, 64);// max 64 characters
				TSTR name(strbuf);
				palm->SetName(name);
				break;
			}
			default:
				if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_PALMOBJEDIT));
				break;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	virtual void DeleteThis() { }//delete this; }
};

// Declare a static global instance of the PalmObj rollout data,
// so that the clip root can use it in EndEditParams() to
// delete the rollout (instead of having to remember the HWND
// returned by Interface::AddRollupPage().  Making it static
// of course means that there can only ever be one clip rollout
// active at a time.
static PalmObjRolloutData staticPalmObjRollout;

// This is the actual callback that just calls the class method
// of the dialog callback function.
//
static INT_PTR CALLBACK PalmObjRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticPalmObjRollout.DlgProc(hWnd, message, wParam, lParam);
};

static PalmMotionParamDlgCallBack PalmMotionParamsCallback;
static INT_PTR CALLBACK PalmMotionParamsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return PalmMotionParamsCallback.DlgProc(hWnd, message, wParam, lParam);
};

class PalmIKParamsDlgProc : public ParamMap2UserDlgProc
{
	INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND, UINT msg, WPARAM, LPARAM)
	{
		switch (msg)
		{
		case WM_INITDIALOG:
		{
			PalmTrans2* pPalm = dynamic_cast<PalmTrans2*>(map->GetParamBlock()->GetOwner());
			DbgAssert(pPalm != NULL);

			LimbData2* pLimb = pPalm->GetLimb();
			if (pLimb == NULL)
				break;

			// Enable target align if we have an IK target only
			map->Enable(PalmTrans2::PB_LAYERTARGETALIGN, pLimb->GetIKTarget() != NULL);
		}
		break;
		}
		return FALSE;
	}

	void DeleteThis(void) {};
};

static PalmIKParamsDlgProc palmIKDlgProc;

static ParamBlockDesc2 PalmTrans2_t_param_blk(PalmTrans2::pb_idParams, _T("Palm Params"), 0, &PalmTrans2Desc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, PalmTrans2::PALMPB_REF,
	// rollout
	IDD_PALM_IK, IDS_PALM_ANIM, 0, 0, &palmIKDlgProc,
	// params
	PalmTrans2::PB_LIMBDATA, _T("Limb"), TYPE_REFTARG, 0, IDS_CL_LIMBDATA,
		p_end,
	PalmTrans2::PB_NODE_DEPRECATED, _T(""), TYPE_INODE, P_OBSOLETE | P_NO_REF, 0,
		p_end,
	PalmTrans2::PB_OBJPB_DEPRECATED, _T(""), TYPE_REFTARG, P_OBSOLETE, NULL,
		p_end,
	PalmTrans2::PB_SCALE_DEPRECATED, _T(""), TYPE_POINT3, P_OBSOLETE | P_ANIMATABLE, IDS_SCALE,
		p_end,
	PalmTrans2::PB_IKNODE_DEPRECATED, _T(""), TYPE_INODE, P_OBSOLETE, IDS_IKNODE,
		p_end,
	PalmTrans2::PB_IKFKRATIO_DEPRECATED, _T(""), TYPE_FLOAT, P_OBSOLETE | P_ANIMATABLE, IDS_IKFK,
		p_end,
	PalmTrans2::PB_SETUPTARGETALIGN_DEPRECATED, _T(""), TYPE_FLOAT, P_OBSOLETE, NULL,
		p_end,
	PalmTrans2::PB_LAYERTARGETALIGN, _T(""), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_LAYERTARGETALIGN,
		p_default, 0.0f,
		p_range, 0.0f, 1.0f,
		p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_TARGETALIGN, IDC_SLIDER_LAYERTARGETALIGN, 5,
		p_end,
	PalmTrans2::PB_TMSETUP_DEPRECATED, _T(""), TYPE_MATRIX3, P_OBSOLETE | P_RESET_DEFAULT, NULL,
		p_end,

	PalmTrans2::PB_CATDIGITBEND, _T(""), TYPE_POINT3, P_ANIMATABLE, IDS_CATDIGITBEND,
		p_end,
	PalmTrans2::PB_NUMDIGITS, _T("NumDigits"), TYPE_INT, 0, IDS_NUMFINGERS,
		p_default, 0,
		p_end,
	PalmTrans2::PB_DIGITDATATAB, _T("Digits"), TYPE_REFTARG_TAB, 0, P_NO_REF + P_VARIABLE_SIZE, 0,	// (Requires) intial size of tab
		p_end,
	PalmTrans2::PB_SYMPALM, _T(""), TYPE_REFTARG, P_NO_REF, NULL,
		p_end,
	PalmTrans2::PB_NAME, _T(""), TYPE_STRING, P_RESET_DEFAULT, NULL,
		p_default, _T("Palm"),
		p_end,
	p_end
);

// This switch will tell us whether or not we are
// currently in the middle of a load. If we are,
// then when creating fingers do NOT give them any
// default
static BOOL isLoading = FALSE;

PalmTrans2::PalmTrans2(BOOL loading)
{
	layerIKTargetTrans = NULL;
	mLayerTrans = NULL;
	weights = NULL;

	pblock = NULL;
	ipbegin = NULL;

	validFK = validIK = NEVER;

	// Palm defaults to being locked onto the pLimb
	ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	//	ccflags |= CNCFLAG_LOCK_SETUPMODE_LOCAL_POS;
	// If you are in FK we jump to IK anyway
	// but not in setupmode
	ccflags |= CCFLAG_EFFECT_HIERARCHY;

	ikpivotoffset = Point3(0.0f, 0.0f, 0.0f);;

	// Always create new parameter block
	PalmTrans2Desc.MakeAutoParamBlocks(this);
	pblock->ResetAll(TRUE, FALSE);

	//
	//	If we're loading then we don't want to make new default controllers
	//	as they already exist (in the file).  Max will hook them up later
	//  for us. (probably using SetReference())
	//
	if (!loading)
	{
	}
}

RefTargetHandle PalmTrans2::Clone(RemapDir& remap)
{
	// make a new PalmTrans2 object to be the clone
	// call true for loading so the new PalmTrans2 doesn't
	// make new default subcontrollers.
	PalmTrans2 *newPalmTrans2 = new PalmTrans2(TRUE);

	// We add this entry now so that
	remap.AddEntry(this, newPalmTrans2);

	// clone our subcontrollers and assign them to the new object.
	newPalmTrans2->ReplaceReference(PALMPB_REF, CloneParamBlock(pblock, remap));

	if (weights)			newPalmTrans2->ReplaceReference(WEIGHTS, remap.CloneRef(weights));
	if (mLayerTrans)			newPalmTrans2->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));
	//	if (pTargetAlign)	newPalmTrans2->ReplaceReference(LAYERIKTM, remap.CloneRef(pTargetAlign));

	CATNodeControl::CloneCATNodeControl(newPalmTrans2, remap);

	//	remap.PatchPointer((RefTargetHandle*)&newPalmTrans2->pLimb, (RefTargetHandle)pLimb);

	for (int i = 0; i < GetNumDigits(); i++) {
		newPalmTrans2->pblock->SetValue(PB_DIGITDATATAB, 0, remap.CloneRef(GetDigit(i)), i);
		newPalmTrans2->GetDigit(i)->SetPalm(newPalmTrans2);
	}

	BaseClone(this, newPalmTrans2, remap);

	// now return the new object.
	return newPalmTrans2;
}

PalmTrans2::~PalmTrans2()
{
	DeleteAllRefs();
}

void PalmTrans2::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		PalmTrans2 *newctrl = (PalmTrans2*)from;

		if (newctrl->pblock)
			ReplaceReference(PALMPB_REF, newctrl->pblock);
		//		if (newctrl->layerIKTargetTrans)
		//			ReplaceReference(LAYERIKTM, newctrl->layerIKTargetTrans);
		if (newctrl->mLayerTrans)
			ReplaceReference(LAYERTRANS, newctrl->mLayerTrans);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void PalmTrans2::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	LimbData2* pLimb = (LimbData2*)GetLimb();
	if (pLimb == NULL)
		return;

	Interval iv;
	float dIKFKRatio = pLimb->GetIKFKRatio(t, iv);

	SetXFormPacket* pXform = (SetXFormPacket*)val;

	// If we are in
	bool bKeyFIK = ((dIKFKRatio > 0.5f) && (pXform->command == XFORM_MOVE));
	if (bKeyFIK)
	{
		Matrix3 tmCurrent = GetNodeTM(t);
		Matrix3 tmIKTarget = tmCurrent;
		Point3 p3Pos = tmCurrent.GetTrans() + pXform->tmAxis.VectorTransform(pXform->p);
		tmIKTarget.SetTrans(p3Pos);

		pLimb->SetFlag(LIMBFLAG_LOCKED_IK);
		pLimb->SetTemporaryIKTM(tmIKTarget, tmCurrent);
		pLimb->KeyLimbBones(t, false);
		pLimb->ClearFlag(LIMBFLAG_LOCKED_IK);

		// Ensure we maintain the current orientation
		// The position part of this SetValue is discarded
		SetNodeTM(t, tmCurrent);
	}
	else
		CATNodeControl::SetValue(t, val, commit, method);
}

void PalmTrans2::ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid)
{
	Point3 p3Position = tmWorldTransform.GetTrans();
	CATNodeControl::ApplySetupOffset(t, tmOrigParent, tmWorldTransform, p3BoneLocalScale, ivValid);

	// Reset the position (we can't move from the arm)
	tmWorldTransform.SetTrans(p3Position);
}

void PalmTrans2::CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid)
{
	LimbData2* pLimb = GetLimb();
	if (!pLimb)
		return;

	int iNumBones = pLimb->GetNumBones();
	float dIKFKRatio = pLimb->GetIKFKRatio(t, ivLocalValid, iNumBones);

	// Ensure we don't move from the end of the palm
	Point3 p3CurrentPos = tmWorld.GetTrans();

	if (pLimb->TestFlag(LIMBFLAG_LOCKED_IK))
	{
		// Just return the current IKTM
		tmWorld = pLimb->GettmIKTarget(t, ivLocalValid);
		tmWorld.SetTrans(p3CurrentPos);
		return;
	}

	GetFKValue(t, ivLocalValid, tmParent, tmWorld, p3LocalScale);
	// Remove any position offset our FK solution may have added.
	tmWorld.SetTrans(p3CurrentPos);

	// Apply our IK retargetting.
	if (dIKFKRatio < 1.0f)
	{
		// Cache FK result
		Matrix3 tmFK = tmWorld;

		// First, blend our IK Target if necessary
		Point3 p3IKTarget = tmPalmTarget.GetTrans();
		if (dIKFKRatio > 0)
			p3IKTarget = BlendVector(tmPalmTarget.GetTrans(), pLimb->GetBoneFKPosition(t, iNumBones + 2), dIKFKRatio);

		// Get IK directly, no need to cache.
		GetIKValue(t, p3IKTarget, tmFK, tmWorld, p3LocalScale, ivLocalValid);

		// If our IK position is being blended down this bone, blend off the effect of the IK
		float limbIKPos = pLimb->GetLimbIKPos(t, ivLocalValid);
		if (limbIKPos > iNumBones && limbIKPos < (iNumBones + 1))
		{
			float blendRatio = limbIKPos - iNumBones;
			BlendRot(tmWorld, tmFK, 1.0f - blendRatio);
		}
	}
}

void PalmTrans2::SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method)
{
	// IF:	We are FK, don't change anything
	//		In IK - calculate position relative to the IKTarget
	// If we are in locked IK, or keying freeform, the values
	// we calculate are the FK ones.
	LimbData2* pLimb = GetLimb();
	CATClipRoot* pRoot = mLayerTrans->GetRoot();

	// Shouldn't happen
	DbgAssert(pLimb != NULL && pRoot != NULL);
	if (pLimb == NULL || pRoot == NULL)
		return;

	// Only do the reverse IK parenting for the leg.
	// An arm will always use the IKTarget as its parent
	BOOL bDoIKSet = pLimb->GetisLeg();
	if (bDoIKSet)
	{
		Interval iv;
		float dIKFKRatio = pLimb->GetIKFKRatio(t, iv);
		if (dIKFKRatio < 1.0f)
		{
			Matrix3 tmIKTarget = pLimb->GettmIKTarget(t, iv);
			switch (packet->command)
			{
			case XFORM_SET:
			{
				// Find the tip of the ankle
				CATNodeControl::ApplyChildOffset(t, packet->tmAxis);

				// Find this point relative to the IK target
				Matrix3 tmRelToIKTarget = packet->tmAxis * Inverse(tmIKTarget);

				// Blend the parent to blah based on target align
				BlendRot(packet->tmParent, tmIKTarget, GetTargetAlign(t, iv));

				// Now (HACK) set the requested pos to be this position applied
				// to the requested parent.
				Point3 p3IKTargetFromParent = tmRelToIKTarget.GetTrans();
				p3IKTargetFromParent = packet->tmParent.PointTransform(p3IKTargetFromParent);
				packet->tmAxis.SetTrans(p3IKTargetFromParent);
			}
			break;
			case XFORM_MOVE:
				//		if (pLimb->GetisLeg())
				//			packet->tmParent = pLimb->GettmIKTarget(t, iv);
				//		break;
			case XFORM_ROTATE:
			{
				// When we are blending the IK position down this bone,
				// we blend the parent to the IK target, and then back
				// again as we blend down the next bone.  This means that
				// the first FK bone in an IK chain is relative to the IK
				// target.
				Interval ivWhoCares;

				// If we are here, our IK ratio == 0.  We only
				// want to perform this blend if we would be blending to tmIKTarget
				float realIKFKRatio = pLimb->GetIKFKRatio(t, ivWhoCares);
				realIKFKRatio += (1.0f - realIKFKRatio) * (1.0f - GetTargetAlign(t, ivWhoCares));
				if (realIKFKRatio < 1.0f)
				{
					// Modulate myIKPos by realIKFK ratio.  This prevents
					// popping when blending IK on with IK pos != NumBones + 1
					float myIKPos = pLimb->GetLimbIKPos(t, ivWhoCares);
					myIKPos -= pLimb->GetNumBones();
					myIKPos *= realIKFKRatio;

					if (myIKPos >= 0 && myIKPos < 1.0f)
					{
						// If IK pos is in the middle of the palm, blend
						// the parent to be the IK target.
						// This is because, when we are the first non-IK
						// bone, we want to have a constant parent.  If we
						// just blended to FK, our bone would keep on inheriting
						// all the transforms from the hub.  This would mean that
						// the target for this work flow (crawling along the floor)
						// would be virtually impossible, because you need to counter-
						// rotate to keep your transform constant vs the floor
						Matrix3 tmIKTargetWorld = pLimb->GettmIKTarget(t, ivWhoCares);
						BlendRot(packet->tmParent, tmIKTargetWorld, 1.0f - myIKPos);
					}
					else if (myIKPos < 0.0f && myIKPos > -1.0f)
					{
						// For the sake of blending, when the IK position travels
						// through the palm and back into the last limb bone,
						// we need to reset the parent of the palm back to being the
						// last limb bone (at the base of the palm, the parent is the
						// IKTarget).
						Matrix3 tmIKTargetWorld = pLimb->GettmIKTarget(t, ivWhoCares);
						BlendRot(packet->tmParent, tmIKTargetWorld, (1.0f + myIKPos));
					}
				}
			}
			break;
			}
		}
	}
	CATNodeControl::SetWorldTransform(t, tmOrigParent, p3ParentScale, packet, commit, method);
}

void PalmTrans2::ModifyIKTMParent(TimeValue t, Matrix3 tmIKTarget, Matrix3& tmParent)
{
	// In IK mode, our parent rotation blends from our Max parent to the IKNode
	Interval iv;
	float dTargetAlign = GetTargetAlign(t, iv);
	if (dTargetAlign > 0)
	{
		// Get our IKTM transform. Align to it
		BlendMat(tmParent, tmIKTarget, dTargetAlign);
	}
	if (GetLimb()->GetisLeg())
		tmParent.SetTrans(tmIKTarget.GetTrans());
}

void PalmTrans2::GetFKValue(TimeValue t, Interval& ivLocalValid, const Matrix3& tmParent, Matrix3& tmWorld, Point3& p3LocalScale)
{
	//if (validFK.InInterval(t))
	{
		//	tmWorld = FKTM;
		//	valid &= validFK;
		//	return;
	}

	CATNodeControl::CalcWorldTransform(t, tmParent, P3_IDENTITY_SCALE, tmWorld, p3LocalScale, ivLocalValid);
	validFK = ivLocalValid;
}

void PalmTrans2::GetIKValue(TimeValue t, const Point3& p3PalmTarget, const Matrix3& tmFKWorld, Matrix3& tmWorld, Point3& /*p3LocalScale*/, Interval& ivLocalValid)
{
	/************************************************************************/
	/* This does a look-at for the palm. One difference is that we are      */
	/* trying to preserve our target matrix so this is actually the target  */
	/* looking at the ankle pos									            */
	/************************************************************************/

	Point3 p3AnklePos = tmWorld.GetTrans();
	int lengthaxis = GetLengthAxis();
	float dTargetAlign = GetTargetAlign(t, ivLocalValid);

	// Blend off target align (IK only feature) as we blend to FK mode
	ILimb* pLimb = GetLimb();
	if (pLimb)
		dTargetAlign *= 1.0f - pLimb->GetIKFKRatio(t, ivLocalValid);

	Matrix3 tmIKWorld = tmPalmTarget;
	// Remove scale from the IK calculation.  Any scale will be re-applied in
	// CATNodeControl::GetValue.
	tmIKWorld.NoScale();
	BlendRot(tmIKWorld, tmFKWorld, 1.0f - dTargetAlign);

	// Always look directly at the IK target.
	Point3 p3ZVect = FNormalize(p3PalmTarget - tmWorld.GetTrans());
	float boneAngle = (float)acos(min(DotProd(p3ZVect, Normalize(tmIKWorld.GetRow(lengthaxis))), 1.0f));
	Point3 fboneAxis = Normalize(CrossProd(p3ZVect, tmIKWorld.GetRow(lengthaxis)));

	RotateMatrix(tmIKWorld, AngAxis(fboneAxis, boneAngle));	// Rotation is in world space

	tmWorld = tmIKWorld;
	tmWorld.SetTrans(p3AnklePos);
}

void PalmTrans2::ModifyFKTM(TimeValue t, Matrix3 &tmFKTarget)
{
	Interval ivValid;
	Matrix3 tmChildFK = tmFKTarget;
	CATNodeControl::ApplyChildOffset(t, tmChildFK);
	BlendMat(tmFKTarget, tmChildFK, (1.0f - GetTargetAlign(t, ivValid)));
}

//  What this function does:
//	First - understand the CAT leg, and Target Align.
//	When Target Align == 0, the Ankle evaluates in FK relative to the last bone:
//	When Target Algin == 1, The ankle evaluates in FK relative to the Ankle:
//	In All cases, the Ankle evaluates so that its FK solution terminates precisely
//	on the IKTarget.  So, we are evaluating in FK in a way that ensures our IK is valid.
//	We do this by creating IK parameters that evaluates to our desired FK result!
//	This function calculates the IK Target that
void PalmTrans2::ModifyIKTM(TimeValue t, Interval &iktargetvalid, const Matrix3& tmFKLocal, Matrix3 &tmIKTarget)
{
	int isUndoing = 0;
	if (IsEvaluationBlocked() && (!theHold.Restoring(isUndoing) && isUndoing)) return;

	LimbData2* pLimb = GetLimb();
	if (!pLimb) return;

	float dTargetAlign = GetTargetAlign(t, iktargetvalid);

	// Find our (FK) offset from our parent, which could be either
	// our parent pLimb bone, or the IK target, depending on dTargetAlign.
	Matrix3 tmPalmLocal = tmFKLocal;

	// Find our position, relative to teh IK target.  This position
	// is the IK Goal of the system (tho not necessarily the limb, depending on dTargetAlign)
	Matrix3 tmPalmRelToIKTarget = CalcPalmRelToIKTarget(t, iktargetvalid, tmIKTarget);
	Point3 p3IkTargPos = tmPalmRelToIKTarget.GetTrans();

	BlendMat(tmPalmLocal, tmPalmRelToIKTarget, dTargetAlign);
	tmPalmLocal.SetTrans(p3IkTargPos);

	//////////////////////////////////
	// calculate extra length
	float palmlength = GetBoneLength();
	float dPivotShiftDist = palmlength * dTargetAlign;

	////////////////////////////////////
	// Calculate the IKTargets

	// tmPalmLocal includes position offsets from tmSetup. We apply this to ankles to lift them off the iktarget
	Matrix3 temp = tmIKTarget;
	if (pLimb->GetisLeg())
		temp = tmPalmLocal * temp;

	// The tm is now at the tip of the ankle or palm. Our GetIKValue will aim the ankle at this transform
	tmPalmTarget = temp;

	// Now we apply a small offset in the coodinate space of the ankle
	tmPalmTarget.PreTranslate(ikpivotoffset * GetCATUnits());

	// Now move the ik target up to the anklejoint or writst
	temp.PreTranslate(FloatToVec(-dPivotShiftDist, GetLengthAxis()));
	tmIKTarget.SetTrans(temp.GetTrans());
}

float PalmTrans2::GetExtensionLength(TimeValue t, const Matrix3& tmPalmFKLocal)
{
	LimbData2* pLimb = GetLimb();
	if (!pLimb)
		return 0.0f;

	Interval ivValid = FOREVER;
	// Extra length to be added to the last pLimb...
	float dTargetAlign = GetTargetAlign(t, ivValid);
	if (dTargetAlign < 1.0f)
	{
		float palmlength = GetBoneLength();
		int lengthaxis = GetLengthAxis();
		float localAngle = (float)acos(min(1.0, max(-1.0, tmPalmFKLocal.GetRow(lengthaxis)[lengthaxis])));

		float lastbonelength = pLimb->GetBoneData(pLimb->GetNumBones() - 1)->GetBoneLength();
		// Cosine Rule, this will return the length of the line between the start of the last leg bone and the
		// end of the ankle given that the angle between them  is local angle. To ensure this angle is
		// worked out by the IK system, set the length of the last bone to be the length worked out here
		// by adding on the extra lenght (this length - lastBoneLength)
		float dExtraLength = (float)sqrt(sq(palmlength) + sq(lastbonelength) - 2.0f*palmlength*lastbonelength*cos((float)M_PI - localAngle)) - lastbonelength;
		dExtraLength *= (1.0f - dTargetAlign);
		return dExtraLength;
	}
	return 0.0f;
}

Matrix3 PalmTrans2::CalcPalmRelToIKTarget(TimeValue t, Interval& ivValid, const Matrix3& tmIKTarget)
{
	Matrix3 tmPalmRelToIKTarget = tmIKTarget;
	// This should technically be a complete GetValue, but sans parent offset...
	int iCATMode = GetCATMode();
	Point3 p3Scale(1, 1, 1);
	if (iCATMode == SETUPMODE || mLayerTrans->TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE))
		mLayerTrans->ApplySetupVal(t, tmIKTarget, tmPalmRelToIKTarget, p3Scale, ivValid);
	if (iCATMode != SETUPMODE)
		mLayerTrans->GetTransformation(t, tmIKTarget, tmPalmRelToIKTarget, ivValid, p3Scale, p3Scale); // Ignore scale, because it doesn't affect positions.

	return tmPalmRelToIKTarget * Inverse(tmIKTarget);
}

RefTargetHandle PalmTrans2::GetReference(int i)
{
	switch (i)
	{
	case PALMPB_REF:		return pblock;
	case LAYERIKTM:			return layerIKTargetTrans;
	case LAYERTRANS:		return mLayerTrans;
	case WEIGHTS:			return (Control*)weights;
	default:				return NULL;
	}
}

void PalmTrans2::SetReference(int i, RefTargetHandle rtarg)
{   // RefTargetHandle is just
	switch (i)
	{
	case PALMPB_REF:		pblock = (IParamBlock2*)rtarg;					break;
	case LAYERIKTM:			layerIKTargetTrans = (CATClipMatrix3*)rtarg;	break;
	case LAYERTRANS:		mLayerTrans = (CATClipMatrix3*)rtarg;			break;
	case WEIGHTS:			weights = (CATClipWeights*)rtarg; break;
	}
}

Animatable* PalmTrans2::SubAnim(int i)
{
	switch (i)
	{
	case SUBLIMB:		return GetLimb();
	case SUBROTATION:	return mLayerTrans;
	case SUBTARGETALIGN:return GetLayerTargetAlign();
	default:			return NULL;
	}
}

TSTR PalmTrans2::SubAnimName(int i)
{
	switch (i)
	{
	case SUBLIMB:			return GetString(IDS_LIMBDATA);
	case SUBROTATION:		return GetString(IDS_LAYERTRANS);
	case SUBTARGETALIGN:	return GetString(IDS_LAYERTARGETALIGN);
	default:				return _T("");
	}
}

RefResult PalmTrans2::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
	{
		if (pblock == hTarg)
		{
			int tabIndex = 0;
			ParamID index = pblock->LastNotifyParamID(tabIndex);
			switch (index)
			{
			case PB_NUMDIGITS:
				if (!theHold.RestoreOrRedoing())
					CreateDigits();
				break;
			case PB_LAYERTARGETALIGN:
			{
				LimbData2* pLimb = GetLimb();
				if (pLimb)
					pLimb->UpdateLimb();
			}
			break;
			case PB_NAME:
				UpdateName();
				break;
			}
		}
		// Added the check for PART_OBJ to fix issue http://mejira.autodesk.com/browse/SIMC-719
		// ReactorFloat was spamming out REFMSG_CHANGE messages on every evaluation which
		// would invalidate our solution, causing us to query again (repeat).
		// Oddly though, it was only sending messages with PART_OBJ, so this
		// seemed like a pretty safe filter (we never care if only an object is changing).
		else if (hTarg == mLayerTrans && (partID != PART_OBJ))
		{
			validFK.SetEmpty();
			InvalidateTransform();

			if (!IsEvaluationBlocked())
			{
				BlockEvaluation block(this);
				LimbData2* pLimb = GetLimb();
				if (pLimb == NULL)
					return REF_DONTCARE;
				pLimb->UpdateLimb();
				int data = 0;
				pLimb->CATMessage(GetCOREInterface()->GetTime(), CAT_UPDATE, data);
			}
		}
		else if (hTarg == weights)
		{ // Refresh viewport when weights are modified
			if (!GetCOREInterface()->IsAnimPlaying())
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
		}
	}
	break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (pblock == hTarg)				pblock = NULL;
		else if (layerIKTargetTrans == hTarg)	layerIKTargetTrans = NULL;
		else if (mLayerTrans == hTarg)			mLayerTrans = NULL;
		break;
	}
	return REF_SUCCEED;
}

//////////////////////////////////////////////////////////////////////////
// CATControl

int PalmTrans2::NumLayerControllers()
{
	return 3 + NumLayerFloats();
}

CATClipValue* PalmTrans2::GetLayerController(int i)
{
	switch (i) {
	case 0:		return (CATClipValue*)weights;
	case 1:		return GetLayerTrans();
	case 2:		return GetLayerTargetAlign();
	default:	return GetLayerFloat(i - 3);
	}
}

CATControl* PalmTrans2::GetParentCATControl()
{
	return GetLimb();
}

int PalmTrans2::NumChildCATControls()
{
	return tabArbBones.Count() + GetNumDigits();
}

CATControl* PalmTrans2::GetChildCATControl(int i)
{
	if (i < tabArbBones.Count())	return (CATControl*)tabArbBones[i];
	i -= tabArbBones.Count();
	if (i < GetNumDigits())		return (CATControl*)GetDigit(i);
	//	if (i==GetNumDigits())		return (CATControl*)GetFootTrans();
	else						return NULL;
}

INode*	PalmTrans2::GetBoneByAddress(TSTR address) {
	if (address.Length() == 0) return GetNode();
	int boneindex;
	USHORT rig_id = GetNextRigID(address, boneindex);
	DbgAssert(boneindex >= 0);// all child bones are indexed on the hub
	switch (rig_id) {
	case idArbBone:		if (boneindex < GetNumArbBones())	return GetArbBone(boneindex)->GetBoneByAddress(address);	break;
	case idDigit:		if (boneindex < GetNumDigits())	return GetDigit(boneindex)->GetBoneByAddress(address);		break;
	}
	// We didn't find a bone that matched the address given
	// this may be because this rigs structure is different
	// from that of the rig that generated this address
	return NULL;
};

BOOL PalmTrans2::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	isLoading = TRUE;

	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return FALSE;

	int digitNumber = 0;

	ICATObject *iobj = GetICATObject();
	float val;
	//pblock->EnableNotifications(FALSE);

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{

			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idPalm)
			{
				isLoading = FALSE;
				//	pblock->EnableNotifications(TRUE);
				return FALSE;
			}
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idObjectParams:	if (iobj) iobj->LoadRig(load);			break;
			case idController:		DbgAssert(GetLayerTrans()); GetLayerTrans()->LoadRig(load);		break;
			case idDigitParams:
			case idDigit:
			{
				// I am assuming here that the digits were already creaeted
				// when numdigits got loaded
				if (digitNumber >= GetNumDigits()) SetNumDigits(digitNumber + 1);

				DigitData *newDigit = GetDigit(digitNumber);
				DbgAssert(newDigit);
				ok = newDigit->LoadRig(load);
				digitNumber++;
				break;
			}
			case idPlatform:
			{
				if (!pLimb->GetIKTarget()) pLimb->CreateIKTarget();
				if (pLimb->GetIKTarget())
				{
					Control *ctrlPlatform = pLimb->GetIKTarget()->GetTMController();
					if (ctrlPlatform->ClassID() == GetFootTrans2Desc()->ClassID())
						((FootTrans2*)ctrlPlatform)->LoadRig(load);
				}
				else load->SkipGroup();
				break;
			}
			case idArbBones:		LoadRigArbBones(load);					break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idBoneName:	load->GetValue(name);		SetName(name);	break;
			case idFlags:		load->GetValue(ccflags);					break;
			case idSetupTM: {
				Matrix3 tmSetup;
				load->GetValue(tmSetup);

				// now our
				if (load->GetVersion() < CAT_VERSION_1700) {
					tmSetup.SetTrans(tmSetup.GetTrans()*RotateYMatrix((float)M_PI));
					//	tmSetup.SetTrans(tmSetup.GetTrans()*=1.0f)
				}

				mLayerTrans->SetSetupVal((void*)&tmSetup);
				break;
			}
			case idNumBones:	load->ToParamBlock(pblock, PB_NUMDIGITS);				break;
			case idWidth:		load->GetValue(val);	if (iobj) iobj->SetX(val);		break;
			case idLength:		load->GetValue(val);	if (iobj) iobj->SetZ(val);		break;
			case idHeight:		load->GetValue(val);	if (iobj) iobj->SetY(val);		break;
			case idTargetAlign:
			case idPivotPosZ:
			{
				CATClipValue* pTargetAlign = GetLayerTargetAlign();
				if (pTargetAlign != NULL)
				{
					load->GetValue(val);
					pTargetAlign->SetSetupVal((void*)&val);
				}
				break;
			}
			default:			load->AssertOutOfPlace();
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

	// ST - CATParent sends out CATUnits message post-load anyway?
	//	UpdateCATUnits();
	//	pblock->EnableNotifications(TRUE);

	if (ok) {
		if (load->GetVersion() < CAT3_VERSION_2707)
		{
			HoldSuspend hs;
			SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_POS);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL);
			ClearCCFlag(CCFLAG_ANIM_STRETCHY);
		}
	}

	isLoading = FALSE;
	// Technically, setting values should invalidate our
	// caches, but we have notifications disabled in so many places.
	// can't be bothered figuring it out, just disable them.
	validFK = validIK = NEVER;
	return ok && load->ok();
}

BOOL PalmTrans2::SaveRig(CATRigWriter *save)
{
	LimbData2* pLimb = GetLimb();
	if (!pLimb)
		return FALSE;

	int numDigits = GetNumDigits();

	save->BeginGroup(idPalm);
	save->Write(idBoneName, GetName());

	save->Write(idFlags, ccflags);

	//	Matrix3 tmSetup = GetSetupMatrix();
	//	save->Write(idSetupTM, (void*)&tmSetup);
	if (GetLayerTrans()) GetLayerTrans()->SaveRig(save);

	ICATObject* iobj = (ICATObject*)GetObject()->GetInterface(I_CATOBJECT);
	if (iobj) iobj->SaveRig(save);

	CATClipValue* pTargetAlign = GetLayerTargetAlign();
	if (pTargetAlign != NULL) {
		float val;
		pTargetAlign->GetSetupVal((void*)&val);
		save->Write(idTargetAlign, val);
	}

	if (numDigits > 0) {
		save->Write(idNumBones, numDigits);
		for (int i = 0; i < numDigits; i++) GetDigit(i)->SaveRig(save);
	}

	SaveRigArbBones(save);

	save->EndGroup();
	return TRUE;
}

BOOL PalmTrans2::LoadClip(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return FALSE;

	BOOL done = FALSE;
	BOOL ok = TRUE;

	int digitNumber = 0;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{

			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idPalm) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idGroupWeights: {
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				ok = GetWeights()->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idController: {
				int newflags = flags;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)) newflags |= CLIPFLAG_SKIPPOS;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)) newflags |= CLIPFLAG_SKIPROT;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) newflags |= CLIPFLAG_SKIPSCL;

				if (mLayerTrans) mLayerTrans->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idIKTargetValues: {
				if (pLimb->GetIKTarget()) {
					((CATNodeControl*)pLimb->GetIKTarget()->GetTMController())->LoadClip(load, timerange, layerindex, dScale, flags);
					break;
					//	INode *parentNode = pLimb->GetIKTarget()->GetParentNode();
					//	if(parentNode->IsRootNode())
					//		 flags |= CLIPFLAG_WORLDSPACE;
					//	else flags &= ~CLIPFLAG_WORLDSPACE;
				}

				int newflags = flags;
				newflags |= CLIPFLAG_WORLDSPACE;

				// old method of loading IKTarget Animation
				CATClipMatrix3* pLayerIKTargetTrans = dynamic_cast<CATClipMatrix3*>(pLimb->GetReference(LimbData2::REF_LAYERIKTARGETRANS));
				if (load->GetVersion() <= CAT_VERSION_1210)
				{
					if (pLayerIKTargetTrans)
						pLayerIKTargetTrans->LoadClip(load, timerange, layerindex, dScale, newflags);
					else load->SkipGroup();
				}
				else
				{
					// All new clip and pose loader that is way cooler than before
					// now poses will load onto ik target irrelevant of whether the
					// IK target is linked to something else in the scene
					BOOL iktargok = TRUE;
					while (load->ok() && !done && iktargok) {

						if (!load->NextClause())
							if (load->CurGroupID() != idIKTargetValues) break;

						// Now check the clause ID and act accordingly.
						switch (load->CurClauseID()) {
						case rigBeginGroup:
							switch (load->CurIdentifier())
							{
							case idController:
								if (pLayerIKTargetTrans)
									pLayerIKTargetTrans->LoadClip(load, timerange, layerindex, dScale, newflags);
								break;
							default:
								load->AssertOutOfPlace();
								load->SkipGroup();
								break;
							}
							break;
						case rigAssignment:
							switch (load->CurIdentifier()) {
							case idValMatrix3: {
								Matrix3 val;
								// this method will do all the processing of the pose for us
								if (load->GetValuePose(newflags, SuperClassID(), (void*)&val)) {

									if (pLimb->GetIKTarget()) pLimb->GetIKTarget()->SetNodeTM(timerange.Start(), val);
									else {
										SetXFormPacket XFormSet(val);
										pLayerIKTargetTrans->SetValue(timerange.Start(), (void*)&XFormSet);
									}
								}
								break;
							}
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
				}
				break;
			}
			case idIKFKValues: {
				//As of V1.7 the IKFK controller is stored on the pLimb.
				CATClipValue* layerIKFK = (CATClipValue*)pLimb->GetReference(LimbData2::REF_LAYERIKFKRATIO);
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				if (layerIKFK) layerIKFK->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idTargetAlignValues: {
				int newflags = flags;
				newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
				newflags = newflags&~CLIPFLAG_MIRROR;
				newflags = newflags&~CLIPFLAG_SCALE_DATA;
				CATClipValue* pTargetAlign = GetLayerTargetAlign();
				if (pTargetAlign != NULL)
					pTargetAlign->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idDigitParams:
			case idDigit: {
				DigitData *newDigit = GetDigit(digitNumber);
				if (newDigit) ok = newDigit->LoadClip(load, timerange, layerindex, dScale, flags);
				else		 load->SkipGroup();
				digitNumber++;
				break;
			}
			case idPlatform: {
				if (pLimb->GetIKTarget())
					((CATNodeControl*)pLimb->GetIKTarget()->GetTMController())->LoadClip(load, timerange, layerindex, dScale, flags);
				else load->SkipGroup();
				break;
			}
			case idArbBones:
				LoadClipArbBones(load, timerange, layerindex, dScale, flags);
				break;
			case idExtraControllers:
				LoadClipExtraControllers(load, timerange, layerindex, dScale, flags);
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idSubNum:
			{
				int subNum;
				load->GetValue(subNum);

				load->NextClause();
				if (load->CurIdentifier() == idController)
				{
					int numSubs = NumSubs();
					if (subNum <= numSubs)
					{
						if (SubAnim(subNum)) ((CATClipValue*)SubAnim(subNum))->LoadClip(load, timerange, layerindex, dScale, flags);
						else load->SkipGroup();
					}
					else // from now on this won't be needed
					{
						subNum -= numSubs;
						// Stephen, need to verify this
						CATClipValue *ctrlClipValue = (CATClipValue*)pblock->GetControllerByIndex(subNum);
						if (ctrlClipValue) ctrlClipValue->LoadClip(load, timerange, layerindex, dScale, flags);
						else load->SkipGroup();
					}
				}
			}
			break;
			case idLMR:
				// If a pose or clip was saved out from the palm,
				// then as we load it back in we need to be able
				// to know if we need to mirror it.
				int lmr;
				load->GetValue(lmr);
				if ((lmr + pLimb->GetLMR()) == 0) {
					flags |= CLIPFLAG_MIRROR;
					// We assume we are loading onto the correct body part now.
					// We do not need to swap at any point
					flags |= CLIPFLAG_MIRROR_DATA_SWAPPED;
				}
				break;
			default:
				load->AssertOutOfPlace();
				break;
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

	UpdateCATUnits();

	return ok && load->ok();
}

BOOL PalmTrans2::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return FALSE;

	int numDigits = pblock->GetInt(PB_NUMDIGITS);
	save->BeginGroup(idPalm);

	// If a pose or clip if being saved out from the palm,
	// then as we load it back in we need to be able
	// to know if we need to mirror it.
	int lmr = pLimb->GetLMR();
	save->Write(idLMR, lmr);

	if (flags&CLIPFLAG_CLIP) {
		save->BeginGroup(idGroupWeights);
		GetWeights()->SaveClip(save, flags, timerange, layerindex);
		//		((CATClipValue*)GetClipPalm()->GetWeights())->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}

	if (mLayerTrans)
	{
		save->BeginGroup(idController);
		mLayerTrans->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	CATClipValue* pTargetAlign = GetLayerTargetAlign();
	if (pTargetAlign != NULL)
	{
		save->BeginGroup(idTargetAlignValues);
		pTargetAlign->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	if (numDigits) {
		for (int i = 0; i < numDigits; i++) {
			DigitData *digit = GetDigit(i);
			if (digit) digit->SaveClip(save, flags, timerange, layerindex);
		}
	}

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

void PalmTrans2::Update()
{
	TimeValue t = GetCOREInterface()->GetTime();
	// Even the FK system is evaluated from the start of the pLimb down
	//	if(pLimb->GetIKFKRatio(t, pLimb->GetNumBones()) < 1.0f){
	GetLimb()->CATMessage(t, CAT_UPDATE);
	//	}else CATMessage(t, CAT_UPDATE);
};

/************************** CAT Methods ****************************/
// Create Destroy ect Digits
void BailCreateDigits(PalmTrans2* palm) {
	int oldNumDigits = palm->GetNumDigits();
	int newNumDigits = palm->pblock->GetInt(PalmTrans2::PB_NUMDIGITS);
	if (newNumDigits != oldNumDigits) {
		palm->pblock->EnableNotifications(FALSE);
		palm->pblock->SetValue(PalmTrans2::PB_NUMDIGITS, 0, oldNumDigits);
		palm->pblock->EnableNotifications(TRUE);
	}
}

class PalmRefeshUIRestore : public RestoreObj {
public:
	PalmTrans2	*ctrl;
	PalmRefeshUIRestore(PalmTrans2 *c) {
		ctrl = c;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			ctrl->UpdateUI();
			ctrl->GetLimb()->UpdateLimb();
		}
	}
	void Redo() {
	}
	int Size() { return 1; }
	void EndHold() {}
};

void	PalmTrans2::SetNumDigits(int nNumDigits)
{
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }
	if (theHold.Holding()) theHold.Put(new PalmRefeshUIRestore(this));
	pblock->SetValue(PB_NUMDIGITS, 0, nNumDigits);
	CreateDigits();
	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_NUMDIGITS));
	}
};

DigitData* PalmTrans2::GetDigit(int id)
{
	return (id < GetNumDigits()) ? (DigitData*)pblock->GetReferenceTarget(PB_DIGITDATATAB, 0, id) : NULL;
}

void PalmTrans2::SetDigit(int id, DigitData* newdigit)
{
	if (id >= GetNumDigits())
	{
		pblock->EnableNotifications(FALSE);
		pblock->SetCount(PB_DIGITDATATAB, id + 1);
		pblock->SetValue(PB_NUMDIGITS, 0, id + 1);
		pblock->EnableNotifications(TRUE);
	}
	pblock->SetValue(PB_DIGITDATATAB, 0, (ReferenceTarget*)newdigit, id);
}

void PalmTrans2::CreateDigits()
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return;

	int oldNumDigits = GetNumDigits();
	int newNumDigits = pblock->GetInt(PB_NUMDIGITS);
	if (newNumDigits == oldNumDigits)
		return;

	// Access to max's core
	Interface *ip = GetCOREInterface();

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }

	int j;
	DigitData* digit;
	if (newNumDigits < oldNumDigits)
	{
		// count down, it makes so much more sense
		for (j = (oldNumDigits - 1); j >= newNumDigits; j--)	// If newNumDigits < oldNumDigits, delete some
		{
			digit = GetDigit(j);
			//	digit->RemoveDigit();
			// Delete the Nodes
			INodeTab nodes;
			digit->AddSystemNodes(nodes, kSNCDelete);
			for (int i = (nodes.Count() - 1); i >= 0; i--)
				if (nodes[i])	nodes[i]->Delete(0, FALSE);
		}
	}

	pblock->Resize(PB_DIGITDATATAB, newNumDigits);
	pblock->SetCount(PB_DIGITDATATAB, newNumDigits);

	if (newNumDigits > oldNumDigits)
	{
		for (j = oldNumDigits; j < newNumDigits; j++)
		{
			digit = (DigitData*)CreateInstance(REF_TARGET_CLASS_ID, DIGITDATA_CLASS_ID);
			digit->Initialise(this, j, isLoading);
			pblock->SetValue(PB_DIGITDATATAB, 0, (ReferenceTarget*)digit, j);
		}
	}

	///////////////////////////////////////////////////
	// Now position the digits
	BOOL isLeg = pLimb->GetisLeg();
	BOOL isArm = pLimb->GetisArm();

	// this will only happen if not during rig loading
	if ((newNumDigits > 0) && !isLoading)
	{
		const int lengthaxis = GetLengthAxis();
		for (j = 0; j < newNumDigits; j++)
		{
			digit = GetDigit(j);

			Point3 p3RootPos = digit->GetRootPos();
			if (j > oldNumDigits) p3RootPos[lengthaxis] = 1.0f;
			Point3 p3RootRot = P3_IDENTITY;

			float x, y, z;
			if (lengthaxis == Z) {
				x = 0.0f;
				y = 0.0f;
				z = 1.0f;
			}
			else {
				x = 1.0f;
				y = 0.0f;
				z = 0.0f;
			}

			BOOL hasThumb = isArm && !isLeg;
			if (digit)
			{
				// if we have only 2 fingers 1 is the thumb and this one should stick straight out
				if ((newNumDigits == 1) || (hasThumb && (newNumDigits == 2) && j == 1)) {
					p3RootPos = FloatToVec(1.0f, lengthaxis);
				}
				else if ((j > 0) || isLeg)
				{
					// TOD: one day clean up this code so we don't have so much duplication
					if (lengthaxis == Z) {
						x = (-2.0f * ((float)(j - hasThumb) / (newNumDigits - hasThumb - 1.0f))) + 1.0f;

						// Space digits evenly along tip of palm
						if (j < oldNumDigits) {
							y = p3RootPos.y;
							z = p3RootPos.z;
						}
						else {
							p3RootPos.y = y;
							p3RootPos.z = z;
						}
						p3RootPos.x = x;
					}
					else {
						z = -((-2.0f * ((float)(j - hasThumb) / (newNumDigits - hasThumb - 1.0f))) + 1.0f);

						// Space digits evenly along tip of palm
						if (j < oldNumDigits) {
							y = p3RootPos.y;
							x = p3RootPos.x;
						}
						else {
							p3RootPos.y = y;
							p3RootPos.x = x;
						}
						p3RootPos.z = z;
					}
				}
				else
				{	// position the thumb back down the edge palm
					// Give the digit a position at the side of the palm
					// halfway up. (Should be near to thumb pos...)
					if (lengthaxis == Z) {
						p3RootPos = Point3(1.0f, 0.0f, 0.3f);
						p3RootRot = Point3(0.0f, 30.0f, -30.0f);
					}
					else {
						p3RootPos = Point3(0.3f, 0.0f, -1.0f);
						p3RootRot = Point3(30.0f, 30.0f, 0.0f);
					}
				}
				if (lengthaxis == Z)	p3RootPos[X] *= pLimb->GetLMR();
				else				p3RootPos[Z] *= pLimb->GetLMR();
				digit->SetRootPos(p3RootPos);

				// Convert from XYZ to matrix Rot
				if (j >= oldNumDigits)
				{
					Matrix3 tmSetup = digit->GetBone(0)->GetSetupMatrix();

					pLimb->SetLeftRightRot(p3RootRot);
					Quat catQuat;
					float catEulerArr[] = { -DegToRad(p3RootRot.x), DegToRad(p3RootRot.y), DegToRad(p3RootRot.z) };
					EulerToQuat(&catEulerArr[0], catQuat);
					PreRotateMatrix(tmSetup, catQuat);

					digit->GetBone(0)->SetSetupMatrix(tmSetup);
				}
				if (oldNumDigits == 0) {
					// Make up default values for the new digis sizes
					ICATObject *iobj = GetICATObject();
					float dNewDigitLength = iobj->GetZ() * 0.6f;
					if (isLeg) dNewDigitLength *= 0.5;
					digit->SetDigitLength(dNewDigitLength);

					float dNewDigitWidth = iobj->GetX() * 0.9f;
					dNewDigitWidth /= (hasThumb && newNumDigits > 1) ? (newNumDigits - 1) : newNumDigits;
					digit->SetDigitWidth(dNewDigitWidth);
					digit->SetDigitDepth(iobj->GetY() * 0.7f);
				}
			}
		}
		CATMessage(0, CAT_CATUNITS_CHANGED);
		// Make sure all the new digits have the right name
		CATMessage(0, CAT_NAMECHANGE);
	}

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_NUMDIGITS));
	}

	GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
}

CATNodeControl* PalmTrans2::FindParentCATNodeControl()
{
	LimbData2* pLimb = GetLimb();
	DbgAssert(pLimb != NULL);
	if (pLimb != NULL)
	{
		int iNumBones = pLimb->GetNumBones();
		if (iNumBones > 0)
		{
			BoneData* pLastBone = pLimb->GetBoneData(iNumBones - 1);
			if (pLastBone != NULL)
			{
				int iNumSegs = pLastBone->GetNumBones();
				if (iNumSegs > 0)
					return pLastBone->GetBone(iNumSegs - 1);
			}
		}
	}
	return NULL;
}

int	PalmTrans2::NumChildCATNodeControls()
{
	return GetNumDigits();
}

CATNodeControl*	PalmTrans2::GetChildCATNodeControl(int i)
{
	DigitData* pDigit = GetDigit(i);
	if (pDigit != NULL)
		return pDigit->GetBone(0);
	return NULL;
};

TSTR PalmTrans2::GetName()
{
	return pblock->GetStr(PB_NAME);
}

void PalmTrans2::SetName(TSTR newname, BOOL quiet/*=FALSE*/)
{
	if (newname != GetName()) {
		if (quiet) DisableRefMsgs();
		pblock->SetValue(PB_NAME, 0, newname);
		if (quiet) EnableRefMsgs();
	};
}

TSTR PalmTrans2::GetRigName()
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return _T("ERROR - No Limb");

	DbgAssert(GetCATParentTrans());
	return GetCATParentTrans()->GetCATName() + pLimb->GetName() + GetName();
}

//////////////////////////////////////////////////////////////////////////

float PalmTrans2::GetTargetAlign(TimeValue t, Interval& ivValid)
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return 0.0f;

	if (pLimb->TestFlag(LIMBFLAG_LOCKED_IK))
		return 1.0f;

	float val;
	pblock->GetValue(PB_LAYERTARGETALIGN, t, val, ivValid);

	return min(1.0f, max(val, 0.0f));;
}

/***********************************************************************
*/

void PalmTrans2::Initialise(CATControl* owner, BOOL loading)
{
	LimbData2* pLimb = (LimbData2*)owner;
	DbgAssert(owner);
	SetLimb(pLimb);

	//pblock->EnableNotifications(FALSE);
	pblock->SetValue(PB_LIMBDATA, 0, pLimb);

	//////////////////////////////////////////////////////////////////////////
	// Layers Stuff
	// Plug into the Clip Hierarchy

	CATClipWeights* newweights = CreateClipWeightsController(pLimb->GetWeights(), GetCATParentTrans(), FALSE);
	CATClipValue* layerPalm = CreateClipValueController(GetCATClipMatrix3Desc(), newweights, GetCATParentTrans(), loading);
	CATClipValue* pTargetAlign = CreateClipValueController(GetCATClipFloatDesc(), newweights, GetCATParentTrans(), loading);

	float val = 1.0f;
	pTargetAlign->SetSetupVal((void*)&val);

	ReplaceReference(WEIGHTS, newweights);
	ReplaceReference(LAYERTRANS, layerPalm);
	pblock->SetControllerByID(PB_LAYERTARGETALIGN, 0, pTargetAlign, FALSE);

	//pblock->EnableNotifications(TRUE);

	if (!pLimb->GetisLeg()) {
		ccflags |= CNCFLAG_LOCK_SETUPMODE_LOCAL_POS;
		ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	}

	SimpleObject2* objPalm = (SimpleObject2*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	INode* pNode = CreateNode(objPalm);
	pNode->SetWireColor(asRGB(pLimb->GetGroupColour()));

	if (!loading)// if we are not loading a rig preset, then set some defaults
	{
		LimbData2 *symlimb = static_cast<LimbData2*>(pLimb->GetSymLimb());
		if (symlimb && symlimb->GetPalmTrans())
		{
			PasteRig(symlimb->GetPalmTrans(), PASTERIGFLAG_MIRROR, 1.0f);
		}
		else
		{
			int isLeg = pLimb->GetisLeg();
			if (isLeg) {
				pblock->SetValue(PB_NAME, 0, GetString(IDS_ANKLE));
			}
			else {
				pblock->SetValue(PB_NAME, 0, GetString(IDS_PALM));
			}

			ICATObject* iobj = (ICATObject*)objPalm->GetInterface(I_CATOBJECT);
			if (pLimb->GetisLeg()) {
				// default ankle shape
				iobj->SetZ(15.0f);
				iobj->SetX(10.0f);
				iobj->SetY(5.0f);

				Matrix3 tmSetup(1);
				if (GetLengthAxis() == Z)
					tmSetup.PreRotateX(-0.5f);
				else tmSetup.PreRotateZ(0.5f);
				SetSetupMatrix(tmSetup);
			}
			else {
				// default palm shape
				iobj->SetZ(15.0f);
				iobj->SetX(10.0f);
				iobj->SetY(5.0f);
			}

		}
		UpdateCATUnits();
	}
}

// user Properties.
// these will be used by the runtime libraries
void	PalmTrans2::UpdateUserProps() {

	CATNodeControl::UpdateUserProps();

	// Generate a unique identifier for the pLimb.
	// we will borrow the node handle from the 1st bone in the pLimb
	//	ULONG limbid = GetLimb()->GetBone(0)->GetNode()->GetHandle();
	//	node->SetUserPropInt(_T("CATProp_LimbID"), limbid);

	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return;

	if (pLimb->GetisLeg())
		GetNode()->SetUserPropInt(_T("CATProp_AnkleBone"), true);
	else GetNode()->SetUserPropInt(_T("CATProp_PalmBone"), true);
}

// Displays our Rollouts
void PalmTrans2::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;

	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return;

	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY)
	{
		pLimb->BeginEditParams(ip, flags, prev);
		// Let the CATNode manage the UIs for motino and hierarchy panel
		CATNodeControl::BeginEditParams(ip, flags, prev);
		if (flagsbegin&BEGIN_EDIT_MOTION) {

			//////////////////////////////////////////////////////
			// Initialise Sub-Object mode
			moveMode = new MoveCtrlApparatusCMode(this, ip);

			TSTR type(GetString(IDS_ANKLE_PIVOTPOS));
			const TCHAR *ptype = type;
			ip->RegisterSubObjectTypes((const TCHAR**)&ptype, 1);

			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			ip->RedrawViews(ip->GetTime());
			//////////////////////////////////////////////////////

			if (GetNumDigits() > 0) ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_PALM_DIGITMANAGER), &PalmMotionParamsProc, GetString(IDS_DIGITMANAGER), (LPARAM)this);

			if (pLimb->GetisLeg())
				PalmTrans2_t_param_blk.local_name = IDS_ANKLE_ANIM;
			else
				PalmTrans2_t_param_blk.local_name = IDS_PALM_ANIM;

			GetPalmTrans2Desc()->BeginEditParams(ip, this, flags, prev);
		}
	}
	else if (flagsbegin == 0) {
		TSTR palmsetuprolloutname;
		if (pLimb->GetisLeg())
			palmsetuprolloutname = GetString(IDS_ANKLE_SETUP);
		else palmsetuprolloutname = GetString(IDS_PALM_SETUP);

		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_PALM_SETUP_CAT3), PalmObjRolloutProc, palmsetuprolloutname, (LPARAM)this);

	}
	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void PalmTrans2::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return;

	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY) {
		// Let the CATNode manage the UIs for motion and hierarchy panel
		CATNodeControl::EndEditParams(ip, flags, next);
		if (pLimb) pLimb->EndEditParams(ip, END_EDIT_REMOVEUI);
		if (flagsbegin&BEGIN_EDIT_MOTION) {
			///////////////////////////////////
			// End the subobject mode
			ip->DeleteMode(moveMode);
			if (moveMode) delete moveMode;
			moveMode = NULL;
			///////////////////////////////////

			if (GetNumDigits() > 0) ip->DeleteRollupPage(PalmMotionParamsCallback.GetHWnd());
			GetPalmTrans2Desc()->EndEditParams(ip, this, flags, next);
		}
	}
	else if (flagsbegin == 0) {
		ip->DeleteRollupPage(staticPalmObjRollout.GetHWnd());
	}

	ipbegin = NULL;
}

void PalmTrans2::UpdateUI()
{
	if (!ipbegin) return;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO) {
		CATNodeControl::UpdateUI();

		LimbData2* pLimb = GetLimb();
		if (pLimb) pLimb->UpdateUI();

	}
	else if (flagsbegin == 0) {
		if (staticPalmObjRollout.GetHWnd()) staticPalmObjRollout.Update();
	}

	GetPalmTrans2Desc()->InvalidateUI(&PalmTrans2_t_param_blk);

	//IParamMap2* pMap = GetPalmTrans2Desc()->GetParamMap(&PalmTrans2_t_param_blk);
	//if (pMap) pMap->UpdateUI(GetCOREInterface()->GetTime());
}

void PalmTrans2::KeyFreeform(TimeValue t, ULONG flags)
{
	LimbData2* pLimb = GetLimb();
	if (pLimb == NULL)
		return;

	if (!pLimb->TestFlag(LIMBFLAG_LOCKED_IK))
	{
		CATClipValue* pTargetAlign = GetLayerTargetAlign();
		if (pTargetAlign) {
			Interval iv = FOREVER;
			float dTargetAlign = 0;
			KeyFreeformMode SetMode(pTargetAlign);
			pTargetAlign->GetValue(t, (void*)&dTargetAlign, iv, CTRL_ABSOLUTE);
			pTargetAlign->SetValue(t, (void*)&dTargetAlign, 1, CTRL_ABSOLUTE);
		}
	}

	CATNodeControl::KeyFreeform(t, flags);
}

void PalmTrans2::RemoveDigit(int id, BOOL deletenodes)
{
	BOOL newundo = FALSE;;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	int numDigits = GetNumDigits();
	DbgAssert(id < numDigits);
	DigitData* dyingdigit = GetDigit(id);

	// shuffle the remaining digits down the list
	for (int i = id; i < (numDigits - 1); i++) {
		SetDigit(i, GetDigit(i + 1));
		if (GetDigit(i)) GetDigit(i)->SetDigitID(i);
	}

	pblock->EnableNotifications(FALSE);
	pblock->SetCount(PB_DIGITDATATAB, numDigits - 1);
	pblock->SetValue(PB_NUMDIGITS, 0, numDigits - 1);
	pblock->EnableNotifications(TRUE);

	if (deletenodes && dyingdigit) {
		INodeTab nodes;
		dyingdigit->AddSystemNodes(nodes, kSNCDelete);
		for (int i = (nodes.Count() - 1); i >= 0; i--)	nodes[i]->Delete(0, FALSE);
	}
	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_DIGITREM));
	}
}

void PalmTrans2::InsertDigit(int id, DigitData* digit)
{
	int numdigits = GetNumDigits() + 1;
	pblock->EnableNotifications(FALSE);
	pblock->SetCount(PB_DIGITDATATAB, numdigits);
	pblock->SetValue(PB_NUMDIGITS, 0, numdigits);
	pblock->EnableNotifications(TRUE);

	// shuffle the remaining up a knotch to make room for the new bone
	if (id < numdigits - 1) {
		for (int i = (numdigits - 1); i > (id - 1); --i) {
			SetDigit(i, GetDigit(i - 1));
			if (GetDigit(i)) GetDigit(i)->SetDigitID(i);
		}
	}
	SetDigit(id, digit);
}

BOOL PalmTrans2::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	PalmTrans2* pastepalm = (PalmTrans2*)pastectrl;

	if (theHold.Holding()) theHold.Put(new PalmRefeshUIRestore(this));

	int numDigits = GetNumDigits();
	int pasteNumDigits = pastepalm->GetNumDigits();
	//	Force the number of bones to be the same
	if (numDigits != pasteNumDigits)
		pblock->SetValue(PB_NUMDIGITS, 0, pasteNumDigits);

	float val;
	pastepalm->GetLayerTargetAlign()->GetSetupVal((void*)&val);
	GetLayerTargetAlign()->SetSetupVal((void*)&val);

	// handles adding sufficint Arbitrary bones
	CATNodeControl::PasteRig(pastectrl, flags, scalefactor);

	return TRUE;
}

void PalmTrans2::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	switch (ctxt) {
	case kSNCDelete:
		// In a delete operation, it may be that only the palm is being deleted.
		// In this case, we need to update the pointer on the LimbData2.
		if (GetCATMode() == SETUPMODE) {
			AddSystemNodes(nodes, ctxt);
			LimbData2* pLimb = (LimbData2*)GetLimb();
			if (pLimb != NULL)
				pLimb->SetPalmTrans(NULL);
		}
		else {
			GetCATParentTrans()->AddSystemNodes(nodes, ctxt);
		}
		break;
	default:
		CATNodeControl::GetSystemNodes(nodes, ctxt);
		break;
	}
}

BaseInterface* PalmTrans2::GetInterface(Interface_ID id)
{
	if (id == I_CATGROUP) return (CATGroup*)this;
	return CATNodeControl::GetInterface(id);
}

FPInterfaceDesc* PalmTrans2::GetDescByID(Interface_ID id) {
	// No published functions (so no desc) for CATGroup
	return CATNodeControl::GetDescByID(id);
}

//////////////////////////////////////////////////////////////////////
// Sub-Object Selection
MoveCtrlApparatusCMode*		PalmTrans2::moveMode = NULL;
RotateCtrlApparatusCMode*	PalmTrans2::rotateMode = NULL;
static GenSubObjType SOT_ProjVector(23);

// selection levels:
#define SEL_OBJECT	0
#define SEL_PIVOT	1

/*
int PalmTrans2::NumSubObjTypes()
{
return 1;
}

ISubObjType *PalmTrans2::GetSubObjType(int i)
{
static bool initialized = false;
if(!initialized)
{
initialized = true;
SOT_ProjVector.SetName(_T("xxxxx"));
}

if(i == -1)
{
if(selLevel > 0)
return GetSubObjType(selLevel-1);
}

switch(i)
{
case 0:	return &SOT_ProjVector;
}
return NULL;
}
*/

void PalmTrans2::DrawGizmo(TimeValue, GraphicsWindow *gw)
{
	bbox.Init();
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));
	Point3 pos = tmPalmTarget.GetTrans() - tmPalmTarget.VectorTransform(ikpivotoffset * GetCATUnits());
	gw->marker(&pos, PLUS_SIGN_MRKR);
}

int PalmTrans2::HitTest(
	TimeValue t, INode *, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	UNREFERENCED_PARAMETER(flags);
	Matrix3 tm(1);
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0, 0, 0);

	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(Matrix3(1));
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	//	DrawAndHit(t, inode, vpt);
	DrawGizmo(t, gw);

	/*
	if (showAxis) {
	DrawAxis(vpt,tm,axisLength,screenSize);
	}
	vpt->getGW()->setTransform(tm);
	vpt->getGW()->marker(&pt,X_MRKR);
	*/

	gw->setRndLimits(savedLimits);

	// CAL-08/27/03: This doesn't make sense. It shouldn't do this. (Defect #468271)
	// This will always select this helper when there's an intersection on the bounding box and the selection window.
	// TODO: There's still a problem with window selection. We need to check if it hits all components in DrawAndHit.
	/*
	if((hitRegion.type != POINT_RGN) && !hitRegion.crossing)
	return TRUE;
	*/

	return gw->checkHitCode();
}

int PalmTrans2::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!ipbegin) return 0;

	Interval iv;
	GraphicsWindow *gw = vpt->getGW();	// This line is here because I don't know how to initialize
	// a *gw. I will change it in the next line
	gw->setTransform(Matrix3(1));
	DrawGizmo(t, gw);

	return CATNodeControl::Display(t, inode, vpt, flags);;
}

void PalmTrans2::GetWorldBoundBox(TimeValue, INode *, ViewExp*, Box3& box)
{
	//	box.Init();
	//	Matrix3 m = inode->GetNodeTM(t);
	//	box += m.GetRow(3);
	box += bbox;
	//	box = bbox * m;
}

void PalmTrans2::ActivateSubobjSel(int level, XFormModes& modes)
{
	// Set the meshes level
	selLevel = level;

	if (level == SEL_PIVOT) {
		modes = XFormModes(moveMode, NULL, NULL, NULL, NULL, NULL);
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void PalmTrans2::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	UNREFERENCED_PARAMETER(invert); UNREFERENCED_PARAMETER(all); UNREFERENCED_PARAMETER(selected);
	//	HoldTrack();
	while (hitRec) {
		//		if (selected) {
		//			sel.Set(hitRec->hitInfo);
		//		} else {
		//			sel.Clear(hitRec->hitInfo);
		//			}
		//		if (all)
		hitRec = hitRec->Next();
		//		else break;
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void PalmTrans2::ClearSelection(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
	//	HoldTrack();
	//	sel.ClearAll();
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

int PalmTrans2::SubObjectIndex(CtrlHitRecord *hitRec)
{
	UNREFERENCED_PARAMETER(hitRec);
	//	for (ulong i=0; i<hitRec->hitInfo; i++) {
	//		if (sel[i]) count++;
	//		}
	return 1;//count;
}
void PalmTrans2::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue, INode *)
{
	if (selLevel == SEL_OBJECT) return;	// shouldn't happen.

	Point3 center = tmPalmTarget.GetTrans() - tmPalmTarget.VectorTransform(ikpivotoffset * GetCATUnits());
	cb->Center(center, 0);
}

void PalmTrans2::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue, INode *)
{
	Matrix3 tm = tmPalmTarget;
	tm.PreTranslate(-ikpivotoffset * GetCATUnits());
	cb->TM(tm, 0);
}

class PalmIKPivotMoveRestore : public RestoreObj {
public:
	PalmTrans2	*p;
	Point3		ikpivotoffset_undo, ikpivotoffset_redo;

	PalmIKPivotMoveRestore(PalmTrans2 *p) {
		this->p = p;
		ikpivotoffset_undo = p->ikpivotoffset;
	}

	void Restore(int isUndo) {
		if (isUndo) {
			ikpivotoffset_redo = p->ikpivotoffset;
		}
		p->ikpivotoffset = ikpivotoffset_undo;
		p->Update();
	}

	void Redo() {
		p->ikpivotoffset = ikpivotoffset_redo;
		p->Update();
	}

	int Size() { return 4; }
	void EndHold() { }
};

void PalmTrans2::SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
{
	UNREFERENCED_PARAMETER(localOrigin);
	Point3 pos = VectorTransform(tmAxis*Inverse(partm), val);

	if (theHold.Holding()) theHold.Put(new PalmIKPivotMoveRestore(this));

	ikpivotoffset += -pos / GetCATUnits();;

	// Until I figure out what this does, I'm disabling it.
	// The SetValue causes the limb to freak out
	// I don't remember this feature from CAT 2.X,
	// and the versions I have seen it in it seems
	// it is completely borked.

	// When someone complains about it, I'll fix it.

	//SetXFormPacket ptr(-val, partm, tmAxis);
	//SetValue(t, (void*)&ptr, 1, CTRL_RELATIVE);

	Update();
}

Color PalmTrans2::GetGroupColour()
{
	LimbData2* pLimb = GetLimb();
	return (pLimb != NULL) ? pLimb->GetGroupColour() : 0;
}

void PalmTrans2::SetGroupColour(Color clr)
{
	LimbData2* pLimb = GetLimb();
	if (pLimb != NULL)
		pLimb->SetGroupColour(clr);
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class PalmTransPLCB : public PostLoadCallback {
protected:
	PalmTrans2 *palm;

public:
	PalmTransPLCB(PalmTrans2 *pOwner) { palm = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(palm);
		if (palm->GetFileSaveVersion() > 0) return palm->GetFileSaveVersion();

		ICATParentTrans *catparent = palm->GetCATParentTrans();
		if (catparent) return catparent->GetFileSaveVersion();
		// old method of getting to the catparent
		LimbData2* limb = palm->GetLimb();
		DbgAssert(limb);
		catparent = limb->GetCATParentTrans();
		DbgAssert(catparent);
		return catparent->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		if (GetFileSaveVersion() < CAT_VERSION_1730) {

			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_2430 && GetFileSaveVersion() > CAT_VERSION_2000) {
			// This code is to fix CATMotion layers assigned to CAT2 rigs. CAT1 rigs are upgraded correcly
			for (int i = 0; i < palm->mLayerTrans->GetNumLayers(); i++) {
				// This upgrade is because between versions 2.0 and 2.5 CATMition layers were being set up incorectly on digits
				// the wrong controllers were bing assigned. CATMotionRot, instead of CATMotionDigitRot
				if (palm->mLayerTrans->GetRoot()->GetLayer(i)->GetMethod() == LAYER_CATMOTION &&
					palm->mLayerTrans->GetLayer(i)->GetRotationController()->ClassID() == CATMOTIONROT_CLASS_ID) {
					CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)palm->mLayerTrans->GetLayer(i)->GetRotationController();
					Control* p3 = ctrlCATMotionRot->pblock->GetControllerByID(CATMotionRot::PB_P3CATMOTION);

					if (palm->GetLengthAxis() == X) {
						// Foot Bend was working in the wrong direction
						CATGraph* footbend = (CATGraph*)p3->GetReference(Z);
						footbend->FlipValues();
					}
				}
			}
			iload->SetObsolete();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

/**********************************************************************
* Loading and saving....
*/
#define PALMTRANS_CATNODECONTROL		0
IOResult PalmTrans2::Save(ISave *isave)
{
	IOResult res = IO_OK;

	isave->BeginChunk(PALMTRANS_CATNODECONTROL);
	res = CATNodeControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult PalmTrans2::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case PALMTRANS_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new PalmTransPLCB(this));

	return IO_OK;
}

