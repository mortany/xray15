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

#include "ICATObject.h"
#include <CATAPI/CATClassID.h>

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

#define AXIS_LENGTH 20.0f
#define ZFACT (float).005;

// The following two enums are transfered to the istdplug.h by AG: 01/20/2002
// in order to access the parameters for use in Spline IK Control modifier
// and the Spline IK Solver

enum {
	iktarget_params
};

enum {
	iktarget_controller, iktarget_cross, iktarget_length, iktarget_width, iktarget_catunits
};

class IKTargetObject : public HelperObject, ICATObject {
protected:
	DWORD			dwFileSaveVersion;

public:
	friend class IKTargetPostLoadCallback;

	static IObjParam *ip;
	static IKTargetObject *editOb;
	IParamBlock2 *pblock2;

	INode *node;
	DWORD flags;

	// Snap suspension flag (TRUE during creation only)
	BOOL suspendSnap;
	Box3 bbox;

	// Old params... these are for loading old files only. Params are now stored in pb2.
	BOOL showAxis;
	float axisLength;

	// For use by display system
	int extDispFlags;

	//  inherited virtual methods for Reference-management
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

	IKTargetObject();
	~IKTargetObject();

	// From BaseObject
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	void SetExtendedDisplay(int flags);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CreateMouseCallBack* GetCreateMouseCallBack();
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);
	const TCHAR *GetObjectName() { return GetString(IDS_CL_IKTARGET_OBJECT); }

	// From Object
	ObjectState Eval(TimeValue time);
	void InitNodeName(TSTR& s) { s = GetString(IDS_CL_IKTARGET_OBJECT); }
	int CanConvertToType(Class_ID) { return FALSE; }
	Object* ConvertToType(TimeValue, Class_ID) { DbgAssert(0); return NULL; }
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box);
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box);
	int DoOwnSelectHilite() { return 1; }
	Interval ObjectValidity(TimeValue t);
	int UsesWireColor() { return TRUE; }

	// Animatable methods
	void DeleteThis() { delete this; }
	Class_ID ClassID() { return IKTARGET_OBJECT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = TSTR(GetString(IDS_CL_IKTARGET_OBJECT)); }
	int IsKeyable() { return 0; }
	int NumSubs() { return 1; }
	Animatable* SubAnim(int) { return pblock2; }
	TSTR SubAnimName(int) { return TSTR(_T("Parameters")); }
	IParamArray *GetParamBlock();
	int GetParamBlockIndex(int id);
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int) { return pblock2; }
	IParamBlock2* GetParamBlockByID(short) { return pblock2; }

	// From ref
	RefTargetHandle Clone(RemapDir& remap);
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock2; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock2 = (IParamBlock2*)rtarg; }
public:

	// Local methods
	void InvalidateUI();
	int DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt);

	/////////////////////////////////////////////////////////////////////////
	// Class ICATObject

	Control* GetTransformController();//{ return (Control*)pblock2->GetReferenceTarget(iktarget_controller); }
	void	 SetTransformController(Control* ctrl) { pblock2->SetValue(iktarget_controller, 0, (ReferenceTarget*)ctrl); };

	Object* AsObject() { return this; }

	float GetX();//{ return pblock2->GetFloat(iktarget_width); }
	float GetY();//{ return pblock2->GetFloat(iktarget_length);	}
	float GetZ();//{ return 0.0f; }

	void SetX(float val);//{ pblock2->SetValue(iktarget_width, 0, val); };
	void SetY(float val);//{ pblock2->SetValue(iktarget_length, 0, val); };
	void SetZ(float val);//{};

	virtual Point3	GetBoneDim() { return Point3(GetX(), GetY(), 0.0f); };
	virtual void	SetBoneDim(Point3 val) { SetX(val.x); SetY(val.y); };

	//	virtual void SetLengthAxis(int axis){ Update(); } //pblock2->SetValue(iktarget_lengthaxis, 0, axis); }
	virtual void SetLengthAxis(int axis) { if (axis == X) flags |= CATOBJECTFLAG_LENGTHAXIS_X; else flags &= ~CATOBJECTFLAG_LENGTHAXIS_X; Update(); }
	virtual int  GetLengthAxis() { return (flags&CATOBJECTFLAG_LENGTHAXIS_X ? X : Z); }

	float GetCATUnits() { return pblock2->GetFloat(iktarget_catunits); }
	void SetCATUnits(float val) { pblock2->SetValue(iktarget_catunits, 0, val); };

	BOOL SaveRig(CATRigWriter *save);
	BOOL LoadRig(CATRigReader *load);

	BOOL CopyMeshFromNode(INode*) { return FALSE; };
	BOOL UseCustomMesh() { return FALSE; }

	// TODO impliment validity intervals on this guys Build mesh
	void Update() { /* ivalid = NEVER;*/	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }

	// this guy doesn't use custom meshs
	BOOL CustomMeshAvailable() { return FALSE; }
	BOOL UsingCustomMesh() { return FALSE; }
	void SetUseCustomMesh(BOOL) {	}

	void SetCross(BOOL tf) { pblock2->SetValue(iktarget_cross, 0, tf); }
	int  GetCross() { return pblock2->GetInt(iktarget_cross); }

	void SetCATMesh(Mesh *catmesh) { UNREFERENCED_PARAMETER(catmesh); }

	void DisplayObject(TimeValue t, INode* inode, ViewExp *vpt, int flags, Color color, Box3 &bbox);

	// From class Interface
	BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_CATOBJECT)
			return (ICATObject*)this;
		BaseInterface* pInterface = BaseInterface::GetInterface(id);
		if (pInterface != NULL)
		{
			return pInterface;
		}
		// return the GetInterface() of its super class
		return HelperObject::GetInterface(id);
	}
};
