/**********************************************************************
 *<
	FILE: editsops.cpp

	DESCRIPTION:  Edit BezierShape OSM operations

	CREATED BY: Tom Hudson & Rolf Berteig

	HISTORY: created 25 April, 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/
#include "dllmain.h"
#include "editspl.h"
#include "evrouter.h"
#include "MouseCursors.h"

extern CoreExport Class_ID splineShapeClassID; 

// in mods.cpp
extern HINSTANCE hInstance;

/*-------------------------------------------------------------------*/

static void XORDottedLine( HWND hwnd, IPoint2 p0, IPoint2 p1, bool bErase, bool bPresent)
	{
	IPoint2 pts[2];
    pts[0].x = p0.x;
    pts[0].y = p0.y;
    pts[1].x = p1.x;
    pts[1].y = p1.y;
    XORDottedPolyline(hwnd, 2, pts, 0, bErase, !bPresent);
	}

/*-------------------------------------------------------------------*/

ESTempData::~ESTempData()
	{
	if (shape) delete shape;
	}

ESTempData::ESTempData(EditSplineMod *m,EditSplineData *sd)
	{
	shape = NULL;
	shapeValid.SetEmpty();
	shapeData = sd;
	mod = m;
	}

void ESTempData::Invalidate(ChannelMask channels,BOOL shapeValid)
	{
	if ( !shapeValid ) {
		delete shape;
		shape = NULL;
		}
	if ( channels & TOPO_CHANNEL ) {
		}
	if ( channels & GEOM_CHANNEL ) {
		}
	if ( channels & SELECT_CHANNEL ) {
		}
	}

BezierShape *ESTempData::GetShape(TimeValue t)
	{
	if ( shapeValid.InInterval(t) && shape ) {
		return shape;
	} else {
		shapeData->SetFlag(ESD_UPDATING_CACHE,TRUE);
		mod->NotifyDependents(Interval(t,t), PART_GEOM|PART_SELECT|PART_SUBSEL_TYPE|PART_DISPLAY|PART_TOPO, REFMSG_MOD_EVAL);
		shapeData->SetFlag(ESD_UPDATING_CACHE,FALSE);
		return shape;
		}
	}

BOOL ESTempData::ShapeCached(TimeValue t)
	{
	return (shapeValid.InInterval(t) && shape);
	}

void ESTempData::UpdateCache(SplineShape *splShape)
	{
	if ( shape ) delete shape;
	shape = new BezierShape(splShape->shape);

	shapeValid = FOREVER;
	
	// These are the channels we care about.
	shapeValid &= splShape->ChannelValidity(0,GEOM_CHAN_NUM);
	shapeValid &= splShape->ChannelValidity(0,TOPO_CHAN_NUM);
	shapeValid &= splShape->ChannelValidity(0,SELECT_CHAN_NUM);
	shapeValid &= splShape->ChannelValidity(0,SUBSEL_TYPE_CHAN_NUM);
	shapeValid &= splShape->ChannelValidity(0,DISP_ATTRIB_CHAN_NUM);	
	}

HCURSOR OutlineSelectionProcessor::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur =  UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Outline);
		}

	return hCur; 
	}

/*-------------------------------------------------------------------*/

HCURSOR FilletSelectionProcessor::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Fillet); 
		}

	return hCur; 
	}

/*-------------------------------------------------------------------*/

HCURSOR ESChamferSelectionProcessor::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Chamfer);
		}

	return hCur; 
	}

/*-------------------------------------------------------------------*/

HCURSOR SegBreakMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SegmentBreak);
		}

	return hCur; 
	}

BOOL SegBreakMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

int SegBreakMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp &vpt = ip->GetViewExp(hwnd);	

	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;

	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				HitRecord *bestRec = rec;
				DWORD best = rec->distance;
				while(rec) {
					rec = rec->Next();
					if(rec) {
						if(rec->distance < best) {
							best = rec->distance;
							bestRec = rec;
							}
						}
					}
				ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
				es->DoSegBreak(vpt.ToPointer(), hit->shape, hit->poly, hit->index, m);
				}
			res = FALSE;
			break;
		
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				SetCursor(GetTransformCursor());
				}
			else {
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
			break;
			
		}


	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR SegRefineMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SegmentRefine); 
		}

	return hCur; 
	}

BOOL SegRefineMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(ES_SEGMENT);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

int SegRefineMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp &vpt = ip->GetViewExp(hwnd);	

	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;

	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				HitRecord *bestRec = rec;
				DWORD best = rec->distance;
				while(rec) {
					rec = rec->Next();
					if(rec) {
						if(rec->distance < best) {
							best = rec->distance;
							bestRec = rec;
							}
						}
					}
				ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
				es->DoSegRefine(vpt.ToPointer(), hit->shape, hit->poly, hit->index, m);
				}
			res = FALSE;
			break;
		
		case MOUSE_FREEMOVE:
			vpt.SnapPreview(m,m,NULL, SNAP_IN_3D);
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				SetCursor(GetTransformCursor());
				}
			else {
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
			break;
			
		}


	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR CrossInsertMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CrossInsert);
		}

	return hCur; 
	}

BOOL CrossInsertMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(SS_SPLINE);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	// Make sure that there are exactly two hits and they are on different polygons of
	// the same shape
	if ( vpt->NumSubObjHits() == 2 ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		ShapeHitData *h1 = (ShapeHitData *)rec->hitData;
		rec = rec->Next();
		assert(rec);
		ShapeHitData *h2 = (ShapeHitData *)rec->hitData;
		if((h1->shape == h2->shape) && (h1->poly != h2->poly))
			return TRUE;
		}
	return FALSE;			
	}

int CrossInsertMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp &vpt = ip->GetViewExp(hwnd);	

	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;

	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				ShapeHitData *h1 = (ShapeHitData *)rec->hitData;
				rec = rec->Next();
				assert(rec);
				ShapeHitData *h2 = (ShapeHitData *)rec->hitData;
				assert(h1->shape == h2->shape);
				es->DoCrossInsert(vpt.ToPointer(), h1->shape, h1->poly, h1->index, h2->poly, h2->index, m);
				}
			res = FALSE;
			break;
		
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				SetCursor(GetTransformCursor());
				}
			else {
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
			break;
			
		}


	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR VertConnectMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::VerticalConnect);
		}

	return hCur; 
	}

BOOL VertConnectMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

BOOL VertConnectMouseProc::HitAnEndpoint(ViewExp *vpt, IPoint2 *p, BezierShape *shape, int poly, int vert,
	BezierShape **shapeOut, int *polyOut, int *vertOut) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int first = 1;
	
	if(HitTest(vpt, p, HITTYPE_POINT, 0) ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		DWORD best = 9999;
		HitRecord *bestRec = NULL;
		while(rec) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			// If there's an exclusion shape, this must be a part of it!
			if(!shape || shape == hit->shape) {
				// If there's an exclusion shape, the vert & poly can't be the same!
				if(!shape || (shape && !(poly == hit->poly && vert == hit->index))) {
					Spline3D *spline = hit->shape->splines[hit->poly];
					if(!spline->Closed()) {
						int hitKnot = hit->index / 3;
						if(hitKnot == 0 || hitKnot == (spline->KnotCount() - 1)) {
							if(first || rec->distance < best) {
								first = 0;
								best = rec->distance;
								bestRec = rec;
								}
							}
						}
					}
				}
			rec = rec->Next();
			}
		if(!first) {
			ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
			if(shapeOut)
				*shapeOut = hit->shape;
			if(polyOut)
				*polyOut = hit->poly;
			if(vertOut)
				*vertOut = hit->index;
			return TRUE;
			}
		}
	return FALSE;
	}

int VertConnectMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp & vpt = ip->GetViewExp(hwnd);	
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;
	static BezierShape *shape1 = NULL;
	static int poly1, vert1, poly2, vert2;
	static IPoint2 anchor, lastPoint;

	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			switch(point) {
				case 0:
					if(HitAnEndpoint(vpt.ToPointer(), &m, NULL, -1, -1, &shape1, &poly1, &vert1))
						res = TRUE;
					else
						res = FALSE;
					anchor = lastPoint = m;
					XORDottedLine(hwnd, anchor, m, false, true);	// Draw it!
					break;
				case 1:
					XORDottedLine(hwnd, anchor, lastPoint, true, true);	// Erase it!
					if(HitAnEndpoint(vpt.ToPointer(), &m, shape1, poly1, vert1, NULL, &poly2, &vert2))
						es->DoVertConnect(vpt.ToPointer(), shape1, poly1, vert1, poly2, vert2); 
					res = FALSE;
					break;
				default:
					assert(0);
				}
			break;

		case MOUSE_MOVE:
			// Erase old dotted line
			XORDottedLine(hwnd, anchor, lastPoint, true, false);
			// Draw new dotted line
			XORDottedLine(hwnd, anchor, m, false, true);
			lastPoint = m;

			if(HitAnEndpoint(vpt.ToPointer(), &m, shape1, poly1, vert1, NULL, NULL, NULL))
				SetCursor(GetTransformCursor());
			else
				SetCursor(LoadCursor(NULL,IDC_ARROW));

			break;
					
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				Spline3D *spline = hit->shape->splines[hit->poly];
				if(!spline->Closed()) {
					int hitKnot = hit->index / 3;
					if(hitKnot == 0 || hitKnot == (spline->KnotCount() - 1)) {
						SetCursor( UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Selection));
						break;
						}
					}
				}
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		
		case MOUSE_ABORT:
			// Erase old dotted line
			XORDottedLine(hwnd, anchor, lastPoint, true, true);
			break;			
		}


	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR VertInsertMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::VerticalInsert); 
		}

	return hCur; 
	}

BOOL VertInsertMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags, int hitType )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(hitType);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	if ( vpt->NumSubObjHits() )
		return TRUE;
	return FALSE;
	}

// Determine where we'll be inserting.  If they clicked on an endpoint, fill in 'vertOut'.
// If they clicked on a segment, fill in 'segOut'.

BOOL VertInsertMouseProc::InsertWhere(ViewExp *vpt, IPoint2 *p, BezierShape **shapeOut, int *polyOut,
		int *segOut, int *vertOut) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int first = 1;

	// Reset these!
	*segOut = -1;
	*vertOut = -1;

	// Only valid insertion vertices are endpoints of the spline
	if(HitTest(vpt, p, HITTYPE_POINT, 0, ES_VERTEX) ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		DWORD best = 9999;
		HitRecord *bestRec = NULL;
		while(rec) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			int hitKnot = hit->index / 3;
			Spline3D *spline = hit->shape->splines[hit->poly];
			if(!spline->Closed() && (hitKnot == 0 || hitKnot == (spline->KnotCount() - 1))) {
				if(first || rec->distance < best) {
					first = 0;
					best = rec->distance;
					bestRec = rec;
					}
				}
			rec = rec->Next();
			}
		if(!first) {
			ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
			if(shapeOut)
				*shapeOut = hit->shape;
			if(polyOut)
				*polyOut = hit->poly;
			if(vertOut)
				*vertOut = hit->index;
			return TRUE;
			}
		}

	// No hits on endpoints, so let's try segments!
	if(HitTest(vpt, p, HITTYPE_POINT, 0, ES_SEGMENT) ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		DWORD best = 9999;
		HitRecord *bestRec = NULL;
		while(rec) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			if(first || rec->distance < best) {
				first = 0;
				best = rec->distance;
				bestRec = rec;
				}
			rec = rec->Next();
			}
		if(!first) {
			ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
			if(shapeOut)
				*shapeOut = hit->shape;
			if(polyOut)
				*polyOut = hit->poly;
			if(segOut)
				*segOut = hit->index;
			return TRUE;
			}
		}

	return FALSE;
	}

