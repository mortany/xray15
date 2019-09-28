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

#define CATMOTIONHUB2_CLASS_ID		Class_ID(0x1edd686d, 0x45a56eb3)

class Hub;
class LimbData2;
class CATHierarchyRoot;
class CATHierarchyBranch2;

class CATMotionHub2 : public CATMotionController {
public:

	float			pathoffset;
	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};

	enum {
		PB_HUB,
		PB_P3OFFSETROT,
		PB_P3OFFSETPOS,
		PB_P3MOTIONROT,
		PB_P3MOTIONPOS,
		PB_ROOT,
		//			PB_CATMOTION_HUBGROUP,
		PB_CATMOTIONHUB_TAB,
		PB_CATMOTIONLIMB_TAB,
		PB_CATMOTIONTAIL_TAB
	};

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONHUB2_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONHUB2); }
	//Constructor/Destructor
	CATMotionHub2();
	~CATMotionHub2();

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
	BOOL IsLeaf() { return FALSE; };

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	/*
		CATMotion methods
	*/

	// Returns the CATMotionHub of our inspines BaseHub (if those elements exist)
	Control* GetOwningCATMotionController();

	void Initialise(int index, Hub* hub, CATHierarchyRoot *root);
	float GetFootStepMask(TimeValue t);

	CATHierarchyRoot* GetCATHierarchyRoot() { return (CATHierarchyRoot*)pblock->GetReferenceTarget(PB_ROOT); }
	Hub* GetHub() { return (Hub*)pblock->GetReferenceTarget(PB_HUB); }

	void CATMotionMessage(TimeValue t, UINT msg, int data = 0);
	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

};

extern ClassDesc2* GetCATMotionHub2Desc();
