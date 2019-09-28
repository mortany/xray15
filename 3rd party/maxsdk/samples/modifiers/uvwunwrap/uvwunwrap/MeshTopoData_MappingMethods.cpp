
/********************************************************************** *<
FILE: MeshTopoData.cpp

DESCRIPTION: local mode data for the unwrap dealing wiht mapping

HISTORY: 9/25/2006
CREATED BY: Peter Watje

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/

#include "unwrap.h"
#include "modsres.h"
#include "utilityMethods.h"
#include <Util/BailOut.h>

void MeshTopoData::DetachFromGeoFaces(const BitArray& faceSel, BitArray &vertSel)
{
	TimeValue t = 0;
	if (GetCOREInterface())
		t = GetCOREInterface()->GetTime();

	//loop through the geo vertices and create our geo list of all verts in the 
	Tab<int> geoPoints;

	BitArray usedGeoPoints;
	usedGeoPoints.SetSize(TVMaps.geomPoints.Count());
	usedGeoPoints.ClearAll();

	BitArray isolatedPoints;
	isolatedPoints.SetSize(TVMaps.v.Count());
	isolatedPoints.SetAll();
	int fCount = TVMaps.f.Count();
	for (int i = 0; i < fCount; i++)
	{
		if (!(TVMaps.f[i]->flags & FLAG_DEAD))
		{
			if (faceSel[i])
			{
				int degree = TVMaps.f[i]->count;
				for (int j = 0; j < degree; j++)
				{
					int index = TVMaps.f[i]->v[j];
					if (!usedGeoPoints[index])
					{
						usedGeoPoints.Set(index);
						geoPoints.Append(1, &index, 3000);
					}
					if (TVMaps.f[i]->vecs)
					{
						int index = TVMaps.f[i]->vecs->vhandles[j * 2];
						if (index != -1)
						{
							if (!usedGeoPoints[index])
							{
								usedGeoPoints.Set(index);
								geoPoints.Append(1, &index, 3000);
							}
						}
						index = TVMaps.f[i]->vecs->vhandles[j * 2 + 1];
						if (index != -1)
						{
							if (!usedGeoPoints[index])
							{
								usedGeoPoints.Set(index);
								geoPoints.Append(1, &index, 3000);
							}
						}
						index = TVMaps.f[i]->vecs->vinteriors[j];
						if (index != -1)
						{
							if (!usedGeoPoints[index])
							{
								usedGeoPoints.Set(index);
								geoPoints.Append(1, &index, 3000);
							}
						}
					}
				}
			}
			else
			{
				int degree = TVMaps.f[i]->count;
				for (int j = 0; j < degree; j++)
				{
					int index = TVMaps.f[i]->t[j];
					isolatedPoints.Clear(index);
					if (TVMaps.f[i]->vecs)
					{
						int index = TVMaps.f[i]->vecs->handles[j * 2];
						if (index != -1)
						{
							isolatedPoints.Clear(index);
						}
						index = TVMaps.f[i]->vecs->handles[j * 2 + 1];
						if (index != -1)
						{
							isolatedPoints.Clear(index);
						}
						index = TVMaps.f[i]->vecs->interiors[j];
						if (index != -1)
						{
							isolatedPoints.Clear(index);
						}
					}
				}
			}
		}
	}

	int vCount = TVMaps.v.Count();
	for (int i = 0; i < vCount; i++)
	{
		if (isolatedPoints[i])
			SetTVVertDead(i, TRUE);
	}

	//get our dead verts
	Tab<int> deadVerts;
	for (int i = 0; i < vCount; i++)
	{
		if (TVMaps.v[i].IsDead() && !TVMaps.mSystemLockedFlag[i])
		{
			deadVerts.Append(1, &i, 1000);
		}
	}

	//build the look up list
	Tab<int> lookupList;
	lookupList.SetCount(TVMaps.geomPoints.Count());
	for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
		lookupList[i] = -1;
	int deadIndex = 0;
	for (int i = 0; i < geoPoints.Count(); i++)
	{
		int vIndex = geoPoints[i];
		Point3 p = TVMaps.geomPoints[vIndex];

		if (deadIndex < deadVerts.Count())
		{
			lookupList[vIndex] = deadVerts[deadIndex];
			SetTVVertControlIndex(deadVerts[deadIndex], -1);
			SetTVVert(t, deadVerts[deadIndex], p);//TVMaps.v[found].p = p;
			SetTVVertInfluence(deadVerts[deadIndex], 0.0f);//TVMaps.v[found].influence = 0.0f;
			SetTVVertDead(deadVerts[deadIndex], FALSE);//TVMaps.v[found].flags -= FLAG_DEAD;
			if (IsTVVertexFilterValid())
			{
				mTVVertexFilter.Set(deadVerts[deadIndex]);
			}
			deadIndex++;
		}
		else
		{
			lookupList[vIndex] = TVMaps.v.Count();
			UVW_TVVertClass tv;
			tv.SetP(p);
			tv.SetFlag(0);
			tv.SetInfluence(0.0f);
			tv.SetControlID(-1);
			TVMaps.v.Append(1, &tv, 1);
			if (IsTVVertexFilterValid())
			{
				mTVVertexFilter.SetSize(TVMaps.v.Count(), 1);
				mTVVertexFilter.Set((TVMaps.v.Count() - 1));//enable the newly added one!
			}
			ResizeTVVertSelection(TVMaps.v.Count(), 1);
			TVMaps.mSystemLockedFlag.SetSize(TVMaps.v.Count(), 1);
		}
	}

	vertSel.SetSize(TVMaps.v.Count());
	vertSel.ClearAll();

	int fCount2 = TVMaps.f.Count();
	for (int i = 0; i < fCount2; i++)
	{
		if ((faceSel[i]) && (!(TVMaps.f[i]->flags & FLAG_DEAD)))
		{

			int degree = TVMaps.f[i]->count;
			for (int j = 0; j < degree; j++)
			{
				int index = TVMaps.f[i]->v[j];
				TVMaps.f[i]->t[j] = lookupList[index];
				vertSel.Set(lookupList[index], TRUE);

				if (TVMaps.f[i]->vecs)
				{
					int index = TVMaps.f[i]->vecs->vhandles[j * 2];
					if ((index != -1) && (lookupList[index] != -1))
					{
						TVMaps.f[i]->vecs->handles[j * 2] = lookupList[index];
						vertSel.Set(lookupList[index], TRUE);
					}

					index = TVMaps.f[i]->vecs->vhandles[j * 2 + 1];
					if ((index != -1) && (lookupList[index] != -1))
					{
						TVMaps.f[i]->vecs->handles[j * 2 + 1] = lookupList[index];
						vertSel.Set(lookupList[index], TRUE);
					}
					index = TVMaps.f[i]->vecs->vinteriors[j];
					if ((index != -1) && (lookupList[index] != -1))
					{
						TVMaps.f[i]->vecs->interiors[j] = lookupList[index];
						vertSel.Set(lookupList[index], TRUE);
					}
				}
			}
		}
	}
}

int MeshTopoData::GetVertexClusterListCount() 
{ 
	return mVertexClusterList.Count(); 
}

int MeshTopoData::GetVertexClusterData(int i)
{
	if (i >= 0 && i < mVertexClusterList.Count())
	{
		return mVertexClusterList[i];
	}
	return -1;
}

int MeshTopoData::GetVertexClusterSharedListCount() 
{ 
	return mVertexClusterListCounts.Count(); 
}

int MeshTopoData::GetVertexClusterSharedData(int i)
{
	if (i >= 0 && i < mVertexClusterListCounts.Count())
	{
		return mVertexClusterListCounts[i];
	}
	return -1;
}

void	MeshTopoData::AddVertsToCluster(int faceIndex, BitArray &processedVerts, ClusterClass *cluster)
{
	int degree = TVMaps.f[faceIndex]->count;
	for (int k = 0; k < degree; k++)
	{
		int index = TVMaps.f[faceIndex]->t[k];
		if (!processedVerts[index])
		{
			cluster->verts.Append(1, &index, 100);
			processedVerts.Set(index, TRUE);
		}
		if (TVMaps.f[faceIndex]->vecs)
		{
			int index = TVMaps.f[faceIndex]->vecs->handles[k * 2];
			if (!processedVerts[index])
			{
				cluster->verts.Append(1, &index, 100);
				processedVerts.Set(index, TRUE);
			}
			index = TVMaps.f[faceIndex]->vecs->handles[k * 2 + 1];
			if (!processedVerts[index])
			{
				cluster->verts.Append(1, &index, 100);
				processedVerts.Set(index, TRUE);
			}
			index = TVMaps.f[faceIndex]->vecs->interiors[k];
			if (!processedVerts[index])
			{
				cluster->verts.Append(1, &index, 100);
				processedVerts.Set(index, TRUE);
			}
		}
	}
}

void	MeshTopoData::UpdateClusterVertices(Tab<ClusterClass*> &clusterList)
{
	BitArray processedVerts;
	processedVerts.SetSize(TVMaps.v.Count());

	for (int i = 0; i < clusterList.Count(); i++)
	{

		if (clusterList[i]->ld == this)
		{
			clusterList[i]->verts.SetCount(0);
			processedVerts.ClearAll();
			for (int j = 0; j < clusterList[i]->faces.Count(); j++)
			{
				int findex = clusterList[i]->faces[j];
				AddVertsToCluster(findex, processedVerts, clusterList[i]);
			}
		}
	}
}

void	MeshTopoData::AlignCluster(Tab<ClusterClass*> &clusterList, int moveCluster, int innerFaceIndex, int outerFaceIndex, int edgeIndex)
{
	//get edges that are coincedent
	int vInner[2];
	int vOuter[2];

	int vInnerVec[2];
	int vOuterVec[2];


	int ct = 0;

	for (int i = 0; i < TVMaps.f[innerFaceIndex]->count; i++)
	{
		int innerIndex = TVMaps.f[innerFaceIndex]->v[i];
		for (int j = 0; j < TVMaps.f[outerFaceIndex]->count; j++)
		{
			int outerIndex = TVMaps.f[outerFaceIndex]->v[j];
			if (innerIndex == outerIndex)
			{
				vInner[ct] = TVMaps.f[innerFaceIndex]->t[i];
				vOuter[ct] = TVMaps.f[outerFaceIndex]->t[j];
				ct++;
			}
		}
	}

	vInnerVec[0] = -1;
	vInnerVec[1] = -1;
	vOuterVec[0] = -1;
	vOuterVec[1] = -1;
	ct = 0;

	if ((TVMaps.f[innerFaceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[innerFaceIndex]->vecs) &&
		(TVMaps.f[outerFaceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[outerFaceIndex]->vecs)
		)
	{
		for (int i = 0; i < TVMaps.f[innerFaceIndex]->count * 2; i++)
		{
			int innerIndex = TVMaps.f[innerFaceIndex]->vecs->vhandles[i];
			for (int j = 0; j < TVMaps.f[outerFaceIndex]->count * 2; j++)
			{
				int outerIndex = TVMaps.f[outerFaceIndex]->vecs->vhandles[j];
				if (innerIndex == outerIndex)
				{
					int vec = TVMaps.f[innerFaceIndex]->vecs->handles[i];
					vInnerVec[ct] = vec;


					vec = TVMaps.f[outerFaceIndex]->vecs->handles[j];
					vOuterVec[ct] = vec;
					ct++;
				}
			}
		}
	}

	//get  align vector
	Point3 pInner[2];
	Point3 pOuter[2];

	pInner[0] = TVMaps.v[vInner[0]].GetP();
	pInner[1] = TVMaps.v[vInner[1]].GetP();

	pOuter[0] = TVMaps.v[vOuter[0]].GetP();
	pOuter[1] = TVMaps.v[vOuter[1]].GetP();

	Point3 offset = pInner[0] - pOuter[0];

	Point3 vecA, vecB;
	vecA = Normalize(pInner[1] - pInner[0]);
	vecB = Normalize(pOuter[1] - pOuter[0]);
	float dot = DotProd(vecA, vecB);

	float angle = 0.0f;
	if (dot == -1.0f)
		angle = PI;
	else if (dot >= 1.0f)
		angle = 0.f;
	else angle = acos(dot);

	if ((_isnan(angle)) || (!_finite(angle)))
		angle = 0.0f;



	Matrix3 tempMat(1);
	tempMat.RotateZ(angle);
	Point3 vecC = VectorTransform(tempMat, vecB);




	float negAngle = -angle;
	Matrix3 tempMat2(1);
	tempMat2.RotateZ(negAngle);
	Point3 vecD = VectorTransform(tempMat2, vecB);

	float la, lb;
	la = Length(vecA - vecC);
	lb = Length(vecA - vecD);
	if (la > lb)
		angle = negAngle;

	clusterList[moveCluster]->newX = offset.x;
	clusterList[moveCluster]->newY = offset.y;
	//build vert list
	//move those verts
	BitArray processVertList;
	processVertList.SetSize(TVMaps.v.Count());
	processVertList.ClearAll();
	for (int i = 0; i < clusterList[moveCluster]->faces.Count(); i++)
	{
		int faceIndex = clusterList[moveCluster]->faces[i];
		for (int j = 0; j < TVMaps.f[faceIndex]->count; j++)
		{
			int vertexIndex = TVMaps.f[faceIndex]->t[j];
			processVertList.Set(vertexIndex);


			if ((patch) && (TVMaps.f[faceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[faceIndex]->vecs))
			{
				int vertIndex;

				if (TVMaps.f[faceIndex]->flags & FLAG_INTERIOR)
				{
					vertIndex = TVMaps.f[faceIndex]->vecs->interiors[j];
					if ((vertIndex >= 0) && (vertIndex < processVertList.GetSize()))
						processVertList.Set(vertIndex);
				}

				vertIndex = TVMaps.f[faceIndex]->vecs->handles[j * 2];
				if ((vertIndex >= 0) && (vertIndex < processVertList.GetSize()))
					processVertList.Set(vertIndex);
				vertIndex = TVMaps.f[faceIndex]->vecs->handles[j * 2 + 1];
				if ((vertIndex >= 0) && (vertIndex < processVertList.GetSize()))
					processVertList.Set(vertIndex);

			}


		}
	}
	for (int i = 0; i < processVertList.GetSize(); i++)
	{
		if (processVertList[i])
		{
			Point3 p = TVMaps.v[i].GetP();
			//move to origin

			p -= pOuter[0];

			//rotate
			Matrix3 mat(1);

			mat.RotateZ(angle);

			p = p * mat;
			//move to anchor point        
			p += pInner[0];

			SetTVVert(0, i, p);//TVMaps.v[i].p = p;
			//if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(0,&TVMaps.v[i].p);

		}
	}

	if ((vInnerVec[0] != -1) && (vInnerVec[1] != -1) && (vOuterVec[0] != -1) && (vOuterVec[1] != -1))
	{
		SetTVVert(0, vOuterVec[0], TVMaps.v[vInnerVec[0]].GetP());
		SetTVVert(0, vOuterVec[1], TVMaps.v[vInnerVec[1]].GetP());
	}
}

void MeshTopoData::PlanarMapNoScale(Point3 gNormal)
{
	Matrix3 gtm;
	UnwrapUtilityMethods::UnwrapMatrixFromNormal(gNormal, gtm);

	gtm = Inverse(gtm);

	BitArray tempVSel;
	DetachFromGeoFaces(mFSel, tempVSel);

	TimeValue t = 0;
	if (GetCOREInterface())
		t = GetCOREInterface()->GetTime();

	Box3 bounds;
	bounds.Init();
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if (tempVSel[i])
		{
			bounds += TVMaps.v[i].GetP();
		}
	}
	Point3 gCenter = bounds.Center();

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		if (tempVSel[i])
		{
			TVMaps.v[i].SetFlag(0);
			TVMaps.v[i].SetInfluence(0.0f);

			Point3 tp = TVMaps.v[i].GetP() - gCenter;
			tp = tp * gtm;
			tp.z = 0.0f;

			SetTVVert(t, i, tp);
		}
	}

	SetTVVertSel(tempVSel);
}

void MeshTopoData::BuildNormals(Tab<Point3>& normList)
{
	// TODO: we always calculate normal ourself but not respect normal from edit normal. May need be fixed.
	if (GetMesh())
	{
		normList.SetCount(GetMesh()->numFaces);
		for (int i = 0; i < GetMesh()->numFaces; i++)
		{
			//build normal
			Point3 p[3];
			for (int j = 0; j < 3; j++)
			{
				p[j] = GetMesh()->verts[GetMesh()->faces[i].v[j]];
			}
			Point3 vecA, vecB, norm;
			vecA = Normalize(p[1] - p[0]);
			vecB = Normalize(p[2] - p[0]);
			norm = Normalize(CrossProd(vecA, vecB));


			normList[i] = norm;
		}
	}
	else if (GetMNMesh())
	{
		normList.SetCount(GetMNMesh()->numf);
		for (int i = 0; i < GetMNMesh()->numf; i++)
		{
			Point3 norm = GetMNMesh()->GetFaceNormal(i, TRUE);
			normList[i] = norm;
		}
	}
	else if (GetPatch())
	{
		normList.SetCount(GetPatch()->numPatches);
		for (int i = 0; i < GetPatch()->numPatches; i++)
		{
			Point3 norm(0.0f, 0.0f, 0.0f);
			for (int j = 0; j < GetPatch()->patches[i].type; j++)
			{
				Point3 vecA, vecB, p;
				p = GetPatch()->verts[GetPatch()->patches[i].v[j]].p;
				vecA = GetPatch()->vecs[GetPatch()->patches[i].vec[j * 2]].p;
				if (j == 0)
					vecB = GetPatch()->vecs[GetPatch()->patches[i].vec[GetPatch()->patches[i].type * 2 - 1]].p;
				else vecB = GetPatch()->vecs[GetPatch()->patches[i].vec[j * 2 - 1]].p;
				vecA = Normalize(p - vecA);
				vecB = Normalize(p - vecB);
				norm += Normalize(CrossProd(vecA, vecB));
			}
			normList[i] = Normalize(norm / (float)GetPatch()->patches[i].type);
		}
	}
}

BOOL MeshTopoData::NormalMap(Tab<Point3*> *normalList, Tab<ClusterClass*> &clusterList)
{
	if (TVMaps.f.Count() == 0) return FALSE;

	BitArray holdPolySel = GetFaceSel();

	Point3 normal(0.0f, 0.0f, 1.0f);

	Tab<Point3> mapNormal;
	mapNormal.SetCount(normalList->Count());
	for (int i = 0; i < mapNormal.Count(); i++)
	{
		mapNormal[i] = *(*normalList)[i];
		ClusterClass *cluster = new ClusterClass();
		cluster->normal = mapNormal[i];
		cluster->ld = this;
		clusterList.Append(1, &cluster);
	}

	//check for type

	BOOL bContinue = TRUE;
	TSTR statusMessage;

	Tab<Point3> objNormList;
	BuildNormals(objNormList);

	if (objNormList.Count() == 0)
	{
		return FALSE;
	}

	BitArray skipFace;
	skipFace.SetSize(mFSel.GetSize());
	skipFace.ClearAll();

	for (int i = 0; i < mFSel.GetSize(); i++)
	{
		if (!GetFaceSelected(i))
			skipFace.Set(i);
	}

    MaxSDK::Util::BailOutManager bailoutManager;
	for (int i = 0; i < objNormList.Count(); i++)
	{
		int index = -1;
		float angle = 0.0f;
		if (skipFace[i] == FALSE)
		{
			for (int j = 0; j < clusterList.Count(); j++)
			{
				if (clusterList[j]->ld == this)
				{
					float dot = DotProd(objNormList[i], clusterList[j]->normal);//mapNormal[j]);
					float newAngle = (acos(dot));

					if ((dot >= 1.0f) || (newAngle <= angle) || (index == -1))
					{
						index = j;
						angle = newAngle;
					}
				}
			}
			if (index != -1)
			{
				clusterList[index]->faces.Append(1, &i);
			}
		}
	}

	BitArray sel;
	sel.SetSize(TVMaps.f.Count());

	for (int i = 0; i < clusterList.Count(); i++)
	{
		if (clusterList[i]->ld == this)
		{
			sel.ClearAll();
			for (int j = 0; j < clusterList[i]->faces.Count(); j++)
				sel.Set(clusterList[i]->faces[j]);
			if (sel.NumberSet() > 0)
			{
				SetFaceSelectionByRef(sel);
				PlanarMapNoScale(clusterList[i]->normal);

			}

			int per = (i * 100) / clusterList.Count();
			statusMessage.printf(_T("%s %d%%."), GetString(IDS_PW_STATUS_MAPPING), per);
            UnwrapUtilityMethods::UpdatePrompt(GetCOREInterface(), statusMessage.data());
			if (bailoutManager.ShouldBail())
			{
				i = clusterList.Count();
				bContinue = FALSE;
			}
		}
	}

	for (int i = 0; i < clusterList.Count(); i++)
	{
		if (clusterList[i]->faces.Count() == 0)
		{
			delete clusterList[i];
			clusterList.Delete(i, 1);
			i--;
		}
	}

	BitArray clusterVerts;
	clusterVerts.SetSize(TVMaps.v.Count());

	for (int i = 0; i < clusterList.Count(); i++)
	{

		if (clusterList[i]->ld == this)
		{
			clusterVerts.ClearAll();
			for (int j = 0; j < clusterList[i]->faces.Count(); j++)
			{
				int findex = clusterList[i]->faces[j];
				AddVertsToCluster(findex, clusterVerts, clusterList[i]);
			}
		}
	}

	SetFaceSelectionByRef(holdPolySel);

	return bContinue;
}

void MeshTopoData::ApplyMap(int mapType, BOOL normalizeMap, const Matrix3& gizmoTM, int matid)
{
	//this just catches some strane cases where the z axis is 0 which causes some bad numbers
	/*
	if (Length(gizmoTM.GetRow(2)) == 0.0f)
		{
			gizmoTM.SetRow(2,Point3(0.0f,0.0f,1.0f));
		}
	*/

	float circ = 1.0f;
	/*
		if (!normalizeMap)
		{
			for (int i = 0; i < 3; i++)
			{
				Point3 vec = gizmoTM.GetRow(i);
				vec = Normalize(vec);
				gizmoTM.SetRow(i,vec);
			}
		}
	*/
	Matrix3 toGizmoSpace = gizmoTM;

	BitArray tempVSel;
	DetachFromGeoFaces(mFSel, tempVSel);
	TimeValue t = GetCOREInterface()->GetTime();

	//build available list

	if (mapType == SPHERICALMAP)
	{
		Tab<int> quads;
		quads.SetCount(TVMaps.v.Count());

		float longestR = 0.0f;
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			quads[i] = -1;
			if (tempVSel[i])
			{
				Point3 tp = TVMaps.v[i].GetP() * gizmoTM;//gverts.d[i].p * gtm;
					//z our y

				int quad = 0;
				if ((tp.x >= 0) && (tp.y >= 0))
					quad = 0;
				else if ((tp.x < 0) && (tp.y >= 0))
					quad = 1;
				else if ((tp.x < 0) && (tp.y < 0))
					quad = 2;
				else if ((tp.x >= 0) && (tp.y < 0))
					quad = 3;

				quads[i] = quad;

				//find the quad the point is in
				Point3 xvec(1.0f, 0.0f, 0.0f);
				Point3 zvec(0.0f, 0.0f, -1.0f);

				float x = 0.0f;
				Point3 zp = tp;
				zp.z = 0.0f;

				float dot = DotProd(xvec, Normalize(zp));
				float angle = 0.0f;
				if (dot >= 1.0f)
					angle = 0.0f;
				else angle = acos(dot);

				x = angle / (PI*2.0f);
				if (quad > 1)
					x = (0.5f - x) + 0.5f;

				dot = DotProd(zvec, Normalize(tp));
				float yangle = 0.0f;
				if (dot >= 1.0f)
					yangle = 0.0f;
				else yangle = acos(dot);

				float y = yangle / (PI);

				tp.x = x;
				tp.y = y;//tp.z;	
				float d = Length(tp);
				tp.z = d;
				if (d > longestR)
					longestR = d;

				TVMaps.v[i].SetFlag(0);
				TVMaps.v[i].SetInfluence(0.0f);
				TVMaps.v[i].SetP(tp);
				TVMaps.v[i].SetControlID(-1);
				SetTVVertSelected(i, TRUE);
			}
		}

		//now copy our face data over
		Tab<int> faceAcrossSeam;
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			if (GetFaceSelected(i))
			{
				int ct = i;
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				int quad0 = 0;
				int quad3 = 0;
				for (int j = 0; j < pcount; j++)
				{
					int index = TVMaps.f[i]->t[j];
					if (quads[index] == 0) quad0++;
					if (quads[index] == 3) quad3++;
					//find spot in our list
					if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
					{
						index = TVMaps.f[i]->vecs->handles[j * 2];

						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list
//						TVMaps.f[ct]->vecs->handles[j*2] = gverts.d[index].newindex;

						index = TVMaps.f[i]->vecs->handles[j * 2 + 1];
						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list
//						TVMaps.f[ct]->vecs->handles[j*2+1] = gverts.d[index].newindex;

						if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
						{
							index = TVMaps.f[i]->vecs->interiors[j];
							if (quads[index] == 0) quad0++;
							if (quads[index] == 3) quad3++;
							//find spot in our list
//							TVMaps.f[ct]->vecs->interiors[j] = gverts.d[index].newindex;
						}
					}
				}

				if ((quad3 > 0) && (quad0 > 0))
				{
					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (TVMaps.v[index].GetP().x <= 0.25f)
						{
							UVW_TVVertClass tempv = TVMaps.v[index];
							Point3 fp = tempv.GetP();
							fp.x += 1.0f;
							tempv.SetP(fp);
							tempv.SetFlag(0);
							tempv.SetInfluence(0.0f);
							TVMaps.v.Append(1, &tempv, 5000);
							int vct = TVMaps.v.Count() - 1;
							TVMaps.f[ct]->t[j] = vct;
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{


							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j * 2];

							if (TVMaps.v[index].GetP().x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								Point3 fp = tempv.GetP();
								fp.x += 1.0f;
								tempv.SetP(fp);
								tempv.SetFlag(0);
								tempv.SetInfluence(0.0f);
								TVMaps.v.Append(1, &tempv, 5000);
								int vct = TVMaps.v.Count() - 1;
								TVMaps.f[ct]->vecs->handles[j * 2] = vct;
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j * 2 + 1];
							if (TVMaps.v[index].GetP().x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								Point3 fp = tempv.GetP();
								fp.x += 1.0f;
								tempv.SetP(fp);
								tempv.SetFlag(0);
								tempv.SetInfluence(0.0f);
								TVMaps.v.Append(1, &tempv, 5000);
								int vct = TVMaps.v.Count() - 1;
								TVMaps.f[ct]->vecs->handles[j * 2 + 1] = vct;
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (TVMaps.v[index].GetP().x <= 0.25f)
								{
									UVW_TVVertClass tempv = TVMaps.v[index];
									Point3 fp = tempv.GetP();
									fp.x += 1.0f;
									tempv.SetP(fp);
									tempv.SetFlag(0);
									tempv.SetInfluence(0.0f);
									TVMaps.v.Append(1, &tempv, 5000);
									int vct = TVMaps.v.Count() - 1;
									TVMaps.f[ct]->vecs->interiors[j] = vct;
								}
							}

						}
					}
				}
			}
		}

		if (!normalizeMap)
		{
			BitArray processedVerts;
			processedVerts.SetSize(TVMaps.v.Count());
			processedVerts.ClearAll();
			circ = circ * PI * longestR * 2.0f;
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				if (GetFaceSelected(i))
				{
					int pcount = 3;
					pcount = TVMaps.f[i]->count;
					int ct = i;

					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (!processedVerts[index])
						{
							Point3 p = TVMaps.v[index].GetP();
							p.x *= circ;
							p.y *= circ;
							TVMaps.v[index].SetP(p);
							processedVerts.Set(index, TRUE);
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j * 2];
							if (!processedVerts[index])
							{
								Point3 p = TVMaps.v[index].GetP();
								p.x *= circ;
								p.y *= circ;
								TVMaps.v[index].SetP(p);
								processedVerts.Set(index, TRUE);
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j * 2 + 1];
							if (!processedVerts[index])
							{
								Point3 p = TVMaps.v[index].GetP();
								p.x *= circ;
								p.y *= circ;
								TVMaps.v[index].SetP(p);
								processedVerts.Set(index, TRUE);
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (!processedVerts[index])
								{
									Point3 p = TVMaps.v[index].GetP();
									p.x *= circ;
									p.y *= circ;
									TVMaps.v[index].SetP(p);
									processedVerts.Set(index, TRUE);
								}
							}
						}
					}
				}
			}
		}

		//now find the seam
		if (IsTVVertexFilterValid())
		{
			mTVVertexFilter.SetSize(TVMaps.v.Count(), 1);
			mTVVertexFilter.Set((TVMaps.v.Count() - 1));//enable the newly added one!
		}
		mVSel.SetSize(TVMaps.v.Count(), 1);
		SetTVVertSelected(TVMaps.v.Count() - 1, TRUE);
		TVMaps.mSystemLockedFlag.SetSize(TVMaps.v.Count(), 1);
	}
	else if (mapType == CYLINDRICALMAP)
	{
		Tab<int> quads;
		quads.SetCount(TVMaps.v.Count());

		float longestR = 0.0f;

		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			quads[i] = -1;
			if (tempVSel[i])
			{

				Point3 gp = TVMaps.v[i].GetP();//gverts.d[i].p;
				Point3 tp = gp * toGizmoSpace;
				//z our y

				int quad = 0;
				if ((tp.x >= 0) && (tp.y >= 0))
					quad = 0;
				else if ((tp.x < 0) && (tp.y >= 0))
					quad = 1;
				else if ((tp.x < 0) && (tp.y < 0))
					quad = 2;
				else if ((tp.x >= 0) && (tp.y < 0))
					quad = 3;

				quads[i] = quad;

				//find the quad the point is in
				Point3 xvec(1.0f, 0.0f, 0.0f);

				float x = 0.0f;
				Point3 zp = tp;
				zp.z = 0.0f;

				float dot = DotProd(xvec, Normalize(zp));
				float angle = 0.0f;
				if (dot >= 1.0f)
					angle = 0.0f;
				else angle = acos(dot);

				x = angle / (PI*2.0f);
				if (quad > 1)
					x = (0.5f - x) + 0.5f;

				TVMaps.v[i].SetFlag(0);
				TVMaps.v[i].SetInfluence(0.0f);

				Point3 fp;
				fp.x = x;
				fp.y = tp.z;
				float d = Length(zp);
				fp.z = d;

				SetTVVert(t, i, fp);


				if (d > longestR)
					longestR = d;

			}
		}

		//now copy our face data over
		Tab<int> faceAcrossSeam;
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			if (GetFaceSelected(i))
			{
				int ct = i;
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				int quad0 = 0;
				int quad3 = 0;
				for (int j = 0; j < pcount; j++)
				{
					int index = TVMaps.f[i]->t[j];
					if (quads[index] == 0) quad0++;
					if (quads[index] == 3) quad3++;
					if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
					{
						index = TVMaps.f[i]->vecs->handles[j * 2];

						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;

						index = TVMaps.f[i]->vecs->handles[j * 2 + 1];
						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list

						if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
						{
							index = TVMaps.f[i]->vecs->interiors[j];
							if (quads[index] == 0) quad0++;
							if (quads[index] == 3) quad3++;
						}
					}
				}

				if ((quad3 > 0) && (quad0 > 0))
				{
					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (TVMaps.v[index].GetP().x <= 0.25f)
						{
							UVW_TVVertClass tempv = TVMaps.v[index];
							Point3 tpv = tempv.GetP();
							tpv.x += 1.0f;
							tempv.SetP(tpv);
							tempv.SetFlag(0);
							tempv.SetInfluence(0.0f);
							tempv.SetControlID(-1);
							TVMaps.v.Append(1, &tempv, 5000);
							int vct = TVMaps.v.Count() - 1;
							TVMaps.f[ct]->t[j] = vct;
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j * 2];

							if (TVMaps.v[index].GetP().x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								Point3 tpv = tempv.GetP();
								tpv.x += 1.0f;
								tempv.SetP(tpv);
								tempv.SetFlag(0);
								tempv.SetInfluence(0.0f);
								tempv.SetControlID(-1);
								TVMaps.v.Append(1, &tempv, 5000);
								int vct = TVMaps.v.Count() - 1;
								TVMaps.f[ct]->vecs->handles[j * 2] = vct;
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j * 2 + 1];
							if (TVMaps.v[index].GetP().x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								Point3 tpv = tempv.GetP();
								tpv.x += 1.0f;
								tempv.SetP(tpv);
								tempv.SetFlag(0);
								tempv.SetInfluence(0.0f);
								tempv.SetControlID(-1);
								TVMaps.v.Append(1, &tempv, 5000);
								int vct = TVMaps.v.Count() - 1;
								TVMaps.f[ct]->vecs->handles[j * 2 + 1] = vct;
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (TVMaps.v[index].GetP().x <= 0.25f)
								{
									UVW_TVVertClass tempv = TVMaps.v[index];
									Point3 tpv = tempv.GetP();
									tpv.x += 1.0f;
									tempv.SetP(tpv);
									tempv.SetFlag(0);
									tempv.SetInfluence(0.0f);
									tempv.SetControlID(-1);
									TVMaps.v.Append(1, &tempv, 5000);
									int vct = TVMaps.v.Count() - 1;
									TVMaps.f[ct]->vecs->interiors[j] = vct;
								}
							}
						}
					}
				}
			}
		}

		if (!normalizeMap)
		{
			BitArray processedVerts;
			processedVerts.SetSize(GetNumberTVVerts());//TVMaps.v.Count());
			processedVerts.ClearAll();
			circ = circ * PI * longestR * 2.0f;
			for (int i = 0; i < GetNumberFaces(); i++)
			{
				if (GetFaceSelected(i))
				{
					int pcount = 3;
					pcount = GetFaceDegree(i);
					int ct = i;

					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (!processedVerts[index])
						{
							Point3 p = TVMaps.v[index].GetP();
							p.x *= circ;
							TVMaps.v[index].SetP(p);
							processedVerts.Set(index, TRUE);
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j * 2];
							if (!processedVerts[index])
							{
								Point3 p = TVMaps.v[index].GetP();
								p.x *= circ;
								TVMaps.v[index].SetP(p);
								processedVerts.Set(index, TRUE);
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j * 2 + 1];
							if (!processedVerts[index])
							{
								Point3 p = TVMaps.v[index].GetP();
								p.x *= circ;
								TVMaps.v[index].SetP(p);
								processedVerts.Set(index, TRUE);
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (!processedVerts[index])
								{
									Point3 p = TVMaps.v[index].GetP();
									p.x *= circ;
									TVMaps.v[index].SetP(p);
									processedVerts.Set(index, TRUE);
								}
							}
						}
					}
				}
			}
		}

		//now find the seam
		if (IsTVVertexFilterValid())
		{
			mTVVertexFilter.SetSize(TVMaps.v.Count(), 1);
			mTVVertexFilter.Set((TVMaps.v.Count() - 1));//enable the newly added one!
		}
		mVSel.SetSize(TVMaps.v.Count(), 1);
		SetTVVertSelected(TVMaps.v.Count() - 1, TRUE);
		TVMaps.mSystemLockedFlag.SetSize(TVMaps.v.Count(), 1);
	}
	else if ((mapType == PLANARMAP) || (mapType == PELTMAP) || (mapType == BOXMAP))
	{
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			if (tempVSel[i])
			{
				TVMaps.v[i].SetFlag(0);
				TVMaps.v[i].SetInfluence(0.0f);

				Point3 tp = TVMaps.v[i].GetP();
				tp = tp * toGizmoSpace;
				tp.x += 0.5f;
				tp.y += 0.5f;
				tp.z = 0.0f;
				SetTVVert(t, i, tp);
			}
		}
	}

	BuildTVEdges();
	BuildVertexClusterList();

	if (matid >= 0)
		BuildMatIDFilter(matid);
}
