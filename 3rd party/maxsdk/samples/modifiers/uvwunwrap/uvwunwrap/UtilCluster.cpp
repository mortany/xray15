#include <cmath>
#include "unwrap.h"
#include "modsres.h"
#include "utilityMethods.h"

#include "3dsmaxport.h"
#include <Util/BailOut.h>
#include "../../../Include/uvwunwrap/uvwunwrapNewAPIs.h"

//these are just debug globals so I can stuff data into to draw
/*
#ifdef DEBUGMODE
//just some pos tabs to draw bounding volumes
extern Tab<float> jointClusterBoxX;
extern Tab<float> jointClusterBoxY;
extern Tab<float> jointClusterBoxW;
extern Tab<float> jointClusterBoxH;


extern float hitClusterBoxX,hitClusterBoxY,hitClusterBoxW,hitClusterBoxH;

extern int currentCluster, subCluster;

//used to turn off the regular display and only show debug display
extern BOOL drawOnlyBounds;
#endif
*/

float AreaOfTriangle(Point3 a, Point3 b, Point3 c)

{
	float area;
	Point3 vec1 = a - b;
	Point3 vec2 = a - c;
	Point3 crossProd = CrossProd(vec1,vec2);
	area = 0.5*FLength(crossProd);

	return area;

}

float AreaOfPolygon(Tab<Point3> &points)

{
	float area = 0.0f;

	if (points.Count() < 3)
		return area;

	Point3 a = points[0];
	for (int i = 0; i < (points.Count()-2); i++)
	{ 
		Point3 b = points[i+1];
		Point3 c = points[i+2];
		area += AreaOfTriangle(a,b,c);
	}

	return area;


}

ClusterClass::ClusterClass()
{
	pNewPackData = CreateClusterNewPackData();
}

ClusterClass::~ClusterClass()
{
	DestroyClusterNewPackData(pNewPackData);
}

//this just though cluster building a bounding box for each face that is in this cluster
void ClusterClass::BuildList()
{
	int ct = faces.Count();
	boundsList.SetCount(ct);
	for (int i = 0; i < ct; i++)
	{
		boundsList[i].Init();
		int faceIndex = faces[i];
		int degree = ld->GetFaceDegree(faceIndex);
		for (int k = 0; k <  degree; k++)//TVMaps.f[faceIndex]->count; k++)
		{
			//need to put patch handles in here also
			int index = ld->GetFaceTVVert(faceIndex,k);//TVMaps.f[faceIndex]->t[k];
			Point3 a = ld->GetTVVert(index);//TVMaps.v[index].GetP();
			boundsList[i] += a;

			if ( ld->GetFaceHasVectors(faceIndex))//(TVMaps.f[faceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[faceIndex]->vecs))
			{
				int vertIndex = ld->GetFaceTVInterior(faceIndex,k);//TVMaps.f[faceIndex]->vecs->interiors[k];
				if (vertIndex >=0) 
				{
					Point3 a = ld->GetTVVert(vertIndex);//TVMaps.v[vertIndex].GetP();
					boundsList[i] += a;
				}

				vertIndex = ld->GetFaceTVHandle(faceIndex,k*2);//TVMaps.f[faceIndex]->vecs->handles[k*2];
				if (vertIndex >=0)
				{
					Point3 a = ld->GetTVVert(vertIndex);// TVMaps.v[vertIndex].GetP();
					boundsList[i] += a;
				}

				vertIndex = ld->GetFaceTVHandle(faceIndex,k*2+1);//TVMaps.f[faceIndex]->vecs->handles[k*2+1];
				if (vertIndex >= 0 )
				{
					Point3 a = ld->GetTVVert(vertIndex);// TVMaps.v[vertIndex].GetP();
					boundsList[i] += a;
				}

			}

		}
		boundsList[i].pmin.z = -1.0f;
		boundsList[i].pmax.z = 1.0f;

		if ((boundsList[i].pmax.x-boundsList[i].pmin.x) == 0.0f)
		{
			boundsList[i].pmax.x += 0.5f;
			boundsList[i].pmin.x -= 0.5f;
		}

		if ((boundsList[i].pmax.y-boundsList[i].pmin.y) == 0.0f)
		{
			boundsList[i].pmax.y += 0.5f;
			boundsList[i].pmin.y -= 0.5f;
		}

	}
}

BOOL ClusterClass::DoesIntersect(float x, float y, float w, float h)
{
	int ct = boundsList.Count();
	Point3 a1,b1,c1,d1;
	float miny, minx, maxx,maxy;
	minx = x;
	miny = y;
	maxx = x+w;
	maxy = y+h;

	for (int i = 0; i < ct; i++)
	{
		BOOL intersect = TRUE;
		Box3& b = boundsList[i];
		float bminy, bminx, bmaxx,bmaxy;
		bminx = b.pmin.x;
		bminy = b.pmin.y;
		bmaxx = b.pmax.x;
		bmaxy = b.pmax.y;

		if (minx > bmaxx) 
			intersect = FALSE;
		if (miny > bmaxy) 
			intersect = FALSE;

		if (maxx < bminx) 
			intersect = FALSE;
		if (maxy < bminy) 
			intersect = FALSE;

		if (intersect) return TRUE;

	}

	return FALSE;
}

int ClusterClass::ComputeOpenSpaces(float spacing, BOOL onlyCenter)
{

#ifdef DEBUGMODE
	/*
	hitClusterBoxX = bounds.pmin.x;
	hitClusterBoxY = bounds.pmin.y;
	hitClusterBoxW = w;
	hitClusterBoxH = h;

	jointClusterBoxX.ZeroCount();
	jointClusterBoxY.ZeroCount();
	jointClusterBoxW.ZeroCount();
	jointClusterBoxH.ZeroCount();
	*/
#endif 

	float boundsArea = w*h;

	float perCoverage = surfaceArea/boundsArea;

	if (perCoverage > 0.95f) return 0;

	//right now we split the cluster into a 200x200 grid to look for open spots
	int iGridW,iGridH;
	iGridW = 100;
	iGridH = 100;
	float fEdgeLenW = w/(float) (iGridW);
	float fEdgeLenH = h/(float) (iGridH);

	int area = iGridW * iGridW;

	BitArray tusedList;
	tusedList.SetSize(area);
	tusedList.ClearAll();

	float fX,fY;
	fY = bounds.pmin.y;
	//loop through bitmap marking used areas

	for (int j =0; j < iGridH; j++)
	{
		fX = bounds.pmin.x;

		for (int k =0; k < iGridW; k++)
		{
			if (DoesIntersect(fX-spacing,fY-spacing,fEdgeLenW+spacing,fEdgeLenH+spacing))
				tusedList.Set(j*iGridW+k);
			fX += fEdgeLenW;
		}
		fY += fEdgeLenH;
	}


	//now we need to find groups of bounding boxes
	int iX, iY;
	iX = 0;
	iY = 0;

	int iCX, iCY;
	int iDirY, iDirX;
	int iBoxX = 0, iBoxY = 0, iBoxW = 0, iBoxH = 0;

	if (!onlyCenter)
	{
		//start at the upp left corners

		iCX = 0;
		iCY = iGridW-1;
		iDirX = 1;
		iDirY = -1;
		iBoxX = 0;
		iBoxY = 0;
		iBoxW = 0;
		iBoxH = 0;


		FindRectDiag(iCX, iCY, iGridW, iGridH, iDirX, iDirY,tusedList, iBoxX, iBoxY, iBoxW, iBoxH);
		if ((iBoxW != 0) && (iBoxH != 0))
		{
			SubClusterClass temp;
			temp.x = bounds.pmin.x+ (iBoxX*fEdgeLenW);
			temp.y = bounds.pmin.y+(iBoxY*fEdgeLenH);
			temp.w = iBoxW*fEdgeLenW;
			temp.h = iBoxH*fEdgeLenH;
			openSpaces.Append(1,&temp);
		}

		//start at the upp right corners
		iCX = iGridW-1;
		iCY = iGridH-1;
		iDirX = -1;
		iDirY = -1;
		iBoxX = 0;
		iBoxY = 0;
		iBoxW = 0;
		iBoxH = 0;
		FindRectDiag(iCX, iCY, iGridW, iGridH, iDirX, iDirY,tusedList, iBoxX, iBoxY, iBoxW, iBoxH);
		if ((iBoxW != 0) && (iBoxH != 0))
		{
			SubClusterClass temp;
			temp.x = bounds.pmin.x+ (iBoxX*fEdgeLenW);
			temp.y = bounds.pmin.y+(iBoxY*fEdgeLenH);
			temp.w = iBoxW*fEdgeLenW;
			temp.h = iBoxH*fEdgeLenH;
			openSpaces.Append(1,&temp);
		}


		//start at the lower left corners
		iCX = 0;
		iCY = 0;
		iDirX = 1;
		iDirY = 1;
		iBoxX = 0;
		iBoxY = 0;
		iBoxW = 0;
		iBoxH = 0;
		FindRectDiag(iCX, iCY, iGridW, iGridH, iDirX, iDirY,tusedList, iBoxX, iBoxY, iBoxW, iBoxH);
		if ((iBoxW != 0) && (iBoxH != 0))
		{
			SubClusterClass temp;
			temp.x = bounds.pmin.x+ (iBoxX*fEdgeLenW);
			temp.y = bounds.pmin.y+(iBoxY*fEdgeLenH);
			temp.w = iBoxW*fEdgeLenW;
			temp.h = iBoxH*fEdgeLenH;
			openSpaces.Append(1,&temp);
			/*
			#ifdef DEBUGMODE
			jointClusterBoxX.Append(1,&temp.x);
			jointClusterBoxY.Append(1,&temp.y);
			jointClusterBoxW.Append(1,&temp.w);
			jointClusterBoxH.Append(1,&temp.h);
			#endif
			*/
		}

		//start at the lower right corners
		iCX = iGridW-1;
		iCY = 0;
		iDirX = -1;
		iDirY = 1;
		iBoxX = 0;
		iBoxY = 0;
		iBoxW = 0;
		iBoxH = 0;
		FindRectDiag(iCX, iCY, iGridW, iGridH, iDirX, iDirY,tusedList, iBoxX, iBoxY, iBoxW, iBoxH);
		if ((iBoxW != 0) && (iBoxH != 0))
		{
			SubClusterClass temp;
			temp.x = bounds.pmin.x+ (iBoxX*fEdgeLenW);
			temp.y = bounds.pmin.y+(iBoxY*fEdgeLenH);
			temp.w = iBoxW*fEdgeLenW;
			temp.h = iBoxH*fEdgeLenH;
			openSpaces.Append(1,&temp);
			/*
			#ifdef DEBUGMODE
			jointClusterBoxX.Append(1,&temp.x);
			jointClusterBoxY.Append(1,&temp.y);
			jointClusterBoxW.Append(1,&temp.w);
			jointClusterBoxH.Append(1,&temp.h);
			#endif
			*/
		}
	}

	//do center
	int iCenterX = iGridW/2;
	int iCenterY = iGridH/2;
	iBoxX = 0;
	iBoxY = 0;
	iBoxW = 0;
	iBoxH = 0;
	FindSquareX(iCenterX, iCenterY,iGridW, iGridH, tusedList, iBoxX, iBoxY, iBoxW, iBoxH);

	if ((iBoxW != 0) && (iBoxH != 0))
	{
		SubClusterClass temp;
		temp.x = bounds.pmin.x+ (iBoxX*fEdgeLenW);
		temp.y = bounds.pmin.y+(iBoxY*fEdgeLenH);
		temp.w = iBoxW*fEdgeLenW;
		temp.h = iBoxH*fEdgeLenH;
		openSpaces.Append(1,&temp);
		//we fit so stuff it in the cluster
		/*
		#ifdef DEBUGMODE
		jointClusterBoxX.Append(1,&temp.x);
		jointClusterBoxY.Append(1,&temp.y);
		jointClusterBoxW.Append(1,&temp.w);
		jointClusterBoxH.Append(1,&temp.h);
		#endif
		*/
	}

	//now loop through all the remaining points
	if (!onlyCenter)
	{
		for (int j =0; j < iGridH; j++)
		{
			for (int k =0; k < iGridW; k++)
			{
				int index = j * iGridW + k;
				if (!tusedList[index])
				{
					iBoxX = 0;
					iBoxY = 0;
					iBoxW = 0;
					iBoxH = 0;
					FindRectDiag(k, j,iGridW, iGridH, 1,1, tusedList, iBoxX, iBoxY, iBoxW, iBoxH);

					if ((iBoxW >2 ) && (iBoxH > 2))
					{
						SubClusterClass temp;
						temp.x = bounds.pmin.x+ (iBoxX*fEdgeLenW);
						temp.y = bounds.pmin.y+(iBoxY*fEdgeLenH);
						temp.w = iBoxW*fEdgeLenW;
						temp.h = iBoxH*fEdgeLenH;
						openSpaces.Append(1,&temp);
						//we fit so stuff it in the cluster
						/*
						#ifdef DEBUGMODE
						jointClusterBoxX.Append(1,&temp.x);
						jointClusterBoxY.Append(1,&temp.y);
						jointClusterBoxW.Append(1,&temp.w);
						jointClusterBoxH.Append(1,&temp.h);
						#endif
						*/


					}

				}

			}


		}
	}


#ifdef DEBUGMODE
	/*      
	jointClusterBoxX.SetCount(openSpaces.Count());
	jointClusterBoxY.SetCount(openSpaces.Count());
	jointClusterBoxW.SetCount(openSpaces.Count());
	jointClusterBoxH.SetCount(openSpaces.Count());

	for (int j = 0; j < openSpaces.Count(); j++)
	{
	jointClusterBoxX[j] = openSpaces[j].x;
	jointClusterBoxY[j] = openSpaces[j].y;
	jointClusterBoxW[j] = openSpaces[j].w;
	jointClusterBoxH[j] = openSpaces[j].h;
	}
	*/
#endif
	return 1;
}


