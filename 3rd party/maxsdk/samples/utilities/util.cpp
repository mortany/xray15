/**********************************************************************
 *<
	FILE: util.cpp

	DESCRIPTION:   Sample utilities

	CREATED BY: Rolf Berteig

	HISTORY: created 23 December 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "util.h"

HINSTANCE hInstance;

// russom - 10/16/01
// The classDesc array was created because it was way to difficult
// to maintain the switch statement return the Class Descriptors
// with so many different #define used in this module.

#define MAX_UTIL_OBJECTS 15
static ClassDesc *classDescArray[MAX_UTIL_OBJECTS];
static int classDescCount = 0;
static int classDescCountOrig = 0;	// this is unique to the util.cpp implementation

void initClassDescArray(void)
{
   if( !classDescCount )
   {
	classDescArray[classDescCount++] = GetColorClipDesc();
	classDescArray[classDescCount++] = GetCollapseUtilDesc();
	classDescArray[classDescCount++] = GetRandKeysDesc();
	classDescArray[classDescCount++] = GetORTKeysDesc();
	classDescArray[classDescCount++] = GetSelKeysDesc();
	classDescArray[classDescCount++] = GetLinkInfoUtilDesc();
	classDescArray[classDescCount++] = GetCellTexDesc();
	classDescArray[classDescCount++] = GetRescaleDesc();
	classDescArray[classDescCount++] = GetShapeCheckDesc();
#ifdef _DEBUG
	classDescArray[classDescCount++] = GetUtilTestDesc();
	classDescArray[classDescCount++] = GetAppDataTestDesc();
	classDescArray[classDescCount++] = GetTestSoundObjDescriptor();
#endif
	classDescArray[classDescCount++] = GetEulerFilterDesc();

	classDescCountOrig = classDescCount;
   }
}

/** public functions **/
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();
      hInstance = hinstDLL;
      DisableThreadLibraryCalls(hInstance);
   }

	return (TRUE);
	}


//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() { return GetString(IDS_LIBDESCRIPTION); }

__declspec( dllexport ) int LibNumberClasses() {
   initClassDescArray();

	classDescCount = classDescCountOrig;

	// RB 11/17/2000: Only provide set-key mode plug-in if the feature is enabled.
	if (IsSetKeyModeFeatureEnabled())
		classDescArray[classDescCount++] = GetSetKeyUtilDesc();

	return classDescCount;
	}


__declspec( dllexport ) ClassDesc*
LibClassDesc(int i) {
   initClassDescArray();

	if( i < classDescCount )
		return classDescArray[i];
	else
		return NULL;
}


// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() { return VERSION_3DSMAX; }

TCHAR *GetString(int id)
	{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
	}

__declspec(dllexport) ULONG CanAutoDefer()
{
	// registering AppDataLoadProc to process AppDataTest's appdata on scene file load,
	// cannot defer plugin loading
	return 0;
}

__declspec(dllexport) void LibInitialize()
{
	Animatable::RegisterAppDataLoadCallback(APPDATA_TEST_CLASS_ID, UTILITY_CLASS_ID, AppDataTest_UpdateAppDataLoadProc);
}

