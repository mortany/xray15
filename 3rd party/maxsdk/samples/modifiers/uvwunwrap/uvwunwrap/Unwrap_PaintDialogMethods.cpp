/********************************************************************** *<
FILE: Unwrap_PaintDialogMethods.cpp

DESCRIPTION: A UVW map modifier unwraps the UVWs onto the image dialog paint methdos

HISTORY: 9/26/2006
CREATED BY: Peter Watje


*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/


#ifndef NOMINMAX
#define NOMINMAX 1; //so that minwindef.h does not the define min/max macros
#endif

#include "unwrap.h"
#include "modsres.h"
#include "utilityMethods.h"
#include <Graphics/IPrimitiveRenderer.h>
#include <Graphics/IVirtualDevice.h>
#include <Graphics/IDisplayManager.h>
#include "IColorCorrectionMgr.h"
#include <algorithm> // std::sort
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#define SAFE_RELEASE(P) if(P){P->Release() ; P = NULL ;}
#define SideDelta 0.003f
#define FilledColorRedWithAlpha Point4(0.7,0.0,0.0,0.5)
#define FacePreviewRedWithAlpha Point4(0.5,0.5,0.25,0.8)

#include "PerformanceTools.h"
static int globalTimerID = 300;
static double timeElapsed = 0; // used to record how much time to finish a paint call.
static double time2 = -1.0; // used to help record how many times the paint is executed in about 10s.
static int tickCount = 0; // used to record how many times the paint is called in about 10s

DrawingElement::DrawingElement()
	: mVertexCount(0)
{}

DrawingElement::~DrawingElement()
{
	Clear();
}

void DrawingElement::Clear()
{
	mPositions.clear();
	mNormals.clear();
	mColors.clear();
	mPositionsIndex.clear();
	mOtherPositions.clear();
	Reset();
}

void DrawingElement::Reset()
{
	mVertexCount = 0;
}

void QuadShape::Build(float left,float top,float right,float bottom)
{
	//////        0...........1(3)
	////          .          . .
	///           .       .    .
	///           .    .       .
	///           2(4).........5
	/////
	//vertex positions and texture coordinates

	mPositions[0].x = left;
	mPositions[0].y = top;
	mPositions[0].z = 0;
	mTexcoords[0]	= 0.0;
	mTexcoords[1]	= 1.0;
	mTexcoords[2]	= 0.0;

	mPositions[1].x = right;
	mPositions[1].y = top;
	mPositions[1].z = 0;
	mTexcoords[3]	= 1.0;
	mTexcoords[4]	= 1.0;
	mTexcoords[5]	= 0.0;

	mPositions[2].x = left;
	mPositions[2].y = bottom;
	mPositions[2].z = 0;
	mTexcoords[6]	= 0.0;
	mTexcoords[7]	= 0.0;
	mTexcoords[8]	= 0.0;

	mPositions[3].x = right;
	mPositions[3].y = top;
	mPositions[3].z = 0;
	mTexcoords[9]	= 1.0;
	mTexcoords[10]	= 1.0;
	mTexcoords[11]	= 0.0;

	mPositions[4].x = left;
	mPositions[4].y = bottom;
	mPositions[4].z = 0;
	mTexcoords[12]	= 0.0;
	mTexcoords[13]	= 0.0;
	mTexcoords[14]	= 0.0;

	mPositions[5].x = right;
	mPositions[5].y = bottom;
	mPositions[5].z = 0;
	mTexcoords[15]	= 1.0;
	mTexcoords[16]	= 0.0;
	mTexcoords[17]	= 0.0;

	//index of positions
	mPositionsIndex[0] = 0;
	mPositionsIndex[1] = 2;
	mPositionsIndex[2] = 1;

	mPositionsIndex[3] = 3;
	mPositionsIndex[4] = 4;
	mPositionsIndex[5] = 5;

	//normal
	for (int i=0;i<6;i++)
	{
		mNormal[i].x = 0.0;
		mNormal[i].y = 0.0;
		mNormal[i].z = 1.0;
	}
}

TextContainer::~TextContainer()
{
	Reset();
}

void TextContainer::Reset()
{
	mTexts.clear();
	mPositions.clear();
}

//Window2DDisplayCallback is used for Nitrous driver in 2D window
class Window2DDisplayCallback: public MaxSDK::Graphics::IDisplayCallback
{
public:
	Window2DDisplayCallback(Painter2D* pPainter2D) {mpPainter2D = pPainter2D;active = 0;}
private:
	virtual void DoDisplay(TimeValue t, MaxSDK::Graphics::IPrimitiveRenderer& renderer,const MaxSDK::Graphics::DisplayCallbackContext& displayContext);
	int active;
	Painter2D* mpPainter2D;
};

void Window2DDisplayCallback::DoDisplay(TimeValue t, MaxSDK::Graphics::IPrimitiveRenderer& renderer,const MaxSDK::Graphics::DisplayCallbackContext& displayContext)
{
	if(mpPainter2D)
	{
		mpPainter2D->DoDisplay(t,renderer,displayContext);
	}
}

Painter2D::Painter2D()	:
	mSelFacesFillMode(-1)
{
	if(MaxSDK::Graphics::IsRetainedModeEnabled())
	{		
		MaxSDK::Graphics::DeviceCaps caps;
		MaxSDK::Graphics::IDisplayManager* pManager = MaxSDK::Graphics::GetIDisplayManager();
		DbgAssert(nullptr != pManager);
		pManager->GetDeviceCaps(caps);
		if (caps.FeatureLevel < MaxSDK::Graphics::Level4_0)
		{
			mbDX11Mode = false;
		}
		else
		{
			mbDX11Mode = true;
		}
	}
	else
	{
		mbDX11Mode = false;
	}
}

Painter2D::~Painter2D()
{
}

Painter2D& Painter2D::Singleton()
{
	static Painter2D s_Painter2D;
	return s_Painter2D;
}

bool Painter2D::CheckDX11Mode()
{
	return mbDX11Mode;
}

void Painter2D::SetUnwrapMod(UnwrapMod* pUnwrapMod)
{
	mUnwrapModRefMaker.SetRef(pUnwrapMod);
}

const std::set<Painter2D::uvOffset>& Painter2D::GetTilesContainer()
{
	if ( GetUnwrapModPtr() && !GetUnwrapModPtr()->showMultiTile )
	{   // If "show multi tile" turns off, we still want to compute the tiles
		ComputeTiles( true );
	}
	return mTilesContainer;
}

UnwrapMod* Painter2D::GetUnwrapModPtr()
{ 
	return dynamic_cast<UnwrapMod*>(mUnwrapModRefMaker.GetRef());
}

void  Painter2D::DoDisplay(TimeValue t, MaxSDK::Graphics::IPrimitiveRenderer& renderer,const MaxSDK::Graphics::DisplayCallbackContext& displayContext)
{
	if(nullptr == GetUnwrapModPtr())
	{
		return;
	}

	using namespace MaxSDK::Graphics;
	//Background
	if(GetUnwrapModPtr()->showMap &&
		GetUnwrapModPtr()->GetDistortionType() == eUndefined)
	{
		paintTilesBackground(renderer);
	}

	//For background lines such as grid
	CommitLineList(renderer,mGridLines);
	//For background bold lines such as axis
	CommitTriangleList(renderer,mAxisBoldLines);	

	//For polygon that contain filled pattern
	if(mpFaceVertexes && miFaceVertexesCount > 0 && miFacesCount > 0)
	{
		renderer.SetMaterial(mVertexColorMaterial);
		DrawPatternFaces(renderer);
	}

	//Paint the angle or area distortion
	if(GetUnwrapModPtr()->showMap)
	{
		if (GetUnwrapModPtr()->GetDistortionType() == eAngleDistortion ||
			GetUnwrapModPtr()->GetDistortionType() == eAreaDistortion)
		{
			int iVertexCount = mDistortion2DElement.mVertexCount;
			if(iVertexCount >= 3)
			{
				renderer.SetMaterial(mVertexColorMaterial);
				SimpleVertexStream simpleVertexS;
				simpleVertexS.Positions = mDistortion2DElement.mPositions.data();
				simpleVertexS.Normals	= mDistortion2DElement.mNormals.data();
				simpleVertexS.Colors	= mDistortion2DElement.mColors.data();
				renderer.DrawIndexedPrimitiveUP(
					PrimitiveTriangleList,
					simpleVertexS,
					iVertexCount/3,
					mDistortion2DElement.mPositionsIndex.data(),
					iVertexCount);
			}
		}
	}

	//For edges
	CommitLineList(renderer,mUVRegularEdges);
	CommitLineList(renderer,mUVSelectedEdges);
	CommitLineList(renderer,mUVLines);
	CommitLineList(renderer,mFaceSelectedLines);

	//For vertex ticks
	CommitLineList(renderer,mVertexTicks);

	//for gizmo
	CommitLineList(renderer,mGizmoLines);
	CommitTriangleList(renderer,mGizmoPolygons);
	CommitTriangleList(renderer,mGizmoTransparentPolygons,true);

	//draw text
	int iCount = int(mTextContainer.mTexts.size());
	if(iCount > 0)
	{
		float xzoom, yzoom;
		int width,height;
		GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

		for (int i=0; i<iCount;++i)
		{
			//Text will not be affected by the world transform matrix, so it needs the position in the screen space.
			int tx = (width-int(xzoom))/2;
			int ty = (height-int(yzoom))/2;
			//transfer the value from the UVW space to the screen.
			float x = mTextContainer.mPositions[i].x*xzoom+GetUnwrapModPtr()->xscroll+tx;
			float y = height-mTextContainer.mPositions[i].y*yzoom+GetUnwrapModPtr()->yscroll-ty;
			Point3 textPos(x,y,0.0f);
			//draw text
			renderer.DrawText(textPos,mTextContainer.mTexts[i]);
		}
	}

	if (GetUnwrapModPtr()->bShowFPSinEditor)
	{
		//indicate the Mudbox format UV index in the right-top corner of this tile
		TSTR FPSStr;
		if (timeElapsed > 0)
			FPSStr.printf(_T("FPS(only paint): %f"), 1000.0 / timeElapsed);
		else
			FPSStr = _T("FPS(only paint): N/A");
		Point3 FPS_pos(30.0f, 70.0f, 0.0f); //show at left-top corner
		renderer.DrawText(FPS_pos, FPSStr);

		if (time2 >= 0) // >= 0 means it started
		{
			tickCount++;
			time2 = MaxSDK::PerformanceTools::Timer::EndTimerGlobal(globalTimerID + 1);

			TSTR str2;
			if (time2 > 0)
				str2.printf(_T("FPS(all): %f"), tickCount * 1000 / time2);
			else
				str2 = _T("FPS(all): N/A");
			Point3 FPS_pos(30.0f, 100.0f, 0.0f); //show below the FPS(only paint) info.
			renderer.DrawText(FPS_pos, str2);
		}

		if (time2 < 0 || time2 > 10000) //reset after about 10s
		{
			MaxSDK::PerformanceTools::Timer::StartTimerGlobal(globalTimerID + 1);
			tickCount = 0;
			time2 = 0;
		}
	}
}


void Painter2D::paintTilesBackground(MaxSDK::Graphics::IPrimitiveRenderer& renderer)
{	
	UnwrapMod* pUnWrapModifier = GetUnwrapModPtr();
	
	if ( mTextureHandlesContainer.size() > 0 )
	{
		for ( const auto &uvTile : mTilesContainer )
		{
			auto it = std::find_if( mTextureHandlesContainer.begin(), mTextureHandlesContainer.end(), 
				[&]( const multiTextureElement &elem ){
					return (elem.uvInfor == uvTile);
				});

			
			if ( it != mTextureHandlesContainer.end() )
			{
				mBackgroundMaterial.SetTexture( it->singleTextureHandle );
			}
			else
			{   //If the corresponding texture handle in the container is not found, use the checker texture handle 
				mBackgroundMaterial.SetTexture( mBackgroundOneTextureHandle );
			}

			if (pUnWrapModifier && pUnWrapModifier->filterMap)
			{
				mBackgroundMaterial.SetFilters(MaxSDK::Graphics::TextureMaterialHandle::FilterMode::FilterLinear);
			}
			else
			{
				mBackgroundMaterial.SetFilters(MaxSDK::Graphics::TextureMaterialHandle::FilterMode::FilterNone);
			}
			

			renderer.SetMaterial(mBackgroundMaterial);
			paintSingleTileBackground( renderer, uvTile.first, uvTile.second );
		}
	}
	else
	{
		mBackgroundMaterial.SetTexture(mBackgroundOneTextureHandle);
		if (pUnWrapModifier && pUnWrapModifier->filterMap)
		{
			mBackgroundMaterial.SetFilters(MaxSDK::Graphics::TextureMaterialHandle::FilterMode::FilterLinear);
		}
		else
		{
			mBackgroundMaterial.SetFilters(MaxSDK::Graphics::TextureMaterialHandle::FilterMode::FilterNone);
		}

		renderer.SetMaterial(mBackgroundMaterial);

		for ( const auto &uvTile : mTilesContainer )
		{
			paintSingleTileBackground( renderer, uvTile.first, uvTile.second );
		}
	}
}

void Painter2D::paintSingleTileBackground(MaxSDK::Graphics::IPrimitiveRenderer& renderer,int iUStart,int iVStart)
{
	using namespace MaxSDK::Graphics;

	//indicate the range of left-top corner and right-bottom corner
	mBackgroundImageQuad.Build(iUStart,
		iVStart+1,
		iUStart+1,
		iVStart);

	SimpleVertexStream simpleVertexQuad;
	simpleVertexQuad.Positions	= const_cast<Point3*>(mBackgroundImageQuad.GetPositionBuffer());
	simpleVertexQuad.Normals	= const_cast<Point3*>(mBackgroundImageQuad.GetNormal());
	simpleVertexQuad.TextureStreams[0].Data			= const_cast<float*>(mBackgroundImageQuad.GetTexcoords());
	simpleVertexQuad.TextureStreams[0].Dimension	= 3;
	renderer.DrawIndexedPrimitiveUP(
		PrimitiveTriangleList,
		simpleVertexQuad,
		mBackgroundImageQuad.GetTriangleCount(),
		mBackgroundImageQuad.GetPositionsIndex(),
		mBackgroundImageQuad.GetTriangleCount()*3);

	if(GetUnwrapModPtr()->showMultiTile)
	{
		//indicate the Mudbox format UV index in the right-top corner of this tile
		TSTR tileIndexStr;
		tileIndexStr.printf( _T("U%dV%d"), 
			(iUStart < 0) ? iUStart : iUStart + 1, 
			(iVStart < 0) ? iVStart : iVStart + 1 );

		static const float fTextPosUOffset = 0.8f;
		static const float fTextPosVOffset = 0.9f;
		mTextContainer.mPositions.emplace_back( iUStart + fTextPosUOffset, iVStart + fTextPosVOffset);
		mTextContainer.mTexts.push_back(tileIndexStr);
	}
}

void Painter2D::CommitLineList(MaxSDK::Graphics::IPrimitiveRenderer& renderer,DrawingElement& dElement)
{
	using namespace MaxSDK::Graphics;

	int iCount = dElement.mVertexCount;
	if(iCount > 1)
	{
		renderer.SetMaterial(mVertexColorMaterial);

		SimpleVertexStream simpleVertexS;

		UnwrapMod* pUnwrapMod = GetUnwrapModPtr();
		if (pUnwrapMod->fnGetNonSquareApplyBitmapRatio())
		{
			static float aspectThreshold = 0.01f;
			if (pUnwrapMod->bRelaxFinished == FALSE &&
				abs(pUnwrapMod->aspect - 1.0) > aspectThreshold)
			{
				//Non-square need adjust the y value when displaying the intermidate relax result.
				for (auto& it : dElement.mPositions)
				{
					it.y = (it.y - 0.5)*pUnwrapMod->aspect + 0.5;
				}
			}
		}		

		simpleVertexS.Positions = dElement.mPositions.data();		
		simpleVertexS.Normals	= dElement.mNormals.data();
		simpleVertexS.Colors	= dElement.mColors.data();

		renderer.DrawIndexedPrimitiveUP(
			PrimitiveLineList,
			simpleVertexS,
			iCount/2,
			dElement.mPositionsIndex.data(),
			iCount);
	}
}

void Painter2D::CommitTriangleList(MaxSDK::Graphics::IPrimitiveRenderer& renderer,DrawingElement& dElement,bool bRequireTransparantStatus)
{
	using namespace MaxSDK::Graphics;
	int iCount = dElement.mVertexCount;
	if(iCount >= 3 )
	{
		renderer.SetMaterial(mVertexColorMaterial);

		IVirtualDevice& vDevice = renderer.GetVirtualDevice();
		BlendState blendStateBack;
		if(bRequireTransparantStatus)
		{
			blendStateBack = vDevice.GetBlendState();
			BlendState			newBlendStatus;
			TargetBlendState& targetBlendStatus = newBlendStatus.GetTargetBlendState(0);
			SetTransparentBlendStatus(targetBlendStatus);
			vDevice.SetBlendState(newBlendStatus);
		}

		SimpleVertexStream simpleVertexS;
		simpleVertexS.Positions = dElement.mPositions.data();
		simpleVertexS.Normals	= dElement.mNormals.data();
		simpleVertexS.Colors	= dElement.mColors.data();
		renderer.DrawIndexedPrimitiveUP(
			PrimitiveTriangleList,
			simpleVertexS,
			iCount/3,
			dElement.mPositionsIndex.data(),
			iCount);

		//Restore the old blend status after requiring transparent status
		if(bRequireTransparantStatus)
		{
			vDevice.SetBlendState(blendStateBack);
		}
	}
}

void Painter2D::PaintBackground()
{
	if (!GetUnwrapModPtr()->tileValid)
	{
		GetUnwrapModPtr()->tileValid = TRUE;

		mTextureHandlesContainer.clear();
		for ( const auto &image : GetUnwrapModPtr()->imagesContainer )
		{
			MaxSDK::Graphics::TextureHandle hTexture;
			hTexture.Initialize(GetUnwrapModPtr()->rendW, GetUnwrapModPtr()->rendH, MaxSDK::Graphics::TargetFormatA8R8G8B8);
			FillBKTextureHandleData(hTexture, image.singleImage);
			mTextureHandlesContainer.emplace_back( image.uvInfor, hTexture );
		}
		
		FillBKTextureHandleData(mBackgroundOneTextureHandle, GetUnwrapModPtr()->image);
	}
}

using namespace tbb;

void AssignPixel(UBYTE *sourceImage,
				 COLORREF* pStart,				 
				 int iWidth,
				 int y,
				 BOOL bTile,
				 BOOL brightCenterTile,
				 BOOL blendTileToBackGround,
				 float contrast,
				 const Point3& backColor,
				 int scanw)
{	
	UBYTE *pSource = sourceImage +  (y*scanw);
	UBYTE b = 255;
	UBYTE g = 255;
	UBYTE r = 255;
	COLORREF *pTarget = nullptr;
	for (int x = 0; x < iWidth; x++)
	{
		pTarget = pStart + y*iWidth + x;
		if (bTile)
		{
			if (!brightCenterTile)
			{
				b = pSource[x*3];
				g = pSource[x*3+1];
				r = pSource[x*3+2];
			}
			else
			{
				if (blendTileToBackGround)
				{
					b = (UBYTE) (backColor.z + ( (float)pSource[x*3]-backColor.z) * contrast);
					g = (UBYTE) (backColor.y + ( (float)pSource[x*3+1]-backColor.y) * contrast);
					r = (UBYTE) (backColor.x + ( (float)pSource[x*3+2]-backColor.x) * contrast);
				}
				else
				{
					b = (UBYTE) ((float)pSource[x*3] * contrast);
					g = (UBYTE) ((float)pSource[x*3+1] * contrast);
					r = (UBYTE) ((float)pSource[x*3+2] * contrast);
				}
			}
		}
		else
		{
			if ((contrast == 1.0f) || (!brightCenterTile))
			{
				b = pSource[x*3];
				g = pSource[x*3+1];
				r = pSource[x*3+2];
			}
			else if (blendTileToBackGround)
			{
				b = (UBYTE) (backColor.z + ( (float)pSource[x*3]-backColor.z) * contrast);
				g = (UBYTE) (backColor.y + ( (float)pSource[x*3+1]-backColor.y) * contrast);
				r = (UBYTE) (backColor.x + ( (float)pSource[x*3+2]-backColor.x) * contrast);
			}
			else
			{	
				b = (UBYTE) ((float)pSource[x*3] * contrast);
				g = (UBYTE) ((float)pSource[x*3+1] * contrast);
				r = (UBYTE) ((float)pSource[x*3+2] * contrast);
			}
		}

		*pTarget = (DWORD)(0xff000000 | (r<<16) | (g << 8) | b);
	}
}


class fill_background{	
	UBYTE* sourceImage;
	COLORREF *pStart;
	int width;	
	BOOL bTile;
	BOOL bRightCenterTile;
	BOOL bBlendTileToBackGround;
	float contrast;
	const Point3 backColor;
	int scanw;
public:  
	fill_background (UBYTE *ima,COLORREF* pBits,int w,BOOL bTi,BOOL bRi,BOOL bBl,float contr,Point3 colr): 
		sourceImage(ima),
		pStart(pBits),
		width(w),		
		bTile(bTi),
		bRightCenterTile(bRi),
		bBlendTileToBackGround(bBl),
		contrast(contr),
		backColor(colr),
		scanw(ByteWidth(width)){}
	void operator()(const blocked_range<int> & r) const
	{
		for (int y=r.begin(); y!=r.end(); y++)
		{			
			AssignPixel(sourceImage,pStart,width,y,bTile,bRightCenterTile,bBlendTileToBackGround,contrast,backColor,scanw);
		}
	}  
};

void Painter2D::FillBKTextureHandleData(MaxSDK::Graphics::TextureHandle& targetTextureHandle,UBYTE* sourceImage)
{
	if(NULL == sourceImage)
	{
		return;
	}

	UnwrapMod* pUnWrapModifier = GetUnwrapModPtr();
	//After the function SetupImage is called, the image width or height may change.
	if(targetTextureHandle.GetWidth() != pUnWrapModifier->iw || 
		targetTextureHandle.GetHeight() != pUnWrapModifier->ih)
	{
		targetTextureHandle.Initialize(pUnWrapModifier->iw,pUnWrapModifier->ih,MaxSDK::Graphics::TargetFormatA8R8G8B8);
	}

	Point3 backColor;
	backColor.x =  GetRValue(pUnWrapModifier->backgroundColor);
	backColor.y =  GetGValue(pUnWrapModifier->backgroundColor);
	backColor.z =  GetBValue(pUnWrapModifier->backgroundColor);

	int w = pUnWrapModifier->iw;
	int h = pUnWrapModifier->ih;
	BOOL bTi = pUnWrapModifier->bTile;
	BOOL bRi = pUnWrapModifier->brightCenterTile;
	BOOL bBl = pUnWrapModifier->blendTileToBackGround;
	float contrast = pUnWrapModifier->fContrast;

	MaxSDK::Graphics::LockedRect lRec;
	targetTextureHandle.WriteOnlyLockRectangle(0,lRec,nullptr);
	COLORREF *pStart = (COLORREF*)lRec.pBits;

	parallel_for(blocked_range<int>(0, h), fill_background(sourceImage,pStart,w,bTi,bRi,bBl,contrast,backColor), auto_partitioner());
	
	targetTextureHandle.WriteOnlyUnlockRectangle();
}

void Painter2D::PaintGrid()
{
	//draw grid
	int i1, i2;
	GetUnwrapModPtr()->GetUVWIndices(i1,i2);
	float xzoom, yzoom;
	int width,height;
	GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

	//x0 is in the UVW space, it is coresponding to the screen x == 0
	float x0 = 0.5 - (0.5*width+GetUnwrapModPtr()->xscroll)/xzoom;
	//x1 is in the UVW space, it is coresponding to the screen x == width
	float x1 = 0.5 + (0.5*width-GetUnwrapModPtr()->xscroll)/xzoom;
	//The direction of Y axis is inversed in the UVW to the screen
	//y0 is in the UVW space, it is coresponding to the screen y == 0
	float y0 = 0.5 + (0.5*height+GetUnwrapModPtr()->yscroll)/yzoom;
	//y1 is in the UVW space, it is coresponding to the screen y == height
	float y1 = 0.5 - (0.5*height-GetUnwrapModPtr()->yscroll)/yzoom;

	//The gird unit size is from the 0.0 to 1.0
	float fGirdSize = GetUnwrapModPtr()->gridSize;
	if ((GetUnwrapModPtr()->gridVisible) && (fGirdSize>0.0f))
	{
		Point4 gridColorP4((float)GetRValue(GetUnwrapModPtr()->gridColor)/255.0f,
			(float)GetGValue(GetUnwrapModPtr()->gridColor)/255.0f,
			(float)GetBValue(GetUnwrapModPtr()->gridColor)/255.0f,
			1.0f);

		//let the grid line align with the UV axis
		//do vertical lines in X+ 
		float x = 0;		
		while (x < x1)
		{
			DrawSingleEdge(
				mGridLines,
				x,
				y0,
				x,
				y1,
				gridColorP4);

			x+= fGirdSize;
		}

		//do vertical lines in X-
		x = -fGirdSize;
		while (x > x0)
		{
			DrawSingleEdge(
				mGridLines,
				x,
				y0,
				x,
				y1,
				gridColorP4);

			x-= fGirdSize;
		}

		//do horizontal lines in Y+
		float y = 0;		
		while (y < y0)
		{
			DrawSingleEdge(
				mGridLines,
				x0,
				y,
				x1,
				y,
				gridColorP4);

			y+= fGirdSize;
		}

		//do horizontal lines in Y-
		y = -fGirdSize;		
		while (y > y1)
		{
			DrawSingleEdge(
				mGridLines,
				x0,
				y,
				x1,
				y,
				gridColorP4);

			y-= fGirdSize;
		}

	}

	if ((GetUnwrapModPtr()->tileGridVisible))
	{
		Point4 penColorP4((float)GetRValue(GetUnwrapModPtr()->gridColor) *0.45f/255.0f,
			(float)GetGValue(GetUnwrapModPtr()->gridColor) *0.45f/255.0f,
			GetBValue(GetUnwrapModPtr()->gridColor) *0.45f/255.0f,
			1.0f);

		for ( const auto &uvTile : mTilesContainer )
		{   //indicate the range of left-top corner and right-bottom corner
			DrawRectangle(
				mAxisBoldLines,
				uvTile.first,
				uvTile.second + 1,
				uvTile.first + 1,
				uvTile.second,
				penColorP4, 3);
		}
	

		//The horizontal line is from the [x0, 0.0] to the [x1, 0.0] in the UVW space
		DrawSingleEdge(
			mAxisBoldLines,
			x0,
			0.0,
			x1,
			0.0,
			penColorP4,3);

		//The vertical line is from the [0.0, y1] to the [0.0, y0] in the UVW space
		DrawSingleEdge(
			mAxisBoldLines,
			0.0,
			y1,
			0.0,
			y0,
			penColorP4,3);
	}
}

// give the delta value for length and width of rectangle in the UVW space
void Painter2D::PaintTick(DrawingElement& targetLines,float x, float y, BOOL largeTick,Point4& tickColor)
{
	static float fSideDeltaForTick = 0.0015f;
	//The offset is inversely proportional to the zoom in the UVW space
	float fYOffset = fSideDeltaForTick / GetUnwrapModPtr()->zoom;
	float fXOffset = fYOffset / GetUnwrapModPtr()->aspect;
	if (largeTick)
	{
		if (GetUnwrapModPtr()->tickSize==1)
		{
			DrawSingleEdge(
				targetLines,
				x-fXOffset,
				y+fYOffset,
				x+fXOffset,
				y-fYOffset,
				tickColor);
		}
		else 
		{
			DrawRectangle(targetLines,x-fXOffset,y+fYOffset,x+fXOffset,y-fYOffset,tickColor);
		}		
	}
	else
	{
		DrawRectangle(targetLines,x-fXOffset,y+fYOffset,x+fXOffset,y-fYOffset,tickColor);
	}
}

void Painter2D::PaintX(DrawingElement& targetLines,float x, float y, float size,Point4& color)
{
	DrawSingleEdge(
		targetLines,
		x-size,
		y-size,
		x+size,
		y+size,
		color);

	DrawSingleEdge(
		targetLines,
		x+size,
		y-size,
		x-size,
		y+size,
		color);

	DrawRectangle(targetLines,x-size,y-size,x+size,y+size,color);
}

void Painter2D::PaintVertexInfluence()
{
	Point4 unselColorP4((float)GetRValue(GetUnwrapModPtr()->lineColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->lineColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->lineColor)/255.0f,
		1.0f);

	int index1 = 0;
	int index2 = 1;
	GetUnwrapModPtr()->GetUVWIndices(index1,index2);

	for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[ldID];
		if (ld == NULL)
		{
			DbgAssert(0);
			continue;
		}

		//get our soft selection colors
		Point3 selSoft = GetUIColor(COLOR_SUBSELECTION_SOFT);
		Point3 selMedium = GetUIColor(COLOR_SUBSELECTION_MEDIUM);
		Point3 selHard = GetUIColor(COLOR_SUBSELECTION_HARD);
		int frozenr = (int) (GetRValue(GetUnwrapModPtr()->lineColor)*0.5f);
		int frozeng = (int) (GetGValue(GetUnwrapModPtr()->lineColor)*0.5f);
		int frozenb = (int) (GetBValue(GetUnwrapModPtr()->lineColor)*0.5f);
		COLORREF frozenColor = RGB(frozenr,frozeng,frozenb);
		//draw the weighted ticks
		for (int i=0; i< ld->GetNumberTVVerts(); i++) 
		{
			if (ld->IsTVVertVisible(i) && (!ld->GetTVVertSelected(i) && !ld->GetTVVertSelectPreview(i)))
			{
				Point3 selColor(0.0f,0.0f,0.0f);

				float influence = ld->GetTVVertInfluence(i);

				if (influence <= 0.0f)
				{
					influence = 0.0f;
				}
				else if (influence <0.5f)
					selColor = selSoft + ( (selMedium-selSoft) * (influence*2.0f));
				else if (influence<=1.0f)
					selColor = selMedium + ( (selHard-selMedium) * ((influence-0.5f)*2.0f));

				BOOL largeTick = FALSE;
				Point4 weightedTicksColor(0.0f,0.0f,0.0f,1.0f);
				if (ld->GetTVVertFrozen(i))
				{
					weightedTicksColor.x = (float)GetRValue(frozenColor)/255.0f;
					weightedTicksColor.y = (float)GetGValue(frozenColor)/255.0f;
					weightedTicksColor.z = (float)GetBValue(frozenColor)/255.0f;
					weightedTicksColor.w = 1.0f;
				}
				else if (influence == 0.0f)
				{
					weightedTicksColor = unselColorP4;
				}
				else 
				{
					weightedTicksColor.x = selColor.x;
					weightedTicksColor.y = selColor.y;
					weightedTicksColor.z = selColor.z;
					weightedTicksColor.w = 1.0f;
					largeTick = TRUE;
				}
				float x  = ld->GetTVVert(i)[index1];
				float y  = ld->GetTVVert(i)[index2];
				PaintTick(mVertexTicks,x,y,largeTick,weightedTicksColor);
			}
		}
	}
}

void Painter2D::PaintVertexTicks()
{
	Point4 selColorP4((float)GetRValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->selColor)/255.0f,
		1.0f);
	Point4 selPreviewColorP4((float)GetRValue(GetUnwrapModPtr()->selPreviewColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->selPreviewColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->selPreviewColor)/255.0f,
		1.0f);
	Point4 sharedColorP4((float)GetRValue(GetUnwrapModPtr()->sharedColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->sharedColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->sharedColor)/255.0f,
		1.0f);

	int index1 = 0;
	int index2 = 1;
	GetUnwrapModPtr()->GetUVWIndices(index1,index2);

	for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[ldID];
		if (ld == NULL)
		{
			DbgAssert(0);
			continue;
		}

		//build our shared list
		BitArray usedClusters;

		if (GetUnwrapModPtr()->showVertexClusterList || GetUnwrapModPtr()->showShared) 
		{
			usedClusters.SetSize(ld->GetNumberGeomVerts());
			usedClusters.ClearAll();
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (ld->GetTVVertSelected(i))
				{
					int gIndex = ld->GetTVVertGeoIndex(i);
					if (gIndex >= 0) //need this check since a tv vert can not be attached to a geo vert
						usedClusters.Set(gIndex);
				}
			}
		}


		//Draw our selected vertex ticks
		for (int i=0; i< ld->GetNumberTVVerts(); i++) 
		{
			//PELT
			if (ld->IsTVVertVisible(i) && (ld->GetTVVertSelected(i) || ld->GetTVVertSelectPreview(i)))
			{
				bool bPaintPreviewVertex = ld->GetTVVertSelectPreview(i) && !ld->GetTVVertSelected(i) ;
				float x  = ld->GetTVVert(i)[index1];
				float y  = ld->GetTVVert(i)[index2];

				PaintTick(mVertexTicks,x,y,TRUE, bPaintPreviewVertex ? selPreviewColorP4 : selColorP4);

				if ( (GetUnwrapModPtr()->showVertexClusterList) )
				{

					int vCluster = ld->GetTVVertGeoIndex(i);
					if ((vCluster >=0) && (usedClusters[vCluster]))
					{

						TSTR vertStr;
						vertStr.printf(_T("%d"),vCluster);
						mTextContainer.mPositions.push_back(Point2(x+SideDelta,y-SideDelta));
						mTextContainer.mTexts.push_back(vertStr);
					}
				}
			}

			//draw the unselected vertex that shared the same geometry vertex with the selected vertex.
			if ((GetUnwrapModPtr()->showVertexClusterList || GetUnwrapModPtr()->showShared) &&
				ld->IsTVVertVisible(i) && 
				(!ld->GetTVVertSelected(i)))
			{
				int vCluster = ld->GetTVVertGeoIndex(i);
				if ((vCluster >=0) && (usedClusters[vCluster]))
				{
					float x  = ld->GetTVVert(i)[index1];
					float y  = ld->GetTVVert(i)[index2];

					PaintTick(mVertexTicks,x,y,TRUE, sharedColorP4);
				}
			}

		}
	}
}

void Painter2D::GetPeelModeFaces(UnwrapMod* unwrapMod, MeshTopoData* md, BitArray& peelFaces)
{
	if (md == nullptr)
		return;
	peelFaces.SetSize(md->GetNumberFaces());
	//grab the data from the peel mode tool
	LSCMLocalData *ld = unwrapMod->mLSCMTool.GetData(md);
	if(ld != nullptr)
	{
		for (int i = 0; i < ld->NumberClusterData(); i++)
		{
			LSCMClusterData* cluster = ld->GetClusterData(i);
			if (cluster == nullptr)
				continue;
			int faceCt = cluster->NumberFaces();
			for (int j = 0; j < faceCt; j++)
			{
				LSCMFace* lface = cluster->GetFace(j);
				if (lface == nullptr)
					continue;
				//mark this face
				peelFaces.Set(lface->mPolyIndex);
			}
		}
	}
}

void Painter2D::PaintEdges()
{
	auto unwrapMod = GetUnwrapModPtr();

	//setup the colors
	int frozenr = (int) (GetRValue(unwrapMod->lineColor)*0.5f);
	int frozeng = (int) (GetGValue(unwrapMod->lineColor)*0.5f);
	int frozenb = (int) (GetBValue(unwrapMod->lineColor)*0.5f);
	Point4 frozenColorP4(
		frozenr / 255.0f,
		frozeng / 255.0f,
		frozenb / 255.0f, 1.0f);
	Point4 selColorP4(
		(float)GetRValue(unwrapMod->selColor)/255.0f,
		(float)GetGValue(unwrapMod->selColor)/255.0f,
		(float)GetBValue(unwrapMod->selColor)/255.0f,
		1.0f);
	Point4 selPreviewColorP4(
		(float)GetRValue(unwrapMod->selPreviewColor)/255.0f,
		(float)GetGValue(unwrapMod->selPreviewColor)/255.0f,
		(float)GetBValue(unwrapMod->selPreviewColor)/255.0f,
		1.0f);
	Point4 openEdgeColorP4(
		(float)GetRValue(unwrapMod->openEdgeColor)/255.0f,
		(float)GetGValue(unwrapMod->openEdgeColor)/255.0f,
		(float)GetBValue(unwrapMod->openEdgeColor)/255.0f,
		1.0f);
	Point4 handleColorP4(
		(float)GetRValue(unwrapMod->handleColor)/255.0f,
		(float)GetGValue(unwrapMod->handleColor)/255.0f,
		(float)GetBValue(unwrapMod->handleColor)/255.0f,
		1.0f);
	Point4 sharedColorP4(
		(float)GetRValue(unwrapMod->sharedColor)/255.0f,
		(float)GetGValue(unwrapMod->sharedColor)/255.0f,
		(float)GetBValue(unwrapMod->sharedColor)/255.0f,
		1.0f);
	Point4 peelColorP4(
		(float)GetRValue(unwrapMod->peelColor) / 255.0f,
		(float)GetGValue(unwrapMod->peelColor) / 255.0f,
		(float)GetBValue(unwrapMod->peelColor) / 255.0f,
		1.0f);
	Point3 baseLineColor(
		(float)GetRValue(unwrapMod->lineColor) / 255.0f,
		(float)GetGValue(unwrapMod->lineColor) / 255.0f,
		(float)GetBValue(unwrapMod->lineColor) / 255.0f);
	

	BOOL showLocalDistortion;
	TimeValue t = GetCOREInterface()->GetTime();
	unwrapMod->pblock->GetValue(unwrap_localDistorion,t,showLocalDistortion,FOREVER);

	int index1 = 0;
	int index2 = 1;
	unwrapMod->GetUVWIndices(index1,index2);

	//are we in peel mode
	BOOL bLivePeel = unwrapMod->GetLivePeelModeEnabled();
	
	for (int whichLD = 0; whichLD < unwrapMod->mMeshTopoData.Count(); whichLD++)
	{
		MeshTopoData *md = unwrapMod->mMeshTopoData[whichLD];

		//determine the color
		Point3 lColor = baseLineColor;
		Point3 nColor = unwrapMod->mMeshTopoData.GetNodeColor(whichLD);
		if ( unwrapMod->mMeshTopoData.Count() > 1)
			lColor = lColor + (nColor - lColor) * 0.75f;
		Point4 unselColorP4(lColor.x,lColor.y,lColor.z,1.0f);
		
		int iSharedEdgeCount = md->mSharedTVEdges.GetSize();
		int iOpenEdgeCount = md->mOpenTVEdges.GetSize();

		//if we are in peel mode, grab the faces
		BitArray peelFaces;
		if(bLivePeel)
		{
			GetPeelModeFaces(unwrapMod, md, peelFaces);
		}

		//draw regular edges
		for (int i=0; i< md->GetNumberTVEdges(); i++) 
		{			
			int a = md->GetTVEdgeVert(i,0);
			int b = md->GetTVEdgeVert(i,1);

			//determine if we paint the edge or not
			BOOL paintEdge = TRUE;
			if (!unwrapMod->displayHiddenEdges && md->GetTVEdgeHidden(i))
			{
				if (unwrapMod->mTVSubObjectMode == TVEDGEMODE)
				{
					if (!md->GetTVEdgeSelected(i))
						paintEdge =FALSE;
				}
				else
				{
					paintEdge =FALSE;
				}
			}

			//if we paint the edge and the edge is visible, paint it
			if (paintEdge && md->IsTVVertVisible(a) && md->IsTVVertVisible(b))
			{
				bool sharedEdge = false; //is this a shared edge?
				if (unwrapMod->showShared && //do we show shared edges?
					iSharedEdgeCount > 0 && //are there any shared edges?
					i < iSharedEdgeCount &&	md->mSharedTVEdges[i]) //is the edge shared?
				{
					sharedEdge = true; //this is a shared edge
				}

				bool bShowOpenEdge = false;		 //is this an open edge?		
				if(unwrapMod->displayOpenEdges && //do we show open edges?
					iOpenEdgeCount > 0 && //are there any open edges?
					i < iOpenEdgeCount && md->mOpenTVEdges[i]) //is this an open edge?
				{
					bShowOpenEdge = true; //this is an open edge
				}

				int veca = md->GetTVEdgeVec(i, 0); //-1 if none
				int vecb = md->GetTVEdgeVec(i, 1); //-1 if none

				// A
				Point2 tvPoint2_a;
				tvPoint2_a.x = md->GetTVVert(a)[index1];
				tvPoint2_a.y = md->GetTVVert(a)[index2];

				Point2 tvPoint2_veca(0.0,0.0);
				if(veca >= 0) //is there a vector handle index?
				{
					auto textureVertexA = md->GetTVVert(veca);
					tvPoint2_veca.x  = textureVertexA[index1];
					tvPoint2_veca.y  = textureVertexA[index2];
				}

				// B
				Point2 tvPoint2_b;
				tvPoint2_b.x  = md->GetTVVert(b)[index1];
				tvPoint2_b.y  = md->GetTVVert(b)[index2];

				Point2 tvPoint2_vecb(0.0,0.0);
				if(vecb >= 0) //is there a vector handle index?
				{
					auto textureVertexB = md->GetTVVert(veca);
					tvPoint2_vecb.x  = textureVertexB[index1];
					tvPoint2_vecb.y  = textureVertexB[index2];
				}

				//If peel mode is on, determine if this edge is related to a peelmode face
				bool aPeelModeFace = false;
				if(bLivePeel)
				{
					//Go through all the faces that this edge is connected to and try to find one that is
					//'in' peel mode
					for (int m = 0; m < md->GetTVEdgeNumberTVFaces(i); m++)
					{
						//try to find a face that is in peel mode
						int polyIndex = md->GetTVEdgeConnectedTVFace(i, m);
						if(peelFaces[polyIndex])
						{
							aPeelModeFace = true;
							break;
						}
					}
				}

				if (aPeelModeFace) //if in peel mode and this edge is marked
				{
					//draw peel mode edges
					DrawEdge(mUVRegularEdges, veca, vecb, tvPoint2_a, tvPoint2_b, tvPoint2_veca, tvPoint2_vecb, peelColorP4);
				}
				else if ((unwrapMod->mTVSubObjectMode == TVEDGEMODE) && ((md->GetTVEdgeSelected(i) && !bLivePeel) || md->GetTVEdgeSelectedPreview(i)))
				{
					//draw selected edges
					bool bPaintPreviewEdges = md->GetTVEdgeSelectedPreview(i) && !md->GetTVEdgeSelected(i);
					DrawEdge(mUVSelectedEdges,veca,vecb, tvPoint2_a, tvPoint2_b, tvPoint2_veca, tvPoint2_vecb, bPaintPreviewEdges ? selPreviewColorP4 : selColorP4);
				}
				else if (sharedEdge && (!(md->GetTVVertSelected(a) && md->GetTVVertSelected(b))))
				{
					//draw shared edges
					DrawEdge(mUVRegularEdges,veca,vecb, tvPoint2_a, tvPoint2_b, tvPoint2_veca, tvPoint2_vecb,sharedColorP4);
				}
				else if (bShowOpenEdge)
				{
					//draw open edges
					DrawEdge(mUVRegularEdges,veca,vecb, tvPoint2_a, tvPoint2_b, tvPoint2_veca, tvPoint2_vecb,openEdgeColorP4);
				}
				else if ((md->GetTVVertFrozen(a)) || (md->GetTVVertFrozen(b)))
				{
					//draw frozen edges
					DrawEdge(mUVRegularEdges,veca,vecb, tvPoint2_a, tvPoint2_b, tvPoint2_veca, tvPoint2_vecb,frozenColorP4);
				}
				else
				{
					if (!unwrapMod->showEdgeDistortion )
					{
						//draw normal edge
						DrawEdge(mUVRegularEdges,veca,vecb, tvPoint2_a, tvPoint2_b, tvPoint2_veca, tvPoint2_vecb,unselColorP4);
					}
				}

				//draw handles			
				if ((veca!= -1) && (vecb!= -1))
				{
					DrawEdge(mUVRegularEdges,-1,-1, tvPoint2_b, tvPoint2_vecb, tvPoint2_b, tvPoint2_vecb,handleColorP4);
					DrawEdge(mUVRegularEdges,-1,-1, tvPoint2_a, tvPoint2_veca, tvPoint2_a, tvPoint2_veca,handleColorP4);
				}
			}
		}

		//draw our pinned as Xs
		for (int i=0; i< md->GetNumberTVVerts(); i++) 
		{
			if (md->IsTVVertVisible(i) && md->IsTVVertPinned(i))
			{
				float x  = md->GetTVVert(i)[index1];
				float y  = md->GetTVVert(i)[index2];
				PaintX(mUVLines,x,y,2*SideDelta,sharedColorP4);
			}
		}	
	}
}

void Painter2D::PaintFaces()
{
	Point4 selColorP4((float)GetRValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->selColor)/255.0f,
		1.0f);
	Point4 edgeThinColorP4((float)GetRValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->selColor)/255.0f,
		1.0f);

	//now do selected faces

	Point3 selColorP3;
	selColorP3.x = (int) GetRValue(GetUnwrapModPtr()->selColor);
	selColorP3.y = (int) GetGValue(GetUnwrapModPtr()->selColor);
	selColorP3.z = (int) GetBValue(GetUnwrapModPtr()->selColor);

	selColorP3.x += (255.0f - selColorP3.x) *0.25f;
	selColorP3.y += (255.0f - selColorP3.y) *0.25f;
	selColorP3.z += (255.0f - selColorP3.z) *0.25f;

	int size = 0;
	int iFacesCount = 0;
	int iFaceVerticesCount = 0;
	for (int whichLD = 0; whichLD < GetUnwrapModPtr()->mMeshTopoData.Count(); whichLD++)
	{
		MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[whichLD];
		if(ld)
		{
			iFacesCount = iFacesCount + ld->GetNumberFaces();
			for (int i=0; i< ld->GetNumberFaces(); i++) 
			{
				iFaceVerticesCount = iFaceVerticesCount + ld->GetFaceDegree(i)*6;

				if (ld->GetFaceDegree(i) > size)
					size = ld->GetFaceDegree(i);
			}
		}		
	}

	Point2 *ipt = new Point2[size];

	if(iFaceVerticesCount > miFaceVertexesCountMax)
	{
		miFaceVertexesCountMax = iFaceVerticesCount;
		if(mpFaceVertexes)
		{
			delete[] mpFaceVertexes;
			mpFaceVertexes = NULL;
		}
		mpFaceVertexes = new Point3[miFaceVertexesCountMax];
	}

	if(iFacesCount > miFacesCountMax)
	{
		miFacesCountMax = iFacesCount;
		if(mpFacesDistribution)
		{
			delete[] mpFacesDistribution;
			mpFacesDistribution = NULL;
		}
		mpFacesDistribution = new int[miFacesCountMax];
	}

	int index1 = 0;
	int index2 = 1;
	GetUnwrapModPtr()->GetUVWIndices(index1,index2);
	for (int whichLD = 0; whichLD < GetUnwrapModPtr()->mMeshTopoData.Count(); whichLD++)
	{
		MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[whichLD];
		if (ld)
		{
			for (int i=0; i< ld->GetNumberFaces(); i++) 
			{
				// Grap the three points, xformed
				BOOL hidden = FALSE;

				bool bDraw = false;
				BOOL faceSelected = ld->GetFaceSelected(i);
				BOOL faceSelectedPreview = ld->GetFaceSelectedPreview(i);
				bool bPaintPreviewFace = faceSelectedPreview && !faceSelected;
				bDraw = ld->IsFaceVisible(i) && (GetUnwrapModPtr()->mTVSubObjectMode == TVFACEMODE) && (faceSelected || faceSelectedPreview) ;

				if (bDraw)
				{
					int pcount = 3;
					pcount = ld->GetFaceDegree(i);
					if(pcount == 0 || ld->GetFaceDead(i))
					{
						continue;
					}
					//if it is patch with curve mapping
					if ( ld->GetFaceCurvedMaping(i) &&
						ld->GetFaceHasVectors(i) )
					{
						Spline3D spl;
						spl.NewSpline();
						BOOL pVis[4];
						for (int j=0; j<pcount; j++) 
						{
							Point3 in, p, out;
							Point2 iin, ip, iout;
							int index = ld->GetFaceTVVert(i,j);

							ip.x = ld->GetTVVert(index)[index1];
							ip.y = ld->GetTVVert(index)[index2];

							pVis[j] = ld->IsTVVertVisible(index);

							index = ld->GetFaceTVHandle(i,j*2);
							iout.x = ld->GetTVVert(index)[index1];
							iout.y = ld->GetTVVert(index)[index2];

							if (j==0)
								index = ld->GetFaceTVHandle(i,pcount*2-1);
							else index = ld->GetFaceTVHandle(i,j*2-1);

							iin.x = ld->GetTVVert(index)[index1];
							iin.y = ld->GetTVVert(index)[index2];

							in.x = (float)iin.x;
							in.y = (float)iin.y;
							in.z = 0.0f;

							out.x = (float)iout.x;
							out.y = (float)iout.y;
							out.z = 0.0f;

							p.x = (float)ip.x;
							p.y = (float)ip.y;
							p.z = 0.0f;


							SplineKnot kn(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, in, out);
							spl.AddKnot(kn);
						}
						spl.SetClosed();
						spl.ComputeBezPoints();
						//draw curves
						Point2 lp;
						int polyct = 0;
						for (int j=0; j<pcount; j++) 
						{
							int jNext = j+1;
							if (jNext >= pcount) jNext = 0;
							if (pVis[j] && pVis[jNext])
							{
								Point3 p;
								Point2 ip;
								int index = ld->GetFaceTVVert(i,j);

								ip.x = ld->GetTVVert(index)[index1];
								ip.y = ld->GetTVVert(index)[index2];

								if(j==0)
								{
									polyct++;

									mpFaceVertexes[miFaceVertexesCount].x = ip.x;
									mpFaceVertexes[miFaceVertexesCount].y = ip.y;
									mpFaceVertexes[miFaceVertexesCount].z = 0.0;
									miFaceVertexesCount++;
								}

								for (int iu = 1; iu < 5; iu++)
								{
									float u = (float) iu/5.f;
									p = spl.InterpBezier3D(j, u);

									polyct++;

									mpFaceVertexes[miFaceVertexesCount].x = p.x;
									mpFaceVertexes[miFaceVertexesCount].y = p.y;
									mpFaceVertexes[miFaceVertexesCount].z = 0.0;
									miFaceVertexesCount++;
								}

								if (j == pcount-1)
									index = ld->GetFaceTVVert(i,0);
								else index = ld->GetFaceTVVert(i,j+1);

								ip.x = ld->GetTVVert(index)[index1];
								ip.y = ld->GetTVVert(index)[index2];

								polyct++;

								mpFaceVertexes[miFaceVertexesCount].x = ip.x;
								mpFaceVertexes[miFaceVertexesCount].y = ip.y;
								mpFaceVertexes[miFaceVertexesCount].z = 0.0;
								miFaceVertexesCount++;
							}
						}

						if(polyct > 0)
						{
							DrawOutlineOfCurrentFace(polyct, bPaintPreviewFace);
							mpFacesDistribution[miFacesCount] = polyct;
							miFacesCount++;								
						}							
					}
					else  //else it is just regular poly so just draw the straight edges
					{
						for (int j=0; j<pcount; j++) 
						{
							int index = ld->GetFaceTVVert(i,j);
							ipt[j].x = ld->GetTVVert(index)[index1];
							ipt[j].y = ld->GetTVVert(index)[index2];

							if (ld->GetTVVertHidden(index)) hidden = TRUE;
						}
						// Now draw the face
						if (!hidden)
						{
							if(faceSelected)
							{
								mpFaceVertexes[miFaceVertexesCount].x = ipt[0].x;
								mpFaceVertexes[miFaceVertexesCount].y = ipt[0].y;
								mpFaceVertexes[miFaceVertexesCount].z = 0.0;
								miFaceVertexesCount++;

								for (int j=0; j<pcount; j++) 
								{
									if (j != (pcount-1))
									{

										mpFaceVertexes[miFaceVertexesCount].x = ipt[j+1].x;
										mpFaceVertexes[miFaceVertexesCount].y = ipt[j+1].y;
										mpFaceVertexes[miFaceVertexesCount].z = 0.0;
										miFaceVertexesCount++;
									}
								}

								if(pcount > 0)
								{
									DrawOutlineOfCurrentFace(pcount, bPaintPreviewFace);
									mpFacesDistribution[miFacesCount] = pcount;
									miFacesCount++;
								}
							}

							if(faceSelectedPreview)
							{
								if (pcount >= 3)
								{
									Point2 firstPoint(ipt[0].x, ipt[0].y);
									int iTriangleCount = pcount - 2;
									for (int j = 0; j < iTriangleCount; j++)
									{
										Point2 secondPoint(ipt[j+1].x, ipt[j+1].y);
										Point2 thirdPoint(ipt[j+2].x, ipt[j+2].y);
										DrawFilledTriangle(mGizmoTransparentPolygons,
											firstPoint,
											secondPoint,
											thirdPoint,
											FacePreviewRedWithAlpha);
									}
								}
							}

						}
					}
				}

			}	
		}
	}
	delete [] ipt;
}

void Painter2D::PaintFreeFormGizmo()
{
	//need in rotation in rotation drag

	if (GetUnwrapModPtr()->mInRelax)
		return;
	BOOL inFreeFormMode = FALSE;
	if (GetUnwrapModPtr()->mode == ID_FREEFORMMODE)
		inFreeFormMode = TRUE;

	if ( (inFreeFormMode) ||
		(GetUnwrapModPtr()->mode == ID_MOVE) ||
		(GetUnwrapModPtr()->mode == ID_ROTATE) ||
		(GetUnwrapModPtr()->mode == ID_SCALE) )
	{
		float xzoom = 1.0;
		float yzoom = 1.0;
		int	width = 1;
		int height = 1;
		GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

		int i1, i2;
		GetUnwrapModPtr()->GetUVWIndices(i1,i2);

		Point4 freeFormColorP4((float)GetRValue(GetUnwrapModPtr()->freeFormColor)/255.0f,
			(float)GetGValue(GetUnwrapModPtr()->freeFormColor)/255.0f,
			(float)GetBValue(GetUnwrapModPtr()->freeFormColor)/255.0f,
			1.0f);

		GetUnwrapModPtr()->TransferSelectionStart();

		int vselNumberSet = 0;
		GetUnwrapModPtr()->freeFormBounds.Init();
		for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[ldID];
			if (ld == NULL)
			{
				DbgAssert(0);
			}
			else
			{
				int vselCount = ld->GetNumberTVVerts();

				if (!GetUnwrapModPtr()->inRotation)
					GetUnwrapModPtr()->selCenter = Point3(0.0f,0.0f,0.0f);
				for (int i = 0; i < vselCount; i++)
				{
					if ( ld->GetTVVertSelected(i) )
					{
						//get bounds
						Point3 p(0.0f,0.0f,0.0f);
						p[i1] = ld->GetTVVert(i)[i1];
						p[i2] = ld->GetTVVert(i)[i2];
						GetUnwrapModPtr()->freeFormBounds += p;
						vselNumberSet++;
					}
				}
			}
		}

		Point3 tempCenter;
		if (!GetUnwrapModPtr()->inRotation)
			GetUnwrapModPtr()->selCenter = GetUnwrapModPtr()->freeFormBounds.Center();			
		else tempCenter = GetUnwrapModPtr()->freeFormBounds.Center();

		//Upate the pivot in screen space under free form
		if (vselNumberSet > 0)
		{
			Point2 pivotPoint = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->selCenter,xzoom,yzoom,width,height);
			Point2 aPivot = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->freeFormPivotOffset,xzoom,yzoom,width,height);
			Point2 bPivot = GetUnwrapModPtr()->UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);
			GetUnwrapModPtr()->freeFormPivotScreenSpace = pivotPoint +(aPivot - bPivot);
		}

		if (vselNumberSet > 0)
		{	
			//draw center
			Point2 prect[4];
			Point2 prectNoExpand[4];
			Point2 pivotPoint;

			prect[0].x = GetUnwrapModPtr()->selCenter[i1];
			prect[0].y = GetUnwrapModPtr()->selCenter[i2];

			pivotPoint = prect[0];
			prect[1] = prect[0];

			float fYDelta = SideDelta/GetUnwrapModPtr()->zoom;
			float fXDelta = fYDelta/GetUnwrapModPtr()->aspect;

			//draw gizmo bounds
			prect[0].x = GetUnwrapModPtr()->freeFormBounds.pmin[i1];
			prect[0].y = GetUnwrapModPtr()->freeFormBounds.pmin[i2];

			prect[1].x = GetUnwrapModPtr()->freeFormBounds.pmax[i1];
			prect[1].y = GetUnwrapModPtr()->freeFormBounds.pmax[i2];

			//If this value is too small, then the boundary box of one vertex 
			//will become too small and cause moving the vertex difficult.
			const float EXPAND_BOUNDING_BOX_COEFFICIENT = 10.0f;
			float xexpand = EXPAND_BOUNDING_BOX_COEFFICIENT / xzoom;
			float yexpand = EXPAND_BOUNDING_BOX_COEFFICIENT / yzoom;

			BOOL expandedFFX = FALSE;
			BOOL expandedFFY = FALSE;

			prectNoExpand[0] = prect[0];
			prectNoExpand[1] = prect[1];
			prectNoExpand[2] = prect[2];
			prectNoExpand[3] = prect[3];

			if (!GetUnwrapModPtr()->freeFormMode->dragging)
			{
				const float UV_DIFFERENCE_BETWEEN_TWO = 0.2f;
				//when u value difference is less than UV_DIFFERENCE_BETWEEN_TWO,then expand
				if((prect[1].x - prect[0].x) < UV_DIFFERENCE_BETWEEN_TWO)
				{
					prect[0].x -= xexpand;
					prect[1].x += xexpand;
					//expand bounds
					GetUnwrapModPtr()->freeFormBounds.pmin[i1] -= xexpand;
					GetUnwrapModPtr()->freeFormBounds.pmax[i1] += xexpand;					
					expandedFFX = TRUE;
				}

				//when v value difference is less than UV_DIFFERENCE_BETWEEN_TWO,then expand
				if((prect[1].y - prect[0].y) < UV_DIFFERENCE_BETWEEN_TWO)
				{
					prect[0].y -= yexpand;
					prect[1].y += yexpand;
					//expand bounds
					GetUnwrapModPtr()->freeFormBounds.pmin[i2] -= yexpand;
					GetUnwrapModPtr()->freeFormBounds.pmax[i2] += yexpand;					
					expandedFFY = TRUE;
				}

			}

			Point2 aPivot(GetUnwrapModPtr()->freeFormPivotOffset[i1],GetUnwrapModPtr()->freeFormPivotOffset[i2]);
			Point2 bPivot(0.0,0.0);
			pivotPoint = pivotPoint + ( aPivot-bPivot);

			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode)
			{
				//Draw the rectangle for indicating the outline of the selection
				DrawRectangle(mGizmoLines,prect[0].x,prect[0].y,prect[1].x,prect[1].y,freeFormColorP4);

				//Set the yellow color for indicating movement along X\Y or both
				Point4 yellowColor(1.0,1.0,0.0,1.0);

				//Draw the arrow that is composed of one line and the sign '>'.
				float fExtensionY = GetUnwrapModPtr()->freeFormMoveAxisLength / yzoom;
				float fExtensionX = GetUnwrapModPtr()->freeFormMoveAxisLength / xzoom;
				//This edge will be used for moving along X axis
				Point4 axisXColor = freeFormColorP4;
				if(GetUnwrapModPtr()->freeFormSubMode == ID_MOVE && 
					(GetUnwrapModPtr()->freeFormMoveAxisForDisplay == eMoveX || GetUnwrapModPtr()->freeFormMoveAxisForDisplay == eMoveBoth))
				{
					axisXColor = yellowColor;
				}

				DrawSingleEdge(mGizmoLines,pivotPoint.x,pivotPoint.y,pivotPoint.x+fExtensionX,pivotPoint.y,axisXColor);

				float fDeltaX = fExtensionX*0.1f;
				float fHalfDeltaX = fDeltaX*0.5;

				float fDeltaY = fExtensionY*0.1f;
				float fHalfDeltaY = fDeltaY*0.5;

				//The x axis sign '>' color is red
				Point4 redColor(1.0,0.0,0.0,1.0);
				DrawFilledTriangle(mGizmoPolygons,
					Point2(pivotPoint.x+fExtensionX-fDeltaX,pivotPoint.y-fHalfDeltaY),
					Point2(pivotPoint.x+fExtensionX-fDeltaX,pivotPoint.y+fHalfDeltaY),
					Point2(pivotPoint.x+fExtensionX,pivotPoint.y),
					redColor);

				//This edge will be used for moving along Y axis
				Point4 axisYColor = freeFormColorP4;
				if((GetUnwrapModPtr()->freeFormSubMode == ID_MOVE || GetUnwrapModPtr()->freeFormSubMode == ID_SELECT ) &&
					(GetUnwrapModPtr()->freeFormMoveAxisForDisplay == eMoveY || GetUnwrapModPtr()->freeFormMoveAxisForDisplay == eMoveBoth))
				{
					axisYColor = yellowColor;
				}
				DrawSingleEdge(mGizmoLines,pivotPoint.x,pivotPoint.y,pivotPoint.x,pivotPoint.y+fExtensionY,axisYColor);
				//The y axis sign '>' color is green
				Point4 greenColor(0.0,1.0,0.0,1.0);
				DrawFilledTriangle(mGizmoPolygons,
					Point2(pivotPoint.x-fHalfDeltaX,pivotPoint.y+fExtensionY-fDeltaY),
					Point2(pivotPoint.x+fHalfDeltaX,pivotPoint.y+fExtensionY-fDeltaY),
					Point2(pivotPoint.x,pivotPoint.y+fExtensionY),
					greenColor);

				//These 2 edges and the above 2 edges will be one rectangle.
				Point4 axisBothColor = freeFormColorP4;
				if((GetUnwrapModPtr()->freeFormSubMode == ID_MOVE || GetUnwrapModPtr()->freeFormSubMode == ID_SELECT) &&
					GetUnwrapModPtr()->freeFormMoveAxisForDisplay == eMoveBoth)
				{
					axisBothColor = yellowColor;
				}
				float fQUarterExtensionX = fExtensionX*FREEMODEMOVERECTANGLE;
				float fQUarterExtensionY = fExtensionY*FREEMODEMOVERECTANGLE;
				DrawSingleEdge(mGizmoLines,
					pivotPoint.x+fQUarterExtensionX,
					pivotPoint.y,
					pivotPoint.x+fQUarterExtensionX,
					pivotPoint.y+fQUarterExtensionY,
					axisBothColor);

				DrawSingleEdge(mGizmoLines,
					pivotPoint.x,
					pivotPoint.y+fQUarterExtensionY,
					pivotPoint.x+fQUarterExtensionX,
					pivotPoint.y+fQUarterExtensionY,
					axisBothColor);

				//one transparent rectangle that is composed of 2 triangles.
				if((GetUnwrapModPtr()->freeFormSubMode == ID_MOVE || GetUnwrapModPtr()->freeFormSubMode == ID_SELECT) &&
					GetUnwrapModPtr()->freeFormMoveAxisForDisplay == eMoveBoth)
				{
					Point4 yellowTransparentColor(1.0,1.0,0.0,0.5);
					DrawFilledTriangle(mGizmoTransparentPolygons,
						Point2(pivotPoint.x,pivotPoint.y),
						Point2(pivotPoint.x,pivotPoint.y+fQUarterExtensionY),
						Point2(pivotPoint.x+fQUarterExtensionX,pivotPoint.y+fQUarterExtensionY),
						yellowTransparentColor);

					DrawFilledTriangle(mGizmoTransparentPolygons,
						Point2(pivotPoint.x,pivotPoint.y),
						Point2(pivotPoint.x+fQUarterExtensionX,pivotPoint.y+fQUarterExtensionY),
						Point2(pivotPoint.x+fQUarterExtensionX,pivotPoint.y),
						yellowTransparentColor);
				}				
			}

			//draw hit boxes
			Point3 frect[4];
			Point2 pmin, pmax;

			pmin = prect[0];
			pmax = prect[1];

			Point2 pminNoExpand, pmaxNoExpand;
			pminNoExpand = prectNoExpand[0];
			pmaxNoExpand = prectNoExpand[1];

			int corners[4] = {0};
			if ((i1 == 0) && (i2 == 1))
			{
				corners[0] = 0;
				corners[1] = 1;
				corners[2] = 2;
				corners[3] = 3;
			}
			else if ((i1 == 1) && (i2 == 2)) 
			{
				corners[0] = 1;//1,2,5,6
				corners[1] = 3;
				corners[2] = 5;
				corners[3] = 7;
			}
			else if ((i1 == 0) && (i2 == 2))
			{
				corners[0] = 0;
				corners[1] = 1;
				corners[2] = 4;
				corners[3] = 5;
			}

			for (int i = 0; i < 4; i++)
			{
				int index = corners[i];
				GetUnwrapModPtr()->freeFormCorners[i] = GetUnwrapModPtr()->freeFormBounds[index];
				if (i==0)
					prect[i] = pmin;
				else if (i==1)
				{
					prect[i].x = pmin.x;
					prect[i].y = pmax.y;
				}
				else if (i==2)
					prect[i] = pmax;
				else if (i==3)
				{
					prect[i].x = pmax.x;
					prect[i].y = pmin.y;
				}

				if (i==0)
					prectNoExpand[i] = pminNoExpand;
				else if (i==1)
				{
					prectNoExpand[i].x = pminNoExpand.x;
					prectNoExpand[i].y = pmaxNoExpand.y;
				}
				else if (i==2)
					prectNoExpand[i] = pmaxNoExpand;
				else if (i==3)
				{
					prectNoExpand[i].x = pmaxNoExpand.x;
					prectNoExpand[i].y = pminNoExpand.y;
				}

				GetUnwrapModPtr()->freeFormCornersScreenSpace[i] = Point2(prectNoExpand[i].x*xzoom+GetUnwrapModPtr()->xscroll+(width-int(xzoom))/2, 
					(float(height)-prectNoExpand[i].y*yzoom)+GetUnwrapModPtr()->yscroll-(height-int(yzoom))/2);
				if (!GetUnwrapModPtr()->inRotation && inFreeFormMode) 
				{
					DrawRectangle(mGizmoLines,prect[i].x-fXDelta,prect[i].y-fYDelta,prect[i].x+fXDelta,prect[i].y+fYDelta,freeFormColorP4);
				}
			}
			Point2 centerEdge;
			centerEdge = (prect[0] + prect[1]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[0] = (GetUnwrapModPtr()->freeFormCorners[0] + GetUnwrapModPtr()->freeFormCorners[1]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[0] = Point2(centerEdge.x*xzoom+GetUnwrapModPtr()->xscroll+(width-int(xzoom))/2, 
				(float(height)-centerEdge.y*yzoom)+GetUnwrapModPtr()->yscroll-(height-int(yzoom))/2);
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode)
			{
				DrawRectangle(mGizmoLines,centerEdge.x-fXDelta,centerEdge.y-fYDelta,centerEdge.x+fXDelta,centerEdge.y+fYDelta,freeFormColorP4);
			}

			centerEdge = (prect[1] + prect[2]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[2] = (GetUnwrapModPtr()->freeFormCorners[2] + GetUnwrapModPtr()->freeFormCorners[3]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[2] = Point2(centerEdge.x*xzoom+GetUnwrapModPtr()->xscroll+(width-int(xzoom))/2, 
				(float(height)-centerEdge.y*yzoom)+GetUnwrapModPtr()->yscroll-(height-int(yzoom))/2);
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode)
			{
				DrawRectangle(mGizmoLines,centerEdge.x-fXDelta,centerEdge.y-fYDelta,centerEdge.x+fXDelta,centerEdge.y+fYDelta,freeFormColorP4);
			}

			centerEdge = (prect[2] + prect[3]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[3] = (GetUnwrapModPtr()->freeFormCorners[0] + GetUnwrapModPtr()->freeFormCorners[2]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[3] = Point2(centerEdge.x*xzoom+GetUnwrapModPtr()->xscroll+(width-int(xzoom))/2, 
				(float(height)-centerEdge.y*yzoom)+GetUnwrapModPtr()->yscroll-(height-int(yzoom))/2);
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode)
			{
				DrawRectangle(mGizmoLines,centerEdge.x-fXDelta,centerEdge.y-fYDelta,centerEdge.x+fXDelta,centerEdge.y+fYDelta,freeFormColorP4);
			}

			centerEdge = (prect[3] + prect[0]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[1] = (GetUnwrapModPtr()->freeFormCorners[1] + GetUnwrapModPtr()->freeFormCorners[3]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[1] = Point2(centerEdge.x*xzoom+GetUnwrapModPtr()->xscroll+(width-int(xzoom))/2, 
				(float(height)-centerEdge.y*yzoom)+GetUnwrapModPtr()->yscroll-(height-int(yzoom))/2);
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode)
			{
				DrawRectangle(mGizmoLines,centerEdge.x-fXDelta,centerEdge.y-fYDelta,centerEdge.x+fXDelta,centerEdge.y+fYDelta,freeFormColorP4);
			}

			//draw pivot (the shape is + )
			Point4 pivotColor = freeFormColorP4;
			if(GetUnwrapModPtr()->freeFormSubMode == ID_MOVEPIVOT)
			{
				pivotColor.x = 1.0;
				pivotColor.y = 1.0;
				pivotColor.z = 0.0;
				pivotColor.w = 1.0;
			}
			float fPivotLeft = pivotPoint.x-fXDelta*2;
			float fPivotRight = pivotPoint.x+fXDelta*2;
			float fPivotTop = pivotPoint.y+fYDelta*2;
			float fPivotBottom = pivotPoint.y-fYDelta*2;
			DrawSingleEdge(
				mGizmoLines,
				fPivotLeft,
				pivotPoint.y,
				fPivotRight,
				pivotPoint.y,
				pivotColor);

			DrawSingleEdge(
				mGizmoLines,
				pivotPoint.x,
				fPivotBottom,
				pivotPoint.x,
				fPivotTop,
				pivotColor);
			//draw a box surrounding the pivot
			if (!GetUnwrapModPtr()->inRotation)
			{
				DrawRectangle(mGizmoLines,
					fPivotLeft,
					fPivotTop,
					fPivotRight,
					fPivotBottom,
					pivotColor);
			}

			if (GetUnwrapModPtr()->inRotation)
			{
				Point2 a(tempCenter[i1],tempCenter[i2]);
				Point2 b(GetUnwrapModPtr()->origSelCenter[i1],GetUnwrapModPtr()->origSelCenter[i2]);
				DrawSingleEdge(
					mGizmoLines,
					a.x,
					a.y,
					pivotPoint.x,
					pivotPoint.y,
					freeFormColorP4);

				DrawSingleEdge(
					mGizmoLines,
					pivotPoint.x,
					pivotPoint.y,
					b.x,
					b.y,
					freeFormColorP4);

				TSTR rotAngleStr;
				rotAngleStr.printf(_T("%3.2f"),GetUnwrapModPtr()->currentRotationAngle);
				mTextContainer.mPositions.push_back(pivotPoint);
				mTextContainer.mTexts.push_back(rotAngleStr);
			}

		}
		GetUnwrapModPtr()->TransferSelectionEnd(FALSE,FALSE);
	}
}

void Painter2D::PaintPelt()
{
	//PELT 
	//draw the mirror plane

	if (GetUnwrapModPtr()->peltData.peltDialog.hWnd && GetUnwrapModPtr()->peltData.mBaseMeshTopoDataCurrent)
	{
		COLORREF yellowColor = RGB(255,255,64);

		Point4 selThinColorP4((float)GetRValue(GetUnwrapModPtr()->selColor)/255.0f,
			(float)GetGValue(GetUnwrapModPtr()->selColor)/255.0f,
			(float)GetBValue(GetUnwrapModPtr()->selColor)/255.0f,
			1.0f);
		Point4 yellowColorP4((float)GetRValue(yellowColor)/255.0f,
			(float)GetGValue(yellowColor)/255.0f,
			(float)GetBValue(yellowColor)/255.0f,
			1.0f);
		Point4 sharedColorP4((float)GetRValue(GetUnwrapModPtr()->sharedColor)/255.0f,
			(float)GetGValue(GetUnwrapModPtr()->sharedColor)/255.0f,
			(float)GetBValue(GetUnwrapModPtr()->sharedColor)/255.0f,
			1.0f);

		MeshTopoData *ld = GetUnwrapModPtr()->peltData.mBaseMeshTopoDataCurrent;

		GetUnwrapModPtr()->peltData.mIsSpring.SetSize(ld->GetNumberTVEdges());
		GetUnwrapModPtr()->peltData.mIsSpring.ClearAll();

		GetUnwrapModPtr()->peltData.mPeltSpringLength.SetCount(ld->GetNumberTVEdges());

		Point3 mirrorVec = Point3(0.0f,1.0f,0.0f);
		Matrix3 tm(1);
		tm.RotateZ(GetUnwrapModPtr()->peltData.GetMirrorAngle());
		mirrorVec = mirrorVec  * tm;
		mirrorVec *= 2.0f;

		Matrix3 tma(1);
		tma.RotateZ(GetUnwrapModPtr()->peltData.GetMirrorAngle()+PI*0.5f);
		Point3 mirrorVeca = Point3(0.0f,1.0f,0.0f);
		mirrorVeca = mirrorVeca  * tma;
		mirrorVeca *= 2.0f;

		int index1 = 0;
		int index2 = 1;
		GetUnwrapModPtr()->GetUVWIndices(index1,index2);

		//get the center
		Point3 ma,mb;
		ma = GetUnwrapModPtr()->peltData.GetMirrorCenter() + mirrorVec;
		mb = GetUnwrapModPtr()->peltData.GetMirrorCenter() - mirrorVec;
		//get our vec
		Point2 pa(ma[index1],ma[index2]);
		Point2 pb(mb[index1],mb[index2]);

		DrawSingleEdge(
			mUVLines,
			pa.x,
			pa.y,
			pb.x,
			pb.y,
			yellowColorP4);

		ma = GetUnwrapModPtr()->peltData.GetMirrorCenter();
		mb = GetUnwrapModPtr()->peltData.GetMirrorCenter() - mirrorVeca;
		//get our vec
		pa.x = ma[index1];
		pa.y = ma[index2];

		pb.x = mb[index1];
		pb.y = mb[index2];

		DrawSingleEdge(
			mUVLines,
			pa.x,
			pa.y,
			pb.x,
			pb.y,
			yellowColorP4);

		//draw our springs
		COLORREF baseColor = ColorMan()->GetColor(EDGEDISTORTIONCOLORID);
		COLORREF goalColor = ColorMan()->GetColor(EDGEDISTORTIONGOALCOLORID);

		Color goalc(goalColor);
		Color basec(baseColor);

		for (int i = 0; i < GetUnwrapModPtr()->peltData.springEdges.Count(); i++)
		{

			if (GetUnwrapModPtr()->peltData.springEdges[i].isEdge)
			{
				int a,b;
				a = GetUnwrapModPtr()->peltData.springEdges[i].v1;
				b = GetUnwrapModPtr()->peltData.springEdges[i].v2;

				if ( ( a >= 0) && (a < ld->GetNumberTVVerts()/*TVMaps.v.Count()*/) && 
					( b >= 0) && (b < ld->GetNumberTVVerts()/*TVMaps.v.Count()*/) )
				{
					DrawSingleEdge(
						mUVLines,
						ld->GetTVVert(a)[index1],
						ld->GetTVVert(a)[index2],
						ld->GetTVVert(b)[index1],
						ld->GetTVVert(b)[index2],
						selThinColorP4);

					Point3 vec = ld->GetTVVert(b) - ld->GetTVVert(b);//(TVMaps.v[b].p-TVMaps.v[a].p);
					if (Length(vec) > 0.00001f)
					{
						vec = Normalize(vec)*GetUnwrapModPtr()->peltData.springEdges[i].dist*GetUnwrapModPtr()->peltData.springEdges[i].distPer;
						vec = ld->GetTVVert(b)/*TVMaps.v[b].p*/ - vec;

						Point2 tvPoint2;
						tvPoint2.x  = vec[index1];
						tvPoint2.y  = vec[index2];

						DrawSingleEdge(
							mUVLines,
							ld->GetTVVert(b)[index1],
							ld->GetTVVert(b)[index2],
							tvPoint2.x,
							tvPoint2.y,
							sharedColorP4);
					}
				}
			}
			else if (GetUnwrapModPtr()->showEdgeDistortion )
			{
				int edgeIndex = GetUnwrapModPtr()->peltData.springEdges[i].edgeIndex;
				if (edgeIndex != -1)
				{
					GetUnwrapModPtr()->peltData.mIsSpring.Set(edgeIndex);
					GetUnwrapModPtr()->peltData.mPeltSpringLength[edgeIndex] = GetUnwrapModPtr()->peltData.springEdges[i].dist;
				}
			}
		}

		for (int i = 0; i < GetUnwrapModPtr()->peltData.rigPoints.Count(); i++)
		{
			int a = GetUnwrapModPtr()->peltData.rigPoints[i].lookupIndex;
			int b = GetUnwrapModPtr()->peltData.rigPoints[0].lookupIndex;
			if ((i+1) <  GetUnwrapModPtr()->peltData.rigPoints.Count())
				b = GetUnwrapModPtr()->peltData.rigPoints[i+1].lookupIndex;

			DrawSingleEdge(
				mUVLines,
				ld->GetTVVert(a)[index1],
				ld->GetTVVert(a)[index2],
				ld->GetTVVert(b)[index1],
				ld->GetTVVert(b)[index2],
				yellowColorP4);
		}
	}
}