#define MODE_CENTER     0
#define MODE_LEFTTOP 1
#define MODE_RIGHTTOP   2
#define MODE_LEFTBOTTOM 3
#define MODE_RIGHTBOTTOM 4

class WanderClass
{
public:

	WanderClass(float x, float y, float dirX, float dirY, int mode) : h(0.0f), w(0.0f), hit(0)
	{
		this->x = x;
		this->y = y;
		this->dirX = dirX;
		this->dirY = dirY;
		this->mode = mode;
		rowHeight = -1.0f;
	}

	float GetX() {
		if (mode == MODE_CENTER)
			return (x - (w *0.5f));
		else if (mode == MODE_LEFTTOP)
			return (x) ;
		else if (mode == MODE_LEFTBOTTOM)
			return (x - w);
		else if (mode == MODE_RIGHTTOP)
			return (x) ;
		else if (mode == MODE_RIGHTBOTTOM)
			return (x - w) ;
		return x;
	};
	float GetY() {
		if (mode == MODE_CENTER)
			return (y - (y *0.5f));
		else if (mode == MODE_RIGHTBOTTOM)
			return (y) ;
		else if (mode == MODE_LEFTBOTTOM)
			return (y );
		else if (mode == MODE_RIGHTTOP)
			return (y - h) ;
		else if (mode == MODE_LEFTTOP)
			return (y - h) ;
		return y;
	};

	void SetW(float w) {this->w = w;}
	void SetH(float h) {this->h = h;}
	void Advance()
	{
		x += w * dirX;
		if (rowHeight < 0.0f)
			rowHeight = h;
	}
	void NextRow()
	{
		if (rowHeight > 0.0f)
		{
			y+= rowHeight*dirY;
			rowHeight = -1.0f;
		}
	}
	void ClearHit() { hit = FALSE; }
	void Hit() { hit = TRUE; }
	BOOL IsHit() { return hit; }

private:
	int mode;
	float x,y;
	float w,h;
	float dirX,dirY;
	float rowHeight;
	BOOL hit;

};

IMeshTopoData* ClusterClass::GetOwnerMeshTopoData()
{ 
	return ld; 
}

void  ClusterClass::FindSquareX(int x, int y, int w, int h, BitArray &used, int &iretX, int &iretY, int &iretW, int &iretH)
{  
	// start in the w direction one
	int iAmount = 0;


	if (used[y*w+x])  return;

	BOOL done = FALSE;
	while (!done)
	{
		iAmount++;

		//check if goes out of border
		if ((x-iAmount) < 0)
			done = TRUE;
		if ((y-iAmount) < 0)
			done = TRUE;
		if ((x+iAmount) >=w)
			done = TRUE;
		if ((y+iAmount) >=h)
			done = TRUE;
		//now make sure alll pixels are unused
		if (!done)
		{
			int iStartX = x - iAmount;
			//top edge
			int iY = y - iAmount;
			int iStart = iStartX + (iY*w);
			for (int i = 0; i < iAmount*2 +1; i++)
			{
				if (used[iStart]) 
				{
					done = TRUE;
					i = iAmount*2 +1;
				}
				iStart++;
			}

			//bottom edge
			iY = y + iAmount;
			iStart = iStartX + (iY*w);
			for (int i = 0; i < iAmount*2 +1; i++)
			{
				if (used[iStart]) 
				{
					done = TRUE;
					i = iAmount*2 +1;
				}
				iStart++;
			}

			//left edge
			iY = y - iAmount;
			iStart = iStartX + (iY*w);
			for (int i = 0; i < iAmount*2 +1; i++)
			{
				if (used[iStart]) 
				{
					done = TRUE;
					i = iAmount*2 +1;
				}
				iStart+=w;
			}
			//left edge
			iY = y - iAmount;
			iStart = (x+iAmount) + (iY*w);
			for (int i = 0; i < iAmount*2 +1; i++)
			{
				if (used[iStart]) 
				{
					done = TRUE;
					i = iAmount*2 +1;
				}
				iStart+=w;
			}
		}


	}
	iAmount--;
	iretX = x - iAmount;
	iretY = y - iAmount;
	iretW = iAmount*2+1;
	iretH = iAmount*2+1;

	for (int i = iretY; i < (iretY+iretH); i++)
	{
		int iStart =  i*w + iretX;
		for (int j = iretX; j < (iretX+iretW); j++)
		{
			used.Set(iStart);
			iStart++;
		}
	}

}
void  ClusterClass::FindRectDiag(int x, int y, int w, int h, 
								 int dirX, int dirY,
								 BitArray &used, int &iretX, int &iretY, int &iretW, int &iretH)
{  
	if (used[y*w+x])  return;

	int iAmount = 0;
	int iXAdditionalAmount = 0;
	int iYAdditionalAmount = 0;
	BOOL done = FALSE;
	//start walking a diag till a hit
	int iX, iY;
	iX = x;
	iY = y;
	BOOL bHitSide = FALSE;
	BOOL bHitTop = FALSE;
	int sidecount =0;
	int topcount =0;
	while (!done)
	{
		iAmount++;
		iX = x + (iAmount*dirX);
		iY = y + (iAmount*dirY);

		//check if goes out of border
		if (iX < 0)
		{
			done = TRUE;
			bHitSide = TRUE;
			sidecount++;
		}
		if (iY < 0)
		{
			done = TRUE;
			bHitTop = TRUE;
			topcount++;
		}
		if (iX >=w)
		{
			done = TRUE;
			bHitSide = TRUE;
			sidecount++;
		}
		if (iY >=h)
		{
			done = TRUE;
			bHitTop = TRUE;
			topcount++;
		}
		//top/bottom edge
		if (!done)
		{
			int iStartX, iEndX;
			iStartX = x;
			iEndX = iX-(dirX);

			int iStartY, iEndY;
			iStartY = y;
			iEndY = iY-(dirY);



			if (iStartX > iEndX)
			{
				int temp = iStartX;
				iStartX = iEndX;
				iEndX = temp;
			}

			if (iStartY > iEndY)
			{
				int temp = iStartY;
				iStartY = iEndY;
				iEndY = temp;
			}


			int iStart = (iY*w) + iStartX ;


			for (int i = iStartX; i < (iEndX+1);i++)
			{
				if (used[iStart]) 
				{
					done = TRUE;
					bHitTop = TRUE;
					topcount++;
				}
				//          else 
				//             used.Set(iStart);
				iStart++;


			}

			///left/right edge

			iStart = (iStartY*w) + iX ;
			for (int i = iStartY; i < (iEndY+1);i++)
			{
				if (used[iStart]) 
				{
					done = TRUE;
					bHitSide = TRUE;
					sidecount++;
				}
				//          else 
				//             used.Set(iStart);
				iStart+=w;

			}
			iStart = iY*w+iX;
			if (used[iStart]) 
			{
				done = TRUE;
				bHitSide = TRUE;
				bHitTop = TRUE;
				topcount++;
				sidecount++;
			}
			//       else 
			//          used.Set(iStart);




		}
	}
	iAmount--;



	if ((bHitSide) && (bHitTop))
	{
		if (sidecount>topcount)
		{
			bHitSide = TRUE;
			bHitTop = FALSE;
		}
		else
		{
			bHitSide = FALSE;
			bHitTop = TRUE;
		}

	}

	if (iAmount == 0) return;

	//backup one and decide whether to go right, down  or quite
	//both hit we are done
	if ((bHitSide) && (bHitTop))
	{
		//return the new x,y, and w, h
		if (dirX == 1)
			iretX = x; 
		else iretX = x + (iAmount*dirX);
		if (dirY == 1)
			iretY = y;
		else iretY = y + (iAmount*dirY);
		iretW = iAmount;
		iretH = iAmount;
	}
	else 
	{
		// start in the w direction one
		BOOL bGoingSide = TRUE;
		//       if (bHitTop) dirY = 0;
		if (bHitSide) 
		{
			//          dirX = 0;
			bGoingSide = FALSE;
		}

		done = FALSE;

		while (!done)
		{
			if (bGoingSide)
				iXAdditionalAmount++;//=dirX;
			else iYAdditionalAmount++;//=dirY;



			int iStart, iEnd;
			int inc = 0;
			int index = 0;
			if (bGoingSide)
			{
				int startY = y;
				int endY = y + (iAmount*dirY);
				int xPos = x+(iAmount+iXAdditionalAmount) * dirX;
				iStart = (startY * w)   + xPos;
				iEnd =   (endY*w)   +  xPos;

				inc = w * dirY;

				index = x + (iAmount*dirX)  + (iXAdditionalAmount *dirX);
			}
			else
			{
				int yPos = y + (iAmount+iYAdditionalAmount)*dirY; 
				iStart = (yPos*w) + x ;
				iEnd = (yPos*w) + x+(iAmount+iXAdditionalAmount)*dirX;


				inc = dirX; 
				index = y + (iAmount*dirY)  + (iYAdditionalAmount *dirY);
			}

			//check if goes out of border
			if (index < 0)
				done = TRUE;
			if (index < 0)
				done = TRUE;
			if ((bGoingSide) && (index >=w))
				done = TRUE;
			if ((!bGoingSide) && (index >=h))
				done = TRUE;

			if (!done)
			{
				while (iStart != iEnd)
				{
					if (used[iStart]) 
					{
						done = TRUE;
						iStart = iEnd;
					}
					else
					{

						iStart += inc;
					}


				}
			}


		}

		if (bGoingSide)
			iXAdditionalAmount--;
		else iYAdditionalAmount--;

		//return the new x,y, and w, h

		if (dirX == 1)
			iretX = x; 
		else iretX = x + (iAmount*dirX) + ((iXAdditionalAmount)*dirX);
		if (dirY == 1)
			iretY = y; 
		else iretY = y + (iAmount*dirY) + ((iYAdditionalAmount)*dirY);
		iretW = iAmount+(iXAdditionalAmount);
		iretH = iAmount+(iYAdditionalAmount);

	}



	for (int i = iretY; i < (iretY+iretH+1); i++)
	{
		int iStart =  i*w + iretX;
		for (int j = iretX; j < (iretX+iretW+1); j++)
		{
			used.Set(iStart);
			iStart++;
		}
	}


}


