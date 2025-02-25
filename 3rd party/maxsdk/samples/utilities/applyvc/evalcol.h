/**********************************************************************
 *<
	FILE:			EvalColor.cpp
	DESCRIPTION:	Vertex Color Renderer
	CREATED BY:		Christer Janson
	HISTORY:		Created Monday, December 12, 1996

 *>	Copyright (c) 1997 Kinetix, All Rights Reserved.
 **********************************************************************/
//
// Description:
// These functions calculates the diffuse or ambient color at each vertex
// or face of an INode.
//
// Exports:
// BOOL calcMixedVertexColors(INode*, TimeValue, int, ColorTab&, EvalColProgressCallback* callb = NULL);
//      This function calculates the interpolated diffuse or ambient 
//      color at each vertex of an INode.
//      Usage: Pass in a node pointer and the TimeValue to generate
//      a list of Colors corresponding to each vertex in the mesh
//      Use the int flag to specify if you want to have diffuse or 
//      ambient colors, or if you want to use the lights in the scene.
//      Note: 
//        You are responsible for deleting the Color objects in the table.
//      Additional note:
//        Since materials are assigned by face, this function renders each
//        face connected to the specific vertex (at the point of the vertex)
//        and then mixes the colors.
//
//***************************************************************************

#ifndef __EVALCOL__H
#define __EVALCOL__H

#include "istdplug.h"

//#define LIGHT_AMBIENT		0x00
//#define LIGHT_DIFFUSE		0x01
//#define LIGHT_SCENELIGHT	0x02

// SR NOTE64: was previously
//    using IAssignVertexColors_R7::LightingModel;
// which is not kosher with VS2005 (outside a derived class def).

typedef IAssignVertexColors_R7::Options2   EvalVCOptions;

typedef IVertexPaint::FaceColor		FaceColor;
typedef IVertexPaint::FaceColorTab	FaceColorTab;
typedef IVertexPaint::VertColorTab	ColorTab;

// Progress callback for the evaluation functions below
class EvalColProgressCallback {
public:
	virtual BOOL progress(float prog) = 0;
};

// Returns the active radiosity engine, if compatible with this utility. NULL otherwise.
RadiosityEffect* GetCompatibleRadiosity();

class MeshInstance;
typedef Tab<MeshInstance*> MeshInstanceTab;
BOOL calcMixedVertexColors(INode* node, TimeValue t, const EvalVCOptions& evalOptions, ColorTab& vxColTab, MeshInstanceTab&	instanceList, EvalColProgressCallback* callb = NULL);
BOOL calcFaceColors(INode* node, TimeValue t, const EvalVCOptions& evalOptions, FaceColorTab& faceColTab, MeshInstanceTab&	instanceList, EvalColProgressCallback* callb = NULL);


#endif //__EVALCOL__H
