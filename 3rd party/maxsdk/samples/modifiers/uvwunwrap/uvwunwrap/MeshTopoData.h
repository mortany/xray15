
//**************************************************************************/
// Copyright (c) 1998-2006 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: This contains our local data classes
// AUTHOR: Peter Watje
// DATE: 2006/10/07 
//***************************************************************************/

#pragma once

#include "object.h"
#include "peltData.h"
#include "namesel.h"

#include "ToolGroupingData.h"
#include "SingleWeakRefMaker.h"
#include "IUnwrapMax8.h"
#include "ITopoChangedListener.h"
#include <cmath>
#include <vector>
#include <memory>
#include <atomic>
#include "IMeshTopoDataContainer.h"

class ClusterClass;
class ToolLSCM;
class MirrorDataHelper;
class LSCMFace;

//we use geom selection to hold since it is stactic
class HoldGeomSelection
{
public:
	BitArray mFace;
	BitArray mEdge;  
	BitArray mVert;
};

class MeshPolyData: public MaxHeapOperators
{
public:
	std::vector<int> mFaceList;// the face list which belong to this poly.
	MeshPolyData()
	{}
	virtual ~MeshPolyData()
	{}
};

class GeomSelMirrorData : public MaxHeapOperators
{
public:
	int index; //index of the mirror geom sub level obj based on the current mirror settings.
	//int flags;
	GeomSelMirrorData() : index(-1)
	{}

	virtual ~GeomSelMirrorData()
	{}
};

//Connected faces on a vertex
class FacesAtVert : public MaxHeapOperators
{
public:
	FacesAtVert() {}
	virtual ~FacesAtVert() {}

public:
	Tab<int> mData;
};

//Connected faces on vertices
class FacesAtVerts : public MaxHeapOperators
{
public:
	FacesAtVerts() {}
	virtual ~FacesAtVerts()
	{
		for (int i = 0; i < mData.Count(); ++i)
			delete mData[i];
	}

	Tab<FacesAtVert*> mData;
};

class MeshTopoData : public LocalModData ,public ITopoChangedListener, public IMeshTopoData
{
	friend class ConstantTopoAccelerator;
public:
	enum GroupBy {kFaceAngle = 0, kSmoothingGroup = 1, kMaterialID = 2 };
	
	MeshTopoData(ObjectState *os, int mapChannel);
	MeshTopoData();
	~MeshTopoData();
	LocalModData *Clone();

	//these fucntion let you get at the cached geo object
	//returns our cached mesh
	virtual Mesh *GetMesh() {return mesh;}
	//returns our cached mnmesh
	virtual MNMesh *GetMNMesh() {return mnMesh;}
	//returns our cached patch
	virtual PatchMesh *GetPatch() {return patch;}

	//this free our geo data cache
	void FreeCache();

	//this sets the cache based on the object passed in
	void SetGeoCache(ObjectState *os);

	//this just makes sure to check the cache against the incoming mesh
	//it checks for topological and type changes and return whether or not it has detected a change
	BOOL HasCacheChanged(ObjectState *os);

	//this reset the caches and rebuilds 
	void Reset(ObjectState *os, int mapChannel);

	//this updates any geo data from the object in the stack into our cache
	bool UpdateGeoData(TimeValue t,ObjectState *os);

	//this copy the face selection in our cache into the stack object 
	void CopyMatID(ObjectState *os, TimeValue t);

	//this applies the mapping in the cache to the pipe object
	void ApplyMapping(ObjectState *os, int mapChannel, TimeValue t);

	// Apply mesh selection level in order to display sub object correctly
	void ApplyMeshSelectionLevel(ObjectState *os);

	//this copy the face selection in our cache into the stack object for display purposes
	void CopyFaceSelection(ObjectState *os);
	void ResetFaceSelChanged() {faceSelChanged = FALSE;}
	BOOL GetFaceSelChanged(){ return faceSelChanged;}

	//this gets/sets whether the incoming pipe object is set to face selection
	BOOL HasIncomingFaceSelection();
	BitArray GetIncomingFaceSelection();

	//these are just load save function for the various components
	ULONG SaveTVVertSelection(ISave *isave);
	ULONG SaveGeoVertSelection(ISave *isave);
	ULONG SaveTVEdgeSelection(ISave *isave);
	ULONG SaveGeoEdgeSelection(ISave *isave);
	ULONG SaveFaceSelection(ISave *isave);

	ULONG LoadTVVertSelection(ILoad *iload);
	ULONG LoadGeoVertSelection(ILoad *iload);
	ULONG LoadTVEdgeSelection(ILoad *iload);
	ULONG LoadGeoEdgeSelection(ILoad *iload);
	ULONG LoadFaceSelection(ILoad *iload);

	ULONG SaveFaces(ISave *isave);
	ULONG SaveTVVerts(ISave *isave);
	ULONG SaveGeoVerts(ISave *isave);

	ULONG LoadFaces(ILoad *iload);
	ULONG LoadTVVerts(ILoad *iload);
	ULONG LoadGeoVerts(ILoad *iload);

	//theser are save/load functions for external file support
	int LoadTVVerts(FILE *file);
	void SaveTVVerts(FILE *file);

	void LoadFaces(FILE *file, int ver);
	void SaveFaces(FILE *file);

	//these are some basic selection methods for the tv data
	//mode = TVVERTMODE, TVEDGEMODE, TVFACEMODE
	void ClearSelection(int mode);
	void InvertSelection(int mode);
	void AllSelection(int mode);

	//these are some basic selection preview methods for the tv data
	//mode = TVVERTMODE, TVEDGEMODE, TVFACEMODE
	void ClearSelectionPreview(int mode);
	
	//This just syncs the handle selection with the knot selection
	// dir determines whether to remove the selection when set to 0 or add to the selection when set to 1
	void SelectHandles(int dir);
	//this just does an element select based on the current selection
	//mode is the current subobject mode
	//addSelection determines whether to add to the current selection or remove from the current selection
	void SelectElement(int mode,BOOL addSelection);
	//just a vertex specific version of the above
	void SelectVertexElement(BOOL addSelection, BOOL enableFilterFaceSelection=FALSE);

