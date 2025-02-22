//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

// local
#include "resource.h"
#include "RapidRTRendererPlugin.h"
#include "Plugins/NoiseFilter_RenderElement.h"

// Max SDK
#include <iparamb2.h>
#include <dllutilities.h>

int controlsInit = FALSE;

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.

BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/,ULONG fdwReason,LPVOID /*lpvReserved*/)
{
	//TODO( "Check for props as we have removed the additionnalcompileroptions.txt")*-*********************************************TODO
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		MaxSDK::Util::UseLanguagePackLocale();
		// Hang on to this DLL's instance handle.
		DisableThreadLibraryCalls(MaxSDK::GetHInstance());
	}
	return(TRUE);
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription()
{
    static const MSTR str = MaxSDK::GetResourceStringAsMSTR(IDS_RAPID_CLASS_NAME);
    return str;
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
	return 2;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i) {
    case 0: 
        return &(Max::RapidRTTranslator::RapidRTRendererPlugin::GetTheClassDescriptor());
    case 1:
        return &(Max::RapidRTTranslator::NoiseFilter_RenderElement::GetTheClassDescriptor());
    default: 
        return nullptr;
	}
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

// This function is called once, right after your plugin has been loaded by 3ds Max. 
// Perform one-time plugin initialization in this method.
// Return TRUE if you deem your plugin successfully loaded, or FALSE otherwise. If 
// the function returns FALSE, the system will NOT load the plugin, it will then call FreeLibrary
// on your DLL, and send you a message.
__declspec( dllexport ) int LibInitialize(void)
{
	return TRUE; // TODO: Perform initialization here.
}

// This function is called once, just before the plugin is unloaded. 
// Perform one-time plugin un-initialization in this method."
// The system doesn't pay attention to a return value.
__declspec( dllexport ) int LibShutdown(void)
{
	return TRUE;
}

