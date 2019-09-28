//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include <regex>
#include <set>

#include "util.h"
#include "IMultiTile.h"

// Utility to parse the multi tile image files in a folder
// Construct an image set sorted by UV index
class FilePatternParser
{
public:
	#ifdef UNICODE
		typedef std::wcmatch _tmatch;
		typedef std::wregex _tregex;
	#else
		typedef std::cmatch _tmatch;
		typedef std::regex _tregex;
	#endif

	typedef int UVIndex;
	// data structure hold u index, v index, file name and full file path
	typedef std::tuple<UVIndex, UVIndex, _tstring, _tstring> UVMapFileEntry;
	typedef std::set<UVMapFileEntry> FileList;

private:
	const TilePatternFormat m_curFormat;
	const MCHAR *m_InputFileName;

	// members that hold splitted input file name
	MCHAR m_Drive[_MAX_DRIVE];
	MCHAR m_Dir[_MAX_DIR];
	MCHAR m_FileName[_MAX_FNAME];
	MCHAR m_FileExt[_MAX_EXT];

	// string that contains file name before UV index string,
	// e.g. "abc_" as for "abc_U0V0"
	_tstring m_FilePathPrefix;

	// match result
	FileList m_FileList;

	static const _tregex zbrush_mudbox_rgx;
	static const _tstring zbrush_mudbox_rgx_suffix;
	static const _tregex udim_rgx;
	static const _tstring udim_rgx_suffix;

public:
	FilePatternParser( const TilePatternFormat format, const MCHAR *inputFilePath );

	FileList FindMatchedFiles();
	
	bool CheckFilePattern();
	
	_tstring GetPattenedFilePrefix() const;

	static TilePatternFormat GetFilePatternFormat( const MCHAR *filePath );

	// extract file name + file extension from full file path
	static _tstring ExtractFileName( const MCHAR *fullPath );

private:
	static bool ExtractUV( const _tmatch &match, TilePatternFormat format, UVIndex *uIndex, UVIndex *vIndex );

	bool IsFileMatch( const MCHAR *fileName, _tmatch *match ) const;

	bool CreateEntryFromMatchResult( const _tmatch &match, UVMapFileEntry *entry );
};

