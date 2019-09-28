 /**********************************************************************  
 *<
	FILE: EditPolyModes.cpp

	DESCRIPTION: Edit Poly Modifier - Command modes

	CREATED BY: Steve Anderson

	HISTORY: created April 2004

 *>	Copyright (c) 2004, All Rights Reserved.
 **********************************************************************/

#include "EPoly.h"
#include "EditPoly.h"
#include "decomp.h"
#include "spline3d.h"
#include "splshape.h"
#include "shape.h"
#include "MouseCursors.h"
#include <imenuman.h>

#define EPSILON .0001f

// Some defines for viewport work...

// This is a standard depth used in this module for mapping screen coordinates to world for mouse operations
#define VIEWPORT_MAPPING_DEPTH float(-200)

// These scale the coordinates for adjusting them with the mouse
#define HORIZONTAL_CHAMFER_TENSION_ADJUST_SCALE 50.0f
#define HORIZONTAL_CHAMFER_SEGMENT_ADJUST_SCALE 10.0f

CommandMode * EditPolyMod::getCommandMode (int mode) {
	switch (mode) {
	case ep_mode_create_vertex: return createVertMode;
	case ep_mode_create_edge: return createEdgeMode;
	case ep_mode_create_face: return createFaceMode;
	case ep_mode_divide_edge: return divideEdgeMode;
	case ep_mode_divide_face: return divideFaceMode;
	case ep_mode_bridge_border: return bridgeBorderMode;
	case ep_mode_bridge_polygon: return bridgePolyMode;
	case ep_mode_bridge_edge: return bridgeEdgeMode;


	case ep_mode_extrude_vertex:
	case ep_mode_extrude_edge:
		return extrudeVEMode;
	case ep_mode_extrude_face: return extrudeMode;
	case ep_mode_chamfer_vertex:
	case ep_mode_chamfer_edge:
		return chamferMode;
	case ep_mode_bevel: return bevelMode;
	case ep_mode_inset_face: return insetMode;
	case ep_mode_outline: return outlineMode;
	case ep_mode_cut: return cutMode;
	case ep_mode_quickslice: return quickSliceMode;
	case ep_mode_weld: return weldMode;
	case ep_mode_hinge_from_edge: return hingeFromEdgeMode;
	case ep_mode_pick_hinge: return pickHingeMode;
	case ep_mode_pick_bridge_1: return pickBridgeTarget1;
	case ep_mode_pick_bridge_2: return pickBridgeTarget2;
	case ep_mode_edit_tri: return editTriMode;
	case ep_mode_turn_edge: return turnEdgeMode;
	case ep_mode_edit_ss: return editSSMode;
	case ep_mode_subobjectpick:  return subobjectPickMode;
	case ep_mode_point_to_point:  return pointToPointMode;
	}
	return NULL;
}

int EditPolyMod::EpModGetCommandMode () {
	if (mSliceMode) return ep_mode_sliceplane;

	if (!ip) return -1;
	CommandMode *currentMode = ip->GetCommandMode ();

	if (currentMode == createVertMode) return ep_mode_create_vertex;
	if (currentMode == createEdgeMode) return ep_mode_create_edge;
	if (currentMode == createFaceMode) return ep_mode_create_face;
	if (currentMode == divideEdgeMode) return ep_mode_divide_edge;
	if (currentMode == divideFaceMode) return ep_mode_divide_face;
	if (currentMode == bridgeBorderMode) return ep_mode_bridge_border;
	if (currentMode == bridgePolyMode) return ep_mode_bridge_polygon;
	if (currentMode == bridgeEdgeMode) return ep_mode_bridge_edge;
	if (currentMode == extrudeVEMode) {
		if (meshSelLevel[selLevel] == MNM_SL_EDGE) return ep_mode_extrude_edge;
		else return ep_mode_extrude_vertex;
	}
	if (currentMode == extrudeMode) return ep_mode_extrude_face;
	if (currentMode == chamferMode) {
		if (meshSelLevel[selLevel] == MNM_SL_EDGE) return ep_mode_chamfer_edge;
		else return ep_mode_chamfer_vertex;
	}
	if (currentMode == bevelMode) return ep_mode_bevel;
	if (currentMode == insetMode) return ep_mode_inset_face;
	if (currentMode == outlineMode) return ep_mode_outline;
	if (currentMode == cutMode) return ep_mode_cut;
	if (currentMode == quickSliceMode) return ep_mode_quickslice;
	if (currentMode == weldMode) return ep_mode_weld;
	if (currentMode == hingeFromEdgeMode) return ep_mode_hinge_from_edge;
	if (currentMode == pickHingeMode) return ep_mode_pick_hinge;
	if (currentMode == pickBridgeTarget1) return ep_mode_pick_bridge_1;
	if (currentMode == pickBridgeTarget2) return ep_mode_pick_bridge_2;
	if (currentMode == editTriMode) return ep_mode_edit_tri;
	if (currentMode == turnEdgeMode) return ep_mode_turn_edge;
	if(currentMode == editSSMode) return ep_mode_edit_ss;
	if (currentMode==subobjectPickMode) return ep_mode_subobjectpick;
	if (currentMode==pointToPointMode) return ep_mode_point_to_point;
	return -1;
}

void EditPolyMod::EpModToggleCommandMode (int mode) {
	if (!ip) return;
	if ((mode == ep_mode_sliceplane) && (selLevel == EPM_SL_OBJECT)) return;

	if (mode == ep_mode_sliceplane) {
		// Special case.
		ip->ClearPickMode();
		if (mSliceMode) ExitSliceMode();
		else {
			EpModCloseOperationDialog ();	// Cannot have slice mode, settings at same time.
			// Exit any EditPoly command mode we may currently be in:
			ExitAllCommandModes (false, false);
			EnterSliceMode();
		}
		return;
	}

	// Otherwise, make sure we're not in Slice mode:
	if (mSliceMode) ExitSliceMode ();

	CommandMode *cmd = getCommandMode (mode);
	if (cmd==NULL) return;
	CommandMode *currentMode = ip->GetCommandMode ();

	switch (mode) {
	case ep_mode_extrude_vertex:
	case ep_mode_chamfer_vertex:
		// Special case - only use DeleteMode to exit mode if in correct SO level,
		// Otherwise use EpModEnterCommandMode to switch SO levels and enter mode again.
		if ((currentMode==cmd) && (selLevel==EPM_SL_VERTEX)) ip->DeleteMode (cmd);
		else {
			EpModCloseOperationDialog ();	// Cannot have command mode, settings at same time.
			EpModEnterCommandMode (mode);
		}
		break;

	case ep_mode_extrude_edge:
	case ep_mode_chamfer_edge:
		// Special case - only use DeleteMode to exit mode if in correct SO level,
		// Otherwise use EpModEnterCommandMode to switch SO levels and enter mode again.
		if ((currentMode==cmd) && (meshSelLevel[selLevel]==MNM_SL_EDGE)) ip->DeleteMode (cmd);
		else {
			EpModCloseOperationDialog ();	// Cannot have command mode, settings at same time.
			EpModEnterCommandMode (mode);
		}
		break;

	case ep_mode_pick_hinge:
	case ep_mode_pick_bridge_1:
	case ep_mode_pick_bridge_2:
		// Special case - we do not want to end our preview or close the dialog,
		// since this command mode is controlled from the dialog.
		if (currentMode == cmd) ip->DeleteMode (cmd);
		else EpModEnterCommandMode (mode);
		break;

	default:
		if (currentMode == cmd) ip->DeleteMode (cmd);
		else {
			EpModCloseOperationDialog ();	// Cannot have command mode, settings at same time.
			EpModEnterCommandMode (mode);
		}
		break;
	}
}

void EditPolyMod::EpModEnterCommandMode (int mode) {
	if (!ip) return;

	// First of all, we don't want to pile up our command modes:
	ExitAllCommandModes (false, false);

	switch (mode) {
	case ep_mode_create_vertex:
		if (selLevel != EPM_SL_VERTEX) ip->SetSubObjectLevel (EPM_SL_VERTEX);
		ip->PushCommandMode(createVertMode);
		break;

	case ep_mode_create_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_EDGE);
		ip->PushCommandMode (createEdgeMode);
		break;

	case ep_mode_create_face:
	
		if (selLevel != EPM_SL_FACE)  ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (createFaceMode);
		break;

	case ep_mode_divide_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_EDGE);
		ip->PushCommandMode (divideEdgeMode);
		break;

	case ep_mode_divide_face:
		if (selLevel < EPM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (divideFaceMode);
		break;

	case ep_mode_bridge_border:
		if (selLevel != EPM_SL_BORDER) ip->SetSubObjectLevel (EPM_SL_BORDER);
		ip->PushCommandMode (bridgeBorderMode);
		break;

	case ep_mode_bridge_polygon:
		if (selLevel != EPM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (bridgePolyMode);
		break;

	case ep_mode_bridge_edge:
		if (selLevel != EPM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_EDGE);
		ip->PushCommandMode (bridgeEdgeMode);
		break;

	case ep_mode_extrude_vertex:
		if (selLevel != EPM_SL_VERTEX) ip->SetSubObjectLevel (EPM_SL_VERTEX);
	
		ip->PushCommandMode (extrudeVEMode);
		break;

	case ep_mode_extrude_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_EDGE);
	
		ip->PushCommandMode (extrudeVEMode);
		break;

	case ep_mode_extrude_face:
		if (selLevel < EPM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (extrudeMode);
		break;

	case ep_mode_chamfer_vertex:
		if (selLevel != EPM_SL_VERTEX) ip->SetSubObjectLevel (EPM_SL_VERTEX);
		ip->PushCommandMode (chamferMode);
		break;

	case ep_mode_chamfer_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_EDGE);
		ip->PushCommandMode (chamferMode);
		break;

	case ep_mode_bevel:
		if (selLevel < EPM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (bevelMode);
		break;
	case ep_mode_edit_ss:
		ip->PushCommandMode (editSSMode);
		break;
	case ep_mode_inset_face:
		if (meshSelLevel[selLevel] != MNM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (insetMode);
		break;

	case ep_mode_outline:
		if (meshSelLevel[selLevel] != MNM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (outlineMode);
		break;

	case ep_mode_cut:
		ip->PushCommandMode (cutMode);
		break;

	case ep_mode_quickslice:
		ip->PushCommandMode (quickSliceMode);
		break;

	case ep_mode_weld:
		if (selLevel > EPM_SL_BORDER) ip->SetSubObjectLevel (EPM_SL_VERTEX);
		if (selLevel == EPM_SL_BORDER) ip->SetSubObjectLevel (EPM_SL_EDGE);
		ip->PushCommandMode (weldMode);
		break;

	case ep_mode_hinge_from_edge:
		if (selLevel != EPM_SL_FACE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (hingeFromEdgeMode);
		break;

	case ep_mode_pick_hinge:
		ip->PushCommandMode (pickHingeMode);
		break;

	case ep_mode_pick_bridge_1:
		ip->PushCommandMode (pickBridgeTarget1);
		break;

	case ep_mode_pick_bridge_2:
		ip->PushCommandMode (pickBridgeTarget2);
		break;

	case ep_mode_edit_tri:
		if (selLevel < EPM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_FACE);
		ip->PushCommandMode (editTriMode);
		break;

	case ep_mode_turn_edge:
		if (selLevel < EPM_SL_EDGE) ip->SetSubObjectLevel (EPM_SL_EDGE);
		ip->PushCommandMode (turnEdgeMode);
		break;
	case ep_mode_subobjectpick:
		ip->PushCommandMode (subobjectPickMode);
		break;
	case ep_mode_point_to_point:
		ip->PushCommandMode (pointToPointMode);
		break;

	}
}

int EditPolyMod::EpModGetPickMode () {
	if (!ip) return -1;
	PickModeCallback *currentMode = ip->GetCurPickMode ();

	if (currentMode == attachPickMode) return ep_mode_attach;
	if (currentMode == mpShapePicker) return ep_mode_pick_shape;

	return -1;
}

void EditPolyMod::EpModEnterPickMode (int mode) {
	if (!ip) return;

	// Make sure we're not in Slice mode:
	if (mSliceMode) ExitSliceMode ();

	switch (mode) {
	case ep_mode_attach:
		ip->SetPickMode (attachPickMode);
		break;
	case ep_mode_pick_shape:
		ip->SetPickMode (mpShapePicker);
		break;
	}
}

//------------Command modes & Mouse procs----------------------

HitRecord *EditPolyPickEdgeMouseProc::HitTestEdges (IPoint2 &m, ViewExp *vpt, float *prop, 
											Point3 *snapPoint) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

  vpt->ClearSubObjHitList();

	if (HitTestResult ()) mpEPoly->SetHitTestResult ();
	if (mpEPoly->GetEPolySelLevel() != EPM_SL_BORDER)
		mpEPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt);
	if (mpEPoly->GetEPolySelLevel() != EPM_SL_BORDER)
		mpEPoly->ClearHitLevelOverride ();
	mpEPoly->ClearHitTestResult ();

	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();
	HitRecord *hr = hitLog.ClosestHit();
	if (!hr) return hr;
	if (!prop) return hr;

	// Find where along this edge we hit
	// Strategy:
	// Get Mouse click, plus viewport z-direction at mouse click, in object space.
	// Then find the direction of the edge in a plane orthogonal to z, and see how far
	// along that edge we are.

	DWORD ee = hr->hitInfo;
	Matrix3 obj2world = hr->nodeRef->GetObjectTM (ip->GetTime ());

	Ray r;
	vpt->MapScreenToWorldRay ((float)m.x, (float)m.y, r);
	if (!snapPoint) snapPoint = &(r.p);
	Point3 Zdir = Normalize (r.dir);

	MNMesh *pMesh = HitTestResult() ? mpEPoly->EpModGetOutputMesh (hr->nodeRef)
		: mpEPoly->EpModGetMesh (hr->nodeRef);
	if (pMesh == NULL) return NULL;

	MNMesh & mm = *pMesh;
	Point3 A = obj2world * mm.v[mm.e[ee].v1].p;
	Point3 B = obj2world * mm.v[mm.e[ee].v2].p;
	Point3 Xdir = B-A;
	Xdir -= DotProd(Xdir, Zdir)*Zdir;
	*prop = DotProd (Xdir, *snapPoint-A) / LengthSquared (Xdir);
	if (*prop<.0001f) *prop=0;
	if (*prop>.9999f) *prop=1;
	return hr;
}

int EditPolyPickEdgeMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	float prop(0.0f);
	Point3 snapPoint(0.0f,0.0f,0.0f);

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestEdges (m, vpt.ToPointer(), &prop, &snapPoint);

	
		if (!hr) break;
		if (!EdgePick (hr, prop)) {
			// False return value indicates exit mode.
			ip->PopCommandMode ();
			return false;
		}
		return true;
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);//|SNAP_SEL_OBJS_ONLY);
		if (HitTestEdges(m,vpt.ToPointer(),NULL,NULL)) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));

		break;
		}
	}

	return TRUE;
}

// --------------------------------------------------------

HitRecord *EditPolyPickFaceMouseProc::HitTestFaces (IPoint2 &m, ViewExp *vpt) {
	
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (SUBHIT_MNFACES);
	if (HitTestResult()) mpEPoly->SetHitTestResult ();
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt);
	mpEPoly->ClearHitTestResult();
	mpEPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();
	HitRecord *hr = hitLog.ClosestHit();
	return hr;
}

void EditPolyPickFaceMouseProc::ProjectHitToFace (IPoint2 &m, ViewExp *vpt,
										  HitRecord *hr, Point3 *snapPoint) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	
	if (!hr) return ;

	// Find subtriangle, barycentric coordinates of hit in face.
	face = hr->hitInfo;
	Matrix3 obj2world = hr->nodeRef->GetObjectTM (ip->GetTime ());

	Ray r;
	vpt->MapScreenToWorldRay ((float)m.x, (float)m.y, r);
	if (!snapPoint) snapPoint = &(r.p);
	Point3 Zdir = Normalize (r.dir);

	// Find an approximate location for the point on the surface we hit:
	// Get the average normal for the face, thus the plane, and intersect.
	Point3 intersect;
	MNMesh *pMesh = HitTestResult() ? mpEPoly->EpModGetOutputMesh () : mpEPoly->EpModGetMesh();
	if (pMesh == NULL) return;

	MNMesh & mm = *pMesh;
	Point3 planeNormal = mm.GetFaceNormal (face, TRUE);
	planeNormal = Normalize (obj2world.VectorTransform (planeNormal));
	float planeOffset=0.0f;
	for (int i=0; i<mm.f[face].deg; i++)
		planeOffset += DotProd (planeNormal, obj2world*mm.v[mm.f[face].vtx[i]].p);
	planeOffset = planeOffset/float(mm.f[face].deg);

	// Now we intersect the snapPoint + r.dir*t line with the
	// DotProd (planeNormal, X) = planeOffset plane.
	float rayPlane = DotProd (r.dir, planeNormal);
	float firstPointOffset = planeOffset - DotProd (planeNormal, *snapPoint);
	if (fabsf(rayPlane) > EPSILON) {
		float amount = firstPointOffset / rayPlane;
		intersect = *snapPoint + amount*r.dir;
	} else {
		intersect = *snapPoint;
	}

	Matrix3 world2obj = Inverse (obj2world);
	intersect = world2obj * intersect;

	mm.FacePointBary (face, intersect, bary);
}

int EditPolyPickFaceMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint(0.0f,0.0f,0.0f);

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestFaces (m, vpt.ToPointer());
		ProjectHitToFace (m, vpt.ToPointer(), hr, &snapPoint);

		if (hr) FacePick ();
		break;
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		if (HitTestFaces(m,vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));

		break;
		}
	}

	return TRUE;
}

// -------------------------------------------------------

HitRecord *EditPolyConnectVertsMouseProc::HitTestVertices (IPoint2 & m, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (SUBHIT_MNVERTS);
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt);
	mpEPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;

	HitLog& hitLog = vpt->GetSubObjHitList();
	HitRecord *bestHit = NULL;

	for (HitRecord *hr = hitLog.First(); hr; hr = hr->Next())
	{
		if (v1>-1)
		{
			// Check that this hit vertex is on the right mesh, and if it's a neighbor of v1.
			if (hr->nodeRef != mpEPoly->EpModGetPrimaryNode()) continue;
         int i;
			for (i=0; i<neighbors.Count(); i++) if (neighbors[i] == hr->hitInfo) break;
			if (i>=neighbors.Count()) continue;
		}
		else
		{
			// can't accept vertices on no faces.
			if (hr->modContext->localData == NULL) continue;
			EditPolyData *pEditData = (EditPolyData *) hr->modContext->localData;
			if (!pEditData->GetMeshOutput()) continue;	// Shouldn't happen.
			if (pEditData->GetMeshOutput()->vfac[hr->hitInfo].Count() == 0) continue;
		}
		if ((bestHit == NULL) || (bestHit->distance>hr->distance)) bestHit = hr;
	}
	return bestHit;
}

void EditPolyConnectVertsMouseProc::DrawDiag (HWND hWnd, const IPoint2 & m, bool clear) {
	if (v1<0) return;


	IPoint2 points[2];
	points[0]  = m1;
	points[1]  = m;
	XORDottedPolyline(hWnd, 2, points, 0, clear);
}

void EditPolyConnectVertsMouseProc::SetV1 (int vv) {
	v1 = vv;
	neighbors.ZeroCount();
	MNMesh *pMesh = mpEPoly->EpModGetOutputMesh ();
	if (!pMesh) return;
	Tab<int> & vf = pMesh->vfac[vv];
	Tab<int> & ve = pMesh->vedg[vv];
	// Add to neighbors all the vertices that share faces (but no edges) with this one:
	int i,j,k;
	for (i=0; i<vf.Count(); i++) {
		MNFace & mf = pMesh->f[vf[i]];
		for (j=0; j<mf.deg; j++) {
			// Do not consider v1 a neighbor:
			if (mf.vtx[j] == v1) continue;

			// Filter out those that share an edge with v1:
			for (k=0; k<ve.Count(); k++) {
				if (pMesh->e[ve[k]].OtherVert (vv) == mf.vtx[j]) break;
			}
			if (k<ve.Count()) continue;

			// Add to neighbor list.
			neighbors.Append (1, &(mf.vtx[j]), 4);
		}
	}
}

int EditPolyConnectVertsMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord *hr = NULL;
	Point3 snapPoint;

	switch (msg) {
	case MOUSE_INIT:
		v1 = v2 = -1;
		neighbors.ZeroCount ();
		break;

	case MOUSE_PROPCLICK:
		DrawDiag (hwnd, lastm, true);	// erase last dotted line.
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestVertices (m, vpt.ToPointer());

		if (!hr) break;
		if (v1<0) {
			mpEPoly->EpModSetPrimaryNode (hr->nodeRef);
			SetV1 (hr->hitInfo);
			m1 = m;
			lastm = m;
			DrawDiag (hwnd, m);
			break;
		}
		// Otherwise, we've found a connection.
		DrawDiag (hwnd, lastm, true);	// erase last dotted line.
		v2 = hr->hitInfo;
		VertConnect ();
		v1 = -1;
		return FALSE;	// Done with this connection.
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		hr=HitTestVertices(m,vpt.ToPointer());
		if (hr!=NULL) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));

		DrawDiag (hwnd, lastm, true);
		DrawDiag (hwnd, m);
		lastm = m;
		break;
		}
	}

	return TRUE;
}

// -------------------------------------------------------

static HCURSOR hCurCreateVert = NULL;

void EditPolyCreateVertCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
	if (but) {
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
}

void EditPolyCreateVertCMode::ExitMode() {
	// Here's where we auto-commit to the "Create" operation.
	if (mpEditPoly->GetPolyOperationID () == ep_op_create)
	{
		theHold.Begin();
		mpEditPoly->EpModCommitUnlessAnimating (TimeValue(0));
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
	if (but) {
		but->SetCheck(FALSE);
		ReleaseICustButton(but);
	}
}

int EditPolyCreateVertMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	if (!hCurCreateVert) hCurCreateVert = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CreateVertex);

	ViewExp &vpt = ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Matrix3 ctm, nodeTM;
	Point3 pt;
	IPoint2 m2;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		ip->SetActiveViewport(hwnd);

		// Get the point in world-space:
		vpt.GetConstructionTM(ctm);
		pt = vpt.SnapPoint (m, m2, &ctm);
		pt = pt * ctm;

		// Put the point into object space:
		// (This chooses a primary node if one is not already chosen.)
		nodeTM = mpEPoly->EpModGetNodeTM (ip->GetTime());
		nodeTM.Invert ();
		pt = pt * nodeTM;

		// Set the operation in the mod and in the localdata, and add the vertex.
		theHold.Begin ();
		mpEPoly->EpModCreateVertex (pt);
		theHold.Accept (GetString (IDS_CREATE_VERTEX));

		mpEPoly->EpModRefreshScreen ();
		break;

	case MOUSE_FREEMOVE:
		SetCursor(hCurCreateVert);
		vpt.SnapPreview(m, m, NULL, SNAP_FORCE_3D_RESULT);
		break;
	}


	return TRUE;
}

//----------------------------------------------------------

void EditPolyCreateEdgeCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyCreateEdgeCMode::ExitMode() {
	// Here's where we auto-commit to the "Create" operation.
	if (mpEditPoly->GetPolyOperationID () == ep_op_create_edge)
	{
		theHold.Begin();
		mpEditPoly->EpModCommitUnlessAnimating (TimeValue(0));
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck (FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyCreateEdgeMouseProc::VertConnect () {
	theHold.Begin();
	mpEPoly->EpModCreateEdge (v1, v2);
	theHold.Accept (GetString (IDS_CREATE_EDGE));
	mpEPoly->EpModRefreshScreen ();
}

//----------------------------------------------------------

void EditPolyCreateFaceCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->SetHitTestResult ();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyCreateFaceCMode::ExitMode() {
	// Here's where we auto-commit to the "Create" operation.
	if (mpEditPoly->GetPolyOperationID () == ep_op_create)
	{
		theHold.Begin();
		mpEditPoly->EpModCommitUnlessAnimating (TimeValue(0));
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck (FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->ClearHitTestResult ();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyCreateFaceCMode::Display (GraphicsWindow *gw) {
	proc.DrawEstablishedFace(gw);
}

// -----------------------------------------
EditPolyCreateFaceMouseProc::EditPolyCreateFaceMouseProc( EPolyMod* in_e, IObjParam* in_i ) 
: CreateFaceMouseProcTemplate(in_i), m_EPoly(in_e) {
	if (!hCurCreateVert) hCurCreateVert = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CreateVertex);
}

bool EditPolyCreateFaceMouseProc::CreateVertex(const Point3& in_worldLocation, int& out_vertex) {
	if (HasPrimaryNode()) {
		m_EPoly->EpModSetPrimaryNode(GetPrimaryNode());
	} else {
		m_EPoly->EpModSetPrimaryNode(0);
	}
	const Matrix3& l_world2Object = Inverse(m_EPoly->EpModGetPrimaryNode()->GetObjectTM(GetInterface()->GetTime()));
	out_vertex = m_EPoly->EpModCreateVertex(in_worldLocation * l_world2Object);
	return out_vertex >= 0;
}

bool EditPolyCreateFaceMouseProc::CreateFace(MaxSDK::Array<int>& in_vertices) {
	if (HasPrimaryNode()) {
		m_EPoly->EpModSetPrimaryNode(GetPrimaryNode());
	} else {
		m_EPoly->EpModSetPrimaryNode(0);
	}
	Tab<int> l_vertices;
	l_vertices.SetCount((int)in_vertices.length());
	for (int i = 0; i < in_vertices.length(); i++) l_vertices[i] = in_vertices[i];
	return m_EPoly->EpModCreateFace(&l_vertices) >= 0;
}

Point3 EditPolyCreateFaceMouseProc::GetVertexWP(int in_vertex)
{
	DbgAssert(m_EPoly->EpModGetPrimaryNode());
	MNMesh *pMesh = m_EPoly->EpModGetOutputMesh (m_EPoly->EpModGetPrimaryNode());
	
	if(pMesh&&in_vertex>=0&&in_vertex< pMesh->VNum())
		return m_EPoly->EpModGetPrimaryNode()->GetObjectTM(GetInterface()->GetTime()) * pMesh->P(in_vertex);
	return Point3(0.0f,0.0f,0.0f);
}

bool EditPolyCreateFaceMouseProc::HitTestVertex(IPoint2 in_mouse, ViewExp& in_viewport, INode*& out_node, int& out_vertex) {

	if ( ! in_viewport.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	in_viewport.ClearSubObjHitList();

	// why SUBHIT_OPENONLY? In an MNMesh, an edge can belong to at most two faces. If two vertices are selected
	// in sequence that specify an edge that belongs to two faces, then a new face containing this subsequence 
	// of vertices cannot be created. Unfortunately, in this situation, EpfnCreateFace will return successfully 
	// but no new face will actually be created. To prevent this odd behaviour, only vertices on edges with less 
	// than two faces can be selected (i.e., vertices on "open" edges can be selected).
	m_EPoly->SetHitLevelOverride(SUBHIT_MNVERTS|SUBHIT_OPENONLY);
	//#error PW sees something wrong here ... need to figure out how this relates to faces only being created on first object
	GetInterface()->SubObHitTest(GetInterface()->GetTime(),HITTYPE_POINT,0, 0, &in_mouse, in_viewport.ToPointer());
	m_EPoly->ClearHitLevelOverride();

	HitRecord* l_bestHit = 0; 
	// Find the closest vertex on the primary node. If the primary node hasn't been specified, 
	// then use the closest hit from all nodes. 
	for (HitRecord *hr = in_viewport.GetSubObjHitList().First(); hr != 0; hr = hr->Next()) {
		if (HasPrimaryNode() && hr->nodeRef != GetPrimaryNode()) {
			continue;
		}
		if (l_bestHit == 0 || l_bestHit->distance > hr->distance) {
			l_bestHit = hr;
		}
	}

	if (l_bestHit != 0) {
		out_vertex = l_bestHit->hitInfo;
		out_node = l_bestHit->nodeRef;
	}
	return l_bestHit != 0;
}

void EditPolyCreateFaceMouseProc::ShowCreateVertexCursor() {
	SetCursor(hCurCreateVert);
}

//-----------------------------------------------------------------------/

BOOL EditPolyAttachPickMode::Filter(INode *node) {
	if (!mpEditPoly) return false;
	if (!ip) return false;
	if (!node) return FALSE;

	// Make sure the node does not depend on us
	Modifier *pMod = mpEditPoly->GetModifier();
	node->BeginDependencyTest();
	pMod->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) return FALSE;

	ObjectState os = node->GetObjectRef()->Eval(ip->GetTime());
	if (os.obj->IsSubClassOf(polyObjectClassID)) return TRUE;
	if (os.obj->CanConvertToType(polyObjectClassID)) return TRUE;
	return FALSE;
}

BOOL EditPolyAttachPickMode::HitTest(IObjParam *ip, HWND hWnd, ViewExp * /*vpt*/, IPoint2 m,int flags) {
	if (!mpEditPoly) return false;
	if (!ip) return false;
	return ip->PickNode(hWnd,m,this) ? TRUE : FALSE;
}

// Note: we always return false, because we always want to be done picking.
BOOL EditPolyAttachPickMode::Pick(IObjParam *ip,ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	if (!mpEditPoly) return false;
	INode *node = vpt->GetClosestHit();
	if (!Filter(node)) return false;

	INode *pMyNode = mpEditPoly->EpModGetPrimaryNode();
	BOOL ret = TRUE;
	if (pMyNode->GetMtl() && node->GetMtl() && (pMyNode->GetMtl() != node->GetMtl())) {
		//the material attach dialog clears pick modes to prevent crash issues
		//so as a hack push the move mode and then call the dialog and the pop the command stack
		//to end up back where we were
		mSuspendUI = true;
		ip->PushStdCommandMode(CID_OBJMOVE);
		ret = DoAttachMatOptionDialog (ip, mpEditPoly);
		ip->PopCommandMode();
		mSuspendUI = false;
	}
	if (!ret) return false;

	theHold.Begin ();
	mpEditPoly->EpModAttach (node, pMyNode, ip->GetTime());
	theHold.Accept (GetString (IDS_ATTACH));
	mpEditPoly->EpModRefreshScreen ();
	return false;
}

void EditPolyAttachPickMode::EnterMode(IObjParam *ip) {
	if (!mpEditPoly) return;
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	if (mSuspendUI == false)
	{
		ICustButton *but = GetICustButton (GetDlgItem (hGeom, IDC_ATTACH));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
}

void EditPolyAttachPickMode::ExitMode(IObjParam *ip) {
	if (!mpEditPoly) return;

	// If we're still in Attach mode, and the popup dialog isn't engaged, leave attach mode:
	if ((mpEditPoly->GetPolyOperationID() == ep_op_attach) && !mpEditPoly->EpModShowingOperationDialog()) {
		theHold.Begin ();
		mpEditPoly->EpModCommitUnlessAnimating (ip->GetTime());
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	if (mSuspendUI == false)
	{
		ICustButton *but = GetICustButton (GetDlgItem (hGeom, IDC_ATTACH));
		but->SetCheck(FALSE);
		ReleaseICustButton(but);
	}
}

// -----------------------------

int EditPolyAttachHitByName::filter(INode *node) {
	if (!node) return FALSE;

	// Make sure the node does not depend on this modifier.
	node->BeginDependencyTest();
	mpEditPoly->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) return FALSE;

	ObjectState os = node->GetObjectRef()->Eval(ip->GetTime());
	if (os.obj->IsSubClassOf(polyObjectClassID)) return TRUE;
	if (os.obj->CanConvertToType(polyObjectClassID)) return TRUE;
	return FALSE;
}

void EditPolyAttachHitByName::proc (INodeTab & nodeTab) {
	INode *pMyNode = mpEditPoly->EpModGetPrimaryNode();
	BOOL ret = TRUE;
	if (pMyNode->GetMtl()) {
      int i;
		for (i=0; i<nodeTab.Count(); i++) {
			if (nodeTab[i]->GetMtl() && (pMyNode->GetMtl()!=nodeTab[i]->GetMtl())) break;
		}
		if (i<nodeTab.Count()) ret = DoAttachMatOptionDialog (ip, mpEditPoly);
	}
	if (!ret) return;

	theHold.Begin();
	mpEditPoly->EpModMultiAttach (nodeTab, pMyNode, ip->GetTime ());
	theHold.Accept (GetString (IDS_ATTACH_LIST));
}

//--- EditPolyShapePickMode ---------------------------------

BOOL EditPolyShapePickMode::HitTest(IObjParam *ip, HWND hWnd, ViewExp * /*vpt*/, IPoint2 m,int flags) {
	if (!mpMod) return false;
	return ip->PickNode(hWnd,m,this) ? TRUE : FALSE;
}

BOOL EditPolyShapePickMode::Pick(IObjParam *ip,ViewExp *vpt) {
	
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	if (!mpMod) return false;
	INode *node = vpt->GetClosestHit();
	if (!Filter(node)) return false;

	Matrix3 world2obj(true);
	ModContextList list;
	INodeTab nodes;
	ip->GetModContexts (list, nodes);
	if (list.Count() > 0) {
		Interval foo(FOREVER);
		Matrix3 obj2world = nodes[0]->GetObjectTM (ip->GetTime(), &foo);
		Matrix3 mctm = *(list[0]->tm);
		world2obj = Inverse (obj2world);
		world2obj = world2obj * mctm;
	}

	bool inPopup = (mpMod->GetPolyOperationID() == ep_op_extrude_along_spline) && mpMod->EpModShowingOperationDialog();

	if (!inPopup) {
		theHold.Begin ();
		mpMod->EpModSetOperation (ep_op_extrude_along_spline);
		mpMod->getParamBlock()->SetValue (epm_extrude_spline_node, ip->GetTime(), node);
		mpMod->getParamBlock()->SetValue (epm_world_to_object_transform, ip->GetTime(), world2obj);
		mpMod->EpModCommitUnlessAnimating (ip->GetTime());
		theHold.Accept (GetString (IDS_EXTRUDE_ALONG_SPLINE));
	} else {
		theHold.Begin ();
		mpMod->getParamBlock()->SetValue (epm_extrude_spline_node, ip->GetTime(), node);
		mpMod->getParamBlock()->SetValue (epm_world_to_object_transform, ip->GetTime(), world2obj);
		theHold.Accept (GetString (IDS_EDITPOLY_PICK_SPLINE));
	}
	return true;
}

void EditPolyShapePickMode::EnterMode(IObjParam *ip) {
	mpMod->RegisterCMChangedForGrip();
	ICustButton *but = NULL;
	HWND hWnd = mpMod->GetDlgHandle (ep_settings);
	if (hWnd && (but = GetICustButton (GetDlgItem (hWnd, IDC_EXTRUDE_PICK_SPLINE))) != NULL) {
		but->SetCheck(true);
		ReleaseICustButton(but);
		but = NULL;
	}

	hWnd = mpMod->GetDlgHandle (ep_subobj);
	if (hWnd && (but = GetICustButton (GetDlgItem (hWnd, IDC_EXTRUDE_ALONG_SPLINE))) != NULL) {
		but->SetCheck(true);
		ReleaseICustButton(but);
		but = NULL;
	}
}

void EditPolyShapePickMode::ExitMode(IObjParam *ip) {
	ICustButton *but = NULL;
	HWND hWnd = mpMod->GetDlgHandle (ep_settings);
	if (hWnd && (but = GetICustButton (GetDlgItem (hWnd, IDC_EXTRUDE_PICK_SPLINE))) != NULL) {
		but->SetCheck(false);
		ReleaseICustButton(but);
		but = NULL;
	}

	hWnd = mpMod->GetDlgHandle (ep_subobj);
	if (hWnd && (but = GetICustButton (GetDlgItem (hWnd, IDC_EXTRUDE_ALONG_SPLINE))) != NULL) {
		but->SetCheck(false);
		ReleaseICustButton(but);
		but = NULL;
	}
	mpMod->UnRegisterCMChangeForGrip();
}

BOOL EditPolyShapePickMode::Filter(INode *node) {
	if (!mpMod) return false;
	if (!ip) return false;
	if (!node) return false;

	// Make sure the node does not depend on us
	Modifier *pMod = mpMod->GetModifier();
	node->BeginDependencyTest();
	pMod->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) return FALSE;

	ObjectState os = node->GetObjectRef()->Eval(ip->GetTime());
	if (os.obj->IsSubClassOf(splineShapeClassID)) return true;
	if (os.obj->CanConvertToType(splineShapeClassID)) return true;
	return false;
}


//----------------------------------------------------------

// Divide edge modifies two faces; creates a new vertex and a new edge.
bool EditPolyDivideEdgeProc::EdgePick (HitRecord *hr, float prop) {
	theHold.Begin ();
	mpEPoly->EpModDivideEdge (hr->hitInfo, prop, hr->nodeRef);
	theHold.Accept (GetString (IDS_INSERT_VERTEX));
	mpEPoly->EpModRefreshScreen ();
	return true;	// false = exit mode when done; true = stay in mode.
}

void EditPolyDivideEdgeCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen();
}

void EditPolyDivideEdgeCMode::ExitMode() {
	if (mpEditPoly->GetPolyOperationID() == ep_op_insert_vertex_edge) {
		// This is where we commit, if we did any face dividing.
		theHold.Begin ();
		mpEditPoly->EpModCommit (mpEditPoly->ip->GetTime(), true, true);
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
		but->SetCheck(FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen();
}

//----------------------------------------------------------

void EditPolyDivideFaceProc::FacePick () {
	theHold.Begin ();
	mpEPoly->EpModDivideFace (face, &bary);
	theHold.Accept (GetString (IDS_INSERT_VERTEX));
	mpEPoly->EpModRefreshScreen ();
}

void EditPolyDivideFaceCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen();
}

void EditPolyDivideFaceCMode::ExitMode() {
	if (mpEditPoly->GetPolyOperationID() == ep_op_insert_vertex_face) {
		// This is where we commit, if we did any face dividing.
		theHold.Begin ();
		mpEditPoly->EpModCommit (mpEditPoly->ip->GetTime(), true, true);
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen();
}

// ------------------------------------------------------

HitRecord *EditPolyBridgeMouseProc::HitTest (IPoint2 & m, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (mHitLevel);
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt);
	mpEPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;

	HitLog& hitLog = vpt->GetSubObjHitList();
	HitRecord *bestHit = NULL;

	for (HitRecord *hr = hitLog.First(); hr; hr = hr->Next())
	{
		if (hit1>-1) {
			// The only requirement we have for the second hit
			// is that it's not the same as the first.
			if (hr->hitInfo == hit1) continue;
		}
		if ((bestHit == NULL) || (bestHit->distance>hr->distance)) bestHit = hr;
	}
	return bestHit;
}

void EditPolyBridgeMouseProc::DrawDiag (HWND hWnd, const IPoint2 & m, bool clear) {
	if (hit1<0) return;

	IPoint2 points[2];
	points[0]  = m1;
	points[1]  = m;
	XORDottedPolyline(hWnd, 2, points, 0, clear);

}

int EditPolyBridgeMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint(0.0f,0.0f,0.0f);

	switch (msg) {
	case MOUSE_INIT:
		hit1 = hit2 = -1;
		break;

	case MOUSE_PROPCLICK:
		DrawDiag (hwnd, lastm, true);	// erase last dotted line.
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		if (point==1) break;

		ip->SetActiveViewport(hwnd);
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTest (m, vpt.ToPointer());

		if (!hr) break;
		if (hit1<0) {
			mpEPoly->EpModSetPrimaryNode (hr->nodeRef);
			hit1 = hr->hitInfo;
			m1 = m;
			lastm = m;
			DrawDiag (hwnd, m);
			break;
		}
		// Otherwise, we've found a connection.
		DrawDiag (hwnd, lastm, true);	// erase last dotted line.
		hit2 = hr->hitInfo;
		Bridge ();
		hit1 = -1;
		return FALSE;	// Done with this connection.
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		hr=HitTest(m,vpt.ToPointer());
		if (hr!=NULL) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));

		DrawDiag (hwnd, lastm, true);
		DrawDiag (hwnd, m);
		lastm = m;
		break;
		}
	}

	return TRUE;
}

void EditPolyBridgeBorderProc::Bridge () {
	theHold.Begin ();
	mpEPoly->EpModBridgeBorders (hit1, hit2);
	mpEPoly->EpModCommitUnlessAnimating (ip->GetTime());
	theHold.Accept (GetString (IDS_BRIDGE_BORDERS));
	mpEPoly->EpModRefreshScreen ();
}

void EditPolyBridgeBorderCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
}

void EditPolyBridgeBorderCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
}