	//these must be called in pairs
	//this transfer our current selection into vertex selection
	//these are used since alot of operation work on vertices and this a quick way 
	//to do thos operations from edge/face slection by just transferring the selection
	void	TransferSelectionStart(int currentSubObjectMode);
	//this transfer our vertex selection back into our current selection
	//partial if set and recomputeSelection is set will only transfer back the suobjects where all the components vertices are set 
	//for instance in edge if only 1 vertex of the edge was selected that edge would get selected if partial was on
	//recomputeSelection if false the cached selection from the TransferSelectionSatrt is used otherwise the current selection
	//is used to get the new selection
	void	TransferSelectionEnd(int currentSubObjectMode,BOOL partial,BOOL recomputeSelection);

	//Converts the edge selection into a vertex selection set
	void    GetVertSelFromEdge(BitArray &sel);
	//Converts the vertex selection into a edge selection set
	//PartialSelect determines whether all the vertices of a edge need to be selected for that edge to be selected
	void    GetEdgeSelFromVert(BitArray &sel,BOOL partialSelect);
	
	void    GetVertSelFromFace(BitArray &sel);		 //Converts the face selection into a vertex selection set
	void    GetGeomVertSelFromFace(BitArray &sel);	//Converts the face selection into a geom vertex selection set
	//PartialSelect determines whether all the vertices of a face need to be selected for that face to be selected
	void    GetFaceSelFromVert(BitArray &sel,BOOL partialSelect);
	void	GetFaceSelFromEdge(BitArray &sel,BOOL partialSelect);

	//converts the face selection to the edge selection
	void	ConvertFaceToEdgeSel();
	void	ConvertFaceToEdgeSelPreview();
	void	ConvertFaceToEdgeSelection(BOOL bPreview = FALSE);

	//this returns whether a vertex is visible
	BOOL IsTVVertVisible(int index);

	//this returns whether a face is visible
	BOOL IsFaceVisible(int index);

//TV VERTEX METHODS********************************************
	//returns the number texture vertices
	virtual int GetNumberTVVerts();
	//returns a copy of the texture vertices selections
	BitArray GetTVVertSelection();
	//returns a copy of the texture vertices selections preview
	const BitArray& GetTVVertSelectionPreview();
	//returns a ptr to the texture vertices selections
	BitArray *GetTVVertSelectionPtr();
	//returns a ptr to the texture vertices selections preview
	BitArray *GetTVVertSelectionPreviewPtr();
	//copy a new selection into our texture vertices selection
	void SetTVVertSelection(BitArray newSel);
	//copy a new selection preview into our texture vertices selection preview
	void SetTVVertSelectionPreview(const BitArray& newSelPreview);
	const BitArray& GetTVVertSel();
	void SetTVVertSel(const BitArray& newSel);
	void UpdateTVVertSelInIsolationMode(); // update the current TVVert selection based on the "Display only selected" filter.
	//clears all our texture vertex selection
	void ClearTVVertSelection();
	bool ResizeTVVertSelection(int newSize, int save = 0);
	void SetAllTVVertSelection();
	//clears all our texture vertex selection preview
	void ClearTVVertSelectionPreview();
	//returns the UVW position of a texture vertex
	virtual Point3 GetTVVert(int index);

	//this adds a texture vert to our tv list
	//it returns the position that the vertex was inserted at
	int AddTVVert(TimeValue t,Point3 p, Tab<int> *deadVertList = NULL);

	//this adds a point to our tv list and also assigns it to a face vertex 
	// t is the current time in case we need to key the controller
	// p it the point to add
	// faceIndex is the face to add it to 
	// ithVert is the ith vertex of that face
	// mod is the pointer to the unwrap modifie in case we need to key the conreoller
	// sel determines whether that vertex is selected or nor
	// returns the position of the new vertex in the tvert list
	int AddTVVert(TimeValue t, Point3 p, int faceIndex, int ithVert, BOOL sel = FALSE, Tab<int> *deadVertList = NULL);
	//same as above but for a patch handle
	int AddTVHandle(TimeValue t,Point3 p, int faceIndex, int ithVert, BOOL sel = FALSE, Tab<int> *deadVertList = NULL);
	//same as above but for a patch interior
	int AddTVInterior(TimeValue t,Point3 p, int faceIndex, int ithVert, BOOL sel = FALSE, Tab<int> *deadVertList = NULL);

	//this deletes a tvert from the list, it is not actually deleted but just flagged
	void DeleteTVVert(int index);

	//these get and set the tv vert properties
	//this gets/sets the position
	virtual void SetTVVert(TimeValue t, int index, Point3 p);
	//this gets/sets the soft selection influence
	float GetTVVertInfluence(int index);	
	void SetTVVertInfluence(int index, float influ);
	//this gets/sets the selection state
	inline BOOL GetTVVertSelected(int index);
	void SetTVVertSelected(int index, BOOL sel);
	//this gets/sets the selection preview state
	BOOL GetTVVertSelectPreview(int index);
	void SetTVVertSelectPreview(int index, BOOL selPreview);
	
	//this gets/sets the hidden state
	BOOL GetTVVertHidden(int index);
	void SetTVVertHidden(int index, BOOL hidden);
	//this gets/sets the frozen state
	BOOL GetTVVertFrozen(int index);
	void SetTVVertFrozen(int index, BOOL frozen);
	//this gets/sets the controler index, if no control is linked to this vertex it is set to -1
	//the controller list is actually kept in the modifier
	virtual void SetTVVertControlIndex(int index, int id);
	virtual  int GetTVVertControlIndex(int index);
	//this gets/sets the dead flag
	BOOL GetTVVertDead(int index);
	void SetTVVertDead(int index, BOOL dead);
	//this cleans up dead vertices
	void CleanUpDeadVertices();

	virtual BOOL IsTVVertPinned(int index);
	void TVVertPin(int index);
	void TVVertUnpin(int index);

	//this flag means that the system has locked this vertex and it should not be touched, displayed etc.
	BOOL GetTVSystemLock(int index);
	//this gets/sets whether the soft selection influnce has been modified.  If it is modified
	//it wont get recomputed.
	BOOL GetTVVertWeightModified(int index);
	void SetTVVertWeightModified(int index, BOOL modified);
	//this is just a generic flag access
	int GetTVVertFlag(int index);
	void SetTVVertFlag(int index, int flag);

	//this returns the first geo index vert that touches the tvvert
	//this is slow
	int GetTVVertGeoIndex(int index);
	
	//this makes a copy of the UV vertx channel to v
	void CopyTVData(Tab<UVW_TVVertClass> &v);
	//this paste the tv data back from v
	void PasteTVData(Tab<UVW_TVVertClass> &v);

