 /**********************************************************************  
 *<
	FILE: PolyModes.cpp

	DESCRIPTION: Editable Polygon Mesh Object - Command modes

	CREATED BY: Steve Anderson

	HISTORY: created April 2000

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#include "EPoly.h"
#include "PolyEdit.h"
#include "macrorec.h"
#include "decomp.h"
#include "spline3d.h"
#include "splshape.h"
#include "shape.h"
#include "winutil.h"
#include "dpoint3.h"
#include "dmatrix3.h"
#include "MouseCursors.h"
#include <imenuman.h>

#define EPSILON .0001f

// Some defines for viewport work...

// This is a standard depth used in this module for mapping screen coordinates to world for mouse operations
#define VIEWPORT_MAPPING_DEPTH float(-200)

// These scale the coordinates for adjusting them with the mouse
#define HORIZONTAL_CHAMFER_TENSION_ADJUST_SCALE 50.0f
#define HORIZONTAL_CHAMFER_SEGMENT_ADJUST_SCALE 10.0f

CommandMode * EditPolyObject::getCommandMode (int mode) {
	switch (mode) {
	case epmode_create_vertex: return createVertMode;
	case epmode_create_edge: return createEdgeMode;
	case epmode_create_face: return createFaceMode;
	case epmode_divide_edge: return divideEdgeMode;
	case epmode_divide_face: return divideFaceMode;

	case epmode_extrude_vertex:
	case epmode_extrude_edge:
		return extrudeVEMode;
	case epmode_extrude_face: return extrudeMode;
	case epmode_chamfer_vertex:
	case epmode_chamfer_edge:
		return chamferMode;
	case epmode_bevel: return bevelMode;
	case epmode_inset_face: return insetMode;
	case epmode_outline: return outlineMode;
	case epmode_cut_vertex: return cutMode;
	case epmode_cut_edge: return cutMode;
	case epmode_cut_face: return cutMode;
	case epmode_quickslice: return quickSliceMode;
	case epmode_weld: return weldMode;
	case epmode_lift_from_edge: return liftFromEdgeMode;
	case epmode_pick_lift_edge: return pickLiftEdgeMode;
	case epmode_edit_tri: return editTriMode;
	case epmode_bridge_border: return bridgeBorderMode;
	case epmode_bridge_polygon: return bridgePolyMode;
	case epmode_bridge_edge: return bridgeEdgeMode;
	case epmode_pick_bridge_1: return pickBridgeTarget1;
	case epmode_pick_bridge_2: return pickBridgeTarget2;
	case epmode_turn_edge: return turnEdgeMode;
	case epmode_edit_ss:  return editSSMode;
	case epmode_subobjectpick:  return subobjectPickMode;
	case epmode_point_to_point:  return pointToPointMode;
	}
	return NULL;
}
int EditPolyObject::EpActionGetCommandMode()
{

	if (sliceMode) return epmode_sliceplane;

	if (!ip) return -1;
	CommandMode *currentMode = ip->GetCommandMode ();

	if (currentMode == createVertMode) return epmode_create_vertex;
	if (currentMode == createEdgeMode) return epmode_create_edge;
	if (currentMode == createFaceMode) return epmode_create_face;
	if (currentMode == divideEdgeMode) return epmode_divide_edge;
	if (currentMode == divideFaceMode) return epmode_divide_face;
	if (currentMode == bridgeBorderMode) return epmode_bridge_border;
	if (currentMode == bridgePolyMode) return epmode_bridge_polygon;
	if (currentMode == bridgeEdgeMode) return epmode_bridge_edge;
	if (currentMode == pickLiftEdgeMode) return epmode_pick_lift_edge; //added for grips. wasno easy way to now we are in this mode.
	if (currentMode == extrudeVEMode) {
		if (meshSelLevel[selLevel] == MNM_SL_EDGE) return epmode_extrude_edge;
		else return epmode_extrude_vertex;
	}
	if (currentMode == extrudeMode) return epmode_extrude_face;
	if (currentMode == chamferMode) {
		if (meshSelLevel[selLevel] == MNM_SL_EDGE) return epmode_chamfer_edge;
		else return epmode_chamfer_vertex;
	}
	if (currentMode == bevelMode) return epmode_bevel;
	if (currentMode == insetMode) return epmode_inset_face;
	if (currentMode == outlineMode) return epmode_outline;
	if (currentMode == cutMode)
	{
		if (meshSelLevel[selLevel] == MNM_SL_VERTEX)
			return epmode_cut_vertex;
		else if(meshSelLevel[selLevel]==MNM_SL_EDGE)
			return epmode_cut_edge;
		else 
			return epmode_cut_face;
	}
	if (currentMode == quickSliceMode) return epmode_quickslice;
	if (currentMode == weldMode) return epmode_weld;
	if (currentMode == pickBridgeTarget1) return epmode_pick_bridge_1;
	if (currentMode == pickBridgeTarget2) return epmode_pick_bridge_2;
	if (currentMode == editTriMode) return epmode_edit_tri;
	if (currentMode == turnEdgeMode) return epmode_turn_edge;
	if (currentMode==editSSMode) return epmode_edit_ss;
	if (currentMode==subobjectPickMode) return epmode_subobjectpick;
	if (currentMode==pointToPointMode) return epmode_point_to_point;

	return -1;

}
void EditPolyObject::EpActionToggleCommandMode (int mode) {
	if (!ip) return;
	if ((mode == epmode_sliceplane) && (selLevel == EP_SL_OBJECT)) return;

#ifdef EPOLY_MACRO_RECORD_MODES_AND_DIALOGS
	macroRecorder->FunctionCall(_T("$.EditablePoly.toggleCommandMode"), 1, 0, mr_int, mode);
	macroRecorder->EmitScript ();
#endif

	if (mode == epmode_sliceplane) {
		// Special case.
		ip->ClearPickMode();
		if (sliceMode) ExitSliceMode();
		else {
			EpPreviewCancel ();
			EpfnClosePopupDialog ();	// Cannot have slice mode, settings at same time.
			EnterSliceMode();
		}
		return;
	}

	// Otherwise, make sure we're not in Slice mode:
	if (sliceMode) ExitSliceMode ();

	CommandMode *cmd = getCommandMode (mode);
	if (cmd==NULL) return;
	CommandMode *currentMode = ip->GetCommandMode ();

	switch (mode) {
	case epmode_extrude_vertex:
	case epmode_chamfer_vertex:
		// Special case - only use DeleteMode to exit mode if in correct SO level,
		// Otherwise use EnterCommandMode to switch SO levels and enter mode again.
		if ((currentMode==cmd) && (selLevel==EP_SL_VERTEX)) ip->DeleteMode (cmd);
		else {
			EpPreviewCancel ();
			EpfnClosePopupDialog ();	// Cannot have command mode, settings at same time.
			EnterCommandMode (mode);
		}
		break;
	case epmode_extrude_edge:
	case epmode_chamfer_edge:
		// Special case - only use DeleteMode to exit mode if in correct SO level,
		// Otherwise use EnterCommandMode to switch SO levels and enter mode again.
		if ((currentMode==cmd) && (meshSelLevel[selLevel]==MNM_SL_EDGE)) ip->DeleteMode (cmd);
		else {
			EpPreviewCancel ();
			EpfnClosePopupDialog ();	// Cannot have command mode, settings at same time.
			EnterCommandMode (mode);
		}
		break;
	case epmode_pick_lift_edge:
		// Special case - we do not want to end our preview or close the dialog,
		// since this command mode is controlled from the dialog.
		if (currentMode == cmd) ip->DeleteMode (cmd);
		else EnterCommandMode (mode);
		break;

	case epmode_pick_bridge_1:
	case epmode_pick_bridge_2:
		// Another special case - we do not want to end our preview or close the dialog,
		// since this command mode is controlled from the dialog.
		if (currentMode == cmd) ip->DeleteMode (cmd);
		else EnterCommandMode (mode);
		break;

	default:
		if (currentMode == cmd) ip->DeleteMode (cmd);
		else {
			EpPreviewCancel ();
			EpfnClosePopupDialog ();	// Cannot have command mode, settings at same time.
			EnterCommandMode (mode);
		}
		break;
	}
}

void EditPolyObject::EnterCommandMode(int mode) {
	if (!ip) return;

	// First of all, we don't want to pile up our command modes:
	ExitAllCommandModes (false, false);

	switch (mode) {
	case epmode_create_vertex:
		if (selLevel != EP_SL_VERTEX) ip->SetSubObjectLevel (EP_SL_VERTEX);
		ip->PushCommandMode(createVertMode);
		break;

	case epmode_create_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EP_SL_EDGE);
		ip->PushCommandMode (createEdgeMode);
		break;

	case epmode_create_face:
	
		if (selLevel != EP_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (createFaceMode);
		break;

	case epmode_divide_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EP_SL_EDGE);
		ip->PushCommandMode (divideEdgeMode);
		break;

	case epmode_divide_face:
		if (selLevel < EP_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (divideFaceMode);
		break;

	case epmode_extrude_vertex:
		if (selLevel != EP_SL_VERTEX) ip->SetSubObjectLevel (EP_SL_VERTEX);
	
		ip->PushCommandMode (extrudeVEMode);
		break;

	case epmode_extrude_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EP_SL_EDGE);
	
		ip->PushCommandMode (extrudeVEMode);
		break;

	case epmode_extrude_face:
		if (selLevel < EP_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (extrudeMode);
		break;

	case epmode_chamfer_vertex:
		if (selLevel != EP_SL_VERTEX) ip->SetSubObjectLevel (EP_SL_VERTEX);
		ip->PushCommandMode (chamferMode);
		break;

	case epmode_chamfer_edge:
		if (meshSelLevel[selLevel] != MNM_SL_EDGE) ip->SetSubObjectLevel (EP_SL_EDGE);
		ip->PushCommandMode (chamferMode);
		break;

	case epmode_bevel:
		if (selLevel < EP_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (bevelMode);
		break;


	case epmode_inset_face:
		if (meshSelLevel[selLevel] != MNM_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (insetMode);
		break;

	case epmode_outline:
		if (meshSelLevel[selLevel] != MNM_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (outlineMode);
		break;
	case epmode_edit_ss:
		ip->PushCommandMode (editSSMode);
		break;

	//(Should never have been 3 of these.)
	case epmode_cut_vertex:
	case epmode_cut_edge:
	case epmode_cut_face:
		ip->PushCommandMode (cutMode);
		break;


	case epmode_quickslice:
		ip->PushCommandMode (quickSliceMode);
		break;

	case epmode_weld:
		if (selLevel > EP_SL_BORDER) ip->SetSubObjectLevel (EP_SL_VERTEX);
		if (selLevel == EP_SL_BORDER) ip->SetSubObjectLevel (EP_SL_EDGE);
		ip->PushCommandMode (weldMode);
		break;


	case epmode_lift_from_edge:
		if (selLevel != EP_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (liftFromEdgeMode);
		break;


	case epmode_pick_lift_edge:
		ip->PushCommandMode (pickLiftEdgeMode);
		break;

	case epmode_edit_tri:
		if (selLevel < EP_SL_EDGE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (editTriMode);
		break;

	case epmode_turn_edge:
		if (selLevel<EP_SL_EDGE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (turnEdgeMode);
		break;

	case epmode_bridge_border:
		if (selLevel != EP_SL_BORDER) ip->SetSubObjectLevel (EP_SL_BORDER);
		ip->PushCommandMode (bridgeBorderMode);
		break;

	case epmode_bridge_edge:
		if (selLevel != EP_SL_EDGE) 
			ip->SetSubObjectLevel (EP_SL_EDGE);
		ip->PushCommandMode (bridgeEdgeMode);
		break;
		
	case epmode_bridge_polygon:
		if (selLevel != EP_SL_FACE) ip->SetSubObjectLevel (EP_SL_FACE);
		ip->PushCommandMode (bridgePolyMode);
		break;

	case epmode_pick_bridge_1:
		ip->PushCommandMode (pickBridgeTarget1);
		break;
		
	case epmode_pick_bridge_2:
		ip->PushCommandMode (pickBridgeTarget2);
		break;
	case epmode_subobjectpick:
		ip->PushCommandMode (subobjectPickMode);
		break;
	case epmode_point_to_point:
		ip->PushCommandMode (pointToPointMode);
		break;


	}
}

void EditPolyObject::EpActionEnterPickMode (int mode) {
	if (!ip) return;

	// Make sure we're not in Slice mode:
	if (sliceMode) ExitSliceMode ();

#ifdef EPOLY_MACRO_RECORD_MODES_AND_DIALOGS
	macroRecorder->FunctionCall(_T("$.EditablePoly.enterPickMode"), 1, 0, mr_int, mode);
	macroRecorder->EmitScript ();
#endif

	switch (mode) {
	case epmode_attach:
		ip->SetPickMode (attachPickMode);
		break;
	case epmode_pick_shape:
		if (GetMNSelLevel () != MNM_SL_FACE) return;
	
		ip->SetPickMode (shapePickMode);
		break;
	}
}

int EditPolyObject::EpActionGetPickMode () {
	if (!ip) return -1;
	PickModeCallback *currentMode = ip->GetCurPickMode ();

	if (currentMode == attachPickMode) return epmode_attach;
	if (currentMode == shapePickMode) return epmode_pick_shape;

	return -1;
}

//------------Command modes & Mouse procs----------------------

HitRecord *PickEdgeMouseProc::HitTestEdges (IPoint2 &m, ViewExp& vpt, float *prop, 
											Point3 *snapPoint) {

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();

	EditPolyObject *pEditPoly = (EditPolyObject *) mpEPoly;	// STEVE: make go away someday.
	if (pEditPoly->GetEPolySelLevel() != EP_SL_BORDER)
		pEditPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt.ToPointer());
	if (pEditPoly->GetEPolySelLevel() != EP_SL_BORDER)
		pEditPoly->ClearHitLevelOverride ();
	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();
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
	vpt.MapScreenToWorldRay ((float)m.x, (float)m.y, r);
	if (!snapPoint) snapPoint = &(r.p);
	Point3 Zdir = Normalize (r.dir);

	MNMesh & mm = *(mpEPoly->GetMeshPtr());
	Point3 A = obj2world * mm.v[mm.e[ee].v1].p;
	Point3 B = obj2world * mm.v[mm.e[ee].v2].p;
	Point3 Xdir = B-A;
	Xdir -= DotProd(Xdir, Zdir)*Zdir;

	// OLP Avoid division by zero
	float XdirLength = LengthSquared(Xdir);
	if (XdirLength == 0.0f) {
		*prop = 1;
		return hr;
	}
	*prop = DotProd (Xdir, *snapPoint-A) / XdirLength;

	if (*prop<.0001f) *prop=0;
	if (*prop>.9999f) *prop=1;
	return hr;
}

int PickEdgeMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	float prop;
	Point3 snapPoint;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}

		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestEdges (m, vpt, &prop, &snapPoint);
		
		if (!hr) break;
		if (!EdgePick (hr->hitInfo, prop)) {
			// False return value indicates exit mode.
			ip->PopCommandMode ();
			return false;
		}
		return true;
		} // case
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

		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);//|SNAP_SEL_OBJS_ONLY);
		if (HitTestEdges(m,vpt,NULL,NULL)) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		break;
		}
	}

	return TRUE;
}

// --------------------------------------------------------

HitRecord *PickFaceMouseProc::HitTestFaces (IPoint2 &m, ViewExp& vpt) {

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();

	EditPolyObject *pEditPoly = (EditPolyObject *) mpEPoly;	// STEVE: make go away someday.
	pEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt.ToPointer());
	pEditPoly->ClearHitLevelOverride ();
	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();
	HitRecord *hr = hitLog.ClosestHit();
	return hr;
}

void PickFaceMouseProc::ProjectHitToFace (IPoint2 &m, ViewExp& vpt,
										  HitRecord *hr, Point3 *snapPoint) {
	if (!hr) return ;

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}

	// Find subtriangle, barycentric coordinates of hit in face.
	face = hr->hitInfo;
	Matrix3 obj2world = hr->nodeRef->GetObjectTM (ip->GetTime ());

	Ray r;
	vpt.MapScreenToWorldRay ((float)m.x, (float)m.y, r);
	if (!snapPoint) snapPoint = &(r.p);
	Point3 Zdir = Normalize (r.dir);

	// Find an approximate location for the point on the surface we hit:
	// Get the average normal for the face, thus the plane, and intersect.
	Point3 intersect;
	MNMesh & mm = *(mpEPoly->GetMeshPtr());
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

int PickFaceMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}

		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestFaces (m, vpt);
		ProjectHitToFace (m, vpt, hr, &snapPoint);
		if (hr) FacePick ();
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
		if (HitTestFaces(m,vpt)) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		break;
		}
	}

	return TRUE;
}

// -------------------------------------------------------

HitRecord *ConnectVertsMouseProc::HitTestVertices (IPoint2 & m, ViewExp& vpt) {
	
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();

	EditPolyObject *pEditPoly = (EditPolyObject *) mpEPoly;	// STEVE: make go away someday.
	pEditPoly->SetHitLevelOverride (SUBHIT_MNVERTS);
	ip->SubObHitTest(ip->GetTime(),HITTYPE_POINT,0, 0, &m, vpt.ToPointer());
	pEditPoly->ClearHitLevelOverride ();
	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();
	HitRecord *hr = hitLog.ClosestHit();
	if (v1<0) {
		// can't accept vertices on no faces.
		if (mpEPoly->GetMeshPtr()->vfac[hr->hitInfo].Count() == 0) return NULL;
		return hr;
	}

	// Otherwise, we're looking for a vertex on v1's faces - these are listed in neighbors.
	for (int i=0; i<neighbors.Count(); i++) 
	{
		if (neighbors[i] == hr->hitInfo) 
			return hr;
	}

	return NULL;
}

void ConnectVertsMouseProc::DrawDiag (HWND hWnd, const IPoint2 & m, bool bErase, bool bPresent) {
	if (v1<0) return;

	IPoint2 pts[2];
	pts[0].x = m1.x;
	pts[0].y = m1.y;
	pts[1].x = m.x;
	pts[1].y = m.y;
	XORDottedPolyline(hWnd, 2, pts, 0, bErase, !bPresent);
}

void ConnectVertsMouseProc::SetV1 (int vv) {
	v1 = vv;
	neighbors.ZeroCount();
	Tab<int> & vf = mpEPoly->GetMeshPtr()->vfac[vv];
	Tab<int> & ve = mpEPoly->GetMeshPtr()->vedg[vv];
	// Add to neighbors all the vertices that share faces (but no edges) with this one:
	int i,j,k;
	for (i=0; i<vf.Count(); i++) {
		MNFace & mf = mpEPoly->GetMeshPtr()->f[vf[i]];
		for (j=0; j<mf.deg; j++) {
			// Do not consider v1 a neighbor:
			if (mf.vtx[j] == v1) continue;

			// Filter out those that share an edge with v1:
			for (k=0; k<ve.Count(); k++) {
				if (mpEPoly->GetMeshPtr()->e[ve[k]].OtherVert (vv) == mf.vtx[j]) break;
			}
			if (k<ve.Count()) continue;

			// Add to neighbor list.
			neighbors.Append (1, &(mf.vtx[j]), 4);
		}
	}
}

int ConnectVertsMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord *hr = NULL;
	Point3 snapPoint;

	switch (msg) {
	case MOUSE_INIT:
		v1 = v2 = -1;
		neighbors.ZeroCount ();
		break;

	case MOUSE_PROPCLICK:
		DrawDiag (hwnd, lastm, true, true);	// erase last dotted line.
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestVertices (m, vpt);
		if (!hr) break;
		if (v1<0) {
			SetV1 (hr->hitInfo);
			m1 = m;
			lastm = m;
			DrawDiag (hwnd, m, false, true);
			break;
		}
		// Otherwise, we've found a connection.
		DrawDiag (hwnd, lastm, true, true);	// erase last dotted line.
		v2 = hr->hitInfo;
		VertConnect ();
		v1 = -1;
		return FALSE;	// Done with this connection.
		} // case
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
		hr=HitTestVertices(m,vpt);
		
		if (hr!=NULL) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		
		DrawDiag (hwnd, lastm, true, false);
		DrawDiag (hwnd, m, false, true);
		lastm = m;
		break;
		}
	}

	return TRUE;
}

// -------------------------------------------------------

static HCURSOR hCurCreateVert = NULL;

void CreateVertCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void CreateVertCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

int CreateVertMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	if (!hCurCreateVert) hCurCreateVert = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CreateVertex);

	ViewExp& vpt = ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Matrix3 ctm;
	Point3 pt;
	IPoint2 m2;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		ip->SetActiveViewport(hwnd);
		vpt.GetConstructionTM(ctm);
		pt = vpt.SnapPoint (m, m2, &ctm);
		pt = pt * ctm;
		theHold.Begin ();
		if (mpEPoly->EpfnCreateVertex(pt)<0) {
			theHold.Cancel ();
		} else {
			theHold.Accept (GetString (IDS_CREATE_VERTEX));
			mpEPoly->RefreshScreen ();
		}
		break;

	case MOUSE_FREEMOVE:
		SetCursor(hCurCreateVert);
		vpt.SnapPreview(m, m, NULL, SNAP_FORCE_3D_RESULT);
		break;
	}

	return TRUE;
}

//----------------------------------------------------------

void CreateEdgeCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void CreateEdgeCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck (FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void CreateEdgeMouseProc::VertConnect () {
	theHold.Begin();
	if (mpEPoly->EpfnCreateEdge (v1, v2) < 0) {
		theHold.Cancel ();
		return;
	}
	theHold.Accept (GetString (IDS_CREATE_EDGE));
	mpEPoly->RefreshScreen ();
}

//----------------------------------------------------------

void CreateFaceCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void CreateFaceCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CREATE));
		but->SetCheck (FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	if (mpEditPoly->ip != nullptr)
		mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void CreateFaceCMode::Display (GraphicsWindow *gw) {
	proc.DrawEstablishedFace(gw);
}
Point3 CreateFaceMouseProc::GetVertexWP(int in_vertex)
{
	ModContextList mcList;
	INodeTab nodes;
	GetInterface()->GetModContexts(mcList,nodes);

	MNMesh *pMesh = &m_EPoly->mm;

	if(nodes[0]&&in_vertex>=0&&in_vertex< pMesh->VNum())
		return nodes[0]->GetObjectTM(GetInterface()->GetTime()) * pMesh->P(in_vertex);
	return Point3(0.0f,0.0f,0.0f);
}
bool CreateFaceMouseProc::HitTestVertex(IPoint2 in_mouse, ViewExp& in_viewport, INode*& out_node, int& out_vertex) {

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
	m_EPoly->SetHitLevelOverride (SUBHIT_MNVERTS|SUBHIT_OPENONLY);
	GetInterface()->SubObHitTest(GetInterface()->GetTime(), HITTYPE_POINT, 0, 0, &in_mouse, in_viewport.ToPointer());
	m_EPoly->ClearHitLevelOverride ();

	HitRecord* l_closest = in_viewport.GetSubObjHitList().ClosestHit();
	if (l_closest != 0) { 
		out_vertex = l_closest->hitInfo;
		out_node = l_closest->nodeRef;
	}
	return l_closest != 0;
}

bool CreateFaceMouseProc::CreateVertex(const Point3& in_worldLocation, int& out_vertex) {
	out_vertex = m_EPoly->EpfnCreateVertex(in_worldLocation, false);
	return out_vertex >= 0;
}

bool CreateFaceMouseProc::CreateFace(MaxSDK::Array<int>& in_vertices) {
	return m_EPoly->EpfnCreateFace(in_vertices.asArrayPtr(), static_cast<int>(in_vertices.length())) >= 0;
}

CreateFaceMouseProc::CreateFaceMouseProc( EditPolyObject* in_e, IObjParam* in_i ) 
: CreateFaceMouseProcTemplate(in_i), m_EPoly(in_e) {
	if (!hCurCreateVert) hCurCreateVert = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CreateVertex);

}

void CreateFaceMouseProc::ShowCreateVertexCursor() {
	SetCursor(hCurCreateVert);
}

BOOL AttachPickMode::Filter(INode *node) {
	if (!mpEditPoly) return false;
	if (!ip) return false;
	if (!node) return FALSE;

	// Make sure the node does not depend on us
	node->BeginDependencyTest();
	mpEditPoly->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) return FALSE;

	ObjectState os = node->GetObjectRef()->Eval(ip->GetTime());
	if (os.obj->IsSubClassOf(polyObjectClassID)) return TRUE;
	if (os.obj->CanConvertToType(polyObjectClassID)) return TRUE;
	return FALSE;
}

BOOL AttachPickMode::HitTest(IObjParam *ip, HWND hWnd, ViewExp * /*vpt*/, IPoint2 m,int flags) {
	if (!mpEditPoly) return false;
	if (!ip) return false;
	return ip->PickNode(hWnd,m,this) ? TRUE : FALSE;
}

