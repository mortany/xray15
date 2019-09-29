#include "stdafx.h"
#include "xrdebug.h"
#include "os_clipboard.h"
#include "../../3rd party/DxErr/src/dxerr.h"
#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#include <direct.h>
#pragma warning(pop)
#include <intrin.h>
#ifdef __BORLANDC__
    #	include "d3d9.h"
    #	include "d3dx9.h"
    #	include "D3DX_Wrapper.h"
    #	pragma comment(lib,"EToolsB.lib")
    #	define DEBUG_INVOKE	DebugBreak()
        static BOOL			bException	= TRUE;
    #   define USE_BUG_TRAP
#else
    #   define USE_BUG_TRAP
    #	define DEBUG_INVOKE	__debugbreak()
        static BOOL			bException	= FALSE;
#endif
#ifndef USE_BUG_TRAP
#	include <exception>
#endif // #ifndef USE_BUG_TRAP
#include <dbghelp.h>						// MiniDump flags
#ifdef USE_BUG_TRAP
#	include <BugTrap/BugTrap.h>				// for BugTrap functionality
    #ifdef __BORLANDC__
        #	pragma comment(lib,"BugTrapB.lib")		// Link to ANSI DLL
    #endif
#endif // USE_BUG_TRAP
#include <new.h>							// for _set_new_mode
#include <signal.h>							// for signals
#include <Shellapi.h>

#ifdef DEBUG
#	define USE_OWN_ERROR_MESSAGE_WINDOW
#else // DEBUG
#	define USE_OWN_MINI_DUMP
#endif // DEBUG

XRCORE_API	xrDebug		Debug;

static bool	error_after_dialog = false;

extern void BuildStackTrace();
extern char g_stackTrace[100][4096];
extern int	g_stackTraceCount;
extern bool shared_str_initialized;

void LogStackTrace	(LPCTSTR header)
{
	if (!shared_str_initialized)
		return;

	BuildStackTrace	();		

	Msg				(TEXT("%s"),header);

	for (int i=1; i<g_stackTraceCount; ++i)
		Msg			(TEXT("%s"),g_stackTrace[i]);
}

void xrDebug::gather_info		(LPCTSTR expression, LPCTSTR description, LPCTSTR argument0, LPCTSTR argument1, LPCTSTR file, int line, LPCTSTR function, LPTSTR assertion_info)
{
	LPTSTR				buffer = assertion_info;
	LPCTSTR				endline = TEXT("\n");
	LPCTSTR				prefix = TEXT("[error]");
	bool				extended_description = (description && !argument0 && wcschr(description,'\n'));
	for (int i=0; i<2; ++i) {
		if (!i)
			buffer		+= wprintf(buffer,TEXT("%sFATAL ERROR%s%s"),endline,endline,endline);
		buffer			+= wprintf(buffer,TEXT("%sExpression    : %s%s"),prefix,expression,endline);
		buffer			+= wprintf(buffer,TEXT("%sFunction      : %s%s"),prefix,function,endline);
		buffer			+= wprintf(buffer,TEXT("%sFile          : %s%s"),prefix,file,endline);
		buffer			+= wprintf(buffer,TEXT("%sLine          : %d%s"),prefix,line,endline);
		
		if (extended_description) {
			buffer		+= wprintf(buffer,TEXT("%s%s%s"),endline,description,endline);
			if (argument0) {
				if (argument1) {
					buffer	+= wprintf(buffer,TEXT("%s%s"),argument0,endline);
					buffer	+= wprintf(buffer,TEXT("%s%s"),argument1,endline);
				}
				else
					buffer	+= wprintf(buffer,TEXT("%s%s"),argument0,endline);
			}
		}
		else {
			buffer		+= wprintf(buffer,TEXT("%sDescription   : %s%s"),prefix,description,endline);
			if (argument0) {
				if (argument1) {
					buffer	+= wprintf(buffer,TEXT("%sArgument 0    : %s%s"),prefix,argument0,endline);
					buffer	+= wprintf(buffer,TEXT("%sArgument 1    : %s%s"),prefix,argument1,endline);
				}
				else
					buffer	+= wprintf(buffer,TEXT("%sArguments     : %s%s"),prefix,argument0,endline);
			}
		}

		buffer			+= wprintf(buffer,TEXT("%s"),endline);
		if (!i) {
			if (shared_str_initialized) {
				Msg		(TEXT("%s"),assertion_info);
				FlushLog();
			}
			buffer		= assertion_info;
			endline		= TEXT("\r\n");
			prefix		= TEXT("");
		}
	}

#ifdef USE_MEMORY_MONITOR
	memory_monitor::flush_each_time	(true);
	memory_monitor::flush_each_time	(false);
#endif // USE_MEMORY_MONITOR

	if (!IsDebuggerPresent() && !wcsstr(GetCommandLine(),TEXT("-no_call_stack_assert"))) {
		if (shared_str_initialized)
			Msg			(TEXT("stack trace:\n"));

#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
		buffer			+= wprintf(buffer,TEXT("stack trace:%s%s"),endline,endline);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW

		BuildStackTrace	();		

		for (int i=2; i<g_stackTraceCount; ++i) {
			if (shared_str_initialized)
				Msg		(TEXT("%s"),g_stackTrace[i]);

#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
			buffer		+= wprintf(buffer,TEXT("%s%s"),g_stackTrace[i],endline);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW
		}

		if (shared_str_initialized)
			FlushLog	();

		os_clipboard::copy_to_clipboard	(assertion_info);
	}
}