// ------------------------------------------------------
void EditPolyBridgeEdgeProc::Bridge () {
	theHold.Begin ();
	mpEPoly->EpModBridgeEdges (hit1, hit2);
	mpEPoly->EpModCommitUnlessAnimating (ip->GetTime());
	theHold.Accept (GetString (IDS_BRIDGE_EDGES));
	mpEPoly->EpModRefreshScreen ();
}

void EditPolyBridgeEdgeCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
}

void EditPolyBridgeEdgeCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
}


// ------------------------------------------------------

void EditPolyBridgePolygonProc::Bridge () {
	theHold.Begin ();
	mpEPoly->EpModBridgePolygons (hit1, hit2);
	mpEPoly->EpModCommitUnlessAnimating (ip->GetTime());
	theHold.Accept (GetString (IDS_BRIDGE_POLYGONS));
	mpEPoly->EpModRefreshScreen ();
}

void EditPolyBridgePolygonCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
}

void EditPolyBridgePolygonCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
}

// ------------------------------------------------------

void EditPolyExtrudeProc::BeginExtrude (TimeValue t)
{
	if (mInExtrude) return;
	mInExtrude = true;

	theHold.Begin ();
	int type = mpEditPoly->GetPolyOperationID ();
	if (type != ep_op_extrude_face) {
		mpEditPoly->EpModSetOperation (ep_op_extrude_face);
		mStartHeight = 0.0f;

		SuspendAnimate();
		AnimateOff();
		SuspendSetKeyMode(); // AF (5/13/03)
		mpEditPoly->getParamBlock()->SetValue (epm_extrude_face_height, t, 0.0f);
		ResumeSetKeyMode();
		ResumeAnimate ();
	} else {
		mStartHeight = mpEditPoly->getParamBlock()->GetFloat (epm_extrude_face_height, t);
	}
}

void EditPolyExtrudeProc::EndExtrude (TimeValue t, bool accept)
{
	if (!mInExtrude) return;
	mInExtrude = FALSE;

	if (accept)
	{
		mpEditPoly->EpModCommitUnlessAnimating (t);
		theHold.Accept (GetString(IDS_EXTRUDE));

		if (mpEditPoly->getParamBlock()->GetInt(epm_animation_mode)) {
			mpEditPoly->SetHitTestResult (true);
		}
	}
	else theHold.Cancel ();
}

void EditPolyExtrudeProc::DragExtrude (TimeValue t, float value)
{
	if (!mInExtrude) return;
	mpEditPoly->getParamBlock()->SetValue (epm_extrude_face_height, t, value+mStartHeight);
}

int EditPolyExtrudeProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {	
	ViewExp &vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1;
	IPoint2 m2;
	float height;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		if (!point) {
			BeginExtrude (ip->GetTime());
			om = m;
		} else {
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			EndExtrude (ip->GetTime(), true);
		}
		break;

	case MOUSE_MOVE:
		p0 = vpt.MapScreenToView (om,float(-200));
		m2.x = om.x;
		m2.y = m.y;
		p1 = vpt.MapScreenToView (m2,float(-200));
		height = Length (p1-p0);
		if (m.y > om.y) height *= -1.0f;
		DragExtrude (ip->GetTime(), height);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		EndExtrude (ip->GetTime (), false);
		ip->RedrawViews (ip->GetTime(), REDRAW_END);
		break;
	}


	return TRUE;
}

HCURSOR EditPolyExtrudeSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur =UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Extrude);
	return hCur; 
}

void EditPolyExtrudeCMode::EnterMode() {
	if (mpEditPoly->GetPolyOperationID() == ep_op_extrude_face) {
		mpEditPoly->SetHitTestResult (true);
	} else {
		mpEditPoly->SetHitTestResult ();
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	if (but) {
		but->SetCheck (true);
		ReleaseICustButton(but);
	} else {
		DebugPrint (_T("Edit Poly: we're entering Extrude mode, but we can't find the extrude button!\n"));
		DbgAssert (0);
	}
}

void EditPolyExtrudeCMode::ExitMode() {
	mpEditPoly->ClearHitTestResult();

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	if (but) {
		but->SetCheck(FALSE);
		ReleaseICustButton(but);
	} else {
		DebugPrint (_T("Edit Poly: we're exiting Extrude mode, but we can't find the extrude button!\n"));
		DbgAssert (0);
	}
}

// ------------------------------------------------------

void EditPolyExtrudeVEMouseProc::BeginExtrude (TimeValue t)
{
	if (mInExtrude) return;
	mInExtrude = true;

	int type = mpEditPoly->GetPolyOperationID ();
	theHold.Begin ();

	switch (mpEditPoly->GetMNSelLevel()) {
	case MNM_SL_VERTEX:
		if (type != ep_op_extrude_vertex) {
			mpEditPoly->EpModSetOperation (ep_op_extrude_vertex);
			mpEditPoly->getParamBlock()->SetValue (epm_extrude_vertex_height, TimeValue(0), 0.0f);
			mpEditPoly->getParamBlock()->SetValue (epm_extrude_vertex_width, TimeValue(0), 0.0f);
			mStartHeight = 0.0f;
			mStartWidth = 0.0f;
		} else {
			mStartHeight = mpEditPoly->getParamBlock()->GetFloat (epm_extrude_vertex_height);
			mStartWidth = mpEditPoly->getParamBlock()->GetFloat (epm_extrude_vertex_width);
		}
		break;

	case MNM_SL_EDGE:
		if (type != ep_op_extrude_edge) {
			mpEditPoly->EpModSetOperation (ep_op_extrude_edge);
			mpEditPoly->getParamBlock()->SetValue (epm_extrude_edge_height, TimeValue(0), 0.0f);
			mpEditPoly->getParamBlock()->SetValue (epm_extrude_edge_width, TimeValue(0), 0.0f);
			mStartHeight = 0.0f;
			mStartWidth = 0.0f;
		} else {
			mStartHeight = mpEditPoly->getParamBlock()->GetFloat (epm_extrude_edge_height);
			mStartWidth = mpEditPoly->getParamBlock()->GetFloat (epm_extrude_edge_width);
		}
		break;

	default:
		mInExtrude = false;
		theHold.Cancel ();
		break;
	}
}

void EditPolyExtrudeVEMouseProc::EndExtrude (TimeValue t, bool accept)
{
	if (!mInExtrude) return;
	mInExtrude = FALSE;

	if (accept)
	{
		mpEditPoly->EpModCommitUnlessAnimating (ip->GetTime());
		switch (mpEditPoly->GetMNSelLevel()) {
			case MNM_SL_VERTEX:
				theHold.Accept (GetString (IDS_EDITPOLY_EXTRUDE_VERTEX));
				break;
			case MNM_SL_EDGE:
				theHold.Accept (GetString (IDS_EDITPOLY_EXTRUDE_EDGE));
				break;
		}

		if (mpEditPoly->getParamBlock()->GetInt(epm_animation_mode)) {
			mpEditPoly->SetHitTestResult (true);
		}
	}
	else theHold.Cancel ();
}

void EditPolyExtrudeVEMouseProc::DragExtrude (TimeValue t, float height, float width)
{
	if (!mInExtrude) return;

	height += mStartHeight;
	width += mStartWidth;
	if (width<0) width=0;

	switch (mpEditPoly->GetMNSelLevel()) {
	case MNM_SL_VERTEX:
		mpEditPoly->getParamBlock()->SetValue (epm_extrude_vertex_width, ip->GetTime(), width);
		mpEditPoly->getParamBlock()->SetValue (epm_extrude_vertex_height, ip->GetTime(), height);
		break;
	case MNM_SL_EDGE:
		mpEditPoly->getParamBlock()->SetValue (epm_extrude_edge_width, ip->GetTime(), width);
		mpEditPoly->getParamBlock()->SetValue (epm_extrude_edge_height, ip->GetTime(), height);
		break;
	}
}

int EditPolyExtrudeVEMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	ViewExp &vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1, p2;
	IPoint2 m1, m2;
	float width, height;

	switch (msg) {
	case MOUSE_PROPCLICK:
		if (mInExtrude) EndExtrude (ip->GetTime(), false);
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch (point) {
		case 0:
			m0 = m;
			BeginExtrude (ip->GetTime());
			break;
		case 1:
			EndExtrude (ip->GetTime(), true);
			ip->RedrawViews(ip->GetTime());
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!mInExtrude) break;
		p0 = vpt.MapScreenToView (m0, float(-200));
		m1.x = m.x;
		m1.y = m0.y;
		p1 = vpt.MapScreenToView (m1, float(-200));
		m2.x = m0.x;
		m2.y = m.y;
		p2 = vpt.MapScreenToView (m2, float(-200));
		width = Length (p0 - p1);
		height = Length (p0 - p2);
		if (m.y > m0.y) height *= -1.0f;
		if ((mStartWidth>0) && (m.x < m0.x)) width = -width;

		DragExtrude (ip->GetTime(), height, width);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		if (mInExtrude) EndExtrude (ip->GetTime(), false);
		ip->RedrawViews (ip->GetTime(), REDRAW_END);
		break;
	}


	return TRUE;
}

HCURSOR EditPolyExtrudeVESelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if (!hCur) {
		if (mpEPoly->GetMNSelLevel() == MNM_SL_EDGE) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::EdgeExt);
		else hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ExtrudeVertex);
	}
	return hCur; 
}

void EditPolyExtrudeVECMode::EnterMode() {
	if ((mpEditPoly->GetPolyOperationID() == ep_op_extrude_vertex) ||
		(mpEditPoly->GetPolyOperationID() == ep_op_extrude_edge)) {
		mpEditPoly->SetHitTestResult (true);
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void EditPolyExtrudeVECMode::ExitMode() {
	mpEditPoly->ClearHitTestResult();

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

void EditPolyBevelMouseProc::Begin (TimeValue t)
{
	if (mInBevel) return;
	mInBevel = true;

	theHold.Begin ();
	int type = mpEditPoly->GetPolyOperationID ();
	if (type != ep_op_bevel) {
		mStartHeight = 0.0f;
		mStartOutline = 0.0f;
		mpEditPoly->EpModSetOperation (ep_op_bevel);

		SuspendAnimate();
		AnimateOff();
		SuspendSetKeyMode(); // AF (5/13/03)
		mpEditPoly->getParamBlock()->SetValue (epm_bevel_height, TimeValue(0), 0.0f);
		mpEditPoly->getParamBlock()->SetValue (epm_bevel_outline, TimeValue(0), 0.0f);
		ResumeSetKeyMode();
		ResumeAnimate ();
	} else {
		mStartHeight = mpEditPoly->getParamBlock()->GetFloat (epm_bevel_height, t);
		mStartOutline = mpEditPoly->getParamBlock()->GetFloat (epm_bevel_outline, t);
	}
}

void EditPolyBevelMouseProc::End (TimeValue t, bool accept)
{
	if (!mInBevel) return;
	mInBevel = FALSE;

	if (accept)
	{
		mpEditPoly->EpModCommitUnlessAnimating (t);
		theHold.Accept (GetString(IDS_BEVEL));

		if (mpEditPoly->getParamBlock()->GetInt(epm_animation_mode)) {
			mpEditPoly->SetHitTestResult (true);
		}
	}
	else theHold.Cancel ();
}

void EditPolyBevelMouseProc::DragHeight (TimeValue t, float height)
{
	if (!mInBevel) return;
	mpEditPoly->getParamBlock()->SetValue (epm_bevel_height, t, height+mStartHeight);
}

void EditPolyBevelMouseProc::DragOutline (TimeValue t, float outline)
{
	if (!mInBevel) return;
	mpEditPoly->getParamBlock()->SetValue (epm_bevel_outline, t, outline+mStartOutline);
}

int EditPolyBevelMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {	
	
	ViewExp &vpt=ip->GetViewExp (hwnd);

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	Point3 p0, p1;
	IPoint2 m2;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch (point) {
		case 0:
			m0 = m;
			m0set = TRUE;
			// Flag ignored in Edit Poly.
			Begin (ip->GetTime());
			break;
		case 1:
			m1 = m;
			m1set = TRUE;
			p0 = vpt.MapScreenToView (m0, float(-200));
			m2.x = m0.x;
			m2.y = m.y;
			p1 = vpt.MapScreenToView (m2, float(-200));
			height = Length (p0-p1);
			if (m1.y > m0.y) height *= -1.0f;
			DragHeight (ip->GetTime(), height);
			break;
		case 2:
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			End (ip->GetTime(), true);
			m1set = m0set = FALSE;
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!m0set) break;
		m2.y = m.y;
		if (!m1set) {
			p0 = vpt.MapScreenToView (m0, float(-200));
			m2.x = m0.x;
			p1 = vpt.MapScreenToView (m2, float(-200));
			height = Length (p1-p0);
			if (m.y > m0.y) height *= -1.0f;
			DragHeight (ip->GetTime(), height);
		} else {
			p0 = vpt.MapScreenToView (m1, float(-200));
			m2.x = m1.x;
			p1 = vpt.MapScreenToView (m2, float(-200));
			float outline = Length (p1-p0);
			if (m.y > m1.y) outline *= -1.0f;
			DragOutline (ip->GetTime(), outline);
		}

		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		End (ip->GetTime(), false);
		ip->RedrawViews(ip->GetTime(),REDRAW_END);
		m1set = m0set = FALSE;
		break;
	}


	return TRUE;
}

HCURSOR EditPolyBevelSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Bevel);
	return hCur;
}

void EditPolyBevelCMode::EnterMode() {
	if (mpEditPoly->GetPolyOperationID() == ep_op_bevel) {
		mpEditPoly->SetHitTestResult (true);
	} else {
		mpEditPoly->SetHitTestResult ();
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BEVEL));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void EditPolyBevelCMode::ExitMode() {
	mpEditPoly->ClearHitTestResult ();

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BEVEL));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}







// ------------------------------------------------------

void EditPolyInsetMouseProc::Begin (TimeValue t)
{
	if (mInInset) return;
	mInInset = true;

	theHold.Begin ();
	int type = mpEditPoly->GetPolyOperationID ();
	if (type != ep_op_inset) {
		mpEditPoly->EpModSetOperation (ep_op_inset);
		mStartAmount = 0.0f;

		SuspendAnimate();
		AnimateOff();
		SuspendSetKeyMode(); // AF (5/13/03)
		mpEditPoly->getParamBlock()->SetValue (epm_inset, t, 0.0f);
		ResumeSetKeyMode();
		ResumeAnimate ();
	} else {
		mStartAmount = mpEditPoly->getParamBlock()->GetFloat (epm_inset, t);
	}
}

void EditPolyInsetMouseProc::End (TimeValue t, bool accept)
{
	if (!mInInset) return;
	mInInset = FALSE;

	if (accept)
	{
		mpEditPoly->EpModCommitUnlessAnimating (t);
		theHold.Accept (GetString(IDS_INSET));

		if (mpEditPoly->getParamBlock()->GetInt(epm_animation_mode)) {
			mpEditPoly->SetHitTestResult (true);
		}
	}
	else theHold.Cancel ();
}

void EditPolyInsetMouseProc::Drag (TimeValue t, float amount)
{
	if (!mInInset) return;
	amount += mStartAmount;
	if (amount<0) amount=0;
	mpEditPoly->getParamBlock()->SetValue (epm_inset, t, amount);
}

int EditPolyInsetMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	ViewExp &vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1;
	IPoint2 m2;
	ISpinnerControl *spin=NULL;
	float inset;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch (point) {
		case 0:
			m0 = m;
			m0set = TRUE;
			// Flag ignored in Edit Poly.
			Begin (ip->GetTime());
			break;
		case 1:
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			End (ip->GetTime(), true);
			m0set = FALSE;
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!m0set) break;
		p0 = vpt.MapScreenToView (m0,float(-200));
		m2 = IPoint2(m0.x, m.y);
		p1 = vpt.MapScreenToView (m2,float(-200));
		inset = Length (p1-p0);
		if ((m.y > m0.y) && (mStartAmount>0)) inset *= -1.0f;
		Drag (ip->GetTime(), inset);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		End (ip->GetTime(), false);
		ip->RedrawViews(ip->GetTime(),REDRAW_END);
		m0set = FALSE;
		break;
	}


	return TRUE;
}

HCURSOR EditPolyInsetSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Inset);
	return hCur;
}

void EditPolyInsetCMode::EnterMode() {
	if (mpEditPoly->GetPolyOperationID() == ep_op_inset) {
		mpEditPoly->SetHitTestResult (true);
	} else {
		mpEditPoly->SetHitTestResult ();
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSET));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void EditPolyInsetCMode::ExitMode() {
	mpEditPoly->ClearHitTestResult ();

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSET));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

void EditPolyOutlineMouseProc::Begin (TimeValue t)
{
	if (mInOutline) return;
	mInOutline = true;

	theHold.Begin ();
	int type = mpEditPoly->GetPolyOperationID ();
	if (type != ep_op_outline) {
		mpEditPoly->EpModSetOperation (ep_op_outline);
		mStartAmount = 0.0f;

		SuspendAnimate();
		AnimateOff();
		SuspendSetKeyMode(); // AF (5/13/03)
		mpEditPoly->getParamBlock()->SetValue (epm_outline, t, 0.0f);
		ResumeSetKeyMode();
		ResumeAnimate ();
	} else {
		mStartAmount = mpEditPoly->getParamBlock()->GetFloat (epm_outline, t);
	}
}

void EditPolyOutlineMouseProc::Accept (TimeValue t)
{
	if (!mInOutline) return;
	mInOutline = false;

	mpEditPoly->EpModCommitUnlessAnimating (t);
	theHold.Accept (GetString(IDS_OUTLINE));

	if (mpEditPoly->getParamBlock()->GetInt(epm_animation_mode))
		mpEditPoly->SetHitTestResult (true);

	mpEditPoly->EpModRefreshScreen();
}

void EditPolyOutlineMouseProc::Cancel () {
	if (!mInOutline) return;
	mInOutline = false;
	theHold.Cancel ();
	mpEditPoly->EpModRefreshScreen();
}

void EditPolyOutlineMouseProc::Drag (TimeValue t, float amount)
{
	if (!mInOutline) return;
	mpEditPoly->getParamBlock()->SetValue (epm_outline, t, amount+mStartAmount);
}

int EditPolyOutlineMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	ViewExp &vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1;
	IPoint2 m1;
	ISpinnerControl *spin=NULL;
	float outline;

	switch (msg) {
	case MOUSE_PROPCLICK:
		if (mInOutline) {
			Cancel ();
			ip->RedrawViews (ip->GetTime(), REDRAW_END);
		}
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch (point) {
		case 0:
			m0 = m;
			Begin (ip->GetTime());
			ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
			break;
		case 1:
			Accept (ip->GetTime());
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!mInOutline) break;

		// Get signed right/left distance from original point:
		p0 = vpt.MapScreenToView (m0, float(-200));
		m1.y = m.y;
		m1.x = m0.x;
		p1 = vpt.MapScreenToView (m1, float(-200));
		outline = Length (p1-p0);
		if (m.y > m0.y) outline *= -1.0f;

		Drag (ip->GetTime(), outline);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		Cancel ();
		ip->RedrawViews(ip->GetTime(),REDRAW_END);
		break;
	}


	return TRUE;
}

HCURSOR EditPolyOutlineSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Outline);
	return hCur;
}