BOOL  UnwrapMod::BuildClusterFromTVVertexElement(MeshTopoData *ld)
{

	if (!ip) return FALSE;


	if (ld == NULL) 
	{
		return FALSE;
	}

	ConstantTopoAccelerator topoAccl(ld, FALSE);

	Tab<Point3> objNormList;
	ld->BuildNormals(objNormList);

	//need to  hold the soft selection since the below code clears it
	Tab<float> holdSoftSel;
	holdSoftSel.SetCount(ld->GetNumberTVVerts());
	for (int i = 0; i < holdSoftSel.Count(); i++)
	{
		holdSoftSel[i] = ld->GetTVVertInfluence(i);
	}

	//build normals
	TSTR statusMessage;


	BitArray processedVerts;
	processedVerts.SetSize(ld->GetNumberTVVerts());
	processedVerts.ClearAll();

	//get our clusters from user defined clusters first a little tricky since we only track faces but need
	//faces and vertices in this case
    MaxSDK::Util::BailOutManager bailoutManager;
	int clustCount = pblock->Count(unwrap_group_name);
	for (int clustID = 0; clustID < clustCount; clustID++)
	{
		ld->HoldFaceSel();
		ld->ClearFaceSelection();
		ld->GetToolGroupingData()->SelectGroup(clustID,TVFACEMODE);

		BitArray vsel;
		vsel.SetSize(ld->GetNumberTVVerts());
		vsel.ClearAll();
		ld->GetVertSelFromFace(vsel);	

		Point3 normal(0.0f,0.0f,0.0f);
		int fct =0;

		if (vsel.NumberSet() > 0)
		{
			//create a cluster and add it
			ClusterClass *cluster = new ClusterClass();
			cluster->ld = ld;

			BOOL add= FALSE;
			for (int j = 0; j < ld->GetNumberTVVerts(); j++)
			{
				if (processedVerts[j])
					vsel.Set(j,FALSE);
				if (vsel[j] )
					cluster->verts.Append(1,&j,100);
			}

			for (int j = 0; j < ld->GetNumberFaces(); j++)
			{
				int ct = ld->GetFaceDegree(j);
				BOOL hit = FALSE;
				for (int k =0; k < ct; k++)
				{
					int index = ld->GetFaceTVVert(j,k);
					if (vsel[index]) hit = TRUE;
				}
				//add to cluster
				if (hit)
				{
					add = TRUE;
					cluster->faces.Append(1,&j,100);

					normal += objNormList[j];
					fct++;
				}
			}  

			processedVerts |= vsel;

			//add edges that were processed

			if (add)
			{
				cluster->normal = normal/(float)fct;
				clusterList.Append(1,&cluster);
			}
			else delete cluster;
		}

		ld->RestoreFaceSel();
	}

	for (int i =0; i < ld->GetNumberTVVerts(); i++)
	{
		if ((!(ld->GetTVVertDead(i))) && (!processedVerts[i]) && !ld->GetTVSystemLock(i))
		{

			ld->ClearSelection(TVVERTMODE);
			ld->SetTVVertSelected(i,TRUE);
			ld->SelectVertexElement(TRUE);

			Point3 normal(0.0f,0.0f,0.0f);
			int fct =0;
			BitArray vsel = ld->GetTVVertSel();

			if (vsel.NumberSet() > 0)
			{
				//create a cluster and add it
				ClusterClass *cluster = new ClusterClass();
				cluster->ld = ld;

				BOOL add= FALSE;
				for (int j = 0; j < ld->GetNumberTVVerts(); j++)
				{
					//remove any processed verts
					if (processedVerts[j])
						vsel.Set(j,FALSE);
					if (vsel[j] )
						cluster->verts.Append(1,&j,100);
				}

				for (int j = 0; j < ld->GetNumberFaces(); j++)
				{
					int ct = ld->GetFaceDegree(j);
					BOOL hit = FALSE;
					for (int k =0; k < ct; k++)
					{
						int index = ld->GetFaceTVVert(j,k);
						if (vsel[index]) hit = TRUE;
					}
					//add to cluster
					if (hit)
					{
						add = TRUE;
						cluster->faces.Append(1,&j,100);

						normal += objNormList[j];
						fct++;
					}
				}  
				//add the selection to our processed verts list so we dont add them twice
				processedVerts |= ld->GetTVVertSel();

				//add edges that were processed

				if (add)
				{
					cluster->normal = normal/(float)fct;
					clusterList.Append(1,&cluster);
				}
				else delete cluster;
			}
			int per = (i * 100)/ld->GetNumberTVVerts();
			statusMessage.printf(_T("%s %d%%."),GetString(IDS_PW_STATUS_BUILDCLUSTER),per);
            UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
			if (bailoutManager.ShouldBail())
            {
				//restore the soft selection 
				for (int k = 0; k < holdSoftSel.Count(); k++)
				{
					if (!ld->GetTVVertDead(k))
						ld->SetTVVertInfluence(k, holdSoftSel[k]);
				}

				return FALSE;
			}
		}                          
	}

	//restore the soft selection 
	for (int k = 0; k < holdSoftSel.Count(); k++)
	{
		if (!ld->GetTVVertDead(k))
			ld->SetTVVertInfluence(k, holdSoftSel[k]);
	}

	return TRUE;

}


void UnwrapMod::FreeClusterList()
{
	for (int i = 0; i < clusterList.Count(); i++)
	{
		delete clusterList[i];
	}
	clusterList.ZeroCount();
}

BOOL UnwrapMod::BuildMeshTopoDataCluster(
	MeshTopoData* ld, const Tab<Point3>& normalList, float threshold, BOOL connected,
	BOOL cleanUpStrayFaces, MeshTopoData::GroupBy groupBy, Tab<ClusterClass*> &clusterList)
{
	//If the TVMaps has not been set yet
	if (!ld || (ld->GetNumberFaces() == 0) || (ld->GetFaceSel().GetSize() != ld->GetNumberFaces()))
	{
		DbgAssert(0);
		return FALSE;
	}

	BitArray processedFaces;
	Tab<BorderClass> clusterBorder;
	BitArray sel;
	sel.SetSize(ld->GetNumberFaces());

	processedFaces.SetSize(ld->GetNumberFaces());
	processedFaces.ClearAll();

	//check for type
	float radThreshold = threshold * PI / 180.0f;
	TSTR statusMessage;

	Tab<Point3> objNormList;
	ld->BuildNormals(objNormList);

	int mfSize = ld->GetFaceSel().GetSize();
	for (int i = 0; i < mfSize; i++)
	{
		if (!ld->GetFaceSelected(i))
			processedFaces.Set(i);
	}

	//build normals
	AdjEdgeList *edges = NULL;
	BOOL deleteEdges = FALSE;
	if ((ld->GetMesh()) && (connected))
	{
		edges = new AdjEdgeList(*(ld->GetMesh()));
		deleteEdges = TRUE;
	}

    MaxSDK::Util::BailOutManager bailoutManager;
	if (connected)
	{
		BOOL done = FALSE;
		int currentNormal = 0;

		// Precompute angles between specified normals and face normals to avoid potentially large number of transcendental
        // function calculations within the while loop
        Tab<float> normAngles;

		int nNormals = normalList.Count();
        int listSize = objNormList.Count();
        normAngles.SetCount(nNormals * listSize);
        for (int i = 0; i != nNormals; ++i)
        {
            const Point3& normCurr = normalList[i];
            float* anglesCurr = &(normAngles[i * listSize]);
            for (int j = 0; j != listSize; ++j)
            {
                anglesCurr[j] = 0.0f;
                if (!(processedFaces[j]))
                {
                   float normalProd = DotProd(normCurr, objNormList[j]);
                   anglesCurr[j] = std::acos(normalProd);
                }
            }
        }

		if (normalList.Count() == 0)
			done = TRUE;

		while (!done)
		{
			sel.ClearAll();

            // Find the closest normal within the threshold
            float angDist = -1.0f;
			int hitIndex = -1;
            float* anglesCurr = &(normAngles[currentNormal * listSize]);
			for (int i = 0; i < listSize; i++)
			{
                float cangle = anglesCurr[i];
                if ((!(processedFaces[i])) && ((cangle < angDist) || (hitIndex == -1)) && (cangle < radThreshold))
                {
                    angDist = cangle;
                    hitIndex = i;
                }
			}

			int bail = 0;
			if ((hitIndex != -1))
			{
				//FIX HERE BY MAT OR SMGRP
				SelectFacesByGroup(ld, sel, hitIndex, normalList[currentNormal], threshold, FALSE, groupBy, objNormList,
					clusterBorder,
					edges);
                DbgAssert((objNormList.Count()) == listSize);

				//To be safe. In case the size of sel is changed in SelectFacesByGroup
				if (processedFaces.GetSize() != sel.GetSize())
					processedFaces.SetSize(sel.GetSize(), 1);
				if (processedFaces.GetSize() == 0)
				{
					DbgAssert(0);
					return FALSE;
				}

				//add cluster
				if (sel.NumberSet() > 0)
				{
					//create a cluster and add it
					BitArray clusterVerts;
					clusterVerts.SetSize(ld->GetNumberTVVerts());
					clusterVerts.ClearAll();

					ClusterClass *cluster = new ClusterClass();
					cluster->ld = ld;
					BOOL hit = FALSE;
					cluster->normal = normalList[currentNormal];
					int selSize = sel.GetSize();
					for (int j = 0; j < selSize; j++)
					{
						if (sel[j] && (!processedFaces[j]))
						{
							//add to cluster
							processedFaces.Set(j, TRUE);
							cluster->faces.Append(1, &j);
							ld->AddVertsToCluster(j, clusterVerts, cluster);
							hit = TRUE;
						}
					}

					cluster->borderData = clusterBorder;
					//add edges that were processed
					if (hit)
					{
						clusterList.Append(1, &cluster);
						bail++;
					}
					else
					{
						delete cluster;
					}
				}
			}
			currentNormal++;
			if (currentNormal >= normalList.Count())
			{
				currentNormal = 0;
				if (bail == 0) done = TRUE;
			}

			int per = (processedFaces.NumberSet() * 100) / processedFaces.GetSize();
			statusMessage.printf(_T("%s %d%%."), GetString(IDS_PW_STATUS_BUILDCLUSTER), per);
            UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
			if (bailoutManager.ShouldBail())
			{
				if (deleteEdges) delete edges;
				return FALSE;
			}
		}
	}
	else
	{
		for (int i = 0; i < normalList.Count(); i++)
		{
			sel.ClearAll();

			//find closest face norm
			SelectFacesByNormals(ld, sel, normalList[i], threshold, objNormList);
			//To be safe. In case the size of sel is changed in SelectFacesByNormals
			if (processedFaces.GetSize() != sel.GetSize())
				processedFaces.SetSize(sel.GetSize(), 1);
			if (processedFaces.GetSize() == 0)
			{
				DbgAssert(0);
				return FALSE;
			}

			//add cluster
			int numberSet = sel.NumberSet();

			if (numberSet > 0)
			{
				//create a cluster and add it
				ClusterClass *cluster = new ClusterClass();
				cluster->ld = ld;
				BOOL hit = FALSE;
				cluster->normal = normalList[i];
				BitArray clusterVerts;
				clusterVerts.SetSize(ld->GetNumberTVVerts());
				clusterVerts.ClearAll();
				int selSize = sel.GetSize();
				for (int j = 0; j < selSize; j++)
				{
					if (sel[j] && (!processedFaces[j]))
					{
						//add to cluster
						processedFaces.Set(j, TRUE);
						cluster->faces.Append(1, &j);
						ld->AddVertsToCluster(j, clusterVerts, cluster);
						hit = TRUE;
					}
				}
				if (hit)
					clusterList.Append(1, &cluster);
				else delete cluster;
			}

			int per = (i * 100) / normalList.Count();
			statusMessage.printf(_T("%s %d%%."), GetString(IDS_PW_STATUS_BUILDCLUSTER), per);
            UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
			if (bailoutManager.ShouldBail())
			{
				if (deleteEdges) delete edges;
				return FALSE;
			}

		}
	}
	//process the ramaining

	if (cleanUpStrayFaces)
	{
		int ct = 0;
		if (seedFaces.Count() > 0)
			ct = seedFaces[0];
		while ((processedFaces.NumberSet() != processedFaces.GetSize()))
		{
			if (!processedFaces[ct])
			{
				if (connected)
				{
					SelectFacesByGroup(ld, sel, ct, objNormList[ct], threshold, FALSE, groupBy, objNormList,
						clusterBorder,
						edges);

				}
				else SelectFacesByNormals(ld, sel, objNormList[ct], threshold, objNormList);
				//add cluster
				if (sel.NumberSet() > 0)
				{
					//create a cluster and add it
					ClusterClass *cluster = new ClusterClass();
					cluster->ld = ld;
					cluster->normal = objNormList[ct];
					BOOL hit = FALSE;

					BitArray clusterVerts;
					clusterVerts.SetSize(ld->GetNumberTVVerts());
					clusterVerts.ClearAll();

					int selSize = sel.GetSize();
					for (int j = 0; j < selSize; j++)
					{
						if (sel[j] && (!processedFaces[j]))
						{
							//add to cluster
							processedFaces.Set(j, TRUE);
							cluster->faces.Append(1, &j);
							ld->AddVertsToCluster(j, clusterVerts, cluster);
							hit = TRUE;
						}
					}
					if (connected)
					{
						cluster->borderData = clusterBorder;
					}
					if (hit)
					{
						clusterList.Append(1, &cluster);
					}
					else
					{
						delete cluster;
					}
				}
			}
			ct++;
			if (ct >= processedFaces.GetSize())
				ct = 0;

			int per = (processedFaces.NumberSet() * 100) / processedFaces.GetSize();
			statusMessage.printf(_T("%s %d%%."), GetString(IDS_PW_STATUS_BUILDCLUSTER), per);
            UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
			if (bailoutManager.ShouldBail())
			{
				if (deleteEdges) delete edges;
				return FALSE;
			}
		}
	}

	if (deleteEdges) delete edges;
	return TRUE;
}