void xrDebug::do_exit	(const std::wstring &message)
{
	FlushLog			();
	MessageBoxW			(NULL,message.c_str(),TEXT("Error"),MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
	TerminateProcess	(GetCurrentProcess(),1);
}

void xrDebug::backend	(LPCTSTR expression, LPCTSTR description, LPCTSTR argument0, LPCTSTR argument1, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	static xrCriticalSection CS
#ifdef PROFILE_CRITICAL_SECTIONS
	(MUTEX_PROFILE_ID(xrDebug::backend))
#endif // PROFILE_CRITICAL_SECTIONS
	;

	CS.Enter			();

	error_after_dialog	= true;

	string4096			assertion_info;

	gather_info			(expression, description, argument0, argument1, file, line, function, assertion_info);

#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
	LPCTSTR				endline = TEXT("\r\n");
	LPTSTR				buffer = assertion_info + xr_strlen(assertion_info);
	buffer				+= wprintf(buffer,TEXT("%sPress CANCEL to abort execution%s"),endline,endline);
	buffer				+= wprintf(buffer,TEXT("Press TRY AGAIN to continue execution%s"),endline);
	buffer				+= wprintf(buffer,TEXT("Press CONTINUE to continue execution and ignore all the errors of this type%s%s"),endline,endline);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW

	if (handler)
		handler			();

	if (get_on_dialog())
		get_on_dialog()	(true);

	FlushLog();

#ifdef XRCORE_STATIC
	MessageBox			(NULL,assertion_info,TEXT("X-Ray error"),MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
#else
	HWND wnd = GetActiveWindow();
	if (wnd == NULL)
		wnd = GetForegroundWindow();
	ShowWindow(wnd, SW_MINIMIZE);
	while (ShowCursor(TRUE) < 0);
#	ifdef USE_OWN_ERROR_MESSAGE_WINDOW
		int					result = 
			MessageBox(
				wnd,
				assertion_info,
				TEXT("Fatal Error"),
				MB_CANCELTRYCONTINUE|MB_ICONERROR|MB_SYSTEMMODAL
			);

		switch (result) {
			case IDCANCEL : {
#		ifdef USE_BUG_TRAP
				BT_SetUserMessage	(assertion_info);
#		endif // USE_BUG_TRAP
				DEBUG_INVOKE;
				break;
			}
			case IDTRYAGAIN : {
				error_after_dialog	= false;
				break;
			}
			case IDCONTINUE : {
				error_after_dialog	= false;
				ignore_always	= true;
				break;
			}
			default : NODEFAULT;
		}
#	else // USE_OWN_ERROR_MESSAGE_WINDOW
#		ifdef USE_BUG_TRAP
			BT_SetUserMessage	(assertion_info);
#		endif // USE_BUG_TRAP
#       ifndef DEBUG
			if (wcsstr(GetCommandLine(),TEXT("-show_log")))
				ShellExecute(nullptr, nullptr, logFullName(), nullptr, nullptr, SW_SHOWNORMAL);
			//TerminateProcess(GetCurrentProcess(), 1);
			DEBUG_INVOKE;
#       else
		DEBUG_INVOKE;
#       endif
#	endif // USE_OWN_ERROR_MESSAGE_WINDOW
	ShowCursor(FALSE);
#endif

	if (get_on_dialog())
		get_on_dialog()	(false);

	CS.Leave			();
}

LPCTSTR xrDebug::error2string	(long code)
{
	LPCTSTR				result	= 0;
	static	string1024	desc_storage;

#ifdef _M_AMD64
#else
	result				= DXGetErrorString(code);
#endif
	if (0==result) 
	{
		FormatMessage	(FORMAT_MESSAGE_FROM_SYSTEM,0,code,0,desc_storage,sizeof(desc_storage)-1,0);
		result			= desc_storage;
	}
	return		result	;
}

void xrDebug::error		(long hr, LPCTSTR  expr, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	backend		(error2string(hr),expr,0,0,file,line,function,ignore_always);
}

void xrDebug::error		(long hr, LPCTSTR  expr, LPCTSTR  e2, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	backend		(error2string(hr),expr,e2,0,file,line,function,ignore_always);
}

void xrDebug::fail		(LPCTSTR e1, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	backend		(TEXT("assertion failed"),e1,0,0,file,line,function,ignore_always);
}

#ifdef UNICODE
void xrDebug::fail(LPCTSTR e1, const std::wstring& e2, LPCTSTR file, int line, LPCTSTR function, bool& ignore_always)
{
    backend(e1, e2.c_str(), 0, 0, file, line, function, ignore_always);
}
#else
void xrDebug::fail(LPCTSTR e1, const std::string& e2, LPCTSTR file, int line, LPCTSTR function, bool& ignore_always)
{
    backend(e1, e2.c_str(), 0, 0, file, line, function, ignore_always);
}
#endif




void xrDebug::fail		(LPCTSTR e1, LPCTSTR e2, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	backend		(e1,e2,0,0,file,line,function,ignore_always);
}

void xrDebug::fail		(LPCTSTR e1, LPCTSTR e2, LPCTSTR e3, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	backend		(e1,e2,e3,0,file,line,function,ignore_always);
}

void xrDebug::fail		(LPCTSTR e1, LPCTSTR e2, LPCTSTR e3, LPCTSTR e4, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always)
{
	backend		(e1,e2,e3,e4,file,line,function,ignore_always);
}

void __cdecl xrDebug::fatal(LPCTSTR file, int line, LPCTSTR function, LPCTSTR  F,...)
{
	string1024	buffer;

	va_list		p;
	//va_start	(p,F);
	//vwprintf	(buffer,F,p);
	//va_end		(p);

	//bool		ignore_always = true;

	//backend		(TEXT("fatal error","<no expression>"),buffer,0,file,line,function,ignore_always);
}

int out_of_memory_handler	(size_t size)
{
	Memory.mem_compact		();
#ifndef _EDITOR
	u32						crt_heap		= mem_usage_impl((HANDLE)_get_heap_handle(),0,0);
#else // _EDITOR
	u32						crt_heap		= 0;
#endif // _EDITOR
	u32						process_heap	= mem_usage_impl(GetProcessHeap(),0,0);
	int						eco_strings		= (int)g_pStringContainer->stat_economy			();
	int						eco_smem		= (int)g_pSharedMemoryContainer->stat_economy	();
	Msg						(TEXT("* [x-ray]: crt heap[%d K], process heap[%d K]"),crt_heap/1024,process_heap/1024);
	Msg						(TEXT("* [x-ray]: economy: strings[%d K], smem[%d K]"),eco_strings/1024,eco_smem);
	Debug.fatal				(DEBUG_INFO,TEXT("Out of memory. Memory request: %d K"),size/1024);
	return					1;
}

extern LPCTSTR log_name();

XRCORE_API string_path g_bug_report_file;

void CALLBACK PreErrorHandler	(INT_PTR)
{
#ifdef USE_BUG_TRAP
	if (!xr_FS || !FS.m_Flags.test(CLocatorAPI::flReady))
		return;

	string_path				log_folder;

	__try {
		FS.update_path		(log_folder,TEXT("$logs$"),TEXT(""));
		if ((log_folder[0] != '\\') && (log_folder[1] != ':')) {
			string256		current_folder;
			_wgetcwd			(current_folder,sizeof(current_folder));
			
			string256		relative_path;
			wcscpy_s		(relative_path,log_folder);
			strconcat		(sizeof(log_folder),log_folder,current_folder,TEXT("\\"),relative_path);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		wcscpy_s				(log_folder,TEXT("logs"));
	}

	string_path				temp;
	strconcat				(sizeof(temp), temp, log_folder, log_name());
	BT_AddLogFile			(temp);

	if (*g_bug_report_file)
		BT_AddLogFile		(g_bug_report_file);

	BT_SaveSnapshot			( 0 );
#endif // USE_BUG_TRAP
}

#ifdef USE_BUG_TRAP
void SetupExceptionHandler	(const bool &dedicated)
{
	BT_InstallSehFilter		();
#if 1//ndef USE_OWN_ERROR_MESSAGE_WINDOW
	if (!dedicated && !wcsstr(GetCommandLine(),TEXT("-silent_error_mode")))
		BT_SetActivityType	(BTA_SHOWUI);
	else
		BT_SetActivityType	(BTA_SAVEREPORT);
#else // USE_OWN_ERROR_MESSAGE_WINDOW
	BT_SetActivityType		(BTA_SAVEREPORT);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW

	BT_SetDialogMessage				(
		BTDM_INTRO2,
		TEXT("\
This is X-Ray Engine v1.5 crash reporting client. \
To help the development process, \
please Submit Bug or save report and email it manually (button More...).\
\r\nMany thanks in advance and sorry for the inconvenience."
	));

	BT_SetPreErrHandler		(PreErrorHandler,0);
	BT_SetAppName			(TEXT("XRay Engine"));
	BT_SetReportFormat		(BTRF_TEXT);
	BT_SetFlags				(/**/BTF_DETAILEDMODE | /**BTF_EDIETMAIL | /**/BTF_ATTACHREPORT /**| BTF_LISTPROCESSES /**| BTF_SHOWADVANCEDUI /**| BTF_SCREENCAPTURE/**/);

	u32 const minidump_flags	=
#ifndef MASTER_GOLD
		(
			MiniDumpWithDataSegs |
//			MiniDumpWithFullMemory |
//			MiniDumpWithHandleData |
//			MiniDumpFilterMemory |
//			MiniDumpScanMemory |
//			MiniDumpWithUnloadedModules |
#	ifndef _EDITOR
			MiniDumpWithIndirectlyReferencedMemory |
#	endif // _EDITOR
//			MiniDumpFilterModulePaths |
//			MiniDumpWithProcessThreadData |
//			MiniDumpWithPrivateReadWriteMemory |
//			MiniDumpWithoutOptionalData |
//			MiniDumpWithFullMemoryInfo |
//			MiniDumpWithThreadInfo |
//			MiniDumpWithCodeSegs |
			0
		);
#else // #ifndef MASTER_GOLD
		!dedicated ?
		MiniDumpNoDump :
		(
			MiniDumpWithDataSegs |
//			MiniDumpWithFullMemory |
//			MiniDumpWithHandleData |
//			MiniDumpFilterMemory |
//			MiniDumpScanMemory |
//			MiniDumpWithUnloadedModules |
#	ifndef _EDITOR
			MiniDumpWithIndirectlyReferencedMemory |
#	endif // _EDITOR
//			MiniDumpFilterModulePaths |
//			MiniDumpWithProcessThreadData |
//			MiniDumpWithPrivateReadWriteMemory |
//			MiniDumpWithoutOptionalData |
//			MiniDumpWithFullMemoryInfo |
//			MiniDumpWithThreadInfo |
//			MiniDumpWithCodeSegs |
			0
		);
#endif // #ifndef MASTER_GOLD

	BT_SetDumpType			(minidump_flags);
	//BT_SetSupportEMail		("cs-crash-report@stalker-game.com");
//	BT_SetSupportServer		(TEXT("localhost"), 9999);
//	BT_SetSupportURL		("www.gsc-game.com");
}
#endif // USE_BUG_TRAP

#if 1
extern void BuildStackTrace(struct _EXCEPTION_POINTERS *pExceptionInfo);
typedef LONG WINAPI UnhandledExceptionFilterType(struct _EXCEPTION_POINTERS *pExceptionInfo);
typedef LONG ( __stdcall *PFNCHFILTFN ) ( EXCEPTION_POINTERS * pExPtrs ) ;
extern "C" BOOL __stdcall SetCrashHandlerFilter ( PFNCHFILTFN pFn );

static UnhandledExceptionFilterType	*previous_filter = 0;

#ifdef USE_OWN_MINI_DUMP
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
										 );

void save_mini_dump			(_EXCEPTION_POINTERS *pExceptionInfo)
{
	return;
}
#endif // USE_OWN_MINI_DUMP

void format_message	(LPTSTR buffer, const u32 &buffer_size)
{
    LPVOID		message;
    DWORD		error_code = GetLastError(); 

	if (!error_code) {
		*buffer	= 0;
		return;
	}

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&message,
        0,
		NULL
	);

	wprintf		(buffer,TEXT("[error][%8d]    : %s"),error_code,message);
    LocalFree	(message);
}

LONG WINAPI UnhandledFilter	(_EXCEPTION_POINTERS *pExceptionInfo)
{
	string256				error_message;
	format_message			(error_message,sizeof(error_message));

	if (!error_after_dialog && !wcsstr(GetCommandLine(),TEXT("-no_call_stack_assert"))) {
		CONTEXT				save = *pExceptionInfo->ContextRecord;
		BuildStackTrace		(pExceptionInfo);
		*pExceptionInfo->ContextRecord = save;

		if (shared_str_initialized)
			Msg				(TEXT("stack trace:\n"));

		if (!IsDebuggerPresent())
		{
			os_clipboard::copy_to_clipboard	(TEXT("stack trace:\r\n\r\n"));
		}

		string4096			buffer;
		for (int i=0; i<g_stackTraceCount; ++i) {
			if (shared_str_initialized)
				Msg			(TEXT("%s"),g_stackTrace[i]);
			wprintf			(buffer,TEXT("%s\r\n"),g_stackTrace[i]);
#ifdef DEBUG
			if (!IsDebuggerPresent())
				os_clipboard::update_clipboard(buffer);
#endif // #ifdef DEBUG
		}

		if (*error_message) {
			if (shared_str_initialized)
				Msg			(TEXT("\n%s"),error_message);

			wcscat			(error_message,TEXT("\r\n"));
#ifdef DEBUG
			if (!IsDebuggerPresent())
				os_clipboard::update_clipboard(buffer);
#endif // #ifdef DEBUG
		}
	}

	if (shared_str_initialized)
		FlushLog			();

#ifndef USE_OWN_ERROR_MESSAGE_WINDOW
#	ifdef USE_OWN_MINI_DUMP
		save_mini_dump		(pExceptionInfo);
#	endif // USE_OWN_MINI_DUMP
#else // USE_OWN_ERROR_MESSAGE_WINDOW
	if (!error_after_dialog) {
		if (Debug.get_on_dialog())
			Debug.get_on_dialog()	(true);

		MessageBox			(NULL,TEXT("Fatal error occured\n\nPress OK to abort program execution","Fatal error"),MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
	}
#endif // USE_OWN_ERROR_MESSAGE_WINDOW

	if (!previous_filter) {
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
		if (Debug.get_on_dialog())
			Debug.get_on_dialog()	(false);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW

		return				(EXCEPTION_CONTINUE_SEARCH) ;
	}

	previous_filter			(pExceptionInfo);

#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
	if (Debug.get_on_dialog())
		Debug.get_on_dialog()		(false);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW

	return					(EXCEPTION_CONTINUE_SEARCH) ;
}
#endif

//////////////////////////////////////////////////////////////////////
#ifdef M_BORLAND
	namespace std{
		extern new_handler _RTLENTRY _EXPFUNC set_new_handler( new_handler new_p );
	};

	static void __cdecl def_new_handler() 
    {
		FATAL		("Out of memory.");
    }

    void	xrDebug::_initialize		(const bool &dedicated)
    {
		handler							= 0;
		m_on_dialog						= 0;
        std::set_new_handler			(def_new_handler);	// exception-handler for 'out of memory' condition
//		::SetUnhandledExceptionFilter	(UnhandledFilter);	// exception handler to all "unhandled" exceptions
    }
#else

#ifndef USE_BUG_TRAP
	void _terminate		()
	{
		if (strstr(GetCommandLine(),"-silent_error_mode"))
			exit				(-1);

		string4096				assertion_info;
		
		Debug.gather_info			(
		//gather_info				(
			TEXT("<no expression>"),
			TEXT("Unexpected application termination"),
			0,
			0,
	#ifdef ANONYMOUS_BUILD
			TEXT(""),
			0,
	#else
			__FILE__,
			__LINE__,
	#endif
	#ifndef _EDITOR
			__FUNCTION__,
	#else // _EDITOR
			TEXT(""),
	#endif // _EDITOR
			assertion_info
		);
		
		LPCTSTR					endline = TEXT("\r\n");
		LPTSTR					buffer = assertion_info + xr_strlen(assertion_info);
		buffer					+= wprintf(buffer,TEXT("Press OK to abort execution%s"),endline);

		MessageBox				(
			GetTopWindow(NULL),
			assertion_info,
			TEXT("Fatal Error"),
			MB_OK|MB_ICONERROR|MB_SYSTEMMODAL
		);
		
		exit					(-1);
	//	FATAL					("Unexpected application termination");
	}
#endif // USE_BUG_TRAP

	static void handler_base				(LPCTSTR reason_string)
	{
		bool							ignore_always = false;
		Debug.backend					(
			TEXT("error handler is invoked!"),
			reason_string,
			0,
			0,
			DEBUG_INFO,
			ignore_always
		);
	}

	static void invalid_parameter_handler	(
			const wchar_t *expression,
			const wchar_t *function,
			const wchar_t *file,
			unsigned int line,
			uintptr_t reserved
		)
	{
		bool							ignore_always = false;

		string4096						expression_;
		string4096						function_;
		string4096						file_;
		size_t							converted_chars = 0;
//		errno_t							err = 
		if (expression)
			wcscpy_s(expression_, TEXT(""));

		if (function)
			wcscpy_s(function_, TEXT(__FUNCTION__));
			

		if (file)
		{
			line = __LINE__;
			wcscpy_s(file_, TEXT(__FILE__));
		}

		Debug.backend					(
			TEXT("error handler is invoked!"),
			expression_,
			0,
			0,
			file_,
			line,
			function_,
			ignore_always
		);
	}

	static void pure_call_handler			()
	{
		handler_base					(TEXT("pure virtual function call"));
	}

#ifdef CS_USE_EXCEPTIONS
	static void unexpected_handler			()
	{
		handler_base					("unexpected program termination");
	}
#endif // CS_USE_EXCEPTIONS

	static void abort_handler				(int signal)
	{
		handler_base					(TEXT("application is aborting"));
	}

	static void floating_point_handler		(int signal)
	{
		handler_base					(TEXT("floating point error"));
	}

	static void illegal_instruction_handler	(int signal)
	{
		handler_base					(TEXT("illegal instruction"));
	}

//	static void storage_access_handler		(int signal)
//	{
//		handler_base					("illegal storage access");
//	}

	static void termination_handler			(int signal)
	{
		handler_base					(TEXT("termination with exit code 3"));
	}

	void debug_on_thread_spawn			()
	{
#ifdef USE_BUG_TRAP
		BT_SetTerminate					();
#else // USE_BUG_TRAP
//		std::set_terminate				(_terminate);
#endif // USE_BUG_TRAP

		_set_abort_behavior				(0,_WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		signal							(SIGABRT,		abort_handler);
		signal							(SIGABRT_COMPAT,abort_handler);
		signal							(SIGFPE,		floating_point_handler);
		signal							(SIGILL,		illegal_instruction_handler);
		signal							(SIGINT,		0);
//		signal							(SIGSEGV,		storage_access_handler);
		signal							(SIGTERM,		termination_handler);

		_set_invalid_parameter_handler	(&invalid_parameter_handler);

		_set_new_mode					(1);
		_set_new_handler				(&out_of_memory_handler);
//		std::set_new_handler			(&std_out_of_memory_handler);

		_set_purecall_handler			(&pure_call_handler);

#if 0// should be if we use exceptions
		std::set_unexpected				(_terminate);
#endif
	}

    void	xrDebug::_initialize		(const bool &dedicated)
    {
		*g_bug_report_file				= 0;

		debug_on_thread_spawn			();


#ifdef USE_BUG_TRAP
		SetupExceptionHandler			(dedicated);
#endif // USE_BUG_TRAP
		previous_filter					= ::SetUnhandledExceptionFilter(UnhandledFilter);	// exception handler to all "unhandled" exceptions

#if 0
		struct foo {static void	recurs	(const u32 &count)
		{
			if (!count)
				return;

			_alloca			(4096);
			recurs			(count - 1);
		}};
		foo::recurs			(u32(-1));
		std::terminate		();
#endif // 0
	}
#endif