BOOL AttachPickMode::Pick(IObjParam *ip,ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	if (!mpEditPoly) return false;
	INode *node = vpt->GetClosestHit();
	if (!Filter(node)) return FALSE;

	ModContextList mcList;
	INodeTab nodes;
	ip->GetModContexts(mcList,nodes);

	BOOL ret = TRUE;
	if (nodes[0]->GetMtl() && node->GetMtl() && (nodes[0]->GetMtl()!=node->GetMtl())) {
		//the material attach dialog clears pick modes to prevent crash issues
		//so as a hack push the move mode and then call the dialog and the pop the command stack
		//to end up back where we were
		mSuspendUI = true;
		ip->PushStdCommandMode(CID_OBJMOVE);
		ret = DoAttachMatOptionDialog (ip, mpEditPoly);
		ip->PopCommandMode();
		mSuspendUI = false;
	}
	if (!ret) {
		nodes.DisposeTemporary ();
		return FALSE;
	}

	bool canUndo = TRUE;
	mpEditPoly->EpfnAttach (node, canUndo, nodes[0], ip->GetTime());
	if (!canUndo) GetSystemSetting (SYSSET_CLEAR_UNDO);
	// Rechecking mpEditPoly in case deleting the picked node exited edit mode 
	if (mpEditPoly)
		mpEditPoly->RefreshScreen ();
	nodes.DisposeTemporary ();
	return FALSE;
}

void AttachPickMode::EnterMode(IObjParam *ip) {
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

void AttachPickMode::ExitMode(IObjParam *ip) {
	if (!mpEditPoly) return;
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

int AttachHitByName::filter(INode *node) {
	if (!node) return FALSE;

	// Make sure the node does not depend on this modifier.
	node->BeginDependencyTest();
	mpEditPoly->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) return FALSE;

	ObjectState os = node->GetObjectRef()->Eval(mpEditPoly->ip->GetTime());
	if (os.obj->IsSubClassOf(polyObjectClassID)) return TRUE;
	if (os.obj->CanConvertToType(polyObjectClassID)) return TRUE;
	return FALSE;
}

void AttachHitByName::proc(INodeTab &nodeTab) {
	if (inProc) return;
	if (!mpEditPoly->ip) return;
	inProc = TRUE;
	ModContextList mcList;
	INodeTab nodes;
	mpEditPoly->ip->GetModContexts (mcList, nodes);
	BOOL ret = TRUE;
	if (nodes[0]->GetMtl()) {
      int i;
		for (i=0; i<nodeTab.Count(); i++) {
			if (nodeTab[i]->GetMtl() && (nodes[0]->GetMtl()!=nodeTab[i]->GetMtl())) break;
		}
		if (i<nodeTab.Count()) ret = DoAttachMatOptionDialog ((IObjParam *)mpEditPoly->ip, mpEditPoly);
		if (!mpEditPoly->ip) ret = FALSE;
	}
	inProc = FALSE;
	if (ret) mpEditPoly->EpfnMultiAttach (nodeTab, nodes[0], mpEditPoly->ip->GetTime ());
	nodes.DisposeTemporary ();
}

//-----------------------------------------------------------------------/

BOOL ShapePickMode::Filter(INode *node) {
	if (!mpEditPoly) return false;
	if (!ip) return false;
	if (!node) return FALSE;

	// Make sure the node does not depend on us
	node->BeginDependencyTest();
	mpEditPoly->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) return FALSE;

	ObjectState os = node->GetObjectRef()->Eval(ip->GetTime());
	if (os.obj->IsSubClassOf(splineShapeClassID)) return TRUE;
	if (os.obj->CanConvertToType(splineShapeClassID)) return TRUE;
	return FALSE;
}

BOOL ShapePickMode::HitTest(IObjParam *ip, HWND hWnd, ViewExp * /*vpt*/, IPoint2 m,int flags) {
	if (!mpEditPoly) return false;
	if (!ip) return false;
	return ip->PickNode(hWnd,m,this) ? TRUE : FALSE;
}

BOOL ShapePickMode::Pick(IObjParam *ip,ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	mpEditPoly->RegisterCMChangedForGrip();
	if (!mpEditPoly) return false;
	INode *node = vpt->GetClosestHit();
	if (!Filter(node)) return FALSE;
	mpEditPoly->getParamBlock()->SetValue (ep_extrude_spline_node, ip->GetTime(), node);
	if (!mpEditPoly->EpPreviewOn()) {
		// We're not in preview mode - do the extrusion.
		mpEditPoly->EpActionButtonOp (epop_extrude_along_spline);
	} else {
	mpEditPoly->RefreshScreen ();
	}
	return TRUE;
}

void ShapePickMode::EnterMode(IObjParam *ip) {
	HWND hOp = mpEditPoly->GetDlgHandle (ep_settings);
	ICustButton *but = NULL;
	if (hOp) but = GetICustButton (GetDlgItem (hOp, IDC_EXTRUDE_PICK_SPLINE));
	if (but) {
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
		but = NULL;
	}
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hGeom) but = GetICustButton (GetDlgItem (hGeom, IDC_EXTRUDE_ALONG_SPLINE));
	if (but) {
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
		but = NULL;
	}
}

void ShapePickMode::ExitMode(IObjParam *ip) {
	HWND hOp = mpEditPoly->GetDlgHandle (ep_settings);
	ICustButton *but = NULL;
	if (hOp) but = GetICustButton (GetDlgItem (hOp, IDC_EXTRUDE_PICK_SPLINE));
	if (but) {
		but->SetCheck(false);
		ReleaseICustButton(but);
		but = NULL;
	}
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hGeom) but = GetICustButton (GetDlgItem (hGeom, IDC_EXTRUDE_ALONG_SPLINE));
	if (but) {
		but->SetCheck(false);
		ReleaseICustButton(but);
		but = NULL;
	}
}

//----------------------------------------------------------

// Divide edge modifies two faces; creates a new vertex and a new edge.
bool DivideEdgeProc::EdgePick (int edge, float prop) {
	theHold.Begin ();
	mpEPoly->EpfnDivideEdge (edge, prop);
	theHold.Accept (GetString (IDS_INSERT_VERTEX));
	mpEPoly->RefreshScreen ();
	return true;	// false = exit mode when done; true = stay in mode.
}

void DivideEdgeCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_edge);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void DivideEdgeCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_edge);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
		but->SetCheck(FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

//----------------------------------------------------------

void DivideFaceProc::FacePick () {
	theHold.Begin ();
	mpEPoly->EpfnDivideFace (face, bary);
	theHold.Accept (GetString (IDS_INSERT_VERTEX));
	mpEPoly->RefreshScreen ();
}

void DivideFaceCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);		
	mpEditPoly->SetDisplayLevelOverride (MNDISP_VERTTICKS);
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void DivideFaceCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSERT_VERTEX));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
	mpEditPoly->ClearDisplayLevelOverride ();
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

// ------------------------------------------------------

int ExtrudeProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {	
	ViewExp& vpt=ip->GetViewExp (hwnd);
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
			mpEditPoly->EpfnBeginExtrude(mpEditPoly->GetMNSelLevel(), MN_SEL, ip->GetTime());
			om = m;
		} else {
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			mpEditPoly->EpfnEndExtrude (true, ip->GetTime());
		}
		break;

	case MOUSE_MOVE:
		p0 = vpt.MapScreenToView (om,VIEWPORT_MAPPING_DEPTH);
		m2.x = om.x;
		m2.y = m.y;
		p1 = vpt.MapScreenToView (m2,VIEWPORT_MAPPING_DEPTH);
		height = Length (p1-p0);
		if (m.y > om.y) height *= -1.0f;
		mpEditPoly->EpfnDragExtrude (height, ip->GetTime());
		mpEditPoly->getParamBlock()->SetValue (ep_face_extrude_height, ip->GetTime(), height);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		mpEditPoly->EpfnEndExtrude (false, ip->GetTime ());
		ip->RedrawViews (ip->GetTime(), REDRAW_END);
		break;
	}

	return TRUE;
}

HCURSOR ExtrudeSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Extrude);
	return hCur; 
}

void ExtrudeCMode::EnterMode() {
	mpEditPoly->SuspendContraints (true);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	if (but) {
		but->SetCheck (true);
		ReleaseICustButton(but);
	} else {
		DebugPrint(_T("Editable Poly: we're entering Extrude mode, but we can't find the extrude button!\n"));
		DbgAssert (0);
	}
}

void ExtrudeCMode::ExitMode() {
	mpEditPoly->SuspendContraints (false);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	if (but) {
		but->SetCheck(FALSE);
		ReleaseICustButton(but);
	} else {
		DebugPrint(_T("Editable Poly: we're exiting Extrude mode, but we can't find the extrude button!\n"));
		DbgAssert (0);
	}
}

// ------------------------------------------------------

int ExtrudeVEMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	ViewExp& vpt=ip->GetViewExp (hwnd);
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
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch (point) {
		case 0:
			m0 = m;
			m0set = TRUE;
			switch (mpEditPoly->GetMNSelLevel()) {
			case MNM_SL_VERTEX:
				mpEditPoly->getParamBlock()->SetValue (ep_vertex_extrude_height, TimeValue(0), 0.0f);
				mpEditPoly->getParamBlock()->SetValue (ep_vertex_extrude_width, TimeValue(0), 0.0f);
				break;
			case MNM_SL_EDGE:
				mpEditPoly->getParamBlock()->SetValue (ep_edge_extrude_height, TimeValue(0), 0.0f);
				mpEditPoly->getParamBlock()->SetValue (ep_edge_extrude_width, TimeValue(0), 0.0f);
				break;
			}
			mpEditPoly->EpPreviewBegin (epop_extrude);
			break;
		case 1:
			p0 = vpt.MapScreenToView (m0, VIEWPORT_MAPPING_DEPTH);
			m1.x = m.x;
			m1.y = m0.y;
			p1 = vpt.MapScreenToView (m1, VIEWPORT_MAPPING_DEPTH);
			m2.x = m0.x;
			m2.y = m.y;
			p2 = vpt.MapScreenToView (m2, VIEWPORT_MAPPING_DEPTH);
			width = Length (p0 - p1);
			height = Length (p0 - p2);
			if (m.y > m0.y) height *= -1.0f;
			switch (mpEditPoly->GetMNSelLevel()) {
			case MNM_SL_VERTEX:
				mpEditPoly->getParamBlock()->SetValue (ep_vertex_extrude_width, ip->GetTime(), width);
				mpEditPoly->getParamBlock()->SetValue (ep_vertex_extrude_height, ip->GetTime(), height);
				break;
			case MNM_SL_EDGE:
				mpEditPoly->getParamBlock()->SetValue (ep_edge_extrude_width, ip->GetTime(), width);
				mpEditPoly->getParamBlock()->SetValue (ep_edge_extrude_height, ip->GetTime(), height);
				break;
			}
			mpEditPoly->EpPreviewAccept ();
			ip->RedrawViews(ip->GetTime());
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!m0set) break;
		p0 = vpt.MapScreenToView (m0, VIEWPORT_MAPPING_DEPTH);
		m1.x = m.x;
		m1.y = m0.y;
		p1 = vpt.MapScreenToView (m1, VIEWPORT_MAPPING_DEPTH);
		m2.x = m0.x;
		m2.y = m.y;
		p2 = vpt.MapScreenToView (m2, VIEWPORT_MAPPING_DEPTH);
		width = Length (p0 - p1);
		height = Length (p0 - p2);
		if (m.y > m0.y) height *= -1.0f;
		switch (mpEditPoly->GetMNSelLevel()) {
		case MNM_SL_VERTEX:
			mpEditPoly->getParamBlock()->SetValue (ep_vertex_extrude_width, ip->GetTime(), width);
			mpEditPoly->getParamBlock()->SetValue (ep_vertex_extrude_height, ip->GetTime(), height);
			break;
		case MNM_SL_EDGE:
			mpEditPoly->getParamBlock()->SetValue (ep_edge_extrude_width, ip->GetTime(), width);
			mpEditPoly->getParamBlock()->SetValue (ep_edge_extrude_height, ip->GetTime(), height);
			break;
		}
		mpEditPoly->EpPreviewSetDragging (true);
		mpEditPoly->EpPreviewInvalidate ();
		mpEditPoly->EpPreviewSetDragging (false);

		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		mpEditPoly->EpPreviewCancel ();
		ip->RedrawViews (ip->GetTime(), REDRAW_END);
		m0set = FALSE;
		break;
	}

	
	return TRUE;
}

HCURSOR ExtrudeVESelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if (!hCur) {
		if (mpEPoly->GetMNSelLevel() == MNM_SL_EDGE) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::EdgeExt);
		else hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ExtrudeVertex);
	}
	return hCur; 
}

void ExtrudeVECMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_vertex);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void ExtrudeVECMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_vertex);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_EXTRUDE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

int BevelMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {	
	ViewExp& vpt=ip->GetViewExp (hwnd);
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
			mpEditPoly->EpfnBeginBevel (mpEditPoly->GetMNSelLevel(), MN_SEL, true, ip->GetTime());
			break;
		case 1:
			m1 = m;
			m1set = TRUE;
			p0 = vpt.MapScreenToView (m0, VIEWPORT_MAPPING_DEPTH);
			m2.x = m0.x;
			m2.y = m.y;
			p1 = vpt.MapScreenToView (m2, VIEWPORT_MAPPING_DEPTH);
			height = Length (p0-p1);
			if (m1.y > m0.y) height *= -1.0f;
			mpEditPoly->EpfnDragBevel (0.0f, height, ip->GetTime());
			mpEditPoly->getParamBlock()->SetValue (ep_bevel_height, ip->GetTime(), height);
			break;
		case 2:
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			mpEditPoly->EpfnEndBevel (true, ip->GetTime());
			m1set = m0set = FALSE;
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!m0set) break;
		m2.y = m.y;
		if (!m1set) {
			p0 = vpt.MapScreenToView (m0, VIEWPORT_MAPPING_DEPTH);
			m2.x = m0.x;
			p1 = vpt.MapScreenToView (m2, VIEWPORT_MAPPING_DEPTH);
			height = Length (p1-p0);
			if (m.y > m0.y) height *= -1.0f;
			mpEditPoly->EpfnDragBevel (0.0f, height, ip->GetTime());
			mpEditPoly->getParamBlock()->SetValue (ep_bevel_height, ip->GetTime(), height);
		} else {
			p0 = vpt.MapScreenToView (m1, VIEWPORT_MAPPING_DEPTH);
			m2.x = m1.x;
			p1 = vpt.MapScreenToView (m2, VIEWPORT_MAPPING_DEPTH);
			float outline = Length (p1-p0);
			if (m.y > m1.y) outline *= -1.0f;
			mpEditPoly->EpfnDragBevel (outline, height, ip->GetTime());
			mpEditPoly->getParamBlock()->SetValue (ep_bevel_outline, ip->GetTime(), outline);
		}

		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		mpEditPoly->EpfnEndBevel (false, ip->GetTime());
		ip->RedrawViews(ip->GetTime(),REDRAW_END);
		m1set = m0set = FALSE;
		break;
	}

	
	return TRUE;
}

HCURSOR BevelSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Bevel);
	return hCur;
}

void BevelCMode::EnterMode() {
	mpEditPoly->SuspendContraints (true);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BEVEL));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void BevelCMode::ExitMode() {
	mpEditPoly->SuspendContraints (false);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BEVEL));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

int InsetMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	ViewExp& vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1;
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
			mpEditPoly->EpfnBeginInset (mpEditPoly->GetMNSelLevel(), MN_SEL, ip->GetTime());
			break;
		case 1:
			ip->RedrawViews(ip->GetTime(),REDRAW_END);
			mpEditPoly->EpfnEndInset (true, ip->GetTime());
			m0set = FALSE;
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!m0set) break;
		p0 = vpt.MapScreenToView (m0, VIEWPORT_MAPPING_DEPTH);
		p1 = vpt.MapScreenToView (m, VIEWPORT_MAPPING_DEPTH);
		inset = Length (p1-p0);
		mpEditPoly->EpfnDragInset (inset, ip->GetTime());
		mpEditPoly->getParamBlock()->SetValue (ep_inset, ip->GetTime(), inset);
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		mpEditPoly->EpfnEndInset (false, ip->GetTime());
		ip->RedrawViews(ip->GetTime(),REDRAW_END);
		m0set = FALSE;
		break;
	}

	
	return TRUE;
}

HCURSOR InsetSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Inset);
	return hCur;
}

void InsetCMode::EnterMode() {
	mpEditPoly->SuspendContraints (true);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSET));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void InsetCMode::ExitMode() {
	mpEditPoly->SuspendContraints (false);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_INSET));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

int OutlineMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	ViewExp& vpt=ip->GetViewExp (hwnd);
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
		if (mpEditPoly->EpPreviewOn()) mpEditPoly->EpPreviewCancel ();
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch (point) {
		case 0:
			m0 = m;
			m0set = TRUE;
			// Prevent flash of last outline value:
			mpEditPoly->getParamBlock()->SetValue (ep_outline, ip->GetTime(), 0.0f);
			mpEditPoly->EpPreviewBegin (epop_outline);
			break;
		case 1:
			mpEditPoly->EpPreviewAccept();
			m0set = FALSE;
			break;
		}
		break;

	case MOUSE_MOVE:
		if (!m0set) break;

		// Get signed right/left distance from original point:
		p0 = vpt.MapScreenToView (m0, VIEWPORT_MAPPING_DEPTH);
		m1.y = m.y;
		m1.x = m0.x;
		p1 = vpt.MapScreenToView (m1, VIEWPORT_MAPPING_DEPTH);
		outline = Length (p1-p0);
		if (m.y > m0.y) outline *= -1.0f;

		mpEditPoly->EpPreviewSetDragging (true);
		mpEditPoly->getParamBlock()->SetValue (ep_outline, ip->GetTime(), outline);
		mpEditPoly->EpPreviewSetDragging (false);
		mpEditPoly->RefreshScreen ();
		break;

	case MOUSE_ABORT:
		mpEditPoly->EpPreviewCancel ();
		m0set = FALSE;
		break;
	}

	
	return TRUE;
}

HCURSOR OutlineSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hCur = NULL;
	if ( !hCur ) hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Outline);
	return hCur;
}

void OutlineCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_OUTLINE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void OutlineCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_OUTLINE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// ------------------------------------------------------

int ChamferMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {	
	ViewExp& vpt=ip->GetViewExp (hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 p0, p1;
	static float chamfer;
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
		mpEditPoly->getParamBlock()->GetValue (ep_edge_chamfer_type, ip->GetTime(), chamferType, FOREVER);

	// Look for the Alt key
	bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) ? true : false;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		switch(point) {
		case 0:
			mpEditPoly->EpfnBeginChamfer (selLevel, ip->GetTime());
			chamfer = 0;
			if(chamferType == EP_QUAD_EDGE_CHAMFER) {
				mpEditPoly->getParamBlock()->GetValue (ep_edge_chamfer_tension, ip->GetTime(), baseTension, FOREVER);
				mpEditPoly->getParamBlock()->GetValue (ep_edge_chamfer_segments, ip->GetTime(), baseSegments, FOREVER);
			}
			om = m;
			cumDelta = IPoint2(0,0);
			cumSegDelta = 0;
			cumTensDelta = 0.0f;
			wasAltPressed = false;
			break;
		case 1:
			switch (selLevel) {
			case MNM_SL_VERTEX:
				mpEditPoly->getParamBlock()->SetValue (ep_vertex_chamfer, ip->GetTime(), chamfer);
				break;
			case MNM_SL_EDGE:
				mpEditPoly->getParamBlock()->SetValue (ep_edge_chamfer, ip->GetTime(), chamfer);
				break;
			}
			om = m;
			cumDelta = IPoint2(0,0);
			if(chamferType == EP_STANDARD_EDGE_CHAMFER) {
				ip->RedrawViews (ip->GetTime(),REDRAW_END);
				mpEditPoly->EpfnEndChamfer (true, ip->GetTime());
			}
			break;
		case 2:
			wasAltPressed = false;
			ip->RedrawViews (ip->GetTime(),REDRAW_END);
			mpEditPoly->EpfnEndChamfer (true, ip->GetTime());
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
				}
			}
			p0 = vpt.MapScreenToView(om, VIEWPORT_MAPPING_DEPTH);
			m1.x = m.x;
			m1.y = om.y;
			p1 = vpt.MapScreenToView(m1, VIEWPORT_MAPPING_DEPTH);
			segDelta = (int)(Length (p1 - p0) / HORIZONTAL_CHAMFER_SEGMENT_ADJUST_SCALE);
			if (m.x < om.x)
				segDelta = -segDelta;
			segments = baseSegments + cumSegDelta + segDelta;
			if(segments < 1)
				segments = 1;
			mpEditPoly->getParamBlock()->SetValue (ep_edge_chamfer_segments, ip->GetTime(), segments);
		}
		else {
			switch(point) {
			case 1:
				if(wasAltPressed) {
					wasAltPressed = false;
					// Maintain cumulative segment delta
					cumSegDelta += segDelta;
					// Reset so length doesn't 'pop'
					cumDelta += (mHold - m);
					om = omHold;
				}
				p0 = vpt.MapScreenToView(om, VIEWPORT_MAPPING_DEPTH);
				p1 = vpt.MapScreenToView(m + cumDelta, VIEWPORT_MAPPING_DEPTH);
				chamfer = Length(p1-p0);
				switch (mpEditPoly->GetMNSelLevel()) {
				case MNM_SL_VERTEX:
					mpEditPoly->getParamBlock()->SetValue (ep_vertex_chamfer, ip->GetTime(), chamfer);
					break;
				case MNM_SL_EDGE:
					mpEditPoly->getParamBlock()->SetValue (ep_edge_chamfer, ip->GetTime(), chamfer);
					break;
				}
				break;
			case 2:
				if(wasAltPressed) {
					wasAltPressed = false;
					// Maintain cumulative segment delta
					cumSegDelta += segDelta;
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
				tension = baseTension + cumTensDelta + tensDelta;
				if(tension < 0.0f)
					tension = 0.0f;
				else
				if(tension > 1.0f)
					tension = 1.0f;
				mpEditPoly->getParamBlock()->SetValue (ep_edge_chamfer_tension, ip->GetTime(), tension);
				break;
			}
		}
		mpEditPoly->EpfnDragChamfer (chamfer, tension, ip->GetTime());
		ip->RedrawViews(ip->GetTime(),REDRAW_INTERACTIVE);
		break;

	case MOUSE_ABORT:
		wasAltPressed = false;
		mpEditPoly->EpfnEndChamfer (false, ip->GetTime());			
		ip->RedrawViews (ip->GetTime(),REDRAW_END);
		break;
	}

	
	return TRUE;
}

HCURSOR ChamferSelectionProcessor::GetTransformCursor() { 
	static HCURSOR hECur = NULL;
	static HCURSOR hVCur = NULL;
	if (mpEditPoly->GetSelLevel() == EP_SL_VERTEX) {
		if ( !hVCur ) hVCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ChamferV);
		return hVCur;
	}
	if ( !hECur ) hECur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::ChamferE);
	return hECur;
}

