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

#include "HubObject.h"
#include <Util/StaticAssert.h>

static bool hubobjectInterfaceAdded = false;

class CATHubClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) {
		UNREFERENCED_PARAMETER(loading);
		HubObject* hubobject = new HubObject();
		if (!hubobjectInterfaceAdded)
		{
			AddInterface(hubobject->GetDescByID(I_CATOBJECT));
			hubobjectInterfaceAdded = true;
		}
		return hubobject;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_HUBOBJECT); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return CATHUB_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("HubObject"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};

static CATHubClassDesc CATHubDesc;
ClassDesc2* GetCATHubDesc() { return &CATHubDesc; }

namespace
{
	MaxSDK::Util::StaticAssert< (HubObject::raifobjects_params == HUBOBJ_PARAMBLOCK_ID) > validator;
}

static ParamBlockDesc2 raifobjects_param_blk ( HubObject::raifobjects_params, _T("HubObject Params"),  0, &CATHubDesc,
	P_AUTO_CONSTRUCT/* + P_AUTO_UI*/, HubObject::HUB_PBLOCK_REF,

	HubObject::PB_HUBTRANS,		_T("HubTrans"), 			TYPE_REFTARG, 	P_NO_REF, 		0,
		p_end,

	HubObject::PB_CATUNITS, 		_T("CATUnits"), 		TYPE_FLOAT, 	0,			NULL,
		p_default, 		1.0f,
		p_end,

	HubObject::PB_LENGTH,			_T("Length"),			TYPE_FLOAT, 	0, 			IDS_LENGTH,
		p_default, 		25.0f,
		p_end,

	HubObject::PB_WIDTH, 			_T("Width"), 			TYPE_FLOAT, 	0, 			IDS_WIDTH,
		p_default, 		20.0f,
		p_end,

	HubObject::PB_HEIGHT, 			_T("Height"), 			TYPE_FLOAT, 	0, 			IDS_HEIGHT,
		p_default, 		10.0f,
		p_end,

	HubObject::PB_PIVOTPOSY, 		_T("PivotPosY"), 		TYPE_FLOAT, 	0, 			IDS_PIVOTPOSY,
		p_default, 		0.0f,
		p_end,

	HubObject::PB_PIVOTPOSZ, 		_T("PivotPosZ"), 		TYPE_FLOAT, 	0, 			IDS_PIVOTPOSZ,
		p_default, 		0.0f,
		p_end,
	p_end
	);


//--- HubObject -------------------------------------------------------

HubObject::HubObject()
{
	CATObject::Init();
	CATHubDesc.MakeAutoParamBlocks(this);
}

HubObject::~HubObject()
{
}

void HubObject::SetParams(float dWidth, float height, float length, float pivotY/*=0.0f*/, float pivotZ/*=0.0f*/)
{
	pblock2->SetValue(PB_WIDTH, 0, dWidth);
	pblock2->SetValue(PB_LENGTH, 0, length);
	pblock2->SetValue(PB_HEIGHT, 0, height);
	pblock2->SetValue(PB_PIVOTPOSY, 0, pivotY);
	pblock2->SetValue(PB_PIVOTPOSZ, 0, pivotZ);
}

/*
void HubObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;

	SimpleObject2::BeginEditParams(ip,flags,prev);
	CATHubDesc.BeginEditParams(ip, this, flags, prev);

	Animatable* ctrlHubTrans = (Animatable*)pblock2->GetReferenceTarget(PB_HUBTRANS);
	if(ctrlHubTrans) ctrlHubTrans->BeginEditParams(ip,flags,prev);

}

void HubObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	//TODO: Save plugin parameter values into class variables, if they are not hosted in ParamBlocks.

	SimpleObject2::EndEditParams(ip,flags,next);
	CATHubDesc.EndEditParams(ip, this, flags, next);

	Animatable* ctrlHubTrans = (Animatable*)pblock2->GetReferenceTarget(PB_HUBTRANS);
	if(ctrlHubTrans) ctrlHubTrans->EndEditParams(ip,flags,next);

	this->ip = NULL;
}

//Class for interactive creation of the object using the mouse
class CATHubCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;		//First point in screen coordinates
	HubObject *ob;		//Pointer to the object
	Point3 p0;			//First point in world coordinates
public:
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(HubObject *obj) {ob = obj;}
};

int CATHubCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat )
{
	//TODO: Implement the mouse creation code here
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
		case 0: // only happens with MOUSE_POINT msg
			ob->suspendSnap = TRUE;
			sp0 = m;
			p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_PLANE);
			mat.SetTrans(p0);

			ob->suspendSnap = FALSE;
			return CREATE_STOP;
			break;
		//TODO: Add for the rest of points
		}
	} else {
		if (msg == MOUSE_ABORT) return CREATE_ABORT;
	}

	return FALSE;
}

static CATHubCreateCallBack CATHubCreateCB;

//From BaseObject
CreateMouseCallBack* HubObject::GetCreateMouseCallBack()
{
	CATHubCreateCB.SetObj(this);
	return(&CATHubCreateCB);
}
*/

