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

// Tokeniser and dictionary stuff used by rig and motion presets.

#pragma once

#include <vector>
#include <fstream>
 //#include <fstream>
#include <sstream>
#include <string>

#include "CATMessages.h"

//
// Token types.
//
enum {
	idUnknown,
	idNothing,
	idTrue,
	idFalse,
	idPoint,
	idQuat,
	idAngAxis,
	idMatrix,

	// clip saver tokens
	idLayerInfo,
	idClassIDs,
	idInterval,

	idKeyBezXYZ,
	idKeyBezRot,
	idKeyBezScale,
	idKeyBezFloat,

	idKeyTCBXYZ,
	idKeyTCBRot,
	idKeyTCBScale,
	idKeyTCBFloat,

	idKeyLinXYZ,
	idKeyLinRot,
	idKeyLinScale,
	idKeyLinFloat,

	idKeyFloat,
	idKeyPoint,
	idKeyMatrix,

	idFace,
	idINode,

	idStartYourIdentifiersHere
};

/////////////////////////////////////////////////////
// Is used for Saving out all the class IDs in one go
class TokClassIDs : public MaxHeapOperators
{
public:
	//Parameters for the bezier interpolation
	ULONG usSuperClassID, usClassIDA, usClassIDB;
	int subindex;
	TSTR classname;

	// we could save things like inheritance flags
	int flags;

	inline TokClassIDs() : subindex(0), usSuperClassID(0L), flags(0), usClassIDA(0L), usClassIDB(0L) {}
	inline TokClassIDs(ULONG newSuperClassID, ULONG newClassIDA, ULONG newClassIDB,/* int sub,*/ TSTR clsname) :
		subindex(0), flags(0)
	{
		usSuperClassID = newSuperClassID;
		usClassIDA = newClassIDA;
		usClassIDB = newClassIDB;
		//	subindex = sub;
		classname = clsname;
	}

};

//
// These are the types of token that CATToken can handle
//
typedef enum tagCatTokenType CatTokenType;
enum tagCatTokenType {
	tokUnknown,			// unknown / illegal token
	tokNothing,			// default token / no token found
	tokComment,			// comment (begins with '#')
	tokIdentifier,		// identifier (found on left side of '=')
	tokGroup,			// group (found on left side of '{')
	tokBool,			// boolean value ('true' or 'false')
	tokInt,				// integer value
	tokULONG,			// ULONG value
	tokFloat,			// floating point value
	tokString,			// string value
	tokPoint,			// Point3 value
	tokQuat,			// Quat value
	tokAngAxis,			// AngAxis value
	tokMatrix,			// Matrix3 value

	tokEquals,			// assignment operator '='
	tokOpenCurly,		// left curly brace '{'
	tokCloseCurly,		// right curly brace '}'
	tokEOL,				// end of line
	tokEOF,				// end of file

// clipsaver data types
tokLayerInfo,		// enough info so we could rebuild this layer
tokClassIDs,		// enough info so we could rebuild this controller
tokInterval,

tokKeyBezXYZ,
tokKeyBezRot,
tokKeyBezScale,
tokKeyBezFloat,

tokKeyTCBXYZ,
tokKeyTCBRot,
tokKeyTCBScale,
tokKeyTCBFloat,

tokKeyLinXYZ,
tokKeyLinRot,
tokKeyLinScale,
tokKeyLinFloat,

tokKeyFloat,		// basic floating point keyframe
tokKeyPoint,		// basic Point3
tokKeyMatrix,		// basic Matrix3 keyframe

tokFace,			// Polygon surface face
tokINode,
};

extern const TCHAR *TokenName(int tok);

//
// This is a dictionary entry.  It's used internally, but needs to
// be here because various other internal classes that use it (such
// as CATToken) have to be defined here.
//
typedef struct tagDictionaryEntry DictionaryEntry;
struct tagDictionaryEntry {
	LPTSTR name;				// the identifier's name
	USHORT id;				// the identifier's id
	CatTokenType type;		// the identifier's type
	CatTokenType expect;	// expected type to follow this identifier
};

//
// This is a dictionary node.  Nodes are created by the Dictionary
// class.  The id specifies which DictionaryEntry this node
// corresponds to.
//
class DictionaryNode : public MaxHeapOperators
{
public:
	DWORD chars[26];		// we allow storage of lower-case letters only
	long parent;			// points back to the parent node (used for lookup fallback) // SA changed DWORD -> long because parent is signed below (-1)
	USHORT partial;			// the id of the word (used for partial lookup)
	USHORT id;				// this is the id of the word stored.
	DictionaryNode() {
		memset(this, 0, sizeof DictionaryNode);
		parent = -1;
	}
};

/////////////////////////////////////////////////////////////////
// class Dictionary
//
// This class stores a dictionary lookup tree.  It is built from
// a table of DictionaryEntry values, where the last table entry
// has its name field set to NULL
class Dictionary : public MaxHeapOperators
{
private:
	ULONG numNodes;
	std::vector<DictionaryNode> nodes;
	std::vector<DictionaryEntry*> entries;	// indexed by the id type

	// Adds an entry to the dictionary.
	// Returns FALSE if entry id already exists.
	BOOL AddEntry(DictionaryEntry* entry);

	// Builds a dictionary from a list of entries.
	// The last entry must have its name point to NULL.
	void BuildDictionary(DictionaryEntry* table);

public:
	Dictionary();
	Dictionary(DictionaryEntry* table, BOOL bAddStandardTokens = TRUE);