void UnwrapMod::SetDistortionType(DistortionOptions eType)
{
	mDistortionOption = eType;
}

void Painter2D::ResetDistortionElement()
{
	//If find the size of 3d distortion elements is less than the mesh topo data count,
	//push the extra element into the mDistortion3DElements.
	int iMeshTopoDataCount = GetUnwrapModPtr()->mMeshTopoData.Count();
	if(mDistortion3DElements.size() < iMeshTopoDataCount)
	{
		int iCount = iMeshTopoDataCount - (int)mDistortion3DElements.size();
		for (int i=0;i<iCount;++i)
		{
			DrawingElement distortion3DElement;
			mDistortion3DElements.push_back(distortion3DElement);
		}
	}
	//Reset every element in the mDistortion3DElements for refreshing the new distortion data.
	for (int i=0;i<mDistortion3DElements.size();++i)
	{
		mDistortion3DElements[i].Reset();
	}
	mDistortion2DElement.Reset();
}

extern float AreaOfPolygon(Tab<Point3> &points);
void Painter2D::ComputeClusterFaceAreaRatio(MeshTopoData *md,std::vector<float>& everyFaceClusterAreaRatio)
{
	UnwrapMod* pUnwrapMod = Painter2D::GetUnwrapModPtr();
	if (NULL == pUnwrapMod ||
		NULL == md)
	{
		return;
	}

	int in1 = 0;
	int in2 = 1;
	pUnwrapMod->GetUVWIndices(in1, in2);

	pUnwrapMod->FreeClusterList();

	BitArray TVsel = md->GetTVVertSel();
	BitArray GeomVsel = md->GetGeomVertSel();
	BOOL bClusterSuccess = pUnwrapMod->BuildClusterFromTVVertexElement(md);
	if (bClusterSuccess)
	{
		int iClusterCount = pUnwrapMod->clusterList.Count();
		for (int i = 0; i < iClusterCount; ++i)
		{
			float geometryAreaThisCluster = 0.0;
			float uvAreaThisCluster = 0.0;
			if (pUnwrapMod->clusterList[i]->ld == md)
			{
				int iThisClusterFaceCount = pUnwrapMod->clusterList[i]->faces.Count();
				for (int j = 0; j < iThisClusterFaceCount; ++j)
				{
					int findex = pUnwrapMod->clusterList[i]->faces[j];
					int degree = md->GetFaceDegree(findex);
					Tab<Point3> plistGeom;
					plistGeom.SetCount(degree);
					Tab<Point3> plistUVW;
					plistUVW.SetCount(degree);
					for (int ithVertex = 0; ithVertex < degree; ++ithVertex)
					{
						int index = md->GetFaceGeomVert(findex, ithVertex);
						plistGeom[ithVertex] = md->GetGeomVert(index);

						index = md->GetFaceTVVert(findex, ithVertex);
						plistUVW[ithVertex] = Point3(md->GetTVVert(index)[in1], md->GetTVVert(index)[in2], 0.0f);
					}

					geometryAreaThisCluster += AreaOfPolygon(plistGeom);
					uvAreaThisCluster += AreaOfPolygon(plistUVW);
				}

				for (int j = 0; j < iThisClusterFaceCount; ++j)
				{
					int findex = pUnwrapMod->clusterList[i]->faces[j];
					if (uvAreaThisCluster != 0.0)
					{
						everyFaceClusterAreaRatio[findex] = geometryAreaThisCluster / uvAreaThisCluster;
					}

				}
			}
		}
	}
	//recover to its vertex original selection because the function BuildClusterFromTVVertexElement may change the vertex selection.			
	md->SetTVVertSel(TVsel);
	md->SetGeomVertSel(GeomVsel);

	pUnwrapMod->FreeClusterList();
}

