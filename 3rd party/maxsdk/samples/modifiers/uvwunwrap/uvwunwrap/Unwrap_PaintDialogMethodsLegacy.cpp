/********************************************************************** *<
FILE: Unwrap_PaintDialogMethods.cpp

DESCRIPTION: A UVW map modifier unwraps the UVWs onto the image dialog paint methdos

HISTORY: 9/26/2006
CREATED BY: Peter Watje


*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/

#include "unwrap.h"
#include "modsres.h"

#define SAFE_RELEASE(P) if(P){P->Release() ; P = NULL ;}

static inline  void RectNoFill(HDC hdc,Point2 a, Point2 b)
{
	MoveToEx(hdc,(int)a.x, (int)a.y, NULL);
	LineTo(hdc,(int)b.x, (int)a.y);
	LineTo(hdc,(int)b.x, (int)b.y);
	LineTo(hdc,(int)a.x, (int)b.y);
	LineTo(hdc,(int)a.x, (int)a.y);
}

static inline  void MiniRect(HDC hdc,Point2 a, int offset)
{
	MoveToEx(hdc,(int)a.x-offset, (int)a.y-offset, NULL);
	LineTo(hdc,(int)a.x+offset, (int)a.y-offset);
	LineTo(hdc,(int)a.x+offset, (int)a.y+offset);
	LineTo(hdc,(int)a.x-offset, (int)a.y+offset);
	LineTo(hdc,(int)a.x-offset, (int)a.y-offset);
}

static inline  void IMiniRect(HDC hdc,IPoint2 a, int offset)
{
	MoveToEx(hdc,a.x-offset, a.y-offset, NULL);
	LineTo(hdc,a.x+offset, a.y-offset);
	LineTo(hdc,a.x+offset, a.y+offset);
	LineTo(hdc,a.x-offset, a.y+offset);
	LineTo(hdc,a.x-offset, a.y-offset);
}

Painter2DLegacy::Painter2DLegacy()
{	
	iBuf   = NULL;
	iTileBuf   = NULL;
}

Painter2DLegacy::~Painter2DLegacy()
{
	DestroyOffScreenBuffer();
}

void Painter2DLegacy::DestroyOffScreenBuffer()
{
	if(iBuf)
	{
		DestroyIOffScreenBuf(iBuf); 
		iBuf   = NULL;
	}
	
	if(iTileBuf)
	{
		DestroyIOffScreenBuf(iTileBuf); 
		iTileBuf   = NULL;
	}
}

Painter2DLegacy& Painter2DLegacy::Singleton()
{
	static Painter2DLegacy s_Painter2D;
	return s_Painter2D;
}

void Painter2DLegacy::SetUnwrapMod(UnwrapMod* pUnwrapMod)
{
	mUnwrapModRefMaker.SetRef(pUnwrapMod);

	if(NULL == iBuf)
	{
		iBuf = CreateIOffScreenBuf(pUnwrapMod->hView);
		iBuf->SetBkColor(pUnwrapMod->backgroundColor);
	}
	
	if(NULL == iTileBuf)
	{
		iTileBuf = CreateIOffScreenBuf(pUnwrapMod->hView);
		iTileBuf->SetBkColor(pUnwrapMod->backgroundColor);
	}
}

UnwrapMod* Painter2DLegacy::GetUnwrapModPtr()
{ 
	return dynamic_cast<UnwrapMod*>(mUnwrapModRefMaker.GetRef());
}

void Painter2DLegacy::DrawEdge(HDC hdc, /*w4int a,int b,*/ int vecA,int vecB, 
						 IPoint2 pa, IPoint2 pb, IPoint2 pvecA, IPoint2 pvecB)
{
	if ((vecA ==-1) || (vecB == -1))
	{
		MoveToEx(hdc,pa.x, pa.y, NULL);
		LineTo(hdc,pb.x, pb.y);
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

		MoveToEx(hdc,pa.x, pa.y, NULL);
		for (int i = 1; i < 7; i++)
		{
			float t = float (i)/7.0f;

			float s = (float)1.0-t;
			float t2 = t*t;
			Point2 p = ( ( s*fpa + (3.0f*t)*fpvecA)*s + (3.0f*t2)* fpvecB)*s + t*t2*fpb;
			LineTo(hdc,(int)p.x, (int)p.y);
		}
		LineTo(hdc,pb.x, pb.y);
	}
}

void Painter2DLegacy::PaintBackground(HDC hdc)
{
	int i1, i2;
	GetUnwrapModPtr()->GetUVWIndices(i1,i2);

	//watje tile
	float xzoom, yzoom;
	int width,height;
	GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

	HDC tilehdc = iTileBuf->GetDC();
	if (!GetUnwrapModPtr()->tileValid)
	{
		iTileBuf->Erase();
		GetUnwrapModPtr()->tileValid = TRUE;
		Point3 p(0.0f,0.0f,0.0f);
		p[i2] = 1.0f;
		Point2 sp;
		sp = GetUnwrapModPtr()->UVWToScreen(p,xzoom,yzoom,width,height);
		Rect dest;
		dest.left   = (int)sp.x;
		dest.top    = (int)sp.y;
		dest.right  = dest.left + int(xzoom)-1;
		dest.bottom = dest.top + int(yzoom)-1;
		Rect src;
		src.left   = src.top = 0;
		src.right  = GetUnwrapModPtr()->iw;
		src.bottom = GetUnwrapModPtr()->ih;

		Point2 spCenter;
		Point3 pCenter(0.0f,0.0f,0.0f);

		spCenter = GetUnwrapModPtr()->UVWToScreen(pCenter,xzoom,yzoom,width,height);

		int iTileLimit = GetUnwrapModPtr()->iTileLimit;
		if (!GetUnwrapModPtr()->fnGetTile()) iTileLimit = 0;
		//compute startx
		//compute starty
		while (sp.x > 0)
			sp.x -= xzoom;
		while (sp.y > 0)
			sp.y -= yzoom;



		int centerX,centerY;
		centerX = (int)spCenter.x;
		centerY = (int)spCenter.y;
		int xMinLimit,yMinLimit;
		int xMaxLimit,yMaxLimit;

		xMinLimit = centerX - ((int)xzoom * (iTileLimit));
		yMinLimit = (height-centerY) - ((int)yzoom * (iTileLimit));


		xMaxLimit = centerX + ((int)xzoom * (iTileLimit+1)) ;
		yMaxLimit = (height-centerY) + ((int)yzoom * (iTileLimit+1));

		int cxMinLimit,cyMinLimit;
		int cxMaxLimit,cyMaxLimit;

		cxMinLimit = centerX - ((int)xzoom * (0));
		cyMinLimit = (height-centerY) - ((int)yzoom * (0));


		cxMaxLimit = centerX + ((int)xzoom) ;
		cyMaxLimit = (height-centerY) + ((int)yzoom );



		Point3 backColor;
		backColor.x =  GetRValue(GetUnwrapModPtr()->backgroundColor);
		backColor.y =  GetGValue(GetUnwrapModPtr()->backgroundColor);
		backColor.z =  GetBValue(GetUnwrapModPtr()->backgroundColor);

		int *xscaleList = new int[width];
		int *yscaleList = new int[height];

		float cx = sp.x;
		while (cx < 0.0f)
			cx += xzoom;
		float cy = sp.y;
		cy *= -1.0f;
		while (cy < 0.0f)
			cy += yzoom;



		float xinc = GetUnwrapModPtr()->iw/(xzoom);
		float yinc = GetUnwrapModPtr()->ih/(yzoom);

		float c = (xzoom - cx);							
		c = (c * GetUnwrapModPtr()->iw)/xzoom;
		//compute x scale
		int id;
		for (int tix = 0; tix < width; tix++)
		{

			id = ((int)c)%GetUnwrapModPtr()->iw;
			xscaleList[tix] = ((int)c)%GetUnwrapModPtr()->iw;
			c+=xinc;
		}
		c = yzoom - cy;	
		c -= height - yzoom;
		c = (c * GetUnwrapModPtr()->ih)/yzoom;
		//compute x scale

		for (int tix = 0; tix < height; tix++)
		{

			id = ((int)c)%GetUnwrapModPtr()->ih;
			if (id < 0)
			{
				id = GetUnwrapModPtr()->ih+id;
			}

			yscaleList[tix] = id;
			c+=yinc;
		}

		UBYTE *imageScaled = new UBYTE[ByteWidth(width)*(height)];
		UBYTE *p1 = imageScaled;
		float contrast = GetUnwrapModPtr()->fContrast;

		for (int iy = 0; iy < height; iy++)
		{
			int y = yscaleList[iy];

			UBYTE *p2 = GetUnwrapModPtr()->image +  (y*ByteWidth(GetUnwrapModPtr()->iw));
			p1 = imageScaled + (iy*ByteWidth(width));
			for (int ix = 0; ix < width; ix++)
			{
				int x = xscaleList[ix];

				if ((ix > xMaxLimit) || (ix < xMinLimit) ||
					(iy > yMaxLimit) || (iy < yMinLimit) 
					)
				{
					*p1++ = backColor.z;
					*p1++ = backColor.y;
					*p1++ = backColor.x;	
				}
				else if (GetUnwrapModPtr()->fnGetTile())
				{

					if ( (!GetUnwrapModPtr()->brightCenterTile) &&
						((ix <= cxMaxLimit) && (ix >= cxMinLimit) &&
						(iy <= cyMaxLimit) && (iy >= cyMinLimit) ) )
					{
						*p1++ = p2[x*3];
						*p1++ = p2[x*3+1];
						*p1++ = p2[x*3+2];

					}
					else
					{
						if (GetUnwrapModPtr()->blendTileToBackGround)
						{
							*p1++ = (UBYTE) (backColor.z + ( (float)p2[x*3]-backColor.z) * contrast);
							*p1++ = (UBYTE) (backColor.y + ( (float)p2[x*3+1]-backColor.y) * contrast);
							*p1++ = (UBYTE) (backColor.x + ( (float)p2[x*3+2]-backColor.x) * contrast);
						}
						else
						{	
							*p1++ = (UBYTE) ((float)p2[x*3] * contrast);
							*p1++ = (UBYTE) ((float)p2[x*3+1] * contrast);
							*p1++ = (UBYTE) ((float)p2[x*3+2] * contrast);
						}


					}
				}
				else
				{
					if ((contrast == 1.0f) || (!GetUnwrapModPtr()->brightCenterTile))
					{
						*p1++ = p2[x*3];
						*p1++ = p2[x*3+1];
						*p1++ = p2[x*3+2];
					}
					else if (GetUnwrapModPtr()->blendTileToBackGround)
					{
						*p1++ = (UBYTE) (backColor.z + ( (float)p2[x*3]-backColor.z) * contrast);
						*p1++ = (UBYTE) (backColor.y + ( (float)p2[x*3+1]-backColor.y) * contrast);
						*p1++ = (UBYTE) (backColor.x + ( (float)p2[x*3+2]-backColor.x) * contrast);
					}
					else
					{	
						*p1++ = (UBYTE) ((float)p2[x*3] * contrast);
						*p1++ = (UBYTE) ((float)p2[x*3+1] * contrast);
						*p1++ = (UBYTE) ((float)p2[x*3+2] * contrast);
					}

				}
			}
		}
		dest.left   = 0;
		dest.top    = 0;
		dest.right  =  width;
		dest.bottom =  height;
		GetGPort()->DisplayMap(tilehdc, dest, 0,0,  imageScaled, ByteWidth(width));

		if (imageScaled)
			delete [] imageScaled;
		if (xscaleList)
			delete [] xscaleList;
		if (yscaleList)
			delete [] yscaleList;

	}
	BitBlt(hdc,0,0,width,height,tilehdc,0,0,SRCCOPY);
}

