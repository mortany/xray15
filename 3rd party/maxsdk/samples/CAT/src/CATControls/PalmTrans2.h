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

// Takes Matrix passed and uses it to find position. Also has IK/FK blending.

#pragma once

#include "CATNodeControl.h"

class DigitData;

class PalmTrans2 : public CATNodeControl, public CATGroup {
public:

	friend class PalmTransPLCB;
	friend class PalmIKPivotMoveRestore;

	CATClipMatrix3	*layerIKTargetTrans;

private:

	Matrix3 tmPalmTarget;
	Interval validFK, validIK;

	Point3 ikpivotoffset;

public:

	// This matrix is the alignment of the finger in IK mode
	////////////////////////////////////////////////////////////////

	// Scripted object pb enumeration
	enum {
		PLMOBJ_PB_PALMTRANS,
		PLMOBJ_PB_LENGTH,
		PLMOBJ_PB_WIDTH,
		PLMOBJ_PB_DEPTH,
		PLMOBJ_PB_CATUNITS
	};

	// Pblock ref ID
	enum { pb_idParams };

	// ref list
	enum PARAMLIST {
		PALMPB_REF,
		LAYERIKTM,
		LAYERTRANS,
		WEIGHTS,
		NUMPARAMS
	};

	// our subanim list.
	enum SUBLIST {
		SUBLIMB,
		SUBROTATION,
		SUBTARGETALIGN,
		NUMSUBS
	};

	enum {
		PB_LIMBDATA,

		PB_NODE_DEPRECATED,
		PB_OBJPB_DEPRECATED,

		PB_SCALE_DEPRECATED,
		PB_IKNODE_DEPRECATED,
		PB_IKFKRATIO_DEPRECATED,
		//		PB_KAWEIGHT,

		PB_SETUPTARGETALIGN_DEPRECATED,
		PB_LAYERTARGETALIGN,

		PB_TMSETUP_DEPRECATED,
		PB_CATDIGITBEND,
		PB_NUMDIGITS,
		PB_DIGITDATATAB,
		PB_SYMPALM,

		PB_NAME
	};

public:
	IParamBlock2	*pblock;

