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

#define CATMOTIONDIGITROT_CLASS_ID		Class_ID(0x3b4b3f31, 0x393a7822)

class CATNodeControl;
class CATMotionLimb;
class CATMotionDigitRot : public CATMotionController {
public:

	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};

	enum {
		PB_DIGITSEGTRANS,
		PB_P3CATMOTION,
		PB_TMSETUP
	};

	//From Animatable
	SClass_ID SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
	Class_ID ClassID() { return CATMOTIONDIGITROT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONDIGITROT); }
	//Constructor/Destructor
	CATMotionDigitRot();
	~CATMotionDigitRot();

	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }

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

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//////////////////////////////////////////////////////////////////////////
	Control* GetOwningCATMotionController() { return (Control*)pblock->GetControllerByID(PB_P3CATMOTION); }

	void DestructCATMotionHierarchy(LimbData2* pLimb);

	////////////////////////////////////////////////
	// CATMotionDigitRot functions
	void Initialise(CATNodeControl *digitsegtrans, Control* catmotionrot);
};

extern ClassDesc2* GetCATMotionDigitRotDesc();
