//
// Copyright 2015 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#include "PFBoxGeometry.h"
#include <Graphics/IVirtualDevice.h>

using namespace MaxSDK::Graphics;

static MaterialRequiredStreams sPositionOnlyFormat;
PFBoxGeometry::PFBoxGeometry(Box3& box)
{
	static USHORT boxEdgeIndices[] = {
		0,1,1,3,3,2,2,0,
		4,5,5,7,7,6,6,4,
		0,4,1,5,3,7,2,6
	};
	mBoxIB.Initialize(IndexTypeShort, 24, boxEdgeIndices);
	VertexBufferHandle boxVB;
	Point3 boxVBData[] = 
	{
		box[0],box[1],box[2],box[3],box[4],box[5],box[6],box[7]
	};
	boxVB.Initialize(sizeof(Point3), 8, boxVBData);
	mBoxVB.append(boxVB);
	if (sPositionOnlyFormat.GetNumberOfStreams() == 0)
	{
		MaterialRequiredStreamElement ele;
		ele.SetChannelCategory(MeshChannelPosition);
		ele.SetType(VertexFieldFloat3);
		sPositionOnlyFormat.AddStream(ele);
	}
}

PFBoxGeometry::~PFBoxGeometry()
{

}

void PFBoxGeometry::Display(DrawContext& drawContext, int start, int count, int lod)
{
	IVirtualDevice& vd = drawContext.GetVirtualDevice();
	vd.SetStreamFormat(sPositionOnlyFormat);
	vd.SetVertexStreams(mBoxVB);
	vd.SetIndexBuffer(mBoxIB);
	vd.Draw(PrimitiveLineList, 0, count);
}

PrimitiveType PFBoxGeometry::GetPrimitiveType()
{
	return PrimitiveLineList;
}

void PFBoxGeometry::SetPrimitiveType(PrimitiveType /*type*/)
{

}

size_t PFBoxGeometry::GetPrimitiveCount()
{
	return 12;
}

size_t PFBoxGeometry::GetVertexCount()
{
	return 8;
}
	
MaterialRequiredStreams& PFBoxGeometry::GetSteamRequirement()
{
	return sPositionOnlyFormat;
}

IndexBufferHandle& PFBoxGeometry::GetIndexBuffer()
{
	return mBoxIB;
}

VertexBufferHandleArray& PFBoxGeometry::GetVertexBuffers()
{
	return mBoxVB;
}

int PFBoxGeometry::GetStartPrimitive() const
{
	return 0;
}
