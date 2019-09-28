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

#include "CATPlugins.h"
#include "CATMotionPresets.h"
#include "Locale.h"
#include "ProgressWindow.h"
#include "maxtextfile.h"

 //
 // Dictionary for all our preset file identifiers, including internal
 // identifiers used for tokenising.
 //
static DictionaryEntry dictMotionEntries[] = {
	//  <NAME>					<IDENTIFIER ID>			<IDENTIFIER TYPE>		<FOLLOWING TYPE>
	//---------------------------------------------------------------------------------------------- Global stuff
		_T("Version"),				idCATMotionPresetVersion,	tokIdentifier,			tokInt,
		//---------------------------------------------------------------------------------------------- Blah
			_T("Branch"),				idBranch,				tokGroup,				tokNothing,
			_T("Value"),				idValue,				tokIdentifier,			tokNothing,
			//--<END OF TABLE>------------------------------------------------------------------------------ END OF TABLE
				NULL,					idNothing,				tokNothing,				tokNothing
};

// This builds a dictionary for our tokeniser / parser.
static Dictionary catMotionDictionary(dictMotionEntries);

//
// This is used for error message generation.
//
static const TCHAR *MotionIdentName(int id) {
	DictionaryEntry* entry = catMotionDictionary.GetEntry(id);
	DbgAssert(entry != NULL);
	return entry->name;
}

/////////////////////////////////////////////////////////////////
// CATPresetReader functions
/////////////////////////////////////////////////////////////////

CATPresetReader::CATPresetReader(const TCHAR *filename)
	: branchstack(),
	errors(),
	instream()
{
	// GB 24-Oct-03: Save current numeric locale and set to standard C.
	// This cures our number translation problems in central europe.
	strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));

	curIdentifier = idNothing;
	curClause = motionNothing;
	curType = tokNothing;

	thisToken = &tokens[0];
	nextToken = &tokens[1];
	thisToken->SetDictionary(&catMotionDictionary);
	nextToken->SetDictionary(&catMotionDictionary);

	pProgressWindow = NULL;
	nNextProgressUpdateChars = 0;
	nProgressIncrementChars = 0;

	dwFileVersion = 0;
	nFileSize = 0;
	nCharsRead = 0;
	nLineNumber = 1;
	nBranchLevel = 0;
	branchstack.resize(16);

	// Open file.  If it fails, set the current clause
	// to rigAbort and DbgAssert an error.  Otherwise,
	// Read the first token in the file, so we're all
	// ready for a call to ReadClause().

	//Use TextFile::Reader to read the file in full.In case the file is too big to read in full, read one line in a loop

	MaxSDK::Util::TextFile::Reader fileReader;
	// Original file open mode is Text
	if (!fileReader.Open(filename)) {
		curClause = motionAbort;
		errors.Error(0, _T("%s \"%s\""), GetString(IDS_NOFILEOPEN), filename);
	}
	else {
		while (!fileReader.IsEndOfFile()) {
			TSTR fullContent = fileReader.ReadLine();
			const TCHAR* pt = fullContent.data();
			for (int i = 0; i < fullContent.Length(); i++)
				instream.put(pt[i]);
		}
		CalcFileSize();
		GetNextToken();
	}
}

CATPresetReader::~CATPresetReader() {
	//	if (instream.is_open()) instream.close();
	RestoreLocale(LC_NUMERIC, strOldNumericLocale);
	if (pProgressWindow) pProgressWindow->ProgressEnd();
}

// Calculates the size of the file
void CATPresetReader::CalcFileSize()
{
	std::stringstream::pos_type nInitialPos = instream.tellg();
	instream.seekg(0, std::ios_base::end);
	nFileSize = (ULONG)instream.tellg();
	instream.seekg(nInitialPos, std::ios_base::beg);

	// Init progress bar stuff.
	nProgressIncrementChars = nFileSize / 64;
	if (nProgressIncrementChars <= 0)
		nProgressIncrementChars = 1;	// would be a very strange case.

	nNextProgressUpdateChars = min(nProgressIncrementChars, nFileSize);
}

//
// Turns progress bar on.
//
void CATPresetReader::ShowProgress(TCHAR *szProgressMessage)
{
	UNREFERENCED_PARAMETER(szProgressMessage);
	if (!pProgressWindow) {
		// Initialise the progress bar.
		pProgressWindow = GetProgressWindow();
		if (!pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS), GetString(IDS_LOADING)))
			pProgressWindow = NULL;
	}
}

