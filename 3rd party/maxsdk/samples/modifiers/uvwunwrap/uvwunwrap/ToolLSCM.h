
/*

Copyright [2010] Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement 
provided at the time of installation or download, or which otherwise accompanies 
this software in either electronic or hard copy form.   

*/



#pragma once

#include "3dsmaxport.h"

#include "TVData.h"

#include "../UvwunwrapBase/SuperLU_SRC/slu_sdefs.h"
#include "../UvwunwrapBase/SuperLU_SRC/slu_util.h"
#include <queue>
#include "../UvwunwrapBase/LSCMData.h"
#include "IClusterInternal.h"
#include "MeshTopoData.h"

//this contains all our data we need to solve for a cluster/element
class LSCMClusterData : public ILSCMClusterData
{
public:
	
	LSCMClusterData();
	~LSCMClusterData();

	// Sets the face count to zero
	void Reset();

	//adds a polygon to this cluster
	void Add(int polyIndex, MeshTopoData *md);

	//adds a default mapping just copies the XYZ data into uv space
	void ApplyMapping(MeshTopoData *md, UnwrapMod *mod);

	//adds the pins, either user defined or computed internally.  We need at least 2 pinned verts
	//to solve the equations
	void ComputePins(MeshTopoData *md, UnwrapMod *mod, bool useExistingMapping);

	//if the solver created any temporary pins this will unmark them
	void UnmarkTempPins(MeshTopoData *md);

	//this just fills out the angles which is needed for the solve
	void ComputeFaceAngles(MeshTopoData *md);

	//this fills out all our sparse matrices
	void ComputeMatrices(MeshTopoData *md, bool useSimplifyModel);

	// does all out math to actually solve the equations
	void Solve(MeshTopoData *md);

	//this copies the UVWs computed back into the modifier
	void SendBackUVWs(UnwrapMod *mod, MeshTopoData *md, bool useSimplifyModel);

	void OutputMatrixToFile(FastSparseMatrix &m1, FastSparseMatrix &m2);



	//returns the number faces in this cluster
	int		NumberFaces();
	//Gets a particular face
	LSCMFace *GetFace(int index);

	//send a bitarray of changed tvverts to check if this cluster needs to be changed
	void Invalidate(BitArray changedTVVerts,MeshTopoData *md);

	//forces the system to invalidate
	void Invalidate();

	int TempPinCount();

	//Normalize the quick peeled result into the range [0.0,1.0]
	void NormalizeQuickPeeledResult(UnwrapMod *mod, MeshTopoData *md);

	// from ILSCMClusterData
	virtual bool IsValidGeom() { return mValidGeom; }
	virtual bool IsResolveRequired() { return mNeedToResolve; }
	virtual Tab<LSCMFace>& GetFaceData() { return mFaceData; }
	virtual ABFPeelData& GetABFPeelData() { return *pABFData; }
	virtual void PreparePinDataForLSCM(Tab<LSCMFace> &faceData, IMeshTopoData * md);
	virtual BitArray& GetClusterVerts() { return mClusterVerts; }
		
private:
	//converts a TV index into an index into the sparse matrix
	int ComputeMatrixIndex(const Tab<LSCMFace> &faceData, int faceIndex, int ithEdge, int tvIndex);

	//Takes our temporary pin and nonpinned entries and fills them out in the matrices
	void FillSparseMatrix();

	//Adds our entries into the temp tables
	//if index < 0 then this is a pin vert and added to the pin list
	//otherwise it goes in the nonpinned list
	void AddToTempTable( int index, float val, float uv); 

	//Used for computing pins
	void FindGeoVertsByTV( MeshTopoData * md, BitArray mapFaces, int pVert1, int &pVertGeo1, int pVert2, int &pVertGeo2 );

	//Find existing pin verts and store result into mPinVerts
	void FindExistPinVerts( BitArray mapFaces, MeshTopoData * md );

	void OptimizeTVertShareGeoVert( MeshTopoData * md, BitArray mapFaces, int pVertGeo1, int pVert1, int &pVert2, int &pVertGeo2, Box3 &potentialBounds, Box3 &tvBounds, UnwrapMod * mod );