	void CopyFaceData(Tab<UVW_TVFaceClass*> &f);
	//this paste the tv face data back from f
	void PasteFaceData(Tab<UVW_TVFaceClass*> &f);

//GEOM VERTEX METHODS******************************************************
	//gets/sets the number geometric vertices
	int GetNumberGeomVerts();
	void SetNumberGeomVerts(int ct);
	//gets/sets the geomvert position
	virtual Point3 GetGeomVert(int index);
	void SetGeomVert(int index, Point3 p);

	//Geom vert selection methods
	BitArray GetGeomVertSelection();
	BitArray *GetGeomVertSelectionPtr();
	void SetGeomVertSelection(BitArray newSel);
	const BitArray& GetGeomVertSel();
	void SetGeomVertSel(const BitArray& newSel);
	void UpdateGeomVertSelInIsolationMode();
	void ClearGeomVertSelection();
	void SetAllGeomVertSelection();
	BOOL GetGeomVertSelected(int index);
	void SetGeomVertSelected(int index, BOOL sel, BOOL mirrorEnabled = FALSE);

//TV EDGE METHODS***********************************************************
	//returns the number texture edges
	int GetNumberTVEdges();
	//returns the texture vertex index of the start or end vertex of the edge
	//if whichEnd is 0 it returns the start else it returns the end
	int GetTVEdgeVert(int edgeIndex, int whichEnd) const;
	//returns the geom vertex index of the start or end vertex of the edge
	//if whichEnd is 0 it returns the start else it returns the end
	int GetTVEdgeGeomVert(int edgeIndex, int whichEnd);

	//returns the vector handle index if there is none -1 is returned
	int GetTVEdgeVec(int edgeIndex, int whichEnd);
	//returns the number of TV faces connected to this edge
	int GetTVEdgeNumberTVFaces(int edgeIndex);
	//returns the TV face connected to an edge
	int GetTVEdgeConnectedTVFace(int edgeIndex,int ithFaceIndex);
	//return if the edge is hidden
	BOOL GetTVEdgeHidden(int edgeIndex);

	//returns if the TV edge valid flag is set which means the edge data needs to be recomputed
	BOOL IsTVEdgeValid();
	//this sets an invalid flag on the tv edges and the tv edges will be rebuilt on the next
	//evaluation
	void SetTVEdgeInvalid();
	//this sets an invalid flag on the geo edges and the tv edges will be rebuilt on the next
	//evaluation
	void SetGeoEdgeInvalid();

	//this forces an tv edge rebuilt immediately
	void BuildTVEdges();

	//TV Edge selection methods
	const BitArray& GetTVEdgeSel();
	void SetTVEdgeSel(const BitArray& newSel);
	void UpdateTVEdgeSelInIsolationMode();
	BitArray GetTVEdgeSelection();
	BitArray *GetTVEdgeSelectionPtr();
	void SetTVEdgeSelection(BitArray newSel);
	void ClearTVEdgeSelection();
	BOOL GetTVEdgeSelected(int index);
	void SetTVEdgeSelected(int index, BOOL sel);
	inline void SetAllTVEdgeSelection();

	//TV Edge selection preview methods
	const BitArray& GetTVEdgeSelectionPreview();
	BitArray *GetTVEdgeSelectionPreviewPtr();
	void SetTVEdgeSelectionPreview(const BitArray& newSel);
	inline void ClearTVEdgeSelectionPreview();
	BOOL GetTVEdgeSelectedPreview(int index);
	void SetTVEdgeSelectedPreview(int index, BOOL sel);

	//this determines whether a point intersects a tv edge within the threshold
	int EdgeIntersect(Point3 p, float threshold, int i1, int i2);

	//this turns a geometric edge selection into a UV edge selection
	void ConvertGeomEdgeSelectionToTV(BitArray geoSel, BitArray &uvwSel);
	//this turns a UV edge selection into a geometric edge selection
	void ConvertTVEdgeSelectionToGeom(BitArray uvwSel, BitArray &geoSel);
	//get tv edge indices related to geometric edge selection
	//data in GeoTVEdgesMap will be <geoEdgeIndex, <tvEdgeIndex1, tvEdgeIndex2>>
	void GetGeomEdgeRelatedTVEdges(BitArray geoSel, GeoTVEdgesMap &gtvInfo);

//GEOM EDGE METHODS **************************************************************
	//returns the number geometric edges
	int GetNumberGeomEdges();

	//Selection Methods
	const BitArray& GetGeomEdgeSel();
	void SetGeomEdgeSel(const BitArray& newSel, BOOL bMirrorSel = FALSE);
	void UpdateGeomEdgeSelInIsolationMode();
	inline void SetAllGeomEdgeSelection();
	BitArray *GetGeomEdgeSelectionPtr();
	BitArray GetGeomEdgeSelection();
	void SetGeomEdgeSelection(BitArray newSel, BOOL bMirrorSel = FALSE);
	void ClearGeomEdgeSelection();

	inline BOOL GetGeomEdgeSelected(int index)
	{
		if ((index < 0) || (index >= mGESel.GetSize()))
		{
			DbgAssert(0);
			return FALSE;
		}
		return mGESel[index];
	}

	void SetGeomEdgeSelected(int index, BOOL sel, BOOL bMirrorSel = FALSE);
	BOOL IsOpenGeomEdge(int index);

	//return geom space open edges
	BitArray GetOpenGeomEdges();
	BitArray ComputeOpenGeomEdges();
	//return tv space open edges
	BitArray GetOpenTVEdges();
	BitArray ComputeOpenTVEdges();

	//returns whether an edge is a hidden edge or not
	BOOL GetGeomEdgeHidden(int index) const;
	//returns the vertex index of the start or end vertex of the edge
	int GetGeomEdgeVert(int edgeIndex, int whichEnd) const;
	//returns the vector index of the start or end vertex of the edge, if the
	//object is not a patch -1 is returned
	int GetGeomEdgeVec(int edgeIndex, int whichEnd) const;
	//returns the number of faces connected to an edge
	int GetGeomEdgeNumberOfConnectedFaces(int edgeIndex) const;
	//returns the face connected to an edge
	int GetGeomEdgeConnectedFace(int edgeIndex, int ithFaceIndex) const;

