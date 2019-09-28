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

#include <CATAPI/CATClassID.h>
#include "CATMotionController.h"

class ECATParent;
class LimbData2;
class CATMotionPlatform;

class CATHierarchyRoot;
class CATHierarchyBranch2;

class CATMotionLimb : public CATMotionController {
public:

	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};

	enum {
		PB_CATPARENT,
		PB_LIMB,
		PB_CATHIERARCHY,
		PB_CATHIERARCHYROOT,
		PB_CATMOTIONPLATFORM_DEPRECATED,
		PB_PHASEOFFSET,
		PB_LIFTPLANTMOD,
		PB_CATMOTION_STEPPING_NODES,
		NUMPARAMS
	};

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONLIMB_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONLIMB); }
	//Constructor/Destructor
	CATMotionLimb();
	~CATMotionLimb();
	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(valid); UNREFERENCED_PARAMETER(method);
	};
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit); UNREFERENCED_PARAMETER(method);
	};
	void Copy(Control *from);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int) { return pblock; }

	// TODO: Maintain the number or references here
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:
	void RefDeleted();

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; };

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

	//////////////////////////////////////////////////////////////////////////
	TimeValue tEvalTime;
	TimeValue tStepTimeEvalTime;
	TimeValue tStepTime;
	float	  dFootStepMask;

	Interval StepTimeRange;

	// Cache of Seg range used on all limbs GetStepTime function
	Interval LastStepRange;
	Interval ThisStepRange;
	Interval NextStepRange;
	int		 iStepNum;
	float	 dStepRatio;

	float segstartv, segendv;
	float phaseStart, phaseEnd;

	void Initialise(int index, class LimbData2* limb, CATHierarchyBranch2* CATHierarchyHub);
	Control* GetOwningCATMotionController();
	void DestructCATMotionHierarchy(LimbData2* pLimb);

	class ICATParentTrans*	GetCATParentTrans();

	LimbData2*	GetLimb() { return (LimbData2*)pblock->GetReferenceTarget(PB_LIMB); }
	int					GetNumCATMotionPlatforms() { return pblock->Count(PB_CATMOTION_STEPPING_NODES); }
	CATMotionPlatform*	GetCATMotionPlatform(int i) { return (CATMotionPlatform*)pblock->GetReferenceTarget(PB_CATMOTION_STEPPING_NODES, 0, i); }
	CATHierarchyRoot*		GetCATMotionRoot() { return (CATHierarchyRoot*)pblock->GetReferenceTarget(PB_CATHIERARCHYROOT); }
	CATHierarchyBranch2*	GetCATHierarchyLimb() { return (CATHierarchyBranch2*)pblock->GetReferenceTarget(PB_CATHIERARCHY); }

	Color	GetLimbColour();
	TSTR	GetLimbName();

	void UpdateHub();

	int			GetStepNum(TimeValue t);
	Interval	GetStepTimeRange(TimeValue t);
	float		GetStepTime(TimeValue t, float ModAmount, int &tOutStepTime, BOOL isStepMasked = TRUE, BOOL maskedbyFootPrints = FALSE);
	float		GetStepRatio() { return dStepRatio; }

	void GetFootPrintPos(TimeValue t, Point3 &pos);
	void GetFootPrintRot(TimeValue t, float &zRot);

	///
	int  GetLMR();
	void SetLeftRight(Matrix3 &inVal);
	void SetLeftRight(float &inVal);
	void SetLeftRightRot(Point3 &inVal);
	void SetLeftRightPos(Point3 &inVal);
	void SetLeftRight(Quat &inVal);

	BOOL GetisLeg();
	BOOL GetisArm();

	float GetPathOffset();
	float GetFootStepMask(TimeValue t);

	void CATMotionMessage(TimeValue t, UINT msg, int data);
	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
};

extern ClassDesc2* GetCATMotionLimbDesc();
