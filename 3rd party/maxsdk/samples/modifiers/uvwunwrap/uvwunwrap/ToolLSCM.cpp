
/*

Copyright [2010] Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement 
provided at the time of installation or download, or which otherwise accompanies 
this software in either electronic or hard copy form.   

	

*/


#include "ToolLSCM.h"
#include "unwrap.h"
#include "PerformanceTools.h"
#include "modsres.h"

#include <math.h>
#include <fstream>
#include "../../../Include/uvwunwrap/uvwunwrapNewAPIs.h"


extern float AreaOfTriangle(Point3 a, Point3 b, Point3 c);


InteractiveStartRestoreObj::InteractiveStartRestoreObj(UnwrapMod *m)
{
	mMod = m;
}
void InteractiveStartRestoreObj::Restore(int isUndo)
{
	if (mMod->fnGetMapMode() == LSCMMAP)								
	{
		mMod->fnSetMapMode(NOMAP);
	}
}
void InteractiveStartRestoreObj::Redo()
{
	if (mMod->fnGetMapMode() != LSCMMAP)								
	{
		mMod->fnSetMapMode(LSCMMAP);
	}

}
void InteractiveStartRestoreObj::EndHold()
{

}
TSTR InteractiveStartRestoreObj::Description()
{
	return TSTR(GetString(IDS_PW_LSCM));
}

void UnwrapMod::LSCMForceResolve()
{
	if (fnGetMapMode() == LSCMMAP)
		mLSCMTool.Solve(true);
}


void UnwrapMod::fnLSCMInteractive(BOOL start)
{
	Tab<MeshTopoData*> mdList;
	mdList.SetCount(mMeshTopoData.Count());

	if (start)
	{
		for (int i = 0; i < mMeshTopoData.Count(); i++)
		{
			mdList[i] = mMeshTopoData[i];
		}

		if (theHold.IsSuspended() == FALSE)
		{
			theHold.Begin();
			HoldPointsAndFaces();
			theHold.Put(new InteractiveStartRestoreObj(this));
			theHold.Accept(GetString(IDS_PW_LSCM));
		}
		BOOL autoEdit = FALSE;
		pblock->GetValue(unwrap_peel_autoedit,0,autoEdit,FOREVER);
		if (hDialogWnd == NULL && autoEdit)
			fnEdit();
		mLSCMTool.Start(true,this,mdList);
	}
	else
	{
		// ideally we should restore the edge selection when exiting live peel. But PO said
		// it is acceptable to simply clear all edge selection since they have been all converted into open edge in live peel.
		for (int i = 0; i < mMeshTopoData.Count(); i++)
		{
			mMeshTopoData[i]->ClearTVEdgeSelection();
			mMeshTopoData[i]->ClearGeomEdgeSelection();
		}
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		InvalidateView();
		if (ip) ip->RedrawViews(ip->GetTime());
		mLSCMTool.End();
	}
}
void UnwrapMod::fnLSCMSolve()
{
	BOOL autoEdit = FALSE;
	pblock->GetValue(unwrap_peel_autoedit,0,autoEdit,FOREVER);
	if (hDialogWnd == NULL && autoEdit)
		fnEdit();

	theHold.Begin();
	HoldPointsAndFaces();
	theHold.Accept(GetString(IDS_PW_LSCM));

	Tab<MeshTopoData*> mdList;
	mdList.SetCount(mMeshTopoData.Count());
	for (int i = 0; i < mMeshTopoData.Count(); i++)
	{
		mdList[i] = mMeshTopoData[i];
	}
	mLSCMTool.Start(true,this,mdList);
	mLSCMTool.End();
}
void UnwrapMod::fnLSCMReset()
{
	BOOL autoEdit = FALSE;
	pblock->GetValue(unwrap_peel_autoedit,0,autoEdit,FOREVER);
	if (hDialogWnd == NULL && autoEdit)
		fnEdit();

	theHold.Begin();
	HoldPointsAndFaces();
	theHold.Accept(GetString(IDS_PW_LSCM));

	
	Tab<MeshTopoData*> mdList;
	mdList.SetCount(mMeshTopoData.Count());
	for (int i = 0; i < mMeshTopoData.Count(); i++)
	{
		mdList[i] = mMeshTopoData[i];
	}

	if(BuildBBoxOfRecord())
	{
		mbValidBBoxOfRecord = true;
	}
	BOOL normalize = packNormalize;
	packNormalize = FALSE;
	mLSCMTool.Start(false,this,mdList);
	mLSCMTool.End();
	packNormalize = normalize;
	
}

void UnwrapMod::fnLSCMInvalidateTopo(MeshTopoData *md)
{
	mLSCMTool.InvalidateTopo(md);
}

LSCMClusterData::LSCMClusterData() : mValidSolve(false), mUserDefinedPinVertsCount(0)
{
	pABFData = CreateABFPeelData();

	SecureZeroMemory(&mSuperLuStats, sizeof(SuperLUStat_t));
	SecureZeroMemory(&mU, sizeof(SuperMatrix));

	mNumberCoefficients = 0;
	mValidGeom = true;
	mSuperLuLandUAllocated = false;
	
	mNeedToResolve = true;
	//mUseHierarchy = false;

	mL.Store = nullptr;
	mU.Store = nullptr;
}

LSCMClusterData::~LSCMClusterData()
{
	if (mSuperLuLandUAllocated)
	{
		if (mL.Store)
		{
			Destroy_SuperNode_Matrix(&(mL));
			mL.Store = nullptr;
		}

		if (mU.Store)
		{
			Destroy_CompCol_Matrix(&(mU));
			mU.Store = nullptr;
		}

		StatFree(&(mSuperLuStats));
	}
	mSuperLuLandUAllocated = false;

	DestroyABFPeelData(pABFData);
}
	
	

void LSCMClusterData::Reset()
{
	mFaceData.SetCount(0);
}

int		LSCMClusterData::NumberFaces()
{
	return mFaceData.Count();
}


LSCMFace *LSCMClusterData::GetFace(int index)
{
	if ( (index < 0) || (index >= mFaceData.Count()))
		return NULL;
	return mFaceData.Addr(index);
}

void LSCMClusterData::Invalidate(BitArray changedTVVerts, MeshTopoData *md)
{
	BitArray usedPoly;
	usedPoly.SetSize(md->GetNumberFaces());
	usedPoly.ClearAll();

	//Loop through this cluster face list and see if shares any of the changed
	//tvverts.  If so the cluster needs to be invalidated
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		if (usedPoly[polyIndex] == false)
		{
			usedPoly.Set(polyIndex);
			int degree = md->GetFaceDegree(polyIndex);
			for (int j = 0; j < degree; j++)
			{
				int tvIndex = md->GetFaceTVVert(polyIndex,j);
				if (tvIndex < changedTVVerts.GetSize())
				{
					if (changedTVVerts[tvIndex])
					{
						mNeedToResolve = true;
						return;
					}
				}
			}
		}
	}
	mNeedToResolve = false;
}

//forces the system to invalidate
void LSCMClusterData::Invalidate()
{
	mNeedToResolve = true;
}

int LSCMClusterData::TempPinCount()
{
	return mTempPins.Count();
}

void LSCMClusterData::Add(int polyIndex, MeshTopoData *md)
{
	if (md)
	{
		//degenerate and dead faces should be skipped
		if(md->CheckPoly(polyIndex) == false)
		{
			return;
		}		

		md->AddLSCMFaceData(polyIndex,mFaceData);
	}
}

void LSCMClusterData::ApplyMapping(MeshTopoData *md, UnwrapMod *mod)
{

	//mark our faces for this cluster
	BitArray mapFaces;
	mapFaces.SetSize(md->GetNumberFaces());
	mapFaces.ClearAll();
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		mapFaces.Set(polyIndex,TRUE);
	}

	BitArray borderVerts;
	borderVerts.SetSize(md->GetNumberGeomVerts());
	borderVerts.ClearAll();

	int numGeoEdges = md->GetNumberGeomEdges();

	//mark vertices that are on the border of the cluster
	for (int i = 0; i < numGeoEdges; i++)
	{
		int numberConnectedFaces = md->GetGeomEdgeNumberOfConnectedFaces(i);
		if (numberConnectedFaces >1)
		{
			int numSelected = 0;
			
			for (int j = 0; j < numberConnectedFaces; j++)
			{
				int fa = md->GetGeomEdgeConnectedFace(i,j);
				if (mapFaces[fa])
					numSelected++;
			}
			if ((numSelected > 0) && (numSelected != numberConnectedFaces))
			{
				int va = md->GetGeomEdgeVert(i,0);
				borderVerts.Set(va,TRUE);
				va = md->GetGeomEdgeVert(i,1);
				borderVerts.Set(va,TRUE);

			}
		}
	}

	TimeValue t = GetCOREInterface()->GetTime();

	//find our dead verts ( just an optimization when adding tvs) 
	Tab<int> deadVerts;
	for (int m = 0; m < md->GetNumberTVVerts();m++)
	{
		if (md->GetTVVertDead(m))
		{
			deadVerts.Append(1,&m,1000);						
		}
	}



	Tab<int> geomToTV; //maps a geom vert to a tv 
	geomToTV.SetCount(md->GetNumberTVVerts());
	for (int i = 0; i < geomToTV.Count(); i++)
		geomToTV[i] = -1;

	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		if (mapFaces[polyIndex])
		{
			mapFaces.Clear(polyIndex);
			int degree = md->GetFaceDegree(polyIndex);

			for (int j = 0; j < degree; j++)
			{
				int geomIndex = md->GetFaceGeomVert(polyIndex,j);
				int tvIndex = md->GetFaceTVVert(polyIndex,j);

				Point3 xyz = md->GetGeomVert(geomIndex);

				//border verts need new verts added and then assigned
				if (geomToTV[geomIndex] != -1)
				{
					md->SetFaceTVVert(polyIndex,j,geomToTV[geomIndex]);
				}
				else if (borderVerts[geomIndex])
				{
					int newTVIndex = md->AddTVVert(t,xyz,polyIndex,j,0,&deadVerts);
					geomToTV[geomIndex] = newTVIndex;
					
				}
				//interior verts can just be assigned a pos
				else
				{
					md->SetTVVert(t,tvIndex,xyz);
					geomToTV[geomIndex] = tvIndex;
				}	

			}
		}
	}
}

void LSCMClusterData::UnmarkTempPins(MeshTopoData *md)
{
	for (int i = 0; i < mTempPins.Count(); i++)
	{
		md->TVVertUnpin(mTempPins[i]);
	}
	mTempPins.SetCount(0);
}

class TempPinData
{
public:

	TempPinData();
	void AddTV(int index);
	int mGeoIndex;
	int mTVIndex[2];
	int mTVCount;
};

TempPinData::TempPinData()
{
	mGeoIndex = -1;
	mTVCount = 0;
	mTVIndex[0] = -1;
	mTVIndex[1] = -1;
}
void TempPinData::AddTV(int index)
{


	if (mTVIndex[0] == -1) 
	{
		mTVIndex[0] = index;
		mTVCount++;
		return;
	}
	if ( (mTVIndex[1] == -1) && (mTVIndex[0] != index) )
	{
		mTVIndex[1] = index;
		mTVCount++;
		return;
	}

	if (mTVIndex[0] == index)
		return;
	if (mTVIndex[1] == index)
		return;

	//if we have more than 2 we are not interested in this pin
	if ( (mTVIndex[0] != index) && (mTVIndex[1] != index) )
		mTVCount = 3;
}

class TempBoundVert
{
public:
	TempBoundVert(){}
	TempBoundVert(int tv, int geo, Point3 p):tvIndex(tv), geoIndex(geo), geoPos(p){}

	int tvIndex;
	int geoIndex;

	Point3 geoPos;
};

class TempBoundEdge
{
public:
	TempBoundEdge()
	{
		start = false;
		used = false;
	}

	float getLength();

	TempBoundVert v1;
	TempBoundVert v2;