void Painter2D::MakeDistortion()
{
	int in1 = 0;
	int in2 = 1;

	UnwrapMod* pUnwrapMod = Painter2D::GetUnwrapModPtr();
	pUnwrapMod->GetUVWIndices(in1,in2);

	DistortionOptions distortion = pUnwrapMod->GetDistortionType();

	for (int ldID = 0; ldID < pUnwrapMod->mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *md = pUnwrapMod->mMeshTopoData[ldID];
		if(nullptr == md )
		{
			continue;
		}
				
		std::vector<float> everyFaceAreaRatio;
		everyFaceAreaRatio.resize(md->GetNumberFaces());
		Tab<LSCMFace>	facesData;
		for (int i = 0; i < md->GetNumberFaces(); ++i)
		{
			if (md->CheckPoly(i))
			{
				md->AddLSCMFaceData(i, facesData, false);
			}

			everyFaceAreaRatio[i] = 1.0;
		}

		if (distortion == eAreaDistortion)
		{
			ComputeClusterFaceAreaRatio(md, everyFaceAreaRatio);
		}

		int iFacesCount = facesData.Count();
		for (int j = 0; j < iFacesCount; j++)
		{
			LSCMFace* face = facesData.Addr(j);
			if(nullptr == face)
			{
				continue;
			}

			int iPolyIndex = face->mPolyIndex;
			//geometry data
			int geoIndexA = md->GetFaceGeomVert(iPolyIndex,face->mEdge[0].mIthEdge);
			int geoIndexB = md->GetFaceGeomVert(iPolyIndex,face->mEdge[1].mIthEdge);
			int geoIndexC = md->GetFaceGeomVert(iPolyIndex,face->mEdge[2].mIthEdge);

			Point3 gePosA = md->GetGeomVert(geoIndexA);
			Point3 gePosB = md->GetGeomVert(geoIndexB);
			Point3 gePosC = md->GetGeomVert(geoIndexC);

			//UVW data
			int uvIndexA = md->GetFaceTVVert(iPolyIndex,face->mEdge[0].mIthEdge);
			int uvIndexB = md->GetFaceTVVert(iPolyIndex,face->mEdge[1].mIthEdge);
			int uvIndexC = md->GetFaceTVVert(iPolyIndex,face->mEdge[2].mIthEdge);

			Point3 uvPA = md->GetTVVert(uvIndexA);
			Point3 uvPB = md->GetTVVert(uvIndexB);
			Point3 uvPC = md->GetTVVert(uvIndexC);

			//For displaying the triangle in the 2d view
			Point3 uvPosA = Point3(uvPA[in1],uvPA[in2],0.0f);
			Point3 uvPosB = Point3(uvPB[in1],uvPB[in2],0.0f);
			Point3 uvPosC = Point3(uvPC[in1],uvPC[in2],0.0f);

			//Read to get the color by computing the angle or area delta between the UVW and geometry space
			bool bAngleDeltaA = true;
			bool bAngleDeltaB = true;
			bool bAngleDeltaC = true;
			float fAngleDeltaA = 0.0;
			float fAngleDeltaB = 0.0;
			float fAngleDeltaC = 0.0;

			int degree = md->GetFaceDegree(iPolyIndex);

			Tab<Point3> plistGeo;
			plistGeo.SetCount(degree);
			Tab<Point3> plistUVW;
			plistUVW.SetCount(degree);
			for (int m = 0; m < degree; m++)
			{						
				int iPri = (m == 0)?degree-1:m-1;
				int gIndexPri = md->GetFaceGeomVert(iPolyIndex,iPri);
				int gIndexCur = md->GetFaceGeomVert(iPolyIndex,m);
				int gIndexNex = md->GetFaceGeomVert(iPolyIndex,(m+1)%degree);

				int tIndexPri = md->GetFaceTVVert(iPolyIndex,iPri);
				int tIndexCur = md->GetFaceTVVert(iPolyIndex,m);
				int tIndexNex = md->GetFaceTVVert(iPolyIndex,(m+1)%degree);

				Point3 gPointPri = md->GetGeomVert(gIndexPri);
				Point3 gPointCur = md->GetGeomVert(gIndexCur);
				Point3 gPointNex = md->GetGeomVert(gIndexNex);

				Point3 tPointPri = Point3(md->GetTVVert(tIndexPri)[in1],md->GetTVVert(tIndexPri)[in2],0.0f);
				Point3 tPointCur = Point3(md->GetTVVert(tIndexCur)[in1],md->GetTVVert(tIndexCur)[in2],0.0f);
				Point3 tPointNex = Point3(md->GetTVVert(tIndexNex)[in1],md->GetTVVert(tIndexNex)[in2],0.0f);

				if(distortion == eAngleDistortion)
				{
					Point3 gVec[2];
					gVec[0] = Normalize(gPointPri - gPointCur);
					gVec[1] = Normalize(gPointNex - gPointCur);
					float gAngle = fabs(md->AngleFromVectors(gVec[0],gVec[1]));

					Point3 tVec[2];
					tVec[0] = Normalize(tPointPri - tPointCur);
					tVec[1] = Normalize(tPointNex - tPointCur);
					float tAngle = fabs(md->AngleFromVectors(tVec[0],tVec[1]));

					if(uvIndexA == tIndexCur)
					{
						fAngleDeltaA = ComputeAngleDelta(tAngle,gAngle,bAngleDeltaA);
					}
					else if(uvIndexB == tIndexCur)
					{							
						fAngleDeltaB = ComputeAngleDelta(tAngle,gAngle,bAngleDeltaB);
					}
					else if(uvIndexC == tIndexCur)
					{
						fAngleDeltaC = ComputeAngleDelta(tAngle,gAngle,bAngleDeltaC);
					}
				}

				if(distortion == eAreaDistortion)
				{
					plistGeo[m] = gPointCur;
					plistUVW[m] = tPointCur;
				}						
			}		

			Point4 colABase;
			Point4 colBBase;
			Point4 colCBase;
			if(distortion == eAngleDistortion)
			{
				colABase = AdjustColorValueByAistortion(bAngleDeltaA,fAngleDeltaA);
				colBBase = AdjustColorValueByAistortion(bAngleDeltaB,fAngleDeltaB);
				colCBase = AdjustColorValueByAistortion(bAngleDeltaC,fAngleDeltaC);
			}
			else if(distortion == eAreaDistortion)
			{
				float geomArea = AreaOfPolygon(plistGeo);
				float uvArea   = AreaOfPolygon(plistUVW);				
				if(geomArea == 0.0)
				{
					geomArea = 1.0;
					uvArea = 1.0;
				}
				else
				{
					uvArea = everyFaceAreaRatio[iPolyIndex] * uvArea;
				}
				bool bDeltaArea = (uvArea - geomArea) >= 0 ? true : false;
				float fAreaDelta = fabs(uvArea - geomArea)/geomArea;
				fAreaDelta = fAreaDelta > 1.0?1.0:fAreaDelta;

				colABase = AdjustColorValueByAistortion(bDeltaArea,fAreaDelta);
				colBBase = colABase;
				colCBase = colABase;
			}
			FillDistortionFaceData(mDistortion2DElement,uvPosA,uvPosB,uvPosC,GammaCorrect(colABase),GammaCorrect(colBBase),GammaCorrect(colCBase));			
			FillDistortionFaceData(mDistortion3DElements[ldID],gePosA,gePosB,gePosC,colABase,colBBase,colCBase);
		}
	}	
}