BOOL UnwrapMod::BuildCluster( Tab<Point3> normalList, float threshold,
							 BOOL connected,
							 BOOL cleanUpStrayFaces,
							 MeshTopoData::GroupBy crossSmgrp)
{

	FreeClusterList();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BuildMeshTopoDataCluster(ld, normalList, threshold, connected, cleanUpStrayFaces,crossSmgrp, clusterList);
	}

	return TRUE;
}

void  UnwrapMod::NormalizeCluster(float spacing)
{

	TimeValue t = 0;
	if (GetCOREInterface())
		t = GetCOREInterface()->GetTime();

	float minx = FLT_MAX,miny = FLT_MAX;
	float maxx = FLT_MIN,maxy = FLT_MIN;
	bool bInit = false;
	for (int i =0; i < clusterList.Count(); i++)
	{
		MeshTopoData *ld = clusterList[i]->ld;
		for (int j = 0; j < clusterList[i]->verts.Count();j++)
		{
			int vIndex = clusterList[i]->verts[j];
			Point3 p = ld->GetTVVert(vIndex);
			bInit = true;
			if (p.x<minx) minx = p.x;
			if (p.y<miny) miny = p.y;
			if (p.x>maxx) maxx = p.x;
			if (p.y>maxy) maxy = p.y;
		}
	}

	float w = 0, h = 0;
	if (bInit)
	{
		w = maxx - minx;
		h = maxy - miny;
	}
	gBArea = w *h;
	

	for (int i =0; i < clusterList.Count(); i++)
	{
		MeshTopoData *ld = clusterList[i]->ld;
		for (int j = 0; j < clusterList[i]->verts.Count();j++)
		{
			int vIndex = clusterList[i]->verts[j];
			Point3 p = ld->GetTVVert(vIndex);
			p.x -= minx;
			p.y -= miny;
			ld->SetTVVert(t,vIndex,p);
		}
	}
	float amount = w > h ? w : h;
	if(fabs(amount) > std::numeric_limits<float>::epsilon())
	{
		for (int i =0; i < clusterList.Count(); i++)
		{
			MeshTopoData *ld = clusterList[i]->ld;
			for (int j = 0; j < clusterList[i]->verts.Count();j++)
			{
				int vIndex = clusterList[i]->verts[j];
				Point3 p = ld->GetTVVert(vIndex);
				p.x /= amount;
				p.y /= amount;
				ld->SetTVVert(t,vIndex,p);
			}
		}
	}

	if(fabs(spacing) > std::numeric_limits<float>::epsilon())
	{
		float mid = spacing;
		float scale = 1.0f - (spacing*2.0f);
		for (int i =0; i < clusterList.Count(); i++)
		{
			MeshTopoData *ld = clusterList[i]->ld;
			for (int j = 0; j < clusterList[i]->verts.Count();j++)
			{
				int vIndex = clusterList[i]->verts[j];
				Point3 p = ld->GetTVVert(vIndex);
				p.x *= scale;
				p.y *= scale;
				p.x += mid;
				p.y += mid;
				ld->SetTVVert(t,vIndex,p);
			}
		}
	}
}

class TempEdgeList
{
public:
	Tab<int> edges;
};

BOOL UnwrapMod::RotateClusters(float &finalArea)
{
	TimeValue t = 0;
	if (GetCOREInterface())
		t = GetCOREInterface()->GetTime();

	float totalArea = 0.0f;

	TSTR statusMessage;
    MaxSDK::Util::BailOutManager bailoutManager;
	for (int  i=0; i < clusterList.Count(); i++)
	{
		//compute
		MeshTopoData *ld = clusterList[i]->ld;
		BOOL done = FALSE;

		//build edges lists for this polygon
		Tab<TempEdgeList*> pEdges;
		pEdges.SetCount(clusterList[i]->faces.Count());
		for (int j =0; j < clusterList[i]->faces.Count(); j++)
		{
			pEdges[j] = new TempEdgeList();
			int faceIndex = clusterList[i]->faces[j];
			int degree = ld->GetFaceDegree(faceIndex);
			pEdges[j]->edges.SetCount(degree);
			for (int k = 0; k <  degree; k++)
			{
				int vID = ld->GetFaceTVVert(faceIndex,k);
				pEdges[j]->edges[k] = vID;
			}
		}

		Tab<Point3> transformedPoints, bestPoints;
		transformedPoints.SetCount(ld->GetNumberTVVerts());
		bestPoints.SetCount(ld->GetNumberTVVerts());

		for (int j =0; j < ld->GetNumberTVVerts(); j++)
		{
			transformedPoints[j] = ld->GetTVVert(j);
		}

		int angle = 0;

		float bestArea = -1.0f;
		int bestAngle = 0;
		float sArea =0.0f;
		float surfaceArea =0.0f;

		for (int j =0; j < clusterList[i]->faces.Count(); j++)
		{
			int faceIndex = clusterList[i]->faces[j];
			int faceCount = ld->GetFaceDegree(faceIndex);


			Tab<Point3> areaPoints;
			for (int k = 0; k <  faceCount; k++)
			{
				int vID = ld->GetFaceTVVert(faceIndex,k);
				// MAXX-32224/MAXX-33851 -- CER 131967111
				// Apparently there are some conditions where the vID can be out of range, causing a crash when the transformedPoints array is accessed with it.
				// We'll try to deal with it here, because the following code for computing area relies on there being the same number of values in the
				// areaPoints table...
				if (vID >= transformedPoints.Count())
				{
					DbgAssert(0);							// Try to catch this in debugging
					vID = transformedPoints.Count() - 1;	// Use a safe index
				}
				Point3 p = transformedPoints[vID];
				areaPoints.Append(1, &p);
			}
			//compute the area of the faces
			int degree = ld->GetFaceDegree(faceIndex);
			if (degree==3)
			{
				surfaceArea += AreaOfTriangle(areaPoints[0], areaPoints[1],areaPoints[2]);
			}
			else
			{
				surfaceArea += AreaOfPolygon(areaPoints);
			}
		}

		clusterList[i]->surfaceArea = surfaceArea;

		while (!done)
		{
			float boundsArea =0.0f;
			Box3 bounds;
			bounds.Init();
			//transforms the points
			if (angle != 0)
			{
				//float fangle = ((float) angle) * 180.0f/PI; // seems it is a bug? should be fangle = DegToRad(angle);
				float fangle = DegToRad(angle);
				for (int m = 0; m < clusterList[i]->verts.Count(); m++)
				{
					int j = clusterList[i]->verts[m];
					float x = ld->GetTVVert(j).x;
					float y = ld->GetTVVert(j).y;
					float tx = (x * cos(fangle)) - (y * sin(fangle));
					float ty = (x * sin(fangle)) + (y * cos(fangle));
					transformedPoints[j].x =  tx;
					transformedPoints[j].y =  ty;
				}
			}

			//build the list 
			for (int m = 0; m < clusterList[i]->verts.Count(); m++)
			{
				int vID = clusterList[i]->verts[m];
				Point3 p = transformedPoints[vID];
				bounds += p;
			}

			float w, h;

			h = bounds.pmax.y-bounds.pmin.y;
			w = bounds.pmax.x-bounds.pmin.x;

			boundsArea = (w*h);

			//compute the area of the bounding box
			if ((surfaceArea/boundsArea) > 0.95f )
			{
				done = TRUE;
				sArea = surfaceArea;
			}
			else
			{
				if ((boundsArea < bestArea) || (bestArea < 0.0f))
				{
					bestArea = boundsArea;
					bestPoints = transformedPoints;
					bestAngle = angle;
				}
			}

			angle+=2;

			if (angle >= 45)
			{
				done = TRUE;
				sArea = surfaceArea;
			}
		}
		if (bestAngle != 0)
		{
			for (int m = 0; m < clusterList[i]->verts.Count(); m++)
			{
				int vID = clusterList[i]->verts[m];
				ld->SetTVVert(t,vID,bestPoints[vID]);
			}
		}

		for (int j =0; j < pEdges.Count(); j++)
			delete pEdges[j];
		totalArea += sArea;

		int per = (i * 100)/clusterList.Count();
		statusMessage.printf(_T("%s %d%%."),GetString(IDS_PW_STATUS_ROTATING),per);
        UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
		if (bailoutManager.ShouldBail())
		{
			i = clusterList.Count();
			finalArea = totalArea;
			return FALSE;
		}

	}

	finalArea = totalArea;

	return TRUE;
}

void  UnwrapMod::JoinCluster(ClusterClass *baseCluster, ClusterClass *joinCluster)
{
	//append faces
	int ct= joinCluster->faces.Count();
	for (int i = 0; i < ct; i++)
	{
		baseCluster->faces.Append(1,&joinCluster->faces[i],100);
	}

	ct = joinCluster->boundsList.Count();
	for (int i = 0; i < ct; i++)
	{
		baseCluster->boundsList.Append(1,&joinCluster->boundsList[i],100);
	}
	for (int i = 0; i < joinCluster->verts.Count(); i++)
	{
		baseCluster->verts.Append(1,&joinCluster->verts[i],100);
	}

	baseCluster->normal = (baseCluster->normal + joinCluster->normal) *0.5f;
}


void  UnwrapMod::JoinCluster(ClusterClass *baseCluster, ClusterClass *joinCluster, float x, float y)
{
	Point3 offset = joinCluster->bounds.pmin;
	offset.x = x - offset.x;
	offset.y = y - offset.y;
	offset.z = 0.0f;

	TimeValue t = 0;
	if (GetCOREInterface())
		t = GetCOREInterface()->GetTime();

	for (int j = 0; j < joinCluster->verts.Count(); j++)
	{
		int vIndex = joinCluster->verts[j];
		Point3 p = joinCluster->ld->GetTVVert(vIndex);
		p += offset;
		joinCluster->ld->SetTVVert(t,vIndex,p);
	}

	//append faces
	int ct= joinCluster->faces.Count();
	for (int i = 0; i < ct; i++)
	{
		baseCluster->faces.Append(1,&joinCluster->faces[i]);
	}


	for (int i = 0; i < joinCluster->verts.Count(); i++)
	{
		baseCluster->verts.Append(1,&joinCluster->verts[i],100);
	}

	baseCluster->normal = (baseCluster->normal + joinCluster->normal) *0.5f;

	baseCluster->BuildList();
}


static int CompSubClusterArea( const void *elem1, const void *elem2 ) {
	SubClusterClass *ta = (SubClusterClass *)elem1;
	SubClusterClass *tb = (SubClusterClass *)elem2;

	float aH, bH;
	aH = ta->w * ta->h;
	bH = tb->w * tb->h;

	if (aH == bH) return 0;
	else if (aH > bH) return -1;
	else return 1;
}