	bool start;
	bool used;
};

class TempPinFinder 
{
public:
	TempPinFinder(UnwrapMod *mod, MeshTopoData *md);

	void AddVert(int geoIndex, int tvIndex);

	void AddEdge(int geoIndex1, int tvIndex1, int geoIndex2, int tvIndex2);

	void BuildBoundary();

	void ComputePins(int &pin1, int &pin2);

	bool FindPinsOnVertSplitBoundary( int &pin1, int &pin2 );

	bool FindFarthestVertSplitPins( float &farthestD, int &farthestID, int &pin1, int &pin2 );

	int FindPinVert(int tvIndex);

	MeshTopoData* mMd;
	UnwrapMod *mMod;
	Tab<int> mGeoToPinIndex;
	Tab<TempPinData> mPinData;
	Tab<TempBoundEdge> mTempBoundEdge;
	Tab<TempBoundEdge> mFinalBoundEdge;
	int mMaxStart, mMaxEnd;
};

enum AxisType {x_axis=0, y_axis, z_axis};
class ExtremaPinFinder  // TODO: we might want this be private
{
public:
	ExtremaPinFinder(Tab<TempPinData> &pinData, UnwrapMod *mod, MeshTopoData *md);

	void findBestPins(int &aID, int &bID);

	MeshTopoData *mMd;
	UnwrapMod *mMod;
	AxisType axis;	// 0 for x, 1 for y, 2 for z
	int minIndex[3];
	float minValue[3];
	int maxIndex[3];
	float maxValue[3];
};

ExtremaPinFinder::ExtremaPinFinder(Tab<TempPinData> &pinData, UnwrapMod *mod, MeshTopoData *md)
	:mMod(mod), mMd(md)
{
	axis = x_axis;
	for (int i=0; i<3; i++)
	{
		minIndex[i] = maxIndex[i] = -1;
		maxValue[i] = -FLT_MAX * 0.5f;
		minValue[i] = FLT_MAX * 0.5f;
	}

	for (int i=0; i<pinData.Count(); i++)
	{
		if (pinData[i].mTVCount == 1)
		{
			Point3 tva = md->GetGeomVert(pinData[i].mGeoIndex);

			if (tva.x < minValue[x_axis]) { minValue[x_axis] = tva.x; minIndex[x_axis] = i; }
			if (tva.x > maxValue[x_axis]) { maxValue[x_axis] = tva.x; maxIndex[x_axis] = i; }
			if (tva.y < minValue[y_axis]) { minValue[y_axis] = tva.y; minIndex[y_axis] = i; }
			if (tva.y > maxValue[y_axis]) { maxValue[y_axis] = tva.y; maxIndex[y_axis] = i; }
			if (tva.z < minValue[z_axis]) { minValue[z_axis] = tva.z; minIndex[z_axis] = i; }
			if (tva.z > maxValue[z_axis]) { maxValue[z_axis] = tva.z; maxIndex[z_axis] = i; }
		}
	}
}

void ExtremaPinFinder::findBestPins(int &aID, int &bID)
{
	// use mirror information to check symmetry
	// Build Mirror Data
	// The result is acceptable even without mirror info, so following code commented
	// 	const float MIRROR_RATE = 0.8;
	// 	bool findMirror[3];
	// 	mMod->fnSetMirrorSelectionStatus(TRUE);
	// 	for (int i=0; i<3; i++)
	// 	{
	// 	 	mMod->fnSetMirrorAxis(i);
	// 	 	int mirCount =0;
	// 	 	for (int j=0; j<mMd->GetNumberGeomVerts(); j++)
	// 	 	{
	// 	 		if(mMd->mGeomVertMirrorDataList[j].index >= 0) mirCount++;
	// 	 	}
	// 	 	findMirror[i] = false;
	// 	 	if ((float)mirCount / (float)mMd->GetNumberGeomVerts() > MIRROR_RATE)
	// 	 	{
	// 	 		findMirror[i] = true;
	// 	 	}
	// 	}

	// find the axis with the largest distance
	float longestDist = 0.0;
	float tempDist = 0.0;
	for (int i=0; i<3; i++)
	{
		tempDist = maxValue[i] - minValue[i];
		//if (findMirror[i]) tempDist *= 4;
		if (tempDist > longestDist)
		{
			longestDist = tempDist;
			axis = AxisType(i);
		}
	}

	aID = minIndex[axis];
	bID = maxIndex[axis];
}

float TempBoundEdge::getLength()
{
	return (v1.geoPos - v2.geoPos).Length();
}

TempPinFinder::TempPinFinder(UnwrapMod *mod, MeshTopoData *md)
{
	mMod = mod;
	mMd = md;
	mGeoToPinIndex.SetCount(mMd->GetNumberGeomVerts());
	for (int i = 0; i < mGeoToPinIndex.Count(); i++)
		mGeoToPinIndex[i] = -1;
}

void TempPinFinder::AddVert(int geoIndex, int tvIndex)
{
	int pinIndex = mGeoToPinIndex[geoIndex];
	if (pinIndex == -1)
	{
		TempPinData pinData;
		pinData.mGeoIndex = geoIndex;
		pinData.AddTV(tvIndex);
		mPinData.Append(1,&pinData,10000);
		mGeoToPinIndex[geoIndex] = mPinData.Count()-1;
	}
	else
	{		
		mPinData[pinIndex].AddTV(tvIndex);
	}	
}

void TempPinFinder::AddEdge(int geoIndex1, int tvIndex1, int geoIndex2, int tvIndex2)
{
	TempBoundVert v1(tvIndex1, geoIndex1, mMd->GetGeomVert(geoIndex1));
	TempBoundVert v2(tvIndex2, geoIndex2, mMd->GetGeomVert(geoIndex2));
	TempBoundEdge edge;
	edge.v1 = v1;	edge.v2 = v2;

	mTempBoundEdge.Append(1, &edge, 100);
}

void TempPinFinder::BuildBoundary()
{
	if (mTempBoundEdge.Count() == 0) return;
	mTempBoundEdge[0].start = true;
	mFinalBoundEdge.Append(1, &mTempBoundEdge[0], 100);
	int startPt = 0;
	TempBoundVert *curVert = &mTempBoundEdge[0].v2;
	mTempBoundEdge[0].used = true;
	bool findEdge = false;
	while (startPt < mTempBoundEdge.Count())
	{
		for (int i=0; i<mTempBoundEdge.Count(); i++)
		{
			if (mTempBoundEdge[i].used) continue;
			// try to find an edge with the current vert
			if (mTempBoundEdge[i].v1.tvIndex == curVert->tvIndex)
			{
				curVert = &mTempBoundEdge[i].v2;
				mTempBoundEdge[i].used = true;
				mFinalBoundEdge.Append(1, &mTempBoundEdge[i], 100);
				findEdge = true;
				break;	// break for
			}
			else if(mTempBoundEdge[i].v2.tvIndex == curVert->tvIndex)
			{
				// swap of v1, v2
				TempBoundVert tempVert = mTempBoundEdge[i].v1;
				mTempBoundEdge[i].v1 = mTempBoundEdge[i].v2;
				mTempBoundEdge[i].v2 = tempVert;

				curVert = &mTempBoundEdge[i].v2;
				mTempBoundEdge[i].used = true;
				mFinalBoundEdge.Append(1, &mTempBoundEdge[i], 100);
				findEdge = true;
				break;	// break for
			}
		}

		if (!findEdge)
		{
			// the boundary from the start has been found, update start point now
			bool findNextStart = false;
			for (int i=0; i<mTempBoundEdge.Count(); i++)
			{
				if (!mTempBoundEdge[i].used)
				{
					startPt = i;
					findNextStart = true;
					mTempBoundEdge[startPt].start = true;
					mFinalBoundEdge.Append(1, &mTempBoundEdge[startPt], 100);
					mTempBoundEdge[startPt].used = true;
					curVert = &mTempBoundEdge[startPt].v2;	// use v1 of the start point as the start

					break;	// break for
				}
			}
			if (!findNextStart) break;	// break while
		}
		findEdge = false;
	}

	// find the max loop boundary
	float len, maxLen;
	len = maxLen = mFinalBoundEdge[0].getLength();
	mMaxStart = 0, mMaxEnd = 0;
	int curStart = 0, curEnd = 0;
	for (int i=1; i<mFinalBoundEdge.Count(); i++)
	{
		if (mFinalBoundEdge[i].start)
		{
			// find start means have found an end
			if (len > maxLen)
			{
				// should check if it is a loop
				if (mFinalBoundEdge[i-1].v2.tvIndex == mFinalBoundEdge[curStart].v1.tvIndex)
				{
					mMaxStart = curStart; mMaxEnd = curEnd; maxLen = len;
				}
			}

			len = mFinalBoundEdge[i].getLength();
			curStart = curEnd = i;
		}

		len += mFinalBoundEdge[i].getLength();
		curEnd = i;
	}
	// check the last boundary
	if (len > maxLen)
	{
		// should check if it is a loop
		if (mFinalBoundEdge[mFinalBoundEdge.Count()-1].v2.tvIndex == mFinalBoundEdge[curStart].v1.tvIndex)
		{
			mMaxStart = curStart; mMaxEnd = curEnd; maxLen = len;
		}
	}


}

int TempPinFinder::FindPinVert(int tvIndex)
{
	for (int i = 0; i < mPinData.Count(); i++)
	{
		int tvCount = mPinData[i].mTVCount;		
		if (tvCount > 2)
			tvCount = 2;
		for (int j = 0; j < tvCount; j++)
		{
			if (mPinData[i].mTVIndex[j] == tvIndex)
			{
				//we don't want pin verts with more than 2 connections
				if (mPinData[i].mTVCount > 2)
					return -1;
				else
					return i;
			}
		}
	}
	return -1;
}

void TempPinFinder::ComputePins(int &pin1, int &pin2)
{
	//if we only need to compute 1
	if (pin1 != -1)
	{
		//find our geo vert attached to this pin
		int pinIndex = FindPinVert(pin1);
		float dist = 0.0f;
		//if we share a common geometry vert find the distance
		if ( (pinIndex != -1) && (mPinData[pinIndex].mTVCount == 2))
		{			
			if (mPinData[pinIndex].mTVIndex[0] == pin1)
				pin2 = mPinData[pinIndex].mTVIndex[1];
			else
				pin2 = mPinData[pinIndex].mTVIndex[0];
			dist = LengthSquared(mMd->GetTVVert(pin1)-mMd->GetTVVert(pin2));
		}

		//now find all the other distances
		Point3 p = mMd->GetTVVert(pin1);
		BitArray usedVerts;
		usedVerts.SetSize(mMd->GetNumberTVVerts());
		usedVerts.ClearAll();
		usedVerts.Set(pin1,TRUE);
		int farthestID = -1;
		float farthestD = 0.0f;
		for (int i = 0; i < mPinData.Count(); i++)
		{
			int tvCount = mPinData[i].mTVCount;						
			if (tvCount <= 2)
			{
				for (int j = 0; j < tvCount; j++)
				{
					int tvIndex = mPinData[i].mTVIndex[j];
					if (usedVerts[tvIndex] == false)
					{
						Point3 p1 = mMd->GetTVVert(tvIndex);
						float d = LengthSquared(p-p1);
						if ((farthestID == -1) || (d > farthestD))
						{
							farthestID = tvIndex;
							farthestD = d;
						}
						usedVerts.Set(tvIndex,TRUE);
					}
				}

			}
		}

		// if the non shared distance is greater by 2x use it instead (if the object is a long rect we want to use that )
		if (farthestD > (dist*2))
			pin2 = farthestID;
		return;

	}
	else
	{
		//loop through looking for pins that share a geom vert
		//look for the farthest distance, if there is one
		//use that one
		int farthestID = -1;
		float farthestD = 0.0f;		
		FindFarthestVertSplitPins(farthestD, farthestID, pin1, pin2);

		// Strategy 2: using boundary on 3d model to find pins on vert split path
// 		if (FindPinsOnVertSplitBoundary(pin1, pin2))
// 			return;

		//now look pins that dont share geom and find the closest one to the extremes
		ExtremaPinFinder tPinData(mPinData, mMod, mMd);

		int aID = -1;
		int bID = -1;
		tPinData.findBestPins(aID, bID);

		int tempPin1 = -1;
		int tempPin2 = -1;
		if (aID != -1)
			tempPin1 = mPinData[aID].mTVIndex[0];
		if (bID != -1)
			tempPin2 = mPinData[bID].mTVIndex[0];

		if ( (tempPin1 != -1) && (tempPin2 != -1) ) 
		{
// 			pin1 = tempPin1;
// 			pin2 = tempPin2;
			Point3 tva = mMd->GetTVVert(tempPin1);
			Point3 tvb = mMd->GetTVVert(tempPin2);
			tva.z = 0.0f;
			tvb.z = 0.0f;
			float tempDist = LengthSquared( tva - tvb );
			// if the non shared distance is greater by 2x use it instead (if the object is a long rect we want to use that )
			if (tempDist > (farthestD*2))
			{
				pin1 = tempPin1;
				pin2 = tempPin2;
			}
		}

	}
}