class ESIBackspaceUser : public EventUser {
	Spline3D *spline;
	EditSplineMod *mod;

	public:
		void Notify();
		void SetPtrs(EditSplineMod *m,Spline3D *s) { mod = m; spline = s; }
	};

void ESIBackspaceUser::Notify() {
	if(spline->KnotCount() > 2) {
		// Tell the spline we just backspaced to remove the last point
		spline->Create(NULL, -1, 0, 0, IPoint2(0,0), NULL);
		mod->insertShapeData->Invalidate(GEOM_CHANNEL);
		mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime(), REDRAW_NORMAL);
		}
	}

static ESIBackspaceUser iBack;

int VertInsertMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp & vpt = ip->GetViewExp(hwnd);	
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;
	static int poly, seg, vert;
	static BezierShape *shape;
	static int inserting = FALSE;
	static Matrix3 mat;
	static EditSplineMod *mod;

	static int originalInsert;

	if(inserting) {
		// Ignore the messages the spline code doesn't care about... TH 3/12/99
		switch(msg) {
			case MOUSE_POINT:
				{
				}
			case MOUSE_MOVE:
			case MOUSE_ABORT:
				break;
			default:
				return TRUE;
			}
		int res = shape->splines[poly]->Create(vpt.ToPointer(),msg,point,flags,m,&mat,ip);
		
		// Must update the shape's selection set for this spline
		BitArray& vsel = shape->vertSel[poly];
		BitArray& ssel = shape->segSel[poly];
		while(vsel.GetSize() < shape->splines[poly]->Verts()) {
			vsel.SetSize(vsel.GetSize() + 3,1);
			vsel.Shift(RIGHT_BITSHIFT,3,vert+3);
			vsel.Clear(vert+2);
			vsel.Clear(vert+3);
			vsel.Clear(vert+4);
			}
		while(ssel.GetSize() < shape->splines[poly]->Segments()) {
			ssel.SetSize(ssel.GetSize() + 1,1);
			ssel.Shift(RIGHT_BITSHIFT,1,vert/3+1);
			ssel.Clear(vert/3+1);
			}
			
		switch(res) {
			case CREATE_STOP: {
				// Must update the shape's selection set for this spline
				BitArray& vsel = shape->vertSel[poly];
				BitArray& ssel = shape->segSel[poly];
				while(vsel.GetSize() > shape->splines[poly]->Verts()) {
					vsel.Shift(LEFT_BITSHIFT,3,vert+1);
					vsel.SetSize(vsel.GetSize() - 3,1);
					}
				while(ssel.GetSize() > shape->splines[poly]->Segments()) {
					ssel.Shift(LEFT_BITSHIFT,1,(vert+1)/3);
					ssel.SetSize(ssel.GetSize() - 1,1);
					}
				es->EndVertInsert();
				inserting = FALSE;
				backspaceRouter.UnRegister(&iBack);


				break;
				}
			case CREATE_ABORT:
				shape->DeleteSpline(poly);
				es->insertPoly = -1;
				es->EndVertInsert();
				inserting = FALSE;
				backspaceRouter.UnRegister(&iBack);
				break;
			}	
		mod->insertShapeData->Invalidate(GEOM_CHANNEL);
		mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		ip->RedrawViews(ip->GetTime(), REDRAW_NORMAL);
		}
	else {
		switch(msg)	{
			case MOUSE_PROPCLICK:
				ip->SetStdCommandMode(CID_OBJMOVE);
				break;

			case MOUSE_POINT:
				if(InsertWhere(vpt.ToPointer(), &m, &shape, &poly, &seg, &vert)) {
					vert = es->StartVertInsert(vpt.ToPointer(), shape, poly, seg, vert, &mod);
					if(vert < 0) {
						res = FALSE;
						break;
						}
					originalInsert = seg;
					mat = mod->insertTM;
					shape->splines[poly]->StartInsert(vpt.ToPointer(), msg, point, flags, m, &mat, vert/3+1 );
					// Must update the shape's selection set for this spline
					BitArray& vsel = shape->vertSel[poly];
					BitArray& ssel = shape->segSel[poly];
					while(vsel.GetSize() < shape->splines[poly]->Verts()) {
						vsel.SetSize(vsel.GetSize() + 3,1);
						vsel.Shift(RIGHT_BITSHIFT,3,vert+3);
						vsel.Clear(vert+2);
						vsel.Clear(vert+3);
						vsel.Clear(vert+4);
						}
					while(ssel.GetSize() < shape->splines[poly]->Segments()) {
						ssel.SetSize(ssel.GetSize() + 1,1);
						ssel.Shift(RIGHT_BITSHIFT,1,vert/3+1);
						ssel.Clear(vert/3+1);
						}
					inserting = TRUE;
					iBack.SetPtrs(mod, shape->splines[poly]);
					backspaceRouter.Register(&iBack);
					}
				else
					res = FALSE;
				break;

			case MOUSE_MOVE:
			case MOUSE_ABORT:
				res = FALSE;
				break;

			case MOUSE_FREEMOVE:
				if ( HitTest(vpt.ToPointer(), &m, HITTYPE_POINT, HIT_ABORTONHIT, ES_VERTEX) )
					SetCursor(GetTransformCursor());
				else
				if ( HitTest(vpt.ToPointer(), &m, HITTYPE_POINT, HIT_ABORTONHIT, ES_SEGMENT) )
					SetCursor(GetTransformCursor());
				else
					SetCursor(LoadCursor(NULL,IDC_ARROW));
				break;
			
			}
		}

	return res;
	}