void Painter2DLegacy::PaintGrid(HDC hdc)
{
	//draw grid
	int i1, i2;
	GetUnwrapModPtr()->GetUVWIndices(i1,i2);
	float xzoom, yzoom;
	int width,height;
	GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

	if ((GetUnwrapModPtr()->gridVisible) && (GetUnwrapModPtr()->gridSize>0.0f))
	{
		Point3 pz(0.0f,0.0f,0.0f);
		pz[i1] = GetUnwrapModPtr()->gridSize;

		Point2 checkp = GetUnwrapModPtr()->UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);
		Point2 checkp2 = GetUnwrapModPtr()->UVWToScreen(pz,xzoom,yzoom,width,height);

		if ((checkp2.x - checkp.x) > 4.5f)
		{
			HPEN gridPen = CreatePen(PS_SOLID,1,GetUnwrapModPtr()->gridColor);
			SelectObject(hdc,gridPen);

			//do x++
			float x = 0.0f;
			int iX = -1;
			while (iX  < width) 
			{
				//						Point3 p1(x,0.0f,0.0f);
				Point3 p1(0.0f,0.0f,0.0f);
				p1[i1] = x;

				Point2 p = GetUnwrapModPtr()->UVWToScreen(p1,xzoom,yzoom,width,height);

				int dX = int(p.x); 

				if (dX != iX)
				{
					MoveToEx(hdc,dX, 0, NULL);
					LineTo(hdc,dX, height);						
				}

				iX = dX;

				x+= GetUnwrapModPtr()->gridSize;
			}

			//do x--
			iX = 1;
			x = 0.0f;
			while (iX  > 0) 
			{

				//						Point3 p1(x,0.0f,0.0f);
				Point3 p1(0.0f,0.0f,0.0f);
				p1[i1] = x;

				Point2 p = GetUnwrapModPtr()->UVWToScreen(p1,xzoom,yzoom,width,height);

				int dX = int(p.x); 

				if (dX != iX)
				{
					MoveToEx(hdc,dX, 0, NULL);
					LineTo(hdc,dX, height);						
				}

				iX = dX;

				x-= GetUnwrapModPtr()->gridSize;
			}

			//do y++
			float y = 0.0f;
			int iY = -1;
			while (iY  < height) 
			{

				//						Point3 p1(0.0f,y,0.0f);
				Point3 p1(0.0f,0.0f,0.0f);
				p1[i2] = y;

				Point2 p = GetUnwrapModPtr()->UVWToScreen(p1,xzoom,yzoom,width,height);

				int dY = int(p.y); 

				if (dY != iY)
				{
					MoveToEx(hdc,0,dY, NULL);
					LineTo(hdc,width, dY);						
				}

				iY = dY;

				y-= GetUnwrapModPtr()->gridSize;
			}

			//do x--
			iY = 1;
			y = 0.0f;
			while (iY  > 0) 
			{

				//						Point3 p1(0.0f,y,0.0f);
				Point3 p1(0.0f,0.0f,0.0f);
				p1[i2] = y;

				Point2 p = GetUnwrapModPtr()->UVWToScreen(p1,xzoom,yzoom,width,height);

				int dY = int(p.y); 

				if (dY != iY)
				{
					MoveToEx(hdc,0,dY, NULL);
					LineTo(hdc,width, dY);						
				}

				iY = dY;

				y+= GetUnwrapModPtr()->gridSize;
			}
			SelectObject(hdc,GetStockObject(WHITE_PEN));
			DeleteObject(gridPen);

		}
	}

	//this just draws our bounding rectangle 0 to 1 
	//and extends the x/y lines
	int gr,gg,gb;
	gr = (int) (GetRValue(GetUnwrapModPtr()->gridColor) *0.45f);
	gg = (int) (GetGValue(GetUnwrapModPtr()->gridColor) *0.45f);
	gb = (int) (GetBValue(GetUnwrapModPtr()->gridColor) *0.45f);
	HPEN gPen = CreatePen(PS_SOLID,3,RGB(gr,gg,gb));
	Point3 p1(0.0f,0.0f,0.0f),p2(0.0f,0.0f,0.0f);
	Point2 sp1, sp2;
	p2[i1] = 1.0f;
	p2[i2] = 1.0f;
	sp1 = GetUnwrapModPtr()->UVWToScreen(p1,xzoom,yzoom,width,height);
	sp2 = GetUnwrapModPtr()->UVWToScreen(p2,xzoom,yzoom,width,height);			
	SelectObject(hdc,gPen);
	Rectangle(hdc,(int)sp1.x,(int)sp2.y,(int)sp2.x+1,(int)sp1.y);

	MoveToEx(hdc,(int)sp1.x, (int)0, NULL);
	LineTo(hdc,(int)sp1.x, (int)sp1.y);

	MoveToEx(hdc,(int)sp1.x, (int)sp1.y, NULL);
	LineTo(hdc,(int)width, (int)sp1.y);

	SelectObject(hdc,GetStockObject(WHITE_PEN));
	DeleteObject(gPen);
}

