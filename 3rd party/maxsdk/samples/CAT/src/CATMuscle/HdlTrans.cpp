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
	HdlTrans is a transform controller that returns the trasformation
	Matrix as calculated by PatchMucle.
	It is a very simple little controller with no Subs or References
 **********************************************************************/////

#include "CATMuscle.h"
#include "MuscleStrand.h"
#include "Muscle.h"
#include "HdlObj.h"
#include "HdlTrans.h"
#include "../DataRestoreObj.h"
 //
 //	HdlTransClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class HdlTransClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }									// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { return new HdlTrans(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_HDLTRANS); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }			// This determins the type of our controller
	Class_ID		ClassID() { return HDLTRANS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("HdlTrans"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }							// returns owning module handle

};

static HdlTransClassDesc HdlTransDesc;
ClassDesc2* GetHdlTransDesc() { return &HdlTransDesc; }

HdlTrans::HdlTrans(BOOL loading)
	: mPRS(NULL), dVersion(0L)
{
	if (!loading) {
		ReplaceReference(PRSREF, (Control*)CreateInstance(CTRL_MATRIX3_CLASS_ID, Class_ID(PRS_CONTROL_CLASS_ID, 0)));
	}

}

void* HdlTrans::GetInterface(ULONG id) {
	if (id == I_MASTER)
		return (void *)GetOwner();
	else
		return Control::GetInterface(id);
}

RefTargetHandle HdlTrans::Clone(RemapDir& remap)
{
	// make a new HdlTrans object to be the clone
	HdlTrans *newctrl = (HdlTrans*)remap.FindMapping(this);
	if (!newctrl) {
		newctrl = new HdlTrans(TRUE);
		remap.AddEntry(this, newctrl);
	}

	// if the controller is cloneing, force the mPRS to clone as well
	newctrl->ReplaceReference(PRSREF, (Control*)remap.CloneRef(mPRS));

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newctrl, remap);

	// Hook ourselves into the new MuscleStrand.  This is quite backwards,
	// because normally the class holding the references is responsible for
	// cloning the references.  However, because we can't explicitly clone
	// nodes (for some reason), the muscle strand has no way to guarantee
	// that its nodes have been cloned when it is created.  To compensate for
	// this, this class needs to hook itself back into muscle strand.
	ReferenceTarget* pOrigOwner = GetOwner();
	if (pOrigOwner != NULL)
	{
		MuscleStrand* pClonedStrand = static_cast<MuscleStrand*>(remap.CloneRef(pOrigOwner));

		// Find this classes node
		INode* pMyNode = FindReferencingClass<INode>(this);
		DbgAssert(pMyNode != NULL); // Assumption - should always be found
		if (pMyNode != NULL)
		{
			INode* pClonedNode = static_cast<INode*>(remap.FindMapping(pMyNode));
			DbgAssert(pClonedNode != NULL); // Assumption: It's cloned before we are.
			for (int i = 0; i < pOrigOwner->NumRefs(); i++)
			{
				if (pOrigOwner->GetReference(i) == pMyNode)
				{
					DbgAssert(pClonedStrand->GetReference(i) == NULL);
					pClonedStrand->ReplaceReference(i, pClonedNode);
					break;
				}
			}
		}
	}

	// now return the new object.
	return newctrl;
}

HdlTrans::~HdlTrans()
{
	// TODO see if this is still a problem
//	if(mSystem) muscle->tabStrips[nStrandID]->tabSegs[nSegID] = NULL;
	DeleteAllRefsFromMe();
}

void HdlTrans::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// aswell.

	}
	else
	{
		//wont be done (i hopes)
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void HdlTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method) {
	if (mPRS) mPRS->SetValue(t, val, commit, method);
};
void HdlTrans::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method) {
	if (mPRS) mPRS->GetValue(t, val, valid, method);
}
void HdlTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) {
	UNREFERENCED_PARAMETER(prev);
	if (mPRS)	mPRS->BeginEditParams(ip, flags, NULL);
}
void HdlTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) {
	UNREFERENCED_PARAMETER(next);
	if (mPRS)	mPRS->EndEditParams(ip, flags, NULL);
}
BOOL HdlTrans::AssignController(Animatable *control, int subAnim) {
	if (subAnim == PRSREF) {
		ReplaceReference(PRSREF, (RefTargetHandle)control, TRUE);
		return TRUE;
	}
	return FALSE;
};