/*-------------------------------------------------------------------*/

class ESMBackspaceUser : public EventUser {
	Spline3D *spline;
	EditSplineMod *mod;
	public:
		void Notify();
		void SetPtrs(EditSplineMod *m,Spline3D *s) { mod = m; spline = s; }
	};

void ESMBackspaceUser::Notify() {
	if(spline->KnotCount() > 2) {
		// Tell the spline we just backspaced to remove the last point
		spline->Create(NULL, -1, 0, 0, IPoint2(0,0), NULL);
		mod->createShapeData->Invalidate(GEOM_CHANNEL);
		mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime(), REDRAW_NORMAL);
		}
	}

static ESMBackspaceUser pBack;

BOOL CreateLineMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags, int hitType )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(hitType);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	if ( vpt->NumSubObjHits() )
		return TRUE;
	return FALSE;
	}

// Determine where we'll be inserting.  If they clicked on an endpoint, fill in 'vertOut'.
// If they clicked on a segment, fill in 'segOut'.

BOOL CreateLineMouseProc::InsertWhere(ViewExp *vpt, IPoint2 *p, BezierShape **shapeOut, int *polyOut,
		int *segOut, int *vertOut) {
	
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int first = 1;

	// Reset these!
	*segOut = -1;
	*vertOut = -1;

	// Only valid insertion vertices are endpoints of the spline
	if(HitTest(vpt, p, HITTYPE_POINT, 0, ES_VERTEX) ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		DWORD best = 9999;
		HitRecord *bestRec = NULL;
		while(rec) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			int hitKnot = hit->index / 3;
			Spline3D *spline = hit->shape->splines[hit->poly];
			if(!spline->Closed() && (hitKnot == 0 || hitKnot == (spline->KnotCount() - 1))) {
				if(first || rec->distance < best) {
					first = 0;
					best = rec->distance;
					bestRec = rec;
					}
				}
			rec = rec->Next();
			}
		if(!first) {
			ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
			if(shapeOut)
				*shapeOut = hit->shape;
			if(polyOut)
				*polyOut = hit->poly;
			if(vertOut)
				*vertOut = hit->index;
			return TRUE;
			}
		}

	return FALSE;
	}

int CreateLineMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp & vpt = ip->GetViewExp(hwnd);	
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }

	int res = TRUE;
	static int poly, seg, vert;
	static BezierShape *shape;
	static Spline3D *spline;
	static BOOL creating = FALSE;
	static BOOL inserting = FALSE;
	static Matrix3 mat;
	static EditSplineMod *mod;

	static int originalInsert;

	if(creating) {
		// Ignore the messages the spline code doesn't care about... TH 3/12/99
		switch(msg) {
			case MOUSE_POINT:
			case MOUSE_MOVE:
			case MOUSE_ABORT:
				break;
			default:
				return TRUE;
			}
		vpt.getGW()->setTransform(mat);	
		int res = shape->splines[poly]->Create(vpt.ToPointer(),msg,point,flags,m,&mat,ip);
		// Must update the shape's selection set for this spline
		shape->UpdateSels(TRUE);		
			
		switch(res) {
			case CREATE_STOP:
				es->EndCreateLine();
				creating = FALSE;
				backspaceRouter.UnRegister(&pBack);
				break;
			case CREATE_ABORT:
				shape->DeleteSpline(poly);
				es->EndCreateLine(FALSE);
				creating = FALSE;
				backspaceRouter.UnRegister(&pBack);
				break;
			}	
		mod->createShapeData->Invalidate(GEOM_CHANNEL);
		mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		ip->RedrawViews(ip->GetTime(), REDRAW_NORMAL);
		}
	else
	if(inserting) {
		// Ignore the messages the spline code doesn't care about... TH 3/12/99
		switch(msg) {
			case MOUSE_POINT:
				{
				for (int i = 0; i < shape->bindList.length();i++)
					{
					if (poly == shape->bindList[i].segSplineIndex)  
						{
						if ((seg!=-1)&&(originalInsert <= shape->bindList[i].seg))
							{
							if (originalInsert == shape->bindList[i].seg)
								{
									{
									shape->bindList[i].seg++;
									}
								}
							else
								{
								shape->bindList[i].seg++;

								}
							}
						}
					}
				}
			case MOUSE_MOVE:
			case MOUSE_ABORT:
				break;
			default:
				return TRUE;
			}
		int res = shape->splines[poly]->Create(vpt.ToPointer(),msg,point,flags,m,&mat,ip);
		
		// Must update the shape's selection set for this spline
		BitArray& vsel = shape->vertSel[poly];
		BitArray& ssel = shape->segSel[poly];
		while(vsel.GetSize() < shape->splines[poly]->Verts()) {
			vsel.SetSize(vsel.GetSize() + 3,1);
			vsel.Shift(RIGHT_BITSHIFT,3,vert+3);
			vsel.Clear(vert+2);
			vsel.Clear(vert+3);
			vsel.Clear(vert+4);
			}
		while(ssel.GetSize() < shape->splines[poly]->Segments()) {
			ssel.SetSize(ssel.GetSize() + 1,1);
			ssel.Shift(RIGHT_BITSHIFT,1,vert/3+1);
			ssel.Clear(vert/3+1);
			}
			
		switch(res) {
			case CREATE_STOP: {
				// Must update the shape's selection set for this spline
				BitArray& vsel = shape->vertSel[poly];
				BitArray& ssel = shape->segSel[poly];
				while(vsel.GetSize() > shape->splines[poly]->Verts()) {
					vsel.Shift(LEFT_BITSHIFT,3,vert+1);
					vsel.SetSize(vsel.GetSize() - 3,1);
					}
				while(ssel.GetSize() > shape->splines[poly]->Segments()) {
					ssel.Shift(LEFT_BITSHIFT,1,(vert+1)/3);
					ssel.SetSize(ssel.GetSize() - 1,1);
					}
				es->EndVertInsert();
				inserting = FALSE;
				backspaceRouter.UnRegister(&iBack);
				for (int i = 0; i < shape->bindList.length();i++)
					{
					if (poly == shape->bindList[i].segSplineIndex)  
						{
						if ((seg!=-1)&&(originalInsert <= shape->bindList[i].seg))
							{
							if (originalInsert == shape->bindList[i].seg)
								{
									{
									shape->bindList[i].seg--;
									}
								}
							else
								{
								shape->bindList[i].seg--;

								}
							}
						}
					}


				break;
				}
			case CREATE_ABORT:
				shape->DeleteSpline(poly);
				es->insertPoly = -1;
				es->EndVertInsert();
				inserting = FALSE;
				backspaceRouter.UnRegister(&iBack);
				break;
			}	
		mod->insertShapeData->Invalidate(GEOM_CHANNEL);
		mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		ip->RedrawViews(ip->GetTime(), REDRAW_NORMAL);
		}
	else
		{
		switch(msg)	{
			case MOUSE_PROPCLICK:
				ip->SetStdCommandMode(CID_OBJMOVE);
				break;

			case MOUSE_POINT:
				mod = es;
				if((SplineShape::GetUseEndPointAutoConnect()==BST_CHECKED) && 
					InsertWhere(vpt.ToPointer(), &m, &shape, &poly, &seg, &vert)) {
					vert = es->StartVertInsert(vpt.ToPointer(), shape, poly, seg, vert, &mod);
					if(vert < 0) {
						res = FALSE;
						break;
						}
					originalInsert = seg;
					mat = mod->insertTM;
					shape->splines[poly]->StartInsert(vpt.ToPointer(), msg, point, flags, m, &mat, vert/3+1 );
					// Must update the shape's selection set for this spline
					BitArray& vsel = shape->vertSel[poly];
					BitArray& ssel = shape->segSel[poly];
					while(vsel.GetSize() < shape->splines[poly]->Verts()) {
						vsel.SetSize(vsel.GetSize() + 3,1);
						vsel.Shift(RIGHT_BITSHIFT,3,vert+3);
						vsel.Clear(vert+2);
						vsel.Clear(vert+3);
						vsel.Clear(vert+4);
						}
					while(ssel.GetSize() < shape->splines[poly]->Segments()) {
						ssel.SetSize(ssel.GetSize() + 1,1);
						ssel.Shift(RIGHT_BITSHIFT,1,vert/3+1);
						ssel.Clear(vert/3+1);
						}
					inserting = TRUE;
					iBack.SetPtrs(mod, shape->splines[poly]);
					backspaceRouter.Register(&iBack);
					}
				else {
					if(!mod->StartCreateLine(&shape))
						return CREATE_ABORT;
					poly = shape->splineCount;
					spline = shape->NewSpline();
					shape->UpdateSels(TRUE);
					mat = mod->createTM;
					vpt.getGW()->setTransform(mat);	
					if(spline->Create(vpt.ToPointer(),msg,point,flags,m,&mat,ip) == CREATE_CONTINUE) {
						creating = TRUE;
						shape->UpdateSels(TRUE);
						pBack.SetPtrs(mod, spline);
						backspaceRouter.Register(&pBack);
						}
					else {
						shape->DeleteSpline(poly);
						res = FALSE;
						}
					}
				SetCursor( UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Crosshair));
				break;

			case MOUSE_MOVE:
			case MOUSE_ABORT:
				res = FALSE;
				break;

			case MOUSE_FREEMOVE:
				if((SplineShape::GetUseEndPointAutoConnect()==BST_CHECKED) && 
					InsertWhere(vpt.ToPointer(), &m, &shape, &poly, &seg, &vert))
					SetCursor( UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CreateWeld));
				else
					SetCursor( UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Crosshair));
				vpt.SnapPreview(m,m,NULL, SNAP_IN_3D);
				break;
			
			}
		}

	return res;
	}

/*-------------------------------------------------------------------*/
// CAL-02/26/03: Add Cross Section. (FID #827)

bool CrossSectionMouseProc::mCreating = false;
bool CrossSectionMouseProc::mCrossingDrawn = false;
EditSplineData *CrossSectionMouseProc::mShapeData = NULL;
int CrossSectionMouseProc::mPolys;
Tab<int> CrossSectionMouseProc::mSelectedSplines;
Matrix3 CrossSectionMouseProc::mObjToWorldTM;
IPoint2 CrossSectionMouseProc::mMouse;

HCURSOR CrossSectionMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur )
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::CrossSection);

	return hCur;
	}

