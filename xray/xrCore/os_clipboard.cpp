////////////////////////////////////////////////////////////////////////////
//	Module 		: os_clipboard.cpp
//	Created 	: 21.02.2008
//	Author		: Evgeniy Sokolov
//	Description : os clipboard class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "os_clipboard.h"

void os_clipboard::copy_to_clipboard	( LPCTSTR buf )
{
	if ( !OpenClipboard(0) )
		return;
	u32 handle_size = ( xr_strlen(buf) + 1 ) * sizeof(TCHAR);
	HGLOBAL handle = GlobalAlloc( GHND, handle_size );
	if ( !handle )
	{
		CloseClipboard		();
		return;
	}

	TCHAR* memory			= (TCHAR*)GlobalLock( handle );
	wcscpy_s				( memory, handle_size, buf );
	GlobalUnlock			( handle );
	EmptyClipboard			();
	SetClipboardData		( CF_TEXT, handle );
	CloseClipboard			();
}

void os_clipboard::paste_from_clipboard	( LPTSTR buffer, u32 const& buffer_size )
{
	VERIFY					(buffer);
	VERIFY					(buffer_size > 0);

	if (!OpenClipboard(0))
		return;

	HGLOBAL	hmem			= GetClipboardData( CF_TEXT );
	if ( !hmem )
		return;

	LPCTSTR clipdata			= (LPCTSTR)GlobalLock( hmem );
	wcsncpy					( buffer, clipdata, buffer_size );
	buffer[buffer_size]		= 0;
	for ( u32 i = 0; i < wcslen( buffer ); ++i )
	{
		TCHAR c = buffer[i];
		if ( ( (isprint(c) == 0) && (c != TCHAR(-1)) ) || c == '\t' || c == '\n' )// "я" = -1
		{
			buffer[i]		= ' ';
		}
	}

	GlobalUnlock			( hmem );
	CloseClipboard			();
}

void os_clipboard::update_clipboard		( LPCTSTR string )
{
	if ( !OpenClipboard(0) )
		return;

	HGLOBAL	handle			= GetClipboardData(CF_TEXT);
	if (!handle) {
		CloseClipboard		();
		copy_to_clipboard	(string);
		return;
	}

	LPTSTR	memory			= (LPTSTR)GlobalLock(handle);
	int		memory_length	= (int)wcslen(memory);
	int		string_length	= (int)wcslen(string);
	int		buffer_size		= (memory_length + string_length + 1) * sizeof(TCHAR);
#ifndef _EDITOR
	LPTSTR	buffer			= (LPTSTR)xr_alloc<int>( buffer_size );
#else // #ifndef _EDITOR
	LPTSTR	buffer			= (LPTSTR)xr_alloc<char>( buffer_size );
#endif // #ifndef _EDITOR
	wcscpy_s				(buffer, buffer_size, memory);
	GlobalUnlock			(handle);

	wcscat					(buffer, string);
	CloseClipboard			();
	copy_to_clipboard		(buffer);
#ifdef _EDITOR
	xr_free					(buffer);
#endif // #ifdef _EDITOR
}