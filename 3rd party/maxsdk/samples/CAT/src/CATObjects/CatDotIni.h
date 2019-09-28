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
//
//
//   Added default values to the Get() functions.  This is important,
//   as we want the ini file support to be introduced transparently.
//   Every new key that is added is only created when it is required,
//   and is created with its default value.
//
#pragma once

class IniFile;

//
// The Paths group specifies locations file sets.
//
#define GRP_PATHS						_T("Paths")
#define GRP_DEFAULTS					_T("CATDefaults")

#define KEY_RIG_PRESET_PATH				GRP_PATHS, _T("RigPresets")
#define KEY_MOTION_PRESET_PATH			GRP_PATHS, _T("MotionPresets")
#define KEY_HAND_POSE_PRESET_PATH		GRP_PATHS, _T("HandPosePresets")
#define KEY_CLIPS_PATH					GRP_PATHS, _T("Clips")
#define KEY_POSES_PATH					GRP_PATHS, _T("Poses")

#define KEY_LENGTH_AXIS					GRP_DEFAULTS, _T("LengthAxis")
#define KEY_MUSCLE_TYPE					GRP_DEFAULTS, _T("DeformerType")

#define INI_RIG_PRESET_PATH				KEY_RIG_PRESET_PATH, DefaultRigPresetPath()
#define INI_MOTION_PRESET_PATH			KEY_MOTION_PRESET_PATH, DefaultMotionPresetPath()
#define INI_HAND_POSE_PRESET_PATH		KEY_HAND_POSE_PRESET_PATH, DefaultHandPosePresetPath()
#define INI_CLIPS_PATH					KEY_CLIPS_PATH, DefaultClipsPath()
#define INI_POSES_PATH					KEY_POSES_PATH, DefaultPosesPath()

extern const TCHAR *DefaultRigPresetPath();
extern const TCHAR *DefaultMotionPresetPath();
extern const TCHAR *DefaultHandPosePresetPath();
extern const TCHAR *DefaultClipsPath();
extern const TCHAR *DefaultPosesPath();

//
// The Dialog group specifies which popups have been chosen
// to "Not be shown in future".  Values always default to false.
//
#define GRP_NOSHOW							_T("NoShow")

#define KEY_NOSHOW_DELETE_IN_SETUPMODE_DLG	GRP_NOSHOW,	_T("DeleteInSetupModeDlg")

#define INI_NOSHOW_DELETE_IN_SETUPMODE_DLG	KEY_NOSHOW_DELETE_IN_SETUPMODE_DLG, _T("0")

//////////////////////////////////////////////////////////////////////
// CatDotIni
//
// This class wraps the IniFile functions to give a set of general
// functions for operating on labels.  The functions provided here
// are slightly less efficient than the IniFile functions, as we are
// always performing a search.  However, this gives us the ability
// to set up new groups and keys without unwanted complexity.
//
class CatDotIni : public MaxHeapOperators
{
private:
	bool bDirty;
	IniFile *ini;

public:
	// Constructs the CatDotIni class and automatically loads the file.
	CatDotIni();
	~CatDotIni();

	// These functions read and write the entire .ini file.
	bool Reload();
	bool Write();

	// Stuff to do with the dirty-flag.
	bool Dirty() const;
	void ClearDirty();

	// This returns true if the given key exists within the given group.
	bool Exists(const TCHAR *szGroup, const TCHAR *szKey) const;

	// This returns the value of the requested key.  If the key does not
	// exist, it is created and assigned the default value.
	const TCHAR *Get(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault = _T(""));

	// These functions retrieve the value of the requested key and
	// convert it to the required type.  If the key does not exist,
	// the default value for each type is returned.  This is usually
	// a representation of zero.
	int GetInt(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault = _T(""));
	bool GetBool(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault = _T(""));
	float GetFloat(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault = _T(""));
	double GetDouble(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault = _T(""));

	// These functions set the value of a key.  If the key does not exist
	// it is created (along with the group if that does not exist).
	// Note that all values are converted to strings, so in the case of
	// floating point numbers, some precision may be lost.
	void Set(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szValue);
	void SetInt(const TCHAR *szGroup, const TCHAR *szKey, int nValue);
	void SetBool(const TCHAR *szGroup, const TCHAR *szKey, bool bValue);
	void SetFloat(const TCHAR *szGroup, const TCHAR *szKey, float fValue);
	void SetDouble(const TCHAR *szGroup, const TCHAR *szKey, double dValue);

	// This retrieves the actual IniFile class, in case one needs to
	// iterate through groups and keys, or perform more complicated
	// operation than this class provides.
	const IniFile *GetIniFile() const;
};
