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
#include "Bone.h"

static bool catboneobjectInterfaceAdded = false;

class CATBoneClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) {
		UNREFERENCED_PARAMETER(loading);
		CATBone* catboneobject = new CATBone();
		if (!catboneobjectInterfaceAdded)
		{
			AddInterface(catboneobject->GetDescByID(I_CATOBJECT));
			catboneobjectInterfaceAdded = true;
		}
		return catboneobject;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_BONE); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return CATBONE_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATBone"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};

static CATBoneClassDesc CATBoneDesc;
ClassDesc2* GetCATBoneDesc() { return &CATBoneDesc; }

static ParamBlockDesc2 raifobjects_param_blk(CATBone::CATBone_params, _T("CATBone Params"), 0, &CATBoneDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, CATBone::BONE_PBLOCK_REF,
	//rollout
	IDD_CATBONE, IDS_PARAMS, 0, 0, NULL,
	// params
	CATBone::PB_BONETRANS, _T("DigitData"), TYPE_REFTARG, P_NO_REF, NULL,
		p_end,

	CATBone::PB_CATUNITS, _T("CATUnits"), TYPE_FLOAT, 0, IDS_CATUNITS,
		p_default, 1.0f,
		p_end,

	CATBone::PB_LENGTH, _T("Length"), TYPE_FLOAT, 0, IDS_LENGTH,
		p_default, 20.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LENGTH, IDC_SPIN_LENGTH, 0.01f,
		p_end,

	CATBone::PB_WIDTH, _T("Width"), TYPE_FLOAT, 0, IDS_WIDTH,
		p_default, 5.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_WIDTH, IDC_SPIN_WIDTH, 0.01f,
		p_end,

	CATBone::PB_DEPTH, _T("Depth"), TYPE_FLOAT, 0, IDS_DEPTH,
		p_default, 5.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_DEPTH, IDC_SPIN_DEPTH, 0.01f,
		p_end,

	p_end
);

IObjParam *CATBone::ip = NULL;

//--- CATBone -------------------------------------------------------

CATBone::CATBone()
{
	CATObject::Init();
	CATBoneDesc.MakeAutoParamBlocks(this);
}

CATBone::~CATBone()
{
}

/*
IOResult CATBone::Load(ILoad *iload)
{
	//TODO: Add code to allow plugin to load its data

	return IO_OK;
}

IOResult CATBone::Save(ISave *isave)
{
	//TODO: Add code to allow plugin to save its data

	return IO_OK;
}

void CATBone::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;

//	SimpleObject2::BeginEditParams(ip,flags,prev);
//	CATBoneDesc.BeginEditParams(ip, this, flags, prev);

	Animatable* ctrlBoneTrans = pblock2->GetReferenceTarget(PB_BONETRANS);
	if(ctrlBoneTrans) ctrlBoneTrans->BeginEditParams(ip,flags,prev);
}

void CATBone::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	//TODO: Save plugin parameter values into class variables, if they are not hosted in ParamBlocks.

//	SimpleObject2::EndEditParams(ip,flags,next);
//	CATBoneDesc.EndEditParams(ip, this, flags, next);

	Animatable* ctrlBoneTrans = pblock2->GetReferenceTarget(PB_BONETRANS);
	if(ctrlBoneTrans) ctrlBoneTrans->EndEditParams(ip,flags,next);

	this->ip = NULL;
}
*/

//From Object
BOOL CATBone::HasUVW()
{
	//TODO: Return whether the object has UVW coordinates or not
	return FALSE;
}

void CATBone::SetGenUVW(BOOL sw)
{
	if (sw == HasUVW()) return;
	//TODO: Set the plugin's internal value to sw
}

//Class for interactive creation of the object using the mouse
class CATBoneCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;		//First point in screen coordinates
	CATBone *ob;		//Pointer to the object
	Point3 p0;			//First point in world coordinates
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(CATBone *obj) { ob = obj; }
};

int CATBoneCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	UNREFERENCED_PARAMETER(flags);
	//TODO: Implement the mouse creation code here
	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch (point) {
		case 0: // only happens with MOUSE_POINT msg
			ob->suspendSnap = TRUE;
			sp0 = m;
			p0 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
			mat.SetTrans(p0);

			ob->suspendSnap = FALSE;
			return CREATE_STOP;
			break;
			//TODO: Add for the rest of points
		}
	}
	else {
		if (msg == MOUSE_ABORT) return CREATE_ABORT;
	}

	return FALSE;
}

static CATBoneCreateCallBack CATBoneCreateCB;

//From BaseObject
CreateMouseCallBack* CATBone::GetCreateMouseCallBack()
{
	CATBoneCreateCB.SetObj(this);
	return(&CATBoneCreateCB);
}

/**********************************************************************
 * Display functions (From SimpleObject)
 */

void CATBone::BuildMesh(TimeValue t)
{
	if (UsingCustomMesh()) {
		CATObject::BuildMesh(t);
		return;
	}

	float dLength, dWidth, dDepth, catunits;
	//	ECATParent *catparent;

		// Get necessary params
	catunits = pblock2->GetFloat(PB_CATUNITS);
	dLength = pblock2->GetFloat(PB_LENGTH) * catunits;
	dWidth = pblock2->GetFloat(PB_WIDTH)* catunits;
	dDepth = pblock2->GetFloat(PB_DEPTH)* catunits;

	if (flags&CATOBJECTFLAG_LENGTHAXIS_X) {
		//		float temp = dLength;
		//		dLength = dWidth;
		//		dWidth = temp;
	}

	dWidth = dWidth / 2.0f;
	dDepth = dDepth / 2.0f;

	// Initialise the mesh...
	mesh.setNumVerts(8);
	mesh.setNumFaces(12);
	mesh.InvalidateTopologyCache();

	if (!(flags&CATOBJECTFLAG_LENGTHAXIS_X)) {
		// Vertices
		mesh.setVert(0, -dWidth, -dDepth, 0.0f);
		mesh.setVert(1, dWidth, -dDepth, 0.0f);
		mesh.setVert(2, -dWidth, dDepth, 0.0f);
		mesh.setVert(3, dWidth, dDepth, 0.0f);
		mesh.setVert(4, -dWidth, -dDepth, dLength);
		mesh.setVert(5, dWidth, -dDepth, dLength);
		mesh.setVert(6, -dWidth, dDepth, dLength);
		mesh.setVert(7, dWidth, dDepth, dLength);
	}
	else {
		// Vertices
		mesh.setVert(0, 0.0f, -dDepth, dWidth);
		mesh.setVert(1, 0.0f, -dDepth, -dWidth);
		mesh.setVert(2, 0.0f, dDepth, dWidth);
		mesh.setVert(3, 0.0f, dDepth, -dWidth);
		mesh.setVert(4, dLength, -dDepth, dWidth);
		mesh.setVert(5, dLength, -dDepth, -dWidth);
		mesh.setVert(6, dLength, dDepth, dWidth);
		mesh.setVert(7, dLength, dDepth, -dWidth);

		/*		mesh.setVert( 0,   0.0f,     -dDepth,	 dWidth);
				mesh.setVert( 1,   0.0f,     -dDepth,	-dWidth);
				mesh.setVert( 2,   0.0f,      dDepth,	 dWidth);
				mesh.setVert( 3,   0.0f,	  dDepth,	-dWidth);
				mesh.setVert( 4,   dLength,  -dDepth,	 dWidth);
				mesh.setVert( 5,   dLength,  -dDepth,	-dWidth);
				mesh.setVert( 6,   dLength,   dDepth,	 dWidth);
				mesh.setVert( 7,   dLength,   dDepth,	-dWidth);
		*/
	}

	// Faces
	MakeFace(mesh.faces[0], 0, 2, 3, 1, 2, 0, 1, 1);
	MakeFace(mesh.faces[1], 3, 1, 0, 1, 2, 0, 1, 1);
	MakeFace(mesh.faces[2], 4, 5, 7, 1, 2, 0, 2, 0);
	MakeFace(mesh.faces[3], 7, 6, 4, 1, 2, 0, 2, 0);
	MakeFace(mesh.faces[4], 0, 1, 5, 1, 2, 0, 3, 4);
	MakeFace(mesh.faces[5], 5, 4, 0, 1, 2, 0, 3, 4);
	MakeFace(mesh.faces[6], 1, 3, 7, 1, 2, 0, 4, 3);
	MakeFace(mesh.faces[7], 7, 5, 1, 1, 2, 0, 4, 3);
	MakeFace(mesh.faces[8], 3, 2, 6, 1, 2, 0, 5, 5);
	MakeFace(mesh.faces[9], 6, 7, 3, 1, 2, 0, 5, 5);
	MakeFace(mesh.faces[10], 2, 0, 4, 1, 2, 0, 6, 2);
	MakeFace(mesh.faces[11], 4, 6, 2, 1, 2, 0, 6, 2);

	mesh.InvalidateTopologyCache();

	ivalid.SetInfinite();
}

