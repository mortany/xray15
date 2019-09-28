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
EditableMesh/Icon names
*>
**********************************************************************/
#pragma once
#include <winnt.h>

#define EDITABLEMESH_REGULAR(type, iconName, cmd) \
	ToolButtonItem(type,iconName,EditableMeshIcon::IconSize,EditableMeshIcon::IconSize,24,23,cmd)

namespace EditableMeshIcon {
	//icon size
	static int IconSize = 16;


	//EditableMesh rollup
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