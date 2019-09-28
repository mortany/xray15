#include "UnwrapRenderItem.h"
#include "unwrap.h"

#include <Graphics/IVirtualDevice.h>
#include <Graphics/IDisplayManager.h>
#include <Graphics/IViewportViewSetting.h>
#include "modsres.h"

MaxSDK::Graphics::MaterialRequiredStreams UnwrapRenderItem::msFormat;
MaxSDK::Graphics::HLSLMaterialHandle UnwrapRenderItem::mUnwrapShaderDX11;

UnwrapRenderItem::UnwrapRenderItem(UnwrapMod* pData,INode* pNode)
	: mpUnwrapMod(pData),
	mpMaxNode(pNode),
	mbDX11Mode(true),
	mEdgeThickness(1.0),
	mbShowSeamEdges(TRUE),
	mbShowOpenEdges(TRUE)
{
	using namespace MaxSDK::Graphics;
	DeviceCaps caps;
	IDisplayManager* pManager = GetIDisplayManager();
	DbgAssert(nullptr != pManager);
	pManager->GetDeviceCaps(caps);
	
	if (caps.FeatureLevel < Level4_0)
	{
		mbDX11Mode = false;
	}
	else
	{
		mbDX11Mode = true;

		if (!mUnwrapShaderDX11.IsValid())
			mUnwrapShaderDX11.InitializeWithResource(IDR_UNWRAP_RENDER_ITEM_FX, MaxSDK::GetHInstance(), L"SHADERS");

		if (0 == msFormat.GetNumberOfStreams())
		{
			MaterialRequiredStreamElement posElement;
			posElement.SetChannelCategory(MeshChannelPosition);
			posElement.SetType(VertexFieldFloat3);
			msFormat.AddStream(posElement);
		}
	}
}

void UnwrapRenderItem::Realize(MaxSDK::Graphics::DrawContext& drawContext)
{	
	if (!mbDX11Mode)
	{
		return;
	}
	else
	{
		RealizeDX11(drawContext);
	}
}

enum LinesType
{
	eNoType = 0,
	eSeamPreview,
	eOpenEdge,
	eOpenEdgeSelected,
	eSeam,
	eRegularSelectedEdge,
	eTypeCount
};

