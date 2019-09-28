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
#pragma once

#include <vector>
#include "maxtextfile.h"
enum IniClauseType {
	iniEmpty,
	iniEOF,
	iniError,
	iniComment,
	iniGroup,
	iniBadGroup,
	iniKey
};

class IniParser : public MaxHeapOperators
{
private:
	MaxSDK::Util::TextFile::Reader *fp;

	TCHAR*	strbuf;
	int		bufsize;
	int		buflength;
	int		line;

	enum IniClauseType clausetype;

	// Functions that operate on the string buffer
	bool ExpandBuffer();
	bool ReadLine();
	bool ReadIdentifier(int &pos);
	void ReadWhiteSpace(int &pos);
	void ReadWhiteSpaceRev(int &pos);

public:

	IniParser(const TCHAR *file = NULL);
	~IniParser();

	bool Open(const TCHAR *file);
	void Close();
	bool IsOpen() const;
	bool EndOfFile() const;

	IniClauseType ReadNextClause();
	bool GetCommentClause(TCHAR **comment);
	bool GetGroupClause(TCHAR **group);
	bool GetKeyClause(TCHAR **key, TCHAR **val);

	enum IniClauseType GetClauseType() const;
	int GetLineNumber() const;
};

class IniFile : public MaxHeapOperators
{
private:
	TCHAR *szFileName;
	bool bDirty;

	class IniGroupRecord {
	public:
		typedef std::pair<TCHAR*, TCHAR*> IniKeyRecord;

		IniGroupRecord(const TCHAR *name = NULL);
		~IniGroupRecord();

		const TCHAR *Name() const;
		int NumKeys() const;

		int AddKey(const IniKeyRecord& key);
		const IniKeyRecord& GetKey(int nKey) const;
		IniKeyRecord& GetKey(int nKey);

	private:
		TCHAR *szName;
		std::vector<IniKeyRecord> vecKeys;
	};

	typedef IniGroupRecord::IniKeyRecord IniKeyRecord;
	std::vector<IniGroupRecord*> vecGroups;

	void Reset();

	const IniGroupRecord& GetGroup(int nGroup) const;
	IniGroupRecord& GetGroup(int nGroup);

public:
	IniFile(const TCHAR *file = NULL);
	~IniFile();

	// Sets the operating filename.  NULL is allowed.
	void SetFile(const TCHAR *file);

	// Loads and saves using the operating filename.
	bool Load();
	bool Save();

	bool Dirty() const;
	void SetDirty();
	void ClearDirty();

	int NumGroups() const;
	int NumKeys(int nGroup) const;

	// Retrieve groups, keys, or values as strings.
	const TCHAR *Group(int nGroup) const;
	const TCHAR *Key(int nGroup, int nKey) const;
	const TCHAR *Value(int nGroup, int nKey) const;

	// Retrieve group or key indexes.  Returns -1 if the group or
	// key does not exist.
	int FindGroup(const TCHAR *szGroup) const;
	int FindKey(int nGroup, const TCHAR *szKey) const;

	// Add a group or key.  Each returns the index of the new
	// group or key, or -1 if the given name already exists.
	int AddGroup(const TCHAR *szGroup);
	int AddKey(int nGroup, const TCHAR *szKey, const TCHAR *szValue = _T(""));

	// Set a key's value.  The key must exist.
	void SetKey(int nGroup, int nKey, const TCHAR *szValue);

	// Set a key's value.  If the key doesn't exist, it is created.
	void SetKey(const TCHAR *szGroup, const TCHAR *szKey, const TCHAR *szValue);

	// Retrieves a key's value and returns a pointer to it.  If the
	// key doesn't exist, NULL is returned.
	const TCHAR *GetKey(int nGroup, int nKey) const;
	const TCHAR *GetKey(const TCHAR *szGroup, const TCHAR *szKey) const;
};
