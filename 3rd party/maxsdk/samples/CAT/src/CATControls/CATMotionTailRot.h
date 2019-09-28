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

class CATMotionTail;
class TailTrans;

class CATMotionTailRot : public CATMotionController {
public:
	float pathoffset;
	float stepmask;

	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};

	enum {
		PB_TAILTRANS,
		PB_P3OFFSETROT_DEPRECATED,
		PB_P3MOTIONROT,
		PB_TMOFFSET
	};

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONTAIL_ROT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONTAIL_ROT); }
	//Constructor/Destructor
	CATMotionTailRot();
	~CATMotionTailRot();

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

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; };

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	void Initialise(TailTrans *tailbone, CATMotionTail* catmotiontail);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	//////////////////////////////////////////////////////////////////////////
	CATMotionController* GetOwningCATMotionController() { return static_cast<CATMotionController*>(pblock->GetControllerByID(PB_P3MOTIONROT)); }

	// Remove class from the CATMotion hierarchy
	// This is overridden in CATMotionLimb to allow
	// it to un-weld leaves (something we can't do
	// auto-magically)
	virtual void DestructCATMotionHierarchy(LimbData2* pLimb);
};

extern ClassDesc2* GetCATMotionTailRotDesc();
