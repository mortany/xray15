/**********************************************************************
*<
FILE: sysinfo.cpp

DESCRIPTION: 

CREATED BY: Larry Minton

HISTORY: Created 15 April 2007

*>	Copyright (c) 2007, All Rights Reserved.
**********************************************************************/

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/util/script_resource_file_utils.h>
#include "MXSAgni.h"

#include "assetmanagement\AssetUser.h"
#include "AssetManagement\iassetmanager.h"
using namespace MaxSDK::AssetManagement;

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

using namespace MAXScript;


// ============================================================================

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include "sysInfo_wraps.h"

#include "agnidefs.h"

// -----------------------------------------------------------------------------------------

Value*
	getProcessAffinity()
{	
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(res);
	vl.res = &undefined;
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	DWORD_PTR ProcessAffinityMask, SystemAffinityMask;
	if (GetProcessAffinityMask(hProcess,&ProcessAffinityMask, &SystemAffinityMask))
		vl.res = IntegerPtr::intern(ProcessAffinityMask);
	CloseHandle( hProcess );
	return_value_tls (vl.res);
}
Value*
	setProcessAffinity(Value* val)
{
	DWORD ProcessAffinityMask = val->to_int();
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processID);
	SetProcessAffinityMask(hProcess,ProcessAffinityMask);
	CloseHandle( hProcess );
	return val;
}

Value*
	getSystemAffinity()
{
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(res);
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	DWORD_PTR ProcessAffinityMask, SystemAffinityMask;
	if (GetProcessAffinityMask(hProcess,&ProcessAffinityMask, &SystemAffinityMask))
		vl.res = IntegerPtr::intern(SystemAffinityMask);
	CloseHandle( hProcess );
	return_value_tls ( vl.res );
}
Value*
	setSystemAffinity(Value* val)
{
	throw RuntimeError (_T("Cannot set system affinity"));
	return val;
}

Value*
	getDesktopSize()
{
	int wScreen = GetValueUIUnscaled(GetScreenWidth());
	int hScreen = GetValueUIUnscaled(GetScreenHeight());
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new Point2Value((float)wScreen,(float)hScreen) );
}

Value*
	getDesktopSizeUnscaled()
{
	int wScreen = GetScreenWidth();
	int hScreen = GetScreenHeight();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new Point2Value((float)wScreen,(float)hScreen) );
}

Value*
	getDesktopBPP()
{
	HDC hdc = GetDC(GetDesktopWindow());
	int bits = GetDeviceCaps(hdc,BITSPIXEL);
	ReleaseDC(GetDesktopWindow(),hdc);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern(bits) );
}

Value*
	getSystemMemoryInfo_cf(Value** arg_list, int count)
{
	check_arg_count(getSystemMemoryInfo, 0, count);
	MEMORYSTATUS ms;
	ms.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&ms);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array (7);
	vl.result->append(Integer64::intern(ms.dwMemoryLoad));   // percent of memory in use
	vl.result->append(Integer64::intern(ms.dwTotalPhys)); // bytes of physical memory
	vl.result->append(Integer64::intern(ms.dwAvailPhys)); // free physical memory bytes
	vl.result->append(Integer64::intern(ms.dwTotalPageFile));// bytes of paging file
	vl.result->append(Integer64::intern(ms.dwAvailPageFile));// free bytes of paging file
	vl.result->append(Integer64::intern(ms.dwTotalVirtual)); // user bytes of address space
	vl.result->append(Integer64::intern(ms.dwAvailVirtual)); // free user bytes
	return_value_tls(vl.result);
}

Value*
getCommandLine_cf(Value** arg_list, int count)
{
	check_arg_count(getCommandLine, 0, count);
	TCHAR* commandLine = GetCommandLine();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(new String(commandLine));
}

