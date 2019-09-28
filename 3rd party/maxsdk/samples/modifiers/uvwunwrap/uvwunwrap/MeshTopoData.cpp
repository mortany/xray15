/********************************************************************** *<
FILE: MeshTopoData.cpp

DESCRIPTION: local mode data for the unwrap

HISTORY: 9/25/2006
CREATED BY: Peter Watje

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/

#include "unwrap.h"
#include "TvConnectionInfo.h"
#include <algorithm>
#pragma warning( push )
#pragma warning( disable : 4265 )
#include <thread>
#pragma warning( pop )
#include <Util/TipSystem.h>
#include <IViewPanelManager.h>
#include <IViewPanel.h>
#include "modsres.h"
#include <GetCOREInterface.h>

extern void Draw3dEdge(GraphicsWindow *gw, float size, Point3 a, Point3 b, Color c);

extern float AreaOfTriangle(Point3 a, Point3 b, Point3 c);
extern float AreaOfPolygon(Tab<Point3> &points);

const static int sNotFoundMirrorIndex = -2;

MeshTopoData::MeshTopoData() : mLoaded(FALSE)
{
	mLoaded = FALSE;
	Init();
	InitializeCriticalSection(&mCritSect);
}

MeshTopoData::MeshTopoData(ObjectState *os, int mapChannel) : MeshTopoData()
{
	Reset(os, mapChannel);
	InitializeCriticalSection(&mCritSect);
}

ToolGroupingData* MeshTopoData::GetToolGroupingData()
{
	return &mToolClusteringData;
}

void MeshTopoData::SetGeoCache(ObjectState *os)
{
	if (os->obj->IsSubClassOf(patchObjectClassID) && (patch == NULL))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		patch = new PatchMesh(pobj->patch);
		if (TVMaps.f.Count() == patch->numPatches)
		{
			for (int j = 0; j < TVMaps.f.Count(); j++)
			{
				int pcount = 3;
				if (patch->patches[j].type == PATCH_QUAD)
				{
					pcount = 4;
				}
				for (int k = 0; k < pcount; k++)
				{
					int index = patch->patches[j].v[k];
					TVMaps.f[j]->v[k] = index;
					//do handles and interiors
					//check if linear
					if (!(patch->patches[j].flags & PATCH_LINEARMAPPING))
					{
						//do geometric points						
						if (TVMaps.f[j]->vecs)
						{
							index = patch->patches[j].interior[k];
							TVMaps.f[j]->vecs->vinteriors[k] = patch->getNumVerts() + index;
							index = patch->patches[j].vec[k * 2];
							TVMaps.f[j]->vecs->vhandles[k * 2] = patch->getNumVerts() + index;
							index = patch->patches[j].vec[k * 2 + 1];
							TVMaps.f[j]->vecs->vhandles[k * 2 + 1] = patch->getNumVerts() + index;
						}
					}
				}
			}
			TVMaps.BuildGeomEdges();
			mGESel.SetSize(TVMaps.gePtrList.Count());
			ClearGeomEdgeSelection();
		}
	}
	else
		if (os->obj->IsSubClassOf(triObjectClassID) && (mesh == NULL))
		{
			TriObject *tobj = (TriObject*)os->obj;
			mesh = new Mesh(tobj->mesh);

			if (TVMaps.f.Count() == mesh->numFaces)
			{
				for (int j = 0; j < mesh->numFaces; j++)
				{
					for (int k = 0; k < 3; k++)
					{
						int index = mesh->faces[j].v[k];
						TVMaps.f[j]->v[k] = index;
					}
				}
				TVMaps.BuildGeomEdges();
				mGESel.SetSize(TVMaps.gePtrList.Count());
				ClearGeomEdgeSelection();
			}
		}
		else if (os->obj->IsSubClassOf(polyObjectClassID) && (mnMesh == NULL))
		{
			PolyObject *pobj = (PolyObject*)os->obj;
			mnMesh = new MNMesh(pobj->mm);

			if (TVMaps.f.Count() == mnMesh->numf)
			{

				for (int j = 0; j < mnMesh->numf; j++)
				{
					int fct = mnMesh->f[j].deg;
					if (!mnMesh->f[j].GetFlag(MN_DEAD))
					{
						for (int k = 0; k < fct; k++)
						{
							int index = mnMesh->f[j].vtx[k];
							TVMaps.f[j]->v[k] = index;
						}
					}
				}
				TVMaps.BuildGeomEdges();
				mGESel.SetSize(TVMaps.gePtrList.Count());
				ClearGeomEdgeSelection();
			}
		}
	if (
		(!os->obj->IsSubClassOf(patchObjectClassID)) &&
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)) && (mesh == NULL))
	{
		//neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TimeValue t = GetCOREInterface()->GetTime();
			TriObject *tri = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			mesh = new Mesh(tri->GetMesh());

			if (TVMaps.f.Count() == mesh->numFaces)
			{

				for (int j = 0; j < mesh->numFaces; j++)
				{
					for (int k = 0; k < 3; k++)
					{
						int index = mesh->faces[j].v[k];
						TVMaps.f[j]->v[k] = index;
					}
				}
				TVMaps.BuildGeomEdges();
				mGESel.SetSize(TVMaps.gePtrList.Count());
				ClearGeomEdgeSelection();
			}
			delete tri;
		}
	}
	//make sure to initialize all the cluster IDs to -1
	mToolClusteringData.Init(this);
}

void MeshTopoData::Reset(ObjectState *os, int channel)
{
	FreeCache();
	if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		SetCache(pobj->patch, channel);
	}
	else
		if (os->obj->IsSubClassOf(triObjectClassID))
		{
			TriObject *tobj = (TriObject*)os->obj;
			SetCache(tobj->GetMesh(), channel);
		}
		else if (os->obj->IsSubClassOf(polyObjectClassID))
		{
			PolyObject *pobj = (PolyObject*)os->obj;
			SetCache(pobj->mm, channel);
		}
	if (
		(!os->obj->IsSubClassOf(patchObjectClassID)) &&
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)))
	{
		//neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TimeValue t = GetCOREInterface()->GetTime();
			TriObject *tri = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			os->obj = tri;
			os->obj->UnlockObject();
			SetCache(tri->GetMesh(), channel);
		}
	}

	ResizeTVVertSelection(TVMaps.v.Count());
	ClearTVVertSelection();

	mVSelPreview.SetSize(TVMaps.v.Count());
	ClearTVVertSelectionPreview();

	mFSel.SetSize(TVMaps.f.Count());
	ClearFaceSelection();

	SetFaceSelectionByRef(mFSelPrevious);

	SetFaceSelectionPreview(GetFaceSel());

	mGVSel.SetSize(TVMaps.geomPoints.Count());
	ClearGeomVertSelection();

	TVMaps.BuildGeomEdges();

	mSeamEdges.SetSize(TVMaps.ePtrList.Count(), TRUE);

	if (mESel.GetSize() != TVMaps.ePtrList.Count())
	{
		mESel.SetSize(TVMaps.ePtrList.Count());
		ClearTVEdgeSelection();
	}

	SetTVEdgeSelectionPreview(mESel);

	if (mGESel.GetSize() != TVMaps.gePtrList.Count())
	{
		mGESel.SetSize(TVMaps.gePtrList.Count());
		ClearGeomEdgeSelection();
	}

	TVMaps.edgesValid = FALSE;
	mToolClusteringData.Init(this);

	mMaterialIDFaceFilter.SetSize(TVMaps.f.Count());
	mMaterialIDFaceFilter.SetAll();
}

void MeshTopoData::Init()
{
	this->mesh = NULL;
	this->patch = NULL;
	this->mnMesh = NULL;
	mHasIncomingSelection = FALSE;
	TVMaps.mGeoEdgesValid = FALSE;
	TVMaps.edgesValid = FALSE;
	mUserCancel = FALSE;
	faceSelChanged = FALSE;
	mConstantTopoAccelerationNestingLevel = 0;
	mbNeedSynchronizationToRenderItem = FALSE;

	InvalidateMirrorData();
	mpMeshAdjFaceData = nullptr;
	GetMeshAdjFaceList();
	InvalidateMeshPolyData();
	ClearSewingPending();
	TVMaps.GetTopoChangedManager().RegisterTopoChangedListener(this);

	mbSeamEdgesDirty = false;
	mbOpenTVEdgesDirty = false;
	mbSeamEdgesPreviewDirty = false;

	mbValidTopoForBreakEdges = true;
	mbBreakEdgesSucceeded = false;
	mListenerTab.ZeroCount();
}

int MeshTopoData::ValidateTVVertices(bool doWarning)
{

    // Loop over all vertices in TVMaps.v, remediating and invalid vertices
    int nInvalid = 0;
    int nVertices = (TVMaps.v).Count();
    for (int v = 0; v != nVertices; ++v)
    {
        bool isValid = ValidateTVVertex(v);
        if (!isValid)
        {
            ++nInvalid;
        }
    }

    // Issue a warning message, if requested
    if (doWarning && (nInvalid > 0))
    {
        Interface* coreInterface = GetCOREInterface();
        if (coreInterface != NULL)
        {
            TSTR warningMsg;

            // Issue warning dialog so long as we are not in "quiet" mode; add entry to log, otherwise
            bool isQuiet = coreInterface->GetQuietMode();
            TSTR warningTitle = GetString(IDS_MTD_VERTEX_WARNING_TITLE);
            warningMsg.printf(_T("%s %d."), GetString(IDS_MTD_VERTEX_WARNING_MSG), nInvalid);
            if (isQuiet)
            {
                (coreInterface->Log())->LogEntry(SYSLOG_WARN, NO_DIALOG, warningTitle, warningMsg);
            }
            else
            {
                MessageBox(NULL, warningMsg, warningTitle, MB_OK);
            }
        }
    }

    // Return number of invalid vertices encountered
    return nInvalid;
}

MeshTopoData::~MeshTopoData()
{
	RaiseMeshTopoDataDeleted();
	FreeCache();
	TVMaps.GetTopoChangedManager().UnRegisterTopoChangedListener(this);
	DbgAssert(originalSels.size() == 0);
	DeleteCriticalSection(&mCritSect);
}

LocalModData *MeshTopoData::Clone() {
	MeshTopoData *d = new MeshTopoData();
	d->mesh = NULL;
	d->mnMesh = NULL;
	d->patch = NULL;


	//because of the d->TVMaps = TVMaps,the TopoChangedManager of d->TVMaps will become  the same one of the TVMaps'
	//so record the old one, then ready to recover it after the operation d->TVMaps = TVMaps.
	TopoChangedManager tChangedManager = d->TVMaps.GetTopoChangedManager();

	d->TVMaps = TVMaps;
	d->TVMaps.channel = TVMaps.channel;
	d->TVMaps.v = TVMaps.v;
	d->TVMaps.geomPoints = TVMaps.geomPoints;
	d->TVMaps.CloneFaces(TVMaps.f);

	d->TVMaps.e.Resize(0); // LAM - 8/23/04 - in case mod is currently active in modify panel
	d->TVMaps.ge.Resize(0);
	d->TVMaps.ePtrList.Resize(0); // LAM - 8/23/04 - in case mod is currently active in modify panel
	d->TVMaps.gePtrList.Resize(0);

	//d->TVMaps.e is resized as 0, the validation should be false.
	d->TVMaps.edgesValid = FALSE;
	//recover it.
	d->TVMaps.SetTopoChangedManager(tChangedManager);

	//firstly copy the filters
	d->mTVEdgeFilter = mTVEdgeFilter;
	d->mTVVertexFilter = mTVVertexFilter;
	d->mGeomEdgeFilter = mGeomEdgeFilter;
	d->mGeomVertexFilter = mGeomVertexFilter;
	d->mFaceFilter = mFaceFilter;

	d->SetTVVertSel(mVSel);
	d->SetTVVertSelectionPreview(mVSelPreview);

	d->SetTVEdgeSel(mESel);
	d->SetTVEdgeSelectionPreview(mESelPreview);
	d->SetFaceSelectionByRef(GetFaceSel());
	d->SetFaceSelectionPreview(mFSelPreview, FALSE);
	d->SetGeomEdgeSel(mGESel);
	d->SetGeomVertSel(mGVSel);

	d->mToolClusteringData = mToolClusteringData;
	return d;
}

void MeshTopoData::FreeCache() {
	if (mesh) delete mesh;
	mesh = NULL;
	if (patch) delete patch;
	patch = NULL;
	if (mnMesh) delete mnMesh;
	mnMesh = NULL;

	if (mFacesAtVs)
	{
		mFacesAtVs.reset();
	}
	if (mFacesAtVsFiltered)
	{
		mFacesAtVsFiltered.reset();
	}

	TVMaps.FreeFaces();
	TVMaps.FreeEdges();
	TVMaps.f.SetCount(0);
	TVMaps.v.SetCount(0);

	ReleaseAdjFaceList();
	InvalidateMirrorData();
	InvalidateMeshPolyData();
}

BOOL MeshTopoData::HasCacheChanged(ObjectState *os)
{
	BOOL reset = FALSE;
	if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		if ((patch == NULL) || (pobj->patch.numPatches != patch->numPatches) || (TVMaps.f.Count() != patch->numPatches))
		{
			reset = TRUE;
		}
		//no make sure all faces have the same degree
		if (!reset && patch)
		{
			BOOL faceSubSel = FALSE;
			if (pobj->patch.selLevel == PATCH_PATCH)
				faceSubSel = TRUE;

			for (int i = 0; i < patch->numPatches; i++)
			{

				if (faceSubSel)
				{
					if (pobj->patch.patchSel[i] && GetFaceDead(i))
					{
						reset = TRUE;
					}
					else if (!pobj->patch.patchSel[i] && !GetFaceDead(i))
					{
						reset = TRUE;
					}
				}
				else
				{
					if (GetFaceDead(i))
						reset = TRUE;
				}

				if (pobj->patch.patches[i].type != patch->patches[i].type)
				{
					reset = TRUE;
					i = patch->numPatches;
				}
				else
				{
					int deg = 3;
					if (pobj->patch.patches[i].type == PATCH_QUAD)
						deg = 4;

					for (int j = 0; j < deg; j++)
					{
						int a = patch->patches[i].v[j];
						int b = pobj->patch.patches[i].v[j];
						if (a != b)
						{
							TVMaps.mGeoEdgesValid = FALSE;
						}
					}
				}
			}
		}
	}
	else
		if (os->obj->IsSubClassOf(triObjectClassID))
		{
			TriObject *tobj = (TriObject*)os->obj;
			if ((mesh == NULL) || (tobj->mesh.numFaces != mesh->numFaces) || (TVMaps.f.Count() != mesh->numFaces))
			{
				reset = TRUE;
			}
			if (!reset && mesh)
			{
				BOOL faceSubSel = FALSE;
				if (tobj->mesh.selLevel == MESH_FACE)
					faceSubSel = TRUE;
				for (int i = 0; i < mesh->numFaces; i++)
				{
					if (faceSubSel)
					{
						if (tobj->mesh.faceSel[i] && GetFaceDead(i))
						{
							reset = TRUE;
						}
						else if (!tobj->mesh.faceSel[i] && !GetFaceDead(i))
						{
							reset = TRUE;
						}
					}
					else
					{
						if (GetFaceDead(i))
							reset = TRUE;
					}
					int deg = 3;
					for (int j = 0; j < deg; j++)
					{
						int a = mesh->faces[i].v[j];
						int b = tobj->mesh.faces[i].v[j];
						if (a != b)
						{
							TVMaps.mGeoEdgesValid = FALSE;
						}
					}
				}
			}
		}
		else if (os->obj->IsSubClassOf(polyObjectClassID))
		{
			PolyObject *pobj = (PolyObject*)os->obj;
			if ((mnMesh == NULL) || (pobj->mm.numf != mnMesh->numf) || (TVMaps.f.Count() != mnMesh->numf))
			{
				reset = TRUE;
			}
			//no make sure all faces have the same degree
			if (!reset && mnMesh)
			{
				BOOL faceSubSel = FALSE;
				if (pobj->mm.selLevel == MNM_SL_FACE)
					faceSubSel = TRUE;

				BitArray bs;
				pobj->mm.getFaceSel(bs);

				for (int i = 0; i < pobj->mm.numf; i++)
				{

					if (faceSubSel)
					{
						if (bs[i] && GetFaceDead(i))
						{
							reset = TRUE;
						}
						else if (!bs[i] && !GetFaceDead(i))
						{
							reset = TRUE;
						}
					}
					else
					{
						if (GetFaceDead(i))
							reset = TRUE;
					}

					if (mnMesh->f[i].deg != pobj->mm.f[i].deg)
					{
						reset = TRUE;
						i = pobj->mm.numf;
					}
					else
					{
						int deg = pobj->mm.f[i].deg;
						for (int j = 0; j < deg; j++)
						{
							int a = mnMesh->f[i].vtx[j];
							int b = pobj->mm.f[i].vtx[j];
							if (a != b)
							{
								TVMaps.mGeoEdgesValid = FALSE;
							}
						}
					}
				}
			}
		}
	if (
		(!os->obj->IsSubClassOf(patchObjectClassID)) &&
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)))
	{
		//neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TimeValue t = GetCOREInterface()->GetTime();
			TriObject *tobj = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));

			if ((mesh == NULL) || (tobj->mesh.numFaces != mesh->numFaces))
			{
				reset = TRUE;
			}
			if (!reset && mesh)
			{
				BOOL faceSubSel = FALSE;
				if (tobj->mesh.selLevel == MESH_FACE)
					faceSubSel = TRUE;
				for (int i = 0; i < mesh->numFaces; i++)
				{
					if (faceSubSel)
					{
						if (tobj->mesh.faceSel[i] && GetFaceDead(i))
						{
							reset = TRUE;
						}
						else if (!tobj->mesh.faceSel[i] && !GetFaceDead(i))
						{
							reset = TRUE;
						}
					}
					else
					{
						if (GetFaceDead(i))
							reset = TRUE;
					}

					int deg = 3;
					for (int j = 0; j < deg; j++)
					{
						int a = mesh->faces[i].v[j];
						int b = tobj->mesh.faces[i].v[j];
						if (a != b)
						{
							TVMaps.mGeoEdgesValid = FALSE;
						}
					}
				}
			}
			if (tobj != os->obj)
				delete tobj;
		}
	}

	return reset;
}

bool MeshTopoData::UpdateGeoData(TimeValue t, ObjectState *os)
{
	bool bGeoVertChanged = false;
	BOOL remapGeoData = FALSE;
	int newGeomVertSize = os->obj->NumPoints();
	if (GetNumberGeomVerts() != newGeomVertSize)
	{
		SetNumberGeomVerts(newGeomVertSize);
		remapGeoData = TRUE;
		bGeoVertChanged = true;
	}

	Point3 tmpPoint, newPoint;
	for (int i = 0; i < newGeomVertSize; i++)
	{
		newPoint = os->obj->GetPoint(i);
		if (!bGeoVertChanged)
		{
			tmpPoint = GetGeomVert(i);
			if (tmpPoint != newPoint)
			{
				bGeoVertChanged = true;
			}
		}
		SetGeomVert(i, newPoint);
	}

	if (!TVMaps.mGeoEdgesValid)
	{
		remapGeoData = TRUE;
	}

	if (os->obj->IsSubClassOf(triObjectClassID))
	{
		TriObject *tobj = (TriObject*)os->obj;
		mHiddenPolygonsFromPipe.SetSize(tobj->GetMesh().getNumFaces());
		mHiddenPolygonsFromPipe.ClearAll();
		for (int i = 0; i < tobj->GetMesh().getNumFaces(); i++)
		{
			if (tobj->GetMesh().faces[i].Hidden())
				mHiddenPolygonsFromPipe.Set(i, TRUE);
		}
		if (tobj->GetMesh().selLevel != MESH_FACE)
		{
			//if the selection going up the stack changed since last time use that 
			if (!(mFSelPrevious == tobj->GetMesh().faceSel))
			{
				SetFaceSelectionByRef(tobj->GetMesh().faceSel);
				mFSelPrevious = GetFaceSel();
			}
			tobj->GetMesh().faceSel = GetFaceSel();
		}
	}
	else if (os->obj->IsSubClassOf(polyObjectClassID))
	{
		PolyObject *pobj = (PolyObject*)os->obj;
		mHiddenPolygonsFromPipe.SetSize(pobj->GetMesh().numf);
		mHiddenPolygonsFromPipe.ClearAll();
		for (int i = 0; i < pobj->GetMesh().numf; i++)
		{
			if (pobj->GetMesh().f[i].GetFlag(MN_HIDDEN))
				mHiddenPolygonsFromPipe.Set(i, TRUE);
		}
		if (pobj->GetMesh().selLevel != MNM_SL_FACE)
		{
			BitArray incomingFaceSel;
			pobj->GetMesh().getFaceSel(incomingFaceSel);
			//if the selection going up the stack changed since last time use that 
			if (!(mFSelPrevious == incomingFaceSel))
			{
				SetFaceSelectionByRef(incomingFaceSel);
				mFSelPrevious = GetFaceSel();
			}

			pobj->GetMesh().FaceSelect(GetFaceSel());
		}
	}
	else if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		mHiddenPolygonsFromPipe.SetSize(pobj->patch.numPatches);
		mHiddenPolygonsFromPipe.ClearAll();
		for (int i = 0; i < pobj->patch.numPatches; i++)
		{
			if (pobj->patch.patches[i].IsHidden())
				mHiddenPolygonsFromPipe.Set(i, TRUE);
		}
		if (pobj->patch.selLevel != PATCH_PATCH)
		{
			if (!(mFSelPrevious == pobj->patch.patchSel))
			{
				SetFaceSelectionByRef(pobj->patch.patchSel);
				mFSelPrevious = GetFaceSel();
			}
			pobj->patch.patchSel = GetFaceSel();
		}
	}

	if (remapGeoData)
	{
		//this sets the geo edges and cache
		if (mesh) delete mesh;
		mesh = NULL;
		if (patch) delete patch;
		patch = NULL;
		if (mnMesh) delete mnMesh;
		mnMesh = NULL;

		SetGeoCache(os);
	}

	if (mGVSel.GetSize() != TVMaps.geomPoints.Count())
	{
		mGVSel.SetSize(TVMaps.geomPoints.Count());
		ClearGeomVertSelection();
	}

	if (!TVMaps.edgesValid)
	{
		TVMaps.BuildEdges();
	}

	TVMaps.mGeoEdgesValid = TRUE;
	TVMaps.edgesValid = TRUE;
	return bGeoVertChanged;
}


void MeshTopoData::CopyFaceSelection(ObjectState *os)
{}

void MeshTopoData::CopyMatID(ObjectState *os, TimeValue t)
{
	if (os->obj->IsSubClassOf(polyObjectClassID))
	{
		PolyObject *pobj = (PolyObject*)os->obj;
		CopyMatID(pobj->GetMesh());

	}
	else if (os->obj->IsSubClassOf(triObjectClassID))
	{
		TriObject *tobj = (TriObject*)os->obj;
		CopyMatID(tobj->GetMesh());
	}
	else if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		CopyMatID(pobj->patch);
	}
	else if (
		(!os->obj->IsSubClassOf(patchObjectClassID)) &&
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)))
	{
		//neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TriObject *tri = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			os->obj = tri;
			os->obj->UnlockObject();
			CopyMatID(tri->GetMesh());
		}
	}
}

void MeshTopoData::ApplyMapping(ObjectState *os, int mapChannel, TimeValue t)
{
	if (os->obj->IsSubClassOf(triObjectClassID))
	{
		TriObject *tobj = (TriObject*)os->obj;
		ApplyMapping(tobj->GetMesh(), mapChannel);
	}
	else if (os->obj->IsSubClassOf(polyObjectClassID))
	{
		PolyObject *pobj = (PolyObject*)os->obj;
		ApplyMapping(pobj->GetMesh(), mapChannel);
	}
	else if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		ApplyMapping(pobj->patch, mapChannel);
	}
	else if (
		(!os->obj->IsSubClassOf(patchObjectClassID)) &&
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)))
	{
		//neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TimeValue t = GetCOREInterface()->GetTime();
			TriObject *tri = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			os->obj = tri;
			os->obj->UnlockObject();
			ApplyMapping(tri->GetMesh(), mapChannel);
		}
	}
}

void MeshTopoData::ApplyMeshSelectionLevel(ObjectState *os)
{
	if (os->obj->IsSubClassOf(triObjectClassID))
	{
		TriObject *tobj = (TriObject*)os->obj;
		if (GetCOREInterface()->GetSubObjectLevel() == 3)
		{
			tobj->GetMesh().SetDispFlag(DISP_SELPOLYS);
			tobj->GetMesh().selLevel = MESH_FACE;
		}
	}
	else if (os->obj->IsSubClassOf(polyObjectClassID))
	{
		PolyObject *pobj = (PolyObject*)os->obj;
		if (GetCOREInterface()->GetSubObjectLevel() == 3)
		{
			pobj->GetMesh().SetDispFlag(MNDISP_SELFACES);
			pobj->GetMesh().selLevel = MNM_SL_FACE;
		}
	}
	else if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		if (GetCOREInterface()->GetSubObjectLevel() == 3)
		{
			pobj->patch.SetDispFlag(DISP_SELPATCHES | DISP_LATTICE);
			pobj->patch.selLevel = PATCH_PATCH;
		}
	}
}

void MeshTopoData::SetFaceSel(const BitArray &set)
{
	if (set.GetSize() != TVMaps.f.Count())
	{
		DbgAssert(0);
		return;
	}

	mFSel = set;
	if (IsFaceFilterValid())
	{
		mFSel &= mFaceFilter;
	}

	if (mesh) mesh->faceSel = mFSel;
	if (mnMesh) mnMesh->FaceSelect(mFSel);
	if (patch) patch->patchSel = mFSel;
	RaiseNotifyFaceSelectionChanged();
}

int MeshTopoData::GetNumberGeomVerts()
{
	return TVMaps.geomPoints.Count();
}
void MeshTopoData::SetNumberGeomVerts(int ct)
{
	TVMaps.geomPoints.SetCount(ct);
}
Point3 MeshTopoData::GetGeomVert(int index)
{
	if ((index < 0) || (index >= TVMaps.geomPoints.Count()))
	{
		DbgAssert(0);
		return Point3(0.0f, 0.0f, 0.0f);
	}
	else
		return TVMaps.geomPoints[index];
}
void MeshTopoData::SetGeomVert(int index, Point3 p)
{
	if ((index < 0) || (index >= TVMaps.geomPoints.Count()))
	{
		DbgAssert(0);
		return;
	}
	else TVMaps.geomPoints[index] = p;
}

int MeshTopoData::GetNumberFaces()
{
	return TVMaps.f.Count();
}

int MeshTopoData::GetNumberTVVerts()
{
	return TVMaps.v.Count();
}

Point3 MeshTopoData::GetTVVert(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return Point3(0.0f, 0.0f, 0.0f);
	}
	else return TVMaps.v[index].GetP();
}

void MeshTopoData::SetTVVert(TimeValue t, int index, Point3 p)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else
	{
		RaisePinInvalidated(index);
		RaisePinAddedOrDeleted(index);

		TVMaps.v[index].SetP(p);
		RaiseTVVertChanged(t, index, p);
	}
}


int MeshTopoData::GetFaceDegree(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return 0;
	}
	else return TVMaps.f[faceIndex]->count;
}



BitArray MeshTopoData::GetTVVertSelection()
{
	return mVSel;
}

const BitArray& MeshTopoData::GetTVVertSelectionPreview()
{
	return mVSelPreview;
}

BitArray *MeshTopoData::GetTVVertSelectionPtr()
{
	return &mVSel;
}

BitArray *MeshTopoData::GetTVVertSelectionPreviewPtr()
{
	return &mVSelPreview;
}

void MeshTopoData::SetTVVertSelection(BitArray newSel)
{
	SetTVVertSel(newSel);
}

const BitArray&  MeshTopoData::GetTVVertSel()
{
	return mVSel;
}

void  MeshTopoData::SetTVVertSel(const BitArray& newSel)
{
	if (newSel.GetSize() != GetNumberTVVerts())
	{
		DbgAssert(0);
		return;
	}

	mVSel = newSel;
	UpdateTVVertSelInIsolationMode();
}

void  MeshTopoData::UpdateTVVertSelInIsolationMode()
{
	if (IsTVVertexFilterValid())
	{
		mVSel &= mTVVertexFilter;
	}
}

void MeshTopoData::SetTVVertSelectionPreview(const BitArray& newSelPreview)
{
	if (newSelPreview.GetSize() != GetNumberTVVerts())
	{
		DbgAssert(0);
		return;
	}

	mVSelPreview =  newSelPreview;
	if (IsTVVertexFilterValid())
	{
		mVSelPreview &= mTVVertexFilter;
	}
}

float MeshTopoData::GetTVVertInfluence(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return 0.0f;
	}
	else return TVMaps.v[index].GetInfluence();
}

void MeshTopoData::SetTVVertInfluence(int index, float influ)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else TVMaps.v[index].SetInfluence(influ);
}

BOOL MeshTopoData::GetTVVertSelected(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else return mVSel[index];
}

BOOL MeshTopoData::GetTVVertSelectPreview(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		return FALSE;
	}
	else return mVSelPreview[index];
}

void MeshTopoData::SetTVVertSelected(int index, BOOL sel)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else
	{
		if (sel && (index < TVMaps.mSystemLockedFlag.GetSize()) && TVMaps.mSystemLockedFlag[index])
			return;

		if (ResizeTVVertSelection(TVMaps.v.Count()))
		{
			DbgAssert(0);//should not resize
			ClearTVVertSelection();
		}
		
		if (sel && IsTVVertexFilterValid())
		{
			sel &= static_cast<BOOL>(DoesTVVertexPassFilter(index));
		}
		mVSel.Set(index, sel);
	}
}

void MeshTopoData::SetTVVertSelectPreview(int index, BOOL selPreview)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else
	{
		if (selPreview && (index < TVMaps.mSystemLockedFlag.GetSize()) && TVMaps.mSystemLockedFlag[index])
			return;

		if (mVSelPreview.GetSize() != TVMaps.v.Count())
		{
			DbgAssert(0);
			mVSelPreview.SetSize(TVMaps.v.Count());
			ClearTVVertSelectionPreview();
		}

		if (IsTVVertexFilterValid())
		{
			selPreview &= static_cast<BOOL>(DoesTVVertexPassFilter(index));
		}

		mVSelPreview.Set(index, selPreview);

	}
}

BOOL MeshTopoData::GetTVVertFrozen(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else
		return TVMaps.v[index].IsFrozen();
}
void MeshTopoData::SetTVVertFrozen(int index, BOOL frozen)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else
	{
		if (frozen)
			TVMaps.v[index].Freeze();
		else TVMaps.v[index].Unfreeze();
	}
}

int MeshTopoData::GetTVVertControlIndex(int index)
{
	if ((index >= 0) && (index < TVMaps.v.Count()))
	{
		return TVMaps.v[index].GetControlID();
	}

	return -1;
}

void MeshTopoData::SetTVVertControlIndex(int index, int id)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else
	{

		TVMaps.v[index].SetControlID(id);
	}
}

BOOL MeshTopoData::GetTVVertHidden(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else
		return TVMaps.v[index].IsHidden();
}
void MeshTopoData::SetTVVertHidden(int index, BOOL hidden)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else
	{
		if (hidden)
			TVMaps.v[index].Hide();
		else TVMaps.v[index].Show();
	}
}


int MeshTopoData::GetNumberTVEdges()
{
	return TVMaps.ePtrList.Count();
}

int MeshTopoData::GetTVEdgeVert(int index, int whichEnd) const
{
	if ((index < 0) || (index >= TVMaps.ePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else
	{
		if (whichEnd == 0)
			return TVMaps.ePtrList[index]->a;
		else return TVMaps.ePtrList[index]->b;
	}
}

int MeshTopoData::GetTVEdgeGeomVert(int index, int whichEnd)
{
	if ((index < 0) || (index >= TVMaps.ePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else
	{
		if (whichEnd == 0)
			return TVMaps.ePtrList[index]->ga;
		else return TVMaps.ePtrList[index]->gb;
	}
}

int MeshTopoData::GetTVEdgeVec(int edgeIndex, int whichEnd)
{
	if ((edgeIndex < 0) || (edgeIndex >= TVMaps.ePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else
	{
		if (whichEnd == 0)
			return TVMaps.ePtrList[edgeIndex]->avec;
		else return TVMaps.ePtrList[edgeIndex]->bvec;
	}
}

int MeshTopoData::GetTVEdgeNumberTVFaces(int index)
{
	if ((index < 0) || (index >= TVMaps.ePtrList.Count()))
	{
		DbgAssert(0);
		return 0;
	}
	else
		return TVMaps.ePtrList[index]->faceList.Count();
}

int MeshTopoData::GetTVEdgeConnectedTVFace(int index, int findex)
{
	if ((index < 0) || (index >= TVMaps.ePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else if ((findex < 0) || (findex >= TVMaps.ePtrList[index]->faceList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else
		return TVMaps.ePtrList[index]->faceList[findex];
}

BOOL MeshTopoData::GetTVEdgeHidden(int index)
{
	if ((index < 0) || (index >= TVMaps.ePtrList.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else return (TVMaps.ePtrList[index]->flags & FLAG_HIDDENEDGEA);
}


void MeshTopoData::InvertSelection(int mode)
{
	if (mode == TVVERTMODE) // TODO: check if invert follow the filter!
	{
		SetTVVertSel(~mVSel);
	}
	else if (mode == TVEDGEMODE)
	{
		SetTVEdgeSel(~mESel);
	}
	else if (mode == TVFACEMODE)
	{
		SetFaceSelectionByRef(~mFSel);
	}
}
void MeshTopoData::AllSelection(int mode)
{
	// TODO: check select all works with filter
	if (mode == TVVERTMODE)
	{
		SetAllTVVertSelection();
		SetAllGeomVertSelection();
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			if (!(TVMaps.v[i].GetFlag() & FLAG_WEIGHTMODIFIED))
				TVMaps.v[i].SetInfluence(0.0f);
		}
	}
	else if (mode == TVEDGEMODE)
	{
		SetAllTVEdgeSelection();
		SetAllGeomEdgeSelection();
		//Once the geometry edge selection is changed,the corresponding render item (UnwrapRenderItem)
		//will update the attribute buffer of selection by checking this dirty flag.
		SetSynchronizationToRenderItemFlag(TRUE);
	}
	else if (mode == TVFACEMODE)
	{
		SetAllFaceSelection();
	}
}

void MeshTopoData::ClearSelection(int mode)
{
	if (mode == TVVERTMODE)
	{
		ClearTVVertSelection();
		ClearGeomVertSelection();
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			if (!(TVMaps.v[i].GetFlag() & FLAG_WEIGHTMODIFIED))
				TVMaps.v[i].SetInfluence(0.0f);
		}

	}
	else if (mode == TVEDGEMODE)
	{
		ClearTVEdgeSelection();
		ClearGeomEdgeSelection();
	}
	else if (mode == TVFACEMODE)
	{
		ClearFaceSelection();
	}
}

void MeshTopoData::ClearSelectionPreview(int mode)
{
	if (mode == TVVERTMODE)
	{
		ClearTVVertSelectionPreview();
	}
	else if (mode == TVEDGEMODE)
	{
		ClearTVEdgeSelectionPreview();
	}
	else if (mode == TVFACEMODE)
	{
		ClearFaceSelectionPreview();
	}
}

int MeshTopoData::TransformedPointsCount()
{
	return mTransformedPoints.Count();
}
void MeshTopoData::TransformedPointsSetCount(int ct)
{
	mTransformedPoints.SetCount(ct);
}
IPoint2 MeshTopoData::GetTransformedPoint(int i)
{
	//special case here we use -1 as a null vector so just return nothing
	if (i == -1)
		return IPoint2(0, 0);

	if ((i < 0) || (i >= mTransformedPoints.Count()))
	{
		DbgAssert(0);
		return IPoint2(0, 0);
	}
	return mTransformedPoints[i];
}
void MeshTopoData::SetTransformedPoint(int i, IPoint2 pt)
{
	if ((i < 0) || (i >= mTransformedPoints.Count()))
	{
		DbgAssert(0);
	}
	mTransformedPoints[i] = pt;
}


//this makes a copy of the UV channel to v
void MeshTopoData::CopyTVData(Tab<UVW_TVVertClass> &v)
{
	v = TVMaps.v;
}
void MeshTopoData::PasteTVData(Tab<UVW_TVVertClass> &v)
{
	TVMaps.v = v;
}

BitArray MeshTopoData::GetTVEdgeSelection()
{
	return mESel;
}

BitArray *MeshTopoData::GetTVEdgeSelectionPtr()
{
	return &mESel;
}

const BitArray& MeshTopoData::GetTVEdgeSel()
{
	return mESel;
}

void MeshTopoData::SetTVEdgeSel(const BitArray& newSel)
{
	mESel = newSel;
	UpdateTVEdgeSelInIsolationMode();
}

void MeshTopoData::UpdateTVEdgeSelInIsolationMode()
{
	if (IsTVEdgeFilterValid())
	{
		mESel &= mTVEdgeFilter;
	}
}

void MeshTopoData::SetTVEdgeSelection(BitArray newSel)
{
	SetTVEdgeSel(newSel);
}

BOOL MeshTopoData::GetTVEdgeSelected(int index)
{
	if ((index < 0) || (index >= mESel.GetSize()))
	{
		DbgAssert(0);
		return FALSE;
	}
	return mESel[index];
}

void MeshTopoData::SetAllTVEdgeSelection()
{
	mESel.SetAll();
	UpdateTVEdgeSelInIsolationMode();
}

void MeshTopoData::SetTVEdgeSelected(int index, BOOL sel)
{
	if ((index < 0) || (index >= mESel.GetSize()))
	{
		DbgAssert(0);
		return;
	}

	if (IsTVEdgeFilterValid())
	{
		sel &= static_cast<BOOL>(DoesTVEdgePassFilter(index));
	}

	mESel.Set(index, sel);
}

const BitArray& MeshTopoData::GetTVEdgeSelectionPreview()
{
	return mESelPreview;
}

BitArray *MeshTopoData::GetTVEdgeSelectionPreviewPtr()
{
	return &mESelPreview;
}

void MeshTopoData::SetTVEdgeSelectionPreview(const BitArray& newSel)
{
	mESelPreview = newSel;
	if (IsTVEdgeFilterValid())
	{
		mESelPreview &= mTVEdgeFilter;
	}
}

BOOL MeshTopoData::GetTVEdgeSelectedPreview(int index)
{
	if ((index < 0) || (index >= mESelPreview.GetSize()))
	{
		return FALSE;
	}
	return mESelPreview[index];
}
void MeshTopoData::SetTVEdgeSelectedPreview(int index, BOOL sel)
{
	if ((index < 0) || (index >= mESel.GetSize()))
	{
		DbgAssert(0);
		return;
	}
	if (mESelPreview.GetSize() != mESel.GetSize())
	{
		mESelPreview.SetSize(mESel.GetSize());
	}

	if (IsTVEdgeFilterValid())
	{
		sel &= static_cast<BOOL>(DoesTVEdgePassFilter(index));
	}

	mESelPreview.Set(index, sel);
}

int MeshTopoData::EdgeIntersect(Point3 p, float threshold, int i1, int i2)
{
	return TVMaps.EdgeIntersect(p, threshold, i1, i2);
}


//Converts the edge selection into a vertex selection set
void    MeshTopoData::GetVertSelFromEdge(BitArray &sel)
{
	int iSelSize = TVMaps.v.Count();
	sel.SetSize(iSelSize);
	sel.ClearAll();
	if (mESel.GetSize() != TVMaps.ePtrList.Count())
	{
		mESel.SetSize(TVMaps.ePtrList.Count(), 1);
	}
	if (GetTVEdgeSel().AnyBitSet())
	{
		for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
		{
			if (GetTVEdgeSelected(i))
			{
				int index = TVMaps.ePtrList[i]->a;
				if ((index >= 0) && (index < iSelSize))
					sel.Set(index);

				index = TVMaps.ePtrList[i]->b;
				if ((index >= 0) && (index < iSelSize))
					sel.Set(index);

				index = TVMaps.ePtrList[i]->avec;
				if ((index >= 0) && (index < iSelSize))
					sel.Set(index);

				index = TVMaps.ePtrList[i]->bvec;
				if ((index >= 0) && (index < iSelSize))
					sel.Set(index);
			}
		}
	}
}
//Converts the vertex selection into a edge selection set
//PartialSelect determines whether all the vertices of a edge need to be selected for that edge to be selected
void    MeshTopoData::GetEdgeSelFromVert(BitArray &sel, BOOL partialSelect)
{
	sel.SetSize(TVMaps.ePtrList.Count());
	sel.ClearAll();
	if (GetTVVertSel().AnyBitSet())
	{
		for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
		{
			int a, b;
			a = TVMaps.ePtrList[i]->a;
			b = TVMaps.ePtrList[i]->b;
			if (partialSelect)
			{
				if (GetTVVertSelected(a) || GetTVVertSelected(b))
					sel.Set(i);
			}
			else
			{
				if (GetTVVertSelected(a) && GetTVVertSelected(b))
					sel.Set(i);
			}
		}
	}
}

void    MeshTopoData::GetGeomVertSelFromFace(BitArray &sel)
{
	sel.SetSize(TVMaps.geomPoints.Count());
	sel.ClearAll();
	if (GetFaceSel().AnyBitSet())
	{
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			if (GetFaceSelected(i))
			{
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[i]->v[k];
					sel.Set(index);
					if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
					{
						index = TVMaps.f[i]->vecs->vhandles[k * 2];
						if ((index >= 0) && (index < sel.GetSize()))
							sel.Set(index);

						index = TVMaps.f[i]->vecs->vhandles[k * 2 + 1];
						if ((index >= 0) && (index < sel.GetSize()))
							sel.Set(index);

						if (TVMaps.f[i]->flags & FLAG_INTERIOR)
						{
							index = TVMaps.f[i]->vecs->vinteriors[k];
							if ((index >= 0) && (index < sel.GetSize()))
								sel.Set(index);
						}
					}
				}
			}
		}
	}
}

//Converts the face selection into a vertex selection set
void    MeshTopoData::GetVertSelFromFace(BitArray &sel)		//Converts the vertex selection into a face selection set
{
	sel.SetSize(TVMaps.v.Count());
	sel.ClearAll();
	if (GetFaceSel().AnyBitSet())
	{
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			if (GetFaceSelected(i))
			{
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[i]->t[k];
					sel.Set(index);
					if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
					{
						index = TVMaps.f[i]->vecs->handles[k * 2];
						if ((index >= 0) && (index < sel.GetSize()))
							sel.Set(index);

						index = TVMaps.f[i]->vecs->handles[k * 2 + 1];
						if ((index >= 0) && (index < sel.GetSize()))
							sel.Set(index);

						if (TVMaps.f[i]->flags & FLAG_INTERIOR)
						{
							index = TVMaps.f[i]->vecs->interiors[k];
							if ((index >= 0) && (index < sel.GetSize()))
								sel.Set(index);
						}
					}
				}
			}
		}
	}
}
//PartialSelect determines whether all the vertices of a face need to be selected for that face to be selected
void    MeshTopoData::GetFaceSelFromVert(BitArray &sel, BOOL partialSelect)
{

	sel.SetSize(TVMaps.f.Count());
	sel.ClearAll();
	if (GetTVVertSel().AnyBitSet())
	{
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int pcount = 3;
			pcount = TVMaps.f[i]->count;
			if (pcount > 0)
			{
				int total = 0;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[i]->t[k];
					if (GetTVVertSelected(index)) total++;
					if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
					{
						index = TVMaps.f[i]->vecs->handles[k * 2];
						if ((index >= 0) && (index < sel.GetSize()))
						{
							if (GetTVVertSelected(index)) total++;
						}

						index = TVMaps.f[i]->vecs->handles[k * 2 + 1];
						if ((index >= 0) && (index < sel.GetSize()))
						{
							if (GetTVVertSelected(index)) total++;
						}

						if (TVMaps.f[i]->flags & FLAG_INTERIOR)
						{
							index = TVMaps.f[i]->vecs->interiors[k];
							if ((index >= 0) && (index < sel.GetSize()))
							{
								if (GetTVVertSelected(index))
									total++;
							}
						}

					}
				}
				if (partialSelect && total)
					sel.Set(i);
				else if (!partialSelect && total >= pcount)
					sel.Set(i);
			}
		}
	}
}
void    MeshTopoData::GetFaceSelFromEdge(BitArray &sel, BOOL partialSelect)
{
	sel.SetSize(TVMaps.f.Count());
	sel.ClearAll();
	if (partialSelect)
	{
		//For each selected edge, mark its corresponding face as selected as well.
		if (GetTVEdgeSel().AnyBitSet())
		{
			for (int i = 0; i < mESel.GetSize(); i++)
			{
				if (GetTVEdgeSelected(i))
				{
					for (int j = 0; j < TVMaps.ePtrList[i]->faceList.Count(); j++)
					{
						sel.Set(TVMaps.ePtrList[i]->faceList[j]);
					}
				}
			}
		}
	}
	else
	{
		BitArray vsel = GetTVVertSel();
		BitArray tempSel;
		tempSel.SetSize(vsel.GetSize());
		tempSel = vsel;
		GetVertSelFromEdge(vsel);
		SetTVVertSel(vsel);
		GetFaceSelFromVert(sel, FALSE);
		SetTVVertSel(tempSel);
	}
}

void MeshTopoData::TransferSelectionStart(int subMode)
{
	EnterCriticalSection(&mCritSect);

	originalSels.push_back(mVSel);
	holdFSels.push_back(GetFaceSel());
	holdESels.push_back(mESel);

	if (subMode == TVEDGEMODE)
	{
		GetVertSelFromEdge(mVSel);
		UpdateTVVertSelInIsolationMode();
	}
	else if (subMode == TVFACEMODE)
	{
		GetVertSelFromFace(mVSel);
		UpdateTVVertSelInIsolationMode();
	}

	LeaveCriticalSection(&mCritSect);

}

void MeshTopoData::TransferSelectionEnd(int subMode, BOOL partial, BOOL recomputeSelection)
{
	EnterCriticalSection(&mCritSect);

	if (subMode == TVEDGEMODE) //face mode
	{
		//need to convert our vertex selection to face
		if ((recomputeSelection) || (holdESels.back().GetSize() != TVMaps.ePtrList.Count()))
			GetEdgeSelFromVert(mESel, partial);
		else
			SetTVEdgeSel(holdESels.back());
		//now we need to restore the vertex selection
		if (mVSel.GetSize() == originalSels.back().GetSize())
			SetTVVertSel(originalSels.back()); 
	}
	else if (subMode == TVFACEMODE) //face mode
	{
		//need to convert our vertex selection to face
		if (recomputeSelection)
		{
			GetFaceSelFromVert(mFSel, partial);
		}
		else
		{
			SetFaceSelectionByRef(holdFSels.back());
		}
		//now we need to restore the vertex selection as long as we have not changed topo
		if (mVSel.GetSize() == originalSels.back().GetSize())
			SetTVVertSel(originalSels.back());
	}
	else if (subMode == TVVERTMODE) //vertex mode
	{	
		//Things changed, cleanup dead verticies in the selection
		auto vertSel = GetTVVertSel();
		BitArray tempSel;
		tempSel.SetSize(vertSel.GetSize());
		tempSel = vertSel;
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			if (TVMaps.v[i].IsDead() && !TVMaps.mSystemLockedFlag[i] && i <= tempSel.GetSize())
			{
				tempSel.Clear(i);
			}
		}
		SetTVVertSel(tempSel);
	}

	originalSels.pop_back();
	holdFSels.pop_back();
	holdESels.pop_back();
	LeaveCriticalSection(&mCritSect);
}

BOOL MeshTopoData::GetFaceSelected(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()) || (faceIndex >= mFSel.GetSize()))
	{
		DbgAssert(0);
		return FALSE;
	}

	return mFSel[faceIndex];
}
void MeshTopoData::SetFaceSelected(int faceIndex, BOOL sel, BOOL bMirrorSel)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()) || (faceIndex >= mFSel.GetSize()))
	{
		DbgAssert(0);
		return;
	}

	if (IsFaceFilterValid())
	{
		sel &= static_cast<BOOL>(DoesFacePassFilter(faceIndex));
	}

	mFSel.Set(faceIndex, sel);
	faceSelChanged = TRUE;
	if (bMirrorSel)
	{
		MirrorFaceSel(faceIndex, sel);
	}
	RaiseNotifyFaceSelectionChanged();
}

BOOL MeshTopoData::GetFaceSelectedPreview(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()) || (faceIndex >= mFSelPreview.GetSize()))
	{
		return FALSE;
	}

	return mFSelPreview[faceIndex];
}
void MeshTopoData::SetFaceSelectedPreview(int faceIndex, BOOL sel, BOOL bMirrorSel)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()) || (faceIndex >= mFSel.GetSize()))
	{
		DbgAssert(0);
		return;
	}

	if (mFSelPreview.GetSize() != mFSel.GetSize())
	{
		mFSelPreview.SetSize(mFSel.GetSize());
	}

	if (sel && IsFaceFilterValid())
	{
		sel &= static_cast<BOOL>(DoesFacePassFilter(faceIndex));
	}

	mFSelPreview.Set(faceIndex, sel);

	if (bMirrorSel)
	{
		MirrorFaceSel(faceIndex, sel, TRUE);
	}
}

BOOL MeshTopoData::GetFaceCurvedMaping(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	if (TVMaps.f[faceIndex]->flags & FLAG_CURVEDMAPPING)
		return TRUE;
	else return FALSE;
}

BOOL MeshTopoData::GetFaceFrozen(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	return TVMaps.f[faceIndex]->flags & FLAG_FROZEN;

}
void MeshTopoData::SetFaceFrozen(int faceIndex, BOOL frozen)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return;
	}

	if (frozen)
		TVMaps.f[faceIndex]->flags |= FLAG_FROZEN;
	else TVMaps.f[faceIndex]->flags &= ~FLAG_FROZEN;

}

BOOL MeshTopoData::GetFaceHasVectors(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	if (TVMaps.f[faceIndex]->vecs)
		return TRUE;
	else return FALSE;

}

BOOL MeshTopoData::GetFaceHasInteriors(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	if (!TVMaps.f[faceIndex]->vecs)
		return FALSE;

	return TVMaps.f[faceIndex]->flags & FLAG_INTERIOR;

}


int MeshTopoData::GetFaceTVVert(int faceIndex, int vertexID)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	if ((vertexID < 0) || (vertexID >= TVMaps.f[faceIndex]->count))
	{
		DbgAssert(0);
		return -1;
	}

	return TVMaps.f[faceIndex]->t[vertexID];
}



void MeshTopoData::SetFaceTVVert(int faceIndex, int vertexID, int id)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return;
	}
	if ((vertexID < 0) || (vertexID >= TVMaps.f[faceIndex]->count))
	{
		DbgAssert(0);
		return;
	}
	if ((id < 0) || (id >= GetNumberTVVerts()))
	{
		DbgAssert(0);
		return;
	}

	TVMaps.f[faceIndex]->t[vertexID] = id;

}

int MeshTopoData::GetFaceTVHandle(int faceIndex, int handleID)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	if (TVMaps.f[faceIndex]->vecs == NULL)
	{
		DbgAssert(0);
		return -1;
	}
	if (handleID < 0 || handleID >= 8)
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->vecs->handles[handleID];
}

void MeshTopoData::SetFaceTVHandle(int faceIndex, int handleID, int id)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return;
	}
	if (TVMaps.f[faceIndex]->vecs == NULL)
	{
		DbgAssert(0);
		return;
	}
	if (handleID < 0 || handleID >= 8)
	{
		DbgAssert(0);
		return;
	}
	//KZ 19/04/2017: Stop anyone from setting a bad vertex list handle.
	//See 'AddTVVert', the return is what we stuff into 'handles'.
	if(id >= TVMaps.v.Count())
	{
		DbgAssert(0);
		return;
	}
	TVMaps.f[faceIndex]->vecs->handles[handleID] = id;
}

int MeshTopoData::GetFaceTVInterior(int faceIndex, int handleID)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	if (TVMaps.f[faceIndex]->vecs == NULL)
	{
		DbgAssert(0);
		return -1;
	}
	if ((handleID < 0) || (handleID >= 4))
	{
		DbgAssert(0);
		return -1;
	}

	return TVMaps.f[faceIndex]->vecs->interiors[handleID];
}

void MeshTopoData::SetFaceTVInterior(int faceIndex, int handleID, int id)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return;
	}
	if (TVMaps.f[faceIndex]->vecs == NULL)
	{
		DbgAssert(0);
		return;
	}
	if ((handleID < 0) || (handleID >= 4))
	{
		DbgAssert(0);
		return;
	}
	if ((id < 0) || (id >= GetNumberTVVerts()))
	{
		DbgAssert(0);
		return;
	}

	TVMaps.f[faceIndex]->vecs->interiors[handleID] = id;
}

BitArray MeshTopoData::GetFaceSelection()
{
	return mFSel;
}

BitArray *MeshTopoData::GetFaceSelectionPtr()
{
	return &mFSel;
}

void MeshTopoData::SetFaceSelectionByRef(const BitArray& sel, BOOL bMirrorSel)
{
	mFSel = sel;
	faceSelChanged = TRUE;;
	if (bMirrorSel)
	{
		MirrorFaceSel();
	}
	UpdateFaceSelInIsolationMode();
	RaiseNotifyFaceSelectionChanged();
}

void MeshTopoData::UpdateFaceSelInIsolationMode()
{
	if (IsFaceFilterValid())
	{
		mFSel &= mFaceFilter;
	}
}

void MeshTopoData::SetFaceSelection(BitArray sel, BOOL bMirrorSel)
{
	SetFaceSelectionByRef(sel, bMirrorSel);
}

BitArray MeshTopoData::GetFaceSelectionPreview()
{
	return mFSelPreview;
}

BitArray *MeshTopoData::GetFaceSelectionPreviewPtr()
{
	return &mFSelPreview;
}


void MeshTopoData::SetFaceSelectionPreview(const BitArray& sel, BOOL bMirrorSel)
{
	mFSelPreview = sel;

	if (bMirrorSel)
	{
		MirrorFaceSel(-1, 1, TRUE);
	}

	if (IsFaceFilterValid())
	{
		mFSelPreview &= mFaceFilter;
	}
}

int MeshTopoData::PolyIntersect(Point3 p1, int i1, int i2, BitArray *ignoredFaces)
{

	static int startFace = 0;
	int ct = 0;
	Point3 p2 = p1;
	float x = FLT_MIN;
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if (!(TVMaps.v[i].IsDead()) && !TVMaps.mSystemLockedFlag[i])
		{
			float tx = TVMaps.v[i].GetP()[i1];
			if (tx > x) x = tx;
		}
	}
	p2.x = x + 10.0f;

	if (startFace >= TVMaps.f.Count()) startFace = 0;


	while (ct != TVMaps.f.Count())
	{
		int pcount = TVMaps.f[startFace]->count;
		int hit = 0;
		BOOL bail = FALSE;
		if (ignoredFaces)
		{
			if ((*ignoredFaces)[startFace])
				bail = TRUE;
		}
		if (IsFaceVisible(startFace) && (!bail))
		{
			int frozen = 0, hidden = 0;
			for (int j = 0; j < pcount; j++)
			{
				int index = TVMaps.f[startFace]->t[j];
				if (index >= TVMaps.v.Count())	// CER 130067051, MAXX-33851: Fix up any out-of-range texture vertex indexes
				{
					DbgAssert(0);
					index = TVMaps.f[startFace]->t[j] = TVMaps.v.Count() - 1;
				}
				if (IsTVVertVisible(index) == FALSE) hidden++;
				if (TVMaps.v[index].IsFrozen()) frozen++;
			}
			if ((frozen == pcount) || (hidden == pcount))
			{
			}
			else if ((patch != NULL) && (!(TVMaps.f[startFace]->flags & FLAG_CURVEDMAPPING)) && (TVMaps.f[startFace]->vecs))
			{
				Spline3D spl;
				spl.NewSpline();
				int i = startFace;
				for (int j = 0; j < pcount; j++)
				{
					Point3 in, p, out;
					int index = TVMaps.f[i]->t[j];
					p = GetTVVert(index);

					index = TVMaps.f[i]->vecs->handles[j * 2];
					out = GetTVVert(index);
					if (j == 0)
						index = TVMaps.f[i]->vecs->handles[pcount * 2 - 1];
					else index = TVMaps.f[i]->vecs->handles[j * 2 - 1];

					in = GetTVVert(index);

					SplineKnot kn(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, in, out);
					spl.AddKnot(kn);

					spl.SetClosed();
					spl.ComputeBezPoints();
				}
				//draw curves
				Point3 ptList[7 * 4];
				int ct = 0;
				for (int j = 0; j < pcount; j++)
				{
					int jNext = j + 1;
					if (jNext >= pcount) jNext = 0;
					Point3 p;
					int index = TVMaps.f[i]->t[j];
					if (j == 0)
						ptList[ct++] = GetTVVert(index);

					for (int iu = 1; iu <= 5; iu++)
					{
						float u = (float)iu / 5.f;
						ptList[ct++] = spl.InterpBezier3D(j, u);

					}



				}
				for (int j = 0; j < ct; j++)
				{
					int index;
					if (j == (ct - 1))
						index = 0;
					else index = j + 1;
					Point3 a(0.0f, 0.0f, 0.0f), b(0.0f, 0.0f, 0.0f);
					a.x = ptList[j][i1];
					a.y = ptList[j][i2];

					b.x = ptList[index][i1];
					b.y = ptList[index][i2];

					if (LineIntersect(p1, p2, a, b)) hit++;
					//					if (LineIntersect(p1, p2, ptList[j],ptList[index])) hit++;
				}


			}
			else
			{
				for (int i = 0; i < pcount; i++)
				{
					int faceIndexA;
					faceIndexA = TVMaps.f[startFace]->t[i];
					int faceIndexB;
					if (i == (pcount - 1))
						faceIndexB = TVMaps.f[startFace]->t[0];
					else faceIndexB = TVMaps.f[startFace]->t[i + 1];
					Point3 a(0.0f, 0.0f, 0.0f), b(0.0f, 0.0f, 0.0f);
					a.x = TVMaps.v[faceIndexA].GetP()[i1];
					a.y = TVMaps.v[faceIndexA].GetP()[i2];
					b.x = TVMaps.v[faceIndexB].GetP()[i1];
					b.y = TVMaps.v[faceIndexB].GetP()[i2];
					if (LineIntersect(p1, p2, a, b))
						hit++;
				}
			}


			if ((hit % 2) == 1)
				return startFace;
		}
		startFace++;
		if (startFace >= TVMaps.f.Count()) startFace = 0;
		ct++;
	}
	return -1;

}

BOOL MeshTopoData::LineIntersect(Point3 p1, Point3 p2, Point3 q1, Point3 q2)
{


	float a, b, c, d, det;  /* parameter calculation variables */
	float max1, max2, min1, min2; /* bounding box check variables */

								  /*  First make the bounding box test. */
	max1 = maxmin(p1.x, p2.x, min1);
	max2 = maxmin(q1.x, q2.x, min2);
	if ((max1 < min2) || (min1 > max2)) return(FALSE); /* no intersection */
	max1 = maxmin(p1.y, p2.y, min1);
	max2 = maxmin(q1.y, q2.y, min2);
	if ((max1 < min2) || (min1 > max2)) return(FALSE); /* no intersection */

													   /* See if the endpoints of the second segment lie on the opposite
													   sides of the first.  If not, return 0. */
	a = (q1.x - p1.x) * (p2.y - p1.y) -
		(q1.y - p1.y) * (p2.x - p1.x);
	b = (q2.x - p1.x) * (p2.y - p1.y) -
		(q2.y - p1.y) * (p2.x - p1.x);
	if (a != 0.0f && b != 0.0f && SAME_SIGNS(a, b)) return(FALSE);

	/* See if the endpoints of the first segment lie on the opposite
	sides of the second.  If not, return 0.  */
	c = (p1.x - q1.x) * (q2.y - q1.y) -
		(p1.y - q1.y) * (q2.x - q1.x);
	d = (p2.x - q1.x) * (q2.y - q1.y) -
		(p2.y - q1.y) * (q2.x - q1.x);
	if (c != 0.0f && d != 0.0f && SAME_SIGNS(c, d)) return(FALSE);

	/* At this point each segment meets the line of the other. */
	det = a - b;
	if (det == 0.0f) return(FALSE); /* The segments are colinear.  Determining */
	return(TRUE);
}


