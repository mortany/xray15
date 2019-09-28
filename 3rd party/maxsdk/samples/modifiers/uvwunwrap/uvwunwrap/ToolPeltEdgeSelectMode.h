/*

Copyright 2015 Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/

#pragma once

#include "matrix3.h"

class MeshTopoData;

enum HitGeomType
{
	HitGeomType_None,
	HitGeomType_Vertex,
	HitGeomType_Edge,
};

class GeomHit : public MaxHeapOperators
{
public:
	GeomHit()
	{
		Reset();
	}

	GeomHit(HitGeomType geomType, int index)
		: mGeomType(geomType)
		, mIndex(index)
	{
	}

	virtual ~GeomHit()
	{
	}

	bool operator==(const GeomHit &other) const
	{
		return mGeomType == other.mGeomType && mIndex == other.mIndex;
	}

	bool operator!=(const GeomHit &other) const
	{
		return !operator==(other);
	}

	void Reset()
	{
		mGeomType = HitGeomType_None;
		mIndex = -1;
	}

	bool IsValid() const
	{
		return mGeomType != HitGeomType_None && mIndex >= 0;
	}

public:
	HitGeomType mGeomType;
	int mIndex;
};

class MouseProcHitTestInfo : public MaxHeapOperators
{
public:
	MouseProcHitTestInfo()
		: mLD(nullptr)
	{
	}

	virtual ~MouseProcHitTestInfo()
	{
	}

	bool IsValid() const
	{
		return mHit.IsValid() && mLD;
	}

public:
	GeomHit mHit;
	Matrix3 mTM;
	MeshTopoData *mLD;
};