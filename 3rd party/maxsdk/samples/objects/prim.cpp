/**********************************************************************
 *<
   FILE: prim.cpp

   DESCRIPTION:   DLL implementation of primitives

   CREATED BY: Dan Silva

   HISTORY: created 12 December 1994

 *>   Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "buildver.h"
#include "prim.h"
#include "3dsurfer.h"

HINSTANCE hInstance;
int controlsInit = FALSE;
SurferPatchDataReaderCallback patchReader;
SurferSplineDataReaderCallback splineReader;

// russom - 05/07/01
// The classDesc array was created because it was way to difficult
// to maintain the switch statement return the Class Descriptors
// with so many different #define used in this module.
#define MAX_PRIM_OBJECTS   44
ClassDesc *classDescArray[MAX_PRIM_OBJECTS];
int classDescCount = 0;

static BOOL InitObjectsDLL(void)
{
   if( !classDescCount )
   {
      classDescArray[classDescCount++] = GetBoxobjDesc();
      classDescArray[classDescCount++] = GetSphereDesc();
      classDescArray[classDescCount++] = GetCylinderDesc();
      classDescArray[classDescCount++] = GetLookatCamDesc();
      classDescArray[classDescCount++] = GetSimpleCamDesc();
      classDescArray[classDescCount++] = GetTargetObjDesc();
      classDescArray[classDescCount++] = GetTSpotLightDesc();
      classDescArray[classDescCount++] = GetFSpotLightDesc();
      classDescArray[classDescCount++] = GetTDirLightDesc();
      classDescArray[classDescCount++] = GetDirLightDesc();
      classDescArray[classDescCount++] = GetOmniLightDesc();
      classDescArray[classDescCount++] = GetSplineDesc();
      classDescArray[classDescCount++] = GetNGonDesc();
      classDescArray[classDescCount++] = GetDonutDesc();
      classDescArray[classDescCount++] = GetPipeDesc();
      classDescArray[classDescCount++] = GetBonesDesc();
      classDescArray[classDescCount++] = GetRingMasterDesc();
      classDescArray[classDescCount++] = GetSlaveControlDesc();
      classDescArray[classDescCount++] = GetQuadPatchDesc();
      classDescArray[classDescCount++] = GetTriPatchDesc();
      classDescArray[classDescCount++] = GetTorusDesc();
      classDescArray[classDescCount++] = GetMorphObjDesc();
      classDescArray[classDescCount++] = GetCubicMorphContDesc();
      classDescArray[classDescCount++] = GetRectangleDesc();
      classDescArray[classDescCount++] = GetTapeHelpDesc();
      classDescArray[classDescCount++] = GetTubeDesc();
      classDescArray[classDescCount++] = GetConeDesc();
      classDescArray[classDescCount++] = GetHedraDesc();
      classDescArray[classDescCount++] = GetCircleDesc();
      classDescArray[classDescCount++] = GetEllipseDesc();
      classDescArray[classDescCount++] = GetArcDesc();
      classDescArray[classDescCount++] = GetStarDesc();
      classDescArray[classDescCount++] = GetHelixDesc();
      classDescArray[classDescCount++] = GetRainDesc();
      classDescArray[classDescCount++] = GetSnowDesc();
      classDescArray[classDescCount++] = GetTextDesc();
      classDescArray[classDescCount++] = GetTeapotDesc();
      classDescArray[classDescCount++] = GetBaryMorphContDesc();
      classDescArray[classDescCount++] = GetProtHelpDesc();
      classDescArray[classDescCount++] = GetGridobjDesc();
      classDescArray[classDescCount++] = GetNewBonesDesc();
      classDescArray[classDescCount++] = GetHalfRoundDesc();
      classDescArray[classDescCount++] = GetQuarterRoundDesc();
	  classDescArray[classDescCount++] = GetBoolObjDesc();
      RegisterObjectAppDataReader(&patchReader);
      RegisterObjectAppDataReader(&splineReader);
   }

   return TRUE;
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
LibDescription() { return GetString(IDS_LIBDESCRIPTION); }

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS
__declspec( dllexport ) int LibNumberClasses() 
{
   InitObjectsDLL();

   return classDescCount;
}

// russom - 05/07/01 - changed to use classDescArray
__declspec( dllexport ) ClassDesc*
LibClassDesc(int i) 
{
   InitObjectsDLL();

   if( i < classDescCount )
      return classDescArray[i];
   else
      return NULL;

}


// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() { return VERSION_3DSMAX; }

__declspec( dllexport ) int
LibInitialize()
{
   return InitObjectsDLL();
}

TCHAR *GetString(int id)
   {
   static TCHAR buf[256];

   if (hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
   }
