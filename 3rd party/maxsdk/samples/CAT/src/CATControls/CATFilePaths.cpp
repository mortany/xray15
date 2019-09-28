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

/**********************************************************************
	Functions to retrieve the standard path for various CAT preset
	files.  Each of these returns a temporary pointer to an
	internal buffer containing the full pathname requested, without
	the trailing backslash.  Successive calls to any of these
	functions will overwrite the buffer.

	We now use an INI file to store our settings.  For now, I've
	simply hooked these settings into the existing file path
	functions, but in future this entire module might be thrown
	away, and the calls simply replaced with catCfg.Get() calls.
 **********************************************************************/

#include "Max.h"
#include "CATFilePaths.h"
#include "../CATObjects/CatDotIni.h"

static TCHAR pPathBuffer[MAX_PATH];

static int nLastRigPresetFilterIndex = -1;
static int nLastMotionPresetFilterIndex = -1;
static int nLastHandPosePresetFilterIndex = -1;
static int nLastClipFilterIndex = -1;
static int nLastFreeformFilterIndex = -1;
static int nLastCliporPoseFilterIndex = -1;

TCHAR *GetRigPresetPath()
{
	CatDotIni catCfg;
	_tcscpy(pPathBuffer, catCfg.Get(INI_RIG_PRESET_PATH));
	return pPathBuffer;
}

TCHAR *GetMotionPresetPath()
{
	CatDotIni catCfg;
	_tcscpy(pPathBuffer, catCfg.Get(INI_MOTION_PRESET_PATH));
	return pPathBuffer;
}

TCHAR *GetHandPosePresetPath()
{
	CatDotIni catCfg;
	_tcscpy(pPathBuffer, catCfg.Get(INI_HAND_POSE_PRESET_PATH));
	return pPathBuffer;
}

TCHAR *GetClipPath()
{
	CatDotIni catCfg;
	_tcscpy(pPathBuffer, catCfg.Get(INI_CLIPS_PATH));
	return pPathBuffer;
}

TCHAR *GetPosePath()
{
	CatDotIni catCfg;
	_tcscpy(pPathBuffer, catCfg.Get(INI_POSES_PATH));
	return pPathBuffer;
}

TCHAR *GetPlugCFGPath(const TCHAR *filename/*=NULL*/)
{
	if (!filename) _sntprintf(pPathBuffer, MAX_PATH, _T("%s\\CAT"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	else _sntprintf(pPathBuffer, MAX_PATH, _T("%s\\CAT\\%s"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR), filename);
	return pPathBuffer;
}

static void InitOpenFileName(OPENFILENAME *ofn, HINSTANCE hInstance, HWND hWnd, const TCHAR* fileBuf, int bufSize, LPCTSTR filter, LPCTSTR fileTitle = NULL, LPCTSTR initialDir = NULL, LPCTSTR caption = NULL);

static void InitOpenFileName(OPENFILENAME *ofn, HINSTANCE hInstance, HWND hWnd, const TCHAR* fileBuf, int bufSize, LPCTSTR filter, LPCTSTR fileTitle/*=NULL*/, LPCTSTR initialDir/*=NULL*/, LPCTSTR caption/*=NULL*/)
{
	memset(ofn, 0, sizeof(OPENFILENAME));

	ofn->lStructSize = sizeof(OPENFILENAME); //OPENFILENAME_SIZE_VERSION_400;
	ofn->hInstance = hInstance;
	ofn->hwndOwner = hWnd;
	ofn->lpstrFilter = filter;
	ofn->nFilterIndex = 1;
	ofn->lpstrFile = const_cast<TCHAR*>(fileBuf);
	ofn->nMaxFile = bufSize;
	ofn->lpstrFileTitle = const_cast<TCHAR*>(fileTitle);
	ofn->lpstrInitialDir = initialDir;
	ofn->lpstrTitle = caption;
	ofn->Flags = OFN_EXPLORER | OFN_ENABLEHOOK | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_HIDEREADONLY;
	ofn->lpfnHook = (LPOFNHOOKPROC)MaxSDK::MinimalMaxFileOpenHookProc;
}

BOOL GetSaveFileName(HINSTANCE hInstance, HWND hWnd, TCHAR* fileBuf, int bufSize, LPTSTR filter, LPTSTR defaultExtension, LPTSTR fileTitle/*=NULL*/, LPTSTR initialDir/*=NULL*/, LPTSTR caption/*=NULL*/)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME); //OPENFILENAME_SIZE_VERSION_400;
	ofn.hInstance = hInstance;
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fileBuf;
	ofn.nMaxFile = bufSize;
	ofn.lpstrFileTitle = fileTitle;
	ofn.lpstrInitialDir = initialDir;
	ofn.lpstrTitle = caption;
	ofn.lpstrDefExt = defaultExtension;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_HIDEREADONLY;
	ofn.lpfnHook = (LPOFNHOOKPROC)MaxSDK::MinimalMaxFileOpenHookProc;

	return GetSaveFileName(&ofn);
}

