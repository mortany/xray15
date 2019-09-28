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

#include "Max.h"
#include "CatDotIni.h"
#include "IniFile.h"
#include "../CATControls/Locale.h"

#define CAT_INI_FILE _T("CAT\\cat.ini")

//////////////////////////////////////////////////////////////////////
// Defaults for the [Paths] group.
//
static TCHAR pTempPathBuffer[MAX_PATH];

const TCHAR *DefaultRigPresetPath() {
	_stprintf(pTempPathBuffer, _T("%s\\CAT\\CATRigs"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	return pTempPathBuffer;
}

const TCHAR *DefaultMotionPresetPath() {
	_stprintf(pTempPathBuffer, _T("%s\\CAT\\MotionPresets"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	return pTempPathBuffer;
}

const TCHAR *DefaultHandPosePresetPath() {
	_stprintf(pTempPathBuffer, _T("%s\\CAT\\HandPosePresets"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	return pTempPathBuffer;
}

const TCHAR *DefaultClipsPath() {
	_stprintf(pTempPathBuffer, _T("%s\\CAT\\Clips"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	return pTempPathBuffer;
}

const TCHAR *DefaultPosesPath() {
	_stprintf(pTempPathBuffer, _T("%s\\CAT\\Poses"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR));
	return pTempPathBuffer;
}

//
// Constructs the CatDotIni class and automatically loads the file.
//
CatDotIni::CatDotIni()
{
	TCHAR szIniFile[MAX_PATH] = { 0 };
	_stprintf(szIniFile, _T("%s\\%s"), GetCOREInterface()->GetDir(APP_PLUGCFG_LN_DIR), CAT_INI_FILE);
	bDirty = false;

	ini = new IniFile(szIniFile);
	DbgAssert(ini);
}

//
// If there has been any changes since
CatDotIni::~CatDotIni()
{
	if (Dirty()) Write();
	delete ini;
}

//
// These functions read and write the entire .ini file.
//
bool CatDotIni::Reload()
{
	return ini->Load();
}

bool CatDotIni::Write()
{
	return ini->Save();
}

//
// Returns true if anything has changed since loading/saving.
//
bool CatDotIni::Dirty() const
{
	return ini->Dirty();
}

void CatDotIni::ClearDirty()
{
	ini->ClearDirty();
}

//
// Returns true if the given key exists within the given group
//
bool CatDotIni::Exists(const TCHAR *szGroup, const TCHAR *szKey) const
{
	int nGroup = ini->FindGroup(szGroup);
	if (nGroup == -1) return false;
	if (ini->FindKey(nGroup, szKey) == -1) return false;
	return true;
}

//
// This returns the value of the requested key, or NULL if the key
// does not exist.
//
const TCHAR *CatDotIni::Get(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault/*=""*/)
{
	const TCHAR *szValue = ini->GetKey(szGroup, szKey);
	if (!szValue) {
		Set(szGroup, szKey, szDefault);
		return ini->GetKey(szGroup, szKey);
	}
	return szValue;
}

//
// These functions retrieve the value of the requested key and
// convert it to the required type.  If the key does not exist,
// the default value for each type is returned.  This is usually
// a representation of zero.
//
int CatDotIni::GetInt(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault/*=""*/)
{
	const TCHAR *szValue = Get(szGroup, szKey, szDefault);
	if (!szValue) return 0;
	return _ttoi(szValue);
}

bool CatDotIni::GetBool(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault/*=""*/)
{
	const TCHAR *szValue = Get(szGroup, szKey, szDefault);
	if (!_tcscmp(szValue, _T("")) || !_tcscmp(szValue, _T("0"))) return false;
	return true;
}

float CatDotIni::GetFloat(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault/*=""*/)
{
	const TCHAR *szValue = Get(szGroup, szKey, szDefault);
	if (!szValue) return 0.0f;
	TCHAR *szOldLocale = ReplaceLocale(LC_NUMERIC, _T("C"));
	float fValue = (float)_tstof(szValue);
	RestoreLocale(LC_NUMERIC, szOldLocale);
	return fValue;
}

double CatDotIni::GetDouble(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szDefault/*=""*/)
{
	const TCHAR *szValue = Get(szGroup, szKey, szDefault);
	if (!szValue) return 0.0;
	TCHAR *szOldLocale = ReplaceLocale(LC_NUMERIC, _T("C"));
	float dValue = (float)_tstof(szValue);
	RestoreLocale(LC_NUMERIC, szOldLocale);
	return dValue;
}

//
// These functions set the value of a key.  If the key does not exist
// it is created (along with the group if that does not exist).
// Note that all values are converted to strings, so in the case of
// floating point numbers, some precision may be lost.
//
void CatDotIni::Set(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szValue)
{
	bDirty = true;
	ini->SetKey(szGroup, szKey, szValue);
}

void CatDotIni::SetInt(const TCHAR *szGroup, const TCHAR *szKey, int nValue)
{
	static TCHAR szBuffer[33];
	Set(szGroup, szKey, _itot(nValue, szBuffer, 10));
}

void CatDotIni::SetBool(const TCHAR *szGroup, const TCHAR *szKey, bool bValue)
{
	if (bValue) Set(szGroup, szKey, _T("1"));
	else Set(szGroup, szKey, _T("0"));
}

void CatDotIni::SetFloat(const TCHAR *szGroup, const TCHAR *szKey, float fValue)
{
	static TCHAR szBuffer[50];
	_stprintf(szBuffer, _T("%e"), (double)fValue);
	Set(szGroup, szKey, szBuffer);
}

void CatDotIni::SetDouble(const TCHAR *szGroup, const TCHAR *szKey, double dValue)
{
	static TCHAR szBuffer[50];
	_stprintf(szBuffer, _T("%e"), dValue);
	Set(szGroup, szKey, szBuffer);
}

const IniFile *CatDotIni::GetIniFile() const
{
	return ini;
}
