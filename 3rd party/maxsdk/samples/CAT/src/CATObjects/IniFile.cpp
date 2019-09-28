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

/****************************************************************
 * This file implements the following classes:
 *
 *   IniParser:
 *     A basic class for reading a basic .ini file.
 *
 *   IniFile:
 *     A container for an entire .ini file, allowing keys to be
 *     modified, new keys and groups added, and the file to be
 *     written to disk.  This class uses IniParser to load the
 *     file.
 ****************************************************************/

#include "Max.h"		// included for DbgAssert()
#include "IniFile.h"
#include <maxchar.h>
#include <ctype.h>

#define BUF_SIZE_INIT		100

 //[private]
 //
 // Doubles the size of the string buffer and copies its current contents
 // to the new location (like realloc(), except we always have to move the
 // memory).
 //
 // Returns true if OK; false if there was an error (out of memory)
 //
bool IniParser::ExpandBuffer() {
	TCHAR *newbuf = new TCHAR[bufsize * 2];
	if (!newbuf) return false;
	bufsize += bufsize;
	_tcsnccpy(newbuf, strbuf, buflength);
	delete[] strbuf;
	strbuf = newbuf;
	return true;
}

//[private]
//
// Reads a line from file into the string buffer, expanding it as needed.
// Maintains the line number count.  Handles Unix-style text files by acting
// only on carriage returns and ignoring form-feeds.  All other characters
// are copied (null might screw things up a little though).
//
// Returns true if OK; false if End-of-file.
//
bool IniParser::ReadLine() {
	if (!fp || fp->IsEndOfFile()) return false;

	buflength = 0;
	MaxSDK::Util::Char ch;
	while (!fp->IsEndOfFile()) {
		ch = fp->ReadChar();
		if (ch.IsNull()) break;
		TCHAR tmp;
		ch.ToMCHAR(&tmp, 1);
		if (tmp == _T('\n')) break;
		if (tmp == _T('\f')) continue;
		strbuf[buflength++] = tmp;
		if (buflength == bufsize) {
			bool bufferexpanded = ExpandBuffer();
			DbgAssert(bufferexpanded);
			UNREFERENCED_PARAMETER(bufferexpanded);
		}
	}
	strbuf[buflength] = _T('\0');
	line++;
	return true;
}

//[private]
//
// Parameters:
//   &pos - Position within string buffer
//
// Reads a standard identifier (begins with alpha, followed by any alpha-numeric
// character or the underscore).  Terminates when 'pos' points to a non-identifier
// character or a null character, or is equal to 'buflength' (the length of the
// string buffer).
//
// Returns true if an identifier has been detected; false otherwise
//
bool IniParser::ReadIdentifier(int &pos) {
	if (_istalpha(strbuf[pos])) {
		++pos;
		while (pos < buflength && strbuf[pos] && (_istalnum(strbuf[pos]) || strbuf[pos] == _T('_'))) pos++;
		return true;
	}
	return false;
}

//[private]
//
// Parameters:
//   &pos - Position within string buffer
//
// Reads whitespace in the string buffer, beginning at 'pos'.  Terminates when
// pos points to a non-whitespace character, a null character, or if it is
// equal to 'buflength'
//
void IniParser::ReadWhiteSpace(int &pos) {
	while (pos < buflength && strbuf[pos] && _istspace(strbuf[pos])) pos++;
}

//[private]
//
// Parameters:
//   &pos - Position within string buffer
//
// Reads whitespace in the string buffer, beginning at 'pos' and moving backwards.
// Terminates when pos points to a non-whitespace character, a null character, or
// if we have reached the beginning of the string.
//
void IniParser::ReadWhiteSpaceRev(int &pos) {
	while (pos > 0 && strbuf[pos] && _istspace(strbuf[pos])) pos--;
}

//
// Constructs a new IniParser parser.  A file is opened if specified.  Otherwise it
// must be opened by calling Open().  The string buffer is allocated here.
//
IniParser::IniParser(const TCHAR *file/*=NULL*/)
{
	fp = new MaxSDK::Util::TextFile::Reader();
	bufsize = BUF_SIZE_INIT;
	strbuf = new TCHAR[bufsize];
	strbuf[0] = _T('a');
	strbuf[1] = _T('b');
	strbuf[2] = _T('c');

	buflength = 0;
	line = 0;
	clausetype = iniEmpty;
	// Original open mode was "r" - assuming Text
	if (file) fp->Open(file);
}