MouseCallBack *ChamferCMode::MouseProc(int *numPoints) {
	// Find out what kind of chamfer we're performing
	int chamferType = EP_STANDARD_EDGE_CHAMFER;
	if(mpEditPoly->GetMNSelLevel() == MNM_SL_EDGE)
		mpEditPoly->getParamBlock()->GetValue (ep_edge_chamfer_type, ip->GetTime(), chamferType, FOREVER);
	*numPoints = (chamferType == EP_QUAD_EDGE_CHAMFER) ? 3 : 2;
	return &mouseProc;
}

void ChamferCMode::EnterMode() {
	mpEditPoly->SuspendContraints (true);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_vertex);
	if (!hGeom) return;
	ICustButton *but = GetICustButton(GetDlgItem(hGeom,IDC_CHAMFER));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void ChamferCMode::ExitMode() {
	mpEditPoly->SuspendContraints (false);
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_vertex);
	if (!hGeom) return;
	ICustButton *but = GetICustButton(GetDlgItem(hGeom,IDC_CHAMFER));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

// --- Slice: not really a command mode, just looks like it.--------- //

static bool sPopSliceCommandMode = false;

void EditPolyObject::EnterSliceMode () {
	sliceMode = TRUE;
	HWND hGeom = GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem(hGeom,IDC_SLICE));
		but->Enable ();
		ReleaseICustButton (but);
		but = GetICustButton (GetDlgItem (hGeom, IDC_RESET_PLANE));
		but->Enable ();
		ReleaseICustButton (but);
		but = GetICustButton (GetDlgItem (hGeom, IDC_SLICEPLANE));
		but->SetCheck (TRUE);
		ReleaseICustButton (but);
	}

	if (!sliceInitialized) EpResetSlicePlane ();

	EpPreviewBegin (epop_slice);

	// If we're already in a SO move or rotate mode, stay in it;
	// Otherwise, enter SO move.
	if ((ip->GetCommandMode() != moveMode) && (ip->GetCommandMode() != rotMode)) {
		ip->PushCommandMode (moveMode);
		sPopSliceCommandMode = true;
	} else sPopSliceCommandMode = false;

	NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	RefreshScreen ();
}

void EditPolyObject::ExitSliceMode () {
	sliceMode = FALSE;

	if (EpPreviewOn()) EpPreviewCancel ();
	HWND hGeom = GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem(hGeom,IDC_SLICE));
		but->Disable ();
		ReleaseICustButton (but);
		but = GetICustButton (GetDlgItem (hGeom, IDC_RESET_PLANE));
		but->Disable ();
		ReleaseICustButton (but);
		but = GetICustButton (GetDlgItem (hGeom, IDC_SLICEPLANE));
		but->SetCheck (FALSE);
		ReleaseICustButton (but);
	}

	if (sPopSliceCommandMode && (ip->GetCommandStackSize()>1)) ip->PopCommandMode ();

	NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	RefreshScreen ();
}

void EditPolyObject::GetSlicePlaneBoundingBox (Box3 & box, Matrix3 *tm) {
	Matrix3 rotMatrix;
	sliceRot.MakeMatrix (rotMatrix);
	rotMatrix.SetTrans (sliceCenter);
	if (tm) rotMatrix *= (*tm);
	box += Point3(-sliceSize,-sliceSize,0.0f)*rotMatrix;
	box += Point3(-sliceSize,sliceSize,0.0f)*rotMatrix;
	box += Point3(sliceSize,sliceSize,0.0f)*rotMatrix;
	box += Point3(sliceSize,-sliceSize,0.0f)*rotMatrix;
}

void EditPolyObject::DisplaySlicePlane (GraphicsWindow *gw) {
	// Draw rectangle representing slice plane.
	Point3 rp[5];
	Matrix3 rotMatrix;
	sliceRot.MakeMatrix (rotMatrix);
	rotMatrix.SetTrans (sliceCenter);
	rp[0] = Point3(-sliceSize,-sliceSize,0.0f)*rotMatrix;
	rp[1] = Point3(-sliceSize,sliceSize,0.0f)*rotMatrix;
	rp[2] = Point3(sliceSize,sliceSize,0.0f)*rotMatrix;
	rp[3] = Point3(sliceSize,-sliceSize,0.0f)*rotMatrix;
	gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
	gw->polyline (4, rp, NULL, NULL, TRUE, NULL);
}

// -- Cut proc/mode -------------------------------------

HitRecord *CutProc::HitTestVerts (IPoint2 &m, ViewExp& vpt, bool completeAnalysis) {

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	vpt.ClearSubObjHitList();

	mpEditPoly->SetHitLevelOverride (SUBHIT_MNVERTS);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEditPoly->ClearHitLevelOverride ();
	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();

	// Ok, if we haven't yet started a cut, and we don't want to know the hit location,
	// we're done:
	if ((startIndex<0) && !completeAnalysis) return hitLog.First();

	// Ok, now we need to find the closest eligible point:
	// First we grab our node's transform from the first hit:
	mObj2world = hitLog.First()->nodeRef->GetObjectTM(ip->GetTime());

	int bestHitIndex = -1;
	int bestHitDistance = 0;
	Point3 bestHitPoint(0.0f,0.0f,0.0f);
	HitRecord *ret = NULL;

	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Always want the closest hit pixelwise:
		if ((bestHitIndex>-1) && (bestHitDistance < hitRec->distance)) continue;

		if (startIndex >= 0)
		{
			// Check that our hit doesn't touch starting component:
			switch (startLevel) {
			case MNM_SL_VERTEX:
				if (startIndex == int(hitRec->hitInfo)) continue;
				break;
			case MNM_SL_EDGE:
				if (mpEPoly->GetMeshPtr()->e[startIndex].v1 == hitRec->hitInfo) continue;
				if (mpEPoly->GetMeshPtr()->e[startIndex].v2 == hitRec->hitInfo) continue;
				break;
			case MNM_SL_FACE:
				// Any face is suitable.
				break;
			}
		}

		Point3 & p = mpEPoly->GetMeshPtr()->P(hitRec->hitInfo);

		if (bestHitIndex>-1)
		{
			Point3 diff = p - bestHitPoint;
			diff = VectorTransform(mObj2world,diff);
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

HitRecord *CutProc::HitTestEdges (IPoint2 &m, ViewExp& vpt, bool completeAnalysis) {
	vpt.ClearSubObjHitList();

	mpEditPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEditPoly->ClearHitLevelOverride ();
	int numHits = vpt.NumSubObjHits();
	if (numHits == 0) return NULL;

	HitLog& hitLog = vpt.GetSubObjHitList();

	// Ok, if we haven't yet started a cut, and we don't want to know the actual hit location,
	// we're done:
	if ((startIndex<0) && !completeAnalysis) return hitLog.First ();

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

		if (startIndex>-1) 	{
			// Check that component doesn't touch starting component:
			switch (startLevel) {
			case MNM_SL_VERTEX:
				if (mpEPoly->GetMeshPtr()->e[hitRec->hitInfo].v1 == startIndex) continue;
				if (mpEPoly->GetMeshPtr()->e[hitRec->hitInfo].v2 == startIndex) continue;
				break;
			case MNM_SL_EDGE:
				if (startIndex == int(hitRec->hitInfo)) continue;
				break;
				// (any face is acceptable)
			}
		}

		// Get endpoints in world space:
		Point3 Aobj = mpEPoly->GetMeshPtr()->P(mpEPoly->GetMeshPtr()->e[hitRec->hitInfo].v1);
		Point3 Bobj = mpEPoly->GetMeshPtr()->P(mpEPoly->GetMeshPtr()->e[hitRec->hitInfo].v2);
		Point3 A = mObj2world * Aobj;
		Point3 B = mObj2world * Bobj;

		// Find proportion of our nominal hit point along this edge:
		Point3 Xdir = B-A;
		Xdir -= DotProd(Xdir, mLastHitDirection)*mLastHitDirection;	// make orthogonal to mLastHitDirection.

		// OLP Avoid division by zero
		float prop = 0.0f;
		float XdirLength = LengthSquared (Xdir);
		if (XdirLength == 0.0f)
		{
			prop = 1.0f;
		}
		else
		{
			prop = DotProd (Xdir, mLastHitPoint-A) / XdirLength;
		if (prop<.0001f) prop=0;
		if (prop>.9999f) prop=1;
		}

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

HitRecord *CutProc::HitTestFaces (IPoint2 &m, ViewExp& vpt, bool completeAnalysis) {

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();

	mpEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEditPoly->ClearHitLevelOverride ();
	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();

	// We don't bother to choose the view-direction-closest face,
	// since generally that's the only one we expect to get.
	HitRecord *ret = hitLog.ClosestHit();
	if (!completeAnalysis) return ret;

	mLastHitIndex = ret->hitInfo;
	mObj2world = ret->nodeRef->GetObjectTM (ip->GetTime ());
	
	// Get the average normal for the face, thus the plane, and intersect.
	MNMesh & mm = *(mpEPoly->GetMeshPtr());
	Point3 planeNormal = mm.GetFaceNormal (mLastHitIndex, TRUE);
	planeNormal = Normalize (mObj2world.VectorTransform (planeNormal));
	float planeOffset=0.0f;
	for (int i=0; i<mm.f[mLastHitIndex].deg; i++)
		planeOffset += DotProd (planeNormal, mObj2world*mm.v[mm.f[mLastHitIndex].vtx[i]].p);
	planeOffset = planeOffset/float(mm.f[mLastHitIndex].deg);

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

HitRecord *CutProc::HitTestAll (IPoint2 & m, ViewExp& vpt, int flags, bool completeAnalysis) {
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	HitRecord *hr = NULL;
	mLastHitLevel = MNM_SL_OBJECT;	// no hit.

	mpEditPoly->ForceIgnoreBackfacing (true);
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
	mpEditPoly->ForceIgnoreBackfacing (false);

	return hr;
}

void CutProc::DrawCutter (HWND hWnd, bool bErase, bool bPresent) {
	if (startIndex<0) return;

	IPoint2 pts[2];
	pts[0].x = mMouse1.x;
	pts[0].y = mMouse1.y;
	pts[1].x = mMouse2.x;
	pts[1].y = mMouse2.y;
	XORDottedPolyline(hWnd, 2, pts, 0, bErase, !bPresent);
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

	const DPoint3& GetHitPoint() const;
	const IPoint2& GetBetterMousePosition() const;
	// If the mouse ray did not hit any of the polygons, then we should create
	// another plane, compute the intersection of that plane and the ray, in 
	// order to satisfy MNMesh::Cut alrogithm.
	void HitTestFarPlane(
		OUT Point3& hitPoint, 
		IN EditPolyObject* pEditPoly, 
		IN const Matrix3& obj2World);

private:
	const DPoint3& GetZAxis() const;

	DPoint3 mHitPoint;
	IPoint2 mBetterMousePosition;

	DPoint3 mXAxis;
	DPoint3 mYAxis;
	DPoint3 mZAxis;
	DPoint3 mEyePosition;
	DMatrix3 mMatrix;
	DMatrix3 mInvertMatrix;
};

ViewSnapPlane::ViewSnapPlane(
	IN ViewExp* pView, 
	IN const IPoint2& mousePosition)
{
	DbgAssert(NULL != pView && pView->IsAlive());

	Ray ray;
	pView->MapScreenToWorldRay(mousePosition.x, mousePosition.y, ray);

	mEyePosition = ray.p;

	mZAxis = -Normalize(ray.dir);
	DPoint3 another(0.f, 0.f, 0.f);
    another[mZAxis.MinComponent()] = 1.f;
	mXAxis = Normalize(CrossProd(another, mZAxis));
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


	Matrix3 itm = mInvertMatrix.ToMatrix3();
	mHitPoint = pView->SnapPoint (
		mousePosition, 
		mBetterMousePosition, 
		&itm);
	mHitPoint = mInvertMatrix.PointTransform(mHitPoint);
}

inline const DPoint3& ViewSnapPlane::GetHitPoint() const
{
	return mHitPoint;
}

inline const IPoint2& ViewSnapPlane::GetBetterMousePosition() const
{
	return mBetterMousePosition;
}

inline const DPoint3& ViewSnapPlane::GetZAxis() const
{
	return mZAxis;
}

void ViewSnapPlane::HitTestFarPlane(
	OUT Point3& hitPoint, 
	IN EditPolyObject* pEditPoly, 
	IN const Matrix3& objToWorld)
{
	DbgAssert(NULL != pEditPoly);

	DMatrix3 obj2World;
	obj2World.FromMatrix3(objToWorld);
	Box3 bbox;
	pEditPoly->mm.BBox(bbox);

	// convert bbox to bounding sphere
	DPoint3 l_boundCenter = bbox.Center();
	double l_boundRadius = ((bbox.Width()/2)*obj2World).Length();

	// convert bounding sphere from object space to world space
	l_boundCenter = obj2World * l_boundCenter;

	// create a plane
	DPoint3 v1 = l_boundCenter - GetZAxis() * l_boundRadius;
	const DPoint3& planeNormal = GetZAxis();
	double d = DotProd(planeNormal, v1);

	// get the intersection of (mLastHitPoint, mLastHitPoint-zAxis) and the plane
	DPoint3 A = mHitPoint;
	DPoint3 B = mHitPoint - GetZAxis();
	double dA = DotProd(planeNormal, A) - d;
	double dB = DotProd(planeNormal, B) - d;
	double scale = dA / (dA - dB);
	DPoint3 C = B-A;
	C *= scale;
	A += C;

	hitPoint = Point3(A.x,A.y,A.z);

	// We still need to transform the hit point into object space.
	DMatrix3 l_world2obj = Inverse (obj2World);
	DPoint3 dhitPoint = l_world2obj * hitPoint;
	hitPoint.x = dhitPoint.x;
	hitPoint.y = dhitPoint.y;
	hitPoint.z = dhitPoint.z;
}

int CutProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	IPoint2 betterM(0,0);
	Matrix3 world2obj;
	Ray r;
	static HCURSOR hCutVertCur = NULL, hCutEdgeCur=NULL, hCutFaceCur = NULL;
	bool bDrawNewCutter = false;

	if (!hCutVertCur) {
		hCutVertCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CutVert);
		hCutEdgeCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CutEdge);
		hCutFaceCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CutFace);
	}

	switch (msg) {


	case MOUSE_PROPCLICK:
		if (startIndex==-1) 
		{
			ip->PopCommandMode ();
			// Turn off Cut preview:
			mpEditPoly->EpPreviewCancel ();
			if (mpEditPoly->EpPreviewGetSuspend ()) {
				mpEditPoly->EpPreviewSetSuspend (false);
				ip->PopPrompt ();
			}
			return FALSE;
		}
		else
		{
			mpEditPoly->EpPreviewCancel ();
			DrawCutter (hwnd, true ,false);
			startIndex = -1;
			startLevel = -1;

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
			DPoint3 dHitPoint = snapPlane.GetHitPoint();
			mLastHitPoint = Point3(dHitPoint.x,dHitPoint.y,dHitPoint.z);
			betterM = snapPlane.GetBetterMousePosition();
			vpt.MapScreenToWorldRay ((float)betterM.x, (float)betterM.y, r);
			mLastHitDirection = Normalize (r.dir);	// in world space

			// Hit test all subobject levels:
			if (NULL == HitTestAll (m, vpt, flags, true))
			{
				snapPlane.HitTestFarPlane(mLastHitPoint, mpEditPoly, mObj2world);
			}
		}

		if (mLastHitLevel == MNM_SL_OBJECT) return true;

		// Get hit directon in object space:
		world2obj = Inverse (mObj2world);
		mLastHitDirection = Normalize (VectorTransform (world2obj, mLastHitDirection));

		if (startIndex<0) {
			startIndex = mLastHitIndex;
			startLevel = mLastHitLevel;
			mpEPoly->getParamBlock()->SetValue (ep_cut_start_level, TimeValue(0), startLevel);
			mpEPoly->getParamBlock()->SetValue (ep_cut_start_index, TimeValue(0), startIndex);
			mpEPoly->getParamBlock()->SetValue (ep_cut_start_coords, TimeValue(0), mLastHitPoint);
			mpEPoly->getParamBlock()->SetValue (ep_cut_end_coords, TimeValue(0), mLastHitPoint);
			mpEPoly->getParamBlock()->SetValue (ep_cut_normal, TimeValue(0), -mLastHitDirection);
			if (ip->GetSnapState()) {
				// Must suspend "fully interactive" while snapping in Cut mode.
				mpEditPoly->EpPreviewSetSuspend (true);
				ip->PushPrompt (GetString (IDS_CUT_SNAP_PREVIEW_WARNING));
			}
			if (true) {
				MNMeshUtilities mmu(mpEditPoly->GetMeshPtr());
				mmu.CutPrepare();
			}
			mpEditPoly->EpPreviewBegin (epop_cut);
			mMouse1 = betterM;
			mMouse2 = betterM;
			DrawCutter (hwnd, false, true);
			break;
		}

		// Erase last cutter line:
		DrawCutter (hwnd, true, true);

		// Do the cut:
		mpEPoly->getParamBlock()->SetValue (ep_cut_end_coords, TimeValue(0), mLastHitPoint);
		mpEPoly->getParamBlock()->SetValue (ep_cut_normal, TimeValue(0), -mLastHitDirection);
		mpEditPoly->EpPreviewAccept ();

		mpEPoly->getParamBlock()->GetValue (ep_cut_start_index, TimeValue(0), mLastHitIndex, FOREVER);
		mpEPoly->getParamBlock()->GetValue (ep_cut_start_level, TimeValue(0), mLastHitLevel, FOREVER);
		if ((startLevel == mLastHitLevel) && (startIndex == mLastHitIndex)) {
			// no change - cut was unable to finish.
			startIndex = -1;
			if (mpEditPoly->EpPreviewGetSuspend ()) {
				mpEditPoly->EpPreviewSetSuspend (false);
				ip->PopPrompt ();
			}
			return false;	// end cut mode.
		}

		// Otherwise, start on next cut.
		startIndex = mLastHitIndex;
		startLevel = MNM_SL_VERTEX;
		mpEPoly->getParamBlock()->SetValue (ep_cut_start_coords, TimeValue(0),
			mpEPoly->GetMeshPtr()->P(startIndex));
		mpEditPoly->EpPreviewBegin (epop_cut);
		mMouse1 = betterM;
		mMouse2 = betterM;
		DrawCutter (hwnd, false, true);
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

		// Get hit directon in object space:
		world2obj = Inverse (mObj2world);


		// Show snap preview:
		vpt.SnapPreview (m, m, NULL, SNAP_FORCE_3D_RESULT);
		ip->RedrawViews (ip->GetTime(), REDRAW_INTERACTIVE);	// hey - why are we doing this?

		// Find 3D point in object space:
		{
			// Qilin.Ren Defect 932406
			// Fixed the problem while mouse ray is almost parallel to the relative xoy plane
			ViewSnapPlane snapPlane(vpt.ToPointer(), m);
			DPoint3 dHitPoint = snapPlane.GetHitPoint();
			mLastHitPoint = Point3(dHitPoint.x,dHitPoint.y,dHitPoint.z);
			betterM = snapPlane.GetBetterMousePosition();
			vpt.MapScreenToWorldRay ((float)betterM.x, (float)betterM.y, r);
			mLastHitDirection = Normalize (r.dir);	// in world space

			// Hit test all subobject levels:
			if (NULL == HitTestAll (m, vpt, flags, true))
			{
				snapPlane.HitTestFarPlane(mLastHitPoint, mpEditPoly, mObj2world);
			}
		}
		mLastHitDirection = Normalize (VectorTransform (world2obj, mLastHitDirection));
		mpEPoly->getParamBlock()->SetValue (ep_cut_normal, TimeValue(0), -mLastHitDirection);
		
		if (startIndex>-1) {
			// Erase last dotted line
			DrawCutter (hwnd, true ,false);

			// Set the cut preview to use the point we just hit on:
			mpEditPoly->EpPreviewSetDragging (true);
			mpEPoly->getParamBlock()->SetValue (ep_cut_end_coords, TimeValue(0), mLastHitPoint);
			mpEditPoly->EpPreviewSetDragging (false);

			GraphicsWindow *gw = vpt.getGW();
			gw->setTransform(Matrix3(1));
			// Draw new dotted line
			mMouse2 = betterM;
			Point3 lastMousePos = mLastHitPoint;
			lastMousePos = lastMousePos * mObj2world;
			Point3 outP(0,0,0);
			gw->transPoint(&lastMousePos,&outP);
			mMouse2.x = (int)outP.x;
			mMouse2.y = (int)outP.y;			

			bDrawNewCutter = true;

				
			
			Point3 startHitPoint;
			mpEPoly->getParamBlock()->GetValue (ep_cut_start_coords, TimeValue(0), startHitPoint,FOREVER);
			startHitPoint = startHitPoint * mObj2world;
			gw->transPoint(&startHitPoint,&outP);
			mMouse1.x = (int)outP.x;
			mMouse1.y = (int)outP.y;
		}

		// Set cursor based on best subobject match:
		switch (mLastHitLevel) {
		case MNM_SL_VERTEX: SetCursor (hCutVertCur); break;
		case MNM_SL_EDGE: SetCursor (hCutEdgeCur); break;
		case MNM_SL_FACE: SetCursor (hCutFaceCur); break;
		default: SetCursor (LoadCursor (NULL, IDC_ARROW));
		}

		// STEVE: why does this need preview protection when the same call in QuickSliceProc doesn't
		int fullyInteractive;
		mpEPoly->getParamBlock()->GetValue (ep_interactive_full, TimeValue(0), fullyInteractive, FOREVER);
		if (fullyInteractive)
			ip->RedrawViews (ip->GetTime(), REDRAW_INTERACTIVE);
		if (bDrawNewCutter)
			DrawCutter (hwnd, false, true);

		break;
		}
	}	 // switch

	return TRUE;
}

void CutCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CUT));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}

	proc.startIndex = -1;
}

void CutCMode::ExitMode() {
	// Lets make double-extra-sure that Cut's suspension of the preview system doesn't leak out...
	// (This line is actually necessary in the case where the user uses a shortcut key to jump-start
	// another command mode.)
	if (mpEditPoly->EpPreviewGetSuspend ()) {
		mpEditPoly->EpPreviewSetSuspend (false);
		if (mpEditPoly->ip) mpEditPoly->ip->PopPrompt ();
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_CUT));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

//------- QuickSlice proc/mode------------------------

void QuickSliceProc::DrawSlicer (HWND hWnd, bool bErase, bool bPresent) {
	if (!mSlicing) return;

	IPoint2 pts[2];
	pts[0].x = mMouse1.x;
	pts[0].y = mMouse1.y;
	pts[1].x = mMouse2.x;
	pts[1].y = mMouse2.y;
	XORDottedPolyline(hWnd, 2, pts, 0, bErase, !bPresent);
}

// Given two points and a view direction (in obj space), find slice plane:
void QuickSliceProc::ComputeSliceParams (bool center) {
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
	float size;
	if (!mpEditPoly->EpGetSliceInitialized()) mpEPoly->EpResetSlicePlane ();	// initializes size if needed.
	Point3 foo1, foo2;
	mpEPoly->EpGetSlicePlane (foo1, foo2, &size);	// Don't want to modify size.
	if (center) {
		Box3 bbox;
		mpEPoly->GetMeshPtr()->BBox(bbox, false);
		Point3 planeCtr = bbox.Center();
		planeCtr = planeCtr - DotProd (planeNormal, planeCtr) * planeNormal;
		planeCtr += DotProd (planeNormal, mLocal1) * planeNormal;
		mpEPoly->EpSetSlicePlane (planeNormal, planeCtr, size);
	} else {
		mpEPoly->EpSetSlicePlane (planeNormal, (mLocal1+mLocal2)*.5f, size);
	}
}

static HCURSOR hCurQuickSlice = NULL;

int QuickSliceProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	if (!hCurQuickSlice)
		hCurQuickSlice = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::QuickSlice);

	IPoint2 betterM(0,0);
	Point3 snapPoint(0.0f,0.0f,0.0f);
	Ray r;

	switch (msg) {
	case MOUSE_ABORT:
		if (mSlicing) DrawSlicer (hwnd, true, true);	// Erase last slice line.
		mSlicing = false;
		mpEditPoly->EpPreviewCancel ();
		if (mpEditPoly->EpPreviewGetSuspend()) {
			mpInterface->PopPrompt();
			mpEditPoly->EpPreviewSetSuspend (false);
		}
		return FALSE;

	case MOUSE_PROPCLICK:
		if (mSlicing) DrawSlicer (hwnd, true, true);	// Erase last slice line.
		mSlicing = false;
		mpEditPoly->EpPreviewCancel ();
		if (mpEditPoly->EpPreviewGetSuspend()) {
			mpInterface->PopPrompt();
			mpEditPoly->EpPreviewSetSuspend (false);
		}
		mpInterface->PopCommandMode ();
		return FALSE;

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
			// We need to get node of this polyobject, and obtain world->obj transform.
			ModContextList mcList;
			INodeTab nodes;
			mpInterface->GetModContexts(mcList,nodes);
			Matrix3 otm = nodes[0]->GetObjectTM(mpInterface->GetTime());
			nodes.DisposeTemporary();
			mWorld2obj = Inverse (otm);

			// first point:
			mLocal1 = mWorld2obj * snapPoint;
			mLocal2 = mLocal1;

			// We'll also need to find our Z direction:
			vpt.MapScreenToWorldRay ((float)betterM.x, (float)betterM.y, r);
			mZDir = Normalize (mWorld2obj.VectorTransform (r.dir));

			// Set the slice plane position:
			ComputeSliceParams (false);

			if (mpInterface->GetSnapState()) {
				// Must suspend "fully interactive" while snapping in Quickslice mode.
				mpEditPoly->EpPreviewSetSuspend (true);
				mpInterface->PushPrompt (GetString (IDS_QUICKSLICE_SNAP_PREVIEW_WARNING));
			}

			// Start the slice preview mode:
			mpEditPoly->EpPreviewBegin (epop_slice);

			mSlicing = true;
			mMouse1 = betterM;
			mMouse2 = betterM;			
			DrawSlicer (hwnd, false, true);
		} else {
			DrawSlicer (hwnd, true, true);	// Erase last slice line.

			// second point:
			mLocal2 = mWorld2obj * snapPoint;
			ComputeSliceParams (true);
			mSlicing = false;	// do before PreviewAccept to make sure display is correct.
			mpEditPoly->EpPreviewAccept ();
			if (mpEditPoly->EpPreviewGetSuspend()) {
				mpInterface->PopPrompt();
				mpEditPoly->EpPreviewSetSuspend (false);
			}
		}
		return true;
		} // case
	case MOUSE_MOVE:
		{
		if (!mSlicing) break;	// Nothing to do if we haven't clicked first point yet

		SetCursor (hCurQuickSlice);
		DrawSlicer (hwnd, true, false);	// Erase last slice line.

		// Find where we hit, in world space, on the construction plane or snap location:
		ViewExp& vpt = mpInterface->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, mMouse2, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);

		// Find object-space equivalent for mLocal2:
		mLocal2 = mWorld2obj * snapPoint;

		mpEditPoly->EpPreviewSetDragging (true);
		ComputeSliceParams ();
		mpEditPoly->EpPreviewSetDragging (false);

		mpInterface->RedrawViews (mpInterface->GetTime());

		DrawSlicer (hwnd, false, true);	// Draw this slice line.
		break;
		} // case
	case MOUSE_FREEMOVE:
		{
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
		} // case
	}

	return TRUE;
}

void QuickSliceCMode::EnterMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_QUICKSLICE));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
	proc.mSlicing = false;
}

void QuickSliceCMode::ExitMode() {
	// Lets make double-extra-sure that QuickSlice's suspension of the preview system doesn't leak out...
	// (This line is actually necessary in the case where the user uses a shortcut key to jump-start
	// another command mode.)
	if (mpEditPoly->EpPreviewGetSuspend()) {
		if (mpEditPoly->ip) mpEditPoly->ip->PopPrompt();
		mpEditPoly->EpPreviewSetSuspend (false);
	}

	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_QUICKSLICE));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

//------- LiftFromEdge proc/mode------------------------

HitRecord *LiftFromEdgeProc::HitTestEdges (IPoint2 &m, ViewExp& vpt) {

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();
	EditPolyObject *pEditPoly = (EditPolyObject *) mpEPoly;	// STEVE: make go away someday.
	pEditPoly->SetHitLevelOverride (SUBHIT_MNEDGES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	pEditPoly->ClearHitLevelOverride ();
	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();
	return hitLog.ClosestHit();	// (may be NULL.)
}

// Mouse interaction:
// - user clicks on hinge edge
// - user drags angle.

int LiftFromEdgeProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m ) {
	HitRecord* hr = NULL;
	Point3 snapPoint;
	IPoint2 diff;
	float angle;
	EditPolyObject* pEditPoly = NULL;

	switch (msg) {
	case MOUSE_ABORT:
		pEditPoly = (EditPolyObject *) mpEPoly;
		pEditPoly->EpPreviewCancel ();
		return FALSE;

	case MOUSE_PROPCLICK:
		pEditPoly = (EditPolyObject *) mpEPoly;
		pEditPoly->EpPreviewCancel ();
		ip->PopCommandMode ();
		return FALSE;

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
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		if (!edgeFound) {
			hr = HitTestEdges (m, vpt);
			if (!hr) return true;
			pEditPoly = (EditPolyObject *) mpEPoly;
			edgeFound = true;
			mpEPoly->getParamBlock()->SetValue (ep_lift_edge, ip->GetTime(), int(hr->hitInfo));
			mpEPoly->getParamBlock()->SetValue (ep_lift_angle, ip->GetTime(), 0.0f);	// prevent "flash"
			pEditPoly->EpPreviewBegin (epop_lift_from_edge);
			firstClick = m;
		} else {
			IPoint2 diff = m - firstClick;
			// (this arbirtrarily scales each pixel to one degree.)
			float angle = diff.y * PI / 180.0f;
			mpEPoly->getParamBlock()->SetValue (ep_lift_angle, ip->GetTime(), angle);
			pEditPoly = (EditPolyObject *) mpEPoly;
			pEditPoly->EpPreviewAccept ();
			ip->RedrawViews (ip->GetTime());
			edgeFound = false;
			return false;
		}
		return true;
		} // case
	case MOUSE_MOVE:
		{
		// Find where we hit, in world space, on the construction plane or snap location:
		ViewExp& vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);

		if (!edgeFound) {
			// Just set cursor depending on presence of edge:
			if (HitTestEdges(m,vpt)) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
			else SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		}
		diff = m - firstClick;
		// (this arbirtrarily scales each pixel to one degree.)
		angle = diff.y * PI / 180.0f;

		pEditPoly = (EditPolyObject *) mpEPoly;	// STEVE: Temporarily necessary.
		pEditPoly->EpPreviewSetDragging (true);
		mpEPoly->getParamBlock()->SetValue (ep_lift_angle, ip->GetTime(), angle);
		pEditPoly->EpPreviewSetDragging (false);

		// Even if we're not fully interactive, we need to update the dotted line display:
		pEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
		ip->RedrawViews (ip->GetTime());
		break;
		} // case
	case MOUSE_FREEMOVE:
		{
		ViewExp& vpt = ip->GetViewExp (hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);

		if (!edgeFound) {
			// Just set cursor depending on presence of edge:
			if (HitTestEdges(m,vpt)) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		}
		break;
		} // case
	}

	return TRUE;
}

void LiftFromEdgeCMode::EnterMode() {
	mpEditPoly->RegisterCMChangedForGrip();
	proc.Reset();
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_LIFT_FROM_EDG));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void LiftFromEdgeCMode::ExitMode() {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_LIFT_FROM_EDG));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

//-------------------------------------------------------

bool PickLiftEdgeProc::EdgePick (int edge, float prop) {
	mpEPoly->getParamBlock()->SetValue (ep_lift_edge, ip->GetTime(), edge);
	ip->RedrawViews (ip->GetTime());
	return false;	// false = exit mode when done; true = stay in mode.
}

void PickLiftEdgeCMode::EnterMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_LIFT_PICK_EDGE));
	but->SetCheck(true);
	ReleaseICustButton(but);
}

void PickLiftEdgeCMode::ExitMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_LIFT_PICK_EDGE));
	but->SetCheck(false);
	ReleaseICustButton(but);
}