void Painter2DLegacy::PaintTick(HDC hdc, int x, int y, BOOL largeTick)
{

	if (largeTick)
	{
		if (GetUnwrapModPtr()->tickSize==1)
		{
			MoveToEx(hdc,x-1,y-1, NULL);
			LineTo(hdc,x,y);						
		}
		else Rectangle(hdc,
			x-GetUnwrapModPtr()->tickSize+1,y-GetUnwrapModPtr()->tickSize+1,
			x+GetUnwrapModPtr()->tickSize+1,y+GetUnwrapModPtr()->tickSize+1);		
	}
	else Rectangle(hdc,
		x-1,y-1,
		x+1,y+1);		
}

void Painter2DLegacy::PaintX(HDC hdc, int x, int y, int size)
{
	MoveToEx(hdc,x-size,y-size, NULL);
	LineTo(hdc,x+size,y+size);						

	MoveToEx(hdc,x+size,y-size, NULL);
	LineTo(hdc,x-size,y+size);	

	Rectangle(hdc,
		x-size,y-size,
		x+size,y+size);
}

void Painter2DLegacy::PaintVertexTicks(HDC hdc)
{
	//get our soft selection colors
	Point3 selSoft = GetUIColor(COLOR_SUBSELECTION_SOFT);
	Point3 selMedium = GetUIColor(COLOR_SUBSELECTION_MEDIUM);
	Point3 selHard = GetUIColor(COLOR_SUBSELECTION_HARD);


	int frozenr = (int) (GetRValue(GetUnwrapModPtr()->lineColor)*0.5f);
	int frozeng = (int) (GetGValue(GetUnwrapModPtr()->lineColor)*0.5f);
	int frozenb = (int) (GetBValue(GetUnwrapModPtr()->lineColor)*0.5f);
	COLORREF frozenColor = RGB(frozenr,frozeng,frozenb);

	HPEN selPen   = CreatePen(PS_SOLID,2,GetUnwrapModPtr()->selColor);
	HPEN frozenPen = CreatePen(PS_SOLID,0,frozenColor);
	HPEN unselPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->lineColor);
	HPEN sharedPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->sharedColor);

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
		SelectObject(hdc,selPen);
		for (int i=0; i< ld->GetNumberTVVerts(); i++) 
		{
			//PELT
			if (ld->IsTVVertVisible(i) && ld->GetTVVertSelected(i))

			{

				int x  = ld->GetTransformedPoint(i).x;
				int y  = ld->GetTransformedPoint(i).y;
				PaintTick(hdc, x,y,TRUE);

				if ( (GetUnwrapModPtr()->showVertexClusterList) )
				{

					int vCluster = ld->GetTVVertGeoIndex(i);//vertexClusterList[i];
					if ((vCluster >=0) && (usedClusters[vCluster]))
					{

						TSTR vertStr;
						vertStr.printf(_T("%d"),vCluster);
						TextOut(hdc, x+4,y-4,vertStr,vertStr.Length());
					}

				}

			}
		}
		SelectObject(hdc,GetStockObject(BLACK_PEN));

		IPoint2 ipt[4];
		//now draw our unselected 
		SelectObject(hdc,unselPen);


		int holdSize = GetUnwrapModPtr()->tickSize;
		GetUnwrapModPtr()->tickSize = 1;
		for (int i=0; i< ld->GetNumberTVVerts(); i++) 
		{

			if (ld->IsTVVertVisible(i) && (!ld->GetTVVertSelected(i)))
			{
				Point3 selColor(0.0f,0.0f,0.0f);

				ipt[0] = ld->GetTransformedPoint(i);


				float influence = ld->GetTVVertInfluence(i);

				if (influence == 0.0f)
				{

					int x  = ld->GetTransformedPoint(i).x;
					int y  = ld->GetTransformedPoint(i).y;
					PaintTick(hdc, x,y,TRUE);


					if (GetUnwrapModPtr()->showShared) 
					{
						int vCluster = ld->GetTVVertGeoIndex(i);//vertexClusterList[i];
						if ((vCluster >=0) && (usedClusters[vCluster]))
						{
							SelectObject(hdc,sharedPen);

							GetUnwrapModPtr()->tickSize = holdSize;
							PaintTick(hdc, x,y,TRUE);
							GetUnwrapModPtr()->tickSize = 1;

						}

					}
					if (GetUnwrapModPtr()->showVertexClusterList) 
					{
						int vCluster = ld->GetTVVertGeoIndex(i);//vertexClusterList[i];
						if ((vCluster >=0) && (usedClusters[vCluster]))
						{

							TSTR vertStr;
							vertStr.printf(_T("%d"),vCluster);
							TextOut(hdc, x+4,y-4,vertStr,vertStr.Length());
						}
					}

				}
			}
		}
		GetUnwrapModPtr()->tickSize = holdSize;

		//draw the weighted ticks
		for (int i=0; i< ld->GetNumberTVVerts(); i++) 
		{
			//PELT
			if (ld->IsTVVertVisible(i) && (!ld->GetTVVertSelected(i)))
			{
				Point3 selColor(0.0f,0.0f,0.0f);

				ipt[0] = ld->GetTransformedPoint(i);


				float influence = ld->GetTVVertInfluence(i);

				if (influence == 0.0f)
				{
				}
				else if (influence <0.5f)
					selColor = selSoft + ( (selMedium-selSoft) * (influence/0.5f));
				else if (influence<=1.0f)
					selColor = selMedium + ( (selHard-selMedium) * ((influence-0.5f)/0.5f));

				COLORREF inflColor;
				inflColor = RGB((int)(selColor.x * 255.f), (int)(selColor.y * 255.f),(int)(selColor.z * 255.f));

				HPEN inflPen = CreatePen(PS_SOLID,0,inflColor);
				BOOL largeTick = FALSE;
				if (ld->GetTVVertFrozen(i))
					SelectObject(hdc,frozenPen);
				else if (influence == 0.0f)
					SelectObject(hdc,unselPen);
				else 
				{
					SelectObject(hdc,inflPen);
					largeTick = TRUE;
				}

				int x  = ld->GetTransformedPoint(i).x;
				int y  = ld->GetTransformedPoint(i).y;
				PaintTick(hdc, x,y,largeTick);

				SelectObject(hdc,GetStockObject(BLACK_PEN));
				DeleteObject(inflPen);
			}
		}
	}

	SelectObject(hdc,GetStockObject(BLACK_PEN));
	DeleteObject(selPen);
	DeleteObject(frozenPen);
	DeleteObject(unselPen);
	DeleteObject(sharedPen);
}

