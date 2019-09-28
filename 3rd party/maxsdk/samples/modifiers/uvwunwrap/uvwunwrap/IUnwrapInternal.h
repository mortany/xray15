/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/
#pragma once

class IClusterInternal;

// this interface is used to expose certain methods from Unwrap Modifier for internal use only.
class IUnwrapInternal
{
public:
	virtual ~IUnwrapInternal() {}

	virtual BOOL RotateClusters(float &area) = 0;
	virtual void CalculateClusterBounds(int i) = 0;
	virtual BOOL CollapseClusters(float spacing, BOOL rotateClusters, BOOL onlyCenter) = 0;
	virtual int GetClusterCount() = 0;
	virtual IClusterInternal* GetCluster(int idx) = 0;
	virtual void RemoveCluster(int idx) = 0;
	virtual void SortClusterList(CompareFnc fnc) = 0;
	virtual float GetSArea() = 0;
	virtual float GetBArea() = 0;
	virtual void SetSArea(float f) = 0;
	virtual void SetBArea(float f) = 0;
};