BOOL UnwrapMod::CollapseClusters(float spacing, BOOL rotateClusters, BOOL onlyCenter)
{
	Tab<SubClusterClass> tempSubCluster;
	TSTR statusMessage;
	int clusterCount = clusterList.Count();
	for (int  i=0; i < clusterCount; i++)
	{
		clusterList[i]->BuildList();
	}

	for (int i=0; i < clusterCount; i++)
	{
		clusterList[i]->openSpaces.ZeroCount();
	}

    MaxSDK::Util::BailOutManager bailoutManager;
	for (int i=0; i < clusterCount; i++)
	{
		clusterList[i]->ComputeOpenSpaces(spacing, onlyCenter);

		int per = (i * 100)/clusterList.Count();
		statusMessage.printf(_T("%s %d%%."),GetString(IDS_PW_STATUS_DEADSPACE),per);
        UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
		if (bailoutManager.ShouldBail())
		{
			i = clusterList.Count();
			return FALSE;
		}

#ifdef DEBUGMODE
		if (gDebugLevel >=2)
		{
			NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
			ip->RedrawViews(ip->GetTime());
			InvalidateView();
			UpdateWindow(hView);
		}
#endif
	}

	//start at the back and work our way to the front
	for (int i=clusterList.Count()-2; i >= 0 ; i--)
	{

		BOOL bSubClusterDone = FALSE;
		int iSubClusterCount = 0;

#ifdef DEBUGMODE
		/*
		jointClusterBoxX.SetCount(clusterList[i]->openSpaces.Count());
		jointClusterBoxY.SetCount(clusterList[i]->openSpaces.Count());
		jointClusterBoxW.SetCount(clusterList[i]->openSpaces.Count());
		jointClusterBoxH.SetCount(clusterList[i]->openSpaces.Count());
		for (int jk = 0; jk < clusterList[i]->openSpaces.Count(); jk++)
		{
		jointClusterBoxX[jk] = clusterList[i]->openSpaces[jk].x;
		jointClusterBoxY[jk] = clusterList[i]->openSpaces[jk].y;
		jointClusterBoxW[jk] = clusterList[i]->openSpaces[jk].w;
		jointClusterBoxH[jk] = clusterList[i]->openSpaces[jk].h;
		}
		*/
#endif

		while ((!bSubClusterDone) &&  (iSubClusterCount < 4))
		{

			clusterList[i]->openSpaces.Sort(CompSubClusterArea);
			tempSubCluster.ZeroCount();



			for (int j = 0; j < clusterList[i]->openSpaces.Count(); j++)
			{
				SubClusterClass subCluster;
				subCluster = clusterList[i]->openSpaces[j];
				for (int k = i+1; k < clusterList.Count(); k++)
				{
					//see if any of the smaller cluster fits in our sub cluster
					float clusterW,clusterH;
					clusterW = clusterList[k]->w;
					clusterH = clusterList[k]->h;
					BOOL flip = FALSE;
					BOOL join = FALSE;
					if ( (clusterW< subCluster.w) && (clusterH< subCluster.h))
						join = TRUE;

					if ( rotateClusters && (clusterH< subCluster.w) && (clusterW< subCluster.h))
					{
						join = TRUE;
						flip = TRUE;
						float tw = clusterW;
						clusterW = clusterH;
						clusterH = tw;

					}

					if ( join && (clusterList[i]->ld == clusterList[k]->ld))
					{
						float subX = subCluster.x;
						float subY = subCluster.y;
						float subW = subCluster.w;
						float subH = subCluster.h;

#ifdef DEBUGMODE
						/*
						hitClusterBoxX = subX;
						hitClusterBoxY = subY;
						hitClusterBoxW = subW;
						hitClusterBoxH = subH;
						*/
#endif

						if (flip)
							FlipSingleCluster(k,spacing);

						JoinCluster(clusterList[i], clusterList[k], subX, subY); 

						//split the rect and put the remaining 2 pieces back in the list
						SubClusterClass newSpots[4];

						newSpots[0] = SubClusterClass(subX,subY+clusterH,clusterW,subH - clusterH);
						newSpots[1] = SubClusterClass(subX+clusterW,subY,subW-clusterW,subH);

						newSpots[2] = SubClusterClass(subX,subY+clusterH,subW,subH - clusterH);
						newSpots[3] = SubClusterClass(subX+clusterW,subY,subW-clusterW,clusterH);

						float cArea = newSpots[0].w*newSpots[0].h;
						int cIndex = 0;
						if ((newSpots[1].w*newSpots[1].h) > cArea)
						{
							cIndex = 1;
							cArea = newSpots[1].w*newSpots[1].h;
						}
						if ((newSpots[2].w*newSpots[2].h) > cArea)
						{
							cIndex = 2;
							cArea = newSpots[2].w*newSpots[2].h;
						}
						if ((newSpots[3].w*newSpots[3].h) > cArea)
						{
							cIndex = 3;
							cArea = newSpots[3].w*newSpots[3].h;
						}

						if (cIndex < 2)
						{
							if ((newSpots[0].h > 0.0f) && (newSpots[0].w > 0.0f))
								tempSubCluster.Append(1,&newSpots[0]);
							if ((newSpots[1].h > 0.0f) && (newSpots[1].w > 0.0f))
								tempSubCluster.Append(1,&newSpots[1]);
						}
						else
						{
							if ((newSpots[2].h > 0.0f) && (newSpots[2].w > 0.0f))
								tempSubCluster.Append(1,&newSpots[2]);
							if ((newSpots[3].h > 0.0f) && (newSpots[3].w > 0.0f))
								tempSubCluster.Append(1,&newSpots[3]);
						}
#ifdef DEBUGMODE
						if (gDebugLevel >= 2)
						{
							/*
							currentCluster = i;
							drawOnlyBounds = TRUE;
							NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
							ip->RedrawViews(ip->GetTime());
							InvalidateView();
							UpdateWindow(hView);
							drawOnlyBounds = FALSE;
							*/
						}
#endif

						delete clusterList[k];
						clusterList.Delete(k,1);

						k = clusterList.Count();
					}
				}
			}
			clusterList[i]->openSpaces.ZeroCount();
			clusterList[i]->openSpaces = tempSubCluster;
			iSubClusterCount++;
			if (clusterList[i]->openSpaces.Count() == 0) bSubClusterDone = TRUE;

		}

		int per = ((clusterList.Count()-i) * 100)/clusterList.Count();
		statusMessage.printf(_T("%s %d%%."),GetString(IDS_PW_STATUS_COLLAPSING),per);
        UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
		if (bailoutManager.ShouldBail())
		{
			i = clusterList.Count();
    		return FALSE;
		}

	}

	return TRUE;
}



void UnwrapMod::FlipSingleCluster(int i,float spacing)
{
	/*
	BitArray usedVerts;
	usedVerts.SetSize(TVMaps.v.Count());
	usedVerts.ClearAll();

	BuildUsedList(usedVerts ,i);

	*/

	TimeValue t = 0;
	if (GetCOREInterface())
		t = GetCOREInterface()->GetTime();

	clusterList[i]->bounds.Init();
	MeshTopoData *ld = clusterList[i]->ld;
	for (int j = 0; j < clusterList[i]->verts.Count(); j++)
	{
		int vIndex = clusterList[i]->verts[j];
		Point3 tempPoint;
		tempPoint = ld->GetTVVert(vIndex);
		Point3 p = tempPoint;
		p.x = -tempPoint.y;
		p.y = tempPoint.x;
		ld->SetTVVert(t,vIndex,p);
		clusterList[i]->bounds += p;
	}

	/*
	for (int j = 0; j < usedVerts.GetSize(); j++)
	{
	if (usedVerts[j])
	{  
	Point3 tempPoint;
	tempPoint = TVMaps.v[j].p;
	TVMaps.v[j].p.x = -tempPoint.y;
	TVMaps.v[j].p.y = tempPoint.x;
	if (TVMaps.cont[j]) TVMaps.cont[j]->SetValue(0,&TVMaps.v[j].p);
	clusterList[i]->bounds += TVMaps.v[j].p;
	}
	}
	*/

	//build offset
	float x,y;
	x = clusterList[i]->bounds.pmin.x;
	y = clusterList[i]->bounds.pmin.y;
	clusterList[i]->offset.x = x;
	clusterList[i]->offset.y = y;
	clusterList[i]->offset.z = 0.0f;

	float tw =  clusterList[i]->w;
	clusterList[i]->w = clusterList[i]->h;
	clusterList[i]->h = tw;

	clusterList[i]->h = clusterList[i]->h+spacing;
	clusterList[i]->w = clusterList[i]->w+spacing;

}

void  UnwrapMod::FlipClusters(BOOL flipW,float spacing)
{

	for (int i=0; i < clusterList.Count(); i++)
	{
		BOOL flip = FALSE;
		if (flipW)
		{
			if (clusterList[i]->h > clusterList[i]->w)
				flip = TRUE;
		}
		else
		{
			if (clusterList[i]->w > clusterList[i]->h)
				flip = TRUE;
		}
		if (flip)
		{
			FlipSingleCluster(i,spacing);
		}
	}
}

float UnwrapMod::PlaceClusters2(float area)
{

	BOOL done = FALSE;
	Tab<OpenAreaList> openAreaList;
	float edgeLength = 0.0f;
	edgeLength = sqrt(area);
	float edgeInc = edgeLength*0.01f;

	while (!done)
	{
		openAreaList.ZeroCount();
		//append to stack
		OpenAreaList tempSpot(0,0,edgeLength,edgeLength);
		openAreaList.Append(1,&tempSpot);
		//loop through polys
		BOOL suceeded = TRUE;

		for (int i = 0; i < clusterList.Count(); i++)
		{
			//look for hole
			int index = -1;
			float currentArea = 0.0f;
			currentArea = clusterList[i]->w * clusterList[i]->h;
			float bArea = -1.0f;
			for (int j =0; j < openAreaList.Count(); j++)
			{
				if ((clusterList[i]->w<=openAreaList[j].w) && (clusterList[i]->h<=openAreaList[j].h))
				{
					if (((currentArea < bArea) && (currentArea >= 0.0f)) || (bArea < 0.0f))
					{
						index = j;
						bArea = currentArea;
					}
				}
			}
			//if hole add to split stack and delete holw
			if (index < 0)  // failed to find a holw
			{
				suceeded = FALSE; //bail and resize grid
			}
			else
			{
				//now subdivide the area and find the one that is the best fit
				OpenAreaList newSpots[4];
				float x,y,w,h;
				x = openAreaList[index].x;
				y = openAreaList[index].y;
				w = openAreaList[index].w;    
				h = openAreaList[index].h;
				float clusterW, clusterH;

				clusterW = clusterList[i]->w;
				clusterH = clusterList[i]->h;

				if ((currentArea) > 0.0f)
				{
					newSpots[0] = OpenAreaList(x,y+clusterH,clusterW,h - clusterH);
					newSpots[1] = OpenAreaList(x+clusterW,y,w-clusterW,h);

					newSpots[2] = OpenAreaList(x,y+clusterH,w,h - clusterH);
					newSpots[3] = OpenAreaList(x+clusterW,y,w-clusterW,clusterH);

					float cArea = newSpots[0].area;
					int cIndex = 0;
					if (newSpots[1].area > cArea)
					{
						cIndex = 1;
						cArea = newSpots[1].area;
					}
					if (newSpots[2].area > cArea)
					{
						cIndex = 2;
						cArea = newSpots[2].area;
					}
					if (newSpots[3].area > cArea)
					{
						cIndex = 3;
						cArea = newSpots[3].area;
					}

					if (cIndex < 2)
					{
						openAreaList.Append(1,&newSpots[0]);
						openAreaList.Append(1,&newSpots[1]);
					}
					else
					{
						openAreaList.Append(1,&newSpots[2]);
						openAreaList.Append(1,&newSpots[3]);
					}

					//set the cluster newx, newy
					clusterList[i]->newX = openAreaList[index].x;
					clusterList[i]->newY = openAreaList[index].y;
					//delete that open spot since it is now occuppied
					openAreaList.Delete(index,1);
				}
			}

		}  
		if (suceeded)
		{
			//move all verts
			done = TRUE;
		}
		else
		{
			edgeLength += edgeInc;

		}

	}

	return edgeLength;

}

static int CompTable( const void *elem1, const void *elem2 ) {
	ClusterClass **ta = (ClusterClass **)elem1;
	ClusterClass **tb = (ClusterClass **)elem2;

	ClusterClass *a = *ta;
	ClusterClass *b = *tb;

	float aH, bH;
	aH = a->bounds.pmax.y - a->bounds.pmin.y;
	bH = b->bounds.pmax.y - b->bounds.pmin.y;

	if (aH == bH) return 0;
	else if (aH > bH) return -1;
	else return 1;

}

