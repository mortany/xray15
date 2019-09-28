/*

Copyright 2010 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#include <cmath>
#include "unwrap.h"
#include "modsres.h"
#include "iMenuMan.h"


void UnwrapMod::UpdatePaintInfluence(Tab<TVHitData>& hits, Point2 center, Tab<float>& hitsInflu)
{
	int count = hits.Count();
	int localDataID;
	int index;
	MeshTopoData* meshTopoData = nullptr;
	Point3 tvPt3;
	float xzoom, yzoom;
	int width, height;
	ComputeZooms(hView, xzoom, yzoom, width, height);
	Point2 screenUV;
	float outerRadius = fnGetPaintFallOffSize();
	float innerRadius = fnGetPaintFullStrengthSize();
	float closest_dist = BIGFLOAT;
	float delta = outerRadius - innerRadius;
	hitsInflu.SetCount(count);
	for (int i = 0; i < count; ++i)
	{
		closest_dist = BIGFLOAT;
		localDataID = hits[i].mLocalDataID;
		index = hits[i].mID;
		meshTopoData = mMeshTopoData[localDataID];
		hitsInflu[i] = 0.0f;
		if (meshTopoData)
		{
			tvPt3 = meshTopoData->GetTVVert(index);
			screenUV = UVWToScreen(tvPt3, xzoom, yzoom, width, height);
			closest_dist = (screenUV - center).Length();
			if (closest_dist <= innerRadius)
			{
				hitsInflu[i] = 1.0f;
			}
			else if (closest_dist <= outerRadius && delta != 0.0f)
			{
				float influ = 1.0f - ((closest_dist - innerRadius) / delta);
				ComputeFalloff(influ, fnGetPaintFallOffType());
				hitsInflu[i] = influ;
			}
		}
	}
}

void UnwrapMod::RebuildDistCache()
{
	UpdatePivot();
	freeFormPivotOffset.x = 0.0f;
	freeFormPivotOffset.y = 0.0f;
	freeFormPivotOffset.z = 0.0f;

	float str = mUIManager.GetSpinFValue(ID_SOFTSELECTIONSTR_SPINNER);	
	float sstr = str*str;
	if ((str == 0.0f) || (enableSoftSelection == FALSE))
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (!(ld->GetTVVertWeightModified(i)))
					ld->SetTVVertInfluence(i, 0.0f);
			}
		}

		return;
	}

	Tab<Point3> selectedVerts;
	if (falloffSpace == 0)
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld->GetGeomVertSel().AnyBitSet())
			{
				for (int i = 0; i < ld->GetNumberGeomVerts(); i++)
				{
					if (ld->GetGeomVertSelected(i))
					{
						Point3 p = ld->GetGeomVert(i);
						selectedVerts.Append(1, &p, 10000);
					}
				}
			}
		}
	}
	else
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld->GetTVVertSel().AnyBitSet())
			{
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if (ld->GetTVVertSelected(i))
					{
						Point3 p = ld->GetTVVert(i);
						selectedVerts.Append(1, &p, 10000);
					}
				}
			}
		}
	}

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray useableVerts;

		useableVerts.SetSize(ld->GetNumberTVVerts());//TVMaps.v.Count());

		if (limitSoftSel)
		{
			int oldMode = fnGetTVSubMode();
			fnSetTVSubMode(TVVERTMODE);
			useableVerts.ClearAll();
			BitArray holdSel;
			BitArray vsel = ld->GetTVVertSel();
			holdSel.SetSize(vsel.GetSize());
			holdSel = vsel;
			for (int i = 0; i < limitSoftSelRange; i++)
			{
				ExpandSelection(ld, 0);
			}
			vsel = ld->GetTVVertSel();
			useableVerts = vsel;
			ld->SetTVVertSel(holdSel);
			fnSetTVSubMode(oldMode);
		}
		else useableVerts.SetAll();

		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++)
		{
			if (!(ld->GetTVVertWeightModified(i)))//TVMaps.v[i].flags & FLAG_WEIGHTMODIFIED))
				ld->SetTVVertInfluence(i, 0.0f);//TVMaps.v[i].influence = 0.0f;
		}

		Tab<Point3> xyzSpace;

		if (falloffSpace == 0) //xy space
		{
			xyzSpace.SetCount(ld->GetNumberTVVerts());
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				xyzSpace[i] = Point3(0.0f, 0.0f, 0.0f);
			for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count(); i++)
			{
				if (!ld->GetFaceDead(i))
				{
					int pcount = 3;
					pcount = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
					for (int j = 0; j < pcount; j++)
					{
						xyzSpace[ld->GetFaceTVVert(i, j)] = ld->GetGeomVert(ld->GetFaceGeomVert(i, j));//TVMaps.f[i]->t[j]] = TVMaps.geomPoints[TVMaps.f[i]->v[j]];
						if (ld->GetFaceHasVectors(i))//(TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) &&
//							(TVMaps.f[i]->vecs)
						{
							int uvwIndex = ld->GetFaceTVHandle(i, j * 2);
							int geoIndex = ld->GetFaceGeomHandle(i, j * 2);
							xyzSpace[uvwIndex] = ld->GetGeomVert(geoIndex);

							uvwIndex = ld->GetFaceTVHandle(i, j * 2 + 1);
							geoIndex = ld->GetFaceGeomHandle(i, j * 2 + 1);
							xyzSpace[uvwIndex] = ld->GetGeomVert(geoIndex);

							if (ld->GetFaceHasInteriors(i))//TVMaps.f[i]->flags & FLAG_INTERIOR) 
							{
								uvwIndex = ld->GetFaceTVInterior(i, j);
								geoIndex = ld->GetFaceGeomInterior(i, j);
								xyzSpace[uvwIndex] = ld->GetGeomVert(geoIndex);
							}
						}
					}
				}
			}
		}

		BitArray vsel = ld->GetTVVertSel();
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++)
		{
			if ((vsel[i] == 0) && (useableVerts[i]) && (!(ld->GetTVVertWeightModified(i))))//seems we never set MODIFIED?
			{
				float closest_dist = BIGFLOAT;
				Point3 sp(0.0f, 0.0f, 0.0f);
				if (falloffSpace == 0)
				{
					sp = xyzSpace[i];
				}
				else
				{
					sp = ld->GetTVVert(i);
				}

				for (int j = 0; j < selectedVerts.Count(); j++)
				{
					//use XYZ space values
					if (falloffSpace == 0)
					{
						Point3 rp = selectedVerts[j];
						float d = LengthSquared(sp - rp);
						if (d < closest_dist) closest_dist = d;

					}
					else//use UVW space values
					{
						Point3 rp = selectedVerts[j];
						float d = LengthSquared(sp - rp);
						if (d < closest_dist) closest_dist = d;
					}
				}
				if (closest_dist < sstr)
				{
					closest_dist = (float)sqrt(closest_dist);
					float influ = 1.0f - closest_dist / str;
					ComputeFalloff(influ, falloff);
					ld->SetTVVertInfluence(i, influ);
				}
				else
					ld->SetTVVertInfluence(i, 0.0f);
			}
		}
	}
}

void UnwrapMod::ExpandSelection(MeshTopoData *ld, int dir)
{
	BitArray faceHasSelectedVert;

	BitArray vsel = ld->GetTVVertSel();

	faceHasSelectedVert.SetSize(ld->GetNumberFaces());//TVMaps.f.Count());
	faceHasSelectedVert.ClearAll();
	for (int i = 0; i < ld->GetNumberFaces(); i++)
	{
		if (!(ld->GetFaceDead(i)))//TVMaps.f[i]->flags & FLAG_DEAD))
		{
			int pcount = 3;
			pcount = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
			int totalSelected = 0;
			for (int k = 0; k < pcount; k++)
			{
				int index = ld->GetFaceTVVert(i, k);//TVMaps.f[i]->t[k];
				if (vsel[index])
				{
					totalSelected++;
				}
			}

			if ((totalSelected != pcount) && (totalSelected != 0))
			{
				faceHasSelectedVert.Set(i);
			}

		}
	}
	for (int i = 0; i < ld->GetNumberFaces()/*TVMaps.f.Count()*/; i++)
	{
		if (faceHasSelectedVert[i])
		{
			int pcount = 3;
			pcount = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
			for (int k = 0; k < pcount; k++)
			{
				int index = ld->GetFaceTVVert(i, k);//TVMaps.f[i]->t[k];
				if (dir == 0)
					vsel.Set(index, 1);
				else vsel.Set(index, 0);
			}

		}
	}
	ld->SetTVVertSel(vsel);
}

void UnwrapMod::ExpandSelection(int dir, BOOL rebuildCache, BOOL hold)

{
	// LAM - 6/19/04 - defect 576948
	if (hold)
	{
		theHold.Begin();
		HoldSelection();
	}

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ExpandSelection(ld, dir);
	}

	SelectHandles(dir);
	if (rebuildCache)
		RebuildDistCache();
	//convert our sub selection back

	TransferSelectionEnd(FALSE, TRUE);

	if (hold)
		theHold.Accept(GetString(IDS_DS_SELECT));
}

void	UnwrapMod::fnSelectElement()
{
	theHold.Begin();
	HoldSelection();
	SelectElement();
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	RebuildDistCache();
	theHold.Accept(GetString(IDS_DS_SELECT));
}

void UnwrapMod::SelectElement(BOOL addSelection)
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->SelectElement(fnGetTVSubMode(), addSelection);
	}
}


void UnwrapMod::SelectHandles(int dir)

{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->SelectHandles(dir);
	}
}





void UnwrapMod::SelectFacesByNormals(MeshTopoData *md, BitArray &sel, Point3 norm, float angle, Tab<Point3> &normList)
{

	if (normList.Count() == 0)
		md->BuildNormals(normList);
	norm = Normalize(norm);
	angle = angle * PI / 180.0f;
	if (md->GetMesh())
	{
		//check for contigous faces
		double newAngle;
		if (sel.GetSize() != md->GetMesh()->numFaces)
			sel.SetSize(md->GetMesh()->numFaces);
		sel.ClearAll();
		for (int i = 0; i < md->GetMesh()->numFaces; i++)
		{
			Point3 debugNorm = Normalize(normList[i]);
			float dot = DotProd(debugNorm, norm);
			newAngle = (acos(dot));




			if ((dot >= 1.0f) || (newAngle <= angle))
				sel.Set(i);
			else
			{
				sel.Set(i, 0);
			}
		}
	}
	else if (md->GetMNMesh())
	{
		//check for contigous faces
		double newAngle;
		if (sel.GetSize() != md->GetMNMesh()->numf)
			sel.SetSize(md->GetMNMesh()->numf);
		for (int i = 0; i < md->GetMNMesh()->numf; i++)
		{
			Point3 debugNorm = normList[i];
			float dot = DotProd(normList[i], norm);
			newAngle = (acos(dot));
			if ((dot >= 1.0f) || (newAngle <= angle))
				sel.Set(i);
			else
			{
				sel.Set(i, 0);
			}
		}
	}
	else if (md->GetPatch())
	{
		//check for contigous faces
		double newAngle;
		if (sel.GetSize() != md->GetPatch()->numPatches)
			sel.SetSize(md->GetPatch()->numPatches);
		for (int i = 0; i < md->GetPatch()->numPatches; i++)
		{
			Point3 debugNorm = normList[i];
			float dot = DotProd(normList[i], norm);
			newAngle = (acos(dot));
			if ((dot >= 1.0f) || (newAngle <= angle))
				sel.Set(i);
			else
			{
				sel.Set(i, 0);
			}
		}
	}


}


