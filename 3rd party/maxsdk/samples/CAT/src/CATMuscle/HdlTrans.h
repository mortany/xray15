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

#include <CATAPI/CATClassID.h>

//
//	HdlTrans.
//
//class Muscle;
class HdlTrans : public Control {
private:
	DWORD dVersion;
public:

	Control *mPRS;

	enum REFS {
		PRSREF,
		NUMREFS
	};

	HdlTrans(BOOL loading = FALSE);
	~HdlTrans();

	Class_ID ClassID() { return HDLTRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_HDLTRANS); }
	void DeleteThis() { delete this; }
	int NumSubs() { return NUMREFS; }
	Animatable* SubAnim(int i) { return (i == PRSREF) ? mPRS : NULL; };
	TSTR SubAnimName(int i) { return (i == PRSREF) ? GetString(IDS_TRANSFORM) : _T(""); };
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	BOOL AssignController(Animatable *control, int subAnim);

	IOResult Save(ISave *save);
	IOResult Load(ILoad *load);
	//
	// from class ReferenceMaker?
	//
	int NumRefs() { return 1; };								// Only reference pblock...
	RefTargetHandle GetReference(int) { return mPRS; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { mPRS = (Control*)rtarg; };
public:
	RefResult NotifyRefChanged(const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	void Copy(Control *from);

	void* GetInterface(ULONG id);
	RefTargetHandle Clone(RemapDir& remap);

	BOOL IsLeaf() { return FALSE; }
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// Sub-Controllers
	Control *GetPositionController() { return mPRS ? mPRS->GetPositionController() : NULL; };
	Control *GetRotationController() { return mPRS ? mPRS->GetRotationController() : NULL; };
	Control *GetScaleController() { return mPRS ? mPRS->GetScaleController() : NULL; };
	BOOL SetPositionController(Control *c) { return mPRS ? mPRS->SetPositionController(c) : FALSE; };
	BOOL SetRotationController(Control *c) { return mPRS ? mPRS->SetRotationController(c) : FALSE; };
	BOOL SetScaleController(Control *c) { return mPRS ? mPRS->SetScaleController(c) : FALSE; };
	BOOL ChangeParents(TimeValue t, const Matrix3& oldP, const Matrix3& newP, const Matrix3& tm) { return mPRS ? mPRS->ChangeParents(t, oldP, newP, tm) : FALSE; }

	void CommitValue(TimeValue t) { mPRS->CommitValue(t); }
	void RestoreValue(TimeValue t) { mPRS->RestoreValue(t); }

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) { if (mPRS) mPRS->CopyKeysFromTime(src, dst, flags); }
	void AddNewKey(TimeValue t, DWORD flags) { if (mPRS) mPRS->AddNewKey(t, flags); }

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return FALSE; }				// can we be copied and pasted?
	virtual BOOL CanInstanceController() { return FALSE; };

	virtual BOOL BypassTreeView() { return TRUE; }
	virtual BOOL BypassTrackBar() { return TRUE; }
	virtual BOOL BypassPropertyLevel() { return TRUE; }

	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; } // { if(subNum == 0) return PBLOCK_REF; else return -1; };

	/////////////////////////////////////////////////////////////////////

	INode* BuildNode(ReferenceTarget *system, INode* nodeparent = NULL,
		BOOL lockXPos = FALSE, BOOL lockYPos = FALSE, BOOL lockZPos = FALSE,
		BOOL lockXRot = FALSE, BOOL lockYRot = FALSE, BOOL lockZRot = FALSE);

	ReferenceTarget* GetOwner();
};

// our global instance of our classdesc class.
extern ClassDesc2* GetHdlTransDesc();
