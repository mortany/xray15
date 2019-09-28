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

// Controls rate of foot movement through 1 step

#pragma once

#include "CATGraph.h"

#define STEPSHAPE_CLASS_ID	Class_ID(0x18e02664, 0x21d309c3)

class StepShape : public CATGraph {
public:

	// Parameter block param IDs
	enum {
		PB_LIMBDATA,
		PB_CATBRANCH,
		PB_LIFTTANGENT,
		PB_LIFTOUTTANLEN,
		PB_PLANTTANGENT,
		PB_PLANTINTANLEN,
	};

	//From Animatable
	Class_ID ClassID() { return STEPSHAPE_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_STEPSHAPE); }

	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	StepShape();
	~StepShape();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 2; };
	void GetCATKey(const int i,
		const TimeValue	t,
		CATKey &key,
		bool &isInTan, bool &isOutTan) const;

	void GetGraphKey(
		int iKeyNum, CATHierarchyBranch* ctrlBranch,
		Control** ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control** ctrlSlider);

	//		void GetYRange(TimeValue t, float	&minY, float &maxY);
	float GetGraphYval(TimeValue t, int LoopT) { return GetYval(t, LoopT); }
	float GetYval(TimeValue t, int LoopT);
	COLORREF GetGraphColour() { return RGB(150, 100, 50); }
};

extern ClassDesc2* GetStepShapeDesc();
