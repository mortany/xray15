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

#define DIGITSEGTRANS_CLASS_ID	Class_ID(0x1adf2684, 0x2e3b60d2)

class DigitData;
//class DigitSegTransPLCB;
class DigitSegTrans : public CATNodeControlDistributed {
private:
	friend class DigitSegTransPLCB;

	// the nasty hack we use to fix the CAT2 toed moving bug uses thsi tm.
	Matrix3			tmPreToePosHack;

public:

	enum { PBLOCK_REF, LAYERTRANS, NUMREFS };

	enum {
		PB_DIGITDATA,
		PB_BONEID_DEPRECATED,
		PB_BONELENGTH,
		PB_TMSETUP,
		PB_OBJSEGREF,
		PB_NODESEG,

		PB_BENDWEIGHT
	};

	IParamBlock2	*pblock;	//ref 0
public:
	//
	// constructors.
	//
	DigitSegTrans(BOOL loading = FALSE);
	~DigitSegTrans();

	//From Animatable
	Class_ID ClassID() { return DIGITSEGTRANS_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATDigitSegTrans"); /*GetString(IDS_CL_DIGITSEGTRANS);*/ }

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return NUMREFS; }
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);

	// TODO: Maintain the number or references here
	int NumRefs() { return NUMREFS; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 	return (void *)GetDigit();
		else				return CATNodeControl::GetInterface(id);
	}
	BaseInterface* GetInterface(Interface_ID id) { return CATNodeControl::GetInterface(id); }

	/**********************************************************************
	 * Loading and saving....
	 */
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	BOOL LoadRig(CATRigReader *load);
	BOOL SaveRig(CATRigWriter *save);

	//
	// from class Control:
	//
	void Copy(Control *from);

	void	UpdateCATUnits();

	void	GetRotation(TimeValue t, Matrix3* tmLocalRot);
	void	SetRotation(TimeValue t, const Matrix3& tmLocalRot);

	//////////////////////////////////////////////////////////////////////////
	// CATControl

	CATGroup* GetGroup();

	virtual CATControl*	GetParentCATControl();;

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	void UpdateUI();

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	USHORT GetRigID() { return idDigitSegParams; }

	CATNodeControl* FindParentCATNodeControl(void);

	// Stuff for the IK system
	virtual int				NumChildCATNodeControls();
	virtual CATNodeControl*	GetChildCATNodeControl(int i);

	TSTR	GetRigName();
	TSTR	GetBoneAddress();

	void	UpdateUserProps();

	virtual void CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	virtual void ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid);

	// Handle differences introduced by ApplySetupOffset
	virtual void SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method);

	// Similar to BoneSegTrans and TailTrans, Digits have a common width/height for all segments.
	void	SetObjX(float val);	// Digit width
	void	SetObjY(float val);	// Digit height

	//////////////////////////////////////////////////////////////////////////
	// DigitBoneTrans

	float GetBendWeight() { return  pblock->GetFloat(PB_BENDWEIGHT); }

	INode*	Initialise(DigitData* digit, int iSegNum);

	DigitData*	GetDigit();
	void	SetDigit(DigitData* d);

	CatAPI::IBoneGroupManager* GetManager();

	LimbData2*	GetLimbData();
};