bool TempPinFinder::FindFarthestVertSplitPins( float &farthestD, int &farthestID, int &pin1, int &pin2 )
{
	int ct = 0;

	Box3 pinBounds;
	pinBounds.Init();

	int sealedTV = 0;
	Tab<int> sealedPins;
	for (int i = 0; i < mPinData.Count(); i++)
	{
		if (mPinData[i].mTVCount == 2)
		{
			Point3 tva = mMd->GetTVVert(mPinData[i].mTVIndex[0]);
			Point3 tvb = mMd->GetTVVert(mPinData[i].mTVIndex[1]);
			tva.z = 0.0f;
			tvb.z = 0.0f;
			pinBounds += tva;
			pinBounds += tvb;

			float d = LengthSquared(tva-tvb);
			ct++;
			if (d < 0.001f)
			{
				sealedTV++;
				sealedPins.Append(1,&i,1000);
			}
			if (d > farthestD)
			{
				farthestID = i;
				farthestD = d;
			}
		}
	}

	//this is our first potential pin
	if ((farthestID != -1) && (farthestD > 0.001f))
	{
		pin1 = mPinData[farthestID].mTVIndex[0];
		pin2 = mPinData[farthestID].mTVIndex[1];

		return true;
	}

	//we did not find a pair with distance between them, that means the seam is not yet split
	//so find one near the center and the system will push them apart
	else if (sealedPins.Count())
	{
		Point3 center = pinBounds.Center();
		center.z = 0.0f;
		int closestID = -1;
		float closestD = 0.0f;
		for (int k = 0; k < sealedPins.Count(); k++)
		{
			int i = sealedPins[k];
			if (mPinData[i].mTVCount == 2)
			{
				Point3 tva = mMd->GetTVVert(mPinData[i].mTVIndex[0]);	
				tva.z = 0.0f;
				float d = LengthSquared(center-tva);				
				if ((closestID == -1) || (d < closestD))
				{
					closestID = i;
					closestD = d;
				}
			}
		}
		if (closestID != -1) 
		{
			pin1 = mPinData[closestID].mTVIndex[0];
			pin2 = mPinData[closestID].mTVIndex[1];

			return true;
		}
	}

	return false;
}

bool TempPinFinder::FindPinsOnVertSplitBoundary( int &pin1, int &pin2 )
{
	BuildBoundary();

	// use boundary data to find symmetry pins
	// find the longest series of verts split on the boundary
	if (mFinalBoundEdge.Count() > 0)
	{
		float totLen = 0.0;
		float curSplitLen = 0.0, maxSplitLen = 0.0;
		int curSplitIndex = -1;
		int maxSplitStart = -1, maxSplitEnd = -1;
		int firstE1 = -1, firstE2 = -1;
		float firstLen = 0.0;
		int curEdgeIndex = mMaxStart, lastEdgeIndex = mMaxEnd;
		do 
		{
			float len = mFinalBoundEdge[curEdgeIndex].getLength();
			totLen += len;

			if ((mPinData[mGeoToPinIndex[mFinalBoundEdge[curEdgeIndex].v1.geoIndex]].mTVCount == 2) /* vert split */
				||
				((mPinData[mGeoToPinIndex[mFinalBoundEdge[curEdgeIndex].v2.geoIndex]].mTVCount == 2) /* vert split */
				&&
				(mPinData[mGeoToPinIndex[mFinalBoundEdge[lastEdgeIndex].v1.geoIndex]].mTVCount == 2)) /* vert split */
				)
			{
				if (curSplitIndex == -1)
				{
					if (curEdgeIndex == mMaxStart) firstE1 = curEdgeIndex;
					curSplitIndex = curEdgeIndex;
				}
				else
				{
					curSplitLen += mFinalBoundEdge[lastEdgeIndex].getLength();
				}
			}
			else if (curSplitIndex != -1)
			{
				// find end of the split boundary
				if (curSplitLen > maxSplitLen)
				{
					maxSplitLen = curSplitLen;
					maxSplitStart = curSplitIndex;
					maxSplitEnd = lastEdgeIndex;
				}

				if (firstE1 == curSplitIndex)
				{
					firstLen = curSplitLen;
					firstE2 = lastEdgeIndex;
				}

				curSplitLen = 0.0f;
				curSplitIndex = -1;
			}

			lastEdgeIndex = curEdgeIndex;
			curEdgeIndex++;
		} while (curEdgeIndex <= mMaxEnd);

		// calculate the first edge
		curEdgeIndex--;
		if ((curSplitIndex != -1) && (curSplitIndex != mMaxStart) && (firstE1 != -1))
		{
			firstLen += curSplitLen + mFinalBoundEdge[curEdgeIndex].getLength();

			if (firstLen > maxSplitLen)
			{
				maxSplitLen = firstLen;
				maxSplitStart = curSplitIndex;
				maxSplitEnd = firstE2;
			}
		}

		if ((maxSplitStart != -1) && (maxSplitEnd != -1) && (maxSplitLen > 0.5 * totLen))
		{
			// Strategy 1: just use start and end as pin points
			 TempBoundVert tempPin1 = mFinalBoundEdge[maxSplitStart].v1;
			 TempBoundVert tempPin2 = mFinalBoundEdge[maxSplitEnd].v1;

			// Strategy 2: try to find symmetric points
			// find pin1 in the split vertices
// 			int curStart = maxSplitStart;
// 			int curEnd = maxSplitEnd;
// 			float lenStart = 0.0f;
// 			float lenEnd = 0.0f;
// 			do 
// 			{
// 			 	if (lenStart < lenEnd)
// 			 	{
// 			 		lenStart += mFinalBoundEdge[curStart].getLength();
// 			 		curStart++;
// 			 		if (curStart > mMaxEnd) curStart = mMaxStart + curStart - mMaxEnd;
// 			 	}
// 			 	else
// 			 	{
// 			 		curEnd--;
// 			 		if (curEnd < mMaxStart) curEnd = mMaxEnd - mMaxStart + curEnd;
// 			 		lenEnd += mFinalBoundEdge[curEnd].getLength();
// 			 	}
// 			} while (curStart != curEnd);
// 			 
// 			TempBoundVert tempPin1 = mFinalBoundEdge[curStart].v1;
// 			 
// 			// find pin2 outside the split vertices
// 			curStart = maxSplitStart;
// 			curEnd = maxSplitEnd;
// 			lenStart = 0.0f;
// 			lenEnd = 0.0f;
// 			do 
// 			{
// 			 	if (lenStart < lenEnd)
// 			 	{
// 			 		curStart--;
// 			 		if (curStart < mMaxStart) curStart = mMaxEnd - mMaxStart + curStart;
// 			 		lenStart += mFinalBoundEdge[curStart].getLength();
// 			 	}
// 			 	else
// 			 	{
// 			 		lenEnd += mFinalBoundEdge[curEnd].getLength();
// 			 		curEnd++;
// 			 		if (curEnd > mMaxEnd) curEnd = mMaxStart + curEnd - mMaxEnd;
// 			 	}
// 			} while (curStart != curEnd);
// 			 
// 			TempBoundVert tempPin2 = mFinalBoundEdge[curStart].v1;

			pin1 = tempPin1.tvIndex;
			pin2 = tempPin2.tvIndex;

			return true;
		}
	}

	return false;
}

bool LSCMClusterData::IsFlat() const
{
	return mIsFlat;
}

void LSCMClusterData::ComputeSimplePin(const TimeValue t,  MeshTopoData *md,  const bool useExistingMapping)
{
	//this is used for flat geometry just pick the first edge and use the end points as our bind points
	if (mFaceData.Count())
	{
		mPinVerts.SetCount(2);
		mPinVerts[0] = mFaceData[0].mEdge[0].mTVVert;
		mPinVerts[1] = mFaceData[0].mEdge[1].mTVVert;
		md->TVVertPin(mPinVerts[0]);
		md->TVVertPin(mPinVerts[1]);
		mTempPins = mPinVerts;
		if (!useExistingMapping)
			CalculatePinPos2D(md, mPinVerts[0], mPinVerts[1], mFaceData[0].mEdge[0].mGeoVert, mFaceData[0].mEdge[1].mGeoVert, t);
	}
}

