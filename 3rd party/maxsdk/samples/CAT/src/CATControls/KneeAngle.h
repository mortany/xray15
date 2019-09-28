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

// Controls the knee angle through lift/plant

#pragma once

#include "CATGraph.h"

#define KNEEANGLE_CLASS_ID	Class_ID(0x40534bdc, 0x3ae34b58)

class KneeAngle : public CATGraph {
private:
	float plantval;

public:

	enum { PBLOCK_REF };

	// Parameter block
//		IParamBlock2	*pblock;	//ref 0
	enum {
		PB_LIMBDATA, PB_CATBRANCH,
		PB_LIFTVALUE, PB_LIFTTANGENT, PB_LIFTINTANLEN, PB_LIFTOUTTANLEN,
		PB_MIDLIFTTIME, PB_MIDLIFTVALUE, PB_MIDLIFTTANGENT, PB_MIDLIFTINTANLEN, PB_MIDLIFTOUTTANLEN,
		PB_PLANTVALUE, PB_PLANTTANGENT, PB_PLANTINTANLEN, PB_PLANTOUTTANLEN,
		PB_MIDPLANTTIME, PB_MIDPLANTVALUE, PB_MIDPLANTTANGENT, PB_MIDPLANTINTANLEN, PB_MIDPLANTOUTTANLEN,
	};

	//From Animatable
	Class_ID ClassID() { return KNEEANGLE_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_KNEEANGLE); }

	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	KneeAngle();
	~KneeAngle();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

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

	//	void GetYRange(TimeValue t, float	&minY, float &maxY);

	//	COLORREF GetGraphColour();
	//	TSTR GetGraphName();

	//	float GetGraphYval(TimeValue t, int LoopT);
	float GetYval(TimeValue t, int LoopT);

};

class KneeAngleClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new KneeAngle(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_KNEEANGLE); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return KNEEANGLE_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("KneeAngle"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

extern ClassDesc2* GetKneeAngleDesc();