	//void PreparePinDataForLSCM( Tab<LSCMFace> &faceData, MeshTopoData * md );

	void CalculatePinPos2D( MeshTopoData * md, const int pVert1, const  int pVert2, const  int pVertGeo1, const  int pVertGeo2, const  TimeValue t);

	//just checks to see if the cluster is flat so we can optimize the pin creation
	bool IsFlat() const;

	//just a simple pin computation for meshes that are flat
	void ComputeSimplePin(const TimeValue t,  MeshTopoData *md,  const  bool useExistingMapping);


	//Used for ABF, find internal vertex
	bool			mValidGeom;
	Tab<LSCMFace>	mFaceData;				// list of faces for this cluster

	BitArray		mClusterVerts;			// bitarray of all the tvverts in the cluster
	Tab<int>		mPinVerts;				// a tab of all the pin verts for this clust
	Tab<int>		mTempPins;				// a list of temp pins, these are only valid during the solve
	Tab<int>		mTVIndexToMatrixIndex;	// an array to remap vertex indices into indices into the sparse matrix


	//per face temporary tabs to store pinned and non pinned entries
	Tab<TempData>		mNonPinnedVertsList;
	Tab<TempData>		mPinnedVertsList;

	
	bool mIsFlat = false;

	int					mNumberCoefficients;
	int					mUserDefinedPinVertsCount;

	// A*x = b
	// A is our sparse matrix 2* non pin tvvertices in this cluster x 2* non pin tvvertices in this cluster filled with face angles at those verts
	// x is our results
	// b is based on face angles and pinned verts
	LSCMMatrix			mAMatrix;
	Tab<float>			mXMatrix;
	Tab<float>			mBMatrix;

	//these are our actual super lu variables
	SuperLUStat_t		mSuperLuStats;
	SuperMatrix			mL;
	SuperMatrix			mU;
	bool				mSuperLuLandUAllocated;

	Tab<int>			mPermR;
	Tab<int>			mPermC;

	bool		mValidSolve;    //just a flag that is set if there was a valid solve, if set to 0 there was no solve 
	bool		mNeedToResolve;  //if this is set the data has changed and the this cluster needs to be resolved
	Point3		mLscmMove;		//the whole cluster should move according to this vector after LSCM solver completed

	//------Data for Angle based flattening------------------ //
	// Remember this is just the original minimization equations
	ABFPeelData* pABFData;
};

//this represents a local data for a modifier
class LSCMLocalData
{
public:
	// When the local data is added the faces are split up into contiguous clusters.  If useExistingMapping
	// is set the clusters are based on the mapping clusters, otherwise the geom clusters are used
	// useSelectedFaces will only split the selected faces
	LSCMLocalData(UnwrapMod *mod, MeshTopoData* md, bool useExistingMapping, bool useSelectedFaces);
	~LSCMLocalData();

	//Helper method to return the local data assigned to this cluster
	MeshTopoData* GetLocalData();

	// Releases all the data
	void Free();

	//adds a cluster, faces is the list of polygons to add ass a cluster
	void AddCluster(Tab<int> &polygons);

	//build all clusters
	void BuildClusterData();

	//this just adds some default mapping to the clusters
	void ApplyMapping();

	//this just computes the pin vertices for all our clusters if none exist
	void ComputePins();
	//computes the angles of all our face corners
	void ComputeFaceAngles();

	//this builds our spares matrices
	void ComputeMatrices();

	//this solves the matrices
	
	void Solve();

	//this copies the UVWs computed back into the modifier
	void SendBackUVWs();

	//resolves this cluster based on the invalidation flags
	bool Resolve(bool allClusters);

	//computes angles using LABF 
	void ComputeLABFAngles();

	//compute angles using ABF++
	void ComputeABFPlusPlusAngles();

	//Invalidate :  a pin has moved.
	void InvalidatePin(int index);
	//Invalidate :  a pin has been added or deleted.
	void InvalidatePinAddDelete(int index);
	//Invalidate : the UVW topology as changed
	void InvalidateTopo();

	//returns the amount of time required for the last solve
	// if this return 0 the solve was skipped
	double GetSolveTime(); 