void Painter2DLegacy::GetPeelModeFaces(UnwrapMod* unwrapMod, MeshTopoData* md, BitArray& peelFaces)
{
	if (md == nullptr)
		return;
	peelFaces.SetSize(md->GetNumberFaces());
	//grab the data from the peel mode tool
	LSCMLocalData *ld = unwrapMod->mLSCMTool.GetData(md);
	if (ld != nullptr)
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

void Painter2DLegacy::PaintEdges(HDC hdc)
{
	auto unwrapMod = GetUnwrapModPtr();

	int frozenr = (int) (GetRValue(unwrapMod->lineColor)*0.5f);
	int frozeng = (int) (GetGValue(unwrapMod->lineColor)*0.5f);
	int frozenb = (int) (GetBValue(unwrapMod->lineColor)*0.5f);
	COLORREF frozenColor = RGB(frozenr,frozeng,frozenb);
	HPEN selPen   = CreatePen(PS_SOLID,2,unwrapMod->selColor);
	HPEN selThinPen   = CreatePen(PS_DOT,1,unwrapMod->selColor);
	HPEN frozenPen = CreatePen(PS_SOLID,0,frozenColor);
	HPEN openEdgePen = CreatePen(PS_SOLID,0,unwrapMod->openEdgeColor);
	HPEN handlePen = CreatePen(PS_SOLID,0,unwrapMod->handleColor);
	HPEN sharedPen = CreatePen(PS_SOLID,0,unwrapMod->sharedColor);
	HPEN peelPen = CreatePen(PS_SOLID, 0, unwrapMod->peelColor);

	BOOL showLocalDistortion = 0;
	TimeValue t = GetCOREInterface()->GetTime();
	unwrapMod->pblock->GetValue(unwrap_localDistorion,t,showLocalDistortion,FOREVER);

	//are we in peel mode
	BOOL bLivePeel = unwrapMod->GetLivePeelModeEnabled();

	for (int whichLD = 0; whichLD < unwrapMod->mMeshTopoData.Count(); whichLD++)
	{
		MeshTopoData *md = unwrapMod->mMeshTopoData[whichLD];
		Point3 lColor((float)GetRValue(unwrapMod->lineColor)/255.0f,(float)GetGValue(unwrapMod->lineColor)/255.0f,(float)GetBValue(unwrapMod->lineColor)/255.0f);
		Point3 nColor = unwrapMod->mMeshTopoData.GetNodeColor(whichLD);

		if ( unwrapMod->mMeshTopoData.Count() > 1)
			lColor = lColor + (nColor - lColor) * 0.75f;

		COLORREF penColor = RGB((int)(lColor.x * 255.0f),(int)(lColor.y * 255.0f),(int)(lColor.z * 255.0f));
		HPEN unselPen = CreatePen(PS_SOLID,0,penColor);


		//build our shared list
		BitArray usedClusters;
		if (unwrapMod->showVertexClusterList || unwrapMod->showShared) 
		{
			usedClusters.SetSize(md->GetNumberGeomVerts());
			usedClusters.ClearAll();


			md->TransferSelectionStart(unwrapMod->fnGetTVSubMode());
			for (int i = 0; i < md->GetNumberTVVerts(); i++)
			{
				if (md->GetTVVertSelected(i))
				{
					int gIndex = md->GetTVVertGeoIndex(i);
					if (gIndex >= 0) //need this check since a tv vert can not be attached to a geo vert
						usedClusters.Set(gIndex);
				}
			}
			md->TransferSelectionEnd(unwrapMod->fnGetTVSubMode(),FALSE,FALSE);
		}

		//if we are in peel mode, grab the faces
		BitArray peelFaces;
		if (bLivePeel)
		{
			GetPeelModeFaces(unwrapMod, md, peelFaces);
		}

		//draw regular edges
		for (int i=0; i< md->GetNumberTVEdges(); i++) 
		{
			int a = md->GetTVEdgeVert(i,0);
			int b = md->GetTVEdgeVert(i,1);

			BOOL sharedEdge = FALSE;
			if (unwrapMod->showShared)
			{
				int ct =0;
				int vCluster = md->GetTVVertGeoIndex(a);//vertexClusterList[a];
				if ((vCluster >=0) && (usedClusters[vCluster])) 
					ct++;

				vCluster = md->GetTVVertGeoIndex(b);//vertexClusterList[b];
				if ((vCluster >=0) && (usedClusters[vCluster])) 
					ct++;
				if (ct ==2) 
					sharedEdge = TRUE;

			}

			BOOL paintEdge = TRUE;
			BOOL hiddenEdge = FALSE;
			int numberFacesAtThisEdge = md->GetTVEdgeNumberTVFaces(i);
			for (int j = 0; j < md->GetTVEdgeNumberTVFaces(i); j++)
			{
				int faceIndex = md->GetTVEdgeConnectedTVFace(i,j);
				int deg = md->GetFaceDegree(faceIndex);
				for (int k = 0; k < deg; k++)
				{				
					int a = k;
					int b = (k+1)%deg;
					if (md->GetFaceTVVert(faceIndex,a) == md->GetFaceTVVert(faceIndex,b))
					{
						numberFacesAtThisEdge--;
						k = deg;
					}
				}

			}
			if ( (!unwrapMod->displayHiddenEdges) && ( numberFacesAtThisEdge > 1))
			{
				if (md->GetTVEdgeHidden(i))
				{
					hiddenEdge = TRUE;
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
			}

			if (paintEdge && md->IsTVVertVisible(a) && md->IsTVVertVisible(b))
			{
				//We are drawing this edge, now determine what color to draw it as:

				//If peel mode is on, determine if this edge is related to a peelmode face
				bool aPeelModeFace = false;
				if (bLivePeel)
				{
					//Go through all the faces that this edge is connected to and try to find one that is
					//'in' peel mode
					for (int m = 0; m < md->GetTVEdgeNumberTVFaces(i); m++)
					{
						//try to find a face that is in peel mode
						int polyIndex = md->GetTVEdgeConnectedTVFace(i, m);
						if (peelFaces[polyIndex])
						{
							aPeelModeFace = true;
							break;
						}
					}
				}
				
				int veca = md->GetTVEdgeVec(i,0);
				int vecb = md->GetTVEdgeVec(i,1);

				if(aPeelModeFace) //if in peel mode and this edge is marked
				{
					//draw peel mode edge
					SelectObject(hdc, peelPen);
					DrawEdge(hdc, veca, vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
				}
				//draw shared edges
				else if ((unwrapMod->mTVSubObjectMode == TVEDGEMODE) && (hiddenEdge)&& (md->GetTVEdgeSelected(i)))
				{
					SelectObject(hdc,selThinPen);
					DrawEdge(hdc,  veca,vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
				}
				else if ((unwrapMod->mTVSubObjectMode == TVEDGEMODE) && (md->GetTVEdgeSelected(i)))
				{
					SelectObject(hdc,selPen);
					DrawEdge(hdc,  veca,vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
				}
				else if (sharedEdge && (!(md->GetTVVertSelected(a) && md->GetTVVertSelected(b))))
				{
					SelectObject(hdc,sharedPen);
					DrawEdge(hdc,  veca,vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
				}
				//draw open edges
				else if ((numberFacesAtThisEdge == 1) && (unwrapMod->displayOpenEdges))
				{
					SelectObject(hdc,openEdgePen);
					DrawEdge(hdc,  veca,vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
				}
				else if ((md->GetTVVertFrozen(a)) || (md->GetTVVertFrozen(b)))
				{
					SelectObject(hdc,frozenPen);
					DrawEdge(hdc,  veca,vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
				}
				//draw normal edge
				else 
				{
					if (!unwrapMod->showEdgeDistortion )
					{
						SelectObject(hdc,unselPen);
						DrawEdge(hdc,  veca,vecb, md->GetTransformedPoint(a), md->GetTransformedPoint(b), md->GetTransformedPoint(veca), md->GetTransformedPoint(vecb));
					}
				}
				//draw handles			
				if ((veca!= -1) && (vecb!= -1))
				{
					SelectObject(hdc,handlePen);
					DrawEdge(hdc,   -1,-1, md->GetTransformedPoint(b), md->GetTransformedPoint(vecb), md->GetTransformedPoint(b), md->GetTransformedPoint(vecb));
					DrawEdge(hdc,   -1,-1, md->GetTransformedPoint(a), md->GetTransformedPoint(veca), md->GetTransformedPoint(a), md->GetTransformedPoint(veca));
				}
			}
		}

		//draw our pinned as Xs
		SelectObject(hdc,sharedPen);
		for (int i=0; i< md->GetNumberTVVerts(); i++) 
		{
			if (md->IsTVVertVisible(i) && md->IsTVVertPinned(i))

			{
				int x  = md->GetTransformedPoint(i).x;
				int y  = md->GetTransformedPoint(i).y;
				PaintX(hdc, x,y,5);
			}
		}

		SelectObject(hdc,GetStockObject(BLACK_PEN));
		DeleteObject(unselPen);		
	}

	SelectObject(hdc,GetStockObject(BLACK_PEN));
	DeleteObject(selPen);
	DeleteObject(selThinPen);	
	DeleteObject(frozenPen);		

	DeleteObject(openEdgePen);		
	DeleteObject(handlePen);		
	DeleteObject(sharedPen);
}

//TODO: remove the peel mode code from here, handles in the edges code
void Painter2DLegacy::PaintFaces(HDC hdc)
{
	HPEN selPen   = CreatePen(PS_SOLID, 2, GetUnwrapModPtr()->selColor);
	HPEN selEdgePen = CreatePen(PS_SOLID, 2, GetUnwrapModPtr()->selColor);
	HPEN selEdgeThinPen = CreatePen(PS_DOT, 1, GetUnwrapModPtr()->selColor);
	//now do selected faces
	SelectObject(hdc,selPen);
	HBRUSH  selBrush = NULL;

	Point3 selP3Color;
	selP3Color.x = (int) GetRValue(GetUnwrapModPtr()->selColor);
	selP3Color.y = (int) GetGValue(GetUnwrapModPtr()->selColor);
	selP3Color.z = (int) GetBValue(GetUnwrapModPtr()->selColor);

	selP3Color.x += (255.0f - selP3Color.x) *0.25f;
	selP3Color.y += (255.0f - selP3Color.y) *0.25f;
	selP3Color.z += (255.0f - selP3Color.z) *0.25f;

	COLORREF selFillColor  = RGB(0, 0, 0);
	selFillColor  = RGB((int)selP3Color.x,(int)selP3Color.y,(int)selP3Color.z);

	if (GetUnwrapModPtr()->fnGetFillMode() != FILL_MODE_OFF)
	{
		if (GetUnwrapModPtr()->fnGetFillMode() != FILL_MODE_SOLID)
			SetBkMode( hdc, TRANSPARENT);

		if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_SOLID)
			selBrush = CreateSolidBrush(selFillColor);
		else if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_BDIAGONAL)
			selBrush = CreateHatchBrush(HS_BDIAGONAL, selFillColor );
		else if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_CROSS)
			selBrush = CreateHatchBrush(HS_CROSS, selFillColor );
		else if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_DIAGCROSS)
			selBrush = CreateHatchBrush(HS_DIAGCROSS, selFillColor );
		else if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_FDIAGONAL)
			selBrush = CreateHatchBrush(HS_FDIAGONAL, selFillColor );
		else if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_HORIZONAL)
			selBrush = CreateHatchBrush(HS_HORIZONTAL, selFillColor );
		else if (GetUnwrapModPtr()->fnGetFillMode() == FILL_MODE_VERTICAL)
			selBrush = CreateHatchBrush(HS_VERTICAL, selFillColor );

		if (selBrush) 
			SelectObject(hdc,selBrush);
	}

	int size = 0;
	for (int whichLD = 0; whichLD < GetUnwrapModPtr()->mMeshTopoData.Count(); whichLD++)
	{
		MeshTopoData *ld = GetUnwrapModPtr()->mMeshTopoData[whichLD];
		for (int i=0; i< ld->GetNumberFaces(); i++) 
		{
			if (ld->GetFaceDegree(i) > size)
				size = ld->GetFaceDegree(i);
		}
	}
	IPoint2 *ipt = new IPoint2[size];

	POINT *polypt = new POINT[size+(7*7)];

	if (selBrush)
	{	

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
					bDraw = ld->IsFaceVisible(i) && (GetUnwrapModPtr()->mTVSubObjectMode == TVFACEMODE) && (faceSelected || faceSelectedPreview) ;

					if (bDraw)
					{
						int pcount = 3;
						pcount = ld->GetFaceDegree(i);
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
								IPoint2 iin, ip, iout;
								int index = ld->GetFaceTVVert(i,j);
								ip = ld->GetTransformedPoint(index);
								pVis[j] = ld->IsTVVertVisible(index);

								index = ld->GetFaceTVHandle(i,j*2);
								iout = ld->GetTransformedPoint(index);
								if (j==0)
									index = ld->GetFaceTVHandle(i,pcount*2-1);
								else index = ld->GetFaceTVHandle(i,j*2-1);

								iin = ld->GetTransformedPoint(index);

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
									IPoint2 ip;
									int index = ld->GetFaceTVVert(i,j);
									ip = ld->GetTransformedPoint(index);
									MoveToEx(hdc,ip.x, ip.y, NULL);
									if ((j==0) && (selBrush))
									{
										polypt[polyct].x = ip.x;
										polypt[polyct++].y = ip.y;
									}

									for (int iu = 1; iu < 5; iu++)
									{
										float u = (float) iu/5.f;
										p = spl.InterpBezier3D(j, u);
										if (selBrush) 
										{
											polypt[polyct].x = (int)p.x;
											polypt[polyct++].y = (int)p.y;
										}
									}
									if (j == pcount-1)
										index = ld->GetFaceTVVert(i,0);
									else index = ld->GetFaceTVVert(i,j+1);
									ip = ld->GetTransformedPoint(index);
									if (selBrush) 
									{
										polypt[polyct].x = (int)ip.x;
										polypt[polyct++].y = (int)ip.y;
									}
								}
							}
							if (selBrush) 
								Polygon(hdc, polypt,  polyct);
						}
						else  //else it is just regular poly so just draw the straight edges
						{
							for (int j=0; j<pcount; j++) 
							{
								int index = ld->GetFaceTVVert(i,j);
								ipt[j] = ld->GetTransformedPoint(index); 
								if (ld->GetTVVertHidden(index)) hidden = TRUE;
							}
							// Now draw the face
							if (!hidden)
							{
								MoveToEx(hdc,ipt[0].x, ipt[0].y, NULL);
								if (selBrush) 
								{
									polypt[0].x = ipt[0].x;
									polypt[0].y = ipt[0].y;
								}
								for (int j=0; j<pcount; j++) 
								{
									if ((selBrush) && (j != (pcount-1)))
									{
										polypt[j+1].x = ipt[j+1].x;
										polypt[j+1].y = ipt[j+1].y;
									}

								}
								if (selBrush) 
									Polygon(hdc, polypt,  pcount);
							}
						}
					}

				}

			}
		}

	}
	delete [] polypt;
	delete [] ipt;
	if (GetUnwrapModPtr()->fnGetFillMode() != FILL_MODE_OFF)
	{
		if (selBrush) 
		{
			SelectObject(hdc,GetStockObject(WHITE_BRUSH));
			DeleteObject(selBrush);
		}
		SetBkMode( hdc, OPAQUE);
	}
	SelectObject(hdc,GetStockObject(BLACK_PEN));
	DeleteObject(selPen);
	DeleteObject(selEdgeThinPen);
	DeleteObject(selEdgePen);
}

