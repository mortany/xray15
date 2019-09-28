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
#pragma once

#include <Graphics/ImmediateFragment.h>
#include <Graphics/VertexColorMaterialHandle.h>
#include <Graphics/SolidColorMaterialHandle.h>
#include <Graphics/TextureMaterialHandle.h>
#include <Graphics/TextureHandle.h>
#include <Graphics/ICustomRenderItem.h>
#include <Graphics/RenderStates.h>

class UnwrapMod;

//For Nitrous draw vertex,edges and faces
class DrawingElement
{
public:
	DrawingElement();
	virtual ~DrawingElement();
	void Reset();
	void Clear();

	std::vector<Point3>	mPositions;
	std::vector<Point3>	mNormals;
	std::vector<Point4> mColors;
	std::vector<int> mPositionsIndex;
	std::vector<Point3>	mOtherPositions;
	int mVertexCount;
};

class QuadShape
{
public:
	QuadShape()	{}
	virtual ~QuadShape(){}
	void Build(float left,float top,float right,float bottom);
	const Point3* GetPositionBuffer() const
	{
		return mPositions;
	}
	const Point3* GetNormal() const
	{
		return mNormal;
	}
	const float* GetTexcoords() const
	{
		return mTexcoords;
	}
	const int* GetPositionsIndex() const
	{
		return mPositionsIndex;
	}
	const int GetTriangleCount() const
	{
		return 2;
	}
private:
	Point3	mPositions[6];
	Point3	mNormal[6];
	float	mTexcoords[18];
	int		mPositionsIndex[6];
};

class TextContainer
{
public:
	TextContainer(){}
	virtual ~TextContainer();

	void Reset();

	std::vector<TSTR>	mTexts;
	std::vector<Point2>	mPositions;
};

class Painter2D : public MaxSDK::Util::Noncopyable
{
public:
	typedef std::pair<int, int> uvOffset;

	virtual ~Painter2D();

	// Get the singleton instance.
	static Painter2D& Singleton();

	//collect the data from the unwrap and organize the drawing data such as vertex buffer, index buffer
	void PaintView();

	//transfer the drawing data to the pipeline under the Nitrous mode
	void  DoDisplay(TimeValue t, MaxSDK::Graphics::IPrimitiveRenderer& renderer,const MaxSDK::Graphics::DisplayCallbackContext& displayContext);

	//check if the display mode is Direct3D 11
	bool CheckDX11Mode();

	void SetUnwrapMod(UnwrapMod* pUnwrapMod);

	const std::set<uvOffset>& GetTilesContainer();

	//Draw element of the angle or area distortion in the viewpot after applying peeling
	std::vector<DrawingElement>			mDistortion3DElements;

	//make distortion data valid and redraw in the 2d view and 3d viewport
	void ForceDistortionRedraw();
	//make distortion data valid in the 2d view and 3d viewport
	void MakeDistortion();
	//Before making distortion, reset the data and make them valid
	void ResetDistortionElement();
private:
	Painter2D();
	MaxSDK::SingleWeakRefMaker					mUnwrapModRefMaker;
	bool										mbDX11Mode; //Indicate if the DX11 mode

	//Paint the vertex,edge and polygon by using the Nitrous SDK
	MaxSDK::Graphics::PresentableTargetHandle	mPresenttableTarget;
	MaxSDK::Graphics::ImmediateFragment			mImmediateFragment;
	MaxSDK::Graphics::IDisplayCallbackPtr		mWindow2DDisplayCallback;
	MaxSDK::Graphics::TextureMaterialHandle		mBackgroundMaterial;
	MaxSDK::Graphics::TextureHandle				mBackgroundOneTextureHandle;
	MaxSDK::Graphics::VertexColorMaterialHandle mVertexColorMaterial;

	//store the UVOffset and the image data for multi-tile material background	
	std::set<uvOffset>						mTilesContainer;
	struct multiTextureElement
	{
		uvOffset								uvInfor;
		MaxSDK::Graphics::TextureHandle			singleTextureHandle;

		multiTextureElement( const uvOffset &uvInfo, const MaxSDK::Graphics::TextureHandle &textureHandle )
			: uvInfor(uvInfo)
			, singleTextureHandle(textureHandle)
		{ }
	};
	std::vector<multiTextureElement>			mTextureHandlesContainer;

	DrawingElement			mUVRegularEdges;		//For the regular edges in the PaintEdge function
	DrawingElement			mUVSelectedEdges;		//For the selected edges in the PaintEdge function
	DrawingElement			mVertexTicks;			//For vertex ticks
	DrawingElement			mUVLines;				//For regular UVW lines
	DrawingElement			mGridLines;				//For grid lines
	DrawingElement			mAxisBoldLines;			//For axis bold lines 
	DrawingElement			mFaceSelectedLines;		//For selected faces' outline
	DrawingElement			mGizmoLines;			//For gizmo lines
	DrawingElement			mGizmoPolygons;			//For gizmo solid polygons
	DrawingElement			mGizmoTransparentPolygons;	//For gizmo transparent polygons
	QuadShape				mBackgroundImageQuad;	//For background image
	TextContainer			mTextContainer;			//For all text

