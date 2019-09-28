#include <maxscript/maxscript.h>
#include "MXSAgni.h"

extern void le_init();
extern void viewport_init();
extern void sysInfo_init();
extern void AngleCtrlInit();
extern void GroupBoxInit();
extern void ImgTagInit();
extern void LinkCtrlInit();
extern void rk_init();
extern void MXSAgni_init1();
extern void MXSAgni_init2();
extern void install_i3_custom_controls();
extern void avg_init();
extern void registry_init();

HINSTANCE g_hInst = NULL;

BOOL APIENTRY
	DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	static BOOL controlsInit = FALSE;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		MaxSDK::Util::UseLanguagePackLocale();
		// Hang on to this DLL's instance handle.
		g_hInst = hModule;
		BOOL res = DisableThreadLibraryCalls(hModule);
		break;
	}

	return(TRUE);
}

__declspec( dllexport ) void
	LibInit() { 
		// do any setup here
		le_init();
		viewport_init();
		sysInfo_init();
		AngleCtrlInit();
		GroupBoxInit();
		ImgTagInit();
		LinkCtrlInit();
		rk_init();
		MXSAgni_init1();
		MXSAgni_init2();
		install_i3_custom_controls();
		avg_init();
		registry_init();
}


__declspec( dllexport ) const TCHAR *
	LibDescription() 
{ 
	static TSTR libDescription (MaxSDK::GetResourceStringAsMSTR(IDS_LIBDESCRIPTION_MSXAGNI)); 
	return libDescription;
}

__declspec( dllexport ) ULONG
	LibVersion() {  return VERSION_3DSMAX; }
