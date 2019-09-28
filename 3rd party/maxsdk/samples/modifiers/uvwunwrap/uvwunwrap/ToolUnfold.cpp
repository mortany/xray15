#include "unwrap.h"
#include "modsres.h"
#include "utilityMethods.h"
#include <helpsys.h>
#include <Util/BailOut.h>

#include "3dsmaxport.h"

//these are just debug globals so I can stuff data into to draw
#ifdef DEBUGMODE
//just some pos tabs to draw bounding volumes
extern Tab<float> jointClusterBoxX;
extern Tab<float> jointClusterBoxY;
extern Tab<float> jointClusterBoxW;
extern Tab<float> jointClusterBoxH;


extern float hitClusterBoxX, hitClusterBoxY, hitClusterBoxW, hitClusterBoxH;

extern int currentCluster, subCluster;

//used to turn off the regular display and only show debug display
extern BOOL drawOnlyBounds;
#endif

Point3*  UnwrapMod::fnGetNormal(int faceIndex, INode *node)
{
	Point3 norm(0.0f, 0.0f, 0.0f);
	MeshTopoData *ld = GetMeshTopoData(node);
	if (ld)
	{
		Tab<Point3> objNormList;
		ld->BuildNormals(objNormList);
		if ((faceIndex >= 0) && (faceIndex < objNormList.Count()))
			norm = objNormList[faceIndex];
	}


	n = norm;
	return &n;
}


Point3*  UnwrapMod::fnGetNormal(int faceIndex)
{
	Point3 norm(0.0f, 0.0f, 0.0f);
	if (mMeshTopoData.Count())
	{
		MeshTopoData *ld = mMeshTopoData[0];
		if (ld)
		{
			Tab<Point3> objNormList;
			ld->BuildNormals(objNormList);
			if ((faceIndex >= 0) && (faceIndex < objNormList.Count()))
				norm = objNormList[faceIndex];
		}
	}


	n = norm;
	return &n;
}



