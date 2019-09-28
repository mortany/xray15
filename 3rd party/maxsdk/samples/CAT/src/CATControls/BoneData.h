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

#include "CATNodeControl.h"
#include "FnPub/IBoneGroupManagerFP.h"

class LimbData2;
class CATHierarchyBranch2;
class BoneDataPLCB;

//
//	Static IDs and Strings for theBoneData class.
//
#define BONEDATA_CLASS_ID	Class_ID(0x1f781be7, 0x19707e49)

#define REFMSG_USER_TEST 0x1a716c96

class BoneData : public CATNodeControl, public IBoneGroupManagerFP {
private:
	friend class BoneDataPLCB;

	// Cached stuff
	Matrix3 FKTM, IKTM;
	float mBendRatio;

	// Cached FK results
	Matrix3 mLocalFK;
	Point3	mLocalScale;

	// the INode validity interval may be different to that of the FK solution
	Interval validIK, validFK;

	float mdChildTwistAngle;
	float mdParentTwistAngle;
	Interval mTwistValid;

	IParamBlock2	*pblock;

	// We store an Evaluating switch
	// to prevent our BoneSegTrans from
	// looping when calculating twist.
	// The problem is that when the child evaluates,
	// it calls ApplyParentOffset on the parent
	// (BoneSegTrans), which then resets the
	// transform to remove twist.
	bool mIsEvaluating;

public:

	enum {
		pb_idParams
	};

	// Reference enumeration
	enum PARAMLIST {
		BONEDATAPB_REF,
		LAYERTRANS,
		NUMPARAMS
	};

	// Sub enumeration
	enum SUBLIST {
		SUBLIMB,
		SUBTRANS,
		NUMSUBS
	};

	enum {
		PB_LIMBDATA,
		PB_BONEID_DEPRECATED,
		PB_BONELENGTH,
		PB_BONEWIDTH,
		PB_BONEDEPTH,
		PB_TMSETUP_DEPRECATED,
		PB_CHILD_LENGTHS_DEPRECATED,
		PB_NUMSEGS,
		PB_SEGNODE_TAB_DEPRECATED,
		PB_SETUP_BONERATIO_DEPRECATED,
		PB_ANGLERATIO_DEPRECATED,
		PB_SETUP_SWIVEL_DEPRECATED,
		PB_TWISTWEIGHT,
		PB_TWISTANGLE,
		PB_SEGOBJ_PB_TAB_DEPRECATED,
		PB_SCALE_DEPRECATED,
		PB_NAME,
		PB_SEG_TAB
	};