HitRecord* CrossSectionMouseProc::HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	HitRecord *bestRec = NULL;

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(SS_SPLINE);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	
	if ( vpt->NumSubObjHits() ) {
		HitLog &hits = vpt->GetSubObjHitList();
		DWORD best = 0;
		for (HitRecord *rec = hits.First(); rec != NULL; rec = rec->Next()) {
			if (mCreating) {
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				if (hit->poly >= mPolys) continue;
			}
			if(bestRec == NULL || rec->distance < best) {
				best = rec->distance;
				bestRec = rec;
				}
			}
		}
	return bestRec;
	}

void CrossSectionMouseProc::DrawCrossing(HWND hWnd, bool bErase, bool bPresent)
	{
	if (mShapeData == NULL) return;

	BezierShape *shape = mShapeData->TempData(es)->GetShape(ip->GetTime());
	if (shape == NULL) return;

	int polys = mSelectedSplines.Count();
	if (polys <= 0) return;

	Spline3D *spline = shape->GetSpline(mSelectedSplines[polys-1]);
	int knots = spline->KnotCount();
	Point3 p(0.0f, 0.0f, 0.0f);
	IPoint3 sp;

	ViewExp & vpt = ip->GetViewExp(hWnd);
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return;
	 }

	GraphicsWindow *gw = vpt.getGW();

	gw->setTransform(mObjToWorldTM);

	for (int i = 0, j = 0; i < 2 && i <= j; i++, j += (knots-1)) {
		if (knots > i) p = spline->GetKnotPoint(j);
		gw->wTransPoint(&p, &sp);
		IPoint2 pts[2];
	    pts[0].x = sp.x;
	    pts[0].y = sp.y;
	    pts[1].x = mMouse.x;
	    pts[1].y = mMouse.y;
	    XORDottedPolyline(hWnd, 2, pts, 0, bErase, !bPresent && !((i+1) < 2 && (i+1) <= j+(knots-1)));
		}
	}

int CrossSectionMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp& vpt = ip->GetViewExp(hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	HitRecord *rec = NULL;
	int res = TRUE;

	switch ( msg ) {
		case MOUSE_ABORT:
			if (mCreating) {
				if (mCrossingDrawn)
					DrawCrossing(hwnd, true, true);		// erase the previous line
				mCreating = false;
				es->EndCrossSection(mSelectedSplines.Count() > 1 ? TRUE : FALSE);
			}
			res = FALSE;
			break;

		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			res = FALSE;
			break;

		case MOUSE_POINT:
			if((rec = HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0)) != NULL) {
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				int poly = hit->poly;
				mObjToWorldTM = rec->nodeRef->GetObjectTM(ip->GetTime());

				if (!mCreating) {
					if (!es->StartCrossSection(&mShapeData))
						return CREATE_ABORT;
					mCreating = true;
					mSelectedSplines.ZeroCount();
					BezierShape *shape = mShapeData->TempData(es)->GetShape(ip->GetTime());
					mPolys = (shape == NULL) ? 0 : shape->SplineCount();
					}
				mSelectedSplines.Append(1, &poly, 1);
				es->DoCrossSection(mShapeData, mSelectedSplines);
				mMouse = m;
				mCrossingDrawn = false;
				}

			if (!mCreating) res = FALSE;
			break;
		
		case MOUSE_MOVE:
			if (mCreating) {
				if (mCrossingDrawn)
					DrawCrossing(hwnd, true, false);		// erase the previous line
				mMouse = m;
				DrawCrossing(hwnd, false, true);
				mCrossingDrawn = true;
				}
			// continue to the next case to set cursor.
			
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) )
				SetCursor(GetTransformCursor());
			else
				SetCursor(LoadCursor(NULL,IDC_ARROW));

			break;
		}

	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR BooleanMouseProc::GetTransformCursor() 
	{ 
		return UI::MouseCursors::LoadMouseCursor(es->GetBoolCursorID());
	}

BOOL BooleanMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	if ( vpt->NumSubObjHits() )
		return TRUE;
	return FALSE;
	}

int BooleanMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp &vpt = ip->GetViewExp(hwnd);	

	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;
	static int poly2 = -1;

	switch(msg)	{
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			if(poly2 >= 0)
				if(ValidBooleanPolygon(es->ip, es->boolShape->splines[poly2]))
					es->DoBoolean(poly2);
			res = FALSE;
			break;

		case MOUSE_MOVE:
		case MOUSE_ABORT:
			res = FALSE;
			break;

		case MOUSE_FREEMOVE:
			poly2 = -1;
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				Spline3D *spline = hit->shape->splines[hit->poly];
				if(spline->Closed() && hit->shape == es->boolShape && hit->poly != es->boolPoly1) {
					poly2 = hit->poly;
					SetCursor(GetTransformCursor());
					break;
					}
				}
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		
		}

	return res;
	}

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
//watje


HCURSOR BindMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::VerticalConnect);
		}

	return hCur; 
	}

BOOL BindMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}


BOOL BindMouseProc::HitTestSeg( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(SS_SEGMENT);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

BOOL BindMouseProc::HitAnEndpoint(ViewExp *vpt, IPoint2 *p, BezierShape *shape, int poly, int vert,
	BezierShape **shapeOut, int *polyOut, int *vertOut) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int first = 1;
	
	if(HitTest(vpt, p, HITTYPE_POINT, 0) ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		DWORD best = 9999;
		HitRecord *bestRec = NULL;
		while(rec) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			// If there's an exclusion shape, this must be a part of it!
			if(!shape || shape == hit->shape) {
				// If there's an exclusion shape, the vert & poly can't be the same!
				if(!shape || (shape && !(poly == hit->poly && vert == hit->index))) {
					Spline3D *spline = hit->shape->splines[hit->poly];
					if(!spline->Closed()) {
						int hitKnot = hit->index / 3;
						if(hitKnot == 0 || hitKnot == (spline->KnotCount() - 1)) {
							if(first || rec->distance < best) {
								first = 0;
								best = rec->distance;
								bestRec = rec;
								}
							}
						}
					}
				}
			rec = rec->Next();
			}
		if(!first) {
			ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
			if(shapeOut)
				*shapeOut = hit->shape;
			if(polyOut)
				*polyOut = hit->poly;
			if(vertOut)
				*vertOut = hit->index;
			return TRUE;
			}
		}
	return FALSE;
	}


