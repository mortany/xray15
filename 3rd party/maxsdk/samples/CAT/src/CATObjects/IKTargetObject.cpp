//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "../CATControls/CatPlugins.h"
#include "../CATControls/CATNodeControl.h"
#include "../CATControls/ICATParent.h"
#include "IKTargetObject.h"
#include "iparamm2.h"

class IKTargetObjClassDesc: public ClassDesc2 {
	public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) {UNREFERENCED_PARAMETER(loading);  return new IKTargetObject; }
	const TCHAR *	ClassName() { return GetString(IDS_CL_IKTARGET_OBJECT); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return IKTARGET_OBJECT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY);  }
	void			ResetClassParams(BOOL fileReset) {UNREFERENCED_PARAMETER(fileReset); /*if(fileReset) resetPointParams();*/ }
	const TCHAR*	InternalName() {return _T("IKTargetObj");}
	HINSTANCE		HInstance() {return hInstance;}			// returns owning module handle
	};

static IKTargetObjClassDesc handleObjDesc;
ClassDesc2* GetIKTargetObjDesc() { return &handleObjDesc; }

// class variable for point class.
IObjParam *IKTargetObject::ip = NULL;
IKTargetObject *IKTargetObject::editOb = NULL;

#define PBLOCK_REF_NO	 0

class IKObjParamDlgCallBack : public  ParamMap2UserDlgProc {
	IKTargetObject* ikobj;
public:

	IKObjParamDlgCallBack() : ikobj(NULL) {}
	void InitControls(HWND hDlg, IParamMap2 *map)
	{
		ikobj = (IKTargetObject*)map->GetParamBlock()->GetOwner();
		DbgAssert(ikobj);

		if(ikobj->pblock2->GetInt(iktarget_cross))
			 SET_CHECKED(hDlg, IDC_RADIO_CROSS, TRUE);
		else SET_CHECKED(hDlg, IDC_RADIO_PLATFORM, TRUE);

		SetFocus(GetDlgItem(hDlg,IDC_RADIO_CROSS));
	}
	void ReleaseControls(){
	}
	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		switch(msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, map);
			break;
		case WM_DESTROY:
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)){
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
					case IDC_RADIO_PLATFORM:
						ikobj->SetCross(!IS_CHECKED(hWnd, IDC_RADIO_PLATFORM));
						SET_CHECKED(hWnd, IDC_RADIO_CROSS, !IS_CHECKED(hWnd, IDC_RADIO_PLATFORM));
						ikobj->Update();
						break;
					case IDC_RADIO_CROSS:
						ikobj->SetCross(IS_CHECKED(hWnd, IDC_RADIO_CROSS));
						SET_CHECKED(hWnd, IDC_RADIO_PLATFORM, !IS_CHECKED(hWnd, IDC_RADIO_CROSS));
						ikobj->Update();
						break;
				}
				break;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	virtual void DeleteThis() { }//delete this; }
};

static IKObjParamDlgCallBack IKObjParamCallBack;

static ParamBlockDesc2 iktarget_param_blk(
	iktarget_params, _T("IKTarget Setup"),  0, &handleObjDesc,
	P_AUTO_CONSTRUCT+P_AUTO_UI, PBLOCK_REF_NO,
	IDD_IKTARGET_OBJECT, IDS_CL_IKTARGET_OBJECT, 0, 0, &IKObjParamCallBack,

	// params
	iktarget_controller,_T("controller"),	TYPE_REFTARG,		P_NO_REF,		0,
		p_end,

	iktarget_cross,		_T("cross"),		TYPE_BOOL,		0,		0,
		p_default, 		FALSE,
		p_end,

	iktarget_length,	_T("Length"),		TYPE_FLOAT,		0,		IDS_LENGTH,
		p_default, 		20.0,
		p_range, 		0.0f, float(1.0E30),
		p_ui, 			TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_EDIT_LENGTH, IDC_SPIN_LENGTH, SPIN_AUTOSCALE,
		p_end,

	iktarget_width,		_T("Width"),		TYPE_FLOAT,		0,		IDS_WIDTH,
		p_default, 		20.0,
		p_range, 		0.0f, float(1.0E30),
		p_ui, 			TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_EDIT_WIDTH, IDC_SPIN_WIDTH, SPIN_AUTOSCALE,
		p_end,
	iktarget_catunits, 	_T("CATUnits"), 		TYPE_FLOAT, 	0,			NULL,
		p_default,		1.0f,
		p_end,
	p_end
	);