void LSCMClusterData::ComputePins(MeshTopoData *md, UnwrapMod *mod, bool useExistingMapping)
{
	if (mNeedToResolve == false)
		return;
	
	mTempPins.SetCount(0);
	mPinVerts.SetCount(0);

	//just create a bitarray to prevent the same pin getting added twice
	BitArray mapFaces;
	mapFaces.SetSize(md->GetNumberFaces());
	mapFaces.ClearAll();
	
	mClusterVerts.SetSize(md->GetNumberTVVerts());
	mClusterVerts.ClearAll();
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		for (int j = 0; j < 3; j++)	
		{			
			int tvIndex = md->GetFaceTVVert(polyIndex,mFaceData[i].mEdge[j].mIthEdge);
			mClusterVerts.Set(tvIndex,TRUE);
		}
	
	}

	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		mapFaces.Set(polyIndex,TRUE);
	}

	//WXY 7/8/2015 : PO says we want to forbid users to select pin points later
	//So we should calculate 2 pin verts automatically every time
	//WXY 9/15/2015 : It is a bad idea to choose 2 pin verts automatically every time,
	//because in peel mode, users don't want to change the position of existing pin points.
	//Find existing pin verts, and set them into mPinVerts
	FindExistPinVerts(mapFaces, md);


	//we need at least 2 pin verts if not we need to create 2

	if ((mPinVerts.Count() < 2) && IsFlat())
	{
		ComputeSimplePin(GetCOREInterface()->GetTime(),md,useExistingMapping);
	}
	else 
		if (mPinVerts.Count() < 2)
	{	
		bool onePinVert = false;
		int pVert1 = -1;
		int pVert2 = -1;

		if (mPinVerts.Count() == 1)
		{
			onePinVert = true;
			pVert1 = mPinVerts[0];
		}

		mPinVerts.SetCount(2);

		TempPinFinder pinFinder(mod, md);
		
		//pin verts are best if they are on open edges we actually need open edges to do a solve
		int  potentialPinVertsCount = 0;

		Box3 potentialBounds;
		potentialBounds.Init();

		Box3 tvBounds;
		tvBounds.Init();

		//find our potential verts
		for (int i = 0; i < md->GetNumberTVEdges(); i++)
		{
			int numberConnectedFaces = md->GetTVEdgeNumberTVFaces(i);

			//it is an open edge so could be a pin vert		
			if (numberConnectedFaces == 1)  
			{
				int fa = md->GetTVEdgeConnectedTVFace(i,0);
				if (mapFaces[fa])
				{
					int va1 = md->GetTVEdgeVert(i,0);
					potentialBounds += md->GetTVVert(va1);
					tvBounds += md->GetTVVert(va1);	
					int geoIndex1 = -1;
					int degree = md->GetFaceDegree(fa);
					for (int k = 0; k < degree; k++)
					{
						if (va1 == md->GetFaceTVVert(fa,k))
						{
							geoIndex1 = md->GetFaceGeomVert(fa,k);
							 k = degree;
						}
					}
					pinFinder.AddVert(geoIndex1,va1);
					potentialPinVertsCount++;

					int geoIndex2 = -1;
					int va2 = md->GetTVEdgeVert(i,1);
					potentialBounds += md->GetTVVert(va2);
					tvBounds += md->GetTVVert(va2);
					for (int k = 0; k < degree; k++)
					{
						if (va2 == md->GetFaceTVVert(fa,k))
						{
							geoIndex2 = md->GetFaceGeomVert(fa,k);
							k = degree;
						}
					}
					pinFinder.AddVert(geoIndex2,va2);
					potentialPinVertsCount++;

					pinFinder.AddEdge(geoIndex1, va1, geoIndex2, va2);
				}
			}
			//just add to our bounding box so we can find the center of the tvs later
			else
			{
				int numberFaces = md->GetTVEdgeNumberTVFaces(i);
				for (int j = 0; j < numberFaces; j++)
				{
					int fa = md->GetTVEdgeConnectedTVFace(i,0);
					if (mapFaces[fa])
					{
						int va = md->GetTVEdgeVert(i,0);
						tvBounds += md->GetTVVert(va);
						va = md->GetTVEdgeVert(i,1);
						tvBounds += md->GetTVVert(va);
						j = numberFaces;
					}
				}
			}
		}

		mValidGeom = true;
		if (potentialPinVertsCount == 0)
			mValidGeom = false;
		pinFinder.ComputePins(pVert1,pVert2);

		//find the geom verts attached to the tv pin verts
		int pVertGeo1 = -1;
		int pVertGeo2 = -1;
		FindGeoVertsByTV(md, mapFaces, pVert1, pVertGeo1, pVert2, pVertGeo2);


		//if we have on defined pin vert just use one of our computed pins
		if (onePinVert)
		{			
			if (mPinVerts[0] != pVert2)
				pVert1 = mPinVerts[0];
			else
			{
				pVert2 = pVert1;
				pVert1 = mPinVerts[0];
			}
		}

		//see if there is a matching TV vert sharing same geovert, is use that instead
		//this helps with symmetry
		//seems new algorithm will not produce result of this case, but not sure, so still keep code here
		if (onePinVert == false)
		{
			OptimizeTVertShareGeoVert(md, mapFaces, pVertGeo1, pVert1, pVert2, pVertGeo2, potentialBounds, tvBounds, mod);
		}

		//set position of pin verts
		TimeValue t = GetCOREInterface()->GetTime();
		
		mPinVerts[0] = pVert1;
		mPinVerts[1] = pVert2;

		//if we are using existing mapping don't move the pins
		if ((pVert1 != -1) && (pVert2 != -1))
		{
			//WXY 7/8/2015 : New pin position calculation algorithm
			if (!useExistingMapping)
				CalculatePinPos2D(md, pVert1, pVert2, pVertGeo1, pVertGeo2, t);
			// reset lscm move
			mLscmMove.x = mLscmMove.y = mLscmMove.z = 0;

			md->TVVertPin(pVert1);
			md->TVVertPin(pVert2);

			//In peel mode, there is no need to append newly created one or two pin vertices to mTempPins, so they could be displayed in UVW editor view.
			bool bPinsShowConditions;
			if (mod->showPins)
				bPinsShowConditions = true;
			else
				bPinsShowConditions = mod->WtIsChecked(ID_LSCM_INTERACTIVE) == TRUE;
			
			if(!bPinsShowConditions)
			{
				//if we have one defined pin vert we only want to store off our computed so we dont
				//erase it when remove the temp pins
				mTempPins.SetCount(0);
				if (onePinVert)
				{
					mTempPins.Append(1,&pVert2);
				}
				else
					mTempPins = mPinVerts;
			}
		}
		else
		{
			mValidGeom = false;
		}

	}
	
	if (mPinVerts.Count() >= 2 && mPinVerts[0] != -1 && mPinVerts[1] != -1)  //if we have pins we alway need to compute the center
	{
		int pVert1 = mPinVerts[0];
		int pVert2 = mPinVerts[1];

		// move center of v1->v2 to (0,0.5), after lscm is finished, move it back
		// just empirical data, need to find the reason of symmetry problem later, in ComputeMatrices()
		Point3 pos1 = md->GetTVVert(pVert1);
		Point3 pos2 = md->GetTVVert(pVert2);

		Point3 pCenter = pos1 + pos2;
		pCenter *= 0.5;
		Point3 guessCenter(0.0, 0.5, 0.0);
		pCenter = pCenter - guessCenter;

		//set position of pin verts
		TimeValue t = GetCOREInterface()->GetTime();

		int pinCount = mPinVerts.Count();
		for (int pI=0; pI<pinCount; pI++)
		{
			Point3 posI = md->GetTVVert(mPinVerts[pI]);
			posI = posI - pCenter;
			md->SetTVVert(t, mPinVerts[pI], posI);
		}

		mLscmMove = pCenter;
	}

	PreparePinDataForLSCM(mFaceData, md);
}

//this just fills out the angles which is needed for the solve
void LSCMClusterData::ComputeFaceAngles(MeshTopoData *md)
{
	if (mNeedToResolve == false)
		return;

	Point3 startFaceNormal(0,0,1);
	mIsFlat = true;
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		LSCMFace *face = &mFaceData[i];
		face->mEdge[0].mTVVert = md->GetFaceTVVert(face->mPolyIndex, face->mEdge[0].mIthEdge);
		face->mEdge[1].mTVVert = md->GetFaceTVVert(face->mPolyIndex, face->mEdge[1].mIthEdge);
		face->mEdge[2].mTVVert = md->GetFaceTVVert(face->mPolyIndex, face->mEdge[2].mIthEdge);

		int index[3];
		index[0] = md->GetFaceGeomVert(face->mPolyIndex,face->mEdge[0].mIthEdge);
		index[1] = md->GetFaceGeomVert(face->mPolyIndex,face->mEdge[1].mIthEdge);
		index[2] = md->GetFaceGeomVert(face->mPolyIndex,face->mEdge[2].mIthEdge);
		face->mEdge[0].mGeoVert = index[0];
		face->mEdge[1].mGeoVert = index[1];
		face->mEdge[2].mGeoVert = index[2];

		if (i == 0) //first face grab the initial normal
		{
			startFaceNormal = md->GetGeomFaceNormal(face->mPolyIndex);
		}
		else if (mIsFlat) //once we become unflat we can skip 
		{
			Point3 faceNormal = md->GetGeomFaceNormal(face->mPolyIndex);
			float dotProd = DotProd(faceNormal, startFaceNormal);
			if (dotProd < 0.99f) //if the angle between the face is greater the about 0 it is not flat
			{
				mIsFlat = false;
			}
		}

		Point3 p[3];
		p[0] = md->GetGeomVert(index[0]);
		p[1] = md->GetGeomVert(index[1]);
		p[2] = md->GetGeomVert(index[2]);

		Point3 vec[3];
		vec[0] = Normalize(p[1] - p[0]);
		vec[1] = Normalize(p[2] - p[1]);
		vec[2] = Normalize(p[0] - p[2]);

		face->mEdge[0].mGeoAngle = fabs(md->AngleFromVectors(vec[0],vec[2]*-1.0f));
		face->mEdge[1].mGeoAngle = fabs(md->AngleFromVectors(vec[1],vec[0]*-1.0f));
		face->mEdge[2].mGeoAngle = fabs(md->AngleFromVectors(vec[2],vec[1]*-1.0f));

		face->mEdge[0].mSin = sin(face->mEdge[0].mGeoAngle);
		face->mEdge[1].mSin = sin(face->mEdge[1].mGeoAngle);
		face->mEdge[2].mSin = sin(face->mEdge[2].mGeoAngle);

		//for best result we want the largest angle in the last slot
		for (int j = 0; j < 2; j++)
		{		
			//shift until we get the largest angle in the last slot
			if ( ( face->mEdge[2].mSin < face->mEdge[1].mSin)  || ( face->mEdge[2].mSin < face->mEdge[0].mSin) )
			{				
				LSCMFaceEdge temp = face->mEdge[0];
				face->mEdge[0] = face->mEdge[1];
				face->mEdge[1] = face->mEdge[2];
				face->mEdge[2] = temp;
			}
		}

	}

}


void LSCMClusterData::AddToTempTable(int index, float val, float uv)
{
	
	TempData data;
	
	if (index < 0)
	{
		data.mIndex = 0;
		data.mValue = val * uv;
//		DebugPrint("Pin Add to Matirx %d %f\n",index,val*uv);
		mPinnedVertsList.Append(1,&data,10000);
	}
	else
	{
		data.mIndex = index;
		data.mValue = val;
//		DebugPrint("Add to Matirx %d %f\n",index,val);
		mNonPinnedVertsList.Append(1,&data,10000);
	}
}

int LSCMClusterData::ComputeMatrixIndex(const Tab<LSCMFace> &faceData, int faceIndex, int ithEdge, int tvIndex)
{
	int matrixIndex = -1;
	if (faceData[faceIndex].mEdge[ithEdge].mPinned)
		matrixIndex = -1;
	else
		matrixIndex = (tvIndex < 0 || tvIndex >= mTVIndexToMatrixIndex.Count()) ? -1 : mTVIndexToMatrixIndex[tvIndex];	// CER 130191081/130046581, MAXX-33851
	return matrixIndex;
}

