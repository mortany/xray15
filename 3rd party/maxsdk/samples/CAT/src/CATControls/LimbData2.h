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

#include "CATGroup.h"
#include "CATControl.h"
#include "FnPub/ILimbFP.h"
#include "FnPub/IBoneGroupManagerFP.h"

class CATHierarchyBranch2;
class Hub;
class PalmTrans2;
class CollarBoneTrans;
class CATMotionLimb;
class CATClipMatrix3;
class BoneData;

#define LIMBFLAG_ISARM								(1<<1)//2
#define LIMBFLAG_ISLEG								(1<<2)//4
#define LIMBFLAG_LEFT								(1<<3)//8
#define LIMBFLAG_MIDDLE								(1<<4)//16
#define LIMBFLAG_RIGHT								(1<<5)//32
#define LIMBFLAG_SYMLIMBSETUP_ENABLED				(1<<6)//64
#define LIMBFLAG_SYMLIMBMOTION_ENABLED				(1<<7)//128
#define LIMBFLAG_LOCKED_FK							(1<<8)//256
#define LIMBFLAG_LOCKED_IK							(1<<9)//512
// as opposed to being in IK in setup mode
#define LIMBFLAG_SETUP_FK							(1<<10)//1024

#define LIMBFLAG_FFB_WORLDZ							(1<<12)//4096

#define LIMBFLAG_STOP_EVALUATING					(1<<14)//4096
#define LIMBFLAG_DISPAY_FK_LIMB_IN_IK				(1<<16)//4096

#define LIMBFLAG_SOFT_IK_LIMITS						(1<<17)//4096
#define LIMBFLAG_STRETCHY_LIMB						(1<<18)//4096
#define LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS	(1<<19)//4096

class LimbData2 : public CATControl,
	public CATGroup,
	public ILimbFP,
	public IBoneGroupManagerFP {
private:
	friend class LimbPLCB/*_1200*/;
	friend class LimbMotionRollout;

	//////////////////////////////////////////////////////////////////////////
	// These are variables that are saved by limbdata
	// for the rest of the limb to use..
	ULONG			flags;

	//////////////////////////////////////////////////////////////////////////
	Interval mFKTargetValid;
	Interval mIKSystemValid;
	Matrix3	mtmIKTarget, mtmFKTarget, mFKTargetLocalTM, mFKPalmLocalTM;
	Matrix3 mtmFKParent;
	Point3 mp3LimbRoot;
	Point3 mp3PalmLocalVec;

	INode*	upvectornode;

#ifdef SOFTIK
	float	iksoftlimit_min, iksoftlimit_max;
#endif

	// Store the position of this bone
	// Calculated from the pure FK World Solution
	Tab<Matrix3> mdFKBonePosition;

	//	int iNumBones;

		// Keep track of the panel we are displaying our rollouts on
	static int EditPanel;

	AngAxis axIKOffset;
	AngAxis axUpNodeOffset;

	//Point3 rootpos;
	Point3 ikstretch;

	//////////////////////////////////////////////////////////////////////////
	// all our layer controllers
	CATClipValue	*layerRetargeting;
	CATClipValue	*layerIKFKRatio;
	CATClipValue	*layerLimbIKPos;
	// if we do not have an IKTarget we store the animation in this transform controller
	CATClipMatrix3	*layerIKTargetTrans;
	//	CATClipValue	*layerUpVectorWeight;

	INode			*iktarget;
	IParamBlock2	*pblock;

	float mStretch;

public:

	enum { limbBone, ankleBone, toeBone };

	// Pblock ref ID
	enum { pb_idParams };

	// Reference enumerations
	enum {
		LIMBDATAPB_REF,
		REF_WEIGHTS,
		REF_LAYERRETARGETING,
		REF_LAYERIKFKRATIO,
		REF_LAYERLIMBIKPOS,
		REF_IKTARGET,
		REF_LAYERIKTARGETRANS,
		REF_UPVECTORNODE,
		NUMPARAMS,
	};

	// Reference enumerations
	enum SUBLIST {
		SUB_WEIGHTS,
		SUB_LAYERIKFKRATIO,
		SUB_LAYERLIMBIKPOS,
		SUB_LAYERFORCEFEEDBACK,
		SUB_LAYERIKTRANS,
		NUMSUBS
	};

	enum {
		PB_CATPARENT,
		PB_HUB,

		PB_CATHIERARCHY,
		PB_LAYERHIERARCHY,

		PB_COLOUR,
		PB_NAME,
		PB_LIMBID_DEPRECATED,
		PB_LIMBFLAGS,

		PB_CTRLCOLLARBONE,
		PB_CTRLPALM,

		PB_P3ROOTPOS,

		PB_NUMBONES,
		PB_BONEDATATAB,

		PB_LEGWEIGHT,
		PB_LAYERBENDANGLE,			// MotionCapture BendAngles
		PB_LAYERSWIVEL,

		PB_LIMBSCALE,
		PB_CATSCALE,

		PB_PHASEOFFSET,
		PB_LIFTPLANTMOD,

		PB_SYMLIMB_SETUP,
		PB_SYMLIMBID,

		PB_SYMLIMB_MOTION,

		PB_SETUPTARGETALIGN,
#ifdef SOFTIK
		PB_SOFTIKWEIGHT,
#endif
	};

public:
	LimbData2(BOOL loading = FALSE);
	virtual ~LimbData2();

	//
	// from class Control:
	//

	// From Animatable
	virtual Class_ID ClassID() { return LIMBDATA2_CLASS_ID; }
	virtual SClass_ID SuperClassID();
	virtual void GetClassName(TSTR& s) { s = GetString(IDS_CL_LIMBDATA2); }

	virtual int NumSubs() { return NUMSUBS; }
	virtual TSTR SubAnimName(int i);
	virtual Animatable* SubAnim(int i);

	// From ReferenceTarget
	virtual RefTargetHandle Clone(RemapDir &remap);
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);// { return REF_SUCCEED; }

	void CATMessage(TimeValue t, UINT msg, int data = -1);

	int NumRefs() { return NUMPARAMS; }	// keep a spare space...
	RefTargetHandle GetReference(int i); // { return pblock; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg); // { pblock=(IParamBlock2*)rtarg; }
