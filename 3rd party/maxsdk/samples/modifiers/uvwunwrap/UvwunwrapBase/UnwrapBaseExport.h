/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/
#pragma once


#ifdef _UNWRAP_BASE_EXPORT_
#define UNWRAP_BASE_EXPORT __declspec(dllexport)
#define UNWRAP_BASE_TEMPLATE
#else
#define UNWRAP_BASE_EXPORT __declspec(dllimport)
#define UNWRAP_BASE_TEMPLATE extern
#endif
