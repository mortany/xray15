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

/**********************************************************************
	DESCRIPTION:

		CATToken reads tokens from a file.  Give it a dictionary
		and it'll recognise and label your own custom tokens.

		CATMessages is a basic class that records errors and
		warnings and debug messages.  It has a method that brings
		up a window

		This also implements a rather useful class called Dictionary.
		It's a fast-lookup string tree.  Can't remember what the
		data structure is called but it's one of those ones that store
		a pointer for each character in your word so you can find it
		in N comparisons where N is the number of characters in your
		word.  I know there's a name for it and it's frustrating me
		that I can't remember what it is.
 **********************************************************************/

#include "cat.h"
#include "resource.h"
#include "CATToken.h"

 /*
  * Dictionary of internal identifiers used for tokenising.
  */
static DictionaryEntry dictBaseTokenEntries[] = {
	//  <NAME>					<IDENTIFIER ID>			<IDENTIFIER TYPE>		<FOLLOWING TYPE>
		_T("Unknown"),				idUnknown,				tokUnknown,				tokUnknown,
		NULL,					idNothing,				tokNothing,				tokNothing
};

static DictionaryEntry dictStandardTokenEntries[] = {
	//  <NAME>					<IDENTIFIER ID>			<IDENTIFIER TYPE>		<FOLLOWING TYPE>
		_T("nothing"),				idNothing,				tokNothing,				tokNothing,
		_T("true"),					idTrue,					tokBool,				tokNothing,
		_T("false"),				idFalse,				tokBool,				tokNothing,
		_T("point"),				idPoint,				tokPoint,				tokNothing,
		_T("quat"),					idQuat,					tokQuat,				tokNothing,
		_T("angaxis"),				idAngAxis,				tokAngAxis,				tokNothing,
		_T("matrix"),				idMatrix,				tokMatrix,				tokNothing,

		_T("LayerInfo"),			idLayerInfo,			tokLayerInfo,			tokNothing,
		_T("ClassID"),				idClassIDs,				tokClassIDs,			tokNothing,
		_T("Interval"),				idInterval,				tokInterval,			tokNothing,

		_T("KeyBezXYZ"),			idKeyBezXYZ,			tokKeyBezXYZ,			tokNothing,
		_T("KeyBezRot"),			idKeyBezRot,			tokKeyBezRot,			tokNothing,
		_T("KeyBezScale"),			idKeyBezScale,			tokKeyBezScale,			tokNothing,
		_T("KeyBezFloat"),			idKeyBezFloat,			tokKeyBezFloat,			tokNothing,

		_T("KeyTCBXYZ"),			idKeyTCBXYZ,			tokKeyTCBXYZ,			tokNothing,
		_T("KeyTCBRot"),			idKeyTCBRot,			tokKeyTCBRot,			tokNothing,
		_T("KeyTCBScale"),			idKeyTCBScale,			tokKeyTCBScale,			tokNothing,
		_T("KeyTCBFloat"),			idKeyTCBFloat,			tokKeyTCBFloat,			tokNothing,

		_T("KeyLinXYZ"),			idKeyLinXYZ,			tokKeyLinXYZ,			tokNothing,
		_T("KeyLinRot"),			idKeyLinRot,			tokKeyLinRot,			tokNothing,
		_T("KeyLinScale"),			idKeyLinScale,			tokKeyLinScale,			tokNothing,
		_T("KeyLinFloat"),			idKeyLinFloat,			tokKeyLinFloat,			tokNothing,

		_T("KeyFloat"),				idKeyFloat,				tokKeyFloat,			tokNothing,
		_T("KeyPoint"),				idKeyPoint,				tokKeyPoint,			tokNothing,
		_T("KeyMatrix"),			idKeyMatrix,			tokKeyMatrix,			tokNothing,

		_T("Face"),					idFace,					tokFace,				tokNothing,
		_T("INode"),				idINode,				tokINode,				tokNothing,

		NULL,					idNothing,				tokNothing,				tokNothing
};

/////////////////////////////////////////////////////////////////
// class Dictionary
//
// This class stores a dictionary lookup tree.  It is built from
// a table of DictionaryEntry values, where the last table entry
// has its name field set to NULL
//
Dictionary::Dictionary()
	: nodes(), entries()
{
	nodes.resize(numNodes = 1);
}