BOOL UnwrapMod::LayoutClusters2(float spacing, BOOL rotateClusters, BOOL combineClusters)
{

	TimeValue t = GetCOREInterface()->GetTime();

	//build bounding data
	float area = 0.0f;

	for (int i=0; i < clusterList.Count(); i++)
	{
		MeshTopoData *ld = clusterList[i]->ld;
		if (clusterList[i]->faces.Count() == 0) //check for clusters with no faces
		{
			delete clusterList[i];
			clusterList.Delete(i,1);
			i--;
		}
		else //check for clusters that have no points in them since we can get faces with no points
		{

			int totalPointCount = 0;
			for (int j =0; j < clusterList[i]->faces.Count(); j++)
			{
				int faceIndex = clusterList[i]->faces[j];
				totalPointCount += ld->GetFaceDegree(faceIndex);//TVMaps.f[faceIndex]->count;
			}
			if (totalPointCount==0)
			{
				delete clusterList[i];
				clusterList.Delete(i,1);
				i--;
			}
		}

	}
	gSArea = 0.0f;
	gBArea = 0.0f;
	if (rotateClusters)
	{
		if (!RotateClusters(gSArea)) return FALSE;
	}

	for ( int i=0; i < clusterList.Count(); i++)
	{
		CalculateClusterBounds(i);

		area += clusterList[i]->h * clusterList[i]->w;
	}


	spacing = sqrt(area)*spacing;

	area = 0.0f;


	for ( int i=0; i < clusterList.Count(); i++)
	{
		clusterList[i]->h = clusterList[i]->h+spacing;
		clusterList[i]->w = clusterList[i]->w+spacing;

		area += clusterList[i]->h * clusterList[i]->w;
	}

	for (int i=0; i < clusterList.Count(); i++)
	{
		clusterList[i]->newX = 0.0f;
		clusterList[i]->newY = 0.0f;
	}

	//sort bounding data by area 
	clusterList.Sort(UnwrapUtilityMethods::CompTableArea);

	if (combineClusters)
	{
		if (!CollapseClusters(spacing,rotateClusters, FALSE)) return FALSE;
	}

	//rotate all of them so the the width is the shortest
	if (rotateClusters)
	{
		FlipClusters(TRUE,spacing);
		float fEdgeLenW = PlaceClusters2(area);

		FlipClusters(FALSE,spacing);
		float fEdgeLenH = PlaceClusters2(area);

		if (fEdgeLenW < fEdgeLenH)
		{
			FlipClusters(TRUE,spacing);
			PlaceClusters2(area);

		}
	}
	else
	{
		PlaceClusters2(area);
	}


	//now move vertices
	for (int i=0; i < clusterList.Count(); i++)
	{
		clusterList[i]->offset.x = clusterList[i]->newX-clusterList[i]->offset.x;
		clusterList[i]->offset.y = clusterList[i]->newY-clusterList[i]->offset.y;


		MeshTopoData *ld = clusterList[i]->ld;
		for (int j = 0; j <  clusterList[i]->verts.Count(); j++)
		{
			int vIndex = clusterList[i]->verts[j];
			Point3 p = ld->GetTVVert(vIndex);
			p += clusterList[i]->offset;//	TVMaps.v[index].p + clusterList[i]->offset;
			ld->SetTVVert(t,vIndex,p);
		}
	}

	return TRUE;

}

BOOL UnwrapMod::LayoutClusters3(float spacing, BOOL rotateClusters, BOOL combineClusters)
{

	spacing *= 2.0f;

	float totalWidth = 0;
	//build bounding data

	float area = 0.0f;

	for (int i=0; i < clusterList.Count(); i++)
	{
		if (clusterList[i]->faces.Count() == 0) //check for clusters with no faces
		{
			delete clusterList[i];
			clusterList.Delete(i,1);
			i--;
		}
		else //check for clusters that have no points in them since we can get faces with no points
		{

			int totalPointCount = 0;
			for (int j =0; j < clusterList[i]->faces.Count(); j++)
			{
				int faceIndex = clusterList[i]->faces[j];
				MeshTopoData *ld = clusterList[i]->ld;
				totalPointCount += ld->GetFaceDegree(faceIndex);//TVMaps.f[faceIndex]->count;
			}
			if (totalPointCount==0)
			{
				delete clusterList[i];
				clusterList.Delete(i,1);
				i--;
			}
		}

	}
	gSArea = 0.0f;
	gBArea = 0.0f;
	if (rotateClusters)
	{
		if (!RotateClusters(gSArea)) return FALSE;
	}


	for ( int i=0; i < clusterList.Count(); i++)
	{

		clusterList[i]->h = 0.0f;
		clusterList[i]->w = 0.0f;


		clusterList[i]->bounds.Init();
		for (int j =0; j < clusterList[i]->faces.Count(); j++)
		{
			int faceIndex = clusterList[i]->faces[j];
			MeshTopoData *ld = clusterList[i]->ld;
			int degree = ld->GetFaceDegree(faceIndex);
			for (int k = 0; k < degree; k++)
			{
				//need to put patch handles in here also
				int index = ld->GetFaceTVVert(faceIndex,k);//TVMaps.f[faceIndex]->t[k];
				Point3 a = ld->GetTVVert(index);//TVMaps.v[index].p;
				clusterList[i]->bounds += a;

				if (ld->GetFaceHasVectors(faceIndex))//(TVMaps.f[faceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[faceIndex]->vecs))
				{
					index = ld->GetFaceTVHandle(faceIndex,k*2);//TVMaps.f[faceIndex]->vecs->handles[k*2];
					a = ld->GetTVVert(index);//TVMaps.v[index].p;
					clusterList[i]->bounds += a;

					index = ld->GetFaceTVHandle(faceIndex,k*2+1);//TVMaps.f[faceIndex]->vecs->handles[k*2+1];
					a = ld->GetTVVert(index);//TVMaps.v[index].p;
					clusterList[i]->bounds += a;

					if (ld->GetFaceHasInteriors(faceIndex))//TVMaps.f[faceIndex]->flags & FLAG_INTERIOR)
					{
						index = ld->GetFaceTVInterior(faceIndex,k);//TVMaps.f[faceIndex]->vecs->interiors[k];
						a = ld->GetTVVert(index);//TVMaps.v[index].p;
						clusterList[i]->bounds += a;
					}
				}
			}
			//FIX
			Point3 width = clusterList[i]->bounds.Width();
			Point3 pmin,pmax;
			pmin = clusterList[i]->bounds.pmin;
			pmax = clusterList[i]->bounds.pmax;
			if ( width.x == 0.0f )
			{
				Point3 p = pmin;
				p.x -= 0.0001f;
				clusterList[i]->bounds += p;
				p = pmax;
				p.x += 0.0001f;
				clusterList[i]->bounds += p;

			}
			if ( width.y == 0.0f)
			{
				Point3 p = pmin;
				p.y -= 0.0001f;
				clusterList[i]->bounds += p;
				p = pmax;
				p.y += 0.0001f;
				clusterList[i]->bounds += p;

			}


		}
		//build offset
		float x,y;
		x = clusterList[i]->bounds.pmin.x;
		y = clusterList[i]->bounds.pmin.y;
		clusterList[i]->offset.x = x;
		clusterList[i]->offset.y = y;
		clusterList[i]->offset.z = 0.0f;
		if (clusterList[i]->faces.Count() == 0)
		{
			clusterList[i]->h = 0.0f;
			clusterList[i]->w = 0.0f;

		}
		else
		{
			clusterList[i]->h = clusterList[i]->bounds.pmax.y-clusterList[i]->bounds.pmin.y;
			clusterList[i]->w = clusterList[i]->bounds.pmax.x-clusterList[i]->bounds.pmin.x;
			if (clusterList[i]->h == 0.0f) clusterList[i]->h = 0.5f;
			if (clusterList[i]->w == 0.0f) clusterList[i]->w = 0.5f;
		}

		clusterList[i]->h += clusterList[i]->h*spacing;
		clusterList[i]->w += clusterList[i]->w*spacing;

		totalWidth +=clusterList[i]->w;
		area += clusterList[i]->h * clusterList[i]->w;
	}



	//sort bounding data by area 
	clusterList.Sort(UnwrapUtilityMethods::CompTableArea);

	if (combineClusters)
	{
		if (!CollapseClusters(spacing,rotateClusters, FALSE)) return FALSE;
	}

	//rotate all of them so the the width is the shortest
	if (rotateClusters)
	{
		FlipClusters(TRUE,spacing);
		float fEdgeLenW = PlaceClusters2(area);

		FlipClusters(FALSE,spacing);
		float fEdgeLenH = PlaceClusters2(area);

		if (fEdgeLenW < fEdgeLenH)
		{
			FlipClusters(TRUE,spacing);
			PlaceClusters2(area);
		}
	}
	else
	{
		PlaceClusters2(area);
	}


	//now move vertices

	TimeValue t = GetCOREInterface()->GetTime();
	for (int i=0; i < clusterList.Count(); i++)
	{
		clusterList[i]->offset.x = clusterList[i]->newX-clusterList[i]->offset.x;
		clusterList[i]->offset.y = clusterList[i]->newY-clusterList[i]->offset.y;
		MeshTopoData *ld = clusterList[i]->ld;
		for (int j = 0; j <  clusterList[i]->verts.Count(); j++)
		{
			int vindex = clusterList[i]->verts[j];
			Point3 p = ld->GetTVVert(vindex);
			p = p + clusterList[i]->offset;
			ld->SetTVVert(t,vindex,p);
		}
	}
	return TRUE;
}