//
// Destructs the IniParser parser.  Here we make sure the file has been closed, and
// delete the string buffer.
//
IniParser::~IniParser()
{
	if (fp != NULL)	delete fp;
	if (strbuf != NULL) delete[] strbuf; //Why not delete strbuf????
}

//
// Opens an INI file for parsing.
//
// Returns true if the file was opened successfully; false if there was an error.
// An error could constitute either the file open operation failing, or the string buffer not
// being allocated (which would mean your computer at this stage would already have
// been falling down around you).
//
bool IniParser::Open(const TCHAR *file)
{
	if (!fp || !strbuf) return false;

	if (fp->IsFileOpen()) return true; //from the above function comment, if already opened, should return true

	buflength = 0;
	line = 0;
	clausetype = iniEmpty;

	return fp->Open(file);
}

//
// Closes the file and resets appropriate variables.
//
void IniParser::Close()
{
	if (fp && fp->IsFileOpen()) {
		fp->Close();
		line = 0;
		clausetype = iniEmpty;
	}
}

//
// Returns true if the file is open; false if not.
//
bool IniParser::IsOpen() const {
	return fp->IsFileOpen();
}

//
// Returns true if we have reached the end of the file; false if not.
//
bool IniParser::EndOfFile() const {
	return (fp->IsEndOfFile() || clausetype == iniEOF) ? true : false;
}

//
// This reads a line from the file and does a quick scan through to figure
// out what sort of clause (if any) it is.  The possible return types are:
//
//   iniEmpty    - the clause contains no data
//   iniEOF      - the end of the file has been reached
//   iniError    - there is a syntax error in the clause
//   iniComment  - the clause is a comment
//   iniGroup    - the clause is a group heading
//   iniBadGroup - the clause looks like a group but has bad syntax
//   iniKey      - the clause is an assignment (key=value)
//
// You can extract the text for a comment, group, or key clause by calling:
//
//   GetCommentClause()
//   GetGroupClause()
//   GetKeyClause()
//
IniClauseType IniParser::ReadNextClause()
{
	if (ReadLine()) {
		int pos = 0;
		ReadWhiteSpace(pos);
		clausetype = iniError;

		if (pos == buflength) {
			clausetype = iniEmpty;
		}
		else if (strbuf[pos] == _T('#')) {
			clausetype = iniComment;
		}
		else if (strbuf[pos] == _T('[')) {
			while (pos < buflength && strbuf[pos] != _T(']')) pos++;
			if (strbuf[pos] == _T(']'))
				clausetype = iniGroup;
			else
				clausetype = iniBadGroup;
		}
		else {
			if (ReadIdentifier(pos)) {
				ReadWhiteSpace(pos);
				if (strbuf[pos] == _T('='))
					clausetype = iniKey;
			}
		}
	}
	else {
		clausetype = iniEOF;
	}
	return clausetype;
}

//
// This tokenises a comment clause in the string buffer and points
// 'comment' to the beginning of the comment text (not including the
// '#' character.
//
// Returns true if OK; false if this is not a comment clause
//
// Note: comment points into the string buffer.  It must not be used after a
// subsequent call to ReadNextClause(), or after the class has been destructed,
// and must not be deallocated!
//
bool IniParser::GetCommentClause(TCHAR **comment)
{
	if (clausetype != iniComment) return false;

	// Skip past preamble
	int pos = 0;
	ReadWhiteSpace(pos);
	pos++;

	// Read and terminate the comment
	*comment = &strbuf[pos];
	pos = buflength - 1;
	ReadWhiteSpaceRev(pos);
	strbuf[pos + 1] = _T('\0');

	return true;
}

//
// This tokenises a group clause in the string buffer and points
// 'group' to the beginning of group identifier.  The identifier
// does not include the surrounding square brackets.
//
// Returns true if OK; false if this is not a group clause
//
// Note: group points into the string buffer.  It must not be used after a
// subsequent call to ReadNextClause(), or after the class has been destructed,
// and must not be deallocated!
//
bool IniParser::GetGroupClause(TCHAR **group)
{
	if (clausetype != iniGroup) return false;

	// Skip past preamble
	int pos = 0;
	ReadWhiteSpace(pos);
	pos++; // Skip the '['

	// Read and terminate the group identifier
	*group = &strbuf[pos];
	while (pos < buflength && strbuf[pos] != _T(']')) pos++;
	strbuf[pos] = _T('\0');

	return true;
}