// This function is a bit confusing.  It reads the next
// token in the stream into nextToken.  That means, the
// result of the previous call to GetNextToken() will be
// in thisToken.  However, the function returns thisToken
// because it's usually the one we're interested in.
CATToken* CATPresetReader::GetNextToken()
{
	if (thisToken->type == tokEOL) nLineNumber++;
	std::swap(thisToken, nextToken);

	do {
		nextToken->ReadToken(instream);
		nCharsRead += nextToken->NumCharsRead();
	} while (nextToken->type == tokComment);

	return thisToken;
}

/////////////////////////////////////////////////////////////////
// This is the guts of our parser.  It reads useful clauses from
// the rig file and provides a GetValue() function to grab them.
// The possible clauses are listed below:
//
//   motionBeginBranch     looks like "Branch <string> {"
//   motionEndBranch       looks like "}"
//   motionValue           looks like "Value <string> <float>"
//   motionEnd             means we're done
//   motionAbort           means something messed up big-time
//
// To read the next clause, call NextClause().  There is no
// return value, but the user can access any error messages
// by calling GetErrors().
//
// We find out what type of clause we're looking at by calling
// CurClauseID().  The two forms motionBeginBranch and motionValue
// have an associated string, which can be found by calling
// CurName().
//
// Some type-checking is done, plus some very basic error
// recovery (skipping to the next line).  All-up, it's best not
// to try writing a file that will confuse this parser...  Let's
// just all try to be nice.
//
BOOL CATPresetReader::ReadClause()
{
	// This strange wee loop condition means that if there
	// are errors, we read all clauses until the end of the
	// file.  This is because there's no point returning a
	// clause or any successive clauses once we've had an
	// error cos it's probably going to have some incorrect,
	// corrupted or missing information.  But we still parse
	// the whole file to pick up all the errors we can.
	bool done = false;
	while (curClause != motionEnd && curClause != motionAbort && (!done || errors.NumErrors() > 0)) {
		// Automatic branch level increment from the last call
		// to this function, which was a BeginBranch clause.
		if (curClause == motionBeginBranch) nBranchLevel++;

		done = false;
		while (!done) {
			GetNextToken();

			// First examine thisToken.  If we've a line separator,
			// just loop again and get the next token because we're
			// looking for a clause beginning at thisToken.  If it's
			// end-of-file, we exit the loop.
			//
			if (thisToken->isEOL()) continue;
			else if (thisToken->isEOF()) {
				if (nBranchLevel != 0) {
					errors.Error(nLineNumber, GetString(IDS_ERR_CURLYBRACE));
				}
				curClause = motionEnd;
				done = true;
				continue;
			}

			// Look at the token.  The only one we're interested in
			// here is close-curly.
			//
			switch (thisToken->type) {
				// If ending a branch, decrement the branch level
				// immediately.  If the branch level is already at
				// ground-zero, we give an error bitching about how
				// there's too many close curlies.
			case tokCloseCurly:
				curClause = motionEndBranch;
				done = true;

				if (nBranchLevel > 0) {
					nBranchLevel--;
				}
				else {
					errors.Error(nLineNumber, GetString(IDS_ERR_NOCURLY));
				}
				continue;
			}

			// Look at the identifier.  It should be either a branch
			// or a value.  In both cases we expect a string as the
			// next token.  If this is not present, we do some basic
			// error recovery.
			//
			switch (thisToken->id) {
				// If we get a branch identifier, it means we must open
				// a new branch.  We expect a string to follow, and
				// that followed by an open curly.  If we have a string
				// and no curly, complain about an expected curly.  If
				// we have a curly in the string's place, complain about
				// a missing string.  If we have neither string nor
				// curly complain about an expected branch, skip to the
				// end of the line, and continue parsing.
				//
			case idBranch:
				curIdentifier = thisToken->id;
				curClause = motionBeginBranch;
				curType = thisToken->type;

				GetNextToken();

				switch (thisToken->type) {
				case tokString:
					curName = thisToken->asString().c_str();

					// Branch anyway.  This looks like a branch clause at
					// least a little (if not completely).
					if (nBranchLevel >= branchstack.size())
						branchstack.resize(branchstack.size() + 16);
					branchstack[nBranchLevel] = curName;

					done = true;

					switch (nextToken->type) {
					case tokOpenCurly:
						// This is a correctly formed branch.  Skip past
						// the curly brace.
						GetNextToken();
						break;

					case tokCloseCurly:
						// Probably a typo.
						errors.Error(nLineNumber, GetString(IDS_ERR_TYPO), TokenName(tokCloseCurly), TokenName(tokOpenCurly));
						GetNextToken();
						break;

					default:
						// It's something else weird.  Don't skip past it.
						// If the curly is simply missing, it won't make a
						// difference.  If it's something else that should
						// be skipped past, it'll generate a different
						// error next time.  We can't think of everything!
						AssertExpectedGot(tokOpenCurly, nextToken->type);
						break;
					}
					break;

				case tokOpenCurly:
					// Missing the string!  It's okay though, we just whinge
					// about it and still branch.
					errors.Error(nLineNumber, GetString(IDS_ERR_MISSINGBNAME));
					curName = _T("");

					// Branch.
					if (nBranchLevel >= branchstack.size())
						branchstack.resize(branchstack.size() + 16);
					branchstack[nBranchLevel] = curName;

					done = true;
					break;

				default:
					// Dunno what this is!  We bitch and then skip to EOL.
					AssertSyntaxError();
					SkipToEOL();
				}
				break;

				// If we get a value identifier, we want a string to follow,
				// and then a float value.  An integer will be acceptable I
				// suppose.  Could even be fancy and use the TransmuteTo()
				// function on tokens but I don't think that's necessary.
				// Same deal as the branches when things are missing.
			case idValue:
				curIdentifier = thisToken->id;
				curClause = motionValue;
				curType = thisToken->type;

				GetNextToken();

				switch (thisToken->type) {
					// If we have a string and the next token is
					// a float or int, we accept.  Otherwise we
					// just whine and continue without skipping the
					// token.
				case tokString:
					curName = thisToken->asString().c_str();

					switch (nextToken->type) {
					case tokInt:
					case tokFloat:
						GetNextToken();
						done = true;
						break;

					default:
						AssertExpectedGot(tokFloat, nextToken->type);
					}
					break;

					// If we have an int or a float, assume that
					// the string is missing.  Don't skip the
					// token.
				case tokInt:
				case tokFloat:
					curName = _T("");
					errors.Error(nLineNumber, GetString(IDS_ERR_MISSINGEXP), TokenName(tokString), MotionIdentName(idValue));
					done = true;
					break;

					// Otherwise check if the second token is an int
					// or float.  Then just complain about the first
					// token and how it should be a string.  Chances
					// are that it's a string but they forgot to put
					// quotes around it (I know I did - that's why
					// this case is now handled.  Grrrrr).
				default:
					if (nextToken->type == tokInt || nextToken->type == tokFloat) {
						errors.Error(nLineNumber, GetString(IDS_ERR_FIRSTPARAM), MotionIdentName(idValue));
						GetNextToken();
					}
					else {
						AssertSyntaxError();
						SkipToEOL();
					}
				}
				break;

				// Up to this point everything legal or seemingly
				// legal has been handled.  If we are here, there's
				// a problem.  Issue a syntax error, skip the line,
				// and be done.
			default:
				AssertSyntaxError();
				SkipToEOL();
			}
		}
	}

	// Update progress bar
	if (pProgressWindow && nCharsRead >= nNextProgressUpdateChars) {
		nNextProgressUpdateChars = min(nCharsRead + nProgressIncrementChars, nFileSize);
		pProgressWindow->SetPercent((100 * nCharsRead) / nFileSize);
	}

	return errors.NumErrors() == 0 && ok();
}

