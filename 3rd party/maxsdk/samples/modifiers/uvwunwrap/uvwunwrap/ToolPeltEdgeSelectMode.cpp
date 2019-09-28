/*

Copyright 2010 Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/

#include "unwrap.h"
#include "modsres.h"
#include "ToolPeltEdgeSelectMode.h"


void PointToPointMouseProc::ClearPreviewAndRedraw()
{
	if (ClearPreview())
	{
		TimeValue t = GetCOREInterface()->GetTime();
		mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		mod->ip->RedrawViews(t);
	}
}

bool PointToPointMouseProc::ClearPreview()
{
	bool cleaned = false;
	if(mod == nullptr) return cleaned;
	for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData *ld = mod->GetMeshTopoData(ldID);
		if(ld)
		{
			ld->ClearSeamEdgesPreview();
			cleaned = true;
		}
	}
	return cleaned;
}

bool PointToPointMouseProc::RemoveValFromTab(int val, const Tab<int> &inTab, Tab<int> &outTab)
{
	bool exist = false;
	for (int i = 0; i < inTab.Count(); ++i)
	{
		if (inTab[i] == val)
		{
			exist = true;
		}
		else
		{
			outTab.Append(1, &inTab[i], 1);
		}
	}
	return exist;
}

void PointToPointMouseProc::KeepShortestSeam(const Tab<int> &edges, int &minLen, Tab<int> &outputEdges)
{
	if (edges.Count() < minLen || minLen == -1)
	{
		outputEdges = edges;
		minLen = edges.Count();
	}
}

bool PointToPointMouseProc::IsOperatingOpenEdges()
{
	return mod->GetLivePeelModeEnabled();
}

bool PointToPointMouseProc::IsOperatingSeams()
{
	return mod->peltData.PointToPointSelSeams() && (!mod->GetLivePeelModeEnabled());
}


void PointToPointMouseProc::GetRemovableEdges(MeshTopoData *ld, BitArray &removableEdges)
{

	//Point-to-Point Edge Selection routine
	if(IsOperatingOpenEdges()) // In live peel mode, edge-to-seam is auto on
	{
		//Point-to-Point Edges routine and Live Peel Mode on
		removableEdges = ld->GetOpenGeomEdges();
	}
	else
	{
		if(IsOperatingSeams())
		{
			//Point-to-Point Edges routine and Live Peel Mode off and edge-to-seam on
			removableEdges = ld->mSeamEdges;
		}
		else
		{
			// edge selection
			removableEdges = ld->GetGeomEdgeSel();
		}
	}
}

void PointToPointMouseProc::_BuildSeamEdges(MeshTopoData *ld, const GeomHit &hitPrevious, const GeomHit &hitCurrent, bool isRemoval, Tab<int> &outputEdges)
{
	int minLen = -1;
	Tab<int> pointsPrevious;
	Tab<int> pointsCurrent;
	GetPointsFromGeomHit(ld, hitPrevious, pointsPrevious);
	GetPointsFromGeomHit(ld, hitCurrent, pointsCurrent);

	int previousDim = pointsPrevious.Count();
	int currentDim = pointsCurrent.Count();

	Tab<bool> visitedPaths;
	visitedPaths.SetCount(previousDim * currentDim);
	for (int i = 0; i < visitedPaths.Count(); ++i) visitedPaths[i] = false;

	//set candidateEdges only in removal case
	BitArray candidateEdges;
	if (isRemoval)
	{
		GetRemovableEdges(ld, candidateEdges);
	}

	for (int i = 0; i < previousDim; ++i)
	{
		int pPrevious = pointsPrevious[i];
		for (int j = 0; j < currentDim; ++j)
		{
			int pCurrent = pointsCurrent[j];

			int pathIndex = i * currentDim + j;
			if (visitedPaths[pathIndex])
				continue;

			Tab<int> resultEdges;
			bool connected = ld->EdgeListFromPoints(resultEdges, pPrevious, pCurrent, candidateEdges);
			visitedPaths[pathIndex] = true;

			if (!connected)
				continue;
			
			Tab<int> previousExcludedEdges, currentExcludedEdges, bothExcludedEdges;
			bool previousEdgeExist = hitPrevious.mGeomType == HitGeomType_Edge ?
				RemoveValFromTab(hitPrevious.mIndex, resultEdges, previousExcludedEdges) : false;
			bool currentEdgeExist = hitCurrent.mGeomType == HitGeomType_Edge ?
				RemoveValFromTab(hitCurrent.mIndex, resultEdges, currentExcludedEdges) : false;
			bool bothExist = previousEdgeExist && currentEdgeExist ?
				RemoveValFromTab(hitCurrent.mIndex, previousExcludedEdges, bothExcludedEdges) : false;
			
			if (bothExist)
			{
				int oppoIndexA = (1 - i) * currentDim + (1 - j);
				int oppoIndexB = i * currentDim + (1 - j);
				int oppoIndexC = (1 - i) * currentDim + j;
				visitedPaths[oppoIndexA] = true;
				visitedPaths[oppoIndexB] = true;
				visitedPaths[oppoIndexC] = true;
				KeepShortestSeam(bothExcludedEdges, minLen, outputEdges);
			}
			if (previousEdgeExist)
			{
				int oppoIndex = (1 - i) * currentDim + j;
				visitedPaths[oppoIndex] = true;
				KeepShortestSeam(previousExcludedEdges, minLen, outputEdges);
			}
			if (currentEdgeExist)
			{
				int oppoIndex = i * currentDim + (1 - j);
				visitedPaths[oppoIndex] = true;
				KeepShortestSeam(currentExcludedEdges, minLen, outputEdges);
			}
			KeepShortestSeam(resultEdges, minLen, outputEdges);
		}
	}

	//add hit edges themselves
	if (hitPrevious.mGeomType == HitGeomType_Edge)
	{
		int edgeIndex = hitPrevious.mIndex;
		outputEdges.Append(1, &edgeIndex, 1);
	}
	if (hitCurrent.mGeomType == HitGeomType_Edge)
	{
		int edgeIndex = hitCurrent.mIndex;
		if (!isRemoval || candidateEdges[edgeIndex])
		{
			outputEdges.Append(1, &edgeIndex, 1);
		}
	}
}

void PointToPointMouseProc::GetPointsFromGeomHit(MeshTopoData *ld, const GeomHit &hit, Tab<int> &points)
{
	if (!hit.IsValid())
		return;

	switch (hit.mGeomType)
	{
	case HitGeomType_Vertex:
		{
			int point = hit.mIndex;
			points.Append(1, &point, 1);
		}
		break;
	case HitGeomType_Edge:
		{
			int pointA = ld->GetGeomEdgeVert(hit.mIndex, 0);
			int pointB = ld->GetGeomEdgeVert(hit.mIndex, 1);
			points.Append(1, &pointA, 1);
			points.Append(1, &pointB, 1);
		}
		break;
	default:
		break;
	}
}

void PointToPointMouseProc::HitTestSubObject(IPoint2 m, ViewExp& vpt, MouseProcHitTestInfo &hitInfo)
{
	TimeValue t = GetCOREInterface()->GetTime();
	GraphicsWindow *gw = vpt.getGW();
	HitRegion hr;

	int savedLimits = gw->getRndLimits();

	int crossing = TRUE;
	int type = HITTYPE_POINT;
	MakeHitRegion(hr,type, crossing,4,&m);
	gw->setHitRegion(&hr);

	for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
	{			
		MeshTopoData *ld = mod->GetMeshTopoData(ldID);

		INode *inode = mod->GetMeshTopoDataNode(ldID);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);	

		int hitFlags = 0;
		if (mat.Parity())
			hitFlags |= HIT_MIRROREDTM;

		ModContext mc;

		//point & edge hit tests share the same visible faces info
		//use pre-calculated visible faces here to eliminate this redundant calculation
		BitArray visibleFaces;
		gw->setRndLimits(((gw->getRndLimits() | GW_PICK) & ~GW_ILLUM) | GW_BACKCULL);
		mod->BuildVisibleFaces(gw, ld, visibleFaces, hitFlags);
		gw->setRndLimits(savedLimits);

		//hit point
		gw->setRndLimits((gw->getRndLimits() | GW_PICK) & ~GW_ILLUM);
		int hindex = mod->peltData.HitTestPointToPointMode(mod, ld, vpt, gw, &m, hr, inode, &mc, &visibleFaces);
		gw->setRndLimits(savedLimits);

		if (hindex != -1)
		{
			hitInfo.mHit.mGeomType = HitGeomType_Vertex;
			hitInfo.mHit.mIndex = hindex;
		}
		else if (enableEdgeSelection)
		{
			//hit edge
			Tab<UVWHitData> hitDataList;
			BOOL oldBackCull = mod->fnGetBackFaceCull();
			mod->fnSetBackFaceCull(TRUE);

			gw->setRndLimits((gw->getRndLimits() | GW_PICK) & ~GW_ILLUM);
			gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL);
			mod->peltData.HitTestEdgeMode(mod, ld, vpt, gw, hitDataList, hr, inode, &mc, 0, 
				HITTYPE_POINT, &visibleFaces);
			gw->setRndLimits(savedLimits);

			mod->fnSetBackFaceCull(oldBackCull);

			if (hitDataList.Count() >= 1)
			{
				hitInfo.mHit.mGeomType = HitGeomType_Edge;
				hitInfo.mHit.mIndex = hitDataList[0].index;
			}
		}

		if (hitInfo.mHit.IsValid())
		{
			hitInfo.mLD = ld;
			hitInfo.mTM = mat;
			break;
		}
	}

	gw->setRndLimits(savedLimits);
}

void PointToPointMouseProc::SetAnchorAndRetrievePrevious(const MouseProcHitTestInfo &hitInfo, Point3 &previousAnchor)
{
	previousAnchor = mod->peltData.pointToPointAnchorPoint;
	switch (hitInfo.mHit.mGeomType)
	{
	case HitGeomType_Vertex:
		mod->peltData.pointToPointAnchorPoint = hitInfo.mLD->GetGeomVert(hitInfo.mHit.mIndex) * hitInfo.mTM;
		break;
	case HitGeomType_Edge:
		{
			int pointA = hitInfo.mLD->GetGeomEdgeVert(hitInfo.mHit.mIndex, 0);
			int pointB = hitInfo.mLD->GetGeomEdgeVert(hitInfo.mHit.mIndex, 1);
			Point3 posA = hitInfo.mLD->GetGeomVert(pointA);
			Point3 posB = hitInfo.mLD->GetGeomVert(pointB);
			mod->peltData.pointToPointAnchorPoint = (posA + posB) / 2.0f * hitInfo.mTM;
		}
		break;
	default:
		break;
	}
}

void PointToPointMouseProc::ResizeEdgesArrary(BitArray &edges, const MouseProcHitTestInfo &hitInfo)
{
	if (edges.GetSize() != hitInfo.mLD->GetNumberGeomEdges())
	{
		edges.SetSize(hitInfo.mLD->GetNumberGeomEdges());
		edges.ClearAll();
	}
}

void PointToPointMouseProc::SetSeamEdges(const Tab<int> &seamEdges, const MouseProcHitTestInfo &hitInfo, Point3 previousAnchor, int procFlags)
{
	TimeValue t = GetCOREInterface()->GetTime();

	ResizeEdgesArrary(hitInfo.mLD->mSeamEdges, hitInfo);

	if (seamEdges.Count() == 0) return;

	if(IsOperatingOpenEdges())
	{
		theHold.SuperBegin();
	}

	theHold.Begin();
	if(IsOperatingSeams() || mod->GetLivePeelModeEnabled())
	{
		theHold.Put (new UnwrapPeltSeamRestore (mod,hitInfo.mLD,&previousAnchor));
		if(mod->GetLivePeelModeEnabled())
		{
			mod->HoldPointsAndFaces();
		}
	}
	else
	{
		mod->HoldSelection();
	}
	theHold.Accept (GetString (IDS_DS_SELECT));


	bool isRemoval = (procFlags & MOUSE_ALT) != 0;

	if ( IsOperatingOpenEdges() ) // In live peel mode, edge-to-seam is auto on
	{
		//Point-to-Point Seams routine, Live Peel Mode on
		if (isRemoval)
		{
			//trigger lazy sewing to current selection
			hitInfo.mLD->mSeamEdges.ClearAll();
			hitInfo.mLD->SetSewingPending();
		}
	}

	for (int i = 0; i < seamEdges.Count(); i++)
	{
		int edgeIndex = seamEdges[i];
		if(IsOperatingOpenEdges()) // In Live Peel mode, edge-to-seam is auto on
		{
			hitInfo.mLD->mSeamEdges.Set(edgeIndex, TRUE);
		}
		else
		{
			if(IsOperatingSeams())
			{
				BOOL isSet = isRemoval ? FALSE : TRUE;
				hitInfo.mLD->mSeamEdges.Set(edgeIndex, isSet);
			}
			else
			{
				BOOL isSet = isRemoval ? FALSE : TRUE;
				hitInfo.mLD->SetGeomEdgeSelected(edgeIndex, isSet, FALSE);
			}
		}
	}

	mod->SyncTVFromGeomSelection(hitInfo.mLD);
	if(IsOperatingOpenEdges())
	{
		mod->fnLSCMInvalidateTopo(hitInfo.mLD);
		mod->LSCMForceResolve();
		theHold.SuperAccept(GetString(IDS_LIVE_PEEL));
	}
	else
	{
		mod->InvalidateView();
	}

	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->ip->RedrawViews(t);

}

void PointToPointMouseProc::PreviewSeamEdges(const Tab<int> &seamEdges, const MouseProcHitTestInfo &hitInfo)
{
	TimeValue t = GetCOREInterface()->GetTime();

	hitInfo.mLD->ResetSeamEdgesPreview();
	hitInfo.mLD->ClearSeamEdgesPreview();
	int edgeIndex = -1;
	for (int i = 0; i < seamEdges.Count(); i++)
	{
		edgeIndex = seamEdges[i];
		hitInfo.mLD->SetSeamEdgesPreview(edgeIndex, TRUE, FALSE);
	}
	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->ip->RedrawViews(t);
}

void PointToPointMouseProc::RecordMacroPeltSelectedSeams(const MouseProcHitTestInfo &hitInfo)
{
	INode *node = NULL;
	for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
	{
		if (hitInfo.mLD == mod->GetMeshTopoData(ldID))
			node = mod->GetMeshTopoDataNode(ldID);
	}

	if (mod->peltData.PointToPointSelSeams())
	{
		TSTR mstr;
		mstr.printf(_T("$%s.modifiers[#unwrap_uvw].unwrap6.setPeltSelectedSeamsByNode"),node->GetName());
		macroRecorder->FunctionCall(mstr, 2, 0,
			mr_bitarray,&(hitInfo.mLD->mSeamEdges),
			mr_reftarg,node
			);
	}
}

void PointToPointMouseProc::SetCursorByHit(const MouseProcHitTestInfo &hitInfo)
{
	if (hitInfo.IsValid())
	{
		SetCursor(GetCOREInterface()->GetSysCursor(SYSCUR_SELECT));
	}
	else
	{
		SetCursor(LoadCursor(NULL,IDC_ARROW));
	}
}

void PointToPointMouseProc::SetSeam(const MouseProcHitTestInfo &hitInfo, IPoint2 m, int procFlags)
{
	_SetOrPreviewSeam(hitInfo, m, procFlags, true);
}

void PointToPointMouseProc::PreviewSeam(const MouseProcHitTestInfo &hitInfo, IPoint2 m, int procFlags)
{
	_SetOrPreviewSeam(hitInfo, m, procFlags, false);
}

int GetMirrorIndexFromGeomHit(const GeomHit& input, MeshTopoData* pTopo)
{
	if (input.IsValid() && pTopo)
	{
		if (input.mGeomType == HitGeomType_Vertex)
		{
			return pTopo->GetGeomVertMirrorIndex(input.mIndex);
		}
		else
		{
			return pTopo->GetGeomEdgeMirrorIndex(input.mIndex);
		}
	}
	return -1;
}

void PointToPointMouseProc::BuildSeamEdges(const MouseProcHitTestInfo &hitInfo, int procFlags, Tab<int> &seamEdges)
{
	if (hitInfo.IsValid() && hitInfo.mHit != mod->peltData.currentGeomHit)
	{
		bool isSameLD = mod->peltData.mBaseMeshTopoDataCurrent == hitInfo.mLD;
		bool isEdgeHit = hitInfo.mHit.mGeomType == HitGeomType_Edge;

		if (isSameLD || isEdgeHit)
		{
			bool isRemoval = (procFlags & MOUSE_ALT) != 0;
			//GeomHit on different MeshTopoData will be left as empty
			GeomHit optionalPreviousHit;
			if (isSameLD) optionalPreviousHit = mod->peltData.currentGeomHit;

			_BuildSeamEdges(
				hitInfo.mLD,
				optionalPreviousHit,
				hitInfo.mHit,
				isRemoval,
				seamEdges
				);
			if (mod->fnGetMirrorSelectionStatus())
			{
				
				GeomHit mirrorPrevHit(optionalPreviousHit.mGeomType, GetMirrorIndexFromGeomHit(optionalPreviousHit, hitInfo.mLD));
				GeomHit mirrorCurHit(hitInfo.mHit.mGeomType, GetMirrorIndexFromGeomHit(hitInfo.mHit, hitInfo.mLD));

				if (mirrorCurHit.IsValid())
				{
					Tab<int> mirrorSeams;
					_BuildSeamEdges(
						hitInfo.mLD,
						mirrorPrevHit,
						mirrorCurHit,
						isRemoval,
						mirrorSeams
						);
					for (int i = 0; i < mirrorSeams.Count(); ++i)
					{
						seamEdges.Append(1, &mirrorSeams[i]);
					}
				}
			}
		}
	}
}

void PointToPointMouseProc::ReplaceSavedHit(const MouseProcHitTestInfo &hitInfo, IPoint2 m)
{
	mod->peltData.mBaseMeshTopoDataCurrent = hitInfo.mLD;

	mod->peltData.previousGeomHit = mod->peltData.currentGeomHit;
	mod->peltData.currentGeomHit = hitInfo.mHit;

	mod->peltData.lastMouseClick = mod->peltData.currentMouseClick;
	mod->peltData.currentMouseClick = m;
}

void PointToPointMouseProc::_SetOrPreviewSeam(const MouseProcHitTestInfo &hitInfo, IPoint2 m, int procFlags, bool isSet)
{
	if (!hitInfo.IsValid())
	{
		if (!isSet) ClearPreviewAndRedraw();
		return;
	}

	Tab<int> seamEdges;
	BuildSeamEdges(hitInfo, procFlags, seamEdges);

	if (isSet)
	{
		Point3 previousAnchor;
		SetAnchorAndRetrievePrevious(hitInfo, previousAnchor);
		SetSeamEdges(seamEdges, hitInfo, previousAnchor, procFlags);
		ReplaceSavedHit(hitInfo, m);
		RecordMacroPeltSelectedSeams(hitInfo);
	}
	else
	{
		PreviewSeamEdges(seamEdges, hitInfo);
	}
}

void PointToPointMouseProc::DrawAnchorLine(IPoint2 m, ViewExp &vpt, HWND &cancelHWND, IPoint2 *cancelPoints)
{
	GraphicsWindow *gw = vpt.getGW();

	int savedLimits = gw->getRndLimits();
	//draw the xor line
	if (!mod->peltData.currentGeomHit.IsValid()) 
	{
		mod->peltData.basep = m;
	}
	mod->peltData.pp = mod->peltData.cp;
	mod->peltData.cp = m;
	//xor our last line
	//draw our new one

	static IPoint2 lastPoint(0,0);
	static HWND lastHWND;

	Point3 anchorScreen;

	gw->setTransform(Matrix3(1));
	gw->transPoint(&mod->peltData.pointToPointAnchorPoint,&anchorScreen);

	IPoint2 points[3];
	points[0].x = (int) anchorScreen.x;
	points[0].y = (int) anchorScreen.y;

	HWND hWnd = gw->getHWnd();
	if (mod->peltData.currentGeomHit.IsValid()) 
	{
		if (lastPoint == points[0])
		{
			points[1] = mod->peltData.pp;
			XORDottedPolyline(hWnd, 2, points,0,true);

		}
		if (hWnd != lastHWND)
		{
			IPoint2 tpoints[3];
			tpoints[0] = lastPoint;
			tpoints[1] = mod->peltData.pp;
			XORDottedPolyline(lastHWND, 2, tpoints,0,true);
		}

		points[1] = mod->peltData.cp;
		XORDottedPolyline(hWnd, 2, points);
		cancelHWND = hWnd;
		cancelPoints[0] = points[0];
		cancelPoints[1] = points[1];
	}

	lastPoint = points[0];

	lastHWND = hWnd;

	gw->setRndLimits(savedLimits);
}

int PointToPointMouseProc::proc(
                        HWND hwnd, 
                        int msg, 
                        int point, 
                        int flags, 
                        IPoint2 m )
{
	ViewExp& vpt = iObjParams->GetViewExp(hwnd);   
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	
	HitRegion hr;
	int res = TRUE;

	static IPoint2 cancelPoints[3];
	static HWND cancelHWND;

	switch ( msg ) 
	{
		case MOUSE_PROPCLICK:
			//reset start point
			if (mod->peltData.currentGeomHit.IsValid()) 
			{
				//Erase last drawn xor lines
				XORDottedPolyline(cancelHWND, 2, cancelPoints, 0, true);
			}
			else
			{
				mod->peltData.SetPointToPointSeamsMode(mod,FALSE,TRUE);
			}
			
			ClearPreview();

			mod->peltData.currentGeomHit.Reset();
			break;
		case MOUSE_POINT:
			{
				mod->peltData.basep = m;
				MouseProcHitTestInfo hitInfo;
				HitTestSubObject(m, vpt, hitInfo);
				SetSeam(hitInfo, m, flags);
			}
			break;
		case MOUSE_MOVE:
			break;
		case MOUSE_FREEMOVE:
			{
				MouseProcHitTestInfo hitInfo;
				HitTestSubObject(m, vpt, hitInfo);
				SetCursorByHit(hitInfo);
				PreviewSeam(hitInfo, m, flags);
				DrawAnchorLine(m, vpt, cancelHWND, cancelPoints);
			}
			break;
	}

	return res;
}



/*-------------------------------------------------------------------*/
PeltPointToPointMode::PeltPointToPointMode(UnwrapMod* bmod, IObjParam *i) :
	  fgProc(bmod), eproc(bmod,i) 
{
	mod=bmod;
}

void PeltPointToPointMode::EnterMode()
{
	mod->GetUIManager()->UpdateCheckButtons();
}

void PeltPointToPointMode::ExitMode()
{
	mod->GetUIManager()->UpdateCheckButtons();
}
