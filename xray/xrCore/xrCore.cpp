// xrCore.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"


#include <mmsystem.h>
#include <objbase.h>
#include "xrCore.h"
 
#pragma comment(lib,"winmm.lib")

#ifdef DEBUG
#	include	<malloc.h>
#endif // DEBUG

XRCORE_API		xrCore	Core;
XRCORE_API		u32		build_id;
XRCORE_API		LPCTSTR	build_date;

namespace CPU
{
	extern	void			Detect	();
};

static u32	init_counter	= 0;

extern TCHAR g_application_path[256];

//. extern xr_vector<shared_str>*	LogFile;

void xrCore::_initialize	(LPCTSTR _ApplicationName, LogCallback cb, BOOL init_fs, LPCTSTR fs_fname)
{
	#ifdef UNICODE
    wcscpy_s(ApplicationName, _ApplicationName);
	#else
    strcpy_s(ApplicationName, _ApplicationName);
	#endif

	if (0==init_counter) {
#ifdef XRCORE_STATIC	
		_clear87	();
		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
		_control87	( _RC_NEAR, MCW_RC );
		_control87	( _MCW_EM,  MCW_EM );
#endif
		// Init COM so we can use CoCreateInstance
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		#ifdef UNICODE
                wcscpy_s(Params, sizeof(Params), GetCommandLine());
                _wcslwr_s(Params, sizeof(Params));
		#else
                strcpy_s(Params, sizeof(Params), GetCommandLine());
                _strlwr_s(Params, sizeof(Params));
		#endif


		string_path		fn,dr,di;

		// application path
        GetModuleFileName(GetModuleHandle(MODULE_NAME),fn,sizeof(fn));
		#ifdef UNICODE
        _wsplitpath(fn, dr, di, 0, 0);
		#else
        _splitpath(fn, dr, di, 0, 0);
		#endif

 
        strconcat(sizeof(ApplicationPath), ApplicationPath, dr, di);
 


#ifndef _EDITOR
		#ifdef UNICODE
        wcscpy_s(g_application_path, sizeof(g_application_path), ApplicationPath);
		#else
        strcpy_s(g_application_path, sizeof(g_application_path), ApplicationPath);
		#endif

#endif

#ifdef _EDITOR
		// working path
        if( strstr(Params,"-wf") )
        {
            string_path				c_name;
            sscanf					(strstr(Core.Params,"-wf ")+4,"%[^ ] ",c_name);
            SetCurrentDirectory     (c_name);
        }
#endif

		GetCurrentDirectory(sizeof(WorkingPath),WorkingPath);

		// User/Comp Name
		DWORD	sz_user		= sizeof(UserName);
		GetUserName			(UserName,&sz_user);

		DWORD	sz_comp		= sizeof(CompName);
		GetComputerName		(CompName,&sz_comp);

		// Mathematics & PSI detection
		CPU::Detect			();
		
		#ifdef UNICODE
                Memory._initialize(wcsstr(Params, TEXT("-mem_debug")) ? TRUE : FALSE);
		#else
                Memory._initialize(strstr(Params, TEXT("-mem_debug")) ? TRUE : FALSE);
		#endif
		DUMP_PHASE;

		InitLog				();
		_initialize_cpu		();

//		Debug._initialize	();

		rtc_initialize		();

		xr_FS				= xr_new<CLocatorAPI>	();

		xr_EFS				= xr_new<EFS_Utils>		();
//.		R_ASSERT			(co_res==S_OK);
	}
	if (init_fs){
		u32 flags			= 0;
		#ifdef UNICODE 
		            if (0 != wcsstr(Params, TEXT("-build")))
                    flags |= CLocatorAPI::flBuildCopy;
                if (0 != wcsstr(Params, TEXT("-ebuild")))
                    flags |= CLocatorAPI::flBuildCopy | CLocatorAPI::flEBuildCopy;
		#else
                if (0 != strstr(Params, TEXT("-build")))
                    flags |= CLocatorAPI::flBuildCopy;
                if (0 != strstr(Params, TEXT("-ebuild")))
                    flags |= CLocatorAPI::flBuildCopy | CLocatorAPI::flEBuildCopy;
		#endif

#ifdef DEBUG
				#ifdef UNICODE
                if (wcsstr(Params, TEXT("-cache")))
                    flags |= CLocatorAPI::flCacheFiles;
				#else
                if (strstr(Params, TEXT("-cache")))
                    flags |= CLocatorAPI::flCacheFiles;
				#endif

		else flags &= ~CLocatorAPI::flCacheFiles;
#endif // DEBUG
#ifdef _EDITOR // for EDITORS - no cache
		flags 				&=~ CLocatorAPI::flCacheFiles;
#endif // _EDITOR
		flags |= CLocatorAPI::flScanAppRoot;

#ifndef	_EDITOR
	#ifndef ELocatorAPIH
		if (0!=wcsstr(Params,TEXT("-file_activity")))	 flags |= CLocatorAPI::flDumpFileActivity;
	#endif
#endif
		FS._initialize		(flags,0,fs_fname);
		Msg					(TEXT("'%s' build %d, %s\n","xrCore"),build_id, build_date);
		EFS._initialize		();
#ifdef DEBUG
    #ifndef	_EDITOR
		Msg					("CRT heap 0x%08x",_get_heap_handle());
		Msg					("Process heap 0x%08x",GetProcessHeap());
    #endif
#endif // DEBUG
	}
	SetLogCB				(cb);
	init_counter++;
}

#ifndef	_EDITOR
#include "compression_ppmd_stream.h"
extern compression::ppmd::stream	*trained_model;
#endif
void xrCore::_destroy		()
{
	--init_counter;
	if (0==init_counter){
		FS._destroy			();
		EFS._destroy		();
		xr_delete			(xr_FS);
		xr_delete			(xr_EFS);

#ifndef	_EDITOR
		if (trained_model) {
			void			*buffer = trained_model->buffer();
			xr_free			(buffer);
			xr_delete		(trained_model);
		}
#endif

		Memory._destroy		();
	}
}

#ifndef XRCORE_STATIC

//. why ??? 
#ifdef _EDITOR
	BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#else
	BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#endif
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			_clear87		();
			_control87		( _PC_53,   MCW_PC );
			_control87		( _RC_CHOP, MCW_RC );
			_control87		( _RC_NEAR, MCW_RC );
			_control87		( _MCW_EM,  MCW_EM );
		}
//.		LogFile.reserve		(256);
		break;
	case DLL_THREAD_ATTACH:
		timeBeginPeriod	(1);
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#ifdef USE_MEMORY_MONITOR
		memory_monitor::flush_each_time	(true);
#endif // USE_MEMORY_MONITOR
		break;
	}
    return TRUE;
}
#endif // XRCORE_STATIC