// Skips to EOL (or EOF), so that the token is stored in
// thisToken.
void CATPresetReader::SkipToEOL()
{
	do {
		GetNextToken();
	} while (!thisToken->isEOLorEOF());
}

// This skips an entire branch, including its subbranchs.  If
// the current identifier is a beginbranch, we skip it and all its
// subbranchs.  Otherwise we skip the rest of the branch, including
// its subbranchs.
void CATPresetReader::SkipBranch()
{
	unsigned int nTargetLevel = 0;

	if (curClause == motionBeginBranch) {
		// The branch level hasn't yet been incremented and
		// won't be until the next call to ReadClause(),
		// so the current level is the target.
		nTargetLevel = nBranchLevel;
		errors.Warning(nLineNumber, GetString(IDS_SKIPPINGBRANCH), MotionIdentName(curIdentifier));
	}
	else {
		// The target level is one below the current level.
		// if the current level is zero, it's okay because
		// we'll finish when motionEnd is hit.
		errors.Warning(nLineNumber, GetString(IDS_SKIPPINGBRANCHREST), branchstack[nBranchLevel - 1].data());
	}

	// This part does the skipping.  Note that errors 
	// in the clauses being skipped will still show up.
	while (curClause != motionEnd && curClause != motionAbort) {
		ReadClause();
		if (nBranchLevel == nTargetLevel) break;
	}
}

