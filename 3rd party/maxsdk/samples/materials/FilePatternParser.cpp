//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include "FilePatternParser.h"
#include <Strsafe.h>	// for StringCch* function
#include <tuple>

// Zbrush and Mudbox have following multi tile pattern:
// xxxxu0_v0.jpg
// the difference is ZBrush is 0-based, while Mudbox is 1-based.
// The regex consists of 2 parts, file name prefix and uv pattern.
// File name prefix contains all valid characters.
// UV pattern contains u|U, v|V prefix with digits.
const FilePatternParser::_tregex FilePatternParser::zbrush_mudbox_rgx( _T("(.+)[uU](-?[[:digit:]]+)_[vV](-?[[:digit:]]+)") );

const _tstring FilePatternParser::zbrush_mudbox_rgx_suffix( _T("[uU](-?[[:digit:]]+)_[vV](-?[[:digit:]]+)") );

// UDIM has following multi tile pattern:
// xxxx1023.jpg
// It starts from xxxx_-1001.jpg
// The formula to calculate UV is: UDIM<U, V> = 1000 + (V * 10) + (U + 1)
const FilePatternParser::_tregex FilePatternParser::udim_rgx( _T("(.+)-1([[:digit:]]{3})") );

const _tstring FilePatternParser::udim_rgx_suffix( _T("-1([[:digit:]]{3})") );


FilePatternParser::FilePatternParser( const TilePatternFormat format, const MCHAR *inputFilePath )
	: m_curFormat( format )
	, m_InputFileName( inputFilePath )
{

}