Value*
getCommandLineArgs_cf(Value** arg_list, int count)
{
	check_arg_count(getCommandLineArgs, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	int argCount = __argc;
	vl.result = new Array(argCount);
	for (int i = 1; i < argCount; i++)
	{
		TCHAR* arg = (__targv[i]);
		vl.result->append(new String(arg));
	}
	return_value_tls(vl.result);
}

//-----------------------------------------------------------------------------
// Structure for GetProcessMemoryInfo()
//
// This is from psapi.h which is only included in the Platform SDK

typedef struct _PROCESS_MEMORY_COUNTERS {
	DWORD cb;
	DWORD PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

//-- This is in psapi.dll (NT4 and W2k only)

typedef BOOL (WINAPI *GetProcessMemoryInfo)(HANDLE Process,PROCESS_MEMORY_COUNTERS* ppsmemCounters,DWORD cb);

GetProcessMemoryInfo getProcessMemoryInfo;
//-----------------------------------------------------------------------------

Value*
	getMAXMemoryInfo_cf(Value** arg_list, int count)
{
	check_arg_count(getMAXMemoryInfo, 0, count);

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	HMODULE hPsapi = LoadLibrary(_T("psapi.dll"));
	BOOL resOK = FALSE;
	if (hPsapi) {
		getProcessMemoryInfo = (GetProcessMemoryInfo)GetProcAddress(hPsapi,"GetProcessMemoryInfo");
		if (getProcessMemoryInfo) {
			PROCESS_MEMORY_COUNTERS pmc;
			int processID = GetCurrentProcessId();
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
			if (getProcessMemoryInfo(hProcess,&pmc,sizeof(pmc))) {
				vl.result = new Array (9);
				vl.result->append(Integer64::intern(pmc.PageFaultCount));
				vl.result->append(Integer64::intern(pmc.PeakWorkingSetSize));
				vl.result->append(Integer64::intern(pmc.WorkingSetSize));
				vl.result->append(Integer64::intern(pmc.QuotaPeakPagedPoolUsage));
				vl.result->append(Integer64::intern(pmc.QuotaPagedPoolUsage));
				vl.result->append(Integer64::intern(pmc.QuotaPeakNonPagedPoolUsage));
				vl.result->append(Integer64::intern(pmc.QuotaNonPagedPoolUsage));
				vl.result->append(Integer64::intern(pmc.PagefileUsage));
				vl.result->append(Integer64::intern(pmc.PeakPagefileUsage));
				resOK=TRUE;
			}
			CloseHandle( hProcess );
		}
		FreeLibrary(hPsapi);
	}
	if (resOK) 	return_value_tls(vl.result);
	return_value_tls(&undefined);// LAM - 5/18/01 - was: return &undefined;
}

Value*
	getMAXPriority()
{
	def_process_priority_types();
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	int priority=GetPriorityClass(hProcess);
	CloseHandle( hProcess );
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( ConvertIDToValue(processPriorityTypes, elements(processPriorityTypes), priority) );
}

Value*
	setMAXPriority(Value* val)
{
	def_process_priority_types();
	int type = ConvertValueToID(processPriorityTypes, elements(processPriorityTypes), val);
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processID);
	SetPriorityClass(hProcess,type);
	CloseHandle( hProcess );
	return val;
}

Value*
	getusername()
{
	// There seems to be a runtime bug with GetUserName where it can fail with
	// error code of ERROR_DLL_INIT_FAILED (1114). Seems to occur in Win7+ debug
	// build when GetUserName is called during startup in static ctors. Works then,
	// but then at some later point stops working.
	// //social.msdn.microsoft.com/Forums/windowsdesktop/en-US/2e2e80b6-bc98-44ab-97c0-2f12cd8a74ed/getusername-returns-1114-error
	TCHAR username[MAX_PATH] = {0};
	DWORD namesize = MAX_PATH;
	GetUserName(username,&namesize);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String (username) );
}

Value*
	getcomputername()
{
	TCHAR computername[MAX_COMPUTERNAME_LENGTH+1] = {0};
	DWORD namesize = MAX_COMPUTERNAME_LENGTH+1;
	GetComputerName(computername,&namesize);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String (computername) );
}

Value*
	getSystemDirectory()
{
	TCHAR sysDir[MAX_PATH] = {0};
	GetSystemDirectory(sysDir,MAX_PATH);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String (sysDir) );
}

Value*
	getWinDirectory()
{
	TCHAR winDir[MAX_PATH] = {0};
	GetWindowsDirectory(winDir,MAX_PATH);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String (winDir) );
}

