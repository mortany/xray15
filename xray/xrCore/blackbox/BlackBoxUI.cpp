#include "BugSlayerUtil.h"
#include "stdafx_.h"
#include <stdio.h>

#define MAX_STACK_TRACE 100

TCHAR g_stackTrace[MAX_STACK_TRACE][4096];
int g_stackTraceCount = 0;

void BuildStackTrace(struct _EXCEPTION_POINTERS* g_BlackBoxUIExPtrs)
{
    for (int i = 0; i != MAX_STACK_TRACE; ++i)
        FillMemory(g_stackTrace[i], 4096, 0);

    const TCHAR* traceDump = GetFirstStackTraceString(GSTSO_MODULE | GSTSO_SYMBOL | GSTSO_SRCLINE, g_BlackBoxUIExPtrs);
    g_stackTraceCount = 0;

    int incr = 85;
    while (NULL != traceDump) {
#ifdef UNICODE
        int length = wcslen(traceDump);
#else
        int length = strlen(traceDump);
#endif

        if (length < 4096) {
 
            lstrcpy(g_stackTrace[g_stackTraceCount], traceDump);
 

		}
        else
        {
            memcpy(g_stackTrace[g_stackTraceCount], traceDump, 4092);
            TCHAR* i = g_stackTrace[g_stackTraceCount] + 4092;
            *i++ = TEXT('.');
            *i++ = TEXT('.');
            *i++ = TEXT('.');
            *i = 0;
        }

        g_stackTraceCount++;

        incr += 2;
        traceDump = GetNextStackTraceString(GSTSO_MODULE | GSTSO_SYMBOL | GSTSO_SRCLINE, g_BlackBoxUIExPtrs);
    }
}

#ifdef _EDITOR
#    pragma auto_inline(off)
DWORD_PTR program_counter()
{
    DWORD_PTR programcounter;

    // Get the return address out of the current stack frame
    __asm mov eax,
        [ ebp + 4 ]
        // Put the return address into the variable we'll return
        __asm mov[programcounter],
        eax

        return programcounter;
}
#    pragma auto_inline(on)
#else // _EDITOR
extern "C" void* _ReturnAddress(void);

#    pragma intrinsic(_ReturnAddress)

#    pragma auto_inline(off)
DWORD_PTR program_counter() { return (DWORD_PTR)_ReturnAddress(); }
#    pragma auto_inline(on)
#endif // _EDITOR

void BuildStackTrace()
{
    CONTEXT context;
    context.ContextFlags = CONTEXT_FULL;

#ifdef _EDITOR
    DWORD* EBP = &context.Ebp;
    DWORD* ESP = &context.Esp;
#endif // _EDITOR

    if (!GetThreadContext(GetCurrentThread(), &context))
        return;

#ifdef _M_X64
    context.Rip = program_counter();
#else
    context.Eip = program_counter();
#    ifndef _EDITOR
    __asm mov context.Ebp, ebp __asm mov context.Esp,
        esp
#    else // _EDITOR
    __asm mov EBP, ebp __asm mov ESP,
        esp
#    endif // _EDITOR
#endif

    EXCEPTION_POINTERS ex_ptrs;
    ex_ptrs.ContextRecord = &context;
    ex_ptrs.ExceptionRecord = 0;

    BuildStackTrace(&ex_ptrs);
}

#ifndef _EDITOR
__declspec(noinline)
#endif // _EDITOR
    void OutputDebugStackTrace(LPCTSTR header)
{
    BuildStackTrace();

    if (header) {
		OutputDebugStringW(header);
		OutputDebugStringW(TEXT(":\r\n"));
    }

    for (int i = 2; i < g_stackTraceCount; ++i) {
        OutputDebugStringW(g_stackTrace[i]);
		OutputDebugStringW(TEXT("\r\n"));
    }
}