//-------------------------------------------------------

void WeldCMode::EnterMode () {
	mproc.Reset();
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_vertex);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom, IDC_TARGET_WELD));
	but->SetCheck(TRUE);
	ReleaseICustButton(but);
}

void WeldCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_vertex);
	if (!hGeom) return;
	ICustButton *but = GetICustButton (GetDlgItem (hGeom, IDC_TARGET_WELD));
	but->SetCheck(FALSE);
	ReleaseICustButton(but);
}

void WeldMouseProc::DrawWeldLine (HWND hWnd,IPoint2 &m, bool bErase, bool bPresent) {
	if (targ1<0) return;
	
	IPoint2 pts[2];
	pts[0].x = m1.x;
	pts[0].y = m1.y;
	pts[1].x = m.x;
	pts[1].y = m.y;
	XORDottedPolyline(hWnd, 2, pts, 0, bErase, !bPresent);
}

bool WeldMouseProc::CanWeldVertices (int v1, int v2)
{
	MNMesh & mm = mpEditPoly->GetMesh();
	// If vertices v1 and v2 share an edge, then take a collapse type approach;
	// Otherwise, weld them if they're suitable (open verts, etc.)

	// Check for bogus values for v1 and v2
	DbgAssert(mm.vedg != NULL);
	DbgAssert(v1 < mm.numv);
	DbgAssert(v2 < mm.numv);
	if ((v1 >= mm.numv) || (v2 >= mm.numv))
	{
		return false;
	}

	int v1_EdgeCount = mm.vedg[v1].Count();
	int v2_EdgeCount = mm.vedg[v2].Count();
	
	int i = 0;
	for (i=0; i < v1_EdgeCount; i++)
	{
		int edgeIndex = mm.vedg[v1][i];
		MNEdge& edge = mm.e[edgeIndex];
		if (edge.OtherVert(v1) == v2)
			break;
	}

	if (i < v1_EdgeCount)
	{
		int ee = mm.vedg[v1][i];
		// If the faces are the same, then don't do anything
		if (mm.e[ee].f1 == mm.e[ee].f2) 
			return false;
		// there are other conditions, but they're complex....
	}
	else
	{
		if (v1_EdgeCount && (v1_EdgeCount <= mm.vfac[v1].Count())) 
			return false;

		for (i=0; i < v1_EdgeCount; i++)
		{	
			for (int j=0; j < v2_EdgeCount; j++)
			{
				int e1 = mm.vedg[v1][i];
				int e2 = mm.vedg[v2][j];
				int ov = mm.e[e1].OtherVert (v1);
				if (ov != mm.e[e2].OtherVert (v2)) 
					continue;
				// Edges from these vertices connect to the same other vert.
				// That means we have additional conditions:
				if (((mm.e[e1].v1 == ov) && (mm.e[e2].v1 == ov)) ||
					((mm.e[e1].v2 == ov) && (mm.e[e2].v2 == ov))) 
					return false;	// edges trace same direction, so cannot be merged.
				if (mm.e[e1].f2 > -1) 
					return false;
				if (mm.e[e2].f2 > -1)
					return false;
				if (mm.vedg[ov].Count() <= mm.vfac[ov].Count()) 
					return false;
			}
		}
	}
	return true;
}

bool WeldMouseProc::CanWeldEdges (int e1, int e2) {
	MNMesh & mm = mpEditPoly->GetMesh ();
	if (mm.e[e1].f2 > -1) return false;
	if (mm.e[e2].f2 > -1) return false;
	if (mm.e[e1].f1 == mm.e[e2].f1) return false;
	if (mm.e[e1].v1 == mm.e[e2].v1) return false;
	if (mm.e[e1].v2 == mm.e[e2].v2) return false;
	return true;
}

HitRecord *WeldMouseProc::HitTest (IPoint2 &m, ViewExp *vpt) {

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
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt);
	mpEditPoly->ClearHitLevelOverride ();
	if (!vpt->NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt->GetSubObjHitList();
	HitRecord *hr = hitLog.ClosestHit();
	if (targ1>-1) {
		if (targ1 == hr->hitInfo) return NULL;
		if (mpEditPoly->GetMNSelLevel() == MNM_SL_EDGE) {
			if (!CanWeldEdges (targ1, hr->hitInfo)) return NULL;
		} else {
			if (!CanWeldVertices (targ1, hr->hitInfo)) return NULL;
		}
	}
	return hr;
}

int WeldMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	int res = TRUE, stringID = 0;
	HitRecord* hr = NULL;
	bool ret(false);

	switch (msg) {
	case MOUSE_ABORT:
		// Erase last weld line:
		if (targ1>-1) DrawWeldLine (hwnd, oldm2, true, true);
		targ1 = -1;
		return FALSE;

	case MOUSE_PROPCLICK:
		// Erase last weld line:
		if (targ1>-1) DrawWeldLine (hwnd, oldm2, true, true);
		ip->PopCommandMode ();
		return FALSE;

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
		hr = HitTest (m, vpt.ToPointer());
		if (!hr) break;
		if (targ1 < 0) {
			targ1 = hr->hitInfo;
			m1 = m;
			DrawWeldLine (hwnd, m, false, true);
			oldm2 = m;
			break;
		}

		// Otherwise, we're completing the weld.
		// Erase the last weld-line:
		DrawWeldLine (hwnd, oldm2, true, true);

		// Do the weld:
		theHold.Begin();
		ret      = false;
		stringID = 0;
		if (mpEditPoly->GetSelLevel() == EP_SL_VERTEX) {
			ret = mpEditPoly->EpfnWeldVertToVert (targ1, hr->hitInfo)?true:false;
			stringID = IDS_WELD_VERTS;
		} else if (mpEditPoly->GetMNSelLevel() == MNM_SL_EDGE) {
			ret = mpEditPoly->EpfnWeldEdges (targ1, hr->hitInfo)?true:false;
			stringID = IDS_WELD_EDGES;
		}
		if (ret) {
			theHold.Accept (GetString(stringID));
			mpEditPoly->RefreshScreen ();
		} else {
			theHold.Cancel ();
		}
		targ1 = -1;
		return false;
		} // case
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
		if (targ1 > -1) {
			DrawWeldLine (hwnd, oldm2, true, false);
			oldm2 = m;		
		}
		if (HitTest (m,vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor (LoadCursor (NULL, IDC_ARROW));
		if (targ1 > -1) DrawWeldLine (hwnd, m, false, true);
		break;
		} // case
	}
	
	
	return true;	
}

//-------------------------------------------------------

void EditTriCMode::EnterMode() {
	HWND hSurf = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_FS_EDIT_TRI));
		but->SetCheck(TRUE);
		ReleaseICustButton(but);
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_DIAGONALS);
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void EditTriCMode::ExitMode() {
	HWND hSurf = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_FS_EDIT_TRI));
		but->SetCheck (FALSE);
		ReleaseICustButton(but);
	}
	mpEditPoly->ClearDisplayLevelOverride();
	mpEditPoly->NotifyDependents (FOREVER, PART_DISPLAY, REFMSG_CHANGE);
	mpEditPoly->ip->RedrawViews(mpEditPoly->ip->GetTime());
}

void EditTriProc::VertConnect () {
	DbgAssert (v1>=0);
	DbgAssert (v2>=0);
	Tab<int> v1fac = mpEPoly->GetMeshPtr()->vfac[v1];
	int i, j, ff = 0, v1pos = 0, v2pos=-1;
	for (i=0; i<v1fac.Count(); i++) {
		MNFace & mf = mpEPoly->GetMeshPtr()->f[v1fac[i]];
		for (j=0; j<mf.deg; j++) {
			if (mf.vtx[j] == v2) v2pos = j;
			if (mf.vtx[j] == v1) v1pos = j;
		}
		if (v2pos<0) continue;
		ff = v1fac[i];
		break;
	}

	if (v2pos<0) return;

	theHold.Begin();
	mpEPoly->EpfnSetDiagonal (ff, v1pos, v2pos);
	theHold.Accept (GetString (IDS_EDIT_TRIANGULATION));
	mpEPoly->RefreshScreen ();
}

//-------------------------------------------------------

void TurnEdgeCMode::EnterMode() {
	HWND hSurf = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_TURN_EDGE));
		if (but) {
			but->SetCheck(true);
			ReleaseICustButton(but);
		}
	}
	mpEditPoly->SetDisplayLevelOverride (MNDISP_DIAGONALS);
	mpEditPoly->LocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->RefreshScreen ();
}

void TurnEdgeCMode::ExitMode() {
	HWND hSurf = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hSurf) {
		ICustButton *but = GetICustButton (GetDlgItem (hSurf, IDC_TURN_EDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
	mpEditPoly->ClearDisplayLevelOverride();
	mpEditPoly->LocalDataChanged(DISP_ATTRIB_CHANNEL);
	mpEditPoly->RefreshScreen ();
}

HitRecord *TurnEdgeProc::HitTestEdges (IPoint2 & m, ViewExp& vpt) {

	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();

	mpEPoly->ForceIgnoreBackfacing (true);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEPoly->ForceIgnoreBackfacing (false);

	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();

	// Find the closest hit that is not a border edge:
	HitRecord *hr = NULL;
	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Always want the closest hit pixelwise:
		if (hr && (hr->distance < hitRec->distance)) continue;
		MNMesh & mesh = mpEPoly->GetMesh();
		if (mesh.e[hitRec->hitInfo].f2<0) continue;
		hr = hitRec;
	}

	return hr;	// (may be NULL.)
}

HitRecord *TurnEdgeProc::HitTestDiagonals (IPoint2 & m, ViewExp& vpt) {
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt.ClearSubObjHitList();

	mpEPoly->SetHitLevelOverride (SUBHIT_MNDIAGONALS);
	mpEPoly->ForceIgnoreBackfacing (true);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEPoly->ForceIgnoreBackfacing (false);
	mpEPoly->ClearHitLevelOverride ();

	if (!vpt.NumSubObjHits()) return NULL;
	HitLog& hitLog = vpt.GetSubObjHitList();
	return hitLog.ClosestHit ();
}

int TurnEdgeProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint;
	MNDiagonalHitData *hitDiag=NULL;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		if (point==1) break;

		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if ( ! vpt.IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		snapPoint = vpt.SnapPoint (m, m, NULL);
		snapPoint = vpt.MapCPToWorld (snapPoint);
		hr = HitTestDiagonals (m, vpt);
		if (hr) hitDiag = (MNDiagonalHitData *) hr->hitData;
		
		if (!hr) break;

		theHold.Begin ();
		if (hitDiag) mpEPoly->EpfnTurnDiagonal (hitDiag->mFace, hitDiag->mDiag);
		//else mpEPoly->EpModTurnEdge (hr->hitInfo, hr->nodeRef);
		theHold.Accept (GetString (IDS_TURN_EDGE));
		mpEPoly->RefreshScreen();
		break;
		} // case
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
		if (/*HitTestEdges(m,vpt) ||*/ HitTestDiagonals(m,vpt)) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		break;
		} // case
	}

	return true;
}

//-------------------------------------------------------

HitRecord *BridgeMouseProc::HitTest (IPoint2 & m, ViewExp *vpt) {

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
         int j;
			for (j=0; j<mDisallowedSecondHits.Count(); j++)
				if (hr->hitInfo == mDisallowedSecondHits[j]) break;
			if (j<mDisallowedSecondHits.Count()) continue;
		}
		if ((bestHit == NULL) || (bestHit->distance>hr->distance)) bestHit = hr;
	}
	return bestHit;
}

void BridgeMouseProc::DrawDiag (HWND hWnd, const IPoint2 & m, bool bErase, bool bPresent) {
	if (hit1<0) return;

	IPoint2 pts[2];
	pts[0].x = m1.x;
	pts[0].y = m1.y;
	pts[1].x = m.x;
	pts[1].y = m.y;
	XORDottedPolyline(hWnd, 2, pts, 0, bErase, !bPresent);
}

int BridgeMouseProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord *hr = NULL;
	Point3 snapPoint;

	switch (msg) {
	case MOUSE_INIT:
		hit1 = hit2 = -1;
		mDisallowedSecondHits.ZeroCount();
		break;

	case MOUSE_PROPCLICK:
		DrawDiag (hwnd, lastm, true, true);	// erase last dotted line.
		hit1 = hit2 = -1;
		mDisallowedSecondHits.ZeroCount();
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		if (point==1) break;

		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
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
			hit1 = hr->hitInfo;
			m1 = m;
			lastm = m;
			SetupDisallowedSecondHits ();
			DrawDiag (hwnd, m, false, true);
			break;
		}
		// Otherwise, we've found a connection.
		DrawDiag (hwnd, lastm, true, true);	// erase last dotted line.
		hit2 = hr->hitInfo;
		Bridge ();
		hit1 = -1;
		mDisallowedSecondHits.ZeroCount();
		return FALSE;	// Done with this connection.
		} // case
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
		hr=HitTest(m,vpt.ToPointer());
		if (hr!=NULL) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		DrawDiag (hwnd, lastm, true, false);
		DrawDiag (hwnd, m, false, true);
		lastm = m;
		break;
		} // case
	}

	return TRUE;
}

void BridgeBorderProc::Bridge () {
	theHold.Begin ();
	mpEPoly->getParamBlock()->SetValue (ep_bridge_selected, 0, false);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_target_1, 0, hit1+1);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_target_2, 0, hit2+1);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_1, 0, 0);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_2, 0, 0);

	if (!mpEPoly->EpfnBridge (EP_SL_BORDER, MN_SEL)) {
		theHold.Cancel ();
		return;
	}
	theHold.Accept (GetString (IDS_BRIDGE_BORDERS));
	mpEPoly->RefreshScreen ();
}

void BridgeBorderProc::SetupDisallowedSecondHits () {
	// Second hit can't be in same border loop as first hit.
	MNMeshUtilities mmu(mpEPoly->GetMeshPtr());
	mmu.GetBorderFromEdge (hit1, mDisallowedSecondHits);
}

void BridgeBorderCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_border);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
}

void BridgeBorderCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_border);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
}

// ------------------------------------------------------

void BridgePolygonProc::Bridge () {
	theHold.Begin ();
	mpEPoly->getParamBlock()->SetValue (ep_bridge_selected, 0, false);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_target_1, 0, hit1+1);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_target_2, 0, hit2+1);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_1, 0, 0);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_2, 0, 0);

	int twist2(0);
	if ((hit1>=0) && (hit2>=0)) {
		MNMeshUtilities mmu(mpEPoly->GetMeshPtr());
		twist2 = mmu.FindDefaultBridgeTwist (hit1, hit2);
	}
	mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_2, 0, twist2);

	if (!mpEPoly->EpfnBridge (EP_SL_FACE, MN_SEL)) {
		theHold.Cancel ();
		return;
	}

	theHold.Accept (GetString (IDS_BRIDGE_POLYGONS));
	mpEPoly->RefreshScreen ();
}

void BridgePolygonProc::SetupDisallowedSecondHits () {
	// Second hit can't be the same as, or a neighbor of, the first hit:
	MNMesh & mesh = mpEPoly->GetMesh();
	mDisallowedSecondHits.Append (1, &hit1, mesh.f[hit1].deg*3);
	for (int i=0; i<mesh.f[hit1].deg; i++) {
		Tab<int> & vf = mesh.vfac[mesh.f[hit1].vtx[i]];
		for (int j=0; j<vf.Count(); j++) {
			if (vf[j] == hit1) continue;
			mDisallowedSecondHits.Append (1, vf.Addr(j));	// counts many faces twice; that's fine, still a short list.
		}
	}
}

void BridgePolygonCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
}

void BridgePolygonCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_face);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
}
// ------------------------------------------------------

void BridgeEdgeProc::Bridge () {
	float l_Angle = PI/4.0f;
	theHold.Begin ();
	mpEPoly->getParamBlock()->SetValue (ep_bridge_selected, 0, false);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_target_1, 0, hit1+1);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_target_2, 0, hit2+1);
	mpEPoly->getParamBlock()->SetValue (ep_bridge_adjacent, 0, l_Angle);

	if (!mpEPoly->EpfnBridge (EP_SL_EDGE, MN_SEL)) {
		theHold.Cancel ();
		return;
	}

	theHold.Accept (GetString (IDS_BRIDGE_POLYGONS));
	mpEPoly->RefreshScreen ();
}