void UnwrapMod::SelectFacesByGroup(
	MeshTopoData *md, 
	BitArray &sel, 
	int seedFaceIndex, 
	Point3 norm, 
	float angle, 
	BOOL relative, 
	MeshTopoData::GroupBy groupBy,
	Tab<Point3> &normList,
	Tab<BorderClass> &borderData,
	AdjEdgeList *medges)
{
    enum FacesAtEdgeType {FIRST_FACE = 0, SECOND_FACE = 1, N_FACES_EDGE = 2};
    const FacesAtEdgeType oppositeFace[N_FACES_EDGE] = {SECOND_FACE, FIRST_FACE};

	//check for type

	if (normList.Count() == 0)
		md->BuildNormals(normList);

	angle = angle * PI / 180.0f;

	sel.ClearAll();

	if (md->GetMesh())
	{
		// Get seed face
        int nFaces = (md->GetMesh())->numFaces;
		if (seedFaceIndex < nFaces)
		{
			if (sel.GetSize() != nFaces)
				sel.SetSize(nFaces);
			//select it
			sel.Set(seedFaceIndex);

			//build egde list of all edges that have only one edge selected
			AdjEdgeList *edges;
			BOOL deleteEdges = FALSE;
			if (medges == NULL)
			{
				edges = new AdjEdgeList(*md->GetMesh());
				deleteEdges = TRUE;
			}
			else edges = medges;

			borderData.ZeroCount();

            float cosAngle = std::cos(angle);
			int numberWorkableEdges = 1;
			while (numberWorkableEdges > 0)
			{
				numberWorkableEdges = 0;
				for (int i = 0; i < edges->edges.Count(); ++i)
				{
					DWORD* edgeFaces = (edges->edges[i]).f;
                    if ((edgeFaces[FIRST_FACE] < (DWORD)nFaces) && (edgeFaces[SECOND_FACE] < (DWORD)nFaces))
                    {

                        // Consider this edge only if it joins a selected face with a presently unselected face
                        size_t indexFace = FIRST_FACE;
                        FacesAtEdgeType selectedFace = N_FACES_EDGE;
                        while ((selectedFace == N_FACES_EDGE) && (indexFace != N_FACES_EDGE))
                        {
                            if (sel[(int)(edgeFaces[indexFace])] && (!(sel[(int)(edgeFaces[oppositeFace[indexFace]])])))
                            {
                                selectedFace = (FacesAtEdgeType)indexFace;
                                FacesAtEdgeType unselectedFace = oppositeFace[selectedFace];

                                // A candidate edge has been encountered; determine whether the presently unselected face
                                // should be added to the selection
                                bool isMatching = false;
                                Face* faces = (md->GetMesh())->faces;
                                switch (groupBy)
                                {
                                case MeshTopoData::kSmoothingGroup:
                                    {
                                        DWORD faceSmoothingGroup[N_FACES_EDGE];

                                        // Match by smoothing group
                                        for (size_t f = 0; f != N_FACES_EDGE; ++f)
                                        {
                                            faceSmoothingGroup[f] = (faces[edgeFaces[f]]).getSmGroup();
                                        }

                                        if (faceSmoothingGroup[FIRST_FACE] & faceSmoothingGroup[SECOND_FACE])
                                        {
                                            isMatching = true;
                                        }
                                    }
                                    break;
								case MeshTopoData::kMaterialID:
                                    {
                                        DWORD faceMaterialID[N_FACES_EDGE];

										// Match by face material
                                        for (size_t f = 0; f != N_FACES_EDGE; ++f)
                                        {
                                            faceMaterialID[f] = (faces[edgeFaces[f]]).getMatID();
                                        }

                                        if (faceMaterialID[FIRST_FACE] == faceMaterialID[SECOND_FACE])
                                        {
                                            isMatching = true;
                                        }
                                    }
									break;
								case MeshTopoData::kFaceAngle:
                                    {

                                        // Match by checking if angle between normal of candidate face and either normal of
                                        // member face (case relative) or cluster normal (case !relative) is not greater than
                                        // input variable angle
									    Point3* normAngle = &norm;
                                        if (relative)
                                        {
                                            normAngle = &(normList[edgeFaces[selectedFace]]);
                                        }

                                        float dotProd = DotProd(normList[edgeFaces[unselectedFace]], *normAngle);
                                        if (dotProd >= cosAngle)
                                        {
                                            isMatching = true;
                                        }
                                    }
									break;
                                default:
                                    break;
                                }

                                // If a match is found between faces at edge, add to selection; otherwise, add this edge to the
                                // border of the selected region
                                if (isMatching)
                                {
                                    sel.Set((int)(edgeFaces[unselectedFace]));
                                    ++numberWorkableEdges;
                                }
                                else
                                {
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = (int)(edgeFaces[selectedFace]);
									tempData.outerFace = (int)(edgeFaces[unselectedFace]);
									borderData.Append(1, &tempData);
                                }
                            }

                            ++indexFace;
                        }
                    }
				}
			}

			if (deleteEdges) delete edges;
		}
	}
	else if (md->GetPatch())
	{
		if (seedFaceIndex < md->GetPatch()->numPatches)
		{
			//select it
			if (sel.GetSize() != md->GetPatch()->numPatches)
				sel.SetSize(md->GetPatch()->numPatches);

			sel.Set(seedFaceIndex);

			borderData.ZeroCount();

			//build egde list of all edges that have only one edge selected
			PatchEdge *edges = md->GetPatch()->edges;

			int numberWorkableEdges = 1;
			while (numberWorkableEdges > 0)
			{
				numberWorkableEdges = 0;
				for (int i = 0; i < md->GetPatch()->numEdges; i++)
				{
					if (edges[i].patches.Count() == 2)
					{
						int a = edges[i].patches[0];
						int b = edges[i].patches[1];
						if ((a >= 0) && (b >= 0))
						{
							int smgrpA = md->GetPatch()->patches[a].smGroup;
							int smgrpB = md->GetPatch()->patches[b].smGroup;

							BOOL isMatching = FALSE;
							if (groupBy == MeshTopoData::kSmoothingGroup)
							{
								if (smgrpA & smgrpB)
									isMatching = TRUE;
							}

							if (groupBy == MeshTopoData::kMaterialID)
							{
								DWORD matIDA = md->GetPatch()->patches[a].getMatID();
								DWORD matIDB = md->GetPatch()->patches[b].getMatID();
								if (matIDB == matIDA)
									isMatching = TRUE;
							}

							if (sel[a] && (!sel[b]))
							{
								if (groupBy == MeshTopoData::kFaceAngle)
								{
									float newAngle = 0.0f;
									float dot = 0.0f;
									if (!relative)
										dot = DotProd(normList[b], norm);
									else
										dot = DotProd(normList[b], normList[a]);

									if (dot < 1.0f)
										newAngle = acos(dot);

									if (newAngle <= angle)
									{
										isMatching = TRUE;
									}
								}
								if (isMatching)
								{
									sel.Set(b);
									numberWorkableEdges++;
								}
								else
								{
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = a;
									tempData.outerFace = b;
									borderData.Append(1, &tempData);
								}
							}
							else if (sel[b] && (!sel[a]))
							{
								if (groupBy == MeshTopoData::kFaceAngle)
								{
									float newAngle = 0.0f;
									float dot = 0.0f;
									if (!relative)
										dot = DotProd(normList[a], norm);
									else
										dot = DotProd(normList[a], normList[b]);

									if (dot < 1.0f)
										newAngle = acos(dot);

									if (newAngle <= angle)
									{
										isMatching = TRUE;
									}
								}
								if (isMatching)
								{
									sel.Set(a);
									numberWorkableEdges++;
								}
								else
								{
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = b;
									tempData.outerFace = a;
									borderData.Append(1, &tempData);
								}
							}
						}
					}
				}
			}
		}
	}
	else if (md->GetMNMesh())
	{
		//select it
		if (seedFaceIndex < md->GetMNMesh()->numf)
		{
			if (sel.GetSize() != md->GetMNMesh()->numf)
				sel.SetSize(md->GetMNMesh()->numf);
			sel.Set(seedFaceIndex);

			borderData.ZeroCount();

			//build egde list of all edges that have only one edge selected
			MNEdge *edges = md->GetMNMesh()->E(0);
			int numberWorkableEdges = 1;
			while (numberWorkableEdges > 0)
			{
				numberWorkableEdges = 0;
				for (int i = 0; i < md->GetMNMesh()->nume; i++)
				{
					int a = edges[i].f1;
					int b = edges[i].f2;

					if ((a >= 0) && (b >= 0))
					{
						int smgrpA = md->GetMNMesh()->f[a].smGroup;
						int smgrpB = md->GetMNMesh()->f[b].smGroup;

						BOOL isMatching = FALSE;
						if (groupBy == MeshTopoData::kSmoothingGroup)
						{
							if (smgrpA & smgrpB)
								isMatching = TRUE;
						}

						if (groupBy == MeshTopoData::kMaterialID)
						{
							DWORD matIDA = md->GetMNMesh()->f[a].material;
							DWORD matIDB = md->GetMNMesh()->f[b].material;
							if (matIDB == matIDA)
								isMatching = TRUE;
						}

						if (sel[a] && (!sel[b]))
						{
							if (groupBy == MeshTopoData::kFaceAngle)
							{
								float newAngle = 0.0f;
								if (!relative)
								{
									float dot = DotProd(normList[b], norm);
									if (dot < -0.99999f)
										newAngle = PI;
									else if (dot > 0.99999f)
										newAngle = 0.0f;
									else
										newAngle = (acos(dot));
								}
								else
								{
									float dot = DotProd(normList[b], normList[a]);
									if (dot < -0.99999f)
										newAngle = PI;
									else if (dot > 0.99999f)
										newAngle = 0.0f;
									else
										newAngle = (acos(dot));
								}
								if (newAngle <= angle)
								{
									isMatching = TRUE;
								}
							}

							if (isMatching)
							{
								sel.Set(b);
								numberWorkableEdges++;
							}
							else
							{
								BorderClass tempData;
								tempData.edge = i;
								tempData.innerFace = a;
								tempData.outerFace = b;
								borderData.Append(1, &tempData);
							}
						}
						else if (sel[b] && (!sel[a]))
						{
							if (groupBy == MeshTopoData::kFaceAngle)
							{
								float newAngle = 0.0f;
								if (!relative)
								{
									float dot = DotProd(normList[a], norm);
									if (dot < -0.99999f)
										newAngle = PI;
									else if (dot > 0.99999f)
										newAngle = 0.0f;
									else
										newAngle = (acos(dot));
								}
								else
								{
									float dot = DotProd(normList[a], normList[b]);
									if (dot < -0.99999f)
										newAngle = PI;
									else if (dot > 0.99999f)
										newAngle = 0.0f;
									else
										newAngle = (acos(dot));
								}
								if (newAngle <= angle)
								{
									isMatching = TRUE;
								}
							}

							if (isMatching)
							{
								sel.Set(a);
								numberWorkableEdges++;
							}
							else
							{
								BorderClass tempData;
								tempData.edge = i;
								tempData.innerFace = b;
								tempData.outerFace = a;
								borderData.Append(1, &tempData);
							}
						}
					}
				}
			}
		}
	}
}

void UnwrapMod::fnSelectPolygonsUpdate(BitArray *sel, BOOL update, INode *node)
{
	SelectFaces(sel, node, update ? true : false);
}

void UnwrapMod::fnSelectPolygonsUpdate(BitArray *sel, BOOL update)
{
	SelectFaces(sel, nullptr, update ? true : false);
}

void	UnwrapMod::fnSelectFacesByNormal(Point3 normal, float angleThreshold, BOOL update)
{
	if (!ip) return;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *md = mMeshTopoData[ldID];

		if (md)
		{
			if (theHold.Holding())
				HoldSelection();

			BitArray sel;
			Tab<Point3> normList;
			normList.ZeroCount();
			SelectFacesByNormals(md, sel, normal, angleThreshold, normList);
			
			md->SetFaceSelectionByRef(sel, fnGetMirrorSelectionStatus());

			if (fnGetSyncSelectionMode())
				fnSyncGeomSelection();

			if (update)
				InvalidateView();
		}
	}
}

void	UnwrapMod::fnSelectClusterByNormal(float angleThreshold, int seedIndex, BOOL relative, BOOL update)
{
	//check for type
	if (!ip) return;
	seedIndex--;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *md = mMeshTopoData[ldID];//(MeshTopoData*)mcList[0]->localData;

		if (md)
		{
			BitArray sel;

			Tab<BorderClass> dummy;
			Tab<Point3> normList;
			normList.ZeroCount();
			md->BuildNormals(normList);
			if ((seedIndex >= 0) && (seedIndex < normList.Count()))
			{
				Point3 normal = normList[seedIndex];
				SelectFacesByGroup(md, sel, seedIndex, normal, angleThreshold, relative, MeshTopoData::kFaceAngle, normList, dummy);
				
				md->SetFaceSelectionByRef(sel, fnGetMirrorSelectionStatus());

				if (fnGetSyncSelectionMode())
					fnSyncGeomSelection();

				if (update)
					InvalidateView();
			}
		}
	}
}



BOOL	UnwrapMod::fnGetLimitSoftSel()
{
	return limitSoftSel;
}

void	UnwrapMod::fnSetLimitSoftSel(BOOL limit)
{
	if (limit != limitSoftSel)
	{
		limitSoftSel = limit;
		RebuildDistCache();
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		InvalidateView();
		mUIManager.UpdateCheckButtons();
	}
}

int		UnwrapMod::fnGetLimitSoftSelRange()
{
	return limitSoftSelRange;
}

void	UnwrapMod::fnSetLimitSoftSelRange(int range)
{
	if (range != limitSoftSelRange)
	{
		limitSoftSelRange = range;
		RebuildDistCache();
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		InvalidateView();
		mUIManager.SetSpinFValue(ID_SOFTSELECTIONLIMIT_SPINNER, limitSoftSelRange);
	}
}


float	UnwrapMod::fnGetVertexWeight(int index, INode *node)
{
	float v = 0.0f;
	index--;
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		v = ld->GetTVVertInfluence(index);
	}

	return v;
}

float	UnwrapMod::fnGetVertexWeight(int index)
{
	float v = 0.0f;
	index--;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			v = ld->GetTVVertInfluence(index);
		}
	}
	return v;
}

void	UnwrapMod::fnSetVertexWeight(int index, float weight, INode *node)
{
	index--;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = GetMeshTopoData(node);
		if (ld)
		{
			ld->SetTVVertInfluence(index, weight);//	TVMaps.v[index].influence = weight;
			int flag = ld->GetTVVertFlag(index);
			ld->SetTVVertFlag(index, flag | FLAG_WEIGHTMODIFIED);//TVMaps.v[index].flags  |= FLAG_WEIGHTMODIFIED;
			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			InvalidateView();
		}
	}
}


void	UnwrapMod::fnSetVertexWeight(int index, float weight)
{
	index--;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			ld->SetTVVertInfluence(index, weight);//	TVMaps.v[index].influence = weight;
			int flag = ld->GetTVVertFlag(index);
			ld->SetTVVertFlag(index, flag | FLAG_WEIGHTMODIFIED);//TVMaps.v[index].flags  |= FLAG_WEIGHTMODIFIED;
			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			InvalidateView();
		}
	}
}


BOOL	UnwrapMod::fnIsWeightModified(int index, INode *node)
{
	index--;
	BOOL mod = FALSE;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = GetMeshTopoData(node);
		if (ld)
		{
			int flag = ld->GetTVVertFlag(index);
			mod = (flag & FLAG_WEIGHTMODIFIED);
		}
	}
	return mod;
}