void EditPolyOutlineCMode::EnterMode() {
	if ((mpEditPoly->GetPolyOperationID() == ep_op_outline)) {
		mpEditPoly->SetHitTestResult (true);
	} else {
		mpEditPoly->SetHitTestResult ();
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_OUTLINE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void EditPolyOutlineCMode::ExitMode() {
	mpEditPoly->ClearHitTestResult ();

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_OUTLINE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

void EditPolyChamferMouseProc::Begin (TimeValue t, int meshSelLevel)
{
	if (mInChamfer) return;
	mInChamfer = true;

	mChamferOperation = (meshSelLevel == MNM_SL_EDGE) ? ep_op_chamfer_edge : ep_op_chamfer_vertex;

	theHold.Begin ();
	int type = mpEditPoly->GetPolyOperationID ();
	if (type != mChamferOperation) {
		mpEditPoly->EpModSetOperation (mChamferOperation);
		mStartAmount = 0.0f;

		SuspendAnimate();
		AnimateOff();
		SuspendSetKeyMode(); // AF (5/13/03)
		if (mChamferOperation==ep_op_chamfer_edge)
			mpEditPoly->getParamBlock()->SetValue (epm_chamfer_edge, t, 0.0f);
		else
			mpEditPoly->getParamBlock()->SetValue (epm_chamfer_vertex, t, 0.0f);
		ResumeSetKeyMode();
		ResumeAnimate ();
	} else {
		if (mChamferOperation==ep_op_chamfer_edge)
			mStartAmount = mpEditPoly->getParamBlock()->GetFloat (epm_chamfer_edge, t);
		else
			mStartAmount = mpEditPoly->getParamBlock()->GetFloat (epm_chamfer_vertex, t);
	}
}

void EditPolyChamferMouseProc::End (TimeValue t, bool accept)
{
	if (!mInChamfer) return;
	mInChamfer = FALSE;

	if (accept)
	{
		mpEditPoly->EpModCommitUnlessAnimating (t);
		theHold.Accept (GetString(IDS_CHAMFER));

		if (mpEditPoly->getParamBlock()->GetInt(epm_animation_mode)) {
			mpEditPoly->SetHitTestResult (true);
		}
	}
	else theHold.Cancel ();
}

void EditPolyChamferMouseProc::Drag (TimeValue t, float amount, int segments, float tension)
{
	if (!mInChamfer) return;

	if (mChamferOperation==ep_op_chamfer_edge) {
		int chamferType;
		mpEditPoly->getParamBlock()->GetValue (epm_edge_chamfer_type, ip->GetTime(), chamferType, FOREVER);
		mpEditPoly->getParamBlock()->SetValue (epm_chamfer_edge, t, amount);
		if(chamferType == EP_QUAD_EDGE_CHAMFER) {
			mpEditPoly->getParamBlock()->SetValue (epm_edge_chamfer_segments, t, segments);
			mpEditPoly->getParamBlock()->SetValue (epm_edge_chamfer_tension, t, tension);
		}
	}
	else mpEditPoly->getParamBlock()->SetValue (epm_chamfer_vertex, t, amount);
}

int EditPolyChamferMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {	
	ViewExp &vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1;
	IPoint2 m2;
	static float chamfer = 0.0f;
	static float baseTension = 1.0f;
	static float tension = 1.0f;
	static int baseSegments = 1;
	static int segments = 1;
	static IPoint2 omHold, mHold, m1;
	static IPoint2 cumDelta, delta;
	static bool wasAltPressed = false;
	static int cumSegDelta = 0, segDelta = 0;
	static float cumTensDelta = 0.0f, tensDelta = 0.0f;

	// Find out what kind of chamfer we're performing
	int chamferType = EP_STANDARD_EDGE_CHAMFER;
	int selLevel = mpEditPoly->GetMNSelLevel();
	if(selLevel == MNM_SL_EDGE)
		mpEditPoly->getParamBlock()->GetValue (epm_edge_chamfer_type, ip->GetTime(), chamferType, FOREVER);

	// Look for the Alt key
	bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) ? true : false;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch(point) {
		case 0:
			Begin (ip->GetTime(), mpEditPoly->GetMNSelLevel());
			chamfer = 0;
			if(chamferType == EP_QUAD_EDGE_CHAMFER) {
				mpEditPoly->getParamBlock()->GetValue (epm_edge_chamfer_tension, ip->GetTime(), baseTension, FOREVER);
				mpEditPoly->getParamBlock()->GetValue (epm_edge_chamfer_segments, ip->GetTime(), baseSegments, FOREVER);
			}
			om = m;
			cumDelta = IPoint2(0,0);
			segDelta = cumSegDelta = 0;
			tensDelta = cumTensDelta = 0.0f;
			wasAltPressed = false;
			break;
		case 1:		// Sets the chamfer amount
			om = m;
			cumDelta = IPoint2(0,0);
			if(chamferType == EP_STANDARD_EDGE_CHAMFER) {
				End (ip->GetTime(), true);
				ip->RedrawViews (ip->GetTime(),REDRAW_END);
				return FALSE;
			}
			break;
		case 2:		// Sets the tension
			wasAltPressed = false;
			End (ip->GetTime(), true);
			ip->RedrawViews (ip->GetTime(),REDRAW_END);
			return FALSE;
			break;
		}
		break;

	case MOUSE_MOVE:
		// If doing quad chamfer and the ALT key is pressed, we go into handling to alter the number of segments by moving mouse L/R
		if(altPressed && chamferType == EP_QUAD_EDGE_CHAMFER) {
			if(!wasAltPressed) {
				wasAltPressed = true;
				omHold = om;			// Remember starting point for regular ops
				mHold = m;				// Remember where they stopped the regular interaction
				om = m;					// Make current point our starting location
				if(point == 2) {
					// Maintain cumulative tension delta
					cumTensDelta += tensDelta;
					tensDelta = 0.0f;
				}
			}
			p0 = vpt.MapScreenToView(om, VIEWPORT_MAPPING_DEPTH);
			m1.x = m.x;
			m1.y = om.y;
			p1 = vpt.MapScreenToView(m1, VIEWPORT_MAPPING_DEPTH);
			segDelta = (int)(Length (p1 - p0) / HORIZONTAL_CHAMFER_SEGMENT_ADJUST_SCALE);
			if (m.x < om.x)
				segDelta = -segDelta;
		}
		else {
			switch(point) {
			case 1:
				if(wasAltPressed) {
					wasAltPressed = false;
					// Maintain cumulative segment delta
					cumSegDelta += segDelta;
					segDelta = 0;
					// Reset so length doesn't 'pop'
					cumDelta += (mHold - m);
					om = omHold;
				}
				p0 = vpt.MapScreenToView(om, VIEWPORT_MAPPING_DEPTH);
				p1 = vpt.MapScreenToView(m + cumDelta, VIEWPORT_MAPPING_DEPTH);
				chamfer = Length(p1-p0);
				break;
			case 2:
				if(wasAltPressed) {
					wasAltPressed = false;
					// Maintain cumulative segment delta
					cumSegDelta += segDelta;
					segDelta = 0;
					// Reset so length doesn't 'pop'
					om = m;
				}
				p0 = vpt.MapScreenToView(om, VIEWPORT_MAPPING_DEPTH);
				m1.x = m.x;
				m1.y = om.y;
				p1 = vpt.MapScreenToView(m1, VIEWPORT_MAPPING_DEPTH);
				tensDelta = Length (p1 - p0) / HORIZONTAL_CHAMFER_TENSION_ADJUST_SCALE;
				if (m.x < om.x)
					tensDelta = -tensDelta;
				break;
			}
		}
		// Update tension and segments for all cases when in quad chamfer mode
		if(chamferType == EP_QUAD_EDGE_CHAMFER) {
			tension = baseTension + cumTensDelta + tensDelta;
			if(tension < 0.0f)
				tension = 0.0f;
			else
			if(tension > 1.0f)
				tension = 1.0f;
			segments = baseSegments + cumSegDelta + segDelta;
			if(segments < 1)
				segments = 1;
		}
		Drag (ip->GetTime(), chamfer, segments, tension);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		End (ip->GetTime(), false);			
		ip->RedrawViews (ip->GetTime(),REDRAW_END);
		return FALSE;
		break;
	}


	return TRUE;
}

HCURSOR EditPolyChamferSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hECur = NULL;
	static HCURSOR hVCur = NULL;
	if (mpEditPoly->GetEPolySelLevel() == EPM_SL_VERTEX) {
		if ( !hVCur ) hVCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ChamferV);
		return hVCur;
	}
	if ( !hECur ) hECur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ChamferE);
	return hECur;
}

void EditPolyChamferCMode::EnterMode() {
	if ((mpEditPoly->GetPolyOperationID() == ep_op_chamfer_vertex) ||
		(mpEditPoly->GetPolyOperationID() == ep_op_chamfer_edge)) {
		mpEditPoly->SetHitTestResult (true);
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hGeom) {
		ICustButton *but = GetICustButton(GetDlgItem(hGeom,IDC_CHAMFER));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}

	eproc.mInChamfer = false;
}

void EditPolyChamferCMode::ExitMode() {
	mpEditPoly->ClearHitTestResult ();

	if (eproc.mInChamfer) eproc.End (TimeValue(0), false);

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton(GetDlgItem(hGeom,IDC_CHAMFER));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// -- Cut proc/mode -------------------------------------

HitRecord *EditPolyCutProc::HitTestVerts (IPoint2 &m, ViewExp *vpt, bool completeAnalysis) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (SUBHIT_MNVERTS);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();

	// Ok, if we haven't yet started a cut, and we don't want to know the hit location,
	// we're done:
	if ((mStartIndex<0) && !completeAnalysis) return hitLog.First();

	// Ok, now we need to find the closest eligible point:
	// First we grab our node's transform from the first hit:
	mObj2world = hitLog.First()->nodeRef->GetObjectTM(ip->GetTime());

	int bestHitIndex = -1;
	int bestHitDistance = 0;
	Point3 bestHitPoint(0.0f,0.0f,0.0f);
	HitRecord *ret = NULL;
	MNMesh* pMesh = NULL;

	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Always want the closest hit pixelwise:
		if ((bestHitIndex>-1) && (bestHitDistance < hitRec->distance)) continue;

		if (mStartIndex >= 0)
		{
			// Check that our hit doesn't touch starting component,
			// and that it's on the right mesh.
			if (hitRec->nodeRef != mpEPoly->EpModGetPrimaryNode ()) continue;
			pMesh = mpEPoly->EpModGetOutputMesh ();
			if (!pMesh) continue;
			switch (mStartLevel) {
			case MNM_SL_VERTEX:
				if (mStartIndex == int(hitRec->hitInfo)) continue;
				break;
			case MNM_SL_EDGE:
				if (pMesh->e[mStartIndex].v1 == hitRec->hitInfo) continue;
				if (pMesh->e[mStartIndex].v2 == hitRec->hitInfo) continue;
				break;
			case MNM_SL_FACE:
				// Any face is suitable.
				break;
			}
		} else pMesh = mpEPoly->EpModGetOutputMesh (hitRec->nodeRef);
		if (!pMesh) continue;

		Point3 & p = pMesh->P(hitRec->hitInfo);

		if (bestHitIndex>-1)
		{
			Point3 diff = p - bestHitPoint;
			diff = mObj2world.VectorTransform(diff);
			if (DotProd (diff, mLastHitDirection) > 0) continue;	// this vertex clearly further away.
		}

		bestHitIndex = hitRec->hitInfo;
		bestHitDistance = hitRec->distance;
		bestHitPoint = p;
		ret = hitRec;
	}

	if (bestHitIndex>-1)
	{
		mLastHitLevel = MNM_SL_VERTEX;
		mLastHitIndex = bestHitIndex;
		mLastHitPoint = bestHitPoint;
	}

	return ret;
}

HitRecord *EditPolyCutProc::HitTestEdges (IPoint2 &m, ViewExp *vpt, bool completeAnalysis) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return NULL;
	}
	vpt->ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEPoly->ClearHitLevelOverride ();
	int numHits = vpt->NumSubObjHits();
	if (numHits == 0) return NULL;

	HitLog& hitLog = vpt->GetSubObjHitList();

	// Ok, if we haven't yet started a cut, and we don't want to know the actual hit location,
	// we're done:
	if ((mStartIndex<0) && !completeAnalysis) return hitLog.First ();

	// Ok, now we want the edge with the closest hit point.
	// So we form a list of all the edges and their hit points.

	// First we grab our node's transform from the first hit:
	mObj2world = hitLog.First()->nodeRef->GetObjectTM(ip->GetTime());

	int bestHitIndex = -1;
	int bestHitDistance = 0;
	Point3 bestHitPoint(0.0f,0.0f,0.0f);
	HitRecord *ret = NULL;

	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Always want the closest hit pixelwise:
		if ((bestHitIndex>-1) && (bestHitDistance < hitRec->distance)) continue;

		MNMesh* pMesh = NULL;
		if (mStartIndex>-1) 	{
			// Check that our hit doesn't touch starting component,
			// and that it's on the right mesh.
			if (hitRec->nodeRef != mpEPoly->EpModGetPrimaryNode ()) continue;
			pMesh = mpEPoly->EpModGetOutputMesh ();
			if (!pMesh) continue;
			switch (mStartLevel) {
			case MNM_SL_VERTEX:
				if (pMesh->e[hitRec->hitInfo].v1 == mStartIndex) continue;
				if (pMesh->e[hitRec->hitInfo].v2 == mStartIndex) continue;
				break;
			case MNM_SL_EDGE:
				if (mStartIndex == int(hitRec->hitInfo)) continue;
				break;
				// (any face is acceptable)
			}
		} else pMesh = mpEPoly->EpModGetOutputMesh (hitRec->nodeRef);
		if (!pMesh) continue;

		// Get endpoints in world space:
		Point3 Aobj = pMesh->P(pMesh->e[hitRec->hitInfo].v1);
		Point3 Bobj = pMesh->P(pMesh->e[hitRec->hitInfo].v2);
		Point3 A = mObj2world * Aobj;
		Point3 B = mObj2world * Bobj;

		// Find proportion of our nominal hit point along this edge:
		Point3 Xdir = B-A;
		Xdir -= DotProd(Xdir, mLastHitDirection)*mLastHitDirection;	// make orthogonal to mLastHitDirection.
		float prop = DotProd (Xdir, mLastHitPoint-A) / LengthSquared (Xdir);
		if (prop<.0001f) prop=0;
		if (prop>.9999f) prop=1;

		// Find hit point in object space:
		Point3 p = Aobj*(1.0f-prop) + Bobj*prop;

		if (bestHitIndex>-1) {
			Point3 diff = p - bestHitPoint;
			diff = mObj2world.VectorTransform(diff);
			if (DotProd (diff, mLastHitDirection)>0) continue; // this edge clearly further away.
		}
		bestHitIndex = hitRec->hitInfo;
		bestHitDistance = hitRec->distance;
		bestHitPoint = p;
		ret = hitRec;
	}

	if (bestHitIndex>-1)
	{
		mLastHitLevel = MNM_SL_EDGE;
		mLastHitIndex = bestHitIndex;
		mLastHitPoint = bestHitPoint;
	}

	return ret;
}

HitRecord *EditPolyCutProc::HitTestFaces (IPoint2 &m, ViewExp *vpt, bool completeAnalysis) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return NULL;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (SUBHIT_MNFACES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();

	// We don't bother to choose the view-direction-closest face,
	// since generally that's the only one we expect to get.
	if (!completeAnalysis) return hitLog.First();

	// Find the closest hit on the right mesh:
	Point3 bestHitPoint;
	HitRecord *ret = NULL;
	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Always want the closest hit pixelwise:
		if ((ret!=NULL) && (ret->distance < hitRec->distance)) continue;

		if (mStartIndex>-1) 	{
			// Check that the hit is on the right mesh.
			if (hitRec->nodeRef != mpEPoly->EpModGetPrimaryNode ()) continue;
		}
		ret = hitRec;
	}
	if (ret == NULL) return ret;

	mLastHitIndex = ret->hitInfo;
	mObj2world = ret->nodeRef->GetObjectTM (ip->GetTime ());
	
	// Get the average normal for the face, thus the plane, and intersect.
	MNMesh *pMesh = mpEPoly->EpModGetOutputMesh(ret->nodeRef);
	if (!pMesh) return NULL;
	Point3 planeNormal = pMesh->GetFaceNormal (mLastHitIndex, TRUE);
	planeNormal = Normalize (mObj2world.VectorTransform (planeNormal));
	float planeOffset=0.0f;
	for (int i=0; i<pMesh->f[mLastHitIndex].deg; i++)
		planeOffset += DotProd (planeNormal, mObj2world*pMesh->v[pMesh->f[mLastHitIndex].vtx[i]].p);
	planeOffset = planeOffset/float(pMesh->f[mLastHitIndex].deg);

	// Now we intersect the mLastHitPoint + mLastHitDirection*t line with the
	// DotProd (planeNormal, X) = planeOffset plane.
	float rayPlane = DotProd (mLastHitDirection, planeNormal);
	float firstPointOffset = planeOffset - DotProd (planeNormal, mLastHitPoint);
	if (fabsf(rayPlane) > EPSILON) {
		float amount = firstPointOffset / rayPlane;
		mLastHitPoint += amount*mLastHitDirection;
	}

	// Put hitPoint in object space:
	Matrix3 world2obj = Inverse (mObj2world);
	mLastHitPoint = world2obj * mLastHitPoint;
	mLastHitLevel = MNM_SL_FACE;

	return ret;
}

HitRecord *EditPolyCutProc::HitTestAll (IPoint2 & m, ViewExp *vpt, int flags, bool completeAnalysis) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return NULL;
	}

	HitRecord *hr = NULL;
	mLastHitLevel = MNM_SL_OBJECT;	// no hit.

	mpEPoly->ForceIgnoreBackfacing (true);
	mpEPoly->SetHitTestResult ();

	if (!(flags & (MOUSE_SHIFT|MOUSE_CTRL))) {
		hr = HitTestVerts (m,vpt, completeAnalysis);
		if (hr) mLastHitLevel = MNM_SL_VERTEX;
	}
	if (!hr && !(flags & MOUSE_SHIFT)) {
		hr = HitTestEdges (m, vpt, completeAnalysis);
		if (hr) mLastHitLevel = MNM_SL_EDGE;
	}
	if (!hr) {
		hr = HitTestFaces (m, vpt, completeAnalysis);
		if (hr) mLastHitLevel = MNM_SL_FACE;
	}

	mpEPoly->ClearHitTestResult();
	mpEPoly->ForceIgnoreBackfacing (false);

	return hr;
}

void EditPolyCutProc::DrawCutter (HWND hWnd, bool clear) {
	if (mStartIndex<0) return;


	IPoint2 points[2];
	points[0]  = mMouse1;
	points[1]  = mMouse2;
	XORDottedPolyline(hWnd, 2, points, 0, clear);

}

void EditPolyCutProc::Start ()
{
	mStartIndex = mLastHitIndex;
	mStartLevel = mLastHitLevel;

	if (ip->GetSnapState() || true) {
		// Must suspend "fully interactive" while snapping in Cut mode.
		mSuspendPreview = true;
		//ip->PushPrompt (GetString (IDS_CUT_SNAP_PREVIEW_WARNING));
		mStartPoint = mLastHitPoint;
	} else {
		mSuspendPreview = false;
		theHold.Begin ();
		mpEPoly->EpModCut (mStartLevel, mStartIndex, mLastHitPoint, -mLastHitDirection);
	}
}

void EditPolyCutProc::Cancel ()
{
	if (mStartIndex<0) return;

	if (mSuspendPreview) {
		//ip->PopPrompt ();
	} else {
		theHold.Cancel ();
		ip->RedrawViews (ip->GetTime());
	}
	mpEPoly->EpModClearLastCutEnd ();

	mStartIndex = -1;
}

void EditPolyCutProc::Update ()
{
	if ((mStartIndex<0) || mSuspendPreview) return;
	mpEPoly->EpModSetCutEnd (mLastHitPoint);
}

void EditPolyCutProc::Accept ()
{
	if (mStartIndex<0) return;

	if (mSuspendPreview) {
		//ip->PopPrompt();
		theHold.Begin ();
		mpEPoly->EpModCut (mStartLevel, mStartIndex, mStartPoint, -mLastHitDirection);
	}
	mpEPoly->EpModSetCutEnd (mLastHitPoint);
	theHold.Accept (GetString (IDS_CUT));

	mStartIndex = -1;
}

// Qilin.Ren Defect 932406
// The class is used for encapsulating my algorithms to fix the bug, which is 
// caused by float point precision problems when mouse ray is almost parallel 
// to the XOY plane.
class ViewSnapPlane
{
public:
	// Create a matrix of a new plane, which is orthogonal to the mouse ray
	ViewSnapPlane(IN ViewExp* pView, IN const IPoint2& mousePosition);

	const Point3& GetHitPoint() const;
	const IPoint2& GetBetterMousePosition() const;
	// If the mouse ray did not hit any of the polygons, then we should create
	// another plane, compute the intersection of that plane and the ray, in 
	// order to satisfy MNMesh::Cut alrogithm.
	void HitTestFarPlane(
		OUT Point3& hitPoint, 
		IN EPolyMod* pEditPoly, 
		IN const Matrix3& obj2World);

private:
	const Point3& GetXAxis() const;
	const Point3& GetYAxis() const;
	const Point3& GetZAxis() const;
	const Point3& GetEyePosition() const;

	const Matrix3& GetMatrix() const;
	const Matrix3& GetInvertMatrix() const;

private:
	Point3 mHitPoint;
	IPoint2 mBetterMousePosition;

	Point3 mXAxis;
	Point3 mYAxis;
	Point3 mZAxis;
	Point3 mEyePosition;
	Matrix3 mMatrix;
	Matrix3 mInvertMatrix;
};