BOOL UnwrapMod::LayoutClusters(float spacing,BOOL rotateClusters, BOOL alignWidth, BOOL combineClusters)
{

	TimeValue t = GetCOREInterface()->GetTime();
	gSArea = 0.0f;
	gBArea = 0.0f;
	if (rotateClusters)
	{
		if (!RotateClusters(gSArea)) return FALSE;
	}

	float totalWidth = 0;
	float area = 0.0f;
	//build bounding data

	for (int i=0; i < clusterList.Count(); i++)
	{
		clusterList[i]->bounds.Init();
		MeshTopoData *ld = clusterList[i]->ld;
		for (int j =0; j < clusterList[i]->verts.Count(); j++)
		{
			int vIndex = clusterList[i]->verts[j];
			clusterList[i]->bounds += ld->GetTVVert(vIndex);
		}
		/*
		for (int j =0; j < clusterList[i]->faces.Count(); j++)
		{
		int faceIndex = clusterList[i]->faces[j];
		for (int k = 0; k <  TVMaps.f[faceIndex]->count; k++)
		{
		//need to put patch handles in here also
		int index = TVMaps.f[faceIndex]->t[k];

		Point3 a = TVMaps.v[index].p;
		clusterList[i]->bounds += a;

		if ( (TVMaps.f[faceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[faceIndex]->vecs))
		{
		if (TVMaps.f[faceIndex]->flags & FLAG_INTERIOR) 
		{

		index = TVMaps.f[faceIndex]->vecs->interiors[k];
		if ((index >=0) && (index < TVMaps.v.Count()))
		{
		Point3 a = TVMaps.v[index].p;
		clusterList[i]->bounds += a;
		}
		}

		index = TVMaps.f[faceIndex]->vecs->handles[k*2];
		if ( (index >=0) && (index < TVMaps.v.Count()))
		{
		Point3 a = TVMaps.v[index].p;
		clusterList[i]->bounds += a;
		}

		index = TVMaps.f[faceIndex]->vecs->handles[k*2+1];
		if ((index >=0) && (index < TVMaps.v.Count()))
		{
		Point3 a = TVMaps.v[index].p;
		clusterList[i]->bounds += a;
		}

		}

		}


		}
		*/

		Point3 width = clusterList[i]->bounds.Width();
		Point3 pmin,pmax;
		pmin = clusterList[i]->bounds.pmin;
		pmax = clusterList[i]->bounds.pmax;
		if ( width.x == 0.0f )
		{
			Point3 p = pmin;
			p.x -= 0.0001f;
			clusterList[i]->bounds += p;
			p = pmax;
			p.x += 0.0001f;
			clusterList[i]->bounds += p;

		}
		if ( width.y == 0.0f)
		{
			Point3 p = pmin;
			p.y -= 0.0001f;
			clusterList[i]->bounds += p;
			p = pmax;
			p.y += 0.0001f;
			clusterList[i]->bounds += p;

		}

		//build offset
		float x,y;
		x = clusterList[i]->bounds.pmin.x;
		y = clusterList[i]->bounds.pmin.y;
		clusterList[i]->offset.x = x;
		clusterList[i]->offset.y = y;
		clusterList[i]->offset.z = 0.0f;
		clusterList[i]->h = clusterList[i]->bounds.pmax.y-clusterList[i]->bounds.pmin.y;
		clusterList[i]->w = clusterList[i]->bounds.pmax.x-clusterList[i]->bounds.pmin.x;

		if (clusterList[i]->h == 0.0f) clusterList[i]->h = 0.5f;
		if (clusterList[i]->w == 0.0f) clusterList[i]->w = 0.5f;


		area += clusterList[i]->h*clusterList[i]->w;

		totalWidth +=clusterList[i]->w;
	}


	spacing = sqrt(area)*spacing;

	totalWidth =0.0f;
	area = 0.0f;

	for ( int i=0; i < clusterList.Count(); i++)
	{
		clusterList[i]->h = clusterList[i]->h+spacing;
		clusterList[i]->w = clusterList[i]->w+spacing;

		totalWidth +=clusterList[i]->w;
		area += clusterList[i]->h * clusterList[i]->w;

	}

	if (rotateClusters)
		FlipClusters(alignWidth,spacing);



	//sort bounding data by height 
	clusterList.Sort(CompTable);

	//    BOOL combineClusters = TRUE;
	if (combineClusters)
	{
		if (!CollapseClusters(spacing,rotateClusters, FALSE)) return FALSE;
	}



	totalWidth = 0.0f;
	for ( int i=0; i < clusterList.Count(); i++)
		totalWidth += clusterList[i]->w;

	//now shuffle the clusters to fit in normalized space
	int split = 2;
	BOOL done = FALSE;

	while (!done)
	{
		Tab<int> splitList;
		splitList.SetCount(split);


		float maxWidth = totalWidth/(float) split;

		for (int i =0; i < splitList.Count(); i++)
			splitList[i] = -1;

		splitList[0] = 0;
		int index =1;

		int currentChunk = 0;


		while (index < split)
		{
			float currentWidth = 0.f;
			while ( (currentWidth < maxWidth) && (currentChunk < (clusterList.Count()-1)) )
			{
				currentWidth +=  clusterList[currentChunk]->w;
				currentChunk++;
			}

			splitList[index] = currentChunk;
			index++;
		}

		float totalHeight = 0.0f;
		for (int i = 0; i <  splitList.Count(); i++)
		{
			if ( (splitList[i] >=0) && (splitList[i] <clusterList.Count()) )
				totalHeight += clusterList[splitList[i]]->h;
		}
		if (totalHeight >= maxWidth)
		{
			done = TRUE;
			//now move points to there respective spots
			int start =0;
			Point3 corner(0.0f,0.0f,0.0f);
			for (int i =0; i < splitList.Count(); i++)
			{
				if (splitList[i] >=0)
				{
					int end;
					if (i == splitList.Count()-1)
						end = clusterList.Count();
					else end = splitList[i+1];
					float xOffset =0;
					for (int j = start; j < end; j++)
					{
						Point3 boundingCorner;
						boundingCorner.x = clusterList[j]->bounds.pmin.x;
						boundingCorner.y = clusterList[j]->bounds.pmin.y;
						boundingCorner.z = 0;
						Point3 vec = corner - boundingCorner;
						clusterList[j]->offset = vec;
						clusterList[j]->offset.x += xOffset;
						xOffset += clusterList[j]->w;


					}
					if (i < splitList.Count()-1)
					{
						if (splitList[i] < clusterList.Count())
						{
							start = splitList[i + 1];
							corner.y += clusterList[splitList[i]]->h;
						}
						else
						{
							DbgAssert(0);	// MAXX-32224/MAXX-33851, CER 133124274
						}
					}
				}
			}

			for (int i=0; i < clusterList.Count(); i++)
			{
				MeshTopoData *ld = clusterList[i]->ld;
				for (int j = 0; j <  clusterList[i]->verts.Count(); j++)
				{
					int vIndex = clusterList[i]->verts[j];
					Point3 p = ld->GetTVVert(vIndex) + clusterList[i]->offset;
					ld->SetTVVert(t,vIndex,p);
				}
			}
		}
		else split++;

	}

	return TRUE;

}

void UnwrapMod::ComputeGeometryAreaRatio(std::vector<float>& vecRadios)
{
	//Compute the area radio of every node to the first one's geometry area.
	if(mMeshTopoData.Count() > 1)
	{
		int in1, in2;
		GetUVWIndices(in1,in2);
		TimeValue t = GetCOREInterface()->GetTime();
		float firstGeomAreaTotal = 0.0;
		float firstUVAreaTotal = 0.0;
		float fFirstRatio = 0.0;
		float fDeltaRadio = 0.0;
		int fFirstID = 0;
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *md = mMeshTopoData[ldID];
			if(NULL == md)
			{
				continue;
			}

			Matrix3 mat;
			mat.IdentityMatrix();
			INode* pNode = GetMeshTopoDataNode(ldID);
			if(pNode)
			{
				mat = pNode->GetObjectTM(t);
			}
			
			float geomAreaTotal = 0.0;
			float uvAreaTotal   = 0.0;
			for (int i = 0; i < md->GetNumberFaces(); i++)
			{
				if (md->CheckPoly(i))
				{
					int degree = md->GetFaceDegree(i);
					Tab<Point3> plistGeom;
					plistGeom.SetCount(degree);
					Tab<Point3> plistUVW;
					plistUVW.SetCount(degree);
					for (int m = 0; m < degree; m++)
					{
						int index = md->GetFaceGeomVert(i,m);
						plistGeom[m] = md->GetGeomVert(index)*mat;

						index = md->GetFaceTVVert(i,m);
						plistUVW[m] = Point3(md->GetTVVert(index)[in1],md->GetTVVert(index)[in2],0.0f);
					}
					geomAreaTotal += AreaOfPolygon(plistGeom);
					uvAreaTotal   += AreaOfPolygon(plistUVW);
				}
			}

			if(ldID == fFirstID)
			{
				firstGeomAreaTotal = geomAreaTotal;
				firstUVAreaTotal   = uvAreaTotal;
				if(firstGeomAreaTotal != 0.0)
				{
					fFirstRatio =  sqrt(firstUVAreaTotal/firstGeomAreaTotal);
				}
				else
				{
					// use the first non-zero area geometry as the base
					fFirstID++;
				}

				vecRadios.push_back(1.0);
			}
			else
			{
				if(geomAreaTotal != 0.0 && uvAreaTotal != 0.0)
				{					
					float fOtherRatio = sqrt(uvAreaTotal/geomAreaTotal);
					fDeltaRadio = (fFirstRatio - fOtherRatio)/fOtherRatio;
				}

				vecRadios.push_back(1.0+fDeltaRadio);
			}
		}
	}
}

float  UnwrapMod::Pack(int layoutType, float spacing, BOOL normalize, BOOL rotateClusters, BOOL fillHoles, BOOL buildClusters, BOOL useSelection)
{
	TimeValue t = GetCOREInterface()->GetTime();
	theHold.Begin();
	HoldPointsAndFaces();   

	//rescale the clusters if need be before any packing happens
	if (packRescaleCluster)
	{
		theHold.Suspend();
		if(useSelection)
		{
			RescaleSelectedCluster();
		}
		else
		{
			RescaleSelectedCluster(FALSE);
		}
		theHold.Resume();
	}

	std::vector<float> vecFRadios;
	if(packRescaleCluster && 
		mMeshTopoData.Count() > 1)
	{
		ComputeGeometryAreaRatio(vecFRadios);
	}
	

	TSTR statusMessage;
	BOOL bContinue = TRUE;

	//FIX
	int holdSubMode = fnGetTVSubMode();

	if (buildClusters)
		FreeClusterList();

	//Firstly ,find if some faces are selected in one object or among multi-objects.
	//Secondly,find if some faces are filtered when all faces are unselected.
	bool bPartialInvolved = false;
	if (useSelection)
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			BitArray fsel = ld->GetFaceSel();
			//should not only consider the face selection, but also the face filter and materialID filter.
			if(fsel.AnyBitSet() ||
				ld->IsFaceFilterBitsetOrMaterialIDFilterSet())
			{
				bPartialInvolved = true;
				break;
			}
		}
	}

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray fsel = ld->GetFaceSel();
		BitArray vsel = ld->GetTVVertSel();

		BitArray fselHold;
		fselHold = fsel;
		if (holdSubMode == TVVERTMODE)
		{
			ld->GetFaceSelFromVert(fselHold, FALSE);
		}
		else if (holdSubMode == TVEDGEMODE)
		{
			BitArray vselTemp = vsel;
			ld->GetVertSelFromEdge(vsel);
			ld->SetTVVertSel(vsel);
			ld->GetFaceSelFromVert(fselHold,FALSE);    		  
			vsel = vselTemp;
			ld->SetTVVertSel(vsel);
		}

		if (buildClusters)
			bContinue = BuildClusterFromTVVertexElement(ld);

		gBArea = 0.0f;
		//use only selected faces if find some faces are selected and the flag useSelection is true.
		//otherwise, all faces will be packed.
		if (useSelection && bPartialInvolved)
		{
			bool bFaceSeleced = fselHold.AnyBitSet();
			bool bFilterSet = ld->IsFaceFilterBitsetOrMaterialIDFilterSet();
			if(bFaceSeleced ||
				bFilterSet)
			{
				for (int i =0; i < clusterList.Count(); i++)
				{
					BOOL involved = FALSE;
					if (clusterList[i]->ld == ld)
					{
						for (int j = 0; j <  clusterList[i]->faces.Count(); j++)
						{        
							int findex = clusterList[i]->faces[j];
							if ((bFaceSeleced && fselHold[findex]) ||
								(bFilterSet && ld->DoesFacePassFilter(findex)))
							{
								involved = TRUE;
								break;
							}

						}
						if (!involved)
						{
							//Remove the unselected faces in one object
							delete clusterList[i];
							clusterList.Delete(i,1);
							i--;
						}
					}
				}
			}
			else
			{
				//when multi-objects are packed together,some objects may be unselected and they should be removed.
				if(mMeshTopoData.Count() > 1)
				{
					for (int i =0; i < clusterList.Count(); i++)
					{
						if (clusterList[i]->ld == ld)
						{
							delete clusterList[i];
							clusterList.Delete(i,1);
							i--;
						}
					}
				}				
			}			
		}		

		//recover to its vertex original selection because the function BuildClusterFromTVVertexElement may change the vertex selection.
		if (buildClusters)
		{
			ld->SetTVVertSel(vsel);
		}
	}

	if (bContinue)
	{
		for (int i =0; i < clusterList.Count(); i++)
		{
			Box3 bounds;
			bounds.Init();
			MeshTopoData *ld = clusterList[i]->ld;

			for (int j = 0; j <  clusterList[i]->verts.Count(); j++)
			{
				int index = clusterList[i]->verts[j];
				bounds += ld->GetTVVert(index);
			}

			//find the scale  of this cluster based on each node's geometry area comparing to the first one
			//when multi nodes exist
			float fScale = 1.0;
			if(packRescaleCluster && 
				mMeshTopoData.Count() > 1)
			{
				for (int ldIndex = 0; ldIndex < mMeshTopoData.Count(); ldIndex++)
				{
					if (ld == mMeshTopoData[ldIndex])
					{
						fScale = vecFRadios[ldIndex];
						break;
					}
				}
			}

			Point3 center = bounds.Center();
			//center all the the clusters
			for (int j =0; j < clusterList[i]->verts.Count(); j++)
			{
				int index =  clusterList[i]->verts[j];
				Point3 p = ld->GetTVVert(index)-center;
				p = p*fScale;
				ld->SetTVVert(t,index,p);
			}
		}

		if (layoutType == unwrap_linear_pack)
			bContinue = LayoutClusters( spacing, rotateClusters, TRUE,fillHoles);
		else if (layoutType == unwrap_non_convex_pack)
			bContinue = LayoutClustersXY( spacing, rotateClusters, fillHoles, this);
		else //layoutType == unwrap_recursive_pack
			bContinue = LayoutClusters2( spacing, rotateClusters, fillHoles);

		//normalize map to 0,0 to 1,1
		if ((bContinue) && (normalize))
		{
			NormalizeCluster(spacing);
		}

		//if the valid position and scale value stored in one box have been got before some operations such as Peel Rest,
		//then the selections need to restore to the recorded position and scale after the Peel Reset operation.
		if(mbValidBBoxOfRecord)
		{
			RestoreToBBoxOfRecord();
		}

	}

	CleanUpDeadVertices();

	if (bContinue)
	{
		theHold.Accept(GetString(IDS_PW_PACK));
	}
	else
	{
		theHold.Cancel();
	}

	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	InvalidateView();

	float fABArea = 0.0f;
	if(gBArea > std::numeric_limits<float>::epsilon() )
		fABArea = gSArea/gBArea;

