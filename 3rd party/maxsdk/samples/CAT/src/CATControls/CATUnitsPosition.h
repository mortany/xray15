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

#include "CATPlugins.h"

#define I_CATUNITSCTRL_FP		Interface_ID(0xeaf0fd9, 0x36b80174)

class ICATUnitsCtrlFP : public FPMixinInterface {
public:
	virtual INode*		GetCATParentNode() = 0;
	virtual void		SetCATParentNode(INode* n) = 0;

	enum {
		propGetCATParentNode,
		propSetCATParentNode
	};

	BEGIN_FUNCTION_MAP
		PROP_FNS(propGetCATParentNode, GetCATParentNode, propSetCATParentNode, SetCATParentNode, TYPE_INODE);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_CATUNITSCTRL_FP); }
};

//*********************************************************************
//	CATUnitsCtrl:
//	This controller controlls all Matrix3 Transform values in CAT

class NLAInfo;
class CATUnitsCtrl : public Control, public ICATUnitsCtrlFP {
protected:
	friend class CATUnitsPosRolloutData;
	INode	*catparent_node;
	DWORD	dwVersion;
	ULONG	flagsbegin;
	float	initCATUnits;

	IObjParam *ipbegin;

	Control	*ctrl;

	enum REF_ENUM
	{
		REF_CTRL,
		REF_CATPARENTNODE
	};

public:

	void DeleteThis() { delete this; }

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_CATUNITSCTRL_FP) return (ICATUnitsCtrlFP*)this;
		return Control::GetInterface(id);
	}
	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	void* GetInterface(ULONG id) {
		//	if (id==I_MASTER && REF_CTRL) 	return (void *)REF_CTRL->GetInterface(id);
		return Control::GetInterface(id);
	}

	BOOL BypassTreeView() { return true; };
	BOOL IsLeaf() { return FALSE; }

	int NumSubs() { return 1; }
	Animatable* SubAnim(int i) { return ((i == REF_CTRL) && ctrl) ? ctrl : NULL; }

	int NumRefs() { return 2; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) { if (ctrl) ctrl->CopyKeysFromTime(src, dst, flags); }
	void AddNewKey(TimeValue t, DWORD flags) { if (ctrl) ctrl->AddNewKey(t, flags); }
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (ctrl) return ctrl->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (ctrl) return ctrl->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (ctrl) return ctrl->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (ctrl) return ctrl->GetKeySelState(sel, range, flags); return 0; }

	// From control -- for apparatus manipulation
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) { return ctrl ? ctrl->Display(t, inode, vpt, flags) : 0; };
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box) { if (ctrl)		 ctrl->GetWorldBoundBox(t, inode, vpt, box); };
	int  HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) { return ctrl ? ctrl->HitTest(t, inode, type, crossing, flags, p, vpt) : 0; };
	void ActivateSubobjSel(int level, XFormModes& modes) { if (ctrl)		 ctrl->ActivateSubobjSel(level, modes); };
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all) { if (ctrl)		 ctrl->SelectSubComponent(hitRec, selected, all); };
	void ClearSelection(int selLevel) { if (ctrl)		 ctrl->ClearSelection(selLevel); };
	int  SubObjectIndex(CtrlHitRecord *hitRec) { return ctrl ? ctrl->SubObjectIndex(hitRec) : 0; };
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) { if (ctrl)		 ctrl->SelectSubComponent(hitRec, selected, all, invert); };

	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node) { if (ctrl)		 ctrl->GetSubObjectCenters(cb, t, node); };
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node) { if (ctrl)		 ctrl->GetSubObjectTMs(cb, t, node); };
	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) { if (ctrl)		 ctrl->SubMove(t, partm, tmAxis, val, localOrigin); };
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE) { if (ctrl)		 ctrl->SubRotate(t, partm, tmAxis, val, localOrigin); };
	void SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) { if (ctrl)		 ctrl->SubScale(t, partm, tmAxis, val, localOrigin); };

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir &remap);

	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//////////////////////////////////////////////////////////////////////////
	// CATUnitsPos
//		INode*		Initialise(INode* node);

	DWORD GetVersion() { return dwVersion; }
	void Init();

	// CATUnitsPosFP
	virtual INode*		GetCATParentNode() { return catparent_node; };
	virtual void		SetCATParentNode(INode* n);

};

//*********************************************************************
//	CATUnitsPos:

#define CATUNITSPOS_CLASS_ID		Class_ID(0x41b57ffd, 0x63ec353a)
extern ClassDesc2* GetCATUnitsPosDesc();

class NLAInfo;
class CATUnitsPos : public CATUnitsCtrl {
private:
	Matrix3 tmOffset;
public:

	//From Animatable
	Class_ID ClassID() { return CATUNITSPOS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_POSITION_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATUnitsPos"); } //GetString(IDS_CL_CATUNITSPOS);}

	TSTR SubAnimName(int) { return _T("Position"); }

	CATUnitsPos(BOOL loading);
	~CATUnitsPos();

	// currenty the layer transform does not work ralative to the nodes parent.
	// that is so that it is consistant between transformnode and no tranform node.
	// We don't want the change parent system to do anything.
	virtual BOOL ChangeParents(TimeValue, const Matrix3&, const Matrix3&, const Matrix3&) { return TRUE; };

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);
};

//*********************************************************************
//	CATUnitsScl:

#define CATUNITSSCL_CLASS_ID		Class_ID(0x5a77a3f, 0x56631fa2)
extern ClassDesc2* GetCATUnitsSclDesc();

class NLAInfo;
class CATUnitsScl : public CATUnitsCtrl {
private:
public:

	//From Animatable
	Class_ID ClassID() { return CATUNITSSCL_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_SCALE_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATUnitsScl"); } //GetString(IDS_CL_CATUNITSSCL);}

	TSTR SubAnimName(int) { return _T("Scale"); }

	CATUnitsScl(BOOL loading);
	~CATUnitsScl();

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

};

//*********************************************************************
//	CATUnitsFlt:

#define CATUNITSFLT_CLASS_ID		Class_ID(0x6e0d0962, 0x5ec97892)
extern ClassDesc2* GetCATUnitsSclDesc();

class NLAInfo;
class CATUnitsFlt : public CATUnitsCtrl {
private:
public:

	//From Animatable
	Class_ID ClassID() { return CATUNITSFLT_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATUnitsFloat"); } //GetString(IDS_CL_CATUNITSFLT);}

	TSTR SubAnimName(int) { return _T("Float"); }

	CATUnitsFlt(BOOL loading);
	~CATUnitsFlt();

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

};