void LSCMClusterData::ComputeMatrices(MeshTopoData *md, bool useSimplifyModel)
{
	if (mValidGeom ==  false) return;

	if (mNeedToResolve == false)
		return;
	if (mNumberCoefficients == 0)
		return;

	//Tab<LSCMFace> &faceData = useSimplifyModel ? mSimFaceData : mFaceData;
	Tab<LSCMFace> &faceData = mFaceData;
	mNonPinnedVertsList.SetCount(0,FALSE);
	mPinnedVertsList.SetCount(0,FALSE);

	mAMatrix.SetSize(mNumberCoefficients);

	mBMatrix.SetCount(mNumberCoefficients);
	mXMatrix.SetCount(mNumberCoefficients);

	for (int i = 0; i < mBMatrix.Count(); i++)
	{
		mBMatrix[i] = 0.0f;
		mXMatrix[i] = 0.0f;
	}

	for (int i=0; i < faceData.Count(); i++)
	{

		float ratio = 1.0f;

		LSCMFace *curFace = &faceData[i];
		if (curFace->mEdge[2].mSin != 0.0f)
		{
			ratio =  curFace->mEdge[1].mSin/curFace->mEdge[2].mSin;
		}

		float c = cos(curFace->mEdge[0].mGeoAngle) *ratio;
		float s = curFace->mEdge[0].mSin*ratio;

		//		DebugPrint("c %f s %f ratio %f \n",c,s,ratio);

		int tvIndex[3];
		tvIndex[0] = curFace->mEdge[0].mTVVert;
		tvIndex[1] = curFace->mEdge[1].mTVVert;
		tvIndex[2] = curFace->mEdge[2].mTVVert;

		Point3 uv[3];
		uv[0] = md->GetTVVert(tvIndex[0]);
		uv[1] = md->GetTVVert(tvIndex[1]);
		uv[2] = md->GetTVVert(tvIndex[2]);

//		DebugPrint("Angles %f %f %f \n",mFaceData[i].mEdge[0].mGeoAngle,mFaceData[i].mEdge[1].mGeoAngle,mFaceData[i].mEdge[2].mGeoAngle);
//		DebugPrint("Sin %f %f %f \n",mFaceData[i].mEdge[0].mSin,mFaceData[i].mEdge[1].mSin,mFaceData[i].mEdge[2].mSin);


//		DebugPrint("Add Row\n");
		mNonPinnedVertsList.SetCount(0,FALSE);
		mPinnedVertsList.SetCount(0,FALSE);

		int matrixIndex1 = ComputeMatrixIndex(faceData, i,0,tvIndex[0]);
		AddToTempTable(matrixIndex1*2,	c - 1.0f,	uv[0].x); 
		AddToTempTable(matrixIndex1*2+1,	-s,		uv[0].y); 

		int matrixIndex2 = ComputeMatrixIndex(faceData, i,1,tvIndex[1]);
		AddToTempTable(matrixIndex2*2,		-c,		uv[1].x); 
		AddToTempTable(matrixIndex2*2+1,	s,		uv[1].y); 

		int matrixIndex3 = ComputeMatrixIndex(faceData, i,2,tvIndex[2]);
		AddToTempTable(matrixIndex3*2,		1.0f,	uv[2].x); 
		
		FillSparseMatrix();


		mNonPinnedVertsList.SetCount(0,FALSE);
		mPinnedVertsList.SetCount(0,FALSE);
//		DebugPrint("Add Row\n");

		AddToTempTable(matrixIndex1*2,		s,			uv[0].x); 
		AddToTempTable(matrixIndex1*2+1,	c - 1.0f,	uv[0].y); 

		AddToTempTable(matrixIndex2*2,		-s,			uv[1].x); 
		AddToTempTable(matrixIndex2*2+1,	-c,			uv[1].y); 

		AddToTempTable(matrixIndex3*2+1,	1.0f,		uv[2].y); 

		FillSparseMatrix();


	}

	int numberEntries = 0;
	for (int i = 0; i < mAMatrix.Size(); i++)
	{
		numberEntries += (int) mAMatrix.GetRow(i)->mData.size();
	}

	Tab<int> rowCounter;
	rowCounter.SetCount(mNumberCoefficients+1);

	Tab<float> val;
	val.SetCount(numberEntries);
	
	Tab<int> valueIndex;
	valueIndex.SetCount(numberEntries);


	SuperMatrix At, AtP;
	At.Store = nullptr;
	AtP.Store = nullptr;
	int info = 0, panel_size = 0, relax = 0;
	superlu_options_t options;

	// Convert M to compressed column format 
	for(int i = 0, count=0; i < mAMatrix.Size(); i++) 
	{
		 LSCMRow *row = mAMatrix.GetRow(i);
		rowCounter[i] = count;

		SparseData :: iterator mIter;
		mIter = row->mData.begin();

		for (int j = 0; j < row->mData.size(); j++, count++)
		{
			val[count] = mIter->second;
			valueIndex[count] = mIter->first;
//DebugPrint("i %d j %d   a %f asub %d \n",i,j,a[count],asub[count]);
			mIter++;
		}
	}
	rowCounter[mNumberCoefficients] = numberEntries;

	if  ( ( val.Count() > 0 ) && ( valueIndex.Count() > 0 ) && ( rowCounter.Count() > 0 ) )
		sCreate_CompCol_Matrix(	&At, mNumberCoefficients, mNumberCoefficients, numberEntries, val.Addr(0), valueIndex.Addr(0), rowCounter.Addr(0), 
			SLU_NC,	SLU_S,	SLU_GE	);

	// Set superlu options 
	options.Fact = DOFACT;
	options.Equil = YES;
	options.ColPerm = COLAMD;
	options.DiagPivotThresh = 1.0;
	options.Trans = NOTRANS;
	options.IterRefine = NOREFINE;
	options.SymmetricMode = NO;
	options.PivotGrowth = NO;
	options.ConditionNumber = NO;
	options.PrintStat = YES;
	options.ColPerm = MY_PERMC;
	options.Fact = DOFACT;

	StatInit(&mSuperLuStats);

	panel_size = sp_ienv(1);
	relax = sp_ienv(2);

	// Compute permutation and permuted matrix 
	mPermC.SetCount(mNumberCoefficients);
	mPermR.SetCount(mNumberCoefficients);

	for (int i = 0; i < mNumberCoefficients; i++)
	{
		mPermR[i] = 0;
		mPermC[i] = 0;
	}

	Tab<int> elimationTree;
	elimationTree.SetCount(mNumberCoefficients);
	
	if ( (mPermC.Count() > 0) && (elimationTree.Count() > 0) )
	{
		get_perm_c(3, &At, mPermC.Addr(0));

		sp_preorder(&options, &At, mPermC.Addr(0), elimationTree.Addr(0), &AtP);
	}

	
	if (mSuperLuLandUAllocated)
	{
		if(mL.Store)
		{
			Destroy_SuperNode_Matrix(&(mL));
			mL.Store = nullptr;
		}
		
		if(mU.Store)
		{
			Destroy_CompCol_Matrix(&(mU));
			mU.Store = nullptr;
		}
		
	}
	
	
	if ( (mPermC.Count() > 0) && (elimationTree.Count() > 0) && (mPermR.Count() > 0) )
	{
		// Decompose into L and U 
		sgstrf(&options, &AtP, 0.0f,
			relax, panel_size,	
			elimationTree.Addr(0), NULL, 0, 
			mPermC.Addr(0), mPermR.Addr(0),	&mL, &mU, 
			&mSuperLuStats, &info);

	}

	mSuperLuLandUAllocated = true;

	if(At.Store)
	{
		Destroy_SuperMatrix_Store(&At);
	}
	
	if(AtP.Store)
	{
		Destroy_SuperMatrix_Store(&AtP);
	}
}



void LSCMClusterData::FillSparseMatrix()
{
	int numberOfNonPinned		  = mNonPinnedVertsList.Count();
	int numberOfPinned		  = mPinnedVertsList.Count();
	for (int i = 0; i < numberOfNonPinned; i++)
	{
		for(int j = 0; j < numberOfNonPinned; j++) 
		{
			float val = mNonPinnedVertsList[i].mValue*mNonPinnedVertsList[j].mValue;
			int x = mNonPinnedVertsList[i].mIndex;
			int y = mNonPinnedVertsList[j].mIndex;

#ifdef DEBUG_COMPARE_NEW_OVAL_VALUE_WITH_OLD
			float oval = mAMatrix.GetRow(x)->mData[y];
#endif
			// Update the oval value
			mAMatrix.GetRow(x)->mData[y] += val;

#ifdef DEBUG_COMPARE_NEW_OVAL_VALUE_WITH_OLD
			DebugPrint("%d %d  value %f new value %f \n",x,y,oval,mAMatrix.GetRow(x)->mData[y]);
#endif
		}	
	}
	float sum = 0.0f;
	for(int i = 0; i < numberOfPinned; i++)
		sum += mPinnedVertsList[i].mValue;

	for(int i = 0; i < numberOfNonPinned; i++)
	{
		mBMatrix[ mNonPinnedVertsList[i].mIndex ] -= mNonPinnedVertsList[i].mValue * sum;
	}

}


void LSCMClusterData::Solve(MeshTopoData *md)
{
	if (mValidGeom ==  false) return;

	if (mNeedToResolve == false)
		return;

	if (mNumberCoefficients == 0)
		return;

	int count = mXMatrix.Count();

	SuperMatrix B;
	int info = 0;

	sCreate_Dense_Matrix(&B, count, 1, mBMatrix.Addr(0), count, SLU_DN, SLU_S,  SLU_GE  );

	sgstrs(TRANS, &(mL), &(mU),	mPermC.Addr(0), mPermR.Addr(0), &B,	&(mSuperLuStats), &info);

	mValidSolve = false;
	if(info == 0)
	{
		memcpy_s(mXMatrix.Addr(0), sizeof(float)*count, ((DNformat*)B.Store)->nzval, sizeof(float)*count);
		mValidSolve = true;
	}

	Destroy_SuperMatrix_Store(&B);

	return;
}


void LSCMClusterData::SendBackUVWs(UnwrapMod *mod, MeshTopoData *md, bool useSimplifyModel)
{
	if (mNeedToResolve == false)
		return;

	mNeedToResolve = false;

	if (mValidGeom ==  false) return;
	if (mValidSolve == false) return;

	TimeValue t = GetCOREInterface()->GetTime();
	//Tab<LSCMFace> &faceData = useSimplifyModel ? mSimFaceData : mFaceData;
	Tab<LSCMFace> &faceData = mFaceData;

	BitArray useVerts;
	useVerts.SetSize(md->GetNumberTVVerts());
	useVerts.ClearAll();

	for (int i=0; i<faceData.Count(); i++)
	{
		for (int j=0; j<3; j++)
		{
			int tvIndex = faceData[i].mEdge[j].mTVVert;
			useVerts.Set(tvIndex, TRUE);
		}
	}

	int currentIndex = 0;
	for (int i = 0; i < md->GetNumberTVVerts(); i++)
	{
		if (/*(md->IsTVVertPinned(i) == false) && */useVerts[i])
		{
			if (md->IsTVVertPinned(i))
			{
				// move pin verts
				Point3 val = md->GetTVVert(i);
				val += mLscmMove;
				md->SetTVVert(t,i,val);
			}
			else
			{
				Point3 val(0,0,0);

				val.x =  mXMatrix[currentIndex*2] + mLscmMove.x;
				val.y =  mXMatrix[currentIndex*2+1] + mLscmMove.y;

				if ((_isnan(val.x) == false) && (_isnan(val.y) == false))
					md->SetTVVert(t,i,val);

				currentIndex++;
			}
		}
	}


	// Calculate UV values on all the vertices on the original model
// 	if (useSimplifyModel)
// 	{
// 		ComplexifyModel(mod, md);
// 	}
}

void LSCMClusterData::FindGeoVertsByTV( MeshTopoData * md, BitArray mapFaces, int pVert1, int &pVertGeo1, int pVert2, int &pVertGeo2 )
{
	int ct = 0;
	for (int i = 0; i < md->GetNumberFaces(); i++)
	{
		if (mapFaces[i])
		{

			int degree = md->GetFaceDegree(i);
			for (int j = 0; j < degree; j++)
			{
				int tvIndex = md->GetFaceTVVert(i,j);
				int geomIndex = md->GetFaceGeomVert(i,j);
				if ((tvIndex == pVert1) && (pVertGeo1 == -1))
				{
					pVertGeo1 = geomIndex;

					//add them to our list

					ct++;
				}
				else if ((tvIndex == pVert2)  && (pVertGeo2 == -1))
				{
					pVertGeo2 = geomIndex;

					//add them to our list

					ct++;
				}
				if (ct >= 2)
				{
					j = degree;
					i = md->GetNumberFaces();
				}
			}
		}
	}
}

void LSCMClusterData::FindExistPinVerts( BitArray mapFaces, MeshTopoData * md )
{
	//loop through our verts looking for pin verts
	BitArray usedVerts;
	usedVerts.SetSize(md->GetNumberTVVerts());
	usedVerts.ClearAll();
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		if (mapFaces[polyIndex])
		{
			mapFaces.Clear(polyIndex);
			int degree = md->GetFaceDegree(polyIndex);


			for (int j = 0; j < degree; j++)
			{
				int tvIndex = md->GetFaceTVVert(polyIndex,j);
				if (md->IsTVVertPinned(tvIndex) && (!usedVerts[tvIndex]))
				{
					mPinVerts.Append(1,&tvIndex,200);
					usedVerts.Set(tvIndex,TRUE);
				}
			}
		}
	}
}