	void RescaleClusters();

	int NumberClusterData();

	LSCMClusterData* GetClusterData(int iIndex);

	//this create a contiguous face cluster form the geom data
	// startFace is where to start building the cluster from
	// usedFaces are faces that you don't want added to the clusters
	// faces is where the results are stored
	static void GetGeomCluster(UnwrapMod *mod, MeshTopoData* ld, int &startFace, BitArray &usedFaces, Tab<int> &faces);
	//same as above but uses the texture data
	static void GetTVCluster(UnwrapMod *mod, MeshTopoData* ld, int &startFace, BitArray &usedFaces, Tab<int> &faces);

	//Normalize all quick peeled result into the range [0.0,1.0]
	void NormalizeQuickPeeledResult();
private:
	//implementation of GetGeomCluster() and GetTVCluster()
	static void _GetCluster(UnwrapMod *mod, MeshTopoData* ld, int &startFace, BitArray &usedFaces, Tab<int> &faces, bool isTVSpace);

	//when cutting the edge that will automatically change to the seam, only the related cluster will be created,  
	//no need consider the used faces.
	void PreprocessUsedFaces();

	//just pointers to our modifier and local data for this cluster
	UnwrapMod *mMod;
	MeshTopoData* mMd;

	//these are the faces that are to not be preprocessed
	BitArray	mUsedFaces;

	//this is a list of all our clusters
	Tab<LSCMClusterData*> mClusterData;

	//after a solve the texture verts are stored here
	Tab<Point3> mOutputTVVerts;

	//determines if this cluster will create new mapping or use the existing mapping
	bool mUseExistingMapping;

	//whether use Angle Based Flattening
	bool mUseAbf;

//out invalidation data
	BitArray	mMovedPin;		//bit array of pins that have been moved
	BitArray	mChangedPin;	//bit array of pins that have been added/deleted
	bool		mTopoChange;	//flag is the UVW topology has changed

	double mTimeToSolve;		//the time to solve all the cluster
};

// this is our solver
class ToolLSCM
{
public:
	ToolLSCM();
	~ToolLSCM();

	//called when you want to start a solve
	bool Start(bool useExistingMapping, UnwrapMod *mod, Tab<MeshTopoData*> &localData);
	
	//solves, needs to be called when when ever the topology/geometry changes
	// hasToSolve means that the system has to solve, if set to false and the solver thinks it is going
	//		take a long time it will skip.
	// sendNotify if true will cause a notifydep message and invalidate view to be sent
	// allClusters if all clusters should be solved or only the selected clusters should be solved
	bool Solve(bool hasToSolve = true, bool sendNotify = true, bool allClusters = true);

	//cleans up all our data
	bool End();

	//Invalidate :  a pin has moved in this local data.
	void InvalidatePin(MeshTopoData *md, int index);
	//Invalidate :  a pin has beend added/deleted in this local data.
	void InvalidatePinAddDelete(MeshTopoData *md, int index);
	//Invalidate :  a UVW topology changed in this local data.
	void InvalidateTopo(MeshTopoData *md);
	//Get the local data for accessing the face data such as under the drawing circumstance
	LSCMLocalData* GetData(MeshTopoData *md);
private:
	void ApplyMapping(bool useSelection,  Tab<MeshTopoData*> &localData,UnwrapMod *mod);
	void CutSeams(Tab<MeshTopoData*> &localData, UnwrapMod *mod);
	void Pack();
	void PackNormalize(bool isAutoPack); // Pack Normalized/Fill holes

	void Free();

	void NonSquareAdjustVertexY(bool bInverse);

	Tab<LSCMLocalData*> mLocalData;
	
	UnwrapMod *mMod;

	double mTimeOverAll;
	double mTimeCut;
	double mTimeClusterCreation;
	
	double mTimePin;
	double mTimeMatrixConstruction;
	double mTimeSolve;
};

class InteractiveStartRestoreObj : public RestoreObj 
{


public:
	UnwrapMod *mMod;


	InteractiveStartRestoreObj(UnwrapMod *m); 
	void Restore(int isUndo);
	void Redo();
	void EndHold();
	TSTR Description();


};
