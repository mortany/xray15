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

class LiftOffset : public CATGraph {

public:

	enum {
		PB_LIMBDATA,
		PB_CATBRANCH,
		PB_LIFTVAL,
		PB_PLANTVAL
	};

	//From Animatable
	Class_ID ClassID() { return LIFTOFFSET_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_LIFTOFFSET); }
	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	LiftOffset();
	~LiftOffset();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 2; };

	void GetCATKey(const int			i,
		const TimeValue		t,
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

//
//	LiftOffsetClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class LiftOffsetClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading);  return new LiftOffset(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_LIFTOFFSET); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }		// This determins the type of our controller
	Class_ID		ClassID() { return LIFTOFFSET_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CAT LiftOffset"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

extern ClassDesc2* GetLiftOffsetDesc();
