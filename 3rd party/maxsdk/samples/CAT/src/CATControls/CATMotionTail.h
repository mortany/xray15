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

class TailData2;
class CATHierarchyBranch2;
class CATMotionLimb;

class CATMotionTail : public CATMotionController {
public:
	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF,
		NUMREFS
	};

	enum {
		//	PB_HUB,
		PB_P3OFFSETROT,
		PB_P3MOTIONROT,
		PB_PHASEOFFSET,
		PB_PHASE_BIAS,
		PB_FREQUENCY,
		PB_CATHIERARCHYBRANCH
		//	PB_TAIL,
	};

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_POINT3_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONTAIL_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONTAIL); }
	//Constructor/Destructor
	CATMotionTail(BOOL loading = FALSE);
	~CATMotionTail();

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

	// TODO: Maintain the number or references here
	int NumRefs() { return NUMREFS; }
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
	BOOL IsLeaf() { return FALSE; };

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);//{ return REF_SUCCEED; };

	////////////////////////////////////////////
	// CATMotion methods

	void Initialise(int index, TailData2* tail, CATHierarchyBranch2* branchHubGroup);
	Control* GetOwningCATMotionController();
	void DestructCATMotionHierarchy(LimbData2* pLimb);

	void RegisterLimb(CATMotionLimb* ctrlNewLimb, CATHierarchyBranch2* branchTail);

	CATHierarchyBranch2* GetCATHierarchyBranch() { return (CATHierarchyBranch2*)pblock->GetReferenceTarget(PB_CATHIERARCHYBRANCH); }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

};

extern ClassDesc2* GetCATMotionTailDesc();