BitArray MeshTopoData::GetGeomVertSelection()
{
	return mGVSel;
}

BitArray *MeshTopoData::GetGeomVertSelectionPtr()
{
	return &mGVSel;
}

const BitArray& MeshTopoData::GetGeomVertSel()
{
	return mGVSel;
}

void MeshTopoData::SetGeomVertSel(const BitArray& newSel)
{
	mGVSel = newSel;
	UpdateGeomVertSelInIsolationMode();
}

void MeshTopoData::UpdateGeomVertSelInIsolationMode()
{
	if (IsGeomVertexFilterValid())
	{
		mGVSel &= mGeomVertexFilter;
	}
}

void MeshTopoData::SetGeomVertSelection(BitArray newSel)
{
	SetGeomVertSel(newSel);
}

BOOL MeshTopoData::GetGeomVertSelected(int index)
{
	if ((index < 0) || (index >= TVMaps.geomPoints.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	if (mGVSel.GetSize() != TVMaps.geomPoints.Count())
	{
		DbgAssert(0);
		mGVSel.SetSize(TVMaps.geomPoints.Count());
		ClearGeomVertSelection();
		return FALSE;
	}

	return mGVSel[index];

}
void MeshTopoData::SetGeomVertSelected(int index, BOOL sel, BOOL mirrorEnabled)
{
	if ((index < 0) || (index >= TVMaps.geomPoints.Count()))
	{
		DbgAssert(0);
		return;
	}
	if (mGVSel.GetSize() != TVMaps.geomPoints.Count())
	{
		DbgAssert(0);
		mGVSel.SetSize(TVMaps.geomPoints.Count());
		ClearGeomVertSelection();
		return;
	}

	if (IsGeomVertexFilterValid())
	{
		sel &= static_cast<BOOL>(DoesGeomVertexPassFilter(index));
	}

	mGVSel.Set(index, sel);

	if (mirrorEnabled)
	{
		MirrorGeomVertSel(index, sel);
	}
}

void MeshTopoData::InvalidateMirrorData()
{
	mGeomEdgeMirrorDataList.ZeroCount();
	mGeomVertMirrorDataList.ZeroCount();
	mGeomFaceMirrorDataList.ZeroCount();
	mMirrorBoundingBox.Init();
	mMirrorMatrix.IdentityMatrix();
}

class Point3Sort
{
public:
	static float MirrorThreshold;

	static double GetPoint3Delta(const Point3& a, const Point3& b)
	{
		double result = 0.0;
		result += abs(a.x - b.x);
		result += abs(a.y - b.y);
		result += abs(a.z - b.z);
		return result;
	}
};

float Point3Sort::MirrorThreshold = 0.0f;

class MirrorDataHelper
{
public:
	MirrorDataHelper(const Point3& pt3, int index, bool bHidden)
		: mIndex(index), mHidden(bHidden), mpVertex(new Point3(pt3)), mbVertex(true)
	{}

	MirrorDataHelper(MeshPolyData* poly, const UVW_ChannelClass& uvMap, int index, bool bHidden)
		: mIndex(index), mHidden(bHidden), mbVertex(false)
	{
		mpVertIndice = new std::vector<int>();
		Point3 tmp;
		if (poly)
		{
			int faceCount = (int)poly->mFaceList.size();
			int faceIndex = -1;
			int vCount = 0;
			int vIdx = -1;
			for (int i = 0; i < faceCount; ++i)
			{
				faceIndex = poly->mFaceList[i];
				vCount = uvMap.f[faceIndex]->count;
				for (int j = 0; j < vCount; ++j)
				{
					vIdx = uvMap.f[faceIndex]->v[j];
					mpVertIndice->push_back(vIdx);
				}
			}
			std::sort(mpVertIndice->begin(), mpVertIndice->end());
			RemoveDuplicatedIndices(mpVertIndice);
		}
	}

	MirrorDataHelper(int* verts, int vertCount, const Tab<Point3>& geomVertTab, int index, bool bHidden)
		: mIndex(index), mHidden(bHidden), mbVertex(false)
	{
		mpVertIndice = new std::vector<int>();
		Point3 tmp;
		for (int i = 0; i < vertCount; ++i)
		{
			mpVertIndice->push_back(verts[i]);
		}
		std::sort(mpVertIndice->begin(), mpVertIndice->end());
	}

	// create a tmp mirrored data helper to search in the sorted data helper vector.
	// should be destroyed right after using.
	MirrorDataHelper* CreateMirroredDataHelper(const Matrix3& mirrorMat, const Tab<GeomSelMirrorData>* pMirrorTab = nullptr)
	{
		MirrorDataHelper* pHelper = new MirrorDataHelper();
		pHelper->mbVertex = this->mbVertex;
		if (pHelper->mbVertex)
		{
			Point3 tmp = *mpVertex;
			tmp = tmp*mirrorMat;
			pHelper->mpVertex = new Point3(tmp);
		}
		else
		{
			DbgAssert(pMirrorTab != nullptr);
			pHelper->mpVertIndice = new std::vector<int>();
			size_t s = this->mpVertIndice->size();
			pHelper->mpVertIndice->reserve(s);
			int tmp = -1;
			for (auto i = 0; i < s; ++i)
			{
				tmp = this->mpVertIndice->operator[](i);
				tmp = pMirrorTab->operator[](tmp).index;
				pHelper->mpVertIndice->push_back(tmp);
			}
			std::sort(pHelper->mpVertIndice->begin(), pHelper->mpVertIndice->end());
		}

		return pHelper;
	}

	virtual ~MirrorDataHelper()
	{
		if (mpVertex && mbVertex)
		{
			delete mpVertex;
		}

		if (mpVertIndice && !mbVertex)
		{
			delete mpVertIndice;
		}
	}

	union
	{
		Point3* mpVertex; //used when mbVertex is true
		std::vector<int>* mpVertIndice; //used when mbVertex is false
	};
	bool mbVertex; // true means this helper represent a vertex data, false otherwise.
	int mIndex;
	bool mHidden;
private:
	void RemoveDuplicatedIndices(std::vector<int>* sortedVec)
	{
		int a, b;
		for (auto it = sortedVec->begin(); it != sortedVec->end();)
		{
			auto next = it + 1;
			if (next != sortedVec->end())
			{
				a = *it;
				b = *next;
				if (a == b)
				{
					it = sortedVec->erase(it);
					continue;
				}
			}
			++it;
		}
	}

	MirrorDataHelper() : mHidden(false), mIndex(-1), mbVertex(true), mpVertex(nullptr)
	{}
};

int MirrorDataCompare(const void* v1, const void* v2, double threshold)
{
	MirrorDataHelper* p1 = (MirrorDataHelper*)v1;
	MirrorDataHelper* p2 = (MirrorDataHelper*)v2;

	DbgAssert(p1->mbVertex == p2->mbVertex);
	// for vertex data, we only sort the helper based on x value.
	if (p1->mbVertex)
	{
		if (abs(p1->mpVertex->x - p2->mpVertex->x) <= threshold)
		{
			return 0;
		}
		else if (p1->mpVertex->x > p2->mpVertex->x)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		// compare the first valid indice and then compare the size
		size_t p1Size = p1->mpVertIndice->size();
		size_t p2Size = p2->mpVertIndice->size();
		size_t pSize = p1Size > p2Size ? p2Size : p1Size;
		int tmp1, tmp2;
		for (size_t i = 0; i < pSize; ++i)
		{
			tmp1 = p1->mpVertIndice->operator[](i);
			tmp2 = p2->mpVertIndice->operator[](i);
			if (tmp1 > tmp2)
			{
				return 1;
			}
			else if (tmp1 < tmp2)
			{
				return -1;
			}
			// if res == 0, continue check next element.
		}
		// the one with larger size is considerred as the greater one.
		if (p1Size > p2Size)
		{
			return 1;
		}
		else if (p1Size < p2Size)
			return -1;

		return 0;
	}
}

// check if p1 is greater than p2
bool MirrorDataGreater(MirrorDataHelper* p1, MirrorDataHelper* p2)
{
	DbgAssert(p1->mbVertex == p2->mbVertex);

	if (p1->mbVertex)
	{
		return p1->mpVertex->x > p2->mpVertex->x;
	}
	else// (p1->mpVertIndice)
	{
		return (MirrorDataCompare(p1, p2, 0.0) > 0);
	}
}

int searchVec(std::vector<MirrorDataHelper*>& vec, const MirrorDataHelper* pTarget)
{
	int left = 0;
	int right = (int)vec.size() - 1;
	int mid = (right + left) / 2;
	MirrorDataHelper* pHelper = nullptr;
	int result = -1;
	size_t vecSize = vec.size();
	double tolerance = static_cast<double>(Point3Sort::MirrorThreshold);

	// find a start index whose sumOnMirrorAxis is within the allowed error range.
	while (left <= right)
	{
		mid = (right + left) / 2;
		pHelper = vec[mid];
		int res = MirrorDataCompare(pHelper, pTarget, tolerance);
		if (res == 0)
		{
			result = mid;
			break;
		}
		else if (res > 0)
			left = mid + 1;
		else
			right = mid - 1;
	}

	if (!pTarget->mbVertex)
	{
		return result;
	}

	// since we only sort vertex helper by x value, here we search down/up the sorted vector
	// to see if there are any other element meet the tolerance and use the best result.
	if (result >= 0)
	{
		double totalDelta = Point3Sort::GetPoint3Delta(*(vec[result]->mpVertex), *(pTarget->mpVertex));
		double tmp = 0.0;
		int candidate = result;
		bool searchDown = true;
		bool searchUp = true;
		int index = -1;
		for (auto i = 1; (i < vecSize) && (searchDown || searchUp); ++i)
		{
			if (totalDelta == 0.0) // found an exact match, stop searching.
			{
				return candidate;
			}

			if (searchDown)
			{
				index = result - i;
				pHelper = (index >= 0) ? vec[index] : nullptr;
				if (pHelper && 0 == MirrorDataCompare(pHelper, pTarget, tolerance))
				{
					tmp = Point3Sort::GetPoint3Delta(*(pHelper->mpVertex), *(pTarget->mpVertex));
					if (tmp < totalDelta)
					{
						totalDelta = tmp;
						candidate = index;
					}
				}
				else
				{
					searchDown = false;
				}
			}

			if (searchUp)
			{
				index = result + i;
				pHelper = (index < vecSize) ? vec[index] : nullptr;
				if (pHelper && 0 == MirrorDataCompare(pHelper, pTarget, tolerance))
				{
					tmp = Point3Sort::GetPoint3Delta(*(pHelper->mpVertex), *(pTarget->mpVertex));
					if (tmp < totalDelta)
					{
						totalDelta = tmp;
						candidate = index;
					}
				}
				else
				{
					searchUp = false;
				}
			}
		}

		if (totalDelta > 3.0 * tolerance) // point3 contains 3 float so the total delta must be within 3*tolerance.
		{
			return -1;
		}
		return candidate;
	}

	return -1;
}

void MeshTopoData::UpdateMirrorMatrix(MirrorAxisOptions axis, INode* pNode)
{
	if (pNode)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		Interval tmp;
		Matrix3 ntm = pNode->GetNodeTM(t, &tmp);
		Matrix3 otm = pNode->GetObjectTM(t, &tmp);
		Matrix3 intm = Inverse(ntm);
		Matrix3 tmpMatrix = otm*intm;
		Matrix3 itmpMatrix = Inverse(tmpMatrix);
		Point3 newRow(0.0f, 0.0f, 0.0f);
		newRow[axis] = -1.0f;
		Matrix3 mirrorAxisMat(TRUE);
		mirrorAxisMat.SetRow(axis, newRow);
		tmpMatrix = tmpMatrix*mirrorAxisMat*itmpMatrix;
		if (tmpMatrix != mMirrorMatrix)
		{
			InvalidateMirrorData();
			mMirrorMatrix = tmpMatrix; //set after invalidate, since in the above call, it will set mMirrorMatrix to identityMat
		}
	}
}

void MeshTopoData::BuildMirrorData(MirrorAxisOptions axis, int subObjLvl, float threshold, INode* pNode)
{
	UpdateMirrorMatrix(axis, pNode);
	Point3Sort::MirrorThreshold = threshold;
	BuildVertMirrorData();
	// edge and face mirror are based on vertice mirror info
	BuildEdgeMirrorData();
	BuildFaceMirrorData();
	mMirrorBoundingBox = GetBoundingBox(); // cache the local bounding box.
}

void MeshTopoData::BuildVertMirrorData()
{
	int newGeomVertSize = TVMaps.geomPoints.Count();
	if (newGeomVertSize != mGVSel.GetSize())
	{
		DbgAssert(FALSE);
		mGeomVertMirrorDataList.ZeroCount();
		return;
	}

	if (mGeomVertMirrorDataList.Count() == newGeomVertSize)
	{
		return;
	}

	std::vector<MirrorDataHelper*> dataHelperVec;
	dataHelperVec.resize(newGeomVertSize);
	mGeomVertMirrorDataList.SetCount(newGeomVertSize);

	MirrorDataHelper* pHelper = nullptr;
	Point3 curPoint;
	for (int i = 0; i < newGeomVertSize; ++i)
	{
		curPoint = GetGeomVert(i);
		pHelper = new MirrorDataHelper(curPoint, i, !DoesGeomVertexPassFilter(i));
		dataHelperVec[i] = (pHelper);
		mGeomVertMirrorDataList[i].index = -1;
	}

	CalculateMirrorIndex(mGeomVertMirrorDataList, dataHelperVec, nullptr);
}

void MeshTopoData::MirrorGeomVertSel(int vertIndex, BOOL bSel)
{
	int geomVertCount = TVMaps.geomPoints.Count();
	int geomVSelCount = mGVSel.GetSize();
	if (geomVertCount != geomVSelCount)
	{
		DbgAssert(0);
		mGVSel.SetSize(TVMaps.geomPoints.Count());
		ClearGeomVertSelection();
		return;
	}

	int mirrorIndex = -1;
	if (vertIndex >= 0)
	{
		mirrorIndex = GetGeomVertMirrorIndex(vertIndex);
		if (mirrorIndex >= 0 && mirrorIndex < geomVSelCount)
		{
			SetGeomVertSelected(mirrorIndex, bSel, FALSE);
		}
	}
	else
	{
		for (int i = 0; i < geomVertCount; ++i)
		{
			if (mGVSel[i])
			{
				mirrorIndex = GetGeomVertMirrorIndex(i);
				if (mirrorIndex >= 0 && mirrorIndex < geomVSelCount)
				{
					SetGeomVertSelected(mirrorIndex, TRUE, FALSE);
				}
			}
		}
	}
}

void MeshTopoData::ReleaseAdjFaceList()
{
	if (mpMeshAdjFaceData)
	{
		delete mpMeshAdjFaceData;
		mpMeshAdjFaceData = nullptr;
	}
}

AdjFaceList* MeshTopoData::GetMeshAdjFaceList()
{
	if (mpMeshAdjFaceData)
	{
		return mpMeshAdjFaceData;
	}

	if (this->GetMesh())
	{
		AdjEdgeList ae(*GetMesh());
		mpMeshAdjFaceData = new AdjFaceList(*GetMesh(), ae);
		return mpMeshAdjFaceData;
	}
	return nullptr;
}

namespace
{
	Point3 FaceNormal(DWORD fi, BOOL nrmlize, Mesh& mesh)
	{
		DWORD *fv = mesh.faces[fi].v;
		Point3 v1 = mesh.verts[fv[1]] - mesh.verts[fv[0]];
		Point3 v2 = mesh.verts[fv[2]] - mesh.verts[fv[1]];
		if (nrmlize) return Normalize(v1^v2);
		else return v1^v2;
	}

	float AngleBetweenFaces(DWORD f0, DWORD f1, Mesh& mesh)
	{
		if (f0 == UNDEFINED || f1 == UNDEFINED) return DegToRad(180);
		Point3 n0 = FaceNormal(f0, TRUE, mesh);
		Point3 n1 = FaceNormal(f1, TRUE, mesh);
		float dp = DotProd(n0, n1);
		if (dp > float(1)) return 0.0f;
		if (dp < float(-1)) return DegToRad(180);
		return float(acos(dp));
	}
}

void MeshTopoData::GetPolyFromFaces(int faceIndex, BitArray& set, BOOL bMirrorSel)
{
	if (faceIndex >= 0 && faceIndex < TVMaps.f.Count())
	{
		BuildPolyFromFace(faceIndex);
		int polyIdx = mFacePolyIndexTable[faceIndex];
		if (polyIdx >= 0 && polyIdx < mMeshPolyData.size())
		{
			for (auto it = mMeshPolyData[polyIdx]->mFaceList.begin(); it != mMeshPolyData[polyIdx]->mFaceList.end(); ++it)
			{
				set.Set(*it, TRUE);
			}
			// add the mirrored face index to the set.
			if (bMirrorSel && polyIdx < mGeomFaceMirrorDataList.Count())
			{
				int mirrorIdx = mGeomFaceMirrorDataList[polyIdx].index;

				if (mirrorIdx >= 0 && mirrorIdx < mMeshPolyData.size())
				{
					for (auto it = mMeshPolyData[mirrorIdx]->mFaceList.begin(); it != mMeshPolyData[mirrorIdx]->mFaceList.end(); ++it)
					{
						set.Set(*it, TRUE);
					}
				}
			}
		}
	}
}

void MeshTopoData::InvalidateMeshPolyData()
{
	// clear the previous data.
	for (auto it = mMeshPolyData.begin(); it != mMeshPolyData.end(); ++it)
	{
		delete *it;
	}
	mMeshPolyData.clear();
	int faceCount = TVMaps.f.Count();
	mFacePolyIndexTable.resize(faceCount);
	for (int i = 0; i < faceCount; ++i)
	{
		mFacePolyIndexTable[i] = -1;
	}
}

void MeshTopoData::BuildPolyFromFace(int f, float thresh, bool bIgnoreVisEdge)
{
	Mesh* pMesh = GetMesh();
	if (pMesh == nullptr)
		return; //only valid for mesh

	int faceCount = TVMaps.f.Count();
	if (mFacePolyIndexTable.size() == 0)
	{
		mFacePolyIndexTable.resize(faceCount);
		for (int i = 0; i < faceCount; ++i)
		{
			mFacePolyIndexTable[i] = -1;
		}
	}

	if (mFacePolyIndexTable[f] != -1)
	{
		// we already which poly the face belong to, skip;
		return;
	}

	int curPolyIndex = (int)mMeshPolyData.size() - 1;

	++curPolyIndex; // now a new poly is found.
	mMeshPolyData.push_back(new MeshPolyData());
	DbgAssert(mMeshPolyData.size() == curPolyIndex + 1);
	AdjFaceList* af = GetMeshAdjFaceList();
	thresh = (float)fabs(thresh);
	thresh = DegToRad(thresh);
	std::vector<int> curPolyFaces;
	int curFaceIdx = -1;
	curPolyFaces.clear();
	curPolyFaces.push_back(f);
	while (!curPolyFaces.empty())
	{
		auto it = curPolyFaces.end() - 1;
		curFaceIdx = *it;
		curPolyFaces.pop_back();

		mFacePolyIndexTable[curFaceIdx] = curPolyIndex;
		mMeshPolyData[curPolyIndex]->mFaceList.push_back(curFaceIdx);

		// Push neighbors on the stack
		for (int i = 0; i < 3; ++i)
		{
			// Adjacent face index
			DWORD of = (*af)[curFaceIdx].f[i];
			// If it's been done, then continue;
			if (of == UNDEFINED || mFacePolyIndexTable[of] != -1) continue;
			// It must be adjacent across hidden edges and within
			// the threshold angle.
			if (bIgnoreVisEdge || !pMesh->faces[curFaceIdx].getEdgeVis(i)) {
				Face *face = &(pMesh->faces[of]);
				DWORD v0 = pMesh->faces[curFaceIdx].v[i];
				DWORD v1 = pMesh->faces[curFaceIdx].v[(i + 1) % 3];
				if (bIgnoreVisEdge || !face->getEdgeVis(face->GetEdgeIndex(v0, v1))) {
					if (AngleBetweenFaces(curFaceIdx, of, *pMesh) < thresh)
						curPolyFaces.push_back(of);
				}
			}
		}
	}
}

void MeshTopoData::BuildMeshPolyData(float thresh, bool bIgnoreVisEdge)
{
	Mesh* pMesh = GetMesh();
	if (pMesh == nullptr || mMeshPolyData.size() > 0)
		return; //only valid for mesh

	int faceCount = TVMaps.f.Count();
	DbgAssert(faceCount == GetMesh()->numFaces);
	mMeshPolyData.reserve(faceCount);

	for (int f = 0; f < faceCount; ++f)
	{
		BuildPolyFromFace(f, thresh, bIgnoreVisEdge);
	}
	mMeshPolyData.shrink_to_fit();
}

void MeshTopoData::BuildMeshFaceMirrorData()
{
	if (GetMesh() == nullptr)
	{
		return;
	}

	BuildMeshPolyData(); // build all the faces' poly data

	int meshPolyCount = (int)mMeshPolyData.size();

	if (mGeomFaceMirrorDataList.Count() == meshPolyCount)
	{
		return;
	}

	std::vector<MirrorDataHelper*> faceHelperVec;
	faceHelperVec.resize(meshPolyCount);
	mGeomFaceMirrorDataList.SetCount(meshPolyCount);

	MirrorDataHelper* pHelper = nullptr;

	for (int i = 0; i < meshPolyCount; ++i)
	{
		bool bHidden = false;
		if (mMeshPolyData[i]->mFaceList.size() > 0)
		{
			bHidden = !DoesFacePassFilter(mMeshPolyData[i]->mFaceList[0]);
		}
		pHelper = new MirrorDataHelper(mMeshPolyData[i], TVMaps, i, bHidden);
		faceHelperVec[i] = (pHelper);
		mGeomFaceMirrorDataList[i].index = -1;
	}

	CalculateMirrorIndex(mGeomFaceMirrorDataList, faceHelperVec, &mGeomVertMirrorDataList);
}

void MeshTopoData::BuildEdgeMirrorData()
{
	int geomEdgeCount = GetNumberGeomEdges();
	if (geomEdgeCount != mGESel.GetSize())
	{
		DbgAssert(FALSE);
		mGeomEdgeMirrorDataList.ZeroCount();
		return;
	}

	if (mGeomEdgeMirrorDataList.Count() == geomEdgeCount)
	{
		return;//use the cached data!
	}

	std::vector<MirrorDataHelper*> dataHelperVec;
	dataHelperVec.resize(geomEdgeCount);
	mGeomEdgeMirrorDataList.SetCount(geomEdgeCount);

	MirrorDataHelper* pHelper = nullptr;
	UVW_TVEdgeDataClass* pEdge = nullptr;
	int edgeIndex[2];
	for (int i = 0; i < geomEdgeCount; ++i)
	{
		pEdge = TVMaps.gePtrList[i];
		edgeIndex[0] = GetGeomEdgeVert(i, 0);
		edgeIndex[1] = GetGeomEdgeVert(i, 1);
		pHelper = new MirrorDataHelper(edgeIndex, 2, TVMaps.geomPoints, i, !DoesGeomEdgePassFilter(i));
		dataHelperVec[i] = (pHelper);
		mGeomEdgeMirrorDataList[i].index = -1;
	}

	CalculateMirrorIndex(mGeomEdgeMirrorDataList, dataHelperVec, &mGeomVertMirrorDataList);
}

void MeshTopoData::CalculateMirrorIndex(
	Tab<GeomSelMirrorData>& mirrorTable, std::vector<MirrorDataHelper*>& dataHelperVec, const Tab<GeomSelMirrorData>* pVertTab)
{
	int count = mirrorTable.Count();
	DbgAssert(count == (int)dataHelperVec.size());
	std::sort(dataHelperVec.begin(), dataHelperVec.end(), MirrorDataGreater);

	for (int i = 0; i < count; ++i)
	{
		MirrorDataHelper* pHelper = dataHelperVec[i];
		int index1 = pHelper->mIndex;
		if (pHelper->mHidden) // if hidden, reset mirror info
		{
			if (mirrorTable[index1].index >= 0)
			{
				int mirrorIndex = mirrorTable[index1].index;
				mirrorTable[mirrorIndex].index = sNotFoundMirrorIndex;
			}
			mirrorTable[index1].index = sNotFoundMirrorIndex;
			continue;
		}
		if (mirrorTable[index1].index == -1) //unprocessed, process it. 
		{
			bool bFound = false;

			MirrorDataHelper* pMirroredHelper = pHelper->CreateMirroredDataHelper(mMirrorMatrix, pVertTab);
			int res = searchVec(dataHelperVec, pMirroredHelper);
			delete pMirroredHelper;
			int index2;
			if (res >= 0 && res < dataHelperVec.size())
			{
				MirrorDataHelper* pRes = dataHelperVec[res];
				if (pRes && !pRes->mHidden)
				{
					index2 = pRes->mIndex;
					mirrorTable[index1].index = index2;
					mirrorTable[index2].index = index1;
					bFound = true;
				}
			}
			if (!bFound)
				mirrorTable[index1].index = sNotFoundMirrorIndex;
		}
	}

	for (int i = 0; i < count; ++i)
	{
		delete dataHelperVec[i];
	}
	dataHelperVec.clear();
}

void MeshTopoData::BuildFaceMirrorData()
{
	if (GetMesh() != nullptr)
	{
		BuildMeshFaceMirrorData();
		return;
	}
	int faceCount = TVMaps.f.Count();
	if (faceCount != mFSel.GetSize())
	{
		DbgAssert(FALSE); // faceSel data is invalid
		mGeomFaceMirrorDataList.ZeroCount();
		return;
	}

	if (mGeomFaceMirrorDataList.Count() == faceCount)
	{
		return;//use the cached data!
	}

	std::vector<MirrorDataHelper*> faceHelperVec;
	faceHelperVec.resize(faceCount);

	mGeomFaceMirrorDataList.SetCount(faceCount);

	UVW_TVFaceClass* pFace = nullptr;
	MirrorDataHelper* pHelper = nullptr;
	for (int i = 0; i < faceCount; ++i)
	{
		pFace = TVMaps.f[i];
		pHelper = new MirrorDataHelper(pFace->v, pFace->count, TVMaps.geomPoints, i, !DoesFacePassFilter(i));
		faceHelperVec[i] = (pHelper);
		mGeomFaceMirrorDataList[i].index = -1;
	}

	CalculateMirrorIndex(mGeomFaceMirrorDataList, faceHelperVec, &mGeomVertMirrorDataList);
}

int MeshTopoData::GetGeomEdgeMirrorIndex(int index) const
{
	if (index >= 0 && index < mGeomEdgeMirrorDataList.Count())
	{
		return mGeomEdgeMirrorDataList[index].index;
	}
	return -1;
}

int MeshTopoData::GetGeomVertMirrorIndex(int index) const
{
	if (index >= 0 && index < mGeomVertMirrorDataList.Count())
	{
		return mGeomVertMirrorDataList[index].index;
	}
	return -1;
}

void MeshTopoData::MirrorGeomEdgeSel(int edgeIndex, BOOL bSel)
{
	int geomEdgeCount = GetNumberGeomEdges();
	if (geomEdgeCount != mGeomEdgeMirrorDataList.Count())
	{
		DbgAssert(FALSE); //the geom vertex mirror data is not built correctly yet
		return;
	}

	int mirrorIndex = -1;
	if (edgeIndex >= 0)
	{
		mirrorIndex = GetGeomEdgeMirrorIndex(edgeIndex);
		if (mirrorIndex >= 0 && mirrorIndex < mGESel.GetSize())
			SetGeomEdgeSelected(mirrorIndex, bSel, FALSE);
	}
	else
	{
		for (int i = 0; i < geomEdgeCount; ++i)
		{
			if (GetGeomEdgeSelected(i))
			{
				mirrorIndex = GetGeomEdgeMirrorIndex(i);
				if (mirrorIndex >= 0 && mirrorIndex < geomEdgeCount)
				{
					SetGeomEdgeSelected(mirrorIndex, TRUE, FALSE);
				}
			}
		}
	}
}

void MeshTopoData::MirrorMeshFaceSel(int faceIndex, BOOL bSel, BOOL bPreview)
{
	if (GetMesh() != nullptr)
	{
		int faceCount = mGeomFaceMirrorDataList.Count();
		if (faceIndex >= 0 && faceIndex < mFacePolyIndexTable.size())
		{
			int polyIndex = mFacePolyIndexTable[faceIndex];
			int mirrorIndex = mGeomFaceMirrorDataList[polyIndex].index;
			if (mirrorIndex >= 0 && mirrorIndex < mMeshPolyData.size())
			{
				MeshPolyData* pData = mMeshPolyData[mirrorIndex];
				int faceMirrorIndex = -1;
				for (int i = 0; i < pData->mFaceList.size(); ++i)
				{
					faceMirrorIndex = pData->mFaceList[i];
					if (faceMirrorIndex >= 0 && faceMirrorIndex < mFSel.GetSize() && !bPreview)
					{
						SetFaceSelected(faceMirrorIndex, bSel, FALSE);
					}
					else if (faceMirrorIndex >= 0 && faceMirrorIndex < mFSelPreview.GetSize() && bPreview)
					{
						SetFaceSelectedPreview(faceMirrorIndex, bSel, FALSE);
					}
				}
			}
		}
		else
		{
			faceCount = (int)mFacePolyIndexTable.size();
			for (int i = 0; i < faceCount; ++i)
			{
				if (mFSel[i])
				{
					int polyIndex = mFacePolyIndexTable[i];
					int mirrorIndex = mGeomFaceMirrorDataList[polyIndex].index;
					if (mirrorIndex >= 0 && mirrorIndex < mMeshPolyData.size())
					{
						MeshPolyData* pData = mMeshPolyData[mirrorIndex];
						int faceMirrorIndex = -1;
						for (int f = 0; f < pData->mFaceList.size(); ++f)
						{
							faceMirrorIndex = pData->mFaceList[f];
							if (faceMirrorIndex >= 0 && faceMirrorIndex < mFSel.GetSize() && !bPreview)
							{
								SetFaceSelected(faceMirrorIndex, TRUE, FALSE);
							}
							else if (faceMirrorIndex >= 0 && faceMirrorIndex < mFSelPreview.GetSize() && bPreview)
							{
								SetFaceSelectedPreview(faceMirrorIndex, TRUE, FALSE);
							}
						}
					}
				}
			}
		}
	}
}

void MeshTopoData::MirrorFaceSel(int faceIndex, BOOL bSel, BOOL bPreview)
{
	if (GetMesh() != nullptr)
	{
		MirrorMeshFaceSel(faceIndex, bSel, bPreview);
		return;
	}

	int faceCount = mGeomFaceMirrorDataList.Count();
	if (faceCount != mFSel.GetSize())
	{
		DbgAssert(FALSE); // faceSel data is invalid
		return;
	}

	int mirrorIndex = -1;
	if (faceIndex >= 0)
	{
		mirrorIndex = mGeomFaceMirrorDataList[faceIndex].index;
		if (mirrorIndex >= 0 && mirrorIndex < mFSel.GetSize() && !bPreview)
		{
			SetFaceSelected(mirrorIndex, bSel, FALSE);
		}
		else if (mirrorIndex >= 0 && mirrorIndex < mFSelPreview.GetSize() && bPreview)
		{
			SetFaceSelectedPreview(mirrorIndex, bSel, FALSE);
		}
	}
	else
	{
		for (int i = 0; i < faceCount; ++i)
		{
			if (mFSel[i])
			{
				mirrorIndex = mGeomFaceMirrorDataList[i].index;
				if (mirrorIndex >= 0 && mirrorIndex < faceCount && !bPreview)
				{
					SetFaceSelected(mirrorIndex, TRUE, FALSE);
				}
				else if (mirrorIndex >= 0 && mirrorIndex < faceCount && bPreview)
				{
					SetFaceSelectedPreview(mirrorIndex, TRUE, FALSE);
				}
			}
		}
	}
}

int MeshTopoData::GetNumberGeomEdges()
{
	return TVMaps.gePtrList.Count();
}

BitArray MeshTopoData::ComputeOpenGeomEdges()
{
	BitArray openGeomEdges;
	ConvertTVEdgeSelectionToGeom(GetOpenTVEdges(), openGeomEdges);

	return openGeomEdges;
}

BitArray MeshTopoData::GetOpenGeomEdges()
{
	return mOpenGeomEdges;
}

BitArray MeshTopoData::ComputeOpenTVEdges()
{
	BitArray openTVEdges(GetNumberTVEdges());

	for (int tvEIndex = 0; tvEIndex < openTVEdges.GetSize(); tvEIndex++)
	{
		bool isOpenEdge = GetTVEdgeNumberTVFaces(tvEIndex) == 1;
		if (isOpenEdge) openTVEdges.Set(tvEIndex);
	}

	return openTVEdges;
}

BitArray MeshTopoData::GetOpenTVEdges()
{
	return mOpenTVEdges;
}

void MeshTopoData::SetAllGeomEdgeSelection()
{
	mGESel.SetAll();
	UpdateGeomEdgeSelInIsolationMode();
}

BitArray *MeshTopoData::GetGeomEdgeSelectionPtr()
{
	return &mGESel;
}

BitArray MeshTopoData::GetGeomEdgeSelection()
{
	return mGESel;
}

const BitArray&  MeshTopoData::GetGeomEdgeSel()
{
	return mGESel;
}

void MeshTopoData::SetGeomEdgeSel(const BitArray& newSel, BOOL bMirrorSel)
{
	//Once the geometry edge selection is changed,the corresponding render item (UnwrapRenderItem)
	//will update the attribute buffer of selection by checking this dirty flag.
	SetSynchronizationToRenderItemFlag(TRUE);

	if (newSel.GetSize() != TVMaps.gePtrList.Count())
	{
		// In Clone operations, TVMaps.gePtrList.Resize(0) will cause this to fire, so make that case OK and just catch all others
		if (TVMaps.gePtrList.Count() != 0)
		{
			DbgAssert(0);
		}
		mGESel = newSel;
		return;
	}

	mGESel = newSel;

	if (bMirrorSel)
	{
		MirrorGeomEdgeSel();
	}
	UpdateGeomEdgeSelInIsolationMode();
}

void MeshTopoData::UpdateGeomEdgeSelInIsolationMode()
{
	if (IsGeomEdgeFilterValid())
	{
		mGESel &= mGeomEdgeFilter;
	}
}

void MeshTopoData::SetGeomEdgeSelection(BitArray newSel, BOOL bMirrorSel)
{
	SetGeomEdgeSel(newSel, bMirrorSel);
}

BOOL MeshTopoData::IsOpenGeomEdge(int index)
{
	if ((index < 0) || (index >= mOpenGeomEdges.GetSize()))
	{
		return FALSE;
	}
	return mOpenGeomEdges[index];
}

void MeshTopoData::SetGeomEdgeSelected(int index, BOOL sel, BOOL bMirrorSel)
{
	if ((index < 0) || (index >= mGESel.GetSize()))
	{
		DbgAssert(0);
		return;
	}
	if (IsGeomEdgeFilterValid())
	{
		sel &= static_cast<BOOL>(DoesGeomEdgePassFilter(index));
	}

	mGESel.Set(index, sel);

	if (bMirrorSel)
	{
		MirrorGeomEdgeSel(index, sel);
	}

	//Once the geometry edge selection is changed,the corresponding render item (UnwrapRenderItem)
	//will update the attribute buffer of selection by checking this dirty flag.
	SetSynchronizationToRenderItemFlag(TRUE);
}


int MeshTopoData::GetGeomEdgeVert(int edgeIndex, int whichEnd) const
{
	if ((edgeIndex < 0) || (edgeIndex >= TVMaps.gePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	if (whichEnd == 0)
		return TVMaps.gePtrList[edgeIndex]->a;
	else
		return TVMaps.gePtrList[edgeIndex]->b;
}

int MeshTopoData::GetGeomEdgeNumberOfConnectedFaces(int index) const
{
	if ((index < 0) || (index >= TVMaps.gePtrList.Count()))
	{
		DbgAssert(0);
		return 0;
	}
	else
		return TVMaps.gePtrList[index]->faceList.Count();
}
int MeshTopoData::GetGeomEdgeConnectedFace(int index, int findex) const
{
	if ((index < 0) || (index >= TVMaps.gePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else if ((findex < 0) || (findex >= TVMaps.gePtrList[index]->faceList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	else
		return TVMaps.gePtrList[index]->faceList[findex];
}


BOOL MeshTopoData::GetGeomEdgeHidden(int index) const
{
	if ((index < 0) || (index >= TVMaps.gePtrList.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	return TVMaps.gePtrList[index]->flags & FLAG_HIDDENEDGEA;
}

int MeshTopoData::GetGeomEdgeVec(int edgeIndex, int whichEnd) const
{
	if ((edgeIndex < 0) || (edgeIndex >= TVMaps.gePtrList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	if (whichEnd == 0)
		return TVMaps.gePtrList[edgeIndex]->avec;
	else
		return TVMaps.gePtrList[edgeIndex]->bvec;
}

int MeshTopoData::GetFaceGeomVert(int faceIndex, int ithVert)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	if ((ithVert < 0) || (ithVert >= TVMaps.f[faceIndex]->count))
	{
		DbgAssert(0);
		return -1;
	}

	return  TVMaps.f[faceIndex]->v[ithVert];

}

bool MeshTopoData::ResizeTVVertSelection(int newSize, int save/*= 0*/)
{
	if (newSize != mVSel.GetSize())
	{
		mVSel.SetSize(newSize, save);
		return true;
	}
	return false;
}

void MeshTopoData::SetAllTVVertSelection()
{
	mVSel.SetAll();
	UpdateTVVertSelInIsolationMode();
}

void MeshTopoData::ClearTVVertSelection()
{
	mVSel.ClearAll();
}

void MeshTopoData::ClearTVVertSelectionPreview()
{
	mVSelPreview.ClearAll();
}

void MeshTopoData::SetAllGeomVertSelection()
{
	mGVSel.SetAll();
	UpdateGeomVertSelInIsolationMode();
}

void MeshTopoData::ClearGeomVertSelection()
{
	mGVSel.ClearAll();
}

void MeshTopoData::ClearTVEdgeSelection()
{
	mESel.ClearAll();
}

void MeshTopoData::ClearTVEdgeSelectionPreview()
{
	mESelPreview.ClearAll();
}

void MeshTopoData::ClearGeomEdgeSelection()
{
	mGESel.ClearAll();
}

void MeshTopoData::SetAllFaceSelection()
{
	mFSel.SetAll();
	UpdateFaceSelInIsolationMode();
	RaiseNotifyFaceSelectionChanged();
}

void MeshTopoData::ClearFaceSelection()
{
	mFSel.ClearAll();
	RaiseNotifyFaceSelectionChanged();
}

void MeshTopoData::ClearFaceSelectionPreview()
{
	mFSelPreview.ClearAll();
}

int MeshTopoData::FindGeomEdge(int faceIndex, int a, int b)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->FindGeomEdge(a, b);
}

int MeshTopoData::FindUVEdge(int faceIndex, int a, int b)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->FindUVEdge(a, b);
}


ULONG MeshTopoData::SaveFaces(ISave *isave)
{
	return TVMaps.SaveFaces(isave);
}

ULONG MeshTopoData::SaveTVVerts(ISave *isave)
{
	int vct = TVMaps.v.Count();
	ULONG nb = 0;
	isave->Write(&vct, sizeof(vct), &nb);
	return isave->WriteVoid(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*vct, &nb); // no TCHAR data
}

ULONG MeshTopoData::SaveGeoVerts(ISave *isave)
{
	int vct = TVMaps.geomPoints.Count();
	ULONG nb = 0;
	isave->Write(&vct, sizeof(vct), &nb);
	return isave->Write(TVMaps.geomPoints.Addr(0), sizeof(Point3)*vct, &nb);
}


ULONG MeshTopoData::LoadFaces(ILoad *iload)
{
	ULONG iret = TVMaps.LoadFaces(iload);
	mFSel.SetSize(TVMaps.f.Count());
	ClearFaceSelection();
	mFSelPreview = GetFaceSel();
	return iret;
}
ULONG MeshTopoData::LoadTVVerts(ILoad *iload)
{
	ULONG nb = 0;
	int ct = 0;
	iload->Read(&ct, sizeof(ct), &nb);
	TVMaps.v.SetCount(ct);

	ResizeTVVertSelection(TVMaps.v.Count());
	ClearTVVertSelection();

	mVSelPreview.SetSize(TVMaps.v.Count());
	ClearTVVertSelectionPreview();

    ULONG loadFlag = iload->ReadVoid(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*TVMaps.v.Count(), &nb);
    ValidateTVVertices();

	return loadFlag;
}
ULONG MeshTopoData::LoadGeoVerts(ILoad *iload)
{
	ULONG nb = 0;
	int ct = 0;
	iload->Read(&ct, sizeof(ct), &nb);
	TVMaps.geomPoints.SetCount(ct);
	return iload->Read(TVMaps.geomPoints.Addr(0), sizeof(Point3)*TVMaps.geomPoints.Count(), &nb);
}

BOOL MeshTopoData::IsTVEdgeValid()
{
	return TVMaps.edgesValid;
}

void MeshTopoData::SetTVEdgeInvalid()
{
	RaiseTopoInvalidated();
	//mLSCM->InvalidateTopo(this);
	TVMaps.edgesValid = FALSE;
}

void MeshTopoData::SetGeoEdgeInvalid()
{
	TVMaps.mGeoEdgesValid = FALSE;
}

ULONG MeshTopoData::LoadTVVertSelection(ILoad *iload)
{
	return mVSel.Load(iload);
}

ULONG MeshTopoData::LoadGeoVertSelection(ILoad *iload)
{
	return mGVSel.Load(iload);
}

ULONG MeshTopoData::LoadTVEdgeSelection(ILoad *iload)
{
	ULONG result = mESel.Load(iload);
	SetTVEdgeSelectionPreview(mESel);
	return result;
}
ULONG MeshTopoData::LoadGeoEdgeSelection(ILoad *iload)
{
	return mGESel.Load(iload);
}
ULONG MeshTopoData::LoadFaceSelection(ILoad *iload)
{
	ULONG ret = mFSel.Load(iload);
	RaiseNotifyFaceSelectionChanged();
	return ret;
}

ULONG MeshTopoData::SaveTVVertSelection(ISave *isave)
{
	return mVSel.Save(isave);
}

ULONG MeshTopoData::SaveGeoVertSelection(ISave *isave)
{
	return mGVSel.Save(isave);
}

ULONG MeshTopoData::SaveTVEdgeSelection(ISave *isave)
{
	return mESel.Save(isave);
}
ULONG MeshTopoData::SaveGeoEdgeSelection(ISave *isave)
{
	return mGESel.Save(isave);
}
ULONG MeshTopoData::SaveFaceSelection(ISave *isave)
{
	return mFSel.Save(isave);
}

void MeshTopoData::ResolveOldData(UVW_ChannelClass &oldData)
{
	//see if we have matching face data if not bail
	TVMaps.ResolveOldData(oldData);
}

void MeshTopoData::SaveOldData(ISave *isave)
{
	ULONG nb;

	int vct = GetNumberTVVerts();//TVMaps.v.Count(), 
	int fct = GetNumberFaces();//TVMaps.f.Count();

	isave->BeginChunk(VERTCOUNT_CHUNK);
	isave->Write(&vct, sizeof(vct), &nb);
	isave->EndChunk();

	if (vct) {
		isave->BeginChunk(VERTS2_CHUNK);
		//FIX
		Tab<UVW_TVVertClass_Max9> oldTVData;
		oldTVData.SetCount(vct);
		for (int i = 0; i < vct; i++)
		{
			oldTVData[i].p = TVMaps.v[i].GetP();
			oldTVData[i].flags = TVMaps.v[i].GetFlag();
			oldTVData[i].influence = TVMaps.v[i].GetInfluence();
		}

		isave->WriteVoid(oldTVData.Addr(0), sizeof(UVW_TVVertClass_Max9)*vct, &nb); // no TCHAR data
		isave->EndChunk();
	}

	isave->BeginChunk(FACECOUNT_CHUNK);
	isave->Write(&fct, sizeof(fct), &nb);
	isave->EndChunk();

	if (fct) {
		isave->BeginChunk(FACE4_CHUNK);
		TVMaps.SaveFacesMax9(isave);
		isave->EndChunk();
	}

	isave->BeginChunk(VERTSEL_CHUNK);
	mVSel.Save(isave);
	isave->EndChunk();

	fct = TVMaps.geomPoints.Count();
	isave->BeginChunk(GEOMPOINTSCOUNT_CHUNK);
	isave->Write(&fct, sizeof(fct), &nb);
	isave->EndChunk();

	if (fct) {
		isave->BeginChunk(GEOMPOINTS_CHUNK);
		isave->Write(TVMaps.geomPoints.Addr(0), sizeof(Point3)*fct, &nb);
		isave->EndChunk();
	}


	isave->BeginChunk(FACESELECTION_CHUNK);
	mFSel.Save(isave);
	isave->EndChunk();


	isave->BeginChunk(UEDGESELECTION_CHUNK);
	mESel.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(GEDGESELECTION_CHUNK);
	mGESel.Save(isave);
	isave->EndChunk();
}


int MeshTopoData::GetFaceGeomHandle(int faceIndex, int ithVert)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->vecs->vhandles[ithVert];
}

int MeshTopoData::GetFaceGeomInterior(int faceIndex, int ithVert)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->vecs->vinteriors[ithVert];
}

int MeshTopoData::FindGeoEdge(int a, int b)
{
	return TVMaps.FindGeoEdge(a, b);
}

int MeshTopoData::FindUVEdge(int a, int b)
{
	return TVMaps.FindUVEdge(a, b);
}

int  MeshTopoData::GetFaceMatID(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->GetMatID();
}

void  MeshTopoData::SetFaceMatID(int faceIndex, int matID)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return;
	}
	TVMaps.f[faceIndex]->SetMatID(matID);
}


BOOL MeshTopoData::GetFaceDead(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return TVMaps.f[faceIndex]->flags & FLAG_DEAD;
}

void MeshTopoData::SetFaceDead(int faceIndex, BOOL dead)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return;
	}
	if (dead)
		TVMaps.f[faceIndex]->flags |= FLAG_DEAD;
	else TVMaps.f[faceIndex]->flags &= ~FLAG_DEAD;
}

void MeshTopoData::CleanUpDeadVertices()
{
	BitArray usedList;

	usedList.SetSize(GetNumberTVVerts());//TVMaps.v.Count());
	usedList.ClearAll();

	for (int i = 0; i < GetNumberFaces(); i++)//TVMaps.f.Count(); i++)
	{
		if (!GetFaceDead(i))
		{
			int degree = GetFaceDegree(i);
			for (int j = 0; j < degree; j++)
			{
				int vertIndex = GetFaceTVVert(i, j);//TVMaps.f[i]->t[j];
				usedList.Set(vertIndex);

				//index into the geometric vertlist
				if ((GetFaceHasVectors(i)/*TVMaps.f[i]->vecs*/) && (j < 4))
				{
					vertIndex = GetFaceTVInterior(i, j);//TVMaps.f[i]->vecs->interiors[j];
					if ((vertIndex >= 0) && (vertIndex < usedList.GetSize()))
						usedList.Set(vertIndex);

					vertIndex = GetFaceTVHandle(i, j * 2);//TVMaps.f[i]->vecs->handles[j*2];
					if ((vertIndex >= 0) && (vertIndex < usedList.GetSize()))
						usedList.Set(vertIndex);
					vertIndex = GetFaceTVHandle(i, j * 2 + 1);//TVMaps.f[i]->vecs->handles[j*2+1];
					if ((vertIndex >= 0) && (vertIndex < usedList.GetSize()))
						usedList.Set(vertIndex);
				}

			}
		}
	}

	int vInitalDeadCount = 0;
	int vFinalDeadCount = 0;

	for (int i = 0; i < GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++)
	{
		if (GetTVVertDead(i))//TVMaps.v[i].flags & FLAG_DEAD)
			vInitalDeadCount++;
	}

	for (int i = 0; i < usedList.GetSize(); i++)
	{
		BOOL isRigPoint = GetTVVertFlag(i) & FLAG_RIGPOINT;
		if (!usedList[i] && (!isRigPoint))
		{
			DeleteTVVert(i);
			//					TVMaps.v[i].flags |= FLAG_DEAD;
		}
	}

	for (int i = 0; i < GetNumberTVVerts(); i++)
	{
		if (GetTVVertDead(i))//TVMaps.v[i].flags & FLAG_DEAD)
			vFinalDeadCount++;
	}


#ifdef DEBUGMODE 

	if (gDebugLevel >= 3)
		ScriptPrint(_T("Cleaning Dead Verts Total Verts %d Initial Dead Verts %d Final Dead Verts %d \n"), vTotalCount, vInitalDeadCount, vFinalDeadCount);

#endif
}

BOOL MeshTopoData::GetFaceHidden(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	int deg = TVMaps.f[faceIndex]->count;
	for (int i = 0; i < deg; i++)
	{
		int id = TVMaps.f[faceIndex]->t[i];

		if (!IsTVVertVisible(id))
			return TRUE;
	}
	return FALSE;
}


Point3 MeshTopoData::GetGeomFaceNormal(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return Point3(1.0f, 0.0f, 0.0f);
	}
	return TVMaps.GeomFaceNormal(faceIndex);
}
Point3 MeshTopoData::GetUVFaceNormal(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return Point3(1.0f, 0.0f, 0.0f);
	}

	return TVMaps.UVFaceNormal(faceIndex);
}