	//this displays the geometry edge selection
	void DisplayGeomEdge(GraphicsWindow *gw, int edgeIndex, float size, BOOL thick, Color c);
	// display the mirror plane for geometry mirror seleciton.
	void DisplayMirrorPlane(GraphicsWindow* gw, int mirrorAxis, Color c);
	

//TV Face Methods*********************************************************************
	//we don't differentiate between tv and geo faces since they are a 1 to 1 correspondance
	//this returns the number faces
	int GetNumberFaces();
	//this returns the degree of the face ie the number of border edges to the face
	virtual int GetFaceDegree(int faceIndex);

	//this returns texture vert index of the ith border vert
	virtual int GetFaceTVVert(int faceIndex, int ithVert);
	void SetFaceTVVert(int faceIndex, int ithVert, int id);

	//this returns the texture vertex index of the ith border vec if no vectors are present -1 is returned	
	int GetFaceTVHandle(int faceIndex, int vertexID);
	void SetFaceTVHandle(int faceIndex, int ithVert, int id);

	//this returns the texture  vertex index of the ith interior vec if no vectors are present -1 is returned
	int GetFaceTVInterior(int faceIndex, int vertexID);
	void SetFaceTVInterior(int faceIndex, int ithVert, int id);

	//this returns the geom vertex index of the ith border vert
	int GetFaceGeomVert(int faceIndex, int ithVert);
	//this returns the geom vertex index of the ith border vec if no vectors are present -1 is returned
	int GetFaceGeomHandle(int faceIndex, int ithVert);
	//this returns the geom  vertex index of the ith interior vec if no vectors are present -1 is returned
	int GetFaceGeomInterior(int faceIndex, int ithVert);


	//face property accessor
	//gets/sets whether a face is selected
	BOOL GetFaceSelected(int faceIndex);
	void SetFaceSelected(int faceIndex, BOOL sel, BOOL bMirrorSel=FALSE);
	//gets/sets whether a face is selected preview
	BOOL GetFaceSelectedPreview(int faceIndex);
	void SetFaceSelectedPreview(int faceIndex, BOOL sel, BOOL bMirrorSel=FALSE);
	//gets/sets whether a face is frozen
	BOOL GetFaceFrozen(int faceIndex);
	void SetFaceFrozen(int faceIndex, BOOL frozen);
	//gets/sets whether a face is dead
	BOOL GetFaceDead(int faceIndex);
	void SetFaceDead(int faceIndex, BOOL dead);
	//gets/sets whether a face has curved mapping flag is set
	BOOL GetFaceCurvedMaping(int faceIndex);
	//gets/sets whether a face has curved mapping vectors are actually allocated
	BOOL GetFaceHasVectors(int faceIndex);
	//gets/sets whether a face has curved mapping interior vectors are actually allocated
	BOOL GetFaceHasInteriors(int faceIndex);
	BOOL GetFaceHidden(int faceIndex);

	//returns the material id of the face
	int GetFaceMatID(int faceIndex);

	//set the material id of the face
	void  SetFaceMatID(int faceIndex, int matID);

	//returns the geometric normal of a face
	Point3 GetGeomFaceNormal(int faceIndex);
	//returns the texture normal of a face
	Point3 GetUVFaceNormal(int faceIndex);

//given a face index return a tab of all the uvw vertex indices on this face
	void GetFaceTVPoints(int faceIndex, Tab<int> &vIndices);
//given a face index return a tab of all the geom vertex indices on this face
	void GetFaceGeomPoints(int faceIndex, Tab<int> &vIndices);

//given a face index OR all the uvw indices with the incoming bitarray
	void GetFaceTVPoints(int faceIndex, BitArray &vIndices);
//given a face index OR all the geom indices with the incoming bitarray
	void GetFaceGeomPoints(int faceIndex, BitArray &vIndices);

	//clones a particular face
	UVW_TVFaceClass *CloneFace(int faceIndex);

	//Face selection methods
	//this returns the current face selection for this local data
	const BitArray &GetFaceSel() { return mFSel; }
	//this copies the a selection into our unwrap face selection
	void SetFaceSel(const BitArray &set);
	void ClearFaceSelection();
	void SetAllFaceSelection();
	BitArray GetFaceSelection();
	BitArray *GetFaceSelectionPtr();
	void SetFaceSelectionByRef(const BitArray& sel, BOOL bMirrorSel = FALSE);
	void SetFaceSelection(BitArray sel, BOOL bMirrorSel=FALSE);
	void UpdateFaceSelInIsolationMode();
	//Face selection preview methods
	inline void ClearFaceSelectionPreview();
	BitArray GetFaceSelectionPreview();
	BitArray *GetFaceSelectionPreviewPtr();
	const BitArray &GetFaceSelPreview() { return mFSelPreview; }
	void SetFaceSelectionPreview(const BitArray& sel, BOOL bMirrorSel=FALSE);
	const BitArray& GetFaceFilter();
	bool IsFaceFilterBitsetOrMaterialIDFilterSet() const;

	//Selects faces based on material ID
	void SelectByMatID(int matID);
	//set selection faces material id
	void SetSelectionMatID(int matID);

	//given 2 geometric points return the geo edge index between them
	int FindGeoEdge(int a, int b);
	//given geom face index and 2 vert index on the face return this ith edge index that touches a and b
	int FindGeomEdge(int faceIndex,int a, int b);

	//given 2 uv points return the uv edge index between them
	int FindUVEdge(int a, int b);
	//given geom face index and 2 vert index on the face return this ith edge index that touches a and b
	int FindUVEdge(int faceIndex,int a, int b);

	//returns the tv faces that intersects point p
	//i1, i2 are the UVW space to check
	int PolyIntersect(Point3 p, int i1, int i2, BitArray *ignoredFaces = NULL);

	//hitests our geometry returning a list of faces that were hit
	void HitTestFaces(GraphicsWindow *gw, INode *node, HitRegion hr, Tab<int> &hitFaces);

	//hitests our geometry returning a closest face that was hit
	int HitTestFace(GraphicsWindow *gw, INode *node, HitRegion hr);

	//Returns ths angle between 2 vectors
	virtual float AngleFromVectors(Point3 vec1, Point3 vec2);
	//returns a tm based on a face
	Matrix3 GetTMFromFace(int faceIndex, BOOL useTVFace);

//returns the local bounding box of the selecetd faces
	Box3 GetSelFaceBoundingBox();
//returns the local bounding box of the whole mesh
	Box3 GetBoundingBox();

