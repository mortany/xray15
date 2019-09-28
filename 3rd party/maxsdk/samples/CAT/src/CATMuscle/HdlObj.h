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

#pragma once

#include "SingleWeakRefMaker.h"
#include <CATAPI/CATClassID.h>

 //------------------------------------------------------

 // in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

class MuscleStrand;

//class Muscle;
class HdlObj : public HelperObject {
private:
	friend class HdlObjPLCB;
	DWORD dVersion;

	static IObjParam *ip;
	static HdlObj *editOb;

	//wrap the parent node.
	INode* mpParentNodeForLoad;//only used for loading!
	MaxSDK::SingleWeakRefMaker mWeakRefToParentNode;

	//ReferenceTarget* mSystem;
	float mSize;

public:
	//  inherited virtual methods for Reference-management
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	HdlObj();
	~HdlObj();

	// From BaseObject
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; };
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);
	const MCHAR *GetObjectName() { return GetString(IDS_CL_HDLOBJ); }

	// From Object
	ObjectState Eval(TimeValue time);
	void InitNodeName(TSTR& s) { s = GetString(IDS_CL_HDLOBJ); }
	int CanConvertToType(Class_ID) { return FALSE; }
	Object* ConvertToType(TimeValue, Class_ID) { DbgAssert(0); return NULL; }
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box);
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box);
	int DoOwnSelectHilite() { return 0; }
	Interval ObjectValidity(TimeValue) { return FOREVER; };
	int UsesWireColor() { return TRUE; }

	// If we say we are a WorldSpaceObject,
	// we will not be instanced (which is not good)
	BOOL IsWorldSpaceObject() { return TRUE; }

	// Animatable methods
	void DeleteThis() { delete this; }
	Class_ID ClassID() { return HDLOBJ_CLASS_ID; }
	void GetClassName(TSTR& s) { s = TSTR(GetString(IDS_CL_HDLOBJ)); }

	// From ref
	RefTargetHandle Clone(RemapDir& remap);
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	// Local methods
	void InvalidateUI();
	int DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt);

	void SetParent(INode* pParent) { mWeakRefToParentNode.SetRef(pParent); }
	void SetSize(float size) { mSize = size; }

private:
	MuscleStrand* GetOwner();
};

ClassDesc* GetHdlObjDesc();

template <class T>
class HdlObjOwnerPLCB : public PostLoadCallback
{
private:
	T* mOwner;
public:
	HdlObjOwnerPLCB(T& owner) : mOwner(&owner) {}

	void proc(ILoad *iload)
	{
		mOwner->SetHandleSize(mOwner->GetHandleSize());
		delete this;
	}

	// Very Low Priority, visual only effect.
	int Priority() { return 9; }
};