static BOOL EdgeSelectionFromFaceSelection(int faceIndex, int edgeIndex, const BitArray& faceSelection, BitArray& edgeSelection, BOOL& bOutOfRange)
{
	if (faceIndex < 0 && faceIndex >= faceSelection.GetSize())
	{
		bOutOfRange = TRUE;
		return FALSE;
	}
	if (faceSelection[faceIndex])
	{
		edgeSelection.Set(edgeIndex);
		return TRUE;
	}
	return FALSE;
}

void MeshTopoData::ConvertFaceToEdgeSelection(BOOL bPreview)
{
	if (bPreview)
	{
		if (mESelPreview.GetSize() != mESel.GetSize())
		{
			mESelPreview.SetSize(mESel.GetSize());
		}
		ClearTVEdgeSelectionPreview();
	}
	else
	{
		ClearTVEdgeSelection();
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		for (int j = 0; j < TVMaps.ePtrList[i]->faceList.Count(); j++)
		{
			int index = TVMaps.ePtrList[i]->faceList[j];
			BOOL bOutOfRange = FALSE;
			BOOL bResult = FALSE;
			if (bPreview)  // select preview
			{
				bResult = EdgeSelectionFromFaceSelection(index, i, mFSelPreview, mESelPreview, bOutOfRange);
			}
			else       // select
			{
				bResult = EdgeSelectionFromFaceSelection(index, i, mFSel, mESel, bOutOfRange);
			}
			if (bOutOfRange || bResult)
			{
				break;
			}
		}
	}
}