	//these are just some access to a temporary IPoint2 array of the tvs in dialog space
	//they are created and destroyed in the dialog paint method
	int TransformedPointsCount();
	void TransformedPointsSetCount(int ct);
	IPoint2 GetTransformedPoint(int i);
	void SetTransformedPoint(int i, IPoint2 pt);

	//this just returns whether the file was just loaded so we know to convert over old load data if need be
	BOOL IsLoaded() { return mLoaded; };
	void ClearLoaded() {mLoaded = FALSE;}
	void SetLoaded() {mLoaded = TRUE;}

	//this just takes old max data and put into our new format
	void ResolveOldData(UVW_ChannelClass &oldData);
	//this saves the old data back into the modifier so we can reload max 9.5 files into max9
	void SaveOldData(ISave *isave);

	void BuildMatIDFilter(int matID);

	void BuildAllFilters(bool enable);
	void UpdateAllFilters();
	void ClearAllFilters();

	bool DoesFacePassFilter(int fIndex) const;
	bool DoesTVVertexPassFilter(int vIndex) const;
	bool DoesGeomVertexPassFilter(int vIndex) const;
	bool DoesTVEdgePassFilter(int eIndex) const;
	bool DoesGeomEdgePassFilter(int eIndex) const;

	//these are our snap buffers
	//This builds the buffer where w/h are the size of the window
	void BuildSnapBuffer(int w, int h);
	void FreeSnapBuffer();
	//this gets/sets the snap vertex buffer were each cell contains the vertex that reside in it
	void SetVertexSnapBuffer(int index, int value);
	int GetVertexSnapBuffer(int index);

	//this gets/sets the snap edge buffer were each cell contains the vertex that reside in it
	void SetEdgeSnapBuffer(int index, int value);
	int GetEdgeSnapBuffer(int index);
	//Allows direct access to the edge snap buffer
	Tab<int> &GetEdgeSnapBuffer();

	//this are accessors to a buffer which tells you if an edge is touching a vertex that was snapped
	void SetEdgesConnectedToSnapvert(int index, BOOL value);
	BOOL GetEdgesConnectedToSnapvert(int index);

	//break the selected edges
	void BreakEdges();

//breaks the UV edges
	void BreakEdges(BitArray uvEdges);

	//break the selected verts
	void BreakVerts();
	//selected the selected verts
	void WeldSelectedVerts(float threshold, BOOL weldOnlyShared);

	//this takes the associated geo faces an ddetaches them and copies them into uv space
	//faceSel is the selection of faces to be detached
	//vertSel is the selection of vertices for the new detached element
	//mod is the modifier that own the local data needed to update the controllers if animation is on
	void DetachFromGeoFaces(const BitArray& faceSel, BitArray &vertSel);

	//given the vertex selection this returns all the opposing shared vertices
	void  GetEdgeCluster(BitArray &cluster);

	//this stitches give geometric edges together
	void SewEdges(const GeoTVEdgesMap &gtvInfo);

//several relax algorithms
	//first generation just center based relax
	//only called from maxscript now
	void  Relax(int subobjectMode, int iteration, float str, BOOL lockEdges, BOOL matchArea);
	//Experimental relax/fit algorithm that was never worked well
	//only called from maxscript now
	void  RelaxFit(int subobjectMode,int iteration, float str);
	//a spring based relax only called from script
	void  RelaxBySprings(int subobjectMode,int frames, float stretch, BOOL lockEdges, PeltData& peltData);

	//relax from the mesh/mnmesh algorithms
	void  RelaxVerts2(int subobjectMode, float relax, int iter, BOOL boundary, BOOL saddle, PeltData& peltData, BOOL applyToAll, BOOL updateUI);
	
	//this a relax that uses the shape of the geo faces to align texture faces
	void RelaxByFaceAngle(int subobjectMode, int frames, float stretch, float str, BOOL lockEdges,HWND statusHWND, PeltData& peltData, float aspect, BOOL applyToAll);

	//this a relax algorithm based on the angles of edges that touch a vertex
	void RelaxByEdgeAngle(int subobjectMode, int frames, float stretch,float str, BOOL lockEdges,HWND statusHWND, PeltData& peltData, BOOL applyToAll);

	void	UpdateClusterVertices(Tab<ClusterClass*> &clusterList);
	void	AlignCluster(Tab<ClusterClass*> &clusterList, int moveCluster, int innerFaceIndex, int outerFaceIndex,int edgeIndex);
	
	//these are various mapping algorithms
	//this applies a world space planar mapping from a normal
	void PlanarMapNoScale(Point3 gNormal);
	void BuildNormals(Tab<Point3>& normalList);
	BOOL NormalMap(Tab<Point3*> *normalList, Tab<ClusterClass*> &clusterList);
	void ApplyMap(int mapType, BOOL normalizeMap, const Matrix3& tm, int matid);


	//just a temporary hold/restore face selection
	//this does not use the undo buffer, just a quick method to store and restore the face selection
	void HoldFaceSel();
	void RestoreFaceSel();

	//these are the seam edges for the pelt mapper
	//return true if a and b are connected, false otherwise
	bool EdgeListFromPoints(Tab<int> &selEdges, int a, int b, const BitArray &candidateEdges);
	BitArray mSeamEdges;

	//Use the reference to check if the seam edges are dirty
	BitArray mSeamEdgesRef;
	bool	 mbSeamEdgesDirty;

	//Use the reference to check if the open edges are dirty
	BitArray mOpenTVEdges;
	BitArray mOpenTVEdgesRef;
	BitArray mOpenGeomEdges;
	BitArray mOpenGeomEdgesRef;
	bool	 mbOpenTVEdgesDirty;

	BitArray GetSeamEdgePreiveBitArray() const { return mSeamEdgesPreview;}
	//Use the reference to check if the preview of seam edges is dirty
	BitArray mSeamEdgesPreviewRef;
	bool	 mbSeamEdgesPreviewDirty;

	//implements lazy sewing operation
	bool GetSewingPending();
	void SetSewingPending();
	void ClearSewingPending();