//
// This tokenises a key clause in the string buffer, pointing 'key' and 'val'
// to the key identifier and the value, respectively.  Leading and trailing
// whitespace is stripped from the value.
//
// Returns true if OK; false if this is not a group clause
//
// Note: key and val point into the string buffer.  They must not be used after
// a subsequent call to ReadNextClause(), or after the class has been destructed,
// and must not be deallocated!
//
bool IniParser::GetKeyClause(TCHAR **key, TCHAR **val)
{
	if (clausetype != iniKey) return false;

	// Skip past preamble
	int pos = 0;
	ReadWhiteSpace(pos);

	// Read and terminate the key
	*key = &strbuf[pos];
	ReadIdentifier(pos);

	// Skip past the assignment
	if (strbuf[pos] != _T('=')) {
		strbuf[pos++] = _T('\0');
		ReadWhiteSpace(pos);
		pos++;	// skip '='
	}
	else {
		strbuf[pos++] = _T('\0');
	}

	// Skip to the value
	ReadWhiteSpace(pos);

	// Read and terminate the value
	*val = &strbuf[pos];
	pos = buflength - 1;
	ReadWhiteSpaceRev(pos);
	strbuf[pos + 1] = _T('\0');

	return true;
}

//
// Returns the type of the clause that has just been read by ReadNextClause().
//
enum IniClauseType IniParser::GetClauseType() const {
	return clausetype;
}

//
// Returns the 1-based line number of the clause that has just been read by
// ReadNextClause().  If no clause has been read, or the file is not open,
// the line number will be zero.
//
int IniParser::GetLineNumber() const {
	return line;
}

////////////////////////////////////////////////////////////////////////////////
//[private]
//
// class IniFile::IniGroupRecord
//
// The IniGroupRecord contains a list of pairs for one group,
// and stores the name of the group.  The global (unnamed) group
// uses the value NULL for its name.
//
IniFile::IniGroupRecord::IniGroupRecord(const TCHAR *name/*=NULL*/)
{
	if (name) {
		szName = new TCHAR[_tcslen(name) + 4];
		DbgAssert(szName);
		_tcscpy(szName, name);
	}
	else {
		szName = NULL;
	}
}

IniFile::IniGroupRecord::~IniGroupRecord()
{
	delete[] szName;
	for (unsigned int i = 0; i < vecKeys.size(); i++) {
		delete[] vecKeys[i].first;
		delete[] vecKeys[i].second;
	}
}

int IniFile::IniGroupRecord::NumKeys() const {
	return (int)vecKeys.size();
}

const TCHAR *IniFile::IniGroupRecord::Name() const {
	return szName;
}

int IniFile::IniGroupRecord::AddKey(const IniKeyRecord& key) {
	vecKeys.push_back(key);
	return (int)vecKeys.size() - 1;
}

const IniFile::IniGroupRecord::IniKeyRecord& IniFile::IniGroupRecord::GetKey(int nKey) const {
	DbgAssert(nKey >= 0 && nKey < NumKeys());
	return vecKeys[nKey];
}

IniFile::IniGroupRecord::IniKeyRecord& IniFile::IniGroupRecord::GetKey(int nKey) {
	DbgAssert(nKey >= 0 && nKey < NumKeys());
	return vecKeys[nKey];
}

////////////////////////////////////////////////////////////////////////////////
// class IniFile
//
//

//
//[private]
//
// Clear all ini information, except for the file name!  This is
// called each time the ini file is reloaded, and when the class
// is destroyed.
//
void IniFile::Reset()
{
	for (int nGroup = 0; nGroup < NumGroups(); nGroup++) {
		delete vecGroups[nGroup];
	}
	vecGroups.resize(0);
}

//
//[private]
//
// Returns the record for the specified group
//
const IniFile::IniGroupRecord& IniFile::GetGroup(int nGroup) const
{
	DbgAssert(nGroup >= 0 && nGroup < NumGroups());
	return *vecGroups[nGroup];
}

IniFile::IniGroupRecord& IniFile::GetGroup(int nGroup)
{
	DbgAssert(nGroup >= 0 && nGroup < NumGroups());
	return *vecGroups[nGroup];
}

//
// Constructs the class to operate on the specified file.
//
IniFile::IniFile(const TCHAR *file)
	: vecGroups()
{
	szFileName = NULL;
	bDirty = false;
	SetFile(file);
	Load();
}

