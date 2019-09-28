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

#include "CATNodeControlDistributed.h"

#define TAILTRANS_CLASS_ID	Class_ID(0x4f7c09d6, 0x6c8e29bf)

class TailData2;

class TailTrans : public CATNodeControlDistributed {
private:
public:
	friend class TailTransPLCB;
	friend class TailPLCB;

	IParamBlock2 *pblock;

	enum PARAMLIST {
		PBLOCK_REF,
		LAYERTRANS,
		NUMPARAMS
	};		// our subanim list.

	enum {
		PB_TAILDATA,
	};

public:
	//
	// constructors.
	//
	TailTrans(BOOL loading = FALSE);
	~TailTrans();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return TAILTRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_TAILTRANS); }

	// ST: As a ReferenceTarget TailData will NOT be queried for its
	// SubAnims. Because its got loads of useful data in its param
	// block, we handle returning its subanims our selves here
	int NumSubs() { return NUMPARAMS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	//
	// from class ReferenceMaker?
	// Do I need to do anything to this??
	// Later a node reference will be added to the
	// controller as well as the subanims
	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//
	// from class Control:
	//
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// We have to do just a bit of hacking to fix tails with
	// dangle ratios, and also scaling is a bit whacked.
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	// We need to override the CATNodeControl implimentation here because TailData is Not derrived from
	// Control. See CATNodeControl::GetInterface, and TailData::AsControl();
	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 	return (void *)GetTail();
		else				return CATNodeControl::GetInterface(id);
	}

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idTailBone; }

	CATGroup* GetGroup();

	CATControl*	GetParentCATControl();
	Color GetCurrentColour(TimeValue t);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl
	CATNodeControl*	FindParentCATNodeControl();

	void CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	// Stuff for the IK system
	int				NumChildCATNodeControls();
	CATNodeControl*	GetChildCATNodeControl(int i);

	TSTR	GetRigName();

	// We don't have a real width or height for tails, its a common parameter shared by all members of the tail.
	void	SetObjX(float val);
	void	SetObjY(float val);
	void	SetObjZ(float val);

	/////////////////////////////////////////////////////////////////////////
	// TailTrans Methods

	TailData2* GetTail();
	IBoneGroupManager* GetManager();

	void SetLinkLength(float length);
	INode* Initialise(TailData2 *tail, int linkNum, BOOL loading);
};
