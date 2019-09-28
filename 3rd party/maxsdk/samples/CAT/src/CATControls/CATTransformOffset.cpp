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

#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>
#include "CATTransformOffset.h"
#include <plugapi.h>
 //
 //	CATTransformOffsetClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class CATTransformOffsetClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { return new CATTransformOffset(loading); }
	const TCHAR *	ClassName() { return _T("CATTransformOffset"); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return CAT_TRANSFORM_OFFSET_CONTROL_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATTransformOffset"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static CATTransformOffsetClassDesc CATTransformOffsetDesc;
ClassDesc2* GetCATTransformOffsetDesc()
{
	CATTransformOffsetDesc.AddInterface(ICATTransformOffsetFP::GetFnPubDesc());
	return &CATTransformOffsetDesc;
}

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc cattransformoffset_FPinterface(
	I_CATTRANSFORMOFFSET_FP, _T("ICATTransformOffsetFPInterface"), 0, NULL, FP_MIXIN,

	properties,

	ICATTransformOffsetFP::propGetCoordSysNode, ICATTransformOffsetFP::propSetCoordSysNode, _T("CoordSysNode"), 0, TYPE_INODE,
	ICATTransformOffsetFP::propGetFlags, ICATTransformOffsetFP::propSetFlags, _T("Flags"), 0, TYPE_BITARRAY,

	p_end
);

FPInterfaceDesc* CATTransformOffset::GetDescByID(Interface_ID id) {
	if (id == I_CATTRANSFORMOFFSET_FP) return &cattransformoffset_FPinterface;
	return NULL;
}

FPInterfaceDesc* ICATTransformOffsetFP::GetFnPubDesc()
{
	return &cattransformoffset_FPinterface;
}

void CATTransformOffset::Init()
{
	trans = NULL;
	offset = NULL;
	ipbegin = NULL;
	flags = dwFileSaveVersion = 0;
	coordsys_node = NULL;
};

CATTransformOffset::CATTransformOffset(BOOL loading) : hdl(0L), EditPanel(0)
{
	Init();

	if (!loading)
	{
		ReplaceReference(TRANSFORM, NewDefaultMatrix3Controller());
		ReplaceReference(OFFSET, NewDefaultMatrix3Controller());
	}
}

RefTargetHandle CATTransformOffset::Clone(RemapDir& remap)
{
	// make a new CATTransformOffset object to be the clone
	// call true for loading so the new CATTransformOffset doesn't
	// make new default subcontrollers.
	CATTransformOffset *newCATTransformOffset = new CATTransformOffset(TRUE);

	// clone our subcontrollers and assign them to the new object.
	if (trans)	newCATTransformOffset->ReplaceReference(TRANSFORM, remap.CloneRef(trans));
	if (offset)	newCATTransformOffset->ReplaceReference(OFFSET, remap.CloneRef(offset));

	BaseClone(this, newCATTransformOffset, remap);

	// now return the new object.
	return newCATTransformOffset;
}

CATTransformOffset::~CATTransformOffset()
{
	DeleteAllRefs();
}

void CATTransformOffset::Copy(Control *from)
{
	if (from->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID, 0)) {
		ReplaceReference(TRANSFORM, from);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

DWORD CATTransformOffset::GetInheritanceFlags() {
	return INHERIT_ALL;
};
BOOL  CATTransformOffset::SetInheritanceFlags(DWORD f, BOOL keepPos)
{
	UNREFERENCED_PARAMETER(f); UNREFERENCED_PARAMETER(keepPos);
	return TRUE;
};

void CATTransformOffset::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	if (!trans || !offset) return;

	SetXFormPacket *ptr = (SetXFormPacket*)val;
	Interval iv = FOREVER;
	trans->GetValue(t, (void*)&(ptr->tmParent), iv, CTRL_RELATIVE);
	/*	if(trans->GetPositionController()&&trans->GetRotationController()&&trans->GetScaleController()){
			Matrix3 &val = ptr->tmParent;
			trans->GetPositionController()->GetValue(t, (void*)&val, FOREVER, CTRL_RELATIVE);

			//Scale the transfrom amount using the offset scale controller
			ScaleValue sv;
			trans->GetScaleController()->GetValue(t, (void*)&sv, FOREVER, CTRL_ABSOLUTE);
			val.SetTrans(ptr->tmParent.GetTrans() + ((val.GetTrans() - ptr->tmParent.GetTrans()) * sv.s));

			trans->GetRotationController()->GetValue(t, (void*)&val, FOREVER, CTRL_RELATIVE);
		}else{
			trans->GetValue(t, val, FOREVER, CTRL_RELATIVE);
		}
	*/
	offset->SetValue(t, val, commit, method);
}

void CATTransformOffset::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	if (!trans || !offset) return;

	Matrix3 tmParent = *(Matrix3*)val;

	// The code below caused problems because of the way that CATClipMatrix3 deals with scale controllers
	// The goal was to use the scale controller to scale the motion of the position controller.
	// This would allow you to resize the motion dynamicly from one
	if (trans->GetPositionController() && trans->GetRotationController() && trans->GetScaleController()) {
		trans->GetPositionController()->GetValue(t, val, valid, method);

		//Scale the transfrom amount using the offset scale controller
		ScaleValue sv;
		trans->GetScaleController()->GetValue(t, (void*)&sv, valid, CTRL_ABSOLUTE);
		(*(Matrix3*)val).SetTrans(tmParent.GetTrans() + (((*(Matrix3*)val).GetTrans() - tmParent.GetTrans()) * sv.s));

		trans->GetRotationController()->GetValue(t, val, valid, method);
	}
	else {
		trans->GetValue(t, val, valid, method);
	}

	if (offset->GetPositionController() && offset->GetRotationController() && offset->GetScaleController())
	{
		if (flags&CATTMOFFSET_FLAG_APPLY_OFFSET_IN_XY_PLANE) {
			Matrix3 pos_par = *(Matrix3*)val;
			AngAxis ax;
			ax.angle = -acos((float)min(1.0f, DotProd(pos_par.GetRow(Z), P3_UP_VEC)));
			ax.axis = Normalize(CrossProd(pos_par.GetRow(Z), P3_UP_VEC));
			RotateMatrix(pos_par, ax); pos_par.SetTrans(((Matrix3*)val)->GetTrans());
			offset->GetPositionController()->GetValue(t, (void*)&pos_par, valid, method);
			((Matrix3*)val)->SetTrans(pos_par.GetTrans());
		}
		else if (flags&CATTMOFFSET_FLAG_APPLY_OFFSET_NODE_COORDSYS && coordsys_node) {
			Matrix3 pos_par = coordsys_node->GetNodeTM(t, &valid);
			pos_par.SetTrans(((Matrix3*)val)->GetTrans());
			offset->GetPositionController()->GetValue(t, (void*)&pos_par, valid, method);
			((Matrix3*)val)->SetTrans(pos_par.GetTrans());
		}
		else {
			offset->GetPositionController()->GetValue(t, val, valid, method);
		}

		offset->GetRotationController()->GetValue(t, val, valid, method);
		offset->GetScaleController()->GetValue(t, val, valid, method);
	}
	else {
		offset->GetValue(t, val, valid, method);
	}
}