void IKTargetObject::BeginEditParams(
		IObjParam *ip, ULONG flags,Animatable *prev)
	{
	this->ip = ip;
	editOb   = this;
	handleObjDesc.BeginEditParams(ip, this, flags, prev);

	}

void IKTargetObject::EndEditParams(
		IObjParam *ip, ULONG flags,Animatable *next)
	{
	editOb   = NULL;
	this->ip = NULL;
	handleObjDesc.EndEditParams(ip, this, flags, next);
	ClearAFlag(A_OBJ_CREATING);
	}

IKTargetObject::IKTargetObject()
	{
		flags				= NULL;
		pblock2				= NULL;
		node				= NULL;
		suspendSnap			= FALSE;
		dwFileSaveVersion	= 0;
		SetAFlag(A_OBJ_CREATING);
		handleObjDesc.MakeAutoParamBlocks(this);
	}

IKTargetObject::~IKTargetObject()
	{
	DeleteAllRefsFromMe();
	}

IParamArray *IKTargetObject::GetParamBlock()
	{
	return (IParamArray*)pblock2;
	}

int IKTargetObject::GetParamBlockIndex(int id)
	{
	if (pblock2 && id>=0 && id<pblock2->NumParams()) return id;
	else return -1;
	}

class IKTargetObjCreateCallBack: public CreateMouseCallBack {
	IKTargetObject *ob;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(IKTargetObject *obj) { ob = obj; }
	};

int IKTargetObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {

	UNREFERENCED_PARAMETER(flags);
	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0: {

				// Find the node and plug in the wire color
				ob->node = FindReferencingClass<INode>(ob);
				if (ob->node) {
					Point3 color = GetUIColor(COLOR_POINT_OBJ);
					ob->node->SetWireColor(RGB(color.x*255.0f, color.y*255.0f, color.z*255.0f));
					}

				ob->suspendSnap = TRUE;
					mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_3D));
				break;
				}

			case 1:
					mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_3D));
				if (msg==MOUSE_POINT) {
					ob->suspendSnap = FALSE;
					return 0;
					}
				break;
			}
	} else
	if (msg == MOUSE_ABORT) {
		return CREATE_ABORT;
		}
	return 1;
	}

static IKTargetObjCreateCallBack handleCreateCB;

CreateMouseCallBack* IKTargetObject::GetCreateMouseCallBack() {
	handleCreateCB.SetObj(this);
	return(&handleCreateCB);
	}

void IKTargetObject::SetExtendedDisplay(int flags)
	{
	extDispFlags = flags;
	}

void IKTargetObject::GetLocalBoundBox(
		TimeValue t, INode* inode, ViewExp*, Box3& box )
	{
	Matrix3 tm = inode->GetObjectTM(t);

	float dLength, dWidth;
//	int screenSize;
	Interval iv = FOREVER;
	pblock2->GetValue(iktarget_length, t, dLength, iv);
	iv = FOREVER;
	pblock2->GetValue(iktarget_width, t, dWidth, iv);

	float dCATUnits = GetCATUnits();
	int lengthaxis = (flags&CATOBJECTFLAG_LENGTHAXIS_X) ? X : Z;

	dLength *= dCATUnits/2.0f;
	dWidth *= dCATUnits/2.0f;

	box =  Box3(Point3(0,0,0), Point3(0,0,0));
	if(lengthaxis==Z){
		box += Point3(dWidth,	 dLength,	0.0f);
		box += Point3(-dWidth,  -dLength,	0.0f);
	}else{
		box += Point3(0.0f,	 dLength,	 dWidth);
		box += Point3(0.0f, -dLength,	-dWidth);
	}

}