void MeshTopoData::ConvertFaceToEdgeSelPreview()
{
	ConvertFaceToEdgeSelection(TRUE);
}

void	MeshTopoData::ConvertFaceToEdgeSel()
{
	ConvertFaceToEdgeSelection(FALSE);
}


int MeshTopoDataContainer::Count()
{
	return mMeshTopoDataNodeInfo.Count();
}

void MeshTopoDataContainer::SetCount(int ct)
{
	mMeshTopoDataNodeInfo.SetCount(ct);
}

int MeshTopoDataContainer::Append(int ct, MeshTopoData* ld, INode *node, int extraCT)
{
	for (int k = 0; k < Count(); k++)
	{
		if (mMeshTopoDataNodeInfo[k].mMeshTopoData == ld)
			return 0;
	}

	MeshTopoDataNodeInfo t;
	t.mWeakRefToNode.SetRef(node);
	t.mMeshTopoData = ld;
	mMeshTopoDataNodeInfo.Append(1, &t, 10);
	return 1;
}

Point3 MeshTopoDataContainer::GetNodeColor(int index)
{
	if ((index < 0) || (index >= mMeshTopoDataNodeInfo.Count()) || nullptr == mMeshTopoDataNodeInfo[index].GetNode())
	{
		DbgAssert(0);
		return Point3(0.5f, 0.5f, 0.5f);
	}
	DWORD clr = mMeshTopoDataNodeInfo[index].GetNode()->GetWireColor();
	return Point3((float)GetRValue(clr) / 255.0f, (float)GetGValue(clr) / 255.0f, (float)GetBValue(clr) / 255.0f);

}

INode* MeshTopoDataContainer::GetNode(int index)
{
	if ((index < 0) || (index >= mMeshTopoDataNodeInfo.Count()))
	{
		DbgAssert(0);
		return NULL;
	}
	return mMeshTopoDataNodeInfo[index].GetNode();
}

void MeshTopoDataContainer::HoldPointsAndFaces()
{
	if (theHold.Holding())
	{
		for (int ldID = 0; ldID < this->Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoDataNodeInfo[ldID].mMeshTopoData;
			theHold.Put(new TVertAndTFaceRestore(ld));
		}
	}
}

void MeshTopoDataContainer::HoldPoints()
{
	if (theHold.Holding())
	{
		for (int ldID = 0; ldID < this->Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoDataNodeInfo[ldID].mMeshTopoData;
			theHold.Put(new TVertRestore(ld));
		}
	}
}

void MeshTopoDataContainer::HoldSelection()
{
	if (theHold.Holding())
	{
		for (int ldID = 0; ldID < this->Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoDataNodeInfo[ldID].mMeshTopoData;
			theHold.Put(new TSelRestore(ld));
		}
	}
}

void MeshTopoDataContainer::ClearTopoData(MeshTopoData* ld)
{
	for (int i = (Count() - 1); i >= 0; --i)
	{
		if (mMeshTopoDataNodeInfo[i].mMeshTopoData == ld)
		{
			mMeshTopoDataNodeInfo.Delete(i, 1);
		}
	}
}

Matrix3 MeshTopoDataContainer::GetNodeTM(TimeValue t, int index)
{
	Matrix3 tm(1);
	if ((index < 0) || (index >= mMeshTopoDataNodeInfo.Count()) || (mMeshTopoDataNodeInfo[index].GetNode() == NULL))
	{
		DbgAssert(0);
		return tm;
	}

	tm = mMeshTopoDataNodeInfo[index].GetNode()->GetObjectTM(t);
	return tm;

}

int MeshTopoData::GetTVVertFlag(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return 0;
	}
	return TVMaps.v[index].GetFlag();
}

void MeshTopoData::SetTVVertFlag(int index, int flag)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	return TVMaps.v[index].SetFlag(flag);
}


void MeshTopoData::SetTVVertDead(int index, BOOL dead)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else if (dead)
		TVMaps.v[index].SetDead();
	else
		TVMaps.v[index].SetAlive();

}
BOOL MeshTopoData::GetTVVertDead(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else
		return TVMaps.v[index].IsDead();
}

BOOL MeshTopoData::GetTVSystemLock(int index)
{
	if ((index < 0) || (index >= TVMaps.mSystemLockedFlag.GetSize()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else
		return TVMaps.mSystemLockedFlag[index];
}

BOOL MeshTopoData::GetTVVertWeightModified(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else
		return TVMaps.v[index].IsWeightModified();
}
void MeshTopoData::SetTVVertWeightModified(int index, BOOL modified)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	else if (modified)
		TVMaps.v[index].SetWeightModified();
	else TVMaps.v[index].SetWeightUnModified();

}


int MeshTopoData::AddTVVert(TimeValue t, Point3 p, Tab<int> *deadVertList)
{
	// TODO: add tvvert should also update filter list??
	//loop through vertex list looking for dead ones else attache to end
	int found = -1;
	if (deadVertList)
	{
		if (deadVertList->Count())
		{
			int index = (*deadVertList)[deadVertList->Count() - 1];

			BOOL isSystemLockVert = FALSE;
			if (index < TVMaps.mSystemLockedFlag.GetSize())
				isSystemLockVert = TVMaps.mSystemLockedFlag[index];
			if (!isSystemLockVert)
			{
				found = index;
			}
			deadVertList->Delete(deadVertList->Count() - 1, 1);
		}
	}
	else
	{
		for (int m = 0; m < GetNumberTVVerts(); m++)//TVMaps.v.Count();m++)
		{
			BOOL isSystemLockVert = FALSE;
			if (m < TVMaps.mSystemLockedFlag.GetSize())
				isSystemLockVert = TVMaps.mSystemLockedFlag[m];
			if (GetTVVertDead(m) && !isSystemLockVert)//TVMaps.v[m].flags & FLAG_DEAD)
			{
				found = m;
				m = TVMaps.v.Count();
			}
		}
	}
	//found dead spot add to it
	if (found != -1)
	{
		SetTVVertControlIndex(found, -1);
		SetTVVert(t, found, p);//TVMaps.v[found].p = p;
		SetTVVertInfluence(found, 0.0f);//TVMaps.v[found].influence = 0.0f;
		SetTVVertDead(found, FALSE);//TVMaps.v[found].flags -= FLAG_DEAD;
		if (IsTVVertexFilterValid())
		{
			mTVVertexFilter.Set(found);//enable the newly added one!
		}
		return found;
	}
	//create a new vert
	else
	{
		UVW_TVVertClass tv;
		tv.SetP(p);
		tv.SetFlag(0);
		tv.SetInfluence(0.0f);
		tv.SetControlID(-1);
		TVMaps.v.Append(1, &tv, 1000);
		if (IsTVVertexFilterValid())
		{
			mTVVertexFilter.SetSize(TVMaps.v.Count(), 1);
			mTVVertexFilter.Set((TVMaps.v.Count() - 1));//enable the newly added one!
		}
		ResizeTVVertSelection(TVMaps.v.Count(), 1);
		TVMaps.mSystemLockedFlag.SetSize(TVMaps.v.Count(), 1);
		return TVMaps.v.Count() - 1;
	}
}

int MeshTopoData::AddTVVert(TimeValue t, Point3 p, int faceIndex, int ithVert, BOOL sel, Tab<int> *deadVertList)
{
	if ((faceIndex < 0) || (faceIndex >= GetNumberFaces()))
	{
		DbgAssert(0);
		return -1;
	}

	int id = AddTVVert(t, p, deadVertList);
	if (sel)
		SetTVVertSelected(id, TRUE);
	TVMaps.f[faceIndex]->t[ithVert] = id;
	return id;

}


int MeshTopoData::AddTVHandle(TimeValue t, Point3 p, int faceIndex, int ithVert, BOOL sel, Tab<int> *deadVertList)
{
	if ((faceIndex < 0) || (faceIndex >= GetNumberFaces()))
	{
		DbgAssert(0);
		return -1;
	}
	if (TVMaps.f[faceIndex]->vecs == NULL)
	{
		DbgAssert(0);
		return -1;
	}
	if (ithVert < 0 || ithVert >= 8)
	{
		DbgAssert(0);
		return -1;
	}

	int id = AddTVVert(t, p,deadVertList);
	if (sel)
		SetTVVertSelected(id, TRUE);
	TVMaps.f[faceIndex]->vecs->handles[ithVert] = id;
	return id;
}


int MeshTopoData::AddTVInterior(TimeValue t, Point3 p, int faceIndex, int ithVert, BOOL sel, Tab<int> *deadVertList)
{
	if ((faceIndex < 0) || (faceIndex >= GetNumberFaces()))
	{
		DbgAssert(0);
		return -1;
	}
	if (TVMaps.f[faceIndex]->vecs == NULL)
	{
		DbgAssert(0);
		return -1;
	}

	int id = AddTVVert(t, p, deadVertList);
	if (sel)
		SetTVVertSelected(id, TRUE);
	TVMaps.f[faceIndex]->vecs->interiors[ithVert] = id;
	return id;
}

void MeshTopoData::DeleteTVVert(int index)
{
	if ((index < 0) || (index >= GetNumberTVVerts()))
	{
		DbgAssert(0);
		return;
	}
	TVMaps.v[index].SetDead();
	RaiseTVVertDeleted(index);
}

BOOL MeshTopoData::IsTVVertVisible(int index)
{
	if ((index < 0) || (index >= GetNumberTVVerts()))
	{
		DbgAssert(0);
		return FALSE;
	}


	if ((index < TVMaps.mSystemLockedFlag.GetSize()) && TVMaps.mSystemLockedFlag[index])
		return FALSE;

	if (TVMaps.v[index].IsDead())
		return FALSE;

	if (TVMaps.v[index].IsHidden())
		return FALSE;

	//also need to check matid
	if (mVertMatIDFilterList.GetSize() != GetNumberTVVerts())
	{
		mVertMatIDFilterList.SetSize(GetNumberTVVerts());
		mVertMatIDFilterList.SetAll();
	}
	BOOL iret = FALSE;
	if (mVertMatIDFilterList[index])
		iret = TRUE;

	//and then selected face
	if (!DoesTVVertexPassFilter(index))
		iret = FALSE;

	return iret;
}


BOOL MeshTopoData::IsFaceVisible(int index)
{
	if ((index < 0) || (index >= GetNumberFaces()))
	{
		DbgAssert(0);
		return FALSE;
	}

	if (TVMaps.f[index]->flags & FLAG_DEAD)
		return FALSE;

	if (!DoesFacePassFilter(index))
		return FALSE;

	return TRUE;
}

void MeshTopoData::BuildMatIDFilter(int matid)
{
	if (mVertMatIDFilterList.GetSize() != TVMaps.v.Count())
	{
		mVertMatIDFilterList.SetSize(TVMaps.v.Count());
		mVertMatIDFilterList.SetAll();
	}

	if (mMaterialIDFaceFilter.GetSize() != TVMaps.f.Count())
	{
		mMaterialIDFaceFilter.SetSize(TVMaps.f.Count());
		mMaterialIDFaceFilter.SetAll();
	}

	if (matid == -1)
	{
		mVertMatIDFilterList.SetAll();
		mMaterialIDFaceFilter.SetAll();
	}		
	else
	{
		mVertMatIDFilterList.ClearAll();
		mMaterialIDFaceFilter.ClearAll();

		for (int j = 0; j < TVMaps.f.Count(); j++)
		{
			int faceMatID = TVMaps.f[j]->GetMatID();

			if (faceMatID == matid)
			{
				mMaterialIDFaceFilter.Set(j,TRUE);

				int pcount = 3;
				pcount = TVMaps.f[j]->count;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[j]->t[k];
					//6-29-99 watje
					mVertMatIDFilterList.Set(index, TRUE);

					if ((TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[j]->vecs)
						)
					{
						index = TVMaps.f[j]->vecs->handles[k * 2];
						mVertMatIDFilterList.Set(index, TRUE);

						index = TVMaps.f[j]->vecs->handles[k * 2 + 1];
						mVertMatIDFilterList.Set(index, TRUE);

						index = TVMaps.f[j]->vecs->interiors[k];
						mVertMatIDFilterList.Set(index, TRUE);
					}
				}
			}
			else
			{
				if (GetFaceSelected(j))
				{
					SetFaceSelected(j, FALSE, FALSE);
				}
			}
		}
	}

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if (GetTVVertSelected(i))
		{
			if (IsTVVertVisible(i) == FALSE)
			{
				SetTVVertSelected(i, FALSE);
			}
		}
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		if (GetTVEdgeSelected(i))
		{
			int a = TVMaps.ePtrList[i]->a;
			int b = TVMaps.ePtrList[i]->b;

			if ((IsTVVertVisible(a) == FALSE) ||
				(IsTVVertVisible(b) == FALSE))
			{
				SetTVEdgeSelected(i, FALSE);
			}
		}
	}
}

int MeshTopoData::GetTVVertGeoIndex(int index)
{
	if (mVertexClusterList.Count() != TVMaps.v.Count())
		BuildVertexClusterList();
	if ((index < 0) || (index >= mVertexClusterList.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return mVertexClusterList[index];
}

void MeshTopoData::BuildVertexClusterList()
{
	int vCount = 0;

	int mapVertexCount = TVMaps.v.Count();
	if (mVertexClusterList.Count() != mapVertexCount)
		mVertexClusterList.SetCount(mapVertexCount);

	for (int i = 0; i < mapVertexCount; i++)
		mVertexClusterList[i] = -1;

	if (mapVertexCount == 0) return;

	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int ct = TVMaps.f[i]->count;
		for (int j = 0; j < ct; j++)
		{
			int tIndex = TVMaps.f[i]->t[j];
			if (tIndex >= mapVertexCount)
			{
				DbgAssert(!_T("You just re-produced CER bucket 23846621(MAXX-19314)!"));
				continue;
			}

			int vIndex = TVMaps.f[i]->v[j];
			mVertexClusterList[tIndex] = vIndex;
			if (vIndex > vCount) vCount = vIndex;
			//do patch handles also
			if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
			{
				int tvertIndex = TVMaps.f[i]->vecs->interiors[j];
				int vvertIndex = TVMaps.f[i]->vecs->vinteriors[j];
				if (tvertIndex >= 0)
				{
					mVertexClusterList[tvertIndex] = vvertIndex;
					if (vvertIndex > vCount) vCount = vvertIndex;

				}

				tvertIndex = TVMaps.f[i]->vecs->handles[j * 2];
				vvertIndex = TVMaps.f[i]->vecs->vhandles[j * 2];
				if (tvertIndex >= 0)
				{
					mVertexClusterList[tvertIndex] = vvertIndex;
					if (vvertIndex > vCount) vCount = vvertIndex;

				}

				tvertIndex = TVMaps.f[i]->vecs->handles[j * 2 + 1];
				vvertIndex = TVMaps.f[i]->vecs->vhandles[j * 2 + 1];

				if (tvertIndex >= 0)
				{
					mVertexClusterList[tvertIndex] = vvertIndex;
					if (vvertIndex > vCount) vCount = vvertIndex;
				}

			}
		}
	}
	vCount++;
	Tab<int> tVertexClusterListCounts;
	if (tVertexClusterListCounts.Count() != vCount)
		tVertexClusterListCounts.SetCount(vCount);
	for (int i = 0; i < vCount; i++)
		tVertexClusterListCounts[i] = 0;

	for (int i = 0; i < mVertexClusterList.Count(); i++)
	{
		int vIndex = mVertexClusterList[i];
		if ((vIndex < 0) || (vIndex >= tVertexClusterListCounts.Count()))
		{
		}
		else tVertexClusterListCounts[vIndex] += 1;
	}

	if (mVertexClusterListCounts.Count() != TVMaps.v.Count())
		mVertexClusterListCounts.SetCount(TVMaps.v.Count());

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		int vIndex = mVertexClusterList[i];
		if (vIndex != -1)
		{
			int ct = tVertexClusterListCounts[vIndex];
			mVertexClusterListCounts[i] = ct;
		}
	}
}

void MeshTopoData::BuildSnapBuffer(int width, int height)
{
	//clear out the buffers
	int s = width*height;
	if (mVertexSnapBuffer.Count() != s)
		mVertexSnapBuffer.SetCount(s);
	if (mEdgeSnapBuffer.Count() != s)
		mEdgeSnapBuffer.SetCount(s);

	for (int i = 0; i < s; i++)
	{
		mVertexSnapBuffer[i] = -1;
		mEdgeSnapBuffer[i] = -2;
	}

	mEdgesConnectedToSnapvert.SetSize(GetNumberTVEdges());//TVMaps.ePtrList.Count());
	mEdgesConnectedToSnapvert.ClearAll();
}

void MeshTopoData::FreeSnapBuffer()
{
	mVertexSnapBuffer.SetCount(0);
	mEdgeSnapBuffer.SetCount(0);
	mEdgesConnectedToSnapvert.SetSize(0);
}

void MeshTopoData::SetVertexSnapBuffer(int index, int value)
{
	if ((index < 0) || (index >= mVertexSnapBuffer.Count()))
	{
		DbgAssert(0);
		return;
	}
	mVertexSnapBuffer[index] = value;
}
int MeshTopoData::GetVertexSnapBuffer(int index)
{
	if ((index < 0) || (index >= mVertexSnapBuffer.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return mVertexSnapBuffer[index];
}

void MeshTopoData::SetEdgeSnapBuffer(int index, int value)
{
	if ((index < 0) || (index >= mEdgeSnapBuffer.Count()))
	{
		DbgAssert(0);
		return;
	}
	mEdgeSnapBuffer[index] = value;
}
int MeshTopoData::GetEdgeSnapBuffer(int index)
{
	if ((index < 0) || (index >= mEdgeSnapBuffer.Count()))
	{
		DbgAssert(0);
		return -1;
	}
	return mEdgeSnapBuffer[index];
}

Tab<int> &MeshTopoData::GetEdgeSnapBuffer()
{
	return mEdgeSnapBuffer;
}

void MeshTopoData::SetEdgesConnectedToSnapvert(int index, BOOL value)
{
	if ((index < 0) || (index >= mEdgesConnectedToSnapvert.GetSize()))
	{
		DbgAssert(0);
		return;
	}
	mEdgesConnectedToSnapvert.Set(index, value);
}
BOOL MeshTopoData::GetEdgesConnectedToSnapvert(int index)
{
	if ((index < 0) || (index >= mEdgesConnectedToSnapvert.GetSize()))
	{
		DbgAssert(0);
		return FALSE;
	}
	return mEdgesConnectedToSnapvert[index];
}


void MeshTopoData::SaveFaces(FILE *file)
{
	TVMaps.SaveFaces(file);
}

void MeshTopoData::LoadFaces(FILE *file, int ver)
{
	int fct;
	fread(&fct, sizeof(fct), 1, file);
	TVMaps.SetCountFaces(fct);
	if ((fct) && (ver < 4)) {
		//fix me old data
		Tab<UVW_TVFaceOldClass> oldData;
		oldData.SetCount(fct);
		fread(oldData.Addr(0), sizeof(UVW_TVFaceOldClass)*fct, 1, file);
		for (int i = 0; i < fct; i++)
		{
			TVMaps.f[i]->t[0] = oldData[i].t[0];
			TVMaps.f[i]->t[1] = oldData[i].t[1];
			TVMaps.f[i]->t[2] = oldData[i].t[2];
			TVMaps.f[i]->t[3] = oldData[i].t[3];
			TVMaps.f[i]->FaceIndex = oldData[i].FaceIndex;
			TVMaps.f[i]->SetMatID(oldData[i].MatID);
			TVMaps.f[i]->flags = oldData[i].flags;
			TVMaps.f[i]->vecs = NULL;
			if (TVMaps.f[i]->flags & 8)  // not this was FLAG_QUAD but this define got removed
				TVMaps.f[i]->count = 4;
			else TVMaps.f[i]->count = 3;

		}
		//now compute the geom points
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int pcount = 3;
			pcount = TVMaps.f[i]->count;

			for (int j = 0; j < pcount; j++)
			{
				BOOL found = FALSE;
				int index = 0;
				for (int k = 0; k < TVMaps.geomPoints.Count(); k++)
				{
					if (oldData[i].pt[j] == TVMaps.geomPoints[k])
					{
						found = TRUE;
						index = k;
						k = TVMaps.geomPoints.Count();
					}
				}
				if (found)
				{
					TVMaps.f[i]->v[j] = index;
				}
				else
				{
					TVMaps.f[i]->v[j] = TVMaps.geomPoints.Count();
					TVMaps.geomPoints.Append(1, &oldData[i].pt[j], 1);
				}
			}

		}

	}
	else
	{
		TVMaps.LoadFaces(file);
	}

	TVMaps.edgesValid = FALSE;

}

int MeshTopoData::LoadTVVerts(FILE *file)
{
	//check if oldversion
	int ver = 3;
	int vct = GetNumberTVVerts();//TVMaps.v.Count(); 

	fread(&ver, sizeof(ver), 1, file);
	if (ver == -1)
	{
		fread(&ver, sizeof(ver), 1, file);
		fread(&vct, sizeof(vct), 1, file);
	}
	else
	{
		vct = ver;
		ver = 3;
	}

	if (vct)
	{
		Tab<UVW_TVVertClass_Max9> oldData;
		oldData.SetCount(vct);
		fread(oldData.Addr(0), sizeof(UVW_TVVertClass_Max9)*vct, 1, file);

		TVMaps.v.SetCount(vct);
		for (int i = 0; i < vct; i++)
		{
			TVMaps.v[i].SetP(oldData[i].p);
			TVMaps.v[i].SetInfluence(0.0f);
			TVMaps.v[i].SetFlag(oldData[i].flags);
			TVMaps.v[i].SetControlID(-1);
		}

		ResizeTVVertSelection(vct);
        ValidateTVVertices();
	}

	return ver;
}
void MeshTopoData::SaveTVVerts(FILE *file)
{
	int vct = TVMaps.v.Count();
	Tab<UVW_TVVertClass_Max9> oldData;
	oldData.SetCount(vct);
	for (int i = 0; i < vct; i++)
	{
		oldData[i].p = GetTVVert(i);
		oldData[i].flags = GetTVVertFlag(i);
		oldData[i].influence = 0.0f;
	}

	fwrite(oldData.Addr(0), sizeof(UVW_TVVertClass_Max9)*vct, 1, file);//				fwrite(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*vct, 1,file);

}



UVW_TVFaceClass *MeshTopoData::CloneFace(int faceIndex)
{
	if ((faceIndex < 0) || (faceIndex >= TVMaps.f.Count()))
	{
		DbgAssert(0);
		return NULL;
	}

	return TVMaps.f[faceIndex]->Clone();
}


void MeshTopoData::CopyFaceData(Tab<UVW_TVFaceClass*> &f)
{
	TVMaps.CloneFaces(f);
}
//this paste the tv face data back from f
void MeshTopoData::PasteFaceData(Tab<UVW_TVFaceClass*> &f)
{
	TVMaps.AssignFaces(f);
	SetTVEdgeInvalid();

}

const static UINT ELAPSED_TIME_FOR_TIP = 5000; // 5000ms elapsed time for tip window
void MeshTopoData::BreakEdges(BitArray uvEdges)
{
	mbBreakEdgesSucceeded = false;
	std::thread tVerify(VerifyTopoValidForBreakEdges, this);
	tVerify.detach();  // don't affect the main thread's operation
	TVMaps.SplitUVEdges(uvEdges, std::ref(mbValidTopoForBreakEdges));
	if (!mbValidTopoForBreakEdges) // the 2D topo for break edges is invalid, a warning tip pops up
	{
		HWND hParent = GetViewPanelManager()->GetActiveViewPanel()->GetHWnd();
		MaxSDK::Util::GetTipSystem()->ShowTip(ELAPSED_TIME_FOR_TIP, GetString(IDS_INVALID_UV), 0.5f, 0.9f, hParent);
		mbValidTopoForBreakEdges = true;
	}
	mbBreakEdgesSucceeded = true;
}

void MeshTopoData::BreakEdges()
{
	TimeValue t = GetCOREInterface()->GetTime();
	BitArray weldEdgeList;
	weldEdgeList.SetSize(TVMaps.ePtrList.Count());
	weldEdgeList.ClearAll();


	BitArray vertsOnselectedEdge;
	vertsOnselectedEdge.SetSize(TVMaps.v.Count());
	vertsOnselectedEdge.ClearAll();

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		if (GetTVEdgeSelected(i))
		{
			int a = TVMaps.ePtrList[i]->a;
			vertsOnselectedEdge.Set(a, TRUE);
			a = TVMaps.ePtrList[i]->b;
			vertsOnselectedEdge.Set(a, TRUE);
		}
	}
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		if (!GetTVEdgeSelected(i))
		{
			int a = TVMaps.ePtrList[i]->a;
			int b = TVMaps.ePtrList[i]->b;
			if (vertsOnselectedEdge[a] || vertsOnselectedEdge[b])
				weldEdgeList.Set(i, TRUE);

		}
	}

	BitArray processedVerts;
	processedVerts.SetSize(TVMaps.v.Count());
	processedVerts.ClearAll();

	BitArray processedFace;
	processedFace.SetSize(TVMaps.f.Count());
	processedFace.ClearAll();


	Tab<int> groupsAtThisVert;
	groupsAtThisVert.SetCount(TVMaps.v.Count());

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		groupsAtThisVert[i] = 0;
	}

	Tab<AdjacentItem*> edgesAtVert;
	TVMaps.BuildAdjacentUVEdgesToVerts(edgesAtVert);

	Tab<AdjacentItem*> facesAtVert;
	TVMaps.BuildAdjacentUVFacesToVerts(facesAtVert);

	BitArray boundaryVert;
	boundaryVert.SetSize(TVMaps.v.Count());

	Tab<UVW_TVFaceClass*> tempF;
	tempF.SetCount(TVMaps.f.Count());
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		tempF[i] = TVMaps.f[i]->Clone();
	}

	Tab<UVW_TVVertClass> tempV;
	tempV = TVMaps.v;

	for (int i = 0; i < tempV.Count(); i++)
	{
		if (vertsOnselectedEdge[i])
		{
			//get a selected edge
			int numberOfEdges = edgesAtVert[i]->index.Count();
			boundaryVert.ClearAll();
			for (int j = 0; j < numberOfEdges; j++)
			{
				int edgeIndex = edgesAtVert[i]->index[j];
				if (GetTVEdgeSelected(edgeIndex) || TVMaps.ePtrList[edgeIndex]->faceList.Count() == 1)
				{

					int a = TVMaps.ePtrList[edgeIndex]->a;
					int b = TVMaps.ePtrList[edgeIndex]->b;
					if (a == i)
					{
						boundaryVert.Set(b, TRUE);
					}
					else
					{
						boundaryVert.Set(a, TRUE);
					}
				}
			}
			//get a seed face
			int currentGroup = 0;

			while (facesAtVert[i]->index.Count() > 0)
			{
				Tab<int> groupFaces;
				int faceIndex = facesAtVert[i]->index[0];
				groupFaces.Append(1, &faceIndex, 20);
				facesAtVert[i]->index.Delete(0, 1);
				int a = i;
				int nextVert = -1;
				int previousVert = -1;
				tempF[faceIndex]->GetConnectedUVVerts(a, nextVert, previousVert);

				BOOL done = FALSE;
				while (!done)
				{
					//add face

					done = TRUE;
					for (int k = 0; k < facesAtVert[i]->index.Count(); k++)
					{
						int testFaceIndex = facesAtVert[i]->index[k];
						int n, p;

						tempF[testFaceIndex]->GetConnectedUVVerts(a, n, p);

						BOOL degen = FALSE;
						if ((a == n) || (a == p))
							degen = TRUE;

						if ((n == previousVert) && ((!boundaryVert[previousVert]) || degen))
						{
							if (p != a)
								previousVert = p;
							groupFaces.Append(1, &testFaceIndex, 20);
							facesAtVert[i]->index.Delete(k, 1);
							k--;
							done = FALSE;
						}
						else if ((p == nextVert) && ((!boundaryVert[nextVert]) || degen))
						{
							if (n != a)
								nextVert = n;
							groupFaces.Append(1, &testFaceIndex, 20);
							facesAtVert[i]->index.Delete(k, 1);
							k--;
							done = FALSE;
						}
					}

					if (boundaryVert[nextVert] && boundaryVert[previousVert])
						done = TRUE;
				}

				if (currentGroup > 0)
				{
					int newIndex = -1;
					for (int k = 0; k < groupFaces.Count(); k++)
					{
						//find all the verts == i 
						int faceIndex = groupFaces[k];
						int deg = TVMaps.f[faceIndex]->count;
						//make a new vert

						for (int m = 0; m < deg; m++)
						{
							int tindex = TVMaps.f[faceIndex]->t[m];
							if (tindex == a)
							{
								if (newIndex == -1)
								{
									Point3 p = TVMaps.v[tindex].GetP();
									AddTVVert(t, p, faceIndex, m, FALSE);
									newIndex = TVMaps.f[faceIndex]->t[m];
								}
								else
								{
									TVMaps.f[faceIndex]->t[m] = newIndex;
								}
							}
						}

					}
				}

				currentGroup++;


			}


		}
	}

	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		if (tempF[i])
			delete tempF[i];
	}

	for (int i = 0; i < facesAtVert.Count(); i++)
	{
		if (facesAtVert[i])
			delete facesAtVert[i];
	}
	for (int i = 0; i < edgesAtVert.Count(); i++)
	{
		if (edgesAtVert[i])
			delete edgesAtVert[i];
	}
	//now split patch handles if any
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		if (GetTVEdgeSelected(i))
		{
			int veca = TVMaps.ePtrList[i]->avec;
			if (veca != -1)
			{
				int faceIndex = TVMaps.ePtrList[i]->faceList[0];
				int deg = TVMaps.f[faceIndex]->count;
				int j = faceIndex;
				for (int k = 0; k < deg * 2; k++)
				{

					if ((TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[j]->vecs) &&
						(TVMaps.f[j]->vecs->handles[k] == veca) &&
						(!(TVMaps.f[j]->flags & FLAG_DEAD))
						)
					{
						Point3 p = TVMaps.v[veca].GetP();
						AddTVHandle(t, p, j, k, TRUE);
					}

				}
			}
			veca = TVMaps.ePtrList[i]->bvec;
			if (veca != -1)
			{
				int faceIndex = TVMaps.ePtrList[i]->faceList[0];
				int deg = TVMaps.f[faceIndex]->count;
				int j = faceIndex;
				for (int k = 0; k < deg * 2; k++)
				{

					if ((TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[j]->vecs) &&
						(TVMaps.f[j]->vecs->handles[k] == veca) &&
						(!(TVMaps.f[j]->flags & FLAG_DEAD))
						)
					{
						Point3 p = TVMaps.v[veca].GetP();
						AddTVHandle(t, p, j, k, TRUE);
					}

				}
			}
		}
	}
	SetTVEdgeInvalid();
	TVMaps.BuildEdges();

}
void MeshTopoData::BreakVerts()
{
	TimeValue t = GetCOREInterface()->GetTime();

	BitArray weldEdgeList;
	weldEdgeList.SetSize(TVMaps.ePtrList.Count());
	weldEdgeList.ClearAll();

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if ( GetTVVertSelected(i) && !(TVMaps.v[i].IsDead()) && !TVMaps.mSystemLockedFlag[i])
		{
			//find all faces attached to this vertex
			Point3 p = TVMaps.v[i].GetP();
			BOOL first = TRUE;
			for (int j = 0; j < TVMaps.f.Count(); j++)
			{
				int pcount = 3;
				pcount = TVMaps.f[j]->count;
				for (int k = 0; k < pcount; k++)
				{
					if ((TVMaps.f[j]->t[k] == i) && (!(TVMaps.f[j]->flags & FLAG_DEAD)))
					{
						if (first)
						{
							first = FALSE;
						}
						else
						{
							AddTVVert(t, p, j, k,TRUE);
						}
					}
					if ((TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[j]->vecs) &&
						(TVMaps.f[j]->vecs->handles[k * 2] == i) &&
						(!(TVMaps.f[j]->flags & FLAG_DEAD))
						)
					{
						if (first)
						{
							first = FALSE;
						}
						else
						{
							AddTVHandle(t, p, j, k * 2, TRUE);
						}

					}
					if ((TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[j]->vecs) &&
						(TVMaps.f[j]->vecs->handles[k * 2 + 1] == i) &&
						(!(TVMaps.f[j]->flags & FLAG_DEAD))
						)
					{
						if (first)
						{
							first = FALSE;
						}
						else
						{
							AddTVHandle(t, p, j, k * 2 + 1, TRUE);
						}

					}
					if ((TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[j]->flags & FLAG_INTERIOR) &&
						(TVMaps.f[j]->vecs) &&
						(TVMaps.f[j]->vecs->interiors[k] == i) &&
						(!(TVMaps.f[j]->flags & FLAG_DEAD))
						)
					{
						if (first)
						{
							first = FALSE;
						}
						else
						{
							AddTVInterior(t, p, j, k, TRUE);
						}
					}
				}
			}

		}
	}

	SetTVEdgeInvalid();
	TVMaps.BuildEdges();
}