void Painter2DLegacy::PaintFreeFormGizmo(HDC hdc)
{
	//need in rotation in rotation drag

	BOOL inFreeFormMode = FALSE;
	if (GetUnwrapModPtr()->mode == ID_FREEFORMMODE)
		inFreeFormMode = TRUE;

	if ( (inFreeFormMode) ||
		(GetUnwrapModPtr()->mode == ID_MOVE) ||
		(GetUnwrapModPtr()->mode == ID_ROTATE) ||
		(GetUnwrapModPtr()->mode == ID_SCALE) )
	{
		//		int count = 0;
		float xzoom, yzoom;
		int width,height;
		GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);
		int i1, i2;
		GetUnwrapModPtr()->GetUVWIndices(i1,i2);


		HPEN freeFormPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->freeFormColor);


		GetUnwrapModPtr()->TransferSelectionStart();

		int vselNumberSet = 0;
		GetUnwrapModPtr()->freeFormBounds.Init();
		for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
		{
			//			count = vsel.NumberSet();

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

		if (vselNumberSet > 0)
		{	
			SelectObject(hdc,freeFormPen);
			SetBkMode( hdc, TRANSPARENT);
			//draw center

			Point2 prect[4];
			Point2 prectNoExpand[4];
			Point2 pivotPoint;
			prect[0] = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->selCenter,xzoom,yzoom,width,height);
			pivotPoint = prect[0];
			prect[1] = prect[0];
			prect[0].x -= 2;
			prect[0].y -= 2;
			prect[1].x += 2;
			prect[1].y += 2;
			if (!GetUnwrapModPtr()->inRotation)
				RectNoFill(hdc, prect[0],prect[1]);

			//draw gizmo bounds
			prect[0] = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->freeFormBounds.pmin,xzoom,yzoom,width,height);
			prect[1] = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->freeFormBounds.pmax,xzoom,yzoom,width,height);
			float xexpand = 15.0f/xzoom;
			float yexpand = 15.0f/yzoom;

			BOOL expandedFFX = FALSE;
			BOOL expandedFFY = FALSE;

			prectNoExpand[0] = prect[0];
			prectNoExpand[1] = prect[1];
			prectNoExpand[2] = prect[2];
			prectNoExpand[3] = prect[3];

			if (!GetUnwrapModPtr()->freeFormMode->dragging)
			{
				if ((prect[1].x-prect[0].x) < 30)
				{
					prect[1].x += 15;
					prect[0].x -= 15;
					//expand bounds
					GetUnwrapModPtr()->freeFormBounds.pmax[i1] += xexpand;
					GetUnwrapModPtr()->freeFormBounds.pmin[i1] -= xexpand;
					expandedFFX = TRUE;
				}
				else expandedFFX = FALSE;
				if ((prect[0].y-prect[1].y) < 30)
				{
					prect[1].y -= 15;
					prect[0].y += 15;
					GetUnwrapModPtr()->freeFormBounds.pmax[i2] += yexpand;
					GetUnwrapModPtr()->freeFormBounds.pmin[i2] -= yexpand;
					expandedFFY = TRUE;
				}
				else expandedFFY = FALSE;

			}


			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode)
				RectNoFill(hdc, prect[0],prect[1]);
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

				GetUnwrapModPtr()->freeFormCornersScreenSpace[i] = prectNoExpand[i];
				if (!GetUnwrapModPtr()->inRotation && inFreeFormMode) MiniRect(hdc,prect[i],3);
			}
			Point2 centerEdge;
			centerEdge = (prect[0] + prect[1]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[0] = (GetUnwrapModPtr()->freeFormCorners[0] + GetUnwrapModPtr()->freeFormCorners[1]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[0] = centerEdge;
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode) MiniRect(hdc,centerEdge,3);

			centerEdge = (prect[1] + prect[2]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[2] = (GetUnwrapModPtr()->freeFormCorners[2] + GetUnwrapModPtr()->freeFormCorners[3]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[2] = centerEdge;
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode) MiniRect(hdc,centerEdge,3);

			centerEdge = (prect[2] + prect[3]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[3] = (GetUnwrapModPtr()->freeFormCorners[0] + GetUnwrapModPtr()->freeFormCorners[2]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[3] = centerEdge;
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode) MiniRect(hdc,centerEdge,3);

			centerEdge = (prect[3] + prect[0]) *0.5f;
			GetUnwrapModPtr()->freeFormEdges[1] = (GetUnwrapModPtr()->freeFormCorners[1] + GetUnwrapModPtr()->freeFormCorners[3]) *0.5f;
			GetUnwrapModPtr()->freeFormEdgesScreenSpace[1] = centerEdge;
			if (!GetUnwrapModPtr()->inRotation && inFreeFormMode) MiniRect(hdc,centerEdge,3);

			//draw pivot
			Point2 aPivot = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->freeFormPivotOffset,xzoom,yzoom,width,height);
			Point2 bPivot = GetUnwrapModPtr()->UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);

			pivotPoint = pivotPoint + ( aPivot-bPivot);
			GetUnwrapModPtr()->freeFormPivotScreenSpace = pivotPoint;

			MoveToEx(hdc,(int)pivotPoint.x-20, (int)pivotPoint.y, NULL);
			LineTo(hdc,(int)pivotPoint.x+20, (int)pivotPoint.y);
			MoveToEx(hdc,(int)pivotPoint.x, (int)pivotPoint.y-20, NULL);
			LineTo(hdc,(int)pivotPoint.x, (int)pivotPoint.y+20);
			SetBkMode( hdc, OPAQUE);
			if (GetUnwrapModPtr()->inRotation)
			{
				Point2 a = GetUnwrapModPtr()->UVWToScreen(tempCenter,xzoom,yzoom,width,height);
				Point2 b = GetUnwrapModPtr()->UVWToScreen(GetUnwrapModPtr()->origSelCenter,xzoom,yzoom,width,height);
				MoveToEx(hdc,(int)a.x, (int)a.y, NULL);
				LineTo(hdc,(int)pivotPoint.x, (int)pivotPoint.y);
				LineTo(hdc,(int)b.x, (int)b.y);

				TSTR rotAngleStr;
				rotAngleStr.printf(_T("%3.2f"),GetUnwrapModPtr()->currentRotationAngle);
				DLTextOut(hdc, (int)pivotPoint.x,(int)pivotPoint.y,rotAngleStr);
			}

		}
		GetUnwrapModPtr()->TransferSelectionEnd(FALSE,FALSE);

		SelectObject(hdc,GetStockObject(BLACK_PEN));
		DeleteObject(freeFormPen);
	}
}