/**********************************************************************
 * Display functions (From SimpleObject)
 */

void HubObject::BuildMesh(TimeValue t)
{
	if(UsingCustomMesh()){
		CATObject::BuildMesh(t);
		return;
	}

	float dLength, dWidth, dHeight, catunits, dPivotY, dPivotZ;
//	ECATParent *catparent;

	// Start the validity interval at forever and widdle it down.
//	Interval interval = FOREVER;

	// Get necessary params
	catunits = pblock2->GetFloat(PB_CATUNITS);
	// Everything is scaled by CATUnits.
	dLength = pblock2->GetFloat(PB_LENGTH) * catunits;
	dWidth = pblock2->GetFloat(PB_WIDTH) * catunits;
	dHeight = pblock2->GetFloat(PB_HEIGHT) * catunits;

//	if(flags&CATOBJECTFLAG_LENGTHAXIS_X){
//		float temp = dHeight;
//		dHeight = dWidth;
//		dWidth = temp;
//	}

	dWidth = dWidth / 2.0f;

	dPivotZ = pblock2->GetFloat(PB_PIVOTPOSZ)/2.0f;
	dPivotY = pblock2->GetFloat(PB_PIVOTPOSY)/2.0f;

	// Initialise the mesh...
	mesh.setNumVerts(8);
	mesh.setNumFaces(12);
	mesh.InvalidateTopologyCache();

	if(!(flags&CATOBJECTFLAG_LENGTHAXIS_X)){
		// Vertices
		mesh.setVert( 0,   -dWidth,  -dLength * (0.5f + dPivotY),	-dHeight * (0.5f + dPivotZ));
		mesh.setVert( 1,    dWidth,  -dLength * (0.5f + dPivotY),	-dHeight * (0.5f + dPivotZ));
		mesh.setVert( 2,   -dWidth,   dLength * (0.5f - dPivotY),	-dHeight * (0.5f + dPivotZ));
		mesh.setVert( 3,    dWidth,   dLength * (0.5f - dPivotY),	-dHeight * (0.5f + dPivotZ));
		mesh.setVert( 4,   -dWidth,  -dLength * (0.5f + dPivotY),	 dHeight * (0.5f - dPivotZ));
		mesh.setVert( 5,    dWidth,  -dLength * (0.5f + dPivotY),	 dHeight * (0.5f - dPivotZ));
		mesh.setVert( 6,   -dWidth,   dLength * (0.5f - dPivotY),	 dHeight * (0.5f - dPivotZ));
		mesh.setVert( 7,    dWidth,   dLength * (0.5f - dPivotY),	 dHeight * (0.5f - dPivotZ));
	}else{
		// Vertices
		mesh.setVert( 0,	-dHeight * (0.5f + dPivotZ),  -dLength * (0.5f + dPivotY),    dWidth);
		mesh.setVert( 1,	-dHeight * (0.5f + dPivotZ),  -dLength * (0.5f + dPivotY),   -dWidth);
		mesh.setVert( 2,	-dHeight * (0.5f + dPivotZ),   dLength * (0.5f - dPivotY),    dWidth);
		mesh.setVert( 3,	-dHeight * (0.5f + dPivotZ),   dLength * (0.5f - dPivotY),   -dWidth);
		mesh.setVert( 4,	 dHeight * (0.5f - dPivotZ),  -dLength * (0.5f + dPivotY),    dWidth);
		mesh.setVert( 5,	 dHeight * (0.5f - dPivotZ),  -dLength * (0.5f + dPivotY),   -dWidth);
		mesh.setVert( 6,	 dHeight * (0.5f - dPivotZ),   dLength * (0.5f - dPivotY),    dWidth);
		mesh.setVert( 7,	 dHeight * (0.5f - dPivotZ),   dLength * (0.5f - dPivotY),   -dWidth);
	}

	// Faces
	MakeFace(mesh.faces[ 0],  0,  2,  3, 1, 2, 0,  1, 1);
	MakeFace(mesh.faces[ 1],  3,  1,  0, 1, 2, 0,  1, 1);
	MakeFace(mesh.faces[ 2],  4,  5,  7, 1, 2, 0,  2, 0);
	MakeFace(mesh.faces[ 3],  7,  6,  4, 1, 2, 0,  2, 0);
	MakeFace(mesh.faces[ 4],  0,  1,  5, 1, 2, 0,  3, 4);
	MakeFace(mesh.faces[ 5],  5,  4,  0, 1, 2, 0,  3, 4);
	MakeFace(mesh.faces[ 6],  1,  3,  7, 1, 2, 0,  4, 3);
	MakeFace(mesh.faces[ 7],  7,  5,  1, 1, 2, 0,  4, 3);
	MakeFace(mesh.faces[ 8],  3,  2,  6, 1, 2, 0,  5, 5);
	MakeFace(mesh.faces[ 9],  6,  7,  3, 1, 2, 0,  5, 5);
	MakeFace(mesh.faces[10],  2,  0,  4, 1, 2, 0,  6, 2);
	MakeFace(mesh.faces[11],  4,  6,  2, 1, 2, 0,  6, 2);

	mesh.InvalidateTopologyCache();

	ivalid.SetInfinite();
}

