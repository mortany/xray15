// file: MeshExpPlugin.cpp

#include "stdafx.h"
#pragma hdrstop

#include <gdiplus.h>

#include "MeshExpUtility.h"

static const TCHAR _className[] = _T("S.T.A.L.K.E.R. Export");
static const TCHAR _classNameDesc[] = _T("S.T.A.L.K.E.R. Mesh Export utility");

Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

#define ApexImp_CLASS_ID	Class_ID(0xceb7b2fb, 0x8a86cdb0)

//-------------------------------------------------------------------
// Class Descriptor

class MeshExpUtilityClassDesc : public ClassDesc2 {
public:
	virtual int 					IsPublic()	override { return TRUE; }
	virtual void* Create(BOOL/*loading = FALSE*/) override { return MeshExpUtility::GetInstance(); }
	virtual const TCHAR* ClassName()		override { return _className; }
	virtual SClass_ID				SuperClassID()	override { return UTILITY_CLASS_ID; }
	virtual Class_ID 				ClassID()	override { return ApexImp_CLASS_ID; }
	virtual const TCHAR* Category()	 override { return NULL; }
	virtual HINSTANCE		HInstance() override { return hInstance; }// returns owning module handle
};

ClassDesc2* GetMeshExpUtility() 
{ 
	static MeshExpUtilityClassDesc MeshExpUtilityClassDescCD;
	return &MeshExpUtilityClassDescCD; 

}

MeshExpUtility U;
//MeshExpUtilityClassDesc MeshExpUtilityClassDescCD;

//-------------------------------------------------------------------
// DLL interface

ClassDesc2* GetMeshExpUtility();

HINSTANCE hInstance;
int controlsInit = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID /*lpvReserved*/)
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

__declspec(dllexport) int LibInitialize(void)
{
	if (!controlsInit) 
	{
		controlsInit = TRUE;
		Core._initialize(TEXT("S.T.A.L.K.E.R.Plugin"), ELogCallback, FALSE);
		FS._initialize(CLocatorAPI::flScanAppRoot, NULL, TEXT("xray_path.ltx"));
		//FPU::m64r(); // нужно чтобы макс не сбрасывал контрольки в 0
		//InitCustomControls(hInstance);
		//InitCommonControls();
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		ELog.Msg(mtInformation, TEXT("S.T.A.L.K.E.R. Object Export (ver. %d.%02d))", EXPORTER_VERSION, EXPORTER_BUILD));
		ELog.Msg(mtInformation, TEXT("-------------------------------------------------------"));
	}
	//Core._initialize("S.T.A.L.K.E.R.Plugin", ELogCallback, FALSE);
	//FS._initialize(CLocatorAPI::flScanAppRoot, NULL, "xray_path.ltx");
	//FPU::m64r(); // нужно чтобы макс не сбрасывал контрольки в 0
	//InitCommonControls();
	//Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	return TRUE;
}


__declspec( dllexport ) const TCHAR *
LibDescription() { return _classNameDesc; }


__declspec( dllexport ) int LibNumberClasses() {
	return 1;
}


__declspec( dllexport ) ClassDesc* LibClassDesc(int i) {
	switch(i) {
		case 0: return GetMeshExpUtility();
		default: return 0;
	}
}


__declspec(dllexport) int LibShutdown(void)
{
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return TRUE;
}

__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX; 
}


TCHAR* GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
	{
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	}

	return NULL;
}