void Painter2DLegacy::PaintPelt(HDC hdc)
{
	//PELT 
	//draw the mirror plane
	COLORREF yellowColor = RGB(255,255,64);
	HPEN selThinPen   = CreatePen(PS_DOT,1,GetUnwrapModPtr()->selColor);
	HPEN yellowPen = CreatePen(PS_SOLID,0,yellowColor);
	HPEN sharedPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->sharedColor);

	BOOL showLocalDistortion = 0;
	TimeValue t = GetCOREInterface()->GetTime();
	GetUnwrapModPtr()->pblock->GetValue(unwrap_localDistorion,t,showLocalDistortion,FOREVER);


	if (GetUnwrapModPtr()->peltData.peltDialog.hWnd && GetUnwrapModPtr()->peltData.mBaseMeshTopoDataCurrent)
	{

		float xzoom, yzoom;
		int width,height;
		GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

		MeshTopoData *ld = GetUnwrapModPtr()->peltData.mBaseMeshTopoDataCurrent;

		GetUnwrapModPtr()->peltData.mIsSpring.SetSize(ld->GetNumberTVEdges());//TVMaps.ePtrList.Count());
		GetUnwrapModPtr()->peltData.mIsSpring.ClearAll();

		GetUnwrapModPtr()->peltData.mPeltSpringLength.SetCount(ld->GetNumberTVEdges());//TVMaps.ePtrList.Count());

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


		//get the center
		Point3 ma,mb;
		ma = GetUnwrapModPtr()->peltData.GetMirrorCenter() + mirrorVec;
		mb = GetUnwrapModPtr()->peltData.GetMirrorCenter() - mirrorVec;
		//get our vec
		Point2 pa = GetUnwrapModPtr()->UVWToScreen(ma,xzoom,yzoom,width,height);
		Point2 pb = GetUnwrapModPtr()->UVWToScreen(mb,xzoom,yzoom,width,height);

		SelectObject(hdc,yellowPen);

		MoveToEx(hdc,(int)pa.x,(int) pa.y, NULL);
		LineTo(hdc,(int)pb.x,(int) pb.y);						

		ma = GetUnwrapModPtr()->peltData.GetMirrorCenter();
		mb = GetUnwrapModPtr()->peltData.GetMirrorCenter() - mirrorVeca;
		//get our vec
		pa = GetUnwrapModPtr()->UVWToScreen(ma,xzoom,yzoom,width,height);
		pb = GetUnwrapModPtr()->UVWToScreen(mb,xzoom,yzoom,width,height);

		MoveToEx(hdc,(int)pa.x,(int) pa.y, NULL);
		LineTo(hdc,(int)pb.x,(int) pb.y);

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

					SelectObject(hdc,selThinPen);
					MoveToEx(hdc,ld->GetTransformedPoint(a).x,ld->GetTransformedPoint(a).y, NULL);
					LineTo(hdc,ld->GetTransformedPoint(b).x,ld->GetTransformedPoint(b).y);	

					Point3 vec = ld->GetTVVert(b) - ld->GetTVVert(b);//(TVMaps.v[b].p-TVMaps.v[a].p);
					if (Length(vec) > 0.00001f)
					{
						vec = Normalize(vec)*GetUnwrapModPtr()->peltData.springEdges[i].dist*GetUnwrapModPtr()->peltData.springEdges[i].distPer;
						vec = ld->GetTVVert(b)/*TVMaps.v[b].p*/ - vec;

						Point3 tvPoint = GetUnwrapModPtr()->UVWToScreen(vec ,xzoom,yzoom,width,height);
						IPoint2 tvPoint2;
						tvPoint2.x  = (int) tvPoint.x;
						tvPoint2.y  = (int) tvPoint.y;

						SelectObject(hdc,sharedPen);
						MoveToEx(hdc,ld->GetTransformedPoint(b).x,ld->GetTransformedPoint(b).y,NULL);
						LineTo(hdc,tvPoint2.x,tvPoint2.y);	
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

			MoveToEx(hdc,ld->GetTransformedPoint(a).x,ld->GetTransformedPoint(a).y, NULL);
			LineTo(hdc,ld->GetTransformedPoint(b).x,ld->GetTransformedPoint(b).y);
		}
	}

	SelectObject(hdc,GetStockObject(BLACK_PEN));
	DeleteObject(selThinPen);
	DeleteObject(yellowPen);
	DeleteObject(sharedPen);
}