void Painter2D::ForceDistortionRedraw()
{
	if(nullptr == GetUnwrapModPtr())
	{
		return;
	}

	ResetDistortionElement();
	MakeDistortion();

	for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *md = GetUnwrapModPtr()->mMeshTopoData[ldID];
		if(nullptr == md )
		{
			continue;
		}
		//The ViewportRenderNodeNotifier will receive the notification and set dirty to update the render item.
		INode* pNode = GetUnwrapModPtr()->GetMeshTopoDataNode(ldID);
		if(pNode)
		{
			pNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
		}
	}

	TimeValue t = GetUnwrapModPtr()->ip->GetTime();
	GetUnwrapModPtr()->ip->RedrawViews(t);
}

float Painter2D::ComputeAngleDelta(float tAngle,float gAngle,bool& bPositive)
{
	bPositive = (tAngle - gAngle) >= 0 ? true : false;

	//the unit is degree
	const float fAngleLimit = 60.0;

	float fAngleDelta = fabs(tAngle - gAngle)*180.0/PI;
	fAngleDelta = fAngleDelta/fAngleLimit;
	fAngleDelta = fAngleDelta > 1.0 ? 1.0:fAngleDelta;

	return fAngleDelta;
}

Point4 Painter2D::AdjustColorValueByAistortion(bool bPositive,float fDelta)
{
	Point4 resultColor(1.0f,1.0f,1.0f,1.0f);

	//RGB(70,70,70)
	Point3 grayColor(0.275f,0.275f,0.275f);
	// 0.78 = 200/255
	const float fLimit = 0.78f;
	if(!bPositive)
	{
		resultColor.x = grayColor.x + grayColor.x*fDelta;
		resultColor.y = grayColor.y - grayColor.y*fDelta;
		resultColor.z = grayColor.z - grayColor.z*fDelta;

		resultColor.x = resultColor.x > fLimit ? fLimit:resultColor.x;
		resultColor.y = resultColor.y < 0.0 ? 0.0:resultColor.y;
		resultColor.z = resultColor.z < 0.0 ? 0.0:resultColor.z;
	}
	else
	{
		resultColor.x = grayColor.x - grayColor.x*fDelta;
		resultColor.y = grayColor.y - grayColor.y*fDelta;
		resultColor.z = grayColor.z + grayColor.z*fDelta;

		resultColor.x = resultColor.x < 0.0 ? 0.0:resultColor.x;
		resultColor.y = resultColor.y < 0.0 ? 0.0:resultColor.y;
		resultColor.z = resultColor.z > fLimit ? fLimit:resultColor.z;
	}

	return resultColor;
}

