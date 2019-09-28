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

#define CATROT_USEORIENT		(1<<1)
#define CATROT_INHERITORIENT	(1<<2)

class CATNodeControl;
class CATMotionLimb;
class CATMotionRot : public CATMotionController {
public:

	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};

	enum {
		PB_LIMBDATA,
		PB_P3CATOFFSET,
		PB_P3CATMOTION,
		PB_FLAGS,
		PB_TMOFFSET,
	};

	Control* ctrlCATOffset;
	Control* ctrlCATRotation;
	int flags;

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONROT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONROT); }
	//Constructor/Destructor
	CATMotionRot();
	~CATMotionRot();

	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }
	void RefDeleted();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

	int NumSubs() { return 2; }
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);

	// TODO: Maintain the number or references here
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags);  return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	void SetFlag(USHORT f) { pblock->SetValue(PB_FLAGS, 0, pblock->GetInt(PB_FLAGS) | f); }
	void ClearFlag(USHORT f) { pblock->SetValue(PB_FLAGS, 0, pblock->GetInt(PB_FLAGS) & ~f); }
	BOOL TestFlag(USHORT f) const { return (pblock->GetInt(PB_FLAGS) & f) == f; }

	void InitControls();

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	////////////////////////////////////////////////
	// CATMotionRot functions
	void Initialise(CATMotionLimb* catmotionlimb, CATNodeControl* bone, Control* p3CATOffset, Control* p3CATRotation, int flags);

	CATMotionController* GetOwningCATMotionController() { return static_cast<CATMotionController*>(pblock->GetReferenceTarget(PB_LIMBDATA)); }
};

extern ClassDesc2* GetCATMotionRotDesc();
