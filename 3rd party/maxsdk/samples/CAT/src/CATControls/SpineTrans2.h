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

// Implements a basic PRS controller, except inherited scale is disregarded.
// CONTROLS:
//   Position
//   Rotation
//   Scale

#pragma once

#include "CATNodeControlDistributed.h"

#define SPINETRANS2_CLASS_ID				Class_ID(0x542e413e, 0x8241aa2)

class SpineData2;
class CatAPI::IBoneGroupManager;

class SpineTrans2 : public CATNodeControlDistributed {
private:

	float			dRotWt;
	float			dPosWt;
	float			dLinkLength;
	float			dSpineLengthRatio;

	Point3 mBoneLocalScale;
	float mSpineStretch;

	Matrix3			mWorldTransform;
public:
	friend class SpineTransPLCB;

	IParamBlock2 *pblock;
	//	enum PARAMLIST {PBLOCK_REF }; // NUMPARAMS};		// our subanim list.

	enum {
		PB_SPINEDATA,
		PB_LINKNUM_DEPRECATED,
		PB_TMSETUP,
		PB_ROTWT,
		PB_POSWT
	};

	enum PARAMLIST {
		PBLOCK_REF,
		LAYERTRANS,
		NUMPARAMS
	};		// our subanim list.

public:
	//
	// constructors.
	//
	SpineTrans2(BOOL loading = FALSE);
	~SpineTrans2();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return SPINETRANS2_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_SPINETRANS2); }

	int NumSubs() { return NUMPARAMS; }
	TSTR SubAnimName(int i);// { return _T(""); } //GetString(IDS_PARAMS); }
	Animatable* SubAnim(int i);// { return NULL; } //pblock; }

	int NumRefs() { return NUMPARAMS; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);// { pblock=(IParamBlock2*)rtarg; }
public:

	//
	// from class ReferenceMaker?
	// Do I need to do anything to this??
	// Later a node reference will be added to the
	// controller as well as the subanims
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	//
	// from class Control:
	//
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);
	void* GetInterface(ULONG id);

	//////////////////////////////////////////////////////////////////////////

	SpineData2* GetSpine();
	const SpineData2* GetSpine() const;
	void	SetSpine(SpineData2* sp);

	CatAPI::IBoneGroupManager* GetManager();

	////////////////////////////////////////////////////////////
	// For the procedural spine sub-objet selection
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);//{										if(GetSpine())return GetSpine()->Display(t, inode, vpt, flags);			else return CATNodeControl::Display(t, inode, vpt, flags);			};

	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);//{								if(GetSpine())		 GetSpine()->GetWorldBoundBox(t,inode, vpt, box);					};
	int  HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);//{	return GetSpine() ? GetSpine()->HitTest(t, inode, type, crossing, flags, p, vpt) : 0;	};
	void ActivateSubobjSel(int level, XFormModes& modes);//{													if(GetSpine())		 GetSpine()->ActivateSubobjSel(level, modes);						};
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert = FALSE);//{								if(GetSpine())		 GetSpine()->SelectSubComponent(hitRec, selected, all);				};
	void ClearSelection(int selLevel);//{																		if(GetSpine())		 GetSpine()->ClearSelection(selLevel);								};
	int  SubObjectIndex(CtrlHitRecord *hitRec);//{																return GetSpine() ? GetSpine()->SubObjectIndex(hitRec) : 0;								};
	//	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert);//{					if(GetSpine())		 GetSpine()->SelectSubComponent(hitRec, selected, all, invert);		};

	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node);//{								if(GetSpine())		 GetSpine()->GetSubObjectCenters(cb, t, node);						};
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node);//{									if(GetSpine())		 GetSpine()->GetSubObjectTMs(cb, t, node);							};
	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);//{		if(GetSpine())		 GetSpine()->SubMove(t, partm, tmAxis, val, localOrigin);			};
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE);//{		if(GetSpine())		 GetSpine()->SubRotate(t, partm, tmAxis, val, localOrigin);			};
	void SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);//{		if(GetSpine())		 GetSpine()->SubScale(t, partm, tmAxis, val, localOrigin);			};
	/////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idSpineLink; }

	CATGroup* GetGroup();
	CATControl*	GetParentCATControl();

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	virtual Color		GetCurrentColour(TimeValue t);
	virtual TSTR		GetRigName();

	virtual void Initialise(CATControl* owner, int id/*=0*/, BOOL loading = TRUE);

	CATNodeControl* FindParentCATNodeControl();

	virtual void	Update();
	virtual void	UpdateObjDim();

	virtual BOOL SaveRig(CATRigWriter *save);
	virtual BOOL LoadRig(CATRigReader *load);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void	UpdateUserProps();

	// Our setup matrix is defined a little differently.
	// The legacy SetupTM (on the paramblock) is still
	// used, because it is not certain that we have a LayerTrans
	// When we finally unify this, we can kill these functions.
	Matrix3 GetSetupMatrix();
	void SetSetupMatrix(Matrix3 tmSetup);

	//////////////////////////////////////////////////////////////////////////
	// Spine evaluation methods.

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// Evaluate current spine bone transform
	Matrix3 CalcRotTM(const Matrix3& tmParent, const Matrix3& tmBaseHub, const Matrix3& tmTipHub, const Point3& p3ScaleIncr, Point3& p3LastBoneScale);
	Matrix3 CalcPosTM(Matrix3 tmParent, AngAxis axPos);
	void SetSpineStretch(float dStretch) { mSpineStretch = dStretch; }

	// Calc the world transform.  For spines, the first bone
	// pre-calculates the entire spine, and remaining bones simply apply the values.
	void CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid);

	// Set the world transform value.  SpineTrans may or may not have
	// have a LayerTrans controller.  We need to overwrite the default
	// implementation to ensure that the value gets set in the correct place
	void SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method);

	///////////////////////////////////////////////////////////////////////
	// ISpineBone
	void UpdateRotPosWeights();

	float GetRotWt() { return dRotWt; };
	float GetPosWt() { return dPosWt; };

	void CreateLayerTransformController();
	void DestroyLayerTransformController();

	virtual int				NumChildCATNodeControls() { return 1 + tabArbBones.Count(); };
	virtual CATNodeControl*	GetChildCATNodeControl(int i);
};