BOOL BindMouseProc::HitASegment(ViewExp *vpt, IPoint2 *p, BezierShape *shape, int poly, int vert,
	BezierShape **shapeOut, int *polyOut, int *vertOut) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int first = 1;
	
	SetSplineHitOverride(SS_SEGMENT);
	if(HitTest(vpt, p, HITTYPE_POINT, 0) ) {
		HitLog &hits = vpt->GetSubObjHitList();
		HitRecord *rec = hits.First();
		DWORD best = 9999;
		HitRecord *bestRec = NULL;
		while(rec) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			// If there's an exclusion shape, this must be a part of it!
			Spline3D *spline = hit->shape->splines[hit->poly];
			int scount = spline->KnotCount()-1;
			if ((poly == hit->poly) && ((vert-1) == 0) && (hit->index == 0))
				{
				}
			else if ((poly == hit->poly) && (vert == ((scount+1)*3-2)) && (hit->index == (scount-1)))
				{
				}
			else if(first || rec->distance < best) 
				{
				first = 0;
				best = rec->distance;
				bestRec = rec;
				}
			rec = rec->Next();
			}
		if(!first) {
			ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
			if(shapeOut)
				*shapeOut = hit->shape;
			if(polyOut)
				*polyOut = hit->poly;
			if(vertOut)
				*vertOut = hit->index;
			ClearSplineHitOverride();
			return TRUE;
			}
		}
	ClearSplineHitOverride();
	return FALSE;

	}


int BindMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp &vpt = ip->GetViewExp(hwnd);	
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }


	int res = TRUE;
	static BezierShape *shape1 = NULL;
	static int poly1, vert1, poly2, vert2;
	static IPoint2 anchor, lastPoint;

	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			switch(point) {
				case 0:
					if(HitAnEndpoint(vpt.ToPointer(), &m, NULL, -1, -1, &shape1, &poly1, &vert1))
						res = TRUE;
					else
						res = FALSE;
					anchor = lastPoint = m;
					XORDottedLine(hwnd, anchor, m, false, true);	// Draw it!
					break;
				case 1:
					XORDottedLine(hwnd, anchor, lastPoint, true, true);	// Erase it!
					if(HitASegment(vpt.ToPointer(), &m, shape1, poly1, vert1, NULL, &poly2, &vert2))
						ss->DoBind( poly1, vert1, poly2, vert2); 
					res = FALSE;
					break;
				default:
					assert(0);
				}
			break;

		case MOUSE_MOVE:
			// Erase old dotted line
			XORDottedLine(hwnd, anchor, lastPoint, true, false);
			// Draw new dotted line
			XORDottedLine(hwnd, anchor, m, false, true);
			lastPoint = m;

			if(HitASegment(vpt.ToPointer(), &m, shape1, poly1, vert1, NULL, NULL, NULL))
				SetCursor(GetTransformCursor());
			else
				SetCursor(LoadCursor(NULL,IDC_ARROW));

			break;
					
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				Spline3D *spline = hit->shape->splines[hit->poly];
				if(!spline->Closed()) {
					int hitKnot = hit->index / 3;
					if(hitKnot == 0 || hitKnot == (spline->KnotCount() - 1)) {
						SetCursor(UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Selection));
						break;
						}
					}
				}
			else SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		
		case MOUSE_ABORT:
			// Erase old dotted line
			XORDottedLine(hwnd, anchor, lastPoint, true, true);
			break;			
		}


	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR RefineConnectMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SegmentRefine);
		}

	return hCur; 
	}

BOOL RefineConnectMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(SS_SEGMENT);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

int RefineConnectMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp &vpt = ip->GetViewExp(hwnd);	
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }

	int res = TRUE;

	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				HitRecord *bestRec = rec;
				DWORD best = rec->distance;
				while(rec) {
					rec = rec->Next();
					if(rec) {
						if(rec->distance < best) {
							best = rec->distance;
							bestRec = rec;
							}
						}
					}
				ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
				ss->DoRefineConnect(vpt.ToPointer(), hit->shape, hit->poly, hit->index, m);
				}
			res = FALSE;
			break;
		
		case MOUSE_FREEMOVE:
			vpt.SnapPreview(m,m,NULL, SNAP_IN_3D);
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				SetCursor(GetTransformCursor());
				}
			else {
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
			break;
			
		}


	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR TrimMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur =  UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Trim);
		}

	return hCur; 
	}

BOOL TrimMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

int TrimMouseProc::proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m )
	{	
	ViewExp &vpt = ip->GetViewExp(hwnd);
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }

	int res = TRUE;
//	IntersectPt fromPt, toPt;
	BOOL extend = FALSE;
	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:						
			if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				HitRecord *bestRec = rec;
				DWORD best = rec->distance;
				while(rec) {
					rec = rec->Next();
					if(rec) {
						if(rec->distance < best) {
							best = rec->distance;
							bestRec = rec;
							}
						}
					}
				ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
				es->HandleTrimExtend(vpt.ToPointer(), hit, m, SHAPE_TRIM);
				}
			res = FALSE;
			break;

		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				SetCursor(GetTransformCursor());
				}
			else {
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
			break;
		}
	

	return res;
	}