	//
	// constructors.
	//
	PalmTrans2(BOOL loading = FALSE);
	~PalmTrans2();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return PALMTRANS2_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_PALMTRANS2); }

	int NumSubs() { return NUMSUBS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	// Parameter block access
	int NumParamBlocks() { return 1; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }	// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	//
	// from class ReferenceMaker?
	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	int SubNumToRefNum(int subNum) { return subNum; };

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	//
	// from class Control:
	//
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	//*********************************************************************
	// Loading and saving....
	//
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//////////////////////////////////////////////////////////////////////////
	// Sub Object Gizmo methods
	static MoveCtrlApparatusCMode *moveMode;
	static RotateCtrlApparatusCMode *rotateMode;
	Box3 bbox;
	DWORD selLevel;

	// Our own method for drawing the gizmo to the screen
	void DrawGizmo(TimeValue t, GraphicsWindow *gw);

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void ActivateSubobjSel(int level, XFormModes& modes);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all);
	void ClearSelection(int selLevel);
	int SubObjectIndex(CtrlHitRecord *hitRec);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert = FALSE);

	//int NumSubObjects(TimeValue t,INode *node);
	//void GetSubObjectTM(TimeValue t,INode *node,int subIndex,Matrix3& tm);
	//Point3 GetSubObjectCenter(TimeValue t,INode *node,int subIndex,int type);
	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node);
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node);

	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);

	///////////////////////////////////////////////////
	// CATGroup Functions

	CATControl* AsCATControl() { return this; }

	Color GetGroupColour();
	void  SetGroupColour(Color clr);

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idPalm; }
	CATGroup* GetGroup() { return this; };

	int NumLayerControllers();
	CATClipValue* GetLayerController(int i);

	CATControl* GetParentCATControl();
	int		NumChildCATControls();
	CATControl*	GetChildCATControl(int i);

	virtual INode*	GetBoneByAddress(TSTR address);
	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	// manual UI upate, cause parammap->Update dont do stuff...
	void UpdateUI();
	virtual void Update();

	virtual void Initialise(CATControl* owner, BOOL loading);

	// Limb Creation/Maintenance ect.
	BOOL SaveRig(CATRigWriter *save);
	BOOL LoadRig(CATRigReader *load);

	// Limb Creation/Maintenance ect.
	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);
	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	CATNodeControl*			FindParentCATNodeControl();

	virtual int				NumChildCATNodeControls();
	virtual CATNodeControl*	GetChildCATNodeControl(int i);

	TSTR GetName();
	void SetName(TSTR newname, BOOL quiet = FALSE);
	TSTR GetRigName();

	void KeyFreeform(TimeValue t, ULONG flags = KEY_ALL);

	void UpdateUserProps();

	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	//////////////////////////////////////////////////////////////////////////
	// IPalmAnkle
	//virtual Matrix3		GetValue(TimeValue t, Matrix3 tmBoneParent, Point3 p3BoneLocalScale);
	//virtual void ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale) {};
	virtual void ApplyChildOffset(TimeValue /*t*/, Matrix3& /*tmChild*/) const {};

	// A palm/ankle works in quite a strange way, in that its transform is calculated relative to its parent, but its
	// setup position value is applied relative to its IK target (its not possible to dislocate the palm at the moment.
	virtual void ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid);
	virtual void CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid);

	virtual void SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method);

	void ModifyIKTMParent(TimeValue t, Matrix3 tmIKTarget, Matrix3& tmParent);
	void GetFKValue(TimeValue t, Interval& ivLocalValid, const Matrix3& tmParent, Matrix3& tmWorld, Point3& p3LocalScale);
	void GetIKValue(TimeValue t, const Point3& p3PalmTarget, const Matrix3& tmFKWorld, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid);

	LimbData2*	GetLimb() { return (LimbData2*)pblock->GetReferenceTarget(PB_LIMBDATA); };
	void		SetLimb(LimbData2* l) { pblock->SetValue(PB_LIMBDATA, 0, (ReferenceTarget*)l); };

	// this is the transformation matrix that the palm looks at while in IK
	// the last bone also needs this matrix and hence this method
	Matrix3 GetPalmTarget() { return tmPalmTarget; }

	void	SetNumDigits(int nNumDigits);// { pblock->SetValue(PB_NUMDIGITS, 0, nNumDigits); };
	int		GetNumDigits() { return pblock->Count(PB_DIGITDATATAB); };
	void	CreateDigits();

	DigitData*	GetDigit(int id);
	void		SetDigit(int id, DigitData* newdigit);

	void	RemoveDigit(int nDyingDigit, BOOL deletenodes = TRUE);
	void	InsertDigit(int id, DigitData* digit);

	float GetTargetAlign(TimeValue t, Interval& ivValid);
	void	SetTargetAlign(TimeValue t, float val) { pblock->SetValue(PB_LAYERTARGETALIGN, t, val); }

	void ModifyIKTM(TimeValue t, Interval &iktargetvalid, const Matrix3& tmFKLocal, Matrix3 &tmIKTarget);
	void	ModifyFKTM(TimeValue t, Matrix3 &tmFKTarget);

	Matrix3 CalcPalmRelToIKTarget(TimeValue t, Interval& ivValid, const Matrix3& tmIKTarget);

	float	GetExtensionLength(TimeValue t, const Matrix3 &tmPalmFKLocal);

	//////////////////////////////////////////////////////////////////////////
	// Published Functions
	BaseInterface* GetInterface(Interface_ID id);
	virtual FPInterfaceDesc* GetDescByID(Interface_ID id);

	//////////////////////////////////////////////////////////////////////////
	// Helper fns
private:

	CATClipValue* GetLayerTargetAlign() { return (CATClipValue*)pblock->GetControllerByID(PB_LAYERTARGETALIGN); };
};