BOOL	UnwrapMod::fnIsWeightModified(int index)
{
	index--;
	BOOL mod = FALSE;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			int flag = ld->GetTVVertFlag(index);
			mod = (flag & FLAG_WEIGHTMODIFIED);
		}
	}
	return mod;

}

void	UnwrapMod::fnModifyWeight(int index, BOOL modified, INode *node)
{
	index--;

	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		int flag = ld->GetTVVertFlag(index);
		if (modified)
			ld->SetTVVertFlag(index, flag | FLAG_WEIGHTMODIFIED);//TVMaps.v[index].flags |= FLAG_WEIGHTMODIFIED;
		else
		{
			ld->SetTVVertFlag(index, flag & ~FLAG_WEIGHTMODIFIED);//TVMaps.v[index].flags &= ~FLAG_WEIGHTMODIFIED;
		}
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		InvalidateView();
	}
}

void	UnwrapMod::fnModifyWeight(int index, BOOL modified)
{
	index--;

	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			int flag = ld->GetTVVertFlag(index);
			if (modified)
				ld->SetTVVertFlag(index, flag | FLAG_WEIGHTMODIFIED);//TVMaps.v[index].flags |= FLAG_WEIGHTMODIFIED;
			else
			{
				ld->SetTVVertFlag(index, flag & ~FLAG_WEIGHTMODIFIED);//TVMaps.v[index].flags &= ~FLAG_WEIGHTMODIFIED;
			}
			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			InvalidateView();
		}
	}
}

BOOL	UnwrapMod::fnGetGeomElemMode()
{
	return geomElemMode;
}

void	UnwrapMod::fnSetGeomElemMode(BOOL elem)
{
	geomElemMode = elem;
	mUIManager.UpdateCheckButtons();
}



void	UnwrapMod::SelectGeomFacesByAngle(MeshTopoData* tmd)
{
	Tab<BorderClass> dummy;
	Tab<Point3> normList;
	normList.ZeroCount();
	tmd->BuildNormals(normList);
	BitArray tempSel;
	tempSel.SetSize(tmd->GetNumberFaces());//faceSel.GetSize());

	tempSel = tmd->GetFaceSel();

	for (int i = 0; i < tmd->GetNumberFaces(); i++)
	{
		if ((tempSel[i]) && (i >= 0) && (i < normList.Count()))
		{
			BitArray sel;
			Point3 normal = normList[i];
			SelectFacesByGroup(tmd, sel, i, normal, planarThreshold, FALSE, MeshTopoData::kFaceAngle, normList, dummy);
			BitArray faceSel = tmd->GetFaceSel();
			faceSel |= sel;
			tmd->SetFaceSelectionByRef(faceSel);
			for (int j = 0; j < tempSel.GetSize(); j++)
			{
				if (sel[j]) tempSel.Set(j, FALSE);
			}
		}
	}
}

BOOL	UnwrapMod::fnGetGeomPlanarMode()
{
	return planarMode;
}

void	UnwrapMod::fnSetGeomPlanarMode(BOOL planar)
{
	planarMode = planar;
	mUIManager.UpdateCheckButtons();
}

float	UnwrapMod::fnGetGeomPlanarModeThreshold()
{
	return planarThreshold;
}

void	UnwrapMod::fnSetGeomPlanarModeThreshold(float threshold)
{
	planarThreshold = threshold;
	mUIManager.SetSpinFValue(ID_PLANARSPIN, planarThreshold);
}

void UnwrapMod::fnSetSelectionMatID(int matID)
{
	if (!ip) return;

	int curSelMatID = GetCurrentMatID();

	matID--;
	if (curSelMatID == matID)
	{
		return;
	}

	theHold.Begin();
	HoldPointsAndFaces();
	theHold.Accept(GetString(IDS_RB_SETMATID));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->SetSelectionMatID(matID);
	}
	UpdateMatFilters();
	UpdateMatIDUI();
}

void	UnwrapMod::fnSelectByMatID(int matID)
{
	if (!ip) return;

	matID--;

	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{

		mMeshTopoData[ldID]->SelectByMatID(matID);
	}

	theHold.Suspend();
	if (fnGetSyncSelectionMode()) fnSyncTVSelection();
	theHold.Resume();

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	InvalidateView();
	ip->RedrawViews(ip->GetTime());

	UpdateMatIDUI();
}

void	UnwrapMod::fnSelectBySG(int sg)
{
	if (!ip) return;
	sg--;

	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *tmd = mMeshTopoData[ldID];

		if (tmd == NULL)
		{
			continue;
		}


		BitArray faceSel = tmd->GetFaceSel();
		faceSel.ClearAll();

		sg = 1 << sg;
		if (tmd->GetMesh())
		{
			for (int i = 0; i < tmd->GetMesh()->numFaces; i++)
			{
				if (tmd->GetMesh()->faces[i].getSmGroup() & sg)
					faceSel.Set(i);
			}
		}
		else if (tmd->GetMNMesh())
		{
			for (int i = 0; i < tmd->GetMNMesh()->numf; i++)
			{
				if (tmd->GetMNMesh()->f[i].smGroup & sg)
					faceSel.Set(i);
			}
		}
		else if (tmd->GetPatch())
		{
			for (int i = 0; i < tmd->GetPatch()->numPatches; i++)
			{
				if (tmd->GetPatch()->patches[i].smGroup & sg)
					faceSel.Set(i);
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);
	}

	if (fnGetSyncSelectionMode())
		fnSyncTVSelection();

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	InvalidateView();
	ip->RedrawViews(ip->GetTime());


}



void	UnwrapMod::GeomExpandFaceSel(MeshTopoData* tmd)
{
	BitArray selectedVerts;

	if (tmd->GetMesh())
	{
		Mesh *mesh = tmd->GetMesh();
		BitArray faceSel = tmd->GetFaceSel();

		selectedVerts.SetSize(mesh->getNumVerts());
		selectedVerts.ClearAll();
		for (int i = 0; i < mesh->getNumFaces(); i++)
		{
			if (faceSel[i])
			{
				for (int j = 0; j < 3; j++)
				{
					int index = mesh->faces[i].v[j];
					selectedVerts.Set(index);
				}
			}
		}
		for (int i = 0; i < mesh->getNumFaces(); i++)
		{
			if (!faceSel[i])
			{
				for (int j = 0; j < 3; j++)
				{
					int index = mesh->faces[i].v[j];
					if (selectedVerts[index])
						faceSel.Set(i);

				}
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);

	}
	else if (tmd->GetPatch())
	{
		PatchMesh *patch = tmd->GetPatch();
		BitArray faceSel = tmd->GetFaceSel();

		selectedVerts.SetSize(patch->getNumVerts());
		selectedVerts.ClearAll();
		for (int i = 0; i < patch->getNumPatches(); i++)
		{
			if (faceSel[i])
			{
				int ct = 4;
				if (patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0; j < ct; j++)
				{
					int index = patch->patches[i].v[j];
					selectedVerts.Set(index);
				}
			}
		}
		for (int i = 0; i < patch->getNumPatches(); i++)
		{
			if (!faceSel[i])
			{
				int ct = 4;
				if (patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0; j < ct; j++)
				{
					int index = patch->patches[i].v[j];
					if (selectedVerts[index])
						faceSel.Set(i);

				}
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);
	}
	else if (tmd->GetMNMesh())
	{
		MNMesh *mnMesh = tmd->GetMNMesh();
		BitArray faceSel = tmd->GetFaceSel();

		selectedVerts.SetSize(mnMesh->numv);
		selectedVerts.ClearAll();
		for (int i = 0; i < mnMesh->numf; i++)
		{
			if (faceSel[i])
			{
				int ct = mnMesh->f[i].deg;
				for (int j = 0; j < ct; j++)
				{
					int index = mnMesh->f[i].vtx[j];
					selectedVerts.Set(index);
				}
			}
		}
		for (int i = 0; i < mnMesh->numf; i++)
		{
			if (!faceSel[i])
			{
				int ct = mnMesh->f[i].deg;
				for (int j = 0; j < ct; j++)
				{
					int index = mnMesh->f[i].vtx[j];
					if (selectedVerts[index])
						faceSel.Set(i);

				}
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);
	}

}
void	UnwrapMod::GeomContractFaceSel(MeshTopoData* tmd)
{

	BitArray unselectedVerts;

	if (tmd->GetMesh())
	{
		Mesh *mesh = tmd->GetMesh();
		BitArray faceSel = tmd->GetFaceSel();
		unselectedVerts.SetSize(mesh->getNumVerts());
		unselectedVerts.ClearAll();

		for (int i = 0; i < mesh->getNumFaces(); i++)
		{
			if (!faceSel[i])
			{
				for (int j = 0; j < 3; j++)
				{
					int index = mesh->faces[i].v[j];
					unselectedVerts.Set(index);
				}
			}
		}
		for (int i = 0; i < mesh->getNumFaces(); i++)
		{
			if (faceSel[i])
			{
				for (int j = 0; j < 3; j++)
				{
					int index = mesh->faces[i].v[j];
					if (unselectedVerts[index])
						faceSel.Set(i, FALSE);

				}
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);

	}
	else if (tmd->GetPatch())
	{
		PatchMesh *patch = tmd->GetPatch();
		BitArray faceSel = tmd->GetFaceSel();

		unselectedVerts.SetSize(patch->getNumVerts());
		unselectedVerts.ClearAll();
		for (int i = 0; i < patch->getNumPatches(); i++)
		{
			if (!faceSel[i])
			{
				int ct = 4;
				if (patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0; j < ct; j++)
				{
					int index = patch->patches[i].v[j];
					unselectedVerts.Set(index);
				}
			}
		}
		for (int i = 0; i < patch->getNumPatches(); i++)
		{
			if (faceSel[i])
			{
				int ct = 4;
				if (patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0; j < ct; j++)
				{
					int index = patch->patches[i].v[j];
					if (unselectedVerts[index])
						faceSel.Set(i, FALSE);

				}
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);
	}
	else if (tmd->GetMNMesh())
	{
		MNMesh *mnMesh = tmd->GetMNMesh();
		BitArray faceSel = tmd->GetFaceSel();

		unselectedVerts.SetSize(mnMesh->numv);
		unselectedVerts.ClearAll();
		for (int i = 0; i < mnMesh->numf; i++)
		{
			if (!faceSel[i])
			{
				int ct = mnMesh->f[i].deg;
				for (int j = 0; j < ct; j++)
				{
					int index = mnMesh->f[i].vtx[j];
					unselectedVerts.Set(index);
				}
			}
		}
		for (int i = 0; i < mnMesh->numf; i++)
		{
			if (faceSel[i])
			{
				int ct = mnMesh->f[i].deg;
				for (int j = 0; j < ct; j++)
				{
					int index = mnMesh->f[i].vtx[j];
					if (unselectedVerts[index])
						faceSel.Set(i, FALSE);

				}
			}
		}
		tmd->SetFaceSelectionByRef(faceSel);
	}
}
void	UnwrapMod::fnGeomExpandFaceSel()
{
	//check for type
	ModContextList mcList;
	INodeTab nodes;

	if (!ip) return;
	/*	ip->GetModContexts(mcList,nodes);

		int objects = mcList.Count();

		if (objects != 0)
		{
	*/
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *tmd = mMeshTopoData[ldID];//(MeshTopoData*)mcList[0]->localData;

		if (tmd == NULL)
		{
			continue;
		}
		GeomExpandFaceSel(tmd);

	}
	if (fnGetSyncSelectionMode())
		fnSyncTVSelection();

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	InvalidateView();
	ip->RedrawViews(ip->GetTime());

}
void	UnwrapMod::fnGeomContractFaceSel()
{
	if (!ip) return;
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *tmd = mMeshTopoData[ldID];//(MeshTopoData*)mcList[0]->localData;

		if (tmd == NULL)
		{
			continue;
		}
		GeomContractFaceSel(tmd);

	}
	if (fnGetSyncSelectionMode())
		fnSyncTVSelection();

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	InvalidateView();
	ip->RedrawViews(ip->GetTime());
}



BitArray* UnwrapMod::fnGetSelectedVerts(INode *node)
{
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetTVVertSelectionPtr();
	}
	return NULL;

}

BitArray* UnwrapMod::fnGetSelectedVerts()
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetTVVertSelectionPtr();
		}
	}
	return NULL;

}


void UnwrapMod::fnSelectVerts(BitArray *sel, INode *node)
{

	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		theHold.Begin();
		if (theHold.Holding())
			theHold.Put(new TSelRestore(ld));
		theHold.Accept(GetString(IDS_PW_SELECT_UVW));

		BitArray vsel = ld->GetTVVertSel();
		vsel.ClearAll();
		for (int i = 0; i < vsel.GetSize(); i++)
		{
			if (i < sel->GetSize())
			{
				if ((*sel)[i])
					vsel.Set(i);
			}
		}
		ld->SetTVVertSel(vsel);

		if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

		InvalidateView();
	}

}

void UnwrapMod::fnSelectVerts(BitArray *sel)
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			theHold.Begin();
			if (theHold.Holding())
				theHold.Put(new TSelRestore(ld));
			theHold.Accept(GetString(IDS_PW_SELECT_UVW));

			BitArray vsel = ld->GetTVVertSel();
			vsel.ClearAll();
			for (int i = 0; i < vsel.GetSize(); i++)
			{
				if (i < sel->GetSize())
				{
					if ((*sel)[i])
						vsel.Set(i);
				}
			}
			ld->SetTVVertSel(vsel);

			if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

			InvalidateView();
		}
	}
}

