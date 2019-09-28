//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "cat.h"
#include "resource.h"
#include "CATMessages.h"
#include "CatPlugins.h"

// Get handle to listener
#include <maxscript\editor\scripteditor.h>

#define ScriptPrint the_listener->edit_stream->printf

#include <string>

CATMessages &GetCATMessages()
{
	static CATMessages catMessages(GetPlugCFGPath(_T("catcontrols.log")));
	return catMessages;
}

static int iMacroEnableDepth;

int &GetMacroDepth() { return iMacroEnableDepth; }

/////////////////////////////////////////////////////////////////
// CATMessages functions
/////////////////////////////////////////////////////////////////

CATMessages::CATMessages() : nMutexFails(0)
{
	nNumMessages = nNumErrors = nNumWarnings = 0;
	hFileAccessMutex = NULL;
}

CATMessages::CATMessages(const TCHAR *logtothisfile)
{
	nNumMessages = nNumErrors = nNumWarnings = 0;
	hFileAccessMutex = NULL;
	nMutexFails = 0;

	debug.open(logtothisfile);
	if (!debug.good()) {
		// Do nothing?
	}
	else {
		DebugPrint(_T("Log file opened: \"%s\"\n"), logtothisfile);

		debug.sync_with_stdio(true);

		// Create a mutex to control access to the debug file stream.
		// It is initially owned but will not deadlock when calling
		// Debug() (which waits on the mutex), because an owning
		// thread is not permitted to deadlock itself in this manner.
		hFileAccessMutex = CreateMutex(NULL, TRUE, NULL);

		// Insert a comment with a timestamp at the top of the file.
		time_t ltime;
		time(&ltime);
		Debug(_T("CATMessages log file generated on %s"), _tctime(&ltime));

		if (!hFileAccessMutex) {
			Debug(_T("Mutex creation failed for this file.\n"));
		}
		else {
			ReleaseMutex(hFileAccessMutex);
		}

		if (CodePageForSave() == CP_UTF8) {
			unsigned char marker[3];
			marker[0] = 0xEF;
			marker[1] = 0xBB;
			marker[2] = 0xBF;
			debug.write((const char*)marker, 3);
		}
	}
}

CATMessages::~CATMessages()
{
	if (debug.is_open()) debug.close();
	if (hFileAccessMutex) CloseHandle(hFileAccessMutex);
}

UINT CATMessages::CodePageForSave()
{
	Interface14 *iface = GetCOREInterface14();
	if (iface->LegacyFilesCanBeStoredUsingUTF8())
		return CP_UTF8;
	LANGID langID = iface->LanguageToUseForFileIO();
	return iface->CodePageForLanguage(langID);

}
void CATMessages::WriteLogMessage(const TCHAR *msg)
{
	DWORD dwMutexResult;

	if (hFileAccessMutex) {
		dwMutexResult = WaitForSingleObject(hFileAccessMutex, 1000);
		if (dwMutexResult != WAIT_OBJECT_0 && dwMutexResult != WAIT_ABANDONED) {
			nMutexFails++;
			return;
		}
		if (nMutexFails > 0) {
			nMutexFails = 0;
			debug << ("[") << nMutexFails << (" Mutex operations have failed]\n");
		}
	}
	if (debug.good()) {
		TSTR strmsg = msg;
		MaxSDK::Util::MaxString mx_msg;
		strmsg.ToMaxString(mx_msg);
		debug << (const char*)mx_msg.ToCP(CodePageForSave()) << std::flush<char>;
	}
	if (hFileAccessMutex)
		ReleaseMutex(hFileAccessMutex);
}

// The following generate error and warning messages used by
// CATMessages.  If a debug log file is open, the messages
// are also written to the file.
//
void CATMessages::Debug(const TCHAR* msg, ...)
{
	va_list args;
	va_start(args, msg);

	_vstprintf(scratchCharBuffer, msg, args);
	WriteLogMessage(scratchCharBuffer);
}

void CATMessages::Error(const TCHAR* msg, ...)
{
	va_list args;
	va_start(args, msg);
	nNumErrors++;

	_stprintf(scratchCharBuffer, GetString(IDS_ERROR1));
	_vstprintf(&scratchCharBuffer[_tcslen(scratchCharBuffer)], msg, args);

	WriteLogMessage(scratchCharBuffer);
}

void CATMessages::Warning(const TCHAR* msg, ...)
{
	va_list args;
	va_start(args, msg);
	nNumWarnings++;
	_stprintf(scratchCharBuffer, GetString(IDS_WARNING1));
	_vstprintf(&scratchCharBuffer[_tcslen(scratchCharBuffer)], msg, args);
	WriteLogMessage(scratchCharBuffer);
}

void CATMessages::Error(int nLineNumber)
{
	_stprintf(scratchCharBuffer, GetString(IDS_ERROR2), nLineNumber);
	nNumErrors++;
	WriteLogMessage(scratchCharBuffer);
}

void CATMessages::Error(int nLineNumber, const TCHAR* msg, ...)
{
	nNumErrors++;
	va_list args;
	va_start(args, msg);
	_stprintf(scratchCharBuffer, GetString(IDS_ERROR2), nLineNumber);
	_vstprintf(&scratchCharBuffer[_tcslen(scratchCharBuffer)], msg, args);
	va_end(args);
	WriteLogMessage(scratchCharBuffer);
}

void CATMessages::Warning(int nLineNumber)
{
	nNumWarnings++;
	_stprintf(scratchCharBuffer, GetString(IDS_WARNING2), nLineNumber);
	WriteLogMessage(scratchCharBuffer);
}

void CATMessages::Warning(int nLineNumber, const TCHAR* msg, ...)
{
	nNumWarnings++;
	va_list args;
	va_start(args, msg);
	_stprintf(scratchCharBuffer, GetString(IDS_WARNING2), nLineNumber);
	_vstprintf(&scratchCharBuffer[_tcslen(scratchCharBuffer)], msg, args);
	va_end(args);
	WriteLogMessage(scratchCharBuffer);
}

