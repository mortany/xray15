/**********************************************************************
 *<
	FILE: gridhelp.cpp

	DESCRIPTION:  A grid helper implementation

	CREATED BY: Tom Hudson (based on Dan Silva's Object implementations)

	HISTORY: created 31 January 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#include "helpers.h"
#include "gridhelp.h"
#include <trig.h>

#define _USE_MATH_DEFINES
#include <math.h>

 // in helpers.cpp  - The dll instance handle
extern HINSTANCE hInstance;

//------------------------------------------------------

class GridHelpObjClassDesc :public ClassDesc2 {
	public:
	int 				IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override { return new GridHelpObject; }
	const TCHAR *	ClassName() override { return GetString(IDS_DB_GRID_CLASS); }
	SClass_ID		SuperClassID() override { return HELPER_CLASS_ID; }
	Class_ID			ClassID() override { return Class_ID(GRIDHELP_CLASS_ID, 0); }
	const TCHAR* 	Category() override { return _T(""); }
	const TCHAR*	InternalName() override { return _T("Grid"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle
	};

static GridHelpObjClassDesc gridHelpObjDesc;

ClassDesc* GetGridHelpDesc() { return &gridHelpObjDesc; }

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { 
	grid_params = GRIDHELP_PARAMBLOCK_ID, 
};
enum grid_param_param_ids {
	grid_length = GRIDHELP_LENGTH, 
	grid_width = GRIDHELP_WIDTH, 
	grid_grid = GRIDHELP_GRID,
	grid_active_color = GRIDHELP_ACTIVECOLOR, 
	grid_display_plane = GRIDHELP_DISPLAYPLANE, 
	grid_mytm,
};

// radio buttons
enum { id_gray, id_object_color, id_home_color, id_home_intensity };
enum { id_xy_plane, id_yz_plane, id_zx_plane };

// per instance grid block
static ParamBlockDesc2 grid_param_blk(grid_params, _T("GridParameters"), 0, &gridHelpObjDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_GRIDPARAM, IDS_DB_PARAMETERS, 0, 0, NULL,
	// params
	grid_length, _T("length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_TH_LENGTH,
		p_default, BDEF_DIM,
		p_ms_default, 50.0,
		p_range, BMIN_LENGTH, BMAX_LENGTH,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTHEDIT, IDC_LENSPINNER, SPIN_AUTOSCALE,
		p_end,
	grid_width, _T("width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_TH_WIDTH,
		p_default, BDEF_DIM,
		p_ms_default, 50.0,
		p_range, BMIN_WIDTH, BMAX_WIDTH,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_WIDTHEDIT, IDC_WIDTHSPINNER, SPIN_AUTOSCALE,
		p_end,
	grid_grid, _T("grid"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_DB_GRID,
		p_default, GRIDHELP_DFLT_SPACING,
		p_ms_default, 10.0,
		p_range, BMIN_GRID, BMAX_GRID,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_GRID, IDC_GRIDSPINNER, SPIN_AUTOSCALE,
		p_end,
	grid_active_color, _T("activeColor"), TYPE_RADIOBTN_INDEX, 0, IDS_ACTIVE_COLOR,
		p_default, 		id_gray,
		p_range, 		id_gray, id_home_intensity,
		p_ui, 			TYPE_RADIO, 4, IDC_GRID_GRAY_COLOR, IDC_GRID_OBJECT_COLOR, IDC_GRID_HOME_COLOR, IDC_GRID_HOME_INTENSITY,
		p_end,
	grid_display_plane, _T("displayPlane"), TYPE_RADIOBTN_INDEX, 0, IDS_DISPLAY_PLANE,
		p_default, 		id_xy_plane,
		p_range, 		id_xy_plane, id_zx_plane,
		p_ui, 			TYPE_RADIO, 3, IDC_GRID_XY_PLANE, IDC_GRID_YZ_PLANE, IDC_GRID_ZX_PLANE,
		p_end,
	grid_mytm, _T(""), TYPE_MATRIX3, 0, 0,
		p_end,
	p_end
	);

int MaxCoord(Point3 p)
{
	int best = 0;
	if(fabs(p.x)<fabs(p.y))
		best = 1;
	switch(best)
	{
	case 0:
		if(fabs(p.x)<fabs(p.z))
			best = 2;
		break;
	case 1:
		if(fabs(p.y)<fabs(p.z))
			best = 2;
		break;
	}
	return best;
}

int MostOrthogonalPlane( Matrix3& tmConst, ViewExp *vpt)
{
	int plane =  GRID_PLANE_TOP;

	Matrix3 tm,tmv, itmv;
	vpt->GetAffineTM(tm );  
	tmv = tmConst*tm;   // CP to view transform.
	itmv = Inverse(tmv);
	Point3 viewz = itmv.GetRow(2);
	switch(MaxCoord(viewz))
	{
		bool dir;
	case 0:
		dir = viewz.x>0.0f;
		plane = dir?GRID_PLANE_LEFT:GRID_PLANE_RIGHT;
		break;
	case 1:
		dir = viewz.y>0.0f;
		plane = dir?GRID_PLANE_BACK:GRID_PLANE_FRONT;
		break;
	case 2:
		dir = viewz.z<0.0f;
		plane = dir?GRID_PLANE_BOTTOM:GRID_PLANE_TOP;
		break;
	}

	return plane;
}

void GridHelpObject::FixConstructionTM(Matrix3 &tm, ViewExp *vpt)
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	int plane = GRID_PLANE_TOP;

	if(vpt->GetGridType() < 0)
	{//JH 10/03/98 extending this to fix the construction plane to the most orthogonal plane
		//when the view is orthographic
		plane = pblock->GetInt(grid_display_plane); //Original behavior for all non-grid views
	}
	else
		plane = vpt->GetGridType();

	if(plane == GRID_PLANE_BOTTOM)
		tm.PreRotateX(PI);
	else if(plane == GRID_PLANE_RIGHT)
		tm.PreRotateY(HALFPI);
	else if(plane == GRID_PLANE_LEFT)
		tm.PreRotateY(-HALFPI);
	else if(plane == GRID_PLANE_FRONT)
		tm.PreRotateX(HALFPI);
	else if(plane == GRID_PLANE_BACK)
		tm.PreRotateX(-HALFPI);
		}

void GridHelpObject::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	// throw up all the appropriate auto-rollouts
	gridHelpObjDesc.BeginEditParams(ip, this, flags, prev);
}

void GridHelpObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next)
{
	__super::EndEditParams(ip, flags, next);
	gridHelpObjDesc.EndEditParams(ip, this, flags, next);
}

void GridHelpObject::UpdateMesh(TimeValue t) {
	if ( ivalid.InInterval(t) )
		return;
	// Start the validity interval at forever and whittle it down.
	ivalid = FOREVER;
	float length, width;
	pblock->GetValue(grid_length, t, length, ivalid);
	pblock->GetValue(grid_width, t, width, ivalid);
	}

ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, grid_length },
	{ TYPE_FLOAT, NULL, TRUE, 1, grid_width },
	{ TYPE_FLOAT, NULL, TRUE, 2, grid_grid },
	{ TYPE_FLOAT, NULL, TRUE, 3, -1 },
	{ TYPE_FLOAT, NULL, TRUE, 4, -1 },
	{ TYPE_FLOAT, NULL, TRUE, 5, -1 },
	{ TYPE_FLOAT, NULL, TRUE, 6, -1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, -1 },
	{ TYPE_FLOAT, NULL, TRUE, 8, -1 },
	{ TYPE_INT, NULL, TRUE, 9, -1 },
	{ TYPE_INT, NULL, TRUE,10, -1  },
	{ TYPE_FLOAT, NULL, TRUE,11, -1 } };

ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, grid_length },
	{ TYPE_FLOAT, NULL, TRUE, 1, grid_width },
	{ TYPE_FLOAT, NULL, TRUE, 2, grid_grid } };

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,12,0),
	ParamVersionDesc(descVer1,3,1)
	};
#define NUM_OLDVERSIONS	2

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH 3
#define CURRENT_VERSION	1

GridHelpObject::GridHelpObject() : ConstObject() 
	{
	pblock = NULL;
	InvalidateGrid();

	gridHelpObjDesc.MakeAutoParamBlocks(this);
	}

void GridHelpObject::SetGrid( TimeValue t, float len )
	{
	pblock->SetValue(grid_grid, t, len);
	}

void GridHelpObject::SetConstructionPlane(int p, BOOL notify)
	{
	BOOL notifiesEnabled = pblock->IsNotificationEnabled();
	pblock->EnableNotifications(FALSE);
	pblock->SetValue(grid_display_plane, 0, p);
	if (notify)
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	pblock->EnableNotifications(notifiesEnabled);
	}

int GridHelpObject::GetConstructionPlane(void)
	{
	return pblock->GetInt(grid_display_plane);
	}

float GridHelpObject::GetGrid( TimeValue t, Interval& valid )
	{
	float f = 0.;
	pblock->GetValue(grid_grid, t, f, valid);
	return f;
	}

class GridHelpObjCreateCallBack: public CreateMouseCallBack {
	GridHelpObject *ob;
	Point3 p0,p1;
	IPoint2 sp1, sp0;
	public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;
		void SetObj(GridHelpObject *obj) { ob = obj; }
	};

int GridHelpObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	Point3 d;

	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0:
				sp0 = m;
					p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				p1 = p0 + Point3(.01,.01,0.0);
				mat.SetTrans(float(.5)*(p0+p1));				
				break;
			case 1:
				sp1 = m;
					p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);

				//JH 7/22/03 Fix for DID 474833
				//When snapping a grid taking the midpoint of the inputs is strange
				//instead, use the z value of p0
				//mat.SetTrans(float(.5)*(p0+p1));
				mat.SetTrans(float(.5)*(p0+Point3(p1.x, p1.y, p0.z)));
				d = p1-p0;
			IParamBlock2 *pblock = ob->GetParamBlockByID(GRIDHELP_PARAMBLOCK_ID);
			pblock->SetValue(grid_width, 0, float(fabs(d.x)));
			pblock->SetValue(grid_length, 0, float(fabs(d.y)));
				if (msg==MOUSE_POINT) {
					if (Length(sp1-sp0) < 4) return CREATE_ABORT;
					else return CREATE_STOP;					
					}
				break;
			}
		}
	else
	if (msg == MOUSE_ABORT)
		return CREATE_ABORT;

	return TRUE;
	}

static GridHelpObjCreateCallBack gridHelpCreateCB;

CreateMouseCallBack* GridHelpObject::GetCreateMouseCallBack() {
	gridHelpCreateCB.SetObj(this);
	return(&gridHelpCreateCB);
	}

void GridHelpObject::GetBBox(TimeValue t,  Matrix3& tm, Box3& box) {	
	float length=0., width=0.;
	Point2 vert[2];
		
	pblock->GetValue(grid_length, t, length, FOREVER);
	pblock->GetValue(grid_width, t, width, FOREVER);

	vert[0].x = -width/float(2);
	vert[0].y = -length/float(2);
	vert[1].x = width/float(2);
	vert[1].y = length/float(2);

	box.Init();	
	box += tm * Point3( vert[0].x, vert[0].y, (float)0 );
	box += tm * Point3( vert[1].x, vert[0].y, (float)0 );
	box += tm * Point3( vert[0].x, vert[1].y, (float)0 );
	box += tm * Point3( vert[1].x, vert[1].y, (float)0 );
	box += tm * Point3( vert[0].x, vert[0].y, (float)0.1 );
	box += tm * Point3( vert[1].x, vert[0].y, (float)0.1 );
	box += tm * Point3( vert[0].x, vert[1].y, (float)0.1 );
	box += tm * Point3( vert[1].x, vert[1].y, (float)0.1 );
	}

void GridHelpObject::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}

	Matrix3 tm = pblock->GetMatrix3(grid_mytm);
	FixConstructionTM(tm, vpt);
	GetBBox(t,tm,box);
	}

void GridHelpObject::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
	{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}

	Matrix3 tm = pblock->GetMatrix3(grid_mytm) * (inode->GetObjectTM(t));
	FixConstructionTM(tm, vpt);
	GetBBox(t,tm,box);
	}

// Get the transform for this view
void GridHelpObject::GetConstructionTM( TimeValue t, INode* inode, ViewExp *vpt, Matrix3 &tm ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		tm.Zero();
		return;
	}
	tm = inode->GetObjectTM(t);
	FixConstructionTM(tm, vpt);
	}

// Get snap values
Point3 GridHelpObject::GetSnaps( TimeValue t ) {	
	float snap = GetGrid(t);
	return Point3(snap,snap,snap);
	}

void GridHelpObject::SetSnaps(TimeValue t, Point3 p)
{
	SetGrid(t, p.x);
}

Point3 GridHelpObject::GetExtents(TimeValue t)
{
	float x=0.f, y=0.f;
	pblock->GetValue(grid_length, t, x, FOREVER);
	pblock->GetValue(grid_width, t, y, FOREVER);
	return Point3(x, y, 0.0f);	
}
		
void GridHelpObject::SetExtents(TimeValue t, Point3 p)
{
	pblock->SetValue(grid_length, t, p.x);
	pblock->SetValue(grid_width, t, p.y);
}

int GridHelpObject::Select(TimeValue t, INode *inode, GraphicsWindow *gw, Material *mtl, HitRegion *hr, int abortOnHit ) {
	DWORD	savedLimits;
	Matrix3 tm;
	float width=0.f, w2, height=0.f, h2;
	pblock->GetValue(grid_length, t, height, FOREVER);
	pblock->GetValue(grid_width, t, width, FOREVER);
	if ( width==0 || height==0 )
		return 0;

	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	gw->setHitRegion(hr);
	gw->clearHitCode();
	gw->setMaterial(*mtl);

	w2 = width / (float)2;
	h2 = height / (float)2;
		
	Point3 pt[3];

	if(!inode->IsActiveGrid()) {

		pt[0] = Point3( -w2, -h2,(float)0.0);
		pt[1] = Point3( -w2, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );

		if((hr->type != POINT_RGN) && !hr->crossing) {	// window select needs *every* face to be enclosed
			if(gw->checkHitCode())
				gw->clearHitCode();
			else
				return FALSE;
			}
				
		if ( abortOnHit ) {
			if(gw->checkHitCode()) {
				gw->setRndLimits(savedLimits);
				return TRUE;
				}
			}

		pt[0] = Point3( w2, -h2,(float)0.0);
		pt[1] = Point3( w2, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
		if((hr->type != POINT_RGN) && !hr->crossing) {
			if(gw->checkHitCode())
				gw->clearHitCode();
			else
				return FALSE;
			}
			
		if ( abortOnHit ) {
			if(gw->checkHitCode()) {
				gw->setRndLimits(savedLimits);
				return TRUE;
				}
			}

		pt[0] = Point3( -w2, -h2,(float)0.0);
		pt[1] = Point3( w2, -h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
		if((hr->type != POINT_RGN) && !hr->crossing) {
			if(gw->checkHitCode())
				gw->clearHitCode();
			else
				return FALSE;
			}
			
		if ( abortOnHit ) {
			if(gw->checkHitCode()) {
				gw->setRndLimits(savedLimits);
				return TRUE;
				}
			}

		pt[0] = Point3( -w2, h2,(float)0.0);
		pt[1] = Point3( w2, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
		if((hr->type != POINT_RGN) && !hr->crossing) {
			if(gw->checkHitCode())
				gw->clearHitCode();
			else
				return FALSE;
			}
			
		if ( abortOnHit ) {
			if(gw->checkHitCode()) {
				gw->setRndLimits(savedLimits);
				return TRUE;
				}
			}

		pt[0] = Point3( -w2, (float)0, (float)0.0);
		pt[1] = Point3( w2, (float)0, (float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
		if((hr->type != POINT_RGN) && !hr->crossing) {
			if(gw->checkHitCode())
				gw->clearHitCode();
			else
				return FALSE;
			}
			
		if ( abortOnHit ) {
			if(gw->checkHitCode()) {
				gw->setRndLimits(savedLimits);
				return TRUE;
				}
			}

		pt[0] = Point3( (float)0, -h2,(float)0.0);
		pt[1] = Point3( (float)0, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
		if((hr->type != POINT_RGN) && !hr->crossing) {
			if(gw->checkHitCode())
				gw->clearHitCode();
			else
				return FALSE;
			}
			
		if ( abortOnHit ) {
			if(gw->checkHitCode()) {
				gw->setRndLimits(savedLimits);
				return TRUE;
				}
			}
		}
	else {
		float grid = GetGrid(t);
		int xSteps = (int)floor(w2 / grid);
		int ySteps = (int)floor(h2 / grid);
		float minX = (float)-xSteps * grid;
		float maxX = (float)xSteps * grid;
		float minY = (float)-ySteps * grid;
		float maxY = (float)ySteps * grid;
		float x,y;
		int ix;

		// Adjust steps for whole range
		xSteps *= 2;
		ySteps *= 2;

		// First, the vertical lines
		pt[0].y = minY;
		pt[0].z = (float)0;
		pt[1].y = maxY;
		pt[1].z = (float)0;

		for(ix=0,x=minX; ix<=xSteps; x+=grid,++ix) {
			pt[0].x = pt[1].x = x;
			gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
			if((hr->type != POINT_RGN) && !hr->crossing) {
				if(gw->checkHitCode())
					gw->clearHitCode();
				else
					return FALSE;
				}
			
			if ( abortOnHit ) {
				if(gw->checkHitCode()) {
					gw->setRndLimits(savedLimits);
					return TRUE;
					}
				}
   			}

		// Now, the horizontal lines
		pt[0].x = minX;
		pt[0].z = (float)0;
		pt[1].x = maxX;
		pt[1].z = (float)0;

		for(ix=0,y=minY; ix<=ySteps; y+=grid,++ix) {
			pt[0].y = pt[1].y = y;
			gw->polyline( 2, pt, NULL, NULL, FALSE, NULL );
		
			if((hr->type != POINT_RGN) && !hr->crossing) {
				if(gw->checkHitCode())
					gw->clearHitCode();
				else
					return FALSE;
				}
			
			if ( abortOnHit ) {
				if(gw->checkHitCode()) {
					gw->setRndLimits(savedLimits);
					return TRUE;
					}
				}
			}
		}

	if((hr->type != POINT_RGN) && !hr->crossing)
		return TRUE;
	return gw->checkHitCode();	
	}

// From BaseObject
int GridHelpObject::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	Matrix3 tm;	
	HitRegion hitRegion;
	GraphicsWindow *gw = vpt->getGW();	
	Material *mtl = gw->getMaterial();

	tm = pblock->GetMatrix3(grid_mytm) * (inode->GetObjectTM(t));
	FixConstructionTM(tm, vpt);
	UpdateMesh(t);
	gw->setTransform(tm);

	MakeHitRegion(hitRegion, type, crossing, 4, p);
	return Select(t, inode, gw, mtl, &hitRegion, flags & HIT_ABORTONHIT );
	}

static float GridCoord(float v, float g) {
	float r = (float)(int((fabs(v)+0.5f*g)/g))*g;	
	return v<0.0f ? -r : r;
	}			

void GridHelpObject::Snap(TimeValue t, INode* inode, SnapInfo *info, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	
	Matrix3 invPlane = Inverse(info->plane);

	// If this isn't the current grid object, forget it!
	if(!inode->IsActiveGrid())
		return;

	Matrix3 tm = inode->GetObjectTM(t);
	FixConstructionTM(tm, vpt);
	GraphicsWindow *gw = vpt->getGW();	

	UpdateMesh(t);
	gw->setTransform(tm);

	Point2 fp = Point2((float)p->x, (float)p->y);

	// Don't bother snapping unless the grid intersection priority is at least as important as what we have so far
	if(info->gIntPriority > 0 && info->gIntPriority <= info->priority) {
		// Find where it lies on the plane
		Point3 local = vpt->GetPointOnCP(*p);
		// Get the grid size
		float grid = GetGrid(t);
		// Snap it to the grid
		Point3 snapped = Point3(GridCoord(local.x,grid),GridCoord(local.y,grid),0.0f);
		// If constrained to the plane, make sure this point is in it!
		if(info->snapType == SNAP_2D || info->flags & SNAP_IN_PLANE) {
			Point3 test = snapped * tm * invPlane;
			if(fabs(test.z) > 0.0001)	// Is it in the plane (within reason)?
				goto testLines;
			}
		// Now find its screen location...
		Point2 screen2;
		IPoint3 pt3;
		gw->wTransPoint(&snapped,&pt3);
		screen2.x = (float)pt3.x;
		screen2.y = (float)pt3.y;
		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if(len <= info->strength) {
			// Is this priority better than the best so far?
			if(info->gIntPriority < info->priority) {
				info->priority = info->gIntPriority;
				info->bestWorld = snapped * tm;
				info->bestScreen = screen2;
				info->bestDist = len;
				}
			else
			if(len < info->bestDist) {
				info->priority = info->gIntPriority;
				info->bestWorld = snapped * tm;
				info->bestScreen = screen2;
				info->bestDist = len;
				}
			}
		}
	// Don't bother snapping unless the grid line priority is at least as important as what we have so far
	testLines:
	if(info->gLinePriority > 0 && info->gLinePriority <= info->priority) {
		// Find where it lies on the plane
		Point3 local = vpt->GetPointOnCP(*p);
		// Get the grid size
		float grid = GetGrid(t);
		// Snap it to the grid axes
		float xSnap = GridCoord(local.x,grid);
		float ySnap = GridCoord(local.y,grid);
		float xDist = (float)fabs(xSnap - local.x);
		float yDist = (float)fabs(ySnap - local.y);
		Point3 snapped;
		// Which one is closer?
		if(xDist < yDist)
			snapped = Point3(xSnap,local.y,0.0f);
		else
			snapped = Point3(local.x,ySnap,0.0f);
		// If constrained to the plane, make sure this point is in it!
		if(info->snapType == SNAP_2D || info->flags & SNAP_IN_PLANE) {
			Point3 test = snapped * tm * invPlane;
			if(fabs(test.z) > 0.0001)	// Is it in the plane (within reason)?
				return;
			}
		// Now find its screen location...
		Point2 screen2;
		IPoint3 pt3;
		gw->wTransPoint(&snapped,&pt3);
		screen2.x = (float)pt3.x;
		screen2.y = (float)pt3.y;
		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if(len <= info->strength) {
			// Is this priority better than the best so far?
			if(info->gLinePriority < info->priority) {
				info->priority = info->gLinePriority;
				info->bestWorld = snapped * tm;
				info->bestScreen = screen2;
				info->bestDist = len;
				}
			else
			if(len < info->bestDist) {
				info->priority = info->gLinePriority;
				info->bestWorld = snapped * tm;
				info->bestScreen = screen2;
				info->bestDist = len;
				}
			}
		}		
	}

int GridHelpObject::IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm)
	{
	if (r.dir.z==0.0f) return FALSE;

	at = -r.p.z/r.dir.z;
	norm = Point3(0,0,1);
	return TRUE;
	}

// This (viewport intensity) should be a globally accessible variable!
#define VPT_INTENS ((float)0.62)

// SECSTART is the fraction of the viewport intensity where the secondary lines start
#define SECSTART ((float)0.75)
static int dotted_es[2] = {GW_EDGE_INVIS, GW_EDGE_INVIS};

int GridHelpObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	Matrix3 tm;
	float width=0., w2, height=0., h2;
	pblock->GetValue(grid_length, t, height, FOREVER);
	pblock->GetValue(grid_width, t, width, FOREVER);
	if ( width==0 || height==0 )
		return 0;

	w2 = width / (float)2;
	h2 = height / (float)2;
		
	GraphicsWindow *gw = vpt->getGW();
	Material *mtl = gw->getMaterial();
	bool gw_dot_support = (gw->getRndLimits() & (GW_POLY_EDGES | GW_WIREFRAME))?true:false;
	int *es = (IsTransient() && gw_dot_support)?dotted_es:NULL;

	tm = pblock->GetMatrix3(grid_mytm) * (inode->GetObjectTM(t));
	FixConstructionTM(tm, vpt);
	UpdateMesh(t);		
	gw->setTransform(tm);

	Point3 pt[3];

	float grid = GetGrid(t);
	int xSteps = (int)floor(w2 / grid);
	int ySteps = (int)floor(h2 / grid);

	BOOL badGrid = (xSteps > 200 || ySteps > 200) ? TRUE : FALSE;

	if(!inode->IsActiveGrid() || badGrid) {

		//The line color only gets set correctly when the display is in wireframe mode. Objects that always display as lines, must set the color prior to draw.
		//The color is picked up from the current material color.
		gw->setColor(LINE_COLOR, mtl->Kd[0], mtl->Kd[1], mtl->Kd[2]);

		pt[0] = Point3( -w2, -h2,(float)0.0);
		pt[1] = Point3( -w2, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, es );

		pt[0] = Point3( w2, -h2,(float)0.0);
		pt[1] = Point3( w2, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, es );

		pt[0] = Point3( -w2, -h2,(float)0.0);
		pt[1] = Point3( w2, -h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, es );

		pt[0] = Point3( -w2, h2,(float)0.0);
		pt[1] = Point3( w2, h2,(float)0.0);
		gw->polyline( 2, pt, NULL, NULL, FALSE, es );

		if(badGrid) {
			pt[0] = Point3( -w2, -h2, (float)0.0);
			pt[1] = Point3( w2, h2, (float)0.0);
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );

			pt[0] = Point3( w2, -h2,(float)0.0);
			pt[1] = Point3( -w2, h2,(float)0.0);
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );
			}
		else {
			pt[0] = Point3( -w2, (float)0, (float)0.0);
			pt[1] = Point3( w2, (float)0, (float)0.0);
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );

			pt[0] = Point3( (float)0, -h2,(float)0.0);
			pt[1] = Point3( (float)0, h2,(float)0.0);
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );
			}
		}
	else {	// Active grid representation
		float minX = (float)-xSteps * grid;
		float maxX = (float)xSteps * grid;
		float minY = (float)-ySteps * grid;
		float maxY = (float)ySteps * grid;
		float x,y;
		int ix;
		int selected = inode->Selected();
		float priBrite = VPT_INTENS * SECSTART;

		// Adjust steps for whole range
		xSteps *= 2;
		ySteps *= 2;

		// First, the vertical lines
		pt[0].y = minY;
		pt[0].z = (float)0;
		pt[1].y = maxY;
		pt[1].z = (float)0;

		Point3 dspClr1, dspClr2;
		DWORD rgb;
		int gridColor = pblock->GetInt(grid_active_color);
		switch(gridColor) {
		case id_gray:
			dspClr2 = Point3(0,0,0);
			dspClr1 = Point3(priBrite, priBrite, priBrite);
			break;
		case id_object_color:
			rgb = inode->GetWireColor();
			dspClr2 = Point3(GetRValue(rgb)/255.0f, GetGValue(rgb)/255.0f, GetBValue(rgb)/255.0f);
			dspClr1 = (Point3(1,1,1) + dspClr2) / 2.0f;
			break;
		case id_home_color:
			dspClr2 = GetUIColor(COLOR_GRID);
			dspClr1 = (Point3(1,1,1) + dspClr2) / 2.0f;
			break;
		case id_home_intensity:
			dspClr1 = GetUIColor(COLOR_GRID_INTENS);
			if(dspClr1.x < 0.0f) {	// means "invert"
				dspClr1 = Point3(1,1,1) + dspClr1;
				dspClr2 = Point3(0.8f,0.8f,0.8f);
			}
			else
				dspClr2 = Point3(0,0,0);
			break;
		}

		if(!selected && !inode->IsFrozen() && !inode->Dependent())
			gw->setColor( LINE_COLOR, dspClr1 );

		for(ix=0,x=minX; ix<=xSteps; x+=grid,++ix) {
			pt[0].x = pt[1].x = x;
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );
   			}

		// Draw origin line if not selected
		if(!selected && !inode->IsFrozen() && !inode->Dependent()) {
			gw->setColor( LINE_COLOR, dspClr2 );
			pt[0].x = pt[1].x = 0.0f;
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );
			}

		// Now, the horizontal lines
		pt[0].x = minX;
		pt[0].z = 0.0f;
		pt[1].x = maxX;
		pt[1].z = 0.0f;

		if(!selected && !inode->IsFrozen() && !inode->Dependent())
			gw->setColor( LINE_COLOR, dspClr1 );

		for(ix=0,y=minY; ix<=ySteps; y+=grid,++ix) {
			pt[0].y = pt[1].y = y;
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );
			}

		// Draw origin line if not selected
		if(!selected && !inode->IsFrozen() && !inode->Dependent()) {
			gw->setColor( LINE_COLOR, dspClr2 );
			pt[0].y = pt[1].y = 0.0f;
			gw->polyline( 2, pt, NULL, NULL, FALSE, es );
			}

		// Inform the viewport about the smallest grid scale
		vpt->SetGridSize(grid);
		}

	return(0);
	}

//
// Reference Management:
//

// This is only called if the object MAKES references to other things.
RefResult GridHelpObject::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
     PartID& partID, RefMessage message, BOOL propagate ) 
    {
	switch (message) {
		case REFMSG_CHANGE:
			InvalidateGrid();
			break;
		}
	return(REF_SUCCEED);
	}

ObjectState GridHelpObject::Eval(TimeValue time){
	return ObjectState(this);
	}

Interval GridHelpObject::ObjectValidity(TimeValue time) {
	UpdateMesh(time);
	return ivalid;	
	}

int GridHelpObject::CanConvertToType(Class_ID obtype) {
	return 0;
	}

Object* GridHelpObject::ConvertToType(TimeValue t, Class_ID obtype) {
	return NULL;
	}

RefTargetHandle GridHelpObject::Clone(RemapDir& remap) {
	GridHelpObject* newob = new GridHelpObject();	
	newob->ReplaceReference(0,remap.CloneRef(pblock));
	newob->InvalidateGrid();
	BaseClone(this, newob, remap);
	return(newob);
	}

#define TM_CHUNK	0x2100
#define COLOR_CHUNK	0x2110
#define PLANE_CHUNK	0x2120

// IO
bool GridHelpObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock, PBLOCK_REF_NO, descVer1, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult GridHelpObject::Save(ISave *isave) {

	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		int gridColor = pblock->GetInt(grid_active_color);
		int constPlane = pblock->GetInt(grid_display_plane);
		Matrix3 myTM = pblock->GetMatrix3(grid_mytm);
	ULONG nb;
	isave->BeginChunk(TM_CHUNK);
	isave->Write(&myTM,sizeof(Matrix3), &nb);
	isave->EndChunk();

	isave->BeginChunk(COLOR_CHUNK);
	isave->Write(&gridColor, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(PLANE_CHUNK);
	isave->Write(&constPlane, sizeof(int), &nb);
	isave->EndChunk();
	}
	return IO_OK;
	}

class GridHelpObjectPLCB : public PostLoadCallback
{
	GridHelpObject* theGrid;
	int gridColor;
	int constPlane;
	Matrix3 myTM;
public:
	GridHelpObjectPLCB(GridHelpObject* theGrid, int gridColor, int constPlane, Matrix3 &myTM) : 
		theGrid(theGrid), gridColor(gridColor), constPlane(constPlane), myTM(myTM) {}
	virtual void proc(ILoad *iload) override
	{
		IParamBlock2* pb = theGrid->GetParamBlock(0);
		if (pb)
		{
			pb->SetValue(grid_active_color, 0, gridColor);
			pb->SetValue(grid_display_plane, 0, constPlane);
			pb->SetValue(grid_mytm, 0, myTM);
		}
	}
};

IOResult  GridHelpObject::Load(ILoad *iload) {
	ULONG nb;
	IOResult res;
	int gridColor = id_gray;
	int constPlane = id_xy_plane;
	Matrix3 myTM(TRUE);
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &grid_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	bool registerMigrationCallback = false;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case TM_CHUNK:
				res = iload->Read(&myTM,sizeof(Matrix3), &nb);
			registerMigrationCallback = true;
				break;
			case COLOR_CHUNK:
				res = iload->Read(&gridColor, sizeof(int), &nb);
			registerMigrationCallback = true;
				break;
			case PLANE_CHUNK:
				res = iload->Read(&constPlane, sizeof(int), &nb);
			registerMigrationCallback = true;
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	if (registerMigrationCallback)
	{
		GridHelpObjectPLCB* ghplcb = new GridHelpObjectPLCB(this, gridColor, constPlane, myTM);
		iload->RegisterPostLoadCallback(ghplcb);
		}
	return IO_OK;
	}

Animatable* GridHelpObject::SubAnim(int i) {
	return pblock;
	}

TSTR GridHelpObject::SubAnimName(int i) {
	return TSTR(GetString(IDS_DB_PARAMETERS));
	}

