/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#pragma once

#include "UnwrapBaseExport.h"
#include <map>
#include <vector>
#include "point3.h"
#include "tab.h"
#include "../uvwunwrap/IMeshTopoDataContainer.h"

 typedef std::map<int, float> SparseData;
#pragma warning( push )  
#pragma warning( disable: 4251 ) //disable C4251 that template class needs to have dll-interface to be used
//represents a row in our sparse matrix
class UNWRAP_BASE_EXPORT LSCMRow
{
public:
	SparseData mData;  //the data in the row
};


//this is our sparse matrix to hold all our coefficients its size is number of non pinned tv vert x 2
//this is a square matrix (nxn)
class  UNWRAP_BASE_EXPORT LSCMMatrix
{
public:
	LSCMMatrix();
	virtual ~LSCMMatrix();

	//returns the size of the matrix
	int Size();
	//sets the size of the matrix
	void SetSize(int size);

	//frees all the data held by this matrix
	void Free();

	//returns a specific row, if out of range returns NULL
	LSCMRow*  GetRow(int index);
private:
	Tab<LSCMRow*> mRow;
};

class UNWRAP_BASE_EXPORT FastSparseMatrix
{
public:
	FastSparseMatrix() :numEntries(0), numRows(0), numCols(0) {}
	FastSparseMatrix(LSCMMatrix &matrix, int rowLength, int colLength);
	virtual ~FastSparseMatrix() {};

	void getTransform(FastSparseMatrix &Tmat);

	Tab<int> rowCounter;
	Tab<float> val;
	Tab<int> valueIndex;

	int numEntries, numRows, numCols;
};

//just a temp data class to store data for a matrix entry
class UNWRAP_BASE_EXPORT TempData
{
public:
	int mIndex;		//index into the  sparse matrix
	float mValue;	//the coefficient
};


//describes an edge of a face
class UNWRAP_BASE_EXPORT LSCMFaceEdge
{
public:
	int mIthEdge;		// the index into the face vertex array
	float mGeoAngle;	// the angles at this vertex
	float mSin;			// the sin  at this vertex
	bool  mPinned;		// whether this corner is pinned (pinned vertices do not move when solved)
	int mTVVert;		// index of corresponding vert on tv map
	int mGeoVert;		// index of corresponding vert on 3d model
};

class HLSCMFace;
class UNWRAP_BASE_EXPORT LSCMFace
{
public:
	LSCMFace();
	LSCMFace(HLSCMFace &hFace);
	int mPolyIndex;			// the index of the polygon that owns this face
	LSCMFaceEdge mEdge[3];		// the edge description for this face

	virtual void swapFaceVert(int id1, int id2);
	virtual bool containsVert(int vert);
	virtual int getFaceTVVert(int index);
	virtual float getFaceGeoAngle(int index);
	virtual int findVertFaceIndex(int vertTVIndex);

	bool orderUpdated;
};

class UNWRAP_BASE_EXPORT HLSCMFaceVert
{
public:
	int mIthVert;
	int mGeoVert;
	int mTVVert;
	float mGeoAngle;
	float mSin;
	bool mPinned;
};

class UNWRAP_BASE_EXPORT HalfEdge
{
public:
	HalfEdge() { collpased = false; }
	HalfEdge(int index1, int index2) { startIndex = index1; endIndex = index2; collpased = false; }
	~HalfEdge() {}

	bool operator < (const HalfEdge h2) const
	{
		return cost > h2.cost;
	}

	float cost;
	int startIndex, endIndex;	// vertex index in tv map
	int startGeoIndex, endGeoIndex;	// vertex index in 3d model
	Point3 geoLine;
	bool collpased;
};

class UNWRAP_BASE_EXPORT HLSCMFace
{
public:
	HLSCMFace();
	~HLSCMFace() {}

	// cost is calculated on 3D model
	bool containsHalfEdge(HalfEdge &halfEdge);
	void initValueFromOldFace(HLSCMFace *face, HalfEdge &halfEdge, IMeshTopoData *md);
	bool findVertsExceptStart(int start, int &vertA, int &vertB, int &vertGeoA, int &vertGeoB);
	bool insideVerts(std::vector<int> &verts);
	bool containsVert(int vert);
	void addEdges(int edgeIndex0, int edgeIndex1, int edgeIndex2);
	float UVAreaSigned(IMeshTopoData *md);
	void swapFaceVert(int id1, int id2);

	int getFaceTVVert(int index);
	float getFaceGeoAngle(int index);
	int findVertFaceIndex(int vertTVIndex);

	bool exist;
	bool orderUpdated;
	HLSCMFaceVert mVerts[3];	// the vertices of this face
	int edgeIndices[3];			// edge index in the LoopAbfEdgeList
};

class UNWRAP_BASE_EXPORT HalfEdgeSet
{
public:
	HalfEdgeSet() { lastCollapseIndex = -2; }
	~HalfEdgeSet() {}

	bool addHalfEdge(HalfEdge &halfEdge);
	//void addFaceEdges(HLSCMFace &face, MeshTopoData *md);
	//void updateEdgeCost( LoopAbfVertList &vertList, Tab<HLSCMFace> &hlsmFaces, MeshTopoData * md );
	int count();
	int minEdge();	// return index of the half edge with the minimum cost which has not been collapsed
	void removeEdgeOfVertex(int vIndex);
	bool popLastCollapsedEdge(HalfEdge &edge);
	void clear();

	std::vector< HalfEdge > edgelist; // maybe change it to linkedList???
	int lastCollapseIndex;
};

#pragma warning( pop )