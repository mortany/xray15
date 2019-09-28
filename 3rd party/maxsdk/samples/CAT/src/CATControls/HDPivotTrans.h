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

#define HD_PIVOTTRANS_CLASS_ID		Class_ID(0x6e341cba, 0x6f9c52ca)

// Super class for all Pivot Trans Controllers
class PivotTrans : public Control {
protected:
	Matrix3 tm, pivottm;
	Control *prs;
	DWORD dwFileSaveVersion;

public:
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }

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

	virtual DWORD GetInheritanceFlags() { if (prs) return prs->GetInheritanceFlags(); return 0; };
	virtual BOOL  SetInheritanceFlags(DWORD f, BOOL keepPos) { if (prs) return prs->SetInheritanceFlags(f, keepPos); return FALSE; };

	DWORD GetFileSaveVersion() { return dwFileSaveVersion; }
};

class HDPivotTrans : public PivotTrans {
private:
	// Keep track of the panel we are displaying our rollouts on
	int EditPanel;

public:
	// pointers to our controls.
//	Control *prs;
	Control *wspivot;
	Control *pivot;
	//	Matrix3 tm, ptm;

		// I have to use a pblock here so that
		// the Clip Saver will work with this
		// controller and save out the new preset
		// pivot locations.
	IParamBlock2 *pblock;

	static MoveCtrlApparatusCMode *moveMode;
	Box3 bbox;
	DWORD selLevel;
	// Class Interface pointer
	IObjParam *ipbegin;

	enum REFLIST {
		REF_PRS,
		REF_WSPIVOT,
		REF_PIVOT,
		REF_PBLOCK,
		NUMREFS
	};		// our REFERENCE LIST

	enum SUBLIST {
		SUB_PRS,
		SUB_PIVOT,
#ifdef _DEBUG
		SUB_WSPIVOT,
#endif
		SUB_PBLOCK,
		NUMSUBS
	};		// our SUBANIM LIST

	// Pblock ref ID
	enum { pb_id };
	enum {
		PB_PRESET_PIVOT_LOCATIONS
	};
	ULONG hdl;

public:
	//
	// constructors.
	//
	void Init();
	HDPivotTrans(BOOL loading = FALSE);
	~HDPivotTrans();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return HD_PIVOTTRANS_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATHDPivotTrans"); }
	void DeleteThis() { delete this; }
	int NumSubs() { return NUMSUBS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	ParamDimension* GetParamDimension(int) { return defaultDim; }
	BOOL AssignController(Animatable *control, int subAnim);

	// BOOL BypassTreeView() { return true; };

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//
	// from class Control:
	//
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

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

	BOOL ChangeParents(TimeValue t, const Matrix3& oldP, const Matrix3& newP, const Matrix3& tm);

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) {
		if (prs)		prs->CopyKeysFromTime(src, dst, flags);
		if (pivot && flags&COPYKEY_POS)	pivot->CopyKeysFromTime(src, dst, flags);

	}
	void AddNewKey(TimeValue t, DWORD flags) {
		if (prs)		prs->AddNewKey(t, flags);
		if (pivot)	pivot->AddNewKey(t, flags);
	}
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (prs && pivot) return (prs->IsKeyAtTime(t, flags) || pivot->IsKeyAtTime(t, flags)); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (prs) return prs->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (prs) return prs->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (prs) return prs->GetKeySelState(sel, range, flags); return 0; }

	virtual Interval GetTimeRange(DWORD flags);
	virtual void EditTimeRange(Interval range, DWORD flags) { if (prs) prs->EditTimeRange(range, flags); if (pivot) pivot->EditTimeRange(range, flags); }
	virtual void MapKeys(TimeMap *map, DWORD flags) { if (prs) prs->MapKeys(map, flags); if (pivot) pivot->MapKeys(map, flags); };

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

	/**********************************************************************
		* Loading and saving....
		*/

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// Parameter block access
	int NumParamBlocks() { return 0; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }	// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	void UpdateWSPivot(Control* changing);
};