void MeshTopoData::SelectHandles(int dir)
{
	//if face is selected select me
	for (int j = 0; j < TVMaps.f.Count(); j++)
	{
		if ((!(TVMaps.f[j]->flags & FLAG_DEAD)) &&
			(TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
			(TVMaps.f[j]->vecs)
			)

		{
			int pcount = 3;
			pcount = TVMaps.f[j]->count;
			for (int k = 0; k < pcount; k++)
			{
				int id = TVMaps.f[j]->t[k];
				if (dir == 0)
				{
					if (GetTVVertSelected(id))
					{
						int vid1 = TVMaps.f[j]->vecs->handles[k * 2];
						int vid2 = 0;
						if (k == 0)
							vid2 = TVMaps.f[j]->vecs->handles[pcount * 2 - 1];
						else vid2 = TVMaps.f[j]->vecs->handles[k * 2 - 1];

						if ((IsTVVertVisible(vid1)) && (!(TVMaps.v[vid1].IsFrozen())))
							SetTVVertSelected(vid1, TRUE);
						if ((IsTVVertVisible(vid2)) && (!(TVMaps.v[vid2].IsFrozen())))
							SetTVVertSelected(vid2, TRUE);

						if (TVMaps.f[j]->flags & FLAG_INTERIOR)
						{
							int ivid1 = TVMaps.f[j]->vecs->interiors[k];
							if ((ivid1 >= 0) && (IsTVVertVisible(ivid1)) && (!(TVMaps.v[ivid1].IsFrozen())))
								SetTVVertSelected(ivid1, TRUE);
						}
					}
				}
				else
				{
					if (!GetTVVertSelected(id))
					{
						int vid1 = TVMaps.f[j]->vecs->handles[k * 2];
						int vid2 = 0;
						if (k == 0)
							vid2 = TVMaps.f[j]->vecs->handles[pcount * 2 - 1];
						else vid2 = TVMaps.f[j]->vecs->handles[k * 2 - 1];

						if ((IsTVVertVisible(vid1)) && (!(TVMaps.v[vid1].IsFrozen())))
							SetTVVertSelected(vid1, FALSE);
						if ((IsTVVertVisible(vid2)) && (!(TVMaps.v[vid2].IsFrozen())))
							SetTVVertSelected(vid1, FALSE);

						if (TVMaps.f[j]->flags & FLAG_INTERIOR)
						{
							int ivid1 = TVMaps.f[j]->vecs->interiors[k];
							if ((ivid1 >= 0) && (IsTVVertVisible(ivid1)) && (!(TVMaps.v[ivid1].IsFrozen())))
								SetTVVertSelected(ivid1, FALSE);
						}

					}

				}

			}
		}
	}

}

void MeshTopoData::BeginConstantTopoAcceleration(BOOL enableFilterFaceSelection)
{
	if (enableFilterFaceSelection && !mFacesAtVsFiltered)
		mFacesAtVsFiltered.reset(BuildNewFacesAtVs(TRUE));
	else if (!enableFilterFaceSelection && !mFacesAtVs)
		mFacesAtVs.reset(BuildNewFacesAtVs(FALSE));

	++mConstantTopoAccelerationNestingLevel;
}

void MeshTopoData::EndConstantTopoAcceleration()
{
	if (--mConstantTopoAccelerationNestingLevel == 0)
	{
		if (mFacesAtVs)
		{
			mFacesAtVs.reset();
		}

		if (mFacesAtVsFiltered)
		{
			mFacesAtVsFiltered.reset();
		}
	}
}

const FacesAtVerts* MeshTopoData::GetFacesAtVerts(BOOL enableFilterFaceSelection)
{
	return enableFilterFaceSelection == TRUE ? mFacesAtVsFiltered.get() : mFacesAtVs.get();
}

const FacesAtVerts* MeshTopoData::BuildNewFacesAtVs(BOOL enableFilterFaceSelection)
{
	const int tabExtraAlloc = 3;

	FacesAtVerts *facesAtVs = new FacesAtVerts;
	facesAtVs->mData.SetCount(GetNumberTVVerts());
	for (int vIndex = 0; vIndex < facesAtVs->mData.Count(); ++vIndex)
	{
		facesAtVs->mData[vIndex] = new FacesAtVert;
	}

	for (int fIndex = 0; fIndex < GetNumberFaces(); ++fIndex)
	{
		if (GetFaceDead(fIndex))
			continue;

		if (enableFilterFaceSelection == TRUE && !DoesFacePassFilter(fIndex))
			continue;

		for (int vIndexAtF = 0; vIndexAtF < GetFaceDegree(fIndex); ++vIndexAtF)
		{
			int vIndex = GetFaceTVVert(fIndex, vIndexAtF);
			facesAtVs->mData[vIndex]->mData.Append(1, &fIndex, tabExtraAlloc);
		}
	}

	return facesAtVs;
}

void MeshTopoData::SelectVertexElement(BOOL addSelection, BOOL enableFilterFaceSelection)
{
	const int tabExtraAlloc = 500;

	ConstantTopoAccelerator topoAccl(this, enableFilterFaceSelection);

	BitArray processedVerts(mVSel);

	Tab<int> currentVerts;
	for (int vIndex = 0; vIndex < mVSel.GetSize(); ++vIndex)
	{
		if (GetTVVertSelected(vIndex))
		{
			currentVerts.Append(1, &vIndex, processedVerts.NumberSet());
		}
	}

	if (addSelection)
	{
		//this does not work for removing selection since it grows the selection
		while (currentVerts.Count() > 0)
		{
			Tab<int> candidateVerts;
			//loop through all the selected verts
			for (int vIndexAtCur = 0; vIndexAtCur < currentVerts.Count(); ++vIndexAtCur)
			{
				int vIndex = currentVerts[vIndexAtCur];
				SetTVVertSelected(vIndex, addSelection ? TRUE : FALSE);

				//get the faces connected and grow out by one
				const FacesAtVert *facesAtV = topoAccl.GetFacesAtVerts()->mData[vIndex];
				for (int fIndexAtV = 0; fIndexAtV < facesAtV->mData.Count(); ++fIndexAtV)
				{
					int fIndex = facesAtV->mData[fIndexAtV];
					for (int vIndexAtF = 0; vIndexAtF < GetFaceDegree(fIndex); ++vIndexAtF)
					{
						int vertIndex = GetFaceTVVert(fIndex, vIndexAtF);
						//anything we have not marked add to our list
						if (vertIndex != vIndex && !processedVerts[vertIndex])
						{
							candidateVerts.Append(1, &vertIndex, tabExtraAlloc);
							processedVerts.Set(vertIndex);
						}
					}
				}
			}
			//add new verts to our list
			currentVerts = candidateVerts;
		}
	}
	else
	{
		bool done = false;
		processedVerts.ClearAll();
		while (!done)
		{
			//get the seed that is has not been processed
			int seed = -1;
			done = true;
			for (int i = 0; i < currentVerts.Count(); i++)
			{
				int vindex = currentVerts[i];
				if (!processedVerts[vindex])
				{
					seed = vindex;
					done = false;
					break;
				}
			}
			//if seed == -1 we are done
			if (seed != -1)
			{
				//get our element attached to this seed vertex
				processedVerts.Set(seed, TRUE);
				Tab<int> vertexStack;		//keep a stack of unselecetd
				Tab<int> elementVerts;		//and a list of of verts for this element
				vertexStack.Append(1, &seed, 5000);
				
				bool unselected = false;	// see if we have unselected vertices in the element
				while (vertexStack.Count())
				{
					int last = vertexStack.Count() - 1;   //pop the stack
					int child = vertexStack[last];
					vertexStack.Delete(last, 1);
					processedVerts.Set(child, TRUE);		//mark it as processed
					if (!mVSel[child])
						unselected = true;					//check if it is unselected

					elementVerts.Append(1, &child, 5000);   //add it to the element list

					//get faces attached to this vertex
					const FacesAtVert *facesAtV = topoAccl.GetFacesAtVerts()->mData[child];
					for (int fIndexAtV = 0; fIndexAtV < facesAtV->mData.Count(); ++fIndexAtV)
					{
						//loop through the vertices of this face
						int fIndex = facesAtV->mData[fIndexAtV];
						for (int vIndexAtF = 0; vIndexAtF < GetFaceDegree(fIndex); ++vIndexAtF)
						{
							int vertIndex = GetFaceTVVert(fIndex, vIndexAtF);  //get the vertex
							if (!processedVerts[vertIndex])      //if it is not processed
							{
								vertexStack.Append(1, &vertIndex, 5000);  //add it to the stack
								processedVerts.Set(vertIndex);			// and mark it so it does not process again
							}
						}
					}
				}
				if (unselected) //the element has unselected so unselect this element
				{
					for (int i = 0; i < elementVerts.Count(); i++)
					{
						int vIndex = elementVerts[i];
						SetTVVertSelected(vIndex, FALSE);
					}
				}
			}
		}
	}

	SelectHandles(0);
}

void MeshTopoData::SelectElement(int mode, BOOL addSelection)
{
	TransferSelectionStart(mode);

	if (mode == TVFACEMODE && !addSelection)
	{
		for (int i = 0; i < GetNumberFaces(); ++i)
		{
			if (!GetFaceDead(i) && !GetFaceSelected(i))
			{
				int pcount = GetFaceDegree(i);
				for (int k = 0; k < pcount; k++)
				{
					int index = GetFaceTVVert(i, k);
					SetTVVertSelected(index, FALSE);
				}
			}
		}
	}

	SelectVertexElement(addSelection, TRUE);

	TransferSelectionEnd(mode, FALSE, TRUE);
}

void  MeshTopoData::GetEdgeCluster(BitArray &cluster)
{
	BitArray tempArray;
	//next check to make sure we only have one cluster and if not element the smaller ones

	tempArray.SetSize(TVMaps.v.Count());
	tempArray.ClearAll();
	tempArray = mVSel;

	int seedVert = -1;
	int seedSize = -1;
	BitArray oppoProcessedElement;
	oppoProcessedElement.SetSize(TVMaps.v.Count());
	oppoProcessedElement.ClearAll();


	//5.1.04  remove any verts that share common geo verts
	Tab<int> geoIDs;
	geoIDs.SetCount(TVMaps.v.Count());
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		UVW_TVFaceClass *face = TVMaps.f[i];
		for (int j = 0; j < face->count; j++)
		{
			int gid = face->v[j];
			int vid = face->t[j];
			if ((gid >= 0) && (gid < geoIDs.Count()))
				geoIDs[vid] = gid;
		}
	}
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if (cluster[i])
		{
			//tag all shared geo verts
			int gid = geoIDs[i];
			BitArray sharedVerts;
			sharedVerts.SetSize(TVMaps.v.Count());
			sharedVerts.ClearAll();
			for (int j = 0; j < TVMaps.v.Count(); j++)
			{
				if ((geoIDs[j] == gid) && cluster[j])
				{
					if (IsTVVertVisible(j))
					{
						sharedVerts.Set(j);
					}
				}
			}
			//now look to see any of these
			if (sharedVerts.NumberSet() > 1)
			{
				int sharedEdge = -1;
				for (int j = 0; j < TVMaps.v.Count(); j++)
				{
					if (sharedVerts[j])
					{
						//loop through our edges and see if this one touches any of our vertices
						for (int k = 0; k < TVMaps.ePtrList.Count(); k++)
						{
							int a = TVMaps.ePtrList[k]->a;
							int b = TVMaps.ePtrList[k]->b;
							if (a == j)
							{
								if (cluster[b]) sharedEdge = j;
							}
							else if (b == j)
							{
								if (cluster[a]) sharedEdge = j;
							}

						}
					}
				}
				if (sharedEdge == -1)
				{
					BOOL first = TRUE;
					for (int j = 0; j < TVMaps.v.Count(); j++)
					{
						if (sharedVerts[j])
						{
							if (!first)
								cluster.Set(j, FALSE);
							first = FALSE;
						}
					}
				}
				else
				{
					for (int j = 0; j < TVMaps.v.Count(); j++)
					{
						if (sharedVerts[j])
						{
							cluster.Set(j, FALSE);
						}
					}
					cluster.Set(sharedEdge, TRUE);

				}
			}

		}
	}


	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if ((!oppoProcessedElement[i]) && (cluster[i]))
		{
			ClearTVVertSelection();
			SetTVVertSelected(i, TRUE);
			SelectVertexElement(TRUE);

			oppoProcessedElement |= mVSel;

			SetTVVertSel(mVSel & cluster);
			int ct = mVSel.NumberSet();
			if (ct > seedSize)
			{
				seedSize = ct;
				seedVert = i;
			}

		}
	}
	if (seedVert != -1)
	{
		ClearTVVertSelection();
		SetTVVertSelected(seedVert, TRUE);
		SelectVertexElement(TRUE);
		cluster = mVSel & cluster;
	}

	SetTVVertSel(tempArray);
}

void MeshTopoData::SewEdges(const GeoTVEdgesMap &gtvInfo)
{
	WeldSelectedSeam(gtvInfo);
	CleanUpDeadVertices();
	BuildTVEdges();
}

void MeshTopoData::BuildTVEdges()
{
	RaiseTopoInvalidated();
//	mLSCM->InvalidateTopo(this);
	TVMaps.BuildEdges();
}


void MeshTopoData::SelectByMatID(int matID)
{
	ClearFaceSelection();
	for (int i = 0; i < GetNumberFaces(); ++i)
	{
		if (GetFaceMatID(i) == matID)
		{
			SetFaceSelected(i, TRUE, FALSE);
		}
	}
}

void MeshTopoData::SetSelectionMatID(int matID)
{
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		if (GetFaceSelected(i))
		{
			SetFaceMatID(i, matID);
		}
	}
}

void  MeshTopoData::HoldFaceSel()
{
	holdFSel = GetFaceSel();
}

void  MeshTopoData::RestoreFaceSel()
{
	SetFaceSelectionByRef(holdFSel);
}

bool MeshTopoData::EdgeListFromPoints(Tab<int> &selEdges, int a, int b, const BitArray &candidateEdges)
{
	return TVMaps.EdgeListFromPoints(selEdges, a, b, candidateEdges);
}

void MeshTopoData::ClearSeamEdgesPreview()
{
	if (mSeamEdgesPreview.GetSize() != 0)
	{
		mSeamEdgesPreview.ClearAll();
	}
}

void MeshTopoData::ResetSeamEdgesPreview()
{
	if (mSeamEdgesPreview.GetSize() != GetNumberGeomEdges())
	{
		mSeamEdgesPreview.SetSize(GetNumberGeomEdges());
		mSeamEdgesPreview.ClearAll();
	}
}

void MeshTopoData::SetSeamEdgesPreview(int index, int val, BOOL bMirror)
{
	if (index >= 0 && index < mSeamEdgesPreview.GetSize())
	{
		mSeamEdgesPreview.Set(index, val);
		if (bMirror)
		{
			int edgeIndex = GetGeomEdgeMirrorIndex(index);
			SetSeamEdgesPreview(edgeIndex, val, FALSE);
		}
	}
}

int MeshTopoData::GetSeamEdgesPreviewSize()
{
	return mSeamEdgesPreview.GetSize();
}

int MeshTopoData::GetSeamEdgesPreview(int index)
{
	if (index >= 0 && index < mSeamEdgesPreview.GetSize())
	{
		return mSeamEdgesPreview[index];
	}
	return 0;
}

void MeshTopoData::ExpandSelectionToSeams()
{
	for (int i = 0; i < GetNumberTVEdges(); i++)//tvData->ePtrList.Count(); i++)
	{
		TVMaps.ePtrList[i]->lookupIndex = i;
	}
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		TVMaps.gePtrList[i]->lookupIndex = i;
	}

	BitArray newFSel;
	newFSel.SetSize(TVMaps.f.Count());
	newFSel.ClearAll();

	Tab<FConnnections*> face;
	face.SetCount(TVMaps.f.Count());
	for (int i = 0; i < TVMaps.f.Count(); i++)
		face[i] = new FConnnections();

	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int deg = TVMaps.f[i]->count;
		for (int j = 0; j < deg; j++)
		{
			int a = TVMaps.f[i]->v[j];
			int b = TVMaps.f[i]->v[(j + 1) % deg];
			int edge = TVMaps.FindGeoEdge(a, b);

			face[i]->connectedEdge.Append(1, &edge, 4);
		}
	}

	for (int i = 0; i < mFSel.GetSize(); i++)
	{
		if (GetFaceSelected(i) && !newFSel[i])
		{
			// start adding edges
			BOOL done = FALSE;
			int currentFace = i;

			BitArray selEdges;
			selEdges.SetSize(TVMaps.gePtrList.Count());
			selEdges.ClearAll();

			BitArray processedFaces;
			processedFaces.SetSize(TVMaps.f.Count());
			processedFaces.ClearAll();
			Tab<int> faceStack;

			if (mSeamEdges.GetSize() != TVMaps.gePtrList.Count())
			{
				mSeamEdges.SetSize(TVMaps.gePtrList.Count());
				mSeamEdges.ClearAll();
			}

			while (!done)
			{
				//loop through the current face edges
				int deg = TVMaps.f[currentFace]->count;
				//mark this face as processed
				processedFaces.Set(currentFace, TRUE);
				for (int j = 0; j < deg; j++)
				{
					int a = TVMaps.f[currentFace]->v[j];
					int b = TVMaps.f[currentFace]->v[(j + 1) % deg];
					int edge = TVMaps.FindGeoEdge(a, b);
					//add the edges if they are not a seam
					if (!mSeamEdges[edge])
					{
						selEdges.Set(edge, TRUE);

						//find the connected faces to this edge
						int numberConnectedFaces = TVMaps.gePtrList[edge]->faceList.Count();
						for (int k = 0; k < numberConnectedFaces; k++)
						{
							//add them to the stack if they have not been processed
							int potentialFaceIndex = TVMaps.gePtrList[edge]->faceList[k];

							if (!processedFaces[potentialFaceIndex])
								faceStack.Append(1, &potentialFaceIndex, 100);
						}

					}
				}

				//if stack == empty
				if (faceStack.Count() == 0)
					done = TRUE;
				else
				{
					//else current face = pop top of the stack		
					currentFace = faceStack[faceStack.Count() - 1];
					faceStack.Delete(faceStack.Count() - 1, 1);
				}

			}

			processedFaces.ClearAll();
			for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
			{
				if (selEdges[i])
				{
					int numberConnectedFaces = TVMaps.gePtrList[i]->faceList.Count();
					for (int k = 0; k < numberConnectedFaces; k++)
					{
						//add them to the stack if they have not been processed
						int potentialFaceIndex = TVMaps.gePtrList[i]->faceList[k];

						processedFaces.Set(potentialFaceIndex, TRUE);
					}
				}
			}

			// merge the result
			newFSel |= processedFaces;
		}
	}

	SetFaceSelectionByRef(newFSel);

	for (int i = 0; i < TVMaps.f.Count(); i++)
		delete face[i];
}


void MeshTopoData::CutSeams(BitArray seams)
{
	UVW_ChannelClass *tvData = &TVMaps;

	//loop through our seams and remove any that are open edges

	for (int i = 0; i < seams.GetSize(); i++)
	{
		if (seams[i])
		{
			if (tvData->gePtrList[i]->faceList.Count() == 1)
			{
				seams.Set(i, FALSE);
			}
		}
	}



	Tab<int> geomToUVVerts;
	geomToUVVerts.SetCount(tvData->geomPoints.Count());
	for (int i = 0; i < tvData->geomPoints.Count(); i++)
		geomToUVVerts[i] = -1;


	for (int i = 0; i < tvData->f.Count(); i++)
	{
		if (GetFaceSelected(i))
		{
			int deg = tvData->f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int tvA = tvData->f[i]->t[j];
				int geoA = tvData->f[i]->v[j];
				geomToUVVerts[geoA] = tvA;
			}
		}
	}
	for (int i = 0; i < tvData->ePtrList.Count(); i++)
	{
		tvData->ePtrList[i]->lookupIndex = i;
	}
	//convert our geom edges to uv edges
	BitArray newUVEdgeSel;
	newUVEdgeSel.SetSize(tvData->ePtrList.Count());
	newUVEdgeSel.ClearAll();

	for (int i = 0; i < seams.GetSize(); i++)
	{
		if (seams[i])
		{
			int ga = tvData->gePtrList[i]->a;
			int gb = tvData->gePtrList[i]->b;
			//convert to uv indices
			int uvA = geomToUVVerts[ga];
			int uvB = geomToUVVerts[gb];
			//find matching UV edge 
			int uvEdge = -1;
			if ((uvA == -1) || (uvB == -1))
				continue;

			int ct = tvData->e[uvA]->data.Count();
			for (int j = 0; j < ct; j++)
			{
				int matchA = tvData->e[uvA]->data[j]->a;
				int matchB = tvData->e[uvA]->data[j]->b;
				if (((matchA == uvA) && (matchB == uvB)) ||
					((matchA == uvB) && (matchB == uvA)))
				{
					uvEdge = tvData->e[uvA]->data[j]->lookupIndex;
				}
			}
			if (uvEdge == -1)
			{
				ct = tvData->e[uvB]->data.Count();
				for (int j = 0; j < ct; j++)
				{
					int matchA = tvData->e[uvB]->data[j]->a;
					int matchB = tvData->e[uvB]->data[j]->b;
					if (((matchA == uvA) && (matchB == uvB)) ||
						((matchA == uvB) && (matchB == uvA)))
					{
						uvEdge = tvData->e[uvB]->data[j]->lookupIndex;
					}
				}
			}
			if (uvEdge != -1)
			{
				newUVEdgeSel.Set(uvEdge, TRUE);
			}
			else
			{
			}
		}
	}

	//do a  break now
	TVMaps.SplitUVEdges(newUVEdgeSel, mbValidTopoForBreakEdges);


	mESel.SetSize(TVMaps.ePtrList.Count());
	ClearTVEdgeSelection();
}

void MeshTopoData::BuildSpringData(Tab<EdgeBondage> &springEdges, Tab<SpringClass> &verts, Point3 &rigCenter, Tab<RigPoint> &rigPoints, Tab<Point3> &initialPointData, float rigStrength)
{

	UVW_ChannelClass *tvData = &TVMaps;
	//get our face selection
	//for now we are building all of them
	//copy our geom data to the uvw data
	Tab<int> uvToGeom;
	uvToGeom.SetCount(tvData->v.Count());
	Box3 bounds;
	bounds.Init();
	springEdges.ZeroCount();


	verts.SetCount(tvData->v.Count());
	for (int i = 0; i < tvData->v.Count(); i++)
	{
		verts[i].pos = tvData->v[i].GetP();
		verts[i].vel = Point3(0.0f, 0.0f, 0.0f);
	}

	bounds.Init();
	for (int i = 0; i < tvData->f.Count(); i++)
	{
		if ((!(tvData->f[i]->flags & FLAG_DEAD)) && GetFaceSelected(i))//(tvData->f[i]->flags & FLAG_SELECTED))
		{
			int deg = tvData->f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int tvA = tvData->f[i]->t[j];
				bounds += tvData->v[tvA].GetP();
			}
		}
	}

	Point3 center = bounds.Center();
	//build our spring list
	for (int i = 0; i < tvData->ePtrList.Count(); i++)
	{
		//loop the through the edges
		int a = tvData->ePtrList[i]->a;
		int b = tvData->ePtrList[i]->b;
		int veca = tvData->ePtrList[i]->avec;
		int vecb = tvData->ePtrList[i]->bvec;
		BOOL isHidden = tvData->ePtrList[i]->flags & FLAG_HIDDENEDGEA;

		BOOL faceSelected = FALSE;
		for (int j = 0; j < tvData->ePtrList[i]->faceList.Count(); j++)
		{
			int faceIndex = tvData->ePtrList[i]->faceList[j];
			if ((!(tvData->f[faceIndex]->flags & FLAG_DEAD)) && GetFaceSelected(faceIndex))
				faceSelected = TRUE;
		}

		if (faceSelected)
		{
			EdgeBondage sp;
			sp.v1 = a;
			sp.v2 = b;
			sp.vec1 = veca;
			sp.vec2 = vecb;
			float dist = Length(tvData->v[a].GetP() - tvData->v[b].GetP());
			sp.dist = dist;
			sp.str = 1.0f;
			sp.distPer = 1.0f;
			sp.isEdge = FALSE;
			sp.edgeIndex = i;
			springEdges.Append(1, &sp, 5000);

			//add a spring for each edge
			//if edge is not visible find cross edge
			if ((isHidden) && (tvData->ePtrList[i]->faceList.Count() > 1))
			{
				//get face 1
				int a1, b1;
				a1 = -1;
				b1 = -1;

				int faceIndex = tvData->ePtrList[i]->faceList[0];

				if ((!(tvData->f[faceIndex]->flags & FLAG_DEAD)) && GetFaceSelected(faceIndex))
				{
					int deg = tvData->f[faceIndex]->count;
					for (int j = 0; j < deg; j++)
					{
						int tvA = tvData->f[faceIndex]->t[j];
						if ((tvA != a) && (tvA != b))
							a1 = tvA;

					}

					//get face 2
					faceIndex = tvData->ePtrList[i]->faceList[1];
					deg = tvData->f[faceIndex]->count;
					for (int j = 0; j < deg; j++)
					{
						int tvA = tvData->f[faceIndex]->t[j];
						if ((tvA != a) && (tvA != b))
							b1 = tvA;
					}

					if ((a1 != -1) && (b1 != -1))
					{
						EdgeBondage sp;
						sp.v1 = a1;
						sp.v2 = b1;
						sp.vec1 = -1;
						sp.vec2 = -1;
						float dist = Length(tvData->v[a1].GetP() - tvData->v[b1].GetP());
						sp.dist = dist;
						sp.str = 1.0f;
						sp.distPer = 1.0f;
						sp.isEdge = FALSE;
						sp.edgeIndex = -1;
						springEdges.Append(1, &sp, 5000);
					}
				}
			}
		}
	}

	//build our initial rig

	//find our edge verts
	//loop through our seams
	//build our outeredge list
	//check for multiple holes
	BitArray masterSeamList;
	masterSeamList.SetSize(tvData->ePtrList.Count());
	masterSeamList.ClearAll();



	Tab<int> tempBorderVerts;
	GetBorders(tempBorderVerts);
	Tab<int> borderVerts;
	for (int j = 0; j < tempBorderVerts.Count(); j++)
	{
		if (tempBorderVerts[j] == -1)
			j = tempBorderVerts.Count();
		else
		{
			borderVerts.Append(1, &tempBorderVerts[j], 1000);
			//			DebugPrint(_T("Border vert %d\n"),tempBorderVerts[j]);
		}
	}

	Point3 initialVec = Point3(0.0f, 0.0f, 0.0f);
	rigCenter = center;
	float d = 0.0f;
	float rigSize = 0.0f;
	for (int i = 0; i < borderVerts.Count(); i++)
	{

		int currentVert = borderVerts[i];
		int nextVert = borderVerts[(i + 1) % borderVerts.Count()];
		Point3 a = tvData->v[currentVert].GetP();
		Point3 b = tvData->v[nextVert].GetP();
		d += Length(a - b);
		if (Length(a - rigCenter) > rigSize)
			rigSize = Length(a - rigCenter);
		if (Length(b - rigCenter) > rigSize)
			rigSize = Length(b - rigCenter);
	}
	rigSize *= 2.0f;

	BOOL first = TRUE;
	float currentAngle = 0.0f;

	for (int i = 0; i < borderVerts.Count(); i++)
	{

		int currentVert = borderVerts[i];

		int nextVert = borderVerts[(i + 1) % borderVerts.Count()];
		int prevVert = 0;
		if (i == 0)
			prevVert = borderVerts[borderVerts.Count() - 1];
		else
			prevVert = borderVerts[i - 1];



		//need to make sure we are going the right direction

		Point3 a = tvData->v[currentVert].GetP();
		Point3 b = tvData->v[nextVert].GetP();

		float l = Length(a - b);
		float per = l / d;
		float angle = PI * 2.0f * per;

		//rotate our current
		Matrix3 rtm(1);
		rtm.RotateZ(currentAngle);
		if (first)
		{
			first = FALSE;
			Point3 ta = a;
			a.z = 0.0f;
			Point3 tb = center;
			b.z = 0.0f;

			initialVec = Normalize(ta - tb);
		}
		Point3 v = initialVec;
		v = v * rtm;

		RigPoint rp;

		rp.p = v * rigSize + center;//Point3(0.5f,0.5f,0.0f);
		rp.d = Length(rp.p - tvData->v[currentVert].GetP());
		rp.index = currentVert;
		rp.neighbor = nextVert;
		rp.elen = l;
		rp.springIndex = springEdges.Count();
		rp.angle = currentAngle;

		EdgeBondage sp;
		sp.v1 = currentVert;

		int found = -1;
		BOOL append = TRUE;
		for (int m = 0; m < tvData->v.Count(); m++)
		{
			if (tvData->v[m].IsDead() && !TVMaps.mSystemLockedFlag[m])//flags & FLAG_DEAD)
			{
				found = m;
				append = FALSE;
				m = tvData->v.Count();
			}
		}
		int newVertIndex = found;
		if (found == -1)
			newVertIndex = tvData->v.Count();


		UVW_TVVertClass newVert;
		newVert.SetP(rp.p);
		newVert.SetInfluence(0.0f);
		newVert.SetFlag(0);
		newVert.SetControlID(-1);
		rp.lookupIndex = newVertIndex;
		if (append)
		{
			tvData->v.Append(1, &newVert);
		}
		else
		{
			tvData->v[newVertIndex].SetP(newVert.GetP());
			tvData->v[newVertIndex].SetInfluence(newVert.GetInfluence());
			tvData->v[newVertIndex].SetFlag(newVert.GetFlag());
		}

		tvData->v[newVertIndex].SetRigPoint(TRUE);

		rigPoints.Append(1, &rp, 5000);

		SpringClass vd;
		vd.pos = tvData->v[newVertIndex].GetP();
		vd.vel = Point3(0.0f, 0.0f, 0.0f);
		vd.weight = 1.0f;

		if (append)
			verts.Append(1, &vd, 5000);
		else
		{
			verts[newVertIndex] = vd;
		}


		sp.v2 = newVertIndex;
		sp.vec1 = -1;
		sp.vec2 = -1;
		//build our rig springs			
		sp.dist = rp.d * 0.1f;
		//				sp.dist = rigSize*0.1f;
		sp.str = rigStrength;
		sp.edgePos = rp.p;
		sp.edgeIndex = -1;
		sp.originalVertPos = tvData->v[currentVert].GetP();

		sp.distPer = 1.0f;
		sp.isEdge = TRUE;
		springEdges.Append(1, &sp, 5000);
		currentAngle += angle;
	}


	initialPointData.SetCount(verts.Count());
	for (int i = 0; i < verts.Count(); i++)
	{
		verts[i].vel = Point3(0.0f, 0.0f, 0.0f);
		for (int j = 0; j < 6; j++)
		{
			verts[i].tempVel[j] = Point3(0.0f, 0.0f, 0.0f);
			verts[i].tempPos[j] = Point3(0.0f, 0.0f, 0.0f);
		}
		initialPointData[i] = tvData->v[i].GetP();
	}

	TVMaps.mSystemLockedFlag.SetSize(tvData->v.Count(), 1);
}



