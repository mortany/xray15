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

#include "CATHierarchyRoot.h"

extern HWND CreateCATWindow(HINSTANCE hInstance, HWND hWnd, CATHierarchyRoot *catHierarchyRoot);

extern HWND CreateFloatingGraphWindow(HINSTANCE hInst, HWND hwndParent,
	CATHierarchyRoot *pHierarchyRoot,
	CATHierarchyBranch *pGraphableBranch);
