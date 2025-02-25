/**********************************************************************
 *<
	FILE: pthelp.cpp

	DESCRIPTION:  A point helper implementation

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#include "helpers.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "istdplug.h"

 //------------------------------------------------------

 // in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

#define AXIS_LENGTH 20.0f
#define ZFACT (float).005;

void DrawAxis(ViewExp *vpt, const Matrix3 &tm, float length, BOOL screenSize);

// forward declarations
class PointHelpObjCreateCallBack;
class PointHelperPostLoadCallback;

class PointHelpObject : public HelperObject {
	friend class PointHelpObjCreateCallBack;
	friend class PointHelperPostLoadCallback;
private:
	IParamBlock2 *pblock2;

	// Snap suspension flag (TRUE during creation only)
	BOOL suspendSnap;

	// Old params... these are for loading old files only. Params are now stored in pb2.
	BOOL showAxis;
	float axisLength;

	// For use by display system
	int extDispFlags;

	static IObjParam *ip;
	static PointHelpObject *editOb;

public:

	//  inherited virtual methods for Reference-management
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

	PointHelpObject();

	// From BaseObject
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	void SetExtendedDisplay(int flags);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CreateMouseCallBack* GetCreateMouseCallBack();
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);
	const TCHAR *GetObjectName() { return GetString(IDS_POINT_HELPER_NAME); }

	// From Object
	ObjectState Eval(TimeValue time);
	void InitNodeName(TSTR& s) { s = GetString(IDS_DB_POINT); }
	int CanConvertToType(Class_ID obtype) { return FALSE; }
	Object* ConvertToType(TimeValue t, Class_ID obtype) { assert(0); return NULL; }
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box);
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box);
	int DoOwnSelectHilite() { return 1; }
	Interval ObjectValidity(TimeValue t);
	int UsesWireColor() { return TRUE; }

	// Animatable methods
	void DeleteThis() { delete this; }
	Class_ID ClassID() { return Class_ID(POINTHELP_CLASS_ID, 0); }
	void GetClassName(TSTR& s) { s = TSTR(GetString(IDS_DB_POINTHELPER_CLASS)); }
	int NumSubs() { return 1; }
	Animatable* SubAnim(int i) { return (i == 0) ? pblock2 : nullptr; }
	TSTR SubAnimName(int i) { return _T("Parameters"); }
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i) { return (i == 0) ? pblock2 : nullptr; }
	IParamBlock2* GetParamBlockByID(short id) { return (id == pointobj_params) ? pblock2 : nullptr; }

	// From ref
	RefTargetHandle Clone(RemapDir& remap);
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return (i == 0) ? pblock2 : nullptr; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { if (i == 0) pblock2 = (IParamBlock2*)rtarg; }
public:

private:
	// local (non-virtual) methods
	void InvalidateUI();
	void UpdateParamblockFromVars();
	int DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt);
};

// class variable for point class.
IObjParam *PointHelpObject::ip = NULL;
PointHelpObject *PointHelpObject::editOb = NULL;

class PointHelpObjClassDesc :public ClassDesc2 {
public:
	int 				IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new PointHelpObject; }
	const TCHAR *	ClassName() { return GetString(IDS_DB_POINT_CLASS); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID			ClassID() { return Class_ID(POINTHELP_CLASS_ID, 0); }
	const TCHAR* 	Category() { return _T(""); }
	const TCHAR*	InternalName() { return _T("PointHelperObj"); }
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static PointHelpObjClassDesc pointHelpObjDesc;

ClassDesc* GetPointHelpDesc() { return &pointHelpObjDesc; }

#define PBLOCK_REF_NO	 0

// The following two enums are transfered to the istdplug.h by AG: 01/20/2002 
// in order to access the parameters for use in Spline IK Control modifier
// and the Spline IK Solver

// block IDs
//enum { pointobj_params, };

// pointobj_params IDs

// enum { 
//	pointobj_size, pointobj_centermarker, pointobj_axistripod, 
//	pointobj_cross, pointobj_box, pointobj_screensize, pointobj_drawontop };

// per instance block
static ParamBlockDesc2 pointobj_param_blk(

	pointobj_params, _T("PointObjectParameters"), 0, &pointHelpObjDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,

	//rollout
	IDD_NEW_POINTPARAM, IDS_POINT_PARAMS, 0, 0, NULL,

	// params
	pointobj_size, _T("size"), TYPE_WORLD, P_ANIMATABLE, IDS_POINT_SIZE,
	p_default, 20.0,
	p_ms_default, 20.0,
	p_range, 0.0f, float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_POINT_SIZE, IDC_POINT_SIZESPIN, SPIN_AUTOSCALE,
	p_end,

	pointobj_centermarker, _T("centermarker"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_CENTERMARKER,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_POINT_MARKER,
	p_end,

	pointobj_axistripod, _T("axistripod"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_AXISTRIPOD,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_POINT_AXIS,
	p_end,

	pointobj_cross, _T("cross"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_CROSS,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_POINT_CROSS,
	p_end,

	pointobj_box, _T("box"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_BOX,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_POINT_BOX,
	p_end,

	pointobj_screensize, _T("constantscreensize"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_SCREENSIZE,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_POINT_SCREENSIZE,
	p_end,

	pointobj_drawontop, _T("drawontop"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_DRAWONTOP,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_POINT_DRAWONTOP,
	p_end,

	p_end
	);

void PointHelpObject::BeginEditParams(
	IObjParam *ip, ULONG flags, Animatable *prev)
{
	this->ip = ip;
	editOb = this;
	pointHelpObjDesc.BeginEditParams(ip, this, flags, prev);
}

void PointHelpObject::EndEditParams(
	IObjParam *ip, ULONG flags, Animatable *next)
{
	editOb = NULL;
	this->ip = NULL;
	pointHelpObjDesc.EndEditParams(ip, this, flags, next);
	ClearAFlag(A_OBJ_CREATING);
}

PointHelpObject::PointHelpObject()
{
	pblock2 = NULL;
	pointHelpObjDesc.MakeAutoParamBlocks(this);
	showAxis = TRUE;
	axisLength = 10.0f;
	suspendSnap = FALSE;
	SetAFlag(A_OBJ_CREATING);
}

class PointHelpObjCreateCallBack : public CreateMouseCallBack {
	PointHelpObject *ob;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(PointHelpObject *obj) { ob = obj; }
};

int PointHelpObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) {

	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch (point) {
		case 0: {
			// Find the node and plug in the wire color
			ULONG handle;
			ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
			INode *node;
			node = GetCOREInterface()->GetINodeByHandle(handle);
			if (node) {
				Point3 color = GetUIColor(COLOR_POINT_OBJ);
				node->SetWireColor(RGB(color.x*255.0f, color.y*255.0f, color.z*255.0f));
			}

			ob->suspendSnap = TRUE;
			mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
			break;
		}

		case 1:
			mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
			if (msg == MOUSE_POINT) {
				ob->suspendSnap = FALSE;
				return 0;
			}
			break;
		}
	}
	else
		if (msg == MOUSE_ABORT) {
			return CREATE_ABORT;
		}
	return 1;
}

static PointHelpObjCreateCallBack pointHelpCreateCB;

CreateMouseCallBack* PointHelpObject::GetCreateMouseCallBack() {
	pointHelpCreateCB.SetObj(this);
	return(&pointHelpCreateCB);
}

void PointHelpObject::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

void PointHelpObject::GetLocalBoundBox(
	TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{

	if (!vpt || !vpt->IsAlive())
	{
		box.Init();
		return;
	}

	Matrix3 tm = inode->GetObjectTM(t);

	float size = 0.f;
	int screenSize = 0;
	pblock2->GetValue(pointobj_size, t, size, FOREVER);
	pblock2->GetValue(pointobj_screensize, t, screenSize, FOREVER);

	float zoom = 1.0f;
	if (screenSize) {
		zoom = vpt->GetScreenScaleFactor(tm.GetTrans())*ZFACT;
	}
	if (zoom == 0.0f) zoom = 1.0f;

	size *= zoom;
	box = Box3(Point3(0, 0, 0), Point3(0, 0, 0));
	box += Point3(size*0.5f, 0.0f, 0.0f);
	box += Point3(0.0f, size*0.5f, 0.0f);
	box += Point3(0.0f, 0.0f, size*0.5f);
	box += Point3(-size*0.5f, 0.0f, 0.0f);
	box += Point3(0.0f, -size*0.5f, 0.0f);
	box += Point3(0.0f, 0.0f, -size*0.5f);

	//JH 6/18/03
	//This looks odd but I'm being conservative an only excluding it for
	//the case I care about which is when computing group boxes
	if (!(extDispFlags & EXT_DISP_GROUP_EXT))
		box.EnlargeBy(10.0f / zoom);
}

void PointHelpObject::GetWorldBoundBox(
	TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	if (!vpt || !vpt->IsAlive())
	{
		box.Init();
		return;
	}

	Matrix3 tm;
	tm = inode->GetObjectTM(t);
	Box3 lbox;

	GetLocalBoundBox(t, inode, vpt, lbox);
	box = Box3(tm.GetTrans(), tm.GetTrans());
	for (int i = 0; i < 8; i++) {
		box += lbox * tm;
	}
}

void PointHelpObject::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}

	if (suspendSnap)
		return;

	Matrix3 tm = inode->GetObjectTM(t);
	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(tm);

	Matrix3 invPlane = Inverse(snap->plane);

	// Make sure the vertex priority is active and at least as important as the best snap so far
	if (snap->vertPriority > 0 && snap->vertPriority <= snap->priority) {
		Point2 fp = Point2((float)p->x, (float)p->y);
		Point2 screen2;
		IPoint3 pt3;

		Point3 thePoint(0, 0, 0);
		// If constrained to the plane, make sure this point is in it!
		if (snap->snapType == SNAP_2D || snap->flags & SNAP_IN_PLANE) {
			Point3 test = thePoint * tm * invPlane;
			if (fabs(test.z) > 0.0001)	// Is it in the plane (within reason)?
				return;
		}
		gw->wTransPoint(&thePoint, &pt3);
		screen2.x = (float)pt3.x;
		screen2.y = (float)pt3.y;

		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if (len <= snap->strength) {
			// Is this priority better than the best so far?
			if (snap->vertPriority < snap->priority) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
			}
			else
				if (len < snap->bestDist) {
					snap->priority = snap->vertPriority;
					snap->bestWorld = thePoint * tm;
					snap->bestScreen = screen2;
					snap->bestDist = len;
				}
		}
	}
}

int PointHelpObject::DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	float size;
	int centerMarker, axisTripod, cross, box, screenSize, drawOnTop;

	Color color(inode->GetWireColor());

	Interval ivalid = FOREVER;
	pblock2->GetValue(pointobj_size, t, size, ivalid);
	pblock2->GetValue(pointobj_centermarker, t, centerMarker, ivalid);
	pblock2->GetValue(pointobj_axistripod, t, axisTripod, ivalid);
	pblock2->GetValue(pointobj_cross, t, cross, ivalid);
	pblock2->GetValue(pointobj_box, t, box, ivalid);
	pblock2->GetValue(pointobj_screensize, t, screenSize, ivalid);
	pblock2->GetValue(pointobj_drawontop, t, drawOnTop, ivalid);

	Matrix3 tm(1);
	Point3 pt(0, 0, 0);
	Point3 pts[5];

	vpt->getGW()->setTransform(tm);
	tm = inode->GetObjectTM(t);

	int limits = vpt->getGW()->getRndLimits();
	if (drawOnTop) vpt->getGW()->setRndLimits(limits & ~GW_Z_BUFFER);

	if (inode->Selected()) {
		vpt->getGW()->setColor(TEXT_COLOR, GetUIColor(COLOR_SELECTION));
		vpt->getGW()->setColor(LINE_COLOR, GetUIColor(COLOR_SELECTION));
	}
	else if (!inode->IsFrozen() && !inode->Dependent()) {
		vpt->getGW()->setColor(TEXT_COLOR, color);
		vpt->getGW()->setColor(LINE_COLOR, color);
	}

	if (axisTripod) {
		DrawAxis(vpt, tm, size, screenSize);
	}

	size *= 0.5f;

	float zoom = vpt->GetScreenScaleFactor(tm.GetTrans())*ZFACT;
	if (screenSize) {
		tm.Scale(Point3(zoom, zoom, zoom));
	}

	vpt->getGW()->setTransform(tm);

	if (!inode->IsFrozen() && !inode->Dependent() && !inode->Selected()) {
		vpt->getGW()->setColor(LINE_COLOR, color);
	}

	if (centerMarker) {
		vpt->getGW()->marker(&pt, X_MRKR);
	}

	if (cross) {
		// X
		pts[0] = Point3(-size, 0.0f, 0.0f); pts[1] = Point3(size, 0.0f, 0.0f);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		// Y
		pts[0] = Point3(0.0f, -size, 0.0f); pts[1] = Point3(0.0f, size, 0.0f);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		// Z
		pts[0] = Point3(0.0f, 0.0f, -size); pts[1] = Point3(0.0f, 0.0f, size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
	}

	if (box) {

		// Make the box half the size
		size = size * 0.5f;

		// Bottom
		pts[0] = Point3(-size, -size, -size);
		pts[1] = Point3(-size, size, -size);
		pts[2] = Point3(size, size, -size);
		pts[3] = Point3(size, -size, -size);
		vpt->getGW()->polyline(4, pts, NULL, NULL, TRUE, NULL);

		// Top
		pts[0] = Point3(-size, -size, size);
		pts[1] = Point3(-size, size, size);
		pts[2] = Point3(size, size, size);
		pts[3] = Point3(size, -size, size);
		vpt->getGW()->polyline(4, pts, NULL, NULL, TRUE, NULL);

		// Sides
		pts[0] = Point3(-size, -size, -size);
		pts[1] = Point3(-size, -size, size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		pts[0] = Point3(-size, size, -size);
		pts[1] = Point3(-size, size, size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		pts[0] = Point3(size, size, -size);
		pts[1] = Point3(size, size, size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		pts[0] = Point3(size, -size, -size);
		pts[1] = Point3(size, -size, size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
	}

	vpt->getGW()->setRndLimits(limits);

	return 1;
}

int PointHelpObject::HitTest(
	TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	Matrix3 tm(1);
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0, 0, 0);

	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(tm);
	Material *mtl = gw->getMaterial();

	tm = inode->GetObjectTM(t);
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	DrawAndHit(t, inode, vpt);

	gw->setRndLimits(savedLimits);
	return gw->checkHitCode();
}

int PointHelpObject::Display(
	TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	DrawAndHit(t, inode, vpt);
	return(0);
}


//
// Reference Management:
//

// This is only called if the object MAKES references to other things.
RefResult PointHelpObject::NotifyRefChanged(
	const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate)
{
	switch (message) {
	case REFMSG_CHANGE:
		if (editOb == this) InvalidateUI();
		break;
	}
	return(REF_SUCCEED);
}

void PointHelpObject::InvalidateUI()
{
	pointobj_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

Interval PointHelpObject::ObjectValidity(TimeValue t)
{
	float size;
	int centerMarker, axisTripod, cross, box, screenSize, drawOnTop;

	Interval ivalid = FOREVER;
	pblock2->GetValue(pointobj_size, t, size, ivalid);
	pblock2->GetValue(pointobj_centermarker, t, centerMarker, ivalid);
	pblock2->GetValue(pointobj_axistripod, t, axisTripod, ivalid);
	pblock2->GetValue(pointobj_cross, t, cross, ivalid);
	pblock2->GetValue(pointobj_box, t, box, ivalid);
	pblock2->GetValue(pointobj_screensize, t, screenSize, ivalid);
	pblock2->GetValue(pointobj_drawontop, t, drawOnTop, ivalid);

	return ivalid;
}

ObjectState PointHelpObject::Eval(TimeValue t)
{
	return ObjectState(this);
}

RefTargetHandle PointHelpObject::Clone(RemapDir& remap)
{
	PointHelpObject* newob = new PointHelpObject();
	newob->showAxis = showAxis;
	newob->axisLength = axisLength;
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	BaseClone(this, newob, remap);
	return(newob);
}

void PointHelpObject::UpdateParamblockFromVars()
{
	AnimateSuspend as;

	pblock2->SetValue(pointobj_size, TimeValue(0), axisLength);
	pblock2->SetValue(pointobj_centermarker, TimeValue(0), TRUE);
	pblock2->SetValue(pointobj_axistripod, TimeValue(0), showAxis);
	pblock2->SetValue(pointobj_cross, TimeValue(0), FALSE);
	pblock2->SetValue(pointobj_box, TimeValue(0), FALSE);
	pblock2->SetValue(pointobj_screensize, TimeValue(0), TRUE);
}

class PointHelperPostLoadCallback : public PostLoadCallback {
public:
	PointHelpObject *pobj;

	PointHelperPostLoadCallback(PointHelpObject *p) { pobj = p; }
	void proc(ILoad *iload) {
		pobj->UpdateParamblockFromVars();
	}
};

#define SHOW_AXIS_CHUNK				0x0100
#define AXIS_LENGTH_CHUNK			0x0110
#define POINT_HELPER_R4_CHUNKID	0x0120 // new version of point helper for R4 (updated to use PB2)

IOResult PointHelpObject::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res = IO_OK;
	BOOL oldVersion = TRUE;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case SHOW_AXIS_CHUNK:
			res = iload->Read(&showAxis, sizeof(showAxis), &nb);
			break;

		case AXIS_LENGTH_CHUNK:
			res = iload->Read(&axisLength, sizeof(axisLength), &nb);
			break;

		case POINT_HELPER_R4_CHUNKID:
			oldVersion = FALSE;
			break;
		}

		res = iload->CloseChunk();
		if (res != IO_OK)  return res;
	}

	if (oldVersion) {
		iload->RegisterPostLoadCallback(new PointHelperPostLoadCallback(this));
	}

	return IO_OK;
}

IOResult PointHelpObject::Save(ISave *isave)
{
	isave->BeginChunk(POINT_HELPER_R4_CHUNKID);
	isave->EndChunk();

	return IO_OK;
}

/*--------------------------------------------------------------------*/
// Stole this from scene.cpp
// Probably couldn't hurt to make an API...