BOOL UnwrapMod::fnIsVertexSelected(int index, INode *node)
{

	index--;

	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetTVVertSelected(index);
	}
	return FALSE;
}

BOOL UnwrapMod::fnIsVertexSelected(int index)
{

	index--;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetTVVertSelected(index);
		}
	}
	return FALSE;
}

BitArray* UnwrapMod::fnGetSelectedFaces(INode *node)
{
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetFaceSelectionPtr();
	}
	return NULL;
}

BitArray* UnwrapMod::fnGetSelectedFaces()
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetFaceSelectionPtr();
		}
	}
	return NULL;
}

void UnwrapMod::SelectFaces(BitArray* sel, INode* node, bool bUpdate)
{
	MeshTopoData *ld = nullptr;
	if (node == nullptr && mMeshTopoData.Count())
	{
		ld = mMeshTopoData[0];
	}
	else
	{
		ld = GetMeshTopoData(node);
	}

	if (ld)
	{
		if (theHold.Holding())
			HoldSelection();

		BitArray fsel = ld->GetFaceSel();
		fsel.ClearAll();
		for (int i = 0; i < fsel.GetSize(); i++)
		{
			if (i < sel->GetSize())
			{
				if ((*sel)[i]) fsel.Set(i);
			}
		}
		ld->SetFaceSelectionByRef(fsel, fnGetMirrorSelectionStatus());

		if (fnGetSyncSelectionMode()) 
			fnSyncGeomSelection();

		if (bUpdate)
			InvalidateView();
	}
}

void	UnwrapMod::fnSelectFaces(BitArray *sel, INode *node)
{
	SelectFaces(sel, node, true);
}

void	UnwrapMod::fnSelectFaces(BitArray *sel)
{
	SelectFaces(sel, nullptr, true);
}

BOOL	UnwrapMod::fnIsFaceSelected(int index, INode *node)
{
	index--;

	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetFaceSelected(index);
	}

	return FALSE;
}

BOOL	UnwrapMod::fnIsFaceSelected(int index)
{

	index--;

	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetFaceSelected(index);
		}
	}


	return FALSE;
}

void	UnwrapMod::TransferSelectionStart()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			ld->TransferSelectionStart(fnGetTVSubMode());
		}
	}
}

//this transfer our vertex selection into our curren selection
void	UnwrapMod::TransferSelectionEnd(BOOL partial, BOOL recomputeSelection)
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			ld->TransferSelectionEnd(fnGetTVSubMode(), partial, recomputeSelection);
		}
	}
}

BitArray* UnwrapMod::fnGetSelectedEdges(INode *node)
{
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetTVEdgeSelectionPtr();
	}

	return NULL;
}

BitArray* UnwrapMod::fnGetSelectedEdges()
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetTVEdgeSelectionPtr();
		}
	}

	return NULL;
}

void	UnwrapMod::fnSelectEdges(BitArray *sel, INode *node)
{
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		theHold.Begin();
		if (theHold.Holding())
			theHold.Put(new TSelRestore(ld));
		theHold.Accept(GetString(IDS_PW_SELECT_UVW));

		BitArray esel = ld->GetTVEdgeSel();
		esel.ClearAll();
		for (int i = 0; i < esel.GetSize(); i++)
		{
			if (i < sel->GetSize())
			{
				if ((*sel)[i]) esel.Set(i);
			}
		}
		ld->SetTVEdgeSel(esel);
		if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
		InvalidateView();

	}

}

void	UnwrapMod::fnSelectEdges(BitArray *sel)
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			theHold.Begin();
			if (theHold.Holding())
				theHold.Put(new TSelRestore(ld));
			theHold.Accept(GetString(IDS_PW_SELECT_UVW));

			BitArray esel = ld->GetTVEdgeSel();
			esel.ClearAll();
			for (int i = 0; i < esel.GetSize(); i++)
			{
				if (i < sel->GetSize())
				{
					if ((*sel)[i]) esel.Set(i);
				}
			}
			ld->SetTVEdgeSel(esel);
			if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
			InvalidateView();

		}
	}

}

BOOL	UnwrapMod::fnIsEdgeSelected(int index, INode *node)
{

	index--;
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetTVEdgeSelected(index);
	}
	return FALSE;
}

BOOL	UnwrapMod::fnIsEdgeSelected(int index)
{

	index--;
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetTVEdgeSelected(index);
		}
	}
	return FALSE;
}

BOOL	UnwrapMod::fnGetUVEdgeMode()
{
	return uvEdgeMode;
}
void	UnwrapMod::fnSetUVEdgeMode(BOOL uvmode)
{
	if (uvmode)
	{
		tvElementMode = FALSE;
		openEdgeMode = FALSE;
	}
	uvEdgeMode = uvmode;
}

BOOL	UnwrapMod::fnGetTVElementMode()
{
	return tvElementMode;
}
void	UnwrapMod::fnSetTVElementMode(BOOL mode)
{
	if (mode)
	{
		uvEdgeMode = FALSE;
		openEdgeMode = FALSE;
	}
	tvElementMode = mode;
	if ((ip) && (hDialogWnd))
	{
		IMenuBarContext* pContext = (IMenuBarContext*)GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
		if (pContext)
			pContext->UpdateWindowsMenu();
	}
}

void	UnwrapMod::SelectUVEdge(BOOL selectOpenEdges)
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			ld->SelectUVEdgeLoop(selectOpenEdges);
		}
	}
}


void	UnwrapMod::SelectOpenEdge()
{

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		int eselSet = 0;

		while (eselSet != ld->GetTVEdgeSel().NumberSet())
		{
			eselSet = ld->GetTVEdgeSel().NumberSet();//esel.NumberSet();
			GrowSelectOpenEdge();
			//get connecting a edge
		}
	}
}


void	UnwrapMod::GrowSelectOpenEdge()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray esel = ld->GetTVEdgeSel();
		if(esel.IsEmpty()) continue;

		int edgeCount = esel.GetSize();

		Tab<int> edgeCounts;
		edgeCounts.SetCount(ld->GetNumberTVVerts());//TVMaps.v.Count());
		for (int i = 0; i < ld->GetNumberTVVerts()/*TVMaps.v.Count()*/; i++)
			edgeCounts[i] = 0;
			for (int i = 0; i < edgeCount; i++)
			{
				int a = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
				int b = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
				if (esel[i])
				{
					edgeCounts[a]++;
					edgeCounts[b]++;
				}
			}
		for (int i = 0; i < edgeCount; i++)
		{
			if ((!esel[i]) && (ld->GetTVEdgeNumberTVFaces(i)/*TVMaps.ePtrList[i]->faceList.Count()*/ == 1))
			{
				int a = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
				int b = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
				int aCount = edgeCounts[a];
				int bCount = edgeCounts[b];
				if (((aCount == 0) && (bCount >= 1)) ||
					((bCount == 0) && (aCount >= 1)) ||
					((bCount >= 1) && (aCount >= 1)))
				{
					esel.Set(i, TRUE);
				}
			}
		}
		ld->SetTVEdgeSel(esel);
	}

}

void	UnwrapMod::ShrinkSelectOpenEdge()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray esel = ld->GetTVEdgeSel();
		if (esel.IsEmpty()) continue;

		int edgeCount = esel.GetSize();

		Tab<int> edgeCounts;
		edgeCounts.SetCount(ld->GetNumberTVVerts());//TVMaps.v.Count());
		for (int i = 0; i < ld->GetNumberTVVerts()/*TVMaps.v.Count()*/; i++)
			edgeCounts[i] = 0;
			for (int i = 0; i < edgeCount; i++)
			{
				int a = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
				int b = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
				if (esel[i])
				{
					edgeCounts[a]++;
					edgeCounts[b]++;
				}
			}
		for (int i = 0; i < edgeCount; i++)
		{
			if ((esel[i]) && (ld->GetTVEdgeNumberTVFaces(i)/*TVMaps.ePtrList[i]->faceList.Count()*/ == 1))
			{
				int a = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
				int b = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
				if ((edgeCounts[a] == 1) || (edgeCounts[b] == 1))
					esel.Set(i, FALSE);
			}
		}
		ld->SetTVEdgeSel(esel);
	}
}

void	UnwrapMod::GrowUVLoop(BOOL selectOpenEdges)
{
 	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if(ld)
			ld->GrowUVLoop(selectOpenEdges);
	}
}

void	UnwrapMod::ShrinkUVLoop()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray esel = ld->GetTVEdgeSel();
		BitArray vsel = ld->GetTVVertSel();
		if (esel.IsEmpty()) continue;


		int edgeCount = esel.GetSize();

		Tab<int> edgeCounts;
		edgeCounts.SetCount(ld->GetNumberTVVerts());
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			edgeCounts[i] = 0;
		for (int i = 0; i < edgeCount; i++)
		{
			int a = ld->GetTVEdgeVert(i, 0);
			int b = ld->GetTVEdgeVert(i, 1);
			if (esel[i])
			{
				edgeCounts[a]++;
				edgeCounts[b]++;
			}
		}
		for (int i = 0; i < edgeCount; i++)
		{
			if (esel[i])
			{
				int a = ld->GetTVEdgeVert(i, 0);
				int b = ld->GetTVEdgeVert(i, 1);
				if ((edgeCounts[a] == 1) || (edgeCounts[b] == 1))
					esel.Set(i, FALSE);
			}
		}
		ld->SetTVEdgeSel(esel);
	}
}

void	UnwrapMod::GrowUVRing(bool doall)
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		BitArray tempGeSel;
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray gesel = ld->GetTVEdgeSel();
		if (gesel.IsEmpty()) continue;
		tempGeSel = gesel;
		tempGeSel.ClearAll();

		BitArray edgeRing;
		//get the selected edge
		for (int i = 0; i < ld->GetNumberTVEdges(); i++)
		{
			if (gesel[i])
			{
				ld->GetUVRingEdges(i, edgeRing, doall);
				tempGeSel |= edgeRing;
			}
		}
		gesel = gesel | tempGeSel;
		ld->SetTVEdgeSel(gesel);
	}
}
void	UnwrapMod::ShrinkUVRing()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray esel = ld->GetTVEdgeSel();
		if(esel.IsEmpty()) continue;
		BitArray vsel;
		vsel.SetSize(ld->GetNumberTVVerts());
		vsel.ClearAll();

		Tab<int> connectionCount;
		connectionCount.SetCount(ld->GetNumberTVVerts());
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			connectionCount[i] = 0;


		for (int i = 0; i < ld->GetNumberTVEdges(); i++)
		{
			if (!ld->GetTVEdgeHidden(i) && esel[i])
			{
				for (int j = 0; j < 2; j++)
				{
					int tid = ld->GetTVEdgeVert(i, j);
					vsel.Set(tid, TRUE);
				}
			}
		}

		for (int i = 0; i < ld->GetNumberTVEdges(); i++)
		{
			if (!ld->GetTVEdgeHidden(i) && !esel[i])
			{
				int tid0 = ld->GetTVEdgeVert(i, 0);
				int tid1 = ld->GetTVEdgeVert(i, 1);
				if (vsel[tid0] && vsel[tid1])
				{
					connectionCount[tid0] += 1;
					connectionCount[tid1] += 1;
				}
			}
		}

		for (int i = 0; i < ld->GetNumberTVEdges(); i++)
		{
			if (!ld->GetTVEdgeHidden(i) && esel[i])
			{
				int tid0 = ld->GetTVEdgeVert(i, 0);
				int tid1 = ld->GetTVEdgeVert(i, 1);
				if (connectionCount[tid0] < 2 ||
					connectionCount[tid1] < 2)
				{
					esel.Set(i, FALSE);
				}
			}
		}

		ld->SetTVEdgeSel(esel);
	}
}

void UnwrapMod::UVEdgeRing(Tab<TVHitData> &hits)
{
	if (fnGetTVSubMode() != TVEDGEMODE)
		return;

	// hold selection for later undo redo.
	if (!fnGetPaintMode())
		HoldSelection();

	for (int i = 0; i < hits.Count(); ++i)
	{
		int edgeIndex = hits[i].mID;
		int ldID = hits[i].mLocalDataID;

		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld)
		{
			if (ld->GetTVEdgeSelected(edgeIndex))
			{
				continue;
			}

			BitArray oldTVEdgeSelection = ld->GetTVEdgeSel();

			BitArray newEdgeSel;
			ld->GetUVRingEdges(edgeIndex, newEdgeSel, true);
			BitArray temp = oldTVEdgeSelection & newEdgeSel;
			if (!temp.IsEmpty())
			{
				newEdgeSel |= oldTVEdgeSelection;
				ld->SetTVEdgeSel(newEdgeSel);
				SyncSelect();
				return;
			}
		}
	}
	return;
}