	Point3*					mpFaceVertexes;			//store the vertexes of all faces
	int						miFaceVertexesCount;	//the real number of vertexes of all faces
	int						miFacesCount;			//the real number of all faces
	int*					mpFacesDistribution;	//store the count distribution of all faces
	int						miViewWidth;			//Record the view width
	int						miViewHeight;			//record the view height
	int						miFaceVertexesCountMax;	//the possible maximum number of vertexes count of all faces
	int						miFacesCountMax;		//the possible maximum number of all faces

	int						miVertexesCountMaxInStencil;//the possible maximum number of vertexes count of all faces in the stencil buffer
	DrawingElement			mFacesInStencil;			//Draw the faces in the stencil buffer
	DrawingElement			mPatternForFilledFace;		//Patterns such as cross lines

	DrawingElement			mDistortion2DElement;	//Draw element of the angle or area distortion in the 2d view after applying peeling

	float					mBkgColorRed;				//background color red value,its range is[0,1]
	float					mBkgColorGreen;				//background color green value,its range is[0,1]
	float					mBkgColorBlue;				//background color blue value,its range is[0,1]

	int						mSelFacesFillMode;			//indicate the fill mode when some polygons are selected

	void PaintBackground();
	void PaintGrid();
	void PaintVertexInfluence();
	void PaintVertexTicks();
	void GetPeelModeFaces(UnwrapMod* unwrapMod, MeshTopoData* md, BitArray& peelFaces);
	void PaintTick(DrawingElement& targetLines,float x, float y, BOOL largeTick,Point4& tickColor);
	void PaintEdges();
	void PaintEdgeDistortion();
	void PaintFaces();
	void PaintPelt();
	void PaintX(DrawingElement& targetLines,float x, float y, float size,Point4& color);
	void PaintFreeFormGizmo();

	void DrawEdge(DrawingElement& targetLines,int va,int vb, 
		Point2& pa, Point2& pb, Point2& pva, Point2& pvb,Point4& edgeColor,int iwidth = 0,bool bDot = false);

	void DrawRectangle(DrawingElement& targetLines,float left,float top,float right,float bottom,Point4& rectangleColor,int iwidth = 0);	

	void DrawSingleDotEdge(DrawingElement& targetLines,float startX, float startY,float endX,float endY,Point4& edgeColor,int iwidth = 0);
	void DrawSingleEdge(DrawingElement& targetLines,float startX, float startY,float endX,float endY,Point4& edgeColor,int iwidth = 0);
	void DrawFilledTriangle(DrawingElement& targetTriangles,Point2& pa, Point2& pb,Point2& pc,Point4& triColor);

	void PreparePaintFacesData();
	void PrepareFacesInStencil(int iVertexCount);
	double isLeft(Point3 P0, Point3 P1, Point3 P2);
	void DrawPatternFaces(MaxSDK::Graphics::IPrimitiveRenderer& renderer);
	void DrawOutlineOfCurrentFace(int iCurrentPolygonVertexCount, bool bDrawPreview = false);

	//change the position in the windows into the NDC(Normalized Device Coordinate) whose range is [-1,1]
	float ClientXToNDCX(const int x,int width);
	float ClientYToNDCY(const int y,int height);

	void CommitLineList(MaxSDK::Graphics::IPrimitiveRenderer& renderer,DrawingElement& dElement);
	void CommitTriangleList(MaxSDK::Graphics::IPrimitiveRenderer& renderer,DrawingElement& dElement,bool bRequireTransparantStatus = false);

	//compute the angle delta between the uvw and geometry value, then the delta will be used for the different color.
	float ComputeAngleDelta(float tAngle,float gAngle,bool& bPositive);
	//Based on the angle or area delta to compute the display color
	Point4 AdjustColorValueByAistortion(bool bPositive,float fDelta);
	//Input the position and color data to make the distortion drawing element aware.
	void FillDistortionFaceData(DrawingElement& targetElement,
		Point3& posA,Point3& posB,Point3& posC,
		Point4& colA,Point4& colB,Point4& colC);


	UnwrapMod* GetUnwrapModPtr();
	
	//Compute the tiles that contain all vertices
	void ComputeTiles( bool forceCompute = false );
	//By using the source image data from the check material,multi-tile material or others, make the texture handle valid
	void FillBKTextureHandleData(MaxSDK::Graphics::TextureHandle& targetTextureHandle,UBYTE* sourceImage);

	void paintTilesBackground(MaxSDK::Graphics::IPrimitiveRenderer& renderer);
	void paintSingleTileBackground(MaxSDK::Graphics::IPrimitiveRenderer& renderer,int iUStart,int iVStart);

	Point4 GammaCorrect(const Point4& inputColor) const;

	void SetTransparentBlendStatus(MaxSDK::Graphics::TargetBlendState& blendStatus);

	//Based on the ratio of the cluster's geometry and UV, compute every face's area ratio
	void ComputeClusterFaceAreaRatio(MeshTopoData *md, std::vector<float>& everyFaceClusterAreaRatio);
};