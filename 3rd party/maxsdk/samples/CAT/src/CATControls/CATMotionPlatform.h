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
class FootTrans2;
class CATMotionLimb;

class CATMotionPlatform : public CATMotionController {
private:
	ULONG flagsBegin;
	IObjParam *ip;
	Matrix3 m_ptm;

	//////////////////////////////////////////////////////////////////////////
	// these flags are turned on and off while footprint positinos get calculated
	BOOL calculating_footprints;

	// Used in GetFootPrintPos for animated footprints
	Matrix3 tmPrint1;

public:

	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};

	enum {
		PB_FOOTTRANS,
		PB_CATMOTIONLIMB,
		PB_P3CATOFFSETPOS,
		PB_P3CATMOTIONPOS,
		PB_P3CATOFFSETROT,
		PB_FOLLOW_PATH,

		// CAT V1203
		PB_PIVOTPOS,
		PB_PIVOTROT,
		PB_STEPSHAPE,
		PB_STEPMASK,
		PB_CAT_PRINT_TM,
		PB_CAT_PRINT_INV_TM,
		PB_PRINT_NODES
	};

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONPLATFORM_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONPLATFORM); }
	//Constructor/Destructor
	CATMotionPlatform();
	~CATMotionPlatform();

	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int) { return pblock; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit); UNREFERENCED_PARAMETER(method);
	}
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags);  return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	//////////////////////////////////////////////////////////////////////////

	void Initialise(int index, class CATNodeControl* iktarget, CATHierarchyBranch2* CATHierarchyLimb, CATMotionLimb* catmotionlimb);

	CATNodeControl *GetIKTargetControl() { return (CATNodeControl*)pblock->GetReferenceTarget(PB_FOOTTRANS); }
	// footprint management
	float GetStepMask(TimeValue t) {
		return pblock->GetFloat(PB_STEPMASK, t);
	};

	Matrix3 GetPrintTM(int i, TimeValue t);
	void UpdateStepMasks();
	BOOL CalcAllPrintPos(BOOL bCreateNodes = FALSE, BOOL bKeepCurrentOffsets = TRUE, BOOL bOnlySelected = FALSE);

	INode *GetPrintNode(int index) { return pblock->GetINode(PB_PRINT_NODES, 0, index); };

	void CreateFootPrints();
	void RemoveFootPrints(BOOL bOnlySelected);
	void ResetFootPrints(BOOL bOnlySelected);

	BOOL SnapToGround(INode *targ, BOOL bOnlySelected);
	BOOL SnapPrintToGround(TimeValue t, INode *footprint, Mesh *grndMesh, const Matrix3 &invGrndTM, const Matrix3 &grndTM);
	void SetFootPrintColour();

	void GetFootPrintPos(TimeValue t, Point3 &pos);
	void GetFootPrintRot(TimeValue t, float &zRot);
	void GetFootPrintMot(TimeValue t, Matrix3 &printMot);
	void CalcFootPrintMot(TimeValue t, float ratio);

	//////////////////////////////////////////////////////////////////////////
	// StepGraphtime
	int GetStepGraphTime(TimeValue t);
	float GetStepGraphRatio(TimeValue t);

	CATMotionLimb* GetCATMotionLimb() { return (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB); };

	void CATMotionMessage(TimeValue t, UINT msg, int data);
	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	void PutPrintTMtoPblock(int printid, TimeValue t);

	void SaveClip(CATRigWriter *save, int flags, Interval timerange);
	BOOL LoadClip(CATRigReader *load, Interval range, int flags);

	//////////////////////////////////////////////////////////////////////////
	//
	Control* GetOwningCATMotionController();
};

extern ClassDesc2* GetCATMotionPlatformDesc();
