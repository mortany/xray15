/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: dllMain.cpp

	 DESCRIPTION: DLL entry implementation

	 CREATED BY: michael malone (mjm)

	 HISTORY: created January 27, 1999

   	 Copyright (c) 1998, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */
#include "dllMain.h"


// global variables
HINSTANCE hInstance;

// global functions
BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();
      hInstance = hinstDLL;				// Hang on to this DLL's instance handle.
      DisableThreadLibraryCalls(hInstance);
   }
	return (TRUE);
}


//------------------------------------------------------
// interface to Max
//------------------------------------------------------

// returns dll descriptive string
__declspec( dllexport ) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}


// returns number of classes in dll
__declspec( dllexport ) int LibNumberClasses()
{
	return 1;
}


// returns appropriate class descriptor
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i)
	{
	case 0:
		return GetBriteConDesc();
	default:
		return 0;
	}
}


// returns version to detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}


// standard hInstance dependent GetString()
TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}
