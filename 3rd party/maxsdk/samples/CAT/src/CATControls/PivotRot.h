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

// Controls the foots angle

#pragma once

#include "CATGraph.h"
#include <CATAPI/CATClassID.h>

class PivotRot : public CATGraph {
public:

	enum { PIVOTROT_PB };
	enum { PBLOCK_REF };

	// enums for various parameters
	enum {
		PB_LIMBDATA, PB_CATBRANCH,

		//		PB_PIVOTPOSLIFT, PB_PIVOTPOSPLANT,

		PB_STARTIME, PB_STARTTANGENT, PB_STARTTANLEN,
		PB_KEY1VAL, PB_KEY1TIME, PB_KEY1TANGENT, PB_KEY1INTANLEN, PB_KEY1OUTTANLEN,
		PB_KEY2VAL, PB_KEY2TIME, PB_KEY2TANGENT, PB_KEY2INTANLEN, PB_KEY2OUTTANLEN,
		PB_ENDTIME, PB_ENDTANGENT, PB_ENDTANLEN

	};

	//From Animatable
	Class_ID ClassID() { return PIVOTROT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_PIVOTROT); }

	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }
	//Constructor/Destructor

	PivotRot();
	~PivotRot();

	void Copy(Control *from);

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);

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

	//		COLORREF GetGraphColour();
	//		TSTR GetGraphName();
	//		float GetGraphYval(TimeValue t, int LoopT);

	float GetYval(TimeValue t, int LoopT);

};

extern ClassDesc2* GetPivotRotDesc();