Dictionary::Dictionary(DictionaryEntry* table, BOOL bAddStandardTokens/*=TRUE*/)
	: nodes(), entries()
{
	nodes.resize(numNodes = 1);

	BuildDictionary(dictBaseTokenEntries);
	if (bAddStandardTokens) BuildDictionary(dictStandardTokenEntries);
	BuildDictionary(table);

	//	DebugPrint(_T("> Dictionary built using %d entries spanning %d nodes.\n"), entries.size(), numNodes);
}

// Adds an entry to the dictionary.
// Returns FALSE if entry id already exists.
BOOL Dictionary::AddEntry(DictionaryEntry* entry)
{
	TCHAR *c, lc;
	int nodeid = 0, nextid;

	// Rip through the node tree, adding nodes if necessary to reach the
	// end-point of the word.  We want to point that node at its counterpart
	// in the entries list, unless a previous node already exists, in which
	// case we do nothing and return FALSE.
	for (c = entry->name; *c; c++) {
		lc = (TCHAR)_totlower(*c);
		DbgAssert(lc >= _T('a') && lc <= _T('z'));
		nextid = nodes[nodeid].chars[lc - _T('a')];
		if (nextid == 0) {
			if (numNodes >= nodes.size())
				nodes.resize(nodes.size() + 16);
			nodes[nodeid].chars[lc - _T('a')] = numNodes;
			nodes[numNodes].parent = nodeid;
			nodeid = numNodes++;
		}
		else {
			nodeid = nextid;
		}
	}

	// Add the entry to our entries list and patch the end of our node
	// tree traversal to point to the correct item in the entries list.
	if (nodes[nodeid].id == 0) {
		nodes[nodeid].id = entry->id;
		if (entry->id >= entries.size())
			entries.resize(entries.size() + 16);
		entries[entry->id] = entry;

		// Overwrite the 'partial' fields back up the tree.  We don't include
		// the first node (node 0) in this, as it is silly to partially infer
		// anything from a string with no characters.
		for (nodeid = GetParentNode(nodeid); nodeid > 0; nodeid = GetParentNode(nodeid)) {
			if (nodes[nodeid].id == 0 && (nodes[nodeid].partial == 0 || entry->id < nodes[nodeid].partial))
				nodes[nodeid].partial = entry->id;
		}
		return TRUE;
	}

	return FALSE;
}

// Builds a dictionary from a list of entries.
// The last entry must have its name point to NULL.
void Dictionary::BuildDictionary(DictionaryEntry* table)
{
	DictionaryEntry* entry = table;
	while (entry->name) {
		BOOL succeed = AddEntry(entry);
		DbgAssert(succeed);
		UNREFERENCED_PARAMETER(succeed);
		entry++;
	}
}

// Return the node pointed to from the specified node by the
// given character.  Returns -1 if there is an error.
int Dictionary::GetNextNode(int nodeID, TCHAR c)
{
	if (nodeID == -1)
		return -1;
	c = (TCHAR)_totlower(c);
	if (c >= _T('a') && c <= _T('z'))
		return nodes[nodeID].chars[c - _T('a')];
	else
		return -1;
}

// Return the parent of the specified node.
int Dictionary::GetParentNode(int nodeID)
{
	if (nodeID == -1)
		return -1;
	return nodes[nodeID].parent;
}

// Return a pointer to the entry pointed to by the specified node.
DictionaryEntry* Dictionary::GetEntryAtNode(int nodeID)
{
	if (nodeID == -1) return NULL;
	else return entries[nodes[nodeID].id];
}

// Return a pointer to the specified entry.
DictionaryEntry* Dictionary::GetEntry(int entryID)
{
	if (entryID == -1) return NULL;
	else return entries[entryID];
}

// Lookup the string, and return its entry, or NULL if it is not
// in the dictionary.
DictionaryEntry* Dictionary::Lookup(const TCHAR* s)
{
	int node = 0;
	for (; *s; s++)
		if (-1 == (node = GetNextNode(node, *s)))
			return NULL;
	return GetEntryAtNode(node);
}