void IKTargetObject::GetWorldBoundBox(
		TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
	{
	Matrix3 tm;
	tm = inode->GetObjectTM(t);
	Box3 lbox;

	GetLocalBoundBox(t, inode, vpt, lbox);
	box = Box3(tm.GetTrans(), tm.GetTrans());
	for (int i=0; i<8; i++) {
		box += lbox * tm;
		}

	//	box += bbox;
}

void IKTargetObject::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt)
	{
	if(suspendSnap)
		return;

	Matrix3 tm = inode->GetObjectTM(t);
	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(tm);

	Matrix3 invPlane = Inverse(snap->plane);

	// Make sure the vertex priority is active and at least as important as the best snap so far
	if(snap->vertPriority > 0 && snap->vertPriority <= snap->priority) {
		Point2 fp = Point2((float)p->x, (float)p->y);
		Point2 screen2;
		IPoint3 pt3;

		Point3 thePoint(0,0,0);
		// If constrained to the plane, make sure this point is in it!
		if(snap->snapType == SNAP_2D || snap->flags & SNAP_IN_PLANE) {
			Point3 test = thePoint * tm * invPlane;
			if(fabs(test.z) > 0.0001)	// Is it in the plane (within reason)?
				return;
			}
		gw->wTransPoint(&thePoint,&pt3);
		screen2.x = (float)pt3.x;
		screen2.y = (float)pt3.y;

		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if(len <= snap->strength) {
			// Is this priority better than the best so far?
			if(snap->vertPriority < snap->priority) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			else
			if(len < snap->bestDist) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			}
		}
	}

int IKTargetObject::DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt)
{
	float dLength, dWidth, dCATUnits;
	int cross, lengthaxis;

	Interval ivalid = FOREVER;
	pblock2->GetValue(iktarget_length,		t,	dLength,	ivalid);
	pblock2->GetValue(iktarget_width,		t,	dWidth,		ivalid);
	pblock2->GetValue(iktarget_cross,		t,	cross,		ivalid);
//	pblock2->GetValue(iktarget_lengthaxis,	t,	axis,		ivalid);

	dCATUnits = GetCATUnits();
	lengthaxis = (flags&CATOBJECTFLAG_LENGTHAXIS_X) ? X : Z;

	dLength *= dCATUnits / 2.0f;
	dWidth *= dCATUnits / 2.0f;

	Matrix3 tm(1);
	Point3 pt(0,0,0);
	Point3 pts[5];

	vpt->getGW()->setTransform(tm);
	tm = inode->GetObjectTM(t);
	vpt->getGW()->setTransform(tm);

	if (cross) {
		// X
		pts[0] = Point3(-dWidth, 0.0f, 0.0f);  pts[1] = Point3(dWidth, 0.0f, 0.0f);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
		bbox+=tm.VectorTransform(pts[0]);  bbox+= tm.VectorTransform(pts[1]);

		// Y
		pts[0] = Point3(0.0f, -dWidth, 0.0f);  pts[1] = Point3(0.0f, dWidth, 0.0f);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
		bbox+=tm.VectorTransform(pts[0]);  bbox+= tm.VectorTransform(pts[1]);

		// Z
		pts[0] = Point3(0.0f, 0.0f, -dWidth);  pts[1] = Point3(0.0f, 0.0f, dWidth);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
		bbox+=tm.VectorTransform(pts[0]);  bbox+= tm.VectorTransform(pts[1]);
	}
	else{

		if(lengthaxis==Z){
			// Bottom
			pts[0] = Point3(-dWidth, -dLength, 0.0f); bbox+=tm.VectorTransform(pts[0]);
			pts[1] = Point3(-dWidth,  dLength, 0.0f); bbox+=tm.VectorTransform(pts[1]);
			pts[2] = Point3( dWidth,  dLength, 0.0f); bbox+=tm.VectorTransform(pts[2]);
			pts[3] = Point3( dWidth, -dLength, 0.0f); bbox+=tm.VectorTransform(pts[3]);
		}else{
			// Bottom
			pts[0] = Point3( 0.0f, -dLength, -dWidth); bbox+=tm.VectorTransform(pts[0]);
			pts[1] = Point3( 0.0f,  dLength, -dWidth); bbox+=tm.VectorTransform(pts[1]);
			pts[2] = Point3( 0.0f,  dLength,  dWidth); bbox+=tm.VectorTransform(pts[2]);
			pts[3] = Point3( 0.0f, -dLength,  dWidth); bbox+=tm.VectorTransform(pts[3]);
		}
		vpt->getGW()->polyline(4, pts, NULL, NULL, TRUE, NULL);
	}

//	vpt->getGW()->setRndLimits(limits);

	return 1;
}

