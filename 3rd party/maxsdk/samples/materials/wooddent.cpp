/**********************************************************************
 *<
	FILE:			wooddent.cpp

	DESCRIPTION:	DLL Main for wood and dent 3D textures

	CREATED BY:		Suryan Stalin

	HISTORY:		Modified from mtlmain.cpp, 4th April 1996

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "mtlhdr.h"
#include "stdmat.h"
#include "woodres.h"
#include "wooddent.h"
#include "wood.h"
#include "dent.h"
#include "buildver.h" // orb 01-03-2001 Removing map types


HINSTANCE	hInstance;

// public functions 
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      MaxSDK::Util::UseLanguagePackLocale();
      hInstance = hinstDLL;
      DisableThreadLibraryCalls(hInstance);
   }
	return(TRUE);
}

static BOOL InitMaterialsDll()
{
	static BOOL InitBool=TRUE;

	if( InitBool )
	{
      // register SXP readers
      RegisterSXPReader(_T("WOOD_I.SXP"), Class_ID(WOOD_CLASS_ID));
      RegisterSXPReader(_T("DENTS_I.SXP"), Class_ID(DENT_CLASS_ID));
      RegisterSXPReader(_T("DENTS2_I.SXP"), Class_ID(DENT_CLASS_ID));
	  InitBool=FALSE;
    }

   return TRUE;
}

//------------------------------------------------------
// This is the interface to Max:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() { return GetString(IDS_LIBDESCRIPTION);}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS
__declspec( dllexport ) int LibNumberClasses() 
{
   InitMaterialsDll();

	return 2;
}

__declspec( dllexport ) ClassDesc*
LibClassDesc(int i) 
{
   InitMaterialsDll();

	switch(i) 
	{
		case 0: return GetWoodDesc();
		case 1: return GetDentDesc();
		default: return 0;
	}
}

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() { return VERSION_3DSMAX; }

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

__declspec( dllexport ) int LibInitialize()
{
   return InitMaterialsDll();
}

Color ColrFromCol24(Col24 a) 
{
	Color c;
	c.r = (float)a.r/255.0f;
	c.g = (float)a.g/255.0f;
	c.b = (float)a.b/255.0f;
	return c;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if(hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}
