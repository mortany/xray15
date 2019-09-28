#include "unwrap.h"
#include "modsres.h"
#include "TvConnectionInfo.h"
#include "3dsmaxport.h"
#include <helpsys.h>

void  UnwrapMod::fnStitchVerts(BOOL bAlign, float fBias)
{
	theHold.Begin();
	HoldPointsAndFaces();   	
	theHold.Accept(GetString(IDS_PW_STITCH));

	theHold.Suspend();
	StitchVerts(bAlign, fBias,FALSE);
	theHold.Resume();
}

void  UnwrapMod::fnStitchVerts2(BOOL bAlign, float fBias, BOOL bScale)
{
	StitchVerts(bAlign, fBias,bScale);
}

void UnwrapMod::StitchMeshTopoDataVerts(MeshTopoData* ld, BOOL bAlign, float fBias, BOOL bScale)
{
	if (!ld)
		return;

	BOOL firstVert = ld->GetTVVertSelected(0);

	TimeValue t = GetCOREInterface()->GetTime();
	BitArray initialVertexSelection = ld->GetTVVertSel();

	ld->BuildVertexClusterList();
	//clean up selection
	//remove any verts not on edge
	//exclude dead vertices
	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		//only process selected vertices that have more than one connected verts
		//and that have not been already processed (tprocessedVerts)
		if (ld->GetTVVertSelected(i))
		{
			if (ld->GetVertexClusterSharedData(i) <= 1 || ld->GetTVVertDead(i))
			{
				ld->SetTVVertSelected(i, FALSE);
			}
		}
	}

	//hold our original sel we will need it later
	BitArray orignalVSel(ld->GetTVVertSel());

	//now clear out any that are not on the same element	
	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		//find out first sel vertex
		if (ld->GetTVVertSelected(i))
		{
			ld->ClearTVVertSelection();
			ld->SetTVVertSelected(i, TRUE);
			//now get the element for that vertex
			ld->SelectVertexElement(TRUE);
			//now the selection is just the constrained to the first element
			BitArray tmpArray = ld->GetTVVertSel();
			tmpArray &= orignalVSel;
			ld->SetTVVertSel(tmpArray);

			i = ld->GetNumberTVVerts();
		}
	}

	//if multiple edges selected set it to the largest
	ld->GetEdgeCluster(const_cast<BitArray&>(ld->GetTVVertSel()));

	firstVert = ld->GetTVVertSelected(0);

	//this builds a list of verts connected to the selection
	BitArray oppositeVerts;
	oppositeVerts.SetSize(ld->GetNumberTVVerts());
	oppositeVerts.ClearAll();

	BitArray selectedOppositeVerts;
	selectedOppositeVerts.SetSize(ld->GetNumberTVVerts());
	selectedOppositeVerts.ClearAll();

	if (ld->GetTVVertSel().NumberSet() == 1)
	{
		//get our one vertex
		int vIndex = 0;
		for (int i = 0; i < ld->GetTVVertSel().GetSize(); i++)
		{
			if (ld->GetTVVertSelected(i)) {
				vIndex = i;
				break;
			}
		}

		int clusterId = ld->GetVertexClusterData(vIndex);
		Point3 uvpos = ld->GetTVVert(vIndex);
		float closestDistance = 1000000.0f;
		int matchingVert = -1;
		//Find the closest matching seam vert if there is more than one match
		for (int i = 0; i < ld->GetNumberTVEdges(); i++)
		{
			int a = ld->GetTVEdgeVert(i, 0);
			int b = ld->GetTVEdgeVert(i, 1);
			int potentialVertex = -1;
			if (ld->GetVertexClusterData(a) == clusterId && a != vIndex) potentialVertex = a;
			else if (ld->GetVertexClusterData(b) == clusterId  && b != vIndex) potentialVertex = b;
			if (potentialVertex != -1)
			{
				if (ld->IsTVVertVisible(potentialVertex))
				{
					Point3 uvpos2 = ld->GetTVVert(potentialVertex);
					float dist = (uvpos2 - uvpos).FLength();
					if (dist < closestDistance)
					{
						closestDistance = dist;
						matchingVert = potentialVertex;
					}
				}
			}
		}
		if (matchingVert != -1)
		{
			oppositeVerts.Set(matchingVert);
		}
	}
	else
	{
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)
		{
			//only process selected vertices that have more than one connected verts
			//and that have not been already processed (tprocessedVerts)
			if ((ld->GetVertexClusterSharedData(i) > 1) && ld->GetTVVertSelected(i))
			{
				int clusterId = ld->GetVertexClusterData(i);
				//now loop through all the
				for (int j = 0; j < ld->GetNumberTVVerts(); j++)
				{
					if ((clusterId == ld->GetVertexClusterData(j)) && (j != i))
					{
						if (!ld->GetTVVertSelected(j))
						{
							if (ld->IsTVVertVisible(j))
							{
								oppositeVerts.Set(j);
								//add the vertex if it was part of our original selection and not part of current the vertex selection
								if (orignalVSel[j])
								{
									selectedOppositeVerts.Set(j);
								}
							}
						}
					}
				}
			}
		}
	}

	//if we have vertices that are selected and opposite the vertex selection use those instead
	if (selectedOppositeVerts.NumberSet() > 0)
		oppositeVerts = selectedOppositeVerts;

	firstVert = ld->GetTVVertSelected(0);

	ld->GetEdgeCluster(oppositeVerts);

	BitArray elem, tempArray;
	tempArray.SetSize(ld->GetNumberTVVerts());
	tempArray.ClearAll();

	//clean up mVSel and rmeove any that are not part of the opposite set
	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		if (oppositeVerts[i])
		{
			int clusterId = ld->GetVertexClusterData(i);
			for (int j = 0; j < ld->GetNumberTVVerts(); j++)
			{
				if ((clusterId == ld->GetVertexClusterData(j)) && (j != i) && ld->GetTVVertSelected(j))
					tempArray.Set(j);
			}
		}
	}

	ld->SetTVVertSel(tempArray & ld->GetTVVertSel());

	tempArray.SetSize(ld->GetNumberTVVerts());
	tempArray.ClearAll();
	tempArray = ld->GetTVVertSel();

	//this builds a list of the two elements clusters
	//basically all the verts that are connected to the selection and the opposing verts
	int seedA = -1, seedB = -1;
	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		if (ld->GetTVVertSelected(i) && (seedA == -1))
			seedA = i;
		if ((oppositeVerts[i]) && (seedB == -1))
			seedB = i;
	}

	if (seedA == -1) return;
	if (seedB == -1) return;

	elem.SetSize(ld->GetNumberTVVerts());
	elem.SetSize(ld->GetNumberTVVerts());

	tempArray.SetSize(ld->GetNumberTVVerts());
	tempArray.ClearAll();
	tempArray = ld->GetTVVertSel();

	ld->ClearTVVertSelection();
	ld->SetTVVertSelected(seedA, TRUE);
	ld->SelectVertexElement(TRUE);
	elem = ld->GetTVVertSel();

	firstVert = ld->GetTVVertSelected(0);

	ld->ClearTVVertSelection();
	ld->SetTVVertSelected(seedB, TRUE);
	ld->SelectVertexElement(TRUE);
	elem |= ld->GetTVVertSel();

	firstVert = ld->GetTVVertSelected(0);
	ld->SetTVVertSel(tempArray);

	//now need to clean out any noncontigous verts in our vertex selection
	{
		TVConnectionInfo connectionData(ld);
		int firstIndex = -1;
		for (int i = 0; i < ld->GetNumberTVVerts(); i++)
		{
			if (ld->GetTVVertSelected(i))
			{
				firstIndex = i;
				break;
			}
		}
		//now find all the vertices that are connected to this one
		if (firstIndex != -1)
		{
			Tab<int> vertsToProcess;
			BitArray processedVerts;

			processedVerts.SetSize(ld->GetNumberTVVerts());
			processedVerts.ClearAll();

			vertsToProcess.Append(1, &firstIndex, 1000);
			while (vertsToProcess.Count() != 0)
			{
				//pop the last vertex
				int lastVert = vertsToProcess.Count() - 1;
				int vIndex = vertsToProcess[lastVert];
				vertsToProcess.Delete(lastVert, 1);
				//make sure to mark it 
				processedVerts.Set(vIndex, TRUE);

				//get all the connected verts that have not been processed
				int ct = connectionData.mVertex[vIndex]->mConnectedTo.Count();
				for (int j = 0; j < ct; j++)
				{
					int connectedVert = connectionData.mVertex[vIndex]->mConnectedTo[j].mVert;
					if (connectedVert < 0 || connectedVert > processedVerts.GetSize())
						continue;
					if (!processedVerts[connectedVert])
					{
						if (ld->GetTVVertSelected(connectedVert))
							vertsToProcess.Append(1, &connectedVert, 1000);
					}
				}
			}
			ld->SetTVVertSel(processedVerts);
		}
	}

	int sourcePoints[2];
	int targetPoints[2];

	//align the clusters if they are seperate 
	if (bAlign)
	{
		//check to make sure there are only 2 elements
		int numberOfElements = 2;

		BitArray elem1, elem2, holdArray;

		//build element 1
		elem1.SetSize(ld->GetNumberTVVerts());
		elem2.SetSize(ld->GetNumberTVVerts());
		elem1.ClearAll();
		elem2.ClearAll();

		holdArray.SetSize(ld->GetTVVertSel().GetSize());
		holdArray = ld->GetTVVertSel();

		ld->SelectVertexElement(TRUE);
		firstVert = ld->GetTVVertSelected(0);

		elem1 = ld->GetTVVertSel();

		ld->SetTVVertSel(oppositeVerts);
		elem2 = oppositeVerts;
		ld->SelectVertexElement(TRUE);

		firstVert = ld->GetTVVertSelected(0);

		for (int i = 0; i < ld->GetNumberTVVerts(); i++)
		{
			if (ld->GetTVVertSelected(i) && elem1[i])
			{
				numberOfElements = 0;
				break;
			}
		}

		if (numberOfElements == 2)
		{
			//build a line from selection of element 1
			elem1 = holdArray;

			sourcePoints[0] = -1;
			sourcePoints[1] = -1;
			targetPoints[0] = -1;
			targetPoints[1] = -1;

			int ct = 0;
			Tab<int> connectedCount;
			connectedCount.SetCount(ld->GetNumberTVVerts());

			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				connectedCount[i] = 0;

			//find a center point and then the farthest point from it
			Point3 centerP(0.0f, 0.0f, 0.0f);
			Box3 bounds;
			bounds.Init();
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if ((holdArray[i]) && (ld->GetVertexClusterSharedData(i) > 1))
				{
					bounds += ld->GetTVVert(i);
				}
			}
			centerP = bounds.Center();
			int closestIndex = -1;
			float closestDist = 0.0f;
			centerP.z = 0.0f;

			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if ((holdArray[i]) && (ld->GetVertexClusterSharedData(i) == 2))//watjeFIX
				{
					float dist;
					Point3 p = ld->GetTVVert(i);
					p.z = 0.0f;
					dist = LengthSquared(p - centerP);
					if ((dist > closestDist) || (closestIndex == -1))
					{
						closestDist = dist;
						closestIndex = i;
					}
				}
			}
			sourcePoints[0] = closestIndex;

			closestIndex = -1;
			closestDist = 0.0f;
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if ((holdArray[i]) && (ld->GetVertexClusterSharedData(i) == 2))//watjeFIX
				{
					float dist;
					Point3 p = ld->GetTVVert(i);
					p.z = 0.0f;
					dist = LengthSquared(p - ld->GetTVVert(sourcePoints[0]));
					if (((dist > closestDist) || (closestIndex == -1)) && (i != sourcePoints[0])) //fix 5.1.04
					{
						closestDist = dist;
						closestIndex = i;
					}
				}
			}
			sourcePoints[1] = closestIndex;

			//fix 5.1.04 if our guess fails  pick 2 at random
			if ((sourcePoints[0] == -1) || (sourcePoints[1] == -1))
			{
				int ct = 0;
				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if ((holdArray[i]) && (ld->GetVertexClusterSharedData(i) >= 2))
					{
						sourcePoints[ct] = i;
						ct++;
						if (ct == 2) break;
					}
				}
			}

			// find the matching corresponding points on the opposite edge
			//by matching the 
			ct = 0;
			//check to make sure we found a cluster, an orphaned vertex could trigger this
			if (sourcePoints[0] == -1)
				return;

			int clusterID = ld->GetVertexClusterData(sourcePoints[0]);
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (elem2[i])
				{
					if ((sourcePoints[ct] != i) && (clusterID == ld->GetVertexClusterData(i)))
					{
						targetPoints[ct] = i;
						ct++;
						break;
					}
				}
			}
			//check to make sure we find an opposing cluster
			if (sourcePoints[1] == -1)
				return;
			clusterID = ld->GetVertexClusterData(sourcePoints[1]);
			for (int i = 0; i < ld->GetNumberTVVerts(); i++)
			{
				if (elem2[i])
				{
					if ((sourcePoints[ct] != i) && (clusterID == ld->GetVertexClusterData(i)) && (targetPoints[0] != i)) //watjeFIX
					{
						targetPoints[ct] = i;
						ct++;
						break;
					}
				}
			}

			if ((targetPoints[0] != -1) && (targetPoints[1] != -1) &&
				(sourcePoints[0] != -1) && (sourcePoints[1] != -1))
			{
				//build a line from selection of element 1
				Point3 centerTarget, centerSource, vecTarget, vecSource;

				vecSource = Normalize(ld->GetTVVert(sourcePoints[1]) - ld->GetTVVert(sourcePoints[0]));
				vecSource.z = 0.0f;
				vecSource = Normalize(vecSource);

				//build a line from selection of element 2
				vecTarget = Normalize(ld->GetTVVert(targetPoints[1]) - ld->GetTVVert(targetPoints[0]));
				vecTarget.z = 0.0f;
				vecTarget = Normalize(vecTarget);

				//Point3 targetStart = TVMaps.v[targetPoints[0]].GetP();
				Point3 targetEnd = ld->GetTVVert(targetPoints[0]);// TVMaps.v[targetPoints[0]].GetP();
				//Point3 sourceStart = TVMaps.v[sourcePoints[0]].GetP();
				Point3 sourceEnd = ld->GetTVVert(sourcePoints[0]);// TVMaps.v[sourcePoints[0]].GetP();

				centerSource = (ld->GetTVVert(sourcePoints[0]) + ld->GetTVVert(sourcePoints[1])) / 2.0f;
				centerTarget = (ld->GetTVVert(targetPoints[0]) + ld->GetTVVert(targetPoints[1])) / 2.0f;

				float angle = 0.0f;
				float dist = Length(vecSource - vecTarget*-1.0f);
				if (dist < 0.001f) angle = PI;
				else if (Length(vecSource - vecTarget) < 0.001f)  //fixes 5.1.01 fixes a stitch error
					angle = 0.0f;
				else angle = acos(DotProd(vecSource, vecTarget));

				float x, y, x1, y1;

				Point3 sp, spn;
				sp = vecTarget;
				spn = vecTarget;
				Matrix3 tm(1);
				tm.SetRotateZ(angle);
				sp = sp * tm;

				tm.IdentityMatrix();
				tm.SetRotateZ(-angle);
				spn = spn * tm;

				x = sp.x;
				y = sp.y;

				x1 = spn.x;
				y1 = spn.y;

				if ((fabs(x - vecSource.x) > 0.001f) || (fabs(y - vecSource.y) > 0.001f))
				{
					angle = -angle;
				}
				//now align the elem2
				Point3 delta = centerSource - centerTarget;

				for (int i = 0; i < ld->GetNumberTVVerts(); i++)
				{
					if (ld->GetTVVertSelected(i))
					{
						Point3 p = ld->GetTVVert(i);
						p = p - centerTarget;
						float tx = p.x;
						float ty = p.y;
						p.x = (tx * cos(angle)) - (ty * sin(angle));
						p.y = (tx * sin(angle)) + (ty * cos(angle));
						p += centerSource;

						ld->SetTVVert(t, i, p);
					}
				}
				//find an offset so the elements do not intersect
				//find an offset so the elements do not intersect
				//new scale
				if (bScale)
				{
					//get the center to start point vector
					//scale all the points
					float tLen = Length(centerTarget - targetEnd);
					float sLen = Length(centerSource - sourceEnd);
					float scale = (sLen / tLen);
					Point3 scaleVec = Normalize(targetEnd - centerTarget);

					for (int i = 0; i < ld->GetNumberTVVerts(); i++)
					{
						if (ld->GetTVVertSelected(i))
						{
							Point3 vec = (ld->GetTVVert(i) - centerSource) * scale;
							vec = centerSource + vec;
							ld->SetTVVert(t, i, vec + ((ld->GetTVVert(i) - vec) * fBias));
						}
					}
				}
			}
			//align element 2 to element 1
		}
		ld->SetTVVertSel(holdArray);
	}
	//now loop through the vertex list and process only those that are selected.
	BitArray processedVerts;
	processedVerts.SetSize(ld->GetNumberTVVerts());
	processedVerts.ClearAll();

	Tab<Point3> movePoints;
	movePoints.SetCount(ld->GetNumberTVVerts());
	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
		movePoints[i] = Point3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		//only process selected vertices that have more than one connected verts
		//and that have not been already processed (tprocessedVerts)
		if ((ld->GetVertexClusterSharedData(i) > 1) && (!oppositeVerts[i]) && ld->GetTVVertSelected(i) && (!processedVerts[i]))
		{
			int clusterId = ld->GetVertexClusterData(i);
			//now loop through all the
			Tab<int> slaveVerts;
			processedVerts.Set(i);
			for (int j = 0; j < ld->GetNumberTVVerts(); j++)
			{
				if ((clusterId == ld->GetVertexClusterData(j)) && (j != i) && (oppositeVerts[j]))
				{
					slaveVerts.Append(1, &j);
				}
			}
			Point3 center;
			center = ld->GetTVVert(i);


			if (slaveVerts.Count() >= 2) //watjeFIX
			{
				// find the selected then find the closest
				float dist = -1.0f;
				int closest = 0;
				for (int j = 0; j < slaveVerts.Count(); j++)
				{
					float ldist = Length(ld->GetTVVert(slaveVerts[j]) - ld->GetTVVert(i));
					if ((dist < 0.0f) || (ldist < dist))
					{
						dist = ldist;
						closest = slaveVerts[j];
					}
				}

				processedVerts.Set(closest);
				slaveVerts.SetCount(1);
				slaveVerts[0] = closest;
			}
			else if (slaveVerts.Count() == 1) processedVerts.Set(slaveVerts[0]);

			Tab<Point3> averages;
			averages.SetCount(slaveVerts.Count());

			for (int j = 0; j < slaveVerts.Count(); j++)
			{
				int slaveIndex = slaveVerts[j];
				averages[j] = (ld->GetTVVert(slaveIndex) - center) * fBias;
			}

			Point3 mid(0.0f, 0.0f, 0.0f);
			for (int j = 0; j < slaveVerts.Count(); j++)
			{
				mid += averages[j];
			}
			mid = mid / (float)slaveVerts.Count();

			center += mid;

			for (int j = 0; j < slaveVerts.Count(); j++)
			{
				int slaveIndex = slaveVerts[j];
				movePoints[slaveIndex] = center;
			}
			movePoints[i] = center;
		}
	}

	BitArray oldSel;
	oldSel.SetSize(ld->GetTVVertSel().GetSize());
	oldSel = ld->GetTVVertSel();

	ld->SetTVVertSel(processedVerts);

	RebuildDistCache();

	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		float influ = ld->GetTVVertInfluence(i);
		if ((influ != 0.0f) && (!processedVerts[i]) && (!(ld->GetTVVertDead(i))) && (elem[i]) && !ld->GetTVSystemLock(i))
		{
			//find closest processed vert and use that delta
			int closestIndex = -1;
			float closestDist = 0.0f;
			for (int j = 0; j < ld->GetNumberTVVerts(); j++)
			{
				if (processedVerts[j])
				{
					int vertexCT = ld->GetVertexClusterSharedData(j);
					if ((vertexCT > 1) && (!(ld->GetTVVertDead(j))) && !ld->GetTVSystemLock(j))
					{
						float tdist = LengthSquared(ld->GetTVVert(i) - ld->GetTVVert(j));
						if ((closestIndex == -1) || (tdist < closestDist))
						{
							closestDist = tdist;
							closestIndex = j;
						}
					}
				}
			}
			if (closestIndex == -1)
			{
			}
			else
			{
				Point3 delta = (movePoints[closestIndex] - ld->GetTVVert(closestIndex)) * influ;
				ld->SetTVVert(t, i, ld->GetTVVert(i) + delta);
			}
		}

	}

	for (int i = 0; i < ld->GetNumberTVVerts(); i++)
	{
		if (processedVerts[i])
		{
			ld->SetTVVert(t, i, movePoints[i]);
		}
	}
}

