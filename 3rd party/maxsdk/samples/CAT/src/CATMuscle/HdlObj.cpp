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

// An object to use on corner Nodes for CATMuscels

#include "CATMuscle.h"
#include "MuscleStrand.h"
#include "HdlObj.h"
#include "../DataRestoreObj.h"

class HdlObjObjClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading);  return new HdlObj; }
	const TCHAR *	ClassName() { return GetString(IDS_CL_HDLOBJ); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return HDLOBJ_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }
	void			ResetClassParams(BOOL fileReset) { UNREFERENCED_PARAMETER(fileReset); /*if(fileReset) resetPointParams();*/ }
	const TCHAR*	InternalName() { return _T("HdlObjObj"); }
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static HdlObjObjClassDesc handleObjDesc;
ClassDesc* GetHdlObjDesc() { return &handleObjDesc; }

#define PBLOCK_REF_NO	 0

// The following two enums are transfered to the istdplug.h by AG: 01/20/2002
// in order to access the parameters for use in Spline IK Control modifier
// and the Spline IK Solver

void HdlObj::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	MuscleStrand* pStrand = GetOwner();
	if (pStrand) pStrand->BeginEditParams(ip, flags, prev);
}

void HdlObj::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	MuscleStrand* pStrand = GetOwner();
	if (pStrand) pStrand->EndEditParams(ip, flags, next);
}

HdlObj::HdlObj() : dVersion(0L), mpParentNodeForLoad(nullptr)
{
	mWeakRefToParentNode.SetRef(nullptr);
	mSize = 5.0f;
}

HdlObj::~HdlObj()
{
	DeleteAllRefsFromMe();
}

void HdlObj::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp*, Box3& box)
{
	Matrix3 tm = inode->GetObjectTM(t);

	box = Box3(Point3(0, 0, 0), Point3(0, 0, 0));
	box += Point3(mSize*0.5f, 0.0f, 0.0f);
	box += Point3(0.0f, mSize*0.5f, 0.0f);
	box += Point3(0.0f, 0.0f, mSize*0.5f);
	box += Point3(-mSize*0.5f, 0.0f, 0.0f);
	box += Point3(0.0f, -mSize*0.5f, 0.0f);
	box += Point3(0.0f, 0.0f, -mSize*0.5f);
}

void HdlObj::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	Matrix3 tm;
	tm = inode->GetObjectTM(t);
	Box3 lbox;

	GetLocalBoundBox(t, inode, vpt, lbox);
	box = Box3(tm.GetTrans(), tm.GetTrans());
	for (int i = 0; i < 8; i++) {
		box += lbox * tm;
	}
}

int HdlObj::DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt)
{
	float size = mSize;
	// end handles are half the size
	size /= 2.0f;

	Color color(inode->GetWireColor());

	Interval ivalid = FOREVER;

	Matrix3 tm(1);
	Point3 pt(0, 0, 0);
	Point3 pts[5];

	vpt->getGW()->setTransform(tm);
	tm = inode->GetObjectTM(t);

	vpt->getGW()->setTransform(tm);

	if (!inode->IsFrozen() && !inode->Dependent() && !inode->Selected()) {
		vpt->getGW()->setColor(LINE_COLOR, color);
	}
	else if (inode->IsFrozen())
	{
		vpt->getGW()->setColor(LINE_COLOR, GetUIColor(COLOR_FREEZE));
	}
	else
	{
		vpt->getGW()->setColor(LINE_COLOR, GetUIColor(COLOR_SELECTION));
	}

	// Make the box half the size
	size *= 0.5f;

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

	////////////////////////////////////////////////////////////////
	// Draw a line to our parent
	INode* pParentNode = dynamic_cast<INode*>(mWeakRefToParentNode.GetRef());
	if (pParentNode) {
		pts[0] = Point3(0.0f, 0.0f, 0.0f);
		pts[1] = (pParentNode->GetNodeTM(t, &ivalid) * Inverse(tm)).GetTrans();
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
	}
	//	vpt->getGW()->setRndLimits(limits);

	return 1;
}

int HdlObj::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	UNREFERENCED_PARAMETER(flags);
	Matrix3 tm(1);
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0, 0, 0);

	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(tm);

	tm = inode->GetObjectTM(t);
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	DrawAndHit(t, inode, vpt);

	/*
		if (showAxis) {
			DrawAxis(vpt,tm,axisLength,screenSize);
			}
		gw->setTransform(tm);
		gw->marker(&pt,X_MRKR);
	*/

	gw->setRndLimits(savedLimits);

	if ((hitRegion.type != POINT_RGN) && !hitRegion.crossing)
		return TRUE;
	return gw->checkHitCode();
}

int HdlObj::Display(
	TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	UNREFERENCED_PARAMETER(flags);
	DrawAndHit(t, inode, vpt);
	/*
	Matrix3 tm(1);
	Point3 pt(0,0,0);

	vpt->getGW()->setTransform(tm);
	tm = inode->GetObjectTM(t);

	if (showAxis) {
		DrawAxis(vpt,tm,axisLength,inode->Selected(),inode->IsFrozen());
		}

	vpt->getGW()->setTransform(tm);
	if(!inode->IsFrozen())
		vpt->getGW()->setColor(LINE_COLOR,GetUIColor(COLOR_POINT_OBJ));
	vpt->getGW()->marker(&pt,X_MRKR);
	*/

	return(0);
}