void UnwrapMod::UVFaceLoop(Tab<TVHitData> &hits)
{
	if (fnGetTVSubMode() != TVFACEMODE)
		return;

	// hold selection for later undo redo.
	if (!fnGetPaintMode())
		HoldSelection();

	for (int i = 0; i < hits.Count(); ++i)
	{
		int fID = hits[i].mID;
		int ldID = hits[i].mLocalDataID;

		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld && ld->GetMNMesh()) // only support editable poly now
		{
			if (ld->GetFaceSelected(fID))
			{
				continue;
			}

			BitArray loopFSel = ld->GetFaceSel();
			loopFSel.ClearAll();

			// for each edge of this face
			int faceDegree = ld->GetFaceDegree(fID);
			int theResult = -1;
			int closestDist = ld->GetNumberTVEdges();
			for (int k = 0; k < faceDegree; k++)
			{
				int a = ld->GetFaceTVVert(fID, k);
				int b = ld->GetFaceTVVert(fID, (k + 1) % faceDegree);
				if (a == b)
				{
					continue;
				}
				// for each edge on the input face, get its ring edge vector	
				int eindex = ld->FindUVEdge(a, b);
				BitArray ringEdges;
				std::vector<int> ringVec;
				ringVec.clear();
				ld->GetUVRingEdges(eindex, ringEdges, true, &ringVec);
				int ringCount = (int)ringVec.size();
				if (ringCount <= 1)
				{
					continue;
				}
				bool found = false;
				bool bStop = false;
				// loop thru the ring array to find the edge which is closet to the input edge and on a select face!
				// record the closest distance and index of the closest edge
				int faceIndex = -1, selIndex = -1, firstDist = -1;
				for (int d = 0; d < ringCount && !bStop; ++d)
				{
					int ed = ringVec[d];
					int fCount = ld->GetTVEdgeNumberTVFaces(ed);
					for (int f = 0; (f < fCount) && !bStop; ++f)
					{
						int fIndex = ld->GetTVEdgeConnectedTVFace(ed, f);
						if (fIndex == fID)
						{
							faceIndex = d;
							if (selIndex != -1)
								firstDist = faceIndex - selIndex;
						}
						if (ld->GetFaceSelected(fIndex))
						{
							found = true;
							if (faceIndex == -1) selIndex = d;
							else
							{
								if (firstDist == -1) selIndex = d;
								else
								{
									if (d - faceIndex < firstDist) selIndex = d;
								}
								bStop = true;
							}
						}
					}
				}

				if (found)
				{
					int dist = faceIndex > selIndex ? faceIndex - selIndex : selIndex - faceIndex;
					if (dist < closestDist)
					{
						theResult = eindex;
						closestDist = dist;
					}
				}
			}

			// if ever found a valid ring, then select the poly faces on this ring edges.
			if (theResult >= 0)
			{
				std::vector<int> ringVec;
				ringVec.clear();
				BitArray tmp;
				ld->GetUVRingEdges(theResult, tmp, true, &ringVec);

				int ringCount = (int)ringVec.size();
				for (int d = 0; d < ringCount; ++d)
				{
					int tvEdgeIndex = ringVec[d];
					int fCount = ld->GetTVEdgeNumberTVFaces(tvEdgeIndex);
					for (int f = 0; f < fCount; ++f)
					{
						int fIndex = ld->GetTVEdgeConnectedTVFace(tvEdgeIndex, f);
						if (4 == ld->GetFaceDegree(fIndex) && ld->DoesFacePassFilter(fIndex))
						{
							loopFSel.Set(fIndex);
						}
					}
				}

				BitArray curFSel = ld->GetFaceSel();
				curFSel |= loopFSel;
				ld->SetFaceSelectionByRef(curFSel, false);
				return; // only process the first valid one for now.
			}
		}
	}
	return;
}

BOOL	UnwrapMod::fnGetOpenEdgeMode()
{
	return openEdgeMode;
}

void	UnwrapMod::fnSetOpenEdgeMode(BOOL mode)
{
	if (mode)
	{
		uvEdgeMode = FALSE;
		tvElementMode = FALSE;
	}

	openEdgeMode = mode;
}

void	UnwrapMod::fnUVEdgeSelect()
{
	theHold.Begin();
	HoldSelection();

	SelectUVEdge(FALSE);
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}

void	UnwrapMod::fnOpenEdgeSelect()
{
	theHold.Begin();
	HoldSelection();

	SelectOpenEdge();
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}


void	UnwrapMod::fnVertToEdgeSelect(BOOL bPartialSelect)
{
	theHold.Begin();
	HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray esel = ld->GetTVEdgeSel();
		ld->GetEdgeSelFromVert(esel, bPartialSelect);
		ld->SetTVEdgeSel(esel);
	}
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}
void	UnwrapMod::fnVertToFaceSelect(BOOL bPartialSelect)
{
	theHold.Begin();
	HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray fsel = ld->GetFaceSel();
		ld->GetFaceSelFromVert(fsel, bPartialSelect);
		ld->SetFaceSelectionByRef(fsel);
	}

	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}

void	UnwrapMod::fnEdgeToVertSelect()
{
	theHold.Begin();
	HoldSelection();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray vsel = ld->GetTVVertSel();
		ld->GetVertSelFromEdge(vsel);
		ld->SetTVVertSel(vsel);
	}

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}
void	UnwrapMod::fnEdgeToFaceSelect(BOOL bPartialSelect)
{
	theHold.Begin();
	HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray fsel = ld->GetFaceSel();
		ld->GetFaceSelFromEdge(fsel, bPartialSelect);
		ld->SetFaceSelectionByRef(fsel);
	}

	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}

void	UnwrapMod::fnFaceToVertSelect()
{
	theHold.Begin();
	HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray vsel = ld->GetTVVertSel();
		ld->GetVertSelFromFace(vsel);
		ld->SetTVVertSel(vsel);
	}

	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}

void	UnwrapMod::fnFaceToEdgeSelect()
{
	theHold.Begin();
	HoldSelection();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->ConvertFaceToEdgeSel();
	}

	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));
	InvalidateView();
}


void UnwrapMod::InitReverseSoftData()
{


	RebuildDistCache();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->InitReverseSoftData();
	}

}
void UnwrapMod::ApplyReverseSoftData()
{

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->ApplyReverseSoftData();
	}
}


int		UnwrapMod::fnGetHitSize(bool removeUIScaling)
{
	int size = hitSize;
	if (removeUIScaling)
		size = MaxSDK::UIUnScaled(size);
	return size;
}
void	UnwrapMod::fnSetHitSize(int size, bool applyUIScaling)
{
	if (applyUIScaling)
		size = MaxSDK::UIScaled(size);
	hitSize = size;
}


BOOL	UnwrapMod::fnGetPolyMode()
{
	return polyMode;
}
void	UnwrapMod::fnSetPolyMode(BOOL pmode)
{
	polyMode = pmode;
}

void UnwrapMod::PolySelection(BOOL add, BOOL bPreview)
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (nullptr != ld)
		{
			ld->PolySelection(add, bPreview);
		}
	}

	if (!bPreview)
	{
		if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

		mUIManager.UpdateCheckButtons();
	}

	InvalidateView();
}

void	UnwrapMod::fnPolySelect()
{
	fnPolySelect2(TRUE);
}

void	UnwrapMod::fnPolySelect2(BOOL add)
{
	PolySelection(add, FALSE);
}


BOOL	UnwrapMod::fnGetSyncSelectionMode()
{
	return TRUE;
	//	return syncSelection;
}

void	UnwrapMod::fnSetSyncSelectionMode(BOOL sync)
{
	syncSelection = sync;
}


void	UnwrapMod::SyncGeomFromTVSelection(MeshTopoData *ld)
{
	if (ld->IsTVEdgeValid() == FALSE)
	{
		ld->BuildTVEdges();
		ld->BuildVertexClusterList();
		if (matid != -1) // if we have a matID fileter set we need to rebuild since topology has changed
			SetMatFilters();
	}

	//get our geom face sel
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		ld->ClearGeomVertSelection();//gvsel.ClearAll();
		//loop through our faces
		for (int i = 0; i < ld->GetNumberFaces()/*TVMaps.f.Count()*/; i++)
		{
			if (!ld->GetFaceDead(i))
			{
				//iterate through the faces
				int deg = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
				for (int j = 0; j < deg; j++)
				{
					//get our geom index
					int geomIndex = ld->GetFaceGeomVert(i, j);//TVMaps.f[i]->v[j];
					//get our tv index
					int tvIndex = ld->GetFaceTVVert(i, j);//TVMaps.f[i]->t[j];
					//if geom index is selected select the tv index
					if (ld->GetTVVertSelected(tvIndex))//vsel[tvIndex])
						ld->SetGeomVertSelected(geomIndex, TRUE);
					//					gvsel.Set(geomIndex,TRUE);
				}
			}
		}
	}
	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		BitArray geomEdgeSel;;
		ld->ConvertTVEdgeSelectionToGeom(ld->GetTVEdgeSel(), geomEdgeSel);
		ld->SetGeomEdgeSel(geomEdgeSel);
	}
	else if (fnGetTVSubMode() == TVFACEMODE)
	{
	}

	//After the geometry selection perhaps is changed,update the shared TV edges in the 2d editor.
	UpdateShowSharedTVEdges(ld);
}

void	UnwrapMod::SyncTVFromGeomSelection(MeshTopoData *ld)
{
	if (ip == NULL) return;
	//get our geom face sel
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		ld->ClearTVVertSelection();
		//loop through our faces
		for (int i = 0; i < ld->GetNumberFaces(); i++)
		{
			//iterate through the faces
			int deg = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				//get our geom index
				int geomIndex = ld->GetFaceGeomVert(i, j);//TVMaps.f[i]->v[j];
				//get our tv index
				int tvIndex = ld->GetFaceTVVert(i, j);//TVMaps.f[i]->t[j];
				//if geom index is selected select the tv index
				if (ld->GetGeomVertSelected(geomIndex)/*gvsel[geomIndex]*/)
				{
					ld->SetTVVertSelected(tvIndex, TRUE);//vsel.Set(tvIndex,TRUE);
					if (ld->GetTVVertFrozen(tvIndex)/*TVMaps.v[tvIndex].flags & FLAG_FROZEN*/)
					{
						ld->SetTVVertSelected(tvIndex, FALSE);
						ld->SetGeomVertSelected(geomIndex, FALSE);
					}
				}
			}
		}
	}
	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		BitArray uvEdgeSel;
		ld->ConvertGeomEdgeSelectionToTV(ld->GetGeomEdgeSel(), uvEdgeSel);
		ld->SetTVEdgeSel(uvEdgeSel);
	}
	else if (fnGetTVSubMode() == TVFACEMODE)
	{
		BitArray fsel = ld->GetFaceSel();
		for (int i = 0; i < ld->GetNumberFaces()/*TVMaps.f.Count()*/; i++)
		{
			if (!ld->GetFaceDead(i))
			{
				int deg = ld->GetFaceDegree(i);//TVMaps.f[i]->count;
				BOOL frozen = FALSE;
				for (int j = 0; j < deg; j++)
				{
					int index = ld->GetFaceTVVert(i, j);//TVMaps.f[i]->t[j];
					if (ld->GetTVVertFrozen(index))//TVMaps.v[index].flags & FLAG_FROZEN)
						frozen = TRUE;
				}
				if (frozen)
				{
					fsel.Set(i, FALSE);
				}
			}
		}
		ld->SetFaceSelectionByRef(fsel);
	}

	//After the geometry selection perhaps is changed,update the shared TV edges in the 2d editor.
	UpdateShowSharedTVEdges(ld);
}


void	UnwrapMod::fnSyncTVSelection()
{
	if (!ip || bSuspendSelectionSync) return;

	// LAM - 6/19/04 - defect 576948 
	BOOL wasHolding = theHold.Holding();
	if (!wasHolding)
		theHold.Begin();

	HoldSelection();


	/*
		//check for type
		ModContextList mcList;
		INodeTab nodes;

		ip->GetModContexts(mcList,nodes);

		int objects = mcList.Count();
	*/

	//	if (objects != 0)
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{

		MeshTopoData *md = mMeshTopoData[ldID];//(MeshTopoData*)mcList[0]->localData;
		SyncTVFromGeomSelection(md);
	}

	mUIManager.UpdateCheckButtons();
	InvalidateView();

	if (!wasHolding)
		theHold.Accept(GetString(IDS_DS_SELECT));
}

void	UnwrapMod::SyncGeomSelection(bool bUpdateUI)
{
	if (!ip || bSuspendSelectionSync) return;

	// LAM - 6/19/04 - defect 576948 
	BOOL wasHolding = theHold.Holding();
	if (!wasHolding)
		theHold.Begin();

	HoldSelection();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *md = mMeshTopoData[ldID];//MeshTopoData *md = (MeshTopoData*)mcList[0]->localData;

		if (md)
		{
			SyncGeomFromTVSelection(md);
		}
	}

	if (!wasHolding)
		theHold.Accept(GetString(IDS_DS_SELECT));

	if (bUpdateUI)
	{
		mUIManager.UpdateCheckButtons();
		InvalidateView();
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip)	ip->RedrawViews(ip->GetTime());
	}
}

void	UnwrapMod::fnSyncGeomSelection()
{
	SyncGeomSelection(true);
}


BOOL	UnwrapMod::fnGetPaintMode()
{
	if (mode == ID_TV_PAINTSELECTMODE)
		return TRUE;
	else return FALSE;
}
void	UnwrapMod::fnSetPaintMode(BOOL paint)
{
	if (paint)
	{
		if (ip) ip->ReplacePrompt(GetString(IDS_PW_PAINTSELECTPROMPT));
		SetMode(ID_TV_PAINTSELECTMODE);
	}
	else SetMode(oldMode);

}


int		UnwrapMod::fnGetPaintSize(bool removeUIScaling)
{
	int res = paintSize;
	if (removeUIScaling)
		res = MaxSDK::UIUnScaled(res);
	return res;
}

void	UnwrapMod::fnSetPaintSize(int size, bool applyUIScaling)
{
	if (applyUIScaling)
		size = MaxSDK::UIScaled(size);
	paintSize = size;
	if (paintSize < MaxSDK::UIScaled(1)) paintSize = MaxSDK::UIScaled(1);
}