RefTargetHandle CATTransformOffset::GetReference(int i)
{
	switch (i)
	{
	case TRANSFORM:		return trans;
	case OFFSET:		return offset;
	case COORDSYSNODE:	return coordsys_node;
	default:		return NULL;
	}
}

void CATTransformOffset::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case TRANSFORM:		trans = (Control*)rtarg; 	break;
	case OFFSET:		offset = (Control*)rtarg; 	break;
	case COORDSYSNODE:	coordsys_node = (INode*)rtarg; 	break;
	}
}

Animatable* CATTransformOffset::SubAnim(int i)
{
	switch (i)
	{
	case TRANSFORM:	return trans;
	case OFFSET:	return offset;
	default:		return NULL;
	}
}

TSTR CATTransformOffset::SubAnimName(int i)
{
	switch (i)
	{
	case TRANSFORM: return GetString(IDS_TRANSFORM);
	case OFFSET:	return GetString(IDS_OFFSET);
	default:		return _T("");
	}
}

RefResult CATTransformOffset::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (trans == hTarg)				trans = NULL;
		if (offset == hTarg)			offset = NULL;
		if (coordsys_node == hTarg)		coordsys_node = NULL;
		break;
	}
	return REF_SUCCEED;
}

BOOL CATTransformOffset::AssignController(Animatable *control, int subAnim)
{
	switch (subAnim)
	{
	case TRANSFORM:	ReplaceReference(TRANSFORM, (RefTargetHandle)control);		break;
	case OFFSET:	ReplaceReference(OFFSET, (RefTargetHandle)control);			break;
	}

	// Note: 0 here means that there is no special information we need to include
	// in this message.
	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE, TREE_VIEW_CLASS_ID, FALSE);	// this explicitly refreshes the tree view.		( the false here says for it not to propogate )
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);								// this refreshes all objects (controllers etc..) that reference us.  (and it propogates)
	return TRUE;
}

void CATTransformOffset::BeginEditParams(IObjParam *, ULONG, Animatable *)
{

}

void CATTransformOffset::EndEditParams(IObjParam *, ULONG, Animatable *)
{

}

BitArray*	CATTransformOffset::IGetFlags()
{
	BitArray *ba = new BitArray(31);
	for (int i = 0; i < 31; i++) { if (flags&(1 << i)) ba->Set(i); }
	return ba;
}
void		CATTransformOffset::ISetFlags(BitArray *ba_flags)
{
	for (int i = 0; i < ba_flags->GetSize(); i++) { SetFlag((*ba_flags)[i]); }
}

/**********************************************************************
 * Loading and saving....
 */
 //////////////////////////////////////////////////////////////////////
 // Backwards compatibility
 //
class CATTransformOffsetPLCB : public PostLoadCallback {
protected:
	CATTransformOffset *bone;

public:
	CATTransformOffsetPLCB(CATTransformOffset *pOwner) { bone = pOwner; }

	DWORD GetFileSaveVersion() {

	}

	int Priority() { return 5; }

	void proc(ILoad *) {
		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define CAT_TRANSFORM_OFFSET_CONTROL_VERSION		1
#define CAT_TRANSFORM_OFFSET_CONTROL_FOOT			2
#define CAT_TRANSFORM_OFFSET_CONTROL_FLAGS			3

IOResult CATTransformOffset::Save(ISave *isave) {
	IOResult res;
	ULONG nb;

	isave->BeginChunk(CAT_TRANSFORM_OFFSET_CONTROL_VERSION);
	dwFileSaveVersion = CAT_VERSION_CURRENT;
	res = isave->Write(&dwFileSaveVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(CAT_TRANSFORM_OFFSET_CONTROL_FLAGS);
	res = isave->Write(&flags, sizeof(DWORD), &nb);
	isave->EndChunk();

	return res;
}

IOResult CATTransformOffset::Load(ILoad *iload) {
	IOResult res;
	ULONG nb;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CAT_TRANSFORM_OFFSET_CONTROL_VERSION:
			iload->Read(&dwFileSaveVersion, sizeof(DWORD), &nb);
			break;
		case CAT_TRANSFORM_OFFSET_CONTROL_FLAGS:
			iload->Read(&flags, sizeof(DWORD), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	return IO_OK;
}