RefResult HdlTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL) {
	switch (msg)
	{
	case REFMSG_TARGET_DELETED:
		if (hTarg == mPRS) mPRS = NULL;
		break;
	}
	return REF_SUCCEED;
};;

#define CHUNK_SEG_ID			0X01
#define CHUNK_STRAND_ID			0X02
#define CHUNK_MUSCLE			0X03
#define CHUNK_NODE				0X04
#define CHUNK_VERSION			0X05

IOResult HdlTrans::Save(ISave *isave)
{
	DWORD nb;

	isave->BeginChunk(CHUNK_VERSION);
	dVersion = CAT_VERSION_CURRENT;
	isave->Write(&dVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult HdlTrans::Load(ILoad *iload)
{
	DWORD nb;
	IOResult res = IO_OK;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID())
		{
		case CHUNK_VERSION:
			res = iload->Read(&dVersion, sizeof(DWORD), &nb);
			break;
		}
		if (res != IO_OK)  return res;
		iload->CloseChunk();
	}
	return IO_OK;
}

// this method will either build a corner handle with no restictions in movement
// or an end handle which can only move in the XY plane
INode* HdlTrans::BuildNode(ReferenceTarget *pOwner, INode* nodeparent/*=NULL*/,
	BOOL lockXPos/*=FALSE*/, BOOL lockYPos/*=FALSE*/, BOOL lockZPos/*=FALSE*/,
	BOOL lockXRot/*=FALSE*/, BOOL lockYRot/*=FALSE*/, BOOL lockZRot/*=FALSE*/)
{
	DbgAssert(pOwner != NULL);
	if (pOwner == NULL)
		return NULL;

	HdlObj *obj = (HdlObj*)CreateInstance(HELPER_CLASS_ID, HDLOBJ_CLASS_ID);
	MuscleStrand* pStrand = static_cast<MuscleStrand*>(pOwner);
	obj->SetSize(pStrand->GetHandleSize());

	INode* node = GetCOREInterface()->CreateObjectNode(obj);
	node->SetTMController(this);

	if (nodeparent) {
		nodeparent->AttachChild(node, FALSE);
		//	if( !((HdlObj*)nodeparent->GetObjectRef()->FindBaseObject())->handle1)
		//		 ((HdlObj*)nodeparent->GetObjectRef()->FindBaseObject())->handle1 = node;
		//	else ((HdlObj*)nodeparent->GetObjectRef()->FindBaseObject())->handle2 = node;
		obj->SetParent(nodeparent);

		if (lockXPos) node->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_X, TRUE);
		if (lockYPos) node->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_Y, TRUE);
		if (lockZPos) node->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_Z, TRUE);

		if (lockXRot) node->SetTransformLock(INODE_LOCKROT, INODE_LOCK_X, TRUE);
		if (lockYRot) node->SetTransformLock(INODE_LOCKROT, INODE_LOCK_Y, TRUE);
		if (lockZRot) node->SetTransformLock(INODE_LOCKROT, INODE_LOCK_Z, TRUE);

		node->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_X, TRUE);
		node->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_Y, TRUE);
		node->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_Z, TRUE);
	}
	return node;
}

ReferenceTarget* HdlTrans::GetOwner()
{
	// The references are as follows:
	// MuscleStrand->INode->ThisClass
	MuscleStrand* pStrand = FindReferencingClass<MuscleStrand>(this, 2);
	if (pStrand != NULL)
		return pStrand;

	Muscle* pMuscle = FindReferencingClass<Muscle>(this, 2);
	return pMuscle;
}