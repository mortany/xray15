   /**********************************************************************
    *<
      FILE: ffdmod.cpp

      DESCRIPTION: DllMain is in here

      CREATED BY: Rolf Berteig

      HISTORY: created 7/22/96

    *>   Copyright (c) 1996 Rolf Berteig, All Rights Reserved.
    **********************************************************************/

////////////////////////////////////////////////////////////////////
//
// Free Form Deformation Patent #4,821,214 licensed 
// from Viewpoint DataLabs Int'l, Inc., Orem, UT
// www.viewpoint.com
// 
////////////////////////////////////////////////////////////////////

#include "ffdmod.h"

#include "3dsmaxport.h"

HINSTANCE hInstance;

// russom - 10/12/01
// The classDesc array was created because it was way to difficult
// to maintain the switch statement return the Class Descriptors
// with so many different #define used in this module.

#define MAX_FFD_OBJECTS 9
static ClassDesc *classDescArray[MAX_FFD_OBJECTS];
static int classDescCount = 0;

void initClassDescArray(void)
{
   if( !classDescCount )
   {
   classDescArray[classDescCount++] = GetFFDDesc44();
   classDescArray[classDescCount++] = GetFFDDesc33();
   classDescArray[classDescCount++] = GetFFDDesc22();
   classDescArray[classDescCount++] = GetFFDNMSquareOSDesc();
   classDescArray[classDescCount++] = GetFFDNMSquareWSDesc();
   classDescArray[classDescCount++] = GetFFDNMSquareWSModDesc();
   classDescArray[classDescCount++] = GetFFDNMCylOSDesc();
   classDescArray[classDescCount++] = GetFFDNMCylWSDesc();
   classDescArray[classDescCount++] = GetFFDSelModDesc();
   }
}

/** public functions **/
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();
      hInstance = hinstDLL;            // Hang on to this DLL's instance handle.
      DisableThreadLibraryCalls(hInstance);
   }

   return(TRUE);
   }


//------------------------------------------------------
// This is the interface to MAX
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() { return GetString(IDS_LIBDESCRIPTION); }


// This function returns the number of plug-in classes this DLL implements
__declspec( dllexport ) int 
LibNumberClasses() 
{ 
   initClassDescArray();

   return classDescCount; 
}


// This function return the ith class descriptor. We have one.
__declspec( dllexport ) ClassDesc* 
LibClassDesc(int i) 
{
   initClassDescArray();

   if( i < classDescCount )
      return classDescArray[i];
   else
      return NULL;
}


// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() { return VERSION_3DSMAX; }

__declspec( dllexport ) ULONG CanAutoDefer()
{
	// Turning off defer loading until mxs value proxy systems are in place 
	return 0;
}


// Loads a string from the resource into a static buffer.
TCHAR *GetString(int id)
   {
   static TCHAR buf[256];
   if (hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
   }

INT_PTR CALLBACK DefaultSOTProc(
      HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
   {
   IObjParam *ip = DLGetWindowLongPtr<IObjParam*>(hWnd);

   switch (msg) {
      case WM_INITDIALOG:
         DLSetWindowLongPtr(hWnd, lParam);
         break;

      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         if (ip) ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         return FALSE;

      default:
         return FALSE;
      }
   return TRUE;
   }