//
// Reference Managment:
//

// This is only called if the object MAKES references to other things.
RefResult HdlObj::NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage message, BOOL)
{
	return REF_SUCCEED;
}

void HdlObj::InvalidateUI()
{
	MuscleStrand* pStrand = GetOwner();
	if (pStrand != NULL)
		pStrand->RefreshUI();
}

ObjectState HdlObj::Eval(TimeValue)
{
	return ObjectState(this);
}

RefTargetHandle HdlObj::Clone(RemapDir& remap)
{
	HdlObj *newob = (HdlObj*)remap.FindMapping(this);
	if (!newob) {
		newob = new HdlObj();
		remap.AddEntry(this, newob);
	}

	INode* pParentNode = dynamic_cast<INode*>(mWeakRefToParentNode.GetRef());
	RefTargetHandle newRefTargetHandle = newob->mWeakRefToParentNode.GetRef();
	if (pParentNode)
		remap.PatchPointer(&newRefTargetHandle, (RefTargetHandle)pParentNode);

	// Copy the size cache.
	newob->mSize = mSize;

	BaseClone(this, newob, remap);
	return(newob);
}

class HdlObjPLCB : public PostLoadCallback {
public:
	HdlObj *pobj;

	HdlObjPLCB(HdlObj *p) { pobj = p; }
	void proc(ILoad *) {

		if (pobj->dVersion < CATMUSCLE_VERSION_0850) {
			ULONG handle;
			Interface *ip = GetCOREInterface();
			pobj->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
			INode* node = ip->GetINodeByHandle(handle);
			if (node &&
				!node->GetParentNode()->IsRootNode() &&
				node->GetParentNode()->GetTMController()->GetInterface(I_MASTER) != node->GetTMController()->GetInterface(I_MASTER)) {
				pobj->mWeakRefToParentNode.SetRef(node->GetParentNode());
			}
		}
		if (pobj && pobj->mpParentNodeForLoad)
		{
			pobj->mWeakRefToParentNode.SetRef(pobj->mpParentNodeForLoad);
			pobj->mpParentNodeForLoad = nullptr;
		}
		delete this;
	}
};

#define HDLOBJ_NODE				0
#define HDLOBJ_MUSCLE			2
//#define HDLOBJ_HANDLE1			4
//#define HDLOBJ_HANDLE2			6
#define HDLOBJ_VERSION			8
#define HDLOBJ_PARENT			6
#define HDLOBJ_SIZE				16

IOResult HdlObj::Load(ILoad *iload)
{
	ULONG nb = 0L, refID = 0L;
	IOResult res = IO_OK;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID())
		{
		case HDLOBJ_VERSION:
			res = iload->Read(&dVersion, sizeof(DWORD), &nb);
			break;
		case HDLOBJ_SIZE:
			// NOTE: Loading the size here is only for safeties sake.  In general,
			// the size will be overwritten by the size loaded in either Muscle or MuscleStrand
			res = iload->Read(&mSize, sizeof mSize, &nb);
			break;
		case HDLOBJ_PARENT:
			res = iload->Read(&refID, sizeof DWORD, &nb);
			if (res == IO_OK)
			{
				mpParentNodeForLoad = nullptr;
				iload->RecordBackpatch(refID, (void**)(&mpParentNodeForLoad));
			}
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new HdlObjPLCB(this));
	return IO_OK;
}

IOResult HdlObj::Save(ISave *isave)
{
	ULONG nb, refID;

	isave->BeginChunk(HDLOBJ_VERSION);
	dVersion = CAT_VERSION_CURRENT;
	isave->Write(&dVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	// The system handle
	if (isave->SavingVersion() < MAX_VERSION_MAJOR)
	{
		MuscleStrand* pStrand = GetOwner();
		isave->BeginChunk(HDLOBJ_MUSCLE);
		refID = isave->GetRefID((void*)pStrand);
		isave->Write(&refID, sizeof DWORD, &nb);
		isave->EndChunk();
	}

	INode* pParentNode = dynamic_cast<INode*>(mWeakRefToParentNode.GetRef());
	if (pParentNode) {
		isave->BeginChunk(HDLOBJ_PARENT);
		refID = isave->GetRefID((void*)pParentNode);
		isave->Write(&refID, sizeof DWORD, &nb);
		isave->EndChunk();
	}

	isave->BeginChunk(HDLOBJ_SIZE);
	isave->Write(&mSize, sizeof mSize, &nb);
	isave->EndChunk();

	return IO_OK;
}

MuscleStrand* HdlObj::GetOwner()
{
	// The references are as follows:
	// MuscleStrand->INode->ThisClass
	MuscleStrand* pStrand = FindReferencingClass<MuscleStrand>(this, 2);
	return pStrand;
}