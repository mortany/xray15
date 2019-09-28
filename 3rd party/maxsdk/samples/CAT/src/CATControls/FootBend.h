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

// Bend in the feet segments, bezier curve 0->amount->0 as liftTime->PeakTime->plantTime

#pragma once

#include "CATGraph.h"
#include <CATAPI/CATClassID.h>

class FootBend : public CATGraph
{
public:

	//		float dFlipVal;

	enum { PBLOCK_REF };
	enum { FootBend_params };

	// enums for various parameters
	enum {
		PB_LIMBDATA, PB_CATBRANCH,

		PB_KEY1VAL, PB_KEY1TIME, PB_KEY1TANGENT, PB_KEY1INTANLEN, PB_KEY1OUTTANLEN,
		PB_KEY2VAL, PB_KEY2TIME, PB_KEY2TANGENT, PB_KEY2INTANLEN, PB_KEY2OUTTANLEN,

		PB_KEY3VAL, PB_KEY3TIME, PB_KEY3TANGENT, PB_KEY3INTANLEN, PB_KEY3OUTTANLEN,
		PB_KEY4VAL, PB_KEY4TIME, PB_KEY4TANGENT, PB_KEY4INTANLEN, PB_KEY4OUTTANLEN,

	};

	//From Animatable
	Class_ID ClassID() { return FOOTBEND_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_FOOTBEND); }

	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	FootBend();
	~FootBend();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 4; };

	void GetCATKey(const int			i,
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

	COLORREF GetGraphColour();
	TSTR GetGraphName();

	float GetGraphYval(TimeValue t, int LoopT);
	float GetYval(TimeValue t, int LoopT);

};

extern ClassDesc2* GetFootBendDesc();