void MeshTopoData::InitReverseSoftData()
{


	BitArray originalVSel(mVSel);

	mSketchBelongsToList.SetCount(TVMaps.v.Count());
	mOriginalPos.SetCount(TVMaps.v.Count());

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		mSketchBelongsToList[i] = -1;
		mOriginalPos[i] = TVMaps.v[i].GetP();
	}

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if ((!originalVSel[i]) && (TVMaps.v[i].GetInfluence() > 0.0f))
		{
			int closest = -1;
			float closestDist = 0.0f;
			Point3 a = TVMaps.v[i].GetP();
			for (int j = 0; j < TVMaps.v.Count(); j++)
			{
				if (GetTVVertSelected(j))
				{
					Point3 b = TVMaps.v[j].GetP();
					float dist = Length(a - b);
					if ((dist < closestDist) || (closest == -1))
					{
						closest = j;
						closestDist = dist;
					}
				}
			}
			if (closest != -1)
			{
				mSketchBelongsToList[i] = closest;
			}
		}
	}

	SetTVVertSel(originalVSel);
}
void MeshTopoData::ApplyReverseSoftData()
{

	TimeValue t = GetCOREInterface()->GetTime();

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if (mSketchBelongsToList[i] >= 0)
		{
			Point3 accumVec(0.0f, 0.0f, 0.0f);
			int index = mSketchBelongsToList[i];
			Point3 vec = TVMaps.v[index].GetP() - mOriginalPos[index];
			accumVec += vec * TVMaps.v[i].GetInfluence();

			Point3 p = TVMaps.v[i].GetP() + accumVec;
			SetTVVert(t, i, p);
		}
	}

}

BOOL MeshTopoData::HasIncomingFaceSelection()
{
	if (mesh && (mesh->selLevel == MESH_FACE))
		return TRUE;
	else if (mnMesh && (mnMesh->selLevel == MNM_SL_FACE))
		return TRUE;
	else if (patch && (patch->selLevel == PATCH_PATCH))
		return TRUE;

	return FALSE;
}

void MeshTopoData::DisplayGeomEdge(GraphicsWindow *gw, int eindex, float size, BOOL thickOpenEdges, Color c)
{
	if (!DoesGeomEdgePassFilter(eindex)) return;

	Point3 plist[2];
	int ga = GetGeomEdgeVert(eindex, 0);
	int gb = GetGeomEdgeVert(eindex, 1);

	plist[0] = GetGeomVert(gb);
	plist[1] = GetGeomVert(ga);

	if (!patch)
	{
		if (thickOpenEdges)
			Draw3dEdge(gw, size, plist[0], plist[1], c);
		else gw->segment(plist, 1);
	}
	else
	{
		Point3 avec, bvec;

		int faceIndex = this->GetGeomEdgeConnectedFace(eindex, 0);
		int deg = GetFaceDegree(faceIndex);
		for (int i = 0; i < deg; i++)
		{
			int a = i;
			int b = (i + 1) % deg;
			int pa = patch->patches[faceIndex].v[a];
			int pb = patch->patches[faceIndex].v[b];
			if ((pa == ga) && (pb == gb))
			{
				plist[0] = patch->verts[pa].p;
				plist[1] = patch->verts[pb].p;
				avec = patch->vecs[patch->patches[faceIndex].vec[a * 2]].p;
				bvec = patch->vecs[patch->patches[faceIndex].vec[a * 2 + 1]].p;
			}
			else if ((pb == ga) && (pa == gb))
			{
				plist[0] = patch->verts[pa].p;
				plist[1] = patch->verts[pb].p;

				avec = patch->vecs[patch->patches[faceIndex].vec[a * 2]].p;
				bvec = patch->vecs[patch->patches[faceIndex].vec[a * 2 + 1]].p;
			}
		}

		Spline3D sp;
		SplineKnot ka(KTYPE_BEZIER_CORNER, LTYPE_CURVE, plist[0], avec, avec);
		SplineKnot kb(KTYPE_BEZIER_CORNER, LTYPE_CURVE, plist[1], bvec, bvec);
		sp.NewSpline();
		sp.AddKnot(ka);
		sp.AddKnot(kb);
		sp.SetClosed(0);
		sp.InvalidateGeomCache();
		Point3 ip1, ip2;

		//										Draw3dEdge(gw,size, plist[0], plist[1], c);
		for (int k = 0; k < 8; k++)
		{
			float per = k / 7.0f;
			ip1 = sp.InterpBezier3D(0, per);
			if (k > 0)
			{
				if (thickOpenEdges)
					Draw3dEdge(gw, size, ip1, ip2, c);
				else
				{
					plist[0] = ip1;
					plist[1] = ip2;
					gw->segment(plist, 1);
				}
			}
			ip2 = ip1;

		}
	}
}