BOOL GetOpenFileName(HINSTANCE hInstance, HWND hWnd, TCHAR* fileBuf, int bufSize, LPTSTR filter, LPTSTR fileTitle/*=NULL*/, LPTSTR initialDir/*=NULL*/, LPTSTR caption/*=NULL*/)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME); //OPENFILENAME_SIZE_VERSION_400;
	ofn.hInstance = hInstance;
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fileBuf;
	ofn.nMaxFile = bufSize;
	ofn.lpstrFileTitle = fileTitle;
	ofn.lpstrInitialDir = initialDir;
	ofn.lpstrTitle = caption;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_HIDEREADONLY;
	ofn.lpfnHook = (LPOFNHOOKPROC)MaxSDK::MinimalMaxFileOpenHookProc;

	return GetOpenFileName(&ofn);
}

BOOL DialogOpenRigPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize)
{
	TSTR path, fname, extension;

	if (fileBuf[0] != _T('\0'))
	{
		// if a full path/file/extension was sent - split it into components
		TSTR file = fileBuf;
		SplitFilename(file, &path, &fname, &extension);
		// copy just the name,and extension, into the fileBuf
		_tcscpy(fileBuf, fname.data());
		_tcscat(fileBuf, extension.data());
	}
	else path = GetRigPresetPath();

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_RIG_PRESET_FILTER, NULL, path.data());
	ofn.Flags |= OFN_FILEMUSTEXIST;

	BOOL result = GetOpenFileName(&ofn);
	if (result)	nLastRigPresetFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogOpenMotionPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_MOTION_PRESET_FILTER,
		//					 NULL, GetMotionPresetPath());
		NULL, (initialDir || nLastMotionPresetFilterIndex != -1) ? initialDir : GetMotionPresetPath());
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (nLastMotionPresetFilterIndex != -1)
		ofn.nFilterIndex = nLastMotionPresetFilterIndex;

	BOOL result = GetOpenFileName(&ofn);
	if (result) nLastMotionPresetFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogOpenHandPosePreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_HANDPOSE_PRESET_FILTER,
		NULL, (initialDir || nLastHandPosePresetFilterIndex != -1) ? initialDir : GetHandPosePresetPath());
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (nLastHandPosePresetFilterIndex != -1)
		ofn.nFilterIndex = nLastHandPosePresetFilterIndex;

	BOOL result = GetOpenFileName(&ofn);
	if (result) nLastHandPosePresetFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogOpenClip(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_CLIP_FILTER,
		NULL, (initialDir || nLastClipFilterIndex != -1) ? initialDir : GetClipPath());
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (nLastClipFilterIndex != -1)
		ofn.nFilterIndex = nLastClipFilterIndex;

	BOOL result = GetOpenFileName(&ofn);
	if (result) nLastClipFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogOpenPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_POSE_FILTER,
		NULL, (initialDir || nLastClipFilterIndex != -1) ? initialDir : GetClipPath());
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (nLastClipFilterIndex != -1)
		ofn.nFilterIndex = nLastClipFilterIndex;

	BOOL result = GetOpenFileName(&ofn);
	if (result) nLastClipFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogOpenFreeform(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, BOOL bIsClip, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_FREEFORM_FILTER,
		NULL, (initialDir || nLastFreeformFilterIndex != -1) ? initialDir : (bIsClip ? GetClipPath() : GetPosePath()));
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (nLastFreeformFilterIndex != -1)
		ofn.nFilterIndex = nLastFreeformFilterIndex;
	else
		ofn.nFilterIndex = (bIsClip) ? 2 : 1;

	BOOL result = GetOpenFileName(&ofn);
	if (result) nLastFreeformFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogSaveRigPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName)
{
	TSTR path, fname, extension;

	if (fileBuf[0] != _T('\0'))
	{
		// if a full path/file/extension was sent - split it into components
		TSTR file = fileBuf;
		SplitFilename(file, &path, &fname, &extension);
		// copy just the name,and extension, into the fileBuf
		_tcscpy(fileBuf, fname.data());
		_tcscat(fileBuf, extension.data());
	}
	else path = GetRigPresetPath();

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_RIG_PRESET_FILTER, initialName, path.data());
	ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
	ofn.lpstrDefExt = CAT_RIG_PRESET_EXT;

	BOOL result = GetSaveFileName(&ofn);
	return result;
}

