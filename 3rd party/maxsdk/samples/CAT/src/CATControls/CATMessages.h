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

#pragma once

#include "CATDialog.h"
#include "CATFilePaths.h"

#include "macrorec.h"

#include <vector>
#include <fstream>

/////////////////////////////////////////////////////////////////
// class CATMessages
//
// This records errors and warnings in an accessible form,
// and also optionally logs them to the specified file.
//
#define SCRATCH_BUF_SIZE 512

class CATMessages : public MaxHeapOperators
{
protected:
	std::ofstream debug;
	HANDLE hFileAccessMutex;
	int nMutexFails;

	//	std::vector<std::string> strMessages;	// all messages
	//	std::vector<int> nErrors;				// index into strMessages
	//	std::vector<int> nWarnings;				// index into strMessages

	//	TCHAR* strMessages[200000];	// Steve's quick fix hack, maybe something
	//	TCHAR* strMessages[1000];	// will turn up later, but for now I am sick of this!!!
	//	TCHAR* strMessages[1000];

	int nNumMessages;
	int nNumErrors;
	int nNumWarnings;

	TCHAR scratchCharBuffer[SCRATCH_BUF_SIZE + 1];

	void WriteLogMessage(const TCHAR *msg);

public:

	static int iMacroEnableDepth;

	CATMessages();
	CATMessages(const TCHAR *logtothisfile);
	~CATMessages();

	void Debug(const TCHAR* msg, ...);
	void Error(const TCHAR* msg, ...);
	void Warning(const TCHAR* msg, ...);

	void Error(int nLineNumber);
	void Error(int nLineNumber, const TCHAR* msg, ...);
	void Warning(int nLineNumber);
	void Warning(int nLineNumber, const TCHAR* msg, ...);
	//	void Warning(const TCHAR* msg, ...);

	int NumErrors() const { return nNumErrors; }//nErrors.size(); }
	int NumWarnings() const { return nNumWarnings; }//nWarnings.size(); }
	int NumMessages() const { return nNumMessages; }//strMessages.size(); }

	UINT CodePageForSave();

	//	const TCHAR* GetError(int n) const { return strMessages[nErrors[n]]; }
	//	const TCHAR* GetWarning(int n) const { return strMessages[nWarnings[n]]; }
	//	const TCHAR* GetMessage(int n) const { return strMessages[n]; }

	//	CATDialog *DisplayMessages(HINSTANCE hInstance, HWND hWnd) const;
};

// This returns a static CATMessages instance that is included
// in each project that compiles CATMessages.cpp.  Each project
// must have defined a symbol (either CATCONROLS or CATOBJECTS)
// so we can tell which log file to create.  You must define
// exactly one of these.
extern CATMessages &GetCATMessages();

extern int &GetMacroDepth();

#define MACRO_ENABLE	GetMacroDepth()--; macroRecorder->Enable();
#define MACRO_DISABLE	GetMacroDepth()++; macroRecorder->Disable();