void  UnwrapMod::StitchVerts(BOOL bAlign, float fBias, BOOL bScale)
{
	if (fnGetTVSubMode() == TVEDGEMODE)
	{
		//if we are doing a edge stitch we need at least one open edge to stitch to if not we can bail
		bool validEdgeSelection = false;
		for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
		{
			MeshTopoData *ld = mMeshTopoData[ldID];
			if (ld->GetTVEdgeSel().AnyBitSet())
			{
				for (int j = 0; j < ld->GetNumberTVEdges(); j++)
				{
					if (ld->GetTVEdgeSelected(j))
					{
						//we need at least one open edge to do a stitch
						if (ld->GetTVEdgeNumberTVFaces(j) == 1)
						{
							validEdgeSelection = true;
							j = ld->GetNumberTVEdges();
							ldID = mMeshTopoData.Count();
						}
					}
				}
			}
			
			if(validEdgeSelection)
			{
				BitArray gesel = ld->GetGeomEdgeSel();	
				if (gesel.AnyBitSet())
				{
					for (int j = 0; j < ld->GetNumberGeomEdges(); j++)
					{
						if (gesel[j] &&
							ld->mSeamEdges[j])
						{
							ld->mSeamEdges.Set(j, FALSE);
						}
					}
				}
			}
		}

		if (validEdgeSelection == false)
		{
			//no valid edges so we can giveup
			return;
		}
	}

	//build my connection list first
	TransferSelectionStart();
	int oldSubObjectMode = fnGetTVSubMode();
	fnSetTVSubMode(TVVERTMODE);

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		StitchMeshTopoDataVerts(ld, bAlign, fBias, bScale);
	}

	//now weld the verts
	float tempWeld = weldThreshold;
	weldThreshold = 0.001f;
	WeldSelected(FALSE);
	weldThreshold = tempWeld;

	CleanUpDeadVertices();

	fnSetTVSubMode(oldSubObjectMode);
	TransferSelectionEnd(FALSE,TRUE);

	RebuildDistCache();
	RebuildEdges();
	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(currentTime);
}