void Painter2DLegacy::PaintEdgeDistortion(HDC hdc)
{
	COLORREF goalColor = ColorMan()->GetColor(EDGEDISTORTIONCOLORID);
	COLORREF baseColor = ColorMan()->GetColor(EDGEDISTORTIONGOALCOLORID);

	Color goalc(goalColor);
	Color basec(baseColor);

	int gr,gg,gb;
	gr = (int) (255 * basec.r);
	gg = (int) (255 * basec.g);
	gb = (int) (255 * basec.b);
	HPEN gPen = CreatePen(PS_SOLID,1,RGB(gr,gg,gb));
	SelectObject(hdc,gPen);

	Tab<int> whichColor;
	Tab<float> whichPer;
	Tab<float> goalLength;
	Tab<float> currentLength;
	Tab<Point3> vec;

	float xzoom, yzoom;
	int width,height;
	GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);

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
			if (ld->GetTVEdgeHidden(edgeIndex))//TVMaps.ePtrList[edgeIndex]->flags & FLAG_HIDDENEDGEA)
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
				a = ld->GetTVEdgeVert(edgeIndex,0);//TVMaps.ePtrList[edgeIndex]->a;
				b = ld->GetTVEdgeVert(edgeIndex,1);//TVMaps.ePtrList[edgeIndex]->b;

				int va,vb;
				va = ld->GetTVEdgeGeomVert(edgeIndex,0);//TVMaps.ePtrList[edgeIndex]->ga;
				vb = ld->GetTVEdgeGeomVert(edgeIndex,1);//TVMaps.ePtrList[edgeIndex]->gb;

				if ((ld->IsTVVertVisible(a) || ld->IsTVVertVisible(b)) )
				{
					//					if (peltData.peltDialog.hWnd)
					///						goalLength[i] = peltData.mPeltSpringLength[i];
					//					else 
					goalLength[i] = Length(ld->GetGeomVert(va) -ld->GetGeomVert(vb))*GetUnwrapModPtr()->edgeScale* GetUnwrapModPtr()->edgeDistortionScale;
					pa = ld->GetTVVert(a);//TVMaps.v[a].p;
					pb = ld->GetTVVert(b);//TVMaps.v[b].p;
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

		Tab<IPoint2> la,lb;

		la.SetCount(ePtrListCount);
		lb.SetCount(ePtrListCount);

		Tab<int> colorList;
		colorList.SetCount(ePtrListCount);
		BitArray fullEdge;
		fullEdge.SetSize(ePtrListCount);
		fullEdge.ClearAll();

		SelectObject(hdc,GetStockObject(WHITE_PEN));

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
				if (ld->GetTVEdgeHidden(edgeIndex))//TVMaps.ePtrList[edgeIndex]->flags & FLAG_HIDDENEDGEA)
					showEdge = FALSE;
			}
			else showEdge = FALSE;
			//get our edge spring length goal
			if (showEdge)
			{
				int va,vb;
				va = ld->GetTVEdgeGeomVert(edgeIndex,0);//TVMaps.ePtrList[edgeIndex]->ga;
				vb = ld->GetTVEdgeGeomVert(edgeIndex,1);//TVMaps.ePtrList[edgeIndex]->gb;


				//						float goalLength = peltData.springEdges[i].dist;
				int a,b;
				a = ld->GetTVEdgeVert(edgeIndex,0);//TVMaps.ePtrList[edgeIndex]->a;
				b = ld->GetTVEdgeVert(edgeIndex,1);//TVMaps.ePtrList[edgeIndex]->b;
				if ((ld->IsTVVertVisible(a) || ld->IsTVVertVisible(b)) )
				{

					float goalL = goalLength[i];

					//get our current length
					Point3 pa, pb;
					pa = ld->GetTVVert(a);//TVMaps.v[a].p;
					pb = ld->GetTVVert(b);//TVMaps.v[b].p;
					pa.z = 0.0f;
					pb.z = 0.0f;

					Point3 vec = (pb-pa);
					Point3 uva,uvb;
					uva = pa;
					uvb = pb;
					uva.z = 0.0f;
					uvb.z = 0.0f;

					float currentL = currentLength[i];//Length(uva-uvb);
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
						la[edgeIndex] = ld->GetTransformedPoint(a);
						lb[edgeIndex] = ld->GetTransformedPoint(b);

					}
					else
					{
						Point3 mid = (pb+pa) * 0.5f;
						Point3 nvec = Normalize(vec);
						nvec = nvec * goalL * 0.5f;

						Point3 tvPoint = GetUnwrapModPtr()->UVWToScreen(mid + nvec ,xzoom,yzoom,width,height);
						IPoint2 aPoint;
						aPoint.x  = (int) tvPoint.x;
						aPoint.y  = (int) tvPoint.y;

						Point3 tvPoint2 = GetUnwrapModPtr()->UVWToScreen(mid - nvec ,xzoom,yzoom,width,height);
						IPoint2 bPoint;
						bPoint.x  = (int) tvPoint2.x;
						bPoint.y  = (int) tvPoint2.y;

						la[edgeIndex] = aPoint;
						lb[edgeIndex] = bPoint;

						MoveToEx(hdc,aPoint.x,aPoint.y,NULL);
						LineTo(hdc,ld->GetTransformedPoint(b).x,ld->GetTransformedPoint(b).y);
						MoveToEx(hdc,bPoint.x,bPoint.y,NULL);
						LineTo(hdc,ld->GetTransformedPoint(a).x,ld->GetTransformedPoint(a).y);
					}

				}

			}
		}
		for (int j = 0; j <= numberOfColors;j++)
		{
			float per =(float)j/(float)numberOfColors;
			Color c = (basec * (per)) + (goalc*(1.0f-per));
			int gr=0,gg=0,gb=0;
			gr = (int) (255 * c.r);
			gg = (int) (255 * c.g);
			gb = (int) (255 * c.b);

			HPEN gPen = CreatePen(PS_SOLID,1,RGB(gr,gg,gb));
			SelectObject(hdc,gPen);
			for (int i = 0; i < ePtrListCount; i++)
			{
				BOOL showEdge = TRUE;
				int edgeIndex =  i;
				if (edgeIndex != -1)
				{
					if (GetUnwrapModPtr()->mTVSubObjectMode == TVEDGEMODE)
					{
						if (ld->GetTVEdgeSelected(edgeIndex))//esel[edgeIndex])
							showEdge = FALSE;

					}
					if (ld->GetTVEdgeHidden(edgeIndex))//TVMaps.ePtrList[edgeIndex]->flags & FLAG_HIDDENEDGEA)
						showEdge = FALSE;
				}
				else showEdge = FALSE;

				if (showEdge && (colorList[i] == j))
				{
					int a,b;
					a = ld->GetTVEdgeVert(i,0);//TVMaps.ePtrList[i]->a;
					b = ld->GetTVEdgeVert(i,1);//TVMaps.ePtrList[i]->b;

					if ((ld->IsTVVertVisible(a) || ld->IsTVVertVisible(b)) )
					{
						MoveToEx(hdc,la[i].x,la[i].y,NULL);
						LineTo(hdc,lb[i].x,lb[i].y);	
					}
				}
			}
			SelectObject(hdc,GetStockObject(WHITE_BRUSH));					
			DeleteObject(gPen);
		}

	}
}

