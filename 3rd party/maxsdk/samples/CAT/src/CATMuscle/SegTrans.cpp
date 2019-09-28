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

/**********************************************************************
	SegTrans is a transform controller that returns the trasformation
	Matrix as calculated by PatchMucle.
	It is a very simple little controller with no Subs or References
 **********************************************************************/////

#include "CATMuscle.h"
#include "IMuscle.h"
#include "SegTrans.h"

 //
 //	SegTransClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class SegTransClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }									// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { return new SegTrans(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_SEGTRANS); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }			// This determins the type of our controller
	Class_ID		ClassID() { return SEGTRANS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("SegTrans"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }							// returns owning module handle

};

static SegTransClassDesc SegTransDesc;
ClassDesc2* GetSegTransDesc() { return &SegTransDesc; }

void SegTrans::Init()
{
	mWeakRefToObject.SetRef(nullptr);
	nStrandID = -1;
	nSegID = -1;
}

SegTrans::SegTrans(BOOL loading) : dVersion(0L), mpObjectForLoad(nullptr)
{
	Init();
	if (!loading) {
	}
}

void* SegTrans::GetInterface(ULONG id) {
	if (id == I_MASTER)
		return (void *)mWeakRefToObject.GetRef();
	else
		return Control::GetInterface(id);
}

RefTargetHandle SegTrans::Clone(RemapDir& remap)
{
	// make a new SegTrans object to be the clone
	SegTrans *newctrl = new SegTrans(TRUE);

	// if the controller is cloning, force the object to clone as well
	newctrl->mWeakRefToObject.SetRef(remap.CloneRef(mWeakRefToObject.GetRef()));
	newctrl->nStrandID = nStrandID;
	newctrl->nSegID = nSegID;

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newctrl, remap);
	// now return the new object.
	return newctrl;
}

SegTrans::~SegTrans()
{
	// TODO see if this is still a problem
//	if(object) object->tabStrips[nStrandID]->tabSegs[nSegID] = NULL;
	DeleteAllRefs();
}

void SegTrans::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.

	}
	else
	{
		//wont be done (i hopes)
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void SegTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	SetXFormPacket* ptr = reinterpret_cast<SetXFormPacket*>(val);

	if (mWeakRefToObject.GetRef() && GetCATMuscleSystemInterface(mWeakRefToObject.GetRef()))
		GetCATMuscleSystemInterface(mWeakRefToObject.GetRef())->SetValue(t, ptr, commit, this);
};

void SegTrans::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod)
{
	if (!mWeakRefToObject.GetRef()) return;
	GetCATMuscleSystemInterface(mWeakRefToObject.GetRef())->GetTrans(t, nStrandID, nSegID, *(Matrix3*)val, valid);
}

void SegTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) {
	if (mWeakRefToObject.GetRef())	mWeakRefToObject.GetRef()->BeginEditParams(ip, flags, prev);
}

void SegTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) {
	if (mWeakRefToObject.GetRef())	mWeakRefToObject.GetRef()->EndEditParams(ip, flags, next);
}

#define CHUNK_SEG_ID			0X01
#define CHUNK_STRAND_ID			0X02
#define CHUNK_MUSCLE			0X03
#define CHUNK_NODE				0X04
#define CHUNK_VERSION			0X05

IOResult SegTrans::Save(ISave *isave)
{
	DWORD nb, refID;

	isave->BeginChunk(CHUNK_VERSION);
	dVersion = CAT_VERSION_CURRENT;
	isave->Write(&dVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(CHUNK_STRAND_ID);
	isave->Write(&nStrandID, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(CHUNK_SEG_ID);
	isave->Write(&nSegID, sizeof(int), &nb);
	isave->EndChunk();

	// The object handle
	refID = isave->GetRefID((void*)mWeakRefToObject.GetRef());
	isave->BeginChunk(CHUNK_MUSCLE);
	isave->Write(&refID, sizeof DWORD, &nb);
	isave->EndChunk();

	// The object handle
//	refID = isave->GetRefID((void*)node);
//	isave->BeginChunk(CHUNK_NODE);
//	iSave->Write( &refID, sizeof DWORD, &nb);
//	isave->EndChunk();

	return IO_OK;
}

class SegTransPostLoad : public PostLoadCallback {
public:
	SegTrans *pobj;

	SegTransPostLoad(SegTrans *p) { pobj = p; }
	void proc(ILoad *)
	{
		if (pobj && pobj->mpObjectForLoad)
		{
			pobj->mWeakRefToObject.SetRef(pobj->mpObjectForLoad);
			pobj->mpObjectForLoad = nullptr;
		}
		delete this;
	}
};

IOResult SegTrans::Load(ILoad *iload)
{
	DWORD nb = 0L, refID = 0L;
	IOResult res = IO_OK;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID())
		{
		case CHUNK_STRAND_ID:
			res = iload->Read(&nStrandID, sizeof(int), &nb);
			break;
		case CHUNK_SEG_ID:
			res = iload->Read(&nSegID, sizeof(int), &nb);
			break;
		case CHUNK_MUSCLE:
			res = iload->Read(&refID, sizeof DWORD, &nb);
			if (res == IO_OK && refID != (DWORD)-1)
			{
				mpObjectForLoad = nullptr;
				iload->RecordBackpatch(refID, (void**)(&mpObjectForLoad));
			}
			break;
			//			case CHUNK_NODE:
			//				res = iload->Read(&refID, sizeof DWORD, &nb);
			//				if (res == IO_OK && refID != (DWORD)-1)
			//					iload->RecordBackpatch(refID, (void**)&node);
			//				break;
		}
		if (res != IO_OK)  return res;
		iload->CloseChunk();
	}
	iload->RegisterPostLoadCallback(new SegTransPostLoad(this));
	return IO_OK;
}