void	UnwrapMod::fnIncPaintSize()
{
	paintSize += MaxSDK::UIScaled(1);

}
void	UnwrapMod::fnDecPaintSize()
{
	paintSize -= MaxSDK::UIScaled(1);
	if (paintSize < MaxSDK::UIScaled(1)) paintSize = MaxSDK::UIScaled(1);

}


void	UnwrapMod::fnSelectInvertedFaces()
{
	//see if face mode if not bail
	if (fnGetTVSubMode() != TVFACEMODE)
		return;

	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray fsel = ld->GetFaceSel();
		//clear our selection
		fsel.ClearAll();
		//loop through our faces
		for (int i = 0; i < ld->GetNumberFaces(); i++)//TVMaps.f.Count(); i++)
		{
			//get the normal of that face
			int deg = ld->GetFaceDegree(i);//TVMaps.f[i]->count;

			BOOL hidden = FALSE;
			int hiddenCT = 0;
			for (int j = 0; j < deg; j++)
			{
				int index = ld->GetFaceTVVert(i, j);//TVMaps.f[i]->t[j];
				if ((ld->GetTVVertFrozen(index)) || (!ld->IsTVVertVisible(index)))
					//				if ((TVMaps.v[index].flags & FLAG_FROZEN) || (!IsVertVisible(index)) )
					hiddenCT++;
			}

			if (hiddenCT == deg)
				hidden = TRUE;

			if (ld->IsFaceVisible(i) && (!hidden))
			{

				Point3 vecA, vecB;
				int a, b;

				BOOL validFace = FALSE;
				for (int j = 0; j < deg; j++)
				{
					int a1, a2, a3;
					a1 = j - 1;
					a2 = j;
					a3 = j + 1;

					if (j == 0)
						a1 = deg - 1;
					if (j == (deg - 1))
						a3 = 0;

					a = ld->GetFaceTVVert(i, a2);//TVMaps.f[i]->t[a2];
					b = ld->GetFaceTVVert(i, a1);//TVMaps.f[i]->t[a1];
					vecA = Normalize(ld->GetTVVert(b) - ld->GetTVVert(a));
					//					vecA = Normalize(TVMaps.v[b].p - TVMaps.v[a].p);

					a = ld->GetFaceTVVert(i, a2);//TVMaps.f[i]->t[a2];
					b = ld->GetFaceTVVert(i, a3);//TVMaps.f[i]->t[a3];
					vecB = Normalize(ld->GetTVVert(b) - ld->GetTVVert(a));
					float dot = DotProd(vecA, vecB);
					if (dot < 1.0f)
					{
						j = deg;
						validFace = TRUE;
					}
					else
					{
					}
				}
				if (validFace)
				{
					//if it is negative flip it
					Point3 vec = CrossProd(vecA, vecB);
					if (vec.z >= 0.0f)
						fsel.Set(i, TRUE);
				}
			}
		}
		ld->SetFaceSelectionByRef(fsel);
	}

	//put back the selection
	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();

	InvalidateView();
}

void	UnwrapMod::fnGeomExpandEdgeSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray gesel = ld->GetGeomEdgeSel();
		//get the an empty vertex selection
		BitArray tempVSel;
		tempVSel.SetSize(ld->GetNumberGeomVerts());//TVMaps.v.Count());
		tempVSel.ClearAll();
		for (int i = 0; i < ld->GetNumberGeomEdges()/*TVMaps.gePtrList.Count()*/; i++)
		{
			if (gesel[i] && (!(ld->GetGeomEdgeHidden(i))))//TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA)))
			{
				int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
				tempVSel.Set(a, TRUE);
				a = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
				tempVSel.Set(a, TRUE);
			}
		}

		BitArray tempESel;
		tempESel.SetSize(ld->GetNumberGeomEdges());
		tempESel.ClearAll();
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
		{
			if (!(ld->GetGeomEdgeHidden(i)))//TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA))
			{
				int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
				int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
				if (tempVSel[a] || tempVSel[b])
					tempESel.Set(i);
			}
		}
		gesel = tempESel;
		ld->SetGeomEdgeSel(gesel);
	}

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}
void	UnwrapMod::fnGeomContractEdgeSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray gesel = ld->GetGeomEdgeSel();
		BitArray tempVSel;
		tempVSel.SetSize(ld->GetNumberGeomVerts());//TVMaps.v.Count());
		tempVSel.ClearAll();
		for (int i = 0; i < ld->GetNumberGeomEdges()/*TVMaps.gePtrList.Count()*/; i++)
		{
			if ((gesel[i]) && (!(ld->GetGeomEdgeHidden(i))))
			{
				int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
				tempVSel.Set(a, TRUE);
				a = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
				tempVSel.Set(a, TRUE);
			}
		}

		BitArray tempVBorderSel;
		tempVBorderSel.SetSize(ld->GetNumberGeomVerts());//TVMaps.v.Count());
		tempVBorderSel.ClearAll();
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
		{
			if (!(ld->GetGeomEdgeHidden(i)))
			{
				int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
				int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
				if (tempVSel[a] && !tempVSel[b])
					tempVBorderSel.Set(a, TRUE);
				if (tempVSel[b] && !tempVSel[a])
					tempVBorderSel.Set(b, TRUE);
			}
		}

		BitArray tempESel;
		tempESel.SetSize(ld->GetNumberGeomEdges());
		tempESel = gesel;
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)
		{
			int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
			int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
			if (tempVBorderSel[a] || tempVBorderSel[b])
				tempESel.Set(i, FALSE);
		}

		gesel = tempESel;
		ld->SetGeomEdgeSel(gesel);
	}

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}

void	UnwrapMod::fnGeomExpandVertexSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	//get the an empty vertex selection
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray gvsel = ld->GetGeomVertSel();
		if (gvsel.IsEmpty()) continue;
		BitArray tempVSel;
		tempVSel.SetSize(ld->GetNumberGeomVerts());//TVMaps.geomPoints.Count());
		tempVSel.ClearAll();
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)//TVMaps.gePtrList.Count(); i++)
		{

			int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
			int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;

			if (gvsel[a] || gvsel[b])
			{
				tempVSel.Set(a, TRUE);
				tempVSel.Set(b, TRUE);
			}
		}
		gvsel = gvsel | tempVSel;
		ld->SetGeomVertSel(gvsel);
	}

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());


}
void	UnwrapMod::fnGeomContractVertexSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray gvsel = ld->GetGeomVertSel();
		//get the an empty vertex selection
		BitArray tempVSel = gvsel;
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++)//TVMaps.gePtrList.Count(); i++)
		{

			int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
			int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;

			if (gvsel[a] && (!gvsel[b]))
			{
				tempVSel.Set(a, FALSE);
			}
			else if (gvsel[b] && (!gvsel[a]))
			{
				tempVSel.Set(b, FALSE);
			}
		}


		gvsel = tempVSel;
		ld->SetGeomVertSel(gvsel);
	}

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}

void UnwrapMod::fnGeomLoopSelect()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_EDGELOOPSELECTION));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray geSel = ld->GetGeomEdgeSel();
		BitArray loopedGeSel;
		ld->GetLoopedEdges(geSel, loopedGeSel);
		ld->SetGeomEdgeSel(loopedGeSel);
	}

	theHold.Suspend();
	fnSyncTVSelection();
	theHold.Resume();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::fnGeomRingSelect()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_EDGERINGSELECTION));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];

		BitArray gesel = ld->GetGeomEdgeSel();
		BitArray ringedGeSel;
		ld->GetRingedEdges(gesel, ringedGeSel);
		ld->SetGeomEdgeSel(ringedGeSel);
	}

	theHold.Suspend();
	fnSyncTVSelection();
	theHold.Resume();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::MouseGuidedEdgeLoopSelect(HitRecord *hit, int flags)
{
	_MouseGuidedEdgeSelect(hit, flags, IterateMethod_Loop);
}

void UnwrapMod::MouseGuidedEdgeRingSelect(HitRecord *hit, int flags)
{
	_MouseGuidedEdgeSelect(hit, flags, IterateMethod_Ring);
}

void UnwrapMod::_MouseGuidedEdgeSelect(HitRecord *hit, int flags, IterateMethod iter)
{
	DbgAssert(hit);

	theHold.Begin();
	HoldSelection();
	switch (iter)
	{
	case IterateMethod_Loop:
		theHold.Accept(GetString(IDS_EDGELOOPSELECTION));
		break;
	case IterateMethod_Ring:
		theHold.Accept(GetString(IDS_EDGERINGSELECTION));
		break;
	default:
		break;
	}

	int hitEdgeIndex = hit->hitInfo;
	INode *hitNode = hit->nodeRef;
	MeshTopoData *hitLD = nullptr;
	for (int ldID = 0; ldID < GetMeshTopoDataCount(); ldID++)
	{
		if (GetMeshTopoDataNode(ldID) == hitNode) {
			hitLD = GetMeshTopoData(ldID);
			break;
		}
	}

	DbgAssert(hitLD);

	BitArray oldGeSel(hitLD->GetGeomEdgeSel());
	BitArray deltaGeSel(oldGeSel.GetSize());
	BitArray iteratedDeltaGeSel(oldGeSel.GetSize());
	BitArray newGeSel(oldGeSel);

	deltaGeSel.Set(hitEdgeIndex);
	if (fnGetMirrorSelectionStatus() == TRUE)
	{
		int mirroredIndex = hitLD->GetGeomEdgeMirrorIndex(hitEdgeIndex);
		if (mirroredIndex >= 0 && mirroredIndex < deltaGeSel.GetSize())
			deltaGeSel.Set(mirroredIndex);
	}

	switch (iter)
	{
	case IterateMethod_Loop:
		hitLD->GetLoopedEdges(deltaGeSel, iteratedDeltaGeSel);
		break;
	case IterateMethod_Ring:
		hitLD->GetRingedEdges(deltaGeSel, iteratedDeltaGeSel);
		break;
	default:
		break;
	}

	switch (iter)
	{
	case IterateMethod_Loop:
		//neither ctrl nor alt pressed
		if (!(flags & MOUSE_CTRL) && !(flags & MOUSE_ALT))
		{
			newGeSel = iteratedDeltaGeSel;
		}
		//ctrl without alt
		else if ((flags & MOUSE_CTRL) && !(flags & MOUSE_ALT))
		{
			newGeSel = oldGeSel | iteratedDeltaGeSel;
		}
		//alt without ctrl
		else if (!(flags & MOUSE_CTRL) && (flags & MOUSE_ALT))
		{
			newGeSel = oldGeSel & ~iteratedDeltaGeSel;
		}
		hitLD->SetGeomEdgeSel(newGeSel);
		break;
	case IterateMethod_Ring:
		if (!(oldGeSel & iteratedDeltaGeSel).IsEmpty())
		{
			if (flags & MOUSE_ALT)
			{
				newGeSel = oldGeSel & ~iteratedDeltaGeSel;
			}
			else
			{
				newGeSel = oldGeSel | iteratedDeltaGeSel;
			}
			hitLD->SetGeomEdgeSel(newGeSel);
		}
		break;
	default:
		break;
	}

	theHold.Suspend();
	fnSyncTVSelection();
	theHold.Resume();

	AutoEdgeToSeamAndLSCMResolve();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}

void	UnwrapMod::SelectGeomElement(MeshTopoData *ld, BOOL addSel, BitArray *originalSel)
{
	if (ip)
	{
		if (ip->GetSubObjectLevel() == 1)
		{
			BitArray gvsel = ld->GetGeomVertSel();
			if (!addSel && (originalSel != NULL))
			{
				BitArray oSel = *originalSel;
				gvsel = (~gvsel) & oSel;
			}
			//loop through our edges finding ones with selected vertices
			//get the an empty vertex selection
			BitArray tempVSel;
			tempVSel.SetSize(ld->GetNumberGeomVerts());//TVMaps.geomPoints.Count());
			tempVSel.ClearAll();
			int selCount = -1;
			while (selCount != tempVSel.NumberSet())
			{
				selCount = tempVSel.NumberSet();
				for (int i = 0; i < ld->GetNumberGeomEdges();/*TVMaps.gePtrList.Count();*/ i++)
				{

					int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
					int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;

					if (gvsel[a] || gvsel[b])
					{
						tempVSel.Set(a, TRUE);
						tempVSel.Set(b, TRUE);
					}
				}
				gvsel = gvsel | tempVSel;
			}

			if (!addSel && (originalSel != NULL))
			{
				BitArray oSel = *originalSel;
				gvsel = oSel & (~gvsel);
			}

			ld->SetGeomVertSel(gvsel);
		}

		else if (ip->GetSubObjectLevel() == 2)
		{
			BitArray gesel = ld->GetGeomEdgeSel();
			if (!addSel && (originalSel != NULL))
			{
				BitArray oSel = *originalSel;
				gesel = (~gesel) & oSel;
			}

			//get the an empty vertex selection
			int selCount = -1;
			while (selCount != gesel.NumberSet())
			{
				selCount = gesel.NumberSet();
				BitArray tempVSel;
				tempVSel.SetSize(ld->GetNumberGeomVerts());//TVMaps.geomPoints.Count());
				tempVSel.ClearAll();
				for (int i = 0; i < ld->GetNumberGeomEdges();/*TVMaps.gePtrList.Count();*/ i++)
				{
					if (gesel[i])
					{
						int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
						tempVSel.Set(a, TRUE);
						a = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
						tempVSel.Set(a, TRUE);
					}
				}

				BitArray tempESel;
				tempESel.SetSize(ld->GetNumberGeomEdges());//TVMaps.gePtrList.Count());
				tempESel.ClearAll();
				for (int i = 0; i < ld->GetNumberGeomEdges();/*TVMaps.gePtrList.Count();*/ i++)
				{
					int a = ld->GetGeomEdgeVert(i, 0);//TVMaps.gePtrList[i]->a;
					int b = ld->GetGeomEdgeVert(i, 1);//TVMaps.gePtrList[i]->b;
					if (tempVSel[a] || tempVSel[b])
						tempESel.Set(i);
				}
				gesel = tempESel;
			}

			if (!addSel && (originalSel != NULL))
			{
				BitArray oSel = *originalSel;
				gesel = oSel & (~gesel);
			}
			ld->SetGeomEdgeSel(gesel);
		}
		else if (ip->GetSubObjectLevel() == 3)
		{
			BitArray holdSel = ld->GetFaceSel();
			BitArray faceSel = ld->GetFaceSel();
			if (!addSel && (originalSel != NULL))
			{
				BitArray oSel = *originalSel;
				faceSel = (~faceSel) & oSel;
			}

			Tab<BorderClass> dummy;
			Tab<Point3> normList;
			normList.ZeroCount();
			ld->BuildNormals(normList);
			BitArray tempSel;
			tempSel.SetSize(faceSel.GetSize());

			tempSel = faceSel;

			for (int i = 0; i < faceSel.GetSize(); i++)
			{
				if ((tempSel[i]) && (i >= 0) && (i < normList.Count()))
				{
					BitArray sel;
					Point3 normal = normList[i];
					SelectFacesByGroup(ld, sel, i, normal, 180.0f, FALSE, MeshTopoData::kFaceAngle, normList, dummy);
					faceSel |= sel;
					for (int j = 0; j < tempSel.GetSize(); j++)
					{
						if (sel[j])
							tempSel.Set(j, FALSE);
					}
				}
			}

			if (!addSel && (originalSel != NULL))
			{
				BitArray oSel = *originalSel;
				faceSel = oSel & (~faceSel);
			}
			ld->SetFaceSelectionByRef(faceSel);
		}
	}
}