ViewSnapPlane::ViewSnapPlane(
	IN ViewExp* pView, 
	IN const IPoint2& mousePosition)
{
	if ( ! pView || ! pView->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}


	Ray ray;
	pView->MapScreenToWorldRay(mousePosition.x, mousePosition.y, ray);

	mEyePosition = ray.p;

	mZAxis = -Normalize(ray.dir);
	Point3 up = mZAxis;
	up[mZAxis.MinComponent()] = +2.0f + mZAxis[mZAxis.MaxComponent()]*2;
	up[mZAxis.MaxComponent()] = -2.0f + mZAxis[mZAxis.MinComponent()]/2;
	mXAxis = Normalize(CrossProd(up, mZAxis));
	mYAxis = CrossProd(mZAxis, mXAxis);

	mMatrix.SetColumn(0, Point4(
		mXAxis.x, 
		mXAxis.y, 
		mXAxis.z, 
		-DotProd(mXAxis, mEyePosition)));
	mMatrix.SetColumn(1, Point4(
		mYAxis.x, 
		mYAxis.y, 
		mYAxis.z, 
		-DotProd(mYAxis, mEyePosition)));
	mMatrix.SetColumn(2, Point4(
		mZAxis.x, 
		mZAxis.y, 
		mZAxis.z, 
		-DotProd(mZAxis, mEyePosition)));
	mInvertMatrix = mMatrix;
	mInvertMatrix.Invert();

	mHitPoint = pView->SnapPoint (
		mousePosition, 
		mBetterMousePosition, 
		&mInvertMatrix);
	mHitPoint = mInvertMatrix.PointTransform(mHitPoint);
}

inline const Point3& ViewSnapPlane::GetHitPoint() const
{
	return mHitPoint;
}

inline const IPoint2& ViewSnapPlane::GetBetterMousePosition() const
{
	return mBetterMousePosition;
}


inline const Point3& ViewSnapPlane::GetXAxis() const
{
	return mXAxis;
}

inline const Point3& ViewSnapPlane::GetYAxis() const
{
	return mYAxis;
}

inline const Point3& ViewSnapPlane::GetZAxis() const
{
	return mZAxis;
}

inline const Point3& ViewSnapPlane::GetEyePosition() const
{
	return mEyePosition;
}


inline const Matrix3& ViewSnapPlane::GetMatrix() const
{
	return mMatrix;
}

inline const Matrix3& ViewSnapPlane::GetInvertMatrix() const
{
	return mInvertMatrix;
}

void ViewSnapPlane::HitTestFarPlane(
	OUT Point3& hitPoint, 
	IN EPolyMod* pEditPoly, 
	IN const Matrix3& obj2World)
{
	if(pEditPoly == NULL) {
		DbgAssert(NULL != pEditPoly);
		return;
	}
	Box3 bbox;
	Tab<Box3> boxes;
	IObjParam* l_ip = pEditPoly->EpModGetIP();
	if (NULL != l_ip)
	{
		// combine all bounding box together, then we are safe
		int i;
		ModContextList l_list;
		INodeTab l_nodes;	
		l_ip->GetModContexts(l_list, l_nodes);
		for (i = 0; i < l_list.Count(); ++i)
		{
			EditPolyData* l_pData = (EditPolyData *)(l_list[i]->localData);
			if (NULL == l_pData)
			{
				continue;
			}
			MNMesh* pMesh = l_pData->GetMesh();
			if (NULL == pMesh)
			{
				continue;
			}
			pMesh->BBox(bbox);
			boxes.Append(1, &bbox);
		}
		bbox.Init();
		for (int i = 0; i < boxes.Count(); ++i)
		{
			bbox += boxes[i];
		}
	}
	if (boxes.Count() == 0)
	{
		// We still need to transform the hit point into object space.
		Matrix3 l_world2obj = Inverse (obj2World);
		hitPoint = l_world2obj * hitPoint;
		return;
	}

	// convert bbox to bounding sphere
	Point3 l_boundCenter = bbox.Center();
	float l_boundRadius = ((bbox.Width()/2)*obj2World).Length();

	// convert bounding sphere from object space to world space
	l_boundCenter = obj2World * l_boundCenter;

	// create a plane
	Point3 v1 = l_boundCenter - GetZAxis() * l_boundRadius;
	const Point3& planeNormal = GetZAxis();
	float d = DotProd(planeNormal, v1);

	// get the intersection of (mLastHitPoint, mLastHitPoint-zAxis) and the plane
	Point3 A = mHitPoint;
	Point3 B = mHitPoint - GetZAxis();
	float dA = DotProd(planeNormal, A) - d;
	float dB = DotProd(planeNormal, B) - d;
	float scale = dA / (dA - dB);
	Point3 C = B-A;
	C *= scale;
	A += C;

	hitPoint = A;

	// We still need to transform the hit point into object space.
	Matrix3 l_world2obj = Inverse (obj2World);
	hitPoint = l_world2obj * hitPoint;
}

int EditPolyCutProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	IPoint2 betterM(0,0);
	Matrix3 world2obj;
	HitRecord* hr = NULL;
	Ray r;
	static HCURSOR hCutVertCur = NULL, hCutEdgeCur=NULL, hCutFaceCur = NULL;

	if (!hCutVertCur) {
		hCutVertCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CutVert);
		hCutEdgeCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CutEdge);
		hCutFaceCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CutFace);
	}

	switch (msg) {

	case MOUSE_PROPCLICK:
		if (mStartIndex==-1) 
		{
			DrawCutter (hwnd, true);
			Cancel ();
			ip->PopCommandMode ();

		}
		else
		{
			DrawCutter (hwnd, true );
			mStartIndex = -1;
			mStartLevel = -1;

		}

		break;

	case MOUSE_DBLCLICK:
		return false;

	case MOUSE_POINT:
		{
		if (point==1) break;		// Want click-move-click behavior.
		ip->SetActiveViewport (hwnd);
		ViewExp& vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		// Get a worldspace hit point and view direction:
		{
			// Qilin.Ren Defect 932406
			// Fixed the problem while mouse ray is almost parallel to the relative xoy plane
			ViewSnapPlane snapPlane(vpt.ToPointer(), m);
			mLastHitPoint = snapPlane.GetHitPoint();
			betterM = snapPlane.GetBetterMousePosition();
			vpt.MapScreenToWorldRay ((float)betterM.x, (float)betterM.y, r);
			mLastHitDirection = Normalize (r.dir);	// in world space

			// Hit test all subobject levels:
			hr = HitTestAll (m, vpt.ToPointer(), flags, true);
			if (NULL == hr)
			{
				snapPlane.HitTestFarPlane(mLastHitPoint, mpEPoly, mObj2world);
			}
		}


		if (mLastHitLevel == MNM_SL_OBJECT) return true;

		// Get hit directon in object space:
		world2obj = Inverse (mObj2world);
		mLastHitDirection = Normalize (VectorTransform (world2obj, mLastHitDirection));

		if (mStartIndex<0) {
			mpEPoly->EpModSetPrimaryNode (hr->nodeRef);
			Start ();

			mMouse1 = betterM;
			mMouse2 = betterM;
			DrawCutter (hwnd);
			return true;
		}

		// Erase last cutter line:
		DrawCutter (hwnd, true);

		// Do the cut:
		Accept ();

		// Refresh, so that the "LastCutEnd" data is updated along with everything else:
		mpEPoly->UpdateCache (ip->GetTime());
		ip->RedrawViews (ip->GetTime());

		int cutEnd;
		cutEnd = mpEPoly->EpModGetLastCutEnd ();
		if (cutEnd>-1) {
			// Last cut was successful - start the next cut:
			mLastHitIndex = cutEnd;
			mLastHitLevel = MNM_SL_VERTEX;
			Start ();
			mMouse1 = betterM;
			mMouse2 = betterM;
			DrawCutter (hwnd);
			return true;
		}

		// Otherwise, we had to stop somewhere, so we discontinue cutting:
		return false;
		}
	case MOUSE_FREEMOVE:
	case MOUSE_MOVE:
		{
		ViewExp& vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		// Show snap preview:
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		//ip->RedrawViews (ip->GetTime(), REDRAW_INTERACTIVE);	// hey - why are we doing this?

		// Find 3D point in object space:
		{
			// Qilin.Ren Defect 932406
			// Fixed the problem while mouse ray is almost parallel to the relative xoy plane
			ViewSnapPlane snapPlane(vpt.ToPointer(), m);
			mLastHitPoint = snapPlane.GetHitPoint();
			betterM = snapPlane.GetBetterMousePosition();
			vpt.MapScreenToWorldRay ((float)betterM.x, (float)betterM.y, r);
			mLastHitDirection = Normalize (r.dir);	// in world space

			if (NULL == HitTestAll (betterM, vpt.ToPointer(), flags, true))
			{
				snapPlane.HitTestFarPlane(mLastHitPoint, mpEPoly, mObj2world);
			}
		}

		if (mStartIndex>-1) {
			// Erase last dotted line
			DrawCutter (hwnd, true);

			// Set the cut preview to use the point we just hit on:
			Update ();

			// Draw new dotted line
			mMouse2 = betterM;

			GraphicsWindow *gw = vpt.getGW();	
			gw->setTransform(Matrix3(1));
			Point3 startHitPoint,outP;
			startHitPoint = mStartPoint * mObj2world;
			gw->transPoint(&startHitPoint,&outP);
			mMouse1.x = (int)outP.x;
			mMouse1.y = (int)outP.y;

			DrawCutter (hwnd);
		}

		// Set cursor based on best subobject match:
		switch (mLastHitLevel) {
		case MNM_SL_VERTEX: SetCursor (hCutVertCur); break;
		case MNM_SL_EDGE: SetCursor (hCutEdgeCur); break;
		case MNM_SL_FACE: SetCursor (hCutFaceCur); break;
		default: SetCursor (LoadCursor (NULL, IDC_ARROW));
		}


		ip->RedrawViews (ip->GetTime());
		break;
		}
	}


	return TRUE;
}

void EditPolyCutCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CUT));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
	proc.mStartIndex = -1;
}

void EditPolyCutCMode::ExitMode() {
	// Lets make double-extra-sure that Cut's suspension of the preview system doesn't leak out...
	// (This line is actually necessary in the case where the user uses a shortcut key to jump-start
	// another command mode.)
	if (proc.mStartIndex>-1) proc.Cancel ();

	// Commit if needed:
	if (mpEditPoly->GetPolyOperationID () == ep_op_cut)
	{
		theHold.Begin();
		mpEditPoly->EpModCommit (TimeValue(0));
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CUT));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

//------- QuickSlice proc/mode------------------------

void EditPolyQuickSliceProc::DrawSlicer (HWND hWnd, bool clear) {
	static int callCount = 0;
	if (!mSlicing) return;

	callCount++;

	IPoint2 points[2];
	points[0]  = mMouse1;
	points[1]  = mMouse2;
	XORDottedPolyline(hWnd, 2, points,0,clear);

}

// Given two points and a view direction (in obj space), find slice plane:
void EditPolyQuickSliceProc::ComputeSliceParams () {
	Point3 Xdir = mLocal2 - mLocal1;
	Xdir = Xdir - mZDir * DotProd (Xdir, mZDir);	// make orthogonal to view
	float len = Length(Xdir);
	Point3 planeNormal;
	if (len<.001f) {
		// Xdir is insignificant, but mZDir is valid.
		// Choose an arbitrary Xdir that'll work:
		Xdir = Point3(1,0,0);
		Xdir = Xdir - mZDir * DotProd (Xdir, mZDir);
		len = Length(Xdir);
	}

	if (len<.001f) {
		// straight X-direction didn't work; therefore straight Y-direction should:
		Xdir = Point3(0,1,0);
		Xdir = Xdir - mZDir * DotProd (Xdir, mZDir);
		len = Length (Xdir);
	}

	// Now guaranteed to have some valid Xdir:
	Xdir = Xdir/len;
	planeNormal = mZDir^Xdir;

	float dp = DotProd (planeNormal, mLocal1-mBoxCenter);
	Point3 planeCtr = mBoxCenter + dp * planeNormal;

	planeNormal = FNormalize (mWorld2ModContext.VectorTransform (planeNormal));
	mpEPoly->EpSetSlicePlane (planeNormal, mWorld2ModContext*planeCtr, mpInterface->GetTime());
}

static HCURSOR hCurQuickSlice = NULL;

void EditPolyQuickSliceProc::Start ()
{
	if (mpInterface->GetSnapState()) {
		// Must suspend "fully interactive" while snapping in Quickslice mode.
		mSuspendPreview = true;
		mpInterface->PushPrompt (GetString (IDS_QUICKSLICE_SNAP_PREVIEW_WARNING));
	} else {
		mSuspendPreview = false;
		theHold.Begin ();
		ComputeSliceParams ();
		if (mpEPoly->GetMNSelLevel()==MNM_SL_FACE) mpEPoly->EpModSetOperation (ep_op_slice_face);
		else mpEPoly->EpModSetOperation (ep_op_slice);
	}

	// Get the current world2obj and mod context transforms for node 0:
	ModContextList list;
	INodeTab nodes;
	mpInterface->GetModContexts (list, nodes);
	Matrix3 objTM = nodes[0]->GetObjectTM (mpInterface->GetTime());
	nodes.DisposeTemporary();
	mWorld2ModContext = (*(list[0]->tm)) * Inverse(objTM);

	mBoxCenter = objTM * list[0]->box->Center();

	mSlicing = true;
}

void EditPolyQuickSliceProc::Cancel ()
{
	if (!mSlicing) return;

	if (mSuspendPreview) {
		mpInterface->PopPrompt ();
	} else {
		mpEPoly->EpModCancel ();
		theHold.Cancel ();
		mpEPoly->EpModRefreshScreen();
	}

	mSlicing = false;
}

void EditPolyQuickSliceProc::Update ()
{
	if (!mSlicing || mSuspendPreview) return;

	ComputeSliceParams ();
	mpEPoly->EpModRefreshScreen();
}

void EditPolyQuickSliceProc::Accept ()
{
	if (!mSlicing) return;

	if (mSuspendPreview) {
		mpInterface->PopPrompt();
		theHold.Begin ();
		if (mpEPoly->GetMNSelLevel()==MNM_SL_FACE) mpEPoly->EpModSetOperation (ep_op_slice_face);
		else mpEPoly->EpModSetOperation (ep_op_slice);
	}
	ComputeSliceParams ();
	mpEPoly->EpModCommit (mpInterface->GetTime());
	theHold.Accept (GetString (IDS_SLICE));

	mSlicing = false;
}

int EditPolyQuickSliceProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	if (!hCurQuickSlice)
		hCurQuickSlice = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::QuickSlice);

	IPoint2 betterM(0,0);
	Point3 snapPoint(0.0f,0.0f,0.0f);
	Ray r;

	switch (msg) {
	case MOUSE_ABORT:
		if (mSlicing) {
			DrawSlicer (hwnd, true);	// Erase last slice line.
			Cancel ();
		}
		return false;

	case MOUSE_PROPCLICK:
		if (mSlicing) {
			DrawSlicer (hwnd, true);	// Erase last slice line.
			Cancel ();
		}
		mpInterface->PopCommandMode ();
		return false;

	case MOUSE_DBLCLICK:
		return false;

	case MOUSE_POINT:
		{
		if (point==1) break;	// don't want to get a notification on first mouse-click release.
		mpInterface->SetActiveViewport (hwnd);

		// Find where we hit, in world space, on the construction plane or snap location:
		ViewExp& vpt = mpInterface->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, betterM, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);

		if (!mSlicing) {
			// Initialize our endpoints:
			mLocal2 = mLocal1 = snapPoint;

			// We'll also need to find our Z direction:
			vpt.MapScreenToWorldRay ((float)betterM.x, (float)betterM.y, r);
			mZDir = Normalize (r.dir);

			// Begin slicing:
			Start ();

			mMouse1 = betterM;
			mMouse2 = betterM;
			DrawSlicer (hwnd);
		} else {
			DrawSlicer (hwnd, true);	// Erase last slice line.
			
			// second point:
			mLocal2 = snapPoint;

			// Finish Slicing:
			Accept ();
			GetCOREInterface()->ForceCompleteRedraw();
		}

		return true;
		}
	case MOUSE_MOVE:
		{
		if (!mSlicing) break;	// Nothing to do if we haven't clicked first point yet

		SetCursor (hCurQuickSlice);
		DrawSlicer (hwnd, true);	// Erase last slice line.

		// Find where we hit, in world space, on the construction plane or snap location:
		ViewExp &vpt = mpInterface->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, mMouse2, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);


		// Find object-space equivalent for mLocal2:
		mLocal2 = snapPoint;

		Update ();

		DrawSlicer (hwnd);	// Draw this slice line.
		break;
		}
	case MOUSE_FREEMOVE:
		SetCursor (hCurQuickSlice);
		ViewExp& vpt = mpInterface->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		// Show any snapping:
		vpt.SnapPreview(m,m,NULL);//, SNAP_SEL_OBJS_ONLY);

		break;
	}

	return TRUE;
}

void EditPolyQuickSliceCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_QUICKSLICE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
	proc.mSlicing = false;
}

void EditPolyQuickSliceCMode::ExitMode() {
	// Lets make double-extra-sure that QuickSlice's suspension of the preview system doesn't leak out...
	// (This line is actually necessary in the case where the user uses a shortcut key to jump-start
	// another command mode.)
	if (proc.mSlicing) proc.Cancel ();

	// Commit if needed:
	if ((mpEditPoly->GetPolyOperationID () == ep_op_slice) ||
		(mpEditPoly->GetPolyOperationID () == ep_op_slice_face))
	{
		theHold.Begin();
		mpEditPoly->EpModCommitUnlessAnimating (mpEditPoly->ip->GetTime());
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_QUICKSLICE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

//------- HingeFromEdge proc/mode------------------------

HitRecord *EditPolyHingeFromEdgeProc::HitTestEdges (ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	mpEPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
	mpEPoly->ForceIgnoreBackfacing (true);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &mCurrentPoint, vpt);
	mpEPoly->ForceIgnoreBackfacing (false);
	mpEPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();
	return hitLog.ClosestHit();	// (may be NULL.)
}

void EditPolyHingeFromEdgeProc::Start ()
{
	if (mEdgeFound) return;
	mEdgeFound = true;

	int oldEdge = mpEPoly->EpModGetHingeEdge (mpHitRec->nodeRef);
	if (oldEdge == mpHitRec->hitInfo)
	{
		// We hit the same edge - so we're just adjusting the hinge angle.
		mStartAngle = mpEPoly->getParamBlock()->GetFloat (epm_hinge_angle, ip->GetTime());
		theHold.Begin ();
	}
	else
	{
		mStartAngle = 0.0f;
		theHold.Begin ();
		mpEPoly->EpModSetHingeEdge (mpHitRec->hitInfo, 
			mpHitRec->modContext->tm ? *(mpHitRec->modContext->tm) : Matrix3(true),
			mpHitRec->nodeRef);

		SuspendAnimate();
		AnimateOff();
		SuspendSetKeyMode(); // AF (5/13/03)
		mpEPoly->getParamBlock()->SetValue (epm_hinge_angle, TimeValue(0), 0.0f);
		ResumeSetKeyMode();
		ResumeAnimate ();
	}

	mFirstPoint = mCurrentPoint;

	ip->RedrawViews (ip->GetTime());
}

void EditPolyHingeFromEdgeProc::Update ()
{
	if (!mEdgeFound) return;

	IPoint2 diff = mCurrentPoint - mFirstPoint;
	// (this arbirtrarily scales each pixel to one degree.)
	float angle = diff.y * PI / 180.0f;
	mpEPoly->getParamBlock()->SetValue (epm_hinge_angle, ip->GetTime(), angle + mStartAngle);
}

void EditPolyHingeFromEdgeProc::Cancel ()
{
	if (!mEdgeFound) return;
	theHold.Cancel ();
	ip->RedrawViews (ip->GetTime());
	mEdgeFound = false;
}

void EditPolyHingeFromEdgeProc::Accept ()
{
	if (!mEdgeFound) return;

	Update ();
	mpEPoly->EpModCommitUnlessAnimating (ip->GetTime());
	theHold.Accept (GetString (IDS_HINGE_FROM_EDGE));
	ip->RedrawViews (ip->GetTime());
	mEdgeFound = false;
}

// Mouse interaction:
// - user clicks on hinge edge
// - user drags angle.

int EditPolyHingeFromEdgeProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	Point3 snapPoint(0.0f,0.0f,0.0f);
	IPoint2 diff(0,0);

	switch (msg) {
	case MOUSE_ABORT:
		Cancel ();
		return FALSE;

	case MOUSE_PROPCLICK:
		Cancel ();
		ip->PopCommandMode ();
		return FALSE;

	case MOUSE_DBLCLICK:
		return false;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport (hwnd);
		ViewExp &vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		mCurrentPoint = m;

		if (!mEdgeFound) {
			mpHitRec = HitTestEdges (vpt.ToPointer());
			if (!mpHitRec) return true;
			Start ();
		} else {
			Accept ();
			return false;
		}

		return true;
		}
	case MOUSE_MOVE:
		{
		// Find where we hit, in world space, on the construction plane or snap location:
		ViewExp &vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}

		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		mCurrentPoint = m;

		if (!mEdgeFound) {
			// Just set cursor depending on presence of edge:
			if (HitTestEdges(vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
			else SetCursor(LoadCursor(NULL,IDC_ARROW));

			break;
		}


		Update ();
		ip->RedrawViews (ip->GetTime());
		break;
		}
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		mCurrentPoint = m;

		if (!mEdgeFound) {
			// Just set cursor depending on presence of edge:
			if (HitTestEdges (vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
			else SetCursor(LoadCursor(NULL,IDC_ARROW));
		}

		break;
		}
	}

	return TRUE;
}

void EditPolyHingeFromEdgeCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_HINGE_FROM_EDGE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void EditPolyHingeFromEdgeCMode::ExitMode() {
	proc.Reset ();
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_HINGE_FROM_EDGE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

//-------------------------------------------------------

bool EditPolyPickHingeProc::EdgePick (HitRecord *hr, float prop) {
	theHold.Begin ();
	mpEPoly->EpModSetHingeEdge (hr->hitInfo, 
		hr->modContext->tm ? *(hr->modContext->tm) : Matrix3(true), hr->nodeRef);
	theHold.Accept (GetString (IDS_PICK_HINGE));

	ip->RedrawViews (ip->GetTime());
	return false;	// false = exit mode when done; true = stay in mode.
}

void EditPolyPickHingeCMode::EnterMode () {
	mpEditPoly->RegisterCMChangedForGrip();
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_HINGE_PICK_EDGE));
	but->SetCheck(true);
	ReleaseICustButton(but);
}

