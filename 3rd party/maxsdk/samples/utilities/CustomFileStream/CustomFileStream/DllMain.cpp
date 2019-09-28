//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include <maxscript/maxscript.h>
#include "resource.h"

HMODULE hInstance = NULL;

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			MaxSDK::Util::UseLanguagePackLocale();
			// Hang on to this DLL's instance handle.
			hInstance = hModule;
			DisableThreadLibraryCalls(hInstance);
			break;
	}
		
	return(TRUE);
}

__declspec( dllexport ) void LibInit() 
{ 
	// do any setup here
}

__declspec( dllexport ) const TCHAR* LibDescription() 
{ 
	static TSTR libDescription (MaxSDK::GetResourceStringAsMSTR(IDS_LIBDESCRIPTION)); 
	return libDescription;
}

__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX; 
}

__declspec(dllexport) int LibNumberClasses() 
{
	return 0; 
}

__declspec(dllexport) ClassDesc* LibClassDesc(int i) 
{
	return nullptr; 
}

__declspec(dllexport) ULONG CanAutoDefer()
{
	// Cannot auto-defer because it registers for file load/save callbacks.
	return 0;
}

