/**********************************************************************
 *<
   FILE: DllEntry.cpp

   DESCRIPTION: Contains the Dll Entry stuff

   CREATED BY: 

   HISTORY: 

 *>   Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/
#include "painterInterface.h"

extern ClassDesc2* GetPainterInterfaceDesc();
class BrushPresetMgr;
extern BrushPresetMgr* GetBrushPresetMgr();

HINSTANCE hInstance;

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();
      hInstance = hinstDLL;            // Hang on to this DLL's instance handle.
      DisableThreadLibraryCalls(hInstance);
   }

   return (TRUE);
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription()
{
   return GetString(IDS_LIBDESCRIPTION);
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
   return 1;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
   switch(i) {
      case 0: return GetPainterInterfaceDesc();
      default: return 0;
   }
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{
   return VERSION_3DSMAX;
}

__declspec( dllexport ) int LibInitialize()
{
	// so IBrushPresetMgr FPS core interface gets registered at startup before any scripts are compiled
	GetBrushPresetMgr();
	return 1;
}



TCHAR *GetString(int id)
{
   static TCHAR buf[256];

   if (hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
}

TCHAR *GetString( int id, TCHAR* buf, int nbOfChar )
{
   static TCHAR static_buf[256];
   if( buf==NULL )
      buf = static_buf, nbOfChar = _countof(static_buf);

   if (hInstance)
      return LoadString(hInstance, id, buf, nbOfChar) ? buf : NULL;
   return NULL;
}