void EditPolyPickHingeCMode::ExitMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_HINGE_PICK_EDGE));
	but->SetCheck(false);
	ReleaseICustButton(but);
	mpEditPoly->UnRegisterCMChangeForGrip();
}

// --------------------------------------------------------

HitRecord *EditPolyPickBridgeTargetProc::HitTest (IPoint2 &m, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt);
	if (!vpt->NumSubObjHits()) return NULL;

	HitLog& hitLog = vpt->GetSubObjHitList();
	mpBridgeNode = mpEPoly->EpModGetBridgeNode();

	HitRecord *ret = NULL;
	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// If we've established a bridge node, filter out anything not on it.
		if (mpBridgeNode && (hitRec->nodeRef != mpBridgeNode)) continue;

		// TODO: Would be nice if we could filter out edges in the other border,
		// and polygons neighboring the other polygon.

		// Always want the closest hit pixelwise:
		if ((ret==NULL) || (ret->distance > hitRec->distance)) ret = hitRec;
	}
	return ret;
}

int EditPolyPickBridgeTargetProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint(0.0f,0.0f,0.0f);

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);

		// Check to see if we've hit any of the subobjects we're looking for:
		hr = HitTest (m, vpt.ToPointer());

		// Done with the viewport:

	
		if (!hr) break;

		theHold.Begin ();
		{
			// if mWhichEnd is 0, we're setting targ1 based on this hit, and leaving targ2 as it is;
			// otherwise, we do the reverse.
			int targ1 = mWhichEnd ? mpEPoly->getParamBlock()->GetInt(epm_bridge_target_1)-1 : hr->hitInfo;
			int targ2 = mWhichEnd ? hr->hitInfo : mpEPoly->getParamBlock()->GetInt (epm_bridge_target_2)-1;

			// These methods set both the node as well as the parameters for the targets at each end.
			// We use a different method for borders and polygons.
			if (mpEPoly->GetEPolySelLevel() == EPM_SL_BORDER) 
				mpEPoly->EpModBridgeBorders (targ1, targ2, hr->nodeRef);
			else if (mpEPoly->GetEPolySelLevel() == EPM_SL_FACE )
				mpEPoly->EpModBridgePolygons (targ1, targ2, hr->nodeRef);
			else if ( mpEPoly->GetEPolySelLevel() == EPM_SL_EDGE )
				mpEPoly->EpModBridgeEdges (targ1, targ2, hr->nodeRef);
		}


		// Set the undo string as appropriate for the end and subobject level we're picking.
		theHold.Accept (GetString ((mpEPoly->GetEPolySelLevel()==EPM_SL_BORDER) ?
			(mWhichEnd ? IDS_BRIDGE_PICK_EDGE_2 : IDS_BRIDGE_PICK_EDGE_1) :
			(mWhichEnd ? IDS_BRIDGE_PICK_POLYGON_2 : IDS_BRIDGE_PICK_POLYGON_1)));

		// Update the view.
		mpEPoly->EpModRefreshScreen();

		// All done picking.
		ip->PopCommandMode ();
		return false;
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		if (HitTest(m,vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));

		break;
		}
	}

	return true;
}

void EditPolyPickBridge1CMode::EnterMode () {
	mpEditPoly->RegisterCMChangedForGrip();
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG1));
	if (but) {
		but->SetCheck(true);
		ReleaseICustButton(but);
	}
}

void EditPolyPickBridge1CMode::ExitMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG1));
	if (but) {
		but->SetCheck(false);
		ReleaseICustButton(but);
	}
	mpEditPoly->UnRegisterCMChangeForGrip();
}

void EditPolyPickBridge2CMode::EnterMode () {
	mpEditPoly->RegisterCMChangedForGrip();
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG2));
	if (but) {
		but->SetCheck(true);
		ReleaseICustButton(but);
	}
}

void EditPolyPickBridge2CMode::ExitMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG2));
	if (but) {
		but->SetCheck(false);
		ReleaseICustButton(but);
	}
	mpEditPoly->UnRegisterCMChangeForGrip();
}

//-------------------------------------------------------

void EditPolyWeldCMode::EnterMode () {
	mproc.Reset();
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom, IDC_TARGET_WELD));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void EditPolyWeldCMode::ExitMode () {
	// Commit if needed:
	if ((mpEditPoly->GetPolyOperationID () == ep_op_target_weld_vertex) ||
		(mpEditPoly->GetPolyOperationID () == ep_op_target_weld_edge))
	{
		theHold.Begin();
		mpEditPoly->EpModCommit (TimeValue(0));	// non-animatable operation.
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_subobj);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom, IDC_TARGET_WELD));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

void EditPolyWeldMouseProc::DrawWeldLine (HWND hWnd,IPoint2 &m, bool clear) {
	if (targ1<0) return;

	IPoint2 points[2];
	points[0]  = m1;
	points[1]  = m;
	XORDottedPolyline(hWnd, 2, points, 0, clear);

}

bool EditPolyWeldMouseProc::CanWeldVertices (int v1, int v2) {
	MNMesh *pMesh = mpEditPoly->EpModGetOutputMesh ();
	if (pMesh == NULL) return false;

	MNMesh & mm = *pMesh;
	// If vertices v1 and v2 share an edge, then take a collapse type approach;
	// Otherwise, weld them if they're suitable (open verts, etc.)
	int i;
	for (i=0; i<mm.vedg[v1].Count(); i++) {
		if (mm.e[mm.vedg[v1][i]].OtherVert(v1) == v2) break;
	}
	if (i<mm.vedg[v1].Count()) {
		int ee = mm.vedg[v1][i];
		if (mm.e[ee].f1 == mm.e[ee].f2) return false;
		// there are other conditions, but they're complex....
	} else {
		if (mm.vedg[v1].Count() && (mm.vedg[v1].Count() <= mm.vfac[v1].Count())) return false;
		for (i=0; i<mm.vedg[v1].Count(); i++) {
			for (int j=0; j<mm.vedg[v2].Count(); j++) {
				int e1 = mm.vedg[v1][i];
				int e2 = mm.vedg[v2][j];
				int ov = mm.e[e1].OtherVert (v1);
				if (ov != mm.e[e2].OtherVert (v2)) continue;
				// Edges from these vertices connect to the same other vert.
				// That means we have additional conditions:
				if (((mm.e[e1].v1 == ov) && (mm.e[e2].v1 == ov)) ||
					((mm.e[e1].v2 == ov) && (mm.e[e2].v2 == ov))) return false;	// edges trace same direction, so cannot be merged.
				if (mm.e[e1].f2 > -1) return false;
				if (mm.e[e2].f2 > -1) return false;
				if (mm.vedg[ov].Count() <= mm.vfac[ov].Count()) return false;
			}
		}
	}
	return true;
}

bool EditPolyWeldMouseProc::CanWeldEdges (int e1, int e2) {
	MNMesh *pMesh = mpEditPoly->EpModGetOutputMesh ();
	if (pMesh == NULL) return false;

	MNMesh & mm = *pMesh;
	if (mm.e[e1].f2 > -1) return false;
	if (mm.e[e2].f2 > -1) return false;
	if (mm.e[e1].f1 == mm.e[e2].f1) return false;
	if (mm.e[e1].v1 == mm.e[e2].v1) return false;
	if (mm.e[e1].v2 == mm.e[e2].v2) return false;
	return true;
}

HitRecord *EditPolyWeldMouseProc::HitTest (IPoint2 &m, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	// Use the default pixel value - no one wanted the old weld_pixels spinner.
	if (mpEditPoly->GetMNSelLevel()==MNM_SL_EDGE) {
		mpEditPoly->SetHitLevelOverride (SUBHIT_EDGES|SUBHIT_OPENONLY);
	}
	mpEditPoly->SetHitTestResult ();
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEditPoly->ClearHitLevelOverride ();
	mpEditPoly->ClearHitTestResult ();
	if (!vpt->NumSubObjHits()) return NULL;

	HitLog& hitLog = vpt->GetSubObjHitList();
	HitRecord *bestHit = NULL;
	if (targ1>-1)
	{
		for (HitRecord *hr = hitLog.First(); hr; hr=hr->Next())
		{
			if (hr->nodeRef->GetActualINode() != mpEditPoly->EpModGetPrimaryNode()) continue;
			if (targ1 == hr->hitInfo) continue;
			if (mpEditPoly->GetMNSelLevel() == MNM_SL_EDGE) {
				if (!CanWeldEdges (targ1, hr->hitInfo)) continue;
			} else {
				if (!CanWeldVertices (targ1, hr->hitInfo)) continue;
			}
			if (!bestHit || (bestHit->distance > hr->distance)) bestHit = hr;
		}
		return bestHit;
	}

	return hitLog.ClosestHit();
}

int EditPolyWeldMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	int res = TRUE;
	HitRecord* hr = NULL;
	MNMesh *pMesh = NULL;
	int stringID = 0;

	switch (msg) {
	case MOUSE_ABORT:
		// Erase last weld line:
		if (targ1>-1) DrawWeldLine (hwnd, oldm2, true);
		targ1 = -1;
		return FALSE;

	case MOUSE_PROPCLICK:
		// Erase last weld line:
		if (targ1>-1) DrawWeldLine (hwnd, oldm2, true);
		ip->PopCommandMode ();
		return FALSE;

	case MOUSE_DBLCLICK:
		return false;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport (hwnd);
		ViewExp &vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		hr = HitTest (m, vpt.ToPointer());
		if (!hr) break;
		if (targ1 < 0) {
			targ1 = hr->hitInfo;
			mpEditPoly->EpModSetPrimaryNode (hr->nodeRef);
			m1 = m;
			DrawWeldLine (hwnd, m);
			oldm2 = m;
			break;
		}

		// Otherwise, we're completing the weld.
		// Erase the last weld-line:
		DrawWeldLine (hwnd, oldm2, true);

		//pMesh = mpEditPoly->EpModGetOutputMesh ();
		//if (pMesh == NULL) return false;

		// Do the weld:
		theHold.Begin();

		switch (mpEditPoly->GetMNSelLevel()) {
		case MNM_SL_VERTEX:
			//Point3 destination = pMesh->P(hr->hitInfo);
			mpEditPoly->EpModWeldVerts (targ1, hr->hitInfo);
			stringID = IDS_WELD_VERTS;
			break;
		case MNM_SL_EDGE:
			mpEditPoly->EpModWeldEdges (targ1, hr->hitInfo);
			stringID = IDS_WELD_EDGES;
			break;
		}

		theHold.Accept (GetString(stringID));
		mpEditPoly->EpModRefreshScreen ();
		targ1 = -1;

		return false;
		} // case
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		if (targ1 > -1) {
			DrawWeldLine (hwnd, oldm2, true);
			oldm2 = m;
		}
		if (HitTest (m,vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor (LoadCursor (NULL, IDC_ARROW));
		ip->RedrawViews (ip->GetTime());
		if (targ1 > -1) DrawWeldLine (hwnd, m);
		break;
		}
	}
	

	return true;	
}

//-------------------------------------------------------

void EditPolyEditTriCMode::EnterMode() {
	HWND hSurf = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_FS_EDIT_TRI));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_DIAGONALS);
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyEditTriCMode::ExitMode() {
	// Auto-commit if needed:
	if (mpEditPoly->GetPolyOperationID () == ep_op_edit_triangulation)
	{
		theHold.Begin();
		mpEditPoly->EpModCommitUnlessAnimating (TimeValue(0));
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hSurf = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_FS_EDIT_TRI));
		but->SetCheck (FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyEditTriProc::VertConnect () {
	theHold.Begin();
	mpEPoly->EpModSetDiagonal (v1, v2);
	theHold.Accept (GetString (IDS_EDIT_TRIANGULATION));
	mpEPoly->EpModRefreshScreen ();
}

//-------------------------------------------------------

void EditPolyTurnEdgeCMode::EnterMode() {
	HWND hSurf = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_TURN_EDGE));
		if (but) {
			but->SetCheck(true);
			ReleaseICustButton(but);
		}
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_DIAGONALS);
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

void EditPolyTurnEdgeCMode::ExitMode() {
	// Auto-commit if needed:
	if (mpEditPoly->GetPolyOperationID () == ep_op_turn_edge)
	{
		theHold.Begin();
		mpEditPoly->EpModCommitUnlessAnimating (TimeValue(0));
		theHold.Accept (GetString (IDS_EDITPOLY_COMMIT));
	}

	HWND hSurf = mpEditPoly->GetDlgHandle (ep_subobj);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_TURN_EDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
	mpEditPoly->ClearDisplayLevelOverride();
	mpEditPoly->EpModLocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->EpModRefreshScreen ();
}

HitRecord *EditPolyTurnEdgeProc::HitTestEdges (IPoint2 & m, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return NULL;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitTestResult ();
	mpEPoly->ForceIgnoreBackfacing (true);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEPoly->ForceIgnoreBackfacing (false);
	mpEPoly->ClearHitTestResult ();

	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();

	// Find the closest hit that is not a border edge:
	HitRecord *hr = NULL;
	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Always want the closest hit pixelwise:
		if (hr && (hr->distance < hitRec->distance)) continue;
		MNMesh *pMesh = mpEPoly->EpModGetOutputMesh (hitRec->nodeRef);
		if (pMesh==NULL) continue;
		if (pMesh->e[hitRec->hitInfo].f2<0) continue;
		hr = hitRec;
	}

	return hr;	// (may be NULL.)
}

HitRecord *EditPolyTurnEdgeProc::HitTestDiagonals (IPoint2 & m, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return NULL;
	}

	vpt->ClearSubObjHitList();

	mpEPoly->SetHitTestResult ();
	mpEPoly->SetHitLevelOverride (SUBHIT_MNDIAGONALS);
	mpEPoly->ForceIgnoreBackfacing (true);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEPoly->ForceIgnoreBackfacing (false);
	mpEPoly->ClearHitLevelOverride ();
	mpEPoly->ClearHitTestResult ();

	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();
	return hitLog.ClosestHit ();
}

int EditPolyTurnEdgeProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint(0.0f,0.0f,0.0f);
	MNDiagonalHitData *hitDiag=NULL;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		if (point==1) break;

		ip->SetActiveViewport(hwnd);
		ViewExp &vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		//hr = HitTestEdges (m, vpt);
		//if (!hr) {
			hr = HitTestDiagonals (m, vpt.ToPointer());
			if (hr) hitDiag = (MNDiagonalHitData *) hr->hitData;
		//}

	
		if (!hr) break;

		theHold.Begin ();
		if (hitDiag) mpEPoly->EpModTurnDiagonal (hitDiag->mFace, hitDiag->mDiag, hr->nodeRef);
		//else mpEPoly->EpModTurnEdge (hr->hitInfo, hr->nodeRef);
		theHold.Accept (GetString (IDS_TURN_EDGE));
		mpEPoly->EpModRefreshScreen();
		break;
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
		{
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		if (/*HitTestEdges(m,vpt) ||*/ HitTestDiagonals(m,vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));

		break;
		}
	}

	return true;
}

//////////////////////////////////////////

void EditPolyMod::DoAccept(TimeValue t)
{
	EpModCommitUnlessAnimating (t);
}

//for EditSSCB 
void EditPolyMod::SetPinch(TimeValue t, float pinch)
{
	getParamBlock()->SetValue (epm_ss_pinch, t, pinch);
}
void EditPolyMod::SetBubble(TimeValue t, float bubble)
{
	getParamBlock()->SetValue (epm_ss_bubble, t, bubble);
}
float EditPolyMod::GetPinch(TimeValue t)
{
	return getParamBlock()->GetFloat (epm_ss_pinch, t);

}
float EditPolyMod::GetBubble(TimeValue t)
{
	return getParamBlock()->GetFloat (epm_ss_bubble, t);
}
float EditPolyMod::GetFalloff(TimeValue t)
{
	return getParamBlock()->GetFloat (epm_ss_falloff, t);
}

void EditPolyMod::SetFalloff(TimeValue t, float falloff)
{
	getParamBlock()->SetValue (epm_ss_falloff, t, falloff);
			//not needed I don't think!
	EpModCommitUnlessAnimating (t);

}


INode* SubobjectPickProc::HitTest(IPoint2 &m, ViewExp& vpt,int& subLevel, int& subIndex )
{
	subLevel = 0;
	subIndex = -1;
	INode* node = nullptr;

	float hitDist = BIGFLOAT;
	int faceHitIndex = -1;

	vpt.ClearSubObjHitList();
	mpEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEditPoly->ClearHitLevelOverride ();
	if (vpt.NumSubObjHits())
	{
		HitLog& hitLog = vpt.GetSubObjHitList();
		HitRecord* hitRecord = hitLog.ClosestHit();  
		subLevel = 3;
		subIndex = hitRecord->hitInfo;
		faceHitIndex = subIndex;

		hitDist = hitRecord->distance;
		node = hitRecord->nodeRef;
	}

	IPoint2 startM(0,0);
	int fudge = 5;

	startM = m;
	startM.x -= fudge;
	startM.y -= fudge;
	//hit test vertices over a sample of the mouse pos
	for (int indexM = 0; indexM < 3; indexM++)
	{		
		for (int indexN = 0; indexN < 3; indexN++)
		{	
			vpt.ClearSubObjHitList();
			mpEditPoly->SetHitLevelOverride (SUBHIT_MNVERTS);
			ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &startM, vpt.ToPointer());
			mpEditPoly->ClearHitLevelOverride ();
			if (vpt.NumSubObjHits())
			{
				HitLog& hitLog = vpt.GetSubObjHitList();
				HitRecord* hitRecord = hitLog.ClosestHit();  
				if ((subLevel == 0) || (hitRecord->distance <= hitDist) )
				{
					subLevel = 1;
					subIndex = hitRecord->hitInfo;
					hitDist = hitRecord->distance;
					node = hitRecord->nodeRef;
				}
			}
			startM.x += fudge;
		}
		startM.y += fudge;
		startM.x = m.x-fudge;
	}	

	//if we did not hit a vertex see if we hit an edge
	if (subLevel != 1)
	{
		startM = m;
		startM.x -= fudge;
		startM.y -= fudge;
		//hit test vertices over a sample of the mouse pos
		for (int indexM = 0; indexM < 3; indexM++)
		{		
			for (int indexN = 0; indexN < 3; indexN++)
			{	
				vpt.ClearSubObjHitList();
				mpEditPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
				ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &startM, vpt.ToPointer());
				mpEditPoly->ClearHitLevelOverride ();
				if (vpt.NumSubObjHits())
				{
					HitLog& hitLog = vpt.GetSubObjHitList();
					HitRecord* hitRecord = hitLog.ClosestHit();  
					if ((subLevel == 0) || (hitRecord->distance < hitDist))
					{
						subLevel = 2;
						subIndex = hitRecord->hitInfo;
						hitDist = hitRecord->distance;
						node = hitRecord->nodeRef;
					}	
				}
				startM.x += fudge;
			}
			startM.y += fudge;
			startM.x = m.x-fudge;
		}	
	}


	return node;

}


int SubobjectPickProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	static HCURSOR hCutVertCur = NULL, hCutEdgeCur=NULL, hCutFaceCur = NULL;


	switch (msg) {


	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		return FALSE;
		break;

	case MOUSE_DBLCLICK:
		return false;

	case MOUSE_POINT:
		{
			ip->SetActiveViewport (hwnd);
			ViewExp& vpt = ip->GetViewExp (hwnd);
			if ( ! vpt.IsAlive() )
			{
				// why are we here
				DbgAssert(!_T("Invalid viewport!"));
				return FALSE;
			}
			if (point == 1)
			{
				SubobjectPickMode* mode = (SubobjectPickMode*) mpEditPoly->getCommandMode(ep_mode_subobjectpick);
				if ( !mode->mInOverride  )
					ip->PopCommandMode ();
			}

			return true;
		}
	case MOUSE_FREEMOVE:
	case MOUSE_MOVE:
		{

			ViewExp& vpt = ip->GetViewExp (hwnd);
			if ( ! vpt.IsAlive() )
			{
				// why are we here
				DbgAssert(!_T("Invalid viewport!"));
				return FALSE;
			}


			hitNode = HitTest(m, vpt,hitSubLevel, hitSubIndex );
			if (hitSubLevel == 0)
				SetCursor(LoadCursor(NULL,IDC_ARROW));
			else
				SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
			ip->RedrawViews(ip->GetTime());
			break;
		}
	}	 // switch

	return TRUE;
}

void SubobjectPickMode::EnterMode() {
	inMode = true;
}

void SubobjectPickMode::ExitMode() {
	inMode = false;
	mInOverride = false;
	HWND hWnd = mpEditPoly->GetDlgHandle (ep_select);
	PostMessage(hWnd,WM_SUBOBJECTPICK,proc.hitSubLevel,0);

}

void SubobjectPickMode::Display (GraphicsWindow *gw, MNMesh* mm, INode* testNode) {
	//	proc.DrawEstablishedFace(gw);

	if (testNode != proc.hitNode) return;  //we can have instances so need to check the inode

	IColorManager *pClrMan = GetColorManager();;

	Point3 newSubColor = pClrMan->GetColorAsPoint3(SUBOBJECT_PICK_COLORID);
	gw->setColor (FILL_COLOR,newSubColor);
	gw->setColor (LINE_COLOR,newSubColor);


	TimeValue t = GetCOREInterface()->GetTime();
	 
	if(proc.hitSubLevel == 1)
	{
		mpEditPoly->DisplayHighlightVert(t,gw,mm,proc.hitSubIndex);
	}
	else if(proc.hitSubLevel == 2)
	{
		mpEditPoly->DisplayHighlightEdge(t,gw,mm,proc.hitSubIndex);
	}
	else if(proc.hitSubLevel == 3)
	{
		gw->setRndLimits(GW_SHADE_SEL_FACES|GW_Z_BUFFER|GW_ILLUM|GW_TRANSPARENCY|GW_TEXTURE| GW_TRANSPARENT_PASS);
		Material mat;
		//Set Material properties
		mat.dblSided = 1;
		//diffuse, ambient and specular
		mat.Ka = newSubColor ;
		mat.Kd = newSubColor ;
		mat.Ks = newSubColor ;
		// Set opacity
		mat.opacity = 1.0f;
		//Set shininess
		mat.shininess = 0.0f;
		mat.shinStrength = 0.0f;
		mat.selfIllum = 1.0f;
		gw->setMaterial(mat);

		Point3 xyz[4];
		Point3 nor[4];
		Point3 uvw[4];
		uvw[0] = Point3(0.0f,0.0f,0.0f);
		uvw[1] = Point3(1.0f,0.0f,0.0f);
		uvw[2] = Point3(0.0f,1.0f,0.0f);

		gw->startTriangles();
		Tab<int> tri;
		tri.ZeroCount();
		MNFace *face = &mm->f[proc.hitSubIndex];
		face->GetTriangles(tri);
		int		*vv		= face->vtx;
		int		deg		= face->deg;
		DWORD	smGroup = face->smGroup;
		for (int tt=0; tt<tri.Count(); tt+=3)
		{
			int *triv = tri.Addr(tt);
			for (int z=0; z<3; z++) xyz[z] = mm->v[vv[triv[z]]].p;
			nor[0] = nor[1] = nor[2] = mm->GetFaceNormal(proc.hitSubIndex, TRUE);
			gw->triangleN(xyz,nor,uvw);
		}
		gw->endTriangles();		
	}
}

