/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/
#pragma once
#include "tab.h"
#include "../UvwunwrapBase/LSCMData.h"

class IMeshTopoData;
class ClusterNewPackData;
class ABFPeelData;

// this interface is used to expose certain methods from ClusterClass for internal use only.
class IClusterInternal
{
public:
	virtual ~IClusterInternal() {}

	virtual IMeshTopoData* GetOwnerMeshTopoData() = 0;
	virtual Tab<int>& GetFaces() = 0;
	virtual Tab<int>& GetVerts() = 0;
	virtual float GetWidth() = 0;
	virtual float GetHeight() = 0;
	virtual void SetWidth(float w) = 0;
	virtual void SetHeight(float h) = 0;
	virtual float GetNewX() = 0;
	virtual float GetNewY() = 0;
	virtual void SetNewX(float x) = 0;
	virtual void SetNewY(float y) = 0;
	virtual Point3 GetOffset() = 0;
	virtual void SetOffset(const Point3& pt3) = 0;
	virtual Tab<Point2>& GetHorizontalValues() = 0;
	virtual Tab<Point2>& GetVerticalValues() = 0;
	virtual Box3& GetBounds() = 0;
	virtual ClusterNewPackData& GetNewPackData() = 0;
	virtual bool GetPasted() = 0;
	virtual void SetPasted(bool b) = 0;
	virtual float GetSurfaceArea() = 0;
};

// this interface is used to expose certain methods from LSCMCLusterData for internal use only.
class ILSCMClusterData
{
public:
	virtual ~ILSCMClusterData() {}
	virtual bool IsValidGeom() = 0;
	virtual bool IsResolveRequired() = 0;
	virtual Tab<LSCMFace>& GetFaceData() = 0;
	virtual ABFPeelData& GetABFPeelData() = 0;
	virtual void PreparePinDataForLSCM(Tab<LSCMFace> &faceData, IMeshTopoData * md) = 0;
	virtual BitArray& GetClusterVerts() = 0;
};