/*
// For the procedural spine sub-objet selection
int SegTrans::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
		GetCATMuscleSystemInterface(object)->Display(t, inode, vpt, flags);

	return Control::Display(t, inode, vpt, flags);
};
void SegTrans::GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
		GetCATMuscleSystemInterface(object)->GetWorldBoundBox(t,inode, vpt, box);
	Control::GetWorldBoundBox(t,inode, vpt, box);
};
int  SegTrans::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			return GetCATMuscleSystemInterface(object)->HitTest(t, inode, type, crossing, flags, p, vpt);
	else	return Control::HitTest(t, inode, type, crossing, flags, p, vpt);
};
void SegTrans::ActivateSubobjSel(int level, XFormModes& modes){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->ActivateSubobjSel(level, modes);
	else	Control::ActivateSubobjSel(level, modes);
};
void SegTrans::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->SelectSubComponent(hitRec, selected, all, invert);
	else	Control::SelectSubComponent(hitRec, selected, all, invert);
};
void SegTrans::ClearSelection(int selLevel){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->ClearSelection(selLevel);
	else	Control::ClearSelection(selLevel);
};
int  SegTrans::SubObjectIndex(CtrlHitRecord *hitRec){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			return	GetCATMuscleSystemInterface(object)->SubObjectIndex(hitRec);
	else	return	Control::SubObjectIndex(hitRec);
};
void SegTrans::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object)->GetSpineFK())
			GetCATMuscleSystemInterface(object)->SelectSubComponent(hitRec, selected, all, invert);
	else	Control::SelectSubComponent(hitRec, selected, all, invert);
};

void SegTrans::GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->GetSubObjectCenters(cb, t, node);
	else	Control::GetSubObjectCenters(cb, t, node);
};
void SegTrans::GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->GetSubObjectTMs(cb, t, node);
	else	Control::GetSubObjectTMs(cb, t, node);
};
void SegTrans::SubMove( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin ){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->SubMove(t, partm, tmAxis, val, localOrigin);
	else	Control::SubMove(t, partm, tmAxis, val, localOrigin);
};
void SegTrans::SubRotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin ){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->SubRotate(t, partm, tmAxis, val, localOrigin);
	else	Control::SubRotate(t, partm, tmAxis, val, localOrigin);
};
void SegTrans::SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin){
	if(GetCATMuscleSystemInterface(object) && GetCATMuscleSystemInterface(object))
			GetCATMuscleSystemInterface(object)->SubScale(t, partm, tmAxis, val, localOrigin);
	else	Control::SubScale(t, partm, tmAxis, val, localOrigin);
};
*/

//////////////////////////////////////////////////////////////////////////////

INode* SegTrans::BuildNode(Object* object, int nStrandID, int nSegID, INode* nodeParent)
{
	this->mWeakRefToObject.SetRef(object);
	this->nStrandID = nStrandID;
	this->nSegID = nSegID;

	INode *node = GetCOREInterface()->CreateObjectNode(object);
	node->SetTMController(this);

	if (nodeParent) {
		nodeParent->AttachChild(node, FALSE);
		node->SetWireColor(nodeParent->GetWireColor());
	}

	//////////////////////////////////////////////////////////////////////
	// user Properties.
	// these will be used by the runtime libraries
	TSTR key = _T("CATMuscleProp_SegU");
	node->SetUserPropInt(key, nStrandID);
	key = _T("CATMuscleProp_SegV");
	node->SetUserPropInt(key, nSegID);
	//////////////////////////////////////////////////////////////////////

	return node;
}

INode* SegTrans::BuildNode(Object* object, int id, INode* nodeParent)
{
	this->mWeakRefToObject.SetRef(object);
	this->nStrandID = id;
	this->nSegID = -1;

	INode *node = GetCOREInterface()->CreateObjectNode(object);
	node->SetTMController(this);

	if (nodeParent) {
		nodeParent->AttachChild(node, FALSE);
		node->SetWireColor(nodeParent->GetWireColor());
	}
	//////////////////////////////////////////////////////////////////////
	// user Properties.
	// these will be used by the runtime libraries
	TSTR key = _T("CATMuscleProp_SphereID");
	node->SetUserPropInt(key, nStrandID);
	//////////////////////////////////////////////////////////////////////

	return node;
}