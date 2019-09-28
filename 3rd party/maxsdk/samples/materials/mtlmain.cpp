/**********************************************************************
 *<
   FILE: mtl.cpp

   DESCRIPTION:   DLL implementation of material and textures

   CREATED BY: Dan Silva

   HISTORY: created 12 December 1994

 *>   Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "mtlhdr.h"
#include "stdmat.h"
#include "mtlres.h"

#include <vector>

HINSTANCE hInstance;

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
// This is the interface to Max:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() { return GetString(IDS_LIBDESCRIPTION); }

// orb - 01/03/01
// The classDesc array was created because it was way to difficult
// to maintain the switch statement return the Class Descriptors
// with so many different #define used in this module.

// Made classDescArray dynamic to avoid problems with MAX_MTLTEX_OBJECTS
// being less than the number of maps.
std::vector<ClassDesc *> classDescArray;

static BOOL InitMtlDLL(void)
{
   if( classDescArray.empty() )
   {

      classDescArray.push_back(  GetStdMtl2Desc() );
      classDescArray.push_back(  GetMultiDesc() );
      classDescArray.push_back(  GetCMtlDesc() );
      classDescArray.push_back(  GetBMTexDesc() );
      classDescArray.push_back(  GetMaskDesc() );
      classDescArray.push_back(  GetTintDesc() );
      classDescArray.push_back(  GetCheckerDesc() );
      classDescArray.push_back(  GetMixDesc() );
      classDescArray.push_back(  GetMarbleDesc() );
      classDescArray.push_back(  GetNoiseDesc() );
      classDescArray.push_back(  GetTexmapsDesc() );
      classDescArray.push_back(  GetOldTexmapsDesc() );
      classDescArray.push_back(  GetDoubleSidedDesc() );
      classDescArray.push_back(  GetMixMatDesc() );
      classDescArray.push_back(  GetACubicDesc() );
      classDescArray.push_back(  GetMirrorDesc() );
      classDescArray.push_back(  GetGradientDesc() );
      classDescArray.push_back(  GetCompositeDesc() );
	  classDescArray.push_back(  GetMultiTileDesc() );
      classDescArray.push_back(  GetMatteDesc() );
      classDescArray.push_back(  GetRGBMultDesc() );
      classDescArray.push_back(  GetOutputDesc() );
      classDescArray.push_back(  GetFalloffDesc() );
      classDescArray.push_back(  GetVColDesc() );
      classDescArray.push_back(  GetPhongShaderCD() );
      classDescArray.push_back(  GetMetalShaderCD() );
      classDescArray.push_back(  GetBlinnShaderCD() );
      classDescArray.push_back(  GetPlateDesc() );
      classDescArray.push_back(  GetCompositeMatDesc() );
      classDescArray.push_back(  GetPartBlurDesc() );
      classDescArray.push_back(  GetPartAgeDesc() );
      classDescArray.push_back(  GetBakeShellDesc() );
      classDescArray.push_back(  GetColorCorrectionDesc() );
      classDescArray.push_back(  GetEmptyMultiDesc() );

      // register SXP readers
      RegisterSXPReader(_T("MARBLE_I.SXP"), Class_ID(MARBLE_CLASS_ID,0));
      RegisterSXPReader(_T("NOISE_I.SXP"),  Class_ID(NOISE_CLASS_ID,0));
      RegisterSXPReader(_T("NOISE2_I.SXP"), Class_ID(NOISE_CLASS_ID,0));
   }

   return TRUE;
}

__declspec( dllexport ) int LibNumberClasses() 
{ 
   InitMtlDLL();

   return int(classDescArray.size());
}

// This function return the ith class descriptor.
__declspec( dllexport ) ClassDesc* 
LibClassDesc(int i) {
   InitMtlDLL();

   if( i < classDescArray.size() )
      return classDescArray[i];
   else
      return NULL;

   }



// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() { return VERSION_3DSMAX; }

__declspec( dllexport ) int LibInitialize() 
{
   return InitMtlDLL();
}

TCHAR *GetString(int id)
{
   static TCHAR buf[256];
   if(hInstance)
      return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
   return NULL;
}