Value*
	getTempDirectory()
{
	TCHAR tempDir[MAX_PATH] = {0};
	GetTempPath(MAX_PATH,tempDir);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String (tempDir) );
}

Value*
	getCurrentDirectory()
{
	TCHAR currentDir[MAX_PATH] = {0};
	GetCurrentDirectory(MAX_PATH,currentDir);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String (currentDir) );
}

Value*
	setCurrentDirectory(Value* val)
{
	DispInfo info;
	GetUnitDisplayInfo(&info);
	TSTR dirName = val->to_filename();
	AssetId assetId;
	if (IAssetManager::GetInstance()->StringToAssetId(dirName, assetId))
		dirName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();
	if (!SetCurrentDirectory(dirName))
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_SETTING_CURRENT_DIRECTORY), val);
	return val;
}

Value*
	getCPUcount()
{
	SYSTEM_INFO     si;
	GetSystemInfo(&si);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern(si.dwNumberOfProcessors) );
}

Value*
	getLanguage_cf(Value** arg_list, int count)
{
	// sysinfo.getLanguage user:true
	check_arg_count_with_keys(getLanguage, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array (3);
	LANGID wLang;
	Value* tmp;
	bool user = bool_key_arg(user,tmp,FALSE) == TRUE;
	if (user)
		wLang = GetUserDefaultLangID();
	else
		wLang = GetSystemDefaultLangID();
	vl.result->append(Integer::intern(PRIMARYLANGID(wLang)));
	vl.result->append(Integer::intern(SUBLANGID(wLang)));

	// Fill in language description
	const int kBUF_LEN = 256;
	TCHAR tmpString[kBUF_LEN] = {0};
	DWORD res = VerLanguageName(wLang, tmpString, kBUF_LEN);
	if (res != 0)
	{
		vl.result->append(new String(tmpString));
	}
	else 
	{
		vl.result->append(&undefined); 
	}

	// Fill in system's 2-letter localization language and country names in the form <language>-<country>
	vl.result->append(new String(GetSystemLocaleName(user)));

	return_value_tls(vl.result);
}

Value*
	getMaxLanguage_cf(Value** arg_list, int count)
{
	// sysinfo.getMaxLanguage 
	check_arg_count(getMaxLanguage, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array (4);

	// Fill in language and sub-language ids
	WORD wLang = MaxSDK::Util::GetLanguageID();
	vl.result->append(Integer::intern(PRIMARYLANGID(wLang)));
	vl.result->append(Integer::intern(SUBLANGID(wLang)));

	// Fill in TLA of language
	vl.result->append(new String(MaxSDK::Util::GetLanguageTLA()));

	// Fill in language description
	const int kBUF_LEN = 256;
	TCHAR tmpString[kBUF_LEN] = {0};
	// Will truncate description to kBUF_LEN
	DWORD res = VerLanguageName(wLang, tmpString, kBUF_LEN); 
	if (0 != res)
	{
		vl.result->append(new String(tmpString));
	}
	else 
	{
		vl.result->append(&undefined); 
	}

	// Fill in Max's 2-letter localization language and country names in the form <language>-<country>
	vl.result->append(new String(MaxSDK::Util::GetLanguagePackDirName()));

	return_value_tls(vl.result);
}

Value*
getMAXHandleCount_cf(Value** arg_list, int count)
{
	// sysinfo.getMAXHandleCount()
	DWORD handleCount = 0;
	GetProcessHandleCount(GetCurrentProcess(), &handleCount);
	return_value(Integer::intern(handleCount));
}

Value*
getMAXUserObjectCount_cf(Value** arg_list, int count)
{
	// sysinfo.getMAXUserObjectCount()
	DWORD obj_count = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
	return_value(Integer::intern(obj_count));
}

Value*
getMAXGDIObjectCount_cf(Value** arg_list, int count)
{
	// sysinfo.getMAXGDIObjectCount()
	DWORD obj_count = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
	return_value(Integer::intern(obj_count));
}

/* --------------------- plug-in init --------------------------------- */
// this is called by the dlx initializer, register the global vars here
void sysInfo_init()
{
#include "sysInfo_glbls.h"
}
