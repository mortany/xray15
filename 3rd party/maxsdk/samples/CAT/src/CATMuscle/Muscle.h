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

#include "CATMuscle.h"
#include "IMuscle.h"

#include "notify.h"
#include "../DataRestoreObj.h"

#define MUSCLEFLAG_CREATED				(1<<1)
#define MUSCLEFLAG_CLONING				(1<<2)
#define MUSCLEFLAG_DELETING				(1<<3)
#define MUSCLEFLAG_CONVERTING			(1<<4)

#define MUSCLEFLAG_HANDLES_VISIBLE		(1<<6)
#define MUSCLEFLAG_MIDDLE_HANDLES		(1<<8)

#define MUSCLEFLAG_REMOVE_SKEW			(1<<10)
#define MUSCLEFLAG_KEEP_ROLLOUTS		(1<<12)

FPInterfaceDesc* GetCATMuscleFPInterface();

class MuscleBonesStrip {
public:
	Tab <INode*>	tabSegs;
	MuscleBonesStrip() { tabSegs.SetCount(0); }
};

class Muscle : public SimpleObject2, public IMuscle, public ICATMuscleSystemMaster,
	public IDataRestoreOwner<float>,
	public IDataRestoreOwner<int>,
	public IDataRestoreOwner<Color>,
	public IDataRestoreOwner<Axis>,
	public IDataRestoreOwner<Tab<Matrix3>>
{
protected:
	friend class MuscleCreateCallBack;
	friend class MuscleConvertRestore;
	friend class INodePointerRestore;
	friend class CATMusclePLCB;

	DWORD dVersion;

	Color clrColour;
	TSTR strName;
	// left middle right like limbs on CAT
	Axis mirroraxis;
	int lmr;
	float handlesize;

	int flags;

	std::vector<std::vector<Point3>> tabPosCache;

	Tab <INode*>	tabColObjs;
	Tab <Control*>	tabDistortionByColObj;
	Tab <Control*>	tabColObjHardness;
	Tab <int>		tabColObjFlags;

	Matrix3		tmMuscleUp;
	Point3		p3MuscleUp;
	Interval	iMuscleUpValid;

	// keeps track of the panel our rollout is being displayed on
	int flagsBegin;
	// Class Interface pointer
	IObjParam *ipRollout;

	enum REFS {
		HDL_A = 0,
		HDL_AC,
		HDL_AB,

		HDL_B,
		HDL_BD,
		HDL_BA,

		HDL_C,
		HDL_CA,
		HDL_CD,

		HDL_D,
		HDL_DB,
		HDL_DC,

		HDL_ACB,
		HDL_BAD,
		HDL_CAD,
		HDL_DBC,

		NUMNODEREFS
	};

	// Handles
	Tab <INode*>	tabHandles;

	// caches of our handle positions
	Point3	ws_hdl_pos[NUMNODEREFS];
	// if we decide to delete our handles, then we cache our offsets
	Tab<Matrix3>	ls_hdl_tm;

	Interval handlesvalid;

	DEFORMER_TYPE deformer_type;

	// these are used before we are actually created
	int nTempNumUSegs, nTempNumVSegs;

	Tab <MuscleBonesStrip*>	tabStrips;

	INode *node;
	int nNumVSegs;
	int nNumUSegs;

public:

	/////////////////////////////////////////////////////////
	// class IMuscle

	virtual FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual int GetVersion() { return dVersion; }

	virtual TSTR	IGetDeformerType();
	virtual void	ISetDeformerType(const TSTR& s);

	virtual TSTR	GetName() { return strName; };
	virtual void	SetName(const TSTR& newname);

	virtual Color*	GetColour() { return &clrColour; };
	virtual void	SetColour(Color* clr);

	// LMR is used to help in naming and pasting muscles.
	// Left = -1; Middle = 0; Right = 1;
	virtual int		GetLMR() { return lmr; };
	virtual void	SetLMR(int newlmr);

	virtual Axis	GetMirrorAxis() { return mirroraxis; };
	virtual void	SetMirrorAxis(Axis newmirroraxis);

	virtual BOOL	GetHandlesVisible() { return TestFlag(MUSCLEFLAG_HANDLES_VISIBLE); };
	virtual void	SetHandlesVisible(BOOL tf) { if (tf) CreateHandles(); else RemoveHandles(); };

	virtual BOOL	GetMiddleHandles() { return TestFlag(MUSCLEFLAG_MIDDLE_HANDLES); };
	virtual void	SetMiddleHandles(BOOL tf) { if (tf) CreateMiddleHandles(); else RemoveMiddleHandles(); };

	virtual int		GetNumVSegs();// = 0;
	virtual void	SetNumVSegs(int n);// = 0;

	virtual int		GetNumUSegs();// = 0;
	virtual void	SetNumUSegs(int n);// = 0;

	virtual int		GetRemoveSkew() { return TestFlag(MUSCLEFLAG_REMOVE_SKEW); };
	virtual void	SetRemoveSkew(BOOL tf) { return SetFlag(MUSCLEFLAG_REMOVE_SKEW, tf); };

	virtual Tab <INode*>	GetHandles() { return tabHandles; };

	Matrix3 GetLSHandleTM(int id, TimeValue t);
	void SetLSHandleTM(int id, TimeValue t, Matrix3 &tmm, BOOL bMirror);

	// Paste the settings from another muscle onto this muscle
	virtual void	PasteMuscle(ReferenceTarget* pasteRef);

	void SetFlag(USHORT f, BOOL tf = TRUE);// { if(tf) flags |= f; else flags &= ~f; };
	void ClearFlag(USHORT f) { SetFlag(f, FALSE); }
	BOOL TestFlag(USHORT f) const { return (flags & f) == f; };

	// Collision detection stuff
	virtual void	AddColObj(INode *newColObj);
	virtual void	RemoveColObj(int n);
	virtual int		GetNumColObjs() { return tabColObjs.Count(); };
	virtual INode*	GetColObj(int index) { return  (index >= 0 && index < tabColObjs.Count()) ? tabColObjs[index] : NULL; };
	virtual void	MoveColObjUp(int n);
	virtual void	MoveColObjDown(int n);

	virtual float	GetColObjDistortion(int n, TimeValue t, Interval& valid);
	virtual void	SetColObjDistortion(int n, float newVal, TimeValue t);

	virtual float	GetColObjHardness(int n, TimeValue t, Interval& valid);
	virtual void	SetColObjHardness(int n, float newVal, TimeValue t);

	void SetColObjFlag(int flag, BOOL tf, int n);
	BOOL TestColObjFlag(int flag, int n) { return tabColObjFlags[n] & flag; };

	BOOL GetObjXDistortion(int n) { return TestColObjFlag(COLLISIONFLAG_OBJ_X_DISTORTION, n); }
	virtual void SetObjXDistortion(int n, BOOL tf) { SetColObjFlag(COLLISIONFLAG_OBJ_X_DISTORTION, tf, n); }

	BOOL GetInvertCollision(int n) { return TestColObjFlag(COLLISIONFLAG_INVERT, n); }
	virtual void SetInvertCollision(int n, BOOL tf) { SetColObjFlag(COLLISIONFLAG_INVERT, tf, n); }

	BOOL GetSmoothCollision(int n) { return TestColObjFlag(COLLISIONFLAG_SMOOTH, n); }
	virtual void SetSmoothCollision(int n, BOOL tf) { SetColObjFlag(COLLISIONFLAG_SMOOTH, tf, n); }

	/////////////////////////////////////////////////////////
	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack();

	// Class Object
	BOOL HasUVW() { return TRUE; };
	void SetGenUVW(BOOL sw) { UNREFERENCED_PARAMETER(sw); return; };

	// From SimpleObject
	int CanConvertToType(Class_ID obtype);
	Object* ConvertToType(TimeValue t, Class_ID obtype);
	void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist);

	void BuildMesh(TimeValue t);

	BOOL OKtoDisplay(TimeValue t);

	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	ObjectState Eval(TimeValue t) { UpdateMesh(t); return ObjectState(this); };
	Interval ObjectValidity(TimeValue) { return handlesvalid; };

	// From Animatable
	void BeginEditParams(IObjParam  *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	// From SimpleObject
	void RefreshUI();

	// As a world space object, we can't be instanced (we can't do that)
	BOOL IsWorldSpaceObject() { return TRUE; };

	/////////////////////////////////////////////////////////
	// from class ReferenceMaker
	//
	int NumRefs() { return NUMNODEREFS + (tabColObjs.Count() * 3); };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	int NumSubs() { return NUMNODEREFS + (tabColObjs.Count() * 2); }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	BOOL AssignController(Animatable *control, int subAnim);

	int	NumParamBlocks() { return 0; }					// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return NULL; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID) { return NULL; } // return id'd ParamBlock

	void InitNodeName(TSTR&) { GetString(IDS_NODE_NAME_CATMUSCLE); }

	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	RefTargetHandle Clone(RemapDir& remap);

	/////////////////////////////////////////////////////////
	// Class Muscle

	virtual DEFORMER_TYPE	GetDeformerType() { return deformer_type; };
	virtual void			SetDeformerType(DEFORMER_TYPE type);
	virtual void	GetTrans(TimeValue t, int nStrand, int nSeg, Matrix3 &tm, Interval &valid);//=0;
	virtual void	UpdateName();
	virtual void	UpdateColours();
	virtual void	UpdateMuscle();

	void RegisterCallbacks();
	void UnRegisterCallbacks();

	//Constructor/Destructor
	Muscle();
	~Muscle();
	void Init();

	void UpdateHandles(TimeValue t, Interval&valid);
	Point3	GetSegPos(TimeValue t, int vindex, int uindex, Interval&valid);

	BOOL IsCreated() { return TestFlag(MUSCLEFLAG_CREATED); }
	void Create(INode* node);

	virtual void AssignSegTransTMController(INode *node);
	void CreateHandles();
	void RemoveHandles();
	void TurnPointersIntoReferences();

	void CreateMiddleHandles();
	void RemoveMiddleHandles();

	void CreateHandleNode(int refid, INode* nodeparent = NULL);

	float GetHandleSize() { return handlesize; }
	void  SetHandleSize(float sz);

	void SetValue(TimeValue t, SetXFormPacket *ptr, int commit, Control* pOriginator);

	// this method manages cloning all the handles, and all the collision spheres
	void CloneMuscle(Muscle* newob, RemapDir& remap);

	// Loading/Saving
	IOResult LoadMuscle(ILoad *iload);
	IOResult SaveMuscle(ISave *isave);
	virtual IOResult Load(ILoad *iload);
	virtual IOResult Save(ISave *isave);

	/////////////////////////////////////////////////////////
	// From Class Muscle

	BaseInterface* GetInterface(Interface_ID id);

	//////////////////////////////////////////////////////////////////////////
	// From IDataRestoreOwner
	void OnRestoreDataChanged(float val);
	void OnRestoreDataChanged(int val);
	void OnRestoreDataChanged(Axis val);
	void OnRestoreDataChanged(Color /*val*/);
	void OnRestoreDataChanged(Tab<Matrix3> /*val*/);
};