void LSCMClusterData::CalculatePinPos2D( MeshTopoData * md, const  int pVert1, const int pVert2, const int pVertGeo1, const  int pVertGeo2, const  TimeValue t )
{
	Point3 a = md->GetTVVert(pVert1);
	Point3 b = md->GetTVVert(pVert2);
	Point3 pa = md->GetGeomVert(pVertGeo1);
	Point3 pb = md->GetGeomVert(pVertGeo2);
	if (pVertGeo1 == pVertGeo2)
	{
		if (a.x < b.x)
		{
			a.x = -0.5f;
			a.y = 0.5f;
			b.x = 0.5f;
			b.y = 0.5f;
		}
		else
		{
			a.x = 0.5f;
			a.y = 0.5f;
			b.x = -0.5f;
			b.y = 0.5f;
		}
	}
	else
	{
		int diru, dirv, dirx, diry;
		Point3 sub = pa - pb;
		sub.x = fabs(sub.x);
		sub.y = fabs(sub.y);
		sub.z = fabs(sub.z);
		float auv[2];
		float buv[2];
		float paPos[3];
		float pbPos[3];
		paPos[0] = pa.x;	paPos[1] = pa.y;	paPos[2] = pa.z;
		pbPos[0] = pb.x;	pbPos[1] = pb.y;	pbPos[2] = pb.z;

		if ((sub.x > sub.y) && (sub.x > sub.z))
		{
			dirx = 0;
			diry = (sub.y > sub.z) ? 1 : 2;
		}
		else if ((sub.y > sub.x) && (sub.y > sub.z))
		{
			dirx = 1;
			diry = (sub.x > sub.z) ? 0 : 2;
		}
		else
		{
			dirx = 2;
			diry = (sub.x > sub.y) ? 0 : 1;
		}

		if (dirx == 2)
		{
			diru = 1;
			dirv = 0;
		}
		else
		{
			diru = 0;
			dirv = 1;
		}

 		auv[diru] = paPos[dirx];
 		auv[dirv] = paPos[diry];
 		buv[diru] = pbPos[dirx];
 		buv[dirv] = pbPos[diry];

		// map to -0.5, 0.5 area, however, it will cause the scale wrong
		// need to move center to (0, 0.5)? need more test
		auv[diru] = -0.5f;
		const float minDiff = 1e-7f;
		if (fabs(pbPos[dirx] - paPos[dirx]) < minDiff )
		{
			auv[dirv] = 0.5f;
		}
		else
		{
			auv[dirv] = 0.5f*(paPos[diry]-pbPos[diry])/(pbPos[dirx]-paPos[dirx]);
		}
		
		buv[diru] = 0.5f;
		buv[dirv] = -auv[dirv];

		a.x = auv[0];	a.y = auv[1];
		b.x = buv[0];	b.y = buv[1];
	}

	md->SetTVVert(t,pVert1,a);
	md->SetTVVert(t,pVert2,b);
}

void LSCMClusterData::PreparePinDataForLSCM( Tab<LSCMFace> &faceData, IMeshTopoData * md )
{
	mClusterVerts.SetSize(md->GetNumberTVVerts());
	mClusterVerts.ClearAll();
	for (int i = 0; i < faceData.Count(); i++)
	{
		for (int j = 0; j < 3; j++)	
		{			
			int tvIndex = faceData[i].mEdge[j].mTVVert;
			mClusterVerts.Set(tvIndex,TRUE);
		}

	}

	mTVIndexToMatrixIndex.SetCount(md->GetNumberTVVerts());
	int current = 0;
	for (int i = 0; i < md->GetNumberTVVerts(); i++)
	{
		if (md->IsTVVertPinned(i))
		{
			mTVIndexToMatrixIndex[i] = -1;			
		}
		else if (mClusterVerts[i])
		{
			mTVIndexToMatrixIndex[i] = current++;
		}

	}

	BitArray pinnedVerts;
	pinnedVerts.SetSize(md->GetNumberTVVerts());
	pinnedVerts.ClearAll();
	for (int k = 0; k < mPinVerts.Count(); k++)
	{
		if (mPinVerts[k] != -1)
			pinnedVerts.Set(mPinVerts[k],TRUE);
	}

	BitArray otherVerts;
	otherVerts.SetSize(md->GetNumberTVVerts());
	otherVerts.ClearAll();

	for (int i = 0; i < faceData.Count(); i++)
	{
		LSCMFace* face = &faceData[i];
		int tvIndex[3];
		tvIndex[0] = face->mEdge[0].mTVVert;
		tvIndex[1] = face->mEdge[1].mTVVert;
		tvIndex[2] = face->mEdge[2].mTVVert;

		face->mEdge[0].mPinned = false;
		face->mEdge[1].mPinned = false;
		face->mEdge[2].mPinned = false;

		if (pinnedVerts[tvIndex[0]])
		{
			face->mEdge[0].mPinned = true;
		}
		else
			otherVerts.Set(tvIndex[0]);

		if (pinnedVerts[tvIndex[1]])
		{
			face->mEdge[1].mPinned = true;
		}
		else
			otherVerts.Set(tvIndex[1]);


		if (pinnedVerts[tvIndex[2]])
		{
			face->mEdge[2].mPinned = true;
		}
		else
			otherVerts.Set(tvIndex[2]);

	}

	mNumberCoefficients = otherVerts.NumberSet() * 2;
}

void LSCMClusterData::OptimizeTVertShareGeoVert( MeshTopoData * md, BitArray mapFaces, int pVertGeo1, int pVert1, int &pVert2, int &pVertGeo2, Box3 &potentialBounds, Box3 &tvBounds, UnwrapMod * mod )
{
	for (int i = 0; i < md->GetNumberFaces(); i++)
	{
		if (mapFaces[i])
		{
			int degree = md->GetFaceDegree(i);
			for (int j = 0; j < degree; j++)
			{
				int tvIndex = md->GetFaceTVVert(i,j);
				int geomIndex = md->GetFaceGeomVert(i,j);
				if (geomIndex == pVertGeo1)
				{
					if (tvIndex != pVert1)
					{
						Point3 pa = md->GetTVVert(pVert1);
						Point3 pb = md->GetTVVert(tvIndex);
						float d = Length(pa-pb);
						//cannot of coincident verts
						if (d > 0.001)
						{
							pVert2 = tvIndex;
							pVertGeo2 = geomIndex;
							j = degree;
							i = md->GetNumberFaces();
						}
						//they are on top of each other so spread them apart
						//along the shortest axis
						else
						{
							float d = potentialBounds.Width().x;
							int axis = 0;
							if (d == 0.0f)
							{
								d = potentialBounds.Width().y;
								axis = 1;
							}
							else
							{
								if (d < potentialBounds.Width().y)
								{
									d = potentialBounds.Width().y;
									axis = 1;
								}
							}
							if (d > 0.0f)
							{
								pVert2 = tvIndex;
								pVertGeo2 = geomIndex;
								float expandDist = tvBounds.Width()[axis] * 0.5f;

								TimeValue t = GetCOREInterface()->GetTime();
								Point3 tva = md->GetTVVert(pVert1);
								Point3 tvb = md->GetTVVert(pVert2);
								tva[axis] -= expandDist;
								tvb[axis] += expandDist;
								md->SetTVVert(t,pVert1,tva);
								md->SetTVVert(t,pVert2,tvb);
								i = md->GetNumberFaces();
								j = degree;
							}
						}
					}
				}				
			}
		}
	}
}

void LSCMClusterData::NormalizeQuickPeeledResult(UnwrapMod *mod, MeshTopoData *md)
{
	if(nullptr == mod ||
		nullptr == md)
	{
		return;
	}

	BitArray processedVerts;
	processedVerts.SetSize(md->GetNumberTVVerts());
	processedVerts.ClearAll();

	TimeValue t = GetCOREInterface()->GetTime();

	float minx = FLT_MAX,miny = FLT_MAX;
	float maxx = FLT_MIN,maxy = FLT_MIN;
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		for (int j = 0; j < 3; j++)
		{
			int tvIndex = md->GetFaceTVVert(polyIndex,mFaceData[i].mEdge[j].mIthEdge);
			if(!processedVerts[tvIndex]&&
				!md->GetTVVertDead(tvIndex)&&
				!md->GetTVSystemLock(tvIndex))
			{
				Point3 p = md->GetTVVert(tvIndex);
				if (p.x<minx) minx = p.x;
				if (p.y<miny) miny = p.y;
				if (p.x>maxx) maxx = p.x;
				if (p.y>maxy) maxy = p.y;
				processedVerts.Set(tvIndex);
			}
			
		}
	}

	processedVerts.ClearAll();

	float w = maxx-minx;
	float h = maxy-miny;
	float fMultiply = 1.0;
	float amount = w > h ? w : h;
	if(fabs(amount) > std::numeric_limits<float>::epsilon())
	{
		fMultiply = 1.0/amount;
	}
	for (int i = 0; i < mFaceData.Count(); i++)
	{
		int polyIndex = mFaceData[i].mPolyIndex;
		for (int j = 0; j < 3; j++)
		{
			int tvIndex = md->GetFaceTVVert(polyIndex,mFaceData[i].mEdge[j].mIthEdge);
			if(!processedVerts[tvIndex]&&
				!md->GetTVVertDead(tvIndex)&&
				!md->GetTVSystemLock(tvIndex))
			{
				Point3 p = md->GetTVVert(tvIndex);
				p.x -= minx;
				p.y -= miny;
				p.x = p.x * fMultiply;
				p.y = p.y * fMultiply;
				md->SetTVVert(t,tvIndex,p);
				processedVerts.Set(tvIndex);
			}
		}
	}
}

LSCMLocalData::LSCMLocalData(UnwrapMod *mod, MeshTopoData* md, bool useExistingMapping, bool useSelectedFaces)
{
	mMod = mod;
	mMd = md;
	mTopoChange = false;

	mTimeToSolve = 0;

	mUseExistingMapping = useExistingMapping;
	mUseAbf = true;

	mChangedPin.SetSize(md->GetNumberTVVerts());
	mChangedPin.ClearAll();
	mMovedPin = mChangedPin;

	mUsedFaces.SetSize(mMd->GetNumberFaces());
	mUsedFaces.ClearAll();
	if (useSelectedFaces)
	{
		BitArray faceSel = md->GetFaceSel();		
		mUsedFaces = ~faceSel; 
	}
	else if (mod->fnGetFilteredSelected() || mod->fnGetMatID() != 1) // if not use selected faces but in "Display only selected" mode, use the face filter.
	{
		//mod->fnGetMatID() == 1 means all faces pass the materialID filter, 
		//then mod->fnGetMatID() != 1 means some face pass the materialID filter.
		BitArray faceSel = md->GetFaceFilter();
		mUsedFaces = ~faceSel;
	}

	BuildClusterData();
}

LSCMLocalData::~LSCMLocalData()
{
	Free();
}

void LSCMLocalData::Free()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		if (mClusterData[i])
			delete mClusterData[i];
		mClusterData[i] = NULL;
	}
	mClusterData.ZeroCount();
}

MeshTopoData* LSCMLocalData::GetLocalData()
{
	return mMd;
}

int LSCMLocalData::NumberClusterData()
{
	return mClusterData.Count();
}

LSCMClusterData* LSCMLocalData::GetClusterData(int iIndex)
{
	if(iIndex >= 0 && iIndex < mClusterData.Count())
	{
		return mClusterData[iIndex];
	}

	return nullptr;
}

void LSCMLocalData::AddCluster(Tab<int> &faces)
{
	if (!faces.Count()) return;

	LSCMClusterData *data = new LSCMClusterData();
	

	for (int i = 0; i < faces.Count(); i++)
	{
		int faceIndex = faces[i];
		data->Add(faceIndex,mMd);
	}
	mClusterData.Append(1,&data,100);
}

void LSCMLocalData::ApplyMapping()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->ApplyMapping(mMd,mMod);
		mMd->SetTVEdgeInvalid();
		mMd->BuildTVEdges();
	}

	if (mMod->matid != -1) // if we have a matID fileter set we need to rebuild since topology has changed
		mMod->SetMatFilters();
}

void LSCMLocalData::ComputePins()
{

	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->ComputePins(mMd,mMod,mUseExistingMapping);
	}
}

void LSCMLocalData::ComputeFaceAngles()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->ComputeFaceAngles(mMd);
	}
}