void BridgeEdgeProc::SetupDisallowedSecondHits () {
	// Second hit can't be the same as, or a neighbor of, the first hit:
	MNMesh & mesh = mpEPoly->GetMesh();
	mDisallowedSecondHits.Append (1, &hit1, 20);

	int l_StartVertx = mesh.e[hit1].v1;

	Tab<int> & startEdge = mesh.vedg[mesh.e[hit1][0]];

	for (int j=0; j<startEdge.Count(); j++) 
	{
		if (startEdge[j] == hit1) continue;
		mDisallowedSecondHits.Append (1, startEdge.Addr(j));	// counts many edges twice; that's fine, still a short list.
	}

	Tab<int> & endEdge = mesh.vedg[mesh.e[hit1][1]];
	for (int j=0; j<endEdge.Count(); j++) 
	{
		if (endEdge[j] == hit1) continue;
		mDisallowedSecondHits.Append (1, endEdge.Addr(j));	// counts many edges twice; that's fine, still a short list.
	}


}

void BridgeEdgeCMode::EnterMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_edge);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck(TRUE);
			ReleaseICustButton(but);
		}
	}
}

void BridgeEdgeCMode::ExitMode () {
	HWND hGeom = mpEditPoly->GetDlgHandle (ep_geom_edge);
	if (hGeom) {
		ICustButton *but = GetICustButton (GetDlgItem (hGeom,IDC_BRIDGE));
		if (but) {
			but->SetCheck (false);
			ReleaseICustButton(but);
		}
	}
}

// ------------------------------------------------------

PickBridgeTargetProc::PickBridgeTargetProc (EditPolyObject* e, IObjParam *i, int end) : mpEPoly(e), ip(i), mWhichEnd(end) 
{
	int otherEnd = e->getParamBlock()->GetInt (mWhichEnd ? ep_bridge_target_1 : ep_bridge_target_2);
	
	if (otherEnd <= 0 )
		return;

	otherEnd--;

	if (e->GetEPolySelLevel() == EP_SL_BORDER) 
	{
		// data validation BUG-CER FIX 904837 laurent 04/07
		if ( otherEnd >= mpEPoly->GetMesh().nume)
			return; 

		MNMeshUtilities mmu (e->GetMeshPtr());
		mmu.GetBorderFromEdge (otherEnd, mDisallowedHits);
	} 
	else if (e->GetEPolySelLevel() == EP_SL_FACE)
	{
		MNMesh &mesh = mpEPoly->GetMesh();

		// data validation  BUG FIX 904837
		if ( otherEnd >= mesh.numf)
			return; 

		// Second hit can't be the same as, or a neighbor of, the first hit:
		mDisallowedHits.Append (1, &otherEnd, 20);
		
		for (int i = 0; i< mesh.f[otherEnd].deg; i++) 
		{
			Tab<int> & vf = mesh.vfac[mesh.f[otherEnd].vtx[i]];

			for (int j=0; j<vf.Count(); j++) 
			{
				if (vf[j] == otherEnd) continue;
				mDisallowedHits.Append (1, vf.Addr(j));	// counts many faces twice; that's fine, still a short list.
			}
		}
	}
	else if (e->GetEPolySelLevel() == EP_SL_EDGE)
	{
		// Second hit can't be the same as, or a neighbor of, the first hit:
		MNMesh & mesh = mpEPoly->GetMesh();

		// data validation  BUG FIX 904837
		if ( otherEnd >= mesh.nume)
			return; 

		mDisallowedHits.Append (1, &otherEnd, 20);

		int l_StartVertx = mesh.e[otherEnd].v1;

		Tab<int> & startEdge = mesh.vedg[mesh.e[otherEnd][0]];

		for (int j=0; j<startEdge.Count(); j++) 
		{
			if (startEdge[j] == otherEnd) continue;
			mDisallowedHits.Append (1, startEdge.Addr(j));	// counts many faces twice; that's fine, still a short list.
		}

		Tab<int> & endEdge = mesh.vedg[mesh.e[otherEnd][1]];
		for (int j=0; j<endEdge.Count(); j++) 
		{
			if (endEdge[j] == otherEnd) continue;
			mDisallowedHits.Append (1, endEdge.Addr(j));	// counts many faces twice; that's fine, still a short list.
		}

	}
}

HitRecord *PickBridgeTargetProc::HitTest (IPoint2 &m, ViewExp *vpt) 
{

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

	HitRecord *ret = NULL;
	for (HitRecord *hitRec=hitLog.First(); hitRec != NULL; hitRec=hitRec->Next()) {
		// Filter out edges in the other border,
		// or polygons neighboring the other polygon.
      int j;
		for (j=0; j<mDisallowedHits.Count(); j++) {
			if (mDisallowedHits[j] == hitRec->hitInfo) break;
		}
		if (j<mDisallowedHits.Count()) continue;

		// Always want the closest hit pixelwise:
		if ((ret==NULL) || (ret->distance > hitRec->distance)) ret = hitRec;
	}
	return ret;
}

int PickBridgeTargetProc::proc (HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	HitRecord* hr = NULL;
	Point3 snapPoint;

	switch (msg) {
	case MOUSE_PROPCLICK:
		ip->PopCommandMode ();
		break;

	case MOUSE_POINT:
		{
		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
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

		theHold.Begin ();
		{
			mpEPoly->getParamBlock()->SetValue (mWhichEnd ? ep_bridge_target_2 : ep_bridge_target_1, ip->GetTime(),
				(int)hr->hitInfo+1);

			int targ1 = mpEPoly->getParamBlock()->GetInt (ep_bridge_target_1)-1;
			int targ2 = mpEPoly->getParamBlock()->GetInt (ep_bridge_target_2)-1;
			int twist2(0);
			if ((targ1>=0) && (targ2>=0)) {
				MNMeshUtilities mmu(mpEPoly->GetMeshPtr());
				twist2 = mmu.FindDefaultBridgeTwist (targ1, targ2);
			}
			mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_1, ip->GetTime(), 0);
			mpEPoly->getParamBlock()->SetValue (ep_bridge_twist_2, 0, twist2);
		}

		theHold.Accept (GetString ((mpEPoly->GetEPolySelLevel()==EP_SL_BORDER ||mpEPoly->GetEPolySelLevel()==EP_SL_EDGE) ?
			(mWhichEnd ? IDS_BRIDGE_PICK_EDGE_2 : IDS_BRIDGE_PICK_EDGE_1) :
			(mWhichEnd ? IDS_BRIDGE_PICK_POLYGON_2 : IDS_BRIDGE_PICK_POLYGON_1)));
		mpEPoly->RefreshScreen();

		ip->PopCommandMode ();
		return false;
		} // case
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
		if (HitTest(m,vpt.ToPointer())) SetCursor(ip->GetSysCursor(SYSCUR_SELECT));
		else SetCursor(LoadCursor(NULL,IDC_ARROW));
		break;
		} // case
	}

	return true;
}

void PickBridge1CMode::EnterMode () {
	mpEditPoly->RegisterCMChangedForGrip();
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG1));
	if (but) {
		but->SetCheck(true);
		ReleaseICustButton(but);
	}

}

void PickBridge1CMode::ExitMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG1));
	if (but) {
		but->SetCheck(false);
		ReleaseICustButton(but);
	}
}

void PickBridge2CMode::EnterMode () {
	mpEditPoly->RegisterCMChangedForGrip();
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG2));
	if (but) {
		but->SetCheck(true);
		ReleaseICustButton(but);
	}

}

void PickBridge2CMode::ExitMode () {
	HWND hSettings = mpEditPoly->GetDlgHandle (ep_settings);
	if (!hSettings) return;
	ICustButton *but = GetICustButton (GetDlgItem (hSettings,IDC_BRIDGE_PICK_TARG2));
	if (but) {
		but->SetCheck(false);
		ReleaseICustButton(but);
	}
}


//////////////////////////////////////////

void EditPolyObject::DoAccept(TimeValue t)
{
}

//for EditSSCB 
void EditPolyObject::SetPinch(TimeValue t, float pinch)
{
	pblock->SetValue (ep_ss_pinch, t, pinch);
}
void EditPolyObject::SetBubble(TimeValue t, float bubble)
{
	pblock->SetValue (ep_ss_bubble, t, bubble);
}
float EditPolyObject::GetPinch(TimeValue t)
{
	return pblock->GetFloat (ep_ss_pinch, t);

}
float EditPolyObject::GetBubble(TimeValue t)
{
	return pblock->GetFloat (ep_ss_bubble, t);
}
float EditPolyObject::GetFalloff(TimeValue t)
{
	return pblock->GetFloat (ep_ss_falloff, t);
}
void EditPolyObject::SetFalloff(TimeValue t, float falloff)
{
	pblock->SetValue (ep_ss_falloff, t, falloff);
}


int SubobjectPickProc::HitTest(IPoint2 &m, ViewExp& vpt,int& subIndex)
{
	int res = 0;
	subIndex = -1;
	float hitDist = BIGFLOAT;

	//hit test face first
	vpt.ClearSubObjHitList();
	mpEditPoly->SetHitLevelOverride (SUBHIT_MNFACES);
	ip->SubObHitTest (ip->GetTime(), HITTYPE_POINT, 0, 0, &m, vpt.ToPointer());
	mpEditPoly->ClearHitLevelOverride ();
	if (vpt.NumSubObjHits())
	{
		HitLog& hitLog = vpt.GetSubObjHitList();
		HitRecord* hitRecord = hitLog.ClosestHit();  
		res = 3;
		subIndex = hitRecord->hitInfo;
		hitDist = hitRecord->distance;
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
				if ((res == 0) ||  (hitRecord->distance <= hitDist) )
				{
					res = 1;
					subIndex = hitRecord->hitInfo;
					hitDist = hitRecord->distance;
				}
			}
			startM.x += fudge;
		}
		startM.y += fudge;
		startM.x = m.x-fudge;
	}
	//if we did not hit a vertex see if we hit an edge
	if (res != 1)
	{
		startM = m;
		startM.x -= fudge;
		startM.y -= fudge;
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

					if ((res == 0) || (hitRecord->distance <= hitDist) )
					{
						res = 2;
						subIndex = hitRecord->hitInfo;
						hitDist = hitRecord->distance;
					}	
				}
				startM.x += fudge;
			}
			startM.y += fudge;
			startM.x = m.x-fudge;
		}
	}
		
	return res;

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
			SubobjectPickMode* mode = (SubobjectPickMode*) mpEditPoly->getCommandMode(epmode_subobjectpick);
			if ( !mode->mInOverride  )
				ip->PopCommandMode ();
			return false;
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

		
		hitSubLevel = HitTest(m, vpt,hitSubIndex);
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

void SubobjectPickProc::SetSubobjectLevel()
{
	if (hitSubLevel == 1)
		GetCOREInterface()->SetSubObjectLevel(EP_SL_VERTEX);
	else if (hitSubLevel == 2)
		GetCOREInterface()->SetSubObjectLevel(EP_SL_EDGE);
	else if (hitSubLevel == 3)
		GetCOREInterface()->SetSubObjectLevel(EP_SL_FACE);

}

void SubobjectPickMode::EnterMode() {
	inMode = true;
}

void SubobjectPickMode::ExitMode() {
	inMode = false;
	mInOverride = false;
	HWND hWnd = mpEditPoly->GetDlgHandle (ep_select);
	PostMessage(hWnd,WM_SUBOBJECTPICK,proc.hitSubLevel,0);
};

void SubobjectPickMode::SetSubobjectLevel()
{
	proc.SetSubobjectLevel();
}

void SubobjectPickMode::Display (GraphicsWindow *gw) {
//	proc.DrawEstablishedFace(gw);

	IColorManager *pClrMan = GetColorManager();;

	Point3 newSubColor = pClrMan->GetColorAsPoint3(SUBOBJECT_PICK_COLORID);
	gw->setColor (FILL_COLOR,newSubColor);
	gw->setColor (LINE_COLOR,newSubColor);


	TimeValue t = GetCOREInterface()->GetTime();

	if(proc.hitSubLevel == 1)
	{
		mpEditPoly->DisplayHighlightVert(t,gw,&mpEditPoly->mm,proc.hitSubIndex);
	}
	else if(proc.hitSubLevel == 2)
	{
		mpEditPoly->DisplayHighlightEdge(t,gw,&mpEditPoly->mm,proc.hitSubIndex);
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
		MNFace *face = &mpEditPoly->mm.f[proc.hitSubIndex];
		face->GetTriangles(tri);
		int		*vv		= face->vtx;
		int		deg		= face->deg;
		DWORD	smGroup = face->smGroup;
		for (int tt=0; tt<tri.Count(); tt+=3)
		{
			int *triv = tri.Addr(tt);
			for (int z=0; z<3; z++) xyz[z] = mpEditPoly->mm.v[vv[triv[z]]].p;
			nor[0] = nor[1] = nor[2] = mpEditPoly->mm.GetFaceNormal(proc.hitSubIndex, TRUE);
			gw->triangleN(xyz,nor,uvw);
		}
		gw->endTriangles();		
	}
}


bool HasAdjacentSelection(MNMesh* mm, int subObjectLevel, int subObjectIndex, bool useRing, bool& isRing)
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
				//first check for the loop
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
				if (!hasSelectedNeighbor)
				{
					if (useRing)
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

	MNMesh& mm = mpEditPoly->mm;
	IMNMeshUtilities13* l_mesh13 = static_cast<IMNMeshUtilities13*>(mm.GetInterface(IMNMESHUTILITIES13_INTERFACE_ID));
	if (l_mesh13 == nullptr) return false;
	bool isRing = false;
	if (mode == EP_SL_VERTEX)
	{
		bool hasSelectedNeighbor = HasAdjacentSelection(&mm, MNM_SL_VERTEX, index, false, isRing);
		if (originalSelection.GetSize() != mm.numv)
		{
			mm.getVertexSel(originalSelection);
		}

		if (hasSelectedNeighbor && !originalSelection[index])
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
				mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
				mpEditPoly->RefreshScreen();
				mInLoop = true;
			}

		}
		else
		{
			mInLoop = false;
			mpEditPoly->SetVertSel(originalSelection, mpEditPoly, GetCOREInterface()->GetTime());
			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();
		}
	}
	else if (mode == EP_SL_EDGE)
	{
		bool hasSelectedNeighbor = HasAdjacentSelection(&mm, MNM_SL_EDGE, index, false, isRing);
		if (originalSelection.GetSize() != mm.nume)
		{
			mm.getEdgeSel(originalSelection);
		}
		if (hasSelectedNeighbor && !originalSelection[index])
		{
			BitArray loopEdges(originalSelection);
			loopEdges.ClearAll();
			loopEdges.Set(index, TRUE);
			{
				// The following saves the MN_MESH_NO_BAD_VERTS flag state, turns the flag on and then restores it when it goes out of scope.
				TemporaryNoBadVerts bv(mm);
				mm.SelectEdgeLoop(loopEdges);
			}
			selection = loopEdges;
			loopEdges |= originalSelection;
			mpEditPoly->SetEdgeSel(loopEdges, mpEditPoly, GetCOREInterface()->GetTime());
			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();
			mInLoop = true;
		}
		else
		{
			mInLoop = false;
			mpEditPoly->SetEdgeSel(originalSelection, mpEditPoly, GetCOREInterface()->GetTime());
			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();
		}
	}
	else if (mode == EP_SL_FACE)
	{
		bool hasSelectedNeighbor = HasAdjacentSelection(&mm, MNM_SL_FACE, index, false, isRing);

		if (originalSelection.GetSize() != mm.numf)
		{
			mm.getFaceSel(originalSelection);
		}

		if (hasSelectedNeighbor && !originalSelection[index])
		{
			IMNMeshUtilities13* l_mesh13 = static_cast<IMNMeshUtilities13*>(mm.GetInterface(IMNMESHUTILITIES13_INTERFACE_ID));
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
					mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
					mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
					mpEditPoly->RefreshScreen();
					mInLoop = true;
				}
			}
		}
		else
		{
			mInLoop = false;
			mpEditPoly->SetFaceSel(originalSelection, mpEditPoly, GetCOREInterface()->GetTime());
			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();
		}
	}

	return mInLoop;
}