void  UnwrapMod::fnUnfoldSelectedPolygons(int unfoldMethod, BOOL normalize)
{
	// flatten selected polygons
	if (!ip) return;

	theHold.Begin();
	HoldPointsAndFaces();

	Point3 normal(0.0f, 0.0f, 1.0f);

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		ld->HoldFaceSel();
	}

	BOOL bContinue = TRUE;
    MaxSDK::Util::BailOutManager bailoutManager;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		Tab<Point3> mapNormal;
		mapNormal.SetCount(0);
		MeshTopoData *ld = mMeshTopoData[ldID];
		// only restore the current meshTopoData's face selection, clear all the others.
		for (int ldIDPrep = 0; ldIDPrep < mMeshTopoData.Count(); ldIDPrep++)
		{
			MeshTopoData *ldPrep = mMeshTopoData[ldIDPrep];
			if (ld != ldPrep)
				ldPrep->ClearFaceSelection();
			else
				ldPrep->RestoreFaceSel();
		}

		//hold our face selection
		//get our processed list 
		BitArray holdFaces = ld->GetFaceSel();
		BitArray processedFaces = ld->GetFaceSel();
		while (processedFaces.NumberSet())
		{
			//select the first one
			int seed = -1;
			for (int faceID = 0; faceID < processedFaces.GetSize(); faceID++)
			{
				if (processedFaces[faceID])
				{
					seed = faceID;
					faceID = processedFaces.GetSize();
				}
			}
			BitArray faceSel = ld->GetFaceSel();
			faceSel.ClearAll();
			//select the element the first one
			faceSel.Set(seed, TRUE);
			//select it
			ld->SetFaceSel(faceSel);
			SelectGeomElement(ld);
			faceSel = ld->GetFaceSel();

			//			ld->SelectElement(TVFACEMODE,FALSE);
			faceSel &= holdFaces;
			//remove that from our process list
			for (int faceID = 0; faceID < faceSel.GetSize(); faceID++)
			{
				if (faceSel[faceID])
				{
					processedFaces.Set(faceID, FALSE);
				}
			}
			ld->SetFaceSel(faceSel);

			bContinue = BuildCluster(mapNormal, 5.0f, TRUE, TRUE, MeshTopoData::kFaceAngle);
			TSTR statusMessage;

			if (bContinue)
			{
				for (int i = 0; i < clusterList.Count(); i++)
				{
					ld->ClearFaceSelection();
					for (int j = 0; j < clusterList[i]->faces.Count(); j++)
						ld->SetFaceSelected(clusterList[i]->faces[j], TRUE);//	sel.Set(clusterList[i]->faces[j]);
					ld->PlanarMapNoScale(clusterList[i]->normal);

					int per = (i * 100) / clusterList.Count();
					statusMessage.printf(_T("%s %d%%."), GetString(IDS_PW_STATUS_MAPPING), per);
                    UnwrapUtilityMethods::UpdatePrompt(ip, statusMessage.data());
					if (bailoutManager.ShouldBail())
					{
						i = clusterList.Count();
						bContinue = FALSE;
					}
				}

				DebugPrint(_T("Final Vct %d \n"), ld->GetNumberTVVerts());

				if ((bContinue) && (clusterList.Count() > 1))
				{
					Tab<Point3> objNormList;
					ld->BuildNormals(objNormList);

					//remove internal edges
					Tab<int> clusterGroups;
					clusterGroups.SetCount(ld->GetNumberFaces());
					for (int i = 0; i < clusterGroups.Count(); i++)
					{
						clusterGroups[i] = -1;
					}

					for (int i = 0; i < clusterList.Count(); i++)
					{
						for (int j = 0; j < clusterList[i]->faces.Count(); j++)
						{
							int faceIndex = clusterList[i]->faces[j];
							clusterGroups[faceIndex] = i;
						}
					}
					BitArray processedClusters;
					processedClusters.SetSize(clusterList.Count());
					processedClusters.ClearAll();

					Tab<BorderClass> edgesToBeProcessed;

					BOOL done = FALSE;
					processedClusters.Set(0);
					clusterList[0]->newX = 0.0f;
					clusterList[0]->newY = 0.0f;
					//    clusterList[0]->angle = 0.0f;
					for (int i = 0; i < clusterList[0]->borderData.Count(); i++)
					{
						int outerFaceIndex = clusterList[0]->borderData[i].outerFace;
						int connectedClusterIndex = clusterGroups[outerFaceIndex];
						if ((connectedClusterIndex != 0) && (connectedClusterIndex != -1))
						{
							edgesToBeProcessed.Append(1, &clusterList[0]->borderData[i]);
						}
					}

					BitArray seedFaceList;
					seedFaceList.SetSize(clusterGroups.Count());
					seedFaceList.ClearAll();
					for (int i = 0; i < seedFaces.Count(); i++)
					{
						seedFaceList.Set(seedFaces[i]);
					}

					while (!done)
					{
						Tab<int> clustersJustProcessed;
						clustersJustProcessed.ZeroCount();
						done = TRUE;

						int edgeToAlign = -1;
						float angDist = PI * 2;
						if (unfoldMethod == 1)
							angDist = PI * 2;
						else if (unfoldMethod == 2) angDist = 0;
						int i;
						for (i = 0; i < edgesToBeProcessed.Count(); i++)
						{
							int outerFace = edgesToBeProcessed[i].outerFace;
							int connectedClusterIndex = clusterGroups[outerFace];
							if (!processedClusters[connectedClusterIndex])
							{
								int innerFaceIndex = edgesToBeProcessed[i].innerFace;
								int outerFaceIndex = edgesToBeProcessed[i].outerFace;
								//get angle
								Point3 innerNorm, outerNorm;
								innerNorm = objNormList[innerFaceIndex];
								outerNorm = objNormList[outerFaceIndex];
								float dot = DotProd(innerNorm, outerNorm);

								float angle = 0.0f;

								if (dot == -1.0f)
									angle = PI;
								else if (dot >= 1.0f)
									angle = 0.f;
								else angle = acos(dot);

								if (unfoldMethod == 1)
								{
									if (seedFaceList[outerFaceIndex])
										angle = 0.0f;
									if (angle < angDist)
									{
										angDist = angle;
										edgeToAlign = i;
									}
								}

								else if (unfoldMethod == 2)
								{
									if (seedFaceList[outerFaceIndex])
										angle = 180.0f;
									if (angle > angDist)
									{
										angDist = angle;
										edgeToAlign = i;
									}
								}
							}
						}
						if (edgeToAlign != -1)
						{
							int innerFaceIndex = edgesToBeProcessed[edgeToAlign].innerFace;
							int outerFaceIndex = edgesToBeProcessed[edgeToAlign].outerFace;
							int edgeIndex = edgesToBeProcessed[edgeToAlign].edge;

							int connectedClusterIndex = clusterGroups[outerFaceIndex];

							seedFaceList.Set(outerFaceIndex, FALSE);

							processedClusters.Set(connectedClusterIndex);
							clustersJustProcessed.Append(1, &connectedClusterIndex);
							ld->AlignCluster(clusterList, connectedClusterIndex, innerFaceIndex, outerFaceIndex, edgeIndex);
							done = FALSE;
						}

						//build new cluster list
						for (int j = 0; j < clustersJustProcessed.Count(); j++)
						{
							int clusterIndex = clustersJustProcessed[j];
							for (int i = 0; i < clusterList[clusterIndex]->borderData.Count(); i++)
							{
								int outerFaceIndex = clusterList[clusterIndex]->borderData[i].outerFace;
								int connectedClusterIndex = clusterGroups[outerFaceIndex];
								if ((connectedClusterIndex != 0) && (connectedClusterIndex != -1) && (!processedClusters[connectedClusterIndex]))
								{
									edgesToBeProcessed.Append(1, &clusterList[clusterIndex]->borderData[i]);
								}
							}
						}
					}
				}

				ld->ClearSelection(TVVERTMODE);

				for (int i = 0; i < clusterList.Count(); i++)
				{
					MeshTopoData *ld = clusterList[i]->ld;
					ld->UpdateClusterVertices(clusterList);
					for (int j = 0; j < clusterList[i]->faces.Count(); j++)
					{
						int faceIndex = clusterList[i]->faces[j];
						int degree = ld->GetFaceDegree(faceIndex);
						for (int k = 0; k < degree; k++)
						{
							int vertexIndex = ld->GetFaceTVVert(faceIndex, k);//TVMaps.f[faceIndex]->t[k];
							ld->SetTVVertSelected(vertexIndex, TRUE);//vsel.Set(vertexIndex);
						}
					}
				}

				//now weld the verts
				if (normalize)
				{
					NormalizeCluster();
				}
				BOOL weldOnlyShared = FALSE;
				pblock->GetValue(unwrap_weldonlyshared, 0, weldOnlyShared, FOREVER);
				ld->WeldSelectedVerts(0.001f, weldOnlyShared);
			}
			FreeClusterList();
		}
	}

	if (bContinue)
	{
		theHold.Accept(GetString(IDS_PW_PLANARMAP));

		theHold.Suspend();
		fnSyncTVSelection();
		theHold.Resume();
	}
	else
	{
		theHold.Cancel();
	}

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->BuildTVEdges();
		mMeshTopoData[ldID]->RestoreFaceSel();
	}

	theHold.Suspend();
	fnSyncGeomSelection();
	theHold.Resume();

	if (matid != -1) // if we have a matID fileter set we need to rebuild since topology has changed
		SetMatFilters();

	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	InvalidateView();
}