void  UnwrapMod::fnStitchVertsNoParams()
{
	theHold.Begin();
	HoldPointsAndFaces();   
	fnStitchVerts2(bStitchAlign,fStitchBias,bStitchScale);
	theHold.Accept(GetString(IDS_PW_STITCH));
	InvalidateView();

}
void  UnwrapMod::fnStitchVertsDialog()
{
	//bring up the dialog
	DialogBoxParam(   hInstance,
		MAKEINTRESOURCE(IDD_STICTHDIALOG),
		GetCOREInterface()->GetMAXHWnd(),
		//                   hWnd,
		UnwrapStitchFloaterDlgProc,
		(LPARAM)this );


}

void  UnwrapMod::SetStitchDialogPos()
{
	if (stitchWindowPos.length != 0) 
		SetWindowPlacement(stitchHWND,&stitchWindowPos);
}

void  UnwrapMod::SaveStitchDialogPos()
{
	stitchWindowPos.length = sizeof(WINDOWPLACEMENT); 
	GetWindowPlacement(stitchHWND,&stitchWindowPos);
}




INT_PTR CALLBACK UnwrapStitchFloaterDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam); commented out by sca 10/7/98 -- causing warning since unused.
	static ISpinnerControl *iBias = NULL;
	static BOOL bAlign = TRUE;
	static BOOL bScale = TRUE;
	static float fBias= 0.0f;
	static float fSoftSel = 0.0f;
	static BitArray sel;
	static BOOL syncGeom = TRUE;
	switch (msg) {
	  case WM_INITDIALOG:


		  mod = (UnwrapMod*)lParam;
		  UnwrapMod::stitchHWND = hWnd;

		  DLSetWindowLongPtr(hWnd, lParam);
		  ::SetWindowContextHelpId(hWnd, idh_unwrap_stitch);

		  syncGeom = mod->fnGetSyncSelectionMode();
		  mod->fnSetSyncSelectionMode(FALSE);

		  //create bias spinner and set value
		  iBias = SetupFloatSpinner(
			  hWnd,IDC_UNWRAP_BIASSPIN,IDC_UNWRAP_BIAS,
			  0.0f,1.0f,mod->fStitchBias);  
		  iBias->SetScale(0.01f);
		  //set align cluster
		  CheckDlgButton(hWnd,IDC_ALIGN_CHECK,mod->bStitchAlign);
		  CheckDlgButton(hWnd,IDC_SCALE_CHECK,mod->bStitchScale);
		  bAlign = mod->bStitchAlign;
		  fBias = mod->fStitchBias;
		  bScale = mod->bStitchScale;

		  //restore window pos
		  mod->SetStitchDialogPos();
		  //start the hold begin
		  if (!theHold.Holding())
		  {
			  theHold.SuperBegin();
			  theHold.Begin();
		  }
		  //hold the points and faces
		  mod->HoldPointsAndFaces();
		  //stitch initial selection
		  mod->fnStitchVerts2(bAlign,fBias,bScale);
		  mod->InvalidateView();

		  break;
	  case WM_SYSCOMMAND:
		  if ((wParam & 0xfff0) == SC_CONTEXTHELP) 
		  {
			  MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_stitch); 
		  }
		  return FALSE;
		  break;
	  case CC_SPINNER_BUTTONDOWN:
		  if (LOWORD(wParam) == IDC_UNWRAP_BIASSPIN) 
		  {
		  }
		  break;


	  case CC_SPINNER_CHANGE:
		  if (LOWORD(wParam) == IDC_UNWRAP_BIASSPIN) 
		  {
			  //get align
			  fBias = iBias->GetFVal();
			  //revert hold
			  theHold.Restore();

			  //call stitch again
			  mod->fnStitchVerts2(bAlign,fBias,bScale);
			  mod->InvalidateView();
		  }
		  break;

	  case WM_CUSTEDIT_ENTER:
	  case CC_SPINNER_BUTTONUP:
		  if ( (LOWORD(wParam) == IDC_UNWRAP_BIAS) || (LOWORD(wParam) == IDC_UNWRAP_BIASSPIN) )
		  {
		  }
		  break;


	  case WM_COMMAND:
		  switch (LOWORD(wParam)) {
	  case IDC_APPLY:
		  {
			  theHold.Accept(GetString(IDS_PW_STITCH));
			  theHold.SuperAccept(GetString(IDS_PW_STITCH));
			  mod->SaveStitchDialogPos();

			  fBias = iBias->GetFVal();
			  mod->fStitchBias = fBias;

			  //get align
			  bAlign = IsDlgButtonChecked(hWnd,IDC_ALIGN_CHECK);
			  mod->bStitchAlign = bAlign;

			  bScale = IsDlgButtonChecked(hWnd,IDC_SCALE_CHECK);
			  mod->bStitchScale = bScale;
			  

			  ReleaseISpinner(iBias);
			  iBias = NULL;


			  mod->fnSetSyncSelectionMode(syncGeom);


			  EndDialog(hWnd,1);

			  break;
		  }
	  case IDC_REVERT:
		  {
			  theHold.Restore();
			  theHold.Cancel();
			  theHold.SuperCancel();

			  mod->SaveStitchDialogPos();
			  ReleaseISpinner(iBias);
			  iBias = NULL;

			  mod->fnSetSyncSelectionMode(syncGeom);

			  EndDialog(hWnd,0);
			  mod->InvalidateView();

			  break;
		  }
	  case IDC_DEFAULT:
		  {
			  //get bias
			  fBias = iBias->GetFVal();
			  mod->fStitchBias = fBias;

			  //get align
			  bAlign = IsDlgButtonChecked(hWnd,IDC_ALIGN_CHECK);
			  mod->bStitchAlign = bAlign;

			  bScale = IsDlgButtonChecked(hWnd,IDC_SCALE_CHECK);
			  mod->bStitchScale = bScale;
			  //set as defaults
			  mod->fnSetAsDefaults(STITCHSECTION);
			  break;
		  }
	  case IDC_ALIGN_CHECK:
		  {
			  //get align
			  bAlign = IsDlgButtonChecked(hWnd,IDC_ALIGN_CHECK);
			  //revert hold
			  theHold.Restore();


			  //call stitch again
			  mod->fnStitchVerts2(bAlign,fBias,bScale);
			  mod->InvalidateView();
			  break;
		  }
	  case IDC_SCALE_CHECK:
		  {
			  //get align
			  bScale = IsDlgButtonChecked(hWnd,IDC_SCALE_CHECK);
			  //revert hold
			  theHold.Restore();

			  //call stitch again
			  mod->fnStitchVerts2(bAlign,fBias,bScale);
			  mod->InvalidateView();
			  break;
		  }

		  }
		  break;

	  default:
		  return FALSE;
	}
	return TRUE;
}



