/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#include "stdafx.h"
#include "LSCMData.h"
#include "trig.h"


LSCMMatrix::LSCMMatrix()
{}

LSCMMatrix::~LSCMMatrix()
{
	Free();
}

void LSCMMatrix::SetSize(int size)
{
	Free();
	mRow.SetCount(size);
	for (int i = 0; i < mRow.Count(); i++)
	{
		mRow[i] = new LSCMRow();
	}
}

int LSCMMatrix::Size()
{
	return mRow.Count();
}

void LSCMMatrix::Free()
{
	for (int i = 0; i < mRow.Count(); i++)
	{
		if (mRow[i])
			delete mRow[i];
		mRow[i] = NULL;
	}
	mRow.SetCount(0);
}

LSCMRow* LSCMMatrix::GetRow(int index)
{
	if ((index < 0) || (index >= Size()))
		return NULL;
	return mRow[index];
}


FastSparseMatrix::FastSparseMatrix(LSCMMatrix &matrix, int rowLength, int colLength)
	:numRows(rowLength), numCols(colLength)
{
	// Used to accelerate Matrix Arithmetic
	numEntries = 0;
	int totalRowCount = matrix.Size();
	for (int i = 0; i < totalRowCount; i++)
	{
		numEntries += (int)matrix.GetRow(i)->mData.size();
	}

	rowCounter.SetCount(totalRowCount + 1);
	val.SetCount(numEntries);
	valueIndex.SetCount(numEntries);

	// Convert A to compressed column format 
	for (int i = 0, count = 0; i < totalRowCount; i++)
	{
		LSCMRow *row = matrix.GetRow(i);
		rowCounter[i] = count;

		SparseData::iterator mIter;
		mIter = row->mData.begin();

		for (int j = 0; j < row->mData.size(); j++, count++)
		{
			val[count] = mIter->second;
			valueIndex[count] = mIter->first;
			mIter++;
		}
	}
	rowCounter[totalRowCount] = numEntries;
}

void FastSparseMatrix::getTransform(FastSparseMatrix &Tmat)
{
	Tmat.numEntries = numEntries;
	Tmat.numCols = numRows;
	Tmat.numRows = numCols;
	Tmat.rowCounter.SetCount(numCols + 1);
	Tmat.val.SetCount(numEntries);
	Tmat.valueIndex.SetCount(numEntries);

	Tab<int> colFirstIndex;
	colFirstIndex.SetCount(numCols + 1);
	for (int i = 0; i<numCols + 1; i++)
	{
		colFirstIndex[i] = 0;
	}
	for (int i = 0; i<numEntries; i++)
	{
		colFirstIndex[valueIndex[i] + 1]++;
	}
	for (int i = 1; i<numCols + 1; i++)
	{
		colFirstIndex[i] += colFirstIndex[i - 1];
		Tmat.rowCounter[i] = colFirstIndex[i];
	}	// [0 , numCols-1] is valid
	Tmat.rowCounter[0] = 0;

	int row = 0;
	for (int i = 0; i<numEntries; i++)
	{
		if (i == rowCounter[row + 1]) row++;
		int pos = colFirstIndex[valueIndex[i]];
		Tmat.valueIndex[pos] = row;
		Tmat.val[pos] = val[i];

		colFirstIndex[valueIndex[i]]++;
	}
}


LSCMFace::LSCMFace()
{
	mEdge[0].mIthEdge = -1;
	mEdge[1].mIthEdge = -1;
	mEdge[2].mIthEdge = -1;


	mEdge[0].mGeoAngle = 0.0f;
	mEdge[1].mGeoAngle = 0.0f;
	mEdge[2].mGeoAngle = 0.0f;


	mEdge[0].mPinned = false;
	mEdge[1].mPinned = false;
	mEdge[2].mPinned = false;

	mPolyIndex = -1;

	orderUpdated = false;
}

LSCMFace::LSCMFace(HLSCMFace &hFace)
{
	// when LSCMFace is obtained from HLSCMFace, 
	// mPolyIndex, mIthEdge is not useful
	mPolyIndex = -1;
	orderUpdated = false;
	for (int i = 0; i<3; i++)
	{
		mEdge[i].mIthEdge = -1;
		mEdge[i].mGeoVert = hFace.mVerts[i].mGeoVert;
		mEdge[i].mGeoAngle = hFace.mVerts[i].mGeoAngle;
		mEdge[i].mTVVert = hFace.mVerts[i].mTVVert;
		mEdge[i].mSin = hFace.mVerts[i].mSin;
		mEdge[i].mPinned = hFace.mVerts[i].mPinned;
	}
}

void LSCMFace::swapFaceVert(int id1, int id2)
{
	LSCMFaceEdge temp = mEdge[id2];
	mEdge[id2] = mEdge[id1];
	mEdge[id1] = temp;
}

bool LSCMFace::containsVert(int vert)
{
	for (int i = 0; i<3; i++)
	{
		if (mEdge[i].mTVVert == vert) return true;
	}

	return false;
}