int IKTargetObject::HitTest(
		TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
	{
	UNREFERENCED_PARAMETER(flags);
	Matrix3 tm(1);
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0,0,0);

	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(tm);

   	tm = inode->GetObjectTM(t);
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits())|GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	DrawAndHit(t, inode, vpt);

	gw->setRndLimits(savedLimits);

	if((hitRegion.type != POINT_RGN) && !hitRegion.crossing)
		return TRUE;
	return gw->checkHitCode();
	}

int IKTargetObject::Display(
		TimeValue t, INode* inode, ViewExp *vpt, int flags)
	{
	UNREFERENCED_PARAMETER(flags);
	if (inode->Selected()) {
		vpt->getGW()->setColor( TEXT_COLOR, GetUIColor(COLOR_SELECTION) );
		vpt->getGW()->setColor( LINE_COLOR, GetUIColor(COLOR_SELECTION) );
	} else if (!inode->IsFrozen() && !inode->Dependent()) {
		Color color = Color(inode->GetWireColor());
		vpt->getGW()->setColor( TEXT_COLOR, color);
		vpt->getGW()->setColor( LINE_COLOR, color);
	}

	bbox.Init();
	DrawAndHit(t, inode, vpt);

	return(0);
	}

void IKTargetObject::DisplayObject(TimeValue t, INode* inode, ViewExp *vpt, int flags, Color color, Box3 &){
	UNREFERENCED_PARAMETER(flags);
	vpt->getGW()->setColor( TEXT_COLOR, color);
	vpt->getGW()->setColor( LINE_COLOR, color);
	DrawAndHit(t, inode, vpt);
}

//
// Reference Managment:
//

// This is only called if the object MAKES references to other things.
RefResult IKTargetObject::NotifyRefChanged(
		const Interval&, RefTargetHandle,
		PartID&, RefMessage message, BOOL )
    {
	switch (message) {
		case REFMSG_CHANGE:
			if (editOb==this) InvalidateUI();
			break;
		}
	return(REF_SUCCEED);
	}

void IKTargetObject::InvalidateUI()
	{
	iktarget_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
	}

Interval IKTargetObject::ObjectValidity(TimeValue)
	{
	return FOREVER;
	}

ObjectState IKTargetObject::Eval(TimeValue)
	{
	return ObjectState(this);
	}

RefTargetHandle IKTargetObject::Clone(RemapDir& remap)
	{
	IKTargetObject* newob = new IKTargetObject();
	newob->ReplaceReference(0, remap.CloneRef(pblock2));

	newob->flags = flags;
	BaseClone(this, newob, remap);
	return(newob);
	}

class IKTargetPostLoadCallback : public PostLoadCallback {
public:
	IKTargetObject *pobj;

	IKTargetPostLoadCallback(IKTargetObject *p) {pobj=p;}
	void proc(ILoad *) {

		if(pobj->dwFileSaveVersion < CAT_VERSION_2515){
			if(pobj->GetTransformController() && pobj->GetTransformController()->GetInterface(I_CATNODECONTROL)){
				CATNodeControl* ctrl = (CATNodeControl*)pobj->GetTransformController()->GetInterface(I_CATNODECONTROL);
				ICATParentTrans* catparent = ctrl->GetCATParentTrans();
				if(catparent){
					pobj->pblock2->EnableNotifications(FALSE);
					pobj->SetCATUnits(catparent->GetCATUnits());
					pobj->SetLengthAxis(catparent->GetLengthAxis());
					pobj->pblock2->EnableNotifications(TRUE);
				}
			}
		}

		pobj->dwFileSaveVersion = CAT_VERSION_CURRENT;
	}
};