void UnwrapRenderItem::RealizeDX11(MaxSDK::Graphics::DrawContext& drawContext)
{
	if (mpUnwrapMod->ip)
	{
		bool bFisrtTimeRealization = false;
		if (!mUnwrapIB.IsValid())
		{
			InitializeData();
			bFisrtTimeRealization = true;
		}
		
		MeshTopoData *md = nullptr;
		for (int whichLD = 0; whichLD < mpUnwrapMod->GetMeshTopoDataCount(); whichLD++)
		{
			md = mpUnwrapMod->GetMeshTopoData(whichLD);

			//need the match between the mesh topo data and the render item that belong to the same node.
			// And the attribute data need updating or user modify the seam or open edges display check box.
			if(mpUnwrapMod->GetMeshTopoDataNode(whichLD) == mpMaxNode &&
				md &&
				(md->GetSynchronizationToRenderItemFlag() || 
					bFisrtTimeRealization || 
					md->mbSeamEdgesPreviewDirty ||
					md->mbOpenTVEdgesDirty ||
					md->mbSeamEdgesDirty ||
					mbShowSeamEdges != mpUnwrapMod->fnGetAlwayShowPeltSeams() ||
					mbShowOpenEdges != mpUnwrapMod->fnGetViewportOpenEdges()))
			{
				mbShowSeamEdges = mpUnwrapMod->fnGetAlwayShowPeltSeams();
				mbShowOpenEdges = mpUnwrapMod->fnGetViewportOpenEdges();

				if(bFisrtTimeRealization || md->mbOpenTVEdgesDirty)
				{
					mOpenGeomEdges = md->GetOpenGeomEdges();
				}				
				int iOpenEdgeCount = mOpenGeomEdges.GetSize();
				int iSeamCount = md->mSeamEdges.GetSize();

				unsigned int* attributeBuffer = (unsigned int*)mUnwrapAttributeVB.Lock(0, 0);
				if(attributeBuffer == nullptr)
				{
					continue;
				}
				for (int i = 0; i < md->GetNumberGeomEdges(); i++)
				{					
					bool bPassFilter = md->DoesGeomEdgePassFilter(i);
					if(bPassFilter && md->GetSeamEdgesPreview(i))
					{
						//if it is preview of the seam edge
						*attributeBuffer = eSeamPreview;
					}
					else if (bPassFilter &&
							mbShowOpenEdges &&
							iOpenEdgeCount > 0 && 
							i < iOpenEdgeCount && 
							mOpenGeomEdges[i])
					{
						//if it is open geometry edge						

						if(!md->GetGeomEdgeSelected(i))
						{
							
							*attributeBuffer = eOpenEdge;
						}
						else
						{
							// if it is selected,then take it as the selected one
							*attributeBuffer = eOpenEdgeSelected;
						}
					}
					else if(bPassFilter &&
							mbShowSeamEdges &&
							iSeamCount > 0 
							&& i < iSeamCount 
							&& md->mSeamEdges[i])
					{
						//if it is seam edge
						*attributeBuffer = eSeam;
					}
					else if(bPassFilter && md->GetGeomEdgeSelected(i))
					{
						 *attributeBuffer = eRegularSelectedEdge;
					}
					else
					{
						 *attributeBuffer = eNoType;
					}
					attributeBuffer++;
				}
				
				mUnwrapAttributeVB.Unlock();
				
				//After the data is updated, set all flags as false
				md->SetSynchronizationToRenderItemFlag(FALSE);
				md->mbSeamEdgesPreviewDirty = false;				
				md->mbSeamEdgesDirty		= false;

				if(md->mbOpenTVEdgesDirty)
				{
					md->mbOpenTVEdgesDirty	= false;
				}

				//after the node and mesh topo data is matched, break.
				break;
			}				
		}

		//For selected edges, they often show red.But if add the Unwrap UVW modifier on the incoming selected faces,
		//then select some edges again,they often show red on the red face and cause the show not obvious,
		//So choose the selected gizmo color to replace red.
		if(md && md->HasIncomingFaceSelection())
		{
			mSelectedEdgeColor =  GetUIColor(COLOR_SEL_GIZMOS);
		}
		else
		{
			mSelectedEdgeColor.x = (float)GetRValue(mpUnwrapMod->selColor)/255.0f;
			mSelectedEdgeColor.y = (float)GetGValue(mpUnwrapMod->selColor)/255.0f;
			mSelectedEdgeColor.z = (float)GetBValue(mpUnwrapMod->selColor)/255.0f;
		}

		mSeamEdgePreviewColor.x = (float)GetRValue(mpUnwrapMod->selPreviewColor)/255.0f;
		mSeamEdgePreviewColor.y = (float)GetGValue(mpUnwrapMod->selPreviewColor)/255.0f;
		mSeamEdgePreviewColor.z = (float)GetBValue(mpUnwrapMod->selPreviewColor)/255.0f;

		COLORREF seamColorRef = ColorMan()->GetColor(PELTSEAMCOLORID);
		mSeamEdgeColor.x = (float)GetRValue(seamColorRef)/255.0f;
		mSeamEdgeColor.y = (float)GetGValue(seamColorRef)/255.0f;
		mSeamEdgeColor.z = (float)GetBValue(seamColorRef)/255.0f;

		mOpenEdgeColor.x = (float)GetRValue(mpUnwrapMod->openEdgeColor)/255.0f;
		mOpenEdgeColor.y = (float)GetGValue(mpUnwrapMod->openEdgeColor)/255.0f;
		mOpenEdgeColor.z = (float)GetBValue(mpUnwrapMod->openEdgeColor)/255.0f;

		//Will get from the designer
		mOpenEdgeSelectedColor.x = 1.0f;
		mOpenEdgeSelectedColor.y = 0.1f;
		mOpenEdgeSelectedColor.z = 0.9f;

		//This thickness will be used in the shader file UnwrapShader.fx by passing the attribute buffer
		mEdgeThickness = mpUnwrapMod->fnGetThickOpenEdges() ? 1.0f : 0.5f;
	}
}