void LSCMLocalData::ComputeLABFAngles()
{
	if (!mUseAbf) return;

	for (int i = 0; i < mClusterData.Count(); i++)
	{
		ComputeLABFFaceAngles(mClusterData[i], mMd, mMod->abfErrorBound, mMod->abfMaxItNum);
	}
}

void LSCMLocalData::ComputeABFPlusPlusAngles()
{
	if (!mUseAbf) return;

	for (int i = 0; i < mClusterData.Count(); i++)
	{
		ComputeABFPlusPlusFaceAngles(mClusterData[i], mMd, mMod->useSimplifyModel, mMod->abfMaxItNum);
	}
}

//this just fills out the angles which is needed for the solve
void LSCMLocalData::ComputeMatrices()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->ComputeMatrices(mMd, mMod->useSimplifyModel);
	}
}
void LSCMLocalData::Solve()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->Solve(mMd);
	}
}

void LSCMLocalData::SendBackUVWs()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->SendBackUVWs(mMod,mMd,mMod->useSimplifyModel);
	}

}

void LSCMLocalData::GetGeomCluster(UnwrapMod *mod, MeshTopoData* ld, int &startFace, BitArray &usedFaces, Tab<int> &faces)
{
	_GetCluster(mod, ld, startFace, usedFaces, faces, false);
}

void LSCMLocalData::GetTVCluster(UnwrapMod *mod, MeshTopoData* ld, int &startFace, BitArray &usedFaces, Tab<int> &faces)
{
	_GetCluster(mod, ld, startFace, usedFaces, faces, true);
}

void LSCMLocalData::_GetCluster(UnwrapMod *mod, MeshTopoData* ld, int &startFace, BitArray &usedFaces, Tab<int> &faces, bool isTVSpace)
{
	//get the start index
	int numberFaces = ld->GetNumberFaces();
	BitArray selFaces = ld->GetFaceSel();
	faces.SetCount(0);

	// make sure the start face is a valid face and not used yet.
	if (usedFaces[startFace] || (ld->GetFaceDegree(startFace) < 3))
	{
		for (int i = startFace; i < usedFaces.GetSize(); i++)
		{
			// directly mark those invalid face as used.
			if (ld->GetFaceDegree(i) < 3)
			{
				usedFaces.Set(i);
			}

			if (!usedFaces[i])
			{
				startFace = i;
				break;
			}
		}
	}

	//get our tv element from the start face
	ld->ClearFaceSelection();

	ld->SetFaceSelected(startFace,TRUE);
	if (ld->DoesFacePassFilter(startFace))
	{
		if (isTVSpace)
		{
			ld->SelectElement(TVFACEMODE,TRUE);
		}
		else
		{
			mod->SelectGeomElement(ld, FALSE);
		}
		
		for (int i = 0; i < numberFaces; i++)
		{
			if (!usedFaces[i] && ld->GetFaceSelected(i))
			{
				faces.Append(1,&i,10000);
			}
		}
	}
	usedFaces |= ld->GetFaceSel();

	ld->SetFaceSelectionByRef(selFaces);
}

void  LSCMLocalData::InvalidatePin(int index)
{
	if (index >= mMovedPin.GetSize())
	{
		mMovedPin.SetSize(index+1,1);
		mChangedPin.SetSize(index+1,1);
	}

	mMovedPin.Set(index);
}
void LSCMLocalData::InvalidatePinAddDelete(int index)
{
	if (index >= mChangedPin.GetSize())
	{
		mMovedPin.SetSize(index+1,1);
		mChangedPin.SetSize(index+1,1);
	}

	mChangedPin.Set(index);
}
void LSCMLocalData::InvalidateTopo()
{
	mTopoChange = true;
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->Invalidate();
	}
}

double LSCMLocalData::GetSolveTime()
{
	return mTimeToSolve;
}

bool LSCMLocalData::Resolve(bool allClusters)
{
	BitArray holdVertSel;
	holdVertSel.SetSize(mMd->GetNumberTVVerts());
	holdVertSel = mMd->GetTVVertSel();

	bool solveHappened = false;
	double tempTime = 0.0;

	if (mTopoChange )
	{
		for (int i = 0; i < mClusterData.Count(); i++)
		{
			mClusterData[i]->Invalidate();
		}
		mMovedPin.SetSize(mMd->GetNumberTVVerts());
		mChangedPin.SetSize(mMd->GetNumberTVVerts());
		mMovedPin.ClearAll();
		mChangedPin.ClearAll();
	}
	else if ( mMovedPin.NumberSet() || mChangedPin.NumberSet() )
	{		
		for (int i = 0; i < mClusterData.Count(); i++)
		{
			mClusterData[i]->Invalidate(mMovedPin,mMd);
			mClusterData[i]->Invalidate(mChangedPin,mMd);
		}
	}

	if (mTopoChange )
	{
		solveHappened = true;

		MaxSDK::PerformanceTools::Timer timer;
		timer.StartTimer();

		if (!allClusters)
		{
			PreprocessUsedFaces();
		}

		//cut or sew the seams
		theHold.Suspend();
		BitArray &seam = mMd->mSeamEdges;
		if (seam.NumberSet())
		{
			if (mMd->GetSewingPending())
			{
				GeoTVEdgesMap gtvInfo;
				mMd->GetGeomEdgeRelatedTVEdges(seam, gtvInfo);
				mMd->SewEdges(gtvInfo);
				//sewing seams doesn't accumulate
				seam.ClearAll();
				mMd->ClearSewingPending();
			}
			else
			{
				BitArray edgeSel(mMd->GetTVEdgeSel().GetSize());
				mMd->ConvertGeomEdgeSelectionToTV(seam, edgeSel);
				mMd->BreakEdges(edgeSel);
			}
		}
		theHold.Resume();

		Free();
		BuildClusterData();

		//compute the face angles
		ComputeFaceAngles();
	
		tempTime += timer.EndTimer();
	}

	if (mTopoChange || mMovedPin.NumberSet() || mChangedPin.NumberSet())
	{
		solveHappened = true;

		MaxSDK::PerformanceTools::Timer timer;
		timer.StartTimer();
	
		//see if add/delete pin
		//these need to be recomputed if in this case
		ComputePins();

		// ABF is much more time consuming than LSCM
		// If only pin points are operated without topology change, time for ABF can be saved here.
		if (mTopoChange)
		{
			// use abf first
			//SimplifyModel();
			//ComputeLABFAngles();
			ComputeABFPlusPlusAngles();
		}

		//see if pin moved this 
		//compute our matrices only B matrix needs to be computed
		ComputeMatrices();

		//solve our solution
		Solve();

		SendBackUVWs();		
		RescaleClusters();

		tempTime += timer.EndTimer();
	}

	mTimeToSolve = tempTime;

	mMovedPin.ClearAll();
	mChangedPin.ClearAll();
	
	if(mTopoChange)
	{
		mTopoChange = false;
	}

	int nv = mMd->GetNumberTVVerts();
	holdVertSel.SetSize(nv,1);
	mMd->SetTVVertSel(holdVertSel);
	return solveHappened;
}

void LSCMLocalData::RescaleClusters()
{
	Tab<int> clusterDefinitions;
	clusterDefinitions.SetCount(mMd->GetNumberFaces());
	for (int i = 0; i < clusterDefinitions.Count(); i++)
		clusterDefinitions[i] = -1;
	BitArray processFaces;
	processFaces.SetSize(mMd->GetNumberFaces());
	processFaces.ClearAll();

//get our cluster scale values
	Tab<float> rescale;
	rescale.SetCount(mClusterData.Count());

	Tab<float> groupScales;
	groupScales.SetCount(mMod->pblock->Count(unwrap_group_density));
	for (int i = 0; i < groupScales.Count(); i++)
	{
		groupScales[i] = mMod->pblock->GetFloat(unwrap_group_density,0,i);
	}

	for (int i = 0; i < mClusterData.Count(); i++)
	{
		rescale[i] = 1.0f;
		//DebugPrint("Cluster %d  pin count %d\n",i,(mClusterData[i]->TempPinCount()));
		if (mClusterData[i]->TempPinCount() > 1)
		{
			int ct = mClusterData[i]->NumberFaces();
			//DebugPrint("Faces   ");
			for (int j = 0; j < ct; j++)
			{
				LSCMFace *face = mClusterData[i]->GetFace(j);
//if the cluster is user defined get the rescale value attached to that cluster otherwise it will be 1.0f
				int groupID = mMd->GetToolGroupingData()->GetGroupID(face->mPolyIndex);
				if (groupID != -1)
				{
					rescale[i] = groupScales[groupID];
				}
					
				if (processFaces[face->mPolyIndex] == false)
				{
					clusterDefinitions[face->mPolyIndex] = i;
			//		DebugPrint("%d, ",face->mPolyIndex);
					processFaces.Set(face->mPolyIndex);
				}
			}		
			//DebugPrint("\n");
		}
		mClusterData[i]->UnmarkTempPins(mMd);		

	}


	mMd->RescaleClusters(clusterDefinitions,rescale);

}

void LSCMLocalData::BuildClusterData()
{
	ConstantTopoAccelerator topoAccl(mMd, TRUE);

	//find our clusters
	int startFace = 0;
	BitArray processedFaces = mUsedFaces;
	Tab<int> faces;
	int numberFace = mMd->GetNumberFaces();

	int processedFaceCount = processedFaces.NumberSet();
	while (processedFaceCount != numberFace)
	{
		faces.SetCount(0);
		GetTVCluster(mMod, mMd, startFace, processedFaces, faces);
		AddCluster(faces);
		processedFaceCount = processedFaces.NumberSet();
	}
	//DebugPrint(_T("cluster count: %d\n"), mClusterData.Count());
}

void LSCMLocalData::NormalizeQuickPeeledResult()
{
	for (int i = 0; i < mClusterData.Count(); i++)
	{
		mClusterData[i]->NormalizeQuickPeeledResult(mMod,mMd);
	}
}

void LSCMLocalData::PreprocessUsedFaces()
{
	//when cutting the edge that will automatically change to the seam, the related cluster will be created in the 
	//BuildClusterData() function and then invoke the ABF and LSCM method.		
	if (mMod->fnGetTVSubMode() == TVEDGEMODE &&
		mMod->fnGetMapMode() == LSCMMAP &&
		mMod->GetLivePeelModeEnabled())
	{
		int iFaceCount = mMd->GetNumberFaces();
		BitArray fOriginalSel = mMd->GetFaceSel();

		//Select one face whose one edge is selected.
		int iSelectedFaceIndex = -1;
		if (mMd->GetTVEdgeSel().AnyBitSet())
		{
			for (int i = 0; i < mMd->GetNumberTVEdges(); ++i)
			{
				if (mMd->GetTVEdgeSelected(i) &&
					mMd->GetTVEdgeNumberTVFaces(i) > 0)
				{
					iSelectedFaceIndex = mMd->GetTVEdgeConnectedTVFace(i, 0);
					if (iSelectedFaceIndex >= 0 &&
						iSelectedFaceIndex < iFaceCount &&
						mMd->DoesFacePassFilter(iSelectedFaceIndex))
					{
						break;
					}
				}
			}
		}

		//From the selected face, select the whole cluster,update the mUsedFaces that will affect the BuildClusterData() function.
		if (iSelectedFaceIndex >= 0 &&
			iSelectedFaceIndex < iFaceCount)
		{
			mMd->ClearFaceSelection();
			mMd->SetFaceSelected(iSelectedFaceIndex, TRUE);
			mMd->SelectElement(TVFACEMODE, TRUE);

			mUsedFaces.SetSize(iFaceCount);
			mUsedFaces.ClearAll();
			BitArray faceSel = mMd->GetFaceSel();
			//All the other unselected faces will be considered as processed.
			mUsedFaces = ~faceSel;
		}
		else
		{
			//If no edge is selected, then all cluster will be considered as processed.
			mUsedFaces.SetAll();
		}

		//restore the original face selection
		mMd->SetFaceSel(fOriginalSel);
	}
}

