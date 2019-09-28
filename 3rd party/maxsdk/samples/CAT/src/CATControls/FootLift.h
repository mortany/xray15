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

// Determines ratio of maximum foot hight the foot is currently at

#pragma once

#include "CATGraph.h"

#define FOOTLIFT_CLASS_ID	Class_ID(0x2a234435, 0x750465f5)

class FootLift : public CATGraph {
public:

	enum { PBLOCK_REF };

	// enums for various parameters
	enum {
		PB_LIMBDATA,
		PB_CATBRANCH,

		PB_LIFTTANGENT,
		PB_LIFTOUTTANLEN,
		PB_PEAKTIME,
		PB_PEAKTANGENT,
		PB_PEAKINTANLEN,
		PB_PEAKOUTTANLEN,
		PB_PLANTTANGENT,
		PB_PLANTINTANLEN,
		PB_AMOUNT
	};

	Control* ctrlLIFTTANGENT;
	Control* ctrlLIFTOUTTANLEN;

	Control* ctrlPEAKTIME;
	Control* ctrlPEAKTANGENT;
	Control* ctrlPEAKINTANLEN;
	Control* ctrlPEAKOUTTANLEN;

	Control* ctrlPLANTTANGENT;
	Control* ctrlPLANTINTANLEN;

	Control* ctrlAMOUNT;

	float dLiftTangent;
	float dLiftOutlength;

	float dPlantTangent;
	float dPlantInlength;

	float dPeaktime;
	float dPeakVal;
	float dPeaktangent;
	float dPeakinlength;
	float dPeakoutlength;

	//From Animatable
	Class_ID ClassID() { return FOOTLIFT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_FOOTLIFT); }

	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	FootLift();
	~FootLift();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); };

	//
	// from class CATGraph:
	//
	void InitControls();

	int GetNumGraphKeys() { return 3; };
	void GetCATKey(const int i,
		const TimeValue	t,
		CATKey &key,
		bool &isInTan, bool &isOutTan) const;

	//	CATGraphKey* GetCATGraphKey(int iKeyNum);

	void GetGraphKey(
		int iKeyNum, CATHierarchyBranch* ctrlBranch,
		Control** ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control** ctrlSlider);

	void GetYRange(TimeValue t, float	&minY, float &maxY);

	//		COLORREF GetGraphColour();
	//		TSTR GetGraphName();

	float GetGraphYval(TimeValue t, int LoopT) { return GetYval(t, LoopT); };
	float GetYval(TimeValue t, int LoopT);

};

class FootLiftClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new FootLift(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_FOOTLIFT); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return FOOTLIFT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("FootLift"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

extern ClassDesc2* GetFootLiftDesc();
