[!output TEMPLATESTRING_COPYRIGHT]

#include "[!output PROJECT_NAME].h"

extern ClassDesc2* Get[!output CLASS_NAME]Desc();
[!if SPACE_WARP_TYPE != 0]
extern ClassDesc2* Get[!output CLASS_NAME]ObjDesc();
[!endif]

HINSTANCE hInstance;
int controlsInit = FALSE;

[!if ADD_COMMENTS != 0]
// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.
[!endif]

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID /*lpvReserved*/)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		MaxSDK::Util::UseLanguagePackLocale();
		// Hang on to this DLL's instance handle.
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);
		// DO NOT do any initialization here. Use LibInitialize() instead.
	}
	return(TRUE);
}

[!if ADD_COMMENTS != 0]
// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
[!endif]
__declspec( dllexport ) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}

[!if ADD_COMMENTS != 0]
// This function returns the number of plug-in classes this DLL
[!endif]
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
[!if SPACE_WARP_TYPE != 0]
	return 2;
[!else]
	return 1;
[!endif]
}

[!if ADD_COMMENTS != 0]
// This function returns the number of plug-in classes this DLL
[!endif]
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i)
	{
		case 0: return Get[!output CLASS_NAME]Desc();
[!if SPACE_WARP_TYPE != 0]
		case 1: return Get[!output CLASS_NAME]ObjDesc();
[!endif]
		default: return 0;
	}
}

[!if ADD_COMMENTS != 0]
// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
[!endif]
__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

[!if ADD_COMMENTS != 0]
// This function is called once, right after your plugin has been loaded by 3ds Max. 
// Perform one-time plugin initialization in this method.
// Return TRUE if you deem your plugin successfully loaded, or FALSE otherwise. If 
// the function returns FALSE, the system will NOT load the plugin, it will then call FreeLibrary
// on your DLL, and send you a message.
[!endif]
__declspec( dllexport ) int LibInitialize(void)
{
	#pragma message(TODO("Perform initialization here."))
	return TRUE;
}

[!if ADD_COMMENTS != 0]
// This function is called once, just before the plugin is unloaded. 
// Perform one-time plugin un-initialization in this method."
// The system doesn't pay attention to a return value.
[!endif]
__declspec( dllexport ) int LibShutdown(void)
{
	#pragma message(TODO("Perform un-initialization here."))
	return TRUE;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
	{
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	}

	return NULL;
}

