/**********************************************************************
 *<
   FILE: mods.cpp

   DESCRIPTION:   DLL implementation of modifiers

   CREATED BY: Rolf Berteig (based on prim.cpp)

   HISTORY: created 30 January 1995

 *>   Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "mods.h"
#include "buildver.h"

#include "3dsmaxport.h"

HINSTANCE hInstance;

#define MAX_MOD_OBJECTS 52 // LAM - 2/3/03 - bounced up from 51
ClassDesc *classDescArray[MAX_MOD_OBJECTS];
int classDescCount = 0;

// russom - 05/24/01
// The classDesc array was created because it was way to difficult
// to maintain the switch statement return the Class Descriptors
// with so many different #define used in this module.

void initClassDescArray(void)
{
   if( !classDescCount )
   {
    classDescArray[classDescCount++] = GetBendModDesc();
    classDescArray[classDescCount++] = GetTaperModDesc();

    classDescArray[classDescCount++] = GetSinWaveObjDesc();
    classDescArray[classDescCount++] = GetSinWaveModDesc();

    classDescArray[classDescCount++] = GetEditMeshModDesc();
    classDescArray[classDescCount++] = GetTwistModDesc();
    classDescArray[classDescCount++] = GetExtrudeModDesc();

    classDescArray[classDescCount++] = GetBombObjDesc();
    classDescArray[classDescCount++] = GetBombModDesc();    

    classDescArray[classDescCount++] = GetClustModDesc();
    classDescArray[classDescCount++] = GetSkewModDesc();
    classDescArray[classDescCount++] = GetNoiseModDesc();
    classDescArray[classDescCount++] = GetSinWaveOModDesc();
    classDescArray[classDescCount++] = GetLinWaveOModDesc();

    classDescArray[classDescCount++] = GetLinWaveObjDesc();
    classDescArray[classDescCount++] = GetLinWaveModDesc();

    classDescArray[classDescCount++] = GetOptModDesc();
    classDescArray[classDescCount++] = GetDispModDesc();
    classDescArray[classDescCount++] = GetClustNodeModDesc();

  classDescArray[classDescCount++] = GetGravityObjDesc();
  classDescArray[classDescCount++] = GetGravityModDesc();
  classDescArray[classDescCount++] = GetWindObjDesc();
  classDescArray[classDescCount++] = GetWindModDesc();
  classDescArray[classDescCount++] = GetDispObjDesc();
  classDescArray[classDescCount++] = GetDispWSModDesc();
  classDescArray[classDescCount++] = GetDeflectObjDesc();
  classDescArray[classDescCount++] = GetDeflectModDesc();

   classDescArray[classDescCount++] = GetUVWMapModDesc();
   classDescArray[classDescCount++] = GetSelModDesc();
   classDescArray[classDescCount++] = GetSmoothModDesc();
   classDescArray[classDescCount++] = GetMatModDesc();
   classDescArray[classDescCount++] = GetNormalModDesc();
   classDescArray[classDescCount++] = GetSurfrevModDesc();

   classDescArray[classDescCount++] = GetResetXFormDesc();

   classDescArray[classDescCount++] = GetAFRModDesc();         
   classDescArray[classDescCount++] = GetTessModDesc();
   classDescArray[classDescCount++] = GetDeleteModDesc();
   classDescArray[classDescCount++] = GetMeshSelModDesc();
   classDescArray[classDescCount++] = GetFaceExtrudeModDesc();
   classDescArray[classDescCount++] = GetUVWXFormModDesc();
   classDescArray[classDescCount++] = GetUVWXFormMod2Desc();
   classDescArray[classDescCount++] = GetMirrorModDesc();

  classDescArray[classDescCount++] = GetBendWSMDesc();
  classDescArray[classDescCount++] = GetTwistWSMDesc();
  classDescArray[classDescCount++] = GetTaperWSMDesc();
  classDescArray[classDescCount++] = GetSkewWSMDesc();
  classDescArray[classDescCount++] = GetNoiseWSMDesc();

   classDescArray[classDescCount++] = GetSDeleteModDesc();

   classDescArray[classDescCount++] = GetDispApproxModDesc();

   classDescArray[classDescCount++] = GetMeshMesherWSMDesc();

   classDescArray[classDescCount++] = GetNormalizeSplineDesc();

   classDescArray[classDescCount++] = GetDeletePatchModDesc();

   DbgAssert (classDescCount <= MAX_MOD_OBJECTS);
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

   return(TRUE);
   }


//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() { return
 GetString(IDS_LIBDESCRIPTION); }


__declspec( dllexport ) int LibNumberClasses() 
{
   initClassDescArray();

   return classDescCount;
}

// russom - 05/07/01 - changed to use classDescArray
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

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
   return 1;
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

TCHAR *GetString(int id)
   {
   static TCHAR buf[256];

   if (hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
   }