void Text(ViewExp *vpt, const TCHAR *str, Point3 &pt)
{
	vpt->getGW()->text(&pt, str);
}

static void DrawAnAxis(ViewExp *vpt, Point3 axis)
{
	Point3 v1, v2, v[3];
	v1 = axis * (float)0.9;
	if (axis.x != 0.0 || axis.y != 0.0) {
		v2 = Point3(axis.y, -axis.x, axis.z) * (float)0.1;
	}
	else {
		v2 = Point3(axis.x, axis.z, -axis.y) * (float)0.1;
	}

	v[0] = Point3(0.0, 0.0, 0.0);
	v[1] = axis;
	vpt->getGW()->polyline(2, v, NULL, NULL, FALSE, NULL);
	v[0] = axis;
	v[1] = v1 + v2;
	vpt->getGW()->polyline(2, v, NULL, NULL, FALSE, NULL);
	v[0] = axis;
	v[1] = v1 - v2;
	vpt->getGW()->polyline(2, v, NULL, NULL, FALSE, NULL);
}

void DrawAxis(ViewExp *vpt, const Matrix3 &tm, float length, BOOL screenSize)
{
	Matrix3 tmn = tm;
	float zoom;

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;

	if (screenSize) {
		tmn.Scale(Point3(zoom, zoom, zoom));
	}
	vpt->getGW()->setTransform(tmn);

	Text(vpt, _T("x"), Point3(length, 0.0f, 0.0f));
	DrawAnAxis(vpt, Point3(length, 0.0f, 0.0f));

	Text(vpt, _T("y"), Point3(0.0f, length, 0.0f));
	DrawAnAxis(vpt, Point3(0.0f, length, 0.0f));

	Text(vpt, _T("z"), Point3(0.0f, 0.0f, length));
	DrawAnAxis(vpt, Point3(0.0f, 0.0f, length));
}
