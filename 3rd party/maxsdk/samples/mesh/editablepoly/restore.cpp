 /**********************************************************************  
 *<
	FILE: Restore.cpp

	DESCRIPTION: Editable Polygon Mesh Object - RestoreObjects.

	CREATED BY: Steve Anderson

	HISTORY: created April 2000

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#include "EPoly.h"
#include "PolyEdit.h"

//#define EPOLY_RESTORE_DEBUG_PRINT

// --- Restore objects ---------------------------------------------

// Not a real restore object - just used in drags to restore geometric
// and map vertex changes.
TempMoveRestore::TempMoveRestore (EditPolyObject *em) {
	init.SetCount (em->mm.numv);
	active.SetSize (em->mm.numv);
	active.ClearAll ();
	for (int i=0; i<em->mm.numv; i++) {
		if (em->mm.v[i].GetFlag (MN_DEAD)) continue;
		init[i] = em->mm.v[i].p;
	}

	mapInit = new Tab<UVVert>[em->mm.numm + NUM_HIDDENMAPS];
	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<em->mm.numm; mapChannel++) {
		if (em->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		int offsetMapChannel = NUM_HIDDENMAPS + mapChannel;
		mapInit[offsetMapChannel].SetCount (em->mm.M(mapChannel)->numv);
		for (int i=0; i<em->mm.M(mapChannel)->numv; i++) {
			mapInit[offsetMapChannel][i] = em->mm.M(mapChannel)->v[i];
		}
	}
}

TempMoveRestore::~TempMoveRestore () {
	delete [] mapInit;
}

void TempMoveRestore::Restore (EditPolyObject *em) {
	if (!init.Count()) return;
	IMNMeshUtilities8* mesh8 = static_cast<IMNMeshUtilities8*>(em->mm.GetInterface( IMNMESHUTILITIES8_INTERFACE_ID ));
	for (int i=0; i<em->mm.numv; i++) {
		if (em->mm.v[i].GetFlag (MN_DEAD)) continue;
		if (em->mm.v[i].p != init[i]) {
			em->mm.v[i].p = init[i];
			if (mesh8) {
				mesh8->InvalidateVertexCache(i);
			}
		}
	}

	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<em->mm.numm; mapChannel++) {
		if (em->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		int offsetMapChannel = NUM_HIDDENMAPS + mapChannel;
		for (int i=0; i<em->mm.M(mapChannel)->numv; i++) {
			em->mm.M(mapChannel)->v[i] = mapInit[offsetMapChannel][i];
		}
	}
}

// -----------------------------------

ComponentFlagRestore::ComponentFlagRestore (EditPolyObject *e, int selLevel) {
	mpEditPoly = e;
	sl = selLevel;

	switch (sl) {
	case MNM_SL_OBJECT:
		return;

	case MNM_SL_VERTEX:
		undo.SetCount (mpEditPoly->mm.numv);
		for (int i=0; i<undo.Count(); i++) {
			undo[i] = mpEditPoly->mm.v[i].ExportFlags ();
		}
		break;

	case MNM_SL_EDGE:
		undo.SetCount (mpEditPoly->mm.nume);
		for (int i=0; i<undo.Count(); i++) {
			undo[i] = mpEditPoly->mm.e[i].ExportFlags ();
		}
		break;

	case MNM_SL_FACE:
		undo.SetCount (mpEditPoly->mm.numf);
		for (int i=0; i<undo.Count(); i++) {
			undo[i] = mpEditPoly->mm.f[i].ExportFlags ();
		}
		break;
	}
}

void ComponentFlagRestore::Restore (int isUndo) {
	int max = undo.Count ();

	switch (sl) {
	case MNM_SL_OBJECT:
		return;

	case MNM_SL_VERTEX:
		redo.SetCount (mpEditPoly->mm.numv);
		for (int i=0; i<redo.Count(); i++) {
			redo[i] = mpEditPoly->mm.v[i].ExportFlags ();
			if (i<max) mpEditPoly->mm.v[i].ImportFlags (undo[i]);
		}
		break;

	case MNM_SL_EDGE:
		redo.SetCount (mpEditPoly->mm.nume);
		for (int i=0; i<redo.Count(); i++) {
			redo[i] = mpEditPoly->mm.e[i].ExportFlags ();
			if (i<max) mpEditPoly->mm.e[i].ImportFlags (undo[i]);
		}
		break;

	case MNM_SL_FACE:
		redo.SetCount (mpEditPoly->mm.numf);
		for (int i=0; i<redo.Count(); i++) {
			redo[i] = mpEditPoly->mm.f[i].ExportFlags ();
			if (i<max) mpEditPoly->mm.f[i].ImportFlags (undo[i]);
		}
		break;
	}

	// We may have changed topology or selection ...
	mpEditPoly->LocalDataChanged (TOPO_CHANNEL|GEOM_CHANNEL|SELECT_CHANNEL);
	// sca - or geometry - such as bounding boxes.
}

void ComponentFlagRestore::Redo () {
	int max = undo.Count ();

	switch (sl) {
	case MNM_SL_OBJECT:
		return;

	case MNM_SL_VERTEX:
		if (max>mpEditPoly->mm.numv) max = mpEditPoly->mm.numv;
		for (int i=0; i<max; i++) mpEditPoly->mm.v[i].ImportFlags (redo[i]);
		break;

	case MNM_SL_EDGE:
		if (max>mpEditPoly->mm.nume) max = mpEditPoly->mm.nume;
		for (int i=0; i<max; i++) mpEditPoly->mm.e[i].ImportFlags (redo[i]);
		break;

	case MNM_SL_FACE:
		if (max>mpEditPoly->mm.numf) max = mpEditPoly->mm.numf;
		for (int i=0; i<max; i++) mpEditPoly->mm.f[i].ImportFlags (redo[i]);
		break;
	}

	// We may have changed topology or selection ...
	mpEditPoly->LocalDataChanged (TOPO_CHANNEL|GEOM_CHANNEL|SELECT_CHANNEL);
	// sca - or geometry - such as bounding boxes.
}

//-----------------------------------------------------
// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
// PipelineClient restore object.
TopoPipelineClientRestore::~TopoPipelineClientRestore()
{
	// Clear out pipeline copies.
	ClearStorage(mTopoChannelsBefore);
	ClearStorage(mTopoChannelsAfter);
}
void TopoPipelineClientRestore::ClearStorage(Tab<BaseInterface*>& storage)
{
	for (int i = 0; i < storage.Count(); i++) {
		storage[i]->DeleteInterface();
	}
	storage.ZeroCount();
}
void TopoPipelineClientRestore::FillStorage(Tab<BaseInterface*>& storage, MNMesh& sourceMesh)
{
	// Make copies of the before TOPO channels for use during undo/redo.
	IPipelineClient* plClient = NULL;
	BaseInterface* pBI = NULL;
	BaseInterface* pClone = NULL;
	
	ClearStorage(storage); // reset storage

	for (int i = 0; i < sourceMesh.NumInterfaces(); i++) {
		pBI = sourceMesh.GetInterfaceAt(i);
		DbgAssert(pBI != NULL);

		plClient = static_cast<IPipelineClient*>(pBI->GetInterface(PIPELINECLIENT_INTERFACE));
		if (plClient != NULL) {
			pClone = pBI->CloneInterface();
			storage.Append(1, &pClone);
		}
	}
}
void TopoPipelineClientRestore::UseStorage(MNMesh& targetMesh, Tab<BaseInterface*>& storage)
{
	// Update TOPO_CHANNELS with face changes - 15/12/2005 - Kuo-Cheng Tong
	IPipelineClient* plClient = NULL;
	BaseInterface* pBI = NULL;

	// Copy the TOPO_CHANNELS interfaces into the meshes'
	for (int i = 0; i < storage.Count(); i++) {
		pBI = targetMesh.GetInterface(storage[i]->GetID()); // get the matching pipeline client
		DbgAssert(pBI != NULL);
		plClient = static_cast<IPipelineClient*>(pBI->GetInterface(PIPELINECLIENT_INTERFACE));
		if (plClient != NULL) {
			plClient->FreeChannels(TOPO_CHANNEL, 0);
			plClient->DeepCopy(static_cast<IPipelineClient*>(storage[i]->GetInterface(PIPELINECLIENT_INTERFACE)), TOPO_CHANNEL);
		}
	}
}
void TopoPipelineClientRestore::Before(MNMesh& beforeMesh)
{
	// Make copies of the before TOPO channels for use during undo/redo.
	FillStorage(mTopoChannelsBefore, beforeMesh);
}
void TopoPipelineClientRestore::After(MNMesh& afterMesh)
{
	// Make copies of the after TOPO channels for use during undo/redo.
	FillStorage(mTopoChannelsAfter, afterMesh);
}
void TopoPipelineClientRestore::RestoreBefore(MNMesh& changedMesh)
{
	// Copy the stored before channels back onto the mesh
	UseStorage(changedMesh, mTopoChannelsBefore);
}
void TopoPipelineClientRestore::RestoreAfter(MNMesh& changedMesh)
{
	// Copy the stored after channels back onto the mesh
	UseStorage(changedMesh, mTopoChannelsAfter);
}

//-----------------------------------------------------

TopoChangeRestore::TopoChangeRestore (EditPolyObject *e) {
	mpEditPoly = e;
	vfacBefore = NULL;
	vfacAfter = NULL;
	vedgBefore = NULL;
	vedgAfter = NULL;
	mapVertID = NULL;
	mapFaceID = NULL;
	mapVertsBefore = NULL;
	mapVertsAfter = NULL;
	mapFacesBefore = NULL;
	mapFacesAfter = NULL;
	vertexDataCount = edgeDataCount = 0;
	vertexDataBefore = NULL;
	vertexDataAfter = NULL;
	edgeDataBefore = NULL;
	edgeDataAfter = NULL;

	mNormBefore = NULL;
	mNormAfter = NULL;

}

TopoChangeRestore::~TopoChangeRestore () {
	if (vfacBefore) delete [] vfacBefore;
	if (vfacAfter) delete [] vfacAfter;
	if (vedgBefore) delete [] vedgBefore;
	if (vedgAfter) delete [] vedgAfter;
	if (mapVertID) delete [] mapVertID;
	if (mapFaceID) delete [] mapFaceID;
	if (mapVertsBefore) delete [] mapVertsBefore;
	if (mapVertsAfter) delete [] mapVertsAfter;
	if (vertexDataBefore) delete [] vertexDataBefore;
	if (vertexDataAfter) delete [] vertexDataAfter;
	if (edgeDataBefore) delete [] edgeDataBefore;
	if (edgeDataAfter) delete [] edgeDataAfter;
	if (mapFacesBefore) {
		for (int mapChannel=0; mapChannel<numMvBefore.Count(); mapChannel++) {
			Tab<MNMapFace> & mf = mapFacesBefore[mapChannel];
			for (int i=0; i<mf.Count(); i++) mf[i].Clear ();
		}
		delete [] mapFacesBefore;
	}
	if (mapFacesAfter) {
		for (int mapChannel=0; mapChannel<numMvBefore.Count(); mapChannel++) {
			Tab<MNMapFace> & mf = mapFacesAfter[mapChannel];
			for (int i=0; i<mf.Count(); i++) mf[i].Clear ();
		}
		delete [] mapFacesAfter;
	}

	for (int i=0; i<facesBefore.Count(); i++) {
		facesBefore[i].Clear();
	}
	for (int i=0; i<facesAfter.Count(); i++) {
		facesAfter[i].Clear();
	}

	if (mNormBefore) {
		delete mNormBefore;
	}
	if (mNormAfter) {
		delete mNormAfter;
	}
}

void TopoChangeRestore::After () {
	numvBefore = start.numv;
	numeBefore = start.nume;
	numfBefore = start.numf;
	numvAfter = mpEditPoly->mm.numv;
	numeAfter = mpEditPoly->mm.nume;
	numfAfter = mpEditPoly->mm.numf;
	int mapChannel;
	int mapCount = NUM_HIDDENMAPS + mpEditPoly->mm.numm;	// Note: assuming that start has <= numm.
	numMvBefore.SetCount (mapCount);
	numMvAfter.SetCount (mapCount);
	mapVertID = new Tab<int>[mapCount];
	mapFaceID = new Tab<int>[mapCount];
	mapVertsBefore = new Tab<UVVert>[mapCount];
	mapVertsAfter = new Tab<UVVert>[mapCount];
	mapFacesBefore = new Tab<MNMapFace>[mapCount];
	mapFacesAfter = new Tab<MNMapFace>[mapCount];
	for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		int offsetMapChannel = NUM_HIDDENMAPS + mapChannel;
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) {
			numMvBefore[offsetMapChannel] = numMvAfter[offsetMapChannel] = 0;
		} else {
			numMvBefore[offsetMapChannel] = start.M(mapChannel)->numv;
			numMvAfter[offsetMapChannel] = mpEditPoly->mm.M(mapChannel)->numv;
		}
	}

	// Record vertex changes:
	int i = 0, j = 0, ct = 0, minv = (numvBefore < numvAfter) ? numvBefore : numvAfter;
	for (i=0; i<minv; i++) {
		bool differs=FALSE;
		if (!(start.v[i] == mpEditPoly->mm.v[i])) differs=TRUE;
		if (!differs && (start.vfac[i].Count() != mpEditPoly->mm.vfac[i].Count())) differs=TRUE;
		if (!differs) {
			for (j=0; j<start.vfac[i].Count(); j++) if (start.vfac[i][j] != mpEditPoly->mm.vfac[i][j]) break;
			if (j<start.vfac[i].Count()) differs=TRUE;
		}
		if (!differs && (start.vedg[i].Count() != mpEditPoly->mm.vedg[i].Count())) differs = TRUE;
		if (!differs) {
			for (j=0; j<start.vedg[i].Count(); j++) if (start.vedg[i][j] != mpEditPoly->mm.vedg[i][j]) break;
			if (j<start.vedg[i].Count()) differs=TRUE;
		}
		if (!differs) continue;
		// Something's changed:
		vertID.Append (1, &i, minv/10);
	}
	vertsBefore.SetCount (vertID.Count()+numvBefore-minv);
	vertsAfter.SetCount (vertID.Count()+numvAfter-minv);
	ct=vertsBefore.Count();
	if (ct > 0) {
		vfacBefore = new Tab<int>[ct];
		vedgBefore = new Tab<int>[ct];
	}
	ct=vertsAfter.Count();
	if (ct > 0) {
		vfacAfter = new Tab<int>[ct];
		vedgAfter = new Tab<int>[ct];
	}
	ct=vertID.Count();
	for (i=0; i<ct; i++) {
		int j = vertID[i];
		vertsBefore[i] = start.v[j];
		vfacBefore[i] = start.vfac[j];
		vedgBefore[i] = start.vedg[j];
		vertsAfter[i] = mpEditPoly->mm.v[j];
		vfacAfter[i] = mpEditPoly->mm.vfac[j];
		vedgAfter[i] = mpEditPoly->mm.vedg[j];
	}
	for (i=minv; i<numvBefore; i++) {
		int j = i-minv+ct;
		vertsBefore[j] = start.v[i];
		vfacBefore[j] = start.vfac[i];
		vedgBefore[j] = start.vedg[i];
	}
	for (i=minv; i<numvAfter; i++) {
		int j = i-minv+ct;
		vertsAfter[j] = mpEditPoly->mm.v[i];
		vfacAfter[j] = mpEditPoly->mm.vfac[i];
		vedgAfter[j] = mpEditPoly->mm.vedg[i];
	}

	// Don't get fancy with the vertex and edge data, just back it up.
	vertexDataCount = start.vdSupport.GetSize ();
	if (mpEditPoly->mm.vdSupport.GetSize() > vertexDataCount) vertexDataCount = mpEditPoly->mm.vdSupport.GetSize();
	if (vertexDataCount) {
		vertexDataBefore = new Tab<float>[vertexDataCount];
		vertexDataAfter = new Tab<float>[vertexDataCount];
		for (int dataChannel=0; dataChannel<vertexDataCount; dataChannel++) {
			if (dataChannel == VDATA_SELECT) continue;	// Please don't bother storing soft selection data.
			float *ovd = start.vertexFloat (dataChannel);
			float *nvd = mpEditPoly->mm.vertexFloat (dataChannel);
			if (ovd && start.numv) {
				vertexDataBefore[dataChannel].SetCount (start.numv);
				memcpy (vertexDataBefore[dataChannel].Addr(0), ovd, start.numv*sizeof(float));
			}
			if (nvd && mpEditPoly->mm.numv) {
				vertexDataAfter[dataChannel].SetCount (mpEditPoly->mm.numv);
				memcpy (vertexDataAfter[dataChannel].Addr(0), nvd, mpEditPoly->mm.numv*sizeof(float));
			}
		}
	}

	edgeDataCount = start.edSupport.GetSize ();
	if (mpEditPoly->mm.edSupport.GetSize() > edgeDataCount) edgeDataCount = mpEditPoly->mm.edSupport.GetSize();
	if (edgeDataCount) {
		edgeDataBefore = new Tab<float>[edgeDataCount];
		edgeDataAfter = new Tab<float>[edgeDataCount];
		for (int dataChannel=0; dataChannel<edgeDataCount; dataChannel++) {
			float *oed = start.edgeFloat (dataChannel);
			float *ned = mpEditPoly->mm.edgeFloat (dataChannel);
			if (oed && start.nume) {
				edgeDataBefore[dataChannel].SetCount (start.nume);
				memcpy (edgeDataBefore[dataChannel].Addr(0), oed, start.nume*sizeof(float));
			}
			if (ned && mpEditPoly->mm.nume) {
				edgeDataAfter[dataChannel].SetCount (mpEditPoly->mm.nume);
				memcpy (edgeDataAfter[dataChannel].Addr(0), ned, mpEditPoly->mm.nume*sizeof(float));
			}
		}
	}

	// Record Edge changes:
	int mine = (numeBefore < numeAfter) ? numeBefore : numeAfter;
	for (i=0; i<mine; i++) {
		if (start.e[i] == mpEditPoly->mm.e[i]) continue;
		// Something's changed:
		edgeID.Append (1, &i, mine/10);
	}
	edgesBefore.SetCount (edgeID.Count()+numeBefore-mine);
	edgesAfter.SetCount (edgeID.Count()+numeAfter-mine);
	for (i=0; i<edgeID.Count(); i++) {
		int j = edgeID[i];
		edgesBefore[i] = start.e[j];
		edgesAfter[i] = mpEditPoly->mm.e[j];
	}
	for (i=mine; i<numeBefore; i++) {
		int j = i-mine+edgeID.Count();
		edgesBefore[j] = start.e[i];
	}
	for (i=mine; i<numeAfter; i++) {
		int j = i-mine+edgeID.Count();
		edgesAfter[j] = mpEditPoly->mm.e[i];
	}

	// Record face changes:
	int minf = (numfBefore < numfAfter) ? numfBefore : numfAfter;
	for (i=0; i<minf; i++) {
		if (start.f[i] == mpEditPoly->mm.f[i]) continue;
		// Something's changed:
		faceID.Append (1, &i, minf/10);
	}
	MNFace ftemp;
	ftemp.SetAlloc (0);
	facesBefore.Resize (faceID.Count()+numfBefore-minf);
	facesAfter.Resize (faceID.Count()+numfAfter-minf);
	for (i=0; i<faceID.Count(); i++) {
		int j = faceID[i];
		facesBefore.Append (1, &ftemp);
		facesBefore[i] = start.f[j];
		facesAfter.Append (1, &ftemp);
		facesAfter[i] = mpEditPoly->mm.f[j];
	}
	for (i=minf; i<numfBefore; i++) {
		int j = i-minf+faceID.Count();
		facesBefore.Append (1, &ftemp);
		facesBefore[j] = start.f[i];
	}
	for (i=minf; i<numfAfter; i++) {
		int j = i-minf+faceID.Count();
		facesAfter.Append (1, &ftemp);
		facesAfter[j] = mpEditPoly->mm.f[i];
	}

	// Record Specified Normal changes, and ensure that they are in synch with the Face changes:
	if (mpEditPoly->mm.GetSpecifiedNormals())
	{
		if (mNormAfter) delete mNormAfter;
		mNormAfter = new MNNormalSpec();
		*mNormAfter = *mpEditPoly->mm.GetSpecifiedNormals();
	}

	// Record Map Vertex and Face changes:
	for (int offsetMapChannel=0; offsetMapChannel<numMvBefore.Count(); offsetMapChannel++) {
		mapChannel = offsetMapChannel - NUM_HIDDENMAPS;
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		int minmv = numMvBefore[offsetMapChannel];
		if (minmv > numMvAfter[offsetMapChannel]) minmv = numMvAfter[offsetMapChannel];
		for (i=0; i<minmv; i++) {
			if (start.M(mapChannel)->v[i] == mpEditPoly->mm.M(mapChannel)->v[i]) continue;
			// Something's changed:
			mapVertID[offsetMapChannel].Append (1, &i, minmv/10);
		}

		mapVertsBefore[offsetMapChannel].SetCount (mapVertID[offsetMapChannel].Count()+numMvBefore[offsetMapChannel]-minmv);
		mapVertsAfter[offsetMapChannel].SetCount (mapVertID[offsetMapChannel].Count()+numMvAfter[offsetMapChannel]-minmv);

		ct=mapVertID[offsetMapChannel].Count();
		for (i=0; i<ct; i++) {
			int j = mapVertID[offsetMapChannel][i];
			mapVertsBefore[offsetMapChannel][i] = start.M(mapChannel)->v[j];
			mapVertsAfter[offsetMapChannel][i] = mpEditPoly->mm.M(mapChannel)->v[j];
		}
		for (i=minmv; i<numMvBefore[offsetMapChannel]; i++) {
			mapVertsBefore[offsetMapChannel][i-minmv+ct] = start.M(mapChannel)->v[i];
		}
		for (i=minmv; i<numMvAfter[offsetMapChannel]; i++) {
			mapVertsAfter[offsetMapChannel][i-minmv+ct] = mpEditPoly->mm.M(mapChannel)->v[i];
		}

		// Faces.
		// (Use same minf, numfbefore, numfafter, etc, as for main mesh.)
		mapFaceID[offsetMapChannel].SetCount(0);
		for (i=0; i<minf; i++) {
			if (start.M(mapChannel)->f[i] == mpEditPoly->mm.M(mapChannel)->f[i]) continue;
			// Something's changed:
			mapFaceID[offsetMapChannel].Append (1, &i, minf/10);
		}

		MNMapFace mftemp;
		mftemp.SetAlloc(0);

		mapFacesBefore[offsetMapChannel].Resize (mapFaceID[offsetMapChannel].Count()+numfBefore-minf);
		mapFacesAfter[offsetMapChannel].Resize (mapFaceID[offsetMapChannel].Count()+numfAfter-minf);
		for (i=0; i<mapFaceID[offsetMapChannel].Count(); i++) {
			int j = mapFaceID[offsetMapChannel][i];
			mapFacesBefore[offsetMapChannel].Append (1, &mftemp);
			mapFacesBefore[offsetMapChannel][i] = start.M(mapChannel)->f[j];
			mapFacesAfter[offsetMapChannel].Append (1, &mftemp);
			mapFacesAfter[offsetMapChannel][i] = mpEditPoly->mm.M(mapChannel)->f[j];
		}
		for (i=minf; i<numfBefore; i++) {
			int j = i-minf+mapFaceID[offsetMapChannel].Count();
			mapFacesBefore[offsetMapChannel].Append (1, &mftemp);
			mapFacesBefore[offsetMapChannel][j] = start.M(mapChannel)->f[i];
		}
		for (i=minf; i<numfAfter; i++) {
			int j = i-minf+mapFaceID[offsetMapChannel].Count();
			mapFacesAfter[offsetMapChannel].Append (1, &mftemp);
			mapFacesAfter[offsetMapChannel][j] = mpEditPoly->mm.M(mapChannel)->f[i];
		}
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Make copies of the before and after TOPO channels for use during undo/redo.
	mTopoPipelineRestore.Before(start);
	mTopoPipelineRestore.After(mpEditPoly->mm);

	// Get rid of bulky extra mesh
	start.ClearAndFree ();
}

void TopoChangeRestore::Restore (int isUndo) {
	// Undo vertex changes:
#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Undoing a topological change.\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif
	if (numvBefore != numvAfter) mpEditPoly->mm.setNumVerts (numvBefore);
	for (int i=0; i<vertID.Count (); i++) {
		int j=vertID[i];
		mpEditPoly->mm.v[j] = vertsBefore[i];
		mpEditPoly->mm.vfac[j] = vfacBefore[i];
		mpEditPoly->mm.vedg[j] = vedgBefore[i];
	}
	int offset = vertID.Count() - numvAfter;
	for (int i=numvAfter; i<numvBefore; i++) {
		int j=i+offset;
		mpEditPoly->mm.v[i] = vertsBefore[j];
		mpEditPoly->mm.vfac[i] = vfacBefore[j];
		mpEditPoly->mm.vedg[i] = vedgBefore[j];
	}

	// Restore the vertex data:
	for (int dataChannel=0; dataChannel<vertexDataCount; dataChannel++) {
		if (dataChannel == VDATA_SELECT) continue;
		if (vertexDataBefore[dataChannel].Count() == 0) {
			mpEditPoly->mm.setVDataSupport (dataChannel, false);
			continue;
		}
		mpEditPoly->mm.setVDataSupport (dataChannel, true);
		float *vd = mpEditPoly->mm.vertexFloat (dataChannel);
		if (vd && numvBefore) memcpy (vd, vertexDataBefore[dataChannel].Addr(0), numvBefore * sizeof(float));
	}

	// Undo edge changes:
	if (numeBefore != numeAfter) mpEditPoly->mm.setNumEdges (numeBefore);
	for (int i=0; i<edgeID.Count (); i++) mpEditPoly->mm.e[edgeID[i]] = edgesBefore[i];
	offset = edgeID.Count() - numeAfter;
	for (int i=numeAfter; i<numeBefore; i++) mpEditPoly->mm.e[i] = edgesBefore[i+offset];

	// Restore the edge data:
	for (int dataChannel=0; dataChannel<edgeDataCount; dataChannel++) {
		if (edgeDataBefore[dataChannel].Count() == 0) {
			mpEditPoly->mm.setEDataSupport (dataChannel, false);
			continue;
		}
		mpEditPoly->mm.setEDataSupport (dataChannel, true);
		float *ed = mpEditPoly->mm.edgeFloat (dataChannel);
		if (ed && numeBefore) memcpy (ed, edgeDataBefore[dataChannel].Addr(0), numeBefore * sizeof(float));
	}

	// Undo face changes:
	if (numfBefore != numfAfter) mpEditPoly->mm.setNumFaces (numfBefore);
	for (int i=0; i<faceID.Count (); i++) mpEditPoly->mm.f[faceID[i]] = facesBefore[i];
	offset = faceID.Count() - numfAfter;
	for (int i=numfAfter; i<numfBefore; i++) {
		mpEditPoly->mm.f[i] = facesBefore[i+offset];
	}

	// Undo specified normal changes:
	MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals();
	if (mNormBefore)
	{
		if (pNorm == NULL)
		{
			mpEditPoly->mm.SpecifyNormals ();
			pNorm = mpEditPoly->mm.GetSpecifiedNormals();
		}
		pNorm->SetParent(&mpEditPoly->mm);
		*pNorm = *mNormBefore;
		pNorm->ComputeNormals();
	}

	// Undo map changes:
	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		int offsetMapChannel = mapChannel + NUM_HIDDENMAPS;

		// Map Vertices:
		if (numMvBefore[offsetMapChannel] != numMvAfter[offsetMapChannel]) {
			mpEditPoly->mm.M(mapChannel)->setNumVerts (numMvBefore[offsetMapChannel]);
		}
		for (int i=0; i<mapVertID[offsetMapChannel].Count (); i++) {
			int j=mapVertID[offsetMapChannel][i];
			mpEditPoly->mm.M(mapChannel)->v[j] = mapVertsBefore[offsetMapChannel][i];
		}
		int offset = mapVertID[offsetMapChannel].Count() - numMvAfter[offsetMapChannel];
		for (int i=numMvAfter[offsetMapChannel]; i<numMvBefore[offsetMapChannel]; i++) {
			mpEditPoly->mm.M(mapChannel)->v[i] = mapVertsBefore[offsetMapChannel][i+offset];
		}

		// Map Faces:
		if (numfBefore != numfAfter) mpEditPoly->mm.M(mapChannel)->setNumFaces (numfBefore);
		for (int i=0; i<mapFaceID[offsetMapChannel].Count(); i++) {
			mpEditPoly->mm.M(mapChannel)->f[mapFaceID[offsetMapChannel][i]] = mapFacesBefore[offsetMapChannel][i];
		}
		offset = mapFaceID[offsetMapChannel].Count() - numfAfter;
		for (int i=numfAfter; i<numfBefore; i++) {
			mpEditPoly->mm.M(mapChannel)->f[i].Init ();
			mpEditPoly->mm.M(mapChannel)->f[i] = mapFacesBefore[offsetMapChannel][i+offset];
		}
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the before interfaces into the mesh's
	mTopoPipelineRestore.RestoreBefore(mpEditPoly->mm);

#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Topological change undone.\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	mpEditPoly->LocalDataChanged (ALL_CHANNELS - SUBSEL_TYPE_CHANNEL);
}

void TopoChangeRestore::Redo () {
#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Undoing a topological change.\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif
	// Redo vertex changes:
	if (numvBefore != numvAfter) mpEditPoly->mm.setNumVerts (numvAfter);
	for (int i=0; i<vertID.Count (); i++) {
		int j = vertID[i];
		mpEditPoly->mm.v[j] = vertsAfter[i];
		mpEditPoly->mm.vfac[j] = vfacAfter[i];
		mpEditPoly->mm.vedg[j] = vedgAfter[i];
	}
	int offset = vertID.Count() - numvBefore;
	for (int i=numvBefore; i<numvAfter; i++) {
		int j = i+offset;
		mpEditPoly->mm.v[i] = vertsAfter[j];
		mpEditPoly->mm.vfac[i] = vfacAfter[j];
		mpEditPoly->mm.vedg[i] = vedgAfter[j];
	}

	// Redo the vertex data:
	for (int dataChannel=0; dataChannel<vertexDataCount; dataChannel++) {
		if (dataChannel == VDATA_SELECT) continue;
		if (vertexDataAfter[dataChannel].Count() == 0) {
			mpEditPoly->mm.setVDataSupport (dataChannel, false);
			continue;
		}
		mpEditPoly->mm.setVDataSupport (dataChannel, true);
		float *vd = mpEditPoly->mm.vertexFloat (dataChannel);
		if (vd && numvAfter) memcpy (vd, vertexDataAfter[dataChannel].Addr(0), numvAfter * sizeof(float));
	}

	// Redo edge changes:
	if (numeBefore != numeAfter) mpEditPoly->mm.setNumEdges (numeAfter);
	for (int i=0; i<edgeID.Count (); i++) mpEditPoly->mm.e[edgeID[i]] = edgesAfter[i];
	offset = edgeID.Count() - numeBefore;
	for (int i=numeBefore; i<numeAfter; i++) mpEditPoly->mm.e[i] = edgesAfter[i+offset];

	// Redo the edge data:
	for (int dataChannel=0; dataChannel<edgeDataCount; dataChannel++) {
		if (edgeDataAfter[dataChannel].Count() == 0) {
			mpEditPoly->mm.setEDataSupport (dataChannel, false);
			continue;
		}
		mpEditPoly->mm.setEDataSupport (dataChannel, true);
		float *ed = mpEditPoly->mm.edgeFloat (dataChannel);
		if (ed && numeAfter) memcpy (ed, edgeDataAfter[dataChannel].Addr(0), numeAfter * sizeof(float));
	}

	// Redo face changes:
	if (numfBefore != numfAfter) mpEditPoly->mm.setNumFaces (numfAfter);
	for (int i=0; i<faceID.Count (); i++) {
		int j=faceID[i];
		mpEditPoly->mm.f[j] = facesAfter[i];
	}
	offset = faceID.Count() - numfBefore;
	for (int i=numfBefore; i<numfAfter; i++) {
		mpEditPoly->mm.f[i] = facesAfter[i+offset];
	}

	// Redo specified normal changes:
	MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals();
	if (mNormAfter)
	{
		if (pNorm == NULL)
		{
			mpEditPoly->mm.SpecifyNormals ();
			pNorm = mpEditPoly->mm.GetSpecifiedNormals();
		}
		pNorm->SetParent(&mpEditPoly->mm);
		*pNorm = *mNormAfter;
		pNorm->ComputeNormals();
	}

	// Redo map changes:
	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		int offsetMapChannel = NUM_HIDDENMAPS + mapChannel;
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;

		// Map Vertices:
		if (numMvBefore[offsetMapChannel] != numMvAfter[offsetMapChannel]) {
			mpEditPoly->mm.M(mapChannel)->setNumVerts (numMvAfter[offsetMapChannel]);
		}
		for (int i=0; i<mapVertID[offsetMapChannel].Count (); i++) {
			int j=mapVertID[offsetMapChannel][i];
			mpEditPoly->mm.M(mapChannel)->v[j] = mapVertsAfter[offsetMapChannel][i];
		}
		int offset = mapVertID[offsetMapChannel].Count() - numMvBefore[offsetMapChannel];
		for (int i=numMvBefore[offsetMapChannel]; i<numMvAfter[offsetMapChannel]; i++) {
			mpEditPoly->mm.M(mapChannel)->v[i] = mapVertsAfter[offsetMapChannel][i+offset];
		}

		// Map Faces:
		if (numfBefore != numfAfter) mpEditPoly->mm.M(mapChannel)->setNumFaces (numfAfter);
		for (int i=0; i<mapFaceID[offsetMapChannel].Count(); i++) {
			mpEditPoly->mm.M(mapChannel)->f[mapFaceID[offsetMapChannel][i]] = mapFacesAfter[offsetMapChannel][i];
		}
		offset = mapFaceID[offsetMapChannel].Count() - numfBefore;
		for (int i=numfBefore; i<numfAfter; i++) {
			mpEditPoly->mm.M(mapChannel)->f[i].Init ();
			mpEditPoly->mm.M(mapChannel)->f[i] = mapFacesAfter[offsetMapChannel][i+offset];
		}
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the after interfaces into the mesh's
	mTopoPipelineRestore.RestoreAfter(mpEditPoly->mm);

	DbgAssert (mpEditPoly->mm.CheckAllData ());
	mpEditPoly->LocalDataChanged (ALL_CHANNELS - SUBSEL_TYPE_CHANNEL);
}

void TopoChangeRestore::MyDebugPrint ()
{
	DebugPrint(_T("\nTopological change restore record:\n"));

	if (vertID.Count() > 0)
	{
		DebugPrint(_T("%d changed vertices:\n"), vertID.Count());
		for (int i=0; i<vertID.Count(); i++)
		{
			DebugPrint(_T("  Vertex %d - Before (%5.2f, %5.2f, %5.2f), After (%5.2f, %5.2f, %5.2f)\n"),
				vertID[i], vertsBefore[i].p.x, vertsBefore[i].p.y, vertsBefore[i].p.z,
				vertsAfter[i].p.x, vertsAfter[i].p.y, vertsAfter[i].p.z);
			// TODO: Maybe later add vfacBefore, vfacAfter, vedgBefore, vedgAfter to output.
		}
	}
	if (vertsBefore.Count() > vertID.Count())
	{
		DebugPrint(_T("%d vertices removed:\n"), vertsBefore.Count() - vertID.Count());
		int offset = numvAfter - vertID.Count();
		for (int i=vertID.Count(); i<vertsBefore.Count(); i++)
		{
			DebugPrint(_T("  Vertex %d - (%5.2f, %5.2f, %5.2f)\n"), i+offset,
				vertsBefore[i].p.x, vertsBefore[i].p.y, vertsBefore[i].p.z);
			// TODO: Maybe later add vfacBefore, vedgBefore to output.
		}
	}
	if (vertsAfter.Count() > vertID.Count())
	{
		DebugPrint(_T("%d vertices added:\n"), vertsAfter.Count() - vertID.Count());
		int offset = numvBefore - vertID.Count();
		for (int i=vertID.Count(); i<vertsAfter.Count(); i++)
		{
			DebugPrint(_T("  Vertex %d - (%5.2f, %5.2f, %5.2f)\n"), i+offset,
				vertsAfter[i].p.x, vertsAfter[i].p.y, vertsAfter[i].p.z);
			// TODO: Maybe later add vfacAfter, vedgAfter to output.
		}
	}

	// TODO: Edges?

	if (faceID.Count() > 0)
	{
		DebugPrint(_T("%d changed faces:\n"), faceID.Count());
		for (int i=0; i<faceID.Count(); i++)
		{
			DebugPrint(_T("  Face %d - Before:\n"), faceID[i]);
			facesBefore[i].MNDebugPrint (false);
			DebugPrint(_T("  Face %d - After:\n"), faceID[i]);
			facesAfter[i].MNDebugPrint (false);
		}
	}

	if (numfBefore > numfAfter)
	{
		DebugPrint(_T("%d faces removed:\n"), numfBefore - numfAfter);
		int offset = numfAfter - faceID.Count();
		for (int i=faceID.Count(); i<facesBefore.Count(); i++)
		{
			DebugPrint(_T("  Face %d - Before:\n"), i+offset);
			facesBefore[i].MNDebugPrint (false);
		}
	}

	if (numfAfter > numfBefore)
	{
		DebugPrint(_T("%d faces added:\n"), numfAfter - numfBefore);
		int offset = numfBefore - faceID.Count();
		for (int i=faceID.Count(); i<facesAfter.Count(); i++)
		{
			DebugPrint(_T("  Face %d - After:\n"), i+offset);
			facesAfter[i].MNDebugPrint (false);
		}
	}

	// TODO: Texture maps?  Vertex and Edge Data?

}

MapChangeRestore::MapChangeRestore (EditPolyObject *e, int mapChannel) : mpEditPoly(e), mMapChannel(mapChannel), mAfterCalled(false) {
	if ((mpEditPoly->mm.numm <= mMapChannel) || (mpEditPoly->mm.M(mMapChannel)->GetFlag (MN_DEAD))) {
		preVerts.SetCount (0);
		preFaces.SetCount (0);
		mapFlagsBefore = MN_DEAD;
	} else {
		preVerts.SetCount (mpEditPoly->mm.M(mMapChannel)->numv);
		preFaces.SetCount (mpEditPoly->mm.numf);
		if (mpEditPoly->mm.M(mMapChannel)->numv) memcpy (preVerts.Addr(0), mpEditPoly->mm.M(mMapChannel)->v, mpEditPoly->mm.M(mMapChannel)->numv * sizeof(UVVert));
		for (int i=0; i<mpEditPoly->mm.numf; i++) {
			preFaces[i].Init ();
			preFaces[i] = mpEditPoly->mm.M(mMapChannel)->f[i];
		}
		mapFlagsBefore = mpEditPoly->mm.M(mMapChannel)->ExportFlags ();
	}
}

MapChangeRestore::~MapChangeRestore () {
	for (int i=0; i<preFaces.Count(); i++) preFaces[i].Clear ();
	for (int i=0; i<fbefore.Count(); i++) fbefore[i].Clear ();
	for (int i=0; i<fafter.Count(); i++) fafter[i].Clear ();
}

bool MapChangeRestore::After () {
	mAfterCalled = true;
	numvBefore = preVerts.Count();
	numvAfter = mpEditPoly->mm.M(mMapChannel)->numv;
	numfBefore = preFaces.Count();
	numfAfter = mpEditPoly->mm.M(mMapChannel)->numf;
	mapFlagsAfter = mpEditPoly->mm.M(mMapChannel)->ExportFlags ();

	// Record vertex changes:
	int i, minv = (numvBefore < numvAfter) ? numvBefore : numvAfter;
	for (i=0; i<minv; i++) {
		if (preVerts[i] == mpEditPoly->mm.M(mMapChannel)->v[i]) continue;
		// Something's changed:
		vertID.Append (1, &i, minv/10);
	}
	vbefore.SetCount (vertID.Count()+numvBefore-minv);
	vafter.SetCount (vertID.Count()+numvAfter-minv);
	int ct=vertID.Count();

	for (i=0; i<ct; i++) {
		int j = vertID[i];
		vbefore[i] = preVerts[j];
		vafter[i] = mpEditPoly->mm.M(mMapChannel)->v[j];
	}
	for (i=minv; i<numvBefore; i++) {
		int j = i-minv+ct;
		vbefore[j] = preVerts[i];
	}
	for (i=minv; i<numvAfter; i++) {
		int j = i-minv+ct;
		vafter[j] = mpEditPoly->mm.M(mMapChannel)->v[i];
	}

	// Record Face changes:
	int minf = mpEditPoly->mm.numf;
	if (numfBefore < minf) minf = numfBefore;
	for (i=0; i<minf; i++) {
		if (preFaces[i] == mpEditPoly->mm.M(mMapChannel)->f[i]) continue;
		// Something's changed:
		faceID.Append (1, &i, minf/10);
	}

	ct=faceID.Count ();
	fbefore.SetCount (ct + numfBefore-minf);
	fafter.SetCount (ct + numfAfter-minf);

	for (i=0; i<ct; i++) {
		int j = faceID[i];
		fbefore[i].Init ();
		fafter[i].Init ();
		fbefore[i] = preFaces[j];
		fafter[i] = mpEditPoly->mm.M(mMapChannel)->f[j];
	}
	for (i=minf; i<numfBefore; i++) {
		int j = i-minf+ct;
		fbefore[j].Init ();
		fbefore[j] = preFaces[i];
	}
	for (i=minf; i<numfAfter; i++) {
		int j = i-minf+ct;
		fafter[j].Init ();
		fafter[j] = mpEditPoly->mm.M(mMapChannel)->f[i];
	}

	for (i=0; i<preFaces.Count(); i++) preFaces[i].Clear();
	preFaces.ZeroCount ();
	preFaces.Shrink ();
	preVerts.ZeroCount ();
	preVerts.Shrink ();

	if (mapFlagsAfter != mapFlagsAfter) return true;
	if (numfAfter != numfBefore) return true;
	if (numvAfter != numvBefore) return true;
	if (vbefore.Count() || vafter.Count() || fbefore.Count() || fafter.Count()) return true;
	// no change whatsoever.
	return false;
}

void MapChangeRestore::Restore (int isUndo) {
	if (!mAfterCalled) After ();

	// Undo vertex changes:
	if (numvBefore != numvAfter) mpEditPoly->mm.M(mMapChannel)->setNumVerts (numvBefore);
	for (int i=0; i<vertID.Count (); i++) {
		mpEditPoly->mm.M(mMapChannel)->v[vertID[i]] = vbefore[i];
	}
	int offset = vertID.Count() - numvAfter;
	for (int i=numvAfter; i<numvBefore; i++) {
		mpEditPoly->mm.M(mMapChannel)->v[i] = vbefore[i+offset];
	}

	// Undo face changes:
	if (numfBefore != numfAfter) mpEditPoly->mm.M(mMapChannel)->setNumFaces (numfBefore);
	for (int i=0; i<faceID.Count(); i++) {
		mpEditPoly->mm.M(mMapChannel)->f[faceID[i]] = fbefore[i];
	}
	offset = faceID.Count() - numfAfter;
	for (int i=numfAfter; i<numfBefore; i++) {
		mpEditPoly->mm.M(mMapChannel)->f[i] = fbefore[i+offset];
	}

	// Set flags to earlier state:
	mpEditPoly->mm.M(mMapChannel)->ImportFlags (mapFlagsBefore);

	mpEditPoly->LocalDataChanged(VERTCOLOR_CHANNEL);
}

void MapChangeRestore::Redo () {
	// Redo vertex changes:
	if (numvBefore != numvAfter) mpEditPoly->mm.M(mMapChannel)->setNumVerts (numvAfter);
	for (int i=0; i<vertID.Count (); i++) {
		mpEditPoly->mm.M(mMapChannel)->v[vertID[i]] = vafter[i];
	}
	int offset = vertID.Count() - numvBefore;
	for (int i=numvBefore; i<numvAfter; i++) {
		mpEditPoly->mm.M(mMapChannel)->v[i] = vafter[i+offset];
	}

	// Undo face changes:
	if (numfBefore != numfAfter) mpEditPoly->mm.M(mMapChannel)->setNumFaces (numfAfter);
	for (int i=0; i<faceID.Count(); i++) {
		mpEditPoly->mm.M(mMapChannel)->f[faceID[i]] = fafter[i];
	}
	offset = faceID.Count() - numfBefore;
	for (int i=numfBefore; i<numfAfter; i++) {
		mpEditPoly->mm.M(mMapChannel)->f[i] = fafter[i+offset];
	}

	// Set flags to earlier state:
	mpEditPoly->mm.M(mMapChannel)->ImportFlags (mapFlagsAfter);

	mpEditPoly->LocalDataChanged(VERTCOLOR_CHANNEL);
}

//---------------------------------------------------

MapVertRestore::MapVertRestore (EditPolyObject *e, int mapchannel) : mpEditPoly(e), mMapChannel(mapchannel), mAfterCalled(false) {
	if ((mpEditPoly->mm.numm <= mMapChannel) || (mpEditPoly->mm.M(mMapChannel)->GetFlag (MN_DEAD))) {
		preVerts.SetCount (0);
	} else {
		preVerts.SetCount (mpEditPoly->mm.M(mMapChannel)->numv);
		if (mpEditPoly->mm.M(mMapChannel)->numv)
			memcpy (preVerts.Addr(0), mpEditPoly->mm.M(mMapChannel)->v, mpEditPoly->mm.M(mMapChannel)->numv * sizeof(UVVert));
	}
}

bool MapVertRestore::After () {
	mAfterCalled = true;
	int numv = mpEditPoly->mm.M(mMapChannel)->numv;

	// Record vertex changes:
	vertID.ZeroCount();
	for (int i=0; i<numv; i++) {
		if (preVerts[i] == mpEditPoly->mm.M(mMapChannel)->v[i]) continue;
		// Something's changed:
		vertID.Append (1, &i, numv/10);
	}
	int ct=vertID.Count();
	vbefore.SetCount (ct);
	vafter.SetCount (ct);

	for (int i=0; i<ct; i++) {
		int j = vertID[i];
		vbefore[i] = preVerts[j];
		vafter[i] = mpEditPoly->mm.M(mMapChannel)->v[j];
	}

	// Clear out preVerts array
	preVerts.ZeroCount ();
	preVerts.Shrink ();

	if (ct) return true;
	// no change whatsoever.
	return false;
}

void MapVertRestore::Restore (int isUndo) {
	if (!mAfterCalled) After ();
	// Undo vertex changes:
	for (int i=0; i<vertID.Count (); i++) {
		mpEditPoly->mm.M(mMapChannel)->v[vertID[i]] = vbefore[i];
	}
	mpEditPoly->LocalDataChanged ((mMapChannel>0) ? TEXMAP_CHANNEL : VERTCOLOR_CHANNEL);
}

void MapVertRestore::Redo () {
	for (int i=0; i<vertID.Count (); i++) {
		mpEditPoly->mm.M(mMapChannel)->v[vertID[i]] = vafter[i];
	}
	mpEditPoly->LocalDataChanged ((mMapChannel>0) ? TEXMAP_CHANNEL : VERTCOLOR_CHANNEL);
}

//---------------------------------------------------

CreateOnlyRestore::CreateOnlyRestore (EditPolyObject *e) {
	mpEditPoly = e;
	ovnum = mpEditPoly->mm.numv;
	oenum = mpEditPoly->mm.nume;
	ofnum = mpEditPoly->mm.numf;
	omvnum.SetCount (mpEditPoly->mm.numm + NUM_HIDDENMAPS);
	omfnum.SetCount (mpEditPoly->mm.numm + NUM_HIDDENMAPS);
	omapsUsed.SetSize (mpEditPoly->mm.numm + NUM_HIDDENMAPS);
	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		int offsetMapChannel = mapChannel + NUM_HIDDENMAPS;
		MNMap *map = mpEditPoly->mm.M(mapChannel);
		if (map->GetFlag (MN_DEAD)) {
			omvnum[offsetMapChannel] = 0;
			omfnum[offsetMapChannel] = 0;
			omapsUsed.Clear (offsetMapChannel);
		} else {
			omvnum[offsetMapChannel] = map->numv;
			omfnum[offsetMapChannel] = map->numf;
			omapsUsed.Set (offsetMapChannel);
		}
	}
	nvfac = NULL;
	nvedg = NULL;
	nmverts = NULL;
	nmfaces = NULL;
	afterCalled = false;

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Make copies of the before TOPO channels for use during undo
	mTopoPipelineRestore.Before(mpEditPoly->mm);
}

CreateOnlyRestore::~CreateOnlyRestore () {
	if (nvfac) delete [] nvfac;
	if (nvedg) delete [] nvedg;
	if (nmverts) delete [] nmverts;
	if (nmfaces) delete [] nmfaces;
}

void CreateOnlyRestore::After () {
	int nvnum = mpEditPoly->mm.numv - ovnum;
	if (nvnum>0) {
		nverts.ZeroCount ();
		nverts.Append (nvnum, mpEditPoly->mm.V(ovnum));
		if (!nvfac) nvfac = new Tab<int>[nvnum];
		if (!nvedg) nvedg = new Tab<int>[nvnum];
		for (int i=0; i<nvnum; i++) {
			nvfac[i] = mpEditPoly->mm.vfac[i+ovnum];
			nvedg[i] = mpEditPoly->mm.vedg[i+ovnum];
		}
	}

	int nenum = mpEditPoly->mm.nume - oenum;
	if (nenum>0) {
		nedges.ZeroCount ();
		nedges.Append (nenum, mpEditPoly->mm.E(oenum));
	}

	int nfnum = mpEditPoly->mm.numf - ofnum;
	if (nfnum>0) {
		nfaces.ZeroCount ();
		nfaces.Resize (nfnum);
		MNFace temp;
		temp.Init();
		for (int i=0; i<nfnum; i++) {
			nfaces.Append (1, &temp);
			nfaces[i] = mpEditPoly->mm.f[i+ofnum];
		}

		MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals();
		if (pNorm)
		{
			nNormals = *pNorm;
		}
	}

	int mapChannel;
	if (mpEditPoly->mm.numm + NUM_HIDDENMAPS > omvnum.Count()) {
		int oldnumm = omvnum.Count();
		omvnum.SetCount (mpEditPoly->mm.numm + NUM_HIDDENMAPS);
		omfnum.SetCount (mpEditPoly->mm.numm + NUM_HIDDENMAPS);
		omapsUsed.SetSize (mpEditPoly->mm.numm + NUM_HIDDENMAPS, true);
		for (int offsetMapChannel=oldnumm; offsetMapChannel<mpEditPoly->mm.numm + NUM_HIDDENMAPS; offsetMapChannel++) {
			omvnum[offsetMapChannel] = 0;
			omfnum[offsetMapChannel] = 0;
			omapsUsed.Clear (offsetMapChannel);
		}
	}
	nmapsUsed.SetSize (mpEditPoly->mm.numm + NUM_HIDDENMAPS);
	if (!nmverts) nmverts = new Tab<UVVert>[mpEditPoly->mm.numm + NUM_HIDDENMAPS];
	if (!nmfaces) nmfaces = new Tab<MNMapFace>[mpEditPoly->mm.numm + NUM_HIDDENMAPS];
	for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		int offsetMapChannel = NUM_HIDDENMAPS + mapChannel;
		MNMap *map = mpEditPoly->mm.M(mapChannel);
		nmapsUsed.Set (offsetMapChannel, !map->GetFlag (MN_DEAD));
		if (map->GetFlag (MN_DEAD)) continue;
		int nmvnum = map->numv - omvnum[offsetMapChannel];
		if (nmvnum>0) {
			nmverts[offsetMapChannel].SetCount (nmvnum);
			memcpy (nmverts[offsetMapChannel].Addr(0), (void*)(map->v+omvnum[offsetMapChannel]), nmvnum*sizeof(UVVert));
		}

		int nmfnum = map->numf - omfnum[offsetMapChannel];
		if (nmfnum>0) {
			nmfaces[offsetMapChannel].SetCount (nmfnum);
			for (int i=0; i<nmfnum; i++) {
				nmfaces[offsetMapChannel][i].Init ();
				nmfaces[offsetMapChannel][i] = map->f[i+omfnum[offsetMapChannel]];
			}
		}
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Make copies of the after TOPO channels for use during redo.
	mTopoPipelineRestore.After(mpEditPoly->mm);

	afterCalled = true;
}

void CreateOnlyRestore::Restore (BOOL isUndo) {
	if (isUndo && !afterCalled) After ();

	// Simplest possible deletion of new components:
	mpEditPoly->mm.setNumVerts (ovnum);
	mpEditPoly->mm.setNumEdges (oenum);
	mpEditPoly->mm.setNumFaces (ofnum);
	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		int offsetMapChannel = mapChannel + NUM_HIDDENMAPS;
		mpEditPoly->mm.M(mapChannel)->setNumFaces (omfnum[offsetMapChannel]);
		mpEditPoly->mm.M(mapChannel)->setNumVerts (omvnum[offsetMapChannel]);
		if (!omapsUsed[offsetMapChannel]) mpEditPoly->mm.M(mapChannel)->SetFlag (MN_DEAD);
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the before interfaces into the mesh's
	mTopoPipelineRestore.RestoreBefore(mpEditPoly->mm);

	mpEditPoly->LocalDataChanged (TOPO_CHANNEL|GEOM_CHANNEL|SELECT_CHANNEL);
	DbgAssert (mpEditPoly->mm.CheckAllData ());
}

void CreateOnlyRestore::Redo () {
	// Put back the new components:
	int i;
	if (nverts.Count()) {
		mpEditPoly->mm.setNumVerts (ovnum + nverts.Count());
		memcpy (mpEditPoly->mm.V(ovnum), nverts.Addr(0), nverts.Count()*sizeof(MNVert));
		for (i=0; i<nverts.Count(); i++) {
			mpEditPoly->mm.vedg[i+ovnum] = nvedg[i];
			mpEditPoly->mm.vfac[i+ovnum] = nvfac[i];
		}
	}

	if (nedges.Count()) {
		mpEditPoly->mm.setNumEdges (oenum + nedges.Count());
		memcpy (mpEditPoly->mm.E(oenum), nedges.Addr(0), nedges.Count()*sizeof(MNEdge));
	}

	if (nfaces.Count()) {
		mpEditPoly->mm.setNumFaces (ofnum + nfaces.Count());
		for (i=0; i<nfaces.Count(); i++) {
			mpEditPoly->mm.f[i+ofnum] = nfaces[i];
		}

		MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals ();
		if (pNorm)
		{
			pNorm->operator =( nNormals );
		}
	}

	int mapChannel;
	for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		int offsetMapChannel = NUM_HIDDENMAPS + mapChannel;
		if (!nmapsUsed[offsetMapChannel]) continue;
		MNMap *map = mpEditPoly->mm.M(mapChannel);
		map->ClearFlag (MN_DEAD);
		map->setNumFaces (omfnum[offsetMapChannel] + nmfaces[offsetMapChannel].Count());
		map->setNumVerts (omvnum[offsetMapChannel] + nmverts[offsetMapChannel].Count());
		for (i=0; i<nmfaces[offsetMapChannel].Count(); i++) {
			map->f[i+omfnum[offsetMapChannel]].Init ();
			map->f[i+omfnum[offsetMapChannel]] = nmfaces[offsetMapChannel][i];
		}
		if (nmverts[offsetMapChannel].Count()) memcpy ((void *)(map->v + omvnum[offsetMapChannel]), nmverts[offsetMapChannel].Addr(0), nmverts[offsetMapChannel].Count()*sizeof(UVVert));
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the after interfaces into the mesh's
	mTopoPipelineRestore.RestoreAfter(mpEditPoly->mm);

	mpEditPoly->LocalDataChanged (TOPO_CHANNEL|GEOM_CHANNEL|SELECT_CHANNEL);
	DbgAssert (mpEditPoly->mm.CheckAllData ());
}

MeshVertRestore::MeshVertRestore(EditPolyObject *e) {
	mpEditPoly = e;
	undo.SetCount(mpEditPoly->mm.numv);
	for (int i=0; i<mpEditPoly->mm.numv; i++) undo[i] = mpEditPoly->mm.v[i].p;
}

void MeshVertRestore::Restore(int isUndo) {
	if (undo.Count() != mpEditPoly->mm.numv)
	{
		DbgAssert(0 && _T("Vertex undo size mismatch"));
		return;
	}
	int i;
	if (isUndo) {
		redo.SetCount(mpEditPoly->mm.numv);
		for (i=0; i<mpEditPoly->mm.numv; i++) redo[i] = mpEditPoly->mm.v[i].p;
	}

	for (i=0; i<mpEditPoly->mm.numv; i++) 
	{
		mpEditPoly->mm.v[i].p = undo[i];
	}
	mpEditPoly->mm.SetFlag(MN_CACHEINVALID);
	mpEditPoly->LocalDataChanged(GEOM_CHANNEL);
}

void MeshVertRestore::Redo() {
	if (redo.Count() != mpEditPoly->mm.numv)
	{
		DbgAssert(0 && _T("Vertex redo size mismatch"));
		return;
	}
	for (int i=0; i<mpEditPoly->mm.numv; i++)
	{
		mpEditPoly->mm.v[i].p = redo[i];
	}

	mpEditPoly->mm.SetFlag(MN_CACHEINVALID);
	mpEditPoly->LocalDataChanged(GEOM_CHANNEL);
}

MtlIDRestore::MtlIDRestore (EditPolyObject *e) {
	mpEditPoly = e;
	undo.SetCount(mpEditPoly->mm.numf);
	for (int i=0; i<mpEditPoly->mm.numf; i++) undo[i] = mpEditPoly->mm.f[i].material;
}

void MtlIDRestore::After () {
	redo.SetCount(mpEditPoly->mm.numf);
	for (int i=0; i<mpEditPoly->mm.numf; i++) redo[i] = mpEditPoly->mm.f[i].material;
}

void MtlIDRestore::Restore(int isUndo) {
	if (redo.Count() == 0) After ();
	for (int i=0; i<mpEditPoly->mm.numf; i++) mpEditPoly->mm.f[i].material = undo[i];
	mpEditPoly->InvalidateSurfaceUI();
	mpEditPoly->NotifyDependents(FOREVER, PART_TOPO, REFMSG_CHANGE);
}

void MtlIDRestore::Redo() {
	for (int i=0; i<mpEditPoly->mm.numf; i++) mpEditPoly->mm.f[i].material = redo[i];
	mpEditPoly->InvalidateSurfaceUI();
	mpEditPoly->NotifyDependents(FOREVER, PART_TOPO, REFMSG_CHANGE);
}

SmGroupRestore::SmGroupRestore (EditPolyObject *e) {
	mpEditPoly = e;
	osmg.SetCount (mpEditPoly->mm.numf);
	for (int i=0; i<mpEditPoly->mm.numf; i++) osmg[i] = mpEditPoly->mm.f[i].smGroup;
}

void SmGroupRestore::After () {
	nsmg.SetCount (mpEditPoly->mm.numf);
	for (int i=0; i<mpEditPoly->mm.numf; i++) nsmg[i] = mpEditPoly->mm.f[i].smGroup;
}

void SmGroupRestore::Restore (int isUndo) {
	if (!nsmg.Count ()) After ();
	int i, min = mpEditPoly->mm.numf;
	if (osmg.Count() < min) min = osmg.Count ();
	for (i=0; i<min; i++) mpEditPoly->mm.f[i].smGroup = osmg[i];
	mpEditPoly->LocalDataChanged(TOPO_CHANNEL);
}

void SmGroupRestore::Redo () {
	int i, min = mpEditPoly->mm.numf;
	if (nsmg.Count() < min) min = nsmg.Count ();
	for (i=0; i<min; i++) mpEditPoly->mm.f[i].smGroup = nsmg[i];
	mpEditPoly->LocalDataChanged(TOPO_CHANNEL);
}

CollapseDeadVertsRestore::CollapseDeadVertsRestore (EditPolyObject *e) {
	undovedg = NULL;
	undovfac = NULL;
	mpEditPoly = e;
	startNum = mpEditPoly->mm.numv;
	int allocAmt = mpEditPoly->mm.numv/10;
	int ct=0;
	for (int i=0; i<mpEditPoly->mm.numv; i++) {
		if (!mpEditPoly->mm.v[i].GetFlag (MN_DEAD)) continue;
		positions.Append (1, &i, allocAmt);
		ct++;
	}
	if (!ct) return;
	undo.SetCount (ct);
	undovedg = new Tab<int>[ct];
	undovfac = new Tab<int>[ct];
	for (int i=0; i<ct; i++) {
		undo[i] = mpEditPoly->mm.v[positions[i]];
		undovedg[i] = mpEditPoly->mm.vedg[positions[i]];
		undovfac[i] = mpEditPoly->mm.vfac[positions[i]];
	}
	ucont = mpEditPoly->cont;
}

void CollapseDeadVertsRestore::Restore (int isUndo) {
#ifdef EPOLY_RESTORE_DEBUG_PRINT
	MyDebugPrint ();
	DebugPrint(_T("Restoring CollapseDeadVerts - starting mesh:\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	int i, j, k, ct = positions.Count();
	if (!ct) return;
	int newNumV = mpEditPoly->mm.numv + ct;

	if (rcont.Count() < mpEditPoly->cont.Count()) rcont = mpEditPoly->cont;

	// Figure out the renumbering scheme for non-deleted faces:
	Tab<int> renum;
	renum.SetCount (mpEditPoly->mm.numv);
	for (i=0, j=0, k=0; i<mpEditPoly->mm.numv; i++) {
		while ((j<ct) && (positions[j] == k)) {
			// leave room for a deleted face:
			k++;
			j++;
		}
		renum[i] = k;
		k++;
	}
	DbgAssert (k+(ct-j) == newNumV);

	// Correct vertex references in edges, faces:
	DbgAssert (mpEditPoly->mm.GetFlag (MN_MESH_FILLED_IN));

	int renumSize = renum.Count();
	for (i=0; i<mpEditPoly->mm.nume; i++) {
		if (mpEditPoly->mm.e[i].GetFlag (MN_DEAD)) continue;
		// Check these for possible out-of-bounds values (can happen if CheckAllData above shows errors)
		if (mpEditPoly->mm.e[i].v1 >= renumSize || mpEditPoly->mm.e[i].v2 >= renumSize)
		{
			DbgAssert(0);
		}
		else
		{
			mpEditPoly->mm.e[i].v1 = renum[mpEditPoly->mm.e[i].v1];
			mpEditPoly->mm.e[i].v2 = renum[mpEditPoly->mm.e[i].v2];
		}
	}

	for (i=0; i<mpEditPoly->mm.numf; i++) {
		if (mpEditPoly->mm.f[i].GetFlag (MN_DEAD)) continue;
		for (j = 0; j < mpEditPoly->mm.f[i].deg; j++)
		{
			// Check for possible out-of-bounds values
			if (mpEditPoly->mm.f[i].vtx[j] >= renumSize)
			{
				DbgAssert(0);
			}
			else
			{
				mpEditPoly->mm.f[i].vtx[j] = renum[mpEditPoly->mm.f[i].vtx[j]];
			}
		}
	}

	// Insert room for deleted vertices:
	int oldNumV = mpEditPoly->mm.numv;
	mpEditPoly->mm.setNumVerts (newNumV);
	for (i=oldNumV-1; i>=0; i--) {
		if (renum[i]==i) continue;
		mpEditPoly->mm.v[renum[i]] = mpEditPoly->mm.v[i];
		mpEditPoly->mm.vfac[renum[i]] = mpEditPoly->mm.vfac[i];
		mpEditPoly->mm.vedg[renum[i]] = mpEditPoly->mm.vedg[i];
	}

	// Replace deleted vertices:
	for (i=0; i<ct; i++) {
		mpEditPoly->mm.v[positions[i]] = undo[i];
		mpEditPoly->mm.vfac[positions[i]] = undovfac[i];
		mpEditPoly->mm.vedg[positions[i]] = undovedg[i];
	}

#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Restoring CollapseDeadVerts - ending mesh:\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif

	mpEditPoly->ReplaceContArray (ucont);
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	mpEditPoly->LocalDataChanged(TOPO_CHANNEL|SELECT_CHANNEL);
}

void CollapseDeadVertsRestore::Redo () {
#ifdef EPOLY_RESTORE_DEBUG_PRINT
	MyDebugPrint ();
	DebugPrint(_T("Redoing CollapseDeadVerts - starting mesh:\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif
	int i, j, k, ct = positions.Count();
	if (!ct) return;
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	int newNumV = mpEditPoly->mm.numv - ct;
	int oldNumV = mpEditPoly->mm.numv;

	// Figure out the renumbering scheme for non-deleted faces:
	Tab<int> renum;
	renum.SetCount (mpEditPoly->mm.numv);
	for (i=0, j=0, k=0; i<mpEditPoly->mm.numv; i++) {
		if ((j<ct) && (positions[j] == i)) {
			// this face will be deleted.
			renum[i] = -1;
			j++;
			continue;
		}
		renum[i] = k;
		k++;
	}
	DbgAssert (k == newNumV);

	// Correct vertex references in edges, faces:
	DbgAssert (mpEditPoly->mm.GetFlag (MN_MESH_FILLED_IN));

	for (i=0; i<mpEditPoly->mm.nume; i++) {
		if (mpEditPoly->mm.e[i].GetFlag (MN_DEAD)) continue;
		mpEditPoly->mm.e[i].v1 = renum[mpEditPoly->mm.e[i].v1];
		mpEditPoly->mm.e[i].v2 = renum[mpEditPoly->mm.e[i].v2];
	}

	for (i=0; i<mpEditPoly->mm.numf; i++) {
		if (mpEditPoly->mm.f[i].GetFlag (MN_DEAD)) continue;
		for (j=0; j<mpEditPoly->mm.f[i].deg; j++) mpEditPoly->mm.f[i].vtx[j] = renum[mpEditPoly->mm.f[i].vtx[j]];
	}

	// Remove deleted vertices:
	for (i=0; i<oldNumV; i++) {
		if (renum[i] == i) continue;
		if (renum[i] == -1) continue;
		mpEditPoly->mm.v[renum[i]] = mpEditPoly->mm.v[i];
		mpEditPoly->mm.vfac[renum[i]] = mpEditPoly->mm.vfac[i];
		mpEditPoly->mm.vedg[renum[i]] = mpEditPoly->mm.vedg[i];
	}

	mpEditPoly->mm.setNumVerts (newNumV);

#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Redoing CollapseDeadVerts - ending mesh:\n"));
	mpEditPoly->mm.MNDebugPrint ();
#endif

	mpEditPoly->ReplaceContArray (rcont);
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	mpEditPoly->LocalDataChanged(TOPO_CHANNEL|SELECT_CHANNEL);
}

void CollapseDeadVertsRestore::MyDebugPrint () {
	int i, ct = positions.Count();
	DebugPrint(_T("CollapseDeadVertsRestore Debug Printout:\n"));
	DebugPrint(_T("  Before: %d vertices.  After: %d vertices.  %d vertices culled.\n"), startNum, startNum-ct, ct);

	for (i=0; i<ct; i++) {
		DebugPrint(_T("%d  "), positions[i]);
		if (i%5==4) DebugPrint(_T("\n"));
	}
	if (i%5!=0) DebugPrint(_T("\n"));
}

CollapseDeadEdgesRestore::CollapseDeadEdgesRestore (EditPolyObject *e) {
	mpEditPoly = e;
	int allocAmt = mpEditPoly->mm.nume/10;
	int ct=0;
	for (int i=0; i<mpEditPoly->mm.nume; i++) {
		if (!mpEditPoly->mm.e[i].GetFlag (MN_DEAD)) continue;
		positions.Append (1, &i, allocAmt);
		ct++;
	}
	if (!ct) return;
	undo.SetCount (ct);
	for (int i=0; i<ct; i++) undo[i] = mpEditPoly->mm.e[positions[i]];
}

void CollapseDeadEdgesRestore::Restore (int isUndo) {
#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Restoring CollapseDeadEdges\n"));
#endif
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	int i, j, k, ct = positions.Count();
	if (!ct) return;
	int newNumE = mpEditPoly->mm.nume + ct;

	// Figure out the renumbering scheme for non-deleted edges:
	Tab<int> renum;
	renum.SetCount (mpEditPoly->mm.nume);
	for (i=0, j=0, k=0; i<mpEditPoly->mm.nume; i++) {
		while ((j<ct) && (positions[j] == k)) {
			// leave room for a deleted edge:
			k++;
			j++;
		}
		renum[i] = k;
		k++;
	}
	DbgAssert (k+(ct-j) == newNumE);

	// Correct edge references in faces, verts:
	DbgAssert (mpEditPoly->mm.GetFlag (MN_MESH_FILLED_IN));

	for (i=0; i<mpEditPoly->mm.numf; i++) {
		if (mpEditPoly->mm.f[i].GetFlag (MN_DEAD)) continue;
		int *ee = mpEditPoly->mm.f[i].edg;
		for (j=0; j<mpEditPoly->mm.f[i].deg; j++) ee[j] = renum[ee[j]];
	}

	for (i=0; i<mpEditPoly->mm.numv; i++) {
		if (mpEditPoly->mm.v[i].GetFlag (MN_DEAD)) continue;
		for (j=0; j<mpEditPoly->mm.vedg[i].Count(); j++) mpEditPoly->mm.vedg[i][j] = renum[mpEditPoly->mm.vedg[i][j]];
	}

	// Insert room for deleted edges:
	int oldNumE = mpEditPoly->mm.nume;
	mpEditPoly->mm.setNumEdges (newNumE);
	for (i=oldNumE-1; i>=0; i--) {
		if (renum[i]==i) continue;
		mpEditPoly->mm.e[renum[i]] = mpEditPoly->mm.e[i];
	}

	// Replace deleted edges:
	for (i=0; i<ct; i++) mpEditPoly->mm.e[positions[i]] = undo[i];

	DbgAssert (mpEditPoly->mm.CheckAllData ());
	mpEditPoly->LocalDataChanged(TOPO_CHANNEL|SELECT_CHANNEL);
}

void CollapseDeadEdgesRestore::Redo () {
	int i, j, k, ct = positions.Count();
	if (!ct) return;
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	int newNumE = mpEditPoly->mm.nume - ct;
	int oldNumE = mpEditPoly->mm.nume;

	// Figure out the renumbering scheme for non-deleted faces:
	Tab<int> renum;
	renum.SetCount (mpEditPoly->mm.nume);
	for (i=0, j=0, k=0; i<mpEditPoly->mm.nume; i++) {
		if ((j<ct) && (positions[j] == i)) {
			// this face will be deleted.
			renum[i] = -1;
			j++;
			continue;
		}
		renum[i] = k;
		k++;
	}
	DbgAssert (k == newNumE);

	// Correct edge references in faces, verts:
	DbgAssert (mpEditPoly->mm.GetFlag (MN_MESH_FILLED_IN));

	for (i=0; i<mpEditPoly->mm.numf; i++) {
		if (mpEditPoly->mm.f[i].GetFlag (MN_DEAD)) continue;
		int *ee = mpEditPoly->mm.f[i].edg;
		for (j=0; j<mpEditPoly->mm.f[i].deg; j++) ee[j] = renum[ee[j]];
	}

	for (i=0; i<mpEditPoly->mm.numv; i++) {
		if (mpEditPoly->mm.v[i].GetFlag (MN_DEAD)) continue;
		for (j=0; j<mpEditPoly->mm.vedg[i].Count(); j++) mpEditPoly->mm.vedg[i][j] = renum[mpEditPoly->mm.vedg[i][j]];
	}

	// Remove deleted faces and map faces:
	for (i=0; i<oldNumE; i++) {
		if (renum[i] == i) continue;
		if (renum[i] == -1) continue;
		mpEditPoly->mm.e[renum[i]] = mpEditPoly->mm.e[i];
	}

	mpEditPoly->mm.setNumEdges (newNumE);
	DbgAssert (mpEditPoly->mm.CheckAllData ());
	mpEditPoly->LocalDataChanged(TOPO_CHANNEL|SELECT_CHANNEL);
}

CollapseDeadFacesRestore::CollapseDeadFacesRestore (EditPolyObject *e) {
	// Always initialize variables!
	mpUndoNorm = NULL;

	mpEditPoly = e;
	int allocAmt = 1 + mpEditPoly->mm.numf/10;
	int ct=0;
	for (int i=0; i<mpEditPoly->mm.numf; i++) {
		if (!mpEditPoly->mm.f[i].GetFlag (MN_DEAD)) continue;
		positions.Append (1, &i, allocAmt);
		MNFace mntemp;
		undo.Append (1, &mntemp, allocAmt);
		undo[ct] = mpEditPoly->mm.f[i];
		ct++;
	}
	if (!ct) return;
	undoMap.SetCount (mpEditPoly->mm.numm+NUM_HIDDENMAPS);
	for (int mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		int offsetMapChannel = mapChannel + NUM_HIDDENMAPS;
		undoMap[offsetMapChannel] = NULL;
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		undoMap[offsetMapChannel] = new MNMapFace[ct];
		for (int i=0; i<ct; i++) {
			undoMap[offsetMapChannel][i].Init();
			undoMap[offsetMapChannel][i] = mpEditPoly->mm.M(mapChannel)->f[positions[i]];
		}
	}

	MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals();
	MNNormalFace *pNormFace = pNorm ? pNorm->GetFaceArray() : NULL;
	if (pNormFace)
	{
		mpUndoNorm = new MNNormalFace[ct];
		for (int i=0; i<ct; i++)
		{
			mpUndoNorm[i].Init();
			mpUndoNorm[i] = pNormFace[positions[i]];
		}
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the before data channels for use during undo.
	mTopoPipelineRestore.Before(mpEditPoly->mm);
}

CollapseDeadFacesRestore::~CollapseDeadFacesRestore () {
	for (int mapChannel=0; mapChannel<undoMap.Count(); mapChannel++) {
		if (!undoMap[mapChannel]) continue;
		for (int i=0; i<undo.Count(); i++) {
			undoMap[mapChannel][i].Clear();
		}
		delete [] undoMap[mapChannel];
	}
	for (int i=0; i<undo.Count(); i++) {
		undo[i].Clear();
	}
	if (mpUndoNorm)
	{
		for (int i=0; i<undo.Count(); i++) mpUndoNorm[i].Clear();
		delete [] mpUndoNorm;
	}
}

void CollapseDeadFacesRestore::Restore (int isUndo) {
#ifdef EPOLY_RESTORE_DEBUG_PRINT
	DebugPrint(_T("Restoring CollapseDeadFaces\n"));
#endif
	int i, j, k, ct = positions.Count();
	if (!ct) return;

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the current TOPO_CHANNELS as "after" for redo.
	mTopoPipelineRestore.After(mpEditPoly->mm);

	int newNumF = mpEditPoly->mm.numf + ct;

	// Figure out the renumbering scheme for non-deleted faces:
	Tab<int> renum;
	renum.SetCount (mpEditPoly->mm.numf);
	for (i=0, j=0, k=0; i<mpEditPoly->mm.numf; i++) {
		while ((j<ct) && (positions[j] == k)) {
			// leave room for a deleted face:
			k++;
			j++;
		}
		renum[i] = k;
		k++;
	}
	DbgAssert (k+(ct-j) == newNumF);

	// Correct face references in edges, verts:
	if (mpEditPoly->mm.GetFlag (MN_MESH_FILLED_IN)) {
		for (i=0; i<mpEditPoly->mm.nume; i++) {
			if (mpEditPoly->mm.e[i].GetFlag (MN_DEAD)) continue;
			mpEditPoly->mm.e[i].f1 = renum[mpEditPoly->mm.e[i].f1];
			if (mpEditPoly->mm.e[i].f2>-1) mpEditPoly->mm.e[i].f2 = renum[mpEditPoly->mm.e[i].f2];
		}

		for (i=0; i<mpEditPoly->mm.numv; i++) {
			if (mpEditPoly->mm.v[i].GetFlag (MN_DEAD)) continue;
			for (j=0; j<mpEditPoly->mm.vfac[i].Count(); j++) mpEditPoly->mm.vfac[i][j] = renum[mpEditPoly->mm.vfac[i][j]];
		}
	}

	// Insert room for deleted faces, map faces, normal faces:
	int mapChannel, oldNumF = mpEditPoly->mm.numf;
	mpEditPoly->mm.setNumFaces (newNumF);
	for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		mpEditPoly->mm.M(mapChannel)->setNumFaces (newNumF);
	}
	MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals();
	MNNormalFace *pNormFace = pNorm ? pNorm->GetFaceArray() : NULL;

	for (i=oldNumF-1; i>=0; i--) {
		if (renum[i]==i) continue;
		mpEditPoly->mm.f[renum[i]] = mpEditPoly->mm.f[i];
		mpEditPoly->mm.f[i].Clear ();
		for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
			if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
			mpEditPoly->mm.M(mapChannel)->f[renum[i]].Clear ();
			memcpy (mpEditPoly->mm.M(mapChannel)->F(renum[i]), mpEditPoly->mm.M(mapChannel)->F(i), sizeof(MNMapFace));
			mpEditPoly->mm.M(mapChannel)->f[i].Init();
		}
		if (pNormFace)
			pNormFace[renum[i]].ShallowTransfer (pNormFace[i]);
	}

	// Replace deleted faces, map faces:
	for (i=0; i<ct; i++) {
		mpEditPoly->mm.f[positions[i]] = undo[i];
	}
	for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		int offsetMapChannel = mapChannel + NUM_HIDDENMAPS;
		if (undoMap[offsetMapChannel] == NULL) continue;
		for (i=0; i<ct; i++) {
			mpEditPoly->mm.M(mapChannel)->f[positions[i]] = undoMap[offsetMapChannel][i];
		}
	}
	if (pNormFace && mpUndoNorm)
	{
		for (i=0; i<ct; i++) pNormFace[positions[i]] = mpUndoNorm[i];
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the before TOPO_CHANNEL interfaces into the mesh's to restore them
	mTopoPipelineRestore.RestoreBefore(mpEditPoly->mm);

	mpEditPoly->LocalDataChanged(TOPO_CHANNEL|SELECT_CHANNEL);
}

void CollapseDeadFacesRestore::Redo () {
	int i, j, k, ct = positions.Count();
	if (!ct) return;
	int newNumF = mpEditPoly->mm.numf - ct;
	int mapChannel, oldNumF = mpEditPoly->mm.numf;

	// Figure out the renumbering scheme for non-deleted faces:
	Tab<int> renum;
	renum.SetCount (mpEditPoly->mm.numf);
	for (i=0, j=0, k=0; i<mpEditPoly->mm.numf; i++) {
		if ((j<ct) && (positions[j] == i)) {
			// this face will be deleted.
			renum[i] = -1;
			j++;
			continue;
		}
		renum[i] = k;
		k++;
	}
	DbgAssert (k == newNumF);

	// Correct face references in edges, verts:
	if (mpEditPoly->mm.GetFlag (MN_MESH_FILLED_IN)) {
		for (i=0; i<mpEditPoly->mm.nume; i++) {
			if (mpEditPoly->mm.e[i].GetFlag (MN_DEAD)) continue;
			mpEditPoly->mm.e[i].f1 = renum[mpEditPoly->mm.e[i].f1];
			if (mpEditPoly->mm.e[i].f2>-1) mpEditPoly->mm.e[i].f2 = renum[mpEditPoly->mm.e[i].f2];
		}

		for (i=0; i<mpEditPoly->mm.numv; i++) {
			if (mpEditPoly->mm.v[i].GetFlag (MN_DEAD)) continue;
			for (j=0; j<mpEditPoly->mm.vfac[i].Count(); j++) mpEditPoly->mm.vfac[i][j] = renum[mpEditPoly->mm.vfac[i][j]];
		}
	}

	// Remove deleted faces, map faces, normal faces:
	MNNormalSpec *pNorm = mpEditPoly->mm.GetSpecifiedNormals();
	MNNormalFace *pNormFace = pNorm ? pNorm->GetFaceArray() : NULL;
	for (i=0; i<oldNumF; i++) {
		if (renum[i] == i) continue;
		if (renum[i] == -1) continue;
		mpEditPoly->mm.f[renum[i]] = mpEditPoly->mm.f[i];
		mpEditPoly->mm.f[i].Clear ();
		for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
			if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
			mpEditPoly->mm.M(mapChannel)->f[renum[i]].Clear ();
			memcpy (mpEditPoly->mm.M(mapChannel)->F(renum[i]), mpEditPoly->mm.M(mapChannel)->F(i), sizeof(MNMapFace));
			mpEditPoly->mm.M(mapChannel)->f[i].Init();
		}
		if (pNormFace) pNormFace[renum[i]].ShallowTransfer (pNormFace[i]);
	}

	// Correct the face count:
	mpEditPoly->mm.setNumFaces (newNumF);
	for (mapChannel=-NUM_HIDDENMAPS; mapChannel<mpEditPoly->mm.numm; mapChannel++) {
		if (mpEditPoly->mm.M(mapChannel)->GetFlag (MN_DEAD)) continue;
		mpEditPoly->mm.M(mapChannel)->setNumFaces (newNumF);
	}

	// ktong|17/2/2006|740288|Update TOPO_CHANNELS pipeline clients for undo/redo.
	// Copy the after interfaces into the mesh's
	mTopoPipelineRestore.RestoreAfter(mpEditPoly->mm);

	mpEditPoly->LocalDataChanged(TOPO_CHANNEL|SELECT_CHANNEL);
}

PerDataRestore::PerDataRestore (EditPolyObject *e, int mslevel, int chan) {
	mpEditPoly = e;
	msl = mslevel;
	channel = chan;
	float *vd=NULL;
	switch (msl) {
	case MNM_SL_VERTEX:
		vd = mpEditPoly->mm.vertexFloat (chan);
		num = mpEditPoly->mm.numv;
		break;
	case MNM_SL_EDGE:
		vd = mpEditPoly->mm.edgeFloat (chan);
		num = mpEditPoly->mm.nume;
		break;
	}
	if (!vd) return;
	oldData.SetCount (num);
	if (!num) return;
	memcpy (oldData.Addr(0), vd, num * sizeof(float));
}

void PerDataRestore::After () {
	if (!mpEditPoly) return;
	float *vd=NULL;
	switch (msl) {
	case MNM_SL_VERTEX: vd = mpEditPoly->mm.vertexFloat (channel); break;
	case MNM_SL_EDGE: vd = mpEditPoly->mm.edgeFloat (channel); break;
	}
	// Create redo data.
	newData.SetCount (num);
	if (!num) return;
	memcpy (newData.Addr(0), vd, num*sizeof(float));
}

void PerDataRestore::Restore (int isUndo) {
	if (!mpEditPoly) return;
	if (!num) return;
	float *vd=NULL;
	switch (msl) {
	case MNM_SL_VERTEX: vd = mpEditPoly->mm.vertexFloat (channel); break;
	case MNM_SL_EDGE: vd = mpEditPoly->mm.edgeFloat (channel); break;
	}
	if (vd && !newData.Count()) After ();

	if (oldData.Count()) {
		if (!vd) {
			if (msl==MNM_SL_VERTEX) {
				mpEditPoly->mm.setVDataSupport (channel, true);
				vd = mpEditPoly->mm.vertexFloat (channel);
			} else {
				mpEditPoly->mm.setEDataSupport (channel, true);
				vd = mpEditPoly->mm.edgeFloat (channel);
			}
			if (!vd) return;
		}
		memcpy (vd, oldData.Addr(0), num * sizeof(float));
	} else {
		if (msl == MNM_SL_VERTEX) {
			mpEditPoly->mm.setVDataSupport (channel, false);
		} else {
			mpEditPoly->mm.setEDataSupport (channel, false);
		}
	}

	if (mpEditPoly->ip) mpEditPoly->UpdatePerDataDisplay (mpEditPoly->ip->GetTime(), msl, channel);
	mpEditPoly->LocalDataChanged ((msl==MNM_SL_VERTEX) ? GEOM_CHANNEL : TOPO_CHANNEL);
}

void PerDataRestore::Redo () {
	if (!mpEditPoly) return;
	if (!num) return;
	if (!newData.Count()) {
		switch (msl) {
		case MNM_SL_VERTEX: mpEditPoly->mm.setVDataSupport (channel, false);
		case MNM_SL_EDGE: mpEditPoly->mm.setEDataSupport (channel, false);
		}
		return;
	}

	float *vd=NULL;
	switch (msl) {
	case MNM_SL_VERTEX: vd = mpEditPoly->mm.vertexFloat (channel); break;
	case MNM_SL_EDGE: vd = mpEditPoly->mm.edgeFloat (channel); break;
	}
	if (!vd) {
		switch (msl) {
		case MNM_SL_VERTEX:
			mpEditPoly->mm.setVDataSupport (channel, true);
			vd = mpEditPoly->mm.vertexFloat (channel);
			break;
		case MNM_SL_EDGE:
			mpEditPoly->mm.setEDataSupport (channel, true);
			vd = mpEditPoly->mm.edgeFloat (channel);
		}
	}
	if (!vd) return;

	memcpy (vd, newData.Addr(0), num * sizeof(float));

	if (mpEditPoly->ip) mpEditPoly->UpdatePerDataDisplay (mpEditPoly->ip->GetTime(), msl, channel);
	mpEditPoly->LocalDataChanged ((msl==MNM_SL_VERTEX) ? GEOM_CHANNEL : TOPO_CHANNEL);
}

void AppendSetRestore::Restore(int isUndo) {
	set  = *setList->sets[setList->Count()-1];
	name = *setList->names[setList->Count()-1];
	setList->DeleteSet(setList->Count()-1);
	if (mpEditPoly->ip) {
		mpEditPoly->ip->NamedSelSetListChanged();
		mpEditPoly->SetupNamedSelDropDown ();
		mpEditPoly->UpdateNamedSelDropDown ();
	}
}

void AppendSetRestore::Redo() {
	setList->AppendSet(set, 0, name);
	if (mpEditPoly->ip) {
		mpEditPoly->ip->NamedSelSetListChanged();
		mpEditPoly->SetupNamedSelDropDown ();
		mpEditPoly->UpdateNamedSelDropDown ();
	}
}

void RingLoopRestore::Restore(int isUndo) {
	int			l_ringValue = mpEditPoly->getRingValue();
	int			l_loopValue = mpEditPoly->getLoopValue();
	BitArray	l_sel		= mpEditPoly->GetEdgeSel();

	mpEditPoly->SetEdgeSel(mSelectedEdges,mpEditPoly,0);
	mpEditPoly->setRingValue(m_ringValue);
	mpEditPoly->setLoopValue(m_loopValue);

	// update the selected edge list for redo purpose 
	mSelectedEdges	= l_sel;
	m_ringValue		= l_ringValue;
	m_loopValue		= l_loopValue;

	mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
}

void RingLoopRestore::Redo() {
	BitArray	l_sel		= mpEditPoly->GetEdgeSel();
	int			l_ringValue = mpEditPoly->getRingValue();
	int			l_loopValue = mpEditPoly->getLoopValue();

	mpEditPoly->SetEdgeSel(mSelectedEdges,mpEditPoly,0);
	mpEditPoly->setRingValue(m_ringValue);
	mpEditPoly->setLoopValue(m_loopValue);

	// update the selected edge list for restore purpose 
	mSelectedEdges	= l_sel;
	m_ringValue		= l_ringValue;
	m_loopValue		= l_loopValue;

	mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
}


void EPDeleteSetRestore::Restore(int isUndo) {
	setList->AppendSet (set, 0, name);
	if (mpEditPoly->ip) {
		mpEditPoly->ip->NamedSelSetListChanged();
		mpEditPoly->SetupNamedSelDropDown ();
		mpEditPoly->UpdateNamedSelDropDown ();
	}
}

void EPDeleteSetRestore::Redo() {
	setList->RemoveSet (name);
	if (mpEditPoly->ip) {
		mpEditPoly->ip->NamedSelSetListChanged();
		mpEditPoly->SetupNamedSelDropDown ();
		mpEditPoly->UpdateNamedSelDropDown ();
	}
}

void EPSetNameRestore::Restore(int isUndo) {			
	redo = *setList->names[index];
	*setList->names[index] = undo;
	if (mpEditPoly->ip) mpEditPoly->ip->NamedSelSetListChanged();
}

void EPSetNameRestore::Redo() {
	*setList->names[index] = redo;
	if (mpEditPoly->ip) mpEditPoly->ip->NamedSelSetListChanged();
}

NamedSetChangeRestore::NamedSetChangeRestore (int level, int index, EditPolyObject *e) : mLevel(level), mIndex(index), mpEditPoly(e)
{
	BitArray *sel = mpEditPoly->GetNamedSelSet (mLevel, mIndex);
	if (sel == NULL) return;
	mUndo = *sel;
}

void NamedSetChangeRestore::Restore (int isUndo)
{
	if (mUndo.GetSize() == 0) return;
	if (mRedo.GetSize() == 0) {
		BitArray *sel = mpEditPoly->GetNamedSelSet (mLevel, mIndex);
		if (sel != NULL) mRedo = *sel;
	}
	mpEditPoly->SetNamedSelSet (mLevel, mIndex, &mUndo);
}

void NamedSetChangeRestore::Redo ()
{
	mpEditPoly->SetNamedSelSet (mLevel, mIndex, &mRedo);
}

void TransformPlaneRestore::Restore (int isUndo) {
	newSliceCenter = mpEditPoly->sliceCenter;
	newSliceRot = mpEditPoly->sliceRot;
	newSliceSize = mpEditPoly->sliceSize;
	mpEditPoly->sliceCenter = oldSliceCenter;
	mpEditPoly->sliceRot = oldSliceRot;
	mpEditPoly->sliceSize = oldSliceSize;
	if (mpEditPoly->EpPreviewOn()) mpEditPoly->EpPreviewInvalidate ();	// Luna task 748A
	mpEditPoly->NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
}

void TransformPlaneRestore::Redo () {
	oldSliceCenter = mpEditPoly->sliceCenter;
	oldSliceRot = mpEditPoly->sliceRot;
	oldSliceSize = mpEditPoly->sliceSize;
	mpEditPoly->sliceCenter = newSliceCenter;
	mpEditPoly->sliceRot = newSliceRot;
	mpEditPoly->sliceSize = newSliceSize;
	if (mpEditPoly->EpPreviewOn()) mpEditPoly->EpPreviewInvalidate ();	// Luna task 748A
	mpEditPoly->NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
}