// Performs a partial lookup with fallback, returning the entry
// detected, the index of its first character within the search
// string and the length of the detected substring.  Fallback means
// that, failing to find a correct match, we search backwards along
// the tree path and taking the first node with a partial match.
DictionaryEntry* Dictionary::PartialLookup(const TCHAR* s, int &nFirst, int &nLength, BOOL bAllowPartialMatches/*=TRUE*/)
{
	DictionaryEntry *entry = NULL;
	int node, nextnode;
	int first, last;

	// We search the dictionary beginning at each position in
	// the search string, until we find a perfect match.
	for (first = 0; s[first]; first++) {
		node = 0;
		last = first;

		while (s[last]) {
			nextnode = GetNextNode(node, *s);
			if (nextnode == -1) {
				// We have reached the end of the search tree.
				// Look backwards and find the first perfect match,
				// or if there is none, partial match.
				int partialEntryID = nodes[node].partial;
				int partialLast = last;
				int partialNode = node;

				while (-1 != (node = GetParentNode(node))) {
					last--;
					if (nodes[node].id != 0) {
						entry = GetEntryAtNode(node);
						break;
					}
				}

				// If we have reached the beginning of the tree again,
				// there is no perfect match.  We can now use any partial
				// match information found along the way, if instructed
				// to do so.  Otherwise we skip out and allow the search
				// to continue from the next character in the string.
				if (node == -1 && bAllowPartialMatches) {
					// Initiate a search without partial matches by recursing
					// this function with bAllowPartialMatches set to FALSE.
					// The search string begins at the character following the
					// first character of the partial match.
					//
					// We then allow the detected partial match to pass in the
					// following two cases:
					//
					//  * There is no matching dictionary entry after the
					//    partial match.
					//
					//  * There is a matching dictionary entry whose first
					//    character is after the end of the partial match.
					//
					int nNextFirst, nNextLength;
					DictionaryEntry *pNextEntry = PartialLookup(&s[first + 1], nNextFirst, nNextLength, FALSE);
					nNextFirst += first + 1;

					if (!pNextEntry || nNextFirst > partialLast) {
						if (partialEntryID != 0) {
							entry = entries[partialEntryID];
							last = partialLast;
						}
					}
					else {
						// If there was a non-partial match beginning before the
						// end of this partial match, shorten the partial match
						// until it is the right length.
						while (partialLast >= nNextFirst) {
							partialLast--;
							partialNode = GetParentNode(partialNode);
						}

						if (partialNode != -1) {
							partialEntryID = nodes[partialNode].partial;
							if (partialEntryID != 0) {
								entry = entries[partialEntryID];
								last = partialLast;
							}
						}
					}
				}

				// Increment 'last' so it still points to the character AFTER
				// the last one to be used.  This is what would happen if the
				// loop terminated normally.
				last++;
				break;
			}
			node = nextnode;
			last++;
		}

		// If an entry was found, set the values of nFirst and nLength,
		// and break out of the search loop.
		if (entry) {
			nFirst = first;
			nLength = last - first;
			break;
		}
	}

	return entry;
}

//
// This is used for error message generation.
//
const TCHAR * TokenName(int tok)
{
	switch (tok) {
	case tokUnknown:		return _T("unknown");
	case tokNothing:		return _T("nothing");
	case tokComment:		return _T("comment");
	case tokIdentifier:		return _T("identifier");
	case tokGroup:			return _T("group");
	case tokBool:			return _T("boolean");
	case tokInt:			return _T("integer");
	case tokULONG:			return _T("unsignedlong");
	case tokFloat:			return _T("float");
	case tokString:			return _T("string");
	case tokQuat:			return _T("'quat'");
	case tokAngAxis:		return _T("'angaxis'");
	case tokPoint:			return _T("'point'");
	case tokMatrix:			return _T("'matrix'");

	case tokOpenCurly:		return _T("'{'");
	case tokCloseCurly:		return _T("'}'");
	case tokEquals:			return _T("'='");
	case tokEOL:			return _T("EOL");
	case tokEOF:			return _T("EOF");

	case tokKeyBezXYZ:		return _T("'KeyBezXYZ'");
	case tokKeyBezRot:		return _T("'KeyBezRot'");
	case tokKeyBezScale:	return _T("'KeyBezScale'");
	case tokKeyBezFloat:	return _T("'KeyBezFloat'");

	case tokKeyTCBXYZ:		return _T("'KeyTCBXYZ'");
	case tokKeyTCBRot:		return _T("'KeyTCBRot'");
	case tokKeyTCBScale:	return _T("'KeyTCBScale'");
	case tokKeyTCBFloat:	return _T("'KeyTCBFloat'");

	case tokKeyLinXYZ:		return _T("'KeyLinXYZ'");
	case tokKeyLinRot:		return _T("'KeyLinRot'");
	case tokKeyLinScale:	return _T("'KeyLinScale'");
	case tokKeyLinFloat:	return _T("'KeyLinFloat'");

	case tokClassIDs:		return _T("'ClassIDs'");
	case tokInterval:		return _T("'Interval'");

	case tokFace:			return _T("'Face'");

	default:				return _T("unrecognised");
	}
}