	void ClearSeamEdgesPreview();
	void ResetSeamEdgesPreview();
	void SetSeamEdgesPreview(int index, int val, BOOL bMirror);
	int GetSeamEdgesPreviewSize();
	int GetSeamEdgesPreview(int index);
	void ExpandSelectionToSeams();
	void CutSeams(BitArray seams);
	void BuildSpringData(Tab<EdgeBondage> &springEdges, Tab<SpringClass> &verts, Point3 &rigCenter, Tab<RigPoint> &rigPoints, Tab<Point3> &initialPointData, float rigStrength);

	//tools to help manage the sketch tool soft selection
	void InitReverseSoftData();
	void ApplyReverseSoftData();

	//this builds our tv vert to geo vert list so we can look to see which geo vert own which tv vert
	void BuildVertexClusterList();

//these are our named selection sets for each local data and subobject
	GenericNamedSelSetList fselSet;
	GenericNamedSelSetList vselSet;
	GenericNamedSelSetList eselSet;
	
//this fixes up an issue with the system lock flags
//it goes through and recomputes these flags
	void FixUpLockedFlags();

//some tools to help intersect the mesh with a ray
//needs to be called before any checks
	void PreIntersect();
//intersects the ray with the mesh.  The ray needs to be in local space of the mesh
//returns whether hit amd the distance along the ray
	bool  Intersect(Ray r,bool checkFrontFaces,bool checkBackFaces, float &at, Point3 &bary, int &hitFace);
//needs to be called to clear out our memory after you are done intersecting
	void PostIntersect();

	//these are functions that tell meshtopodata that the user or system wants to cancel
	//their current operation, we used to just listen to the asynckeyboard state
	//but with threading we something more
	BOOL		GetUserCancel();
	void		SetUserCancel(BOOL cancel);

	//this builds a list of all the border verts for the visible faces.  the borderverts
	//is the order list of the border verts, each seperate border is seperated by a -1 entry 
	//in this list
	void GetBorders(Tab<int> &borderVerts);

	//this welds the the uvw vertices that share the same geometry vertex if they lie within a threshold
	// selectedFaces is whiuch faces to weld
	// useEdgeLimit determines if all uvws vertices will be welded or only ones with the threshold
	// edgeLimit is the weld threshold based on distance from the edge. this is based on a percent distance based the egde length
	// mod is the modifier that owns this local data
	void WeldFaces(BitArray &selectedFaces, BOOL useEdgeLimit, float edgeLimit);

	//this takes a list of clusters and rescales them to have an equal ratio with the geometry
	//	clusters is a tab the size of the number faces where each entry is the cluster id of the face
	//  clustRescale is the rescale value after the cluster scales have been computed this allows the users to set cluster priorities
	//  if set to -1 that face is not considered
	void RescaleClusters(const Tab<int> &clusters, const Tab<float> &clustRescale);

	//this holds all the selections into a temp buffer
	void HoldSelection();
	//this restores all the selections from a temp buffer
	void RestoreSelection();

	//this returns whether 2 texture vertices can be welded
	// weldOnlyShared is a flag to set to only weld texture verts that share a common geo vert
	// tvVert1 and tvVert2 are the 2 vertices to check
	BOOL OkToWeld(BOOL weldOnlyShared, int tvVert1, int tvVert2);

//returns a pointer to the grouping tool data
	ToolGroupingData* GetToolGroupingData();
	void InvalidateMirrorData();
	void BuildMirrorData(MirrorAxisOptions axis, int subObjLvl, float threshold, INode* pNode);
	int GetGeomEdgeMirrorIndex(int index) const;
	int GetGeomVertMirrorIndex(int index) const;
	void MirrorFaceSel(int faceIndex = -1, BOOL bSel = TRUE, BOOL bPreview = FALSE); //if faceIndex = -1, it mirror all the current face selection.
	void MirrorGeomEdgeSel(int edgeIndex = -1, BOOL bSel = TRUE); // if edgeIndex = -1, it mirrors all the current geom edge selection.
	void MirrorGeomVertSel(int vertIndex = -1, BOOL bSel = TRUE); // if vertIndex = -1, it mirrors all the current geom vert selection.
	void InvalidateMeshPolyData();
	void BuildMeshPolyData(float thresh = 45.0, bool bIgnoreVisEdge = false);
	void GetPolyFromFaces(int faceIndex, BitArray& set, BOOL bMirrorSel);
	AdjFaceList* GetMeshAdjFaceList();
	void ReleaseAdjFaceList(); 

	void GetLoopedEdges(const BitArray &inSel, BitArray &outSel);
	void GetRingedEdges(const BitArray &inSel, BitArray &outSel);

	void GetUVRingEdges(int edgeIdx, BitArray& ringEdges, bool bfindAll, std::vector<int>* pVec = nullptr);

	void GrowUVLoop(BOOL selectOpenEdges);
	void SelectUVEdgeLoop(BOOL selectOpenEdges);

	void PolySelection(BOOL add = TRUE, BOOL bPreview = FALSE);

	//In advance calling this function to avoid some data's write/read at the same time 
	//under the following parallel circumstance such as the PaintEdge function.
	void SychronizeSomeData();

	// from ITopoChangedListener
	void HandleTopoChanged();

	// Verify 2D topo valid or not for breaking edges
	static void VerifyTopoValidForBreakEdges(MeshTopoData* meshTopoData);

	BOOL GetSynchronizationToRenderItemFlag();
	void SetSynchronizationToRenderItemFlag(BOOL bFlag);

	//checks to see if this polygon is valid, degenerate polygons are not solved
	bool CheckPoly(int polyIndex);
	//based on the mesh type, add LSCM face one by one.
	void AddLSCMFaceData(int polyIndex, Tab<LSCMFace>&	targetFacesTab,bool forPeel = true);
	//Shared TV edges mean every 2 or more TV edges belong to the same selected geometry edge.	
	BitArray mSharedTVEdges;
	void UpdateSharedTVEdges(int curSubLvl);
	void	AddVertsToCluster(int faceIndex, BitArray &processedVerts, ClusterClass *cluster);

	int GetVertexClusterListCount();
	int GetVertexClusterData(int i);
	int GetVertexClusterSharedListCount();
	int GetVertexClusterSharedData(int i);

	void NonSquareAdjustVertexY(float aspect, float fVOffset, bool bInverse);

