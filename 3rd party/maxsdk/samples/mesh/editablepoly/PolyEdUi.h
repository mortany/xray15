//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license
//  agreement provided at the time of installation or download, or which
//  otherwise accompanies this software in either electronic or hard copy form.
//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <winnt.h>

#define EDITPOLY_REGULAR(type, iconName, cmd) \
	ToolButtonItem(type,iconName,EditPolyIcon::IconSize,EditPolyIcon::IconSize,24,23,cmd)

namespace EditPolyIcon {
	//icon size
	static int IconSize = 16;


	//EditablePoly rollup
	static const TCHAR* vertexSelection = _T("Common/Vertex");
	static const TCHAR* edgeSelection = _T("Common/Edge");
	static const TCHAR* borderSelection = _T("Common/Border");
	static const TCHAR* polygonSelection = _T("Common/Polygon");
	static const TCHAR* elementSelection = _T("Common/Element");

	//Modifier Stack
	static const TCHAR* stackVertex = _T("CommandPanel/Modify/ModifierStack/VertexStack");
	static const TCHAR* stackEdge = _T("CommandPanel/Modify/ModifierStack/EdgeStack");
	static const TCHAR* stackBorder = _T("CommandPanel/Modify/ModifierStack/BorderStack");
	static const TCHAR* stackPolygon = _T("CommandPanel/Modify/ModifierStack/PloygonStack");
	static const TCHAR* stackElement = _T("CommandPanel/Modify/ModifierStack/ElementStack");
}