public:

	// Parameter block access
	int NumParamBlocks() { return 1; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }	// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	/*
	 *	LimbMethods
	 */
	void SetNumBones(int i) { SetNumBones(i, FALSE); }
	void SetNumBones(int i, BOOL loading = TRUE);		// { pblock->SetValue(PB_NUMBONES, 0, i); }
	int  GetNumBones() const { return pblock->Count(PB_BONEDATATAB); }
	void CreateLimbBones(BOOL loading);

	CATNodeControl* CreateCollarbone(BOOL loading);
	CATNodeControl* CreatePalmAnkle(BOOL loading);

	CatAPI::INodeControl* CreateCollarbone();
	CatAPI::INodeControl* CreatePalmAnkle();
	BOOL RemoveCollarbone();
	BOOL RemovePalmAnkle();

	virtual CatAPI::INodeControl*		GetBoneINodeControl(int boneid);
	virtual CatAPI::INodeControl*		GetPalmAnkleINodeControl();
	virtual CatAPI::INodeControl*		GetCollarboneINodeControl();

	PalmTrans2* GetPalmTrans();
	BoneData* GetBoneData(int iBoneID);
	CATNodeControl* GetCollarbone();

	void SetPalmTrans(PalmTrans2* pPalm);

	/************************************************************************/
	/* Limb Stuff                                                          */
	/************************************************************************/
	CatAPI::IHub*	GetIHub();
	Hub*		GetHub();
	void		SetHub(Hub* h);

	int			GetLimbID() { return GetBoneID(); }
	void		SetLimbID(int id) { SetBoneID(id); }

	//////////////////////////////////////////////////////////////////////////
	// Limb Flags
	virtual DWORD GetFlags() { return pblock->GetInt(PB_LIMBFLAGS); };
	virtual void  SetFlags(DWORD flags) { pblock->SetValue(PB_LIMBFLAGS, 0, (int)flags); };

	void SetFlag(DWORD f, BOOL on = TRUE) { if (on) { flags |= f; } else { flags &= ~f; }  pblock->SetValue(PB_LIMBFLAGS, 0, (int)flags); };
	void ClearFlag(DWORD f) { flags = flags & ~f; pblock->SetValue(PB_LIMBFLAGS, 0, (int)flags); }
	BOOL TestFlag(DWORD f) const { return (pblock->GetInt(PB_LIMBFLAGS) & f) == f; }

	void SetisArm(BOOL isArm) { isArm ? SetFlag(LIMBFLAG_ISARM) : ClearFlag(LIMBFLAG_ISARM); }
	BOOL GetisArm() { return TestFlag(LIMBFLAG_ISARM); }

	void SetisLeg(BOOL isLeg) { isLeg ? SetFlag(LIMBFLAG_ISLEG) : ClearFlag(LIMBFLAG_ISLEG); }
	BOOL GetisLeg() { return TestFlag(LIMBFLAG_ISLEG); }

	void SetLMR(int nLMR);//{ nLMR == -1 ? (ClearFlag(LIMBFLAG_RIGHT);ClearFlag(LIMBFLAG_MIDDLE); SetFlag(LIMBFLAG_LEFT)) : (nLMR == 1 ? SetFlag(LIMBFLAG_RIGHT) : SetFlag(LIMBFLAG_MIDDLE)); };
	int  GetLMR() { return TestFlag(LIMBFLAG_LEFT) ? -1 : (TestFlag(LIMBFLAG_RIGHT) ? 1 : 0); };

	void SetClipLeftRight();
	void SetLeftRight(Matrix3 &inVal);
	void SetLeftRight(float &inVal);
	void SetLeftRightRot(Point3 &inVal);
	void SetLeftRightPos(Point3 &inVal);
	void SetLeftRight(Quat &inVal);

	/*
	 *	RigLoading saving maintenance
	 */
	void UpdateLimb();
	void InvalidateFKSolution() { mFKTargetValid.SetEmpty(); }

	/**********************************************************************
	 * Loading and saving....
	 */
	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);
	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
	// Initialise function
	// Limb Creation/Maintenance ect.
	BOOL SaveRig(CATRigWriter *save);
	BOOL LoadRig(CATRigReader *load);

	void SetIsArmIsLeg(BOOL isArm, BOOL isLeg);

	void Initialise(Hub *hub, int id, BOOL loading, ULONG flags = 0);

	//////////////////////////////////////////////////////////////////////////
	void KeyFreeform(TimeValue t, ULONG flags = KEY_ALL);
	void KeyLimbBones(TimeValue t, bool bKeyFreeform = true);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	virtual void UpdateUI();

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	//////////////////////////////////////////////////////////////////////////////////////
	// 1.3 Functions
	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	///////////////////////////////////////////////////
	// Traversal Functions

	CATControl* AsCATControl() { return this; }

	Color GetGroupColour() { return pblock->GetColor(PB_COLOUR); };
	void  SetGroupColour(Color clr) { pblock->SetValue(PB_COLOUR, 0, clr); };

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idLimb; }

	CATGroup* GetGroup() { return this; };

	int NumLayerControllers() { return 6; };
	CATClipValue* GetLayerController(int i);

	int		NumChildCATControls();

	CATControl*	GetChildCATControl(int i);
	CATControl*	GetParentCATControl();

	virtual void DestructAllCATMotionLayers();

	TSTR	GetBoneAddress();
	INode*	GetBoneByAddress(TSTR address);

	BOOL PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap);

	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	virtual TSTR GetName() { return pblock->GetStr(LimbData2::PB_NAME); }
	virtual void SetName(TSTR newname, BOOL quiet = FALSE) {
		if (newname != GetName()) {
			if (quiet) DisableRefMsgs();
			pblock->SetValue(PB_NAME, 0, newname);
			if (quiet) EnableRefMsgs();
			if (!quiet) CATMessage(GetCOREInterface()->GetTime(), CAT_NAMECHANGE);
		}
	};

	///////////////////////////////////////////
	//ILimb
	ILimb*	GetSymLimb();

	// This is used to temporarily switch the system into IK and give it a point to solve to.
	// The palm uses this to allow the user to click and drag that hand (similar to IK) while
	// in FK, and can then simply store the calculated values in the FK controllers.
	// We need to pass in the FK target (Palm position) because the IK requires an evaluation
	// of the FK solution to generate its solution from.
	// \param tmIKTarget The transform you want the IK Target to reach
	// \param tmFKTarget Where the target would be in pure FK.  This should be the palms current transform
	void	SetTemporaryIKTM(const Matrix3& tmIKTarget, const Matrix3& tmFKTarget);
	void GetIKTMs(TimeValue t, Interval &valid, const Point3& p3LimbRoot, const Matrix3& tmFKParent, Matrix3 &IKTarget, Matrix3 &FKTarget, int boneid = 0);
	bool GetIKAngleFlip(TimeValue t, float dDesiredAngle, const Point3& p3Axis, const Matrix3& tmIkBase, int iBoneID);

	// Tests if the limb can stretch, and would need stretching to reach the given target.
	bool IsLimbStretching(TimeValue t, const Point3& ikTarget);
	void CacheLimbStretch(float v) { mStretch = v; }
	float GetLimbStretchCache() { return mStretch; }

	Point3	GetLimbRoot(TimeValue t);
	Point3	GetBoneFKPosition(TimeValue t, int iBoneID);
	bool GetBoneFKTM(TimeValue t, int iBoneID, Matrix3& tm);

	Ray GetReachVec(TimeValue t, Interval& valid, const Matrix3& tmHub, const Point3& p3HubScale, float &weight);

	float	GetIKFKRatio(TimeValue t, Interval& valid, int boneid = -1);
	void	SetIKFKRatio(TimeValue t, float val);

	float	GetForceFeedback(TimeValue t, Interval& valid);
	void	SetForceFeedback(TimeValue t, float val);

	float	GetLimbIKPos(TimeValue t, Interval& valid);
	void	SetLimbIKPos(TimeValue t, float val);

