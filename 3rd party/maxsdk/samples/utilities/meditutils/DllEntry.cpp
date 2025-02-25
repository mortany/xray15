/**********************************************************************
 *<
   FILE: DllEntry.cpp


   DESCRIPTION:   
   -- Multi Material Clean
   clean unused sub materials in scene multi-materials
   -- Instance Duplicate Map
   substitude duplicates scene material maps with instances

   CREATED BY:    Alex Zadorozhny

   HISTORY:    Created 6/17/03

   *> Copyright (c) 2003, All Rights Reserved.
   **********************************************************************/

#include "meditutils.h"

extern ClassDesc2* GetMMCleanDesc();
extern ClassDesc2* GetInstanceDuplMapDesc();

HINSTANCE hInstance;

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

__declspec( dllexport ) const TCHAR* LibDescription()
{
   return GetString(IDS_LIBDESCRIPTION);
}

//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
   return 2;
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
   switch(i) {
      case 0: return GetMMCleanDesc();
      case 1: return GetInstanceDuplMapDesc();
      default:  return 0;
   }
}

__declspec( dllexport ) ULONG LibVersion()
{
   return VERSION_3DSMAX;
}

TCHAR *GetString(int id)
{
   static TCHAR buf[256];

   if (hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
}