Point4 Painter2D::GammaCorrect(const Point4& inputColor) const
{
	Point4 resultColor = inputColor;

	IColorCorrectionMgr* idispGamMgr = (IColorCorrectionMgr*) GetCOREInterface(COLORCORRECTIONMGR_INTERFACE);
	if(idispGamMgr)
	{
		resultColor.z = idispGamMgr->ColorCorrect8((UBYTE)(resultColor.z*255.0f),IColorCorrectionMgr::kBLUE_C) / 255.0f;
		resultColor.y = idispGamMgr->ColorCorrect8((UBYTE)(resultColor.y*255.0f),IColorCorrectionMgr::kGREEN_C) / 255.0f;
		resultColor.x = idispGamMgr->ColorCorrect8((UBYTE)(resultColor.x*255.0f),IColorCorrectionMgr::kRED_C) / 255.0f;
	}

	return resultColor;
}

void Painter2D::FillDistortionFaceData(DrawingElement& targetElement,
									   Point3& posA,Point3& posB,Point3& posC,
									   Point4& colA,Point4& colB,Point4& colC)
{
	Point3 normalP(0.0f,0.0f,1.0f);
	int iExistingCount = targetElement.mVertexCount;
	int iSize = int(targetElement.mPositions.size());
	if(iExistingCount + 3 < iSize)
	{
		int index0 = iExistingCount;
		int index1 = iExistingCount+1;
		int index2 = iExistingCount+2;

		targetElement.mPositionsIndex[index0] = index0;
		targetElement.mPositionsIndex[index1] = index1;
		targetElement.mPositionsIndex[index2] = index2;

		targetElement.mPositions[index0] = posA;
		targetElement.mPositions[index1] = posB;
		targetElement.mPositions[index2] = posC;

		targetElement.mNormals[index0] = normalP;
		targetElement.mNormals[index1] = normalP;
		targetElement.mNormals[index2] = normalP;

		targetElement.mColors[index0] = colA;
		targetElement.mColors[index1] = colB;
		targetElement.mColors[index2] = colC;
	}
	else
	{
		targetElement.mPositionsIndex.push_back(iExistingCount + 0);
		targetElement.mPositionsIndex.push_back(iExistingCount + 1);
		targetElement.mPositionsIndex.push_back(iExistingCount + 2);

		targetElement.mPositions.push_back(posA);
		targetElement.mPositions.push_back(posB);
		targetElement.mPositions.push_back(posC);

		targetElement.mNormals.push_back(normalP);
		targetElement.mNormals.push_back(normalP);
		targetElement.mNormals.push_back(normalP);

		targetElement.mColors.push_back(colA);
		targetElement.mColors.push_back(colB);
		targetElement.mColors.push_back(colC);
	}
	targetElement.mVertexCount += 3;
}