FilePatternParser::FileList FilePatternParser::FindMatchedFiles()
{
	// Check that the input path plus 2 is not longer than _MAX_PATH.
	// Two characters are for the "*" plus NULL appended below.
	MCHAR searchStr[_MAX_PATH];
	errno_t ret = _tmakepath_s( searchStr, m_Drive, m_Dir, m_FilePathPrefix.c_str(), nullptr );
	if ( 0 != ret ) { return m_FileList; }

	size_t strLength;
	StringCchLength(searchStr, _MAX_PATH, &strLength);
	if ( strLength > (_MAX_PATH - 2) ) {
		return m_FileList;
	}
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '*' after file prefix.
	StringCchCat( searchStr, _MAX_PATH, _T("*") );

	// Find the first file in the directory.
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(searchStr, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
		return m_FileList;
	} 

	// List all normal files in the directory
	const int excludeFlags = (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
	do {
		if ( !(ffd.dwFileAttributes & excludeFlags ) ) {
			MCHAR drive[_MAX_DRIVE];
			MCHAR dir[_MAX_DIR];
			MCHAR fileName[_MAX_FNAME];
			MCHAR fileExt[_MAX_EXT];
			errno_t ret = _tsplitpath_s( ffd.cFileName, drive, dir, fileName, fileExt );
			if ( 0 != ret ) { continue; }
			// skip different file type
			if ( _tcsnicmp(m_FileExt, fileExt, _MAX_EXT) ) { continue; }
			
			_tmatch match;
			if ( IsFileMatch( fileName, &match ) ) {
				UVMapFileEntry entry;
				if ( CreateEntryFromMatchResult( match, &entry ) ) {
					m_FileList.insert( entry );
				}
			}
		}
	} while( FindNextFile(hFind, &ffd) );

	DbgAssert(GetLastError() == ERROR_NO_MORE_FILES);
	FindClose(hFind);

	return m_FileList;
}

bool FilePatternParser::CheckFilePattern()
{
	errno_t ret = _tsplitpath_s( m_InputFileName, m_Drive, m_Dir, m_FileName, m_FileExt );
	if ( 0 != ret ) { return false; }

	_tmatch match;
	if ( IsFileMatch(m_FileName, &match) ) {
		m_FilePathPrefix = match.str(1);

		UVMapFileEntry entry;
		if ( CreateEntryFromMatchResult( match, &entry ) ) {
			m_FileList.insert( entry );
			return true;
		}
	}
	return false;
}

_tstring FilePatternParser::GetPattenedFilePrefix() const
{
	return m_FilePathPrefix;
}

TilePatternFormat FilePatternParser::GetFilePatternFormat( const MCHAR * filePath )
{
	if ( nullptr == filePath ) {
		return TilePatternFormat::Invalid;
	}

	MCHAR drive[_MAX_DRIVE];
	MCHAR dir[_MAX_DIR];
	MCHAR fileName[_MAX_FNAME];
	MCHAR fileExt[_MAX_EXT];
	errno_t ret = _tsplitpath_s( filePath, drive, dir, fileName, fileExt );
	if ( 0 != ret ) {
		return TilePatternFormat::Invalid;
	}

	_tmatch match;
	UVIndex uIndex;
	UVIndex vIndex;
	if ( std::regex_match(fileName, match, zbrush_mudbox_rgx) ) {
		// Find all matched files to discern ZBrush or Mudbox format
		// Force ZBrush format to maximize matched files
		FilePatternParser parser( TilePatternFormat::ZBrush, filePath );
		parser.CheckFilePattern();
		FileList fileList = parser.FindMatchedFiles();
		auto it = std::find_if( fileList.cbegin(), fileList.cend(), 
			[]( const UVMapFileEntry &item ) -> bool {
				return ( (0 == std::get<0>(item)) || (0 == std::get<1>(item)) );
			} );

		if ( it == fileList.end() ) {
			// Not find UV contains 0 does not mean it must be Mudbox format,
			// but that's all we could do.
			// If so, user must change tile pattern format and reload images again
			// after MultiTile texture is created.
			return TilePatternFormat::Mudbox;
		}
		return TilePatternFormat::ZBrush;
	}
	else if ( std::regex_match(fileName, match, udim_rgx) ) {
		if ( !ExtractUV(match, TilePatternFormat::UDIM, &uIndex, &vIndex) ) {
			return TilePatternFormat::Custom;
		}
		return TilePatternFormat::UDIM;
	}
	else {
		return TilePatternFormat::Custom;
	}
}

_tstring FilePatternParser::ExtractFileName( const MCHAR *fullPath )
{
	_tstring retString;
	if ( !fullPath ) {
		return retString;
	}

	MCHAR drive[_MAX_DRIVE];
	MCHAR dir[_MAX_DIR];
	MCHAR fileName[_MAX_FNAME];
	MCHAR fileExt[_MAX_EXT];
	errno_t ret = _tsplitpath_s( fullPath, drive, dir, fileName, fileExt );
	if ( 0 == ret ) {
		retString = fileName;
		retString += fileExt;
	}
	return retString;
}

bool FilePatternParser::ExtractUV( const _tmatch &match, TilePatternFormat format, UVIndex *uIndex, UVIndex *vIndex )
{
	UVIndex uValue;
	UVIndex vValue;
	if ( TilePatternFormat::UDIM == format ) {
		auto sumStr = match.str(2);
		auto uvSum = _tstoi(sumStr.c_str());
		// UDIM<u, v> = ( v * 10 ) + ( u + 1 )
		vValue = uvSum / 10;
		uValue = uvSum - 10 * vValue - 1;
		if ( (uValue < 0) || (vValue < 0) ) {
			return false;
		}
	}
	else {
		auto uIdxStr = match.str(2);
		DbgAssert( !uIdxStr.empty() );
		if ( uIdxStr.empty() ) {
			return false;
		}
		uValue = _tstoi(uIdxStr.c_str());

		auto vIdxStr = match.str(3);
		DbgAssert( !vIdxStr.empty() );
		if ( vIdxStr.empty() ) {
			return false;
		}
		vValue = _tstoi(vIdxStr.c_str());

		if ( TilePatternFormat::Mudbox == format ) {
			if ( (0 == uValue) || (0 == vValue) ) {
				// Mudbox UV index can't be 0
				return false;
			}
			// Mudbox positive UV is 1 offset, but negative UV should keep as-is
			if ( uValue > 0 ) {
				--uValue;
			}
			if ( vValue > 0 ) {
				--vValue;
			}
		}
	}

	if ( uIndex ) {
		*uIndex = uValue;
	}
	if ( vIndex ) {
		*vIndex = vValue;
	}
	return true;
}

bool FilePatternParser::IsFileMatch( const MCHAR *fileName, _tmatch *match ) const
{
	switch ( m_curFormat ) {
	case TilePatternFormat::ZBrush:
	case TilePatternFormat::Mudbox:
		{
			_tregex curRgx = zbrush_mudbox_rgx;
			if ( !m_FilePathPrefix.empty() ) {
				// When file path prefix has filled in, construct regex with it instead of wildcard.
				// Otherwise it may include unexpected items.
				curRgx = _tregex( _T("(") + m_FilePathPrefix + _T(")") + zbrush_mudbox_rgx_suffix );
			}
			return std::regex_match( fileName, *match, curRgx );
		}
	case TilePatternFormat::UDIM:
		{
			_tregex curRgx = udim_rgx;
			if ( !m_FilePathPrefix.empty() ) {
				// When file path prefix has filled in, construct regex with it instead of wildcard.
				// Otherwise it may include unexpected items.
				curRgx = _tregex( _T("(") + m_FilePathPrefix + _T(")") + udim_rgx_suffix );
			}
			return std::regex_match( fileName, *match, curRgx );
		}
	default:
		return false;
	}
}

bool FilePatternParser::CreateEntryFromMatchResult( const _tmatch &match, UVMapFileEntry *entry )
{
	auto prefix = match.str(1);
	DbgAssert( !prefix.empty() );
	if ( prefix.empty() ) {
		return false;
	}

	UVIndex uIndex;
	UVIndex vIndex;
	if ( !ExtractUV(match, m_curFormat, &uIndex, &vIndex) ) {
		return false;
	}

	if ( entry ) {
		MCHAR fullPath[_MAX_PATH];
		errno_t ret = _tmakepath_s( fullPath, m_Drive, m_Dir, match.str(0).c_str(), m_FileExt );
		if ( 0 != ret ) {
			return false;
		}
		*entry = std::make_tuple( uIndex, vIndex, match.str(0) + m_FileExt, fullPath );
		return true;
	}
	return false;
}