int PointToPointProc::HitTest(IPoint2 &m, ViewExp& vpt)
{
	int res = -1;

	if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
	{
		int faceIndex = -1;
		vpt.ClearSubObjHitList();
		mpEditPoly->SetHitLevelOverride(SUBHIT_MNFACES);
		ip->SubObHitTest(ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
		mpEditPoly->ClearHitLevelOverride();
		if (vpt.NumSubObjHits())
		{
			HitLog& hitLog = vpt.GetSubObjHitList();
			HitRecord* hitRecord = hitLog.ClosestHit();
			faceIndex = hitRecord->hitInfo;
		}

		if (faceIndex != -1)
		{
			vpt.ClearSubObjHitList();
			mpEditPoly->SetHitLevelOverride(SUBHIT_MNVERTS);
			ip->SubObHitTest(ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
			mpEditPoly->ClearHitLevelOverride();
			if (vpt.NumSubObjHits())
			{
				HitLog& hitLog = vpt.GetSubObjHitList();
				HitRecord* hitRecord = hitLog.ClosestHit();
				int testHit = hitRecord->hitInfo;
				MNFace &face = mpEditPoly->mm.f[faceIndex];
				for (int i = 0; i < face.deg; i++)
				{
					if (face.vtx[i] == testHit)
						res = testHit;
				}
			}
		}
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
	{

		vpt.ClearSubObjHitList();
		mpEditPoly->SetHitLevelOverride(SUBHIT_MNFACES);
		ip->SubObHitTest(ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
		mpEditPoly->ClearHitLevelOverride();
		if (vpt.NumSubObjHits())
		{
			HitLog& hitLog = vpt.GetSubObjHitList();
			HitRecord* hitRecord = hitLog.ClosestHit();
			res = hitRecord->hitInfo;
		}
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE)
	{
		int faceIndex = -1;
		vpt.ClearSubObjHitList();
		mpEditPoly->SetHitLevelOverride(SUBHIT_MNFACES);
		ip->SubObHitTest(ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
		mpEditPoly->ClearHitLevelOverride();
		if (vpt.NumSubObjHits())
		{
			HitLog& hitLog = vpt.GetSubObjHitList();
			HitRecord* hitRecord = hitLog.ClosestHit();
			faceIndex = hitRecord->hitInfo;
		}

		if (faceIndex != -1)
		{
			vpt.ClearSubObjHitList();
			mpEditPoly->SetHitLevelOverride(SUBHIT_MNEDGES);
			ip->SubObHitTest(ip->GetTime(), HITTYPE_POINT, 0, HIT_ANYSOLID, &m, vpt.ToPointer());
			mpEditPoly->ClearHitLevelOverride();
			if (vpt.NumSubObjHits())
			{
				HitLog& hitLog = vpt.GetSubObjHitList();
				HitRecord* hitRecord = hitLog.ClosestHit();
				int testHit = hitRecord->hitInfo;
				MNEdge &edge = mpEditPoly->mm.e[testHit];
				if ((edge.f1 == faceIndex) || (edge.f2 == faceIndex))
					res = hitRecord->hitInfo;
			}
		}
	}

	return res;

}

void PointToPointProc::RestoreSelection()
{
	if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
	{
		mpEditPoly->SetVertSel(originalSelection, mpEditPoly, GetCOREInterface()->GetTime());
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE)
	{
		mpEditPoly->SetEdgeSel(originalSelection, mpEditPoly, GetCOREInterface()->GetTime());

	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
	{
		mpEditPoly->SetFaceSel(originalSelection, mpEditPoly, GetCOREInterface()->GetTime());
	}
	mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
	mpEditPoly->RefreshScreen();

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

void PointToPointProc::SetFirstHit(int index)
{
	//first see if we are adjacent loop test
	//otherwise point to point
	if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
	{
		mpEditPoly->mm.getVertexSel(originalSelection);
		mFirstHit = index;
		mPointToPointPath->SetStartPoint(mFirstHit);
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE)
	{
		mpEditPoly->mm.getEdgeSel(originalSelection);
		RebuildList(index);
		mFirstHit = index;
		mPointToPointPath->SetStartPoint(mpEditPoly->mm.numv);
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
	{
		mpEditPoly->mm.getFaceSel(originalSelection);
		mFirstHit = index;
		mPointToPointPath->SetStartPoint(mFirstHit);
	}
	mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
	mpEditPoly->RefreshScreen();
}

int PointToPointProc::proc(HWND hwnd, int msg, int point, int flags, IPoint2 m) {
	static HCURSOR hCutVertCur = NULL, hCutEdgeCur = NULL, hCutFaceCur = NULL;



	BOOL shiftDown = flags & MOUSE_SHIFT;

	static unsigned long ct = 0;

	switch (msg) {
	case MOUSE_DBLCLICK:
	case MOUSE_PROPCLICK:
	{
		RestoreSelection();
		PointToPointMode* mode = (PointToPointMode*)mpEditPoly->getCommandMode(epmode_point_to_point);
		if (mode->mShiftLaunched)
		{
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
	}
	return FALSE;
	break;

	case MOUSE_POINT:
	{
		ip->SetActiveViewport(hwnd);
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if (!vpt.IsAlive())
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
			if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
			{
				mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
			}
			else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE)
			{
				mpEditPoly->SetEdgeSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());

			}
			else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
			{
				mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
			}
			theHold.Accept(GetString(IDS_EP_POINT_TO_POINT));

			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();

			originalSelection = currentSel;

		}
		else if (mFirstHit == -1)
		{
			int hitIndex = HitTest(m, vpt);
			if (hitIndex != -1)
			{
				SetFirstHit(hitIndex);

			}

		}
		else if (mSecondHit != -1)
		{




			mFirstHit = mSecondHit;
			mSecondHit = -1;

			theHold.Begin();
			BitArray currentSel(originalSelection);
			currentSel |= selection;
			if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
			{
				mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				mPointToPointPath->SetStartPoint(mFirstHit);
			}
			else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE)
			{
				mpEditPoly->SetEdgeSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				RebuildList(mFirstHit);
				mPointToPointPath->SetStartPoint(mpEditPoly->mm.numv);

			}
			else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
			{
				mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				mPointToPointPath->SetStartPoint(mFirstHit);
			}
			theHold.Accept(GetString(IDS_EP_POINT_TO_POINT));

			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();

			originalSelection = currentSel;

		}

		return true;
	}
	case MOUSE_MOVE:
	case MOUSE_FREEMOVE:
	{
		PointToPointMode* mode = (PointToPointMode*)mpEditPoly->getCommandMode(epmode_point_to_point);
		if (mode->mShiftLaunched)
		{
			if (!shiftDown)
			{
				RestoreSelection();
				GetCOREInterface()->PopCommandMode();
				return FALSE;
			}
		}
		ViewExp& vpt = ip->GetViewExp(hwnd);
		if (!vpt.IsAlive())
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}


		int index = HitTest(m, vpt);
		if (index == -1)
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		else
			SetCursor(ip->GetSysCursor(SYSCUR_SELECT));

		bool loopSet = false;
		if (mode->mShiftLaunched && (index != -1) && (mFirstHit == -1))
		{
			loopSet = AdjacentLoop(GetCOREInterface()->GetSubObjectLevel(), index);
		}

		if ((mFirstHit != -1) && (index != -1) && !loopSet)
		{
			BitArray currentSel(originalSelection);
			mSecondHit = index;
			PointToPointMode* mode = (PointToPointMode*)mpEditPoly->getCommandMode(epmode_point_to_point);
			if ((index != -1) && (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX))
			{
				BitArray loopTest;
				ShortLoop(EP_SL_VERTEX, mFirstHit, mSecondHit, &mpEditPoly->mm, loopTest);

				if (loopTest.NumberSet() > 0)
				{
					selection = loopTest;
					currentSel |= selection;
					mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
				else
				{
					mPointToPointPath->SetEndPoint(index);
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
					mpEditPoly->SetVertSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
			}
			else if ((index != -1) && (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE))
			{
				BitArray loopTest;
				ShortLoop(EP_SL_EDGE, mFirstHit, mSecondHit, &mpEditPoly->mm, loopTest);

				if (loopTest.NumberSet() > 0)
				{
					selection = loopTest;
					currentSel |= selection;
					mpEditPoly->SetEdgeSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
				else
				{
					int v1 = mpEditPoly->mm.e[index].v1;
					mPointToPointPath->SetEndPoint(v1);
					Tab<MaxSDK::EdgeDescr> edges1;
					float d1 = mPointToPointPath->GetPath(edges1);

					int v2 = mpEditPoly->mm.e[index].v2;
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
					mpEditPoly->SetEdgeSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}
			}
			else if ((index != -1) && (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE))
			{
				mPointToPointPath->SetEndPoint(index);

				BitArray loopTest;
				ShortLoop(EP_SL_FACE, mFirstHit, mSecondHit, &mpEditPoly->mm, loopTest);

				if (loopTest.NumberSet() > 0)
				{
					selection = loopTest;
					currentSel |= selection;
					mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
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
					mpEditPoly->SetFaceSel(currentSel, mpEditPoly, GetCOREInterface()->GetTime());
				}

			}
			mpEditPoly->LocalDataChanged(SELECT_CHANNEL);
			mpEditPoly->RefreshScreen();
		}


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
	if (mode == EP_SL_VERTEX)
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

	else if (mode == EP_SL_EDGE)
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
	else if (mode == EP_SL_FACE)
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

	mPointToPointPath = MaxSDK::PointToPointPath::Create();
	if ((GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX) ||
		(GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE))
	{
		if (startEdgeIndex == -1)
			mPointToPointPath->SetNumberNodes(mpEditPoly->mm.numv);
		else
			mPointToPointPath->SetNumberNodes(mpEditPoly->mm.numv + 1);

		Tab<MaxSDK::EdgeDescr> edges;
		for (int i = 0; i < mpEditPoly->mm.numv; i++)
		{
			if (mpEditPoly->mm.v[i].GetFlag(MN_DEAD)) continue;
			edges.SetCount(0);
			int ct = mpEditPoly->mm.vedg[i].Count();
			for (int j = 0; j < ct; j++)
			{
				int edgeIndex = mpEditPoly->mm.vedg[i][j];
				int vertIndex = mpEditPoly->mm.e[edgeIndex].v1;
				if (vertIndex == i)
					vertIndex = mpEditPoly->mm.e[edgeIndex].v2;
				float d = Length(mpEditPoly->mm.v[i].p - mpEditPoly->mm.v[vertIndex].p);

				if (edgeIndex == startEdgeIndex)
				{
					MaxSDK::EdgeDescr eDescr(mpEditPoly->mm.numv, edgeIndex, 0.1f);
					edges.Append(1, &eDescr, 500);
				}
				else
				{
					MaxSDK::EdgeDescr eDescr(vertIndex, edgeIndex, d);
					edges.Append(1, &eDescr, 500);
				}
			}
			mPointToPointPath->AddNode(i, edges);
		}

		if (startEdgeIndex != -1)
		{
			edges.SetCount(0);
			MaxSDK::EdgeDescr eDescr1(mpEditPoly->mm.e[startEdgeIndex].v1, startEdgeIndex, 0.1f);
			edges.Append(1, &eDescr1, 500);
			MaxSDK::EdgeDescr eDescr2(mpEditPoly->mm.e[startEdgeIndex].v2, startEdgeIndex, 0.1f);
			edges.Append(1, &eDescr2, 500);
			mPointToPointPath->AddNode(mpEditPoly->mm.numv, edges);
		}

		if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
			selection.SetSize(mpEditPoly->mm.numv);
		else selection.SetSize(mpEditPoly->mm.nume);

	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
	{
		mPointToPointPath->SetNumberNodes(mpEditPoly->mm.numf);
		Tab<MaxSDK::EdgeDescr> edges;
		BitArray connectedFaces;
		connectedFaces.SetSize(mpEditPoly->mm.numf);


		for (int i = 0; i < mpEditPoly->mm.numf; i++)
		{
			MNFace& face = mpEditPoly->mm.f[i];
			if (face.GetFlag(MN_DEAD)) continue;
			connectedFaces.ClearAll();
			connectedFaces.Set(i, TRUE);

			edges.SetCount(0);
			Point3 faceCenter(0, 0, 0);
			mpEditPoly->mm.ComputeCenter(i, faceCenter);

			for (int fi = 0; fi < face.deg; fi++)
			{
				int edgeIndex = face.edg[fi];
				int faceIndex = mpEditPoly->mm.e[edgeIndex].f1;
				if ((faceIndex == -1) || (faceIndex == i))
					faceIndex = mpEditPoly->mm.e[edgeIndex].f2;
				if ((faceIndex != -1) && (faceIndex != i))
				{
					Point3 center(0, 0, 0);
					mpEditPoly->mm.ComputeCenter(faceIndex, center);
					float d = Length(center - faceCenter);
					MaxSDK::EdgeDescr eDescr(faceIndex, 0, d);
					connectedFaces.Set(faceIndex, TRUE);
					edges.Append(1, &eDescr, 500);
				}
			}
			for (int fi = 0; fi < face.deg; fi++)
			{
				int vertexIndex = face.vtx[fi];
				int ct = mpEditPoly->mm.vfac[vertexIndex].Count();

				for (int m = 0; m < ct; m++)
				{
					int faceIndex = mpEditPoly->mm.vfac[vertexIndex][m];
					if (!connectedFaces[faceIndex])
					{
						Point3 center(0, 0, 0);
						mpEditPoly->mm.ComputeCenter(faceIndex, center);
						float d = Length(faceCenter - center);
						MaxSDK::EdgeDescr eDescr(faceIndex, 0, d);
						connectedFaces.Set(faceIndex, TRUE);
						edges.Append(1, &eDescr, 500);
					}
				}

			}

			mPointToPointPath->AddNode(i, edges);
		}
		selection.SetSize(mpEditPoly->mm.numf);

	}

}

void PointToPointProc::StoreOffOriginalSelection()
{
	int numSet = 0;
	if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_VERTEX)
	{
		mpEditPoly->mm.getVertexSel(originalSelection);
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_EDGE)
	{
		originalSelection = mpEditPoly->GetEdgeSel();
		numSet = originalSelection.NumberSet();
	}
	else if (GetCOREInterface()->GetSubObjectLevel() == EP_SL_FACE)
	{
		mpEditPoly->mm.getFaceSel(originalSelection);
	}

}

#define IDT_TIMER 899

void PointToPointMode::EnterMode() {
	if (!mpEditPoly->mm.GetFlag(MN_MESH_FILLED_IN))
		mpEditPoly->mm.FillInMesh();
	if (!mpEditPoly->mm.GetFlag(MN_MESH_VERTS_ORDERED)) //BUG this flag is not getting set all the time so need to force the order now :(
		mpEditPoly->mm.OrderVerts();
	mInMode = true;
	proc.mFirstHit = -1;
	proc.mSecondHit = -1;
	proc.RebuildList(-1);
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

	KillTimer(GetCOREInterface()->GetMAXHWnd(), IDT_TIMER); // kill timer
};


void PointToPointMode::Display(ViewExp *vpt, GraphicsWindow *gw) {

	if (!IsWindowVisible(vpt->getGW()->getHWnd())) // if window's not visible, cp may be invalid
		return;

	TSTR title;
	title.printf(_T("%s"), GetString(IDS_EP_POINT_TO_POINT));

	//	if (proc.mFirstHit != -1)
	//		title.printf(_T("%s %d to %d"), GetString(IDS_EP_POINT_TO_POINT), proc.mFirstHit, proc.mSecondHit);

		// turn off z-buffering
	vpt->getGW()->setRndLimits(vpt->getGW()->getRndMode() & ~GW_Z_BUFFER);
	vpt->getGW()->setColor(TEXT_COLOR, GetUIColor(COLOR_VP_LABELS));
	vpt->getGW()->wText(&IPoint3(4, 32, 0), title.data()); //draw it undrneath the viewport text!
}