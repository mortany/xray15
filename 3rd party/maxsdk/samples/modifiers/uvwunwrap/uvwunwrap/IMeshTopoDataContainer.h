/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/
#pragma once
#include "point3.h"
#include "inode.h"
#include "matrix3.h"

#define TVERT_FACE_CHANGED	0x0001

class IMeshTopoData;
class Mesh;
class MNMesh;
class PatchMesh;

// This class is used to respond the events raised by IMeshTopoData and used internally.
class IMeshTopoDataChangeListener
{
public:
	virtual ~IMeshTopoDataChangeListener() {}
	virtual int OnTVDataChanged(IMeshTopoData* pTopoData, BOOL bUpdateView) = 0;
	virtual int OnTVertFaceChanged(IMeshTopoData* pTopoData, BOOL bUpdateView) = 0;
	virtual int OnSelectionChanged(IMeshTopoData* pTopoData, BOOL bUpdateView) = 0;
	virtual int OnPinAddedOrDeleted(IMeshTopoData* pTopoData, int index) = 0;
	virtual int OnPinInvalidated(IMeshTopoData* pTopoData, int index) = 0;
	virtual int OnTopoInvalidated(IMeshTopoData* pTopoData) = 0;
	virtual int OnTVVertChanged(IMeshTopoData* pTopoData, TimeValue t, int index, const Point3& p) = 0;
	virtual int OnTVVertDeleted(IMeshTopoData* pTopoData, int index) = 0;
	virtual int OnMeshTopoDataDeleted(IMeshTopoData* pTopoData) = 0;
	virtual int OnNotifyUpdateTexmap(IMeshTopoData* pTopoData) = 0;
	virtual int OnNotifyUIInvalidation(IMeshTopoData* pTopoData, BOOL bRedrawAtOnce) = 0;
	virtual int OnNotifyFaceSelectionChanged(IMeshTopoData* pTopoData) = 0;
};

// This class is used to expose certain methods from MeshTopoData for internal usage.
class IMeshTopoData
{
public:
	virtual ~IMeshTopoData() {}
	virtual int RegisterNotification(IMeshTopoDataChangeListener* listener) = 0;
	virtual int UnRegisterNotification(IMeshTopoDataChangeListener* listener) = 0;
	virtual int RaiseTVDataChanged(BOOL bUpdateView) = 0;
	virtual int RaiseTVertFaceChanged(BOOL bUpdateView) = 0;
	virtual int RaiseSelectionChanged(BOOL bUpdateView) = 0;
	virtual int RaisePinAddedOrDeleted(int index) = 0;
	virtual int RaisePinInvalidated(int index) = 0;
	virtual int RaiseTopoInvalidated() = 0;
	virtual int RaiseTVVertChanged(TimeValue t, int index, const Point3& p) = 0;
	virtual int RaiseTVVertDeleted(int index) = 0;
	virtual int RaiseMeshTopoDataDeleted() = 0;
	virtual int RaiseNotifyUpdateTexmap() = 0;
	virtual int RaiseNotifyUIInvalidation(BOOL bRedrawAtOnce) = 0;
	virtual int RaiseNotifyFaceSelectionChanged() = 0;

	virtual Mesh *GetMesh() = 0;
	virtual MNMesh *GetMNMesh() = 0;
	virtual PatchMesh *GetPatch() = 0;

	virtual int GetTVVertControlIndex(int index) = 0;
	virtual void SetTVVertControlIndex(int index, int id) = 0;
	virtual int GetFaceDegree(int faceIndex) = 0;
	virtual Point3 GetTVVert(int index) = 0;
	virtual void SetTVVert(TimeValue t, int index, Point3 p) = 0;
	virtual int GetFaceTVVert(int faceIndex, int ithVert) = 0;
	virtual int GetNumberTVVerts() = 0;
	virtual Point3 GetGeomVert(int index) = 0;
	virtual float AngleFromVectors(Point3 vec1, Point3 vec2) = 0;
	virtual BOOL IsTVVertPinned(int index) = 0;
};

// This class is used to expose certain methods from MeshTopoDataContainer for internal usage.
class IMeshTopoDataContainer
{
public:
	virtual ~IMeshTopoDataContainer() {}
	virtual int Count() = 0;
	virtual IMeshTopoData* Get(int i) = 0;

	virtual Point3 GetNodeColor(int index) = 0;
	virtual Matrix3 GetNodeTM(TimeValue t, int index) = 0;
	virtual INode* GetNode(int index) = 0;
	virtual void HoldPointsAndFaces() = 0;
	virtual void HoldPoints() = 0;
	virtual void HoldSelection() = 0;
};