int LSCMFace::getFaceTVVert(int index)
{
	//return md->GetFaceTVVert(mPolyIndex, mEdge[index].mIthEdge);
	return mEdge[index].mTVVert;
}

float LSCMFace::getFaceGeoAngle(int index)
{
	return mEdge[index].mGeoAngle;
}

int LSCMFace::findVertFaceIndex(int vertTVIndex)
{
	for (int k = 0; k<3; k++)
	{
		if (mEdge[k].mTVVert == vertTVIndex)
		{
			return k;
		}
	}

	return -1;
}

// Used for Hierachical Peel /////////////////////////////////////////////////////
HLSCMFace::HLSCMFace()
{
	mVerts[0].mIthVert = -1;
	mVerts[1].mIthVert = -1;
	mVerts[2].mIthVert = -1;

	mVerts[0].mGeoAngle = 0.0f;
	mVerts[1].mGeoAngle = 0.0f;
	mVerts[2].mGeoAngle = 0.0f;

	mVerts[0].mPinned = false;
	mVerts[1].mPinned = false;
	mVerts[2].mPinned = false;

	exist = true;
	orderUpdated = false;
}

inline bool IsAngleValid(float geoAngle)
{
	if (geoAngle > DegToRad(2.5) && geoAngle < DegToRad(177.5)) return true;
	return false;
}

void HLSCMFace::initValueFromOldFace(HLSCMFace *face, HalfEdge &halfEdge, IMeshTopoData *md)
{
	int startEdgeIndex = -1;
	int endEdgeIndex = -1;
	for (int j = 0; j<3; j++)
	{
		if (face->mVerts[j].mTVVert == halfEdge.startIndex) startEdgeIndex = j;
		if (face->mVerts[j].mTVVert == halfEdge.endIndex) endEdgeIndex = j;
	}
	if (startEdgeIndex == -1 || endEdgeIndex != -1)
	{
		//input invalid
		return;
	}

	// verts
	// 	int mIthVert;
	// 	int mGeoVert;
	// 	int mTVVert;
	// 	float mGeoAngle;
	// 	float mSin;
	// 	bool mPinned; --------just ignore pin here, move all the pin point later!!!!!!
	int aIndex = -1, bIndex = -1;
	if (startEdgeIndex == 0)
	{
		aIndex = 1;	bIndex = 2;
	}
	else
	{
		aIndex = 0;
		bIndex = 3 - startEdgeIndex;
	}

	// copy old verts
	mVerts[aIndex].mIthVert = face->mVerts[aIndex].mIthVert;
	mVerts[aIndex].mGeoVert = face->mVerts[aIndex].mGeoVert;
	mVerts[aIndex].mTVVert = face->mVerts[aIndex].mTVVert;
	mVerts[bIndex].mIthVert = face->mVerts[bIndex].mIthVert;
	mVerts[bIndex].mGeoVert = face->mVerts[bIndex].mGeoVert;
	mVerts[bIndex].mTVVert = face->mVerts[bIndex].mTVVert;

	// set new vert
	mVerts[startEdgeIndex].mIthVert = face->mVerts[startEdgeIndex].mIthVert;
	mVerts[startEdgeIndex].mGeoVert = halfEdge.endGeoIndex;
	mVerts[startEdgeIndex].mTVVert = halfEdge.endIndex;

	// calculate angles, the angles must be valid, because it has been calculated before
	Point3 endPt = md->GetGeomVert(halfEdge.endGeoIndex);
	Point3 aPt = md->GetGeomVert(mVerts[aIndex].mGeoVert);
	Point3 bPt = md->GetGeomVert(mVerts[bIndex].mGeoVert);

	Point3 vec[3];
	vec[0] = Normalize(aPt - endPt);
	vec[1] = Normalize(bPt - aPt);
	vec[2] = Normalize(endPt - bPt);

	mVerts[startEdgeIndex].mGeoAngle = fabs(md->AngleFromVectors(vec[0], vec[2] * -1.0f));
	mVerts[aIndex].mGeoAngle = fabs(md->AngleFromVectors(vec[1], vec[0] * -1.0f));
	mVerts[bIndex].mGeoAngle = fabs(md->AngleFromVectors(vec[2], vec[1] * -1.0f));

	mVerts[startEdgeIndex].mSin = sin(mVerts[startEdgeIndex].mGeoAngle);
	mVerts[aIndex].mSin = sin(mVerts[aIndex].mGeoAngle);
	mVerts[bIndex].mSin = sin(mVerts[bIndex].mGeoAngle);

	// should also update edge list, update it outside of this function

	// area, norm
	// 	Point3 crossProd = CrossProd(vec[0], vec[2]);
	// 	area = 0.5*FLength(crossProd);
	// 	if (area < 1e-5) area = 0.0;
	// 	norm = crossProd.FNormalize();
}