	virtual int RegisterNotification(IMeshTopoDataChangeListener* listener);
	virtual int UnRegisterNotification(IMeshTopoDataChangeListener* listener);
	virtual int RaiseTVDataChanged(BOOL bUpdateView);
	virtual int RaiseTVertFaceChanged(BOOL bUpdateView);
	virtual int RaiseSelectionChanged(BOOL bUpdateView);
	virtual int RaisePinAddedOrDeleted(int index);
	virtual int RaisePinInvalidated(int index);
	virtual int RaiseTopoInvalidated();
	virtual int RaiseTVVertChanged(TimeValue t, int index, const Point3& p);
	virtual int RaiseTVVertDeleted(int index);
	virtual int RaiseMeshTopoDataDeleted();
	virtual int RaiseNotifyUpdateTexmap();
	virtual int RaiseNotifyUIInvalidation(BOOL bRedrawAtOnce);
	virtual int RaiseNotifyFaceSelectionChanged();

private:
	Tab<IMeshTopoDataChangeListener*> mListenerTab;
//does a quick intersect test on a specific face
	bool CheckFace(Ray ray, Point3 p1,Point3 p2,Point3 p3, Point3 n, float &at, Point3 &bary);
	Tab<Point3> mIntersectNorms;

	Tab<int> mSketchBelongsToList;
	Tab<Point3> mOriginalPos;

	BitArray mSeamEdgesPreview;

	//a patch specific relax called from relaxverts2 
	void  RelaxPatch(int subobjectMode, int iteration, float str, BOOL lockEdges);

	inline BOOL LineIntersect(Point3 p1, Point3 p2, Point3 q1, Point3 q2);

	UVW_ChannelClass TVMaps;
	VertexLookUpListClass gverts;
	BOOL mHasIncomingSelection;

	//this is an bitarray of all our hidden polygons incoming from the stack mesh
	//we use this list to prevent those polygon from being displayed in the UV editor and 
	//to make sure they are unselectable
	BitArray mHiddenPolygonsFromPipe;

	//just sets all our pointers to null, it does not free anything
	void Init();

    // ValidateTVVertex checks the specified vertex for any invalid component (infinite or NaN), and dummies up any such
    // component to be equal to remediationValue, returning true if the vertex was valildate, and false if remediation was
    // required. ValidateTVVertices performs validation for all vertices in TVMaps.v, returning the number of vertices for
    // which remediation was required, and issuing a warning message if doWarning is true and any such vertices were
    // encountered.
    int ValidateTVVertices(bool doWarning = true);
    inline bool ValidateTVVertex(int indexVertex, float remediationValue = 0.0f)
    {
        bool isValid = true;
        Point3& vertex = (TVMaps.v[indexVertex]).GetPoint();
        for (int i = 0; i != 3; ++i)
        {
            if ((std::isnan(vertex[i])) || (std::isinf(vertex[i])))
            {
                vertex[i] = remediationValue;
                isValid = false;
            }
        }

        return isValid;
    }

	void ResetSelectionAndTVMapData();
	void SetCache(Mesh &mesh, int mapChannel);
	void SetCache(MNMesh &mesh, int mapChannel);
	void SetCache(PatchMesh &patch, int mapChannel);

	void BuildInitialMapping(Mesh *msh);
	void BuildInitialMapping(MNMesh *msh);
	void BuildInitialMapping(PatchMesh *patch);

	void CopyMatID(Mesh &mesh);
	void CopyMatID(MNMesh &mesh);
	void CopyMatID(PatchMesh &mesh);

	void ApplyMapping(Mesh &mesh, int mapChannel);
	void ApplyMapping(MNMesh &mesh, int mapChannel);
	void RemoveDeadVerts(PatchMesh *mesh,int CurrentChannel);
	void ApplyMapping(PatchMesh &patch, int mapChannel);

	void ApplyHide(Mesh &mesh);
	void ApplyHide(MNMesh &mesh);
	void ApplyHide(PatchMesh &patch);

	void SyncNonFaceFilters();
	bool IsNonFaceFiltersValid();

	bool IsFaceFilterValid() const;
	void BuildFaceFilter();
	void ClearFaceFilter();

	bool IsTVVertexFilterValid() const;
	void SyncTVVertexFilter();
	void ClearTVVertexFilter();

	bool IsGeomVertexFilterValid() const;
	void SyncGeomVertexFilter();
	void ClearGeomVertexFilter();

	bool IsTVEdgeFilterValid() const;
	void SyncTVEdgeFilter();
	void ClearTVEdgeFilter();

	bool IsGeomEdgeFilterValid() const;
	void SyncGeomEdgeFilter();
	void ClearGeomEdgeFilter();

	//weld edges selected by geom seams
	void WeldSelectedSeam(const GeoTVEdgesMap &gtvInfo);
	//weld two tv verts
	void WeldTVVerts(int tv1, int tv2, BitArray &replaceVerts, ReplaceVertMap &vertMap);

	void BuildVertMirrorData(); //build the mirror data based on current geom vertices.
	void BuildFaceMirrorData(); //build help data structure to fast look up mirrored face index.
	void BuildEdgeMirrorData(); //build help data structure to fast look up mirrored edge index.
	// mesh only has triangle faces while we select poly instead of faces, so we need build a poly list to help mirror the triangle faces.
	void BuildMeshFaceMirrorData();
	void BuildPolyFromFace(int faceIndex, float thresh = 45.0, bool bIgnoreVisEdge = false);
	void UpdateMirrorMatrix(MirrorAxisOptions axis, INode* pNode);
	void CalculateMirrorIndex(Tab<GeomSelMirrorData>& mirrorTable, std::vector<MirrorDataHelper*>& dataHelperVec, const Tab<GeomSelMirrorData>* pVertTab);
	void MirrorMeshFaceSel(int faceIndex, BOOL bSel, BOOL bPreview);

	std::vector<MeshPolyData*> mMeshPolyData; // get indice of the face list which belong to a given polygon
	std::vector<int> mFacePolyIndexTable;// get the poly index from a given face index.
	AdjFaceList* mpMeshAdjFaceData;
	Tab<GeomSelMirrorData> mGeomVertMirrorDataList;
	Tab<GeomSelMirrorData> mGeomFaceMirrorDataList;
	Tab<GeomSelMirrorData> mGeomEdgeMirrorDataList;
	Box3				   mMirrorBoundingBox;
	Matrix3	mMirrorMatrix;
	
	//this is the texture vertex selection bitarray
	BitArray mVSel;
	//this is the texture vertex selection preview bitarray
	BitArray mVSelPreview;

	//this is the geo vertex selection bitarray
	BitArray mGVSel;

