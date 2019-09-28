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

#include "BoneData.h"

class BoneData;
class BoneSegTransPLCB;
class BoneDataPLCB;

#define BONESEGTRANS_CLASS_ID	Class_ID(0x485a734c, 0x795557cc)

class BoneSegTrans : public CATNodeControl {
private:
	IParamBlock2	*pblock;	//ref 0

public:
	friend class BoneSegTransPLCB;
	friend class BoneDataPLCB;

	enum { BONEDATAPB_REF };

	// Parameter block bollocks
	enum {
		pb_params
	};

	enum {
		PB_BONEDATA,
		PB_SEGID_DEPRECATED,
	};

	//////////////////////////////////////////////////////////////////////////
	// BoneSegTrans - The following functions may be called by BoneData et al.

	INode* Initialise(BoneData* bonedata, int id, BOOL loading);

	BoneData*	GetBoneData();
	const BoneData* GetBoneData() const { return const_cast<BoneSegTrans*>(this)->GetBoneData(); }
	void		SetBoneData(BoneData* b);

	BoneSegTrans();
	~BoneSegTrans();

	//////////////////////////////////////////////////////////////////////////
	// Everything else is either derived or private.

private:
	//From Animatable
	Class_ID ClassID() { return BONESEGTRANS_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_BONESEGTRANS); }

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int) { return pblock; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i);

	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
	// this guy never gets shown
	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	//
	// from class Control:
	//

	// this is set to false mostly for the Renderware exporter, because it will now export keyframe per frame
	int IsKeyable() { return FALSE; };

	void CommitValue(TimeValue t) { if (GetBoneData()) GetBoneData()->CommitValue(t); }
	void RestoreValue(TimeValue t) { if (GetBoneData()) GetBoneData()->RestoreValue(t); }

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) { if (GetBoneData()) GetBoneData()->CopyKeysFromTime(src, dst, flags); }
	void AddNewKey(TimeValue t, DWORD flags) { if (GetBoneData()) GetBoneData()->AddNewKey(t, flags); }
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (GetBoneData()) return GetBoneData()->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (GetBoneData()) return GetBoneData()->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (GetBoneData()) return GetBoneData()->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (GetBoneData()) return GetBoneData()->GetKeySelState(sel, range, flags); return 0; }

	void CalcBoneDataParent(TimeValue t, Matrix3& tmParent);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);

	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	void* GetInterface(ULONG id)
	{
		if (id == I_CATNODECONTROL) return this; //GetBoneData()->GetInterface(id);
		return CATNodeControl::GetInterface(id);
	}

	int				Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) { return GetBoneData() ? GetBoneData()->Display(t, inode, vpt, flags) : 0; }

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idBoneSeg; }
	virtual CATControl*	GetParentCATControl() { return GetBoneData(); };

	CATGroup* GetGroup();;

	virtual TSTR	GetBoneAddress();

	virtual int				NumLayerControllers() { return tabArbControllers.Count(); };
	virtual CATClipValue*	GetLayerController(int i) { return (i < tabArbControllers.Count()) ? tabArbControllers[i] : NULL; };

	virtual BOOL SaveRig(CATRigWriter *save);
	virtual BOOL LoadRig(CATRigReader *load);

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor) { return CATNodeControl::PasteRig(pastectrl, flags, scalefactor); };

	// KeyFreeform would be called on our BoneData anyway, we don't need to process it
	void KeyFreeform(TimeValue /*t*/, ULONG /*flags=KEY_ALL*/) {};

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	void SetNode(INode* n);

	CATNodeControl* FindParentCATNodeControl();
	int NumChildCATNodeControls() { return 1; };
	CATNodeControl*	GetChildCATNodeControl(int i);

	// From CATNodeControl
	virtual CATClipMatrix3* GetLayerTrans() const { return (GetBoneID() == 0 && GetBoneData()) ? GetBoneData()->GetLayerTrans() : NULL; }

	// Script uses these functions
	virtual Matrix3 GetSetupMatrix() { return GetBoneData() ? GetBoneData()->GetSetupMatrix() : Matrix3::Identity; };
	virtual void SetSetupMatrix(Matrix3 tmSetup) { if (GetBoneData()) GetBoneData()->SetSetupMatrix(tmSetup); };

	virtual void SetLengthAxis(int axis) { UNREFERENCED_PARAMETER(axis); };

	TSTR GetRigName();

	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);

	void	UpdateUserProps();

	BitArray*	GetSetupModeLocks() { return GetBoneData() ? GetBoneData()->GetSetupModeLocks() : nullptr; };
	void		SetSetupModeLocks(BitArray *locks) { GetBoneData()->SetSetupModeLocks(locks); };
	BitArray*	GetAnimationLocks() { return GetBoneData() ? GetBoneData()->GetAnimationLocks() : nullptr; };
	void		SetAnimationLocks(BitArray *locks) { GetBoneData()->SetAnimationLocks(locks); };

	// Pass through limits to BoneData who has actual animation data...
	Point3 IGetMaxPositionLimits() { return GetBoneData()->IGetMaxPositionLimits(); }
	void ISetMaxPositionLimits(Point3 p) { GetBoneData()->ISetMaxPositionLimits(p); }
	Point3 IGetMinPositionLimits() { return GetBoneData()->IGetMinPositionLimits(); }
	void ISetMinPositionLimits(Point3 p) { GetBoneData()->ISetMinPositionLimits(p); }

	Point3 IGetMaxRotationLimits() { return GetBoneData()->IGetMaxRotationLimits(); }
	void ISetMaxRotationLimits(Point3 p) { GetBoneData()->ISetMaxRotationLimits(p); }
	Point3 IGetMinRotationLimits() { return GetBoneData()->IGetMinRotationLimits(); }
	void ISetMinRotationLimits(Point3 p) { GetBoneData()->ISetMinRotationLimits(p); }

	Point3 IGetMinScaleLimits() { return GetBoneData()->IGetMinScaleLimits(); }
	void ISetMinScaleLimits(Point3 p) { GetBoneData()->ISetMinScaleLimits(p); }
	Point3 IGetMaxScaleLimits() { return GetBoneData()->IGetMaxScaleLimits(); }
	void ISetMaxScaleLimits(Point3 p) { GetBoneData()->ISetMaxScaleLimits(p); }

	void CalcInheritance(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);
	void CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid);
	void ApplyChildOffset(TimeValue t, Matrix3& tmChild) const;
	void ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid);
	void CalcPositionStretch(TimeValue t, Matrix3& tmCurrent, Point3 p3Pivot, const Point3& p3Initial, const Point3& p3Final);

	void CollapsePoseToCurrLayer(TimeValue t);

	// Pass through all size-setting operations to BoneData
	virtual void	SetObjX(float val);
	virtual void	SetObjY(float val);
	virtual void	SetObjZ(float val);

	IBoneGroupManager* GetManager() { return GetBoneData(); }

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
};