void MeshTopoData::DisplayMirrorPlane(GraphicsWindow* gw, int mirrorAxis, Color c)
{
	Box3 bounds = mMirrorBoundingBox;
	// make the bounds larger than the local bounding box, so that the mirror plane won't be blocked by geom
	bounds.Scale(1.5f);

	gw->setColor(LINE_COLOR, c);

	bounds.pmin[mirrorAxis] = 0.0f;
	bounds.pmax[mirrorAxis] = 0.0f;

	Point3 plist[3];

	plist[0] = bounds[0];
	plist[1] = bounds[1];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[1];
	plist[1] = bounds[3];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[3];
	plist[1] = bounds[2];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[2];
	plist[1] = bounds[0];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[0 + 4];
	plist[1] = bounds[1 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[1 + 4];
	plist[1] = bounds[3 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[3 + 4];
	plist[1] = bounds[2 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[2 + 4];
	plist[1] = bounds[0 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[0];
	plist[1] = bounds[0 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[1];
	plist[1] = bounds[1 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[3];
	plist[1] = bounds[3 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);

	plist[0] = bounds[2];
	plist[1] = bounds[2 + 4];
	gw->polyline(2, plist, NULL, NULL, 1, NULL);
}


BitArray MeshTopoData::GetIncomingFaceSelection()
{
	if (mesh && (mesh->selLevel == MESH_FACE))
	{
		return mesh->faceSel;
	}
	else if (mnMesh && (mnMesh->selLevel == MNM_SL_FACE))
	{
		BitArray s;
		mnMesh->getFaceSel(s);
		return s;
	}
	else if (patch && (patch->selLevel == PATCH_PATCH))
	{
		return patch->patchSel;
	}

	BitArray error;
	return error;
}

BOOL MeshTopoData::OkToWeld(BOOL weldOnlyShared, int tvVert1, int tvVert2)
{
	if (weldOnlyShared == FALSE)
	{
		return TRUE;
	}
	else
	{
		if (GetTVVertGeoIndex(tvVert1) == GetTVVertGeoIndex(tvVert2))
			return TRUE;
	}
	return FALSE;

}
void MeshTopoData::WeldSelectedVerts(float sweldThreshold, BOOL weldOnlyShared)
{
	sweldThreshold = sweldThreshold * sweldThreshold;
	TimeValue t = GetCOREInterface()->GetTime();
	for (int m = 0; m < GetNumberTVVerts(); m++)
	{
		if (GetTVVertSelected(m) && (!GetTVVertDead(m)))
		{
			Point3 p(0.0f, 0.0f, 0.0f);
			Point3 op(0.0f, 0.0f, 0.0f);
			p = GetTVVert(m);
			op = p;
			int ct = 0;
			int index = -1;

			for (int i = m + 1; i < GetNumberTVVerts(); i++)
			{
				if (GetTVVertSelected(i) && (!GetTVVertDead(i)))
				{
					if (OkToWeld(weldOnlyShared, m, i))
					{
						Point3 np;
						np = GetTVVert(i);
						if (LengthSquared(np - op) < sweldThreshold)
						{
							p += np;
							ct++;
							if (index == -1)
								index = m;
							DeleteTVVert(i);
						}
					}
				}
			}

			if ((index == -1) || (ct == 0))
			{}
			else
			{
				ct++;
				p = p / (float)ct;

				SetTVVert(t, index, p);
				for (int i = 0; i < GetNumberFaces(); i++)
				{
					int pcount = 3;
					pcount = GetFaceDegree(i);
					for (int j = 0; j < pcount; j++)
					{
						int tvfIndex = GetFaceTVVert(i, j);
						Point3 np = GetTVVert(tvfIndex);
						if (OkToWeld(weldOnlyShared, m, tvfIndex))
						{
							if ((GetTVVertSelected(tvfIndex)) && (LengthSquared(np - op) < sweldThreshold))
							{
								if (tvfIndex != index)
								{
									SetFaceTVVert(i, j, index);
								}
							}
						}

						if ((GetFaceHasVectors(i)))
						{
							tvfIndex = GetFaceTVHandle(i, j * 2);
							Point3 np = GetTVVert(tvfIndex);
							if (OkToWeld(weldOnlyShared, m, tvfIndex))
							{
								if ((GetTVVertSelected(tvfIndex)) && (LengthSquared(np - op) < sweldThreshold))
								{
									if (tvfIndex != index)
									{
										SetFaceTVHandle(i, j * 2, index);
									}
								}
							}

							tvfIndex = GetFaceTVHandle(i, j * 2 + 1);
							np = GetTVVert(tvfIndex);
							if (OkToWeld(weldOnlyShared, m, tvfIndex))
							{
								if ((GetTVVertSelected(tvfIndex)) && (LengthSquared(np - op) < sweldThreshold))
								{
									if (tvfIndex != index)
									{
										SetFaceTVHandle(i, j * 2 + 1, index);
									}
								}
							}

							if (GetFaceHasInteriors(i))
							{
								tvfIndex = GetFaceTVInterior(i, j);
								np = GetTVVert(tvfIndex);
								if (OkToWeld(weldOnlyShared, m, tvfIndex))
								{
									if ((GetTVVertSelected(tvfIndex)) && (LengthSquared(np - op) < sweldThreshold))
									{
										if (tvfIndex != index)
										{
											SetFaceTVInterior(i, j, index);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	HandleTopoChanged();
	SetTVEdgeInvalid();
}

void MeshTopoData::WeldTVVerts(int tv1, int tv2, BitArray &replaceVerts, ReplaceVertMap &vertMap)
{
	// check state of tv1 & tv2
	if (GetTVVertDead(tv1))
	{
		tv1 = vertMap.findLiveVert(tv1);
		DbgAssert(!GetTVVertDead(tv1));
	}
	if (GetTVVertDead(tv2))
	{
		tv2 = vertMap.findLiveVert(tv2);
		DbgAssert(!GetTVVertDead(tv2));
	}
	if (tv1 == tv2) return; // no need to weld

	TimeValue t = GetCOREInterface()->GetTime();
	Point3 newPos = (GetTVVert(tv1) + GetTVVert(tv2)) * 0.5f;

	// keep tv1 and delete tv2
	SetTVVert(t, tv1, newPos);
	DeleteTVVert(tv2);

	replaceVerts.Set(tv2); // mark tv2 as replaced
	vertMap.addReplacePair(tv2, tv1); // save this replace pair tv2->tv1
}

void MeshTopoData::WeldSelectedSeam(const GeoTVEdgesMap &gtvInfo)
{
	BitArray replaceVerts;
	replaceVerts.SetSize(GetNumberTVVerts());
	replaceVerts.ClearAll();

	ReplaceVertMap vertMap;

	// loop all the selected geometric edges and weld corresponding tv edges
	for (auto mit = gtvInfo.gtvInfo.begin(); mit != gtvInfo.gtvInfo.end(); mit++)
	{
		//int geIndex = mit->first;	// geometric edge index
		int teIndex[2];
		teIndex[0] = mit->second.tvIndex1;	// first tv edge index
		teIndex[1] = mit->second.tvIndex2;	// second tv edge index

											// weld the two tv edges, a/b are two ends of the edge
		int ta[2], tb[2], ga[2], gb[2];
		for (int i = 0; i < 2; i++)
		{
			ta[i] = GetTVEdgeVert(teIndex[i], 0);
			tb[i] = GetTVEdgeVert(teIndex[i], 1);
			ga[i] = GetTVEdgeGeomVert(teIndex[i], 0);
			gb[i] = GetTVEdgeGeomVert(teIndex[i], 1);
		}

		if ((ga[0] == ga[1]) && (gb[0] == gb[1]))
		{
			// tow tv edges should be welded this way: a->a, b->b
			WeldTVVerts(ta[0], ta[1], replaceVerts, vertMap);
			WeldTVVerts(tb[0], tb[1], replaceVerts, vertMap);
		}
		else if ((ga[0] == gb[1]) && (gb[0] == ga[1]))
		{
			// tow tv edges should be welded this way: a->b, b->a
			WeldTVVerts(ta[0], tb[1], replaceVerts, vertMap);
			WeldTVVerts(tb[0], ta[1], replaceVerts, vertMap);
		}
		else
		{
			// should not happen...
			DbgAssert(FALSE);
			continue;
		}
	}

	// update face data to be consistent with new welded verts
	for (int fi = 0; fi < GetNumberFaces(); ++fi)
	{
		for (int viAtF = 0; viAtF < GetFaceDegree(fi); ++viAtF)
		{
			int vi;

			vi = GetFaceTVVert(fi, viAtF);
			if (replaceVerts[vi] && GetTVVertDead(vi))
			{
				int newVI = vertMap.findLiveVert(vi);
				SetFaceTVVert(fi, viAtF, newVI);
			}

			if (GetFaceHasVectors(fi))
			{
				vi = GetFaceTVHandle(fi, viAtF * 2);
				if (replaceVerts[vi] && GetTVVertDead(vi))
				{
					int newVI = vertMap.findLiveVert(vi);
					SetFaceTVHandle(fi, viAtF * 2, newVI);
				}

				vi = GetFaceTVHandle(fi, viAtF * 2 + 1);
				if (replaceVerts[vi] && GetTVVertDead(vi))
				{
					int newVI = vertMap.findLiveVert(vi);
					SetFaceTVHandle(fi, viAtF * 2 + 1, newVI);
				}

				if (GetFaceHasInteriors(fi))
				{
					vi = GetFaceTVInterior(fi, viAtF);
					if (replaceVerts[vi] && GetTVVertDead(vi))
					{
						int newVI = vertMap.findLiveVert(vi);
						SetFaceTVInterior(fi, viAtF, newVI);
					}
				}
			}
		}
	}

	HandleTopoChanged();
	SetTVEdgeInvalid();
}

void MeshTopoData::FixUpLockedFlags()
{
	TVMaps.mSystemLockedFlag.SetSize(GetNumberTVVerts());
	TVMaps.mSystemLockedFlag.SetAll();
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		if (!GetFaceDead(i))
		{
			int pcount = 3;
			pcount = GetFaceDegree(i);
			//if it is patch with curve mapping
			for (int j = 0; j < pcount; j++)
			{
				int index = TVMaps.f[i]->t[j];
				TVMaps.mSystemLockedFlag.Clear(index);
				if (GetFaceCurvedMaping(i) &&
					GetFaceHasVectors(i))
				{
					index = GetFaceTVHandle(i, j * 2);
					TVMaps.mSystemLockedFlag.Clear(index);
					index = GetFaceTVHandle(i, j * 2 + 1);
					TVMaps.mSystemLockedFlag.Clear(index);
					if (GetFaceHasInteriors(i))
					{
						index = GetFaceTVInterior(i, j);
						TVMaps.mSystemLockedFlag.Clear(index);
					}
				}
			}
		}
	}
}

void MeshTopoData::_GeomTVEdgeSelMutualConvertion(BitArray &geoSel, BitArray &uvwSel, bool isGeoToTV)
{
	Tab<GeomToTVEdges*> edgeInfo;
	edgeInfo.SetCount(GetNumberFaces());
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		edgeInfo[i] = new GeomToTVEdges();
		int deg = GetFaceDegree(i);
		edgeInfo[i]->edgeInfo.SetCount(deg);
		for (int j = 0; j < deg; j++)
		{
			edgeInfo[i]->edgeInfo[j].gIndex = -1;
			edgeInfo[i]->edgeInfo[j].tIndex = -1;
		}
	}

	//loop through the geo edges
	for (int i = 0; i < GetNumberGeomEdges(); i++)
	{
		int a = GetGeomEdgeVert(i, 0);
		int b = GetGeomEdgeVert(i, 1);
		int numberOfFaces = GetGeomEdgeNumberOfConnectedFaces(i);
		for (int j = 0; j < numberOfFaces; j++)
		{
			int faceIndex = GetGeomEdgeConnectedFace(i, j);
			int ithEdge = FindGeomEdge(faceIndex, a, b);
			if (ithEdge != -1)
			{
				edgeInfo[faceIndex]->edgeInfo[ithEdge].gIndex = i;
			}
		}
	}

	//loop through the uv edges
	for (int i = 0; i < GetNumberTVEdges(); i++)
	{
		int a = GetTVEdgeVert(i, 0);
		int b = GetTVEdgeVert(i, 1);
		int numberOfFaces = GetTVEdgeNumberTVFaces(i);
		for (int j = 0; j < numberOfFaces; j++)
		{
			int faceIndex = GetTVEdgeConnectedTVFace(i, j);
			int ithEdge = FindUVEdge(faceIndex, a, b);
			if (ithEdge != -1)
			{
				edgeInfo[faceIndex]->edgeInfo[ithEdge].tIndex = i;
			}
		}
	}

	if (isGeoToTV)
	{
		uvwSel.SetSize(GetNumberTVEdges());
		uvwSel.ClearAll();
	}
	else
	{
		geoSel.SetSize(GetNumberGeomEdges());
		geoSel.ClearAll();
	}

	for (int i = 0; i < GetNumberFaces(); i++)
	{
		int deg = GetFaceDegree(i);
		for (int j = 0; j < deg; j++)
		{
			int gIndex = edgeInfo[i]->edgeInfo[j].gIndex;
			int tIndex = edgeInfo[i]->edgeInfo[j].tIndex;
			if (gIndex != -1 && tIndex != -1)
			{
				if (gIndex >= geoSel.GetSize() || (tIndex >= uvwSel.GetSize()))
				{
					DbgAssert(0);
				}
				else
				{
					int tva, tvb;
					tva = GetTVEdgeVert(tIndex, 0);
					tvb = GetTVEdgeVert(tIndex, 1);

					if (isGeoToTV && geoSel[gIndex])
					{
						if (GetTVVertFrozen(tva) || GetTVVertFrozen(tvb))
						{
							uvwSel.Set(tIndex, FALSE);
							geoSel.Set(gIndex, FALSE);
						}
						else
						{
							uvwSel.Set(tIndex, TRUE);
						}
					}
					else if (!isGeoToTV && uvwSel[tIndex])
					{
						if (GetTVVertFrozen(tva) || GetTVVertFrozen(tvb))
						{
							uvwSel.Set(tIndex, FALSE);
							geoSel.Set(gIndex, FALSE);
						}
						else
						{
							geoSel.Set(gIndex, TRUE);
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < GetNumberFaces(); i++)
	{
		delete edgeInfo[i];
	}
}

void MeshTopoData::GetGeomEdgeRelatedTVEdges(BitArray geoSel, GeoTVEdgesMap &gtvInfo)
{
	Tab<GeomToTVEdges*> edgeInfo;
	edgeInfo.SetCount(GetNumberFaces());
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		edgeInfo[i] = new GeomToTVEdges();
		int deg = GetFaceDegree(i);
		edgeInfo[i]->edgeInfo.SetCount(deg);
		for (int j = 0; j < deg; j++)
		{
			edgeInfo[i]->edgeInfo[j].gIndex = -1;
			edgeInfo[i]->edgeInfo[j].tIndex = -1;
		}
	}

	//loop through the geo edges
	for (int i = 0; i < GetNumberGeomEdges(); i++)
	{
		int a = GetGeomEdgeVert(i, 0);
		int b = GetGeomEdgeVert(i, 1);
		int numberOfFaces = GetGeomEdgeNumberOfConnectedFaces(i);
		for (int j = 0; j < numberOfFaces; j++)
		{
			int faceIndex = GetGeomEdgeConnectedFace(i, j);
			int ithEdge = FindGeomEdge(faceIndex, a, b);
			if (ithEdge != -1)
			{
				edgeInfo[faceIndex]->edgeInfo[ithEdge].gIndex = i;
			}
		}
	}

	//loop through the uv edges
	for (int i = 0; i < GetNumberTVEdges(); i++)
	{
		int a = GetTVEdgeVert(i, 0);
		int b = GetTVEdgeVert(i, 1);
		int numberOfFaces = GetTVEdgeNumberTVFaces(i);
		for (int j = 0; j < numberOfFaces; j++)
		{
			int faceIndex = GetTVEdgeConnectedTVFace(i, j);
			int ithEdge = FindUVEdge(faceIndex, a, b);
			if (ithEdge != -1)
			{
				edgeInfo[faceIndex]->edgeInfo[ithEdge].tIndex = i;
			}
		}
	}

	for (int i = 0; i < GetNumberFaces(); i++)
	{
		int deg = GetFaceDegree(i);
		for (int j = 0; j < deg; j++)
		{
			int gIndex = edgeInfo[i]->edgeInfo[j].gIndex;
			int tIndex = edgeInfo[i]->edgeInfo[j].tIndex;
			if (gIndex != -1 && tIndex != -1)
			{
				if (gIndex >= geoSel.GetSize() || (tIndex >= GetNumberTVEdges()))
				{
					DbgAssert(0);
				}
				else
				{
					int tva, tvb;
					tva = GetTVEdgeVert(tIndex, 0);
					tvb = GetTVEdgeVert(tIndex, 1);

					if (geoSel[gIndex])
					{
						if (!GetTVVertFrozen(tva) && !GetTVVertFrozen(tvb))
						{
							gtvInfo.addGeomTVEdgePair(gIndex, tIndex);
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < GetNumberFaces(); i++)
	{
		delete edgeInfo[i];
	}
}

void MeshTopoData::ConvertGeomEdgeSelectionToTV(BitArray geoSel, BitArray &uvwSel)
{
	_GeomTVEdgeSelMutualConvertion(geoSel, uvwSel, true);
}

void MeshTopoData::ConvertTVEdgeSelectionToGeom(BitArray uvwSel, BitArray &geoSel)
{
	_GeomTVEdgeSelMutualConvertion(geoSel, uvwSel, false);
}

void MeshTopoData::GetFaceTVPoints(int faceIndex, Tab<int> &vIndices)
{
	vIndices.SetCount(0);
	if (GetFaceDead(faceIndex))
		return;

	int degree = GetFaceDegree(faceIndex);
	for (int j = 0; j < degree; j++)
	{
		int tvIndex = GetFaceTVVert(faceIndex, j);

		vIndices.Append(1, &tvIndex, 10000);
		if (GetFaceCurvedMaping(faceIndex))
		{
			if (GetFaceHasVectors(faceIndex))
			{

				int tvIndex = GetFaceTVHandle(faceIndex, j * 2);
				vIndices.Append(1, &tvIndex, 10000);
				tvIndex = GetFaceTVHandle(faceIndex, j * 2 + 1);
				vIndices.Append(1, &tvIndex, 10000);

				if (GetFaceHasInteriors(faceIndex))
				{
					int tvIndex = GetFaceTVInterior(faceIndex, j);
					vIndices.Append(1, &tvIndex, 10000);
				}
			}
		}
	}
}
void MeshTopoData::GetFaceGeomPoints(int faceIndex, Tab<int> &vIndices)
{
	vIndices.SetCount(0);
	if (GetFaceDead(faceIndex))
		return;

	int degree = GetFaceDegree(faceIndex);
	for (int j = 0; j < degree; j++)
	{
		int geomIndex = GetFaceGeomVert(faceIndex, j);

		vIndices.Append(1, &geomIndex, 10000);
		if (GetFaceCurvedMaping(faceIndex))
		{
			if (GetFaceHasVectors(faceIndex))
			{

				int geomIndex = GetFaceGeomHandle(faceIndex, j * 2);
				vIndices.Append(1, &geomIndex, 10000);
				geomIndex = GetFaceGeomHandle(faceIndex, j * 2 + 1);
				vIndices.Append(1, &geomIndex, 10000);

				if (GetFaceHasInteriors(faceIndex))
				{
					int geomIndex = GetFaceGeomInterior(faceIndex, j);
					vIndices.Append(1, &geomIndex, 10000);
				}
			}
		}
	}
}


void MeshTopoData::GetFaceTVPoints(int faceIndex, BitArray &vIndices)
{
	if (GetFaceDead(faceIndex))
		return;

	int degree = GetFaceDegree(faceIndex);
	for (int j = 0; j < degree; j++)
	{
		int tvIndex = GetFaceTVVert(faceIndex, j);

		vIndices.Set(tvIndex, TRUE);
		if (GetFaceCurvedMaping(faceIndex))
		{
			if (GetFaceHasVectors(faceIndex))
			{

				int tvIndex = GetFaceTVHandle(faceIndex, j * 2);
				vIndices.Set(tvIndex, TRUE);
				tvIndex = GetFaceTVHandle(faceIndex, j * 2 + 1);
				vIndices.Set(tvIndex, TRUE);

				if (GetFaceHasInteriors(faceIndex))
				{
					int tvIndex = GetFaceTVInterior(faceIndex, j);
					vIndices.Set(tvIndex, TRUE);
				}
			}
		}
	}
}
void MeshTopoData::GetFaceGeomPoints(int faceIndex, BitArray &vIndices)
{
	if (GetFaceDead(faceIndex))
		return;

	int degree = GetFaceDegree(faceIndex);
	for (int j = 0; j < degree; j++)
	{
		int geomIndex = GetFaceGeomVert(faceIndex, j);

		vIndices.Set(geomIndex, TRUE);
		if (GetFaceCurvedMaping(faceIndex))
		{
			if (GetFaceHasVectors(faceIndex))
			{

				int geomIndex = GetFaceGeomHandle(faceIndex, j * 2);
				vIndices.Set(geomIndex, TRUE);
				geomIndex = GetFaceGeomHandle(faceIndex, j * 2 + 1);
				vIndices.Set(geomIndex, TRUE);

				if (GetFaceHasInteriors(faceIndex))
				{
					int geomIndex = GetFaceGeomInterior(faceIndex, j);
					vIndices.Set(geomIndex, TRUE);
				}
			}
		}
	}
}


void MeshTopoData::PreIntersect()
{

	mIntersectNorms.SetCount(GetNumberFaces());
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		mIntersectNorms[i] = GetGeomFaceNormal(i);
	}

}

#define EPSILON   0.0001f

bool MeshTopoData::CheckFace(Ray ray, Point3 p1, Point3 p2, Point3 p3, Point3 n, float &at, Point3 &bary)
{
	///	Point3 v0, v2;
	Point3  p, bry;
	float d, rn, a;

	// See if the ray intersects the plane (backfaced)
	rn = DotProd(ray.dir, n);
	if (rn > -EPSILON) return false;

	// Use a point on the plane to find d
	Point3 v1 = p1;
	d = DotProd(v1, n);

	// Find the point on the ray that intersects the plane
	a = (d - DotProd(ray.p, n)) / rn;

	// Must be positive...
	if (a < 0.0f) return false;

	// Must be closer than the closest at so far
	if (at != -1.0f)
	{
		if (a > at) return false;
	}


	// The point on the ray and in the plane.
	p = ray.p + a*ray.dir;

	// Compute barycentric coords.
	bry = BaryCoords(p1, p2, p3, p);  // DbgAssert(bry.x + bry.y+ bry.z = 1.0) 

									  // barycentric coordinates must sum to 1 and each component must
									  // be in the range 0-1
	if (bry.x < 0.0f || bry.y < 0.0f || bry.z < 0.0f) return false;   // DS 3/8/97 this test is sufficient

	bary = bry;
	at = a;
	return true;
}
bool  MeshTopoData::Intersect(Ray ray, bool checkFrontFaces, bool checkBackFaces, float &at, Point3 &bary, int &hitFace)
{
	at = -1.0f;
	hitFace = -1;

	for (int i = 0; i < GetNumberFaces(); i++)
	{
		int deg = GetFaceDegree(i);

		for (int j = 0; j < deg - 2; j++)
		{

			Point3 p1, p2, p3;


			Point3 n = mIntersectNorms[i];

			p1 = GetGeomVert(GetFaceGeomVert(i, 0));
			p2 = GetGeomVert(GetFaceGeomVert(i, j + 1));
			p3 = GetGeomVert(GetFaceGeomVert(i, j + 2));

			if (checkFrontFaces && CheckFace(ray, p1, p2, p3, n, at, bary))
				hitFace = i;

			n = n*-1.0f;
			if (checkBackFaces && CheckFace(ray, p1, p2, p3, n, at, bary))
				hitFace = i;
		}
	}

	if (hitFace == -1)
		return false;
	else return true;
}
void MeshTopoData::PostIntersect()
{
	mIntersectNorms.SetCount(0);
}

int MeshTopoData::HitTestFace(GraphicsWindow *gw, INode *node, HitRegion hr)
{
	int iret = -1;
	DWORD dist = 0;



	TimeValue t = GetCOREInterface()->GetTime();



	DWORD limits = gw->getRndLimits();
	gw->setRndLimits((limits | GW_PICK) & ~GW_ILLUM | GW_BACKCULL);

	gw->clearHitCode();

	gw->setHitRegion(&hr);
	Matrix3 mat = node->GetObjectTM(t);
	gw->setTransform(mat);

	if (GetMesh())
	{



		SubObjHitList hitList;

		Mesh &mesh = *GetMesh();
		//				mesh.faceSel = ((MeshTopoData*)mc->localData)->faceSel;
		mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			SUBHIT_FACES | SUBHIT_SELSOLID, hitList);

		for (auto& rec : hitList) {
			int hitface = rec.index;
			if (iret == -1)
			{
				iret = hitface;
				dist = rec.dist;
			}
			else
			{
				if (rec.dist < dist)
				{
					iret = hitface;
					dist = rec.dist;
				}
			}
		}
	}
	else if (GetPatch())
	{
		SubPatchHitList hitList;

		PatchMesh &patch = *GetPatch();
		patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			SUBHIT_PATCH_PATCHES | SUBHIT_SELSOLID | SUBHIT_PATCH_IGNORE_BACKFACING, hitList);

		PatchSubHitRec *rec = hitList.First();
		while (rec) {
			int hitface = rec->index;
			if (iret == -1)
			{
				iret = hitface;
				dist = rec->dist;
			}
			else
			{
				if (rec->dist < dist)
				{
					iret = hitface;
					dist = rec->dist;
				}
			}
			rec = rec->Next();
		}

	}
	else if (GetMNMesh())
	{
		SubObjHitList hitList;


		MNMesh &mesh = *GetMNMesh();
		mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			SUBHIT_MNFACES | SUBHIT_SELSOLID, hitList);

		for (auto& rec : hitList) {
			int hitface = rec.index;
			if (iret == -1)
			{
				iret = hitface;
				dist = rec.dist;
			}
			else
			{
				if (rec.dist < dist)
				{
					iret = hitface;
					dist = rec.dist;
				}
			}
		}


	}

	gw->setRndLimits(limits);
	return iret;
}
void MeshTopoData::HitTestFaces(GraphicsWindow *gw, INode *node, HitRegion hr, Tab<int> &hitFaces)
{

	hitFaces.SetCount(0);


	TimeValue t = GetCOREInterface()->GetTime();



	DWORD limits = gw->getRndLimits();
	gw->setRndLimits((limits | GW_PICK) & ~GW_ILLUM | GW_BACKCULL);

	gw->clearHitCode();

	gw->setHitRegion(&hr);
	Matrix3 mat = node->GetObjectTM(t);
	gw->setTransform(mat);

	if (GetMesh())
	{
		SubObjHitList hitList;

		Mesh &mesh = *GetMesh();
		//				mesh.faceSel = ((MeshTopoData*)mc->localData)->faceSel;
		mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			SUBHIT_FACES | SUBHIT_SELSOLID, hitList);

		for (auto& rec : hitList) {
			int hitface = rec.index;
			hitFaces.Append(1, &hitface);
		}
	}
	else if (GetPatch())
	{
		SubPatchHitList hitList;

		PatchMesh &patch = *GetPatch();
		patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			SUBHIT_PATCH_PATCHES | SUBHIT_SELSOLID | SUBHIT_PATCH_IGNORE_BACKFACING, hitList);

		PatchSubHitRec *rec = hitList.First();
		while (rec) {
			int hitface = rec->index;
			hitFaces.Append(1, &hitface);
			rec = rec->Next();
		}

	}
	else if (GetMNMesh())
	{
		SubObjHitList hitList;

		MNMesh &mesh = *GetMNMesh();
		mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			SUBHIT_MNFACES | SUBHIT_SELSOLID, hitList);

		for (auto& rec : hitList) {
			int hitface = rec.index;
			hitFaces.Append(1, &hitface);
		}


	}
	gw->setRndLimits(limits);

}

Box3 MeshTopoData::GetSelFaceBoundingBox()
{
	Box3 bounds;
	bounds.Init();
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		if (GetFaceSelected(i))
		{
			int degree = GetFaceDegree(i);
			for (int j = 0; j < degree; j++)
			{
				int id = GetFaceGeomVert(i, j);//TVMaps.f[i]->v[j];
				bounds += GetGeomVert(id);
			}
		}
	}
	return bounds;
}

Box3 MeshTopoData::GetBoundingBox()
{
	Box3 bounds;
	bounds.Init();
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		int degree = GetFaceDegree(i);
		for (int j = 0; j < degree; j++)
		{
			int id = GetFaceGeomVert(i, j);//TVMaps.f[i]->v[j];
			bounds += GetGeomVert(id);
		}
	}
	return bounds;
}

BOOL MeshTopoData::GetUserCancel()
{
	//check to see if the user pressed esc or the system wants to end the thread
	SHORT iret = GetAsyncKeyState(VK_ESCAPE);

	if (iret == -32767)
		mUserCancel = TRUE;

	return mUserCancel;
}
void MeshTopoData::SetUserCancel(BOOL cancel)
{
	mUserCancel = cancel;
}


BOOL MeshTopoData::IsTVVertPinned(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return FALSE;
	}
	else
		return TVMaps.v[index].IsPinned();
}
void MeshTopoData::TVVertPin(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	RaisePinAddedOrDeleted(index);
	//mLSCM->InvalidatePinAddDelete(this, index);
	TVMaps.v[index].Pin();

}
void MeshTopoData::TVVertUnpin(int index)
{
	if ((index < 0) || (index >= TVMaps.v.Count()))
	{
		DbgAssert(0);
		return;
	}
	RaisePinAddedOrDeleted(index);
	//mLSCM->InvalidatePinAddDelete(this, index);
	TVMaps.v[index].Unpin();

}

class VertData
{
public:
	int mNextVertID;
	int mPrevVertID;
	BOOL mIsBorderEdge;
};

class CornerData
{
public:
	Tab<VertData> mData;
};


void MeshTopoData::GetBorders(Tab<int> &borderVerts)
{
	borderVerts.SetCount(0);
	Tab<CornerData*> corner;
	int numVerts = GetNumberTVVerts();
	corner.SetCount(numVerts);
	for (int i = 0; i < numVerts; i++)
	{
		corner[i] = new CornerData();
	}


	int numFaces = GetNumberFaces();
	BitArray degenerateVert;
	degenerateVert.SetSize(GetNumberTVVerts());
	//build a list of verts connected toverts in the face direction
	for (int i = 0; i < numFaces; i++)
	{
		int deg = GetFaceDegree(i);

		//make sure that the face is not degenerate
		BOOL degenerateFace = FALSE;
		degenerateVert.ClearAll();
		for (int j = 0; j < deg; j++)
		{
			int index = GetFaceTVVert(i, j);
			if (degenerateVert[index])
				degenerateFace = TRUE;
			degenerateVert.Set(index, TRUE);
		}

		if (!degenerateFace)
		{
			for (int j = 0; j < deg; j++)
			{
				//get all our openedges
				int prevID = (j - 1);
				if (prevID < 0) prevID = deg - 1;
				int nextID = (j + 1);
				if (nextID >= deg) nextID = 0;
				//get the order verts connecting to verts
				VertData d;
				int id = GetFaceTVVert(i, j);
				nextID = GetFaceTVVert(i, nextID);
				prevID = GetFaceTVVert(i, prevID);

				d.mNextVertID = nextID;
				d.mPrevVertID = prevID;
				d.mIsBorderEdge = TRUE;
				corner[id]->mData.Append(1, &d, 4);
			}
		}
	}

	//now compute our borders
	for (int i = 0; i < numVerts; i++)
	{
		int deg = corner[i]->mData.Count();
		for (int j = 0; j < deg; j++)
		{
			int nextVert = corner[i]->mData[j].mNextVertID;
			int nextVertDeg = corner[nextVert]->mData.Count();
			for (int k = 0; k < nextVertDeg; k++)
			{
				int checkVert = corner[nextVert]->mData[k].mNextVertID;
				//if we can reach back to the vert from another vert it is not
				//a border edge
				if (checkVert == i)
				{
					corner[nextVert]->mData[k].mIsBorderEdge = FALSE;
					corner[i]->mData[j].mIsBorderEdge = FALSE;
					k = nextVertDeg;
				}
			}
		}
	}

	BitArray processedVert;
	processedVert.SetSize(numVerts);
	processedVert.ClearAll();

	BitArray border;
	border.SetSize(numVerts);
	border.ClearAll();
	//now build our borders
	for (int i = 0; i < numVerts; i++)
	{
		if (!processedVert[i])
		{
			if (IsTVVertVisible(i))
			{
				int currentVert = i;

				BOOL done = FALSE;
				int lastVert = -1;
				border.ClearAll();
				int startVert = i;
				while (!done)
				{
					int deg = corner[currentVert]->mData.Count();
					BOOL found = FALSE;
					int numberOfConnectingEdges = 0;
					int connectingEdgeID = -1;
					for (int j = 0; j < deg; j++)
					{
						if (corner[currentVert]->mData[j].mIsBorderEdge)
						{

							if (!processedVert[currentVert])
							{
								numberOfConnectingEdges++;
								connectingEdgeID = j;
								found = TRUE;
							}
						}
					}

					if (found)
					{
						borderVerts.Append(1, &currentVert, 1000);
						if (numberOfConnectingEdges == 1)
							processedVert.Set(currentVert, TRUE);
						lastVert = currentVert;
						currentVert = corner[currentVert]->mData[connectingEdgeID].mNextVertID;
					}

					if (!found)
						done = TRUE;
					if (startVert == currentVert)
						done = TRUE;
				}
				int endID = -1;
				if (borderVerts.Count())
					borderVerts.Append(1, &endID, 1000);
			}
		}
	}

	for (int i = 0; i < numVerts; i++)
	{
		delete corner[i];
	}

}


class UVPair
{
public:
	int mFace;
	int mIthVert;
	int mUVWVert;

};

class UVPairList
{
public:
	Tab<UVPair> mList;
	Tab<int>	mWithInThreshold;
};


void MeshTopoData::WeldFaces(BitArray &selectedFaces, BOOL useEdgeLimit, float edgeLimit)
{
	TimeValue t = GetCOREInterface()->GetTime();
	Tab<UVPairList*> mConnectedVerts;
	mConnectedVerts.SetCount(GetNumberGeomVerts());
	for (int i = 0; i < GetNumberGeomVerts(); i++)
	{
		mConnectedVerts[i] = new UVPairList();
	}
	//loop through our selected tv faces and find all our geo verts that have multiple UVWs
	//	DebugPrint(_T("Faces \n"));
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		if (i < selectedFaces.GetSize() && selectedFaces[i])
		{
			//			DebugPrint(_T(" %d \n"),i);
			int degree = GetFaceDegree(i);
			for (int j = 0; j < degree; j++)
			{
				int geomVertIndex = GetFaceGeomVert(i, j);
				int tvVertIndex = GetFaceTVVert(i, j);
				int ithVert = j;
				int faceIndex = i;
				int ct = mConnectedVerts[geomVertIndex]->mList.Count();
				bool found = false;
				for (int k = 0; k < ct; k++)
				{
					if ((mConnectedVerts[geomVertIndex]->mList[k].mUVWVert == tvVertIndex) &&
						(mConnectedVerts[geomVertIndex]->mList[k].mFace == faceIndex))
					{
						found = true;
					}
				}
				if (!found)
				{
					UVPair pair;
					pair.mFace = faceIndex;
					pair.mIthVert = ithVert;
					pair.mUVWVert = tvVertIndex;
					mConnectedVerts[geomVertIndex]->mList.Append(1, &pair, 10);
				}
			}
		}
	}

	/*
	for (int i = 0; i < mConnectedVerts.Count(); i++)
	{
	if (mConnectedVerts[i]->mList.Count() > 1)
	{
	DebugPrint(_T("Geomvert %d\n"),i);
	for (int j = 0; j < mConnectedVerts[i]->mList.Count(); j++)
	{
	DebugPrint("	TVVert %d\n",mConnectedVerts[i]->mList[j].mUVWVert);
	}
	}
	}
	*/

	//now loop through the verts that have multiple UVWs


	BitArray commonVerts;
	commonVerts.SetSize(GetNumberTVVerts());
	for (int i = 0; i < mConnectedVerts.Count(); i++)
	{
		int connectedCount = mConnectedVerts[i]->mList.Count();
		if (connectedCount > 1)
		{
			//			Tab<int> withInThreshold;
			if (!useEdgeLimit)
			{
				//make sure the faces connected to the verts share a common tv vert

				//				DebugPrint(_T("Building geom vert data %d\n"),i);


				BitArray hitVerts;
				hitVerts.SetSize(connectedCount);


				for (int m = 0; m < connectedCount; m++)
				{

					mConnectedVerts[i]->mWithInThreshold.SetCount(0, FALSE);
					commonVerts.ClearAll();
					hitVerts.ClearAll();

					int faceIndex = mConnectedVerts[i]->mList[m].mFace;
					int degree = GetFaceDegree(faceIndex);

					//					DebugPrint("	base mList %d\n",m);
					for (int k = 0; k < degree; k++)
					{
						int tvVertIndex = GetFaceTVVert(faceIndex, k);
						commonVerts.Set(tvVertIndex, TRUE);
						//						DebugPrint("	tv vert %d\n",tvVertIndex);
					}

					mConnectedVerts[i]->mWithInThreshold.Append(1, &m, connectedCount);
					hitVerts.Set(m, TRUE);


					bool done = false;
					while (!done)
					{
						done = true;
						for (int j = 0; j < connectedCount; j++)
						{
							if (!hitVerts[j])
							{

								int faceIndex = mConnectedVerts[i]->mList[j].mFace;
								//								DebugPrint("		check mList %d face %d \n",j,faceIndex);
								int degree = GetFaceDegree(faceIndex);
								bool found = false;

								for (int k = 0; k < degree; k++)
								{
									int tvVertIndex = GetFaceTVVert(faceIndex, k);
									//									DebugPrint("		tv vert %d\n",tvVertIndex);
									if (commonVerts[tvVertIndex])
									{
										//										DebugPrint("		 common geom vert %d\n",j);
										found = true;
										k = degree;
									}
								}
								if (found)
								{
									mConnectedVerts[i]->mWithInThreshold.Append(1, &j, connectedCount);
									for (int k = 0; k < degree; k++)
									{
										int tvVertIndex = GetFaceTVVert(faceIndex, k);
										commonVerts.Set(tvVertIndex, TRUE);
									}
									hitVerts.Set(j, TRUE);
									done = false;
									m = connectedCount;
								}
							}
						}
					}
				}
			}
			else
			{
				//check to see if we have 2 or more within the  threshold

				for (int j = 0; j < connectedCount; j++)
				{

					Point3 faceCenter(0, 0, 0);
					int faceIndex = mConnectedVerts[i]->mList[j].mFace;
					int degree = GetFaceDegree(faceIndex);
					int anchorTVIndex = mConnectedVerts[i]->mList[j].mUVWVert;
					Point3 p = GetTVVert(anchorTVIndex);
					for (int k = 0; k < degree; k++)
					{
						int tvVertIndex = GetFaceTVVert(faceIndex, k);
						faceCenter += GetTVVert(tvVertIndex);
					}
					faceCenter = faceCenter / (float)degree;

					for (int k = j + 1; k < connectedCount; k++)
					{
						int tvIndex = mConnectedVerts[i]->mList[k].mUVWVert;
						if (anchorTVIndex != tvIndex)
						{
							Point3 testPoint = GetTVVert(tvIndex);

							float dist = Length(p - faceCenter);
							float travel = Length(p - testPoint);

							float l = travel / dist;
							if (l < edgeLimit)
							{
								if (mConnectedVerts[i]->mWithInThreshold.Count() == 0)
									mConnectedVerts[i]->mWithInThreshold.Append(1, &j, connectedCount);

								mConnectedVerts[i]->mWithInThreshold.Append(1, &k, connectedCount);
								k = connectedCount;
							}

						}
					}
				}
			}
			/*
			if (withInThreshold.Count() > 1)
			{

			Point3 center(0,0,0);
			for (int j = 0; j < withInThreshold.Count(); j++)
			{
			int index = withInThreshold[j];
			int tvIndex = mConnectedVerts[i]->mList[index].mUVWVert;
			center += GetTVVert(tvIndex);
			}
			center = center/(float) withInThreshold.Count();

			int mListIndex = withInThreshold[0];
			int index = mConnectedVerts[i]->mList[mListIndex].mUVWVert;

			SetTVVert(t, index, center);
			for (int j = 1; j < withInThreshold.Count(); j++)
			{
			mListIndex = withInThreshold[j];
			int faceIndex = mConnectedVerts[i]->mList[mListIndex].mFace;
			int ithVert = mConnectedVerts[i]->mList[mListIndex].mIthVert;
			SetFaceTVVert(faceIndex, ithVert, index);
			}
			}
			*/
		}
	}

	for (int i = 0; i < mConnectedVerts.Count(); i++)
	{

		if (mConnectedVerts[i]->mWithInThreshold.Count() > 1)
		{

			Point3 center(0, 0, 0);
			for (int j = 0; j < mConnectedVerts[i]->mWithInThreshold.Count(); j++)
			{
				int index = mConnectedVerts[i]->mWithInThreshold[j];
				int tvIndex = mConnectedVerts[i]->mList[index].mUVWVert;
				center += GetTVVert(tvIndex);
			}
			center = center / (float)mConnectedVerts[i]->mWithInThreshold.Count();

			int mListIndex = mConnectedVerts[i]->mWithInThreshold[0];
			int index = mConnectedVerts[i]->mList[mListIndex].mUVWVert;

			SetTVVert(t, index, center);
			for (int j = 1; j < mConnectedVerts[i]->mWithInThreshold.Count(); j++)
			{
				mListIndex = mConnectedVerts[i]->mWithInThreshold[j];
				int faceIndex = mConnectedVerts[i]->mList[mListIndex].mFace;
				int ithVert = mConnectedVerts[i]->mList[mListIndex].mIthVert;
				SetFaceTVVert(faceIndex, ithVert, index);
			}
		}

	}

	//tbd CLEAN UP curve handles



	//clean out unused verts
	BitArray usedVerts;
	usedVerts.SetSize(GetNumberTVVerts());
	usedVerts.ClearAll();
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		int degree = GetFaceDegree(i);
		for (int j = 0; j < degree; j++)
		{
			int index = TVMaps.f[i]->t[j];
			usedVerts.Set(index, TRUE);
			if (GetFaceCurvedMaping(i) &&
				GetFaceHasVectors(i))
			{
				index = GetFaceTVHandle(i, j * 2);
				usedVerts.Set(index, TRUE);

				index = GetFaceTVHandle(i, j * 2 + 1);
				usedVerts.Set(index, TRUE);

				if (GetFaceHasInteriors(i))
				{
					index = GetFaceTVInterior(i, j);
					usedVerts.Set(index, TRUE);
				}
			}
		}
	}
	for (int i = 0; i < GetNumberTVVerts(); i++)
	{
		if (usedVerts[i] == FALSE)
		{
			SetTVVertDead(i, TRUE);
		}
	}


	for (int i = 0; i < mConnectedVerts.Count(); i++)
	{
		if (mConnectedVerts[i])
			delete mConnectedVerts[i];
	}

}

class AreaCluster
{
public:
	float mTVArea;
	float mGeomArea;

	float mTVPerimeter;
	float mGeomPerimeter;

	float mRatio;
	Box3 mTVBounds;
	float mScaleAmount;
};

void MeshTopoData::RescaleClusters(const Tab<int> &clusters, const Tab<float> &clustersRescales)
{
	int largestID = -1;
	Tab<AreaCluster> clusterInfo;

	bool hasCluster = false;
	for (int i = 0; i < clusters.Count(); i++)
	{

		if ((clusters[i] > largestID) && (clusters[i] != -1))
		{
			largestID = clusters[i];
			hasCluster = true;
		}
	}

	if (hasCluster == false)
		return;

	clusterInfo.SetCount(largestID + 1);
	for (int i = 0; i < clusterInfo.Count(); i++)
	{
		clusterInfo[i].mTVArea = 0.0f;
		clusterInfo[i].mGeomArea = 0.0f;
		clusterInfo[i].mTVPerimeter = 0.0f;
		clusterInfo[i].mGeomPerimeter = 0.0f;
		clusterInfo[i].mRatio = 0.0f;
		clusterInfo[i].mTVBounds.Init();
	}
	Tab<Point3> tvPoints;
	Tab<Point3> geomPoints;

	//find the area of th clust in geom and tv space
	for (int i = 0; i < clusters.Count(); i++)
	{
		int clusterIndex = clusters[i];
		if (clusterIndex != -1)
		{
			int degree = GetFaceDegree(i);
			tvPoints.SetCount(degree, FALSE);
			geomPoints.SetCount(degree, FALSE);

			for (int j = 0; j < degree; j++)
			{
				tvPoints[j] = GetTVVert(GetFaceTVVert(i, j));
				tvPoints[j].z = 0.0f;  //we set z to zero since some mapping puts in strange values here
				clusterInfo[clusterIndex].mTVBounds += tvPoints[j];
				geomPoints[j] = GetGeomVert(GetFaceGeomVert(i, j));
			}

			if (degree == 3)
			{
				clusterInfo[clusterIndex].mTVArea += AreaOfTriangle(tvPoints[0], tvPoints[1], tvPoints[2]);
				clusterInfo[clusterIndex].mGeomArea += AreaOfTriangle(geomPoints[0], geomPoints[1], geomPoints[2]);
			}
			else if (degree > 0)
			{
				clusterInfo[clusterIndex].mTVArea += AreaOfPolygon(tvPoints);
				clusterInfo[clusterIndex].mGeomArea += AreaOfPolygon(geomPoints);
			}
			for (int i = 0; i < tvPoints.Count(); i++)
			{
				int nextI = i + 1;
				if (nextI >= tvPoints.Count())
					nextI = 0;
				clusterInfo[clusterIndex].mTVPerimeter += Length(tvPoints[nextI] - tvPoints[i]);
				clusterInfo[clusterIndex].mGeomPerimeter += Length(geomPoints[nextI] - geomPoints[i]);

			}
		}
	}


	float avgRatio = 0.0f;
	int avgRatioCt = 0;
	for (int i = 0; i < clusterInfo.Count(); i++)
	{
		if (clusterInfo[i].mTVArea > 0.0f &&  clusterInfo[i].mGeomArea > 0.0f)
		{
			float geoWidth = clusterInfo[i].mGeomPerimeter;// sqrt(clusterInfo[i].mGeomArea);
			float tvWidth = clusterInfo[i].mTVPerimeter;// sqrt(clusterInfo[i].mTVArea);
			float scale = geoWidth / tvWidth;
			clusterInfo[i].mScaleAmount = scale;
			clusterInfo[i].mRatio = 1.0 / scale;

			avgRatio += clusterInfo[i].mRatio;
			avgRatioCt++;
		}
	}

	if (avgRatioCt > 0)
		avgRatio = avgRatio / (float)avgRatioCt;
	else
		avgRatio = 1.0f;

	//now scale everything
	BitArray usedVerts;
	usedVerts.SetSize(GetNumberTVVerts());
	usedVerts.ClearAll();
	TimeValue t = GetCOREInterface()->GetTime();
	for (int i = 0; i < clusters.Count(); i++)
	{
		int clusterIndex = clusters[i];
		if ((clusterIndex != -1) && (clusterInfo[clusterIndex].mTVArea > 0.0f)
			&& (clusterInfo[clusterIndex].mGeomArea > 0.0f)
			)
		{
			int degree = GetFaceDegree(i);
			tvPoints.SetCount(degree, FALSE);
			geomPoints.SetCount(degree, FALSE);

			for (int j = 0; j < degree; j++)
			{
				int tvIndex = GetFaceTVVert(i, j);
				if (usedVerts[tvIndex] == false)
				{
					usedVerts.Set(tvIndex);
					Point3 center = clusterInfo[clusterIndex].mTVBounds.Center();
					center.z = 0.0f;
					Point3 p = GetTVVert(tvIndex);
					Point3 vec = p - center;
					float scaleAmount = clusterInfo[clusterIndex].mScaleAmount;
					vec = vec *  scaleAmount;
					vec = vec * avgRatio;
					float sc = clustersRescales[clusterIndex];
					if (sc <= 0.0f)
						sc = 0.0001f;
					if (sc > 1.0f)
						sc = 1.0f;
					vec = vec * sc;
					p = center + vec;
					SetTVVert(t, tvIndex, p);
				}
			}
		}
	}

}


void MeshTopoData::HoldSelection()
{
	mHoldGeomSel.mFace = GetFaceSel();
	mHoldGeomSel.mEdge = GetGeomEdgeSel();

	int total = 0;
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		total += GetFaceDegree(i);
	}
	mHoldGeomSel.mVert.SetSize(total);
	mHoldGeomSel.mVert.ClearAll();

	int ct = 0;

	for (int i = 0; i < GetNumberFaces(); i++)
	{
		if (GetFaceDead(i) == FALSE)
		{
			int deg = GetFaceDegree(i);
			for (int j = 0; j < deg; j++)
			{
				int tvIndex = GetFaceTVVert(i, j);
				if (tvIndex < mVSel.GetSize() &&
					GetTVVertSelected(tvIndex))
				{
					mHoldGeomSel.mVert.Set(ct, TRUE);
				}
				ct++;
			}
		}
	}

}
void MeshTopoData::RestoreSelection()
{
	if (mHoldGeomSel.mFace.GetSize() == 0)
		return;
	if (mHoldGeomSel.mEdge.GetSize() == 0)
		return;
	if (mHoldGeomSel.mVert.GetSize() == 0)
		return;
	SetFaceSelectionByRef(mHoldGeomSel.mFace);
	SetGeomEdgeSel(mHoldGeomSel.mEdge);

	//sync the edges
	BitArray uvEdgeSel = GetTVEdgeSel();
	ConvertGeomEdgeSelectionToTV(GetGeomEdgeSel(), uvEdgeSel);
	SetTVEdgeSel(uvEdgeSel);

	ClearTVVertSelection();
	//loop through our faces sync the verts
	int ct = 0;
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		if (GetFaceDead(i) == FALSE)
		{
			int deg = GetFaceDegree(i);
			for (int j = 0; j < deg; j++)
			{
				//get our geom index
				int geomIndex = GetFaceGeomVert(i, j);
				//get our tv index
				int tvIndex = GetFaceTVVert(i, j);
				//if geom index is selected select the tv index
				if (mHoldGeomSel.mVert[ct])
				{
					SetTVVertSelected(tvIndex, TRUE);
					SetGeomVertSelected(geomIndex, TRUE);
					if (GetTVVertFrozen(tvIndex))
					{
						SetTVVertSelected(tvIndex, FALSE);
						SetGeomVertSelected(geomIndex, FALSE);
					}
				}
				ct++;
			}
		}
	}

	mHoldGeomSel.mFace.SetSize(0);
	mHoldGeomSel.mEdge.SetSize(0);
	mHoldGeomSel.mVert.SetSize(0);
}

bool MeshTopoData::IsTVVertexFilterValid() const
{
	return (mTVVertexFilter.GetSize() && (mTVVertexFilter.GetSize() == mVSel.GetSize()));
}

bool MeshTopoData::DoesTVVertexPassFilter(int vIndex) const
{
	if (IsTVVertexFilterValid() && (vIndex < 0 || vIndex >= mTVVertexFilter.GetSize()))
	{
		DbgAssert(0); //we shouldn't be here
		return true;
	}

	return !IsTVVertexFilterValid() || mTVVertexFilter[vIndex];
}

void MeshTopoData::SyncTVVertexFilter()
{
	// We are assuming that the filtered faces (mFilterFaceSelection) are correct
	// Minimum check to make sure we are ok, we may not have a filtered list yet
	DbgAssert(mFaceFilter.GetSize() == GetNumberFaces());

	// Go through all filtered faces and add their vertices. 
	mTVVertexFilter.SetSize(GetNumberTVVerts());
	mTVVertexFilter.ClearAll();
	for (int i = 0; i < mFaceFilter.GetSize(); ++i)
	{
		if (!mFaceFilter[i]) continue;

		for (int j = 0; j < TVMaps.f[i]->count; ++j)
		{
			// Adding vertices 
			mTVVertexFilter.Set(TVMaps.f[i]->t[j]);

			if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
			{
				mTVVertexFilter.Set(TVMaps.f[i]->vecs->handles[j * 2]);
				mTVVertexFilter.Set(TVMaps.f[i]->vecs->handles[j * 2 + 1]);
				if (TVMaps.f[i]->flags & FLAG_INTERIOR)
				{
					mTVVertexFilter.Set(TVMaps.f[i]->vecs->interiors[j]);
				}
			}
		}
	}
	UpdateTVVertSelInIsolationMode();
}

void MeshTopoData::ClearTVVertexFilter()
{
	mTVVertexFilter.SetSize(0);
}

bool MeshTopoData::IsGeomVertexFilterValid() const
{
	return mGeomVertexFilter.GetSize() && (mGeomVertexFilter.GetSize() == mGVSel.GetSize());
}

bool MeshTopoData::DoesGeomVertexPassFilter(int vIndex) const
{
	if (IsGeomVertexFilterValid() && (vIndex < 0 || vIndex >= mGeomVertexFilter.GetSize()))
	{
		DbgAssert(0); //we shouldn't be here
		return true;
	}

	return !IsGeomVertexFilterValid() || mGeomVertexFilter[vIndex];
}

void MeshTopoData::SyncGeomVertexFilter()
{
	DbgAssert(mFaceFilter.GetSize() == GetNumberFaces());

	mGeomVertexFilter.SetSize(GetNumberGeomVerts());
	mGeomVertexFilter.ClearAll();
	for (int i = 0; i < mFaceFilter.GetSize(); ++i)
	{
		if (!mFaceFilter[i]) continue;

		for (int j = 0; j < GetFaceDegree(i); ++j)
		{
			mGeomVertexFilter.Set(GetFaceGeomVert(i, j));
		}
	}
	UpdateGeomVertSelInIsolationMode();
}

void MeshTopoData::ClearGeomVertexFilter()
{
	mGeomVertexFilter.SetSize(0);
}

bool MeshTopoData::IsTVEdgeFilterValid() const
{
	return (mTVEdgeFilter.GetSize() && (mTVEdgeFilter.GetSize() == mESel.GetSize()));
}

bool MeshTopoData::DoesTVEdgePassFilter(int eIndex) const
{
	if (IsTVEdgeFilterValid() && (eIndex < 0 || eIndex >= mTVEdgeFilter.GetSize()))
	{
		DbgAssert(0); //we shouldn't be here
		return true;
	}

	return !IsTVEdgeFilterValid() || mTVEdgeFilter[eIndex];
}

void MeshTopoData::SyncTVEdgeFilter()
{
	DbgAssert(mFaceFilter.GetSize() == GetNumberFaces());

	mTVEdgeFilter.SetSize(GetNumberTVEdges());
	mTVEdgeFilter.ClearAll();

	for (int i = 0; i < GetNumberTVEdges(); i++)
	{
		//get the face attached to that edge
		for (int j = 0; j < GetTVEdgeNumberTVFaces(i); j++)
		{
			//if find one face pass the filter,then the edge pass too.
			int currentFace = GetTVEdgeConnectedTVFace(i, j);
			if (mFaceFilter[currentFace])
			{
				mTVEdgeFilter.Set(i);
				break;
			}
		}
	}
	UpdateTVEdgeSelInIsolationMode();
}

void MeshTopoData::ClearTVEdgeFilter()
{
	mTVEdgeFilter.SetSize(0);
}

bool MeshTopoData::IsGeomEdgeFilterValid() const
{
	return (mGeomEdgeFilter.GetSize() && mGeomEdgeFilter.GetSize() == mGESel.GetSize());
}

bool MeshTopoData::DoesGeomEdgePassFilter(int eIndex) const
{
	if (IsGeomEdgeFilterValid() && (eIndex < 0 || eIndex >= mGeomEdgeFilter.GetSize()))
	{
		DbgAssert(0); //we shouldn't be here
		return true;
	}

	return !IsGeomEdgeFilterValid() || mGeomEdgeFilter[eIndex];

}

void MeshTopoData::SyncGeomEdgeFilter()
{
	DbgAssert(mFaceFilter.GetSize() == GetNumberFaces());

	mGeomEdgeFilter.SetSize(GetNumberGeomEdges());
	mGeomEdgeFilter.ClearAll();

	for (int i = 0; i < GetNumberGeomEdges(); i++)
	{
		//get the face attached to that edge
		for (int j = 0; j < GetGeomEdgeNumberOfConnectedFaces(i); j++)
		{
			//if find one face pass the filter,then the edge pass too.
			int currentFace = GetGeomEdgeConnectedFace(i, j);
			if (mFaceFilter[currentFace])
			{
				mGeomEdgeFilter.Set(i);
				break;
			}
		}
	}
	UpdateGeomEdgeSelInIsolationMode();
}

void MeshTopoData::ClearGeomEdgeFilter()
{
	mGeomEdgeFilter.SetSize(0);
}

void MeshTopoData::GetLoopedEdges(const BitArray &inSel, BitArray &outSel)
{
	//get our selection
	//build a vertex connection list
	Tab<VEdges*> edgesAtVertex;
	edgesAtVertex.SetCount(GetNumberGeomVerts());
	for (int i = 0; i < GetNumberGeomVerts(); i++)
		edgesAtVertex[i] = NULL;

	BitArray vertsConnectedToDegenerateFace;
	vertsConnectedToDegenerateFace.SetSize(GetNumberGeomVerts());
	vertsConnectedToDegenerateFace.ClearAll();
	for (int i = 0; i < GetNumberFaces(); i++)
	{
		int deg = GetFaceDegree(i);
		bool degenerateFace = false;
		for (int j = 0; j < deg; j++)
		{
			int vertIndex0 = GetFaceGeomVert(i, j);
			int vertIndex1 = GetFaceGeomVert(i, (j + 1) % deg);
			if (vertIndex0 == vertIndex1)
				degenerateFace = true;

		}
		if (degenerateFace)
		{
			for (int j = 0; j < deg; j++)
			{
				int vertIndex = GetFaceGeomVert(i, j);
				vertsConnectedToDegenerateFace.Set(vertIndex);
			}
		}
	}

	for (int i = 0; i < GetNumberGeomEdges(); i++)
	{
		{
			int a = GetGeomEdgeVert(i, 0);
			{
				if (edgesAtVertex[a] == NULL)
					edgesAtVertex[a] = new VEdges();

				edgesAtVertex[a]->edgeIndex.Append(1, &i, 5);
			}

			a = GetGeomEdgeVert(i, 1);
			{
				if (edgesAtVertex[a] == NULL)
					edgesAtVertex[a] = new VEdges();

				edgesAtVertex[a]->edgeIndex.Append(1, &i, 5);
			}
		}
	}

	//loop through our selection 
	outSel = inSel;
	//get our start and end point
	for (int i = 0; i < GetNumberGeomEdges(); i++)
	{
		//find the mid edge repeat until hit self or no mid edge
		if (inSel[i])
		{
			for (int j = 0; j < 2; j++)
			{
				int starVert = GetGeomEdgeVert(i, 0);
				if (j == 1) starVert = GetGeomEdgeVert(i, 1);
				int startEdge = i;
				int currentEdge = i;
				BOOL done = FALSE;
				while (!done)
				{
					//get the number of visible edges at this vert
					int numberEdgesAtThisVertex = 0;
					BOOL openEdge = FALSE;
					BOOL degenFound = FALSE;
					if (vertsConnectedToDegenerateFace[starVert])
						degenFound = TRUE;
					for (int k = 0; k < edgesAtVertex[starVert]->edgeIndex.Count(); k++)
					{
						int eindex = edgesAtVertex[starVert]->edgeIndex[k];
						int a = GetGeomEdgeVert(eindex, 0);
						int b = GetGeomEdgeVert(eindex, 1);
						if (a == b)
							degenFound = TRUE;
						if (GetGeomEdgeNumberOfConnectedFaces(eindex) == 1)
							openEdge = TRUE;
						if (!(GetGeomEdgeHidden(eindex)))
							numberEdgesAtThisVertex++;
					}

					//special case on an open edge where one of the loop edges is on the open edge,
					//then in this case the opposing open edge is a loop edge 
					if ((numberEdgesAtThisVertex == 3) && openEdge)
					{
						//make sure currentEdge is open edge
						if (GetGeomEdgeNumberOfConnectedFaces(currentEdge) == 1)
						{
							//find the other open edge
							int opposingEdge = -1;
							for (int k = 0; k < edgesAtVertex[starVert]->edgeIndex.Count(); k++)
							{
								int eindex = edgesAtVertex[starVert]->edgeIndex[k];
								int faceCount = GetGeomEdgeNumberOfConnectedFaces(eindex);
								if ((eindex != currentEdge) && (faceCount == 1))
								{
									opposingEdge = eindex;
									k = edgesAtVertex[starVert]->edgeIndex.Count();
								}
							}
							//if we find another open edge that is our next loop edge
							if (opposingEdge != -1)
							{
								currentEdge = opposingEdge;
							}
							else
								done = TRUE;

						}
						else
							done = TRUE;
					}
					//if odd bail
					//if there is an open edge bail
					else if (((numberEdgesAtThisVertex % 2) == 1) || openEdge || degenFound)
					{
						done = TRUE;
					}
					else
					{
						int goalEdge = numberEdgesAtThisVertex / 2;
						int goalCount = 0;
						//now find our opposite edge 
						int currentFace = GetGeomEdgeConnectedFace(currentEdge, 0);
						while (goalCount != goalEdge)
						{
							//loop through our edges find the one that is connected to this face
							for (int k = 0; k < edgesAtVertex[starVert]->edgeIndex.Count(); k++)
							{
								int eindex = edgesAtVertex[starVert]->edgeIndex[k];
								if (eindex != currentEdge)
								{
									for (int m = 0; m < GetGeomEdgeNumberOfConnectedFaces(eindex); m++)
									{
										if (GetGeomEdgeConnectedFace(eindex, m) == currentFace)
										{
											currentEdge = eindex;
											if (!(GetGeomEdgeHidden(eindex)))
											{
												goalCount++;
											}
											m = GetGeomEdgeNumberOfConnectedFaces(eindex);
											k = edgesAtVertex[starVert]->edgeIndex.Count();
										}
									}
								}
							}
							//find our next face
							for (int m = 0; m < GetGeomEdgeNumberOfConnectedFaces(currentEdge); m++)
							{
								if (GetGeomEdgeConnectedFace(currentEdge, m) != currentFace)
								{
									currentFace = GetGeomEdgeConnectedFace(currentEdge, m);
									m = GetGeomEdgeNumberOfConnectedFaces(currentEdge);
								}
							}
						}
						//set new edge
						//set the new vert
					}
					int a = GetGeomEdgeVert(currentEdge, 0);
					if (a == starVert)
						a = GetGeomEdgeVert(currentEdge, 1);
					starVert = a;

					if (!DoesGeomEdgePassFilter(currentEdge))
					{
						done = TRUE;// the edge is filtered out, stop.
					}
					else
					{
						outSel.Set(currentEdge, TRUE);
						if (currentEdge == startEdge)
							done = TRUE;
					}
				}
			}
		}
	}

	for (int i = 0; i < edgesAtVertex.Count(); i++)
	{
		if (edgesAtVertex[i])
			delete edgesAtVertex[i];
	}
}

void MeshTopoData::GetRingedEdges(const BitArray &inSel, BitArray &outSel)
{
	outSel = inSel;

	//get the selected edge
	for (int i = 0; i < GetNumberGeomEdges(); i++)
	{
		if (!inSel[i]) continue;

		//get the face attached to that edge
		for (int j = 0; j < GetGeomEdgeNumberOfConnectedFaces(i); j++)
		{
			//get all the visible edges attached to that face
			int currentFace = GetGeomEdgeConnectedFace(i, j);
			int currentEdge = i;
			Tab<int> facesToProcess;
			BitArray processedFaces(GetNumberFaces());
			do
			{
				BitArray edgesForThisFace(GetNumberGeomEdges());
				facesToProcess.Append(1, &currentFace, 100);
				while (facesToProcess.Count() > 0)
				{
					//pop the stack
					currentFace = facesToProcess[0];
					facesToProcess.Delete(0, 1);

					processedFaces.Set(currentFace, TRUE);

					int numberOfEdges = GetFaceDegree(currentFace);
					if (!IsFaceVisible(currentFace))
					{
						continue;
					}
					//loop through the edges
					for (int k = 0; k < numberOfEdges; k++)
					{
						//if edge is invisible add the edges of the cross face
						int a = GetFaceGeomVert(currentFace, k);
						int b = GetFaceGeomVert(currentFace, (k + 1) % numberOfEdges);
						if (a != b)
						{
							int eindex = FindGeoEdge(a, b);
							if (!(GetGeomEdgeHidden(eindex)))
							{
								edgesForThisFace.Set(eindex, TRUE);
							}
							else
							{
								for (int m = 0; m < GetGeomEdgeNumberOfConnectedFaces(eindex); m++)
								{
									int faceIndex = GetGeomEdgeConnectedFace(eindex, m);
									if (!processedFaces[faceIndex])
									{
										facesToProcess.Append(1, &faceIndex, 100);
									}
								}
							}
						}
					}
				}

				if (edgesForThisFace.NumberSet() != 4) break;

				//get the opposite edge 
				Tab<int> edgeList;
				for (int m = 0; m < edgesForThisFace.GetSize(); m++)
				{
					if (edgesForThisFace[m])
						edgeList.Append(1, &m, 100);
				}

				int vertIndex = GetGeomEdgeVert(currentEdge, 0);
				int step = 0;
				while (step != 2)
				{
					for (int ei = 0; ei < edgeList.Count(); ei++)
					{
						int potentialEdge = edgeList[ei];
						if (potentialEdge != currentEdge)
						{
							int a = GetGeomEdgeVert(potentialEdge, 0);
							int b = GetGeomEdgeVert(potentialEdge, 1);
							if (a == vertIndex)
							{
								vertIndex = b;
								currentEdge = potentialEdge;
								step++;
								break;
							}
							else if (b == vertIndex)
							{
								vertIndex = a;
								currentEdge = potentialEdge;
								step++;
								break;
							}
						}
					}
				}
				if (outSel[currentEdge]) 
					break;

				// if the edge is filtered by isolation mode, stop here.
				if (!DoesGeomEdgePassFilter(currentEdge))
					break;

				//now we're sure we've found a valid next edge
				outSel.Set(currentEdge, TRUE);

				if (GetGeomEdgeNumberOfConnectedFaces(currentEdge) == 1) break;

				//if we hit the start edge we are done
				if (currentEdge == i) break;

				//find next face
				for (int m = 0; m < GetGeomEdgeNumberOfConnectedFaces(currentEdge); m++)
				{
					int faceIndex = GetGeomEdgeConnectedFace(currentEdge, m);
					if ((faceIndex != currentFace) && (!processedFaces[faceIndex]))
					{
						currentFace = faceIndex;
						break;
					}
				}
			} while (true);
		}
	}
}

void MeshTopoData::SelectUVEdgeLoop(BOOL selectOpenEdges)
{
	BitArray holdSelection = GetTVEdgeSel();
	BitArray finalSelection = holdSelection;
	finalSelection.ClearAll();

	Tab<int> selEdges;
	for (int i = 0; i < holdSelection.GetSize(); i++)
	{
		if (holdSelection[i])
			selEdges.Append(1, &i, 1000);
	}

	for (int i = 0; i < selEdges.Count(); i++)
	{
		int edgeIndex = selEdges[i];
		int eselSet = 0;
		ClearTVEdgeSelection();
		SetTVEdgeSelected(edgeIndex, TRUE);

		while (eselSet != GetTVEdgeSel().NumberSet())
		{
			eselSet = GetTVEdgeSel().NumberSet();
			GrowUVLoop(selectOpenEdges);
			//get connecting a edge
		}
		finalSelection |= GetTVEdgeSel();
	}
	SetTVEdgeSel(finalSelection);
}

void MeshTopoData::GrowUVLoop(BOOL selectOpenEdges)
{
	BitArray esel = GetTVEdgeSel();
	BitArray vsel = GetTVVertSel();

	int edgeCount = esel.GetSize();
	int vertCount = vsel.GetSize();

	BitArray openEdgeSels = esel;
	openEdgeSels.ClearAll();

	if (!selectOpenEdges)
	{
		for (int i = 0; i < edgeCount; i++)
		{
			if (esel[i])
			{
				if (GetTVEdgeNumberTVFaces(i) <= 1)// open edge case
				{
					openEdgeSels.Set(i, TRUE);
					esel.Set(i, FALSE);
				}
			}
		}
	}

	// store per vertex info
	Tab<int> edgeConnectedCount; // how many SELECTED edges use this vertex
	Tab<int> numberEdgesAtVert; // how many edges use this vertex
	edgeConnectedCount.SetCount(vertCount);
	numberEdgesAtVert.SetCount(vertCount);

	for (int i = 0; i < vertCount; i++)
	{
		edgeConnectedCount[i] = 0;
		numberEdgesAtVert[i] = 0;
	}

	// find all the vertices that touch a selected edge
	// and keep a count of the number of selected edges that touch that //vertex  
	for (int i = 0; i < edgeCount; i++)
	{
		int a = GetTVEdgeVert(i, 0);
		int b = GetTVEdgeVert(i, 1);
		if (a != b)
		{
			if (!(GetTVEdgeHidden(i)))
			{
				numberEdgesAtVert[a]++;
				numberEdgesAtVert[b]++;
			}
			if (esel[i])
			{
				edgeConnectedCount[a]++;
				edgeConnectedCount[b]++;
			}
		}
	}

	BitArray edgesToExpand;
	edgesToExpand.SetSize(edgeCount);
	edgesToExpand.ClearAll();

	//tag any edge that has only one vertex count since that will be an end edge  
	for (int i = 0; i < edgeCount; i++)
	{
		int a = GetTVEdgeVert(i, 0);
		int b = GetTVEdgeVert(i, 1);
		if (a != b)
		{
			if (esel[i])
			{
				if ((edgeConnectedCount[a] == 1) || (edgeConnectedCount[b] == 1))
					edgesToExpand.Set(i, TRUE);
			}
		}
	}

	for (int i = 0; i < edgeCount; i++)
	{
		if (edgesToExpand[i])
		{
			// make sure we have an even number of edges at this vert
			// if odd then we can not go further
			// if ((numberEdgesAtVert[i] % 2) == 0)
			// now need to find our most opposing edge
			// find all edges connected to the vertex
			for (int k = 0; k < 2; k++)
			{
				int a = 0;
				if (k == 0)
					a = GetTVEdgeVert(i, 0);
				else
					a = GetTVEdgeVert(i, 1);

				int oddCount = (numberEdgesAtVert[a] % 2);


				if ((GetTVEdgeNumberTVFaces(i) <= 1) && selectOpenEdges)
				{
					if (numberEdgesAtVert[a] == 3) // open edge case
						oddCount = 0;
				}

				if ((edgeConnectedCount[a] == 1) && (oddCount == 0)) //only one edge of this vertex is selected, this should be true based on how edgesToExpand is created
				{
					int centerVert = a;
					Tab<int> edgesConnectedToVert;
					// collect all the edges indices of vertex a
					for (int j = 0; j < edgeCount; j++)
					{
						if (j != i)  //make sure we dont add our selected vertex
						{
							int a = GetTVEdgeVert(j, 0);
							int b = GetTVEdgeVert(j, 1);

							if (a != b)
							{
								if ((a == centerVert) || (b == centerVert))
								{
									edgesConnectedToVert.Append(1, &j);
								}
							}
						}
					}

					int count = numberEdgesAtVert[centerVert] / 2; //usually a vertex will be shared with 4 faces, 4edges if not open. If open, the vertex may only be has 2 to 3 edges to it.
					if ((GetTVEdgeNumberTVFaces(i) <= 1) && selectOpenEdges)
					{
						count = 2;
					}
					int tally = 0;
					BOOL done = FALSE;
					// since edge i is selected, get all the faces which share this selected edges
					// then if the edge connected to this vertex belong to any of these faces,
					// then the edge is not a edge to be selected by grow, else the edge will be selected.
					int faceIndex = GetTVEdgeConnectedTVFace(i, 0);
					while (!done)
					{
						int lastEdge = -1;
						for (int m = 0; m < edgesConnectedToVert.Count(); m++)
						{
							int edgeIndex = edgesConnectedToVert[m];
							for (int n = 0; n < GetTVEdgeNumberTVFaces(edgeIndex); n++)
							{
								if (faceIndex == GetTVEdgeConnectedTVFace(edgeIndex, n))
								{
									for (int p = 0; p < GetTVEdgeNumberTVFaces(edgeIndex); p++)
									{
										if (faceIndex != GetTVEdgeConnectedTVFace(edgeIndex, p))
										{
											faceIndex = GetTVEdgeConnectedTVFace(edgeIndex, p);
											p = GetTVEdgeNumberTVFaces(edgeIndex);//break the loop
										}
									}
									if (!(GetTVEdgeHidden(edgeIndex)))
										tally++;
									edgesConnectedToVert.Delete(m, 1);
									m = edgesConnectedToVert.Count(); // break the loop
									n = GetTVEdgeNumberTVFaces(edgeIndex); // break the loop
									lastEdge = edgeIndex;
								}
							}
						}
						if (lastEdge == -1)
						{
							done = TRUE;
						}
						if (tally >= count)
						{
							done = TRUE;
							if (lastEdge != -1)
								esel.Set(lastEdge, TRUE);
						}
					}
				}
			}
		}
	}
	esel |= openEdgeSels;
	SetTVEdgeSel(esel);
}

void MeshTopoData::GetUVRingEdges(int edgeIdx, BitArray& ringEdges, bool bFindAll, std::vector<int>* pVec)
{
	BitArray gesel = GetTVEdgeSel();
	ringEdges = gesel;
	ringEdges.ClearAll();
	ringEdges.Set(edgeIdx);
	if (pVec)
	{
		pVec->push_back(edgeIdx);
	}

	//get the face attached to that edge
	for (int j = 0; j < GetTVEdgeNumberTVFaces(edgeIdx); ++j)
	{
		//get all the visible edges attached to that face
		int currentFace = GetTVEdgeConnectedTVFace(edgeIdx, j);
		int currentEdge = edgeIdx;
		BOOL done = FALSE;
		int startEdge = currentEdge;
		BitArray edgesForThisFace;
		edgesForThisFace.SetSize(GetNumberTVEdges());

		Tab<int> facesToProcess;
		BitArray processedFaces;
		processedFaces.SetSize(GetNumberFaces());
		processedFaces.ClearAll();

		while (!done)
		{
			edgesForThisFace.ClearAll();
			facesToProcess.Append(1, &currentFace, 100);
			while (facesToProcess.Count() > 0)
			{
				//pop the stack
				currentFace = facesToProcess[0];
				facesToProcess.Delete(0, 1);

				processedFaces.Set(currentFace, TRUE);
				if (!IsFaceVisible(currentFace)) // halt if we meet an invisible face.
				{
					continue;
				}
				int numberOfEdges = GetFaceDegree(currentFace);
				//loop through the edges
				for (int k = 0; k < numberOfEdges; k++)
				{
					//if edge is invisisble add the edges of the cross face
					int a = GetFaceTVVert(currentFace, k);
					int b = GetFaceTVVert(currentFace, (k + 1) % numberOfEdges);
					if (a != b)
					{
						int eindex = FindUVEdge(a, b);
						if (!GetTVEdgeHidden(eindex))
						{
							edgesForThisFace.Set(eindex, TRUE);
						}
						else
						{
							for (int m = 0; m < GetTVEdgeNumberTVFaces(eindex); m++)
							{
								int faceIndex = GetTVEdgeConnectedTVFace(eindex, m);
								if (!processedFaces[faceIndex])
								{
									facesToProcess.Append(1, &faceIndex, 100);
								}
							}
						}
					}
				}
			}
			//if it is not a quad we are done
			if (edgesForThisFace.NumberSet() != 4)
				done = TRUE;
			else // ok we have all the edges, decides which one is in the ring!
			{
				//get the mid edge 
				//start at the seed
				int goal = edgesForThisFace.NumberSet() / 2;
				int currentGoal = 0;
				int vertIndex = GetTVEdgeVert(currentEdge, 0);
				Tab<int> edgeList;
				for (int m = 0; m < edgesForThisFace.GetSize(); m++)
				{
					if (edgesForThisFace[m])
						edgeList.Append(1, &m, 100);
				}

				int holdCurrentEdge = currentEdge;
				//find next edge
				while (currentGoal != goal)
				{

					BOOL noHit = TRUE;
					for (int i = 0; i < edgeList.Count(); i++)
					{

						int potentialEdge = edgeList[i];
						if (potentialEdge != currentEdge)
						{
							int a = GetTVEdgeVert(potentialEdge, 0);
							int b = GetTVEdgeVert(potentialEdge, 1);
							if (a == vertIndex)
							{
								vertIndex = b;
								currentEdge = potentialEdge;
								i = edgeList.Count();

								//increment current
								currentGoal++;
								noHit = FALSE;
							}
							else if (b == vertIndex)
							{
								vertIndex = a;
								currentEdge = potentialEdge;
								i = edgeList.Count();

								//increment current
								currentGoal++;
								noHit = FALSE;
							}

						}
					}
					//this is a case where we have a hidden edge on the border which breaks the border loop
					//in this case we can just bail since the loop is incomplete and we are done
					if (noHit)
					{
						currentGoal = goal;
						done = TRUE;
						currentEdge = holdCurrentEdge;
					}
				}
			}

			if (ringEdges[currentEdge])
				done = TRUE;

			if (pVec && !ringEdges[currentEdge])
			{
				pVec->push_back(currentEdge);
			}
			ringEdges.Set(currentEdge, TRUE);


			for (int m = 0; m < GetTVEdgeNumberTVFaces(currentEdge); m++)
			{
				int faceIndex = GetTVEdgeConnectedTVFace(currentEdge, m);
				if ((faceIndex != currentFace) && (!processedFaces[faceIndex]))
				{
					currentFace = faceIndex;
					m = GetTVEdgeNumberTVFaces(currentEdge);
				}
			}
			if (GetTVEdgeNumberTVFaces(currentEdge) == 1)
				done = TRUE;
			//if we hit the start egde we are done
			if (currentEdge == startEdge)
				done = TRUE;

			if (!bFindAll)
				done = TRUE;
		}
	}
}

void MeshTopoData::PolySelection(BOOL add, BOOL bPreview)
{
	BitArray esel = bPreview ? GetTVEdgeSelectionPreview() : GetTVEdgeSel();
	BitArray fsel = bPreview ? GetFaceSelectionPreview() : GetFaceSel();
	BitArray originalESel(esel);
	//convert to edge sel
	if (bPreview)
	{
		ConvertFaceToEdgeSelPreview();
	}
	else
	{
		ConvertFaceToEdgeSel();
	}

	esel = bPreview ? GetTVEdgeSelectionPreview() : GetTVEdgeSel();
	int eSelCount = esel.GetSize();

	for (int i = 0; i < eSelCount; i++)
	{
		if ((esel[i]) && (GetTVEdgeHidden(i)))
		{
			int ct = GetTVEdgeNumberTVFaces(i);
			BOOL unselFace = FALSE;
			for (int j = 0; j < ct; j++)
			{
				int index = GetTVEdgeConnectedTVFace(i, j);
				if (add && (!fsel[index]))
				{
					fsel.Set(index);
				}
				else if ((!add) && (!fsel[index]))
				{
					unselFace = TRUE;
					break;
				}
			}
			if (unselFace)
			{
				for (int j = 0; j < ct; j++)
				{
					int index = GetTVEdgeConnectedTVFace(i, j);//TVMaps.ePtrList[i]->faceList[j];
					fsel.Set(index, FALSE);
				}
			}
		}
	}
	esel = originalESel;
	if (bPreview)
	{
		SetTVEdgeSelectionPreview(esel);
		SetFaceSelectionPreview(fsel);
	}
	else
	{
		SetTVEdgeSel(esel);
		SetFaceSelectionByRef(fsel);
	}

}

bool MeshTopoData::GetSewingPending()
{
	return mhasSewingPending;
}

void MeshTopoData::SetSewingPending()
{
	mhasSewingPending = true;
}

void MeshTopoData::ClearSewingPending()
{
	mhasSewingPending = false;
}

void MeshTopoData::VerifyTopoValidForBreakEdges(MeshTopoData* meshTopoData)
{
	// To verify whether the 2D topo faces are welded or have the same UV.
	// If that, the break edges cannot be done in these 2D topo edges.
	if (nullptr == meshTopoData) return;
	int numFaces = meshTopoData->TVMaps.f.Count();
	for (int i = 0; i < numFaces; i++)
	{
		if (meshTopoData->mbBreakEdgesSucceeded) // if break edge operation is done, it means that the 2D topo is valid for breaking edges.
		{
			meshTopoData->mbValidTopoForBreakEdges = true;
			return;
		}
		int numEdgesInOneFace = meshTopoData->TVMaps.f[i]->count; // numEdgesInOneFace can only be 3 or 4

		for (int j = i + 1; j < numFaces; j++)
		{
			if (meshTopoData->mbBreakEdgesSucceeded) // if break edge operation is done, it means that the 2D topo is valid for breaking edges.
			{
				meshTopoData->mbValidTopoForBreakEdges = true;
				return;
			}

			int tempIndex0 = 0;
			int tempIndex1 = 0;
			int tempIndex2 = 0;
			int tempIndex3 = 0;
			if ((numEdgesInOneFace == 3) && (meshTopoData->TVMaps.f[j]->count==3)) // mesh corresponding 2D topo
			{
				tempIndex0 = meshTopoData->TVMaps.f[i]->t[0];
				tempIndex1 = meshTopoData->TVMaps.f[i]->t[1];
				tempIndex2 = meshTopoData->TVMaps.f[i]->t[2];
				if (tempIndex0 == meshTopoData->TVMaps.f[j]->t[0] &&
					tempIndex1 == meshTopoData->TVMaps.f[j]->t[1] &&
					tempIndex2 == meshTopoData->TVMaps.f[j]->t[2])
				{
					meshTopoData->mbValidTopoForBreakEdges = false;
					return;
				}
			}
			else if ((numEdgesInOneFace == 4) && (meshTopoData->TVMaps.f[j]->count==4)) // poly corresponding 2D topo
			{
				tempIndex0 = meshTopoData->TVMaps.f[i]->t[0];
				tempIndex1 = meshTopoData->TVMaps.f[i]->t[1];
				tempIndex2 = meshTopoData->TVMaps.f[i]->t[2];
				tempIndex3 = meshTopoData->TVMaps.f[i]->t[3];
				if (tempIndex0 == meshTopoData->TVMaps.f[j]->t[0] &&
					tempIndex1 == meshTopoData->TVMaps.f[j]->t[1] &&
					tempIndex2 == meshTopoData->TVMaps.f[j]->t[2] &&
					tempIndex3 == meshTopoData->TVMaps.f[j]->t[3])
				{
					meshTopoData->mbValidTopoForBreakEdges = false;
					return;
				}
			}
		}
	}
	meshTopoData->mbValidTopoForBreakEdges = true;
}

void MeshTopoData::SychronizeSomeData()
{
	if (mVertexClusterList.Count() != GetNumberTVVerts())
	{
		BuildVertexClusterList();
	}

	//need to check matid
	if (mVertMatIDFilterList.GetSize() != GetNumberTVVerts())
	{
		mVertMatIDFilterList.SetSize(GetNumberTVVerts());
		mVertMatIDFilterList.SetAll();
	}
}

void MeshTopoData::HandleTopoChanged()
{
	if (ResizeTVVertSelection(TVMaps.v.Count()))
	{
		ClearTVVertSelection();
	}

	if (mVSelPreview.GetSize() != TVMaps.v.Count())
	{
		mVSelPreview.SetSize(TVMaps.v.Count());
		ClearTVVertSelectionPreview();
	}
	if (mESel.GetSize() != TVMaps.ePtrList.Count())
	{
		mESel.SetSize(TVMaps.ePtrList.Count());
		ClearTVEdgeSelection();
	}
	if (mESelPreview.GetSize() != TVMaps.ePtrList.Count())
	{
		mESelPreview.SetSize(TVMaps.ePtrList.Count());
		ClearTVEdgeSelectionPreview();
	}
	if (mFSel.GetSize() != TVMaps.f.Count())
	{
		mFSel.SetSize(TVMaps.f.Count());
		ClearFaceSelection();
	}
	if (mFSelPreview.GetSize() != TVMaps.f.Count())
	{
		mFSelPreview.SetSize(TVMaps.f.Count());
		ClearFaceSelectionPreview();
	}
	if (mGVSel.GetSize() != TVMaps.geomPoints.Count())
	{
		mGVSel.SetSize(TVMaps.geomPoints.Count());
		ClearGeomVertSelection();
	}
	if (mGESel.GetSize() != TVMaps.gePtrList.Count())
	{
		mGESel.SetSize(TVMaps.gePtrList.Count());
		ClearGeomEdgeSelection();
	}
	mOpenTVEdges = ComputeOpenTVEdges();
	mOpenGeomEdges = ComputeOpenGeomEdges();
	BuildVertexClusterList();
	UpdateAllFilters();
}

void MeshTopoData::BuildAllFilters(bool enable)
{
	if (enable)
	{
		BuildFaceFilter();
		SyncNonFaceFilters();
	}
	else
	{
		ClearAllFilters();
	}
}

void MeshTopoData::UpdateAllFilters()
{
	if (!IsFaceFilterValid())
	{
		ClearAllFilters();
	}
	else
	{
		if (!IsNonFaceFiltersValid())
		{
			SyncNonFaceFilters();
		}
	}
}

void MeshTopoData::ClearAllFilters()
{
	ClearFaceFilter();
	ClearTVVertexFilter();
	ClearTVEdgeFilter();
	ClearGeomVertexFilter();
	ClearGeomEdgeFilter();
}

void MeshTopoData::SyncNonFaceFilters()
{
	SyncTVVertexFilter();
	SyncTVEdgeFilter();
	SyncGeomVertexFilter();
	SyncGeomEdgeFilter();
}

bool MeshTopoData::IsNonFaceFiltersValid()
{
	return IsTVVertexFilterValid()
		&& IsTVEdgeFilterValid()
		&& IsGeomVertexFilterValid()
		&& IsGeomEdgeFilterValid();
}

bool MeshTopoData::IsFaceFilterValid() const
{
	return (mFaceFilter.GetSize() && (mFaceFilter.GetSize() == mFSel.GetSize()));
}

bool MeshTopoData::DoesFacePassFilter(int fIndex) const
{
	if (mMaterialIDFaceFilter.GetSize() == 0)
		return true;
	if (IsFaceFilterValid() && fIndex >= mFaceFilter.GetSize())
	{
		DbgAssert(0); //we shouldn't be here
		return true;
	}

	if (fIndex < 0 || fIndex >= mMaterialIDFaceFilter.GetSize())
	{
		DbgAssert(0); //we shouldn't be here
		return true;
	}

	return (!IsFaceFilterValid() || mFaceFilter[fIndex]) && mMaterialIDFaceFilter[fIndex];
}

void MeshTopoData::BuildFaceFilter()
{
	mFaceFilter = GetFaceSel();
}

void MeshTopoData::ClearFaceFilter()
{
	mFaceFilter.SetSize(0);
}

BOOL MeshTopoData::GetSynchronizationToRenderItemFlag()
{
	return mbNeedSynchronizationToRenderItem;
}
void MeshTopoData::SetSynchronizationToRenderItemFlag(BOOL bFlag)
{
	mbNeedSynchronizationToRenderItem = bFlag;
}

bool MeshTopoData::CheckPoly(int polyIndex)
{
	//degenerate and dead faces should be skipped
	if (GetFaceDead(polyIndex))
		return false;

	//we dont process hidden faces
	if (GetFaceHidden(polyIndex) == TRUE)
		return false;

	//make sure no shared vertices or zero length edges
	BitArray verts;
	verts.SetSize(GetNumberGeomVerts());
	verts.ClearAll();
	int degree = GetFaceDegree(polyIndex);
	for (int i = 0; i < degree; i++)
	{
		int index = GetFaceGeomVert(polyIndex, i);
		if (verts[index])
			return false;
		verts.Set(index, TRUE);
		int nextIndex = GetFaceGeomVert(polyIndex, (i + 1) % degree);
		Point3 a = GetGeomVert(index);
		Point3 b = GetGeomVert(nextIndex);

		if (Length(a - b) == 0.0f)
			return false;
	}

	return true;
}

void MeshTopoData::AddLSCMFaceData(int polyIndex, Tab<LSCMFace>& targetFacesTab,bool forPeel)
{
	//if mesh we can just add the triangle
	if (GetMesh())
	{
		LSCMFace face;
		face.mPolyIndex = polyIndex;
		face.mEdge[0].mIthEdge = 0;
		face.mEdge[1].mIthEdge = 1;
		face.mEdge[2].mIthEdge = 2;
		targetFacesTab.Append(1, &face, GetNumberFaces() / 10);

	}
	//if poly we need to triangulate
	else if (GetMNMesh())
	{
		MNMesh *mesh = GetMNMesh();
		int degree = GetFaceDegree(polyIndex);
		if (degree == 4 &&
			forPeel)
		{
			// Deal with this case specifically
			// Use bias and length to make sure split direction is identical
			// This algorithm can produces symmetric triangulation when model is symmetric
			float bias = 1.0f + 1e-6f;
			int index[4];
			index[0] = GetFaceGeomVert(polyIndex, 0);
			index[1] = GetFaceGeomVert(polyIndex, 1);
			index[2] = GetFaceGeomVert(polyIndex, 2);
			index[3] = GetFaceGeomVert(polyIndex, 3);

			Point3 p[4];
			p[0] = GetGeomVert(index[0]);
			p[1] = GetGeomVert(index[1]);
			p[2] = GetGeomVert(index[2]);
			p[3] = GetGeomVert(index[3]);

			float len02 = (p[0] - p[2]).Length();
			float len13 = (p[1] - p[3]).Length();

			float fac = len02 * bias - len13;

			LSCMFace face;
			face.mPolyIndex = polyIndex;
			if (fac <= 0)
			{
				face.mEdge[0].mIthEdge = 0;
				face.mEdge[1].mIthEdge = 1;
				face.mEdge[2].mIthEdge = 2;
				targetFacesTab.Append(1, &face, GetNumberFaces() / 10);

				face.mEdge[0].mIthEdge = 0;
				face.mEdge[1].mIthEdge = 2;
				face.mEdge[2].mIthEdge = 3;
				targetFacesTab.Append(1, &face, GetNumberFaces() / 10);
			}
			else
			{
				face.mEdge[0].mIthEdge = 0;
				face.mEdge[1].mIthEdge = 1;
				face.mEdge[2].mIthEdge = 3;
				targetFacesTab.Append(1, &face, GetNumberFaces() / 10);

				face.mEdge[0].mIthEdge = 2;
				face.mEdge[1].mIthEdge = 3;
				face.mEdge[2].mIthEdge = 1;
				targetFacesTab.Append(1, &face, GetNumberFaces() / 10);
			}
		}
		else
		{
			int numberFaces = degree - 2;
			MNFace *mnFace = &mesh->f[polyIndex];
			Tab<int> tri;
			mnFace->GetTriangles(tri);
			for (int i = 0; i < numberFaces; i++)
			{
				int *triv = tri.Addr(i * 3);

				LSCMFace face;
				face.mPolyIndex = polyIndex;
				face.mEdge[0].mIthEdge = triv[0];
				face.mEdge[1].mIthEdge = triv[1];
				face.mEdge[2].mIthEdge = triv[2];
				targetFacesTab.Append(1, &face, GetNumberFaces() / 10);
			}
		}
	}
	//if patch we need create triangulation
	else if (GetPatch())
	{
		int degree = GetFaceDegree(polyIndex);
		if (degree == 3)
		{
			LSCMFace face;
			face.mPolyIndex = polyIndex;
			face.mEdge[0].mIthEdge = 0;
			face.mEdge[1].mIthEdge = 1;
			face.mEdge[2].mIthEdge = 2;
			targetFacesTab.Append(1, &face, GetNumberFaces() / 10);
		}
		else
		{
			LSCMFace face;
			face.mPolyIndex = polyIndex;
			face.mEdge[0].mIthEdge = 0;
			face.mEdge[1].mIthEdge = 1;
			face.mEdge[2].mIthEdge = 3;
			targetFacesTab.Append(1, &face, GetNumberFaces() / 10);

			face.mEdge[0].mIthEdge = 2;
			face.mEdge[1].mIthEdge = 3;
			face.mEdge[2].mIthEdge = 1;
			targetFacesTab.Append(1, &face, GetNumberFaces() / 10);
		}
	}
}

void MeshTopoData::UpdateSharedTVEdges(int curSubLvl)
{
	if (curSubLvl == TVEDGEMODE)
	{
		_GeomTVEdgeSelMutualConvertion(mGESel, mSharedTVEdges, true);
		int numberSet = mESel.NumberSet();
		if(numberSet > 1)
		{
			//if numberSet == 1 and mSharedTVEdges &= ~mESel,
			//then the first seam will be invisible because the first seam and selected TV edge is same.
			mSharedTVEdges &= ~mESel;
		}			
	}
	else
	{
		BitArray usedClusters;
		usedClusters.SetSize(GetNumberGeomVerts());
		usedClusters.ClearAll();

		TransferSelectionStart(curSubLvl);
		for (int i = 0; i < GetNumberTVVerts(); i++)
		{
			if (GetTVVertSelected(i))
			{
				int gIndex = GetTVVertGeoIndex(i);
				if (gIndex >= 0)
					usedClusters.Set(gIndex);
			}
		}
		TransferSelectionEnd(curSubLvl, FALSE, FALSE);

		int iTVEdgesCount = GetNumberTVEdges();
		if (mSharedTVEdges.GetSize() != iTVEdgesCount)
		{
			mSharedTVEdges.SetSize(iTVEdgesCount);
		}
		mSharedTVEdges.ClearAll();

		for (int i = 0; i < iTVEdgesCount; i++)
		{
			int a = GetTVEdgeVert(i, 0);
			int b = GetTVEdgeVert(i, 1);

			int ct = 0;
			int vCluster = GetTVVertGeoIndex(a);
			if ((vCluster >= 0) && (usedClusters[vCluster]))
				ct++;

			vCluster = GetTVVertGeoIndex(b);
			if ((vCluster >= 0) && (usedClusters[vCluster]))
				ct++;

			if (ct == 2)
				mSharedTVEdges.Set(i, TRUE);
		}
	}
}

int MeshTopoData::RegisterNotification(IMeshTopoDataChangeListener* listener)
{
	if (listener)
	{
		for (int i = 0; i < mListenerTab.Count(); ++i)
		{
			if (mListenerTab[i] == listener)
			{
				return 1; //already registered, return.
			}
		}
		// firstly try to use any null member.
		for (int i = 0; i < mListenerTab.Count(); ++i)
		{
			if (mListenerTab[i] == nullptr)
			{
				mListenerTab[i] = listener;
				return 1;
			}
		}
		mListenerTab.Append(1, &listener); //append if no null member exists.
	}
	return 1;
}

int MeshTopoData::UnRegisterNotification(IMeshTopoDataChangeListener* listener)
{
	if (listener)
	{
		for (int i = 0; i < mListenerTab.Count(); ++i)
		{
			if (mListenerTab[i] == listener)
			{
				mListenerTab[i] = nullptr;
			}
		}
	}
	return 1;
}


int MeshTopoData::RaiseTVDataChanged(BOOL bUpdateView)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnTVDataChanged(this, bUpdateView);
		}
	}
	return 1;
}

int MeshTopoData::RaiseTVertFaceChanged(BOOL bUpdateView)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnTVertFaceChanged(this, bUpdateView);
		}
	}
	return 1;
}

int MeshTopoData::RaiseSelectionChanged(BOOL bUpdateView)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnSelectionChanged(this, bUpdateView);
		}
	}
	return 1;
}

