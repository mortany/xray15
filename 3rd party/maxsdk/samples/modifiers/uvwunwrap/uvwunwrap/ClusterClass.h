/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#pragma once
#include "tab.h"
#include "point3.h"
#include "bitarray.h"
#include "box3.h"
#include <vector>
#include "IClusterInternal.h"

//This is an edge class for the TV Face Data
//It is pretty simplistic but may grow as needed to support more advance tools
class BorderClass
{
public:
	int innerFace, outerFace;	//Index into the face list of the left and right faces that touch this edge
								//This only support edges with 2 faces

	int edge;					//Index into the edge list for the mesh/patch/polyobj
};


//Just a simple rect class to hold rects for sub clusters
class SubClusterClass
{
public:
	float x, y;
	float w, h;

	SubClusterClass()
	{
		this->x = 0.0f;
		this->y = 0.0f;
		this->w = 0.0f;
		this->h = 0.0f;
	}

	SubClusterClass(float x, float y, float w, float h)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
};

class MeshTopoData;

//Class that contains data on a cluster.  A cluster is just a group of TV faces.  These do not have 
//to be contiguous 
class ClusterClass : public IClusterInternal
{
public:
	ClusterClass();
	~ClusterClass();
	MeshTopoData *ld;				//this is the local data that owns these clusters
	Tab<int> faces;					//List of face indices that this cluster own into TVMaps.f
	Tab<int> verts;					//list of vertex indices for this cluster
	Tab<BorderClass> borderData;	//List of border data for this cluster.  This is computed in Unwrap::BuildCluster
	Point3 normal;					//This is the geometric normal for this cluster
	Point3 offset;					//this is the min corner of the bounding box for the cluster
	float w, h;						//Width and Height of the cluster
	Box3 bounds;					//The bounding box for the cluster

	float newX, newY;				//Temp data to store offsets
	float surfaceArea;				//Surface area of the faces for the cluster

									//this just though cluster building a bounding box for each face that is in this cluster
									//UVW_ChannelClass &TVMaps needs to be passed in to get the vertex position data
	void BuildList();

	Tab<Box3> boundsList;			//this is the result of the BuildList method.  
									//It is a list of bounding boxes for each face

									//This determines whether the rect passed in intersects with any of the
									//bounds in boundsList.  Basically it deteremines if a quad intersects
									//any of the faces in this cluster based on bounding boxes
	BOOL DoesIntersect(float x, float y, float w, float h);

	//this function finds a squre bounding box that fits in the clusters open space
	void	FindSquareX(int x, int y,									//the center of the square to start looking from
		int w, int h, BitArray &used,					//the bitmap and width and height of the bitmap
														//used must be set to the right size and will be filled
		int &iretX, int &iretY, int &iretW, int &iretH); //the lower right of the square it is width and height that fits 

														 //this function finds a rect bounding box that fits in the clusters open space
	void	FindRectDiag(int x, int y,				//the starting point to start lookng from
		int w, int h,				//the width and height of the bitmap used grid
		int dirX, int dirY,			//the direction opf travel to progress through the map
									//needs to be 1 or -1
		BitArray &used,				// the map used list
		int &iretX, int &iretY, int &iretW, int &iretH);  //the bounding box return

														  //this function goes through the boundsList and finds all the open spaces
	int ComputeOpenSpaces(float spacing, BOOL onlyCenter = FALSE);

	Tab<SubClusterClass> openSpaces; // this is a list of bounding box open spaces in this cluster
									 // This is computed in ComputeOpenSpaces

	Tab<Point2> horizonValues; // this describes the horizon border of the cluster
							   // values of points is scanned using static steps, point.x represents the highest value, point.y represent the lowest value

	Tab<Point2> verticalValues; // this describes the vertical border of the cluster
								// values of points is scanned using static steps, point.x represents the highest value, point.y represent the lowest value

	bool pasted;	// used during packing process, indicate whether this cluster is pasted

	ClusterNewPackData* pNewPackData;
	// from IClusterInternal
	virtual IMeshTopoData* GetOwnerMeshTopoData();
	virtual Tab<int>& GetFaces() { return faces; }
	virtual Tab<int>& GetVerts() { return verts; }
	virtual float GetWidth() { return w; }
	virtual float GetHeight() { return h; }
	virtual void SetWidth(float nw) { w = nw; }
	virtual void SetHeight(float nh) { h = nh; }
	virtual float GetNewX() { return newX; }
	virtual float GetNewY() { return newY; }
	virtual void SetNewX(float x) { newX = x; }
	virtual void SetNewY(float y) { newY = y; }
	virtual Point3 GetOffset() { return offset; }
	virtual void SetOffset(const Point3& pt3) { offset = pt3; }
	virtual Tab<Point2>& GetHorizontalValues() { return horizonValues; }
	virtual Tab<Point2>& GetVerticalValues() { return verticalValues; }
	virtual Box3& GetBounds() { return bounds; }

	// NB: Because rotation is applied only to the vertices blonging to this cluster, for any particular rotation angle with index
    //     i, pNewPackData->rotatedPoints[i] is sized by verts.Count(), so that vertex pNewPackData->rotatedPoints[i][v]
    //     corresponds to vertex verts[v]. This follows from the setup in function PreparePackingDataForClusters.
	virtual ClusterNewPackData& GetNewPackData() { return *pNewPackData; }
	virtual bool GetPasted() { return pasted; }
	virtual void SetPasted(bool b) { pasted = b; }
	virtual float GetSurfaceArea() { return surfaceArea; }
};

//Just another rect class but this one contains an area
class OpenAreaList
{
public:
	float x, y;
	float w, h;
	float area;
	OpenAreaList() : x(0.0f), y(0.0f), w(0.0f), h(0.0f), area(0.0f) {}
	OpenAreaList(float x, float y, float w, float h)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
		area = w*h;
	}
};