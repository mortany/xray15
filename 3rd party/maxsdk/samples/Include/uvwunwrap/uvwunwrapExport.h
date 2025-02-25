//**************************************************************************/
// Copyright (c) 2015 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
#pragma once


#ifdef UNWRAP_DATA_EXPORT
#define UNWRAP_EXPORT __declspec(dllexport)
#define UNWRAP_TEMPLATE
#else
#define UNWRAP_EXPORT __declspec(dllimport)
#define UNWRAP_TEMPLATE extern
#endif
