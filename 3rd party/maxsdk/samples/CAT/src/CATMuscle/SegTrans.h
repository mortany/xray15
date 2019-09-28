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
//
//	SegTrans.
//
//	Our SegTrans class implementation.
//
//	Stephen 22 Nov 2002
//

class SegTrans : public Control {
private:
	DWORD dVersion;

public:
	// this stuff is public because of the :AssignSegTransTMController() on MuscleBones
	int nStrandID;
	int nSegID;
	//wrap the object
	Object* mpObjectForLoad;//only used to load files!
	MaxSDK::SingleWeakRefMaker mWeakRefToObject;

	void Init();
	SegTrans(BOOL loading = FALSE);
	~SegTrans();

	Class_ID ClassID() { return SEGTRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_SEGTRANS); }
	void DeleteThis() { delete this; }
	int NumSubs() { return 0; }
	Animatable* SubAnim(int) { return NULL; }
	TSTR SubAnimName(int) { return _T(""); };
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	BOOL AssignController(Animatable *control, int subAnim) { UNREFERENCED_PARAMETER(control); UNREFERENCED_PARAMETER(subAnim); return FALSE; };

	IOResult Save(ISave *save);
	IOResult Load(ILoad *load);
	//
	// from class ReferenceMaker?
	//
	int NumRefs() { return 0; };								// Only reference pblock...
	RefTargetHandle GetReference(int) { return NULL; }
private:
	virtual void SetReference(int, RefTargetHandle) {};
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

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

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return FALSE; }				// can we be copied and pasted?
	virtual BOOL CanInstanceController() { return FALSE; };

	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; } // { if(subNum == 0) return PBLOCK_REF; else return -1; };

	/////////////////////////////////////////////////////////////////////
	INode* BuildNode(Object* object, int nStrandID, int nSegID, INode* nodeParent);
	INode* BuildNode(Object* object, int id, INode* nodeParent);
};

// our global instance of our classdesc class.
extern ClassDesc2* GetSegTransDesc();