BitArray* UnwrapMod::fnGetGeomVertexSelection(INode *node)
{
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		return ld->GetGeomVertSelectionPtr();
	}
	return NULL;
}

BitArray* UnwrapMod::fnGetGeomVertexSelection()
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetGeomVertSelectionPtr();
		}
	}
	return NULL;
}

void UnwrapMod::fnSetGeomVertexSelection(BitArray *sel, INode *node)
{
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		BitArray gvsel = ld->GetGeomVertSel();
		gvsel.ClearAll();
		for (int i = 0; i < (*sel).GetSize(); i++)
		{
			if ((i < gvsel.GetSize()) && ((*sel)[i]))
				gvsel.Set(i, TRUE);
		}

		ld->SetGeomVertSel(gvsel);
		if (fnGetMirrorSelectionStatus())
		{
			ld->MirrorGeomVertSel();
		}

		if (fnGetSyncSelectionMode())
			fnSyncTVSelection();
		RebuildDistCache();

		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);

		if (ip) ip->RedrawViews(ip->GetTime());
	}
}

void UnwrapMod::fnSetGeomVertexSelection(BitArray *sel)
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			BitArray gvsel = ld->GetGeomVertSel();
			gvsel.ClearAll();
			for (int i = 0; i < (*sel).GetSize(); i++)
			{
				if ((i < gvsel.GetSize()) && ((*sel)[i]))
					gvsel.Set(i, TRUE);
			}

			ld->SetGeomVertSel(gvsel);
			if (fnGetMirrorSelectionStatus())
			{
				ld->MirrorGeomVertSel();
			}
			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);

			if (ip) ip->RedrawViews(ip->GetTime());
		}
	}
}

BitArray* UnwrapMod::fnGetGeomEdgeSelection(INode *node)
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = GetMeshTopoData(node);
		if (ld)
		{
			return ld->GetGeomEdgeSelectionPtr();
		}
	}
	return NULL;
}


BitArray* UnwrapMod::fnGetGeomEdgeSelection()
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			return ld->GetGeomEdgeSelectionPtr();
		}
	}
	return NULL;
}

void UnwrapMod::fnSetGeomEdgeSelection(BitArray *sel, INode *node)
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = GetMeshTopoData(node);
		if (ld)
		{
			BitArray gesel = ld->GetGeomEdgeSel();
			gesel.ClearAll();
			for (int i = 0; i < (*sel).GetSize(); i++)
			{
				if ((i < gesel.GetSize()) && ((*sel)[i]))
					gesel.Set(i, TRUE);
			}
			ld->SetGeomEdgeSel(gesel);

			if (fnGetMirrorSelectionStatus())
			{
				ld->MirrorGeomEdgeSel();
			}

			if (fnGetSyncSelectionMode())
				fnSyncTVSelection();
			RebuildDistCache();
			AutoEdgeToSeamAndLSCMResolve();
			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			if (ip) ip->RedrawViews(ip->GetTime());
		}
	}
}

void UnwrapMod::fnSetGeomEdgeSelection(BitArray *sel)
{
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			BitArray gesel = ld->GetGeomEdgeSel();
			gesel.ClearAll();
			for (int i = 0; i < (*sel).GetSize(); i++)
			{
				if ((i < gesel.GetSize()) && ((*sel)[i]))
					gesel.Set(i, TRUE);
			}

			ld->SetGeomEdgeSel(gesel);
			if (fnGetMirrorSelectionStatus())
			{
				ld->MirrorGeomEdgeSel();
			}
			AutoEdgeToSeamAndLSCMResolve();

			NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			if (ip) ip->RedrawViews(ip->GetTime());
		}
	}
}


void UnwrapMod::fnPeltSeamsToSel(BOOL replace)
{

	theHold.Begin();
	HoldSelection();
	if (replace)
		theHold.Accept(GetString(IDS_PW_SEAMTOSEL));
	else theHold.Accept(GetString(IDS_PW_SEAMTOSEL2));


	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (replace)
			ld->SetGeomEdgeSel(ld->mSeamEdges);
		else
		{
			for (int i = 0; i < ld->mSeamEdges.GetSize(); i++)
			{
				if (ld->mSeamEdges[i])
					ld->SetGeomEdgeSelected(i, TRUE);//	gesel.Set(i,TRUE);
			}
		}
	}


	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());


}
void UnwrapMod::fnPeltSelToSeams(BOOL replace)
{
	theHold.Begin();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (theHold.Holding())
			theHold.Put(new UnwrapPeltSeamRestore(this, ld));

		if (fnGetMapMode() == LSCMMAP)
		{
			HoldPointsAndFaces();
		}
	}

	if (replace)
		theHold.Accept(GetString(IDS_PW_SELTOSEAM));
	else theHold.Accept(GetString(IDS_PW_SELTOSEAM2));

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray gesel = ld->GetGeomEdgeSel();
		if (replace)
		{
			ld->mSeamEdges = gesel;
		}
		else
		{
			if (ld->mSeamEdges.GetSize() != gesel.GetSize())
			{
				ld->mSeamEdges.SetSize(gesel.GetSize());
				ld->mSeamEdges.ClearAll();
			}

			for (int i = 0; i < gesel.GetSize(); i++)
			{
				if (gesel[i])
					ld->mSeamEdges.Set(i, TRUE);
			}
		}
		if (fnGetMapMode() == LSCMMAP)
		{
			fnLSCMInvalidateTopo(ld);
		}
	}

	if (fnGetMapMode() == LSCMMAP)
	{
		mLSCMTool.Solve(true,true,false);
		InvalidateView();
	}

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}

class CellInfo
{
public:
	Tab<int> facesInThisCell;
};

class Line
{
	Tab<CellInfo*> cells;
public:
	int gridSize;
	float min, max;
	float len;

	void InitializeGrid(int cellSize, float min, float max);
	void AddLine(int index, float a, float b);
	void HitLine(float a, float b);
	void FreeLine();
	int ct;
	BitArray bHitFaces;
	Tab<int> hitFaces;

};

void Line::InitializeGrid(int size, float min, float max)
{
	this->min = min;
	this->max = max;
	gridSize = size;
	len = max - min;
	cells.SetCount(size);
	for (int i = 0; i < size; i++)
		cells[i] = NULL;
	ct = 0;

}
void Line::AddLine(int index, float a, float b)
{
	a -= min;
	b -= min;
	if (a > b)
	{
		float temp = a;
		a = b;
		b = temp;
	}
	//get the start cell
	int startIndex = (a / len) * gridSize;
	//get the end cell
	int endIndex = (b / len) * gridSize;

	if (startIndex >= gridSize) startIndex = gridSize - 1;
	if (endIndex >= gridSize) endIndex = gridSize - 1;

	//if null create it
	for (int i = startIndex; i <= endIndex; i++)
	{
		if (cells[i] == NULL)
			cells[i] = new CellInfo();
		//add the primitive
		cells[i]->facesInThisCell.Append(1, &index, 10);
	}
	if (index >= ct)
		ct = index + 1;
}
void Line::HitLine(float a, float b)
{
	a -= min;
	b -= min;

	if (bHitFaces.GetSize() != ct)
		bHitFaces.SetSize(ct);
	bHitFaces.ClearAll();
	hitFaces.SetCount(0);

	if (a > b)
	{
		float temp = a;
		a = b;
		b = temp;
	}
	//get the start cell
	int startIndex = (a / len) * gridSize;
	//get the end cell
	int endIndex = (b / len) * gridSize;

	if (startIndex >= gridSize) startIndex = gridSize - 1;
	if (endIndex >= gridSize) endIndex = gridSize - 1;
	for (int i = startIndex; i <= endIndex; i++)
	{
		if (cells[i] != NULL)
		{
			int numberOfFaces = cells[i]->facesInThisCell.Count();
			for (int j = 0; j < numberOfFaces; j++)
			{
				int index = cells[i]->facesInThisCell[j];
				if (!bHitFaces[index])
				{
					hitFaces.Append(1, &index, 500);
					bHitFaces.Set(index, TRUE);
				}
			}
		}
	}

}
void Line::FreeLine()
{

	for (int i = 0; i < cells.Count(); i++)
	{
		if (cells[i])
			delete cells[i];
		cells[i] = NULL;
	}
}

void UnwrapMod::fnSelectOverlap()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(GetString(IDS_SELECT_OVERLAP));

	if (fnGetSyncSelectionMode())
		fnSyncGeomSelection();

	SelectOverlap();

	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}



void OverlapMap::Hit(int mapIndex, MeshTopoData *ld, int faceIndex)
{
	BOOL hit = TRUE;

	if (mBuffer[mapIndex].mLD == NULL)
		hit = FALSE;

	//if nothing in this cell just add it
	if (!hit)
	{
		mBuffer[mapIndex].mLD = ld;
		mBuffer[mapIndex].mFaceID = faceIndex;
	}
	else
	{
		//have somethign in the cell need to check
		//get the ld and face id in the cell
		MeshTopoData *baseLD = mBuffer[mapIndex].mLD;
		int baseFaceIndex = mBuffer[mapIndex].mFaceID;
		//hit on the same mesh id cannot be the same
		if ((baseLD == ld) &&
			(baseFaceIndex != faceIndex))
		{
			ld->SetFaceSelected(faceIndex, TRUE);
			baseLD->SetFaceSelected(baseFaceIndex, TRUE);
		}
		//hit on different mesh dont care about the face ids
		else if ((baseLD != ld))
		{
			ld->SetFaceSelected(faceIndex, TRUE);
			baseLD->SetFaceSelected(baseFaceIndex, TRUE);
		}

	}
}

