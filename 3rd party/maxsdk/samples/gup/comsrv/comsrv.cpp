//-----------------------------------------------------------------------------
// ---------------------
// File ....: comsrv.cpp
// ---------------------
// Author...: Gus J Grubba
// Date ....: September 1998
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "comsrv.h"

#include <initguid.h>
#include "comsrv_i.c"

#include "MaxRenderer.h"
#include "BitmapInfo.h"
#include "TestMarshalSpeed.h"

//#ifdef _DEBUG
//#include <mmsystem.h>
//#endif

const DWORD dwTimeOut	= 0;	// time for EXE to be idle before shutting down
const DWORD dwPause		= 1000; // time to wait for threads to finish up

//-----------------------------------------------------------------------------
// *> MonitorProc()
//
//    Passed to CreateThread to monitor the shutdown event

static DWORD WINAPI MonitorProc(void* pv) {
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

//-----------------------------------------------------------------------------
// #> CExeModule::Lock()

LONG CExeModule::Lock() {
	return CComModule::Lock();
}

//-----------------------------------------------------------------------------
// #> CExeModule::Unlock()

LONG CExeModule::Unlock() {
#if 0
	SetEvent(hEventShutdown);
	return 0;
#else
	LONG l = CComModule::Unlock();
	if (l == 0) {
		bActivity = true;
		SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
	}
	return l;
#endif
}

//-----------------------------------------------------------------------------
// #> CExeModule::MonitorShutdown()

void CExeModule::MonitorShutdown() {
	WaitForSingleObject(hEventShutdown, INFINITE);
	#ifdef _ATL_FREE_THREADED
	CoSuspendClassObjects();
	#endif
	CloseHandle(hEventShutdown);
	CloseHandle(hMonitorThread);
	#ifdef _DEBUG
	PlaySound(_T("chimes.wav"),NULL,SND_FILENAME|SND_ASYNC);
	#endif
	PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

//-----------------------------------------------------------------------------
// #> CExeModule::StartMonitor()

bool CExeModule::StartMonitor() {
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}

//-----------------------------------------------------------------------------
//

CExeModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_MaxRenderer,	CMaxRenderer)
OBJECT_ENTRY(CLSID_MaxBitmapInfo, CMaxBitmapInfo)
END_OBJECT_MAP()

//-----------------------------------------------------------------------------
// *> FindOneToken()

LPCTSTR FindToken(LPCTSTR p1) {
    TCHAR p2[] = _T("-/");
    while (p1 != NULL && *p1 != NULL) {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL) {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// *> OpenKeyAndSetValue()

bool OpenKeyAndSetValue(LPCTSTR szKeyName, LPCTSTR szValue) {
	HKEY	hKey;
	long	retVal;
    retVal = RegOpenKeyEx(HKEY_CLASSES_ROOT,szKeyName,0,KEY_WRITE,&hKey);
	if (retVal == ERROR_SUCCESS) {
		RegSetValue(hKey,__TEXT(""),REG_SZ,szValue,lstrlen(szValue));
		RegCloseKey(hKey);
	}
	return retVal == ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// *> RegisterCOM()

bool RegisterCOM(HINSTANCE MaxhInstance) {
	
	_Module.UpdateRegistryFromResource(IDR_Comsrv, TRUE);
	HRESULT hRes = _Module.RegisterServer(TRUE);
    _ASSERTE(SUCCEEDED(hRes));
	
	//-- Update Server Location

	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(MaxhInstance,szModule,_MAX_PATH);
	TCHAR szKeyName[256];
	TCHAR keyLocalServer[] = {_T("LocalServer32")};
	
	//-- MaxRenderer Class
	// DC 707698 - VISTA compatibility
	// this key needs to be created at install time
	TCHAR clsidApp[] = {_T("{4AD72E6E-5A4B-11D2-91CB-0060081C257E}")};
	_stprintf(szKeyName,_T("CLSID\\%s\\%s"),clsidApp,keyLocalServer);
	OpenKeyAndSetValue(szKeyName,szModule);

	//-- MaxBitmapInfo Class
	// DC 707698 - VISTA compatibility
	// this key needs to be created at install time
	_tcscpy(clsidApp,_T("{D888A162-6543-11D2-91CC-0060081C257E}"));
	_stprintf(szKeyName,_T("CLSID\\%s\\%s"),clsidApp,keyLocalServer);
	OpenKeyAndSetValue(szKeyName,szModule);

	return true;
}

//-----------------------------------------------------------------------------
// *> UnRegisterCOM()

bool UnRegisterCOM( ) {
	_Module.UpdateRegistryFromResource(IDR_Comsrv, FALSE);
	return(_Module.UnregisterServer(TRUE) != 0);
}

//-----------------------------------------------------------------------------
// *> StartServer()

bool StartServer( HINSTANCE hInstance, HINSTANCE MaxhInstance, int registerCOM ) {

	bool res = true;

	HRESULT hRes = CoInitialize(NULL);
    _ASSERTE(SUCCEEDED(hRes));
	_Module.Init(ObjectMap, hInstance, &LIBID_COMSRVLib);
	_Module.dwThreadID = GetCurrentThreadId();

	switch (registerCOM) {
		case 1:
			res = UnRegisterCOM();
			CoUninitialize();
            return res;
		case 2:
			res = RegisterCOM(MaxhInstance);
			CoUninitialize();
            return res;
	}

	//-- Register Classes (Runtime)

    _Module.StartMonitor();
	hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
		REGCLS_SINGLEUSE);
    _ASSERTE(SUCCEEDED(hRes));
	
	return res;

}

//-----------------------------------------------------------------------------
// *> StopServer()

void StopServer( ) {
//    _Module.Unlock();
	_Module.RevokeClassObjects();
	Sleep(dwPause);
	_Module.Term();
	CoUninitialize();
}

//-- EOF: comsrv.cpp ----------------------------------------------------------