//
// KA-BOOM!!!
//
IniFile::~IniFile()
{
	if (szFileName)
	{
		delete[] szFileName;
		szFileName = NULL;
	}
	if (bDirty) Save();
	Reset();
}

//
// Sets the operating .ini file.  NULL is allowed.
//
void IniFile::SetFile(const TCHAR *file)
{
	delete[] szFileName;

	if (file) {
		szFileName = new TCHAR[_tcslen(file) + 4];
		DbgAssert(szFileName);
		_tcscpy(szFileName, file);
		if (NumGroups() > 0) bDirty = true;
	}
	else {
		szFileName = NULL;
	}
}

//
// Opens the ini file and parses every line, storing all groups
// and keys in arrays for future access.  The file is then closed.
//
bool IniFile::Load()
{
	IniParser ini(szFileName);
	if (!ini.IsOpen()) return false;

	Reset();

	int nCurrentGroup = -1;
	int nGroup, nKey;
	TCHAR *szGroup, *szKey, *szValue;
	bool bSkipGroup = false;
	nGroup = 0;

	while (!ini.EndOfFile()) {
		switch (ini.ReadNextClause()) {
		case iniGroup:
			// Grab the group clause and add it to our list of groups.
			// If the group already exists, switch to that group.  When
			// we save out again, the group will not be fragmented.
			ini.GetGroupClause(&szGroup);
			nGroup = AddGroup(szGroup);
			if (nGroup == -1) nGroup = FindGroup(szGroup);
			nCurrentGroup = nGroup;
			bSkipGroup = false;
			break;

		case iniBadGroup:
			bSkipGroup = true;
			break;

		case iniKey:
			if (!bSkipGroup) {
				// Grab the key and its value, and add it to the list of keys
				// in the current group.  If there is no current group, create
				// the global (NULL) group.  If the key has already been given,
				// log an error.
				if (nCurrentGroup == -1) nCurrentGroup = AddGroup(_T(""));
				ini.GetKeyClause(&szKey, &szValue);
				nKey = AddKey(nGroup, szKey, szValue);

				if (nKey == -1) {
					// Log an error.
				}
			}
			break;
		}
	}

	ini.Close();
	bDirty = false;
	return true;
}

bool IniFile::Save()
{
	if (!szFileName) return false;
	MaxSDK::Util::TextFile::Writer testWriter;
	Interface14 *iface = GetCOREInterface14();
	unsigned int encoding = 0;
	if (iface->LegacyFilesCanBeStoredUsingUTF8())
		encoding = CP_UTF8 | MaxSDK::Util::TextFile::Writer::WRITE_BOM;
	else
	{
		LANGID langID = iface->LanguageToUseForFileIO();
		encoding = iface->CodePageForLanguage(langID);
	}

	// Original open mode was "w" - assuming Text
	if (!testWriter.Open(szFileName, false, encoding))
		return false;
	int nNumGroups, nNumKeys;
	int nGroup, nKey;

	for (nNumGroups = NumGroups(), nGroup = 0; nGroup < nNumGroups; nGroup++) {
		const IniGroupRecord& refGroup = GetGroup(nGroup);
		if (refGroup.Name()) {
			// WriteGroup(refGroup.Name());
			testWriter.Write(refGroup.Name());
		}

		for (nNumKeys = refGroup.NumKeys(), nKey = 0; nKey < nNumKeys; nKey++) {
			//WriteKey(refGroup.GetKey(nKey).first, refGroup.GetKey(nKey).second);
			testWriter.Printf(_T("%s=%s\n"), refGroup.GetKey(nKey).first, refGroup.GetKey(nKey).second);
		}
	}

	testWriter.Close();
	bDirty = false;
	return true;
}

//
// The dirty flag tells us whether any key, value, or group has been modified.
// When the class is destroyed, the changes are saved if the flag is set.  If
// you wish to abandon your changes, you can call ClearDirty() before destroying
// the instance.  SetDirty() might have a use, but I'm not sure what.  All
// dirty flag stuff is handled internally.
//
bool IniFile::Dirty() const {
	return bDirty;
}

void IniFile::SetDirty() {
	bDirty = true;
}

void IniFile::ClearDirty() {
	bDirty = false;
}

//
// The following functions are used for enumerating the groups,
// keys and values.
//
int IniFile::NumGroups() const {
	return (int)vecGroups.size();
}

int IniFile::NumKeys(int nGroup) const {
	return GetGroup(nGroup).NumKeys();
}

