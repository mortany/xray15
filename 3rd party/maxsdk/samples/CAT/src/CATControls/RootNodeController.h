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

#define ROOT_NODE_CTRL_CLASS_ID		Class_ID(0x38095b21, 0xdb065ae)
#define I_ROOT_NODE_CTRL			0x38095b21

#include "CATPlugins.h"
#include "CATClipValues.h"

 /////////////////////////////////////////////////////
 // CATControl
 // Superclass for all controllers in CAT
#define RNFLAG_MATCHLOWESTPOINT				(1<<1)
#define RNFLAG_RESTONGROUND					(1<<2)
//#define CCFLAG_EFFECT_HIERARCHY				(1<<4)
#define RNFLAG_ALLOWKEYFRAMING				(1<<8)
#define RNFLAG_LINEARMOTION					(1<<10)
#define RNFLAG_STATIC_ORIENTATION			(1<<12)

class ICATParentTrans;
class CATClipMatrix3;
class RootNodeCtrl : public Control {
private:
	friend class RootNodeCtrlRolloutData;
	friend class SetTMRestore;
	friend class SetRNFlagRestore;

	class ICATParentTrans *catparenttrans;

	CATClipMatrix3*		layerTrans;

	Matrix3 tmTransform;
	Box3 bbox;
	ULONG flags;
	BOOL bEvaluatingTM;
	Interval clip_range;

	enum refs { LAYERTRANS };

public:

	//From Animatable
	Class_ID ClassID() { return ROOT_NODE_CTRL_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("RootNodeCtrl"); }

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return _T("LayerTrans"); }
	Animatable* SubAnim(int) { return layerTrans; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return layerTrans;; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { layerTrans = (CATClipMatrix3*)rtarg; }
public:

	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 			return (void *)this;
		if (id == I_ROOT_NODE_CTRL) 	return (void *)this;
		return Control::GetInterface(id);
	}

	// this guy never gets shown
//	BOOL BypassTreeView() { return true; };
//	BOOL BypassPropertyLevel() { return true; };

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	RootNodeCtrl(BOOL loading);
	~RootNodeCtrl();

	//
	// from class Control:
	//
	int IsKeyable() { return TRUE; };

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) { if (layerTrans) layerTrans->CopyKeysFromTime(src, dst, flags); }
	void AddNewKey(TimeValue t, DWORD flags) { if (layerTrans) layerTrans->AddNewKey(t, flags); }
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (layerTrans) return layerTrans->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (layerTrans) return layerTrans->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (layerTrans) return layerTrans->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (layerTrans) return layerTrans->GetKeySelState(sel, range, flags); return 0; }

	// GB 07-Nov-2003: Make key filters on the Time Slider work correctly
	Control *GetPositionController() { return layerTrans ? layerTrans->GetPositionController() : NULL; }
	Control *GetRotationController() { return layerTrans ? layerTrans->GetRotationController() : NULL; }
	Control *GetScaleController() { return layerTrans ? layerTrans->GetScaleController() : NULL; }
	BOOL SetPositionController(Control *c) { return layerTrans ? layerTrans->SetPositionController(c) : FALSE; }
	BOOL SetRotationController(Control *c) { return layerTrans ? layerTrans->SetRotationController(c) : FALSE; }
	BOOL SetScaleController(Control *c) { return layerTrans ? layerTrans->SetScaleController(c) : FALSE; }

	// currenty the layer transform does not work ralative to the nodes parent.
	// that is so that it is consistant between transformnode and no tranform node.
	// We don't want the change parent system to do anything.
	virtual BOOL ChangeParents(TimeValue, const Matrix3&, const Matrix3&, const Matrix3&) { return TRUE; };

	BOOL IsLeaf() { return FALSE; }
	void MouseCycleStarted(TimeValue t);
	void MouseCycleCompleted(TimeValue t);

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//////////////////////////////////////////////////////////////////////////
	// RootNodeCtrl

	ICATParentTrans*	GetCATParentTrans() { return catparenttrans; }
	INode*				Initialise(ICATParentTrans* catparenttrans);
	void				CalcRootNodeTransform(TimeValue t);

	void SetFlag(ULONG f, BOOL on = TRUE, BOOL enableundo = TRUE);
	void ClearFlag(ULONG f, BOOL enableundo = TRUE) { SetFlag(f, FALSE, enableundo); }
	BOOL TestFlag(ULONG f) const { return (flags & f) == f; };

	BOOL SaveClip(CATRigWriter* save, int flags, Interval timerange, int layerindex);
	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	void CATMessage(TimeValue t, UINT msg, int data = -1);

	void Update();

	void AddLayerControllers(Tab <Control*>	 &layerctrls);

};

extern ClassDesc2* GetRootNodeCtrlDesc();