BOOL CATBone::OKtoDisplay(TimeValue)
{
	//TODO: Check whether all the parameters have valid values and return the state
	return TRUE;
}

void CATBone::InvalidateUI()
{
	// Hey! Update the param blocks
}

Object* CATBone::ConvertToType(TimeValue t, Class_ID obtype)
{
	//TODO: If the plugin can convert to a nurbs surface then check to see
	//		whether obtype == EDITABLE_SURF_CLASS_ID and convert the object
	//		to nurbs surface and return the object

	return SimpleObject2::ConvertToType(t, obtype);
}

int CATBone::CanConvertToType(Class_ID obtype)
{
	//TODO: Check for any additional types the plugin supports
	if (obtype == defObjectClassID ||
		obtype == triObjectClassID) {
		return 1;
	}
	else {
		return SimpleObject2::CanConvertToType(obtype);
	}
}
/*
void CATBone::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
	 Object::GetCollapseTypes(clist, nlist);
	//TODO: Append any any other collapse type the plugin supports
}
*/
// From Object
int CATBone::IntersectRay(
	TimeValue t, Ray& ray, float& at, Point3& norm)
{
	UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(ray); UNREFERENCED_PARAMETER(at); UNREFERENCED_PARAMETER(norm);
	//TODO: Return TRUE after you implement this method
	return FALSE;
}

// From ReferenceTarget
RefTargetHandle CATBone::Clone(RemapDir& remap)
{
	CATBone* newob = new CATBone();
	//TODO: Make a copy all the data and also clone all the references
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->catmesh = catmesh;
	newob->flags = flags;
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}

RefTargetHandle CATBone::GetReference(int i)
{
	return SimpleObject2::GetReference(i);
}

RefResult CATBone::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if (pblock2 == hTarg)
		{
			int tabIndex = 0;
			ParamID index = pblock2->LastNotifyParamID(tabIndex);
			switch (index)
			{
			case PB_WIDTH:
			case PB_DEPTH:
			case PB_LENGTH:
				Update();
				if (GetTransformController() && GetTransformController()->GetInterface(I_CATNODECONTROL)) {
					CATNodeControl *ctrl = (CATNodeControl*)GetTransformController()->GetInterface(I_CATNODECONTROL);

					// Size change doesn't affect the link info panel
					CATControl::SuppressLinkInfoUpdate suppress;
					ctrl->UpdateUI();

					ctrl->UpdateObjDim();
					// A new tmChildParent needs to be calculated.
					// This will force the Node to be re-evaluated.
					ctrl->GetNode()->InvalidateTreeTM();
				}
				break;
			}
		}
		break;
	}
	return REF_SUCCEED;
}