void UnwrapRenderItem::InitializeData()
{
	using namespace MaxSDK::Graphics;

	for (int whichLD = 0; whichLD < mpUnwrapMod->GetMeshTopoDataCount(); whichLD++)
	{
		MeshTopoData *md = mpUnwrapMod->GetMeshTopoData(whichLD);

		//need the match between the mesh topo data and the render item that belong to the same node.
		if((mpUnwrapMod->GetMeshTopoDataNode(whichLD) == mpMaxNode) &&
			md)
		{
			int numGeomVerts = md->GetNumberGeomVerts();
			int numGeomEdges = md->GetNumberGeomEdges();

			if(numGeomVerts > 0 &&
				numGeomEdges > 0)
			{
				using namespace MaxSDK::Graphics;
				VertexBufferHandle posVB;
				posVB.Initialize(sizeof(Point3), numGeomVerts);
				mUnwrapVB.append(posVB);

				mUnwrapIB.Initialize(IndexTypeInt, numGeomEdges*2);
				mUnwrapAttributeVB.Initialize(sizeof(DWORD), numGeomEdges, nullptr, (BufferUsageType)(BufferUsageStatic | BufferUsageStructure));

				Point3* posBuffer = (Point3*)mUnwrapVB[0].Lock(0, 0);
				for (int i = 0; i < numGeomVerts; i++)
				{
					*posBuffer = md->GetGeomVert(i);
					posBuffer++;
				}
				mUnwrapVB[0].Unlock();

				int* indexBuffer = (int*)mUnwrapIB.Lock(0, 0, WriteAcess);
				for (int i = 0; i < numGeomEdges; i++)
				{
					*indexBuffer = md->GetGeomEdgeVert(i,0);
					indexBuffer++;

					*indexBuffer = md->GetGeomEdgeVert(i,1);
					indexBuffer++;
				}
				mUnwrapIB.Unlock();
			}
			break;
		}
	}
}

void UnwrapRenderItem::Display(MaxSDK::Graphics::DrawContext& drawContext)
{
	const MaxSDK::Graphics::IViewportViewSetting* pViewportSetting = drawContext.GetViewportSettings();
	if(pViewportSetting &&
		pViewportSetting->GetViewportVisualStyle() == MaxSDK::Graphics::VisualStyleBoundingBox)
	{
		return;
	}

	if (mpUnwrapMod->ip &&
		mbDX11Mode)
	{		
		if (!mUnwrapIB.IsValid()
			|| 0 == mUnwrapIB.GetNumberOfIndices())
		{
			return;
		}

		BOOL bShowSelectedEdges = (mpUnwrapMod->ip->GetSubObjectLevel() == 2 && !mpUnwrapMod->GetLivePeelModeEnabled()) ? TRUE : FALSE;

		using namespace MaxSDK::Graphics;
		IVirtualDevice& vd = drawContext.GetVirtualDevice();

		mUnwrapShaderDX11.SetBufferParameter(L"gSelectionAttribute", mUnwrapAttributeVB);

		mUnwrapShaderDX11.SetFloat3Parameter(L"gSelectedEdgeColor", mSelectedEdgeColor);
		mUnwrapShaderDX11.SetBoolParameter(L"gShowSelectedEdges", bShowSelectedEdges);

		mUnwrapShaderDX11.SetFloat3Parameter(L"gSeamEdgePreviewColor", mSeamEdgePreviewColor);
		mUnwrapShaderDX11.SetFloat3Parameter(L"gSeamEdgeColor", mSeamEdgeColor);

		mUnwrapShaderDX11.SetFloat3Parameter(L"gOpenEdgeColor", mOpenEdgeColor);
		mUnwrapShaderDX11.SetFloat3Parameter(L"gOpenEdgeSelectedColor", mOpenEdgeSelectedColor);

		mUnwrapShaderDX11.SetFloatParameter(L"gEdgeThickness", mEdgeThickness);

		vd.SetStreamFormat(msFormat);
		vd.SetVertexStreams(mUnwrapVB);
		vd.SetIndexBuffer(mUnwrapIB);

		mUnwrapShaderDX11.Activate(drawContext);
		mUnwrapShaderDX11.ActivatePass(drawContext, 0);
		vd.Draw(PrimitiveLineList, 0, (int)mUnwrapIB.GetNumberOfIndices() / 2);
		mUnwrapShaderDX11.PassesFinished(drawContext);
		mUnwrapShaderDX11.Terminate();
	}
}

size_t UnwrapRenderItem::GetPrimitiveCount() const
{
	if (mpUnwrapMod->ip &&
		mbDX11Mode)
	{
		return mUnwrapIB.GetNumberOfIndices() / 2;
	}

	return 0;
}