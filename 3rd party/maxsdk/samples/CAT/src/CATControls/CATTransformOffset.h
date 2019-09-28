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

/**********************************************************************
	ICATTransformOffsetFP:
	This controller controls all Matrix3 Transform values in CAT
 */
#define I_CATTRANSFORMOFFSET_FP		Interface_ID(0x4ccf4504, 0x53f45167)
class ICATTransformOffsetFP : public FPMixinInterface {
public:

	virtual BitArray*	IGetFlags() = 0;
	virtual void		ISetFlags(BitArray *ba_flags) = 0;

	virtual INode*		GetCoordSysNode() = 0;
	virtual void		SetCoordSysNode(INode* n) = 0;

	enum {
		propGetFlags,
		propSetFlags,
		propGetCoordSysNode,
		propSetCoordSysNode
	};

	BEGIN_FUNCTION_MAP
		PROP_FNS(propGetFlags, IGetFlags, propSetFlags, ISetFlags, TYPE_BITARRAY);
		PROP_FNS(propGetCoordSysNode, GetCoordSysNode, propSetCoordSysNode, SetCoordSysNode, TYPE_INODE);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_CATTRANSFORMOFFSET_FP); }
	static FPInterfaceDesc* GetFnPubDesc();
};

#define CAT_TRANSFORM_OFFSET_CONTROL_CLASS_ID		Class_ID( 0x53957e33, 0)
extern ClassDesc2* GetCATTransformOffsetDesc();

#define CATTMOFFSET_FLAG_APPLY_OFFSET_IN_XY_PLANE		(1<<1)//2
#define CATTMOFFSET_FLAG_APPLY_OFFSET_NODE_COORDSYS		(1<<2)//2

class CATTransformOffset : public Control, public ICATTransformOffsetFP {
private:
	// Keep track of the panel we are displaying our rollouts on
	int EditPanel;

	DWORD	flags;
	DWORD	dwFileSaveVersion;

	INode	*coordsys_node;

public:
	// pointers to our controls.
	Control *trans;
	Control *offset;

	// Class Interface pointer
	IObjParam *ipbegin;

	enum REFLIST {
		TRANSFORM,
		OFFSET,
		COORDSYSNODE,
		NUMREFS
	};		// our REFERENCE LIST
	enum SUBLIST {
		SUB_TRANSFORM,
		SUB_OFFSET,
		NUMSUBS
	};		// our subanim LIST

	ULONG hdl;

public:
	//
	// constructors.
	//
	void Init();
	CATTransformOffset(BOOL loading = FALSE);
	~CATTransformOffset();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return CAT_TRANSFORM_OFFSET_CONTROL_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATTransformOffset"); }
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

	void CommitValue(TimeValue t) { offset->CommitValue(t); }
	void RestoreValue(TimeValue t) { offset->RestoreValue(t); }

	BOOL InheritsParentTransform() { return  (trans ? trans->InheritsParentTransform() : FALSE); }

	// GB 07-Nov-2003: Make key filters on the Time Slider work correctly
	Control *GetPositionController() { return offset->GetPositionController(); }
	Control *GetRotationController() { return offset->GetRotationController();; }
	Control *GetScaleController() { return offset->GetScaleController();; }
	BOOL SetPositionController(Control *c) { return offset->SetPositionController(c); }
	BOOL SetRotationController(Control *c) { return offset->SetRotationController(c); }
	BOOL SetScaleController(Control *c) { return offset->SetScaleController(c); }

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) {
		if (offset)	offset->CopyKeysFromTime(src, dst, flags);
	}
	void AddNewKey(TimeValue t, DWORD flags) {
		if (offset)	offset->AddNewKey(t, flags);
	}
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (offset) return offset->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (offset) return offset->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (offset) return offset->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (offset) return offset->GetKeySelState(sel, range, flags); return 0; }

	virtual DWORD GetInheritanceFlags();
	virtual BOOL  SetInheritanceFlags(DWORD f, BOOL keepPos);

	/**********************************************************************
	 * Loading and saving....
	 */

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// Parameter block access
	int NumParamBlocks() { return 0; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return NULL; }	// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID) { return NULL; } // return id'd ParamBlock

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_CATTRANSFORMOFFSET_FP) return (ICATTransformOffsetFP*)this;
		return Control::GetInterface(id);
	}

	/////////////////////////////////////////////////////////////////////////////////////

	void SetFlag(ULONG f, BOOL on = TRUE) { if (on) flags |= f; else flags &= ~f; }
	void ClearFlag(ULONG f) { flags &= ~f; }
	BOOL TestFlag(ULONG f) const { return (flags & f) == f; };

	/////////////////////////////////////////////////////////////////////////////////////
	// ICATTransformOffsetFP

	virtual BitArray*	IGetFlags();
	virtual void		ISetFlags(BitArray *ba_flags);

	virtual INode*		GetCoordSysNode() { return coordsys_node; };
	virtual void		SetCoordSysNode(INode* n) { ReplaceReference(COORDSYSNODE, n); };

};
