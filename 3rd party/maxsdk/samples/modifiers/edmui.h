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
EditMesh/Icon names
*>
**********************************************************************/
#pragma once
#include <winnt.h>

#define EDITMESH_REGULAR(type, iconName, cmd) \
	ToolButtonItem(type,iconName,EditMeshIcon::IconSize,EditMeshIcon::IconSize,24,23,cmd)

namespace EditMeshIcon {
	//icon size
	static int IconSize = 16;


	//EditMesh rollup
	static const TCHAR* vertexSelection = _T("Common/Vertex");
	static const TCHAR* edgeSelection = _T("Common/Edge");
	static const TCHAR* faceSelection = _T("Common/Face");
	static const TCHAR* polygonSelection = _T("Common/Polygon");
	static const TCHAR* elementSelection = _T("Common/Element");

	//Modifier Stack
	static const TCHAR* stackVertex = _T("CommandPanel/Modify/ModifierStack/VertexStack");
	static const TCHAR* stackPolygon = _T("CommandPanel/Modify/ModifierStack/PloygonStack");
	static const TCHAR* stackElement = _T("CommandPanel/Modify/ModifierStack/ElementStack");
	static const TCHAR* stackFace = _T("CommandPanel/Modify/ModifierStack/FaceStack");
	static const TCHAR* stackEdge = _T("CommandPanel/Modify/ModifierStack/EdgeStack");
}