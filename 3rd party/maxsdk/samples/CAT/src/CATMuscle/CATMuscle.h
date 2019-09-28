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

#pragma once

#include "Max.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "simpobj.h"

#include "..\CATControls\CatPlugins.h"
#include "..\CATControls\Math.h"

 // flag to stop our muscles from evaluating during a reset
extern BOOL g_bIsResetting;
extern BOOL g_bKeepRollouts;
extern class Muscle* muscleCopy;

extern HINSTANCE hInstance;

#define CATMUSCLE_VERSION_0860		((DWORD) 860)
#define CATMUSCLE_VERSION_0850		((DWORD) 850)
#define CATMUSCLE_VERSION_0400		((DWORD) 400)
#define CATMUSCLE_VERSION_0200		((DWORD) 200)
#define CATMUSCLE_VERSION_0100		((DWORD) 100)
#define CATMUSCLE_VERSION_START		((DWORD) 000)

BOOL DeSelectAndDelete(INode* node);
INode* FindParent(INode* node);
