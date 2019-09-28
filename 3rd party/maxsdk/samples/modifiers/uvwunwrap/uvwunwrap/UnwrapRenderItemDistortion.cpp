//
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.   
//
//

#include "UnwrapRenderItemDistortion.h"
#include "unwrap.h"
#include <Graphics/IVirtualDevice.h>

MaxSDK::Graphics::MaterialRequiredStreams UnwrapRenderItemDistortion::msFormatDistortion;
UnwrapRenderItemDistortion::UnwrapRenderItemDistortion(UnwrapMod* pData,INode* pNode)
	: mpUnwrapMod(pData),
	mpMaxNode(pNode)
{
	using namespace MaxSDK::Graphics;
	mpVertexColorMaterialDistortion.Initialize();
	if (0 == msFormatDistortion.GetNumberOfStreams())
	{
		MaterialRequiredStreamElement positionStream;
		positionStream.SetType(VertexFieldFloat3);
		positionStream.SetChannelCategory(MeshChannelPosition);
		positionStream.SetUsageIndex(0);
		positionStream.SetStreamIndex(0);
		positionStream.SetOffset(0);
		msFormatDistortion.AddStream(positionStream);

		MaterialRequiredStreamElement normalStream;
		normalStream.SetType(VertexFieldFloat3);
		normalStream.SetChannelCategory(MeshChannelVertexNormal);
		normalStream.SetUsageIndex(0);
		normalStream.SetStreamIndex(1);
		normalStream.SetOffset(0);
		msFormatDistortion.AddStream(normalStream);

		MaterialRequiredStreamElement vertexColorStream;
		vertexColorStream.SetType(VertexFieldColor);
		vertexColorStream.SetChannelCategory(MeshChannelVertexColor);
		vertexColorStream.SetUsageIndex(0);
		vertexColorStream.SetStreamIndex(2);
		vertexColorStream.SetOffset(0);
		msFormatDistortion.AddStream(vertexColorStream);
	}
}

void UnwrapRenderItemDistortion::Realize(MaxSDK::Graphics::DrawContext& drawContext)
{
	if(!mpUnwrapMod->viewValid ||
		GetCOREInterface()->IsAnimPlaying())
	{
		Painter2D::Singleton().ResetDistortionElement();
		Painter2D::Singleton().MakeDistortion();
	}

	for (int whichLD = 0; whichLD < mpUnwrapMod->GetMeshTopoDataCount(); whichLD++)
	{
		//need the match between the mesh topo data and the render item that belong to the same node.
		if(mpUnwrapMod->GetMeshTopoDataNode(whichLD) == mpMaxNode)
		{
			int iVertexCountDistortion = Painter2D::Singleton().mDistortion3DElements[whichLD].mVertexCount;

			if((mpUnwrapMod->GetDistortionType() == eAngleDistortion ||
				mpUnwrapMod->GetDistortionType() == eAreaDistortion )&&
				iVertexCountDistortion >= 3)
			{
				using namespace MaxSDK::Graphics;

				mBufferArrayDistortion.removeAll();

				//position data
				VertexBufferHandle posVB;
				posVB.Initialize(sizeof(Point3), iVertexCountDistortion, Painter2D::Singleton().mDistortion3DElements[whichLD].mPositions.data());
				mBufferArrayDistortion.append(posVB);

				//normal data
				VertexBufferHandle normalVB;
				normalVB.Initialize(sizeof(Point3), iVertexCountDistortion, Painter2D::Singleton().mDistortion3DElements[whichLD].mNormals.data());
				mBufferArrayDistortion.append(normalVB);

				//color data
				VertexBufferHandle colorVB;
				colorVB.Initialize(sizeof(DWORD), iVertexCountDistortion);
				DWORD* pColorResult = (DWORD*)colorVB.Lock(0,iVertexCountDistortion);
				UBYTE b = 255;
				UBYTE g = 255;
				UBYTE r = 255;
				for (int i = 0; i < iVertexCountDistortion; i++)
				{
					r = (UBYTE) (Painter2D::Singleton().mDistortion3DElements[whichLD].mColors[i].x * 255.0);
					g = (UBYTE) (Painter2D::Singleton().mDistortion3DElements[whichLD].mColors[i].y * 255.0);
					b = (UBYTE) (Painter2D::Singleton().mDistortion3DElements[whichLD].mColors[i].z * 255.0);
					*pColorResult = (DWORD)(0xff000000 | (b<<16) | (g << 8) | r);
					pColorResult++;
				}
				colorVB.Unlock();
				mBufferArrayDistortion.append(colorVB);

				//index data
				mIndexBufferDistortion.Initialize(IndexTypeInt, iVertexCountDistortion, Painter2D::Singleton().mDistortion3DElements[whichLD].mPositionsIndex.data());
			}
		}
	}
}

void UnwrapRenderItemDistortion::Display(MaxSDK::Graphics::DrawContext& drawContext)
{
	if(mpUnwrapMod->showMap)
	{
		for (int whichLD = 0; whichLD < mpUnwrapMod->GetMeshTopoDataCount(); whichLD++)
		{
			//need the match between the mesh topo data and the render item that belong to the same node.
			if(mpUnwrapMod->GetMeshTopoDataNode(whichLD) == mpMaxNode)
			{
				int iVertexCountDistortion = Painter2D::Singleton().mDistortion3DElements[whichLD].mVertexCount;	
				if((mpUnwrapMod->GetDistortionType() == eAngleDistortion ||
					mpUnwrapMod->GetDistortionType() == eAreaDistortion) &&
					iVertexCountDistortion >= 3)
				{
					using namespace MaxSDK::Graphics;
					IVirtualDevice& vd = drawContext.GetVirtualDevice();

					vd.SetStreamFormat(msFormatDistortion);
					vd.SetVertexStreams(mBufferArrayDistortion);
					vd.SetIndexBuffer(mIndexBufferDistortion);

					mpVertexColorMaterialDistortion.Activate(drawContext);
					int iPassCount = mpVertexColorMaterialDistortion.GetPassCount(drawContext);
					for(int iPass=0;iPass<iPassCount;iPass++)
					{
						mpVertexColorMaterialDistortion.ActivatePass(drawContext,iPass);
						vd.Draw(PrimitiveTriangleList, 0, int(iVertexCountDistortion/3));
					}
					mpVertexColorMaterialDistortion.PassesFinished(drawContext);
				}
			}
		}		
	}
}

size_t UnwrapRenderItemDistortion::GetPrimitiveCount() const
{
	if(mpUnwrapMod->GetDistortionType() == eAngleDistortion ||
		mpUnwrapMod->GetDistortionType() == eAreaDistortion)
	{
		if(!mBufferArrayDistortion.isEmpty())
		{
			return mBufferArrayDistortion[0].GetNumberOfVertices()/3;
		}
	}

	return 0;
}