	//these are the UVW edge selections	
	BitArray mESel;

	//these are the UVW edge selections preview bitarray
	BitArray mESelPreview;

	//these are the geometric vertex, edge selections
	BitArray mGESel;

	//this is the face selection
	//since faces are 1 to 1 we dont differeniate between geo and tv faces
	BitArray mFSel;
	//this flag is used to update the mat id ui to the new face selection
	BOOL faceSelChanged;
	
	//this is the face selection preview
	BitArray mFSelPreview;

	//this is the previsous face selection flowing up the stack, if the current stack selection
	//is different from the previous we want to use that
	BitArray mFSelPrevious;

	//These are just some bittarray to hold temporary selections
	BitArray holdFSel; //used by holdFSel/RestoreFSel.
	// transferSelectionStart/End can be called recursively, so we need a stack to hold different levels' selection.
	std::vector<BitArray> originalSels;
	std::vector<BitArray> holdFSels;
	std::vector<BitArray> holdESels;

	//this is a visiblity list for verts based on the matid filter;
	BitArray mVertMatIDFilterList;

	//this is the filter selected tv vert bitarray used to keep track of visible verts attached to selected faces
	BitArray mTVVertexFilter;
	//this is the filter selected geom vert bitarray used to keep track of visible verts attached to selected faces
	BitArray mGeomVertexFilter;
	//this is the face selection associated with the show selected face filter
	BitArray mFaceFilter;
	//this is the filter selected tv edge bitarray used to keep track of visible edges attached to selected faces
	BitArray mTVEdgeFilter;
	//this is the filter selected geom edge bitarray used to keep track of visible edges attached to selected faces
	BitArray mGeomEdgeFilter;

	//this is just a temporary list of the tvs in dialog space used to speed up some operations
	Tab<IPoint2> mTransformedPoints;
	BOOL mLoaded;

	//these are cluster id list so we know which texture verts are assigned to which geom verts
	//this a list were each enrty points to the equivalent geom index so you can find out whihc 
	//geo vert is attached to which texture vert
	Tab<int> mVertexClusterList;
	
	//this is just a cout of the number of shared tvverts at each tvert
	Tab<int> mVertexClusterListCounts;

	Mesh *mesh;
	PatchMesh *patch;
	MNMesh *mnMesh;

	//Snap data buffers
	BitArray mEdgesConnectedToSnapvert;
	Tab<int> mVertexSnapBuffer;
	Tab<int> mEdgeSnapBuffer;

	BOOL		mUserCancel;

	HoldGeomSelection mHoldGeomSel;
	
	ToolGroupingData mToolClusteringData;

	//Constant topo acceleration related stuff
	std::unique_ptr<const FacesAtVerts> mFacesAtVs;
	std::unique_ptr<const FacesAtVerts> mFacesAtVsFiltered;
	int mConstantTopoAccelerationNestingLevel;
	//Begin / End an acceleration scope.
	//Must be called in pairs, can be nested.
	void BeginConstantTopoAcceleration(BOOL enableFilterFaceSelection);
	void EndConstantTopoAcceleration();
	//Build new FacesAtVerts
	const FacesAtVerts* BuildNewFacesAtVs(BOOL enableFilterFaceSelection);
	//Retrieve current FacesAtVerts
	const FacesAtVerts* GetFacesAtVerts(BOOL enableFilterFaceSelection);

	bool mhasSewingPending;

	//If the edge geometry selection is changed in the function fnSyncGeomSelection, this flag will be set TRUE.
	// Then the render item will update the attribute buffer.After the update is finished, the flag will be set FALSE.
	BOOL mbNeedSynchronizationToRenderItem;

	// To judge whether the 2D topo faces are welded or have the same UV.
	// If that, the break edges cannot be done in these 2D topo edges.
	bool mbValidTopoForBreakEdges;
	bool mbBreakEdgesSucceeded;

	//Denote some faces are filtered by the materialID
	BitArray mMaterialIDFaceFilter;

	//Sync sel from Geom to TV if isGeoToTV is true
	//Sync sel from TV to Geom if isGeoToTV is false
	void _GeomTVEdgeSelMutualConvertion(BitArray &geoSel, BitArray &uvwSel, bool isGeoToTV);

	CRITICAL_SECTION mCritSect;  // used to prevent TransferSelectionStart/TransferSelectionEnd from  being called at the same time
};

//Guard class for using constant topo acceleration on MeshTopoData
class ConstantTopoAccelerator : public MaxSDK::Util::Noncopyable
{
public:
	ConstantTopoAccelerator(MeshTopoData *ld, BOOL enableFilterFaceSelection);
	virtual ~ConstantTopoAccelerator();
	const FacesAtVerts* GetFacesAtVerts();

private:
	MeshTopoData *mLD;
	BOOL mFiltered;
};

/*************************************
This is just a container class that contains
all the active local data for a specific unwrap
**************************************/
class MeshTopoDataNodeInfo
{
public:
	MaxSDK::SingleWeakRefMaker mWeakRefToNode;
	MeshTopoData* mMeshTopoData;
	INode* GetNode()
	{
		return (INode*)(mWeakRefToNode.GetRef());
	}
};

class MeshTopoDataContainer : public IMeshTopoDataContainer
{
public:
		MeshTopoDataContainer()	{}

		virtual ~MeshTopoDataContainer() {}

		bool Lock() 
		{
			return mLocked.test_and_set();
		}
		void Unlock() { mLocked.clear(); }
		int Count();
		void SetCount(int ct);
		int Append(int ct, MeshTopoData* ld, INode *node, int extraCT);
		virtual IMeshTopoData* Get(int i)
		{
			return this->operator[](i);
		}

		MeshTopoData* operator[](int i) 
		{ 
			if ((i < 0) || (i >= mMeshTopoDataNodeInfo.Count()))
			{
				DbgAssert(0);
				return NULL;
			}
			return mMeshTopoDataNodeInfo[i].mMeshTopoData;  
		};

		Point3 GetNodeColor(int index);
		Matrix3 GetNodeTM(TimeValue t, int index);
		INode* GetNode(int index);
		void HoldPointsAndFaces();
		void HoldPoints();
		void HoldSelection();
		virtual void ClearTopoData(MeshTopoData* ld);

private:
	Tab<MeshTopoDataNodeInfo>  mMeshTopoDataNodeInfo;
	std::atomic_flag	mLocked;
};