void Painter2D::PaintEdgeDistortion()
{
	COLORREF goalColor = ColorMan()->GetColor(EDGEDISTORTIONCOLORID);
	COLORREF baseColor = ColorMan()->GetColor(EDGEDISTORTIONGOALCOLORID);

	Color goalc(goalColor);
	Color basec(baseColor);

	Tab<int> whichColor;
	Tab<float> whichPer;
	Tab<float> goalLength;
	Tab<float> currentLength;
	Tab<Point3> vec;

	int index1 = 0;
	int index2 = 1;
	GetUnwrapModPtr()->GetUVWIndices(index1,index2);

	BOOL localDistortion = TRUE;
	TimeValue t = GetCOREInterface()->GetTime();
	GetUnwrapModPtr()->pblock->GetValue(unwrap_localDistorion,t,localDistortion,FOREVER);
	if (GetUnwrapModPtr()->showEdgeDistortion && localDistortion)
		GetUnwrapModPtr()->BuildEdgeDistortionData();

	for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[ldID];

		int ePtrListCount = ld->GetNumberTVEdges();

		whichColor.SetCount(ePtrListCount);
		whichPer.SetCount(ePtrListCount);
		currentLength.SetCount(ePtrListCount);
		goalLength.SetCount(ePtrListCount);
		vec.SetCount(ePtrListCount);
		int numberOfColors = 10;

		for (int i = 0; i < ePtrListCount; i++)
		{
			BOOL showEdge = TRUE;
			int edgeIndex =  i;
			if (GetUnwrapModPtr()->mTVSubObjectMode == TVEDGEMODE)
			{
				if (ld->GetTVEdgeSelected(edgeIndex))
					showEdge = FALSE;

			}
			if (ld->GetTVEdgeHidden(edgeIndex))
				showEdge = FALSE;

			if (GetUnwrapModPtr()->peltData.peltDialog.hWnd)
			{
				if (ld == GetUnwrapModPtr()->peltData.mBaseMeshTopoDataCurrent)
				{
					if (!GetUnwrapModPtr()->peltData.mIsSpring[i])
						showEdge = FALSE;
				}
			}

			whichColor[i] = -1;
			if (showEdge)
			{
				Point3 pa, pb;
				int a,b;
				a = ld->GetTVEdgeVert(edgeIndex,0);
				b = ld->GetTVEdgeVert(edgeIndex,1);

				int va,vb;
				va = ld->GetTVEdgeGeomVert(edgeIndex,0);
				vb = ld->GetTVEdgeGeomVert(edgeIndex,1);

				if ((ld->IsTVVertVisible(a) || ld->IsTVVertVisible(b)) )
				{
					goalLength[i] = Length(ld->GetGeomVert(va) -ld->GetGeomVert(vb))*GetUnwrapModPtr()->edgeScale* GetUnwrapModPtr()->edgeDistortionScale;
					pa = ld->GetTVVert(a);
					pb = ld->GetTVVert(b);
					pa.z = 0.0f;
					pb.z = 0.0f;

					vec[i] = (pb-pa);
					Point3 uva,uvb;
					uva = pa;
					uvb = pb;
					uva.z = 0.0f;
					uvb.z = 0.0f;

					currentLength[i] = Length(uva-uvb);


					float dif = fabs(goalLength[i]-currentLength[i])*4.0f;
					float per = ((dif/goalLength[i]) * numberOfColors);
					if (per < 0.0f) per = 0.0f;
					if (per > 1.0f) per = 1.0f;
					whichPer[i] = per ;
					whichColor[i] = (int)(whichPer[i]*(numberOfColors-1));
				}
			}

		}

		Tab<Point2> la,lb;

		la.SetCount(ePtrListCount);
		lb.SetCount(ePtrListCount);

		Tab<int> colorList;
		colorList.SetCount(ePtrListCount);
		BitArray fullEdge;
		fullEdge.SetSize(ePtrListCount);
		fullEdge.ClearAll();

		Point4 whiteColorP4(1.0f,1.0f,1.0f,1.0f);

		numberOfColors = 10;
		for (int i = 0; i < ePtrListCount; i++)
		{
			BOOL showEdge = TRUE;
			int edgeIndex =  i;
			if (edgeIndex != -1)
			{
				if (GetUnwrapModPtr()->mTVSubObjectMode == TVEDGEMODE)
				{
					if (ld->GetTVEdgeSelected(edgeIndex))
						showEdge = FALSE;

				}
				if (ld->GetTVEdgeHidden(edgeIndex))
					showEdge = FALSE;
			}
			else showEdge = FALSE;
			//get our edge spring length goal
			if (showEdge)
			{
				int va,vb;
				va = ld->GetTVEdgeGeomVert(edgeIndex,0);
				vb = ld->GetTVEdgeGeomVert(edgeIndex,1);

				int a,b;
				a = ld->GetTVEdgeVert(edgeIndex,0);
				b = ld->GetTVEdgeVert(edgeIndex,1);
				if ((ld->IsTVVertVisible(a) || ld->IsTVVertVisible(b)) )
				{
					float goalL = goalLength[i];

					//get our current length
					Point3 pa, pb;
					pa = ld->GetTVVert(a);
					pb = ld->GetTVVert(b);
					pa.z = 0.0f;
					pb.z = 0.0f;

					Point3 vec = (pb-pa);
					Point3 uva,uvb;
					uva = pa;
					uvb = pb;
					uva.z = 0.0f;
					uvb.z = 0.0f;

					float currentL = currentLength[i];
					float per = 1.0f;
					float dif = fabs(goalL-currentL)*4.0f;
					if (goalL != 0.0f)
						per = (dif/goalL);

					if (per < 0.0f) per = 0.0f;
					if (per > 1.0f) per = 1.0f;
					//compute the color
					//get the center and draw out
					Color c = (basec * (per)) + (goalc*(1.0f-per));
					int gr=0,gg=0,gb=0;
					gr = (int) (255 * c.r);
					gg = (int) (255 * c.g);
					gb = (int) (255 * c.b);

					colorList[i] = int(per * numberOfColors);

					if (currentL <= goalL)
					{
						fullEdge.Set(i);

						la[edgeIndex].x = ld->GetTVVert(a)[index1];
						la[edgeIndex].y = ld->GetTVVert(a)[index2];

						lb[edgeIndex].x = ld->GetTVVert(b)[index1];
						lb[edgeIndex].y = ld->GetTVVert(b)[index2];
					}
					else
					{
						Point3 mid = (pb+pa) * 0.5f;
						Point3 nvec = Normalize(vec);
						nvec = nvec * goalL * 0.5f;

						Point3 tvPoint = mid + nvec;
						Point2 aPoint;
						aPoint.x  = tvPoint[index1];
						aPoint.y  = tvPoint[index2];

						Point3 tvPoint2 = mid - nvec;
						Point2 bPoint;
						bPoint.x  = tvPoint2[index1];
						bPoint.y  = tvPoint2[index2];

						la[edgeIndex] = aPoint;
						lb[edgeIndex] = bPoint;

						DrawSingleEdge(mUVLines,aPoint.x,aPoint.y,ld->GetTVVert(b)[index1],ld->GetTVVert(b)[index2],whiteColorP4);
						DrawSingleEdge(mUVLines,bPoint.x,bPoint.y,ld->GetTVVert(a)[index1],ld->GetTVVert(a)[index2],whiteColorP4);
					}

				}

			}
		}
		for (int j = 0; j <= numberOfColors;j++)
		{
			float per =(float)j/(float)numberOfColors;
			Color c = (basec * (per)) + (goalc*(1.0f-per));

			Point4 penColorP4(c.r,c.g,c.b,1.0f);

			for (int i = 0; i < ePtrListCount; i++)
			{
				BOOL showEdge = TRUE;
				int edgeIndex =  i;
				if (edgeIndex != -1)
				{
					if (GetUnwrapModPtr()->mTVSubObjectMode == TVEDGEMODE)
					{
						if (ld->GetTVEdgeSelected(edgeIndex))
							showEdge = FALSE;

					}
					if (ld->GetTVEdgeHidden(edgeIndex))
						showEdge = FALSE;
				}
				else showEdge = FALSE;

				if (showEdge && (colorList[i] == j))
				{
					int a,b;
					a = ld->GetTVEdgeVert(i,0);
					b = ld->GetTVEdgeVert(i,1);

					if ((ld->IsTVVertVisible(a) || ld->IsTVVertVisible(b)) )
					{
						DrawSingleEdge(mUVLines,la[i].x,la[i].y,lb[i].x,lb[i].y,penColorP4);
					}
				}
			}
		}
	}
}

void UnwrapMod::fnPaintView()
{
	if (MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		Painter2D::Singleton().SetUnwrapMod(this);
		Painter2D::Singleton().PaintView();
	}
	else
	{
		Painter2DLegacy::Singleton().SetUnwrapMod(this);
		Painter2DLegacy::Singleton().PaintView();
	}

	// after redraw, the XOR gizmo has been cleared, so reset the first flag.
	MouseCallBack* pCallback = mouseMan.GetMouseProc(LEFT_BUTTON);
	if (pCallback)
	{
		if (pCallback == paintMoveMode)
		{
			paintMoveMode->first = TRUE;
		}
		else if (pCallback == relaxMoveMode)
		{
			relaxMoveMode->first = TRUE;
		}
		else if (pCallback == paintSelectMode)
		{
			paintSelectMode->first = TRUE;
		}
	}
}

void Painter2D::PaintView()
{
	UnwrapMod* pUnwrapMod = GetUnwrapModPtr();
	if(nullptr == pUnwrapMod)
	{
		return;
	}

	if (pUnwrapMod->bringUpPanel)
	{
		pUnwrapMod->bringUpPanel = FALSE;
		SetFocus(pUnwrapMod->hDialogWnd);
	}

	// russom - August 21, 2006 - 804464
	// Very very quick switching between modifiers might create a
	// situation in which EndEditParam is called within the MoveScriptUI call above.
	// This will set the ip ptr to NULL and we'll crash further down.
	if(pUnwrapMod->ip == NULL )
		return;

	if (pUnwrapMod->bShowFPSinEditor)
		MaxSDK::PerformanceTools::Timer::StartTimerGlobal(globalTimerID);

	PAINTSTRUCT		ps;
	BeginPaint(pUnwrapMod->hView,&ps);
	EndPaint(pUnwrapMod->hView,&ps);

	float xzoom, yzoom;
	int width,height;
	pUnwrapMod->ComputeZooms(pUnwrapMod->hView,xzoom,yzoom,width,height);

	if(NULL == mWindow2DDisplayCallback)
	{
		using namespace MaxSDK::Graphics;

		mWindow2DDisplayCallback = new Window2DDisplayCallback(this);

		mPresenttableTarget.Initialize(width,height,TargetFormatA8R8G8B8);			
		mImmediateFragment.Initialize();
		mImmediateFragment.SetUseDepthTarget(true);
		mVertexColorMaterial.Initialize();			

		mBackgroundMaterial.Initialize();
		miViewWidth = width;
		miViewHeight = height;			

		//dot line or rectangle needs more lines

		mpFaceVertexes = NULL;
		miFaceVertexesCount = 0;
		miFacesCount = 0;
		mpFacesDistribution = NULL;

		miFaceVertexesCountMax = 0;
		miFacesCountMax = 0;
		miVertexesCountMaxInStencil = 0;

		mBackgroundOneTextureHandle.Initialize(pUnwrapMod->rendW, pUnwrapMod->rendH,MaxSDK::Graphics::TargetFormatA8R8G8B8);
	}

	mBkgColorRed	=  GetRValue(pUnwrapMod->backgroundColor)/255.0f;
	mBkgColorGreen	=  GetGValue(pUnwrapMod->backgroundColor)/255.0f;
	mBkgColorBlue	=  GetBValue(pUnwrapMod->backgroundColor)/255.0f;

	if(miViewWidth != width || miViewHeight != height)
	{
		mPresenttableTarget.Initialize(width,height,MaxSDK::Graphics::TargetFormatA8R8G8B8);	
		miViewWidth = width;
		miViewHeight = height;
	}

	mTextContainer.Reset();
	// Now paint points
	mVertexTicks.Reset();
	if (pUnwrapMod->mTVSubObjectMode == TVVERTMODE)
	{
		PaintVertexTicks();
	}

	if (pUnwrapMod->fnGetPaintMoveBrush() || (pUnwrapMod->mTVSubObjectMode == TVVERTMODE && pUnwrapMod->fnGetEnableSoftSelection()))
	{
		PaintVertexInfluence();
	}

	mGizmoLines.Reset();
	mGizmoPolygons.Reset();
	mGizmoTransparentPolygons.Reset();
	if (pUnwrapMod->mTVSubObjectMode != TVOBJECTMODE)
	{
		PaintFreeFormGizmo();
	}

	if (!pUnwrapMod->viewValid)
	{
		pUnwrapMod->viewValid = TRUE;

		//Compute the tiles that contain all vertices
		ComputeTiles();

		//make sure our background texture is right
		if (!pUnwrapMod->image && pUnwrapMod->GetActiveMap())
			pUnwrapMod->SetupImage();

		//paint the background	
		if (pUnwrapMod->image && pUnwrapMod->showMap)
		{
			PaintBackground();
		}

		//Reset the data container	
		mUVRegularEdges.Reset();
		mUVSelectedEdges.Reset();
		mUVLines.Reset();
		mFaceSelectedLines.Reset();

		//Paint all the edges now
		PaintEdges();

		PreparePaintFacesData();
		//now paint the selected faces
		if (pUnwrapMod->mTVSubObjectMode == TVFACEMODE)
		{
			// paint selected faces
			PaintFaces();
		}

		PaintPelt();

		ResetDistortionElement();
		if (pUnwrapMod->GetDistortionType() == eAngleDistortion ||
			pUnwrapMod->GetDistortionType() == eAreaDistortion)
		{
			MakeDistortion();
		}

		BOOL showLocalDistortion = 0;
		TimeValue t = GetCOREInterface()->GetTime();
		pUnwrapMod->pblock->GetValue(unwrap_localDistorion,t,showLocalDistortion,FOREVER);

		if (pUnwrapMod->showEdgeDistortion)
			PaintEdgeDistortion();

		if ( (pUnwrapMod->mode == ID_SKETCHMODE))
		{		
			Point4 selColorP4((float)GetRValue(pUnwrapMod->selColor)/255.0f,
				(float)GetGValue(pUnwrapMod->selColor)/255.0f,
				(float)GetBValue(pUnwrapMod->selColor)/255.0f,
				1.0f);
			Point2 ipt[4];

			int index1 = 0;
			int index2 = 1;
			pUnwrapMod->GetUVWIndices(index1,index2);
			for (int i = 0; i < pUnwrapMod->sketchMode->indexList.Count(); i++)
			{
				int index = pUnwrapMod->sketchMode->indexList[i].mIndex;
				MeshTopoData *ld = pUnwrapMod->sketchMode->indexList[i].mLD;

				ipt[0].x = ld->GetTVVert(index)[index1];
				ipt[0].y = ld->GetTVVert(index)[index2];

				DrawRectangle(
					mUVLines,
					ipt[0].x-SideDelta,
					ipt[0].y-SideDelta,
					ipt[0].x+SideDelta,
					ipt[0].y+SideDelta,
					selColorP4);

				if (pUnwrapMod->sketchDisplayPoints)
				{
					TSTR vertStr;
					vertStr.printf(_T("%d"),i);

					mTextContainer.mPositions.push_back(Point2(ipt[0].x+SideDelta,ipt[0].y-SideDelta));
					mTextContainer.mTexts.push_back(vertStr);
				}
			}
		}

		if (pUnwrapMod->fnGetShowCounter())
		{
			int vct = 0;
			int ect = 0;
			int fct = 0;

			for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
			{
				vct += pUnwrapMod->mMeshTopoData[ldID]->GetTVVertSel().NumberSet();
				ect += pUnwrapMod->mMeshTopoData[ldID]->GetTVEdgeSel().NumberSet();
				fct += pUnwrapMod->mMeshTopoData[ldID]->GetFaceSel().NumberSet();
			}

			TSTR vertStr;
			if (pUnwrapMod->fnGetTVSubMode() == TVVERTMODE)
				vertStr.printf(_T("%d %s"),(int)vct,GetString(IDS_VERTEXCOUNTER));
			if (pUnwrapMod->fnGetTVSubMode() == TVEDGEMODE)
				vertStr.printf(_T("%d %s"),(int)ect,GetString(IDS_EDGECOUNTER));
			if (pUnwrapMod->fnGetTVSubMode() == TVFACEMODE)
				vertStr.printf(_T("%d %s"),(int)fct,GetString(IDS_FACECOUNTER));

			mTextContainer.mPositions.push_back(Point2(SideDelta,SideDelta));
			mTextContainer.mTexts.push_back(vertStr);
		}
	}

	mGridLines.Reset();
	mAxisBoldLines.Reset();
	PaintGrid();

	if(mImmediateFragment.Begin())
	{			
		mImmediateFragment.SetTarget(mPresenttableTarget);
		mImmediateFragment.Clear(AColor(mBkgColorRed,mBkgColorGreen,mBkgColorBlue));

		MaxSDK::Graphics::Matrix44 matTransWorld;
		matTransWorld.MakeIdentity();

		//construct the matrix that transfer the data from the UVW space to the NDC-Normalized Device Coordinate
		//The process is the combination of the UVW to Screen and Screen to NDC.

		//Reference to the function UnwrapMod::UVWToScreen
		//////////////////////////////////////////////////////////////////////////
		//														 width
		// /|\1.0										----------------------->x
		//	|											|	
		//	| V											|	
		//	|											|
		//	|				1.0							|
		//	|---------------->			TO				| 
		//			U									|	height
		//											   \|/y	
		//  
		//////////////////////////////////////////////////////////////////////////

		//Reference to the function UnwrapMod::ClientXToNDCX and ClientYToNDCY
		//////////////////////////////////////////////////////////////////////////
		//            width
		//	----------------------->x
		//	|												 /|\ y
		//	|												  |+1
		//	|												  |
		//  |										-1		  |				+1
		//  |                            TO         ----------|------------->x
		//  |	height										  |	
		// \|/y												  |-1
		//  
		//////////////////////////////////////////////////////////////////////////
		float xScale = xzoom*2.0/width;
		float xOffset = (pUnwrapMod->xscroll*2.0 - xzoom)/width;

		float yScale = pUnwrapMod->zoom*2.0;
		float yOffset = (-yzoom - pUnwrapMod->yscroll*2.0)/height;

		matTransWorld._11 = xScale;
		matTransWorld._22 = yScale;
		matTransWorld._41 = xOffset;
		matTransWorld._42 = yOffset;

		mImmediateFragment.SetWorldMatrix(matTransWorld);

		mImmediateFragment.DrawCallback(mWindow2DDisplayCallback);
		mImmediateFragment.End();
		mPresenttableTarget.Present(pUnwrapMod->hView);
	}

	if (pUnwrapMod->bShowFPSinEditor)
		timeElapsed = MaxSDK::PerformanceTools::Timer::EndTimerGlobal(globalTimerID);
}