	// Return the node pointed to from the specified node by the
	// given character.  Returns -1 if there is an error.
	int GetNextNode(int nodeID, TCHAR c);

	// Returns the parent of the specified node.
	int GetParentNode(int nodeID);

	// Lookup the string, and return its entry, or NULL if it is not
	// in the dictionary.
	DictionaryEntry* Lookup(const TCHAR* s);

	// Performs a partial lookup with fallback, returning the entry
	// detected, the index of its first character within the search
	// string and the length of the detected substring.  Fallback means
	// that, failing to find a correct match, we search backwards along
	// the tree path and taking the first node with a partial match.
	DictionaryEntry* PartialLookup(const TCHAR *s, int &nFirst, int &nLength, BOOL bAllowPartialMatches = TRUE);

	// Return a pointer to the entry pointed to by the specified node.
	DictionaryEntry* GetEntryAtNode(int nodeID);

	// Return a pointer to the specified entry.
	DictionaryEntry* GetEntry(int entryID);
};

/////////////////////////////////////////////////////////////////
// class CATToken
//
// Tokens are extracted using ReadToken().  Once extracted, we
// can query token id.  If it's an identifier token we can query
// its identifier number, the expected next token type, and grab
// its dictionary entry.  For any token we can get its integer,
// boolean or floating point value...  But it would only make
// sense to do so if it actually IS a token of one of those
// types, you knob.
//
// This is an internal class, but must be defined here because
// it's used explicitly in CATRigReader (in private members).
// You need not use this class...  In fact, don't.
//

#ifdef UNICODE
#define tstring std::wstring
#define tstringstream std::wstringstream
#else
#define tstring std::string
#define tstringstream std::stringstream
#endif

class CATToken : public MaxHeapOperators
{
public:
	CatTokenType		type;
	USHORT	id;
	CatTokenType		expect;

	bool				transmuted;
	CatTokenType		transmutedFrom;

protected:
	// The total number of characters read from the stream
	ULONG				nCharsRead;

	// Parameters for maintaining the token string
	ULONG				nPos;
	ULONG				nCapacity;
	tstring				strValue;

	// Dictionary from which tokens are drawn
	Dictionary*			dictionary;
	DictionaryEntry*	dictEntry;

public:

	CATToken() : strValue(_T(""))
		, transmuted(false)
		, expect(tokNothing)
		, nPos(0L)
		, type(tokNothing)
		, nCapacity(0L)
		, dictEntry(NULL)
		, transmutedFrom(tokNothing)
		, id(0)
	{
		dictionary = NULL; nCharsRead = 0;
	}

	void SetDictionary(Dictionary* dict) { dictionary = dict; }

	bool ReadToken(tstringstream& s);
	ULONG NumCharsRead() const { return nCharsRead; }

	bool isUnknown() const { return type == tokUnknown; }
	bool isIdentifier() const { return type == tokIdentifier; }
	bool isComment() const { return type == tokComment; }
	bool isString() const { return type == tokString; }
	bool isNumber() const { return (type == tokInt || type == tokFloat || type == tokULONG); }
	bool isEquals() const { return type == tokEquals; }
	bool isOpenCurly() const { return type == tokOpenCurly; }
	bool isCloseCurly() const { return type == tokCloseCurly; }
	bool isEOL() const { return type == tokEOL; }
	bool isEOF() const { return type == tokEOF; }
	bool isEOLorEOF() const { return (type == tokEOL || type == tokEOF); }
	bool isValue() const {
		switch (type) {
		case tokBool:
		case tokInt:
		case tokFloat:
		case tokString:
		case tokPoint:
		case tokQuat:
		case tokAngAxis:
		case tokMatrix:

		case tokLayerInfo:		// enough info so we could rebuild this layer
		case tokClassIDs:		// enough info so we could rebuild this controller
		case tokInterval:

		case tokKeyBezXYZ:
		case tokKeyBezRot:
		case tokKeyBezScale:
		case tokKeyBezFloat:

		case tokKeyTCBXYZ:
		case tokKeyTCBRot:
		case tokKeyTCBScale:
		case tokKeyTCBFloat:

		case tokKeyLinXYZ:
		case tokKeyLinRot:
		case tokKeyLinScale:
		case tokKeyLinFloat:

		case tokKeyFloat:		// basic floating point keyframe
		case tokKeyPoint:		// basic Point3
		case tokKeyMatrix:

		case tokFace:
		case tokINode:

			return true;
		default:
			return false;
		}
	}

	BOOL CanTransmuteTo(CatTokenType to);
	BOOL TransmuteTo(CatTokenType to);

	//	int asInt() { return atoi(strValue.c_str()); }
	int asInt() { return _tstol(strValue.c_str()); }
	ULONG asULONG() {
		return (ULONG)_tstof(&strValue[0]);
	}

	float asFloat() { return (float)_tstof(strValue.c_str()); }
	const tstring& asString() { return strValue; }
	bool asBool() {
		if (type == tokBool) return (id == idTrue);
		else if (type == tokInt) return asInt() != 0;
		else if (type == tokULONG) return asULONG() != 0;
		else if (type == tokFloat) return asFloat() != 0.0f;
		else if (type == tokString) return strValue != _T("");
		return false;
	}

	DictionaryEntry* GetEntry() const { return dictEntry; }
};