#define CATOBJECT_FLAGS						1<<5
#define CATOBJECT_VERSION					1<<6

IOResult IKTargetObject::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CATOBJECT_FLAGS:
			res = iload->Read(&flags, sizeof DWORD, &nb);
			break;
		case CATOBJECT_VERSION:
			res = iload->Read(&dwFileSaveVersion, sizeof DWORD, &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new IKTargetPostLoadCallback(this));
	return IO_OK;
}

IOResult IKTargetObject::Save(ISave *isave)
{

	ULONG nb;

	// Stores the number of faces.
	isave->BeginChunk(CATOBJECT_VERSION);
	dwFileSaveVersion = CAT_VERSION_CURRENT;
	isave->Write( &dwFileSaveVersion, sizeof (DWORD), &nb);
	isave->EndChunk();

	// Stores the number of faces.
	isave->BeginChunk(CATOBJECT_FLAGS);
	isave->Write( &flags, sizeof (DWORD), &nb);
	isave->EndChunk();

	return IO_OK;
}

BOOL IKTargetObject::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idObjectParams);

	float x = GetX();
	save->Write(idWidth, x);

	float y = GetY();
	save->Write(idLength, y);

	// Object
	save->EndGroup();

	return TRUE;
}

BOOL IKTargetObject::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok   = TRUE;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idObjectParams) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch(load->CurClauseID())
		{
			case rigBeginGroup:
//				switch(load->CurIdentifier()) {
//				default:
					load->AssertOutOfPlace();
//				}
				break;

			case rigAssignment:
				switch(load->CurIdentifier())
				{
					case idWidth:{
						float x;
						load->GetValue(x);
						SetX(x);
						break;
					}
					case idLength:{
						float y;
						load->GetValue(y);
						SetY(y);
						break;
					}
					default:
						load->AssertOutOfPlace();
					}
				break;
			case rigAbort:
			case rigEnd:
				ok = FALSE;
			case rigEndGroup:
				done=TRUE;
				break;
		}
	}
	return ok && load->ok();
}

float IKTargetObject::GetX(){
	if(GetCross())	return pblock2->GetFloat(iktarget_width);

	// This code broke the scale tool onthe footplatforms
	// And the CATMotion pivot system
/*	if(GetLengthAxis()==X)
		 return 0.0f;
	else*/ return pblock2->GetFloat(iktarget_width);
}

float IKTargetObject::GetY(){
	if(GetCross())	return pblock2->GetFloat(iktarget_width);
	return pblock2->GetFloat(iktarget_length);
}

float IKTargetObject::GetZ(){
	// This code broke the scale tool onthe footplatforms
	// And the CATMotion pivot system
/*	if(GetCross())	return pblock2->GetFloat(iktarget_width);
	if(GetLengthAxis()==X)
		 return pblock2->GetFloat(iktarget_width);
	else*/ return 0.0f;
}

void IKTargetObject::SetX(float val)
{
	if(GetCross()) pblock2->SetValue(iktarget_width, 0, val);
	// This code broke the scale tool onthe footplatforms
	// And the CATMotion pivot system
//	if(GetLengthAxis()==X) return;
	pblock2->SetValue(iktarget_width, 0, val);
};

void IKTargetObject::SetY(float val)
{
	pblock2->SetValue(iktarget_length, 0, val);
};

void IKTargetObject::SetZ(float val)
{
	UNREFERENCED_PARAMETER(val);
//	if(GetCross()) pblock2->SetValue(iktarget_width, 0, val);
	// This code broke the scale tool onthe footplatforms
	// And the CATMotion pivot system
//	if(GetLengthAxis()==Z) return;
//	pblock2->SetValue(iktarget_width, 0, val);
};

Control* IKTargetObject::GetTransformController()
{
	if(!node){
		// Find the node using the handlemessage system
		node = FindReferencingClass<INode>(this);
	}
	if(node)	return node->GetTMController();
	else		return NULL;
}