bool HasAdjacentSelection(MNMesh* mm, int subObjectLevel, int subObjectIndex,  bool findRing, bool& isRing)
{
	bool hasSelectedNeighbor = false;
	isRing = false;
	if (subObjectLevel == MNM_SL_VERTEX)
	{
		if ((subObjectIndex >= 0) && (subObjectIndex < mm->numv))
		{
			MNVert& vert = mm->v[subObjectIndex];
			if (!vert.GetFlag(MN_DEAD))
			{
				mm->OrderVert(subObjectIndex);
				int deg = mm->vedg[subObjectIndex].Count();

				BitArray originalSelection;
				mm->getVertexSel(originalSelection);

				for (int i = 0; i < deg; i++)
				{
					int v1 = mm->e[mm->vedg[subObjectIndex][i]].v1;
					int v2 = mm->e[mm->vedg[subObjectIndex][i]].v2;
					if (v1 == subObjectIndex)
						v1 = v2;

					if (originalSelection[v1])
					{
						hasSelectedNeighbor = true;
						break;
					}
				}
			}
		}
	}
	else if (subObjectLevel == MNM_SL_EDGE)
	{
		if ((subObjectIndex >= 0) && (subObjectIndex < mm->nume))
		{
			MNEdge& edge = mm->e[subObjectIndex];
			if (!edge.GetFlag(MN_DEAD))
			{
				int v[2];
				v[0] = edge.v1;
				v[1] = edge.v2;

				BitArray originalSelection;
				mm->getEdgeSel(originalSelection);

				for (int i = 0; i < 2; i++)
				{
					mm->OrderVert(v[i]);
					int ct = mm->vedg[v[i]].Count();
					if (ct == 4)
					{
						for (int j = 0; j < 4; j++)
						{
							if (mm->vedg[v[i]][j] == subObjectIndex)
							{
								int oppoEdge = (j + 2) % 4;
								int oppoEdgeIndex = mm->vedg[v[i]][oppoEdge];
								if (originalSelection[oppoEdgeIndex])
								{
									hasSelectedNeighbor = true;
									j = 4;
									break;
								}
							}
						}
					}
				}
				//now check for the ring
				if (!hasSelectedNeighbor && findRing)
				{
					//get our coonnected faces
					for (int i = 0; i < 2; i++)
					{
						int fIndex = mm->e[subObjectIndex].f1;
						if (i == 1)
							fIndex = mm->e[subObjectIndex].f2;
						//make sure we are not an open edge
						if (fIndex != -1)
						{
							MNFace& f = mm->f[fIndex];
							// the face has to be a quad
							if (f.deg == 4)
							{
								//find the our edge on the face
								for (int j = 0; j < 4; j++)
								{
									if (f.edg[j] == subObjectIndex)
									{
										//get the opposite edge
										int oppoEdge = (j + 2) % 4;
										int oppoEdgeIndex = f.edg[oppoEdge];
										//see if it is selected
										if (originalSelection[oppoEdgeIndex])
										{
											hasSelectedNeighbor = true;
											isRing = true;
											break;
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
	else if (subObjectLevel == MNM_SL_FACE)
	{
		if ((subObjectIndex >= 0) && (subObjectIndex < mm->numf))
		{
			MNFace& face = mm->f[subObjectIndex];
			if (!face.GetFlag(MN_DEAD))
			{
				int deg = face.deg;

				BitArray originalSelection;
				mm->getFaceSel(originalSelection);
				for (int i = 0; i < deg; i++)
				{
					int f1 = mm->e[face.edg[i]].f1;
					int f2 = mm->e[face.edg[i]].f2;
					if (f1 == subObjectIndex)
						f1 = f2;
					if (f1 != -1)
					{
						if (originalSelection[f1])
						{
							hasSelectedNeighbor = true;
							break;
						}
					}
				}
			}
		}
	}
	return hasSelectedNeighbor;
}

bool PointToPointProc::AdjacentLoop(int mode, int index)
{
	mInLoop = false;
	MNMesh* mm = mpEditPoly->EpModGetOutputMesh(mActiveNode);
	if (mm == nullptr)
		mm = mpEditPoly->EpModGetMesh(mActiveNode);
	if (mm == nullptr)
		return false;

	bool hasSelectedNeighbor = false;
	bool isRing = false;
	if (mode == EPM_SL_VERTEX)
	{
		hasSelectedNeighbor = HasAdjacentSelection(mm, MNM_SL_VERTEX, index, false, isRing);
		if (originalSelection.GetSize() != mm->numv)
		{
			mm->getVertexSel(originalSelection);
		}
		if (hasSelectedNeighbor && !originalSelection[index])
		{
			IMNMeshUtilities13* l_mesh13 = static_cast<IMNMeshUtilities13*>(mm->GetInterface(IMNMESHUTILITIES13_INTERFACE_ID));
			if (l_mesh13)
			{
				BitArray loopVerts(originalSelection);
				loopVerts.ClearAll();
				loopVerts.Set(index, TRUE);
				bool foundLoop = l_mesh13->FindLoopVertex(loopVerts, originalSelection);
				if (foundLoop)
				{
					BitArray currentSel(originalSelection);
					selection = loopVerts;
					currentSel |= loopVerts;
					mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, currentSel, mActiveNode);
					mInLoop = true;
				}
			}
		}
		else
		{
			mInLoop = false;
			mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, originalSelection, mActiveNode);
		}



	}
	else if (mode == EPM_SL_EDGE)
	{
		hasSelectedNeighbor = HasAdjacentSelection(mm, MNM_SL_EDGE, index, false, isRing);

		if (originalSelection.GetSize() != mm->nume)
		{
			mm->getEdgeSel(originalSelection);
		}
		if (hasSelectedNeighbor && !originalSelection[index])
		{

			BitArray loopEdges(originalSelection);
			loopEdges.ClearAll();
			loopEdges.Set(index, TRUE);
			{
				TemporaryNoBadVerts nbv(*mm);
				mm->SelectEdgeLoop(loopEdges);
			}
			selection = loopEdges;
			loopEdges |= originalSelection;
			mpEditPoly->EpModSetSelection(MNM_SL_EDGE, loopEdges, mActiveNode);
			mInLoop = true;
		}
		else
		{
			mInLoop = false;
			mpEditPoly->EpModSetSelection(MNM_SL_EDGE, originalSelection, mActiveNode);
		}


	}
	else if (mode == EPM_SL_FACE)
	{
		hasSelectedNeighbor = HasAdjacentSelection(mm, MNM_SL_FACE, index, false, isRing);
		if (originalSelection.GetSize() != mm->numf)
		{
			mm->getFaceSel(originalSelection);
		}
		if (hasSelectedNeighbor && !originalSelection[index])
		{
			IMNMeshUtilities13* l_mesh13 = static_cast<IMNMeshUtilities13*>(mm->GetInterface(IMNMESHUTILITIES13_INTERFACE_ID));
			if (l_mesh13)
			{
				BitArray loopFaces(originalSelection);
				loopFaces.ClearAll();
				loopFaces.Set(index, TRUE);
				bool foundLoop = l_mesh13->FindLoopFace(loopFaces, originalSelection);
				if (foundLoop)
				{
					BitArray currentSel(originalSelection);
					selection = loopFaces;
					currentSel |= loopFaces;
					mpEditPoly->EpModSetSelection(MNM_SL_FACE, currentSel, mActiveNode);
					mInLoop = true;
				}
			}
		}
		else
		{
			mInLoop = false;
			mpEditPoly->EpModSetSelection(MNM_SL_FACE, originalSelection, mActiveNode);
		}


	}

	return mInLoop;
}


int PointToPointProc::HitTest(IPoint2 &m, ViewExp& vpt, bool setHitNode)
{
	int res = -1;
	INode* hitNode = nullptr;
		
	if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
	{
		int faceIndex = -1;
		vpt.ClearSubObjHitList();
		mpEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
		ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
		mpEditPoly->ClearHitLevelOverride ();
		if (vpt.NumSubObjHits())
		{
			HitLog& hitLog = vpt.GetSubObjHitList();
			HitRecord* hitRecord = hitLog.ClosestHit();  
			faceIndex = hitRecord->hitInfo;
			hitNode = hitRecord->nodeRef;
		}

		if (faceIndex != -1)
		{
			if (setHitNode)
				mActiveNode = hitNode;			
			if (mActiveNode != hitNode) return -1;
			MNMesh* mm = mpEditPoly->EpModGetOutputMesh (mActiveNode);		
			if (mm == nullptr)
				mm = mpEditPoly->EpModGetMesh(mActiveNode);

			vpt.ClearSubObjHitList();
			mpEditPoly->SetHitLevelOverride (SUBHIT_MNVERTS);
			ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
			mpEditPoly->ClearHitLevelOverride ();
			if (vpt.NumSubObjHits())
			{
				HitLog& hitLog = vpt.GetSubObjHitList();
				HitRecord* hitRecord = hitLog.ClosestHit();  
				if (mm != nullptr)
				{
				int testHit = hitRecord->hitInfo;
				MNFace &face = mm->f[faceIndex];
				for (int i = 0; i < face.deg; i++)
				{
					if (face.vtx[i] == testHit)
						res = testHit;
				}
			}
				else
				{
					res = hitRecord->hitInfo;
				}
			}
		}
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
	{

		vpt.ClearSubObjHitList();
		mpEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
		ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
		mpEditPoly->ClearHitLevelOverride ();
		if (vpt.NumSubObjHits())
		{
			HitLog& hitLog = vpt.GetSubObjHitList();
			HitRecord* hitRecord = hitLog.ClosestHit();  
			hitNode = hitRecord->nodeRef;

			if (setHitNode)
				mActiveNode = hitNode;			
			if (mActiveNode != hitNode) return -1;

			res = hitRecord->hitInfo;
		}
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE)
	{
		int faceIndex = -1;
		vpt.ClearSubObjHitList();
		mpEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
		ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
		mpEditPoly->ClearHitLevelOverride ();
		if (vpt.NumSubObjHits())
		{
			HitLog& hitLog = vpt.GetSubObjHitList();
			HitRecord* hitRecord = hitLog.ClosestHit();  
			faceIndex = hitRecord->hitInfo;
			hitNode = hitRecord->nodeRef;
		}

		if (faceIndex != -1)
		{
			if (setHitNode)
				mActiveNode = hitNode;			
			if (mActiveNode != hitNode) return -1;
			MNMesh* mm = mpEditPoly->EpModGetOutputMesh (mActiveNode);		
			if (mm == nullptr)
				mm = mpEditPoly->EpModGetMesh(mActiveNode);

			vpt.ClearSubObjHitList();
			mpEditPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
			ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
			mpEditPoly->ClearHitLevelOverride ();
			if (vpt.NumSubObjHits())
			{
				HitLog& hitLog = vpt.GetSubObjHitList();
				HitRecord* hitRecord = hitLog.ClosestHit();  
				int testHit = hitRecord->hitInfo;
				if (mm != nullptr)
				{
				MNEdge &edge = mm->e[testHit];
				if ((edge.f1 == faceIndex) || (edge.f2 == faceIndex))
					res = hitRecord->hitInfo;
			}
				else
					res = testHit;
			}
		}
	}

	return res;

}

void PointToPointProc::RestoreSelection()
{
	if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
	{
		mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, originalSelection, mActiveNode);
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE)
	{
		mpEditPoly->EpModSetSelection(MNM_SL_EDGE, originalSelection, mActiveNode);
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
	{
		mpEditPoly->EpModSetSelection(MNM_SL_FACE, originalSelection, mActiveNode);
	}

}

void PointToPointProc::SetFirstHit(int hitIndex, INode* lastHitNode)
{
	if (mActiveNode != nullptr)
	{
		mInLoop = false;
		MNMesh* mm = mpEditPoly->EpModGetOutputMesh(mActiveNode);
		if (mm == nullptr)
			mm = mpEditPoly->EpModGetMesh(mActiveNode);

		if (mm != nullptr)
		{
		if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
		{
			if ((lastHitNode != mActiveNode) || (mPointToPointPath == nullptr))
				RebuildList(-1);
			mm->getVertexSel(originalSelection);
			mFirstHit = hitIndex;
			mPointToPointPath->SetStartPoint(mFirstHit);

			BitArray sel(originalSelection);
			sel.Set(mFirstHit, TRUE);
			mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, sel, mActiveNode);
		}
		else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE)
		{
			mm->getEdgeSel(originalSelection);
			RebuildList(hitIndex);
			mFirstHit = hitIndex;
			mPointToPointPath->SetStartPoint(mm->numv);

			BitArray sel(originalSelection);
			sel.Set(mFirstHit, TRUE);
			mpEditPoly->EpModSetSelection(MNM_SL_EDGE, sel, mActiveNode);
		}
		else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
		{
			if ((lastHitNode != mActiveNode) || (mPointToPointPath == nullptr))
				RebuildList(-1);
			mm->getFaceSel(originalSelection);
			mFirstHit = hitIndex;
			mPointToPointPath->SetStartPoint(mFirstHit);

			BitArray sel(originalSelection);
			sel.Set(mFirstHit, TRUE);
			mpEditPoly->EpModSetSelection(MNM_SL_FACE, sel, mActiveNode);
		}
	}
	}

}

int PointToPointProc::proc(HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	static HCURSOR hCutVertCur = NULL, hCutEdgeCur=NULL, hCutFaceCur = NULL;

	BOOL shiftDown = flags & MOUSE_SHIFT;

	switch (msg) {


	case MOUSE_DBLCLICK:
	case MOUSE_PROPCLICK:
		{			
			PointToPointMode* mode = (PointToPointMode*)mpEditPoly->getCommandMode(ep_mode_point_to_point);
			if (mode->mShiftLaunched)
			{
				RestoreSelection();
				ip->PopCommandMode();
				IMenuManager* menuManager = GetCOREInterface()->GetMenuManager();
				IQuadMenu* menu = menuManager->GetViewportRightClickMenu(IQuadMenuContext::kShiftPressed);
				if (menu != nullptr)
					menu->TrackMenu(hwnd, menuManager->GetShowAllQuads(menu));
			}
			else
			{
				RestoreSelection();
				ip->PopCommandMode();
			}
			return FALSE;
		}
		
		break;


	case MOUSE_POINT:
		{
		ip->SetActiveViewport (hwnd);
		ViewExp& vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		if (mInLoop)
		{
			theHold.Begin();
			BitArray currentSel(originalSelection);
			currentSel |= selection;
			if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
			{
				mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, currentSel, mActiveNode);
//				mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
			}
			else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE)
			{
				mpEditPoly->EpModSetSelection(MNM_SL_EDGE, currentSel, mActiveNode);
//				mpEditPoly->SetEdgeSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
			}
			else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
			{
				mpEditPoly->EpModSetSelection(MNM_SL_FACE, currentSel, mActiveNode);
//				mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
			}
			theHold.Accept(GetString(IDS_EP_POINT_TO_POINT));

//			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
//			mpEditPoly->RefreshScreen();

			originalSelection = currentSel;

		}
		else if (mFirstHit == -1)
		{
			INode* lastHitNode = mActiveNode;
			int hitIndex = HitTest(m, vpt,true);
			if (hitIndex != -1)
			{
				SetFirstHit(hitIndex, lastHitNode);
			}
			
		}
		else if (mSecondHit != -1)
		{
			mFirstHit = mSecondHit;
			mSecondHit = -1;			

			theHold.Begin();
			BitArray currentSel(originalSelection);
			currentSel |= selection;				
			if (mActiveNode != nullptr)
			{
				MNMesh* mm = mpEditPoly->EpModGetOutputMesh(mActiveNode);
				if (mm == nullptr)
					mm = mpEditPoly->EpModGetMesh(mActiveNode);
				if (mm != nullptr)
				{
				if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
				{
					mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, currentSel, mActiveNode);
					mPointToPointPath->SetStartPoint(mFirstHit);
				}
				else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE)
				{
					mpEditPoly->EpModSetSelection(MNM_SL_EDGE, currentSel, mActiveNode);
					RebuildList(mFirstHit);
					mPointToPointPath->SetStartPoint(mm->numv);
				}
				else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
				{
					mpEditPoly->EpModSetSelection(MNM_SL_FACE, currentSel, mActiveNode);
					mPointToPointPath->SetStartPoint(mFirstHit);
				}
			}
			}

			theHold.Accept(GetString (IDS_EP_POINT_TO_POINT));
			originalSelection = currentSel;			
		}

		return true;
		}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:	
		{
		PointToPointMode* mode = (PointToPointMode*)mpEditPoly->getCommandMode(ep_mode_point_to_point);
		if (mode->mShiftLaunched)
		{
			if (!shiftDown)
			{
				RestoreSelection();
				GetCOREInterface()->PopCommandMode();
				return FALSE;
			}
		}
		ViewExp& vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}

		INode* holdNode = mActiveNode;
		int index = HitTest(m, vpt,true);
		


		if (index == -1)
			SetCursor(LoadCursor(NULL,IDC_ARROW));
		else
			SetCursor(ip->GetSysCursor(SYSCUR_SELECT));

		//if we hovering and not in a drag point to point we need to update the active node and also the original selection
		if (mFirstHit == -1)
		{ 
			if (holdNode != mActiveNode)
			{ 
				mActiveNode = holdNode;
				StoreOffOriginalSelection();
			}
		}
		bool loopSet = false;
		if (mode->mShiftLaunched && (index != -1) && (mFirstHit == -1))
		{
			loopSet = AdjacentLoop(GetCOREInterface()->GetSubObjectLevel(), index);
		}
		
		if ((mFirstHit != -1) && (index != -1)  && !loopSet)
		{			
			mActiveNode = holdNode;
			MNMesh* mm = mpEditPoly->EpModGetOutputMesh (mActiveNode);	
			if (mm == nullptr)
				mm = mpEditPoly->EpModGetMesh(mActiveNode);
			
			BitArray currentSel(originalSelection);			
			mSecondHit = index;
			PointToPointMode* mode = (PointToPointMode*) mpEditPoly->getCommandMode(ep_mode_point_to_point);
			if ((index != -1) && (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX))
			{
				BitArray loopTest;
				ShortLoop(EPM_SL_VERTEX, mFirstHit, mSecondHit, mm, loopTest);

				if (loopTest.NumberSet() > 0)
				{
					selection = loopTest;
					currentSel |= selection;
					mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, currentSel, mActiveNode);
//					mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
				else
				{
					mPointToPointPath->SetEndPoint(index);
					Tab<MaxSDK::EdgeDescr> edges;
					mPointToPointPath->GetPath(edges);
					selection.ClearAll();
					for (int i = 0; i < edges.Count(); i++)
					{
						selection.Set(edges[i].NodeIndex(), TRUE);
					}
					selection.Set(mFirstHit, TRUE);
					selection.Set(mSecondHit, TRUE);
					currentSel |= selection;
					mpEditPoly->EpModSetSelection(MNM_SL_VERTEX, currentSel, mActiveNode);
				}
			}
			else if ((index != -1) && (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE))
			{
				BitArray loopTest;
				ShortLoop(EPM_SL_EDGE, mFirstHit, mSecondHit, mm, loopTest);

				if (loopTest.NumberSet() > 0)
				{
					selection = loopTest;
					currentSel |= selection;
					mpEditPoly->EpModSetSelection(MNM_SL_EDGE, currentSel, mActiveNode);
//					mpEditPoly->SetEdgeSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
				else
				{
					if (mm != nullptr)
					{
					int v1 = mm->e[index].v1;
					mPointToPointPath->SetEndPoint(v1);
					Tab<MaxSDK::EdgeDescr> edges1;
					float d1 = mPointToPointPath->GetPath(edges1);

					int v2 = mm->e[index].v2;
					mPointToPointPath->SetEndPoint(v2);
					Tab<MaxSDK::EdgeDescr> edges2;
					float d2 = mPointToPointPath->GetPath(edges2);

					Tab<MaxSDK::EdgeDescr> edges;
					edges = edges1;
					if (d2 < d1)
						edges = edges2;

					selection.ClearAll();
					for (int i = 0; i < edges.Count(); i++)
					{
						selection.Set(edges[i].EdgeIndex(), TRUE);
					}

					selection.Set(mFirstHit, TRUE);
					selection.Set(mSecondHit, TRUE);
					currentSel |= selection;
					mpEditPoly->EpModSetSelection(MNM_SL_EDGE, currentSel, mActiveNode);
				}
			}
			}
			else if ((index != -1) && (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE))
			{
				mPointToPointPath->SetEndPoint(index);


				BitArray loopTest;
				ShortLoop(EPM_SL_FACE, mFirstHit, mSecondHit, mm, loopTest);

				if (loopTest.NumberSet() > 0)
				{
					selection = loopTest;
					currentSel |= selection;
					mpEditPoly->EpModSetSelection(MNM_SL_FACE, currentSel, mActiveNode);
//					mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
				else
				{
					Tab<MaxSDK::EdgeDescr> edges;
					float d = mPointToPointPath->GetPath(edges);
					selection.ClearAll();
					for (int i = 0; i < edges.Count(); i++)
					{
						selection.Set(edges[i].NodeIndex(), TRUE);
					}
					selection.Set(mFirstHit, TRUE);
					selection.Set(mSecondHit, TRUE);
					currentSel |= selection;
					mpEditPoly->EpModSetSelection(MNM_SL_FACE, currentSel, mActiveNode);
				}
			}
		}
		
		ip->RedrawViews(ip->GetTime());
		break;
		}
	}	 // switch

	return TRUE;
}

int GetIthEdge(MNMesh* mm, int edgeIndex, int vertexIndex)
{
	int ct = mm->vedg[vertexIndex].Count();
	for (int i = 0; i < ct; i++)
	{
		if (mm->vedg[vertexIndex][i] == edgeIndex)
			return i;
	}
	return -1;
}

bool GetNextEdge(MNMesh* mm, int edgeIndex, int vertexIndex, int& newEdge, int& newVertex)
{
	newVertex = mm->e[edgeIndex].v1;
	if (newVertex == vertexIndex)
		newVertex = mm->e[edgeIndex].v2;
	if (mm->vedg[newVertex].Count() != 4)
		return false;
	int ithEdge = GetIthEdge(mm, edgeIndex, newVertex);
	ithEdge = (ithEdge + 2) % 4;
	newEdge = mm->vedg[newVertex][ithEdge];
	return true;
}


bool GetShortLoop(MNMesh *mm, int edgeIndex, int vertIndex, int endConditionIndex, BitArray &loop, bool testVert)
{
	loop.ClearAll();
	int currentEdge = edgeIndex;
	int currentVert = vertIndex;
	bool hitEnd = false;
	bool done = false;
	loop.Set(edgeIndex, TRUE);
	while (!done)
	{
		int nextVert = -1;
		int nextEdge = -1;
		bool isLoop = GetNextEdge(mm, currentEdge, currentVert, nextEdge, nextVert);
		if (!isLoop)
		{
			done = true;
		}
		else if (nextEdge == edgeIndex)
		{
			done = true;
		}
		else if (!testVert && (nextEdge == endConditionIndex))
		{
			hitEnd = true;
			done = true;
			loop.Set(nextEdge, TRUE);
		}
		else if (testVert && (nextVert == endConditionIndex))
		{
			hitEnd = true;
			done = true;
		}
		else
		{
			currentVert = nextVert;
			currentEdge = nextEdge;
			loop.Set(nextEdge, TRUE);
		}
	}

	return hitEnd;

}

void PointToPointProc::ShortLoop(int mode, int start, int end, MNMesh *mm, BitArray& loop)
{
	if (!mm->GetFlag(MN_MESH_VERTS_ORDERED))
		mm->OrderVerts();
	if (mode == EPM_SL_VERTEX)
	{
		if ((start >= 0) && (start < mm->numv) &&
			(end >= 0) && (end < mm->numv))
		{
			MNVert& startVert = mm->v[start];
			MNVert& endVert = mm->v[end];
			if (startVert.GetFlag(MN_DEAD))
				return;
			if (endVert.GetFlag(MN_DEAD))
				return;

			int deg = mm->vedg[start].Count();
			BitArray testLoop;
			testLoop.SetSize(mm->nume);
			testLoop.ClearAll();
			BitArray eLoop;
			eLoop.SetSize(mm->nume);
			eLoop.ClearAll();
			for (int i = 0; i < deg; i++)
			{
				testLoop.ClearAll();
				testLoop.Set(start, TRUE);
				int currentVert = start;
				int currentEdge = mm->vedg[start][i];

				bool hitEnd = GetShortLoop(mm, currentEdge, currentVert, end, testLoop, true);
				if (((testLoop.NumberSet() != 0) && ((testLoop.NumberSet() < eLoop.NumberSet()) || (eLoop.NumberSet() == 0))) && hitEnd)
				{
					eLoop = testLoop;
				}
			}
			loop.SetSize(mm->numv);
			loop.ClearAll();
			for (int i = 0; i < eLoop.GetSize(); i++)
			{
				if (eLoop[i])
				{
					loop.Set(mm->e[i].v1, TRUE);
					loop.Set(mm->e[i].v2, TRUE);
				}
			}
		}
	}

	else if (mode == EPM_SL_EDGE)
	{
		if ((start >= 0) && (start < mm->nume) &&
			(end >= 0) && (end < mm->nume))
		{
			MNEdge& startEdge = mm->e[start];
			MNEdge& endEdge = mm->e[end];
			if (startEdge.GetFlag(MN_DEAD))
				return;
			if (endEdge.GetFlag(MN_DEAD))
				return;

			int deg = 2;
			BitArray testLoop;
			testLoop.SetSize(mm->nume);
			testLoop.ClearAll();
			loop.SetSize(mm->nume);
			loop.ClearAll();
			for (int i = 0; i < deg; i++)
			{
				testLoop.ClearAll();
				testLoop.Set(start, TRUE);
				int currentVert = startEdge.v1;
				if (i == 1)
					currentVert = startEdge.v2;

				bool hitEnd = GetShortLoop(mm, start, currentVert, end, testLoop, false);
				if (((testLoop.NumberSet() != 0) && ((testLoop.NumberSet() < loop.NumberSet()) || (loop.NumberSet() == 0))) && hitEnd)
				{
					loop = testLoop;
				}
			}
		}
	}
	else if (mode == EPM_SL_FACE)
	{
		if ((start >= 0) && (start < mm->numf) &&
			(end >= 0) && (end < mm->numf))
		{
			MNFace& startFace = mm->f[start];
			MNFace& endFace = mm->f[end];
			if (startFace.GetFlag(MN_DEAD))
				return;
			if (endFace.GetFlag(MN_DEAD))
				return;

			int deg = startFace.deg;
			BitArray testLoop;
			testLoop.SetSize(mm->numf);
			testLoop.ClearAll();
			loop.SetSize(mm->numf);
			loop.ClearAll();
			//			DebugPrint(_T("%d to %d\n"), start, end);
			for (int i = 0; i < deg; i++)
			{
				testLoop.ClearAll();
				testLoop.Set(start, TRUE);
				int currentFace = start;
				int edgeIndex = i;
				bool hitEnd = false;
				bool done = false;
				while (!done)
				{
					int nextFace = -1;
					if (mm->f[currentFace].deg == 4)
					{
						int oppositeEdge = (edgeIndex + 2) % 4;
						int oppositeEdgeIndex = mm->f[currentFace].edg[oppositeEdge];
						int f1 = mm->e[oppositeEdgeIndex].f1;
						int f2 = mm->e[oppositeEdgeIndex].f2;
						nextFace = f1;
						if ((nextFace == -1) || (nextFace == currentFace))
						{
							if (f2 != currentFace)
								nextFace = f2;
						}
						if (nextFace != -1)
						{
							edgeIndex = mm->f[nextFace].EdgeIndex(oppositeEdgeIndex);
						}
					}
					if ((nextFace == -1) || (nextFace == start))
					{
						done = true;
					}
					else if (nextFace == end)
					{
						hitEnd = true;
						done = true;
						testLoop.Set(nextFace, TRUE);
					}
					else if (mm->f[nextFace].deg != 4)
						done = true;
					else if (testLoop[nextFace])
					{
						done = true;
					}
					else
					{
						currentFace = nextFace;
						//					DebugPrint(_T("%d "), currentFace);
						testLoop.Set(nextFace, TRUE);
					}

				}
				//				DebugPrint(_T("\n "));
				if (((testLoop.NumberSet() != 0) && ((testLoop.NumberSet() < loop.NumberSet()) || (loop.NumberSet() == 0))) && hitEnd)
				{
					loop = testLoop;
				}
			}
		}
	}

}


void PointToPointProc::RebuildList(int startEdgeIndex)
{
	
	if (mPointToPointPath != nullptr)
		mPointToPointPath->DeleteMe();
	mPointToPointPath = nullptr;

	MNMesh* mm = mpEditPoly->EpModGetOutputMesh(mActiveNode);
	if (mm == nullptr)
		mm = mpEditPoly->EpModGetMesh(mActiveNode);
	if (mm == nullptr) return;

	mPointToPointPath = MaxSDK::PointToPointPath::Create();
	if ( (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX) ||
		(GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE) )
	{
		if (startEdgeIndex == -1)
			mPointToPointPath->SetNumberNodes(mm->numv);
		else
			mPointToPointPath->SetNumberNodes(mm->numv+1);

		Tab<MaxSDK::EdgeDescr> edges;
		for (int i = 0; i < mm->numv; i++)
		{
			if (mm->v[i].GetFlag(MN_DEAD)) continue;
			edges.SetCount(0);
			int ct = mm->vedg[i].Count();
			for (int j = 0; j < ct; j++)
			{
				int edgeIndex = mm->vedg[i][j];
				int vertIndex = mm->e[edgeIndex].v1;
				if (vertIndex == i)
					vertIndex = mm->e[edgeIndex].v2;
				float d = Length(mm->v[i].p - mm->v[vertIndex].p);

				if (edgeIndex == startEdgeIndex)
				{
					MaxSDK::EdgeDescr eDescr(mm->numv, edgeIndex, 0.1f);
					edges.Append(1, &eDescr, 500);
				}
				else
				{
					MaxSDK::EdgeDescr eDescr(vertIndex, edgeIndex, d);
					edges.Append(1, &eDescr, 500);
				}
			}
			mPointToPointPath->AddNode(i,edges);
		}
		if (startEdgeIndex != -1)
		{
			edges.SetCount(0);
			MaxSDK::EdgeDescr eDescr1(mm->e[startEdgeIndex].v1, startEdgeIndex, 0.1f);
			edges.Append(1, &eDescr1, 500);
			MaxSDK::EdgeDescr eDescr2(mm->e[startEdgeIndex].v2, startEdgeIndex, 0.1f);
			edges.Append(1, &eDescr2, 500);
			mPointToPointPath->AddNode(mm->numv, edges);
		}
		if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
			selection.SetSize(mm->numv);
		else selection.SetSize(mm->nume);


	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
	{
		mPointToPointPath->SetNumberNodes(mm->numf);
		Tab<MaxSDK::EdgeDescr> edges;
		BitArray connectedFaces;
		connectedFaces.SetSize(mm->numf);


		for (int i = 0; i < mm->numf; i++)
		{
			MNFace& face = mm->f[i];
			if (face.GetFlag(MN_DEAD)) continue;
			connectedFaces.ClearAll();
			connectedFaces.Set(i,TRUE);

			edges.SetCount(0);
			Point3 faceCenter(0,0,0);
			mm->ComputeCenter (i,faceCenter);
			
			for (int fi = 0; fi < face.deg; fi++)
			{				
				int edgeIndex = face.edg[fi];
				int faceIndex = mm->e[edgeIndex].f1;
				if ((faceIndex == -1) || (faceIndex == i))
					faceIndex = mm->e[edgeIndex].f2;
				if ( (faceIndex != -1) && (faceIndex != i) )
				{
					Point3 center(0,0,0);
					mm->ComputeCenter (faceIndex,center);
					float d = Length(center-faceCenter);
					MaxSDK::EdgeDescr eDescr(faceIndex,0,d);
					connectedFaces.Set(faceIndex,TRUE);
					edges.Append(1,&eDescr,500);
				}
			}
			for (int fi = 0; fi < face.deg; fi++)
			{				
				int vertexIndex = face.vtx[fi];
				int ct = mm->vfac[vertexIndex].Count();

				for (int m = 0; m < ct; m++)
				{
					int faceIndex = mm->vfac[vertexIndex][m];
					if (!connectedFaces[faceIndex])
					{
						Point3 center(0,0,0);
						mm->ComputeCenter (faceIndex,center);
						float d = Length(faceCenter-center);
						MaxSDK::EdgeDescr eDescr(faceIndex,0,d);
						connectedFaces.Set(faceIndex,TRUE);
						edges.Append(1,&eDescr,500);
					}
				}

			}			

			mPointToPointPath->AddNode(i,edges);
		}
		selection.SetSize(mm->numf);

	}

}

void PointToPointProc::StoreOffOriginalSelection()
{
	if (mActiveNode != nullptr)
	{
		MNMesh* mm = mpEditPoly->EpModGetOutputMesh(mActiveNode);
		if (mm == nullptr)
			mm = mpEditPoly->EpModGetMesh(mActiveNode);
		if (mm == nullptr) return;

		if (!mm->GetFlag(MN_MESH_FILLED_IN))
			mm->FillInMesh();
		if (!mm->GetFlag(MN_MESH_VERTS_ORDERED))
			mm->OrderVerts();


		if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_VERTEX)
		{
			mm->getVertexSel(originalSelection);
		}
		else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_EDGE)
		{
			mm->getEdgeSel(originalSelection);
		}
		else if (GetCOREInterface()->GetSubObjectLevel() == EPM_SL_FACE)
		{
			mm->getFaceSel(originalSelection);
		}
	}

}

VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{

	CommandMode* mode = GetCOREInterface()->GetCommandMode();
	if (mode->ID() == CID_POLY_POINT_TO_POINT)
	{
		PointToPointMode* pmode = (PointToPointMode*)mode;
		if (pmode->mShiftLaunched)
		{
			if (!(GetKeyState(VK_SHIFT) & 0x8000))
			{
				pmode->proc.RestoreSelection();
				GetCOREInterface()->PopCommandMode();
			}
		}
	}
}

#define IDT_TIMER 899

void PointToPointMode::EnterMode() {
	mInMode = true;
	proc.mFirstHit = -1;
	proc.mSecondHit = -1;


	proc.StoreOffOriginalSelection();
	GetCOREInterface()->ReplacePrompt(GetString(IDS_EP_POINT_TO_POINT));

	SetTimer(GetCOREInterface()->GetMAXHWnd(), IDT_TIMER, 500, (TIMERPROC)TimerProc); // timer callback

}

void PointToPointMode::ExitMode() {
	mInMode = false;
	if (proc.mPointToPointPath != nullptr)
		proc.mPointToPointPath->DeleteMe();
	proc.mPointToPointPath = nullptr;

	mShiftLaunched = false;
	TSTR emptyString;
	GetCOREInterface()->ReplacePrompt(emptyString);

	KillTimer(GetCOREInterface()->GetMAXHWnd(), IDT_TIMER); // killtimer callback


};


void PointToPointMode::Display(ViewExp *vpt, GraphicsWindow *gw) {

	if (!IsWindowVisible(vpt->getGW()->getHWnd())) // if window's not visible, cp may be invalid
		return;

	TSTR title;
	title.printf(_T("%s"), GetString(IDS_EP_POINT_TO_POINT));

	// turn off z-buffering
	vpt->getGW()->setRndLimits(vpt->getGW()->getRndMode() & ~GW_Z_BUFFER);
	vpt->getGW()->setColor(TEXT_COLOR, GetUIColor(COLOR_VP_LABELS));
	vpt->getGW()->wText(&IPoint3(4, 32, 0), title.data()); //draw it undrneath the viewport text!
}