BOOL HubObject::OKtoDisplay(TimeValue)
{
	//TODO: Check whether all the parameters have valid values and return the state
	return TRUE;
}

void HubObject::InvalidateUI()
{
	// Hey! Update the param blocks
}

// From ReferenceTarget
RefTargetHandle HubObject::Clone(RemapDir& remap)
{
	HubObject* newob = new HubObject();
	//TODO: Make a copy all the data and also clone all the references
	newob->ReplaceReference(0,remap.CloneRef(pblock2));

	newob->catmesh = catmesh;
	newob->flags = flags;
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}

RefResult HubObject::NotifyRefChanged( const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if(pblock2 == hTarg)
		{
			int tabIndex = 0;
			ParamID index = pblock2->LastNotifyParamID(tabIndex);
			switch(index)
			{
			case PB_WIDTH:
			case PB_HEIGHT:
			case PB_LENGTH:
				Update();
				if(GetTransformController() && GetTransformController()->GetInterface(I_CATNODECONTROL)){
					CATNodeControl *ctrl = (CATNodeControl*)GetTransformController()->GetInterface(I_CATNODECONTROL);
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

RefTargetHandle HubObject::GetReference(int i)
{
	return SimpleObject2::GetReference(i);
}

/*
void HubObject::SetColour()
{
	INode *nodeHub = pblock2->GetINode(PB_NODE);
	if(nodeHub)
	{
		Color HubColour = pblock2->GetColor(PB_COLOUR);
		nodeHub->SetWireColor(HubColour);
	}
}

void HubObject::SetName()
{
	INode *nodeHub = pblock2->GetINode(PB_NODE);
	if(nodeHub)
	{
		TSTR HubName = pblock2->GetStr(PB_NAME);
		nodeHub->SetName(HubName);
		nodeHub->InvalidateWS();
	}
}

*/

BOOL HubObject::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idObjectParams);

	float x = GetX();
	save->Write(idWidth, x);

	float y = GetY();
	save->Write(idLength, y);

	float z = GetZ();
	save->Write(idHeight, z);

	save->FromParamBlock(pblock2, idPivotPosZ, PB_PIVOTPOSZ);
	save->FromParamBlock(pblock2, idPivotPosY, PB_PIVOTPOSY);

	SaveMesh(save);

	// Object
	save->EndGroup();

	return TRUE;
}

BOOL HubObject::LoadRig(CATRigReader *load)
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
				switch(load->CurIdentifier()) {
				case idMeshParams:
					LoadMesh(load);
					break;
				default:
					load->AssertOutOfPlace();
				}
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
					case idHeight:{
						float z;
						load->GetValue(z);
						SetZ(z);
						break;
					}
					case idPivotPosY:
						load->ToParamBlock(pblock2, PB_PIVOTPOSY);
						break;
					case idPivotPosZ:
						load->ToParamBlock(pblock2, PB_PIVOTPOSZ);
						break;
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
