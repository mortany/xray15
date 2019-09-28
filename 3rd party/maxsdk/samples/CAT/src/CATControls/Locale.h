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

// ReplaceLocale() saves the current locale in a newly allocated string,
//                 replaces the locale with that specified, and returns
//                 the string containing the old locale.
//
// RestoreLocale() sets the locale to the given string and deallocates it.
//                 you should only pass in a string that was returned from
//                 ReplaceLocale().
//
#pragma once

#include <locale.h>
TCHAR *ReplaceLocale(int category, const TCHAR *locale);
void RestoreLocale(int category, TCHAR *locale);