void Painter2DLegacy::PaintView()
{
	if (GetUnwrapModPtr()->bringUpPanel)
	{
		GetUnwrapModPtr()->bringUpPanel = FALSE;
		SetFocus(GetUnwrapModPtr()->hDialogWnd);
	}

	// russom - August 21, 2006 - 804464
	// Very very quick switching between modifiers might create a
	// situation in which EndEditParam is called within the MoveScriptUI call above.
	// This will set the ip ptr to NULL and we'll crash further down.
	if( GetUnwrapModPtr()->ip == NULL )
		return;

	if (GetUnwrapModPtr()->fnGetMapMode() == LSCMMAP)
		GetUnwrapModPtr()->mLSCMTool.Solve(false);

	//this can cached to only update when the selection has changed
	//	static BitArray clustersUsed;

	PAINTSTRUCT		ps;
	BeginPaint(GetUnwrapModPtr()->hView,&ps);
	EndPaint(GetUnwrapModPtr()->hView,&ps);

	int frozenr = (int) (GetRValue(GetUnwrapModPtr()->lineColor)*0.5f);
	int frozeng = (int) (GetGValue(GetUnwrapModPtr()->lineColor)*0.5f);
	int frozenb = (int) (GetBValue(GetUnwrapModPtr()->lineColor)*0.5f);
	COLORREF frozenColor = RGB(frozenr,frozeng,frozenb);
	COLORREF yellowColor = RGB(255,255,64);
	COLORREF darkyellowColor = RGB(117,117,28);
	COLORREF darkgreenColor = RGB(28,117,28);
	COLORREF greenColor = RGB(0,255,0);
	COLORREF blueColor = RGB(0,0,255);

	if (!GetUnwrapModPtr()->viewValid) 
	{
		//make sure our background texture is right
		if (!GetUnwrapModPtr()->image && GetUnwrapModPtr()->GetActiveMap()) 
			GetUnwrapModPtr()->SetupImage();

		GetUnwrapModPtr()->viewValid = TRUE;


		//compute your zooms and scale		
		iBuf->Erase();
		HDC hdc = iBuf->GetDC();


		//blit the background to the back buffer		
		if (GetUnwrapModPtr()->image && GetUnwrapModPtr()->showMap) 
		{	
			PaintBackground(hdc);	
		} 

		int i1, i2;
		GetUnwrapModPtr()->GetUVWIndices(i1,i2);
		float xzoom, yzoom;
		int width,height;
		GetUnwrapModPtr()->ComputeZooms(GetUnwrapModPtr()->hView,xzoom,yzoom,width,height);


		Point3 selP3Color;
		selP3Color.x = (int) GetRValue(GetUnwrapModPtr()->selColor);
		selP3Color.y = (int) GetGValue(GetUnwrapModPtr()->selColor);
		selP3Color.z = (int) GetBValue(GetUnwrapModPtr()->selColor);

		selP3Color.x += (255.0f - selP3Color.x) *0.25f;
		selP3Color.y += (255.0f - selP3Color.y) *0.25f;
		selP3Color.z += (255.0f - selP3Color.z) *0.25f;

		// Paint faces		
		HPEN selPen   = CreatePen(PS_SOLID,2,GetUnwrapModPtr()->selColor);
		HPEN selThinPen   = CreatePen(PS_DOT,1,GetUnwrapModPtr()->selColor);
		HPEN unselPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->lineColor);
		HPEN frozenPen = CreatePen(PS_SOLID,0,frozenColor);
		HPEN yellowPen = CreatePen(PS_SOLID,0,yellowColor);
		HPEN darkyellowPen = CreatePen(PS_SOLID,0,darkyellowColor);
		HPEN darkgreenPen = CreatePen(PS_SOLID,0,darkgreenColor);
		HPEN greenPen = CreatePen(PS_SOLID,0,greenColor);
		HPEN bluePen = CreatePen(PS_SOLID,0,blueColor);

		HPEN openEdgePen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->openEdgeColor);

		HPEN handlePen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->handleColor);

		HPEN freeFormPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->freeFormColor);
		HPEN sharedPen = CreatePen(PS_SOLID,0,GetUnwrapModPtr()->sharedColor);

		HPEN currentPen = NULL;

		PaintGrid(hdc);

		//loop through all our instances and transform the uvs to dialog space
		for (int whichLD = 0; whichLD < GetUnwrapModPtr()->mMeshTopoData.Count(); whichLD++)
		{

			MeshTopoData *md = GetUnwrapModPtr()->mMeshTopoData[whichLD]; 

			//put all our tvs in dialog space so we can quicly draw edges,faces and verts
			md->TransformedPointsSetCount( md->GetNumberTVVerts());
			for (int i=0; i< md->GetNumberTVVerts(); i++) 
			{
				Point3 tvPoint = GetUnwrapModPtr()->UVWToScreen(md->GetTVVert(i),xzoom,yzoom,width,height);
				IPoint2 tvPoint2;
				tvPoint2.x  = (int) tvPoint.x;
				tvPoint2.y  = (int) tvPoint.y;
				md->SetTransformedPoint(i,tvPoint2);				
			}
		}


		currentPen = unselPen;

		//Paint all the edges now
		PaintEdges(hdc);
		// Now paint points	

		if (GetUnwrapModPtr()->mTVSubObjectMode == TVVERTMODE)
		{
			PaintVertexTicks(hdc);	
		}

		//now paint the selected faces
		if (GetUnwrapModPtr()->mTVSubObjectMode == TVFACEMODE)
		{
			// paint selected faces
			PaintFaces(hdc);
		}

		PaintPelt(hdc);

		BOOL showLocalDistortion = 0;
		TimeValue t = GetCOREInterface()->GetTime();
		GetUnwrapModPtr()->pblock->GetValue(unwrap_localDistorion,t,showLocalDistortion,FOREVER);


		if (GetUnwrapModPtr()->showEdgeDistortion )
			PaintEdgeDistortion(hdc);

		if (GetUnwrapModPtr()->mTVSubObjectMode != TVOBJECTMODE)		
			PaintFreeFormGizmo(hdc);



		if ( (GetUnwrapModPtr()->mode == ID_SKETCHMODE))
		{
			SelectObject(hdc,selPen);
			IPoint2 ipt[4];
			for (int i = 0; i < GetUnwrapModPtr()->sketchMode->indexList.Count(); i++)
			{
				int index = GetUnwrapModPtr()->sketchMode->indexList[i].mIndex;
				MeshTopoData *ld = GetUnwrapModPtr()->sketchMode->indexList[i].mLD;


				ipt[0] = ld->GetTransformedPoint(index);

				Rectangle(hdc,	ipt[0].x-1,ipt[0].y-1,
					ipt[0].x+1,ipt[0].y+1);		

				if (GetUnwrapModPtr()->sketchDisplayPoints)
				{
					TSTR vertStr;
					vertStr.printf(_T("%d"),i);
					DLTextOut(hdc, ipt[0].x+4,ipt[0].y-4,vertStr);
				}
			}
		}


		if (GetUnwrapModPtr()->fnGetShowCounter())
		{
			int vct = 0;
			int ect = 0;
			int fct = 0;

			for (int ldID = 0; ldID < GetUnwrapModPtr()->mMeshTopoData.Count(); ldID++)
			{
				vct += GetUnwrapModPtr()->mMeshTopoData[ldID]->GetTVVertSel().NumberSet();
				ect += GetUnwrapModPtr()->mMeshTopoData[ldID]->GetTVEdgeSel().NumberSet();
				fct += GetUnwrapModPtr()->mMeshTopoData[ldID]->GetFaceSel().NumberSet();
			}
			SelectObject(hdc,GetStockObject(WHITE_PEN));
			TSTR vertStr;
			if (GetUnwrapModPtr()->fnGetTVSubMode() == TVVERTMODE)			
				vertStr.printf(_T("%d %s"),(int)vct,GetString(IDS_VERTEXCOUNTER));
			if (GetUnwrapModPtr()->fnGetTVSubMode() == TVEDGEMODE)			
				vertStr.printf(_T("%d %s"),(int)ect,GetString(IDS_EDGECOUNTER));
			if (GetUnwrapModPtr()->fnGetTVSubMode() == TVFACEMODE)			
				vertStr.printf(_T("%d %s"),(int)fct,GetString(IDS_FACECOUNTER));
			DLTextOut(hdc, 2,30,vertStr);

		}

		SelectObject(hdc,GetStockObject(BLACK_PEN));
		DeleteObject(selPen);
		DeleteObject(selThinPen);

		DeleteObject(yellowPen);		
		DeleteObject(darkyellowPen);		
		DeleteObject(darkgreenPen);		
		DeleteObject(greenPen);		
		DeleteObject(bluePen);		
		DeleteObject(frozenPen);		
		DeleteObject(unselPen);		
		DeleteObject(openEdgePen);		
		DeleteObject(handlePen);		
		DeleteObject(freeFormPen);		
		DeleteObject(sharedPen);

		iBuf->Blit();
	}
	else
	{
		if (!GetUnwrapModPtr()->fnGetPaintMode())
		{
			iBuf->Blit();
		}

	}
}
