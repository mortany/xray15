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

class PivotPos : public CATGraph {
private:

	float Liftval;
	float Plantval;

	float Lifttime;
	float Planttime;

public:

	enum { PBLOCK_REF };

	enum {
		PB_LIMBDATA,
		PB_CATBRANCH,
		PB_KEY1TIME,
		PB_KEY1VAL,
		PB_KEY2TIME,
		PB_KEY2VAL
	};

	//From Animatable
	Class_ID ClassID() { return PIVOTPOS_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_PIVOTPOS); }
	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	PivotPos();
	~PivotPos();

	void Copy(Control *ctrl) { UNREFERENCED_PARAMETER(ctrl); };
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 4; };
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

	//	float GetGraphYval(TimeValue t, int LoopT) { return GetYval(t, LoopT); };
	float GetYval(TimeValue t, int LoopT);
	//	COLORREF GetGraphColour();
	//	TSTR GetGraphName();

};

extern ClassDesc2* GetPivotPosDesc();