/*-------------------------------------------------------------------*/

HCURSOR ExtendMouseProc::GetTransformCursor() 
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur )
		hCur =  UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Trim);

	return hCur; 
	}

BOOL ExtendMouseProc::HitTest( 
		ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	vpt->ClearSubObjHitList();
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	if ( vpt->NumSubObjHits() ) {
		return TRUE;
	} else {
		return FALSE;
		}			
	}

int ExtendMouseProc::proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m )
	{	
	ViewExp &vpt = ip->GetViewExp(hwnd);
	if ( ! vpt.IsAlive() )
	 {
		 // why are we here
		 DbgAssert(!_T("Invalid viewport!"));
		 return FALSE;
	 }

	int res = TRUE;
	BOOL extend = FALSE;
	switch ( msg ) {
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:						
			if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
				HitLog &hits = vpt.GetSubObjHitList();
				HitRecord *rec = hits.First();
				HitRecord *bestRec = rec;
				DWORD best = rec->distance;
				while(rec) {
					rec = rec->Next();
					if(rec) {
						if(rec->distance < best) {
							best = rec->distance;
							bestRec = rec;
							}
						}
					}
				ShapeHitData *hit = ((ShapeHitData *)bestRec->hitData);
				es->HandleTrimExtend(vpt.ToPointer(), hit, m, SHAPE_EXTEND);
				}
			res = FALSE;
			break;

		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
				SetCursor(GetTransformCursor());
				}
			else {
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				}
			break;
		}
	

	return res;
	}


/*-------------------------------------------------------------------*/
// CAL-03/03/03: copy tangent. (FID #827)

HCURSOR CopyTangentMouseProc::GetTransformCursor()
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur )
		hCur =  UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Copy);

	return hCur;
	}

HitRecord* CopyTangentMouseProc::HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	HitRecord *bestRec = NULL;

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(SS_VERTEX);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	
	if ( vpt->NumSubObjHits() ) {
		HitLog &hits = vpt->GetSubObjHitList();
		DWORD best = 0;
		for (HitRecord *rec = hits.First(); rec != NULL; rec = rec->Next()) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			if ((hit->index % 3) == 1) continue;
			if(bestRec == NULL || rec->distance < best) {
				best = rec->distance;
				bestRec = rec;
				}
			}
		}
	return bestRec;
	}

int CopyTangentMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp& vpt = ip->GetViewExp(hwnd);;
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	HitRecord *rec = NULL;

	switch ( msg ) {
		case MOUSE_ABORT:
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			if((rec = HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0)) != NULL) {
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				EditSplineData *shapeData = (EditSplineData*)rec->modContext->localData;
				if ( shapeData ) {
					BezierShape *shape = shapeData->TempData(es)->GetShape(ip->GetTime());
					if (es->CopyTangent(shape, hit->poly, hit->index))
						ip->SetStdCommandMode(CID_OBJMOVE);
					}
				}

			break;
		
		case MOUSE_MOVE:
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) )
				SetCursor(GetTransformCursor());
			else
				SetCursor(LoadCursor(NULL,IDC_ARROW));

			break;
		}

	return FALSE;
	}

/*-------------------------------------------------------------------*/
// CAL-03/03/03: paste tangent. (FID #827)

HCURSOR PasteTangentMouseProc::GetTransformCursor()
	{ 
	static HCURSOR hCur = NULL;

	if ( !hCur )
		hCur =  UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Paste);

	return hCur;
	}

HitRecord* PasteTangentMouseProc::HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	HitRecord *bestRec = NULL;

	vpt->ClearSubObjHitList();
	SetSplineHitOverride(SS_VERTEX);
	ip->SubObHitTest(ip->GetTime(),type,ip->GetCrossing(),flags,p,vpt);
	ClearSplineHitOverride();
	
	if ( vpt->NumSubObjHits() ) {
		HitLog &hits = vpt->GetSubObjHitList();
		DWORD best = 0;
		for (HitRecord *rec = hits.First(); rec != NULL; rec = rec->Next()) {
			ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
			if ((hit->index % 3) == 1) continue;
			if(bestRec == NULL || rec->distance < best) {
				best = rec->distance;
				bestRec = rec;
				}
			}
		}
	return bestRec;
	}

int PasteTangentMouseProc::proc(
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m )
	{
	ViewExp& vpt = ip->GetViewExp(hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	HitRecord *rec = NULL;

	switch ( msg ) {
		case MOUSE_ABORT:
		case MOUSE_PROPCLICK:
			ip->SetStdCommandMode(CID_OBJMOVE);
			break;

		case MOUSE_POINT:
			
			if((rec = HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0)) != NULL) {
				ShapeHitData *hit = ((ShapeHitData *)rec->hitData);
				EditSplineData *shapeData = (EditSplineData*)rec->modContext->localData;
				if ( shapeData ) {
					es->StartPasteTangent(shapeData);
					BezierShape *shape = shapeData->TempData(es)->GetShape(ip->GetTime());
					es->PasteTangent(shape, hit->poly, hit->index);
					es->EndPasteTangent(shapeData);
					}
				}

			break;
		
		case MOUSE_MOVE:
		case MOUSE_FREEMOVE:
			if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) )
				SetCursor(GetTransformCursor());
			else
				SetCursor(LoadCursor(NULL,IDC_ARROW));

			break;
		}

	return FALSE;
	}

/*-------------------------------------------------------------------*/