void  UnwrapMod::fnUnfoldSelectedPolygonsNoParams()
{
	fnUnfoldSelectedPolygons(unfoldMethod, unfoldNormalize);
}

void  UnwrapMod::fnUnfoldSelectedPolygonsDialog()
{
	//bring up the dialog
	DialogBoxParam(hInstance,
		MAKEINTRESOURCE(IDD_UNFOLDDIALOG),
		GetCOREInterface()->GetMAXHWnd(),
		//                   hWnd,
		UnwrapUnfoldFloaterDlgProc,
		(LPARAM)this);


}

void  UnwrapMod::SetUnfoldDialogPos()
{
	if (unfoldWindowPos.length != 0)
		SetWindowPlacement(unfoldHWND, &unfoldWindowPos);
}

void  UnwrapMod::SaveUnfoldDialogPos()
{
	unfoldWindowPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(unfoldHWND, &unfoldWindowPos);
}



INT_PTR CALLBACK UnwrapUnfoldFloaterDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam); commented out by sca 10/7/98 -- causing warning since unused.

	switch (msg) {
	case WM_INITDIALOG:
	{

		mod = (UnwrapMod*)lParam;
		UnwrapMod::unfoldHWND = hWnd;

		DLSetWindowLongPtr(hWnd, lParam);
		::SetWindowContextHelpId(hWnd, idh_unwrap_unfoldmap);

		HWND hMethod = GetDlgItem(hWnd, IDC_METHOD_COMBO);
		SendMessage(hMethod, CB_RESETCONTENT, 0, 0);

		SendMessage(hMethod, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_FARTHESTFACE));
		SendMessage(hMethod, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)GetString(IDS_CLOSESTFACE));

		SendMessage(hMethod, CB_SETCURSEL, mod->unfoldMethod, 0L);

		//set normalize cluster
		CheckDlgButton(hWnd, IDC_NORMALIZE_CHECK, mod->unfoldNormalize);
		mod->SetUnfoldDialogPos();
		break;
	}

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_CONTEXTHELP)
		{
			MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_unfoldmap);
		}
		return FALSE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OK:
		{
			mod->SaveUnfoldDialogPos();

			HWND hMethod = GetDlgItem(hWnd, IDC_METHOD_COMBO);
			mod->unfoldMethod = SendMessage(hMethod, CB_GETCURSEL, 0L, 0);
			mod->unfoldNormalize = IsDlgButtonChecked(hWnd, IDC_NORMALIZE_CHECK);

			mod->fnUnfoldSelectedPolygonsNoParams();

			EndDialog(hWnd, 1);
			break;
		}
		case IDC_CANCEL:
		{
			mod->SaveUnfoldDialogPos();
			EndDialog(hWnd, 0);
			break;
		}
		case IDC_DEFAULT:
		{
			//get align
			mod->unfoldNormalize = IsDlgButtonChecked(hWnd, IDC_NORMALIZE_CHECK);
			HWND hMethod = GetDlgItem(hWnd, IDC_METHOD_COMBO);
			mod->unfoldMethod = SendMessage(hMethod, CB_GETCURSEL, 0L, 0);

			mod->fnSetAsDefaults(MAPSECTION);

			break;
		}

		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}


void  UnwrapMod::fnSetSeedFace()
{
	/*FIXPW
		seedFaces.ZeroCount();
		for (int i =0; i < TVMaps.f.Count(); i++)
		{
			if (TVMaps.f[i]->flags & FLAG_SELECTED)
				seedFaces.Append(1,&i);

		}
	*/

}
void UnwrapMod::fnShowVertexConnectionList()
{
	fnSetShowConnection(!fnGetShowConnection());
}


BOOL  UnwrapMod::fnGetShowConnection()
{
	return showVertexClusterList;
}

void  UnwrapMod::fnSetShowConnection(BOOL show)
{
	if (show != showVertexClusterList)
	{
		showVertexClusterList = show;
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		InvalidateView();
	}
}