	//
	// constructors.
	//
	BoneData(BOOL loading = FALSE);
	~BoneData();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return BONEDATA_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_BONEDATA); }
	SClass_ID SuperClassID();

	// In debug mode we can see the pblock.
	int NumSubs() { return NUMSUBS; }

	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	// this guy never gets shown
	BOOL BypassTreeView() { return true; };

	// from class ReferenceMaker?
	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	// Replace the CATNodeControl implementation of RefAdded
	// We are not directly attached to our node, so the CATNodeControl
	// function doesn't work for us.
	void RefDeleted() {}
	void RefAdded(RefMakerHandle rm) {}

	// Parameter block access
	int NumParamBlocks() { return 1; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }	// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);
	void PostCloneManager() { PostCloneNode(); }

	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	int		Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	////////////////////////////////////////////////////////////
	//	Loading and saving....
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	////////////////////////////////////////////////////////////

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idLimbBone; }

	CATGroup*	GetGroup();

	CATControl* GetParentCATControl();
	int			NumChildCATControls();
	CATControl*	GetChildCATControl(int i);

	int				NumLayerControllers();
	CATClipValue*	GetLayerController(int i);

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	void CATMessage(TimeValue t, UINT msg, int data = -1);

	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);
	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);

	BOOL LoadRig(CATRigReader *load);
	BOOL SaveRig(CATRigWriter *save);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	CatAPI::IBoneGroupManager* GetManager();
	CATNodeControl* FindParentCATNodeControl();

	void	SetObjX(float val) { pblock->SetValue(PB_BONEWIDTH, 0, val); };
	void	SetObjY(float val) { pblock->SetValue(PB_BONEDEPTH, 0, val); };
	void	SetObjZ(float val) { pblock->SetValue(PB_BONELENGTH, 0, val); };

	float	GetObjX() { return pblock->GetFloat(PB_BONEWIDTH); };
	float	GetObjY() { return pblock->GetFloat(PB_BONEDEPTH); };
	float	GetObjZ() { return pblock->GetFloat(PB_BONELENGTH); };

	virtual void	UpdateObject();

	BOOL IsThisBoneSelected();

	void UpdateCATUnits();
	void UpdateObjDim();

	TSTR GetName() { return pblock->GetStr(PB_NAME); };
	void SetName(TSTR newname, BOOL quiet = FALSE) {
		if (newname != GetName()) {
			if (quiet) DisableRefMsgs();
			pblock->SetValue(PB_NAME, 0, newname);
			if (quiet) EnableRefMsgs();
		};
	}
	TSTR GetBoneAddress();

	void UpdateVisibility() {};// NOOP: Let BoneSegTrans do this..

	virtual void SetTransformLock(int type, BOOL onOff);

	// Stuff for the IK system
	virtual int				NumChildCATNodeControls();
	virtual CATNodeControl*	GetChildCATNodeControl(int i);

	// we actually want bone seg trans to so the
	virtual void SetLengthAxis(int axis);

	int				GetNumArbBones() { return 0; }
	CATNodeControl* AddArbBone(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return NULL; };
	void			RemoveArbBone(int arbboneID) { UNREFERENCED_PARAMETER(arbboneID); };
	void			InsertArbBone(int id, CATNodeControl *arbbone) { UNREFERENCED_PARAMETER(id); UNREFERENCED_PARAMETER(arbbone); };
	CATNodeControl* GetArbBone(int i) { UNREFERENCED_PARAMETER(i); return  NULL; }
	void			CleanUpArbBones() {};

	//////////////////////////////////////////////////////////////////////////
	// ILimbBone

	LimbData2*		GetLimbData();
	void			SetLimbData(LimbData2* l);

	float CalculateTwistAngle(Matrix3 tmLocal);

	void CalcInheritance(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);
	void ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid);
	void CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmValue, Point3& p3LocalScale, Interval& ivLocalValid);
	Matrix3 FixTwistForPalm(const Matrix3& tmBoneWorld, LimbData2* pLimb);

	void GetFKValue(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmValue, Point3& p3LocalScale, Interval& ivValid);
	void GetIKValue(TimeValue t, const Point3& p3CurrPos, const Matrix3& tmParent, Matrix3& tmValue, Point3& p3LocalScale, Interval& ivValid);

	const Matrix3& GetFKTM(TimeValue t) { UNUSED_PARAM(t); DbgAssert(validFK.InInterval(t)); return FKTM; }
	const Matrix3& GetIKTM(TimeValue t) { UNUSED_PARAM(t); DbgAssert(validIK.InInterval(t)); return IKTM; }

	float GetTwistAngle(int nSegID, Interval& ivValid);
	float GetTwistWeight(float SegRatio);

	void CollapseBone();

	void Update();

	void DeleteBone();

	void UpdateNumSegBones(BOOL loading = FALSE);
	void CreateSegBones(int newNumSegs, int iNumSegs, BOOL bLoading);
	void DeleteSegBones(int newNumSegs, int iNumSegs);

	void SetBendRatio(float ratio) { mBendRatio = ratio; }

	//////////////////////////////////////////////////////////////////////////
	// IBoneGroupManager
	CATNodeControl* GetBone(int id);

	CatAPI::INodeControl* GetBoneINodeControl(int id);
	void SetNumBones(int nNumSegs) { pblock->SetValue(PB_NUMSEGS, 0, nNumSegs); };
	int  GetNumBones() const { return pblock->Count(PB_SEG_TAB); };

	//////////////////////////////////////////////////////////////////////////
	// non-inherited methods
	void Initialise(LimbData2* ctrlLimbData, int id, BOOL loading);

	// Is this BoneData currently in the CalcWorldTransform function?
	bool IsEvaluating() const { return mIsEvaluating; }

	BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == BONEGROUPMANAGER_INTERFACE_FP) return static_cast<IBoneGroupManagerFP*>(this);
		else return CATNodeControl::GetInterface(id);
	}
};