//in the range (0.0,1.0) , the value 0.001 is OK for line becoming bold.
#define BoldCoefficient 0.001
void Painter2D::DrawSingleEdge(DrawingElement& targetLines,float startX, float startY,float endX,float endY,Point4& edgeColor,int iwidth)
{
	Point3 normalPoint(0.0f,0.0f,1.0f);

	int iExistingCount = targetLines.mVertexCount;
	int iSize = int(targetLines.mPositions.size());

	if(iwidth == 0)
	{
		//ready to forward 2 vertex if the capacity is enough
		if(iExistingCount + 2 < iSize)
		{
			int index0 = iExistingCount;
			int index1 = iExistingCount+1;

			targetLines.mPositionsIndex[index0] = index0;
			targetLines.mPositionsIndex[index1] = index1;

			targetLines.mPositions[index0].x = startX;
			targetLines.mPositions[index0].y = startY;
			targetLines.mPositions[index0].z = 0.0f;

			targetLines.mPositions[index1].x = endX;
			targetLines.mPositions[index1].y = endY;
			targetLines.mPositions[index1].z = 0.0f;

			targetLines.mNormals[index0].x = 0.0f;
			targetLines.mNormals[index0].y = 0.0f;
			targetLines.mNormals[index0].z = 1.0f;

			targetLines.mNormals[index1].x = 0.0f;
			targetLines.mNormals[index1].y = 0.0f;
			targetLines.mNormals[index1].z = 1.0f;

			targetLines.mColors[index0].x = edgeColor.x;
			targetLines.mColors[index0].y = edgeColor.y;
			targetLines.mColors[index0].z = edgeColor.z;
			targetLines.mColors[index0].w = edgeColor.w;

			targetLines.mColors[index1].x = edgeColor.x;
			targetLines.mColors[index1].y = edgeColor.y;
			targetLines.mColors[index1].z = edgeColor.z;
			targetLines.mColors[index1].w = edgeColor.w;

		}
		else
		{
			targetLines.mPositionsIndex.push_back(iExistingCount + 0);
			targetLines.mPositionsIndex.push_back(iExistingCount + 1);

			targetLines.mPositions.push_back(Point3(startX,startY,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);

			targetLines.mPositions.push_back(Point3(endX,endY,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);
		}
		//accumulate 2 vertex
		targetLines.mVertexCount += 2;		
	}
	else if(iwidth > 0)
	{
		float dirX = endX-startX;
		float dirY = endY-startY;
		float length = sqrt(dirX*dirX + dirY*dirY);
		if(length != 0.0)
		{
			dirX = dirX/length;
			dirY = dirY/length;
		}

		//The scale is inversely proportional to the zoom in the UVW space.
		float fYSale = BoldCoefficient*iwidth / GetUnwrapModPtr()->zoom;
		float fXSale = fYSale/GetUnwrapModPtr()->aspect;
		//    0                      1
		//
		//   StartPoint------------>EndPoint
		//
		//    3                      2
		float fX0 = startX + (-dirY)*fXSale;
		float fY0 = startY + dirX*fYSale;

		float fX1 = endX + (-dirY)*fXSale;
		float fY1 = endY + dirX*fYSale;

		float fX2 = endX + dirY*fXSale;
		float fY2 = endY + (-dirX)*fYSale;

		float fX3 = startX + dirY*fXSale;
		float fY3 = startY + (-dirX)*fYSale;

		//ready to forward 6 vertex  if the capacity is enough
		if(iExistingCount + 6 < iSize)
		{
			for (int i=0;i<6;++i)
			{
				targetLines.mPositionsIndex[iExistingCount+i] = iExistingCount+i;
			}

			int index0 = iExistingCount;
			int index1 = iExistingCount+1;
			int index2 = iExistingCount+2;
			int index3 = iExistingCount+3;
			int index4 = iExistingCount+4;
			int index5 = iExistingCount+5;

			targetLines.mPositions[index0].x = fX0;
			targetLines.mPositions[index0].y = fY0;
			targetLines.mPositions[index0].z = 0.0f;

			targetLines.mPositions[index1].x = fX1;
			targetLines.mPositions[index1].y = fY1;
			targetLines.mPositions[index1].z = 0.0f;

			targetLines.mPositions[index2].x = fX3;
			targetLines.mPositions[index2].y = fY3;
			targetLines.mPositions[index2].z = 0.0f;

			targetLines.mPositions[index3].x = fX1;
			targetLines.mPositions[index3].y = fY1;
			targetLines.mPositions[index3].z = 0.0f;

			targetLines.mPositions[index4].x = fX2;
			targetLines.mPositions[index4].y = fY2;
			targetLines.mPositions[index4].z = 0.0f;

			targetLines.mPositions[index5].x = fX3;
			targetLines.mPositions[index5].y = fY3;
			targetLines.mPositions[index5].z = 0.0f;

			for (int i=0;i<6;++i)
			{
				targetLines.mNormals[iExistingCount+i].x = 0.0f;
				targetLines.mNormals[iExistingCount+i].y = 0.0f;
				targetLines.mNormals[iExistingCount+i].z = 1.0f;
			}

			for (int i=0;i<6;++i)
			{
				targetLines.mColors[iExistingCount+i].x = edgeColor.x;
				targetLines.mColors[iExistingCount+i].y = edgeColor.y;
				targetLines.mColors[iExistingCount+i].z = edgeColor.z;
				targetLines.mColors[iExistingCount+i].w = edgeColor.w;
			}
		}
		else
		{
			targetLines.mPositionsIndex.push_back(iExistingCount + 0);
			targetLines.mPositionsIndex.push_back(iExistingCount + 1);
			targetLines.mPositionsIndex.push_back(iExistingCount + 2);
			targetLines.mPositionsIndex.push_back(iExistingCount + 3);
			targetLines.mPositionsIndex.push_back(iExistingCount + 4);
			targetLines.mPositionsIndex.push_back(iExistingCount + 5);

			targetLines.mPositions.push_back(Point3(fX0,fY0,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);

			targetLines.mPositions.push_back(Point3(fX1,fY1,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);

			targetLines.mPositions.push_back(Point3(fX3,fY3,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);

			targetLines.mPositions.push_back(Point3(fX1,fY1,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);

			targetLines.mPositions.push_back(Point3(fX2,fY2,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);

			targetLines.mPositions.push_back(Point3(fX3,fY3,0.0f));
			targetLines.mNormals.push_back(normalPoint);
			targetLines.mColors.push_back(edgeColor);
		}
		//accumulate 6 vertex
		targetLines.mVertexCount += 6;
	}
}

void Painter2D::DrawSingleDotEdge(DrawingElement& targetLines,float startX, float startY,float endX,float endY,Point4& edgeColor,int iwidth)
{
	float dirX = endX-startX;
	float dirY = endY-startY;
	float length = sqrt(dirX*dirX + dirY*dirY);

	float fStepLenghth = SideDelta;
	int segments = length / fStepLenghth;
	if(length != 0.0)
	{
		dirX = dirX / length;
		dirY = dirY / length;
	}

	if(0==segments)
	{
		DrawSingleEdge(targetLines,startX, startY,endX,endY,edgeColor,iwidth);
	}
	else
	{
		float fx0 = startX;
		float fy0 = startY;
		float fx1 = 0.0;
		float fy1 = 0.0;
		Point4 lineColor;
		for (int i = 1; i <= segments; i++)
		{
			//change the edge color
			if(i%2 == 0)
			{
				lineColor = edgeColor;
			}
			else
			{
				lineColor.x = 1.0;
				lineColor.y = 1.0;
				lineColor.z = 1.0;
				lineColor.w = 1.0;
			}

			if(i<segments)
			{
				fx1 = fx0 + dirX*fStepLenghth;
				fy1 = fy0 + dirY*fStepLenghth;
			}
			else
			{
				fx1 = endX;
				fy1 = endY;
			}

			DrawSingleEdge(targetLines,fx0, fy0,fx1,fy1,lineColor,iwidth);

			fx0 = fx1;
			fy0 = fy1;
		}
	}
}

void Painter2D::DrawFilledTriangle(DrawingElement& targetTriangles,Point2& pa, Point2& pb,Point2& pc,Point4& triColor)
{
	Point3 normalPoint(0.0f,0.0f,1.0f);

	int iExistingCount = targetTriangles.mVertexCount;
	int iSize = int(targetTriangles.mPositions.size());

	//ready to forward 3 vertex  if the capacity is enough
	if(iExistingCount + 3 < iSize)
	{
		for (int i=0;i<3;++i)
		{
			targetTriangles.mPositionsIndex[iExistingCount+i] = iExistingCount+i;
		}

		targetTriangles.mPositions[iExistingCount].x = pa.x;
		targetTriangles.mPositions[iExistingCount].y = pa.y;
		targetTriangles.mPositions[iExistingCount].z = 0.0f;

		targetTriangles.mPositions[iExistingCount+1].x = pb.x;
		targetTriangles.mPositions[iExistingCount+1].y = pb.y;
		targetTriangles.mPositions[iExistingCount+1].z = 0.0f;

		targetTriangles.mPositions[iExistingCount+2].x = pc.x;
		targetTriangles.mPositions[iExistingCount+2].y = pc.y;
		targetTriangles.mPositions[iExistingCount+2].z = 0.0f;	

		for (int i=0;i<3;++i)
		{
			targetTriangles.mNormals[iExistingCount+i].x = 0.0f;
			targetTriangles.mNormals[iExistingCount+i].y = 0.0f;
			targetTriangles.mNormals[iExistingCount+i].z = 1.0f;
		}

		for (int i=0;i<3;++i)
		{
			targetTriangles.mColors[iExistingCount+i].x = triColor.x;
			targetTriangles.mColors[iExistingCount+i].y = triColor.y;
			targetTriangles.mColors[iExistingCount+i].z = triColor.z;
			targetTriangles.mColors[iExistingCount+i].w = triColor.w;
		}
	}
	else
	{
		targetTriangles.mPositionsIndex.push_back(iExistingCount + 0);
		targetTriangles.mPositionsIndex.push_back(iExistingCount + 1);
		targetTriangles.mPositionsIndex.push_back(iExistingCount + 2);

		targetTriangles.mPositions.push_back(Point3(pa.x,pa.y,0.0f));
		targetTriangles.mNormals.push_back(normalPoint);
		targetTriangles.mColors.push_back(triColor);

		targetTriangles.mPositions.push_back(Point3(pb.x,pb.y,0.0f));
		targetTriangles.mNormals.push_back(normalPoint);
		targetTriangles.mColors.push_back(triColor);

		targetTriangles.mPositions.push_back(Point3(pc.x,pc.y,0.0f));
		targetTriangles.mNormals.push_back(normalPoint);
		targetTriangles.mColors.push_back(triColor);
	}
	//accumulate 3 vertex
	targetTriangles.mVertexCount += 3;
}

void Painter2D::DrawRectangle(DrawingElement& targetLines,float left,float top,float right,float bottom,Point4& rectangleColor,int iwidth)
{
	DrawSingleEdge(targetLines,left, top,right, top,rectangleColor,iwidth);
	DrawSingleEdge(targetLines,right,top,right,bottom,rectangleColor,iwidth);
	DrawSingleEdge(targetLines,right,bottom,left,bottom,rectangleColor,iwidth);
	DrawSingleEdge(targetLines,left,bottom,left,top,rectangleColor,iwidth);
}

void Painter2D::DrawEdge(DrawingElement& targetLines,int vecA,int vecB, 
						 Point2& pa, Point2& pb, Point2& pvecA, Point2& pvecB,Point4& edgeColor,int iwidth,bool bDot)
{	
	if ((vecA ==-1) || (vecB == -1))
	{
		if(bDot)
		{			
			DrawSingleDotEdge(targetLines,pa.x,pa.y,pb.x,pb.y,edgeColor,iwidth);
		}
		else
		{
			DrawSingleEdge(targetLines,pa.x,pa.y,pb.x,pb.y,edgeColor,iwidth);
		}		
	}
	else
	{
		Point2 fpa,fpb,fpvecA,fpvecB;

		fpa.x = (float)pa.x;
		fpa.y = (float)pa.y;

		fpb.x = (float)pb.x;
		fpb.y = (float)pb.y;

		fpvecA.x = (float)pvecA.x;
		fpvecA.y = (float)pvecA.y;

		fpvecB.x = (float)pvecB.x;
		fpvecB.y = (float)pvecB.y;

		//For Line list		
		Point2 StartPoint(pa.x,pa.y);
		for (int i = 1; i < 7; i++)
		{
			float t = float (i)/7.0f;

			float s = (float)1.0-t;
			float t2 = t*t;
			Point2 p = ( ( s*fpa + (3.0f*t)*fpvecA)*s + (3.0f*t2)* fpvecB)*s + t*t2*fpb;

			//For Line list
			if(bDot)
			{
				DrawSingleDotEdge(targetLines,StartPoint.x,StartPoint.y,p.x,p.y,edgeColor,iwidth);
			}
			else
			{
				DrawSingleEdge(targetLines,StartPoint.x,StartPoint.y,p.x,p.y,edgeColor,iwidth);
			}			

			StartPoint.x = p.x;
			StartPoint.y = p.y;
		}

		//For Line list
		if(bDot)
		{
			DrawSingleDotEdge(targetLines,StartPoint.x,StartPoint.y,pb.x,pb.y,edgeColor,iwidth);
		}
		else
		{
			DrawSingleEdge(targetLines,StartPoint.x,StartPoint.y,pb.x,pb.y,edgeColor,iwidth);
		}		
	}
}

#define PatternLinesCount 320
#define PatternColorRed 200
#define PatternColorGreen 200
#define PatternColorBlue 0
//indicate the pattern line start point in the UVW space
#define StartValue 4.0
#define DefaultNormal Point3(0.0,0.0,1.0)

void Painter2D::PreparePaintFacesData()
{
	miFaceVertexesCount = 0;
	miFacesCount = 0;

	if(mSelFacesFillMode != GetUnwrapModPtr()->fnGetFillMode())
	{
		mSelFacesFillMode = GetUnwrapModPtr()->fnGetFillMode();
		mPatternForFilledFace.Clear();
	}

	if(0 == mPatternForFilledFace.mPositions.size())
	{
		switch (mSelFacesFillMode)
		{
		case FILL_MODE_OFF:
			break;
		case FILL_MODE_SOLID:
			{
				// Build a triangle strip for our quad: 2 triangles, 4 vertices, 4 indices
				// 0 - 1
				// |  /|
				// | / |
				// |/  |
				// 3 - 2
				// Above diagram with the vertex indices shown.
				// Triangles to draw: (0, 1, 3) and (1, 3, 2)

				mPatternForFilledFace.mPositions.push_back(Point3(-StartValue,StartValue,0.0));
				mPatternForFilledFace.mPositions.push_back(Point3(StartValue,StartValue,0.0));
				mPatternForFilledFace.mPositions.push_back(Point3(StartValue,-StartValue,0.0));
				mPatternForFilledFace.mPositions.push_back(Point3(-StartValue,-StartValue,0.0));

				mPatternForFilledFace.mNormals.push_back(DefaultNormal);
				mPatternForFilledFace.mNormals.push_back(DefaultNormal);
				mPatternForFilledFace.mNormals.push_back(DefaultNormal);
				mPatternForFilledFace.mNormals.push_back(DefaultNormal);

				mPatternForFilledFace.mPositionsIndex.push_back(0);
				mPatternForFilledFace.mPositionsIndex.push_back(1);
				mPatternForFilledFace.mPositionsIndex.push_back(3);
				mPatternForFilledFace.mPositionsIndex.push_back(2);

				mPatternForFilledFace.mColors.push_back(FilledColorRedWithAlpha);
				mPatternForFilledFace.mColors.push_back(FilledColorRedWithAlpha);
				mPatternForFilledFace.mColors.push_back(FilledColorRedWithAlpha);
				mPatternForFilledFace.mColors.push_back(FilledColorRedWithAlpha);

				break;
			}
		case FILL_MODE_BDIAGONAL:
		case FILL_MODE_CROSS:
		case FILL_MODE_DIAGCROSS:
		case FILL_MODE_FDIAGONAL:
		case FILL_MODE_HORIZONAL:
		case FILL_MODE_VERTICAL:
			{
				// horizontal lines data + vertical lines data in UVW space
				for (int i=0;i<PatternLinesCount;i++)
				{
					//for every horizontal line, its x is from  -StartValue to StartValue, its y is increased by delta
					mPatternForFilledFace.mPositions.push_back(Point3(-StartValue,StartValue - 2*StartValue*i/PatternLinesCount,0.0));
					mPatternForFilledFace.mPositions.push_back(Point3(StartValue, StartValue - 2*StartValue*i/PatternLinesCount,0.0));
				}

				for (int i=0;i<PatternLinesCount;i++)
				{
					//for every horizontal line, its y is from  -StartValue to StartValue, its x is increased by delta
					mPatternForFilledFace.mPositions.push_back(Point3(StartValue  - 2*StartValue*i/PatternLinesCount,StartValue,0.0));
					mPatternForFilledFace.mPositions.push_back(Point3(StartValue  - 2*StartValue*i/PatternLinesCount,-StartValue,0.0));
				}

				int iVertexesCount = PatternLinesCount*4;
				for (int i=0;i<iVertexesCount;i++)
				{
					mPatternForFilledFace.mNormals.push_back(DefaultNormal);
				}

				for (int i=0;i<iVertexesCount;i++)
				{
					mPatternForFilledFace.mPositionsIndex.push_back(i);
				}

				for (int i=0;i<iVertexesCount;i++)
				{
					mPatternForFilledFace.mColors.push_back(Point4(PatternColorRed/255.0,PatternColorGreen/255.0,PatternColorBlue/255.0,1.0));
				}

				break;
			}
		default:
			break;
		}
	}	
}

void Painter2D::PrepareFacesInStencil(int iVertexCount)
{
	if(miVertexesCountMaxInStencil >= iVertexCount)
	{
		return;
	}

	//add the new elements
	for (int i=miVertexesCountMaxInStencil;i<iVertexCount;i++)
	{
		mFacesInStencil.mNormals.push_back(Point3(0.0,0.0,1.0));
		mFacesInStencil.mPositionsIndex.push_back(i);
		mFacesInStencil.mColors.push_back(Point4(0.0,0.0,0.0,1.0));
		mFacesInStencil.mPositions.push_back(Point3(0.0,0.0,0.0));
		mFacesInStencil.mOtherPositions.push_back(Point3(0.0,0.0,0.0));
	}

	miVertexesCountMaxInStencil = iVertexCount;
}

//    Input: three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2 on the line
//            <0 for P2 right of the line
double Painter2D::isLeft(Point3 P0, Point3 P1, Point3 P2)
{
	return ((P1.x - P0.x)*(P2.y - P0.y) - (P2.x - P0.x)*(P1.y - P0.y));
}

void Painter2D::DrawPatternFaces(MaxSDK::Graphics::IPrimitiveRenderer& renderer)
{
	if(GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_OFF)
	{
		return;
	}

	//test for stencil region data
	int iLeft = 0;
	int iRight = 0;
	int iVertexesAccumulate = 0;
	//Position the reference vertex at the bottom-center in the  NDC(Normalized Device Coordinate)
	Point3 referenceVertex(0.0,-1.0,0.0);

	//Prepare the data in stencil based on the selected faces' vertexes count multiplying 3.
	PrepareFacesInStencil(miFaceVertexesCount*3);
	//process all faces
	for (int j=0;j<miFacesCount;j++)
	{
		int iVertexesCountThisFace = mpFacesDistribution[j];

		double dLeftArea = 0;
		double dRightArea = 0;
		//process the vertexes belong to this face
		for(int i=0;i<iVertexesCountThisFace;i++)
		{
			Point3 P0 = mpFaceVertexes[i+iVertexesAccumulate];
			Point3 P1;
			if(i<iVertexesCountThisFace - 1)
			{
				P1 = mpFaceVertexes[i+1+iVertexesAccumulate];
			}
			else
			{
				P1 = mpFaceVertexes[0+iVertexesAccumulate];
			}
			Point3 P2 = referenceVertex;

			double leftTmp = isLeft(P0, P1, P2);
			if(leftTmp > 0 )
			{
				dLeftArea = dLeftArea + leftTmp;
			}

			if(leftTmp < 0)
			{
				dRightArea = dRightArea + leftTmp;
			}
		}

		bool bLeftMore = true;
		if(dLeftArea + dRightArea < 0)
		{
			bLeftMore = false;
		}

		for(int i=0;i<iVertexesCountThisFace;i++)
		{
			Point3 P0 = mpFaceVertexes[i+iVertexesAccumulate];
			Point3 P1;
			if(i<iVertexesCountThisFace - 1)
			{
				P1 = mpFaceVertexes[i+1+iVertexesAccumulate];
			}
			else
			{
				P1 = mpFaceVertexes[0+iVertexesAccumulate];
			}
			Point3 P2 = referenceVertex;

			double leftTmp = isLeft(P0, P1, P2);
			//if the vertex input order doesn't not satisfy, then reverse the order.
			if(false == bLeftMore)
			{
				leftTmp = (0 - leftTmp);
			}

			if(leftTmp >= 0)
			{
				//record the left direction vertexes
				mFacesInStencil.mPositions[0+iLeft*3] = mpFaceVertexes[i+iVertexesAccumulate];

				if(i<iVertexesCountThisFace - 1)
				{
					mFacesInStencil.mPositions[1+iLeft*3] = mpFaceVertexes[i+1+iVertexesAccumulate];
				}			
				else
				{
					mFacesInStencil.mPositions[1+iLeft*3] = mpFaceVertexes[0+iVertexesAccumulate];
				}
				mFacesInStencil.mPositions[2+iLeft*3] = referenceVertex;

				iLeft++;
			}
			else
			{
				//record the right direction vertexes
				mFacesInStencil.mOtherPositions[0+iRight*3] = mpFaceVertexes[i+iVertexesAccumulate];
				if(i<iVertexesCountThisFace - 1)
				{
					mFacesInStencil.mOtherPositions[1+iRight*3] = mpFaceVertexes[i+1+iVertexesAccumulate];
				}			
				else
				{
					mFacesInStencil.mOtherPositions[1+iRight*3] = mpFaceVertexes[0+iVertexesAccumulate];
				}
				mFacesInStencil.mOtherPositions[2+iRight*3] = referenceVertex;

				iRight++;
			}


		}
		iVertexesAccumulate = iVertexesAccumulate + iVertexesCountThisFace;			
	}

	using namespace MaxSDK::Graphics;

	DWORD dOriginal = 0x01;
	DWORD dRef = dOriginal + 1;
	MaxSDK::Graphics::IVirtualDevice& vDevice = renderer.GetVirtualDevice();	

	BlendState blendStateBack = vDevice.GetBlendState();
	DepthStencilState stencilStateBack = vDevice.GetDepthStencilState();

	DepthStencilState	stencilState;	

	stencilState.SetDepthEnabled(false);
	stencilState.SetStencilEnabled(true);

	StencilOperation& frontFaceOperation = stencilState.GetFrontFaceOperation();
	frontFaceOperation.SetStencilFunction(CompareFunctionAlways);
	frontFaceOperation.SetStencilDepthFailOperation(StencilOperationTypeKeep);
	frontFaceOperation.SetStencilFailOperation(StencilOperationTypeKeep);

	StencilOperation& backFaceOperation = stencilState.GetBackFaceOperation();
	backFaceOperation.SetStencilFunction(CompareFunctionAlways);
	backFaceOperation.SetStencilDepthFailOperation(StencilOperationTypeKeep);
	backFaceOperation.SetStencilFailOperation(StencilOperationTypeKeep);

	stencilState.SetStencilReference(dOriginal);
	stencilState.SetStencilReadMask(0xF);
	stencilState.SetStencilWriteMask(0xF);

	if(iLeft > 0 || iRight > 0)
	{
		BlendState			blendSta;
		TargetBlendState& targetBlendSta = blendSta.GetTargetBlendState(0);
		targetBlendSta.SetRenderTargetWriteMask(ColorWriteEnableNone);
		vDevice.SetBlendState(blendSta);

		AColor f4Color(1.0,0.0,0.0);
		vDevice.Clear(ClearStencilBuffer,f4Color,0.0f, dOriginal);			
	}

	if(iLeft > 0)
	{
		frontFaceOperation.SetStencilPassOperation(StencilOperationTypeIncrement);
		backFaceOperation.SetStencilPassOperation(StencilOperationTypeIncrement);

		vDevice.SetDepthStencilState(stencilState);

		SimpleVertexStream simpleVertexS;
		simpleVertexS.Positions = mFacesInStencil.mPositions.data();
		simpleVertexS.Normals	= mFacesInStencil.mNormals.data();
		simpleVertexS.Colors	= mFacesInStencil.mColors.data();
		renderer.DrawIndexedPrimitiveUP(
			PrimitiveTriangleList,
			simpleVertexS,
			iLeft,
			mFacesInStencil.mPositionsIndex.data(),
			3*iLeft);
	}

	if(iRight > 0)
	{
		frontFaceOperation.SetStencilPassOperation(StencilOperationTypeDecrement);
		backFaceOperation.SetStencilPassOperation(StencilOperationTypeDecrement);	

		vDevice.SetDepthStencilState(stencilState);

		SimpleVertexStream simpleVertexS;
		simpleVertexS.Positions = mFacesInStencil.mOtherPositions.data();
		simpleVertexS.Normals	= mFacesInStencil.mNormals.data();
		simpleVertexS.Colors	= mFacesInStencil.mColors.data();
		renderer.DrawIndexedPrimitiveUP(
			PrimitiveTriangleList,
			simpleVertexS,
			iRight,
			mFacesInStencil.mPositionsIndex.data(),
			3*iRight);
	}

	if(iLeft > 0 || iRight > 0)
	{	
		BlendState			blendSta;
		TargetBlendState& targetBlendSta = blendSta.GetTargetBlendState(0);
		targetBlendSta.SetRenderTargetWriteMask(ColorWriteEnableRedGreenBlueAlpha);	

		frontFaceOperation.SetStencilFunction(CompareFunctionLessEqual);
		frontFaceOperation.SetStencilPassOperation(StencilOperationTypeKeep);

		backFaceOperation.SetStencilFunction(CompareFunctionLessEqual);
		backFaceOperation.SetStencilPassOperation(StencilOperationTypeKeep);

		stencilState.SetStencilReference(dRef);

		vDevice.SetDepthStencilState(stencilState);	

		SimpleVertexStream simpleVertexS;
		simpleVertexS.Positions = mPatternForFilledFace.mPositions.data();
		simpleVertexS.Normals	= mPatternForFilledFace.mNormals.data();
		simpleVertexS.Colors	= mPatternForFilledFace.mColors.data();

		if(GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_SOLID)
		{
			SetTransparentBlendStatus(targetBlendSta);

			vDevice.SetBlendState(blendSta);

			renderer.DrawIndexedPrimitiveUP(
				PrimitiveTriangleStrip,
				simpleVertexS,
				2,
				mPatternForFilledFace.mPositionsIndex.data(),
				int(mPatternForFilledFace.mPositionsIndex.size()));
		}
		else
		{
			vDevice.SetBlendState(blendSta);

			renderer.DrawIndexedPrimitiveUP(
				PrimitiveLineList,
				simpleVertexS,
				PatternLinesCount*2,
				mPatternForFilledFace.mPositionsIndex.data(),
				PatternLinesCount*4);
		}
	}

	vDevice.SetBlendState(blendStateBack);
	vDevice.SetDepthStencilState(stencilStateBack);
}

void Painter2D::DrawOutlineOfCurrentFace(int iCurrentPolygonVertexCount, bool bDrawPreview)
{
	Point4 selColorP4((float)GetRValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetGValue(GetUnwrapModPtr()->selColor)/255.0f,
		(float)GetBValue(GetUnwrapModPtr()->selColor)/255.0f,
		1.0f);
	if(bDrawPreview)
	{
		Point4 selPreviewColorP4((float)GetRValue(GetUnwrapModPtr()->selPreviewColor)/255.0f,
			(float)GetGValue(GetUnwrapModPtr()->selPreviewColor)/255.0f,
			(float)GetBValue(GetUnwrapModPtr()->selPreviewColor)/255.0f,
			1.0f);
		selColorP4 = selPreviewColorP4;
	}
	//draw current face's outline
	float fStartX = 0.0;
	float fStartY = 0.0;
	int iPolygonVertexStartIndex = miFaceVertexesCount - iCurrentPolygonVertexCount;	
	if(iPolygonVertexStartIndex >= 0)
	{
		for (int i = iPolygonVertexStartIndex; i < miFaceVertexesCount; i++)
		{
			if(i == iPolygonVertexStartIndex)
			{
				fStartX = mpFaceVertexes[i].x;
				fStartY = mpFaceVertexes[i].y;
			}
			//when reach the last point,draw one line from it to the first point;
			if(i == miFaceVertexesCount - 1)
			{
				DrawSingleEdge(
					mFaceSelectedLines,
					mpFaceVertexes[i].x, 
					mpFaceVertexes[i].y,
					fStartX, 
					fStartY,
					selColorP4);
			}
			else
			{
				//draw one line from it to the next point;	
				DrawSingleEdge(
					mFaceSelectedLines,
					mpFaceVertexes[i].x, 
					mpFaceVertexes[i].y,
					mpFaceVertexes[i+1].x, 
					mpFaceVertexes[i+1].y,
					selColorP4);
			}		
		}		
	}
}

void Painter2D::SetTransparentBlendStatus(MaxSDK::Graphics::TargetBlendState& blendStatus)
{
	using namespace MaxSDK::Graphics;

	blendStatus.SetBlendEnabled(true);
	blendStatus.SetSourceBlend(BlendSelectorSourceAlpha);
	blendStatus.SetDestinationBlend(BlendSelectorInvSourceAlpha);
	blendStatus.SetAlphaSourceBlend(BlendSelectorSourceAlpha);
	blendStatus.SetAlphaDestinationBlend(BlendSelectorInvSourceAlpha);
	blendStatus.SetAlphaBlendOperation(BlendOperationAdd);
	blendStatus.SetColorBlendOperation(BlendOperationAdd);
}

void Painter2D::ComputeTiles( bool forceCompute /*= false*/ )
{
	float min = std::numeric_limits<float>::min();
	float max = std::numeric_limits<float>::max();
	mTilesContainer.clear();

	if ( GetUnwrapModPtr()->showMultiTile || forceCompute )
	{
		int index1 = 0;
		int index2 = 1;
		GetUnwrapModPtr()->GetUVWIndices(index1,index2); //account for the space that the user wants to see

        //for each mesh, go over the faces
		for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[ldID];
			if (ld == nullptr)
			{
				DbgAssert(0);
				continue;
			}

            //cache vertex visibility. This is an expensive operation so lets only call it once for each vertex.
            auto numTVerts = ld->GetNumberTVVerts();
            BitArray vertVisibility(numTVerts);
            for (int i = 0; i < numTVerts; i++)
                vertVisibility.Set(i, ld->IsTVVertVisible(i));

            //go through each face bounding box
            int minX, maxX, minY, maxY;
            Box3 faceBBox;
            for (int i = 0; i < ld->GetNumberFaces(); i++)
            {
                faceBBox.Init();
                int degree = ld->GetFaceDegree(i);
                for (int j = 0; j < degree; j++)
                {
                    auto vertIndex = ld->GetFaceTVVert(i, j);
                    if (vertIndex == -1)
                        continue;
                    if (!vertVisibility[vertIndex])
                        continue;
                    faceBBox += ld->GetTVVert(vertIndex);
                }
                if (faceBBox.IsEmpty())
                    continue;

                //possible tiles that this face touches
                minX = floor(faceBBox.Min()[index1]);
                maxX = ceil(faceBBox.Max()[index1]);
                minY = floor(faceBBox.Min()[index2]);
                maxY = ceil(faceBBox.Max()[index2]);
                if (minX == maxX) //process at least one tile
                    maxX += 1;
                if (minY == maxY)
                    maxY += 1;

                //find the tiles that intersect with the face bounding box
                for (int x = minX; x < maxX; x++)
                {
                    for (int y = minY; y < maxY; y++)
                    {
                        auto tile = std::make_pair(x, y);
                        auto alreadyFound = mTilesContainer.find(tile);
                        if (alreadyFound != mTilesContainer.end())
                            continue;

                        //Build the bounding box of a tile in the display space
                        Point3 minTileBox(min,min,min);
                        Point3 maxTileBox(max,max,max);
                        minTileBox[index1] = (float)x;
                        minTileBox[index2] = (float)y;
                        maxTileBox[index1] = (float)x + 1;
                        maxTileBox[index2] = (float)y + 1;
                        Box3 tileBBox(minTileBox, maxTileBox);
                        
                        //for each triangle in the face
                        for (int j = 0; j < degree - 2; j++)
                        {
                            Point3 p1, p2, p3;
                            p1 = ld->GetTVVert(ld->GetFaceTVVert(i, 0));
                            p2 = ld->GetTVVert(ld->GetFaceTVVert(i, j + 1));
                            p3 = ld->GetTVVert(ld->GetFaceTVVert(i, j + 2));

                            if (tileBBox.TriBoxOverlap(p1,p2,p3))
                            {
                                //this tile is being used, we know it isn't added yet.
                                mTilesContainer.insert(tile);
                                break;
                            }

                        }

                    }
                } //for each tile that the face hits

            } //for each face
		} //for each mesh

        if (mTilesContainer.size() == 0)
            mTilesContainer.insert(std::make_pair(0,0)); //make sure that we at least draw the first tile.
	}
	else
	{
		if ( GetUnwrapModPtr()->fnGetTile() )
		{
			const int iTile = GetUnwrapModPtr()->fnGetTileLimit();
			for ( int i = -iTile; i <= iTile; ++i )
			{
				for ( int j = -iTile; j <= iTile; ++j)
				{
					mTilesContainer.insert(std::make_pair(i, j));
				}
			}
		}
		else
		{
            mTilesContainer.insert(std::make_pair(0, 0));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//                          width
//	----------------------->x
//	|													   /|\y
//	|														|+1
//	|														|
//  |												-1		|		+1
//  |                                  TO         ----------|------------->x
//  |	height												|	
// \|/y														|-1
//  
//////////////////////////////////////////////////////////////////////////
////NDC-Normalized Device Coordinate
float Painter2D::ClientXToNDCX(const int x,int width)
{
	if(width != 0)
	{
		float ViewX = ((float)x)*2/width -1;
		return ViewX;
	}

	return 0.0f;
}

float Painter2D::ClientYToNDCY(const int y,int height)
{
	if(height != 0)
	{
		float ViewY =  ((float)(height - y ))*2/height -1;
		return ViewY;
	}

	return 0.0f;
}