int MeshTopoData::RaisePinAddedOrDeleted(int index)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnPinAddedOrDeleted(this, index);
		}
	}
	return 1;
}

int MeshTopoData::RaisePinInvalidated(int index)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnPinInvalidated(this, index);
		}
	}
	return 1;
}

int MeshTopoData::RaiseTopoInvalidated()
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnTopoInvalidated(this);
		}
	}
	return 1;
}

int MeshTopoData::RaiseTVVertChanged(TimeValue t, int index, const Point3& p)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnTVVertChanged(this, t, index, p);
		}
	}
	return 1;
}

int MeshTopoData::RaiseTVVertDeleted(int index)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnTVVertDeleted(this, index);
		}
	}
	return 1;
}

int MeshTopoData::RaiseMeshTopoDataDeleted()
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnMeshTopoDataDeleted(this);
		}
	}
	return 1;
}

int MeshTopoData::RaiseNotifyUpdateTexmap()
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnNotifyUpdateTexmap(this);
		}
	}
	return 1;
}

int MeshTopoData::RaiseNotifyUIInvalidation(BOOL bRedraw)
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnNotifyUIInvalidation(this, bRedraw);
		}
	}
	return 1;
}

int MeshTopoData::RaiseNotifyFaceSelectionChanged()
{
	for (int i = 0; i < mListenerTab.Count(); ++i)
	{
		if (mListenerTab[i] != nullptr)
		{
			mListenerTab[i]->OnNotifyFaceSelectionChanged(this);
		}
	}
	return 1;
}

ConstantTopoAccelerator::ConstantTopoAccelerator(MeshTopoData *ld, BOOL enableFilterFaceSelection)
	: mLD(ld)
	, mFiltered(enableFilterFaceSelection)
{
	mLD->BeginConstantTopoAcceleration(mFiltered);
}

ConstantTopoAccelerator::~ConstantTopoAccelerator()
{
	mLD->EndConstantTopoAcceleration();
}

const FacesAtVerts* ConstantTopoAccelerator::GetFacesAtVerts()
{
	return mLD->GetFacesAtVerts(mFiltered);
}

void MeshTopoData::NonSquareAdjustVertexY(float aspect, float fVOffset, bool bInverse)
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

	static float aspectThreshold = 0.01f;
	if (abs(aspect - 1.0f) < aspectThreshold)
	{
		return;
	}

	const int iTVVertsCount = GetNumberTVVerts();
	double fYSum = 0.0;
	for (int i = 0; i < iTVVertsCount; i++)
	{
		fYSum += GetTVVert(i).y;
	}

	const float fYCenter = iTVVertsCount != 0 ? float(fYSum / iTVVertsCount) : fVOffset;
	const float aspectScale = bInverse ? (1.0f / aspect) : aspect;

	TimeValue t = GetCOREInterface()->GetTime();
	for (int i = 0; i < iTVVertsCount; i++)
	{
		Point3 p = GetTVVert(i);
		p.y = (p.y - fYCenter) * aspectScale + fYCenter;
		SetTVVert(t, i, p);
	}
}

bool MeshTopoData::IsFaceFilterBitsetOrMaterialIDFilterSet() const
{
	int iSetOneCount = mMaterialIDFaceFilter.NumberSet();
	return (IsFaceFilterValid() && mFaceFilter.AnyBitSet() || (iSetOneCount != 0 && iSetOneCount != mMaterialIDFaceFilter.GetSize()));
}

const BitArray& MeshTopoData::GetFaceFilter()
{
	static BitArray combinedArray;
	if (combinedArray.GetSize() != TVMaps.f.Count())
	{
		combinedArray.SetSize(TVMaps.f.Count());
	}
	
	combinedArray.SetAll();
	if (IsFaceFilterValid())
	{
		combinedArray &= mFaceFilter;
	}
	combinedArray &= mMaterialIDFaceFilter;

	return combinedArray;
}