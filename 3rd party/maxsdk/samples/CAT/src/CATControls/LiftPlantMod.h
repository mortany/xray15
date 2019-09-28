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

#include "CATGraph.h"

#define LIFTPLANTMOD_CLASS_ID	Class_ID(0x46606abf, 0x2d3676)

class LiftPlantMod : public CATGraph {
public:

	float LIFTPLANTHEIGHT;
	//	enum { PBLOCK_REF };
		// enums for various parameters
	enum { PB_LIMBDATA, PB_CATBRANCH, PB_LIFTPLANTRATIO, PB_CATMOTIONLIMB };

	//
	// constructors.
	//
	LiftPlantMod();
	~LiftPlantMod();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return LIFTPLANTMOD_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_LIFTPLANTMOD); }
	void DeleteThis() { delete this; }

	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 3; };
	void GetCATKey(const int i,
		const TimeValue	t,
		CATKey &key,
		bool &isInTan, bool &isOutTan) const;

	void GetGraphKey(
		int iKeyNum, CATHierarchyBranch* ctrlBranch,
		Control** ctrlTime, float &fTimeVal, float &minTime, float &maxTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control** ctrlSlider);

	float GetYval(TimeValue t, int LoopT);
	COLORREF GetGraphColour();
	TSTR GetGraphName();

	float GetRatio(TimeValue t) { return pblock->GetFloat(PB_LIFTPLANTRATIO, t); };

	// Lift plant mod is the only CATGraph to notReference the CATMtoino limb, as CATMotion Limb references us
	virtual CATMotionLimb*	GetCATMotionLimb();
	virtual void			SetCATMotionLimb(Control* newLimb);
};

extern ClassDesc2* GetLiftPlantModDesc();