void OverlapMap::Map(MeshTopoData *ld, int faceIndex, Point3 pa, Point3 pb, Point3 pc)
{
	pa *= mBufferSize;
	pb *= mBufferSize;
	pc *= mBufferSize;

	long x1 = (long)pa.x;
	long y1 = (long)pa.y;
	long x2 = (long)pb.x;
	long y2 = (long)pb.y;
	long x3 = (long)pc.x;
	long y3 = (long)pc.y;

	//sort top to bottom
	int sx[3], sy[3];
	sx[0] = x1;
	sy[0] = y1;

	if (y2 < sy[0])
	{
		sx[0] = x2;
		sy[0] = y2;

		sx[1] = x1;
		sy[1] = y1;
	}
	else
	{
		sx[1] = x2;
		sy[1] = y2;
	}

	if (y3 < sy[0])
	{
		sx[2] = sx[1];
		sy[2] = sy[1];

		sx[1] = sx[0];
		sy[1] = sy[0];


		sx[0] = x3;
		sy[0] = y3;
	}
	else if (y3 < sy[1])
	{
		sx[2] = sx[1];
		sy[2] = sy[1];

		sx[1] = x3;
		sy[1] = y3;
	}
	else
	{
		sx[2] = x3;
		sy[2] = y3;
	}

	int width = mBufferSize;
	int height = mBufferSize;
	//if flat top
	if (sy[0] == sy[1])
	{

		float xInc0 = 0.0f;
		float xInc1 = 0.0f;
		float yDist = DL_abs(sy[0] - sy[2]);
		float xDist0 = sx[2] - sx[0];
		float xDist1 = sx[2] - sx[1];
		xInc0 = xDist0 / yDist;
		xInc1 = xDist1 / yDist;

		float cx0 = sx[0];
		float cx1 = sx[1];
		for (int i = sy[0]; i < sy[2]; i++)
		{

			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;


			//line 
			if (ix0 > ix1)
			{
				int temp = ix0;
				ix0 = ix1;
				ix1 = temp;
			}
			for (int j = ix0; j <= ix1; j++)
			{
				int index = j + i * width;
				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{

					if ((j > (ix0 + 1)) && (j < (ix1 - 1)))
					{
						Hit(index, ld, faceIndex);
					}
					else
					{
						mBuffer[index].mLD = ld;
						mBuffer[index].mFaceID = faceIndex;
					}
				}

			}
			cx0 += xInc0;
			cx1 += xInc1;
		}

	}
	//it flat bottom
	else if (sy[1] == sy[2])
	{

		float xInc0 = 0.0f;
		float xInc1 = 0.0f;
		float yDist = DL_abs(sy[0] - sy[2]);
		float xDist0 = sx[1] - sx[0];
		float xDist1 = sx[2] - sx[0];
		xInc0 = xDist0 / yDist;
		xInc1 = xDist1 / yDist;

		float cx0 = sx[0];
		float cx1 = sx[0];
		for (int i = sy[0]; i < sy[2]; i++)
		{

			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				int temp = ix0;
				ix0 = ix1;
				ix1 = temp;
			}
			for (int j = ix0; j <= ix1; j++)
			{

				int index = j + i * width;
				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{
					if ((j > (ix0 + 1)) && (j < (ix1 - 1)))
					{
						Hit(index, ld, faceIndex);
					}
					else
					{
						mBuffer[index].mLD = ld;
						mBuffer[index].mFaceID = faceIndex;
					}
					//						processedPixels.Set(index,TRUE);
				}


			}
			cx0 += xInc0;
			cx1 += xInc1;
		}

	}
	else
	{


		float xInc1 = 0.0f;
		float xInc2 = 0.0f;
		float xInc3 = 0.0f;
		float yDist0to1 = DL_abs(sy[1] - sy[0]);
		float yDist0to2 = DL_abs(sy[2] - sy[0]);
		float yDist1to2 = DL_abs(sy[2] - sy[1]);

		float xDist1 = sx[1] - sx[0];
		float xDist2 = sx[2] - sx[0];
		float xDist3 = sx[2] - sx[1];
		xInc1 = xDist1 / yDist0to1;
		xInc2 = xDist2 / yDist0to2;
		xInc3 = xDist3 / yDist1to2;

		float cx0 = sx[0];
		float cx1 = sx[0];
		//go from s[0] to s[1]
		for (int i = sy[0]; i < sy[1]; i++)
		{

			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				int temp = ix0;
				ix0 = ix1;
				ix1 = temp;
			}
			for (int j = ix0; j <= ix1; j++)
			{

				int index = j + i * width;
				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{
					if ((j > (ix0 + 1)) && (j < (ix1 - 1)))
					{
						Hit(index, ld, faceIndex);
					}
					else
					{
						mBuffer[index].mLD = ld;
						mBuffer[index].mFaceID = faceIndex;
					}
				}

			}
			cx0 += xInc1;
			cx1 += xInc2;
		}
		//go from s[1] to s[2]
		for (int i = sy[1]; i < sy[2]; i++)
		{

			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				int temp = ix0;
				ix0 = ix1;
				ix1 = temp;
			}
			for (int j = ix0; j <= ix1; j++)
			{

				int index = j + i * width;
				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{
					if ((j > (ix0 + 1)) && (j < (ix1 - 1)))
					{
						Hit(index, ld, faceIndex);
					}
					else
					{
						mBuffer[index].mLD = ld;
						mBuffer[index].mFaceID = faceIndex;
					}
				}

			}
			cx0 += xInc3;
			cx1 += xInc2;
		}

	}
}

void UnwrapMod::SelectOverlap()
{
	//get our bounding box
	Box3 bounds;
	bounds.Init();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		int numVerts = ld->GetNumberTVVerts();
		for (int i = 0; i < numVerts; i++)
		{
			bounds += ld->GetTVVert(i);
		}
	}
	//put a small fudge to keep faces off the edge
	bounds.EnlargeBy(0.05f);
	//build our transform

	float xScale = bounds.pmax.x - bounds.pmin.x;
	float yScale = bounds.pmax.y - bounds.pmin.y;
	Point3 offset = bounds.pmin;

	//create our buffer
	OverlapMap overlapMap;
	overlapMap.Init();


	//see if we have any existing selections
	BOOL hasSelection = FALSE;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (ld->GetFaceSelectionPtr()->AnyBitSet())
		{
			hasSelection = TRUE;
			ldID = mMeshTopoData.Count();
		}
	}

	//add all the faces or just selected faces if there are any
	Tab<OverlapCell> facesToCheck;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		int numFaces = ld->GetNumberFaces();
		for (int i = 0; i < numFaces; i++)
		{
			if (hasSelection)
			{
				if (ld->GetFaceSelected(i))
				{
					OverlapCell t;
					t.mFaceID = i;
					t.mLD = ld;
					facesToCheck.Append(1, &t, 10000);
				}
			}
			else
			{
				OverlapCell t;
				t.mFaceID = i;
				t.mLD = ld;
				facesToCheck.Append(1, &t, 10000);
			}
		}
	}

	//clear all the selections
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->ClearFaceSelection();
	}

	//loop through the faces to check
	for (int ldID = 0; ldID < facesToCheck.Count(); ldID++)
	{
		MeshTopoData *ld = facesToCheck[ldID].mLD;
		//loop through the faces
		int i = facesToCheck[ldID].mFaceID;

		int deg = ld->GetFaceDegree(i);
		int index[4];
		index[0] = ld->GetFaceTVVert(i, 0);
		Point3 pa = ld->GetTVVert(index[0]);
		pa.x -= offset.x;
		pa.x /= xScale;

		pa.y -= offset.y;
		pa.y /= yScale;

		for (int j = 0; j < deg - 2; j++)
		{
			index[1] = ld->GetFaceTVVert(i, j + 1);
			index[2] = ld->GetFaceTVVert(i, j + 2);
			Point3 pb = ld->GetTVVert(index[1]);
			Point3 pc = ld->GetTVVert(index[2]);

			pb.x -= offset.x;
			pb.x /= xScale;
			pb.y -= offset.y;
			pb.y /= yScale;

			pc.x -= offset.x;
			pc.x /= xScale;
			pc.y -= offset.y;
			pc.y /= yScale;

			//add face to buffer
			//select anything that overlaps
			overlapMap.Map(ld, i, pa, pb, pc);
		}
	}
}

BOOL UnwrapMod::BXPLine(long x1, long y1, long x2, long y2,
	int width, int height, int id,
	Tab<int> &map, BOOL clearEnds)



{
	long i, px, py, x, y;
	long dx, dy, dxabs, dyabs, sdx, sdy;
	long count;


	if (clearEnds)
	{
		map[x1 + y1*(width)] = -1;
		map[x2 + y2*(width)] = -1;
	}


	dx = x2 - x1;
	dy = y2 - y1;

	if (dx > 0)
		sdx = 1;
	else
		sdx = -1;

	if (dy > 0)
		sdy = 1;
	else
		sdy = -1;


	dxabs = abs(dx);
	dyabs = abs(dy);

	x = 0;
	y = 0;
	px = x1;
	py = y1;

	count = 0;
	BOOL iret = FALSE;


	if (dxabs >= dyabs)
		for (i = 0; i <= dxabs; i++)
		{
			y += dyabs;
			if (y >= dxabs)
			{
				y -= dxabs;
				py += sdy;
			}


			if ((px >= 0) && (px < width) &&
				(py >= 0) && (py < height))
			{
				int tid = map[px + py*width];
				if (tid != -1)
				{
					if ((tid != 0) && (tid != id))
						iret = TRUE;
					map[px + py*width] = id;
				}
			}


			px += sdx;
		}


	else

		for (i = 0; i <= dyabs; i++)
		{
			x += dxabs;
			if (x >= dyabs)
			{
				x -= dyabs;
				px += sdx;
			}


			if ((px >= 0) && (px < width) &&
				(py >= 0) && (py < height))
			{
				int tid = map[px + py*width];
				if (tid != -1)
				{

					if ((tid != 0) && (tid != id))
						iret = TRUE;
					map[px + py*width] = id;
				}
			}



			py += sdy;
		}
	(count)--;
	return iret;
}

//VSNAP
void UnwrapMod::BuildSnapBuffer()
{
	TransferSelectionStart();
	BOOL vSnap = 0, eSnap = 0;
	pblock->GetValue(unwrap_vertexsnap, 0, vSnap, FOREVER);
	pblock->GetValue(unwrap_edgesnap, 0, eSnap, FOREVER);

	if (!vSnap && !eSnap)
	{
		FreeSnapBuffer();
	}
	else
	{
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			//get our window width height
			float xzoom, yzoom;
			int width, height;
			ComputeZooms(hView, xzoom, yzoom, width, height);

			try
			{
				ld->BuildSnapBuffer(width, height);
			}
			catch (std::bad_alloc&)
			{
				MessageBox(NULL, _T("Out of memory - vertex and edge snaps have been disabled.\nTry using a smaller Edit UVWs window."), _T("Out of memory"), MB_OK);
				FreeSnapBuffer();
				pblock->SetValue(unwrap_vertexsnap, 0, FALSE);
				pblock->SetValue(unwrap_edgesnap, 0, FALSE);
				break;
			}

			Tab<IPoint2> transformedPoints;
			transformedPoints.SetCount(ld->GetNumberTVVerts());//TVMaps.v.Count());

			//build the vertex bufffer list
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
			{
				Point3 tvPoint = UVWToScreen(ld->GetTVVert(i), xzoom, yzoom, width, height);
				IPoint2 tvPoint2;
				tvPoint2.x = (int)tvPoint.x;
				tvPoint2.y = (int)tvPoint.y;
				transformedPoints[i] = tvPoint2;
			}

			//loop through our verts 
			if (vSnap)
			{
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)//TVMaps.v.Count(); i++) 
				{
					//if in window add it
					if ((transformedPoints[i].x >= 0) && (transformedPoints[i].x < width) &&
						(transformedPoints[i].y >= 0) && (transformedPoints[i].y < height))
					{
						int x = transformedPoints[i].x;
						int y = transformedPoints[i].y;

						int index = y * width + x;
						if ((!ld->GetTVVertSelected(i)) && ld->IsTVVertVisible(i))
							ld->SetVertexSnapBuffer(index, i);
						//						vertexSnapBuffer[index] = i;
					}
				}
			}
			//loop through the edges
			if (eSnap)
			{
				for (int i = 0; i < ld->GetNumberTVEdges(); i++)//TVMaps.ePtrList.Count(); i++)
				{
					//add them to the edge buffer
					if (!(ld->GetTVEdgeHidden(i)))//TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA))
					{
						int a = ld->GetTVEdgeVert(i, 0);//TVMaps.ePtrList[i]->a;
						int b = ld->GetTVEdgeVert(i, 1);//TVMaps.ePtrList[i]->b;
						BOOL aHidden = (!ld->IsTVVertVisible(a));
						BOOL bHidden = (!ld->IsTVVertVisible(b));
						if ((a == mMouseHitVert) || (b == mMouseHitVert) || ld->GetTVVertSelected(a) || ld->GetTVVertSelected(b) || aHidden || bHidden)
						{
							ld->SetEdgesConnectedToSnapvert(i, TRUE);
						}
						IPoint2 pa, pb;
						pa = transformedPoints[a];
						pb = transformedPoints[b];

						long x1, y1, x2, y2;

						x1 = (long)pa.x;
						y1 = (long)pa.y;

						x2 = (long)pb.x;
						y2 = (long)pb.y;

						if (!ld->GetEdgesConnectedToSnapvert(i))
							BXPLine(x1, y1, x2, y2,
								width, height, i,
								ld->GetEdgeSnapBuffer(), FALSE);
					}

				}
			}

		}
	}

	TransferSelectionEnd(FALSE, FALSE);

}

void UnwrapMod::FreeSnapBuffer()
{
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ++ldID)
	{
		MeshTopoData* ld = mMeshTopoData[ldID];
		ld->FreeSnapBuffer();
	}
}

BOOL UnwrapMod::GetGridSnap()
{
	BOOL snap = 0;
	pblock->GetValue(unwrap_gridsnap, 0, snap, FOREVER);
	return snap;
}

void UnwrapMod::SetGridSnap(BOOL snap)
{
	pblock->SetValue(unwrap_gridsnap, 0, snap);
}

BOOL UnwrapMod::GetVertexSnap()
{
	BOOL snap = 0;
	pblock->GetValue(unwrap_vertexsnap, 0, snap, FOREVER);
	return snap;

}
void UnwrapMod::SetVertexSnap(BOOL snap)
{
	pblock->SetValue(unwrap_vertexsnap, 0, snap);
}
BOOL UnwrapMod::GetEdgeSnap()
{
	BOOL snap = 0;
	pblock->GetValue(unwrap_edgesnap, 0, snap, FOREVER);
	return snap;

}
void UnwrapMod::SetEdgeSnap(BOOL snap)
{
	pblock->SetValue(unwrap_edgesnap, 0, snap);
}


bool UnwrapMod::AnyThingSelected()
{
	int selCount = 0;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			selCount += ld->GetTVVertSel().NumberSet();
		}
		else if (fnGetTVSubMode() == TVEDGEMODE)
		{
			selCount += ld->GetTVEdgeSel().NumberSet();
		}
		else if (fnGetTVSubMode() == TVFACEMODE)
		{
			selCount += ld->GetFaceSel().NumberSet();
		}

	}

	if (selCount != 0)
	{
		return true;
	}
	return false;
}