void CATPresetReader::AssertExpectedGot(CatTokenType tokExpected, CatTokenType tokGot)
{
	errors.Error(nLineNumber, GetString(IDS_ERR_EXPECTED), TokenName(tokExpected), TokenName(tokGot));
}

void CATPresetReader::AssertSyntaxError()
{
	errors.Error(nLineNumber, GetString(IDS_ERR_SYNTAX));
}

/////////////////////////////////////////////////////////////////
// CATPresetWriter functions
/////////////////////////////////////////////////////////////////

CATPresetWriter::CATPresetWriter()
	: strIndent(_T(""))
{
	nIndentLevel = 0;

	// GB 24-Oct-03: Save current numeric locale and set to standard C.
	// This cures our number translation problems in central europe.
	strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));
}

CATPresetWriter::CATPresetWriter(const TCHAR *filename)
	: strIndent(_T(""))
{
	nIndentLevel = 0;

	// GB 24-Oct-03: Save current numeric locale and set to standard C.
	// This cures our number translation problems in central europe.
	strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));

	//	Open(filename);
	this->filename = filename;
}

CATPresetWriter::~CATPresetWriter()
{
	//	if (outStream.is_open()) outStream.close();
	Close();
	RestoreLocale(LC_NUMERIC, strOldNumericLocale);
}

BOOL CATPresetWriter::Open(const TCHAR *filename)
{
	UNREFERENCED_PARAMETER(filename);
	//	outStream.open(filename);
	return outStream.good();
}

void CATPresetWriter::Close()
{
	if (filename.Length() <= 2) return;

	MaxSDK::Util::TextFile::Writer outfileWriter;
	Interface14 *iface = GetCOREInterface14();
	unsigned int encoding = 0;
	if (iface->LegacyFilesCanBeStoredUsingUTF8())
		encoding = CP_UTF8 | MaxSDK::Util::TextFile::Writer::WRITE_BOM;
	else
	{
		LANGID langID = iface->LanguageToUseForFileIO();
		encoding = iface->CodePageForLanguage(langID);
	}

	if (!outfileWriter.Open(filename.data(), false, encoding)) {
		GetCATMessages().Error(0, GetString(IDS_ERR_NOFILEWRITE), filename.data());
	}
	else {

		outStream.seekg(0, std::ios_base::beg);
		TCHAR c;
		bool done = false;
		while (!done) {
			if (outStream.eof() || outStream.fail()) {
				done = true;
				continue;
			}
			outStream.get(c);
			outfileWriter.WriteChar(c);
		}

		outfileWriter.Close();
	}
}

BOOL CATPresetWriter::BeginBranch(const TSTR& name)
{
	outStream << strIndent << MotionIdentName(idBranch) << _T(" \"") << name.data() << _T("\" {") << std::endl;
	Indent();
	return outStream.good();
}

BOOL CATPresetWriter::EndBranch()
{
	Outdent();
	outStream << strIndent << _T("}") << std::endl;
	return outStream.good();
}

BOOL CATPresetWriter::Comment(const TCHAR *msg, ...)
{
	TCHAR scratch[200] = { 0 };
	va_list args;
	va_start(args, msg);
	_stprintf(scratch, _T("# "));
	_vstprintf(&scratch[_tcslen(scratch)], msg, args);
	va_end(args);
	if (outStream.good()) outStream << (scratch) << std::endl;
	return outStream.good();
}

BOOL CATPresetWriter::WriteValue(const TSTR& name, float val)
{
	outStream << strIndent << MotionIdentName(idValue) << _T(" \"") << name.data() << _T("\" ") << val << std::endl;
	return outStream.good();
}

BOOL CATPresetWriter::FromController(Control* ctrl, TimeValue t, const TSTR& name)
{
	Interval valid = FOREVER;
	if (ctrl) {
		float val;
		ctrl->GetValue(t, (void*)&val, valid, CTRL_ABSOLUTE);
		outStream << strIndent << MotionIdentName(idValue) << _T(" \"") << name.data() << _T("\" ") << val << std::endl;
	}
	return outStream.good();
}
