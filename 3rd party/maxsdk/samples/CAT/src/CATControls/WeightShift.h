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
#include <CATAPI/CATClassID.h>

class WeightShift : public CATGraph {
public:

	//TODO: Add enums for various parameters
	enum {
		PB_LIMBDATA,
		PB_CATBRANCH,
		//			PB_AMOUNT, PB_INTANLEN, PB_OUTTANLEN, PB_PHASEOFFSET
		/*	PB_KEY1VAL,	*/	PB_KEY1TIME, PB_KEY1TANGENT, PB_KEY1INTANLEN, PB_KEY1OUTTANLEN,
		PB_KEY2VAL, PB_KEY2TIME, PB_KEY2TANGENT, PB_KEY2INTANLEN, PB_KEY2OUTTANLEN,

	};

	float dFlipVal;

	//From Animatable
	Class_ID ClassID() { return WEIGHTSHIFT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_WEIGHTSHIFT); }
	//Constructor/Destructor
	WeightShift();
	~WeightShift();

	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 2; };

	void GetCATKey(const int				i,
		const TimeValue	t,
		CATKey				&key,
		bool				&isInTan,
		bool				&isOutTan) const;

	void GetGraphKey(
		int iKeyNum, CATHierarchyBranch* ctrlBranch,
		Control** ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control** ctrlSlider);

	float GetYval(TimeValue t, int LoopT);
};

extern ClassDesc2* GetWeightShiftDesc();