#ifdef SOFTIK
	BOOL	GetIKSoftLimitEnabled() { return TestFlag(LIMBFLAG_SOFT_IK_LIMITS); };
	void	SetIKSoftLimitEnabled(BOOL on = TRUE) { SetFlag(LIMBFLAG_SOFT_IK_LIMITS, on); };
	BOOL	GetStretchyLimbEnabled() { return TestFlag(LIMBFLAG_STRETCHY_LIMB); };
	void	SetStretchyLimbEnabled(BOOL on = TRUE) { SetFlag(LIMBFLAG_STRETCHY_LIMB, on); };
	BOOL	GetRampReachEnabled() { return TestFlag(LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS); };
	void	SetRampReachEnabled(BOOL on = TRUE) { SetFlag(LIMBFLAG_RAMP_REACH_BY_SOFT_LIMITS, on); GetHub()->Update(); };

	float	GetIKSoftLimitMin() { return iksoftlimit_min; };
	void	SetIKSoftLimitMin(float val);
	float	GetIKSoftLimitMax() { return iksoftlimit_max; };
	void	SetIKSoftLimitMax(float val);
	float	GetDistToTarg(TimeValue t);
#endif

	INode*	CreateUpNode(BOOL loading);
	INode*	CreateUpNode();
	BOOL	RemoveUpNode();
	INode*	GetUpNode() { return upvectornode; };

	float	GetLimbIKLength(TimeValue t, Interval& valid, int frombone, float dLimbIKPos = -1);

	INode*	CreateIKTarget(BOOL loading);
	INode*	CreateIKTarget();
	BOOL	RemoveIKTarget();
	INode*	GetIKTarget() { return iktarget; }

	void MatchIKandFK(TimeValue t);
	void MoveIKTargetToEndOfLimb(TimeValue t);

	void	SelectIKTarget();
	void	SelectLimbEnd();

	Matrix3 GettmIKTarget(TimeValue t, Interval &valid);
	Matrix3 GettmPalmTarget(TimeValue t, Interval &valid, const Matrix3& tmFKPalmLocal);
	Matrix3 GettmFKTarget(TimeValue t, Interval &valid, const Matrix3* tmIKChainParent, Matrix3& tmFKPalmLocal);

	void GetBoneTM(int iBoneID, TimeValue t, Matrix3& tmWorld, Interval& ivValid);
	void GetPalmTM(int iBoneID, TimeValue t, Matrix3& tmWorld, Interval& ivValid);

	//	float	GetSetupTargetAlign(){ return pblock->GetFloat(PB_SETUPTARGETALIGN); }

	void	SetUpNodeOffsetAX(AngAxis axOffset) { axUpNodeOffset = axOffset; };
	AngAxis GetUpNodeOffsetAX() { return axUpNodeOffset; };
	void	SetIKOffsetAX(AngAxis axOffset) { axIKOffset = axOffset; };
	AngAxis GetIKOffsetAX() { return axIKOffset; };

	Point3	GetIKStretch() { return ikstretch; }
	void	SetIKStretch(Point3 s) { ikstretch = s; }

	// this is a special loadclip so that it doesn't try and mirror the data onto another limb
	virtual BOOL LoadClipData(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	/////////////////////////////////////////////////////////////////////////
	// LimbData
	// Method used only by this class

	void PostCloneManager();

	//////////////////////////////////////////////////////////////////
	// Published Functions
	//
	BaseInterface* GetInterface(Interface_ID id);

	virtual FPInterfaceDesc* GetDescByID(Interface_ID id);
};