/////////////////////////////////////////////////////////////////
// CATToken functions
/////////////////////////////////////////////////////////////////

// This is the core of CATToken.  It takes an input stream and
// extracts the next token from it.  Tokens are defined in the
// header for this file.
//
bool CATToken::ReadToken(tstringstream& s)
{
	DbgAssert(dictionary);

	TCHAR c;
	bool error = false, done = false;
	bool autoSeps = true;

	enum { noExponent, signOrNum, numOnly } exponent = noExponent;

	// used for searching the dictionary
	int dictNode = 0;
	dictEntry = NULL;

	nCharsRead = 0;

	// initialise string storage
	nPos = 0;
	strValue.resize(0);
	strValue.reserve(16);
	nCapacity = (LONG)strValue.capacity();

	type = tokNothing;
	expect = tokNothing;
	id = idNothing;

	while (!done && !error) {
		s.get(c);

		if (s.eof() && s.fail()) {
			if (type == tokNothing) {
				type = tokEOF;
				done = true;
			}
			else if (type == tokString) {
				error = true;
			}
			else {
				done = true;
			}
			continue;
		}

		++nCharsRead;

		// if our string is at its full capacity, reserve more characters
		if (strValue.size() >= nCapacity) {
			strValue.reserve(nCapacity + 16);
			nCapacity = (ULONG)strValue.capacity();
		}

		// If this is a separator, deal with it specially
		if (autoSeps) {
			if (c == _T('\n')) {
				if (type == tokNothing) type = tokEOL;
				else {
					s.putback(c);
					--nCharsRead;
				}
				done = true;
			}
			else if (c == _T('\r')) {
				if (type == tokNothing) continue;
				done = true;
			}
			else if (_istspace(c)) {
				if (type == tokNothing) continue;
				done = true;
			}

			// End of token.  If this is an unknown token, look
			// it up in the dictionary and find its real type.
			if (done && type == tokUnknown) {
				dictEntry = dictionary->GetEntryAtNode(dictNode);
				if (dictEntry) {
					id = dictEntry->id;
					type = dictEntry->type;
					expect = dictEntry->expect;
				}
				continue;
			}
		}

		// If no token has been recognised yet, see what comes up.
		if (type == tokNothing) {
			if (c == _T('"')) {
				type = tokString;
				autoSeps = false;
				s.get(c);
				++nCharsRead;
			}
			else if (c == _T('-')) {
				type = tokInt;
				strValue += c;
				continue;
			}
			else if (c == _T('#')) {
				type = tokComment;
				autoSeps = false;
				s.get(c);
				++nCharsRead;
			}
			else if (c == _T('=')) {
				type = tokEquals;
				done = true;
				continue;
			}
			else if (c == _T('{')) {
				type = tokOpenCurly;
				done = true;
				continue;
			}
			else if (c == _T('}')) {
				type = tokCloseCurly;
				done = true;
				continue;
			}
			else if (_istdigit(c)) {
				type = tokInt;
			}
			else if (_istalpha(c)) {
				// We will start searching the dictionary for an identifier
				// of some sort.  If nothing is found, this type will remain
				// as unknown and we'll return an error.
				dictNode = 0;
				type = tokUnknown;
			}
		}

		// We now have a token.
		switch (type) {
		case tokUnknown:
			// Use GetNextNode() to lookup this character from our
			// current node in the dictionary.  The function handles
			// when dictNode is -1, or when the character is not
			// an alpha, and in either case returns -1.  When we
			// find the end of our token, if dictNode is -1 we
			// return an error, otherwise we look up the dictionary
			// entry to find out what this token is.
			strValue += c;
			dictNode = dictionary->GetNextNode(dictNode, c);
			break;

		case tokComment:
			if (c == _T('\n') || c == _T('\r')) done = true;
			else if (_istspace(c) && strValue.size() == 0) continue;
			else strValue += c;
			break;

		case tokString:
			if (c == _T('"')) done = true;
			else if (c == _T('\n') || c == _T('\r')) error = true;
			else strValue += c;
			break;

		case tokInt:
			if (_istdigit(c)) {
				strValue += c;
			}
			else if (c == _T('.')) {
				strValue += c;
				type = tokFloat;
			}
			break;
			//		case tokULONG:
			//			if (isdigit(c)) {
			//				strValue += c;
			//			} else if (c=='.') {
			//				strValue += c;
			//				type = tokFloat;
			//			}
			//			break;

		case tokFloat:
			if (_istdigit(c)) {
				if (exponent == signOrNum) exponent = numOnly;
				strValue += c;
			}
			else if (c == _T('e') || c == _T('E')) {
				if (exponent != noExponent) error = true;
				else {
					strValue += c;
					exponent = signOrNum;
				}
			}
			else if (c == _T('-') || c == _T('+')) {
				if (exponent == signOrNum) {
					exponent = numOnly;
					strValue += c;
				}
				else {
					error = true;
				}
			}
			else if (c == _T('.')) {
				error = true;
			}
			break;

		}
	}

	return !error;
}