bool HLSCMFace::findVertsExceptStart(int start, int &vertA, int &vertB, int &vertGeoA, int &vertGeoB)
{
	int index = -1;
	for (int i = 0; i<3; i++)
	{
		if (mVerts[i].mTVVert == start)
		{
			index = i;
			break;
		}
	}

	if (index == -1) return false;

	vertA = mVerts[(index + 1) % 3].mTVVert;
	vertB = mVerts[(index + 2) % 3].mTVVert;
	vertGeoA = mVerts[(index + 1) % 3].mGeoVert;
	vertGeoB = mVerts[(index + 2) % 3].mGeoVert;
	return true;
}

bool HLSCMFace::insideVerts(std::vector<int> &verts)
{
	bool find1 = false, find2 = false, find3 = false;
	int v1 = mVerts[0].mTVVert;
	int v2 = mVerts[1].mTVVert;
	int v3 = mVerts[2].mTVVert;
	for (int i = 0; i<(int)verts.size(); i++)
	{
		if (verts[i] == v1) find1 = true;
		if (verts[i] == v2) find2 = true;
		if (verts[i] == v3) find3 = true;
	}

	if (find1 && find2 && find3) return true;
	return false;
}

bool HLSCMFace::containsVert(int vert)
{
	for (int i = 0; i<3; i++)
	{
		if (mVerts[i].mTVVert == vert) return true;
	}

	return false;
}

void HLSCMFace::addEdges(int edgeIndex0, int edgeIndex1, int edgeIndex2)
{
	edgeIndices[0] = edgeIndex0;
	edgeIndices[1] = edgeIndex1;
	edgeIndices[2] = edgeIndex2;
}

float HLSCMFace::UVAreaSigned(IMeshTopoData *md)
{
	Point3 uv1 = md->GetTVVert(mVerts[0].mTVVert);
	Point3 uv2 = md->GetTVVert(mVerts[1].mTVVert);
	Point3 uv3 = md->GetTVVert(mVerts[2].mTVVert);

	return 0.5f * (((uv2.x - uv1.x) * (uv3.y - uv1.y)) -
		((uv3.x - uv1.x) * (uv2.y - uv1.y)));
}

int HLSCMFace::findVertFaceIndex(int vertTVIndex)
{
	for (int k = 0; k<3; k++)
	{
		if (mVerts[k].mTVVert == vertTVIndex)
		{
			return k;
		}
	}

	return -1;
}

void HLSCMFace::swapFaceVert(int id1, int id2)
{
	HLSCMFaceVert temp = mVerts[id2];
	mVerts[id2] = mVerts[id1];
	mVerts[id1] = temp;
}

int HLSCMFace::getFaceTVVert(int index)
{
	return mVerts[index].mTVVert;
}

float HLSCMFace::getFaceGeoAngle(int index)
{
	return mVerts[index].mGeoAngle;
}

bool HLSCMFace::containsHalfEdge(HalfEdge &halfEdge)
{
	// if current face contains the half edge, continue
	int startEdgeIndex = -1;
	int endEdgeIndex = -1;
	for (int j = 0; j<3; j++)
	{
		if (mVerts[j].mTVVert == halfEdge.startIndex) startEdgeIndex = j;
		if (mVerts[j].mTVVert == halfEdge.endIndex) endEdgeIndex = j;
	}
	if (startEdgeIndex != -1 && endEdgeIndex != -1)
	{
		// current face contains the half edge, ignore this face
		return true;
	}

	return false;
}


bool HalfEdgeSet::addHalfEdge(HalfEdge &halfEdge)
{
	for (int i = 0; i<(int)edgelist.size(); i++)
	{
		if (edgelist[i].startIndex == halfEdge.startIndex
			&& edgelist[i].endIndex == halfEdge.endIndex)
		{
			return false;
		}
	}

	edgelist.push_back(halfEdge);
	return true;
}

int HalfEdgeSet::count()
{
	return (int)edgelist.size();
}

int HalfEdgeSet::minEdge()
{
	int ct = (int)edgelist.size();
	if (ct == 0) return -1;

	float minCost = FLT_MAX;
	int minIndex = -1;
	for (int i = 0; i<ct; i++)
	{
		if (!edgelist[i].collpased && (edgelist[i].cost < minCost))
		{
			minIndex = i;
			minCost = edgelist[i].cost;
		}
	}

	return minIndex;
}

void HalfEdgeSet::removeEdgeOfVertex(int vIndex)
{
	// may be change it to a list later! this is quite inefficient
	for (auto it = edgelist.begin(); it != edgelist.end(); )
	{
		if (it->startIndex == vIndex)
		{
			it = edgelist.erase(it);
		}
		else
		{
			++it;
		}
	}
}

bool HalfEdgeSet::popLastCollapsedEdge(HalfEdge &edge)
{
	if (edgelist.empty()) return false;
	if (lastCollapseIndex == -2)
	{
		// first time pop
		lastCollapseIndex = (int)edgelist.size() - 1;
	}
	else if (lastCollapseIndex < 0)
	{
		return false;
	}

	edge = edgelist[lastCollapseIndex];
	lastCollapseIndex--;

	return true;
}

void HalfEdgeSet::clear()
{
	edgelist.clear();
}