BOOL DialogSaveMotionPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName/*=NULL*/, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');
	OPENFILENAME ofn;

	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_MOTION_PRESET_FILTER,
		initialName, initialDir ? initialDir : GetMotionPresetPath());
	ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
	ofn.lpstrDefExt = CAT_MOTION_PRESET_EXT;

	BOOL result = GetSaveFileName(&ofn);
	return result;
}

BOOL DialogSaveHandPosePreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName/*=NULL*/, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');
	OPENFILENAME ofn;

	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_HANDPOSE_PRESET_FILTER,
		initialName, initialDir ? initialDir : GetHandPosePresetPath());
	ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
	ofn.lpstrDefExt = CAT_HANDPOSE_EXT;

	BOOL result = GetSaveFileName(&ofn);
	return result;
}

BOOL DialogOpenClipOrPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');

	OPENFILENAME ofn;
	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_CLIPORPOSE_FILTER,
		NULL, (initialDir || nLastCliporPoseFilterIndex != -1) ? initialDir : GetPosePath());
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (nLastCliporPoseFilterIndex != -1)
		ofn.nFilterIndex = nLastCliporPoseFilterIndex;
	else
		ofn.nFilterIndex = 1;

	BOOL result = GetOpenFileName(&ofn);
	if (result) nLastCliporPoseFilterIndex = ofn.nFilterIndex;
	return result;
}

// this will pop up eith a save clip or a save pose box
BOOL DialogSaveClipOrPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, BOOL bIsClip, TCHAR *initialName/*=NULL*/, TCHAR *initialDir/*=NULL*/)
{
	fileBuf[0] = _T('\0');
	OPENFILENAME ofn;

	if (bIsClip)
	{
		InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_CLIP_FILTER,
			initialName, initialDir ? initialDir : GetClipPath());
		ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
		ofn.lpstrDefExt = CAT_CLIP_EXT;
	}
	else
	{
		InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_POSE_FILTER,
			initialName, initialDir ? initialDir : GetPosePath());
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = CAT_POSE_EXT;
	}
	BOOL result = GetSaveFileName(&ofn);
	return result;
}

/*
// this will pop up a save as dialogue with the option to choose CLP of PSE files
BOOL DialogSaveClipAndPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName, TCHAR *initialDir)
{
	fileBuf[0] = _T('\0');
	OPENFILENAME ofn;

	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_CLIPORPOSE_FILTER,
		NULL, (initialDir || nLastCliporPoseFilterIndex != -1) ? initialDir : GetPosePath());
	ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
	if (nLastCliporPoseFilterIndex != -1)
		ofn.nFilterIndex = nLastCliporPoseFilterIndex;
	else
		ofn.nFilterIndex = 1;

	ofn.lpstrDefExt = CAT_CLIP_EXT;
	BOOL result = GetSaveFileName(&ofn);
	if (result) nLastCliporPoseFilterIndex = ofn.nFilterIndex;
	return result;
}

BOOL DialogSavePose(HINSTANCE hInstance, HWND hWnd, char *fileBuf, int bufSize, char *initialName, char *initialDir)
{
	fileBuf[0] = _T('\0');
	OPENFILENAME ofn;

	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_POSE_FILTER,
					 initialName, initialDir ? initialDir : GetPosePath());
	ofn.Flags |= OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = CAT_CLIP_EXT;

	BOOL result = GetSaveFileName(&ofn);
	return result;
}
*/
BOOL DialogSaveFreeform(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName, TCHAR *initialDir)
{
	fileBuf[0] = _T('\0');
	OPENFILENAME ofn;

	InitOpenFileName(&ofn, hInstance, hWnd, fileBuf, bufSize, CAT_FREEFORM_FILTER,
		initialName, initialDir ? initialDir : GetClipPath());
	ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
	//ofn.lpstrDefExt = CAT_CLIP_EXT;

	BOOL result = GetSaveFileName(&ofn);
	return result;
}

void ResetLastFreeformFilterIndex()
{
	nLastFreeformFilterIndex = -1;
}