ToolLSCM::ToolLSCM() : mTimeClusterCreation(0.0), mTimeMatrixConstruction(0.0), 
	mTimeSolve(0.0), mTimeCut(0.0), mTimePin(0.0)
{
	mMod = NULL;
	mTimeOverAll = 0.0;
	
}
ToolLSCM::~ToolLSCM()
{
	Free();
}

void ToolLSCM::ApplyMapping(bool useSelection, Tab<MeshTopoData*> &localData, UnwrapMod *mod)
{
	for (int i = 0; i < localData.Count(); i++)
	{
		for (int j = 0; j < localData[i]->GetNumberTVVerts(); j++)
		{
			localData[i]->TVVertUnpin(j);
		}

		BitArray faceSel;
		faceSel = localData[i]->GetFaceSel();

		if (useSelection == false)
			faceSel.SetAll();

		//detach faces
		BitArray vertSel;
		localData[i]->DetachFromGeoFaces(faceSel,vertSel);
		localData[i]->SetTVEdgeInvalid();
		localData[i]->BuildTVEdges();
		localData[i]->BuildVertexClusterList();
		
	}

	if (mMod->matid != -1) // if we have a matID fileter set we need to rebuild since topology has changed
		mMod->SetMatFilters();
}

void ToolLSCM::CutSeams(Tab<MeshTopoData*> &localData, UnwrapMod *mod)
{
	theHold.Suspend();
	for (int i = 0; i < localData.Count(); i++)
	{
		BitArray seam = localData[i]->mSeamEdges;
		if (seam.NumberSet())
		{
			BitArray uvEdgeSel = localData[i]->GetTVEdgeSel();
			localData[i]->ConvertGeomEdgeSelectionToTV(seam, uvEdgeSel );
			localData[i]->BreakEdges(uvEdgeSel);
			if (localData[i]->GetNumberTVEdges() != localData[i]->GetTVEdgeSel().GetSize())
			{
				localData[i]->GetTVEdgeSelectionPtr()->SetSize(localData[i]->GetNumberTVEdges());
			}
		}
	}
	theHold.Resume();

}

void ToolLSCM::Pack()
{
	BOOL holdRescale = mMod->fnGetPackRescaleCluster();
	mMod->fnSetPackRescaleCluster(FALSE);
	{
		HoldSuspend holdSuspend;
		mMod->fnPackNoParams();
	}
	mMod->fnSetPackRescaleCluster(holdRescale);

}

void ToolLSCM::PackNormalize(bool isAutoPack=false)
{
	if(nullptr == mMod) return;
	mMod->PackFull(isAutoPack, true);
}

bool ToolLSCM::Start( bool useExistingMapping, UnwrapMod *mod, Tab<MeshTopoData*> &localData)
{
	if(NULL == mod)
	{
		return false;
	}

	mMod = mod;
	MaxSDK::PerformanceTools::Timer timer;

	//clean out data if we ended abnormally from the last time
	//free our data after the free action is removed from the ToolLSCM::End function
	Free();

	for (int ldID = 0; ldID < localData.Count(); ldID++)
	{
		localData[ldID]->HoldSelection();
	}

	int holdSelLevel = mod->fnGetTVSubMode();
	mod->fnSetTVSubMode(TVFACEMODE);


	//see if have any selected faces, if so we only operate on the selected faces
	bool useSelectedFaces = false;
 	if (holdSelLevel == TVFACEMODE)
 	{
 		for (int ldID = 0; ldID < localData.Count(); ldID++)
 		{
 			if (localData[ldID]->GetFaceSel().NumberSet() > 0)
 			{
 				useSelectedFaces = true;
 				break;
 			}
 		}
 	}

	//split our seams
	if (!useExistingMapping)
	{
		//apply our default mapping
		ApplyMapping(useSelectedFaces, localData, mod);
	}

	//cut our seams
	timer.StartTimer();
	CutSeams(localData, mod);
	mTimeCut =  timer.EndTimer();

	timer.StartTimer();
	mLocalData.SetCount(localData.Count());
	for (int i = 0; i < localData.Count(); i++)
	{
		//initialize each local data
		LSCMLocalData *data = new LSCMLocalData(mod,localData[i],useExistingMapping,useSelectedFaces);
		mLocalData[i] = data;
		//make sure we invalidate so we get a solve
		mLocalData[i]->InvalidateTopo();
	}

	//If meet the texture height/width ratio is not the 1.0, then the coordinate X/Y axis' unit will not be consistence,
	//adjust the v data according to the texture height/width ratio.	
	NonSquareAdjustVertexY(true);

	mTimeClusterCreation = timer.EndTimer();

	//fill out the LCSM data
	timer.StartTimer();

	for (int i = 0; i < mLocalData.Count(); i++)
	{
		//compute the face angles
		mLocalData[i]->ComputeFaceAngles();	

		//Compute the pin verts if we need be
		mLocalData[i]->ComputePins();

		//mLocalData[i]->SimplifyModel();
		//mLocalData[i]->ComputeLABFAngles();	// Linear Angle Based Flattening
		mLocalData[i]->ComputeABFPlusPlusAngles();	// ABF++
	}
	mTimePin = timer.EndTimer();

	//compute our matrices
	timer.StartTimer();
	for (int i = 0; i < mLocalData.Count(); i++)
	{
		mLocalData[i]->ComputeMatrices();
	}
	mTimeMatrixConstruction = timer.EndTimer();


	//solve our solution
	timer.StartTimer();
	for (int i = 0; i < mLocalData.Count(); i++)
	{
		mLocalData[i]->Solve();
	}
	mTimeSolve = timer.EndTimer();


	for (int i = 0; i < mLocalData.Count(); i++)
	{
		mLocalData[i]->SendBackUVWs();		
	}

	theHold.Suspend();
	for (int i = 0; i < mLocalData.Count(); i++)
	{	
		mLocalData[i]->RescaleClusters();		
	}

	//Normalize all quick peeled result into the range [0.0,1.0]
	if (!useExistingMapping)
	{
		for (int i = 0; i < mLocalData.Count(); i++)
		{
			mLocalData[i]->NormalizeQuickPeeledResult();
		}
	}
	theHold.Resume();

	//If meet the texture height/width ratio is not the 1.0, then the coordinate X/Y axis' unit will not be consistence,
	//adjust the v data according to the texture height/width ratio.	
	NonSquareAdjustVertexY(false);

	//do a solve
	if (!useExistingMapping || mod->GetAutoPackEnabled())
	{
		theHold.Suspend();
		if (!useExistingMapping)
			mMod->fnPackNoParams(); //the use existing mapping happens on reset so we don't have to worry about realtime pack
		else 
			PackNormalize(true); //this is a fast full object pack
		theHold.Resume();
	}

	mod->fnSetTVSubMode(holdSelLevel);

	for (int ldID = 0; ldID < localData.Count(); ldID++)
	{
		localData[ldID]->RestoreSelection();
	}

	TimeValue t = GetCOREInterface()->GetTime();

	mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (mod->ip) mod->ip->RedrawViews(t);
	mod->InvalidateView();
	

	mTimeOverAll = mTimeClusterCreation + mTimeCut + mTimeMatrixConstruction + mTimePin + mTimeSolve;

	return true;
}

bool ToolLSCM::Solve(bool hasToSolve, bool sendNotify, bool allClusters)
{

	bool forceSolve = false;
	if (hasToSolve)
		forceSolve = true;
	else
	{
		if (mTimeOverAll < 500.0)
		{
			forceSolve = true;
		}
	}
	if (forceSolve)
	{
		if (mMod)
		{
			//If meet the texture height/width ratio is not the 1.0, then the coordinate X/Y axis' unit will not be consistence,
			//adjust the v data according to the texture height/width ratio.
			NonSquareAdjustVertexY(true);
		}

		double tempTime = 0.0;
		bool somethingSolved = false;
		for (int i = 0; i < mLocalData.Count(); i++)
		{
			mLocalData[i]->GetLocalData()->HoldSelection();
			if (mLocalData[i]->Resolve(allClusters) == true)
			{			
				somethingSolved = true;
			}
			tempTime += mLocalData[i]->GetSolveTime();
			mLocalData[i]->GetLocalData()->RestoreSelection();
		}

		if (tempTime > 0.0)
			mTimeOverAll = tempTime;

		if (mMod)
		{
			//If meet the texture height/width ratio is not the 1.0, then the coordinate X/Y axis' unit will not be consistence,
			//adjust the v data according to the texture height/width ratio.
			NonSquareAdjustVertexY(false);
		}

		if (mMod && sendNotify && somethingSolved)
		{
			if(mMod->GetAutoPackEnabled())
			{
				PackNormalize(true);
			}
			mMod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
			mMod->InvalidateView();
		}
	}

	return true;
}

bool ToolLSCM::End()
{	
	return true;
}

void ToolLSCM::Free()
{
	for (int i = 0; i < mLocalData.Count(); i++)
	{
		if (mLocalData[i])
			delete mLocalData[i];
		mLocalData[i] = NULL;
	}
	mLocalData.SetCount(0);
}

LSCMLocalData* ToolLSCM::GetData(MeshTopoData *md)
{
	for (int i = 0; i < mLocalData.Count(); i++)
	{
		if (mLocalData[i]->GetLocalData() == md)
			return mLocalData[i];
	}
	return NULL;
}

void ToolLSCM::InvalidatePin(MeshTopoData *md, int index)
{
	LSCMLocalData *ld = GetData(md);
	if (ld)
		ld->InvalidatePin(index);
}

void ToolLSCM::InvalidatePinAddDelete(MeshTopoData *md, int index)
{
	LSCMLocalData *ld = GetData(md);
	if (ld)
		ld->InvalidatePinAddDelete(index);
}

void ToolLSCM::InvalidateTopo(MeshTopoData *md)
{
	LSCMLocalData *ld = GetData(md);
	if (ld)
		ld->InvalidateTopo();
}

void ToolLSCM::NonSquareAdjustVertexY(bool bInverse)
{
	// This fixup is important and cosmetic. Imagine you have a cylinder, it breaks down as a rectangle
	// for the body and two circles for the top/bottom caps. Now attempting to map this onto a non-square
	// texture will cause the circles to stretch into ellipses to avoid texture stretching. However, this
	// makes it complicated to paint as you end up having to paint an ellipse that renders as a circle.
	// To fix this, we scale the height to compensate against the aspect ratio. This transforms the ellipse
	// back into a circle in UV space.
	//
	// In order to avoid drifting to occur after repeated solving calls, this fixup must be removed prior
	// to solving. That is, the UV circle is converted back into an ellipse, we solve which yields back the same
	// ellipse, and apply the fixup again in order to end up with the same identical circle.

	if (!mMod->fnGetNonSquareApplyBitmapRatio())
	{
		return;
	}

	static float aspectThreshold = 0.01f;
	if (abs(mMod->aspect - 1.0f) < aspectThreshold)
	{
		return;
	}

	TimeValue t = GetCOREInterface()->GetTime();
	const float aspectScale = bInverse ? (1.0f / mMod->aspect) : mMod->aspect;
	for (int i = 0; i < mMod->GetMeshTopoDataCount(); ++i)
	{
		MeshTopoData* md = mMod->GetMeshTopoData(i);
		LSCMLocalData* lscmLocalData = GetData(md);
		if (NULL == lscmLocalData)
		{
			continue;
		}

		const int numVerts = md->GetNumberTVVerts();
		if (numVerts == 0)
		{
			continue;
		}

		double fYSum = 0.0;
		for (int vtxIdx = 0; vtxIdx < numVerts; ++vtxIdx)
		{
			fYSum += md->GetTVVert(vtxIdx).y;
		}

		const float fYCenter = float(fYSum / numVerts);
		for (int vtxIdx = 0; vtxIdx < numVerts; ++vtxIdx)
		{
			Point3 vtx = md->GetTVVert(vtxIdx);
			vtx.y = (vtx.y - fYCenter) * aspectScale + fYCenter;
			md->SetTVVert(t, vtxIdx, vtx);
		}
	}
}
