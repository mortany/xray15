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

#include "cat.h"
#include "Locale.h"

TCHAR *ReplaceLocale(int category, const TCHAR *locale)
{
	TCHAR *oldLocale = _tsetlocale(category, NULL);
	TCHAR *newOldLocale = new TCHAR[_tcslen(oldLocale) + 4];
	DbgAssert(newOldLocale);
	_tcscpy(newOldLocale, oldLocale);
	_tsetlocale(category, locale);
	return newOldLocale;
}

void RestoreLocale(int category, TCHAR *locale)
{
	_tsetlocale(category, locale);
	delete[] locale;
}