#ifdef DEBUGMODE 
	if (gDebugLevel >= 1)
	{
		int finalCluster = clusterList.Count();
		gEdgeHeight = 0.0f;
		gEdgeWidth = 0.0f;
		for (int i =0; i < clusterList.Count(); i++)
		{
			gEdgeHeight += clusterList[i]->h;
			gEdgeWidth += clusterList[i]->w;

		}

		ScriptPrint(_T("Surface Area %f bounds area %f  per used %f\n"),gSArea,gBArea,fABArea); 
		ScriptPrint(_T("Edge Height %f Edge Width %f\n"),gEdgeHeight,gEdgeWidth); 
		ScriptPrint(_T("Initial Clusters %d finalClusters %d\n"),initialCluster,finalCluster); 
	}

#endif

	FreeClusterList();

	statusMessage.printf(_T("%s %3.2f"), GetString(IDS_PW_AREACOVERAGE), (fABArea)*100.f);
    UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());

	return fABArea;
}



float  UnwrapMod::fnPack( int method, float spacing, BOOL normalize, BOOL rotate, BOOL fillHoles)
{
	return Pack(method, spacing, normalize, rotate, fillHoles);
}

void  UnwrapMod::fnPackNoParams()
{
	// between clusters, each padding the same amount, so make input half.
	Pack(packMethod, packSpacing * 0.5f, packNormalize, packRotate, packFillHoles);
}

void  UnwrapMod::fnPackDialog()
{
	//bring up the dialog
	DialogBoxParam(   hInstance,
		MAKEINTRESOURCE(IDD_PACKDIALOG),
		hDialogWnd,
		UnwrapPackFloaterDlgProc,
		(LPARAM)this );
}

void  UnwrapMod::SetPackDialogPos()
{
	if (packWindowPos.length != 0) 
		SetWindowPlacement(packHWND,&packWindowPos);
}

void  UnwrapMod::SavePackDialogPos()
{
	packWindowPos.length = sizeof(WINDOWPLACEMENT); 
	GetWindowPlacement(packHWND,&packWindowPos);
}

INT_PTR CALLBACK UnwrapPackFloaterDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam); commented out by sca 10/7/98 -- causing warning since unused.

	static ISpinnerControl *iSpacing = NULL;
	static float iSpacingScale = 1.0f;

	switch (msg) {
	case WM_INITDIALOG:

		{
			mod = (UnwrapMod*)lParam;
			UnwrapMod::packHWND = hWnd;

			DLSetWindowLongPtr(hWnd, lParam);

			int axis=0;
			bool spinnerPixelUnits = mod->IsSpinnerPixelUnits(IDC_UNWRAP_SPACINGSPIN,&axis);

			//create spinners and set value
			if( spinnerPixelUnits )
			{
				iSpacingScale = mod->GetScalePixelUnits(axis);
				iSpacing = SetupIntSpinner(
					hWnd,IDC_UNWRAP_SPACINGSPIN,IDC_UNWRAP_SPACING,
					0.0f,iSpacingScale,iSpacingScale*mod->packSpacing); 
			}
			else
			{
				iSpacingScale = 1.0f;
				iSpacing = SetupFloatSpinner(
					hWnd,IDC_UNWRAP_SPACINGSPIN,IDC_UNWRAP_SPACING,
					0.0f,1.0f,mod->packSpacing);  
			}

			//set align cluster
			CheckDlgButton(hWnd,IDC_NORMALIZE_CHECK,mod->packNormalize);
			CheckDlgButton(hWnd,IDC_ROTATE_CHECK,mod->packRotate);
			CheckDlgButton(hWnd,IDC_COLLAPSE_CHECK,mod->packFillHoles);
			CheckDlgButton(hWnd,IDC_RESCALECLUSTER_CHECK,mod->packRescaleCluster);

			HWND hMethod = GetDlgItem(hWnd,IDC_COMBO1);
			SendMessage(hMethod, CB_RESETCONTENT, 0, 0);

			SendMessage(hMethod, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_RECURSIVEPACK));
			SendMessage(hMethod, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_LINEARPACK));
			SendMessage(hMethod, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_NON_CONVEX));

			SendMessage(hMethod, CB_SETCURSEL, mod->packMethod, 0L);

			mod->SetPackDialogPos();

			break;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OK:
			{
				mod->SavePackDialogPos();

				mod->packSpacing = iSpacing->GetFVal() / iSpacingScale;

				mod->packNormalize = IsDlgButtonChecked(hWnd,IDC_NORMALIZE_CHECK);
				mod->packRotate = IsDlgButtonChecked(hWnd,IDC_ROTATE_CHECK);
				mod->packFillHoles = IsDlgButtonChecked(hWnd,IDC_COLLAPSE_CHECK); 
				mod->packRescaleCluster = IsDlgButtonChecked(hWnd,IDC_RESCALECLUSTER_CHECK); 

				HWND hMethod = GetDlgItem(hWnd,IDC_COMBO1);
				mod->packMethod = SendMessage(hMethod, CB_GETCURSEL, 0L, 0);

				mod->fnPackNoParams();

				ReleaseISpinner(iSpacing);
				iSpacing = NULL;

				EndDialog(hWnd,1);

				break;
			}
		case IDC_CANCEL:
			{
				mod->SavePackDialogPos();
				ReleaseISpinner(iSpacing);
				iSpacing = NULL;

				EndDialog(hWnd,0);

				break;
			}
		case IDC_DEFAULT:
			{
				//get bias
				mod->packSpacing = iSpacing->GetFVal() / iSpacingScale;

				//get align
				mod->packNormalize = IsDlgButtonChecked(hWnd,IDC_NORMALIZE_CHECK);
				mod->packRotate = IsDlgButtonChecked(hWnd,IDC_ROTATE_CHECK);
				mod->packFillHoles = IsDlgButtonChecked(hWnd,IDC_COLLAPSE_CHECK); 


				HWND hMethod = GetDlgItem(hWnd,IDC_COMBO1);
				mod->packMethod = SendMessage(hMethod, CB_GETCURSEL, 0L, 0);
				mod->packRescaleCluster = IsDlgButtonChecked(hWnd,IDC_RESCALECLUSTER_CHECK); 

				//set as defaults
				mod->fnSetAsDefaults(PACKSECTION);
				break;
			}

		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}


bool  UnwrapMod::BuildBBoxOfRecord()
{

	mBBoxOfRecord.Init();
	bool bAddPointToBBox = false;

	for (int i = 0; i < mMeshTopoData.Count(); i++)
	{
		MeshTopoData* ld = mMeshTopoData[i];

		//Get clusters
		BitArray usedFaces;
		usedFaces.SetSize(ld->GetNumberFaces());
		usedFaces.ClearAll();

		BitArray faceSel = ld->GetFaceSel();		
		usedFaces = ~faceSel; 

		int numberFaces = ld->GetNumberFaces();
		int startFace = 0;

		//find our clusters
		
		while (numberFaces != usedFaces.NumberSet())
		{
			Tab<int> faces;

			LSCMLocalData::GetTVCluster(this, ld, startFace, usedFaces, faces);

			if(faces.Count() > 0)
			{
				BitArray VertProcessed;

				VertProcessed.SetSize(ld->GetNumberTVVerts());
				VertProcessed.ClearAll();
				for (int i = 0; i < faces.Count(); i++)
				{
					int faceIndex = faces[i];

					int ct = ld->GetFaceDegree(faceIndex);
					for (int k =0; k < ct; k++)
					{
						int vIndex = ld->GetFaceTVVert(faceIndex,k);
						if(!VertProcessed[vIndex])
						{
							VertProcessed.Set(vIndex,TRUE);
							Point3 p = ld->GetTVVert(vIndex);

							mBBoxOfRecord += p;
							bAddPointToBBox = true;
						}
					}
				}
			}
		}
	}

	return bAddPointToBBox;
}

void  UnwrapMod::RestoreToBBoxOfRecord()
{
	TimeValue t = GetCOREInterface()->GetTime();

	float minx = FLT_MAX,miny = FLT_MAX;
	float maxx = FLT_MIN,maxy = FLT_MIN;
	for (int i =0; i < clusterList.Count(); i++)
	{
		MeshTopoData *ld = clusterList[i]->ld;
		for (int j = 0; j < clusterList[i]->verts.Count();j++)
		{
			int vIndex = clusterList[i]->verts[j];
			Point3 p = ld->GetTVVert(vIndex);
			if (p.x<minx) minx = p.x;
			if (p.y<miny) miny = p.y;
			if (p.x>maxx) maxx = p.x;
			if (p.y>maxy) maxy = p.y;
		}
	}

	float w = 1.0;
	float h = 1.0;
	w = maxx-minx;
	h = maxy-miny;
	float amount = h;
	if (w > h) 
		amount = w;

	float fRecordW = 1.0;
	float fRecordH = 1.0;
	fRecordW = mBBoxOfRecord.pmax.x - mBBoxOfRecord.pmin.x;
	fRecordH = mBBoxOfRecord.pmax.y - mBBoxOfRecord.pmin.y;
	float originalAmount = fRecordH;
	if(fRecordW > fRecordH)
		originalAmount = fRecordW;

	float fScale	= originalAmount/amount;
	float fRecordMinX = mBBoxOfRecord.pmin.x;
	float fRecordMinY = mBBoxOfRecord.pmin.y;

	for (int i=0; i < clusterList.Count(); i++)
	{

		MeshTopoData *ld = clusterList[i]->ld;
		for(int j = 0; j < clusterList[i]->verts.Count();j++)
		{
			int vIndex = clusterList[i]->verts[j];
			Point3 p = ld->GetTVVert(vIndex);
			p.x -= minx;
			p.y -= miny;
			p.x *= fScale;
			p.y *= fScale;
			p.x += fRecordMinX;
			p.y += fRecordMinY;
			ld->SetTVVert(t,vIndex,p);

		}
	}

	mbValidBBoxOfRecord = false;
}
void UnwrapMod::CalculateClusterBounds(int i)
{
	static const float PosValueLowLimit = 1e-4f;

	clusterList[i]->h = 0.0f;
	clusterList[i]->w = 0.0f;

	clusterList[i]->bounds.Init();
	MeshTopoData *ld = clusterList[i]->ld;
	for (int j = 0; j < clusterList[i]->verts.Count(); j++)
	{
		int vIndex = clusterList[i]->verts[j];
		Point3 p = ld->GetTVVert(vIndex);
		clusterList[i]->bounds += p;
	}

	Point3 width = clusterList[i]->bounds.Width();
	Point3 pmin, pmax;
	pmin = clusterList[i]->bounds.pmin;
	pmax = clusterList[i]->bounds.pmax;
	if (width.x < PosValueLowLimit)
	{
		Point3 p = pmin;
		p.x -= 0.0001f;
		clusterList[i]->bounds += p;
		p = pmax;
		p.x += 0.0001f;
		clusterList[i]->bounds += p;
	}
	if (width.y < PosValueLowLimit)
	{
		Point3 p = pmin;
		p.y -= 0.0001f;
		clusterList[i]->bounds += p;
		p = pmax;
		p.y += 0.0001f;
		clusterList[i]->bounds += p;
	}

	//build offset
	float x, y;
	x = clusterList[i]->bounds.pmin.x;
	y = clusterList[i]->bounds.pmin.y;
	clusterList[i]->offset.x = x;
	clusterList[i]->offset.y = y;
	clusterList[i]->offset.z = 0.0f;
	if (clusterList[i]->faces.Count() == 0)
	{
		clusterList[i]->h = 0.0f;
		clusterList[i]->w = 0.0f;

	}
	else
	{
		clusterList[i]->h = clusterList[i]->bounds.pmax.y - clusterList[i]->bounds.pmin.y;
		clusterList[i]->w = clusterList[i]->bounds.pmax.x - clusterList[i]->bounds.pmin.x;
		if (clusterList[i]->h < PosValueLowLimit) clusterList[i]->h = 0.5f;
		if (clusterList[i]->w < PosValueLowLimit) clusterList[i]->w = 0.5f;
	}
}

int UnwrapMod::GetClusterCount()
{
	return clusterList.Count();
}

IClusterInternal* UnwrapMod::GetCluster(int idx)
{
	if (idx >= 0 && idx < clusterList.Count())
	{
		return clusterList[idx];
	}
	return nullptr;
}

void UnwrapMod::RemoveCluster(int idx)
{
	if (idx >= 0 && idx < clusterList.Count())
		clusterList.Delete(idx, 1);
}

void UnwrapMod::SortClusterList(CompareFnc fnc)
{
	clusterList.Sort(fnc);
}
