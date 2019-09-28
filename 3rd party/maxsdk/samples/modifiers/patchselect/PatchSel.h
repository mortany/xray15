//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license
//  agreement provided at the time of installation or download, or which
//  otherwise accompanies this software in either electronic or hard copy form.
//
//////////////////////////////////////////////////////////////////////////////
/**********************************************************************
*<
SelectPatch/Icon names
*>
**********************************************************************/
#pragma once
#include <winnt.h>

#define SELECTPATCH_REGULAR(type, iconName, cmd) \
	ToolButtonItem(type,iconName,SelectPatchIcon::IconSize,SelectPatchIcon::IconSize,24,23,cmd)

namespace SelectPatchIcon {
	//icon size
	static int IconSize = 16;

	//SelectPatch rollup
	static const TCHAR* vertexSelection = _T("Common/VertexEditPatch");
	static const TCHAR* handleSelection = _T("Common/Handle");
	static const TCHAR* edgeSelection = _T("Common/EdgeEditPatch");
	static const TCHAR* patchSelection = _T("Common/Patch");
	static const TCHAR* elementSelection = _T("Common/Element");

	//Modifier Stack
	static const TCHAR* stackVertex = _T("CommandPanel/Modify/ModifierStack/VertexEditPatchStack");
	static const TCHAR* stackHandle = _T("CommandPanel/Modify/ModifierStack/HandleStack");
	static const TCHAR* stackEdge = _T("CommandPanel/Modify/ModifierStack/EdgeEditPatchStack");
	static const TCHAR* stackPatch = _T("CommandPanel/Modify/ModifierStack/PatchStack");
	static const TCHAR* stackElement = _T("CommandPanel/Modify/ModifierStack/ElementStack");
}