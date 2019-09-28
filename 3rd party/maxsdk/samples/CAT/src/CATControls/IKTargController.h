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

#define IKTARGTRANS_CLASS_ID		Class_ID(0x490324f3, 0x155b0d26)

#include "CATNodeControl.h"

class LimbData2;
class PalmTrans2;
class IKTargPLCB;
class PalmTransPLCB;

class IKTargTrans : public CATNodeControl {
private:
	friend class IKTargPLCB;
	friend class PalmTrans2;
	friend class PalmTransPLCB;

	USHORT mRigID;

	LimbData2 *limb;

public:

	// Sub enumeration
	enum SUBLIST {
		SUBLIMB,
		SUBTRANS,
		NUMSUBS
	};

	enum REF_ENUM {
		LAYERTRANS
	};

	//Constructor/Destructor
	IKTargTrans(BOOL loading);
	~IKTargTrans();

	//From Animatable
	Class_ID ClassID() { return IKTARGTRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_IKTARGET_TRANSFORM); }

	RefTargetHandle Clone(RemapDir &remap);
	void PostCloneNode();

	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return NUMSUBS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return (i == LAYERTRANS) ? mLayerTrans : NULL; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { if (i == LAYERTRANS) mLayerTrans = (CATClipMatrix3*)rtarg; }
public:

	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 			return (void *)this;
		else						return CATNodeControl::GetInterface(id);
	}

	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return mRigID; }

	//	TSTR GetName(){ return _T("IKTarget"); }

	CATGroup* GetGroup() { return limb ? ((CATControl*)limb)->GetGroup() : NULL; };// ((CATControl*)limb);; };

	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);
	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl
	CATNodeControl* FindParentCATNodeControl() { return NULL; };

	TSTR	GetRigName();
	void	GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);
	void	UpdateUserProps();
	void	CATMessage(TimeValue t, UINT msg, int data);

	// No offset for
	void ApplyChildOffset(TimeValue /*t*/, Matrix3& /*tmChild*/) const {};

	//////////////////////////////////////////////////////////////////////////////
	// IKTragetTrans
	void		SetLimbData(LimbData2 *owningLimb);
	LimbData2*	GetLimbData() { return limb; }

	CATControl* GetParentCATControl();
	void		ClearParentCATControl();

	INode* Initialise(LimbData2 *limb, CATClipMatrix3 *layers, USHORT rigID, BOOL loading);
};

extern ClassDesc2* GetIKTargTransDesc();