const TCHAR *IniFile::Group(int nGroup) const {
	return GetGroup(nGroup).Name();
}

const TCHAR *IniFile::Key(int nGroup, int nKey) const {
	return GetGroup(nGroup).GetKey(nKey).first;
}

const TCHAR *IniFile::Value(int nGroup, int nKey) const {
	return GetGroup(nGroup).GetKey(nKey).second;
}

//
// Searches for the named group (case-insensitive) and returns its
// group index.  If the group name is not found, -1 is returned.
//
int IniFile::FindGroup(const TCHAR *szGroup) const
{
	int nNumGroups = NumGroups();
	for (int i = 0; i < nNumGroups; i++) {
		if (!szGroup) {
			if (!GetGroup(i).Name()) return i;
		}
		else {
			if (!_tcsicmp(szGroup, GetGroup(i).Name())) return i;
		}
	}

	return -1;
}

//
// Searches for the named key (case-insensitive) within the given
// group, and returns its key index.  If the key name is not found,
// -1 is returned.
//
int IniFile::FindKey(int nGroup, const TCHAR *szKey) const
{
	const IniGroupRecord& refGroup = GetGroup(nGroup);
	int nNumKeys = NumKeys(nGroup);

	for (int i = 0; i < nNumKeys; i++) {
		if (!_tcsicmp(szKey, refGroup.GetKey(i).first)) return i;
	}

	return -1;
}

//
// Add a new group.  Returns the index of the new group if successful,
// or -1 if the group already exists.  The NULL group can only be added
// if there are no other groups.
//
int IniFile::AddGroup(const TCHAR *szGroup)
{
	// The following line prevents someone from adding the global
	// (NULL) group when other groups already exist.
	if (szGroup == NULL && vecGroups.size() > 0) return -1;

	if (FindGroup(szGroup) != -1) return -1;
	int nGroup = (int)vecGroups.size();

	IniGroupRecord *pNewGroup = new IniGroupRecord(szGroup);
	DbgAssert(pNewGroup);
	vecGroups.push_back(pNewGroup);

	bDirty = true;
	return nGroup;
}

//
// Add a new key within the given group.  Returns the index of the new
// key if successful, or -1 if the key already exists.
//
int IniFile::AddKey(int nGroup, const TCHAR *szKey, const TCHAR *szValue/*=""*/)
{
	DbgAssert(szKey && szValue);
	if (FindKey(nGroup, szKey) != -1) return -1;

	TCHAR *szNewKey = new TCHAR[_tcslen(szKey) + 4];
	TCHAR *szNewValue = new TCHAR[_tcslen(szValue) + 4];
	DbgAssert(szNewKey && szNewValue);
	_tcscpy(szNewKey, szKey);
	_tcscpy(szNewValue, szValue);

	bDirty = true;
	return GetGroup(nGroup).AddKey(IniKeyRecord(szNewKey, szNewValue));
}

//
// Set a key's value.  The key must exist.
//
void IniFile::SetKey(int nGroup, int nKey, const TCHAR *szValue)
{
	TCHAR *szNewValue = new TCHAR[_tcslen(szValue) + 4];
	DbgAssert(szNewValue);
	_tcscpy(szNewValue, szValue);

	delete[] GetGroup(nGroup).GetKey(nKey).second;
	GetGroup(nGroup).GetKey(nKey).second = szNewValue;

	bDirty = true;
}

//
// Set a key's value.  If the key doesn't exist, it is created.
//
void IniFile::SetKey(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szValue)
{
	DbgAssert(szKey && szValue);

	int nGroup = FindGroup(szGroup);
	if (nGroup == -1) nGroup = AddGroup(szGroup);

	int nKey = FindKey(nGroup, szKey);
	if (nKey == -1) AddKey(nGroup, szKey, szValue);
	else SetKey(nGroup, nKey, szValue);
}

//
// Retrieves a key's value and returns a pointer to it.  The key
// must exist.
//
const TCHAR *IniFile::GetKey(int nGroup, int nKey) const
{
	return Value(nGroup, nKey);
}

//
// Retrieves a key's value and returns a pointer to it.  If the
// key doesn't exist, NULL is returned.
//
const TCHAR *IniFile::GetKey(const TCHAR *szGroup, const TCHAR *szKey) const
{
	int nGroup = FindGroup(szGroup);
	if (nGroup == -1) return NULL;
	int nKey = FindKey(nGroup, szKey);
	if (nKey == -1) return NULL;

	return Value(nGroup, nKey);
}
