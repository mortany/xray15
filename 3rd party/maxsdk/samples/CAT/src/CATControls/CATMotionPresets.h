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

// Loads and saves CAT Rig preset scripts.

#pragma once

#include "CATToken.h"
#include "ProgressWindow.h"

 //
 // These are the types of clause that come out of CATPresetReader
 //
typedef enum tagCatPresetClauseType CatPresetClauseType;
enum tagCatPresetClauseType {
	motionNothing,
	motionBeginBranch,
	motionEndBranch,
	motionValue,
	motionEnd,
	motionAbort
};

//
// These are types of identifier that come out of CATPresetReader
//
enum CATMotionPresetTypes {
	//------------------------- PresetParams
	idCATMotionPresetVersion = idStartYourIdentifiersHere,
	//-------------------------
	idBranch,
	idValue
};

/////////////////////////////////////////////////////////////////
// class CATPresetReader
//
//
//
const TSTR cstrEmptyString(_T(""));

class CATPresetReader {
private:
	tstringstream instream;
	TSTR filename;

	CATMessages errors;

	unsigned int nLineNumber;
	unsigned int nBranchLevel;

	ULONG nFileSize;
	ULONG nCharsRead;
	IProgressWindow *pProgressWindow;
	ULONG nNextProgressUpdateChars;
	ULONG nProgressIncrementChars;

	std::vector<TSTR> branchstack;

	DWORD				dwFileVersion;
	USHORT				curIdentifier;
	CatPresetClauseType	curClause;
	CatTokenType		curType;
	TSTR				curName;

	TCHAR* strOldNumericLocale;

	CATToken*	thisToken;
	CATToken*	nextToken;
	CATToken	tokens[2];

	CATToken* GetNextToken();
	void AssertExpectedGot(CatTokenType tokExpected, CatTokenType tokGot);
	void AssertSyntaxError();

	BOOL ReadClause();
	void SkipToEOL();
	void CalcFileSize();

public:
	CATPresetReader(const TCHAR *filename);
	~CATPresetReader();

	BOOL ok() { return (instream.good() && curClause != motionAbort); }
	ULONG FileSize() const { return nFileSize; }
	ULONG NumCharsRead() const { return nCharsRead; }
	void ShowProgress(TCHAR *szProgressMessage);

	DWORD GetVersion() const { return dwFileVersion; }
	const CATMessages* GetErrors() const { return &errors; }

	BOOL NextClause() { return ReadClause(); }
	USHORT CurIdentifier() { return curIdentifier; }
	CatPresetClauseType CurClauseID() { return curClause; }
	int CurBranchLevel() { return nBranchLevel; }
	const TSTR& CurBranchName() { return nBranchLevel > 0 ? branchstack[nBranchLevel - 1] : cstrEmptyString; }
	const TSTR& CurName() { return curName; }

	void SkipBranch();

	float GetValue() { return thisToken->asFloat(); }

	void WarnNoSuchPresetBranch() { };
	void WarnNoSuchBranch() {};
};

/////////////////////////////////////////////////////////////////
// class CATPresetWriter
//
//
//
class CATPresetWriter {
private:
	tstringstream outStream;
	TSTR filename;

	int nIndentLevel;
	tstring strIndent;

	TCHAR* strOldNumericLocale;

	void Indent() { nIndentLevel++; strIndent = tstring(nIndentLevel * 2, _T(' ')); }
	void Outdent() { if (--nIndentLevel < 0) nIndentLevel = 0; strIndent = tstring(nIndentLevel * 2, _T(' ')); }

public:
	CATPresetWriter();
	CATPresetWriter(const TCHAR *filename);
	~CATPresetWriter();

	BOOL Open(const TCHAR *filename);
	void Close();
	BOOL ok() const { return outStream.good(); }

	BOOL BeginBranch(const TSTR& name);
	BOOL EndBranch();

	BOOL Comment(const TCHAR *msg, ...);
	BOOL WriteValue(const TSTR& name, float val);
	BOOL FromController(Control* ctrl, TimeValue t, const TSTR& name);

};