/////////////////////////////////////////////////////////////////
// Through wonders of modern science we can turn stuff into other
// stuff.  
//
// This function just tells us if it can take something from you and
// give you something back.
//
BOOL CATToken::CanTransmuteTo(CatTokenType to)
{
	switch (type) {
	case tokInt:
		if (to == tokFloat || to == tokBool || to == tokULONG) return TRUE;
		break;
	case tokULONG:
		if (to == tokFloat || to == tokBool || to == tokInt) return TRUE;
		break;
	case tokFloat:
		if (to == tokBool) return TRUE;
		break;
	case tokBool:
		if (to == tokInt || to == tokFloat || to == tokString) return TRUE;
		break;
	case tokQuat:
		if (to == tokAngAxis) return TRUE;
		break;
	case tokAngAxis:
		if (to == tokQuat) return TRUE;
		break;
	}
	return FALSE;
}

BOOL CATToken::TransmuteTo(CatTokenType to)
{
	transmuted = false;
	transmutedFrom = type;

	switch (type) {
	case tokInt:
		if (to == tokFloat || to == tokBool || to == tokULONG) {
			// transmution handled by asInt() or asBool()
			transmuted = true;
			type = to;
		}
		break;
	case tokULONG:
		if (to == tokFloat || to == tokBool || to == tokInt) {
			// transmution handled by asULONG()
			transmuted = true;
			type = to;
		}
		break;
	case tokFloat:
		if (to == tokBool) {
			// transmution handled by asBool()
			transmuted = true;
			type = to;
		}
		break;
	case tokBool:
		if (to == tokInt) {
			transmuted = true;
			strValue = asBool() ? _T("1") : _T("0");
			type = to;
		}
		else if (to == tokFloat) {
			transmuted = true;
			strValue = asBool() ? _T("1.0") : _T("0.0");
			type = to;
		}
		else if (to == tokString) {
			transmuted = true;
			strValue = asBool() ? _T("true") : _T("false");
			type = to;
		}
		break;
	case tokQuat:
		if (to == tokAngAxis) {
			// transmution happens after the quat value is read
			transmuted = true;
			type = to;
		}
		break;
	case tokAngAxis:
		if (to == tokQuat) {
			// transmution happens after the angaxis value is read
			transmuted = true;
			type = to;
		}
		break;
	}

	if (transmuted) return TRUE;
	transmutedFrom = tokNothing;
	return FALSE;
}
