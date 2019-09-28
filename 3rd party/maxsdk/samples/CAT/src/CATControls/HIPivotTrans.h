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

#define HI_PIVOTTRANS_CLASS_ID		Class_ID(0x6e134cba, 0x6f9c52ca)

class CATNodeControl;

class HIPivotTrans : public Control {
private:
	// Keep track of the panel we are displaying our rollouts on
	int EditPanel;

	// pointers to our controls.
	Control *prs;
	Control *pivotpos;
	Control *pivotrot;

	static MoveCtrlApparatusCMode *mMoveMode;
	static RotateCtrlApparatusCMode *mRotateMode;

	// Class Interface pointer
	IObjParam *ipbegin;

	enum REFLIST {
		PRS,
		PIVOTPOS,
		PIVOTROT,
		NUMREFS
	};		// our REFERENCE LIST

public:
	//
	// constructors.
	//
	void Init();
	HIPivotTrans(BOOL loading = FALSE);
	~HIPivotTrans();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return HI_PIVOTTRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATHIPivotTrans"); }
	void DeleteThis() { delete this; }
	int NumSubs() { return NUMREFS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	ParamDimension* GetParamDimension(int) { return defaultDim; }
	BOOL AssignController(Animatable *control, int subAnim);

	// BOOL BypassTreeView() { return true; };

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//
	// from class ReferenceMaker?
	// Do I need to do anything to this??
	// Later a node reference will be added to the
	// controller as well as the subanims
	int NumRefs() { return NUMREFS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);
	BOOL IsLeaf() { return FALSE; }
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	void CommitValue(TimeValue t) { prs->CommitValue(t); }
	void RestoreValue(TimeValue t) { prs->RestoreValue(t); }

	BOOL InheritsParentTransform() { return  (prs ? prs->InheritsParentTransform() : FALSE); }

	// GB 07-Nov-2003: Make key filters on the Time Slider work correctly
	Control *GetPositionController() { return prs->GetPositionController(); }
	Control *GetRotationController() { return prs->GetRotationController(); }
	Control *GetScaleController() { return prs->GetScaleController(); }
	BOOL SetPositionController(Control *c) { return prs->SetPositionController(c); }
	BOOL SetRotationController(Control *c) { return prs->SetRotationController(c); }
	BOOL SetScaleController(Control *c) { return prs->SetScaleController(c); }

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) {
		if (prs)		prs->CopyKeysFromTime(src, dst, flags);
		if (pivotrot)	pivotrot->CopyKeysFromTime(src, dst, flags);

	}
	void AddNewKey(TimeValue t, DWORD flags) {
		if (prs)		prs->AddNewKey(t, flags);
		if (pivotrot)	pivotrot->AddNewKey(t, flags);
	}
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (prs) return prs->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (prs) return prs->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (prs) return prs->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (prs) return prs->GetKeySelState(sel, range, flags); return 0; }

	virtual DWORD GetInheritanceFlags() { if (prs) return prs->GetInheritanceFlags(); return 0; };
	virtual BOOL  SetInheritanceFlags(DWORD f, BOOL keepPos) { if (prs) return prs->SetInheritanceFlags(f, keepPos); return FALSE; };

	// From control -- for apparatus manipulation
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void ActivateSubobjSel(int level, XFormModes& modes);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all);
	void ClearSelection(int selLevel);
	int SubObjectIndex(CtrlHitRecord *hitRec);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert);

	//int NumSubObjects(TimeValue t,INode *node);
	//void GetSubObjectTM(TimeValue t,INode *node,int subIndex,Matrix3& tm);
	//Point3 GetSubObjectCenter(TimeValue t,INode *node,int subIndex,int type);
	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node);
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node);

	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE);
	/**********************************************************************
	 * Loading and saving....
		Note: Nothing is Saved in CAT 3.5, all parameters are obsolete
	 */

	 // Parameter block access
	int NumParamBlocks() { return 0; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return NULL; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID) { return NULL; } // return id'd ParamBlock

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	void UpdateWSPivot(Control* changing);

	CATNodeControl* GetACATNodeControl();
};
